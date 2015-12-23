/*
 * (C) 2015 see Authors.txt
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "libavutil/avassert.h"
#include "libavutil/pixdesc.h"

#include <windows.h>
#include "dxva_internal.h"
#include "dxva_vp9.h"

void vp9_getcurframe(struct AVCodecContext* avctx, AVFrame** frame)
{
    const VP9SharedContext* h = avctx->priv_data;
    *frame                    = h->frames[CUR_FRAME].tf.f;
}

static void fill_picture_entry(DXVA_PicEntry_VPx *pic,
                               unsigned index, unsigned flag)
{
    av_assert0((index & 0x7f) == index && (flag & 0x01) == flag);
    pic->bPicEntry = index | (flag << 7);
}

static int fill_picture_parameters(const AVCodecContext *avctx, dxva_context *ctx, const VP9SharedContext *h,
                                   DXVA_PicParams_VP9 *pp)
{
    int i;
    const AVPixFmtDescriptor * pixdesc = av_pix_fmt_desc_get(avctx->sw_pix_fmt);

    if (!pixdesc)
        return -1;

    memset(pp, 0, sizeof(*pp));

    fill_picture_entry(&pp->CurrPic, (unsigned)h->frames[CUR_FRAME].tf.f->data[4], 0);

    pp->profile = h->h.profile;
    pp->wFormatAndPictureInfoFlags = ((h->h.keyframe == 0)   <<  0) |
                                     ((h->h.invisible == 0)  <<  1) |
                                     (h->h.errorres          <<  2) |
                                     (pixdesc->log2_chroma_w <<  3) | /* subsampling_x */
                                     (pixdesc->log2_chroma_h <<  4) | /* subsampling_y */
                                     (0                      <<  5) | /* extra_plane */
                                     (h->h.refreshctx        <<  6) |
                                     (h->h.parallelmode      <<  7) |
                                     (h->h.intraonly         <<  8) |
                                     (h->h.framectxid        <<  9) |
                                     (h->h.resetctx          << 11) |
                                     ((h->h.keyframe ? 0 : h->h.highprecisionmvs) << 13) |
                                     (0                      << 14);  /* ReservedFormatInfo2Bits */

    pp->width  = avctx->width;
    pp->height = avctx->height;
    pp->BitDepthMinus8Luma   = pixdesc->comp[0].depth - 8;
    pp->BitDepthMinus8Chroma = pixdesc->comp[1].depth - 8;
    /* swap 0/1 to match the reference */
    pp->interp_filter = h->h.filtermode ^ (h->h.filtermode <= 1);
    pp->Reserved8Bits = 0;

    for (i = 0; i < 8; i++) {
        if (h->refs[i].f->buf[0]) {
            fill_picture_entry(&pp->ref_frame_map[i], (unsigned)h->refs[i].f->data[4], 0);
            pp->ref_frame_coded_width[i]  = h->refs[i].f->width;
            pp->ref_frame_coded_height[i] = h->refs[i].f->height;
        } else
            pp->ref_frame_map[i].bPicEntry = 0xFF;
    }

    for (i = 0; i < 3; i++) {
        uint8_t refidx = h->h.refidx[i];
        if (h->refs[refidx].f->buf[0])
            fill_picture_entry(&pp->frame_refs[i], (unsigned)h->refs[refidx].f->data[4], 0);
        else
            pp->frame_refs[i].bPicEntry = 0xFF;

        pp->ref_frame_sign_bias[i + 1] = h->h.signbias[i];
    }

    pp->filter_level    = h->h.filter.level;
    pp->sharpness_level = h->h.filter.sharpness;

    pp->wControlInfoFlags = (h->h.lf_delta.enabled   << 0) |
                            (h->h.lf_delta.updated   << 1) |
                            (h->h.use_last_frame_mvs << 2) |
                            (0                       << 3);  /* ReservedControlInfo5Bits */

    for (i = 0; i < 4; i++)
        pp->ref_deltas[i]  = h->h.lf_delta.ref[i];

    for (i = 0; i < 2; i++)
        pp->mode_deltas[i]  = h->h.lf_delta.mode[i];

    pp->base_qindex   = h->h.yac_qi;
    pp->y_dc_delta_q  = h->h.ydc_qdelta;
    pp->uv_dc_delta_q = h->h.uvdc_qdelta;
    pp->uv_ac_delta_q = h->h.uvac_qdelta;

    /* segmentation data */
    pp->stVP9Segments.wSegmentInfoFlags = (h->h.segmentation.enabled       << 0) |
                                          (h->h.segmentation.update_map    << 1) |
                                          (h->h.segmentation.temporal      << 2) |
                                          (h->h.segmentation.absolute_vals << 3) |
                                          (0                               << 4);  /* ReservedSegmentFlags4Bits */

    for (i = 0; i < 7; i++)
        pp->stVP9Segments.tree_probs[i] = h->h.segmentation.prob[i];

    if (h->h.segmentation.temporal)
        for (i = 0; i < 3; i++)
            pp->stVP9Segments.pred_probs[i] = h->h.segmentation.pred_prob[i];
    else
        memset(pp->stVP9Segments.pred_probs, 255, sizeof(pp->stVP9Segments.pred_probs));

    for (i = 0; i < 8; i++) {
        pp->stVP9Segments.feature_mask[i] = (h->h.segmentation.feat[i].q_enabled    << 0) |
                                            (h->h.segmentation.feat[i].lf_enabled   << 1) |
                                            (h->h.segmentation.feat[i].ref_enabled  << 2) |
                                            (h->h.segmentation.feat[i].skip_enabled << 3);

        pp->stVP9Segments.feature_data[i][0] = h->h.segmentation.feat[i].q_val;
        pp->stVP9Segments.feature_data[i][1] = h->h.segmentation.feat[i].lf_val;
        pp->stVP9Segments.feature_data[i][2] = h->h.segmentation.feat[i].ref_val;
        pp->stVP9Segments.feature_data[i][3] = 0; /* no data for skip */
    }

    pp->log2_tile_cols = h->h.tiling.log2_tile_cols;
    pp->log2_tile_rows = h->h.tiling.log2_tile_rows;

    pp->uncompressed_header_size_byte_aligned = h->h.uncompressed_header_size;
    pp->first_partition_size = h->h.compressed_header_size;

    pp->StatusReportFeedbackNumber = 1 + ctx->report_id++;
    return 0;
}

static void fill_slice_short(DXVA_Slice_VPx_Short *slice,
                             unsigned position, unsigned size)
{
    memset(slice, 0, sizeof(*slice));
    slice->BSNALunitDataLocation = position;
    slice->SliceBytesInBuffer    = size;
    slice->wBadSliceChopping     = 0;
}

static int dxva_start_frame(AVCodecContext *avctx,
                            struct DXVA_VP9_Picture_Context *ctx_pic)
{
    const VP9SharedContext *h = avctx->priv_data;
    dxva_context* ctx         = (dxva_context*)avctx->dxva_context;
    
    av_assert0(ctx_pic);

    /* Fill up DXVA_PicParams_VP9 */
    if (fill_picture_parameters(avctx, ctx, h, &ctx_pic->pp) < 0)
        return -1;

    ctx_pic->bitstream_size = 0;
    ctx_pic->bitstream      = NULL;
    return 0;
}

static int dxva_decode_slice(AVCodecContext *avctx,
                             struct DXVA_VP9_Picture_Context *ctx_pic,
                             const uint8_t *buffer,
                             uint32_t size)
{
    const VP9SharedContext *h = avctx->priv_data;
    unsigned position;

    if (!ctx_pic->bitstream)
        ctx_pic->bitstream = buffer;
    ctx_pic->bitstream_size += size;

    position = buffer - ctx_pic->bitstream;
    fill_slice_short(&ctx_pic->slice, position, size);

    return 0;
}

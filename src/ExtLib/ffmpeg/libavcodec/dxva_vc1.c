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

#include <windows.h>
#include "dxva_internal.h"
#include "dxva_vc1.h"

void vc1_getcurframe(struct AVCodecContext* avctx, AVFrame** frame)
{
    const VC1Context *v = avctx->priv_data;
    *frame              = v->s.current_picture_ptr->f;
}

static void dxva_fill_picture_parameters(AVCodecContext *avctx,
                                         const VC1Context *v,
                                         DXVA_PictureParameters *pp)
{
    const MpegEncContext *s = &v->s;
    const Picture *current_picture = s->current_picture_ptr;
    dxva_context* ctx = (dxva_context*)avctx->dxva_context;
    int intcomp = 0;

    // determine if intensity compensation is needed
    if (s->pict_type == AV_PICTURE_TYPE_P) {
      if ((v->fcm == ILACE_FRAME && v->intcomp) || (v->fcm != ILACE_FRAME && v->mv_mode == MV_PMODE_INTENSITY_COMP)) {
        if (v->lumscale != 32 || v->lumshift != 0 || (s->picture_structure != PICT_FRAME && (v->lumscale2 != 32 || v->lumshift2 != 0)))
          intcomp = 1;
      }
    }

    memset(pp, 0, sizeof(*pp));
    pp->wDecodedPictureIndex    =
    pp->wDeblockedPictureIndex  = (unsigned)current_picture->f->data[4];
    if (s->pict_type != AV_PICTURE_TYPE_I && !v->bi_type)
        pp->wForwardRefPictureIndex = (unsigned)s->last_picture.f->data[4];
    else
        pp->wForwardRefPictureIndex = 0xffff;
    if (s->pict_type == AV_PICTURE_TYPE_B && !v->bi_type)
        pp->wBackwardRefPictureIndex = (unsigned)s->next_picture.f->data[4];
    else
        pp->wBackwardRefPictureIndex = 0xffff;
    if (v->profile == PROFILE_ADVANCED) {
        /* It is the cropped width/height -1 of the frame */
        pp->wPicWidthInMBminus1 = avctx->width  - 1;
        pp->wPicHeightInMBminus1= avctx->height - 1;
    } else {
        /* It is the coded width/height in macroblock -1 of the frame */
        pp->wPicWidthInMBminus1 = s->mb_width  - 1;
        pp->wPicHeightInMBminus1= s->mb_height - 1;
    }
    pp->bMacroblockWidthMinus1  = 15;
    pp->bMacroblockHeightMinus1 = 15;
    pp->bBlockWidthMinus1       = 7;
    pp->bBlockHeightMinus1      = 7;
    pp->bBPPminus1              = 7;
    if (s->picture_structure & PICT_TOP_FIELD)
        pp->bPicStructure      |= 0x01;
    if (s->picture_structure & PICT_BOTTOM_FIELD)
        pp->bPicStructure      |= 0x02;
    pp->bSecondField            = v->interlace && v->fcm == ILACE_FIELD && v->second_field;
    pp->bPicIntra               = s->pict_type == AV_PICTURE_TYPE_I || v->bi_type;
    pp->bPicBackwardPrediction  = s->pict_type == AV_PICTURE_TYPE_B && !v->bi_type;
    pp->bBidirectionalAveragingMode = (1                                           << 7) |
                                      ((ctx->cfg->ConfigIntraResidUnsigned != 0)   << 6) |
                                      ((ctx->cfg->ConfigResidDiffAccelerator != 0) << 5) |
                                      (intcomp                                     << 4) |
                                      ((v->profile == PROFILE_ADVANCED)            << 3);
    pp->bMVprecisionAndChromaRelation = ((v->mv_mode == MV_PMODE_1MV_HPEL_BILIN) << 3) |
                                        (1                                       << 2) |
                                        (0                                       << 1) |
                                        (!s->quarter_sample                          );
    pp->bChromaFormat           = v->chromaformat;
    ctx->report_id++;
    if (ctx->report_id >= (1 << 16))
        ctx->report_id = 1;
    pp->bPicScanFixed           = ctx->report_id >> 8;
    pp->bPicScanMethod          = ctx->report_id & 0xff;
    pp->bPicReadbackRequests    = 0;
    pp->bRcontrol               = v->rnd;
    pp->bPicSpatialResid8       = (v->panscanflag  << 7) |
                                  (v->refdist_flag << 6) |
                                  (s->loop_filter  << 5) |
                                  (v->fastuvmc     << 4) |
                                  (v->extended_mv  << 3) |
                                  (v->dquant       << 1) |
                                  (v->vstransform      );
    pp->bPicOverflowBlocks      = (v->quantizer_mode << 6) |
                                  (v->multires       << 5) |
                                  (v->resync_marker  << 4) |
                                  (v->rangered       << 3) |
                                  (s->max_b_frames       );
    pp->bPicExtrapolation       = (!v->interlace || v->fcm == PROGRESSIVE) ? 1 : 2;
    pp->bPicDeblocked           = ((!pp->bPicBackwardPrediction && v->overlap)        << 6) |
                                  ((v->profile != PROFILE_ADVANCED && v->rangeredfrm) << 5) |
                                  (s->loop_filter                                     << 1);
    pp->bPicDeblockConfined     = (v->postprocflag             << 7) |
                                  (v->broadcast                << 6) |
                                  (v->interlace                << 5) |
                                  (v->tfcntrflag               << 4) |
                                  (v->finterpflag              << 3) |
                                  ((s->pict_type != AV_PICTURE_TYPE_B) << 2) |
                                  (v->psf                      << 1) |
                                  (v->extended_dmv                 );
    if (s->pict_type != AV_PICTURE_TYPE_I)
        pp->bPic4MVallowed      = v->mv_mode == MV_PMODE_MIXED_MV ||
                                  (v->mv_mode == MV_PMODE_INTENSITY_COMP &&
                                   v->mv_mode2 == MV_PMODE_MIXED_MV);
    if (v->profile == PROFILE_ADVANCED)
        pp->bPicOBMC            = (v->range_mapy_flag  << 7) |
                                  (v->range_mapy       << 4) |
                                  (v->range_mapuv_flag << 3) |
                                  (v->range_mapuv          );
    pp->bPicBinPB               = 0;
    pp->bMV_RPS                 = (v->fcm == ILACE_FIELD && pp->bPicBackwardPrediction) ? v->refdist + 9 : 0;
    pp->bReservedBits           = v->pq;
    if (s->picture_structure == PICT_FRAME) {
        if (intcomp) {
            pp->wBitstreamFcodes      = v->lumscale;
            pp->wBitstreamPCEelements = v->lumshift;
        } else {
            pp->wBitstreamFcodes      = 32;
            pp->wBitstreamPCEelements = 0;
        }
    } else {
        /* Syntax: (top_field_param << 8) | bottom_field_param */
        if (intcomp) {
            pp->wBitstreamFcodes      = (v->lumscale << 8) | v->lumscale2;
            pp->wBitstreamPCEelements = (v->lumshift << 8) | v->lumshift2;
        } else {
            pp->wBitstreamFcodes      = (32 << 8) | 32;
            pp->wBitstreamPCEelements = 0;
        }
    }
    pp->bBitstreamConcealmentNeed   = 0;
    pp->bBitstreamConcealmentMethod = 0;
}

static void fill_slice(AVCodecContext *avctx, DXVA_SliceInfo *slice,
                       unsigned position, unsigned size)
{
    const VC1Context *v = avctx->priv_data;
    const MpegEncContext *s = &v->s;

    memset(slice, 0, sizeof(*slice));
    slice->wHorizontalPosition = 0;
    slice->wVerticalPosition   = s->mb_y;
    slice->dwSliceBitsInBuffer = 8 * size;
    slice->dwSliceDataLocation = position;
    slice->bStartCodeBitOffset = 0;
    slice->bReservedBits       = (s->pict_type == AV_PICTURE_TYPE_B && !v->bi_type) ? v->bfraction_lut_index + 9 : 0;
    slice->wMBbitOffset        = v->p_frame_skipped ? 0xffff : get_bits_count(&s->gb) + (avctx->codec_id == AV_CODEC_ID_VC1 ? 32 : 0);
    /* XXX We store the index of the first MB and it will be fixed later */
    slice->wNumberMBsInSlice   = (s->mb_y >> v->field_mode) * s->mb_width + s->mb_x;
    slice->wQuantizerScaleCode = v->pq;
    slice->wBadSliceChopping   = 0;
}

static int dxva_decode_slice(AVCodecContext *avctx,
                             DXVA_VC1_Picture_Context *ctx_pic,
                             const uint8_t *buffer, uint32_t size)
{
    unsigned position;

    if (ctx_pic->slice_count >= MAX_SLICE)
        return -1;

    if (avctx->codec_id == AV_CODEC_ID_VC1 &&
        size >= 4 && IS_MARKER(AV_RB32(buffer))) {
        buffer += 4;
        size   -= 4;
    }

    if (!ctx_pic->bitstream)
        ctx_pic->bitstream = buffer;
    ctx_pic->bitstream_size += size;

    position = buffer - ctx_pic->bitstream;
    fill_slice(avctx, &ctx_pic->slice[ctx_pic->slice_count++], position, size);
    return 0;
}

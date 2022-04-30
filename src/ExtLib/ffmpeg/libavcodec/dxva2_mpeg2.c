/*
 * MPEG-2 HW acceleration.
 *
 * copyright (c) 2010 Laurent Aimar
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "config_components.h"

#include "libavutil/log.h"

#include "dxva2_internal.h"
#include "mpegutils.h"
#include "mpegvideodec.h"

#define MAX_SLICES 1024
struct dxva2_picture_context {
    DXVA_PictureParameters pp;
    DXVA_QmatrixData       qm;
    unsigned               slice_count;
    DXVA_SliceInfo         slice[MAX_SLICES];

    const uint8_t          *bitstream;
    unsigned               bitstream_size;
};

static void fill_picture_parameters(AVCodecContext *avctx,
                                    AVDXVAContext *ctx,
                                    const struct MpegEncContext *s,
                                    DXVA_PictureParameters *pp)
{
    const Picture *current_picture = s->current_picture_ptr;
    int is_field = s->picture_structure != PICT_FRAME;

    memset(pp, 0, sizeof(*pp));
    pp->wDecodedPictureIndex         = ff_dxva2_get_surface_index(avctx, ctx, current_picture->f);
#if CONFIG_D3D12
    if (ff_dxva2_is_d3d12(avctx))
        // Post processing operations are not supported in D3D12 Video
        pp->wDeblockedPictureIndex       = 0xffff;
    else
#endif
		pp->wDeblockedPictureIndex       = 0;
    if (s->pict_type != AV_PICTURE_TYPE_I && s->last_picture.f->buf[0])
        pp->wForwardRefPictureIndex  = ff_dxva2_get_surface_index(avctx, ctx, s->last_picture.f);
    else
        pp->wForwardRefPictureIndex  = 0xffff;
    if (s->pict_type == AV_PICTURE_TYPE_B && s->last_picture.f->buf[0])
        pp->wBackwardRefPictureIndex = ff_dxva2_get_surface_index(avctx, ctx, s->next_picture.f);
    else
        pp->wBackwardRefPictureIndex = 0xffff;
    pp->wPicWidthInMBminus1          = s->mb_width  - 1;
    pp->wPicHeightInMBminus1         = (s->mb_height >> is_field) - 1;
    pp->bMacroblockWidthMinus1       = 15;
    pp->bMacroblockHeightMinus1      = 15;
    pp->bBlockWidthMinus1            = 7;
    pp->bBlockHeightMinus1           = 7;
    pp->bBPPminus1                   = 7;
    pp->bPicStructure                = s->picture_structure;
    pp->bSecondField                 = is_field && !s->first_field;
    pp->bPicIntra                    = s->pict_type == AV_PICTURE_TYPE_I;
    pp->bPicBackwardPrediction       = s->pict_type == AV_PICTURE_TYPE_B;
    pp->bBidirectionalAveragingMode  = 0;
    pp->bMVprecisionAndChromaRelation= 0; /* FIXME */
    pp->bChromaFormat                = s->chroma_format;
    pp->bPicScanFixed                = 1;
    pp->bPicScanMethod               = s->alternate_scan ? 1 : 0;
    pp->bPicReadbackRequests         = 0;
    pp->bRcontrol                    = 0;
    pp->bPicSpatialResid8            = 0;
    pp->bPicOverflowBlocks           = 0;
    pp->bPicExtrapolation            = 0;
    pp->bPicDeblocked                = 0;
    pp->bPicDeblockConfined          = 0;
    pp->bPic4MVallowed               = 0;
    pp->bPicOBMC                     = 0;
    pp->bPicBinPB                    = 0;
    pp->bMV_RPS                      = 0;
    pp->bReservedBits                = 0;
    pp->wBitstreamFcodes             = (s->mpeg_f_code[0][0] << 12) |
                                       (s->mpeg_f_code[0][1] <<  8) |
                                       (s->mpeg_f_code[1][0] <<  4) |
                                       (s->mpeg_f_code[1][1]      );
    pp->wBitstreamPCEelements        = (s->intra_dc_precision         << 14) |
                                       (s->picture_structure          << 12) |
                                       (s->top_field_first            << 11) |
                                       (s->frame_pred_frame_dct       << 10) |
                                       (s->concealment_motion_vectors <<  9) |
                                       (s->q_scale_type               <<  8) |
                                       (s->intra_vlc_format           <<  7) |
                                       (s->alternate_scan             <<  6) |
                                       (s->repeat_first_field         <<  5) |
                                       (s->chroma_420_type            <<  4) |
                                       (s->progressive_frame          <<  3);
    pp->bBitstreamConcealmentNeed    = 0;
    pp->bBitstreamConcealmentMethod  = 0;
}

static void fill_quantization_matrices(AVCodecContext *avctx,
                                       AVDXVAContext *ctx,
                                       const struct MpegEncContext *s,
                                       DXVA_QmatrixData *qm)
{
    int i;
    for (i = 0; i < 4; i++)
        qm->bNewQmatrix[i] = 1;
    for (i = 0; i < 64; i++) {
        int n = s->idsp.idct_permutation[ff_zigzag_direct[i]];
        qm->Qmatrix[0][i] = s->intra_matrix[n];
        qm->Qmatrix[1][i] = s->inter_matrix[n];
        qm->Qmatrix[2][i] = s->chroma_intra_matrix[n];
        qm->Qmatrix[3][i] = s->chroma_inter_matrix[n];
    }
}

static void fill_slice(AVCodecContext *avctx,
                       const struct MpegEncContext *s,
                       DXVA_SliceInfo *slice,
                       unsigned position,
                       const uint8_t *buffer, unsigned size)
{
    int is_field = s->picture_structure != PICT_FRAME;
    GetBitContext gb;

    memset(slice, 0, sizeof(*slice));
    slice->wHorizontalPosition = s->mb_x;
    slice->wVerticalPosition   = s->mb_y >> is_field;
    slice->dwSliceBitsInBuffer = 8 * size;
    slice->dwSliceDataLocation = position;
    slice->bStartCodeBitOffset = 0;
    slice->bReservedBits       = 0;
    /* XXX We store the index of the first MB and it will be fixed later */
    slice->wNumberMBsInSlice   = (s->mb_y >> is_field) * s->mb_width + s->mb_x;
    slice->wBadSliceChopping   = 0;

    init_get_bits(&gb, &buffer[4], 8 * (size - 4));

    slice->wQuantizerScaleCode = get_bits(&gb, 5);
    skip_1stop_8data_bits(&gb);

    slice->wMBbitOffset        = 4 * 8 + get_bits_count(&gb);
}
static int commit_bitstream_and_slice_buffer(AVCodecContext *avctx,
                                             DECODER_BUFFER_DESC *bs,
                                             DECODER_BUFFER_DESC *sc)
{
    const struct MpegEncContext *s = avctx->priv_data;
    AVDXVAContext *ctx = DXVA_CONTEXT(avctx);
    struct dxva2_picture_context *ctx_pic =
        s->current_picture_ptr->hwaccel_picture_private;
    const int is_field = s->picture_structure != PICT_FRAME;
    const unsigned mb_count = s->mb_width * (s->mb_height >> is_field);
	DXVA_SliceInfo *slice = NULL;
    void     *dxva_data_ptr;
    uint8_t  *dxva_data, *current, *end;
    unsigned dxva_size;
	unsigned padding;
    unsigned i;
    unsigned type;

#if CONFIG_D3D11VA
    if (ff_dxva2_is_d3d11(avctx)) {
        type = D3D11_VIDEO_DECODER_BUFFER_BITSTREAM;
        if (FAILED(ID3D11VideoContext_GetDecoderBuffer(D3D11VA_CONTEXT(ctx)->video_context,
                                                       D3D11VA_CONTEXT(ctx)->decoder,
                                                       type,
                                                       &dxva_size, &dxva_data_ptr)))
            return -1;
    }
#endif
#if CONFIG_DXVA2
    if (avctx->pix_fmt == AV_PIX_FMT_DXVA2_VLD) {
        type = DXVA2_BitStreamDateBufferType;
        if (FAILED(IDirectXVideoDecoder_GetBuffer(DXVA2_CONTEXT(ctx)->decoder,
                                                  type,
                                                  &dxva_data_ptr, &dxva_size)))
            return -1;
    }
#endif
#if CONFIG_D3D12
    if (ff_dxva2_is_d3d12(avctx)) {
        FFDXVASharedContext *sctx = DXVA_SHARED_CONTEXT(avctx);
        D3D12_VIDEO_DECODE_COMPRESSED_BITSTREAM *bistream = bs;
        *bistream = (D3D12_VIDEO_DECODE_COMPRESSED_BITSTREAM) { .Offset = 0 };
        HRESULT hr;

        dxva_size = 0;
        for (size_t i=0; i<ctx_pic->slice_count; i++)
        {
            DXVA_SliceInfo *slice = &ctx_pic->slice[i];
            dxva_size += slice->dwSliceBitsInBuffer / 8;
        }
        dxva_size = (dxva_size + (128 - 1)) & ~(128 - 1); // 128 bytes alignment

        bistream->Size = dxva_size;

        D3D12_HEAP_PROPERTIES constProp = {
            .Type = D3D12_HEAP_TYPE_UPLOAD,
            .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
            .CreationNodeMask = 1,
            .VisibleNodeMask = 1,
        };
        D3D12_RESOURCE_DESC constantDesc = {
            .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
            .Width = bistream->Size,
            .Height = 1,
            .DepthOrArraySize = 1,
            .MipLevels = 1,
            .Format = DXGI_FORMAT_UNKNOWN,
            .SampleDesc.Count = 1,
            .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            .Flags = D3D12_RESOURCE_FLAG_NONE,
        };

        hr = ID3D12Device1_CreateCommittedResource(sctx->d3d12_device, &constProp, D3D12_HEAP_FLAG_NONE,
                                                   &constantDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL,
                                                   &IID_ID3D12Resource, (void**)&bistream->pBuffer);
        if (FAILED(hr))
            return -1;

        D3D12_RANGE readRange = {0}; // no reading of buffers we write in
        hr = ID3D12Resource_Map(bistream->pBuffer, 0, &readRange, (void**)&dxva_data_ptr);
        if (FAILED(hr))
        {
            ID3D12Resource_Release(bistream->pBuffer);
            return -1;
        }
    }
#endif

    dxva_data = dxva_data_ptr;
    current = dxva_data;
    end = dxva_data + dxva_size;

    for (i = 0; i < ctx_pic->slice_count; i++) {
        slice = &ctx_pic->slice[i];
        unsigned position = slice->dwSliceDataLocation;
        unsigned size     = slice->dwSliceBitsInBuffer / 8;
        if (size > end - current) {
            av_log(avctx, AV_LOG_ERROR, "Failed to build bitstream");
            break;
        }
        slice->dwSliceDataLocation = current - dxva_data;

        if (i < ctx_pic->slice_count - 1)
            slice->wNumberMBsInSlice =
                slice[1].wNumberMBsInSlice - slice[0].wNumberMBsInSlice;
        else
            slice->wNumberMBsInSlice =
                mb_count - slice[0].wNumberMBsInSlice;

        memcpy(current, &ctx_pic->bitstream[position], size);
        current += size;
    }
	padding = FFMIN(128 - ((current - dxva_data) & 127), end - current);
    if (slice && padding > 0) {
        memset(current, 0, padding);
        current += padding;

        slice->dwSliceBitsInBuffer += 8 * padding;
    }
#if CONFIG_D3D11VA
    if (ff_dxva2_is_d3d11(avctx))
        if (FAILED(ID3D11VideoContext_ReleaseDecoderBuffer(D3D11VA_CONTEXT(ctx)->video_context, D3D11VA_CONTEXT(ctx)->decoder, type)))
            return -1;
#endif
#if CONFIG_DXVA2
    if (avctx->pix_fmt == AV_PIX_FMT_DXVA2_VLD)
        if (FAILED(IDirectXVideoDecoder_ReleaseBuffer(DXVA2_CONTEXT(ctx)->decoder, type)))
            return -1;
#endif
#if CONFIG_D3D12
    if (ff_dxva2_is_d3d12(avctx)) {
        D3D12_VIDEO_DECODE_COMPRESSED_BITSTREAM *bistream = bs;
        ID3D12Resource_Unmap(bistream->pBuffer, 0, NULL);
    }
#endif
    if (i < ctx_pic->slice_count)
        return -1;

#if CONFIG_D3D11VA
    if (ff_dxva2_is_d3d11(avctx)) {
        D3D11_VIDEO_DECODER_BUFFER_DESC *dsc11 = bs;
        memset(dsc11, 0, sizeof(*dsc11));
        dsc11->BufferType           = type;
        dsc11->DataSize             = current - dxva_data;
        dsc11->NumMBsInBuffer       = mb_count;

        type = D3D11_VIDEO_DECODER_BUFFER_SLICE_CONTROL;
    }
#endif
#if CONFIG_DXVA2
    if (avctx->pix_fmt == AV_PIX_FMT_DXVA2_VLD) {
        DXVA2_DecodeBufferDesc *dsc2 = bs;
        memset(dsc2, 0, sizeof(*dsc2));
        dsc2->CompressedBufferType = type;
        dsc2->DataSize             = current - dxva_data;
        dsc2->NumMBsInBuffer       = mb_count;

        type = DXVA2_SliceControlBufferType;
    }
#endif
#if CONFIG_D3D12
    if (ff_dxva2_is_d3d12(avctx)) {
        type = D3D12_VIDEO_DECODE_ARGUMENT_TYPE_SLICE_CONTROL;
    }
#endif

    return ff_dxva2_commit_buffer(avctx, ctx, sc,
                                  type,
                                  ctx_pic->slice,
                                  ctx_pic->slice_count * sizeof(*ctx_pic->slice),
                                  mb_count);
}

static int dxva2_mpeg2_start_frame(AVCodecContext *avctx,
                                   av_unused const uint8_t *buffer,
                                   av_unused uint32_t size)
{
    const struct MpegEncContext *s = avctx->priv_data;
    AVDXVAContext *ctx = DXVA_CONTEXT(avctx);
    struct dxva2_picture_context *ctx_pic =
        s->current_picture_ptr->hwaccel_picture_private;

    if (!DXVA_CONTEXT_VALID(avctx, ctx))
        return -1;
    assert(ctx_pic);

    fill_picture_parameters(avctx, ctx, s, &ctx_pic->pp);
    fill_quantization_matrices(avctx, ctx, s, &ctx_pic->qm);

    ctx_pic->slice_count    = 0;
    ctx_pic->bitstream_size = 0;
    ctx_pic->bitstream      = NULL;
    return 0;
}

static int dxva2_mpeg2_decode_slice(AVCodecContext *avctx,
                                    const uint8_t *buffer, uint32_t size)
{
    const struct MpegEncContext *s = avctx->priv_data;
    struct dxva2_picture_context *ctx_pic =
        s->current_picture_ptr->hwaccel_picture_private;
    unsigned position;

    if (ctx_pic->slice_count >= MAX_SLICES) {
        avpriv_request_sample(avctx, "%d slices in dxva2",
                              ctx_pic->slice_count);
        return -1;
    }
    if (!ctx_pic->bitstream)
        ctx_pic->bitstream = buffer;
    ctx_pic->bitstream_size += size;

    position = buffer - ctx_pic->bitstream;
    fill_slice(avctx, s, &ctx_pic->slice[ctx_pic->slice_count++], position,
               buffer, size);
    return 0;
}

static int dxva2_mpeg2_end_frame(AVCodecContext *avctx)
{
    struct MpegEncContext *s = avctx->priv_data;
    struct dxva2_picture_context *ctx_pic =
        s->current_picture_ptr->hwaccel_picture_private;
    int ret;

    if (ctx_pic->slice_count <= 0 || ctx_pic->bitstream_size <= 0)
        return -1;
    ret = ff_dxva2_common_end_frame(avctx, s->current_picture_ptr->f,
                                    &ctx_pic->pp, sizeof(ctx_pic->pp),
                                    &ctx_pic->qm, sizeof(ctx_pic->qm),
                                    commit_bitstream_and_slice_buffer);
    if (!ret)
        ff_mpeg_draw_horiz_band(s, 0, avctx->height);
    return ret;
}

#if CONFIG_MPEG2_DXVA2_HWACCEL
const AVHWAccel ff_mpeg2_dxva2_hwaccel = {
    .name           = "mpeg2_dxva2",
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_MPEG2VIDEO,
    .pix_fmt        = AV_PIX_FMT_DXVA2_VLD,
    .init           = ff_dxva2_decode_init,
    .uninit         = ff_dxva2_decode_uninit,
    .start_frame    = dxva2_mpeg2_start_frame,
    .decode_slice   = dxva2_mpeg2_decode_slice,
    .end_frame      = dxva2_mpeg2_end_frame,
    .frame_params   = ff_dxva2_common_frame_params,
    .frame_priv_data_size = sizeof(struct dxva2_picture_context),
    .priv_data_size = sizeof(FFDXVASharedContext),
};
#endif

#if CONFIG_MPEG2_D3D11VA_HWACCEL
const AVHWAccel ff_mpeg2_d3d11va_hwaccel = {
    .name           = "mpeg2_d3d11va",
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_MPEG2VIDEO,
    .pix_fmt        = AV_PIX_FMT_D3D11VA_VLD,
    .init           = ff_dxva2_decode_init,
    .uninit         = ff_dxva2_decode_uninit,
    .start_frame    = dxva2_mpeg2_start_frame,
    .decode_slice   = dxva2_mpeg2_decode_slice,
    .end_frame      = dxva2_mpeg2_end_frame,
    .frame_params   = ff_dxva2_common_frame_params,
    .frame_priv_data_size = sizeof(struct dxva2_picture_context),
    .priv_data_size = sizeof(FFDXVASharedContext),
};
#endif

#if CONFIG_MPEG2_D3D11VA2_HWACCEL
const AVHWAccel ff_mpeg2_d3d11va2_hwaccel = {
    .name           = "mpeg2_d3d11va2",
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_MPEG2VIDEO,
    .pix_fmt        = AV_PIX_FMT_D3D11,
    .init           = ff_dxva2_decode_init,
    .uninit         = ff_dxva2_decode_uninit,
    .start_frame    = dxva2_mpeg2_start_frame,
    .decode_slice   = dxva2_mpeg2_decode_slice,
    .end_frame      = dxva2_mpeg2_end_frame,
    .frame_params   = ff_dxva2_common_frame_params,
    .frame_priv_data_size = sizeof(struct dxva2_picture_context),
    .priv_data_size = sizeof(FFDXVASharedContext),
};
#endif

#if CONFIG_MPEG2_D3D12_HWACCEL
const AVHWAccel ff_mpeg2_d3d12_hwaccel = {
    .name           = "mpeg2_d3d12",
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_MPEG2VIDEO,
    .pix_fmt        = AV_PIX_FMT_D3D12_VLD,
    .init           = ff_dxva2_decode_init,
    .uninit         = ff_dxva2_decode_uninit,
    .start_frame    = dxva2_mpeg2_start_frame,
    .decode_slice   = dxva2_mpeg2_decode_slice,
    .end_frame      = dxva2_mpeg2_end_frame,
    .frame_params   = ff_dxva2_common_frame_params,
    .frame_priv_data_size = sizeof(struct dxva2_picture_context),
    .priv_data_size = sizeof(FFDXVASharedContext),
};
#endif

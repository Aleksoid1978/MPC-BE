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
#include "dxva_mpeg2.h"

void mpeg2_getcurframe(struct AVCodecContext* avctx, AVFrame** frame)
{
    const MpegEncContext *s = avctx->priv_data;
    *frame                  = s->current_picture.f;
}

static void fill_picture_parameters(AVCodecContext *avctx,
                                    const struct MpegEncContext *s,
                                    DXVA_PictureParameters *pp)
{
    const Picture *current_picture = s->current_picture_ptr;
    int is_field = s->picture_structure != PICT_FRAME;

    memset(pp, 0, sizeof(*pp));
    pp->wDecodedPictureIndex         = (unsigned)s->current_picture_ptr->f->data[4];
    pp->wDeblockedPictureIndex       = 0;
    if (s->pict_type != AV_PICTURE_TYPE_I)
        pp->wForwardRefPictureIndex  = (unsigned)s->last_picture.f->data[4];
    else
        pp->wForwardRefPictureIndex  = 0xffff;
    if (s->pict_type == AV_PICTURE_TYPE_B)
        pp->wBackwardRefPictureIndex = (unsigned)s->next_picture.f->data[4];
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

static int dxva_start_frame(AVCodecContext *avctx,
                            struct DXVA_MPEG2_Context *ctx,
                            struct DXVA_MPEG2_Picture_Context *ctx_pic)
{
    const struct MpegEncContext *s = avctx->priv_data;

    assert(ctx_pic);

    fill_picture_parameters(avctx, s, &ctx_pic->pp);
    fill_quantization_matrices(avctx, s, &ctx_pic->qm);

    ctx_pic->slice_count    = 0;
    ctx_pic->bitstream_size = 0;
    ctx_pic->bitstream      = NULL;
    ctx_pic->frame_start    = 1;
    return 0;
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

static int dxva_decode_slice(AVCodecContext *avctx,
                             DXVA_MPEG2_Picture_Context *ctx_pic,
                             const uint8_t *buffer, uint32_t size)
{
    const struct MpegEncContext *s = avctx->priv_data;
    unsigned position;

    if (ctx_pic->slice_count >= MAX_SLICE)
        return -1;

    if (!ctx_pic->bitstream)
        ctx_pic->bitstream = buffer;
    ctx_pic->bitstream_size += size;

    position = buffer - ctx_pic->bitstream;
    fill_slice(avctx, s, &ctx_pic->slice[ctx_pic->slice_count++], position,
               buffer, size);
    return 0;
}

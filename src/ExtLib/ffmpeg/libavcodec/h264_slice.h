/*
 * (C) 2009-2013 see Authors.txt
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

#include "avcodec.h"
#include "h264.h"

static const enum AVPixelFormat h264_hwaccel_pixfmt_list_420[] = {
#if CONFIG_H264_DXVA2_HWACCEL
    AV_PIX_FMT_DXVA2_VLD,
#endif
#if CONFIG_H264_VAAPI_HWACCEL
    AV_PIX_FMT_VAAPI_VLD,
#endif
#if CONFIG_H264_VDA_HWACCEL
    AV_PIX_FMT_VDA_VLD,
    AV_PIX_FMT_VDA,
#endif
#if CONFIG_H264_VDPAU_HWACCEL
    AV_PIX_FMT_VDPAU,
#endif
    AV_PIX_FMT_YUV420P,
    AV_PIX_FMT_NONE
};

enum AVPixelFormat get_pixel_format(H264Context *h, int force_callback)
{
    enum AVPixelFormat pix_fmts[2];
    const enum AVPixelFormat *choices = pix_fmts;
    int i;

    pix_fmts[1] = AV_PIX_FMT_NONE;

    switch (h->sps.bit_depth_luma) {
    case 9:
        if (CHROMA444(h)) {
            if (h->avctx->colorspace == AVCOL_SPC_RGB) {
                pix_fmts[0] = AV_PIX_FMT_GBRP9;
            } else
                pix_fmts[0] = AV_PIX_FMT_YUV444P9;
        } else if (CHROMA422(h))
            pix_fmts[0] = AV_PIX_FMT_YUV422P9;
        else
            pix_fmts[0] = AV_PIX_FMT_YUV420P9;
        break;
    case 10:
        if (CHROMA444(h)) {
            if (h->avctx->colorspace == AVCOL_SPC_RGB) {
                pix_fmts[0] = AV_PIX_FMT_GBRP10;
            } else
                pix_fmts[0] = AV_PIX_FMT_YUV444P10;
        } else if (CHROMA422(h))
            pix_fmts[0] = AV_PIX_FMT_YUV422P10;
        else
            pix_fmts[0] = AV_PIX_FMT_YUV420P10;
        break;
    case 12:
        if (CHROMA444(h)) {
            if (h->avctx->colorspace == AVCOL_SPC_RGB) {
                pix_fmts[0] = AV_PIX_FMT_GBRP12;
            } else
                pix_fmts[0] = AV_PIX_FMT_YUV444P12;
        } else if (CHROMA422(h))
            pix_fmts[0] = AV_PIX_FMT_YUV422P12;
        else
            pix_fmts[0] = AV_PIX_FMT_YUV420P12;
        break;
    case 14:
        if (CHROMA444(h)) {
            if (h->avctx->colorspace == AVCOL_SPC_RGB) {
                pix_fmts[0] = AV_PIX_FMT_GBRP14;
            } else
                pix_fmts[0] = AV_PIX_FMT_YUV444P14;
        } else if (CHROMA422(h))
            pix_fmts[0] = AV_PIX_FMT_YUV422P14;
        else
            pix_fmts[0] = AV_PIX_FMT_YUV420P14;
        break;
    case 8:
        if (CHROMA444(h)) {
            if (h->avctx->colorspace == AVCOL_SPC_YCGCO)
                av_log(h->avctx, AV_LOG_WARNING, "Detected unsupported YCgCo colorspace.\n");
            if (h->avctx->colorspace == AVCOL_SPC_RGB)
                pix_fmts[0] = AV_PIX_FMT_GBRP;
            /*else if (h->avctx->color_range == AVCOL_RANGE_JPEG)
                pix_fmts[0] = AV_PIX_FMT_YUVJ444P;*/
            else
                pix_fmts[0] = AV_PIX_FMT_YUV444P;
        } else if (CHROMA422(h)) {
            /*if (h->avctx->color_range == AVCOL_RANGE_JPEG)
                pix_fmts[0] = AV_PIX_FMT_YUVJ422P;
            else*/
                pix_fmts[0] = AV_PIX_FMT_YUV422P;
        } else {
            if (h->avctx->codec->pix_fmts)
                choices = h->avctx->codec->pix_fmts;
            /*else if (h->avctx->color_range == AVCOL_RANGE_JPEG)
                choices = h264_hwaccel_pixfmt_list_jpeg_420;
            else*/
                choices = h264_hwaccel_pixfmt_list_420;
        }
        break;
    default:
        av_log(h->avctx, AV_LOG_ERROR,
               "Unsupported bit depth %d\n", h->sps.bit_depth_luma);
        return AVERROR_INVALIDDATA;
    }

    for (i=0; choices[i] != AV_PIX_FMT_NONE; i++)
        if (choices[i] == h->avctx->pix_fmt && !force_callback)
            return choices[i];
    return ff_thread_get_format(h->avctx, choices);
}

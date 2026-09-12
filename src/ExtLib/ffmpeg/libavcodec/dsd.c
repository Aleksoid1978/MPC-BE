/*
 * Direct Stream Digital (DSD) decoder
 * Copyright (c) 2014 Peter Ross
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

#include "config.h"

#include "version_major.h"
#include "libavutil/attributes.h"
#include "dsd.h"

#if CONFIG_SWRESAMPLE && FF_API_DSD_PCM
#include "libswresample/swresample.h"
#include "avcodec.h"

av_cold int ff_dsd_to_pcm_init(AVCodecContext *avctx, struct SwrContext **swrp)
{
    SwrContext *swr = NULL;
    int ret;

    swr_free(swrp);

    ret = swr_alloc_set_opts2(&swr, &avctx->ch_layout, avctx->sample_fmt,
                              avctx->sample_rate, &avctx->ch_layout,
                              AV_SAMPLE_FMT_DSD, avctx->sample_rate, 0, avctx);
    if (ret < 0)
        return ret;

    ret = swr_init(swr);
    if (ret < 0) {
        swr_free(&swr);
        return ret;
    }

    av_log(avctx, AV_LOG_WARNING,
           "Converting DSD to PCM in the decoder is deprecated and will be "
           "removed. Set request_sample_fmt to AV_SAMPLE_FMT_DSD to receive "
           "the raw bitstream, and use libswresample to convert it to PCM "
           "when needed.\n");

    *swrp = swr;
    return 0;
}
#endif

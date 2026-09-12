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

/**
 * @file
 * Direct Stream Digital (DSD) decoder
 */

#include "config.h"

#include <string.h>

#include "libavutil/avassert.h"
#include "libavutil/mem.h"
#include "libavutil/reverse.h"

#include "avcodec.h"
#include "codec_internal.h"
#include "decode.h"
#include "dsd.h"

#if CONFIG_SWRESAMPLE && FF_API_DSD_PCM
#include "libswresample/swresample.h"

typedef struct DSDDecContext {
    struct SwrContext *swr;
    uint8_t *scratch;
    unsigned scratch_size;
} DSDDecContext;

#define PRIV_DATA_SIZE sizeof(DSDDecContext)
#else
#define PRIV_DATA_SIZE 0
#endif

static av_cold int decode_init(AVCodecContext *avctx)
{
    if (!avctx->ch_layout.nb_channels)
        return AVERROR_INVALIDDATA;

    avctx->sample_fmt = AV_SAMPLE_FMT_DSD;

#if CONFIG_SWRESAMPLE && FF_API_DSD_PCM
    if (avctx->request_sample_fmt != AV_SAMPLE_FMT_DSD)
        avctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
#endif

    return 0;
}

static av_cold int decode_close(AVCodecContext *avctx)
{
#if CONFIG_SWRESAMPLE && FF_API_DSD_PCM
    DSDDecContext *s = avctx->priv_data;

    swr_free(&s->swr);
    av_freep(&s->scratch);
#endif
    return 0;
}

// repack the input to interleaved DSD bytes, most significant bit first
static void repack(AVCodecContext *avctx, uint8_t *dst, const uint8_t *src,
                   int nb_samples)
{
    const int channels = avctx->ch_layout.nb_channels;

    switch (avctx->codec_id) {
    case AV_CODEC_ID_DSD_MSBF:
        memcpy(dst, src, nb_samples * channels);
        break;
    case AV_CODEC_ID_DSD_LSBF:
        for (int i = 0; i < nb_samples * channels; i++)
            dst[i] = ff_reverse[src[i]];
        break;
    case AV_CODEC_ID_DSD_MSBF_PLANAR:
        for (int ch = 0; ch < channels; ch++) {
            const uint8_t *plane = src + ch * nb_samples;
            for (int i = 0; i < nb_samples; i++)
                dst[i * channels + ch] = plane[i];
        }
        break;
    case AV_CODEC_ID_DSD_LSBF_PLANAR:
        for (int ch = 0; ch < channels; ch++) {
            const uint8_t *plane = src + ch * nb_samples;
            for (int i = 0; i < nb_samples; i++)
                dst[i * channels + ch] = ff_reverse[plane[i]];
        }
        break;
    default:
        av_assert1(0);
    }
}

static int decode_frame(AVCodecContext *avctx, AVFrame *frame,
                        int *got_frame_ptr, AVPacket *avpkt)
{
    const int channels = avctx->ch_layout.nb_channels;
    int ret;

    frame->nb_samples = avpkt->size / channels;

    if ((ret = ff_get_buffer(avctx, frame, 0)) < 0)
        return ret;

#if CONFIG_SWRESAMPLE && FF_API_DSD_PCM
    DSDDecContext *s = avctx->priv_data;
    if (avctx->sample_fmt != AV_SAMPLE_FMT_DSD) {
        if (!s->swr && (ret = ff_dsd_to_pcm_init(avctx, &s->swr)) < 0)
            return ret;

        av_fast_malloc(&s->scratch, &s->scratch_size,
                       frame->nb_samples * channels);
        if (!s->scratch)
            return AVERROR(ENOMEM);

        repack(avctx, s->scratch, avpkt->data, frame->nb_samples);

        ret = swr_convert(s->swr, frame->extended_data, frame->nb_samples,
                          (const uint8_t *const []){ s->scratch },
                          frame->nb_samples);
        if (ret != frame->nb_samples)
            return ret < 0 ? ret : AVERROR_BUG;

        *got_frame_ptr = 1;
        return frame->nb_samples * channels;
    }
#endif

    repack(avctx, frame->data[0], avpkt->data, frame->nb_samples);

    *got_frame_ptr = 1;
    return frame->nb_samples * channels;
}

#define DSD_DECODER(id_, name_, long_name_) \
const FFCodec ff_ ## name_ ## _decoder = { \
    .p.name       = #name_, \
    CODEC_LONG_NAME(long_name_), \
    .p.type       = AVMEDIA_TYPE_AUDIO, \
    .p.id         = AV_CODEC_ID_##id_, \
    .priv_data_size = PRIV_DATA_SIZE, \
    .init         = decode_init, \
    .close        = decode_close, \
    FF_CODEC_DECODE_CB(decode_frame), \
    .p.capabilities = AV_CODEC_CAP_DR1, \
};

DSD_DECODER(DSD_LSBF, dsd_lsbf, "DSD (Direct Stream Digital), least significant bit first")
DSD_DECODER(DSD_MSBF, dsd_msbf, "DSD (Direct Stream Digital), most significant bit first")
DSD_DECODER(DSD_MSBF_PLANAR, dsd_msbf_planar, "DSD (Direct Stream Digital), most significant bit first, planar")
DSD_DECODER(DSD_LSBF_PLANAR, dsd_lsbf_planar, "DSD (Direct Stream Digital), least significant bit first, planar")

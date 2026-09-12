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

#ifndef AVCODEC_DSD_H
#define AVCODEC_DSD_H

struct AVCodecContext;
struct SwrContext;

/**
 * (Re)create a libswresample context converting AV_SAMPLE_FMT_DSD to
 * avctx->sample_fmt at the same sample rate. This is a transitional helper for
 * the deprecated in-decoder DSD to PCM conversion.
 * Only available if CONFIG_SWRESAMPLE && FF_API_DSD_PCM.
 */
int ff_dsd_to_pcm_init(struct AVCodecContext *avctx, struct SwrContext **swrp);

#endif /* AVCODEC_DSD_H */

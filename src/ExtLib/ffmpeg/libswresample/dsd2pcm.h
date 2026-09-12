/*
 * DSD to PCM conversion
 * based on BSD licensed dsd2pcm by Sebastian Gesemann
 * Copyright (c) 2009, 2011 Sebastian Gesemann. All rights reserved.
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

#ifndef SWRESAMPLE_DSD2PCM_H
#define SWRESAMPLE_DSD2PCM_H

#include <stddef.h>
#include <stdint.h>

#define DSD_HTAPS   48               /** number of FIR constants */
#define DSD_FIFOSIZE 16              /** must be a power of two */
#define DSD_FIFOMASK (DSD_FIFOSIZE - 1)  /** bit mask for FIFO offsets */

#if DSD_FIFOSIZE * 8 < DSD_HTAPS * 2
#error "DSD_FIFOSIZE too small"
#endif

/**
 * Per-channel buffer
 */
typedef struct DSDContext {
    uint8_t buf[DSD_FIFOSIZE];
    unsigned pos;
} DSDContext;

void swri_dsd2pcm_init(void);

/**
 * Convert one channel of MSB-first DSD data (one byte = 8 samples) to
 * float PCM at 1/8th of the DSD bit rate. Strides are in elements.
 */
void swri_dsd2pcm_translate(DSDContext *s, size_t samples,
                            const uint8_t *src, ptrdiff_t src_stride,
                            float *dst, ptrdiff_t dst_stride);

#endif /* SWRESAMPLE_DSD2PCM_H */

/*
 * Copyright (C) 2026 Ramiro Polla <ramiro.polla@gmail.com>
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

#ifndef SWSCALE_JIT_H
#define SWSCALE_JIT_H

#include <stddef.h>

/* Allocate size bytes of writable memory for JIT code generation. */
void *ff_sws_jit_alloc(size_t size);

/* Protect JIT memory from further writes. */
int ff_sws_jit_protect(void *ptr, size_t size);

/* Free memory allocated by ff_sws_jit_alloc(). */
void ff_sws_jit_free(void *ptr, size_t size);

#endif /* SWSCALE_JIT_H */

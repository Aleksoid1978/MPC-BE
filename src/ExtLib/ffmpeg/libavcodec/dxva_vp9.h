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

#ifndef _MSC_VER
#include <windows.h>
#endif

#include "compat/windows/dxva_vpx.h"

typedef struct DXVA_VP9_Picture_Context {
    DXVA_PicParams_VP9    pp;
    DXVA_Slice_VPx_Short  slice;
    const uint8_t         *bitstream;
    unsigned              bitstream_size;
} DXVA_VP9_Picture_Context;

void vp9_getcurframe(struct AVCodecContext* avctx, AVFrame** frame);
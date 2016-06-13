/*
 * (C) 2015-2016 see Authors.txt
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

#ifdef __GNUC__
    #include <windows.h>
    #include "compat/windows/dxva_hevc.h"
#else
    #include <dxva.h>
#endif

#define MAX_HEVC_SLICES 256

typedef struct DXVA_HEVC_Picture_Context {
    DXVA_PicParams_HEVC   pp;
    DXVA_Qmatrix_HEVC     qm;
    unsigned              slice_count;
    DXVA_Slice_HEVC_Short slice_short[MAX_HEVC_SLICES];
    const uint8_t         *bitstream;
    unsigned              bitstream_size;
} DXVA_HEVC_Picture_Context;

void hevc_getcurframe(struct AVCodecContext* avctx, AVFrame** frame);
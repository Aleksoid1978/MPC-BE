/*
 *
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

#include <dxva.h>

#ifndef MAX_SLICES
#define MAX_SLICES 32
#endif

#define FF_DXVA2_WORKAROUND_SCALING_LIST_ZIGZAG 1 ///< Work around for DXVA2 and old UVD/UVD+ ATI video cards
#define FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO    2 ///< Work around for DXVA2 and old Intel GPUs with ClearVideo interface

typedef struct DXVA_H264_Picture_Context {
    DXVA_PicParams_H264       pp;
    DXVA_Qmatrix_H264         qm;
    unsigned                  slice_count;
    DXVA_Slice_H264_Short     slice_short[MAX_SLICES];
    DXVA_Slice_H264_Long      slice_long[MAX_SLICES];
    const uint8_t             *bitstream;
    unsigned                  bitstream_size;
} DXVA_H264_Picture_Context;
typedef struct DXVA_H264_Context {
    uint64_t                  workaround;
    int                       longslice;
    DXVA_H264_Picture_Context ctx_pic[2];
} DXVA_H264_Context;

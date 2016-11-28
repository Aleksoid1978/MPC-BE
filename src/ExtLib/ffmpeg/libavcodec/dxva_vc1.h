/*
 * (C) 2014-2015 see Authors.txt
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

#define MAX_SLICE               1024
#define MAX_VC1_PICTURE_CONTEXT    2

typedef struct DXVA_VC1_Picture_Context {
    DXVA_PictureParameters   pp;
    unsigned                 slice_count;
    DXVA_SliceInfo           slice[MAX_SLICE];
    const uint8_t            *bitstream;
    unsigned                 bitstream_size;
} DXVA_VC1_Picture_Context;
typedef struct DXVA_VC1_Context {
    unsigned                 ctx_pic_count;
    DXVA_VC1_Picture_Context ctx_pic[MAX_VC1_PICTURE_CONTEXT];
} DXVA_VC1_Context;

void vc1_getcurframe(struct AVCodecContext* avctx, AVFrame** frame);
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

#define MAX_SLICE                 1024
#define MAX_MPEG2_PICTURE_CONTEXT    2

typedef struct DXVA_MPEG2_Picture_Context {
    DXVA_PictureParameters     pp;
    DXVA_QmatrixData           qm;
    unsigned                   slice_count;
    DXVA_SliceInfo             slice[MAX_SLICE];

    const uint8_t              *bitstream;
    unsigned                   bitstream_size;
    int                        frame_start;
} DXVA_MPEG2_Picture_Context;
typedef struct DXVA_MPEG2_Context {
    unsigned                   ctx_pic_count;
    DXVA_MPEG2_Picture_Context ctx_pic[MAX_MPEG2_PICTURE_CONTEXT];
} DXVA_MPEG2_Context;

void mpeg2_getcurframe(struct AVCodecContext* avctx, AVFrame** frame);
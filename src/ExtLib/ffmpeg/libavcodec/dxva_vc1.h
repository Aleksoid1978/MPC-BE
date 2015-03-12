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

typedef struct DXVA_VC1_Picture_Context {
    DXVA_PictureParameters   pp;
    DXVA_SliceInfo           slice;
    const uint8_t            *bitstream;
    unsigned                 bitstream_size;
} DXVA_VC1_Picture_Context;
typedef struct DXVA_VC1_Context {
    unsigned                 frame_count;
    DXVA_VC1_Picture_Context ctx_pic[2];	
} DXVA_VC1_Context;

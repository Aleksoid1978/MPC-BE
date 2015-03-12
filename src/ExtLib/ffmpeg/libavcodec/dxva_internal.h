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

#include <dxva2api.h>

#define FF_DXVA2_WORKAROUND_SCALING_LIST_ZIGZAG 1 ///< Work around for DXVA2 and old UVD/UVD+ ATI video cards
#define FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO    2 ///< Work around for DXVA2 and old Intel GPUs with ClearVideo interface

typedef struct dxva_context {
    DXVA2_ConfigPictureDecode *cfg;
    int                       longslice;
    uint64_t                  workaround;
    unsigned                  report_id;
    
    void                      *dxva_decoder_context;
} dxva_context;

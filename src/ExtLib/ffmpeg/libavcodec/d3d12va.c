/*
 * Direct3D12 HW acceleration
 *
 * copyright (c) 2021 Steve Lhomme
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

#include <stddef.h>

#include "config.h"

#if CONFIG_D3D12
#include "libavutil/error.h"
#include "libavutil/mem.h"

#include "d3d12va.h"

AVD3D12VAContext *av_d3d12va_alloc_context(void)
{
    AVD3D12VAContext* res = av_mallocz(sizeof(AVD3D12VAContext));
    if (!res)
        return NULL;
    return res;
}
#else /* !CONFIG_D3D12 */
struct AVD3D12VAContext *av_d3d12va_alloc_context(void);

struct AVD3D12VAContext *av_d3d12va_alloc_context(void)
{
    return NULL;
}
#endif /* !CONFIG_D3D12 */

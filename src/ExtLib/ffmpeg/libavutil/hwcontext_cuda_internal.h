/*
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


#ifndef AVUTIL_HWCONTEXT_CUDA_INTERNAL_H
#define AVUTIL_HWCONTEXT_CUDA_INTERNAL_H

#include "compat/cuda/dynlink_loader.h"
#include "hwcontext_cuda.h"

/**
 * @file
 * FFmpeg internal API for CUDA.
 */

struct AVCUDADeviceContextInternal {
    CudaFunctions *cuda_dl;
    int is_allocated;
    CUdevice cuda_device;
    int flags;
};

/**
 * Return the element size in bytes for a CUarray_format, or 0 for unknown.
 *
 * Used to compute frame->linesize[] for CUarray frames (block-linear).
 * Callers that need a fallback should treat a 0 return as an error.
 */
static inline int ff_cuda_cuarray_elem_size(CUarray_format fmt)
{
    switch (fmt) {
    case CU_AD_FORMAT_UNSIGNED_INT8:
    case CU_AD_FORMAT_SIGNED_INT8:   return 1;
    case CU_AD_FORMAT_UNSIGNED_INT16:
    case CU_AD_FORMAT_SIGNED_INT16:
    case CU_AD_FORMAT_HALF:          return 2;
    case CU_AD_FORMAT_UNSIGNED_INT32:
    case CU_AD_FORMAT_SIGNED_INT32:
    case CU_AD_FORMAT_FLOAT:         return 4;
    default:                         return 0;
    }
}

#endif /* AVUTIL_HWCONTEXT_CUDA_INTERNAL_H */

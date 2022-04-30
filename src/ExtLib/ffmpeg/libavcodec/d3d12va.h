/*
 * Direct3D12 HW acceleration
 *
 * copyright (c) 2009 Laurent Aimar
 * copyright (c) 2015-2021 Steve Lhomme
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

#ifndef AVCODEC_D3D12VA_H
#define AVCODEC_D3D12VA_H

/**
 * @file
 * @ingroup lavc_codec_hwaccel_d3d12va
 * Public libavcodec D3D12VA header.
 */

#if !defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0A00
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#include <stdint.h>
#include <d3d12.h>
#include <d3d12video.h>

/**
 * @defgroup lavc_codec_hwaccel_d3d12va Direct3D12
 * @ingroup lavc_codec_hwaccel
 *
 * @{
 */

#define FF_DXVA2_WORKAROUND_HEVC_REXT           4 ///< Signal the D3D11VA decoder is using the HEVC Rext picture structure

/**
 * This structure is used to provides the necessary configurations and data
 * to the Direct3D12 FFmpeg HWAccel implementation.
 *
 * The application must make it available as AVCodecContext.hwaccel_context.
 *
 * Use av_d3d11va_alloc_context() exclusively to allocate an AVD3D12VAContext.
 */
typedef struct AVD3D12VAContext {
    /**
     * D3D12 decoder object
     */
    ID3D12VideoDecoder *decoder;

    /**
     * The Direct3D surfaces used to output pictures and used for reference buffers.
     * Only Texture2DArray supported for the moment (works with Tier1 and Tier2).
     */
    D3D12_VIDEO_DECODE_REFERENCE_FRAMES surfaces;

    /**
     * Heap format used to create the surfaces
     */
    const D3D12_VIDEO_DECODER_HEAP_DESC *cfg;

    /**
     * A bit field configuring the workarounds needed for using the decoder
     */
    uint64_t workaround;

    /**
     * Private to the FFmpeg AVHWAccel implementation
     */
    unsigned report_id;
} AVD3D12VAContext;

/**
 * Allocate an AVD3D12VAContext.
 *
 * @return Newly-allocated AVD3D12VAContext or NULL on failure.
 */
AVD3D12VAContext *av_d3d12va_alloc_context(void);

/**
 * @}
 */

#endif /* AVCODEC_D3D12VA_H */

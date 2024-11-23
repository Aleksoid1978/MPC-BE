/*
 * (C) 2020-2024 see Authors.txt
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

#include "stdafx.h"
#include "PixelUtils_AviSynth.h"
#include "PixelUtils_VirtualDub.h"
#include "PixelUtils.h"

#include "pixconv/yuv420_nv12_unscaled.h"

void CopyPlane(const UINT h, BYTE* dst, UINT dst_pitch, const BYTE* src, UINT src_pitch)
{
	if (dst_pitch == src_pitch) {
		memcpy(dst, src, dst_pitch * h);
		return;
	}

	const UINT linesize = std::min(src_pitch, dst_pitch);

	for (UINT y = 0; y < h; ++y) {
		memcpy(dst, src, linesize);
		src += src_pitch;
		dst += dst_pitch;
	}
}

void CopyI420toNV12(UINT w, UINT h, BYTE* dst, UINT dst_pitch, const BYTE* const src[3], UINT src_pitch)
{
	if (!(dst_pitch % 32) && !(src_pitch % 16)) {
		const ptrdiff_t srcStride[3] = { src_pitch, src_pitch / 2, src_pitch / 2 };
		uint8_t* dstData[2]          = { dst, dst + dst_pitch * h };
		const ptrdiff_t dstStride[2] = { dst_pitch, dst_pitch };

		convert_yuv420_nv12(src, srcStride, dstData, w, h, dstStride);

		return;
	}

	CopyPlane(h, dst, dst_pitch, src[0], src_pitch); // Y

	dst += dst_pitch * h;
	h /= 2;
	src_pitch /= 2;
	const UINT linesize = std::min(src_pitch, dst_pitch);

	const BYTE* srcU = src[1];
	const BYTE* srcV = src[2];

	for (UINT y = 0; y < h; ++y) {
		for (UINT x = 0; x < linesize; ++x) {
			dst[2 * x + 0] = srcU[x];
			dst[2 * x + 1] = srcV[x];
		}
		dst += dst_pitch;
		srcU += src_pitch;
		srcV += src_pitch;
	}
}

void CopyI420toYV12(UINT h, BYTE* dst, UINT dst_pitch, const BYTE* const src[3], UINT src_pitch)
{
	CopyPlane(h, dst, dst_pitch, src[0], src_pitch); // Y

	dst += dst_pitch * h;
	h /= 2;
	src_pitch /= 2;
	dst_pitch /= 2;
	CopyPlane(h, dst, dst_pitch, src[2], src_pitch); // V

	dst += dst_pitch * h;
	CopyPlane(h, dst, dst_pitch, src[1], src_pitch); // U
}

void ConvertI420toYUY2(UINT h, BYTE* dst, UINT dst_pitch, const BYTE* const src[3], UINT src_pitch, const bool bInterlaced)
{
	const int src_pitch_uv = src_pitch / 2;

	convert_yuv420p_to_yuy2(src[0], src[1], src[2], src_pitch, src_pitch, src_pitch_uv, dst, dst_pitch, h, bInterlaced);
}

void BlendPlane(BYTE* dst, BYTE* src, UINT w, UINT h, UINT dstpitch, UINT srcpitch)
{
	BlendPlane(dst, dstpitch, src, srcpitch, w, h);
}

void CopyOddLines(UINT h, BYTE* dst, UINT dst_pitch, BYTE* src, UINT src_pitch)
{
	const UINT linesize = std::min(src_pitch, dst_pitch);

	h /= 2;
	src_pitch *= 2;
	dst_pitch *= 2;

	for (UINT y = 0; y < h; ++y) {
		memcpy(dst, src, linesize);
		src += src_pitch;
		dst += dst_pitch;
	}
}

void AvgLines8_(BYTE* dst, DWORD h, DWORD pitch)
{
	if(h <= 1) return;

	BYTE* s = dst;
	BYTE* d = dst + (h-2)*pitch;

	for(; s < d; s += pitch*2) {
		BYTE* tmp = s;

		for(int i = pitch; i--; tmp++) {
				tmp[pitch] = (tmp[0] + tmp[pitch<<1] + 1) >> 1;
		}
	}

	if(!(h&1)) {
		dst += (h-2)*pitch;
		memcpy(dst + pitch, dst, pitch);
	}
}

void BobPlane(BYTE* dst, BYTE* src, UINT w, UINT h, UINT dstpitch, UINT srcpitch, bool topfield)
{
	if(topfield) {
		CopyOddLines(h, dst, dstpitch, src, srcpitch);
		AvgLines8_(dst, h, dstpitch);
	}
	else {
		CopyOddLines(h, dst + dstpitch, dstpitch, src + srcpitch, srcpitch);
		AvgLines8_(dst + dstpitch, h-1, dstpitch);
	}
}

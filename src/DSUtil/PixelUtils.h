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

#pragma once

extern void CopyPlane(const UINT h, BYTE* dst, UINT dst_pitch, const BYTE* src, UINT src_pitch);

extern void CopyI420toNV12(UINT w, UINT h, BYTE* dst, UINT dst_pitch, const BYTE* const src[3], UINT src_pitch);
extern void CopyI420toYV12(UINT h, BYTE* dst, UINT dst_pitch, const BYTE* const src[3], UINT src_pitch);

extern void ConvertI420toYUY2(UINT h, BYTE* dst, UINT dst_pitch, const BYTE* const src[3], UINT src_pitch, const bool bInterlaced);

extern void BlendPlane(BYTE* dst, BYTE* src, UINT w, UINT h, UINT dstpitch, UINT srcpitch);
extern void BobPlane(BYTE* dst, BYTE* src, UINT w, UINT h, UINT dstpitch, UINT srcpitch, bool topfield);

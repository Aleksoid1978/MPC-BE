/*
 * (C) 2006-2024 see Authors.txt
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

struct AVCodecContext;

// Bitmasks for DXVA compatibility check
#define DXVA_UNSUPPORTED_LEVEL			1
#define DXVA_TOO_MANY_REF_FRAMES		DXVA_UNSUPPORTED_LEVEL << 1
#define DXVA_INCOMPATIBLE_SAR			DXVA_UNSUPPORTED_LEVEL << 2
#define DXVA_PROFILE_HIGHER_THAN_HIGH	DXVA_UNSUPPORTED_LEVEL << 3
#define DXVA_HIGH_BIT					DXVA_UNSUPPORTED_LEVEL << 4

// === H264 functions
int FFH264CheckCompatibility(int nWidth, int nHeight, struct AVCodecContext* pAVCtx,
							 UINT nPCIVendor, UINT nPCIDevice, UINT64 VideoDriverVersion);

// === Common functions
void FillAVCodecProps(struct AVCodecContext* pAVCtx, BITMAPINFOHEADER* pBMI);

bool IsATIUVD(UINT nPCIVendor, UINT nPCIDevice);

// check frame size for DXVA for old drivers
BOOL DXVACheckFramesize(int width, int height, UINT nPCIVendor, UINT nPCIDevice, UINT64 VideoDriverVersion);

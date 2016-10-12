/*
 * (C) 2006-2016 see Authors.txt
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
struct AVFrame;

// Bitmasks for DXVA compatibility check
#define DXVA_UNSUPPORTED_LEVEL			1
#define DXVA_TOO_MANY_REF_FRAMES		DXVA_UNSUPPORTED_LEVEL << 1
#define DXVA_INCOMPATIBLE_SAR			DXVA_UNSUPPORTED_LEVEL << 2
#define DXVA_PROFILE_HIGHER_THAN_HIGH	DXVA_UNSUPPORTED_LEVEL << 3
#define DXVA_HIGH_BIT					DXVA_UNSUPPORTED_LEVEL << 4

// === H264 functions
int		FFH264CheckCompatibility(int nWidth, int nHeight, struct AVCodecContext* pAVCtx,
								 DWORD nPCIVendor, DWORD nPCIDevice, UINT64 VideoDriverVersion);

// === Mpeg2 functions
int		MPEG2CheckCompatibility(struct AVCodecContext* pAVCtx);

// === Common functions
HRESULT	FFGetCurFrame(struct AVCodecContext* pAVCtx, AVFrame** ppFrameOut);
UINT	FFGetMBCount(struct AVCodecContext* pAVCtx);

void	FillAVCodecProps(struct AVCodecContext* pAVCtx);

bool	IsATIUVD(DWORD nPCIVendor, DWORD nPCIDevice);
BOOL	DXVACheckFramesize(enum AVCodecID nCodecId, int width, int height,
						   DWORD nPCIVendor, DWORD nPCIDevice, UINT64 VideoDriverVersion);

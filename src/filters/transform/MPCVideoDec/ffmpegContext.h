/*
 * (C) 2006-2014 see Authors.txt
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

#include <dxva.h>

struct AVCodecContext;
struct AVFrame;

enum PCI_Vendors {
	PCIV_ATI			= 0x1002,
	PCIV_nVidia			= 0x10DE,
	PCIV_Intel			= 0x8086,
	PCIV_S3_Graphics	= 0x5333
};

#define PCID_Intel_HD2500    0x0152
#define PCID_Intel_HD4000    0x0162

// Bitmasks for DXVA compatibility check
#define DXVA_UNSUPPORTED_LEVEL			1
#define DXVA_TOO_MANY_REF_FRAMES		DXVA_UNSUPPORTED_LEVEL << 1
#define DXVA_INCOMPATIBLE_SAR			DXVA_UNSUPPORTED_LEVEL << 2
#define DXVA_PROFILE_HIGHER_THAN_HIGH	DXVA_UNSUPPORTED_LEVEL << 3
#define DXVA_HIGH_BIT					DXVA_UNSUPPORTED_LEVEL << 4

bool			IsATIUVD(DWORD nPCIVendor, DWORD nPCIDevice);
// === H264 functions
int				FFH264CheckCompatibility(int nWidth, int nHeight, struct AVCodecContext* pAVCtx,
										 DWORD nPCIVendor, DWORD nPCIDevice, LARGE_INTEGER VideoDriverVersion, bool nIsAtiDXVACompatible);
void			FFH264SetDxvaParams(struct AVCodecContext* pAVCtx, void* pDXVA_Context);

// === VC1 functions
void			FFVC1SetDxvaParams(struct AVCodecContext* pAVCtx, void* pDXVA_Context);

// === Mpeg2 functions
int				MPEG2CheckCompatibility(struct AVCodecContext* pAVCtx);
void			FFMPEG2SetDxvaParams(struct AVCodecContext* pAVCtx, void* pDXVA_Context);

// === HEVC functions
void			FFHEVCSetDxvaParams(struct AVCodecContext* pAVCtx, void* pDXVA_Context);

// === Common functions
HRESULT			FFDecodeFrame(struct AVCodecContext* pAVCtx, struct AVFrame* pFrame,
							  BYTE* pBuffer, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop,
							  int* got_picture_ptr, AVFrame** ppFrameOut);
BOOL			FFGetAlternateScan(struct AVCodecContext* pAVCtx);
UINT			FFGetMBCount(struct AVCodecContext* pAVCtx);
void			FFGetFrameProps(struct AVCodecContext* pAVCtx, struct AVFrame* pFrame, int& width, int& height);
BOOL			DXVACheckFramesize(enum AVCodecID nCodecId, int width, int height, DWORD nPCIVendor, DWORD nPCIDevice);

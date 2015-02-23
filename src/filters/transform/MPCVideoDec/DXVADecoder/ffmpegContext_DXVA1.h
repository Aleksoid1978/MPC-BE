/*
 * (C) 2006-2015 see Authors.txt
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

// === H264 functions
HRESULT			FFH264DecodeFrame(struct AVCodecContext* pAVCtx, struct AVFrame* pFrame, BYTE* pBuffer, UINT nSize, REFERENCE_TIME rtStart,
								  int* pFramePOC, int* pOutPOC, REFERENCE_TIME* pOutrtStart,
								  UINT* SecondFieldOffset, int* Sync, int* NALLength);
HRESULT			FFH264BuildPicParams(struct AVCodecContext* pAVCtx, DWORD nPCIVendor, DWORD nPCIDevice,
									 DXVA_PicParams_H264* pDXVAPicParams, DXVA_Qmatrix_H264* pDXVAScalingMatrix,
									 int* nPictStruct, bool IsATIUVD);
void			FFH264SetCurrentPicture(int nIndex, DXVA_PicParams_H264* pDXVAPicParams, struct AVCodecContext* pAVCtx);
void			FFH264UpdateRefFramesList(DXVA_PicParams_H264* pDXVAPicParams, struct AVCodecContext* pAVCtx);
BOOL			FFH264IsRefFrameInUse(int nFrameNum, struct AVCodecContext* pAVCtx);
void			FF264UpdateRefFrameSliceLong(DXVA_PicParams_H264* pDXVAPicParams, DXVA_Slice_H264_Long* pSlice, struct AVCodecContext* pAVCtx);
void			FFH264SetDxvaSliceLong(struct AVCodecContext* pAVCtx, void* pSliceLong);

// === VC1 functions
HRESULT			FFVC1DecodeFrame(DXVA_PictureParameters* pPicParams, struct AVCodecContext* pAVCtx, struct AVFrame* pFrame, REFERENCE_TIME rtStart,
								 BYTE* pBuffer, UINT nSize,
								 UINT* nFrameSize, BOOL b_SecondField);

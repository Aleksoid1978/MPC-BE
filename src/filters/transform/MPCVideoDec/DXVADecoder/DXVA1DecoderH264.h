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

#include "DXVA1Decoder.h"

#define MAX_SLICES 32	// Also define in ffmpeg!

class CDXVA1DecoderH264 : public CDXVA1Decoder
{
public:
	CDXVA1DecoderH264(CMPCVideoDecFilter* pFilter, IAMVideoAccelerator*  pAMVideoAccelerator, int nPicEntryNumber);

	virtual HRESULT 		DecodeFrame(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);
	virtual void			CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize);
	virtual void			Flush();

protected :
	virtual int				FindOldestFrame();

private:
	DXVA_PicParams_H264		m_DXVAPicParams;
	DXVA_Qmatrix_H264		m_DXVAScalingMatrix;
	DXVA_Slice_H264_Short	m_pSliceShort[MAX_SLICES];
	DXVA_Slice_H264_Long	m_pSliceLong[MAX_SLICES];
	UINT					m_nMaxSlices;
	int						m_nNALLength;
	bool					m_bUseLongSlice;
	int						m_nOutPOC;
	REFERENCE_TIME			m_rtOutStart;

	UINT					m_nSlices;

	// DXVA functions
	void					ClearUnusedRefFrames();

	int						m_nPictStruct;
	bool					m_IsATIUVD;
};

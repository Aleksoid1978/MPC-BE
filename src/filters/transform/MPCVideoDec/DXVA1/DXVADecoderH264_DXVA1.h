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

#include "../DXVADecoder.h"

#define MAX_SLICES 32 // Also define in ffmpeg!

class CDXVADecoderH264_DXVA1 : public CDXVADecoder
{
public:
	CDXVADecoderH264_DXVA1(CMPCVideoDecFilter* pFilter, IAMVideoAccelerator* pAMVideoAccelerator, const GUID* guidDecoder, DXVAMode nMode, int nPicEntryNumber);
	virtual ~CDXVADecoderH264_DXVA1();

	virtual HRESULT			DecodeFrame(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);
	virtual HRESULT			CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize, UINT nDXVASize = UINT_MAX);
	virtual void			Flush();

protected :
	virtual int				FindOldestFrame();
	virtual HRESULT			GetFreeSurfaceIndex(int& nSurfaceIndex, IMediaSample** ppSampleToDeliver, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);
	bool					AddToStore(int nSurfaceIndex, IMediaSample* pSample, bool bRefPicture, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, bool bIsField, int nCodecSpecific);

private:
	DXVA_PicParams_H264		m_DXVAPicParams;
	DXVA_Qmatrix_H264		m_DXVAScalingMatrix;
	DXVA_Slice_H264_Short	m_pSliceShort[MAX_SLICES];
	DXVA_Slice_H264_Long	m_pSliceLong[MAX_SLICES];
	int						m_nNALLength;
	int						m_nOutPOC;
	REFERENCE_TIME			m_rtOutStart;

	UINT					m_nSlices;
	bool					m_bFlushed;
	int						m_nFieldSurface;

	bool					m_IsATIUVD;

	void					Init();
	void					ClearUnusedRefFrames();
	void					RemoveRefFrame(int nSurfaceIndex);
};

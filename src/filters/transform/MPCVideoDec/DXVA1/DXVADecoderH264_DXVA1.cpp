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

#include "../stdafx.h"
#include "../../../../DSUtil/DSUtil.h"
#include "DXVADecoderH264_DXVA1.h"
#include "../MPCVideoDec.h"
#include "FfmpegContext_DXVA1.h"
#include "../FfmpegContext.h"
#include "../memcpy_sse.h"
#include <ffmpeg/libavcodec/avcodec.h>

#if 0
	#define TRACE_H264 TRACE
#else
	#define TRACE_H264(...)
#endif

CDXVADecoderH264_DXVA1::CDXVADecoderH264_DXVA1(CMPCVideoDecFilter* pFilter, IAMVideoAccelerator* pAMVideoAccelerator, const GUID* guidDecoder, DXVAMode nMode, int nPicEntryNumber)
	: CDXVADecoder(pFilter, pAMVideoAccelerator, guidDecoder, nMode, nPicEntryNumber)
{
	m_IsATIUVD = false;
	Init();
}

CDXVADecoderH264_DXVA1::~CDXVADecoderH264_DXVA1()
{
	DbgLog((LOG_TRACE, 3, L"CDXVADecoderH264_DXVA1::Destroy()"));
}

void CDXVADecoderH264_DXVA1::Init()
{
	DbgLog((LOG_TRACE, 3, L"CDXVADecoderH264_DXVA1::Init()"));

	memset(&m_DXVAPicParams, 0, sizeof(DXVA_PicParams_H264));
	memset(&m_pSliceLong, 0, sizeof(DXVA_Slice_H264_Long) * MAX_SLICES);
	memset(&m_pSliceShort, 0, sizeof(DXVA_Slice_H264_Short) * MAX_SLICES);

	m_DXVAPicParams.MbsConsecutiveFlag					= 1;
	m_DXVAPicParams.Reserved16Bits						= 3;
	if (m_pFilter->GetPCIVendor() == PCIV_Intel) {
		m_DXVAPicParams.Reserved16Bits					= 0x534c;
	} else if (IsATIUVD(m_pFilter->GetPCIVendor(), m_pFilter->GetPCIDevice())) {
		m_DXVAPicParams.Reserved16Bits					= 0;
		m_IsATIUVD										= true;
	}
	m_DXVAPicParams.ContinuationFlag					= 1;
	m_DXVAPicParams.MinLumaBipredSize8x8Flag			= 1;	// Improve accelerator performances
	m_DXVAPicParams.StatusReportFeedbackNumber			= 0;	// Use to report status

	for (int i = 0; i < _countof(m_DXVAPicParams.RefFrameList); i++) {
		m_DXVAPicParams.RefFrameList[i].bPicEntry		= 255;
	}

	m_nNALLength	= 4;
	m_nSlices		= 0;

	switch (GetMode()) {
		case H264_VLD :
			AllocExecuteParams(4);
			break;
		default :
			ASSERT(FALSE);
	}

	Flush();
}

HRESULT CDXVADecoderH264_DXVA1::CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize, UINT nDXVASize/* = UINT_MAX*/)
{
	CH264Nalu	Nalu;
	UINT		m_nSize		= nSize;
	int			nDxvaNalLength;

	m_nSlices = 0;

	Nalu.SetBuffer(pBuffer, m_nSize, m_nNALLength);
	nSize = 0;
	while (Nalu.ReadNext()) {
		switch (Nalu.GetType()) {
			case NALU_TYPE_SLICE:
			case NALU_TYPE_IDR:
				// Skip the NALU if the data length is below 0
				if ((int)Nalu.GetDataLength() < 0) {
					break;
				}

				// For AVC1, put startcode 0x000001
				pDXVABuffer[0] = pDXVABuffer[1] = 0; pDXVABuffer[2] = 1;

				// Copy NALU
				memcpy_sse(pDXVABuffer + 3, Nalu.GetDataBuffer(), Nalu.GetDataLength());

				// Update slice control buffer
				nDxvaNalLength									= Nalu.GetDataLength() + 3;
				m_pSliceShort[m_nSlices].BSNALunitDataLocation	= nSize;
				m_pSliceShort[m_nSlices].SliceBytesInBuffer		= nDxvaNalLength;

				nSize											+= nDxvaNalLength;
				pDXVABuffer										+= nDxvaNalLength;
				m_nSlices++;
				break;
		}
	}

	// Complete bitstream buffer with zero padding (buffer size should be a multiple of 128)
	if (nSize % 128) {
		int nDummy = 128 - (nSize % 128);

		memset(pDXVABuffer, 0, nDummy);
		m_pSliceShort[m_nSlices-1].SliceBytesInBuffer	+= nDummy;
		nSize											+= nDummy;
	}

	return S_OK;
}

void CDXVADecoderH264_DXVA1::Flush()
{
	m_DXVAPicParams.UsedForReferenceFlags	= 0;
	m_nOutPOC								= INT_MIN;
	m_rtOutStart							= INVALID_TIME;

	m_nFieldSurface							= -1;
	m_bFlushed								= true;

	__super::Flush();
}

HRESULT CDXVADecoderH264_DXVA1::DecodeFrame(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	HRESULT					hr					= S_FALSE;
	int						nSurfaceIndex		= -1;
	int						nFramePOC			= INT_MIN;
	int						nOutPOC				= INT_MIN;
	REFERENCE_TIME			rtOutStart			= INVALID_TIME;
	CH264Nalu				Nalu;
	CComPtr<IMediaSample>	pSampleToDeliver;

	CHECK_HR_FALSE (FFH264DecodeFrame(m_pFilter->GetAVCtx(), m_pFilter->GetFrame(), pDataIn, nSize, rtStart,
					&nFramePOC, &nOutPOC, &rtOutStart, &m_nNALLength));

	// If parsing fail (probably no PPS/SPS), continue anyway it may arrived later (happen on truncated streams)
	CHECK_HR_FALSE (FFH264BuildPicParams(m_pFilter->GetAVCtx(), &m_DXVAPicParams, &m_DXVAScalingMatrix, m_IsATIUVD));

	TRACE_H264 ("CDXVADecoderH264_DXVA1::DecodeFrame() : nFramePOC = %11d, nOutPOC = %11d[%11d], [%d - %d], rtOutStart = [%20I64d]\n", nFramePOC, nOutPOC, m_nOutPOC, m_DXVAPicParams.field_pic_flag, m_DXVAPicParams.RefPicFlag, rtOutStart);

	// Wait I frame after a flush
	if (m_bFlushed && !m_DXVAPicParams.IntraPicFlag) {
		TRACE_H264 ("CDXVADecoderH264_DXVA1::DecodeFrame() : Flush - wait I frame\n");
		return S_FALSE;
	}

	CHECK_HR_FALSE (GetFreeSurfaceIndex(nSurfaceIndex, &pSampleToDeliver, rtStart, rtStop));
	FFH264SetCurrentPicture(nSurfaceIndex, &m_DXVAPicParams, m_pFilter->GetAVCtx());

	{
		m_DXVAPicParams.StatusReportFeedbackNumber++;

		CHECK_HR_FALSE (BeginFrame(nSurfaceIndex, pSampleToDeliver));
		// Send picture parameters
		CHECK_HR_FALSE (AddExecuteBuffer(DXVA2_PictureParametersBufferType, sizeof(m_DXVAPicParams), &m_DXVAPicParams));
		// Add bitstream
		CHECK_HR_FALSE (AddExecuteBuffer(DXVA2_BitStreamDateBufferType, nSize, pDataIn));
		// Add quantization matrix
		CHECK_HR_FALSE (AddExecuteBuffer(DXVA2_InverseQuantizationMatrixBufferType, sizeof(DXVA_Qmatrix_H264), &m_DXVAScalingMatrix));
		// Add slice control
		CHECK_HR_FALSE (AddExecuteBuffer(DXVA2_SliceControlBufferType, sizeof(DXVA_Slice_H264_Short) * m_nSlices, m_pSliceShort));
		// Decode frame
		CHECK_HR_FALSE (Execute());
		CHECK_HR_FALSE (EndFrame(nSurfaceIndex));
	}

	bool bAdded = AddToStore(nSurfaceIndex, pSampleToDeliver, m_DXVAPicParams.RefPicFlag, rtStart, rtStop,
							 m_DXVAPicParams.field_pic_flag, nFramePOC);


	FFH264UpdateRefFramesList(&m_DXVAPicParams, m_pFilter->GetAVCtx());
	ClearUnusedRefFrames();

	if (bAdded) {
		hr = DisplayNextFrame();
	}

	if (nOutPOC != INT_MIN) {
		m_nOutPOC		= nOutPOC;
		m_rtOutStart	= rtOutStart;
	}

	m_bFlushed = false;
	return hr;
}

void CDXVADecoderH264_DXVA1::ClearUnusedRefFrames()
{
	// Remove old reference frames (not anymore a short or long ref frame)
	for (int i = 0; i < m_nPicEntryNumber; i++) {
		if (m_pPictureStore[i].bRefPicture && m_pPictureStore[i].bDisplayed) {
			if (!FFH264IsRefFrameInUse(i, m_pFilter->GetAVCtx())) {
				RemoveRefFrame(i);
			}
		}
	}
}

void CDXVADecoderH264_DXVA1::RemoveRefFrame(int nSurfaceIndex)
{
	ASSERT(nSurfaceIndex < m_nPicEntryNumber);

	m_pPictureStore[nSurfaceIndex].bRefPicture = false;
	if (m_pPictureStore[nSurfaceIndex].bDisplayed) {
		FreePictureSlot(nSurfaceIndex);
	}
}


int CDXVADecoderH264_DXVA1::FindOldestFrame()
{
	int				nPos	= -1;
	REFERENCE_TIME	rtPos	= _I64_MAX;

	for (int i = 0; i < m_nPicEntryNumber; i++) {
		if (m_pPictureStore[i].bInUse && !m_pPictureStore[i].bDisplayed) {
			if ((m_pPictureStore[i].nCodecSpecific == m_nOutPOC) && (m_pPictureStore[i].rtStart < rtPos) && (m_pPictureStore[i].rtStart >= m_rtOutStart)) {
				nPos  = i;
				rtPos = m_pPictureStore[i].rtStart;
			}
		}
	}

	if (nPos != -1) {
		m_pPictureStore[nPos].rtStart = m_rtOutStart;
		m_pPictureStore[nPos].rtStop = INVALID_TIME;
		m_pFilter->ReorderBFrames(m_pPictureStore[nPos].rtStart, m_pPictureStore[nPos].rtStop);
		m_pFilter->UpdateFrameTime(m_pPictureStore[nPos].rtStart, m_pPictureStore[nPos].rtStop);
	}

	return nPos;
}

HRESULT CDXVADecoderH264_DXVA1::GetFreeSurfaceIndex(int& nSurfaceIndex, IMediaSample** ppSampleToDeliver, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	int		nPos			= -1;
	DWORD	dwMinDisplay	= MAXDWORD;
	*ppSampleToDeliver		= NULL;

	if (m_nFieldSurface != -1) {
		nSurfaceIndex = m_nFieldSurface;
		return S_FALSE;
	}

	for (int i = 0; i < m_nPicEntryNumber; i++) {
		if (!m_pPictureStore[i].bInUse && m_pPictureStore[i].dwDisplayCount < dwMinDisplay) {
			dwMinDisplay = m_pPictureStore[i].dwDisplayCount;
			nPos  = i;
		}
	}

	if (nPos != -1) {
		nSurfaceIndex = nPos;
		return S_OK;
	}

	// Ho ho...
	ASSERT(FALSE);
	Flush();

	return E_UNEXPECTED;
}

bool CDXVADecoderH264_DXVA1::AddToStore(int nSurfaceIndex, IMediaSample* pSample, bool bRefPicture,
										REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, bool bIsField,
										int nCodecSpecific)
{
	if (bIsField && m_nFieldSurface == -1) {
		m_nFieldSurface									= nSurfaceIndex;
		m_pPictureStore[nSurfaceIndex].rtStart			= rtStart;
		m_pPictureStore[nSurfaceIndex].rtStop			= rtStop;
		m_pPictureStore[nSurfaceIndex].nCodecSpecific	= nCodecSpecific;
		return false;
	} else {
		ASSERT(nSurfaceIndex < m_nPicEntryNumber);

		m_pPictureStore[nSurfaceIndex].bRefPicture		= bRefPicture;
		m_pPictureStore[nSurfaceIndex].bInUse			= true;
		m_pPictureStore[nSurfaceIndex].bDisplayed		= false;
		m_pPictureStore[nSurfaceIndex].pSample			= pSample;

		if (!bIsField) {
			m_pPictureStore[nSurfaceIndex].rtStart			= rtStart;
			m_pPictureStore[nSurfaceIndex].rtStop			= rtStop;
			m_pPictureStore[nSurfaceIndex].nCodecSpecific	= nCodecSpecific;
		}

		m_nFieldSurface	= -1;
		return true;
	}
}

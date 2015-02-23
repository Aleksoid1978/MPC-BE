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

#include "stdafx.h"
#include "DXVA1DecoderH264.h"
#include "../MPCVideoDec.h"
#include "../FfmpegContext.h"
#include "FfmpegContext_DXVA1.h"
#include <ffmpeg/libavcodec/avcodec.h>

#if 0
	#define TRACE_H264 TRACE
#else
	#define TRACE_H264 __noop
#endif

CDXVA1DecoderH264::CDXVA1DecoderH264(CMPCVideoDecFilter* pFilter, IAMVideoAccelerator*  pAMVideoAccelerator, int nPicEntryNumber)
	: CDXVA1Decoder(pFilter, pAMVideoAccelerator, nPicEntryNumber)
	, m_bUseLongSlice(m_DXVA1Config.bConfigBitstreamRaw != 2)
	, m_nNALLength(4)
	, m_nMaxSlices(0)
	, m_nSlices(0)
	, m_IsATIUVD(false)
{
	memset (&m_DXVAPicParams, 0, sizeof(DXVA_PicParams_H264));
	memset (&m_pSliceLong,    0, sizeof(DXVA_Slice_H264_Long) * MAX_SLICES);
	memset (&m_pSliceShort,   0, sizeof(DXVA_Slice_H264_Short) * MAX_SLICES);

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

	if (m_bUseLongSlice) {
		FFH264SetDxvaSliceLong(m_pFilter->GetAVCtx(), m_pSliceLong);
	}

	Flush();
}

void CDXVA1DecoderH264::CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize)
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
				memcpy(pDXVABuffer + 3, Nalu.GetDataBuffer(), Nalu.GetDataLength());

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
		m_pSliceShort[m_nSlices - 1].SliceBytesInBuffer	+= nDummy;
		nSize											+= nDummy;		
	}
}

void CDXVA1DecoderH264::Flush()
{
	m_DXVAPicParams.UsedForReferenceFlags	= 0;
	m_nOutPOC								= INT_MIN;
	m_rtOutStart							= INVALID_TIME;
	m_nPictStruct							= PICT_NONE;

	__super::Flush();
}

HRESULT CDXVA1DecoderH264::DecodeFrame(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	HRESULT			hr					= S_FALSE;
	UINT			nSlices				= 0;
	int				nSurfaceIndex		= -1;
	int				nFramePOC			= INT_MIN;
	int				nOutPOC				= INT_MIN;
	REFERENCE_TIME	rtOutStart			= INVALID_TIME;
	UINT			nNalOffset			= 0;
	UINT			SecondFieldOffset	= 0;
	UINT			nSize_Result		= 0;
	int				Sync				= 0;
	int				nPictStruct			= PICT_NONE;
	CH264Nalu		Nalu;

	CHECK_HR_FALSE (FFH264DecodeFrame(m_pFilter->GetAVCtx(), m_pFilter->GetFrame(), pDataIn, nSize, rtStart, 
					&nFramePOC, &nOutPOC, &rtOutStart, 
					&SecondFieldOffset, &Sync, &m_nNALLength));

	Nalu.SetBuffer(pDataIn, nSize, m_nNALLength);
	while (Nalu.ReadNext()) {
		switch (Nalu.GetType()) {
			case NALU_TYPE_SLICE:
			case NALU_TYPE_IDR:
				if (m_bUseLongSlice) {
					m_pSliceLong[nSlices].BSNALunitDataLocation	= nNalOffset;
					m_pSliceLong[nSlices].SliceBytesInBuffer	= Nalu.GetDataLength() + 3;
					m_pSliceLong[nSlices].slice_id				= nSlices;
					FF264UpdateRefFrameSliceLong(&m_DXVAPicParams, &m_pSliceLong[nSlices], m_pFilter->GetAVCtx());

					if (nSlices) {
						m_pSliceLong[nSlices-1].NumMbsForSlice = m_pSliceLong[nSlices].NumMbsForSlice = m_pSliceLong[nSlices].first_mb_in_slice - m_pSliceLong[nSlices - 1].first_mb_in_slice;
					}
				}
				nSlices++;
				nNalOffset += (UINT)(Nalu.GetDataLength() + 3);
				if (nSlices > MAX_SLICES) {
					break;
				}
				break;
		}
	}

	if (!nSlices) {
		return S_FALSE;
	}

	// If parsing fail (probably no PPS/SPS), continue anyway it may arrived later (happen on truncated streams)
	CHECK_HR_FALSE (FFH264BuildPicParams(m_pFilter->GetAVCtx(), m_pFilter->GetPCIVendor(), m_pFilter->GetPCIDevice(), &m_DXVAPicParams, &m_DXVAScalingMatrix, &nPictStruct, m_IsATIUVD));

	TRACE_H264 ("CDXVA1DecoderH264::DecodeFrame() : nFramePOC = %11d, nOutPOC = %11d[%11d], [%d - %d], rtOutStart = [%20I64d]\n", nFramePOC, nOutPOC, m_nOutPOC, m_DXVAPicParams.field_pic_flag, m_DXVAPicParams.RefPicFlag, rtOutStart);

	if (m_DXVAPicParams.field_pic_flag && m_nPictStruct == nPictStruct && !SecondFieldOffset) {
		TRACE_H264 ("		CDXVA1DecoderH264::DecodeFrame() : broken frame\n");
		return S_FALSE;
	}

	m_nPictStruct = nPictStruct;

	m_nMaxWaiting = min(max(m_DXVAPicParams.num_ref_frames, 3), 8);

	if (!m_DXVAPicParams.field_pic_flag && nOutPOC == INT_MIN && m_nOutPOC != INT_MIN && !m_bFlushed && !m_DXVAPicParams.IntraPicFlag) {
		TRACE_H264 ("		CDXVA1DecoderH264::DecodeFrame() : Skip frame\n");
		return S_FALSE;
	}

	// Wait I frame after a flush
	if (m_bFlushed && !(m_DXVAPicParams.IntraPicFlag || (Sync && SecondFieldOffset))) {
		TRACE_H264 ("CDXVA1DecoderH264::DecodeFrame() : Flush - wait I frame\n");
		return S_FALSE;
	}

	CHECK_HR_FALSE (GetFreeSurfaceIndex(nSurfaceIndex));
	FFH264SetCurrentPicture(nSurfaceIndex, &m_DXVAPicParams, m_pFilter->GetAVCtx());

	bool bAdded = false;
	{
		m_DXVAPicParams.StatusReportFeedbackNumber++;
		
		// Begin frame
		CHECK_HR_FALSE (BeginFrame(nSurfaceIndex));
		// Send picture parameters
		CHECK_HR_FRAME_DXVA1 (AddExecuteBuffer(DXVA_PICTURE_DECODE_BUFFER, sizeof(DXVA_PicParams_H264), &m_DXVAPicParams));
		// Send quantization matrix
		CHECK_HR_FRAME_DXVA1 (AddExecuteBuffer(DXVA_INVERSE_QUANTIZATION_MATRIX_BUFFER, sizeof(DXVA_Qmatrix_H264), &m_DXVAScalingMatrix));
		// Send bitstream
		CHECK_HR_FRAME_DXVA1 (AddExecuteBuffer(DXVA_BITSTREAM_DATA_BUFFER, SecondFieldOffset ? SecondFieldOffset : nSize, pDataIn, &nSize_Result));
		// Send slice control
		if (m_bUseLongSlice) {
			CHECK_HR_FRAME_DXVA1 (AddExecuteBuffer(DXVA_SLICE_CONTROL_BUFFER, sizeof(DXVA_Slice_H264_Long) * m_nSlices, m_pSliceLong));
		} else {
			CHECK_HR_FRAME_DXVA1 (AddExecuteBuffer(DXVA_SLICE_CONTROL_BUFFER, sizeof(DXVA_Slice_H264_Short) * m_nSlices, m_pSliceShort));
		}
		// Decode frame
		CHECK_HR_FRAME_DXVA1 (Execute());
		CHECK_HR_FALSE (EndFrame(nSurfaceIndex));

		if (SecondFieldOffset) {
			
			// Begin frame
			CHECK_HR_FALSE (BeginFrame(nSurfaceIndex));
			// Send picture parameters
			CHECK_HR_FRAME_DXVA1 (AddExecuteBuffer(DXVA_PICTURE_DECODE_BUFFER, sizeof(DXVA_PicParams_H264), &m_DXVAPicParams));
			// Send quantization matrix
			CHECK_HR_FRAME_DXVA1 (AddExecuteBuffer(DXVA_INVERSE_QUANTIZATION_MATRIX_BUFFER, sizeof(DXVA_Qmatrix_H264), &m_DXVAScalingMatrix));
			// Send bitstream
			CHECK_HR_FRAME_DXVA1 (AddExecuteBuffer(DXVA_BITSTREAM_DATA_BUFFER, nSize - SecondFieldOffset, pDataIn + SecondFieldOffset, &nSize_Result));
			// Send slice control
			if (m_bUseLongSlice) {
				CHECK_HR_FRAME_DXVA1 (AddExecuteBuffer(DXVA_SLICE_CONTROL_BUFFER, sizeof(DXVA_Slice_H264_Long) * m_nSlices, m_pSliceLong));
			} else {
				CHECK_HR_FRAME_DXVA1 (AddExecuteBuffer(DXVA_SLICE_CONTROL_BUFFER, sizeof(DXVA_Slice_H264_Short) * m_nSlices, m_pSliceShort));
			}
			// Decode frame
			CHECK_HR_FRAME_DXVA1 (Execute());
			CHECK_HR_FALSE (EndFrame(nSurfaceIndex));

			bAdded = AddToStore(nSurfaceIndex, m_DXVAPicParams.RefPicFlag,
								rtStart, rtStop,
								false, nFramePOC);

		} else {
			bAdded = AddToStore(nSurfaceIndex, m_DXVAPicParams.RefPicFlag,
								rtStart, rtStop,
								m_DXVAPicParams.field_pic_flag, nFramePOC);
		}
	}

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

void CDXVA1DecoderH264::ClearUnusedRefFrames()
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

int CDXVA1DecoderH264::FindOldestFrame()
{
	int				nPos  = -1;
	REFERENCE_TIME	rtPos = _I64_MAX;

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

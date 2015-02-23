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
#include "DXVA1DecoderVC1.h"
#include "../MPCVideoDec.h"
#include "ffmpegContext_DXVA1.h"
#include <ffmpeg/libavcodec/avcodec.h>
#include <vector>

#if 0
	#define TRACE_VC1 TRACE
#else
	#define TRACE_VC1 __noop
#endif

CDXVA1DecoderVC1::CDXVA1DecoderVC1(CMPCVideoDecFilter* pFilter, IAMVideoAccelerator*  pAMVideoAccelerator, int nPicEntryNumber)
	: CDXVA1Decoder(pFilter, pAMVideoAccelerator, nPicEntryNumber)
{
	memset(&m_PictureParams, 0, sizeof(m_PictureParams));
	memset(&m_SliceInfo, 0, sizeof(m_SliceInfo));

	m_PictureParams.bMacroblockWidthMinus1	= 15;
	m_PictureParams.bMacroblockHeightMinus1	= 15;
	m_PictureParams.bBlockWidthMinus1		= 7;
	m_PictureParams.bBlockHeightMinus1		= 7;
	m_PictureParams.bBPPminus1				= 7;
	m_PictureParams.bChromaFormat			= 1;

	// iWMV9 - i9IRU - iOHIT - iINSO - iWMVA - 0 - 0 - 0			| Section 3.2.5
	m_PictureParams.bBidirectionalAveragingMode = (1                                         << 7) |
												  (m_DXVA1Config.bConfigIntraResidUnsigned   << 6) |	// i9IRU
												  (m_DXVA1Config.bConfigResidDiffAccelerator << 5);		// iOHIT

	m_nMaxWaiting		  = 5;
	m_wRefPictureIndex[0] = NO_REF_FRAME;
	m_wRefPictureIndex[1] = NO_REF_FRAME;

	Flush();
}

// === Public functions
HRESULT CDXVA1DecoderVC1::DecodeFrame(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	HRESULT	hr				= S_FALSE;
	int		nSurfaceIndex	= -1;
	UINT	nFrameSize		= 0;
	UINT	nSize_Result	= 0;

	CHECK_HR_FALSE (FFVC1DecodeFrame(&m_PictureParams, m_pFilter->GetAVCtx(), m_pFilter->GetFrame(), rtStart, 
									 pDataIn, nSize, 
									 &nFrameSize, FALSE));

	// Wait I frame after a flush
	if (m_bFlushed && !m_PictureParams.bPicIntra) {
		return S_FALSE;
	}

	BYTE bPicBackwardPrediction = m_PictureParams.bPicBackwardPrediction;

	CHECK_HR_FALSE (GetFreeSurfaceIndex(nSurfaceIndex));

	// Begin frame
	CHECK_HR_FALSE (BeginFrame(nSurfaceIndex));

	TRACE_VC1 ("CDXVA1DecoderVC1::DecodeFrame() : rtStart = %10I64d, Surf = %d\n", rtStart, nSurfaceIndex);

	m_PictureParams.wDecodedPictureIndex	= nSurfaceIndex;
	m_PictureParams.wDeblockedPictureIndex	= m_PictureParams.wDecodedPictureIndex;

	// Manage reference picture list
	if (!m_PictureParams.bPicBackwardPrediction) {
		if (m_wRefPictureIndex[0] != NO_REF_FRAME) {
			RemoveRefFrame(m_wRefPictureIndex[0]);
		}
		m_wRefPictureIndex[0] = m_wRefPictureIndex[1];
		m_wRefPictureIndex[1] = nSurfaceIndex;
	}
	m_PictureParams.wForwardRefPictureIndex		= (m_PictureParams.bPicIntra == 0)				? m_wRefPictureIndex[0] : NO_REF_FRAME;
	m_PictureParams.wBackwardRefPictureIndex	= (m_PictureParams.bPicBackwardPrediction == 1) ? m_wRefPictureIndex[1] : NO_REF_FRAME;

	m_PictureParams.bPic4MVallowed				= (m_PictureParams.wBackwardRefPictureIndex == NO_REF_FRAME && m_PictureParams.bPicStructure == 3) ? 1 : 0;
	m_PictureParams.bPicDeblockConfined		   |= (m_PictureParams.wBackwardRefPictureIndex == NO_REF_FRAME) ? 0x04 : 0;

	m_PictureParams.bPicScanMethod++;			// Use for status reporting sections 3.8.1 and 3.8.2

	TRACE_VC1 ("CDXVA1DecoderVC1::DecodeFrame() : Decode frame %i\n", m_PictureParams.bPicScanMethod);

	// Send picture parameters
	CHECK_HR_FRAME_DXVA1 (AddExecuteBuffer(DXVA_PICTURE_DECODE_BUFFER, sizeof(DXVA_PictureParameters), &m_PictureParams));
	// Send bitstream
	CHECK_HR_FRAME_DXVA1 (AddExecuteBuffer(DXVA_BITSTREAM_DATA_BUFFER, nFrameSize ? nFrameSize : nSize, pDataIn, &nSize_Result));
	// Send slice control
	m_SliceInfo.wQuantizerScaleCode	= 1;
	m_SliceInfo.dwSliceBitsInBuffer	= nSize_Result * 8;
	CHECK_HR_FRAME_DXVA1 (AddExecuteBuffer(DXVA_SLICE_CONTROL_BUFFER, sizeof(DXVA_SliceInfo), &m_SliceInfo));

	// Decode frame
	CHECK_HR_FRAME_DXVA1 (Execute());
	CHECK_HR_FALSE (EndFrame(nSurfaceIndex));

	if (nFrameSize) { // Decoding Second Field
		FFVC1DecodeFrame(&m_PictureParams, m_pFilter->GetAVCtx(), m_pFilter->GetFrame(), rtStart,
						 pDataIn, nSize,
						 NULL, TRUE);

		// Begin frame
		CHECK_HR_FALSE (BeginFrame(nSurfaceIndex));
		// Send picture parameters
		CHECK_HR_FRAME_DXVA1 (AddExecuteBuffer(DXVA_PICTURE_DECODE_BUFFER, sizeof(DXVA_PictureParameters), &m_PictureParams));
		// Send bitstream
		CHECK_HR_FRAME_DXVA1 (AddExecuteBuffer(DXVA_BITSTREAM_DATA_BUFFER, nSize - nFrameSize, pDataIn + nFrameSize, &nSize_Result));
		// Send slice control
		m_SliceInfo.wQuantizerScaleCode	= 1;
		m_SliceInfo.dwSliceBitsInBuffer	= nSize_Result * 8;
		CHECK_HR_FRAME_DXVA1 (AddExecuteBuffer(DXVA_SLICE_CONTROL_BUFFER, sizeof(DXVA_SliceInfo), &m_SliceInfo));
		// Decode frame
		CHECK_HR_FRAME_DXVA1 (Execute());
		CHECK_HR_FALSE (EndFrame(nSurfaceIndex));
	}

	// Update timestamp & Re-order B frames
	m_pFilter->UpdateFrameTime(rtStart, rtStop);
	if (m_pFilter->IsReorderBFrame() && m_pFilter->GetAVCtx()->has_b_frames) {
		if (bPicBackwardPrediction == 1) {
			std::swap(rtStart, m_rtStartDelayed);
			std::swap(rtStop, m_rtStopDelayed);
		} else {
			// Save I or P reference time (swap later)
			if (!m_bFlushed) {
				if (m_nDelayedSurfaceIndex != -1) {
					UpdateStore(m_nDelayedSurfaceIndex, m_rtStartDelayed, m_rtStopDelayed);
				}
				m_rtStartDelayed = m_rtStopDelayed = _I64_MAX;
				std::swap(rtStart, m_rtStartDelayed);
				std::swap(rtStop, m_rtStopDelayed);
				m_nDelayedSurfaceIndex = nSurfaceIndex;
			}
		}
	}

	AddToStore(nSurfaceIndex, (bPicBackwardPrediction != 1), rtStart, rtStop, false, 0);

	m_bFlushed = false;

	return DisplayNextFrame();
}

BYTE* CDXVA1DecoderVC1::FindNextStartCode(BYTE* pBuffer, UINT nSize, UINT& nPacketSize)
{
	BYTE*	pStart	= pBuffer;
	BYTE	bCode	= 0;
	for (UINT i = 0; i < nSize - 4; i++) {
		if (((*((DWORD*)(pBuffer + i)) & 0x00FFFFFF) == 0x00010000) || (i >= nSize - 5)) {
			if (bCode == 0) {
				bCode = pBuffer[i + 3];
				if ((nSize == 5) && (bCode == 0x0D)) {
					nPacketSize = nSize;
					return pBuffer;
				}
			} else {
				if (bCode == 0x0D) {
					// Start code found!
					nPacketSize = i - (pStart - pBuffer) + (i >= nSize - 5 ? 5 : 1);
					return pStart;
				} else {
					// Other stuff, ignore it
					pStart	= pBuffer + i;
					bCode	= pBuffer[i + 3];
				}
			}
		}
	}

	ASSERT(FALSE);		// Should never happen!

	return NULL;
}

void CDXVA1DecoderVC1::CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize)
{
	if (m_PictureParams.bSecondField) {
		memcpy(pDXVABuffer, (BYTE*)pBuffer, nSize);
	} else {
		if ((*((DWORD*)pBuffer) & 0x00FFFFFF) != 0x00010000) {
			if (m_pFilter->GetCodec() == AV_CODEC_ID_WMV3) {
				memcpy(pDXVABuffer, (BYTE*)pBuffer, nSize);
			} else {
				pDXVABuffer[0] = pDXVABuffer[1] = 0;
				pDXVABuffer[2] = 1;
				pDXVABuffer[3] = 0x0D;
				memcpy(pDXVABuffer + 4, (BYTE*)pBuffer, nSize);
				nSize +=4;
			}
		} else {
			BYTE*	pStart;
			UINT	nPacketSize;

			pStart = FindNextStartCode(pBuffer, nSize, nPacketSize);
			if (pStart) {
				// Startcode already present
				memcpy(pDXVABuffer, (BYTE*)pStart, nPacketSize);
				nSize = nPacketSize;
			}
		}
	}

	// Complete bitstream buffer with zero padding (buffer size should be a multiple of 128)
	if (nSize % 128) {
		int nDummy = 128 - (nSize % 128);
		
		pDXVABuffer += nSize;
		memset(pDXVABuffer, 0, nDummy);
		nSize += nDummy;
	}
}

void CDXVA1DecoderVC1::Flush()
{
	m_nDelayedSurfaceIndex	= -1;
	m_rtStartDelayed		= _I64_MAX;
	m_rtStopDelayed			= _I64_MAX;

	if (m_wRefPictureIndex[0] != NO_REF_FRAME) {
		RemoveRefFrame(m_wRefPictureIndex[0]);
	}
	if (m_wRefPictureIndex[1] != NO_REF_FRAME) {
		RemoveRefFrame(m_wRefPictureIndex[1]);
	}

	m_wRefPictureIndex[0]	= NO_REF_FRAME;
	m_wRefPictureIndex[1]	= NO_REF_FRAME;

	__super::Flush();
}

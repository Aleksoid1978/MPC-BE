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
#include "../ffmpegContext.h"
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
	memset(&m_DXVA_Context, 0, sizeof(m_DXVA_Context));

	m_wRefPictureIndex[0] = NO_REF_FRAME;
	m_wRefPictureIndex[1] = NO_REF_FRAME;

	FFVC1SetDxvaParams(m_pFilter->GetAVCtx(), &m_DXVA_Context);

	Flush();
}

void CDXVA1DecoderVC1::Flush()
{
	StatusReportFeedbackNumber	= 0;
	
	m_nDelayedSurfaceIndex		= -1;
	m_rtStartDelayed			= _I64_MAX;
	m_rtStopDelayed				= _I64_MAX;

	if (m_wRefPictureIndex[0] != NO_REF_FRAME) {
		RemoveRefFrame(m_wRefPictureIndex[0]);
	}
	if (m_wRefPictureIndex[1] != NO_REF_FRAME) {
		RemoveRefFrame(m_wRefPictureIndex[1]);
	}

	m_wRefPictureIndex[0]		= NO_REF_FRAME;
	m_wRefPictureIndex[1]		= NO_REF_FRAME;

	__super::Flush();
}

void CDXVA1DecoderVC1::CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize)
{
	DXVA_VC1_Picture_Context *ctx_pic	= &m_DXVA_Context.ctx_pic[m_nFieldNum];
	DXVA_SliceInfo *slice				= &ctx_pic->slice;
	BYTE* current						= pDXVABuffer;

    static const uint8_t start_code[]	= { 0, 0, 1, 0x0d };
    const unsigned start_code_size		= m_pFilter->GetCodec() == AV_CODEC_ID_VC1 ? sizeof(start_code) : 0;
    const unsigned slice_size			= slice->dwSliceBitsInBuffer / 8;
    const unsigned padding				= 128 - ((start_code_size + slice_size) & 127);
	nSize								= start_code_size + slice_size + padding;

	if (start_code_size > 0) {
		memcpy(pDXVABuffer, start_code, start_code_size);
		if (m_nFieldNum == 1) {
			pDXVABuffer[3] = 0x0c;
		}
	}
	memcpy(pDXVABuffer + start_code_size, ctx_pic->bitstream + slice->dwSliceDataLocation, slice_size);
	if (padding > 0) {
		memset(pDXVABuffer + start_code_size + slice_size, 0, padding);
	}
	slice->dwSliceBitsInBuffer = 8 * nSize;
}

HRESULT CDXVA1DecoderVC1::DecodeFrame(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	HRESULT	hr				= S_FALSE;
	int		nSurfaceIndex	= -1;
	int		got_picture		= 0;

	memset(&m_DXVA_Context, 0, sizeof(m_DXVA_Context));
	m_DXVA_Context.ctx_pic[0].pp.bBidirectionalAveragingMode =
	m_DXVA_Context.ctx_pic[1].pp.bBidirectionalAveragingMode = (1                                         << 7) |
															   (m_DXVA1Config.bConfigIntraResidUnsigned   << 6) |
															   (m_DXVA1Config.bConfigResidDiffAccelerator << 5);

	AVFrame* pFrame;
	CHECK_HR_FALSE (FFDecodeFrame(m_pFilter->GetAVCtx(), m_pFilter->GetFrame(),
								  pDataIn, nSize, rtStart, rtStop,
							      &got_picture, &pFrame));

	if (!pFrame || !m_DXVA_Context.frame_count) {
		return S_FALSE;
	}

	// Wait I frame after a flush
	if (m_bFlushed && !m_DXVA_Context.ctx_pic[0].pp.bPicIntra) {
		return S_FALSE;
	}

	BYTE bPicBackwardPrediction = m_DXVA_Context.ctx_pic[0].pp.bPicBackwardPrediction;

	CHECK_HR_FALSE (GetFreeSurfaceIndex(nSurfaceIndex));

	for (UINT i = 0; i < m_DXVA_Context.frame_count; i++) {

		DXVA_VC1_Picture_Context *ctx_pic = &m_DXVA_Context.ctx_pic[i];

		m_nFieldNum = i;
		UpdatePictureParams(nSurfaceIndex);

		StatusReportFeedbackNumber++;
		if (StatusReportFeedbackNumber >= (1 << 16)) {
			StatusReportFeedbackNumber = 1;
		}
		ctx_pic->pp.bPicScanFixed	= StatusReportFeedbackNumber >> 8;
		ctx_pic->pp.bPicScanMethod	= StatusReportFeedbackNumber & 0xff;

		// Begin frame
		CHECK_HR_FALSE (BeginFrame(nSurfaceIndex));
		// Send picture parameters
		CHECK_HR_FRAME_DXVA1 (AddExecuteBuffer(DXVA_PICTURE_DECODE_BUFFER, sizeof(DXVA_PictureParameters), &ctx_pic->pp));
		// Send bitstream
		CHECK_HR_FRAME_DXVA1 (AddExecuteBuffer(DXVA_BITSTREAM_DATA_BUFFER));
		// Send slice control
		CHECK_HR_FRAME_DXVA1 (AddExecuteBuffer(DXVA_SLICE_CONTROL_BUFFER, sizeof(DXVA_SliceInfo), &ctx_pic->slice));
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

void CDXVA1DecoderVC1::UpdatePictureParams(int nSurfaceIndex)
{
	DXVA_ConfigPictureDecode* cpd			= &m_DXVA1Config;
	DXVA_PictureParameters* DXVAPicParams	= &m_DXVA_Context.ctx_pic[m_nFieldNum].pp;

	DXVAPicParams->wDecodedPictureIndex		= 
	DXVAPicParams->wDeblockedPictureIndex	= nSurfaceIndex;

	// Manage reference picture list
	if (!DXVAPicParams->bPicBackwardPrediction && m_nFieldNum == 0) {
		if (m_wRefPictureIndex[0] != NO_REF_FRAME) {
			RemoveRefFrame(m_wRefPictureIndex[0]);
		}
		m_wRefPictureIndex[0] = m_wRefPictureIndex[1];
		m_wRefPictureIndex[1] = nSurfaceIndex;
	}
	DXVAPicParams->wForwardRefPictureIndex	= (DXVAPicParams->bPicIntra == 0)			   ? m_wRefPictureIndex[0] : NO_REF_FRAME;
	DXVAPicParams->wBackwardRefPictureIndex	= (DXVAPicParams->bPicBackwardPrediction == 1) ? m_wRefPictureIndex[1] : NO_REF_FRAME;
}

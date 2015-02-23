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
#include "DXVA1DecoderMPEG2.h"
#include "../MPCVideoDec.h"
#include "../FfmpegContext.h"
#include <ffmpeg/libavcodec/avcodec.h>

#if 0
	#define TRACE_MPEG2 TRACE
#else
	#define TRACE_MPEG2 __noop
#endif

CDXVA1DecoderMPEG2::CDXVA1DecoderMPEG2(CMPCVideoDecFilter* pFilter, IAMVideoAccelerator*  pAMVideoAccelerator, int nPicEntryNumber)
	: CDXVA1Decoder(pFilter, pAMVideoAccelerator, nPicEntryNumber)
{
	memset(&m_DXVA_Context, 0, sizeof(m_DXVA_Context));

	FFMPEG2SetDxvaParams(m_pFilter->GetAVCtx(), &m_DXVA_Context);

	Flush();
}

void CDXVA1DecoderMPEG2::Flush()
{
	m_nNextCodecIndex		= INT_MIN;

	m_wRefPictureIndex[0]	= NO_REF_FRAME;
	m_wRefPictureIndex[1]	= NO_REF_FRAME;

	__super::Flush();
}

void CDXVA1DecoderMPEG2::CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize)
{
	DXVA_MPEG2_Picture_Context *ctx_pic	= &m_DXVA_Context.ctx_pic[m_nFieldNum];
	BYTE* current						= pDXVABuffer;
	UINT MBCount						= FFGetMBCount(m_pFilter->GetAVCtx());

	for (unsigned i = 0; i < ctx_pic->slice_count; i++) {
		DXVA_SliceInfo *slice	= &ctx_pic->slice[i];
		UINT position			= slice->dwSliceDataLocation;
		UINT size				= slice->dwSliceBitsInBuffer / 8;

		slice->dwSliceDataLocation = current - pDXVABuffer;

		if (i < ctx_pic->slice_count - 1) {
			slice->wNumberMBsInSlice =
				slice[1].wNumberMBsInSlice - slice[0].wNumberMBsInSlice;
		} else {
			slice->wNumberMBsInSlice =
				MBCount - slice[0].wNumberMBsInSlice;
		}

		memcpy(current, &ctx_pic->bitstream[position], size);
		current += size;
	}

	nSize = current - pDXVABuffer;
}

HRESULT CDXVA1DecoderMPEG2::DecodeFrame(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	HRESULT	hr				= S_FALSE;
	int		got_picture		= 0;
	int		nSurfaceIndex	= -1;

	AVFrame* pFrame;
	memset(&m_DXVA_Context, 0, sizeof(m_DXVA_Context));
	CHECK_HR_FALSE (FFDecodeFrame(m_pFilter->GetAVCtx(), m_pFilter->GetFrame(),
								  pDataIn, nSize, rtStart, rtStop,
								  &got_picture, &pFrame));

	if (!pFrame || !m_DXVA_Context.ctx_pic[0].slice_count) {
		return S_FALSE;
	}

	// Wait I frame after a flush
	if (m_bFlushed && !m_DXVA_Context.ctx_pic[0].pp.bPicIntra) {
		TRACE_MPEG2 ("CDXVA1DecoderMPEG2::DecodeFrame() : Flush - wait I frame\n");
		return S_FALSE;
	}

	if (got_picture) {
		m_nNextCodecIndex = m_pFilter->GetFrame()->coded_picture_number;
	}

	CHECK_HR_FALSE (GetFreeSurfaceIndex(nSurfaceIndex));

	for (UINT i = 0; i < m_DXVA_Context.frame_count; i++) {
		DXVA_MPEG2_Picture_Context* ctx_pic = &m_DXVA_Context.ctx_pic[i];

		if (!ctx_pic->slice_count) {
			continue;
		}

		m_nFieldNum = i;
		UpdatePictureParams(nSurfaceIndex);

		// Begin frame
		CHECK_HR_FALSE (BeginFrame(nSurfaceIndex));
		// Send picture parameters
		CHECK_HR_FRAME_DXVA1 (AddExecuteBuffer(DXVA_PICTURE_DECODE_BUFFER, sizeof(DXVA_PictureParameters), &ctx_pic->pp));
		// Send quantization matrix
		CHECK_HR_FRAME_DXVA1 (AddExecuteBuffer(DXVA_INVERSE_QUANTIZATION_MATRIX_BUFFER, sizeof(DXVA_QmatrixData), &ctx_pic->qm));
		// Send bitstream
		CHECK_HR_FRAME_DXVA1 (AddExecuteBuffer(DXVA_BITSTREAM_DATA_BUFFER));
		// Send slice control
		CHECK_HR_FRAME_DXVA1 (AddExecuteBuffer(DXVA_SLICE_CONTROL_BUFFER, sizeof(DXVA_SliceInfo) * ctx_pic->slice_count, ctx_pic->slice));
		// Decode frame
		CHECK_HR_FRAME_DXVA1 (Execute());
		CHECK_HR_FALSE (EndFrame(nSurfaceIndex));
	}

	AddToStore(nSurfaceIndex, m_DXVA_Context.ctx_pic[0].pp.bPicBackwardPrediction != 1,
			   rtStart, rtStop,
			   false, pFrame->coded_picture_number);

	m_bFlushed = false;
	return DisplayNextFrame();
}

void CDXVA1DecoderMPEG2::UpdatePictureParams(int nSurfaceIndex)
{
	DXVA_ConfigPictureDecode* cpd			= &m_DXVA1Config;
	DXVA_PictureParameters* DXVAPicParams	= &m_DXVA_Context.ctx_pic[m_nFieldNum].pp;

	DXVAPicParams->wDecodedPictureIndex = nSurfaceIndex;

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

	// Shall be 0 if bConfigResidDiffHost is 0 or if BPP > 8
	if (cpd->bConfigResidDiffHost == 0 || DXVAPicParams->bBPPminus1 > 7) {
		DXVAPicParams->bPicSpatialResid8 = 0;
	} else {
		if (DXVAPicParams->bBPPminus1 == 7 && DXVAPicParams->bPicIntra && cpd->bConfigResidDiffHost) {
			// Shall be 1 if BPP is 8 and bPicIntra is 1 and bConfigResidDiffHost is 1
			DXVAPicParams->bPicSpatialResid8 = 1;
		} else {
			// Shall be 1 if bConfigSpatialResid8 is 1
			DXVAPicParams->bPicSpatialResid8 = cpd->bConfigSpatialResid8;
		}
	}

	// Shall be 0 if bConfigResidDiffHost is 0 or if bConfigSpatialResid8 is 0 or if BPP > 8
	if (cpd->bConfigResidDiffHost == 0 || cpd->bConfigSpatialResid8 == 0 || DXVAPicParams->bBPPminus1 > 7) {
		DXVAPicParams->bPicOverflowBlocks = 0;
	}

	// Shall be 1 if bConfigHostInverseScan is 1 or if bConfigResidDiffAccelerator is 0.
	if (cpd->bConfigHostInverseScan == 1 || cpd->bConfigResidDiffAccelerator == 0) {
		DXVAPicParams->bPicScanFixed = 1;

		if (cpd->bConfigHostInverseScan != 0) {
			DXVAPicParams->bPicScanMethod	= 3;    // 11 = Arbitrary scan with absolute coefficient address.
		} else if (FFGetAlternateScan(m_pFilter->GetAVCtx())) {
			DXVAPicParams->bPicScanMethod	= 1;    // 00 = Zig-zag scan (MPEG-2 Figure 7-2)
		} else {
			DXVAPicParams->bPicScanMethod	= 0;    // 01 = Alternate-vertical (MPEG-2 Figure 7-3),
		}
	}
}

int CDXVA1DecoderMPEG2::FindOldestFrame()
{
	int nPos = -1;

	for (int i = 0; i < m_nPicEntryNumber; i++) {
		if (!m_pPictureStore[i].bDisplayed && m_pPictureStore[i].bInUse &&
				(m_pPictureStore[i].nCodecSpecific == m_nNextCodecIndex)) {
			m_nNextCodecIndex	= INT_MIN;
			nPos				= i;
		}
	}

	if (nPos != -1) {
		m_pFilter->UpdateFrameTime(m_pPictureStore[nPos].rtStart, m_pPictureStore[nPos].rtStop);
	}

	return nPos;
}

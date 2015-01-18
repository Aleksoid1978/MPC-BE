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
#include "DXVADecoderMpeg2.h"
#include "MPCVideoDec.h"
#include "FfmpegContext.h"
#include <ffmpeg/libavcodec/avcodec.h>

CDXVADecoderMpeg2::CDXVADecoderMpeg2(CMPCVideoDecFilter* pFilter, IAMVideoAccelerator* pAMVideoAccelerator, const GUID* guidDecoder, DXVAMode nMode, int nPicEntryNumber)
	: CDXVADecoder(pFilter, pAMVideoAccelerator, guidDecoder, nMode, nPicEntryNumber)
{
	Init();
}

CDXVADecoderMpeg2::CDXVADecoderMpeg2(CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, const GUID* guidDecoder, DXVAMode nMode, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config)
	: CDXVADecoder(pFilter, pDirectXVideoDec, guidDecoder, nMode, nPicEntryNumber, pDXVA2Config)
{
	Init();
}

CDXVADecoderMpeg2::~CDXVADecoderMpeg2(void)
{
	DbgLog((LOG_TRACE, 3, L"CDXVADecoderMpeg2::Destroy()"));
}

void CDXVADecoderMpeg2::Init()
{
	DbgLog((LOG_TRACE, 3, L"CDXVADecoderMpeg2::Init()"));

	memset(&m_DXVA_Context, 0, sizeof(m_DXVA_Context));

	switch (GetMode()) {
		case MPEG2_VLD :
			AllocExecuteParams(4);
			break;
		default :
			ASSERT(FALSE);
	}

	FFMPEG2SetDxvaParams(m_pFilter->GetAVCtx(), &m_DXVA_Context);

	Flush();
}

HRESULT CDXVADecoderMpeg2::CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize, UINT nDXVASize/* = UINT_MAX*/)
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

		if ((current - pDXVABuffer) + size > nDXVASize) {
			nSize = 0;
			return E_FAIL;
		}

		memcpy(current, &ctx_pic->bitstream[position], size);
		current += size;
	}

	nSize = current - pDXVABuffer;

	return S_OK;
}

HRESULT CDXVADecoderMpeg2::DecodeFrame(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	HRESULT	hr			= S_FALSE;
	bool	bIsField	= false;
	int		got_picture	= 0;

	memset(&m_DXVA_Context, 0, sizeof(m_DXVA_Context));
	CHECK_HR_FALSE (FFDecodeFrame(m_pFilter->GetAVCtx(), m_pFilter->GetFrame(),
								  pDataIn, nSize, rtStart, rtStop,
								  &got_picture));

	if (m_nSurfaceIndex == -1 || !m_DXVA_Context.ctx_pic[0].slice_count) {
		return S_FALSE;
	}

	m_pFilter->HandleKeyFrame(got_picture);

	for (UINT i = 0; i < m_DXVA_Context.frame_count; i++) {
		DXVA_MPEG2_Picture_Context* ctx_pic = &m_DXVA_Context.ctx_pic[i];

		if (!ctx_pic->slice_count) {
			continue;
		}

		m_nFieldNum = i;
		UpdatePictureParams();

		CHECK_HR_FALSE (BeginFrame(m_nSurfaceIndex, m_pSampleToDeliver));
		// Send picture parameters
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_PictureParametersBufferType, sizeof(DXVA_PictureParameters), &ctx_pic->pp));
		// Add quantization matrix
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_InverseQuantizationMatrixBufferType, sizeof(DXVA_QmatrixData), &ctx_pic->qm));
		// Add bitstream
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_BitStreamDateBufferType));
		// Add slice control
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_SliceControlBufferType, sizeof(DXVA_SliceInfo) * ctx_pic->slice_count, ctx_pic->slice));
		// Decode frame
		CHECK_HR_FRAME (Execute());
		CHECK_HR_FALSE (EndFrame(m_nSurfaceIndex));
	}

	if (got_picture) {
		AddToStore(m_nSurfaceIndex, m_pSampleToDeliver);
		hr = DisplayNextFrame();
	}

	return hr;
}

void CDXVADecoderMpeg2::UpdatePictureParams()
{
	DXVA2_ConfigPictureDecode* cpd			= GetDXVA2Config(); // Ok for DXVA1 too (parameters have been copied)
	DXVA_PictureParameters* DXVAPicParams	= &m_DXVA_Context.ctx_pic[m_nFieldNum].pp;

	// Shall be 0 if bConfigResidDiffHost is 0 or if BPP > 8
	if (cpd->ConfigResidDiffHost == 0 || DXVAPicParams->bBPPminus1 > 7) {
		DXVAPicParams->bPicSpatialResid8 = 0;
	} else {
		if (DXVAPicParams->bBPPminus1 == 7 && DXVAPicParams->bPicIntra && cpd->ConfigResidDiffHost) {
			// Shall be 1 if BPP is 8 and bPicIntra is 1 and bConfigResidDiffHost is 1
			DXVAPicParams->bPicSpatialResid8 = 1;
		} else {
			// Shall be 1 if bConfigSpatialResid8 is 1
			DXVAPicParams->bPicSpatialResid8 = cpd->ConfigSpatialResid8;
		}
	}

	// Shall be 0 if bConfigResidDiffHost is 0 or if bConfigSpatialResid8 is 0 or if BPP > 8
	if (cpd->ConfigResidDiffHost == 0 || cpd->ConfigSpatialResid8 == 0 || DXVAPicParams->bBPPminus1 > 7) {
		DXVAPicParams->bPicOverflowBlocks = 0;
	}

	// Shall be 1 if bConfigHostInverseScan is 1 or if bConfigResidDiffAccelerator is 0.
	if (cpd->ConfigHostInverseScan == 1 || cpd->ConfigResidDiffAccelerator == 0) {
		DXVAPicParams->bPicScanFixed = 1;

		if (cpd->ConfigHostInverseScan != 0) {
			DXVAPicParams->bPicScanMethod	= 3;    // 11 = Arbitrary scan with absolute coefficient address.
		} else if (FFGetAlternateScan(m_pFilter->GetAVCtx())) {
			DXVAPicParams->bPicScanMethod	= 1;    // 00 = Zig-zag scan (MPEG-2 Figure 7-2)
		} else {
			DXVAPicParams->bPicScanMethod	= 0;    // 01 = Alternate-vertical (MPEG-2 Figure 7-3),
		}
	}
}

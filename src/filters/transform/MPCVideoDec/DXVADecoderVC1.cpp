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

#include "stdafx.h"
#include "DXVADecoderVC1.h"
#include "MPCVideoDec.h"
#include "FfmpegContext.h"
#include <ffmpeg/libavcodec/avcodec.h>

CDXVADecoderVC1::CDXVADecoderVC1(CMPCVideoDecFilter* pFilter, IAMVideoAccelerator* pAMVideoAccelerator, const GUID* guidDecoder, DXVAMode nMode, int nPicEntryNumber)
	: CDXVADecoder(pFilter, pAMVideoAccelerator, guidDecoder, nMode, nPicEntryNumber)
{
	Init();
}

CDXVADecoderVC1::CDXVADecoderVC1(CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, const GUID* guidDecoder, DXVAMode nMode, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config)
	: CDXVADecoder(pFilter, pDirectXVideoDec, guidDecoder, nMode, nPicEntryNumber, pDXVA2Config)
{
	Init();
}

CDXVADecoderVC1::~CDXVADecoderVC1(void)
{
	DbgLog((LOG_TRACE, 3, L"CDXVADecoderVC1::Destroy()"));
}

void CDXVADecoderVC1::Init()
{
	DbgLog((LOG_TRACE, 3, L"CDXVADecoderVC1::Init()"));

	memset(&m_PictureParams, 0, sizeof(m_PictureParams));
	memset(&m_SliceInfo, 0, sizeof(m_SliceInfo));

	switch (GetMode()) {
		case VC1_VLD :
			AllocExecuteParams(3);
			break;
		default :
			ASSERT(FALSE);
	}

	FFVC1SetDxvaParams(m_pFilter->GetAVCtx(), m_PictureParams, m_SliceInfo);

	Flush();
}

void CDXVADecoderVC1::Flush()
{
	bSecondField				= FALSE;
	StatusReportFeedbackNumber	= 0;

	__super::Flush();
}

static BYTE* FindNextStartCode(BYTE* pBuffer, UINT nSize, UINT& nPacketSize)
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

HRESULT CDXVADecoderVC1::CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize, UINT nDXVASize/* = UINT_MAX*/)
{
	if (bSecondField) {
		memcpy_sse(pDXVABuffer, (BYTE*)pBuffer, nSize);
	} else {
		if ((*((DWORD*)pBuffer) & 0x00FFFFFF) != 0x00010000) {
			if (m_pFilter->GetCodec() == AV_CODEC_ID_WMV3) {
				memcpy_sse(pDXVABuffer, (BYTE*)pBuffer, nSize);
			} else {
				pDXVABuffer[0] = pDXVABuffer[1] = 0;
				pDXVABuffer[2] = 1;
				pDXVABuffer[3] = 0x0D;
				memcpy_sse(pDXVABuffer + 4, (BYTE*)pBuffer, nSize);
				nSize += 4;
			}
		} else {
			BYTE*	pStart;
			UINT	nPacketSize;

			pStart = FindNextStartCode(pBuffer, nSize, nPacketSize);
			if (pStart) {
				// Startcode already present
				memcpy_sse(pDXVABuffer, (BYTE*)pStart, nPacketSize);
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

	return S_OK;
}

HRESULT CDXVADecoderVC1::DecodeFrame(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	HRESULT	hr				= S_FALSE;
	UINT	nFrameSize		= 0;
	UINT	nSize_Result	= 0;
	int		got_picture		= 0;

	memset(&m_PictureParams, 0, sizeof(m_PictureParams));
	m_PictureParams[1].bBidirectionalAveragingMode =
	m_PictureParams[0].bBidirectionalAveragingMode = (1                               << 7) |
													 (GetConfigIntraResidUnsigned()   << 6) |
													 (GetConfigResidDiffAccelerator() << 5);

	CHECK_HR_FALSE (FFDecodeFrame(m_pFilter->GetAVCtx(), m_pFilter->GetFrame(),
								  pDataIn, nSize, rtStart, rtStop,
							      &got_picture, &nFrameSize));

	if (m_nSurfaceIndex == -1) {
		return S_FALSE;
	}

	m_pFilter->HandleKeyFrame(got_picture);

	{
		bSecondField = FALSE;

		StatusReportFeedbackNumber++;
		if (StatusReportFeedbackNumber >= (1 << 16)) {
			StatusReportFeedbackNumber = 1;
		}
		m_PictureParams[0].bPicScanFixed	= StatusReportFeedbackNumber >> 8;
		m_PictureParams[0].bPicScanMethod	= StatusReportFeedbackNumber & 0xff;

		// Begin frame
		CHECK_HR_FALSE (BeginFrame(m_nSurfaceIndex, m_pSampleToDeliver));
		// Add picture parameters
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_PictureParametersBufferType, sizeof(DXVA_PictureParameters), &m_PictureParams[0]));
		// Add bitstream
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_BitStreamDateBufferType, nFrameSize ? nFrameSize : nSize, pDataIn, &nSize_Result));
		// Add slice control
		m_SliceInfo[0].dwSliceBitsInBuffer = nSize_Result * 8;
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_SliceControlBufferType, sizeof(DXVA_SliceInfo), &m_SliceInfo[0]));
		// Decode frame
		CHECK_HR_FRAME (Execute());
		CHECK_HR_FALSE (EndFrame(m_nSurfaceIndex));

		if (nFrameSize) {
			 // Decoding Second Field
			bSecondField = TRUE;

			StatusReportFeedbackNumber++;
			if (StatusReportFeedbackNumber >= (1 << 16)) {
				StatusReportFeedbackNumber = 1;
			}
			m_PictureParams[1].bPicScanFixed	= StatusReportFeedbackNumber >> 8;
			m_PictureParams[1].bPicScanMethod	= StatusReportFeedbackNumber & 0xff;

			// Begin frame
			CHECK_HR_FALSE (BeginFrame(m_nSurfaceIndex, m_pSampleToDeliver));
			// Add picture parameters
			CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_PictureParametersBufferType, sizeof(DXVA_PictureParameters), &m_PictureParams[1]));
			// Add bitstream
			CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_BitStreamDateBufferType, nSize - nFrameSize, pDataIn + nFrameSize, &nSize_Result));
			// Add slice control
			m_SliceInfo[1].dwSliceBitsInBuffer = nSize_Result * 8;
			CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_SliceControlBufferType, sizeof(DXVA_SliceInfo), &m_SliceInfo[1]));
			// Decode frame
			CHECK_HR_FRAME (Execute());
			CHECK_HR_FALSE (EndFrame(m_nSurfaceIndex));
		}
	}

	if (got_picture) {
		AddToStore(m_nSurfaceIndex, m_pSampleToDeliver);
		hr = DisplayNextFrame();
	}

	return hr;
}

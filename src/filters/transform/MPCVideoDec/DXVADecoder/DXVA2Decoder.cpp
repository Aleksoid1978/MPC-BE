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
#include <dxva2api.h>
#include <moreuuids.h>
#include "DXVA2DecoderH264.h"
#include "DXVA2DecoderHEVC.h"
#include "DXVA2DecoderVC1.h"
#include "DXVA2DecoderMPEG2.h"
#include "DXVA2Decoder.h"
#include "../MPCVideoDec.h"
#include "../DXVAAllocator.h"
#include "../FfmpegContext.h"
extern "C" {
	#include <ffmpeg/libavcodec/avcodec.h>
}

CDXVA2Decoder::CDXVA2Decoder(CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, const GUID* guidDecoder, DXVA2_ConfigPictureDecode* pDXVA2Config, int CompressedBuffersSize)
	: CDXVADecoder()
	, m_pDirectXVideoDec(pDirectXVideoDec)
	, m_guidDecoder(*guidDecoder)
	, m_pFilter(pFilter)
{
	memcpy(&m_DXVA2Config, pDXVA2Config, sizeof(DXVA2_ConfigPictureDecode));
	memset(&m_ExecuteParams, 0, sizeof(m_ExecuteParams));
	m_ExecuteParams.pCompressedBuffers = DNew DXVA2_DecodeBufferDesc[CompressedBuffersSize];

	for (int i = 0; i < CompressedBuffersSize; i++) {
		memset(&m_ExecuteParams.pCompressedBuffers[i], 0, sizeof(DXVA2_DecodeBufferDesc));
	}
};

CDXVA2Decoder::~CDXVA2Decoder()
{
	SAFE_DELETE_ARRAY(m_ExecuteParams.pCompressedBuffers);
	m_pDirectXVideoDec.Release();
}

CDXVA2Decoder* CDXVA2Decoder::CreateDXVA2Decoder(CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, const GUID* guidDecoder, DXVA2_ConfigPictureDecode* pDXVA2Config)
{
	CDXVA2Decoder* pDecoder = NULL;

	if ((*guidDecoder == DXVA2_ModeH264_E) || (*guidDecoder == DXVA2_ModeH264_F) || (*guidDecoder == DXVA_Intel_H264_ClearVideo)) {
		pDecoder = DNew CDXVA2DecoderH264(pFilter, pDirectXVideoDec, guidDecoder, pDXVA2Config);
	} else if (*guidDecoder == DXVA2_ModeVC1_D || *guidDecoder == DXVA2_ModeVC1_D2010) {
		pDecoder = DNew CDXVA2DecoderVC1(pFilter, pDirectXVideoDec, guidDecoder, pDXVA2Config);
	} else if (*guidDecoder == DXVA_ModeHEVC_VLD_Main || *guidDecoder == DXVA_ModeHEVC_VLD_Main10) {
		pDecoder = DNew CDXVA2DecoderHEVC(pFilter, pDirectXVideoDec, guidDecoder, pDXVA2Config);
	} else if (*guidDecoder == DXVA2_ModeMPEG2_VLD) {
		pDecoder = DNew CDXVA2DecoderMPEG2(pFilter, pDirectXVideoDec, guidDecoder, pDXVA2Config);
	} else {
		ASSERT(FALSE); // Unknown decoder !!
	}

	return pDecoder;
}

void CDXVA2Decoder::EndOfStream()
{
	while (DecodeFrame(NULL, 0, INVALID_TIME, INVALID_TIME) == S_OK);
}

HRESULT CDXVA2Decoder::AddExecuteBuffer(DWORD CompressedBufferType, UINT nSize, void* pBuffer)
{
	HRESULT	hr			= E_INVALIDARG;
	DWORD	dwNumMBs	= 0;
	UINT	nDXVASize	= 0;
	BYTE*	pDXVABuffer	= NULL;
	
	hr = m_pDirectXVideoDec->GetBuffer(CompressedBufferType, (void**)&pDXVABuffer, &nDXVASize);
	ASSERT(nSize <= nDXVASize);

	if (SUCCEEDED(hr) && (nSize <= nDXVASize)) {
		if (CompressedBufferType == DXVA2_BitStreamDateBufferType) {
			HRESULT hr2 = CopyBitstream(pDXVABuffer, nSize, nDXVASize);
			ASSERT(SUCCEEDED(hr2));
		} else {
			memcpy(pDXVABuffer, (BYTE*)pBuffer, nSize);
		}

		m_ExecuteParams.pCompressedBuffers[m_ExecuteParams.NumCompBuffers].CompressedBufferType = CompressedBufferType;
		m_ExecuteParams.pCompressedBuffers[m_ExecuteParams.NumCompBuffers].DataSize				= nSize;
		m_ExecuteParams.pCompressedBuffers[m_ExecuteParams.NumCompBuffers].NumMBsInBuffer		= dwNumMBs;
		m_ExecuteParams.NumCompBuffers++;

	}

	return hr;
}

HRESULT CDXVA2Decoder::Execute()
{
	for (DWORD i = 0; i < m_ExecuteParams.NumCompBuffers; i++) {
		HRESULT hr2 = m_pDirectXVideoDec->ReleaseBuffer(m_ExecuteParams.pCompressedBuffers[i].CompressedBufferType);
		ASSERT(SUCCEEDED(hr2));
	}

	HRESULT hr = m_pDirectXVideoDec->Execute(&m_ExecuteParams);
	m_ExecuteParams.NumCompBuffers	= 0;

	return hr;
}

HRESULT CDXVA2Decoder::BeginFrame(IMediaSample* pSampleToDeliver)
{
	HRESULT	hr		= E_INVALIDARG;
	int		nTry	= 0;

	for (int i = 0; i < 20; i++) {
		if (CComQIPtr<IMFGetService> pSampleService = pSampleToDeliver) {
			CComPtr<IDirect3DSurface9> pDecoderRenderTarget;
			hr = pSampleService->GetService(MR_BUFFER_SERVICE, IID_PPV_ARGS(&pDecoderRenderTarget));
			if (SUCCEEDED(hr)) {
				DO_DXVA_PENDING_LOOP (m_pDirectXVideoDec->BeginFrame(pDecoderRenderTarget, NULL));
			}
		}

		// For slow accelerator wait a little...
		if (SUCCEEDED(hr)) {
			break;
		}
		Sleep(1);
	}

	return hr;
}

HRESULT CDXVA2Decoder::EndFrame()
{
	return m_pDirectXVideoDec->EndFrame(NULL);
}

HRESULT CDXVA2Decoder::DisplayNextFrame()
{
	HRESULT hr = E_FAIL;

	AVFrame* pFrame = m_pFilter->GetFrame();
	IMediaSample* pSample;
	REFERENCE_TIME rtStop, rtStart;
	CHECK_HR_FALSE (GetSapleWrapperData(pFrame, &pSample, &rtStart, &rtStop));

	m_pFilter->ReorderBFrames(rtStart, rtStop);
	m_pFilter->UpdateFrameTime(rtStart, rtStop);

	if (rtStart < 0) {
		return S_OK;
	}

	m_pFilter->ReconnectOutput(m_pFilter->PictWidth(), m_pFilter->PictHeight(), true, false, m_pFilter->GetDuration());

	m_pFilter->SetTypeSpecificFlags(pSample);
	pSample->SetTime(&rtStart, &rtStop);
	pSample->SetMediaTime(NULL, NULL);

	CMediaType& mt = m_pFilter->GetOutputPin()->CurrentMediaType();

	bool bSizeChanged = false;
	LONG biWidth, biHeight = 0;

	if (m_pFilter->GetSendMediaType()) {
		AM_MEDIA_TYPE *sendmt = CreateMediaType(&mt);
		BITMAPINFOHEADER *pBMI = NULL;
		if (sendmt->formattype == FORMAT_VideoInfo) {
			VIDEOINFOHEADER *vih	= (VIDEOINFOHEADER *)sendmt->pbFormat;
			pBMI					= &vih->bmiHeader;
			SetRect(&vih->rcSource, 0, 0, 0, 0);
		} else if (sendmt->formattype == FORMAT_VideoInfo2) {
			VIDEOINFOHEADER2 *vih2	= (VIDEOINFOHEADER2 *)sendmt->pbFormat;
			pBMI					= &vih2->bmiHeader;
			SetRect(&vih2->rcSource, 0, 0, 0, 0);
		}

		biWidth		= pBMI->biWidth;
		biHeight	= abs(pBMI->biHeight);
		pSample->SetMediaType(sendmt);
		DeleteMediaType(sendmt);
		m_pFilter->SetSendMediaType(false);
		bSizeChanged = true;
	}

	hr = m_pFilter->GetOutputPin()->Deliver(pSample);

	if (bSizeChanged && biHeight) {
		m_pFilter->NotifyEvent(EC_VIDEO_SIZE_CHANGED, MAKELPARAM(biWidth, biHeight), 0);
	}

	return hr;
}

HRESULT CDXVA2Decoder::GetFreeSurfaceIndex(int& nSurfaceIndex, IMediaSample** ppSampleToDeliver)
{
	HRESULT hr = E_UNEXPECTED;
	CComPtr<IMediaSample> pSample;
	if (SUCCEEDED(hr = m_pFilter->GetOutputPin()->GetDeliveryBuffer(&pSample, NULL, NULL, 0))) {
		CComQIPtr<IMPCDXVA2Sample> pMPCDXVA2Sample	= pSample;
		nSurfaceIndex								= pMPCDXVA2Sample ? pMPCDXVA2Sample->GetDXSurfaceId() : 0;
		*ppSampleToDeliver							= pSample.Detach();
	}

	return hr;
}

HRESULT CDXVA2Decoder::GetSapleWrapperData(AVFrame* pFrame, IMediaSample** pSample, REFERENCE_TIME* rtStart, REFERENCE_TIME* rtStop)
{
	CheckPointer(pFrame, E_FAIL);

	SampleWrapper* pSampleWrapper = (SampleWrapper*)pFrame->data[3];
	if (pSampleWrapper) {
		*pSample = pSampleWrapper->pSample;
		
		if (rtStart && rtStop) {
			m_pFilter->GetFrameTimeStamp(pFrame, *rtStart, *rtStop);
		}

		return S_OK;
	}

	return E_FAIL;
}

HRESULT CDXVA2Decoder::get_buffer_dxva(AVFrame *pic)
{
	HRESULT hr = S_OK;
	
	CComPtr<IMediaSample> pSample;
	int nSurfaceIndex = -1;
	CHECK_HR_FALSE (GetFreeSurfaceIndex(nSurfaceIndex, &pSample));

	SampleWrapper* pSampleWrapper	= DNew SampleWrapper();
	pSampleWrapper->opaque			= (void*)this;
	pSampleWrapper->pSample			= pSample;

	pic->data[3]	= (uint8_t *)pSampleWrapper;
	pic->data[4]	= (uint8_t *)nSurfaceIndex;
	pic->buf[3]		= av_buffer_create(NULL, 0, release_buffer_dxva, pSampleWrapper, 0);

	return hr;
}

void CDXVA2Decoder::release_buffer_dxva(void *opaque, uint8_t *data)
{
	SampleWrapper* pSampleWrapper = (SampleWrapper*)opaque;

	CDXVA2Decoder* pDec = (CDXVA2Decoder*)pSampleWrapper->opaque;
	pSampleWrapper->pSample.Release();

	delete pSampleWrapper;
}

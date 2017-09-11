/*
 * (C) 2006-2017 see Authors.txt
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
#include <moreuuids.h>
#include "DXVA2Decoder.h"
#include "../MPCVideoDec.h"
#include "DXVAAllocator.h"
#include "../FfmpegContext.h"
#include "../../../../DSUtil/DSUtil.h"

extern "C" {
	#include <ffmpeg/libavcodec/avcodec.h>
	#include <ffmpeg/libavcodec/dxva2.h>
}

CDXVA2Decoder::CDXVA2Decoder(CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, const GUID* guidDecoder, DXVA2_ConfigPictureDecode* pDXVA2Config, LPDIRECT3DSURFACE9* ppD3DSurface, UINT numSurfaces)
	: m_pDirectXVideoDec(pDirectXVideoDec)
	, m_guidDecoder(*guidDecoder)
	, m_pFilter(pFilter)
	, m_nNumSurfaces(numSurfaces)
{
	ZeroMemory(&m_pD3DSurface, sizeof(m_pD3DSurface));
	for (UINT i = 0; i < m_nNumSurfaces; i++) {
		m_pD3DSurface[i] = ppD3DSurface[i];
	}

	memcpy(&m_DXVA2Config, pDXVA2Config, sizeof(DXVA2_ConfigPictureDecode));

	FillHWContext();
};

void CDXVA2Decoder::FillHWContext()
{
	dxva_context *ctx = (dxva_context *)m_pFilter->m_pAVCtx->hwaccel_context;
	ctx->cfg           = &m_DXVA2Config;
	ctx->decoder       = m_pDirectXVideoDec;
	ctx->surface       = m_pD3DSurface;
	ctx->surface_count = m_nNumSurfaces;

	if (m_pFilter->m_nPCIVendor == PCIV_Intel && m_guidDecoder == DXVA_Intel_H264_ClearVideo) {
		ctx->workaround = FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO;
	} else if (IsATIUVD(m_pFilter->m_nPCIVendor, m_pFilter->m_nPCIDevice)) {
		ctx->workaround = FF_DXVA2_WORKAROUND_SCALING_LIST_ZIGZAG;
	}
}

HRESULT CDXVA2Decoder::DeliverFrame()
{
	CheckPointer(m_pFilter->m_pFrame, S_FALSE);
	AVFrame* pFrame = m_pFilter->m_pFrame;

	IMediaSample* pSample = (IMediaSample *)pFrame->data[4];
	CheckPointer(pSample, S_FALSE);

	REFERENCE_TIME rtStop, rtStart;
	m_pFilter->GetFrameTimeStamp(pFrame, rtStart, rtStop);
	m_pFilter->UpdateFrameTime(rtStart, rtStop);

	if (rtStart < 0) {
		return S_OK;
	}

	DXVA2_ExtendedFormat dxvaExtFormat = m_pFilter->GetDXVA2ExtendedFormat(m_pFilter->m_pAVCtx, pFrame);

	m_pFilter->ReconnectOutput(m_pFilter->PictWidth(), m_pFilter->PictHeight(), false, m_pFilter->GetFrameDuration(), &dxvaExtFormat);

	m_pFilter->SetTypeSpecificFlags(pSample);
	pSample->SetTime(&rtStart, &rtStop);
	pSample->SetMediaTime(nullptr, nullptr);

	bool bSizeChanged = false;
	LONG biWidth, biHeight = 0;

	CMediaType& mt = m_pFilter->GetOutputPin()->CurrentMediaType();
	if (m_pFilter->m_bSendMediaType) {
		AM_MEDIA_TYPE *sendmt = CreateMediaType(&mt);
		BITMAPINFOHEADER *pBMI = nullptr;
		if (sendmt->formattype == FORMAT_VideoInfo) {
			VIDEOINFOHEADER *vih   = (VIDEOINFOHEADER *)sendmt->pbFormat;
			pBMI                   = &vih->bmiHeader;
		} else if (sendmt->formattype == FORMAT_VideoInfo2) {
			VIDEOINFOHEADER2 *vih2 = (VIDEOINFOHEADER2 *)sendmt->pbFormat;
			pBMI                   = &vih2->bmiHeader;
		} else {
			return E_FAIL;
		}

		biWidth  = pBMI->biWidth;
		biHeight = abs(pBMI->biHeight);
		pSample->SetMediaType(sendmt);
		DeleteMediaType(sendmt);
		m_pFilter->m_bSendMediaType = false;
		bSizeChanged = true;
	}

	m_pFilter->AddFrameSideData(pSample, pFrame);

	HRESULT hr = m_pFilter->GetOutputPin()->Deliver(pSample);

	if (bSizeChanged && biHeight) {
		m_pFilter->NotifyEvent(EC_VIDEO_SIZE_CHANGED, MAKELPARAM(biWidth, abs(biHeight)), 0);
	}

	return hr;
}

int CDXVA2Decoder::get_buffer_dxva(AVFrame *pic)
{
	IMediaSample* pSample = nullptr;
	HRESULT hr = m_pFilter->m_pDXVA2Allocator->GetBuffer(&pSample, nullptr, nullptr, 0);
	if (FAILED(hr)) {
		DLog(L"DXVA2Allocator->GetBuffer() failed: 0x%08x", hr);
		return -1;
	}

	CComQIPtr<IMPCDXVA2Sample> pMPCDXVA2Sample = pSample;
	if (!pMPCDXVA2Sample) {
		DLog(L"Something went wrong, it's not CDXVA2Sample");
		SAFE_RELEASE(pSample);
		return -1;
	}

	int nSurfaceIndex = pMPCDXVA2Sample->GetDXSurfaceId();

	LPDIRECT3DSURFACE9 pSurface = m_pD3DSurface[nSurfaceIndex];

	ZeroMemory(pic->data, sizeof(pic->data));
	ZeroMemory(pic->linesize, sizeof(pic->linesize));
	ZeroMemory(pic->buf, sizeof(pic->buf));

	pic->data[0] = pic->data[3] = (uint8_t *)pSurface;
	pic->data[4] = (uint8_t *)pSample;

	pic->buf[0]  = av_buffer_create(nullptr, 0, release_buffer_dxva, pSample, 0);

	return 0;
}

void CDXVA2Decoder::release_buffer_dxva(void *opaque, uint8_t *data)
{
	IMediaSample* pSample = (IMediaSample*)opaque;
	SAFE_RELEASE(pSample);
}

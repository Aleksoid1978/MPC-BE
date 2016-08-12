/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2016 see Authors.txt
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
#include <mmintrin.h>
#include <memory.h>
#include <mfapi.h>
#include "BaseVideoFilter.h"
#include "../../../DSUtil/DSUtil.h"
#include "../MPCVideoDec/memcpy_sse.h"

#include <InitGuid.h>
#include <moreuuids.h>

static inline bool BitBltFromP016ToP016(size_t w, size_t h, BYTE* dstY, BYTE* dstUV, int dstPitch, BYTE* srcY, BYTE* srcUV, int srcPitch)
{
	// Copy Y plane
	for (size_t row = 0; row < h; row++) {
		BYTE* src = srcY + row * srcPitch;
		BYTE* dst = dstY + row * dstPitch;

		memcpy_sse(dst, src, dstPitch);
	}

	// Copy UV plane. UV plane is half height.
	for (size_t row = 0; row < h / 2; row++) {
		BYTE* src = srcUV + row * srcPitch;
		BYTE* dst = dstUV + row * dstPitch;

		memcpy_sse(dst, src, dstPitch);
	}

	return true;
}
//
// CBaseVideoFilter
//

CBaseVideoFilter::CBaseVideoFilter(TCHAR* pName, LPUNKNOWN lpunk, HRESULT* phr, REFCLSID clsid, long cBuffers)
	: CTransformFilter(pName, lpunk, clsid)
	, m_cBuffers(cBuffers)
	, m_bSendMediaType(false)
	, m_nDecoderMode(MODE_SOFTWARE)
	, m_RenderClsid(CLSID_NULL)
{
	if (phr) {
		*phr = S_OK;
	}

	m_pInput = DNew CBaseVideoInputPin(NAME("CBaseVideoInputPin"), this, phr, L"Video");
	if (!m_pInput) {
		*phr = E_OUTOFMEMORY;
	}
	if (FAILED(*phr)) {
		return;
	}

	m_pOutput = DNew CBaseVideoOutputPin(NAME("CBaseVideoOutputPin"), this, phr, L"Output");
	if (!m_pOutput) {
		*phr = E_OUTOFMEMORY;
	}
	if (FAILED(*phr))  {
		delete m_pInput, m_pInput = NULL;
		return;
	}

	m_wout = m_win = m_w = 0;
	m_hout = m_hin = m_h = 0;
	m_arxout = m_arxin = m_arx = 0;
	m_aryout = m_aryin = m_ary = 0;
}

CBaseVideoFilter::~CBaseVideoFilter()
{
}

void CBaseVideoFilter::SetAspect(CSize aspect)
{
	if (m_arx != aspect.cx || m_ary != aspect.cy) {
		m_arx = aspect.cx;
		m_ary = aspect.cy;
	}
}

int CBaseVideoFilter::GetPinCount()
{
	return 2;
}

CBasePin* CBaseVideoFilter::GetPin(int n)
{
	switch (n) {
		case 0:
			return m_pInput;
		case 1:
			return m_pOutput;
	}
	return NULL;
}

HRESULT CBaseVideoFilter::Receive(IMediaSample* pIn)
{
#ifndef _WIN64
	// TODOX64 : fixme!
	_mm_empty(); // just for safety
#endif

	CAutoLock cAutoLock(&m_csReceive);

	HRESULT hr;

	AM_SAMPLE2_PROPERTIES* const pProps = m_pInput->SampleProps();
	if (pProps->dwStreamId != AM_STREAM_MEDIA) {
		return m_pOutput->Deliver(pIn);
	}

	AM_MEDIA_TYPE* pmt;
	if (SUCCEEDED(pIn->GetMediaType(&pmt)) && pmt) {
		CMediaType mt(*pmt);
		DeleteMediaType(pmt);
		if (mt != m_pInput->CurrentMediaType()) {
			m_pInput->SetMediaType(&mt);
		}
	}

	if (FAILED(hr = Transform(pIn))) {
		return hr;
	}

	return S_OK;
}

HRESULT CBaseVideoFilter::GetDeliveryBuffer(int w, int h, IMediaSample** ppOut, REFERENCE_TIME AvgTimePerFrame, DXVA2_ExtendedFormat* dxvaExtFlags)
{
	CheckPointer(ppOut, E_POINTER);

	HRESULT hr;

	if (FAILED(hr = ReconnectOutput(w, h, false, AvgTimePerFrame, dxvaExtFlags))) {
		return hr;
	}

	if (FAILED(hr = m_pOutput->GetDeliveryBuffer(ppOut, NULL, NULL, 0))) {
		return hr;
	}

	AM_MEDIA_TYPE* pmt;
	if (SUCCEEDED((*ppOut)->GetMediaType(&pmt)) && pmt) {
		CMediaType mt = *pmt;
		m_pOutput->SetMediaType(&mt);
		DeleteMediaType(pmt);
	}

	(*ppOut)->SetDiscontinuity(FALSE);
	(*ppOut)->SetSyncPoint(TRUE);

	// FIXME: hell knows why but without this the overlay mixer starts very skippy
	// (don't enable this for other renderers, the old for example will go crazy if you do)
	if (GetCLSID(m_pOutput->GetConnected()) == CLSID_OverlayMixer) {
		(*ppOut)->SetDiscontinuity(TRUE);
	}

	return S_OK;
}

HRESULT CBaseVideoFilter::ReconnectOutput(int width, int height, bool bForce/* = false*/, REFERENCE_TIME AvgTimePerFrame/* = 0*/, DXVA2_ExtendedFormat* dxvaExtFormat/* = NULL*/, int RealWidth/* = -1*/, int RealHeight/* = -1*/)
{
	CMediaType& mt = m_pOutput->CurrentMediaType();

	if (m_RenderClsid == CLSID_NULL) {
		CComPtr<IPin> pPin = m_pOutput;
		CComPtr<IPin> pPinRenderer;
		for (CComPtr<IBaseFilter> pBF = this; pBF = GetDownStreamFilter(pBF, pPin); pPin = GetFirstPin(pBF, PINDIR_OUTPUT)) {
			m_RenderClsid = GetCLSID(pBF);
		}
	}

	bool extformatsupport = (m_RenderClsid == CLSID_EnhancedVideoRenderer || m_RenderClsid == CLSID_madVR);

	bool bNeedReconnect = bForce;
	{
		int wout = 0, hout = 0, arxout = 0, aryout = 0;
		ExtractDim(&mt, wout, hout, arxout, aryout);
		if (arxout != m_arx || aryout != m_ary) {
			bNeedReconnect = true;
		}
	}

	int w_org = m_w;
	int h_org = m_h;

	if (width != m_w || height != m_h) {
		bNeedReconnect = true;
		m_w = width;
		m_h = height;
	}

	REFERENCE_TIME nAvgTimePerFrame = 0;
	{
		ExtractAvgTimePerFrame(&mt, nAvgTimePerFrame);
		if (!AvgTimePerFrame) {
			AvgTimePerFrame = nAvgTimePerFrame;
		}

		if (abs(nAvgTimePerFrame - AvgTimePerFrame) > 10) {
			bNeedReconnect = true;
		}
	}

	if (m_w != m_wout || m_h != m_hout || m_arx != m_arxout || m_ary != m_aryout) {
		bNeedReconnect = true;
	}

	if (extformatsupport && dxvaExtFormat && mt.formattype == FORMAT_VideoInfo2) {
		// from LAVVideo

		// HACK: 1280 is the value when only chroma location is set to MPEG2, do not bother to send this information, as its the same for basically every clip
		if ((dxvaExtFormat->value & ~0xff) != 0 && (dxvaExtFormat->value & ~0xff) != 1280) {
			dxvaExtFormat->SampleFormat = AMCONTROL_USED | AMCONTROL_COLORINFO_PRESENT;
		} else {
			dxvaExtFormat->value = 0;
		}

		if (dxvaExtFormat->value != 0 && m_RenderClsid != CLSID_madVR) {
			// Remove custom matrix settings, which are not understood upstream
			if (dxvaExtFormat->VideoTransferMatrix > DXVA2_VideoTransferMatrix_SMPTE240M) {
				dxvaExtFormat->VideoTransferMatrix = DXVA2_VideoTransferMatrix_Unknown;
			}
			if (dxvaExtFormat->VideoPrimaries > DXVA2_VideoPrimaries_SMPTE_C) {
				dxvaExtFormat->VideoPrimaries = DXVA2_VideoPrimaries_Unknown;
			}
			if (dxvaExtFormat->VideoTransferFunction > MFVideoTransFunc_Log_316) {
				dxvaExtFormat->VideoTransferFunction = DXVA2_VideoTransFunc_Unknown;
			}
		}

		VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)mt.Format();
		if (vih2->dwControlFlags != dxvaExtFormat->value) {
			bNeedReconnect = true;
		}
	}

	if (bNeedReconnect) {
		const CLSID clsid = GetCLSID(m_pOutput->GetConnected());
		if (clsid == CLSID_VideoRenderer) {
			NotifyEvent(EC_ERRORABORT, 0, 0);
			return E_FAIL;
		}
		const bool m_bOverlayMixer = !!(clsid == CLSID_OverlayMixer);

		CRect vih_rect(0, 0, RealWidth > 0 ? RealWidth : m_w, RealHeight > 0 ? RealHeight : m_h);

		DLog(L"CBaseVideoFilter::ReconnectOutput() : Performing reconnect");
		if (m_w != vih_rect.Width() || m_h != vih_rect.Height()) {
			DLog(L"    => SIZE  : %d:%d => %d:%d(%d:%d)", m_wout, m_hout, m_w, m_h, vih_rect.Width(), vih_rect.Height());
		} else {
			DLog(L"    => SIZE  : %d:%d => %d:%d", m_wout, m_hout, vih_rect.Width(), vih_rect.Height());
		}
		{
			DLog(L"    => AR    : %d:%d => %d:%d", m_arxout, m_aryout, m_arx, m_ary);
			DLog(L"    => FPS   : %I64d => %I64d", nAvgTimePerFrame, AvgTimePerFrame);
		}

		const bool bVideoSizeChanged = (m_w != m_wout || m_h != m_hout);

		BITMAPINFOHEADER* pBMI = NULL;
		if (mt.formattype == FORMAT_VideoInfo) {
			VIDEOINFOHEADER* vih		= (VIDEOINFOHEADER*)mt.Format();
			if (bVideoSizeChanged) {
				vih->rcSource			= vih->rcTarget = vih_rect;
			}
			vih->AvgTimePerFrame		= AvgTimePerFrame;

			pBMI						= &vih->bmiHeader;
			pBMI->biXPelsPerMeter		= m_w * m_ary;
			pBMI->biYPelsPerMeter		= m_h * m_arx;
		} else if (mt.formattype == FORMAT_VideoInfo2) {
			VIDEOINFOHEADER2* vih2	= (VIDEOINFOHEADER2*)mt.Format();
			if (bVideoSizeChanged) {
				vih2->rcSource			= vih2->rcTarget = vih_rect;
			}
			vih2->AvgTimePerFrame		= AvgTimePerFrame;
			vih2->dwPictAspectRatioX	= m_arx;
			vih2->dwPictAspectRatioY	= m_ary;

			DLog(L"    => FLAGS : 0x%0.8x -> 0x%0.8x", vih2->dwControlFlags, (extformatsupport && dxvaExtFormat) ? dxvaExtFormat->value : vih2->dwControlFlags);
			if (extformatsupport && dxvaExtFormat) {
				vih2->dwControlFlags	= dxvaExtFormat->value;
			}

			pBMI						= &vih2->bmiHeader;
		} else {
			return E_FAIL;	//should never be here? prevent null pointer refs for bmi
		}

		pBMI->biWidth     = m_w;
		pBMI->biHeight    = m_h;
		pBMI->biSizeImage = DIBSIZE(*pBMI);

		HRESULT hrQA = m_pOutput->GetConnected()->QueryAccept(&mt);
		ASSERT(SUCCEEDED(hrQA)); // should better not fail, after all "mt" is the current media type, just with a different resolution
		HRESULT hr = S_OK;

		if (m_nDecoderMode == MODE_DXVA2) {
			m_pOutput->SetMediaType(&mt);
			m_bSendMediaType = true;
		} else {
			bool tryReceiveConnection = true;
			for (;;) {
				hr = m_pOutput->GetConnected()->ReceiveConnection(m_pOutput, &mt);
				if (SUCCEEDED(hr)) {
					CComPtr<IMediaSample> pOut;
					if (SUCCEEDED(hr = m_pOutput->GetDeliveryBuffer(&pOut, NULL, NULL, 0))) {
						AM_MEDIA_TYPE* pmt;
						if (SUCCEEDED(pOut->GetMediaType(&pmt)) && pmt) {
							CMediaType mt = *pmt;
							m_pOutput->SetMediaType(&mt);
							DeleteMediaType(pmt);
						} else {
							if (m_bOverlayMixer) {
								// stupid overlay mixer won't let us know the new pitch...
								long size = pOut->GetSize();
								pBMI->biWidth = size ? (size / abs(pBMI->biHeight) * 8 / pBMI->biBitCount) : pBMI->biWidth;
							}
							m_pOutput->SetMediaType(&mt);
						}
					} else {
						m_w = w_org;
						m_h = h_org;
						return E_FAIL;
					}
				} else if (hr == VFW_E_BUFFERS_OUTSTANDING && tryReceiveConnection) {
					DLog(L"    VFW_E_BUFFERS_OUTSTANDING, flushing data ...");
					m_pOutput->DeliverBeginFlush();
					m_pOutput->DeliverEndFlush();
					tryReceiveConnection = false;

					continue;
				} else {
					DLog(L"    ReceiveConnection() failed (hr: %x); QueryAccept: %x", hr, hrQA);
				}

				break;
			}
		}

		m_wout   = m_w;
		m_hout   = m_h;
		m_arxout = m_arx;
		m_aryout = m_ary;

		// some renderers don't send this
		if (m_nDecoderMode != MODE_DXVA2) {
			NotifyEvent(EC_VIDEO_SIZE_CHANGED, MAKELPARAM(m_w, m_h), 0);
		}

		return S_OK;
	}

	return S_FALSE;
}

HRESULT CBaseVideoFilter::CopyBuffer(BYTE* pOut, BYTE* pIn, int w, int h, int pitchIn, const GUID& subtype, bool fInterlaced)
{
	int abs_h = abs(h);
	BYTE* pInYUV[3] = {pIn, pIn + pitchIn * abs_h, pIn + pitchIn * abs_h + (pitchIn >> 1) * (abs_h >> 1)};
	return CopyBuffer(pOut, pInYUV, w, h, pitchIn, subtype, fInterlaced);
}

HRESULT CBaseVideoFilter::CopyBuffer(BYTE* pOut, BYTE** ppIn, int w, int h, int pitchIn, const GUID& subtype, bool fInterlaced)
{
	BITMAPINFOHEADER bihOut;
	ExtractBIH(&m_pOutput->CurrentMediaType(), &bihOut);

	int pitchOut = 0;

	if (bihOut.biCompression == BI_RGB || bihOut.biCompression == BI_BITFIELDS) {
		pitchOut = bihOut.biWidth * bihOut.biBitCount >> 3;

		if (bihOut.biHeight > 0) {
			pOut += pitchOut * (h - 1);
			pitchOut = -pitchOut;
			if (h < 0) {
				h = -h;
			}
		}
	}

	if (h < 0) {
		h = -h;
		ppIn[0] += pitchIn * (h - 1);
		if (subtype == MEDIASUBTYPE_I420 || subtype == MEDIASUBTYPE_IYUV || subtype == MEDIASUBTYPE_YV12) {
			ppIn[1] += (pitchIn >> 1) * ((h >> 1) - 1);
			ppIn[2] += (pitchIn >> 1) * ((h >> 1) - 1);
		} else if(subtype == MEDIASUBTYPE_P010 || subtype == MEDIASUBTYPE_P016 || subtype == MEDIASUBTYPE_NV12) {
			ppIn[1] += pitchIn * ((h >> 1) - 1);
		}
		pitchIn = -pitchIn;
	}

	if (subtype == MEDIASUBTYPE_I420 || subtype == MEDIASUBTYPE_IYUV || subtype == MEDIASUBTYPE_YV12) {
		BYTE* pIn = ppIn[0];
		BYTE* pInU = ppIn[1];
		BYTE* pInV = ppIn[2];

		if (subtype == MEDIASUBTYPE_YV12) {
			BYTE* tmp = pInU;
			pInU = pInV;
			pInV = tmp;
		}

		BYTE* pOutU = pOut + bihOut.biWidth * h;
		BYTE* pOutV = pOut + bihOut.biWidth * h * 5 / 4;

		if (bihOut.biCompression == FCC('YV12')) {
			BYTE* tmp = pOutU;
			pOutU = pOutV;
			pOutV = tmp;
		}

		ASSERT(w <= abs(pitchIn));

		if (bihOut.biCompression == FCC('YUY2')) {
			if (!fInterlaced) {
				BitBltFromI420ToYUY2(w, h, pOut, bihOut.biWidth * 2, pIn, pInU, pInV, pitchIn);
			} else {
				BitBltFromI420ToYUY2Interlaced(w, h, pOut, bihOut.biWidth * 2, pIn, pInU, pInV, pitchIn);
			}
		} else if (bihOut.biCompression == FCC('I420') || bihOut.biCompression == FCC('IYUV') || bihOut.biCompression == FCC('YV12')) {
			BitBltFromI420ToI420(w, h, pOut, pOutU, pOutV, bihOut.biWidth, pIn, pInU, pInV, pitchIn);
		} else if(bihOut.biCompression == FCC('NV12')) {
			BitBltFromI420ToNV12(w, h, pOut, pOutU, pOutV, bihOut.biWidth, pIn, pInU, pInV, pitchIn);
		} else if (bihOut.biCompression == BI_RGB || bihOut.biCompression == BI_BITFIELDS) {
			if (!BitBltFromI420ToRGB(w, h, pOut, pitchOut, bihOut.biBitCount, pIn, pInU, pInV, pitchIn)) {
				for (int y = 0; y < h; y++, pOut += pitchOut) {
					memsetd(pOut, 0, pitchOut);
				}
			}
		}
	}
	else if ((subtype == MEDIASUBTYPE_P010 || subtype == MEDIASUBTYPE_P016)
			 && (bihOut.biCompression == FCC('P010') || bihOut.biCompression == FCC('P016'))) {
		// We currently don't support outputting P010/P016 input to something other than P010/P016
		// P010 and P016 share the same memory layout
		BYTE* pInY = ppIn[0];
		BYTE* pInUV = ppIn[1];
		BYTE* pOutUV = pOut + bihOut.biWidth * h * 2; // 2 bytes per pixel
		BitBltFromP016ToP016(w, h, pOut, pOutUV, bihOut.biWidth * 2, pInY, pInUV, pitchIn);
	} else if (subtype == MEDIASUBTYPE_NV12 && bihOut.biCompression == FCC('NV12')) {
		// We currently don't support outputting NV12 input to something other than NV12
		BYTE* pInY = ppIn[0];
		BYTE* pInUV = ppIn[1];
		BYTE* pOutUV = pOut + bihOut.biWidth * h; // 1 bytes per pixel
		BitBltFromP016ToP016(w, h, pOut, pOutUV, bihOut.biWidth, pInY, pInUV, pitchIn);
	} else if (subtype == MEDIASUBTYPE_YUY2) {
		if (bihOut.biCompression == FCC('YUY2')) {
			BitBltFromYUY2ToYUY2(w, h, pOut, bihOut.biWidth * 2, ppIn[0], pitchIn);
		} else if (bihOut.biCompression == BI_RGB || bihOut.biCompression == BI_BITFIELDS) {
			if (!BitBltFromYUY2ToRGB(w, h, pOut, pitchOut, bihOut.biBitCount, ppIn[0], pitchIn)) {
				for (int y = 0; y < h; y++, pOut += pitchOut) {
					memsetd(pOut, 0, pitchOut);
				}
			}
		}
	} else if (subtype == MEDIASUBTYPE_ARGB32 || subtype == MEDIASUBTYPE_RGB32 || subtype == MEDIASUBTYPE_RGB24 || subtype == MEDIASUBTYPE_RGB565) {
		int sbpp =
			subtype == MEDIASUBTYPE_ARGB32 || subtype == MEDIASUBTYPE_RGB32 ? 32 :
			subtype == MEDIASUBTYPE_RGB24 ? 24 :
			subtype == MEDIASUBTYPE_RGB565 ? 16 : 0;

		if (bihOut.biCompression == FCC('YUY2')) {
			// TODO
			// BitBltFromRGBToYUY2();
		} else if (bihOut.biCompression == BI_RGB || bihOut.biCompression == BI_BITFIELDS) {
			if (!BitBltFromRGBToRGB(w, h, pOut, pitchOut, bihOut.biBitCount, ppIn[0], pitchIn, sbpp)) {
				for (int y = 0; y < h; y++, pOut += pitchOut) {
					memsetd(pOut, 0, pitchOut);
				}
			}
		}
	} else {
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	return S_OK;
}

HRESULT CBaseVideoFilter::CheckInputType(const CMediaType* mtIn)
{
	return S_OK;
}

HRESULT CBaseVideoFilter::CheckOutputType(const CMediaType& mtOut)
{
	return S_OK;
}

HRESULT CBaseVideoFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	return S_OK;
}

HRESULT CBaseVideoFilter::DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties)
{
	if (m_pInput->IsConnected() == FALSE) {
		return E_UNEXPECTED;
	}

	BITMAPINFOHEADER bih;
	ExtractBIH(&m_pOutput->CurrentMediaType(), &bih);

	long cBuffers = m_pOutput->CurrentMediaType().formattype == FORMAT_VideoInfo ? 1 : m_cBuffers;
	UNREFERENCED_PARAMETER(cBuffers);

	pProperties->cBuffers	= m_cBuffers;
	pProperties->cbBuffer	= bih.biSizeImage;
	pProperties->cbAlign	= 1;
	pProperties->cbPrefix	= 0;

	HRESULT hr;
	ALLOCATOR_PROPERTIES Actual;
	if (FAILED(hr = pAllocator->SetProperties(pProperties, &Actual))) {
		return hr;
	}

	return pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer
		   ? E_FAIL
		   : NOERROR;
}

HRESULT CBaseVideoFilter::GetMediaType(int iPosition, CMediaType* pmt)
{
	VIDEO_OUTPUT_FORMATS*	fmts;
	int						nFormatCount;

	if (m_pInput->IsConnected() == FALSE) {
		return E_UNEXPECTED;
	}

	// this will make sure we won't connect to the old renderer in dvd mode
	// that renderer can't switch the format dynamically

	bool fFoundDVDNavigator = false;
	CComPtr<IBaseFilter> pBF = this;
	CComPtr<IPin> pPin = m_pInput;
	for (; !fFoundDVDNavigator && (pBF = GetUpStreamFilter(pBF, pPin)); pPin = GetFirstPin(pBF)) {
		fFoundDVDNavigator = !!(GetCLSID(pBF) == CLSID_DVDNavigator);
	}

	if (fFoundDVDNavigator || m_pInput->CurrentMediaType().formattype == FORMAT_VideoInfo2) {
		iPosition = iPosition*2;
	}

	GetOutputFormats(nFormatCount, &fmts);
	if (iPosition < 0) {
		return E_INVALIDARG;
	}
	if (iPosition >= 2*nFormatCount) {
		return VFW_S_NO_MORE_ITEMS;
	}

	pmt->majortype = MEDIATYPE_Video;
	pmt->subtype   = *fmts[iPosition/2].subtype;

	int w = m_win, h = m_hin, arx = m_arxin, ary = m_aryin;
	int vsfilter = 0;
	GetOutputSize(w, h, arx, ary, vsfilter);

	m_wout = m_win;
	m_hout = m_hin;
	m_arxout = m_arxin;
	m_aryout = m_aryin;

	BITMAPINFOHEADER bihOut = { 0 };
	bihOut.biSize        = sizeof(bihOut);
	bihOut.biWidth       = w;
	bihOut.biHeight      = h;
	bihOut.biPlanes      = fmts[iPosition/2].biPlanes;
	bihOut.biBitCount    = fmts[iPosition/2].biBitCount;
	bihOut.biCompression = fmts[iPosition/2].biCompression;
	bihOut.biSizeImage   = DIBSIZE(bihOut);

	if (iPosition&1) {
		pmt->formattype = FORMAT_VideoInfo;
		VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
		memset(vih, 0, sizeof(VIDEOINFOHEADER));
		vih->bmiHeader = bihOut;
		vih->bmiHeader.biXPelsPerMeter = vih->bmiHeader.biWidth * ary;
		vih->bmiHeader.biYPelsPerMeter = vih->bmiHeader.biHeight * arx;
	} else {
		pmt->formattype = FORMAT_VideoInfo2;
		VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER2));
		memset(vih2, 0, sizeof(VIDEOINFOHEADER2));
		vih2->bmiHeader = bihOut;
		vih2->dwPictAspectRatioX = arx;
		vih2->dwPictAspectRatioY = ary;
		if (IsVideoInterlaced()) {
			vih2->dwInterlaceFlags = AMINTERLACE_IsInterlaced | AMINTERLACE_DisplayModeBobOrWeave;
		}
	}

	CMediaType& mt = m_pInput->CurrentMediaType();

	// these fields have the same field offset in all four structs
	((VIDEOINFOHEADER*)pmt->Format())->AvgTimePerFrame = ((VIDEOINFOHEADER*)mt.Format())->AvgTimePerFrame;
	((VIDEOINFOHEADER*)pmt->Format())->dwBitRate       = ((VIDEOINFOHEADER*)mt.Format())->dwBitRate;
	((VIDEOINFOHEADER*)pmt->Format())->dwBitErrorRate  = ((VIDEOINFOHEADER*)mt.Format())->dwBitErrorRate;

	pmt->SetSampleSize(bihOut.biSizeImage);

	if (!vsfilter) {
		// copy source and target rectangles from input pin
		CMediaType&      pmtInput = m_pInput->CurrentMediaType();
		VIDEOINFOHEADER* vih      = (VIDEOINFOHEADER*)pmt->Format();
		VIDEOINFOHEADER* vihInput = (VIDEOINFOHEADER*)pmtInput.Format();

		ASSERT(vih);
		if (vihInput && (vihInput->rcSource.right != 0) && (vihInput->rcSource.bottom != 0)) {
			vih->rcSource = vihInput->rcSource;
			vih->rcTarget = vihInput->rcTarget;
		} else {
			vih->rcSource.right  = vih->rcTarget.right  = m_win;
			vih->rcSource.bottom = vih->rcTarget.bottom = m_hin;
		}
	}

	return S_OK;
}

HRESULT CBaseVideoFilter::SetMediaType(PIN_DIRECTION dir, const CMediaType* pmt)
{
	if (dir == PINDIR_INPUT) {
		m_w = m_h = m_arx = m_ary = 0;
		ExtractDim(pmt, m_w, m_h, m_arx, m_ary);
		m_win = m_w;
		m_hin = m_h;
		m_arxin = m_arx;
		m_aryin = m_ary;
		int vsfilter = 0;
		GetOutputSize(m_w, m_h, m_arx, m_ary, vsfilter);

		DWORD a = m_arx, b = m_ary;
		while (a) {
			int tmp = a;
			a = b % tmp;
			b = tmp;
		}
		if (b) {
			m_arx /= b, m_ary /= b;
		}
	} else if (dir == PINDIR_OUTPUT) {
		int wout = 0, hout = 0, arxout = 0, aryout = 0;
		ExtractDim(pmt, wout, hout, arxout, aryout);
		if (m_w == wout && m_h == hout && m_arx == arxout && m_ary == aryout) {
			m_wout = wout;
			m_hout = hout;
			m_arxout = arxout;
			m_aryout = aryout;
		}
	}

	return __super::SetMediaType(dir, pmt);
}

//
// CBaseVideoInputAllocator
//

CBaseVideoInputAllocator::CBaseVideoInputAllocator(HRESULT* phr)
	: CMemAllocator(NAME("CBaseVideoInputAllocator"), NULL, phr)
{
	if (phr) {
		*phr = S_OK;
	}
}

void CBaseVideoInputAllocator::SetMediaType(const CMediaType& mt)
{
	m_mt = mt;
}

STDMETHODIMP CBaseVideoInputAllocator::GetBuffer(IMediaSample** ppBuffer, REFERENCE_TIME* pStartTime, REFERENCE_TIME* pEndTime, DWORD dwFlags)
{
	if (!m_bCommitted) {
		return VFW_E_NOT_COMMITTED;
	}

	HRESULT hr = __super::GetBuffer(ppBuffer, pStartTime, pEndTime, dwFlags);

	if (SUCCEEDED(hr) && m_mt.majortype != GUID_NULL) {
		(*ppBuffer)->SetMediaType(&m_mt);
		m_mt.majortype = GUID_NULL;
	}

	return hr;
}

//
// CBaseVideoInputPin
//

CBaseVideoInputPin::CBaseVideoInputPin(TCHAR* pObjectName, CBaseVideoFilter* pFilter, HRESULT* phr, LPCWSTR pName)
	: CTransformInputPin(pObjectName, pFilter, phr, pName)
	, m_pAllocator(NULL)
{
}

CBaseVideoInputPin::~CBaseVideoInputPin()
{
	delete m_pAllocator;
}

STDMETHODIMP CBaseVideoInputPin::GetAllocator(IMemAllocator** ppAllocator)
{
	CheckPointer(ppAllocator, E_POINTER);

	if (m_pAllocator == NULL) {
		HRESULT hr = S_OK;
		m_pAllocator = DNew CBaseVideoInputAllocator(&hr);
		m_pAllocator->AddRef();
	}

	(*ppAllocator = m_pAllocator)->AddRef();

	return S_OK;
}

STDMETHODIMP CBaseVideoInputPin::ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt)
{
	CAutoLock cObjectLock(m_pLock);

	if (m_Connected) {
		CMediaType mt(*pmt);

		if (FAILED(CheckMediaType(&mt))) {
			return VFW_E_TYPE_NOT_ACCEPTED;
		}

		ALLOCATOR_PROPERTIES props, actual;

		CComPtr<IMemAllocator> pMemAllocator;
		if (FAILED(GetAllocator(&pMemAllocator))
				|| FAILED(pMemAllocator->Decommit())
				|| FAILED(pMemAllocator->GetProperties(&props))) {
			return E_FAIL;
		}

		BITMAPINFOHEADER bih;
		if (ExtractBIH(pmt, &bih) && bih.biSizeImage) {
			props.cbBuffer = bih.biSizeImage;
		}

		if (FAILED(pMemAllocator->SetProperties(&props, &actual))
				|| FAILED(pMemAllocator->Commit())
				|| props.cbBuffer != actual.cbBuffer) {
			return E_FAIL;
		}

		if (m_pAllocator) {
			m_pAllocator->SetMediaType(mt);
		}

		return SetMediaType(&mt) == S_OK
			   ? S_OK
			   : VFW_E_TYPE_NOT_ACCEPTED;
	}

	return __super::ReceiveConnection(pConnector, pmt);
}

//
// CBaseVideoOutputPin
//

CBaseVideoOutputPin::CBaseVideoOutputPin(TCHAR* pObjectName, CBaseVideoFilter* pFilter, HRESULT* phr, LPCWSTR pName)
	: CTransformOutputPin(pObjectName, pFilter, phr, pName)
{
}

HRESULT CBaseVideoOutputPin::CheckMediaType(const CMediaType* mtOut)
{
	if (IsConnected()) {
		HRESULT hr = (static_cast<CBaseVideoFilter*>(m_pFilter))->CheckOutputType(*mtOut);
		if (FAILED(hr)) {
			return hr;
		}
	}

	return __super::CheckMediaType(mtOut);
}

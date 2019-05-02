/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2018 see Authors.txt
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
#include <atlpath.h>
#include <math.h>
#include <time.h>
#include <InitGuid.h>
#include "DirectVobSubFilter.h"
#include "TextInputPin.h"
#include "DirectVobSubPropPage.h"
#include "VSFilter.h"
#include "Systray.h"
#include "../../../DSUtil/FileHandle.h"
#include "../../../DSUtil/FileVersion.h"
#include "../../../DSUtil/WinAPIUtils.h"
#include "../../../DSUtil/std_helper.h"
#include "../../../SubPic/MemSubPic.h"
#include "../../../SubPic/SubPicQueueImpl.h"

///////////////////////////////////////////////////////////////////////////

/*removeme*/
bool g_RegOK = true;//false; // doesn't work with the dvd graph builder

////////////////////////////////////////////////////////////////////////////
//
// Constructor
//

CDirectVobSubFilter::CDirectVobSubFilter(LPUNKNOWN punk, HRESULT* phr, const GUID& clsid)
	: CBaseVideoFilter(L"CDirectVobSubFilter", punk, phr, clsid)
	, m_nSubtitleId((DWORD_PTR)-1)
	, m_bMSMpeg4Fix(false)
	, m_fps(25)
	, m_pVideoOutputFormat(nullptr)
	, m_nVideoOutputCount(0)
	, m_hEvtTransform(nullptr)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	m_hdc = 0;
	m_hbm = 0;
	m_hfont = 0;

	{
		LOGFONT lf;
		memset(&lf, 0, sizeof(lf));
		lf.lfCharSet = DEFAULT_CHARSET;
		lf.lfOutPrecision = OUT_CHARACTER_PRECIS;
		lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		lf.lfQuality = ANTIALIASED_QUALITY;
		HDC hdc = GetDC(nullptr);
		lf.lfHeight = -MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		ReleaseDC(nullptr, hdc);
		lf.lfWeight = FW_BOLD;
		wcscpy_s(lf.lfFaceName, L"Arial");
		m_hfont = CreateFontIndirectW(&lf);
	}

	theApp.WriteProfileString(ResStr(IDS_R_DEFTEXTPATHES), L"Hint", L"The first three are fixed, but you can add more up to ten entries.");
	theApp.WriteProfileString(ResStr(IDS_R_DEFTEXTPATHES), L"Path0", L".");
	theApp.WriteProfileString(ResStr(IDS_R_DEFTEXTPATHES), L"Path1", L"c:\\subtitles");
	theApp.WriteProfileString(ResStr(IDS_R_DEFTEXTPATHES), L"Path2", L".\\subtitles");

	m_bLoading = true;

	m_hSystrayThread = 0;
	m_tbid.hSystrayWnd = nullptr;
	m_tbid.graph = nullptr;
	m_tbid.dvs = nullptr;
	m_tbid.fRunOnce = false;
	m_tbid.fShowIcon = (theApp.m_AppName.Find(L"zplayer", 0) < 0 || !!theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_ENABLEZPICON), 0));

	HRESULT hr = S_OK;
	m_pTextInputs.push_back(DNew CTextInputPin(this, m_pLock, &m_csSubLock, &hr));
	ASSERT(SUCCEEDED(hr));

	CAMThread::Create();
	m_frd.EndThreadEvent.Create(0, FALSE, FALSE, 0);
	m_frd.RefreshEvent.Create(0, FALSE, FALSE, 0);

	memset(&m_CurrentVIH2, 0, sizeof(VIDEOINFOHEADER2));

	m_hEvtTransform = CreateEventW(nullptr, FALSE, TRUE, nullptr);
}

CDirectVobSubFilter::~CDirectVobSubFilter()
{
	CAutoLock cAutoLock(&m_csQueueLock);
	if (m_pSubPicQueue) {
		m_pSubPicQueue->Invalidate();
	}
	m_pSubPicQueue = nullptr;

	if (m_hfont) {
		DeleteObject(m_hfont);
		m_hfont = 0;
	}
	if (m_hbm) {
		DeleteObject(m_hbm);
		m_hbm = 0;
	}
	if (m_hdc) {
		DeleteObject(m_hdc);
		m_hdc = 0;
	}

	for (auto& pTextInput : m_pTextInputs) {
		delete pTextInput;
	}

	SAFE_DELETE_ARRAY(m_pVideoOutputFormat);

	m_frd.EndThreadEvent.Set();
	CAMThread::Close();

	if (m_hEvtTransform) {
		CloseHandle(m_hEvtTransform);
		m_hEvtTransform = nullptr;
	}
}

STDMETHODIMP CDirectVobSubFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		QI(IDirectVobSub)
		QI(IDirectVobSub2)
		QI(IDirectVobSub3)
		QI(IFilterVersion)
		QI(ISpecifyPropertyPages)
		QI(IAMStreamSelect)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// CBaseVideoFilter

HRESULT CDirectVobSubFilter::CopyBuffer(BYTE* pOut, BYTE** ppIn, int w, int h, int pitchIn, const GUID& subtype, bool fInterlaced)
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
			std::swap(pInU, pInV);
		}

		BYTE* pOutU = pOut + bihOut.biWidth * h;
		BYTE* pOutV = pOut + bihOut.biWidth * h * 5 / 4;

		if (bihOut.biCompression == FCC('YV12')) {
			std::swap(pOutU, pOutV);
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

void CDirectVobSubFilter::GetOutputFormats(int& nNumber, VIDEO_OUTPUT_FORMATS** ppFormats)
{
	nNumber		= m_nVideoOutputCount;
	*ppFormats	= m_pVideoOutputFormat;
}

void CDirectVobSubFilter::GetOutputSize(int& w, int& h, int& arx, int& ary, int& vsfilter)
{
	CSize s(w, h), os = s;
	AdjustFrameSize(s);
	w = s.cx;
	h = s.cy;
	vsfilter = 1; // enable workaround, see BaseVideoFilter.cpp

	if (w != os.cx) {
		while (arx < 100) {
			arx *= 10, ary *= 10;
		}
		arx = arx * w / os.cx;
	}

	if (h != os.cy) {
		while (ary < 100) {
			arx *= 10, ary *= 10;
		}
		ary = ary * h / os.cy;
	}
}

HRESULT CDirectVobSubFilter::Transform(IMediaSample* pIn)
{
	EXECUTE_ASSERT(WAIT_OBJECT_0 == WaitForSingleObject(m_hEvtTransform, INFINITE));

	REFERENCE_TIME rtStart, rtStop;
	HRESULT hr = pIn->GetTime(&rtStart, &rtStop);
	if (FAILED(hr)) {
		rtStart = rtStop = INVALID_TIME;
	} else if (hr == VFW_S_NO_STOP_TIME || rtStop - 1 <= rtStart) {
		rtStop = INVALID_TIME;
	}

	double dRate = m_pInput->CurrentRate();
	if (rtStart != INVALID_TIME) {
		m_tPrev = m_pInput->CurrentStartTime() + dRate * rtStart;
	}

	if (rtStop != INVALID_TIME) {
		REFERENCE_TIME rtAvgTimePerFrame = rtStop - rtStart;
		if (CComQIPtr<ISubClock2> pSC2 = m_pSubClock) {
			REFERENCE_TIME rt;
			if (S_OK == pSC2->GetAvgTimePerFrame(&rt)) {
				rtAvgTimePerFrame = rt;
			}
		}

		m_fps = 10000000.0 / rtAvgTimePerFrame / dRate;
	}

	{
		CAutoLock cAutoLock(&m_csQueueLock);

		if (m_pSubPicQueue) {
			m_pSubPicQueue->SetTime(CalcCurrentTime());
			m_pSubPicQueue->SetFPS(m_fps);
		}
	}

	BYTE* pDataIn = nullptr;
	if (FAILED(pIn->GetPointer(&pDataIn)) || !pDataIn) {
		SetEvent(m_hEvtTransform);
		return S_FALSE;
	}

	const CMediaType& mt = m_pInput->CurrentMediaType();

	BITMAPINFOHEADER bihIn;
	ExtractBIH(&mt, &bihIn);

	bool fYV12 = (mt.subtype == MEDIASUBTYPE_YV12 || mt.subtype == MEDIASUBTYPE_I420 || mt.subtype == MEDIASUBTYPE_IYUV);
	int bpp = fYV12 ? 8 : bihIn.biBitCount;
	DWORD black = fYV12 ? 0x10101010 : (bihIn.biCompression == FCC('YUY2')) ? 0x80108010 : 0;

	if (mt.subtype == MEDIASUBTYPE_P010 || mt.subtype == MEDIASUBTYPE_P016) {
		bpp = 16;
		black = 0x10001000;
	} else if (mt.subtype == MEDIASUBTYPE_NV12) {
		bpp = 8;
		black = 0x10101010;
	}
	CSize sub(m_wout, m_hout);
	CSize in(bihIn.biWidth, bihIn.biHeight);

	if (FAILED(Copy((BYTE*)m_pTempPicBuff, pDataIn, sub, in, bpp, mt.subtype, black))) {
		SetEvent(m_hEvtTransform);
		return E_FAIL;
	}

	if (fYV12) {
		BYTE* pSubV = (BYTE*)m_pTempPicBuff + (sub.cx*bpp>>3)*sub.cy;
		BYTE* pInV = pDataIn + (in.cx*bpp>>3)*in.cy;
		sub.cx >>= 1;
		sub.cy >>= 1;
		in.cx >>= 1;
		in.cy >>= 1;
		BYTE* pSubU = pSubV + (sub.cx*bpp>>3)*sub.cy;
		BYTE* pInU = pInV + (in.cx*bpp>>3)*in.cy;
		if (FAILED(Copy(pSubV, pInV, sub, in, bpp, mt.subtype, 0x80808080))) {
			SetEvent(m_hEvtTransform);
			return E_FAIL;
		}
		if (FAILED(Copy(pSubU, pInU, sub, in, bpp, mt.subtype, 0x80808080))) {
			SetEvent(m_hEvtTransform);
			return E_FAIL;
		}
	}

	if (mt.subtype == MEDIASUBTYPE_P010 || mt.subtype == MEDIASUBTYPE_P016 || mt.subtype == MEDIASUBTYPE_NV12) {
		BYTE* pSubUV = (BYTE*)m_pTempPicBuff + (sub.cx * bpp >> 3) * sub.cy;
		BYTE* pInUV = pDataIn + (in.cx * bpp >> 3) * in.cy;
		sub.cy >>= 1;
		in.cy >>= 1;
		if (FAILED(Copy(pSubUV, pInUV, sub, in, bpp, mt.subtype, mt.subtype == MEDIASUBTYPE_NV12 ? 0x80808080 : 0x80008000))) {
			SetEvent(m_hEvtTransform);
			return E_FAIL;
		}
	}

	DXVA2_ExtendedFormat dxvaExtFormat = { 0 };
	if (mt.formattype == FORMAT_VideoInfo2) {
		VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)mt.Format();
		dxvaExtFormat.value = vih2->dwControlFlags;
	}

	SubPicDesc spd = m_spd;
	CComPtr<IMediaSample> pOut;
	BYTE* pDataOut = nullptr;
	if (FAILED(hr = GetDeliveryBuffer(spd.w, spd.h, &pOut, 0, &dxvaExtFormat))
			|| FAILED(hr = pOut->GetPointer(&pDataOut))) {
		SetEvent(m_hEvtTransform);
		return hr;
	}

	if (rtStop == INVALID_TIME) {
		if (m_fps > 0.0) {
			REFERENCE_TIME rtAvgTimePerFrame = 10000000.0 / m_fps;
			rtStop = rtStart + rtAvgTimePerFrame;
		} else {
			rtStop = rtStart + 1;
		}
	}
	pOut->SetTime(&rtStart, &rtStop);
	pOut->SetMediaTime(nullptr, nullptr);

	pOut->SetDiscontinuity(pIn->IsDiscontinuity() == S_OK);
	pOut->SetSyncPoint(pIn->IsSyncPoint() == S_OK);
	pOut->SetPreroll(pIn->IsPreroll() == S_OK);

	BITMAPINFOHEADER bihOut;
	ExtractBIH(&m_pOutput->CurrentMediaType(), &bihOut);

	bool fInputFlipped = bihIn.biHeight >= 0 && bihIn.biCompression <= 3;
	bool fOutputFlipped = bihOut.biHeight >= 0 && bihOut.biCompression <= 3;

	bool fFlip = fInputFlipped != fOutputFlipped;
	if (m_bFlipPicture) {
		fFlip = !fFlip;
	}
	if (m_bMSMpeg4Fix) {
		fFlip = !fFlip;
	}

	bool fFlipSub = fOutputFlipped;
	if (m_bFlipSubtitles) {
		fFlipSub = !fFlipSub;
	}

	{
		CAutoLock cAutoLock(&m_csQueueLock);

		if (m_pSubPicQueue) {
			CComPtr<ISubPic> pSubPic;
			if ((m_pSubPicQueue->LookupSubPic(CalcCurrentTime(), pSubPic)) && pSubPic) {
				CRect r;
				pSubPic->GetDirtyRect(r);

				if (fFlip ^ fFlipSub) {
					spd.h = -spd.h;
				}

				pSubPic->AlphaBlt(r, r, &spd);
			}
		}
	}

	CopyBuffer(pDataOut, (BYTE*)spd.bits, spd.w, abs(spd.h)*(fFlip?-1:1), spd.pitch, mt.subtype);

	{
		// copy dwTypeSpecificFlags from input IMediaSample
		if (CComQIPtr<IMediaSample2> pMS2in = pIn) {
			AM_SAMPLE2_PROPERTIES propsIn;
			if (SUCCEEDED(pMS2in->GetProperties(sizeof(propsIn), (BYTE*)&propsIn))) {
				if (CComQIPtr<IMediaSample2> pMS2out = pOut) {
					AM_SAMPLE2_PROPERTIES propsOut;
					if (SUCCEEDED(pMS2out->GetProperties(sizeof(propsOut), (BYTE*)&propsOut))) {
						propsOut.dwTypeSpecificFlags = propsIn.dwTypeSpecificFlags;
						pMS2out->SetProperties(sizeof(propsOut), (BYTE*)&propsOut);
					}
				}
			}
		}
	}

	SetEvent(m_hEvtTransform);
	PrintMessages(pDataOut);
	return m_pOutput->Deliver(pOut);
}

// CBaseFilter

CBasePin* CDirectVobSubFilter::GetPin(int n)
{
	if (n < __super::GetPinCount()) {
		return __super::GetPin(n);
	}

	n -= __super::GetPinCount();

	if (n >= 0 && n < (int)m_pTextInputs.size()) {
		return m_pTextInputs[n];
	}

	n -= m_pTextInputs.size();

	return nullptr;
}

int CDirectVobSubFilter::GetPinCount()
{
	return __super::GetPinCount() + m_pTextInputs.size();
}

HRESULT CDirectVobSubFilter::JoinFilterGraph(IFilterGraph* pGraph, LPCWSTR pName)
{
	if (pGraph) {
		AFX_MANAGE_STATE(AfxGetStaticModuleState());

		if (!theApp.GetProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_SEENDIVXWARNING), 0)) {
			FileVersion::Ver ver = FileVersion::GetVer(L"divx_c32.ax");
			if (ver.major == 4 && ver.minor == 2) {
				AfxMessageBox(IDS_DIVX_WARNING, MB_ICONWARNING | MB_OK, 0);
				theApp.WriteProfileInt(ResStr(IDS_R_GENERAL), ResStr(IDS_RG_SEENDIVXWARNING), 1);
			}
		}

		/*removeme*/
		if (!g_RegOK) {
			DllRegisterServer();
			g_RegOK = true;
		}
	} else {
		if (m_hSystrayThread) {
			SendMessageW(m_tbid.hSystrayWnd, WM_CLOSE, 0, 0);

			if (WaitForSingleObject(m_hSystrayThread, 10000) != WAIT_OBJECT_0) {
				DLog(L"CALL THE AMBULANCE!!!");
				TerminateThread(m_hSystrayThread, (DWORD)-1);
			}

			m_hSystrayThread = 0;
		}
	}

	return __super::JoinFilterGraph(pGraph, pName);
}

STDMETHODIMP CDirectVobSubFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	if (!get_Forced()) {
		return __super::QueryFilterInfo(pInfo);
	}

	wcscpy_s(pInfo->achName, L"DirectVobSub (forced auto-loading version)");
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

// CTransformFilter

HRESULT CDirectVobSubFilter::SetMediaType(PIN_DIRECTION dir, const CMediaType* pmt)
{
	HRESULT hr = __super::SetMediaType(dir, pmt);
	if (FAILED(hr)) {
		return hr;
	}

	if (dir == PINDIR_INPUT) {
		CAutoLock cAutoLock(&m_csReceive);

		REFERENCE_TIME atpf =
			pmt->formattype == FORMAT_VideoInfo ? ((VIDEOINFOHEADER*)pmt->Format())->AvgTimePerFrame :
			pmt->formattype == FORMAT_VideoInfo2 ? ((VIDEOINFOHEADER2*)pmt->Format())->AvgTimePerFrame :
			0;

		m_fps = atpf ? 10000000.0 / atpf : 25;

		if (pmt->formattype == FORMAT_VideoInfo2) {
			m_CurrentVIH2 = *(VIDEOINFOHEADER2*)pmt->Format();
		}

		InitSubPicQueue();

		{
			SAFE_DELETE_ARRAY(m_pVideoOutputFormat);
			m_nVideoOutputCount = 0;

			IPin* pPin = m_pInput->GetConnected();
			std::list<VIDEO_OUTPUT_FORMATS> fmts;

			BeginEnumMediaTypes(pPin, pEMT, pmt) {
				for (const auto& VSFilterFormat : VSFilterDefaultFormats) {
					if (pmt->subtype == *VSFilterFormat.subtype && !Contains(fmts, VSFilterFormat)) {
						fmts.push_back(VSFilterFormat);
					}
				}
			}
			EndEnumMediaTypes(pmt)

			for (const auto& VSFilterFormat : VSFilterDefaultFormats) {
				if (!Contains(fmts, VSFilterFormat)) {
					fmts.push_back(VSFilterFormat);
				}
			}

			m_pVideoOutputFormat = DNew VIDEO_OUTPUT_FORMATS[fmts.size()];

			for (const auto& fmt : fmts) {
				m_pVideoOutputFormat[m_nVideoOutputCount++] = fmt;
			}
		}
	} else if (dir == PINDIR_OUTPUT) {

	}

	return hr;
}

HRESULT CDirectVobSubFilter::CompleteConnect(PIN_DIRECTION dir, IPin* pReceivePin)
{
	bool reconnected = false;
	if (dir == PINDIR_INPUT) {
		CComPtr<IBaseFilter> pFilter;

		// needed when we have a decoder with a version number of 3.x
		if (SUCCEEDED(m_pGraph->FindFilterByName(L"DivX MPEG-4 DVD Video Decompressor ", &pFilter))
				&& FileVersion::GetVer(L"divx_c32.ax").major <= 4
				|| SUCCEEDED(m_pGraph->FindFilterByName(L"Microcrap MPEG-4 Video Decompressor", &pFilter))
				|| SUCCEEDED(m_pGraph->FindFilterByName(L"Microsoft MPEG-4 Video Decompressor", &pFilter))
				&& FileVersion::GetVer(L"mpg4ds32.ax").major <= 3) {
			m_bMSMpeg4Fix = true;
		}
	} else if (dir == PINDIR_OUTPUT) {
		const CMediaType* mtIn	= &m_pInput->CurrentMediaType();
		const CMediaType* mtOut	= &m_pOutput->CurrentMediaType();
		CMediaType desiredMt;

		bool can_reconnect	= false;
		bool can_transform	= SUCCEEDED(DoCheckTransform(mtIn, mtOut, true));
		if (mtIn->subtype != mtOut->subtype) {

			VIDEO_OUTPUT_FORMATS*	fmts;
			int						nFormatCount, iPosition;
			GetOutputFormats(nFormatCount, &fmts);
			for (int iPosition = 0; iPosition < nFormatCount; iPosition++) {
				if (mtOut->subtype == *fmts[iPosition].subtype) {
					if (GetMediaType(iPosition, &desiredMt) != S_OK) {
						break;
					}

					if (SUCCEEDED(DoCheckTransform(&desiredMt, mtOut, true)) && SUCCEEDED(m_pInput->GetConnected()->QueryAccept(&desiredMt))) {
						can_reconnect = true;
						break;
					}
				}
			}

			if (!can_reconnect && !can_transform) {
				iPosition = 0;
				for (;;) {
					if (GetMediaType(iPosition, &desiredMt) != S_OK) {
						break;
					}
					iPosition++;

					if (SUCCEEDED(DoCheckTransform(&desiredMt, mtOut, true)) && SUCCEEDED(m_pInput->GetConnected()->QueryAccept(&desiredMt))) {
						can_reconnect = true;
						break;
					}
				}
			}
		}

		if (can_reconnect) {
			if (SUCCEEDED(ReconnectPin(m_pInput, &desiredMt))) {
				reconnected = true;
			}
		} else if (!can_transform) {
			if(m_pInput->IsConnected()) {
				m_pInput->GetConnected()->Disconnect();
				m_pInput->Disconnect();
			} if(m_pOutput->IsConnected()) {
				m_pOutput->GetConnected()->Disconnect();
				m_pOutput->Disconnect();
			}
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	}

	if (!reconnected && m_pOutput->IsConnected()) {
		if (!m_hSystrayThread) {
			m_tbid.graph = m_pGraph;
			m_tbid.dvs = static_cast<IDirectVobSub*>(this);

			DWORD tid;
			m_hSystrayThread = CreateThread(0, 0, SystrayThreadProc, &m_tbid, 0, &tid);
		}

		// HACK: triggers CBaseVideoFilter::SetMediaType to adjust m_w/m_h/.. and InitSubPicQueue() to realloc buffers
		m_pInput->SetMediaType(&m_pInput->CurrentMediaType());
	}

	return __super::CompleteConnect(dir, pReceivePin);
}

HRESULT CDirectVobSubFilter::BreakConnect(PIN_DIRECTION dir)
{
	if (dir == PINDIR_INPUT) {
		/*
		if (m_pOutput->IsConnected()) {
			m_pOutput->GetConnected()->Disconnect();
			m_pOutput->Disconnect();
		}
		*/
	} else if (dir == PINDIR_OUTPUT) {
		// not really needed, but may free up a little memory
		CAutoLock cAutoLock(&m_csQueueLock);
		m_pSubPicQueue = nullptr;
	}

	return __super::BreakConnect(dir);
}

HRESULT CDirectVobSubFilter::StartStreaming()
{
	// WARNING : calls to m_pGraph member functions from here will generate deadlock with Haali Renderer
	// within MPC-BE (reason is CAutoLock's variables in IFilterGraph functions overriden by CFGManager class)

	m_bLoading = false;

	InitSubPicQueue();

	m_tbid.fRunOnce = true;

	put_MediaFPS(m_bMediaFPSEnabled, m_MediaFPS);

	return __super::StartStreaming();
}

HRESULT CDirectVobSubFilter::StopStreaming()
{
	InvalidateSubtitle();

	return __super::StopStreaming();
}

HRESULT CDirectVobSubFilter::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	m_tPrev = tStart;
	return __super::NewSegment(tStart, tStop, dRate);
}

HRESULT CDirectVobSubFilter::CheckInputType(const CMediaType* mtIn)
{
	BITMAPINFOHEADER bih;
	ExtractBIH(mtIn, &bih);

	CComPtr<IBaseFilter> pFilter;

	return mtIn->majortype == MEDIATYPE_Video
		&& (mtIn->subtype == MEDIASUBTYPE_P016
		       || mtIn->subtype == MEDIASUBTYPE_P010
		       || mtIn->subtype == MEDIASUBTYPE_NV12
		       || mtIn->subtype == MEDIASUBTYPE_YV12
			   || mtIn->subtype == MEDIASUBTYPE_I420
			   || (mtIn->subtype == MEDIASUBTYPE_IYUV && FAILED(m_pGraph->FindFilterByName(L"Lentoid HEVC Decoder", &pFilter)))
			   || mtIn->subtype == MEDIASUBTYPE_YUY2
			   || mtIn->subtype == MEDIASUBTYPE_ARGB32
			   || mtIn->subtype == MEDIASUBTYPE_RGB32
			   || mtIn->subtype == MEDIASUBTYPE_RGB24
			   || mtIn->subtype == MEDIASUBTYPE_RGB565)
		   && (mtIn->formattype == FORMAT_VideoInfo
			   || mtIn->formattype == FORMAT_VideoInfo2)
		   && bih.biHeight > 0
		   ? S_OK
		   : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CDirectVobSubFilter::CheckOutputType(const CMediaType& mtOut)
{
	int wout = 0, hout = 0, arxout = 0, aryout = 0;
	return ExtractDim(&mtOut, wout, hout, arxout, aryout)
		   && m_hin == abs((int)hout)
		   && mtOut.subtype != MEDIASUBTYPE_ARGB32
		   ? S_OK
		   : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CDirectVobSubFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	return DoCheckTransform(mtIn, mtOut, false);
}

HRESULT CDirectVobSubFilter::DoCheckTransform(const CMediaType* mtIn, const CMediaType* mtOut, bool checkReconnection)
{
	if (FAILED(CheckInputType(mtIn)) || mtOut->majortype != MEDIATYPE_Video) {
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	if (mtIn->majortype == MEDIATYPE_Video
				&& (mtIn->subtype == MEDIASUBTYPE_P016 || mtIn->subtype == MEDIASUBTYPE_P010)) {
		if (mtOut->subtype != mtIn->subtype && checkReconnection) {
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	} else if (mtIn->majortype == MEDIATYPE_Video
				&& (mtIn->subtype == MEDIASUBTYPE_YV12
				|| mtIn->subtype == MEDIASUBTYPE_I420
				|| mtIn->subtype == MEDIASUBTYPE_IYUV)) {
		if (mtOut->subtype != MEDIASUBTYPE_YV12
				&& mtOut->subtype != MEDIASUBTYPE_NV12
				&& mtOut->subtype != MEDIASUBTYPE_I420
				&& mtOut->subtype != MEDIASUBTYPE_IYUV
				&& mtOut->subtype != MEDIASUBTYPE_YUY2
				&& mtOut->subtype != MEDIASUBTYPE_ARGB32
				&& mtOut->subtype != MEDIASUBTYPE_RGB32
				&& mtOut->subtype != MEDIASUBTYPE_RGB24
				&& mtOut->subtype != MEDIASUBTYPE_RGB565) {
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	} else if (mtOut->majortype == MEDIATYPE_Video
				&& (mtOut->subtype == MEDIASUBTYPE_P016 || mtOut->subtype == MEDIASUBTYPE_P010 || mtOut->subtype == MEDIASUBTYPE_NV12)) {
		if (mtOut->subtype != mtIn->subtype) {
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	} else if (mtIn->majortype == MEDIATYPE_Video
			   && (mtIn->subtype == MEDIASUBTYPE_YUY2)) {
		if (mtOut->subtype != MEDIASUBTYPE_YUY2
				&& mtOut->subtype != MEDIASUBTYPE_ARGB32
				&& mtOut->subtype != MEDIASUBTYPE_RGB32
				&& mtOut->subtype != MEDIASUBTYPE_RGB24
				&& mtOut->subtype != MEDIASUBTYPE_RGB565) {
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	} else if (mtIn->majortype == MEDIATYPE_Video
			   && (mtIn->subtype == MEDIASUBTYPE_ARGB32
				   || mtIn->subtype == MEDIASUBTYPE_RGB32
				   || mtIn->subtype == MEDIASUBTYPE_RGB24
				   || mtIn->subtype == MEDIASUBTYPE_RGB565)) {
		if (mtOut->subtype != MEDIASUBTYPE_ARGB32
				&& mtOut->subtype != MEDIASUBTYPE_RGB32
				&& mtOut->subtype != MEDIASUBTYPE_RGB24
				&& mtOut->subtype != MEDIASUBTYPE_RGB565) {
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	}

	return S_OK;
}

//

REFERENCE_TIME CDirectVobSubFilter::CalcCurrentTime()
{
	REFERENCE_TIME rt = m_pSubClock ? m_pSubClock->GetTime() : m_tPrev;
	return (rt - 10000i64*m_SubtitleDelay) * m_SubtitleSpeedMul / m_SubtitleSpeedDiv; // no, it won't overflow if we use normal parameters (__int64 is enough for about 2000 hours if we multiply it by the max: 65536 as m_SubtitleSpeedMul)
}

void CDirectVobSubFilter::InitSubPicQueue()
{
	EXECUTE_ASSERT(WAIT_OBJECT_0 == WaitForSingleObject(m_hEvtTransform, INFINITE));

	CAutoLock cAutoLock(&m_csQueueLock);

	m_pSubPicQueue = nullptr;

	const GUID& subtype = m_pInput->CurrentMediaType().subtype;

	BITMAPINFOHEADER bihIn;
	ExtractBIH(&m_pInput->CurrentMediaType(), &bihIn);

	m_spd.type = -1;
	if (subtype == MEDIASUBTYPE_YV12) {
		m_spd.type = MSP_YV12;
	} else if (subtype == MEDIASUBTYPE_P010) {
		m_spd.type = MSP_P010;
	} else if (subtype == MEDIASUBTYPE_P016) {
		m_spd.type = MSP_P016;
	} else if (subtype == MEDIASUBTYPE_NV12) {
		m_spd.type = MSP_NV12;
	} else if (subtype == MEDIASUBTYPE_I420 || subtype == MEDIASUBTYPE_IYUV) {
		m_spd.type = MSP_IYUV;
	} else if (subtype == MEDIASUBTYPE_YUY2) {
		m_spd.type = MSP_YUY2;
	} else if (subtype == MEDIASUBTYPE_RGB32) {
		m_spd.type = MSP_RGB32;
	} else if (subtype == MEDIASUBTYPE_RGB24) {
		m_spd.type = MSP_RGB24;
	} else if (subtype == MEDIASUBTYPE_RGB565) {
		m_spd.type = MSP_RGB16;
	} else if (subtype == MEDIASUBTYPE_RGB555) {
		m_spd.type = MSP_RGB15;
	}
	m_spd.w = m_wout;
	m_spd.h = m_hout;
	m_spd.bpp = (m_spd.type == MSP_YV12 || m_spd.type == MSP_IYUV || m_spd.type == MSP_NV12) ? 8 : bihIn.biBitCount;
	m_spd.bpp = (m_spd.type == MSP_P010 || m_spd.type == MSP_P016) ? 16 : m_spd.bpp;
	m_spd.pitch = m_spd.w * m_spd.bpp >> 3;

	m_pTempPicBuff.Free();
	if (m_spd.type == MSP_YV12 || m_spd.type == MSP_IYUV || m_spd.type == MSP_NV12) {
		m_pTempPicBuff.Allocate(4 * m_spd.pitch * m_spd.h);
	} else if (m_spd.type == MSP_P010 || m_spd.type == MSP_P016) {
		m_pTempPicBuff.Allocate(m_spd.pitch * m_spd.h + m_spd.pitch * m_spd.h / 2);
	} else {
		m_pTempPicBuff.Allocate(m_spd.pitch * m_spd.h);
	}
	m_spd.bits = (void*)m_pTempPicBuff;

	CComPtr<ISubPicAllocator> pSubPicAllocator = DNew CMemSubPicAllocator(m_spd.type, CSize(m_wout, m_hout));

	CSize video(bihIn.biWidth, bihIn.biHeight), window = video;
	if (AdjustFrameSize(window)) {
		video += video;
	}
	ASSERT(window == CSize(m_win, m_hin));

	pSubPicAllocator->SetCurSize(window);
	pSubPicAllocator->SetCurVidRect(CRect(CPoint((window.cx - video.cx) / 2, (window.cy - video.cy) / 2), video));

	HRESULT hr = S_OK;

	m_pSubPicQueue = m_uSubPictToBuffer > 0
					 ? (ISubPicQueue*)DNew CSubPicQueue(m_uSubPictToBuffer, !m_bAnimWhenBuffering, m_bAllowDropSubPic, pSubPicAllocator, &hr)
					 : (ISubPicQueue*)DNew CSubPicQueueNoThread(!m_bAnimWhenBuffering, pSubPicAllocator, &hr);

	if (FAILED(hr)) {
		m_pSubPicQueue = nullptr;
	}

	UpdateSubtitle(false);

	if (m_hbm) {
		DeleteObject(m_hbm);
		m_hbm = nullptr;
	}
	if (m_hdc) {
		DeleteDC(m_hdc);
		m_hdc = nullptr;
	}

	static struct {
		BITMAPINFOHEADER bih;
		DWORD mask[3];
	} b = {{sizeof(BITMAPINFOHEADER), m_win, -(int)m_hin, 1, 32, BI_BITFIELDS, 0, 0, 0, 0, 0}, 0xFF0000, 0x00FF00, 0x0000FF};
	m_hdc = CreateCompatibleDC(nullptr);
	m_hbm = CreateDIBSection(m_hdc, (BITMAPINFO*)&b, DIB_RGB_COLORS, nullptr, nullptr, 0);

	BITMAP bm;
	GetObject(m_hbm, sizeof(bm), &bm);
	memsetd(bm.bmBits, 0xFF000000, bm.bmHeight * bm.bmWidthBytes);

	SetEvent(m_hEvtTransform);
}

bool CDirectVobSubFilter::AdjustFrameSize(CSize& s)
{
	int horizontal, vertical, resx2, resx2minw, resx2minh;
	get_ExtendPicture(&horizontal, &vertical, &resx2, &resx2minw, &resx2minh);

	bool fRet = (resx2 == 1) || (resx2 == 2 && s.cx*s.cy <= resx2minw*resx2minh);

	if (fRet) {
		s.cx <<= 1;
		s.cy <<= 1;
	}

	int h;
	switch (vertical&0x7f) {
		case 1:
			h = s.cx * 9 / 16;
			if (s.cy < h || !!(vertical&0x80)) {
				s.cy = (h + 3) & ~3;
			}
			break;
		case 2:
			h = s.cx * 3 / 4;
			if (s.cy < h || !!(vertical&0x80)) {
				s.cy = (h + 3) & ~3;
			}
			break;
		case 3:
			h = 480;
			if (s.cy < h || !!(vertical&0x80)) {
				s.cy = (h + 3) & ~3;
			}
			break;
		case 4:
			h = 576;
			if (s.cy < h || !!(vertical&0x80)) {
				s.cy = (h + 3) & ~3;
			}
			break;
	}

	if (horizontal == 1) {
		s.cx = (s.cx + 31) & ~31;
		s.cy = (s.cy + 1) & ~1;
	}

	return(fRet);
}

STDMETHODIMP CDirectVobSubFilter::Count(DWORD* pcStreams)
{
	if (!pcStreams) {
		return E_POINTER;
	}

	*pcStreams = 0;

	int nLangs = 0;
	if (SUCCEEDED(get_LanguageCount(&nLangs))) {
		(*pcStreams) += nLangs;
	}

	(*pcStreams) += 2; // enable ... disable

	(*pcStreams) += 2; // normal flipped

	return S_OK;
}

#define MAXPREFLANGS 5

int CDirectVobSubFilter::FindPreferedLanguage(bool fHideToo)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	int nLangs;
	get_LanguageCount(&nLangs);

	if (nLangs <= 0) {
		return(0);
	}

	for (int i = 0; i < MAXPREFLANGS; i++) {
		CString tmp;
		tmp.Format(IDS_RL_LANG, i);

		CString lang = theApp.GetProfileString(ResStr(IDS_R_PREFLANGS), tmp);

		if (!lang.IsEmpty()) {
			for (int ret = 0; ret < nLangs; ret++) {
				CString l;
				WCHAR* pName = nullptr;
				get_LanguageName(ret, &pName);
				l = pName;
				CoTaskMemFree(pName);

				if (!l.CompareNoCase(lang)) {
					return(ret);
				}
			}
		}
	}

	return(0);
}

void CDirectVobSubFilter::UpdatePreferedLanguages(CString l)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CString langs[MAXPREFLANGS+1];

	int i = 0, j = 0, k = -1;
	for (; i < MAXPREFLANGS; i++) {
		CString tmp;
		tmp.Format(IDS_RL_LANG, i);

		langs[j] = theApp.GetProfileString(ResStr(IDS_R_PREFLANGS), tmp);

		if (!langs[j].IsEmpty()) {
			if (!langs[j].CompareNoCase(l)) {
				k = j;
			}
			j++;
		}
	}

	if (k == -1) {
		langs[k = j] = l;
		j++;
	}

	// move the selected to the top of the list

	while (k > 0) {
		std::swap(langs[k], langs[k - 1]);
		/*
		CString tmp = langs[k];
		langs[k] = langs[k-1];
		langs[k-1] = tmp;
		*/
		k--;
	}

	// move "Hide subtitles" to the last position if it wasn't our selection

	CString hidesubs;
	hidesubs.LoadString(IDS_M_HIDESUBTITLES);

	for (k = 1; k < j; k++) {
		if (!langs[k].CompareNoCase(hidesubs)) {
			break;
		}
	}

	while (k < j-1) {
		std::swap(langs[k], langs[k + 1]);
		/*
		CString tmp = langs[k];
		langs[k] = langs[k+1];
		langs[k+1] = tmp;
		*/
		k++;
	}

	for (i = 0; i < j; i++) {
		CString tmp;
		tmp.Format(IDS_RL_LANG, i);

		theApp.WriteProfileString(ResStr(IDS_R_PREFLANGS), tmp, langs[i]);
	}
}

STDMETHODIMP CDirectVobSubFilter::Enable(long lIndex, DWORD dwFlags)
{
	if (!(dwFlags & AMSTREAMSELECTENABLE_ENABLE)) {
		return E_NOTIMPL;
	}

	int nLangs = 0;
	get_LanguageCount(&nLangs);

	if (!(lIndex >= 0 && lIndex < nLangs+2+2)) {
		return E_INVALIDARG;
	}

	int i = lIndex-1;

	if (i == -1 && !m_bLoading) { // we need this because when loading something stupid media player pushes the first stream it founds, which is "enable" in our case
		put_HideSubtitles(false);
	} else if (i >= 0 && i < nLangs) {
		put_HideSubtitles(false);
		put_SelectedLanguage(i);

		WCHAR* pName = nullptr;
		if (SUCCEEDED(get_LanguageName(i, &pName))) {
			UpdatePreferedLanguages(CString(pName));
			if (pName) {
				CoTaskMemFree(pName);
			}
		}
	} else if (i == nLangs && !m_bLoading) {
		put_HideSubtitles(true);
	} else if ((i == nLangs+1 || i == nLangs+2) && !m_bLoading) {
		put_Flip(i == nLangs+2, m_bFlipSubtitles);
	}

	return S_OK;
}

STDMETHODIMP CDirectVobSubFilter::Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	int nLangs = 0;
	get_LanguageCount(&nLangs);

	if (!(lIndex >= 0 && lIndex < nLangs+2+2)) {
		return E_INVALIDARG;
	}

	int i = lIndex-1;

	if (ppmt) {
		*ppmt = CreateMediaType(&m_pInput->CurrentMediaType());
	}

	if (pdwFlags) {
		*pdwFlags = 0;

		if (i == -1 && !m_bHideSubtitles
				|| i >= 0 && i < nLangs && i == m_iSelectedLanguage
				|| i == nLangs && m_bHideSubtitles
				|| i == nLangs+1 && !m_bFlipPicture
				|| i == nLangs+2 && m_bFlipPicture) {
			*pdwFlags |= AMSTREAMSELECTINFO_ENABLED;
		}
	}

	if (plcid) {
		*plcid = 0;
	}

	if (pdwGroup) {
		*pdwGroup = 0x648E51;
	}

	if (ppszName) {
		*ppszName = nullptr;

		CStringW str;
		if (i == -1) {
			str = ResStr(IDS_M_SHOWSUBTITLES);
		} else if (i >= 0 && i < nLangs) {
			get_LanguageName(i, ppszName);
		} else if (i == nLangs) {
			str = ResStr(IDS_M_HIDESUBTITLES);
		} else if (i == nLangs+1) {
			str = ResStr(IDS_M_ORIGINALPICTURE);
			if (pdwGroup) {
				(*pdwGroup)++;
			}
		} else if (i == nLangs+2) {
			str = ResStr(IDS_M_FLIPPEDPICTURE);
			if (pdwGroup) {
				(*pdwGroup)++;
			}
		}

		if (!str.IsEmpty()) {
			*ppszName = (WCHAR*)CoTaskMemAlloc((str.GetLength()+1)*sizeof(WCHAR));
			if (*ppszName == nullptr) {
				return S_FALSE;
			}
			wcscpy_s(*ppszName, str.GetLength() + 1, str);
		}
	}

	if (ppObject) {
		*ppObject = nullptr;
	}

	if (ppUnk) {
		*ppUnk = nullptr;
	}

	return S_OK;
}

STDMETHODIMP CDirectVobSubFilter::GetClassID(CLSID* pClsid)
{
	if (pClsid == nullptr) {
		return E_POINTER;
	}
	*pClsid = m_clsid;
	return NOERROR;
}

STDMETHODIMP CDirectVobSubFilter::GetPages(CAUUID* pPages)
{
	CheckPointer(pPages, E_POINTER);

	pPages->cElems = 7;
	pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID)*pPages->cElems);

	if (pPages->pElems == nullptr) {
		return E_OUTOFMEMORY;
	}

	int i = 0;
	pPages->pElems[i++] = __uuidof(CDVSMainPPage);
	pPages->pElems[i++] = __uuidof(CDVSGeneralPPage);
	pPages->pElems[i++] = __uuidof(CDVSMiscPPage);
	pPages->pElems[i++] = __uuidof(CDVSTimingPPage);
	pPages->pElems[i++] = __uuidof(CDVSColorPPage);
	pPages->pElems[i++] = __uuidof(CDVSPathsPPage);
	pPages->pElems[i++] = __uuidof(CDVSAboutPPage);

	return NOERROR;
}

// IDirectVobSub

STDMETHODIMP CDirectVobSubFilter::put_FileName(WCHAR* fn)
{
	HRESULT hr = CDirectVobSub::put_FileName(fn);

	if (hr == S_OK && !Open()) {
		m_FileName.Empty();
		hr = E_FAIL;
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::get_LanguageCount(int* nLangs)
{
	HRESULT hr = CDirectVobSub::get_LanguageCount(nLangs);

	if (hr == NOERROR && nLangs) {
		CAutoLock cAutolock(&m_csQueueLock);

		*nLangs = 0;
		POSITION pos = m_pSubStreams.GetHeadPosition();
		while (pos) {
			(*nLangs) += m_pSubStreams.GetNext(pos)->GetStreamCount();
		}
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::get_LanguageName(int iLanguage, WCHAR** ppName)
{
	HRESULT hr = CDirectVobSub::get_LanguageName(iLanguage, ppName);

	if (!ppName) {
		return E_POINTER;
	}

	if (hr == NOERROR) {
		CAutoLock cAutolock(&m_csQueueLock);

		hr = E_INVALIDARG;

		int i = iLanguage;

		POSITION pos = m_pSubStreams.GetHeadPosition();
		while (i >= 0 && pos) {
			CComPtr<ISubStream> pSubStream = m_pSubStreams.GetNext(pos);

			if (i < pSubStream->GetStreamCount()) {
				pSubStream->GetStreamInfo(i, ppName, nullptr);
				hr = NOERROR;
				break;
			}

			i -= pSubStream->GetStreamCount();
		}
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::put_SelectedLanguage(int iSelected)
{
	HRESULT hr = CDirectVobSub::put_SelectedLanguage(iSelected);

	if (hr == NOERROR) {
		UpdateSubtitle(false);
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::put_HideSubtitles(bool fHideSubtitles)
{
	HRESULT hr = CDirectVobSub::put_HideSubtitles(fHideSubtitles);

	if (hr == NOERROR) {
		UpdateSubtitle(false);
	}

	return hr;
}

// deprecated
STDMETHODIMP CDirectVobSubFilter::put_PreBuffering(bool fDoPreBuffering)
{
	HRESULT hr = CDirectVobSub::put_PreBuffering(fDoPreBuffering);

	if (hr == NOERROR && m_pInput && m_pInput->IsConnected()) {
		InitSubPicQueue();
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::put_SubPictToBuffer(unsigned int uSubPictToBuffer)
{
	HRESULT hr = CDirectVobSub::put_SubPictToBuffer(uSubPictToBuffer);

	if (hr == NOERROR && m_pInput && m_pInput->IsConnected()) {
		InitSubPicQueue();
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::put_AnimWhenBuffering(bool fAnimWhenBuffering)
{
	HRESULT hr = CDirectVobSub::put_AnimWhenBuffering(fAnimWhenBuffering);

	if (hr == NOERROR && m_pInput && m_pInput->IsConnected()) {
		InitSubPicQueue();
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::put_AllowDropSubPic(bool fAllowDropSubPic)
{
	HRESULT hr = CDirectVobSub::put_AllowDropSubPic(fAllowDropSubPic);

	if (hr == NOERROR && m_pInput && m_pInput->IsConnected()) {
		InitSubPicQueue();
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::put_Placement(bool fOverridePlacement, int xperc, int yperc)
{
	HRESULT hr = CDirectVobSub::put_Placement(fOverridePlacement, xperc, yperc);

	if (hr == NOERROR) {
		UpdateSubtitle();
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::put_VobSubSettings(bool fBuffer, bool fOnlyShowForcedSubs, bool fReserved)
{
	HRESULT hr = CDirectVobSub::put_VobSubSettings(fBuffer, fOnlyShowForcedSubs, fReserved);

	if (hr == NOERROR) {
		//		UpdateSubtitle(false);
		InvalidateSubtitle();
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::put_TextSettings(void* lf, int lflen, COLORREF color, bool fShadow, bool fOutline, bool fAdvancedRenderer)
{
	HRESULT hr = CDirectVobSub::put_TextSettings(lf, lflen, color, fShadow, fOutline, fAdvancedRenderer);

	if (hr == NOERROR) {
		//		UpdateSubtitle();
		InvalidateSubtitle();
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::put_SubtitleTiming(int delay, int speedmul, int speeddiv)
{
	HRESULT hr = CDirectVobSub::put_SubtitleTiming(delay, speedmul, speeddiv);

	if (hr == NOERROR) {
		InvalidateSubtitle();
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::get_MediaFPS(bool* fEnabled, double* fps)
{
	HRESULT hr = CDirectVobSub::get_MediaFPS(fEnabled, fps);

	CComQIPtr<IMediaSeeking> pMS = m_pGraph;
	double rate;
	if (pMS && SUCCEEDED(pMS->GetRate(&rate))) {
		m_MediaFPS = rate * m_fps;
		if (fps) {
			*fps = m_MediaFPS;
		}
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::put_MediaFPS(bool fEnabled, double fps)
{
	HRESULT hr = CDirectVobSub::put_MediaFPS(fEnabled, fps);

	CComQIPtr<IMediaSeeking> pMS = m_pGraph;
	if (pMS) {
		if (hr == NOERROR) {
			hr = pMS->SetRate(m_bMediaFPSEnabled ? m_MediaFPS / m_fps : 1.0);
		}

		double dRate;
		if (SUCCEEDED(pMS->GetRate(&dRate))) {
			m_MediaFPS = dRate * m_fps;
		}
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::get_ZoomRect(NORMALIZEDRECT* rect)
{
	return E_NOTIMPL;
}

STDMETHODIMP CDirectVobSubFilter::put_ZoomRect(NORMALIZEDRECT* rect)
{
	return E_NOTIMPL;
}

// IDirectVobSub2

STDMETHODIMP CDirectVobSubFilter::put_TextSettings(STSStyle* pDefStyle)
{
	HRESULT hr = CDirectVobSub::put_TextSettings(pDefStyle);

	if (hr == NOERROR) {
		UpdateSubtitle();
	}

	return hr;
}

STDMETHODIMP CDirectVobSubFilter::put_AspectRatioSettings(CSimpleTextSubtitle::EPARCompensationType* ePARCompensationType)
{
	HRESULT hr = CDirectVobSub::put_AspectRatioSettings(ePARCompensationType);

	if (hr == NOERROR) {
		UpdateSubtitle(false);
	}

	return hr;
}

// IDirectVobSub3

STDMETHODIMP CDirectVobSubFilter::get_LanguageType(int iLanguage, int* pType)
{
	HRESULT hr = E_INVALIDARG;
	WCHAR *pName;
	if (pType && SUCCEEDED(hr = get_LanguageName(iLanguage, &pName)) && pName) {
		*pType = 0;
		CoTaskMemFree(pName);

		int nLangs = 0;
		get_LanguageCount(&nLangs);

		int nEmbeddedCount = 0;
		for (const auto& pTextInput : m_pTextInputs) {
			if (pTextInput->IsConnected()) {
				CComPtr<ISubStream> pSubStream = pTextInput->GetSubStream();
				nEmbeddedCount += pSubStream->GetStreamCount();
			}
		}

		nLangs -= nEmbeddedCount;

		if (iLanguage < nLangs) {
			*pType = 1;
		}
	}

	return hr;
}


// IDirectVobSubFilterColor

STDMETHODIMP CDirectVobSubFilter::HasConfigDialog(int iSelected)
{
	int nLangs;
	if (FAILED(get_LanguageCount(&nLangs))) {
		return E_FAIL;
	}
	return E_FAIL;
	// TODO: temporally disabled since we don't have a new textsub/vobsub editor dlg for dvs yet
	//	return(nLangs >= 0 && iSelected < nLangs ? S_OK : E_FAIL);
}

STDMETHODIMP CDirectVobSubFilter::ShowConfigDialog(int iSelected, HWND hWndParent)
{
	// TODO: temporally disabled since we don't have a new textsub/vobsub editor dlg for dvs yet
	return(E_FAIL);
}

///////////////////////////////////////////////////////////////////////////

CDirectVobSubFilter2::CDirectVobSubFilter2(LPUNKNOWN punk, HRESULT* phr, const GUID& clsid) :
	CDirectVobSubFilter(punk, phr, clsid)
{
}

HRESULT CDirectVobSubFilter2::CheckConnect(PIN_DIRECTION dir, IPin* pPin)
{
	CPinInfo pi;
	if (FAILED(pPin->QueryPinInfo(&pi))) {
		return E_FAIL;
	}

	if (CComQIPtr<IDirectVobSub>(pi.pFilter)) {
		return E_FAIL;
	}

	if (dir == PINDIR_INPUT) {
		CFilterInfo fi;
		if (SUCCEEDED(pi.pFilter->QueryFilterInfo(&fi))
				&& !_wcsnicmp(fi.achName, L"Overlay Mixer", 13)) {
			return(E_FAIL);
		}
	} else {
	}

	return __super::CheckConnect(dir, pPin);
}

HRESULT CDirectVobSubFilter2::JoinFilterGraph(IFilterGraph* pGraph, LPCWSTR pName)
{
	if (pGraph) {
		BeginEnumFilters(pGraph, pEF, pBF) {
			if (pBF != (IBaseFilter*)this && CComQIPtr<IDirectVobSub>(pBF)) {
				return E_FAIL;
			}
		}
		EndEnumFilters

		// don't look... we will do some serious graph hacking again...
		//
		// we will add dvs2 to the filter graph cache
		// - if the main app has already added some kind of renderer or overlay mixer (anything which accepts video on its input)
		// and
		// - if we have a reason to auto-load (we don't want to make any trouble when there is no need :)
		//
		// This whole workaround is needed because the video stream will always be connected
		// to the pre-added filters first, no matter how high merit we have.

		if (!get_Forced()) {
			BeginEnumFilters(pGraph, pEF, pBF) {
				if (CComQIPtr<IDirectVobSub>(pBF)) {
					continue;
				}

				CComPtr<IPin> pInPin = GetFirstPin(pBF, PINDIR_INPUT);
				CComPtr<IPin> pOutPin = GetFirstPin(pBF, PINDIR_OUTPUT);

				if (!pInPin) {
					continue;
				}

				CComPtr<IPin> pPin;
				if (pInPin && SUCCEEDED(pInPin->ConnectedTo(&pPin))
						|| pOutPin && SUCCEEDED(pOutPin->ConnectedTo(&pPin))) {
					continue;
				}

				if (pOutPin && GetFilterName(pBF) == L"Overlay Mixer") {
					continue;
				}

				bool fVideoInputPin = false;

				do {
					BITMAPINFOHEADER bih = {sizeof(BITMAPINFOHEADER), 384, 288, 1, 16, '2YUY', 384*288*2, 0, 0, 0, 0};

					CMediaType cmt;
					cmt.majortype = MEDIATYPE_Video;
					cmt.subtype = MEDIASUBTYPE_YUY2;
					cmt.formattype = FORMAT_VideoInfo;
					cmt.pUnk = nullptr;
					cmt.bFixedSizeSamples = TRUE;
					cmt.bTemporalCompression = TRUE;
					cmt.lSampleSize = 384 * 288 * 2;
					VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)cmt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
					memset(vih, 0, sizeof(VIDEOINFOHEADER));
					memcpy(&vih->bmiHeader, &bih, sizeof(bih));
					vih->AvgTimePerFrame = 400000;

					if (SUCCEEDED(pInPin->QueryAccept(&cmt))) {
						fVideoInputPin = true;
						break;
					}

					VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)cmt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER2));
					memset(vih2, 0, sizeof(VIDEOINFOHEADER2));
					memcpy(&vih2->bmiHeader, &bih, sizeof(bih));
					vih2->AvgTimePerFrame = 400000;
					vih2->dwPictAspectRatioX = 384;
					vih2->dwPictAspectRatioY = 288;

					if (SUCCEEDED(pInPin->QueryAccept(&cmt))) {
						fVideoInputPin = true;
						break;
					}
				} while (false);

				if (fVideoInputPin) {
					CComPtr<IBaseFilter> pDVS;
					if (ShouldWeAutoload(pGraph) && SUCCEEDED(pDVS.CoCreateInstance(__uuidof(CDirectVobSubFilter2)))) {
						CComQIPtr<IDirectVobSub2>(pDVS)->put_Forced(true);
						CComQIPtr<IGraphConfig>(pGraph)->AddFilterToCache(pDVS);
					}

					break;
				}
			}
			EndEnumFilters
		}
	} else {
	}

	return __super::JoinFilterGraph(pGraph, pName);
}

HRESULT CDirectVobSubFilter2::CheckInputType(const CMediaType* mtIn)
{
	HRESULT hr = __super::CheckInputType(mtIn);

	if (FAILED(hr) || m_pInput->IsConnected()) {
		return hr;
	}

	if (!ShouldWeAutoload(m_pGraph)) {
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	GetRidOfInternalScriptRenderer();

	return NOERROR;
}

bool CDirectVobSubFilter2::ShouldWeAutoload(IFilterGraph* pGraph)
{
	// all entries must be lowercase!
	TCHAR* blacklistedapps[] = {
		L"wm8eutil.",        // wmp8 encoder's dummy renderer releases the outputted media sample after calling Receive on its input pin (yes, even when dvobsub isn't registered at all)
		L"explorer.",        // as some users reported thumbnail preview loads dvobsub, I've never experienced this yet...
		L"producer.",        // this is real's producer
		L"googledesktop",    // Google Desktop
		L"subtitleworkshop", // Subtitle Workshop
		L"darksouls.",       // Dark Souls (Game)
		L"rometw.",          // Rome Total War (Game)
		L"everquest2.",      // EverQuest II (Game)
		L"yso_win.",         // Ys Origin (Game)
		L"launcher_main.",   // Logitech WebCam Software
		L"webcamdell",       // Dell WebCam Software
		L"data.",            // Dark Souls 1 (Game)
		L"unravel",          // Unravel (Game)
		L"mshta",            // MS Scripting Host
		L"origin.exe",       // Origin
		L"train.exe",        // Train Simulator (Game)
#if WIN64
		L"ridex64.exe",      // Ride (Game)
#else
		L"ride.exe",         // Ride (Game)
#endif
	};

	for (size_t i = 0; i < _countof(blacklistedapps); i++) {
		if (theApp.m_AppName.Find(blacklistedapps[i]) >= 0) {
			return false;
		}
	}

	int level;
	bool m_bExternalLoad, m_bWebLoad, m_bEmbeddedLoad;
	get_LoadSettings(&level, &m_bExternalLoad, &m_bWebLoad, &m_bEmbeddedLoad);

	if (level < 0 || level >= 2) {
		return false;
	}

	bool fRet = false;

	if (level == 1) {
		fRet = m_bExternalLoad = m_bWebLoad = m_bEmbeddedLoad = true;
	}

	// find text stream on known splitters

	if (!fRet && m_bEmbeddedLoad) {
		CComPtr<IBaseFilter> pBF;
		if ((pBF = FindFilter(CLSID_AviSplitter, pGraph))
				|| (pBF = FindFilter(L"{F07E245F-5A1F-4d1e-8BFF-DC31D84A55AB}", pGraph)) // old OggSplitter
				|| (pBF = FindFilter(L"{34293064-02F2-41D5-9D75-CC5967ACA1AB}", pGraph)) // matroska demux
				|| (pBF = FindFilter(L"{0A68C3B5-9164-4a54-AFAF-995B2FF0E0D4}", pGraph)) // matroska source
				|| (pBF = FindFilter(L"{149D2E01-C32E-4939-80F6-C07B81015A7A}", pGraph)) // matroska splitter
				|| (pBF = FindFilter(L"{55DA30FC-F16B-49fc-BAA5-AE59FC65F82D}", pGraph)) // Haali's matroska splitter
				|| (pBF = FindFilter(L"{564FD788-86C9-4444-971E-CC4A243DA150}", pGraph)) // Haali's matroska splitter (AR)
				|| (pBF = FindFilter(L"{171252A0-8820-4AFE-9DF8-5C92B2D66B04}", pGraph)) // LAV Splitter
				|| (pBF = FindFilter(L"{B98D13E7-55DB-4385-A33D-09FD1BA26338}", pGraph)) // LAV Splitter Source
				|| (pBF = FindFilter(L"{E436EBB5-524F-11CE-9F53-0020AF0BA770}", pGraph)) // Solveig matroska splitter
				|| (pBF = FindFilter(L"{52B63861-DC93-11CE-A099-00AA00479A58}", pGraph)) // 3ivx splitter
				|| (pBF = FindFilter(L"{6D3688CE-3E9D-42F4-92CA-8A11119D25CD}", pGraph)) // our ogg source
				|| (pBF = FindFilter(L"{9FF48807-E133-40AA-826F-9B2959E5232D}", pGraph)) // our ogg splitter
				|| (pBF = FindFilter(L"{803E8280-F3CE-4201-982C-8CD8FB512004}", pGraph)) // dsm source
				|| (pBF = FindFilter(L"{0912B4DD-A30A-4568-B590-7179EBB420EC}", pGraph)) // dsm splitter
				|| (pBF = FindFilter(L"{3CCC052E-BDEE-408a-BEA7-90914EF2964B}", pGraph)) // mp4 source
				|| (pBF = FindFilter(L"{61F47056-E400-43d3-AF1E-AB7DFFD4C4AD}", pGraph)) // mp4 splitter
				|| (pBF = FindFilter(L"{1365BE7A-C86A-473C-9A41-C0A6E82C9FA3}", pGraph)) // MPEG source
				|| (pBF = FindFilter(L"{DC257063-045F-4BE2-BD5B-E12279C464F0}", pGraph)) // MPEG splitter
				|| (pBF = FindFilter(L"{D8980E15-E1F6-4916-A10F-D7EB4E9E10B8}", pGraph)) // AV Source
				|| (pBF = FindFilter(L"{529A00DB-0C43-4F5B-8EF2-05004CBE0C6F}", pGraph)) // AV Splitter
				) {
			BeginEnumPins(pBF, pEP, pPin) {
				BeginEnumMediaTypes(pPin, pEM, pmt) {
					if (pmt->majortype == MEDIATYPE_Text || pmt->majortype == MEDIATYPE_Subtitle) {
						fRet = true;
						break;
					}
				}
				EndEnumMediaTypes(pmt)
				if (fRet) {
					break;
				}
			}
			EndEnumFilters
		}
	}

	// find file name

	BeginEnumFilters(pGraph, pEF, pBF) {
		if (CComQIPtr<IFileSourceFilter> pFSF = pBF) {
			LPOLESTR fnw = nullptr;
			if (!pFSF || FAILED(pFSF->GetCurFile(&fnw, nullptr)) || !fnw) {
				continue;
			}
			m_videoFileName = CString(fnw);
			CoTaskMemFree(fnw);
			break;
		}
	}
	EndEnumFilters

	if (!m_videoFileName.IsEmpty() && (m_bExternalLoad || m_bWebLoad) && (m_bWebLoad || !(wcsstr(m_videoFileName, L"http://") || wcsstr(m_videoFileName, L"mms://")))) {
		bool bSubtitlesWereHidden = m_bHideSubtitles;

		if (SUCCEEDED(put_FileName((LPWSTR)(LPCWSTR)m_videoFileName))) {
			fRet = true;
		}

		if (bSubtitlesWereHidden) {
			m_bHideSubtitles = true;
		}

	}

	return(fRet);
}

void CDirectVobSubFilter2::GetRidOfInternalScriptRenderer()
{
	while (CComPtr<IBaseFilter> pBF = FindFilter(L"{48025243-2D39-11CE-875D-00608CB78066}", m_pGraph)) {
		BeginEnumPins(pBF, pEP, pPin) {
			PIN_DIRECTION dir;
			CComPtr<IPin> pPinTo;

			if (SUCCEEDED(pPin->QueryDirection(&dir)) && dir == PINDIR_INPUT
					&& SUCCEEDED(pPin->ConnectedTo(&pPinTo))) {
				m_pGraph->Disconnect(pPinTo);
				m_pGraph->Disconnect(pPin);
				m_pGraph->ConnectDirect(pPinTo, GetPin(2 + m_pTextInputs.size()-1), nullptr);
			}
		}
		EndEnumPins

		if (FAILED(m_pGraph->RemoveFilter(pBF))) {
			break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

bool CDirectVobSubFilter::Open()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CAutoLock cAutolock(&m_csQueueLock);

	m_pSubStreams.RemoveAll();

	m_frd.files.clear();

	std::vector<CString> paths;

	for (int i = 0; i < 10; i++) {
		CString tmp;
		tmp.Format(IDS_RP_PATH, i);
		CString path = theApp.GetProfileString(ResStr(IDS_R_DEFTEXTPATHES), tmp);
		if (!path.IsEmpty()) {
			paths.push_back(path);
		}
	}

	std::vector<CString> ret;
	Subtitle::GetSubFileNames(m_FileName, paths, ret);

	for (const auto& sub_fn : ret) {
		if (std::find(m_frd.files.cbegin(), m_frd.files.cend(), sub_fn) != m_frd.files.cend()) {
			continue;
		}

		CComPtr<ISubStream> pSubStream;

		if (!pSubStream) {
			CAutoPtr<CSupSubFile> pSSF(DNew CSupSubFile(&m_csSubLock));
			if (pSSF && GetFileExt(sub_fn).MakeLower() == L".sup") {
				if (pSSF->Open(sub_fn, L"", m_videoFileName)) {
					pSubStream = pSSF.Detach();
				}
			}
		}

		if (!pSubStream) {
			CAutoPtr<CVobSubFile> pVSF(DNew CVobSubFile(&m_csSubLock));
			if (pVSF && pVSF->Open(sub_fn) && pVSF->GetStreamCount() > 0) {
				pSubStream = pVSF.Detach();
				m_frd.files.push_back(sub_fn.Left(sub_fn.GetLength() - 4) + L".sub");
			}
		}

		if (!pSubStream) {
			CAutoPtr<CRenderedTextSubtitle> pRTS(DNew CRenderedTextSubtitle(&m_csSubLock));
			if (pRTS && pRTS->Open(sub_fn, DEFAULT_CHARSET, L"", m_videoFileName) && pRTS->GetStreamCount() > 0) {
				pSubStream = pRTS.Detach();
				m_frd.files.push_back(sub_fn + L".style");
			}
		}

		if (pSubStream) {
			m_pSubStreams.AddTail(pSubStream);
			m_frd.files.push_back(sub_fn);
		}
	}

	for (const auto& pTextInput : m_pTextInputs) {
		if (pTextInput->IsConnected()) {
			m_pSubStreams.AddTail(pTextInput->GetSubStream());
		}
	}

	if (S_FALSE == put_SelectedLanguage(FindPreferedLanguage())) {
		UpdateSubtitle(false);    // make sure pSubPicProvider of our queue gets updated even if the stream number hasn't changed
	}

	m_frd.RefreshEvent.Set();

	return(m_pSubStreams.GetCount() > 0);
}

void CDirectVobSubFilter::UpdateSubtitle(bool fApplyDefStyle)
{
	CAutoLock cAutolock(&m_csQueueLock);

	if (!m_pSubPicQueue) {
		return;
	}

	InvalidateSubtitle();

	CComPtr<ISubStream> pSubStream;

	if (!m_bHideSubtitles) {
		int i = m_iSelectedLanguage;

		for (POSITION pos = m_pSubStreams.GetHeadPosition(); i >= 0 && pos; pSubStream = nullptr) {
			pSubStream = m_pSubStreams.GetNext(pos);

			if (i < pSubStream->GetStreamCount()) {
				CAutoLock cAutoLock(&m_csSubLock);
				pSubStream->SetStream(i);
				break;
			}

			i -= pSubStream->GetStreamCount();
		}
	}

	SetSubtitle(pSubStream, fApplyDefStyle);
}

void CDirectVobSubFilter::SetSubtitle(ISubStream* pSubStream, bool fApplyDefStyle)
{
	CAutoLock cAutolock(&m_csQueueLock);

	if (pSubStream) {
		CAutoLock cAutolock(&m_csSubLock);

		CLSID clsid;
		pSubStream->GetClassID(&clsid);

		if (clsid == __uuidof(CVobSubFile) || clsid == __uuidof(CVobSubStream)) {
			if (auto pVSS = dynamic_cast<CVobSubSettings*>(pSubStream)) {
				pVSS->SetAlignment(m_bOverridePlacement, m_PlacementXperc, m_PlacementYperc);
				pVSS->m_fOnlyShowForcedSubs = m_bOnlyShowForcedVobSubs;
			}
		} else if (clsid == __uuidof(CRenderedTextSubtitle)) {
			CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)pSubStream;

			if (fApplyDefStyle || pRTS->m_fUsingAutoGeneratedDefaultStyle) {
				STSStyle s = m_defStyle;

				if (m_bOverridePlacement) {
					s.scrAlignment = 2;
					int w = pRTS->m_dstScreenSize.cx;
					int h = pRTS->m_dstScreenSize.cy;
					int mw = w - s.marginRect.left - s.marginRect.right;
					s.marginRect.bottom = h - MulDiv(h, m_PlacementYperc, 100);
					s.marginRect.left = MulDiv(w, m_PlacementXperc, 100) - mw/2;
					s.marginRect.right = w - (s.marginRect.left + mw);
				}

				pRTS->SetDefaultStyle(s);
			}

			pRTS->m_ePARCompensationType = m_ePARCompensationType;
			if (m_CurrentVIH2.dwPictAspectRatioX != 0 && m_CurrentVIH2.dwPictAspectRatioY != 0&& m_CurrentVIH2.bmiHeader.biWidth != 0 && m_CurrentVIH2.bmiHeader.biHeight != 0) {
				pRTS->m_dPARCompensation = ((double)abs(m_CurrentVIH2.bmiHeader.biWidth) / (double)abs(m_CurrentVIH2.bmiHeader.biHeight)) /
										   ((double)abs((long)m_CurrentVIH2.dwPictAspectRatioX) / (double)abs((long)m_CurrentVIH2.dwPictAspectRatioY));

			} else {
				pRTS->m_dPARCompensation = 1.00;
			}

			pRTS->Deinit();
		}

		const CMediaType& mt = m_pOutput->CurrentMediaType();
		DXVA2_ExtendedFormat extFormat; extFormat.value = 0;
		if (mt.formattype == FORMAT_VideoInfo2) {
			extFormat.value = ((VIDEOINFOHEADER2*)mt.Format())->dwControlFlags;
		}
		CString yuvMatrix = extFormat.VideoTransferMatrix == DXVA2_VideoTransferMatrix_BT601 ? L"601" : L"709";
		CString inputRange = extFormat.NominalRange == DXVA2_NominalRange_Normal ? L"PC" : L"TV";
		CString outpuRange(L"PC");

		pSubStream->SetSourceTargetInfo(yuvMatrix, inputRange, outpuRange);
	}

	if (!fApplyDefStyle) {
		int i = 0;

		POSITION pos = m_pSubStreams.GetHeadPosition();
		while (pos) {
			CComPtr<ISubStream> pSubStream2 = m_pSubStreams.GetNext(pos);

			if (pSubStream == pSubStream2) {
				m_iSelectedLanguage = i + pSubStream2->GetStream();
				break;
			}

			i += pSubStream2->GetStreamCount();
		}
	}

	m_nSubtitleId = (DWORD_PTR)pSubStream;

	if (m_pSubPicQueue) {
		m_pSubPicQueue->SetSubPicProvider(CComQIPtr<ISubPicProvider>(pSubStream));
	}
}

void CDirectVobSubFilter::InvalidateSubtitle(REFERENCE_TIME rtInvalidate, DWORD_PTR nSubtitleId)
{
	CAutoLock cAutolock(&m_csQueueLock);

	if (m_pSubPicQueue) {
		if (nSubtitleId == -1 || nSubtitleId == m_nSubtitleId) {
			m_pSubPicQueue->Invalidate(rtInvalidate);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////

void CDirectVobSubFilter::AddSubStream(ISubStream* pSubStream)
{
	CAutoLock cAutoLock(&m_csQueueLock);

	POSITION pos = m_pSubStreams.Find(pSubStream);
	if (!pos) {
		m_pSubStreams.AddTail(pSubStream);
	}

	int len = m_pTextInputs.size();
	for (const auto& pTextInput : m_pTextInputs) {
		if (pTextInput->IsConnected()) {
			len--;
		}
	}

	if (len == 0) {
		HRESULT hr = S_OK;
		m_pTextInputs.push_back(DNew CTextInputPin(this, m_pLock, &m_csSubLock, &hr));
	}
}

void CDirectVobSubFilter::RemoveSubStream(ISubStream* pSubStream)
{
	CAutoLock cAutoLock(&m_csQueueLock);

	POSITION pos = m_pSubStreams.Find(pSubStream);
	if (pos) {
		m_pSubStreams.RemoveAt(pos);
	}
}

void CDirectVobSubFilter::Post_EC_OLE_EVENT(CString str, DWORD_PTR nSubtitleId)
{
	if (nSubtitleId != -1 && nSubtitleId != m_nSubtitleId) {
		return;
	}

	CComQIPtr<IMediaEventSink> pMES = m_pGraph;
	if (!pMES) {
		return;
	}

	CComBSTR bstr1("Text"), bstr2(" ");

	str.Trim();
	if (!str.IsEmpty()) {
		bstr2 = CStringA(str);
	}

	pMES->Notify(EC_OLE_EVENT, (LONG_PTR)bstr1.Detach(), (LONG_PTR)bstr2.Detach());
}

////////////////////////////////////////////////////////////////

void CDirectVobSubFilter::SetupFRD(CStringArray& paths, std::vector<HANDLE>& handles)
{
	CAutoLock cAutolock(&m_csSubLock);

	for (size_t i = 2; i < handles.size(); i++) {
		FindCloseChangeNotification(handles[i]);
	}

	paths.RemoveAll();
	handles.clear();

	handles.push_back(m_frd.EndThreadEvent);
	handles.push_back(m_frd.RefreshEvent);

	m_frd.mtime.resize(m_frd.files.size());

	auto it = m_frd.files.begin();
	for (size_t i = 0; it != m_frd.files.end(); i++) {
		CString fn = *it++;

		CFileStatus status;
		if (CFileGetStatus(fn, status)) {
			m_frd.mtime[i] = status.m_mtime;
		}

		fn.Replace('\\', '/');
		fn = fn.Left(fn.ReverseFind('/')+1);

		bool fFound = false;

		for (INT_PTR j = 0; !fFound && j < paths.GetCount(); j++) {
			if (paths[j] == fn) {
				fFound = true;
			}
		}

		if (!fFound) {
			paths.Add(fn);

			HANDLE h = FindFirstChangeNotificationW(fn, FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);
			if (h != INVALID_HANDLE_VALUE) {
				handles.push_back(h);
			}
		}
	}
}

DWORD CDirectVobSubFilter::ThreadProc()
{
	SetThreadPriority(m_hThread, THREAD_PRIORITY_LOWEST/*THREAD_PRIORITY_BELOW_NORMAL*/);

	CStringArray paths;
	std::vector<HANDLE> handles;

	SetupFRD(paths, handles);

	for (;;) {
		DWORD idx = WaitForMultipleObjects(handles.size(), handles.data(), FALSE, INFINITE);

		if (idx == (WAIT_OBJECT_0 + 0)) { // m_frd.hEndThreadEvent
			break;
		}
		if (idx == (WAIT_OBJECT_0 + 1)) { // m_frd.hRefreshEvent
			SetupFRD(paths, handles);
		} else if (idx >= (WAIT_OBJECT_0 + 2) && idx < (WAIT_OBJECT_0 + handles.size())) {
			bool fLocked = true;
			IsSubtitleReloaderLocked(&fLocked);
			if (fLocked) {
				continue;
			}

			if (FindNextChangeNotification(handles[idx - WAIT_OBJECT_0]) == FALSE) {
				break;
			}

			int j = 0;

			auto it = m_frd.files.begin();
			for (size_t i = 0; it != m_frd.files.end() && j == 0; i++) {
				CString fn = *it++;

				CFileStatus status;
				if (CFileGetStatus(fn, status) && m_frd.mtime[i] != status.m_mtime) {
					for (j = 0; j < 10; j++) {
						FILE* f = nullptr;
						if (_wfopen_s(&f, fn, L"rb+") == 0) {
							fclose(f);
							j = 0;
							break;
						} else {
							Sleep(100);
							j++;
						}
					}
				}
			}

			if (j > 0) {
				SetupFRD(paths, handles);
			} else {
				Sleep(500);

				auto it = m_frd.files.begin();
				for (size_t i = 0; it != m_frd.files.end(); i++) {
					CFileStatus status;
					if (CFileGetStatus(*it++, status)
							&& m_frd.mtime[i] != status.m_mtime) {
						Open();
						SetupFRD(paths, handles);
						break;
					}
				}
			}
		} else {
			break;
		}
	}

	for (size_t i = 2; i < handles.size(); i++) {
		FindCloseChangeNotification(handles[i]);
	}

	return 0;
}

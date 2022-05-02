/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2022 see Authors.txt
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
#include "SubPicAllocatorPresenterImpl.h"
#include "filters/renderer/VideoRenderers/RenderersSettings.h"
#include <Version.h>
#include "XySubPicQueueImpl.h"
#include "XySubPicProvider.h"
#include <dxva2api.h>

CSubPicAllocatorPresenterImpl::CSubPicAllocatorPresenterImpl(HWND hWnd, HRESULT& hr, CString *_pError)
	: CUnknown(L"CSubPicAllocatorPresenterImpl", NULL)
	, m_hWnd(hWnd)
{
	if (!IsWindow(m_hWnd)) {
		hr = E_INVALIDARG;
		if (_pError) {
			*_pError += L"Invalid window handle in ISubPicAllocatorPresenterImpl\n";
		}
		return;
	}
	GetWindowRect(m_hWnd, &m_windowRect);
	hr = S_OK;
}

CSubPicAllocatorPresenterImpl::~CSubPicAllocatorPresenterImpl()
{
}

STDMETHODIMP CSubPicAllocatorPresenterImpl::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(ISubPicAllocatorPresenter3)
		QI(ISubRenderOptions)
		QI(ISubRenderConsumer)
		QI(ISubRenderConsumer2)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

void CSubPicAllocatorPresenterImpl::InitMaxSubtitleTextureSize(const int maxWidth, const CSize& desktopSize)
{
	switch (maxWidth) {
		case 0:
			m_maxSubtitleTextureSize = desktopSize;
			break;
		case 512:
			m_maxSubtitleTextureSize.SetSize(512, 384);
			break;
		case 640:
			m_maxSubtitleTextureSize.SetSize(640, 480);
			break;
		case 800:
			m_maxSubtitleTextureSize.SetSize(800, 600);
			break;
		case 1024:
			m_maxSubtitleTextureSize.SetSize(1024, 768);
			break;
		case 1280:
		default:
			m_maxSubtitleTextureSize.SetSize(1280, 720);
			break;
		case 1600:
			m_maxSubtitleTextureSize.SetSize(1600, 900);
			break;
		case 1920:
			m_maxSubtitleTextureSize.SetSize(1920, 1080);
			break;
		case 2560:
			m_maxSubtitleTextureSize.SetSize(2560, 1440);
			break;
	}
}

HRESULT CSubPicAllocatorPresenterImpl::AlphaBltSubPic(const CRect& windowRect, const CRect& videoRect, int xOffsetInPixels/* = 0*/)
{
	if (m_pSubPicProvider) {
		CComPtr<ISubPic> pSubPic;
		if (m_pSubPicQueue->LookupSubPic(m_rtNow, !IsRendering(), pSubPic)) {

			CRect rcWindow(windowRect);
			CRect rcVideo(videoRect);

			const CRenderersSettings& rs = GetRenderersSettings();

			if (rs.iSubpicStereoMode == SUBPIC_STEREO_SIDEBYSIDE) {
				CRect rcTempWindow(windowRect);
				rcTempWindow.right -= rcTempWindow.Width() / 2;
				CRect rcTempVideo(videoRect);
				rcTempVideo.right -= rcTempVideo.Width() / 2;

				AlphaBlt(rcTempWindow, rcTempVideo, pSubPic, NULL, -xOffsetInPixels, FALSE);

				rcWindow.left += rcWindow.Width() / 2;
				rcVideo.left += rcVideo.Width() / 2;

			} else if (rs.iSubpicStereoMode == SUBPIC_STEREO_TOPANDBOTTOM || rs.iStereo3DTransform == STEREO3D_HalfOverUnder_to_Interlace) {
				CRect rcTempWindow(windowRect);
				rcTempWindow.bottom -= rcTempWindow.Height() / 2;
				CRect rcTempVideo(videoRect);
				rcTempVideo.bottom -= rcTempVideo.Height() / 2;

				AlphaBlt(rcTempWindow, rcTempVideo, pSubPic, NULL, -xOffsetInPixels, FALSE);

				rcWindow.top += rcWindow.Height() / 2;
				rcVideo.top += rcVideo.Height() / 2;

			}

			return AlphaBlt(rcWindow, rcVideo, pSubPic, NULL, xOffsetInPixels, rs.iSubpicStereoMode == SUBPIC_STEREO_NONE && rs.iStereo3DTransform != STEREO3D_HalfOverUnder_to_Interlace);
		}
	}

	return E_FAIL;
}

HRESULT CSubPicAllocatorPresenterImpl::AlphaBlt(const CRect& windowRect, const CRect& videoRect, ISubPic* pSubPic, SubPicDesc* pTarget, int xOffsetInPixels/* = 0*/, const BOOL bUseSpecialCase/* = TRUE*/)
{
	CRect rcSource, rcDest;
	const CRenderersSettings& rs = GetRenderersSettings();
	HRESULT hr = pSubPic->GetSourceAndDest(windowRect, videoRect, rs.iSubpicPosRelative, rs.SubpicShiftPos, rcSource, rcDest, xOffsetInPixels, bUseSpecialCase);
	if (SUCCEEDED(hr)) {
		return pSubPic->AlphaBlt(rcSource, rcDest, pTarget);
	}

	return hr;
}

// ISubPicAllocatorPresenter3

STDMETHODIMP_(SIZE) CSubPicAllocatorPresenterImpl::GetVideoSize()
{
	return m_nativeVideoSize;
}

STDMETHODIMP_(SIZE) CSubPicAllocatorPresenterImpl::GetVideoSizeAR()
{
	SIZE size = m_nativeVideoSize;

	if (m_aspectRatio.cx > 0 && m_aspectRatio.cy > 0) {
		size.cx = MulDiv(size.cy, m_aspectRatio.cx, m_aspectRatio.cy);
	}

	return size;
}

STDMETHODIMP_(void) CSubPicAllocatorPresenterImpl::SetPosition(RECT w, RECT v)
{
	const bool bWindowPosChanged  = !!(m_windowRect != w);
	const bool bWindowSizeChanged = !!(m_windowRect.Size() != CRect(w).Size());
	const bool bVideoRectChanged  = !!(m_videoRect != v);

	m_windowRect = w;
	m_videoRect = v;

	if (bWindowSizeChanged || bVideoRectChanged) {
		if (m_pAllocator) {
			m_pAllocator->SetCurSize(m_windowRect.Size());
			m_pAllocator->SetCurVidRect(m_videoRect);
		}

		if (m_pSubPicQueue) {
			m_pSubPicQueue->Invalidate();
		}
	}

	if (bWindowPosChanged || bVideoRectChanged || m_bOtherTransform) {
		Paint(false);
		m_bOtherTransform = false;
	}
}

STDMETHODIMP_(void) CSubPicAllocatorPresenterImpl::SetTime(REFERENCE_TIME rtNow)
{
	m_rtNow = rtNow - m_rtSubtitleDelay;

	if (m_pSubPicQueue) {
		m_pSubPicQueue->SetTime(m_rtNow);
	}
}

STDMETHODIMP_(void) CSubPicAllocatorPresenterImpl::SetSubtitleDelay(int delay_ms)
{
	m_rtSubtitleDelay = delay_ms * 10000i64;
}

STDMETHODIMP_(int) CSubPicAllocatorPresenterImpl::GetSubtitleDelay()
{
	return (int)(m_rtSubtitleDelay / 10000);
}

STDMETHODIMP_(double) CSubPicAllocatorPresenterImpl::GetFPS()
{
	return m_fps;
}

STDMETHODIMP_(void) CSubPicAllocatorPresenterImpl::SetSubPicProvider(ISubPicProvider* pSubPicProvider)
{
	CAutoLock cAutoLock(&m_csSubPicProvider);

	m_pSubPicProvider = pSubPicProvider;

	// Reset the default state to be sure text subtitles will be displayed right.
	// Subtitles with specific requirements will adapt those values later.
	if (m_pAllocator) {
		m_pAllocator->SetMaxTextureSize(m_maxSubtitleTextureSize);
		m_pAllocator->SetCurSize(m_windowRect.Size());
		m_pAllocator->SetCurVidRect(m_videoRect);
		m_pAllocator->Reset();
	}

	if (m_pSubPicQueue) {
		m_pSubPicQueue->SetSubPicProvider(pSubPicProvider);
	}

	Paint(false);
}

STDMETHODIMP_(void) CSubPicAllocatorPresenterImpl::Invalidate(REFERENCE_TIME rtInvalidate)
{
	if (m_pSubPicQueue) {
		m_pSubPicQueue->Invalidate(rtInvalidate);
	}
}

// ISubRenderOptions

STDMETHODIMP CSubPicAllocatorPresenterImpl::GetBool(LPCSTR field, bool* value)
{
	CheckPointer(value, E_POINTER);
	return E_INVALIDARG;
}

STDMETHODIMP CSubPicAllocatorPresenterImpl::GetInt(LPCSTR field, int* value)
{
	CheckPointer(value, E_POINTER);
	return E_INVALIDARG;
}

STDMETHODIMP CSubPicAllocatorPresenterImpl::GetSize(LPCSTR field, SIZE* value)
{
	CheckPointer(value, E_POINTER);
	if (!strcmp(field, "originalVideoSize")) {
		*value = GetVideoSize();
		return S_OK;
	} else if (!strcmp(field, "arAdjustedVideoSize")) {
		*value = GetVideoSizeAR();
		return S_OK;
	}

	return E_INVALIDARG;
}

STDMETHODIMP CSubPicAllocatorPresenterImpl::GetRect(LPCSTR field, RECT* value)
{
	CheckPointer(value, E_POINTER);
	if (!strcmp(field, "videoOutputRect") || !strcmp(field, "subtitleTargetRect")) {
		if (m_videoRect.IsRectEmpty()) {
			*value = m_windowRect;
		} else {
			value->left = 0;
			value->top = 0;
			value->right = m_videoRect.Width();
			value->bottom = m_videoRect.Height();
		}
		return S_OK;
	}

	return E_INVALIDARG;
}

STDMETHODIMP CSubPicAllocatorPresenterImpl::GetUlonglong(LPCSTR field, ULONGLONG* value)
{
	CheckPointer(value, E_POINTER);
	if (!strcmp(field, "frameRate")) {
		*value = (REFERENCE_TIME)(10000000.0 / m_fps);
		return S_OK;
	}

	return E_INVALIDARG;
}

STDMETHODIMP CSubPicAllocatorPresenterImpl::GetDouble(LPCSTR field, double* value)
{
	CheckPointer(value, E_POINTER);
	if (!strcmp(field, "refreshRate")) {
		*value = 1000.0 / m_refreshRate;
		return S_OK;
	}

	return E_INVALIDARG;
}

STDMETHODIMP CSubPicAllocatorPresenterImpl::GetString(LPCSTR field, LPWSTR* value, int* chars)
{
	CheckPointer(value, E_POINTER);
	CheckPointer(chars, E_POINTER);
	CString ret;

	if (!strcmp(field, "name")) {
		ret = L"MPC-BE";
	} else if (!strcmp(field, "version")) {
		ret = MPC_VERSION_FULL_WSTR;
	} else if (!strcmp(field, "yuvMatrix")) {
		ret = L"TV.709";

		if (m_inputMediaType.IsValid() && m_inputMediaType.formattype == FORMAT_VideoInfo2) {
			const VIDEOINFOHEADER2* pVIH2 = (VIDEOINFOHEADER2*)m_inputMediaType.pbFormat;

			if (pVIH2->dwControlFlags & AMCONTROL_COLORINFO_PRESENT) {
				const DXVA2_ExtendedFormat& flags = (DXVA2_ExtendedFormat&)pVIH2->dwControlFlags;

				ret = (flags.NominalRange == DXVA2_NominalRange_Normal) ? L"PC." : L"TV.";

				switch (flags.VideoTransferMatrix) {
					case DXVA2_VideoTransferMatrix_BT601:
						ret.Append(L"601");
						break;
					default:
					case DXVA2_VideoTransferMatrix_BT709:
						ret.Append(L"709");
						break;
					case DXVA2_VideoTransferMatrix_SMPTE240M:
						ret.Append(L"240M");
						break;
				}
			}
		}
	}

	if (!ret.IsEmpty()) {
		const int len = ret.GetLength();
		const size_t sz = (len + 1) * sizeof(WCHAR);
		LPWSTR buf = (LPWSTR)LocalAlloc(LPTR, sz);

		if (!buf) {
			return E_OUTOFMEMORY;
		}

		wcscpy_s(buf, len + 1, ret);
		*chars = len;
		*value = buf;

		return S_OK;
	}

	return E_INVALIDARG;
}

STDMETHODIMP CSubPicAllocatorPresenterImpl::GetBin(LPCSTR field, LPVOID* value, int* size)
{
	CheckPointer(value, E_POINTER);
	CheckPointer(size, E_POINTER);
	return E_INVALIDARG;
}

STDMETHODIMP CSubPicAllocatorPresenterImpl::SetBool(LPCSTR field, bool value)
{
	return E_INVALIDARG;
}

STDMETHODIMP CSubPicAllocatorPresenterImpl::SetInt(LPCSTR field, int value)
{
	return E_INVALIDARG;
}

STDMETHODIMP CSubPicAllocatorPresenterImpl::SetSize(LPCSTR field, SIZE value)
{
	return E_INVALIDARG;
}

STDMETHODIMP CSubPicAllocatorPresenterImpl::SetRect(LPCSTR field, RECT value)
{
	return E_INVALIDARG;
}

STDMETHODIMP CSubPicAllocatorPresenterImpl::SetUlonglong(LPCSTR field, ULONGLONG value)
{
	return E_INVALIDARG;
}

STDMETHODIMP CSubPicAllocatorPresenterImpl::SetDouble(LPCSTR field, double value)
{
	return E_INVALIDARG;
}

STDMETHODIMP CSubPicAllocatorPresenterImpl::SetString(LPCSTR field, LPWSTR value, int chars)
{
	return E_INVALIDARG;
}

STDMETHODIMP CSubPicAllocatorPresenterImpl::SetBin(LPCSTR field, LPVOID value, int size)
{
	return E_INVALIDARG;
}

// ISubRenderConsumer

STDMETHODIMP CSubPicAllocatorPresenterImpl::GetMerit(ULONG* plMerit)
{
	CheckPointer(plMerit, E_POINTER);
	*plMerit = 4 << 16;
	return S_OK;
}

STDMETHODIMP CSubPicAllocatorPresenterImpl::Connect(ISubRenderProvider* subtitleRenderer)
{
	HRESULT hr = E_FAIL;

	if (m_pSubPicProvider) {
		return hr;
	}

	hr = subtitleRenderer->SetBool("combineBitmaps", true);
	if (FAILED(hr)) {
		return hr;
	}

	if (CComQIPtr<ISubRenderConsumer> pSubConsumer = m_pSubPicQueue.p) {
		hr = pSubConsumer->Connect(subtitleRenderer);
	} else {
		CComPtr<ISubPicProvider> pSubPicProvider = (ISubPicProvider*)DNew CXySubPicProvider(subtitleRenderer);
		CComPtr<ISubPicQueue> pSubPicQueue = (ISubPicQueue*)DNew CXySubPicQueueNoThread(m_pAllocator, &hr);

		if (SUCCEEDED(hr)) {
			CAutoLock cAutoLock(this);
			pSubPicQueue->SetSubPicProvider(pSubPicProvider);
			m_pSubPicProvider = pSubPicProvider;
			m_pSubPicQueue = pSubPicQueue;
		}
	}

	return hr;
}

STDMETHODIMP CSubPicAllocatorPresenterImpl::Disconnect()
{
	m_pSubPicProvider = NULL;
	return m_pSubPicQueue->SetSubPicProvider(m_pSubPicProvider);
}

STDMETHODIMP CSubPicAllocatorPresenterImpl::DeliverFrame(REFERENCE_TIME start, REFERENCE_TIME stop, LPVOID context, ISubRenderFrame* subtitleFrame)
{
	HRESULT hr = E_FAIL;

	if (CComQIPtr<IXyCompatProvider> pXyProvider = m_pSubPicProvider.p) {
		hr = pXyProvider->DeliverFrame(start + m_rtSubtitleDelay, stop + m_rtSubtitleDelay, context, subtitleFrame);
	}

	return hr;
}

// ISubRenderConsumer2

STDMETHODIMP CSubPicAllocatorPresenterImpl::Clear(REFERENCE_TIME clearNewerThan /* = 0 */)
{
	return m_pSubPicQueue->Invalidate(clearNewerThan);
}

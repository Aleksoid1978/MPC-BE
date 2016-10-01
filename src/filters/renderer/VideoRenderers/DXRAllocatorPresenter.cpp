/*
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
#include "RenderersSettings.h"
#include "DXRAllocatorPresenter.h"
#include "../../../SubPic/DX9SubPic.h"
#include "../../../SubPic/SubPicQueueImpl.h"
#include <moreuuids.h>

using namespace DSObjects;

//
// CDXRAllocatorPresenter
//

CDXRAllocatorPresenter::CDXRAllocatorPresenter(HWND hWnd, HRESULT& hr, CString &_Error)
	: CSubPicAllocatorPresenterImpl(hWnd, hr, &_Error)
	, m_ScreenSize(0, 0)
{
	if (FAILED(hr)) {
		_Error += L"ISubPicAllocatorPresenterImpl failed\n";
		return;
	}

	hr = S_OK;
}

CDXRAllocatorPresenter::~CDXRAllocatorPresenter()
{
	if (m_pSRCB) {
		// nasty, but we have to let it know about our death somehow
		((CSubRenderCallback*)(ISubRenderCallback*)m_pSRCB)->SetDXRAP(NULL);
	}

	// the order is important here
	m_pSubPicQueue = NULL;
	m_pAllocator = NULL;
	m_pDXR = NULL;
}

STDMETHODIMP CDXRAllocatorPresenter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	/*
		if (riid == __uuidof(IVideoWindow))
			return GetInterface((IVideoWindow*)this, ppv);
		if (riid == __uuidof(IBasicVideo))
			return GetInterface((IBasicVideo*)this, ppv);
		if (riid == __uuidof(IBasicVideo2))
			return GetInterface((IBasicVideo2*)this, ppv);
	*/
	/*
		if (riid == __uuidof(IVMRWindowlessControl))
			return GetInterface((IVMRWindowlessControl*)this, ppv);
	*/

	if (riid != IID_IUnknown && m_pDXR) {
		if (SUCCEEDED(m_pDXR->QueryInterface(riid, ppv))) {
			return S_OK;
		}
	}

	return __super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CDXRAllocatorPresenter::SetDevice(IDirect3DDevice9* pD3DDev)
{
	CheckPointer(pD3DDev, E_POINTER);
	CRenderersSettings& rs = GetRenderersSettings();

	InitMaxSubtitleTextureSize(rs.iSubpicMaxTexWidth, m_ScreenSize);

	if (m_pAllocator) {
		m_pAllocator->ChangeDevice(pD3DDev);
	} else {
		m_pAllocator = DNew CDX9SubPicAllocator(pD3DDev, m_maxSubtitleTextureSize, true);
		if (!m_pAllocator) {
			return E_FAIL;
		}
	}

	HRESULT hr = S_OK;
	if (!m_pSubPicQueue) {
		CAutoLock cAutoLock(this);
		m_pSubPicQueue = rs.nSubpicCount > 0
						 ? (ISubPicQueue*)DNew CSubPicQueue(rs.nSubpicCount, !rs.bSubpicAnimationWhenBuffering, rs.bSubpicAllowDrop, m_pAllocator, &hr)
						 : (ISubPicQueue*)DNew CSubPicQueueNoThread(!rs.bSubpicAnimationWhenBuffering, m_pAllocator, &hr);
	} else {
		m_pSubPicQueue->Invalidate();
	}
	if (!m_pSubPicQueue || FAILED(hr)) {
		return E_FAIL;
	}

	if (m_pSubPicProvider) {
		m_pSubPicQueue->SetSubPicProvider(m_pSubPicProvider);
	}

	return S_OK;
}

HRESULT CDXRAllocatorPresenter::Render(
	REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, REFERENCE_TIME atpf,
	int left, int top, int right, int bottom, int width, int height)
{
	CRect wndRect(0, 0, width, height);
	CRect videoRect(left, top, right, bottom);
	__super::SetPosition(wndRect, videoRect); // needed? should be already set by the player
	SetTime(rtStart);
	if (atpf > 0 && m_pSubPicQueue) {
		m_pSubPicQueue->SetFPS(10000000.0 / atpf);
	}

	AlphaBltSubPic(wndRect, videoRect);

	return S_OK;
}

// ISubPicAllocatorPresenter3

STDMETHODIMP CDXRAllocatorPresenter::CreateRenderer(IUnknown** ppRenderer)
{
	CheckPointer(ppRenderer, E_POINTER);

	if (m_pDXR) {
		return E_UNEXPECTED;
	}
	m_pDXR.CoCreateInstance(CLSID_DXR, GetOwner());
	if (!m_pDXR) {
		return E_FAIL;
	}

	CComQIPtr<ISubRender> pSR = m_pDXR;
	if (!pSR) {
		m_pDXR = NULL;
		return E_FAIL;
	}

	m_pSRCB = DNew CSubRenderCallback(this);
	if (FAILED(pSR->SetCallback(m_pSRCB))) {
		m_pDXR = NULL;
		return E_FAIL;
	}

	(*ppRenderer = (IUnknown*)(INonDelegatingUnknown*)(this))->AddRef();

	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	if (GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi)) {
		m_ScreenSize.SetSize(mi.rcMonitor.right-mi.rcMonitor.left, mi.rcMonitor.bottom-mi.rcMonitor.top);
	}

	return S_OK;
}

STDMETHODIMP_(void) CDXRAllocatorPresenter::SetPosition(RECT w, RECT v)
{
	if (CComQIPtr<IBasicVideo> pBV = m_pDXR) {
		pBV->SetDefaultSourcePosition();
		pBV->SetDestinationPosition(v.left, v.top, v.right - v.left, v.bottom - v.top);
	}

	if (CComQIPtr<IVideoWindow> pVW = m_pDXR) {
		pVW->SetWindowPosition(w.left, w.top, w.right - w.left, w.bottom - w.top);
	}
}

STDMETHODIMP_(SIZE) CDXRAllocatorPresenter::GetVideoSize()
{
	SIZE size = {0, 0};
	if (CComQIPtr<IBasicVideo> pBV = m_pDXR) {
		// Final size of the video, after all scaling and cropping operations
		// This is also aspect ratio adjusted
		pBV->GetVideoSize(&size.cx, &size.cy);
	}
	return size;
}

STDMETHODIMP_(SIZE) CDXRAllocatorPresenter::GetVideoSizeAR()
{
	SIZE size = {0, 0};
	if (CComQIPtr<IBasicVideo2> pBV2 = m_pDXR) {
		pBV2->GetPreferredAspectRatio(&size.cx, &size.cy);
	}
	return size;
}

STDMETHODIMP CDXRAllocatorPresenter::GetDIB(BYTE* lpDib, DWORD* size)
{
	HRESULT hr = E_NOTIMPL;
	if (CComQIPtr<IBasicVideo> pBV = m_pDXR) {
		hr = pBV->GetCurrentImage((long*)size, (long*)lpDib);
	}
	return hr;
}

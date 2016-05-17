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
#include "madVRAllocatorPresenter.h"
#include "../../../SubPic/DX9SubPic.h"
#include "../../../SubPic/SubPicQueueImpl.h"
#include "RenderersSettings.h"
#include <moreuuids.h>
#include <mvrInterfaces.h>

using namespace DSObjects;

extern bool g_bExternalSubtitleTime;
//
// CmadVRAllocatorPresenter
//

CmadVRAllocatorPresenter::CmadVRAllocatorPresenter(HWND hWnd, HRESULT& hr, CString &_Error)
	: CSubPicAllocatorPresenterImpl(hWnd, hr, &_Error)
{
	if (FAILED(hr)) {
		_Error += L"ISubPicAllocatorPresenterImpl failed\n";
		return;
	}

	hr = S_OK;
}

CmadVRAllocatorPresenter::~CmadVRAllocatorPresenter()
{
	// the order is important here
	m_pSubPicQueue = NULL;
	m_pAllocator = NULL;
	m_pMVR = NULL;
}

STDMETHODIMP CmadVRAllocatorPresenter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	if (riid != IID_IUnknown && m_pMVR) {
		if (SUCCEEDED(m_pMVR->QueryInterface(riid, ppv))) {
			return S_OK;
		}
	}

	return QI(ISubRenderCallback)
		   QI(ISubRenderCallback2)
		   QI(ISubRenderCallback3)
		   QI(ISubRenderCallback4)
		   __super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CmadVRAllocatorPresenter::SetDevice(IDirect3DDevice9* pD3DDev)
{
	if (!pD3DDev) {
		// release all resources
		m_pSubPicQueue = NULL;
		m_pAllocator = NULL;
		return S_OK;
	}

	CSize screenSize;
	MONITORINFO mi = { sizeof(MONITORINFO) };
	if (GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi)) {
		screenSize.SetSize(mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top);
	}
	InitMaxSubtitleTextureSize(GetRenderersSettings().iSubpicMaxTexWidth, screenSize);

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
		m_pSubPicQueue = GetRenderersSettings().nSubpicCount > 0
						 ? (ISubPicQueue*)DNew CSubPicQueue(GetRenderersSettings().nSubpicCount, !GetRenderersSettings().bSubpicAnimationWhenBuffering, GetRenderersSettings().bSubpicAllowDrop, m_pAllocator, &hr)
						 : (ISubPicQueue*)DNew CSubPicQueueNoThread(!GetRenderersSettings().bSubpicAnimationWhenBuffering, m_pAllocator, &hr);
	} else {
		m_pSubPicQueue->Invalidate();
	}

	if (SUCCEEDED(hr) && m_pSubPicQueue && m_pSubPicProvider) {
		m_pSubPicQueue->SetSubPicProvider(m_pSubPicProvider);
	}

	return hr;
}

// ISubRenderCallback4

HRESULT CmadVRAllocatorPresenter::RenderEx3(REFERENCE_TIME rtStart,
											REFERENCE_TIME rtStop,
											REFERENCE_TIME atpf,
											RECT croppedVideoRect,
											RECT originalVideoRect,
											RECT viewportRect,
											const double videoStretchFactor,
											int xOffsetInPixels, DWORD flags)
{
	CheckPointer(m_pSubPicQueue, E_UNEXPECTED);

	__super::SetPosition(viewportRect, croppedVideoRect);
	if (!g_bExternalSubtitleTime) {
		SetTime(rtStart);
	}
	if (atpf > 0) {
		m_fps = 10000000.0 / atpf;
		m_pSubPicQueue->SetFPS(m_fps);
	}
	
	AlphaBltSubPic(viewportRect, croppedVideoRect, NULL, xOffsetInPixels);
	return S_OK;
}

// ISubPicAllocatorPresenter

STDMETHODIMP CmadVRAllocatorPresenter::CreateRenderer(IUnknown** ppRenderer)
{
	CheckPointer(ppRenderer, E_POINTER);
	ASSERT(!m_pMVR);

	HRESULT hr = S_FALSE;

	CHECK_HR(m_pMVR.CoCreateInstance(CLSID_madVR, GetOwner()));

	if (CComQIPtr<ISubRender> pSR = m_pMVR) {
		VERIFY(SUCCEEDED(pSR->SetCallback(this)));
	}

	(*ppRenderer = (IUnknown*)(INonDelegatingUnknown*)(this))->AddRef();

	return S_OK;
}

STDMETHODIMP_(void) CmadVRAllocatorPresenter::SetPosition(RECT w, RECT v)
{
	if (CComQIPtr<IBasicVideo> pBV = m_pMVR) {
		pBV->SetDefaultSourcePosition();
		pBV->SetDestinationPosition(v.left, v.top, v.right - v.left, v.bottom - v.top);
	}

	if (CComQIPtr<IVideoWindow> pVW = m_pMVR) {
		pVW->SetWindowPosition(w.left, w.top, w.right - w.left, w.bottom - w.top);
	}
}

STDMETHODIMP_(SIZE) CmadVRAllocatorPresenter::GetVideoSize()
{
	SIZE size = {0, 0};
	if (CComQIPtr<IBasicVideo> pBV = m_pMVR) {
		// Final size of the video, after all scaling and cropping operations
		// This is also aspect ratio adjusted
		pBV->GetVideoSize(&size.cx, &size.cy);
	}
	return size;
}

STDMETHODIMP_(SIZE) CmadVRAllocatorPresenter::GetVideoSizeAR()
{
	SIZE size = {0, 0};
	if (CComQIPtr<IBasicVideo2> pBV2 = m_pMVR) {
		pBV2->GetPreferredAspectRatio(&size.cx, &size.cy);
	}
	return size;
}

STDMETHODIMP_(bool) CmadVRAllocatorPresenter::Paint(bool /*bAll*/)
{
	if (CComQIPtr<IMadVRCommand> pMVRC = m_pMVR) {
		return SUCCEEDED(pMVRC->SendCommand("redraw"));
	}
	return false;
}

STDMETHODIMP CmadVRAllocatorPresenter::GetDIB(BYTE* lpDib, DWORD* size)
{
	HRESULT hr = E_NOTIMPL;
	if (CComQIPtr<IBasicVideo> pBV = m_pMVR) {
		hr = pBV->GetCurrentImage((long*)size, (long*)lpDib);
	}
	return hr;
}

STDMETHODIMP CmadVRAllocatorPresenter::SetPixelShader2(LPCSTR pSrcData, LPCSTR pTarget, bool bScreenSpace)
{
	HRESULT hr = E_NOTIMPL;

	if (CComQIPtr<IMadVRExternalPixelShaders> pMVREPS = m_pMVR) {
		if (!pSrcData && !pTarget) {
			hr = pMVREPS->ClearPixelShaders(bScreenSpace ? ShaderStage_PostScale : ShaderStage_PreScale);
		} else {
			hr = pMVREPS->AddPixelShader(pSrcData, pTarget, bScreenSpace ? ShaderStage_PostScale : ShaderStage_PreScale, NULL);
		}
	}
	return hr;
}

// ISubPicAllocatorPresenter2

STDMETHODIMP_(bool) CmadVRAllocatorPresenter::IsRendering()
{
	if (CComQIPtr<IMadVRInfo> pMVRI = m_pMVR) {
		int playbackState;
		if (SUCCEEDED(pMVRI->GetInt("playbackState", &playbackState))) {
			return playbackState == State_Running;
		}
	}
	return false;
}

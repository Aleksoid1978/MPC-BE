/*
 * (C) 2019-2020 see Authors.txt
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
#include "MPCVRAllocatorPresenter.h"
#include "../../../SubPic/DX9SubPic.h"
#include "../../../SubPic/SubPicQueueImpl.h"
#include "RenderersSettings.h"
#include "Variables.h"
#include <moreuuids.h>
#include "IPinHook.h"
#include <FilterInterfaces.h>

using namespace DSObjects;

//
// CMPCVRAllocatorPresenter
//

CMPCVRAllocatorPresenter::CMPCVRAllocatorPresenter(HWND hWnd, HRESULT& hr, CString &_Error)
	: CSubPicAllocatorPresenterImpl(hWnd, hr, &_Error)
{
	if (FAILED(hr)) {
		_Error += L"ISubPicAllocatorPresenterImpl failed\n";
		return;
	}

	hr = S_OK;
}

CMPCVRAllocatorPresenter::~CMPCVRAllocatorPresenter()
{
	// the order is important here
	m_pSubPicQueue = nullptr;
	m_pAllocator = nullptr;
	m_pMPCVR = nullptr;
}

STDMETHODIMP CMPCVRAllocatorPresenter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	if (riid != IID_IUnknown && m_pMPCVR) {
		if (SUCCEEDED(m_pMPCVR->QueryInterface(riid, ppv))) {
			return S_OK;
		}
	}

	return QI(ISubRenderCallback)
		   QI(ISubRenderCallback2)
		   QI(ISubRenderCallback3)
		   QI(ISubRenderCallback4)
		   __super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CMPCVRAllocatorPresenter::SetDevice(IDirect3DDevice9* pD3DDev)
{
	if (!pD3DDev) {
		// release all resources
		m_pSubPicQueue = nullptr;
		m_pAllocator = nullptr;
		return S_OK;
	}

	CRenderersSettings& rs = GetRenderersSettings();

	CSize screenSize;
	MONITORINFO mi = { sizeof(MONITORINFO) };
	if (GetMonitorInfoW(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi)) {
		screenSize.SetSize(mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top);
	}
	InitMaxSubtitleTextureSize(rs.iSubpicMaxTexWidth, screenSize);

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

	if (SUCCEEDED(hr) && m_pSubPicQueue && m_pSubPicProvider) {
		m_pSubPicQueue->SetSubPicProvider(m_pSubPicProvider);
	}

	return hr;
}

// ISubRenderCallback4

HRESULT CMPCVRAllocatorPresenter::RenderEx3(REFERENCE_TIME rtStart,
											REFERENCE_TIME rtStop,
											REFERENCE_TIME atpf,
											RECT croppedVideoRect,
											RECT originalVideoRect,
											RECT viewportRect,
											const double videoStretchFactor,
											int xOffsetInPixels, DWORD flags)
{
	CheckPointer(m_pSubPicQueue, E_UNEXPECTED);

	if (!g_bExternalSubtitleTime) {
		if (g_bExternalSubtitle && g_dRate != 0.0) {
			const REFERENCE_TIME sampleTime = rtStart - g_tSegmentStart;
			SetTime(g_tSegmentStart + sampleTime * g_dRate);
		} else {
			SetTime(rtStart);
		}
	}
	if (atpf > 0) {
		m_fps = 10000000.0 / atpf;
		m_pSubPicQueue->SetFPS(m_fps);
	}

	return AlphaBltSubPic(viewportRect, croppedVideoRect, xOffsetInPixels);
}

// ISubPicAllocatorPresenter3

STDMETHODIMP CMPCVRAllocatorPresenter::CreateRenderer(IUnknown** ppRenderer)
{
	CheckPointer(ppRenderer, E_POINTER);
	ASSERT(!m_pMPCVR);

	HRESULT hr = S_FALSE;

	CHECK_HR(m_pMPCVR.CoCreateInstance(CLSID_MPCVR, GetOwner()));

	if (CComQIPtr<ISubRender> pSR = m_pMPCVR) {
		VERIFY(SUCCEEDED(pSR->SetCallback(this)));
	}

	(*ppRenderer = (IUnknown*)(INonDelegatingUnknown*)(this))->AddRef();

	CComQIPtr<IBaseFilter> pBF = m_pMPCVR;
	HookNewSegmentAndReceive(GetFirstPin(pBF), true);

	return S_OK;
}

STDMETHODIMP_(void) CMPCVRAllocatorPresenter::SetPosition(RECT w, RECT v)
{
	if (CComQIPtr<IBasicVideo> pBV = m_pMPCVR) {
		pBV->SetDefaultSourcePosition();
		pBV->SetDestinationPosition(v.left, v.top, v.right - v.left, v.bottom - v.top);
	}

	if (CComQIPtr<IVideoWindow> pVW = m_pMPCVR) {
		pVW->SetWindowPosition(w.left, w.top, w.right - w.left, w.bottom - w.top);
	}

	__super::SetPosition(w, v);
}

STDMETHODIMP CMPCVRAllocatorPresenter::SetRotation(int rotation)
{
	HRESULT hr = E_NOTIMPL;
	if (CComQIPtr<IExFilterConfig> pIExFilterConfig = m_pMPCVR) {
		hr = pIExFilterConfig->SetInt("rotation", rotation);
	}
	return hr;
}

STDMETHODIMP_(int) CMPCVRAllocatorPresenter::GetRotation()
{
	if (CComQIPtr<IExFilterConfig> pIExFilterConfig = m_pMPCVR) {
		int rotation = 0;
		if (SUCCEEDED(pIExFilterConfig->GetInt("rotation", &rotation))) {
			return rotation;
		}
	}
	return 0;
}

STDMETHODIMP_(SIZE) CMPCVRAllocatorPresenter::GetVideoSize()
{
	SIZE size = {0, 0};
	if (CComQIPtr<IBasicVideo> pBV = m_pMPCVR) {
		// Final size of the video, after all scaling and cropping operations
		// This is also aspect ratio adjusted
		pBV->GetVideoSize(&size.cx, &size.cy);
	}
	return size;
}

STDMETHODIMP_(SIZE) CMPCVRAllocatorPresenter::GetVideoSizeAR()
{
	SIZE size = {0, 0};
	if (CComQIPtr<IBasicVideo2> pBV2 = m_pMPCVR) {
		pBV2->GetPreferredAspectRatio(&size.cx, &size.cy);
	}
	return size;
}

STDMETHODIMP_(bool) CMPCVRAllocatorPresenter::Paint(bool /*bAll*/)
{
	if (CComQIPtr<IExFilterConfig> pIExFilterConfig = m_pMPCVR) {
		return SUCCEEDED(pIExFilterConfig->SetBool("cmd_redraw", true));
	}
	return false;
}

STDMETHODIMP CMPCVRAllocatorPresenter::GetDIB(BYTE* lpDib, DWORD* size)
{
	HRESULT hr = E_NOTIMPL;
	if (CComQIPtr<IBasicVideo> pBV = m_pMPCVR) {
		hr = pBV->GetCurrentImage((long*)size, (long*)lpDib);
	}
	return hr;
}

STDMETHODIMP CMPCVRAllocatorPresenter::ClearPixelShaders(int target)
{
	HRESULT hr = E_NOTIMPL;

	if (TARGET_SCREEN == target) {
		// experimental
		if (CComQIPtr<IExFilterConfig> pIExFilterConfig = m_pMPCVR) {
			hr = pIExFilterConfig->SetBool("cmd_clearPostScaleShaders", true);
		}
	}
	return hr;
}

STDMETHODIMP CMPCVRAllocatorPresenter::AddPixelShader(int target, LPCWSTR name, LPCSTR profile, LPCSTR sourceCode)
{
	HRESULT hr = E_NOTIMPL;

	int len = strlen(sourceCode);

	if (len && TARGET_SCREEN == target) {
		// experimental
		if (CComQIPtr<IExFilterConfig> pIExFilterConfig = m_pMPCVR) {
			int rtype = 0;
			hr = pIExFilterConfig->GetInt("renderType", &rtype);
			if (S_OK == hr && rtype == 9) { // Direct3D 9
				hr = pIExFilterConfig->SetBin("cmd_addPostScaleShader", (LPVOID)sourceCode, len + 1);
			}
		}
	}
	return hr;
}

STDMETHODIMP_(bool) CMPCVRAllocatorPresenter::IsRendering()
{
	if (CComQIPtr<IExFilterConfig> pIExFilterConfig = m_pMPCVR) {
		int playbackState;
		if (SUCCEEDED(pIExFilterConfig->GetInt("playbackState", &playbackState))) {
			return playbackState == State_Running;
		}
	}

	return false;
}

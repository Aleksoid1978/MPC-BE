/*
 * (C) 2019-2023 see Authors.txt
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
#include "SubPic/DX9SubPic.h"
#include "SubPic/DX11SubPic.h"
#include "SubPic/SubPicQueueImpl.h"
#include "Variables.h"
#include <clsids.h>
#include "IPinHook.h"
#include <FilterInterfaces.h>

using namespace DSObjects;

//
// CMPCVRAllocatorPresenter
//

CMPCVRAllocatorPresenter::CMPCVRAllocatorPresenter(HWND hWnd, HRESULT& hr, CString &_Error)
	: CAllocatorPresenterImpl(hWnd, hr, &_Error)
{
	if (FAILED(hr)) {
		_Error += L"IAllocatorPresenterImpl failed\n";
		return;
	}

	hr = S_OK;
}

CMPCVRAllocatorPresenter::~CMPCVRAllocatorPresenter()
{
	// the order is important here
	m_pSubPicQueue.Release();
	m_pSubPicAllocator.Release();
	m_pMPCVR.Release();
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
		   QI(ISubRender11Callback)
		   __super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CMPCVRAllocatorPresenter::SetDevice(IDirect3DDevice9* pD3DDev)
{
	if (!pD3DDev) {
		// release all resources
		m_pSubPicQueue.Release();
		m_pSubPicAllocator.Release();
		return S_OK;
	}

	CSize screenSize;
	MONITORINFO mi = { sizeof(MONITORINFO) };
	if (GetMonitorInfoW(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi)) {
		screenSize.SetSize(mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top);
	}
	InitMaxSubtitleTextureSize(m_SubpicSets.iMaxTexWidth, screenSize);

	if (m_pSubPicAllocator) {
		m_pSubPicAllocator->ChangeDevice(pD3DDev);
	} else {
		m_pSubPicAllocator = DNew CDX9SubPicAllocator(pD3DDev, m_maxSubtitleTextureSize, true);
		if (!m_pSubPicAllocator) {
			return E_FAIL;
		}
	}

	HRESULT hr = S_OK;
	if (!m_pSubPicQueue) {
		CAutoLock cAutoLock(this);
		m_pSubPicQueue = m_SubpicSets.nCount > 0
						 ? (ISubPicQueue*)DNew CSubPicQueue(m_SubpicSets.nCount, !m_SubpicSets.bAnimationWhenBuffering, m_SubpicSets.bAllowDrop, m_pSubPicAllocator, &hr)
						 : (ISubPicQueue*)DNew CSubPicQueueNoThread(!m_SubpicSets.bAnimationWhenBuffering, m_pSubPicAllocator, &hr);
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

// ISubRender11Callback

HRESULT CMPCVRAllocatorPresenter::SetDevice11(ID3D11Device* pD3DDev)
{
	if (!pD3DDev) {
		// release all resources
		m_pSubPicQueue.Release();
		m_pSubPicAllocator.Release();
		return S_OK;
	}

	CSize screenSize;
	MONITORINFO mi = { sizeof(MONITORINFO) };
	if (GetMonitorInfoW(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi)) {
		screenSize.SetSize(mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top);
	}
	InitMaxSubtitleTextureSize(m_SubpicSets.iMaxTexWidth, screenSize);

	if (m_pSubPicAllocator) {
		m_pSubPicAllocator->ChangeDevice(pD3DDev);
	}
	else {
		m_pSubPicAllocator = DNew CDX11SubPicAllocator(pD3DDev, m_maxSubtitleTextureSize);
		if (!m_pSubPicAllocator) {
			return E_FAIL;
		}
	}

	HRESULT hr = S_OK;
	if (!m_pSubPicQueue) {
		CAutoLock cAutoLock(this);
		m_pSubPicQueue = m_SubpicSets.nCount > 0
			? (ISubPicQueue*)DNew CSubPicQueue(m_SubpicSets.nCount, !m_SubpicSets.bAnimationWhenBuffering, m_SubpicSets.bAllowDrop, m_pSubPicAllocator, &hr)
			: (ISubPicQueue*)DNew CSubPicQueueNoThread(!m_SubpicSets.bAnimationWhenBuffering, m_pSubPicAllocator, &hr);
	}
	else {
		m_pSubPicQueue->Invalidate();
	}

	if (SUCCEEDED(hr) && m_pSubPicQueue && m_pSubPicProvider) {
		m_pSubPicQueue->SetSubPicProvider(m_pSubPicProvider);
	}

	return hr;
}

HRESULT CMPCVRAllocatorPresenter::Render11(
	REFERENCE_TIME rtStart,
	REFERENCE_TIME rtStop,
	REFERENCE_TIME atpf,
	RECT croppedVideoRect,
	RECT originalVideoRect,
	RECT viewportRect,
	const double videoStretchFactor,
	int xOffsetInPixels, DWORD flags)
{
	return RenderEx3(rtStart, rtStop, atpf, croppedVideoRect, originalVideoRect, viewportRect, videoStretchFactor, xOffsetInPixels, flags);
}

// IAllocatorPresenter

STDMETHODIMP CMPCVRAllocatorPresenter::CreateRenderer(IUnknown** ppRenderer)
{
	CheckPointer(ppRenderer, E_POINTER);
	ASSERT(!m_pMPCVR);

	HRESULT hr = m_pMPCVR.CoCreateInstance(CLSID_MPCVR, GetOwner());
	if (FAILED(hr)) {
		return hr;
	}

	if (CComQIPtr<ISubRender> pSR = m_pMPCVR.p) {
		VERIFY(SUCCEEDED(pSR->SetCallback(this)));
	}
	if (CComQIPtr<ISubRender11> pSR11 = m_pMPCVR.p) {
		VERIFY(SUCCEEDED(pSR11->SetCallback11(this)));
	}

	(*ppRenderer = (IUnknown*)(INonDelegatingUnknown*)(this))->AddRef();

	if (CComQIPtr<IExFilterConfig> pIExFilterConfig = m_pMPCVR.p) {
		hr = pIExFilterConfig->SetBool("lessRedraws", true);
		hr = pIExFilterConfig->SetBool("d3dFullscreenControl", m_bMPCVRFullscreenControl);
	}

	CComQIPtr<IBaseFilter> pBF(m_pMPCVR);
	HookNewSegmentAndReceive(GetFirstPin(pBF), true);

	return S_OK;
}

STDMETHODIMP_(CLSID) CMPCVRAllocatorPresenter::GetAPCLSID()
{
	return CLSID_MPCVRAllocatorPresenter;
}

STDMETHODIMP_(void) CMPCVRAllocatorPresenter::SetPosition(RECT w, RECT v)
{
	if (CComQIPtr<IBasicVideo> pBV = m_pMPCVR.p) {
		pBV->SetDefaultSourcePosition();
		pBV->SetDestinationPosition(v.left, v.top, v.right - v.left, v.bottom - v.top);
	}

	if (CComQIPtr<IVideoWindow> pVW = m_pMPCVR.p) {
		pVW->SetWindowPosition(w.left, w.top, w.right - w.left, w.bottom - w.top);
	}

	__super::SetPosition(w, v);
}

STDMETHODIMP CMPCVRAllocatorPresenter::SetRotation(int rotation)
{
	if (AngleStep90(rotation)) {
		HRESULT hr = E_NOTIMPL;
		if (CComQIPtr<IExFilterConfig> pIExFilterConfig = m_pMPCVR.p) {
			int curRotation = rotation;
			hr = pIExFilterConfig->GetInt("rotation", &curRotation);
			if (SUCCEEDED(hr) && rotation != curRotation) {
				hr = pIExFilterConfig->SetInt("rotation", rotation);
				if (SUCCEEDED(hr)) {
					m_bOtherTransform = true;
				}
			}
		}
		return hr;
	}
	return E_INVALIDARG;
}

STDMETHODIMP_(int) CMPCVRAllocatorPresenter::GetRotation()
{
	if (CComQIPtr<IExFilterConfig> pIExFilterConfig = m_pMPCVR.p) {
		int rotation = 0;
		if (SUCCEEDED(pIExFilterConfig->GetInt("rotation", &rotation))) {
			return rotation;
		}
	}
	return 0;
}

STDMETHODIMP CMPCVRAllocatorPresenter::SetFlip(bool flip)
{
	HRESULT hr = E_NOTIMPL;
	if (CComQIPtr<IExFilterConfig> pIExFilterConfig = m_pMPCVR.p) {
		bool curFlip = false;
		hr = pIExFilterConfig->GetBool("flip", &curFlip);
		if (SUCCEEDED(hr) && flip != curFlip) {
			hr = pIExFilterConfig->SetBool("flip", flip);
			if (SUCCEEDED(hr)) {
				m_bOtherTransform = true;
			}
		}
	}
	return hr;
}

STDMETHODIMP_(bool) CMPCVRAllocatorPresenter::GetFlip()
{
	if (CComQIPtr<IExFilterConfig> pIExFilterConfig = m_pMPCVR.p) {
		bool flip = false;
		if (SUCCEEDED(pIExFilterConfig->GetBool("flip", &flip))) {
			return flip;
		}
	}
	return false;
}

STDMETHODIMP_(SIZE) CMPCVRAllocatorPresenter::GetVideoSize()
{
	SIZE size = {0, 0};
	if (CComQIPtr<IBasicVideo> pBV = m_pMPCVR.p) {
		// Final size of the video, after all scaling and cropping operations
		// This is also aspect ratio adjusted
		pBV->GetVideoSize(&size.cx, &size.cy);
	}
	return size;
}

STDMETHODIMP_(SIZE) CMPCVRAllocatorPresenter::GetVideoSizeAR()
{
	SIZE size = {0, 0};
	if (CComQIPtr<IBasicVideo2> pBV2 = m_pMPCVR.p) {
		pBV2->GetPreferredAspectRatio(&size.cx, &size.cy);
	}
	return size;
}

STDMETHODIMP_(bool) CMPCVRAllocatorPresenter::Paint(bool /*bAll*/)
{
	if (CComQIPtr<IExFilterConfig> pIExFilterConfig = m_pMPCVR.p) {
		return SUCCEEDED(pIExFilterConfig->SetBool("cmd_redraw", true));
	}
	return false;
}

STDMETHODIMP CMPCVRAllocatorPresenter::GetDIB(BYTE* lpDib, DWORD* size)
{
	HRESULT hr = E_NOTIMPL;
	if (CComQIPtr<IBasicVideo> pBV = m_pMPCVR.p) {
		hr = pBV->GetCurrentImage((long*)size, (long*)lpDib);
	}
	return hr;
}

STDMETHODIMP CMPCVRAllocatorPresenter::GetDisplayedImage(LPVOID* dibImage)
{
	if (CComQIPtr<IExFilterConfig> pIExFilterConfig = m_pMPCVR.p) {
		unsigned size = 0;
		HRESULT hr = pIExFilterConfig->GetBin("displayedImage", dibImage, &size);

		return hr;
	}

	return E_FAIL;
}

STDMETHODIMP_(int) CMPCVRAllocatorPresenter::GetPixelShaderMode()
{
	if (CComQIPtr<IExFilterConfig> pIExFilterConfig = m_pMPCVR.p) {
		int rtype = 0;
		if (S_OK == pIExFilterConfig->GetInt("renderType", &rtype)) {
			return rtype;
		}
	}
	return -1;
}

STDMETHODIMP CMPCVRAllocatorPresenter::ClearPixelShaders(int target)
{
	HRESULT hr = E_FAIL;

	if (TARGET_SCREEN == target) {
		if (CComQIPtr<IExFilterConfig> pIExFilterConfig = m_pMPCVR.p) {
			hr = pIExFilterConfig->SetBool("cmd_clearPostScaleShaders", true);
		}
	}
	return hr;
}

BYTE* WriteChunk(BYTE* dst, const uint32_t code, const int32_t size, BYTE* data)
{
	memcpy(dst, &code, 4);
	dst += 4;
	memcpy(dst, &size, 4);
	dst += 4;
	memcpy(dst, data, size);
	dst += size;

	return dst;
}

STDMETHODIMP CMPCVRAllocatorPresenter::AddPixelShader(int target, LPCWSTR name, LPCSTR profile, LPCSTR sourceCode)
{
	int iProfile = 0;
	if (!strcmp(profile, "ps_2_0") || !strcmp(profile, "ps_2_a") || !strcmp(profile, "ps_2_b") || !strcmp(profile, "ps_3_0")) {
		iProfile = 3;
	}
	else if (!strcmp(profile, "ps_4_0")) {
		iProfile = 4;
	}
	else {
		return E_INVALIDARG;
	}

	HRESULT hr = E_ABORT;
	const int namesize = wcslen(name) * sizeof(wchar_t);
	const int codesize = strlen(sourceCode);

	if (codesize && TARGET_SCREEN == target) {
		if (CComQIPtr<IExFilterConfig> pIExFilterConfig = m_pMPCVR.p) {
			int rtype = 0;
			hr = pIExFilterConfig->GetInt("renderType", &rtype);
			if (S_OK == hr && (rtype == 9 && iProfile == 3 || rtype == 11 && iProfile == 4)) {
				int size = 8 + codesize;
				if (namesize) {
					size += 8 + namesize;
				}

				BYTE* pBuf = (BYTE*)LocalAlloc(LMEM_FIXED, size);
				if (pBuf) {
					BYTE* p = pBuf;
					if (namesize) {
						p = WriteChunk(p, FCC('NAME'), namesize, (BYTE*)name);
					}
					p = WriteChunk(p, FCC('CODE'), codesize, (BYTE*)sourceCode);

					hr = pIExFilterConfig->SetBin("cmd_addPostScaleShader", (LPVOID)pBuf, size);
					LocalFree(pBuf);
				}
			}
		}
	}

	return hr;
}

STDMETHODIMP_(bool) CMPCVRAllocatorPresenter::IsRendering()
{
	if (CComQIPtr<IExFilterConfig> pIExFilterConfig = m_pMPCVR.p) {
		int playbackState;
		if (SUCCEEDED(pIExFilterConfig->GetInt("playbackState", &playbackState))) {
			return playbackState == State_Running;
		}
	}

	return false;
}

STDMETHODIMP_(void) CMPCVRAllocatorPresenter::SetStereo3DSettings(Stereo3DSettings* pStereo3DSets)
{
	if (pStereo3DSets) {
		m_Stereo3DSets = *pStereo3DSets;
		if (CComQIPtr<IExFilterConfig> pIExFilterConfig = m_pMPCVR.p) {
			pIExFilterConfig->SetInt("stereo3dTransform", m_Stereo3DSets.iTransform);
		}
	}
}

STDMETHODIMP_(void) CMPCVRAllocatorPresenter::SetExtraSettings(ExtraRendererSettings* pExtraSets)
{
	if (pExtraSets && !m_pMPCVR) {
		m_bMPCVRFullscreenControl = pExtraSets->bMPCVRFullscreenControl;
	}
}

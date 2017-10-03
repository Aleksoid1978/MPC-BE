/*
 * (C) 2003-2006 Gabest
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

#pragma once

#include <atlbase.h>
#include <atlcoll.h>
#include "ISubPic.h"
#include <SubRenderIntf.h>

class CSubPicAllocatorPresenterImpl
	: public CUnknown
	, public CCritSec
	, public ISubPicAllocatorPresenter3
	, public ISubRenderConsumer2
{
private:
	CCritSec m_csSubPicProvider;

protected:
	HWND m_hWnd;
	REFERENCE_TIME m_rtSubtitleDelay;

	CSize m_maxSubtitleTextureSize;
	CSize m_nativeVideoSize;
	CSize m_aspectRatio;
	CRect m_videoRect;
	CRect m_windowRect;

	REFERENCE_TIME m_rtNow;
	double m_fps;
	UINT m_refreshRate;

	CMediaType m_inputMediaType;

	CComPtr<ISubPicProvider> m_pSubPicProvider;
	CComPtr<ISubPicAllocator> m_pAllocator;
	CComPtr<ISubPicQueue> m_pSubPicQueue;

	bool m_bDeviceResetRequested;
	bool m_bPendingResetDevice;

	void InitMaxSubtitleTextureSize(const int maxWidth, const CSize& desktopSize);
	void AlphaBltSubPic(const CRect& windowRect, const CRect& videoRect, int xOffsetInPixels = 0);
	void AlphaBlt(const CRect& windowRect, const CRect& videoRect, ISubPic* pSubPic, SubPicDesc* pTarget = NULL, int xOffsetInPixels = 0, const BOOL bUseSpecialCase = TRUE);

public:
	CSubPicAllocatorPresenterImpl(HWND hWnd, HRESULT& hr, CString *_pError);
	virtual ~CSubPicAllocatorPresenterImpl();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// ISubPicAllocatorPresenter3
	STDMETHODIMP CreateRenderer(IUnknown** ppRenderer) { return E_NOTIMPL; }
	STDMETHODIMP_(SIZE) GetVideoSize();
	STDMETHODIMP_(SIZE) GetVideoSizeAR();
	STDMETHODIMP_(void) SetPosition(RECT w, RECT v);
	STDMETHODIMP SetRotation(int rotation) { return E_NOTIMPL; }
	STDMETHODIMP_(int) GetRotation() { return 0; }
	STDMETHODIMP SetFlip(bool flip) { return E_NOTIMPL; }
	STDMETHODIMP_(bool) GetFlip() { return false; }
	STDMETHODIMP_(bool) Paint(bool fAll) { return false; }
	STDMETHODIMP_(void) SetTime(REFERENCE_TIME rtNow);
	STDMETHODIMP_(void) SetSubtitleDelay(int delay_ms);
	STDMETHODIMP_(int) GetSubtitleDelay();
	STDMETHODIMP_(double) GetFPS();
	STDMETHODIMP_(void) SetSubPicProvider(ISubPicProvider* pSubPicProvider);
	STDMETHODIMP_(void) Invalidate(REFERENCE_TIME rtInvalidate = -1);
	STDMETHODIMP GetDIB(BYTE* lpDib, DWORD* size) { return E_NOTIMPL; }
	STDMETHODIMP ClearPixelShaders(int target) { return E_NOTIMPL; }
	STDMETHODIMP AddPixelShader(int target, LPCSTR sourceCode, LPCSTR profile) { return E_NOTIMPL; }
	STDMETHODIMP_(bool) ResetDevice() { return false; }
	STDMETHODIMP_(bool) DisplayChange() { return false; }
	STDMETHODIMP_(bool) IsRendering() { return true; }

	// ISubRenderOptions
	STDMETHODIMP GetBool(LPCSTR field, bool* value);
	STDMETHODIMP GetInt(LPCSTR field, int* value);
	STDMETHODIMP GetSize(LPCSTR field, SIZE* value);
	STDMETHODIMP GetRect(LPCSTR field, RECT* value);
	STDMETHODIMP GetUlonglong(LPCSTR field, ULONGLONG* value);
	STDMETHODIMP GetDouble(LPCSTR field, double* value);
	STDMETHODIMP GetString(LPCSTR field, LPWSTR* value, int* chars);
	STDMETHODIMP GetBin(LPCSTR field, LPVOID* value, int* size);
	STDMETHODIMP SetBool(LPCSTR field, bool value);
	STDMETHODIMP SetInt(LPCSTR field, int value);
	STDMETHODIMP SetSize(LPCSTR field, SIZE value);
	STDMETHODIMP SetRect(LPCSTR field, RECT value);
	STDMETHODIMP SetUlonglong(LPCSTR field, ULONGLONG value);
	STDMETHODIMP SetDouble(LPCSTR field, double value);
	STDMETHODIMP SetString(LPCSTR field, LPWSTR value, int chars);
	STDMETHODIMP SetBin(LPCSTR field, LPVOID value, int size);

	// ISubRenderConsumer
	STDMETHODIMP GetMerit(ULONG* plMerit);
	STDMETHODIMP Connect(ISubRenderProvider* subtitleRenderer);
	STDMETHODIMP Disconnect();
	STDMETHODIMP DeliverFrame(REFERENCE_TIME start, REFERENCE_TIME stop, LPVOID context, ISubRenderFrame* subtitleFrame);

	// ISubRenderConsumer2
	STDMETHODIMP Clear(REFERENCE_TIME clearNewerThan = 0);
};

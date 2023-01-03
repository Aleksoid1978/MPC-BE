/*
 * (C) 2022-2023 see Authors.txt
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

#include "IAllocatorPresenter.h"
#include <SubRenderIntf.h>

class CAllocatorPresenterImpl
	: public CUnknown
	, public CCritSec
	, public IAllocatorPresenter
	, public ISubRenderConsumer2
{
private:
	CCritSec m_csSubPicProvider;

protected:
	HWND m_hWnd;
	REFERENCE_TIME m_rtSubtitleDelay = 0;

	CSize m_maxSubtitleTextureSize;
	CSize m_nativeVideoSize;
	CSize m_aspectRatio;
	CRect m_videoRect;
	CRect m_windowRect;
	bool  m_bOtherTransform = false;

	REFERENCE_TIME m_rtNow = 0;
	double m_fps           = 25.0;
	UINT m_refreshRate     = 0;

	SubpicSettings m_SubpicSets;
	Stereo3DSettings m_Stereo3DSets;

	CMediaType m_inputMediaType;

	CComPtr<ISubPicProvider>  m_pSubPicProvider;
	CComPtr<ISubPicAllocator> m_pSubPicAllocator;
	CComPtr<ISubPicQueue>     m_pSubPicQueue;

	void InitMaxSubtitleTextureSize(const int maxWidth, const CSize& desktopSize);
	HRESULT AlphaBltSubPic(const CRect& windowRect, const CRect& videoRect, int xOffsetInPixels = 0);
	HRESULT AlphaBlt(const CRect& windowRect, const CRect& videoRect, ISubPic* pSubPic, SubPicDesc* pTarget = nullptr, int xOffsetInPixels = 0, const BOOL bUseSpecialCase = TRUE);

public:
	CAllocatorPresenterImpl(HWND hWnd, HRESULT& hr, CString *_pError);
	virtual ~CAllocatorPresenterImpl();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IAllocatorPresenter
	STDMETHODIMP DisableSubPicInitialization() { return E_NOTIMPL; }
	STDMETHODIMP EnablePreviewModeInitialization() { return E_NOTIMPL; }
	STDMETHODIMP CreateRenderer(IUnknown** ppRenderer) { return E_NOTIMPL; }
	STDMETHODIMP_(CLSID) GetAPCLSID() { return GUID_NULL; }
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
	STDMETHODIMP GetVideoFrame(BYTE* lpDib, DWORD* size) { return E_NOTIMPL; }
	STDMETHODIMP GetDisplayedImage(LPVOID* dibImage) { return E_NOTIMPL; }
	STDMETHODIMP_(int) GetPixelShaderMode() { return 0; }
	STDMETHODIMP ClearPixelShaders(int target) { return E_NOTIMPL; }
	STDMETHODIMP AddPixelShader(int target, LPCWSTR name, LPCSTR profile, LPCSTR sourceCode) { return E_NOTIMPL; }
	STDMETHODIMP_(bool) ResizeDevice() { return false; }
	STDMETHODIMP_(bool) ResetDevice() { return false; }
	STDMETHODIMP_(bool) DisplayChange() { return false; }
	STDMETHODIMP_(bool) IsRendering() { return true; }
	STDMETHODIMP_(void) ResetStats() {}
	STDMETHODIMP_(void) SetSubpicSettings(SubpicSettings* pSubpicSets) override;
	STDMETHODIMP_(void) SetStereo3DSettings(Stereo3DSettings* pStereo3DSets) override;
	STDMETHODIMP_(void) SetExtraSettings(ExtraRendererSettings* pExtraSets) override {}

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

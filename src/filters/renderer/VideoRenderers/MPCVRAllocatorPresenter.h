/*
 * (C) 2019 see Authors.txt
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

#include "AllocatorCommon.h"
#include "../../../SubPic/SubPicAllocatorPresenterImpl.h"
#include "../../../SubPic/ISubRender.h"

namespace DSObjects
{
	class CMPCVRAllocatorPresenter : public CSubPicAllocatorPresenterImpl, ISubRenderCallback4
	{
		CComPtr<IUnknown> m_pMPCVR;

	public:
		CMPCVRAllocatorPresenter(HWND hWnd, HRESULT& hr, CString& _Error);
		virtual ~CMPCVRAllocatorPresenter();

		DECLARE_IUNKNOWN
		STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv) override;

		// ISubRenderCallback
		STDMETHODIMP SetDevice(IDirect3DDevice9* pD3DDev) override;
		STDMETHODIMP Render(REFERENCE_TIME rtStart, int left, int top, int right,
							int bottom, int width, int height) override {
			return RenderEx(rtStart, 0, 0, left, top, right, bottom, width, height);
		};

		// ISubRenderCallback2
		STDMETHODIMP RenderEx(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, REFERENCE_TIME atpf,
							  int left, int top, int right, int bottom, int width, int height) override {
			return RenderEx2(rtStart, rtStop, atpf, { left, top, right, bottom },
			{ left, top, right, bottom }, { 0, 0, width, height });
		};

		// ISubRenderCallback3
		STDMETHODIMP RenderEx2(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop,
							   REFERENCE_TIME atpf, RECT croppedVideoRect,
							   RECT originalVideoRect, RECT viewportRect,
							   const double videoStretchFactor = 1.0) override {
			return RenderEx3(rtStart, rtStop, atpf, croppedVideoRect, originalVideoRect, viewportRect, videoStretchFactor);
		}

		// ISubRenderCallback4
		STDMETHODIMP RenderEx3(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop,
							   REFERENCE_TIME atpf, RECT croppedVideoRect,
							   RECT originalVideoRect, RECT viewportRect,
							   const double videoStretchFactor = 1.0,
							   int xOffsetInPixels = 0, DWORD flags = 0) override;

		// ISubPicAllocatorPresenter3
		STDMETHODIMP CreateRenderer(IUnknown** ppRenderer) override;
		STDMETHODIMP_(SIZE) GetVideoSize() override;
		STDMETHODIMP_(SIZE) GetVideoSizeAR() override;
		STDMETHODIMP_(void) SetPosition(RECT w, RECT v) override;
		STDMETHODIMP SetRotation(int rotation) override;
		STDMETHODIMP_(int) GetRotation() override;
		STDMETHODIMP GetDIB(BYTE* lpDib, DWORD* size) override;
		STDMETHODIMP_(bool) IsRendering() override;
	};
}

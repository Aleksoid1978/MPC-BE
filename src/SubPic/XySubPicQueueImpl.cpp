/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2015 see Authors.txt
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
#include "XySubPicQueueImpl.h"
#include "XySubPicProvider.h"

#define SUBPIC_TRACE_LEVEL 0

//
// CXySubPicQueueNoThread
//

CXySubPicQueueNoThread::CXySubPicQueueNoThread(ISubPicAllocator* pAllocator, HRESULT* phr)
	: CSubPicQueueNoThread(false, pAllocator, phr)
	, m_llSubId(0)
{
}

CXySubPicQueueNoThread::~CXySubPicQueueNoThread()
{
}

// ISubPicQueue

STDMETHODIMP CXySubPicQueueNoThread::Invalidate(REFERENCE_TIME rtInvalidate)
{
	m_llSubId = 0;
	return __super::Invalidate(rtInvalidate);
}

STDMETHODIMP_(bool) CXySubPicQueueNoThread::LookupSubPic(REFERENCE_TIME rtNow, CComPtr<ISubPic>& ppSubPic)
{
	// CXySubPicQueueNoThread is always blocking so bAdviseBlocking doesn't matter anyway
	return LookupSubPic(rtNow, true, ppSubPic);
}

STDMETHODIMP_(bool) CXySubPicQueueNoThread::LookupSubPic(REFERENCE_TIME rtNow, bool /*bAdviseBlocking*/, CComPtr<ISubPic>& ppSubPic)
{
	// CXySubPicQueueNoThread is always blocking so we ignore bAdviseBlocking

	CComPtr<ISubPic> pSubPic;
	CComPtr<ISubPicProvider> pSubPicProvider;
	GetSubPicProvider(&pSubPicProvider);
	CComQIPtr<IXyCompatProvider> pXySubPicProvider = pSubPicProvider;

	{
		CAutoLock cAutoLock(&m_csLock);

		pSubPic = m_pSubPic;
	}

	if (pXySubPicProvider) {
		double fps = m_fps;
		REFERENCE_TIME rtTimePerFrame = static_cast<REFERENCE_TIME>(10000000.0 / fps);

		REFERENCE_TIME rtStart = rtNow;
		REFERENCE_TIME rtStop = rtNow + rtTimePerFrame;

		HRESULT hr = pXySubPicProvider->RequestFrame(rtStart, rtStop, (DWORD)(1000.0 / fps));
		if (SUCCEEDED(hr)) {
			ULONGLONG id;
			hr = pXySubPicProvider->GetID(&id);
			if (SUCCEEDED(hr)) {
				bool	bAllocSubPic = !pSubPic;
				SIZE	MaxTextureSize, VirtualSize;
				POINT   VirtualTopLeft;
				HRESULT hr2;
				if (SUCCEEDED(hr2 = pSubPicProvider->GetTextureSize(0, MaxTextureSize, VirtualSize, VirtualTopLeft))) {
					m_pAllocator->SetMaxTextureSize(MaxTextureSize);
					if (!bAllocSubPic) {
						// Ensure the previously allocated subpic is big enough to hold the subtitle to be rendered
						SIZE maxSize;
						bAllocSubPic = FAILED(pSubPic->GetMaxSize(&maxSize)) || maxSize.cx < MaxTextureSize.cx || maxSize.cy < MaxTextureSize.cy;
					}
				}

				SUBTITLE_TYPE sType = pSubPicProvider->GetType();

				if (bAllocSubPic) {
					CAutoLock cAutoLock(&m_csLock);

					m_pSubPic.Release();

					if (FAILED(m_pAllocator->AllocDynamic(&m_pSubPic))) {
						return false;
					}

					pSubPic = m_pSubPic;
				}

				if (!bAllocSubPic && m_llSubId == id) { // same subtitle as last time
					pSubPic->SetStop(rtStop);
					ppSubPic = pSubPic;
				} else if (m_pAllocator->IsDynamicWriteOnly()) {
					CComPtr<ISubPic> pStatic;
					hr = m_pAllocator->GetStatic(&pStatic);
					if (SUCCEEDED(hr)) {
						pStatic->SetInverseAlpha(true);
						hr = RenderTo(pStatic, rtStart, rtStop, fps, true);
					}
					if (SUCCEEDED(hr)) {
						hr = pStatic->CopyTo(pSubPic);
					}
					if (SUCCEEDED(hr)) {
						ppSubPic = pSubPic;
						m_llSubId = id;
					}
				} else {
					pSubPic->SetInverseAlpha(true);
					if (SUCCEEDED(RenderTo(pSubPic, rtStart, rtStop, fps, true))) {
						ppSubPic = pSubPic;
						m_llSubId = id;
					}
				}

				if (ppSubPic) {
					if (SUCCEEDED(hr2)) {
						ppSubPic->SetVirtualTextureSize(VirtualSize, VirtualTopLeft);
					}

					pSubPic->SetType(sType);
				}
			}
		}
	}

	return !!ppSubPic;
}

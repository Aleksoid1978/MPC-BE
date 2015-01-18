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

#pragma once

#include <memory>
#include <mutex>
#include <condition_variable>

#include "ISubPic.h"

class CSubPicQueueImpl : public CUnknown, public ISubPicQueue
{
	static const double DEFAULT_FPS;

protected:
	struct SubPicProviderWithSharedLock {
	private:
		unsigned int m_nRef = 0;
		std::mutex m_mutex;

	public:
		const CComPtr<ISubPicProvider> pSubPicProvider;

		SubPicProviderWithSharedLock(const CComPtr<ISubPicProvider>& pSubPicProvider)
			: pSubPicProvider(pSubPicProvider) {
		}

		HRESULT Lock() {
			HRESULT hr;

			if (pSubPicProvider) {
				std::lock_guard<std::mutex> lock(m_mutex);

				hr = S_OK;
				if ((m_nRef == 0 && SUCCEEDED(hr = pSubPicProvider->Lock()))
						|| m_nRef > 0) {
					m_nRef++;
				}
			} else {
				hr = E_POINTER;
			}

			return hr;
		}

		HRESULT Unlock() {
			HRESULT hr;

			if (pSubPicProvider) {
				std::lock_guard<std::mutex> lock(m_mutex);

				hr = S_OK;
				if ((m_nRef == 1 && SUCCEEDED(hr = pSubPicProvider->Unlock()))
						|| m_nRef > 1) {
					m_nRef--;
				}
			} else {
				hr = E_POINTER;
			}

			return hr;
		}
	};

private:
	CCritSec m_csSubPicProvider;
	std::shared_ptr<SubPicProviderWithSharedLock> m_pSubPicProviderWithSharedLock;

protected:
	double m_fps;
	REFERENCE_TIME m_rtTimePerFrame;
	REFERENCE_TIME m_rtNow;

	CComPtr<ISubPicAllocator> m_pAllocator;

	std::shared_ptr<SubPicProviderWithSharedLock> GetSubPicProviderWithSharedLock() {
		CAutoLock cAutoLock(&m_csSubPicProvider);
		return m_pSubPicProviderWithSharedLock;
	}

	HRESULT RenderTo(ISubPic* pSubPic, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double fps, BOOL bIsAnimated);

public:
	CSubPicQueueImpl(ISubPicAllocator* pAllocator, HRESULT* phr);
	virtual ~CSubPicQueueImpl();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// ISubPicQueue

	STDMETHODIMP SetSubPicProvider(ISubPicProvider* pSubPicProvider);
	STDMETHODIMP GetSubPicProvider(ISubPicProvider** pSubPicProvider);

	STDMETHODIMP SetFPS(double fps);
	STDMETHODIMP SetTime(REFERENCE_TIME rtNow);

	/*
	STDMETHODIMP Invalidate(REFERENCE_TIME rtInvalidate = -1) PURE;
	STDMETHODIMP_(bool) LookupSubPic(REFERENCE_TIME rtNow, ISubPic** ppSubPic) PURE;
	STDMETHODIMP_(bool) LookupSubPic(REFERENCE_TIME rtNow, bool bAdviseBlocking, CComPtr<ISubPic>& pSubPic);

	STDMETHODIMP GetStats(int& nSubPics, REFERENCE_TIME& rtNow, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop) PURE;
	STDMETHODIMP GetStats(int nSubPics, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop) PURE;
	*/
};

class CSubPicQueue : public CSubPicQueueImpl, protected CAMThread
{
protected:
	int  m_nMaxSubPic;
	bool m_bDisableAnim;
	bool m_bAllowDropSubPic;

	bool m_bExitThread;

	CComPtr<ISubPic> m_pSubPic;
	CInterfaceList<ISubPic> m_queue;

	std::mutex m_mutexSubpic; // to protect m_pSubPic
	std::mutex m_mutexQueue; // to protect m_queue
	std::condition_variable m_condQueueFull;
	std::condition_variable m_condQueueReady;

	CAMEvent m_runQueueEvent;

	REFERENCE_TIME m_rtNowLast;

	bool m_bInvalidate;
	REFERENCE_TIME m_rtInvalidate;

	bool EnqueueSubPic(CComPtr<ISubPic>& pSubPic, bool bBlocking);
	REFERENCE_TIME GetCurrentRenderingTime();

	// CAMThread
	virtual DWORD ThreadProc();

public:
	CSubPicQueue(int nMaxSubPic, bool bDisableAnim, bool bAllowDropSubPic, ISubPicAllocator* pAllocator, HRESULT* phr);
	virtual ~CSubPicQueue();

	// ISubPicQueue

	STDMETHODIMP SetFPS(double fps);
	STDMETHODIMP SetTime(REFERENCE_TIME rtNow);

	STDMETHODIMP Invalidate(REFERENCE_TIME rtInvalidate = -1);
	STDMETHODIMP_(bool) LookupSubPic(REFERENCE_TIME rtNow, CComPtr<ISubPic> &pSubPic);
	STDMETHODIMP_(bool) LookupSubPic(REFERENCE_TIME rtNow, bool bAdviseBlocking, CComPtr<ISubPic>& pSubPic);

	STDMETHODIMP GetStats(int& nSubPics, REFERENCE_TIME& rtNow, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop);
	STDMETHODIMP GetStats(int nSubPic, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop);
};

class CSubPicQueueNoThread : public CSubPicQueueImpl
{
protected:
	bool m_bDisableAnim;

	CCritSec m_csLock;
	CComPtr<ISubPic> m_pSubPic;

public:
	CSubPicQueueNoThread(bool bDisableAnim, ISubPicAllocator* pAllocator, HRESULT* phr);
	virtual ~CSubPicQueueNoThread();

	// ISubPicQueue

	STDMETHODIMP Invalidate(REFERENCE_TIME rtInvalidate = -1);
	STDMETHODIMP_(bool) LookupSubPic(REFERENCE_TIME rtNow, CComPtr<ISubPic> &pSubPic);
	STDMETHODIMP_(bool) LookupSubPic(REFERENCE_TIME rtNow, bool bAdviseBlocking, CComPtr<ISubPic>& pSubPic);

	STDMETHODIMP GetStats(int& nSubPics, REFERENCE_TIME& rtNow, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop);
	STDMETHODIMP GetStats(int nSubPic, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop);
};

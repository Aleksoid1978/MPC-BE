/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2025 see Authors.txt
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

#include "FGFilter.h"
#include "BaseGraph.h"

#define LowMerit(x) (x L" (low merit)")

class CFGManager
	: public CUnknown
	, public IGraphBuilder2
	, public IGraphBuilderDeadEnd
	, public IGraphBuilderSub
	, public IGraphBuilderAudio
	, public CCritSec
{
public:
	struct path_t {
		CLSID clsid;
		CString filter, pin;
	};

	class CStreamPath : public std::list<path_t>
	{
	public:
		void Append(IBaseFilter* pBF, IPin* pPin);
		bool Compare(const CStreamPath& path);
	};

	class CStreamDeadEnd : public CStreamPath
	{
	public:
		std::list<CMediaType> mts;
	};

private:
	CComPtr<IUnknown> m_pUnkInner;
	DWORD m_dwRegister = 0;

	CStreamPath m_streampath;
	std::vector<std::unique_ptr<CStreamDeadEnd>> m_deadends;

protected:
	CComPtr<IFilterMapper2> m_pFM;
	std::list<CComQIPtr<IUnknown, &IID_IUnknown>> m_pUnks;
	std::list<CFGFilter*> m_source, m_transform, m_override;

	bool CheckBytes(HANDLE hFile, CString chkbytes);
	bool CheckBytes(PBYTE buf, DWORD size, CString chkbytes);

	HRESULT EnumSourceFilters(LPCWSTR lpcwstrFileName, CFGFilterList& fl);
	HRESULT AddSourceFilterInternal(CFGFilter* pFGF, LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrFilterName, IBaseFilter** ppBF);
	HRESULT ConnectInternal(IPin* pPinOut, IPin* pPinIn, bool bContinueRender);

	// IFilterGraph

	STDMETHODIMP AddFilter(IBaseFilter* pFilter, LPCWSTR pName);
	STDMETHODIMP RemoveFilter(IBaseFilter* pFilter);
	STDMETHODIMP EnumFilters(IEnumFilters** ppEnum);
	STDMETHODIMP FindFilterByName(LPCWSTR pName, IBaseFilter** ppFilter);
	STDMETHODIMP ConnectDirect(IPin* pPinOut, IPin* pPinIn, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP Reconnect(IPin* ppin);
	STDMETHODIMP Disconnect(IPin* ppin);
	STDMETHODIMP SetDefaultSyncSource();

	// IGraphBuilder

	STDMETHODIMP Connect(IPin* pPinOut, IPin* pPinIn);
	STDMETHODIMP Render(IPin* pPinOut);
	STDMETHODIMP RenderFile(LPCWSTR lpcwstrFile, LPCWSTR lpcwstrPlayList);
	STDMETHODIMP AddSourceFilter(LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrFilterName, IBaseFilter** ppFilter);
	STDMETHODIMP SetLogFile(DWORD_PTR hFile);
	STDMETHODIMP Abort();
	STDMETHODIMP ShouldOperationContinue();

	// IFilterGraph2

	STDMETHODIMP AddSourceFilterForMoniker(IMoniker* pMoniker, IBindCtx* pCtx, LPCWSTR lpcwstrFilterName, IBaseFilter** ppFilter);
	STDMETHODIMP ReconnectEx(IPin* ppin, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP RenderEx(IPin* pPinOut, DWORD dwFlags, DWORD* pvContext);

	// IGraphBuilder2

	STDMETHODIMP IsPinDirection(IPin* pPin, PIN_DIRECTION dir);
	STDMETHODIMP IsPinConnected(IPin* pPin);
	STDMETHODIMP ConnectFilter(IBaseFilter* pBF, IPin* pPinIn);
	STDMETHODIMP ConnectFilter(IPin* pPinOut, IBaseFilter* pBF);
	STDMETHODIMP ConnectFilterDirect(IPin* pPinOut, IBaseFilter* pBF, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP NukeDownstream(IUnknown* pUnk);
	STDMETHODIMP FindInterface(REFIID iid, void** ppv, BOOL bRemove);
	STDMETHODIMP AddToROT();
	STDMETHODIMP RemoveFromROT();

	// IGraphBuilderDeadEnd

	STDMETHODIMP_(size_t) GetCount();
	STDMETHODIMP GetDeadEnd(int iIndex, std::list<CStringW>& path, std::list<CMediaType>& mts);

	// IGraphBuilderSub
	STDMETHODIMP RenderSubFile(LPCWSTR lpcwstrFile);

	// IGraphBuilderAudio
	STDMETHODIMP RenderAudioFile(LPCWSTR lpcwstrFile);

	BOOL m_bOnlySub = FALSE;
	BOOL m_bOnlyAudio = FALSE;
	CStringW m_userAgent;
	CStringW m_referrer; // not used

	//
	HWND m_hWnd;

	//
	HRESULT ConnectFilterDirect(IPin* pPinOut, CFGFilter* pFGF);

	bool m_bIsPreview;
	std::vector<CStringW> m_AudioRenderers;

	bool m_bOpeningAborted = false;
	std::mutex m_mutexRender;

public:
	CFGManager(LPCWSTR pName, LPUNKNOWN pUnk, HWND hWnd = 0, bool IsPreview = false);
	virtual ~CFGManager();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);
};

class CFGManagerCustom : public CFGManager
{
public:
	// IFilterGraph

	STDMETHODIMP AddFilter(IBaseFilter* pFilter, LPCWSTR pName) override;

public:
	CFGManagerCustom(LPCWSTR pName, LPUNKNOWN pUnk, HWND hWnd = 0, bool IsPreview = false);
};

class CFGManagerPlayer : public CFGManagerCustom
{
protected:
	// IFilterGraph

	STDMETHODIMP ConnectDirect(IPin* pPinOut, IPin* pPinIn, const AM_MEDIA_TYPE* pmt) override;

public:
	CFGManagerPlayer(LPCWSTR pName, LPUNKNOWN pUnk, HWND hWnd, int preview = 0);

	void SetUserAgent(CStringW useragent) { m_userAgent = useragent; };
	void SetReferrer(CStringW referrer) { m_referrer = referrer; };
};

class CFGManagerDVD : public CFGManagerPlayer
{
protected:
	// IGraphBuilder

	STDMETHODIMP RenderFile(LPCWSTR lpcwstrFile, LPCWSTR lpcwstrPlayList) override;
	STDMETHODIMP AddSourceFilter(LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrFilterName, IBaseFilter** ppFilter) override;

public:
	CFGManagerDVD(LPCWSTR pName, LPUNKNOWN pUnk, HWND hWnd, bool IsPreview = false);
};

class CFGManagerCapture : public CFGManagerPlayer
{
public:
	CFGManagerCapture(LPCWSTR pName, LPUNKNOWN pUnk, HWND hWnd);
};

class CFGManagerMuxer : public CFGManagerCustom
{
public:
	CFGManagerMuxer(LPCWSTR pName, LPUNKNOWN pUnk);
};

//

class CFGAggregator : public CUnknown
{
protected:
	CComPtr<IUnknown> m_pUnkInner;

public:
	CFGAggregator(const CLSID& clsid, LPCWSTR pName, LPUNKNOWN pUnk, HRESULT& hr);
	virtual ~CFGAggregator();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);
};

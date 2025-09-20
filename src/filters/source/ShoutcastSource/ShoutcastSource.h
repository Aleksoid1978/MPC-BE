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

#include <afxinet.h>
#include <qnetwork.h>
#include "MPCSocket.h"
#include "filters/parser/BaseSplitter/BaseSplitter.h"

#define ShoutcastSourceName   L"MPC ShoutCast Source"

enum StreamFormat {
	AUDIO_NONE,
	AUDIO_MPEG,
	AUDIO_AAC,
	AUDIO_PLS,
	AUDIO_M3U,
	AUDIO_XSPF,
};

class __declspec(uuid("68F540E9-766F-44d2-AB07-E26CC6D27A79"))
	CShoutcastSource
	: public CSource
	, public IFileSourceFilter
	, public IAMFilterMiscFlags
	, public IAMOpenProgress
	, public IAMMediaContent
	, public CExFilterConfigImpl
{
	CStringW m_fn;

public:
	CShoutcastSource(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CShoutcastSource();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IFileSourceFilter
	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt);

	// IAMFilterMiscFlags
	STDMETHODIMP_(ULONG) GetMiscFlags();

	// IAMOpenProgress
	STDMETHODIMP QueryProgress(LONGLONG* pllTotal, LONGLONG* pllCurrent);
	STDMETHODIMP AbortOperation();

	// IAMMediaContent
	STDMETHODIMP GetTypeInfoCount(UINT* pctinfo) {
		return E_NOTIMPL;
	}
	STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo) {
		return E_NOTIMPL;
	}
	STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid) {
		return E_NOTIMPL;
	}
	STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr) {
		return E_NOTIMPL;
	}
	STDMETHODIMP get_AuthorName(BSTR* pbstrAuthorName) {
		return E_NOTIMPL;
	}
	STDMETHODIMP get_Title(BSTR* pbstrTitle);
	STDMETHODIMP get_Rating(BSTR* pbstrRating) {
		return E_NOTIMPL;
	}
	STDMETHODIMP get_Description(BSTR* pbstrDescription);
	STDMETHODIMP get_Copyright(BSTR* pbstrCopyright) {
		return E_NOTIMPL;
	}
	STDMETHODIMP get_BaseURL(BSTR* pbstrBaseURL) {
		return E_NOTIMPL;
	}
	STDMETHODIMP get_LogoURL(BSTR* pbstrLogoURL) {
		return E_NOTIMPL;
	}
	STDMETHODIMP get_LogoIconURL(BSTR* pbstrLogoURL) {
		return E_NOTIMPL;
	}
	STDMETHODIMP get_WatermarkURL(BSTR* pbstrWatermarkURL) {
		return E_NOTIMPL;
	}
	STDMETHODIMP get_MoreInfoURL(BSTR* pbstrMoreInfoURL) {
		return E_NOTIMPL;
	}
	STDMETHODIMP get_MoreInfoBannerImage(BSTR* pbstrMoreInfoBannerImage) {
		return E_NOTIMPL;
	}
	STDMETHODIMP get_MoreInfoBannerURL(BSTR* pbstrMoreInfoBannerURL) {
		return E_NOTIMPL;
	}
	STDMETHODIMP get_MoreInfoText(BSTR* pbstrMoreInfoText) {
		return E_NOTIMPL;
	}

	// CBaseFilter
	STDMETHODIMP QueryFilterInfo(FILTER_INFO* pInfo);

	// IExFilterConfig
	STDMETHODIMP Flt_SetInt(LPCSTR field, int value);
};

class CShoutcastStream : public CSourceStream
{
	class CShoutCastPacket
	{
	public:
		std::unique_ptr<BYTE[]> data;
		size_t size = 0;
		CString title;
		REFERENCE_TIME rtStart = INVALID_TIME;
		REFERENCE_TIME rtStop = INVALID_TIME;
		CShoutCastPacket(const void* ptr, int len)
			: size(len)
		{
			ASSERT(len >= 0);
			data.reset(new BYTE[size]);
			memcpy(data.get(), ptr, size);
		}
	};

	class ShoutCastQueue
		: public std::deque<std::unique_ptr<CShoutCastPacket>>
		, public CCritSec
	{
	public:
		REFERENCE_TIME GetDuration() {
			CAutoLock cAutoLock(this);
			return size() ? (back()->rtStop - front()->rtStart) : 0;
		}
	};

	ShoutCastQueue m_queue;

	class CShoutcastSocket : public CMPCSocket
	{
		DWORD m_nBytesRead = 0;

	public:
		CShoutcastSocket() {
			SetTimeOut(3000, 3000);
		}

		void SetCodePage(const UINT codePage);
		int Receive(void* lpBuf, int nBufLen, int nFlags = 0) override;

		StreamFormat m_Format	= AUDIO_NONE;
		unsigned m_metaint		= 0;
		unsigned m_bitrate		= 0;
		unsigned m_samplerate	= 0;
		unsigned m_channels		= 0;
		unsigned m_framesize	= 0;
		unsigned m_aacprofile	= 0;

		UINT m_codepage = CP_ACP;
		CStringW m_title;
		CStringW m_url;
		CStringW m_description;

		bool Connect(const CUrlParser& urlParser, CString& redirectUrl);
		bool FindSync();

		CShoutcastSocket& operator = (const CShoutcastSocket& soc) {
			m_Format		= soc.m_Format;
			m_metaint		= soc.m_metaint;
			m_bitrate		= soc.m_bitrate;
			m_samplerate	= soc.m_samplerate;
			m_channels		= soc.m_channels;
			m_framesize		= soc.m_framesize;
			m_aacprofile	= soc.m_aacprofile;
			m_nBytesRead	= soc.m_nBytesRead;

			m_codepage		= soc.m_codepage;
			m_title			= soc.m_title;
			m_url			= soc.m_url;
			m_description	= soc.m_description;

			return *this;
		}

	} m_socket;

	HANDLE m_hSocketThread = nullptr;
	SOCKET m_hSocket = INVALID_SOCKET;

	CUrlParser m_urlParser;

	bool m_bBuffering = false;
	UINT m_codePage = CP_ACP;
	CString m_title;
	CString m_description;

	bool m_bExitThread = false;;

public:
	CShoutcastStream(const WCHAR* wfn, CShoutcastSource* pParent, HRESULT* phr);
	virtual ~CShoutcastStream();

	UINT SocketThreadProc();

	void EmptyBuffer();
	LONGLONG GetBufferFullness();

	void SetCodePage(const UINT codePage);

	CString GetTitle();
	CString GetDescription();

	HRESULT DecideBufferSize(IMemAllocator* pIMemAlloc, ALLOCATOR_PROPERTIES* pProperties);
	HRESULT FillBuffer(IMediaSample* pSample);
	HRESULT CheckMediaType(const CMediaType* pMediaType);
	HRESULT GetMediaType(int iPosition, CMediaType* pmt);

	STDMETHODIMP Notify(IBaseFilter* pSender, Quality q) {
		return E_NOTIMPL;
	}

	HRESULT OnThreadCreate();
	HRESULT OnThreadDestroy();
	HRESULT Inactive();

	HRESULT SetName(LPCWSTR pName);
};

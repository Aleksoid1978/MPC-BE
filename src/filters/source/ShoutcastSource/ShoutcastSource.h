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

#include <atlbase.h>
#include <atlutil.h>
#include <afxinet.h>
#include <afxsock.h>
#include <qnetwork.h>
#include "../../../DSUtil/MPCSocket.h"
#include "../../parser/BaseSplitter/BaseSplitter.h"

#define ShoutcastSourceName   L"MPC ShoutCast Source"

enum StreamFormat {
	AUDIO_NONE,
	AUDIO_MPEG,
	AUDIO_AAC,
	AUDIO_PLAYLIST
};

struct aachdr
{
	WORD sync:12;
	WORD version:1;
	WORD layer:2;
	WORD fcrc:1;
	WORD profile:2;
	WORD freq:4;
	WORD privatebit:1;
	WORD channels:3;
	WORD original:1;
	WORD home:1; // ?

	WORD copyright_id_bit:1;
	WORD copyright_id_start:1;
	WORD aac_frame_length:13;
	WORD adts_buffer_fullness:11;
	WORD no_raw_data_blocks_in_frame:2;

	WORD crc;

	int FrameSize, nBytesPerSec;
	REFERENCE_TIME rtDuration;

	aachdr() {
		memset(this, 0, sizeof(*this));
	}
};

static int aacfreq[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350};

class __declspec(uuid("68F540E9-766F-44d2-AB07-E26CC6D27A79"))
	CShoutcastSource
	: public CSource
	, public IFileSourceFilter
	, public IAMFilterMiscFlags
	, public IAMOpenProgress
	, public IAMMediaContent
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
};

class CShoutcastStream : public CSourceStream
{
	class CShoutCastPacket : public CAtlArray < BYTE >
	{
	public:
		CString title;
		REFERENCE_TIME rtStart = INVALID_TIME;
		REFERENCE_TIME rtStop = INVALID_TIME;
		void SetData(const void* ptr, DWORD len) {
			SetCount(len);
			memcpy(GetData(), ptr, len);
		}
	};

	class ShoutCastqueue
		: public CAutoPtrList<CShoutCastPacket>
		, public CCritSec
	{
	public:
		REFERENCE_TIME GetDuration() {
			CAutoLock cAutoLock(this);
			return GetCount() ? (GetTail()->rtStop - GetHead()->rtStart) : 0;
		}
	};

	ShoutCastqueue m_queue;

	class CShoutcastSocket : public CMPCSocket
	{
		DWORD m_nBytesRead;

	public:
		CShoutcastSocket() {
			SetTimeOut(3000, 3000);
			m_metaint		= m_bitrate = m_freq = m_channels = 0;
			m_nBytesRead	= 0;
			m_Format		= AUDIO_NONE;
		}

		int Receive(void* lpBuf, int nBufLen, int nFlags = 0);

		DWORD m_metaint, m_bitrate, m_freq, m_channels;
		aachdr m_aachdr;
		StreamFormat m_Format;
		CString m_title, m_url, m_Description;
		bool Connect(CUrl& url, CString& redirectUrl);
		bool FindSync();

		CShoutcastSocket& operator = (const CShoutcastSocket& soc) {
			m_metaint		= soc.m_metaint;
			m_bitrate		= soc.m_bitrate;
			m_freq			= soc.m_freq;
			m_channels		= soc.m_channels;
			m_nBytesRead	= soc.m_nBytesRead;
			m_Format		= soc.m_Format;
			m_aachdr		= soc.m_aachdr;

			m_title			= soc.m_title;
			m_url			= soc.m_url;
			m_Description	= soc.m_Description;

			return *this;
		}

	} m_socket;

	HANDLE m_hSocketThread;
	SOCKET m_hSocket;

	CUrl m_url;

	bool m_fBuffering;
	CString m_title, m_Description;

public:
	CShoutcastStream(const WCHAR* wfn, CShoutcastSource* pParent, HRESULT* phr);
	virtual ~CShoutcastStream();

	bool fExitThread;
	UINT SocketThreadProc();

	void EmptyBuffer();
	LONGLONG GetBufferFullness();
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
	HRESULT Pause();

	HRESULT SetName(LPCWSTR pName);
};

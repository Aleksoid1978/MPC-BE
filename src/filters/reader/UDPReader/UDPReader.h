/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2016 see Authors.txt
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

#include <AsyncReader/asyncio.h>
#include <AsyncReader/asyncrdr.h>
#include "../../../DSUtil/HTTPAsync.h"

#define UDPReaderName   L"MPC UDP/HTTP Reader"
#define STDInReaderName L"MPC Std input Reader"

class CUDPStream : public CAsyncStream, public CAMThread
{
public:
	enum protocol {
		PR_NONE,
		PR_UDP,
		PR_HTTP,
		PR_PIPE
	};

private:
	CCritSec m_csLock;
	CCritSec m_csPacketsLock;

	class packet_t
	{
	public:
		BYTE*   m_buff;
		__int64 m_start, m_end;

		packet_t(BYTE* p, __int64 start, int size);
		virtual ~packet_t() {
			delete [] m_buff;
		}
	};

	CString		m_url_str;
	CUrl		m_url;
	protocol	m_protocol;

	SOCKET		m_UdpSocket;
	sockaddr_in	m_addr;
	WSAEVENT	m_WSAEvent;

	CHTTPAsync	m_HTTPAsync;

	__int64		m_pos, m_len;
	CAtlList<packet_t*> m_packets;

	GUID		m_subtype;
	DWORD		m_RequestCmd;

	CAMEvent         m_EventComplete;
	volatile __int64 m_SizeComplete;

	void Clear();
	void Append(BYTE* buff, int len);

	DWORD ThreadProc();

	inline __int64 GetPacketsSize();
	void CheckBuffer();
	void EmptyBuffer();

public:
	CUDPStream();
	virtual ~CUDPStream();

	enum CMD {
		CMD_INIT,
		CMD_PAUSE,
		CMD_RUN,
		CMD_STOP,
		CMD_EXIT
	};

	bool Load(const WCHAR* fnw);

	// CAsyncStream
	HRESULT SetPointer(LONGLONG llPos);
	HRESULT Read(PBYTE pbBuffer, DWORD dwBytesToRead, BOOL bAlign, LPDWORD pdwBytesRead);
	LONGLONG Size(LONGLONG* pSizeAvailable);
	DWORD Alignment();
	void Lock();
	void Unlock();

	GUID GetSubtype() const { return m_subtype; }
	protocol GetProtocol() const { return m_protocol; }
};

class __declspec(uuid("0E4221A9-9718-48D5-A5CF-4493DAD4A015"))
	CUDPReader
	: public CAsyncReader
	, public IFileSourceFilter
{
	CUDPStream m_stream;
	CStringW   m_fn;

public:
	CUDPReader(IUnknown* pUnk, HRESULT* phr);
	~CUDPReader();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// CBaseFilter
	STDMETHODIMP QueryFilterInfo(FILTER_INFO* pInfo);
	STDMETHODIMP Stop();
	STDMETHODIMP Pause();
	STDMETHODIMP Run(REFERENCE_TIME tStart);

	// IFileSourceFilter
	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt);
};

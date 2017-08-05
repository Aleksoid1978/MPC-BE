/*
 * (C) 2017 see Authors.txt
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

#include <deque>
#include <AsyncReader/asyncio.h>
#include "../../../DSUtil/HTTPAsync.h"

#define UDPReaderName   L"MPC UDP/HTTP Reader"
#define STDInReaderName L"MPC Std input Reader"

class CUDPStream
	: public CAsyncStream
	, public CAMThread
{
public:
	enum protocol {
		PR_NONE,
		PR_UDP,
		PR_HTTP,
		PR_PIPE
	};

private:
	class CPacket
	{
	public:
		BYTE*     m_buff;
		ULONGLONG m_start, m_end;

		CPacket(const BYTE* p, ULONGLONG start, UINT size);
		virtual ~CPacket() {
			delete [] m_buff;
		}
	};

	CCritSec           m_csLock;
	CCritSec           m_csPacketsLock;

	CUrl               m_url;
	CString            m_url_str;
	protocol           m_protocol   = protocol::PR_NONE;
	GUID               m_subtype    = MEDIASUBTYPE_NULL;
	DWORD              m_RequestCmd = 0;

	SOCKET             m_UdpSocket  = INVALID_SOCKET;
	WSAEVENT           m_WSAEvent   = NULL;
	sockaddr_in        m_addr;

	CHTTPAsync         m_HTTPAsync;

	ULONGLONG          m_pos = 0;
	ULONGLONG          m_len = 0;

	std::deque<CPacket*> m_packets;

	CAMEvent           m_EventComplete;

	volatile ULONGLONG m_SizeComplete = 0;
	volatile BOOL      m_bEndOfStream = FALSE;

	void Clear();
	void Append(const BYTE* buff, UINT len);

	inline const ULONGLONG GetPacketsSize();
	void CheckBuffer();
	void EmptyBuffer();

	DWORD ThreadProc();

public:
	CUDPStream();
	virtual ~CUDPStream();

	enum CMD {
		CMD_INIT,
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

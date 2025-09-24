/*
 * (C) 2020-2025 see Authors.txt
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
#include <ExtLib/AsyncReader/asyncio.h>
#include "DSUtil/HTTPAsync.h"
#include "AESDecryptor.h"

#include <chrono>

class CLiveStream
	: public CAsyncStream
	, public CAMThread
{
public:
	enum protocol {
		PR_NONE,
		PR_UDP,
		PR_HTTP,
		PR_PIPE,
		PR_HLS
	};

private:
	class CPacket
	{
	public:
		std::unique_ptr<BYTE[]> m_buff;
		ULONGLONG m_start, m_end;

		CPacket(const BYTE* p, ULONGLONG start, UINT size);
	};

	CCritSec           m_csLock;
	CCritSec           m_csPacketsLock;

	CStringW           m_url_str;
	protocol           m_protocol   = protocol::PR_NONE;
	GUID               m_subtype    = MEDIASUBTYPE_NULL;
	DWORD              m_RequestCmd = 0;

	SOCKET             m_UdpSocket  = INVALID_SOCKET;
	WSAEVENT           m_WSAEvent   = nullptr;
	sockaddr_in        m_addr;

	CHTTPAsync         m_HTTPAsync;

	ULONGLONG          m_pos = 0;
	ULONGLONG          m_len = 0;
	DWORD              m_nBytesRead = 0;

	std::deque<CPacket> m_packets;

	CAMEvent           m_EventComplete;

	volatile ULONGLONG m_SizeComplete = 0;
	volatile BOOL      m_bEndOfStream = FALSE;

	struct {
		DWORD metaint = 0;
		CStringW stationName;
		CStringW genre;
		CStringW description;
		CStringW stationUrl;
		CStringW streamTitle;
		//CStringW streamUrl;
	} m_icydata;

	struct hlsData_t {
		bool                bInit = {};

		std::deque<CStringW> Segments;
		std::list<CStringW>  DiscontinuitySegments;
		uint64_t             SequenceNumber = {};
		CStringW             PlaylistUrl;
		int64_t              PlaylistDuration = {};
		uint64_t             SegmentSize = {};
		uint64_t             SegmentPos = {};
		bool                 bEndList = {};
		bool                 bRunning = {};
		std::chrono::high_resolution_clock::time_point PlaylistParsingTime = {};

		bool                bAes128 = {};
		std::unique_ptr<CAESDecryptor> pAESDecryptor;

		bool                bEndOfSegment = {};
		[[nodiscard]] bool IsEndOfSegment() const { return bEndOfSegment || (SegmentSize && SegmentPos == SegmentSize); }
	} m_hlsData;

	void Clear();
	void Append(const BYTE* buff, UINT len);
	HRESULT HTTPRead(PBYTE pBuffer, DWORD dwSizeToRead, DWORD& dwSizeRead, DWORD dwTimeOut = INFINITE);

	inline const ULONGLONG GetPacketsSize();
	void CheckBuffer();
	void EmptyBuffer();

	DWORD ThreadProc();

	bool ParseM3U8(const CStringW& url, CStringW& realUrl);

	bool OpenHLSSegment();

public:
	CLiveStream() = default;
	virtual ~CLiveStream();

	enum CMD {
		CMD_INIT,
		CMD_RUN,
		CMD_STOP,
		CMD_EXIT
	};

	bool Load(const WCHAR* fnw);

	// CAsyncStream
	HRESULT SetPointer(LONGLONG llPos) override;
	HRESULT Read(PBYTE pbBuffer, DWORD dwBytesToRead, BOOL bAlign, LPDWORD pdwBytesRead) override;
	LONGLONG Size(LONGLONG* pSizeAvailable) override;
	DWORD Alignment() override;
	void Lock() override;
	void Unlock() override;

	GUID GetSubtype() const { return m_subtype; }
	protocol GetProtocol() const { return m_protocol; }

	CStringW CLiveStream::GetTitle() const;
	CStringW GetDescription() const { return m_icydata.description; }
	CStringW GetUrl() const { return m_icydata.stationUrl; }
};

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
#include "UDPReader.h"
#include <moreuuids.h>
#include "../../../DSUtil/DSUtil.h"

#include <fcntl.h>
#include <io.h>
#include <iostream>

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG2_TRANSPORT},
};

const AMOVIESETUP_PIN sudOpPin[] = {
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesOut), sudPinTypesOut}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CUDPReader), UDPReaderName, MERIT_NORMAL, _countof(sudOpPin), sudOpPin, CLSID_LegacyAmFilterCategory}
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CUDPReader>, NULL, &sudFilter[0]}
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	SetRegKeyValue(_T("udp"), 0, _T("Source Filter"), CStringFromGUID(__uuidof(CUDPReader)));
	SetRegKeyValue(_T("http"), 0, _T("Source Filter"), CStringFromGUID(__uuidof(CUDPReader)));

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	// TODO

	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

#define MAXSTORESIZE 10485760 // The maximum size of a buffer for storing the received information is 10 Mb
#define MAXBUFSIZE   16384    // The maximum packet size is 16 Kb

//
// CUDPReader
//

CUDPReader::CUDPReader(IUnknown* pUnk, HRESULT* phr)
	: CAsyncReader(NAME("CUDPReader"), pUnk, &m_stream, phr, __uuidof(this))
{
	if (phr) {
		*phr = S_OK;
	}

#ifndef REGISTER_FILTER
	AfxSocketInit();
#endif
}

CUDPReader::~CUDPReader()
{
}

STDMETHODIMP CUDPReader::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		QI(IFileSourceFilter)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CUDPReader::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	wcscpy_s(pInfo->achName, m_stream.GetProtocol() == CUDPStream::protocol::PR_PIPE ? STDInReaderName : UDPReaderName);
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

STDMETHODIMP CUDPReader::Stop()
{
	m_stream.CallWorker(m_stream.CMD_STOP);
	return S_OK;
}

STDMETHODIMP CUDPReader::Pause()
{
	m_stream.CallWorker(m_stream.CMD_PAUSE);
	return S_OK;
}

STDMETHODIMP CUDPReader::Run(REFERENCE_TIME tStart)
{
	m_stream.CallWorker(m_stream.CMD_RUN);
	return S_OK;
}

// IFileSourceFilter

STDMETHODIMP CUDPReader::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt)
{
	if (!m_stream.Load(pszFileName)) {
		return E_FAIL;
	}

	m_fn = pszFileName;

	CMediaType mt;
	mt.majortype = MEDIATYPE_Stream;
	mt.subtype = m_stream.GetSubtype();
	m_mt = mt;

	return S_OK;
}

STDMETHODIMP CUDPReader::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
{
	CheckPointer(ppszFileName, E_POINTER);

	size_t nCount = m_fn.GetLength() + 1;
	*ppszFileName = (LPOLESTR)CoTaskMemAlloc(nCount * sizeof(WCHAR));
	CheckPointer(*ppszFileName, E_OUTOFMEMORY);
	wcscpy_s(*ppszFileName, nCount, m_fn);

	return S_OK;
}

// CUDPStream

CUDPStream::CUDPStream()
	: m_protocol(PR_NONE)
	, m_UdpSocket(INVALID_SOCKET)
	, m_HttpSocketTread(INVALID_SOCKET)
	, m_pos(0)
	, m_len(0)
	, m_subtype(MEDIASUBTYPE_NULL)
	, m_RequestCmd(0)
	, m_EventComplete(TRUE)
{
	m_WSAEvent[0] = NULL;
}

CUDPStream::~CUDPStream()
{
	Clear();
}

void CUDPStream::Clear()
{
	if (CAMThread::ThreadExists()) {
		CAMThread::CallWorker(CMD_EXIT);
		CAMThread::Close();
	}

	if (m_WSAEvent[0] != NULL) {
		WSACloseEvent(m_WSAEvent[0]);
	}

	if (m_UdpSocket != INVALID_SOCKET) {
		closesocket(m_UdpSocket);
		m_UdpSocket = INVALID_SOCKET;
	}

	if (m_HttpSocketTread != INVALID_SOCKET) {
		m_HttpSocket.Attach(m_HttpSocketTread);
	}
	m_HttpSocket.Close();

	if (m_protocol == PR_UDP) { // ?
		WSACleanup();
	}

	while (!m_packets.IsEmpty()) {
		delete m_packets.RemoveHead();
	}

	m_pos = m_len = 0;
}

void CUDPStream::Append(BYTE* buff, int len)
{
	CAutoLock cPacketLock(&m_csPacketsLock);

	m_packets.AddTail(DNew packet_t(buff, m_len, len));
	m_len += len;
}

static void GetType(BYTE* buf, int size, GUID& subtype)
{
	if (size >= 188 && buf[0] == 0x47) {
		BOOL bIsMPEGTS = TRUE;
		// verify MPEG Stream
		for (int i = 188; i < size; i += 188) {
			if (buf[i] != 0x47) {
				bIsMPEGTS = FALSE;
				break;
			}
		}

		if (bIsMPEGTS) {
			subtype = MEDIASUBTYPE_MPEG2_TRANSPORT;
		}
	} else if (size > 6 && GETDWORD(buf) == 0xBA010000) {
		if ((buf[4] & 0xc4) == 0x44) {
			subtype = MEDIASUBTYPE_MPEG2_PROGRAM;
		} else if ((buf[4] & 0xf1) == 0x21) {
			subtype = MEDIASUBTYPE_MPEG1System;
		}
	} else if (size > 4 && GETDWORD(buf) == 'SggO') {
		subtype = MEDIASUBTYPE_Ogg;
	} else if (size > 4 && GETDWORD(buf) == 0xA3DF451A) {
		subtype = MEDIASUBTYPE_Matroska;
	} else if (size > 4 && GETDWORD(buf) == FCC('FLV\x1')) {
		subtype = MEDIASUBTYPE_FLV;
	} else if (size > 8 && GETDWORD(buf + 4) == FCC('ftyp')) {
		subtype = MEDIASUBTYPE_MP4;
	}
}

bool CUDPStream::Load(const WCHAR* fnw)
{
	Clear();

	m_url_str = CString(fnw);
	CString str_protocol;

	if (m_url_str.Find(L"pipe:") == 0) {
		str_protocol = L"pipe";
	} else {
		if (!m_url.CrackUrl(m_url_str)) {
			return false;
		}

		str_protocol = m_url.GetSchemeName();
		str_protocol.MakeLower();
	}

	if (str_protocol == L"udp") {
		m_protocol = PR_UDP;

		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);

		memset(&m_addr, 0, sizeof(m_addr));
		m_addr.sin_family      = AF_INET;
		m_addr.sin_port        = htons((u_short)m_url.GetPortNumber());
		m_addr.sin_addr.s_addr = htonl(INADDR_ANY);

		ip_mreq imr;
		imr.imr_multiaddr.s_addr = inet_addr(CStringA(m_url.GetHostName()));
		imr.imr_interface.s_addr = INADDR_ANY;

		if ((m_UdpSocket = socket(AF_INET, SOCK_DGRAM, 0)) != INVALID_SOCKET) {
			m_WSAEvent[0] = WSACreateEvent();
			WSAEventSelect(m_UdpSocket, m_WSAEvent[0], FD_READ);

			DWORD dw = TRUE;
			if (setsockopt(m_UdpSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&dw, sizeof(dw)) == SOCKET_ERROR) {
				closesocket(m_UdpSocket);
				m_UdpSocket = INVALID_SOCKET;
			}

			if (bind(m_UdpSocket, (struct sockaddr*)&m_addr, sizeof(m_addr)) == SOCKET_ERROR) {
				closesocket(m_UdpSocket);
				m_UdpSocket = INVALID_SOCKET;
			}

			if (IN_MULTICAST(htonl(imr.imr_multiaddr.s_addr))) {
				int ret = setsockopt(m_UdpSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&imr, sizeof(imr));
				if (ret < 0) {
					closesocket(m_UdpSocket);
					m_UdpSocket = INVALID_SOCKET;
				}
			}

			dw = MAXBUFSIZE;
			if (setsockopt(m_UdpSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&dw, sizeof(dw)) == SOCKET_ERROR) {
				;
			}

			// set non-blocking mode
			u_long param = 1;
			ioctlsocket(m_UdpSocket, FIONBIO, &param);
		}

		if (m_UdpSocket == INVALID_SOCKET) {
			return false;
		}

		UINT timeout = 0;
		while (timeout < 500) {
			DWORD res = WSAWaitForMultipleEvents(1, m_WSAEvent, FALSE, 100, FALSE);
			if (res == WSA_WAIT_EVENT_0) {
				int fromlen = sizeof(m_addr);
				char buf[MAXBUFSIZE];
				int len = recvfrom(m_UdpSocket, buf, MAXBUFSIZE, 0, (SOCKADDR*)&m_addr, &fromlen);
				if (len <= 0) {
					timeout += 100;
					continue;
				}
				break;
			} else {
				timeout += 100;
				continue;
			}

			break;
		}

		if (timeout == 500) {
			return false;
		}

	} else if (str_protocol == L"http") {
		m_protocol = PR_HTTP;

		if (m_url.GetUrlPathLength() == 0) {
			m_url.SetUrlPath(_T("/"));
			m_url_str += '/';
		}

		BOOL connected = FALSE;
		for (int k = 1; k <= 5; k++) {
			bool redir = false;

			CStringA hdr;

			if (!m_HttpSocket.Create()) {
				return false;
			}

			m_HttpSocket.SetUserAgent("MPC UDP/HTTP Reader");
			m_HttpSocket.SetTimeOut(3000, 3000);
			connected = m_HttpSocket.Connect(m_url);
			if (connected) {
				hdr = m_HttpSocket.GetHeader();
				DbgLog((LOG_TRACE, 3, "CUDPStream::Load() - HTTP hdr:\n%s", hdr));

				connected = !hdr.IsEmpty();
				hdr.MakeLower();
			}

			if (!connected) {
				return false;
			}

			CAtlList<CStringA> sl;
			Explode(hdr, sl, '\n');
			POSITION pos = sl.GetHeadPosition();
			while (pos) {
				CStringA& hdrline = sl.GetNext(pos);
				CAtlList<CStringA> sl2;
				Explode(hdrline, sl2, ':', 2);
				if (sl2.GetCount() != 2) {
					continue;
				}

				CStringA param = sl2.GetHead();
				CStringA value = sl2.GetTail();

				if (param == "content-type") {
					if (value == "application/octet-stream") {
						m_subtype = MEDIASUBTYPE_NULL; // "universal" subtype for most splitters

						BYTE buf[1024];
						int len = m_HttpSocket.Receive((LPVOID)&buf, sizeof(buf));
						if (len) {
							GetType(buf, len, m_subtype);
							Append(buf, len);
						}
					} else if (value == "application/x-ogg" || value == "application/ogg" || value == "audio/ogg") {
						m_subtype = MEDIASUBTYPE_Ogg;
					} else if (value == "video/webm") {
						m_subtype = MEDIASUBTYPE_Matroska;
					} else if (value == "video/mp4" || value == "video/3gpp") {
						m_subtype = MEDIASUBTYPE_MP4;
					} else if (value == "video/x-flv") {
						m_subtype = MEDIASUBTYPE_FLV;
					} else { // "text/html..." and other
						 // not supported content-type
						connected = FALSE;
					}
				} else if (param == "content-length") {
					// Not stream. This downloadable file. Here it is better to use "File Source (URL)".
					connected = FALSE;
				} else if (param == "location") {
					if (value.Left(7) == "http://") {
						if (value.Find('/', 8) == -1) { // looking for the beginning of URL path (find '/' after 'http://x')
							value += '/'; // if not found, set minimum URL path
						}
						if (m_url_str != CString(value)) {
							m_url_str = value;
							m_url.CrackUrl(m_url_str);
							redir = true;
						}
					}
				}
			}

			if (!redir) {
				break;
			}

			m_HttpSocket.Close();
		}

		if (!connected) {
			return false;
		}

		m_HttpSocketTread = m_HttpSocket.Detach();
	} else if (str_protocol == L"pipe") {
		if (_setmode(_fileno(stdin), _O_BINARY) == -1) {
			return false;
		}

		m_protocol = PR_PIPE;

		BYTE buf[1024] = { 0 };
		std::cin.read((char*)buf, sizeof(buf));
		std::streamsize len = std::cin.gcount();
		if (len) {
			GetType(buf, len, m_subtype);
			Append(buf, len);
		}
	} else {
		return false; // not supported protocol
	}

	CAMThread::Create();
	if (FAILED(CAMThread::CallWorker(CMD_INIT))) {
		Clear();
		return false;
	}

	clock_t start = clock();
	while (clock() - start < 1000 && m_len < MEGABYTE) {
		Sleep(100);
	}

	return true;
}

HRESULT CUDPStream::SetPointer(LONGLONG llPos)
{
	CAutoLock cAutoLock(&m_csLock);

	if (llPos < 0) {
		return E_FAIL;
	}

	m_pos = llPos;

	const __int64 start = m_packets.IsEmpty() ? 0 : m_packets.GetHead()->m_start;
	if (llPos < start) {
		DbgLog((LOG_TRACE, 3, L"CUDPStream::SetPointer() warning! %lld misses in [%I64d - %I64d]", llPos, start, m_packets.GetTail()->m_end));
		return S_FALSE;
	}

	return S_OK;
}

HRESULT CUDPStream::Read(PBYTE pbBuffer, DWORD dwBytesToRead, BOOL bAlign, LPDWORD pdwBytesRead)
{
	DWORD len = dwBytesToRead;
	BYTE* ptr = pbBuffer;

	if (!m_packets.IsEmpty()
			&& m_pos + len > m_packets.GetTail()->m_end) {
		while (!m_EventComplete.Check() && !m_packets.IsEmpty() && m_pos + len > m_packets.GetTail()->m_end) {
			DbgLog((LOG_TRACE, 3, L"CUDPStream::Read() - wait %I64d bytes, %I64d -> %I64d", m_pos + len - m_packets.GetTail()->m_end, m_packets.GetTail()->m_end, m_pos + len));
			Sleep(500);
		}

		m_EventComplete.Reset();
	}

	CAutoLock cAutoLock(&m_csLock);
	CAutoLock cPacketLock(&m_csPacketsLock);

	POSITION pos = m_packets.GetHeadPosition();
	while (pos && len > 0) {
		packet_t* p = m_packets.GetNext(pos);

		if (p->m_start <= m_pos && m_pos < p->m_end) {
			DWORD size;

			if (m_pos < p->m_start) {
				ASSERT(0);
				size = (DWORD)min(len, p->m_start - m_pos);
				memset(ptr, 0, size);
			} else {
				size = (DWORD)min(len, p->m_end - m_pos);
				memcpy(ptr, &p->m_buff[m_pos - p->m_start], size);
			}

			m_pos += size;

			ptr += size;
			len -= size;
		}
	}

	if (pdwBytesRead) {
		*pdwBytesRead = ptr - pbBuffer;
	}

	CheckBuffer();

	return len == dwBytesToRead ? E_FAIL : (len > 0 ? S_FALSE : S_OK);
}

LONGLONG CUDPStream::Size(LONGLONG* pSizeAvailable)
{
	CAutoLock cAutoLock(&m_csLock);

	if (pSizeAvailable) {
		*pSizeAvailable = m_len;
	}

	return 0;
}

DWORD CUDPStream::Alignment()
{
	return 1;
}

void CUDPStream::Lock()
{
	m_csLock.Lock();
}

void CUDPStream::Unlock()
{
	m_csLock.Unlock();
}

inline __int64 CUDPStream::GetPacketsSize()
{
	CAutoLock cPacketLock(&m_csPacketsLock);

	return m_packets.IsEmpty() ? 0 : m_packets.GetTail()->m_end - m_packets.GetHead()->m_start;
}

void CUDPStream::CheckBuffer()
{
	CAutoLock cPacketLock(&m_csPacketsLock);

	if (m_RequestCmd != CMD_RUN) {
		return;
	}

	while (!m_packets.IsEmpty() && m_packets.GetHead()->m_start < m_pos - 256 * KILOBYTE) {
		delete m_packets.RemoveHead();
	}
}

DWORD CUDPStream::ThreadProc()
{
	SetThreadPriority(m_hThread, THREAD_PRIORITY_TIME_CRITICAL);

	if (m_protocol == PR_HTTP) {
		AfxSocketInit();
	}

#ifdef _DEBUG
	FILE* dump = NULL;
	//dump = _tfopen(_T("c:\\http.ts"), _T("wb"));
#endif

	for (;;) {
		m_RequestCmd = GetRequest();

		switch (m_RequestCmd) {
			default:
			case CMD_EXIT:
				m_EventComplete.Set();
				Reply(S_OK);
				if (m_protocol == PR_HTTP) {
					m_HttpSocketTread = m_HttpSocket.Detach();
				}
#ifdef _DEBUG
				if (dump) { fclose(dump); }
#endif
				return 0;
			case CMD_STOP:
				m_EventComplete.Set();
				Reply(S_OK);
				break;
			case CMD_INIT:
				if (m_protocol == PR_HTTP) {
					m_HttpSocket.Attach(m_HttpSocketTread);
				}
			case CMD_PAUSE:
			case CMD_RUN:
				Reply(S_OK);
				char  buff[MAXBUFSIZE * 2];
				int   buffsize = 0;
				UINT  attempts = 0;

				while (!CheckRequest(NULL) && attempts < 200) {
					int len = 0;

					if (m_protocol == PR_UDP) {
						DWORD res = WSAWaitForMultipleEvents(1, m_WSAEvent, FALSE, 50, FALSE);
						if (res == WSA_WAIT_EVENT_0) {
							WSAResetEvent(m_WSAEvent[0]);
							int fromlen = sizeof(m_addr);
							len = recvfrom(m_UdpSocket, &buff[buffsize], MAXBUFSIZE, 0, (SOCKADDR*)&m_addr, &fromlen);
							if (len <= 0) {
								const int wsaLastError = WSAGetLastError();
								if (wsaLastError != WSAEWOULDBLOCK) {
									attempts++;
								}
								continue;
							}
						} else {
							attempts++;
							continue;
						}
					} else if (m_protocol == PR_HTTP) {
						len = m_HttpSocket.Receive(&buff[buffsize], MAXBUFSIZE);
						if (len == 0) {
							attempts++;
							Sleep(50);
							continue;
						} else if (len == SOCKET_ERROR) {
							attempts += 60;
							continue;
						}
						attempts = 0;
					} else if (m_protocol == PR_PIPE) {
						std::cin.read(&buff[buffsize], MAXBUFSIZE);
						if (std::cin.fail() || std::cin.bad()) {
							std::cin.clear();
							len = 0;
							attempts += 60;
							continue;
						} else if (std::cin.eof()) {
							break;
						} else {
							len = std::cin.gcount();
							if (len == 0) {
								attempts++;
								Sleep(50);
								continue;
							}
						}
					}

					buffsize += len;
					if (buffsize >= MAXBUFSIZE) {
#ifdef _DEBUG
						if (dump) { fwrite(buff, buffsize, 1, dump); }
#endif
						Append((BYTE*)buff, buffsize);
						buffsize = 0;
					}

					while (GetPacketsSize() > MAXSTORESIZE && !CheckRequest(NULL)) {
						Sleep(1000);
					}
				}
				break;
		}
	}

	ASSERT(0);
	return (DWORD)-1;
}

CUDPStream::packet_t::packet_t(BYTE* p, __int64 start, int size)
	: m_start(start)
	, m_end(start + size)
{
	ASSERT(size > 0);
	m_buff = DNew BYTE[size];
	memcpy(m_buff, p, size);
}

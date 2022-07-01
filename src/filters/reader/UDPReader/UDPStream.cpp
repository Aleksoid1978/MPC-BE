/*
 * (C) 2017-2022 see Authors.txt
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
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <Ws2tcpip.h>
#include "UDPStream.h"
#include <moreuuids.h>
#include "DSUtil/DSUtil.h"
#include "DSUtil/UrlParser.h"
#include "DSUtil/entities.h"
#include "Subtitles/TextFile.h"

#define RAPIDJSON_SSE2
#include <ExtLib/rapidjson/include/rapidjson/document.h>

#define MAXSTORESIZE  2 * MEGABYTE // The maximum size of a buffer for storing the received information is 2 Mb
#define MAXBUFSIZE   16 * KILOBYTE // The maximum packet size is 16 Kb

// CUDPStream

CUDPStream::~CUDPStream()
{
	Clear();
}

void CUDPStream::Clear()
{
	if (CAMThread::ThreadExists()) {
		CAMThread::CallWorker(CMD::CMD_EXIT);
		CAMThread::Close();
	}

	if (m_protocol == protocol::PR_UDP) {
		if (m_WSAEvent != nullptr) {
			WSACloseEvent(m_WSAEvent);
		}

		if (m_UdpSocket != INVALID_SOCKET) {
			closesocket(m_UdpSocket);
			m_UdpSocket = INVALID_SOCKET;
		}

		WSACleanup();
	} else if (m_protocol == protocol::PR_HLS) {
		m_HTTPAsync.Close();
		m_hlsData.Segments.clear();
		m_hlsData.DiscontinuitySegments.clear();
		m_hlsData.SequenceNumber = {};
		m_hlsData.SegmentDuration = {};
		m_hlsData.bInit = {};

		m_hlsData.SegmentPos = m_hlsData.SegmentSize = {};
	}

	m_HTTPAsync.Close();

	EmptyBuffer();

	m_pos = m_len = 0;
	m_SizeComplete = 0;
}

void CUDPStream::Append(const BYTE* buff, UINT len)
{
	CAutoLock cPacketLock(&m_csPacketsLock);

	m_packets.emplace_back(buff, m_len, len);
	m_len += len;

	if (m_SizeComplete && m_len >= m_SizeComplete) {
		m_EventComplete.Set();
	}
}

HRESULT CUDPStream::HTTPRead(PBYTE pBuffer, DWORD dwSizeToRead, LPDWORD dwSizeRead, DWORD dwTimeOut/* = INFINITE*/)
{
	if (m_protocol == protocol::PR_HTTP) {
		if (m_icydata.metaint == 0) {
			return m_HTTPAsync.Read(pBuffer, dwSizeToRead, dwSizeRead, dwTimeOut);
		}

		if (m_nBytesRead + dwSizeToRead > m_icydata.metaint) {
			dwSizeToRead = m_icydata.metaint - m_nBytesRead;
		}

		HRESULT hr = m_HTTPAsync.Read(pBuffer, dwSizeToRead, dwSizeRead, dwTimeOut);
		if (FAILED(hr)) {
			return hr;
		}

		if ((m_nBytesRead += *dwSizeRead) == m_icydata.metaint) {
			m_nBytesRead = 0;

			static BYTE buff[255 * 16], b = 0;
			ZeroMemory(buff, sizeof(buff));

			DWORD _dwSizeRead = 0;
			if (S_OK == m_HTTPAsync.Read(&b, 1, &_dwSizeRead) && _dwSizeRead == 1 && b) {
				if (S_OK == m_HTTPAsync.Read(buff, b * 16, &_dwSizeRead) && _dwSizeRead == b * 16) {
					int len = decode_html_entities_utf8((char*)buff, nullptr);

					CString str = UTF8orLocalToWStr((LPCSTR)buff);

					DLog(L"CUDPStream::HTTPRead(): Metainfo: %s", str);

					int i = str.Find(L"StreamTitle='");
					if (i >= 0) {
						i += 13;
						int j = str.Find(L"';", i);
						if (!j) {
							j = str.ReverseFind('\'');
						}
						if (j > i) {
							m_icydata.name = str.Mid(i, j - i);

							// special code for 101.ru - it's use json format in MetaInfo
							if (!m_icydata.name.IsEmpty() && m_icydata.name.GetAt(0) == L'{') {
								CString tmp(m_icydata.name);
								const auto pos = tmp.ReverseFind(L'}');
								if (pos > 0) {
									tmp.Delete(pos + 1, tmp.GetLength() - pos);

									rapidjson::GenericDocument<rapidjson::UTF16<>> d;
									if (!d.Parse(tmp.GetString()).HasParseError()) {
										if (d.HasMember(L"t") && d[L"t"].IsString()) {
											m_icydata.name = d[L"t"].GetString();
										}
										else if (d.HasMember(L"title") && d[L"title"].IsString()) {
											m_icydata.name = d[L"title"].GetString();
										}
									}
								}
							}
						}
					} else {
						DLog(L"CUDPStream::HTTPRead(): StreamTitle is missing");
					}

					i = str.Find(L"StreamUrl='");
					if (i >= 0) {
						i += 11;
						int j = str.Find(L"';", i);
						if (!j) {
							j = str.ReverseFind('\'');
						}
						if (j > i) {
							str = str.Mid(i, j - i);
							if (!str.IsEmpty()) {
								m_icydata.url = str;
							}
						}
					}
				}
			}
		}

		return S_OK;
	}

	return E_FAIL;
}

static void GetType(const BYTE* buf, int size, GUID& subtype)
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
	} else if (size > 6 && GETU32(buf) == 0xBA010000) {
		if ((buf[4] & 0xc4) == 0x44) {
			subtype = MEDIASUBTYPE_MPEG2_PROGRAM;
		} else if ((buf[4] & 0xf1) == 0x21) {
			subtype = MEDIASUBTYPE_MPEG1System;
		}
	} else if (size > 4 && GETU32(buf) == 'SggO') {
		subtype = MEDIASUBTYPE_Ogg;
	} else if (size > 4 && GETU32(buf) == 0xA3DF451A) {
		subtype = MEDIASUBTYPE_Matroska;
	} else if (size > 4 && GETU32(buf) == FCC('FLV\x1')) {
		subtype = MEDIASUBTYPE_FLV;
	} else if (size > 8 && GETU32(buf + 4) == FCC('ftyp')) {
		subtype = MEDIASUBTYPE_MP4;
	}
}

bool CUDPStream::ParseM3U8(const CString& url, CString& realUrl)
{
	CWebTextFile f(CTextFile::UTF8, CTextFile::ANSI);
	if (!f.Open(url)) {
		return false;
	}

	CString str;
	if (!f.ReadString(str) || str != L"#EXTM3U") {
		return false;
	}

	realUrl = url;

	CString base(url);
	base.Truncate(base.ReverseFind('/'));

	bool bMediaSequence = false;
	uint64_t sequenceNumber = {};

	bool bDiscontinuity = false;

	int32_t bandwidth = {};
	std::list<std::pair<uint32_t, CString>> PlaylistItems;

	m_hlsData.SegmentDuration = {};

	while (f.ReadString(str)) {
		FastTrim(str);

		if (str.IsEmpty()) {
			continue;
		}

		auto DeleteLeft = [](const auto pos, auto& str) {
			str = str.Mid(pos, str.GetLength() - pos);
			str.TrimLeft();
		};

		if (StartsWith(str, L"#EXT-X-TARGETDURATION:")) {
			DeleteLeft(22, str);
			StrToInt64(str, m_hlsData.SegmentDuration);
			continue;
		} else if (StartsWith(str, L"#EXT-X-MEDIA-SEQUENCE:")) {
			DeleteLeft(22, str);
			if (StrToUInt64(str, sequenceNumber)) {
				bMediaSequence = true;
			}
			continue;
		} else if (str == L"#EXT-X-DISCONTINUITY") {
			bDiscontinuity = true;
			continue;
		} else if (str == L"#EXT-X-ENDLIST") {
			m_hlsData.bEndList = true;
			continue;
		} else if (StartsWith(str, L"#EXT-X-STREAM-INF:")) {
			auto pos = str.Find(L"BANDWIDTH=");
			if (pos > 0) {
				DeleteLeft(pos + 10, str);
				StrToInt32(str, bandwidth);
			};
			continue;
		} else if (StartsWith(str, L"#EXT-X-KEY:")) {
			if (!m_hlsData.bInit) {
				DeleteLeft(11, str);

				CString method, uri, iv;

				std::list<CString> attributes;
				Explode(str, attributes, L',');
				for (const auto& attribute : attributes) {
					const auto pos = attribute.Find(L'=');
					if (pos > 0) {
						const auto key = attribute.Left(pos);
						const auto value = attribute.Mid(pos + 1);
						if (key == L"METHOD") {
							method = value;
						} else if (key == L"URI") {
							uri = value;
						} else if (key == L"IV") {
							iv = value;
						}
					}
				}

				if (method == L"NONE") {
					continue;
				} else if (method == L"AES-128") {
					m_hlsData.pAESDecryptor.reset(DNew CAESDecryptor());
					if (!m_hlsData.pAESDecryptor->IsInitialized()) {
						return false;
					}
					if (!uri.IsEmpty()) {
						uri.Trim(L'"');
						CHTTPAsync http;
						if (SUCCEEDED(http.Connect(uri))) {
							if (http.GetLenght() != CAESDecryptor::AESBlockSize) {
								DLog(L"CUDPStream::ParseM3U8() : wrong AES key.");
								return false;
							}
							BYTE key[CAESDecryptor::AESBlockSize] = {};
							DWORD dwSizeRead = 0;
							if (SUCCEEDED(http.Read(key, CAESDecryptor::AESBlockSize, &dwSizeRead))) {
								std::vector<BYTE> pIV;
								if (!iv.IsEmpty()) {
									iv.Delete(0, 2);
									if (iv.GetLength() != (CAESDecryptor::AESBlockSize * 2)) {
										DLog(L"CUDPStream::ParseM3U8() : wrong AES IV.");
										return false;
									}
									CStringToBin(iv, pIV);
								}

								if (!m_hlsData.pAESDecryptor->SetKey(key, std::size(key), pIV.data(), pIV.size())) {
									DLog(L"CUDPStream::ParseM3U8() : can't initialize decrypting engine.");
									return false;
								}
								m_hlsData.bAes128 = true;

								continue;
							}
						}
					}
				}

				DLog(L"CUDPStream::ParseM3U8() : not supported encrypted playlist.");
				return false;
			}
			continue;
		} else if (str.GetAt(0) == L'#') {
			continue;
		}

		auto CombinePath = [](const CString& base, const CString& path) {
			CUrlParser urlParser(path.GetString());
			if (urlParser.IsValid()) {
				return path;
			}

			CString output(base);
			if (path.GetAt(0) == L'/') {
				output.Append(path);
			} else {
				output.Append(L'/' + path);
			}

			return output;
		};

		if (!bMediaSequence) {
			PlaylistItems.emplace_back(bandwidth, CombinePath(base, str));
			bandwidth = {};
		} else {
			if (bDiscontinuity) {
				auto fullUrl = CombinePath(base, str);
				auto it = std::find(m_hlsData.DiscontinuitySegments.cbegin(), m_hlsData.DiscontinuitySegments.cend(), fullUrl);
				if (it == m_hlsData.DiscontinuitySegments.cend()) {
					m_hlsData.Segments.emplace_back(fullUrl);
					m_hlsData.DiscontinuitySegments.emplace_back(fullUrl);
				}
			} else {
				if (bMediaSequence && sequenceNumber > m_hlsData.SequenceNumber) {
					m_hlsData.Segments.emplace_back(CombinePath(base, str));
					m_hlsData.SequenceNumber = sequenceNumber;
				}
				sequenceNumber++;
			}
		}
	}

	if (bDiscontinuity) {
		m_hlsData.SequenceNumber = {};
	} else {
		m_hlsData.DiscontinuitySegments.clear();
	}

	m_hlsData.PlaylistParsingTime = std::chrono::high_resolution_clock::now();

	if (m_hlsData.SegmentDuration && (bMediaSequence || bDiscontinuity)) {
		m_hlsData.bInit = true;
		return !m_hlsData.Segments.empty();
	}

	if (!PlaylistItems.empty()) {
		PlaylistItems.sort([](const auto& itemleft, const auto& itemright) {
			return itemleft.first > itemright.first;
		});
		CString newUrl = PlaylistItems.front().second;
		return ParseM3U8(newUrl, realUrl);
	}

	return false;
}

bool CUDPStream::OpenHLSSegment()
{
	if (!m_hlsData.Segments.empty()) {
		if (SUCCEEDED(m_HTTPAsync.Connect(m_hlsData.Segments.front()))) {
			m_hlsData.Segments.pop_front();
			m_hlsData.SegmentSize = m_HTTPAsync.GetLenght();
			m_hlsData.SegmentPos = {};
			return true;
		}
	}

	return false;
}

bool CUDPStream::Load(const WCHAR* fnw)
{
	Clear();

	m_url_str = fnw;
	CUrlParser urlParser;
	CString str_protocol;

	if (StartsWith(m_url_str, L"pipe:")) {
		str_protocol = L"pipe";
	} else {
		if (!urlParser.Parse(fnw)) {
			return false;
		}

		str_protocol = urlParser.GetSchemeName();
	}

	if (str_protocol == L"udp") {
		m_protocol = protocol::PR_UDP;

		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);

		int m_addr_size = sizeof(m_addr);

		memset(&m_addr, 0, m_addr_size);
		m_addr.sin_family      = AF_INET;
		m_addr.sin_port        = htons((u_short)urlParser.GetPortNumber());
		m_addr.sin_addr.s_addr = htonl(INADDR_ANY);

		ip_mreq imr;
		if (InetPton(AF_INET, urlParser.GetHostName(), &imr.imr_multiaddr.s_addr) != 1) {
			return false;
		}
		imr.imr_interface.s_addr = INADDR_ANY;

		if ((m_UdpSocket = socket(AF_INET, SOCK_DGRAM, 0)) != INVALID_SOCKET) {
			m_WSAEvent = WSACreateEvent();
			WSAEventSelect(m_UdpSocket, m_WSAEvent, FD_READ);

			DWORD dw = 1;
			if (setsockopt(m_UdpSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&dw, sizeof(dw)) == SOCKET_ERROR) {
				closesocket(m_UdpSocket);
				m_UdpSocket = INVALID_SOCKET;
			}

			if (bind(m_UdpSocket, (struct sockaddr*)&m_addr, m_addr_size) == SOCKET_ERROR) {
				closesocket(m_UdpSocket);
				m_UdpSocket = INVALID_SOCKET;
			}

			if (IN_MULTICAST(htonl(imr.imr_multiaddr.s_addr))
					&& (setsockopt(m_UdpSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&imr, sizeof(imr)) == SOCKET_ERROR)) {
				closesocket(m_UdpSocket);
				m_UdpSocket = INVALID_SOCKET;
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
			DWORD res = WSAWaitForMultipleEvents(1, &m_WSAEvent, FALSE, 100, FALSE);
			if (res == WSA_WAIT_EVENT_0) {
				WSAResetEvent(m_WSAEvent);
				auto buf = std::make_unique<BYTE[]>(MAXBUFSIZE);
				int len = recvfrom(m_UdpSocket, (PCHAR)buf.get(), MAXBUFSIZE, 0, (SOCKADDR*)&m_addr, &m_addr_size);
				if (len <= 0) {
					timeout += 100;
					continue;
				}
				GetType(buf.get(), len, m_subtype);
				Append(buf.get(), len);
				break;
			} else {
				timeout += 100;
				continue;
			}
		}

		if (timeout == 500) {
			return false;
		}
	} else if (str_protocol == L"http" || str_protocol == L"https") {
		m_protocol = protocol::PR_HTTP;
		HRESULT hr = m_HTTPAsync.Connect(m_url_str, 10000, L"Icy-MetaData:1\r\n");
		if (FAILED(hr)) {
			return false;
		}

		DLog(L"CUDPStream::Load() - HTTP content type: %s", m_HTTPAsync.GetContentType());

		BOOL bConnected = FALSE;

		if (!m_HTTPAsync.GetLenght()) { // only streams without content length
			BOOL bIcyFound = FALSE;
			const CString& hdr = m_HTTPAsync.GetHeader();
			DLog(L"CUDPStream::Load() - HTTP hdr:\n%s", hdr);

			std::list<CString> sl;
			Explode(hdr, sl, '\n');

			for (const auto& hdrline : sl) {
				CString param, value;
				int k = hdrline.Find(':');
				if (k > 0 && k + 1 < hdrline.GetLength()) {
					param = hdrline.Left(k).Trim().MakeLower();
					value = hdrline.Mid(k + 1).Trim();
				}

				if (!bIcyFound && param.Find(L"icy-") == 0) {
					bIcyFound = TRUE;
				}

				if (param == "icy-metaint") {
					m_icydata.metaint = static_cast<DWORD>(_wtol(value));
				} else if (param == "icy-name") {
					m_icydata.name = value;
				} else if (param == "icy-genre") {
					m_icydata.genre = value;
				} else if (param == "icy-url") {
					m_icydata.url = value;
				} else if (param == "icy-description") {
					m_icydata.description = value;
				}
			}

			bConnected = TRUE;
			CString contentType = m_HTTPAsync.GetContentType();
			contentType.MakeLower();
			if (contentType == L"application/octet-stream"
					|| contentType == L"video/unknown" || contentType == L"none"
					|| contentType.IsEmpty()) {
				BYTE buf[1024] = {};
				DWORD dwSizeRead = 0;
				if (HTTPRead(buf, sizeof(buf), &dwSizeRead) == S_OK && dwSizeRead) {
					GetType(buf, dwSizeRead, m_subtype);
					Append(buf, dwSizeRead);
				}
			} else if (contentType == L"video/mp2t" || contentType == L"video/mpeg") {
				m_subtype = MEDIASUBTYPE_MPEG2_TRANSPORT;
			} else if (contentType == L"application/x-ogg" || contentType == L"application/ogg" || contentType == L"audio/ogg") {
				m_subtype = MEDIASUBTYPE_Ogg;
			} else if (contentType == L"video/webm") {
				m_subtype = MEDIASUBTYPE_Matroska;
			} else if (contentType == L"video/mp4" || contentType == L"video/3gpp") {
				m_subtype = MEDIASUBTYPE_MP4;
			} else if (contentType == L"video/x-flv") {
				m_subtype = MEDIASUBTYPE_FLV;
			} else if (contentType.Find(L"audio/") == 0) {
				m_subtype = MEDIASUBTYPE_MPEG1Audio;
			} else if (!bIcyFound) { // other ...
					// not supported content-type
				bConnected = FALSE;
			}
		}

		if (!bConnected && (m_HTTPAsync.GetLenght() || m_HTTPAsync.GetContentType().Find(L"mpegurl") > 0)) {
			DLog(L"CUDPStream::Load() - HTTP hdr:\n%s", m_HTTPAsync.GetHeader());

			BYTE body[8] = {};
			DWORD dwSizeRead = 0;
			if (m_HTTPAsync.Read(body, 7, &dwSizeRead) == S_OK) {
				if (memcmp(body, "#EXTM3U", 7) == 0) {
					ParseM3U8(m_url_str, m_hlsData.PlaylistUrl);
				}
			}
			m_HTTPAsync.Close();

			if (!m_hlsData.Segments.empty()) {
				m_subtype = MEDIASUBTYPE_MPEG2_TRANSPORT;
				bConnected = TRUE;
				m_protocol = protocol::PR_HLS;

				if (!OpenHLSSegment()) {
					bConnected = FALSE;
				}
			}
		}

		if (!bConnected) {
			return false;
		}
	} else if (str_protocol == L"pipe") {
		if (_setmode(_fileno(stdin), _O_BINARY) == -1) {
			return false;
		}

		m_protocol = protocol::PR_PIPE;

		BYTE buf[1024] = {};
		std::cin.read((PCHAR)buf, sizeof(buf));
		std::streamsize len = std::cin.gcount();
		if (len) {
			GetType(buf, len, m_subtype);
			Append(buf, len);
		}
	} else {
		return false; // not supported protocol
	}

	CAMThread::Create();
	if (FAILED(CAMThread::CallWorker(CMD::CMD_INIT))) {
		Clear();
		return false;
	}

	const clock_t start = clock();
	while (clock() - start < 500 && m_len < 64 * KILOBYTE) {
		Sleep(50);
	}

	return true;
}

// CAsyncStream

HRESULT CUDPStream::SetPointer(LONGLONG llPos)
{
	CAutoLock cAutoLock(&m_csLock);

	if (llPos < 0) {
		return E_FAIL;
	}

	m_pos = llPos;

	const __int64 start = m_packets.empty() ? 0 : m_packets.front().m_start;
	if (llPos < start) {
		DLog(L"CUDPStream::SetPointer() warning! %lld misses in [%llu - %llu]", llPos, start, m_packets.back().m_end);
		return S_FALSE;
	}

	return S_OK;
}

HRESULT CUDPStream::Read(PBYTE pbBuffer, DWORD dwBytesToRead, BOOL bAlign, LPDWORD pdwBytesRead)
{
	DWORD len = dwBytesToRead;
	BYTE* ptr = pbBuffer;

	if (!m_packets.empty()
			&& m_pos + len > m_packets.back().m_end) {
		m_SizeComplete = m_pos + len;

#if DEBUG
		DLog(L"CUDPStream::Read() : wait %llu bytes, %llu -> %llu", m_SizeComplete - m_packets.back().m_end, m_packets.back().m_end, m_SizeComplete);
		const ULONGLONG start = GetPerfCounter();
#endif

		while (!m_bEndOfStream && !m_EventComplete.Wait(1));
		m_SizeComplete = 0;

#if DEBUG
		const ULONGLONG end = GetPerfCounter();
		DLog(L"    => do wait %0.3f ms", (end - start) / 10000.0);
#endif

		if (m_bEndOfStream) {
			if (pdwBytesRead) {
				*pdwBytesRead = 0;
			}

			return E_FAIL;
		}
	}

	CAutoLock cAutoLock(&m_csLock);
	CAutoLock cPacketLock(&m_csPacketsLock);

	auto it = m_packets.cbegin();
	while (it != m_packets.cend() && len > 0) {
		const auto& p = *it++;

		DLogIf(m_pos < p.m_start, L"CUDPStream::Read(): requested data is no longer available, %llu - %llu", m_pos, p.m_start);
		if (p.m_start <= m_pos && m_pos < p.m_end) {
			const DWORD size = std::min<DWORD>((ULONGLONG)len, p.m_end - m_pos);
			memcpy(ptr, &p.m_buff.get()[m_pos - p.m_start], size);

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

inline const ULONGLONG CUDPStream::GetPacketsSize()
{
	CAutoLock cPacketLock(&m_csPacketsLock);

	return m_packets.empty() ? 0 : m_packets.back().m_end - m_packets.front().m_start;
}

void CUDPStream::CheckBuffer()
{
	if (m_RequestCmd == CMD::CMD_RUN) {
		CAutoLock cPacketLock(&m_csPacketsLock);

		if (m_pos > 256 * KILOBYTE) {
			while (!m_packets.empty() && m_packets.front().m_start < m_pos - 256 * KILOBYTE) {
				m_packets.pop_front();
			}
		}
	}
}

void CUDPStream::EmptyBuffer()
{
	CAutoLock cPacketLock(&m_csPacketsLock);

	m_packets.clear();
	m_len = m_pos;
}

#define ENABLE_DUMP 0

DWORD CUDPStream::ThreadProc()
{
	SetThreadPriority(m_hThread, THREAD_PRIORITY_TIME_CRITICAL);

#if ENABLE_DUMP
	const CString fname = m_protocol == protocol::PR_HTTP ? L"http.dump" : m_protocol == protocol::PR_UDP ? L"udp.dump" : L"stdin.dump";
	FILE* dump = _wfopen(fname, L"wb");
#endif

	for (;;) {
		m_RequestCmd = GetRequest();

		switch (m_RequestCmd) {
			default:
			case CMD::CMD_EXIT:
				Reply(S_OK);
#if ENABLE_DUMP
				if (dump) {
					fclose(dump);
				}
#endif
				m_bEndOfStream = TRUE;
				EmptyBuffer();
				return 0;
			case CMD::CMD_STOP:
				Reply(S_OK);

				if (m_protocol == protocol::PR_HLS) {
					m_HTTPAsync.Close();
					m_hlsData.Segments.clear();
					m_hlsData.DiscontinuitySegments.clear();
					m_hlsData.SequenceNumber = {};
				}
				break;
			case CMD::CMD_INIT:
			case CMD::CMD_RUN:
				Reply(S_OK);

				auto buff = std::make_unique<BYTE[]>(MAXBUFSIZE * 2);
				int  buffsize = 0;
				UINT attempts = 0;
				int  len      = 0;

				BOOL bEndOfStream = FALSE;

				if (m_protocol == protocol::PR_HLS) {
					if (m_hlsData.Segments.empty()) {
						if (m_hlsData.bEndList || !ParseM3U8(m_hlsData.PlaylistUrl, m_hlsData.PlaylistUrl)) {
							m_bEndOfStream = TRUE;
							break;
						}
						if (!OpenHLSSegment()) {
							bEndOfStream = TRUE;
							break;
						}
					}
				}

				while (!CheckRequest(nullptr)
						&& attempts < 200 && !bEndOfStream) {

					if (!m_SizeComplete && GetPacketsSize() > MAXSTORESIZE) {
						Sleep(50);
						continue;
					}

					len = 0;
					if (m_protocol == protocol::PR_UDP) {
						DWORD res = WSAWaitForMultipleEvents(1, &m_WSAEvent, FALSE, 100, FALSE);
						if (res == WSA_WAIT_EVENT_0) {
							WSAResetEvent(m_WSAEvent);
							static int fromlen = sizeof(m_addr);
							len = recvfrom(m_UdpSocket, (PCHAR)&buff[buffsize], MAXBUFSIZE, 0, (SOCKADDR*)&m_addr, &fromlen);
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
					} else if (m_protocol == protocol::PR_HTTP) {
						DWORD dwSizeRead = 0;
						const HRESULT hr = HTTPRead(&buff[buffsize], MAXBUFSIZE, &dwSizeRead);
						if (FAILED(hr)) {
							attempts += 50;
							continue;
						} else if (hr == S_FALSE) {
							attempts++;
							Sleep(50);
							continue;
						} else if (dwSizeRead == 0) {
							bEndOfStream = TRUE;
							break;
						}
						len = dwSizeRead;
					} else if (m_protocol == protocol::PR_PIPE) {
						std::cin.read((PCHAR)&buff[buffsize], MAXBUFSIZE);
						if (std::cin.fail() || std::cin.bad()) {
							std::cin.clear();
							attempts += 50;
							continue;
						} else if (std::cin.eof()) {
							bEndOfStream = TRUE;
							break;
						} else {
							len = std::cin.gcount();
							if (len == 0) {
								attempts++;
								Sleep(50);
								continue;
							}
						}
					} else if (m_protocol == protocol::PR_HLS) {
						if (!m_hlsData.bEndList) {
							auto now = std::chrono::high_resolution_clock::now();
							auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now - m_hlsData.PlaylistParsingTime).count();
							if (seconds > static_cast<int64_t>(m_hlsData.SegmentDuration)) {
								ParseM3U8(m_hlsData.PlaylistUrl, m_hlsData.PlaylistUrl);
							}
						}

						DWORD dwSizeRead = 0;
						m_HTTPAsync.Read(&buff[buffsize], MAXBUFSIZE, &dwSizeRead);
						if (dwSizeRead == 0) {
							m_HTTPAsync.Close();

							if (m_hlsData.Segments.empty()) {
								if (m_hlsData.bEndList) {
									bEndOfStream = TRUE;
									break;
								}
								attempts++;
								Sleep(50);
							} else {
								if (!OpenHLSSegment()) {
									bEndOfStream = TRUE;
									break;
								}
							}

							continue;
						}

						len = dwSizeRead;
						if (m_hlsData.bAes128) {
							m_hlsData.SegmentPos += dwSizeRead;

							size_t decryptedSize = {};
							if (m_hlsData.pAESDecryptor->Decrypt(&buff[buffsize], dwSizeRead, m_hlsData.DecryptedData, decryptedSize, m_hlsData.SegmentSize == m_hlsData.SegmentPos)) {
								len = decryptedSize;
								memcpy(&buff[buffsize], m_hlsData.DecryptedData.data(), decryptedSize);
							} else {
								len = 0;
							}
						}
					}

					attempts = 0;
					buffsize += len;
					if (buffsize >= MAXBUFSIZE) {
#if ENABLE_DUMP
						if (dump) {
							fwrite(buff, buffsize, 1, dump);
						}
#endif
						Append(buff.get(), (UINT)buffsize);
						buffsize = 0;
					}
				}

				if (attempts >= 200 || bEndOfStream) {
					m_bEndOfStream = TRUE;
				}

				break;
		}
	}

	ASSERT(0);
	return DWORD_MAX;
}

CUDPStream::CPacket::CPacket(const BYTE* p, ULONGLONG start, UINT size)
	: m_start(start)
	, m_end(start + size)
{
	ASSERT(size > 0);
	m_buff = std::make_unique<BYTE[]>(size);
	memcpy(m_buff.get(), p, size);
}

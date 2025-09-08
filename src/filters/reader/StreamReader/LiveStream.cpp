/*
 * (C) 2017-2025 see Authors.txt
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
#include "LiveStream.h"
#include <moreuuids.h>
#include "DSUtil/DSUtil.h"
#include "DSUtil/UrlParser.h"
#include "DSUtil/entities.h"
#include "Subtitles/TextFile.h"

#define RAPIDJSON_SSE2
#include <ExtLib/rapidjson/include/rapidjson/document.h>

#define MAXSTORESIZE  2 * MEGABYTE // The maximum size of a buffer for storing the received information is 2 Mb
#define MAXBUFSIZE   16 * KILOBYTE // The maximum packet size is 16 Kb

// CLiveStream

CLiveStream::~CLiveStream()
{
	Clear();
}

void CLiveStream::Clear()
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
		m_hlsData.PlaylistDuration = {};
		m_hlsData.bInit = {};

		m_hlsData.SegmentPos = m_hlsData.SegmentSize = {};
	}

	m_HTTPAsync.Close();

	EmptyBuffer();

	m_pos = m_len = 0;
	m_SizeComplete = 0;
}

void CLiveStream::Append(const BYTE* buff, UINT len)
{
	CAutoLock cPacketLock(&m_csPacketsLock);

	m_packets.emplace_back(buff, m_len, len);
	m_len += len;

	if (m_SizeComplete && m_len >= m_SizeComplete) {
		m_EventComplete.Set();
	}
}

HRESULT CLiveStream::HTTPRead(PBYTE pBuffer, DWORD dwSizeToRead, DWORD& dwSizeRead, DWORD dwTimeOut/* = INFINITE*/)
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

		if ((m_nBytesRead += dwSizeRead) == m_icydata.metaint) {
			m_nBytesRead = 0;

			static BYTE buff[255 * 16], b = 0;
			ZeroMemory(buff, sizeof(buff));

			DWORD _dwSizeRead = 0;
			if (S_OK == m_HTTPAsync.Read(&b, 1, _dwSizeRead, dwTimeOut) && _dwSizeRead == 1 && b) {
				if (S_OK == m_HTTPAsync.Read(buff, b * 16, _dwSizeRead, dwTimeOut) && _dwSizeRead == b * 16) {
					int len = decode_html_entities_utf8((char*)buff, nullptr);

					CString str = UTF8orLocalToWStr((LPCSTR)buff);

					DLog(L"CLiveStream::HTTPRead(): Metainfo: %s", str);

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
						DLog(L"CLiveStream::HTTPRead(): StreamTitle is missing");
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
	} else if (size > 4 && GETU32(buf) == MAKEFOURCC('F','L','V',1)) {
		subtype = MEDIASUBTYPE_FLV;
	} else if (size > 8 && GETU32(buf + 4) == FCC('ftyp')) {
		subtype = MEDIASUBTYPE_MP4;
	}
}

bool CLiveStream::ParseM3U8(const CString& url, CString& realUrl)
{
	CWebTextFile f(CP_UTF8, CP_ACP, false, 10 * MEGABYTE);
	if (!f.Open(url)) {
		return false;
	}

	CString str;
	if (!f.ReadString(str) || str != L"#EXTM3U") {
		return false;
	}

	realUrl = url;

	CString base(url);
	if (!f.GetRedirectURL().IsEmpty()) {
		base = f.GetRedirectURL();
	}
	base.Truncate(base.ReverseFind('/') + 1);

	bool bMediaSequence = false;
	uint64_t sequenceNumber = {};

	double segmentDuration = {};
	int64_t segmentsDuration = {};

	bool bDiscontinuity = false;

	auto segmentsCount = m_hlsData.Segments.size();

	int32_t bandwidth = {};
	std::list<std::pair<uint32_t, CString>> PlaylistItems;

	m_hlsData.PlaylistDuration = {};

	auto CombinePath = [](const CString& base, const CString& relative) {
		CUrlParser urlParser(relative.GetString());
		if (urlParser.IsValid()) {
			return relative;
		}

		return CUrlParser::CombineUrl(base, relative);
	};

	bool bItemsFound = false;

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
			StrToInt64(str, m_hlsData.PlaylistDuration);
			m_hlsData.PlaylistDuration *= 1000;
			continue;
		} else if (StartsWith(str, L"#EXT-X-MEDIA-SEQUENCE:")) {
			DeleteLeft(22, str);
			if (StrToUInt64(str, sequenceNumber)) {
				bMediaSequence = true;
			}
			continue;
		} else if (str == L"#EXT-X-DISCONTINUITY") {
			if (!bItemsFound) {
				bDiscontinuity = true;
			}
			continue;
		} else if (str == L"#EXT-X-ENDLIST") {
			if (!m_hlsData.bInit) {
				DLog(L"CLiveStream::ParseM3U8() : support only LIVE stream.");
				return false;
			}
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
					if (!uri.IsEmpty() && !iv.IsEmpty()) {
						uri.Trim(L'"');
						uri = CombinePath(base, uri);
						CHTTPAsync http;
						if (SUCCEEDED(http.Connect(uri, http::connectTimeout))) {
							BYTE key[CAESDecryptor::AESBLOCKSIZE] = {};
							DWORD dwSizeRead = 0;
							if (SUCCEEDED(http.Read(key, CAESDecryptor::AESBLOCKSIZE, dwSizeRead, http::readTimeout)) && dwSizeRead == CAESDecryptor::AESBLOCKSIZE) {
								std::vector<BYTE> pIV;
								iv.Delete(0, 2);
								if (iv.GetLength() != (CAESDecryptor::AESBLOCKSIZE * 2)) {
									DLog(L"CLiveStream::ParseM3U8() : wrong AES IV.");
									return false;
								}
								CStringToBin(iv, pIV);

								if (!m_hlsData.pAESDecryptor->SetKey(key, std::size(key), pIV.data(), pIV.size())) {
									DLog(L"CLiveStream::ParseM3U8() : can't initialize decrypting engine.");
									return false;
								}

								m_hlsData.bAes128 = true;
								continue;
							}
						}
					}
				}

				DLog(L"CLiveStream::ParseM3U8() : not supported encrypted playlist.");
				return false;
			}
			continue;
		} else if (StartsWith(str, L"#EXTINF:")) {
			DeleteLeft(8, str);
			StrToDouble(str, segmentDuration);
			continue;
		} else if (StartsWith(str, L"#EXT-X-MAP:")) {
			if (!m_hlsData.bInit) {
				DeleteLeft(11, str);
				std::list<CString> attributes;
				Explode(str, attributes, L',');
				for (const auto& attribute : attributes) {
					const auto pos = attribute.Find(L'=');
					if (pos > 0) {
						const auto key = attribute.Left(pos);
						auto value = attribute.Mid(pos + 1);
						if (key == L"URI") {
							value.Trim(LR"(")");
							m_hlsData.Segments.emplace_back(CombinePath(base, value));
							break;
						}
					}
				}
			}
			continue;
		} else if (str.GetAt(0) == L'#') {
			continue;
		}

		bItemsFound = true;

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

					if (segmentDuration > 0.) {
						segmentsDuration += std::llround(segmentDuration * 1000.);
					}
				}
			} else {
				if (bMediaSequence && sequenceNumber > m_hlsData.SequenceNumber) {
					m_hlsData.Segments.emplace_back(CombinePath(base, str));
					m_hlsData.SequenceNumber = sequenceNumber;

					if (segmentDuration > 0.) {
						segmentsDuration += std::llround(segmentDuration * 1000.);
					}
				}
				sequenceNumber++;
			}
		}

		segmentDuration = {};
	}

	if (bDiscontinuity) {
		m_hlsData.SequenceNumber = {};
	} else {
		m_hlsData.DiscontinuitySegments.clear();
	}

	m_hlsData.PlaylistParsingTime = std::chrono::high_resolution_clock::now();

	if (segmentsDuration) {
		m_hlsData.PlaylistDuration = std::min(m_hlsData.PlaylistDuration, segmentsDuration);
	}
	if (m_hlsData.PlaylistDuration >= 2000) {
		m_hlsData.PlaylistDuration /= 2;
	}

	if (m_hlsData.PlaylistDuration && (bMediaSequence || bDiscontinuity)) {
		if (!m_hlsData.bInit) {
			/*
			https://datatracker.ietf.org/doc/html/draft-pantos-http-live-streaming-19#section-6.3.3
			We still need to check the EXT-X-ENDLIST tag and the duration of each segment - but so far so good.
			If there are problems, we will look at specific links.
			*/
			if (m_hlsData.Segments.size() > 2) {
				m_hlsData.Segments.erase(m_hlsData.Segments.begin(), m_hlsData.Segments.end() - 2);
			}
			m_hlsData.bInit = true;
		}

		bool ret = segmentsCount < m_hlsData.Segments.size();
		if (!ret) {
			/* If we need to reload the playlist again
			 * (if there are no new segments),
			 * then we use an interval of half the duration. */
			m_hlsData.PlaylistDuration /= 2;
		}

		return ret;
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

bool CLiveStream::OpenHLSSegment()
{
	if (!m_hlsData.Segments.empty()) {
		auto hr = m_HTTPAsync.Connect(m_hlsData.Segments.front(), http::connectTimeout);
		m_hlsData.Segments.pop_front();
		if (SUCCEEDED(hr)) {
			m_hlsData.SegmentSize = m_HTTPAsync.GetLenght();
			m_hlsData.SegmentPos = {};
			m_hlsData.bEndOfSegment = {};
			return true;
		}
	}

	return false;
}

bool CLiveStream::Load(const WCHAR* fnw)
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
		HRESULT hr = m_HTTPAsync.Connect(m_url_str, http::connectTimeout, L"Icy-MetaData:1\r\n");
		if (FAILED(hr)) {
			return false;
		}

		DLog(L"CLiveStream::Load() - HTTP content type: %s", m_HTTPAsync.GetContentType());

		BOOL bConnected = FALSE;

		if (!m_HTTPAsync.GetLenght()) { // only streams without content length
			BOOL bIcyFound = FALSE;
			const CString& hdr = m_HTTPAsync.GetHeader();
			DLog(L"CLiveStream::Load() - HTTP hdr:\n%s", hdr);

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

			if (contentType == L"application/octet-stream"
					|| contentType == L"video/unknown" || contentType == L"none"
					|| contentType.IsEmpty()) {
				BYTE buf[1024] = {};
				DWORD dwSizeRead = 0;
				if (HTTPRead(buf, sizeof(buf), dwSizeRead, http::readTimeout) == S_OK && dwSizeRead) {
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
			} else if (contentType.Find(L"audio/wav") == 0) {
				m_subtype = MEDIASUBTYPE_WAVE;
			} else if (contentType.Find(L"audio/") == 0) {
				m_subtype = MEDIASUBTYPE_MPEG1Audio;
			} else if (!bIcyFound) { // other ...
					// not supported content-type
				bConnected = FALSE;
			}
		}

		if (!bConnected && (m_HTTPAsync.GetLenght() || m_HTTPAsync.GetContentType().Find(L"mpegurl") > 0)) {
			DLog(L"CLiveStream::Load() - HTTP hdr:\n%s", m_HTTPAsync.GetHeader());

			bool ret = {};
			if (m_HTTPAsync.IsCompressed()) {
				if (m_HTTPAsync.GetLenght() <= 10 * MEGABYTE) {
					std::vector<BYTE> body;
					ret = m_HTTPAsync.GetUncompressed(body);
					if (ret && memcmp(body.data(), "#EXTM3U", 7) == 0) {
						ret = ParseM3U8(m_url_str, m_hlsData.PlaylistUrl);
					}
				}
			} else {
				BYTE body[8] = {};
				DWORD dwSizeRead = 0;
				if (m_HTTPAsync.Read(body, 7, dwSizeRead, http::readTimeout) == S_OK) {
					if (memcmp(body, "#EXTM3U", 7) == 0) {
						ret = ParseM3U8(m_url_str, m_hlsData.PlaylistUrl);
					}
				}
			}

			m_HTTPAsync.Close();

			if (ret && !m_hlsData.Segments.empty()) {
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

	if (m_protocol == protocol::PR_HTTP && !m_len) {
		BYTE buf[1024] = {};
		DWORD dwSizeRead = 0;
		if (HTTPRead(buf, sizeof(buf), dwSizeRead, http::readTimeout) != S_OK) {
			return false;
		}

		if (dwSizeRead) {
			Append(buf, dwSizeRead);
		}
	}

	CAMThread::Create();
	if (FAILED(CAMThread::CallWorker(CMD::CMD_INIT))) {
		Clear();
		return false;
	}

	const clock_t start = clock();
	const ULONGLONG len = (m_subtype == MEDIASUBTYPE_MPEG2_TRANSPORT ? 128 : 64) * KILOBYTE;
	while (clock() - start < 500 && m_len < len) {
		Sleep(50);
	}

	return true;
}

// CAsyncStream

HRESULT CLiveStream::SetPointer(LONGLONG llPos)
{
	CAutoLock cAutoLock(&m_csLock);

	if (llPos < 0) {
		return E_FAIL;
	}

	m_pos = llPos;

	const __int64 start = m_packets.empty() ? 0 : m_packets.front().m_start;
	if (llPos < start) {
		DLog(L"CLiveStream::SetPointer() warning! %lld misses in [%llu - %llu]", llPos, start, m_packets.back().m_end);
		return S_FALSE;
	}

	return S_OK;
}

HRESULT CLiveStream::Read(PBYTE pbBuffer, DWORD dwBytesToRead, BOOL bAlign, LPDWORD pdwBytesRead)
{
	DWORD len = dwBytesToRead;
	BYTE* ptr = pbBuffer;

	if (!m_packets.empty()
			&& m_pos + len > m_packets.back().m_end) {
		m_SizeComplete = m_pos + len;

#if _DEBUG
		DLog(L"CLiveStream::Read() : wait %llu bytes, %llu -> %llu", m_SizeComplete - m_packets.back().m_end, m_packets.back().m_end, m_SizeComplete);
		const ULONGLONG start = GetPerfCounter();
#endif

		while (!m_bEndOfStream && !m_EventComplete.Wait(1));
		m_SizeComplete = 0;

#if _DEBUG
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

		DLogIf(m_pos < p.m_start, L"CLiveStream::Read(): requested data is no longer available, %llu - %llu", m_pos, p.m_start);
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

LONGLONG CLiveStream::Size(LONGLONG* pSizeAvailable)
{
	CAutoLock cAutoLock(&m_csLock);

	if (pSizeAvailable) {
		*pSizeAvailable = m_len;
	}

	return 0;
}

DWORD CLiveStream::Alignment()
{
	return 1;
}

void CLiveStream::Lock()
{
	m_csLock.Lock();
}

void CLiveStream::Unlock()
{
	m_csLock.Unlock();
}

inline const ULONGLONG CLiveStream::GetPacketsSize()
{
	CAutoLock cPacketLock(&m_csPacketsLock);

	return m_packets.empty() ? 0 : m_packets.back().m_end - m_packets.front().m_start;
}

void CLiveStream::CheckBuffer()
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

void CLiveStream::EmptyBuffer()
{
	CAutoLock cPacketLock(&m_csPacketsLock);

	m_packets.clear();
	m_len = m_pos;
}

#define ENABLE_DUMP 0

DWORD CLiveStream::ThreadProc()
{
	SetThreadPriority(m_hThread, THREAD_PRIORITY_TIME_CRITICAL);

#if ENABLE_DUMP
	const auto prefix = m_protocol == protocol::PR_HLS ? L"hls" :
		m_protocol == protocol::PR_HTTP ? L"http" :
		m_protocol == protocol::PR_UDP ? L"udp" :
		L"stdin";

	SYSTEMTIME st;
	::GetLocalTime(&st);

	CString dump_filename;
	dump_filename.Format(L"%s__%04u_%02u_%02u__%02u_%02u_%02u.dump", prefix, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

	FILE* dump_file = nullptr;
	_wfopen_s(&dump_file, dump_filename.GetString(), L"wb");
#endif

	auto buff = std::make_unique<BYTE[]>(MAXBUFSIZE * 2);
	int  buffsize = 0;
	int  len = 0;

	auto encryptedBuff = m_hlsData.bAes128 ? std::make_unique<BYTE[]>(MAXBUFSIZE) : nullptr;

	for (;;) {
		m_RequestCmd = GetRequest();

		switch (m_RequestCmd) {
			default:
			case CMD::CMD_EXIT:
				Reply(S_OK);
#if ENABLE_DUMP
				if (dump_file) {
					fclose(dump_file);
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
					m_hlsData.bRunning = {};
				}

				buffsize = len = 0;
				break;
			case CMD::CMD_INIT:
			case CMD::CMD_RUN:
				Reply(S_OK);

				UINT attempts = 0;
				BOOL bEndOfStream = FALSE;

				if (m_protocol == protocol::PR_HLS) {
					if (!m_hlsData.bRunning) {
						m_hlsData.bRunning = true;

						if (m_hlsData.Segments.empty() &&
							(m_hlsData.bEndList
							 || !ParseM3U8(m_hlsData.PlaylistUrl, m_hlsData.PlaylistUrl)
							 || !OpenHLSSegment())) {
							m_bEndOfStream = true;
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
						const HRESULT hr = HTTPRead(&buff[buffsize], MAXBUFSIZE, dwSizeRead, http::readTimeout);
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
							auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_hlsData.PlaylistParsingTime).count();
							if (milliseconds > static_cast<int64_t>(m_hlsData.PlaylistDuration)) {
								ParseM3U8(m_hlsData.PlaylistUrl, m_hlsData.PlaylistUrl);
							}
						}

						if (m_hlsData.IsEndOfSegment()) {
							m_HTTPAsync.Close();

							if (m_hlsData.Segments.empty()) {
								if (m_hlsData.bEndList) {
									bEndOfStream = TRUE;
									break;
								}
								attempts++;
								Sleep(50);
								continue;
							} else {
								if (!OpenHLSSegment()) {
									continue;
								}
							}
						}

						DWORD dwSizeRead = 0;
						auto ptr = m_hlsData.bAes128 ? encryptedBuff.get() : &buff[buffsize];
						if (FAILED(m_HTTPAsync.Read(ptr, MAXBUFSIZE, dwSizeRead, http::readTimeout))) {
							bEndOfStream = TRUE;
							break;
						} else if (dwSizeRead == 0) {
							m_hlsData.bEndOfSegment = true;
							continue;
						}

						m_hlsData.SegmentPos += dwSizeRead;
						if (m_hlsData.bAes128) {
							size_t decryptedSize = {};
							if (m_hlsData.pAESDecryptor->Decrypt(ptr, dwSizeRead, &buff[buffsize], decryptedSize, m_hlsData.IsEndOfSegment())) {
								len = decryptedSize;
							} else {
								len = 0;
							}
						} else {
							len = dwSizeRead;
						}
					}

					attempts = 0;
					buffsize += len;
					if (buffsize >= MAXBUFSIZE) {
#if ENABLE_DUMP
						if (dump_file) {
							fwrite(buff.get(), static_cast<size_t>(buffsize), 1, dump_file);
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

CLiveStream::CPacket::CPacket(const BYTE* p, ULONGLONG start, UINT size)
	: m_start(start)
	, m_end(start + size)
{
	ASSERT(size > 0);
	m_buff = std::make_unique<BYTE[]>(size);
	memcpy(m_buff.get(), p, size);
}

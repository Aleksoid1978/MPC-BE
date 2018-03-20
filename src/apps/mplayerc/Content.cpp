/*
 * (C) 2016-2018 see Authors.txt
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
#include <atlutil.h>
#include <regex>
#include "Content.h"
#include "../../DSUtil/FileHandle.h"
#include "../../DSUtil/HTTPAsync.h"

static const CString ConvertToUTF16(const BYTE* pData, size_t size)
{
	BOOL bUTF8    = FALSE;
	BOOL bUTF16LE = FALSE;
	BOOL bUTF16BE = FALSE;
	if (size > 2) {
		if (pData[0] == 0xFE && pData[1] == 0xFF) {
			bUTF16BE = TRUE; pData += 2; size -= 2;
		} else if (pData[0] == 0xFF && pData[1] == 0xFE) {
			bUTF16LE = TRUE; pData += 2; size -= 2;
		} else if (size > 3 && pData[0] == 0xEF && pData[1] == 0xBB && pData[2] == 0xBF) {
			bUTF8 = TRUE; pData += 3; size -= 3;
		}
	}

	if (bUTF16BE || bUTF16LE) {
		size /= 2;
		CString str((LPCTSTR)pData, size);
		if (bUTF16BE) {
			for (int i = 0, j = str.GetLength(); i < j; i++) {
				str.SetAt(i, (str[i] << 8) | (str[i] >> 8));
			}
		}
		return str;
	}

	CStringA lpMultiByteStr((LPCSTR)pData, size);
	if (bUTF8) {
		CString str = UTF8To16(lpMultiByteStr);
		return str;
	}

	CString str = AltUTF8To16(lpMultiByteStr);
	if (str.IsEmpty()) {
		str = ConvertToUTF16(lpMultiByteStr, CP_ACP);
	}

	return str;
}

namespace Content {
	static void GetContentTypeByExt(CString path, CString& ct)
	{
		const CString ext = GetFileExt(path).MakeLower();

		if (ext == L".m3u" || ext == L".m3u8") {
			ct = L"audio/x-mpegurl";
		} else if (ext == L".pls") {
			ct = L"audio/x-scpls";
		} else if (ext == L".xspf") {
			ct = L"application/xspf+xml";
		} else if (ext == L".asx") {
			ct = L"video/x-ms-asf";
		} else if (ext == L".ram") {
			ct = L"audio/x-pn-realaudio";
		} else if (ext == L".qtl") {
			ct = L"application/x-quicktimeplayer";
		} else if (ext == L".wpl") {
			ct = L"application/vnd.ms-wpl";
		} else if (ext == L".mpcpl") {
			ct = L"application/x-mpc-playlist";
		} else if (ext == L".bdmv") {
			ct = L"application/x-bdmv-playlist";
		} else if (ext == L".cue") {
			ct = L"application/x-cue-metadata";
		}
	}

	struct Content {
		bool bInitialized   = false;
		bool bHTTPConnected = false;
		bool bGOTRaw        = false;
		bool bGOTCt         = false;
		bool bGOTRedir      = false;
		std::vector<BYTE> raw;
		std::shared_ptr<CHTTPAsync> HTTPAsync = nullptr;
		CString ct;
		CString body;
	};
	static std::map<CString, Content> Contents;

	static void Encoding(Content& content)
	{
		content.body.Empty();
		if (!content.raw.empty()) {
			content.body = ConvertToUTF16(content.raw.data(), content.raw.size());
		}
	}

	static void GetData(Content& content)
	{
		if (content.bInitialized) {
			if (!content.bGOTRaw) {
				content.bGOTRaw = true;

				if (content.bHTTPConnected) {
					size_t nMinSize = KILOBYTE;
					const QWORD ContentLength = content.HTTPAsync->GetLenght();
					if (ContentLength && ContentLength < nMinSize) {
						nMinSize = ContentLength;
					}

					content.raw.resize(nMinSize);
					DWORD dwSizeRead = 0;
					if (content.HTTPAsync->Read(content.raw.data(), nMinSize, &dwSizeRead) == S_OK) {
						content.raw.resize(dwSizeRead);
						if (dwSizeRead) {
							Encoding(content);
						}
					}
				}
			}
		}
	}

	static const bool Connect(const CString fn)
	{
		auto& content = Contents[fn];

		if (!content.bInitialized) {
			content.bInitialized = true;
			content.HTTPAsync = std::make_shared<CHTTPAsync>();

			CString realPath(fn);
			CorrectAceStream(realPath);

			content.bHTTPConnected = (content.HTTPAsync->Connect(realPath, 10000) == S_OK);

			GetData(content);
		}

		return content.bHTTPConnected;
	}

	static void GetRedirectData(Content& content)
	{
		if (content.bInitialized) {
			if (!content.bGOTRedir) {
				content.bGOTRedir = true;

				if (content.bHTTPConnected) {
					size_t nMaxSize = 16 * KILOBYTE;
					const QWORD ContentLength = content.HTTPAsync->GetLenght();
					if (ContentLength && ContentLength < nMaxSize) {
						nMaxSize = ContentLength;
					}

					if (nMaxSize > content.raw.size()) {
						const size_t old_size = content.raw.size();
						nMaxSize -= old_size;
						content.raw.resize(old_size + nMaxSize);
						DWORD dwSizeRead = 0;
						if (content.HTTPAsync->Read(content.raw.data() + old_size, nMaxSize, &dwSizeRead) == S_OK) {
							content.raw.resize(old_size + dwSizeRead);
							if (dwSizeRead) {
								Encoding(content);
							}
						}
					}
				}
			}
		}
	}

	static void GetContent(const CString& fn, Content& content)
	{
		if (content.bInitialized) {
			if (!content.bGOTCt) {
				content.bGOTCt = true;

				if (content.bHTTPConnected) {
					content.ct = content.HTTPAsync->GetContentType();

					if (content.ct.IsEmpty() || content.ct == L"application/octet-stream") {
						GetContentTypeByExt(fn, content.ct);
					}

					if ((content.body.GetLength() >= 3 && wcsncmp(content.body, L".ra", 3) == 0)
							|| (content.body.GetLength() >= 4 && wcsncmp(content.body, L".RMF", 4) == 0)) {
						content.ct = L"audio/x-pn-realaudio";
					}
					if (content.body.GetLength() >= 4 && GETDWORD((LPCTSTR)content.body) == 0x75b22630) {
						content.ct = L"video/x-ms-wmv";
					}
					if (content.body.GetLength() >= 8 && wcsncmp((LPCTSTR)content.body + 4, L"moov", 4) == 0) {
						content.ct = L"video/quicktime";
					}
					if (content.body.Find(L"#EXTM3U") == 0 && content.body.Find(L"#EXT-X-MEDIA-SEQUENCE") > 0) {
						content.ct = L"application/http-live-streaming";
					}
					if (content.body.Find(L"#EXT-X-STREAM-INF:") >= 0) {
						content.ct = L"application/http-live-streaming-m3u";
					}
					if ((content.ct.Find(L"text/plain") == 0
							|| content.ct.Find(L"application/vnd.apple.mpegurl") == 0
							|| content.ct.Find(L"audio/mpegurl") == 0) && content.body.Find(L"#EXTM3U") == 0) {
						content.ct = L"audio/x-mpegurl";
					}
				}
			}
		}
	}

	enum {
		PLAYLIST_NONE = -1,
		PLAYLIST_M3U,
		PLAYLIST_PLS,
		PLAYLIST_XSPF,
		PLAYLIST_ASX,
		PLAYLIST_RAM,
		PLAYLIST_QTL,
		PLAYLIST_WPL,
	};

	const std::wregex ref_m3u(L"(^|\\n)(?!#)([^\\n]+)");                                                // any lines except those that start with '#'
	const std::wregex ref_pls(L"(^|\\n)File\\d+[ \\t]*=[ \\t]*\"?([^\\n\"]+)");                         // File1=...
	const std::wregex ref_xspf(L"<location>([^<>\\n]+)</location>");                                    // <location>...</location>
	const std::wregex ref_asx(L"<REF HREF[ \\t]*=[ \\t]*\"([^\"\\n]+)\"", std::regex_constants::icase); // <REF HREF = "..." />
	const std::wregex ref_ram(L"(^|\\n)((?:rtsp|http|file)://[^\\n]+)");                                // (rtsp|http|file)://...
	const std::wregex ref_qtl(L"src[ \\t]*=[ \\t]*\"([^\"\\n]+)\"");                                    // src="..."
	const std::wregex ref_wpl(L"<media src=\"([^\"\\n]+)\"");                                           // <media src="..."

	static const bool FindRedir(const CUrl& src, const CString& body, std::list<CString>& urls, const int playlist_type)
	{
		std::wregex rgx;

		switch (playlist_type) {
			case PLAYLIST_M3U: rgx = ref_m3u;  break;
			case PLAYLIST_PLS: rgx = ref_pls;  break;
			case PLAYLIST_XSPF:rgx = ref_xspf; break;
			case PLAYLIST_ASX: rgx = ref_asx;  break;
			case PLAYLIST_RAM: rgx = ref_ram;  break;
			case PLAYLIST_QTL: rgx = ref_qtl;  break;
			case PLAYLIST_WPL: rgx = ref_wpl;  break;
			default:
				return false;
		}

		std::wcmatch match;
		const wchar_t* start = body.GetString();

		while (std::regex_search(start, match, rgx) && match.size() > 1) {
			start = match[0].second;

			const size_t k = match.size() - 1;
			CString url = CString(match[k].first, match[k].length());
			url.Trim();

			if (playlist_type == PLAYLIST_RAM && url.Left(7) == L"file://") {
				url.Delete(0, 7);
				url.Replace('/', '\\');
			}

			CUrl dst;
			dst.CrackUrl(url);

			if (url.Find(L"://") < 0) {
				DWORD dwUrlLen = src.GetUrlLength() + 1;
				WCHAR* szUrl = new WCHAR[dwUrlLen];

				// Retrieve the contents of the CUrl object
				src.CreateUrl(szUrl, &dwUrlLen);
				CString path(szUrl);
				delete[] szUrl;

				int pos = path.ReverseFind('/');
				if (pos > 0) {
					path.Delete(pos + 1, path.GetLength() - pos - 1);
				}

				if (url[0] == '/') {
					path.Delete(path.GetLength() - 1, 1);
				}

				url = path + url;
				dst.CrackUrl(url);
			}

			if (_wcsicmp(src.GetSchemeName(), dst.GetSchemeName())
					|| _wcsicmp(src.GetHostName(), dst.GetHostName())
					|| _wcsicmp(src.GetUrlPath(), dst.GetUrlPath())) {
				urls.push_back(url);
			} else {
				// recursive
				urls.clear();
				break;
			}
		}

		return urls.size() > 0;
	}

	static const bool FindRedir(const CString& fn, std::list<CString>& fns, const int playlist_type)
	{
		CString body;

		CTextFile f(CTextFile::UTF8, CTextFile::ANSI);
		if (f.Open(fn)) {
			for (CString tmp; f.ReadString(tmp); body += tmp + '\n');
		}

		CString dir = fn.Left(std::max(fn.ReverseFind('/'), fn.ReverseFind('\\')) + 1);

		std::wregex rgx;

		switch (playlist_type) {
			case PLAYLIST_M3U: rgx = ref_m3u;  break;
			case PLAYLIST_PLS: rgx = ref_pls;  break;
			case PLAYLIST_XSPF:rgx = ref_xspf; break;
			case PLAYLIST_ASX: rgx = ref_asx;  break;
			case PLAYLIST_RAM: rgx = ref_ram;  break;
			case PLAYLIST_QTL: rgx = ref_qtl;  break;
			case PLAYLIST_WPL: rgx = ref_wpl;  break;
			default:
				return false;
		}

		std::wcmatch match;
		const wchar_t* start = body.GetBuffer();

		while (std::regex_search(start, match, rgx) && match.size() > 1) {
			start = match[0].second;

			const size_t k = match.size() - 1;
			CString fn2 = CString(match[k].first, match[k].length());
			fn2.Trim();

			if (playlist_type == PLAYLIST_RAM && fn2.Left(7) == L"file://") {
				fn2.Delete(0, 7);
				fn2.Replace('/', '\\');
			}

			if (fn2.Find(':') < 0 && fn2.Find(L"\\\\") != 0 && fn2.Find(L"//") != 0) {
				CPath p;
				p.Combine(dir, fn2);
				fn2 = (LPCTSTR)p;
			}

			if (!fn2.CompareNoCase(fn)) {
				continue;
			}

			fns.push_back(fn2);
		}

		return fns.size() > 0;
	}

	const CString GetType(CString fn, std::list<CString>* redir)
	{
		CUrl url;
		CString ct, body;

		CString realPath(fn);
		CorrectAceStream(realPath);

		if (::PathIsURL(realPath) && url.CrackUrl(realPath)) {
			CString schemeName = url.GetSchemeName();
			schemeName.MakeLower();
			if (schemeName == L"pnm") {
				return L"audio/x-pn-realaudio";
			}

			if (schemeName == L"mms") {
				return L"video/x-ms-asf";
			}

			if (Connect(fn)) {
				auto& Content = Contents[fn];

				GetContent(fn, Content);

				if (redir
						&& (Content.ct == L"audio/x-scpls" || Content.ct == L"audio/x-mpegurl" || Content.ct == L"application/xspf+xml")) {
					GetRedirectData(Content);
				}

				ct = Content.ct;
				body = Content.body;
			}
		} else if (!fn.IsEmpty()) {
			GetContentTypeByExt(fn, ct);

			FILE* f = nullptr;
			if (_wfopen_s(&f, fn, L"rb") == 0) {
				CStringA str;
				str.ReleaseBufferSetLength(fread(str.GetBuffer(3), 1, 3, f));
				body = AToT(str);
				fclose(f);
			}
		}

		if (body.GetLength() >= 3) { // here only those which cannot be opened through dshow
			if (!wcsncmp(body, L"FWS", 3) || !wcsncmp(body, L"CWS", 3) || !wcsncmp(body, L"ZWS", 3)) {
				return L"application/x-shockwave-flash";
			}
		}

		if (redir && !ct.IsEmpty()) {
			int playlist_type = PLAYLIST_NONE;

			if (ct == L"audio/x-mpegurl" && fn.Find(L"://") > 0) {
				playlist_type = PLAYLIST_M3U;
			} else if (ct == L"audio/x-scpls") {
				playlist_type = PLAYLIST_PLS;
			} else if (ct == L"application/xspf+xml") {
				playlist_type = PLAYLIST_XSPF;
			} else if (ct == L"video/x-ms-asf") {
				playlist_type = PLAYLIST_ASX;
			} else if (ct == L"audio/x-pn-realaudio") {
				playlist_type = PLAYLIST_RAM;
			} else if (ct == L"application/x-quicktimeplayer") {
				playlist_type = PLAYLIST_QTL;
			} else if (ct == L"application/vnd.ms-wpl") {
				playlist_type = PLAYLIST_WPL;
			}

			if (!body.IsEmpty()) {
				if (fn.Find(L"://") >= 0) {
					FindRedir(url, body, *redir, playlist_type);
				} else {
					FindRedir(fn, *redir, playlist_type);
				}
			}
		}

		return ct;
	}

	namespace Online {
		const bool CheckConnect(const CString& fn)
		{
			CString realPath(fn);
			CorrectAceStream(realPath);

			CUrl url;
			if (::PathIsURL(realPath)
					&& url.CrackUrl(realPath)
					&& (url.GetScheme() == ATL_URL_SCHEME_HTTP || url.GetScheme() == ATL_URL_SCHEME_HTTPS)) {
				return Connect(realPath);
			}

			return true;
		}

		void Clear()
		{
			Contents.clear();
		}

		void Clear(const CString& fn)
		{
			Contents.erase(fn);
		}

		void Disconnect(const CString& fn)
		{
			auto& it = Contents.find(fn);
			if (it != Contents.end()) {
				auto& content = it->second;
				content.HTTPAsync->Close();
			}
		}

		void GetRaw(const CString& fn, std::vector<BYTE>& raw)
		{
			auto& it = Contents.find(fn);
			if (it != Contents.end()) {
				auto& content = it->second;
				raw = content.raw;
			}
		}
	}
}

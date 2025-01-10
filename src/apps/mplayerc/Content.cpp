/*
 * (C) 2016-2025 see Authors.txt
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
#include "Content.h"
#include "DSUtil/FileHandle.h"
#include "DSUtil/HTTPAsync.h"
#include "DSUtil/UrlParser.h"

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
		CStringW str((LPCWSTR)pData, size);
		if (bUTF16BE) {
			for (int i = 0, j = str.GetLength(); i < j; i++) {
				str.SetAt(i, (str[i] << 8) | (str[i] >> 8));
			}
		}
		return str;
	}

	CStringA lpMultiByteStr((LPCSTR)pData, size);
	if (bUTF8) {
		return UTF8ToWStr(lpMultiByteStr);
	}

	return UTF8orLocalToWStr(lpMultiByteStr);
}

namespace Content {
	static void GetContentTypeByExt(const CString& path, CString& ct)
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
		} else if (ext == L".swf") {
			ct = L"application/x-shockwave-flash";
		}
	}

	struct Content {
		bool bInitialized   = false;
		bool bHTTPConnected = false;
		bool bGOTRaw        = false;
		bool bGOTCt         = false;
		bool bGOTRedir      = false;
		std::vector<BYTE> raw;
		std::unique_ptr<CHTTPAsync> HTTPAsync;
		CString ct;
		CString body;
		CString hdr;
		CString realPath;
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
					const UINT64 ContentLength = content.HTTPAsync->GetLenght();
					if (ContentLength && ContentLength < nMinSize) {
						nMinSize = ContentLength;
					}

					content.raw.resize(nMinSize);
					DWORD dwSizeRead = 0;
					if (content.HTTPAsync->Read(content.raw.data(), nMinSize, dwSizeRead, http::readTimeout) == S_OK) {
						content.raw.resize(dwSizeRead);
						if (dwSizeRead) {
							Encoding(content);
						}
					}
				}
			}
		}
	}

	static const bool Connect(const CString& fn)
	{
		auto& content = Contents[fn];

		if (!content.bInitialized) {
			content.bInitialized = true;
			content.HTTPAsync = std::make_unique<CHTTPAsync>();

			CString realPath(fn);
			CorrectAceStream(realPath);
			content.realPath = realPath;

			content.bHTTPConnected = (content.HTTPAsync->Connect(realPath, http::connectTimeout, L"Icy-MetaData: 1\r\n") == S_OK);
			content.hdr = content.HTTPAsync->GetHeader();

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
					const UINT64 ContentLength = content.HTTPAsync->GetLenght();
					if (ContentLength && ContentLength < nMaxSize) {
						nMaxSize = ContentLength;
					}

					if (nMaxSize > content.raw.size()) {
						const size_t old_size = content.raw.size();
						nMaxSize -= old_size;
						content.raw.resize(old_size + nMaxSize);
						DWORD dwSizeRead = 0;
						if (content.HTTPAsync->Read(content.raw.data() + old_size, nMaxSize, dwSizeRead, http::readTimeout) == S_OK) {
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
					if (content.body.GetLength() >= 4 && GETU32((LPCWSTR)content.body) == 0x75b22630) {
						content.ct = L"video/x-ms-wmv";
					}
					if (content.body.GetLength() >= 8 && wcsncmp((LPCWSTR)content.body + 4, L"moov", 4) == 0) {
						content.ct = L"video/quicktime";
					}
					if (StartsWith(content.body, L"#EXTM3U") && content.body.Find(L"#EXT-X-MEDIA-SEQUENCE") > 7) {
						content.ct = L"application/http-live-streaming";
					}
					if (content.body.Find(L"#EXT-X-STREAM-INF:") >= 0) {
						content.ct = L"application/http-live-streaming-m3u";
					}
					if ((content.ct.Find(L"text/plain") == 0
							|| content.ct.Find(L"application/vnd.apple.mpegurl") == 0
							|| content.ct.Find(L"audio/mpegurl") == 0) && StartsWith(content.body, L"#EXTM3U")) {
						content.ct = L"audio/x-mpegurl";
					}
				}
			}
		}
	}

	enum {
		PLAYLIST_NONE = -1,
		PLAYLIST_PLS,
		PLAYLIST_XSPF,
		PLAYLIST_ASX,
		PLAYLIST_RAM,
		PLAYLIST_QTL,
		PLAYLIST_WPL,
		PLAYLIST_HTML_META_REFRESH,
	};

	const std::wregex ref_pls(L"(^|\\n)File\\d+[ \\t]*=[ \\t]*\"?([^\\n\"]+)");                         // File1=...
	const std::wregex ref_xspf(L"<location>([^<>\\n]+)</location>");                                    // <location>...</location>
	const std::wregex ref_asx(L"<REF HREF[ \\t]*=[ \\t]*\"([^\"\\n]+)\"", std::regex_constants::icase); // <REF HREF = "..." />
	const std::wregex ref_ram(L"(^|\\n)((?:rtsp|http|file)://[^\\n]+)");                                // (rtsp|http|file)://...
	const std::wregex ref_qtl(L"src[ \\t]*=[ \\t]*\"([^\"\\n]+)\"");                                    // src="..."
	const std::wregex ref_wpl(L"<media src=\"([^\"\\n]+)\"");                                           // <media src="..."
	const std::wregex ref_html_meta_refresh(L"<meta\\s+http-equiv\\s*=\\s*([\"'])refresh\\1\\s+content\\s*=\\s*([\"'])\\d+;\\s*url=((?!\2)[\"']?)([^\"'\\n]+?)\\3\\2\\s*/?>", std::regex_constants::icase);
	                                                                                                    // <meta http-equiv="refresh" content="0; url='...'">

	static const bool FindRedir(const CUrlParser& src, const CString& body, std::list<CString>& urls, const int playlist_type)
	{
		std::wregex rgx;

		switch (playlist_type) {
			case PLAYLIST_PLS:               rgx = ref_pls;               break;
			case PLAYLIST_XSPF:              rgx = ref_xspf;              break;
			case PLAYLIST_ASX:               rgx = ref_asx;               break;
			case PLAYLIST_RAM:               rgx = ref_ram;               break;
			case PLAYLIST_QTL:               rgx = ref_qtl;               break;
			case PLAYLIST_WPL:               rgx = ref_wpl;               break;
			case PLAYLIST_HTML_META_REFRESH: rgx = ref_html_meta_refresh; break;
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

			if (playlist_type == PLAYLIST_RAM) {
				ConvertFileUriToPath(url);
			}

			CUrlParser dst;
			dst.Parse(url);

			if (url.Find(L"://") < 0) {
				CString path = src.GetUrl();

				int pos = path.ReverseFind('/');
				if (pos > 0) {
					path.Delete(pos + 1, path.GetLength() - pos - 1);
				}

				if (url[0] == '/') {
					path.Delete(path.GetLength() - 1, 1);
				}

				url = path + url;
				dst.Parse(url);
			}

			if (_wcsicmp(src.GetSchemeName(), dst.GetSchemeName())
					|| _wcsicmp(src.GetHostName(), dst.GetHostName())
					|| _wcsicmp(src.GetUrlPath(), dst.GetUrlPath())) {
				urls.emplace_back(url);
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
			case PLAYLIST_PLS:  rgx = ref_pls;  break;
			case PLAYLIST_XSPF: rgx = ref_xspf; break;
			case PLAYLIST_ASX:  rgx = ref_asx;  break;
			case PLAYLIST_RAM:  rgx = ref_ram;  break;
			case PLAYLIST_QTL:  rgx = ref_qtl;  break;
			case PLAYLIST_WPL:  rgx = ref_wpl;  break;
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

			if (playlist_type == PLAYLIST_RAM) {
				ConvertFileUriToPath(fn2);
			}

			if (fn2.Find(':') < 0 && fn2.Find(L"\\\\") != 0 && fn2.Find(L"//") != 0) {
				fn2 = GetCombineFilePath(dir, fn2);
			}

			if (!fn2.CompareNoCase(fn)) {
				continue;
			}

			fns.emplace_back(fn2);
		}

		return fns.size() > 0;
	}

	const std::wregex html_mime_regex(L"text/html(?:;\\s*charset=([\"']?)[\\w-]+\\1)?", std::regex_constants::icase);

	const CString GetType(const CString& fn, std::list<CString>* redir)
	{
		CUrlParser urlParser;
		CString ct, body;

		CString realPath(fn);
		CorrectAceStream(realPath);

		if (::PathIsURLW(realPath) && urlParser.Parse(realPath)) {
			auto schemeName = urlParser.GetSchemeName();
			if (_wcsicmp(schemeName, L"pnm") == 0) {
				return L"audio/x-pn-realaudio";
			}

			if (_wcsicmp(schemeName, L"mms") == 0) {
				return L"video/x-ms-asf";
			}

			if (Connect(fn)) {
				auto& Content = Contents[fn];

				GetContent(fn, Content);

				if (redir
						&& (Content.ct == L"audio/x-scpls" || Content.ct == L"application/xspf+xml")) {
					GetRedirectData(Content);
				}

				ct = Content.ct;
				body = Content.body;
			}
		} else if (!fn.IsEmpty()) {
			GetContentTypeByExt(fn, ct);
		}

		if (redir && !ct.IsEmpty()) {
			int playlist_type = PLAYLIST_NONE;

			if (ct == L"audio/x-scpls") {
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
			} else if (std::regex_match(ct.GetString(), html_mime_regex) && body.GetLength() < 4 * KILOBYTE) {
				playlist_type = PLAYLIST_HTML_META_REFRESH;
			}

			if (playlist_type != PLAYLIST_NONE) {
				if (fn.Find(L"://") >= 0) {
					if (!body.IsEmpty()) {
						FindRedir(urlParser, body, *redir, playlist_type);
					}
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

			CUrlParser urlParser;
			if (::PathIsURLW(realPath)
					&& urlParser.Parse(realPath)
					&& (urlParser.GetScheme() == INTERNET_SCHEME_HTTP || urlParser.GetScheme() == INTERNET_SCHEME_HTTPS)) {
				return Connect(fn);
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
			auto it = Contents.find(fn);
			if (it != Contents.end()) {
				auto& content = it->second;
				content.HTTPAsync->Close();
			}
		}

		void GetRaw(const CString& fn, std::vector<BYTE>& raw)
		{
			auto it = std::find_if(Contents.begin(), Contents.end(), [&fn](const auto& pair) {
				return pair.first == fn || pair.second.realPath == fn;
			});

			if (it != Contents.end()) {
				auto& content = it->second;
				raw = content.raw;
			}
		}

		void GetHeader(const CString& fn, CString& hdr)
		{
			auto it = Contents.find(fn);
			if (it != Contents.end()) {
				auto& content = it->second;
				hdr = content.hdr;
			}
		}
	}
}

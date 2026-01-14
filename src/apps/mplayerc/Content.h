/*
 * (C) 2016-2026 see Authors.txt
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

inline void CorrectAceStream(CStringW& path) {
	if (StartsWith(path, L"acestream://")) {
		path.Format(AfxGetAppSettings().strAceStreamAddress, path.Mid(12));
	}
}

namespace Content {
	constexpr static auto kMPCPlaylistType     = L"application/x-mpc-playlist";
	constexpr static auto kM3UPlaylistType     = L"audio/x-mpegurl";
	constexpr static auto kM3ULivePlaylistType = L"application/http-live-streaming-m3u";
	constexpr static auto kCUEPlaylistType     = L"application/x-cue-metadata";
	constexpr static auto kASXPlaylistType     = L"video/x-ms-asf";
	constexpr static auto kBDMVPlaylistType    = L"application/x-bdmv-playlist";
	constexpr static auto kFLASHType           = L"application/x-shockwave-flash";
	constexpr static auto kWPLPlaylistType     = L"application/vnd.ms-wpl";
	constexpr static auto kQTPlayerType        = L"application/x-quicktimeplayer";
	constexpr static auto kQTVideoType         = L"video/quicktime";
	constexpr static auto kRealAudioType       = L"audio/x-pn-realaudio";
	constexpr static auto kPLSPlaylistType     = L"audio/x-scpls";
	constexpr static auto kXSPFPlaylistType    = L"application/xspf+xml";
	constexpr static auto kASFVideoType        = L"video/x-ms-wmv";

	namespace Online {
		const bool CheckConnect(const CString& fn);
		void Clear();
		void Clear(const CString& fn);
		void Disconnect(const CString& fn);
		void GetRaw(const CString& fn, std::vector<BYTE>& raw);
		void GetHeader(const CString& fn, CString& hdr);
	}
	const CString GetType(const CString& fn, std::list<CString>* redir = nullptr);
}

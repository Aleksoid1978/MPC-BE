/*
 * (C) 2026 see Authors.txt
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

#include "OpenMediaData.h"
#include "rapidjsonHelper.h"
#include "DSUtil/SimpleBuffer.h"

class YT_DLP
{
public:
	enum yt_protocol_type {
		protocol_unknoun = -1,
		protocol_http = 0,
		protocol_dash,
		protocol_hls,
	};
	enum yt_vcodec_type {
		vcodec_none = -2,
		vcodec_unknoun = -1,
		vcodec_h264 = 0,
		vcodec_vp9,
		vcodec_av1,
	};
	enum yt_acodec_type {
		acodec_none = -2,
		acodec_unknoun = -1,
		acodec_aac = 0,
		acodec_opus,
	};

	struct yt_vformat_t {
		CStringA id;
		yt_protocol_type protocol = protocol_unknoun;
		yt_vcodec_type codec = vcodec_unknoun;
		yt_acodec_type audio = acodec_none;
		int height = 0;
		int fps = 0;
		bool hdr = false;
		float bitrate = 0;
		CStringW url;
		CStringW desc;
		CStringA ext;
		CStringA language;
		//int language_preference = 0;
		UINT menuflags = 0;
	};

	struct yt_aformat_t {
		CStringA id;
		yt_protocol_type protocol = protocol_unknoun;
		yt_acodec_type codec = acodec_unknoun;
		float bitrate = 0;
		CStringW url;
		CStringW desc;
		CStringA ext;
		CStringA language;
		//int language_preference = 0;
		UINT menuflags = 0;
	};

	struct yt_sub_t {
		CStringW url;
		CStringW name;
		CStringA ext;
		CStringA language;
	};

	struct yt_thumb_t {
		CStringW url;
		CStringA ext;
		CSimpleBlock<uint8_t> data;

		void Clear() {
			url.Empty();
			ext.Empty();
			data.SetSize(0);
		}
	};

	static LPCWSTR CheckVideoURL(CStringW url);
	static bool CheckNonYtPlaylistURL(CString url);

	static bool Parse_Playlist(const CStringW& pls_url, CFileItemList& playlist, int& idx_CurrentPlay);

public:
	CStringW        mAuthor;
	CStringW        mTitle;
	CStringW        mDescription;
	SYSTEMTIME      mTime = { 0 };
	REFERENCE_TIME  mDuration = 0;
	bool            mIsLive = false;
	CStringW        mUserAgent;

	std::vector<Chapters> mChapters;

private:
	yt_thumb_t      mThumbnail;
	std::vector<yt_sub_t> mSubtitles;

	std::vector<yt_vformat_t> mVideoFormats;
	std::vector<yt_aformat_t> mAudioFormats;

	UINT mIdxVideo = UINT_MAX;
	UINT mIdxAudio = UINT_MAX;

	UINT mHttpVideoCount = 0;
	UINT mDashVideoCount = 0;

	bool SetFormats(const rapidjson::Document& doc);
	void SetSubtitles(const rapidjson::Document& doc);
	void SetInfo(const rapidjson::Document& doc);

	UINT GetVideo(
		const yt_protocol_type protocol,
		const yt_vcodec_type vcodec,
		const int maxHeight,
		const bool bHighFps,
		const bool bHDR,
		const bool bHighBitrate);
	UINT GetAudio(const yt_protocol_type protocol, yt_acodec_type acodec, const bool bHighBitrate);
	bool SelectStreams();

	////////////////////////////////////////////////
public:
	void Clear();

	bool Parse_URL(const CStringW& url);
	bool ChangeFormats(UINT index);
	bool FillOFD(OpenFileData& ofd) const;
	bool DownloadThumbnail();

	UINT GetFormatsCount() const;
	LPCWSTR GetFormatDesc(UINT index, UINT& menuflags) const;
	LPCWSTR GetMainStreamUrl(UINT index) const;
	CStringW GetFilename() const;

	UINT GetFilesCount() const;

	const yt_vformat_t* GetVFormat() const;
	const yt_aformat_t* GetAFormat() const;
	const std::vector<yt_sub_t>& GetSubtitles() const;
	const yt_thumb_t* GetThumbnail() const;
};

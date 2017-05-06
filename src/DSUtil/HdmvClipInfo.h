/*
 * (C) 2006-2017 see Authors.txt
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

#include <vector>
#include "Mpeg2Def.h"
#include "../../include/basestruct.h"

enum BDVM_VideoFormat {
	BDVM_VideoFormat_Unknown = 0,
	BDVM_VideoFormat_480i    = 1,
	BDVM_VideoFormat_576i    = 2,
	BDVM_VideoFormat_480p    = 3,
	BDVM_VideoFormat_1080i   = 4,
	BDVM_VideoFormat_720p    = 5,
	BDVM_VideoFormat_1080p   = 6,
	BDVM_VideoFormat_576p    = 7,
	BDVM_VideoFormat_2160p   = 8,
};

enum BDVM_FrameRate {
	BDVM_FrameRate_Unknown = 0,
	BDVM_FrameRate_23_976  = 1,
	BDVM_FrameRate_24      = 2,
	BDVM_FrameRate_25      = 3,
	BDVM_FrameRate_29_97   = 4,
	BDVM_FrameRate_50      = 6,
	BDVM_FrameRate_59_94   = 7
};

enum BDVM_AspectRatio {
	BDVM_AspectRatio_Unknown = 0,
	BDVM_AspectRatio_4_3     = 2,
	BDVM_AspectRatio_16_9    = 3,
	BDVM_AspectRatio_2_21    = 4
};

enum BDVM_ChannelLayout {
	BDVM_ChannelLayout_Unknown = 0,
	BDVM_ChannelLayout_MONO    = 1,
	BDVM_ChannelLayout_STEREO  = 3,
	BDVM_ChannelLayout_MULTI   = 6,
	BDVM_ChannelLayout_COMBO   = 12
};

enum BDVM_SampleRate {
	BDVM_SampleRate_Unknown = 0,
	BDVM_SampleRate_48      = 1,
	BDVM_SampleRate_96      = 4,
	BDVM_SampleRate_192     = 5,
	BDVM_SampleRate_48_192  = 12,
	BDVM_SampleRate_48_96   = 14
};

class CHdmvClipInfo
{
public:

	struct Stream {
		SHORT              m_PID  = 0;
		PES_STREAM_TYPE    m_Type = INVALID;
		char               m_LanguageCode[4] = {};
		LCID               m_LCID = 0;

		// Valid for video types
		BDVM_VideoFormat   m_VideoFormat = BDVM_VideoFormat_Unknown;
		BDVM_FrameRate     m_FrameRate   = BDVM_FrameRate_Unknown;
		BDVM_AspectRatio   m_AspectRatio = BDVM_AspectRatio_Unknown;
		// Valid for audio types
		BDVM_ChannelLayout m_ChannelLayout = BDVM_ChannelLayout_Unknown;
		BDVM_SampleRate    m_SampleRate    = BDVM_SampleRate_Unknown;
	};

	struct PlaylistItem {
		PlaylistItem() {}

		PlaylistItem(const PlaylistItem& pi) {
			*this = pi;
		}

		CString					m_strFileName;
		REFERENCE_TIME			m_rtIn        = 0;
		REFERENCE_TIME			m_rtOut       = 0;
		REFERENCE_TIME			m_rtStartTime = 0;

		__int64					m_SizeIn      = 0;
		__int64					m_SizeOut     = 0;

		BYTE					m_num_video   = 0;

		std::vector<BYTE>		m_pg_offset_sequence_id;
		std::vector<BYTE>		m_ig_offset_sequence_id;

		CAtlArray<SyncPoint>	m_sps;

		REFERENCE_TIME Duration() const {
			return m_rtOut - m_rtIn;
		}

		__int64 Size() const {
			return m_SizeOut - m_SizeIn;
		}

		bool operator == (const PlaylistItem& pi) const {
			return pi.m_strFileName == m_strFileName;
		}

		PlaylistItem& operator = (const PlaylistItem& pi) {
			m_strFileName			= pi.m_strFileName;
			m_rtIn					= pi.m_rtIn;
			m_rtOut					= pi.m_rtOut;
			m_rtStartTime			= pi.m_rtStartTime;
			m_SizeIn				= pi.m_SizeIn;
			m_SizeOut				= pi.m_SizeOut;
			m_num_video				= pi.m_num_video;

			m_pg_offset_sequence_id	= pi.m_pg_offset_sequence_id;
			m_ig_offset_sequence_id	= pi.m_ig_offset_sequence_id;

			m_sps.Copy(pi.m_sps);

			return *this;
		}
	};

	enum PlaylistMarkType
	{
		Reserved				= 0x00,
		EntryMark				= 0x01,
		LinkPoint				= 0x02
	};

	struct PlaylistChapter {
		PlaylistChapter() {
			memset(this, 0, sizeof(*this));
		}

		SHORT					m_nPlayItemId;
		PlaylistMarkType		m_nMarkType;
		REFERENCE_TIME			m_rtTimestamp;
		SHORT					m_nEntryPID;
		REFERENCE_TIME			m_rtDuration;
	};

	typedef CAutoPtrList<PlaylistItem>	CPlaylist;
	typedef CAtlList<PlaylistChapter>	CPlaylistChapter;

	CHdmvClipInfo();
	~CHdmvClipInfo();

	HRESULT		ReadInfo(LPCTSTR strFile, CAtlArray<SyncPoint>* sps = NULL);
	Stream*		FindStream(SHORT wPID);
	bool		IsHdmv() const { return m_bIsHdmv; };
	size_t		GetStreamCount() const { return m_Streams.GetCount(); };
	Stream*		GetStreamByIndex(size_t nIndex) {return (nIndex < m_Streams.GetCount()) ? &m_Streams[nIndex] : NULL; };

	HRESULT		FindMainMovie(LPCTSTR strFolder, CString& strPlaylistFile, CPlaylist& MainPlaylist, CPlaylist& MPLSPlaylists);
	HRESULT		ReadPlaylist(CString strPlaylistFile, REFERENCE_TIME& rtDuration, CPlaylist& Playlist, BOOL bFullInfoRead = FALSE, BYTE* MVC_Base_View_R_flag = NULL);
	HRESULT		ReadChapters(CString strPlaylistFile, CPlaylist& PlaylistItems, CPlaylistChapter& Chapters);

private :
	DWORD		ProgramInfo_start_address;
	DWORD		Cpi_start_addrress;
	DWORD		Ext_data_start_address;

	HANDLE		m_hFile;

	CAtlArray<Stream>	m_Streams;
	bool				m_bIsHdmv;

	void		ReadBuffer(BYTE* pBuff, DWORD nLen);
	DWORD		ReadDword();
	SHORT		ReadShort();
	BYTE		ReadByte();

	BOOL		Skip(LONGLONG nLen);
	BOOL		GetPos(LONGLONG& Pos);
	BOOL		SetPos(LONGLONG Pos, DWORD dwMoveMethod = FILE_BEGIN);

	HRESULT		ReadProgramInfo();
	HRESULT		ReadCpiInfo(CAtlArray<SyncPoint>* sps);
	HRESULT		CloseFile(HRESULT hr);

private:
	struct ClpiEpCoarse {
		ClpiEpCoarse() {
			memset(this, 0, sizeof(*this));
		}

		WORD			ref_ep_fine_id;
		WORD			pts_ep;
		DWORD			spn_ep;
	};

	struct ClpiEpFine {
		ClpiEpFine() {
			memset(this, 0, sizeof(*this));
		}

		BYTE			is_angle_change_point;
		BYTE			i_end_position_offset;
		SHORT			pts_ep;
		WORD			spn_ep;
	};

	struct ClpiEpMapEntry {
		SHORT			pid;
		BYTE			ep_stream_type;
		SHORT			num_ep_coarse;
		WORD			num_ep_fine;
		DWORD			ep_map_stream_start_addr;

		ClpiEpCoarse*	coarse;
		ClpiEpFine*		fine;

		ClpiEpMapEntry() {
			pid							= 0;
			ep_stream_type				= 0;
			num_ep_coarse				= 0;
			num_ep_fine					= 0;
			ep_map_stream_start_addr	= 0;

			coarse						= NULL;
			fine						= NULL;
		}
	};
};

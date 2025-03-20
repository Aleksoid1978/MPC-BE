/*
 * (C) 2006-2025 see Authors.txt
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
#include "HdmvClipInfo.h"
#include "DSUtil.h"
#include "GolombBuffer.h"
#include "FileHandle.h"
#include <sys\stat.h>
#include <regex>

extern LCID ISO6392ToLcid(LPCSTR code);

CHdmvClipInfo::CHdmvClipInfo()
{
}

CHdmvClipInfo::~CHdmvClipInfo()
{
	CloseFile(S_OK);
}

HRESULT CHdmvClipInfo::CloseFile(HRESULT hr)
{
	if (m_hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}

	return hr;
}

void CHdmvClipInfo::ReadBuffer(BYTE* pBuff, DWORD nLen)
{
	DWORD dwRead;
	ReadFile(m_hFile, pBuff, nLen, &dwRead, nullptr);
}

DWORD CHdmvClipInfo::ReadDword()
{
	DWORD value;
	ReadBuffer((BYTE*)&value, sizeof(value));

	return _byteswap_ulong(value);
}

SHORT CHdmvClipInfo::ReadShort()
{
	WORD value;
	ReadBuffer((BYTE*)&value, sizeof(value));

	return _byteswap_ushort(value);
}

BYTE CHdmvClipInfo::ReadByte()
{
	BYTE value;
	ReadBuffer((BYTE*)&value, sizeof(value));

	return value;
}

BOOL CHdmvClipInfo::Skip(LONGLONG nLen)
{
	LARGE_INTEGER newPos = {};
	newPos.QuadPart = nLen;
	return SetFilePointerEx(m_hFile, newPos, nullptr, FILE_CURRENT);
}

BOOL CHdmvClipInfo::GetPos(LONGLONG& Pos)
{
	LARGE_INTEGER curPos = {};
	const BOOL bRet = SetFilePointerEx(m_hFile, curPos, &curPos, FILE_CURRENT);
	Pos = curPos.QuadPart;

	return bRet;
}
BOOL CHdmvClipInfo::SetPos(LONGLONG Pos, DWORD dwMoveMethod/* = FILE_BEGIN*/)
{
	LARGE_INTEGER newPos = {};
	newPos.QuadPart = Pos;
	return SetFilePointerEx(m_hFile, newPos, nullptr, dwMoveMethod);
}

HRESULT CHdmvClipInfo::ReadLang(Stream& s)
{
	ReadBuffer((BYTE*)s.m_LanguageCode, 3);
	s.m_LCID = ISO6392ToLcid(s.m_LanguageCode);

	return S_OK;
}

HRESULT CHdmvClipInfo::ReadProgramInfo()
{
	SetPos(ProgramInfo_start_address);

	ReadDword();	//length
	ReadByte();		//reserved_for_word_align
	const BYTE number_of_program_sequences = ReadByte();
	for (size_t i = 0; i < number_of_program_sequences; i++) {
		ReadDword();	//SPN_program_sequence_start
		ReadShort();	//program_map_PID
		const BYTE number_of_streams_in_ps = ReadByte(); //number_of_streams_in_ps
		ReadByte();		//reserved_for_future_use

		LONGLONG Pos = 0;
		for (size_t stream_index = 0; stream_index < number_of_streams_in_ps; stream_index++) {
			Stream s;
			s.m_PID = ReadShort();	// stream_PID

			// StreamCodingInfo
			GetPos(Pos);
			Pos += ReadByte() + 1;	// length
			s.m_Type = (PES_STREAM_TYPE)ReadByte();

			BOOL bFoundStream = FALSE;
			for (const auto& stream : m_Streams) {
				if (s.m_PID == stream.m_PID) {
					SetPos(Pos);
					bFoundStream = TRUE;
					break;
				}
			}

			if (bFoundStream) {
				continue;
			}

			switch (s.m_Type) {
				case VIDEO_STREAM_MPEG1:
				case VIDEO_STREAM_MPEG2:
				case VIDEO_STREAM_H264:
				case VIDEO_STREAM_H264_MVC:
				case VIDEO_STREAM_HEVC:
				case VIDEO_STREAM_VC1: {
						BYTE Temp       = ReadByte();
						s.m_VideoFormat = (BDVM_VideoFormat)(Temp >> 4);
						s.m_FrameRate   = (BDVM_FrameRate)(Temp & 0xf);
						Temp            = ReadByte();
						s.m_AspectRatio = (BDVM_AspectRatio)(Temp >> 4);
					}
					break;
				case AUDIO_STREAM_MPEG1:
				case AUDIO_STREAM_MPEG2:
				case AUDIO_STREAM_LPCM:
				case AUDIO_STREAM_AC3:
				case AUDIO_STREAM_DTS:
				case AUDIO_STREAM_AC3_TRUE_HD:
				case AUDIO_STREAM_AC3_PLUS:
				case AUDIO_STREAM_DTS_HD:
				case AUDIO_STREAM_DTS_HD_MASTER_AUDIO:
				case SECONDARY_AUDIO_AC3_PLUS:
				case SECONDARY_AUDIO_DTS_HD: {
						BYTE Temp         = ReadByte();
						s.m_ChannelLayout = (BDVM_ChannelLayout)(Temp >> 4);
						s.m_SampleRate    = (BDVM_SampleRate)(Temp & 0xF);

						ReadLang(s);
					}
					break;
				case PRESENTATION_GRAPHICS_STREAM:
				case INTERACTIVE_GRAPHICS_STREAM:
					ReadLang(s);
					break;
				case SUBTITLE_STREAM:
					ReadByte(); // bd_char_code
					ReadLang(s);
					break;
				default :
					break;
			}

			m_Streams.emplace_back(s);
			SetPos(Pos);
		}
	}

	return S_OK;
}

HRESULT CHdmvClipInfo::ReadCpiInfo(SyncPoints& sps)
{
	sps.clear();

	struct ClpiEpCoarse {
		UINT  ref_ep_fine_id;
		WORD  pts_ep;
		DWORD spn_ep;
	};

	struct ClpiEpFine {
		BYTE is_angle_change_point;
		BYTE i_end_position_offset;
		WORD pts_ep;
		UINT spn_ep;
	};

	struct ClpiEpMapEntry {
		WORD  pid;
		BYTE  ep_stream_type;
		WORD  num_ep_coarse;
		UINT  num_ep_fine;
		DWORD ep_map_stream_start_addr;

		std::vector<ClpiEpCoarse> coarse;
		std::vector<ClpiEpFine>   fine;
	};
	std::vector<ClpiEpMapEntry> ClpiEpMapList;

	SetPos(Cpi_start_addrress);

	DWORD len = ReadDword();
	if (len == 0) {
		return E_FAIL;
	}

	ReadByte();
	const BYTE Type = ReadByte() & 0xF;
	const DWORD ep_map_pos = Cpi_start_addrress + 4 + 2;
	ReadByte();
	const BYTE num_stream_pid = ReadByte();

	DWORD size = num_stream_pid * 12;
	auto buf = std::make_unique<BYTE[]>(size);
	ReadBuffer(buf.get(), size);
	CGolombBuffer gb(buf.get(), size);
	for (BYTE i = 0; i < num_stream_pid; i++) {
		ClpiEpMapEntry em;

		em.pid                      = gb.BitRead(16);
		gb.BitRead(10);
		em.ep_stream_type           = gb.BitRead(4);
		em.num_ep_coarse            = gb.BitRead(16);
		em.num_ep_fine              = gb.BitRead(18);
		em.ep_map_stream_start_addr = gb.ReadDword() + ep_map_pos;

		em.coarse.resize(em.num_ep_coarse);
		em.fine.resize(em.num_ep_fine);

		ClpiEpMapList.emplace_back(em);
	}

	for (auto& em : ClpiEpMapList) {
		SetPos(em.ep_map_stream_start_addr);
		const DWORD fine_start = ReadDword();

		size = em.num_ep_coarse * 8;
		buf = std::make_unique<BYTE[]>(size);
		ReadBuffer(buf.get(), size);
		gb.Reset(buf.get(), size);

		for (WORD j = 0; j < em.num_ep_coarse; j++) {
			em.coarse[j].ref_ep_fine_id = gb.BitRead(18);
			em.coarse[j].pts_ep         = gb.BitRead(14);
			em.coarse[j].spn_ep         = gb.ReadDword();
		}

		SetPos(em.ep_map_stream_start_addr + fine_start);

		size = em.num_ep_fine * 4;
		buf = std::make_unique<BYTE[]>(size);
		ReadBuffer(buf.get(), size);
		gb.Reset(buf.get(), size);

		for (UINT j = 0; j < em.num_ep_fine; j++) {
			em.fine[j].is_angle_change_point = gb.BitRead(1);
			em.fine[j].i_end_position_offset = gb.BitRead(3);
			em.fine[j].pts_ep                = gb.BitRead(11);
			em.fine[j].spn_ep                = gb.BitRead(17);
		}
	}

	if (!ClpiEpMapList.empty()) {
		const auto& entry = ClpiEpMapList[0];
		for (WORD coarse_index = 0; coarse_index < entry.num_ep_coarse; coarse_index++) {
			const auto& coarse = entry.coarse[coarse_index];
			const auto& start = coarse.ref_ep_fine_id;
			const auto& end = (coarse_index < entry.num_ep_coarse - 1) ? entry.coarse[coarse_index + 1].ref_ep_fine_id : entry.num_ep_fine;
			const auto coarse_pts = (uint32_t)(coarse.pts_ep & ~0x01) << 18;

			for (UINT fine_index = start; fine_index < end; fine_index++) {
				const auto pts = coarse_pts + ((uint32_t)entry.fine[fine_index].pts_ep << 8);
				const auto spn = (coarse.spn_ep & ~0x1FFFF) + entry.fine[fine_index].spn_ep;

				sps.emplace_back(llMulDiv(pts, 2000, 9, 0), static_cast<__int64>(spn) * 192 + 4);
			}
		}
	}

	return S_OK;
}

#define dwShareMode          FILE_SHARE_READ | FILE_SHARE_WRITE
#define dwFlagsAndAttributes FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN

#define CheckVer() (!(memcmp(Buff, "0300", 4)) || (!memcmp(Buff, "0200", 4)) || (!memcmp(Buff, "0100", 4)))

HRESULT CHdmvClipInfo::ReadInfo(LPCWSTR strFile, SyncPoints* sps/* = nullprt*/)
{
	m_hFile = CreateFileW(strFile, GENERIC_READ, dwShareMode, nullptr,
						  OPEN_EXISTING, dwFlagsAndAttributes, nullptr);
	if (m_hFile != INVALID_HANDLE_VALUE) {
		BYTE Buff[4] = { 0 };

		ReadBuffer(Buff, 4);
		if (memcmp(Buff, "HDMV", 4)) {
			return CloseFile(VFW_E_INVALID_FILE_FORMAT);
		}

		ReadBuffer(Buff, 4);
		if (!CheckVer()) {
			return CloseFile(VFW_E_INVALID_FILE_FORMAT);
		}

		ReadDword(); // sequence_info_start_address
		ProgramInfo_start_address  = ReadDword();
		Cpi_start_addrress         = ReadDword();
		ReadDword(); // clip_mark_start_address
		Ext_data_start_address     = ReadDword();

		ReadProgramInfo();

		if (Ext_data_start_address) {
			SetPos(Ext_data_start_address);
			ReadDword();					// lenght
			ReadDword();					// start address
			ReadShort(); ReadByte();		// padding
			BYTE nNumEntries = ReadByte();	// number of entries
			for (BYTE i = 0; i < nNumEntries; i++) {
				UINT id1 = ReadShort();
				UINT id2 = ReadShort();
				LONGLONG extsubPos = Ext_data_start_address;
				extsubPos += ReadDword();	// Extension_start_address
				ReadDword();				// Extension_length
				if (id1 == 2) {
					if (id2 == 5) {
						ProgramInfo_start_address = extsubPos;
						ReadProgramInfo();
						break;
					}
				}
			}
		}

		if (sps) {
			ReadCpiInfo(*sps);
		}

		return CloseFile(S_OK);
	}

	return AmHresultFromWin32(GetLastError());
}

const CHdmvClipInfo::Stream* CHdmvClipInfo::FindStream(SHORT wPID)
{
	for (const auto& stream : GetStreams()) {
		if (stream.m_PID == wPID) {
			return &stream;
		}
	}

	return nullptr;
}

HRESULT CHdmvClipInfo::ReadStreamInfo()
{
	BYTE len = ReadByte();
	LONGLONG Pos = 0;
	GetPos(Pos);

	Stream s;
	const BYTE stream_type = ReadByte();
	switch (stream_type) {
		case 1:
			s.m_PID = ReadShort();
			break;
		case 2:
		case 4:
			ReadShort();
			s.m_PID = ReadShort();
			break;
		case 3:
			ReadByte();
			s.m_PID = ReadShort();
			break;
	}

	if (!SetPos(Pos + len)) {
		return E_FAIL;
	}

	len = ReadByte();
	GetPos(Pos);

	for (const auto& stream : stn.m_Streams) {
		if (s.m_PID == stream.m_PID) {
			return SetPos(Pos + len) ? S_OK : E_FAIL;
		}
	}

	s.m_Type = (PES_STREAM_TYPE)ReadByte();
	switch (s.m_Type) {
		case VIDEO_STREAM_MPEG1:
		case VIDEO_STREAM_MPEG2:
		case VIDEO_STREAM_H264:
		case VIDEO_STREAM_H264_MVC:
		case VIDEO_STREAM_HEVC:
		case VIDEO_STREAM_VC1: {
				const BYTE Temp = ReadByte();
				s.m_VideoFormat = (BDVM_VideoFormat)(Temp >> 4);
				s.m_FrameRate   = (BDVM_FrameRate)(Temp & 0xf);
			}
			break;
		case AUDIO_STREAM_MPEG1:
		case AUDIO_STREAM_MPEG2:
		case AUDIO_STREAM_LPCM:
		case AUDIO_STREAM_AC3:
		case AUDIO_STREAM_DTS:
		case AUDIO_STREAM_AC3_TRUE_HD:
		case AUDIO_STREAM_AC3_PLUS:
		case AUDIO_STREAM_DTS_HD:
		case AUDIO_STREAM_DTS_HD_MASTER_AUDIO:
		case SECONDARY_AUDIO_AC3_PLUS:
		case SECONDARY_AUDIO_DTS_HD: {
				const BYTE Temp   = ReadByte();
				s.m_ChannelLayout = (BDVM_ChannelLayout)(Temp >> 4);
				s.m_SampleRate    = (BDVM_SampleRate)(Temp & 0xF);

				ReadLang(s);
			}
			break;
		case PRESENTATION_GRAPHICS_STREAM:
		case INTERACTIVE_GRAPHICS_STREAM:
			ReadLang(s);
			break;
		case SUBTITLE_STREAM:
			ReadByte(); // bd_char_code
			ReadLang(s);
			break;
	}

	stn.m_Streams.emplace_back(s);

	return SetPos(Pos + len) ? S_OK : E_FAIL;
}

HRESULT CHdmvClipInfo::ReadSTNInfo()
{
	ReadShort(); // length
	ReadShort(); // reserved_for_future_use

	stn.num_video           = ReadByte(); // number of Primary Video Streams
	stn.num_audio           = ReadByte(); // number of Primary Audio Streams
	stn.num_pg              = ReadByte(); // number of Presentation Graphic Streams
	stn.num_ig              = ReadByte(); // number of Interactive Graphic Streams
	stn.num_secondary_audio = ReadByte(); // number of Secondary Audio Streams
	stn.num_secondary_video = ReadByte(); // number of Secondary Video Streams
	stn.num_pip_pg          = ReadByte(); // number of Presentation Graphic Streams

	Skip(5); // reserved_for_future_use

	for (BYTE i = 0; i < stn.num_video; i++) {
		if (FAILED(ReadStreamInfo())) {
			return E_FAIL;
		}
	}

	for (BYTE i = 0; i < stn.num_audio; i++) {
		if (FAILED(ReadStreamInfo())) {
			return E_FAIL;
		}
	}

	for (BYTE i = 0; i < (stn.num_pg + stn.num_pip_pg); i++) {
		if (FAILED(ReadStreamInfo())) {
			return E_FAIL;
		}
	}

	for (BYTE i = 0; i < stn.num_ig; i++) {
		if (FAILED(ReadStreamInfo())) {
			return E_FAIL;
		}
	}

	for (BYTE i = 0; i < stn.num_secondary_audio; i++) {
		if (FAILED(ReadStreamInfo())) {
			return E_FAIL;
		}

		// Secondary Audio Extra Attributes
		const BYTE num_secondary_audio_extra = ReadByte();
		ReadByte();
		if (num_secondary_audio_extra) {
			Skip(num_secondary_audio_extra);
			if (num_secondary_audio_extra % 2) {
				ReadByte();
			}
		}
	}

	for (BYTE i = 0; i < stn.num_secondary_video; i++) {
		if (FAILED(ReadStreamInfo())) {
			return E_FAIL;
		}

		// Secondary Video Extra Attributes
		const BYTE num_secondary_video_extra = ReadByte();
		ReadByte();
		if (num_secondary_video_extra) {
			Skip(num_secondary_video_extra);
			if (num_secondary_video_extra % 2) {
				ReadByte();
			}
		}

		const BYTE num_pip_pg_extra = ReadByte();
		ReadByte();
		if (num_pip_pg_extra) {
			Skip(num_pip_pg_extra);
			if (num_pip_pg_extra % 2) {
				ReadByte();
			}
		}
	}

	return S_OK;
}

HRESULT CHdmvClipInfo::ReadPlaylist(const CString& strPlaylistFile, REFERENCE_TIME& rtDuration, CPlaylist& Playlist, BOOL bReadMVCExtension/* = FALSE*/, BOOL bFullInfoRead/* = FALSE*/, BYTE* MVC_Base_View_R_flag/* = nullptr*/)
{
	rtDuration = 0;

	m_Streams.clear();
	stn.m_Streams.clear();

	m_hFile = CreateFileW(strPlaylistFile, GENERIC_READ, dwShareMode, nullptr,
						  OPEN_EXISTING, dwFlagsAndAttributes, nullptr);
	if (m_hFile != INVALID_HANDLE_VALUE) {
		bool bDuplicate = false;
		BYTE Buff[9] = { 0 };

		ReadBuffer(Buff, 4);
		if (memcmp(Buff, "MPLS", 4)) {
			return CloseFile(VFW_E_INVALID_FILE_FORMAT);
		}

		ReadBuffer(Buff, 4);
		if (!CheckVer()) {
			return CloseFile(VFW_E_INVALID_FILE_FORMAT);
		}

		LARGE_INTEGER size = {};
		GetFileSizeEx(m_hFile, &size);
		Playlist.m_mpls_size = size.QuadPart;

		DLog(L"CHdmvClipInfo::ReadPlaylist() : '%s'", strPlaylistFile);

		// Get BDMV folder
		CStringW Path = GetFolderPath(strPlaylistFile);
		RemoveFileSpec(Path); // folder above

		LONGLONG playlistPos = 0;
		LONGLONG extPos = 0;
		USHORT   nPlaylistItems = 0;

		playlistPos = ReadDword();	// PlayList_start_address
		ReadDword();				// PlayListMark_start_address
		extPos = ReadDword();		// Extension_start_address

		if (MVC_Base_View_R_flag) {
			// skip 20 bytes
			Skip(20);

			// App info
			ReadDword();				// lenght
			ReadByte();					// reserved
			ReadByte();					// playback type
			ReadShort();				// playback count
			Skip(8);					// mpls uo
			BYTE flags = ReadByte();	// flags
			*MVC_Base_View_R_flag = (flags >> 4) & 0x1;
		}

		struct ext_sub_path {
			BYTE type;
			BYTE playitem_count;
			std::vector<CString> extFileNames;
		};
		std::vector<ext_sub_path> ext_sub_paths;

		// Extension
		LONGLONG stnssextPos = 0;
		if (extPos && bReadMVCExtension) {
			SetPos(extPos);
			ReadDword();					// lenght
			ReadDword();					// start address
			ReadShort(); ReadByte();		// padding
			BYTE nNumEntries = ReadByte();	// number of entries
			for (BYTE i = 0; i < nNumEntries; i++) {
				UINT id1 = ReadShort();
				UINT id2 = ReadShort();
				LONGLONG extsubPos = extPos;
				extsubPos += ReadDword();	// Extension_start_address
				ReadDword();				// Extension_length

				if (id1 == 2) {
					if (id2 == 1) {
						stnssextPos = extsubPos;
					} else if (id2 == 2) {
						LONGLONG saveextPos = 0;
						GetPos(saveextPos);

						SetPos(extsubPos);
						ReadDword();									// lenght
						const USHORT ExtSubPathsItems = ReadShort();	// number_of_ExtSubPaths
						for (USHORT k = 0; k < ExtSubPathsItems; k++) {
							const DWORD lenght = ReadDword();

							LONGLONG saveextpathPos = 0;
							GetPos(saveextpathPos);
							saveextpathPos += lenght;

							ext_sub_path _ext_sub_path;
							ReadByte();
							_ext_sub_path.type = ReadByte();
							ReadShort();ReadByte();
							_ext_sub_path.playitem_count = ReadByte();

							LONGLONG savePos = 0;
							for (BYTE ii = 0; ii < _ext_sub_path.playitem_count; ii++) {
								GetPos(savePos);
								savePos += ReadShort() + 2;
								ReadBuffer(Buff, 9); // M2TS file name
								if (!memcmp(&Buff[5], "M2TS", 4)) {
									CString fileName;
									fileName.Format(L"%s\\STREAM\\%c%c%c%c%c.M2TS", Path, Buff[0], Buff[1], Buff[2], Buff[3], Buff[4]);
									_ext_sub_path.extFileNames.emplace_back(fileName);
								}

								SetPos(savePos);
							}

							ext_sub_paths.emplace_back(_ext_sub_path);

							SetPos(saveextpathPos);
						}

						SetPos(saveextPos);
					}
				}
			}
		}

		// PlayList
		SetPos(playlistPos);
		ReadDword();						// length
		ReadShort();						// reserved_for_future_use
		nPlaylistItems = ReadShort();		// number_of_PlayItems
		ReadShort();						// number_of_SubPaths

		BOOL bHaveMVCExtension = FALSE;
		for (const auto& ext_sub : ext_sub_paths) {
			if (ext_sub.type == 8 && ext_sub.playitem_count == nPlaylistItems) {
				bHaveMVCExtension = TRUE;
				break;
			}
		}

		playlistPos += 10;
		__int64 TotalSize = 0;
		for (size_t i = 0; i < nPlaylistItems; i++) {
			SetPos(playlistPos);
			playlistPos += ReadShort() + 2;
			ReadBuffer(Buff, 9); // M2TS file name
			if (memcmp(&Buff[5], "M2TS", 4)) {
				return CloseFile(VFW_E_INVALID_FILE_FORMAT);
			}

			LPCWSTR format = L"%s\\STREAM\\%c%c%c%c%c.M2TS";
			if (bHaveMVCExtension) {
				format = L"%s\\STREAM\\SSIF\\%c%c%c%c%c.SSIF";
			}

			PlaylistItem Item;
			Item.m_strFileName.Format(format, Path, Buff[0], Buff[1], Buff[2], Buff[3], Buff[4]);
			if (!::PathFileExistsW(Item.m_strFileName)) {
				DLog(L"    ==> '%s' is missing, skip it", Item.m_strFileName);

				stnssextPos = 0;
				continue;
			}

			ReadBuffer(Buff, 3);
			const BYTE is_multi_angle = (Buff[1] >> 4) & 0x1;

			Item.m_rtIn  = REFERENCE_TIME(20000.0f * ReadDword() / 90);
			Item.m_rtOut = REFERENCE_TIME(20000.0f * ReadDword() / 90);

			Item.m_rtStartTime	= rtDuration;

			rtDuration += (Item.m_rtOut - Item.m_rtIn);

			Skip(8);		// mpls uo
			ReadByte();
			ReadByte();		// still mode
			ReadShort();	// still time
			BYTE angle_count = 1;
			if (is_multi_angle) {
				angle_count = ReadByte();
				if (angle_count < 1) {
					angle_count = 1;
				}
				ReadByte();
			}
			for (BYTE j = 1; j < angle_count; j++) {
				Skip(9);    // M2TS file name
				ReadByte(); // stc_id
			}

			// stn
			ReadSTNInfo();

			Item.m_num_video = stn.num_video;
			Item.m_num_streams = stn.num_video + stn.num_audio + stn.num_pg + stn.num_pip_pg;

			Item.m_pg_offset_sequence_id.resize(stn.num_pg, 0xFF);
			Item.m_ig_offset_sequence_id.resize(stn.num_ig, 0xFF);

			if (bFullInfoRead) {
				size = {};
				HANDLE hFile = CreateFileW(Item.m_strFileName, GENERIC_READ, dwShareMode, nullptr,
										   OPEN_EXISTING, dwFlagsAndAttributes, nullptr);
				if (hFile != INVALID_HANDLE_VALUE) {
					GetFileSizeEx(hFile, &size);
					CloseHandle(hFile);
				}

				Item.m_SizeIn  = TotalSize;
				TotalSize     += size.QuadPart;
				Item.m_SizeOut = TotalSize;
			}

			if (std::find(Playlist.cbegin(), Playlist.cend(), Item) != Playlist.cend()) {
				bDuplicate = true;
			}

			CString sizeStr;
			if (bFullInfoRead) {
				sizeStr.Format(L", Size : %I64d", Item.Size());
			}
			DLog(L"    ==> '%s', Duration : %s [%15I64d], Total duration : %s%s", Item.m_strFileName, ReftimeToString(Item.Duration()), Item.Duration(), ReftimeToString(rtDuration), sizeStr);

			Playlist.emplace_back(Item);
		}

		// stn ss extension
		if (bFullInfoRead && stnssextPos && !Playlist.empty()) {
			SetPos(stnssextPos);

			for (auto& Item : Playlist) {
				SHORT lenght = ReadShort();
				auto buf = std::make_unique<BYTE[]>(lenght);
				ReadBuffer(buf.get(), lenght);
				CGolombBuffer gb(buf.get(), lenght);

				int Fixed_offset_during_PopUp_flag = gb.BitRead(1);
				gb.BitRead(15); // reserved

				for (BYTE i = 0; i < Item.m_num_video; i++) {
					Stream s;

					// stream_entry
					BYTE slen = gb.ReadByte();
					gb.SkipBytes(3);
					s.m_PID = gb.ReadShort();
					slen -= 5;
					gb.SkipBytes(slen);

					// stream_attributes_ss
					slen = gb.ReadByte();
					s.m_Type = (PES_STREAM_TYPE)gb.ReadByte();
					s.m_VideoFormat = (BDVM_VideoFormat)gb.BitRead(4);
					s.m_FrameRate = (BDVM_FrameRate)gb.BitRead(4);
					slen -= 2;
					gb.SkipBytes(slen);

					gb.BitRead(10); // reserved
					gb.BitRead(6);  // number_of_offset_sequences

					auto it = std::find_if(stn.m_Streams.cbegin(), stn.m_Streams.cend(), [&s](const Stream& stream) {
						return stream.m_PID == s.m_PID;
					});
					if (it == stn.m_Streams.cend()) {
						stn.m_Streams.emplace_back(s);
					}
				}

				for (auto& id : Item.m_pg_offset_sequence_id) {
					id = gb.ReadByte();

					gb.BitRead(4); // reserved
					gb.BitRead(1); // dialog_region_offset_valid_flag
					const BYTE is_SS_PG = gb.BitRead(1);
					const BYTE is_top_AS_PG_textST = gb.BitRead(1);
					const BYTE is_bottom_AS_PG_textST = gb.BitRead(1);
					if (is_SS_PG) {
						// stream_entry left eye
						BYTE slen = gb.ReadByte();
						gb.SkipBytes(slen);

						// stream_entry right eye
						slen = gb.ReadByte();
						gb.SkipBytes(slen);

						gb.BitRead(8); // reserved
						gb.BitRead(8); // PG offset
					}

					if (is_top_AS_PG_textST) {
						// stream_entry
						BYTE slen = gb.ReadByte();
						gb.SkipBytes(slen);

						gb.BitRead(8); // reserved
						gb.BitRead(8); // PG offset
					}

					if (is_bottom_AS_PG_textST) {
						// stream_entry
						BYTE slen = gb.ReadByte();
						gb.SkipBytes(slen);

						gb.BitRead(8); // reserved
						gb.BitRead(8); // PG offset
					}
				}

				for (auto& id : Item.m_ig_offset_sequence_id) {
					if (Fixed_offset_during_PopUp_flag) {
						gb.ReadByte();
					} else {
						id = gb.ReadByte();
					}

					gb.BitRead(16); // IG_Plane_offset_during_BB_video
					gb.BitRead(7);  // reserved
					BYTE is_SS_IG = gb.BitRead(1);
					if (is_SS_IG) {
						// stream_entry left eye
						BYTE slen = gb.ReadByte();
						gb.SkipBytes(slen);

						// stream_entry right eye
						slen = gb.ReadByte();
						gb.SkipBytes(slen);

						gb.ReadByte(); // reserved
						gb.ReadByte(); // PG offset
					}
				}
			}
		}

		CloseFile(S_OK);

		if (bFullInfoRead && !Playlist.empty()) {
			for (auto& Item : Playlist) {
				CString fname = Item.m_strFileName;
				if (fname.Find(L".SSIF") > 0) {
					fname.Replace(L"\\STREAM\\SSIF\\", L"\\CLIPINF\\");
					fname.Replace(L".SSIF", L".CLPI");
				} else {
					fname.Replace(L"\\STREAM\\", L"\\CLIPINF\\");
					fname.Replace(L".M2TS", L".CLPI");
				}

				ReadInfo(fname, &Item.m_sps);
			}

			if (!ext_sub_paths.empty()) {
				for (auto& ext_sub : ext_sub_paths) {
					for (auto& extFileName : ext_sub.extFileNames) {
						CString fname = extFileName;
						fname.Replace(L"\\STREAM\\", L"\\CLIPINF\\");
						fname.Replace(L".M2TS", L".CLPI");

						ReadInfo(fname);
					}
				}
			}
		}

		if (!stn.m_Streams.empty()) {
			for (const auto& stream : stn.m_Streams) {
				switch (stream.m_VideoFormat) {
					case BDVM_VideoFormat_480i:
					case BDVM_VideoFormat_480p:
						Playlist.m_max_video_res = std::max(Playlist.m_max_video_res, 480u);
						break;
					case BDVM_VideoFormat_576i:
					case BDVM_VideoFormat_576p:
						Playlist.m_max_video_res = std::max(Playlist.m_max_video_res, 576u);
						break;
					case BDVM_VideoFormat_720p:
						Playlist.m_max_video_res = std::max(Playlist.m_max_video_res, 720u);
						break;
					case BDVM_VideoFormat_1080i:
					case BDVM_VideoFormat_1080p:
						Playlist.m_max_video_res = std::max(Playlist.m_max_video_res, 1080u);
						break;
					case BDVM_VideoFormat_2160p:
						Playlist.m_max_video_res = std::max(Playlist.m_max_video_res, 2160u);
						break;
					default:
						break;
				}
			}
		}

		return Playlist.empty() ? E_FAIL : bDuplicate ? S_FALSE : S_OK;
	}

	return AmHresultFromWin32(GetLastError());
}

HRESULT CHdmvClipInfo::ReadChapters(const CString& strPlaylistFile, const CPlaylist& PlaylistItems, CPlaylistChapter& Chapters)
{
	Chapters.clear();

	m_hFile = CreateFileW(strPlaylistFile, GENERIC_READ, dwShareMode, nullptr,
						  OPEN_EXISTING, dwFlagsAndAttributes, nullptr);
	if (m_hFile != INVALID_HANDLE_VALUE) {
		BYTE Buff[4] = { 0 };

		ReadBuffer(Buff, 4);
		if (memcmp(Buff, "MPLS", 4)) {
			return CloseFile(VFW_E_INVALID_FILE_FORMAT);
		}

		ReadBuffer(Buff, 4);
		if (!CheckVer()) {
			return CloseFile(VFW_E_INVALID_FILE_FORMAT);
		}

		std::vector<REFERENCE_TIME> rtOffsets;
		rtOffsets.reserve(PlaylistItems.size());
		REFERENCE_TIME rtDuration = 0;

		for (const auto& Item : PlaylistItems) {
			rtOffsets.emplace_back(rtDuration - Item.m_rtIn);
			rtDuration += Item.Duration();
		}

		ReadDword();                        // PlayList_start_address
		LONGLONG playlistPos = ReadDword(); // PlayListMark_start_address

		// PlayListMark()
		SetPos(playlistPos);
		ReadDword();                           // length
		const USHORT nMarkCount = ReadShort(); // number_of_PlayList_marks
		for (USHORT i = 0; i < nMarkCount; i++) {
			PlaylistChapter	Chapter;

			ReadByte();                                                          // reserved_for_future_use
			Chapter.m_nMarkType   = (PlaylistMarkType)ReadByte();                // mark_type
			Chapter.m_nPlayItemId = ReadShort();                                 // ref_to_PlayItem_id
			if (Chapter.m_nPlayItemId >= (SHORT)rtOffsets.size()) {
				break;
			}
			Chapter.m_rtTimestamp = REFERENCE_TIME(20000.0f * ReadDword() / 90)  // mark_time_stamp
								  + rtOffsets[Chapter.m_nPlayItemId];
			Chapter.m_nEntryPID   = ReadShort();                                 // entry_ES_PID
			Chapter.m_rtDuration  = REFERENCE_TIME(20000.0f * ReadDword() / 90); // duration

			if (Chapter.m_rtTimestamp < 0 || Chapter.m_rtTimestamp > rtDuration) {
				continue;
			}

			Chapters.emplace_back(Chapter);
		}

		if (!Chapters.empty()) {
			CString tnmt_xml_file = strPlaylistFile;
			tnmt_xml_file.Replace(L"\\PLAYLIST\\", L"\\META\\TN\\tnmt_eng_");
			tnmt_xml_file.Replace(L".mpls", L".xml");

			HANDLE m_hXmlFile = CreateFileW(tnmt_xml_file, GENERIC_READ, dwShareMode, nullptr,
											OPEN_EXISTING, dwFlagsAndAttributes, nullptr);
			if (m_hXmlFile != INVALID_HANDLE_VALUE) {
				LARGE_INTEGER size = {};
				GetFileSizeEx(m_hXmlFile, &size);
				if (size.QuadPart > 0 && size.QuadPart <= 10 * KILOBYTE) {
					char* xmlUtf8 = new(std::nothrow) char[size.QuadPart + 1];
					if (xmlUtf8) {
						DWORD dwRead;
						if (ReadFile(m_hXmlFile, xmlUtf8, size.QuadPart, &dwRead, nullptr) && dwRead == size.QuadPart) {
							xmlUtf8[size.QuadPart] = '\0';
							const CString xml = UTF8ToWStr(xmlUtf8);
							delete [] xmlUtf8;

							if (!xml.IsEmpty()) {
								std::vector<CString> names;

								std::wcmatch match;
								const wchar_t* start = xml.GetString();
								const std::wregex regex(L"<name>([^<>\\n]+)</name>");
								while (std::regex_search(start, match, regex) && match.size() == 2) {
									start = match[1].second;
									const CString name = CString(match[1].first, match[1].length());
									names.emplace_back(name);
								}

								if (names.size() == Chapters.size()) {
									for (size_t i = 0; i < Chapters.size(); i++) {
										Chapters[i].m_strTitle = names[i];
									}
								}
							}
						}
					}
				}

				CloseHandle(m_hXmlFile);
			}
		}

		return CloseFile(S_OK);
	}

	return AmHresultFromWin32(GetLastError());
}

HRESULT CHdmvClipInfo::FindMainMovie(LPCWSTR strFolder, CString& strPlaylistFile, CPlaylist& Playlists)
{
	HRESULT hr = E_FAIL;
	const CString strPath(strFolder);

	Playlists.clear();

	WIN32_FIND_DATA fd = {0};
	HANDLE hFind = FindFirstFileW(strPath + L"\\PLAYLIST\\*.mpls", &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		std::vector<CPlaylist> PlaylistArray;
		REFERENCE_TIME rtMax = 0;
		REFERENCE_TIME rtCurrent;
		CString        strCurrentPlaylist;
		__int64        mpls_size_max = 0;
		unsigned       max_video_res = 0;
		do {
			CPlaylist Playlist;
			strCurrentPlaylist = strPath + L"\\PLAYLIST\\" + fd.cFileName;
			// Main movie shouldn't have duplicate M2TS filename ...
			if (ReadPlaylist(strCurrentPlaylist, rtCurrent, Playlist) == S_OK) {
				if ((rtCurrent > rtMax && Playlist.m_max_video_res >= max_video_res)
						|| (rtCurrent == rtMax && Playlist.m_mpls_size > mpls_size_max)
						|| ((rtCurrent < rtMax && rtCurrent >= rtMax / 2) && Playlist.m_max_video_res > max_video_res)) {
					rtMax           = rtCurrent;
					mpls_size_max   = Playlist.m_mpls_size;
					max_video_res   = Playlist.m_max_video_res;
					strPlaylistFile = strCurrentPlaylist;
					hr = S_OK;
				}

				if (rtCurrent >= 2 * UNITS) { // 2 seconds
					// Search duplicate playlists ...
					bool duplicate = false;
					if (!Playlists.empty()) {
						for (const auto& item : PlaylistArray) {
							if (item.size() != Playlist.size()) {
								continue;
							}

							duplicate = true;
							for (size_t i = 0; i < item.size() && duplicate; i++) {
								if (item[i] == Playlist[i]) {
									continue;
								}

								duplicate = false;
							}

							if (duplicate) {
								duplicate = (item.m_mpls_size == Playlist.m_mpls_size);
							}
						}
					}
					if (duplicate) {
						continue;
					}

					PlaylistItem Item;
					Item.m_strFileName = strCurrentPlaylist;
					Item.m_rtOut       = rtCurrent;
					Item.m_SizeOut     = Playlist.m_mpls_size;
					Playlists.emplace_back(Item);

					PlaylistArray.emplace_back(Playlist);
				}
			}
		} while (FindNextFileW(hFind, &fd));

		FindClose(hFind);
	}

	if (Playlists.size() == 1) {
		Playlists.clear();
	}

	return hr;
}

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

#include "stdafx.h"
#include "HdmvClipInfo.h"
#include "DSUtil.h"
#include "GolombBuffer.h"

extern LCID	ISO6392ToLcid(LPCSTR code);

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
	ReadFile(m_hFile, pBuff, nLen, &dwRead, NULL);
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
	LARGE_INTEGER newPos = {0, 0};
	newPos.QuadPart = nLen;
	return SetFilePointerEx(m_hFile, newPos, NULL, FILE_CURRENT);
}

BOOL CHdmvClipInfo::GetPos(LONGLONG& Pos)
{
	LARGE_INTEGER curPos = {0, 0};
	const BOOL bRet = SetFilePointerEx(m_hFile, curPos, &curPos, FILE_CURRENT);
	Pos = curPos.QuadPart;

	return bRet;
}
BOOL CHdmvClipInfo::SetPos(LONGLONG Pos, DWORD dwMoveMethod/* = FILE_BEGIN*/)
{
	LARGE_INTEGER newPos = {0, 0};
	newPos.QuadPart = Pos;
	return SetFilePointerEx(m_hFile, newPos, NULL, dwMoveMethod);
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

			for (size_t k = 0; k < m_Streams.GetCount(); k++) {
				if (s.m_PID == m_Streams[k].m_PID) {
					SetPos(Pos);
					continue;
				}
			}

			switch (s.m_Type) {
				case VIDEO_STREAM_MPEG1:
				case VIDEO_STREAM_MPEG2:
				case VIDEO_STREAM_H264:
				case VIDEO_STREAM_H264_MVC:
				case VIDEO_STREAM_HEVC:
				case VIDEO_STREAM_VC1: {
						BYTE Temp = ReadByte();
						BDVM_VideoFormat VideoFormat     = (BDVM_VideoFormat)(Temp >> 4);
						BDVM_FrameRate FrameRate         = (BDVM_FrameRate)(Temp & 0xf);
						Temp                             = ReadByte();
						BDVM_AspectRatio AspectRatio     = (BDVM_AspectRatio)(Temp >> 4);

						s.m_VideoFormat = VideoFormat;
						s.m_FrameRate   = FrameRate;
						s.m_AspectRatio = AspectRatio;
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
						BYTE Temp = ReadByte();
						BDVM_ChannelLayout ChannelLayout = (BDVM_ChannelLayout)(Temp >> 4);
						BDVM_SampleRate SampleRate       = (BDVM_SampleRate)(Temp & 0xF);

						ReadBuffer((BYTE*)s.m_LanguageCode, 3);
						s.m_LCID          = ISO6392ToLcid(s.m_LanguageCode);
						s.m_ChannelLayout = ChannelLayout;
						s.m_SampleRate    = SampleRate;
					}
					break;
				case PRESENTATION_GRAPHICS_STREAM:
				case INTERACTIVE_GRAPHICS_STREAM: {
						ReadBuffer((BYTE*)s.m_LanguageCode, 3);
						s.m_LCID = ISO6392ToLcid(s.m_LanguageCode);
					}
					break;
				case SUBTITLE_STREAM: {
						ReadByte(); // Should this really be here?
						ReadBuffer((BYTE*)s.m_LanguageCode, 3);
						s.m_LCID = ISO6392ToLcid(s.m_LanguageCode);
					}
					break;
				default :
					break;
			}

			m_Streams.Add(s);
			SetPos(Pos);
		}
	}

	return S_OK;
}

HRESULT CHdmvClipInfo::ReadCpiInfo(CAtlArray<SyncPoint>* sps)
{
	sps->RemoveAll();

	CAtlArray<ClpiEpMapEntry> ClpiEpMapList;

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
	BYTE* buf = DNew BYTE[size];
	ReadBuffer(buf, size);
	CGolombBuffer gb(buf, size);
	for (int i = 0; i < num_stream_pid; i++) {
		ClpiEpMapEntry em;

		em.pid                      = gb.ReadShort();
		gb.BitRead(10);
		em.ep_stream_type           = gb.BitRead(4);
		em.num_ep_coarse            = gb.ReadShort();
		em.num_ep_fine              = gb.BitRead(18);
		em.ep_map_stream_start_addr = gb.ReadDword() + ep_map_pos;

		em.coarse                   = DNew ClpiEpCoarse[em.num_ep_coarse];
		em.fine                     = DNew ClpiEpFine[em.num_ep_fine];

		ClpiEpMapList.Add(em);
	}
	SAFE_DELETE_ARRAY(buf);

	for (int i = 0; i < num_stream_pid; i++) {
		ClpiEpMapEntry* em = &ClpiEpMapList[i];

		SetPos(em->ep_map_stream_start_addr);
		const DWORD fine_start = ReadDword();

		size = em->num_ep_coarse * 8;
		buf = DNew BYTE[size];
		ReadBuffer(buf, size);
		gb.Reset(buf, size);

		for (int j = 0; j < em->num_ep_coarse; j++) {
			em->coarse[j].ref_ep_fine_id = gb.BitRead(18);
			em->coarse[j].pts_ep         = gb.BitRead(14);
			em->coarse[j].spn_ep         = gb.ReadDword();
		}
		SAFE_DELETE_ARRAY(buf);

		SetPos(em->ep_map_stream_start_addr + fine_start);

		size = em->num_ep_fine * 4;
		buf = DNew BYTE[size];
		ReadBuffer(buf, size);
		gb.Reset(buf, size);

		for (int j = 0; j < em->num_ep_fine; j++) {
			em->fine[j].is_angle_change_point = gb.BitRead(1);
			em->fine[j].i_end_position_offset = gb.BitRead(3);
			em->fine[j].pts_ep                = gb.BitRead(11);
			em->fine[j].spn_ep                = gb.BitRead(17);
		}
		SAFE_DELETE_ARRAY(buf);
	}

	if (!ClpiEpMapList.IsEmpty()) {
		const ClpiEpMapEntry* entry = &ClpiEpMapList[0];
		for (int i = 0; i < entry->num_ep_coarse; i++) {
			int start, end;

			const ClpiEpCoarse* coarse = &entry->coarse[i];
			start = coarse->ref_ep_fine_id;
			if (i < entry->num_ep_coarse - 1) {
				end = entry->coarse[i+1].ref_ep_fine_id;
			} else {
				end = entry->num_ep_fine;
			}

			for (int j = start; j < end; j++) {
				const ClpiEpFine* fine = &entry->fine[j];
				const UINT64 pts = ((UINT64)(coarse->pts_ep & ~0x01) << 18) +
				                   ((UINT64)fine->pts_ep << 8);
				const UINT32 spn = (coarse->spn_ep & ~0x1FFFF) + fine->spn_ep;

				SyncPoint sp = {REFERENCE_TIME(20000.0f * pts / 90), (__int64)spn * 192};
				sps->Add(sp);
			}
		}
	}

	for (size_t i = 0; i < ClpiEpMapList.GetCount(); i++) {
		SAFE_DELETE_ARRAY(ClpiEpMapList[i].coarse);
		SAFE_DELETE_ARRAY(ClpiEpMapList[i].fine);
	}

	ClpiEpMapList.RemoveAll();

	return S_OK;
}

#define dwShareMode          FILE_SHARE_READ | FILE_SHARE_WRITE
#define dwFlagsAndAttributes FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN

#define CheckVer() (!(memcmp(Buff, "0300", 4)) || (!memcmp(Buff, "0200", 4)) || (!memcmp(Buff, "0100", 4)))

HRESULT CHdmvClipInfo::ReadInfo(LPCTSTR strFile, CAtlArray<SyncPoint>* sps)
{
	m_hFile = CreateFile(strFile, GENERIC_READ, dwShareMode, NULL,
						 OPEN_EXISTING, dwFlagsAndAttributes, NULL);
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
			ReadCpiInfo(sps);
		}

		return CloseFile(S_OK);
	}

	return AmHresultFromWin32(GetLastError());
}

CHdmvClipInfo::Stream* CHdmvClipInfo::FindStream(SHORT wPID)
{
	size_t nStreams = m_Streams.GetCount();
	for (size_t i = 0; i < nStreams; i++) {
		if (m_Streams[i].m_PID == wPID) {
			return &m_Streams[i];
		}
	}

	return NULL;
}

HRESULT CHdmvClipInfo::ReadPlaylist(CString strPlaylistFile, REFERENCE_TIME& rtDuration, CPlaylist& Playlist, BOOL bReadMVCExtension/* = FALSE*/, BOOL bFullInfoRead/* = FALSE*/, BYTE* MVC_Base_View_R_flag/* = NULL*/)
{
	CPath Path(strPlaylistFile);
	rtDuration = 0;

	m_Streams.RemoveAll();

	// Get BDMV folder
	Path.RemoveFileSpec();
	Path.RemoveFileSpec();

	m_hFile = CreateFile(strPlaylistFile, GENERIC_READ, dwShareMode, NULL,
						 OPEN_EXISTING, dwFlagsAndAttributes, NULL);
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

		DLog(L"CHdmvClipInfo::ReadPlaylist() : '%s'", strPlaylistFile);

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
		CAtlArray<ext_sub_path> ext_sub_paths;

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
									fileName.Format(L"%s\\STREAM\\%c%c%c%c%c.M2TS", CString(Path), Buff[0], Buff[1], Buff[2], Buff[3], Buff[4]);
									_ext_sub_path.extFileNames.emplace_back(fileName);
								}

								SetPos(savePos);
							}

							ext_sub_paths.Add(_ext_sub_path);

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
		for (size_t i = 0; i < ext_sub_paths.GetCount(); i++) {
			if (ext_sub_paths[i].type == 8 && ext_sub_paths[i].playitem_count == nPlaylistItems) {
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

			CAutoPtr<PlaylistItem> Item(DNew PlaylistItem);
			Item->m_strFileName.Format(format, CString(Path), Buff[0], Buff[1], Buff[2], Buff[3], Buff[4]);
			if (!::PathFileExists(Item->m_strFileName)) {
				DLog(L"    ==> '%s' is missing, skip it", Item->m_strFileName);

				stnssextPos = 0;
				continue;
			}

			ReadBuffer(Buff, 3);
			const BYTE is_multi_angle = (Buff[1] >> 4) & 0x1;

			Item->m_rtIn  = REFERENCE_TIME(20000.0f * ReadDword() / 90);
			Item->m_rtOut = REFERENCE_TIME(20000.0f * ReadDword() / 90);

			Item->m_rtStartTime	= rtDuration;

			rtDuration += (Item->m_rtOut - Item->m_rtIn);

			ReadBuffer(Buff, 8);		// mpls uo
			ReadByte();
			ReadByte();					// still mode
			ReadShort();				// still time
			BYTE angle_count = 1;
			if (is_multi_angle) {
				angle_count = ReadByte();
				if (angle_count < 1) {
					angle_count = 1;
				}
				ReadByte();
			}
			for (BYTE i = 1; i < angle_count; i++) {
				ReadBuffer(Buff, 9);	// M2TS file name
				ReadByte();				// stc_id
			}

			// stn
			ReadShort();					// length
			ReadShort();					// reserved_for_future_use
			Item->m_num_video = ReadByte();	// number of Primary Video Streams
			ReadByte();						// number of Primary Audio Streams
			const BYTE num_pg = ReadByte();	// number of Presentation Graphic Streams
			const BYTE num_ig = ReadByte();	// number of Interactive Graphic Streams

			Item->m_pg_offset_sequence_id.resize(num_pg, 0xFF);
			Item->m_ig_offset_sequence_id.resize(num_ig, 0xFF);

			if (bFullInfoRead) {
				LARGE_INTEGER size = {0, 0};
				HANDLE hFile = CreateFile(Item->m_strFileName, GENERIC_READ, dwShareMode, NULL,
										  OPEN_EXISTING, dwFlagsAndAttributes, NULL);
				if (hFile != INVALID_HANDLE_VALUE) {
					GetFileSizeEx(hFile, &size);
					CloseHandle(hFile);
				}

				Item->m_SizeIn  = TotalSize;
				TotalSize       += size.QuadPart;
				Item->m_SizeOut = TotalSize;
			}

			POSITION pos = Playlist.GetHeadPosition();
			while (pos) {
				PlaylistItem* pItem = Playlist.GetNext(pos);
				if (*pItem == *Item) {
					bDuplicate = true;
					break;
				}
			}

			DLog(L"    ==> '%s', Duration : %s [%15I64d], Total duration : %s, Size : %I64d", Item->m_strFileName, ReftimeToString(Item->Duration()), Item->Duration(), ReftimeToString(rtDuration), Item->Size());

			Playlist.AddTail(Item);
		}

		// stn ss extension
		if (bFullInfoRead && stnssextPos && !Playlist.IsEmpty()) {
			SetPos(stnssextPos);

			int i = 0;
			POSITION pos = Playlist.GetHeadPosition();
			while (pos) {
				PlaylistItem* pItem = Playlist.GetNext(pos);

				SHORT lenght = ReadShort();
				BYTE* buf = DNew BYTE[lenght];
				ReadBuffer(buf, lenght);
				CGolombBuffer gb(buf, lenght);

				int Fixed_offset_during_PopUp_flag = gb.BitRead(1);
				gb.BitRead(15); // reserved

				for (i = 0; i < pItem->m_num_video; i++) {
					// stream_entry
					BYTE slen = gb.ReadByte();
					gb.SkipBytes(slen);

					// stream_attributes_ss
					slen = gb.ReadByte();
					gb.SkipBytes(slen);

					gb.BitRead(10); // reserved
					gb.BitRead(6);  // number_of_offset_sequences
				}

				for (auto it = pItem->m_pg_offset_sequence_id.begin(); it != pItem->m_pg_offset_sequence_id.end(); it++) {
					*it = gb.ReadByte();

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

				for (auto it = pItem->m_ig_offset_sequence_id.begin(); it != pItem->m_ig_offset_sequence_id.end(); it++) {
					if (Fixed_offset_during_PopUp_flag) {
						gb.ReadByte();
					} else {
						*it = gb.ReadByte();
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

				SAFE_DELETE_ARRAY(buf);
			}
		}

		CloseFile(S_OK);

		if (bFullInfoRead && !Playlist.IsEmpty())  {
			POSITION pos = Playlist.GetHeadPosition();
			while (pos) {
				PlaylistItem* pItem = Playlist.GetNext(pos);
				CString fname = pItem->m_strFileName;
				if (fname.Find(L".SSIF") > 0) {
					fname.Replace(L"\\STREAM\\SSIF\\", L"\\CLIPINF\\");
					fname.Replace(L".SSIF", L".CLPI");
				} else {
					fname.Replace(L"\\STREAM\\", L"\\CLIPINF\\");
					fname.Replace(L".M2TS", L".CLPI");
				}

				ReadInfo(fname, &pItem->m_sps);
			}

			if (!ext_sub_paths.IsEmpty()) {
				for (size_t i = 0; i < ext_sub_paths.GetCount(); i++) {
					for (auto& it : ext_sub_paths[i].extFileNames) {
						CString fname = it;
						fname.Replace(L"\\STREAM\\", L"\\CLIPINF\\");
						fname.Replace(L".M2TS", L".CLPI");

						ReadInfo(fname);
					}
				}
			}
		}

		return Playlist.IsEmpty() ? E_FAIL : bDuplicate ? S_FALSE : S_OK;
	}

	return AmHresultFromWin32(GetLastError());
}

HRESULT CHdmvClipInfo::ReadChapters(CString strPlaylistFile, CPlaylist& PlaylistItems, CPlaylistChapter& Chapters)
{
	CPath Path(strPlaylistFile);

	// Get BDMV folder
	Path.RemoveFileSpec();
	Path.RemoveFileSpec();

	m_hFile = CreateFile(strPlaylistFile, GENERIC_READ, dwShareMode, NULL,
						 OPEN_EXISTING, dwFlagsAndAttributes, NULL);
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

		REFERENCE_TIME* rtOffset = DNew REFERENCE_TIME[PlaylistItems.GetCount()];
		REFERENCE_TIME  rtSum    = 0;
		USHORT          nIndex   = 0;

		POSITION pos = PlaylistItems.GetHeadPosition();
		while (pos) {
			const CHdmvClipInfo::PlaylistItem* PI = PlaylistItems.GetNext(pos);

			rtOffset[nIndex] = rtSum - PI->m_rtIn;
			rtSum            = rtSum + PI->Duration();
			nIndex++;
		}

		ReadDword();						// PlayList_start_address
		LONGLONG playlistPos = ReadDword();	// PlayListMark_start_address

		// PlayListMark()
		SetPos(playlistPos);
		ReadDword();							// length
		const USHORT nMarkCount = ReadShort();	// number_of_PlayList_marks
		for (USHORT i = 0; i < nMarkCount; i++) {
			PlaylistChapter	Chapter;

			ReadByte();																// reserved_for_future_use
			Chapter.m_nMarkType   = (PlaylistMarkType)ReadByte();					// mark_type
			Chapter.m_nPlayItemId = ReadShort();									// ref_to_PlayItem_id
			Chapter.m_rtTimestamp = REFERENCE_TIME(20000.0f * ReadDword() / 90) + rtOffset[Chapter.m_nPlayItemId];		// mark_time_stamp
			Chapter.m_nEntryPID   = ReadShort();									// entry_ES_PID
			Chapter.m_rtDuration  = REFERENCE_TIME(20000.0f * ReadDword() / 90);	// duration

			if (Chapter.m_rtTimestamp < 0 || Chapter.m_rtTimestamp > rtSum) {
				continue;
			}

			Chapters.AddTail(Chapter);
		}

		CloseFile(S_OK);
		SAFE_DELETE_ARRAY(rtOffset);

		return S_OK;
	}

	return AmHresultFromWin32(GetLastError());
}

#define MIN_LIMIT 3

HRESULT CHdmvClipInfo::FindMainMovie(LPCTSTR strFolder, CString& strPlaylistFile, CPlaylist& MainPlaylist, CPlaylist& MPLSPlaylists)
{
	HRESULT hr = E_FAIL;
	CString strPath(strFolder);

	MainPlaylist.RemoveAll();
	MPLSPlaylists.RemoveAll();

	WIN32_FIND_DATA fd = {0};
	HANDLE hFind = FindFirstFile(strPath + L"\\PLAYLIST\\*.mpls", &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		CPlaylist      Playlist;
		CPlaylist      PlaylistArray[1024];
		int            idx = 0;
		REFERENCE_TIME rtMax = 0;
		REFERENCE_TIME rtCurrent;
		CString        strCurrentPlaylist;
		do {
			strCurrentPlaylist = strPath + L"\\PLAYLIST\\" + fd.cFileName;
			Playlist.RemoveAll();

			// Main movie shouldn't have duplicate M2TS filename ...
			if (ReadPlaylist(strCurrentPlaylist, rtCurrent, Playlist) == S_OK) {
				if (rtCurrent > rtMax) {
					rtMax			= rtCurrent;
					strPlaylistFile = strCurrentPlaylist;
					MainPlaylist.RemoveAll();
					POSITION pos = Playlist.GetHeadPosition();
					while (pos) {
						CAutoPtr<PlaylistItem> Item(DNew PlaylistItem(*Playlist.GetNext(pos)));
						MainPlaylist.AddTail(Item);
					}
					hr = S_OK;
				}

				if (rtCurrent >= (REFERENCE_TIME)MIN_LIMIT*600000000) {

					// Search duplicate playlists ...
					bool dublicate = false;
					for (int i = 0; i < idx; i++) {
						CPlaylist* pl = &PlaylistArray[i];

						if (pl->GetCount() != Playlist.GetCount()) {
							continue;
						}
						POSITION pos1	= Playlist.GetHeadPosition();
						POSITION pos2	= pl->GetHeadPosition();
						dublicate		= true;
						while (pos1 && pos2) {
							PlaylistItem* pli_1 = Playlist.GetNext(pos1);
							PlaylistItem* pli_2 = pl->GetNext(pos2);
							if (*pli_1 == *pli_2) {
								continue;
							}
							dublicate = false;
							break;
						}

						if (dublicate) {
							break;
						}
					}

					if (dublicate) {
						continue;
					}

					CAutoPtr<PlaylistItem> Item(DNew PlaylistItem);
					Item->m_strFileName = strCurrentPlaylist;
					Item->m_rtIn        = 0;
					Item->m_rtOut       = rtCurrent;
					MPLSPlaylists.AddTail(Item);

					POSITION pos = Playlist.GetHeadPosition();
					while (pos) {
						CAutoPtr<PlaylistItem> Item(DNew PlaylistItem(*Playlist.GetNext(pos)));
						PlaylistArray[idx].AddTail(Item);
					}
					idx++;
				}
			}
		} while (FindNextFile(hFind, &fd));

		FindClose(hFind);
	}

	if (MPLSPlaylists.GetCount() > 1) {
		// bubble sort
		for (size_t j = 0; j < MPLSPlaylists.GetCount(); j++) {
			for (size_t i = 0; i < MPLSPlaylists.GetCount()-1; i++) {
				if (MPLSPlaylists.GetAt(MPLSPlaylists.FindIndex(i))->Duration() < MPLSPlaylists.GetAt(MPLSPlaylists.FindIndex(i+1))->Duration()) {
					MPLSPlaylists.SwapElements(MPLSPlaylists.FindIndex(i), MPLSPlaylists.FindIndex(i+1));
				}
			}
		}
	}

	return hr;
}

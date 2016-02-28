/*
 * (C) 2006-2014 see Authors.txt
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
	: m_hFile(INVALID_HANDLE_VALUE)
	, m_bIsHdmv(false)
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

DWORD CHdmvClipInfo::ReadDword()
{
	return ReadByte()<<24 | ReadByte()<<16 | ReadByte()<<8 | ReadByte();
}

SHORT CHdmvClipInfo::ReadShort()
{
	return ReadByte()<<8 | ReadByte();
}

BYTE CHdmvClipInfo::ReadByte()
{
	BYTE	bVal;
	DWORD	dwRead;
	ReadFile(m_hFile, &bVal, sizeof(bVal), &dwRead, NULL);

	return bVal;
}

void CHdmvClipInfo::ReadBuffer(BYTE* pBuff, DWORD nLen)
{
	DWORD	dwRead;
	ReadFile(m_hFile, pBuff, nLen, &dwRead, NULL);
}

HRESULT CHdmvClipInfo::ReadProgramInfo()
{
	BYTE			number_of_program_sequences;
	BYTE			number_of_streams_in_ps;
	LARGE_INTEGER	Pos = {0, 0};

	m_Streams.RemoveAll();
	Pos.QuadPart = ProgramInfo_start_address;
	SetFilePointerEx(m_hFile, Pos, NULL, FILE_BEGIN);

	ReadDword();	//length
	ReadByte();		//reserved_for_word_align
	number_of_program_sequences	= (BYTE)ReadByte();
	int iStream = 0;
	for (size_t i = 0; i < number_of_program_sequences; i++) {
		ReadDword();	//SPN_program_sequence_start
		ReadShort();	//program_map_PID
		number_of_streams_in_ps	= (BYTE)ReadByte();		//number_of_streams_in_ps
		ReadByte();		//reserved_for_future_use

		for (size_t stream_index = 0; stream_index < number_of_streams_in_ps; stream_index++) {
			m_Streams.SetCount(iStream + 1);
			m_Streams[iStream].m_PID	= ReadShort();	// stream_PID

			// == StreamCodingInfo
			Pos.QuadPart = 0;
			SetFilePointerEx(m_hFile, Pos, &Pos, FILE_CURRENT);
			Pos.QuadPart += ReadByte() + 1;	// length
			m_Streams[iStream].m_Type	= (PES_STREAM_TYPE)ReadByte();

			switch (m_Streams[iStream].m_Type) {
				case VIDEO_STREAM_MPEG1:
				case VIDEO_STREAM_MPEG2:
				case VIDEO_STREAM_H264:
				case VIDEO_STREAM_VC1: {
						BYTE Temp = ReadByte();
						BDVM_VideoFormat VideoFormat		= (BDVM_VideoFormat)(Temp >> 4);
						BDVM_FrameRate FrameRate			= (BDVM_FrameRate)(Temp & 0xf);
						Temp								= ReadByte();
						BDVM_AspectRatio AspectRatio		= (BDVM_AspectRatio)(Temp >> 4);

						m_Streams[iStream].m_VideoFormat	= VideoFormat;
						m_Streams[iStream].m_FrameRate		= FrameRate;
						m_Streams[iStream].m_AspectRatio	= AspectRatio;
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
						BDVM_ChannelLayout ChannelLayout		= (BDVM_ChannelLayout)(Temp >> 4);
						BDVM_SampleRate SampleRate				= (BDVM_SampleRate)(Temp & 0xF);

						ReadBuffer((BYTE*)m_Streams[iStream].m_LanguageCode, 3);
						m_Streams[iStream].m_LanguageCode[3]	= '\0';
						m_Streams[iStream].m_LCID				= ISO6392ToLcid(m_Streams[iStream].m_LanguageCode);
						m_Streams[iStream].m_ChannelLayout		= ChannelLayout;
						m_Streams[iStream].m_SampleRate			= SampleRate;
					}
					break;
				case PRESENTATION_GRAPHICS_STREAM:
				case INTERACTIVE_GRAPHICS_STREAM: {
						ReadBuffer((BYTE*)m_Streams[iStream].m_LanguageCode, 3);
						m_Streams[iStream].m_LanguageCode[3]	= '\0';
						m_Streams[iStream].m_LCID				= ISO6392ToLcid(m_Streams[iStream].m_LanguageCode);
					}
					break;
				case SUBTITLE_STREAM: {
						ReadByte(); // Should this really be here?
						ReadBuffer((BYTE*)m_Streams[iStream].m_LanguageCode, 3);
						m_Streams[iStream].m_LanguageCode[3]	= '\0';
						m_Streams[iStream].m_LCID				= ISO6392ToLcid(m_Streams[iStream].m_LanguageCode);
					}
					break;
				default :
					break;
			}

			iStream++;
			SetFilePointerEx(m_hFile, Pos, NULL, FILE_BEGIN);
		}
	}
	return S_OK;
}

HRESULT CHdmvClipInfo::ReadCpiInfo(CAtlArray<SyncPoint>* sps)
{
	sps->RemoveAll();

	CAtlArray<ClpiEpMapEntry> ClpiEpMapList;

	LARGE_INTEGER Pos = {0, 0};
	Pos.QuadPart = Cpi_start_addrress;
	SetFilePointerEx(m_hFile, Pos, NULL, FILE_BEGIN);

	DWORD len = ReadDword();
	if (len == 0) {
		return E_FAIL;
	}

	BYTE* buf = NULL;

	ReadByte();
	BYTE Type = ReadByte() & 0xF;
	DWORD ep_map_pos = Cpi_start_addrress + 4 + 2;
	ReadByte();
	BYTE num_stream_pid = ReadByte();

	DWORD size = num_stream_pid * 12;
	buf = DNew BYTE[size];
	ReadBuffer(buf, size);
	CGolombBuffer gb(buf, size);
	for (int i = 0; i < num_stream_pid; i++) {
		ClpiEpMapEntry em;

		em.pid						= gb.ReadShort();
		gb.BitRead(10);
		em.ep_stream_type			= gb.BitRead(4);
		em.num_ep_coarse			= gb.ReadShort();
		em.num_ep_fine				= gb.BitRead(18);
		em.ep_map_stream_start_addr	= gb.ReadDword() + ep_map_pos;

		em.coarse					= DNew ClpiEpCoarse[em.num_ep_coarse];
		em.fine						= DNew ClpiEpFine[em.num_ep_fine];

		ClpiEpMapList.Add(em);
	}
	delete[] buf;

	for (int i = 0; i < num_stream_pid; i++) {
		ClpiEpMapEntry* em = &ClpiEpMapList[i];

		Pos.QuadPart = em->ep_map_stream_start_addr;
		SetFilePointerEx(m_hFile, Pos, NULL, FILE_BEGIN);
		DWORD fine_start = ReadDword();

		size = em->num_ep_coarse * 8;
		buf = DNew BYTE[size];
		ReadBuffer(buf, size);
		gb.Reset(buf, size);

		for (int j = 0; j < em->num_ep_coarse; j++) {
			em->coarse[j].ref_ep_fine_id	= gb.BitRead(18);
			em->coarse[j].pts_ep			= gb.BitRead(14);
			em->coarse[j].spn_ep			= gb.ReadDword();
		}
		delete[] buf;

		Pos.QuadPart = em->ep_map_stream_start_addr+fine_start;
		SetFilePointerEx(m_hFile, Pos, NULL, FILE_BEGIN);

		size = em->num_ep_fine * 4;
		buf = DNew BYTE[size];
		ReadBuffer(buf, size);
		gb.Reset(buf, size);

		for (int j = 0; j < em->num_ep_fine; j++) {
			em->fine[j].is_angle_change_point	= gb.BitRead(1);
			em->fine[j].i_end_position_offset	= gb.BitRead(3);
			em->fine[j].pts_ep					= gb.BitRead(11);
			em->fine[j].spn_ep					= gb.BitRead(17);
		}
		delete[] buf;
	}

	if (ClpiEpMapList.GetCount() > 0) {
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
				ClpiEpFine* fine = &entry->fine[j];
				UINT64 pts = ((UINT64)(coarse->pts_ep & ~0x01) << 18) +
							 ((UINT64)fine->pts_ep << 8);
				UINT32 spn = (coarse->spn_ep & ~0x1FFFF) + fine->spn_ep;

				SyncPoint sp = {REFERENCE_TIME(20000.0f*pts/90), (__int64)spn * 192};
				sps->Add(sp);
			}
		}
	}

	for (size_t ii = 0; ii < ClpiEpMapList.GetCount(); ii++) {
		delete[] ClpiEpMapList[ii].coarse;
		delete[] ClpiEpMapList[ii].fine;
	}

	ClpiEpMapList.RemoveAll();

	return S_OK;
}

HRESULT CHdmvClipInfo::ReadInfo(LPCTSTR strFile, CAtlArray<SyncPoint>* sps)
{
	BYTE Buff[4];

	m_bIsHdmv = false;
	m_hFile   = CreateFile(strFile, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
							OPEN_EXISTING, FILE_ATTRIBUTE_READONLY|FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if (m_hFile != INVALID_HANDLE_VALUE) {
		ReadBuffer(Buff, 4);
		if (memcmp(Buff, "HDMV", 4)) {
			return CloseFile(VFW_E_INVALID_FILE_FORMAT);
		}

		ReadBuffer(Buff, 4);
		if ((memcmp(Buff, "0200", 4)) && (memcmp(Buff, "0100", 4))) {
			return CloseFile(VFW_E_INVALID_FILE_FORMAT);
		}

		SequenceInfo_start_address	= ReadDword();
		ProgramInfo_start_address	= ReadDword();
		Cpi_start_addrress			= ReadDword();

		ReadProgramInfo();

		if (sps) {
			ReadCpiInfo(sps);
		}

		m_bIsHdmv = true;

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

LPCTSTR CHdmvClipInfo::Stream::Format()
{
	switch (m_Type) {
		case VIDEO_STREAM_MPEG1:
			return _T("Mpeg1");
		case VIDEO_STREAM_MPEG2:
			return _T("Mpeg2");
		case VIDEO_STREAM_H264:
			return _T("H264");
		case VIDEO_STREAM_VC1:
			return _T("VC-1");
		case AUDIO_STREAM_MPEG1:
			return _T("MPEG1");
		case AUDIO_STREAM_MPEG2:
			return _T("MPEG2");
		case AUDIO_STREAM_LPCM:
			return _T("LPCM");
		case AUDIO_STREAM_AC3:
			return _T("AC3");
		case AUDIO_STREAM_DTS:
			return _T("DTS");
		case AUDIO_STREAM_AC3_TRUE_HD:
			return _T("MLP");
		case AUDIO_STREAM_AC3_PLUS:
			return _T("DD+");
		case AUDIO_STREAM_DTS_HD:
			return _T("DTS-HD");
		case AUDIO_STREAM_DTS_HD_MASTER_AUDIO:
			return _T("DTS-HD XLL");
		case SECONDARY_AUDIO_AC3_PLUS:
			return _T("Sec DD+");
		case SECONDARY_AUDIO_DTS_HD:
			return _T("Sec DTS-HD");
		case PRESENTATION_GRAPHICS_STREAM :
			return _T("PG");
		case INTERACTIVE_GRAPHICS_STREAM :
			return _T("IG");
		case SUBTITLE_STREAM :
			return _T("Text");
		default :
			return _T("Unknown");
	}
}

HRESULT CHdmvClipInfo::ReadPlaylist(CString strPlaylistFile, REFERENCE_TIME& rtDuration, CPlaylist& Playlist, BOOL bFullInfoRead/* = FALSE*/, BYTE* MVC_Base_View_R_flag/* = NULL*/)
{

	BYTE	Buff[20] = { 0 };
	CPath	Path (strPlaylistFile);
	bool	bDuplicate = false;
	rtDuration  = 0;

	// Get BDMV folder
	Path.RemoveFileSpec();
	Path.RemoveFileSpec();

	m_hFile = CreateFile(strPlaylistFile, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
							OPEN_EXISTING, FILE_ATTRIBUTE_READONLY|FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if (m_hFile != INVALID_HANDLE_VALUE) {
		DbgLog((LOG_TRACE, 3, _T("CHdmvClipInfo::ReadPlaylist() : %s"), strPlaylistFile));

		ReadBuffer(Buff, 4);
		if (memcmp(Buff, "MPLS", 4)) {
			return CloseFile(VFW_E_INVALID_FILE_FORMAT);
		}

		ReadBuffer(Buff, 4);
		if ((memcmp(Buff, "0200", 4)) && (memcmp(Buff, "0100", 4))) {
			return CloseFile(VFW_E_INVALID_FILE_FORMAT);
		}

		LARGE_INTEGER	Pos    = {0, 0};
		LARGE_INTEGER	PosExt = {0, 0};
		DWORD			dwTemp = 0;
		USHORT			nPlaylistItems = 0;
		USHORT			nSubPathsItems = 0;

		Pos.QuadPart = ReadDword();		// PlayList_start_address
		ReadDword();					// PlayListMark_start_address
		PosExt.QuadPart = ReadDword();	// Extension_start_address

		if (MVC_Base_View_R_flag) {
			// skip 20 bytes
			ReadBuffer(Buff, 20);

			// App info
			ReadDword();				// lenght
			ReadByte();					// reserved
			ReadByte();					// playback type
			ReadShort();				// playback count
			ReadBuffer(Buff, 8);		// mpls uo
			BYTE flags = ReadByte();	// flags
			*MVC_Base_View_R_flag = (flags >> 4) & 0x1;
		}

		// Extension
		LARGE_INTEGER PosSTNSSExt = {0, 0};
		if (PosExt.QuadPart) {
			SetFilePointerEx(m_hFile, PosExt, NULL, FILE_BEGIN);
			ReadDword();					// lenght
			ReadDword();					// start address
			ReadShort(); ReadByte();		// padding
			BYTE nNumEntries = ReadByte();	// number of entries
			for (BYTE i = 0; i < nNumEntries; i++) {
				UINT id1 = ReadShort();
				UINT id2 = ReadShort();
				LARGE_INTEGER PosExtSub = PosExt;
				PosExtSub.QuadPart += ReadDword();	// Extension_start_address
				ReadDword();						// Extension_length

				if (id1 == 2) {
					if (id2 == 1) {
						PosSTNSSExt = PosExtSub;
					} else if (id2 == 2) {
						LARGE_INTEGER savePosExt = {0, 0};
						SetFilePointerEx(m_hFile, savePosExt, &savePosExt, FILE_CURRENT);

						SetFilePointerEx(m_hFile, PosExtSub, NULL, FILE_BEGIN);
						ReadDword();					// lenght
						nSubPathsItems += ReadShort();	// number_of_SubPaths

						SetFilePointerEx(m_hFile, savePosExt, NULL, FILE_BEGIN);
					}
				}
			}
		}

		// PlayList()
		SetFilePointerEx(m_hFile, Pos, NULL, FILE_BEGIN);
		ReadDword();						// length
		ReadShort();						// reserved_for_future_use
		nPlaylistItems  = ReadShort();		// number_of_PlayItems
		nSubPathsItems += ReadShort();		// number_of_SubPaths

		Pos.QuadPart += 10;
		__int64 TotalSize = 0;
		for (size_t i = 0; i < nPlaylistItems; i++) {
			CAutoPtr<PlaylistItem> Item(DNew PlaylistItem);
			SetFilePointerEx(m_hFile, Pos, NULL, FILE_BEGIN);
			Pos.QuadPart += ReadShort() + 2;
			ReadBuffer(Buff, 9); // M2TS file name
			if (memcmp(&Buff[5], "M2TS", 4)) {
				return CloseFile(VFW_E_INVALID_FILE_FORMAT);
			}

			LPCWSTR format = L"%s\\STREAM\\%c%c%c%c%c.M2TS";
			if (nSubPathsItems >= nPlaylistItems) {
				format = L"%s\\STREAM\\SSIF\\%c%c%c%c%c.SSIF";
			}
			Item->m_strFileName.Format(format, CString(Path), Buff[0], Buff[1], Buff[2], Buff[3], Buff[4]);
			if (!::PathFileExists(Item->m_strFileName)) {
				DbgLog((LOG_TRACE, 3, _T("		==> %s is missing, skip it"), Item->m_strFileName));

				PosSTNSSExt.QuadPart = 0;
				continue;
			}

			ReadBuffer(Buff, 3);
			const BYTE is_multi_angle = (Buff[1] >> 4) & 0x1;

			dwTemp				= ReadDword();
			Item->m_rtIn		= REFERENCE_TIME(20000.0f*dwTemp/90);

			dwTemp				= ReadDword();
			Item->m_rtOut		= REFERENCE_TIME(20000.0f*dwTemp/90);

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
			Item->m_num_pg = ReadByte();	// number of Presentation Graphic Streams
			Item->m_num_ig = ReadByte();	// number of Interactive Graphic Streams

			Item->m_pg_offset_sequence_id.SetCount(Item->m_num_pg);
			for (size_t i = 0; i < Item->m_num_pg; i++) {
				Item->m_pg_offset_sequence_id[i] = 0xff;
			}
			Item->m_ig_offset_sequence_id.SetCount(Item->m_num_ig);
			for (size_t i = 0; i < Item->m_num_ig; i++) {
				Item->m_ig_offset_sequence_id[i] = 0xff;
			}

			if (bFullInfoRead) {
				LARGE_INTEGER size = {0, 0};
				HANDLE hFile = CreateFile(Item->m_strFileName, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
										  OPEN_EXISTING, FILE_ATTRIBUTE_READONLY|FILE_FLAG_SEQUENTIAL_SCAN, NULL);
				if (hFile != INVALID_HANDLE_VALUE) {
					GetFileSizeEx(hFile, &size);
					CloseHandle(hFile);
				}

				Item->m_SizeIn	= TotalSize;
				TotalSize		+= size.QuadPart;
				Item->m_SizeOut	= TotalSize;
			}

			POSITION pos = Playlist.GetHeadPosition();
			while (pos) {
				PlaylistItem* pItem = Playlist.GetNext(pos);
				if (*pItem == *Item) {
					bDuplicate = true;
					break;
				}
			}

			DbgLog((LOG_TRACE, 3, _T("	==> %s, Duration : %s [%15I64d], Total duration : %s, Size : %I64d"), Item->m_strFileName, ReftimeToString(Item->Duration()), Item->Duration(), ReftimeToString(rtDuration), Item->Size()));

			Playlist.AddTail(Item);
		}

		// stn ss extension
		if (bFullInfoRead && PosSTNSSExt.QuadPart && !Playlist.IsEmpty()) {
			SetFilePointerEx(m_hFile, PosSTNSSExt, NULL, FILE_BEGIN);

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

				for (i = 0; i < pItem->m_num_pg; i++) {
					pItem->m_pg_offset_sequence_id[i] = gb.ReadByte();

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

				for (i = 0; i < pItem->m_num_ig; i++) {
					if (Fixed_offset_during_PopUp_flag) {
						gb.ReadByte();
					} else {
						pItem->m_pg_offset_sequence_id[i] = gb.ReadByte();
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

				delete [] buf;
			}
		}

		CloseFile(S_OK);

		if (bFullInfoRead) {
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
		}

		return Playlist.IsEmpty() ? E_FAIL : bDuplicate ? S_FALSE : S_OK;
	}

	return AmHresultFromWin32(GetLastError());
}

HRESULT CHdmvClipInfo::ReadChapters(CString strPlaylistFile, CPlaylist& PlaylistItems, CPlaylistChapter& Chapters)
{
	BYTE	Buff[4];
	CPath	Path(strPlaylistFile);

	// Get BDMV folder
	Path.RemoveFileSpec();
	Path.RemoveFileSpec();

	m_hFile = CreateFile(strPlaylistFile, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
							OPEN_EXISTING, FILE_ATTRIBUTE_READONLY|FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if (m_hFile != INVALID_HANDLE_VALUE) {
		REFERENCE_TIME*		rtOffset = DNew REFERENCE_TIME[PlaylistItems.GetCount()];
		REFERENCE_TIME		rtSum	 = 0;
		USHORT				nIndex   = 0;

		POSITION pos = PlaylistItems.GetHeadPosition();
		while (pos) {
			CHdmvClipInfo::PlaylistItem* PI = PlaylistItems.GetNext(pos);

			rtOffset[nIndex]	= rtSum - PI->m_rtIn;
			rtSum				= rtSum + PI->Duration();
			nIndex++;
		}

		ReadBuffer(Buff, 4);
		if (memcmp(Buff, "MPLS", 4)) {
			SAFE_DELETE_ARRAY(rtOffset);
			return CloseFile(VFW_E_INVALID_FILE_FORMAT);
		}

		ReadBuffer(Buff, 4);
		if ((memcmp(Buff, "0200", 4)!=0) && (memcmp(Buff, "0100", 4)!=0)) {
			SAFE_DELETE_ARRAY(rtOffset);
			return CloseFile(VFW_E_INVALID_FILE_FORMAT);
		}

		LARGE_INTEGER	Pos = {0, 0};
		USHORT			nMarkCount;

		ReadDword();				// PlayList_start_address
		Pos.QuadPart = ReadDword();	// PlayListMark_start_address

		// PlayListMark()
		SetFilePointerEx(m_hFile, Pos, NULL, FILE_BEGIN);
		ReadDword();				// length
		nMarkCount = ReadShort();	// number_of_PlayList_marks
		for (size_t i = 0; i < nMarkCount; i++) {
			PlaylistChapter	Chapter;

			ReadByte();															// reserved_for_future_use
			Chapter.m_nMarkType		= (PlaylistMarkType)ReadByte();				// mark_type
			Chapter.m_nPlayItemId	= ReadShort();								// ref_to_PlayItem_id
			Chapter.m_rtTimestamp	= REFERENCE_TIME(20000.0f*ReadDword()/90) + rtOffset[Chapter.m_nPlayItemId];		// mark_time_stamp
			Chapter.m_nEntryPID		= ReadShort();								// entry_ES_PID
			Chapter.m_rtDuration	= REFERENCE_TIME(20000.0f*ReadDword()/90);	// duration

			if (Chapter.m_rtTimestamp < 0 || Chapter.m_rtTimestamp > rtSum) {
				continue;
			}

			Chapters.AddTail (Chapter);
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

	CPlaylist			Playlist;
	CPlaylist			PlaylistArray[1024];
	int					idx = 0;
	WIN32_FIND_DATA		fd = {0};
	HANDLE hFind = FindFirstFile(strPath + L"\\PLAYLIST\\*.mpls", &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		REFERENCE_TIME	rtMax = 0;
		REFERENCE_TIME	rtCurrent;
		CString			strCurrentPlaylist;
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
					Item->m_strFileName	= strCurrentPlaylist;
					Item->m_rtIn		= 0;
					Item->m_rtOut		= rtCurrent;
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

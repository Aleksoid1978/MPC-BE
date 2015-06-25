/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2015 see Authors.txt
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
#include <io.h>
#include "VobFile.h"
#include "CSSauth.h"
#include "CSSscramble.h"
#include "udf.h"

#include "../../../DSUtil/GolombBuffer.h"

#define AUDIO_BLOCK_SIZE		66
#define VIDEO_BLOCK_SIZE		2
#define SUBTITLE_BLOCK_SIZE		194
#define IFO_HEADER_SIZE			12
#define VIDEO_TS_HEADER			"DVDVIDEO-VMG"
#define VTS_HEADER				"DVDVIDEO-VTS"
#define ATS_HEADER				"DVDAUDIO-ATS"

#define PCM_STREAM_ID 0xa0
#define MLP_STREAM_ID 0xa1

//
// CDVDSession
//

CDVDSession::CDVDSession()
	: m_session(DVD_END_ALL_SESSIONS)
	, m_hDrive(INVALID_HANDLE_VALUE)
{
	memset(&m_SessionKey, 0, sizeof(m_SessionKey));
	memset(&m_DiscKey, 0, sizeof(m_DiscKey));
	memset(&m_TitleKey, 0, sizeof(m_TitleKey));
}

CDVDSession::~CDVDSession()
{
	EndSession();
}

bool CDVDSession::Open(LPCTSTR path)
{
	Close();

	CString fn = path;
	CString drive = _T("\\\\.\\") + fn.Left(fn.Find(':')+1);

	m_hDrive = CreateFile(drive, GENERIC_READ, FILE_SHARE_READ, NULL,
						  OPEN_EXISTING, FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN, (HANDLE)NULL);
	if (m_hDrive == INVALID_HANDLE_VALUE) {
		return false;
	}

	return true;
}

void CDVDSession::Close()
{
	if (m_hDrive != INVALID_HANDLE_VALUE) {
		CloseHandle(m_hDrive);
		m_hDrive = INVALID_HANDLE_VALUE;
	}
}

bool CDVDSession::BeginSession()
{
	EndSession();

	if (m_hDrive == INVALID_HANDLE_VALUE) {
		return false;
	}

	DWORD BytesReturned;
	if (!DeviceIoControl(m_hDrive, IOCTL_DVD_START_SESSION, NULL, 0, &m_session, sizeof(m_session), &BytesReturned, NULL)) {
		m_session = DVD_END_ALL_SESSIONS;
		if (!DeviceIoControl(m_hDrive, IOCTL_DVD_END_SESSION, &m_session, sizeof(m_session), NULL, 0, &BytesReturned, NULL)
				|| !DeviceIoControl(m_hDrive, IOCTL_DVD_START_SESSION, NULL, 0, &m_session, sizeof(m_session), &BytesReturned, NULL)) {
			Close();
			DWORD err = GetLastError();
			UNREFERENCED_PARAMETER(err);
			return false;
		}
	}

	return true;
}

void CDVDSession::EndSession()
{
	if (m_session != DVD_END_ALL_SESSIONS) {
		DWORD BytesReturned;
		DeviceIoControl(m_hDrive, IOCTL_DVD_END_SESSION, &m_session, sizeof(m_session), NULL, 0, &BytesReturned, NULL);
		m_session = DVD_END_ALL_SESSIONS;
	}
}

bool CDVDSession::Authenticate()
{
	if (m_session == DVD_END_ALL_SESSIONS) {
		return false;
	}

	BYTE Challenge[10], Key[10];

	for(int i = 0; i < 10; i++) {
		Challenge[i] = i;
	}

	if (!SendKey(DvdChallengeKey, Challenge)) {
		return false;
	}

	if (!ReadKey(DvdBusKey1, Key)) {
		return false;
	}

	int varient = -1;

	for(int i = 31; i >= 0; i--) {
		BYTE KeyCheck[5];
		CSSkey1(i, Challenge, KeyCheck);
		if (!memcmp(KeyCheck, Key, 5)) {
			varient = i;
		}
	}

	if (!ReadKey(DvdChallengeKey, Challenge)) {
		return false;
	}

	CSSkey2(varient, Challenge, &Key[5]);

	if (!SendKey(DvdBusKey2, &Key[5])) {
		return false;
	}

	CSSbuskey(varient, Key, m_SessionKey);

	return true;
}

bool CDVDSession::GetDiscKey()
{
	if (m_session == DVD_END_ALL_SESSIONS) {
		return false;
	}

	BYTE DiscKeys[2048];
	if (!ReadKey(DvdDiskKey, DiscKeys)) {
		return false;
	}

	for(int i = 0; i < g_nPlayerKeys; i++) {
		for(int j = 1; j < 409; j++) {
			BYTE DiscKey[6];
			memcpy(DiscKey, &DiscKeys[j*5], 5);
			DiscKey[5] = 0;

			CSSdisckey(DiscKey, g_PlayerKeys[i]);

			BYTE Hash[6];
			memcpy(Hash, &DiscKeys[0], 5);
			Hash[5] = 0;

			CSSdisckey(Hash, DiscKey);

			if (!memcmp(Hash, DiscKey, 6)) {
				memcpy(m_DiscKey, DiscKey, 6);
				return true;
			}
		}
	}

	return false;
}

bool CDVDSession::GetTitleKey(int lba, BYTE* pKey)
{
	if (m_session == DVD_END_ALL_SESSIONS) {
		return false;
	}

	if (!ReadKey(DvdTitleKey, pKey, lba)) {
		return false;
	}

	if (!(pKey[0]|pKey[1]|pKey[2]|pKey[3]|pKey[4])) {
		return false;
	}

	pKey[5] = 0;

	CSStitlekey(pKey, m_DiscKey);

	return true;
}

static void Reverse(BYTE* d, BYTE* s, int len)
{
	if (d == s) {
		for(s += len-1; d < s; d++, s--) {
			*d ^= *s, *s ^= *d, *d ^= *s;
		}
	} else {
		for(int i = 0; i < len; i++) {
			d[i] = s[len-1 - i];
		}
	}
}

bool CDVDSession::SendKey(DVD_KEY_TYPE KeyType, BYTE* pKeyData)
{
	CAutoPtr<DVD_COPY_PROTECT_KEY> key;

	switch(KeyType) {
		case DvdChallengeKey:
			key.Attach((DVD_COPY_PROTECT_KEY*)DNew BYTE[DVD_CHALLENGE_KEY_LENGTH]);
			key->KeyLength = DVD_CHALLENGE_KEY_LENGTH;
			Reverse(key->KeyData, pKeyData, 10);
			break;
		case DvdBusKey2:
			key.Attach((DVD_COPY_PROTECT_KEY*)DNew BYTE[DVD_BUS_KEY_LENGTH]);
			key->KeyLength = DVD_BUS_KEY_LENGTH;
			Reverse(key->KeyData, pKeyData, 5);
			break;
		default:
			break;
	}

	if (!key) {
		return false;
	}

	key->SessionId = m_session;
	key->KeyType = KeyType;
	key->KeyFlags = 0;

	DWORD BytesReturned;
	return(!!DeviceIoControl(m_hDrive, IOCTL_DVD_SEND_KEY, key, key->KeyLength, NULL, 0, &BytesReturned, NULL));
}

bool CDVDSession::ReadKey(DVD_KEY_TYPE KeyType, BYTE* pKeyData, int lba)
{
	CAutoPtr<DVD_COPY_PROTECT_KEY> key;

	switch(KeyType) {
		case DvdChallengeKey:
			key.Attach((DVD_COPY_PROTECT_KEY*)DNew BYTE[DVD_CHALLENGE_KEY_LENGTH]);
			key->KeyLength = DVD_CHALLENGE_KEY_LENGTH;
			key->Parameters.TitleOffset.QuadPart = 0;
			break;
		case DvdBusKey1:
			key.Attach((DVD_COPY_PROTECT_KEY*)DNew BYTE[DVD_BUS_KEY_LENGTH]);
			key->KeyLength = DVD_BUS_KEY_LENGTH;
			key->Parameters.TitleOffset.QuadPart = 0;
			break;
		case DvdDiskKey:
			key.Attach((DVD_COPY_PROTECT_KEY*)DNew BYTE[DVD_DISK_KEY_LENGTH]);
			key->KeyLength = DVD_DISK_KEY_LENGTH;
			key->Parameters.TitleOffset.QuadPart = 0;
			break;
		case DvdTitleKey:
			key.Attach((DVD_COPY_PROTECT_KEY*)DNew BYTE[DVD_TITLE_KEY_LENGTH]);
			key->KeyLength = DVD_TITLE_KEY_LENGTH;
			key->Parameters.TitleOffset.QuadPart = 2048i64*lba;
			break;
		default:
			break;
	}

	if (!key) {
		return false;
	}

	key->SessionId = m_session;
	key->KeyType = KeyType;
	key->KeyFlags = 0;

	DWORD BytesReturned;
	if (!DeviceIoControl(m_hDrive, IOCTL_DVD_READ_KEY, key, key->KeyLength, key, key->KeyLength, &BytesReturned, NULL)) {
		DWORD err = GetLastError();
		UNREFERENCED_PARAMETER(err);
		return false;
	}

	switch(KeyType) {
		case DvdChallengeKey:
			Reverse(pKeyData, key->KeyData, 10);
			break;
		case DvdBusKey1:
			Reverse(pKeyData, key->KeyData, 5);
			break;
		case DvdDiskKey:
			memcpy(pKeyData, key->KeyData, 2048);
			for(int i = 0; i < 2048/5; i++) {
				pKeyData[i] ^= m_SessionKey[4-(i%5)];
			}
			break;
		case DvdTitleKey:
			memcpy(pKeyData, key->KeyData, 5);
			for(int i = 0; i < 5; i++) {
				pKeyData[i] ^= m_SessionKey[4-(i%5)];
			}
			break;
		default:
			break;
	}

	return true;
}

//
// CLBAFile
//

CLBAFile::CLBAFile()
{
}

CLBAFile::~CLBAFile()
{
}

bool CLBAFile::IsOpen() const
{
	return(m_hFile != hFileNull);
}

bool CLBAFile::Open(LPCTSTR path)
{
	Close();

	return !!CFile::Open(path, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone | CFile::osSequentialScan);
}

void CLBAFile::Close()
{
	if (m_hFile != hFileNull) {
		CFile::Close();
	}
}

int CLBAFile::GetLengthLBA() const
{
	return (int)(CFile::GetLength() / 2048);
}

int CLBAFile::GetPositionLBA() const
{
	return (int)(CFile::GetPosition() / 2048);
}

int CLBAFile::Seek(int lba)
{
	return (int)(CFile::Seek(2048i64 * lba, CFile::begin) / 2048);
}

bool CLBAFile::Read(BYTE* buff)
{
	return CFile::Read(buff, 2048) == 2048;
}

//
// CVobFile
//

CVobFile::CVobFile()
{
	Close();
	m_rtDuration	= 0;

	memset(&m_Aspect, 0, sizeof(m_Aspect));
}

CVobFile::~CVobFile()
{
	Close();
}

bool CVobFile::IsDVD() const
{
	return m_fDVD;
}

bool CVobFile::HasDiscKey(BYTE* key) const
{
	if (key) {
		memcpy(key, m_DiscKey, 5);
	}
	return m_fHasDiscKey;
}

bool CVobFile::HasTitleKey(BYTE* key) const
{
	if (key) {
		memcpy(key, m_TitleKey, 5);
	}
	return m_fHasTitleKey;
}

BYTE CVobFile::ReadByte()
{
	BYTE value;
	m_ifoFile.Read(&value, sizeof(value));
	return value;
}

WORD CVobFile::ReadWord()
{
	WORD value;
	m_ifoFile.Read(&value, sizeof(value));
	return _byteswap_ushort(value);
}

DWORD CVobFile::ReadDword()
{
	DWORD value;
	m_ifoFile.Read(&value, sizeof(value));
	return _byteswap_ulong(value);
}

bool CVobFile::GetTitleInfo(LPCTSTR fn, ULONG nTitleNum, ULONG& VTSN, ULONG& TTN)
{
	CFile ifoFile;
	if (!ifoFile.Open(fn, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone)) {
		return false;
	}

	char hdr[IFO_HEADER_SIZE + 1] = { 0 };
	ifoFile.Read(hdr, IFO_HEADER_SIZE);
	if (strcmp(hdr, VIDEO_TS_HEADER)) {
		return false;
	}

	ifoFile.Seek(0xC4, CFile::begin);
	DWORD TT_SRPTPosition; // Read a 32-bit unsigned big-endian integer
	ifoFile.Read(&TT_SRPTPosition, sizeof(TT_SRPTPosition));
	TT_SRPTPosition = _byteswap_ulong(TT_SRPTPosition);
	TT_SRPTPosition *= 2048;
	ifoFile.Seek(TT_SRPTPosition + 8 + (nTitleNum - 1) * 12 + 6, CFile::begin);
	BYTE tmp;
	ifoFile.Read(&tmp, sizeof(tmp));
	VTSN = tmp;
	ifoFile.Read(&tmp, sizeof(tmp));
	TTN = tmp;

	ifoFile.Close();

	return true;
}

static short GetFrames( BYTE val)
{
	int byte0_high = val >> 4;
	int byte0_low = val & 0x0F;
	if (byte0_high > 11) {
		return (short)(((byte0_high - 12) * 10) + byte0_low);
	}
	if ((byte0_high <= 3) || (byte0_high >= 8)) {
		return 0;
	}
	return (short)(((byte0_high - 4) * 10) + byte0_low);
}

int BCD2Dec(int BCD)
{
	return (BCD / 0x10) * 10 + (BCD % 0x10);
}

static REFERENCE_TIME FormatTime(BYTE *bytes)
{
	short frames	= GetFrames(bytes[3]);
	int fpsMask		= bytes[3] >> 6;
	double fps		= fpsMask == 0x01 ? 25 : fpsMask == 0x03 ? (30 / 1.001): 0;
	int hours		= BCD2Dec(bytes[0]);
	int minutes		= BCD2Dec(bytes[1]);
	int seconds		= BCD2Dec(bytes[2]);
	int mmseconds = 0;
	if (fps != 0){
		mmseconds = (int)(1000 * frames / fps);
	}

	return REFERENCE_TIME(10000i64 * (((hours*60 + minutes)*60 + seconds)*1000 + mmseconds));
}

static const fraction_t IFO_Aspect[4] = {
	{4,  3},
	{0,  0},
	{0,  0},
	{16, 9}
};

struct stream_t {
	BYTE format;
	BYTE snum;
	char lang[3];
};

struct cell_info_t {
	REFERENCE_TIME duration;
	UINT32 first_sector;
	UINT32 last_sector;
	int VOB_id;
	int Cell_id;
};

struct pgc_t {
	REFERENCE_TIME duration;
	stream_t audios[8];
	stream_t subtitles[32];
	CAtlArray<cell_info_t> chapters;

	pgc_t() {
		duration = 0;
		memset(audios, 0, sizeof(audios));
		memset(subtitles, 0, sizeof(subtitles));
	}
};

bool CVobFile::OpenIFO(CString fn, CAtlList<CString>& vobs, ULONG nProgNum /*= 0*/)
{
	if (fn.Right(6).MakeUpper() != L"_0.IFO") {
		return false;
	}

	if (!m_ifoFile.Open(fn, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone)) {
		return false;
	}

	char hdr[IFO_HEADER_SIZE + 1] = { 0 };
	m_ifoFile.Read(hdr, IFO_HEADER_SIZE);

	bool isAob = false;

	if (strcmp(hdr, VTS_HEADER) == 0) {
		// http://dvdnav.mplayerhq.hu/dvdinfo/index.html
		// http://dvd.sourceforge.net/dvdinfo/index.html

		BYTE buf[8];

		CAtlArray<pgc_t> programs;
		programs.Add();

		// Video Attributes (0x0200)
		m_ifoFile.Seek(0x200, CFile::begin);
		m_ifoFile.Read(buf, 2);
		BYTE aspect = (buf[0] & 0xC) >> 2; // Aspect ratio
		m_Aspect = IFO_Aspect[aspect];

		int nb_audstreams = ReadWord();
		if (nb_audstreams > 8) { nb_audstreams = 8; }
		// Audio Attributes (0x0204)
		for (int i = 0; i < nb_audstreams; i++) {
			m_ifoFile.Read(buf, 8);

			BYTE Coding_mode = buf[0] >> 5;
			switch (Coding_mode) {
				case 0: programs[0].audios[i].format = 0x80; break;
				case 4: programs[0].audios[i].format = 0xA0; break;
				case 6: programs[0].audios[i].format = 0x88; break;
			}

			if (programs[0].audios[i].format) {
				programs[0].audios[i].lang[0] = buf[2];
				programs[0].audios[i].lang[1] = buf[3];
			}
		}

		m_ifoFile.Seek(0x254, CFile::begin);
		int nb_substreams = ReadWord();
		if (nb_substreams > 32) { nb_substreams = 32; }
		// Subpicture Attributes (0x0256)
		for (int i = 0; i < nb_substreams; i++) {
			m_ifoFile.Read(buf, 6);

			programs[0].subtitles[i].format = 0x20;
			programs[0].subtitles[i].lang[0] = buf[2];
			programs[0].subtitles[i].lang[1] = buf[3];
		}

		// VTS_C_ADT (0x00E0)
		m_ifoFile.Seek(0xE0, CFile::begin);
		DWORD posCADT = ReadDword() * 2048;
		m_ifoFile.Seek(posCADT, CFile::begin);
		int nb_cells = ReadWord(); // sometimes it is less than the actual value.
		ReadWord();
		nb_cells = (ReadDword() - 7) / 12; // actual number of cells
		m_ifoFile.Seek(posCADT + 8, CFile::begin);
		for (int i = 0; i < nb_cells; i++) {
			programs[0].chapters.Add();

			programs[0].chapters[i].VOB_id = ReadWord();
			programs[0].chapters[i].Cell_id = ReadByte();
			ReadByte();
			programs[0].chapters[i].first_sector = ReadDword();
			programs[0].chapters[i].last_sector = ReadDword();
		}

		// Chapters & duration ...
		m_ifoFile.Seek(0xCC, CFile::begin); //Get VTS_PGCI adress
		DWORD posPGCI = ReadDword() * 2048;
		m_ifoFile.Seek(posPGCI, CFile::begin);
		WORD nb_PGC = ReadWord();

		for (int prog = 1; prog <= nb_PGC; prog++) {
			m_ifoFile.Seek(posPGCI + 4 + 8 * prog, CFile::begin);
			DWORD offsetPGC = ReadDword();
			m_ifoFile.Seek(posPGCI + offsetPGC, CFile::begin);

			programs.Add();

			// PGC header
			m_ifoFile.Seek(2, CFile::current);
			int nb_programs = ReadByte();
			int nb_pgccells = ReadByte();
			m_ifoFile.Read(buf, 4);
			REFERENCE_TIME playback_time = FormatTime(buf);

			// PGC_AST_CTL
			m_ifoFile.Seek(posPGCI + offsetPGC + 0x0C, CFile::begin);
			for (int i = 0; i < nb_audstreams; i++) {
				m_ifoFile.Read(buf, 2);
				BYTE avail = buf[0] >> 7;	// stream available
				BYTE snum = buf[0] & 0x07;	// Stream number (MPEG audio) or Substream number (all others)

				if (avail && programs[0].audios[i].format) {
					programs[0].audios[i].snum = snum;
					memcpy(&programs[prog].audios[i], &programs[0].audios[i], sizeof(stream_t));
				}
			}

			// PGC_SPST_CTL
			m_ifoFile.Seek(posPGCI + offsetPGC + 0x1C, CFile::begin);
			for (int i = 0; i < nb_substreams; i++) {
				m_ifoFile.Read(buf, 4);
				BYTE avail = buf[0] >> 7;	// stream available
				BYTE snum = buf[1] & 0x1f;	// Stream number for wide (hmm?)

				if (avail && programs[0].subtitles[i].format) {
					programs[0].subtitles[i].snum = snum;
					memcpy(&programs[prog].subtitles[i], &programs[0].subtitles[i], sizeof(stream_t));
				}
			}

			m_ifoFile.Seek(posPGCI + offsetPGC + 0xE6, CFile::begin);
			int offsetProgramMap = ReadWord();
			int offsetCellPlaybackInfoTable = ReadWord();
			int offsetCellPositionInfoTable = ReadWord();

			for (int chap = 0; chap < nb_programs; chap++) {
				m_ifoFile.Seek(posPGCI + offsetPGC + offsetProgramMap + chap, CFile::begin);
				BYTE entryCell = ReadByte();
				BYTE exitCell = nb_pgccells;
				if (chap < (nb_programs - 1)) {
					m_ifoFile.Seek(posPGCI + offsetPGC + offsetProgramMap + (chap + 1), CFile::begin);
					exitCell = ReadByte() - 1;
				}

				cell_info_t chapter;
				chapter.first_sector = UINT32_MAX;
				chapter.last_sector = 0;
				chapter.duration = 0;

				for (int cell = entryCell; cell <= exitCell; cell++) {
					REFERENCE_TIME duration = 0;

					// cell playback time
					m_ifoFile.Seek(posPGCI + offsetPGC + offsetCellPlaybackInfoTable + (cell - 1)*0x18, CFile::begin);
					m_ifoFile.Read(buf, 4);
					int cellType = buf[0] >> 6;
					if (cellType == 0x00 || cellType == 0x01) {
						m_ifoFile.Read(buf, 4);
						duration = FormatTime(buf);
					}
					
					DWORD firstVOBUStartSector = ReadDword(); // first VOBU start sector
					m_ifoFile.Seek(8, CFile::current);
					DWORD lastVOBUEndSector = ReadDword(); // last VOBU end sector

					m_ifoFile.Seek(posPGCI + offsetPGC + offsetCellPositionInfoTable + (cell - 1)*4, CFile::begin);
					chapter.VOB_id = ReadWord();
					ReadByte();
					chapter.Cell_id = ReadByte();

					DbgLog((LOG_TRACE, 3, L"CVobFile::OpenIFO() - PGS : %2d, Pg : %2d, Cell : %2d, VOB id : %2d, Cell id : %2d, first sector : %10lu, last sector : %10lu, duration = %s",
							prog, chap, cell, chapter.VOB_id, chapter.Cell_id, firstVOBUStartSector, lastVOBUEndSector, ReftimeToString(duration)));

					for (size_t i = 0; i < programs[0].chapters.GetCount(); i++) {
						if (chapter.VOB_id == programs[0].chapters[i].VOB_id && chapter.Cell_id == programs[0].chapters[i].Cell_id) {
							if (programs[0].chapters[i].duration == 0) {
								programs[0].chapters[i].duration = duration;
							}
							ASSERT(programs[0].chapters[i].first_sector == firstVOBUStartSector);
							ASSERT(programs[0].chapters[i].last_sector == lastVOBUEndSector);
							break;
						}
					}

					chapter.duration += duration;
					if (chapter.first_sector > firstVOBUStartSector) {
						chapter.first_sector = firstVOBUStartSector;
					}
					if (chapter.last_sector < lastVOBUEndSector) {
						chapter.last_sector = lastVOBUEndSector;
					}
				}

				CAtlArray<cell_info_t>& chapters = programs[prog].chapters;
				if (chapters.GetCount() && chapters[chapters.GetCount() - 1].last_sector + 1 != chapter.first_sector) {
					continue; // ignore jumps for program
				}

				programs[prog].chapters.Add(chapter);
				programs[prog].duration += chapter.duration;
			}
		}

		for (size_t i = 0; i < programs[0].chapters.GetCount(); i++) {
			if (programs[0].chapters[i].duration == 0) {
				programs[0].chapters[i].duration = 10000000i64;
			}
			programs[0].duration += programs[0].chapters[i].duration;
		}

		int selected_prog = 0;
		if (nProgNum < programs.GetCount()) {
			selected_prog = nProgNum;
		}

		for (int i = 0; i < nb_audstreams; i++) {
			stream_t& audio = programs[selected_prog].audios[i];
			if (audio.format) {
				m_pStream_Lang[audio.format + audio.snum] = ISO6391ToLanguage(audio.lang);
			}
		}
		for (int i = 0; i < nb_substreams; i++) {
			stream_t& subtitle = programs[selected_prog].subtitles[i];
			if (subtitle.format) {
				m_pStream_Lang[subtitle.format + subtitle.snum] = ISO6391ToLanguage(subtitle.lang);
			}
		}

		m_rtDuration = programs[selected_prog].duration;

		CAtlArray<cell_info_t>& chapters = programs[selected_prog].chapters;
		REFERENCE_TIME time = 0;
		for (size_t i = 0; i < chapters.GetCount(); i++) {
			chapter_t chapter;
			chapter.first_sector = chapters[i].first_sector;
			chapter.last_sector = chapters[i].last_sector;
			chapter.title = chapter.track = 0;
			chapter.rtime = time;

			m_pChapters.Add(chapter);
			time += chapters[i].duration;
		}
		ASSERT(time == m_rtDuration);
	}
	else if (m_ifoFile.GetLength() >= 4096 && strcmp(hdr, ATS_HEADER) == 0) {

		m_ifoFile.Seek(256, CFile::begin);
		struct audio_fmt_t {
			UINT16 type;
			UINT32 format;
		} audio_formats[8];

		for (int i = 0; i < 8; i++) {
			audio_formats[i].type = ReadWord();
			audio_formats[i].format = ReadDword();
			m_ifoFile.Seek(10, CFile::current);
		}

		BYTE buffer[2048];
		m_ifoFile.Seek(2048, CFile::begin);
		m_ifoFile.Read(buffer, 2048);
		CGolombBuffer gb(buffer, 2048);

		int nb_titles = gb.BitRead(16);
		gb.SkipBytes(2);
		int last_byte = gb.BitRead(32);

		struct track_info_t {
			UINT32 first_PTS;
			UINT32 length_PTS;
			UINT32 first_sector;
			UINT32 last_sector;
		};

		struct title_info_t {
			UINT32 info_offset;
			int nb_tracks;
			UINT32 length_PTS;
			CAtlArray<track_info_t> tracks_info;
		};

		CAtlArray<title_info_t> titles_info;
		titles_info.SetCount(nb_titles);
		for (int i = 0; i < nb_titles; i++) {
			int title_idx = gb.BitRead(8) - 0x81;
			//ASSERT(i == title_idx);
			gb.SkipBytes(3);
			titles_info[i].info_offset = gb.BitRead(32);
		}

		int nb_chapters = 0;
		for (int i = 0; i < nb_titles; i++) {
			title_info_t& title = titles_info[i];

			gb.Seek(title.info_offset);
			gb.SkipBytes(2);
			title.nb_tracks = gb.BitRead(8);
			title.tracks_info.SetCount(title.nb_tracks);
			gb.SkipBytes(1);
			title.length_PTS = gb.BitRead(32);
			TRACE("ATS_XX_0.IFO paser: Title #%d, %u seconds (%u PTS)\n", i, title.length_PTS / 90000, title.length_PTS);
			gb.SkipBytes(8);

			for (int j = 0; j < title.nb_tracks; j++) {
				track_info_t& track = title.tracks_info[j];

				gb.SkipBytes(4);
				int track_num = gb.BitRead(8);
				gb.SkipBytes(1);
				track.first_PTS = gb.BitRead(32);
				track.length_PTS = gb.BitRead(32);
				gb.SkipBytes(6);
				TRACE("ATS_XX_0.IFO paser: Title #%d, track %d, %u seconds (%u PTS), start from %u PTS\n", i, j, track.length_PTS / 90000, track.length_PTS, track.first_PTS);
				nb_chapters++;
			}

			for (int j = 0; j < title.nb_tracks; j++) {
				track_info_t& track = title.tracks_info[j];

				gb.SkipBytes(4);
				track.first_sector = gb.BitRead(32);
				track.last_sector = gb.BitRead(32);
				TRACE("ATS_XX_0.IFO paser: Title #%d, track %d, first sector %u, last_sector %u\n", i, j, track.first_sector, track.last_sector);
			}
		}

		m_pChapters.SetCount(nb_chapters);
		int chapter_idx = 0;
		__int64 pts = 0;

		for (int i = 0; i < nb_titles; i++) {
			title_info_t& title = titles_info[i];
			m_pChapters[chapter_idx].title = i;

			for (int j = 0; j < title.nb_tracks; j++) {
				track_info_t& track = title.tracks_info[j];
				m_pChapters[chapter_idx].track = j;

				m_pChapters[chapter_idx].rtime = (__int64)pts * 1000 / 9;
				m_pChapters[chapter_idx].first_sector = track.first_sector;
				m_pChapters[chapter_idx].last_sector = track.last_sector;

				pts += track.length_PTS;
				chapter_idx++;
			}
		}

		m_rtDuration = (__int64)pts * 1000 / 9;
		m_pStream_Lang[0] = _T("undefined");
		isAob = true;
	}
	else {
		return false;
	}

	if (m_pChapters.IsEmpty()) {
		return false;
	}

	m_ifoFile.Close();

	vobs.RemoveAll();

	fn.Truncate(fn.GetLength() - 5);
	for(int i = 1; i < 9; i++) { // skip VTS_xx_0.VOB
		CString vob;
		if (isAob) {
			vob.Format(_T("%s%d.aob"), fn, i);
		} else {
			vob.Format(_T("%s%d.vob"), fn, i);
		}

		CFileStatus status;
		if (!CFile::GetStatus(vob, status)) {
			break;
		}

		if (status.m_size & 0x7ff) {
			vobs.RemoveAll();
			break;
		}

		if (status.m_size > 0) {
			vobs.AddTail(vob);
		}
	}

	if (!OpenVOBs(vobs)) {
		return false;
	}

	if (isAob) {
		size_t i = 0;
		BYTE data[2048];
		int first_substreamID = 0;
		int first_channelgroup = 0;

		for (; i < m_pChapters.GetCount(); i++) {
			if (m_pChapters[i].track == 0) { // optimization. check only the beginning Title, not all the tracks.
				int pos = Seek(m_pChapters[i].first_sector);

				if (pos != m_pChapters[i].first_sector || !Read(data)) {
					break;
				}

				if (*(DWORD*)(data) != 0xba010000) { // pack hdr
					break;
				}
				int offset = 14 + (data[13] & 0x7); // stuffing_length
				if (*(DWORD*)(data + offset) == 0xbb010000) { // system header
					offset += 4;
					offset += (data[offset] << 8) + data[offset + 1] + 2;
				}
				// pes private1
				offset += 3;
				if (data[offset] != 0xBD || (data[offset + 1] << 8) + data[offset + 2] < 8) {
					break;
				}
				offset += 3 + 2;
				offset += data[offset] + 1;

				int substreamID = data[offset];
				int channelgroup = 0;
				if (substreamID == PCM_STREAM_ID) {
					channelgroup = data[offset + 10];
				}
				else if (substreamID == MLP_STREAM_ID) {
					offset += 4;
					for (; offset < 2048 - 14; offset++) {
						if (*(DWORD*)(data + offset) == 0xBB6F72F8) { // MLP signature
							channelgroup = data[offset + 7];
							break;
						}
					}
				}
				else {
					break;
				}

				if (!first_substreamID) {
					first_substreamID = substreamID;
					first_channelgroup = channelgroup;
				}
				else if (first_substreamID != substreamID || first_channelgroup != channelgroup) {
					break;
				}
			}
		}

		if (i < m_pChapters.GetCount()) {
			m_rtDuration = m_pChapters[i].rtime;
			m_pChapters.SetCount(i);
		}
	}

	int start_sector = m_pChapters[0].first_sector;
	int end_sector = m_pChapters[m_pChapters.GetCount() - 1].last_sector;
	SetOffsets(start_sector, end_sector);

	return true;
}

bool CVobFile::OpenVOBs(const CAtlList<CString>& vobs)
{
	Close();

	if (vobs.GetCount() == 0) {
		return false;
	}

	POSITION pos = vobs.GetHeadPosition();
	while(pos) {
		CString fn = vobs.GetNext(pos);

		WIN32_FIND_DATA fd;
		HANDLE h = FindFirstFile(fn, &fd);
		if (h == INVALID_HANDLE_VALUE) {
			m_files.RemoveAll();
			return false;
		}
		FindClose(h);

		file_t f;
		f.fn = fn;
		f.size = (int)(((__int64(fd.nFileSizeHigh) << 32) | fd.nFileSizeLow) / 2048);
		m_files.Add(f);

		m_size += f.size;
	}

	if (m_files.GetCount() > 0 && CDVDSession::Open(m_files[0].fn)) {
		for (size_t i = 0; !m_fHasTitleKey && i < m_files.GetCount(); i++) {
			if (BeginSession()) {
				m_fDVD = true;
				Authenticate();
				m_fHasDiscKey = GetDiscKey();
				EndSession();
			} else {
				CString fn = m_files[0].fn;
				fn.MakeLower();

				if (fn.Find(_T(":\\video_ts")) == 1 && GetDriveType(fn.Left(3)) == DRIVE_CDROM) {
					m_fDVD = true;
				}

				break;
			}

			if (tp_udf_file f = udf_find_file(m_hDrive, 0, CStringA(m_files[i].fn.Mid(m_files[i].fn.Find(':')+1)))) {
				DWORD start, end;
				if (udf_get_lba(m_hDrive, f, &start, &end)) {
					if (BeginSession()) {
						Authenticate();
						m_fHasTitleKey = GetTitleKey(start + f->partition_lba, m_TitleKey);
						EndSession();
					}
				}

				udf_free(f);
			}

			BYTE key[5];
			if (HasTitleKey(key) && i == 0) {
				i++;

				if (BeginSession()) {
					m_fDVD = true;
					Authenticate();
					m_fHasDiscKey = GetDiscKey();
					EndSession();
				} else {
					break;
				}

				if (tp_udf_file f = udf_find_file(m_hDrive, 0, CStringA(m_files[i].fn.Mid(m_files[i].fn.Find(':')+1)))) {
					DWORD start, end;
					if (udf_get_lba(m_hDrive, f, &start, &end)) {
						if (BeginSession()) {
							Authenticate();
							m_fHasTitleKey = GetTitleKey(start + f->partition_lba, m_TitleKey);
							EndSession();
						}
					}

					udf_free(f);
				}

				if (!m_fHasTitleKey) {
					memcpy(m_TitleKey, key, 5);
					m_fHasTitleKey = true;
				}
			}
		}
	}

	return true;
}

bool CVobFile::SetOffsets(int start_sector, int end_sector)
{
	int length = 0;
	for (size_t i = 0; i < m_files.GetCount(); i++) {
		length += m_files[i].size;
	}

	m_offset = max(0, start_sector);
	m_size = end_sector > 0 ? min(end_sector, length) : length;

	if (start_sector < 0 || start_sector >= length || end_sector > length) {
		return false; // offset assigned, but there were problems.
	}

	return true;
}

void CVobFile::Close()
{
	CDVDSession::Close();
	m_files.RemoveAll();
	m_iFile = -1;
	m_pos = m_size = m_offset = 0;
	m_file.Close();
	m_fDVD = m_fHasDiscKey = m_fHasTitleKey = false;
}

int CVobFile::GetLength() const
{
	return (m_size - m_offset);
}

int CVobFile::GetPosition() const
{
	return (m_pos - m_offset);
}

int CVobFile::Seek(int pos)
{
	pos = min(max(pos + m_offset, m_offset), m_size - 1);

	int i = -1;
	int size = 0;

	// this suxx, but won't take long
	do {
		size += m_files[++i].size;
	} while(i < (int)m_files.GetCount() && pos >= size);

	if (i != m_iFile && i < (int)m_files.GetCount()) {
		if (!m_file.Open(m_files[i].fn)) {
			return(m_pos);
		}

		m_iFile = i;
	}

	m_pos = pos;

	pos -= (size - m_files[i].size);
	m_file.Seek(pos);

	return GetPosition();
}

bool CVobFile::Read(BYTE* buff)
{
	if (m_pos >= m_size) {
		return false;
	}

	if (m_file.IsOpen() && m_file.GetPositionLBA() == m_file.GetLengthLBA()) {
		m_file.Close();
	}

	if (!m_file.IsOpen()) {
		if (m_iFile >= (int)m_files.GetCount() - 1) {
			return false;
		}

		if (!m_file.Open(m_files[++m_iFile].fn)) {
			m_iFile = -1;
			return false;
		}
	}

	if (!m_file.Read(buff)) {
		// dvd still locked?
		return false;
	}

	m_pos++;

	if (buff[0x14] & 0x30) {
		if (m_fHasTitleKey) {
			CSSdescramble(buff, m_TitleKey);
			buff[0x14] &= ~0x30;
		} else {
			// under win9x this is normal, but I'm not developing under win9x :P
			ASSERT(0);
		}
	}

	return true;
}

BSTR CVobFile::GetTrackName(UINT aTrackIdx) const
{
	CString TrackName;
	m_pStream_Lang.Lookup(aTrackIdx, TrackName);
	return TrackName.AllocSysString();
}

REFERENCE_TIME CVobFile::GetChapterTime(UINT ChapterNumber) const
{
	if (ChapterNumber >= m_pChapters.GetCount()) {
		return 0;
	}

	return m_pChapters[ChapterNumber].rtime;
}

__int64 CVobFile::GetChapterPos(UINT ChapterNumber) const
{
	if (ChapterNumber >= m_pChapters.GetCount()) {
		return 0;
	}

	return 2048i64 * (m_pChapters[ChapterNumber].first_sector - m_pChapters[0].first_sector);
}

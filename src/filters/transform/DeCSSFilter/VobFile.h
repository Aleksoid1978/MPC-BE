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

#pragma once

#include <atlbase.h>
#include <atlcoll.h>
#include <winddk/ntddcdvd.h>

#include "../../../DSUtil/DSUtil.h"

class CDVDSession
{
protected:
	HANDLE m_hDrive;

	DVD_SESSION_ID m_session;
	bool BeginSession();
	void EndSession();

	BYTE m_SessionKey[5];
	bool Authenticate();

	BYTE m_DiscKey[6], m_TitleKey[6];
	bool GetDiscKey();
	bool GetTitleKey(int lba, BYTE* pKey);

public:
	CDVDSession();
	virtual ~CDVDSession();

	bool Open(LPCTSTR path);
	void Close();

	operator HANDLE() const { return m_hDrive; }
	operator DVD_SESSION_ID() const { return m_session; }

	bool SendKey(DVD_KEY_TYPE KeyType, BYTE* pKeyData);
	bool ReadKey(DVD_KEY_TYPE KeyType, BYTE* pKeyData, int lba = 0);
};

class CLBAFile : private CFile
{
public:
	CLBAFile();
	virtual ~CLBAFile();

	bool IsOpen() const;

	bool Open(LPCTSTR path);
	void Close();

	int GetLengthLBA() const;
	int GetPositionLBA() const;
	int Seek(int lba);
	bool Read(BYTE* buff);
};

class CVobFile : public CDVDSession
{
	struct chapter_t {
		REFERENCE_TIME rtime;
		UINT32 first_sector;
		UINT32 last_sector;
		UINT32 title:16, track:8;
	};

	// all files
	struct file_t {
		CString fn;
		int size;
	};
	CAtlArray<file_t> m_files;	// group of vob files
	int m_offset;				// first sector for group of vob files
	int m_size;					// last sector for group of vob files
	int m_pos;					// current sector for group of vob files

	// currently opened file
	int m_iFile;				// current vob file index
	CLBAFile m_file;			// current vob file

	// attribs
	bool m_fDVD, m_fHasDiscKey, m_fHasTitleKey;

	CAtlMap<DWORD, CString> m_pStream_Lang;

	CAtlArray<chapter_t> m_pChapters;

	REFERENCE_TIME	m_rtDuration;
	fraction_t		m_Aspect;

public:
	CVobFile();
	virtual ~CVobFile();

	static bool GetTitleInfo(LPCTSTR fn, ULONG nTitleNum, ULONG& VTSN /* out */, ULONG& TTN /* out */); // video_ts.ifo

	bool OpenIFO(CString fn, CAtlList<CString>& files /* out */, ULONG nProgNum = 0); // vts ifo

	bool IsDVD() const;
	bool HasDiscKey(BYTE* key) const;
	bool HasTitleKey(BYTE* key) const;

	int GetLength() const;
	int GetPosition() const;
	int Seek(int pos);
	bool Read(BYTE* buff);

	BSTR			GetTrackName(UINT aTrackIdx) const;
	UINT			GetChaptersCount() const { return (UINT)m_pChapters.GetCount(); }
	REFERENCE_TIME	GetChapterTime(UINT ChapterNumber) const;
	REFERENCE_TIME	GetDuration() const { return m_rtDuration; }
	fraction_t		GetAspectRatio() const { return m_Aspect; }

private:
	bool OpenVOBs(const CAtlList<CString>& files); // vts vobs

	bool SetOffsets(int start_sector, int end_sector = -1); // video vob offset in lba
	void Close();

	CFile		m_ifoFile;
	BYTE		ReadByte();
	WORD		ReadWord();
	DWORD		ReadDword();
};

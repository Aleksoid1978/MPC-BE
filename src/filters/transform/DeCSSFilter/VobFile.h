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

#define ALL_PGCs	ULONG_MAX

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
	// all files
	struct file_t {
		CString fn;
		int size;
	};

	struct chapter_t {
		REFERENCE_TIME rtime;
		UINT32 first_sector;
		UINT32 last_sector;
		UINT32 title:16, track:8;
	};

	CAtlArray<file_t> m_files;
	int m_iFile;
	int m_pos, m_size, m_offset;

	// currently opened file
	CLBAFile m_file;

	// attribs
	bool m_fDVD, m_fHasDiscKey, m_fHasTitleKey;

	CAtlMap<DWORD, CString> m_pStream_Lang;

	CAtlArray<chapter_t> m_pChapters;

	REFERENCE_TIME	m_rtDuration;
	AV_Rational		m_Aspect;

public:
	CVobFile();
	virtual ~CVobFile();

	static bool GetTitleInfo(LPCTSTR fn, ULONG nTitleNum, ULONG& VTSN /* out */, ULONG& TTN /* out */); // video_ts.ifo

	bool IsDVD() const;
	bool HasDiscKey(BYTE* key) const;
	bool HasTitleKey(BYTE* key) const;

	bool OpenIFO(CString fn, CAtlList<CString>& files /* out */, ULONG nProgNum = ALL_PGCs); // vts ifo
	bool OpenVOBs(const CAtlList<CString>& files); // vts vobs
	bool SetOffsets(int start_sector, int end_sector = -1); // video vob offset in lba
	void Close();

	int GetLength() const;
	int GetPosition() const;
	int Seek(int pos);
	bool Read(BYTE* buff);

	BSTR GetTrackName(UINT aTrackIdx) const;

	UINT			GetChaptersCount() const { return (UINT)m_pChapters.GetCount(); }
	REFERENCE_TIME	GetChapterOffset(UINT ChapterNumber) const;

	REFERENCE_TIME	GetDuration() const { return m_rtDuration; }
	AV_Rational		GetAspect() const { return m_Aspect; }

private:
	CFile		m_ifoFile;
	DWORD		ReadDword();
	WORD		ReadWord();
	BYTE		ReadByte();
	void		ReadBuffer(BYTE* pBuff, DWORD nLen);
};

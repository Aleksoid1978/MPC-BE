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

#include <atlcoll.h>
#include <winddk/ntddcdvd.h>

// CDVDSession

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

	bool Open(LPCWSTR path);
	void Close();

	operator HANDLE() const { return m_hDrive; }
	operator DVD_SESSION_ID() const { return m_session; }

	bool SendKey(DVD_KEY_TYPE KeyType, BYTE* pKeyData);
	bool ReadKey(DVD_KEY_TYPE KeyType, BYTE* pKeyData, int lba = 0);
};

// CLBAFile

class CLBAFile : private CFile
{
public:
	CLBAFile();
	virtual ~CLBAFile();

	bool IsOpen() const;

	bool Open(LPCWSTR path);
	void Close();

	int GetLengthLBA() const;
	int GetPositionLBA() const;
	int Seek(int lba);
	bool Read(BYTE* buff);
};

// CVobFile

class CVobFile : public CDVDSession
{
	struct file_t {
		CString fn;
		int size;
	};

	// all files
	CAtlArray<file_t> m_files;	// group of vob files
	int m_offset;				// first sector for group of vob files
	int m_size;					// last sector for group of vob files
	int m_pos;					// current sector for group of vob files

	// currently opened file
	int m_iFile;				// current vob file index
	CLBAFile m_file;			// current vob file

	// attribs
	bool m_fDVD, m_fHasDiscKey, m_fHasTitleKey;

public:
	CVobFile();
	virtual ~CVobFile();

	bool OpenVOBs(const CAtlList<CString>& files); // vts vobs
	bool SetOffsets(int start_sector, int end_sector = -1); // video vob offset in lba
	void Close();

	int GetLength() const;
	int GetPosition() const;
	int Seek(int pos);
	bool Read(BYTE* buff);

	bool IsDVD() const;
	bool HasDiscKey(BYTE* key) const;
	bool HasTitleKey(BYTE* key) const;
};

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
#include "CSSauth.h"
#include "CSSscramble.h"
#include "udf.h"

#include "VobFile.h"

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
}

CVobFile::~CVobFile()
{
	Close();
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

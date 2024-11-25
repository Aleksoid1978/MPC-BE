/*
 * (C) 2018-2024 see Authors.txt
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
#include <ShlObj_core.h>
#include <KnownFolders.h>
#include "FileHandle.h"
#include "Utils.h"
#include "CryptoUtils.h"
#include "Log.h"
#include "Profile.h"
#include "text.h"

CStringW GetIniProgramDir()
{
	CStringW path = GetRenameFileExt(GetModulePath(nullptr), L".ini");

	return path;
}

CStringW GetIniUserProfile()
{
	CStringW path = GetIniProgramDir();
	const CStringW fname = ::PathFindFileNameW(path);

	PWSTR pathRoamingAppData = nullptr;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &pathRoamingAppData);
	if (SUCCEEDED(hr)) {
		path = CStringW(pathRoamingAppData) + L"\\MPC-BE\\" + fname;
	}
	CoTaskMemFree(pathRoamingAppData);

	return path;
}

// CProfile

CProfile::CProfile()
{
	CStringW path = GetIniProgramDir();
	if (::PathFileExistsW(path)) {
		m_IniPath = path;
		m_bIniProgDir = true;
		return;
	}

	path = GetIniUserProfile();
	if (::PathFileExistsW(path)) {
		m_IniPath = path;
		return;
	}

	OpenRegistryKey();
}

LONG CProfile::OpenRegistryKey()
{
	LONG lResult = ERROR_SUCCESS;

	if (!m_hAppRegKey) {
		DWORD dwDisposition = 0;
		lResult = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\MPC-BE", 0, nullptr, 0, KEY_READ, nullptr, &m_hAppRegKey, &dwDisposition);
		DLogIf(lResult != ERROR_SUCCESS, L"OpenRegistryKey(): ERROR! The opening of the registry key failed.");
	}

	return lResult;
}

void CProfile::InitIni()
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	if (m_hAppRegKey) {
		return;
	}

	// Don't reread mpc-be.ini if the cache needs to be flushed or it was accessed recently
	const ULONGLONG tick = GetTickCount64();
	if (m_bIniFirstInit && (m_bIniNeedFlush || tick - m_IniLastAccessTick < 100u)) {
		m_IniLastAccessTick = tick;
		return;
	}

	m_bIniFirstInit = true;
	m_IniLastAccessTick = tick;

	if (!::PathFileExistsW(m_IniPath)) {
		return;
	}

	FILE* fp;
	int fpStatus;
	do { // Open mpc-be.ini in UNICODE mode, retry if it is already being used by another process
		fp = _wfsopen(m_IniPath, L"r, ccs=UNICODE", _SH_SECURE);
		if (fp || (GetLastError() != ERROR_SHARING_VIOLATION)) {
			break;
		}
		Sleep(100);
	} while (true);
	if (!fp) {
		ASSERT(0);
		return;
	}
	if (_ftell_nolock(fp) == 0L) {
		// No BOM was consumed, assume mpc-be.ini is ANSI encoded
		fpStatus = fclose(fp);
		ASSERT(fpStatus == 0);
		do { // Reopen mpc-be.ini in ANSI mode, retry if it is already being used by another process
			fp = _wfsopen(m_IniPath, L"r", _SH_SECURE);
			if (fp || (GetLastError() != ERROR_SHARING_VIOLATION)) {
				break;
			}
			Sleep(100);
		} while (true);
		if (!fp) {
			ASSERT(0);
			return;
		}
	}

	CStdioFile file(fp);

	ASSERT(!m_bIniNeedFlush);
	m_ProfileMap.clear();

	CStringW line, section, var, val;
	while (file.ReadString(line)) {
		// Parse mpc-be.ini file, this parser:
		//  - doesn't trim whitespaces
		//  - doesn't remove quotation marks
		//  - omits keys with empty names
		//  - omits unnamed sections
		int pos = 0;
		if (line[0] == '[') {
			pos = line.Find(']');
			if (pos == -1) {
				continue;
			}
			section = line.Mid(1, pos - 1);
		} else if (line[0] != ';') {
			pos = line.Find('=');
			if (pos == -1) {
				continue;
			}
			var = line.Mid(0, pos);
			val = line.Mid(pos + 1);
			if (!section.IsEmpty() && !var.IsEmpty()) {
				m_ProfileMap[section][var] = val;
			}
		}
	}
	fpStatus = fclose(fp);
	ASSERT(fpStatus == 0);

	m_IniLastAccessTick = GetTickCount64(); // update the last access tick because reading the file can take a long time
}

bool CProfile::StoreSettingsTo(const SettingsLocation newLocation)
{
	if (newLocation == SETS_REGISTRY) {
		if (m_hAppRegKey) {
			DLog(L"StoreSettingsTo: The settings are already stored in the registry.");
			return true;
		}

		InitIni();
		if (::PathFileExistsW(m_IniPath) && _wremove(m_IniPath) != 0) {
			DLog(L"StoreSettingsTo: WARNING! The INI file can not be deleted. Perhaps this is protection against changes in settings. Transfer of settings is canceled.");
			return false;
		}

		OpenRegistryKey();
		if (m_hAppRegKey) {
			m_IniPath.Empty();
			m_bIniProgDir = false;

			return true;
		}

		return false;
	}

	if (m_IniPath.GetLength()) {
		if (newLocation == SETS_USERPROFILE && !m_bIniProgDir) {
			DLog(L"StoreSettingsTo: The settings are already stored in the user profile.");
			return true;
		}
		if (newLocation == SETS_PROGRAMDIR && m_bIniProgDir) {
			DLog(L"StoreSettingsTo: The settings are already stored in the program folder.");
			return true;
		}
	}

	if (m_hAppRegKey) {
		LONG lResult = SHDeleteKeyW(m_hAppRegKey, L"");
		RegCloseKey(m_hAppRegKey);
		m_hAppRegKey = nullptr;
	}

	InitIni();
	if (::PathFileExistsW(m_IniPath) && _wremove(m_IniPath) != 0) {
		DLog(L"StoreSettingsToIni(): WARNING! The old INI file can not be deleted. Perhaps this is protection against changes in settings. Transfer of settings is canceled.");
		return false;
	}

	const bool bProgramDir = (newLocation == SETS_PROGRAMDIR);
	const CStringW newIniPath = bProgramDir ? GetIniProgramDir() : GetIniUserProfile();
	CFile file;
	if (file.Open(newIniPath, CFile::modeWrite | CFile::modeCreate | CFile::modeNoTruncate)) {
		// ok. the file can be opened and written.
		file.Close();

		m_IniPath = newIniPath;
		m_bIniProgDir = bProgramDir;
		Flush(true);

		return true;
	}

	return false;
}

bool CProfile::ReadBool(const wchar_t* section, const wchar_t* entry, bool& value)
{
	int v = value;
	bool ret = ReadInt(section, entry, v);
	if (ret) {
		// very strict check
		if (v == 0) {
			value = false;
		} else if (v == 1) {
			value = true;
		} else {
			ret = false;
		}
	}

	return ret;
}

bool CProfile::ReadInt(const wchar_t* section, const wchar_t* entry, int& value)
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	bool ret = false;

	if (m_hAppRegKey) {
		CRegKey regkey;
		if (ERROR_SUCCESS == regkey.Open(m_hAppRegKey, section, KEY_READ)) {
			if (ERROR_SUCCESS == regkey.QueryDWORDValue(entry, *(DWORD*)&value)) {
				ret = true;
			}
			regkey.Close();
		}
	} else {
		InitIni();
		auto it1 = m_ProfileMap.find(section);
		if (it1 != m_ProfileMap.end()) {
			auto it2 = it1->second.find(entry);
			if (it2 != it1->second.end()) {
				ret = StrToInt32(it2->second, value);
			}
		}
	}

	return ret;
}

bool CProfile::ReadInt(const wchar_t* section, const wchar_t* entry, int& value, const int lo, const int hi)
{
	int v = value;
	bool ret = ReadInt(section, entry, v);
	if (ret) {
		if (v >= lo && v <= hi) {
			value = v;
		} else {
			ret = false;
		}
	}

	return ret;
}

bool CProfile::ReadUInt(const wchar_t* section, const wchar_t* entry, unsigned& value)
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	bool ret = false;

	if (m_hAppRegKey) {
		CRegKey regkey;
		if (ERROR_SUCCESS == regkey.Open(m_hAppRegKey, section, KEY_READ)) {
			if (ERROR_SUCCESS == regkey.QueryDWORDValue(entry, *(DWORD*)&value)) {
				ret = true;
			}
			regkey.Close();
		}
	}
	else {
		InitIni();
		auto it1 = m_ProfileMap.find(section);
		if (it1 != m_ProfileMap.end()) {
			auto it2 = it1->second.find(entry);
			if (it2 != it1->second.end()) {
				ret = StrToUInt32(it2->second, value);
			}
		}
	}

	return ret;
}

bool CProfile::ReadUInt(const wchar_t* section, const wchar_t* entry, unsigned& value, const unsigned lo, const unsigned hi)
{
	unsigned v = value;
	bool ret = ReadUInt(section, entry, v);
	if (ret) {
		if (v >= lo && v <= hi) {
			value = v;
		}
		else {
			ret = false;
		}
	}

	return ret;
}

bool CProfile::ReadInt64(const wchar_t* section, const wchar_t* entry, __int64& value)
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	bool ret = false;

	if (m_hAppRegKey) {
		CRegKey regkey;
		if (ERROR_SUCCESS == regkey.Open(m_hAppRegKey, section, KEY_READ)) {
			if (ERROR_SUCCESS == regkey.QueryQWORDValue(entry, *(ULONGLONG*)&value)) {
				ret = true;
			}
			regkey.Close();
		}
	} else {
		InitIni();
		auto it1 = m_ProfileMap.find(section);
		if (it1 != m_ProfileMap.end()) {
			auto it2 = it1->second.find(entry);
			if (it2 != it1->second.end()) {
				ret = StrToInt64(it2->second, value);
			}
		}
	}

	return ret;
}

bool CProfile::ReadInt64(const wchar_t* section, const wchar_t* entry, __int64& value, const __int64 lo, const __int64 hi)
{
	__int64 v = value;
	bool ret = ReadInt64(section, entry, v);
	if (ret) {
		if (v >= lo && v <= hi) {
			value = v;
		} else {
			ret = false;
		}
	}

	return ret;
}

bool CProfile::ReadDouble(const wchar_t* section, const wchar_t* entry, double& value)
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	bool ret = false;

	if (m_hAppRegKey) {
		CRegKey regkey;
		if (ERROR_SUCCESS == regkey.Open(m_hAppRegKey, section, KEY_READ)) {
			ULONG nChars = 0;
			if (ERROR_SUCCESS == regkey.QueryStringValue(entry, NULL, &nChars) && nChars > 0) {
				CStringW str;
				if (ERROR_SUCCESS == regkey.QueryStringValue(entry, str.GetBufferSetLength(nChars - 1), &nChars)) {
					ret = StrToDouble(str, value);
				}
			}
			regkey.Close();
		}
	} else {
		InitIni();
		auto it1 = m_ProfileMap.find(section);
		if (it1 != m_ProfileMap.end()) {
			auto it2 = it1->second.find(entry);
			if (it2 != it1->second.end()) {
				ret = StrToDouble(it2->second, value);
			}
		}
	}

	return ret;
}

bool CProfile::ReadDouble(const wchar_t* section, const wchar_t* entry, double& value, const double lo, const double hi)
{
	double v = value;
	bool ret = ReadDouble(section, entry, v);
	if (ret) {
		if (v >= lo && v <= hi) {
			value = v;
		} else {
			ret = false;
		}
	}

	return ret;
}

bool CProfile::ReadHex32(const wchar_t* section, const wchar_t* entry, unsigned& value)
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	bool ret = false;

	if (m_hAppRegKey) {
		CRegKey regkey;
		if (ERROR_SUCCESS == regkey.Open(m_hAppRegKey, section, KEY_READ)) {
			if (ERROR_SUCCESS == regkey.QueryDWORDValue(entry, *(DWORD*)&value)) {
				ret = true;
			}
			regkey.Close();
		}
	}
	else {
		InitIni();
		auto it1 = m_ProfileMap.find(section);
		if (it1 != m_ProfileMap.end()) {
			auto it2 = it1->second.find(entry);
			if (it2 != it1->second.end()) {
				ret = StrHexToUInt32(it2->second, value);
			}
		}
	}

	return ret;
}

bool CProfile::ReadString(const wchar_t* section, const wchar_t* entry, CStringW& value)
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	bool ret = false;

	if (m_hAppRegKey) {
		CRegKey regkey;
		if (ERROR_SUCCESS == regkey.Open(m_hAppRegKey, section, KEY_READ)) {
			ULONG nChars = 0;
			if (ERROR_SUCCESS == regkey.QueryStringValue(entry, NULL, &nChars) && nChars > 0) {
				if (ERROR_SUCCESS == regkey.QueryStringValue(entry, value.GetBufferSetLength(nChars - 1), &nChars)) {
					ret = true;
				}
			}
			regkey.Close();
		}
	} else {
		InitIni();
		auto it1 = m_ProfileMap.find(section);
		if (it1 != m_ProfileMap.end()) {
			auto it2 = it1->second.find(entry);
			if (it2 != it1->second.end()) {
				value = it2->second;
				ret = true;
			}
		}
	}

	return ret;
}

bool CProfile::ReadBinaryOld(const wchar_t* section, const wchar_t* entry, BYTE** ppdata, unsigned& nbytes)
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	bool ret = false;

	if (m_hAppRegKey) {
		CRegKey regkey;
		if (ERROR_SUCCESS == regkey.Open(m_hAppRegKey, section, KEY_READ)) {
			if (ERROR_SUCCESS == regkey.QueryBinaryValue(entry, NULL, (ULONG*)&nbytes)) {
				*ppdata = new(std::nothrow) BYTE[nbytes];
				if (*ppdata && ERROR_SUCCESS == regkey.QueryBinaryValue(entry, *ppdata, (ULONG*)&nbytes)) {
					ret = true;
				}
			}
			regkey.Close();
		}
	} else {
		CStringW valueStr;

		InitIni();
		auto it1 = m_ProfileMap.find(section);
		if (it1 != m_ProfileMap.end()) {
			auto it2 = it1->second.find(entry);
			if (it2 != it1->second.end()) {
				valueStr = it2->second;
			}
		}

		if (valueStr.IsEmpty()) {
			return false;
		}
		int length = valueStr.GetLength();
		// Encoding: each 4-bit sequence is coded in one character, from 'A' for 0x0 to 'P' for 0xf
		if (length % 2) {
			ASSERT(0);
			return false;
		}
		for (int i = 0; i < length; i++) {
			if (valueStr[i] < 'A' || valueStr[i] > 'P') {
				ASSERT(0);
				return false;
			}
		}
		nbytes = length / 2;
		*ppdata = new(std::nothrow) BYTE[nbytes];
		if (!(*ppdata)) {
			ASSERT(0);
			return false;
		}
		for (ULONG i = 0; i < nbytes; i++) {
			(*ppdata)[i] = BYTE((valueStr[i * 2] - 'A') | ((valueStr[i * 2 + 1] - 'A') << 4));
		}
		ret = true;
	}

	return ret;
}

bool CProfile::ReadBinary(const wchar_t* section, const wchar_t* entry, BYTE** ppdata, unsigned& nbytes)
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	bool ret = false;

	if (m_hAppRegKey) {
		CRegKey regkey;
		if (ERROR_SUCCESS == regkey.Open(m_hAppRegKey, section, KEY_READ)) {
			if (ERROR_SUCCESS == regkey.QueryBinaryValue(entry, NULL, (ULONG*)&nbytes)) {
				*ppdata = new(std::nothrow) BYTE[nbytes];
				if (*ppdata && ERROR_SUCCESS == regkey.QueryBinaryValue(entry, *ppdata, (ULONG*)&nbytes)) {
					ret = true;
				}
			}
			regkey.Close();
		}
	} else {
		CStringW valueStr;

		InitIni();
		auto it1 = m_ProfileMap.find(section);
		if (it1 != m_ProfileMap.end()) {
			auto it2 = it1->second.find(entry);
			if (it2 != it1->second.end()) {
				valueStr = it2->second;
			}
		}

		nbytes = Base64ToBynary(valueStr, ppdata);
		ret = nbytes ? true : false;
	}

	return ret;
}

bool CProfile::WriteBool(const wchar_t* section, const wchar_t* entry, const bool value)
{
	return WriteInt(section, entry, value ? 1 : 0); // strictly write "true" as 1
}

bool CProfile::WriteInt(const wchar_t* section, const wchar_t* entry, const int value)
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	bool ret = false;

	if (m_hAppRegKey) {
		CRegKey regkey;
		if (ERROR_SUCCESS == regkey.Create(m_hAppRegKey, section)) {
			if (ERROR_SUCCESS == regkey.SetDWORDValue(entry, (DWORD)value)) {
				ret = true;
			}
			regkey.Close();
		}
	} else {
		InitIni();
		CStringW valueStr;
		valueStr.Format(L"%d", value);
		CStringW& old = m_ProfileMap[section][entry];
		if (old != valueStr) {
			old = valueStr;
			m_bIniNeedFlush = true;
		}
		ret = true;
	}

	return ret;
}

bool CProfile::WriteUInt(const wchar_t* section, const wchar_t* entry, const unsigned value)
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	bool ret = false;

	if (m_hAppRegKey) {
		CRegKey regkey;
		if (ERROR_SUCCESS == regkey.Create(m_hAppRegKey, section)) {
			if (ERROR_SUCCESS == regkey.SetDWORDValue(entry, (DWORD)value)) {
				ret = true;
			}
			regkey.Close();
		}
	}
	else {
		InitIni();
		CStringW valueStr;
		valueStr.Format(L"%u", value);
		CStringW& old = m_ProfileMap[section][entry];
		if (old != valueStr) {
			old = valueStr;
			m_bIniNeedFlush = true;
		}
		ret = true;
	}

	return ret;
}

bool CProfile::WriteInt64(const wchar_t* section, const wchar_t* entry, const __int64 value)
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	bool ret = false;

	if (m_hAppRegKey) {
		CRegKey regkey;
		if (ERROR_SUCCESS == regkey.Create(m_hAppRegKey, section)) {
			if (ERROR_SUCCESS == regkey.SetQWORDValue(entry, (ULONGLONG)value)) {
				ret = true;
			}
			regkey.Close();
		}
	} else {
		CStringW valueStr;
		valueStr.Format(L"%I64d", value);

		InitIni();
		CStringW& old = m_ProfileMap[section][entry];
		if (old != valueStr) {
			old = valueStr;
			m_bIniNeedFlush = true;
		}
		ret = true;
	}

	return ret;
}

bool CProfile::WriteDouble(const wchar_t* section, const wchar_t* entry, const double value)
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	bool ret = false;

	CStringW valueStr;
	valueStr.Format(L"%.4f", value);

	if (m_hAppRegKey) {
		CRegKey regkey;
		if (ERROR_SUCCESS == regkey.Create(m_hAppRegKey, section)) {
			if (ERROR_SUCCESS == regkey.SetStringValue(entry, valueStr)) {
				ret = true;
			}
			regkey.Close();
		}
	} else {
		InitIni();
		CStringW& old = m_ProfileMap[section][entry];
		if (old != valueStr) {
			old = valueStr;
			m_bIniNeedFlush = true;
		}
		ret = true;
	}

	return ret;
}

bool CProfile::WriteHex32(const wchar_t* section, const wchar_t* entry, const unsigned value)
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	bool ret = false;

	if (m_hAppRegKey) {
		CRegKey regkey;
		if (ERROR_SUCCESS == regkey.Create(m_hAppRegKey, section)) {
			if (ERROR_SUCCESS == regkey.SetDWORDValue(entry, (DWORD)value)) {
				ret = true;
			}
			regkey.Close();
		}
	}
	else {
		InitIni();
		CStringW valueStr;
		valueStr.Format(L"0x%06X", value);
		CStringW& old = m_ProfileMap[section][entry];
		if (old != valueStr) {
			old = valueStr;
			m_bIniNeedFlush = true;
		}
		ret = true;
	}

	return ret;
}

bool CProfile::WriteString(const wchar_t* section, const wchar_t* entry, const CStringW& value)
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	bool ret = false;

	if (m_hAppRegKey) {
		CRegKey regkey;
		if (ERROR_SUCCESS == regkey.Create(m_hAppRegKey, section)) {
			if (ERROR_SUCCESS == regkey.SetStringValue(entry, value)) {
				ret = true;
			}
			regkey.Close();
		}
	} else {
		InitIni();
		CStringW& old = m_ProfileMap[section][entry];
		if (old != value) {
			old = value;
			m_bIniNeedFlush = true;
		}
		ret = true;
	}

	return ret;
}

bool CProfile::WriteBinaryOld(const wchar_t* section, const wchar_t* entry, const BYTE* pdata, const unsigned nbytes)
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	bool ret = false;

	if (m_hAppRegKey) {
		CRegKey regkey;
		if (ERROR_SUCCESS == regkey.Create(m_hAppRegKey, section)) {
			if (ERROR_SUCCESS == regkey.SetBinaryValue(entry, pdata, nbytes)) {
				ret = true;
			}
			regkey.Close();
		}
		return ret;
	} else {
		CStringW valueStr;
		WCHAR* buffer = valueStr.GetBuffer(nbytes * 2);
		// Encoding: each 4-bit sequence is coded in one character, from 'A' for 0x0 to 'P' for 0xf
		for (ULONG i = 0; i < nbytes; i++) {
			buffer[i * 2] = 'A' + (pdata[i] & 0xf);
			buffer[i * 2 + 1] = 'A' + (pdata[i] >> 4 & 0xf);
		}
		valueStr.ReleaseBufferSetLength(nbytes * 2);

		InitIni();
		CStringW& old = m_ProfileMap[section][entry];
		if (old != valueStr) {
			old = valueStr;
			m_bIniNeedFlush = true;
		}
		ret = true;
	}

	return ret;
}

bool CProfile::WriteBinary(const wchar_t* section, const wchar_t* entry, const BYTE* pdata, const unsigned nbytes)
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	bool ret = false;

	if (m_hAppRegKey) {
		CRegKey regkey;
		if (ERROR_SUCCESS == regkey.Create(m_hAppRegKey, section)) {
			if (ERROR_SUCCESS == regkey.SetBinaryValue(entry, pdata, nbytes)) {
				ret = true;
			}
			regkey.Close();
		}
		return ret;
	} else {
		CStringW base64 = BynaryToBase64W(pdata, nbytes);
		if (base64.GetLength()) {
			InitIni();
			CStringW& old = m_ProfileMap[section][entry];
			if (old != base64) {
				old = base64;
				m_bIniNeedFlush = true;
			}
			ret = true;
		}
	}

	return ret;
}

void CProfile::EnumValueNames(const wchar_t* section, std::vector<CStringW>& valuenames)
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	valuenames.clear();

	if (m_hAppRegKey) {
		CRegKey regkey;
		if (ERROR_SUCCESS == regkey.Open(m_hAppRegKey, section, KEY_READ)) {
			DWORD cValues     = 0;
			DWORD cchMaxValue = 0;

			// Get the class name and the value count.
			DWORD retCode = RegQueryInfoKeyW(
				regkey.m_hKey,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				&cValues,
				&cchMaxValue,
				nullptr,
				nullptr,
				nullptr);

			if (ERROR_SUCCESS == retCode && cValues && cchMaxValue) {
				std::vector<WCHAR> achValue(cchMaxValue + 1, '\0');

				for (DWORD i = 0, retCode = ERROR_SUCCESS; i < cValues; i++) {
					DWORD cchValue = cchMaxValue + 1;
					achValue[0] = '\0';
					retCode = RegEnumValueW(regkey.m_hKey, i, achValue.data(), &cchValue, NULL, NULL, NULL, NULL);
					if (retCode == ERROR_SUCCESS) {
						valuenames.emplace_back(achValue.data(), cchValue);
					}
				}
			}
			regkey.Close();
		}
	}
	else {
		InitIni();
		auto it1 = m_ProfileMap.find(section);
		if (it1 != m_ProfileMap.end()) {
			auto& sectionMap = it1->second;
			for (const auto& entry : sectionMap) {
				valuenames.emplace_back(entry.first);
			}
		}
	}
}

void CProfile::EnumSectionNames(const wchar_t* section, std::vector<CStringW>& sectionnames)
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	sectionnames.clear();

	if (m_hAppRegKey) {
		CRegKey regkey;
		if (ERROR_SUCCESS == regkey.Open(m_hAppRegKey, section, KEY_READ)) {
			DWORD cSubKeys    = 0;
			DWORD cbMaxSubKey = 0;

			// Get the class name and the value count.
			DWORD retCode = RegQueryInfoKeyW(
				regkey.m_hKey,
				nullptr,
				nullptr,
				nullptr,
				&cSubKeys,
				&cbMaxSubKey,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr);

			if (ERROR_SUCCESS == retCode && cSubKeys && cbMaxSubKey) {
				std::vector<WCHAR> achKey(cbMaxSubKey + 1, '\0');

				for (DWORD i = 0; i < cSubKeys; i++) {
					DWORD cbName = cbMaxSubKey + 1;
					achKey[0] = '\0';
					retCode = RegEnumKeyExW(regkey.m_hKey, i, achKey.data(), &cbName, nullptr, nullptr, nullptr, nullptr);
					if (retCode == ERROR_SUCCESS) {
						sectionnames.emplace_back(achKey.data(), cbName);
					}
				}
			}
			regkey.Close();
		}
	}
	else {
		InitIni();
		CStringW prefix(section);
		prefix += L'\\';

		auto it = m_ProfileMap.cbegin();
		while (it != m_ProfileMap.cend() && !StartsWith(it->first, prefix)) {
			++it;
		}

		while (it != m_ProfileMap.cend() && StartsWith(it->first, prefix)) {
			if (it->first.GetLength() > prefix.GetLength()) {
				sectionnames.emplace_back(it->first.Mid(prefix.GetLength()));
			}
			++it;
		}
	}
}

bool CProfile::DeleteValue(const wchar_t* section, const wchar_t* entry)
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	bool ret = false;

	if (m_hAppRegKey) {
		CRegKey regkey;
		if (ERROR_SUCCESS == regkey.Open(m_hAppRegKey, section, KEY_WRITE)) {
			if (ERROR_SUCCESS == regkey.DeleteValue(entry)) {
				ret = true;
			}
			regkey.Close();
		}
	}
	else {
		InitIni();

		auto it = m_ProfileMap.find(section);
		if (it != m_ProfileMap.end()) {
			if (it->second.erase(entry)) {
				m_bIniNeedFlush = true;
				ret = true;
			}
		}
	}

	return ret;
}

bool CProfile::DeleteSection(const wchar_t* section)
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	bool ret = false;

	if (m_hAppRegKey) {
		CRegKey regkey;
		if (ERROR_SUCCESS == regkey.Open(m_hAppRegKey, L"", KEY_WRITE)) {
			if (ERROR_SUCCESS == regkey.RecurseDeleteKey(section)) {
				ret = true;
			}
			regkey.Close();
		}
	}
	else {
		InitIni();
		const CStringW mainsection(section);
		const CStringW prefix(mainsection + L'\\');

		auto start = m_ProfileMap.cbegin();
		while (start != m_ProfileMap.cend() && start->first != mainsection && !StartsWith(start->first, prefix)) {
			++start;
		}

		if (start != m_ProfileMap.cend()) {
			auto end = std::next(start);
			while (end != m_ProfileMap.cend() && StartsWith(end->first, prefix)) {
				++end;
			}

			m_ProfileMap.erase(start, end);
			m_bIniNeedFlush = true;
			ret = true;
		}
	}

	return ret;
}

void CProfile::Flush(bool bForce)
{
	if (m_hAppRegKey || (!bForce && !m_bIniNeedFlush)) {
		return;
	}

	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	m_bIniNeedFlush = false;

	ASSERT(m_bIniFirstInit);
	ASSERT(m_IniPath.GetLength());

	FILE* fp;
	int fpStatus;
	do { // Open mpc-be.ini, retry if it is already being used by another process
		fp = _wfsopen(m_IniPath, L"w, ccs=UTF-8", _SH_SECURE);
		if (fp || (GetLastError() != ERROR_SHARING_VIOLATION)) {
			break;
		}
		Sleep(100);
	} while (true);
	if (!fp) {
		ASSERT(FALSE);
		return;
	}
	CStdioFile file(fp);
	CStringW line;
	try {
		file.WriteString(L"; MPC-BE\n");
		for (auto it1 = m_ProfileMap.begin(); it1 != m_ProfileMap.end(); ++it1) {
			line.Format(L"[%s]\n", it1->first);
			file.WriteString(line);
			for (auto it2 = it1->second.begin(); it2 != it1->second.end(); ++it2) {
				line.Format(L"%s=%s\n", it2->first, it2->second);
				file.WriteString(line);
			}
		}
	}
	catch (CFileException& e) {
		// Fail silently if disk is full
		UNREFERENCED_PARAMETER(e);
		ASSERT(FALSE);
	}

	fpStatus = fclose(fp);
	ASSERT(fpStatus == 0);
}

void CProfile::Clear()
{
	// Remove the settings
	if (m_hAppRegKey) {
		SHDeleteKeyW(m_hAppRegKey, L"");
	}
	else {
		ASSERT(m_IniPath.GetLength());
		CFile file;
		if (file.Open(m_IniPath, CFile::modeWrite)) {
			file.SetLength(0); // clear, but not delete
			file.Close();
		}
	}
}

SettingsLocation CProfile::GetSettingsLocation() const
{
	if (m_hAppRegKey) {
		return SETS_REGISTRY;
	}
	ASSERT(m_IniPath.GetLength());
	if (m_bIniProgDir) {
		return SETS_PROGRAMDIR;
	}
	return SETS_USERPROFILE;
}

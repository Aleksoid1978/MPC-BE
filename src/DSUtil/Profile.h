/*
 * (C) 2018 see Authors.txt
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

#include <mutex>
#include <map>

class CProfile
{
private:
	std::recursive_mutex m_Mutex;

	// registry
	HKEY m_hAppRegKey = nullptr;

	// INI file
	CStringW m_IniPath;
	struct KeyCmp {
		bool operator()(const CStringW& str1, const CStringW& str2) const {
			return str1.CompareNoCase(str2) < 0;
		}
	};
	std::map<CStringW, std::map<CStringW, CStringW, KeyCmp>, KeyCmp> m_ProfileMap;
	bool  m_bIniFirstInit = false;
	bool  m_bIniNeedFlush = false;
	DWORD m_dwIniLastAccessTick = 0;

public:
	CProfile();

private:
	LONG OpenRegistryKey();
	// read all the fields from the ini file
	void InitIni();

public:
	bool StoreSettingsToRegistry();
	bool StoreSettingsToIni();

	bool ReadBool  (const wchar_t* section, const wchar_t* entry, bool&     value);
	bool ReadInt   (const wchar_t* section, const wchar_t* entry, int&      value);
	bool ReadInt   (const wchar_t* section, const wchar_t* entry, int&      value, const int lo, const int hi);
	bool ReadInt64 (const wchar_t* section, const wchar_t* entry, __int64&  value);
	bool ReadInt64 (const wchar_t* section, const wchar_t* entry, __int64&  value, const __int64 lo, const __int64 hi);
	bool ReadDouble(const wchar_t* section, const wchar_t* entry, double&   value);
	bool ReadDouble(const wchar_t* section, const wchar_t* entry, double&   value, const double lo, const double hi);
	bool ReadString(const wchar_t* section, const wchar_t* entry, CStringW& value);
	bool ReadBinary(const wchar_t* section, const wchar_t* entry, BYTE** ppdata, unsigned& nbytes);

	bool WriteBool  (const wchar_t* section, const wchar_t* entry, const bool     value);
	bool WriteInt   (const wchar_t* section, const wchar_t* entry, const int      value);
	bool WriteInt64 (const wchar_t* section, const wchar_t* entry, const __int64  value);
	bool WriteDouble(const wchar_t* section, const wchar_t* entry, const double   value);
	bool WriteString(const wchar_t* section, const wchar_t* entry, const CStringW value);
	bool WriteBinary(const wchar_t* section, const wchar_t* entry, const BYTE* pdata, const unsigned nbytes);

	void EnumValueNames(const wchar_t* section, std::vector<CStringW>& valuenames);

	bool DeleteValue(const wchar_t* section, const wchar_t* entry);
	bool DeleteSection(const wchar_t* section);

	void Flush(bool bForce);
	void Clear();

	CStringW GetIniPath() const
	{
		return m_IniPath;
	}
	bool IsIniValid() const
	{
		return !!::PathFileExistsW(m_IniPath);
	}
};

class CModApp : public CWinApp
{
public:
	CProfile m_Profile;

//private: // TODO: make private.
	// Must override GetProfile and WriteProfile functions, because it uses MFC and other MFC dependent libraries.
	UINT GetProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int nDefault) override
	{
		int value;
		if (m_Profile.ReadInt(lpszSection, lpszEntry, value)) {
			return value;
		}
		return nDefault;
	}
	BOOL WriteProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int nValue) override
	{
		return m_Profile.WriteInt(lpszSection, lpszEntry, nValue);
	}
	CString GetProfileString(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszDefault = NULL) override
	{
		CString value;
		if (!m_Profile.ReadString(lpszSection, lpszEntry, value) && lpszDefault) {
			value.SetString(lpszDefault);
		}
		return value;
	}
	BOOL WriteProfileString(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszValue) override
	{
		if (!lpszEntry) {
			if (!lpszSection) {
				return m_Profile.DeleteSection(lpszSection);
			}
			return m_Profile.DeleteValue(lpszSection, lpszEntry);
		}
		return m_Profile.WriteString(lpszSection, lpszEntry, lpszValue);
	}
	BOOL GetProfileBinary(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPBYTE* ppData, UINT* pBytes) override
	{
		if (m_Profile.ReadBinary(lpszSection, lpszEntry, ppData, *pBytes)) {
			return TRUE;
		}
		return FALSE;
	}
	BOOL WriteProfileBinary(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPBYTE pData, UINT nBytes) override
	{
		return m_Profile.WriteBinary(lpszSection, lpszEntry, pData, nBytes);
	}
};

#define AfxGetProfile() static_cast<CModApp*>(AfxGetApp())->m_Profile

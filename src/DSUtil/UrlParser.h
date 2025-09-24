/*
 * (C) 2020-2025 see Authors.txt
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

#include <WinInet.h>

class CUrlParser
{
	CStringW m_szUrl;
	CStringW m_szSchemeName;
	CStringW m_szHostName;
	CStringW m_szUserName;
	CStringW m_szPassword;
	CStringW m_szUrlPath;
	CStringW m_szExtraInfo;

	INTERNET_PORT m_nPortNumber = 0;
	INTERNET_SCHEME m_nScheme = INTERNET_SCHEME_UNKNOWN;

public:
	CUrlParser() = default;
	CUrlParser(LPCWSTR lpszUrl);
	~CUrlParser() = default;

	CUrlParser(const CUrlParser&) = delete;
	CUrlParser& operator=(const CUrlParser&) = delete;

	void Clear();

	BOOL Parse(LPCWSTR lpszUrl);

	LPCWSTR GetUrl() const { return m_szUrl.GetString(); }
	int GetUrlLength() const { return m_szUrl.GetLength(); }
	LPCWSTR GetExtraInfo() const { return m_szExtraInfo.GetString(); }
	int GetExtraInfoLength() const { return m_szExtraInfo.GetLength(); }
	LPCWSTR GetHostName() const { return m_szHostName.GetString(); }
	int GetHostNameLength() const { return m_szHostName.GetLength(); }
	LPCWSTR GetPassword() const { return m_szPassword.GetString(); }
	int GetPasswordLength() const { return m_szPassword.GetLength(); }
	LPCWSTR GetSchemeName() const { return m_szSchemeName.GetString(); }
	int GetSchemeNameLength() const { return m_szSchemeName.GetLength(); }
	LPCWSTR GetUrlPath() const { return m_szUrlPath.GetString(); }
	int GetUrlPathLength() const { return m_szUrlPath.GetLength(); }
	LPCWSTR GetUserName() const { return m_szUserName.GetString(); }
	int GetUserNameLength() const { return m_szUserName.GetLength(); }

	INTERNET_PORT GetPortNumber() const { return m_nPortNumber; }
	INTERNET_SCHEME GetScheme() const { return m_nScheme; }

	BOOL IsValid() const { return !m_szUrl.IsEmpty(); }

	static CStringW CombineUrl(CStringW strBase, const CStringW& strRelative);
};

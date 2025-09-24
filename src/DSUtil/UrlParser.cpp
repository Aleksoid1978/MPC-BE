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

#include "stdafx.h"
#include "DSUtil.h"
#include "UrlParser.h"
#include <shlwapi.h>

#pragma comment(lib, "WinInet.Lib")
#pragma comment(lib, "Shlwapi.lib")

CUrlParser::CUrlParser(LPCWSTR lpszUrl)
{
	Parse(lpszUrl);
}

BOOL CUrlParser::Parse(LPCWSTR lpszUrl)
{
	Clear();
	CheckPointer(lpszUrl, FALSE);

	URL_COMPONENTSW components = { sizeof(components) };
	components.dwSchemeLength = (DWORD)-1;
	components.dwHostNameLength = (DWORD)-1;
	components.dwUserNameLength = (DWORD)-1;
	components.dwPasswordLength = (DWORD)-1;
	components.dwUrlPathLength = (DWORD)-1;
	components.dwExtraInfoLength = (DWORD)-1;

	const auto ret = InternetCrackUrlW(lpszUrl, wcslen(lpszUrl), 0, &components);
	if (ret && components.dwHostNameLength) {
		m_szUrl = lpszUrl;

		m_szSchemeName.SetString(components.lpszScheme, components.dwSchemeLength);
		m_szHostName.SetString(components.lpszHostName, components.dwHostNameLength);
		m_szUserName.SetString(components.lpszUserName, components.dwUserNameLength);
		m_szPassword.SetString(components.lpszPassword, components.dwPasswordLength);
		m_szUrlPath.SetString(components.lpszUrlPath, components.dwUrlPathLength);
		m_szExtraInfo.SetString(components.lpszExtraInfo, components.dwExtraInfoLength);

		m_nPortNumber = components.nPort ? components.nPort : 80;
		m_nScheme = components.nScheme;

		m_szSchemeName.MakeLower();

		if (m_szUrlPath.IsEmpty()) {
			m_szUrlPath = L"/";
		}
	} else {
		Clear();
	}

	return IsValid();
}

void CUrlParser::Clear()
{
	m_szUrl.Empty();

	m_szSchemeName.Empty();
	m_szHostName.Empty();
	m_szUserName.Empty();
	m_szPassword.Empty();
	m_szUrlPath.Empty();
	m_szExtraInfo.Empty();

	m_nPortNumber = 0;
	m_nScheme = INTERNET_SCHEME_UNKNOWN;
}

CStringW CUrlParser::CombineUrl(CStringW strBase, const CStringW& strRelative)
{
	if (strBase.GetAt(strBase.GetLength() - 1) != L'/') {
		strBase += L'/';
	}
	auto dwLength = static_cast<DWORD>(strBase.GetLength() + strRelative.GetLength() + 1);
	CStringW combined;
	auto hr = UrlCombineW(strBase.GetString(),
						  strRelative.GetString(),
						  combined.GetBuffer(dwLength),
						  &dwLength, 0);
	combined.ReleaseBuffer(-1);
	return combined;
}

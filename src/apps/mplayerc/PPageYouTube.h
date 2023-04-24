/*
 * (C) 2012-2023 see Authors.txt
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

#include "PPageBase.h"

// CPPageYoutube dialog

class CPPageYoutube : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageYoutube)

private:
	CButton m_chkPageParser;
	CComboBox m_cbFormat;
	CComboBox m_cbResolution;
	CButton m_chk60fps;
	CButton m_chkHdr;
	CComboBox m_cbAudioLang;
	CButton m_chkLoadPlaylist;

	CButton   m_chkYDLEnable;
	CComboBox m_cbYDLExePath;
	CComboBox m_cbYDLMaxHeight;
	CButton   m_chkYDLMaximumQuality;

	CEdit m_edAceStreamAddress;
	CEdit m_edTorrServerAddress;
	CEdit m_edUserAgent;

	static constexpr std::pair<LPCWSTR, LCID> m_langcodes[] = {
		{ L"en",    MAKELCID(MAKELANGID(LANG_ENGLISH,    SUBLANG_DEFAULT), SORT_DEFAULT) },
		{ L"ru",    MAKELCID(MAKELANGID(LANG_RUSSIAN,    SUBLANG_DEFAULT), SORT_DEFAULT) },
		{ L"ar",    MAKELCID(MAKELANGID(LANG_ARABIC,     SUBLANG_DEFAULT), SORT_DEFAULT) },
		{ L"de",    MAKELCID(MAKELANGID(LANG_GERMAN,     SUBLANG_DEFAULT), SORT_DEFAULT) },
		{ L"es",    MAKELCID(MAKELANGID(LANG_SPANISH,    SUBLANG_DEFAULT), SORT_DEFAULT) },
		{ L"fr",    MAKELCID(MAKELANGID(LANG_FRENCH,     SUBLANG_DEFAULT), SORT_DEFAULT) },
		{ L"hi",    MAKELCID(MAKELANGID(LANG_HINDI,      SUBLANG_DEFAULT), SORT_DEFAULT) },
		{ L"id",    MAKELCID(MAKELANGID(LANG_INDONESIAN, SUBLANG_DEFAULT), SORT_DEFAULT) },
		{ L"ko",    MAKELCID(MAKELANGID(LANG_KOREAN,     SUBLANG_DEFAULT), SORT_DEFAULT) },
		{ L"pt",    MAKELCID(MAKELANGID(LANG_PORTUGUESE, SUBLANG_DEFAULT), SORT_DEFAULT) },
		{ L"pt-BR", MAKELCID(MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE_BRAZILIAN), SORT_DEFAULT) },
		{ L"th",    MAKELCID(MAKELANGID(LANG_THAI,       SUBLANG_DEFAULT), SORT_DEFAULT) },
		{ L"tr",    MAKELCID(MAKELANGID(LANG_TURKISH,    SUBLANG_DEFAULT), SORT_DEFAULT) },
		{ L"vi",    MAKELCID(MAKELANGID(LANG_VIETNAMESE, SUBLANG_DEFAULT), SORT_DEFAULT) },
		{ L"it",    MAKELCID(MAKELANGID(LANG_ITALIAN,    SUBLANG_DEFAULT), SORT_DEFAULT) },
		{ L"ja",    MAKELCID(MAKELANGID(LANG_JAPANESE,   SUBLANG_DEFAULT), SORT_DEFAULT) },
		{ L"zh",    MAKELCID(MAKELANGID(LANG_CHINESE,    SUBLANG_DEFAULT), SORT_DEFAULT) }
	};

public:
	CPPageYoutube();
	virtual ~CPPageYoutube();

	enum { IDD = IDD_PPAGEYOUTUBE };

	afx_msg void OnCheckPageParser();
	afx_msg void OnCheck60fps();
	afx_msg void OnCheckYDLEnable();

	static CStringW GetDefaultLanguageCode();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()
};

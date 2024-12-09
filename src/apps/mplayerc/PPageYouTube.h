/*
 * (C) 2012-2024 see Authors.txt
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
	CComboBox m_cbVideoFormat;
	CComboBox m_cbResolution;
	CButton m_chk60fps;
	CButton m_chkHdr;
	CComboBox m_cbAudioFormat;
	CComboBox m_cbAudioLang;
	CButton m_chkLoadPlaylist;

	CButton   m_chkYDLEnable;
	CComboBox m_cbYDLExePath;
	CComboBox m_cbYDLMaxHeight;
	CButton   m_chkYDLMaximumQuality;

	CEdit m_edAceStreamAddress;
	CEdit m_edTorrServerAddress;
	CEdit m_edUserAgent;

	static constexpr LPCWSTR m_langcodes[] = {
		L"en",
		L"ru",
		L"ar",
		L"de",
		L"es",
		L"fr",
		L"hi",
		L"id",
		L"ko",
		L"pt",
		L"pt-BR",
		L"th",
		L"tr",
		L"vi",
		L"it",
		L"ja",
		L"zh",
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

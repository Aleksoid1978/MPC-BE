/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2016 see Authors.txt
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
#include "PPageYoutube.h"

// CPPageYoutube dialog

IMPLEMENT_DYNAMIC(CPPageYoutube, CPPageBase)
CPPageYoutube::CPPageYoutube()
	: CPPageBase(CPPageYoutube::IDD, CPPageYoutube::IDD)
{
}

CPPageYoutube::~CPPageYoutube()
{
}

void CPPageYoutube::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_CHECK2, m_chkPageParser);
	DDX_Control(pDX, IDC_COMBO1, m_cbPreferredFormat);
	DDX_Control(pDX, IDC_CHECK1, m_chkLoadPlaylist);
}

BEGIN_MESSAGE_MAP(CPPageYoutube, CPPageBase)
	ON_COMMAND(IDC_CHECK2, OnCheckPageParser)
END_MESSAGE_MAP()

// CPPageYoutube message handlers

BOOL CPPageYoutube::OnInitDialog()
{
	__super::OnInitDialog();

	SetCursor(m_hWnd, IDC_COMBO1, IDC_HAND);

	CAppSettings& s = AfxGetAppSettings();

	m_chkPageParser.SetCheck(s.bYoutubePageParser);

	m_cbPreferredFormat.Clear();

	for (size_t i = 0; i < _countof(YoutubeParser::youtubeVideoProfiles); i++) {
		const CString fmt = YoutubeParser::FormatProfiles(YoutubeParser::youtubeVideoProfiles[i]);
		if (!fmt.IsEmpty()) {
			m_cbPreferredFormat.AddString(fmt);
			m_cbPreferredFormat.SetItemData(i, YoutubeParser::youtubeVideoProfiles[i].iTag);
		}
	}

	int j = 0;
	for (j = 0; j < m_cbPreferredFormat.GetCount(); j++) {
		if (m_cbPreferredFormat.GetItemData(j) == s.iYoutubeTag) {
			m_cbPreferredFormat.SetCurSel(j);
			break;
		}
	}
	if (j >= m_cbPreferredFormat.GetCount()) {
		m_cbPreferredFormat.SetCurSel(0);
	}

	m_chkLoadPlaylist.SetCheck(s.bYoutubeLoadPlaylist);

	CorrectCWndWidth(GetDlgItem(IDC_CHECK2));

	OnCheckPageParser();

	UpdateData(FALSE);

	return TRUE;
}

BOOL CPPageYoutube::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	s.bYoutubePageParser	= !!m_chkPageParser.GetCheck();
	s.iYoutubeTag			= GetCurItemData(m_cbPreferredFormat);
	s.bYoutubeLoadPlaylist	= !!m_chkLoadPlaylist.GetCheck();

	return __super::OnApply();
}

void CPPageYoutube::OnCheckPageParser()
{
	if (m_chkPageParser.GetCheck()) {
		GetDlgItem(IDC_STATIC2)->EnableWindow(TRUE);
		m_cbPreferredFormat.EnableWindow(TRUE);
	} else {
		GetDlgItem(IDC_STATIC2)->EnableWindow(FALSE);
		m_cbPreferredFormat.EnableWindow(FALSE);
	}

	SetModified();
}


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
#include <algorithm>
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

	const CAppSettings& s = AfxGetAppSettings();

	m_chkPageParser.SetCheck(s.bYoutubePageParser);

	m_cbPreferredFormat.Clear();

	{
		LOGFONT lf;
		memset(&lf, 0, sizeof(lf));
		lf.lfPitchAndFamily = DEFAULT_PITCH | FF_MODERN;
		CDC* cDC = m_cbPreferredFormat.GetDC();
		lf.lfHeight = -MulDiv(8, cDC->GetDeviceCaps(LOGPIXELSY), 72);
		wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Courier New");
		m_MonoFont.CreateFontIndirect(&lf);
		m_cbPreferredFormat.SetFont(&m_MonoFont);
	}

	auto getSorted = [] () {
		std::vector<YoutubeParser::YoutubeProfiles> profiles;
		profiles.assign(YoutubeParser::youtubeVideoProfiles, YoutubeParser::youtubeVideoProfiles + _countof(YoutubeParser::youtubeVideoProfiles) - 1);
		std::sort(profiles.begin(), profiles.end(), [](const YoutubeParser::YoutubeProfiles a, const YoutubeParser::YoutubeProfiles b) {
			return a.priority > b.priority && a.quality >= b.quality;
		});

		return profiles;
	};

	static std::vector<YoutubeParser::YoutubeProfiles> profiles = getSorted();
	int i = 0;
	for(auto it = profiles.begin(); it != profiles.end(); ++it) {
		const CString fmt = YoutubeParser::FormatProfiles(*it);
		if (!fmt.IsEmpty()) {
			m_cbPreferredFormat.AddString(fmt);
			m_cbPreferredFormat.SetItemData(i++, it->iTag);
		}
	}

	i = 0;
	for (i = 0; i < m_cbPreferredFormat.GetCount(); i++) {
		if (m_cbPreferredFormat.GetItemData(i) == s.iYoutubeTag) {
			m_cbPreferredFormat.SetCurSel(i);
			break;
		}
	}
	if (i >= m_cbPreferredFormat.GetCount()) {
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

	s.iYoutubeTag          = GetCurItemData(m_cbPreferredFormat);
	s.bYoutubePageParser   = !!m_chkPageParser.GetCheck();
	s.bYoutubeLoadPlaylist = !!m_chkLoadPlaylist.GetCheck();

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


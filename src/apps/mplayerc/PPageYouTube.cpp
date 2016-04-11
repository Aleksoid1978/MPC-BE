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
		CString fmt;
		CString fps;
		switch (YoutubeParser::youtubeVideoProfiles[i].type) {
			case YoutubeParser::ytype::y_mp4:
				fmt = L"MP4";
				break;
			case YoutubeParser::ytype::y_webm:
				fmt = L"WebM";
				break;
			case YoutubeParser::ytype::y_flv:
				fmt = L"FLV";
				break;
			case YoutubeParser::ytype::y_3gp:
				fmt = L"3GP";
				break;
#if ENABLE_YOUTUBE_3D
			case YoutubeParser::ytype::y_3d_mp4:
				fmt = L"3D MP4";
				break;
			case YoutubeParser::ytype::y_3d_webm:
				fmt = L"3D WebM";
				break;
#endif
			case YoutubeParser::ytype::y_webm_video:
				fmt = L"WebM";
				break;
			case YoutubeParser::ytype::y_webm_video_60fps:
				fmt = L"WebM";
				fps = L"60";
				break;
#if ENABLE_YOUTUBE_DASH
			case YoutubeParser::ytype::y_dash_mp4_video:
				fmt = L"DASH MP4";
				break;
#endif
		default:
			continue;
		}
		fmt.AppendFormat(_T("@%dp"), YoutubeParser::youtubeVideoProfiles[i].quality);
		if (!fps.IsEmpty()) {
			fmt.AppendFormat(_T("%s"), fps);
		}

		m_cbPreferredFormat.AddString(fmt);
		m_cbPreferredFormat.SetItemData(i, YoutubeParser::youtubeVideoProfiles[i].iTag);
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


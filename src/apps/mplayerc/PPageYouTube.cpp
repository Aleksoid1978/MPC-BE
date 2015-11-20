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
#include "PPageYoutube.h"

// CPPageYoutube dialog

IMPLEMENT_DYNAMIC(CPPageYoutube, CPPageBase)
CPPageYoutube::CPPageYoutube()
	: CPPageBase(CPPageYoutube::IDD, CPPageYoutube::IDD)
	, m_iYoutubeSourceType(0)
{
}

CPPageYoutube::~CPPageYoutube()
{
}

void CPPageYoutube::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_COMBO1, m_iYoutubeFormatCtrl);
	DDX_Control(pDX, IDC_CHECK1, m_chkYoutubeLoadPlaylist);
	DDX_Radio(pDX, IDC_RADIO1, m_iYoutubeSourceType);
	DDX_Radio(pDX, IDC_RADIO3, m_iYoutubeMemoryType);
	DDX_Control(pDX, IDC_SPIN1, m_nPercentMemoryCtrl);
	DDX_Control(pDX, IDC_SPIN2, m_nMbMemoryCtrl);
	DDX_Text(pDX, IDC_EDIT1, m_iYoutubePercentMemory);
	DDX_Text(pDX, IDC_EDIT2, m_iYoutubeMbMemory);

	m_iYoutubePercentMemory = min(max(1, m_iYoutubePercentMemory), 100);
	m_iYoutubeMbMemory = min(max(1, m_iYoutubeMbMemory), 128);
}

BEGIN_MESSAGE_MAP(CPPageYoutube, CPPageBase)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_RADIO1, IDC_RADIO2, OnBnClickedRadio12)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_RADIO3, IDC_RADIO4, OnBnClickedRadio34)
END_MESSAGE_MAP()

// CPPageYoutube message handlers

BOOL CPPageYoutube::OnInitDialog()
{
	__super::OnInitDialog();

	SetCursor(m_hWnd, IDC_COMBO1, IDC_HAND);

	CAppSettings& s = AfxGetAppSettings();

	m_iYoutubeFormatCtrl.Clear();

	for (size_t i = 0; i < _countof(youtubeVideoProfiles); i++) {
		CString fmt;
		switch (youtubeVideoProfiles[i].type) {
		case y_mp4:
			fmt = L"MP4";
			break;
		case y_webm:
			fmt = L"WebM";
			break;
		case y_flv:
			fmt = L"FLV";
			break;
		case y_3gp:
			fmt = L"3GP";
			break;
#if ENABLE_YOUTUBE_3D
		case y_3d_mp4:
			fmt = L"3D MP4";
			break;
		case y_3d_webm:
			fmt = L"3D WebM";
			break;
#endif
		case y_webm_video:
			fmt = L"WebM(video)";
			break;
		case y_webm_video_60fps:
			fmt = L"WebM(video) 60fps";
			break;
#if ENABLE_YOUTUBE_DASH
		case y_dash_mp4_video:
			fmt = L"DASH MP4";
			break;
#endif
		default:
			continue;
		}
		fmt.AppendFormat(_T("@%dp"), youtubeVideoProfiles[i].quality);

		m_iYoutubeFormatCtrl.AddString(fmt);
		m_iYoutubeFormatCtrl.SetItemData(i, youtubeVideoProfiles[i].iTag);
	}

	int j = 0;
	for (j = 0; j < m_iYoutubeFormatCtrl.GetCount(); j++) {
		if (m_iYoutubeFormatCtrl.GetItemData(j) == s.iYoutubeTag) {
			m_iYoutubeFormatCtrl.SetCurSel(j);
			break;
		}
	}
	if (j >= m_iYoutubeFormatCtrl.GetCount()) {
		m_iYoutubeFormatCtrl.SetCurSel(0);
	}

	m_chkYoutubeLoadPlaylist.SetCheck(s.bYoutubeLoadPlaylist);

	m_nPercentMemoryCtrl.SetRange(1, 100);
	m_nMbMemoryCtrl.SetRange(1, 128);

	m_iYoutubeSourceType	= s.iYoutubeSource ? 1 : 0;
	m_iYoutubeMemoryType	= s.iYoutubeMemoryType ? 1 : 0;
	m_iYoutubePercentMemory	= s.iYoutubePercentMemory;
	m_iYoutubeMbMemory		= s.iYoutubeMbMemory;

	UpdateData(FALSE);

	UpdateMemoryCtrl();

	return TRUE;
}

BOOL CPPageYoutube::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	s.iYoutubeTag			= m_iYoutubeFormatCtrl.GetItemData(m_iYoutubeFormatCtrl.GetCurSel());
	s.bYoutubeLoadPlaylist	= !!m_chkYoutubeLoadPlaylist.GetCheck();
	s.iYoutubeSource		= m_iYoutubeSourceType;
	s.iYoutubeMemoryType	= m_iYoutubeMemoryType;
	s.iYoutubePercentMemory	= m_iYoutubePercentMemory;
	s.iYoutubeMbMemory		= m_iYoutubeMbMemory;

	return __super::OnApply();
}

void CPPageYoutube::OnBnClickedRadio12(UINT nID)
{
	SetModified();
	UpdateMemoryCtrl();
}

void CPPageYoutube::OnBnClickedRadio34(UINT nID)
{
	SetModified();
}

void CPPageYoutube::UpdateMemoryCtrl()
{
	switch (GetCheckedRadioButton(IDC_RADIO1, IDC_RADIO2)) {
		case IDC_RADIO1: {
			GetDlgItem(IDC_STATIC3)->ShowWindow(TRUE);
			GetDlgItem(IDC_RADIO3)->ShowWindow(TRUE);
			GetDlgItem(IDC_RADIO4)->ShowWindow(TRUE);
			GetDlgItem(IDC_EDIT1)->ShowWindow(TRUE);
			GetDlgItem(IDC_EDIT2)->ShowWindow(TRUE);
			GetDlgItem(IDC_SPIN1)->ShowWindow(TRUE);
			GetDlgItem(IDC_SPIN2)->ShowWindow(TRUE);
		}
		break;
		case IDC_RADIO2: {
			GetDlgItem(IDC_STATIC3)->ShowWindow(FALSE);
			GetDlgItem(IDC_RADIO3)->ShowWindow(FALSE);
			GetDlgItem(IDC_RADIO4)->ShowWindow(FALSE);
			GetDlgItem(IDC_EDIT1)->ShowWindow(FALSE);
			GetDlgItem(IDC_EDIT2)->ShowWindow(FALSE);
			GetDlgItem(IDC_SPIN1)->ShowWindow(FALSE);
			GetDlgItem(IDC_SPIN2)->ShowWindow(FALSE);
		}
		break;
	}
}

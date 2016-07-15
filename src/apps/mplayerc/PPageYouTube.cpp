/*
 * (C) 2012-2016 see Authors.txt
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
	DDX_Control(pDX, IDC_COMBO1, m_cbFormat);
	DDX_Control(pDX, IDC_COMBO2, m_cbResolution);
	DDX_Control(pDX, IDC_CHECK3, m_chk60fps);
	DDX_Control(pDX, IDC_CHECK1, m_chkLoadPlaylist);
}

BEGIN_MESSAGE_MAP(CPPageYoutube, CPPageBase)
	ON_COMMAND(IDC_CHECK2, OnCheckPageParser)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnFormatSelChange)
END_MESSAGE_MAP()

// CPPageYoutube message handlers

BOOL CPPageYoutube::OnInitDialog()
{
	__super::OnInitDialog();

	SetCursor(m_hWnd, IDC_COMBO1, IDC_HAND);

	const CAppSettings& s = AfxGetAppSettings();

	m_chkPageParser.SetCheck(s.bYoutubePageParser);

	m_cbFormat.AddString(L"MP4");
	m_cbFormat.AddString(L"WebM");
	m_cbFormat.SetCurSel(s.YoutubeFormat.fmt);

	int resolutions[] = { 2160, 1440, 1080, 720, 480, 360 };
	for (int i = 0; i < _countof(resolutions); i++) {
		CString str;
		str.Format(L"%dp", resolutions[i]);
		m_cbResolution.AddString(str);
		m_cbResolution.SetItemData(i, resolutions[i]);
	}
	SelectByItemData(m_cbResolution, s.YoutubeFormat.res);

	m_chk60fps.SetCheck(s.YoutubeFormat.fps60 ? BST_CHECKED : BST_UNCHECKED);

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
	s.YoutubeFormat.fmt		= m_cbFormat.GetCurSel();
	s.YoutubeFormat.res		= GetCurItemData(m_cbResolution);
	s.YoutubeFormat.fps60	= !!m_chk60fps.GetCheck();
	s.bYoutubeLoadPlaylist	= !!m_chkLoadPlaylist.GetCheck();

	return __super::OnApply();
}

void CPPageYoutube::OnCheckPageParser()
{
	if (m_chkPageParser.GetCheck()) {
		GetDlgItem(IDC_STATIC2)->EnableWindow(TRUE);
		m_cbFormat.EnableWindow(TRUE);
	} else {
		GetDlgItem(IDC_STATIC2)->EnableWindow(FALSE);
		m_cbFormat.EnableWindow(FALSE);
	}

	SetModified();
}

void CPPageYoutube::OnFormatSelChange()
{
	if (m_cbFormat.GetCurSel() == 0) {
		
	}
	else if(m_cbFormat.GetCurSel() == 0) {
		
	}

	SetModified();
}

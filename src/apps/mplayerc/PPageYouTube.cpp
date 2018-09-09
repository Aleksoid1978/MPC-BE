/*
 * (C) 2012-2018 see Authors.txt
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
#include "PlayerYouTube.h"

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
	DDX_Control(pDX, IDC_CHECK4, m_chkHdr);
	DDX_Control(pDX, IDC_CHECK1, m_chkLoadPlaylist);
	DDX_Control(pDX, IDC_COMBO3, m_cbYDLMaxHeight);
}

BEGIN_MESSAGE_MAP(CPPageYoutube, CPPageBase)
	ON_COMMAND(IDC_CHECK2, OnCheckPageParser)
	ON_COMMAND(IDC_CHECK3, OnCheck60fps)
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

	static std::vector<int> resolutions;
	if (resolutions.empty()) {
		for (const auto& profile : Youtube::YProfiles) {
			resolutions.emplace_back(profile.quality);
		}
		// sort
		std::sort(resolutions.begin(), resolutions.end(), std::greater<int>());
		// deduplicate
		resolutions.erase(std::unique(resolutions.begin(), resolutions.end()), resolutions.end());
	}
	for (const auto& res : resolutions) {
		CString str;
		str.Format(L"%dp", res);
		AddStringData(m_cbResolution, str, res);
	}
	SelectByItemData(m_cbResolution, s.YoutubeFormat.res);

	m_chk60fps.SetCheck(s.YoutubeFormat.fps60 ? BST_CHECKED : BST_UNCHECKED);
	m_chkHdr.SetCheck(s.YoutubeFormat.hdr ? BST_CHECKED : BST_UNCHECKED);

	m_chkLoadPlaylist.SetCheck(s.bYoutubeLoadPlaylist);

	CorrectCWndWidth(GetDlgItem(IDC_CHECK2));

	OnCheck60fps();
	OnCheckPageParser();

	for (const auto& h : s_CommonVideoHeights) {
		CString str;
		str.Format(L"%d", h);
		AddStringData(m_cbYDLMaxHeight, str, h);
	}
	SelectByItemData(m_cbYDLMaxHeight, s.iYDLMaxHeight);

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
	s.YoutubeFormat.hdr		= !!m_chkHdr.GetCheck();
	s.bYoutubeLoadPlaylist	= !!m_chkLoadPlaylist.GetCheck();

	s.iYDLMaxHeight = GetCurItemData(m_cbYDLMaxHeight);

	return __super::OnApply();
}

void CPPageYoutube::OnCheckPageParser()
{
	const BOOL bEnable = m_chkPageParser.GetCheck();
	GetDlgItem(IDC_STATIC2)->EnableWindow(bEnable);
	m_cbFormat.EnableWindow(bEnable);
	m_cbResolution.EnableWindow(bEnable);
	m_chk60fps.EnableWindow(bEnable);
	m_chkHdr.EnableWindow(bEnable);

	if (bEnable) {
		OnCheck60fps();
	}

	SetModified();
}

void CPPageYoutube::OnCheck60fps()
{
	if (m_chk60fps.GetCheck()) {
		m_chkHdr.EnableWindow(TRUE);
	} else {
		m_chkHdr.SetCheck(BST_UNCHECKED);
		m_chkHdr.EnableWindow(FALSE);
	}

	SetModified();
}

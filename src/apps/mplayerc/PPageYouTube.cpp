/*
 * (C) 2012-2025 see Authors.txt
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
#include "DSUtil/Filehandle.h"
#include "DSUtil/HTTPAsync.h"

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
	DDX_Control(pDX, IDC_COMBO1, m_cbVideoFormat);
	DDX_Control(pDX, IDC_COMBO2, m_cbResolution);
	DDX_Control(pDX, IDC_CHECK3, m_chk60fps);
	DDX_Control(pDX, IDC_CHECK4, m_chkHdr);
	DDX_Control(pDX, IDC_COMBO3, m_cbAudioFormat);
	DDX_Control(pDX, IDC_COMBO4, m_cbAudioLang);
	DDX_Control(pDX, IDC_CHECK1, m_chkLoadPlaylist);
	DDX_Control(pDX, IDC_CHECK6, m_chkYDLEnable);
	DDX_Control(pDX, IDC_COMBO5, m_cbYDLExePath);
	DDX_Control(pDX, IDC_COMBO6, m_cbYDLMaxHeight);
	DDX_Control(pDX, IDC_CHECK5, m_chkYDLMaximumQuality);
	DDX_Control(pDX, IDC_EDIT1,  m_edAceStreamAddress);
	DDX_Control(pDX, IDC_EDIT2,  m_edTorrServerAddress);
	DDX_Control(pDX, IDC_EDIT3,  m_edUserAgent);
}

BEGIN_MESSAGE_MAP(CPPageYoutube, CPPageBase)
	ON_COMMAND(IDC_CHECK2, OnCheckPageParser)
	ON_COMMAND(IDC_CHECK3, OnCheck60fps)
	ON_COMMAND(IDC_CHECK6, OnCheckYDLEnable)
END_MESSAGE_MAP()

// CPPageYoutube message handlers

BOOL CPPageYoutube::OnInitDialog()
{
	__super::OnInitDialog();

	SetCursor(m_hWnd, IDC_COMBO1, IDC_HAND);
	CorrectCWndWidth(&m_chkYDLEnable);

	const CAppSettings& s = AfxGetAppSettings();

	m_chkPageParser.SetCheck(s.bYoutubePageParser);

	m_cbVideoFormat.AddString(L"MP4-H.264");
	m_cbVideoFormat.AddString(L"WebM-VP9");
	m_cbVideoFormat.AddString(L"MP4-AV1");
	m_cbVideoFormat.SetCurSel(s.YoutubeFormat.vfmt);

	static std::vector<int> resolutions;
	if (resolutions.empty()) {
		resolutions.reserve(std::size(Youtube::YProfiles));
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
	AddStringData(m_cbResolution, ResStr(IDS_AUDIO_ONLY), 0);

	SelectByItemData(m_cbResolution, s.YoutubeFormat.res);

	m_chk60fps.SetCheck(s.YoutubeFormat.fps60 ? BST_CHECKED : BST_UNCHECKED);
	m_chkHdr.SetCheck(s.YoutubeFormat.hdr ? BST_CHECKED : BST_UNCHECKED);

	m_cbAudioFormat.AddString(L"AAC");
	m_cbAudioFormat.AddString(L"Opus");
	m_cbAudioFormat.SetCurSel(s.YoutubeFormat.afmt - Youtube::y_mp4_aac);

	static std::vector<CStringW> langNames;
	if (langNames.empty()) {
		constexpr auto size = 256;
		wchar_t buffer[size] = {};

		langNames.resize(std::size(m_langcodes));
		for (size_t i = 0; i < std::size(m_langcodes); i++) {
			if (GetLocaleInfoEx(m_langcodes[i], LOCALE_SLOCALIZEDDISPLAYNAME, buffer, size)) {
				langNames[i] = buffer;
			} else {
				langNames[i] = m_langcodes[i];
			}
		}
	}

	bool was_added = false;
	m_cbAudioLang.AddString(ResStr(IDS_YOUTUBE_DEFAULT_AUDIO_LANG));
	if (s.strYoutubeAudioLang.CompareNoCase(Youtube::kDefaultAudioLanguage) == 0) {
		was_added = true;
		m_cbAudioLang.SetCurSel(0);
	}

	for (size_t i = 0; i < std::size(m_langcodes); i++) {
		m_cbAudioLang.AddString(langNames[i].GetString());
		if (!was_added && s.strYoutubeAudioLang.CompareNoCase(m_langcodes[i]) == 0) {
			was_added = true;
			m_cbAudioLang.SetCurSel(i + 1);
		}
	}
	if (!was_added) {
		m_cbAudioLang.SetCurSel(0);
	}

	m_chkLoadPlaylist.SetCheck(s.bYoutubeLoadPlaylist);

	CorrectCWndWidth(GetDlgItem(IDC_CHECK2));

	OnCheck60fps();
	OnCheckPageParser();

	m_chkYDLEnable.SetCheck(s.bYDLEnable ? BST_CHECKED : BST_UNCHECKED);

	was_added = false;
	LPCWSTR ydl_filenames[] = {
			L"yt-dlp.exe",
			L"yt-dlp_min.exe",
			L"youtube-dl.exe"
	};
	for (auto& ydl_filename : ydl_filenames) {
		m_cbYDLExePath.AddString(ydl_filename);
		if (s.strYDLExePath.CompareNoCase(ydl_filename) == 0) {
			was_added = true;
		}
	}
	if (!was_added) {
		m_cbYDLExePath.AddString(s.strYDLExePath);
	}
	m_cbYDLExePath.SelectString(0, s.strYDLExePath);

	for (const auto& h : s_CommonVideoHeights) {
		if (h == 0) {
			continue; // TODO
		}
		CString str;
		str.Format(L"%d", h);
		AddStringData(m_cbYDLMaxHeight, str, h);
	}
	SelectByItemData(m_cbYDLMaxHeight, s.iYDLMaxHeight);

	m_chkYDLMaximumQuality.SetCheck(s.bYDLMaximumQuality ? BST_CHECKED : BST_UNCHECKED);

	m_edAceStreamAddress.SetWindowTextW(s.strAceStreamAddress);
	m_edTorrServerAddress.SetWindowTextW(s.strTorrServerAddress);
	m_edUserAgent.SetWindowTextW(s.strUserAgent);

	OnCheckYDLEnable();

	UpdateData(FALSE);

	return TRUE;
}

BOOL CPPageYoutube::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	s.bYoutubePageParser	= !!m_chkPageParser.GetCheck();
	s.YoutubeFormat.vfmt	= m_cbVideoFormat.GetCurSel();
	s.YoutubeFormat.res		= GetCurItemData(m_cbResolution);
	s.YoutubeFormat.fps60	= !!m_chk60fps.GetCheck();
	s.YoutubeFormat.hdr		= !!m_chkHdr.GetCheck();
	s.YoutubeFormat.afmt	= m_cbAudioFormat.GetCurSel() + Youtube::y_mp4_aac;
	if (m_cbAudioLang.GetCurSel() >= 0) {
		s.strYoutubeAudioLang = m_cbAudioLang.GetCurSel() == 0 ? Youtube::kDefaultAudioLanguage : m_langcodes[m_cbAudioLang.GetCurSel() - 1];
	} else {
		s.strYoutubeAudioLang.Empty();
	}

	s.bYoutubeLoadPlaylist	= !!m_chkLoadPlaylist.GetCheck();

	s.bYDLEnable = !!m_chkYDLEnable.GetCheck();
	m_cbYDLExePath.GetWindowTextW(s.strYDLExePath);
	CleanPath(s.strYDLExePath);
	s.iYDLMaxHeight = GetCurItemData(m_cbYDLMaxHeight);
	s.bYDLMaximumQuality = !!m_chkYDLMaximumQuality.GetCheck();

	CString str;
	m_edAceStreamAddress.GetWindowTextW(str);
	if (::PathIsURLW(str)) {
		s.strAceStreamAddress = str;
	}

	str.Empty();
	m_edTorrServerAddress.GetWindowTextW(str);
	if (::PathIsURLW(str)) {
		s.strTorrServerAddress = str;
	}

	m_edUserAgent.GetWindowTextW(s.strUserAgent);
	http::userAgent = s.strUserAgent;

	return __super::OnApply();
}

void CPPageYoutube::OnCheckPageParser()
{
	const BOOL bEnable = m_chkPageParser.GetCheck();
	GetDlgItem(IDC_STATIC2)->EnableWindow(bEnable);
	m_cbVideoFormat.EnableWindow(bEnable);
	m_cbResolution.EnableWindow(bEnable);
	m_chk60fps.EnableWindow(bEnable);
	m_chkHdr.EnableWindow(bEnable);

	if (bEnable) {
		OnCheck60fps();
	}
	GetDlgItem(IDC_STATIC3)->EnableWindow(bEnable);
	m_cbAudioFormat.EnableWindow(bEnable);
	GetDlgItem(IDC_STATIC4)->EnableWindow(bEnable);
	m_cbAudioLang.EnableWindow(bEnable);

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

void CPPageYoutube::OnCheckYDLEnable()
{
	if (m_chkYDLEnable.GetCheck()) {
		m_cbYDLMaxHeight.EnableWindow(TRUE);
		m_chkYDLMaximumQuality.EnableWindow(TRUE);
	} else {
		m_cbYDLMaxHeight.EnableWindow(FALSE);
		m_chkYDLMaximumQuality.EnableWindow(FALSE);
	}

	SetModified();
}

CStringW CPPageYoutube::GetDefaultLanguageCode()
{
	auto defaultUIlcid = MAKELCID(GetUserDefaultUILanguage(), SORT_DEFAULT);
	for (auto code : m_langcodes) {
		auto lcid = LocaleNameToLCID(code, 0);
		if (lcid == defaultUIlcid) {
			return code;
		}
	}

	// default language
	return L"en";
}

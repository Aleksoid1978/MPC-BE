/*
 * (C) 2012-2026 see Authors.txt
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
#include "PlayerYtDlp.h"
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

void CPPageYoutube::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_CHECK1, m_chkYDLEnable);
	DDX_Control(pDX, IDC_COMBO5, m_cbYDLExePath);

	DDX_Control(pDX, IDC_COMBO1, m_cbVideoCodec);
	DDX_Control(pDX, IDC_COMBO2, m_cbMaxHeight);
	DDX_Control(pDX, IDC_CHECK3, m_chkHighFps);
	DDX_Control(pDX, IDC_CHECK4, m_chkHdr);
	DDX_Control(pDX, IDC_COMBO3, m_cbAudioCodec);
	DDX_Control(pDX, IDC_COMBO4, m_cbAudioLang);
	DDX_Control(pDX, IDC_CHECK5, m_chkHighBitrate);
	DDX_Control(pDX, IDC_CHECK6, m_chkLoadPlaylist);
	DDX_Control(pDX, IDC_CHECK7, m_chkIntYtPlaylistParser);

	DDX_Control(pDX, IDC_EDIT1,  m_edAceStreamAddress);
	DDX_Control(pDX, IDC_EDIT2,  m_edTorrServerAddress);
	DDX_Control(pDX, IDC_EDIT3,  m_edUserAgent);
}

BEGIN_MESSAGE_MAP(CPPageYoutube, CPPageBase)
	ON_COMMAND(IDC_CHECK3, OnCheck60fps)
	ON_COMMAND(IDC_CHECK6, OnCheckLoadPlaylist)
	ON_COMMAND(IDC_CHECK1, OnCheckYDLEnable)
	ON_BN_CLICKED(IDC_BUTTON3, OnBnClickedDefault)
END_MESSAGE_MAP()

// CPPageYoutube message handlers

BOOL CPPageYoutube::OnInitDialog()
{
	__super::OnInitDialog();

	SetCursor(m_hWnd, IDC_COMBO1, IDC_HAND);
	CorrectCWndWidth(&m_chkYDLEnable);

	const CAppSettings& s = AfxGetAppSettings();

	m_cbVideoCodec.AddString(L"H.264");
	m_cbVideoCodec.AddString(L"VP9");
	m_cbVideoCodec.AddString(L"AV1");
	m_cbVideoCodec.SetCurSel(s.iYdlVcodec);

	for (const auto& h : s_CommonVideoHeights) {
		CString str;
		if (h == 0) {
			str = ResStr(IDS_AUDIO_ONLY);
		} else {
			str.Format(L"%d", h);
		}
		AddStringData(m_cbMaxHeight, str, h);
	}
	SelectByItemData(m_cbMaxHeight, s.iYdlMaxHeight);

	m_chkHighFps.SetCheck(s.bYdlHighFps ? BST_CHECKED : BST_UNCHECKED);
	m_chkHdr.SetCheck(s.bYdlHDR ? BST_CHECKED : BST_UNCHECKED);

	m_cbAudioCodec.AddString(L"AAC");
	m_cbAudioCodec.AddString(L"Opus");
	m_cbAudioCodec.SetCurSel(s.iYdlAcodec);

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
	if (s.strYdlAudioLang.CompareNoCase(L"default") == 0) {
		was_added = true;
		m_cbAudioLang.SetCurSel(0);
	}

	for (size_t i = 0; i < std::size(m_langcodes); i++) {
		m_cbAudioLang.AddString(langNames[i].GetString());
		if (!was_added && s.strYdlAudioLang.CompareNoCase(m_langcodes[i]) == 0) {
			was_added = true;
			m_cbAudioLang.SetCurSel(i + 1);
		}
	}
	if (!was_added) {
		m_cbAudioLang.SetCurSel(0);
	}

	m_chkLoadPlaylist.SetCheck(s.bYdlLoadPlaylist);
	m_chkIntYtPlaylistParser.SetCheck(s.bYoutubePlaylistParser);

	CorrectCWndWidth(GetDlgItem(IDC_CHECK1));

	OnCheck60fps();

	m_chkYDLEnable.SetCheck(s.bYdlEnable ? BST_CHECKED : BST_UNCHECKED);

	was_added = false;
	LPCWSTR ydl_filenames[] = {
		DEFAULT_YTDLP_EXE,
	};
	for (auto& ydl_filename : ydl_filenames) {
		m_cbYDLExePath.AddString(ydl_filename);
		if (s.strYdlExePath.CompareNoCase(ydl_filename) == 0) {
			was_added = true;
		}
	}
	if (!was_added) {
		m_cbYDLExePath.AddString(s.strYdlExePath);
	}
	m_cbYDLExePath.SelectString(0, s.strYdlExePath);

	m_chkHighBitrate.SetCheck(s.bYdlHighBitrate ? BST_CHECKED : BST_UNCHECKED);

	m_edAceStreamAddress.SetWindowTextW(s.strAceStreamAddress);
	m_edTorrServerAddress.SetWindowTextW(s.strTorrServerAddress);
	m_edUserAgent.SetWindowTextW(s.strUserAgent);

	OnCheckYDLEnable();

#if 1 // TODO yt-dlp
	GetDlgItem(IDC_STATIC4)->ShowWindow(SW_HIDE);
	m_cbAudioLang.ShowWindow(SW_HIDE);
#endif

	UpdateData(FALSE);

	return TRUE;
}

BOOL CPPageYoutube::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	s.bYdlEnable = !!m_chkYDLEnable.GetCheck();
	m_cbYDLExePath.GetWindowTextW(s.strYdlExePath);

	s.iYdlVcodec            = m_cbVideoCodec.GetCurSel();
	s.iYdlMaxHeight         = GetCurItemData(m_cbMaxHeight);
	s.bYdlHighFps           = !!m_chkHighFps.GetCheck();
	s.bYdlHDR               = !!m_chkHdr.GetCheck();
	s.iYdlAcodec            = m_cbAudioCodec.GetCurSel();
	if (m_cbAudioLang.GetCurSel() >= 0) {
		s.strYdlAudioLang = m_cbAudioLang.GetCurSel() == 0 ? L"default" : m_langcodes[m_cbAudioLang.GetCurSel() - 1];
	} else {
		s.strYdlAudioLang.Empty();
	}
	s.bYdlHighBitrate       = !!m_chkHighBitrate.GetCheck();
	s.bYdlLoadPlaylist      = !!m_chkLoadPlaylist.GetCheck();
	s.bYoutubePlaylistParser = !!m_chkIntYtPlaylistParser.GetCheck();

	CleanPath(s.strYdlExePath);

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

void CPPageYoutube::OnCheck60fps()
{
	if (m_chkHighFps.GetCheck()) {
		m_chkHdr.EnableWindow(TRUE);
	} else {
		m_chkHdr.SetCheck(BST_UNCHECKED);
		m_chkHdr.EnableWindow(FALSE);
	}

	SetModified();
}

void CPPageYoutube::OnCheckLoadPlaylist()
{
	if (m_chkLoadPlaylist.GetCheck()) {
		m_chkIntYtPlaylistParser.EnableWindow(TRUE);
	}  else {
		m_chkIntYtPlaylistParser.EnableWindow(FALSE);
	}

	SetModified();
}

void CPPageYoutube::OnCheckYDLEnable()
{
	const BOOL bEnable = m_chkYDLEnable.GetCheck();

	GetDlgItem(IDC_STATIC2)->EnableWindow(bEnable);
	m_cbVideoCodec.EnableWindow(bEnable);
	m_cbMaxHeight.EnableWindow(bEnable);
	m_chkHighFps.EnableWindow(bEnable);
	m_chkHdr.EnableWindow(bEnable);
	GetDlgItem(IDC_STATIC3)->EnableWindow(bEnable);
	m_cbAudioCodec.EnableWindow(bEnable);
	GetDlgItem(IDC_STATIC4)->EnableWindow(bEnable);
	m_cbAudioLang.EnableWindow(bEnable);
	m_chkHighBitrate.EnableWindow(bEnable);
	m_chkLoadPlaylist.EnableWindow(bEnable);
	m_chkIntYtPlaylistParser.EnableWindow(bEnable);

	if (bEnable) {
		OnCheck60fps();
		OnCheckLoadPlaylist();
	}

	SetModified();
}

void CPPageYoutube::OnBnClickedDefault()
{
	m_chkYDLEnable.SetCheck(BST_UNCHECKED);

	m_cbVideoCodec.SetCurSel(YT_DLP::vcodec_vp9);
	SelectByItemData(m_cbMaxHeight, 720);
	m_chkHighFps.SetCheck(BST_UNCHECKED);
	m_chkHdr.SetCheck(BST_UNCHECKED);;
	m_cbAudioCodec.SetCurSel(YT_DLP::acodec_opus);
	m_cbAudioLang.SetCurSel(0);
	m_chkHighBitrate.SetCheck(BST_UNCHECKED);
	m_chkLoadPlaylist.SetCheck(BST_UNCHECKED);
	m_chkIntYtPlaylistParser.SetCheck(BST_UNCHECKED);

	m_edAceStreamAddress.SetWindowTextW(L"http://127.0.0.1:6878/ace/getstream?id=%s");
	m_edTorrServerAddress.SetWindowTextW(L"http://127.0.0.1:8090/stream/fname?link=%s&index=1&m3u");
	m_edUserAgent.SetWindowTextW(L"Mozilla/5.0");

	CStringW ydl_path;
	m_cbYDLExePath.GetWindowTextW(ydl_path);
	if (ydl_path != DEFAULT_YTDLP_EXE) {
		// reset m_cbYDLExePath only if the path is not found
		ydl_path = GetFullExePath(ydl_path, true);
		if (ydl_path.IsEmpty()) {
			m_cbYDLExePath.SelectString(0, DEFAULT_YTDLP_EXE);
		}
	}

	OnCheckYDLEnable();

	Invalidate();
	SetModified();
}

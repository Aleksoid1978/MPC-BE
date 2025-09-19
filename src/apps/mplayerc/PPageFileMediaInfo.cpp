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
#include "MainFrm.h"
#include "PPageFileMediaInfo.h"
#include "DSUtil/FileHandle.h"

static String mi_get_lang_file()
{
	HINSTANCE mpcres = LoadLibraryW(CMPlayerCApp::GetSatelliteDll(AfxGetAppSettings().iLanguage));

	String str = L"  Config_Text_ColumnSize;30";
	if (mpcres) {
		HRSRC hRes = FindResourceW(mpcres, MAKEINTRESOURCEW(IDB_MEDIAINFO_LANGUAGE), L"FILE");

		if (hRes) {
			HANDLE lRes = ::LoadResource(mpcres, hRes);
			if (lRes) {
				LPCSTR lpMultiByteStr = (LPCSTR)LockResource(lRes);

				BOOL acp = FALSE;
				int dstlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, lpMultiByteStr, -1, nullptr, 0);
				if (!dstlen) {
					acp = TRUE;
					dstlen = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, lpMultiByteStr, -1, nullptr, 0);
				}

				if (dstlen) {
					WCHAR* wstr = new(std::nothrow) WCHAR[dstlen];
					if (wstr) {
						MultiByteToWideChar(acp ? CP_ACP : CP_UTF8, MB_ERR_INVALID_CHARS, lpMultiByteStr, -1, wstr, dstlen);
						str = wstr;
						delete [] wstr;
					}
				}
			}
			FreeLibrary(mpcres);
		}
	}

	return str;
}

// CPPageFileMediaInfo dialog

IMPLEMENT_DYNAMIC(CPPageFileMediaInfo, CPropertyPage)
CPPageFileMediaInfo::CPPageFileMediaInfo(const std::list<CString>& files, CDPI* pSheetDpi)
	: CPropertyPage(CPPageFileMediaInfo::IDD, CPPageFileMediaInfo::IDD)
	, m_files(files)
	, m_pSheetDpi(pSheetDpi)
{
	ASSERT(pSheetDpi);
}

void CPPageFileMediaInfo::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_COMBO1, m_cbFilename);
	DDX_Control(pDX, IDC_MIEDIT, m_edMediainfo);
}

BOOL CPPageFileMediaInfo::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->hwnd == m_edMediainfo.m_hWnd) {
		if ((LOWORD(pMsg->wParam) == 'A' || LOWORD(pMsg->wParam) == 'a')
				&& GetKeyState(VK_CONTROL) < 0) {
			m_edMediainfo.SetSel(0, -1, TRUE);
			return TRUE;
		}
	}

	return __super::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(CPPageFileMediaInfo, CPropertyPage)
	ON_WM_SIZE()
	ON_WM_SHOWWINDOW()
	ON_CBN_SELCHANGE(IDC_COMBO1, OnComboFileChange)
END_MESSAGE_MAP()

// CPPageFileMediaInfo message handlers

BOOL CPPageFileMediaInfo::OnInitDialog()
{
	__super::OnInitDialog();

	LOGFONTW lf = {};
	lf.lfPitchAndFamily = DEFAULT_PITCH | FF_MODERN;
	lf.lfHeight = -m_pSheetDpi->ScaleY(12);

	UINT i = 0;
	BOOL success;

	do {
		wcscpy_s(lf.lfFaceName, LF_FACESIZE, MonospaceFonts[i]);
		success = IsFontInstalled(MonospaceFonts[i]) && m_font.CreateFontIndirectW(&lf);
		i++;
	} while (!success && i < std::size(MonospaceFonts));

	m_edMediainfo.SetFont(&m_font);

	for (const auto& fn : m_files) {
		m_cbFilename.AddString(GetFileName(fn));
	}
	m_cbFilename.SetCurSel(0);
	if (m_cbFilename.GetCount() <= 1) {
		m_cbFilename.EnableWindow(FALSE);
	}
	OnComboFileChange();

	return TRUE;
}

BOOL CPPageFileMediaInfo::OnSetActive()
{
	if (GetParent()->GetDlgItem(IDC_BUTTON_MI_SAVEAS)) {
		GetParent()->GetDlgItem(IDC_BUTTON_MI_SAVEAS)->ShowWindow(SW_SHOW);
		GetParent()->GetDlgItem(IDC_BUTTON_MI_CLIPBOARD)->ShowWindow(SW_SHOW);
	}

	return __super::OnSetActive();
}

BOOL CPPageFileMediaInfo::OnKillActive()
{
	if (GetParent()->GetDlgItem(IDC_BUTTON_MI_SAVEAS)) {
		GetParent()->GetDlgItem(IDC_BUTTON_MI_SAVEAS)->ShowWindow(SW_HIDE);
		GetParent()->GetDlgItem(IDC_BUTTON_MI_CLIPBOARD)->ShowWindow(SW_HIDE);
	}

	return __super::OnKillActive();
}

void CPPageFileMediaInfo::OnSize(UINT nType, int cx, int cy)
{
	int dx = cx - m_rCrt.Width();
	int dy = cy - m_rCrt.Height();
	GetClientRect(&m_rCrt);

	if (::IsWindow(m_cbFilename.GetSafeHwnd())) {
		CRect r;
		m_cbFilename.GetWindowRect(&r);
		r.right += dx;
		r.bottom += dy;

		m_cbFilename.SetWindowPos(nullptr, 0, 0, r.Width(), r.Height(), SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
	}

	if (::IsWindow(m_edMediainfo.GetSafeHwnd())) {
		CRect r;
		m_edMediainfo.GetWindowRect(&r);
		r.right += dx;
		r.bottom += dy;

		m_edMediainfo.SetWindowPos(nullptr, 0, 0, r.Width(), r.Height(), SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
	}
}

void CPPageFileMediaInfo::OnComboFileChange()
{
	const int sel = m_cbFilename.GetCurSel();
	if (sel == m_fileindex || sel < 0 || sel >= (int)m_files.size()) {
		return;
	}

	m_fileindex = sel;
	auto it = m_files.cbegin();
	std::advance(it, m_fileindex);

	MediaInfo MI;

	MI.Option(L"ParseSpeed", L"0.5");
	MI.Option(L"Language", mi_get_lang_file());
	MI.Option(L"LegacyStreamDisplay", L"1");
	MI.Option(L"Complete");

	MI_File = (*it);
	MI.Open(MI_File.GetString());
	MI_Text = MI.Inform().c_str();
	MI.Close();

	if (!MI_Text.Find(L"Unable to load")) {
		MI_Text.Empty();
	}

	PAINTSTRUCT ps;
	CDC* cDC = m_edMediainfo.BeginPaint(&ps);
	m_edMediainfo.SetWindowTextW(MI_Text);
	m_edMediainfo.EndPaint(&ps);
}

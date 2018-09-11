/*
 * (C) 2012-2017 see Authors.txt
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
#include "../../DSUtil/WinAPIUtils.h"

static String mi_get_lang_file()
{
	HINSTANCE mpcres = nullptr;
	int lang = AfxGetAppSettings().iLanguage;

	if (lang) {
		mpcres = LoadLibraryW(CMPlayerCApp::GetSatelliteDll(lang));
	}

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
					WCHAR* wstr = DNew WCHAR[dstlen];
					if (wstr) {
						MultiByteToWideChar(acp ? CP_ACP : CP_UTF8, MB_ERR_INVALID_CHARS, lpMultiByteStr, -1, wstr, dstlen);
						str = wstr;
						delete[] wstr;
					}
				}

				UnlockResource(lRes);
				FreeResource(lRes);
			}
			FreeLibrary(mpcres);
		}
	}

	return str;
}

// CPPageFileMediaInfo dialog

IMPLEMENT_DYNAMIC(CPPageFileMediaInfo, CPropertyPage)
CPPageFileMediaInfo::CPPageFileMediaInfo(CString fn)
	: CPropertyPage(CPPageFileMediaInfo::IDD, CPPageFileMediaInfo::IDD)
	, m_fn(fn)
	, m_pCFont(nullptr)
{
}

CPPageFileMediaInfo::~CPPageFileMediaInfo()
{
	delete m_pCFont;
	m_pCFont = nullptr;
}

void CPPageFileMediaInfo::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_MIEDIT, m_mediainfo);
}

BOOL CPPageFileMediaInfo::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->hwnd == m_mediainfo) {
		if ((LOWORD(pMsg->wParam) == 'A' || LOWORD(pMsg->wParam) == 'a')
				&& GetKeyState(VK_CONTROL) < 0) {
			m_mediainfo.SetSel(0, -1, TRUE);
			return TRUE;
		}
	}

	return __super::PreTranslateMessage(pMsg);
}


BEGIN_MESSAGE_MAP(CPPageFileMediaInfo, CPropertyPage)
	ON_WM_SIZE()
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()

// CPPageFileMediaInfo message handlers

BOOL CPPageFileMediaInfo::OnInitDialog()
{
	__super::OnInitDialog();

	if (!m_pCFont) {
		m_pCFont = DNew CFont;
	}

	if (!m_pCFont) {
		return TRUE;
	}

	MediaInfo MI;

	MI.Option(L"ParseSpeed", L"0.5");
	MI.Option(L"Language", mi_get_lang_file());
	MI.Option(L"LegacyStreamDisplay", L"1");
	MI.Option(L"Complete");
	MI.Open(m_fn.GetString());
	MI_Text = MI.Inform().c_str();
	MI.Close();

	if (!MI_Text.Find(L"Unable to load")) {
		MI_Text.Empty();
	}

	LOGFONT lf;
	memset(&lf, 0, sizeof(lf));
	lf.lfPitchAndFamily = DEFAULT_PITCH | FF_MODERN;

	UINT i = 0;
	BOOL success;

	PAINTSTRUCT ps;
	CDC* cDC = m_mediainfo.BeginPaint(&ps);

	do {
		wcscpy_s(lf.lfFaceName, LF_FACESIZE, MonospaceFonts[i]);
		lf.lfHeight = -MulDiv(8, cDC->GetDeviceCaps(LOGPIXELSY), 72);
		success = IsFontInstalled(MonospaceFonts[i]) && m_pCFont->CreateFontIndirectW(&lf);
		i++;
	} while (!success && i < _countof(MonospaceFonts));

	m_mediainfo.SetFont(m_pCFont);
	m_mediainfo.SetWindowTextW(MI_Text);

	m_mediainfo.EndPaint(&ps);

	return TRUE;
}

void CPPageFileMediaInfo::OnShowWindow(BOOL bShow, UINT nStatus)
{
	__super::OnShowWindow(bShow, nStatus);

	if (bShow) {
		GetParent()->GetDlgItem(IDC_BUTTON_MI_SAVEAS)->ShowWindow(SW_SHOW);
		GetParent()->GetDlgItem(IDC_BUTTON_MI_CLIPBOARD)->ShowWindow(SW_SHOW);
	} else {
		GetParent()->GetDlgItem(IDC_BUTTON_MI_SAVEAS)->ShowWindow(SW_HIDE);
		GetParent()->GetDlgItem(IDC_BUTTON_MI_CLIPBOARD)->ShowWindow(SW_HIDE);
	}
}

void CPPageFileMediaInfo::OnSize(UINT nType, int cx, int cy)
{
	int dx = cx - m_rCrt.Width();
	int dy = cy - m_rCrt.Height();
	GetClientRect(&m_rCrt);

	if (::IsWindow(m_mediainfo.GetSafeHwnd())) {
		CRect r;
		m_mediainfo.GetWindowRect(&r);
		r.right += dx;
		r.bottom += dy;

		m_mediainfo.SetWindowPos(nullptr, 0, 0, r.Width(), r.Height(), SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
	}
}

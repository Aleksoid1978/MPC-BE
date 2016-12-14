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
#include "MainFrm.h"
#include "PPageFileMediaInfo.h"
#include "../../DSUtil/WinAPIUtils.h"

static String mi_get_lang_file()
{
	HINSTANCE mpcres = NULL;
	int lang = AfxGetAppSettings().iLanguage;

	if (lang) {
		mpcres = LoadLibrary(CMPlayerCApp::GetSatelliteDll(lang));
	}

	String str = L"  Config_Text_ColumnSize;30";
	if (mpcres) {
		HRSRC hRes = FindResource(mpcres, MAKEINTRESOURCE(IDB_MEDIAINFO_LANGUAGE), L"FILE");

		if (hRes) {
			HANDLE lRes = ::LoadResource(mpcres, hRes);
			if (lRes) {
				LPCSTR lpMultiByteStr = (LPCSTR)LockResource(lRes);

				BOOL acp = FALSE;
				int dstlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, lpMultiByteStr, -1, NULL, 0);
				if (!dstlen) {
					acp = TRUE;
					dstlen = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, lpMultiByteStr, -1, NULL, 0);
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
	, m_pCFont(NULL)
{
}

CPPageFileMediaInfo::~CPPageFileMediaInfo()
{
	delete m_pCFont;
	m_pCFont = NULL;
}

void CPPageFileMediaInfo::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_MIEDIT, m_mediainfo);
}

BEGIN_MESSAGE_MAP(CPPageFileMediaInfo, CPropertyPage)
	ON_WM_SIZE()
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()

// CPPageFileMediaInfo message handlers

static WNDPROC OldControlProc;

static LRESULT CALLBACK ControlProc(HWND control, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_KEYDOWN) {
		if ((LOWORD(wParam)== 'A' || LOWORD(wParam) == 'a') && GetKeyState(VK_CONTROL) < 0) {
			CEdit *pEdit = (CEdit*)CWnd::FromHandle(control);
			pEdit->SetSel(0, pEdit->GetWindowTextLength(), TRUE);
			return 0;
		}
	}

	return CallWindowProc(OldControlProc, control, message, wParam, lParam);
}

static int CALLBACK EnumFontFamExProc(ENUMLOGFONTEX* /*lpelfe*/, NEWTEXTMETRICEX* /*lpntme*/, int /*FontType*/, LPARAM lParam)
{
	LPARAM* l = (LPARAM*)lParam;
	*l = TRUE;
	return TRUE;
}

static bool IsFontInstalled(LPCTSTR lpszFont)
{
	// Get the screen DC
	CDC dc;
	if (!dc.CreateCompatibleDC(NULL)) {
		return false;
	}

	LOGFONT lf = {0};
	// Any character set will do
	lf.lfCharSet = DEFAULT_CHARSET;
	// Set the facename to check for
	wcscpy_s(lf.lfFaceName, lpszFont);
	LPARAM lParam = 0;
	// Enumerate fonts
	EnumFontFamiliesEx(dc.GetSafeHdc(), &lf, (FONTENUMPROC)EnumFontFamExProc, (LPARAM)&lParam, 0);

	return lParam ? true : false;
}

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

	MI.Option(L"ParseSpeed", L"0");
	MI.Option(L"Language", mi_get_lang_file());
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

	LPCTSTR fonts[] = {L"Consolas", L"Lucida Console", L"Courier New", L"" };

	UINT i = 0;
	BOOL success;

	PAINTSTRUCT ps;
	CDC* cDC = m_mediainfo.BeginPaint(&ps);

	do {
		wcscpy_s(lf.lfFaceName, LF_FACESIZE, fonts[i]);
		lf.lfHeight = -MulDiv(8, cDC->GetDeviceCaps(LOGPIXELSY), 72);
		success = IsFontInstalled(fonts[i]) && m_pCFont->CreateFontIndirect(&lf);
		i++;
	} while (!success && i < _countof(fonts));

	m_mediainfo.SetFont(m_pCFont);
	m_mediainfo.SetWindowText(MI_Text);

	m_mediainfo.EndPaint(&ps);

	OldControlProc = (WNDPROC)SetWindowLongPtr(m_mediainfo.m_hWnd, GWLP_WNDPROC, (LONG_PTR)ControlProc);

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

		m_mediainfo.SetWindowPos(NULL, 0, 0, r.Width(), r.Height(), SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
	}
}

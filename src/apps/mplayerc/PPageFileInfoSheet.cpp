/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2018 see Authors.txt
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
#include "PPageFileInfoSheet.h"
#include "PPageFileMediaInfo.h"

IMPLEMENT_DYNAMIC(CMPCPropertySheet, CPropertySheet)
CMPCPropertySheet::CMPCPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	: CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
}

// CPPageFileInfoSheet

IMPLEMENT_DYNAMIC(CPPageFileInfoSheet, CMPCPropertySheet)
CPPageFileInfoSheet::CPPageFileInfoSheet(CString fn, CMainFrame* pMainFrame, CWnd* pParentWnd)
	: CMPCPropertySheet(ResStr(IDS_PROPSHEET_PROPERTIES), pParentWnd, 0)
	, m_clip(fn, pMainFrame->m_pGB)
	, m_details(fn, pMainFrame->m_pGB, pMainFrame->m_pCAP, pMainFrame->m_pDVDI)
	, m_res(fn, pMainFrame->m_pGB)
	, m_pMainFrame(pMainFrame)
	, m_mi(fn)
	, m_fn(fn)
	, m_bNeedInit(TRUE)
	, m_nMinCX(0)
	, m_nMinCY(0)
{
	AddPage(&m_details);
	AddPage(&m_clip);

	BeginEnumFilters(pMainFrame->m_pGB, pEF, pBF) {
		if (CComQIPtr<IDSMResourceBag> pRB = pBF)
			if (pRB && pRB->ResGetCount() > 0) {
				AddPage(&m_res);
				break;
			}
	}
	EndEnumFilters;

	if (!::PathIsURLW(fn)) {
		AddPage(&m_mi);
	}
}

CPPageFileInfoSheet::~CPPageFileInfoSheet()
{
	CAppSettings& s = AfxGetAppSettings();

	s.iDlgPropX = m_rWnd.Width() - m_nMinCX;
	s.iDlgPropY = m_rWnd.Height() - m_nMinCY;
}

BEGIN_MESSAGE_MAP(CPPageFileInfoSheet, CMPCPropertySheet)
	ON_WM_DESTROY()
	ON_WM_GETMINMAXINFO()
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BUTTON_MI_SAVEAS, OnSaveAs)
	ON_BN_CLICKED(IDC_BUTTON_MI_CLIPBOARD, OnCopyToClipboard)
END_MESSAGE_MAP()

// CPPageFileInfoSheet message handlers

BOOL CPPageFileInfoSheet::OnInitDialog()
{
	BOOL bResult = CPropertySheet::OnInitDialog();

	const CAppSettings& s = AfxGetAppSettings();

	m_fn.TrimRight('/');
	int i = std::max(m_fn.ReverseFind('\\'), m_fn.ReverseFind('/'));

	if (i >= 0 && i < m_fn.GetLength()-1) {
		m_fn = m_fn.Mid(i+1);
	}

	m_fn = m_fn + L".MediaInfo.txt";

	GetDlgItem(IDCANCEL)->ShowWindow(SW_HIDE);
	GetDlgItem(ID_APPLY_NOW)->ShowWindow(SW_HIDE);
	GetDlgItem(IDOK)->SetWindowTextW(ResStr(IDS_AG_CLOSE));

	CRect r;
	GetDlgItem(ID_APPLY_NOW)->GetWindowRect(&r);
	ScreenToClient(r);
	GetDlgItem(IDOK)->MoveWindow(r);

	r.MoveToX(5);
	r.right = r.left + m_pMainFrame->ScaleX(120);
	m_Button_MI_SaveAs.Create(ResStr(IDS_AG_SAVE_AS), WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE, r, this, IDC_BUTTON_MI_SAVEAS);
	m_Button_MI_SaveAs.SetFont(GetFont());
	m_Button_MI_SaveAs.ShowWindow(SW_HIDE);

	r.MoveToX(r.Width() + m_pMainFrame->ScaleX(10));
	m_Button_MI_Clipboard.Create(ResStr(IDS_COPY_TO_CLIPBOARD), WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE, r, this, IDC_BUTTON_MI_CLIPBOARD);
	m_Button_MI_Clipboard.SetFont(GetFont());
	m_Button_MI_Clipboard.ShowWindow(SW_HIDE);

	GetTabControl()->SetFocus();

	GetWindowRect(&m_rWnd);
	m_nMinCX = m_rWnd.Width();
	m_nMinCY = m_rWnd.Height();

	m_bNeedInit = FALSE;
	GetClientRect(&m_rCrt);
	ScreenToClient(&m_rCrt);

	GetWindowRect(&r);
	ScreenToClient(&r);
	r.right		+= s.iDlgPropX;
	r.bottom	+= s.iDlgPropY;
	MoveWindow (&r);

	CenterWindow();

	ModifyStyle(0, WS_MAXIMIZEBOX);

	for (int i = 0; i < GetPageCount(); i++) {
		DWORD nID = GetResourceId(i);
		if (nID == s.nLastFileInfoPage) {
			SetActivePage(i);
			break;
		}
	}

	return bResult;
}

void CPPageFileInfoSheet::OnSaveAs()
{
	CFileDialog filedlg (FALSE, L"*.txt", m_fn,
						 OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR,
						 L"Text Files (*.txt)|*.txt|All Files (*.*)|*.*||", nullptr);

	if (filedlg.DoModal() == IDOK) {
		CFile mFile;
		if (mFile.Open(filedlg.GetPathName(), CFile::modeCreate | CFile::modeWrite)) {
			const WCHAR bom = 0xFEFF;
			mFile.Write(&bom, sizeof(WCHAR));
			mFile.Write(LPCTSTR(m_mi.MI_Text), m_mi.MI_Text.GetLength() * sizeof(WCHAR));
			mFile.Close();
		}
	}
}

void CPPageFileInfoSheet::OnCopyToClipboard()
{
	size_t cbStr = (m_mi.MI_Text.GetLength() + 1) * sizeof(WCHAR);
	HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, cbStr);
	if (hGlob) {
		LPVOID pData = GlobalLock(hGlob);
		if (pData) {
			memcpy_s(GlobalLock(pData), cbStr, m_mi.MI_Text.LockBuffer(), cbStr);
			GlobalUnlock(hGlob);
			m_mi.MI_Text.UnlockBuffer();

			if (OpenClipboard()) {
				EmptyClipboard();
				::SetClipboardData(CF_UNICODETEXT, hGlob);
				CloseClipboard();
			}
		}

		GlobalFree(hGlob);
	}
}

int CALLBACK CPPageFileInfoSheet::XmnPropSheetCallback(HWND hWnd, UINT message, LPARAM lParam)
{
	extern int CALLBACK AfxPropSheetCallback(HWND, UINT message, LPARAM lParam);
	int nRes = AfxPropSheetCallback(hWnd, message, lParam);

	switch (message) {
		case PSCB_PRECREATE:
			((LPDLGTEMPLATE)lParam)->style |= (DS_3DLOOK | DS_SETFONT | WS_THICKFRAME | WS_SYSMENU | DS_MODALFRAME | WS_VISIBLE | WS_CAPTION);
			break;
	}

	return nRes;
}

INT_PTR CPPageFileInfoSheet::DoModal()
{
	m_psh.dwFlags |= PSH_USECALLBACK;
	m_psh.pfnCallback = XmnPropSheetCallback;

	return CPropertySheet::DoModal();
}

void CPPageFileInfoSheet::OnSize(UINT nType, int cx, int cy)
{
	CPropertySheet::OnSize(nType, cx, cy);

	CRect r;

	if (m_bNeedInit) {
		return;
	}

	CTabCtrl *pTab = GetTabControl();
	ASSERT(nullptr != pTab && IsWindow(pTab->m_hWnd));

	int dx = cx - m_rCrt.Width();
	int dy = cy - m_rCrt.Height();
	GetClientRect(&m_rCrt);

	HDWP hDWP = ::BeginDeferWindowPos(5);

	UINT uDefFlags = SWP_NOACTIVATE | SWP_NOZORDER;

	pTab->GetClientRect(&r);
	r.right += dx;
	r.bottom += dy;
	::DeferWindowPos(hDWP, pTab->m_hWnd, nullptr, 0, 0, r.Width(), r.Height(), uDefFlags | SWP_NOMOVE);

	for (CWnd *pChild = GetWindow(GW_CHILD); pChild != nullptr; pChild = pChild->GetWindow(GW_HWNDNEXT)) {
		if ((pChild->SendMessageW(WM_GETDLGCODE) & DLGC_BUTTON) && pChild == GetDlgItem(IDOK)) {
			pChild->GetWindowRect(&r);
			ScreenToClient(&r);
			r.top += dy;
			r.bottom += dy;
			r.left+= dx;
			r.right += dx;
			::DeferWindowPos(hDWP, pChild->m_hWnd, nullptr, r.left, r.top, 0, 0, uDefFlags | SWP_NOSIZE);
		} else if ((pChild->SendMessageW(WM_GETDLGCODE) & DLGC_BUTTON) && (pChild == GetDlgItem(IDC_BUTTON_MI_SAVEAS) || pChild == GetDlgItem(IDC_BUTTON_MI_CLIPBOARD))) {
			pChild->GetWindowRect(&r);
			ScreenToClient(&r);
			r.top += dy;
			r.bottom += dy;
			::DeferWindowPos(hDWP, pChild->m_hWnd, nullptr, r.left, r.top, 0, 0, uDefFlags | SWP_NOSIZE);
		} else {
			pChild->GetClientRect(&r);
			r.right += dx;
			r.bottom += dy;
			::DeferWindowPos(hDWP, pChild->m_hWnd, nullptr, 0, 0, r.Width(), r.Height(), uDefFlags | SWP_NOMOVE);
		}
	}

	::EndDeferWindowPos(hDWP);

	if (nType != SIZE_MAXIMIZED) {
		GetWindowRect(&m_rWnd);
	}
}

void CPPageFileInfoSheet::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
	CPropertySheet::OnGetMinMaxInfo(lpMMI);
	lpMMI->ptMinTrackSize.x = m_nMinCX;
	lpMMI->ptMinTrackSize.y = m_nMinCY;
}

void CPPageFileInfoSheet::OnDestroy()
{
	AfxGetAppSettings().nLastFileInfoPage = GetResourceId(GetActiveIndex());

	CPropertySheet::OnDestroy();
}

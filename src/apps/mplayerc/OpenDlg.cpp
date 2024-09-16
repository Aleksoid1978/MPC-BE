/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2024 see Authors.txt
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
#include <ShlObj_core.h>
#include <dlgs.h>
#include "OpenDlg.h"
#include "FileDialogs.h"
#include "MainFrm.h"

//
// COpenDlg dialog
//

//IMPLEMENT_DYNAMIC(COpenDlg, CResizableDialog)

COpenDlg::COpenDlg(CWnd* pParent /*=nullptr*/)
	: CResizableDialog(COpenDlg::IDD, pParent)
{
	m_hIcon = (HICON)LoadImageW(AfxGetInstanceHandle(), MAKEINTRESOURCEW(IDR_MAINFRAME), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
}

COpenDlg::~COpenDlg()
{
	if (m_hIcon) {
		DestroyIcon(m_hIcon);
	}
}

void COpenDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Check(pDX, IDC_CHECK2, m_bPasteClipboardURL);
	DDX_Control(pDX, IDC_COMBO1, m_mrucombo);
	DDX_CBString(pDX, IDC_COMBO1, m_path);
	DDX_Control(pDX, IDC_COMBO2, m_mrucombo2);
	DDX_CBString(pDX, IDC_COMBO2, m_path2);
	DDX_Check(pDX, IDC_CHECK1, m_bAppendPlaylist);
}

BEGIN_MESSAGE_MAP(COpenDlg, CResizableDialog)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedBrowsebutton)
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedBrowsebutton2)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_UPDATE_COMMAND_UI(IDC_STATIC1, OnUpdateDub)
	ON_UPDATE_COMMAND_UI(IDC_COMBO2, OnUpdateDub)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON2, OnUpdateDub)
	ON_UPDATE_COMMAND_UI(IDOK, OnUpdateOk)
END_MESSAGE_MAP()

// COpenDlg message handlers

BOOL COpenDlg::OnInitDialog()
{
	__super::OnInitDialog();

	CAppSettings& s = AfxGetAppSettings();

	m_bPasteClipboardURL = s.bPasteClipboardURL;
	UpdateData(FALSE);

	if (s.bKeepHistory) {
		m_mrucombo.ResetContent();
		m_mrucombo2.ResetContent();

		std::vector<SessionInfo> recentSessions;
		AfxGetMyApp()->m_HistoryFile.GetRecentSessions(recentSessions, AfxGetAppSettings().iRecentFilesNumber);

		for (const auto& rs : recentSessions) {
			if (!rs.Path.IsEmpty()) {
				m_mrucombo.AddString(rs.Path);
			}
			if (!rs.AudioPath.IsEmpty()) {
				m_mrucombo2.AddString(rs.Path);
			}
		}
		CorrectComboListWidth(m_mrucombo);
		CorrectComboListWidth(m_mrucombo2);

		if (m_mrucombo.GetCount() > 0) {
			m_mrucombo.SetCurSel(0);
		}
	}

	if (m_bPasteClipboardURL && ::IsClipboardFormatAvailable(CF_UNICODETEXT) && ::OpenClipboard(m_hWnd)) {
		if (HGLOBAL hglb = ::GetClipboardData(CF_UNICODETEXT)) {
			if (LPCWSTR pText = (LPCWSTR)::GlobalLock(hglb)) {
				if (AfxIsValidString(pText) && ::PathIsURLW(pText)) {
					m_mrucombo.SetWindowTextW(pText);
				}
				GlobalUnlock(hglb);
			}
		}
		CloseClipboard();
	}

	m_fns.clear();
	m_path.Empty();
	m_path2.Empty();
	m_bMultipleFiles = false;
	m_bAppendPlaylist = FALSE;

	AddAnchor(IDC_MAINFRAME_ICON, TOP_LEFT);
	AddAnchor(m_mrucombo, TOP_LEFT, TOP_RIGHT);
	AddAnchor(m_mrucombo2, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_BUTTON1, TOP_RIGHT);
	AddAnchor(IDC_BUTTON2, TOP_RIGHT);
	AddAnchor(IDOK, TOP_RIGHT);
	AddAnchor(IDCANCEL, TOP_RIGHT);
	AddAnchor(IDC_STATIC, TOP_LEFT);
	AddAnchor(IDC_STATIC1, TOP_LEFT);
	AddAnchor(IDC_STATIC2, TOP_LEFT);
	AddAnchor(IDC_CHECK1, TOP_LEFT);
	AddAnchor(IDC_CHECK2, TOP_LEFT);

	CRect r;
	GetWindowRect(r);
	SetMinTrackSize(r.Size());
	SetMaxTrackSize({ 1000, r.Height() });

	if (m_hIcon != nullptr) {
		CStatic *pStat = (CStatic*)GetDlgItem(IDC_MAINFRAME_ICON);
		pStat->SetIcon(m_hIcon);
	}

	return TRUE;
}

void COpenDlg::OnBnClickedBrowsebutton()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	CString filter;
	std::vector<CString> mask;
	s.m_Formats.GetFilter(filter, mask);

	DWORD dwFlags = OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_ALLOWMULTISELECT|OFN_ENABLEINCLUDENOTIFY|OFN_NOCHANGEDIR;

	if (!s.bKeepHistory) {
		dwFlags |= OFN_DONTADDTORECENT;
	}

	COpenFileDialog fd(nullptr, m_path, dwFlags, filter, this);

	if (fd.DoModal() != IDOK) {
		return;
	}

	m_fns.clear();
	fd.GetFilePaths(m_fns);

	if (m_fns.size() > 1
			|| m_fns.size() == 1
			&& (m_fns.front()[m_fns.front().GetLength()-1] == '\\'
				|| m_fns.front()[m_fns.front().GetLength()-1] == '*')) {
		m_bMultipleFiles = true;
		EndDialog(IDOK);
		return;
	}

	m_mrucombo.SetWindowTextW(fd.GetPathName());
}

void COpenDlg::OnBnClickedBrowsebutton2()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	CString filter;
	std::vector<CString> mask;
	s.m_Formats.GetAudioFilter(filter, mask);

	DWORD dwFlags = OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_ALLOWMULTISELECT|OFN_ENABLEINCLUDENOTIFY|OFN_NOCHANGEDIR;

	if (!s.bKeepHistory) {
		dwFlags |= OFN_DONTADDTORECENT;
	}

	COpenFileDialog fd(nullptr, m_path2, dwFlags, filter, this);

	if (fd.DoModal() != IDOK) {
		return;
	}

	std::list<CString> dubfns;
	fd.GetFilePaths(dubfns);

	const CString path = Implode(dubfns, L'|');
	m_mrucombo2.SetWindowTextW(path.GetString());
}

void COpenDlg::OnBnClickedOk()
{
	UpdateData();

	CleanPath(m_path);
	CleanPath(m_path2);

	m_fns.clear();
	m_fns.emplace_back(m_path);

	if (m_mrucombo2.IsWindowEnabled() && !m_path2.IsEmpty()) {
		std::list<CString> dubfns;
		Explode(m_path2, dubfns, L'|');
		for (const auto& fn : dubfns) {
			m_fns.emplace_back(fn);
		}
		if (::PathFileExistsW(dubfns.front())) {
			AfxGetMainFrame()->AddAudioPathsAddons(dubfns.front().GetString());
		}
	}

	m_bMultipleFiles = false;

	AfxGetAppSettings().bPasteClipboardURL = !!m_bPasteClipboardURL;

	OnOK();
}

void COpenDlg::OnUpdateDub(CCmdUI* pCmdUI)
{
	UpdateData();

	pCmdUI->Enable(AfxGetAppSettings().GetFileEngine(m_path) == DirectShow);
}

void COpenDlg::OnUpdateOk(CCmdUI* pCmdUI)
{
	UpdateData();

	pCmdUI->Enable(!CString(m_path).Trim().IsEmpty() || !CString(m_path2).Trim().IsEmpty());
}

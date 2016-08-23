/*
 * Copyright (C) 2012-2015 Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
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
#include "ConfigDlg.h"
#include <winuser.h>

// CConfigDlg dialog

#define MPCBE_PATH_RU	_T("Текущий путь MPC-BE:")
#define MPCBE_PATH_EN	_T("Current MPC-BE path:")

#define CANCEL_RU		_T("Отмена")
#define CANCEL_EN		_T("Cancel")

#define CAPTION_RU		_T("Настройки MPC-BE ShellExt")
#define CAPTION_EN		_T("MPC-BE ShellExt settings")

IMPLEMENT_DYNAMIC(CConfigDlg, CDialog)

CConfigDlg::CConfigDlg(CWnd* pParent)
	: CDialog(CConfigDlg::IDD, pParent)
{
}

CConfigDlg::~CConfigDlg()
{
}

void CConfigDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MPCCOMBO, m_MPCPath);
}

BEGIN_MESSAGE_MAP(CConfigDlg, CDialog)
	ON_BN_CLICKED(IDOK,		OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL,	OnBnClickedCancel)
END_MESSAGE_MAP()

// CConfigDlg message handlers

void CConfigDlg::OnBnClickedOk()
{
	CRegKey key;
	if (ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, _T("Software\\MPC-BE\\ShellExt"))) {
		CString path;
		m_MPCPath.GetLBText(m_MPCPath.GetCurSel(), path);
		if (::PathFileExists(path)) {
			key.SetStringValue(_T("MpcPath"), path);
		}
		key.Close();
	}

	OnOK();
}

void CConfigDlg::OnBnClickedCancel()
{
	OnCancel();
}

BOOL CConfigDlg::OnInitDialog()
{
	__super::OnInitDialog();

	SetWindowText((GetUserDefaultUILanguage() == 1049) ? CAPTION_RU : CAPTION_EN);

	::SetWindowText(GetDlgItem(IDC_STATIC)->m_hWnd, (GetUserDefaultUILanguage() == 1049) ? MPCBE_PATH_RU : MPCBE_PATH_EN);
	::SetWindowText(GetDlgItem(IDCANCEL)->m_hWnd,   (GetUserDefaultUILanguage() == 1049) ? CANCEL_RU : CANCEL_EN);

	SetClassLongPtr(GetDlgItem(IDOK)->m_hWnd,         GCLP_HCURSOR, (LONG_PTR)AfxGetApp()->LoadStandardCursor(IDC_HAND));
	SetClassLongPtr(GetDlgItem(IDCANCEL)->m_hWnd,     GCLP_HCURSOR, (LONG_PTR)AfxGetApp()->LoadStandardCursor(IDC_HAND));
	SetClassLongPtr(GetDlgItem(IDC_MPCCOMBO)->m_hWnd, GCLP_HCURSOR, (LONG_PTR)AfxGetApp()->LoadStandardCursor(IDC_HAND));

	CRegKey key;
	TCHAR path_buff[MAX_PATH];
	memset(path_buff, 0, sizeof(path_buff));
	ULONG len = sizeof(path_buff);

	if (ERROR_SUCCESS == key.Open(HKEY_LOCAL_MACHINE, _T("Software\\MPC-BE"))) {
		if (ERROR_SUCCESS == key.QueryStringValue(_T("ExePath"), path_buff, &len) && ::PathFileExists(path_buff)) {
			m_MPCPath.AddString(path_buff);
		}
		key.Close();
	}
#ifdef _WIN64
	// x86 application on x64 system
	if (ERROR_SUCCESS == key.Open(HKEY_LOCAL_MACHINE, _T("Software\\Wow6432Node\\MPC-BE"))) {
		len = sizeof(path_buff);
		memset(path_buff, 0, sizeof(path_buff));
		if (ERROR_SUCCESS == key.QueryStringValue(_T("ExePath"), path_buff, &len) && ::PathFileExists(path_buff)) {
			m_MPCPath.AddString(path_buff);
		}
		key.Close();
	}
#endif

	CString mpc_path = _T("");
	if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, _T("Software\\MPC-BE\\ShellExt"))) {
		len = sizeof(path_buff);
		memset(path_buff, 0, sizeof(path_buff));
		if (ERROR_SUCCESS == key.QueryStringValue(_T("MpcPath"), path_buff, &len) && ::PathFileExists(path_buff)) {
			mpc_path = CString(path_buff).Trim();
		}
		key.Close();
	}

	int sel = m_MPCPath.FindStringExact(0, mpc_path);
	m_MPCPath.SetCurSel(mpc_path.IsEmpty() ? 0 : (sel != LB_ERR) ? sel : 0);

	return TRUE;
}

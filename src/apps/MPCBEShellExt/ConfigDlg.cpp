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
#include "resource.h"
#include "ConfigDlg.h"
#include <winuser.h>

// CConfigDlg dialog

#define MPCBE_PATH_RU L"Текущий путь MPC-BE:"
#define MPCBE_PATH_EN L"Current MPC-BE path:"

#define CANCEL_RU     L"Отмена"
#define CANCEL_EN     L"Cancel"

#define CAPTION_RU    L"Настройки MPC-BE ShellExt"
#define CAPTION_EN    L"MPC-BE ShellExt settings"

//IMPLEMENT_DYNAMIC(CConfigDlg, CDialog)

CConfigDlg::CConfigDlg()
{
}

// CConfigDlg message handlers

LRESULT CConfigDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	SetWindowTextW((GetUserDefaultUILanguage() == 1049) ? CAPTION_RU : CAPTION_EN);

	GetDlgItem(IDC_STATIC1).SetWindowTextW((GetUserDefaultUILanguage() == 1049) ? MPCBE_PATH_RU : MPCBE_PATH_EN);
	GetDlgItem(IDCANCEL).SetWindowTextW((GetUserDefaultUILanguage() == 1049) ? CANCEL_RU : CANCEL_EN);

	m_hMPCPathCombo = GetDlgItem(IDC_MPCCOMBO);

	CRegKey key;
	WCHAR path_buff[MAX_PATH] = { 0 };
	ULONG len = sizeof(path_buff);

	CString mpc_path;
	if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, L"Software\\MPC-BE\\ShellExt")) {
		if (ERROR_SUCCESS == key.QueryStringValue(L"MpcPath", path_buff, &len) && ::PathFileExistsW(path_buff)) {
			mpc_path = CString(path_buff).Trim();
		}
		key.Close();
	}

	if (ERROR_SUCCESS == key.Open(HKEY_LOCAL_MACHINE, L"Software\\MPC-BE")) {
		len = sizeof(path_buff);
		memset(path_buff, 0, sizeof(path_buff));
		if (ERROR_SUCCESS == key.QueryStringValue(L"ExePath", path_buff, &len) && ::PathFileExistsW(path_buff)) {
			::SendMessage(m_hMPCPathCombo, CB_ADDSTRING, 0, (LPARAM)path_buff);
		}
		key.Close();
	}
#ifdef _WIN64
	// x86 application on x64 system
	if (ERROR_SUCCESS == key.Open(HKEY_LOCAL_MACHINE, L"Software\\Wow6432Node\\MPC-BE")) {
		len = sizeof(path_buff);
		memset(path_buff, 0, sizeof(path_buff));
		if (ERROR_SUCCESS == key.QueryStringValue(L"ExePath", path_buff, &len) && ::PathFileExistsW(path_buff)) {
			::SendMessage(m_hMPCPathCombo, CB_ADDSTRING, 0, (LPARAM)path_buff);
		}
		key.Close();
	}
#endif

	::SendMessage(m_hMPCPathCombo, CB_SELECTSTRING, 0, (LPARAM)mpc_path.GetString());

	return 0;
}

LRESULT CConfigDlg::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	const int comboLength = ::GetWindowTextLengthW(m_hMPCPathCombo);
	CString strCombo = new WCHAR[comboLength + 1];

	::GetWindowTextW(m_hMPCPathCombo, strCombo.GetBuffer(comboLength), comboLength + 1);
	strCombo.ReleaseBuffer();

	CRegKey key;
	if (ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, L"Software\\MPC-BE\\ShellExt")) {
		if (::PathFileExistsW(strCombo)) {
			key.SetStringValue(L"MpcPath", strCombo);
		}
		key.Close();
	}

	EndDialog(IDOK);

	return 0;
}

LRESULT CConfigDlg::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	EndDialog(IDCANCEL);

	return 0;
}

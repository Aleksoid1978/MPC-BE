/*
 * (C) 2014-2017 see Authors.txt
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
#include "CmdLineHelpDlg.h"
#include "SettingsDefines.h"

CmdLineHelpDlg::CmdLineHelpDlg(const CString& cmdLine)
	: CResizableDialog(CmdLineHelpDlg::IDD)
	, m_cmdLine(cmdLine)
{
	m_cmdLine.Replace(L"\n", L"\r\n");
}

CmdLineHelpDlg::~CmdLineHelpDlg()
{
}

void CmdLineHelpDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC1, m_icon);
	DDX_Text(pDX, IDC_EDIT1, m_text);
}


BEGIN_MESSAGE_MAP(CmdLineHelpDlg, CResizableDialog)
END_MESSAGE_MAP()

BOOL CmdLineHelpDlg::OnInitDialog()
{
	__super::OnInitDialog();

	m_icon.SetIcon(LoadIcon(nullptr, IDI_INFORMATION));

	m_text = m_cmdLine;

	UpdateData(FALSE);

	GetDlgItem(IDOK)->SetFocus(); // Force the focus on the OK button

	AddAnchor(IDC_STATIC1, TOP_LEFT);
	AddAnchor(IDC_EDIT1, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);

	EnableSaveRestore(IDS_R_DLG_CMD_LINE_HELP);

	return FALSE;
}

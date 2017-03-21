/*
 * (C) 2017 see Authors.txt
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
#include "ShaderNewDlg.h"

// CShaderNewDlg dialog

IMPLEMENT_DYNAMIC(CShaderNewDlg, CCmdUIDialog)
CShaderNewDlg::CShaderNewDlg(CWnd* pParent)
	: CCmdUIDialog(CShaderNewDlg::IDD, pParent)
{
}

CShaderNewDlg::~CShaderNewDlg()
{
}

void CShaderNewDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_EDIT1, m_Name);
}

BOOL CShaderNewDlg::OnInitDialog()
{
	__super::OnInitDialog();

	UpdateData(FALSE);

	return TRUE;
}

BEGIN_MESSAGE_MAP(CShaderNewDlg, CCmdUIDialog)
	ON_UPDATE_COMMAND_UI(IDOK, OnUpdateOk)
END_MESSAGE_MAP()

// CShaderNewDlg message handlers

void CShaderNewDlg::OnUpdateOk(CCmdUI *pCmdUI)
{
	UpdateData();

	pCmdUI->Enable(!m_Name.IsEmpty());
}

void CShaderNewDlg::OnOK()
{
	UpdateData();

	m_Name.Trim();

	CString str = m_Name;
	FixFilename(str);

	if (m_Name.IsEmpty() && m_Name != str) {
		AfxMessageBox(L"Incorrect file name.", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	__super::OnOK();
}

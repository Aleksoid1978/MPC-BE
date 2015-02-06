/*
 * (C) 2015 see Authors.txt
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
#include <afxinet.h>
#include "ItemPropertiesDlg.h"

// CItemPropertiesDlg

IMPLEMENT_DYNAMIC(CItemPropertiesDlg, CDialog)

CItemPropertiesDlg::CItemPropertiesDlg(LPCTSTR propName, LPCTSTR propPath, CWnd* pParent /*=NULL*/)
	: CDialog(CItemPropertiesDlg::IDD, pParent)
	, m_PropertyName(propName)
	, m_PropertyPath(propPath)
{
}

CItemPropertiesDlg::~CItemPropertiesDlg()
{
}

void CItemPropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDOK, m_okButton);
	DDX_Control(pDX, IDCANCEL, m_cancelButton);

	DDX_Text(pDX, IDC_EDIT1, m_PropertyName);
	DDX_Text(pDX, IDC_EDIT2, m_PropertyPath);
}

BEGIN_MESSAGE_MAP(CItemPropertiesDlg, CDialog)
END_MESSAGE_MAP()

BOOL CItemPropertiesDlg::OnInitDialog()
{
	__super::OnInitDialog();

	CString caption;
	caption.Format(ResStr(IDS_PROPERTIESFOR), m_PropertyName);
	SetWindowText(caption);

	return TRUE;
}

void CItemPropertiesDlg::OnOK()
{
	UpdateData();

	__super::OnOK();
}

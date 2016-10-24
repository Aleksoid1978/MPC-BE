/*
 * (C) 2015-2016 see Authors.txt
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

#pragma once

#include <afxwin.h>
//#include "afxdialogex.h"
#include <ResizableLib/ResizableDialog.h>

class CItemPropertiesDlg : public CResizableDialog
{
	//DECLARE_DYNAMIC(CItemPropertiesDlg)

public:
	CItemPropertiesDlg(LPCTSTR propName, LPCTSTR propPath, CWnd* pParent = NULL);
	virtual ~CItemPropertiesDlg();

	LPCTSTR GetPropertyName() { return m_PropertyName; };
	LPCTSTR GetPropertyPath() { return m_PropertyPath; };

	enum { IDD = IDD_ITEMPROPERTIES_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	afx_msg virtual BOOL OnInitDialog();
	afx_msg virtual void OnOK();

	DECLARE_MESSAGE_MAP()

private:
	CString m_PropertyName;
	CString m_PropertyPath;

	CButton m_okButton;
	CButton m_cancelButton;
};

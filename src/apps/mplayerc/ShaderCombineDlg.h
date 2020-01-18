/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2020 see Authors.txt
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
#include <ResizableLib/ResizableDialog.h>

// CShaderCombineDlg dialog

class CShaderCombineDlg : public CCmdUIDialog
{
#define SHADER1      (1 << 0)
#define SHADER2      (1 << 1)
#define SHADER11_2   (1 << 3)

	const bool m_bEnableD3D11;

	int m_iLastSel = -1; // 0 - DX9, 1 - DX11
	CComboBox m_cbDXNum;
	CComboBox m_cbShaders;

	CButton   m_chEnable1;
	CButton   m_chEnable2;

	CListBox  m_cbList1;
	CListBox  m_cbList2;

	bool m_oldcheck1;
	bool m_oldcheck2;
	std::list<CString> m_oldlabels1;
	std::list<CString> m_oldlabels2;

	CString m_AppSavePath;

	void UpdateShaders(unsigned type);

public:
	CShaderCombineDlg(CWnd* pParent, const bool bD3D11);
	virtual ~CShaderCombineDlg();

	enum { IDD = IDD_SHADERCOMBINE_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

protected:
	virtual void OnOK();
	virtual void OnCancel();

public:
	afx_msg void OnSelChangeDXNum();
	afx_msg void OnUpdateCheck1();
	afx_msg void OnSetFocusList1();
	afx_msg void OnUpdateCheck2();
	afx_msg void OnSetFocusList2();

	afx_msg void OnBnClickedAdd();
	afx_msg void OnBnClickedDel();
	afx_msg void OnBnClickedUp();
	afx_msg void OnBnClickedDown();
};

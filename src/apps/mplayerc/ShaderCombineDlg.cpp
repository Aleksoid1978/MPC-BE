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

#include "stdafx.h"
#include "MainFrm.h"
#include "ShaderCombineDlg.h"

// CShaderCombineDlg dialog

CShaderCombineDlg::CShaderCombineDlg(CWnd* pParent, const bool bD3D11)
	: CCmdUIDialog(CShaderCombineDlg::IDD, pParent)
	, m_bEnableD3D11(bD3D11)
{
	AfxGetMyApp()->GetAppSavePath(m_AppSavePath);
}

CShaderCombineDlg::~CShaderCombineDlg()
{
}

void CShaderCombineDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_COMBO2, m_cbDXNum);
	DDX_Control(pDX, IDC_COMBO1, m_cbShaders);

	DDX_Control(pDX, IDC_CHECK1, m_chEnable1);
	DDX_Control(pDX, IDC_LIST1,  m_cbList1);

	DDX_Control(pDX, IDC_CHECK2, m_chEnable2);
	DDX_Control(pDX, IDC_LIST2,  m_cbList2);
}

BEGIN_MESSAGE_MAP(CShaderCombineDlg, CCmdUIDialog)
	ON_CBN_SELCHANGE(IDC_COMBO2, OnSelChangeDXNum)

	ON_BN_CLICKED(IDC_CHECK1, OnUpdateCheck1)
	ON_LBN_SETFOCUS(IDC_LIST1, OnSetFocusList1)

	ON_BN_CLICKED(IDC_CHECK2, OnUpdateCheck2)
	ON_LBN_SETFOCUS(IDC_LIST2, OnSetFocusList2)

	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedAdd)
	ON_BN_CLICKED(IDC_BUTTON3, OnBnClickedDel)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedUp)
	ON_BN_CLICKED(IDC_BUTTON4, OnBnClickedDown)
END_MESSAGE_MAP()

// CShaderCombineDlg message handlers

BOOL CShaderCombineDlg::OnInitDialog()
{
	__super::OnInitDialog();

	//AddAnchor(IDOK, TOP_RIGHT);
	//AddAnchor(IDCANCEL, TOP_RIGHT);

	CAppSettings& s = AfxGetAppSettings();

	// remember the initial state
	auto pFrame = AfxGetMainFrame();
	m_oldcheck1 = pFrame->m_bToggleShader;
	m_oldcheck2 = pFrame->m_bToggleShaderScreenSpace;
	m_oldlabels1    = s.ShaderList;
	m_oldlabels2    = s.ShaderListScreenSpace;
	m_oldlabels11_2 = s.Shaders11PostScale;

	m_chEnable1.SetCheck(m_oldcheck1 ? BST_CHECKED : BST_UNCHECKED);
	m_chEnable2.SetCheck(m_oldcheck2 ? BST_CHECKED : BST_UNCHECKED);

	if (!m_bEnableD3D11) {
		m_cbDXNum.EnableWindow(FALSE);
	}
	m_cbDXNum.AddString(L"DX9");
	m_cbDXNum.AddString(L"DX11");

	m_cbDXNum.SetCurSel(0);
	OnSelChangeDXNum();

	UpdateData(FALSE);

	return TRUE;
}

void CShaderCombineDlg::OnOK()
{
	__super::OnOK();
}

void CShaderCombineDlg::OnCancel()
{
	auto pFrame = AfxGetMainFrame();
	CAppSettings& s = AfxGetAppSettings();

	s.ShaderList = m_oldlabels1;
	pFrame->EnableShaders1(m_oldcheck1);

	s.ShaderListScreenSpace = m_oldlabels2;
	s.Shaders11PostScale    = m_oldlabels11_2;
	pFrame->EnableShaders2(m_oldcheck2);

	__super::OnCancel();
}

void CShaderCombineDlg::OnSelChangeDXNum()
{
	const int iSel = m_cbDXNum.GetCurSel();
	if (iSel == m_iLastSel) {
		return;
	}

	m_iLastSel = iSel;
	m_bD3D11 = (iSel == 1);
	CAppSettings& s = AfxGetAppSettings();

	CString path(m_AppSavePath);
	m_cbShaders.ResetContent();
	m_cbList1.ResetContent();
	m_cbList2.ResetContent();

	if (m_bD3D11) {
		m_cbList1.EnableWindow(FALSE);

		for (const auto& shader : s.Shaders11PostScale) {
			m_cbList2.AddString(shader);
		}
		m_cbList2.AddString(L"");

		path += L"Shaders11\\";
		m_cbList2.SetFocus();
	}
	else {
		m_cbList1.EnableWindow(TRUE);
		for (const auto& shader : s.ShaderList) {
			m_cbList1.AddString(shader);
		}
		m_cbList1.AddString(L"");

		for (const auto& shader : s.ShaderListScreenSpace) {
			m_cbList2.AddString(shader);
		}
		m_cbList2.AddString(L"");

		path += L"Shaders\\";
		m_cbList1.SetFocus();
	}

	if (::PathFileExistsW(path)) {
		WIN32_FIND_DATAW wfd;
		HANDLE hFile = FindFirstFileW(path + L"*.hlsl", &wfd);
		if (hFile != INVALID_HANDLE_VALUE) {
			do {
				CString filename(wfd.cFileName);
				filename.Truncate(filename.GetLength() - 5);
				m_cbShaders.AddString(filename);
			} while (FindNextFileW(hFile, &wfd));
			FindClose(hFile);
		}

		if (m_cbShaders.GetCount()) {
			m_cbShaders.SetCurSel(0);
			CorrectComboListWidth(m_cbShaders);
		}
	}
}

void CShaderCombineDlg::OnUpdateCheck1()
{
	UpdateData();

	AfxGetMainFrame()->EnableShaders1(!!m_chEnable1.GetCheck());
}

void CShaderCombineDlg::OnUpdateCheck2()
{
	UpdateData();

	AfxGetMainFrame()->EnableShaders2(!!m_chEnable2.GetCheck());
}

void CShaderCombineDlg::OnSetFocusList1()
{
	m_cbList2.SetCurSel(-1);

	if (m_cbList1.GetCurSel() < 0) {
		m_cbList1.SetCurSel(m_cbList1.GetCount()-1);
	}
}

void CShaderCombineDlg::OnSetFocusList2()
{
	m_cbList1.SetCurSel(-1);

	if (m_cbList2.GetCurSel() < 0) {
		m_cbList2.SetCurSel(m_cbList2.GetCount()-1);
	}
}

void CShaderCombineDlg::OnBnClickedAdd()
{
	int i = m_cbShaders.GetCurSel();

	if (i < 0) {
		return;
	}

	CString label;
	m_cbShaders.GetLBText(i, label);

	i = m_cbList1.GetCurSel();

	if (i >= 0) {
		m_cbList1.InsertString(i, label);
		UpdateShaders(SHADER1);
		return;
	}

	i = m_cbList2.GetCurSel();

	if (i >= 0) {
		m_cbList2.InsertString(i, label);
		UpdateShaders(m_bD3D11 ? SHADER11_2 : SHADER2);
		//return;
	}
}

void CShaderCombineDlg::OnBnClickedDel()
{
	int i = m_cbList1.GetCurSel();

	if (i >= 0 && i < m_cbList1.GetCount()-1) {
		m_cbList1.DeleteString(i);

		if (i == m_cbList1.GetCount()-1 && i > 0) {
			i--;
		}

		m_cbList1.SetCurSel(i);

		UpdateShaders(SHADER1);
		return;
	}

	i = m_cbList2.GetCurSel();

	if (i >= 0 && i < m_cbList2.GetCount()-1) {
		m_cbList2.DeleteString(i);

		if (i == m_cbList2.GetCount()-1 && i > 0) {
			i--;
		}

		m_cbList2.SetCurSel(i);

		UpdateShaders(m_bD3D11 ? SHADER11_2 : SHADER2);
		//return;
	}
}

void CShaderCombineDlg::OnBnClickedUp()
{
	int i = m_cbList1.GetCurSel();

	if (i >= 1 && i < m_cbList1.GetCount()-1) {
		CString label;
		m_cbList1.GetText(i, label);
		m_cbList1.DeleteString(i);
		i--;
		m_cbList1.InsertString(i, label);
		m_cbList1.SetCurSel(i);

		UpdateShaders(SHADER1);
		return;
	}

	i = m_cbList2.GetCurSel();

	if (i >= 1 && i < m_cbList2.GetCount()-1) {
		CString label;
		m_cbList2.GetText(i, label);
		m_cbList2.DeleteString(i);
		i--;
		m_cbList2.InsertString(i, label);
		m_cbList2.SetCurSel(i);

		UpdateShaders(m_bD3D11 ? SHADER11_2 : SHADER2);
		//return;
	}
}

void CShaderCombineDlg::OnBnClickedDown()
{
	int i = m_cbList1.GetCurSel();

	if (i >= 0 && i < m_cbList1.GetCount()-2) {
		CString label;
		m_cbList1.GetText(i, label);
		m_cbList1.DeleteString(i);
		i++;
		m_cbList1.InsertString(i, label);
		m_cbList1.SetCurSel(i);

		UpdateShaders(SHADER1);
		return;
	}

	i = m_cbList2.GetCurSel();

	if (i >= 0 && i < m_cbList2.GetCount()-2) {
		CString label;
		m_cbList2.GetText(i, label);
		m_cbList2.DeleteString(i);
		i++;
		m_cbList2.InsertString(i, label);
		m_cbList2.SetCurSel(i);

		UpdateShaders(m_bD3D11 ? SHADER11_2 : SHADER2);
		//return;
	}
}

// Update shaders

void CShaderCombineDlg::UpdateShaders(unsigned type)
{
	auto pFrame = AfxGetMainFrame();
	CAppSettings& s = AfxGetAppSettings();

	if (type & SHADER1) {
		s.ShaderList.clear();

		for (int i = 0, j = m_cbList1.GetCount()-1; i < j; i++) {
			CString label;
			m_cbList1.GetText(i, label);
			s.ShaderList.push_back(label);
		}

		pFrame->EnableShaders1(!!m_chEnable1.GetCheck());
	}

	if (type & SHADER2) {
		s.ShaderListScreenSpace.clear();

		for (int m = 0, n = m_cbList2.GetCount()-1; m < n; m++) {
			CString label;
			m_cbList2.GetText(m, label);
			s.ShaderListScreenSpace.push_back(label);
		}

		pFrame->EnableShaders2(!!m_chEnable2.GetCheck());
	}

	if (type & SHADER11_2) {
		s.Shaders11PostScale.clear();

		for (int m = 0, n = m_cbList2.GetCount() - 1; m < n; m++) {
			CString label;
			m_cbList2.GetText(m, label);
			s.Shaders11PostScale.push_back(label);
		}

		pFrame->EnableShaders2(!!m_chEnable2.GetCheck());
	}
}

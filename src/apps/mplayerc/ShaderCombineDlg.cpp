/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2017 see Authors.txt
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

CShaderCombineDlg::CShaderCombineDlg(CWnd* pParent)
	: CCmdUIDialog(CShaderCombineDlg::IDD, pParent)
	, m_fcheck1(FALSE)
	, m_fcheck2(FALSE)
{
}

CShaderCombineDlg::~CShaderCombineDlg()
{
}

void CShaderCombineDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Check(pDX, IDC_CHECK1, m_fcheck1);
	DDX_Control(pDX, IDC_LIST1, m_list1);

	DDX_Check(pDX, IDC_CHECK2, m_fcheck2);
	DDX_Control(pDX, IDC_LIST2, m_list2);

	DDX_Control(pDX, IDC_COMBO1, m_combo);
}

BEGIN_MESSAGE_MAP(CShaderCombineDlg, CCmdUIDialog)
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
	m_fcheck1 = m_oldcheck1 = pFrame->m_bToggleShader;
	m_fcheck2 = m_oldcheck2 = pFrame->m_bToggleShaderScreenSpace;
	m_oldlabels1.AddTailList(&s.ShaderList);
	m_oldlabels2.AddTailList(&s.ShaderListScreenSpace);

	CString path;
	if (AfxGetMyApp()->GetAppSavePath(path)) {
		path += L"Shaders\\";
		if (::PathFileExists(path)) {
			WIN32_FIND_DATA wfd;
			HANDLE hFile = FindFirstFile(path + L"*.hlsl", &wfd);
			if (hFile != INVALID_HANDLE_VALUE) {
				do {
					CString filename(wfd.cFileName);
					filename.Truncate(filename.GetLength() - 5);
					m_combo.AddString(filename);
				} while (FindNextFile(hFile, &wfd));
				FindClose(hFile);
			}
		}
	}

	if (m_combo.GetCount()) {
		m_combo.SetCurSel(0);
		CorrectComboListWidth(m_combo);
	}


	POSITION pos;

	pos = s.ShaderList.GetHeadPosition();
	while (pos) {
		m_list1.AddString(s.ShaderList.GetNext(pos));
	}
	m_list1.AddString(L"");

	pos = s.ShaderListScreenSpace.GetHeadPosition();
	while (pos) {
		m_list2.AddString(s.ShaderListScreenSpace.GetNext(pos));
	}
	m_list2.AddString(L"");

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

	s.ShaderList.RemoveAll();
	s.ShaderList.AddTailList(&m_oldlabels1);
	pFrame->EnableShaders1(m_oldcheck1);

	s.ShaderListScreenSpace.RemoveAll();
	s.ShaderListScreenSpace.AddTailList(&m_oldlabels2);
	pFrame->EnableShaders2(m_oldcheck2);

	__super::OnCancel();
}

void CShaderCombineDlg::OnUpdateCheck1()
{
	UpdateData();

	AfxGetMainFrame()->EnableShaders1(!!m_fcheck1);
}

void CShaderCombineDlg::OnUpdateCheck2()
{
	UpdateData();

	AfxGetMainFrame()->EnableShaders2(!!m_fcheck2);
}

void CShaderCombineDlg::OnSetFocusList1()
{
	m_list2.SetCurSel(-1);

	if (m_list1.GetCurSel() < 0) {
		m_list1.SetCurSel(m_list1.GetCount()-1);
	}
}

void CShaderCombineDlg::OnSetFocusList2()
{
	m_list1.SetCurSel(-1);

	if (m_list2.GetCurSel() < 0) {
		m_list2.SetCurSel(m_list2.GetCount()-1);
	}
}

void CShaderCombineDlg::OnBnClickedAdd()
{
	int i = m_combo.GetCurSel();

	if (i < 0) {
		return;
	}

	CString label;
	m_combo.GetLBText(i, label);

	i = m_list1.GetCurSel();

	if (i >= 0) {
		m_list1.InsertString(i, label);
		UpdateShaders(SHADER1);
		return;
	}

	i = m_list2.GetCurSel();

	if (i >= 0) {
		m_list2.InsertString(i, label);
		UpdateShaders(SHADER2);
		//return;
	}
}

void CShaderCombineDlg::OnBnClickedDel()
{
	int i = m_list1.GetCurSel();

	if (i >= 0 && i < m_list1.GetCount()-1) {
		m_list1.DeleteString(i);

		if (i == m_list1.GetCount()-1 && i > 0) {
			i--;
		}

		m_list1.SetCurSel(i);

		UpdateShaders(SHADER1);
		return;
	}

	i = m_list2.GetCurSel();

	if (i >= 0 && i < m_list2.GetCount()-1) {
		m_list2.DeleteString(i);

		if (i == m_list2.GetCount()-1 && i > 0) {
			i--;
		}

		m_list2.SetCurSel(i);

		UpdateShaders(SHADER2);
		//return;
	}
}

void CShaderCombineDlg::OnBnClickedUp()
{
	int i = m_list1.GetCurSel();

	if (i >= 1 && i < m_list1.GetCount()-1) {
		CString label;
		m_list1.GetText(i, label);
		m_list1.DeleteString(i);
		i--;
		m_list1.InsertString(i, label);
		m_list1.SetCurSel(i);

		UpdateShaders(SHADER1);
		return;
	}

	i = m_list2.GetCurSel();

	if (i >= 1 && i < m_list2.GetCount()-1) {
		CString label;
		m_list2.GetText(i, label);
		m_list2.DeleteString(i);
		i--;
		m_list2.InsertString(i, label);
		m_list2.SetCurSel(i);

		UpdateShaders(SHADER2);
		//return;
	}
}

void CShaderCombineDlg::OnBnClickedDown()
{
	int i = m_list1.GetCurSel();

	if (i >= 0 && i < m_list1.GetCount()-2) {
		CString label;
		m_list1.GetText(i, label);
		m_list1.DeleteString(i);
		i++;
		m_list1.InsertString(i, label);
		m_list1.SetCurSel(i);

		UpdateShaders(SHADER1);
		return;
	}

	i = m_list2.GetCurSel();

	if (i >= 0 && i < m_list2.GetCount()-2) {
		CString label;
		m_list2.GetText(i, label);
		m_list2.DeleteString(i);
		i++;
		m_list2.InsertString(i, label);
		m_list2.SetCurSel(i);

		UpdateShaders(SHADER2);
		//return;
	}
}

// Update shaders

void CShaderCombineDlg::UpdateShaders(unsigned char type)
{
	auto pFrame = AfxGetMainFrame();
	CAppSettings& s = AfxGetAppSettings();

	if (type & SHADER1) {
		s.ShaderList.RemoveAll();

		for (int i = 0, j = m_list1.GetCount()-1; i < j; i++) {
			CString label;
			m_list1.GetText(i, label);
			s.ShaderList.AddTail(label);
		}

		pFrame->EnableShaders1(!!m_fcheck1);
	}

	if (type & SHADER2) {
		s.ShaderListScreenSpace.RemoveAll();

		for (int m = 0, n = m_list2.GetCount()-1; m < n; m++) {
			CString label;
			m_list2.GetText(m, label);
			s.ShaderListScreenSpace.AddTail(label);
		}

		pFrame->EnableShaders2(!!m_fcheck2);
	}
}

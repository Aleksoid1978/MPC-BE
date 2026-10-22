/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2026 see Authors.txt
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
#include "ShaderAutoCompleteDlg.h"
#include "controls/DarkTheme.h"

// CShaderAutoCompleteDlg dialog

CShaderAutoCompleteDlg::CShaderAutoCompleteDlg(CWnd* pParent)
	: CResizableDialog(CShaderAutoCompleteDlg::IDD, pParent)
{
	m_text[0] = 0;
}

CShaderAutoCompleteDlg::~CShaderAutoCompleteDlg()
{
}

void CShaderAutoCompleteDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_LIST1, m_list);
}

BEGIN_MESSAGE_MAP(CShaderAutoCompleteDlg, CResizableDialog)
	ON_WM_SETFOCUS()
	ON_LBN_SELCHANGE(IDC_LIST1, OnLbnSelchangeList1)
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()

// CShaderAutoCompleteDlg message handlers

BOOL CShaderAutoCompleteDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();

	AddAnchor(IDC_LIST1, TOP_LEFT, BOTTOM_RIGHT);

	m_hToolTipWnd = CreateWindowExW(
						WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr, TTS_NOPREFIX | TTS_ALWAYSTIP,
						CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
						nullptr, nullptr, nullptr, nullptr);

	m_ti.hwnd = m_hWnd;
	m_ti.lpszText = m_text;

	::SendMessageW(m_hToolTipWnd, TTM_ADDTOOLW, 0, (LPARAM)&m_ti);
	::SendMessageW(m_hToolTipWnd, TTM_SETMAXTIPWIDTH, 0, (LPARAM)400);

	DarkTheme::ThemeDialog(GetSafeHwnd());

	return TRUE;
}

void CShaderAutoCompleteDlg::OnSetFocus(CWnd* pOldWnd)
{
	__super::OnSetFocus(pOldWnd);

	GetParent()->SetFocus();
}

void CShaderAutoCompleteDlg::OnLbnSelchangeList1()
{
	::SendMessageW(m_hToolTipWnd, TTM_TRACKACTIVATE, FALSE, (LPARAM)&m_ti);

	int i = m_list.GetCurSel();

	if (i < 0) {
		return;
	}

	if (auto hlslfunc = (hlslfunc_t*)m_list.GetItemData(i)) {

		wcscpy_s(m_ti.lpszText, std::size(m_text), hlslfunc->desc);
		CRect r;
		GetWindowRect(r);
		::SendMessageW(m_hToolTipWnd, TTM_UPDATETIPTEXTW, 0, (LPARAM)&m_ti);
		::SendMessageW(m_hToolTipWnd, TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(r.left, r.bottom+1));
		::SendMessageW(m_hToolTipWnd, TTM_TRACKACTIVATE, TRUE, (LPARAM)&m_ti);
	}
}

void CShaderAutoCompleteDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CResizableDialog::OnShowWindow(bShow, nStatus);

	if (!bShow) {
		::SendMessageW(m_hToolTipWnd, TTM_TRACKACTIVATE, FALSE, (LPARAM)&m_ti);
	}
}

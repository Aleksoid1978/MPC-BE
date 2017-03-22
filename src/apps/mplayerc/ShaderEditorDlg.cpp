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
#include "ShaderNewDlg.h"
#include "ShaderEditorDlg.h"

// CShaderEdit

CShaderEdit::CShaderEdit()
{
	m_acdlg.Create(CShaderAutoCompleteDlg::IDD, NULL);

	m_nEndChar = -1;
	m_nIDEvent = (UINT_PTR)-1;
}

CShaderEdit::~CShaderEdit()
{
	m_acdlg.DestroyWindow();
}

BOOL CShaderEdit::PreTranslateMessage(MSG* pMsg)
{
	if (m_acdlg.IsWindowVisible()
			&& pMsg->message == WM_KEYDOWN
			&& (pMsg->wParam == VK_UP || pMsg->wParam == VK_DOWN
				|| pMsg->wParam == VK_PRIOR || pMsg->wParam == VK_NEXT
				|| pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE)) {
		int i = m_acdlg.m_list.GetCurSel();

		if (pMsg->wParam == VK_RETURN && i >= 0) {
			CString str;
			m_acdlg.m_list.GetText(i, str);
			i = str.Find('(')+1;
			if (i > 0) {
				str = str.Left(i);
			}

			int nStartChar = 0, nEndChar = -1;
			GetSel(nStartChar, nEndChar);

			CString text;
			GetWindowText(text);
			while (nStartChar > 0 && _istalnum(text.GetAt(nStartChar-1))) {
				nStartChar--;
			}

			SetSel(nStartChar, nEndChar);
			ReplaceSel(str, TRUE);
		} else if (pMsg->wParam == VK_ESCAPE) {
			m_acdlg.ShowWindow(SW_HIDE);
			return GetParent()->PreTranslateMessage(pMsg);
		} else {
			m_acdlg.m_list.SendMessage(pMsg->message, pMsg->wParam, pMsg->lParam);
		}

		return TRUE;
	}

	return __super::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(CShaderEdit, CLineNumberEdit)
	ON_CONTROL_REFLECT(EN_UPDATE, OnUpdate)
	ON_WM_KILLFOCUS()
	ON_WM_TIMER()
END_MESSAGE_MAP()

void CShaderEdit::OnUpdate()
{
	if (m_nIDEvent == (UINT_PTR)-1) {
		m_nIDEvent = SetTimer(1, 100, NULL);
	}

	CString text;
	int nStartChar = 0, nEndChar = -1;
	GetSel(nStartChar, nEndChar);

	if (nStartChar == nEndChar) {
		GetWindowText(text);
		while (nStartChar > 0 && _istalnum(text.GetAt(nStartChar-1))) {
			nStartChar--;
		}
	}

	if (nStartChar < nEndChar) {
		text = text.Mid(nStartChar, nEndChar - nStartChar);
		text.TrimRight('(');
		text.MakeLower();

		m_acdlg.m_list.ResetContent();

		CString key, value;
		POSITION pos = m_acdlg.m_inst.GetStartPosition();

		while (pos) {
			POSITION cur = pos;
			m_acdlg.m_inst.GetNextAssoc(pos, key, value);

			if (key.Find(text) == 0) {
				CAtlList<CString> sl;
				Explode(value, sl, L'|', 2);
				if (sl.GetCount() != 2) {
					continue;
				}
				CString name = sl.RemoveHead();
				CString description = sl.RemoveHead();
				int i = m_acdlg.m_list.AddString(name);
				m_acdlg.m_list.SetItemDataPtr(i, cur);
			}
		}

		if (m_acdlg.m_list.GetCount() > 0) {
			int lineheight = GetLineHeight();

			CPoint p = PosFromChar(nStartChar);
			p.y += lineheight;
			ClientToScreen(&p);
			CRect r(p, CSize(100, 100));

			m_acdlg.MoveWindow(r);
			m_acdlg.SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
			m_acdlg.ShowWindow(SW_SHOWNOACTIVATE);

			m_nEndChar = nEndChar;

			return;
		}
	}

	m_acdlg.ShowWindow(SW_HIDE);
}

void CShaderEdit::OnKillFocus(CWnd* pNewWnd)
{
	CString text;
	GetWindowText(text);
	__super::OnKillFocus(pNewWnd);
	GetWindowText(text);

	m_acdlg.ShowWindow(SW_HIDE);
}

void CShaderEdit::OnTimer(UINT_PTR nIDEvent)
{
	if (m_nIDEvent == nIDEvent) {
		int nStartChar = 0, nEndChar = -1;
		GetSel(nStartChar, nEndChar);

		if (nStartChar != nEndChar || m_nEndChar != nEndChar) {
			m_acdlg.ShowWindow(SW_HIDE);
		}
	}

	__super::OnTimer(nIDEvent);
}

// CShaderEditorDlg dialog

CShaderEditorDlg::CShaderEditorDlg()
	: CResizableDialog(CShaderEditorDlg::IDD, NULL)
	, m_fSplitterGrabbed(false)
	, m_pPSC(NULL)
	, m_pShader(NULL)
{
}

CShaderEditorDlg::~CShaderEditorDlg()
{
	m_Font.DeleteObject();
	delete m_pPSC;
}

BOOL CShaderEditorDlg::Create(CWnd* pParent)
{
	if (!__super::Create(IDD, pParent)) {
		return FALSE;
	}

	if (CDPI* pDpi = dynamic_cast<CDPI*>(AfxGetMainWnd())) {
		LOGFONT lf = {};
		lf.lfHeight = -pDpi->PointsToPixels(8);
		lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;

		for (const auto &fontname : MonospaceFonts) {
			wcscpy_s(lf.lfFaceName, LF_FACESIZE, fontname);
			if (IsFontInstalled(fontname) && m_Font.CreateFontIndirectW(&lf)) {
				m_edSrcdata.SetFont(&m_Font);
				break;
			}
		}
	}

	AddAnchor(IDC_COMBO1, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COMBO2, BOTTOM_RIGHT);
	AddAnchor(IDC_EDIT1, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_EDIT2, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_BUTTON1, TOP_RIGHT);
	AddAnchor(IDC_BUTTON2, TOP_RIGHT);
	AddAnchor(IDC_BUTTON3, TOP_RIGHT);
	AddAnchor(IDC_BUTTON4, BOTTOM_RIGHT);

	m_edSrcdata.SetTabStops(16);

	SetMinTrackSize(CSize(250, 40));

	m_cbProfile.AddString(L"ps_2_0");
	m_cbProfile.AddString(L"ps_2_a");
	m_cbProfile.AddString(L"ps_2_b");
	m_cbProfile.AddString(L"ps_3_0");

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
					m_cbLabels.AddString(filename);
				} while (FindNextFile(hFile, &wfd));
				FindClose(hFile);
			}
		}
	}
	CorrectComboListWidth(m_cbLabels);

	return TRUE;
}

void CShaderEditorDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_COMBO1, m_cbLabels);
	DDX_Control(pDX, IDC_COMBO2, m_cbProfile);
	DDX_Control(pDX, IDC_EDIT1, m_edSrcdata);
	DDX_Control(pDX, IDC_EDIT2, m_edOutput);
}

bool CShaderEditorDlg::HitTestSplitter(CPoint p)
{
	CRect r, rs, ro;
	m_edSrcdata.GetWindowRect(&rs);
	m_edOutput.GetWindowRect(&ro);
	ScreenToClient(&rs);
	ScreenToClient(&ro);
	GetClientRect(&r);
	r.left = ro.left;
	r.right = ro.right;
	r.top = rs.bottom;
	r.bottom = ro.top;
	return !!r.PtInRect(p);
}

BEGIN_MESSAGE_MAP(CShaderEditorDlg, CResizableDialog)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnCbnSelchangeCombo1)
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedButtonSave)
	ON_BN_CLICKED(IDC_BUTTON3, OnBnClickedButtonNew)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButtonDelete)
	ON_BN_CLICKED(IDC_BUTTON4, OnBnClickedButtonApply)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()

// CShaderEditorDlg message handlers

BOOL CShaderEditorDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_TAB
			   && pMsg->hwnd == m_edSrcdata.GetSafeHwnd()) {
		int nStartChar, nEndChar;
		m_edSrcdata.GetSel(nStartChar, nEndChar);
		if (nStartChar == nEndChar) {
			m_edSrcdata.ReplaceSel(L"\t");
		}
		return TRUE;
	} else if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE) {
		return GetParent()->PreTranslateMessage(pMsg);
	}

	return __super::PreTranslateMessage(pMsg);
}

void CShaderEditorDlg::OnCbnSelchangeCombo1()
{
	int i = m_cbLabels.GetCurSel();
	if (i >= 0) {
		CString label;
		m_cbLabels.GetLBText(i, label);

		ShaderC* pShader = AfxGetMainFrame()->GetShader(label);

		m_cbProfile.SelectString(0, pShader->profile);

		CString srcdata(pShader->srcdata);
		srcdata.Replace(L"\n", L"\r\n");
		m_edSrcdata.SetWindowText(srcdata);

		m_edOutput.SetWindowText(L"");
	}
}

void CShaderEditorDlg::OnBnClickedButtonSave()
{
	int i = m_cbLabels.GetCurSel();
	if (i >= 0) {
		ShaderC shader;

		m_cbLabels.GetLBText(i, shader.label);
		m_cbProfile.GetLBText(m_cbProfile.GetCurSel(), shader.profile);

		m_edSrcdata.GetWindowText(shader.srcdata);
		shader.srcdata.Remove('\r');

		bool ret = AfxGetMainFrame()->SaveShaderFile(&shader);
	}
}

void CShaderEditorDlg::OnBnClickedButtonNew()
{
	CShaderNewDlg dlg;
	if (IDOK != dlg.DoModal()) {
		return;
	}


	// if shader already exists, then select it
	int i = m_cbLabels.SelectString(0, dlg.m_Name);
	if (i > 0) {
		return;
	}

	CStringA srcdata;
	if (!LoadResource(IDF_SHADER_EMPTY, srcdata, L"FILE")) {
		return;
	}

	CString path;
	if (AfxGetMyApp()->GetAppSavePath(path)) {
		path.AppendFormat(L"Shaders\\%s.hlsl", dlg.m_Name);

		CStdioFile file;
		if (file.Open(path, CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive|CFile::typeBinary)) {
			file.Write(srcdata.GetBuffer(), srcdata.GetLength());
			file.Close();

			m_cbLabels.AddString(dlg.m_Name);
			m_cbLabels.SelectString(0, dlg.m_Name);
			OnCbnSelchangeCombo1();
		}
	}
}

void CShaderEditorDlg::OnBnClickedButtonDelete()
{
	int i = m_cbLabels.GetCurSel();
	if (i >= 0) {
		if (IDYES != AfxMessageBox(ResStr(IDS_SHADEREDITORDLG_0), MB_YESNO)) {
			return;
		}

		CString label;
		m_cbLabels.GetLBText(i, label);

		if (AfxGetMainFrame()->DeleteShaderFile(label)) {
			m_cbLabels.DeleteString(i);
			m_cbLabels.SetCurSel(-1);

			m_edSrcdata.SetWindowText(L"");
			m_edOutput.SetWindowText(L"");
		}
	}
}

void CShaderEditorDlg::OnBnClickedButtonApply()
{
	int i = m_cbLabels.GetCurSel();
	if (i >= 0) {
		CString str;
		m_cbProfile.GetLBText(m_cbProfile.GetCurSel(), str);
		CStringA profile(str);

		m_edSrcdata.GetWindowTextW(str);
		str.Remove('\r');
		CStringA srcdata(str);

		if (srcdata.GetLength() && profile.GetLength()) {
			if (!m_pPSC) {
				m_pPSC = DNew CPixelShaderCompiler(NULL);
			}

			CString disasm, errmsg;
			HRESULT hr = m_pPSC->CompileShader(srcdata, "main", profile, D3DCOMPILE_DEBUG, NULL, NULL, &errmsg, &disasm);

			if (SUCCEEDED(hr)) {
				errmsg = L"D3DXCompileShader succeeded\n";
				errmsg += L"\n";
				errmsg += disasm;

				//if (AfxGetMainFrame()->m_pCAP) {
				//	// add the shader to the existing ones. it will be removed from the list after restarting playback or after changing the main shader list
				//	hr = AfxGetMainFrame()->m_pCAP->AddPixelShader(TARGET_FRAME, srcdata, profile);
				//}
			}

			errmsg.Replace(L"\n", L"\r\n");
			m_edOutput.SetWindowText(errmsg);
		}
	}
}

void CShaderEditorDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (HitTestSplitter(point)) {
		m_fSplitterGrabbed = true;
		SetCapture();
	}

	__super::OnLButtonDown(nFlags, point);
}

void CShaderEditorDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_fSplitterGrabbed) {
		ReleaseCapture();
		m_fSplitterGrabbed = false;
	}

	__super::OnLButtonUp(nFlags, point);
}

void CShaderEditorDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_fSplitterGrabbed) {
		CRect r, rs, ro;
		GetClientRect(&r);
		m_edSrcdata.GetWindowRect(&rs);
		m_edOutput.GetWindowRect(&ro);
		ScreenToClient(&rs);
		ScreenToClient(&ro);

		int dist = ro.top - rs.bottom;
		int avgdist = dist / 2;

		rs.bottom = min(max(point.y, rs.top + 40), ro.bottom - 40) - avgdist;
		ro.top = rs.bottom + dist;
		m_edSrcdata.MoveWindow(&rs);
		m_edOutput.MoveWindow(&ro);

		int div = 100 * ((rs.bottom + ro.top) / 2) / (ro.bottom - rs.top);

		RemoveAnchor(IDC_EDIT1);
		RemoveAnchor(IDC_EDIT2);
		AddAnchor(IDC_EDIT1, TOP_LEFT, CSize(100, div)/*BOTTOM_RIGHT*/);
		AddAnchor(IDC_EDIT2, CSize(0, div)/*BOTTOM_LEFT*/, BOTTOM_RIGHT);
	}

	__super::OnMouseMove(nFlags, point);
}

BOOL CShaderEditorDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	CPoint p;
	GetCursorPos(&p);
	ScreenToClient(&p);

	if (HitTestSplitter(p)) {
		::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZENS));
		return TRUE;
	}

	return __super::OnSetCursor(pWnd, nHitTest, message);
}

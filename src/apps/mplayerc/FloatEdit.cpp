/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2014 see Authors.txt
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
#include "FloatEdit.h"


// CFloatEdit

IMPLEMENT_DYNAMIC(CFloatEdit, CEdit)

bool CFloatEdit::GetFloat(float& f)
{
	CString s;
	GetWindowText(s);

	return(_stscanf_s(s, _T("%f"), &f) == 1);
}

double CFloatEdit::operator = (double d)
{
	CString s;
	s.Format(_T("%.4f"), d);
	s.TrimRight('0');
	if (s[s.GetLength() - 1] == '.') s.Truncate(s.GetLength() - 1);
	SetWindowText(s);

	return(d);
}

CFloatEdit::operator double()
{
	CString s;
	GetWindowText(s);
	float f = 0;

	return(_stscanf_s(s, _T("%f"), &f) == 1 ? f : 0);
}

BEGIN_MESSAGE_MAP(CFloatEdit, CEdit)
	ON_WM_CHAR()
	ON_MESSAGE(WM_PASTE, OnPaste)
END_MESSAGE_MAP()

void CFloatEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar >= '0' && nChar <= '9' || nChar == '-' || nChar == '.') {
		CString str;
		GetWindowText(str);
		int nStartChar, nEndChar;
		GetSel(nStartChar, nEndChar);

		CEdit::OnChar(nChar, nRepCnt, nFlags);

		CString s;
		GetWindowText(s);
		float f = 0;
		wchar_t ch;
		if (_stscanf_s(s, L"%f%c", &f, &ch) != 1 && s != "-") {
			SetWindowText(str);
			SetSel(nStartChar, nEndChar);
		};
	}
	else if (nChar == VK_BACK
			|| nChar == 0x01 // Ctrl+A
			|| nChar == 0x03 // Ctrl+C
			|| nChar == 0x16 // Ctrl+V
			|| nChar == 0x18) { // Ctrl+X
		CEdit::OnChar(nChar, nRepCnt, nFlags);
	}
}

LRESULT CFloatEdit::OnPaste(WPARAM wParam, LPARAM lParam)
{
	CString str;
	GetWindowText(str);
	int nStartChar, nEndChar;
	GetSel(nStartChar, nEndChar);

	LRESULT lr = DefWindowProc(WM_PASTE, wParam, lParam);

	if (lr == 1) {
		CString s;
		GetWindowText(s);
		float f = 0;
		wchar_t ch;
		if (_stscanf_s(s, L"%f%c", &f, &ch) != 1) {
			SetWindowText(str);
			SetSel(nStartChar, nEndChar);
		};
	}

	return lr;
}

// CIntEdit

IMPLEMENT_DYNAMIC(CIntEdit, CEdit)

bool CIntEdit::GetInt(int& integer)
{
	CString s;
	GetWindowText(s);

	return(_stscanf_s(s, _T("%d"), &integer) == 1);
}

int CIntEdit::operator = (int integer)
{
	CString s;
	s.Format(_T("%d"), integer);
	SetWindowText(s);

	return(integer);
}

CIntEdit::operator int()
{
	CString s;
	GetWindowText(s);
	int integer;

	return(_stscanf_s(s, _T("%d"), &integer) == 1 ? integer : 0);
}

void CIntEdit::SetRange(int nLower, int nUpper)
{
	ASSERT(nLower < nUpper);
	m_lower = nLower;
	m_upper = nUpper;
}

BEGIN_MESSAGE_MAP(CIntEdit, CEdit)
	ON_WM_CHAR()
	ON_MESSAGE(WM_PASTE, OnPaste)
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

void CIntEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar >= '0' && nChar <= '9' || nChar == '-') {
		CString str;
		GetWindowText(str);
		int nStartChar, nEndChar;
		GetSel(nStartChar, nEndChar);

		CEdit::OnChar(nChar, nRepCnt, nFlags);

		CString s;
		GetWindowText(s);
		int d = 0;
		wchar_t ch;
		if (_stscanf_s(s, L"%d%c", &d, &ch) != 1 && s != "-") {
			SetWindowText(str);
			SetSel(nStartChar, nEndChar);
		};
	}
	else if (nChar == VK_BACK
			|| nChar == 0x01 // Ctrl+A
			|| nChar == 0x03 // Ctrl+C
			|| nChar == 0x16 // Ctrl+V
			|| nChar == 0x18) { // Ctrl+X
		CEdit::OnChar(nChar, nRepCnt, nFlags);
	}
}

LRESULT CIntEdit::OnPaste(WPARAM wParam, LPARAM lParam)
{
	CString str;
	GetWindowText(str);
	int nStartChar, nEndChar;
	GetSel(nStartChar, nEndChar);

	LRESULT lr = DefWindowProc(WM_PASTE, wParam, lParam);

	if (lr == 1) {
		CString s;
		GetWindowText(s);
		int d = 0;
		wchar_t ch;
		if (_stscanf_s(s, L"%d%c", &d, &ch) != 1) {
			SetWindowText(str);
			SetSel(nStartChar, nEndChar);
		};
	}

	return lr;
}

void CIntEdit::OnKillFocus (CWnd* pNewWnd)
{
	CString s;
	GetWindowText(s);
	int integer;

	if (_stscanf_s(s, _T("%d"), &integer) != 1) {
		integer = 0;
	}

	if (integer > m_upper) { integer = m_upper; }
	else if (integer < m_lower) { integer = m_lower; }

	s.Format(_T("%d"), integer);
	SetWindowText(s);

	CEdit::OnKillFocus(pNewWnd);
}

// CHexEdit

IMPLEMENT_DYNAMIC(CHexEdit, CEdit)

bool CHexEdit::GetDWORD(DWORD& dw)
{
	CString s;
	GetWindowText(s);

	return(_stscanf_s(s, _T("%x"), &dw) == 1);
}

DWORD CHexEdit::operator = (DWORD dw)
{
	CString s;
	s.Format(_T("%08lx"), dw);
	SetWindowText(s);

	return(dw);
}

CHexEdit::operator DWORD()
{
	CString s;
	GetWindowText(s);
	DWORD dw;

	return(_stscanf_s(s, _T("%x"), &dw) == 1 ? dw : 0);
}

BEGIN_MESSAGE_MAP(CHexEdit, CEdit)
	ON_WM_CHAR()
	ON_MESSAGE(WM_PASTE, OnPaste)
END_MESSAGE_MAP()

void CHexEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar >= '0' && nChar <= '9'
			|| nChar >= 'A' && nChar <= 'F'
			|| nChar >= 'a' && nChar <= 'f') {
		CString str;
		GetWindowText(str);
		int nStartChar, nEndChar;
		GetSel(nStartChar, nEndChar);

		CEdit::OnChar(nChar, nRepCnt, nFlags);

		CString s;
		GetWindowText(s);
		DWORD x = 0;
		wchar_t ch;
		if (_stscanf_s(s, L"%x%c", &x, &ch) != 1) {
			SetWindowText(str);
			SetSel(nStartChar, nEndChar);
		};
	}
	else if (nChar == VK_BACK
			|| nChar == 0x01 // Ctrl+A
			|| nChar == 0x03 // Ctrl+C
			|| nChar == 0x16 // Ctrl+V
			|| nChar == 0x18) { // Ctrl+X
		CEdit::OnChar(nChar, nRepCnt, nFlags);
	}
}

LRESULT CHexEdit::OnPaste(WPARAM wParam, LPARAM lParam)
{
	CString str;
	GetWindowText(str);
	int nStartChar, nEndChar;
	GetSel(nStartChar, nEndChar);

	LRESULT lr = DefWindowProc(WM_PASTE, wParam, lParam);

	if (lr == 1) {
		CString s;
		GetWindowText(s);
		DWORD x = 0;
		wchar_t ch;
		if (_stscanf_s(s, L"%x%c", &x, &ch) != 1) {
			SetWindowText(str);
			SetSel(nStartChar, nEndChar);
		};
	}

	return lr;
}

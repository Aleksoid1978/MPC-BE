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
#include "FloatEdit.h"


// CFloatEdit

IMPLEMENT_DYNAMIC(CFloatEdit, CEdit)

bool CFloatEdit::GetFloat(float& f)
{
	CString s;
	GetWindowTextW(s);

	return (swscanf_s(s, L"%f", &f) == 1);
}

double CFloatEdit::operator = (double d)
{
	CString s;
	s.Format(L"%.4f", d);
	s.TrimRight('0');
	if (s[s.GetLength() - 1] == '.') s.Truncate(s.GetLength() - 1);
	SetWindowTextW(s);

	return d;
}

CFloatEdit::operator double()
{
	CString s;
	GetWindowTextW(s);
	float f = 0;

	return (swscanf_s(s, L"%f", &f) == 1 ? f : 0);
}

BEGIN_MESSAGE_MAP(CFloatEdit, CEdit)
	ON_WM_CHAR()
	ON_MESSAGE(WM_PASTE, OnPaste)
END_MESSAGE_MAP()

void CFloatEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar >= '0' && nChar <= '9' || nChar == '-' || nChar == '.') {
		CString str;
		GetWindowTextW(str);
		int nStartChar, nEndChar;
		GetSel(nStartChar, nEndChar);

		CEdit::OnChar(nChar, nRepCnt, nFlags);

		CString s;
		GetWindowTextW(s);
		float f = 0;
		wchar_t ch;
		if (swscanf_s(s, L"%f%c", &f, &ch, 1) != 1 && s != "-") {
			SetWindowTextW(str);
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
	GetWindowTextW(str);
	int nStartChar, nEndChar;
	GetSel(nStartChar, nEndChar);

	LRESULT lr = DefWindowProcW(WM_PASTE, wParam, lParam);

	if (lr == 1) {
		CString s;
		GetWindowTextW(s);
		float f = 0;
		wchar_t ch;
		if (swscanf_s(s, L"%f%c", &f, &ch, 1) != 1) {
			SetWindowTextW(str);
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
	GetWindowTextW(s);

	return (swscanf_s(s, L"%d", &integer) == 1);
}

int CIntEdit::operator = (int integer)
{
	CString s;
	s.Format(L"%d", integer);
	SetWindowTextW(s);

	return integer;
}

CIntEdit::operator int()
{
	CString s;
	GetWindowTextW(s);
	int integer;
	if (swscanf_s(s, L"%d", &integer) != 1) {
		integer = 0;
	}
	integer = clamp(integer, m_lower, m_upper); // correction value after the simultaneous input and closing

	return integer;
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
		GetWindowTextW(str);
		int nStartChar, nEndChar;
		GetSel(nStartChar, nEndChar);

		CEdit::OnChar(nChar, nRepCnt, nFlags);

		CString s;
		GetWindowTextW(s);
		int d = 0;
		wchar_t ch;
		if (swscanf_s(s, L"%d%c", &d, &ch, 1) != 1 && s != "-") {
			SetWindowTextW(str);
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
	GetWindowTextW(str);
	int nStartChar, nEndChar;
	GetSel(nStartChar, nEndChar);

	LRESULT lr = DefWindowProcW(WM_PASTE, wParam, lParam);

	if (lr == 1) {
		CString s;
		GetWindowTextW(s);
		int d = 0;
		wchar_t ch;
		if (swscanf_s(s, L"%d%c", &d, &ch, 1) != 1) {
			SetWindowTextW(str);
			SetSel(nStartChar, nEndChar);
		};
	}

	return lr;
}

void CIntEdit::OnKillFocus (CWnd* pNewWnd)
{
	CString s;
	GetWindowTextW(s);
	int integer;

	if (swscanf_s(s, L"%d", &integer) != 1) {
		integer = 0;
	}
	integer = clamp(integer, m_lower, m_upper);

	s.Format(L"%d", integer);
	SetWindowTextW(s);

	CEdit::OnKillFocus(pNewWnd);
}

// CHexEdit

IMPLEMENT_DYNAMIC(CHexEdit, CEdit)

bool CHexEdit::GetDWORD(DWORD& dw)
{
	CString s;
	GetWindowTextW(s);

	return (swscanf_s(s, L"%x", &dw) == 1);
}

DWORD CHexEdit::operator = (DWORD dw)
{
	CString s;
	s.Format(L"%08lx", dw);
	SetWindowTextW(s);

	return dw;
}

CHexEdit::operator DWORD()
{
	CString s;
	GetWindowTextW(s);
	DWORD dw;

	return (swscanf_s(s, L"%x", &dw) == 1 ? dw : 0);
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
		GetWindowTextW(str);
		int nStartChar, nEndChar;
		GetSel(nStartChar, nEndChar);

		CEdit::OnChar(nChar, nRepCnt, nFlags);

		CString s;
		GetWindowTextW(s);
		DWORD x = 0;
		wchar_t ch;
		if (swscanf_s(s, L"%x%c", &x, &ch, 1) != 1) {
			SetWindowTextW(str);
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
	GetWindowTextW(str);
	int nStartChar, nEndChar;
	GetSel(nStartChar, nEndChar);

	LRESULT lr = DefWindowProcW(WM_PASTE, wParam, lParam);

	if (lr == 1) {
		CString s;
		GetWindowTextW(s);
		DWORD x = 0;
		wchar_t ch;
		if (swscanf_s(s, L"%x%c", &x, &ch, 1) != 1) {
			SetWindowTextW(str);
			SetSel(nStartChar, nEndChar);
		};
	}

	return lr;
}

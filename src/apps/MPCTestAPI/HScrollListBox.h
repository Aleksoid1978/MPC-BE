/*
 * (C) 2008-2017 see Authors.txt
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

// CHScrollListBox window

class CHScrollListBox : public CListBox
{
	// Construction
public:
	CHScrollListBox();

	// Attributes
public:

	// Operations
public:

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHScrollListBox)
protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CHScrollListBox();

	// Generated message map functions
protected:
	//{{AFX_MSG(CHScrollListBox)
	// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	afx_msg LRESULT OnAddString(WPARAM wParam, LPARAM lParam); // wParam - none, lParam - string, returns - int
	afx_msg LRESULT OnInsertString(WPARAM wParam, LPARAM lParam); // wParam - index, lParam - string, returns - int
	afx_msg LRESULT OnDeleteString(WPARAM wParam, LPARAM lParam); // wParam - index, lParam - none, returns - int
	afx_msg LRESULT OnResetContent(WPARAM wParam, LPARAM lParam); // wParam - none, lParam - none, returns - int
	afx_msg LRESULT OnDir(WPARAM wParam, LPARAM lParam); // wParam - attr, lParam - wildcard, returns - int

	DECLARE_MESSAGE_MAP()

private:
	void ResetHExtent();
	void SetNewHExtent(LPCWSTR lpszNewString);
	int GetTextLen(LPCWSTR lpszText);
};

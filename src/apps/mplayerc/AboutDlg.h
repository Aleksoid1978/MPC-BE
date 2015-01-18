/*
 * (C) 2014 see Authors.txt
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

class CAboutDlg : public CDialog
{
	CString m_appname;
	CString m_strVersionNumber;
	CString m_strSVNNumber;
	CString m_MPCCompiler;
	CString m_FFmpegCompiler;
	CString m_libavcodecVersion;
	CString m_Credits;
	CString m_AuthorsPath;

	HICON	m_hIcon;

public:
	CAboutDlg();
	virtual ~CAboutDlg();

	virtual BOOL OnInitDialog();

	afx_msg void OnHomepage(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnAuthors(NMHDR *pNMHDR, LRESULT *pResult);

	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

	// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

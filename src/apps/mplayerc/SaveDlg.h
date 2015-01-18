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

#pragma once

#include <afxcmn.h>
#include <afxwin.h>
#include <afxtaskdialog.h>
#include <ResizableLib/ResizableDialog.h>

// CSaveDlg dialog

class CSaveDlg : public CCmdUIDialog
{
	DECLARE_DYNAMIC(CSaveDlg)

private:
	CString m_in, m_name, m_out;
	CComPtr<IGraphBuilder> pGB;
	CComQIPtr<IMediaControl> pMC;
	CComQIPtr<IMediaEventEx> pME;
	CComQIPtr<IMediaSeeking> pMS;
	UINT_PTR m_nIDTimerEvent;

public:
	CSaveDlg(CString in, CString name, CString out, CWnd* pParent = NULL);
	virtual ~CSaveDlg();

	enum { IDD = IDD_SAVE_DLG };
	CAnimateCtrl m_anim;
	CProgressCtrl m_progress;
	CStatic m_report;
	CStatic m_fromto;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCancel();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg LRESULT OnGraphNotify(WPARAM wParam, LPARAM lParam);
};

// CSaveTaskDlg dialog

class CSaveTaskDlg : public CTaskDialog
{
	DECLARE_DYNAMIC(CSaveTaskDlg)

private:
	CString m_in, m_out;
	CComPtr<IGraphBuilder> pGB;
	CComQIPtr<IMediaControl> pMC;
	CComQIPtr<IMediaEventEx> pME;
	CComQIPtr<IMediaSeeking> pMS;
	HICON	m_hIcon;
	HWND	m_TaskDlgHwnd;

public:
	CSaveTaskDlg(CString in, CString name, CString out);
	virtual ~CSaveTaskDlg();

protected:
	virtual HRESULT OnInit();
	virtual HRESULT OnTimer(_In_ long lTime);

	HRESULT InitFileCopy();
};

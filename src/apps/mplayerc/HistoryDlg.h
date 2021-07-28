/*
 * (C) 2021 see Authors.txt
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
#include <ExtLib/ui/ResizableLib/ResizableDialog.h>

// CHistoryDlg dialog

class CHistoryDlg : public CResizableDialog
{
	//DECLARE_DYNAMIC(CHistoryDlg)

private:
	enum {
		COL_PATH = 0,
		COL_TITLE,
		COL_POS,
		COL_COUNT
	};
	enum { IDD = IDD_HISTORY };
	CListCtrl m_list;
	std::vector<SessionInfo> m_recentSessions;

	UINT_PTR m_nFilterTimerID;
	CEdit m_FilterEdit;
	CButton m_DelSelButton;
	CButton m_ClearButton;

public:
	CHistoryDlg(CWnd* pParent = nullptr);
	virtual ~CHistoryDlg();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	void SetupList();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnChangeFilterEdit();
	afx_msg void OnDelSelBnClicked();
	afx_msg void OnClearBnClicked();
};

/*
 * (C) 2021-2024 see Authors.txt
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

class CExListCtrl : public CListCtrl
{
	BOOL PreTranslateMessage(MSG* pMsg) override
	{
		if (pMsg->message == WM_CHAR) {
			WCHAR chr = static_cast<WCHAR>(pMsg->wParam);
			switch (chr) {
			case 0x01: // Ctrl-A
				for (int i = 0, count = GetItemCount(); i < count; i++) {
					SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
				}
				break;
			}
		}

		return CListCtrl::PreTranslateMessage(pMsg);
	}
};

// CHistoryDlg dialog

class CHistoryDlg : public CResizableDialog
{
	//DECLARE_DYNAMIC(CHistoryDlg)

private:
	enum { IDD = IDD_HISTORY };

	enum {
		COL_PATH = 0,
		COL_TITLE,
		COL_POS,
		COL_COUNT
	};

	CExListCtrl m_list;
	std::vector<SessionInfo> m_recentSessions;

	UINT_PTR m_nFilterTimerID = 0;
	CEdit m_FilterEdit;
	CButton m_DelSelButton;

public:
	CHistoryDlg(CWnd* pParent = nullptr);
	virtual ~CHistoryDlg();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	void SetupList();
	void CopySelectedPaths();
	void RemoveFromJumpList(const std::list<SessionInfo>& sessions);
	void RemoveSelected();
	int  RemoveMissingFiles();
	void ClearHistory();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnChangeFilterEdit();
	afx_msg void OnBnClickedMenu();
	afx_msg void OnBnClickedRemoveSel();
	afx_msg void OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeydownList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClose();
};

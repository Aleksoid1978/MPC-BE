/*
 * (C) 2020-2021 see Authors.txt
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

#include "PPageBase.h"


// CPPageMouse dialog

class CPPageMouse : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageMouse)

	CComboBox m_cmbLeftButtonClick;
	CComboBox m_cmbLeftButtonDblClick;
	CComboBox m_cmbRightButtonClick;

	CButton m_chkMouseLeftClickOpenRecent;

	CPlayerListCtrl m_list;
	enum {
		ROW_BTN_M,
		ROW_BTN_X1,
		ROW_BTN_X2,
		ROW_WHL_U,
		ROW_WHL_D,
		ROW_WHL_L,
		ROW_WHL_R,
		ROW_COUNT
	};
	enum {
		COL_ACTION = 0,
		COL_CMD,
		COL_CTRL,
		COL_SHIFT,
		COL_RBTN,
		COL_COUNT
	};

	struct MOUSE_COMMANDS {
		std::vector<WORD> ids;
		std::list<CString> str_list;
		bool bEllipsisEnd = false;

		void Add(const WORD id)
		{
			if (id == 0) {
				ids.emplace_back(id);
				str_list.emplace_back(L"");
			} else {
				auto& wmcmds = AfxGetAppSettings().wmcmds;
				for (const auto& wc : wmcmds) {
					if (id == wc.cmd) {
						ids.emplace_back(id);
						if (bEllipsisEnd) {
							str_list.emplace(--str_list.end(), ResStr(wc.dwname));
						} else {
							str_list.emplace_back(ResStr(wc.dwname));
						}
						break;
					}
				}
			}
		}

		void AddEllipsisEnd()
		{
			if (!bEllipsisEnd) {
				str_list.emplace_back(L"...");
				bEllipsisEnd = true;
			}
		}
	};

	UINT m_table_values[ROW_COUNT][COL_COUNT] = {};

	MOUSE_COMMANDS m_comands_M; // Middle
	MOUSE_COMMANDS m_comands_X; // X1, X2
	MOUSE_COMMANDS m_comands_W; // Wheel Up/Down/Left/Right

	MOUSE_COMMANDS* m_table_comands[ROW_COUNT][COL_COUNT] = {
		{nullptr, &m_comands_M, &m_comands_M, &m_comands_M, &m_comands_M}, // ROW_BTN_M
		{nullptr, &m_comands_X, &m_comands_X, &m_comands_X, &m_comands_X}, // ROW_BTN_X1
		{nullptr, &m_comands_X, &m_comands_X, &m_comands_X, &m_comands_X}, // ROW_BTN_X2
		{nullptr, &m_comands_W, &m_comands_W, &m_comands_W, &m_comands_W}, // ROW_WHL_U
		{nullptr, &m_comands_W, &m_comands_W, &m_comands_W, &m_comands_W}, // ROW_WHL_D
		{nullptr, &m_comands_W, &m_comands_W, &m_comands_W, &m_comands_W}, // ROW_WHL_L
		{nullptr, &m_comands_W, &m_comands_W, &m_comands_W, &m_comands_W}, // ROW_WHL_R
	};

	LPCWSTR GetCmdString(MOUSE_COMMANDS* mouse_cmds, const WORD id);
	void SyncList();

public:
	CPPageMouse();
	virtual ~CPPageMouse();

	enum { IDD = IDD_PPAGEMOUSE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnLeftClickChange();
	afx_msg void OnBeginlabeleditList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDolabeleditList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndlabeleditList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedReset();
};

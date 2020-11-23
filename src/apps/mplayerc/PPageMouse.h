/*
 * (C) 2020 see Authors.txt
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

	CComboBox m_cmbLeftBottonClick;
	CComboBox m_cmbLeftBottonDblClick;

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

		void Add(WORD id, UINT str_id) {
			ASSERT(ids.size() == str_list.size());
			ids.emplace_back(id);
			if (str_id) {
				str_list.emplace_back(ResStr(str_id));
			} else {
				str_list.emplace_back(L"");
			}
		}
	};

	UINT m_table_values[ROW_COUNT][COL_COUNT] = {};

	//MOUSE_COMMANDS m_comands_1;
	//MOUSE_COMMANDS m_comands_2;
	MOUSE_COMMANDS m_comands_3; // Middle
	MOUSE_COMMANDS m_comands_4; // X1, X2
	MOUSE_COMMANDS m_comands_5; // Wheel Up/Down
	MOUSE_COMMANDS m_comands_6; // Wheel Left/Right

	MOUSE_COMMANDS* m_table_comands[ROW_COUNT][COL_COUNT] = {
		{nullptr, &m_comands_3, &m_comands_3, &m_comands_3, &m_comands_3}, // ROW_BTN_M
		{nullptr, &m_comands_4, &m_comands_4, &m_comands_4, &m_comands_4}, // ROW_BTN_X1
		{nullptr, &m_comands_4, &m_comands_4, &m_comands_4, &m_comands_4}, // ROW_BTN_X2
		{nullptr, &m_comands_5, &m_comands_5, &m_comands_5, &m_comands_5}, // ROW_WHL_U
		{nullptr, &m_comands_5, &m_comands_5, &m_comands_5, &m_comands_5}, // ROW_WHL_D
		{nullptr, &m_comands_6, &m_comands_6, &m_comands_6, &m_comands_6}, // ROW_WHL_L
		{nullptr, &m_comands_6, &m_comands_6, &m_comands_6, &m_comands_6}, // ROW_WHL_R
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
	afx_msg void OnBeginlabeleditList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDolabeleditList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndlabeleditList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedReset();
};

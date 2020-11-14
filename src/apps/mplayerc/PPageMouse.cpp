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

#include "stdafx.h"
#include "MainFrm.h"
#include "PPageMouse.h"

// CPPageMouse dialog

IMPLEMENT_DYNAMIC(CPPageMouse, CPPageBase)
CPPageMouse::CPPageMouse()
	: CPPageBase(CPPageMouse::IDD, CPPageMouse::IDD)
	, m_list(0)
{
	//m_comands_1.Add(0, 0);
	//m_comands_1.Add(ID_PLAY_PLAYPAUSE, IDS_AG_PLAYPAUSE);

	//m_comands_2.Add(0, 0);
	//m_comands_2.Add(ID_PLAY_PLAYPAUSE,  IDS_AG_PLAYPAUSE);
	//m_comands_2.Add(ID_VIEW_FULLSCREEN, IDS_AG_FULLSCREEN);

	m_comands_3.Add(0, 0);
	m_comands_3.Add(ID_PLAY_PLAYPAUSE,  IDS_AG_PLAYPAUSE);
	m_comands_3.Add(ID_VIEW_FULLSCREEN, IDS_AG_FULLSCREEN);
	m_comands_3.Add(ID_VIEW_PLAYLIST,   IDS_AG_TOGGLE_PLAYLIST);
	m_comands_3.Add(ID_BOSS,            IDS_AG_BOSS_KEY);

	m_comands_4.Add(0, 0);
	m_comands_4.Add(ID_NAVIGATE_SKIPFORWARD,     IDS_AG_NEXT);
	m_comands_4.Add(ID_NAVIGATE_SKIPBACK,        IDS_AG_PREVIOUS);
	m_comands_4.Add(ID_NAVIGATE_SKIPFORWARDFILE, IDS_AG_NEXT_FILE);
	m_comands_4.Add(ID_NAVIGATE_SKIPBACKFILE,    IDS_AG_PREVIOUS_FILE);
	m_comands_4.Add(ID_PLAY_SEEKBACKWARDMED,     IDS_AG_JUMP_BACKWARD_2);
	m_comands_4.Add(ID_PLAY_SEEKFORWARDMED,      IDS_AG_JUMP_FORWARD_2);

	m_comands_5.Add(0, 0);
	m_comands_5.Add(ID_VOLUME_UP,            IDS_AG_VOLUME_UP);
	m_comands_5.Add(ID_VOLUME_DOWN,          IDS_AG_VOLUME_DOWN);
	m_comands_5.Add(ID_PLAY_INCRATE,         IDS_AG_INCREASE_RATE);
	m_comands_5.Add(ID_PLAY_DECRATE,         IDS_AG_DECREASE_RATE);
	m_comands_5.Add(ID_PLAY_SEEKBACKWARDMED, IDS_AG_JUMP_BACKWARD_2);
	m_comands_5.Add(ID_PLAY_SEEKFORWARDMED,  IDS_AG_JUMP_FORWARD_2);

	m_comands_6.Add(0, 0);
	m_comands_6.Add(ID_PLAY_INCRATE,             IDS_AG_INCREASE_RATE);
	m_comands_6.Add(ID_PLAY_DECRATE,             IDS_AG_DECREASE_RATE);
	m_comands_6.Add(ID_NAVIGATE_SKIPFORWARDFILE, IDS_AG_NEXT_FILE);
	m_comands_6.Add(ID_NAVIGATE_SKIPBACKFILE,    IDS_AG_PREVIOUS_FILE);
}

CPPageMouse::~CPPageMouse()
{
}

void CPPageMouse::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_COMBO1, m_cmbLeftBottonClick);
	DDX_Control(pDX, IDC_COMBO2, m_cmbLeftBottonDblClick);
	DDX_Control(pDX, IDC_CHECK1, m_chkMouseLeftClickOpenRecent);
	DDX_Control(pDX, IDC_LIST1, m_list);
}

LPCWSTR CPPageMouse::GetCmdString(MOUSE_COMMANDS* mouse_cmds, const WORD id)
{
	const auto& ids = mouse_cmds->ids;
	const auto& str_list = mouse_cmds->str_list;

	for (size_t i = 0; i < ids.size(); i++) {
		if (id == ids[i]) {
			auto it = str_list.cbegin();
			std::advance(it, i);
			return *it;
		}
	}

	return str_list.front();
}

void CPPageMouse::SyncList()
{
	for (int i = 0; i < ROW_COUNT; i++) {
		for (int j = COL_CMD; j < COL_COUNT; j++) {
			MOUSE_COMMANDS* mouse_cmds = m_table_comands[i][j];
			WORD id = m_table_values[i][j];
			m_list.SetItemText(i, j, GetCmdString(mouse_cmds, id));
		}
	}
}

BOOL CPPageMouse::OnInitDialog()
{
	__super::OnInitDialog();

	SetCursor(m_hWnd, IDC_COMBO1, IDC_HAND);

	CAppSettings& s = AfxGetAppSettings();

	AddStringData(m_cmbLeftBottonClick, L"---", 0);
	AddStringData(m_cmbLeftBottonClick, ResStr(IDS_AG_PLAYPAUSE), ID_PLAY_PLAYPAUSE);
	SelectByItemData(m_cmbLeftBottonClick, s.nMouseLeftClick);

	AddStringData(m_cmbLeftBottonDblClick, L"---", 0);
	AddStringData(m_cmbLeftBottonDblClick, ResStr(IDS_AG_PLAYPAUSE), ID_PLAY_PLAYPAUSE);
	AddStringData(m_cmbLeftBottonDblClick, ResStr(IDS_AG_FULLSCREEN), ID_VIEW_FULLSCREEN);
	SelectByItemData(m_cmbLeftBottonDblClick, s.nMouseLeftDblClick);

	m_chkMouseLeftClickOpenRecent.SetCheck(s.bMouseLeftClickOpenRecent ? BST_CHECKED : BST_UNCHECKED);

	m_table_values[ROW_BTN_M][COL_CMD]    = s.MouseMiddleClick.normal;
	m_table_values[ROW_BTN_M][COL_CTRL]   = s.MouseMiddleClick.ctrl;
	m_table_values[ROW_BTN_M][COL_SHIFT]  = s.MouseMiddleClick.shift;
	m_table_values[ROW_BTN_M][COL_RBTN]  = s.MouseMiddleClick.rbtn;
	m_table_values[ROW_BTN_X1][COL_CMD]   = s.MouseX1Click.normal;
	m_table_values[ROW_BTN_X1][COL_CTRL]  = s.MouseX1Click.ctrl;
	m_table_values[ROW_BTN_X1][COL_SHIFT] = s.MouseX1Click.shift;
	m_table_values[ROW_BTN_X1][COL_RBTN] = s.MouseX1Click.rbtn;
	m_table_values[ROW_BTN_X2][COL_CMD]   = s.MouseX2Click.normal;
	m_table_values[ROW_BTN_X2][COL_CTRL]  = s.MouseX2Click.ctrl;
	m_table_values[ROW_BTN_X2][COL_SHIFT] = s.MouseX2Click.shift;
	m_table_values[ROW_BTN_X2][COL_RBTN] = s.MouseX2Click.rbtn;
	m_table_values[ROW_WHL_U][COL_CMD]    = s.MouseWheelUp.normal;
	m_table_values[ROW_WHL_U][COL_CTRL]   = s.MouseWheelUp.ctrl;
	m_table_values[ROW_WHL_U][COL_SHIFT]  = s.MouseWheelUp.shift;
	m_table_values[ROW_WHL_U][COL_RBTN]  = s.MouseWheelUp.rbtn;
	m_table_values[ROW_WHL_D][COL_CMD]    = s.MouseWheelDown.normal;
	m_table_values[ROW_WHL_D][COL_CTRL]   = s.MouseWheelDown.ctrl;
	m_table_values[ROW_WHL_D][COL_SHIFT]  = s.MouseWheelDown.shift;
	m_table_values[ROW_WHL_D][COL_RBTN]  = s.MouseWheelDown.rbtn;
	m_table_values[ROW_WHL_L][COL_CMD]    = s.MouseWheelLeft.normal;
	m_table_values[ROW_WHL_L][COL_CTRL]   = s.MouseWheelLeft.ctrl;
	m_table_values[ROW_WHL_L][COL_SHIFT]  = s.MouseWheelLeft.shift;
	m_table_values[ROW_WHL_L][COL_RBTN]  = s.MouseWheelLeft.rbtn;
	m_table_values[ROW_WHL_R][COL_CMD]    = s.MouseWheelRight.normal;
	m_table_values[ROW_WHL_R][COL_CTRL]   = s.MouseWheelRight.ctrl;
	m_table_values[ROW_WHL_R][COL_SHIFT]  = s.MouseWheelRight.shift;
	m_table_values[ROW_WHL_R][COL_RBTN]  = s.MouseWheelRight.rbtn;

	m_list.SetExtendedStyle(m_list.GetExtendedStyle() | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	m_list.InsertColumn(COL_ACTION, ResStr(IDS_MOUSE_ACTION));
	m_list.InsertColumn(COL_CMD,    ResStr(IDS_MOUSE_COMMAND));
	m_list.InsertColumn(COL_CTRL,   L"Ctrl");
	m_list.InsertColumn(COL_SHIFT,  L"Shift");
	m_list.InsertColumn(COL_RBTN,   ResStr(IDS_MOUSE_RIGHT_BUTTON));

	m_list.InsertItem(ROW_BTN_M,  ResStr(IDS_MOUSE_CLICK_MIDDLE));
	m_list.InsertItem(ROW_BTN_X1, ResStr(IDS_MOUSE_CLICK_X1));
	m_list.InsertItem(ROW_BTN_X2, ResStr(IDS_MOUSE_CLICK_X2));
	m_list.InsertItem(ROW_WHL_U,  ResStr(IDS_MOUSE_WHEEL_UP));
	m_list.InsertItem(ROW_WHL_D,  ResStr(IDS_MOUSE_WHEEL_DOWN));
	m_list.InsertItem(ROW_WHL_L,  ResStr(IDS_MOUSE_WHEEL_LEFT));
	m_list.InsertItem(ROW_WHL_R,  ResStr(IDS_MOUSE_WHEEL_RIGHT));

	SyncList();

	for (int nCol = COL_ACTION; nCol < COL_COUNT; nCol++) {
		m_list.SetColumnWidth(nCol, LVSCW_AUTOSIZE);
		const int contentSize = m_list.GetColumnWidth(nCol);
		m_list.SetColumnWidth(nCol, LVSCW_AUTOSIZE_USEHEADER);
		const int headerSize = m_list.GetColumnWidth(nCol);
		if (contentSize > headerSize) {
			m_list.SetColumnWidth(nCol, contentSize);
		}
		{
			LVCOLUMN col;
			col.mask = LVCF_MINWIDTH;
			col.cxMin = headerSize;
			m_list.SetColumn(nCol, &col);
		}
	}

	return TRUE;
}

BOOL CPPageMouse::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	s.nMouseLeftClick    = (UINT)GetCurItemData(m_cmbLeftBottonClick);
	s.nMouseLeftDblClick = (UINT)GetCurItemData(m_cmbLeftBottonDblClick);

	s.bMouseLeftClickOpenRecent = !!m_chkMouseLeftClickOpenRecent.GetCheck();

	s.MouseMiddleClick.normal = m_table_values[ROW_BTN_M][COL_CMD];
	s.MouseMiddleClick.ctrl   = m_table_values[ROW_BTN_M][COL_CTRL];
	s.MouseMiddleClick.shift  = m_table_values[ROW_BTN_M][COL_SHIFT];
	s.MouseMiddleClick.rbtn   = m_table_values[ROW_BTN_M][COL_RBTN];
	s.MouseX1Click.normal     = m_table_values[ROW_BTN_X1][COL_CMD];
	s.MouseX1Click.ctrl       = m_table_values[ROW_BTN_X1][COL_CTRL];
	s.MouseX1Click.shift      = m_table_values[ROW_BTN_X1][COL_SHIFT];
	s.MouseX1Click.rbtn       = m_table_values[ROW_BTN_X1][COL_RBTN];
	s.MouseX2Click.normal     = m_table_values[ROW_BTN_X2][COL_CMD];
	s.MouseX2Click.ctrl       = m_table_values[ROW_BTN_X2][COL_CTRL];
	s.MouseX2Click.shift      = m_table_values[ROW_BTN_X2][COL_SHIFT];
	s.MouseX2Click.rbtn       = m_table_values[ROW_BTN_X2][COL_RBTN];
	s.MouseWheelUp.normal     = m_table_values[ROW_WHL_U][COL_CMD];
	s.MouseWheelUp.ctrl       = m_table_values[ROW_WHL_U][COL_CTRL];
	s.MouseWheelUp.shift      = m_table_values[ROW_WHL_U][COL_SHIFT];
	s.MouseWheelUp.rbtn       = m_table_values[ROW_WHL_U][COL_RBTN];
	s.MouseWheelDown.normal   = m_table_values[ROW_WHL_D][COL_CMD];
	s.MouseWheelDown.ctrl     = m_table_values[ROW_WHL_D][COL_CTRL];
	s.MouseWheelDown.shift    = m_table_values[ROW_WHL_D][COL_SHIFT];
	s.MouseWheelDown.rbtn     = m_table_values[ROW_WHL_D][COL_RBTN];
	s.MouseWheelLeft.normal   = m_table_values[ROW_WHL_L][COL_CMD];
	s.MouseWheelLeft.ctrl     = m_table_values[ROW_WHL_L][COL_CTRL];
	s.MouseWheelLeft.shift    = m_table_values[ROW_WHL_L][COL_SHIFT];
	s.MouseWheelLeft.rbtn     = m_table_values[ROW_WHL_L][COL_RBTN];
	s.MouseWheelRight.normal  = m_table_values[ROW_WHL_R][COL_CMD];
	s.MouseWheelRight.ctrl    = m_table_values[ROW_WHL_R][COL_CTRL];
	s.MouseWheelRight.shift   = m_table_values[ROW_WHL_R][COL_SHIFT];
	s.MouseWheelRight.rbtn    = m_table_values[ROW_WHL_R][COL_RBTN];

	return __super::OnApply();
}

BEGIN_MESSAGE_MAP(CPPageMouse, CPPageBase)
	ON_NOTIFY(LVN_BEGINLABELEDITW, IDC_LIST1, OnBeginlabeleditList)
	ON_NOTIFY(LVN_DOLABELEDIT, IDC_LIST1, OnDolabeleditList)
	ON_NOTIFY(LVN_ENDLABELEDITW, IDC_LIST1, OnEndlabeleditList)
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedReset)
END_MESSAGE_MAP()

// CPPageMouse message handlers

void CPPageMouse::OnBeginlabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LV_DISPINFOW* pDispInfo = (LV_DISPINFOW*)pNMHDR;
	LV_ITEMW* pItem = &pDispInfo->item;

	if (pItem->iItem >= 0 && pItem->iSubItem >= COL_CMD) {
		*pResult = TRUE;
	} else {
		*pResult = FALSE;
	}
}

void CPPageMouse::OnDolabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LV_DISPINFOW* pDispInfo = (LV_DISPINFOW*)pNMHDR;
	LV_ITEMW* pItem = &pDispInfo->item;

	if (pItem->iItem >= 0) {
		MOUSE_COMMANDS* mouse_cmds = m_table_comands[pItem->iItem][pItem->iSubItem];

		if (mouse_cmds) {
			const auto id = m_table_values[pItem->iItem][pItem->iSubItem];
			const auto& ids = mouse_cmds->ids;
			int nSel = -1;

			for (size_t i = 0; i < ids.size(); i++) {
				if (id == ids[i]) {
					nSel = (int)i;
					break;
				}
			}

			m_list.ShowInPlaceComboBox(pItem->iItem, pItem->iSubItem, mouse_cmds->str_list, nSel);

			*pResult = TRUE;

			return;
		}
	}

	*pResult = FALSE;
}

void CPPageMouse::OnEndlabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LV_DISPINFOW* pDispInfo = (LV_DISPINFOW*)pNMHDR;
	LV_ITEMW* pItem = &pDispInfo->item;

	*pResult = FALSE;

	if (m_list.m_fInPlaceDirty && pItem->iItem >= 0 && pItem->iSubItem >= COL_CMD && pItem->pszText) {
		MOUSE_COMMANDS* mouse_cmds = m_table_comands[pItem->iItem][pItem->iSubItem];

		if (mouse_cmds) {
			m_table_values[pItem->iItem][pItem->iSubItem] = mouse_cmds->ids[pItem->lParam];

			m_list.SetItemText(pItem->iItem, pItem->iSubItem, pItem->pszText);
			m_list.SetColumnWidth(pItem->iSubItem, LVSCW_AUTOSIZE);

			*pResult = TRUE;
			SetModified();

			return;
		}
	}

	*pResult = FALSE;
}

void CPPageMouse::OnBnClickedReset()
{
	CAppSettings& s = AfxGetAppSettings();

	SelectByItemData(m_cmbLeftBottonClick, ID_PLAY_PLAYPAUSE);
	SelectByItemData(m_cmbLeftBottonDblClick, ID_VIEW_FULLSCREEN);

	m_chkMouseLeftClickOpenRecent.SetCheck(BST_UNCHECKED);

	m_table_values[ROW_BTN_M][COL_CMD]    = 0;
	m_table_values[ROW_BTN_M][COL_CTRL]   = 0;
	m_table_values[ROW_BTN_M][COL_SHIFT]  = 0;
	m_table_values[ROW_BTN_M][COL_RBTN]   = 0;
	m_table_values[ROW_BTN_X1][COL_CMD]   = ID_NAVIGATE_SKIPBACK;
	m_table_values[ROW_BTN_X1][COL_CTRL]  = 0;
	m_table_values[ROW_BTN_X1][COL_SHIFT] = 0;
	m_table_values[ROW_BTN_X1][COL_RBTN]  = 0;
	m_table_values[ROW_BTN_X2][COL_CMD]   = ID_NAVIGATE_SKIPFORWARD;
	m_table_values[ROW_BTN_X2][COL_CTRL]  = 0;
	m_table_values[ROW_BTN_X2][COL_SHIFT] = 0;
	m_table_values[ROW_BTN_X2][COL_RBTN]  = 0;
	m_table_values[ROW_WHL_U][COL_CMD]    = ID_VOLUME_UP;
	m_table_values[ROW_WHL_U][COL_CTRL]   = 0;
	m_table_values[ROW_WHL_U][COL_SHIFT]  = 0;
	m_table_values[ROW_WHL_U][COL_RBTN]   = 0;
	m_table_values[ROW_WHL_D][COL_CMD]    = ID_VOLUME_DOWN;
	m_table_values[ROW_WHL_D][COL_CTRL]   = 0;
	m_table_values[ROW_WHL_D][COL_SHIFT]  = 0;
	m_table_values[ROW_WHL_D][COL_RBTN]   = 0;
	m_table_values[ROW_WHL_L][COL_CMD]    = 0;
	m_table_values[ROW_WHL_L][COL_CTRL]   = 0;
	m_table_values[ROW_WHL_L][COL_SHIFT]  = 0;
	m_table_values[ROW_WHL_L][COL_RBTN]   = 0;
	m_table_values[ROW_WHL_R][COL_CMD]    = 0;
	m_table_values[ROW_WHL_R][COL_CTRL]   = 0;
	m_table_values[ROW_WHL_R][COL_SHIFT]  = 0;
	m_table_values[ROW_WHL_R][COL_RBTN]   = 0;
	SyncList();

	SetModified();
}

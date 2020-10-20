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
{
}

CPPageMouse::~CPPageMouse()
{
}

void CPPageMouse::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_COMBO1, m_cmbLeftBottonClick);
	DDX_Control(pDX, IDC_COMBO2, m_cmbLeftBottonDblClick);
	DDX_Control(pDX, IDC_COMBO3, m_cmbMiddleBotton);
	DDX_Control(pDX, IDC_COMBO5, m_cmbXButton1);
	DDX_Control(pDX, IDC_COMBO6, m_cmbXButton2);
	DDX_Control(pDX, IDC_COMBO7, m_cmbWheelUp);
	DDX_Control(pDX, IDC_COMBO8, m_cmbWheelDown);
	DDX_Control(pDX, IDC_COMBO9, m_cmbWheelLeft);
	DDX_Control(pDX, IDC_COMBO10, m_cmbWheelRight);
	DDX_Control(pDX, IDC_CHECK1, m_chkMouseLeftClickOpenRecent);
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
	AddStringData(m_cmbLeftBottonDblClick, ResStr(IDS_AG_FULLSCREEN), ID_VIEW_FULLSCREEN);
	SelectByItemData(m_cmbLeftBottonDblClick, s.nMouseLeftDblClick);

	AddStringData(m_cmbMiddleBotton, L"---", 0);
	AddStringData(m_cmbMiddleBotton, ResStr(IDS_AG_PLAYPAUSE), ID_PLAY_PLAYPAUSE);
	AddStringData(m_cmbMiddleBotton, ResStr(IDS_AG_FULLSCREEN), ID_VIEW_FULLSCREEN);
	AddStringData(m_cmbMiddleBotton, ResStr(IDS_AG_TOGGLE_PLAYLIST), ID_VIEW_PLAYLIST);
	AddStringData(m_cmbMiddleBotton, ResStr(IDS_AG_BOSS_KEY), ID_BOSS);
	SelectByItemData(m_cmbMiddleBotton, s.nMouseMiddleClick);

	AddStringData(m_cmbXButton1, L"---", 0);
	AddStringData(m_cmbXButton1, ResStr(IDS_AG_NEXT), ID_NAVIGATE_SKIPFORWARD);
	AddStringData(m_cmbXButton1, ResStr(IDS_AG_PREVIOUS), ID_NAVIGATE_SKIPBACK);
	AddStringData(m_cmbXButton1, ResStr(IDS_AG_NEXT_FILE), ID_NAVIGATE_SKIPFORWARDFILE);
	AddStringData(m_cmbXButton1, ResStr(IDS_AG_PREVIOUS_FILE), ID_NAVIGATE_SKIPBACKFILE);
	SelectByItemData(m_cmbXButton1, s.nMouseX1Click);

	AddStringData(m_cmbXButton2, L"---", 0);
	AddStringData(m_cmbXButton2, ResStr(IDS_AG_NEXT), ID_NAVIGATE_SKIPFORWARD);
	AddStringData(m_cmbXButton2, ResStr(IDS_AG_PREVIOUS), ID_NAVIGATE_SKIPBACK);
	AddStringData(m_cmbXButton2, ResStr(IDS_AG_NEXT_FILE), ID_NAVIGATE_SKIPFORWARDFILE);
	AddStringData(m_cmbXButton2, ResStr(IDS_AG_PREVIOUS_FILE), ID_NAVIGATE_SKIPBACKFILE);
	SelectByItemData(m_cmbXButton2, s.nMouseX2Click);

	AddStringData(m_cmbWheelUp, L"---", 0);
	AddStringData(m_cmbWheelUp, ResStr(IDS_AG_VOLUME_UP), ID_VOLUME_UP);
	AddStringData(m_cmbWheelUp, ResStr(IDS_AG_VOLUME_DOWN), ID_VOLUME_DOWN);
	AddStringData(m_cmbWheelUp, ResStr(IDS_MPLAYERC_26), ID_PLAY_SEEKBACKWARDMED);
	AddStringData(m_cmbWheelUp, ResStr(IDS_MPLAYERC_25), ID_PLAY_SEEKFORWARDMED);
	SelectByItemData(m_cmbWheelUp, s.nMouseWheelUp);

	OnWheelUpChange();

	AddStringData(m_cmbWheelLeft, L"---", 0);
	AddStringData(m_cmbWheelLeft, ResStr(IDS_AG_NEXT_FILE), ID_NAVIGATE_SKIPFORWARDFILE);
	AddStringData(m_cmbWheelLeft, ResStr(IDS_AG_PREVIOUS_FILE), ID_NAVIGATE_SKIPBACKFILE);
	SelectByItemData(m_cmbWheelLeft, s.nMouseWheelLeft);

	OnWheelLeftChange();

	m_chkMouseLeftClickOpenRecent.SetCheck(s.bMouseLeftClickOpenRecent ? BST_CHECKED : BST_UNCHECKED);

	return TRUE;
}

BOOL CPPageMouse::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	s.nMouseLeftClick    = (UINT)GetCurItemData(m_cmbLeftBottonClick);
	s.nMouseLeftDblClick = (UINT)GetCurItemData(m_cmbLeftBottonDblClick);
	s.nMouseMiddleClick  = (UINT)GetCurItemData(m_cmbMiddleBotton);
	s.nMouseX1Click      = (UINT)GetCurItemData(m_cmbXButton1);
	s.nMouseX2Click      = (UINT)GetCurItemData(m_cmbXButton2);
	s.nMouseWheelUp      = (UINT)GetCurItemData(m_cmbWheelUp);
	s.nMouseWheelDown    = (UINT)GetCurItemData(m_cmbWheelDown);
	s.nMouseWheelLeft    = (UINT)GetCurItemData(m_cmbWheelLeft);
	s.nMouseWheelRight   = (UINT)GetCurItemData(m_cmbWheelRight);

	s.bMouseLeftClickOpenRecent = !!m_chkMouseLeftClickOpenRecent.GetCheck();

	return __super::OnApply();
}

BEGIN_MESSAGE_MAP(CPPageMouse, CPPageBase)
	ON_CBN_SELCHANGE(IDC_COMBO7, OnWheelUpChange)
	ON_CBN_SELCHANGE(IDC_COMBO9, OnWheelLeftChange)
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedReset)
END_MESSAGE_MAP()

// CPPageMouse message handlers

void CPPageMouse::OnWheelUpChange()
{
	UpdateData();

	m_cmbWheelDown.ResetContent();

	unsigned cmd = GetCurItemData(m_cmbWheelUp);
	switch (cmd) {
	case ID_VOLUME_UP:
		AddStringData(m_cmbWheelDown, ResStr(IDS_AG_VOLUME_DOWN), ID_VOLUME_DOWN);
		break;
	case ID_VOLUME_DOWN:
		AddStringData(m_cmbWheelDown, ResStr(IDS_AG_VOLUME_UP), ID_VOLUME_UP);
		break;
	case ID_PLAY_SEEKBACKWARDMED:
		AddStringData(m_cmbWheelDown, ResStr(IDS_MPLAYERC_25), ID_PLAY_SEEKFORWARDMED);
		break;
	case ID_PLAY_SEEKFORWARDMED:
		AddStringData(m_cmbWheelDown, ResStr(IDS_MPLAYERC_26), ID_PLAY_SEEKBACKWARDMED);
		break;
	default:
		AddStringData(m_cmbWheelDown, L"---", 0);
	}
	m_cmbWheelDown.SetCurSel(0);

	SetModified();
}

void CPPageMouse::OnWheelLeftChange()
{
	UpdateData();

	m_cmbWheelRight.ResetContent();

	unsigned cmd = GetCurItemData(m_cmbWheelLeft);
	switch (cmd) {
	case ID_NAVIGATE_SKIPFORWARDFILE:
		AddStringData(m_cmbWheelRight, ResStr(IDS_AG_PREVIOUS_FILE), ID_NAVIGATE_SKIPBACKFILE);
		break;
	case ID_NAVIGATE_SKIPBACKFILE:
		AddStringData(m_cmbWheelRight, ResStr(IDS_AG_NEXT_FILE), ID_NAVIGATE_SKIPFORWARDFILE);
		break;
	default:
		AddStringData(m_cmbWheelRight, L"---", 0);
	}
	m_cmbWheelRight.SetCurSel(0);

	SetModified();
}

void CPPageMouse::OnBnClickedReset()
{
	CAppSettings& s = AfxGetAppSettings();

	SelectByItemData(m_cmbLeftBottonClick, ID_PLAY_PLAYPAUSE);
	SelectByItemData(m_cmbLeftBottonDblClick, ID_VIEW_FULLSCREEN);
	SelectByItemData(m_cmbMiddleBotton, 0);
	SelectByItemData(m_cmbXButton1, ID_NAVIGATE_SKIPBACK);
	SelectByItemData(m_cmbXButton2, ID_NAVIGATE_SKIPFORWARD);
	SelectByItemData(m_cmbWheelUp, ID_VOLUME_UP);
	OnWheelUpChange();
	SelectByItemData(m_cmbWheelLeft, 0);
	OnWheelLeftChange();

	m_chkMouseLeftClickOpenRecent.SetCheck(BST_UNCHECKED);

	SetModified();
}

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
	DDX_Control(pDX, IDC_COMBO4, m_cmbRightBotton);
	DDX_Control(pDX, IDC_COMBO5, m_cmbXButton1);
	DDX_Control(pDX, IDC_COMBO6, m_cmbXButton2);
	DDX_Control(pDX, IDC_COMBO7, m_cmbWheel);
	DDX_Control(pDX, IDC_COMBO8, m_cmbWheelTilt);
}

BOOL CPPageMouse::OnInitDialog()
{
	__super::OnInitDialog();

	CAppSettings& s = AfxGetAppSettings();

	AddStringData(m_cmbLeftBottonClick, L"---", 0);
	AddStringData(m_cmbLeftBottonClick, ResStr(IDS_AG_PLAYPAUSE), ID_PLAY_PLAYPAUSE);
	SelectByItemData(m_cmbLeftBottonClick, ID_PLAY_PLAYPAUSE);

	AddStringData(m_cmbLeftBottonDblClick, L"---", 0);
	AddStringData(m_cmbLeftBottonDblClick, ResStr(IDS_AG_FULLSCREEN), ID_VIEW_FULLSCREEN);
	SelectByItemData(m_cmbLeftBottonDblClick, ID_VIEW_FULLSCREEN);

	AddStringData(m_cmbMiddleBotton, L"---", 0);
	AddStringData(m_cmbMiddleBotton, ResStr(IDS_AG_PLAYPAUSE), ID_PLAY_PLAYPAUSE);
	AddStringData(m_cmbMiddleBotton, ResStr(IDS_AG_FULLSCREEN), ID_VIEW_FULLSCREEN);
	AddStringData(m_cmbMiddleBotton, ResStr(IDS_AG_BOSS_KEY), ID_BOSS);
	m_cmbMiddleBotton.SetCurSel(0);

	m_cmbRightBotton.AddString(L"Ñontext menu"); // read only
	m_cmbRightBotton.SetCurSel(0);

	AddStringData(m_cmbXButton1, L"---", 0);
	AddStringData(m_cmbXButton1, ResStr(IDS_AG_NEXT), ID_NAVIGATE_SKIPFORWARD);
	AddStringData(m_cmbXButton1, ResStr(IDS_AG_PREVIOUS), ID_NAVIGATE_SKIPBACK);
	AddStringData(m_cmbXButton1, ResStr(IDS_AG_NEXT_FILE), ID_NAVIGATE_SKIPFORWARDFILE);
	AddStringData(m_cmbXButton1, ResStr(IDS_AG_PREVIOUS_FILE), ID_NAVIGATE_SKIPBACKFILE);
	SelectByItemData(m_cmbXButton1, ID_NAVIGATE_SKIPBACK);

	AddStringData(m_cmbXButton2, L"---", 0);
	AddStringData(m_cmbXButton2, ResStr(IDS_AG_NEXT), ID_NAVIGATE_SKIPFORWARD);
	AddStringData(m_cmbXButton2, ResStr(IDS_AG_PREVIOUS), ID_NAVIGATE_SKIPBACK);
	AddStringData(m_cmbXButton2, ResStr(IDS_AG_NEXT_FILE), ID_NAVIGATE_SKIPFORWARDFILE);
	AddStringData(m_cmbXButton2, ResStr(IDS_AG_PREVIOUS_FILE), ID_NAVIGATE_SKIPBACKFILE);
	SelectByItemData(m_cmbXButton2, ID_NAVIGATE_SKIPFORWARD);

	AddStringData(m_cmbWheel, L"---", 0);
	AddStringData(m_cmbWheel, ResStr(IDS_AG_VOLUME_UP) + L" / " + ResStr(IDS_AG_VOLUME_DOWN), ID_VOLUME_UP); // and ID_VOLUME_DOWN
	AddStringData(m_cmbWheel, ResStr(IDS_MPLAYERC_25) + L" / " + ResStr(IDS_MPLAYERC_26), ID_PLAY_SEEKBACKWARDMED); // and ID_PLAY_SEEKFORWARDMED
	SelectByItemData(m_cmbWheel, ID_VOLUME_UP);

	AddStringData(m_cmbWheelTilt, L"---", 0);
	AddStringData(m_cmbWheelTilt, ResStr(IDS_AG_VOLUME_UP) + L" / " + ResStr(IDS_AG_VOLUME_DOWN), ID_VOLUME_UP);
	AddStringData(m_cmbWheelTilt, ResStr(IDS_AG_INCREASE_RATE) + L" / " + ResStr(IDS_AG_DECREASE_RATE), ID_PLAY_INCRATE);
	m_cmbWheelTilt.SetCurSel(0);

	return TRUE;
}

BOOL CPPageMouse::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	unsigned v;
	v = GetCurItemData(m_cmbLeftBottonClick);
	v = GetCurItemData(m_cmbLeftBottonDblClick);
	v = GetCurItemData(m_cmbMiddleBotton);
	v = GetCurItemData(m_cmbXButton1);
	v = GetCurItemData(m_cmbXButton2);
	v = GetCurItemData(m_cmbWheel);
	v = GetCurItemData(m_cmbWheelTilt);

	return __super::OnApply();
}

BEGIN_MESSAGE_MAP(CPPageMouse, CPPageBase)

END_MESSAGE_MAP()

// CPPageMouse message handlers


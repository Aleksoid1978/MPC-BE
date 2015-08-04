/*
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

#include "stdafx.h"
#include "MiniDump.h"
#include "PPageMisc.h"

// CPPageMisc dialog

IMPLEMENT_DYNAMIC(CPPageMisc, CPPageBase)
CPPageMisc::CPPageMisc()
	: CPPageBase(CPPageMisc::IDD, CPPageMisc::IDD)
	, m_nJumpDistS(0)
	, m_nJumpDistM(0)
	, m_nJumpDistL(0)
	, m_fFastSeek(FALSE)
	, m_fDontUseSearchInFolder(FALSE)
	, m_fPreventMinimize(FALSE)
	, m_fLCDSupport(FALSE)
	, m_fMiniDump(FALSE)
	, m_nUpdaterDelay(7)
{
}

CPPageMisc::~CPPageMisc()
{
}

void CPPageMisc::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_EDIT1, m_nJumpDistS);
	DDX_Text(pDX, IDC_EDIT2, m_nJumpDistM);
	DDX_Text(pDX, IDC_EDIT3, m_nJumpDistL);
	DDX_Check(pDX, IDC_CHECK6, m_fPreventMinimize);
	DDX_Check(pDX, IDC_CHECK7, m_fDontUseSearchInFolder);
	DDX_Check(pDX, IDC_CHECK1, m_fFastSeek);
	DDX_Check(pDX, IDC_CHECK_LCD, m_fLCDSupport);
	DDX_Check(pDX, IDC_CHECK2, m_fMiniDump);

	DDX_Control(pDX, IDC_CHECK3, m_updaterAutoCheckCtrl);
	DDX_Control(pDX, IDC_EDIT4, m_updaterDelayCtrl);
	DDX_Control(pDX, IDC_SPIN1, m_updaterDelaySpin);
	DDX_Text(pDX, IDC_EDIT4, m_nUpdaterDelay);
}

BEGIN_MESSAGE_MAP(CPPageMisc, CPPageBase)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
	ON_BN_CLICKED(IDC_RESET_SETTINGS, OnResetSettings)
	ON_BN_CLICKED(IDC_EXPORT_SETTINGS, OnExportSettings)
	ON_UPDATE_COMMAND_UI(IDC_EDIT4, OnUpdateDelayEditBox)
	ON_UPDATE_COMMAND_UI(IDC_SPIN1, OnUpdateDelayEditBox)
	ON_UPDATE_COMMAND_UI(IDC_STATIC5, OnUpdateDelayEditBox)
	ON_UPDATE_COMMAND_UI(IDC_STATIC6, OnUpdateDelayEditBox)
END_MESSAGE_MAP()

// CPPageMisc message handlers

BOOL CPPageMisc::OnInitDialog()
{
	__super::OnInitDialog();

	SetCursor(m_hWnd, IDC_COMBO1, IDC_HAND);

	AppSettings& s = AfxGetAppSettings();

	m_nJumpDistS = s.nJumpDistS;
	m_nJumpDistM = s.nJumpDistM;
	m_nJumpDistL = s.nJumpDistL;
	m_fPreventMinimize = s.fPreventMinimize;
	m_fDontUseSearchInFolder = s.fDontUseSearchInFolder;
	m_fFastSeek = s.fFastSeek;
	m_fLCDSupport = s.fLCDSupport;
	m_fMiniDump = s.fMiniDump;

	m_updaterAutoCheckCtrl.SetCheck(s.bUpdaterAutoCheck);
	m_nUpdaterDelay = s.nUpdaterDelay;
	m_updaterDelaySpin.SetRange32(1, 365);

	UpdateData(FALSE);

	return TRUE;
}

BOOL CPPageMisc::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();

	s.nJumpDistS = m_nJumpDistS;
	s.nJumpDistM = m_nJumpDistM;
	s.nJumpDistL = m_nJumpDistL;
	s.fPreventMinimize = !!m_fPreventMinimize;
	s.fDontUseSearchInFolder = !!m_fDontUseSearchInFolder;
	s.fFastSeek = !!m_fFastSeek;
	s.fLCDSupport = !!m_fLCDSupport;
	s.fMiniDump = !!m_fMiniDump;
	CMiniDump::SetState(s.fMiniDump);

	s.bUpdaterAutoCheck	= !!m_updaterAutoCheckCtrl.GetCheck();
	m_nUpdaterDelay		= min(max(1, m_nUpdaterDelay), 365);
	s.nUpdaterDelay		= m_nUpdaterDelay;

	return __super::OnApply();
}

void CPPageMisc::OnBnClickedButton1()
{
	m_nJumpDistS = DEFAULT_JUMPDISTANCE_1;
	m_nJumpDistM = DEFAULT_JUMPDISTANCE_2;
	m_nJumpDistL = DEFAULT_JUMPDISTANCE_3;

	UpdateData(FALSE);

	SetModified();
}

void CPPageMisc::OnUpdateDelayEditBox(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_updaterAutoCheckCtrl.GetCheck() == BST_CHECKED);
}

void CPPageMisc::OnResetSettings()
{
	if (MessageBox(ResStr(IDS_RESET_SETTINGS_WARNING), ResStr(IDS_RESET_SETTINGS), MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2) == IDYES) {
		AfxGetAppSettings().fReset = true;
		GetParent()->PostMessage(WM_CLOSE);
	}
}

void CPPageMisc::OnExportSettings()
{
	if (GetParent()->GetDlgItem(ID_APPLY_NOW)->IsWindowEnabled()) {
		int ret = MessageBox(ResStr(IDS_EXPORT_SETTINGS_WARNING), ResStr(IDS_EXPORT_SETTINGS), MB_ICONEXCLAMATION | MB_YESNOCANCEL);

		if (ret == IDCANCEL) {
			return;
		} else if (ret == IDYES) {
			GetParent()->PostMessage(PSM_APPLY);
		}
	}

	AfxGetMyApp()->ExportSettings();
	SetFocus();
}

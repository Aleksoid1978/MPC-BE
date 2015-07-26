/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2015 see Authors.txt
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
#include "PPagePlayer.h"
#include "../../DSUtil/Filehandle.h"

// CPPagePlayer dialog

IMPLEMENT_DYNAMIC(CPPagePlayer, CPPageBase)
CPPagePlayer::CPPagePlayer()
	: CPPageBase(CPPagePlayer::IDD, CPPagePlayer::IDD)
	, m_iMultipleInst(1)
	, m_iTitleBarTextStyle(0)
	, m_bTitleBarTextTitle(FALSE)
	, m_fKeepHistory(FALSE)
	, m_nRecentFiles(APP_RECENTFILES_DEF)
	, m_fRememberDVDPos(FALSE)
	, m_fRememberFilePos(FALSE)
	, m_fRememberWindowPos(FALSE)
	, m_fRememberWindowSize(FALSE)
	, m_fSavePnSZoom(FALSE)
	, m_bRememberPlaylistItems(FALSE)
	, m_fTrayIcon(FALSE)
	, m_fShowOSD(FALSE)
	, m_fLimitWindowProportions(TRUE)
	, m_fSnapToDesktopEdges(FALSE)
	, m_fUseIni(FALSE)
	, m_fHideCDROMsSubMenu(FALSE)
	, m_bPriority(FALSE)
{
}

CPPagePlayer::~CPPagePlayer()
{
}

void CPPagePlayer::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Radio(pDX, IDC_RADIO1, m_iMultipleInst);
	DDX_Radio(pDX, IDC_RADIO4, m_iTitleBarTextStyle);
	DDX_Check(pDX, IDC_CHECK13, m_bTitleBarTextTitle);
	DDX_Check(pDX, IDC_CHECK3, m_fTrayIcon);
	DDX_Check(pDX, IDC_CHECK6, m_fRememberWindowPos);
	DDX_Check(pDX, IDC_CHECK7, m_fRememberWindowSize);
	DDX_Check(pDX, IDC_CHECK11, m_fSavePnSZoom);
	DDX_Check(pDX, IDC_CHECK12, m_fSnapToDesktopEdges);
	DDX_Check(pDX, IDC_CHECK8, m_fUseIni);
	DDX_Check(pDX, IDC_CHECK1, m_fKeepHistory);
	DDX_Check(pDX, IDC_CHECK10, m_fHideCDROMsSubMenu);
	DDX_Check(pDX, IDC_CHECK9, m_bPriority);
	DDX_Check(pDX, IDC_SHOW_OSD, m_fShowOSD);
	DDX_Check(pDX, IDC_CHECK14, m_fOSDFileName);
	DDX_Check(pDX, IDC_CHECK15, m_fOSDSeekTime);
	DDX_Check(pDX, IDC_CHECK4, m_fLimitWindowProportions);
	DDX_Check(pDX, IDC_DVD_POS, m_fRememberDVDPos);
	DDX_Check(pDX, IDC_FILE_POS, m_fRememberFilePos);
	DDX_Check(pDX, IDC_CHECK2, m_bRememberPlaylistItems);
	DDX_Text(pDX, IDC_EDIT1, m_nRecentFiles);
	DDX_Control(pDX, IDC_SPIN1, m_RecentFilesCtrl);

	m_nRecentFiles = min(max(APP_RECENTFILES_MIN, m_nRecentFiles), APP_RECENTFILES_MAX);
}

BEGIN_MESSAGE_MAP(CPPagePlayer, CPPageBase)
	ON_UPDATE_COMMAND_UI(IDC_CHECK13, OnUpdateCheck13)
	ON_UPDATE_COMMAND_UI(IDC_DVD_POS, OnUpdatePos)
	ON_UPDATE_COMMAND_UI(IDC_FILE_POS, OnUpdatePos)
	ON_UPDATE_COMMAND_UI(IDC_EDIT1, OnUpdatePos)
	ON_UPDATE_COMMAND_UI(IDC_SPIN1, OnUpdatePos)
	ON_UPDATE_COMMAND_UI(IDC_CHECK14, OnUpdateOSD)
	ON_UPDATE_COMMAND_UI(IDC_CHECK15, OnUpdateOSD)
	ON_EN_CHANGE(IDC_EDIT1, OnChangeEdit1)
	ON_EN_KILLFOCUS(IDC_EDIT1, OnKillFocusEdit1)
END_MESSAGE_MAP()

// CPPagePlayer message handlers

BOOL CPPagePlayer::OnInitDialog()
{
	__super::OnInitDialog();

	SetHandCursor(m_hWnd, IDC_COMBO1);

	AppSettings& s = AfxGetAppSettings();

	m_iMultipleInst				= s.iMultipleInst;
	m_iTitleBarTextStyle		= s.iTitleBarTextStyle;
	m_bTitleBarTextTitle		= s.fTitleBarTextTitle;
	m_fTrayIcon					= s.fTrayIcon;
	m_fRememberWindowPos		= s.fRememberWindowPos;
	m_fRememberWindowSize		= s.fRememberWindowSize;
	m_fSavePnSZoom				= s.fSavePnSZoom;
	m_fSnapToDesktopEdges		= s.fSnapToDesktopEdges;
	m_fUseIni					= AfxGetMyApp()->IsIniValid();
	m_fKeepHistory				= s.fKeepHistory;
	m_fHideCDROMsSubMenu		= s.fHideCDROMsSubMenu;
	m_bPriority					= s.dwPriority != NORMAL_PRIORITY_CLASS;
	m_fShowOSD					= !!(s.iShowOSD & OSD_ENABLE);
	m_fOSDFileName				= !!(s.iShowOSD & OSD_FILENAME);
	m_fOSDSeekTime				= !!(s.iShowOSD & OSD_SEEKTIME);
	m_fRememberDVDPos			= s.fRememberDVDPos;
	m_fRememberFilePos			= s.fRememberFilePos;
	m_fLimitWindowProportions	= s.fLimitWindowProportions;
	m_bRememberPlaylistItems	= s.bRememberPlaylistItems;

	m_nRecentFiles = s.iRecentFilesNumber;
	m_RecentFilesCtrl.SetRange(APP_RECENTFILES_MIN, APP_RECENTFILES_MAX);
	m_RecentFilesCtrl.SetPos(m_nRecentFiles);
	UDACCEL acc = {0, 5};
	m_RecentFilesCtrl.SetAccel(1, &acc);

	UpdateData(FALSE);

	GetDlgItem(IDC_FILE_POS)->EnableWindow(s.fKeepHistory);
	GetDlgItem(IDC_DVD_POS)->EnableWindow(s.fKeepHistory);
	m_RecentFilesCtrl.EnableWindow(s.fKeepHistory);

	CString iniDirPath = GetProgramPath();
	HANDLE hDir = CreateFile(iniDirPath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
							 OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	// gray-out "Store settings in the player folder" option when we don't have writing permissions in the target directory
	GetDlgItem(IDC_CHECK8)->EnableWindow(hDir != INVALID_HANDLE_VALUE);
	CloseHandle(hDir);

	return TRUE;
}

BOOL CPPagePlayer::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();
	auto pFrame = AfxGetMainFrame();

	s.iMultipleInst = m_iMultipleInst;
	if (s.iTitleBarTextStyle != m_iTitleBarTextStyle || s.fTitleBarTextTitle != !!m_bTitleBarTextTitle) {
		s.iTitleBarTextStyle = m_iTitleBarTextStyle;
		s.fTitleBarTextTitle = !!m_bTitleBarTextTitle;
		pFrame->OpenSetupWindowTitle(pFrame->m_strFnFull);
	}
	s.fTrayIcon = !!m_fTrayIcon;
	s.fRememberWindowPos = !!m_fRememberWindowPos;
	s.fRememberWindowSize = !!m_fRememberWindowSize;
	s.fSavePnSZoom = !!m_fSavePnSZoom;
	s.fSnapToDesktopEdges = !!m_fSnapToDesktopEdges;
	s.fKeepHistory = !!m_fKeepHistory;
	s.fHideCDROMsSubMenu = !!m_fHideCDROMsSubMenu;
	s.dwPriority = !m_bPriority ? NORMAL_PRIORITY_CLASS : ABOVE_NORMAL_PRIORITY_CLASS;
	BOOL bShowOSDChanged = ((s.iShowOSD & OSD_ENABLE) != !!m_fShowOSD);

	s.iShowOSD = m_fShowOSD ? OSD_ENABLE : 0;
	if (m_fOSDFileName) { s.iShowOSD |= OSD_FILENAME; }
	if (m_fOSDSeekTime) { s.iShowOSD |= OSD_SEEKTIME; }
	if (bShowOSDChanged) {
		if (m_fShowOSD & OSD_ENABLE) {
			pFrame->m_OSD.Start(pFrame->m_pOSDWnd);
			pFrame->OSDBarSetPos();
			pFrame->m_OSD.ClearMessage(false);
		} else {
			pFrame->m_OSD.Stop();
		}
	}
	s.fLimitWindowProportions = !!m_fLimitWindowProportions;
	s.fRememberDVDPos = !!m_fRememberDVDPos;
	s.fRememberFilePos = !!m_fRememberFilePos;
	s.bRememberPlaylistItems = !!m_bRememberPlaylistItems;

	if (!m_fKeepHistory) {
		for (int i = s.MRU.GetSize() - 1; i >= 0; i--) {
			s.MRU.Remove(i);
		}

		for (int i = s.MRUDub.GetSize() - 1; i >= 0; i--) {
			s.MRUDub.Remove(i);
		}

		s.MRU.WriteList();
		s.MRUDub.WriteList();

		// Empty the "Recent" jump list
		CComPtr<IApplicationDestinations> pDests;
		HRESULT hr = pDests.CoCreateInstance(CLSID_ApplicationDestinations, NULL, CLSCTX_INPROC_SERVER);
		if (SUCCEEDED(hr)) {
			hr = pDests->RemoveAllDestinations();
		}
	}
	if (!m_fKeepHistory || !m_fRememberDVDPos) {
		s.ClearDVDPositions();
	}
	if (!m_fKeepHistory || !m_fRememberFilePos) {
		s.ClearFilePositions();
	}

	s.iRecentFilesNumber = m_nRecentFiles;
	s.MRU.SetSize(s.iRecentFilesNumber);
	s.MRUDub.SetSize(s.iRecentFilesNumber);

	// Check if the settings location needs to be changed
	if (AfxGetMyApp()->IsIniValid() != !!m_fUseIni) {
		AfxGetMyApp()->ChangeSettingsLocation(!!m_fUseIni);
	}

	AfxGetMainFrame()->ShowTrayIcon(s.fTrayIcon);

	::SetPriorityClass(::GetCurrentProcess(), s.dwPriority);

	GetDlgItem(IDC_FILE_POS)->EnableWindow(s.fKeepHistory);
	GetDlgItem(IDC_DVD_POS)->EnableWindow(s.fKeepHistory);
	m_RecentFilesCtrl.EnableWindow(s.fKeepHistory);

	return __super::OnApply();
}

void CPPagePlayer::OnUpdateCheck13(CCmdUI* pCmdUI)
{
	UpdateData();

	pCmdUI->Enable(m_iTitleBarTextStyle == 1);
}

void CPPagePlayer::OnUpdatePos(CCmdUI* pCmdUI)
{
	UpdateData();

	pCmdUI->Enable(!!m_fKeepHistory);
}

void CPPagePlayer::OnUpdateOSD(CCmdUI* pCmdUI)
{
	UpdateData();

	pCmdUI->Enable(!!m_fShowOSD);
}

void CPPagePlayer::OnKillFocusEdit1()
{
	m_nRecentFiles = min(max(APP_RECENTFILES_MIN, m_nRecentFiles), APP_RECENTFILES_MAX); // CSpinButtonCtrl.SetRange() does not affect the manual input

	UpdateData(FALSE);
}

void CPPagePlayer::OnChangeEdit1()
{
	CString rString;
	GetDlgItemText(IDC_EDIT1, rString);
	if (rString.IsEmpty()) {
		OnKillFocusEdit1();
	}

	SetModified();
}

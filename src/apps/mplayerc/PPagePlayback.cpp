/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2024 see Authors.txt
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
#include "PPagePlayback.h"

// CPPagePlayback dialog

IMPLEMENT_DYNAMIC(CPPagePlayback, CPPageBase)
CPPagePlayback::CPPagePlayback()
	: CPPageBase(CPPagePlayback::IDD, CPPagePlayback::IDD)
{
}

CPPagePlayback::~CPPagePlayback()
{
}

void CPPagePlayback::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Radio(pDX, IDC_RADIO1, m_iLoopForever);
	DDX_Control(pDX, IDC_EDIT1, m_loopnumctrl);
	DDX_Text(pDX, IDC_EDIT1, m_nLoops);
	DDX_Check(pDX, IDC_CHECK1, m_fRewind);

	DDX_CBIndex(pDX, IDC_COMBOVOLUME, m_nVolumeStep);
	DDX_Control(pDX, IDC_COMBOVOLUME, m_nVolumeStepCtrl);
	DDX_Control(pDX, IDC_COMBOSPEEDSTEP, m_nSpeedStepCtrl);
	DDX_Check(pDX, IDC_CHECK9, m_bSpeedNotReset);

	DDX_Check(pDX, IDC_CHECK4, m_fUseInternalSelectTrackLogic);
	DDX_Text(pDX, IDC_EDIT2, m_subtitlesLanguageOrder);
	DDX_Text(pDX, IDC_EDIT3, m_audiosLanguageOrder);

	DDX_Control(pDX, IDC_COMBO4, m_cbAudioWindowMode);
	DDX_Control(pDX, IDC_COMBO5, m_cbAddSimilarFiles);
	DDX_Check(pDX, IDC_CHECK7, m_fEnableWorkerThreadForOpening);
	DDX_Check(pDX, IDC_CHECK6, m_fReportFailedPins);
	DDX_Check(pDX, IDC_CHECK8, m_bRememberSelectedTracks);

	DDX_Check(pDX, IDC_CHECK3, m_bFastSeek);
	DDX_Check(pDX, IDC_CHECK5, m_bPauseMinimizedVideo);
}

BEGIN_MESSAGE_MAP(CPPagePlayback, CPPageBase)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_RADIO1, IDC_RADIO2, OnBnClickedRadio12)
	ON_UPDATE_COMMAND_UI(IDC_EDIT1, OnUpdateLoopNum)
	ON_UPDATE_COMMAND_UI(IDC_STATIC1, OnUpdateLoopNum)
	ON_UPDATE_COMMAND_UI(IDC_EDIT2, OnUpdateTrackOrder)
	ON_UPDATE_COMMAND_UI(IDC_EDIT3, OnUpdateTrackOrder)
END_MESSAGE_MAP()

// CPPagePlayback message handlers

BOOL CPPagePlayback::OnInitDialog()
{
	__super::OnInitDialog();

	SetCursor(m_hWnd, IDC_COMBO4, IDC_HAND);

	CAppSettings& s = AfxGetAppSettings();

	m_iLoopForever	= s.fLoopForever ? 1 : 0;
	m_nLoops		= s.nLoops;
	m_fRewind		= s.fRewind;

	for (int idx = 1; idx <= 10; idx++) {
		CString str; str.Format(L"%d", idx);
		m_nVolumeStepCtrl.AddString(str);
	}
	m_nVolumeStep = s.nVolumeStep - 1;

	AddStringData(m_nSpeedStepCtrl, ResStr(IDS_AG_AUTO), 0);
	AddStringData(m_nSpeedStepCtrl, L"1",     1);
	AddStringData(m_nSpeedStepCtrl, L"5",     5);
	AddStringData(m_nSpeedStepCtrl, L"10",   10);
	AddStringData(m_nSpeedStepCtrl, L"20",   20);
	AddStringData(m_nSpeedStepCtrl, L"25",   25);
	AddStringData(m_nSpeedStepCtrl, L"50",   50);
	AddStringData(m_nSpeedStepCtrl, L"100", 100);
	SelectByItemData(m_nSpeedStepCtrl, s.nSpeedStep);
	m_bSpeedNotReset = s.bSpeedNotReset;

	m_fUseInternalSelectTrackLogic	= s.fUseInternalSelectTrackLogic;
	m_subtitlesLanguageOrder		= s.strSubtitlesLanguageOrder;
	m_audiosLanguageOrder			= s.strAudiosLanguageOrder;

	m_cbAudioWindowMode.AddString(ResStr(IDS_AUDIOWINDOW_NONE));
	m_cbAudioWindowMode.AddString(ResStr(IDS_AUDIOWINDOW_COVER));
	m_cbAudioWindowMode.AddString(ResStr(IDS_AUDIOWINDOW_HIDE));
	m_cbAudioWindowMode.SetCurSel(s.nAudioWindowMode);

	m_cbAddSimilarFiles.AddString(ResStr(IDS_ADDTOPLS_FROMDIR_NONE));
	m_cbAddSimilarFiles.AddString(ResStr(IDS_ADDTOPLS_FROMDIR_SIMILAR));
	m_cbAddSimilarFiles.AddString(ResStr(IDS_ADDTOPLS_FROMDIR_ALL));
	m_cbAddSimilarFiles.SetCurSel(s.nAddSimilarFiles);

	m_fEnableWorkerThreadForOpening = s.fEnableWorkerThreadForOpening;
	m_fReportFailedPins = s.fReportFailedPins;
	m_bRememberSelectedTracks = s.bRememberSelectedTracks;

	m_bFastSeek = s.fFastSeek;
	m_bPauseMinimizedVideo = s.bPauseMinimizedVideo;

	CorrectCWndWidth(GetDlgItem(IDC_CHECK4));

	UpdateData(FALSE);

	return TRUE;
}

BOOL CPPagePlayback::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	s.fLoopForever = !!m_iLoopForever;
	s.nLoops = m_nLoops;
	s.fRewind = !!m_fRewind;
	s.nAudioWindowMode = m_cbAudioWindowMode.GetCurSel();
	s.nAddSimilarFiles = m_cbAddSimilarFiles.GetCurSel();
	s.fEnableWorkerThreadForOpening = !!m_fEnableWorkerThreadForOpening;
	s.fReportFailedPins = !!m_fReportFailedPins;
	s.strSubtitlesLanguageOrder = m_subtitlesLanguageOrder;
	s.strAudiosLanguageOrder = m_audiosLanguageOrder;
	s.nVolumeStep = m_nVolumeStep + 1;
	AfxGetMainFrame()->m_wndToolBar.m_volctrl.SetPageSize(s.nVolumeStep);

	s.nSpeedStep = GetCurItemData(m_nSpeedStepCtrl);
	s.bSpeedNotReset = !!m_bSpeedNotReset;

	s.fUseInternalSelectTrackLogic = !!m_fUseInternalSelectTrackLogic;

	s.bRememberSelectedTracks = !!m_bRememberSelectedTracks;

	s.fFastSeek = !!m_bFastSeek;
	s.bPauseMinimizedVideo = !!m_bPauseMinimizedVideo;

	return __super::OnApply();
}

void CPPagePlayback::OnBnClickedRadio12(UINT nID)
{
	SetModified();
}

void CPPagePlayback::OnUpdateLoopNum(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!!IsDlgButtonChecked(IDC_RADIO1));
}

void CPPagePlayback::OnUpdateTrackOrder(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!!IsDlgButtonChecked(IDC_CHECK4));
}

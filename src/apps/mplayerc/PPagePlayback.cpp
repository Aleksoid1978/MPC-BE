/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2018 see Authors.txt
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
	, m_iLoopForever(0)
	, m_nLoops(0)
	, m_fRewind(FALSE)
	, m_nVolume(0)
	, m_nBalance(0)
	, m_bAddSimilarFiles(FALSE)
	, m_fEnableWorkerThreadForOpening(FALSE)
	, m_fReportFailedPins(FALSE)
	, m_nVolumeStep(1)
	, m_fUseInternalSelectTrackLogic(TRUE)
	, m_bRememberSelectedTracks(FALSE)
{
}

CPPagePlayback::~CPPagePlayback()
{
}

void CPPagePlayback::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_SLIDER1, m_volumectrl);
	DDX_Control(pDX, IDC_SLIDER2, m_balancectrl);
	DDX_Slider(pDX, IDC_SLIDER1, m_nVolume);
	DDX_Slider(pDX, IDC_SLIDER2, m_nBalance);

	DDX_Radio(pDX, IDC_RADIO1, m_iLoopForever);
	DDX_Control(pDX, IDC_EDIT1, m_loopnumctrl);
	DDX_Text(pDX, IDC_EDIT1, m_nLoops);
	DDX_Check(pDX, IDC_CHECK1, m_fRewind);

	DDX_CBIndex(pDX, IDC_COMBOVOLUME, m_nVolumeStep);
	DDX_Control(pDX, IDC_COMBOVOLUME, m_nVolumeStepCtrl);
	DDX_Control(pDX, IDC_COMBOSPEEDSTEP, m_nSpeedStepCtrl);

	DDX_Check(pDX, IDC_CHECK4, m_fUseInternalSelectTrackLogic);
	DDX_Text(pDX, IDC_EDIT2, m_subtitlesLanguageOrder);
	DDX_Text(pDX, IDC_EDIT3, m_audiosLanguageOrder);

	DDX_Control(pDX, IDC_COMBO4, m_cbAudioWindowMode);
	DDX_Check(pDX, IDC_CHECK3, m_bAddSimilarFiles);
	DDX_Check(pDX, IDC_CHECK7, m_fEnableWorkerThreadForOpening);
	DDX_Check(pDX, IDC_CHECK6, m_fReportFailedPins);

	DDX_Check(pDX, IDC_CHECK8, m_bRememberSelectedTracks);
}

BEGIN_MESSAGE_MAP(CPPagePlayback, CPPageBase)
	ON_WM_HSCROLL()
	ON_CONTROL_RANGE(BN_CLICKED, IDC_RADIO1, IDC_RADIO2, OnBnClickedRadio12)
	ON_UPDATE_COMMAND_UI(IDC_EDIT1, OnUpdateLoopNum)
	ON_UPDATE_COMMAND_UI(IDC_STATIC1, OnUpdateLoopNum)
	ON_UPDATE_COMMAND_UI(IDC_EDIT2, OnUpdateTrackOrder)
	ON_UPDATE_COMMAND_UI(IDC_EDIT3, OnUpdateTrackOrder)
	ON_STN_DBLCLK(IDC_STATIC_BALANCE, OnBalanceTextDblClk)
	ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnToolTipNotify)
END_MESSAGE_MAP()

// CPPagePlayback message handlers

BOOL CPPagePlayback::OnInitDialog()
{
	__super::OnInitDialog();

	SetCursor(m_hWnd, IDC_COMBO1, IDC_HAND);

	CAppSettings& s = AfxGetAppSettings();

	m_volumectrl.SetRange(0, 100);
	m_volumectrl.SetTicFreq(10);
	m_balancectrl.SetRange(-100, 100);
	m_balancectrl.SetLineSize(2);
	m_balancectrl.SetPageSize(2);
	m_balancectrl.SetTicFreq(20);
	m_nVolume = m_oldVolume = s.nVolume;
	m_nBalance = s.nBalance;

	m_iLoopForever	= s.fLoopForever ? 1 : 0;
	m_nLoops		= s.nLoops;
	m_fRewind		= s.fRewind;

	for (int idx = 1; idx <= 10; idx++) {
		CString str; str.Format(L"%d", idx);
		m_nVolumeStepCtrl.AddString(str);
	}
	m_nVolumeStep = s.nVolumeStep - 1;

	AddStringData(m_nSpeedStepCtrl, ResStr(IDS_AG_AUTO), 0);
	AddStringData(m_nSpeedStepCtrl, L"0.1",  10);
	AddStringData(m_nSpeedStepCtrl, L"0.2",  20);
	AddStringData(m_nSpeedStepCtrl, L"0.25", 25);
	AddStringData(m_nSpeedStepCtrl, L"0.5",  50);
	AddStringData(m_nSpeedStepCtrl, L"1.0", 100);
	SelectByItemData(m_nSpeedStepCtrl, s.nSpeedStep);

	m_fUseInternalSelectTrackLogic	= s.fUseInternalSelectTrackLogic;
	m_subtitlesLanguageOrder		= s.strSubtitlesLanguageOrder;
	m_audiosLanguageOrder			= s.strAudiosLanguageOrder;

	m_cbAudioWindowMode.AddString(ResStr(IDS_AUDIOWINDOW_NONE));
	m_cbAudioWindowMode.AddString(ResStr(IDS_AUDIOWINDOW_COVER));
	m_cbAudioWindowMode.AddString(ResStr(IDS_AUDIOWINDOW_HIDE));
	m_cbAudioWindowMode.SetCurSel(s.nAudioWindowMode);

	m_bAddSimilarFiles = s.bAddSimilarFiles;
	m_fEnableWorkerThreadForOpening = s.fEnableWorkerThreadForOpening;
	m_fReportFailedPins = s.fReportFailedPins;

	m_bRememberSelectedTracks = s.bRememberSelectedTracks;

	CorrectCWndWidth(GetDlgItem(IDC_CHECK4));

	UpdateData(FALSE);

	return TRUE;
}

BOOL CPPagePlayback::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	s.nVolume = m_oldVolume = m_nVolume;
	s.nBalance = m_nBalance;
	s.fLoopForever = !!m_iLoopForever;
	s.nLoops = m_nLoops;
	s.fRewind = !!m_fRewind;
	s.nAudioWindowMode = m_cbAudioWindowMode.GetCurSel();
	s.bAddSimilarFiles = !!m_bAddSimilarFiles;
	s.fEnableWorkerThreadForOpening = !!m_fEnableWorkerThreadForOpening;
	s.fReportFailedPins = !!m_fReportFailedPins;
	s.strSubtitlesLanguageOrder = m_subtitlesLanguageOrder;
	s.strAudiosLanguageOrder = m_audiosLanguageOrder;
	s.nVolumeStep = m_nVolumeStep + 1;
	AfxGetMainFrame()->m_wndToolBar.m_volctrl.SetPageSize(s.nVolumeStep);

	s.nSpeedStep = GetCurItemData(m_nSpeedStepCtrl);

	s.fUseInternalSelectTrackLogic = !!m_fUseInternalSelectTrackLogic;

	s.bRememberSelectedTracks = !!m_bRememberSelectedTracks;

	return __super::OnApply();
}

void CPPagePlayback::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (*pScrollBar == m_volumectrl) {
		UpdateData();
		AfxGetMainFrame()->m_wndToolBar.Volume = m_nVolume;
	} else if (*pScrollBar == m_balancectrl) {
		UpdateData();
		AfxGetMainFrame()->SetBalance(m_nBalance);
	}

	SetModified();

	__super::OnHScroll(nSBCode, nPos, pScrollBar);
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

void CPPagePlayback::OnBalanceTextDblClk()
{
	// double click on text "Balance" resets the balance to zero
	m_nBalance = 0;
	m_balancectrl.SetPos(m_nBalance);

	AfxGetMainFrame()->SetBalance(m_nBalance);

	SetModified();
}

BOOL CPPagePlayback::OnToolTipNotify(UINT id, NMHDR * pNMHDR, LRESULT * pResult)
{
	TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;

	UINT_PTR nID = pNMHDR->idFrom;

	if (pTTT->uFlags & TTF_IDISHWND) {
		nID = ::GetDlgCtrlID((HWND)nID);
	}

	if (nID == 0) {
		return FALSE;
	}

	static CString strTipText;

	if (nID == IDC_SLIDER1) {
		strTipText.Format(L"%d%%", m_nVolume);
	} else if (nID == IDC_SLIDER2) {
		if (m_nBalance > 0) {
			strTipText.Format(L"R +%d%%", m_nBalance);
		} else if (m_nBalance < 0) {
			strTipText.Format(L"L +%d%%", -m_nBalance);
		} else { //if (m_nBalance == 0)
			strTipText = L"L = R";
		}
	} else {
		return FALSE;
	}

	pTTT->lpszText = (LPWSTR)(LPCWSTR)strTipText;

	*pResult = 0;

	return TRUE;
}

void CPPagePlayback::OnCancel()
{
	CAppSettings& s = AfxGetAppSettings();

	if (m_nVolume != m_oldVolume) {
		AfxGetMainFrame()->m_wndToolBar.Volume = m_oldVolume;    //not very nice solution
	}

	if (m_nBalance != s.nBalance) {
		AfxGetMainFrame()->SetBalance(s.nBalance);
	}

	__super::OnCancel();
}

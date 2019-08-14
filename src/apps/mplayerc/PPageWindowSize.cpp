/*
 * (C) 2019 see Authors.txt
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
#include "PPageWindowSize.h"

// CPPageWindowSize dialog

IMPLEMENT_DYNAMIC(CPPageWindowSize, CPPageBase)
CPPageWindowSize::CPPageWindowSize()
	: CPPageBase(CPPageWindowSize::IDD, CPPageWindowSize::IDD)
{
}

CPPageWindowSize::~CPPageWindowSize()
{
}

void CPPageWindowSize::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Radio(pDX, IDC_RADIO1, m_nRadioStartup);
	DDX_Text(pDX, IDC_EDIT1, m_iWindowWidth);
	DDX_Text(pDX, IDC_EDIT2, m_iWindowHeigth);
	DDX_Control(pDX, IDC_BUTTON1, m_btnCurrentSize);

	DDX_Radio(pDX, IDC_RADIO5, m_nRadioPlayback);
	DDX_Control(pDX, IDC_COMBO1, m_cmbScaleLevel);
	DDX_Text(pDX, IDC_EDIT3, m_nFitFactor);
	DDX_Control(pDX, IDC_SPIN3, m_spnFitFactor);
	DDX_Control(pDX, IDC_CHECK1, m_chkFitLargerOnly);

	DDX_Control(pDX, IDC_CHECK2, m_chkRememberWindowPos);
	DDX_Control(pDX, IDC_CHECK3, m_chkLimitWindowProportions);
	DDX_Control(pDX, IDC_CHECK4, m_chkSnapToDesktopEdges);
}

BOOL CPPageWindowSize::OnInitDialog()
{
	__super::OnInitDialog();

	CAppSettings& s = AfxGetAppSettings();

	m_nRadioStartup = s.nStartupWindowMode;
	m_iWindowWidth  = s.szSpecifiedWndSize.cx;
	m_iWindowHeigth = s.szSpecifiedWndSize.cy;

	if (s.nPlaybackWindowMode == PLAYBACKWND_FITSCREENLARGER) {
		m_nRadioPlayback = PLAYBACKWND_FITSCREEN;
		m_chkFitLargerOnly.SetCheck(BST_CHECKED);
	} else {
		m_nRadioPlayback = s.nPlaybackWindowMode;
		m_chkFitLargerOnly.SetCheck(BST_UNCHECKED);
	}

	m_cmbScaleLevel.AddString(L"50%");
	m_cmbScaleLevel.AddString(L"100%");
	m_cmbScaleLevel.AddString(L"200%");
	m_cmbScaleLevel.SetCurSel(s.iZoomLevel);

	m_nFitFactor = s.nAutoFitFactor;
	m_spnFitFactor.SetRange(20, 80);

	m_chkRememberWindowPos.SetCheck(s.bRememberWindowPos);
	m_chkLimitWindowProportions.SetCheck(s.bLimitWindowProportions);
	m_chkSnapToDesktopEdges.SetCheck(s.bSnapToDesktopEdges);

	UpdateData(FALSE);

	OnRadioStartupClicked(m_nRadioStartup + IDC_RADIO1);
	OnRadioPlaybackClicked(m_nRadioPlayback + IDC_RADIO5);

	return TRUE;
}

BOOL CPPageWindowSize::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	s.nStartupWindowMode    = m_nRadioStartup;
	s.szSpecifiedWndSize.cx = m_iWindowWidth;
	s.szSpecifiedWndSize.cy = m_iWindowHeigth;

	if (m_nRadioPlayback == PLAYBACKWND_FITSCREEN && m_chkFitLargerOnly.GetCheck()) {
		s.nPlaybackWindowMode = PLAYBACKWND_FITSCREENLARGER;
	} else {
		s.nPlaybackWindowMode = m_nRadioPlayback;
	}

	s.iZoomLevel = m_cmbScaleLevel.GetCurSel();
	s.nAutoFitFactor = m_nFitFactor = std::clamp(m_nFitFactor, 20, 80);

	s.bRememberWindowPos      = !!m_chkRememberWindowPos.GetCheck();
	s.bLimitWindowProportions = !!m_chkLimitWindowProportions.GetCheck();
	s.bSnapToDesktopEdges     = !!m_chkSnapToDesktopEdges.GetCheck();

	return __super::OnApply();
}

BEGIN_MESSAGE_MAP(CPPageWindowSize, CPPageBase)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_RADIO1, IDC_RADIO4, OnRadioStartupClicked)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_RADIO5, IDC_RADIO7, OnRadioPlaybackClicked)
	ON_BN_CLICKED(IDC_BUTTON1, OnBtnCurrentSizeClicked)
END_MESSAGE_MAP()

// CPPageWindowSize message handlers

void CPPageWindowSize::OnRadioStartupClicked(UINT nID)
{
	UpdateData();

	if (nID == IDC_RADIO3) {
		GetDlgItem(IDC_EDIT1)->EnableWindow(TRUE);
		GetDlgItem(IDC_EDIT2)->EnableWindow(TRUE);
		m_btnCurrentSize.EnableWindow(TRUE);
	} else {
		GetDlgItem(IDC_EDIT1)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT2)->EnableWindow(FALSE);
		m_btnCurrentSize.EnableWindow(FALSE);
	}
}

void CPPageWindowSize::OnRadioPlaybackClicked(UINT nID)
{
	UpdateData();

	m_cmbScaleLevel.EnableWindow(FALSE);
	GetDlgItem(IDC_EDIT3)->EnableWindow(FALSE);
	m_spnFitFactor.EnableWindow(FALSE);
	GetDlgItem(IDC_STATIC1)->EnableWindow(FALSE);
	m_chkFitLargerOnly.EnableWindow(FALSE);

	if (nID == IDC_RADIO6) {
		m_cmbScaleLevel.EnableWindow(TRUE);
	}
	else if (nID == IDC_RADIO7) {
		GetDlgItem(IDC_EDIT3)->EnableWindow(TRUE);
		m_spnFitFactor.EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC1)->EnableWindow(TRUE);
		m_chkFitLargerOnly.EnableWindow(TRUE);
	}
}

void CPPageWindowSize::OnBtnCurrentSizeClicked()
{
	CRect rect;
	AfxGetMainFrame()->GetWindowRect(&rect);
	m_iWindowWidth = rect.Width();
	m_iWindowHeigth = rect.Height();

	UpdateData(FALSE);
}

/*
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
#include "PPageColor.h"


// CPPageColor dialog

IMPLEMENT_DYNAMIC(CPPageColor, CPPageBase)
CPPageColor::CPPageColor()
	: CPPageBase(CPPageColor::IDD, CPPageColor::IDD)
{
}

CPPageColor::~CPPageColor()
{
}

void CPPageColor::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	// Color control
	DDX_Control(pDX, IDC_SLI_BRIGHTNESS, m_SliBrightness);
	DDX_Control(pDX, IDC_SLI_CONTRAST, m_SliContrast);
	DDX_Control(pDX, IDC_SLI_HUE, m_SliHue);
	DDX_Control(pDX, IDC_SLI_SATURATION, m_SliSaturation);
	DDX_Text(pDX, IDC_STATIC1, m_sBrightness);
	DDX_Text(pDX, IDC_STATIC2, m_sContrast);
	DDX_Text(pDX, IDC_STATIC3, m_sHue);
	DDX_Text(pDX, IDC_STATIC4, m_sSaturation);
	// Color managment
	DDX_Control(pDX, IDC_CHECK3, m_chkColorManagment);
	DDX_Control(pDX, IDC_COMBO3, m_cbCMInputType);
	DDX_Control(pDX, IDC_COMBO4, m_cbCMAmbientLight);
	DDX_Control(pDX, IDC_COMBO5, m_cbCMRenderingIntent);
}

BEGIN_MESSAGE_MAP(CPPageColor, CPPageBase)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_CHECK3, OnColorManagmentCheck)
	ON_BN_CLICKED(IDC_RESET, OnBnClickedReset)
END_MESSAGE_MAP()

// CPPageColor message handlers

BOOL CPPageColor::OnInitDialog()
{
	__super::OnInitDialog();

	SetCursor(m_hWnd, IDC_COMBO3, IDC_HAND);
	SetCursor(m_hWnd, IDC_COMBO4, IDC_HAND);
	SetCursor(m_hWnd, IDC_COMBO5, IDC_HAND);
	CreateToolTip();

	auto& rs = GetRenderersSettings();

	// Color control
	m_iBrightness = rs.iBrightness;
	m_iContrast   = rs.iContrast;
	m_iHue        = rs.iHue;
	m_iSaturation = rs.iSaturation;

	m_SliBrightness.SetRange		(-100, 100, TRUE);
	m_SliBrightness.SetTic			(0);
	m_SliBrightness.SetPos			(m_iBrightness);
	m_SliBrightness.SetPageSize		(10);

	m_SliContrast.SetRange			(-100, 100, TRUE);
	m_SliContrast.SetTic			(0);
	m_SliContrast.SetPos			(m_iContrast);
	m_SliContrast.SetPageSize		(10);

	m_SliHue.SetRange				(-180, 180, TRUE);
	m_SliHue.SetTic					(0);
	m_SliHue.SetPos					(m_iHue);
	m_SliHue.SetPageSize			(10);

	m_SliSaturation.SetRange		(-100, 100, TRUE);
	m_SliSaturation.SetTic			(0);
	m_SliSaturation.SetPos			(m_iSaturation);
	m_SliSaturation.SetPageSize		(10);

	if (m_iBrightness) { m_sBrightness.Format(L"%+d", m_iBrightness); } else { m_sBrightness = L"0"; }
	if (m_iContrast  ) { m_sContrast.Format  (L"%+d", m_iContrast);   } else { m_sContrast   = L"0"; }
	if (m_iHue       ) { m_sHue.Format       (L"%+d", m_iHue);        } else { m_sHue        = L"0"; }
	if (m_iSaturation) { m_sSaturation.Format(L"%+d", m_iSaturation); } else { m_sSaturation = L"0"; }

	// Color managment
	CorrectCWndWidth(&m_chkColorManagment);
	m_chkColorManagment.SetCheck(rs.ExtraSets.bColorManagementEnable);

	m_cbCMInputType.AddString(ResStr(IDS_CM_INPUT_AUTO));
	m_cbCMInputType.AddString(L"HDTV");
	m_cbCMInputType.AddString(L"SDTV NTSC");
	m_cbCMInputType.AddString(L"SDTV PAL");
	m_cbCMInputType.SetCurSel(rs.ExtraSets.iColorManagementInput);

	m_cbCMAmbientLight.AddString(ResStr(IDS_CM_AMBIENTLIGHT_BRIGHT));
	m_cbCMAmbientLight.AddString(ResStr(IDS_CM_AMBIENTLIGHT_DIM));
	m_cbCMAmbientLight.AddString(ResStr(IDS_CM_AMBIENTLIGHT_DARK));
	m_cbCMAmbientLight.SetCurSel(rs.ExtraSets.iColorManagementAmbientLight);

	m_cbCMRenderingIntent.AddString(ResStr(IDS_CM_INTENT_PERCEPTUAL));
	m_cbCMRenderingIntent.AddString(ResStr(IDS_CM_INTENT_RELATIVECM));
	m_cbCMRenderingIntent.AddString(ResStr(IDS_CM_INTENT_SATURATION));
	m_cbCMRenderingIntent.AddString(ResStr(IDS_CM_INTENT_ABSOLUTECM));
	m_cbCMRenderingIntent.SetCurSel(rs.ExtraSets.iColorManagementIntent);

	CorrectComboListWidth(m_cbCMInputType);
	CorrectComboListWidth(m_cbCMAmbientLight);
	CorrectComboListWidth(m_cbCMRenderingIntent);

	UpdateData(FALSE);

	return TRUE;
}

BOOL CPPageColor::OnSetActive()
{
	auto& rs = GetRenderersSettings();

	if (rs.iVideoRenderer == VIDRNDT_EVR_CP
			&& (rs.ExtraSets.iSurfaceFormat == D3DFMT_A2R10G10B10 || rs.ExtraSets.iSurfaceFormat == D3DFMT_A16B16G16R16F)) {
		m_chkColorManagment.EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC5)->EnableWindow(TRUE);
		UpdateColorManagment();
	} else {
		m_chkColorManagment.EnableWindow(FALSE);
		m_cbCMInputType.EnableWindow(FALSE);
		m_cbCMAmbientLight.EnableWindow(FALSE);
		m_cbCMRenderingIntent.EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC5)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC6)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC7)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC8)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC9)->EnableWindow(FALSE);
	}

	return __super::OnSetActive();
}

BOOL CPPageColor::OnApply()
{
	UpdateData();

	auto& rs = GetRenderersSettings();

	// Color control
	rs.iBrightness = m_iBrightness;
	rs.iContrast   = m_iContrast;
	rs.iHue        = m_iHue;
	rs.iSaturation = m_iSaturation;

	// Color managment
	rs.ExtraSets.bColorManagementEnable       = !!m_chkColorManagment.GetCheck();
	rs.ExtraSets.iColorManagementInput        = m_cbCMInputType.GetCurSel();
	rs.ExtraSets.iColorManagementAmbientLight = m_cbCMAmbientLight.GetCurSel();
	rs.ExtraSets.iColorManagementIntent       = m_cbCMRenderingIntent.GetCurSel();

	AfxGetMainFrame()->ApplyExraRendererSettings();

	return __super::OnApply();
}

void CPPageColor::UpdateColorManagment()
{
	if (m_chkColorManagment.GetCheck() == BST_CHECKED) {
		m_cbCMInputType.EnableWindow(TRUE);
		m_cbCMAmbientLight.EnableWindow(TRUE);
		m_cbCMRenderingIntent.EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC7)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC8)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC9)->EnableWindow(TRUE);
	}
	else {
		m_cbCMInputType.EnableWindow(FALSE);
		m_cbCMAmbientLight.EnableWindow(FALSE);
		m_cbCMRenderingIntent.EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC7)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC8)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC9)->EnableWindow(FALSE);
	}
	// don't use SetModified() here
}


void CPPageColor::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CMainFrame* pMainFrame = AfxGetMainFrame();
	UpdateData();

	if (*pScrollBar == m_SliBrightness) {
		m_iBrightness = m_SliBrightness.GetPos();
		pMainFrame->SetColorControl(ProcAmp_Brightness, m_iBrightness, m_iContrast, m_iHue, m_iSaturation);
		if (m_iBrightness) {
			m_sBrightness.Format(L"%+d", m_iBrightness);
		} else {
			m_sBrightness = L"0";
		}
	}
	else if (*pScrollBar == m_SliContrast) {
		m_iContrast = m_SliContrast.GetPos();
		pMainFrame->SetColorControl(ProcAmp_Contrast, m_iBrightness, m_iContrast, m_iHue, m_iSaturation);
		if (m_iContrast) {
			m_sContrast.Format(L"%+d", m_iContrast);
		} else {
			m_sContrast = L"0";
		}
	}
	else if (*pScrollBar == m_SliHue) {
		m_iHue = m_SliHue.GetPos();
		pMainFrame->SetColorControl(ProcAmp_Hue, m_iBrightness, m_iContrast, m_iHue, m_iSaturation);
		if (m_iHue) {
			m_sHue.Format(L"%+d", m_iHue);
		} else {
			m_sHue = L"0";
		}
	}
	else if (*pScrollBar == m_SliSaturation) {
		m_iSaturation = m_SliSaturation.GetPos();
		pMainFrame->SetColorControl(ProcAmp_Saturation, m_iBrightness, m_iContrast, m_iHue, m_iSaturation);
		if (m_iSaturation) {
			m_sSaturation.Format(L"%+d", m_iSaturation);
		} else {
			m_sSaturation = L"0";
		}
	}

	UpdateData(FALSE);

	SetModified();

	__super::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CPPageColor::OnColorManagmentCheck()
{
	UpdateColorManagment();

	SetModified();
}

void CPPageColor::OnBnClickedReset()
{
	CMainFrame* pMainFrame = AfxGetMainFrame();

	// Color control
	pMainFrame->m_ColorControl.GetDefaultValues(m_iBrightness, m_iContrast, m_iHue, m_iSaturation);

	m_SliBrightness.SetPos	(m_iBrightness);
	m_SliContrast.SetPos	(m_iContrast);
	m_SliHue.SetPos			(m_iHue);
	m_SliSaturation.SetPos	(m_iSaturation);

	if (m_iBrightness) { m_sBrightness.Format(L"%+d", m_iBrightness); } else { m_sBrightness = L"0"; }
	if (m_iContrast  ) { m_sContrast.Format  (L"%+d", m_iContrast);   } else { m_sContrast   = L"0"; }
	if (m_iHue       ) { m_sHue.Format       (L"%+d", m_iHue);        } else { m_sHue        = L"0"; }
	if (m_iSaturation) { m_sSaturation.Format(L"%+d", m_iSaturation); } else { m_sSaturation = L"0"; }

	pMainFrame->SetColorControl(ProcAmp_All, m_iBrightness, m_iContrast, m_iHue, m_iSaturation);

	// Color managment
	m_chkColorManagment.SetCheck(BST_UNCHECKED);
	m_cbCMInputType.SetCurSel(0);
	m_cbCMAmbientLight.SetCurSel(0);
	m_cbCMRenderingIntent.SetCurSel(0);
	UpdateColorManagment();

	UpdateData(FALSE);

	SetModified();
}

void CPPageColor::OnCancel()
{
	auto& rs = GetRenderersSettings();

	AfxGetMainFrame()->SetColorControl(ProcAmp_All, rs.iBrightness, rs.iContrast, rs.iHue, rs.iSaturation);

	__super::OnCancel();
}

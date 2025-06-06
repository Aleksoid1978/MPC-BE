/*
 * (C) 2025 see Authors.txt
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
#include "PPageOSD.h"

int CALLBACK EnumFontProc(ENUMLOGFONT FAR* lf, NEWTEXTMETRIC FAR* tm, int FontType, LPARAM dwData)
{
	std::vector<CString>* fontnames = (std::vector<CString>*)dwData;

	if (FontType == TRUETYPE_FONTTYPE) {
		if (fontnames->empty() || fontnames->back().Compare(lf->elfFullName) != 0) {
			fontnames->emplace_back(lf->elfFullName);
		}
	}

	return true;
}

// CPPageOSD dialog

IMPLEMENT_DYNAMIC(CPPageOSD, CPPageBase)
CPPageOSD::CPPageOSD()
	: CPPageBase(CPPageOSD::IDD, CPPageOSD::IDD)
{
}

void CPPageOSD::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Check(pDX, IDC_SHOW_OSD, m_bShowOSD);
	DDX_Check(pDX, IDC_CHECK14,  m_bOSDFileName);
	DDX_Check(pDX, IDC_CHECK15,  m_bOSDSeekTime);
	
	DDX_Control(pDX, IDC_COMBO1, m_OSDFontType);
	DDX_Control(pDX, IDC_EDIT3, m_edOSDFontSize);
	DDX_Control(pDX, IDC_SPIN3, m_spOSDFontSize);
	DDX_Control(pDX, IDC_SLIDER_OSDTRANS, m_OSDTransparentCtrl);
	DDX_Slider(pDX, IDC_SLIDER_OSDTRANS, m_nOSDTransparent);
	DDX_Text(pDX, IDC_EDIT4, m_OSDBorder);
	DDX_Control(pDX, IDC_SPIN10, m_OSDBorderCtrl);
	DDX_Check(pDX, IDC_CHECK_SHADOW, m_bOSDFontShadow);
	DDX_Check(pDX, IDC_CHECK_AA, m_bOSDFontAA);
}

BEGIN_MESSAGE_MAP(CPPageOSD, CPPageBase)
	ON_UPDATE_COMMAND_UI(IDC_CHECK14, OnUpdateOSD)
	ON_UPDATE_COMMAND_UI(IDC_CHECK15, OnUpdateOSD)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnChangeOSD)
	ON_UPDATE_COMMAND_UI(IDC_SLIDER_OSDTRANS, OnUpdateOSDTransparent)
	ON_COMMAND(IDC_CHECK_SHADOW, OnCheckShadow)
	ON_COMMAND(IDC_CHECK_AA, OnCheckAA)
	ON_UPDATE_COMMAND_UI(IDC_EDIT4, OnUpdateOSDBorder)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN3, OnDeltaposSpin3)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_BUTTON_CLRFONT, OnCustomDrawBtns)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_BUTTON_CLRGRAD1, OnCustomDrawBtns)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_BUTTON_CLRGRAD2, OnCustomDrawBtns)
	ON_BN_CLICKED(IDC_BUTTON_CLRFONT, OnClickClrFont)
	ON_BN_CLICKED(IDC_BUTTON_CLRGRAD1, OnClickClrGrad1)
	ON_BN_CLICKED(IDC_BUTTON_CLRGRAD2, OnClickClrGrad2)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

// CPPageOSD message handlers

BOOL CPPageOSD::OnInitDialog()
{
	__super::OnInitDialog();

	CAppSettings& s = AfxGetAppSettings();

	m_bShowOSD      = s.ShowOSD.Enable;
	m_bOSDFileName  = s.ShowOSD.FileName;
	m_bOSDSeekTime  = s.ShowOSD.SeekTime;

	m_nOSDTransparent = m_nOSDTransparent_Old = s.nOSDTransparent;
	m_OSDBorder       = m_OSDBorder_Old       = s.nOSDBorder;

	m_OSDTransparentCtrl.SetRange(0, 255, TRUE);
	m_OSDBorderCtrl.SetRange32(0, 5);

	m_OSD_Font = s.strOSDFont;

	m_OSDFontType.Clear();
	m_bOSDFontShadow = m_bOSDFontShadow_Old = s.bOSDFontShadow;
	m_bOSDFontAA     = m_bOSDFontAA_Old     = s.bOSDFontAA;
	m_clrFontABGR    = m_clrFontABGR_Old    = s.clrFontABGR;
	m_clrGrad1ABGR   = m_clrGrad1ABGR_Old   = s.clrGrad1ABGR;
	m_clrGrad2ABGR   = m_clrGrad2ABGR_Old   = s.clrGrad2ABGR;

	HDC dc = CreateDCW(L"DISPLAY", nullptr, nullptr, nullptr);
	std::vector<CString> fontnames;
	EnumFontFamiliesW(dc, nullptr, (FONTENUMPROCW)EnumFontProc, (LPARAM)&fontnames);
	DeleteDC(dc);

	for (const auto& fontname : fontnames) {
		m_OSDFontType.AddString(fontname);
	}

	CorrectComboListWidth(m_OSDFontType);
	int iSel = m_OSDFontType.FindStringExact(0, m_OSD_Font);
	if (iSel == CB_ERR) {
		iSel = 0;
	}
	m_OSDFontType.SetCurSel(iSel);

	m_edOSDFontSize.SetRange(8, 40);
	m_spOSDFontSize.SetRange(8, 40);
	m_edOSDFontSize = s.nOSDSize;
	
	UpdateData(FALSE);

	return TRUE;
}

BOOL CPPageOSD::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();
	auto pFrame = AfxGetMainFrame();

	BOOL bShowOSDChanged = s.ShowOSD.Enable != m_bShowOSD;

	s.ShowOSD.Enable   = m_bShowOSD ? 1 : 0;
	s.ShowOSD.FileName = m_bOSDFileName ? 1 : 0;
	s.ShowOSD.SeekTime = m_bOSDSeekTime ? 1 : 0;
	if (bShowOSDChanged) {
		if (s.ShowOSD.Enable) {
			pFrame->m_OSD.Start(pFrame->m_pOSDWnd);
			pFrame->OSDBarSetPos();
			pFrame->m_OSD.ClearMessage(false);
		} else {
			pFrame->m_OSD.Stop();
		}
	}

	s.nOSDTransparent = m_nOSDTransparent;
	s.nOSDBorder      = m_OSDBorder;
	s.nOSDSize        = m_edOSDFontSize;
	m_OSDFontType.GetLBText(m_OSDFontType.GetCurSel(), s.strOSDFont);
	s.bOSDFontShadow = !!m_bOSDFontShadow;
	s.bOSDFontAA     = !!m_bOSDFontAA;

	s.clrFontABGR  = m_clrFontABGR;
	s.clrGrad1ABGR = m_clrGrad1ABGR;
	s.clrGrad2ABGR = m_clrGrad2ABGR;

	m_OSDBorder_Old       = s.nOSDBorder;
	m_bOSDFontShadow_Old  = s.bOSDFontShadow;
	m_bOSDFontAA_Old      = s.bOSDFontAA;
	m_nOSDTransparent_Old = s.nOSDTransparent;

	m_clrFontABGR_Old  = s.clrFontABGR;
	m_clrGrad1ABGR_Old = s.clrGrad1ABGR;
	m_clrGrad2ABGR_Old = s.clrGrad2ABGR;

	return __super::OnApply();
}

void CPPageOSD::OnCancel()
{
	CAppSettings& s = AfxGetAppSettings();

	s.nOSDBorder      = m_OSDBorder_Old;
	s.bOSDFontShadow  = !!m_bOSDFontShadow_Old;
	s.bOSDFontAA      = !!m_bOSDFontAA_Old;
	s.nOSDTransparent = m_nOSDTransparent_Old;

	s.clrFontABGR  = m_clrFontABGR_Old;
	s.clrGrad1ABGR = m_clrGrad1ABGR_Old;
	s.clrGrad2ABGR = m_clrGrad2ABGR_Old;
}

void CPPageOSD::OnUpdateOSD(CCmdUI* pCmdUI)
{
	UpdateData();

	pCmdUI->Enable(!!m_bShowOSD);
}

void CPPageOSD::OnChangeOSD()
{
	auto pFrame = AfxGetMainFrame();
	if (pFrame->m_OSD) {
		CString str;
		int osdFontSize = m_edOSDFontSize;
		m_OSDFontType.GetLBText(m_OSDFontType.GetCurSel(), str);

		pFrame->m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_OSD_TEST), 2000, false, osdFontSize, str);
		pFrame->m_OSD.SetLayeredWindowAttributes(RGB(255, 0, 255), 255 - AfxGetAppSettings().nOSDTransparent, LWA_ALPHA | LWA_COLORKEY);
	}

	SetModified();
}

void CPPageOSD::OnCheckShadow()
{
	CAppSettings& s = AfxGetAppSettings();

	UpdateData();
	BOOL fFontShadow = s.bOSDFontShadow;
	s.bOSDFontShadow = !!m_bOSDFontShadow;
	OnChangeOSD();

	s.bOSDFontShadow = !!fFontShadow;
}

void CPPageOSD::OnCheckAA()
{
	CAppSettings& s = AfxGetAppSettings();

	UpdateData();
	BOOL fFontAA = s.bOSDFontAA;
	s.bOSDFontAA = !!m_bOSDFontAA;
	OnChangeOSD();

	s.bOSDFontAA = !!fFontAA;
}

void CPPageOSD::OnUpdateOSDBorder(CCmdUI* pCmdUI)
{
	CAppSettings& s = AfxGetAppSettings();

	if (s.nOSDBorder != m_OSDBorder) {
		UpdateData();
		int nOSDBorder = s.nOSDBorder;
		s.nOSDBorder = m_OSDBorder;
		OnChangeOSD();

		s.nOSDBorder = nOSDBorder;
	}
}

void CPPageOSD::OnUpdateOSDTransparent(CCmdUI* pCmdUI)
{
	UpdateData();
}

void CPPageOSD::OnClickClrFont()
{
	CColorDialog clrpicker;
	clrpicker.m_cc.Flags |= CC_FULLOPEN | CC_RGBINIT;
	clrpicker.m_cc.rgbResult = m_clrFontABGR;

	if (clrpicker.DoModal() == IDOK) {
		COLORREF clrFontABGR = m_clrFontABGR;
		m_clrFontABGR = clrpicker.GetColor();
		if (clrFontABGR != clrpicker.GetColor()) {
			CAppSettings& s = AfxGetAppSettings();
			s.clrFontABGR = m_clrFontABGR;
			OnChangeOSD();
			UpdateData(FALSE);
		}
	}
}

void CPPageOSD::OnClickClrGrad1()
{
	CColorDialog clrpicker;
	clrpicker.m_cc.Flags |= CC_FULLOPEN | CC_RGBINIT;
	clrpicker.m_cc.rgbResult = m_clrGrad1ABGR;

	if (clrpicker.DoModal() == IDOK) {
		COLORREF clrGrad1ABGR = m_clrGrad1ABGR;
		m_clrGrad1ABGR = clrpicker.GetColor();
		if (clrGrad1ABGR != clrpicker.GetColor()) {
			CAppSettings& s = AfxGetAppSettings();
			s.clrGrad1ABGR = m_clrGrad1ABGR;
			OnChangeOSD();
			UpdateData(FALSE);
		}
	}
}

void CPPageOSD::OnClickClrGrad2()
{
	CColorDialog clrpicker;
	clrpicker.m_cc.Flags |= CC_FULLOPEN | CC_RGBINIT;
	clrpicker.m_cc.rgbResult = m_clrGrad2ABGR;

	if (clrpicker.DoModal() == IDOK) {
		COLORREF clrGrad2ABGR = m_clrGrad2ABGR;
		m_clrGrad2ABGR = clrpicker.GetColor();
		if (clrGrad2ABGR != clrpicker.GetColor()) {
			CAppSettings& s = AfxGetAppSettings();
			s.clrGrad2ABGR = m_clrGrad2ABGR;
			OnChangeOSD();
			UpdateData(FALSE);
		}
	}
}

void CPPageOSD::OnDeltaposSpin3(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);

	if (pNMUpDown != nullptr) {
		if (pNMUpDown->iDelta != 0) {
			OnChangeOSD();
		}
	}

	*pResult = 0;
}

void CPPageOSD::OnCustomDrawBtns(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	*pResult = CDRF_DODEFAULT;

	if (pNMCD->dwDrawStage == CDDS_PREPAINT) {
		if (pNMCD->dwItemSpec == IDC_BUTTON_CLRFONT || pNMCD->dwItemSpec == IDC_BUTTON_CLRGRAD1 || pNMCD->dwItemSpec == IDC_BUTTON_CLRGRAD2) {
			CDC dc;
			dc.Attach(pNMCD->hdc);
			CRect r;
			CopyRect(&r, &pNMCD->rc);
			CPen penFrEnabled(PS_SOLID, 0, GetSysColor(COLOR_BTNTEXT));
			CPen penFrDisabled(PS_SOLID, 0, GetSysColor(COLOR_BTNSHADOW));
			CPen* penOld = dc.SelectObject(&penFrEnabled);

			if (CDIS_HOT == pNMCD->uItemState || CDIS_HOT + CDIS_FOCUS == pNMCD->uItemState || CDIS_DISABLED == pNMCD->uItemState) {
				dc.SelectObject(&penFrDisabled);
			}

			dc.RoundRect(r.left, r.top, r.right, r.bottom, 6, 4);
			r.DeflateRect(2, 2, 2, 2);
			switch (pNMCD->dwItemSpec) {
			case IDC_BUTTON_CLRFONT:
				dc.FillSolidRect(&r, m_clrFontABGR);
				break;
			case IDC_BUTTON_CLRGRAD1:
				dc.FillSolidRect(&r, m_clrGrad1ABGR);
				break;
			case IDC_BUTTON_CLRGRAD2:
				dc.FillSolidRect(&r, m_clrGrad2ABGR);
				break;
			}

			dc.SelectObject(&penOld);
			dc.Detach();

			*pResult = CDRF_SKIPDEFAULT;
		}
	}
}

void CPPageOSD::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CAppSettings& s = AfxGetAppSettings();

	if (*pScrollBar == m_OSDTransparentCtrl) {
		UpdateData();
		int nOSDTransparent = s.nOSDTransparent;
		s.nOSDTransparent = m_nOSDTransparent;
		OnChangeOSD();

		s.nOSDTransparent = nOSDTransparent;
	}

	SetModified();

	__super::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CPPageOSD::OnClickClrDefault()
{
	CAppSettings& s = AfxGetAppSettings();

	s.nOSDTransparent = m_nOSDTransparent = 100;

	m_bOSDFontShadow = FALSE;
	m_bOSDFontAA = TRUE;
	m_OSDBorder = 1;

	m_clrFontABGR = 0x00E0E0E0;
	m_clrGrad1ABGR = 0x00302820;
	m_clrGrad2ABGR = 0x00302820;

	m_OSDBorder_Old       = s.nOSDBorder;
	m_bOSDFontShadow_Old  = s.bOSDFontShadow;
	m_bOSDFontAA_Old      = s.bOSDFontAA;
	m_nOSDTransparent_Old = s.nOSDTransparent;

	m_clrFontABGR_Old  = s.clrFontABGR;
	m_clrGrad1ABGR_Old = s.clrGrad1ABGR;
	m_clrGrad2ABGR_Old = s.clrGrad2ABGR;

	UpdateData(FALSE);
}

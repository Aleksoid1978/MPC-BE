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
	
	DDX_Control(pDX, IDC_COMBO1, m_cbFontName);
	DDX_CBString(pDX, IDC_COMBO1, m_FontName);
	DDX_Control(pDX, IDC_EDIT3, m_edFontSize);
	DDX_Control(pDX, IDC_SPIN3, m_spFontSize);
	DDX_Control(pDX, IDC_SLIDER_OSDTRANS, m_TransparentCtrl);
	DDX_Slider(pDX, IDC_SLIDER_OSDTRANS, m_nTransparent);
	DDX_Text(pDX, IDC_EDIT4, m_nBorder);
	DDX_Control(pDX, IDC_SPIN10, m_BorderCtrl);
	DDX_Check(pDX, IDC_CHECK_SHADOW, m_bFontShadow);
	DDX_Check(pDX, IDC_CHECK_AA, m_bFontAA);
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
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedDefault)
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

	m_nTransparent = m_nTransparent_Old = s.nOSDTransparent;
	m_nBorder      = m_nBorder_Old      = s.nOSDBorder;

	m_TransparentCtrl.SetRange(0, 255, TRUE);
	m_BorderCtrl.SetRange32(0, 5);

	m_FontName = s.strOSDFont;

	m_cbFontName.Clear();
	m_bFontShadow = m_bFontShadow_Old = s.bOSDFontShadow;
	m_bFontAA     = m_bFontAA_Old     = s.bOSDFontAA;
	m_colorFont   = m_colorFont_Old   = s.clrFontABGR;
	m_colorGrad1  = m_colorGrad1_Old  = s.clrGrad1ABGR;
	m_colorGrad2  = m_colorGrad2_Old  = s.clrGrad2ABGR;

	HDC dc = CreateDCW(L"DISPLAY", nullptr, nullptr, nullptr);
	std::vector<CString> fontnames;
	EnumFontFamiliesW(dc, nullptr, (FONTENUMPROCW)EnumFontProc, (LPARAM)&fontnames);
	DeleteDC(dc);

	for (const auto& fontname : fontnames) {
		m_cbFontName.AddString(fontname);
	}

	CorrectComboListWidth(m_cbFontName);
	int iSel = m_cbFontName.FindStringExact(0, m_FontName);
	if (iSel == CB_ERR) {
		iSel = 0;
	}
	m_cbFontName.SetCurSel(iSel);

	m_edFontSize.SetRange(8, 40);
	m_spFontSize.SetRange(8, 40);
	m_edFontSize = s.nOSDSize;
	
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

	s.nOSDTransparent = m_nTransparent;
	s.nOSDBorder      = m_nBorder;
	s.nOSDSize        = m_edFontSize;
	m_cbFontName.GetLBText(m_cbFontName.GetCurSel(), s.strOSDFont);
	s.bOSDFontShadow  = !!m_bFontShadow;
	s.bOSDFontAA      = !!m_bFontAA;

	s.clrFontABGR  = m_colorFont;
	s.clrGrad1ABGR = m_colorGrad1;
	s.clrGrad2ABGR = m_colorGrad2;

	m_nBorder_Old      = s.nOSDBorder;
	m_bFontShadow_Old  = s.bOSDFontShadow;
	m_bFontAA_Old      = s.bOSDFontAA;
	m_nTransparent_Old = s.nOSDTransparent;

	m_colorFont_Old  = s.clrFontABGR;
	m_colorGrad1_Old = s.clrGrad1ABGR;
	m_colorGrad2_Old = s.clrGrad2ABGR;

	return __super::OnApply();
}

void CPPageOSD::OnCancel()
{
	CAppSettings& s = AfxGetAppSettings();

	s.nOSDBorder      = m_nBorder_Old;
	s.bOSDFontShadow  = !!m_bFontShadow_Old;
	s.bOSDFontAA      = !!m_bFontAA_Old;
	s.nOSDTransparent = m_nTransparent_Old;

	s.clrFontABGR  = m_colorFont_Old;
	s.clrGrad1ABGR = m_colorGrad1_Old;
	s.clrGrad2ABGR = m_colorGrad2_Old;
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
		int osdFontSize = m_edFontSize;
		m_cbFontName.GetLBText(m_cbFontName.GetCurSel(), str);

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
	s.bOSDFontShadow = !!m_bFontShadow;
	OnChangeOSD();

	s.bOSDFontShadow = !!fFontShadow;
}

void CPPageOSD::OnCheckAA()
{
	CAppSettings& s = AfxGetAppSettings();

	UpdateData();
	BOOL fFontAA = s.bOSDFontAA;
	s.bOSDFontAA = !!m_bFontAA;
	OnChangeOSD();

	s.bOSDFontAA = !!fFontAA;
}

void CPPageOSD::OnUpdateOSDBorder(CCmdUI* pCmdUI)
{
	CAppSettings& s = AfxGetAppSettings();

	if (s.nOSDBorder != m_nBorder) {
		UpdateData();
		int nOSDBorder = s.nOSDBorder;
		s.nOSDBorder = m_nBorder;
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
	clrpicker.m_cc.rgbResult = m_colorFont;

	if (clrpicker.DoModal() == IDOK) {
		COLORREF clrFontABGR = m_colorFont;
		m_colorFont = clrpicker.GetColor();
		if (clrFontABGR != clrpicker.GetColor()) {
			CAppSettings& s = AfxGetAppSettings();
			s.clrFontABGR = m_colorFont;
			OnChangeOSD();
			UpdateData(FALSE);
		}
	}
}

void CPPageOSD::OnClickClrGrad1()
{
	CColorDialog clrpicker;
	clrpicker.m_cc.Flags |= CC_FULLOPEN | CC_RGBINIT;
	clrpicker.m_cc.rgbResult = m_colorGrad1;

	if (clrpicker.DoModal() == IDOK) {
		COLORREF clrGrad1ABGR = m_colorGrad1;
		m_colorGrad1 = clrpicker.GetColor();
		if (clrGrad1ABGR != clrpicker.GetColor()) {
			CAppSettings& s = AfxGetAppSettings();
			s.clrGrad1ABGR = m_colorGrad1;
			OnChangeOSD();
			UpdateData(FALSE);
		}
	}
}

void CPPageOSD::OnClickClrGrad2()
{
	CColorDialog clrpicker;
	clrpicker.m_cc.Flags |= CC_FULLOPEN | CC_RGBINIT;
	clrpicker.m_cc.rgbResult = m_colorGrad2;

	if (clrpicker.DoModal() == IDOK) {
		COLORREF clrGrad2ABGR = m_colorGrad2;
		m_colorGrad2 = clrpicker.GetColor();
		if (clrGrad2ABGR != clrpicker.GetColor()) {
			CAppSettings& s = AfxGetAppSettings();
			s.clrGrad2ABGR = m_colorGrad2;
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
				dc.FillSolidRect(&r, m_colorFont);
				break;
			case IDC_BUTTON_CLRGRAD1:
				dc.FillSolidRect(&r, m_colorGrad1);
				break;
			case IDC_BUTTON_CLRGRAD2:
				dc.FillSolidRect(&r, m_colorGrad2);
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

	if (*pScrollBar == m_TransparentCtrl) {
		UpdateData();
		int nOSDTransparent = s.nOSDTransparent;
		s.nOSDTransparent = m_nTransparent;
		OnChangeOSD();

		s.nOSDTransparent = nOSDTransparent;
	}

	SetModified();

	__super::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CPPageOSD::OnBnClickedDefault()
{
	CAppSettings& s = AfxGetAppSettings();

	m_bShowOSD     = TRUE;
	m_bOSDFileName = FALSE;
	m_bOSDSeekTime = FALSE;

	m_FontName     = L"Segoe UI";
	m_edFontSize   = 18;
	m_bFontShadow  = FALSE;
	m_bFontAA      = TRUE;
	m_nTransparent = s.nOSDTransparent = 100;
	m_nBorder      = 1;
	m_colorFont    = RGB(224, 224, 224);
	m_colorGrad1   = RGB(32, 40, 48);
	m_colorGrad2   = RGB(32, 40, 48);

	UpdateData(FALSE);

	GetDlgItem(IDC_BUTTON_CLRFONT)->Invalidate();
	GetDlgItem(IDC_BUTTON_CLRGRAD1)->Invalidate();
	GetDlgItem(IDC_BUTTON_CLRGRAD2)->Invalidate();

	m_bFontShadow_Old  = s.bOSDFontShadow;
	m_bFontAA_Old      = s.bOSDFontAA;
	m_nTransparent_Old = s.nOSDTransparent;
	m_nBorder_Old      = s.nOSDBorder;
	m_colorFont_Old    = s.clrFontABGR;
	m_colorGrad1_Old   = s.clrGrad1ABGR;
	m_colorGrad2_Old   = s.clrGrad2ABGR;
}

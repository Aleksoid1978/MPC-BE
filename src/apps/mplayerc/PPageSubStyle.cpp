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
#include "PPageSubStyle.h"

// CPPageSubStyle dialog

IMPLEMENT_DYNAMIC(CPPageSubStyle, CPPageBase)
CPPageSubStyle::CPPageSubStyle()
	: CPPageBase(CPPageSubStyle::IDD, CPPageSubStyle::IDD)
	, m_iCharset(0)
	, m_borderstyle(0)
	, m_screenalignment(0)
	, m_linkalphasliders(FALSE)
	, m_relativeTo(FALSE)
	, m_bUseDefaultStyle(TRUE)
	, m_stss(&AfxGetAppSettings().subdefstyle)
	, m_stss_init(AfxGetAppSettings().subdefstyle)
{
}

CPPageSubStyle::~CPPageSubStyle()
{
}

void CPPageSubStyle::InitSubStyle(CString title, STSStyle* stss)
{
	m_pPSP->pszTitle	= (m_title = title);
	m_psp.dwFlags		|= PSP_USETITLE;

	m_stss				= stss;
	m_stss_init			= *m_stss;

	m_bUseDefaultStyle	= FALSE;
}

void CPPageSubStyle::AskColor(int i)
{
	CColorDialog dlg(m_stss->colors[i]);
	dlg.m_cc.Flags |= CC_FULLOPEN|CC_RGBINIT;

	if (dlg.DoModal() == IDOK) {
		if (m_stss->colors[i] != dlg.m_cc.rgbResult) {
			SetModified();
		}
		m_stss->colors[i] = dlg.m_cc.rgbResult;
	}
}

void CPPageSubStyle::DoDataExchange(CDataExchange* pDX)
{
	CPPageBase::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_BUTTON1, m_font);
	DDX_CBIndex(pDX, IDC_COMBO1, m_iCharset);
	DDX_Control(pDX, IDC_COMBO1, m_charset);
	DDX_Control(pDX, IDC_EDIT3, m_spacing);
	DDX_Control(pDX, IDC_SPIN3, m_spacingspin);
	DDX_Control(pDX, IDC_EDIT4, m_angle);
	DDX_Control(pDX, IDC_SPIN10, m_anglespin);
	DDX_Control(pDX, IDC_EDIT5, m_scalex);
	DDX_Control(pDX, IDC_SPIN4, m_scalexspin);
	DDX_Control(pDX, IDC_EDIT6, m_scaley);
	DDX_Control(pDX, IDC_SPIN5, m_scaleyspin);
	DDX_Radio(pDX, IDC_RADIO1, m_borderstyle);
	DDX_Control(pDX, IDC_EDIT1, m_borderwidth);
	DDX_Control(pDX, IDC_SPIN1, m_borderwidthspin);
	DDX_Control(pDX, IDC_EDIT2, m_shadowdepth);
	DDX_Control(pDX, IDC_SPIN2, m_shadowdepthspin);
	DDX_Radio(pDX, IDC_RADIO3, m_screenalignment);
	DDX_Control(pDX, IDC_EDIT7, m_marginleft);
	DDX_Control(pDX, IDC_SPIN6, m_marginleftspin);
	DDX_Control(pDX, IDC_EDIT8, m_marginright);
	DDX_Control(pDX, IDC_SPIN7, m_marginrightspin);
	DDX_Control(pDX, IDC_EDIT9, m_margintop);
	DDX_Control(pDX, IDC_SPIN8, m_margintopspin);
	DDX_Control(pDX, IDC_EDIT10, m_marginbottom);
	DDX_Control(pDX, IDC_SPIN9, m_marginbottomspin);
	DDX_Slider(pDX, IDC_SLIDER1, m_alpha[0]);
	DDX_Slider(pDX, IDC_SLIDER2, m_alpha[1]);
	DDX_Slider(pDX, IDC_SLIDER3, m_alpha[2]);
	DDX_Slider(pDX, IDC_SLIDER4, m_alpha[3]);
	DDX_Control(pDX, IDC_SLIDER1, m_alphasliders[0]);
	DDX_Control(pDX, IDC_SLIDER2, m_alphasliders[1]);
	DDX_Control(pDX, IDC_SLIDER3, m_alphasliders[2]);
	DDX_Control(pDX, IDC_SLIDER4, m_alphasliders[3]);
	DDX_Check(pDX, IDC_CHECK1, m_linkalphasliders);
	DDX_Check(pDX, IDC_CHECK_RELATIVETO, m_relativeTo);
}

BEGIN_MESSAGE_MAP(CPPageSubStyle, CPPageBase)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
	ON_BN_CLICKED(IDC_RESET, OnBnClickedButton2)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_COLORPRI, OnCustomDrawBtns)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_COLORSEC, OnCustomDrawBtns)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_COLOROUTL, OnCustomDrawBtns)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_COLORSHAD, OnCustomDrawBtns)
	ON_BN_CLICKED(IDC_COLORPRI, OnStnClickedColorpri)
	ON_BN_CLICKED(IDC_COLORSEC, OnStnClickedColorsec)
	ON_BN_CLICKED(IDC_COLOROUTL, OnStnClickedColoroutl)
	ON_BN_CLICKED(IDC_COLORSHAD, OnStnClickedColorshad)
	ON_BN_CLICKED(IDC_CHECK1, OnBnClickedCheck1)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

// CPPageSubStyle message handlers

BOOL CPPageSubStyle::OnInitDialog()
{
	__super::OnInitDialog();

	if (!m_bUseDefaultStyle) {
		GetDlgItem(IDC_RESET)->ShowWindow(SW_HIDE);
	}

	SetCursor(m_hWnd, IDC_BUTTON1, IDC_HAND);
	SetCursor(m_hWnd, IDC_RESET, IDC_HAND);
	SetCursor(m_hWnd, IDC_COMBO1, IDC_HAND);

	m_spacing.SetRange(-10000, 10000);
	m_angle.SetRange(0, 359);
	m_scalex.SetRange(-10000, 10000);
	m_scaley.SetRange(-10000, 10000);
	m_borderwidth.SetRange(0, 10000);
	m_shadowdepth.SetRange(0, 10000);
	m_marginleft.SetRange(-10000, 10000);
	m_marginright.SetRange(-10000, 10000);
	m_margintop.SetRange(-10000, 10000);
	m_marginbottom.SetRange(-10000, 10000);

	m_spacingspin.SetRange(-10000, 10000);
	m_anglespin.SetRange(0, 359);
	m_scalexspin.SetRange(-10000, 10000);
	m_scaleyspin.SetRange(-10000, 10000);
	m_borderwidthspin.SetRange(0, 10000);
	m_shadowdepthspin.SetRange(0, 10000);
	m_marginleftspin.SetRange(-10000, 10000);
	m_marginrightspin.SetRange(-10000, 10000);
	m_margintopspin.SetRange(-10000, 10000);
	m_marginbottomspin.SetRange(-10000, 10000);
	for (int i = 0; i < _countof(m_alphasliders); i++) {
		m_alphasliders[i].SetRange(0, 255);
	}

	Init();

	CreateToolTip();

	return TRUE;
}

void CPPageSubStyle::Init()
{
	m_font.SetWindowText(m_stss->fontName);
	m_iCharset = -1;

	for (int i = 0; i < CharSetLen; i++) {
		CString str;
		str.Format(_T("%s (%d)"), CharSetNames[i], CharSetList[i]);
		m_charset.AddString(str);
		m_charset.SetItemData(i, CharSetList[i]);

		if (m_stss->charSet == CharSetList[i]) {
			m_iCharset = i;
		}
	}

	// TODO: allow floats in these edit boxes
	m_spacing = (int)m_stss->fontSpacing;

	while (m_stss->fontAngleZ < 0) {
		m_stss->fontAngleZ += 360;
	}

	m_angle = (int)fmod(m_stss->fontAngleZ, 360);

	m_scalex = (int)m_stss->fontScaleX;

	m_scaley = (int)m_stss->fontScaleY;


	m_borderstyle = m_stss->borderStyle;
	m_borderwidth = (int)min(m_stss->outlineWidthX, m_stss->outlineWidthY);
	m_shadowdepth = (int)min(m_stss->shadowDepthX, m_stss->shadowDepthY);

	m_screenalignment = m_stss->scrAlignment-1;
	m_marginleft = m_stss->marginRect.left;
	m_marginright = m_stss->marginRect.right;
	m_margintop = m_stss->marginRect.top;
	m_marginbottom = m_stss->marginRect.bottom;

	m_relativeTo = m_stss->relativeTo;

	for (int i = 0; i < _countof(m_alpha); i++) {
		m_alpha[i] = 255-m_stss->alpha[i];
	}

	m_linkalphasliders = FALSE;

	UpdateData(FALSE);
}

BOOL CPPageSubStyle::OnApply()
{
	UpdateData();

	if (m_iCharset >= 0) {
		m_stss->charSet		= m_charset.GetItemData(m_iCharset);
	}

	m_stss->fontSpacing		= m_spacing;
	m_stss->fontAngleZ		= m_angle;
	m_stss->fontScaleX		= m_scalex;
	m_stss->fontScaleY		= m_scaley;

	m_stss->borderStyle		= m_borderstyle;
	m_stss->outlineWidthX	= m_stss->outlineWidthY	= m_borderwidth;
	m_stss->shadowDepthX	= m_stss->shadowDepthY	= m_shadowdepth;

	m_stss->scrAlignment	= m_screenalignment + 1;
	m_stss->marginRect		= CRect(m_marginleft, m_margintop, m_marginright, m_marginbottom);
	m_stss->relativeTo		= m_relativeTo;

	for (int i = 0; i < _countof(m_alpha); i++) {
		m_stss->alpha[i]	= 255 - m_alpha[i];
	}

	BOOL bStyleChanged = (m_stss_init != *m_stss);
	if (bStyleChanged) {
		if (auto pFrame = AfxFindMainFrame()) {
			pFrame->UpdateSubDefaultStyle();
		}
	}

	m_stss_init = *m_stss;

	return __super::OnApply();
}

void CPPageSubStyle::OnBnClickedButton1()
{
	LOGFONT lf;
	lf <<= *m_stss;

	CFontDialog dlg(&lf, CF_SCREENFONTS|CF_INITTOLOGFONTSTRUCT|CF_FORCEFONTEXIST|CF_SCALABLEONLY);

	if (dlg.DoModal() == IDOK) {
		CString str(lf.lfFaceName);

		if (str.GetLength() > 16) {
			str = str.Left(14) + _T("...");
		}

		m_font.SetWindowText(str);

		SelectByItemData(m_charset, lf.lfCharSet);

		*m_stss = lf;

		SetModified();
	}
}

void CPPageSubStyle::OnBnClickedButton2()
{
	m_stss->SetDefault();

	Init();
	SetModified();
}

void CPPageSubStyle::OnStnClickedColorpri()
{
	AskColor(0);
}

void CPPageSubStyle::OnStnClickedColorsec()
{
	AskColor(1);
}

void CPPageSubStyle::OnStnClickedColoroutl()
{
	AskColor(2);
}

void CPPageSubStyle::OnStnClickedColorshad()
{
	AskColor(3);
}

void CPPageSubStyle::OnBnClickedCheck1()
{
	UpdateData();

	int avg = 0;

	for (int i = 0; i < _countof(m_alphasliders); i++) {
		avg += m_alphasliders[i].GetPos();
	}

	avg /= 4;

	for (int i = 0; i < _countof(m_alphasliders); i++) {
		m_alphasliders[i].SetPos(avg);
	}

	SetModified();
}

void CPPageSubStyle::OnCustomDrawBtns(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	*pResult = CDRF_DODEFAULT;

	if (pNMCD->dwItemSpec == IDC_COLORPRI
		|| pNMCD->dwItemSpec == IDC_COLORSEC
		|| pNMCD->dwItemSpec == IDC_COLOROUTL
		|| pNMCD->dwItemSpec == IDC_COLORSHAD) {
		if (pNMCD->dwDrawStage == CDDS_PREPAINT) {
			CDC dc;
			dc.Attach(pNMCD->hdc);
			CRect r;
			CopyRect(&r,&pNMCD->rc);
			CPen penFrEnabled (PS_SOLID, 0, GetSysColor(COLOR_BTNTEXT));
			CPen penFrDisabled (PS_SOLID, 0, GetSysColor(COLOR_BTNSHADOW));
			CPen *penOld = dc.SelectObject(&penFrEnabled);

			if (CDIS_HOT == pNMCD->uItemState || CDIS_HOT + CDIS_FOCUS == pNMCD->uItemState || CDIS_DISABLED == pNMCD->uItemState) {
				dc.SelectObject(&penFrDisabled);
			}

			dc.RoundRect(r.left, r.top, r.right, r.bottom, 6, 4);
			r.DeflateRect(2,2,2,2);
			if (pNMCD->dwItemSpec == IDC_COLORPRI) {
				dc.FillSolidRect(&r, m_stss->colors[0]);
			}
			if (pNMCD->dwItemSpec == IDC_COLORSEC) {
				dc.FillSolidRect(&r, m_stss->colors[1]);
			}
			if (pNMCD->dwItemSpec == IDC_COLOROUTL) {
				dc.FillSolidRect(&r, m_stss->colors[2]);
			}
			if (pNMCD->dwItemSpec == IDC_COLORSHAD) {
				dc.FillSolidRect(&r, m_stss->colors[3]);
			}

			dc.SelectObject(&penOld);
			dc.Detach();

			*pResult = CDRF_SKIPDEFAULT;
		}
	}
}

void CPPageSubStyle::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (m_linkalphasliders && pScrollBar) {
		int pos = ((CSliderCtrl*)pScrollBar)->GetPos();

		for (int i = 0; i < _countof(m_alphasliders); i++) {
			m_alphasliders[i].SetPos(pos);
		}
	}

	SetModified();

	__super::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CPPageSubStyle::OnCancel()
{
	*m_stss = m_stss_init;
}
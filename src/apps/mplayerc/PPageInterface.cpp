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
#include "PPageInterface.h"
#include "../../DSUtil/SysVersion.h"


// CPPageInterface dialog

IMPLEMENT_DYNAMIC(CPPageInterface, CPPageBase)
CPPageInterface::CPPageInterface()
	: CPPageBase(CPPageInterface::IDD, CPPageInterface::IDD)
	, m_bUseDarkTheme(FALSE)
	, m_nThemeBrightness(0)
	, m_nThemeRed(255)
	, m_nThemeGreen(255)
	, m_nThemeBlue(255)
	, m_fUseTimeTooltip(TRUE)
	, m_fSmartSeek(FALSE)
	, m_fChapterMarker(FALSE)
	, m_fFlybar(TRUE)
	, m_OSD_Size(0)
	, m_fFontShadow(FALSE)
	, m_fFontAA(TRUE)
	, m_nOSDTransparent(0)
	, m_OSDBorder(1)
	, m_fUseWin7TaskBar(TRUE)
	, m_clrFaceABGR(0x00ffffff)
	, m_clrOutlineABGR(0x00868686)
	, m_clrFontABGR(0x00E0E0E0)
	, m_clrGrad1ABGR(0x00302820)
	, m_clrGrad2ABGR(0x00302820)
{
}

CPPageInterface::~CPPageInterface()
{
}

void CPPageInterface::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Check(pDX, IDC_CHECK3, m_bUseDarkTheme);
	DDX_Control(pDX, IDC_CHECK3, m_UseDarkThemeCtrl);
	DDX_Slider(pDX, IDC_SLIDER1, m_nThemeBrightness);
	DDX_Slider(pDX, IDC_SLIDER2, m_nThemeRed);
	DDX_Slider(pDX, IDC_SLIDER3, m_nThemeGreen);
	DDX_Slider(pDX, IDC_SLIDER4, m_nThemeBlue);
	DDX_Slider(pDX, IDC_SLIDER_OSDTRANS, m_nOSDTransparent);
	DDX_Control(pDX, IDC_SLIDER1, m_ThemeBrightnessCtrl);
	DDX_Control(pDX, IDC_SLIDER2, m_ThemeRedCtrl);
	DDX_Control(pDX, IDC_SLIDER3, m_ThemeGreenCtrl);
	DDX_Control(pDX, IDC_SLIDER4, m_ThemeBlueCtrl);
	DDX_Control(pDX, IDC_SLIDER_OSDTRANS, m_OSDTransparentCtrl);
	DDX_Check(pDX, IDC_CHECK_WIN7, m_fUseWin7TaskBar);
	DDX_Check(pDX, IDC_CHECK8, m_fUseTimeTooltip);
	DDX_Control(pDX, IDC_COMBO3, m_TimeTooltipPosition);
	DDX_Control(pDX, IDC_COMBO1, m_FontType);
	DDX_Control(pDX, IDC_COMBO2, m_FontSize);
	DDX_Check(pDX, IDC_CHECK_PRV, m_fSmartSeek);
	DDX_Control(pDX, IDC_EDIT1, m_edSmartSeekSize);
	DDX_Check(pDX, IDC_CHECK_CHM, m_fChapterMarker);
	DDX_Check(pDX, IDC_CHECK_FLYBAR, m_fFlybar);
	DDX_Text(pDX, IDC_EDIT4, m_OSDBorder);
	DDX_Control(pDX, IDC_SPIN10, m_OSDBorderCtrl);
	DDX_Check(pDX, IDC_CHECK_SHADOW, m_fFontShadow);
	DDX_Check(pDX, IDC_CHECK_AA, m_fFontAA);
}

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

BOOL CPPageInterface::OnInitDialog()
{
	__super::OnInitDialog();

	SetCursor(m_hWnd, IDC_COMBO1, IDC_HAND);

	CAppSettings& s = AfxGetAppSettings();

	m_bUseDarkTheme	= s.bUseDarkTheme;
	m_nThemeBrightness		= m_nThemeBrightness_Old	= s.nThemeBrightness;
	m_nThemeRed				= m_nThemeRed_Old			= s.nThemeRed;
	m_nThemeGreen			= m_nThemeGreen_Old			= s.nThemeGreen;
	m_nThemeBlue			= m_nThemeBlue_Old			= s.nThemeBlue;
	m_nOSDTransparent		= m_nOSDTransparent_Old		= s.nOSDTransparent;
	m_OSDBorder				= m_OSDBorder_Old			= s.nOSDBorder;

	m_ThemeBrightnessCtrl.SetRange	(0, 100, TRUE);
	m_ThemeRedCtrl.SetRange			(0, 255, TRUE);
	m_ThemeGreenCtrl.SetRange		(0, 255, TRUE);
	m_ThemeBlueCtrl.SetRange		(0, 255, TRUE);
	m_OSDTransparentCtrl.SetRange	(0, 255, TRUE);
	m_OSDBorderCtrl.SetRange32(0, 5);

	m_clrFaceABGR			= m_clrFaceABGR_Old			= s.clrFaceABGR;
	m_clrOutlineABGR		= m_clrOutlineABGR_Old		= s.clrOutlineABGR;
	m_clrFontABGR			= m_clrFontABGR_Old			= s.clrFontABGR;
	m_clrGrad1ABGR			= m_clrGrad1ABGR_Old		= s.clrGrad1ABGR;
	m_clrGrad2ABGR			= m_clrGrad2ABGR_Old		= s.clrGrad2ABGR;
	m_fUseWin7TaskBar		= s.fUseWin7TaskBar;
	GetDlgItem(IDC_CHECK_WIN7)->EnableWindow(SysVersion::IsWin7orLater());

	m_fUseTimeTooltip = s.fUseTimeTooltip;
	m_TimeTooltipPosition.AddString(ResStr(IDS_TIME_TOOLTIP_ABOVE));
	m_TimeTooltipPosition.AddString(ResStr(IDS_TIME_TOOLTIP_BELOW));
	m_TimeTooltipPosition.SetCurSel(s.nTimeTooltipPosition);

	m_OSD_Size	= s.nOSDSize;
	m_OSD_Font	= s.strOSDFont;

	m_fSmartSeek		= s.fSmartSeek;
	m_edSmartSeekSize.SetRange(10, 30);
	m_edSmartSeekSize	= s.iSmartSeekSize;
	m_fChapterMarker	= s.fChapterMarker;
	m_fFlybar			= s.fFlybar;
	m_fFontShadow		= m_fFontShadow_Old	= s.fFontShadow;
	m_fFontAA			= m_fFontAA_Old		= s.fFontAA;
	m_FontType.Clear();
	m_FontSize.Clear();
	HDC dc = CreateDCW(L"DISPLAY", nullptr, nullptr, nullptr);
	std::vector<CString> fontnames;
	EnumFontFamiliesW(dc, nullptr, (FONTENUMPROCW)EnumFontProc, (LPARAM)&fontnames);
	DeleteDC(dc);

	for (const auto& fontname : fontnames) {
		m_FontType.AddString(fontname);
	}

	CorrectComboListWidth(m_FontType);
	int iSel = m_FontType.FindStringExact(0, m_OSD_Font);
	if (iSel == CB_ERR) {
		iSel = 0;
	}
	m_FontType.SetCurSel(iSel);

	CString str;

	for (int i = 10; i < 26; ++i) {
		str.Format(L"%d", i);
		m_FontSize.AddString(str);

		if (m_OSD_Size == i) {
			iSel = i;
		}
	}
	m_FontSize.SetCurSel(iSel - 10);

	EnableToolTips(TRUE);

	GetDlgItem(IDC_CHECK8)->EnableWindow(!m_fSmartSeek);
	GetDlgItem(IDC_COMBO3)->EnableWindow(!m_fSmartSeek && m_fUseTimeTooltip);

	GetDlgItem(IDC_STATIC1)->EnableWindow(m_fSmartSeek);
	GetDlgItem(IDC_EDIT1)->EnableWindow(m_fSmartSeek);
	GetDlgItem(IDC_STATIC2)->EnableWindow(m_fSmartSeek);

	UpdateData(FALSE);

	return TRUE;
}

BOOL CPPageInterface::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	s.clrFaceABGR			= m_clrFaceABGR;
	s.clrOutlineABGR		= m_clrOutlineABGR;
	s.clrFontABGR			= m_clrFontABGR;
	s.clrGrad1ABGR			= m_clrGrad1ABGR;
	s.clrGrad2ABGR			= m_clrGrad2ABGR;
	s.nThemeBrightness		= m_nThemeBrightness;
	s.nThemeRed				= m_nThemeRed;
	s.nThemeGreen			= m_nThemeGreen;
	s.nThemeBlue			= m_nThemeBlue;
	s.nOSDTransparent		= m_nOSDTransparent;
	s.nOSDBorder			= m_OSDBorder;

	ApplyOSDTransparent();

	auto pFrame			= AfxGetMainFrame();
	BOOL bUseDarkTheme	= s.bUseDarkTheme;
	s.bUseDarkTheme	= !!m_bUseDarkTheme;
	if (::IsWindow(pFrame->m_hWnd_toolbar) && (s.bUseDarkTheme != !!bUseDarkTheme)) {
		::PostMessageW(pFrame->m_hWnd_toolbar, WM_SIZE, s.nLastWindowType, MAKELPARAM(s.rcLastWindowPos.Width(), s.rcLastWindowPos.Height()));
		::PostMessageW(pFrame->m_hWnd,         WM_SIZE, s.nLastWindowType, MAKELPARAM(s.rcLastWindowPos.Width(), s.rcLastWindowPos.Height()));
	}

	s.fUseWin7TaskBar		= !!m_fUseWin7TaskBar;
	s.fUseTimeTooltip		= !!m_fUseTimeTooltip;
	s.nTimeTooltipPosition	= m_TimeTooltipPosition.GetCurSel();
	s.nOSDSize				= m_OSD_Size;

	m_FontType.GetLBText(m_FontType.GetCurSel(),s.strOSDFont);

	s.fSmartSeek			= !!m_fSmartSeek;
	s.iSmartSeekSize		= m_edSmartSeekSize;
	s.fChapterMarker		= !!m_fChapterMarker;
	s.fFlybar				= !!m_fFlybar;
	s.fFontShadow			= !!m_fFontShadow;
	s.fFontAA				= !!m_fFontAA;

	if (s.fFlybar && !pFrame->m_wndFlyBar) {
		pFrame->CreateFlyBar();

	} else if (!s.fFlybar && pFrame->m_wndFlyBar){
		pFrame->DestroyFlyBar();
	}

	pFrame->CreateThumbnailToolbar();
	pFrame->UpdateThumbarButton();
	pFrame->SetAudioPicture();

	pFrame->m_wndPreView.SetRelativeSize(s.iSmartSeekSize);

	pFrame->m_wndPlaylistBar.m_bUseDarkTheme = s.bUseDarkTheme;
	if (pFrame->m_wndPlaylistBar.IsWindowVisible()) {
		pFrame->m_wndPlaylistBar.SendMessageW(WM_NCPAINT, 1, NULL);
	}
	pFrame->Invalidate();

	m_nThemeBrightness_Old	= s.nThemeBrightness;
	m_nThemeRed_Old			= s.nThemeRed;
	m_nThemeGreen_Old		= s.nThemeGreen;
	m_nThemeBlue_Old		= s.nThemeBlue;
	m_OSDBorder_Old			= s.nOSDBorder;
	m_fFontShadow_Old		= s.fFontShadow;
	m_fFontAA_Old			= s.fFontAA;
	m_nOSDTransparent_Old	= s.nOSDTransparent;

	m_clrFaceABGR_Old		= s.clrFaceABGR;
	m_clrOutlineABGR_Old	= s.clrOutlineABGR;
	m_clrFontABGR_Old		= s.clrFontABGR;
	m_clrGrad1ABGR_Old		= s.clrGrad1ABGR;
	m_clrGrad2ABGR_Old		= s.clrGrad2ABGR;

	return __super::OnApply();
}

void CPPageInterface::OnCancel()
{
	CAppSettings& s = AfxGetAppSettings();

	s.nThemeBrightness	= m_nThemeBrightness_Old;
	s.nThemeRed			= m_nThemeRed_Old;
	s.nThemeGreen		= m_nThemeGreen_Old;
	s.nThemeBlue		= m_nThemeBlue_Old;
	s.nOSDBorder		= m_OSDBorder_Old;
	s.fFontShadow		= !!m_fFontShadow_Old;
	s.fFontAA			= !!m_fFontAA_Old;
	s.nOSDTransparent	= m_nOSDTransparent_Old;

	s.clrFaceABGR		= m_clrFaceABGR_Old;
	s.clrOutlineABGR	= m_clrOutlineABGR_Old;
	s.clrFontABGR		= m_clrFontABGR_Old;
	s.clrGrad1ABGR		= m_clrGrad1ABGR_Old;
	s.clrGrad2ABGR		= m_clrGrad2ABGR_Old;

	OnThemeChange();
	ApplyOSDTransparent();
}

void CPPageInterface::ApplyOSDTransparent()
{
	auto pFrame = AfxGetMainFrame();
	if (pFrame->m_OSD) {
		CAppSettings& s = AfxGetAppSettings();

		pFrame->m_OSD.SetLayeredWindowAttributes(RGB(255, 0, 255), 255 - s.nOSDTransparent, LWA_ALPHA | LWA_COLORKEY);
		pFrame->m_OSD.ClearMessage();
	}
}

void CPPageInterface::OnThemeChange()
{
	const auto pFrame = AfxGetMainFrame();

	if (::IsWindow(pFrame->m_hWnd_toolbar)) {
		::PostMessageW(pFrame->m_hWnd_toolbar, WM_SIZE, SIZE_RESTORED, MAKELPARAM(320, 240));
	}

	if (pFrame->m_wndPlaylistBar.IsWindowVisible()) {
		pFrame->m_wndPlaylistBar.SendMessageW(WM_NCPAINT, 1, NULL);
	}

	pFrame->Invalidate();
	pFrame->m_wndPlaylistBar.Invalidate();
}

BEGIN_MESSAGE_MAP(CPPageInterface, CPPageBase)
	ON_UPDATE_COMMAND_UI(IDC_CHECK3, OnUpdateCheck3)
	ON_COMMAND(IDC_CHECK_SHADOW, OnCheckShadow)
	ON_COMMAND(IDC_CHECK_AA, OnCheckAA)
	ON_UPDATE_COMMAND_UI(IDC_EDIT4, OnUpdateOSDBorder)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_BUTTON_CLRFACE, OnCustomDrawBtns)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_BUTTON_CLROUTLINE, OnCustomDrawBtns)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_BUTTON_CLRFONT, OnCustomDrawBtns)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_BUTTON_CLRGRAD1, OnCustomDrawBtns)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_BUTTON_CLRGRAD2, OnCustomDrawBtns)
	ON_BN_CLICKED(IDC_BUTTON_CLRFACE, OnClickClrFace)
	ON_BN_CLICKED(IDC_BUTTON_CLRFONT, OnClickClrFont)
	ON_BN_CLICKED(IDC_BUTTON_CLRGRAD1, OnClickClrGrad1)
	ON_BN_CLICKED(IDC_BUTTON_CLRGRAD2, OnClickClrGrad2)
	ON_BN_CLICKED(IDC_BUTTON_CLROUTLINE, OnClickClrOutline)
	ON_BN_CLICKED(IDC_BUTTON_CLRDEFAULT, OnClickClrDefault)
	ON_BN_CLICKED(IDC_CHECK8, OnUseTimeTooltipClicked)
	ON_BN_CLICKED(IDC_CHECK_PRV, OnUsePreview)
	ON_BN_CLICKED(IDC_CHECK_WIN7, OnUseWin7TaskBar)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnChngOSDCombo)
	ON_CBN_SELCHANGE(IDC_COMBO2, OnChngOSDCombo)
	ON_UPDATE_COMMAND_UI(IDC_SLIDER1, OnUpdateThemeBrightness)
	ON_UPDATE_COMMAND_UI(IDC_SLIDER2, OnUpdateThemeRed)
	ON_UPDATE_COMMAND_UI(IDC_SLIDER3, OnUpdateThemeGreen)
	ON_UPDATE_COMMAND_UI(IDC_SLIDER4, OnUpdateThemeBlue)
	ON_UPDATE_COMMAND_UI(IDC_SLIDER_OSDTRANS, OnUpdateOSDTransparent)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

// CPPageInterface message handlers

void CPPageInterface::OnUpdateCheck3(CCmdUI* pCmdUI)
{
	GetDlgItem(IDC_BUTTON_CLRFACE)->EnableWindow(m_bUseDarkTheme);
	GetDlgItem(IDC_BUTTON_CLROUTLINE)->EnableWindow(m_bUseDarkTheme);
	GetDlgItem(IDC_BUTTON_CLRDEFAULT)->EnableWindow(m_bUseDarkTheme);
	GetDlgItem(IDC_STATIC_CLRFACE)->EnableWindow(m_bUseDarkTheme);
	GetDlgItem(IDC_STATIC_CLROUTLINE)->EnableWindow(m_bUseDarkTheme);
}

void CPPageInterface::OnCheckShadow()
{
	CAppSettings& s = AfxGetAppSettings();

	UpdateData();
	BOOL fFontShadow = s.fFontShadow;
	s.fFontShadow = !!m_fFontShadow;
	OnChngOSDCombo();

	s.fFontShadow = !!fFontShadow;
}

void CPPageInterface::OnCheckAA()
{
	CAppSettings& s = AfxGetAppSettings();

	UpdateData();
	BOOL fFontAA = s.fFontAA;
	s.fFontAA = !!m_fFontAA;
	OnChngOSDCombo();

	s.fFontAA = !!fFontAA;
}

void CPPageInterface::OnUpdateOSDBorder(CCmdUI* pCmdUI)
{
	CAppSettings& s = AfxGetAppSettings();

	if (s.nOSDBorder != m_OSDBorder) {
		UpdateData();
		int nOSDBorder = s.nOSDBorder;
		s.nOSDBorder = m_OSDBorder;
		OnChngOSDCombo();

		s.nOSDBorder = nOSDBorder;
	}
}

void CPPageInterface::OnClickClrDefault()
{
	CAppSettings& s = AfxGetAppSettings();

	m_clrFaceABGR = 0x00ffffff;
	m_clrOutlineABGR = 0x00868686;
	m_clrFontABGR = 0x00E0E0E0;
	m_clrGrad1ABGR = 0x00302820;
	m_clrGrad2ABGR = 0x00302820;
	GetDlgItem(IDC_BUTTON_CLRFACE)->Invalidate();
	GetDlgItem(IDC_BUTTON_CLROUTLINE)->Invalidate();
	GetDlgItem(IDC_BUTTON_CLRFONT)->Invalidate();
	GetDlgItem(IDC_BUTTON_CLRGRAD1)->Invalidate();
	GetDlgItem(IDC_BUTTON_CLRGRAD2)->Invalidate();
	PostMessageW(WM_COMMAND, IDC_CHECK3);

	s.nThemeBrightness		= m_nThemeBrightness	= 15;
	s.nThemeRed				= m_nThemeRed			= 255;
	s.nThemeGreen			= m_nThemeGreen			= 255;
	s.nThemeBlue			= m_nThemeBlue			= 255;
	s.nOSDTransparent		= m_nOSDTransparent		= 100;

	m_fFontShadow = FALSE;
	m_fFontAA = TRUE;
	m_OSDBorder = 1;
	OnThemeChange();

	m_nThemeBrightness_Old	= s.nThemeBrightness;
	m_nThemeRed_Old			= s.nThemeRed;
	m_nThemeGreen_Old		= s.nThemeGreen;
	m_nThemeBlue_Old		= s.nThemeBlue;
	m_OSDBorder_Old			= s.nOSDBorder;
	m_fFontShadow_Old		= s.fFontShadow;
	m_fFontAA_Old			= s.fFontAA;
	m_nOSDTransparent_Old	= s.nOSDTransparent;

	m_clrFaceABGR_Old		= s.clrFaceABGR;
	m_clrOutlineABGR_Old	= s.clrOutlineABGR;
	m_clrFontABGR_Old		= s.clrFontABGR;
	m_clrGrad1ABGR_Old		= s.clrGrad1ABGR;
	m_clrGrad2ABGR_Old		= s.clrGrad2ABGR;

	UpdateData(FALSE);
}

void CPPageInterface::OnClickClrFace()
{
	CColorDialog clrpicker;
	clrpicker.m_cc.Flags |= CC_FULLOPEN | CC_RGBINIT;
	clrpicker.m_cc.rgbResult = m_clrFaceABGR;

	if (clrpicker.DoModal() == IDOK) {
		if (m_clrFaceABGR != clrpicker.GetColor()) {
			UpdateData(FALSE);
			PostMessageW(WM_COMMAND, IDC_CHECK3);
		}
		m_clrFaceABGR = clrpicker.GetColor();
	}
}

void CPPageInterface::OnClickClrOutline()
{
	CColorDialog clrpicker;
	clrpicker.m_cc.Flags |= CC_FULLOPEN | CC_RGBINIT;
	clrpicker.m_cc.rgbResult = m_clrOutlineABGR;

	if (clrpicker.DoModal() == IDOK) {
		if (m_clrOutlineABGR != clrpicker.GetColor()) {
			UpdateData(FALSE);
			PostMessageW(WM_COMMAND, IDC_CHECK3);
		}
		m_clrOutlineABGR = clrpicker.GetColor();
	}
}

void CPPageInterface::OnClickClrFont()
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
			OnChngOSDCombo();
			UpdateData(FALSE);
		}
	}
}

void CPPageInterface::OnClickClrGrad1()
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
			OnChngOSDCombo();
			UpdateData(FALSE);
		}
	}
}

void CPPageInterface::OnClickClrGrad2()
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
			OnChngOSDCombo();
			UpdateData(FALSE);
		}
	}
}

void CPPageInterface::OnCustomDrawBtns(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	*pResult = CDRF_DODEFAULT;

	if (pNMCD->dwItemSpec == IDC_BUTTON_CLRFACE
		|| pNMCD->dwItemSpec == IDC_BUTTON_CLROUTLINE
		|| pNMCD->dwItemSpec == IDC_BUTTON_CLRFONT
		|| pNMCD->dwItemSpec == IDC_BUTTON_CLRGRAD1
		|| pNMCD->dwItemSpec == IDC_BUTTON_CLRGRAD2) {
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
			if (pNMCD->dwItemSpec == IDC_BUTTON_CLRFACE) {
				dc.FillSolidRect(&r, m_clrFaceABGR);
			}
			if (pNMCD->dwItemSpec == IDC_BUTTON_CLROUTLINE) {
				dc.FillSolidRect(&r, m_clrOutlineABGR);
			}
			if (pNMCD->dwItemSpec == IDC_BUTTON_CLRFONT) {
				dc.FillSolidRect(&r, m_clrFontABGR);
			}
			if (pNMCD->dwItemSpec == IDC_BUTTON_CLRGRAD1) {
				dc.FillSolidRect(&r, m_clrGrad1ABGR);
			}
			if (pNMCD->dwItemSpec == IDC_BUTTON_CLRGRAD2) {
				dc.FillSolidRect(&r, m_clrGrad2ABGR);
			}

			dc.SelectObject(&penOld);
			dc.Detach();

			*pResult = CDRF_SKIPDEFAULT;
		}
	}
}

void CPPageInterface::OnChngOSDCombo()
{
	auto pFrame = AfxGetMainFrame();
	if (pFrame->m_OSD) {
		CString str;
		m_OSD_Size = m_FontSize.GetCurSel() + 10;
		m_FontType.GetLBText(m_FontType.GetCurSel(), str);

		pFrame->m_OSD.DisplayMessage(OSD_TOPLEFT, ResStr(IDS_OSD_TEST), 2000, m_OSD_Size, str);
		pFrame->m_OSD.SetLayeredWindowAttributes(RGB(255, 0, 255), 255 - AfxGetAppSettings().nOSDTransparent, LWA_ALPHA | LWA_COLORKEY);
	}

	SetModified();
}

void CPPageInterface::OnUseWin7TaskBar()
{
	SetModified();
}

void CPPageInterface::OnUseTimeTooltipClicked()
{
	m_TimeTooltipPosition.EnableWindow(IsDlgButtonChecked(IDC_CHECK8));

	SetModified();
}

void CPPageInterface::OnUsePreview()
{
	UpdateData();

	GetDlgItem(IDC_CHECK8)->EnableWindow(!m_fSmartSeek);
	GetDlgItem(IDC_COMBO3)->EnableWindow(!m_fSmartSeek && m_fUseTimeTooltip);

	GetDlgItem(IDC_STATIC1)->EnableWindow(m_fSmartSeek);
	GetDlgItem(IDC_EDIT1)->EnableWindow(m_fSmartSeek);
	GetDlgItem(IDC_STATIC2)->EnableWindow(m_fSmartSeek);

	SetModified();
}

void CPPageInterface::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CAppSettings& s = AfxGetAppSettings();

	if (*pScrollBar == m_ThemeBrightnessCtrl) {
		UpdateData();
		s.nThemeBrightness	= m_nThemeBrightness;
		OnThemeChange();
	}

	if (*pScrollBar == m_ThemeRedCtrl) {
		UpdateData();
		s.nThemeRed			= m_nThemeRed;
		OnThemeChange();
	}

	if (*pScrollBar == m_ThemeGreenCtrl) {
		UpdateData();
		s.nThemeGreen		= m_nThemeGreen;
		OnThemeChange();
	}

	if (*pScrollBar == m_ThemeBlueCtrl) {
		UpdateData();
		s.nThemeBlue		= m_nThemeBlue;
		OnThemeChange();
	}

	if (*pScrollBar == m_OSDTransparentCtrl) {
		UpdateData();
		int nOSDTransparent	= s.nOSDTransparent;
		s.nOSDTransparent	= m_nOSDTransparent;
		OnChngOSDCombo();

		s.nOSDTransparent	= nOSDTransparent;
	}

	SetModified();

	__super::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CPPageInterface::OnUpdateThemeBrightness(CCmdUI* pCmdUI)
{
	UpdateData();

	pCmdUI->Enable(IsDlgButtonChecked(IDC_CHECK3));
}

void CPPageInterface::OnUpdateThemeRed(CCmdUI* pCmdUI)
{
	UpdateData();

	pCmdUI->Enable(IsDlgButtonChecked(IDC_CHECK3));
}

void CPPageInterface::OnUpdateThemeGreen(CCmdUI* pCmdUI)
{
	UpdateData();

	pCmdUI->Enable(IsDlgButtonChecked(IDC_CHECK3));
}

void CPPageInterface::OnUpdateThemeBlue(CCmdUI* pCmdUI)
{
	UpdateData();

	pCmdUI->Enable(IsDlgButtonChecked(IDC_CHECK3));
}

void CPPageInterface::OnUpdateOSDTransparent(CCmdUI* pCmdUI)
{
	UpdateData();
}

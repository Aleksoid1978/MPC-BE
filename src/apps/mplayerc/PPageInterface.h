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

#pragma once

#include "PPageBase.h"
#include "controls/StaticLink.h"


// CPPageInterface dialog

class CPPageInterface : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageInterface)

public:
	CPPageInterface();
	virtual ~CPPageInterface();

	enum { IDD = IDD_PPAGEINTERFACE };
	BOOL m_bUseDarkTheme    = FALSE;
	int  m_nThemeBrightness = 0;
	int  m_nThemeRed        = 255;
	int  m_nThemeGreen      = 255;
	int  m_nThemeBlue       = 255;
	BOOL m_fUseTimeTooltip  = TRUE;
	BOOL m_fSmartSeek       = FALSE;
	BOOL m_bSmartSeekOnline = FALSE;
	BOOL m_fChapterMarker   = FALSE;
	BOOL m_fFlybar          = TRUE;
	BOOL m_fFontShadow      = FALSE;
	BOOL m_fFontAA          = TRUE;
	int  m_nOSDTransparent  = 0;
	int  m_OSDBorder        = 1;
	BOOL m_fUseWin7TaskBar  = TRUE;
	CString m_OSD_Font;

	CButton m_UseDarkThemeCtrl;
	CSliderCtrl m_ThemeBrightnessCtrl;
	CSliderCtrl m_ThemeRedCtrl;
	CSliderCtrl m_ThemeGreenCtrl;
	CSliderCtrl m_ThemeBlueCtrl;
	CButton     m_chkDarkMenu;
	CButton     m_chkDarkTitle;
	CIntEdit    m_edSmartSeekSize;
	CIntEdit    m_edPlsFontPercent;
	CIntEdit    m_edOSDFontSize;
	CSliderCtrl m_OSDTransparentCtrl;
	CComboBox m_TimeTooltipPosition;
	CComboBox m_FontType;
	CComboBox m_SmartSeekVR;
	CSpinButtonCtrl m_spOSDFontSize;
	CSpinButtonCtrl m_OSDBorderCtrl;

	COLORREF m_clrFaceABGR    = 0x00ffffff;
	COLORREF m_clrOutlineABGR = 0x00868686;
	COLORREF m_clrFontABGR    = 0x00E0E0E0;
	COLORREF m_clrGrad1ABGR   = 0x00302820;
	COLORREF m_clrGrad2ABGR   = 0x00302820;

	int m_nThemeBrightness_Old;
	int m_nThemeRed_Old;
	int m_nThemeGreen_Old;
	int m_nThemeBlue_Old;
	int m_nOSDTransparent_Old;
	int m_OSDBorder_Old;
	BOOL m_fFontShadow_Old;
	BOOL m_fFontAA_Old;

	COLORREF m_clrFaceABGR_Old;
	COLORREF m_clrOutlineABGR_Old;
	COLORREF m_clrFontABGR_Old;
	COLORREF m_clrGrad1ABGR_Old;
	COLORREF m_clrGrad2ABGR_Old;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	void OnCancel();

	DECLARE_MESSAGE_MAP()

	void ApplyOSDTransparent();

public:
	afx_msg void OnUpdateCheck3(CCmdUI* pCmdUI);
	afx_msg void OnCheckShadow();
	afx_msg void OnCheckAA();
	afx_msg void OnUpdateOSDBorder(CCmdUI* pCmdUI);
	afx_msg void OnClickClrDefault();
	afx_msg void OnClickClrFace();
	afx_msg void OnClickClrOutline();
	afx_msg void OnClickClrFont();
	afx_msg void OnClickClrGrad1();
	afx_msg void OnClickClrGrad2();
	afx_msg void OnCustomDrawBtns(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposSpin3(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnUseTimeTooltipClicked();
	afx_msg void OnUsePreview();
	afx_msg void OnChangeOSD();
	afx_msg void OnUseWin7TaskBar();
	afx_msg void OnUpdateThemeBrightness(CCmdUI* pCmdUI);
	afx_msg void OnUpdateThemeRed(CCmdUI* pCmdUI);
	afx_msg void OnUpdateThemeGreen(CCmdUI* pCmdUI);
	afx_msg void OnUpdateThemeBlue(CCmdUI* pCmdUI);
	afx_msg void OnUpdateOSDTransparent(CCmdUI* pCmdUI);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnThemeChange();
};

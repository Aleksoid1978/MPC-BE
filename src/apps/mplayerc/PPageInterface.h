/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2025 see Authors.txt
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
	int  m_nThemeBrightness = 0;
	int  m_nThemeRed        = 255;
	int  m_nThemeGreen      = 255;
	int  m_nThemeBlue       = 255;
	BOOL m_fUseWin7TaskBar  = TRUE;

	CButton m_UseDarkThemeCtrl;
	CSliderCtrl m_ThemeBrightnessCtrl;
	CSliderCtrl m_ThemeRedCtrl;
	CSliderCtrl m_ThemeGreenCtrl;
	CSliderCtrl m_ThemeBlueCtrl;
	CButton     m_chkDarkMenu;
	CButton     m_chkDarkMenuBlurBehind;
	CButton     m_chkDarkTitle;

	BOOL      m_fUseTimeTooltip  = TRUE;
	CComboBox m_TimeTooltipPosition;
	BOOL      m_fSmartSeek       = FALSE;
	BOOL      m_bSmartSeekOnline = FALSE;
	CIntEdit  m_edSmartSeekSize;
	CComboBox m_SmartSeekVR;
	BOOL      m_fChapterMarker   = FALSE;
	BOOL      m_fFlybar          = TRUE;
	CIntEdit  m_edPlsFontPercent;
	CComboBox m_cbToolbarSize;

	int m_nThemeBrightness_Old;
	int m_nThemeRed_Old;
	int m_nThemeGreen_Old;
	int m_nThemeBlue_Old;

	COLORREF m_clrFaceABGR    = 0x00ffffff;
	COLORREF m_clrOutlineABGR = 0x00868686;

	COLORREF m_clrFaceABGR_Old;
	COLORREF m_clrOutlineABGR_Old;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	void OnCancel();

	DECLARE_MESSAGE_MAP()

	void ApplyOSDTransparent();

public:
	afx_msg void OnUpdateCheck3(CCmdUI* pCmdUI);
	afx_msg void OnClickClrDefault();
	afx_msg void OnClickClrFace();
	afx_msg void OnClickClrOutline();
	afx_msg void OnCustomDrawBtns(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnUseTimeTooltipClicked();
	afx_msg void OnUsePreview();
	afx_msg void OnUseWin7TaskBar();
	afx_msg void OnUpdateThemeBrightness(CCmdUI* pCmdUI);
	afx_msg void OnUpdateThemeRed(CCmdUI* pCmdUI);
	afx_msg void OnUpdateThemeGreen(CCmdUI* pCmdUI);
	afx_msg void OnUpdateThemeBlue(CCmdUI* pCmdUI);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnThemeChange();
};

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

#pragma once

#include "PPageBase.h"
#include "controls/FloatEdit.h"

// CPPageOSD dialog

class CPPageOSD : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageOSD)

public:
	CPPageOSD();
	virtual ~CPPageOSD() = default;
	
	enum { IDD = IDD_PPAGEOSD };	
	
	BOOL m_bShowOSD     = FALSE;
	BOOL m_bOSDFileName = FALSE;
	BOOL m_bOSDSeekTime = FALSE;
	
	CString         m_OSD_Font;
	CComboBox       m_OSDFontType;
	CIntEdit        m_edOSDFontSize;
	CSpinButtonCtrl m_spOSDFontSize;
	BOOL m_bOSDFontShadow  = FALSE;
	BOOL m_bOSDFontAA      = TRUE;
	CSliderCtrl     m_OSDTransparentCtrl;
	int  m_nOSDTransparent = 0;
	int  m_OSDBorder       = 1;
	CSpinButtonCtrl m_OSDBorderCtrl;

	COLORREF m_clrFontABGR = 0x00E0E0E0;
	COLORREF m_clrGrad1ABGR = 0x00302820;
	COLORREF m_clrGrad2ABGR = 0x00302820;

	int m_nOSDTransparent_Old;
	int m_OSDBorder_Old;
	BOOL m_bOSDFontShadow_Old;
	BOOL m_bOSDFontAA_Old;

	COLORREF m_clrFontABGR_Old;
	COLORREF m_clrGrad1ABGR_Old;
	COLORREF m_clrGrad2ABGR_Old;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	void OnCancel();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnUpdateOSD(CCmdUI* pCmdUI);
	
	afx_msg void OnChangeOSD();
	afx_msg void OnUpdateOSDTransparent(CCmdUI* pCmdUI);
	afx_msg void OnCheckShadow();
	afx_msg void OnCheckAA();
	afx_msg void OnUpdateOSDBorder(CCmdUI* pCmdUI);
	afx_msg void OnClickClrFont();
	afx_msg void OnClickClrGrad1();
	afx_msg void OnClickClrGrad2();
	afx_msg void OnDeltaposSpin3(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCustomDrawBtns(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnClickClrDefault();
};

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

	BOOL m_bShowOSD     = TRUE;
	BOOL m_bOSDFileName = FALSE;
	BOOL m_bOSDSeekTime = FALSE;

	CString         m_FontName;
	CComboBox       m_cbFontName;
	CIntEdit        m_edFontSize;
	CSpinButtonCtrl m_spFontSize;
	BOOL m_bFontShadow  = FALSE;
	BOOL m_bFontAA      = TRUE;
	int  m_nTransparent = 0;
	CSliderCtrl m_TransparentCtrl;
	int  m_nBorder      = 1;
	CSpinButtonCtrl m_BorderCtrl;

	COLORREF m_colorFont  = RGB(224, 224, 224);
	COLORREF m_colorGrad1 = RGB(32, 40, 48);
	COLORREF m_colorGrad2 = RGB(32, 40, 48);

	int m_nTransparent_Old;
	int m_nBorder_Old;
	BOOL m_bFontShadow_Old;
	BOOL m_bFontAA_Old;

	COLORREF m_colorFont_Old;
	COLORREF m_colorGrad1_Old;
	COLORREF m_colorGrad2_Old;

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
	afx_msg void OnBnClickedDefault();
};

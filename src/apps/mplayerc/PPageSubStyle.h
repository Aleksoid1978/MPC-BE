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
#include "Subtitles/STS.h"

// CPPageSubStyle dialog

class CPPageSubStyle : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageSubStyle)

private:
	CString m_title;
	STSStyle* m_stss;
	STSStyle m_stss_init;
	BOOL m_bUseDefaultStyle = TRUE;

	void AskColor(int i);

public:
	CPPageSubStyle();
	virtual ~CPPageSubStyle();

	void InitSubStyle(CString title, STSStyle* stss);
	void GetSubStyle(STSStyle* stss) {
		stss = m_stss;
	}

	enum { IDD = IDD_PPAGESUBSTYLE };
	CButton m_font;
	CFloatEdit m_spacing;
	CIntEdit m_angle;
	CSpinButtonCtrl m_anglespin;
	CIntEdit m_scalex;
	CSpinButtonCtrl m_scalexspin;
	CIntEdit m_scaley;
	CSpinButtonCtrl m_scaleyspin;
	int m_borderstyle = 0;
	CFloatEdit m_borderwidth;
	CFloatEdit m_shadowdepth;
	int m_screenalignment = 0;
	CIntEdit m_marginleft;
	CIntEdit m_marginright;
	CIntEdit m_margintop;
	CIntEdit m_marginbottom;
	CSpinButtonCtrl m_marginleftspin;
	CSpinButtonCtrl m_marginrightspin;
	CSpinButtonCtrl m_margintopspin;
	CSpinButtonCtrl m_marginbottomspin;
	int m_alpha[4];
	CSliderCtrl m_alphasliders[4];
	BOOL m_linkalphasliders = FALSE;
	BOOL m_relativeTo = FALSE;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnStnClickedColorpri();
	afx_msg void OnStnClickedColorsec();
	afx_msg void OnStnClickedColoroutl();
	afx_msg void OnStnClickedColorshad();
	afx_msg void OnBnClickedCheck1();
	afx_msg void OnCustomDrawBtns(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

	void Init();

public:
	virtual void OnCancel();
};

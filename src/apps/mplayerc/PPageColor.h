/*
 * (C) 2006-2014 see Authors.txt
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

// CPPageColor dialog

class CPPageColor : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageColor)

private:
	// Color control
	int m_iBrightness;
	int m_iContrast;
	int m_iHue;
	int m_iSaturation;
	CString m_sBrightness;
	CString m_sContrast;
	CString m_sHue;
	CString m_sSaturation;

	// Color managment
	CButton   m_chkColorManagment;
	CComboBox m_cbCMInputType;
	CComboBox m_cbCMAmbientLight;
	CComboBox m_cbCMRenderingIntent;

public:
	CPPageColor();
	virtual ~CPPageColor();

	enum { IDD = IDD_PPAGECOLOR };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnSetActive();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	CSliderCtrl m_SliContrast;
	CSliderCtrl m_SliBrightness;
	CSliderCtrl m_SliHue;
	CSliderCtrl m_SliSaturation;

	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnColorManagmentCheck();
	afx_msg void OnBnClickedReset();
	virtual void OnCancel();
};

/*
 * (C) 2019 see Authors.txt
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


// CPPageWindowSize dialog

class CPPageWindowSize : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageWindowSize)

	int m_nRadioStartup = 0;
	int m_iWindowWidth = 1280;
	CSpinButtonCtrl m_spnWindowWidth;
	int m_iWindowHeigth = 720;
	CSpinButtonCtrl m_spnWindowHeigth;

	CButton m_chkRememberWindowPos;

	int m_nRadioPlayback = 0;
	CComboBox m_cmbScaleLevel;
	int m_nFitFactor = 50;
	CSpinButtonCtrl m_spnFitFactor;
	CButton m_chkFitLargerOnly;

	CButton m_chkLimitWindowProportions;
	CButton m_chkSnapToDesktopEdges;

public:
	CPPageWindowSize();
	virtual ~CPPageWindowSize();

	enum { IDD = IDD_PPAGEWINDOWSIZE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnRadioStartupClicked(UINT nID);
	afx_msg void OnRadioPlaybackClicked(UINT nID);
};

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

// CPPageMisc dialog

class CPPageMisc : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageMisc)

private:
	int m_nJumpDistS;
	int m_nJumpDistM;
	int m_nJumpDistL;
	BOOL m_fFastSeek;
	BOOL m_fDontUseSearchInFolder;
	BOOL m_fPreventMinimize;
	BOOL m_bPauseMinimizedVideo;
	BOOL m_fLCDSupport;
	BOOL m_fMiniDump;

	CButton m_updaterAutoCheckCtrl;
	CEdit m_updaterDelayCtrl;
	CSpinButtonCtrl m_updaterDelaySpin;
	int m_nUpdaterDelay;

public:
	CPPageMisc();
	virtual ~CPPageMisc();

	enum { IDD = IDD_PPAGEMISC };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnUpdateDelayEditBox(CCmdUI* pCmdUI);
	afx_msg void OnResetSettings();
	afx_msg void OnExportSettings();
};

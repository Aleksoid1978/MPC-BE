/*
 * (C) 2003-2006 Gabest
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

#include "../../Subtitles/STS.h"


// CPPageSubRend dialog

class CPPageSubRend : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageSubRend)

public:
	CPPageSubRend();
	virtual ~CPPageSubRend();

	BOOL m_fOverridePlacement;
	int m_nHorPos;
	CEdit m_nHorPosEdit;
	CSpinButtonCtrl m_nHorPosCtrl;
	int m_nVerPos;
	CEdit m_nVerPosEdit;
	CSpinButtonCtrl m_nVerPosCtrl;
	int m_nSPCSize;
	CSpinButtonCtrl m_nSPCSizeCtrl;
	CComboBox m_spmaxres;
	BOOL m_bSPCAllowAnimationWhenBuffering;
	BOOL m_bbSPAllowDropSubPic;
	int m_nSubDelayInterval;

	enum { IDD = IDD_PPAGESUBRENDERING };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnUpdatePosOverride(CCmdUI* pCmdUI);
	afx_msg void OnUpdateAllowDropSubPic(CCmdUI* pCmdUI);
	afx_msg void OnSubDelayInterval();
};

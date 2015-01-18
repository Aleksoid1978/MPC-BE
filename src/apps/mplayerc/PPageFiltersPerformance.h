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

#include "PPageBase.h"
#include "PlayerListCtrl.h"

// CPPageFiltersPerformance dialog

class CPPageFiltersPerformance : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageFiltersPerformance)

public:
	CPPageFiltersPerformance();
	virtual ~CPPageFiltersPerformance();

	enum { IDD = IDD_PPAGEFILTERSPERFORMANCE };

protected:
	DWORDLONG m_halfMemMB;

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()
public:
	CSpinButtonCtrl m_nMinQueueSizeCtrl;
	CSpinButtonCtrl m_nMaxQueueSizeCtrl;
	CSpinButtonCtrl m_nCachSizeCtrl;
	DWORD m_nMinQueueSize;
	DWORD m_nMaxQueueSize;
	DWORD m_nCachSize;
	DWORD m_nMinQueuePackets;
	DWORD m_nMaxQueuePackets;
	CButton m_DefaultCtrl;

	afx_msg void OnBnClickedCheck1();
};

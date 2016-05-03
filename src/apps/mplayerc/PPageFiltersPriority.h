/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2016 see Authors.txt
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

// CPPageFiltersPriority dialog

class CPPageFiltersPriority : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageFiltersPriority)

private:
	CAutoPtrList<FilterOverride> m_pFilters;

public:
	CPPageFiltersPriority();
	virtual ~CPPageFiltersPriority();

	enum { IDD = IDD_PPAGEFILTERSPRIORITY };

	CComboBox m_HTTP, m_AVI, m_MKV, m_MPEGTS, m_MPEG, m_MP4, m_FLV, m_WMV;

	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

	void Init();
};

/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2023 see Authors.txt
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


// CPPageDVD dialog

class CPPageDVD : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageDVD)

private:
	void UpdateLCIDList();

public:
	CPPageDVD();
	virtual ~CPPageDVD();

	CListBox m_lcids;
	CString m_dvdpath;
	CEdit m_dvdpathctrl;
	CButton m_dvdpathselctrl;
	int m_iDVDLocation = 0;
	int m_iDVDLangType = 0;

	LCID m_idMenuLang      = 0;
	LCID m_idAudioLang     = 0;
	LCID m_idSubtitlesLang = 0;

	BOOL m_bClosedCaptions = FALSE;
	BOOL m_bStartMainTitle = FALSE;

	enum { IDD = IDD_PPAGEDVD};

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedLangradio123(UINT nID);
	afx_msg void OnLbnSelchangeList1();
	afx_msg void OnUpdateDVDPath(CCmdUI* pCmdUI);
};

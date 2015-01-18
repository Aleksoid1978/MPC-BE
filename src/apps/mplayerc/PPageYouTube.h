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
#include "PlayerYouTube.h"

// CPPageYoutube dialog

class CPPageYoutube : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageYoutube)

private:
	CComboBox m_iYoutubeFormatCtrl;
	CButton m_chkYoutubeLoadPlaylist;

public:
	CPPageYoutube();
	virtual ~CPPageYoutube();

	enum { IDD = IDD_PPAGEYOUTUBE };
	int m_iYoutubeSourceType;
	int m_iYoutubeMemoryType;
	CSpinButtonCtrl m_nPercentMemoryCtrl;
	CSpinButtonCtrl m_nMbMemoryCtrl;
	DWORD m_iYoutubePercentMemory;
	DWORD m_iYoutubeMbMemory;

	afx_msg void OnBnClickedRadio12(UINT nID);
	afx_msg void OnBnClickedRadio34(UINT nID);

	void UpdateMemoryCtrl();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()
};

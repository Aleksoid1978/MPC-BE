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

// CPPageFileMediaInfo dialog

class CPPageFileMediaInfo : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPageFileMediaInfo)

	CString m_fn;
	CEdit m_mediainfo;
	CFont* m_pCFont;
	CString MI_Text;

public:
	CPPageFileMediaInfo(CString fn);
	virtual ~CPPageFileMediaInfo();

	enum { IDD = IDD_FILEMEDIAINFO };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()

	CRect m_rCrt;

public:
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
};

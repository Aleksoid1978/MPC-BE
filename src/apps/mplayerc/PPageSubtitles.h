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


// CPPageSubtitles dialog

class CPPageSubtitles : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageSubtitles)

public:
	CPPageSubtitles();
	virtual ~CPPageSubtitles();

	enum { IDD = IDD_PPAGESUBTITLES };
	CComboBox m_cbSubtitleRenderer;
	BOOL m_fPrioritizeExternalSubtitles;
	BOOL m_fDisableInternalSubtitles;
	BOOL m_fAutoReloadExtSubtitles;
	BOOL m_fUseSybresync;
	CString m_szAutoloadPaths;
	CComboBox m_ISDbCombo;
	CString m_ISDb;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	void UpdateSubRenderersList(int select);

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnUpdateButton2(CCmdUI* pCmdUI);
	afx_msg void OnURLModified();
	afx_msg void OnUpdateISRSelect(CCmdUI* pCmdUI);
	afx_msg void OnUpdateISRSelect2(CCmdUI* pCmdUI);
};

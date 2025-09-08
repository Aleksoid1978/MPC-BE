/*
 * (C) 2006-2025 see Authors.txt
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
	BOOL m_fPrioritizeExternalSubtitles = FALSE;
	BOOL m_fDisableInternalSubtitles    = FALSE;
	BOOL m_fAutoReloadExtSubtitles      = FALSE;
	BOOL m_fUseSybresync                = FALSE;
	CString m_szAutoloadPaths;
	CComboBox m_cbDefaultEncoding;
	BOOL m_bAutoDetectCodePage          = FALSE;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	virtual BOOL OnSetActive();

	void UpdateSubRenderersList(int select);

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnSubRendModified();
	afx_msg void OnUpdateISRSelect(CCmdUI* pCmdUI);
};

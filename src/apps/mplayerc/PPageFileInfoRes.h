/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2018 see Authors.txt
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

#include <afxwin.h>
#include <afxcmn.h>
#include "../../DSUtil/DSMPropertyBag.h"
#include "PPageBase.h"


// CPPageFileInfoRes dialog

class CPPageFileInfoRes : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageFileInfoRes)

private:
	HICON     m_hIcon;
	std::list<CDSMResource> m_resources;

	CStatic   m_icon;
	CString   m_fn;
	CString   m_fullfn;
	CListCtrl m_list;

public:
	CPPageFileInfoRes(CString fn, IFilterGraph* pFG);
	virtual ~CPPageFileInfoRes();

	enum { IDD = IDD_FILEPROPRES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnSetActive();
	virtual LRESULT OnSetPageFocus(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
	CRect  m_rCrt;
public:
	afx_msg void OnSaveAs();
	afx_msg void OnUpdateSaveAs(CCmdUI* pCmdUI);
};

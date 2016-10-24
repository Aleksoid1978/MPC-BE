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

#include "PPageFileInfoClip.h"
#include "PPageFileInfoDetails.h"
#include "PPageFileInfoRes.h"
#include "PPageFileMediaInfo.h"


class CMainFrame;

class CMPCPropertySheet: public CPropertySheet
{
	DECLARE_DYNAMIC(CMPCPropertySheet)

public:
	CMPCPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

	template <class T>
	void AddPage(T* pPage) {
		CPropertySheet::AddPage(pPage);
		m_IdMap[GetPageIndex(pPage)] = T::IDD;
	}

	void RemovePage(CPropertyPage* pPage) {
		m_IdMap.RemoveKey(GetPageIndex(pPage));
		CPropertySheet::RemovePage(pPage);
	}
	void RemovePage(int nPage) {
		m_IdMap.RemoveKey(nPage);
		CPropertySheet::RemovePage(nPage);
	}

	DWORD GetResourceId(int nPage) {
		return m_IdMap[nPage];
	}

protected:
	CAtlMap<int, DWORD> m_IdMap;
};

// CPPageFileInfoSheet

class CPPageFileInfoSheet : public CMPCPropertySheet
{
	DECLARE_DYNAMIC(CPPageFileInfoSheet)

private:
	CPPageFileInfoClip m_clip;
	CPPageFileInfoDetails m_details;
	CPPageFileInfoRes m_res;
	CPPageFileMediaInfo m_mi;

	CButton m_Button_MI_SaveAs;
	CButton m_Button_MI_Clipboard;

	CString m_fn;
	BOOL    m_bNeedInit;
	CRect   m_rCrt;
	CRect   m_rWnd;
	int     m_nMinCX;
	int     m_nMinCY;

public:
	CPPageFileInfoSheet(CString fn, CMainFrame* pMainFrame, CWnd* pParentWnd);
	virtual ~CPPageFileInfoSheet();

	afx_msg void OnSaveAs();
	afx_msg void OnCopyToClipboard();
	INT_PTR DoModal();

protected:
	virtual BOOL OnInitDialog();
	static int CALLBACK XmnPropSheetCallback(HWND hWnd, UINT message, LPARAM lParam);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()
};

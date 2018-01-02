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

#include <ResizableLib/ResizableDialog.h>

// CShaderAutoCompleteDlg dialog

class CShaderAutoCompleteDlg : public CResizableDialog
{
	TOOLINFO m_ti;
	HWND m_hToolTipWnd;
	WCHAR m_text[1024];

public:
	CShaderAutoCompleteDlg(CWnd* pParent = nullptr);
	virtual ~CShaderAutoCompleteDlg();

	struct hlslfunc_t {
		LPCWSTR str;
		LPCWSTR name;
		LPCWSTR desc;
	};
	static const std::vector<hlslfunc_t> m_HLSLFuncs;

	enum { IDD = IDD_SHADERAUTOCOMPLETE_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()

public:
	CListBox m_list;
	virtual BOOL OnInitDialog();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnLbnSelchangeList1();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
};

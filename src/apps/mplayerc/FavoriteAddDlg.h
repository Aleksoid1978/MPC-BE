/*
 * (C) 2003-2006 Gabest
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

#include <ExtLib/ui/ResizableLib/ResizableDialog.h>

// CFavoriteAddDlg dialog

class CFavoriteAddDlg : public CCmdUIDialog
{
	DECLARE_DYNAMIC(CFavoriteAddDlg)

private:
	CStringW m_fullname;
	std::list<CStringW> m_shortnames;

public:
	CFavoriteAddDlg(std::list<CStringW>& shortnames, CStringW fullname, BOOL bShowRelativeDrive = TRUE, CWnd* pParent = nullptr);
	virtual ~CFavoriteAddDlg();

	enum { IDD = IDD_FAVADD };

	CComboBox m_namectrl;
	CStringW m_name;
	BOOL m_bRememberPos = TRUE;
	BOOL m_bRelativeDrive = FALSE;

	BOOL m_bShowRelativeDrive;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnUpdateOk(CCmdUI* pCmdUI);
protected:
	virtual void OnOK();
};

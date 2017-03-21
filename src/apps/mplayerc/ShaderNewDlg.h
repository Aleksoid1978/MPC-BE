/*
 * (C) 2017 see Authors.txt
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

#include "../ExtLib/ui/CmdUI/CmdUI.h"

// ShaderNewDlg dialog

class CShaderNewDlg : public CCmdUIDialog
{
	DECLARE_DYNAMIC(CShaderNewDlg)

private:

public:
	CShaderNewDlg(CWnd* pParent = NULL);
	virtual ~CShaderNewDlg();

	enum { IDD = IDD_SHADERNEW_DLG };

	CString m_Name;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnUpdateOk(CCmdUI* pCmdUI);
protected:
	virtual void OnOK();
};

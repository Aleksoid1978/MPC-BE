/*
 * (C) 2014-2025 see Authors.txt
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
#include <HighDPI.h>
#include "ExtLib/ui/ResizableLib/ResizableDialog.h"

class CmdLineHelpDlg : public CResizableDialog, public CDPI
{
private:
	CStatic m_icon;
	CString m_cmdLine;
	CString m_text;

	CFont m_font;

public:
	CmdLineHelpDlg(const CString& cmdLine);
	virtual ~CmdLineHelpDlg();

	enum { IDD = IDD_CMD_LINE_HELP };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	afx_msg virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
};

/*
 * (C) 2016 see Authors.txt
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

#include <afxtaskdialog.h>

class CMainFrame;

// CThumbsTaskDlg dialog

class CThumbsTaskDlg : public CTaskDialog
{
	DECLARE_DYNAMIC(CThumbsTaskDlg)

private:
	CMainFrame*		pMainFrm;
	HICON			m_hIcon;
	HWND			m_TaskDlgHwnd;
	std::thread		m_Thread;

	CString			m_filename;

	volatile int	m_iProgress;
	volatile bool	m_bAbort;

	void SaveThumbnails(LPCTSTR filepath);

public:
	CThumbsTaskDlg(LPCTSTR filename);
	virtual ~CThumbsTaskDlg();

protected:
	virtual HRESULT OnInit();
	virtual HRESULT OnTimer(_In_ long lTime);
};

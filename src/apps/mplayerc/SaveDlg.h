/*
 * (C) 2003-2006 Gabest
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

#include <afxtaskdialog.h>

// CSaveDlg dialog

class CSaveDlg : public CTaskDialog
{
	DECLARE_DYNAMIC(CSaveDlg)

private:
	CString m_in, m_out;
	CComPtr<IGraphBuilder> pGB;
	CComQIPtr<IMediaControl> pMC;
	CComQIPtr<IMediaEventEx> pME;
	CComQIPtr<IMediaSeeking> pMS;
	HICON m_hIcon;
	HWND  m_TaskDlgHwnd;

public:
	CSaveDlg(CString in, CString name, CString out);
	virtual ~CSaveDlg();

protected:
	virtual HRESULT OnInit();
	virtual HRESULT OnTimer(_In_ long lTime);

	HRESULT InitFileCopy();
};

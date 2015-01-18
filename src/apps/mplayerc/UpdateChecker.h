/*
 * Copyright (C) 2013 Sergey "Exodus8" (rusguy6@gmail.com)
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
#include "afxdialogex.h"

struct Version
{
	unsigned major;
	unsigned minor;
	unsigned patch;
	unsigned revision;
};

enum Update_Status
{
	UPDATER_ERROR,
	UPDATER_NO_NEW_VERSION,
	UPDATER_NEW_VERSION_IS_AVAILABLE,
};

class UpdateChecker
{
	static bool bUpdating;
	static CCritSec csUpdating;

	static Version m_UpdateVersion;
	static CStringA m_UpdateURL;

public:
	UpdateChecker();
	~UpdateChecker(void);

	static bool IsTimeToAutoUpdate(int delay, time_t lastcheck);
	static void CheckForUpdate(bool autocheck = false);

private:
	static Update_Status CheckNewVersion();
	static UINT RunCheckForUpdateThread(LPVOID pParam);
};

class UpdateCheckerDlg : public CDialog
{
	DECLARE_DYNAMIC(UpdateCheckerDlg)

public:
	UpdateCheckerDlg(Update_Status updateStatus, Version UpdateVersion, LPCSTR UpdateURL, CWnd* pParent = NULL);
	virtual ~UpdateCheckerDlg();

	enum { IDD = IDD_UPDATE_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	afx_msg virtual BOOL OnInitDialog();
	afx_msg virtual void OnOK();

	DECLARE_MESSAGE_MAP()

private:
	Update_Status m_updateStatus;
	CString m_text;
	CStatic m_icon;
	CButton m_okButton;
	CButton m_cancelButton;

	CString m_latestURL;
};

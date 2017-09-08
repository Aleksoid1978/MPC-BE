/*
 * (C) 2013-2017 see Authors.txt
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

#include "stdafx.h"
#include <afxinet.h>
#include "../../DSUtil/SysVersion.h"
#include "UpdateChecker.h"

// UpdateChecker

bool UpdateChecker::bUpdating = false;
CCritSec UpdateChecker::csUpdating;
Version UpdateChecker::m_UpdateVersion = { 0, 0, 0, 0 };
CStringA UpdateChecker::m_UpdateURL;

UpdateChecker::UpdateChecker()
{
}

UpdateChecker::~UpdateChecker(void)
{
}

bool UpdateChecker::IsTimeToAutoUpdate(int delay, time_t lastcheck)
{
	return (time(nullptr) >= lastcheck + delay * 24 * 3600);
}

void UpdateChecker::CheckForUpdate(bool autocheck)
{
	CAutoLock lock(&csUpdating);

	if (!bUpdating) {
		bUpdating = true;
		AfxBeginThread(RunCheckForUpdateThread, (LPVOID)autocheck);
	}
}

Update_Status UpdateChecker::CheckNewVersion()
{
	m_UpdateURL.Empty();
	m_UpdateVersion = { 0, 0, 0, 0 };

	Update_Status updatestatus = UPDATER_ERROR;
	CStringA updateinfo;

	HINTERNET hUrl;
	HINTERNET hInet = InternetOpen(L"MPC-BE", 0, nullptr, nullptr, 0);
	if (hInet) {
		hUrl = InternetOpenUrl(hInet, L"http://mpc-be.org/version.txt", nullptr, 0, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0);
		if (hUrl) {
			char buffer[1024] = { 0 }; // limit update file to 1024 bytes
			DWORD dwBytesRead = 0;

			if (InternetReadFile(hUrl, (LPVOID)buffer, _countof(buffer), &dwBytesRead) == TRUE && dwBytesRead < _countof(buffer)) {
				updateinfo = CStringA(buffer);
			}
			InternetCloseHandle(hUrl);
		}
		InternetCloseHandle(hInet);
	}

	if (updateinfo.GetLength()) {
		int pos = 0;
		CStringA updateversion = updateinfo.Tokenize("\r\n", pos).Trim();
		m_UpdateURL = updateinfo.Tokenize("\r\n", pos).Trim();

		if (sscanf_s(updateversion, "%u.%u.%u.%u", &m_UpdateVersion.major, &m_UpdateVersion.minor, &m_UpdateVersion.patch, &m_UpdateVersion.revision) == 4) {
			if (MPC_VERSION_MAJOR < m_UpdateVersion.major
					|| MPC_VERSION_MAJOR == m_UpdateVersion.major && MPC_VERSION_MINOR < m_UpdateVersion.minor
					|| MPC_VERSION_MAJOR == m_UpdateVersion.major && MPC_VERSION_MINOR == m_UpdateVersion.minor && MPC_VERSION_PATCH < m_UpdateVersion.patch
					|| MPC_VERSION_MAJOR == m_UpdateVersion.major && MPC_VERSION_MINOR == m_UpdateVersion.minor && MPC_VERSION_PATCH == m_UpdateVersion.patch && MPC_VERSION_REV < m_UpdateVersion.revision) {
				updatestatus = UPDATER_NEW_VERSION_IS_AVAILABLE;
			} else {
				updatestatus = UPDATER_NO_NEW_VERSION;
			}
		}
	}

	return updatestatus;
}

UINT UpdateChecker::RunCheckForUpdateThread(LPVOID pParam)
{
	bool autocheck = !!pParam;
	Update_Status updatestatus = CheckNewVersion();


	if (!autocheck || updatestatus == UPDATER_NEW_VERSION_IS_AVAILABLE) {
		UpdateCheckerDlg dlg(updatestatus, m_UpdateVersion, m_UpdateURL);
		dlg.DoModal();
	}

	if (autocheck && updatestatus != UPDATER_ERROR) {
		AfxGetAppSettings().tUpdaterLastCheck = time(nullptr);
	}

	bUpdating = false;

	return 0;
}

// UpdateCheckerDlg

IMPLEMENT_DYNAMIC(UpdateCheckerDlg, CDialog)

UpdateCheckerDlg::UpdateCheckerDlg(Update_Status updateStatus, Version UpdateVersion, LPCSTR UpdateURL, CWnd* pParent)
	: CDialog(UpdateCheckerDlg::IDD, pParent), m_updateStatus(updateStatus)
{
	CString VersionStr;

	switch (updateStatus) {
	case UPDATER_ERROR:
		m_text.LoadString(IDS_UPDATE_ERROR);
		break;
	case UPDATER_NO_NEW_VERSION:
		VersionStr.Format(L"%s (build %d)", _T(MPC_VERSION_STR), MPC_VERSION_REV);
		m_text.Format(IDS_USING_NEWER_VERSION, VersionStr);
		break;
	case UPDATER_NEW_VERSION_IS_AVAILABLE:
		VersionStr.Format(L"v%u.%u.%u (build %u)", UpdateVersion.major, UpdateVersion.minor, UpdateVersion.patch, UpdateVersion.revision);
		m_text.Format(IDS_NEW_UPDATE_AVAILABLE, VersionStr);
		m_latestURL = UpdateURL;
		break;

	default:
		ASSERT(0);
	}
}

UpdateCheckerDlg::~UpdateCheckerDlg()
{
}

void UpdateCheckerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_UPDATE_DLG_TEXT, m_text);
	DDX_Control(pDX, IDC_UPDATE_ICON, m_icon);
	DDX_Control(pDX, IDOK, m_okButton);
	DDX_Control(pDX, IDCANCEL, m_cancelButton);
}

BEGIN_MESSAGE_MAP(UpdateCheckerDlg, CDialog)
END_MESSAGE_MAP()

BOOL UpdateCheckerDlg::OnInitDialog()
{
	__super::OnInitDialog();

	switch (m_updateStatus) {
	case UPDATER_NEW_VERSION_IS_AVAILABLE:
		m_icon.SetIcon(LoadIcon(nullptr, IDI_QUESTION));
		break;
	case UPDATER_NO_NEW_VERSION:
	case UPDATER_ERROR:
		m_icon.SetIcon(LoadIcon(nullptr, (m_updateStatus == UPDATER_ERROR) ? IDI_WARNING : IDI_INFORMATION));
		m_okButton.ShowWindow(SW_HIDE);
		m_cancelButton.SetWindowText(ResStr(IDS_UPDATE_CLOSE));
		m_cancelButton.SetFocus();
		break;
	default:
		ASSERT(0);
	}

	return TRUE;
}

void UpdateCheckerDlg::OnOK()
{
	if (m_updateStatus == UPDATER_NEW_VERSION_IS_AVAILABLE) {
		ShellExecute(nullptr, L"open", m_latestURL, nullptr, nullptr, SW_SHOWDEFAULT);
	}

	__super::OnOK();
}

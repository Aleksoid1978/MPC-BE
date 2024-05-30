/*
 * (C) 2013-2024 see Authors.txt
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
#include "DSUtil/SysVersion.h"
#include "DSUtil/text.h"
#include "DSUtil/HTTPAsync.h"
#include "rapidjsonHelper.h"
#include "UpdateChecker.h"

#include "Version.h"

// UpdateChecker

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
	m_UpdateVersion = {};

	Update_Status updatestatus = UPDATER_ERROR_CONNECT;
	CHTTPAsync HTTPAsync;
	if (SUCCEEDED(HTTPAsync.Connect(L"https://api.github.com/repos/Aleksoid1978/MPC-BE/releases/latest", http::connectTimeout))) {
		constexpr auto sizeRead = 16 * KILOBYTE;
		CStringA data;
		DWORD dwSizeRead = 0;
		int dataSize = 0;
		while (S_OK == HTTPAsync.Read(reinterpret_cast<PBYTE>(data.GetBuffer(dataSize + sizeRead) + dataSize), sizeRead, dwSizeRead, http::readTimeout)) {
			data.ReleaseBuffer(dataSize + dwSizeRead);
			dataSize = data.GetLength();
			dwSizeRead = 0;
		}
		data.ReleaseBuffer(dataSize);

		if (!data.IsEmpty()) {
			updatestatus = UPDATER_ERROR_DATA;
			rapidjson::Document json;
			if (!json.Parse(data.GetString()).HasParseError()) {
				CString tag_name;
				if (getJsonValue(json, "tag_name", tag_name)) {
					int n = swscanf_s(tag_name, L"%u.%u.%u.%u",
									  &m_UpdateVersion.major, &m_UpdateVersion.minor, &m_UpdateVersion.patch, &m_UpdateVersion.revision);
					if (n == 3 || n == 4) {
						if (getJsonValue(json, "html_url", m_UpdateURL)) {
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
				}
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

	if (autocheck && updatestatus >= UPDATER_NO_NEW_VERSION) {
		AfxGetAppSettings().tUpdaterLastCheck = time(nullptr);
	}

	bUpdating = false;

	return 0;
}

// UpdateCheckerDlg

IMPLEMENT_DYNAMIC(UpdateCheckerDlg, CDialog)

UpdateCheckerDlg::UpdateCheckerDlg(Update_Status updateStatus, Version UpdateVersion, LPCWSTR UpdateURL)
	: CDialog(UpdateCheckerDlg::IDD), m_updateStatus(updateStatus)
{
	CString VersionStr;

	switch (updateStatus) {
	case UPDATER_ERROR_CONNECT:
		m_text.LoadString(IDS_UPDATE_ERROR_CONNECT);
		break;
	case UPDATER_ERROR_DATA:
		m_text.LoadString(IDS_UPDATE_ERROR_DATA);
		break;
	case UPDATER_NO_NEW_VERSION:
		VersionStr.SetString(MPC_VERSION_WSTR);
		m_text.Format(IDS_USING_NEWER_VERSION, VersionStr);
		break;
	case UPDATER_NEW_VERSION_IS_AVAILABLE:
		VersionStr.Format(L"%u.%u.%u", UpdateVersion.major, UpdateVersion.minor, UpdateVersion.patch);
		if (UpdateVersion.revision) {
			VersionStr.AppendFormat(L".%u", UpdateVersion.revision);
		}
		m_text.Format(IDS_NEW_UPDATE_AVAILABLE, VersionStr);
		m_latestURL = UpdateURL;
		break;

	default:
		ASSERT(0);
	}
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
		m_icon.SetIcon(LoadIconW(nullptr, IDI_QUESTION));
		break;
	case UPDATER_NO_NEW_VERSION:
	case UPDATER_ERROR_CONNECT:
	case UPDATER_ERROR_DATA:
		m_icon.SetIcon(LoadIconW(nullptr, (m_updateStatus == UPDATER_NO_NEW_VERSION) ? IDI_INFORMATION : IDI_WARNING));
		m_okButton.ShowWindow(SW_HIDE);
		m_cancelButton.SetWindowTextW(ResStr(IDS_UPDATE_CLOSE));
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
		ShellExecuteW(nullptr, L"open", m_latestURL, nullptr, nullptr, SW_SHOWDEFAULT);
	}

	__super::OnOK();
}

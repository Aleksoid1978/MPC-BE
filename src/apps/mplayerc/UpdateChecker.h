/*
 * (C) 2013-2026 see Authors.txt
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
	UPDATER_ERROR_CONNECT,
	UPDATER_ERROR_DATA,
	UPDATER_NO_NEW_VERSION,
	UPDATER_NEW_VERSION_IS_AVAILABLE,
};

class UpdateChecker
{
	static inline bool bUpdating = false;
	static inline CCritSec csUpdating;

	static inline Version m_UpdateVersion = {};
	static inline CString m_UpdateURL;

public:
	UpdateChecker() = default;
	~UpdateChecker() = default;

	static bool IsTimeToAutoUpdate(int delay, time_t lastcheck);
	static void CheckForUpdate(bool autocheck = false);

private:
	static Update_Status CheckNewVersion();
	static UINT RunCheckForUpdateThread(LPVOID pParam);
};

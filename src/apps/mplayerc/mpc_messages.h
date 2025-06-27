/*
 * (C) 2022-2025 see Authors.txt
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

enum : UINT {
	WM_HANDLE_CMDLINE   = WM_USER + 300,
	WM_DXVASTATE_CHANGE = WM_USER + 301,
	WM_OSD_HIDE         = WM_USER + 1001,
	WM_OSD_DRAW         = WM_USER + 1002,

	WM_RESIZE_DEVICE    = WM_APP + 1,
	WM_RESET_DEVICE,
	WM_GRAPHNOTIFY      = WM_RESET_DEVICE + 1,
	WM_TUNER_SCAN_PROGRESS,
	WM_TUNER_SCAN_END,
	WM_TUNER_STATS,
	WM_TUNER_NEW_CHANNEL,
	WM_POSTOPEN,
	WM_SAVESETTINGS,

	SETPAGEFOCUS            = WM_APP + 252,
	EDIT_BUTTON_LEFTCLICKED = WM_APP + 842,
	WM_MPCVR_SWITCH_FULLSCREEN = WM_APP + 4096,
	WM_RESTORE,
};

/*
 * (C) 2011-2014 see Authors.txt
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

struct IDirect3D9;

BOOL				IsCompositionEnabled();
bool				SetPrivilege(LPCTSTR privilege, bool bEnable=true);
bool				ExportRegistryKey(CStdioFile& file, HKEY hKeyRoot, CString keyName);
UINT				GetAdapter(IDirect3D9* pD3D, HWND hWnd);
bool				IsFontInstalled(LPCTSTR lpszFont);
bool				ExploreToFile(CString path);
bool				ReadDisplay(CString szDevice, CString* MonitorName, UINT16* MonitorHorRes, UINT16* MonitorVerRes);

BOOL				CFileGetStatus(LPCTSTR lpszFileName, CFileStatus& status);

BOOL				IsUserAdmin();

CString				GetLastErrorMsg(LPTSTR lpszFunction, DWORD dw = GetLastError());

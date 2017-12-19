/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2015 see Authors.txt
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

class CFontInstaller
{
	HANDLE (WINAPI *pAddFontMemResourceEx)(PVOID,DWORD,PVOID,DWORD*);
	BOOL (WINAPI *pRemoveFontMemResourceEx)(HANDLE);
	int (WINAPI *pAddFontResourceEx)(LPCWSTR,DWORD,PVOID);
	BOOL (WINAPI *pRemoveFontResourceEx)(LPCWSTR,DWORD,PVOID);

	CAtlList<HANDLE> m_fonts;
	CAtlList<CString> m_files;
	CAtlList<CString> m_tempfiles;

public:
	CFontInstaller();
	virtual ~CFontInstaller();

	bool InstallFontMemory(const void* pData, UINT len);
	bool InstallFontFile(LPCWSTR filename);
	bool InstallFontTempFile(const void* pData, UINT len);

	void UninstallFonts();
};

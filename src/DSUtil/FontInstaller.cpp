/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2017 see Authors.txt
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
#include "FontInstaller.h"

CFontInstaller::CFontInstaller()
{
	if (HMODULE hGdi = GetModuleHandleW(L"gdi32.dll")) {
		pAddFontMemResourceEx = (HANDLE (WINAPI *)(PVOID,DWORD,PVOID,DWORD*))GetProcAddress(hGdi, "AddFontMemResourceEx");
		pAddFontResourceEx = (int (WINAPI *)(LPCWSTR,DWORD,PVOID))GetProcAddress(hGdi, "AddFontResourceExW");
		pRemoveFontMemResourceEx = (BOOL (WINAPI *)(HANDLE))GetProcAddress(hGdi, "RemoveFontMemResourceEx");
		pRemoveFontResourceEx = (BOOL (WINAPI *)(LPCWSTR,DWORD,PVOID))GetProcAddress(hGdi, "RemoveFontResourceExW");
	}
}

CFontInstaller::~CFontInstaller()
{
	UninstallFonts();
}

void CFontInstaller::UninstallFonts()
{
	if (pRemoveFontMemResourceEx) {
		for (const auto& font : m_fonts) {
			pRemoveFontMemResourceEx(font);
		}
		m_fonts.clear();
	}

	if (pRemoveFontResourceEx) {
		for (const auto& fn : m_files) {
			pRemoveFontResourceEx(fn, FR_PRIVATE, 0);
		}
		m_files.clear();

		for (const auto& fn : m_tempfiles) {
			pRemoveFontResourceEx(fn, FR_PRIVATE, 0);
			if (!DeleteFileW(fn)) {
				// Temp file can not be deleted now, it will be deleted after the reboot.
				MoveFileExW(fn, nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);
			}
		}
		m_tempfiles.clear();
	}
}

bool CFontInstaller::InstallFontMemory(const void* pData, UINT len)
{
	if (!pAddFontMemResourceEx) {
		return false;
	}

	DWORD nFonts = 0;
	HANDLE hFont = pAddFontMemResourceEx((PVOID)pData, len, nullptr, &nFonts);
	if (hFont && nFonts > 0) {
		m_fonts.push_back(hFont);
	}
	return hFont && nFonts > 0;
}

bool CFontInstaller::InstallFontFile(LPCWSTR filename)
{
	if (pAddFontResourceEx && pAddFontResourceEx(filename, FR_PRIVATE, 0) > 0) {
		m_files.push_back(filename);
		return true;
	}

	return false;
}

bool CFontInstaller::InstallFontTempFile(const void* pData, UINT len)
{
	if (!pAddFontResourceEx) {
		return false;
	}

	CFile f;
	WCHAR path[MAX_PATH], fn[MAX_PATH];
	if (!GetTempPathW(MAX_PATH, path) || !GetTempFileNameW(path, L"g_font", 0, fn)) {
		return false;
	}

	if (f.Open(fn, CFile::modeWrite)) {
		f.Write(pData, len);
		f.Close();

		if (pAddFontResourceEx(fn, FR_PRIVATE, 0) > 0) {
			m_tempfiles.push_back(fn);
			return true;
		}
	}

	DeleteFileW(fn);
	return false;
}

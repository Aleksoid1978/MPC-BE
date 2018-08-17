/*
 * (C) 2012-2018 see Authors.txt
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
#include <algorithm>
#include <shellapi.h>
#include "MPCBEContextMenu.h"

#define MPC_WND_CLASS_NAME L"MPC-BE"

// CMPCBEContextMenu
#define ID_MPCBE_PLAY 0

#define PLAY_MPC_RU  L"&Воспроизвести в MPC-BE"
#define ADDTO_MPC_RU L"&Добавить в плейлист MPC-BE"

#define PLAY_MPC_EN  L"&Play with MPC-BE"
#define ADDTO_MPC_EN L"&Add to MPC-BE Playlist"

static HBITMAP TransparentBitmap(HBITMAP hBmp)
{
	HBITMAP RetBmp = NULL;
	if (hBmp) {	
		HDC BufferDC = CreateCompatibleDC(NULL);	// DC for Source Bitmap
		if (BufferDC) {

			HGDIOBJ PreviousBufferObject = SelectObject(BufferDC,hBmp);
			// here BufferDC contains the bitmap

			HDC DirectDC = CreateCompatibleDC(NULL); // DC for working
			if (DirectDC) {
				// Get bitmap size

				BITMAP bm;
				GetObject(hBmp, sizeof(bm), &bm);

				// create a BITMAPINFO for the CreateDIBSection
				BITMAPINFO RGB32BitsBITMAPINFO = { 0 }; 
				RGB32BitsBITMAPINFO.bmiHeader.biSize      = sizeof(BITMAPINFOHEADER);
				RGB32BitsBITMAPINFO.bmiHeader.biWidth     = bm.bmWidth;
				RGB32BitsBITMAPINFO.bmiHeader.biHeight    = bm.bmHeight;
				RGB32BitsBITMAPINFO.bmiHeader.biPlanes    = 1;
				RGB32BitsBITMAPINFO.bmiHeader.biBitCount  = 32;
				RGB32BitsBITMAPINFO.bmiHeader.biSizeImage = bm.bmWidth * bm.bmHeight * 4;

				// pointer used for direct Bitmap pixels access
				UINT* ptPixels;	

				HBITMAP DirectBitmap = CreateDIBSection(DirectDC, 
														&RGB32BitsBITMAPINFO, 
														DIB_RGB_COLORS,
														(void **)&ptPixels, 
														NULL, 0);
				if (DirectBitmap) {
					// here DirectBitmap!=NULL so ptPixels!=NULL no need to test
					HGDIOBJ PreviousObject = SelectObject(DirectDC, DirectBitmap);
					BitBlt(DirectDC, 0, 0,
						   bm.bmWidth, bm.bmHeight,
						   BufferDC, 0, 0,
						   SRCCOPY);

					// Don't delete the result of SelectObject because it's 
					// our modified bitmap (DirectBitmap)

					SelectObject(DirectDC,PreviousObject);

					// finish
					RetBmp = DirectBitmap;
				}
				// clean up
				DeleteDC(DirectDC);
			}			
			SelectObject(BufferDC,PreviousBufferObject);

			// BufferDC is now useless
			DeleteDC(BufferDC);
		}
	}

	return RetBmp;
}

CMPCBEContextMenu::CMPCBEContextMenu()
	: m_hPlayBmp(TransparentBitmap(LoadBitmap(_AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCE(IDB_MPCBEBMP_PLAY))))
	, m_hAddBmp(TransparentBitmap(LoadBitmap(_AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCE(IDB_MPCBEBMP_ADD))))
{
}

CMPCBEContextMenu::~CMPCBEContextMenu()
{
	if (m_hPlayBmp) {
		DeleteObject(m_hPlayBmp);
	}
	if (m_hAddBmp) {
		DeleteObject(m_hAddBmp);
	}
}

// CMPCBEShellContextMenu IContextMenu methods

STDMETHODIMP CMPCBEContextMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
	// If the flags include CMF_DEFAULTONLY then we shouldn't do anything.
	if ((uFlags & CMF_DEFAULTONLY)
			|| m_fileNames.size() == 0) {
		return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
	}

	CString PLAY_MPC  = (GetUserDefaultUILanguage() == 1049) ? PLAY_MPC_RU	: PLAY_MPC_EN;
	CString ADDTO_MPC = (GetUserDefaultUILanguage() == 1049) ? ADDTO_MPC_RU	: ADDTO_MPC_EN;

	CRegKey key;
	if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, L"Software\\MPC-BE\\ShellExt")) {
		WCHAR path_buff[MAX_PATH] = { 0 };
		ULONG len = sizeof(path_buff);

		if (ERROR_SUCCESS == key.QueryStringValue(L"Play", path_buff, &len)) {
			PLAY_MPC = path_buff;
		}

		memset(path_buff, 0, sizeof(path_buff));
		len = sizeof(path_buff);
		if (ERROR_SUCCESS == key.QueryStringValue(L"Add", path_buff, &len)) {
			ADDTO_MPC = path_buff;
		}

		key.Close();
	}


	::InsertMenu(hmenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL);

	::InsertMenu(hmenu, indexMenu, MF_STRING | MF_BYPOSITION, idCmdFirst + ID_MPCBE_PLAY, PLAY_MPC);
	if (NULL != m_hPlayBmp) {
		::SetMenuItemBitmaps(hmenu, indexMenu, MF_BYPOSITION, m_hPlayBmp, NULL);
	}
	indexMenu++;

	::InsertMenu(hmenu, indexMenu, MF_STRING | MF_BYPOSITION, idCmdFirst + ID_MPCBE_PLAY + 1, ADDTO_MPC);
	if (NULL != m_hAddBmp) {
		::SetMenuItemBitmaps(hmenu, indexMenu, MF_BYPOSITION, m_hAddBmp, NULL);
	}
	indexMenu++;

	::InsertMenu(hmenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL);
	return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, (ID_MPCBE_PLAY + 2));
}

// CMPCBEShellContextMenu IShellExtInit methods
STDMETHODIMP CMPCBEContextMenu::Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, HKEY hkeyProgID)
{
	HRESULT   hr        = E_INVALIDARG;
	FORMATETC fmte      = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	STGMEDIUM stg       = { TYMED_HGLOBAL };
	UINT      uNumFiles = 0;
	WCHAR     strFilePath[MAX_PATH] = { 0 };

	// No data object
	if (!lpdobj) {
		return hr;
	}

	// Use the given IDataObject to get a list of filenames (CF_HDROP).
	if (FAILED(hr = lpdobj->GetData(&fmte, &stg))) {
		return E_INVALIDARG;
	}

	// Get a pointer to the actual data.
	HDROP hDrop = (HDROP)GlobalLock(stg.hGlobal);

	// Make sure it worked.
	if (!hDrop) {
		ReleaseStgMedium(&stg);
		return E_INVALIDARG;
	}

	// Make sure HDROP contains at least one file.
	if ((uNumFiles = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0)) >= 1) {
		for (UINT i = 0; i < uNumFiles; i++) {
			DragQueryFileW(hDrop, i, strFilePath, MAX_PATH);
			if (GetFileAttributesW(strFilePath) & FILE_ATTRIBUTE_DIRECTORY) {
				size_t nLen = wcslen(strFilePath);
				if (strFilePath[nLen - 1] != L'\\') {
					wcscat_s(strFilePath, L"\\");
				}
			}
			// Add the file name to the list
			m_fileNames.emplace_back(strFilePath);
		}
		hr = S_OK;
		
		// sort list by path
		static HMODULE h = LoadLibraryW(L"Shlwapi.dll");
		if (h) {
			typedef int (WINAPI *StrCmpLogicalW)(_In_ PCWSTR psz1, _In_ PCWSTR psz2);
			static StrCmpLogicalW pStrCmpLogicalW = (StrCmpLogicalW)GetProcAddress(h, "StrCmpLogicalW");
			if (pStrCmpLogicalW) {
				std::sort(m_fileNames.begin(), m_fileNames.end(), [](const CString& a, const CString& b) {
					return pStrCmpLogicalW(a, b) < 0;
				});
			}
		}
	} else {
		hr = E_INVALIDARG;
	}

	// Release the data.
	GlobalUnlock(stg.hGlobal);
	ReleaseStgMedium(&stg);

	return hr;
}

STDMETHODIMP CMPCBEContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
	if (0 != HIWORD(lpici->lpVerb)) {
		return E_INVALIDARG;
	}

	HRESULT hr = S_OK;
	switch (LOWORD(lpici->lpVerb)) {
		case ID_MPCBE_PLAY:
			SendData(false);
			break;
		case ID_MPCBE_PLAY + 1:
			SendData(true);
			break;
		default:
			hr = E_INVALIDARG;
	}

	return hr;
}

void CMPCBEContextMenu::SendData(bool add_pl)
{
	if (add_pl) {
		m_fileNames.emplace_back(L"/add");
	}

	size_t bufflen = sizeof(DWORD);

	for (const auto& item : m_fileNames) {
		bufflen += (item.GetLength() + 1) * sizeof(WCHAR);
	}

	std::vector<BYTE> buff(bufflen);
	if (buff.empty()) {
		return;
	}

	BYTE* p = buff.data();

	*(DWORD*)p = (DWORD)m_fileNames.size(); 
	p += sizeof(DWORD);

	for (const auto& item : m_fileNames) {
		const size_t len = (item.GetLength() + 1) * sizeof(WCHAR);
		memcpy(p, item, len);
		p += len;
	}

	if (HWND hWnd = ::FindWindowW(MPC_WND_CLASS_NAME, NULL)) {
		COPYDATASTRUCT cds;
		cds.dwData = 0x6ABE51;
		cds.cbData = (DWORD)bufflen;
		cds.lpData = buff.data();
		SendMessageW(hWnd, WM_COPYDATA, (WPARAM)nullptr, (LPARAM)&cds);
	} else {
		CRegKey key;
		WCHAR path_buff[MAX_PATH] = { 0 };
		ULONG len = sizeof(path_buff);
		CString mpc_path;
		
		if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, L"Software\\MPC-BE\\ShellExt")) {
			if (ERROR_SUCCESS == key.QueryStringValue(L"MpcPath", path_buff, &len) && ::PathFileExistsW(path_buff)) {
				mpc_path = path_buff;
			}
			key.Close();
		}

		if (mpc_path.Trim().IsEmpty()) {
			if (ERROR_SUCCESS == key.Open(HKEY_LOCAL_MACHINE, L"Software\\MPC-BE")) {
				memset(path_buff, 0, sizeof(path_buff));
				len = sizeof(path_buff);
				if (ERROR_SUCCESS == key.QueryStringValue(L"ExePath", path_buff, &len) && ::PathFileExistsW(path_buff)) {
					mpc_path = path_buff;
				}
				key.Close();
			}
		}

#ifdef _WIN64
		if (mpc_path.Trim().IsEmpty()) {
			if (ERROR_SUCCESS == key.Open(HKEY_LOCAL_MACHINE, L"Software\\Wow6432Node\\MPC-BE")) {
				memset(path_buff, 0, sizeof(path_buff));
				len = sizeof(path_buff);
				if (ERROR_SUCCESS == key.QueryStringValue(L"ExePath", path_buff, &len) && ::PathFileExistsW(path_buff)) {
					mpc_path = path_buff;
				}
				key.Close();
			}
		}
#endif

		if (!mpc_path.Trim().IsEmpty()) {
			if (HINSTANCE(HINSTANCE_ERROR) < ShellExecute(NULL, L"", path_buff, NULL, 0, SW_SHOWDEFAULT)) {
				Sleep(100);
				int wait_count = 0;
				HWND hWnd = ::FindWindowW(MPC_WND_CLASS_NAME, NULL);
				while (!hWnd && (wait_count++ < 200)) {
					Sleep(100);
					hWnd = ::FindWindowW(MPC_WND_CLASS_NAME, NULL);
				}

				if (hWnd && (wait_count < 200)) {
					COPYDATASTRUCT cds;
					cds.dwData = 0x6ABE51;
					cds.cbData = (DWORD)bufflen;
					cds.lpData = buff.data();
					SendMessageW(hWnd, WM_COPYDATA, (WPARAM)nullptr, (LPARAM)&cds);
				} 
			}
		}
	}
}

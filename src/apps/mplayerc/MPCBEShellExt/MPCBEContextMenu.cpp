/*
 * Copyright (C) 2012 Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
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
#include <VersionHelpers.h>
#include "MPCBEContextMenu.h"

#define MPC_WND_CLASS_NAME L"MPC-BE"

// CMPCBEContextMenu
#define PLAY_MPC_RU		L"&Воспроизвести в MPC-BE"
#define ADDTO_MPC_RU	L"&Добавить в плейлист MPC-BE"

#define PLAY_MPC_EN		L"&Play with MPC-BE"
#define ADDTO_MPC_EN	L"&Add to MPC-BE Playlist"

HBITMAP TransparentBMP(HBITMAP hBmp)
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

				// create a BITMAPINFO with minimal initilisation 

				// for the CreateDIBSection

				BITMAPINFO RGB32BitsBITMAPINFO; 
				ZeroMemory(&RGB32BitsBITMAPINFO,sizeof(BITMAPINFO));
				RGB32BitsBITMAPINFO.bmiHeader.biSize		= sizeof(BITMAPINFOHEADER);
				RGB32BitsBITMAPINFO.bmiHeader.biWidth		= bm.bmWidth;
				RGB32BitsBITMAPINFO.bmiHeader.biHeight		= bm.bmHeight;
				RGB32BitsBITMAPINFO.bmiHeader.biPlanes		= 1;
				RGB32BitsBITMAPINFO.bmiHeader.biBitCount	= 32;

				// pointer used for direct Bitmap pixels access
				UINT * ptPixels;	

				HBITMAP DirectBitmap = CreateDIBSection(DirectDC, 
					(BITMAPINFO *)&RGB32BitsBITMAPINFO, 
					DIB_RGB_COLORS,
					(void **)&ptPixels, 
					NULL, 0);
				if (DirectBitmap) {
					// here DirectBitmap!=NULL so ptPixels!=NULL no need to test
					HGDIOBJ PreviousObject = SelectObject(DirectDC, DirectBitmap);
					BitBlt(DirectDC,0,0,
						bm.bmWidth,bm.bmHeight,
						BufferDC,0,0,SRCCOPY);

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
{
	if (IsWindowsVistaOrGreater()) {
		m_hPlayBmp	= LoadBitmap( _AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCE(IDB_MPCBEBMP_PLAY));
		m_hAddBmp	= LoadBitmap( _AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCE(IDB_MPCBEBMP_ADD));
	} else {
		m_hPlayBmp	= LoadBitmap( _AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCE(IDB_MPCBEBMP_PLAY_XP));
		m_hAddBmp	= LoadBitmap( _AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCE(IDB_MPCBEBMP_ADD_XP));
	}
}

CMPCBEContextMenu::~CMPCBEContextMenu()
{
	if (NULL != m_hPlayBmp) {
		DeleteObject(m_hPlayBmp);
	}
	if (NULL != m_hAddBmp) {
		DeleteObject(m_hAddBmp);
	}
}

// CMPCBEShellContextMenu IContextMenu methods

STDMETHODIMP CMPCBEContextMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
	CString PLAY_MPC	= (GetUserDefaultUILanguage() == 1049) ? PLAY_MPC_RU	: PLAY_MPC_EN;
	CString ADDTO_MPC	= (GetUserDefaultUILanguage() == 1049) ? ADDTO_MPC_RU	: ADDTO_MPC_EN;				

	CRegKey key;
	if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, L"Software\\MPC-BE\\ShellExt")) {
		TCHAR path_buff[MAX_PATH];
		memset(path_buff, 0, sizeof(path_buff));
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

	if (m_listFileNames.GetCount() > 0) {
		::InsertMenu(hmenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL);

		::InsertMenu(hmenu, indexMenu, MF_STRING | MF_BYPOSITION, idCmdFirst + ID_MPCBE_PLAY, PLAY_MPC);
		if (NULL != m_hPlayBmp) {
			SetMenuItemBitmaps(hmenu, indexMenu, MF_BYPOSITION, TransparentBMP(m_hPlayBmp), NULL);
		}
		indexMenu++;

		::InsertMenu(hmenu, indexMenu, MF_STRING | MF_BYPOSITION, idCmdFirst + ID_MPCBE_PLAY + 1, ADDTO_MPC);
		if (NULL != m_hAddBmp) {
			SetMenuItemBitmaps(hmenu, indexMenu, MF_BYPOSITION, TransparentBMP(m_hAddBmp), NULL);
		}
		indexMenu++;

		::InsertMenu(hmenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL);
		return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_NULL, (USHORT)(ID_MPCBE_PLAY + 2)));
	}
	else return 0;
}

// CMPCBEShellContextMenu IShellExtInit methods

typedef int (__stdcall *StrCmpLogicalWPtr)(LPCWSTR arg1, LPCWSTR arg2);
StrCmpLogicalWPtr pStrCmpLogicalW;

typedef struct {LPCTSTR str; POSITION pos;} plsort_t;
int compare(const void* arg1, const void* arg2)
{
	return pStrCmpLogicalW(((plsort_t*)arg1)->str, ((plsort_t*)arg2)->str);
}


STDMETHODIMP CMPCBEContextMenu::Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, HKEY hkeyProgID)
{
	HRESULT		hr		= E_FAIL;
	FORMATETC	fmte	= { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	STGMEDIUM	medium;
	UINT		nFileCount = 0;
	TCHAR		strFilePath[MAX_PATH];

	// No data object
	if (lpdobj == NULL) {
		return hr;
	}

	// Use the given IDataObject to get a list of filenames (CF_HDROP).
	hr = lpdobj->GetData(&fmte, &medium);

	if (FAILED(hr)) {
		return hr;
	}

	// Make sure HDROP contains at least one file.
	if ((nFileCount = DragQueryFile((HDROP)medium.hGlobal, (UINT)(-1), NULL, 0)) >= 1) {
		for (UINT i = 0; i < nFileCount; i++) {
			DragQueryFile((HDROP)medium.hGlobal, i, strFilePath, MAX_PATH);
			if (::GetFileAttributes(strFilePath) & FILE_ATTRIBUTE_DIRECTORY) {
				size_t nLen = _tcslen(strFilePath);
				if (strFilePath[nLen - 1] != L'\\') {
					wcscat_s(strFilePath, L"\\");
				}
			}
			// Add the file name to the list
			m_listFileNames.AddTail(strFilePath);
		}
		hr = S_OK;
		
		// sort list by path
		pStrCmpLogicalW = NULL;
		HMODULE h = LoadLibrary(L"Shlwapi.dll");
		if (h) {
			pStrCmpLogicalW = (StrCmpLogicalWPtr)GetProcAddress(h, "StrCmpLogicalW");
			if (pStrCmpLogicalW) {
				CAtlArray<plsort_t> a;
				a.SetCount(m_listFileNames.GetCount());
				POSITION pos = m_listFileNames.GetHeadPosition();
				for(size_t i = 0; pos; i++, m_listFileNames.GetNext(pos)) {
					a[i].str = m_listFileNames.GetAt(pos), a[i].pos = pos;
				}
				qsort(a.GetData(), a.GetCount(), sizeof(plsort_t), compare);
				for(size_t i = 0; i < a.GetCount(); i++) {
					m_listFileNames.AddTail(m_listFileNames.GetAt(a[i].pos));
					m_listFileNames.RemoveAt(a[i].pos);
				}
			}
			FreeLibrary(h);
		}
	} else {
		hr = E_FAIL;
	}

	// Release the data.
	ReleaseStgMedium(&medium);

	return hr;
}

STDMETHODIMP CMPCBEContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
	switch (LOWORD(lpici->lpVerb)) {		
		case ID_MPCBE_PLAY:
			SendData(false);
			break;
		case ID_MPCBE_PLAY+1:
			SendData(true);
			break;
	}
	return S_OK;
}

void CMPCBEContextMenu::SendData(bool add_pl)
{
	if (add_pl) {
		m_listFileNames.AddTail(L"/add");
	}

	int bufflen = sizeof(DWORD);

	POSITION pos = m_listFileNames.GetHeadPosition();
	while(pos) {
		bufflen += (m_listFileNames.GetNext(pos).GetLength() + 1) * sizeof(TCHAR);
	}

	CAutoVectorPtr<BYTE> buff;
	if(!buff.Allocate(bufflen)) {
		return;
	}

	BYTE* p = buff;

	*(DWORD*)p = (DWORD)m_listFileNames.GetCount(); 
	p += sizeof(DWORD);

	pos = m_listFileNames.GetHeadPosition();
	while(pos) {
		CString s = m_listFileNames.GetNext(pos); 
		int len = (s.GetLength() + 1) * sizeof(TCHAR);
		memcpy(p, s, len);
		p += len;
	}

	if (HWND hWnd = ::FindWindow(MPC_WND_CLASS_NAME, NULL)) {
		COPYDATASTRUCT cds;
		cds.dwData = 0x6ABE51;
		cds.cbData = bufflen;
		cds.lpData = (void*)(BYTE*)buff;
		SendMessage(hWnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
	} else {
		CRegKey key;
		TCHAR path_buff[MAX_PATH];
		memset(path_buff, 0, sizeof(path_buff));
		ULONG len = sizeof(path_buff);
		CString mpc_path;
		
		if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, L"Software\\MPC-BE\\ShellExt")) {
			if (ERROR_SUCCESS == key.QueryStringValue(L"MpcPath", path_buff, &len) && ::PathFileExists(path_buff)) {
				mpc_path = path_buff;
			}
			key.Close();
		}

		if (mpc_path.Trim().IsEmpty()) {
			if (ERROR_SUCCESS == key.Open(HKEY_LOCAL_MACHINE, L"Software\\MPC-BE")) {
				memset(path_buff, 0, sizeof(path_buff));
				len = sizeof(path_buff);
				if (ERROR_SUCCESS == key.QueryStringValue(L"ExePath", path_buff, &len) && ::PathFileExists(path_buff)) {
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
				if (ERROR_SUCCESS == key.QueryStringValue(L"ExePath", path_buff, &len) && ::PathFileExists(path_buff)) {
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
				HWND hWnd = ::FindWindow(MPC_WND_CLASS_NAME, NULL);
				while (!hWnd && (wait_count++ < 200)) {
					Sleep(100);
					hWnd = ::FindWindow(MPC_WND_CLASS_NAME, NULL);
				}

				if (hWnd && (wait_count<200)) {
					COPYDATASTRUCT cds;
					cds.dwData = 0x6ABE51;
					cds.cbData = bufflen;
					cds.lpData = (void*)(BYTE*)buff;
					SendMessage(hWnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
				} 
			}
		}
	}
	return;
}

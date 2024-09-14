/*
 * (C) 2012-2024 see Authors.txt
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
#include <shlwapi.h>
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
	HBITMAP RetBmp = nullptr;
	if (hBmp) {
		HDC BufferDC = CreateCompatibleDC(nullptr);	// DC for Source Bitmap
		if (BufferDC) {

			HGDIOBJ PreviousBufferObject = SelectObject(BufferDC, hBmp);
			// here BufferDC contains the bitmap

			HDC DirectDC = CreateCompatibleDC(nullptr); // DC for working
			if (DirectDC) {
				// Get bitmap size

				BITMAP bm = {};
				GetObjectW(hBmp, sizeof(bm), &bm);

				const auto width  = std::max<int>(::GetSystemMetrics(SM_CXMENUCHECK), bm.bmWidth);
				const auto height = std::max<int>(::GetSystemMetrics(SM_CYMENUCHECK), bm.bmHeight);

				// create a BITMAPINFO for the CreateDIBSection
				BITMAPINFO RGB32BitsBITMAPINFO = {};
				RGB32BitsBITMAPINFO.bmiHeader.biSize      = sizeof(BITMAPINFOHEADER);
				RGB32BitsBITMAPINFO.bmiHeader.biWidth     = width;
				RGB32BitsBITMAPINFO.bmiHeader.biHeight    = height;
				RGB32BitsBITMAPINFO.bmiHeader.biPlanes    = 1;
				RGB32BitsBITMAPINFO.bmiHeader.biBitCount  = 32;
				RGB32BitsBITMAPINFO.bmiHeader.biSizeImage = width * height * 4;

				// pointer used for direct Bitmap pixels access
				UINT* ptPixels;
				HBITMAP DirectBitmap = CreateDIBSection(DirectDC,
														&RGB32BitsBITMAPINFO,
														DIB_RGB_COLORS,
														(void**)&ptPixels,
														nullptr, 0);
				if (DirectBitmap) {
					// here DirectBitmap!=NULL so ptPixels!=NULL no need to test
					HGDIOBJ PreviousObject = SelectObject(DirectDC, DirectBitmap);
					if (width > bm.bmWidth && height > bm.bmHeight) {
						SetStretchBltMode(DirectDC, STRETCH_HALFTONE);
						StretchBlt(DirectDC, 0, 0, width, height, BufferDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCPAINT);
					} else {
						BitBlt(DirectDC, 0, 0, bm.bmWidth, bm.bmHeight, BufferDC, 0, 0, SRCCOPY);
					}

					// Don't delete the result of SelectObject because it's
					// our modified bitmap (DirectBitmap)

					SelectObject(DirectDC, PreviousObject);

					// finish
					RetBmp = DirectBitmap;
				}
				// clean up
				DeleteDC(DirectDC);
			}
			SelectObject(BufferDC, PreviousBufferObject);

			// BufferDC is now useless
			DeleteDC(BufferDC);
		}
	}

	return RetBmp;
}

CMPCBEContextMenu::CMPCBEContextMenu()
	: m_hPlayBmp(TransparentBitmap(LoadBitmapW(_AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCEW(IDB_MPCBEBMP_PLAY))))
	, m_hAddBmp(TransparentBitmap(LoadBitmapW(_AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCEW(IDB_MPCBEBMP_ADD))))
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

static HRESULT DragFiles(LPDATAOBJECT lpdobj, std::vector<CStringW>& fileNames)
{
	fileNames.clear();

	// No data object
	if (!lpdobj) {
		return E_INVALIDARG;
	}

	CComPtr<IShellItemArray> psia;
	auto hr = SHCreateShellItemArrayFromDataObject(lpdobj, IID_PPV_ARGS(&psia));
	if (SUCCEEDED(hr)) {
		CComPtr<IEnumShellItems> pesi;
		hr = psia->EnumItems(&pesi);
		if (SUCCEEDED(hr)) {
			CComPtr<IShellItem> psi;
			while (pesi->Next(1, &psi, nullptr) == S_OK) {
				LPWSTR pszName = nullptr;
				hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszName);
				if (hr == S_OK) {
					CStringW fn(pszName);
					CoTaskMemFree(pszName);

					SFGAOF sfgaoAttribs;
					hr = psi->GetAttributes(SFGAO_FOLDER, &sfgaoAttribs);
					if (hr == S_OK) {
						if (fn[fn.GetLength() - 1] != L'\\') {
							fn += L"\\";
						}
					}

					fileNames.emplace_back(fn);
				}

				psi.Release();
			}

			if (fileNames.size() > 1) {
				// sort list by path
				std::sort(fileNames.begin(), fileNames.end(), [](const CStringW& a, const CStringW& b) {
					return StrCmpLogicalW(a, b) < 0;
				});
			}
		}
	}

	return fileNames.empty() ? E_INVALIDARG : S_OK;
}

// IShellExtInit

STDMETHODIMP CMPCBEContextMenu::Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, HKEY hkeyProgID)
{
	return DragFiles(lpdobj, m_fileNames);
}

// IContextMenu

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

STDMETHODIMP CMPCBEContextMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
	// If the flags include CMF_DEFAULTONLY then we shouldn't do anything.
	if ((uFlags & CMF_DEFAULTONLY)
			|| m_fileNames.size() == 0) {
		return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
	}

	CStringW PLAY_MPC  = (GetUserDefaultUILanguage() == 1049) ? PLAY_MPC_RU  : PLAY_MPC_EN;
	CStringW ADDTO_MPC = (GetUserDefaultUILanguage() == 1049) ? ADDTO_MPC_RU : ADDTO_MPC_EN;

	CRegKey key;
	auto ret = key.Open(HKEY_CURRENT_USER, shellExtKeyName, KEY_READ);
	if (ERROR_SUCCESS != ret) {
		ret = key.Open(HKEY_LOCAL_MACHINE, shellExtKeyName, KEY_READ);
	}
	if (ERROR_SUCCESS == ret) {
		WCHAR path_buff[MAX_PATH] = { 0 };
		ULONG len = (ULONG)std::size(path_buff);

		if (ERROR_SUCCESS == key.QueryStringValue(L"Play", path_buff, &len)) {
			PLAY_MPC = path_buff;
		}

		len = (ULONG)std::size(path_buff);
		ZeroMemory(path_buff, len);
		if (ERROR_SUCCESS == key.QueryStringValue(L"Add", path_buff, &len)) {
			ADDTO_MPC = path_buff;
		}

		key.Close();
	}

	::InsertMenuW(hmenu, indexMenu++, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);

	::InsertMenuW(hmenu, indexMenu, MF_STRING | MF_BYPOSITION, idCmdFirst + ID_MPCBE_PLAY, PLAY_MPC);
	if (NULL != m_hPlayBmp) {
		::SetMenuItemBitmaps(hmenu, indexMenu, MF_BYPOSITION, m_hPlayBmp, NULL);
	}
	indexMenu++;

	::InsertMenuW(hmenu, indexMenu, MF_STRING | MF_BYPOSITION, idCmdFirst + ID_MPCBE_PLAY + 1, ADDTO_MPC);
	if (NULL != m_hAddBmp) {
		::SetMenuItemBitmaps(hmenu, indexMenu, MF_BYPOSITION, m_hAddBmp, NULL);
	}
	indexMenu++;

	::InsertMenuW(hmenu, indexMenu++, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);

	return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, (ID_MPCBE_PLAY + 2));
}

// IDropTarget

STDMETHODIMP CMPCBEContextMenu::DragEnter(LPDATAOBJECT lpdobj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
	auto hr = DragFiles(lpdobj, m_fileNames);
	if (hr == S_OK) {
		*pdwEffect = DROPEFFECT_COPY;
	} else {
		*pdwEffect = DROPEFFECT_NONE;
	}

	return hr;
}

STDMETHODIMP CMPCBEContextMenu::Drop(LPDATAOBJECT lpdobj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
	SendData(false, true, ::MonitorFromWindow(::GetForegroundWindow(), MONITOR_DEFAULTTONEAREST));

	*pdwEffect = DROPEFFECT_COPY;
	return S_OK;
}

static CStringW GetMPCPath()
{
	CRegKey key;
	WCHAR buff[MAX_PATH] = {};
	ULONG len = (ULONG)std::size(buff);
	CStringW mpcPath;

	auto ret = key.Open(HKEY_CURRENT_USER, shellExtKeyName, KEY_READ);
	if (ERROR_SUCCESS != ret) {
		ret = key.Open(HKEY_LOCAL_MACHINE, shellExtKeyName, KEY_READ);
	}
	if (ERROR_SUCCESS == ret) {
		if (ERROR_SUCCESS == key.QueryStringValue(L"MpcPath", buff, &len) && ::PathFileExistsW(buff)) {
			mpcPath = buff; mpcPath.Trim();
		}
		key.Close();
	}

	if (mpcPath.IsEmpty()) {
		if (ERROR_SUCCESS == key.Open(HKEY_LOCAL_MACHINE, L"Software\\MPC-BE", KEY_READ)) {
			len = (ULONG)std::size(buff);
			ZeroMemory(buff, len);
			if (ERROR_SUCCESS == key.QueryStringValue(L"ExePath", buff, &len) && ::PathFileExistsW(buff)) {
				mpcPath = buff; mpcPath.Trim();
			}
			key.Close();
		}
	}

#ifdef _WIN64
	if (mpcPath.IsEmpty()) {
		if (ERROR_SUCCESS == key.Open(HKEY_LOCAL_MACHINE, L"Software\\Wow6432Node\\MPC-BE", KEY_READ)) {
			len = (ULONG)std::size(buff);
			ZeroMemory(buff, len);
			if (ERROR_SUCCESS == key.QueryStringValue(L"ExePath", buff, &len) && ::PathFileExistsW(buff)) {
				mpcPath = buff; mpcPath.Trim();
			}
			key.Close();
		}
	}
#endif

	return mpcPath;
}

static BOOL Execute(LPCWSTR lpszCommand, LPCWSTR lpszParameters, HMONITOR hMonitor)
{
	SHELLEXECUTEINFOW ShExecInfo = { sizeof(SHELLEXECUTEINFOW) };
	ShExecInfo.fMask = hMonitor ? SEE_MASK_HMONITOR : 0;
	ShExecInfo.lpFile = lpszCommand;
	ShExecInfo.lpParameters = lpszParameters;
	ShExecInfo.hMonitor = hMonitor;
	ShExecInfo.nShow = SW_SHOWDEFAULT;

	return ShellExecuteExW(&ShExecInfo);
}

void CMPCBEContextMenu::SendData(const bool bAddPlaylist, const bool bCheckMultipleInstances/* = false*/, HMONITOR hMonitor/* = nullptr*/)
{
	bool bMultipleInstances = false;
	if (bCheckMultipleInstances) {
		CRegKey key;
		auto ret = key.Open(HKEY_CURRENT_USER, shellExtKeyName, KEY_READ);
		if (ERROR_SUCCESS != ret) {
			ret = key.Open(HKEY_LOCAL_MACHINE, shellExtKeyName, KEY_READ);
		}
		if (ERROR_SUCCESS == ret) {
			DWORD dwValue = 0;
			if (ERROR_SUCCESS == key.QueryDWORDValue(L"MultipleInstances", dwValue)) {
				bMultipleInstances = (dwValue == 2);
			}
		}
	}

	if (bMultipleInstances) {
		CStringW mpcPath = GetMPCPath();
		for (auto item : m_fileNames) {
			item = L'\"' + item + L'\"';
			Execute(mpcPath.GetString(), item.GetString(), hMonitor);
		}

	} else {
		if (bAddPlaylist) {
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

		auto SendData = [&](HWND hWnd) {
			COPYDATASTRUCT cds;
			cds.dwData = 0x6ABE51;
			cds.cbData = (DWORD)bufflen;
			cds.lpData = buff.data();
			SendMessageW(hWnd, WM_COPYDATA, (WPARAM)nullptr, (LPARAM)&cds);
		};

		if (HWND hWnd = ::FindWindowW(MPC_WND_CLASS_NAME, nullptr)) {
			if (!bAddPlaylist) {
				DWORD dwProcessId = 0;
				if (GetWindowThreadProcessId(hWnd, &dwProcessId) && dwProcessId) {
					AllowSetForegroundWindow(dwProcessId);
				}

				if (IsIconic(hWnd)) {
					ShowWindow(hWnd, SW_RESTORE);
				}
			}

			SendData(hWnd);
		} else {
			const CStringW mpcPath = GetMPCPath();
			if (!mpcPath.IsEmpty() && Execute(mpcPath.GetString(), nullptr, hMonitor)) {
				Sleep(100);
				int wait_count = 0;
				HWND hWnd = ::FindWindowW(MPC_WND_CLASS_NAME, nullptr);
				while (!hWnd && (wait_count++ < 200)) {
					Sleep(100);
					hWnd = ::FindWindowW(MPC_WND_CLASS_NAME, nullptr);
				}

				if (hWnd) {
					SendData(hWnd);
				}
			}
		}
	}
}

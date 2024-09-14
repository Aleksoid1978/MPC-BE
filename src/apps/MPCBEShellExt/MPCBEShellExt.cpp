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
#include "MPCBEShellExt_i.h"
#include "dllmain.h"
#include <xutility>

// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow(void)
{
	return (_AtlModule.DllCanUnloadNow() == S_OK && _AtlModule.GetLockCount() == 0) ? S_OK : S_FALSE;
}

// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}

CStringW GetFileName(LPCWSTR Path)
{
	CStringW fileName = Path;
	::PathStripPathW(fileName.GetBuffer());
	fileName.ReleaseBuffer();
	return fileName;
}

static CStringW GetKeyName()
{
	CStringW KeyName;

	CRegKey key;
	auto ret = key.Open(HKEY_CURRENT_USER, shellExtKeyName, KEY_READ);
	if (ERROR_SUCCESS != ret) {
		ret = key.Open(HKEY_LOCAL_MACHINE, shellExtKeyName, KEY_READ);
	}
	if (ERROR_SUCCESS == ret) {
		WCHAR path_buff[MAX_PATH] = {};
		ULONG len = (ULONG)std::size(path_buff);
		if (ERROR_SUCCESS == key.QueryStringValue(L"MpcPath", path_buff, &len) && ::PathFileExistsW(path_buff)) {
			KeyName = GetFileName(path_buff);
			KeyName.Truncate(KeyName.GetLength() - 4);
		}
		key.Close();
	}

	return KeyName;
}

static BOOL GetKeyValue(LPCWSTR value)
{
	BOOL bValue = FALSE;

	CRegKey key;
	auto ret = key.Open(HKEY_CURRENT_USER, shellExtKeyName, KEY_READ);
	if (ERROR_SUCCESS != ret) {
		ret = key.Open(HKEY_LOCAL_MACHINE, shellExtKeyName, KEY_READ);
	}
	if (ERROR_SUCCESS == ret) {
		DWORD dwValue = 0;
		if (ERROR_SUCCESS == key.QueryDWORDValue(value, dwValue)) {
			bValue = !!dwValue;
		}
		key.Close();
	}

	return bValue;
}

// DllRegisterServer - Adds entries to the system registry
#define	IS_KEY_LEN 256
STDAPI DllRegisterServer(void)
{
	OLECHAR strWideCLSID[50] = {};
	HRESULT hr = _AtlModule.DllRegisterServer();

	if (SUCCEEDED(hr)) {
		CStringW KeyName = GetKeyName();
		if (KeyName.IsEmpty()) {
			return E_FAIL;
		}

		hr = E_FAIL;
		if (::StringFromGUID2(CLSID_MPCBEContextMenu, strWideCLSID, 50) > 0) {
			CRegKey key;
			if (GetKeyValue(L"ShowDir")) {
				key.SetValue(HKEY_CLASSES_ROOT, L"directory\\shellex\\ContextMenuHandlers\\MPCBEShellExt\\", strWideCLSID);
			}

			const auto bShowFiles = GetKeyValue(L"ShowFiles");
			CRegKey reg;
			if (reg.Open(HKEY_CLASSES_ROOT, nullptr, KEY_READ) == ERROR_SUCCESS) {
				DWORD dwIndex = 0;
				DWORD cbName = IS_KEY_LEN;
				WCHAR szSubKeyName[IS_KEY_LEN] = {};
				LONG lRet;

				while ((lRet = reg.EnumKey(dwIndex, szSubKeyName, &cbName)) != ERROR_NO_MORE_ITEMS) {
					if (lRet == ERROR_SUCCESS) {
						CStringW key_name = szSubKeyName;
						if (!key_name.Find(KeyName)) {
							if (bShowFiles) {
								key.SetValue(HKEY_CLASSES_ROOT, key_name + L"\\shellex\\ContextMenuHandlers\\MPCBEShellExt\\", strWideCLSID);
							}

							if (ERROR_SUCCESS == key.Create(HKEY_CLASSES_ROOT, key_name + L"\\shell\\open\\DropTarget")) {
								key.SetStringValue(L"CLSID", strWideCLSID);
							}
						}
					}
					dwIndex++;
					cbName = IS_KEY_LEN;
				}
				reg.Close();
			}
		}
	}

	return hr;
}

// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void)
{
	HRESULT hr = _AtlModule.DllUnregisterServer();

	if (SUCCEEDED(hr)) {
		CRegKey key;

		if (key.Open(HKEY_CLASSES_ROOT, L"directory\\shellex\\ContextMenuHandlers\\") == ERROR_SUCCESS) {
			key.DeleteSubKey(L"MPCBEShellExt");
		}

		CStringW KeyName = GetKeyName();
		if (KeyName.IsEmpty()) {
			return hr;
		}

		CRegKey reg;
		if (reg.Open(HKEY_CLASSES_ROOT, nullptr, KEY_READ) == ERROR_SUCCESS) {
			DWORD dwIndex = 0;
			DWORD cbName = IS_KEY_LEN;
			WCHAR szSubKeyName[IS_KEY_LEN] = { 0 };
			LONG lRet;

			while ((lRet = reg.EnumKey(dwIndex, szSubKeyName, &cbName)) != ERROR_NO_MORE_ITEMS) {
				if (lRet == ERROR_SUCCESS) {
					CStringW key_name = szSubKeyName;
					if (!key_name.Find(KeyName)) {
						if (key.Open(HKEY_CLASSES_ROOT, key_name) == ERROR_SUCCESS) {
							key.RecurseDeleteKey(L"shellex");
						}

						key_name.Append(L"\\shell\\open");
						if (key.Open(HKEY_CLASSES_ROOT, key_name) == ERROR_SUCCESS) {
							key.RecurseDeleteKey(L"DropTarget");
						}
					}
				}
				dwIndex++;
				cbName = IS_KEY_LEN;
			}
			reg.Close();
		}
	}

	return hr;
}

// DllInstall - Adds/Removes entries to the system registry per user
//              per machine.
STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
	HRESULT hr = E_FAIL;
	static const wchar_t szUserSwitch[] = L"user";

	if (pszCmdLine != nullptr) {
		if (_wcsnicmp(pszCmdLine, szUserSwitch, _countof(szUserSwitch)) == 0) {
			AtlSetPerUserRegistration(true);
		}
	}

	if (bInstall) {
		hr = DllRegisterServer();
		if (FAILED(hr)) {
			DllUnregisterServer();
		}
	} else {
		hr = DllUnregisterServer();
	}

	return hr;
}

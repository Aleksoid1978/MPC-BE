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
#include "MPCBEShellExt_i.h"
#include "dllmain.h"
#include "ConfigDlg.h"

// config & dialog
extern "C" __declspec(dllexport) HRESULT DllConfig(LPCTSTR lpszPath)
{
	if (wcslen(lpszPath)) {
		if (::PathFileExists(lpszPath)) {
			CRegKey key;
			if (ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, L"Software\\MPC-BE\\ShellExt")) {
				key.SetStringValue(L"MpcPath", lpszPath);
				key.Close();
				return S_OK;
			}
		}
		return E_FAIL;
	}

	// ѕровер€ем - надо ли показывать диалог выбора
	CRegKey key;
	TCHAR path_buff[MAX_PATH];
	memset(path_buff, 0, sizeof(path_buff));
	ULONG len = sizeof(path_buff);
	unsigned count = 0;

	if (ERROR_SUCCESS == key.Open(HKEY_LOCAL_MACHINE, L"Software\\MPC-BE")) {
		if (ERROR_SUCCESS == key.QueryStringValue(L"ExePath", path_buff, &len) && ::PathFileExists(path_buff)) {
			count++;
		}
		key.Close();
	}
#ifdef _WIN64
	// x86 application on x64 system
	if (ERROR_SUCCESS == key.Open(HKEY_LOCAL_MACHINE, L"Software\\Wow6432Node\\MPC-BE")) {
		len = sizeof(path_buff);
		memset(path_buff, 0, sizeof(path_buff));
		if (ERROR_SUCCESS == key.QueryStringValue(L"ExePath", path_buff, &len) && ::PathFileExists(path_buff)) {
			count++;
		}
		key.Close();
	}
#endif
	//

	if (count == 2) {
		CConfigDlg dlg;
		dlg.DoModal();
		return S_OK;
	} else {
		memset(path_buff, 0, sizeof(path_buff));
		len = sizeof(path_buff);

		if (ERROR_SUCCESS == key.Open(HKEY_LOCAL_MACHINE, L"Software\\MPC-BE")) {
			if (ERROR_SUCCESS == key.QueryStringValue(L"ExePath", path_buff, &len) && ::PathFileExists(path_buff)) {
				CString path(path_buff);
				key.Close();

				if (ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, L"Software\\MPC-BE\\ShellExt")) {
					key.SetStringValue(L"MpcPath", path);
					key.Close();
					return S_OK;
				}
			}
		}
	}

	return E_FAIL;
}

// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return (AfxDllCanUnloadNow() == S_OK && _AtlModule.GetLockCount() == 0) ? S_OK : S_FALSE;
}

// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}

static CString GetFileOnly(LPCTSTR Path)
{
	CString cs(Path);
	::PathStripPath(cs.GetBuffer(0));
	cs.ReleaseBuffer(-1);
	return cs;
}

static CString GetKeyName()
{
	CString KeyName;
	TCHAR path_buff[MAX_PATH];
	memset(path_buff, 0, sizeof(path_buff));
	ULONG len = sizeof(path_buff);

	CRegKey key;
	if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, L"Software\\MPC-BE\\ShellExt")) {
		if (ERROR_SUCCESS == key.QueryStringValue(L"MpcPath", path_buff, &len) && ::PathFileExists(path_buff)) {
			KeyName = GetFileOnly(path_buff);
			KeyName.Truncate(KeyName.GetLength() - 4);
		}
		key.Close();
	}

	return KeyName;
}

static BOOL GetKeyValue(CString value)
{
	BOOL bValue = TRUE;
	CRegKey key;
	if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, L"Software\\MPC-BE\\ShellExt")) {
		DWORD dw = 0;
		if (ERROR_SUCCESS == key.QueryDWORDValue(value, dw)) {
			bValue = !!dw;
		}
		key.Close();
	}

	return bValue;
}
// DllRegisterServer - Adds entries to the system registry
#define	IS_KEY_LEN 256
STDAPI DllRegisterServer(void)
{
	OLECHAR	strWideCLSID[50] = { 0 };
	HRESULT hr = _AtlModule.DllRegisterServer();

	if (SUCCEEDED(hr)) {
		//DllConfig(NULL);

		CString KeyName = GetKeyName();
		if (KeyName.IsEmpty()) {
			return hr;
		}

		if (::StringFromGUID2(CLSID_MPCBEContextMenu, strWideCLSID, 50) > 0) {
			CRegKey key;
			if (GetKeyValue(L"ShowDir")) {
				key.SetValue(HKEY_CLASSES_ROOT, L"directory\\shellex\\ContextMenuHandlers\\MPCBEShellExt\\", strWideCLSID);
			}

			if (GetKeyValue(L"ShowFiles")) {
				CRegKey reg;
				if (reg.Open(HKEY_CLASSES_ROOT, NULL, KEY_READ) == ERROR_SUCCESS) {
					DWORD dwIndex = 0;
					DWORD cbName = IS_KEY_LEN;
					TCHAR szSubKeyName[IS_KEY_LEN] = { 0 };
					LONG lRet;

					while ((lRet = reg.EnumKey(dwIndex, szSubKeyName, &cbName)) != ERROR_NO_MORE_ITEMS) {
						if (lRet == ERROR_SUCCESS) {
							CString key_name = szSubKeyName;
							if (!key_name.Find(KeyName)) {
								key_name.Append(L"\\shellex\\ContextMenuHandlers\\MPCBEShellExt\\");
								key.SetValue(HKEY_CLASSES_ROOT, key_name, strWideCLSID);
							}
						}
						dwIndex++;
						cbName = IS_KEY_LEN;
					}
					reg.Close();
				}
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

		CString KeyName = GetKeyName();
		if (KeyName.IsEmpty()) {
			return hr;
		}

		CRegKey reg;
		if (reg.Open(HKEY_CLASSES_ROOT, NULL, KEY_READ) == ERROR_SUCCESS) {
			DWORD dwIndex = 0;
			DWORD cbName = IS_KEY_LEN;
			TCHAR szSubKeyName[IS_KEY_LEN] = { 0 };
			LONG lRet;

			while ((lRet = reg.EnumKey(dwIndex, szSubKeyName, &cbName)) != ERROR_NO_MORE_ITEMS) {
				if (lRet == ERROR_SUCCESS) {
					CString key_name = szSubKeyName;
					if (!key_name.Find(KeyName)) {
						if (key.Open(HKEY_CLASSES_ROOT, key_name) == ERROR_SUCCESS) {
							key.RecurseDeleteKey(L"shellex");
						}
						/*
						key_name.Append(L"\\shellex\\ContextMenuHandlers\\");

						if (key.Open(HKEY_CLASSES_ROOT, key_name) == ERROR_SUCCESS) {
							key.DeleteValue(NULL);
							key.DeleteSubKey(L"MPCBEShellExt");
						}
						*/
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

    if (pszCmdLine != NULL) {
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

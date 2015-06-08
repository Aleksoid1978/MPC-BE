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

#include "stdafx.h"
#include <Shlobj.h>
#include <dx/d3dx9.h>
#include "WinAPIUtils.h"
#include "SysVersion.h"

BOOL IsCompositionEnabled()
{
	// check Composition is Enabled
	BOOL bCompositionEnabled = false;

	if (IsWinVistaOrLater()) {
		HRESULT (__stdcall * pDwmIsCompositionEnabled)(__out BOOL* pfEnabled);
		pDwmIsCompositionEnabled = NULL;
		HMODULE hDWMAPI = LoadLibrary(L"dwmapi.dll");
		if (hDWMAPI) {
			(FARPROC &)pDwmIsCompositionEnabled = GetProcAddress(hDWMAPI, "DwmIsCompositionEnabled");
			if (pDwmIsCompositionEnabled) {
				pDwmIsCompositionEnabled(&bCompositionEnabled);
			}
			FreeLibrary(hDWMAPI);
		}
	}

	return bCompositionEnabled;
}

bool SetPrivilege(LPCTSTR privilege, bool bEnable)
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;

	SetThreadExecutionState (ES_CONTINUOUS);

	// Get a token for this process.
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		return false;
	}

	// Get the LUID for the privilege.
	LookupPrivilegeValue(NULL, privilege, &tkp.Privileges[0].Luid);

	tkp.PrivilegeCount = 1;  // one privilege to set
	tkp.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;

	// Set the privilege for this process.
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

	return (GetLastError() == ERROR_SUCCESS);
}

CString GetHiveName(HKEY hive)
{
	switch ((ULONG_PTR)hive) {
		case (ULONG_PTR)HKEY_CLASSES_ROOT:
			return _T("HKEY_CLASSES_ROOT");
		case (ULONG_PTR)HKEY_CURRENT_USER:
			return _T("HKEY_CURRENT_USER");
		case (ULONG_PTR)HKEY_LOCAL_MACHINE:
			return _T("HKEY_LOCAL_MACHINE");
		case (ULONG_PTR)HKEY_USERS:
			return _T("HKEY_USERS");
		case (ULONG_PTR)HKEY_PERFORMANCE_DATA:
			return _T("HKEY_PERFORMANCE_DATA");
		case (ULONG_PTR)HKEY_CURRENT_CONFIG:
			return _T("HKEY_CURRENT_CONFIG");
		case (ULONG_PTR)HKEY_DYN_DATA:
			return _T("HKEY_DYN_DATA");
		case (ULONG_PTR)HKEY_PERFORMANCE_TEXT:
			return _T("HKEY_PERFORMANCE_TEXT");
		case (ULONG_PTR)HKEY_PERFORMANCE_NLSTEXT:
			return _T("HKEY_PERFORMANCE_NLSTEXT");
		default:
			return _T("");
	}
}

bool ExportRegistryKey(CStdioFile& file, HKEY hKeyRoot, CString keyName)
{
	HKEY hKey = NULL;
	if (RegOpenKeyEx(hKeyRoot, keyName, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
		return false;
	}

	DWORD subKeysCount = 0, maxSubKeyLen = 0;
	DWORD valuesCount = 0, maxValueNameLen = 0, maxValueDataLen = 0;
	if (RegQueryInfoKey(hKey, NULL, NULL, NULL, &subKeysCount, &maxSubKeyLen, NULL, &valuesCount, &maxValueNameLen, &maxValueDataLen, NULL, NULL) != ERROR_SUCCESS) {
		return false;
	}
	maxSubKeyLen += 1;
	maxValueNameLen += 1;

	CString buffer;

	buffer.Format(_T("[%s\\%s]\n"), GetHiveName(hKeyRoot), keyName);
	file.WriteString(buffer);

	CString valueName;
	DWORD valueNameLen, valueDataLen, type;
	BYTE* data = DNew BYTE[maxValueDataLen];

	for (DWORD indexValue=0; indexValue < valuesCount; indexValue++) {
		valueNameLen = maxValueNameLen;
		valueDataLen = maxValueDataLen;

		if (RegEnumValue(hKey, indexValue, valueName.GetBuffer(maxValueNameLen), &valueNameLen, NULL, &type, data, &valueDataLen) != ERROR_SUCCESS) {
			return false;
		}

		switch (type) {
			case REG_SZ:
			{
				CString str((TCHAR*)data);
				str.Replace(_T("\\"), _T("\\\\"));
				str.Replace(_T("\""), _T("\\\""));
				buffer.Format(_T("\"%s\"=\"%s\"\n"), valueName, str);
				file.WriteString(buffer);
			}
			break;
			case REG_BINARY:
				buffer.Format(_T("\"%s\"=hex:%02x"), valueName, data[0]);
				file.WriteString(buffer);
				for (DWORD i=1; i < valueDataLen; i++) {
					buffer.Format(_T(",%02x"), data[i]);
					file.WriteString(buffer);
				}
				file.WriteString(_T("\n"));
				break;
			case REG_DWORD:
				buffer.Format(_T("\"%s\"=dword:%08x\n"), valueName, *((DWORD*)data));
				file.WriteString(buffer);
				break;
			default:
			{
				CString msg;
				msg.Format(_T("The value \"%s\\%s\\%s\" has an unsupported type and has been ignored.\nPlease report this error to the developers."),
						   GetHiveName(hKeyRoot), keyName, valueName);
				AfxMessageBox(msg, MB_ICONERROR | MB_OK);
			}
			delete[] data;
			return false;
		}
	}

	delete[] data;

	file.WriteString(_T("\n"));

	CString subKeyName;
	DWORD subKeyLen;

	for (DWORD indexSubKey=0; indexSubKey < subKeysCount; indexSubKey++) {
		subKeyLen = maxSubKeyLen;

		if (RegEnumKeyEx(hKey, indexSubKey, subKeyName.GetBuffer(maxSubKeyLen), &subKeyLen, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) {
			return false;
		}

		buffer.Format(_T("%s\\%s"), keyName, subKeyName);

		if (!ExportRegistryKey(file, hKeyRoot, buffer)) {
			return false;
		}
	}

	RegCloseKey(hKey);

	return true;
}

UINT GetAdapter(IDirect3D9* pD3D, HWND hWnd)
{
	if (hWnd == NULL || pD3D == NULL) {
		return D3DADAPTER_DEFAULT;
	}

	HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
	if (hMonitor == NULL) {
		return D3DADAPTER_DEFAULT;
	}

	for (UINT adp = 0, num_adp = pD3D->GetAdapterCount(); adp < num_adp; ++adp) {
		HMONITOR hAdpMon = pD3D->GetAdapterMonitor(adp);
		if (hAdpMon == hMonitor) {
			return adp;
		}
	}

	return D3DADAPTER_DEFAULT;
}

int CALLBACK EnumFontFamExProc(ENUMLOGFONTEX* /*lpelfe*/, NEWTEXTMETRICEX* /*lpntme*/, int /*FontType*/, LPARAM lParam)
{
	LPARAM* l = (LPARAM*)lParam;
	*l = TRUE;
	return TRUE;
}

bool IsFontInstalled(LPCTSTR lpszFont)
{
	// Get the screen DC
	CDC dc;
	if (!dc.CreateCompatibleDC(NULL)) {
		return false;
	}

	LOGFONT lf = {0};
	// Any character set will do
	lf.lfCharSet = DEFAULT_CHARSET;
	// Set the facename to check for
	_tcscpy_s(lf.lfFaceName, lpszFont);
	LPARAM lParam = 0;
	// Enumerate fonts
	EnumFontFamiliesEx(dc.GetSafeHdc(), &lf, (FONTENUMPROC)EnumFontFamExProc, (LPARAM)&lParam, 0);

	return lParam ? true : false;
}

bool ExploreToFile(CString path)
{
	bool success = false;
	HRESULT res = CoInitialize(NULL);

	if (res == S_OK || res == S_FALSE) {
		PIDLIST_ABSOLUTE pidl;

		if (SHParseDisplayName(path, NULL, &pidl, 0, NULL) == S_OK) {
			success = SUCCEEDED(SHOpenFolderAndSelectItems(pidl, 0, NULL, 0));
			CoTaskMemFree(pidl);
		}

		CoUninitialize();
	}

	return success;
}

// retrieves the monitor EDID info
// thanks to JanWillem32 for this code
static UINT16 const gk_au16CodePage437ForEDIDLookup[256] = {
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,// first 32 control characters taken out, to prevent them from printing
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x0000,// control character DEL (0x7F) taken out
	0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7, 0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
	0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9, 0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,
	0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA, 0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
	0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
	0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F, 0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
	0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B, 0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
	0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4, 0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,
	0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248, 0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0
};

bool ReadDisplay(CString szDevice, CString* MonitorName, UINT16* MonitorHorRes, UINT16* MonitorVerRes)
{
	wchar_t szMonitorName[14] = { 0 };
	UINT16 nMonitorHorRes = 0;
	UINT16 nMonitorVerRes = 0;

	if (MonitorHorRes) {
		*MonitorHorRes = 0;
	}

	if (MonitorVerRes) {
		*MonitorVerRes = 0;
	}

	DISPLAY_DEVICE DisplayDevice;
	DisplayDevice.cb = sizeof(DISPLAY_DEVICEW);

	DWORD dwDevNum = 0;
	while (EnumDisplayDevices(NULL, dwDevNum++, &DisplayDevice, 0)) {

		if ((DisplayDevice.StateFlags & DISPLAY_DEVICE_ACTIVE)
			&& !(DisplayDevice.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)
			&& !_wcsicmp(DisplayDevice.DeviceName, szDevice)) {

			DWORD dwMonNum = 0;
			while (EnumDisplayDevices(szDevice, dwMonNum++, &DisplayDevice, 0)) {
				if (DisplayDevice.StateFlags & DISPLAY_DEVICE_ACTIVE) {
					size_t len = wcslen(DisplayDevice.DeviceID);
					wchar_t* szDeviceIDshort = DisplayDevice.DeviceID + len - 43;// fixed at 43 characters

					HKEY hKey0;
					static wchar_t const gk_szRegCcsEnumDisplay[] = L"SYSTEM\\CurrentControlSet\\Enum\\DISPLAY\\";
					LSTATUS ls = RegOpenKeyEx(HKEY_LOCAL_MACHINE, gk_szRegCcsEnumDisplay, 0, KEY_ENUMERATE_SUB_KEYS | KEY_READ, &hKey0);
					if (ls == ERROR_SUCCESS) {
						DWORD i = 0;
						for (;;) {// iterate over the child keys
							DWORD cbName = _countof(DisplayDevice.DeviceKey);
							ls = RegEnumKeyEx(hKey0, i, DisplayDevice.DeviceKey, &cbName, NULL, NULL, NULL, NULL);
							if (ls == ERROR_NO_MORE_ITEMS) {
								break;
							}

							if (ls == ERROR_SUCCESS) {
								static wchar_t DeviceName[MAX_PATH] = { 0 };
								memcpy(DeviceName, gk_szRegCcsEnumDisplay, sizeof(gk_szRegCcsEnumDisplay) - 2);// chop off null character
								memcpy(DeviceName + _countof(gk_szRegCcsEnumDisplay) - 1, DisplayDevice.DeviceKey, (cbName << 1) + 2);
								wchar_t* pEnd0 = DeviceName + _countof(gk_szRegCcsEnumDisplay) - 1 + cbName;
								HKEY hKey1;
								ls = RegOpenKeyEx(HKEY_LOCAL_MACHINE, DeviceName, 0, KEY_ENUMERATE_SUB_KEYS | KEY_READ, &hKey1);

								if (ls == ERROR_SUCCESS) {
									DWORD j = 0;
									for (;;) {// iterate over the grandchild keys
										cbName = _countof(DisplayDevice.DeviceKey);
										ls = RegEnumKeyEx(hKey1, j, DisplayDevice.DeviceKey, &cbName, NULL, NULL, NULL, NULL);
										if (ls == ERROR_NO_MORE_ITEMS) {
											break;
										}

										if (ls == ERROR_SUCCESS) {
											*pEnd0 = L'\\';
											memcpy(pEnd0 + 1, DisplayDevice.DeviceKey, (cbName << 1) + 2);
											wchar_t* pEnd1 = pEnd0 + 1 + cbName;

											HKEY hKey2;
											ls = RegOpenKeyEx(HKEY_LOCAL_MACHINE, DeviceName, 0, KEY_ENUMERATE_SUB_KEYS | KEY_READ, &hKey2);
											if (ls == ERROR_SUCCESS) {

												static wchar_t const szTDriverKeyN[] = L"Driver";
												cbName = sizeof(DisplayDevice.DeviceKey);// re-use it here
												ls = RegQueryValueEx(hKey2, szTDriverKeyN, NULL, NULL, (LPBYTE)DisplayDevice.DeviceKey, &cbName);
												if (ls == ERROR_SUCCESS) {
													if (!wcscmp(szDeviceIDshort, DisplayDevice.DeviceKey)) {
														static wchar_t const szTDevParKeyN[] = L"\\Device Parameters";
														memcpy(pEnd1, szTDevParKeyN, sizeof(szTDevParKeyN));
														static wchar_t const szkEDIDKeyN[] = L"EDID";
														cbName = sizeof(DisplayDevice.DeviceKey);// 256, perfectly suited to receive a copy of the 128 or 256 bytes of EDID data

														HKEY hKey3;
														ls = RegOpenKeyEx(HKEY_LOCAL_MACHINE, DeviceName, 0, KEY_ENUMERATE_SUB_KEYS | KEY_READ, &hKey3);
														if (ls == ERROR_SUCCESS) {

															ls = RegQueryValueEx(hKey3, szkEDIDKeyN, NULL, NULL, (LPBYTE)DisplayDevice.DeviceKey, &cbName);
															if ((ls == ERROR_SUCCESS) && (cbName > 127)) {
																UINT8* EDIDdata = (UINT8*)DisplayDevice.DeviceKey;
																// memo: bytes 25 to 34 contain the default chromaticity coordinates

																// pixel clock in 10 kHz units (0.01–655.35 MHz)
																UINT16 u16PixelClock = (UINT16)(EDIDdata + 54);
																if (u16PixelClock) {// if the descriptor for pixel clock is 0, the descriptor block is invalid
																	// horizontal active pixels
																	nMonitorHorRes = (UINT16(EDIDdata[58] & 0xF0) << 4) | EDIDdata[56];
																	// vertical active pixels
																	nMonitorVerRes = (UINT16(EDIDdata[61] & 0xF0) << 4) | EDIDdata[59];

																	// validate and identify extra descriptor blocks
																	// memo: descriptor block identifier 0xFB is used for additional white point data
																	ptrdiff_t k = 12;
																	if (!*(UINT16*)(EDIDdata + 72) && (EDIDdata[75] == 0xFC)) {// descriptor block 2, the first 16 bits must be zero, else the descriptor contains detailed timing data, identifier 0xFC is used for monitor name
																		do {
																			szMonitorName[k] = gk_au16CodePage437ForEDIDLookup[EDIDdata[77 + k]];
																		} while (--k >= 0);
																	}
																	else if (!*(UINT16*)(EDIDdata + 90) && (EDIDdata[93] == 0xFC)) {// descriptor block 3
																		do {
																			szMonitorName[k] = gk_au16CodePage437ForEDIDLookup[EDIDdata[95 + k]];
																		} while (--k >= 0);
																	}
																	else if (!*(UINT16*)(EDIDdata + 108) && (EDIDdata[111] == 0xFC)) {// descriptor block 4
																		do {
																			szMonitorName[k] = gk_au16CodePage437ForEDIDLookup[EDIDdata[113 + k]];
																		} while (--k >= 0);
																	}
																}
																RegCloseKey(hKey3);
																RegCloseKey(hKey2);
																RegCloseKey(hKey1);
																RegCloseKey(hKey0);

																if (wcslen(szMonitorName) && nMonitorHorRes && nMonitorVerRes) {
																	if (MonitorName) {
																		*MonitorName = szMonitorName;
																	}
																	if (MonitorHorRes) {
																		*MonitorHorRes = nMonitorHorRes;
																	}
																	if (MonitorVerRes) {
																		*MonitorVerRes = nMonitorVerRes;
																	}

																	return true;
																}
																return false;
															}
															RegCloseKey(hKey3);
														}
													}
												}
												RegCloseKey(hKey2);
											}
										}
										++j;
									}
									RegCloseKey(hKey1);
								}
							}
							++i;
						}
						RegCloseKey(hKey0);
					}
					break;
				}
			}
			break;
		}
    }

	return false;
}

BOOL CFileGetStatus(LPCTSTR lpszFileName, CFileStatus& status)
{
	try {
		return CFile::GetStatus(lpszFileName, status);
	} catch (CException* e) {
		// MFCBUG: E_INVALIDARG / "Parameter is incorrect" is thrown for certain cds (vs2003)
		// http://groups.google.co.uk/groups?hl=en&lr=&ie=UTF-8&threadm=OZuXYRzWDHA.536%40TK2MSFTNGP10.phx.gbl&rnum=1&prev=/groups%3Fhl%3Den%26lr%3D%26ie%3DISO-8859-1
		TRACE(_T("CFile::GetStatus has thrown an exception\n"));
		e->Delete();
		return false;
	}
}

BOOL IsUserAdmin()
/*++
Routine Description: This routine returns TRUE if the caller's
process is a member of the Administrators local group. Caller is NOT
expected to be impersonating anyone and is expected to be able to
open its own process and process token.
Arguments: None.
Return Value:
	TRUE - Caller has Administrators local group.
	FALSE - Caller does not have Administrators local group. --

from http://msdn.microsoft.com/en-us/library/windows/desktop/aa376389(v=vs.85).aspx
*/
{
	BOOL ret;
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	PSID AdministratorsGroup;
	ret = AllocateAndInitializeSid(
		  &NtAuthority,
		  2,
		  SECURITY_BUILTIN_DOMAIN_RID,
		  DOMAIN_ALIAS_RID_ADMINS,
		  0, 0, 0, 0, 0, 0,
		  &AdministratorsGroup);
	if (ret) {
		if (!CheckTokenMembership(NULL, AdministratorsGroup, &ret)) {
			ret = FALSE;
		}
		FreeSid(AdministratorsGroup);
	}

	return ret;
}

// from http://msdn.microsoft.com/ru-RU/library/windows/desktop/ms680582(v=vs.85).aspx
CString GetLastErrorMsg(LPTSTR lpszFunction, DWORD dw/* = GetLastError()*/)
{
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		//MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default system language
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Format the error message

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		L"Function '%s' failed with error %d: %s",
		lpszFunction, dw, lpMsgBuf);

	CString ret = (LPCTSTR)lpDisplayBuf;

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);

	return ret;
}
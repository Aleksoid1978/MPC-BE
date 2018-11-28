/*
 * (C) 2016-2018 see Authors.txt
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
#include "Misc.h"

bool SetPrivilege(LPCTSTR privilege, bool bEnable/* = true*/)
{
	SetThreadExecutionState(ES_CONTINUOUS);

	HANDLE hToken;
	// Get a token for this process.
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		return false;
	}

	TOKEN_PRIVILEGES tkp;
	// Get the LUID for the privilege.
	LookupPrivilegeValue(nullptr, privilege, &tkp.Privileges[0].Luid);

	tkp.PrivilegeCount = 1;  // one privilege to set
	tkp.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;

	// Set the privilege for this process.
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)nullptr, 0);

	return (GetLastError() == ERROR_SUCCESS);
}

bool ExploreToFile(CString path)
{
	bool success = false;
	HRESULT res = CoInitialize(nullptr);

	if (res == S_OK || res == S_FALSE) {
		PIDLIST_ABSOLUTE pidl;

		if (SHParseDisplayName(path, nullptr, &pidl, 0, nullptr) == S_OK) {
			success = SUCCEEDED(SHOpenFolderAndSelectItems(pidl, 0, nullptr, 0));
			CoTaskMemFree(pidl);
		}

		CoUninitialize();
	}

	return success;
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
		if (!CheckTokenMembership(nullptr, AdministratorsGroup, &ret)) {
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

	FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		dw,
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		//MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default system language
		(LPTSTR)&lpMsgBuf,
		0, nullptr);

	// Format the error message

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlenW((LPCWSTR)lpMsgBuf) + lstrlen((LPCWSTR)lpszFunction) + 40) * sizeof(WCHAR));
	StringCchPrintfW((LPWSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(WCHAR),
		L"Function '%s' failed with error %d: %s",
		lpszFunction, dw, lpMsgBuf);

	CString ret = (LPCWSTR)lpDisplayBuf;

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);

	return ret;
}

HICON LoadIcon(CString fn, bool fSmall)
{
	if (fn.IsEmpty()) {
		return nullptr;
	}

	CString ext = fn.Left(fn.Find(L"://")+1).TrimRight(':');
	if (ext.IsEmpty() || !ext.CompareNoCase(L"file")) {
		ext = L"." + fn.Mid(fn.ReverseFind('.')+1);
	}

	CSize size(fSmall?16:32, fSmall?16:32);

	if (!ext.CompareNoCase(L".ifo")) {
		if (HICON hIcon = (HICON)LoadImageW(AfxGetInstanceHandle(), MAKEINTRESOURCEW(IDI_DVD), IMAGE_ICON, size.cx, size.cy, 0)) {
			return hIcon;
		}
	}

	if (!ext.CompareNoCase(L".cda")) {
		if (HICON hIcon = (HICON)LoadImageW(AfxGetInstanceHandle(), MAKEINTRESOURCEW(IDI_AUDIOCD), IMAGE_ICON, size.cx, size.cy, 0)) {
			return hIcon;
		}
	}

	if ((CString(fn).MakeLower().Find(L"://")) >= 0) {
		if (HICON hIcon = (HICON)LoadImageW(AfxGetInstanceHandle(), MAKEINTRESOURCEW(IDR_MAINFRAME), IMAGE_ICON, size.cx, size.cy, 0)) {
			return hIcon;
		}

		return nullptr;
	}

	WCHAR buff[MAX_PATH];
	lstrcpyW(buff, fn.GetBuffer());

	SHFILEINFO sfi;
	ZeroMemory(&sfi, sizeof(sfi));

	if (SUCCEEDED(SHGetFileInfoW(buff, 0, &sfi, sizeof(sfi), (fSmall ? SHGFI_SMALLICON : SHGFI_LARGEICON) |SHGFI_ICON)) && sfi.hIcon) {
		return sfi.hIcon;
	}

	ULONG len;
	HICON hIcon = nullptr;

	do {
		CRegKey key;

		CString RegPathAssociated;
		RegPathAssociated.Format(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%ws\\UserChoice", ext);

		if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, RegPathAssociated, KEY_READ)) {
			len = _countof(buff);
			memset(buff, 0, sizeof(buff));

			CString ext_Associated = ext;
			if (ERROR_SUCCESS == key.QueryStringValue(L"Progid", buff, &len) && !(ext_Associated = buff).Trim().IsEmpty()) {
				ext = ext_Associated;
			}
		}

		if (ERROR_SUCCESS != key.Open(HKEY_CLASSES_ROOT, ext + L"\\DefaultIcon", KEY_READ)) {
			if (ERROR_SUCCESS != key.Open(HKEY_CLASSES_ROOT, ext, KEY_READ)) {
				break;
			}

			len = _countof(buff);
			memset(buff, 0, sizeof(buff));
			if (ERROR_SUCCESS != key.QueryStringValue(nullptr, buff, &len) || (ext = buff).Trim().IsEmpty()) {
				break;
			}

			if (ERROR_SUCCESS != key.Open(HKEY_CLASSES_ROOT, ext + L"\\DefaultIcon", KEY_READ)) {
				break;
			}
		}

		CString icon;

		len = _countof(buff);
		memset(buff, 0, sizeof(buff));
		if (ERROR_SUCCESS != key.QueryStringValue(nullptr, buff, &len) || (icon = buff).Trim().IsEmpty()) {
			break;
		}

		int i = icon.ReverseFind(',');
		if (i < 0) {
			break;
		}

		int id = 0;
		if (swscanf_s(icon.Mid(i+1), L"%d", &id) != 1) {
			break;
		}

		icon = icon.Left(i);
		icon.Replace(L"\"", L"");

		hIcon = nullptr;
		UINT cnt = fSmall
				   ? ExtractIconExW(icon, id, nullptr, &hIcon, 1)
				   : ExtractIconExW(icon, id, &hIcon, nullptr, 1);
		UNREFERENCED_PARAMETER(cnt);
		if (hIcon) {
			return hIcon;
		}
	} while (0);

	return (HICON)LoadImageW(AfxGetInstanceHandle(), MAKEINTRESOURCEW(IDI_UNKNOWN), IMAGE_ICON, size.cx, size.cy, 0);
}

bool LoadType(CString fn, CString& type)
{
	bool found = false;

	if (!fn.IsEmpty()) {
		CString ext = fn.Left(fn.Find(L"://")+1).TrimRight(':');
		if (ext.IsEmpty() || !ext.CompareNoCase(L"file")) {
			ext = L"." + fn.Mid(fn.ReverseFind('.')+1);
		}

		// Try MPC-BE's internal formats list
		CMediaFormatCategory* mfc = AfxGetAppSettings().m_Formats.FindMediaByExt(ext);

		if (mfc != nullptr) {
			found = true;
			type = mfc->GetDescription();
		} else { // Fallback to registry
			CRegKey key;

			CString tmp;
			CString mplayerc_ext = L"mplayerc" + ext;

			if (ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, mplayerc_ext)) {
				tmp = mplayerc_ext;
			}

			if (!tmp.IsEmpty() || ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, ext)) {
				found = true;

				if (tmp.IsEmpty()) {
					tmp = ext;
				}

				WCHAR buff[256] = { 0 };
				ULONG len;

				while (ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, tmp)) {
					len = _countof(buff);
					memset(buff, 0, sizeof(buff));

					if (ERROR_SUCCESS != key.QueryStringValue(nullptr, buff, &len)) {
						break;
					}

					CString str(buff);
					str.Trim();

					if (str.IsEmpty() || str == tmp) {
						break;
					}

					tmp = str;
				}

				type = tmp;
			}
		}
	}

	return found;
}

bool LoadResource(UINT resid, CStringA& str, LPCTSTR restype)
{
	str.Empty();
	HRSRC hrsrc = FindResourceW(AfxGetApp()->m_hInstance, MAKEINTRESOURCEW(resid), restype);
	if (!hrsrc) {
		return false;
	}
	HGLOBAL hGlobal = LoadResource(AfxGetApp()->m_hInstance, hrsrc);
	if (!hGlobal) {
		return false;
	}
	DWORD size = SizeofResource(AfxGetApp()->m_hInstance, hrsrc);
	if (!size) {
		return false;
	}
	memcpy(str.GetBufferSetLength(size), LockResource(hGlobal), size);
	return true;
}

WORD AssignedToCmd(UINT keyOrMouseValue, bool bIsFullScreen/* = false*/, bool bCheckMouse/* = true*/)
{
	CAppSettings& s = AfxGetAppSettings();

	for (const auto& wc : s.wmcmds) {
		if (bCheckMouse) {
			if (bIsFullScreen) {
				if (wc.mouseFS == keyOrMouseValue) {
					return wc.cmd;
				}
			} else if (wc.mouse == keyOrMouseValue) {
				return wc.cmd;
			}
		} else if (wc.key == keyOrMouseValue) {
			return wc.cmd;
		}
	}

	return 0;
}

void SetAudioRenderer(int AudioDevNo)
{
	auto pApp = AfxGetMyApp();
	pApp->m_AudioRendererDisplayName_CL.Empty();

	CStringArray m_AudioRendererDisplayNames;
	m_AudioRendererDisplayNames.Add(AUDRNDT_MPC);
	m_AudioRendererDisplayNames.Add(L""); // Default DirectSound Device

	BeginEnumSysDev(CLSID_AudioRendererCategory, pMoniker) {
		LPOLESTR olestr = nullptr;
		if (FAILED(pMoniker->GetDisplayName(0, 0, &olestr))) {
			continue;
		}

		CComPtr<IPropertyBag> pPB;
		if (SUCCEEDED(pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB))) {
			CComVariant var;
			if (pPB->Read(CComBSTR(L"FriendlyName"), &var, nullptr) == S_OK) {
				// skip WaveOut
				var.Clear();
				if (pPB->Read(CComBSTR(L"WaveOutId"), &var, nullptr) == S_OK) {
					CoTaskMemFree(olestr);
					continue;
				}

				// skip Default DirectSound Device
				var.Clear();
				if (pPB->Read(CComBSTR(L"DSGuid"), &var, nullptr) == S_OK
					&& CString(var.bstrVal) == "{00000000-0000-0000-0000-000000000000}") {
					CoTaskMemFree(olestr);
					continue;
				}

				m_AudioRendererDisplayNames.Add(olestr);
			}
		}
		CoTaskMemFree(olestr);
	}
	EndEnumSysDev

	m_AudioRendererDisplayNames.Add(AUDRNDT_NULL_COMP);
	m_AudioRendererDisplayNames.Add(AUDRNDT_NULL_UNCOMP);

	if (AudioDevNo >= 0 && AudioDevNo < m_AudioRendererDisplayNames.GetCount()) {
		pApp->m_AudioRendererDisplayName_CL = m_AudioRendererDisplayNames[AudioDevNo];
	}
}

void ThemeRGB(int iR, int iG, int iB, int& iRed, int& iGreen, int& iBlue)
{
	const CAppSettings& s = AfxGetAppSettings();

	iRed   = (s.nThemeBrightness + iR) * s.nThemeRed   / 256;
	iGreen = (s.nThemeBrightness + iG) * s.nThemeGreen / 256;
	iBlue  = (s.nThemeBrightness + iB) * s.nThemeBlue  / 256;

	iRed   = std::clamp(iRed,   0, 255);
	iGreen = std::clamp(iGreen, 0, 255);
	iBlue  = std::clamp(iBlue,  0, 255);
}

COLORREF ThemeRGB(const int iR, const int iG, const int iB)
{
	int iRed, iGreen, iBlue;
	ThemeRGB(iR, iG,iB, iRed, iGreen, iBlue);

	return RGB(iRed, iGreen, iBlue);
}

static int CALLBACK EnumFontFamExProc(ENUMLOGFONTEX* /*lpelfe*/, NEWTEXTMETRICEX* /*lpntme*/, int /*FontType*/, LPARAM lParam)
{
	LPARAM* l = (LPARAM*)lParam;
	*l = TRUE;
	return TRUE;
}

bool IsFontInstalled(LPCWSTR lpszFont)
{
	// Get the screen DC
	CDC dc;
	if (!dc.CreateCompatibleDC(nullptr)) {
		return false;
	}

	LOGFONT lf = {0};
	// Any character set will do
	lf.lfCharSet = DEFAULT_CHARSET;
	// Set the facename to check for
	wcscpy_s(lf.lfFaceName, lpszFont);
	LPARAM lParam = 0;
	// Enumerate fonts
	EnumFontFamiliesExW(dc.GetSafeHdc(), &lf, (FONTENUMPROCW)EnumFontFamExProc, (LPARAM)&lParam, 0);

	return lParam ? true : false;
}

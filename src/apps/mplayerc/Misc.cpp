/*
 * (C) 2016-2025 see Authors.txt
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
#include <winddk/ntddcdrm.h>
#include "DSUtil/FileHandle.h"
#include "DSUtil/SysVersion.h"

bool SetPrivilege(LPCWSTR privilege, bool bEnable/* = true*/)
{
	SetThreadExecutionState(ES_CONTINUOUS);

	HANDLE hToken;
	// Get a token for this process.
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		return false;
	}

	TOKEN_PRIVILEGES tkp;
	// Get the LUID for the privilege.
	LookupPrivilegeValueW(nullptr, privilege, &tkp.Privileges[0].Luid);

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
CString GetLastErrorMsg(LPWSTR lpszFunction, DWORD dw/* = GetLastError()*/)
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
		(LPWSTR)&lpMsgBuf,
		0, nullptr);

	// Format the error message

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(wcslen((LPCWSTR)lpMsgBuf) + wcslen((LPCWSTR)lpszFunction) + 40) * sizeof(WCHAR));
	StringCchPrintfW((LPWSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(WCHAR),
		L"Function '%s' failed with error %u: %s",
		lpszFunction, dw, lpMsgBuf);

	CString ret = (LPCWSTR)lpDisplayBuf;

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);

	return ret;
}

HICON LoadIcon(const CString& fn, bool fSmall)
{
	if (fn.IsEmpty()) {
		return nullptr;
	}

	auto ext = GetFileExt(fn).MakeLower();

	CSize size(fSmall ? 16 : 32, fSmall ? 16 : 32);

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

	WCHAR buff[MAX_PATH] = {};
	if (fn.GetLength() <= MAX_PATH && ::PathFileExistsW(fn)) {
		wcscpy_s(buff, fn);

		SHFILEINFO sfi = {};
		if (SUCCEEDED(SHGetFileInfoW(buff, 0, &sfi, sizeof(sfi), (fSmall ? SHGFI_SMALLICON : SHGFI_LARGEICON) | SHGFI_ICON)) && sfi.hIcon) {
			return sfi.hIcon;
		}
	}

	do {
		CRegKey key;
		ULONG len;

		CStringW RegPathAssociated;
		RegPathAssociated.Format(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%s\\UserChoice", ext);

		if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, RegPathAssociated, KEY_READ)) {
			len = std::size(buff);
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

			len = std::size(buff);
			memset(buff, 0, sizeof(buff));
			if (ERROR_SUCCESS != key.QueryStringValue(nullptr, buff, &len) || (ext = buff).Trim().IsEmpty()) {
				break;
			}

			if (ERROR_SUCCESS != key.Open(HKEY_CLASSES_ROOT, ext + L"\\DefaultIcon", KEY_READ)) {
				break;
			}
		}

		CString icon;

		len = std::size(buff);
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

		HICON hIcon = nullptr;
		UINT cnt = fSmall
				   ? ExtractIconExW(icon, id, nullptr, &hIcon, 1)
				   : ExtractIconExW(icon, id, &hIcon, nullptr, 1);
		UNREFERENCED_PARAMETER(cnt);
		if (hIcon) {
			return hIcon;
		}
	} while (0);

	if (fn.Find(L"://") >= 0) {
		if (HICON hIcon = (HICON)LoadImageW(AfxGetInstanceHandle(), MAKEINTRESOURCEW(IDR_MAINFRAME), IMAGE_ICON, size.cx, size.cy, 0)) {
			return hIcon;
		}
	}

	return (HICON)LoadImageW(AfxGetInstanceHandle(), MAKEINTRESOURCEW(IDI_UNKNOWN), IMAGE_ICON, size.cx, size.cy, 0);
}

bool LoadType(const CString& fn, CString& type)
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
					len = std::size(buff);
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

bool LoadResource(UINT resid, CStringA& str, LPCWSTR restype)
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

HRESULT LoadResourceFile(UINT resid, BYTE** ppData, UINT& size)
{
	HRSRC hrsrc = FindResourceW(AfxGetApp()->m_hInstance, MAKEINTRESOURCEW(resid), L"FILE");
	if (!hrsrc) {
		return E_INVALIDARG;
	}
	HGLOBAL hGlobal = LoadResource(AfxGetApp()->m_hInstance, hrsrc);
	if (!hGlobal) {
		return E_FAIL;
	}
	size = SizeofResource(AfxGetApp()->m_hInstance, hrsrc);
	if (!size) {
		return E_FAIL;
	}
	*ppData = (BYTE*)LockResource(hGlobal);
	if (!*ppData) {
		return E_FAIL;
	}

	return S_OK;
}

CStringW GetDragQueryFileName(HDROP hDrop, UINT iFile)
{
	CStringW fname;

	if (iFile < UINT_MAX) {
		UINT len = ::DragQueryFileW(hDrop, iFile, nullptr, 0);
		if (len > 0) {
			len = ::DragQueryFileW(hDrop, iFile, fname.GetBuffer(len), len+1);
			fname.ReleaseBufferSetLength(len);
		}
	}

	return fname;
}

bool LongPathsEnabled()
{
	bool bLongPathsEnabled = false;

	if (SysVersion::IsWin10v1607orLater()) {
		CRegKey regkey;
		if (ERROR_SUCCESS == regkey.Open(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\FileSystem", KEY_READ)) {
			DWORD value;
			if (ERROR_SUCCESS == regkey.QueryDWORDValue(L"LongPathsEnabled", value)) {
				bLongPathsEnabled = (value == 1);
			}
			regkey.Close();
		}
	}

	return bLongPathsEnabled;
}

void ConvertLongPath(CStringW& path)
{
	if (StartsWith(path, L"\\\\?\\")) {
		static const bool bLongPathsEnabled = LongPathsEnabled();
		if (bLongPathsEnabled) {
			path = path.Mid(4);
		}
	}
}

WORD AssignedKeyToCmd(UINT keyValue)
{
	CAppSettings& s = AfxGetAppSettings();

	for (const auto& wc : s.wmcmds) {
		if (wc.key == keyValue) {
			return wc.cmd;
		}
	}

	return 0;
}

WORD AssignedMouseToCmd(UINT mouseValue, UINT nFlags)
{
	CAppSettings& s = AfxGetAppSettings();

	CAppSettings::MOUSE_ASSIGNMENT mcmds = {};

	switch (mouseValue) {
	case MOUSE_CLICK_MIDLE:    mcmds = s.MouseMiddleClick;  break;
	case MOUSE_CLICK_X1:       mcmds = s.MouseX1Click;      break;
	case MOUSE_CLICK_X2:       mcmds = s.MouseX2Click;      break;
	case MOUSE_WHEEL_UP:       mcmds = s.MouseWheelUp;      break;
	case MOUSE_WHEEL_DOWN:     mcmds = s.MouseWheelDown;    break;
	case MOUSE_WHEEL_LEFT:     mcmds = s.MouseWheelLeft;    break;
	case MOUSE_WHEEL_RIGHT:    mcmds = s.MouseWheelRight;   break;
	case MOUSE_CLICK_LEFT:     return (WORD)s.nMouseLeftClick;
	case MOUSE_CLICK_LEFT_DBL: return (WORD)s.nMouseLeftDblClick;
	}

	if (mcmds.ctrl && (nFlags & MK_CONTROL)) {
		return (WORD)mcmds.ctrl;
	}
	if (mcmds.shift && (nFlags & MK_SHIFT)) {
		return (WORD)mcmds.shift;
	}
	if (mcmds.rbtn && (nFlags & MK_RBUTTON)) {
		return (WORD)mcmds.rbtn;
	}

	return (WORD)mcmds.normal;
}

void SetAudioRenderer(const CStringW& str)
{
	auto pApp = AfxGetMyApp();
	pApp->m_AudioRendererDisplayName_CL.Empty();

	if (str == AUDRNDT_MPC || str == "" || str == AUDRNDT_NULL_COMP || str == AUDRNDT_NULL_UNCOMP) {
		pApp->m_AudioRendererDisplayName_CL = str;
		return;
	}

	enum {
		isFriendlyName,
		isDisplayName,
		isCLSID,
	};

	int type = isFriendlyName;
	if (str.GetLength() == 38 && str[0] == '{' && str[37] == '}') {
		type = isCLSID;
	}
	else if (StartsWith(str, L"@device:")) {
		type = isDisplayName;
	}

	BeginEnumSysDev(CLSID_AudioRendererCategory, pMoniker) {
		CComPtr<IPropertyBag> pPB;
		if (SUCCEEDED(pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB))) {
			CComVariant var;
			// skip WaveOut
			if (pPB->Read(CComBSTR(L"WaveOutId"), &var, nullptr) == S_OK) {
				continue;
			}

			LPOLESTR olestr = nullptr;
			if (FAILED(pMoniker->GetDisplayName(0, 0, &olestr))) {
				continue;
			}
			CStringW displayName = olestr;
			CoTaskMemFree(olestr);

			var.Clear();

			switch (type) {
			default:
			case isFriendlyName:
				if (pPB->Read(CComBSTR(L"FriendlyName"), &var, nullptr) == S_OK && var.vt == VT_BSTR) {
					if (str == var.bstrVal) {
						pApp->m_AudioRendererDisplayName_CL = displayName;
						return;
					}
				}
				break;
			case isDisplayName:
				if (str == displayName) {
					pApp->m_AudioRendererDisplayName_CL = displayName;
					return;
				}
				break;
			case isCLSID:
				if (pPB->Read(CComBSTR(L"CLSID"), &var, nullptr) == S_OK && var.vt == VT_BSTR) {
					if (str.CompareNoCase(var.bstrVal) == 0) {
						pApp->m_AudioRendererDisplayName_CL = displayName;
						return;
					}
				}
				break;
			}
		}
	}
	EndEnumSysDev
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

	LOGFONTW lf = {0};
	// Any character set will do
	lf.lfCharSet = DEFAULT_CHARSET;
	// Set the facename to check for
	wcscpy_s(lf.lfFaceName, lpszFont);
	LPARAM lParam = 0;
	// Enumerate fonts
	EnumFontFamiliesExW(dc.GetSafeHdc(), &lf, (FONTENUMPROCW)EnumFontFamExProc, (LPARAM)&lParam, 0);

	return lParam ? true : false;
}

static void FindFiles(CStringW fn, std::list<CStringW>& files)
{
	CStringW path = fn;
	path.Replace('/', '\\');
	path.Truncate(path.ReverseFind('\\') + 1);

	WIN32_FIND_DATA findData;
	HANDLE h = FindFirstFileW(fn, &findData);
	if (h != INVALID_HANDLE_VALUE) {
		do {
			files.push_back(path + findData.cFileName);
		} while (FindNextFileW(h, &findData));

		FindClose(h);
	}
}

cdrom_t GetCDROMType(const WCHAR drive, std::list<CStringW>& files)
{
	files.clear();

	CStringW path;
	path.Format(L"%c:", drive);

	if (GetDriveTypeW(path + L"\\") == DRIVE_CDROM) {
		// Check if it contains a disc
		HANDLE hDevice = CreateFileW(LR"(\\.\)" + path, FILE_READ_ATTRIBUTES,
			FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
		if (hDevice == INVALID_HANDLE_VALUE) {
			return CDROM_Unknown;
		}
		DWORD cbBytesReturned = {};
		BOOL bSuccess = DeviceIoControl(hDevice, IOCTL_STORAGE_CHECK_VERIFY2,
			nullptr, 0, nullptr, 0, &cbBytesReturned, nullptr);
		CloseHandle(hDevice);
		if (!bSuccess) {
			return CDROM_Unknown;
		}

		// CDROM_DVDVideo
		FindFiles(path + L"\\VIDEO_TS\\VIDEO_TS.IFO", files);
		if (files.size() > 0) {
			return CDROM_DVDVideo;
		}

		// CDROM_DVDAudio
		FindFiles(path + L"\\AUDIO_TS\\ATS_0?_0.IFO", files);
		if (files.size() > 0) {
			return CDROM_DVDAudio;
		}

		// CDROM_BD
		FindFiles(path + L"\\BDMV\\index.bdmv", files);
		if (!files.empty()) {
			return CDROM_BDVideo;
		}

		// CDROM_VideoCD
		FindFiles(path + L"\\mpegav\\avseq??.dat", files);
		FindFiles(path + L"\\mpegav\\avseq??.mpg", files);
		FindFiles(path + L"\\mpegav\\music??.dat", files);
		FindFiles(path + L"\\mpegav\\music??.mpg", files);
		FindFiles(path + L"\\mpeg2\\avseq??.dat", files);
		FindFiles(path + L"\\mpeg2\\avseq??.mpg", files);
		FindFiles(path + L"\\mpeg2\\music??.dat", files);
		FindFiles(path + L"\\mpeg2\\music??.mpg", files);
		if (files.size() > 0) {
			return CDROM_VideoCD;
		}

		// CDROM_Audio
		HANDLE hDrive = CreateFileW(CString(L"\\\\.\\") + path, GENERIC_READ, FILE_SHARE_READ, nullptr,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE)nullptr);
		if (hDrive != INVALID_HANDLE_VALUE) {
			DWORD BytesReturned;
			CDROM_TOC TOC;
			if (DeviceIoControl(hDrive, IOCTL_CDROM_READ_TOC, nullptr, 0, &TOC, sizeof(TOC), &BytesReturned, 0)) {
				for (int i = TOC.FirstTrack; i <= TOC.LastTrack; i++) {
					// MMC-3 Draft Revision 10g: Table 222 - Q Sub-channel control field
					TOC.TrackData[i - 1].Control &= 5;
					if (TOC.TrackData[i - 1].Control == 0 || TOC.TrackData[i - 1].Control == 1) {
						CStringW fn;
						fn.Format(L"%s\\track%02d.cda", path.GetString(), i);
						files.push_back(fn);
					}
				}
			}

			CloseHandle(hDrive);
		}
		if (files.size() > 0) {
			return CDROM_Audio;
		}

		// it is a cdrom but nothing special
		return CDROM_Unknown;
	}

	return CDROM_NotFound;
}

//
// CMPCGradient
//

UINT CMPCGradient::Size()
{
	return m_size;
}

void CMPCGradient::Clear()
{
	m_pData.reset();
	m_size = 0;
	m_width = 0;
	m_height = 0;
}

HRESULT CMPCGradient::Create(IWICBitmapSource* pBitmapSource)
{
	if (!pBitmapSource) {
		return E_POINTER;
	}

	WICPixelFormatGUID pixelFormat;

	HRESULT hr = pBitmapSource->GetPixelFormat(&pixelFormat);

	if (SUCCEEDED(hr)) {
		if (pixelFormat != GUID_WICPixelFormat32bppBGRA && pixelFormat != GUID_WICPixelFormat32bppPBGRA) {
			return E_INVALIDARG;
		}
		hr = pBitmapSource->GetSize(&m_width, &m_height);
	}

	if (SUCCEEDED(hr)) {
		m_size = m_width * m_height * 4;
		m_pData.reset(new(std::nothrow) BYTE[m_size]);
		if (m_pData) {
			hr = pBitmapSource->CopyPixels(nullptr, m_width * 4, m_size, m_pData.get());
		} else {
			hr = E_OUTOFMEMORY;
		}

	}

	if (FAILED(hr)) {
		Clear();
	}

	return hr;
}

bool CMPCGradient::Paint(CDC* dc, CRect r, int ptop, int br/* = -1*/, int rc/* = -1*/, int gc/* = -1*/, int bc/* = -1*/)
{
	if (m_size) {
		BYTE* pData = m_pData.get();

		// code from removed CMPCPngImage::PaintExternalGradient
		GRADIENT_RECT gr = {0, 1};

		int bt = 4, st = 2, pa = 255 * 256;
		int sp = bt * ptop, hs = bt * st;

		if (m_width > m_height) {
			for (int k = sp, t = 0; t < r.right; k += hs, t += st) {
				TRIVERTEX tv[2] = {
					{t, 0, pData[k + 2] * 256, pData[k + 1] * 256, pData[k] * 256, pa},
					{t + st, 1, pData[k + hs + 2] * 256, pData[k + hs + 1] * 256, pData[k + hs] * 256, pa},
				};
				dc->GradientFill(tv, 2, &gr, 1, GRADIENT_FILL_RECT_H);
			}
		} else {
			for (int k = sp, t = 0; t < r.bottom; k += hs, t += st) {
				TRIVERTEX tv[2] = {
					{r.left, t, pData[k + 2] * 256, pData[k + 1] * 256, pData[k] * 256, pa},
					{r.right, t + st, pData[k + hs + 2] * 256, pData[k + hs + 1] * 256, pData[k + hs] * 256, pa},
				};
				dc->GradientFill(tv, 2, &gr, 1, GRADIENT_FILL_RECT_V);
			}
		}

		delete [] pData;

		return true;
	}

	return false;
}

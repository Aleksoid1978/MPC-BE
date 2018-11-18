/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2018 see Authors.txt
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
#include <thread>
#include "AboutDlg.h"
#include "CmdLineHelpDlg.h"
#include <Tlhelp32.h>
#include "MainFrm.h"
#include "Misc.h"
#include <winternl.h>
#include <psapi.h>
#include "Ifo.h"
#include "MultiMonitor.h"
#include "../../DSUtil/WinAPIUtils.h"
#include "../../DSUtil/SysVersion.h"
#include "../../DSUtil/FileHandle.h"
#include "../../DSUtil/FileVersion.h"
#include <winddk/ntddcdvd.h>
#include <detours/detours.h>
#include <afxsock.h>
#include <atlsync.h>
#include <atlutil.h>
#include "UpdateChecker.h"

extern "C" {
	// hack to avoid error "unresolved external symbol" when linking
	int mingw_app_type = 1;
}

const LanguageResource CMPlayerCApp::languageResources[] = {
	{ID_LANGUAGE_ARMENIAN,				1067,	L"Armenian",				L"hy"},
	{ID_LANGUAGE_BASQUE,				1069,	L"Basque",					L"eu"},
	{ID_LANGUAGE_BELARUSIAN,			1059,	L"Belarusian",				L"by"},
	{ID_LANGUAGE_CATALAN,				1027,	L"Catalan",					L"ca"},
	{ID_LANGUAGE_CHINESE_SIMPLIFIED,	2052,	L"Chinese (Simplified)",	L"sc"},
	{ID_LANGUAGE_CHINESE_TRADITIONAL,	3076,	L"Chinese (Traditional)",	L"tc"},
	{ID_LANGUAGE_CZECH,					1029,	L"Czech",					L"cz"},
	{ID_LANGUAGE_DUTCH,					1043,	L"Dutch",					L"nl"},
	{ID_LANGUAGE_ENGLISH,				0,		L"English",					L"en"},
	{ID_LANGUAGE_FRENCH,				1036,	L"French",					L"fr"},
	{ID_LANGUAGE_GERMAN,				1031,	L"German",					L"de"},
	{ID_LANGUAGE_GREEK,					1032,	L"Greek",					L"el"},
	{ID_LANGUAGE_HEBREW,				1037,	L"Hebrew",					L"he"},
	{ID_LANGUAGE_HUNGARIAN,				1038,	L"Hungarian",				L"hu"},
	{ID_LANGUAGE_ITALIAN,				1040,	L"Italian",					L"it"},
	{ID_LANGUAGE_JAPANESE,				1041,	L"Japanese",				L"ja"},
	{ID_LANGUAGE_KOREAN,				1042,	L"Korean",					L"kr"},
	{ID_LANGUAGE_POLISH,				1045,	L"Polish",					L"pl"},
	{ID_LANGUAGE_PORTUGUESE_BR,			1046,	L"Portuguese (Brazil)",		L"br"},
	{ID_LANGUAGE_ROMANIAN,				1048,	L"Romanian",				L"ro"},
	{ID_LANGUAGE_RUSSIAN,				1049,	L"Russian",					L"ru"},
	{ID_LANGUAGE_SLOVAK,				1053,	L"Slovak",					L"sk"},
	{ID_LANGUAGE_SWEDISH,				1051,	L"Swedish",					L"sv"},
	{ID_LANGUAGE_SPANISH,				1034,	L"Spanish",					L"es"},
	{ID_LANGUAGE_TURKISH,				1055,	L"Turkish",					L"tr"},
	{ID_LANGUAGE_UKRAINIAN,				1058,	L"Ukrainian",				L"ua"},
};

const size_t CMPlayerCApp::languageResourcesCount = _countof(CMPlayerCApp::languageResources);

typedef struct _PEB_FREE_BLOCK // Size = 8
{
	struct _PEB_FREE_BLOCK *Next;
	ULONG Size;
} PEB_FREE_BLOCK, *PPEB_FREE_BLOCK;

typedef void (*PPEBLOCKROUTINE)(PVOID);

// PEB (Process Environment Block) data structure (FS:[0x30])
// Located at addr. 0x7FFDF000

typedef struct _PEB_NT // Size = 0x1E8
{
	BOOLEAN				InheritedAddressSpace;			//000
	BOOLEAN				ReadImageFileExecOptions;		//001
	BOOLEAN				BeingDebugged;					//002
	BOOLEAN				SpareBool;						//003 Allocation size
	HANDLE				Mutant;							//004
	HINSTANCE			ImageBaseAddress;				//008 Instance
	PPEB_LDR_DATA		LdrData;						//00C
	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters;	//010
	ULONG				SubSystemData;					//014
	HANDLE				ProcessHeap;					//018
	KSPIN_LOCK			FastPebLock;					//01C
	PPEBLOCKROUTINE		FastPebLockRoutine;				//020
	PPEBLOCKROUTINE		FastPebUnlockRoutine;			//024
	ULONG				EnvironmentUpdateCount;			//028
	PVOID *				KernelCallbackTable;			//02C
	PVOID				EventLogSection;				//030
	PVOID				EventLog;						//034
	PPEB_FREE_BLOCK		FreeList;						//038
	ULONG				TlsExpansionCounter;			//03C
	ULONG				TlsBitmap;						//040
	LARGE_INTEGER		TlsBitmapBits;					//044
	PVOID				ReadOnlySharedMemoryBase;		//04C
	PVOID				ReadOnlySharedMemoryHeap;		//050
	PVOID *				ReadOnlyStaticServerData;		//054
	PVOID				AnsiCodePageData;				//058
	PVOID				OemCodePageData;				//05C
	PVOID				UnicodeCaseTableData;			//060
	ULONG				NumberOfProcessors;				//064
	LARGE_INTEGER		NtGlobalFlag;					//068 Address of a local copy
	LARGE_INTEGER		CriticalSectionTimeout;			//070
	ULONG				HeapSegmentReserve;				//078
	ULONG				HeapSegmentCommit;				//07C
	ULONG				HeapDeCommitTotalFreeThreshold;	//080
	ULONG				HeapDeCommitFreeBlockThreshold;	//084
	ULONG				NumberOfHeaps;					//088
	ULONG				MaximumNumberOfHeaps;			//08C
	PVOID **			ProcessHeaps;					//090
	PVOID				GdiSharedHandleTable;			//094
	PVOID				ProcessStarterHelper;			//098
	PVOID				GdiDCAttributeList;				//09C
	KSPIN_LOCK			LoaderLock;						//0A0
	ULONG				OSMajorVersion;					//0A4
	ULONG				OSMinorVersion;					//0A8
	USHORT				OSBuildNumber;					//0AC
	USHORT				OSCSDVersion;					//0AE
	ULONG				OSPlatformId;					//0B0
	ULONG				ImageSubsystem;					//0B4
	ULONG				ImageSubsystemMajorVersion;		//0B8
	ULONG				ImageSubsystemMinorVersion;		//0BC
	ULONG				ImageProcessAffinityMask;		//0C0
	ULONG				GdiHandleBuffer[0x22];			//0C4
	ULONG				PostProcessInitRoutine;			//14C
	ULONG				TlsExpansionBitmap;				//150
	UCHAR				TlsExpansionBitmapBits[0x80];	//154
	ULONG				SessionId;						//1D4
	void *				AppCompatInfo;					//1D8
	UNICODE_STRING		CSDVersion;						//1DC
} PEB_NT, *PPEB_NT;

/////////////////////////////////////////////////////////////////////////////
// CMPlayerCApp

BEGIN_MESSAGE_MAP(CMPlayerCApp, CWinApp)
	//{{AFX_MSG_MAP(CMPlayerCApp)
	ON_COMMAND(ID_HELP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_FILE_EXIT, OnFileExit)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP_SHOWCOMMANDLINESWITCHES, OnHelpShowcommandlineswitches)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMPlayerCApp construction

CMPlayerCApp::CMPlayerCApp()
	: GetRemoteControlCode(GetRemoteControlCodeMicrosoft)
{
}

CMPlayerCApp::~CMPlayerCApp()
{
	// Wait for any pending I/O operations to be canceled
	while (WAIT_IO_COMPLETION == SleepEx(0, TRUE));
}

BOOL CMPlayerCApp::OnIdle(LONG lCount)
{
	BOOL ret = __super::OnIdle(lCount);
	if (!ret) {
		m_Profile.Flush(false);
	}

	return ret;
}

bool CMPlayerCApp::ClearSettings()
{
	// We want the other instances to be closed before resetting the settings.
	HWND hWnd = FindWindowW(MPC_WND_CLASS_NAMEW, nullptr);
	while (hWnd) {
		Sleep(500);
		hWnd = FindWindowW(MPC_WND_CLASS_NAMEW, nullptr);
		if (hWnd && MessageBoxW(nullptr, ResStr(IDS_RESET_SETTINGS_MUTEX), ResStr(IDS_RESET_SETTINGS), MB_ICONEXCLAMATION | MB_RETRYCANCEL) == IDCANCEL) {
			return false;
		}
	}

	m_Profile.Clear();

	// Remove the current playlist if it exists
	CString strSavePath;
	if (AfxGetMyApp()->GetAppSavePath(strSavePath)) {
		CPath playlistPath;
		playlistPath.Combine(strSavePath, L"default.mpcpl");

		CFileStatus status;
		if (CFile::GetStatus(playlistPath, status)) {
			CFile::Remove(playlistPath);
		}
	}

	return true;
}

void CMPlayerCApp::ShowCmdlnSwitches() const
{
	CString s;

	if (m_s.nCLSwitches&CLSW_UNRECOGNIZEDSWITCH) {
		std::list<CString> sl;
		for (int i = 0; i < __argc; i++) {
			sl.emplace_back(__wargv[i]);
		}
		s += ResStr(IDS_UNKNOWN_SWITCH) + Implode(sl, ' ') + L"\n\n";
	}

	s += ResStr(IDS_USAGE);

	//AfxMessageBox(s, MB_ICONINFORMATION | MB_OK);
	CmdLineHelpDlg dlg(s);
	dlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMPlayerCApp object

CMPlayerCApp theApp;

HWND g_hWnd = nullptr;

bool CMPlayerCApp::GetAppSavePath(CString& path)
{
	path.Empty();

	if (m_Profile.IsIniValid()) { // If settings ini file found, store stuff in the same folder as the exe file
		path = GetProgramDir();
	} else {
		HRESULT hr = SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, path.GetBuffer(MAX_PATH));
		path.ReleaseBuffer();
		if (FAILED(hr)) {
			return false;
		}
		CPath p;
		p.Combine(path, L"MPC-BE");
		path = AddSlash(p);
	}

	return true;
}

bool CMPlayerCApp::ChangeSettingsLocation(bool useIni)
{
	CString oldpath;
	AfxGetMyApp()->GetAppSavePath(oldpath);

	// Load favorites so that they can be correctly saved to the new location
	std::list<CString> filesFav, DVDsFav, devicesFav;
	AfxGetAppSettings().GetFav(FAV_FILE, filesFav);
	AfxGetAppSettings().GetFav(FAV_DVD, DVDsFav);
	AfxGetAppSettings().GetFav(FAV_DEVICE, devicesFav);

	if (useIni) {
		bool success = m_Profile.StoreSettingsToIni();
		if (!success) {
			return false;
		}
	} else {
		m_Profile.StoreSettingsToRegistry();
		_wremove(m_Profile.GetIniPath());
	}

	// Save favorites to the new location
	AfxGetAppSettings().SetFav(FAV_FILE, filesFav);
	AfxGetAppSettings().SetFav(FAV_DVD, DVDsFav);
	AfxGetAppSettings().SetFav(FAV_DEVICE, devicesFav);

	// Save external filters to the new location
	m_s.SaveExternalFilters();

	// Write settings immediately
	m_s.SaveSettings();

	if (oldpath.GetLength() > 0) {
		DeleteFileW(oldpath + L"default.mpcpl");

		// moving shader files
		const CStringW shaderpath = oldpath + L"Shaders\\";
		if (::PathFileExistsW(shaderpath)) {
			CString newpath;
			AfxGetMyApp()->GetAppSavePath(newpath);

			// use SHFileOperation, because MoveFile/MoveFileEx will fail on directory moves when the destination is on a different volume.
			WCHAR pathFrom[MAX_PATH] = { 0 }; // for double null-terminated string
			wcscpy(pathFrom, shaderpath);

			WCHAR pathTo[MAX_PATH] = { 0 }; // for double null-terminated string
			wcscpy(pathTo, newpath);

			SHFILEOPSTRUCTW sf = { 0 };
			sf.wFunc = FO_MOVE;
			sf.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_NOERRORUI;
			sf.pFrom = pathFrom;
			sf.pTo = pathTo;
			if (SHFileOperationW(&sf) != 0) {
				MessageBoxW(nullptr, L"Moving shader files failed", ResStr(IDS_AG_ERROR), MB_OK);
			}
		}
	}

	return true;
}

static const CString GetHiveName(HKEY hive)
{
	switch ((ULONG_PTR)hive) {
	case (ULONG_PTR)HKEY_CLASSES_ROOT:
		return L"HKEY_CLASSES_ROOT";
	case (ULONG_PTR)HKEY_CURRENT_USER:
		return L"HKEY_CURRENT_USER";
	case (ULONG_PTR)HKEY_LOCAL_MACHINE:
		return L"HKEY_LOCAL_MACHINE";
	case (ULONG_PTR)HKEY_USERS:
		return L"HKEY_USERS";
	case (ULONG_PTR)HKEY_PERFORMANCE_DATA:
		return L"HKEY_PERFORMANCE_DATA";
	case (ULONG_PTR)HKEY_CURRENT_CONFIG:
		return L"HKEY_CURRENT_CONFIG";
	case (ULONG_PTR)HKEY_DYN_DATA:
		return L"HKEY_DYN_DATA";
	case (ULONG_PTR)HKEY_PERFORMANCE_TEXT:
		return L"HKEY_PERFORMANCE_TEXT";
	case (ULONG_PTR)HKEY_PERFORMANCE_NLSTEXT:
		return L"HKEY_PERFORMANCE_NLSTEXT";
	default:
		return L"";
	}
}

static bool ExportRegistryKey(CStdioFile& file, HKEY hKeyRoot, CString keyName)
{
	HKEY hKey = nullptr;
	if (RegOpenKeyExW(hKeyRoot, keyName, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
		return false;
	}

	DWORD subKeysCount = 0, maxSubKeyLen = 0;
	DWORD valuesCount = 0, maxValueNameLen = 0, maxValueDataLen = 0;
	if (RegQueryInfoKeyW(hKey, nullptr, nullptr, nullptr, &subKeysCount, &maxSubKeyLen, nullptr, &valuesCount, &maxValueNameLen, &maxValueDataLen, nullptr, nullptr) != ERROR_SUCCESS) {
		return false;
	}
	maxSubKeyLen += 1;
	maxValueNameLen += 1;

	CString buffer;

	buffer.Format(L"[%s\\%s]\n", GetHiveName(hKeyRoot), keyName);
	file.WriteString(buffer);

	CString valueName;
	DWORD valueNameLen, valueDataLen, type;
	BYTE* data = DNew BYTE[maxValueDataLen];

	for (DWORD indexValue = 0; indexValue < valuesCount; indexValue++) {
		valueNameLen = maxValueNameLen;
		valueDataLen = maxValueDataLen;

		if (RegEnumValueW(hKey, indexValue, valueName.GetBuffer(maxValueNameLen), &valueNameLen, nullptr, &type, data, &valueDataLen) != ERROR_SUCCESS) {
			delete [] data;
			return false;
		}

		switch (type) {
			case REG_SZ:
				{
					CString str((WCHAR*)data);
					str.Replace(L"\\", L"\\\\");
					str.Replace(L"\"", L"\\\"");
					buffer.Format(L"\"%s\"=\"%s\"\n", valueName, str);
					file.WriteString(buffer);
				}
				break;
			case REG_BINARY:
			case REG_QWORD:
				buffer.Format(L"\"%s\"=hex%s:%02x", valueName, type == REG_QWORD ? L"(b)" : L"", data[0]);
				file.WriteString(buffer);
				for (DWORD i = 1; i < valueDataLen; i++) {
					buffer.Format(L",%02x", data[i]);
					file.WriteString(buffer);
				}
				file.WriteString(L"\n");
				break;
			case REG_DWORD:
				buffer.Format(L"\"%s\"=dword:%08x\n", valueName, *((DWORD*)data));
				file.WriteString(buffer);
				break;
			default:
				{
					CString msg;
					msg.Format(L"The value \"%s\\%s\\%s\" has an unsupported type and has been ignored.\nPlease report this error to the developers.",
							   GetHiveName(hKeyRoot), keyName, valueName);
					AfxMessageBox(msg, MB_ICONERROR | MB_OK);
				}
				delete [] data;
				return false;
		}
	}

	delete [] data;

	file.WriteString(L"\n");

	CString subKeyName;
	DWORD subKeyLen;

	for (DWORD indexSubKey = 0; indexSubKey < subKeysCount; indexSubKey++) {
		subKeyLen = maxSubKeyLen;

		if (RegEnumKeyExW(hKey, indexSubKey, subKeyName.GetBuffer(maxSubKeyLen), &subKeyLen, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) {
			return false;
		}

		buffer.Format(L"%s\\%s", keyName, subKeyName);

		if (!ExportRegistryKey(file, hKeyRoot, buffer)) {
			return false;
		}
	}

	RegCloseKey(hKey);

	return true;
}

void CMPlayerCApp::ExportSettings()
{
	CString ext = m_Profile.IsIniValid() ? L"ini" : L"reg";
	CString ext_list;
	ext_list.Format(L"Export files (*.%s)|*.%s|", ext, ext);

	CFileDialog fileSaveDialog(FALSE, 0, L"mpc-be-settings." + ext,
							   OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR,
							   ext_list);

	if (fileSaveDialog.DoModal() == IDOK) {
		CString savePath = fileSaveDialog.GetPathName();
		if (ext.CompareNoCase(fileSaveDialog.GetFileExt()) != 0) {
			savePath.Append(L"." + ext);
		}

		auto& s = AfxGetAppSettings();
		const int nLastUsedPage = s.nLastUsedPage;
		s.nLastUsedPage = 0;

		bool success = false;
		s.SaveSettings();

		if (m_Profile.IsIniValid()) {
			success = !!CopyFileW(m_Profile.GetIniPath(), savePath, FALSE);
		} else {
			FILE* fStream;
			errno_t error = _wfopen_s(&fStream, savePath, L"wt,ccs=UNICODE");
			CStdioFile file(fStream);
			file.WriteString(L"Windows Registry Editor Version 5.00\n\n");

			success = !error && ExportRegistryKey(file, HKEY_CURRENT_USER, L"Software\\MPC-BE");

			file.Close();
		}

		s.nLastUsedPage = nLastUsedPage;

		if (success) {
			MessageBoxW(GetMainWnd()->m_hWnd, ResStr(IDS_EXPORT_SETTINGS_SUCCESS), ResStr(IDS_EXPORT_SETTINGS), MB_ICONINFORMATION | MB_OK);
		} else {
			MessageBoxW(GetMainWnd()->m_hWnd, ResStr(IDS_EXPORT_SETTINGS_FAILED), ResStr(IDS_EXPORT_SETTINGS), MB_ICONERROR | MB_OK);
		}
	}
}

void CMPlayerCApp::PreProcessCommandLine()
{
	m_cmdln.clear();

	for (int i = 1; i < __argc; i++) {
		m_cmdln.push_back(CString(__wargv[i]).Trim(L" \""));
	}
}

BOOL CMPlayerCApp::SendCommandLine(HWND hWnd)
{
	if (m_cmdln.empty()) {
		return FALSE;
	}

	size_t bufflen = sizeof(DWORD);

	for (const auto& item : m_cmdln) {
		bufflen += (item.GetLength() + 1) * sizeof(WCHAR);
	}

	std::vector<BYTE> buff(bufflen);
	if (buff.empty()) {
		return FALSE;
	}

	BYTE* p = buff.data();

	*(DWORD*)p = m_cmdln.size();
	p += sizeof(DWORD);

	for (const auto& item : m_cmdln) {
		const size_t len = (item.GetLength() + 1) * sizeof(WCHAR);
		memcpy(p, item, len);
		p += len;
	}

	COPYDATASTRUCT cds;
	cds.dwData = 0x6ABE51;
	cds.cbData = bufflen;
	cds.lpData = buff.data();
	return SendMessageW(hWnd, WM_COPYDATA, (WPARAM)nullptr, (LPARAM)&cds);
}

/////////////////////////////////////////////////////////////////////////////
// CMPlayerCApp initialization

BOOL (__stdcall * Real_IsDebuggerPresent)(void)
	= IsDebuggerPresent;

LONG (__stdcall * Real_ChangeDisplaySettingsExA)(LPCSTR a0,
		LPDEVMODEA a1,
		HWND a2,
		DWORD a3,
		LPVOID a4)
	= ChangeDisplaySettingsExA;

LONG (__stdcall * Real_ChangeDisplaySettingsExW)(LPCWSTR a0,
		LPDEVMODEW a1,
		HWND a2,
		DWORD a3,
		LPVOID a4)
	= ChangeDisplaySettingsExW;

HANDLE (__stdcall * Real_CreateFileA)(LPCSTR a0,
									  DWORD a1,
									  DWORD a2,
									  LPSECURITY_ATTRIBUTES a3,
									  DWORD a4,
									  DWORD a5,
									  HANDLE a6)
	= CreateFileA;

HANDLE (__stdcall * Real_CreateFileW)(LPCWSTR a0,
									  DWORD a1,
									  DWORD a2,
									  LPSECURITY_ATTRIBUTES a3,
									  DWORD a4,
									  DWORD a5,
									  HANDLE a6)
	= CreateFileW;

BOOL (__stdcall * Real_DeviceIoControl)(HANDLE a0,
										DWORD a1,
										LPVOID a2,
										DWORD a3,
										LPVOID a4,
										DWORD a5,
										LPDWORD a6,
										LPOVERLAPPED a7)
	= DeviceIoControl;

MMRESULT  (__stdcall * Real_mixerSetControlDetails)( HMIXEROBJ hmxobj,
		LPMIXERCONTROLDETAILS pmxcd,
		DWORD fdwDetails)
	= mixerSetControlDetails;

typedef NTSTATUS (WINAPI *FUNC_NTQUERYINFORMATIONPROCESS)(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength);
static FUNC_NTQUERYINFORMATIONPROCESS		Real_NtQueryInformationProcess = nullptr;

/*
NTSTATUS (* Real_NtQueryInformationProcess) (HANDLE				ProcessHandle,
											 PROCESSINFOCLASS	ProcessInformationClass,
											 PVOID				ProcessInformation,
											 ULONG				ProcessInformationLength,
											 PULONG				ReturnLength)
	= nullptr;
*/

BOOL WINAPI Mine_IsDebuggerPresent()
{
	DLog(L"Oops, somebody was trying to be naughty! (called IsDebuggerPresent)");
	return FALSE;
}

NTSTATUS WINAPI Mine_NtQueryInformationProcess(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength)
{
	NTSTATUS nRet;

	nRet = Real_NtQueryInformationProcess(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);

	if (ProcessInformationClass == ProcessBasicInformation) {
		PROCESS_BASIC_INFORMATION*		pbi = (PROCESS_BASIC_INFORMATION*)ProcessInformation;
		PEB_NT*							pPEB;
		PEB_NT							PEB;

		pPEB = (PEB_NT*)pbi->PebBaseAddress;
		ReadProcessMemory(ProcessHandle, pPEB, &PEB, sizeof(PEB), nullptr);
		PEB.BeingDebugged = 0;
		WriteProcessMemory(ProcessHandle, pPEB, &PEB, sizeof(PEB), nullptr);
	} else if (ProcessInformationClass == 7) { // ProcessDebugPort
		BOOL*		pDebugPort = (BOOL*)ProcessInformation;
		*pDebugPort = FALSE;
	}

	return nRet;
}

LONG WINAPI Mine_ChangeDisplaySettingsEx(LONG ret, DWORD dwFlags, LPVOID lParam)
{
	if (dwFlags&CDS_VIDEOPARAMETERS) {
		VIDEOPARAMETERS* vp = (VIDEOPARAMETERS*)lParam;

		if (vp->Guid == GUIDFromCString(L"{02C62061-1097-11d1-920F-00A024DF156E}")
				&& (vp->dwFlags&VP_FLAGS_COPYPROTECT)) {
			if (vp->dwCommand == VP_COMMAND_GET) {
				if ((vp->dwTVStandard&VP_TV_STANDARD_WIN_VGA)
						&& vp->dwTVStandard != VP_TV_STANDARD_WIN_VGA) {
					DLog(L"Ooops, tv-out enabled? macrovision checks suck...");
					vp->dwTVStandard = VP_TV_STANDARD_WIN_VGA;
				}
			} else if (vp->dwCommand == VP_COMMAND_SET) {
				DLog(L"Ooops, as I already told ya, no need for any macrovision bs here");
				return 0;
			}
		}
	}

	return ret;
}

LONG WINAPI Mine_ChangeDisplaySettingsExA(LPCSTR lpszDeviceName, LPDEVMODEA lpDevMode, HWND hwnd, DWORD dwFlags, LPVOID lParam)
{
	return Mine_ChangeDisplaySettingsEx(
			   Real_ChangeDisplaySettingsExA(lpszDeviceName, lpDevMode, hwnd, dwFlags, lParam),
			   dwFlags,
			   lParam);
}

LONG WINAPI Mine_ChangeDisplaySettingsExW(LPCWSTR lpszDeviceName, LPDEVMODEW lpDevMode, HWND hwnd, DWORD dwFlags, LPVOID lParam)
{
	return Mine_ChangeDisplaySettingsEx(
			   Real_ChangeDisplaySettingsExW(lpszDeviceName, lpDevMode, hwnd, dwFlags, lParam),
			   dwFlags,
			   lParam);
}

HANDLE WINAPI Mine_CreateFileA(LPCSTR p1, DWORD p2, DWORD p3, LPSECURITY_ATTRIBUTES p4, DWORD p5, DWORD p6, HANDLE p7)
{
	//CStringA fn(p1);
	//fn.MakeLower();
	//int i = fn.Find(".part");
	//if (i > 0 && i == fn.GetLength() - 5)
	p3 |= FILE_SHARE_WRITE;

	return Real_CreateFileA(p1, p2, p3, p4, p5, p6, p7);
}

BOOL CreateFakeVideoTS(LPCWSTR strIFOPath, LPWSTR strFakeFile, size_t nFakeFileSize)
{
	BOOL	bRet = FALSE;
	WCHAR	szTempPath[MAX_PATH];
	WCHAR	strFileName[_MAX_FNAME];
	WCHAR	strExt[_MAX_EXT];
	CIfo	Ifo;

	if (!GetTempPathW(MAX_PATH, szTempPath)) {
		return FALSE;
	}

	_wsplitpath_s(strIFOPath, nullptr, 0, nullptr, 0, strFileName, _countof(strFileName), strExt, _countof(strExt));
	_snwprintf_s(strFakeFile, nFakeFileSize, _TRUNCATE, L"%sMPC%s%s", szTempPath, strFileName, strExt);

	if (Ifo.OpenFile (strIFOPath) &&
			Ifo.RemoveUOPs()  &&
			Ifo.SaveFile (strFakeFile)) {
		bRet = TRUE;
	}

	return bRet;
}

HANDLE WINAPI Mine_CreateFileW(LPCWSTR p1, DWORD p2, DWORD p3, LPSECURITY_ATTRIBUTES p4, DWORD p5, DWORD p6, HANDLE p7)
{
	HANDLE	hFile = INVALID_HANDLE_VALUE;
	WCHAR	strFakeFile[MAX_PATH];
	int		nLen  = wcslen(p1);

	p3 |= FILE_SHARE_WRITE;

	if (nLen>=4 && _wcsicmp (p1 + nLen-4, L".ifo") == 0) {
		if (CreateFakeVideoTS(p1, strFakeFile, _countof(strFakeFile))) {
			hFile = Real_CreateFileW(strFakeFile, p2, p3, p4, p5, p6, p7);

			if (hFile != INVALID_HANDLE_VALUE) {
				CAppSettings& s = AfxGetAppSettings();
				CString tmpFile = CString(strFakeFile).MakeUpper();

				if (std::find(s.slTMPFilesList.cbegin(), s.slTMPFilesList.cend(), tmpFile) == s.slTMPFilesList.cend()) {
					s.slTMPFilesList.push_back(tmpFile);
				}
			}
		}
	}

	if (hFile == INVALID_HANDLE_VALUE) {
		hFile = Real_CreateFileW(p1, p2, p3, p4, p5, p6, p7);
	}

	return hFile;
}

MMRESULT WINAPI Mine_mixerSetControlDetails(HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails)
{
	if (fdwDetails == (MIXER_OBJECTF_HMIXER|MIXER_SETCONTROLDETAILSF_VALUE)) {
		return MMSYSERR_NOERROR;    // don't touch the mixer, kthx
	}
	return Real_mixerSetControlDetails(hmxobj, pmxcd, fdwDetails);
}

BOOL WINAPI Mine_DeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped)
{
	BOOL ret = Real_DeviceIoControl(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);

	if (IOCTL_DVD_GET_REGION == dwIoControlCode && lpOutBuffer
			&& lpBytesReturned && *lpBytesReturned == sizeof(DVD_REGION)) {
		DVD_REGION* pDVDRegion = (DVD_REGION*)lpOutBuffer;
		pDVDRegion->SystemRegion = ~pDVDRegion->RegionData;
	}

	return ret;
}

static BOOL SetHeapOptions()
{
	HMODULE hLib = LoadLibraryW(L"kernel32.dll");
	if (hLib == nullptr) {
		return FALSE;
	}

	typedef BOOL (WINAPI *HSI)(HANDLE, HEAP_INFORMATION_CLASS, PVOID, SIZE_T);
	HSI pHsi = (HSI)GetProcAddress(hLib,"HeapSetInformation");
	if (!pHsi) {
		FreeLibrary(hLib);
		return FALSE;
	}

#ifndef HeapEnableTerminationOnCorruption
	#define HeapEnableTerminationOnCorruption (HEAP_INFORMATION_CLASS)1
#endif

	const BOOL fRet = (pHsi)(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0);
	if (hLib) {
		FreeLibrary(hLib);
	}

	return fRet;
}

BOOL CMPlayerCApp::InitInstance()
{
#ifdef DEBUG
	DbgSetModuleLevel(LOG_TRACE, DWORD_MAX);
	DbgSetModuleLevel(LOG_ERROR, DWORD_MAX);
#endif

	// Remove the working directory from the search path to work around the DLL preloading vulnerability
	SetDllDirectory(L"");

	if (SetHeapOptions()) {
		DLog(L"Terminate on corruption enabled");
	} else {
		DLog(L"Terminate on corruption error = %u", GetLastError());
	}

	DetourRestoreAfterWith();
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetourAttach(&(PVOID&)Real_IsDebuggerPresent, (PVOID)Mine_IsDebuggerPresent);
	DetourAttach(&(PVOID&)Real_ChangeDisplaySettingsExA, (PVOID)Mine_ChangeDisplaySettingsExA);
	DetourAttach(&(PVOID&)Real_ChangeDisplaySettingsExW, (PVOID)Mine_ChangeDisplaySettingsExW);
	DetourAttach(&(PVOID&)Real_CreateFileA, (PVOID)Mine_CreateFileA);
	DetourAttach(&(PVOID&)Real_CreateFileW, (PVOID)Mine_CreateFileW);
	DetourAttach(&(PVOID&)Real_mixerSetControlDetails, (PVOID)Mine_mixerSetControlDetails);
	DetourAttach(&(PVOID&)Real_DeviceIoControl, (PVOID)Mine_DeviceIoControl);

	HMODULE hNTDLL = LoadLibraryW(L"ntdll.dll");

#ifndef _DEBUG	// Disable NtQueryInformationProcess in debug (prevent VS debugger to stop on crash address)
	if (hNTDLL) {
		Real_NtQueryInformationProcess = (FUNC_NTQUERYINFORMATIONPROCESS)GetProcAddress (hNTDLL, "NtQueryInformationProcess");

		if (Real_NtQueryInformationProcess) {
			DetourAttach(&(PVOID&)Real_NtQueryInformationProcess, (PVOID)Mine_NtQueryInformationProcess);
		}
	}
#endif

	CFilterMapper2::Init();

	const LONG lError = DetourTransactionCommit();
	ASSERT (lError == NOERROR);

	HRESULT hr;
	if (FAILED(hr = OleInitialize(0))) {
		AfxMessageBox(L"OleInitialize failed!");
		return FALSE;
	}

	WNDCLASS wndcls;
	memset(&wndcls, 0, sizeof(WNDCLASS));
	wndcls.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
	wndcls.lpfnWndProc = ::DefWindowProcW;
	wndcls.hInstance = AfxGetInstanceHandle();
	wndcls.hIcon = LoadIconW(IDR_MAINFRAME);
	wndcls.hCursor = LoadCursorW(IDC_ARROW);
	wndcls.hbrBackground = 0;//(HBRUSH)(COLOR_WINDOW + 1); // no bkg brush, the view and the bars should always fill the whole client area
	wndcls.lpszMenuName = nullptr;
	wndcls.lpszClassName = MPC_WND_CLASS_NAMEW;

	if (!AfxRegisterClass(&wndcls)) {
		AfxMessageBox(L"MainFrm class registration failed!");
		return FALSE;
	}

	if (!AfxSocketInit(nullptr)) {
		AfxMessageBox(L"AfxSocketInit failed!");
		return FALSE;
	}

	// select setting location
	if (m_Profile.IsIniValid()) {
		m_Profile.StoreSettingsToIni();
	} else {
		m_Profile.StoreSettingsToRegistry();
	}

	// process command line
	PreProcessCommandLine();
	m_s.ParseCommandLine(m_cmdln);

	if (m_s.nCLSwitches & (CLSW_HELP | CLSW_UNRECOGNIZEDSWITCH)) { // show comandline help window
		m_s.LoadSettings();
		ShowCmdlnSwitches();
		return FALSE;
	}

	if (m_s.nCLSwitches & CLSW_RESET) { // reset settings
		if (!ClearSettings()) {
			return FALSE;
		}
	}

	if ((m_s.nCLSwitches & CLSW_CLOSE) && m_s.slFiles.empty()) { // "/close" switch and empty file list
		return FALSE;
	}

	if (m_s.nCLSwitches & (CLSW_REGEXTVID | CLSW_REGEXTAUD | CLSW_REGEXTPL)) { // register file types
		if (!IsUserAdmin()) {
			AfxGetMyApp()->RunAsAdministrator(GetProgramPath(), m_lpCmdLine, false);
		} else {

			CPPageFormats::RegisterApp();

			const bool bIs64 = SysVersion::IsW64();
			CPPageFormats::UnRegisterShellExt(ShellExt);
			if (bIs64) {
				CPPageFormats::UnRegisterShellExt(ShellExt64);
			}

			CMediaFormats& mf = m_s.m_Formats;
			mf.UpdateData(false);

			for (auto& mfc : mf) {
				filetype_t filetype = mfc.GetFileType();

				if ((m_s.nCLSwitches & CLSW_REGEXTVID) && filetype == TVideo
						|| (m_s.nCLSwitches & CLSW_REGEXTAUD) && filetype == TAudio
						|| (m_s.nCLSwitches & CLSW_REGEXTPL) && filetype == TPlaylist) {
					int j = 0;
					CString str = mfc.GetExtsWithPeriod();
					for (CString ext = str.Tokenize(L" ", j); !ext.IsEmpty(); ext = str.Tokenize(L" ", j)) {
						CPPageFormats::RegisterExt(ext, mfc.GetDescription(), filetype);
					}
				}
			}

			CPPageFormats::RegisterShellExt(ShellExt);
			if (bIs64) {
				CPPageFormats::RegisterShellExt(ShellExt64);
			}

			if (SysVersion::IsWin8orLater()) {
				CPPageFormats::RegisterUI();
			}

			SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
		}
		return FALSE;
	}

	if (m_s.nCLSwitches & CLSW_UNREGEXT) { // unregistered file types
		if (!IsUserAdmin()) {
			AfxGetMyApp()->RunAsAdministrator(GetProgramPath(), m_lpCmdLine, false);
		} else {
			CPPageFormats::UnRegisterShellExt(ShellExt);
			if (SysVersion::IsW64()) {
				CPPageFormats::UnRegisterShellExt(ShellExt64);
			}

			CMediaFormats& mf = m_s.m_Formats;
			mf.UpdateData(false);

			for (auto& mfc : mf) {
				int j = 0;
				const CString str = mfc.GetExtsWithPeriod();
				for (CString ext = str.Tokenize(L" ", j); !ext.IsEmpty(); ext = str.Tokenize(L" ", j)) {
					CPPageFormats::UnRegisterExt(ext);
				}
			}

			if (SysVersion::IsWin8orLater()) {
				CPPageFormats::RegisterUI();
			}

			SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
		}

		return FALSE;
	}

	// Enable to open options with administrator privilege (for Vista UAC)
	if (m_s.nCLSwitches & CLSW_ADMINOPTION) {
		m_s.LoadSettings(); // read all settings. long time but not critical at this point

		switch (m_s.iAdminOption) {
			case CPPageFormats::IDD : {
				CPPageSheet options(ResStr(IDS_OPTIONS_CAPTION), nullptr, m_s.iAdminOption);
				options.LockPage();
				options.DoModal();
			}
			break;

			default :
				ASSERT (FALSE);
		}
		return FALSE;
	}

	m_mutexOneInstance.Create(nullptr, TRUE, MPC_WND_CLASS_NAMEW);

	if (GetLastError() == ERROR_ALREADY_EXISTS
			&& !(m_s.nCLSwitches & CLSW_NEW)
			&& (m_s.nCLSwitches & CLSW_ADD ||
				(m_s.GetMultiInst() == 1 && !m_cmdln.empty()) ||
				m_s.GetMultiInst() == 0)) {

		const DWORD result = WaitForSingleObject(m_mutexOneInstance.m_h, 5000);
		if (result == WAIT_OBJECT_0 || result == WAIT_ABANDONED) {
			const HWND hWnd = ::FindWindowW(MPC_WND_CLASS_NAMEW, nullptr);
			if (hWnd) {
				DWORD dwProcessId = 0;
				if (GetWindowThreadProcessId(hWnd, &dwProcessId) && dwProcessId) {
					VERIFY(AllowSetForegroundWindow(dwProcessId));
				} else {
					ASSERT(FALSE);
				}

				if (!(m_s.nCLSwitches & CLSW_MINIMIZED) && IsIconic(hWnd)) {
					ShowWindow(hWnd, SW_RESTORE);
				}

				BOOL bDataIsSent = TRUE;

				std::thread sendThread = std::thread([this, hWnd] { SendCommandLine(hWnd); });
				if (WaitForSingleObject(sendThread.native_handle(), 5000) == WAIT_TIMEOUT) { // 5 seconds should be enough to send data
					bDataIsSent = FALSE;
					sendThread.detach();
				}

				if (sendThread.joinable()) {
					sendThread.join();
				}

				if (bDataIsSent) {
					m_mutexOneInstance.Close();
					return FALSE;
				}
			}
		}
	}

	// read settings
	m_s.LoadSettings();

	if (!__super::InitInstance()) {
		AfxMessageBox(L"InitInstance failed!");
		return FALSE;
	}

	CString shaderpath;
	GetAppSavePath(shaderpath);
	shaderpath.Append(L"Shaders");
	if (!::PathFileExistsW(shaderpath)) {
		// restore shaders if the "Shaders" folder is missing only
		CString shaderstorage;
		SHGetFolderPathW(nullptr, CSIDL_COMMON_APPDATA, nullptr, 0, shaderstorage.GetBuffer(MAX_PATH));
		shaderstorage.ReleaseBuffer();
		shaderstorage.Append(L"\\MPC-BE\\Shaders");

		if (::PathFileExistsW(shaderstorage)) {
			WCHAR pathFrom[MAX_PATH] = { 0 }; // for double null-terminated string
			wcscpy(pathFrom, shaderstorage);

			WCHAR pathTo[MAX_PATH] = { 0 }; // for double null-terminated string
			wcscpy(pathTo, shaderpath);

			SHFILEOPSTRUCTW sf = { 0 };
			sf.wFunc = FO_COPY;
			sf.hwnd = 0;
			sf.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_NOERRORUI;
			sf.pFrom = pathFrom;
			sf.pTo = pathTo;
			int ret = SHFileOperationW(&sf);
			if (ret == 0) {
				DLog(L"CMPlayerCApp::InitInstance(): default shaders restored");
			} else {
				DLog(L"CMPlayerCApp::InitInstance(): default shaders are not copied. Error: %x", ret);
			}
		}
	}

	if (!m_Profile.IsIniValid()) {
		CRegKey key;
		if (ERROR_SUCCESS == key.Create(HKEY_LOCAL_MACHINE, L"Software\\MPC-BE")) {
			CString path = GetProgramPath();
			key.SetStringValue(L"ExePath", path);
		}
	}

	AfxEnableControlContainer();

	CMainFrame* pFrame = DNew CMainFrame;
	m_pMainWnd = pFrame;
	if (!pFrame->LoadFrame(IDR_MAINFRAME, WS_OVERLAPPEDWINDOW|FWS_ADDTOTITLE, nullptr, nullptr)) {
		AfxMessageBox(L"CMainFrame::LoadFrame failed!");
		return FALSE;
	}
	pFrame->RestoreControlBars();
	pFrame->SetDefaultWindowRect((m_s.nCLSwitches & CLSW_MONITOR) ? m_s.iMonitor : 0);
	pFrame->SetDefaultFullscreenState();
	pFrame->SetIcon(AfxGetApp()->LoadIconW(IDR_MAINFRAME), TRUE);
	pFrame->DragAcceptFiles();
	pFrame->ShowWindow((m_s.nCLSwitches & CLSW_MINIMIZED) ? SW_SHOWMINIMIZED : SW_SHOW);
	pFrame->UpdateWindow();
	pFrame->m_hAccelTable = m_s.hAccel;
	m_s.WinLircClient.SetHWND(m_pMainWnd->m_hWnd);
	if (m_s.bWinLirc) {
		m_s.WinLircClient.Connect(m_s.strWinLircAddr);
	}
	m_s.UIceClient.SetHWND(m_pMainWnd->m_hWnd);
	if (m_s.bUIce) {
		m_s.UIceClient.Connect(m_s.strUIceAddr);
	}

	if (m_s.bUpdaterAutoCheck) {
		if (UpdateChecker::IsTimeToAutoUpdate(m_s.nUpdaterDelay, m_s.tUpdaterLastCheck)) {
			UpdateChecker::CheckForUpdate(true);
		}
	}

	SendCommandLine(m_pMainWnd->m_hWnd);
	RegisterHotkeys();

	pFrame->SetFocus();

	// set HIGH I/O Priority for better playback perfomance
	if (hNTDLL) {
		typedef NTSTATUS (WINAPI *FUNC_NTSETINFORMATIONPROCESS)(HANDLE, ULONG, PVOID, ULONG);
		FUNC_NTSETINFORMATIONPROCESS NtSetInformationProcess = (FUNC_NTSETINFORMATIONPROCESS)GetProcAddress(hNTDLL, "NtSetInformationProcess");

		if (NtSetInformationProcess && SetPrivilege(SE_INC_BASE_PRIORITY_NAME)) {
			ULONG IoPriority = 3;
			ULONG ProcessIoPriority = 0x21;
			NTSTATUS NtStatus = NtSetInformationProcess(GetCurrentProcess(), ProcessIoPriority, &IoPriority, sizeof(ULONG));
			DLog(L"Set I/O Priority - %d", NtStatus);
		}

		FreeLibrary(hNTDLL);
		hNTDLL = nullptr;
	}

	m_mutexOneInstance.Release();

	return TRUE;
}

UINT CMPlayerCApp::GetRemoteControlCodeMicrosoft(UINT nInputcode, HRAWINPUT hRawInput)
{
	UINT	dwSize		= 0;
	BYTE*	pRawBuffer	= nullptr;
	UINT	nMceCmd		= 0;

	// Support for MCE remote control
	GetRawInputData(hRawInput, RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));
	if (dwSize > 0) {
		pRawBuffer = DNew BYTE[dwSize];
		if (GetRawInputData(hRawInput, RID_INPUT, pRawBuffer, &dwSize, sizeof(RAWINPUTHEADER)) != -1) {
			RAWINPUT*	raw = (RAWINPUT*) pRawBuffer;
			if (raw->header.dwType == RIM_TYPEHID) {
				nMceCmd = 0x10000 + (raw->data.hid.bRawData[1] | raw->data.hid.bRawData[2] << 8);
			}
		}
		delete [] pRawBuffer;
	}

	return nMceCmd;
}

UINT CMPlayerCApp::GetRemoteControlCodeSRM7500(UINT nInputcode, HRAWINPUT hRawInput)
{
	UINT	dwSize		= 0;
	BYTE*	pRawBuffer	= nullptr;
	UINT	nMceCmd		= 0;

	GetRawInputData(hRawInput, RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));
	if (dwSize > 21) {
		pRawBuffer = DNew BYTE[dwSize];
		if (GetRawInputData(hRawInput, RID_INPUT, pRawBuffer, &dwSize, sizeof(RAWINPUTHEADER)) != -1) {
			RAWINPUT*	raw = (RAWINPUT*) pRawBuffer;

			// data.hid.bRawData[21] set to one when key is pressed
			if (raw->header.dwType == RIM_TYPEHID && raw->data.hid.bRawData[21] == 1) {
				// data.hid.bRawData[21] has keycode
				switch (raw->data.hid.bRawData[20]) {
					case 0x0033 :
						nMceCmd = MCE_DETAILS;
						break;
					case 0x0022 :
						nMceCmd = MCE_GUIDE;
						break;
					case 0x0036 :
						nMceCmd = MCE_MYTV;
						break;
					case 0x0026 :
						nMceCmd = MCE_RECORDEDTV;
						break;
					case 0x0005 :
						nMceCmd = MCE_RED;
						break;
					case 0x0002 :
						nMceCmd = MCE_GREEN;
						break;
					case 0x0045 :
						nMceCmd = MCE_YELLOW;
						break;
					case 0x0046 :
						nMceCmd = MCE_BLUE;
						break;
					case 0x000A :
						nMceCmd = MCE_MEDIA_PREVIOUSTRACK;
						break;
					case 0x004A :
						nMceCmd = MCE_MEDIA_NEXTTRACK;
						break;
				}
			}
		}
		delete [] pRawBuffer;
	}

	return nMceCmd;
}

void CMPlayerCApp::RegisterHotkeys()
{
	CAutoVectorPtr<RAWINPUTDEVICELIST> inputDeviceList;
	UINT nInputDeviceCount = 0, nErrCode;
	RID_DEVICE_INFO deviceInfo;
	RAWINPUTDEVICE MCEInputDevice[] = {
		// usUsagePage  usUsage  dwFlags  hwndTarget
		{  0xFFBC,      0x88,    0,       nullptr},
		{  0x000C,      0x01,    0,       nullptr},
		{  0x000C,      0x80,    0,       nullptr}
	};

	// Register MCE Remote Control raw input
	for (unsigned int i=0; i<_countof(MCEInputDevice); i++) {
		MCEInputDevice[i].hwndTarget = m_pMainWnd->m_hWnd;
	}

	// Get the size of the device list
	nErrCode = GetRawInputDeviceList(nullptr, &nInputDeviceCount, sizeof(RAWINPUTDEVICELIST));
	inputDeviceList.Attach(new RAWINPUTDEVICELIST[nInputDeviceCount]);
	if (nErrCode == UINT(-1) || !nInputDeviceCount || !inputDeviceList) {
		ASSERT(nErrCode != UINT(-1));
		return;
	}

	nErrCode = GetRawInputDeviceList(inputDeviceList, &nInputDeviceCount, sizeof(RAWINPUTDEVICELIST));
	if (nErrCode == UINT(-1)) {
		ASSERT(FALSE);
		return;
	}

	for (UINT i=0; i<nInputDeviceCount; i++) {
		UINT nTemp = deviceInfo.cbSize = sizeof(deviceInfo);

		if (GetRawInputDeviceInfoW(inputDeviceList[i].hDevice, RIDI_DEVICEINFO, &deviceInfo, &nTemp) > 0) {
			if (deviceInfo.hid.dwVendorId == 0x00000471 &&  // Philips HID vendor id
				deviceInfo.hid.dwProductId == 0x00000617) { // IEEE802.15.4 RF Dongle (SRM 7500)
				MCEInputDevice[0].usUsagePage = deviceInfo.hid.usUsagePage;
				MCEInputDevice[0].usUsage = deviceInfo.hid.usUsage;
				GetRemoteControlCode = GetRemoteControlCodeSRM7500;
			}
		}
	}

	RegisterRawInputDevices(MCEInputDevice, _countof(MCEInputDevice), sizeof(RAWINPUTDEVICE));

	if (m_s.bGlobalMedia) {
		for (const auto& wc : m_s.wmcmds) {
			if (wc.appcmd != 0) {
				RegisterHotKey(m_pMainWnd->m_hWnd, wc.appcmd, 0, GetVKFromAppCommand (wc.appcmd));
			}
		}
	}
}

void CMPlayerCApp::UnregisterHotkeys()
{
	if (m_s.bGlobalMedia) {
		for (const auto& wc : m_s.wmcmds) {
			if (wc.appcmd != 0) {
				UnregisterHotKey(m_pMainWnd->m_hWnd, wc.appcmd);
			}
		}
	}
}

UINT CMPlayerCApp::GetVKFromAppCommand(UINT nAppCommand)
{
	switch (nAppCommand) {
		case APPCOMMAND_BROWSER_BACKWARD	:
			return VK_BROWSER_BACK;
		case APPCOMMAND_BROWSER_FORWARD		:
			return VK_BROWSER_FORWARD;
		case APPCOMMAND_BROWSER_REFRESH		:
			return VK_BROWSER_REFRESH;
		case APPCOMMAND_BROWSER_STOP		:
			return VK_BROWSER_STOP;
		case APPCOMMAND_BROWSER_SEARCH		:
			return VK_BROWSER_SEARCH;
		case APPCOMMAND_BROWSER_FAVORITES	:
			return VK_BROWSER_FAVORITES;
		case APPCOMMAND_BROWSER_HOME		:
			return VK_BROWSER_HOME;
		case APPCOMMAND_VOLUME_MUTE			:
			return VK_VOLUME_MUTE;
		case APPCOMMAND_VOLUME_DOWN			:
			return VK_VOLUME_DOWN;
		case APPCOMMAND_VOLUME_UP			:
			return VK_VOLUME_UP;
		case APPCOMMAND_MEDIA_NEXTTRACK		:
			return VK_MEDIA_NEXT_TRACK;
		case APPCOMMAND_MEDIA_PREVIOUSTRACK	:
			return VK_MEDIA_PREV_TRACK;
		case APPCOMMAND_MEDIA_STOP			:
			return VK_MEDIA_STOP;
		case APPCOMMAND_MEDIA_PLAY_PAUSE	:
			return VK_MEDIA_PLAY_PAUSE;
		case APPCOMMAND_LAUNCH_MAIL			:
			return VK_LAUNCH_MAIL;
		case APPCOMMAND_LAUNCH_MEDIA_SELECT	:
			return VK_LAUNCH_MEDIA_SELECT;
		case APPCOMMAND_LAUNCH_APP1			:
			return VK_LAUNCH_APP1;
		case APPCOMMAND_LAUNCH_APP2			:
			return VK_LAUNCH_APP2;
	}

	return 0;
}

int CMPlayerCApp::ExitInstance()
{
	if (m_s.bResetSettings) {
		ClearSettings();
		ShellExecuteW(nullptr, L"open", GetProgramPath(), nullptr, nullptr, SW_SHOWNORMAL);
	} else {
		m_s.SaveSettings();
	}

	OleUninitialize();

	return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// CMPlayerCApp message handlers
// App command to run the dialog

void CMPlayerCApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

void CMPlayerCApp::OnFileExit()
{
	OnAppExit();
}

// CRemoteCtrlClient

CRemoteCtrlClient::CRemoteCtrlClient()
	: m_pWnd(nullptr)
	, m_nStatus(DISCONNECTED)
{
}

void CRemoteCtrlClient::SetHWND(HWND hWnd)
{
	CAutoLock cAutoLock(&m_csLock);

	m_pWnd = CWnd::FromHandle(hWnd);
}

void CRemoteCtrlClient::Connect(CString addr)
{
	CAutoLock cAutoLock(&m_csLock);

	if (m_nStatus == CONNECTING && m_addr == addr) {
		DLog(L"CRemoteCtrlClient (Connect): already connecting to %s", addr);
		return;
	}

	if (m_nStatus == CONNECTED && m_addr == addr) {
		DLog(L"CRemoteCtrlClient (Connect): already connected to %s", addr);
		return;
	}

	m_nStatus = CONNECTING;

	DLog(L"CRemoteCtrlClient (Connect): connecting to %s", addr);

	Close();

	Create();

	CString ip = addr.Left(addr.Find(':')+1).TrimRight(':');
	int port = wcstol(addr.Mid(addr.Find(':')+1), nullptr, 10);

	__super::Connect(ip, port);

	m_addr = addr;
}

void CRemoteCtrlClient::DisConnect()
{
	CAutoLock cAutoLock(&m_csLock);

	ShutDown(2);
	Close();
}

void CRemoteCtrlClient::OnConnect(int nErrorCode)
{
	CAutoLock cAutoLock(&m_csLock);

	m_nStatus = (nErrorCode == 0 ? CONNECTED : DISCONNECTED);

	DLog(L"CRemoteCtrlClient (OnConnect): %d", nErrorCode);
}

void CRemoteCtrlClient::OnClose(int nErrorCode)
{
	CAutoLock cAutoLock(&m_csLock);

	if (m_hSocket != INVALID_SOCKET && m_nStatus == CONNECTED) {
		DLog(L"CRemoteCtrlClient (OnClose): connection lost");
	}

	m_nStatus = DISCONNECTED;

	DLog(L"CRemoteCtrlClient (OnClose): %d", nErrorCode);
}

void CRemoteCtrlClient::OnReceive(int nErrorCode)
{
	if (nErrorCode != 0 || !m_pWnd) {
		return;
	}

	CStringA str;
	int ret = Receive(str.GetBuffer(256), 255, 0);
	if (ret <= 0) {
		return;
	}
	str.ReleaseBuffer(ret);

	DLog(L"CRemoteCtrlClient (OnReceive): %s", CString(str));

	OnCommand(str);

	__super::OnReceive(nErrorCode);
}

void CRemoteCtrlClient::ExecuteCommand(CStringA cmd, int repcnt)
{
	cmd.Trim();
	if (cmd.IsEmpty()) {
		return;
	}
	cmd.Replace(' ', '_');

	CAppSettings& s = AfxGetAppSettings();

	for (const auto& wc : s.wmcmds) {
		CStringA name = TToA(wc.GetName());
		name.Replace(' ', '_');
		if ((repcnt == 0 && wc.rmrepcnt == 0 || wc.rmrepcnt > 0 && (repcnt%wc.rmrepcnt) == 0)
				&& (!name.CompareNoCase(cmd) || !wc.rmcmd.CompareNoCase(cmd) || wc.cmd == (WORD)strtol(cmd, nullptr, 10))) {
			CAutoLock cAutoLock(&m_csLock);
			DLog(L"CRemoteCtrlClient (calling command): %s", wc.GetName());
			m_pWnd->SendMessageW(WM_COMMAND, wc.cmd);
			break;
		}
	}
}

// CWinLircClient

CWinLircClient::CWinLircClient()
{
}

void CWinLircClient::OnCommand(CStringA str)
{
	DLog(L"CWinLircClient (OnCommand): %s", CString(str));

	int i = 0, j = 0, repcnt = 0;
	for (CStringA token = str.Tokenize(" ", i);
			!token.IsEmpty();
			token = str.Tokenize(" ", i), j++) {
		if (j == 1) {
			repcnt = strtol(token, nullptr, 16);
		} else if (j == 2) {
			ExecuteCommand(token, repcnt);
		}
	}
}

// CUIceClient

CUIceClient::CUIceClient()
{
}

void CUIceClient::OnCommand(CStringA str)
{
	DLog(L"CUIceClient (OnCommand): %s", CString(str));

	CStringA cmd;
	int i = 0, j = 0;
	for (CStringA token = str.Tokenize("|", i);
			!token.IsEmpty();
			token = str.Tokenize("|", i), j++) {
		if (j == 0) {
			cmd = token;
		} else if (j == 1) {
			ExecuteCommand(cmd, strtol(token, nullptr, 16));
		}
	}
}

void CMPlayerCApp::OnHelpShowcommandlineswitches()
{
	ShowCmdlnSwitches();
}

LRESULT CALLBACK RTLWindowsLayoutCbtFilterHook(int code, WPARAM wParam, LPARAM lParam)
{
	if (code == HCBT_CREATEWND)
	{
		//LPCREATESTRUCT lpcs = ((LPCBT_CREATEWND)lParam)->lpcs;

		//if ((lpcs->style & WS_CHILD) == 0)
		//	lpcs->dwExStyle |= WS_EX_LAYOUTRTL;	// doesn't seem to have any effect, but shouldn't hurt

		HWND hWnd = (HWND)wParam;
		if ((GetWindowLongPtrW(hWnd, GWL_STYLE) & WS_CHILD) == 0) {
			SetWindowLongPtrW(hWnd, GWL_EXSTYLE, GetWindowLongPtrW(hWnd, GWL_EXSTYLE) | WS_EX_LAYOUTRTL);
		}
	}
	return CallNextHookEx(nullptr, code, wParam, lParam);
}

CString CMPlayerCApp::GetSatelliteDll(int nLanguage)
{
	CString path = GetProgramDir();

	if (nLanguage < 0 || nLanguage >= languageResourcesCount || languageResources[nLanguage].resourceID == ID_LANGUAGE_ENGLISH) {
		path.Empty();
	} else {
		path.AppendFormat(L"Lang\\mpcresources.%s.dll", languageResources[nLanguage].strcode);
	}

	return path;
}

int CMPlayerCApp::GetLanguageIndex(UINT resID)
{
	for (size_t i = 0; i < languageResourcesCount; i++) {
		if (resID == languageResources[i].resourceID) {
			return i;
		}
	}
	return -1;
}

int CMPlayerCApp::GetLanguageIndex(CString langcode)
{
	for (size_t i = 0; i < languageResourcesCount; i++) {
		if (langcode == languageResources[i].strcode) {
			return i;
		}
	}
	return -1;
}

int CMPlayerCApp::GetDefLanguage()
{
	const LANGID userlangID = GetUserDefaultUILanguage();
	for (size_t i = 0; i < languageResourcesCount; i++) {
		if (userlangID == languageResources[i].localeID) {
			return (int)i;
		}
	}
	return GetLanguageIndex(ID_LANGUAGE_ENGLISH);
}

void CMPlayerCApp::SetLanguage(int nLanguage, bool bSave/* = true*/)
{
	CAppSettings& s = AfxGetAppSettings();
	HMODULE hMod    = nullptr;

	const CString strSatellite = GetSatelliteDll(nLanguage);
	if (!strSatellite.IsEmpty()) {
		const FileVersion::Ver SatVersion = FileVersion::GetVer(strSatellite);
		if (SatVersion.value != 0) {
			if (SatVersion.value == FileVersion::Ver(MPC_VERSION_SVN_NUM).value) {
				hMod = LoadLibraryW(strSatellite);
				if (bSave) {
					s.iLanguage = nLanguage;
				}
			} else {
				// This message should stay in English!
				MessageBoxW(nullptr, L"Your language pack will not work with this version. Please download a compatible one from the MPC-BE homepage.",
					L"MPC-BE", MB_OK);
			}
		}
	} else if (bSave && nLanguage == GetLanguageIndex(ID_LANGUAGE_ENGLISH)) {
		s.iLanguage = nLanguage;
	}

	// In case no dll was loaded, load the English translation from the executable
	if (hMod == nullptr) {
		hMod = AfxGetApp()->m_hInstance;
	}
	// In case a dll was loaded, check if some special action is needed
	else if (nLanguage == GetLanguageIndex(ID_LANGUAGE_HEBREW)) {
		// Hebrew needs the RTL flag.
		SetProcessDefaultLayout(LAYOUT_RTL);
		SetWindowsHookExW(WH_CBT, RTLWindowsLayoutCbtFilterHook, nullptr, GetCurrentThreadId());
	}

	// Free the old resource if it was a dll
	if (AfxGetResourceHandle() != AfxGetApp()->m_hInstance) {
		FreeLibrary(AfxGetResourceHandle());
	}

	// Set the new resource
	AfxSetResourceHandle(hMod);
}

void CMPlayerCApp::RunAsAdministrator(LPCTSTR strCommand, LPCTSTR strArgs, bool bWaitProcess)
{
	SHELLEXECUTEINFOW execinfo;
	memset(&execinfo, 0, sizeof(execinfo));
	execinfo.lpFile			= strCommand;
	execinfo.cbSize			= sizeof(execinfo);
	execinfo.lpVerb			= L"runas";
	execinfo.fMask			= SEE_MASK_NOCLOSEPROCESS;
	execinfo.nShow			= SW_SHOWDEFAULT;
	execinfo.lpParameters	= strArgs;

	ShellExecuteExW(&execinfo);

	if (bWaitProcess) {
		WaitForSingleObject(execinfo.hProcess, INFINITE);
	}
}

CRenderersSettings& GetRenderersSettings()
{
	return AfxGetAppSettings().m_VRSettings;
}

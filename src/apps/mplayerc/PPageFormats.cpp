/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2015 see Authors.txt
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
#include "PPageFormats.h"
#include "../../DSUtil/FileHandle.h"
#include "../../DSUtil/SysVersion.h"
#include "../../DSUtil/WinAPIUtils.h"
#include <psapi.h>
#include <string>
#include <atlimage.h>
#include <HighDPI.h>


// CPPageFormats dialog

static BOOL ShellExtExists()
{
	if (IsW64()) {
		return ::PathFileExists(ShellExt64);
	}

	return ::PathFileExists(ShellExt);
}

CComPtr<IApplicationAssociationRegistration> CPPageFormats::m_pAAR;

// TODO: change this along with the root key for settings and the mutex name to
//       avoid possible risks of conflict with the old MPC (non BE version).
#ifdef _WIN64
	#define PROGID _T("mpc-be64")
#else
	#define PROGID _T("mpc-be")
#endif // _WIN64

IMPLEMENT_DYNAMIC(CPPageFormats, CPPageBase)
CPPageFormats::CPPageFormats()
	: CPPageBase(CPPageFormats::IDD, CPPageFormats::IDD)
	, m_list(0)
	, m_bInsufficientPrivileges(false)
{
	if (!m_pAAR && IsWinVistaOrLater() && !IsWin10orLater()) {
		// Default manager (requires at least Vista)
		HRESULT hr = CoCreateInstance(CLSID_ApplicationAssociationRegistration,
									  NULL,
									  CLSCTX_INPROC,
									  IID_PPV_ARGS(&m_pAAR));
		UNREFERENCED_PARAMETER(hr);
	}
}

CPPageFormats::~CPPageFormats()
{
}

void CPPageFormats::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list);
	DDX_Text(pDX, IDC_EDIT1, m_exts);
	DDX_Control(pDX, IDC_STATIC1, m_autoplay);
	DDX_Control(pDX, IDC_CHECK1, m_apvideo);
	DDX_Control(pDX, IDC_CHECK2, m_apmusic);
	DDX_Control(pDX, IDC_CHECK3, m_apaudiocd);
	DDX_Control(pDX, IDC_CHECK4, m_apdvd);
	DDX_Control(pDX, IDC_CHECK6, m_fContextDir);
	DDX_Control(pDX, IDC_CHECK7, m_fContextFiles);
	DDX_Control(pDX, IDC_CHECK8, m_fAssociatedWithIcons);
}

int CPPageFormats::GetChecked(int iItem)
{
	LVITEM lvi;
	lvi.iItem = iItem;
	lvi.iSubItem = 0;
	lvi.mask = LVIF_IMAGE;
	m_list.GetItem(&lvi);
	return(lvi.iImage);
}

void CPPageFormats::SetChecked(int iItem, int iChecked)
{
	LVITEM lvi;
	lvi.iItem = iItem;
	lvi.iSubItem = 0;
	lvi.mask = LVIF_IMAGE;
	lvi.iImage = iChecked;
	m_list.SetItem(&lvi);
}

CString CPPageFormats::GetEnqueueCommand()
{
	return _T("\"") + GetProgramPath() + _T("\" /add \"%1\"");
}

CString CPPageFormats::GetOpenCommand()
{
	return _T("\"") + GetProgramPath() + _T("\" \"%1\"");
}

bool CPPageFormats::IsRegistered(CString ext, bool bCheckProgId/* = false*/)
{
	BOOL    bIsDefault = FALSE;
	CString strProgID  = PROGID + ext;

	if (m_pAAR) {
		// The Vista/7/8 way
		m_pAAR->QueryAppIsDefault(ext, AT_FILEEXTENSION, AL_EFFECTIVE, GetRegisteredAppName(), &bIsDefault);
	} else {
		// The 2000/XP/10 way
		CRegKey key;
		TCHAR   buff[_MAX_PATH] = { 0 };
		ULONG   len = _countof(buff);

		if (ERROR_SUCCESS != key.Open(HKEY_CLASSES_ROOT, ext, KEY_READ)) {
			return false;
		}

		if (ERROR_SUCCESS != key.QueryStringValue(NULL, buff, &len) && !CString(buff).Trim().IsEmpty()) {
			return false;
		}

		bIsDefault = (buff == strProgID);
	}

	// Check if association is for this instance of MPC
	if (bIsDefault) {
		CRegKey key;
		TCHAR   buff[_MAX_PATH] = { 0 };
		ULONG   len = _countof(buff);

		bIsDefault = FALSE;
		if (ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, strProgID + _T("\\shell\\open\\command"), KEY_READ)) {
			CString strCommand = GetOpenCommand();
			if (ERROR_SUCCESS == key.QueryStringValue(NULL, buff, &len)) {
				bIsDefault = (strCommand.CompareNoCase(CString(buff)) == 0);
			}
		}
	}

	if (bIsDefault && IsWin10orLater() && bCheckProgId) {
		CRegKey key;
		TCHAR   buff[_MAX_PATH] = { 0 };
		ULONG   len = _countof(buff);

		if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, CString(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\" + ext + L"\\UserChoice"), KEY_READ)) {
			if (ERROR_SUCCESS == key.QueryStringValue(L"ProgId", buff, &len)) {
				bIsDefault = (strProgID.CompareNoCase(CString(buff)) == 0);
			}
		}
	}

	return !!bIsDefault;
}

typedef int (*GetIconIndexFunc)(LPCTSTR);
static int GetIconIndex(LPCTSTR ext)
{
	int iconindex = -1;

	static HMODULE mpciconlib = LoadLibrary(L"mpciconlib.dll");
	if (mpciconlib) {
		static GetIconIndexFunc pGetIconIndexFunc = (GetIconIndexFunc)GetProcAddress(mpciconlib, "get_icon_index");
		if (pGetIconIndexFunc) {
			iconindex = pGetIconIndexFunc(ext);
		}
	}

	return iconindex;
}

bool CPPageFormats::RegisterApp()
{
	if (IsWinVistaOrLater()) {
		CString AppIcon;

		AppIcon = GetProgramPath();
		AppIcon = "\""+AppIcon+"\"";
		AppIcon += _T(",0");

		// Register MPC for the windows "Default application" manager
		CRegKey key;

		if (ERROR_SUCCESS == key.Open(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\RegisteredApplications"))) {
			key.SetStringValue(_T("MPC-BE"), GetRegisteredKey());

			if (ERROR_SUCCESS != key.Create(HKEY_LOCAL_MACHINE, GetRegisteredKey())) {
				return false;
			}

			// ==>>  TODO icon !!!
			key.SetStringValue(_T("ApplicationDescription"), ResStr(IDS_APP_DESCRIPTION), REG_EXPAND_SZ);
			key.SetStringValue(_T("ApplicationIcon"), AppIcon, REG_EXPAND_SZ);
			key.SetStringValue(_T("ApplicationName"), ResStr(IDR_MAINFRAME), REG_EXPAND_SZ);
		}
	}

	return true;
}

bool CPPageFormats::RegisterExt(CString ext, CString strLabel, filetype_t filetype, bool SetContextFiles/* = false*/, bool setAssociatedWithIcon/* = true*/)
{
	CRegKey key;
	CString strProgID = PROGID + ext;

	bool bSetValue = true || (ERROR_SUCCESS != key.Open(HKEY_CLASSES_ROOT, strProgID + _T("\\shell\\open\\command"), KEY_READ));
	// Why bSetValue?

	// Create ProgID for this file type
	if (ERROR_SUCCESS != key.Create(HKEY_CLASSES_ROOT, strProgID)) {
		return false;
	}
	if (ERROR_SUCCESS != key.SetStringValue(NULL, strLabel)) {
		return false;
	}

	// Add to playlist option
	if (SetContextFiles && !ShellExtExists()) {
		if (ERROR_SUCCESS != key.Create(HKEY_CLASSES_ROOT, strProgID + _T("\\shell\\enqueue"))) {
			return false;
		}
		if (ERROR_SUCCESS != key.SetStringValue(NULL, ResStr(IDS_ADD_TO_PLAYLIST))) {
			return false;
		}
		if (ERROR_SUCCESS != key.SetStringValue(_T("Icon"), _T("\"") + GetProgramPath() + _T("\",0"))) {
			return false;
		}

		if (ERROR_SUCCESS != key.Create(HKEY_CLASSES_ROOT, strProgID + _T("\\shell\\enqueue\\command"))) {
			return false;
		}
		if (bSetValue && (ERROR_SUCCESS != key.SetStringValue(NULL, GetEnqueueCommand()))) {
			return false;
		}
	} else {
		key.Close();
		key.Attach(HKEY_CLASSES_ROOT);
		key.RecurseDeleteKey(strProgID + _T("\\shell\\enqueue"));
	}

	// Play option
	if (ERROR_SUCCESS != key.Create(HKEY_CLASSES_ROOT, strProgID + _T("\\shell\\open"))) {
		return false;
	}
	if (SetContextFiles && !ShellExtExists()) {
		if (ERROR_SUCCESS != key.SetStringValue(NULL, ResStr(IDS_OPEN_WITH_MPC))) {
			return false;
		}
		if (ERROR_SUCCESS != key.SetStringValue(_T("Icon"), _T("\"") + GetProgramPath() + _T("\",0"))) {
			return false;
		}
	} else {
		if (ERROR_SUCCESS != key.SetStringValue(NULL, _T(""))) {
			return false;
		}
	}

	if (ERROR_SUCCESS != key.Create(HKEY_CLASSES_ROOT, strProgID + _T("\\shell\\open\\command"))) {
		return false;
	}
	if (bSetValue && (ERROR_SUCCESS != key.SetStringValue(NULL, GetOpenCommand()))) {
		return false;
	}

	if (ERROR_SUCCESS != key.Create(HKEY_LOCAL_MACHINE, CString(GetRegisteredKey()) + _T("\\FileAssociations"))) {
		return false;
	}
	if (ERROR_SUCCESS != key.SetStringValue(ext, strProgID)) {
		return false;
	}

	if (setAssociatedWithIcon) {
		CString AppIcon;

		// first look for the icon
		CString ext_icon = GetProgramDir();
		ext_icon.AppendFormat(_T("icons\\%s.ico"), CString(ext).TrimLeft(_T(".")));
		if (::PathFileExists(ext_icon)) {
			AppIcon.Format(_T("\"%s\",0"), ext_icon);
		} else {
			// then look for the iconlib
			CString mpciconlib = GetProgramDir() + _T("mpciconlib.dll");
			if (::PathFileExists(mpciconlib)) {
				int icon_index = GetIconIndex(ext);
				if (icon_index < 0) {
					if (filetype == TAudio) {
						icon_index = GetIconIndex(_T(":audio"));
					} else if (filetype == TPlaylist) {
						icon_index = GetIconIndex(_T(":playlist"));
					}

					if (icon_index < 0) {
						icon_index = GetIconIndex(_T(":video"));
					}
				}
				if (icon_index >= 0 && ExtractIcon(AfxGetApp()->m_hInstance,(LPCWSTR)mpciconlib, icon_index)) {
					AppIcon.Format(_T("\"%s\",%d"), mpciconlib, icon_index);
				}
			}
		}

		/* no icon was found for the file extension, so use MPC's icon */
		if ((AppIcon.IsEmpty())) {
			AppIcon = GetProgramPath();
			AppIcon = "\""+AppIcon+"\"";
			AppIcon += _T(",0");
		}

		if (ERROR_SUCCESS != key.Create(HKEY_CLASSES_ROOT, strProgID + _T("\\DefaultIcon"))) {
			return false;
		}
		if (bSetValue && (ERROR_SUCCESS != key.SetStringValue(NULL, AppIcon))) {
			return false;
		}
	} else {
		key.Close();
		key.Attach(HKEY_CLASSES_ROOT);
		key.RecurseDeleteKey(strProgID + _T("\\DefaultIcon"));
	}

	if (!IsRegistered(ext)) {
		SetFileAssociation(ext, strProgID, true);
	}

	return true;
}

bool CPPageFormats::UnRegisterExt(CString ext)
{
	CRegKey key;
	CString strProgID = PROGID + ext;

	if (IsRegistered(ext)) {
		SetFileAssociation(ext, strProgID, false);
	}
	
	key.Attach(HKEY_CLASSES_ROOT);
	key.RecurseDeleteKey(strProgID);
	key.Close();

	if (ERROR_SUCCESS == key.Open(HKEY_LOCAL_MACHINE, CString(GetRegisteredKey()) + L"\\FileAssociations")) {
		key.DeleteValue(ext);
	}
	
	return true;
}

HRESULT CPPageFormats::RegisterUI()
{
	int nRegisteredExtCount = 0;
	CMediaFormats& mf = AfxGetAppSettings().m_Formats;
	for (size_t i = 0; i < mf.GetCount(); i++) {
		int j = 0;
		const CString str = mf[i].GetExtsWithPeriod();
		for (CString ext = str.Tokenize(L" ", j); !ext.IsEmpty(); ext = str.Tokenize(L" ", j)) {
			if (IsRegistered(ext)) {
				nRegisteredExtCount++;
			}
		}
	}

	if (nRegisteredExtCount == 0) {
		return S_FALSE;
	}

	HRESULT hr = E_FAIL;

	if (IsWin10orLater()) {
		IOpenControlPanel* pCP = NULL;
		hr = CoCreateInstance(CLSID_OpenControlPanel,
							  NULL,
							  CLSCTX_INPROC,
							  IID_PPV_ARGS(&pCP));

		if (SUCCEEDED(hr) && pCP) {
			hr = pCP->Open(L"Microsoft.DefaultPrograms", L"pageDefaultProgram", NULL);
			pCP->Release();
		}
	} else if (IsWin8orLater()) {
		IApplicationAssociationRegistrationUI* pUI = NULL;
		hr = CoCreateInstance(CLSID_ApplicationAssociationRegistrationUI,
							  NULL,
							  CLSCTX_INPROC,
							  IID_PPV_ARGS(&pUI));

		if (SUCCEEDED(hr) && pUI) {
			hr = pUI->LaunchAdvancedAssociationUI(CPPageFormats::GetRegisteredAppName());
			pUI->Release();
		}
	}

	return hr;
}

void Execute(LPCTSTR lpszCommand, LPCTSTR lpszParameters)
{
	SHELLEXECUTEINFO ShExecInfo = {0};
	ShExecInfo.cbSize		= sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask		= SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd			= NULL;
	ShExecInfo.lpVerb		= NULL;
	ShExecInfo.lpFile		= lpszCommand;
	ShExecInfo.lpParameters	= lpszParameters;
	ShExecInfo.lpDirectory	= NULL;
	ShExecInfo.nShow		= SWP_HIDEWINDOW;
	ShExecInfo.hInstApp		= NULL;

	ShellExecuteEx(&ShExecInfo);
	WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
}

typedef HRESULT (*_DllConfigFunc)(LPCTSTR);
typedef HRESULT (*_DllRegisterServer)(void);
typedef HRESULT (*_DllUnRegisterServer)(void);
bool CPPageFormats::RegisterShellExt(LPCTSTR lpszLibrary)
{
	HINSTANCE hDLL = LoadLibrary(lpszLibrary);
	if (hDLL == NULL) {
		if (::PathFileExists(lpszLibrary)) {
			CRegKey key;
			if (ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, _T("Software\\MPC-BE\\ShellExt"))) {
				key.SetStringValue(_T("MpcPath"), GetProgramPath());
			}

			CString strParameters;
			strParameters.Format(_T(" /s \"%s\""), lpszLibrary);
			Execute(_T("regsvr32.exe"), strParameters);

			return true;
		}
		return false;
	}

	_DllConfigFunc _dllConfigFunc         = (_DllConfigFunc)GetProcAddress(hDLL, "DllConfig");
	_DllRegisterServer _dllRegisterServer = (_DllRegisterServer)GetProcAddress(hDLL, "DllRegisterServer");
	if (_dllConfigFunc == NULL || _dllRegisterServer == NULL) {
		FreeLibrary(hDLL);
		return false;
	}

	HRESULT hr = _dllConfigFunc(GetProgramPath());
	if (FAILED(hr)) {
		FreeLibrary(hDLL);
		return false;
	}

	hr = _dllRegisterServer();

	FreeLibrary(hDLL);

	return SUCCEEDED(hr);
}

bool CPPageFormats::UnRegisterShellExt(LPCTSTR lpszLibrary)
{
	HINSTANCE hDLL = LoadLibrary(lpszLibrary);
	if (hDLL == NULL) {
		if (::PathFileExists(lpszLibrary)) {
			CString strParameters;
			strParameters.Format(_T(" /s /u \"%s\""), lpszLibrary);
			Execute(_T("regsvr32.exe"), strParameters);

			return true;
		}
		return false;
	}

	_DllUnRegisterServer _dllUnRegisterServer = (_DllRegisterServer)GetProcAddress(hDLL, "DllUnregisterServer");
	if (_dllUnRegisterServer == NULL) {
		FreeLibrary(hDLL);
		return false;
	}

	HRESULT hr = _dllUnRegisterServer();

	FreeLibrary(hDLL);

	return SUCCEEDED(hr);
}

static struct {
	LPCSTR verb, cmd;
	UINT action;
} handlers[] = {
	{"VideoFiles",	" %1",		IDS_AUTOPLAY_PLAYVIDEO},
	{"MusicFiles",	" %1",		IDS_AUTOPLAY_PLAYMUSIC},
	{"CDAudio",		" %1 /cd",	IDS_AUTOPLAY_PLAYAUDIOCD},
	{"DVDMovie",	" %1 /dvd",	IDS_AUTOPLAY_PLAYDVDMOVIE},
	{"BluRay",		" %1",		IDS_AUTOPLAY_PLAYBDMOVIE},
};

void CPPageFormats::AddAutoPlayToRegistry(autoplay_t ap, bool fRegister)
{
	CString exe = GetProgramPath();

	int i = (int)ap;
	if (i < 0 || i >= _countof(handlers)) {
		return;
	}

	CRegKey key;

	if (fRegister) {
		if (ERROR_SUCCESS != key.Create(HKEY_CLASSES_ROOT, _T("MPCBE.Autorun"))) {
			return;
		}
		key.Close();

		if (ERROR_SUCCESS != key.Create(HKEY_CLASSES_ROOT,
										CString(CStringA("MPCBE.Autorun\\Shell\\Play") + handlers[i].verb + "\\Command"))) {
			return;
		}
		key.SetStringValue(NULL, _T("\"") + exe + _T("\"") + handlers[i].cmd);
		key.Close();

		if (ERROR_SUCCESS != key.Create(HKEY_LOCAL_MACHINE,
										CString(CStringA("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoplayHandlers\\Handlers\\MPCBEPlay") + handlers[i].verb + "OnArrival"))) {
			return;
		}
		key.SetStringValue(_T("Action"), ResStr(handlers[i].action));
		key.SetStringValue(_T("Provider"), _T("MPC-BE"));
		key.SetStringValue(_T("InvokeProgID"), _T("MPCBE.Autorun"));
		key.SetStringValue(_T("InvokeVerb"), CString(CStringA("Play") + handlers[i].verb));
		key.SetStringValue(_T("DefaultIcon"), exe + _T(",0"));
		key.Close();

		if (ERROR_SUCCESS != key.Create(HKEY_LOCAL_MACHINE,
										CString(CStringA("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoplayHandlers\\EventHandlers\\Play") + handlers[i].verb + "OnArrival"))) {
			return;
		}
		key.SetStringValue(CString(CStringA("MPCBEPlay") + handlers[i].verb + "OnArrival"), _T(""));
	} else {
		if (ERROR_SUCCESS != key.Create(HKEY_LOCAL_MACHINE,
										CString(CStringA("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoplayHandlers\\EventHandlers\\Play") + handlers[i].verb + "OnArrival"))) {
			return;
		}
		key.DeleteValue(CString(CStringA("MPCBEPlay") + handlers[i].verb + "OnArrival"));
	}
}

bool CPPageFormats::IsAutoPlayRegistered(autoplay_t ap)
{
	int i = (int)ap;
	if (i < 0 || i >= _countof(handlers)) {
		return false;
	}

	CRegKey key;

	if (ERROR_SUCCESS != key.Open(HKEY_LOCAL_MACHINE,
								  CString(CStringA("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoplayHandlers\\EventHandlers\\Play") + handlers[i].verb + "OnArrival"),
								  KEY_READ)) {
		return false;
	}

	TCHAR buff[_MAX_PATH] = { 0 };
	ULONG len = _countof(buff);

	CString exe = GetProgramPath();
	
	if (ERROR_SUCCESS != key.QueryStringValue(
				CString(_T("MPCBEPlay")) + handlers[i].verb + _T("OnArrival"),
				buff, &len)) {
		return false;
	}
	key.Close();

	if (ERROR_SUCCESS != key.Open(HKEY_CLASSES_ROOT,
								  CString(CStringA("MPCBE.Autorun\\Shell\\Play") + handlers[i].verb + "\\Command"),
								  KEY_READ)) {
		return false;
	}
	len = _countof(buff);
	if (ERROR_SUCCESS != key.QueryStringValue(NULL, buff, &len)) {
		return false;
	}
	if (_tcsnicmp(_T("\"") + exe, buff, exe.GetLength() + 1)) {
		return false;
	}

	return true;
}

void CPPageFormats::SetListItemState(int nItem)
{
	if (nItem < 0) {
		return;
	}

	CString str = AfxGetAppSettings().m_Formats[(int)m_list.GetItemData(nItem)].GetExtsWithPeriod();

	CAtlList<CString> exts;
	ExplodeMin(str, exts, ' ');

	int cnt = 0;

	POSITION pos = exts.GetHeadPosition();
	while (pos) {
		if (IsRegistered(exts.GetNext(pos), true)) {
			cnt++;
		}
	}

	if (cnt != 0) {
		cnt = (cnt == (int)exts.GetCount() ? 1 : 2);
	}

	SetChecked(nItem, cnt);
}

BEGIN_MESSAGE_MAP(CPPageFormats, CPPageBase)
	ON_NOTIFY(NM_CLICK, IDC_LIST1, OnNMClickList1)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnLvnItemchangedList1)
	ON_NOTIFY(LVN_BEGINLABELEDIT, IDC_LIST1, OnBeginlabeleditList)
	ON_NOTIFY(LVN_DOLABELEDIT, IDC_LIST1, OnDolabeleditList)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_LIST1, OnEndlabeleditList)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedAll)
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedDefault)
	ON_BN_CLICKED(IDC_BUTTON_EXT_SET, OnBnClickedSet)
	ON_BN_CLICKED(IDC_BUTTON4, OnBnClickedVideo)
	ON_BN_CLICKED(IDC_BUTTON3, OnBnClickedAudio)
	ON_BN_CLICKED(IDC_BUTTON5, OnBnRunAdmin)
	ON_BN_CLICKED(IDC_BUTTON6, OnBnClickedNone)
	ON_BN_CLICKED(IDC_CHECK7, OnFilesAssocModified)
	ON_BN_CLICKED(IDC_CHECK8, OnFilesAssocModified)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON2, OnUpdateButtonDefault)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_EXT_SET, OnUpdateButtonSet)
END_MESSAGE_MAP()

// CPPageFormats message handlers

BOOL CPPageFormats::OnInitDialog()
{
	__super::OnInitDialog();

	SetCursor(m_hWnd, IDC_BUTTON1, IDC_HAND);

	m_bFileExtChanged = false;

	m_list.SetExtendedStyle(m_list.GetExtendedStyle() | LVS_EX_FULLROWSELECT);

	m_list.InsertColumn(COL_CATEGORY, _T("Category"), LVCFMT_LEFT);

	int dpiY = 96;

	if (CDPI* pDpi = dynamic_cast<CDPI*>(AfxGetMainWnd())) {
		dpiY = pDpi->GetDPIY();
	} else {
		// this panel can be created without the main window.
		pDpi = DNew CDPI();
		dpiY = pDpi->GetDPIY();
		delete pDpi;
	}

	CImage onoff;
	if (dpiY < 144) {
		onoff.LoadFromResource(AfxGetInstanceHandle(), IDB_ONOFF_96);
		m_onoff.Create(12, 12, ILC_COLOR4 | ILC_MASK, 0, 3);
	} else {
		onoff.LoadFromResource(AfxGetInstanceHandle(), IDB_ONOFF_144);
		m_onoff.Create(18, 18, ILC_COLOR4 | ILC_MASK, 0, 3);
	}
	m_onoff.Add(CBitmap::FromHandle(onoff), 0xffffff);
	m_list.SetImageList(&m_onoff, LVSIL_SMALL);

	CMediaFormats& mf = AfxGetAppSettings().m_Formats;
	mf.UpdateData(FALSE);
	for (int i = 0; i < (int)mf.GetCount(); i++) {
		CString label;
		label.Format (_T("%s (%s)"), mf[i].GetDescription(), mf[i].GetExts());

		int iItem = m_list.InsertItem(i, label);
		m_list.SetItemData(iItem, i);
	}

	m_list.SetColumnWidth(COL_CATEGORY, LVSCW_AUTOSIZE);

	m_list.SetSelectionMark(0);
	m_list.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
	m_exts = mf[(int)m_list.GetItemData(0)].GetExtsWithPeriod();

	CAppSettings& s = AfxGetAppSettings();

	UpdateData(FALSE);

	for (int i = 0; i < m_list.GetItemCount(); i++) {
		SetListItemState(i);
	}

	m_fContextFiles.SetCheck(s.bSetContextFiles);
	m_fContextDir.SetCheck(s.bSetContextDir);
	m_fAssociatedWithIcons.SetCheck(s.fAssociatedWithIcons);

	m_apvideo.SetCheck(IsAutoPlayRegistered(AP_VIDEO));
	m_apmusic.SetCheck(IsAutoPlayRegistered(AP_MUSIC));
	m_apaudiocd.SetCheck(IsAutoPlayRegistered(AP_AUDIOCD));
	m_apdvd.SetCheck(IsAutoPlayRegistered(AP_DVDMOVIE));

	CreateToolTip();

	if (IsWinVistaOrLater() && !IsUserAdmin()) {
		GetDlgItem(IDC_BUTTON1)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_BUTTON3)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_BUTTON4)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_BUTTON6)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_CHECK1)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK2)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK3)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK4)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK6)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK7)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK8)->EnableWindow(FALSE);

		GetDlgItem(IDC_BUTTON5)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_BUTTON5)->SendMessage(BCM_SETSHIELD, 0, 1);

		m_bInsufficientPrivileges = true;
	} else {
		GetDlgItem(IDC_BUTTON5)->ShowWindow(SW_HIDE);
	}

	m_lUnRegisterExts.RemoveAll();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPageFormats::SetFileAssociation(CString strExt, CString strProgID, bool bRegister)
{
	CString extoldreg, extOldIcon;
	CRegKey key;
	HRESULT hr = S_OK;
	TCHAR   buff[_MAX_PATH] = { 0 };
	ULONG   len = _countof(buff);

	if (m_pAAR) {
		// The Vista/7/8 way
		CString strNewApp;
		if (bRegister) {
			// Create non existing file type
			if (ERROR_SUCCESS != key.Create(HKEY_CLASSES_ROOT, strExt)) {
				return false;
			}

			WCHAR* pszCurrentAssociation;
			// Save current application associated
			if (SUCCEEDED(m_pAAR->QueryCurrentDefault(strExt, AT_FILEEXTENSION, AL_EFFECTIVE, &pszCurrentAssociation))) {

				if (ERROR_SUCCESS != key.Create(HKEY_CLASSES_ROOT, strProgID)) {
					return false;
				}

				key.SetStringValue(GetOldAssoc(), pszCurrentAssociation);

				// Get current icon for file type
				/*
				if (ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, CString(pszCurrentAssociation) + _T("\\DefaultIcon")))
				{
					len = sizeof(buff);
					memset(buff, 0, len);
					if (ERROR_SUCCESS == key.QueryStringValue(NULL, buff, &len) && !CString(buff).Trim().IsEmpty())
					{
						if (ERROR_SUCCESS == key.Create(HKEY_CLASSES_ROOT, strProgID + _T("\\DefaultIcon")))
							key.SetStringValue (NULL, buff);
					}
				}
				*/
				CoTaskMemFree(pszCurrentAssociation);
			}
			strNewApp = GetRegisteredAppName();
		} else {
			if (ERROR_SUCCESS != key.Open(HKEY_CLASSES_ROOT, strProgID)) {
				return false;
			}

			if (ERROR_SUCCESS == key.QueryStringValue(GetOldAssoc(), buff, &len)) {
				strNewApp = buff;
			}

			// TODO : retrieve registered app name from previous association (or find Bill function for that...)
		}

		hr = m_pAAR->SetAppAsDefault(strNewApp, strExt, AT_FILEEXTENSION);
	} else {
		// The 2000/XP/10 way
		if (bRegister) {
			// Set new association
			if (ERROR_SUCCESS != key.Create(HKEY_CLASSES_ROOT, strExt)) {
				return false;
			}

			if (ERROR_SUCCESS == key.QueryStringValue(NULL, buff, &len) && !CString(buff).Trim().IsEmpty()) {
				extoldreg = buff;
			}
			if (ERROR_SUCCESS != key.SetStringValue(NULL, strProgID)) {
				return false;
			}

			// Get current icon for file type
			/*
			if (!extoldreg.IsEmpty())
			{
				if (ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, extoldreg + _T("\\DefaultIcon")))
				{
					len = sizeof(buff);
					memset(buff, 0, len);
					if (ERROR_SUCCESS == key.QueryStringValue(NULL, buff, &len) && !CString(buff).Trim().IsEmpty())
						extOldIcon = buff;
				}
			}
			*/

			// Save old association
			if (ERROR_SUCCESS != key.Create(HKEY_CLASSES_ROOT, strProgID)) {
				return false;
			}
			key.SetStringValue(GetOldAssoc(), extoldreg);

			/*
			if (!extOldIcon.IsEmpty() && (ERROR_SUCCESS == key.Create(HKEY_CLASSES_ROOT, strProgID + _T("\\DefaultIcon"))))
				key.SetStringValue (NULL, extOldIcon);
			*/
		} else {
			// Get previous association
			if (ERROR_SUCCESS != key.Create(HKEY_CLASSES_ROOT, strProgID)) {
				return false;
			}
			if (ERROR_SUCCESS == key.QueryStringValue(GetOldAssoc(), buff, &len) && !CString(buff).Trim().IsEmpty()) {
				extoldreg = buff;
			}

			// Set previous association
			if (ERROR_SUCCESS != key.Create(HKEY_CLASSES_ROOT, strExt)) {
				return false;
			}
			key.SetStringValue(NULL, extoldreg);
		}
	}

	return SUCCEEDED(hr);
}

void GetUnRegisterExts(CString saved_ext, CString new_ext, CAtlList<CString>& UnRegisterExts)
{
	if (saved_ext.CompareNoCase(new_ext) != 0) {
		CAtlList<CString> saved_exts;
		Explode(saved_ext, saved_exts, ' ');
		CAtlList<CString> new_exts;
		Explode(new_ext, new_exts, ' ');

		POSITION pos = saved_exts.GetHeadPosition();
		while (pos) {
			saved_ext = saved_exts.GetNext(pos);
			bool bMatch = false;

			POSITION pos2 = new_exts.GetHeadPosition();
			while (pos2) {
				new_ext = new_exts.GetNext(pos2);
				if (new_ext.CompareNoCase(saved_ext) == 0) {
					bMatch = true;
					continue;
				}
			}

			if (!bMatch) {
				UnRegisterExts.AddTail(saved_ext);
			}
		}
	}
}

BOOL CPPageFormats::OnApply()
{
	bool bSetAssociatedWithIconOld	= !!m_fAssociatedWithIcons.GetCheck();
	bool bSetContextFileOld			= !!m_fContextFiles.GetCheck();

	UpdateData();

	CAppSettings& s = AfxGetAppSettings();
	CMediaFormats& mf = s.m_Formats;

	{
		int i = m_list.GetSelectionMark();
		if (i >= 0) {
			i = (int)m_list.GetItemData(i);
		}
		if (i >= 0) {
			if (i > 0) {
				GetUnRegisterExts(mf[i].GetExtsWithPeriod(), m_exts, m_lUnRegisterExts);
			}
			mf[i].SetExts(m_exts);
			m_exts = mf[i].GetExtsWithPeriod();
			UpdateData(FALSE);
		}
	}

	if (m_lUnRegisterExts.GetCount()) {
		POSITION pos = m_lUnRegisterExts.GetHeadPosition();
		while (pos) {
			UnRegisterExt(m_lUnRegisterExts.GetNext(pos));
		}
	}

	RegisterApp();

	bool bSetAssociatedWithIcon	= !!m_fAssociatedWithIcons.GetCheck();
	bool bSetContextFile		= !!m_fContextFiles.GetCheck();

	m_bFileExtChanged = m_bFileExtChanged || (bSetContextFileOld != bSetContextFile || bSetAssociatedWithIconOld != bSetAssociatedWithIcon);

	if (m_bFileExtChanged) {
		for (int i = 0; i < m_list.GetItemCount(); i++) {
			int iChecked = GetChecked(i);

			if (iChecked == 2) {
				continue;
			}

			CAtlList<CString> exts;
			Explode(mf[(int)m_list.GetItemData(i)].GetExtsWithPeriod(), exts, ' ');

			POSITION pos = exts.GetHeadPosition();
			while (pos) {
				if (iChecked) {
					RegisterExt(exts.GetNext(pos), mf[(int)m_list.GetItemData(i)].GetDescription(), mf[i].GetFileType(), bSetContextFile, bSetAssociatedWithIcon);
				} else {
					UnRegisterExt(exts.GetNext(pos));
				}
			}
		}
	}

	if (m_bFileExtChanged
			|| !!m_fContextFiles.GetCheck() != s.bSetContextFiles
			|| !!m_fContextDir.GetCheck() != s.bSetContextDir) {
		CRegKey key;
		if (ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, L"Software\\MPC-BE\\ShellExt")) {
			key.SetStringValue(L"Play", ResStr(IDS_OPEN_WITH_MPC));
			key.SetStringValue(L"Add", ResStr(IDS_ADD_TO_PLAYLIST));

			key.SetDWORDValue(L"ShowFiles", !!m_fContextFiles.GetCheck());
			key.SetDWORDValue(L"ShowDir", !!m_fContextDir.GetCheck());
		}

		BOOL bIs64 = IsW64();
		UnRegisterShellExt(ShellExt);
		if (bIs64) {
			UnRegisterShellExt(ShellExt64);
		}

		RegisterShellExt(ShellExt);
		if (bIs64) {
			RegisterShellExt(ShellExt64);
		}
	}

	CRegKey key;
	if (m_fContextDir.GetCheck() && !ShellExtExists()) {
		if (ERROR_SUCCESS == key.Create(HKEY_CLASSES_ROOT, _T("Directory\\shell\\") PROGID _T(".enqueue"))) {
			key.SetStringValue(NULL, ResStr(IDS_ADD_TO_PLAYLIST));
			key.SetStringValue(_T("Icon"), _T("\"") + GetProgramPath() + _T("\",0"));
		}

		if (ERROR_SUCCESS == key.Create(HKEY_CLASSES_ROOT, _T("Directory\\shell\\") PROGID _T(".enqueue\\command"))) {
			key.SetStringValue(NULL, GetEnqueueCommand());
		}

		if (ERROR_SUCCESS == key.Create(HKEY_CLASSES_ROOT, _T("Directory\\shell\\") PROGID _T(".play"))) {
			key.SetStringValue(NULL, ResStr(IDS_OPEN_WITH_MPC));
			key.SetStringValue(_T("Icon"), _T("\"") + GetProgramPath() + _T("\",0"));
		}

		if (ERROR_SUCCESS == key.Create(HKEY_CLASSES_ROOT, _T("Directory\\shell\\") PROGID _T(".play\\command"))) {
			key.SetStringValue(NULL, GetOpenCommand());
		}
	} else {
		key.Attach(HKEY_CLASSES_ROOT);
		key.RecurseDeleteKey(_T("Directory\\shell\\") PROGID _T(".enqueue"));
		key.RecurseDeleteKey(_T("Directory\\shell\\") PROGID _T(".play"));
	}

	SetListItemState(m_list.GetSelectionMark());

	AddAutoPlayToRegistry(AP_VIDEO,		!!m_apvideo.GetCheck());
	AddAutoPlayToRegistry(AP_MUSIC,		!!m_apmusic.GetCheck());
	AddAutoPlayToRegistry(AP_AUDIOCD,	!!m_apaudiocd.GetCheck());
	AddAutoPlayToRegistry(AP_DVDMOVIE,	!!m_apdvd.GetCheck());
	AddAutoPlayToRegistry(AP_BDMOVIE,	!!m_apdvd.GetCheck());

	s.bSetContextFiles		= !!m_fContextFiles.GetCheck();
	s.bSetContextDir		= !!m_fContextDir.GetCheck();
	s.fAssociatedWithIcons	= !!m_fAssociatedWithIcons.GetCheck();

	if (m_bFileExtChanged) {
		if (IsWin8orLater()) {
			RegisterUI();
		}

		SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
	}

	m_bFileExtChanged = false;

	m_lUnRegisterExts.RemoveAll();

	return __super::OnApply();
}

void CPPageFormats::OnNMClickList1(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW)pNMHDR;

	if (lpnmlv->iItem >= 0 && lpnmlv->iSubItem == COL_CATEGORY) {
		CRect r;
		m_list.GetItemRect(lpnmlv->iItem, r, LVIR_ICON);
		if (r.PtInRect(lpnmlv->ptAction)) {
			if (m_bInsufficientPrivileges) {
				MessageBox (ResStr (IDS_CANNOT_CHANGE_FORMAT));
			} else {
				SetChecked(lpnmlv->iItem, (GetChecked(lpnmlv->iItem)&1) == 0 ? 1 : 0);
				m_bFileExtChanged = true;
				SetModified();
			}
		}
	}

	*pResult = 0;
}

void CPPageFormats::OnLvnItemchangedList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	if (pNMLV->iItem >= 0 && pNMLV->iSubItem == COL_CATEGORY
			&& (pNMLV->uChanged&LVIF_STATE) && (pNMLV->uNewState&LVIS_SELECTED)) {
		m_exts = AfxGetAppSettings().m_Formats[(int)m_list.GetItemData(pNMLV->iItem)].GetExtsWithPeriod();
		UpdateData(FALSE);
	}

	*pResult = 0;
}

void CPPageFormats::OnBeginlabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	LV_ITEM* pItem = &pDispInfo->item;

	*pResult = FALSE;

	if (pItem->iItem < 0) {
		return;
	}

	//... process other columns
}

void CPPageFormats::OnDolabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	LV_ITEM* pItem = &pDispInfo->item;

	*pResult = FALSE;

	if (pItem->iItem < 0) {
		return;
	}

	//... process other columns
}

void CPPageFormats::OnEndlabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	LV_ITEM* pItem = &pDispInfo->item;

	*pResult = FALSE;

	if (!m_list.m_fInPlaceDirty) {
		return;
	}

	if (pItem->iItem < 0) {
		return;
	}

	//... process other columns
}

void CPPageFormats::OnBnClickedAll()
{
	for (int i = 0, j = m_list.GetItemCount(); i < j; i++) {
		SetChecked(i, 1);
	}
	m_bFileExtChanged = true;

	SetModified();
}

void CPPageFormats::OnBnClickedVideo()
{
	CMediaFormats& mf = AfxGetAppSettings().m_Formats;

	for (int i = 0, j = m_list.GetItemCount(); i < j; i++) {
		SetChecked(i, mf[(int)m_list.GetItemData(i)].GetFileType() == TVideo ? 1 : 0);
	}
	m_bFileExtChanged = true;

	SetModified();
}

void CPPageFormats::OnBnClickedAudio()
{
	CMediaFormats& mf = AfxGetAppSettings().m_Formats;

	for (int i = 0, j = m_list.GetItemCount(); i < j; i++) {
		SetChecked(i, mf[(int)m_list.GetItemData(i)].GetFileType() == TAudio ? 1 : 0);
	}
	m_bFileExtChanged = true;

	SetModified();
}

void CPPageFormats::OnBnRunAdmin()
{
	CString strCmd;
	strCmd.Format(_T("/adminoption %d"), IDD);

	AfxGetMyApp()->RunAsAdministrator(GetProgramPath(), strCmd, true);

	for (int i = 0; i < m_list.GetItemCount(); i++) {
		SetListItemState(i);
	}

	CAppSettings& s = AfxGetAppSettings();
	s.LoadSettings(true);

	m_fContextFiles.SetCheck(s.bSetContextFiles);
	m_fContextDir.SetCheck(s.bSetContextDir);
	m_fAssociatedWithIcons.SetCheck(s.fAssociatedWithIcons);

	m_apvideo.SetCheck(IsAutoPlayRegistered(AP_VIDEO));
	m_apmusic.SetCheck(IsAutoPlayRegistered(AP_MUSIC));
	m_apaudiocd.SetCheck(IsAutoPlayRegistered(AP_AUDIOCD));
	m_apdvd.SetCheck(IsAutoPlayRegistered(AP_DVDMOVIE));
}

void CPPageFormats::OnBnClickedNone()
{
	for (int i = 0, j = m_list.GetItemCount(); i < j; i++) {
		SetChecked(i, 0);
	}
	m_bFileExtChanged = true;

	SetModified();
}

void CPPageFormats::OnBnClickedDefault()
{
	int iItem = m_list.GetSelectionMark();
	if (iItem >= 0) {
		DWORD_PTR i = m_list.GetItemData(iItem);
		CMediaFormats& mf = AfxGetAppSettings().m_Formats;

		mf[i].RestoreDefaultExts();
		GetUnRegisterExts(m_exts, mf[i].GetExtsWithPeriod(), m_lUnRegisterExts);
		m_exts = mf[i].GetExtsWithPeriod();

		CString label;
		label.Format(_T("%s (%s)"), mf[i].GetDescription(), mf[i].GetExts());
		m_list.SetItemText(iItem, COL_CATEGORY, label);
		m_list.SetColumnWidth(COL_CATEGORY, LVSCW_AUTOSIZE);

		//SetListItemState(m_list.GetSelectionMark());
		UpdateData(FALSE);

		m_bFileExtChanged = true;

		SetModified();
	}
}

void CPPageFormats::OnBnClickedSet()
{
	UpdateData();

	int iItem = m_list.GetSelectionMark();
	if (iItem >= 0) {
		DWORD_PTR i = m_list.GetItemData(iItem);
		CMediaFormats& mf = AfxGetAppSettings().m_Formats;

		GetUnRegisterExts(mf[i].GetExtsWithPeriod(), m_exts, m_lUnRegisterExts);
		mf[i].SetExts(m_exts);
		m_exts = mf[i].GetExtsWithPeriod();

		CString label;
		label.Format(_T("%s (%s)"), mf[i].GetDescription(), mf[i].GetExts());
		m_list.SetItemText(iItem, COL_CATEGORY, label);
		m_list.SetColumnWidth(COL_CATEGORY, LVSCW_AUTOSIZE);

		//SetListItemState(m_list.GetSelectionMark());
		UpdateData(FALSE);

		m_bFileExtChanged = true;

		SetModified();
	}
}

void CPPageFormats::OnFilesAssocModified()
{
	m_bFileExtChanged = true;
	SetModified();
}

void CPPageFormats::OnUpdateButtonDefault(CCmdUI* pCmdUI)
{
	int i = m_list.GetSelectionMark();
	if (i < 0) {
		pCmdUI->Enable(FALSE);
		return;
	}
	i = (int)m_list.GetItemData(i);

	CString orgexts, newexts;
	GetDlgItem(IDC_EDIT1)->GetWindowText(newexts);
	newexts.Trim();
	orgexts = AfxGetAppSettings().m_Formats[i].GetBackupExtsWithPeriod();

	pCmdUI->Enable(!!newexts.CompareNoCase(orgexts));
}

void CPPageFormats::OnUpdateButtonSet(CCmdUI* pCmdUI)
{
	int i = m_list.GetSelectionMark();
	if (i < 0) {
		pCmdUI->Enable(FALSE);
		return;
	}
	i = (int)m_list.GetItemData(i);

	CString orgexts, newexts;
	GetDlgItem(IDC_EDIT1)->GetWindowText(newexts);
	newexts.Trim();
	orgexts = AfxGetAppSettings().m_Formats[i].GetExtsWithPeriod();

	if (!!newexts.CompareNoCase(orgexts)) {
		m_bFileExtChanged = true;
	}

	pCmdUI->Enable(!!newexts.CompareNoCase(orgexts));
}

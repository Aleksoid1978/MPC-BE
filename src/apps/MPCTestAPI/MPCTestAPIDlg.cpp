/*
 * (C) 2008-2017 see Authors.txt
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
#include "MPCTestAPI.h"
#include "MPCTestAPIDlg.h"
#include <algorithm>
#include <psapi.h>

struct MPCCommandInfo {
	MPCAPI_COMMAND id;
	LPCWSTR name;
	LPCWSTR desc;
};

static const MPCCommandInfo s_UnknownCmd = {(MPCAPI_COMMAND)0, L"CMD_UNKNOWN", L""};

#define ADD_CMD(ID, DESC) {ID, L#ID, DESC},
static const MPCCommandInfo s_ReceivedCmds[] = {
	ADD_CMD(CMD_CONNECT,            L"")
	ADD_CMD(CMD_STATE,              L"")
	ADD_CMD(CMD_PLAYMODE,           L"")
	ADD_CMD(CMD_NOWPLAYING,         L"")
	ADD_CMD(CMD_LISTSUBTITLETRACKS, L"")
	ADD_CMD(CMD_LISTAUDIOTRACKS,    L"")
	ADD_CMD(CMD_PLAYLIST,           L"")
	ADD_CMD(CMD_CURRENTPOSITION,    L"")
	ADD_CMD(CMD_NOTIFYSEEK,         L"")
	ADD_CMD(CMD_NOTIFYENDOFSTREAM,  L"")
	ADD_CMD(CMD_VERSION,            L"")
	ADD_CMD(CMD_DISCONNECT,         L"")
};

static const MPCCommandInfo s_SentCmds[] = {
	ADD_CMD(CMD_OPENFILE,           L"Open file")
	ADD_CMD(CMD_STOP,               L"Stop")
	ADD_CMD(CMD_CLOSEFILE,          L"Close")
	ADD_CMD(CMD_PLAYPAUSE,          L"Play-Pause")
	ADD_CMD(CMD_PLAY,               L"")
	ADD_CMD(CMD_PAUSE,              L"")
	ADD_CMD(CMD_ADDTOPLAYLIST,      L"Add to playlist")
	ADD_CMD(CMD_CLEARPLAYLIST,      L"Clear playlist")
	ADD_CMD(CMD_STARTPLAYLIST,      L"Start playlist")
	ADD_CMD(CMD_REMOVEFROMPLAYLIST, L"")
	ADD_CMD(CMD_SETPOSITION,        L"Set position")
	ADD_CMD(CMD_SETAUDIODELAY,      L"Set audio delay")
	ADD_CMD(CMD_SETSUBTITLEDELAY,   L"Set subtitle delay")
	ADD_CMD(CMD_SETINDEXPLAYLIST,   L"Set position in playlist")
	ADD_CMD(CMD_SETAUDIOTRACK,      L"Set audio track")
	ADD_CMD(CMD_SETSUBTITLETRACK,   L"Set subtitle track")
	ADD_CMD(CMD_GETSUBTITLETRACKS,  L"Get subtitle tracks")
	ADD_CMD(CMD_GETAUDIOTRACKS,     L"Get audio tracks")
	ADD_CMD(CMD_GETNOWPLAYING,      L"")
	ADD_CMD(CMD_GETPLAYLIST,        L"Get playlist")
	ADD_CMD(CMD_GETCURRENTPOSITION, L"")
	ADD_CMD(CMD_JUMPOFNSECONDS,     L"")
	ADD_CMD(CMD_GETVERSION,         L"")
	ADD_CMD(CMD_TOGGLEFULLSCREEN,   L"FullScreen")
	ADD_CMD(CMD_JUMPFORWARDMED,     L"")
	ADD_CMD(CMD_JUMPBACKWARDMED,    L"")
	ADD_CMD(CMD_INCREASEVOLUME,     L"")
	ADD_CMD(CMD_DECREASEVOLUME,     L"")
	ADD_CMD(CMD_SHADER_TOGGLE,      L"")
	ADD_CMD(CMD_CLOSEAPP,           L"")
	ADD_CMD(CMD_SETSPEED,           L"")
	ADD_CMD(CMD_OSDSHOWMESSAGE,     L"")
};
#undef ADD_CMD

const MPCCommandInfo& GetMPCCommandInfo(MPCAPI_COMMAND nCmd)
{
	if (nCmd >> 16 == 0x5000) {
		for (const auto& cmdinfo : s_ReceivedCmds) {
			if (cmdinfo.id == nCmd) {
				return cmdinfo;
			}
		}
	}

	if (nCmd >> 16 == 0xA000) {
		for (const auto& cmdinfo : s_SentCmds) {
			if (cmdinfo.id == nCmd) {
				return cmdinfo;
			}
		}
	}

	return s_UnknownCmd;
}

inline void AddStringData(CComboBox& ComboBox, LPCTSTR str, DWORD_PTR data)
{
	ComboBox.SetItemData(ComboBox.AddString(str), data);
}

inline DWORD_PTR GetCurItemData(CComboBox& ComboBox)
{
	return ComboBox.GetItemData(ComboBox.GetCurSel());
}

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

	// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

	// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}


void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// CRegisterCopyDataDlg dialog

CRegisterCopyDataDlg::CRegisterCopyDataDlg(CWnd* pParent)
	: CDialog(CRegisterCopyDataDlg::IDD, pParent)
	, m_hWndMPC(nullptr)
	, m_RemoteWindow(nullptr)
{
	//{{AFX_DATA_INIT(CRegisterCopyDataDlg)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIconW(IDR_MAINFRAME);
}


void CRegisterCopyDataDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRegisterCopyDataDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_EDIT1, m_strMPCPath);
	DDX_Control(pDX, IDC_LOGLIST, m_listBox);
	DDX_Text(pDX, IDC_EDIT2, m_txtCommand);
	DDX_Control(pDX, IDC_COMBO1, m_cbCommand);
}

BEGIN_MESSAGE_MAP(CRegisterCopyDataDlg, CDialog)
	//{{AFX_MSG_MAP(CRegisterCopyDataDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_FINDWINDOW, OnButtonFindwindow)
	ON_WM_COPYDATA()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_SENDCOMMAND, &CRegisterCopyDataDlg::OnBnClickedButtonSendcommand)
END_MESSAGE_MAP()

// CRegisterCopyDataDlg message handlers

BOOL CRegisterCopyDataDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	{ // set programm dir as current dir 
		CString progpath;
		DWORD bufsize = MAX_PATH;
		DWORD len = 0;
		do {
			len = GetModuleFileNameW(nullptr, progpath.GetBuffer(bufsize), bufsize);
			if (len >= bufsize) {
				bufsize *= 2;
				continue;
			}
		} while (0);
		PathRemoveFileSpecW(progpath.GetBuffer());
		progpath.ReleaseBuffer();
		SetCurrentDirectoryW(progpath);
	}

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr) {
		CString strAboutMenu;
		strAboutMenu.LoadStringW(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty()) {
			pSysMenu->AppendMenuW(MF_SEPARATOR);
			pSysMenu->AppendMenuW(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	m_strMPCPath = L"..\\";

#if defined (_WIN64)
	m_strMPCPath += L"mpc-be_x64";
#else
	m_strMPCPath += L"mpc-be_x86";
#endif // _WIN64

#if defined (_DEBUG)
	m_strMPCPath += L"_Debug\\";
#else
	m_strMPCPath += L"\\";
#endif // _DEBUG

#if defined (_WIN64)
	m_strMPCPath += L"mpc-be64.exe";
#else
	m_strMPCPath += L"mpc-be.exe";
#endif // _WIN64

	m_cbCommand.Clear();
	for (const auto& cmdinfo : s_SentCmds) {
		if (cmdinfo.desc[0] == 0) {
			AddStringData(m_cbCommand, cmdinfo.name, cmdinfo.id);
		}
		else {
			AddStringData(m_cbCommand, cmdinfo.desc, cmdinfo.id);
		}
	}

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CRegisterCopyDataDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	} else {
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CRegisterCopyDataDlg::OnPaint()
{
	if (IsIconic()) {
		CPaintDC dc(this); // device context for painting

		SendMessageW(WM_ICONERASEBKGND, (WPARAM)dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	} else {
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.

HCURSOR CRegisterCopyDataDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CRegisterCopyDataDlg::OnButtonFindwindow()
{
	CString				strExec;
	STARTUPINFOW		StartupInfo;
	PROCESS_INFORMATION	ProcessInfo;

	strExec.Format(L"%s /slave %d", m_strMPCPath.GetString(), GetSafeHwnd());
	UpdateData(TRUE);

	memset(&StartupInfo, 0, sizeof(StartupInfo));
	StartupInfo.cb = sizeof(StartupInfo);
	GetStartupInfoW(&StartupInfo);
	if (CreateProcessW(nullptr, (LPWSTR)(LPCWSTR)strExec, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &StartupInfo, &ProcessInfo)) {
		CloseHandle(ProcessInfo.hProcess);
		CloseHandle(ProcessInfo.hThread);
	}
}

void CRegisterCopyDataDlg::SendData(MPCAPI_COMMAND nCmd, LPCWSTR strCommand)
{
	if (m_hWndMPC) {
		COPYDATASTRUCT MyCDS;

		MyCDS.dwData = nCmd;
		if (nCmd == CMD_OSDSHOWMESSAGE) {
			MPC_OSDDATA osddata;
			osddata.nMsgPos = 1;
			osddata.nDurationMS = 3000;
			size_t len = std::min(wcslen(strCommand), _countof(osddata.strMsg)-1) * sizeof(WCHAR);
			memset(osddata.strMsg, 0, sizeof(osddata.strMsg));
			memcpy(osddata.strMsg, strCommand, len);

			MyCDS.cbData = sizeof(osddata);
			MyCDS.lpData = (LPVOID)&osddata;
		}
		else {
			MyCDS.cbData = (DWORD)(wcslen(strCommand) + 1) * sizeof(WCHAR);
			MyCDS.lpData = (LPVOID)strCommand;
		}

		::SendMessageW(m_hWndMPC, WM_COPYDATA, (WPARAM)GetSafeHwnd(), (LPARAM)&MyCDS);
	}
}

BOOL CRegisterCopyDataDlg::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
	CString strMsg;

	if (pCopyDataStruct->dwData == CMD_CONNECT) {
		m_hWndMPC = (HWND)IntToPtr(_wtoi((LPCWSTR)pCopyDataStruct->lpData));
	}

	strMsg.Format(L"%s : %s", GetMPCCommandInfo((MPCAPI_COMMAND)pCopyDataStruct->dwData).name, (LPCWSTR)pCopyDataStruct->lpData);
	m_listBox.InsertString(0, strMsg);
	return CDialog::OnCopyData(pWnd, pCopyDataStruct);
}

void CRegisterCopyDataDlg::OnBnClickedButtonSendcommand()
{
	UpdateData(TRUE);

	auto cmdInfo = GetMPCCommandInfo((MPCAPI_COMMAND)GetCurItemData(m_cbCommand));

	switch (cmdInfo.id) {
		case CMD_OPENFILE:
		case CMD_ADDTOPLAYLIST:
		case CMD_REMOVEFROMPLAYLIST: // TODO
		case CMD_SETPOSITION:
		case CMD_SETAUDIODELAY:
		case CMD_SETSUBTITLEDELAY:
		case CMD_SETINDEXPLAYLIST:
		case CMD_SETAUDIOTRACK:
		case CMD_SETSUBTITLETRACK:
		case CMD_JUMPOFNSECONDS:
		case CMD_SETSPEED:
		case CMD_OSDSHOWMESSAGE:
			SendData(cmdInfo.id, m_txtCommand);
			break;
		case CMD_STOP:
		case CMD_CLOSEFILE:
		case CMD_PLAYPAUSE:
		case CMD_PLAY:
		case CMD_PAUSE:
		case CMD_CLEARPLAYLIST:
		case CMD_STARTPLAYLIST:
		case CMD_GETSUBTITLETRACKS:
		case CMD_GETAUDIOTRACKS:
		case CMD_GETNOWPLAYING:
		case CMD_GETPLAYLIST:
		case CMD_GETCURRENTPOSITION:
		case CMD_GETVERSION:
		case CMD_TOGGLEFULLSCREEN:
		case CMD_JUMPFORWARDMED:
		case CMD_JUMPBACKWARDMED:
		case CMD_INCREASEVOLUME:
		case CMD_DECREASEVOLUME:
		case CMD_SHADER_TOGGLE:
		case CMD_CLOSEAPP:
			SendData(cmdInfo.id, L"");
			break;
		default:
			ASSERT(0);
	}
}

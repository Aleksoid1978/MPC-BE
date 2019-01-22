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

#pragma once

#ifndef __AFXWIN_H__
#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"
#include <afxadv.h>
#include <atlsync.h>
#include <ShlObj.h>
#include "../../DSUtil/Profile.h"
#include "FakeFilterMapper2.h"
#include "AppSettings.h"
#include "../../../Include/Version.h"

#include <map>
#include <mutex>

#define DEF_LOGO IDF_LOGO1

#if (MPC_VERSION_STATUS == 0) // only for beta versions
	#define MPCVR 1
#else
	#define MPCVR 0
#endif

enum {
	WM_GRAPHNOTIFY = WM_RESET_DEVICE + 1,
	WM_TUNER_SCAN_PROGRESS,
	WM_TUNER_SCAN_END,
	WM_TUNER_STATS,
	WM_TUNER_NEW_CHANNEL,
	WM_POSTOPEN,
	WM_SAVESETTINGS
};

#define WM_MYMOUSELAST WM_XBUTTONDBLCLK

struct LanguageResource {
	const UINT resourceID;
	const LANGID localeID;
	LPCTSTR name;
	LPCTSTR strcode;
	LPCTSTR iso6392;
};

class CMPlayerCApp : public CModApp
{
private:
	ATL::CMutex m_mutexOneInstance;

	std::list<CString> m_cmdln;
	void PreProcessCommandLine();
	BOOL SendCommandLine(HWND hWnd);
	UINT GetVKFromAppCommand(UINT nAppCommand);

	static UINT GetRemoteControlCodeMicrosoft(UINT nInputcode, HRAWINPUT hRawInput);
	static UINT GetRemoteControlCodeSRM7500(UINT nInputcode, HRAWINPUT hRawInput);

	virtual BOOL OnIdle(LONG lCount) override;
public:
	CMPlayerCApp();
	~CMPlayerCApp();

	void ShowCmdlnSwitches() const;

	CString			m_AudioRendererDisplayName_CL;

	CAppSettings m_s;

	typedef UINT (*PTR_GetRemoteControlCode)(UINT nInputcode, HRAWINPUT hRawInput);

	PTR_GetRemoteControlCode	GetRemoteControlCode;

	static const LanguageResource languageResources[];
	static const size_t languageResourcesCount;

	static void		SetLanguage(int nLanguage, bool bSave = true);
	static CString	GetSatelliteDll(int nLanguage);
	static int		GetLanguageIndex(UINT resID);
	static int		GetLanguageIndex(CString langcode);
	static int		GetDefLanguage();

	static void		RunAsAdministrator(LPCTSTR strCommand, LPCTSTR strArgs, bool bWaitProcess);

	void			RegisterHotkeys();
	void			UnregisterHotkeys();

private:
	bool ClearSettings();

public:

	bool			ChangeSettingsLocation(bool useIni);
	void			ExportSettings();
	bool			GetAppSavePath(CString& path);

public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnAppAbout();
	afx_msg void OnFileExit();
	afx_msg void OnHelpShowcommandlineswitches();
};

#define AfxGetMyApp()       static_cast<CMPlayerCApp*>(AfxGetApp())
#define AfxGetAppSettings() static_cast<CMPlayerCApp*>(AfxGetApp())->m_s
#define AfxGetMainFrame()   static_cast<CMainFrame*>(AfxGetMainWnd())
#define AfxFindMainFrame()  dynamic_cast<CMainFrame*>(AfxGetMainWnd())

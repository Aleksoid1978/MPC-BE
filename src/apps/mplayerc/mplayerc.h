/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2024 see Authors.txt
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

#include <afxadv.h>
#include <afxmt.h>
#include <atlsync.h>
#include "DSUtil/Profile.h"
#include "HistoryFile.h"
#include "AppSettings.h"

#include <mutex>

#define DEF_LOGO IDF_LOGO1

#define WM_MYMOUSELAST WM_XBUTTONDBLCLK

struct LanguageResource {
	const UINT resourceID;
	const LANGID localeID;
	LPCWSTR name;
	LPCWSTR strcode;
	LPCWSTR iso6392;
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
	CAppSettings m_s;

	CHistoryFile m_HistoryFile;
	CFavoritesFile m_FavoritesFile;

	CString m_AudioRendererDisplayName_CL;

	CMPlayerCApp();
	~CMPlayerCApp();

	void ShowCmdlnSwitches() const;

	typedef UINT (*PTR_GetRemoteControlCode)(UINT nInputcode, HRAWINPUT hRawInput);

	PTR_GetRemoteControlCode	GetRemoteControlCode;

	static const LanguageResource languageResources[];
	static const size_t languageResourcesCount;

	static void		SetLanguage(int nLanguage, bool bSave = true);
	static CString	GetSatelliteDll(int nLanguage);
	static int		GetLanguageIndex(UINT resID);
	static int		GetLanguageIndex(CString langcode);
	static int		GetDefLanguage();

	static void		RunAsAdministrator(LPCWSTR strCommand, LPCWSTR strArgs, bool bWaitProcess);

	void			RegisterHotkeys();
	void			UnregisterHotkeys();

private:
	bool ClearSettings();

	bool m_bRunAdmin = false;

public:
	bool			ChangeSettingsLocation(const SettingsLocation newSetLocation);
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

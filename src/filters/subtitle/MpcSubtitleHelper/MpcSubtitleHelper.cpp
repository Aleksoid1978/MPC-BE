/*
 * (C) 2016-2024 see Authors.txt
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

#include "pch.h"
#include "framework.h"
#include "MpcSubtitleHelper.h"
#include "CSubtitleHelperFilter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

// CMpcSubtitleHelperApp

BEGIN_MESSAGE_MAP(CMpcSubtitleHelperApp, CWinApp)
END_MESSAGE_MAP()


// CMpcSubtitleHelperApp construction

CMpcSubtitleHelperApp::CMpcSubtitleHelperApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CMpcSubtitleHelperApp object

CMpcSubtitleHelperApp theApp;


// CMpcSubtitleHelperApp initialization

BOOL CMpcSubtitleHelperApp::InitInstance()
{
	BOOL abRet;

	abRet = __super::InitInstance();
	if (abRet)
		abRet = DllEntryPoint(AfxGetInstanceHandle(), DLL_PROCESS_ATTACH, nullptr);

	return abRet;
}

BOOL CMpcSubtitleHelperApp::ExitInstance()
{
	DllEntryPoint(AfxGetInstanceHandle(), DLL_PROCESS_DETACH, nullptr);

	return __super::ExitInstance();
}

template <class T>
static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
	ASSERT(phr);

	*phr = S_OK;
	CUnknown* punk = new T(lpunk, phr);
	if (punk == nullptr) {
		*phr = E_OUTOFMEMORY;
	}
	return punk;
}

const AMOVIESETUP_FILTER sudFilter[] = {
	{ &__uuidof(CSubtitleHelperFilter), CSubtitleHelperFilter::szName, MERIT_DO_NOT_USE, 0, nullptr, CLSID_LegacyAmFilterCategory },
};

CFactoryTemplate g_Templates[] = {
	{ sudFilter[0].strName, &__uuidof(CSubtitleHelperFilter), CreateInstance<CSubtitleHelperFilter>, nullptr, &sudFilter[0] },
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2(FALSE);
}

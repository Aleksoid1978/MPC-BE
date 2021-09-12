/*
* (C) 2021 see Authors.txt
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
#include "MainFrm.h"
#include "MediaControls.h"
#include "DSUtil/SysVersion.h"

#include <systemmediatransportcontrolsinterop.h>

using namespace Windows::Foundation;
using namespace ABI::Windows::Media;
using namespace ABI::Windows::Storage::Streams;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

#pragma comment(lib, "runtimeobject.lib")

bool CMediaControls::Init(CMainFrame* pMainFrame)
{
	if (!SysVersion::IsWin81orLater()) {
		DLog(L"CMediaControls::Init() : Windows 8.1 or later is required for Media Key Support");
		return false;
	}

	ComPtr<ISystemMediaTransportControlsInterop> pInterop;
	HRESULT hr = GetActivationFactory(HStringReference(RuntimeClass_Windows_Media_SystemMediaTransportControls).Get(),
									  pInterop.GetAddressOf());
	if (FAILED(hr)) {
		DLog(L"CMediaControls::Init() : GetActivationFactory() failed");
		return false;
	}

	hr = pInterop->GetForWindow(pMainFrame->GetSafeHwnd(),
								IID_PPV_ARGS(m_pControls.GetAddressOf()));
	if (FAILED(hr)) {
		DLog(L"CMediaControls::Init() : ISystemMediaTransportControlsInterop::GetForWindow() failed");
		return false;
	}

	hr = m_pControls->get_DisplayUpdater(m_pDisplay.GetAddressOf());
	if (FAILED(hr)) {
		DLog(L"CMediaControls::Init() : ISystemMediaTransportControls::get_DisplayUpdater() failed");
		return false;
	}

	hr = m_pControls->put_IsEnabled(true);
	if (FAILED(hr)) {
		DLog(L"CMediaControls::Init() : ISystemMediaTransportControls::put_IsEnabled() failed");
		return false;
	}

	hr = m_pControls->put_PlaybackStatus(ABI::Windows::Media::MediaPlaybackStatus_Stopped);
	if (FAILED(hr)) {
		DLog(L"CMediaControls::Init() : ISystemMediaTransportControls::put_PlaybackStatus() failed");
		return false;
	}

	m_pMainFrame = pMainFrame;
	m_bInitialized = true;
	return true;
}

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

	hr = m_pControls->put_PlaybackStatus(ABI::Windows::Media::MediaPlaybackStatus_Closed);
	if (FAILED(hr)) {
		DLog(L"CMediaControls::Init() : ISystemMediaTransportControls::put_PlaybackStatus() failed");
		return false;
	}

	m_pMainFrame = pMainFrame;
	m_bInitialized = true;

	return true;
}

bool CMediaControls::Update()
{
	if (m_pMainFrame->m_eMediaLoadState == MLS_LOADED) {
		auto title = m_pMainFrame->GetTextForBar(AfxGetAppSettings().iTitleBarTextStyle);
		if (m_pMainFrame->m_bAudioOnly) {
			m_pDisplay->put_Type(MediaPlaybackType::MediaPlaybackType_Music);

			ComPtr<IMusicDisplayProperties> pMusicDisplayProperties;
			m_pDisplay->get_MusicProperties(pMusicDisplayProperties.GetAddressOf());
			pMusicDisplayProperties->put_Title(HStringReference(title).Get());
		} else {
			m_pDisplay->put_Type(MediaPlaybackType::MediaPlaybackType_Video);

			ComPtr<IVideoDisplayProperties> pVideoDisplayProperties;
			m_pDisplay->get_VideoProperties(pVideoDisplayProperties.GetAddressOf());
			pVideoDisplayProperties->put_Title(HStringReference(title).Get());
		}
		m_pDisplay->Update();
	} else {
		m_pDisplay->ClearAll();
	}

	return UpdateButtons();
}

bool CMediaControls::UpdateButtons()
{
	if (m_pMainFrame->m_eMediaLoadState == MLS_LOADED) {
		m_pControls->put_IsEnabled(true);

		auto fs = m_pMainFrame->GetMediaState();
		switch (fs) {
			case State_Running:
				m_pControls->put_PlaybackStatus(ABI::Windows::Media::MediaPlaybackStatus_Playing);

				m_pControls->put_IsPlayEnabled(false);
				m_pControls->put_IsPauseEnabled(true);
				m_pControls->put_IsStopEnabled(true);

				break;
			case State_Paused:
				m_pControls->put_PlaybackStatus(ABI::Windows::Media::MediaPlaybackStatus_Paused);

				m_pControls->put_IsPlayEnabled(true);
				m_pControls->put_IsPauseEnabled(false);
				m_pControls->put_IsStopEnabled(true);

				break;
			case State_Stopped:
				m_pControls->put_PlaybackStatus(ABI::Windows::Media::MediaPlaybackStatus_Stopped);

				m_pControls->put_IsPlayEnabled(true);
				m_pControls->put_IsPauseEnabled(false);
				m_pControls->put_IsStopEnabled(false);

				break;
		}

		const bool bEnabled = m_pMainFrame->IsNavigateSkipEnabled();
		m_pControls->put_IsPreviousEnabled(bEnabled);
		m_pControls->put_IsNextEnabled(bEnabled);
	} else {
		m_pControls->put_PlaybackStatus(ABI::Windows::Media::MediaPlaybackStatus_Closed);
		m_pControls->put_IsEnabled(false);
	}

	return true;
}

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
#include "Misc.h"
#include "DSUtil/SysVersion.h"

#include <shcore.h>
#include <systemmediatransportcontrolsinterop.h>

using namespace Windows::Foundation;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Media;
using namespace ABI::Windows::Storage::Streams;

#pragma comment(lib, "runtimeobject.lib")
#pragma comment(lib, "ShCore.lib")

CMediaControls::~CMediaControls()
{
	if (m_pControls && m_EventRegistrationToken.value) {
		m_pControls->remove_ButtonPressed(m_EventRegistrationToken);
	}
}

bool CMediaControls::Init(CMainFrame* pMainFrame)
{
	if (!SysVersion::IsWin81orLater()) {
		DLog(L"CMediaControls::Init() : Windows 8.1 or later is required for Media Key Support");
		return false;
	}

	if (m_bInitialized) {
		return true;
	}

	ComPtr<ISystemMediaTransportControlsInterop> pInterop;
	HRESULT hr = GetActivationFactory(HStringReference(RuntimeClass_Windows_Media_SystemMediaTransportControls).Get(),
									  pInterop.GetAddressOf());
	if (FAILED(hr)) {
		DLog(L"CMediaControls::Init() : GetActivationFactory(ISystemMediaTransportControlsInterop) failed");
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

	hr = ActivateInstance(HStringReference(RuntimeClass_Windows_Storage_Streams_InMemoryRandomAccessStream).Get(),
						  m_pImageStream.GetAddressOf());
	if (FAILED(hr)) {
		DLog(L"CMediaControls::Init() : ActivateInstance(IRandomAccessStream) failed");
		return false;
	}

	ComPtr<IRandomAccessStreamReferenceStatics> pStreamRefFactory;
	hr = GetActivationFactory(HStringReference(RuntimeClass_Windows_Storage_Streams_RandomAccessStreamReference).Get(),
							  pStreamRefFactory.GetAddressOf());
	if (FAILED(hr)) {
		DLog(L"CMediaControls::Init() : GetActivationFactory(IRandomAccessStreamReferenceStatics) failed");
		return false;
	}

	hr = pStreamRefFactory->CreateFromStream(m_pImageStream.Get(),
											 m_pImageStreamReference.GetAddressOf());
	if (FAILED(hr)) {
		DLog(L"CMediaControls::Init() : CreateFromStream() failed");
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

	auto callbackButtonPressed = Callback<ABI::Windows::Foundation::ITypedEventHandler<SystemMediaTransportControls*, SystemMediaTransportControlsButtonPressedEventArgs*>>(
										  [this](ISystemMediaTransportControls*, ISystemMediaTransportControlsButtonPressedEventArgs* pArgs) {
		HRESULT hr = S_OK;
		SystemMediaTransportControlsButton mediaButton;
		if (FAILED(hr = pArgs->get_Button(&mediaButton))) {
			DLog(L"CMediaControls::Init() : ISystemMediaTransportControls::callback::get_Button() failed");
			return hr;
		}

		OnButtonPressed(mediaButton);
		return S_OK;
	});
	hr = m_pControls->add_ButtonPressed(callbackButtonPressed.Get(), &m_EventRegistrationToken);
	if (FAILED(hr)) {
		DLog(L"CMediaControls::Init() : ISystemMediaTransportControls::add_ButtonPressed() failed");
		return false;
	}

	BYTE* pData = nullptr;
	UINT size;
	hr = LoadResourceFile(IDF_MAIN_ICON, &pData, size);
	if (SUCCEEDED(hr)) {
		m_defaultImageData.insert(m_defaultImageData.end(), pData, pData + size);
	}

	m_pMainFrame = pMainFrame;
	m_bInitialized = true;

	return true;
}

bool CMediaControls::Update()
{
	if (!m_bInitialized) {
		return false;
	}

	if (m_pMainFrame->m_eMediaLoadState == MLS_LOADED) {
		auto title = m_pMainFrame->GetTitleOrFileNameOrPath();
		if (m_pMainFrame->m_bAudioOnly) {
			m_pDisplay->put_Type(MediaPlaybackType::MediaPlaybackType_Music);

			ComPtr<IMusicDisplayProperties> pMusicDisplayProperties;
			m_pDisplay->get_MusicProperties(pMusicDisplayProperties.GetAddressOf());
			pMusicDisplayProperties->put_Title(HStringReference(title).Get());

			for (const auto& pAMMC : m_pMainFrame->m_pAMMC) {
				if (pAMMC) {
					CComBSTR bstr;
					if (SUCCEEDED(pAMMC->get_AuthorName(&bstr)) && bstr.Length()) {
						pMusicDisplayProperties->put_Artist(HStringReference(bstr).Get());
						break;
					}
				}
			}
		} else {
			m_pDisplay->put_Type(MediaPlaybackType::MediaPlaybackType_Video);

			ComPtr<IVideoDisplayProperties> pVideoDisplayProperties;
			m_pDisplay->get_VideoProperties(pVideoDisplayProperties.GetAddressOf());
			pVideoDisplayProperties->put_Title(HStringReference(title).Get());
		}

		static std::list<LPCWSTR> mimeStrings = {
			L"image/jpeg",
			L"image/jpg",
			L"image/png",
			L"image/bmp"
		};
		std::vector<uint8_t> m_imageData;
		BeginEnumFilters(m_pMainFrame->m_pGB, pEF, pBF) {
			if (CComQIPtr<IDSMResourceBag> pRB = pBF) {
				if (pRB && m_pMainFrame->CheckMainFilter(pBF) && pRB->ResGetCount() > 0) {
					for (DWORD i = 0; i < pRB->ResGetCount() && m_imageData.empty(); i++) {
						CComBSTR mime;
						BYTE* pData = nullptr;
						DWORD len = 0;
						if (SUCCEEDED(pRB->ResGet(i, nullptr, nullptr, &mime, &pData, &len, nullptr))) {
							CString mimeStr(mime);
							mimeStr.Trim();

							if (std::find(mimeStrings.cbegin(), mimeStrings.cend(), mimeStr) != mimeStrings.cend()) {
								m_imageData.insert(m_imageData.end(), pData, pData + len);
							}

							CoTaskMemFree(pData);
						}
					}
				}
			}

			if (!m_imageData.empty()) {
				break;
			}
		}
		EndEnumFilters;

		if (m_imageData.empty() && !m_pMainFrame->m_youtubeThumbnailData.empty()) {
			m_imageData = m_pMainFrame->m_youtubeThumbnailData;
		}

		if (!m_imageData.empty()) {
			ComPtr<IStream> stream;
			CreateStreamOverRandomAccessStream(m_pImageStream.Get(), IID_PPV_ARGS(stream.GetAddressOf()));
			stream->Write(m_imageData.data(), m_imageData.size(), nullptr);

			m_pDisplay->put_Thumbnail(m_pImageStreamReference.Get());
		} else if (!m_defaultImageData.empty()) {
			ComPtr<IStream> stream;
			CreateStreamOverRandomAccessStream(m_pImageStream.Get(), IID_PPV_ARGS(stream.GetAddressOf()));
			stream->Write(m_defaultImageData.data(), m_defaultImageData.size(), nullptr);

			m_pDisplay->put_Thumbnail(m_pImageStreamReference.Get());
		}

		m_pDisplay->Update();
	} else {
		m_pDisplay->ClearAll();
	}

	return UpdateButtons();
}

bool CMediaControls::UpdateButtons()
{
	if (!m_bInitialized) {
		return false;
	}

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

void CMediaControls::OnButtonPressed(SystemMediaTransportControlsButton mediaButton)
{
	switch (mediaButton) {
		case SystemMediaTransportControlsButton_Play:
			m_pMainFrame->PostMessageW(WM_COMMAND, ID_PLAY_PLAY);
			break;
		case SystemMediaTransportControlsButton_Pause:
			m_pMainFrame->PostMessageW(WM_COMMAND, ID_PLAY_PAUSE);
			break;
		case SystemMediaTransportControlsButton_Stop:
			m_pMainFrame->PostMessageW(WM_COMMAND, ID_PLAY_STOP);
			break;
		case SystemMediaTransportControlsButton_Previous:
			m_pMainFrame->PostMessageW(WM_COMMAND, ID_NAVIGATE_SKIPBACK);
			break;
		case SystemMediaTransportControlsButton_Next:
			m_pMainFrame->PostMessageW(WM_COMMAND, ID_NAVIGATE_SKIPFORWARD);
			break;
	}
}

/*
* (C) 2021-2024 see Authors.txt
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

#include <Windows.Media.h>
#include <wrl.h>

class CMainFrame;

class CMediaControls final
{
	bool m_bInitialized = false;
	CMainFrame* m_pMainFrame = nullptr;

	Microsoft::WRL::ComPtr<ABI::Windows::Media::ISystemMediaTransportControls> m_pControls;
	Microsoft::WRL::ComPtr<ABI::Windows::Media::ISystemMediaTransportControlsDisplayUpdater> m_pDisplay;

	Microsoft::WRL::ComPtr<ABI::Windows::Storage::Streams::IRandomAccessStream> m_pImageStream;
	Microsoft::WRL::ComPtr<ABI::Windows::Storage::Streams::IRandomAccessStreamReference> m_pImageStreamReference;

	std::vector<uint8_t> m_defaultImageData;

	EventRegistrationToken m_EventRegistrationToken = {};

	void OnButtonPressed(ABI::Windows::Media::SystemMediaTransportControlsButton mediaButton);

public:
	CMediaControls() = default;
	CMediaControls(CMediaControls&&) = delete;
	CMediaControls(const CMediaControls&) = delete;
	CMediaControls& operator=(CMediaControls&&) = delete;
	CMediaControls& operator=(const CMediaControls&) = delete;
	~CMediaControls();

	bool Init(CMainFrame* pMainFrame);
	bool Update();
	bool UpdateButtons();
};


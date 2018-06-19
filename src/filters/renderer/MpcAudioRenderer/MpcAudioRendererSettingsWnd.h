/*
 * (C) 2010-2018 see Authors.txt
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

#include "../../filters/InternalPropertyPage.h"
#include "AudioDevice.h"
#include "IMpcAudioRenderer.h"
#include "resource.h"

class __declspec(uuid("1E53BA32-3BCC-4dff-9342-34E46BE3F5A5"))
	CMpcAudioRendererSettingsWnd : public CInternalPropertyPageWnd
{
private :
	CComQIPtr<IMpcAudioRendererFilter> m_pMAR;

	AudioDevices::devicesList m_devicesList;

	CButton		m_output_group;
	CStatic		m_txtWasapiMode;
	CComboBox	m_cbWasapiMode;
	CStatic		m_txtDevicePeriod;
	CComboBox	m_cbDevicePeriod;

	CButton		m_cbUseBitExactOutput;
	CButton		m_cbUseSystemLayoutChannels;
	CButton		m_cbReleaseDeviceIdle;
	CButton		m_cbUseCrossFeed;

	CStatic		m_txtSoundDevice;
	CComboBox	m_cbSoundDevice;

	enum {
		IDC_PP_SOUND_DEVICE = 10000,
		IDC_PP_WASAPI_MODE,
		IDC_PP_WASAPI_DEVICE_PERIOD,
		IDC_PP_USE_BITEXACT_OUTPUT,
		IDC_PP_USE_SYSTEM_LAYOUT_CHANNELS,
		IDC_PP_FREE_DEVICE_INACTIVE,
		IDC_PP_USE_CROSSFEED
	};

public:
	CMpcAudioRendererSettingsWnd();

	bool OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks);
	void OnDisconnect();
	bool OnActivate();
	void OnDeactivate();
	bool OnApply();

	static LPCWSTR GetWindowTitle() { return MAKEINTRESOURCEW(IDS_FILTER_SETTINGS_CAPTION); }
	static CSize GetWindowSize() { return CSize(340, 211); }

	DECLARE_MESSAGE_MAP()

	afx_msg void OnClickedWasapiMode();
	afx_msg void OnClickedBitExact();
};


class __declspec(uuid("E3D0704B-1579-4E9E-8674-2674CB90D07A"))
	CMpcAudioRendererStatusWnd : public CInternalPropertyPageWnd
{
private :
	CComQIPtr<IMpcAudioRendererFilter> m_pMAR;

	CStatic		m_txtDevice;
	CEdit		m_edtDevice;
	CStatic		m_txtMode;
	CEdit		m_edtMode;

	CButton		m_grpInput;
	CStatic		m_InputFormatLabel;
	CStatic		m_InputFormatText;
	CStatic		m_InputChannelLabel;
	CStatic		m_InputChannelText;
	CStatic		m_InputRateLabel;
	CStatic		m_InputRateText;

	CButton		m_grpOutput;
	CStatic		m_OutputFormatLabel;
	CStatic		m_OutputFormatText;
	CStatic		m_OutputChannelLabel;
	CStatic		m_OutputChannelText;
	CStatic		m_OutputRateLabel;
	CStatic		m_OutputRateText;

	void UpdateStatus();

protected:
	BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;

public:
	CMpcAudioRendererStatusWnd();

	bool OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks);
	void OnDisconnect();
	bool OnActivate();
	void OnDeactivate();

	static LPCWSTR GetWindowTitle() { return MAKEINTRESOURCEW(IDS_ARS_STATUS); }
	static CSize GetWindowSize() { return CSize(392, 143); }

	DECLARE_MESSAGE_MAP()
};

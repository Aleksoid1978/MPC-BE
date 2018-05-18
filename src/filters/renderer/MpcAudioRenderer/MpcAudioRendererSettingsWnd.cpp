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

#include "stdafx.h"
#include <mmdeviceapi.h>
#include <FunctionDiscoveryKeys_devpkey.h>
#include <ks.h>
#include <ksmedia.h>
#include "MpcAudioRendererSettingsWnd.h"
#include "../../../DSUtil/DSUtil.h"
#include "../../../DSUtil/AudioParser.h"


//
// CMpcAudioRendererSettingsWnd
//

CMpcAudioRendererSettingsWnd::CMpcAudioRendererSettingsWnd()
{
}

bool CMpcAudioRendererSettingsWnd::OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks)
{
	ASSERT(!m_pMAR);

	m_pMAR.Release();

	POSITION pos = pUnks.GetHeadPosition();
	while (pos && !(m_pMAR = pUnks.GetNext(pos))) {
		;
	}

	if (!m_pMAR) {
		return false;
	}

	return true;
}

void CMpcAudioRendererSettingsWnd::OnDisconnect()
{
	m_pMAR.Release();
}

bool CMpcAudioRendererSettingsWnd::OnActivate()
{
	const int h20 = ScaleY(20);
	const int h25 = ScaleY(25);
	const int h30 = ScaleY(30);
	CPoint p(10, 10);

	m_txtSoundDevice.Create(ResStr(IDS_ARS_SOUND_DEVICE), WS_VISIBLE | WS_CHILD, CRect(p, CSize(ScaleX(320), m_fontheight)), this, (UINT)IDC_STATIC);
	p.y += h20;
	m_cbSoundDevice.Create(WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, CRect(p, CSize(ScaleX(320), 200)), this, IDC_PP_SOUND_DEVICE);
	p.y += h20 + h20;

	m_txtWasapiMode.Create(ResStr(IDS_ARS_WASAPI_MODE), WS_VISIBLE | WS_CHILD, CRect(p, CSize(ScaleX(200), m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbWasapiMode.Create(WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, CRect(p + CPoint(ScaleX(200), -4), CSize(ScaleX(120), 200)), this, IDC_PP_WASAPI_MODE);
	m_cbWasapiMode.AddString(ResStr(IDS_ARS_SHARED));
	m_cbWasapiMode.AddString(ResStr(IDS_ARS_EXCLUSIVE));
	p.y += h20;
	m_cbUseBitExactOutput.Create(ResStr(IDS_ARS_BITEXACT_OUTPUT), WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | BS_LEFTTEXT, CRect(p, CSize(ScaleX(320), m_fontheight)), this, IDC_PP_USE_BITEXACT_OUTPUT);
	p.y += h20;
	m_cbUseSystemLayoutChannels.Create(ResStr(IDS_ARS_SYSTEM_LAYOUT_CHANNELS), WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | BS_LEFTTEXT, CRect(p, CSize(ScaleX(320), m_fontheight)), this, IDC_PP_USE_SYSTEM_LAYOUT_CHANNELS);
	p.y += h20;
	m_cbReleaseDeviceIdle.Create(ResStr(IDS_ARS_RELEASE_DEVICE_IDLE), WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | BS_LEFTTEXT, CRect(p, CSize(ScaleX(320), m_fontheight)), this, IDC_PP_FREE_DEVICE_INACTIVE);
	p.y += h20;
	m_cbUseCrossFeed.Create(ResStr(IDS_ARS_CROSSFEED), WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | BS_LEFTTEXT, CRect(p, CSize(ScaleX(320), m_fontheight)), this, IDC_PP_USE_CROSSFEED);

	AudioDevices::GetActiveAudioDevices(&m_devicesList, NULL, TRUE);
	for (const auto& device : m_devicesList) {
		m_cbSoundDevice.AddString(device.first);
	}

	if (m_pMAR) {
		if (m_cbSoundDevice.GetCount() > 0) {
			int idx = 0;
			CString soundDeviceId = m_pMAR->GetDeviceId();
			for (size_t i = 0; i < m_devicesList.size(); i++) {
				if (m_devicesList[i].second == soundDeviceId) {
					idx = i;
					break;
				}
			}

			m_cbSoundDevice.SetCurSel(idx);
		}
		m_cbWasapiMode.SetCurSel(m_pMAR->GetWasapiMode());
		m_cbUseBitExactOutput.SetCheck(m_pMAR->GetBitExactOutput());
		m_cbUseSystemLayoutChannels.SetCheck(m_pMAR->GetSystemLayoutChannels());
		m_cbReleaseDeviceIdle.SetCheck(m_pMAR->GetReleaseDeviceIdle());
		m_cbUseCrossFeed.SetCheck(m_pMAR->GetCrossFeed());
	}

	for (CWnd* pWnd = GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetNextWindow()) {
		pWnd->SetFont(&m_font, FALSE);
	}

	CorrectComboListWidth(m_cbSoundDevice);

	SetCursor(m_hWnd, IDC_ARROW);
	SetCursor(m_hWnd, IDC_PP_SOUND_DEVICE, IDC_HAND);

	OnClickedWasapiMode();

	return true;
}

void CMpcAudioRendererSettingsWnd::OnDeactivate()
{
}

bool CMpcAudioRendererSettingsWnd::OnApply()
{
	OnDeactivate();

	if (m_pMAR) {
		m_pMAR->SetWasapiMode(m_cbWasapiMode.GetCurSel());
		m_pMAR->SetBitExactOutput(m_cbUseBitExactOutput.GetCheck());
		m_pMAR->SetSystemLayoutChannels(m_cbUseSystemLayoutChannels.GetCheck());
		m_pMAR->SetReleaseDeviceIdle(m_cbReleaseDeviceIdle.GetCheck());
		m_pMAR->SetCrossFeed(m_cbUseCrossFeed.GetCheck());
		int idx = m_cbSoundDevice.GetCurSel();
		if (idx >= 0) {
			m_pMAR->SetDeviceId(m_devicesList[idx].second);
		}
		m_pMAR->Apply();
	}

	return true;
}

BEGIN_MESSAGE_MAP(CMpcAudioRendererSettingsWnd, CInternalPropertyPageWnd)
	ON_CBN_SELCHANGE(IDC_PP_WASAPI_MODE, OnClickedWasapiMode)
	ON_BN_CLICKED(IDC_PP_USE_BITEXACT_OUTPUT, OnClickedBitExact)
END_MESSAGE_MAP()


void CMpcAudioRendererSettingsWnd::OnClickedWasapiMode()
{
	int selected = m_cbWasapiMode.GetCurSel();
	m_cbUseBitExactOutput.EnableWindow(selected == (int)MODE_WASAPI_EXCLUSIVE);
	OnClickedBitExact();

	m_cbReleaseDeviceIdle.EnableWindow(selected == (int)MODE_WASAPI_EXCLUSIVE);
}

void CMpcAudioRendererSettingsWnd::OnClickedBitExact()
{
	m_cbUseSystemLayoutChannels.EnableWindow(m_cbUseBitExactOutput.GetCheck() && m_cbUseBitExactOutput.IsWindowEnabled());
}


//
// CMpcAudioRendererStatusWnd
//

void CMpcAudioRendererStatusWnd::UpdateStatus()
{
	DLog(L"CMpcAudioRendererStatusWnd: UpdateStatus");

	if (m_pMAR) {
		m_edtDevice.SetWindowTextW(m_pMAR->GetCurrentDeviceName());

		UINT status = m_pMAR->GetMode();
		switch (status) {
		case MODE_NONE:
		default:
			m_edtMode.SetWindowTextW(L"");
			break;
		case MODE_WASAPI_SHARED:
			m_edtMode.SetWindowTextW(L"WASAPI Shared");
			break;
		case MODE_WASAPI_EXCLUSIVE:
			m_edtMode.SetWindowTextW(L"WASAPI Exclusive");
			break;
		case MODE_WASAPI_EXCLUSIVE_BITSTREAM:
			CString btMode_str;
			BITSTREAM_MODE btMode = m_pMAR->GetBitstreamMode();
			switch (btMode) {
			case BITSTREAM_AC3:    btMode_str = L"AC3";    break;
			case BITSTREAM_DTS:    btMode_str = L"DTS";    break;
			case BITSTREAM_EAC3:   btMode_str = L"E-AC3";  break;
			case BITSTREAM_TRUEHD: btMode_str = L"TrueHD"; break;
			case BITSTREAM_DTSHD:  btMode_str = L"DTS-HD"; break;
			}

			CString msg = L"WASAPI Exclusive, Bitstream";
			if (!btMode_str.IsEmpty()) {
				msg.AppendFormat(L" [%s]", btMode_str);
			}
			m_edtMode.SetWindowTextW(msg);
			break;
		}

		if (status == MODE_WASAPI_EXCLUSIVE_BITSTREAM) {
			m_InputFormatText.SetWindowTextW(L"Bitstream");
			m_OutputFormatText.SetWindowTextW(L"Bitstream");
		}
		else {
			WAVEFORMATEX *pWfxIn, *pWfxOut;
			m_pMAR->GetStatus(&pWfxIn, &pWfxOut);
			if (pWfxIn && pWfxOut) {
				auto SetText = [](WAVEFORMATEX* pwfex, CStatic& formatText, CStatic& channelText, CStatic& rateText) {
					bool bIsFloat = false;
					bool m_b24PaddedTo32bit = false;
					DWORD layout = 0;
					if (IsWaveFormatExtensible(pwfex)) {
						WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)pwfex;
						layout = wfex->dwChannelMask;
						if (wfex->SubFormat == MEDIASUBTYPE_IEEE_FLOAT) {
							bIsFloat = true;
						}
						else {
							m_b24PaddedTo32bit = wfex->Samples.wValidBitsPerSample == 24 && pwfex->wBitsPerSample == 32;
						}
					}
					else {
						layout = GetDefChannelMask(pwfex->nChannels);
						if (pwfex->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
							bIsFloat = true;
						}
					}

					CString sFormat;
					sFormat.Format(L"%ubit %s%s", m_b24PaddedTo32bit ? 24 : pwfex->wBitsPerSample, m_b24PaddedTo32bit ? L"[padded] " : L"", bIsFloat ? L"Float" : L"Integer");

					BYTE lfe = 0;
					WORD nChannels = pwfex->nChannels;
					if (layout & SPEAKER_LOW_FREQUENCY) {
						nChannels--;
						lfe = 1;
					}

					CString sChannel;
					sChannel.Format(L"%u.%u / 0x%x", nChannels, lfe, layout);

					CString sSampleRate;
					sSampleRate.Format(L"%u", pwfex->nSamplesPerSec);

					formatText.SetWindowTextW(sFormat);
					channelText.SetWindowTextW(sChannel);
					rateText.SetWindowTextW(sSampleRate);
				};

				SetText(pWfxIn, m_InputFormatText, m_InputChannelText, m_InputRateText);
				SetText(pWfxOut, m_OutputFormatText, m_OutputChannelText, m_OutputRateText);
			}
		}
	}
}

BOOL CMpcAudioRendererStatusWnd::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	// each page activation receives 3 unique messages:
	// WM_WINDOWPOSCHANGING, WM_PAINT, WM_NCPAINT, which does not occur in other cases
	if (message == WM_WINDOWPOSCHANGING) {
		UpdateStatus();
	}

	return CWnd::OnWndMsg(message, wParam, lParam, pResult);
}

CMpcAudioRendererStatusWnd::CMpcAudioRendererStatusWnd()
{
}

bool CMpcAudioRendererStatusWnd::OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks)
{
	ASSERT(!m_pMAR);

	m_pMAR.Release();

	POSITION pos = pUnks.GetHeadPosition();
	while (pos && !(m_pMAR = pUnks.GetNext(pos))) {
		;
	}

	if (!m_pMAR) {
		return false;
	}

	return true;
}

void CMpcAudioRendererStatusWnd::OnDisconnect()
{
	m_pMAR.Release();
}

bool CMpcAudioRendererStatusWnd::OnActivate()
{
	CRect rect;
	const long w1 = 70;  // device, mode
	const long w2 = 280; // values
	const long w3 = 70;  // format, channels, samplerate
	const long w4 = 98; // values

	const long x1 = 10;
	const long x2 = x1 + w3;
	const long x3 = x2 + w4 + 12;
	const long x4 = x3 + w3;
	long y = 10;

	CalcTextRect(rect, x1, y, w1);
	m_txtDevice.Create(ResStr(IDS_ARS_DEVICE), WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	CalcTextRect(rect, x1+w1, y, w2);
	m_edtDevice.Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY, rect, this, (UINT)IDC_STATIC);

	y += 20;
	CalcTextRect(rect, x1, y, w1);
	m_txtMode.Create(ResStr(IDS_ARS_MODE), WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	CalcTextRect(rect, x1+w1, y, w2);
	m_edtMode.Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY, rect, this, (UINT)IDC_STATIC);

	y += 20;
	CalcRect(rect, x1-5, y, w3+w4+7, 80);
	m_grpInput.Create(ResStr(IDS_ARS_INPUT), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, rect, this, (UINT)IDC_STATIC);
	CalcRect(rect, x3-5, y, w3+w4+7, 80);
	m_grpOutput.Create(ResStr(IDS_ARS_OUTPUT), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, rect, this, (UINT)IDC_STATIC);

	// Format
	y += 20;
	CalcTextRect(rect, x1, y, w3);
	m_InputFormatLabel.Create(ResStr(IDS_ARS_FORMAT), WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	CalcTextRect(rect, x2, y, w4);
	m_InputFormatText.Create(L"", WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	CalcTextRect(rect, x3, y, w3);
	m_OutputFormatLabel.Create(ResStr(IDS_ARS_FORMAT), WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	CalcTextRect(rect, x4, y, w4);
	m_OutputFormatText.Create(L"", WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);

	// Sample Rate
	y += 20;
	CalcTextRect(rect, x1, y, w3);
	m_InputRateLabel.Create(ResStr(IDS_ARS_SAMPLERATE), WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	CalcTextRect(rect, x2, y, w4);
	m_InputRateText.Create(L"", WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	CalcTextRect(rect, x3, y, w3);
	m_OutputRateLabel.Create(ResStr(IDS_ARS_SAMPLERATE), WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	CalcTextRect(rect, x4, y, w4);
	m_OutputRateText.Create(L"", WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);

	// Channel
	y += 20;
	CalcTextRect(rect, x1, y, w3);
	m_InputChannelLabel.Create(ResStr(IDS_ARS_CHANNELS), WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	CalcTextRect(rect, x2, y, w4);
	m_InputChannelText.Create(L"", WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	CalcTextRect(rect, x3, y, w3);
	m_OutputChannelLabel.Create(ResStr(IDS_ARS_CHANNELS), WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	CalcTextRect(rect, x4, y, w4);
	m_OutputChannelText.Create(L"", WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);

	for (CWnd* pWnd = GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetNextWindow()) {
		pWnd->SetFont(&m_font, FALSE);
	}

	UpdateStatus();

	SetDirty(false);

	return true;
}

void CMpcAudioRendererStatusWnd::OnDeactivate()
{
	m_edtDevice.SetSel(-1);
	m_edtMode.SetSel(-1);
}

BEGIN_MESSAGE_MAP(CMpcAudioRendererStatusWnd, CInternalPropertyPageWnd)
END_MESSAGE_MAP()

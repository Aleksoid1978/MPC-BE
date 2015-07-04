/*
 * (C) 2010-2014 see Authors.txt
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
#include "MpcAudioRendererSettingsWnd.h"
#include "../../../DSUtil/DSUtil.h"
#include "../../../DSUtil/AudioParser.h"

bool CALLBACK DSEnumProc(LPGUID lpGUID,
						 LPCTSTR lpszDesc,
						 LPCTSTR lpszDrvName,
						 LPVOID lpContext )
{
	CComboBox *pCombo = (CComboBox*)lpContext;
	ASSERT(pCombo);
	LPGUID lpTemp = NULL;

	if (lpGUID != NULL) {// NULL only for "Primary Sound Driver".
		if ((lpTemp = (LPGUID)malloc(sizeof(GUID))) == NULL) {
			return TRUE;
		}
		memcpy(lpTemp, lpGUID, sizeof(GUID));
	}
	pCombo->AddString(lpszDesc);
	free(lpTemp);

	return TRUE;
}

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
	ASSERT(IPP_FONTSIZE == 13);
	const int h20 = IPP_SCALE(20);
	const int h25 = IPP_SCALE(25);
	const int h30 = IPP_SCALE(30);
	CPoint p(10, 10);

	m_txtSoundDevice.Create(ResStr(IDS_ARS_SOUND_DEVICE), WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(320), m_fontheight)), this, (UINT)IDC_STATIC);
	p.y += h20;
	m_cbSoundDevice.Create(WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, CRect(p, CSize(IPP_SCALE(320), 200)), this, IDC_PP_SOUND_DEVICE);
	p.y += h20 + h20;

	m_txtWasapiMode.Create(ResStr(IDS_ARS_WASAPI_MODE), WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(200), m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbWasapiMode.Create(WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, CRect(p + CPoint(200, -4), CSize(IPP_SCALE(120), 200)), this, IDC_PP_WASAPI_MODE);
	m_cbWasapiMode.AddString(ResStr(IDS_ARS_EXCLUSIVE));
	m_cbWasapiMode.AddString(ResStr(IDS_ARS_SHARED));
	p.y += h20;
	m_cbUseBitExactOutput.Create(ResStr(IDS_ARS_BITEXACT_OUTPUT), WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | BS_LEFTTEXT, CRect(p, CSize(IPP_SCALE(320), m_fontheight)), this, IDC_PP_USE_BITEXACT_OUTPUT);
	p.y += h20;
	m_cbUseSystemLayoutChannels.Create(ResStr(IDS_ARS_SYSTEM_LAYOUT_CHANNELS), WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | BS_LEFTTEXT, CRect(p, CSize(IPP_SCALE(320), m_fontheight)), this, IDC_PP_USE_SYSTEM_LAYOUT_CHANNELS);
	p.y += h30;

	m_txtSyncMethod.Create(ResStr(IDS_ARS_SYNC_METHOD), WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(320), m_fontheight)), this, (UINT)IDC_STATIC);
	p.y += h20;
	m_cbSyncMethod.Create(WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, CRect(p, CSize(IPP_SCALE(320), 200)), this, IDC_PP_SYNC_METHOD);
	m_cbSyncMethod.AddString(ResStr(IDS_ARS_SYNC_METHOD_TO_VIDEO));
	m_cbSyncMethod.AddString(ResStr(IDS_ARS_SYNC_METHOD_TO_AUDIO));

	HMODULE hModule = LoadLibrary(L"dsound.dll");
	if (hModule) {
		HRESULT (__stdcall * pDirectSoundEnumerate)(__in LPDSENUMCALLBACKW pDSEnumCallback, __in_opt LPVOID pContext);
		(FARPROC &)pDirectSoundEnumerate = GetProcAddress(hModule, "DirectSoundEnumerateW");
		if (pDirectSoundEnumerate) {
			pDirectSoundEnumerate((LPDSENUMCALLBACK)DSEnumProc, (LPVOID)&m_cbSoundDevice);
		}

		FreeLibrary(hModule);
	}

	if (m_pMAR) {
		if (m_cbSoundDevice.GetCount() > 0) {
			int idx = m_cbSoundDevice.FindString(0, m_pMAR->GetSoundDevice());
			if (idx < 0) {
				m_cbSoundDevice.SetCurSel(0);
			} else {
				m_cbSoundDevice.SetCurSel(m_cbSoundDevice.FindString(0, m_pMAR->GetSoundDevice()));
			}
		}
		m_cbWasapiMode.SetCurSel(m_pMAR->GetWasapiMode());
		m_cbUseBitExactOutput.SetCheck(m_pMAR->GetBitExactOutput());
		m_cbUseSystemLayoutChannels.SetCheck(m_pMAR->GetSystemLayoutChannels());
		m_cbSyncMethod.SetCurSel(m_pMAR->GetSyncMethod());
	}

	for (CWnd* pWnd = GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetNextWindow()) {
		pWnd->SetFont(&m_font, FALSE);
	}

	CorrectComboListWidth(m_cbSoundDevice);

	SetClassLongPtr(m_hWnd, GCLP_HCURSOR, (LONG)AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	SetClassLongPtr(GetDlgItem(IDC_PP_SOUND_DEVICE)->m_hWnd, GCLP_HCURSOR, (LONG)AfxGetApp()->LoadStandardCursor(IDC_HAND));
	SetClassLongPtr(GetDlgItem(IDC_PP_SYNC_METHOD)->m_hWnd, GCLP_HCURSOR, (LONG)AfxGetApp()->LoadStandardCursor(IDC_HAND));

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
		m_pMAR->SetSyncMethod(m_cbSyncMethod.GetCurSel());
		CString str;
		int idx = m_cbSoundDevice.GetCurSel();
		if (idx >= 0) {
			m_cbSoundDevice.GetLBText(idx, str);
			m_pMAR->SetSoundDevice(str);
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
}

void CMpcAudioRendererSettingsWnd::OnClickedBitExact()
{
	m_cbUseSystemLayoutChannels.EnableWindow(m_cbUseBitExactOutput.GetCheck() && m_cbUseBitExactOutput.IsWindowEnabled());
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
	ASSERT(IPP_FONTSIZE == 13);
	const int h20 = IPP_SCALE(20);
	CPoint p(10, 10);

	m_gInput.Create(L"Input", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, CRect(p + CPoint(-5, 0), CSize(IPP_SCALE(150), h20 * 4 + IPP_SCALE(5))), this, (UINT)IDC_STATIC);

	p = CPoint(IPP_SCALE(170), 10);
	m_gOutput.Create(L"Output", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, CRect(p + CPoint(-5, 0), CSize(IPP_SCALE(150), h20 * 4 + IPP_SCALE(5))), this, (UINT)IDC_STATIC);

	{
		// Format
		p = CPoint(IPP_SCALE(10), 10);
		p.y += h20;
		m_InputFormatLabel.Create(L"Format:", WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(50), m_fontheight)), this, (UINT)IDC_STATIC);
		p.x += IPP_SCALE(70);
		m_InputFormatText.Create(L"", WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(70), m_fontheight)), this, (UINT)IDC_STATIC);

		p = CPoint(IPP_SCALE(170), 10);
		p.y += h20;
		m_OutputFormatLabel.Create(L"Format:", WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(50), m_fontheight)), this, (UINT)IDC_STATIC);
		p.x += IPP_SCALE(70);
		m_OutputFormatText.Create(L"", WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(70), m_fontheight)), this, (UINT)IDC_STATIC);
	}

	{
		// Channel
		p = CPoint(IPP_SCALE(10), 10);
		p.y += h20 * 2;
		m_InputChannelLabel.Create(L"Channel:", WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(50), m_fontheight)), this, (UINT)IDC_STATIC);
		p.x += IPP_SCALE(70);
		m_InputChannelText.Create(L"", WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(70), m_fontheight)), this, (UINT)IDC_STATIC);

		p = CPoint(IPP_SCALE(170), 10);
		p.y += h20 * 2;
		m_OutputChannelLabel.Create(L"Channel:", WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(50), m_fontheight)), this, (UINT)IDC_STATIC);
		p.x += IPP_SCALE(70);
		m_OutputChannelText.Create(L"", WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(70), m_fontheight)), this, (UINT)IDC_STATIC);
	}

	{
		// Sample Rate
		p = CPoint(IPP_SCALE(10), 10);
		p.y += h20 * 3;
		m_InputRateLabel.Create(L"Sample Rate:", WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(70), m_fontheight)), this, (UINT)IDC_STATIC);
		p.x += IPP_SCALE(70);
		m_InputRateText.Create(L"", WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(70), m_fontheight)), this, (UINT)IDC_STATIC);

		p = CPoint(IPP_SCALE(170), 10);
		p.y += h20 * 3;
		m_OutputRateLabel.Create(L"Sample Rate:", WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(70), m_fontheight)), this, (UINT)IDC_STATIC);
		p.x += IPP_SCALE(70);
		m_OutputRateText.Create(L"", WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(70), m_fontheight)), this, (UINT)IDC_STATIC);
	}

	p = CPoint(IPP_SCALE(10), 10);
	p.y += h20 * 4 + IPP_SCALE(15);
	m_ModeText.Create(ResStr(IDS_ARS_WASAPI_MODE_STATUS_1), WS_VISIBLE | WS_CHILD, CRect(p, CSize(IPP_SCALE(300), m_fontheight)), this, (UINT)IDC_STATIC);

	p.y += IPP_SCALE(15);
	m_CurrentDeviceText.Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY, CRect(p, CSize(IPP_SCALE(300), m_fontheight)), this, 0);

	if (m_pMAR) {
		UINT status = m_pMAR->GetMode();
		switch (status) {
			case MODE_NONE :
			default:
				break;
			case MODE_WASAPI_EXCLUSIVE :
				m_ModeText.SetWindowText(ResStr(IDS_ARS_WASAPI_MODE_STATUS_2));
				break;
			case MODE_WASAPI_SHARED :
				m_ModeText.SetWindowText(ResStr(IDS_ARS_WASAPI_MODE_STATUS_3));
				break;
			case MODE_WASAPI_EXCLUSIVE_BITSTREAM :
				CString btMode_str;

				BITSTREAM_MODE btMode = m_pMAR->GetBitstreamMode();
				switch (btMode) {
					case BITSTREAM_AC3 : btMode_str = L"AC3";
						break;
					case BITSTREAM_DTS : btMode_str = L"DTS";
						break;
					case BITSTREAM_EAC3 : btMode_str = L"E-AC3";
						break;
					case BITSTREAM_TRUEHD : btMode_str = L"TrueHD";
						break;
					case BITSTREAM_DTSHD : btMode_str = L"DTS-HD";
						break;
				}

				CString msg = ResStr(IDS_ARS_WASAPI_MODE_STATUS_5);
				if (btMode_str.GetLength()) {
					msg.AppendFormat(L" [%s]", btMode_str);
				}
				m_ModeText.SetWindowText(msg);
				break;
		}

		if (status == MODE_WASAPI_EXCLUSIVE_BITSTREAM) {
			m_InputFormatText.SetWindowText(L"Bitstream");
			m_OutputFormatText.SetWindowText(L"Bitstream");
		} else {
			WAVEFORMATEX *pWfxIn, *pWfxOut;
			m_pMAR->GetStatus(&pWfxIn, &pWfxOut);
			if (pWfxIn && pWfxOut) {
				{
					// Input
					bool bIsFloat	= false;
					DWORD layout	= 0;
					if (IsWaveFormatExtensible(pWfxIn)) {
						WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)pWfxIn;
						layout = wfex->dwChannelMask;
						if (wfex->SubFormat == MEDIASUBTYPE_IEEE_FLOAT) {
							bIsFloat = true;
						}
					} else {
						layout = GetDefChannelMask(pWfxIn->nChannels);
						if (pWfxIn->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
							bIsFloat = true;
						}
					}

					CString sFormat;
					sFormat.Format(L"%dbit %s", pWfxIn->wBitsPerSample, bIsFloat ? L"Float" : L"Integer");

					CString sChannel;
					switch (pWfxIn->nChannels) {
						case 6:
							sChannel = L"5.1";
							break;
						case 8:
							sChannel = L"7.1";
							break;
						default:
							sChannel.Format(L"%d", pWfxIn->nChannels);
							break;
					}
					sChannel.AppendFormat(L" / 0x%x", layout);

					CString sSampleRate;
					sSampleRate.Format(L"%d", pWfxIn->nSamplesPerSec);

					m_InputFormatText.SetWindowText(sFormat);
					m_InputChannelText.SetWindowText(sChannel);
					m_InputRateText.SetWindowText(sSampleRate);
				}

				{
					// Output
					bool bIsFloat	= false;
					DWORD layout	= 0;
					if (IsWaveFormatExtensible(pWfxOut)) {
						WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)pWfxOut;
						layout = wfex->dwChannelMask;
						if (wfex->SubFormat == MEDIASUBTYPE_IEEE_FLOAT) {
							bIsFloat = true;
						}
					} else {
						layout = GetDefChannelMask(pWfxOut->nChannels);
						if (pWfxOut->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
							bIsFloat = true;
						}
					}

					CString sFormat;
					sFormat.Format(L"%dbit %s", pWfxOut->wBitsPerSample, bIsFloat ? L"Float" : L"Integer");

					CString sChannel;
					switch (pWfxOut->nChannels) {
						case 6:
							sChannel = L"5.1";
							break;
						case 8:
							sChannel = L"7.1";
							break;
						default:
							sChannel.Format(L"%d", pWfxOut->nChannels);
							break;
					}
					sChannel.AppendFormat(L" / 0x%x", layout);

					CString sSampleRate;
					sSampleRate.Format(L"%d", pWfxOut->nSamplesPerSec);

					m_OutputFormatText.SetWindowText(sFormat);
					m_OutputChannelText.SetWindowText(sChannel);
					m_OutputRateText.SetWindowText(sSampleRate);
				}
			}
		}

		m_CurrentDeviceText.SetWindowText(m_pMAR->GetCurrentPlaybackDevice());
	}

	for (CWnd* pWnd = GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetNextWindow()) {
		pWnd->SetFont(&m_font, FALSE);
	}

	SetDirty(false);

	return true;
}

void CMpcAudioRendererStatusWnd::OnDeactivate()
{
	m_CurrentDeviceText.SetSel(-1);
}

BEGIN_MESSAGE_MAP(CMpcAudioRendererStatusWnd, CInternalPropertyPageWnd)
END_MESSAGE_MAP()

/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2016 see Authors.txt
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
#include <math.h>
#include "../../DSUtil/SysVersion.h"
#include "../../filters/renderer/MpcAudioRenderer/MpcAudioRenderer.h"
#include "ComPropertyPage.h"
#include "MainFrm.h"
#include "PPageAudio.h"

// CPPageAudio dialog

IMPLEMENT_DYNAMIC(CPPageAudio, CPPageBase)
CPPageAudio::CPPageAudio()
	: CPPageBase(CPPageAudio::IDD, CPPageAudio::IDD)
	, m_iAudioRendererType(0)
	, m_iSecAudioRendererType(0)

	, m_fAutoloadAudio(FALSE)
	, m_fPrioritizeExternalAudio(FALSE)
{
}

CPPageAudio::~CPPageAudio()
{
}

void CPPageAudio::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_AUDRND_COMBO, m_iAudioRendererTypeCtrl);
	DDX_Control(pDX, IDC_COMBO1, m_iSecAudioRendererTypeCtrl);
	DDX_CBIndex(pDX, IDC_AUDRND_COMBO, m_iAudioRendererType);
	DDX_CBIndex(pDX, IDC_COMBO1, m_iSecAudioRendererType);
	DDX_Control(pDX, IDC_BUTTON1, m_audRendPropButton);
	DDX_Control(pDX, IDC_CHECK1, m_DualAudioOutput);

	DDX_Check(pDX, IDC_CHECK2, m_fAutoloadAudio);
	DDX_Text(pDX, IDC_EDIT4, m_sAudioPaths);
	DDX_Check(pDX, IDC_CHECK3, m_fPrioritizeExternalAudio);

	DDX_Control(pDX, IDC_CHECK7, m_chkMixer);
	DDX_Control(pDX, IDC_COMBO2, m_cmbMixerLayout);
	DDX_Control(pDX, IDC_CHECK8, m_chkBassRedirect);

	DDX_Control(pDX, IDC_SLIDER1, m_sldGain);
	DDX_Control(pDX, IDC_STATIC4, m_stcGain);

	DDX_Control(pDX, IDC_CHECK5, m_chkAutoVolumeControl);
	DDX_Control(pDX, IDC_CHECK6, m_chkNormBoostAudio);
	DDX_Control(pDX, IDC_STATIC6, m_stcNormLevel);
	DDX_Control(pDX, IDC_STATIC7, m_stcNormRealeaseTime);
	DDX_Control(pDX, IDC_SLIDER3, m_sldNormLevel);
	DDX_Control(pDX, IDC_SLIDER4, m_sldNormRealeaseTime);

	DDX_Control(pDX, IDC_CHECK4, m_chkTimeShift);
	DDX_Control(pDX, IDC_EDIT2, m_edtTimeShift);
	DDX_Control(pDX, IDC_SPIN2, m_spnTimeShift);
}

BEGIN_MESSAGE_MAP(CPPageAudio, CPPageBase)
	ON_CBN_SELCHANGE(IDC_AUDRND_COMBO, OnAudioRendererChange)
	ON_BN_CLICKED(IDC_BUTTON1, OnAudioRenderPropClick)
	ON_BN_CLICKED(IDC_CHECK1, OnDualAudioOutputCheck)
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedResetAudioPaths)
	ON_BN_CLICKED(IDC_CHECK7, OnMixerCheck)
	ON_BN_CLICKED(IDC_CHECK5, OnAutoVolumeControlCheck)
	ON_BN_CLICKED(IDC_CHECK4, OnTimeShiftCheck)
	ON_BN_CLICKED(IDC_BUTTON3, OnBnClickedSoundProcessingDefault)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

// CPPageAudio message handlers

BOOL CPPageAudio::OnInitDialog()
{
	__super::OnInitDialog();

	SetCursor(m_hWnd, IDC_AUDRND_COMBO, IDC_HAND);

	CAppSettings& s = AfxGetAppSettings();

	m_AudioRendererDisplayNames.Add(_T(""));
	m_iAudioRendererTypeCtrl.SetRedraw(FALSE);
	m_iAudioRendererTypeCtrl.AddString(_T("1. ") + ResStr(IDS_PPAGE_OUTPUT_SYS_DEF));
	m_iAudioRendererType = 0;

	m_DualAudioOutput.SetCheck(s.fDualAudioOutput);
	m_iSecAudioRendererTypeCtrl.SetRedraw(FALSE);
	m_iSecAudioRendererTypeCtrl.AddString(_T("1. ") + ResStr(IDS_PPAGE_OUTPUT_SYS_DEF));
	m_iSecAudioRendererType = 0;

	OnDualAudioOutputCheck();

	int i = 2;

	BeginEnumSysDev(CLSID_AudioRendererCategory, pMoniker) {
		LPOLESTR olestr = NULL;
		if (FAILED(pMoniker->GetDisplayName(0, 0, &olestr))) {
			continue;
		}

		CComPtr<IPropertyBag> pPB;
		if (SUCCEEDED(pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB))) {
			CComVariant var;
			if (pPB->Read(CComBSTR(L"FriendlyName"), &var, NULL) == S_OK) {
				CStringW fname = var.bstrVal;
				bool dev_enable = true;

				var.Clear();
				if (pPB->Read(CComBSTR(L"WaveOutId"), &var, NULL) == S_OK) {
					//if (var.lVal >= 0) { fname.Insert(0, L"WaveOut: "); }
					dev_enable = false; // skip WaveOut devices
				}
				else if (pPB->Read(CComBSTR(L"DSGuid"), &var, NULL) == S_OK) {
					if (CString(var.bstrVal) == "{00000000-0000-0000-0000-000000000000}") {
						dev_enable = false; // skip Default DirectSound Device
					}
				}

				if (dev_enable) {
					m_AudioRendererDisplayNames.Add(CString(olestr));

					CString str;
					str.Format(_T("%d. %s"), i++, fname);
					m_iAudioRendererTypeCtrl.AddString(str);
					m_iSecAudioRendererTypeCtrl.AddString(str);
				}
			}
			var.Clear();
		}

		CoTaskMemFree(olestr);
	}
	EndEnumSysDev;


	static CString AudioDevAddon[] = {
		AUDRNDT_NULL_COMP,
		AUDRNDT_NULL_UNCOMP,
		AUDRNDT_MPC
	};

	for (size_t idx = 0; idx < _countof(AudioDevAddon); idx++) {
		if (AudioDevAddon[idx].GetLength() > 0) {
			m_AudioRendererDisplayNames.Add(AudioDevAddon[idx]);

			CString str;
			str.Format(_T("%d. %s"), i++, AudioDevAddon[idx]);
			m_iAudioRendererTypeCtrl.AddString(str);
			m_iSecAudioRendererTypeCtrl.AddString(str);
		}
	}

	for (INT_PTR idx = 0; idx < m_AudioRendererDisplayNames.GetCount(); idx++) {
		if (s.strAudioRendererDisplayName == m_AudioRendererDisplayNames[idx]) {
			m_iAudioRendererType = idx;
		}

		if (s.strSecondAudioRendererDisplayName == m_AudioRendererDisplayNames[idx]) {
			m_iSecAudioRendererType = idx;
		}
	}

	CorrectComboListWidth(m_iAudioRendererTypeCtrl);
	m_iAudioRendererTypeCtrl.SetRedraw(TRUE);
	m_iAudioRendererTypeCtrl.Invalidate();
	m_iAudioRendererTypeCtrl.UpdateWindow();

	CorrectComboListWidth(m_iSecAudioRendererTypeCtrl);
	m_iSecAudioRendererTypeCtrl.SetRedraw(TRUE);
	m_iSecAudioRendererTypeCtrl.Invalidate();
	m_iSecAudioRendererTypeCtrl.UpdateWindow();

	m_fAutoloadAudio           = s.fAutoloadAudio;
	m_fPrioritizeExternalAudio = s.fPrioritizeExternalAudio;
	m_sAudioPaths              = s.strAudioPaths;

	m_chkMixer.SetCheck(s.bAudioMixer);
	m_cmbMixerLayout.SetItemData(m_cmbMixerLayout.AddString(ResStr(IDS_MPADEC_MONO)),   SPK_MONO);
	m_cmbMixerLayout.SetItemData(m_cmbMixerLayout.AddString(ResStr(IDS_MPADEC_STEREO)), SPK_STEREO);
	m_cmbMixerLayout.SetItemData(m_cmbMixerLayout.AddString(_T("4.0")), SPK_4_0);
	m_cmbMixerLayout.SetItemData(m_cmbMixerLayout.AddString(_T("5.0")), SPK_5_0);
	m_cmbMixerLayout.SetItemData(m_cmbMixerLayout.AddString(_T("5.1")), SPK_5_1);
	m_cmbMixerLayout.SetItemData(m_cmbMixerLayout.AddString(_T("7.1")), SPK_7_1);
	SelectByItemData(m_cmbMixerLayout, s.nAudioMixerLayout);
	m_chkBassRedirect.SetCheck(s.bAudioBassRedirect);
	m_chkBassRedirect.ShowWindow(SW_HIDE); // TODO

	m_sldGain.SetRange(-30, 100, TRUE);
	m_sldGain.SetPos(s.fAudioGain_dB > 0 ? floor(s.fAudioGain_dB * 10 + 0.5f) : ceil(s.fAudioGain_dB * 10 - 0.5f));

	m_chkAutoVolumeControl.SetCheck(s.bAudioAutoVolumeControl);
	m_chkNormBoostAudio.SetCheck(s.bAudioNormBoost);
	m_sldNormLevel.SetRange(0, 100, TRUE);
	m_sldNormLevel.SetPos(s.iAudioNormLevel);
	m_sldNormRealeaseTime.SetRange(5, 10, TRUE);
	m_sldNormRealeaseTime.SetPos(s.iAudioNormRealeaseTime);

	m_chkTimeShift.SetCheck(s.bAudioTimeShift);
	m_edtTimeShift = s.iAudioTimeShift;
	m_edtTimeShift.SetRange(APP_AUDIOTIMESHIFT_MIN, APP_AUDIOTIMESHIFT_MAX);
	m_spnTimeShift.SetRange32(APP_AUDIOTIMESHIFT_MIN, APP_AUDIOTIMESHIFT_MAX);

	UpdateGainInfo();
	UpdateNormLevelInfo();
	UpdateNormRealeaseTimeInfo();
	OnMixerCheck();
	OnAutoVolumeControlCheck();
	OnTimeShiftCheck();

	UpdateData(FALSE);

	OnAudioRendererChange();

	return TRUE;
}

BOOL CPPageAudio::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	s.strAudioRendererDisplayName       = m_AudioRendererDisplayNames[m_iAudioRendererType];
	s.strSecondAudioRendererDisplayName = m_iSecAudioRendererType == -1 ? L"" : m_AudioRendererDisplayNames[m_iSecAudioRendererType];
	s.fDualAudioOutput                  = !!m_DualAudioOutput.GetCheck();

	s.fAutoloadAudio = !!m_fAutoloadAudio;
	s.fPrioritizeExternalAudio = !!m_fPrioritizeExternalAudio;
	s.strAudioPaths = m_sAudioPaths;


	s.bAudioMixer				= !!m_chkMixer.GetCheck();
	s.nAudioMixerLayout			= GetCurItemData(m_cmbMixerLayout);
	s.bAudioBassRedirect		= !!m_chkBassRedirect.GetCheck();

	s.fAudioGain_dB				= m_sldGain.GetPos() / 10.0f;

	s.bAudioAutoVolumeControl	= !!m_chkAutoVolumeControl.GetCheck();
	s.bAudioNormBoost			= !!m_chkNormBoostAudio.GetCheck();
	s.iAudioNormLevel			= m_sldNormLevel.GetPos();
	s.iAudioNormRealeaseTime	= m_sldNormRealeaseTime.GetPos();
	s.bAudioTimeShift			= !!m_chkTimeShift.GetCheck();
	s.iAudioTimeShift			= m_edtTimeShift;

	IFilterGraph* pFG = AfxGetMainFrame()->m_pGB;
	if (pFG) {
		CComQIPtr<IAudioSwitcherFilter> m_pASF = FindFilter(__uuidof(CAudioSwitcherFilter), pFG);
		if (m_pASF) {
			m_pASF->SetChannelMixer(s.bAudioMixer, s.nAudioMixerLayout);
			m_pASF->SetAudioGain(s.fAudioGain_dB);
			m_pASF->SetAutoVolumeControl(s.bAudioAutoVolumeControl, s.bAudioNormBoost, s.iAudioNormLevel, s.iAudioNormRealeaseTime);
			m_pASF->SetAudioTimeShift(s.bAudioTimeShift ? 10000i64*s.iAudioTimeShift : 0);
		}
	}

	return __super::OnApply();
}

void CPPageAudio::ShowPPage(CUnknown* (WINAPI * CreateInstance)(LPUNKNOWN lpunk, HRESULT* phr))
{
	if (!CreateInstance) {
		return;
	}

	HRESULT hr;
	CUnknown* pObj = CreateInstance(NULL, &hr);

	if (!pObj) {
		return;
	}

	CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)pObj;

	if (SUCCEEDED(hr)) {
		if (CComQIPtr<ISpecifyPropertyPages> pSPP = pUnk) {
			CComPropertySheet ps(ResStr(IDS_PROPSHEET_PROPERTIES), this);
			ps.AddPages(pSPP);
			ps.DoModal();
		}
	}
}

void CPPageAudio::OnAudioRendererChange()
{
	UpdateData();

	BOOL flag = FALSE;
	CString str_audio = m_AudioRendererDisplayNames[m_iAudioRendererType];
	if (str_audio == AUDRNDT_MPC) {
		flag = TRUE;
	} else {
		BeginEnumSysDev(CLSID_AudioRendererCategory, pMoniker) {
			LPOLESTR olestr = NULL;
			if (FAILED(pMoniker->GetDisplayName(0, 0, &olestr))) {
				continue;
			}

			CStringW str(olestr);
			CoTaskMemFree(olestr);

			if (str == m_AudioRendererDisplayNames[m_iAudioRendererType]) {
				CComPtr<IBaseFilter> pBF;
				HRESULT hr = pMoniker->BindToObject(NULL, NULL, IID_PPV_ARGS(&pBF));
				if (SUCCEEDED(hr)) {
					if (CComQIPtr<ISpecifyPropertyPages> pSPP = pBF) {
						flag = TRUE;
						break;
					}
				}
			}
		}
		EndEnumSysDev
	}

	m_audRendPropButton.EnableWindow(flag);

	SetModified();
}

void CPPageAudio::OnAudioRenderPropClick()
{
	CString str_audio = m_AudioRendererDisplayNames[m_iAudioRendererType];

	if (str_audio == AUDRNDT_MPC) {
		ShowPPage(CreateInstance<CMpcAudioRenderer>);
	} else {
		BeginEnumSysDev(CLSID_AudioRendererCategory, pMoniker) {
			LPOLESTR olestr = NULL;
			if (FAILED(pMoniker->GetDisplayName(0, 0, &olestr))) {
				continue;
			}

			CStringW str(olestr);
			CoTaskMemFree(olestr);

			if (str == str_audio) {
				CComPtr<IBaseFilter> pBF;
				HRESULT hr = pMoniker->BindToObject(NULL, NULL, IID_PPV_ARGS(&pBF));
				if (SUCCEEDED(hr)) {
					ISpecifyPropertyPages *pProp = NULL;
					hr = pBF->QueryInterface(IID_PPV_ARGS(&pProp));
					if (SUCCEEDED(hr)) {
						// Get the filter's name and IUnknown pointer.
						FILTER_INFO FilterInfo;
						hr = pBF->QueryFilterInfo(&FilterInfo);
						if (SUCCEEDED(hr)) {
							IUnknown *pFilterUnk;
							hr = pBF->QueryInterface(IID_PPV_ARGS(&pFilterUnk));
							if (SUCCEEDED(hr)) {

								// Show the page.
								CAUUID caGUID;
								pProp->GetPages(&caGUID);
								pProp->Release();

								OleCreatePropertyFrame(
									this->m_hWnd,			// Parent window
									0, 0,					// Reserved
									FilterInfo.achName,		// Caption for the dialog box
									1,						// Number of objects (just the filter)
									&pFilterUnk,			// Array of object pointers.
									caGUID.cElems,			// Number of property pages
									caGUID.pElems,			// Array of property page CLSIDs
									0,						// Locale identifier
									0, NULL					// Reserved
								);

								// Clean up.
								CoTaskMemFree(caGUID.pElems);
								pFilterUnk->Release();
							}
							if (FilterInfo.pGraph) {
								FilterInfo.pGraph->Release();
							}
						}
					}
				}
				break;
			}
		}
		EndEnumSysDev
	}
}

void CPPageAudio::OnDualAudioOutputCheck()
{
	m_iSecAudioRendererTypeCtrl.EnableWindow(!!m_DualAudioOutput.GetCheck());

	SetModified();
}

void CPPageAudio::OnBnClickedResetAudioPaths()
{
	m_sAudioPaths = DEFAULT_AUDIO_PATHS;

	UpdateData(FALSE);

	SetModified();
}

void CPPageAudio::OnMixerCheck()
{
	if (m_chkMixer.GetCheck()) {
		m_cmbMixerLayout.EnableWindow();
	} else {
		m_cmbMixerLayout.EnableWindow(FALSE);
	}

	SetModified();
}

void CPPageAudio::OnAutoVolumeControlCheck()
{
	if (m_chkAutoVolumeControl.GetCheck()) {
		m_stcGain.EnableWindow(FALSE);
		m_sldGain.EnableWindow(FALSE);

		m_chkNormBoostAudio.EnableWindow();
		m_stcNormLevel.EnableWindow();
		m_stcNormRealeaseTime.EnableWindow();
		m_sldNormLevel.EnableWindow();
		m_sldNormRealeaseTime.EnableWindow();
	} else {
		m_stcGain.EnableWindow();
		m_sldGain.EnableWindow();

		m_chkNormBoostAudio.EnableWindow(FALSE);
		m_stcNormLevel.EnableWindow(FALSE);
		m_stcNormRealeaseTime.EnableWindow(FALSE);
		m_sldNormLevel.EnableWindow(FALSE);
		m_sldNormRealeaseTime.EnableWindow(FALSE);
	}

	SetModified();
}

void CPPageAudio::OnTimeShiftCheck()
{
	if (m_chkTimeShift.GetCheck()) {
		m_edtTimeShift.EnableWindow();
		m_spnTimeShift.EnableWindow();
	} else {
		m_edtTimeShift.EnableWindow(FALSE);
		m_spnTimeShift.EnableWindow(FALSE);
	}

	SetModified();
}

void CPPageAudio::OnBnClickedSoundProcessingDefault()
{
	m_chkMixer.SetCheck(BST_UNCHECKED);
	SelectByItemData(m_cmbMixerLayout, SPK_STEREO);
	m_chkBassRedirect.SetCheck(BST_UNCHECKED);

	m_sldGain.SetPos(0);
	m_chkAutoVolumeControl.SetCheck(BST_UNCHECKED);
	m_chkNormBoostAudio.SetCheck(BST_CHECKED);
	m_sldNormLevel.SetPos(75);
	m_sldNormRealeaseTime.SetPos(8);
	m_chkTimeShift.SetCheck(BST_UNCHECKED);

	UpdateGainInfo();
	UpdateNormLevelInfo();
	UpdateNormRealeaseTimeInfo();
	OnMixerCheck();
	OnAutoVolumeControlCheck();
	OnTimeShiftCheck();
}

void CPPageAudio::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (*pScrollBar == m_sldGain) {
		UpdateGainInfo();
	} else if (*pScrollBar == m_sldNormLevel) {
		UpdateNormLevelInfo();
	} else if (*pScrollBar == m_sldNormRealeaseTime) {
		UpdateNormRealeaseTimeInfo();
	}

	SetModified();

	__super::OnHScroll(nSBCode, nPos, pScrollBar);
}

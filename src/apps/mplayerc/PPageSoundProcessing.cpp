/*
 * (C) 2017-2018 see Authors.txt
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
#include "../../filters/transform/MpaDecFilter/MpaDecFilter.h"
#include "../../filters/switcher/AudioSwitcher/AudioSwitcher.h"
#include "MainFrm.h"
#include "PPageSoundProcessing.h"

// CPPageSoundProcessing dialog

IMPLEMENT_DYNAMIC(CPPageSoundProcessing, CPPageBase)
CPPageSoundProcessing::CPPageSoundProcessing()
	: CPPageBase(CPPageSoundProcessing::IDD, CPPageSoundProcessing::IDD)
{
}

CPPageSoundProcessing::~CPPageSoundProcessing()
{
}

void CPPageSoundProcessing::UpdateCenterInfo()
{
	CString str;
	str.Format(ResStr(IDS_CENTER_LEVEL)+L"*", m_sldCenter.GetPos() / 10.0f);
	m_stcCenter.SetWindowTextW(str);
};

void CPPageSoundProcessing::UpdateSurroundInfo()
{
	CString str;
	str.Format(L"Surround level (%+.1f dB)*", m_sldSurround.GetPos() / 10.0f);
	m_stcSurround.SetWindowTextW(str);
};

void CPPageSoundProcessing::UpdateGainInfo()
{
	CString str;
	str.Format(ResStr(IDS_AUDIO_GAIN), m_sldGain.GetPos() / 10.0f);
	m_stcGain.SetWindowTextW(str);
};

void CPPageSoundProcessing::UpdateNormLevelInfo()
{
	CString str;
	str.Format(ResStr(IDS_AUDIO_LEVEL), m_sldNormLevel.GetPos());
	m_stcNormLevel.SetWindowTextW(str);
};

void CPPageSoundProcessing::UpdateNormRealeaseTimeInfo()
{
	CString str;
	str.Format(ResStr(IDS_AUDIO_RELEASETIME), m_sldNormRealeaseTime.GetPos());
	m_stcNormRealeaseTime.SetWindowTextW(str);
};


int CPPageSoundProcessing::GetSampleFormats()
{
	int sampleformats = 0;
	if (m_chkInt16.GetCheck() == BST_CHECKED) { sampleformats += SFMT_INT16; }
	if (m_chkInt24.GetCheck() == BST_CHECKED) { sampleformats += SFMT_INT24; }
	if (m_chkInt32.GetCheck() == BST_CHECKED) { sampleformats += SFMT_INT32; }
	if (m_chkFloat.GetCheck() == BST_CHECKED) { sampleformats += SFMT_FLOAT; }

	return sampleformats;
}

void CPPageSoundProcessing::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_CHECK7, m_chkMixer);
	DDX_Control(pDX, IDC_COMBO2, m_cmbMixerLayout);
	DDX_Control(pDX, IDC_CHECK8, m_chkStereoFromDecoder);
	DDX_Control(pDX, IDC_BASSREDIRECT_CHECK, m_chkBassRedirect);
	DDX_Control(pDX, IDC_SLIDER2, m_sldCenter);
	DDX_Control(pDX, IDC_STATIC5, m_stcCenter);
	DDX_Control(pDX, IDC_SLIDER5, m_sldSurround);
	DDX_Control(pDX, IDC_STATIC8, m_stcSurround);

	DDX_Control(pDX, IDC_SLIDER1, m_sldGain);
	DDX_Control(pDX, IDC_STATIC4, m_stcGain);

	DDX_Control(pDX, IDC_CHECK5, m_chkAutoVolumeControl);
	DDX_Control(pDX, IDC_CHECK6, m_chkNormBoostAudio);
	DDX_Control(pDX, IDC_STATIC6, m_stcNormLevel);
	DDX_Control(pDX, IDC_STATIC7, m_stcNormRealeaseTime);
	DDX_Control(pDX, IDC_SLIDER3, m_sldNormLevel);
	DDX_Control(pDX, IDC_SLIDER4, m_sldNormRealeaseTime);

	DDX_Control(pDX, IDC_CHECK9, m_chkInt16);
	DDX_Control(pDX, IDC_CHECK10, m_chkInt24);
	DDX_Control(pDX, IDC_CHECK11, m_chkInt32);
	DDX_Control(pDX, IDC_CHECK12, m_chkFloat);

	DDX_Control(pDX, IDC_CHECK4, m_chkTimeShift);
	DDX_Control(pDX, IDC_EDIT2, m_edtTimeShift);
	DDX_Control(pDX, IDC_SPIN2, m_spnTimeShift);
}

BEGIN_MESSAGE_MAP(CPPageSoundProcessing, CPPageBase)
	ON_BN_CLICKED(IDC_CHECK7, OnMixerCheck)
	ON_CBN_SELCHANGE(IDC_COMBO2, OnMixerLayoutChange)
	ON_BN_CLICKED(IDC_CHECK5, OnAutoVolumeControlCheck)
	ON_BN_CLICKED(IDC_CHECK9, OnInt16Check)
	ON_BN_CLICKED(IDC_CHECK10, OnInt24Check)
	ON_BN_CLICKED(IDC_CHECK11, OnInt32Check)
	ON_BN_CLICKED(IDC_CHECK12, OnFloatCheck)
	ON_BN_CLICKED(IDC_CHECK4, OnTimeShiftCheck)
	ON_BN_CLICKED(IDC_BUTTON3, OnBnClickedSoundProcessingDefault)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

// CPPageSoundProcessing message handlers

BOOL CPPageSoundProcessing::OnInitDialog()
{
	__super::OnInitDialog();

	CAppSettings& s = AfxGetAppSettings();

	m_chkMixer.SetCheck(s.bAudioMixer);
	AddStringData(m_cmbMixerLayout, ResStr(IDS_MPADEC_MONO),   SPK_MONO);
	AddStringData(m_cmbMixerLayout, ResStr(IDS_MPADEC_STEREO), SPK_STEREO);
	AddStringData(m_cmbMixerLayout, L"4.0", SPK_4_0);
	AddStringData(m_cmbMixerLayout, L"5.0", SPK_5_0);
	AddStringData(m_cmbMixerLayout, L"5.1", SPK_5_1);
	AddStringData(m_cmbMixerLayout, L"7.1", SPK_7_1);
	SelectByItemData(m_cmbMixerLayout, s.nAudioMixerLayout);
	m_chkStereoFromDecoder.SetCheck(s.bAudioStereoFromDecoder);
	m_chkBassRedirect.SetCheck(s.bAudioBassRedirect);
	m_sldCenter.SetRange(-100, 100, TRUE);
	m_sldCenter.SetPos((int)std::round(s.fAudioCenter_dB * 10));
	m_sldSurround.SetRange(-100, 100, TRUE);
	m_sldSurround.SetPos((int)std::round(s.fAudioSurround_dB * 10));

	m_sldGain.SetRange(-30, 100, TRUE);
	m_sldGain.SetPos((int)std::round(s.dAudioGain_dB * 10));

	m_chkAutoVolumeControl.SetCheck(s.bAudioAutoVolumeControl);
	m_chkNormBoostAudio.SetCheck(s.bAudioNormBoost);
	m_sldNormLevel.SetRange(0, 100, TRUE);
	m_sldNormLevel.SetPos(s.iAudioNormLevel);
	m_sldNormRealeaseTime.SetRange(5, 10, TRUE);
	m_sldNormRealeaseTime.SetPos(s.iAudioNormRealeaseTime);

	m_chkInt16.SetCheck((s.iAudioSampleFormats & SFMT_INT16) ? BST_CHECKED : BST_UNCHECKED);
	m_chkInt24.SetCheck((s.iAudioSampleFormats & SFMT_INT24) ? BST_CHECKED : BST_UNCHECKED);
	m_chkInt32.SetCheck((s.iAudioSampleFormats & SFMT_INT32) ? BST_CHECKED : BST_UNCHECKED);
	m_chkFloat.SetCheck((s.iAudioSampleFormats & SFMT_FLOAT) ? BST_CHECKED : BST_UNCHECKED);

	m_chkTimeShift.SetCheck(s.bAudioTimeShift);
	m_edtTimeShift = s.iAudioTimeShift;
	m_edtTimeShift.SetRange(APP_AUDIOTIMESHIFT_MIN, APP_AUDIOTIMESHIFT_MAX);
	m_spnTimeShift.SetRange32(APP_AUDIOTIMESHIFT_MIN, APP_AUDIOTIMESHIFT_MAX);

	UpdateCenterInfo();
	UpdateSurroundInfo();
	UpdateGainInfo();
	UpdateNormLevelInfo();
	UpdateNormRealeaseTimeInfo();
	OnMixerCheck();
	OnAutoVolumeControlCheck();
	OnTimeShiftCheck();

	UpdateData(FALSE);

	CreateToolTip();

	return TRUE;
}

BOOL CPPageSoundProcessing::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	s.bAudioMixer				= !!m_chkMixer.GetCheck();
	s.nAudioMixerLayout			= GetCurItemData(m_cmbMixerLayout);
	s.bAudioStereoFromDecoder	= !!m_chkStereoFromDecoder.GetCheck();
	s.bAudioBassRedirect		= !!m_chkBassRedirect.GetCheck();
	s.fAudioCenter_dB			= m_sldCenter.GetPos() / 10.0f;
	s.fAudioSurround_dB			= m_sldSurround.GetPos() / 10.0f;

	s.dAudioGain_dB				= m_sldGain.GetPos() / 10.0;

	s.bAudioAutoVolumeControl	= !!m_chkAutoVolumeControl.GetCheck();
	s.bAudioNormBoost			= !!m_chkNormBoostAudio.GetCheck();
	s.iAudioNormLevel			= m_sldNormLevel.GetPos();
	s.iAudioNormRealeaseTime	= m_sldNormRealeaseTime.GetPos();

	s.iAudioSampleFormats		= GetSampleFormats();

	s.bAudioTimeShift			= !!m_chkTimeShift.GetCheck();
	s.iAudioTimeShift			= m_edtTimeShift;

	if (IFilterGraph* pFG = AfxGetMainFrame()->m_pGB) {
		BeginEnumFilters(pFG, pEF, pBF) {
			CLSID clsid;
			if (SUCCEEDED(pBF->GetClassID(&clsid)) && __uuidof(CMpaDecFilter) == clsid) {
				if (CComQIPtr<IExFilterConfig> pEFC = pBF) {
					pEFC->SetBool("stereodownmix", s.bAudioMixer && s.nAudioMixerLayout == SPK_STEREO && s.bAudioStereoFromDecoder);
				}
			}
		}
		EndEnumFilters

		if (CComQIPtr<IAudioSwitcherFilter> pASF = FindFilter(__uuidof(CAudioSwitcherFilter), pFG)) {
			pASF->SetChannelMixer(s.bAudioMixer, s.nAudioMixerLayout);
			pASF->SetBassRedirect(s.bAudioBassRedirect);
			pASF->SetLevels(s.fAudioCenter_dB, s.fAudioSurround_dB);
			pASF->SetAudioGain(s.dAudioGain_dB);
			pASF->SetAutoVolumeControl(s.bAudioAutoVolumeControl, s.bAudioNormBoost, s.iAudioNormLevel, s.iAudioNormRealeaseTime);
			pASF->SetOutputFormats(s.iAudioSampleFormats);
			pASF->SetAudioTimeShift(s.bAudioTimeShift ? 10000i64*s.iAudioTimeShift : 0);
		}
	}

	return __super::OnApply();
}

void CPPageSoundProcessing::OnMixerCheck()
{
	m_cmbMixerLayout.EnableWindow(m_chkMixer.GetCheck());
	m_chkStereoFromDecoder.EnableWindow(m_chkMixer.GetCheck() && GetCurItemData(m_cmbMixerLayout) == SPK_STEREO);

	SetModified();
}

void CPPageSoundProcessing::OnMixerLayoutChange()
{
	m_chkStereoFromDecoder.EnableWindow(m_chkMixer.GetCheck() && GetCurItemData(m_cmbMixerLayout) == SPK_STEREO);
}

void CPPageSoundProcessing::OnAutoVolumeControlCheck()
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

void CPPageSoundProcessing::OnInt16Check()
{
	if (GetSampleFormats() == 0) {
		m_chkInt16.SetCheck(BST_CHECKED);
	} else {
		SetModified();
	}
}

void CPPageSoundProcessing::OnInt24Check()
{
	if (GetSampleFormats() == 0) {
		m_chkInt24.SetCheck(BST_CHECKED);
	} else {
		SetModified();
	}
}

void CPPageSoundProcessing::OnInt32Check()
{
	if (GetSampleFormats() == 0) {
		m_chkInt32.SetCheck(BST_CHECKED);
	} else {
		SetModified();
	}
}

void CPPageSoundProcessing::OnFloatCheck()
{
	if (GetSampleFormats() == 0) {
		m_chkFloat.SetCheck(BST_CHECKED);
	} else {
		SetModified();
	}
}

void CPPageSoundProcessing::OnTimeShiftCheck()
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

void CPPageSoundProcessing::OnBnClickedSoundProcessingDefault()
{
	m_chkMixer.SetCheck(BST_UNCHECKED);
	SelectByItemData(m_cmbMixerLayout, SPK_STEREO);
	m_chkBassRedirect.SetCheck(BST_UNCHECKED);
	m_sldCenter.SetPos(0);
	m_sldSurround.SetPos(0);

	m_sldGain.SetPos(0);
	m_chkAutoVolumeControl.SetCheck(BST_UNCHECKED);
	m_chkNormBoostAudio.SetCheck(BST_CHECKED);
	m_sldNormLevel.SetPos(75);
	m_sldNormRealeaseTime.SetPos(8);

	m_chkInt16.SetCheck(BST_CHECKED);
	m_chkInt24.SetCheck(BST_CHECKED);
	m_chkInt32.SetCheck(BST_CHECKED);
	m_chkFloat.SetCheck(BST_CHECKED);

	m_chkTimeShift.SetCheck(BST_UNCHECKED);

	UpdateCenterInfo();
	UpdateSurroundInfo();
	UpdateGainInfo();
	UpdateNormLevelInfo();
	UpdateNormRealeaseTimeInfo();
	OnMixerCheck();
	OnAutoVolumeControlCheck();
	OnTimeShiftCheck();
}

void CPPageSoundProcessing::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (*pScrollBar == m_sldCenter) {
		UpdateCenterInfo();
	}
	else if (*pScrollBar == m_sldSurround) {
		UpdateSurroundInfo();
	}
	else if (*pScrollBar == m_sldGain) {
		UpdateGainInfo();
	}
	else if (*pScrollBar == m_sldNormLevel) {
		UpdateNormLevelInfo();
	}
	else if (*pScrollBar == m_sldNormRealeaseTime) {
		UpdateNormRealeaseTimeInfo();
	}

	SetModified();

	__super::OnHScroll(nSBCode, nPos, pScrollBar);
}

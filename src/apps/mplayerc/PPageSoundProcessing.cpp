/*
 * (C) 2017 see Authors.txt
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

void CPPageSoundProcessing::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_CHECK7, m_chkMixer);
	DDX_Control(pDX, IDC_COMBO2, m_cmbMixerLayout);
	DDX_Control(pDX, IDC_CHECK8, m_chkStereoFromDecoder);
	DDX_Control(pDX, IDC_BASSREDIRECT_CHECK, m_chkBassRedirect);

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

BEGIN_MESSAGE_MAP(CPPageSoundProcessing, CPPageBase)
	ON_BN_CLICKED(IDC_CHECK7, OnMixerCheck)
	ON_CBN_SELCHANGE(IDC_COMBO2, OnMixerLayoutChange)
	ON_BN_CLICKED(IDC_CHECK5, OnAutoVolumeControlCheck)
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

	s.fAudioGain_dB				= m_sldGain.GetPos() / 10.0f;

	s.bAudioAutoVolumeControl	= !!m_chkAutoVolumeControl.GetCheck();
	s.bAudioNormBoost			= !!m_chkNormBoostAudio.GetCheck();
	s.iAudioNormLevel			= m_sldNormLevel.GetPos();
	s.iAudioNormRealeaseTime	= m_sldNormRealeaseTime.GetPos();
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

		if (CComQIPtr<IAudioSwitcherFilter> m_pASF = FindFilter(__uuidof(CAudioSwitcherFilter), pFG)) {
			m_pASF->SetChannelMixer(s.bAudioMixer, s.nAudioMixerLayout);
			m_pASF->SetBassRedirect(s.bAudioBassRedirect);
			m_pASF->SetAudioGain(s.fAudioGain_dB);
			m_pASF->SetAutoVolumeControl(s.bAudioAutoVolumeControl, s.bAudioNormBoost, s.iAudioNormLevel, s.iAudioNormRealeaseTime);
			m_pASF->SetAudioTimeShift(s.bAudioTimeShift ? 10000i64*s.iAudioTimeShift : 0);
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

void CPPageSoundProcessing::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
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

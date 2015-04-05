/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2015 see Authors.txt
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

#include "PPageBase.h"
#include "FloatEdit.h"
#include "../../filters/switcher/AudioSwitcher/AudioSwitcher.h"


// CPPageAudio dialog

class CPPageAudio : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageAudio)

private:
	CComQIPtr<IAudioSwitcherFilter> m_pASF;

	CStringArray m_AudioRendererDisplayNames;

public:
	CPPageAudio(IFilterGraph* pFG);
	virtual ~CPPageAudio();

	enum { IDD = IDD_PPAGEAUDIO };

	int			m_iAudioRendererType;
	CComboBox	m_iAudioRendererTypeCtrl;
	int			m_iSecAudioRendererType;
	CComboBox	m_iSecAudioRendererTypeCtrl;
	CButton		m_audRendPropButton;
	CButton		m_DualAudioOutput;

	BOOL		m_fAutoloadAudio;
	CString		m_sAudioPaths;
	BOOL		m_fPrioritizeExternalAudio;

	CButton		m_chkMixer;
	CComboBox	m_cmbMixerLayout;

	CStatic		m_stcGain;
	CSliderCtrl	m_sldGain;

	CButton		m_chkAutoVolumeControl;
	CButton		m_chkNormBoostAudio;
	CStatic		m_stcNormLevel;
	CStatic		m_stcNormRealeaseTime;
	CSliderCtrl	m_sldNormLevel;
	CSliderCtrl	m_sldNormRealeaseTime;

	CButton		m_chkTimeShift;
	CIntEdit	m_edtTimeShift;
	CSpinButtonCtrl m_spnTimeShift;

	void UpdateGainInfo() { CString str; str.Format(ResStr(IDS_AUDIO_GAIN), m_sldGain.GetPos() / 10.0f); m_stcGain.SetWindowText(str); };
	void UpdateNormLevelInfo() { CString str; str.Format(ResStr(IDS_AUDIO_LEVEL), m_sldNormLevel.GetPos()); m_stcNormLevel.SetWindowText(str); };
	void UpdateNormRealeaseTimeInfo() { CString str; str.Format(ResStr(IDS_AUDIO_RELEASETIME), m_sldNormRealeaseTime.GetPos()); m_stcNormRealeaseTime.SetWindowText(str); };

	CToolTipCtrl m_tooltip;

	void ShowPPage(CUnknown* (WINAPI * CreateInstance)(LPUNKNOWN lpunk, HRESULT* phr));

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnAudioRendererChange();
	afx_msg void OnAudioRenderPropClick();
	afx_msg void OnDualAudioOutputCheck();
	afx_msg void OnBnClickedResetAudioPaths();
	afx_msg void OnMixerCheck();
	afx_msg void OnAutoVolumeControlCheck();
	afx_msg void OnTimeShiftCheck();
	afx_msg void OnBnClickedSoundProcessingDefault();

	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};

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

#pragma once

#include "PPageBase.h"
#include "controls/FloatEdit.h"

// CPPageSoundProcessing dialog

class CPPageSoundProcessing : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageSoundProcessing)

	enum { IDD = IDD_PPAGESOUNDPROCESSING };

	CButton		m_chkMixer;
	CComboBox	m_cmbMixerLayout;
	CButton		m_chkStereoFromDecoder;
	CButton		m_chkBassRedirect;
	CStatic		m_stcCenter;
	CSliderCtrl	m_sldCenter;
	CStatic		m_stcSurround;
	CSliderCtrl	m_sldSurround;

	CStatic		m_stcGain;
	CSliderCtrl	m_sldGain;

	CButton		m_chkAutoVolumeControl;
	CButton		m_chkNormBoostAudio;
	CStatic		m_stcNormLevel;
	CStatic		m_stcNormRealeaseTime;
	CSliderCtrl	m_sldNormLevel;
	CSliderCtrl	m_sldNormRealeaseTime;

	CButton		m_chkInt16;
	CButton		m_chkInt24;
	CButton		m_chkInt32;
	CButton		m_chkFloat;

	CButton		m_chkTimeShift;
	CIntEdit	m_edtTimeShift;
	CSpinButtonCtrl m_spnTimeShift;

public:
	CPPageSoundProcessing();
	virtual ~CPPageSoundProcessing();

private:
	void UpdateCenterInfo();
	void UpdateSurroundInfo();
	void UpdateGainInfo();
	void UpdateNormLevelInfo();
	void UpdateNormRealeaseTimeInfo();

	int GetSampleFormats();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnMixerCheck();
	afx_msg void OnMixerLayoutChange();
	afx_msg void OnAutoVolumeControlCheck();
	afx_msg void OnInt16Check();
	afx_msg void OnInt24Check();
	afx_msg void OnInt32Check();
	afx_msg void OnFloatCheck();
	afx_msg void OnTimeShiftCheck();
	afx_msg void OnBnClickedSoundProcessingDefault();

	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnCancel();
};

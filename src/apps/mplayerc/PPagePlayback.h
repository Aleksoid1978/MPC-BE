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


// CPPagePlayback dialog

class CPPagePlayback : public CPPageBase
{
	DECLARE_DYNAMIC(CPPagePlayback)

	// private:
	int m_oldVolume; //not very nice solution

public:
	CPPagePlayback();
	virtual ~CPPagePlayback();

	int			m_nVolume;
	CSliderCtrl	m_volumectrl;
	int			m_nBalance;
	CSliderCtrl	m_balancectrl;

	int			m_iLoopForever;
	CEdit		m_loopnumctrl;
	int			m_nLoops;
	BOOL		m_fRewind;

	int			m_nVolumeStep;
	CComboBox	m_nVolumeStepCtrl;
	CComboBox	m_nSpeedStepCtrl;

	CButton		m_chkRememberZoomLevel;
	CComboBox	m_cmbZoomLevel;
	int			m_nAutoFitFactor;
	CSpinButtonCtrl m_spnAutoFitFactor;

	BOOL		m_fUseInternalSelectTrackLogic;
	CString		m_subtitlesLanguageOrder;
	CString		m_audiosLanguageOrder;

	CComboBox	m_cbAudioWindowMode;
	BOOL		m_bAddSimilarFiles;
	BOOL		m_fEnableWorkerThreadForOpening;
	BOOL		m_fReportFailedPins;

	enum { IDD = IDD_PPAGEPLAYBACK };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnBnClickedRadio12(UINT nID);
	afx_msg void OnUpdateLoopNum(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTrackOrder(CCmdUI* pCmdUI);
	afx_msg void OnAutoZoomCheck();
	afx_msg void OnAutoZoomSelChange();
	afx_msg void OnBalanceTextDblClk();
	afx_msg BOOL OnToolTipNotify(UINT id, NMHDR * pNMHDR, LRESULT * pResult);
	virtual void OnCancel();
};

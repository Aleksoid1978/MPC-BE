/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2024 see Authors.txt
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

public:
	CPPagePlayback();
	virtual ~CPPagePlayback();

	int			m_iLoopForever = 0;
	CEdit		m_loopnumctrl;
	int			m_nLoops       = 0;
	BOOL		m_fRewind      = FALSE;

	int			m_nVolumeStep  = 1;
	CComboBox	m_nVolumeStepCtrl;
	CComboBox	m_nSpeedStepCtrl;
	BOOL		m_bSpeedNotReset = FALSE;

	BOOL		m_fUseInternalSelectTrackLogic = TRUE;
	CString		m_subtitlesLanguageOrder;
	CString		m_audiosLanguageOrder;

	BOOL		m_bRememberSelectedTracks = FALSE;

	CComboBox	m_cbAudioWindowMode;
	CComboBox	m_cbAddSimilarFiles;
	BOOL		m_fEnableWorkerThreadForOpening = FALSE;
	BOOL		m_fReportFailedPins = FALSE;

	BOOL		m_bFastSeek = FALSE;
	BOOL		m_bPauseMinimizedVideo = FALSE;

	enum { IDD = IDD_PPAGEPLAYBACK };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedRadio12(UINT nID);
	afx_msg void OnUpdateLoopNum(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTrackOrder(CCmdUI* pCmdUI);
};

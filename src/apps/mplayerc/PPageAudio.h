/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2025 see Authors.txt
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

// CPPageAudio dialog

class CPPageAudio : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageAudio)

private:
	struct audioDeviceInfo_t {
		CStringW friendlyName;
		CLSID clsid = GUID_NULL;
		GUID dsGuid = GUID_NULL;
		CStringW displayName;
	};

	std::vector<audioDeviceInfo_t> m_audioDevices;
	int m_oldVolume = 0; //not very nice solution

public:
	CPPageAudio();
	virtual ~CPPageAudio();

	enum { IDD = IDD_PPAGEAUDIO };

	int			m_iAudioRenderer = 0;
	int			m_iSecAudioRenderer = 1;

	CComboBox	m_iAudioRendererCtrl;
	CComboBox	m_iSecAudioRendererCtrl;
	CButton		m_audRendPropButton;
	CButton		m_DualAudioOutput;

	int			m_nVolume = 0;
	CSliderCtrl	m_volumectrl;
	int			m_nBalance = 0;
	CSliderCtrl	m_balancectrl;

	BOOL		m_fAutoloadAudio = FALSE;
	CString		m_sAudioPaths;
	BOOL		m_fPrioritizeExternalAudio = FALSE;

	void ShowPPage(CUnknown* (WINAPI * CreateInstance)(LPUNKNOWN lpunk, HRESULT* phr));

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnAudioRendererChange();
	afx_msg void OnAudioRenderPropClick();
	afx_msg void OnDualAudioOutputCheck();
	afx_msg void OnBalanceTextDblClk();
	afx_msg void OnBnClickedResetAudioPaths();
	virtual void OnCancel();
	afx_msg BOOL OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
};

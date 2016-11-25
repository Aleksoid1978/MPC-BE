/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2014 see Authors.txt
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

#include <afxwin.h>
#include "PPageBase.h"
#include "afxcmn.h"
#include "FloatEdit.h"

enum {
	SOURCE,
	VIDEO,
	AUDIO
};

struct filter_t {
	LPCTSTR label;
	int type;
	int flag;
	UINT nHintID;
};

class CPPageInternalFiltersListBox : public CCheckListBox
{
	DECLARE_DYNAMIC(CPPageInternalFiltersListBox)

public:
	CPPageInternalFiltersListBox(int n);

protected:
	virtual void PreSubclassWindow();
	virtual INT_PTR OnToolHitTest(CPoint point, TOOLINFO* pTI) const;

	DECLARE_MESSAGE_MAP()
	afx_msg BOOL OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult);

	CFont m_bold;
	int m_n;
	unsigned int m_nbFiltersPerType[FILTER_TYPE_NB];
	unsigned int m_nbChecked[FILTER_TYPE_NB];

public:
	virtual int AddFilter(filter_t* filter, bool checked);
	virtual void UpdateCheckState();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
};

// CPPageInternalFilters dialog

class CPPageInternalFilters : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageInternalFilters)

public:
	CPPageInternalFilters();
	virtual ~CPPageInternalFilters();

	enum { IDD = IDD_PPAGEINTERNALFILTERS };
	CPPageInternalFiltersListBox m_listSrc;
	CPPageInternalFiltersListBox m_listVideo;
	CPPageInternalFiltersListBox m_listAudio;

	CButton m_btnAviCfg;
	CButton m_btnMpegCfg;
	CButton m_btnMatroskaCfg;
	CButton m_btnCDDACfg;
	CButton m_btnVTSCfg;
	CButton m_btnVideoCfg;
	CButton m_btnMPEG2Cfg;
	CButton m_btnAudioCfg;

	CIntEdit m_edtBufferDuration;
	CSpinButtonCtrl m_spnBufferDuration;

	CTabCtrl m_Tab;

	void ShowPPage(CUnknown* (WINAPI * CreateInstance)(LPUNKNOWN lpunk, HRESULT* phr));

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnSelChange();
	afx_msg void OnCheckBoxChange();
	afx_msg void OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnAviSplitterConfig();
	afx_msg void OnMpegSplitterConfig();
	afx_msg void OnMatroskaSplitterConfig();
	afx_msg void OnCDDAReaderConfig();
	afx_msg void OnVTSReaderConfig();
	afx_msg void OnVideoDecConfig();
	afx_msg void OnMPEG2DecConfig();
	afx_msg void OnAudioDecConfig();
};

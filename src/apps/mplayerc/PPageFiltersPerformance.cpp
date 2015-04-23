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

#include "stdafx.h"
#include "PPageFiltersPerformance.h"

// CPPageFiltersPerformance dialog

IMPLEMENT_DYNAMIC(CPPageFiltersPerformance, CPPageBase)
CPPageFiltersPerformance::CPPageFiltersPerformance()
	: CPPageBase(CPPageFiltersPerformance::IDD, CPPageFiltersPerformance::IDD)
	, m_nMinQueueSize(0)
	, m_nMaxQueueSize(0)
	, m_nCachSize(0)
	, m_nMinQueuePackets(0)
	, m_nMaxQueuePackets(0)
{
	MEMORYSTATUSEX msEx;
	msEx.dwLength = sizeof(msEx);
	::GlobalMemoryStatusEx(&msEx);
	m_halfMemMB = msEx.ullTotalPhys/0x200000;
}

CPPageFiltersPerformance::~CPPageFiltersPerformance()
{
}

void CPPageFiltersPerformance::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PERFOMANCE_DEFAULT, m_DefaultCtrl);
	DDX_Text(pDX, IDC_MINQUEUE_PACKETS, m_nMinQueuePackets);
	DDX_Text(pDX, IDC_MAXQUEUE_PACKETS, m_nMaxQueuePackets);
	DDX_Control(pDX, IDC_SPIN2, m_nMinQueueSizeCtrl);
	DDX_Control(pDX, IDC_SPIN3, m_nMaxQueueSizeCtrl);
	DDX_Control(pDX, IDC_SPIN4, m_nCachSizeCtrl);
	DDX_Text(pDX, IDC_MINQUEUE_SIZE, m_nMinQueueSize);
	DDX_Text(pDX, IDC_MAXQUEUE_SIZE, m_nMaxQueueSize);
	DDX_Text(pDX, IDC_CACH_SIZE, m_nCachSize);
}

BEGIN_MESSAGE_MAP(CPPageFiltersPerformance, CPPageBase)
	ON_BN_CLICKED(IDC_PERFOMANCE_DEFAULT, &CPPageFiltersPerformance::OnBnClickedCheck1)
END_MESSAGE_MAP()

// CPPageFiltersPerformance message handlers

BOOL CPPageFiltersPerformance::OnInitDialog()
{
	__super::OnInitDialog();

	SetHandCursor(m_hWnd, IDC_PERFOMANCE_DEFAULT);

	AppSettings& s = AfxGetAppSettings();

	m_nMinQueueSizeCtrl.SetRange(64, KILOBYTE);
	m_nMaxQueueSizeCtrl.SetRange(10, min(512, m_halfMemMB));
	m_nCachSizeCtrl.SetRange(16, KILOBYTE);

	m_nMinQueueSize	= s.PerfomanceSettings.nMinQueueSize;
	m_nMaxQueueSize	= s.PerfomanceSettings.nMaxQueueSize;
	m_nCachSize		= s.PerfomanceSettings.nCacheSize;

	m_nMinQueuePackets = s.PerfomanceSettings.nMinQueuePackets;
	m_nMaxQueuePackets = s.PerfomanceSettings.nMaxQueuePackets;

	m_DefaultCtrl.SetCheck(s.PerfomanceSettings.bDefault);
	OnBnClickedCheck1();

	CorrectCWndWidth(GetDlgItem(IDC_PERFOMANCE_DEFAULT));

	UpdateData(FALSE);

	CreateToolTip();

	return TRUE;
}

BOOL CPPageFiltersPerformance::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();

	s.PerfomanceSettings.nMinQueueSize	= max(64, min(KILOBYTE, m_nMinQueueSize));
	s.PerfomanceSettings.nMaxQueueSize	= max(10, min(min(512, m_halfMemMB), m_nMaxQueueSize));
	s.PerfomanceSettings.nCacheSize		= max(16, min(KILOBYTE, m_nCachSize));

	s.PerfomanceSettings.nMinQueuePackets = max(10, min(MAXQUEUEPACKETS, m_nMinQueuePackets));
	s.PerfomanceSettings.nMaxQueuePackets = max(s.PerfomanceSettings.nMinQueuePackets*2, min(MAXQUEUEPACKETS*10, m_nMaxQueuePackets));

	s.PerfomanceSettings.UpdateStatus();

	return __super::OnApply();
}

void CPPageFiltersPerformance::OnBnClickedCheck1()
{
	if (m_DefaultCtrl.GetCheck()) {
		AppSettings& s = AfxGetAppSettings();

		s.PerfomanceSettings.SetDefault();

		m_nMinQueueSize	= s.PerfomanceSettings.nMinQueueSize;
		m_nMaxQueueSize	= s.PerfomanceSettings.nMaxQueueSize;
		m_nCachSize		= s.PerfomanceSettings.nCacheSize;

		m_nMinQueuePackets = s.PerfomanceSettings.nMinQueuePackets;
		m_nMaxQueuePackets = s.PerfomanceSettings.nMaxQueuePackets;

		UpdateData(FALSE);
	}

	for (CWnd *pChild = GetWindow(GW_CHILD); pChild != NULL; pChild = pChild->GetWindow(GW_HWNDNEXT)) {
		if (pChild != GetDlgItem(IDC_PERFOMANCE_DEFAULT) && pChild != GetDlgItem(IDC_STATIC)) {
			pChild->EnableWindow(!m_DefaultCtrl.GetCheck());
		}
	}

	SetModified();
}

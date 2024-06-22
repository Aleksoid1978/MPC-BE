/*
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

#include "stdafx.h"
#include <clsids.h>
#include "MainFrm.h"
#include "DSUtil/SysVersion.h"
#include "PPageSync.h"


IMPLEMENT_DYNAMIC(CPPageSync, CPPageBase)

CPPageSync::CPPageSync()
	: CPPageBase(CPPageSync::IDD, CPPageSync::IDD)
{
}

CPPageSync::~CPPageSync()
{
}

void CPPageSync::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_VSYNC, m_chkVSync);
	DDX_Control(pDX, IDC_VSYNC_INTERNAL, m_chkVSyncInternal);
	DDX_Control(pDX, IDC_CHECK8, m_chkEnableFrameTimeCorrection);
	DDX_Control(pDX, IDC_CHECK5, m_chkFlushGPUBeforeVSync);
	DDX_Control(pDX, IDC_CHECK6, m_chkFlushGPUAfterPresent);
	DDX_Control(pDX, IDC_CHECK7, m_chkFlushGPUWait);

	DDX_Radio(pDX, IDC_RADIO1, m_iSyncMode);
	DDX_Control(pDX, IDC_CYCLEDELTA, m_edtCycleDelta);
	DDX_Control(pDX, IDC_TARGETSYNCOFFSET, m_edtTargetSyncOffset);
	DDX_Control(pDX, IDC_CONTROLLIMIT, m_edtControlLimit);
}

BOOL CPPageSync::OnInitDialog()
{
	__super::OnInitDialog();

	InitDialogPrivate();

	CreateToolTip();

	return TRUE;
}

BOOL CPPageSync::OnSetActive()
{
	InitDialogPrivate();

	return __super::OnSetActive();
}

void CPPageSync::InitDialogPrivate()
{
	CAppSettings& s = AfxGetAppSettings();
	CRenderersSettings& rs = s.m_VRSettings;

	CMainFrame* pFrame = (CMainFrame *)(AfxGetApp()->m_pMainWnd);

	m_chkVSync.SetCheck(rs.ExtraSets.bVSync);
	m_chkVSyncInternal.SetCheck(rs.ExtraSets.bVSyncInternal);
	m_chkEnableFrameTimeCorrection.SetCheck(rs.ExtraSets.bEVRFrameTimeCorrection);
	m_chkFlushGPUBeforeVSync.SetCheck(rs.ExtraSets.bFlushGPUBeforeVSync);
	m_chkFlushGPUAfterPresent.SetCheck(rs.ExtraSets.bFlushGPUAfterPresent);
	m_chkFlushGPUWait.SetCheck(rs.ExtraSets.bFlushGPUWait);

	m_iSyncMode = (rs.ExtraSets.iSynchronizeMode == SYNCHRONIZE_VIDEO) ? 0 : 1;
	m_edtCycleDelta       = rs.ExtraSets.dCycleDelta;
	m_edtTargetSyncOffset = rs.ExtraSets.dTargetSyncOffset;
	m_edtControlLimit     = rs.ExtraSets.dControlLimit;

	if (pFrame->m_clsidCAP != CLSID_SyncAllocatorPresenter) {
		GetDlgItem(IDC_RADIO1)->EnableWindow(TRUE);
		GetDlgItem(IDC_RADIO2)->EnableWindow(TRUE);
		GetDlgItem(IDC_TARGETSYNCOFFSET)->EnableWindow(TRUE);
		GetDlgItem(IDC_CONTROLLIMIT)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC2)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC3)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC4)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC9)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC10)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC11)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC12)->EnableWindow(TRUE);

		OnSyncModeClicked(m_iSyncMode == 0 ? IDC_RADIO1 : IDC_RADIO2);
	} else {
		GetDlgItem(IDC_RADIO1)->EnableWindow(FALSE);
		GetDlgItem(IDC_RADIO2)->EnableWindow(FALSE);
		GetDlgItem(IDC_TARGETSYNCOFFSET)->EnableWindow(FALSE);
		GetDlgItem(IDC_CYCLEDELTA)->EnableWindow(FALSE);
		GetDlgItem(IDC_CONTROLLIMIT)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC2)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC3)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC4)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC5)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC9)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC10)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC11)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC12)->EnableWindow(FALSE);
	}

	UpdateData(FALSE);
	SetModified(FALSE);
}

BOOL CPPageSync::OnApply()
{
	UpdateData();
	CRenderersSettings& rs = GetRenderersSettings();

	rs.ExtraSets.bVSync						= !!m_chkVSync.GetCheck();
	rs.ExtraSets.bVSyncInternal				= !!m_chkVSyncInternal.GetCheck();
	rs.ExtraSets.bEVRFrameTimeCorrection	= !!m_chkEnableFrameTimeCorrection.GetCheck();
	rs.ExtraSets.bFlushGPUBeforeVSync		= !!m_chkFlushGPUBeforeVSync.GetCheck();
	rs.ExtraSets.bFlushGPUAfterPresent		= !!m_chkFlushGPUAfterPresent.GetCheck();
	rs.ExtraSets.bFlushGPUWait				= !!m_chkFlushGPUWait.GetCheck();

	rs.ExtraSets.iSynchronizeMode	= (m_iSyncMode == 0) ? SYNCHRONIZE_VIDEO : SYNCHRONIZE_NEAREST;
	rs.ExtraSets.dCycleDelta		= m_edtCycleDelta;
	rs.ExtraSets.dTargetSyncOffset	= m_edtTargetSyncOffset;
	rs.ExtraSets.dControlLimit		= m_edtControlLimit;

	AfxGetMainFrame()->ApplyExraRendererSettings();

	return __super::OnApply();
}

BEGIN_MESSAGE_MAP(CPPageSync, CPPageBase)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_RADIO1, IDC_RADIO2, OnSyncModeClicked)
END_MESSAGE_MAP()

void CPPageSync::OnSyncModeClicked(UINT nID)
{
	if (nID != m_iSyncMode + IDC_RADIO1) {
		UpdateData();
		SetModified();
	}

	if (nID == IDC_RADIO1) {
		GetDlgItem(IDC_STATIC5)->EnableWindow(TRUE);
		GetDlgItem(IDC_CYCLEDELTA)->EnableWindow(TRUE);
	} else {
		GetDlgItem(IDC_STATIC5)->EnableWindow(FALSE);
		GetDlgItem(IDC_CYCLEDELTA)->EnableWindow(FALSE);
	}
}

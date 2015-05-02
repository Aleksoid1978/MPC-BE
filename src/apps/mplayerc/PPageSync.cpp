/*
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

#include "stdafx.h"
#include "MainFrm.h"
#include "../../DSUtil/SysVersion.h"
#include "PPageSync.h"


IMPLEMENT_DYNAMIC(CPPageSync, CPPageBase)

CPPageSync::CPPageSync()
	: CPPageBase(CPPageSync::IDD, CPPageSync::IDD)
	, m_bSynchronizeVideo(0)
	, m_bSynchronizeDisplay(0)
	, m_bSynchronizeNearest(0)
	, m_iLineDelta(0)
	, m_iColumnDelta(0)
{
}

CPPageSync::~CPPageSync()
{
}

void CPPageSync::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_CHECK1, m_chkVMR9VSync);
	DDX_Control(pDX, IDC_CHECK2, m_chkVMR9VSyncAccurate);
	DDX_Control(pDX, IDC_CHECK3, m_chkVMR9AlterativeVSync);
	DDX_Control(pDX, IDC_EDIT1, m_edtVMR9VSyncOffset);
	DDX_Control(pDX, IDC_SPIN1, m_spnVMR9VSyncOffset);
	DDX_Control(pDX, IDC_CHECK4, m_chkDisableAero);
	DDX_Control(pDX, IDC_CHECK8, m_chkEnableFrameTimeCorrection);
	DDX_Control(pDX, IDC_CHECK5, m_chkVMRFlushGPUBeforeVSync);
	DDX_Control(pDX, IDC_CHECK6, m_chkVMRFlushGPUAfterPresent);
	DDX_Control(pDX, IDC_CHECK7, m_chkVMRFlushGPUWait);

	DDX_Check(pDX, IDC_SYNCVIDEO, m_bSynchronizeVideo);
	DDX_Check(pDX, IDC_SYNCDISPLAY, m_bSynchronizeDisplay);
	DDX_Check(pDX, IDC_SYNCNEAREST, m_bSynchronizeNearest);
	DDX_Control(pDX, IDC_CYCLEDELTA, m_edtCycleDelta);
	DDX_Text(pDX, IDC_LINEDELTA, m_iLineDelta);
	DDX_Text(pDX, IDC_COLUMNDELTA, m_iColumnDelta);
	DDX_Control(pDX, IDC_TARGETSYNCOFFSET, m_edtTargetSyncOffset);
	DDX_Control(pDX, IDC_CONTROLLIMIT, m_edtControlLimit);
}

BOOL CPPageSync::OnInitDialog()
{
	__super::OnInitDialog();

	InitDialogPrivate();

	return TRUE;
}

BOOL CPPageSync::OnSetActive()
{
	InitDialogPrivate();

	return __super::OnSetActive();
}

void CPPageSync::InitDialogPrivate()
{
	AppSettings& s = AfxGetAppSettings();
	CRenderersSettings& rs = s.m_RenderersSettings;
	CRenderersSettings::CAdvRendererSettings& ars = s.m_RenderersSettings.m_AdvRendSets;

	CMainFrame * pFrame;
	pFrame = (CMainFrame *)(AfxGetApp()->m_pMainWnd);

	m_chkVMR9VSync.SetCheck(ars.iVMR9VSync);
	m_chkVMR9VSyncAccurate.SetCheck(ars.iVMR9VSyncAccurate);
	m_chkVMR9AlterativeVSync.SetCheck(ars.fVMR9AlterativeVSync);
	m_spnVMR9VSyncOffset.SetRange(-20, 20);
	m_edtVMR9VSyncOffset.SetRange(-20, 20);
	m_edtVMR9VSyncOffset = ars.iVMR9VSyncOffset;
	m_chkDisableAero.SetCheck(ars.iVMRDisableDesktopComposition);
	m_chkEnableFrameTimeCorrection.SetCheck(ars.iEVREnableFrameTimeCorrection);
	m_chkVMRFlushGPUBeforeVSync.SetCheck(ars.iVMRFlushGPUBeforeVSync);
	m_chkVMRFlushGPUAfterPresent.SetCheck(ars.iVMRFlushGPUAfterPresent);
	m_chkVMRFlushGPUWait.SetCheck(ars.iVMRFlushGPUWait);

	if ((s.iDSVideoRendererType == VIDRNDT_DS_EVR_CUSTOM || s.iDSVideoRendererType == VIDRNDT_DS_VMR9RENDERLESS) && rs.iAPSurfaceUsage == VIDRNDT_AP_TEXTURE3D) {
		m_chkVMR9VSync.EnableWindow(TRUE);
		m_chkVMR9VSyncAccurate.EnableWindow(TRUE);
		m_chkVMR9AlterativeVSync.EnableWindow(TRUE);
		m_chkVMRFlushGPUBeforeVSync.EnableWindow(TRUE);
		m_chkVMRFlushGPUAfterPresent.EnableWindow(TRUE);
		m_chkVMRFlushGPUWait.EnableWindow(TRUE);
	} else {
		m_chkVMR9VSync.EnableWindow(FALSE);
		m_chkVMR9AlterativeVSync.EnableWindow(FALSE);
		m_chkVMR9VSyncAccurate.EnableWindow(FALSE);
		m_chkVMRFlushGPUBeforeVSync.EnableWindow(FALSE);
		m_chkVMRFlushGPUAfterPresent.EnableWindow(FALSE);
		m_chkVMRFlushGPUWait.EnableWindow(FALSE);
	}
	OnAlterativeVSyncCheck();

	m_chkDisableAero.EnableWindow(IsWinVista() || IsWinSeven() ? TRUE : FALSE);
	m_chkEnableFrameTimeCorrection.EnableWindow(s.iDSVideoRendererType == VIDRNDT_DS_EVR_CUSTOM ? TRUE : FALSE);

	if ((s.iDSVideoRendererType == VIDRNDT_DS_SYNC) && (pFrame->GetPlaybackMode() == PM_NONE)) {
		GetDlgItem(IDC_SYNCVIDEO)->EnableWindow(TRUE);
		GetDlgItem(IDC_SYNCDISPLAY)->EnableWindow(TRUE);
		GetDlgItem(IDC_SYNCNEAREST)->EnableWindow(TRUE);
		GetDlgItem(IDC_TARGETSYNCOFFSET)->EnableWindow(TRUE);
		GetDlgItem(IDC_CYCLEDELTA)->EnableWindow(TRUE);
		GetDlgItem(IDC_LINEDELTA)->EnableWindow(TRUE);
		GetDlgItem(IDC_COLUMNDELTA)->EnableWindow(TRUE);
		GetDlgItem(IDC_CONTROLLIMIT)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC2)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC3)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC4)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC5)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC6)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC7)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC8)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC9)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC10)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC11)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC12)->EnableWindow(TRUE);
	} else {
		GetDlgItem(IDC_SYNCVIDEO)->EnableWindow(FALSE);
		GetDlgItem(IDC_SYNCDISPLAY)->EnableWindow(FALSE);
		GetDlgItem(IDC_SYNCNEAREST)->EnableWindow(FALSE);
		GetDlgItem(IDC_TARGETSYNCOFFSET)->EnableWindow(FALSE);
		GetDlgItem(IDC_CYCLEDELTA)->EnableWindow(FALSE);
		GetDlgItem(IDC_LINEDELTA)->EnableWindow(FALSE);
		GetDlgItem(IDC_COLUMNDELTA)->EnableWindow(FALSE);
		GetDlgItem(IDC_CONTROLLIMIT)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC2)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC3)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC4)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC5)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC6)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC7)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC8)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC9)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC10)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC11)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC12)->EnableWindow(FALSE);
	}

	m_bSynchronizeVideo = ars.bSynchronizeVideo;
	m_bSynchronizeDisplay = ars.bSynchronizeDisplay;
	m_bSynchronizeNearest = ars.bSynchronizeNearest;
	m_iLineDelta = ars.iLineDelta;
	m_iColumnDelta = ars.iColumnDelta;
	m_edtCycleDelta = ars.fCycleDelta;
	m_edtTargetSyncOffset = ars.fTargetSyncOffset;
	m_edtControlLimit = ars.fControlLimit;

	UpdateData(FALSE);
	SetModified(FALSE);
}

BOOL CPPageSync::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();
	CRenderersSettings::CAdvRendererSettings& ars = s.m_RenderersSettings.m_AdvRendSets;

	ars.iVMR9VSync						= !!m_chkVMR9VSync.GetCheck();
	ars.iVMR9VSyncAccurate				= !!m_chkVMR9VSyncAccurate.GetCheck();
	ars.fVMR9AlterativeVSync			= !!m_chkVMR9AlterativeVSync.GetCheck();
	ars.iVMR9VSyncOffset				 = m_edtVMR9VSyncOffset;
	ars.iVMRDisableDesktopComposition	= !!m_chkDisableAero.GetCheck();
	ars.iEVREnableFrameTimeCorrection	= !!m_chkEnableFrameTimeCorrection.GetCheck();
	ars.iVMRFlushGPUBeforeVSync			= !!m_chkVMRFlushGPUBeforeVSync.GetCheck();
	ars.iVMRFlushGPUAfterPresent		= !!m_chkVMRFlushGPUAfterPresent.GetCheck();
	ars.iVMRFlushGPUWait				= !!m_chkVMRFlushGPUWait.GetCheck();

	ars.bSynchronizeVideo	= !!m_bSynchronizeVideo;
	ars.bSynchronizeDisplay	= !!m_bSynchronizeDisplay;
	ars.bSynchronizeNearest	= !!m_bSynchronizeNearest;
	ars.iLineDelta			= m_iLineDelta;
	ars.iColumnDelta		= m_iColumnDelta;
	ars.fCycleDelta			= m_edtCycleDelta;
	ars.fTargetSyncOffset	= m_edtTargetSyncOffset;
	ars.fControlLimit		= m_edtControlLimit;

	return __super::OnApply();
}

BEGIN_MESSAGE_MAP(CPPageSync, CPPageBase)
	ON_BN_CLICKED(IDC_CHECK3, OnAlterativeVSyncCheck)
	ON_BN_CLICKED(IDC_SYNCVIDEO, OnBnClickedSyncVideo)
	ON_BN_CLICKED(IDC_SYNCDISPLAY, OnBnClickedSyncDisplay)
	ON_BN_CLICKED(IDC_SYNCNEAREST, OnBnClickedSyncNearest)
END_MESSAGE_MAP()

void CPPageSync::OnAlterativeVSyncCheck()
{
	AppSettings& s = AfxGetAppSettings();
	CRenderersSettings& rs = s.m_RenderersSettings;

	if (m_chkVMR9AlterativeVSync.GetCheck() == BST_CHECKED &&
			(s.iDSVideoRendererType == VIDRNDT_DS_EVR_CUSTOM || s.iDSVideoRendererType == VIDRNDT_DS_VMR9RENDERLESS) &&
			rs.iAPSurfaceUsage == VIDRNDT_AP_TEXTURE3D) {
		GetDlgItem(IDC_STATIC1)->EnableWindow(TRUE);
		GetDlgItem(IDC_EDIT1)->EnableWindow(TRUE);
		m_spnVMR9VSyncOffset.EnableWindow(TRUE);
	} else {
		GetDlgItem(IDC_STATIC1)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT1)->EnableWindow(FALSE);
		m_spnVMR9VSyncOffset.EnableWindow(FALSE);
	}
	SetModified();
}

void CPPageSync::OnBnClickedSyncVideo()
{
	m_bSynchronizeVideo = !m_bSynchronizeVideo;

	if (m_bSynchronizeVideo) {
		m_bSynchronizeDisplay = FALSE;
		m_bSynchronizeNearest = FALSE;
	}

	UpdateData(FALSE);

	SetModified();
}

void CPPageSync::OnBnClickedSyncDisplay()
{
	m_bSynchronizeDisplay = !m_bSynchronizeDisplay;

	if (m_bSynchronizeDisplay) {
		m_bSynchronizeVideo = FALSE;
		m_bSynchronizeNearest = FALSE;
	}

	UpdateData(FALSE);

	SetModified();
}

void CPPageSync::OnBnClickedSyncNearest()
{
	m_bSynchronizeNearest = !m_bSynchronizeNearest;

	if (m_bSynchronizeNearest) {
		m_bSynchronizeVideo = FALSE;
		m_bSynchronizeDisplay = FALSE;
	}

	UpdateData(FALSE);

	SetModified();
}

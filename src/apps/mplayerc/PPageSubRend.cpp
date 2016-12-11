/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2016 see Authors.txt
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
#include "PPageSubRend.h"


// CPPageSubRend dialog

IMPLEMENT_DYNAMIC(CPPageSubRend, CPPageBase)
CPPageSubRend::CPPageSubRend()
	: CPPageBase(CPPageSubRend::IDD, CPPageSubRend::IDD)
	, m_fOverridePlacement(FALSE)
	, m_nSPCSize(RS_SPCSIZE_DEF)
	, m_bSPCAllowAnimationWhenBuffering(TRUE)
	, m_bbSPAllowDropSubPic(TRUE)
	, m_nSubDelayInterval(0)
{
}

CPPageSubRend::~CPPageSubRend()
{
}

void CPPageSubRend::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Check(pDX, IDC_CHECK3, m_fOverridePlacement);
	DDX_Control(pDX, IDC_EDIT2, m_edtHorPos);
	DDX_Control(pDX, IDC_EDIT3, m_edtVerPos);
	DDX_Control(pDX, IDC_SPIN2, m_nHorPosCtrl);
	DDX_Control(pDX, IDC_SPIN3, m_nVerPosCtrl);
	DDX_Text(pDX, IDC_EDIT1, m_nSPCSize);
	DDX_Control(pDX, IDC_SPIN1, m_nSPCSizeCtrl);
	DDX_Control(pDX, IDC_COMBO1, m_spmaxres);
	DDX_Check(pDX, IDC_CHECK_SPCANIMWITHBUFFER, m_bSPCAllowAnimationWhenBuffering);
	DDX_Check(pDX, IDC_CHECK_ALLOW_DROPPING_SUBPIC, m_bbSPAllowDropSubPic);
	DDX_Text(pDX, IDC_EDIT4, m_nSubDelayInterval);

	m_nSPCSize = clamp(m_nSPCSize, RS_SPCSIZE_MIN, RS_SPCSIZE_MAX);
}

BEGIN_MESSAGE_MAP(CPPageSubRend, CPPageBase)
	ON_UPDATE_COMMAND_UI(IDC_EDIT2, OnUpdatePosOverride)
	ON_UPDATE_COMMAND_UI(IDC_SPIN2, OnUpdatePosOverride)
	ON_UPDATE_COMMAND_UI(IDC_EDIT3, OnUpdatePosOverride)
	ON_UPDATE_COMMAND_UI(IDC_SPIN3, OnUpdatePosOverride)
	ON_UPDATE_COMMAND_UI(IDC_STATIC1, OnUpdatePosOverride)
	ON_UPDATE_COMMAND_UI(IDC_STATIC2, OnUpdatePosOverride)
	ON_UPDATE_COMMAND_UI(IDC_STATIC3, OnUpdatePosOverride)
	ON_UPDATE_COMMAND_UI(IDC_STATIC4, OnUpdatePosOverride)
    ON_UPDATE_COMMAND_UI(IDC_CHECK_ALLOW_DROPPING_SUBPIC, OnUpdateAllowDropSubPic)
	ON_EN_CHANGE(IDC_EDIT4, OnSubDelayInterval)
END_MESSAGE_MAP()

// CPPageSubRend message handlers

static struct MAX_TEX_RES {
	const int width;
	const LPCWSTR name;
}
s_maxTexRes[] = {
	{   0, L"Desktop" },
	{2560, L"2560x1600"},
	{1920, L"1920x1080"},
	{1320, L"1320x900" },
	{1280, L"1280x720" },
	{1024, L"1024x768" },
	{ 800, L"800x600"  },
	{ 640, L"640x480"  },
	{ 512, L"512x384"  },
	{ 384, L"384x288"  }
};

int TexWidth2Index(int w)
{
	for (int i = 0; i < _countof(s_maxTexRes); i++) {
		if (s_maxTexRes[i].width == w) {
			return i;
		}
	}
	ASSERT(s_maxTexRes[4].width == 1280);
	return 4; // default 1280x720
}

int TexIndex2Width(int i)
{
	if (i >= 0 && i < _countof(s_maxTexRes)) {
		return s_maxTexRes[i].width;
	}

	return 1280; // default 1280x720
}

BOOL CPPageSubRend::OnInitDialog()
{
	__super::OnInitDialog();

	SetCursor(m_hWnd, IDC_COMBO1, IDC_HAND);

	CAppSettings& s = AfxGetAppSettings();

	m_fOverridePlacement = s.fOverridePlacement;
	m_edtHorPos = s.nHorPos;
	m_edtHorPos.SetRange(-10,110);
	m_nHorPosCtrl.SetRange(-10,110);
	m_edtVerPos = s.nVerPos;
	m_edtVerPos.SetRange(-10,110);
	m_nVerPosCtrl.SetRange(110,-10);
	m_nSPCSize = s.m_VRSettings.nSubpicCount;
	m_nSPCSizeCtrl.SetRange(RS_SPCSIZE_MIN, RS_SPCSIZE_MAX);
	for (int i = 0; i < _countof(s_maxTexRes); i++) {
		m_spmaxres.AddString(s_maxTexRes[i].name);
	}
	m_spmaxres.SetCurSel(TexWidth2Index(s.m_VRSettings.iSubpicMaxTexWidth));
	m_bSPCAllowAnimationWhenBuffering = s.m_VRSettings.bSubpicAnimationWhenBuffering;
	m_bbSPAllowDropSubPic = s.m_VRSettings.bSubpicAllowDrop;
	m_nSubDelayInterval = s.nSubDelayInterval;

	CorrectCWndWidth(GetDlgItem(IDC_CHECK3));

	UpdateData(FALSE);

	CreateToolTip();

	return TRUE;
}

BOOL CPPageSubRend::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	if (s.fOverridePlacement != !!m_fOverridePlacement
			|| s.nHorPos != m_edtHorPos
			|| s.nVerPos != m_edtVerPos
			|| s.m_VRSettings.nSubpicCount != m_nSPCSize
			|| s.nSubDelayInterval != m_nSubDelayInterval
			|| s.m_VRSettings.iSubpicMaxTexWidth != TexIndex2Width(m_spmaxres.GetCurSel())
			|| s.m_VRSettings.bSubpicAllowDrop != !!m_bbSPAllowDropSubPic
			|| s.m_VRSettings.bSubpicAnimationWhenBuffering != !!m_bSPCAllowAnimationWhenBuffering) {
		s.fOverridePlacement = !!m_fOverridePlacement;
		s.nHorPos = m_edtHorPos;
		s.nVerPos = m_edtVerPos;
		s.m_VRSettings.nSubpicCount = m_nSPCSize;
		s.nSubDelayInterval = m_nSubDelayInterval;
		s.m_VRSettings.iSubpicMaxTexWidth = TexIndex2Width(m_spmaxres.GetCurSel());
		s.m_VRSettings.bSubpicAllowDrop = !!m_bbSPAllowDropSubPic;
		s.m_VRSettings.bSubpicAnimationWhenBuffering = !!m_bSPCAllowAnimationWhenBuffering;

		AfxGetMainFrame()->UpdateSubtitle();
	}

	return __super::OnApply();
}

void CPPageSubRend::OnUpdatePosOverride(CCmdUI* pCmdUI)
{
	UpdateData();

	pCmdUI->Enable(m_fOverridePlacement);
}

void CPPageSubRend::OnUpdateAllowDropSubPic(CCmdUI* pCmdUI)
{
	UpdateData();

	pCmdUI->Enable(m_nSPCSize);
}

void CPPageSubRend::OnSubDelayInterval()
{
	// If incorrect number, revert modifications

	if (!UpdateData()) {
		UpdateData(FALSE);
	}

	SetModified();
}

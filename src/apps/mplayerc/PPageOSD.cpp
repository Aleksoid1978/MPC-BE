/*
 * (C) 2025 see Authors.txt
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
#include "PPageOSD.h"

// CPPageOSD dialog

IMPLEMENT_DYNAMIC(CPPageOSD, CPPageBase)
CPPageOSD::CPPageOSD()
	: CPPageBase(CPPageOSD::IDD, CPPageOSD::IDD)
{
}

void CPPageOSD::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Check(pDX, IDC_SHOW_OSD, m_bShowOSD);
	DDX_Check(pDX, IDC_CHECK14,  m_bOSDFileName);
	DDX_Check(pDX, IDC_CHECK15,  m_bOSDSeekTime);
}

BEGIN_MESSAGE_MAP(CPPageOSD, CPPageBase)
	ON_UPDATE_COMMAND_UI(IDC_CHECK14, OnUpdateOSD)
	ON_UPDATE_COMMAND_UI(IDC_CHECK15, OnUpdateOSD)
END_MESSAGE_MAP()

// CPPageOSD message handlers

BOOL CPPageOSD::OnInitDialog()
{
	__super::OnInitDialog();

	CAppSettings& s = AfxGetAppSettings();

	m_bShowOSD					= s.ShowOSD.Enable;
	m_bOSDFileName				= s.ShowOSD.FileName;
	m_bOSDSeekTime				= s.ShowOSD.SeekTime;

	return TRUE;
}

BOOL CPPageOSD::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();
	auto pFrame = AfxGetMainFrame();

	BOOL bShowOSDChanged = s.ShowOSD.Enable != m_bShowOSD;

	s.ShowOSD.Enable   = m_bShowOSD ? 1 : 0;
	s.ShowOSD.FileName = m_bOSDFileName ? 1 : 0;
	s.ShowOSD.SeekTime = m_bOSDSeekTime ? 1 : 0;
	if (bShowOSDChanged) {
		if (s.ShowOSD.Enable) {
			pFrame->m_OSD.Start(pFrame->m_pOSDWnd);
			pFrame->OSDBarSetPos();
			pFrame->m_OSD.ClearMessage(false);
		} else {
			pFrame->m_OSD.Stop();
		}
	}

	return __super::OnApply();
}

void CPPageOSD::OnUpdateOSD(CCmdUI* pCmdUI)
{
	UpdateData();

	pCmdUI->Enable(!!m_bShowOSD);
}

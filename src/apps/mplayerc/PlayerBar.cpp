/*
 * (C) 2014-2017 see Authors.txt
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
#include "PlayerBar.h"

IMPLEMENT_DYNAMIC(CPlayerBar, CSizingControlBarG)

CPlayerBar::CPlayerBar(void)
{
}

CPlayerBar::~CPlayerBar(void)
{
}

BEGIN_MESSAGE_MAP(CPlayerBar, CSizingControlBarG)
	ON_WM_WINDOWPOSCHANGED()
END_MESSAGE_MAP()

BOOL CPlayerBar::Create(LPCTSTR lpszWindowName, CWnd* pParentWnd, UINT nID, UINT defDockBarID, CString const& strSettingName)
{
	m_defDockBarID = defDockBarID;
	m_strSettingName = strSettingName;

	return __super::Create(lpszWindowName, pParentWnd, nID);
}

void CPlayerBar::LoadState(CFrameWnd *pParent)
{
	CMPlayerCApp* pApp = AfxGetMyApp();

	CRect r;
	pParent->GetWindowRect(r);
	CRect rDesktop;
	GetDesktopWindow()->GetWindowRect(&rDesktop);

	CString section = L"ToolBars\\" + m_strSettingName;

	__super::LoadState(section + L"\\State");

	UINT dockBarID = pApp->GetProfileInt(section, L"DockState", m_defDockBarID);

	if (dockBarID == AFX_IDW_DOCKBAR_FLOAT) {
		CPoint p;
		p.x = pApp->GetProfileInt(section, L"DockPosX", r.right);
		p.y = pApp->GetProfileInt(section, L"DockPosY", r.top);

		if (p.x < rDesktop.left) {
			p.x = rDesktop.left;
		}

		if (p.y < rDesktop.top) {
			p.y = rDesktop.top;
		}

		if (p.x >= rDesktop.right) {
			p.x = rDesktop.right-1;
		}

		if (p.y >= rDesktop.bottom) {
			p.y = rDesktop.bottom-1;
		}

		pParent->FloatControlBar(this, p);
	} else {
		pParent->DockControlBar(this, dockBarID);
	}
}

void CPlayerBar::SaveState()
{
	CMPlayerCApp* pApp = AfxGetMyApp();

	CString section = L"ToolBars\\" + m_strSettingName;

	__super::SaveState(section + L"\\State");

	UINT dockBarID = GetParent()->GetDlgCtrlID();

	if (dockBarID == AFX_IDW_DOCKBAR_FLOAT) {
		CRect r;
		GetParent()->GetParent()->GetWindowRect(r);
		pApp->WriteProfileInt(section, L"DockPosX", r.left);
		pApp->WriteProfileInt(section, L"DockPosY", r.top);
	}

	pApp->WriteProfileInt(section, L"DockState", dockBarID);
}

void CPlayerBar::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	__super::OnWindowPosChanged(lpwndpos);

	if (lpwndpos->flags & SWP_HIDEWINDOW) {
		GetParentFrame()->SetFocus();
	}
}

/*
 * (C) 2014-2019 see Authors.txt
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
	: m_defDockBarID(0)
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
	CProfile& profile = AfxGetProfile();

	CRect r;
	pParent->GetWindowRect(r);
	CRect rDesktop;
	GetDesktopWindow()->GetWindowRect(&rDesktop);

	CString section = L"ToolBars\\" + m_strSettingName;

	__super::LoadState(section + L"\\State");

	UINT dockBarID = m_defDockBarID;
	profile.ReadInt(section, L"DockState", *(int*)&dockBarID);

	if (dockBarID == AFX_IDW_DOCKBAR_FLOAT) {
		CPoint p(r.right, r.top);
		profile.ReadInt(section, L"DockPosX", *(int*)&p.x);
		profile.ReadInt(section, L"DockPosY", *(int*)&p.y);

		if (p.x < rDesktop.left) {
			p.x = rDesktop.left;
		}

		if (p.y < rDesktop.top) {
			p.y = rDesktop.top;
		}

		if (p.x >= rDesktop.right) {
			p.x = rDesktop.left;
		}

		if (p.y >= rDesktop.bottom) {
			p.y = rDesktop.top;
		}

		pParent->FloatControlBar(this, p);
	} else {
		pParent->DockControlBar(this, dockBarID);
	}
}

void CPlayerBar::SaveState()
{
	CProfile& profile = AfxGetProfile();

	CString section = L"ToolBars\\" + m_strSettingName;

	__super::SaveState(section + L"\\State");

	UINT dockBarID = GetParent()->GetDlgCtrlID();

	if (dockBarID == AFX_IDW_DOCKBAR_FLOAT) {
		CRect r;
		GetParent()->GetParent()->GetWindowRect(r);
		profile.WriteInt(section, L"DockPosX", r.left);
		profile.WriteInt(section, L"DockPosY", r.top);
	}

	profile.WriteInt(section, L"DockState", dockBarID);
}

void CPlayerBar::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	__super::OnWindowPosChanged(lpwndpos);

	if (lpwndpos->flags & SWP_HIDEWINDOW) {
		GetParentFrame()->SetFocus();
	}
}

CSize CPlayerBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
{
	CSize ret = __super::CalcFixedLayout(bStretch, bHorz);
	if (bStretch == TRUE && bHorz == TRUE && IsVertDocked()) {
		ret.cy += m_cyGripper + 5;
	}
	return ret;
}

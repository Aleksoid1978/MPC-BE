/*
 * (C) 2016-2023 see Authors.txt
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
#include "CDDAReaderSettingsWnd.h"

CCDDAReaderSettingsWnd::CCDDAReaderSettingsWnd(void)
{
}

bool CCDDAReaderSettingsWnd::OnConnect(const std::list<CComQIPtr<IUnknown, &IID_IUnknown>>& pUnks)
{
	ASSERT(!m_pMSF);

	m_pMSF.Release();

	for (auto& pUnk : pUnks) {
		m_pMSF = pUnk;
		if (m_pMSF) {
			return true;
		}
	}

	return false;
}

void CCDDAReaderSettingsWnd::OnDisconnect()
{
	m_pMSF.Release();
}

bool CCDDAReaderSettingsWnd::OnActivate()
{
	DWORD dwStyle = WS_VISIBLE | WS_CHILD | WS_TABSTOP;
	CPoint p(10, 10);

	m_cbReadTextInfo.Create (ResStr(IDS_CDDAREADER_READ_TEXT_INFO), dwStyle|BS_AUTOCHECKBOX|BS_LEFTTEXT, CRect(p, CSize(ScaleX(270), m_fontheight)), this, IDC_STATIC);

	if (m_pMSF) {
		m_cbReadTextInfo.SetCheck(m_pMSF->GetReadTextInfo());
	}

	for (CWnd* pWnd = GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetNextWindow()) {
		pWnd->SetFont(&m_font, FALSE);
	}

	SetCursor(m_hWnd, IDC_ARROW);
	SetCursor(m_cbReadTextInfo.m_hWnd, IDC_HAND);

	return true;
}

void CCDDAReaderSettingsWnd::OnDeactivate()
{
}

bool CCDDAReaderSettingsWnd::OnApply()
{
	OnDeactivate();

	if (m_pMSF) {
		m_pMSF->SetReadTextInfo(m_cbReadTextInfo.GetCheck());
		m_pMSF->Apply();
	}

	return true;
}

BEGIN_MESSAGE_MAP(CCDDAReaderSettingsWnd, CInternalPropertyPageWnd)
END_MESSAGE_MAP()

/*
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
#include "VTSReaderSettingsWnd.h"
#include "../../../DSUtil/MFCHelper.h"

CVTSReaderSettingsWnd::CVTSReaderSettingsWnd(void)
{
}

bool CVTSReaderSettingsWnd::OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks)
{
	ASSERT(!m_pMSF);

	m_pMSF.Release();

	POSITION pos = pUnks.GetHeadPosition();
	while (pos && !(m_pMSF = pUnks.GetNext(pos))) {
		;
	}

	if (!m_pMSF) {
		return false;
	}

	return true;
}

void CVTSReaderSettingsWnd::OnDisconnect()
{
	m_pMSF.Release();
}

bool CVTSReaderSettingsWnd::OnActivate()
{
	DWORD dwStyle = WS_VISIBLE | WS_CHILD | WS_TABSTOP;
	CPoint p(10, 10);

	m_cbEnableTitleSelection.Create(ResStr(IDS_VTSREADER_LOAD_PGC), dwStyle | BS_AUTOCHECKBOX | BS_LEFTTEXT | WS_DISABLED, CRect(p, CSize(ScaleX(300), m_fontheight)), this, IDC_STATIC);

	if (m_pMSF) {
		m_cbEnableTitleSelection.SetCheck(m_pMSF->GetEnableTitleSelection());
	}

	for (CWnd* pWnd = GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetNextWindow()) {
		pWnd->SetFont(&m_font, FALSE);
	}

	SetCursor(m_hWnd, IDC_ARROW);
	SetCursor(m_cbEnableTitleSelection.m_hWnd, IDC_HAND);

	SetDirty(false);

	return true;
}

void CVTSReaderSettingsWnd::OnDeactivate()
{
}

bool CVTSReaderSettingsWnd::OnApply()
{
	OnDeactivate();

	if (m_pMSF) {
		m_pMSF->SetEnableTitleSelection(m_cbEnableTitleSelection.GetCheck());
		m_pMSF->Apply();
	}

	return true;
}

BEGIN_MESSAGE_MAP(CVTSReaderSettingsWnd, CInternalPropertyPageWnd)
END_MESSAGE_MAP()

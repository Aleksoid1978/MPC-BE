/*
 * (C) 2006-2025 see Authors.txt
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
#include "MatroskaSplitterSettingsWnd.h"

CMatroskaSplitterSettingsWnd::CMatroskaSplitterSettingsWnd(void)
{
}

bool CMatroskaSplitterSettingsWnd::OnConnect(const std::list<CComQIPtr<IUnknown, &IID_IUnknown>>& pUnks)
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

void CMatroskaSplitterSettingsWnd::OnDisconnect()
{
	m_pMSF.Release();
}

bool CMatroskaSplitterSettingsWnd::OnActivate()
{
	DWORD dwStyle = WS_VISIBLE | WS_CHILD | WS_TABSTOP;
	CPoint p(10, 10);

	m_cbLoadEmbeddedFonts.Create(ResStr(IDS_MKVSPLT_LOAD_EMBEDDED_FONTS), dwStyle | BS_AUTOCHECKBOX | BS_LEFTTEXT, CRect(p, CSize(ScaleX(290), m_fontheight)), this, IDC_STATIC);
	p.y += ScaleY(20);

	m_cbCalcDuration.Create(ResStr(IDS_MKVSPLT_CALC_DURATION), dwStyle | BS_AUTOCHECKBOX | BS_LEFTTEXT, CRect(p, CSize(ScaleX(290), m_fontheight)), this, IDC_STATIC);
	p.y += ScaleY(20);

	m_cbReindex.Create(ResStr(IDS_MKVSPLT_REINDEX), dwStyle | BS_AUTOCHECKBOX | BS_LEFTTEXT, CRect(p, CSize(ScaleX(290), m_fontheight)), this, IDC_STATIC);

	if (m_pMSF) {
		m_cbLoadEmbeddedFonts.SetCheck(m_pMSF->GetLoadEmbeddedFonts());
		m_cbCalcDuration.SetCheck(m_pMSF->GetCalcDuration());
		m_cbReindex.SetCheck(m_pMSF->GetReindex());
	}

	for (CWnd* pWnd = GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetNextWindow()) {
		pWnd->SetFont(&m_font, FALSE);
	}

	SetDirty(false);

	return true;
}

void CMatroskaSplitterSettingsWnd::OnDeactivate()
{
}

bool CMatroskaSplitterSettingsWnd::OnApply()
{
	OnDeactivate();

	if (m_pMSF) {
		m_pMSF->SetLoadEmbeddedFonts(m_cbLoadEmbeddedFonts.GetCheck());
		m_pMSF->SetCalcDuration(m_cbCalcDuration.GetCheck());
		m_pMSF->SetReindex(m_cbReindex.GetCheck());
		m_pMSF->Apply();
	}

	return true;
}

BEGIN_MESSAGE_MAP(CMatroskaSplitterSettingsWnd, CInternalPropertyPageWnd)
END_MESSAGE_MAP()

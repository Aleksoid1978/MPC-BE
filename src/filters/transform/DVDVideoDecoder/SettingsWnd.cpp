/*
 * (C) 2003-2006 Gabest
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
#include "SettingsWnd.h"
#include "DSUtil/MFCHelper.h"

//
// CMpeg2DecSettingsWnd
//

static WCHAR m_strBrightness[100];
static WCHAR m_strContrast[100];
static WCHAR m_strHue[100];
static WCHAR m_strSaturation[100];

CMpeg2DecSettingsWnd::CMpeg2DecSettingsWnd()
{
	wcscpy_s(m_strBrightness, ResStr(IDS_MPEG2_BRIGHTNESS));
	wcscpy_s(m_strContrast,   ResStr(IDS_MPEG2_CONTRAST));
	wcscpy_s(m_strHue,        ResStr(IDS_MPEG2_HUE));
	wcscpy_s(m_strSaturation, ResStr(IDS_MPEG2_SATURATION));
}

bool CMpeg2DecSettingsWnd::OnConnect(const std::list<CComQIPtr<IUnknown, &IID_IUnknown>>& pUnks)
{
	ASSERT(!m_pM2DF);

	m_pM2DF.Release();

	for (auto& pUnk : pUnks) {
		m_pM2DF = pUnk;
		if (m_pM2DF) {
			break;
		}
	}

	if (!m_pM2DF) {
		return false;
	}

	m_ditype = m_pM2DF->GetDeinterlaceMethod();
	m_procamp[0] = m_pM2DF->GetBrightness();
	m_procamp[1] = m_pM2DF->GetContrast();
	m_procamp[2] = m_pM2DF->GetHue();
	m_procamp[3] = m_pM2DF->GetSaturation();
	m_forcedsubs = m_pM2DF->IsForcedSubtitlesEnabled();
	m_interlaced = m_pM2DF->IsInterlacedEnabled();

	return true;
}

void CMpeg2DecSettingsWnd::OnDisconnect()
{
	m_pM2DF.Release();
}

bool CMpeg2DecSettingsWnd::OnActivate()
{
	const int h20 = ScaleY(20);
	const int h25 = ScaleY(25);
	DWORD dwStyle = WS_VISIBLE|WS_CHILD|WS_TABSTOP;
	CPoint p(10, 10);

	m_interlaced_check.Create(ResStr(IDS_MPEG2_INTERLACE_FLAG), dwStyle | BS_AUTOCHECKBOX, CRect(p, CSize(ScaleX(300), m_fontheight)), this, IDC_PP_CHECK2);
	m_interlaced_check.SetCheck(m_interlaced ? BST_CHECKED : BST_UNCHECKED);
	p.y += h20;

	m_forcedsubs_check.Create(ResStr(IDS_MPEG2_FORCED_SUBS), dwStyle | BS_AUTOCHECKBOX, CRect(p, CSize(ScaleX(300), m_fontheight)), this, IDC_PP_CHECK3);
	m_forcedsubs_check.SetCheck(m_forcedsubs ? BST_CHECKED : BST_UNCHECKED);
	p.y += h20;

	m_ditype_static.Create(ResStr(IDS_MPEG2_DEINTERLACING), WS_VISIBLE | WS_CHILD, CRect(p, CSize(ScaleX(100), m_fontheight)), this);
	m_ditype_combo.Create(dwStyle | CBS_DROPDOWNLIST, CRect(p + CSize(ScaleX(110), -4), CSize(ScaleX(100), 200)), this, IDC_PP_COMBO1);
	AddStringData(m_ditype_combo, L"Auto",  DIAuto);
	AddStringData(m_ditype_combo, L"Weave", DIWeave);
	AddStringData(m_ditype_combo, L"Blend", DIBlend);
	AddStringData(m_ditype_combo, L"Bob",   DIBob);
	m_ditype_combo.SetCurSel(0);
	SelectByItemData(m_ditype_combo, m_ditype);
	m_ditype_combo.EnableWindow(!IsDlgButtonChecked(m_interlaced_check.GetDlgCtrlID()));
	p.y += h25;

	{
		int h = std::max(21, m_fontheight); // special size for sliders
		static const WCHAR* labels[] = {m_strBrightness, m_strContrast, m_strHue, m_strSaturation};
		for (unsigned i = 0; i < std::size(m_procamp_slider); i++) {
			m_procamp_static[i].Create(labels[i], WS_VISIBLE | WS_CHILD, CRect(p, CSize(ScaleX(80), m_fontheight)), this);
			m_procamp_slider[i].Create(dwStyle|TBS_DOWNISLEFT, CRect(p + CPoint(ScaleX(85), 0), CSize(ScaleX(201), h)), this, IDC_PP_SLIDER1 + i);
			m_procamp_value[i].Create(L"", WS_VISIBLE | WS_CHILD, CRect(p + CPoint(ScaleX(85 + 201), 0), CSize(ScaleX(30), m_fontheight)), this);
			p.y += h;
		}
		m_procamp_slider[0].SetRange(-128, 128, TRUE);
		m_procamp_slider[0].SetTic(0);
		m_procamp_slider[0].SetPos(m_procamp[0]);
		m_procamp_slider[1].SetRange(0, 200, TRUE);
		m_procamp_slider[1].SetTic(100);
		m_procamp_slider[1].SetPos(m_procamp[1]);
		m_procamp_slider[2].SetRange(-180, 180, TRUE);
		m_procamp_slider[2].SetTic(0);
		m_procamp_slider[2].SetPos(m_procamp[2]);
		m_procamp_slider[3].SetRange(0, 200, TRUE);
		m_procamp_slider[3].SetTic(100);
		m_procamp_slider[3].SetPos(m_procamp[3]);
		p.y += 5;

		m_procamp_tv2pc.Create(L"TV->PC", dwStyle, CRect(p + CPoint(ScaleX(85 + 200 / 2 - 80 -5), 0), CSize(ScaleX(80), m_fontheight + 6)), this, IDC_PP_BUTTON1);
		m_procamp_reset.Create(ResStr(IDS_MPEG2_RESET), dwStyle, CRect(p + CPoint(ScaleX(85 + 200 / 2 + 6), 0), CSize(ScaleX(80), m_fontheight + 6)), this, IDC_PP_BUTTON2);
		p.y += h25;

		UpdateProcampValues();
	}

	m_note_static.Create(
		ResStr(IDS_MPEG2_NOTE1), WS_VISIBLE | WS_CHILD, CRect(p, CSize(ScaleX(320), m_fontheight * 4)), this);

	for (CWnd* pWnd = GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetNextWindow()) {
		pWnd->SetFont(&m_font, FALSE);
	}

	SetCursor(m_hWnd, IDC_ARROW);
	SetCursor(m_hWnd, IDC_PP_CHECK1, IDC_HAND);

	return true;
}

void CMpeg2DecSettingsWnd::OnDeactivate()
{
	m_ditype = (ditype)GetCurItemData(m_ditype_combo);
	m_procamp[0] = (float)m_procamp_slider[0].GetPos();
	m_procamp[1] = (float)m_procamp_slider[1].GetPos();
	m_procamp[2] = (float)m_procamp_slider[2].GetPos();
	m_procamp[3] = (float)m_procamp_slider[3].GetPos();
	m_interlaced = !!IsDlgButtonChecked(m_interlaced_check.GetDlgCtrlID());
	m_forcedsubs = !!IsDlgButtonChecked(m_forcedsubs_check.GetDlgCtrlID());
}

bool CMpeg2DecSettingsWnd::OnApply()
{
	OnDeactivate();

	if (m_pM2DF) {
		m_pM2DF->SetDeinterlaceMethod(m_ditype);
		m_pM2DF->SetBrightness(m_procamp[0]);
		m_pM2DF->SetContrast(m_procamp[1]);
		m_pM2DF->SetHue(m_procamp[2]);
		m_pM2DF->SetSaturation(m_procamp[3]);
		m_pM2DF->EnableForcedSubtitles(m_forcedsubs);
		m_pM2DF->EnableInterlaced(m_interlaced);
		m_pM2DF->Apply();
	}

	return true;
}

void CMpeg2DecSettingsWnd::UpdateProcampValues()
{
	CStringW str;

	str.Format(L"%d", m_procamp_slider[0].GetPos());
	m_procamp_value[0].SetWindowTextW(str);
	str.Format(L"%d%%", m_procamp_slider[1].GetPos());
	m_procamp_value[1].SetWindowTextW(str);
	str.Format(L"%d", m_procamp_slider[2].GetPos());
	m_procamp_value[2].SetWindowTextW(str);
	str.Format(L"%d%%", m_procamp_slider[3].GetPos());
	m_procamp_value[3].SetWindowTextW(str);
}

BEGIN_MESSAGE_MAP(CMpeg2DecSettingsWnd, CInternalPropertyPageWnd)
	ON_BN_CLICKED(IDC_PP_BUTTON1, OnButtonProcampPc2Tv)
	ON_BN_CLICKED(IDC_PP_BUTTON2, OnButtonProcampReset)
	ON_BN_CLICKED(IDC_PP_CHECK2, OnButtonInterlaced)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

void CMpeg2DecSettingsWnd::OnButtonProcampPc2Tv()
{
	m_procamp_slider[0].SetPos(- 16);
	m_procamp_slider[1].SetPos(100 * 255/(235-16));

	UpdateProcampValues();
}

void CMpeg2DecSettingsWnd::OnButtonProcampReset()
{
	m_procamp_slider[0].SetPos(0);
	m_procamp_slider[1].SetPos(100);
	m_procamp_slider[2].SetPos(0);
	m_procamp_slider[3].SetPos(100);

	UpdateProcampValues();
}

void CMpeg2DecSettingsWnd::OnButtonInterlaced()
{
	m_ditype_combo.EnableWindow(!IsDlgButtonChecked(m_interlaced_check.GetDlgCtrlID()));
}

void CMpeg2DecSettingsWnd::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	UpdateProcampValues();

	__super::OnHScroll(nSBCode, nPos, pScrollBar);
}

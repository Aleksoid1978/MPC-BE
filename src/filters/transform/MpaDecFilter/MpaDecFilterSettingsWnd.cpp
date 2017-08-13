/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2017 see Authors.txt
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
#include "MpaDecFilterSettingsWnd.h"
#include "../../../DSUtil/MFCHelper.h"
#include "../../../DSUtil/SysVersion.h"

//
// CMpaDecSettingsWnd
//

CMpaDecSettingsWnd::CMpaDecSettingsWnd()
{
}

bool CMpaDecSettingsWnd::OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks)
{
	ASSERT(!m_pMDF);

	m_pMDF.Release();

	POSITION pos = pUnks.GetHeadPosition();
	while (pos && !(m_pMDF = pUnks.GetNext(pos))) {
		;
	}

	if (!m_pMDF) {
		return false;
	}

#ifdef REGISTER_FILTER
	m_outfmt_i16   = m_pMDF->GetOutputFormat(SF_PCM16);
	m_outfmt_i24   = m_pMDF->GetOutputFormat(SF_PCM24);
	m_outfmt_i32   = m_pMDF->GetOutputFormat(SF_PCM32);
	m_outfmt_flt   = m_pMDF->GetOutputFormat(SF_FLOAT);
#endif
	m_av_sync      = m_pMDF->GetAVSyncCorrection();
	m_drc          = m_pMDF->GetDynamicRangeControl();
	m_spdif_ac3    = m_pMDF->GetSPDIF(IMpaDecFilter::ac3);
	m_spdif_eac3   = m_pMDF->GetSPDIF(IMpaDecFilter::eac3);
	m_spdif_truehd = m_pMDF->GetSPDIF(IMpaDecFilter::truehd);
	m_spdif_dts    = m_pMDF->GetSPDIF(IMpaDecFilter::dts);
	m_spdif_dtshd  = m_pMDF->GetSPDIF(IMpaDecFilter::dtshd);
	m_spdif_ac3enc = m_pMDF->GetSPDIF(IMpaDecFilter::ac3enc);;

	return true;
}

void CMpaDecSettingsWnd::OnDisconnect()
{
	m_pMDF.Release();
}

void CMpaDecSettingsWnd::UpdateStatusInfo()
{
	CString str = m_pMDF->GetInformation(AINFO_DecoderInfo);
	m_edtStatus.SetWindowText(str);
}

static WNDPROC OldControlProc;
static LRESULT CALLBACK ControlProc(HWND control, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_KEYDOWN) {
		if (LOWORD(wParam) == VK_ESCAPE) {
			return 0;    // just ignore ESCAPE in edit control
		}
		if ((LOWORD(wParam)== 'A' || LOWORD(wParam) == 'a')
				&&(GetKeyState(VK_CONTROL) < 0)) {
			CEdit *pEdit = (CEdit*)CWnd::FromHandle(control);
			pEdit->SetSel(0, pEdit->GetWindowTextLength(),TRUE);
			return 0;
		}
	}

	return CallWindowProc(OldControlProc, control, message, wParam, lParam); // call edit control's own windowproc
}


bool CMpaDecSettingsWnd::OnActivate()
{
	const int h20 = ScaleY(20);
	const int h25 = ScaleY(25);
	DWORD dwStyle = WS_VISIBLE|WS_CHILD|WS_TABSTOP;
	CPoint p(10, 10);
	CRect r;

#ifdef REGISTER_FILTER
	m_outfmt_group.Create(ResStr(IDS_MPADEC_SAMPLE_FMT), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, CRect(p + CPoint(-5, 0), CSize(ScaleX(230), h20 + h20)), this, (UINT)IDC_STATIC);
	p.y += h20;
	m_outfmt_i16_check.Create(L"Int16", dwStyle | BS_AUTOCHECKBOX, CRect(p, CSize(ScaleX(50), m_fontheight)), this, IDC_PP_CHECK_I16);
	m_outfmt_i24_check.Create(L"Int24", dwStyle | BS_AUTOCHECKBOX, CRect(p + CPoint(ScaleX(55), 0), CSize(ScaleX(50), m_fontheight)), this, IDC_PP_CHECK_I24);
	m_outfmt_i32_check.Create(L"Int32", dwStyle | BS_AUTOCHECKBOX, CRect(p + CPoint(ScaleX(110), 0), CSize(ScaleX(50), m_fontheight)), this, IDC_PP_CHECK_I32);
	m_outfmt_flt_check.Create(L"Float", dwStyle | BS_AUTOCHECKBOX, CRect(p + CPoint(ScaleX(165), 0), CSize(ScaleX(50), m_fontheight)), this, IDC_PP_CHECK_FLT);
	m_outfmt_i16_check.SetCheck(m_outfmt_i16);
	m_outfmt_i24_check.SetCheck(m_outfmt_i24);
	m_outfmt_i32_check.SetCheck(m_outfmt_i32);
	m_outfmt_flt_check.SetCheck(m_outfmt_flt);
	p.y += h25;
#endif

	m_av_sync_check.Create(ResStr(IDS_MPADEC_AV_SYNC), dwStyle | BS_AUTOCHECKBOX, CRect(p, CSize(ScaleX(220), m_fontheight)), this, IDC_PP_CHECK_AV_SYNC);
	m_av_sync_check.SetCheck(m_av_sync);
	p.y += h20;

	m_drc_check.Create(ResStr(IDS_MPADEC_DRC), dwStyle | BS_AUTOCHECKBOX, CRect(p, CSize(ScaleX(220), m_fontheight)), this, IDC_PP_CHECK_DRC);
	m_drc_check.SetCheck(m_drc);
	p.y += h25;

	m_spdif_group.Create(ResStr(IDS_MPADEC_SPDIF), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, CRect(p + CPoint(-5, 0), CSize(ScaleX(230), h20 + h20 * 4)), this, (UINT)IDC_STATIC);
	p.y += h20;
	m_spdif_ac3_check.Create(L"AC-3", dwStyle | BS_AUTOCHECKBOX, CRect(p, CSize(ScaleX(50), m_fontheight)), this, IDC_PP_CHECK_SPDIF_AC3);
	m_spdif_dts_check.Create(L"DTS", dwStyle | BS_AUTOCHECKBOX, CRect(p + CPoint(ScaleX(110), 0), CSize(ScaleX(50), m_fontheight)), this, IDC_PP_CHECK_SPDIF_DTS);
	p.y += h20;
	m_spdif_eac3_check.Create(L"E-AC3", dwStyle | BS_AUTOCHECKBOX, CRect(p, CSize(ScaleX(55), m_fontheight)), this, IDC_PP_CHECK_SPDIF_EAC3);
	m_spdif_dtshd_check.Create(L"DTS-HD", dwStyle | BS_AUTOCHECKBOX, CRect(p + CPoint(ScaleX(110), 0), CSize(ScaleX(65), m_fontheight)), this, IDC_PP_CHECK_SPDIF_DTSHD);
	p.y += h20;
	m_spdif_truehd_check.Create(L"TrueHD", dwStyle | BS_AUTOCHECKBOX, CRect(p, CSize(ScaleX(60), m_fontheight)), this, IDC_PP_CHECK_SPDIF_TRUEHD);
	m_spdif_ac3_check.SetCheck(m_spdif_ac3);
	m_spdif_eac3_check.SetCheck(m_spdif_eac3);
	m_spdif_truehd_check.SetCheck(m_spdif_truehd);
	m_spdif_dts_check.SetCheck(m_spdif_dts);
	m_spdif_dtshd_check.SetCheck(m_spdif_dtshd);
	p.y += h20;
	m_spdif_ac3enc_check.Create(ResStr(IDS_MPADEC_AC3ENCODE), dwStyle | BS_AUTOCHECKBOX, CRect(p, CSize(ScaleX(115), m_fontheight)), this, IDC_PP_CHECK_SPDIF_AC3ENC);
	m_spdif_ac3enc_check.SetCheck(m_spdif_ac3enc);

	OnDTSCheck();

	////////// Status //////////
	p.y += h20;
	m_grpStatus.Create(ResStr(IDS_MPADEC_STATUS), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, CRect(p + CPoint(-5, 0), CSize(ScaleX(230), h25 + m_fontheight * 4 + m_fontheight / 2)), this, (UINT)IDC_STATIC);
	p.y += h20;
	m_edtStatus.Create(WS_CHILD | WS_VISIBLE /*| WS_BORDER*/ | ES_READONLY | ES_MULTILINE, CRect(p, CSize(ScaleX(220), m_fontheight * 4 + m_fontheight / 2)), this, 0);

	if (m_pMDF) {
		UpdateStatusInfo();
	}

	for (CWnd* pWnd = GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetNextWindow()) {
		pWnd->SetFont(&m_font, FALSE);
	}

	SetCursor(m_hWnd, IDC_ARROW);
	SetCursor(m_hWnd, IDC_PP_CHECK_AV_SYNC, IDC_HAND);

	// subclass the edit control
	OldControlProc = (WNDPROC)SetWindowLongPtr(m_edtStatus.m_hWnd, GWLP_WNDPROC, (LONG_PTR)ControlProc);

	return true;
}

void CMpaDecSettingsWnd::OnDeactivate()
{
#ifdef REGISTER_FILTER
	m_outfmt_i16   = !!m_outfmt_i16_check.GetCheck();
	m_outfmt_i24   = !!m_outfmt_i24_check.GetCheck();
	m_outfmt_i32   = !!m_outfmt_i32_check.GetCheck();
	m_outfmt_flt   = !!m_outfmt_flt_check.GetCheck();
#endif
	m_av_sync      = !!m_av_sync_check.GetCheck();
	m_drc          = !!m_drc_check.GetCheck();
	m_spdif_ac3    = !!m_spdif_ac3_check.GetCheck();
	m_spdif_eac3   = !!m_spdif_eac3_check.GetCheck();
	m_spdif_truehd = !!m_spdif_truehd_check.GetCheck();
	m_spdif_dts    = !!m_spdif_dts_check.GetCheck();
	m_spdif_dtshd  = !!m_spdif_dtshd_check.GetCheck();
	m_spdif_ac3enc = !!m_spdif_ac3enc_check.GetCheck();
}

bool CMpaDecSettingsWnd::OnApply()
{
	OnDeactivate();

	if (m_pMDF) {
#ifdef REGISTER_FILTER
		m_pMDF->SetOutputFormat(SF_PCM16, m_outfmt_i16);
		m_pMDF->SetOutputFormat(SF_PCM24, m_outfmt_i24);
		m_pMDF->SetOutputFormat(SF_PCM32, m_outfmt_i32);
		m_pMDF->SetOutputFormat(SF_FLOAT, m_outfmt_flt);
#endif
		m_pMDF->SetAVSyncCorrection(m_av_sync);
		m_pMDF->SetDynamicRangeControl(m_drc);
		m_pMDF->SetSPDIF(IMpaDecFilter::ac3, m_spdif_ac3);
		m_pMDF->SetSPDIF(IMpaDecFilter::eac3, m_spdif_eac3);
		m_pMDF->SetSPDIF(IMpaDecFilter::truehd, m_spdif_truehd);
		m_pMDF->SetSPDIF(IMpaDecFilter::dts, m_spdif_dts);
		m_pMDF->SetSPDIF(IMpaDecFilter::dtshd, m_spdif_dtshd);
		m_pMDF->SetSPDIF(IMpaDecFilter::ac3enc, m_spdif_ac3enc);

		m_pMDF->SaveSettings();
	}

	return true;
}

BEGIN_MESSAGE_MAP(CMpaDecSettingsWnd, CInternalPropertyPageWnd)
#ifdef REGISTER_FILTER
	ON_BN_CLICKED(IDC_PP_CHECK_I16, OnInt16Check)
	ON_BN_CLICKED(IDC_PP_CHECK_I24, OnInt24Check)
	ON_BN_CLICKED(IDC_PP_CHECK_I32, OnInt32Check)
	ON_BN_CLICKED(IDC_PP_CHECK_FLT, OnFloatCheck)
#endif
	ON_BN_CLICKED(IDC_PP_CHECK_SPDIF_DTS, OnDTSCheck)
END_MESSAGE_MAP()

#ifdef REGISTER_FILTER
void CMpaDecSettingsWnd::OnInt16Check()
{
	if (m_outfmt_i16_check.GetCheck() == BST_UNCHECKED &&
			m_outfmt_i24_check.GetCheck() == BST_UNCHECKED &&
			m_outfmt_i32_check.GetCheck() == BST_UNCHECKED &&
			m_outfmt_flt_check.GetCheck() == BST_UNCHECKED) {
		m_outfmt_i16_check.SetCheck(BST_CHECKED);
	}
}

void CMpaDecSettingsWnd::OnInt24Check()
{
	if (m_outfmt_i16_check.GetCheck() == BST_UNCHECKED &&
			m_outfmt_i24_check.GetCheck() == BST_UNCHECKED &&
			m_outfmt_i32_check.GetCheck() == BST_UNCHECKED &&
			m_outfmt_flt_check.GetCheck() == BST_UNCHECKED) {
		m_outfmt_i24_check.SetCheck(BST_CHECKED);
	}
}

void CMpaDecSettingsWnd::OnInt32Check()
{
	if (m_outfmt_i16_check.GetCheck() == BST_UNCHECKED &&
			m_outfmt_i24_check.GetCheck() == BST_UNCHECKED &&
			m_outfmt_i32_check.GetCheck() == BST_UNCHECKED &&
			m_outfmt_flt_check.GetCheck() == BST_UNCHECKED) {
		m_outfmt_i32_check.SetCheck(BST_CHECKED);
	}
}

void CMpaDecSettingsWnd::OnFloatCheck()
{
	if (m_outfmt_i16_check.GetCheck() == BST_UNCHECKED &&
			m_outfmt_i24_check.GetCheck() == BST_UNCHECKED &&
			m_outfmt_i32_check.GetCheck() == BST_UNCHECKED &&
			m_outfmt_flt_check.GetCheck() == BST_UNCHECKED) {
		m_outfmt_flt_check.SetCheck(BST_CHECKED);
	}
}
#endif

void CMpaDecSettingsWnd::OnDTSCheck()
{
	m_spdif_dtshd_check.EnableWindow(!!m_spdif_dts_check.GetCheck());
}

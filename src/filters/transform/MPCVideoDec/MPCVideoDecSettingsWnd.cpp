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
#include "MPCVideoDecSettingsWnd.h"
#include "../../../DSUtil/DSUtil.h"
#include <ffmpeg/libavcodec/avcodec.h>

//
// CMPCVideoDecSettingsWnd
//

int g_AVDiscard[] = {
	AVDISCARD_NONE,
	AVDISCARD_DEFAULT,
	AVDISCARD_NONREF,
	AVDISCARD_BIDIR,
	AVDISCARD_NONKEY,
	AVDISCARD_ALL,
};

int FindDiscardIndex(int nValue)
{
	for (int i=0; i<_countof (g_AVDiscard); i++)
		if (g_AVDiscard[i] == nValue) {
			return i;
		}
	return 1;
}

CMPCVideoDecSettingsWnd::CMPCVideoDecSettingsWnd()
{
}

bool CMPCVideoDecSettingsWnd::OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks)
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

	return true;
}

void CMPCVideoDecSettingsWnd::OnDisconnect()
{
	m_pMDF.Release();
}

void CMPCVideoDecSettingsWnd::UpdateStatusInfo()
{
	CString str;
	m_edtInputFormat.SetWindowText(m_pMDF->GetInformation(INFO_InputFormat));
	m_edtFrameSize.SetWindowText(m_pMDF->GetInformation(INFO_FrameSize));
	m_edtOutputFormat.SetWindowText(m_pMDF->GetInformation(INFO_OutputFormat));
	m_edtGraphicsAdapter.SetWindowText(m_pMDF->GetInformation(INFO_GraphicsAdapter));
}

bool CMPCVideoDecSettingsWnd::OnActivate()
{
	ASSERT(IPP_FONTSIZE == 13);
	const int h16 = IPP_SCALE(16);
	const int h20 = IPP_SCALE(20);
	const int h25 = IPP_SCALE(25);
	DWORD dwStyle = WS_VISIBLE | WS_CHILD | WS_TABSTOP;

	int label_w = IPP_SCALE(225);
	int combo_w = IPP_SCALE(110);
	int width_s = label_w + combo_w;

	////////// Video settings //////////
	CPoint p(10, 10);
	m_grpDecoder.Create(ResStr(IDS_VDF_SETTINGS), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, CRect(p + CPoint(-5, 0), CSize(width_s + 10, IPP_SCALE(115))), this, (UINT)IDC_STATIC);
	p.y += h20;

	// Decoding threads
	m_txtThreadNumber.Create(ResStr(IDS_VDF_THREADNUMBER), WS_VISIBLE | WS_CHILD, CRect(p, CSize(label_w, m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbThreadNumber.Create(dwStyle | CBS_DROPDOWNLIST | WS_VSCROLL, CRect(p + CPoint(label_w, -4), CSize(combo_w, 200)), this, IDC_PP_THREAD_NUMBER);
	m_cbThreadNumber.AddString (ResStr (IDS_VDF_AUTO));
	CString ThreadNumberStr;
	for (int i = 1; i <= 16; i++) {
		ThreadNumberStr.Format(_T("%d"), i);
		m_cbThreadNumber.AddString(ThreadNumberStr);
	}
	p.y += h25;

	// H264 deblocking mode
	m_txtDiscardMode.Create(ResStr(IDS_VDF_SKIPDEBLOCK), WS_VISIBLE | WS_CHILD, CRect(p, CSize(label_w, m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbDiscardMode.Create(dwStyle | CBS_DROPDOWNLIST | WS_VSCROLL, CRect(p + CPoint(label_w, -4), CSize(combo_w, 200)), this, IDC_PP_DISCARD_MODE);
	m_cbDiscardMode.AddString (ResStr (IDS_VDF_DBLK_NONE));
	m_cbDiscardMode.AddString (ResStr (IDS_VDF_DBLK_DEFAULT));
	m_cbDiscardMode.AddString (ResStr (IDS_VDF_DBLK_NONREF));
	m_cbDiscardMode.AddString (ResStr (IDS_VDF_DBLK_BIDIR));
	m_cbDiscardMode.AddString (ResStr (IDS_VDF_DBLK_NONKFRM));
	m_cbDiscardMode.AddString (ResStr (IDS_VDF_DBLK_ALL));
	p.y += h25;

	// Deinterlacing
	m_txtDeinterlacing.Create(ResStr(IDS_VDF_DEINTERLACING), WS_VISIBLE | WS_CHILD, CRect(p, CSize(label_w, m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbDeinterlacing.Create(dwStyle | CBS_DROPDOWNLIST | WS_VSCROLL, CRect(p + CPoint(label_w, -4), CSize(combo_w, 200)), this, IDC_PP_DEINTERLACING);
	m_cbDeinterlacing.AddString (ResStr(IDS_VDF_AUTO));
	m_cbDeinterlacing.AddString (ResStr(IDS_VDF_DEINTER_TOP));
	m_cbDeinterlacing.AddString (ResStr(IDS_VDF_DEINTER_BOTTOM));
	m_cbDeinterlacing.AddString (ResStr(IDS_VDF_DEINTER_PROGRESSIVE));
	p.y += h25;

	// Read AR from stream
	m_cbARMode.Create(ResStr(IDS_VDF_AR_MODE), dwStyle | BS_AUTO3STATE | BS_LEFTTEXT, CRect(p, CSize(width_s, m_fontheight)), this, IDC_PP_AR);
	m_cbARMode.SetCheck(FALSE);


	////////// DXVA settings //////////
	p.y = 10 + IPP_SCALE(115) + 5;
	m_grpDXVA.Create(ResStr(IDS_VDF_DXVA_SETTING), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, CRect(p + CPoint(-5, 0), CSize(width_s + 10, IPP_SCALE(65))), this, (UINT)IDC_STATIC);
	p.y += h20;

	// DXVA Compatibility check
	m_txtDXVACompatibilityCheck.Create(ResStr(IDS_VDF_DXVACOMPATIBILITY), WS_VISIBLE | WS_CHILD, CRect(p, CSize(label_w, m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbDXVACompatibilityCheck.Create(dwStyle | CBS_DROPDOWNLIST | WS_VSCROLL, CRect(p + CPoint(label_w, -4), CSize(combo_w, 200)), this, IDC_PP_DXVA_CHECK);
	m_cbDXVACompatibilityCheck.AddString (ResStr(IDS_VDF_DXVA_FULLCHECK));
	m_cbDXVACompatibilityCheck.AddString (ResStr(IDS_VDF_DXVA_SKIP_LEVELCHECK));
	m_cbDXVACompatibilityCheck.AddString (ResStr(IDS_VDF_DXVA_SKIP_REFCHECK));
	m_cbDXVACompatibilityCheck.AddString (ResStr(IDS_VDF_DXVA_SKIP_ALLCHECK));
	p.y += h25;

	// Set DXVA for SD (H.264)
	m_cbDXVA_SD.Create(ResStr(IDS_VDF_DXVA_SD), dwStyle | BS_AUTOCHECKBOX | BS_LEFTTEXT, CRect(p, CSize(width_s, m_fontheight)), this, IDC_PP_DXVA_SD);
	m_cbDXVA_SD.SetCheck (FALSE);
	p.y += h25;

	////////// Status //////////
	p.y = 10 + IPP_SCALE(115) + 5 + IPP_SCALE(65) + 5;
	int w1 = IPP_SCALE(122);
	int w2 = width_s - w1;
	m_grpStatus.Create(ResStr(IDS_VDF_STATUS), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, CRect(p + CPoint(-5, 0), CSize(width_s + 10, IPP_SCALE(85))), this, (UINT)IDC_STATIC);
	p.y += h20;
	m_txtInputFormat.Create(ResStr(IDS_VDF_STATUS_INPUT), WS_VISIBLE | WS_CHILD, CRect(p, CSize(w1, m_fontheight)), this, (UINT)IDC_STATIC);
	m_edtInputFormat.Create(WS_CHILD | WS_VISIBLE | ES_READONLY, CRect(p + CPoint(w1, 0), CSize(w2, m_fontheight)), this, 0);
	p.y += h16;
	m_txtFrameSize.Create(ResStr(IDS_VDF_STATUS_FRAMESIZE), WS_VISIBLE | WS_CHILD, CRect(p, CSize(w1, m_fontheight)), this, (UINT)IDC_STATIC);
	m_edtFrameSize.Create(WS_CHILD | WS_VISIBLE | ES_READONLY, CRect(p + CPoint(w1, 0), CSize(w2, m_fontheight)), this, 0);
	p.y += h16;
	m_txtOutputFormat.Create(ResStr(IDS_VDF_STATUS_OUTPUT), WS_VISIBLE | WS_CHILD, CRect(p, CSize(w1, m_fontheight)), this, (UINT)IDC_STATIC);
	m_edtOutputFormat.Create(WS_CHILD | WS_VISIBLE | ES_READONLY, CRect(p + CPoint(w1, 0), CSize(w2, m_fontheight)), this, 0);
	p.y += h16;
	m_txtGraphicsAdapter.Create(ResStr(IDS_VDF_STATUS_ADAPTER), WS_VISIBLE | WS_CHILD, CRect(p, CSize(w1, m_fontheight)), this, (UINT)IDC_STATIC);
	m_edtGraphicsAdapter.Create(WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|ES_READONLY, CRect(p + CPoint(w1, 0), CSize(w2, m_fontheight)), this, 0);

	////////// Format conversion //////////
	p = CPoint(10 + width_s + 15, 10);
	combo_w = IPP_SCALE(85);
	label_w = IPP_SCALE(185);
	width_s = label_w + combo_w;
	m_grpFmtConv.Create(ResStr(IDS_VDF_COLOR_FMT_CONVERSION), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, CRect(p + CPoint(-5, 0), CSize(width_s + 10, IPP_SCALE(220))), this, (UINT)IDC_STATIC);
	p.y += h20;

	// Software output formats
	m_txtSwOutputFormats.Create(ResStr(IDS_VDF_COLOR_OUTPUT_FORMATS), WS_VISIBLE|WS_CHILD, CRect(p, CSize(width_s, m_fontheight)), this, (UINT)IDC_STATIC);
	p.y += h20;
	m_txt8bit.Create(_T("8-bit"), WS_VISIBLE|WS_CHILD, CRect(p + CSize(IPP_SCALE(60), 0), CSize(IPP_SCALE(45), m_fontheight)), this, (UINT)IDC_STATIC);
	m_txt10bit.Create(_T("10-bit"), WS_VISIBLE|WS_CHILD, CRect(p + CSize(IPP_SCALE(170), 0), CSize(IPP_SCALE(45), m_fontheight)), this, (UINT)IDC_STATIC);
	m_txt16bit.Create(_T("16-bit"), WS_VISIBLE|WS_CHILD, CRect(p + CSize(IPP_SCALE(225), 0), CSize(IPP_SCALE(45), m_fontheight)), this, (UINT)IDC_STATIC);
	p.y += h20;
	m_txt420.Create(_T("4:2:0 YUV:"), WS_VISIBLE|WS_CHILD, CRect(p, CSize(IPP_SCALE(60), m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbFormat[PixFmt_NV12].Create(_T("NV12"), dwStyle | BS_AUTOCHECKBOX, CRect(p + CSize(IPP_SCALE(60), 0), CSize(IPP_SCALE(47), m_fontheight)), this, IDC_PP_SW_NV12);
	m_cbFormat[PixFmt_YV12].Create(_T("YV12"), dwStyle | BS_AUTOCHECKBOX, CRect(p + CSize(IPP_SCALE(115), 0), CSize(IPP_SCALE(47), m_fontheight)), this, IDC_PP_SW_YV12);
	m_cbFormat[PixFmt_P010].Create(_T("P010"), dwStyle | BS_AUTOCHECKBOX, CRect(p + CSize(IPP_SCALE(170), 0), CSize(IPP_SCALE(45), m_fontheight)), this, IDC_PP_SW_P010);
	m_cbFormat[PixFmt_P016].Create(_T("P016"), dwStyle | BS_AUTOCHECKBOX, CRect(p + CSize(IPP_SCALE(225), 0), CSize(IPP_SCALE(45), m_fontheight)), this, IDC_PP_SW_P016);
	p.y += h20;
	m_txt422.Create(_T("4:2:2 YUV:"), WS_VISIBLE|WS_CHILD, CRect(p, CSize(IPP_SCALE(60), m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbFormat[PixFmt_YUY2].Create(_T("YUY2"), dwStyle | BS_3STATE, CRect(p + CSize(IPP_SCALE(60), 0), CSize(IPP_SCALE(50), m_fontheight)), this, IDC_PP_SW_YUY2);
	m_cbFormat[PixFmt_YV16].Create(_T("YV16"), dwStyle | BS_AUTOCHECKBOX, CRect(p + CSize(IPP_SCALE(115), 0), CSize(IPP_SCALE(47), m_fontheight)), this, IDC_PP_SW_YV16);
	m_cbFormat[PixFmt_P210].Create(_T("P210"), dwStyle | BS_AUTOCHECKBOX, CRect(p + CSize(IPP_SCALE(170), 0), CSize(IPP_SCALE(45), m_fontheight)), this, IDC_PP_SW_P210);
	m_cbFormat[PixFmt_P216].Create(_T("P216"), dwStyle | BS_AUTOCHECKBOX, CRect(p + CSize(IPP_SCALE(225), 0), CSize(IPP_SCALE(45), m_fontheight)), this, IDC_PP_SW_P216);
	p.y += h20;
	m_txt444.Create(_T("4:4:4 YUV:"), WS_VISIBLE|WS_CHILD, CRect(p, CSize(IPP_SCALE(60), m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbFormat[PixFmt_AYUV].Create(_T("AYUV"), dwStyle | BS_AUTOCHECKBOX, CRect(p + CSize(IPP_SCALE(60), 0), CSize(IPP_SCALE(51), m_fontheight)), this, IDC_PP_SW_AYUV);
	m_cbFormat[PixFmt_YV24].Create(_T("YV24"), dwStyle | BS_AUTOCHECKBOX, CRect(p + CSize(IPP_SCALE(115), 0), CSize(IPP_SCALE(47), m_fontheight)), this, IDC_PP_SW_YV24);
	m_cbFormat[PixFmt_Y410].Create(_T("Y410"), dwStyle | BS_AUTOCHECKBOX, CRect(p + CSize(IPP_SCALE(170), 0), CSize(IPP_SCALE(45), m_fontheight)), this, IDC_PP_SW_Y410);
	m_cbFormat[PixFmt_Y416].Create(_T("Y416"), dwStyle | BS_AUTOCHECKBOX, CRect(p + CSize(IPP_SCALE(225), 0), CSize(IPP_SCALE(45), m_fontheight)), this, IDC_PP_SW_Y416);
	p.y += h20;
	m_txtRGB.Create(_T("RGB:"), WS_VISIBLE|WS_CHILD, CRect(p, CSize(IPP_SCALE(60), m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbFormat[PixFmt_RGB32].Create(_T("RGB32"), dwStyle | BS_AUTOCHECKBOX, CRect(p + CSize(IPP_SCALE(60), 0), CSize(IPP_SCALE(56), m_fontheight)), this, IDC_PP_SW_RGB32);
	p.y += h25;

	// Preset
	m_txtSwPreset.Create(ResStr(IDS_VDF_COLOR_PRESET), WS_VISIBLE|WS_CHILD, CRect(p, CSize(label_w, m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbSwPreset.Create (dwStyle|CBS_DROPDOWNLIST|WS_VSCROLL, CRect(p + CSize(label_w, -4), CSize(combo_w, 200)), this, IDC_PP_SWPRESET);
	m_cbSwPreset.AddString(_T("Fastest"));
	m_cbSwPreset.AddString(_T("Fast"));
	m_cbSwPreset.AddString(_T("Normal"));
	m_cbSwPreset.AddString(_T("Full"));
	p.y += h25;

	// Standard (ITU-R Recommendation)
	m_txtSwStandard.Create(ResStr(IDS_VDF_COLOR_STANDARD), WS_VISIBLE|WS_CHILD, CRect(p, CSize(label_w, m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbSwStandard.Create(dwStyle|CBS_DROPDOWNLIST|WS_VSCROLL, CRect(p + CSize(label_w, -4), CSize(combo_w, 200)), this, IDC_PP_SWSTANDARD);
	m_cbSwStandard.AddString(_T("SD (BT.601)"));
	m_cbSwStandard.AddString(_T("HD (BT.709)"));
	m_cbSwStandard.AddString(ResStr(IDS_VDF_AUTO));
	p.y += h25;

	// Output levels
	m_txtSwRGBLevels.Create(ResStr(IDS_VDF_COLOR_RGB_LEVELS), WS_VISIBLE|WS_CHILD, CRect(p, CSize(label_w, m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbSwRGBLevels.Create(dwStyle|CBS_DROPDOWNLIST|WS_VSCROLL, CRect(p + CSize(label_w, -4), CSize(combo_w, 200)), this, IDC_PP_SWRGBLEVELS);
	m_cbSwRGBLevels.AddString(_T("PC (0-255)"));
	m_cbSwRGBLevels.AddString(_T("TV (16-235)"));

	p.y = 10 + IPP_SCALE(115) + 5 + IPP_SCALE(65) + 5 + IPP_SCALE(85) - m_fontheight;
	int btn_w = IPP_SCALE(75);
	m_btnReset.Create(ResStr(IDS_FILTER_RESET_SETTINGS), dwStyle|BS_MULTILINE, CRect(p + CPoint(0, - (m_fontheight + 6)), CSize(btn_w, m_fontheight*2 + 6)), this, IDC_PP_RESET);
	m_txtMPCVersion.Create(_T(""), WS_VISIBLE|WS_CHILD|ES_RIGHT, CRect(p + CPoint(btn_w, - 3), CSize(width_s - btn_w, m_fontheight)), this, (UINT)IDC_STATIC);

	for (CWnd* pWnd = GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetNextWindow()) {
		pWnd->SetFont(&m_font, FALSE);
	}

	CorrectComboListWidth(m_cbDXVACompatibilityCheck);
	CorrectComboListWidth(m_cbDiscardMode);

	if (m_pMDF) {
		m_cbThreadNumber.SetCurSel		(m_pMDF->GetThreadNumber());
		m_cbDiscardMode.SetCurSel		(FindDiscardIndex (m_pMDF->GetDiscardMode()));
		m_cbDeinterlacing.SetCurSel		((int)m_pMDF->GetDeinterlacing());

		m_cbARMode.SetCheck(m_pMDF->GetARMode());

		m_cbDXVACompatibilityCheck.SetCurSel(m_pMDF->GetDXVACheckCompatibility());
		m_cbDXVA_SD.SetCheck(m_pMDF->GetDXVA_SD());

		// === New swscaler options
		for (int i = 0; i < PixFmt_count; i++) {
			if (i == PixFmt_YUY2) {
				m_cbFormat[PixFmt_YUY2].SetCheck(m_pMDF->GetSwPixelFormat(PixFmt_YUY2) ? BST_CHECKED : BST_INDETERMINATE);
			} else {
				m_cbFormat[i].SetCheck(m_pMDF->GetSwPixelFormat((MPCPixelFormat)i) ? BST_CHECKED : BST_UNCHECKED);
			}
		}

		m_cbSwPreset.SetCurSel(m_pMDF->GetSwPreset());
		m_cbSwStandard.SetCurSel(m_pMDF->GetSwStandard());
		m_cbSwRGBLevels.SetCurSel(m_pMDF->GetSwRGBLevels());

		if (m_cbFormat[PixFmt_RGB32].GetCheck() == BST_UNCHECKED) {
			m_cbSwRGBLevels.EnableWindow(FALSE);
		}

		m_txtMPCVersion.SetWindowText(m_pMDF->GetInformation(INFO_MPCVersion));

		UpdateStatusInfo();
	}

	SetClassLongPtr(m_hWnd, GCLP_HCURSOR, (long) AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	SetClassLongPtr(GetDlgItem(IDC_PP_THREAD_NUMBER)->m_hWnd, GCLP_HCURSOR, (long) AfxGetApp()->LoadStandardCursor(IDC_HAND));

	EnableToolTips(TRUE);

	SetDirty(false);

	return true;
}

void CMPCVideoDecSettingsWnd::OnDeactivate()
{
	m_edtInputFormat.SetSel(-1);
	m_edtFrameSize.SetSel(-1);
	m_edtOutputFormat.SetSel(-1);
	m_edtGraphicsAdapter.SetSel(-1);
}

bool CMPCVideoDecSettingsWnd::OnApply()
{
	OnDeactivate();

	if (m_pMDF) {
		m_pMDF->SetThreadNumber		(m_cbThreadNumber.GetCurSel());
		m_pMDF->SetDiscardMode		(g_AVDiscard[m_cbDiscardMode.GetCurSel()]);
		m_pMDF->SetDeinterlacing	((MPC_DEINTERLACING_FLAGS)m_cbDeinterlacing.GetCurSel());

		m_pMDF->SetARMode(m_cbARMode.GetCheck());

		m_pMDF->SetDXVACheckCompatibility(m_cbDXVACompatibilityCheck.GetCurSel());

		m_pMDF->SetDXVA_SD(m_cbDXVA_SD.GetCheck());

		// === New swscaler options
		int refresh = 0; // no refresh

		if (m_cbSwPreset.GetCurSel() != m_pMDF->GetSwPreset()
				|| m_cbSwStandard.GetCurSel() != m_pMDF->GetSwStandard()
				|| m_cbSwRGBLevels.GetCurSel() != m_pMDF->GetSwRGBLevels()) {
			refresh = 1; // soft refresh - signal new swscaler colorspace details
		}

		for (int i = 0; i < PixFmt_count; i++) {
			if ((m_cbFormat[i].GetCheck() == BST_CHECKED) != m_pMDF->GetSwPixelFormat((MPCPixelFormat)i)) {
				refresh = 2;
				break;
			}
		}

		if (refresh >= 2) {
			for (int i = 0; i < PixFmt_count; i++) {
				m_pMDF->SetSwPixelFormat((MPCPixelFormat)i, m_cbFormat[i].GetCheck() == BST_CHECKED);
			}
		}

		if (refresh >= 1) {
			m_pMDF->SetSwPreset(m_cbSwPreset.GetCurSel());
			m_pMDF->SetSwStandard(m_cbSwStandard.GetCurSel());
			m_pMDF->SetSwRGBLevels(m_cbSwRGBLevels.GetCurSel());
		}

		m_pMDF->SetSwRefresh(refresh);

		UpdateStatusInfo();

		m_pMDF->SaveSettings();
	}

	return true;
}


BEGIN_MESSAGE_MAP(CMPCVideoDecSettingsWnd, CInternalPropertyPageWnd)
	ON_BN_CLICKED(IDC_PP_SW_YUY2, OnBnClickedYUY2)
	ON_BN_CLICKED(IDC_PP_SW_RGB32, OnBnClickedRGB32)
	ON_BN_CLICKED(IDC_PP_RESET, OnBnClickedReset)
	ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnToolTipNotify)
END_MESSAGE_MAP()

void CMPCVideoDecSettingsWnd::OnBnClickedYUY2()
{
	if (m_cbFormat[PixFmt_YUY2].GetCheck() == BST_CHECKED) {
		m_cbFormat[PixFmt_YUY2].SetCheck(BST_INDETERMINATE);
	} else {
		m_cbFormat[PixFmt_YUY2].SetCheck(BST_CHECKED);
	}
}

void CMPCVideoDecSettingsWnd::OnBnClickedRGB32()
{
	if (m_cbFormat[PixFmt_RGB32].GetCheck() == BST_UNCHECKED) {
		m_cbSwRGBLevels.EnableWindow(FALSE);
	} else {
		m_cbSwRGBLevels.EnableWindow(TRUE);
	}
}

void CMPCVideoDecSettingsWnd::OnBnClickedReset()
{
	m_cbThreadNumber.SetCurSel(0);
	m_cbDiscardMode.SetCurSel(FindDiscardIndex(AVDISCARD_DEFAULT));
	m_cbDeinterlacing.SetCurSel(AUTO);
	m_cbARMode.SetCheck(BST_INDETERMINATE);

	m_cbDXVACompatibilityCheck.SetCurSel(1);
	m_cbDXVA_SD.SetCheck(BST_UNCHECKED);

	for (int i = 0; i < PixFmt_count; i++) {
		if (i == PixFmt_AYUV) {
			m_cbFormat[i].SetCheck(BST_UNCHECKED);
		} else {
			m_cbFormat[i].SetCheck(BST_CHECKED);
		}
	}
	m_cbSwPreset.SetCurSel(2);
	m_cbSwStandard.SetCurSel(2);
	m_cbSwRGBLevels.SetCurSel(0);
	m_cbSwRGBLevels.EnableWindow(TRUE);
}

BOOL CMPCVideoDecSettingsWnd::OnToolTipNotify(UINT id, NMHDR * pNMHDR, LRESULT * pResult)
{
	TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
	static CString strTipText;
	UINT_PTR nID = pNMHDR->idFrom;

	if (pNMHDR->code == TTN_NEEDTEXT && (pTTT->uFlags & TTF_IDISHWND)) {

		CToolTipCtrl* pToolTip = AfxGetModuleThreadState()->m_pToolTip;
		if (pToolTip) {
			pToolTip->SetMaxTipWidth(SHRT_MAX);
		}

		nID = ::GetDlgCtrlID((HWND)nID);
		switch (nID) {
		case IDC_PP_AR:
			strTipText = ResStr(IDS_VDF_TT_AR);
			break;
		case IDC_PP_SWPRESET:
			strTipText = ResStr(IDS_VDF_TT_PRESET);
			break;
		case IDC_PP_SWSTANDARD:
			strTipText = ResStr(IDS_VDF_TT_STANDARD);
			break;
		case IDC_PP_SWRGBLEVELS:
			strTipText = ResStr(IDS_VDF_TT_RGB_LEVELS);
			break;
		default:
			return FALSE;
		}

		pTTT->lpszText = strTipText.GetBuffer();

		*pResult = 0;
		return TRUE;
	}

	return FALSE;
}

// ====== Codec filter property page (for standalone filter only)
#ifdef REGISTER_FILTER
CMPCVideoDecCodecWnd::CMPCVideoDecCodecWnd()
{
}

bool CMPCVideoDecCodecWnd::OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks)
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

	return true;
}

void CMPCVideoDecCodecWnd::OnDisconnect()
{
	m_pMDF.Release();
}

static const struct {
	ULONGLONG	CodecId;
	LPCTSTR		CodeName;
} mpc_codecs[] = {
	{CODEC_H264_DXVA,	_T("H.264/AVC (DXVA)")},
	{CODEC_HEVC_DXVA,	_T("HEVC (DXVA)")},
	{CODEC_MPEG2_DXVA,	_T("MPEG-2 (DXVA)")},
	{CODEC_VC1_DXVA,	_T("VC-1 (DXVA)")},
	{CODEC_WMV3_DXVA,	_T("WMV3 (DXVA)")},
	{CODEC_AMVV,		_T("AMV video")},
	{CODEC_PRORES,		_T("Apple ProRes")},
	{CODEC_DNXHD,		_T("Avid DNxHD")},
	{CODEC_BINKV,		_T("Bink video")},
	{CODEC_CLLC,		_T("Canopus Lossless")},
	{CODEC_CINEPAK,		_T("Cinepak")},
	{CODEC_DIRAC,		_T("Dirac")},
	{CODEC_DIVX,		_T("DivX")},
	{CODEC_DV,			_T("DV video")},
	{CODEC_FLASH,		_T("FLV1/4")},
	{CODEC_H263,		_T("H.263")},
	{CODEC_H264,		_T("H.264/AVC (FFmpeg)")},
	{CODEC_HEVC,		_T("HEVC (experimental)")},
	{CODEC_INDEO,		_T("Indeo 3/4/5")},
	{CODEC_LOSSLESS,	_T("Lossless video (huffyuv, Lagarith, FFV1)")},
	{CODEC_MJPEG,		_T("MJPEG")},
	{CODEC_MPEG1,		_T("MPEG-1 (FFmpeg)")},
	{CODEC_MPEG2,		_T("MPEG-2 (FFmpeg)")},
	{CODEC_MSMPEG4,		_T("MS-MPEG4")},
	{CODEC_PNG,			_T("PNG")},
	{CODEC_QT,			_T("QuickTime video (8BPS, QTRle, rpza)")},
	{CODEC_SCREC,		_T("Screen Recorder (CSCD, MS, TSCC, VMnc)")},
	{CODEC_SVQ3,		_T("SVQ1/3")},
	{CODEC_THEORA,		_T("Theora")},
	{CODEC_UTVD,		_T("Ut video")},
	{CODEC_VC1,			_T("VC-1 (FFmpeg)")},
	{CODEC_VP356,		_T("VP3/5/6")},
	{CODEC_VP89,		_T("VP7/8/9")},
	{CODEC_WMV,			_T("WMV1/2/3")},
	{CODEC_XVID,		_T("Xvid/MPEG-4")},
	{CODEC_REALV,		_T("Real Video")},
	{CODEC_UNCOMPRESSED,_T("Uncompressed video (v210, V410, Y800, I420, ...)")},
};

bool CMPCVideoDecCodecWnd::OnActivate()
{
	DWORD dwStyle			= WS_VISIBLE|WS_CHILD|WS_BORDER;
	int nPos				= 0;
	ULONGLONG nActiveCodecs	= m_pMDF ? m_pMDF->GetActiveCodecs() : 0;

	m_grpSelectedCodec.Create(_T("Selected codecs"), WS_VISIBLE|WS_CHILD | BS_GROUPBOX, CRect (10,  10, 330, 280), this, (UINT)IDC_STATIC);

	m_lstCodecs.Create(dwStyle | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_TABSTOP, CRect (20,30, 320, 270), this, 0);

	for (size_t i = 0; i < _countof(mpc_codecs); i++) {
		m_lstCodecs.AddString(mpc_codecs[i].CodeName);
		m_lstCodecs.SetCheck(nPos++, (nActiveCodecs & mpc_codecs[i].CodecId) != 0);
	}

	for (CWnd* pWnd = GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetNextWindow()) {
		pWnd->SetFont(&m_font, FALSE);
	}

	return true;
}

void CMPCVideoDecCodecWnd::OnDeactivate()
{
}

bool CMPCVideoDecCodecWnd::OnApply()
{
	OnDeactivate();

	if (m_pMDF) {
		ULONGLONG nActiveCodecs	= 0;
		int nPos				= 0;

		for (size_t i = 0; i < _countof(mpc_codecs); i++) {
			if (m_lstCodecs.GetCheck(nPos++)) {
				nActiveCodecs |= mpc_codecs[i].CodecId;
			}
		}

		m_pMDF->SetActiveCodecs(nActiveCodecs);

		m_pMDF->SaveSettings();
	}

	return true;
}

BEGIN_MESSAGE_MAP(CMPCVideoDecCodecWnd, CInternalPropertyPageWnd)
END_MESSAGE_MAP()

#endif

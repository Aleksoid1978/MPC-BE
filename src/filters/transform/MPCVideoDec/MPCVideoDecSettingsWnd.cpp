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
#include "MPCVideoDecSettingsWnd.h"
#include "../../../DSUtil/MFCHelper.h"
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
	const int h16 = ScaleY(16);
	const int h20 = ScaleY(20);
	const int h25 = ScaleY(25);
	DWORD dwStyle = WS_VISIBLE | WS_CHILD | WS_TABSTOP;

	int label_w = ScaleX(225);
	int combo_w = ScaleX(110);
	int width_s = label_w + combo_w;

	////////// Video settings //////////
	CPoint p(10, 10);
	m_grpDecoder.Create(ResStr(IDS_VDF_SETTINGS), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, CRect(p + CPoint(-5, 0), CSize(width_s + 10, ScaleY(115))), this, (UINT)IDC_STATIC);
	p.y += h20;

	// Decoding threads
	m_txtThreadNumber.Create(ResStr(IDS_VDF_THREADNUMBER), WS_VISIBLE | WS_CHILD, CRect(p, CSize(label_w, m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbThreadNumber.Create(dwStyle | CBS_DROPDOWNLIST | WS_VSCROLL, CRect(p + CPoint(label_w, -4), CSize(combo_w, 200)), this, IDC_PP_THREAD_NUMBER);
	m_cbThreadNumber.AddString (ResStr (IDS_VDF_AUTO));
	CString ThreadNumberStr;
	for (int i = 1; i <= 16; i++) {
		ThreadNumberStr.Format(L"%d", i);
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
	p.y = 10 + ScaleY(115) + 5;
	m_grpDXVA.Create(ResStr(IDS_VDF_DXVA_SETTING), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, CRect(p + CPoint(-5, 0), CSize(width_s + 10, ScaleY(65))), this, (UINT)IDC_STATIC);
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
	p.y = 10 + ScaleY(115) + 5 + ScaleY(65) + 5;
	int w1 = ScaleX(122);
	int w2 = width_s - w1;
	m_grpStatus.Create(ResStr(IDS_VDF_STATUS), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, CRect(p + CPoint(-5, 0), CSize(width_s + 10, ScaleY(85))), this, (UINT)IDC_STATIC);
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
	combo_w = ScaleX(85);
	label_w = ScaleX(185);
	width_s = label_w + combo_w;
	m_grpFmtConv.Create(ResStr(IDS_VDF_COLOR_FMT_CONVERSION), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, CRect(p + CPoint(-5, 0), CSize(width_s + 10, ScaleY(170))), this, (UINT)IDC_STATIC);
	p.y += h20;

	// Software output formats
	m_txtSwOutputFormats.Create(ResStr(IDS_VDF_COLOR_OUTPUT_FORMATS), WS_VISIBLE|WS_CHILD, CRect(p, CSize(width_s, m_fontheight)), this, (UINT)IDC_STATIC);
	p.y += h20;
	m_txt8bit.Create(L"8-bit", WS_VISIBLE|WS_CHILD, CRect(p + CSize(ScaleX(60), 0), CSize(ScaleX(45), m_fontheight)), this, (UINT)IDC_STATIC);
	m_txt10bit.Create(L"10-bit", WS_VISIBLE|WS_CHILD, CRect(p + CSize(ScaleX(170), 0), CSize(ScaleX(45), m_fontheight)), this, (UINT)IDC_STATIC);
	m_txt16bit.Create(L"16-bit", WS_VISIBLE|WS_CHILD, CRect(p + CSize(ScaleX(225), 0), CSize(ScaleX(45), m_fontheight)), this, (UINT)IDC_STATIC);
	p.y += h20;
	m_txt420.Create(L"4:2:0 YUV:", WS_VISIBLE|WS_CHILD, CRect(p, CSize(ScaleX(60), m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbFormat[PixFmt_NV12].Create(L"NV12", dwStyle | BS_AUTOCHECKBOX, CRect(p + CSize(ScaleX(60), 0), CSize(ScaleX(47), m_fontheight)), this, IDC_PP_SW_NV12);
	m_cbFormat[PixFmt_YV12].Create(L"YV12", dwStyle | BS_AUTOCHECKBOX, CRect(p + CSize(ScaleX(115), 0), CSize(ScaleX(47), m_fontheight)), this, IDC_PP_SW_YV12);
	m_cbFormat[PixFmt_P010].Create(L"P010", dwStyle | BS_AUTOCHECKBOX, CRect(p + CSize(ScaleX(170), 0), CSize(ScaleX(45), m_fontheight)), this, IDC_PP_SW_P010);
	m_cbFormat[PixFmt_P016].Create(L"P016", dwStyle | BS_AUTOCHECKBOX, CRect(p + CSize(ScaleX(225), 0), CSize(ScaleX(45), m_fontheight)), this, IDC_PP_SW_P016);
	p.y += h20;
	m_txt422.Create(L"4:2:2 YUV:", WS_VISIBLE|WS_CHILD, CRect(p, CSize(ScaleX(60), m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbFormat[PixFmt_YUY2].Create(L"YUY2", dwStyle | BS_3STATE, CRect(p + CSize(ScaleX(60), 0), CSize(ScaleX(50), m_fontheight)), this, IDC_PP_SW_YUY2);
	m_cbFormat[PixFmt_YV16].Create(L"YV16", dwStyle | BS_AUTOCHECKBOX, CRect(p + CSize(ScaleX(115), 0), CSize(ScaleX(47), m_fontheight)), this, IDC_PP_SW_YV16);
	m_cbFormat[PixFmt_P210].Create(L"P210", dwStyle | BS_AUTOCHECKBOX, CRect(p + CSize(ScaleX(170), 0), CSize(ScaleX(45), m_fontheight)), this, IDC_PP_SW_P210);
	m_cbFormat[PixFmt_P216].Create(L"P216", dwStyle | BS_AUTOCHECKBOX, CRect(p + CSize(ScaleX(225), 0), CSize(ScaleX(45), m_fontheight)), this, IDC_PP_SW_P216);
	p.y += h20;
	m_txt444.Create(L"4:4:4 YUV:", WS_VISIBLE|WS_CHILD, CRect(p, CSize(ScaleX(60), m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbFormat[PixFmt_AYUV].Create(L"AYUV", dwStyle | BS_AUTOCHECKBOX, CRect(p + CSize(ScaleX(60), 0), CSize(ScaleX(51), m_fontheight)), this, IDC_PP_SW_AYUV);
	m_cbFormat[PixFmt_YV24].Create(L"YV24", dwStyle | BS_AUTOCHECKBOX, CRect(p + CSize(ScaleX(115), 0), CSize(ScaleX(47), m_fontheight)), this, IDC_PP_SW_YV24);
	m_cbFormat[PixFmt_Y410].Create(L"Y410", dwStyle | BS_AUTOCHECKBOX, CRect(p + CSize(ScaleX(170), 0), CSize(ScaleX(45), m_fontheight)), this, IDC_PP_SW_Y410);
	m_cbFormat[PixFmt_Y416].Create(L"Y416", dwStyle | BS_AUTOCHECKBOX, CRect(p + CSize(ScaleX(225), 0), CSize(ScaleX(45), m_fontheight)), this, IDC_PP_SW_Y416);
	p.y += h20;
	m_txtRGB.Create(L"RGB:", WS_VISIBLE|WS_CHILD, CRect(p, CSize(ScaleX(60), m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbFormat[PixFmt_RGB32].Create(L"RGB32", dwStyle | BS_AUTOCHECKBOX, CRect(p + CSize(ScaleX(60), 0), CSize(ScaleX(56), m_fontheight)), this, IDC_PP_SW_RGB32);
	p.y += h25;

	// Output levels
	m_txtSwRGBLevels.Create(ResStr(IDS_VDF_COLOR_RGB_LEVELS), WS_VISIBLE|WS_CHILD, CRect(p, CSize(label_w, m_fontheight)), this, (UINT)IDC_STATIC);
	m_cbSwRGBLevels.Create(dwStyle|CBS_DROPDOWNLIST|WS_VSCROLL, CRect(p + CSize(label_w, -4), CSize(combo_w, 200)), this, IDC_PP_SWRGBLEVELS);
	m_cbSwRGBLevels.AddString(L"PC (0-255)");
	m_cbSwRGBLevels.AddString(L"TV (16-235)");

	p.y = 10 + ScaleY(115) + 5 + ScaleY(65) + 5 + ScaleY(85) - m_fontheight;
	int btn_w = ScaleX(75);
	m_btnReset.Create(ResStr(IDS_FILTER_RESET_SETTINGS), dwStyle|BS_MULTILINE, CRect(p + CPoint(0, - (m_fontheight + 6)), CSize(btn_w, m_fontheight*2 + 6)), this, IDC_PP_RESET);
	m_txtMPCVersion.Create(L"", WS_VISIBLE|WS_CHILD|ES_RIGHT, CRect(p + CPoint(btn_w, - 3), CSize(width_s - btn_w, m_fontheight)), this, (UINT)IDC_STATIC);

	for (CWnd* pWnd = GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetNextWindow()) {
		pWnd->SetFont(&m_font, FALSE);
	}

	CorrectComboListWidth(m_cbDeinterlacing);
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

		m_cbSwRGBLevels.SetCurSel(m_pMDF->GetSwRGBLevels());

		if (m_cbFormat[PixFmt_RGB32].GetCheck() == BST_UNCHECKED) {
			m_cbSwRGBLevels.EnableWindow(FALSE);
		}

		m_txtMPCVersion.SetWindowText(m_pMDF->GetInformation(INFO_MPCVersion));

		UpdateStatusInfo();
	}

	SetCursor(m_hWnd, IDC_ARROW);
	SetCursor(m_hWnd, IDC_PP_THREAD_NUMBER, IDC_HAND);

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

		if (m_cbSwRGBLevels.GetCurSel() != m_pMDF->GetSwRGBLevels()) {
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
	{CODEC_H264_DXVA,	L"H.264/AVC (DXVA)"},
	{CODEC_HEVC_DXVA,	L"HEVC (DXVA)"},
	{CODEC_MPEG2_DXVA,	L"MPEG-2 (DXVA)"},
	{CODEC_VC1_DXVA,	L"VC-1 (DXVA)"},
	{CODEC_WMV3_DXVA,	L"WMV3 (DXVA)"},
	{CODEC_VP9_DXVA,	L"VP9 (DXVA)"},
	{CODEC_AMVV,		L"AMV video"},
	{CODEC_PRORES,		L"Apple ProRes"},
	{CODEC_DNXHD,		L"Avid DNxHD"},
	{CODEC_BINKV,		L"Bink video"},
	{CODEC_CANOPUS,		L"Canopus Lossless/HQ/HQX"},
	{CODEC_CINEFORM,	L"CineForm"},
	{CODEC_CINEPAK,		L"Cinepak"},
	{CODEC_DIRAC,		L"Dirac"},
	{CODEC_DIVX,		L"DivX"},
	{CODEC_DV,			L"DV video"},
	{CODEC_FLASH,		L"FLV1/4"},
	{CODEC_H263,		L"H.263"},
	{CODEC_H264,		L"H.264/AVC (FFmpeg)"},
	{CODEC_H264_MVC,	L"H264 (MVC 3D)"},
	{CODEC_HEVC,		L"HEVC (experimental)"},
	{CODEC_INDEO,		L"Indeo 3/4/5"},
	{CODEC_LOSSLESS,	L"Lossless video (huffyuv, Lagarith, FFV1, MagicYUV)"},
	{CODEC_MJPEG,		L"MJPEG"},
	{CODEC_MPEG1,		L"MPEG-1 (FFmpeg)"},
	{CODEC_MPEG2,		L"MPEG-2 (FFmpeg)"},
	{CODEC_MSMPEG4,		L"MS-MPEG4"},
	{CODEC_PNG,			L"PNG"},
	{CODEC_QT,			L"QuickTime video (8BPS, QTRle, rpza)"},
	{CODEC_SCREC,		L"Screen Recorder (CSCD, MS, TSCC, VMnc)"},
	{CODEC_SVQ3,		L"SVQ1/3"},
	{CODEC_THEORA,		L"Theora"},
	{CODEC_UTVD,		L"Ut video"},
	{CODEC_VC1,			L"VC-1 (FFmpeg)"},
	{CODEC_VP356,		L"VP3/5/6"},
	{CODEC_VP89,		L"VP7/8/9"},
	{CODEC_WMV,			L"WMV1/2/3"},
	{CODEC_XVID,		L"Xvid/MPEG-4"},
	{CODEC_REALV,		L"Real Video"},
	{CODEC_UNCOMPRESSED,L"Uncompressed video (v210, V410, Y800, I420, ...)"},
};

bool CMPCVideoDecCodecWnd::OnActivate()
{
	DWORD dwStyle			= WS_VISIBLE|WS_CHILD|WS_BORDER;
	int nPos				= 0;
	ULONGLONG nActiveCodecs	= m_pMDF ? m_pMDF->GetActiveCodecs() : 0;

	m_grpSelectedCodec.Create(L"Selected codecs", WS_VISIBLE|WS_CHILD | BS_GROUPBOX, CRect (10,  10, 330, 280), this, (UINT)IDC_STATIC);

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

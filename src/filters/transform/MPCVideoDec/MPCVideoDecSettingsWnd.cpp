/*
 * (C) 2006-2021 see Authors.txt
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
	KillTimer(m_nTimerID);
	m_pMDF.Release();
}

void CMPCVideoDecSettingsWnd::UpdateStatusInfo()
{
	m_edtInputFormat.SetWindowTextW(m_pMDF->GetInformation(INFO_InputFormat));
	m_edtFrameSize.SetWindowTextW(m_pMDF->GetInformation(INFO_FrameSize));
	m_edtOutputFormat.SetWindowTextW(m_pMDF->GetInformation(INFO_OutputFormat));
	m_edtGraphicsAdapter.SetWindowTextW(m_pMDF->GetInformation(INFO_GraphicsAdapter));
}

bool CMPCVideoDecSettingsWnd::OnActivate()
{
	DWORD dwStyle = WS_VISIBLE | WS_CHILD | WS_TABSTOP;
	CString str;

	CRect rect;
	long label_w = 224;
	long control_w = 108;
	long row_w = label_w + control_w;
	long group_w = row_w + 8;

	long x0 = 4;
	long x1 = 8;
	long x2 = x1 + label_w;
	long y = 8;


	////////// Video settings //////////
	CalcRect(rect, x0, y, group_w, 108);
	m_grpDecoder.Create(ResStr(IDS_VDF_SETTINGS), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, rect, this, (UINT)IDC_STATIC);
	y += 20;

	// Decoding threads
	CalcTextRect(rect, x1, y, label_w);
	m_txtThreadNumber.Create(ResStr(IDS_VDF_THREADNUMBER), WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	CalcRect(rect, x2, y, control_w, 200); rect.top -= 4;
	m_cbThreadNumber.Create(dwStyle | CBS_DROPDOWNLIST | WS_VSCROLL, rect, this, IDC_PP_THREAD_NUMBER);
	m_cbThreadNumber.AddString (ResStr (IDS_VDF_AUTO));
	for (int i = 1; i <= 16; i++) {
		str.Format(L"%d", i);
		m_cbThreadNumber.AddString(str);
	}
	y += 24;

	// Deinterlacing
	CalcTextRect(rect, x1, y, label_w);
	m_txtScanType.Create(ResStr(IDS_VDF_SCANTYPE), WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	CalcRect(rect, x2, y, control_w, 200); rect.top -= 4;
	m_cbScanType.Create(dwStyle | CBS_DROPDOWNLIST | WS_VSCROLL, rect, this, IDC_PP_DEINTERLACING);
	m_cbScanType.AddString(ResStr(IDS_VDF_AUTO));
	m_cbScanType.AddString(ResStr(IDS_VDF_SCANTYPE_TOP));
	m_cbScanType.AddString(ResStr(IDS_VDF_SCANTYPE_BOTTOM));
	m_cbScanType.AddString(ResStr(IDS_VDF_SCANTYPE_PROGRESSIVE));
	y += 24;

	// Read AR from stream
	CalcTextRect(rect, x1, y, row_w);
	m_chARMode.Create(ResStr(IDS_VDF_AR_MODE), dwStyle | BS_AUTO3STATE | BS_LEFTTEXT, rect, this, IDC_PP_AR);
	m_chARMode.SetCheck(FALSE);
	y += 20;

	// Skip B-frames
	CalcTextRect(rect, x1, y, row_w);
	m_chSkipBFrames.Create(ResStr(IDS_VDF_SKIPBFRAMES) + L"*", dwStyle | BS_AUTOCHECKBOX | BS_LEFTTEXT, rect, this, IDC_PP_SKIPBFRAMES);
	m_chSkipBFrames.SetCheck(FALSE);

	////////// Hardware acceleration //////////
	y = 120;
	CalcRect(rect, x0, y, group_w, 128);
	m_grpHwAcceleration.Create(ResStr(IDS_VDF_HW_ACCELERATION), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, rect, this, (UINT)IDC_STATIC);
	y += 20;

	CalcTextRect(rect, x1, y, 64);
	m_txtHWCodec.Create(ResStr(IDS_VDF_HW_CODECS), WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);

	int codec_w = 60;
	int codec_x1 = x1 + 64;
	int codec_x2 = codec_x1 + codec_w + 8;
	int codec_x3 = codec_x2 + codec_w + 8;
	int codec_x4 = codec_x3 + codec_w + 8;

	CalcTextRect(rect, codec_x1, y, codec_w);
	m_cbHWCodec[HWDec_H264].Create(L"H.264", dwStyle | BS_AUTOCHECKBOX, rect, this, IDC_PP_HW_H264);
	CalcTextRect(rect, codec_x2, y, codec_w);
	m_cbHWCodec[HWDec_HEVC].Create(L"HEVC", dwStyle | BS_AUTOCHECKBOX, rect, this, IDC_PP_HW_HEVC);
	CalcTextRect(rect, codec_x3, y, codec_w);
	m_cbHWCodec[HWDec_VP9].Create(L"VP9", dwStyle | BS_AUTOCHECKBOX, rect, this, IDC_PP_HW_VP9);
	CalcTextRect(rect, codec_x4, y, codec_w);
	m_cbHWCodec[HWDec_AV1].Create(L"AV1", dwStyle | BS_AUTOCHECKBOX, rect, this, IDC_PP_HW_AV1);
	y += 20;
	CalcTextRect(rect, codec_x1, y, codec_w);
	m_cbHWCodec[HWDec_MPEG2].Create(L"MPEG-2", dwStyle | BS_AUTOCHECKBOX, rect, this, IDC_PP_HW_MPEG2);
	CalcTextRect(rect, codec_x2, y, codec_w);
	m_cbHWCodec[HWDec_VC1].Create(L"VC-1", dwStyle | BS_AUTOCHECKBOX, rect, this, IDC_PP_HW_VC1);
	CalcTextRect(rect, codec_x3, y, codec_w);
	m_cbHWCodec[HWDec_WMV3].Create(L"WMV3", dwStyle | BS_AUTOCHECKBOX, rect, this, IDC_PP_HW_WMV3);
	y += 20;

	// Use D3D11 decoder
	CalcTextRect(rect, x1, y, row_w);
	m_chUseD3D11Decoder.Create(ResStr(IDS_VDF_USE_D3D11_DECODER), dwStyle | BS_AUTOCHECKBOX | BS_LEFTTEXT, rect, this, IDC_PP_D3D11_DEC);
	if (SysVersion::IsWin8orLater()) {
		m_chUseD3D11Decoder.SetCheck(BST_CHECKED);
	} else {
		m_chUseD3D11Decoder.EnableWindow(FALSE);
		m_chUseD3D11Decoder.SetCheck(BST_UNCHECKED);
	}
	y += 24;

	// DXVA Compatibility check
	CalcTextRect(rect, x1, y, label_w);
	m_txtDXVACompatibilityCheck.Create(ResStr(IDS_VDF_DXVACOMPATIBILITY), WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	CalcRect(rect, x2, y, control_w, 200); rect.top -= 4;
	m_cbDXVACompatibilityCheck.Create(dwStyle | CBS_DROPDOWNLIST | WS_VSCROLL, rect, this, IDC_PP_DXVA_CHECK);
	m_cbDXVACompatibilityCheck.AddString(ResStr(IDS_VDF_DXVA_FULLCHECK));
	m_cbDXVACompatibilityCheck.AddString(ResStr(IDS_VDF_DXVA_SKIP_LEVELCHECK));
	m_cbDXVACompatibilityCheck.AddString(ResStr(IDS_VDF_DXVA_SKIP_REFCHECK));
	m_cbDXVACompatibilityCheck.AddString(ResStr(IDS_VDF_DXVA_SKIP_ALLCHECK));
	y += 24;

	// Set DXVA for SD (H.264)
	CalcTextRect(rect, x1, y, row_w);
	m_chDXVA_SD.Create(ResStr(IDS_VDF_DXVA_SD), dwStyle | BS_AUTOCHECKBOX | BS_LEFTTEXT, rect, this, IDC_PP_DXVA_SD);
	m_chDXVA_SD.SetCheck(FALSE);

	////////// Status //////////
	label_w = 124;
	control_w = row_w - label_w;
	x2 = x1 + label_w;
	y = 252;
	CalcRect(rect, x0, y, group_w, 88);
	m_grpStatus.Create(ResStr(IDS_VDF_STATUS), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, rect, this, (UINT)IDC_STATIC);
	y += 20;

	CalcTextRect(rect, x1, y, label_w);
	m_txtInputFormat.Create(ResStr(IDS_VDF_STATUS_INPUT), WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	CalcTextRect(rect, x2, y, control_w);
	m_edtInputFormat.Create(WS_CHILD | WS_VISIBLE | ES_READONLY, rect, this, 0);
	y += 16;

	CalcTextRect(rect, x1, y, label_w);
	m_txtFrameSize.Create(ResStr(IDS_VDF_STATUS_FRAMESIZE), WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	CalcTextRect(rect, x2, y, control_w);
	m_edtFrameSize.Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY, rect, this, 0);
	y += 16;

	CalcTextRect(rect, x1, y, label_w);
	m_txtOutputFormat.Create(ResStr(IDS_VDF_STATUS_OUTPUT), WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	CalcTextRect(rect, x2, y, control_w);
	m_edtOutputFormat.Create(WS_CHILD | WS_VISIBLE | ES_READONLY, rect, this, 0);
	y += 16;

	CalcTextRect(rect, x1, y, label_w);
	m_txtGraphicsAdapter.Create(ResStr(IDS_VDF_STATUS_ADAPTER), WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	CalcTextRect(rect, x2, y, control_w);
	m_edtGraphicsAdapter.Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY, rect, this, 0);

	////////// Format conversion //////////
	x0 = x0 + group_w + 8;
	label_w = 60;
	control_w = 52;
	row_w = label_w + control_w * 4 + 4;
	group_w = row_w + 8;
	x1 = x0 + 4;
	x2 = x1 + label_w;
	y = 8;
	CalcRect(rect, x0, y, group_w, 172);
	m_grpFmtConv.Create(ResStr(IDS_VDF_COLOR_FMT_CONVERSION), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, rect, this, (UINT)IDC_STATIC);
	y += 20;

	// Software output formats
	CalcTextRect(rect, x1, y, row_w);
	m_txtSwOutputFormats.Create(ResStr(IDS_VDF_COLOR_OUTPUT_FORMATS), WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	y += 20;

	CalcTextRect(rect, x2, y, control_w);
	m_txt8bit.Create(L"8-bit", WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	CalcTextRect(rect, x2 + control_w * 2, y, control_w);
	m_txt10bit.Create(L"10-bit", WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	CalcTextRect(rect, x2 + control_w * 3, y, control_w);
	m_txt16bit.Create(L"16-bit", WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	y += 20;

	CalcTextRect(rect, x1, y, label_w);
	m_txt420.Create(L"4:2:0 YUV:", WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	CalcTextRect(rect, x2, y, control_w);
	m_cbFormat[PixFmt_NV12].Create(L"NV12", dwStyle | BS_AUTOCHECKBOX, rect, this, IDC_PP_SW_NV12);
	CalcTextRect(rect, x2 + control_w, y, control_w);
	m_cbFormat[PixFmt_YV12].Create(L"YV12", dwStyle | BS_AUTOCHECKBOX, rect, this, IDC_PP_SW_YV12);
	CalcTextRect(rect, x2 + control_w * 2, y, control_w);
	m_cbFormat[PixFmt_P010].Create(L"P010", dwStyle | BS_AUTOCHECKBOX, rect, this, IDC_PP_SW_P010);
	CalcTextRect(rect, x2 + control_w * 3, y, control_w);
	m_cbFormat[PixFmt_P016].Create(L"P016", dwStyle | BS_AUTOCHECKBOX, rect, this, IDC_PP_SW_P016);
	y += 20;

	CalcTextRect(rect, x1, y, label_w);
	m_txt422.Create(L"4:2:2 YUV:", WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	CalcTextRect(rect, x2, y, control_w);
	m_cbFormat[PixFmt_YUY2].Create(L"YUY2", dwStyle | BS_3STATE, rect, this, IDC_PP_SW_YUY2);
	CalcTextRect(rect, x2 + control_w, y, control_w);
	m_cbFormat[PixFmt_YV16].Create(L"YV16", dwStyle | BS_AUTOCHECKBOX, rect, this, IDC_PP_SW_YV16);
	CalcTextRect(rect, x2 + control_w * 2, y, control_w);
	m_cbFormat[PixFmt_P210].Create(L"P210", dwStyle | BS_AUTOCHECKBOX, rect, this, IDC_PP_SW_P210);
	CalcTextRect(rect, x2 + control_w * 3, y, control_w);
	m_cbFormat[PixFmt_P216].Create(L"P216", dwStyle | BS_AUTOCHECKBOX, rect, this, IDC_PP_SW_P216);
	y += 20;

	CalcTextRect(rect, x1, y, label_w);
	m_txt444.Create(L"4:4:4 YUV:", WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	CalcTextRect(rect, x2, y, control_w);
	m_cbFormat[PixFmt_AYUV].Create(L"AYUV", dwStyle | BS_AUTOCHECKBOX, rect, this, IDC_PP_SW_AYUV);
	CalcTextRect(rect, x2 + control_w, y, control_w);
	m_cbFormat[PixFmt_YV24].Create(L"YV24", dwStyle | BS_AUTOCHECKBOX, rect, this, IDC_PP_SW_YV24);
	CalcTextRect(rect, x2 + control_w * 2, y, control_w);
	m_cbFormat[PixFmt_Y410].Create(L"Y410", dwStyle | BS_AUTOCHECKBOX, rect, this, IDC_PP_SW_Y410);
	CalcTextRect(rect, x2 + control_w * 3, y, control_w);
	m_cbFormat[PixFmt_Y416].Create(L"Y416", dwStyle | BS_AUTOCHECKBOX, rect, this, IDC_PP_SW_Y416);
	y += 20;

	CalcTextRect(rect, x1, y, label_w);
	m_txtRGB.Create(L"RGB:", WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	CalcTextRect(rect, x2, y, 56);
	m_cbFormat[PixFmt_RGB32].Create(L"RGB32", dwStyle | BS_AUTOCHECKBOX, rect, this, IDC_PP_SW_RGB32);
	CalcTextRect(rect, x2 + control_w * 3, y, 56);
	m_cbFormat[PixFmt_RGB48].Create(L"RGB48", dwStyle | BS_AUTOCHECKBOX, rect, this, IDC_PP_SW_RGB48);
	y += 24;

	// Output levels
	control_w = 88;
	label_w = row_w - control_w;
	x2 = x1 + label_w;
	CalcTextRect(rect, x1, y, label_w);
	m_txtSwRGBLevels.Create(ResStr(IDS_VDF_COLOR_RGB_LEVELS), WS_VISIBLE | WS_CHILD, rect, this, (UINT)IDC_STATIC);
	CalcTextRect(rect, x2, y, control_w);
	m_cbSwRGBLevels.Create(dwStyle | CBS_DROPDOWNLIST | WS_VSCROLL, rect, this, IDC_PP_SWRGBLEVELS);
	m_cbSwRGBLevels.AddString(L"PC (0-255)");
	m_cbSwRGBLevels.AddString(L"TV (16-235)");
	y += 24;

	CalcRect(rect, x0, 340 - 32, 76, 32);
	m_btnReset.Create(ResStr(IDS_FILTER_RESET_SETTINGS), dwStyle | BS_MULTILINE, rect, this, IDC_PP_RESET);
	CalcTextRect(rect, x0 + 76, 340 - 16, group_w - 76);
	m_txtVersion.Create(WS_CHILD | WS_VISIBLE | ES_READONLY | ES_RIGHT, rect, this, (UINT)IDC_STATIC);

	///////////////////////////////////////

	for (CWnd* pWnd = GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetNextWindow()) {
		pWnd->SetFont(&m_font, FALSE);
	}

	CorrectComboListWidth(m_cbScanType);
	CorrectComboListWidth(m_cbDXVACompatibilityCheck);

	if (m_pMDF) {
		m_cbThreadNumber.SetCurSel(m_pMDF->GetThreadNumber());
		m_cbScanType.SetCurSel((int)m_pMDF->GetScanType());

		m_chARMode.SetCheck(m_pMDF->GetARMode());
		m_chSkipBFrames.SetCheck(m_pMDF->GetDiscardMode() == AVDISCARD_BIDIR);

		for (int i = 0; i < HWDec_count; i++) {
			m_cbHWCodec[i].SetCheck(!!m_pMDF->GetHwDecoder((MPCHwDecoder)i));
		}
		if (SysVersion::IsWin8orLater()) {
			m_chUseD3D11Decoder.SetCheck(!!m_pMDF->GetD3D11Decoder());
		}
		m_cbDXVACompatibilityCheck.SetCurSel(m_pMDF->GetDXVACheckCompatibility());
		m_chDXVA_SD.SetCheck(m_pMDF->GetDXVA_SD());

		for (int i = 0; i < PixFmt_count; i++) {
			if (i == PixFmt_YUY2) {
				m_cbFormat[PixFmt_YUY2].SetCheck(m_pMDF->GetSwPixelFormat(PixFmt_YUY2) ? BST_CHECKED : BST_INDETERMINATE);
			} else {
				m_cbFormat[i].SetCheck(m_pMDF->GetSwPixelFormat((MPCPixelFormat)i) ? BST_CHECKED : BST_UNCHECKED);
			}
		}

		m_cbSwRGBLevels.SetCurSel(m_pMDF->GetSwRGBLevels());

		if (m_cbFormat[PixFmt_RGB32].GetCheck() == BST_UNCHECKED && m_cbFormat[PixFmt_RGB48].GetCheck() == BST_UNCHECKED) {
			m_cbSwRGBLevels.EnableWindow(FALSE);
		}

		str.Format(L"MPC Video Decoder %s", m_pMDF->GetInformation(INFO_MPCVersion));
		m_txtVersion.SetWindowTextW(str);

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
		m_pMDF->SetThreadNumber(m_cbThreadNumber.GetCurSel());
		m_pMDF->SetScanType((MPC_SCAN_TYPE)m_cbScanType.GetCurSel());

		m_pMDF->SetARMode(m_chARMode.GetCheck());
		m_pMDF->SetDiscardMode(m_chSkipBFrames.GetCheck() ? AVDISCARD_BIDIR : AVDISCARD_DEFAULT);

		for (int i = 0; i < HWDec_count; i++) {
			m_pMDF->SetHwDecoder((MPCHwDecoder)i, m_cbHWCodec[i].GetCheck() == BST_CHECKED);
		}
		if (SysVersion::IsWin8orLater()) {
			m_pMDF->SetD3D11Decoder(m_chUseD3D11Decoder.GetCheck());
		}
		m_pMDF->SetDXVACheckCompatibility(m_cbDXVACompatibilityCheck.GetCurSel());
		m_pMDF->SetDXVA_SD(m_chDXVA_SD.GetCheck());

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

		if (refresh == 2) {
			SetTimer(m_nTimerID, 500, nullptr);
		}

		m_pMDF->SaveSettings();
	}

	return true;
}


BEGIN_MESSAGE_MAP(CMPCVideoDecSettingsWnd, CInternalPropertyPageWnd)
	ON_BN_CLICKED(IDC_PP_SW_YUY2, OnBnClickedYUY2)
	ON_BN_CLICKED(IDC_PP_SW_RGB32, OnBnClickedRGB32)
	ON_BN_CLICKED(IDC_PP_RESET, OnBnClickedReset)
	ON_NOTIFY_EX(TTN_NEEDTEXTW, 0, OnToolTipNotify)
	ON_WM_TIMER()
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
	if (m_cbFormat[PixFmt_RGB32].GetCheck() == BST_UNCHECKED && m_cbFormat[PixFmt_RGB48].GetCheck() == BST_UNCHECKED) {
		m_cbSwRGBLevels.EnableWindow(FALSE);
	} else {
		m_cbSwRGBLevels.EnableWindow(TRUE);
	}
}

void CMPCVideoDecSettingsWnd::OnBnClickedReset()
{
	m_cbThreadNumber.SetCurSel(0);
	m_cbScanType.SetCurSel(SCAN_AUTO);
	m_chARMode.SetCheck(BST_INDETERMINATE);
	m_chSkipBFrames.SetCheck(BST_UNCHECKED);

	if (SysVersion::IsWin8orLater()) {
		m_chUseD3D11Decoder.SetCheck(BST_CHECKED);
	}
	m_cbDXVACompatibilityCheck.SetCurSel(1);
	m_chDXVA_SD.SetCheck(BST_UNCHECKED);

	for (int i = 0; i < PixFmt_count; i++) {
		if (i == PixFmt_AYUV || i == PixFmt_RGB48) {
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
	TOOLTIPTEXTW* pTTT = (TOOLTIPTEXTW*)pNMHDR;
	static CString strTipText;
	UINT_PTR nID = pNMHDR->idFrom;

	if (pNMHDR->code == TTN_NEEDTEXTW && (pTTT->uFlags & TTF_IDISHWND)) {

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

void CMPCVideoDecSettingsWnd::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == m_nTimerID) {
		KillTimer(m_nTimerID);
		UpdateStatusInfo();
		SetDirty(false);
	}
	else {
		__super::OnTimer(nIDEvent);
	}
}

// ====== Codec filter property page (for standalone filter only)
#ifdef REGISTER_FILTER
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
	LPCWSTR		CodeName;
} mpc_codecs[] = {
	{CODEC_AMVV,		L"AMV video"},
	{CODEC_AV1,			L"AOMedia Video 1 (AV1)"},
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
	{CODEC_H264_MVC,	L"H.264 (MVC 3D)"},
	{CODEC_HEVC,		L"HEVC"},
	{CODEC_INDEO,		L"Indeo 3/4/5"},
	{CODEC_LOSSLESS,	L"Lossless video (huffyuv, Lagarith, FFV1, MagicYUV)"},
	{CODEC_MJPEG,		L"MJPEG"},
	{CODEC_MPEG1,		L"MPEG-1 (FFmpeg)"},
	{CODEC_MPEG2,		L"MPEG-2 (FFmpeg)"},
	{CODEC_MSMPEG4,		L"MS-MPEG4"},
	{CODEC_PNG,			L"PNG"},
	{CODEC_QT,			L"QuickTime video (8BPS, QTRle, rpza)"},
	{CODEC_SCREC,		L"Screen Recorder (CSCD, MS, TSCC, VMnc)"},
	{CODEC_SHQ,			L"SpeedHQ"},
	{CODEC_SVQ3,		L"SVQ1/3"},
	{CODEC_THEORA,		L"Theora"},
	{CODEC_UTVD,		L"Ut video"},
	{CODEC_VC1,			L"VC-1 (FFmpeg)"},
	{CODEC_HAP,			L"Vidvox Hap"},
	{CODEC_VP356,		L"VP3/5/6"},
	{CODEC_VP89,		L"VP7/8/9"},
	{CODEC_WMV,			L"WMV1/2/3"},
	{CODEC_XVID,		L"Xvid/MPEG-4"},
	{CODEC_REALV,		L"Real Video"},
	{CODEC_UNCOMPRESSED,L"Uncompressed video (v210, V410, Y8, I420, ...)"},
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

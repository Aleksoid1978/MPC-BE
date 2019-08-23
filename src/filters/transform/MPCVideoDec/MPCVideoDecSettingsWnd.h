/*
 * (C) 2006-2018 see Authors.txt
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

#pragma once

#include "../../filters/InternalPropertyPage.h"
#include "IMPCVideoDec.h"
#include "resource.h"

enum {
	IDC_PP_THREAD_NUMBER = 10000,
	IDC_PP_DEINTERLACING,
	IDC_PP_AR,
	IDC_PP_SKIPBFRAMES,
	IDC_PP_DXVA_CHECK,
	IDC_PP_DXVA_SD,
	IDC_PP_SW_NV12,
	IDC_PP_SW_YV12,
	IDC_PP_SW_YUY2,
	IDC_PP_SW_YV16,
	IDC_PP_SW_AYUV,
	IDC_PP_SW_YV24,
	IDC_PP_SW_P010,
	IDC_PP_SW_P210,
	IDC_PP_SW_Y410,
	IDC_PP_SW_P016,
	IDC_PP_SW_P216,
	IDC_PP_SW_Y416,
	IDC_PP_SW_RGB32,
	IDC_PP_SW_RGB48,
	IDC_PP_SWRGBLEVELS,
	IDC_PP_RESET,
};

class __declspec(uuid("D5AA0389-D274-48e1-BF50-ACB05A56DDE0"))
	CMPCVideoDecSettingsWnd : public CInternalPropertyPageWnd
{
	CComQIPtr<IMPCVideoDecFilter> m_pMDF;

	CButton		m_grpDecoder;
	CStatic		m_txtThreadNumber;
	CComboBox	m_cbThreadNumber;
	CStatic		m_txtDeinterlacing;
	CComboBox	m_cbDeinterlacing;
	CButton		m_chARMode;
	CButton		m_chSkipBFrames;

	CButton		m_grpDXVA;
	CStatic		m_txtDXVACompatibilityCheck;
	CComboBox	m_cbDXVACompatibilityCheck;
	CButton		m_cbDXVA_SD;

	CButton		m_grpStatus;
	CStatic		m_txtInputFormat;
	CEdit		m_edtInputFormat;
	CStatic		m_txtFrameSize;
	CEdit		m_edtFrameSize;
	CStatic		m_txtOutputFormat;
	CEdit		m_edtOutputFormat;
	CStatic		m_txtGraphicsAdapter;
	CEdit		m_edtGraphicsAdapter;

	CButton		m_grpFmtConv;
	CStatic		m_txtSwOutputFormats;
	CStatic     m_txt8bit;
	CStatic     m_txt10bit;
	CStatic     m_txt16bit;
	CStatic     m_txt420;
	CStatic     m_txt422;
	CStatic     m_txt444;
	CStatic     m_txtRGB;
	CButton		m_cbFormat[PixFmt_count];
	CStatic     m_txtSwRGBLevels;
	CComboBox   m_cbSwRGBLevels;

	CButton		m_btnReset;
	CStatic		m_txtMPCVersion;

	const UINT_PTR m_nTimerID = 1;

	void		UpdateStatusInfo();

public:
	CMPCVideoDecSettingsWnd();

	bool OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks);
	void OnDisconnect();
	bool OnActivate();
	void OnDeactivate();
	bool OnApply();

	static LPCWSTR GetWindowTitle() { return MAKEINTRESOURCEW(IDS_FILTER_SETTINGS_CAPTION); }
	static CSize GetWindowSize() { return CSize(647, 295); }

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedYUY2();
	afx_msg void OnBnClickedRGB32();
	afx_msg void OnBnClickedReset();
	afx_msg BOOL OnToolTipNotify(UINT id, NMHDR * pNMHDR, LRESULT * pResult);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};

#ifdef REGISTER_FILTER
class __declspec(uuid("3C395D46-8B0F-440d-B962-2F4A97355453"))
	CMPCVideoDecCodecWnd : public CInternalPropertyPageWnd
{
	CComQIPtr<IMPCVideoDecFilter> m_pMDF;

	CButton			m_grpSelectedCodec;
	CCheckListBox	m_lstCodecs;
	CImageList		m_onoff;

public:
	CMPCVideoDecCodecWnd();

	bool OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks);
	void OnDisconnect();
	bool OnActivate();
	void OnDeactivate();
	bool OnApply();

	static LPCWSTR GetWindowTitle() { return L"Codecs";    }
	static CSize GetWindowSize()    { return CSize(340, 290); }

	DECLARE_MESSAGE_MAP()
};
#endif

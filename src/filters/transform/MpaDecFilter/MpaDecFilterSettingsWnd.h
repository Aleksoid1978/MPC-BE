/*
 * (C) 2003-2006 Gabest
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

#pragma once

#include "../../filters/InternalPropertyPage.h"
#include "MpaDecFilter.h"
#include "resource.h"

class __declspec(uuid("24103041-884B-4772-B0D3-A600E7CBFEC7"))
	CMpaDecSettingsWnd : public CInternalPropertyPageWnd
{
	CComQIPtr<IMpaDecFilter> m_pMDF;

	bool  m_outfmt_i16;
	bool  m_outfmt_i24;
	bool  m_outfmt_i32;
	bool  m_outfmt_flt;
	bool m_drc;
	bool m_spdif_ac3;
	bool m_spdif_eac3;
	bool m_spdif_truehd;
	bool m_spdif_dts;
	bool m_spdif_dtshd;
	bool m_spdif_ac3enc;

	enum {
		IDC_PP_CHECK_I16 = 10000,
		IDC_PP_CHECK_I24,
		IDC_PP_CHECK_I32,
		IDC_PP_CHECK_FLT,
		IDC_PP_CHECK_DRC,
		IDC_PP_CHECK_SPDIF_AC3,
		IDC_PP_CHECK_SPDIF_EAC3,
		IDC_PP_CHECK_SPDIF_TRUEHD,
		IDC_PP_CHECK_SPDIF_DTS,
		IDC_PP_CHECK_SPDIF_DTSHD,
		IDC_PP_CHECK_SPDIF_AC3ENC
	};

	CButton   m_outfmt_group;
	CButton   m_outfmt_i16_check;
	CButton   m_outfmt_i24_check;
	CButton   m_outfmt_i32_check;
	CButton   m_outfmt_flt_check;

	CButton   m_drc_check;

	CButton   m_spdif_group;
	CButton   m_spdif_ac3_check;
	CButton   m_spdif_eac3_check;
	CButton   m_spdif_truehd_check;
	CButton   m_spdif_dts_check;
	CButton   m_spdif_dtshd_check;
	CButton   m_spdif_ac3enc_check;

	CButton		m_grpStatus;
	CEdit		m_edtStatus;

	void		UpdateStatusInfo();

public:
	CMpaDecSettingsWnd();

	bool OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks);
	void OnDisconnect();
	bool OnActivate();
	void OnDeactivate();
	bool OnApply();

	static LPCWSTR GetWindowTitle() { return MAKEINTRESOURCE(IDS_FILTER_SETTINGS_CAPTION); }
	static CSize GetWindowSize()    { return CSize(273, 240); }

	DECLARE_MESSAGE_MAP()

	afx_msg void OnInt16Check();
	afx_msg void OnInt24Check();
	afx_msg void OnInt32Check();
	afx_msg void OnFloatCheck();
	afx_msg void OnDTSCheck();
};

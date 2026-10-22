/*
 * (C) 2017-2026 see Authors.txt
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
#include "DXVAState.h"
#include "apps/mplayerc/mpc_messages.h"
#include "DSUtil.h"
#include <dxva2api.h>
#include <dxva2_guids.h>

static const struct {
	const GUID* Guid;
	const WCHAR* Description;
} s_dxva2_vld_decoders[] = {
	// MPEG1
	{&DXVA2_ModeMPEG1_VLD,							L"MPEG-1"},
	// MPEG2
	{&DXVA2_ModeMPEG2_VLD,							L"MPEG-2"},
	{&DXVA2_ModeMPEG2and1_VLD,						L"MPEG-2/MPEG-1"},
	// MPEG4
	{&DXVA2_ModeMPEG4pt2_VLD_Simple,				L"MPEG-4 SP"},
	{&DXVA2_ModeMPEG4pt2_VLD_AdvSimple_NoGMC,		L"MPEG-4 ASP, no GMC"},
	{&DXVA2_ModeMPEG4pt2_VLD_AdvSimple_GMC,			L"MPEG-4 ASP, GMC"},
	{&DXVA2_MPEG4pt2_VLD_AdvSimple_Nvidia,			L"MPEG-4 ASP (Nvidia)"},
	// VC-1
	{&DXVA2_ModeVC1_D,/*DXVA2_ModeVC1_VLD*/			L"VC-1"},
	{&DXVA2_ModeVC1_D2010,							L"VC-1 (2010)"},
	{&DXVA2_VC1_VLD_Intel,							L"VC-1 (Intel)"},
	{&DXVA2_VC1_VLD_2_Intel,						L"VC-1 (Intel, 2)"},
	// H.264
	{&DXVA2_ModeH264_E,/*DXVA2_ModeH264_VLD_NoFGT*/	L"H.264, no FGT"},
	{&DXVA2_ModeH264_F,/*DXVA2_ModeH264_VLD_FGT*/	L"H.264, FGT"},
	{&DXVA2_ModeH264_VLD_WithFMOASO_NoFGT,			L"H.264, no FGT, with FMOASO"},
	{&DXVA2_H264_VLD_Intel,							L"H.264 (Intel)"},
	{&DXVA2_H264_VLD_NoFGT_AMD,						L"H.264 (AMD)"},
	// H.264 stereo
	{&DXVA2_ModeH264_VLD_Stereo_Progressive_NoFGT,	L"H.264 stereo progressive, no FGT"},
	{&DXVA2_ModeH264_VLD_Stereo_NoFGT,				L"H.264 stereo, no FGT"},
	{&DXVA2_ModeH264_VLD_Multiview_NoFGT,			L"H.264 multiview, no FGT"},
	// HEVC
	{&DXVA2_ModeHEVC_VLD_Main,						L"HEVC"},
	{&DXVA2_ModeHEVC_VLD_Main10,					L"HEVC 10-bit"},

	{&D3D11_DECODER_PROFILE_HEVC_VLD_MAIN12,		L"HEVC 12-bit"},
	{&D3D11_DECODER_PROFILE_HEVC_VLD_MAIN10_422,	L"HEVC 422 10-bit"},
	{&D3D11_DECODER_PROFILE_HEVC_VLD_MAIN12_422,	L"HEVC 422 12-bit"},
	{&D3D11_DECODER_PROFILE_HEVC_VLD_MAIN_444,		L"HEVC 444 8-bit"},
	{&D3D11_DECODER_PROFILE_HEVC_VLD_MAIN10_444,	L"HEVC 444 10-bit"},
	{&D3D11_DECODER_PROFILE_HEVC_VLD_MAIN12_444,	L"HEVC 444 12-bit"},
	// VP8
	{&DXVA2_ModeVP8_VLD,							L"VP8"},
	// VP9
	{&DXVA2_ModeVP9_VLD_Profile0,					L"VP9"},
	{&DXVA2_ModeVP9_VLD_10bit_Profile2,				L"VP9 10-bit"},
	{&DXVA2_VP9_VLD_Intel,							L"VP9 Intel"},
	// AV1
	{&DXVA2_ModeAV1_VLD_Profile0,					L"AV1 profile 0"},
	// HEVC Intel
	{&DXVA2_HEVC_VLD_Main12_Intel,					L"HEVC 12-bit Intel"},
	{&DXVA2_HEVC_VLD_Main422_10_Intel,				L"HEVC 422 10-bit Intel"},
	{&DXVA2_HEVC_VLD_Main422_12_Intel,				L"HEVC 422 12-bit Intel"},
	{&DXVA2_HEVC_VLD_Main444_Intel,					L"HEVC 444 8-bit Intel"},
	{&DXVA2_HEVC_VLD_Main444_10_Intel,				L"HEVC 444 10-bit Intel"},
	{&DXVA2_HEVC_VLD_Main444_12_Intel,				L"HEVC 444 12-bit Intel"},
};

CStringW GetDXVAModeString(const GUID& guidDecoder)
{
	if (guidDecoder == GUID_NULL) {
		return L"Not using DXVA";
	}

	for (const auto& decoder : s_dxva2_vld_decoders) {
		if (guidDecoder == *decoder.Guid) {
			return decoder.Description;
		}
	}

	return CStringFromGUID(guidDecoder);
}

CStringW GetDXVAModeStringAndName(const GUID& guidDecoder)
{
	CStringW str = CStringFromGUID(guidDecoder) + L" ";

	if (guidDecoder == GUID_NULL) {
		str.Append(L"Not using DXVA");
		return str;
	}

	for (const auto& decoder : s_dxva2_vld_decoders) {
		if (guidDecoder == *decoder.Guid) {
			str.Append(decoder.Description);
			return str;
		}
	}

	str.Append(L"Unknown");
	return str;
}

namespace DXVAState {
	BOOL m_bDXVActive      = FALSE;
	GUID m_guidDXVADecoder = GUID_NULL;

	CString m_sDXVADecoderDescription = L"Software";
	CString m_sDXVADecoderShortDescription;

	static void OnChangeState()
	{
		if (const auto app = AfxGetApp()) {
			if (const auto wnd = app->GetMainWnd()) {
				::PostMessageW(wnd->GetSafeHwnd(), WM_DXVASTATE_CHANGE, 0, 0);
			}
		}
	}

	void ClearState()
	{
		if (m_bDXVActive) {
			m_bDXVActive      = FALSE;
			m_guidDXVADecoder = GUID_NULL;

			m_sDXVADecoderDescription = L"Software";
			m_sDXVADecoderShortDescription.Empty();

			OnChangeState();
		}
	}

	void SetActiveState(const GUID& guidDXVADecoder, LPCWSTR customDescription/* = nullptr*/)
	{
		const bool bChangeState = (m_bDXVActive == FALSE);
		m_bDXVActive = TRUE;

		if (customDescription) {
			m_sDXVADecoderDescription = customDescription;
			if (guidDXVADecoder != GUID_NULL) {
				m_guidDXVADecoder = guidDXVADecoder;
			}
			if (m_guidDXVADecoder != GUID_NULL) {
				m_sDXVADecoderDescription.AppendFormat(L", %s", GetDXVAModeString(m_guidDXVADecoder));
			}
			m_sDXVADecoderShortDescription = L"H/W";
		} else if (guidDXVADecoder != GUID_NULL) {
			m_guidDXVADecoder = guidDXVADecoder;
			m_sDXVADecoderShortDescription = L"DXVA2";
			m_sDXVADecoderDescription.Format(L"DXVA2 Native, %s", GetDXVAModeString(m_guidDXVADecoder));
		}

		if (bChangeState) {
			OnChangeState();
		}
	}

	const BOOL GetState()
	{
		return m_bDXVActive;
	}

	LPCWSTR GetDescription()
	{
		return m_sDXVADecoderDescription.GetString();
	}

	LPCWSTR GetShortDescription()
	{
		return m_sDXVADecoderShortDescription.GetString();
	}

	const GUID GetDecoderGUID()
	{
		return m_guidDXVADecoder;
	}
}

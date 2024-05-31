/*
 * (C) 2017-2024 see Authors.txt
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
				m_sDXVADecoderDescription.AppendFormat(L", %s", GetDXVAMode(m_guidDXVADecoder));
			}
			m_sDXVADecoderShortDescription = L"H/W";
		} else if (guidDXVADecoder != GUID_NULL) {
			m_guidDXVADecoder = guidDXVADecoder;
			m_sDXVADecoderShortDescription = L"DXVA2";
			m_sDXVADecoderDescription.Format(L"DXVA2 Native, %s", GetDXVAMode(m_guidDXVADecoder));
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

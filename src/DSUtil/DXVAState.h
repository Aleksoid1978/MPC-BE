/*
 * (C) 2017 see Authors.txt
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

#include <Windows.h>

namespace DXVAState {
	static BOOL m_bDXVActive      = FALSE;
	static GUID m_guidDXVADecoder = GUID_NULL;

	static CString m_sDXVADecoderDescription = L"Software";
	static CString m_sDXVADecoderShortDescription;

	inline void ClearState()
	{
		m_bDXVActive      = FALSE;
		m_guidDXVADecoder = GUID_NULL;

		m_sDXVADecoderDescription      = L"Software";
		m_sDXVADecoderShortDescription = L"";
	}

	inline void SetActiveState(const GUID& guidDXVADecoder, const CString& customDescription = L"")
	{
		m_bDXVActive = TRUE;

		if (!customDescription.IsEmpty()) {
			m_sDXVADecoderDescription = customDescription;
			if (m_guidDXVADecoder != GUID_NULL) {
				m_sDXVADecoderDescription.AppendFormat(L", %s", GetDXVAMode(m_guidDXVADecoder));
			}
			m_sDXVADecoderShortDescription = L"H/W";
		} else if (guidDXVADecoder != GUID_NULL) {
			m_guidDXVADecoder = guidDXVADecoder;
			m_sDXVADecoderShortDescription = L"DXVA2";
			m_sDXVADecoderDescription.Format(L"DXVA2 Native, %s", GetDXVAMode(m_guidDXVADecoder));
		}
	}

	inline const BOOL GetState()
	{
		return m_bDXVActive;
	}

	inline const CString GetDescription()
	{
		return m_sDXVADecoderDescription;
	}

	inline const CString GetShortDescription()
	{
		return m_sDXVADecoderShortDescription;
	}
}

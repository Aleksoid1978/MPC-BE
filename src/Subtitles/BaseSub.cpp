/*
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
#include "BaseSub.h"

CBaseSub::CBaseSub(SUBTITLE_TYPE nType)
	: m_nType(nType)
	, m_bResizedRender(FALSE)
	, m_pTempSpdBuff(NULL)
	, convertType(ColorConvert::convertType::DEFAULT)
{
}

CBaseSub::~CBaseSub()
{
	m_pTempSpdBuff.Free();
}

void CBaseSub::InitSpd(SubPicDesc& spd, int nWidth, int nHeight)
{
	if (m_spd.w != nWidth || m_spd.h != nHeight || m_pTempSpdBuff == NULL) {
		m_spd.type		= 0;
		m_spd.w			= nWidth;
		m_spd.h			= nHeight;
		m_spd.bpp		= 32;
		m_spd.pitch		= m_spd.w * 4;
		m_spd.vidrect	= CRect(0, 0, m_spd.w, m_spd.h);

		m_pTempSpdBuff.Free();
		m_pTempSpdBuff.Allocate(m_spd.pitch * m_spd.h);
		m_spd.bits		= (void*)m_pTempSpdBuff;
	}

	if (!m_bResizedRender && (m_spd.w != spd.w || m_spd.h != spd.h)) {
		m_bResizedRender = TRUE;

		BYTE* p = (BYTE*)m_spd.bits;
		for (int y = 0; y < m_spd.h; y++, p += m_spd.pitch) {
			memsetd(p, 0xFF000000, m_spd.w * 4);
		}
	}
}

void CBaseSub::FinalizeRender(SubPicDesc& spd)
{
	if (m_bResizedRender) {
		m_bResizedRender = FALSE;

		// StretchBlt ...
		int filter = (spd.w < m_spd.w && spd.h < m_spd.h) ? CResampleARGB::FILTER_BOX : CResampleARGB::FILTER_BILINEAR;

		HRESULT hr = m_resample.SetParameters(spd.w, spd.h, m_spd.w, m_spd.h, filter);
		if (S_OK == hr) {
			hr = m_resample.Process((BYTE*)spd.bits, (BYTE*)m_spd.bits);
		}
		ASSERT(hr == S_OK);
	}
}

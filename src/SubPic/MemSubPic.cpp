/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2022 see Authors.txt
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
#include <mpc_defines.h>
#include "DSUtil/Utils.h"
#include "MemSubPic.h"

//
// CMemSubPic
//

CMemSubPic::CMemSubPic(SubPicDesc& spd)
	: m_spd(spd)
{
	m_maxsize.SetSize(spd.w, spd.h);
	m_rcDirty.SetRect(0, 0, spd.w, spd.h);
}

CMemSubPic::~CMemSubPic()
{
	SAFE_DELETE_ARRAY(m_spd.bits);
}

// ISubPic

STDMETHODIMP_(void*) CMemSubPic::GetObject()
{
	return (void*)&m_spd;
}

STDMETHODIMP CMemSubPic::GetDesc(SubPicDesc& spd)
{
	spd.type    = m_spd.type;
	spd.w       = m_size.cx;
	spd.h       = m_size.cy;
	spd.bpp     = m_spd.bpp;
	spd.pitch   = m_spd.pitch;
	spd.bits    = m_spd.bits;
	spd.bitsU   = m_spd.bitsU;
	spd.bitsV   = m_spd.bitsV;
	spd.vidrect = m_vidrect;

	return S_OK;
}

STDMETHODIMP CMemSubPic::CopyTo(ISubPic* pSubPic)
{
	HRESULT hr;
	if (FAILED(hr = __super::CopyTo(pSubPic))) {
		return hr;
	}

	SubPicDesc src, dst;
	if (FAILED(GetDesc(src)) || FAILED(pSubPic->GetDesc(dst))) {
		return E_FAIL;
	}

	ASSERT(src.bpp == 32 && dst.bpp == 32);

	const UINT copyW_bytes = m_rcDirty.Width() * 4;
	UINT copyH = m_rcDirty.Height();

	BYTE* s = src.bits + src.pitch * m_rcDirty.top + m_rcDirty.left * 4;
	BYTE* d = dst.bits + dst.pitch * m_rcDirty.top + m_rcDirty.left * 4;

	while (copyH--) {
		memcpy(d, s, copyW_bytes);
		s += src.pitch;
		d += dst.pitch;
	}

	return S_OK;
}

STDMETHODIMP CMemSubPic::ClearDirtyRect()
{
	if (m_rcDirty.IsRectEmpty()) {
		return S_FALSE;
	}

	ASSERT(m_spd.bpp == 32);

	BYTE* ptr = m_spd.bits + m_spd.pitch * m_rcDirty.top + m_rcDirty.left * 4;
	const UINT dirtyW = m_rcDirty.Width();
	UINT dirtyH = m_rcDirty.Height();

	while (dirtyH-- > 0) {
		fill_u32(ptr, m_bInvAlpha ? 0x00000000 : 0xFF000000, dirtyW);
		ptr += m_spd.pitch;
	}

	m_rcDirty.SetRectEmpty();

	return S_OK;
}

STDMETHODIMP CMemSubPic::Lock(SubPicDesc& spd)
{
	return GetDesc(spd);
}

STDMETHODIMP CMemSubPic::Unlock(RECT* pDirtyRect)
{
	m_rcDirty = pDirtyRect ? *pDirtyRect : CRect(0, 0, m_spd.w, m_spd.h);

	return S_OK;
}

STDMETHODIMP CMemSubPic::AlphaBlt(RECT* pSrc, RECT* pDst, SubPicDesc* pTarget)
{
	ASSERT(pTarget);

	if (!pSrc || !pDst || !pTarget) {
		return E_POINTER;
	}

	const SubPicDesc& src = m_spd;
	SubPicDesc dst = *pTarget;

	if (src.type != dst.type) {
		return E_INVALIDARG;
	}

	CRect rs(*pSrc), rd(*pDst);

	if (dst.h < 0) {
		dst.h     = -dst.h;
		rd.bottom = dst.h - rd.bottom;
		rd.top    = dst.h - rd.top;
	}

	if (rs.Width() != rd.Width() || rs.Height() != abs(rd.Height())) {
		return E_INVALIDARG;
	}

	ASSERT(src.bpp == 32 && dst.bpp == 32);

	const int w = rs.Width();
	const int h = rs.Height();
	BYTE* s = src.bits + src.pitch * rs.top + (rs.left * 4);
	BYTE* d = dst.bits + dst.pitch * rd.top + (rd.left * 4);

	if (rd.top > rd.bottom) {
		d = dst.bits + dst.pitch * (rd.top - 1) + (rd.left * 4);
		dst.pitch = -dst.pitch;
	}

	for (int j = 0; j < h; j++, s += src.pitch, d += dst.pitch) {
		BYTE* s2 = s;
		BYTE* s2end = s2 + w * 4;

		uint32_t* d2 = (uint32_t*)d;
		for (; s2 < s2end; s2 += 4, d2++) {
#ifdef _WIN64
			uint32_t ia = 256-s2[3];
			if (s2[3] < 0xff) {
				*d2 = ((((*d2&0x00ff00ff)*s2[3])>>8) + (((*((uint32_t*)s2)&0x00ff00ff)*ia)>>8)&0x00ff00ff)
					| ((((*d2&0x0000ff00)*s2[3])>>8) + (((*((uint32_t*)s2)&0x0000ff00)*ia)>>8)&0x0000ff00);
			}
#else
			if (s2[3] < 0xff) {
				*d2 = ((((*d2&0x00ff00ff)*s2[3])>>8) + (*((uint32_t*)s2)&0x00ff00ff)&0x00ff00ff)
					| ((((*d2&0x0000ff00)*s2[3])>>8) + (*((uint32_t*)s2)&0x0000ff00)&0x0000ff00);
			}
#endif
		}
	}

	dst.pitch = abs(dst.pitch);

	return S_OK;
}

//
// CMemSubPicAllocator
//

CMemSubPicAllocator::CMemSubPicAllocator(int type, SIZE maxsize)
	: CSubPicAllocatorImpl(maxsize, false)
	, m_type(type)
	, m_maxsize(maxsize)
{
}

// ISubPicAllocatorImpl

bool CMemSubPicAllocator::Alloc(bool fStatic, ISubPic** ppSubPic)
{
	if (!ppSubPic) {
		return false;
	}

	SubPicDesc spd;
	spd.w     = m_maxsize.cx;
	spd.h     = m_maxsize.cy;
	spd.bpp   = 32;
	spd.pitch = spd.w * 4;
	spd.type  = m_type;
	spd.bits  = new(std::nothrow) BYTE[spd.pitch * spd.h];
	if (!spd.bits) {
		return false;
	}

	*ppSubPic = DNew CMemSubPic(spd);
	if (!(*ppSubPic)) {
		return false;
	}

	(*ppSubPic)->AddRef();
	(*ppSubPic)->SetInverseAlpha(m_bInvAlpha);

	return true;
}

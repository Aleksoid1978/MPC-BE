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
#include "DSUtil/Utils.h"
#include "DX9SubPic.h"

//
// CDX9SubPic
//

CDX9SubPic::CDX9SubPic(IDirect3DSurface9* pSurface, CDX9SubPicAllocator *pAllocator, bool bExternalRenderer)
	: m_pSurface(pSurface)
	, m_pAllocator(pAllocator)
	, m_bExternalRenderer(bExternalRenderer)
{
	D3DSURFACE_DESC d3dsd;
	ZeroMemory(&d3dsd, sizeof(d3dsd));
	if (SUCCEEDED(m_pSurface->GetDesc(&d3dsd))) {
		m_maxsize.SetSize(d3dsd.Width, d3dsd.Height);
		m_rcDirty.SetRect(0, 0, d3dsd.Width, d3dsd.Height);
	}
}

CDX9SubPic::~CDX9SubPic()
{
	{
		CAutoLock Lock(&CDX9SubPicAllocator::ms_SurfaceQueueLock);
		// Add surface to cache
		if (m_pAllocator) {
			for (auto it = m_pAllocator->m_AllocatedSurfaces.begin(), end = m_pAllocator->m_AllocatedSurfaces.end(); it != end; ++it) {
				if (*it == this) {
					m_pAllocator->m_AllocatedSurfaces.erase(it);
					break;
				}
			}
			m_pAllocator->m_FreeSurfaces.push_back(m_pSurface);
		}
	}
}

// ISubPic

STDMETHODIMP_(void*) CDX9SubPic::GetObject()
{
	CComPtr<IDirect3DTexture9> pTexture;
	if (SUCCEEDED(m_pSurface->GetContainer(IID_IDirect3DTexture9, (void**)&pTexture))) {
		return (void*)(IDirect3DTexture9*)pTexture;
	}

	return nullptr;
}

STDMETHODIMP CDX9SubPic::GetDesc(SubPicDesc& spd)
{
	spd.type    = 0;
	spd.w       = m_size.cx;
	spd.h       = m_size.cy;
	spd.bpp     = 32;
	spd.pitch   = 0;
	spd.bits    = nullptr;
	spd.vidrect = m_vidrect;

	return S_OK;
}

STDMETHODIMP CDX9SubPic::CopyTo(ISubPic* pSubPic)
{
	HRESULT hr;
	if (FAILED(hr = __super::CopyTo(pSubPic))) {
		return hr;
	}

	if (m_rcDirty.IsRectEmpty()) {
		return S_FALSE;
	}

	CComPtr<IDirect3DDevice9> pD3DDev;
	if (!m_pSurface || FAILED(m_pSurface->GetDevice(&pD3DDev)) || !pD3DDev) {
		return E_FAIL;
	}

	IDirect3DTexture9* pSrcTex = (IDirect3DTexture9*)GetObject();
	CComPtr<IDirect3DSurface9> pSrcSurf;
	pSrcTex->GetSurfaceLevel(0, &pSrcSurf);
	D3DSURFACE_DESC srcDesc;
	pSrcSurf->GetDesc(&srcDesc);

	IDirect3DTexture9* pDstTex = (IDirect3DTexture9*)pSubPic->GetObject();
	CComPtr<IDirect3DSurface9> pDstSurf;
	pDstTex->GetSurfaceLevel(0, &pDstSurf);
	D3DSURFACE_DESC dstDesc;
	pDstSurf->GetDesc(&dstDesc);

	RECT r;
	SetRect(&r, 0, 0, std::min(srcDesc.Width, dstDesc.Width), std::min(srcDesc.Height, dstDesc.Height));
	POINT p = { 0, 0 };
	hr = pD3DDev->UpdateSurface(pSrcSurf, &r, pDstSurf, &p);

	return SUCCEEDED(hr) ? S_OK : E_FAIL;
}

STDMETHODIMP CDX9SubPic::ClearDirtyRect(DWORD color)
{
	m_ClearColor = color;

	if (m_rcDirty.IsRectEmpty()) {
		return S_FALSE;
	}

	return S_OK;
}

STDMETHODIMP CDX9SubPic::Lock(SubPicDesc& spd)
{
	D3DLOCKED_RECT LockedRect;
	ZeroMemory(&LockedRect, sizeof(LockedRect));
	if (FAILED(m_pSurface->LockRect(&LockedRect, nullptr, D3DLOCK_NO_DIRTY_UPDATE|D3DLOCK_NOSYSLOCK))) {
		return E_FAIL;
	}

	if (!m_rcDirty.IsRectEmpty()) {
		const int linesize = m_rcDirty.Width() * 4;
		int h = m_rcDirty.Height();

		BYTE* ptr = (BYTE*)LockedRect.pBits + LockedRect.Pitch * m_rcDirty.top + (m_rcDirty.left * 4);

		while (h-- > 0) {
			memset_u32(ptr, m_ClearColor, linesize);
			ptr += LockedRect.Pitch;
		}

		m_rcDirty.SetRectEmpty();
	}

	spd.type = 0;
	spd.w       = m_size.cx;
	spd.h       = m_size.cy;
	spd.bpp     = 32;
	spd.pitch   = LockedRect.Pitch;
	spd.bits    = (BYTE*)LockedRect.pBits;
	spd.vidrect = m_vidrect;

	return S_OK;
}

STDMETHODIMP CDX9SubPic::Unlock(RECT* pDirtyRect)
{
	m_pSurface->UnlockRect();

	if (pDirtyRect) {
		m_rcDirty = *pDirtyRect;
		if (!m_rcDirty.IsRectEmpty()) {
			m_rcDirty.InflateRect(1, 1);
			m_rcDirty.left &= ~127;
			m_rcDirty.top &= ~63;
			m_rcDirty.right = (m_rcDirty.right + 127) & ~127;
			m_rcDirty.bottom = (m_rcDirty.bottom + 63) & ~63;
			m_rcDirty &= CRect(CPoint(0, 0), m_size);

			CComPtr<IDirect3DTexture9> pTexture = (IDirect3DTexture9*)GetObject();
			if (pTexture) {
				pTexture->AddDirtyRect(&m_rcDirty);
			}
		}
	} else {
		m_rcDirty = CRect(CPoint(0, 0), m_size);
	}

	return S_OK;
}

STDMETHODIMP CDX9SubPic::AlphaBlt(RECT* pSrc, RECT* pDst, SubPicDesc* pTarget)
{
	ASSERT(pTarget == nullptr);

	if (!pSrc || !pDst) {
		return E_POINTER;
	}
	CRect rSrc(*pSrc), rDst(*pDst);

	CComPtr<IDirect3DDevice9> pD3DDev;
	CComPtr<IDirect3DTexture9> pTexture = (IDirect3DTexture9*)GetObject();
	if (!pTexture || FAILED(pTexture->GetDevice(&pD3DDev)) || !pD3DDev) {
		return E_NOINTERFACE;
	}

	HRESULT hr;

	do {
		D3DSURFACE_DESC d3dsd;
		ZeroMemory(&d3dsd, sizeof(d3dsd));
		if (FAILED(pTexture->GetLevelDesc(0, &d3dsd)) /*|| d3dsd.Type != D3DRTYPE_TEXTURE*/) {
			break;
		}

		float w = (float)d3dsd.Width;
		float h = (float)d3dsd.Height;

		struct {
			float x, y, z, rhw;
			float tu, tv;
		}
		pVertices[] = {
			{(float)rDst.left, (float)rDst.top, 0.5f, 2.0f, (float)rSrc.left / w, (float)rSrc.top / h},
			{(float)rDst.right, (float)rDst.top, 0.5f, 2.0f, (float)rSrc.right / w, (float)rSrc.top / h},
			{(float)rDst.left, (float)rDst.bottom, 0.5f, 2.0f, (float)rSrc.left / w, (float)rSrc.bottom / h},
			{(float)rDst.right, (float)rDst.bottom, 0.5f, 2.0f, (float)rSrc.right / w, (float)rSrc.bottom / h},
		};

		for (size_t i = 0; i < std::size(pVertices); i++) {
			pVertices[i].x -= 0.5f;
			pVertices[i].y -= 0.5f;
		}

		hr = pD3DDev->SetTexture(0, pTexture);

		// GetRenderState fails for devices created with D3DCREATE_PUREDEVICE
		// so we need to provide default values in case GetRenderState fails
		DWORD abe, sb, db;
		if (FAILED(pD3DDev->GetRenderState(D3DRS_ALPHABLENDENABLE, &abe)))
			abe = FALSE;
		if (FAILED(pD3DDev->GetRenderState(D3DRS_SRCBLEND, &sb)))
			sb = D3DBLEND_ONE;
		if (FAILED(pD3DDev->GetRenderState(D3DRS_DESTBLEND, &db)))
			db = D3DBLEND_ZERO;

		hr = pD3DDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		hr = pD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);
		hr = pD3DDev->SetRenderState(D3DRS_ZENABLE, FALSE);
		hr = pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		hr = pD3DDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE); // pre-multiplied src and ...
		hr = pD3DDev->SetRenderState(D3DRS_DESTBLEND, m_bInvAlpha ? D3DBLEND_INVSRCALPHA : D3DBLEND_SRCALPHA); // ... inverse alpha channel for dst

		hr = pD3DDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		hr = pD3DDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		hr = pD3DDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

		if (rSrc == rDst) {
			hr = pD3DDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
			hr = pD3DDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		}
		else {
			hr = pD3DDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			hr = pD3DDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		}
		hr = pD3DDev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

		hr = pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
		hr = pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);
		hr = pD3DDev->SetSamplerState(0, D3DSAMP_BORDERCOLOR, m_bInvAlpha ? 0x00000000 : 0xFF000000);

		/*
		D3DCAPS9 d3dcaps9;
		hr = pD3DDev->GetDeviceCaps(&d3dcaps9);
		if (d3dcaps9.AlphaCmpCaps & D3DPCMPCAPS_LESS)
		{
			hr = pD3DDev->SetRenderState(D3DRS_ALPHAREF, (DWORD)0x000000FE);
			hr = pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
			hr = pD3DDev->SetRenderState(D3DRS_ALPHAFUNC, D3DPCMPCAPS_LESS);
		}
		*/

		hr = pD3DDev->SetPixelShader(nullptr);

		if ((m_bExternalRenderer) && (FAILED(hr = pD3DDev->BeginScene()))) {
			break;
		}

		hr = pD3DDev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
		hr = pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));

		if (m_bExternalRenderer) {
			hr = pD3DDev->EndScene();
		}

		//

		pD3DDev->SetTexture(0, nullptr);

		pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, abe);
		pD3DDev->SetRenderState(D3DRS_SRCBLEND, sb);
		pD3DDev->SetRenderState(D3DRS_DESTBLEND, db);

		return S_OK;
	} while (0);

	return E_FAIL;
}

//
// CDX9SubPicAllocator
//

CDX9SubPicAllocator::CDX9SubPicAllocator(IDirect3DDevice9* pD3DDev, SIZE maxsize, bool bExternalRenderer)
	: CSubPicAllocatorImpl(maxsize, true)
	, m_pD3DDev(pD3DDev)
	, m_maxsize(maxsize)
	, m_bExternalRenderer(bExternalRenderer)
{
}

CCritSec CDX9SubPicAllocator::ms_SurfaceQueueLock;

CDX9SubPicAllocator::~CDX9SubPicAllocator()
{
	ClearCache();
}

void CDX9SubPicAllocator::GetStats(int &_nFree, int &_nAlloc)
{
	CAutoLock Lock(&ms_SurfaceQueueLock);
	_nFree = (int)m_FreeSurfaces.size();
	_nAlloc = (int)m_AllocatedSurfaces.size();
}

void CDX9SubPicAllocator::ClearCache()
{
	{
		// Clear the allocator of any remaining subpics
		CAutoLock Lock(&ms_SurfaceQueueLock);
		for (auto& pSubPic : m_AllocatedSurfaces) {
			pSubPic->m_pAllocator = nullptr;
		}
		m_AllocatedSurfaces.clear();
		m_FreeSurfaces.clear();
	}
}

// ISubPicAllocator

STDMETHODIMP CDX9SubPicAllocator::ChangeDevice(IUnknown* pDev)
{
	ClearCache();
	CComQIPtr<IDirect3DDevice9> pD3DDev = pDev;
	if (!pD3DDev) {
		return E_NOINTERFACE;
	}

	CAutoLock cAutoLock(this);
	HRESULT hr = S_FALSE;
	if (m_pD3DDev != pD3DDev) {
		ClearCache();
		m_pD3DDev = pD3DDev;
		hr = __super::ChangeDevice(pDev);
	}

	return hr;
}

STDMETHODIMP CDX9SubPicAllocator::SetMaxTextureSize(SIZE MaxTextureSize)
{
	CAutoLock cAutoLock(this);
	if (m_maxsize != MaxTextureSize) {
		if (m_maxsize.cx < MaxTextureSize.cx || m_maxsize.cy < MaxTextureSize.cy) {
			ClearCache();
		}
		m_maxsize = MaxTextureSize;
	}

	SetCurSize(MaxTextureSize);
	SetCurVidRect(CRect(CPoint(0,0), MaxTextureSize));

	return S_OK;
}

// ISubPicAllocatorImpl

bool CDX9SubPicAllocator::Alloc(bool fStatic, ISubPic** ppSubPic)
{
	if (!ppSubPic) {
		return false;
	}

	CAutoLock cAutoLock(this);

	*ppSubPic = nullptr;

	CComPtr<IDirect3DSurface9> pSurface;

	if (!fStatic) {
		CAutoLock cAutoLock(&ms_SurfaceQueueLock);
		if (!m_FreeSurfaces.empty()) {
			pSurface = m_FreeSurfaces.front();
			m_FreeSurfaces.pop_front();
		}
	}

	if (!pSurface) {
		CComPtr<IDirect3DTexture9> pTexture;
		if (FAILED(m_pD3DDev->CreateTexture(m_maxsize.cx, m_maxsize.cy, 1, 0, D3DFMT_A8R8G8B8, fStatic?D3DPOOL_SYSTEMMEM:D3DPOOL_DEFAULT, &pTexture, nullptr))) {
			return false;
		}

		if (FAILED(pTexture->GetSurfaceLevel(0, &pSurface))) {
			return false;
		}
	}

	*ppSubPic = DNew CDX9SubPic(pSurface, fStatic ? 0 : this, m_bExternalRenderer);
	if (!(*ppSubPic)) {
		return false;
	}

	(*ppSubPic)->AddRef();

	if (!fStatic) {
		CAutoLock cAutoLock(&ms_SurfaceQueueLock);
		m_AllocatedSurfaces.push_front((CDX9SubPic *)*ppSubPic);
	}

	return true;
}

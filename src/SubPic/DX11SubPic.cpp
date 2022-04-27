// EXPERIMENTAL!

/*
 * (C) 2022 Ti-BEN
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
#include "DX11SubPic.h"

struct VERTEX {
	float Pos[3];
	float TexCoord[2];
};

HRESULT CreateVertexBuffer(ID3D11Device* pDevice, ID3D11Buffer** ppVertexBuffer,
	const UINT srcW, const UINT srcH, const RECT& srcRect)
{
	ASSERT(ppVertexBuffer);
	ASSERT(*ppVertexBuffer == nullptr);
	
	/*RECT srcRect;
	srcRect.left = 0;
	srcRect.right = 1920;
	srcRect.bottom = 1080;
	srcRect.top = 0;

	UINT srcW, srcH;
	srcW = 1920;
	srcH = 1080;*/
	const float src_dx = 1.0f / srcW;
	const float src_dy = 1.0f / srcH;
	float src_l = src_dx * srcRect.left;
	float src_r = src_dx * srcRect.right;
	const float src_t = src_dy * srcRect.top;
	const float src_b = src_dy * srcRect.bottom;

	POINT points[4];
	points[0] = { -1, -1 };
	points[1] = { -1, +1 };
	points[2] = { +1, -1 };
	points[3] = { +1, +1 };

	VERTEX Vertices[4] = {
		// Vertices for drawing whole texture
		// 2 ___4
		//  |\ |
		// 1|_\|3
		{ {(float)points[0].x, (float)points[0].y, 0}, {src_l, src_b} },
		{ {(float)points[1].x, (float)points[1].y, 0}, {src_l, src_t} },
		{ {(float)points[2].x, (float)points[2].y, 0}, {src_r, src_b} },
		{ {(float)points[3].x, (float)points[3].y, 0}, {src_r, src_t} },
	};

	D3D11_BUFFER_DESC BufferDesc = { sizeof(Vertices), D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
	D3D11_SUBRESOURCE_DATA InitData = { Vertices, 0, 0 };

	HRESULT hr = pDevice->CreateBuffer(&BufferDesc, &InitData, ppVertexBuffer);

	return hr;
}

//
// CDX11SubPic
//

CDX11SubPic::CDX11SubPic(ID3D11Texture2D* pSurface, CDX11SubPicAllocator *pAllocator, bool bExternalRenderer)
	: m_pTexture(pSurface), m_pAllocator(pAllocator), m_bExternalRenderer(bExternalRenderer)
{
	D3D11_TEXTURE2D_DESC d3dsd;
	d3dsd = {};
	m_pTexture->GetDesc(&d3dsd);

	if ( d3dsd.Width > 0 ) {
		m_maxsize.SetSize(d3dsd.Width, d3dsd.Height);
		m_rcDirty.SetRect(0, 0, d3dsd.Width, d3dsd.Height);
	}

	CComPtr<ID3D11Device> pD3DDev;


	m_pTexture->GetDevice(&pD3DDev);
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = d3dsd.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	if (!m_pShaderResource)
		pD3DDev->CreateShaderResourceView(m_pTexture, &srvDesc, (ID3D11ShaderResourceView**)&m_pShaderResource);
}

CDX11SubPic::~CDX11SubPic()
{
	{
		CAutoLock Lock(&CDX11SubPicAllocator::ms_SurfaceQueueLock);
		// Add surface to cache
		if (m_pAllocator) {
			for (auto it = m_pAllocator->m_AllocatedSurfaces.begin(), end = m_pAllocator->m_AllocatedSurfaces.end(); it != end; ++it) {
				if (*it == this) {
					m_pAllocator->m_AllocatedSurfaces.erase(it);
					break;
				}
			}
			m_pAllocator->m_FreeSurfaces.push_back(m_pTexture);
		}
	}
}

// ISubPic

STDMETHODIMP_(void*) CDX11SubPic::GetObject()
{
	return (void*)m_pTexture;
	/*CComPtr<IDirect3DTexture9> pTexture;
	if (SUCCEEDED(m_pTexture->GetContainer(IID_IDirect3DTexture9, (void**)&pTexture))) {
		return (void*)(IDirect3DTexture9*)pTexture;
	}

	return NULL;*/
}


STDMETHODIMP CDX11SubPic::GetDesc(SubPicDesc& spd)
{
	D3D11_TEXTURE2D_DESC d3dsd = {};
	
	m_pTexture->GetDesc(&d3dsd);
	ASSERT(d3dsd.Format == DXGI_FORMAT_B8G8R8A8_UNORM);
	if (!d3dsd.Width) {
		return E_FAIL;
	}

	spd.type = 0;
	spd.w = m_size.cx;
	spd.h = m_size.cy;
	spd.bpp = 32;
	spd.pitch = 0;
	spd.bits = nullptr;
	spd.vidrect = m_vidrect;

	return S_OK;
}

STDMETHODIMP CDX11SubPic::CopyTo(ISubPic* pSubPic)
{
	HRESULT hr;
	if (FAILED(hr = __super::CopyTo(pSubPic))) {
		return hr;
	}

	if (m_rcDirty.IsRectEmpty()) {
		return S_FALSE;
	}

	CComPtr<ID3D11Device> pD3DDev;
	CComPtr<ID3D11DeviceContext> pDeviceContext;
	
	m_pTexture->GetDevice(&pD3DDev);
	pD3DDev->GetImmediateContext(&pDeviceContext);
	if (!m_pTexture || !pD3DDev) {
		return E_FAIL;
	}

	ID3D11Texture2D* pSrcTex = (ID3D11Texture2D*)GetObject();
	
	
	D3D11_TEXTURE2D_DESC srcDesc;
	D3D11_TEXTURE2D_DESC dstDesc;
	pSrcTex->GetDesc(&srcDesc);
	
	ID3D11Texture2D* pDstTex = (ID3D11Texture2D*)pSubPic->GetObject();
	pDstTex->GetDesc(&dstDesc);
	D3D11_BOX srcBox = { 0, 0, 0, std::min(srcDesc.Width, dstDesc.Width), std::min(srcDesc.Height, dstDesc.Height), 1 };
	if (srcDesc.Width != dstDesc.Width || srcDesc.Height != dstDesc.Height)
		ASSERT(0);
	//RECT r;
	//SetRect(&r, 0, 0, std::min(srcDesc.Width, dstDesc.Width), std::min(srcDesc.Height, dstDesc.Height));
	//POINT p = { 0, 0 };
	
	pDeviceContext->CopySubresourceRegion(pDstTex, 0, 0, 0, 0, pSrcTex, 0, &srcBox);
	//hr = pD3DDev->UpdateSurface(pSrcSurf, &r, pDstSurf, &p);

	return SUCCEEDED(hr) ? S_OK : E_FAIL;
}

STDMETHODIMP CDX11SubPic::ClearDirtyRect(DWORD color)
{
	if (m_rcDirty.IsRectEmpty()) {
		return S_FALSE;
	}

	CComPtr<ID3D11Device> pD3DDev;
	CComPtr<ID3D11DeviceContext> pDeviceContext;
	//ID3D11RenderTargetView* pRenderTargetView;
	m_pTexture->GetDevice(&pD3DDev);
	D3D11_TEXTURE2D_DESC desc = {};
	m_pTexture->GetDesc(&desc);

	pD3DDev->GetImmediateContext(&pDeviceContext);
	if (!pDeviceContext) {
		return E_FAIL;
	}
	//const FLOAT ClearColorInv[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	//ID3D11Texture2D* pSrcTex = (ID3D11Texture2D*)GetObject();
	//HRESULT hr = pD3DDev->CreateRenderTargetView(pSrcTex, nullptr, &pRenderTargetView);
	//pDeviceContext->ClearRenderTargetView(pRenderTargetView, ClearColorInv);
	SubPicDesc spd;
	
#if 1
	if (SUCCEEDED(Lock(spd))) {
		ASSERT(spd.bpp == 32);
		int h = desc.Height;

		BYTE* ptr = spd.bits;
		while (h-- > 0) {
			memset_u32(ptr, color, 4 * desc.Width);
			ptr += spd.pitch;
		}
#else
	if (SUCCEEDED(Lock(spd))) {
		ASSERT(spd.bpp == 32);
		int h = m_rcDirty.Height();
		BYTE* ptr = spd.bits + spd.pitch * m_rcDirty.top + (m_rcDirty.left * 4);
		while (h-- > 0) {
			memset_u32(ptr, color, 4 * m_rcDirty.Width());
			ptr += spd.pitch;
		}
#endif
		Unlock(NULL);
	}

	//		HRESULT hr = pD3DDev->ColorFill(m_pTexture, m_rcDirty, color);

	m_rcDirty.SetRectEmpty();

	return S_OK;
}

STDMETHODIMP CDX11SubPic::Lock(SubPicDesc& spd)
{
	D3D11_TEXTURE2D_DESC d3dsd = {};
	CComPtr<ID3D11Device> pD3DDev;
	CComPtr<ID3D11DeviceContext> pDeviceContext;
	D3D11_MAPPED_SUBRESOURCE map = {};
	HRESULT hr = S_OK;

	m_pTexture->GetDesc(&d3dsd);
	ASSERT(d3dsd.Format == DXGI_FORMAT_B8G8R8A8_UNORM);
	if (!d3dsd.Width) {
		return E_FAIL;
	}
	
	m_pTexture->GetDevice(&pD3DDev);
	pD3DDev->GetImmediateContext(&pDeviceContext);
	
	hr = pDeviceContext->Map(m_pTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
	if (FAILED(hr))
		return E_FAIL;

	spd.type = 0;
	spd.w = m_size.cx;
	spd.h = m_size.cy;
	spd.bpp = 32;
	spd.pitch = map.RowPitch;
	spd.bits = (BYTE*)map.pData;
	spd.vidrect = m_vidrect;

	return S_OK;
}

STDMETHODIMP CDX11SubPic::Unlock(RECT* pDirtyRect)
{
	CComPtr<ID3D11Device> pD3DDev;
	CComPtr<ID3D11DeviceContext> pDeviceContext;
	D3D11_MAPPED_SUBRESOURCE map = {};
	HRESULT hr = S_OK;

	m_pTexture->GetDevice(&pD3DDev);
	pD3DDev->GetImmediateContext(&pDeviceContext);

	pDeviceContext->Unmap(m_pTexture, 0);

	if (pDirtyRect) {
		m_rcDirty = *pDirtyRect;
		if (!((CRect*)pDirtyRect)->IsRectEmpty()) {
			m_rcDirty.InflateRect(1, 1);
			m_rcDirty.left &= ~127;
			m_rcDirty.top &= ~63;
			m_rcDirty.right = (m_rcDirty.right + 127) & ~127;
			m_rcDirty.bottom = (m_rcDirty.bottom + 63) & ~63;
			m_rcDirty &= CRect(CPoint(0, 0), m_size);
		}
	} else {
		m_rcDirty = CRect(CPoint(0, 0), m_size);
	}

	CComPtr<ID3D11Texture2D> pTexture = (ID3D11Texture2D*)GetObject();
	//TODO don't know the equivalence in d3d11 and d3d12
	//if (pTexture && !((CRect*)pDirtyRect)->IsRectEmpty()) {
	//	pTexture->AddDirtyRect(&m_rcDirty);
	//}

	return S_OK;
}

STDMETHODIMP CDX11SubPic::AlphaBlt(RECT* pSrc, RECT* pDst, SubPicDesc* pTarget)
{

	ASSERT(pTarget == NULL);

	if (!pSrc || !pDst) {
		return E_POINTER;
	}
	CRect src(*pSrc), dst(*pDst);
	CComPtr<ID3D11Device> pD3DDev;
	CComPtr<ID3D11Texture2D> pTexture = (ID3D11Texture2D*)GetObject();
	
	CComPtr<ID3D11DeviceContext> pDeviceContext;

	pTexture->GetDevice(&pD3DDev);
	pD3DDev->GetImmediateContext(&pDeviceContext);
	
	if (!pTexture || !pD3DDev)
		return E_NOINTERFACE;


	do {
		D3D11_TEXTURE2D_DESC d3dsd = {};
		pTexture->GetDesc(&d3dsd);
		if (!(d3dsd.Width)) {
			break;
		}

		float w = (float)d3dsd.Width;
		float h = (float)d3dsd.Height;
		CRect src2;
		src2.left = 0;
		src2.top = 0;
		src2.right = w;
		src2.bottom = h;

		UINT Stride = sizeof(VERTEX);
		UINT Offset = 0;
		ID3D11Buffer* pVertexBuffer = nullptr;

		CreateVertexBuffer(pD3DDev, &pVertexBuffer, w, h, src2);
		pDeviceContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &Stride, &Offset);
		
		pDeviceContext->PSSetShaderResources(0, 1, &m_pShaderResource);
		pDeviceContext->Draw(4, 0);
		pVertexBuffer->Release();
		return S_OK;
	} while (0);
	return E_FAIL;

}

//
// CDX11SubPicAllocator
//

CDX11SubPicAllocator::CDX11SubPicAllocator(ID3D11Device1* pD3DDev, SIZE maxsize, bool bExternalRenderer)
	: CSubPicAllocatorImpl(maxsize, true)
	, m_pD3DDev(pD3DDev)
	, m_maxsize(maxsize)
	, m_bExternalRenderer(bExternalRenderer)
{
	m_pD3DDev->GetImmediateContext1(&m_pDeviceContext);
	if (!m_pDeviceContext)
		ASSERT(0);
}

CCritSec CDX11SubPicAllocator::ms_SurfaceQueueLock;

CDX11SubPicAllocator::~CDX11SubPicAllocator()
{
	ClearCache();
}

void CDX11SubPicAllocator::GetStats(int &_nFree, int &_nAlloc)
{
	CAutoLock Lock(&ms_SurfaceQueueLock);
	_nFree = (int)m_FreeSurfaces.size();
	_nAlloc = (int)m_AllocatedSurfaces.size();
}

void CDX11SubPicAllocator::ClearCache()
{
	{
		// Clear the allocator of any remaining subpics
		CAutoLock Lock(&ms_SurfaceQueueLock);
		for (auto& pSubPic : m_AllocatedSurfaces) {
			pSubPic->m_pAllocator = NULL;
		}
		m_AllocatedSurfaces.clear();
		m_FreeSurfaces.clear();
	}
}

// ISubPicAllocator

STDMETHODIMP CDX11SubPicAllocator::ChangeDevice(IUnknown* pDev)
{
	ClearCache();
	CComQIPtr<ID3D11Device1> pD3DDev = pDev;
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

STDMETHODIMP CDX11SubPicAllocator::SetMaxTextureSize(SIZE MaxTextureSize)
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

bool CDX11SubPicAllocator::Alloc(bool fStatic, ISubPic** ppSubPic)
{
	if (!ppSubPic) {
		return false;
	}

	CAutoLock cAutoLock(this);

	*ppSubPic = NULL;

	CComPtr<ID3D11Texture2D> pTexture;

	if (!fStatic) {
		CAutoLock cAutoLock(&ms_SurfaceQueueLock);
		if (!m_FreeSurfaces.empty()) {
			pTexture = m_FreeSurfaces.front();
			m_FreeSurfaces.pop_front();
		}
	}

	if (!pTexture) {


		D3D11_TEXTURE2D_DESC texdesc {};
		texdesc.Usage = D3D11_USAGE_DYNAMIC;
		texdesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		texdesc.MiscFlags = 0;
		texdesc.Width = m_maxsize.cx;
		texdesc.Height = m_maxsize.cy;
		texdesc.MipLevels = 1;
		texdesc.ArraySize = 1;
		texdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			;//verify its the good one
		texdesc.SampleDesc = { 1, 0 };
		HRESULT hr = m_pD3DDev->CreateTexture2D(&texdesc, nullptr, &pTexture);
		if (FAILED(hr))
		{
			return false;
		}

	}

	*ppSubPic = DNew CDX11SubPic(pTexture, fStatic ? 0 : this, m_bExternalRenderer);
	if (!(*ppSubPic)) {
		return false;
	}

	(*ppSubPic)->AddRef();

	if (!fStatic) {
		CAutoLock cAutoLock(&ms_SurfaceQueueLock);
		m_AllocatedSurfaces.push_front((CDX11SubPic *)*ppSubPic);
	}

	return true;
}

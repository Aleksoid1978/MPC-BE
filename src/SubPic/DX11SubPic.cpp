// EXPERIMENTAL!

/*
 * (C) 2022 see Authors.txt
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
#include "mpc_defines.h"
#include "DSUtil/Utils.h"
#include "DX11SubPic.h"
#include <DirectXMath.h>

#define ENABLE_DUMP_SUBPIC 0

struct VERTEX {
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT2 TexCoord;
};

static HRESULT CreateVertexBuffer(ID3D11Device* pDevice, ID3D11Buffer** ppVertexBuffer,
	const UINT srcW, const UINT srcH, const RECT& srcRect)
{
	ASSERT(ppVertexBuffer);
	ASSERT(*ppVertexBuffer == nullptr);
	const float src_dx = 1.0f / srcW;
	const float src_dy = 1.0f / srcH;
	const float src_l = src_dx * srcRect.left;
	const float src_r = src_dx * srcRect.right;
	const float src_t = src_dy * srcRect.top;
	const float src_b = src_dy * srcRect.bottom;

	POINT points[4] = {
		{ -1, -1 },
		{ -1, +1 },
		{ +1, -1 },
		{ +1, +1 }
	};

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
	DLogIf(FAILED(hr), L"CDX11SubPic : CreateVertexBuffer() failed with error %s", HR2Str(hr));

	return hr;
}

#if _DEBUG & ENABLE_DUMP_SUBPIC
static HRESULT SaveToBMP(BYTE* src, const UINT src_pitch, const UINT width, const UINT height, const UINT bitdepth, const wchar_t* filename)
{
	if (!src || !filename) {
		return E_POINTER;
	}

	if (!src_pitch || !width || !height || bitdepth != 32) {
		return E_ABORT;
	}

	const UINT dst_pitch = width * bitdepth / 8;
	const UINT len = dst_pitch * height;
	ASSERT(dst_pitch <= src_pitch);

	BITMAPFILEHEADER bfh;
	bfh.bfType      = 0x4d42;
	bfh.bfOffBits   = sizeof(bfh) + sizeof(BITMAPINFOHEADER);
	bfh.bfSize      = bfh.bfOffBits + len;
	bfh.bfReserved1 = bfh.bfReserved2 = 0;

	BITMAPINFOHEADER bih = {};
	bih.biSize      = sizeof(BITMAPINFOHEADER);
	bih.biWidth     = width;
	bih.biHeight    = -(LONG)height;
	bih.biBitCount  = bitdepth;
	bih.biPlanes    = 1;
	bih.biSizeImage = DIBSIZE(bih);

	ASSERT(len == bih.biSizeImage);

	FILE* fp;
	if (_wfopen_s(&fp, filename, L"wb") == 0 && fp) {
		fwrite(&bfh, sizeof(bfh), 1, fp);
		fwrite(&bih, sizeof(bih), 1, fp);

		if (dst_pitch == src_pitch) {
			fwrite(src, len, 1, fp);
		}
		else if (dst_pitch < src_pitch) {
			for (UINT y = 0; y < height; ++y) {
				fwrite(src, dst_pitch, 1, fp);
				src += src_pitch;
			}
		}
		fclose(fp);

		return S_OK;
	}

	return E_FAIL;
}

static HRESULT DumpTexture2D(ID3D11DeviceContext* pDeviceContext, ID3D11Texture2D* pTexture2D, const wchar_t* filename)
{
	D3D11_TEXTURE2D_DESC desc;
	pTexture2D->GetDesc(&desc);

	if (desc.Format != DXGI_FORMAT_B8G8R8X8_UNORM && desc.Format != DXGI_FORMAT_B8G8R8A8_UNORM) {
		return E_INVALIDARG;
	}

	HRESULT hr = S_OK;
	CComPtr<ID3D11Texture2D> pTexture2DShared;

	if (desc.CPUAccessFlags & D3D11_CPU_ACCESS_READ) {
		pTexture2DShared = pTexture2D;
	}
	else {
		ID3D11Device* pDevice;
		pTexture2D->GetDevice(&pDevice);

		D3D11_TEXTURE2D_DESC desc2;
		desc2.Width = desc.Width;
		desc2.Height = desc.Height;
		desc2.MipLevels = 1;
		desc2.ArraySize = 1;
		desc2.Format = desc.Format;
		desc2.SampleDesc = { 1, 0 };
		desc2.Usage = D3D11_USAGE_STAGING;
		desc2.BindFlags = 0;
		desc2.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		desc2.MiscFlags = 0;
		hr = pDevice->CreateTexture2D(&desc2, nullptr, &pTexture2DShared);
		pDevice->Release();

		//pDeviceContext->CopyResource(pTexture2DShared, pTexture2D);
		pDeviceContext->CopySubresourceRegion(pTexture2DShared, 0, 0, 0, 0, pTexture2D, 0, nullptr);
	}

	if (SUCCEEDED(hr)) {
		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		hr = pDeviceContext->Map(pTexture2DShared, 0, D3D11_MAP_READ, 0, &mappedResource);

		if (SUCCEEDED(hr)) {
			hr = SaveToBMP((BYTE*)mappedResource.pData, mappedResource.RowPitch, desc.Width, desc.Height, 32, filename);

			pDeviceContext->Unmap(pTexture2DShared, 0);
		}
	}

	return hr;
}
#endif

//
// CDX11SubPic
//

CDX11SubPic::CDX11SubPic(MemPic_t&& pMemPic, CDX11SubPicAllocator *pAllocator, bool bExternalRenderer)
	: m_MemPic(std::move(pMemPic))
	, m_pAllocator(pAllocator)
	, m_bExternalRenderer(bExternalRenderer)
{
	m_maxsize.SetSize(m_MemPic.w, m_MemPic.h);
	m_rcDirty.SetRect(0, 0, m_maxsize.cx, m_maxsize.cy);
}

CDX11SubPic::~CDX11SubPic()
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
		m_pAllocator->m_FreeSurfaces.push_back(std::move(m_MemPic));
	}
}

// ISubPic

STDMETHODIMP_(void*) CDX11SubPic::GetObject()
{
	return reinterpret_cast<void*>(&m_MemPic);
}

STDMETHODIMP CDX11SubPic::GetDesc(SubPicDesc& spd)
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

STDMETHODIMP CDX11SubPic::CopyTo(ISubPic* pSubPic)
{
	HRESULT hr = __super::CopyTo(pSubPic);
	if (FAILED(hr)) {
		return hr;
	}

	if (m_rcDirty.IsRectEmpty()) {
		return S_FALSE;
	}

	auto pDstMemPic = reinterpret_cast<MemPic_t*>(pSubPic->GetObject());

	CRect copyRect;
	RECT subpicRect = { 0, 0, pDstMemPic->w, pDstMemPic->h };
	if (!copyRect.IntersectRect(m_rcDirty, &subpicRect)) {
		return S_FALSE;
	}

	const UINT copyW_bytes = copyRect.Width() * 4;
	UINT copyH = copyRect.Height();
	auto src = m_MemPic.data.get() + m_MemPic.w * copyRect.top + copyRect.left;
	auto dst = pDstMemPic->data.get() + pDstMemPic->w * copyRect.top + copyRect.left;

	while (copyH--) {
		memcpy(dst, src, copyW_bytes);
		src += m_MemPic.w;
		dst += pDstMemPic->w;
	}

	return S_OK;
}

STDMETHODIMP CDX11SubPic::ClearDirtyRect(DWORD color)
{
	if (m_rcDirty.IsRectEmpty()) {
		return S_FALSE;
	}

	const UINT dirtyW_bytes = m_rcDirty.Width() * 4;
	UINT dirtyH = m_rcDirty.Height();
	uint32_t* ptr = m_MemPic.data.get() + m_MemPic.w * m_rcDirty.top + m_rcDirty.left;

	while (dirtyH-- > 0) {
		memset_u32(ptr, color, dirtyW_bytes);
		ptr += m_MemPic.w;
	}

	m_rcDirty.SetRectEmpty();

	return S_OK;
}

STDMETHODIMP CDX11SubPic::Lock(SubPicDesc& spd)
{
	if (!m_MemPic.data) {
		return E_FAIL;
	}

	spd.type    = 0;
	spd.w       = m_size.cx;
	spd.h       = m_size.cy;
	spd.bpp     = 32;
	spd.pitch   = m_MemPic.w * 4;
	spd.bits    = (BYTE*)m_MemPic.data.get();
	spd.vidrect = m_vidrect;

	return S_OK;
}

STDMETHODIMP CDX11SubPic::Unlock(RECT* pDirtyRect)
{
	if (pDirtyRect) {
		m_rcDirty = *pDirtyRect;
		if (!m_rcDirty.IsRectEmpty()) {
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

	return S_OK;
}

STDMETHODIMP CDX11SubPic::AlphaBlt(RECT* pSrc, RECT* pDst, SubPicDesc* pTarget)
{
	ASSERT(pTarget == nullptr);

	if (!pSrc || !pDst) {
		return E_POINTER;
	}
	CRect rSrc(*pSrc), rDst(*pDst);

	auto pTexture = m_pAllocator->GetOutputTexture();
	if (!pTexture) {
		return E_FAIL;
	}

	D3D11_TEXTURE2D_DESC texDesc = {};
	pTexture->GetDesc(&texDesc);

	CComPtr<ID3D11Device> pDevice;
	CComPtr<ID3D11DeviceContext> pDeviceContext;
	pTexture->GetDevice(&pDevice);
	pDevice->GetImmediateContext(&pDeviceContext);

	uint32_t* srcData = m_MemPic.data.get() + m_MemPic.w * m_rcDirty.top + m_rcDirty.left;
	D3D11_BOX dstBox = { m_rcDirty.left, m_rcDirty.top, 0, m_rcDirty.right, m_rcDirty.bottom, 1 };

	pDeviceContext->UpdateSubresource(pTexture, 0, &dstBox, srcData, m_MemPic.w * 4, 0);

	D3D11_VIEWPORT vp;
	vp.TopLeftX = rDst.left;
	vp.TopLeftY = rDst.top;
	vp.Width    = rDst.Width();
	vp.Height   = rDst.Height();
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	pDeviceContext->RSSetViewports(1, &vp);

	UINT Stride = sizeof(VERTEX);
	UINT Offset = 0;
	ID3D11Buffer* pVertexBuffer = nullptr;
	auto hr = CreateVertexBuffer(pDevice, &pVertexBuffer, texDesc.Width, texDesc.Height, rSrc);
	if (FAILED(hr)) {
		return E_FAIL;
	}
	pDeviceContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &Stride, &Offset);
	pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	auto pShaderResource = m_pAllocator->GetShaderResource();
	pDeviceContext->PSSetShaderResources(0, 1, &pShaderResource);

	pDeviceContext->Draw(4, 0);

	pVertexBuffer->Release();

#if _DEBUG & ENABLE_DUMP_SUBPIC
	{
		static int counter = 0;
		CString filepath;
		filepath.Format(L"C:\\Temp\\subpictex%04d.bmp", counter++);
		DumpTexture2D(pDeviceContext, pTexture, filepath);
	}
#endif

	return S_OK;
}

//
// CDX11SubPicAllocator
//

CDX11SubPicAllocator::CDX11SubPicAllocator(ID3D11Device* pDevice, SIZE maxsize, bool bExternalRenderer)
	: CSubPicAllocatorImpl(maxsize, true)
	, m_pDevice(pDevice)
	, m_maxsize(maxsize)
	, m_bExternalRenderer(bExternalRenderer)
{
}

CCritSec CDX11SubPicAllocator::ms_SurfaceQueueLock;

CDX11SubPicAllocator::~CDX11SubPicAllocator()
{
	ClearCache();
}

void CDX11SubPicAllocator::GetStats(int& _nFree, int& _nAlloc)
{
	CAutoLock Lock(&ms_SurfaceQueueLock);
	_nFree = (int)m_FreeSurfaces.size();
	_nAlloc = (int)m_AllocatedSurfaces.size();
}

void CDX11SubPicAllocator::ClearCache()
{
	// Clear the allocator of any remaining subpics
	CAutoLock Lock(&ms_SurfaceQueueLock);
	for (auto& pSubPic : m_AllocatedSurfaces) {
		pSubPic->m_pAllocator = nullptr;
	}
	m_AllocatedSurfaces.clear();
	m_FreeSurfaces.clear();

	m_pOutputShaderResource.Release();
	m_pOutputTexture.Release();
	m_pOutputRowPitch = 0;
}

// ISubPicAllocator

STDMETHODIMP CDX11SubPicAllocator::ChangeDevice(IUnknown* pDev)
{
	ClearCache();
	CComQIPtr<ID3D11Device> pDevice = pDev;
	if (!pDevice) {
		return E_NOINTERFACE;
	}

	CAutoLock cAutoLock(this);
	HRESULT hr = S_FALSE;
	if (m_pDevice != pDevice) {
		ClearCache();
		m_pDevice = pDevice;
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

bool CDX11SubPicAllocator::CreateOutputTex()
{
	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Usage          = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags      = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags      = 0;
	texDesc.Width          = m_maxsize.cx;
	texDesc.Height         = m_maxsize.cy;
	texDesc.MipLevels      = 1;
	texDesc.ArraySize      = 1;
	texDesc.Format         = DXGI_FORMAT_B8G8R8A8_UNORM;
	texDesc.SampleDesc     = { 1, 0 };

	HRESULT hr = m_pDevice->CreateTexture2D(&texDesc, nullptr, &m_pOutputTexture);
	if (FAILED(hr)) {
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	hr = m_pDevice->CreateShaderResourceView(m_pOutputTexture, &srvDesc, &m_pOutputShaderResource);
	if (FAILED(hr)) {
		m_pOutputTexture.Release();
		return false;
	}

	texDesc.Usage          = D3D11_USAGE_DYNAMIC;
	texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	CComPtr<ID3D11Texture2D> pTestTexture;

	hr = m_pDevice->CreateTexture2D(&texDesc, nullptr, &pTestTexture);
	if (SUCCEEDED(hr)) {
		CComPtr<ID3D11DeviceContext> pDeviceContext;
		m_pDevice->GetImmediateContext(&pDeviceContext);
		D3D11_MAPPED_SUBRESOURCE mr = {};

		hr = pDeviceContext->Map(pTestTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mr);
		if (SUCCEEDED(hr)) {
			m_pOutputRowPitch = mr.RowPitch;
			pDeviceContext->Unmap(pTestTexture, 0);
		}
	}

	if (!m_pOutputRowPitch) {
		m_pOutputTexture.Release();
		m_pOutputShaderResource.Release();
		return false;
	}

	return true;
}

// ISubPicAllocatorImpl

bool CDX11SubPicAllocator::Alloc(bool fStatic, ISubPic** ppSubPic)
{
	if (!ppSubPic) {
		return false;
	}

	CAutoLock cAutoLock(this);

	*ppSubPic = nullptr;

	MemPic_t pMemPic;

	if (!fStatic) {
		CAutoLock cAutoLock(&ms_SurfaceQueueLock);
		if (!m_FreeSurfaces.empty()) {
			pMemPic = std::move(m_FreeSurfaces.front());
			m_FreeSurfaces.pop_front();
		}
	}

	if (!pMemPic.data) {
		if (!m_pOutputShaderResource) {
			bool texOk = CreateOutputTex();
			if (!texOk) {
				return false;
			}
		}

		pMemPic.w = m_pOutputRowPitch / 4;
		pMemPic.h = m_maxsize.cy;

		const UINT picSize = pMemPic.w * pMemPic.h;
		auto data = new(std::nothrow) uint32_t[picSize];
		if (!data) {
			return false;
		}

		pMemPic.data.reset(data);
	}

	*ppSubPic = DNew CDX11SubPic(std::move(pMemPic), fStatic ? 0 : this, m_bExternalRenderer);
	if (!(*ppSubPic)) {
		return false;
	}

	(*ppSubPic)->AddRef();

	if (!fStatic) {
		CAutoLock cAutoLock(&ms_SurfaceQueueLock);
		m_AllocatedSurfaces.push_front((CDX11SubPic*)*ppSubPic);
	}

	return true;
}

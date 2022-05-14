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

static const BYTE vertex_shader[] =
{
	 68,  88,  66,  67, 119,  76,
	129,  53, 139, 143, 201, 108,
	 78,  31,  90,  10,  57, 206,
	  5,  93,   1,   0,   0,   0,
	 24,   2,   0,   0,   5,   0,
	  0,   0,  52,   0,   0,   0,
	128,   0,   0,   0, 212,   0,
	  0,   0,  44,   1,   0,   0,
	156,   1,   0,   0,  82,  68,
	 69,  70,  68,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	 28,   0,   0,   0,   0,   4,
	254, 255,   0,   1,   0,   0,
	 28,   0,   0,   0,  77, 105,
	 99, 114, 111, 115, 111, 102,
	116,  32,  40,  82,  41,  32,
	 72,  76,  83,  76,  32,  83,
	104,  97, 100, 101, 114,  32,
	 67, 111, 109, 112, 105, 108,
	101, 114,  32,  49,  48,  46,
	 49,   0,  73,  83,  71,  78,
	 76,   0,   0,   0,   2,   0,
	  0,   0,   8,   0,   0,   0,
	 56,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  3,   0,   0,   0,   0,   0,
	  0,   0,  15,  15,   0,   0,
	 65,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  3,   0,   0,   0,   1,   0,
	  0,   0,   3,   3,   0,   0,
	 80,  79,  83,  73,  84,  73,
	 79,  78,   0,  84,  69,  88,
	 67,  79,  79,  82,  68,   0,
	171, 171,  79,  83,  71,  78,
	 80,   0,   0,   0,   2,   0,
	  0,   0,   8,   0,   0,   0,
	 56,   0,   0,   0,   0,   0,
	  0,   0,   1,   0,   0,   0,
	  3,   0,   0,   0,   0,   0,
	  0,   0,  15,   0,   0,   0,
	 68,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  3,   0,   0,   0,   1,   0,
	  0,   0,   3,  12,   0,   0,
	 83,  86,  95,  80,  79,  83,
	 73,  84,  73,  79,  78,   0,
	 84,  69,  88,  67,  79,  79,
	 82,  68,   0, 171, 171, 171,
	 83,  72,  68,  82, 104,   0,
	  0,   0,  64,   0,   1,   0,
	 26,   0,   0,   0,  95,   0,
	  0,   3, 242,  16,  16,   0,
	  0,   0,   0,   0,  95,   0,
	  0,   3,  50,  16,  16,   0,
	  1,   0,   0,   0, 103,   0,
	  0,   4, 242,  32,  16,   0,
	  0,   0,   0,   0,   1,   0,
	  0,   0, 101,   0,   0,   3,
	 50,  32,  16,   0,   1,   0,
	  0,   0,  54,   0,   0,   5,
	242,  32,  16,   0,   0,   0,
	  0,   0,  70,  30,  16,   0,
	  0,   0,   0,   0,  54,   0,
	  0,   5,  50,  32,  16,   0,
	  1,   0,   0,   0,  70,  16,
	 16,   0,   1,   0,   0,   0,
	 62,   0,   0,   1,  83,  84,
	 65,  84, 116,   0,   0,   0,
	  3,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  4,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   1,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   2,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0
};

static const BYTE pixel_shader[] =
{
	 68,  88,  66,  67,  32,  70,
	217,  59,  23, 217, 111,  68,
	220, 166, 168, 235,  83,   9,
	237, 244,   1,   0,   0,   0,
	 64,   2,   0,   0,   5,   0,
	  0,   0,  52,   0,   0,   0,
	204,   0,   0,   0,  36,   1,
	  0,   0,  88,   1,   0,   0,
	196,   1,   0,   0,  82,  68,
	 69,  70, 144,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   2,   0,   0,   0,
	 28,   0,   0,   0,   0,   4,
	255, 255,   0,   1,   0,   0,
	101,   0,   0,   0,  92,   0,
	  0,   0,   3,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   1,   0,
	  0,   0,   1,   0,   0,   0,
	 97,   0,   0,   0,   2,   0,
	  0,   0,   5,   0,   0,   0,
	  4,   0,   0,   0, 255, 255,
	255, 255,   0,   0,   0,   0,
	  1,   0,   0,   0,  13,   0,
	  0,   0, 115,  97, 109, 112,
	  0, 116, 101, 120,   0,  77,
	105,  99, 114, 111, 115, 111,
	102, 116,  32,  40,  82,  41,
	 32,  72,  76,  83,  76,  32,
	 83, 104,  97, 100, 101, 114,
	 32,  67, 111, 109, 112, 105,
	108, 101, 114,  32,  49,  48,
	 46,  49,   0, 171, 171, 171,
	 73,  83,  71,  78,  80,   0,
	  0,   0,   2,   0,   0,   0,
	  8,   0,   0,   0,  56,   0,
	  0,   0,   0,   0,   0,   0,
	  1,   0,   0,   0,   3,   0,
	  0,   0,   0,   0,   0,   0,
	 15,   0,   0,   0,  68,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   3,   0,
	  0,   0,   1,   0,   0,   0,
	  3,   3,   0,   0,  83,  86,
	 95,  80,  79,  83,  73,  84,
	 73,  79,  78,   0,  84,  69,
	 88,  67,  79,  79,  82,  68,
	  0, 171, 171, 171,  79,  83,
	 71,  78,  44,   0,   0,   0,
	  1,   0,   0,   0,   8,   0,
	  0,   0,  32,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   3,   0,   0,   0,
	  0,   0,   0,   0,  15,   0,
	  0,   0,  83,  86,  95,  84,
	 97, 114, 103, 101, 116,   0,
	171, 171,  83,  72,  68,  82,
	100,   0,   0,   0,  64,   0,
	  0,   0,  25,   0,   0,   0,
	 90,   0,   0,   3,   0,  96,
	 16,   0,   0,   0,   0,   0,
	 88,  24,   0,   4,   0, 112,
	 16,   0,   0,   0,   0,   0,
	 85,  85,   0,   0,  98,  16,
	  0,   3,  50,  16,  16,   0,
	  1,   0,   0,   0, 101,   0,
	  0,   3, 242,  32,  16,   0,
	  0,   0,   0,   0,  69,   0,
	  0,   9, 242,  32,  16,   0,
	  0,   0,   0,   0,  70,  16,
	 16,   0,   1,   0,   0,   0,
	 70, 126,  16,   0,   0,   0,
	  0,   0,   0,  96,  16,   0,
	  0,   0,   0,   0,  62,   0,
	  0,   1,  83,  84,  65,  84,
	116,   0,   0,   0,   2,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   2,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   1,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  1,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0
};

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

CDX11SubPic::CDX11SubPic(MemPic_t&& pMemPic, CDX11SubPicAllocator *pAllocator)
	: m_MemPic(std::move(pMemPic))
	, m_pAllocator(pAllocator)
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

	CRect copyRect(m_rcDirty);
	copyRect.InflateRect(1, 1);
	RECT subpicRect = { 0, 0, pDstMemPic->w, pDstMemPic->h };
	if (!copyRect.IntersectRect(copyRect, &subpicRect)) {
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

STDMETHODIMP CDX11SubPic::ClearDirtyRect()
{
	if (m_rcDirty.IsRectEmpty()) {
		return S_FALSE;
	}

	m_rcDirty.InflateRect(1, 1);
#ifdef _WIN64
	const LONG a = 16 / sizeof(uint32_t) - 1;
	m_rcDirty.left &= ~a;
	m_rcDirty.right = (m_rcDirty.right + a) & ~a;
#endif
	m_rcDirty.IntersectRect(m_rcDirty, CRect(0, 0, m_MemPic.w, m_MemPic.h));

	uint32_t* ptr = m_MemPic.data.get() + m_MemPic.w * m_rcDirty.top + m_rcDirty.left;
	const UINT dirtyW = m_rcDirty.Width();
	UINT dirtyH = m_rcDirty.Height();

	while (dirtyH-- > 0) {
		fill_u32(ptr, m_bInvAlpha ? 0x00000000 : 0xFF000000, dirtyW);
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
		m_rcDirty.IntersectRect(pDirtyRect, CRect(0, 0, m_size.cx, m_size.cy));
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

	return m_pAllocator->Render(m_MemPic, m_rcDirty, rSrc, rDst);
}

//
// CDX11SubPicAllocator
//

CDX11SubPicAllocator::CDX11SubPicAllocator(ID3D11Device* pDevice, SIZE maxsize)
	: CSubPicAllocatorImpl(maxsize, true)
	, m_pDevice(pDevice)
	, m_maxsize(maxsize)
{
	CreateBlendState();
	CreateOtherStates();
}

CDX11SubPicAllocator::~CDX11SubPicAllocator()
{
	ReleaseAllStates();
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
		ReleaseAllStates();
		ClearCache();

		m_pDevice = pDevice;
		hr = __super::ChangeDevice(pDev);

		CreateBlendState();
		CreateOtherStates();
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

STDMETHODIMP_(void) CDX11SubPicAllocator::SetInverseAlpha(bool bInverted)
{
	if (m_bInvAlpha != bInverted) {
		m_bInvAlpha = bInverted;
		m_pAlphaBlendState.Release();
		CreateBlendState();
	}
}

HRESULT CDX11SubPicAllocator::CreateOutputTex()
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
		return hr;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	hr = m_pDevice->CreateShaderResourceView(m_pOutputTexture, &srvDesc, &m_pOutputShaderResource);
	if (FAILED(hr)) {
		m_pOutputTexture.Release();
	}

	return hr;
}

void CDX11SubPicAllocator::CreateBlendState()
{
	D3D11_BLEND_DESC bdesc = {};
	bdesc.RenderTarget[0].BlendEnable = TRUE;
	bdesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	bdesc.RenderTarget[0].DestBlend = m_bInvAlpha ? D3D11_BLEND_INV_SRC_ALPHA : D3D11_BLEND_SRC_ALPHA;
	bdesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bdesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bdesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	bdesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bdesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	EXECUTE_ASSERT(S_OK == m_pDevice->CreateBlendState(&bdesc, &m_pAlphaBlendState));
}

void CDX11SubPicAllocator::CreateOtherStates()
{
	D3D11_BUFFER_DESC BufferDesc = { sizeof(VERTEX) * 4, D3D11_USAGE_DYNAMIC, D3D11_BIND_VERTEX_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, 0 };
	EXECUTE_ASSERT(S_OK == m_pDevice->CreateBuffer(&BufferDesc, nullptr, &m_pVertexBuffer));

	D3D11_SAMPLER_DESC SampDesc = {};
	SampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	SampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	SampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	SampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	SampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	SampDesc.MinLOD = 0;
	SampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	EXECUTE_ASSERT(S_OK == m_pDevice->CreateSamplerState(&SampDesc, &m_pSamplerPoint));

	SampDesc.Filter = D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT; // linear interpolation for magnification
	EXECUTE_ASSERT(S_OK == m_pDevice->CreateSamplerState(&SampDesc, &m_pSamplerLinear));

	D3D11_INPUT_ELEMENT_DESC Layout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	EXECUTE_ASSERT(S_OK == m_pDevice->CreateInputLayout(Layout, std::size(Layout), vertex_shader, sizeof(vertex_shader), &m_pInputLayout));
	EXECUTE_ASSERT(S_OK == m_pDevice->CreateVertexShader(vertex_shader, sizeof(vertex_shader), nullptr, &m_pVertexShader));
	EXECUTE_ASSERT(S_OK == m_pDevice->CreatePixelShader(pixel_shader, sizeof(pixel_shader), nullptr, &m_pPixelShader));
}

void CDX11SubPicAllocator::ReleaseAllStates()
{
	m_pAlphaBlendState.Release();
	m_pVertexBuffer.Release();
	m_pSamplerPoint.Release();
	m_pSamplerLinear.Release();
	m_pInputLayout.Release();
	m_pVertexShader.Release();
	m_pPixelShader.Release();
}

HRESULT CDX11SubPicAllocator::Render(const MemPic_t& memPic, const CRect& dirtyRect, const CRect& srcRect, const CRect& dstRect)
{
	HRESULT hr = S_OK;

	if (!m_pOutputTexture) {
		hr = CreateOutputTex();
		if (FAILED(hr)) {
			return hr;
		}
	}

	bool stretching = (srcRect.Size() != dstRect.Size());

	CRect copyRect(dirtyRect);
	if (stretching) {
		copyRect.InflateRect(1, 1);
		RECT subpicRect = { 0, 0, memPic.w, memPic.h };
		EXECUTE_ASSERT(copyRect.IntersectRect(copyRect, &subpicRect));
	}

	CComPtr<ID3D11DeviceContext> pDeviceContext;
	m_pDevice->GetImmediateContext(&pDeviceContext);

	uint32_t* srcData = memPic.data.get() + memPic.w * copyRect.top + copyRect.left;
	D3D11_BOX dstBox = { copyRect.left, copyRect.top, 0, copyRect.right, copyRect.bottom, 1 };

	pDeviceContext->UpdateSubresource(m_pOutputTexture, 0, &dstBox, srcData, memPic.w * 4, 0);

	D3D11_TEXTURE2D_DESC texDesc = {};
	m_pOutputTexture->GetDesc(&texDesc);

	const float src_dx = 1.0f / texDesc.Width;
	const float src_dy = 1.0f / texDesc.Height;
	const float src_l = src_dx * srcRect.left;
	const float src_r = src_dx * srcRect.right;
	const float src_t = src_dy * srcRect.top;
	const float src_b = src_dy * srcRect.bottom;

	const POINT points[4] = {
		{ -1, -1 },
		{ -1, +1 },
		{ +1, -1 },
		{ +1, +1 }
	};

	const VERTEX Vertices[4] = {
		// Vertices for drawing whole texture
		// 2 ___4
		//  |\ |
		// 1|_\|3
		{ {(float)points[0].x, (float)points[0].y, 0}, {src_l, src_b} },
		{ {(float)points[1].x, (float)points[1].y, 0}, {src_l, src_t} },
		{ {(float)points[2].x, (float)points[2].y, 0}, {src_r, src_b} },
		{ {(float)points[3].x, (float)points[3].y, 0}, {src_r, src_t} },
	};

	D3D11_MAPPED_SUBRESOURCE mr;
	hr = pDeviceContext->Map(m_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mr);
	if (FAILED(hr)) {
		return hr;
	}

	memcpy(mr.pData, &Vertices, sizeof(Vertices));
	pDeviceContext->Unmap(m_pVertexBuffer, 0);

	UINT Stride = sizeof(VERTEX);
	UINT Offset = 0;
	pDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer.p, &Stride, &Offset);
	pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	pDeviceContext->IASetInputLayout(m_pInputLayout);

	pDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);

	pDeviceContext->PSSetSamplers(0, 1, &(stretching ? m_pSamplerLinear.p : m_pSamplerPoint.p));
	pDeviceContext->PSSetShaderResources(0, 1, &m_pOutputShaderResource.p);
	pDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);

	pDeviceContext->OMSetBlendState(m_pAlphaBlendState, nullptr, D3D11_DEFAULT_SAMPLE_MASK);

	D3D11_VIEWPORT vp;
	vp.TopLeftX = dstRect.left;
	vp.TopLeftY = dstRect.top;
	vp.Width    = dstRect.Width();
	vp.Height   = dstRect.Height();
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	pDeviceContext->RSSetViewports(1, &vp);

	pDeviceContext->Draw(4, 0);

#if _DEBUG & ENABLE_DUMP_SUBPIC
	{
		static int counter = 0;
		CString filepath;
		filepath.Format(L"C:\\Temp\\subpictex%04d.bmp", counter++);
		DumpTexture2D(pDeviceContext, m_pOutputTexture, filepath);
	}
#endif

	return hr;
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
		pMemPic.w = ALIGN(m_maxsize.cx, 16/sizeof(uint32_t));
		pMemPic.h = m_maxsize.cy;

		const UINT picSize = pMemPic.w * pMemPic.h;
		auto data = new(std::nothrow) uint32_t[picSize];
		if (!data) {
			return false;
		}

		pMemPic.data.reset(data);
	}

	*ppSubPic = DNew CDX11SubPic(std::move(pMemPic), fStatic ? 0 : this);
	if (!(*ppSubPic)) {
		return false;
	}

	(*ppSubPic)->AddRef();
	(*ppSubPic)->SetInverseAlpha(m_bInvAlpha);

	if (!fStatic) {
		CAutoLock cAutoLock(&ms_SurfaceQueueLock);
		m_AllocatedSurfaces.push_front((CDX11SubPic*)*ppSubPic);
	}

	return true;
}

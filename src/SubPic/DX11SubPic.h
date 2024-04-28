/*
 * (C) 2022-2024 see Authors.txt
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

#include "SubPicImpl.h"

// CDX11SubPic


class CDX11SubPicAllocator;

struct MemPic_t {
	std::unique_ptr<uint32_t[]> data;
	UINT w = 0;
	UINT h = 0;
};

class CDX11SubPic : public CSubPicImpl
{
	MemPic_t m_MemPic;

protected:
	STDMETHODIMP_(void*) GetObject() override; // returns MemPic_t*

public:
	CDX11SubPicAllocator *m_pAllocator;

	CDX11SubPic(MemPic_t&& pMemPic, CDX11SubPicAllocator *pAllocator);
	~CDX11SubPic();

	// ISubPic
	STDMETHODIMP GetDesc(SubPicDesc& spd) override;
	STDMETHODIMP CopyTo(ISubPic* pSubPic) override;
	STDMETHODIMP ClearDirtyRect() override;
	STDMETHODIMP Lock(SubPicDesc& spd) override;
	STDMETHODIMP Unlock(RECT* pDirtyRect) override;
	STDMETHODIMP AlphaBlt(RECT* pSrc, RECT* pDst, SubPicDesc* pTarget) override;
	STDMETHODIMP_(bool) IsNeedAlloc() override;
};

// CDX11SubPicAllocator

class CDX11SubPicAllocator : public CSubPicAllocatorImpl, public CCritSec
{
	CComPtr<ID3D11Device> m_pDevice;
	CSize m_maxsize;

	CComPtr<ID3D11Texture2D> m_pOutputTexture;
	CComPtr<ID3D11ShaderResourceView> m_pOutputShaderResource;
	CComPtr<ID3D11BlendState> m_pAlphaBlendState;
	CComPtr<ID3D11Buffer> m_pVertexBuffer;
	CComPtr<ID3D11SamplerState> m_pSamplerPoint;
	CComPtr<ID3D11SamplerState> m_pSamplerLinear;

	bool Alloc(bool fStatic, ISubPic** ppSubPic) override;

	HRESULT CreateOutputTex();
	void CreateBlendState();
	void CreateOtherStates();
	void ReleaseAllStates();

public:
	inline static CCritSec ms_SurfaceQueueLock;
	std::deque<MemPic_t> m_FreeSurfaces;
	std::deque<CDX11SubPic*> m_AllocatedSurfaces;

	HRESULT Render(const MemPic_t& memPic, const CRect& dirtyRect, const CRect& srcRect, const CRect& dstRect);

	void GetStats(int& _nFree, int& _nAlloc);

	CDX11SubPicAllocator(ID3D11Device* pDevice, SIZE maxsize);
	~CDX11SubPicAllocator();
	void ClearCache();

	// ISubPicAllocator
	STDMETHODIMP ChangeDevice(IUnknown* pDev) override;
	STDMETHODIMP SetMaxTextureSize(SIZE MaxTextureSize) override;
	STDMETHODIMP_(void) SetInverseAlpha(bool bInverted) override;
};

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

#pragma once

#include "SubPicImpl.h"

// CDX11SubPic


class CDX11SubPicAllocator;

struct MemPic_t {
	std::unique_ptr<uint32_t> data;
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
	STDMETHODIMP ClearDirtyRect(DWORD color) override;
	STDMETHODIMP Lock(SubPicDesc& spd) override;
	STDMETHODIMP Unlock(RECT* pDirtyRect) override;
	STDMETHODIMP AlphaBlt(RECT* pSrc, RECT* pDst, SubPicDesc* pTarget) override;
};

// CDX11SubPicAllocator

class CDX11SubPicAllocator : public CSubPicAllocatorImpl, public CCritSec
{
	CComPtr<ID3D11Device> m_pDevice;
	CSize m_maxsize;

	CComPtr<ID3D11Texture2D> m_pOutputTexture;
	CComPtr<ID3D11ShaderResourceView> m_pOutputShaderResource;

	bool Alloc(bool fStatic, ISubPic** ppSubPic) override;

	bool CreateOutputTex();

public:
	static CCritSec ms_SurfaceQueueLock;
	std::list<MemPic_t> m_FreeSurfaces;
	std::list<CDX11SubPic*> m_AllocatedSurfaces;

	ID3D11Texture2D* GetOutputTexture() { return m_pOutputTexture.p; }
	ID3D11ShaderResourceView* GetShaderResource() { return m_pOutputShaderResource.p; }

	void GetStats(int& _nFree, int& _nAlloc);

	CDX11SubPicAllocator(ID3D11Device* pDevice, SIZE maxsize);
	~CDX11SubPicAllocator();
	void ClearCache();

	// ISubPicAllocator
	STDMETHODIMP ChangeDevice(IUnknown* pDev) override;
	STDMETHODIMP SetMaxTextureSize(SIZE MaxTextureSize) override;
};

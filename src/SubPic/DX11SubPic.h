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

class CDX11SubPic : public CSubPicImpl
{
	CComPtr<ID3D11Texture2D> m_pTexture;
	CComPtr<ID3D11ShaderResourceView> m_pShaderResource;

protected:
	STDMETHODIMP_(void*) GetObject(); // returns ID3D11Texture2D*

public:
	CDX11SubPicAllocator *m_pAllocator;
	bool m_bExternalRenderer;
	CDX11SubPic(ID3D11Texture2D* pTexture, CDX11SubPicAllocator *pAllocator, bool bExternalRenderer);
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
	CComPtr<ID3D11Device> m_pD3DDev;
	CComPtr<ID3D11DeviceContext> m_pDeviceContext;
	CSize m_maxsize;
	bool m_bExternalRenderer;

	bool Alloc(bool fStatic, ISubPic** ppSubPic) override;

public:
	static CCritSec ms_SurfaceQueueLock;
	std::list<CComPtr<ID3D11Texture2D> > m_FreeSurfaces;
	std::list<CDX11SubPic*> m_AllocatedSurfaces;

	void GetStats(int &_nFree, int &_nAlloc);

	CDX11SubPicAllocator(ID3D11Device* pD3DDev, SIZE maxsize, bool bExternalRenderer);
	~CDX11SubPicAllocator();
	void ClearCache();

	// ISubPicAllocator
	STDMETHODIMP ChangeDevice(IUnknown* pDev) override;
	STDMETHODIMP SetMaxTextureSize(SIZE MaxTextureSize) override;
};

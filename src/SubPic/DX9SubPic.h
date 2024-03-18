/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2024 see Authors.txt
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

#include "SubPicImpl.h"

// CDX9SubPic


class CDX9SubPicAllocator;
class CDX9SubPic : public CSubPicImpl
{
	CComPtr<IDirect3DSurface9> m_pSurface;
	const bool m_bExternalRenderer;

public:
	CDX9SubPicAllocator *m_pAllocator;

	CDX9SubPic(IDirect3DSurface9* pSurface, CDX9SubPicAllocator *pAllocator, bool bExternalRenderer);
	~CDX9SubPic();

	// ISubPic
protected:
	STDMETHODIMP_(void*) GetObject() override; // returns IDirect3DTexture9*
public:
	STDMETHODIMP GetDesc(SubPicDesc& spd) override;
	STDMETHODIMP CopyTo(ISubPic* pSubPic) override;
	STDMETHODIMP ClearDirtyRect() override;
	STDMETHODIMP Lock(SubPicDesc& spd) override;
	STDMETHODIMP Unlock(RECT* pDirtyRect) override;
	STDMETHODIMP AlphaBlt(RECT* pSrc, RECT* pDst, SubPicDesc* pTarget) override;
};

// CDX9SubPicAllocator

class CDX9SubPicAllocator : public CSubPicAllocatorImpl, public CCritSec
{
	CComPtr<IDirect3DDevice9> m_pDevice;
	CSize m_maxsize;
	const bool m_bExternalRenderer;

	// CSubPicAllocatorImpl
	bool Alloc(bool fStatic, ISubPic** ppSubPic) override;

public:
	inline static CCritSec ms_SurfaceQueueLock;
	std::deque<CComPtr<IDirect3DSurface9> > m_FreeSurfaces;
	std::deque<CDX9SubPic*> m_AllocatedSurfaces;

	CDX9SubPicAllocator(IDirect3DDevice9* pDevice, SIZE maxsize, bool bExternalRenderer);
	~CDX9SubPicAllocator();

	void GetStats(int &_nFree, int &_nAlloc);
	void ClearCache();

	// ISubPicAllocator
	STDMETHODIMP ChangeDevice(IUnknown* pDev) override;
	STDMETHODIMP SetMaxTextureSize(SIZE MaxTextureSize) override;
};

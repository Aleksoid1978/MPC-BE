/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2016 see Authors.txt
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
#include <d3d9.h>

// CDX9SubPic


class CDX9SubPicAllocator;
class CDX9SubPic : public CSubPicImpl
{
	CComPtr<IDirect3DSurface9> m_pSurface;

protected:
	STDMETHODIMP_(void*) GetObject(); // returns IDirect3DTexture9*

public:
	CDX9SubPicAllocator *m_pAllocator;
	bool m_bExternalRenderer;
	CDX9SubPic(IDirect3DSurface9* pSurface, CDX9SubPicAllocator *pAllocator, bool bExternalRenderer);
	~CDX9SubPic();

	// ISubPic
	STDMETHODIMP GetDesc(SubPicDesc& spd);
	STDMETHODIMP CopyTo(ISubPic* pSubPic);
	STDMETHODIMP ClearDirtyRect(DWORD color);
	STDMETHODIMP Lock(SubPicDesc& spd);
	STDMETHODIMP Unlock(RECT* pDirtyRect);
	STDMETHODIMP AlphaBlt(RECT* pSrc, RECT* pDst, SubPicDesc* pTarget);
};

// CDX9SubPicAllocator

class CDX9SubPicAllocator : public CSubPicAllocatorImpl, public CCritSec
{
	CComPtr<IDirect3DDevice9> m_pD3DDev;
	CSize m_maxsize;
	bool m_bExternalRenderer;

	bool Alloc(bool fStatic, ISubPic** ppSubPic);

public:
	static CCritSec ms_SurfaceQueueLock;
	CAtlList<CComPtr<IDirect3DSurface9> > m_FreeSurfaces;
	CAtlList<CDX9SubPic *> m_AllocatedSurfaces;

	void GetStats(int &_nFree, int &_nAlloc);

	CDX9SubPicAllocator(IDirect3DDevice9* pD3DDev, SIZE maxsize, bool bExternalRenderer);
	~CDX9SubPicAllocator();
	void ClearCache();

	// ISubPicAllocator
	STDMETHODIMP ChangeDevice(IUnknown* pDev);
	STDMETHODIMP SetMaxTextureSize(SIZE MaxTextureSize);
};

/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2026 see Authors.txt
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

enum {
	MSP_RGB32 = 0,
	MSP_RGB24,
	MSP_RGB16,
	MSP_RGB15,
	MSP_YUY2,
	MSP_YV12,
	MSP_IYUV,
	MSP_AYUV,
	MSP_RGBA,        //pre-multiplied alpha. Use A*g + RGB to mix
	MSP_RGBA_F,      //pre-multiplied alpha. Use (0xff-A)*g + RGB to mix
	MSP_AYUV_PLANAR, //AYUV in planar form
	MSP_XY_AUYV,
	MSP_P010,
	MSP_P016,
	MSP_NV12,
	MSP_NV21,
	MSP_P210, // 4:2:2 10 bits
	MSP_P216, // 4:2:2 16 bits
	MSP_YV16, // 4:2:2 8 bits
	MSP_YV24  // 4:4:4 8 bits
};

// CMemSubPic

// only RGB32 is supported
class CMemSubPic : public CSubPicImpl
{
protected:
	SubPicDesc m_spd;

public:
	CMemSubPic(SubPicDesc& spd);
	virtual ~CMemSubPic();

	// ISubPic
protected:
	STDMETHODIMP_(void*) GetObject() override; // returns SubPicDesc*
public:
	STDMETHODIMP GetDesc(SubPicDesc& spd) override;
	STDMETHODIMP CopyTo(ISubPic* pSubPic) override;
	STDMETHODIMP ClearDirtyRect() override;
	STDMETHODIMP Lock(SubPicDesc& spd) override;
	STDMETHODIMP Unlock(RECT* pDirtyRect) override;
	STDMETHODIMP AlphaBlt(RECT* pSrc, RECT* pDst, SubPicDesc* pTarget) override;
};

// CMemSubPicAllocator

// only RGB32 is supported
class CMemSubPicAllocator : public CSubPicAllocatorImpl
{
protected:
	CSize m_maxsize;

	// CSubPicAllocatorImpl
	bool Alloc(bool fStatic, ISubPic** ppSubPic) override;

public:
	CMemSubPicAllocator(SIZE maxsize);
};

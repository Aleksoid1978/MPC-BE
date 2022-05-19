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

#pragma once

#include "SubPicImpl.h"

// CMemSubPic

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

class CMemSubPicAllocator : public CSubPicAllocatorImpl
{
protected:
	const int m_type;
	CSize m_maxsize;

	// CSubPicAllocatorImpl
	bool Alloc(bool fStatic, ISubPic** ppSubPic) override;

public:
	CMemSubPicAllocator(int type, SIZE maxsize);
};

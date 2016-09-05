/*
 * (C) 2006-2014 see Authors.txt
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

#include "../SubPic/SubPicProviderImpl.h"
#include "CompositionObject.h"

class __declspec(uuid("86551353-62BE-4513-B14D-B5882A73823E"))
	CXSUBSubtitle : public CSubPicProviderImpl, public ISubStream
{
public:
	CXSUBSubtitle(CCritSec* pLock, const CString& name, LCID lcid, SIZE size);
	~CXSUBSubtitle(void);

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// ISubPicProvider
	STDMETHODIMP_(POSITION) GetStartPosition(REFERENCE_TIME rt, double fps, bool CleanOld = false);
	STDMETHODIMP_(POSITION) GetNext(POSITION pos);
	STDMETHODIMP_(REFERENCE_TIME) GetStart(POSITION pos, double fps);
	STDMETHODIMP_(REFERENCE_TIME) GetStop(POSITION pos, double fps);
	STDMETHODIMP_(bool) IsAnimated(POSITION pos);
	STDMETHODIMP Render(SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox);
	STDMETHODIMP GetTextureSize (POSITION pos, SIZE& MaxTextureSize, SIZE& VirtualSize, POINT& VirtualTopLeft);

	STDMETHODIMP_(SUBTITLE_TYPE) GetType() { return ST_XSUB; };

	// IPersist
	STDMETHODIMP GetClassID(CLSID* pClassID);

	// ISubStream
	STDMETHODIMP_(int) GetStreamCount();
	STDMETHODIMP GetStreamInfo(int i, WCHAR** ppName, LCID* pLCID);
	STDMETHODIMP_(int) GetStream();
	STDMETHODIMP SetStream(int iStream);
	STDMETHODIMP Reload();

	void	Reset();
	void	CleanOld(REFERENCE_TIME rt);

	HRESULT	ParseSample(BYTE* pData, long nLen);
	HRESULT	NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

private :
	CString			m_name;
	LCID			m_lcid;
	SIZE			m_size;

	CAtlList<CompositionObject*>	m_pObjects;

	CCritSec		m_csCritSec;
};

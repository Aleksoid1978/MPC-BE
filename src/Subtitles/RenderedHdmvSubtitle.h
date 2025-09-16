/*
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

#include "SubPic/SubPicProviderImpl.h"
#include "HdmvSub.h"
#include "BaseSub.h"

class __declspec(uuid("FCA68599-C83E-4ea5-94A3-C2E1B0E326B9"))
	CRenderedHdmvSubtitle : public CSubPicProviderImpl, public ISubStream
{
public:
	CRenderedHdmvSubtitle(CCritSec* pLock, SUBTITLE_TYPE nType, const CString& name, LCID lcid);
	CRenderedHdmvSubtitle(CCritSec* pLock);
	~CRenderedHdmvSubtitle(void);

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// ISubPicProvider
	STDMETHODIMP_(POSITION) GetStartPosition(REFERENCE_TIME rt, double fps, bool CleanOld = false);
	STDMETHODIMP_(POSITION) GetNext(POSITION pos);
	STDMETHODIMP_(REFERENCE_TIME) GetStart(POSITION pos, double fps);
	STDMETHODIMP_(REFERENCE_TIME) GetStop(POSITION pos, double fps);
	STDMETHODIMP_(bool) IsAnimated(POSITION pos);
	STDMETHODIMP Render(SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox);
	STDMETHODIMP GetTextureSize(POSITION pos, SIZE& MaxTextureSize, SIZE& VirtualSize, POINT& VirtualTopLeft);

	STDMETHODIMP_(SUBTITLE_TYPE) GetType() { return m_nType; };

	// IPersist
	STDMETHODIMP GetClassID(CLSID* pClassID);

	// ISubStream
	STDMETHODIMP_(int) GetStreamCount();
	STDMETHODIMP GetStreamInfo(int i, WCHAR** ppName, LCID* pLCID);
	STDMETHODIMP_(int) GetStream();
	STDMETHODIMP SetStream(int iStream);
	STDMETHODIMP Reload();
	STDMETHODIMP SetSourceTargetInfo(LPCWSTR yuvMatrix, LPCWSTR inputRange, LPCWSTR outpuRange);

	HRESULT ParseSample(BYTE* pData, long nLen, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);
	HRESULT	NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	HRESULT	EndOfStream();

	bool Open(const CString& fn, const CString& name, const CString& videoName);

private :
	CString			m_name;
	LCID			m_lcid;

	CBaseSub*		m_pSub;
	CCritSec		m_csCritSec;

	SUBTITLE_TYPE	m_nType;

	static const WORD PGS_SYNC_CODE = 'PG';

	bool m_bStopParsing = false;
	std::thread m_parsingThread;

	bool m_bLoadFromFile = false;

	void ParseFile(const CString& fn);
};

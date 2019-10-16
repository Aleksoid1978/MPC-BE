/*
 * (C) 2006-2019 see Authors.txt
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

#include "stdafx.h"
#include <array>
#include <vector>
#include "../DSUtil/GolombBuffer.h"
#include "HdmvSub.h"
#include "DVBSub.h"
#include "SubtitleHelpers.h"
#include "RenderedHdmvSubtitle.h"

CRenderedHdmvSubtitle::CRenderedHdmvSubtitle(CCritSec* pLock, SUBTITLE_TYPE nType, const CString& name, LCID lcid)
	: CSubPicProviderImpl(pLock)
	, m_name(name)
	, m_lcid(lcid)
	, m_nType(nType)
{
	switch (m_nType) {
		case ST_DVB :
			m_pSub = DNew CDVBSub();
			if (name.IsEmpty() || (name == L"Unknown")) m_name = L"DVB";
			break;
		case ST_HDMV :
			m_pSub = DNew CHdmvSub();
			if (name.IsEmpty() || (name == L"Unknown")) m_name = L"PGS";
			break;
		default :
			ASSERT(FALSE);
			m_pSub = nullptr;
	}
}

CRenderedHdmvSubtitle::CRenderedHdmvSubtitle(CCritSec* pLock)
	: CSubPicProviderImpl(pLock)
	, m_lcid(0)
	, m_nType(ST_HDMV)
	, m_pSub(nullptr)
{
}

CRenderedHdmvSubtitle::~CRenderedHdmvSubtitle(void)
{
	m_bStopParsing = true;
	if (m_parsingThread.joinable()) {
		m_parsingThread.join();
	}

	SAFE_DELETE(m_pSub);
}

STDMETHODIMP CRenderedHdmvSubtitle::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);
	*ppv = NULL;

	return
		QI(IPersist)
		QI(ISubStream)
		QI(ISubPicProvider)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// ISubPicProvider

STDMETHODIMP_(POSITION) CRenderedHdmvSubtitle::GetStartPosition(REFERENCE_TIME rt, double fps, bool CleanOld)
{
	CAutoLock cAutoLock(&m_csCritSec);

	return m_pSub->GetStartPosition(rt, fps, CleanOld);
}

STDMETHODIMP_(POSITION) CRenderedHdmvSubtitle::GetNext(POSITION pos)
{
	CAutoLock cAutoLock(&m_csCritSec);

	return m_pSub->GetNext (pos);
}

STDMETHODIMP_(REFERENCE_TIME) CRenderedHdmvSubtitle::GetStart(POSITION pos, double fps)
{
	CAutoLock cAutoLock(&m_csCritSec);

	return m_pSub->GetStart(pos);
}

STDMETHODIMP_(REFERENCE_TIME) CRenderedHdmvSubtitle::GetStop(POSITION pos, double fps)
{
	CAutoLock cAutoLock(&m_csCritSec);

	return m_pSub->GetStop(pos);
}

STDMETHODIMP_(bool) CRenderedHdmvSubtitle::IsAnimated(POSITION pos)
{
	return false;
}

STDMETHODIMP CRenderedHdmvSubtitle::Render(SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox)
{
	CAutoLock cAutoLock(&m_csCritSec);

	const HRESULT hr = m_pSub->Render(spd, rt, bbox);
	m_pSub->CleanOld(rt - 60*10000000i64); // Cleanup subtitles older than 1 minute ...

	return hr;
}

STDMETHODIMP CRenderedHdmvSubtitle::GetTextureSize(POSITION pos, SIZE& MaxTextureSize, SIZE& VideoSize, POINT& VideoTopLeft)
{
	CAutoLock cAutoLock(&m_csCritSec);

	HRESULT hr = m_pSub->GetTextureSize(pos, MaxTextureSize, VideoSize, VideoTopLeft);
	return hr;
};

// IPersist

STDMETHODIMP CRenderedHdmvSubtitle::GetClassID(CLSID* pClassID)
{
	return pClassID ? *pClassID = __uuidof(this), S_OK : E_POINTER;
}

// ISubStream

STDMETHODIMP_(int) CRenderedHdmvSubtitle::GetStreamCount()
{
	return 1;
}

STDMETHODIMP CRenderedHdmvSubtitle::GetStreamInfo(int iStream, WCHAR** ppName, LCID* pLCID)
{
	if (iStream != 0) {
		return E_INVALIDARG;
	}

	if (ppName) {
		*ppName = (WCHAR*)CoTaskMemAlloc((m_name.GetLength() + 1) * sizeof(WCHAR));
		if (!(*ppName)) {
			return E_OUTOFMEMORY;
		}

		wcscpy_s(*ppName, m_name.GetLength() + 1, CStringW(m_name));
	}

	if (pLCID) {
		*pLCID = m_lcid;
	}

	return S_OK;
}

STDMETHODIMP_(int) CRenderedHdmvSubtitle::GetStream()
{
	return 0;
}

STDMETHODIMP CRenderedHdmvSubtitle::SetStream(int iStream)
{
	return iStream == 0 ? S_OK : E_FAIL;
}

STDMETHODIMP CRenderedHdmvSubtitle::Reload()
{
	return S_OK;
}

STDMETHODIMP CRenderedHdmvSubtitle::SetSourceTargetInfo(CString yuvMatrix, CString inputRange, CString outpuRange)
{
	ColorConvert::convertType convertType = ColorConvert::convertType::DEFAULT;
	if (inputRange == L"TV" && outpuRange == L"TV") {
		convertType = ColorConvert::convertType::TV_2_TV;
	} else if (inputRange == L"PC" && outpuRange == L"PC") {
		convertType = ColorConvert::convertType::PC_2_PC;
	} else if (inputRange == L"TV" && outpuRange == L"PC") {
		convertType = ColorConvert::convertType::TV_2_PC;
	} else if (inputRange == L"PC" && outpuRange == L"TV") {
		convertType = ColorConvert::convertType::PC_2_TV;
	}

	return m_pSub->SetConvertType(yuvMatrix, convertType);
}

HRESULT CRenderedHdmvSubtitle::ParseSample(BYTE* pData, long nLen, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	CAutoLock cAutoLock(&m_csCritSec);

	HRESULT hr = m_pSub->ParseSample(pData, nLen, rtStart, rtStop);
	return hr;
}

HRESULT CRenderedHdmvSubtitle::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csCritSec);

	m_pSub->Reset();
	return S_OK;
}

HRESULT CRenderedHdmvSubtitle::EndOfStream()
{
	CAutoLock cAutoLock(&m_csCritSec);

	return m_pSub->EndOfStream();
}

bool CRenderedHdmvSubtitle::Open(const CString& fn, const CString& name, const CString& videoName)
{
	CFile f;
	if (!f.Open(fn, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone)) {
		return false;
	}

	WORD wSyncCode = 0;
	f.Read(&wSyncCode, sizeof(wSyncCode));
	if (_byteswap_ushort(wSyncCode) != PGS_SYNC_CODE) {
		return false;
	}

	if (name.IsEmpty()) {
		m_name = Subtitle::GuessSubtitleName(fn, videoName);
	} else {
		m_name = name;
	}
	m_pSub = DNew CHdmvSub();

	m_parsingThread = std::thread([this, fn] { ParseFile(fn); });

	return true;
}

void CRenderedHdmvSubtitle::ParseFile(const CString& fn)
{
	CFile f;
	if (!f.Open(fn, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone)) {
		return;
	}

	// Header: Sync code | start time | stop time | segment type | segment size
	std::array<BYTE, 2 + 2 * 4 + 1 + 2> header;
	const unsigned nExtraSize = 1 + 2; // segment type + segment size
	std::vector<BYTE> segBuff;

	while (!m_bStopParsing && f.Read(header.data(), header.size()) == header.size()) {
		// Parse the header
		CGolombBuffer headerBuffer(header.data(), (int)header.size());

		if (WORD(headerBuffer.ReadShort()) != PGS_SYNC_CODE) {
			break;
		}

		const REFERENCE_TIME rtStart = REFERENCE_TIME(headerBuffer.ReadDword()) * 1000 / 9;
		const REFERENCE_TIME rtStop = REFERENCE_TIME(headerBuffer.ReadDword()) * 1000 / 9;
		headerBuffer.ReadByte(); // segment type
		const WORD wLenSegment = (WORD)headerBuffer.ReadShort();

		// Leave some room to add the segment type and size
		unsigned nLenData = nExtraSize + wLenSegment;
		if (nLenData > segBuff.size()) {
			segBuff.resize(nLenData);
		}
		memcpy(segBuff.data(), &header[header.size() - nExtraSize], nExtraSize);

		// Read the segment
		if (wLenSegment && f.Read(&segBuff[nExtraSize], wLenSegment) != wLenSegment) {
			break;
		}

		// Parse the data (even if the segment size is 0 because the header itself is important)
		m_pSub->ParseSample(segBuff.data(), nLenData, rtStart, rtStop);
	}
}

/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2025 see Authors.txt
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
#include "DSMMuxer.h"
#include "DSUtil/DSUtil.h"
#include "DSUtil/std_helper.h"
#include <qnetwork.h>
#include <moreuuids.h>

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_DirectShowMedia}
};

const AMOVIESETUP_PIN sudpPins[] = {
	{(LPWSTR)L"Input", FALSE, FALSE, FALSE, TRUE, &CLSID_NULL, nullptr, 0, nullptr},
	{(LPWSTR)L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, std::size(sudPinTypesOut), sudPinTypesOut}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CDSMMuxerFilter), DSMMuxerName, MERIT_DO_NOT_USE, std::size(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory}
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CDSMMuxerFilter>, nullptr, &sudFilter[0]}
};

int g_cTemplates = std::size(g_Templates);

STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}

#include "filters/filters/Filters.h"

CFilterApp theApp;

#endif

template<typename T> static T myabs(T n)
{
	return n >= 0 ? n : -n;
}

static int GetByteLength(UINT64 data, int min = 0)
{
	int i = 7;
	while (i >= min && ((BYTE*)&data)[i] == 0) {
		i--;
	}
	return ++i;
}

//
// CDSMMuxerFilter
//

CDSMMuxerFilter::CDSMMuxerFilter(LPUNKNOWN pUnk, HRESULT* phr, bool fAutoChap, bool fAutoRes)
	: CBaseMuxerFilter(pUnk, phr, __uuidof(this))
	, m_fAutoChap(fAutoChap)
	, m_fAutoRes(fAutoRes)
	, m_rtPrevSyncPoint(INVALID_TIME)
{
	if (phr) {
		*phr = S_OK;
	}
}

CDSMMuxerFilter::~CDSMMuxerFilter()
{
}

STDMETHODIMP CDSMMuxerFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	*ppv = nullptr;

	return
		__super::NonDelegatingQueryInterface(riid, ppv);
}

void CDSMMuxerFilter::MuxPacketHeader(IBitStream* pBS, dsmp_t type, UINT64 len)
{
	ASSERT(type < 32);

	int i = GetByteLength(len, 1);

	pBS->BitWrite(DSMSW, DSMSW_SIZE << 3);
	pBS->BitWrite(type, 5);
	pBS->BitWrite(i - 1, 3);
	pBS->BitWrite(len, i << 3);
}

void CDSMMuxerFilter::MuxFileInfo(IBitStream* pBS)
{
	int len = 1;
	std::list<CStringA> entries;

	for (const auto&[key, value] : m_properties) {
		if (key.length() != 4 || value.IsEmpty()) {
			continue;
		}
		entries.emplace_back(CStringA(key.c_str(), key.length()) + WStrToUTF8(value));
		len += 4 + value.GetLength() + 1;
	}

	MuxPacketHeader(pBS, DSMP_FILEINFO, len);
	pBS->BitWrite(DSMF_VERSION, 8);
	for (const auto& entry : entries) {
		pBS->ByteWrite((LPCSTR)entry, entry.GetLength() + 1);
	}
}

void CDSMMuxerFilter::MuxStreamInfo(IBitStream* pBS, CBaseMuxerInputPin* pPin)
{
	int len = 1;
	std::list<CStringA> entries;

	ULONG cProperties = 0;
	HRESULT hr = pPin->CountProperties(&cProperties);
	if (SUCCEEDED(hr) && cProperties > 0) {
		for (ULONG iProperty = 0; iProperty < cProperties; iProperty++) {
			PROPBAG2 PropBag = {};
			ULONG cPropertiesReturned = 0;
			hr = pPin->GetPropertyInfo(iProperty, 1, &PropBag, &cPropertiesReturned);
			if (FAILED(hr)) {
				continue;
			}

			HRESULT hr2;
			CComVariant var;
			hr = pPin->Read(1, &PropBag, nullptr, &var, &hr2);
			if (SUCCEEDED(hr) && SUCCEEDED(hr2) && (var.vt & (VT_BSTR | VT_BYREF)) == VT_BSTR) {
				const CStringA key(PropBag.pstrName);
				const CStringA value = WStrToUTF8(var.bstrVal);
				if (key.GetLength() != 4 || value.IsEmpty()) {
					continue;
				}
				entries.emplace_back(key + value);
				len += 4 + value.GetLength() + 1;
			}
			CoTaskMemFree(PropBag.pstrName);
		}
	}

	if (len > 1) {
		MuxPacketHeader(pBS, DSMP_STREAMINFO, len);
		pBS->BitWrite(pPin->GetID(), 8);
		for (const auto& entry : entries) {
			pBS->ByteWrite((LPCSTR)entry, entry.GetLength() + 1);
		}
	}
}

void CDSMMuxerFilter::MuxInit()
{
	m_sps.clear();
	m_isps.clear();
	m_rtPrevSyncPoint = INVALID_TIME;
}

void CDSMMuxerFilter::MuxHeader(IBitStream* pBS)
{
	CStringW muxer;
	muxer.Format(L"DSM Muxer (%S)", __TIMESTAMP__);

	SetProperty(L"MUXR", muxer);
	SetProperty(L"DATE", CTime::GetCurrentTime().FormatGmt(L"%Y-%m-%d %H:%M:%S"));

	MuxFileInfo(pBS);

	for (const auto& pPin : m_pPins) {
		const CMediaType& mt = pPin->CurrentMediaType();

		ASSERT((mt.lSampleSize >> 30) == 0); // you don't need >1GB samples, do you?

		MuxPacketHeader(pBS, DSMP_MEDIATYPE, 5 + sizeof(GUID) * 3 + mt.FormatLength());
		pBS->BitWrite(pPin->GetID(), 8);
		pBS->ByteWrite(&mt.majortype, sizeof(mt.majortype));
		pBS->ByteWrite(&mt.subtype, sizeof(mt.subtype));
		pBS->BitWrite(mt.bFixedSizeSamples, 1);
		pBS->BitWrite(mt.bTemporalCompression, 1);
		pBS->BitWrite(mt.lSampleSize, 30);
		pBS->ByteWrite(&mt.formattype, sizeof(mt.formattype));
		pBS->ByteWrite(mt.Format(), mt.FormatLength());

		MuxStreamInfo(pBS, pPin);
	}

	// resources & chapters

	std::list<CComQIPtr<IDSMResourceBag>> pRBs;
	pRBs.emplace_back(this);

	CComQIPtr<IDSMChapterBag> pCB = (IUnknown*)(INonDelegatingUnknown*)this;

	for (const auto& p : m_pPins) {
		for (CComPtr<IPin> pPin = p->GetConnected(); pPin; pPin = GetUpStreamPin(GetFilterFromPin(pPin))) {
			if (m_fAutoRes) {
				CComQIPtr<IDSMResourceBag> pPB = GetFilterFromPin(pPin);
				if (pPB && !Contains(pRBs, pPB)) {
					pRBs.emplace_back(pPB);
				}
			}

			if (m_fAutoChap) {
				if (!pCB || pCB->ChapGetCount() == 0) {
					pCB = GetFilterFromPin(pPin);
				}
			}
		}
	}

	// resources

	for (const auto& pRB : pRBs) {
		for (DWORD i = 0, j = pRB->ResGetCount(); i < j; i++) {
			CComBSTR name, desc, mime;
			BYTE* pData = nullptr;
			DWORD len = 0;
			if (SUCCEEDED(pRB->ResGet(i, &name, &desc, &mime, &pData, &len, nullptr))) {
				CStringA utf8_name = WStrToUTF8(name);
				CStringA utf8_desc = WStrToUTF8(desc);
				CStringA utf8_mime = WStrToUTF8(mime);

				MuxPacketHeader(pBS, DSMP_RESOURCE,
								1 +
								utf8_name.GetLength() + 1 +
								utf8_desc.GetLength() + 1 +
								utf8_mime.GetLength() + 1 +
								len);

				pBS->BitWrite(0, 2);
				pBS->BitWrite(0, 6); // reserved
				pBS->ByteWrite(utf8_name, utf8_name.GetLength() + 1);
				pBS->ByteWrite(utf8_desc, utf8_desc.GetLength() + 1);
				pBS->ByteWrite(utf8_mime, utf8_mime.GetLength() + 1);
				pBS->ByteWrite(pData, len);

				CoTaskMemFree(pData);
			}
		}
	}

	// chapters

	if (pCB) {
		pCB->ChapSort();

		std::vector<CDSMChapter> chapters;
		chapters.reserve(pCB->ChapGetCount());
		REFERENCE_TIME rtPrev = 0;
		int len = 0;

		for (DWORD i = 0; i < pCB->ChapGetCount(); i++) {
			CDSMChapter c;
			CComBSTR name;
			if (SUCCEEDED(pCB->ChapGet(i, &c.rt, &name))) {
				REFERENCE_TIME rtDiff = c.rt - rtPrev;
				rtPrev = c.rt;
				c.rt = rtDiff;
				c.name = name;
				len += 1 + GetByteLength(myabs(c.rt)) + WStrToUTF8(c.name).GetLength() + 1;
				chapters.push_back(c);
			}
		}

		if (chapters.size()) {
			MuxPacketHeader(pBS, DSMP_CHAPTERS, len);

			for (const auto& c : chapters) {
				CStringA name = WStrToUTF8(c.name);
				int irt = GetByteLength(myabs(c.rt));
				pBS->BitWrite(c.rt < 0, 1);
				pBS->BitWrite(irt, 3);
				pBS->BitWrite(0, 4);
				pBS->BitWrite(myabs(c.rt), irt << 3);
				pBS->ByteWrite((LPCSTR)name, name.GetLength() + 1);
			}
		}
	}
}

void CDSMMuxerFilter::MuxPacket(IBitStream* pBS, const MuxerPacket* pPacket)
{
	if (pPacket->IsEOS()) {
		return;
	}

	if (pPacket->pPin->CurrentMediaType().majortype == MEDIATYPE_Text) {
		CStringA str((char*)pPacket->data.get(), (int)pPacket->size);
		str.Replace("\xff", " ");
		str.Replace("&nbsp;", " ");
		str.Replace("&nbsp", " ");
		str.Trim();
		if (str.IsEmpty()) {
			return;
		}
	}

	ASSERT(!pPacket->IsSyncPoint() || pPacket->IsTimeValid());

	REFERENCE_TIME rtTimeStamp = INVALID_TIME, rtDuration = 0;
	int iTimeStamp = 0, iDuration = 0;

	if (pPacket->IsTimeValid()) {
		rtTimeStamp = pPacket->rtStart;
		rtDuration = std::max(pPacket->rtStop - pPacket->rtStart, 0LL);

		iTimeStamp = GetByteLength(myabs(rtTimeStamp));
		ASSERT(iTimeStamp <= 7);

		iDuration = GetByteLength(rtDuration);
		ASSERT(iDuration <= 7);

		IndexSyncPoint(pPacket, pBS->GetPos());
	}

	UINT64 len = 2 + iTimeStamp + iDuration + pPacket->size; // id + flags + data

	MuxPacketHeader(pBS, DSMP_SAMPLE, len);
	pBS->BitWrite(pPacket->pPin->GetID(), 8);
	pBS->BitWrite(pPacket->IsSyncPoint(), 1);
	pBS->BitWrite(rtTimeStamp < 0, 1);
	pBS->BitWrite(iTimeStamp, 3);
	pBS->BitWrite(iDuration, 3);
	pBS->BitWrite(myabs(rtTimeStamp), iTimeStamp << 3);
	pBS->BitWrite(rtDuration, iDuration << 3);
	pBS->ByteWrite(pPacket->data.get(), (int)pPacket->size);
}

void CDSMMuxerFilter::MuxFooter(IBitStream* pBS)
{
	// syncpoints

	int len = 0;
	REFERENCE_TIME rtPrev = 0, rt;
	UINT64 fpPrev = 0, fp;

	std::vector<IndexedSyncPoint> isps;
	isps.reserve(m_isps.size());

	for (const auto& isp : m_isps) {
		TRACE(L"sp[%d]: %I64d %I64x\n", isp.id, isp.rt, isp.fp);

		rt = isp.rt - rtPrev;
		rtPrev = isp.rt;
		fp = isp.fp - fpPrev;
		fpPrev = isp.fp;

		IndexedSyncPoint isp2;
		isp2.fp = fp;
		isp2.rt = rt;
		isps.push_back(isp2);

		len += 1 + GetByteLength(myabs(rt)) + GetByteLength(fp); // flags + rt + fp
	}

	MuxPacketHeader(pBS, DSMP_SYNCPOINTS, len);

	for (const auto& isp : isps) {
		int irt = GetByteLength(myabs(isp.rt));
		int ifp = GetByteLength(isp.fp);

		pBS->BitWrite(isp.rt < 0, 1);
		pBS->BitWrite(irt, 3);
		pBS->BitWrite(ifp, 3);
		pBS->BitWrite(0, 1); // reserved
		pBS->BitWrite(myabs(isp.rt), irt << 3);
		pBS->BitWrite(isp.fp, ifp << 3);
	}
}

void CDSMMuxerFilter::IndexSyncPoint(const MuxerPacket* p, __int64 fp)
{
	// Yes, this is as complicated as it looks.
	// Rule #1: don't write this packet if you can't do it reliably.
	// (think about overlapped subtitles, line1: 0->10, line2: 1->9)

	// FIXME: the very last syncpoints won't get moved to m_isps because there are no more syncpoints to trigger it!

	if (fp < 0 || !p || !p->IsTimeValid() || !p->IsSyncPoint()) {
		return;
	}

	ASSERT(p->rtStart >= m_rtPrevSyncPoint);
	m_rtPrevSyncPoint = p->rtStart;

	SyncPoint sp;
	sp.id = (BYTE)p->pPin->GetID();
	sp.rtStart = p->rtStart;
	sp.rtStop = p->pPin->IsSubtitleStream() ? p->rtStop : _I64_MAX;
	sp.fp = fp;

	{
		SyncPoint& head = !m_sps.empty() ? m_sps.front() : sp;
		SyncPoint& tail = !m_sps.empty() ? m_sps.back() : sp;
		REFERENCE_TIME rtfp = !m_isps.empty() ? m_isps.back().rtfp : INVALID_TIME;

		if (head.rtStart > rtfp + 1000000) { // 100ms limit, just in case every stream had only keyframes, then sycnpoints would be too frequent
			IndexedSyncPoint isp;
			isp.id = head.id;
			isp.rt = tail.rtStart;
			isp.rtfp = head.rtStart;
			isp.fp = head.fp;
			m_isps.push_back(isp);
		}
	}

	auto it = m_sps.begin();
	while (it != m_sps.end()) {
		SyncPoint& sp2 = *it;
		if (sp2.id == sp.id && sp2.rtStop <= sp.rtStop || sp2.rtStop <= sp.rtStart) {
			m_sps.erase(it++);
		} else {
			++it;
		}
	}

	m_sps.push_back(sp);
}

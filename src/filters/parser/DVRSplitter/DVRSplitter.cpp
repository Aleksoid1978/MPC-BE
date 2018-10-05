/*
 * (C) 2018 see Authors.txt
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
#include <initguid.h>
#include "DVRSplitter.h"
#include <moreuuids.h>

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] =
{
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] =
{
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, nullptr, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, 0, nullptr}
};

const AMOVIESETUP_FILTER sudFilter[] =
{
	{&__uuidof(CDVRSplitterFilter), DVRSplitterName, MERIT_NORMAL + 1, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CDVRSourceFilter), DVRSourceName, MERIT_NORMAL + 1, 0, nullptr, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] =
{
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CDVRSplitterFilter>, nullptr, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CDVRSourceFilter>, nullptr, &sudFilter[1]},
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_NULL, L"0,4,,48585653,16,4,,48585646");

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	UnRegisterSourceFilter(MEDIASUBTYPE_NULL);

	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

//
// CDVRSplitterFilter
//

#define HEADER_INFO FCC('HXVS')
#define FOOTER_INFO FCC('HXFI')
#define VIDEO       FCC('HXVF')
#define AUDIO       FCC('HXAF')

#define HeaderSize     16
#define FooterSize 200016

CDVRSplitterFilter::CDVRSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(L"CDVRSplitterFilter", pUnk, phr, __uuidof(this))
{
}

HRESULT CDVRSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;
	m_pFile.Free();

	m_pFile.Attach(DNew CBaseSplitterFileEx(pAsyncReader, hr, FM_FILE | FM_FILE_DL));
	if (!m_pFile) {
		return E_OUTOFMEMORY;
	}
	if (FAILED(hr)) {
		m_pFile.Free();
		return hr;
	}
	m_pFile->SetBreakHandle(GetRequestHandle());

	BYTE buf[28] = {};
	if (FAILED(m_pFile->ByteRead(buf, sizeof(buf)))) {
		m_pFile.Free();
		return E_FAIL;
	}

	if (GETU32(buf) == HEADER_INFO && GETU32(buf + 16) == VIDEO) {
		m_rtOffsetVideo = GETU32(buf + 24) * 10000ll;
	} else {
		m_pFile.Free();
		return E_FAIL;
	}

	std::vector<CMediaType> mts;
	CMediaType mt;

	m_startpos = HeaderSize;
	m_endpos   = m_pFile->GetLength();
	m_pFile->Seek(m_startpos);

	Header hdr;
	std::vector<BYTE> pData;
	while (ReadHeader(hdr) && m_pFile->GetPos() <= MEGABYTE) {
		if (hdr.sync == VIDEO) {
			if (hdr.rt != m_rtOffsetVideo) {
				m_AvgTimePerFrame = hdr.rt - m_rtOffsetVideo;
				if (!pData.empty()) {
					CBaseSplitterFileEx::avchdr h;
					if (m_pFile->Read(h, pData, &mt)) {
						mts.push_back(mt);
						if (mt.subtype == MEDIASUBTYPE_H264 && SUCCEEDED(CreateAVCfromH264(&mt))) {
							mts.push_back(mt);
						}
					}
				}
				break;
			} else {
				const size_t old_size = pData.size();
				pData.resize(old_size + hdr.size);
				if ((hr = m_pFile->ByteRead(pData.data() + old_size, hdr.size)) != S_OK) {
					break;
				}
			}
		} else {
			if (m_rtOffsetAudio == INVALID_TIME) {
				m_rtOffsetAudio = hdr.rt;
			}
			m_pFile->Skip(hdr.size);
		}
	};

	if (mts.empty()) {
		m_pFile.Free();
		return E_FAIL;
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = 0;
	
	// footer
	if (m_pFile->GetLength() > (FooterSize + HeaderSize + HeaderSize)) {
		m_pFile->Seek(m_pFile->GetLength() - FooterSize);
		DWORD sync = 0;
		if (S_OK == m_pFile->ByteRead((BYTE*)&sync, sizeof(sync)) && sync == FOOTER_INFO) {
			m_pFile->Skip(4);
			if (S_OK == m_pFile->ByteRead((BYTE*)&sync, sizeof(sync)) && sync) {
				m_rtNewStop = m_rtStop = m_rtDuration = sync * 10000ll;
			}
			m_endpos = m_pFile->GetLength() - FooterSize;
		}
	}

	// find audio stream
	if (m_rtOffsetAudio == INVALID_TIME) {
		m_pFile->Seek(16);
		while (ReadHeader(hdr) && m_pFile->GetPos() <= MEGABYTE) {
			if (hdr.sync == AUDIO) {
				m_rtOffsetAudio = hdr.rt;
				break;
			}
		}
	}

	// find end PTS
	if (m_rtDuration == 0) {
		m_pFile->Seek(m_endpos - std::min((__int64)MEGABYTE, m_endpos));
		
		REFERENCE_TIME rtLast = INVALID_TIME;
		while (ReadHeader(hdr)) {
			if (hdr.sync == VIDEO) {
				rtLast = hdr.rt;
			}
			m_pFile->Skip(hdr.size);
		};

		if (rtLast > m_rtOffsetVideo) {
			m_rtNewStop = m_rtStop = m_rtDuration = rtLast - m_rtOffsetVideo;
		}
	}

	for (auto& mt : mts) {
		auto vih2 = (VIDEOINFOHEADER2*)mt.Format();
		if (!vih2->AvgTimePerFrame) {
			vih2->AvgTimePerFrame = m_AvgTimePerFrame;
		}
	}

	CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CBaseSplitterOutputPin(mts, L"Video", this, this, &hr));
	EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(0, pPinOut)));

	if (m_rtOffsetAudio != INVALID_TIME) {
		mt.InitMediaType();

		CBaseSplitterFileEx::pcm_law_hdr h;
		m_pFile->Read(h, true, &mt);

		mts.clear();
		mts.push_back(mt);

		CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CBaseSplitterOutputPin(mts, L"Audio", this, this, &hr));
		EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(1, pPinOut)));
	}

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

bool CDVRSplitterFilter::DemuxInit()
{
	SetThreadName((DWORD)-1, "CDVRSplitterFilter");

	m_pFile->Seek(m_startpos);

	return true;
}

#define CalcPos(rt) (llMulDiv(rt, len, m_rtDuration, 0))
void CDVRSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	if (rt <= 0 || m_rtDuration <= 0) {
		m_pFile->Seek(m_startpos);
		return;
	}

	if (m_sps.size() > 1) {
		const auto rtSeek = rt + m_rtOffsetVideo;
		if (rtSeek <= m_sps.back().rt) {
			const auto index = range_bsearch(m_sps, rtSeek);
			if (index >= 0 && ((rtSeek - m_sps[index].rt) < 5 * UNITS)) {
				m_pFile->Seek(m_sps[index].fp);
				return;
			}
		}
	}

	const __int64 len = m_endpos - m_startpos;
	__int64 seekpos   = CalcPos(rt);

	const REFERENCE_TIME rtmax = (rt + m_rtOffsetVideo) - UNITS;
	const REFERENCE_TIME rtmin = rtmax - UNITS / 2;

	__int64 curpos = seekpos;
	double div = 1.0;

	for (;;) {
		REFERENCE_TIME rtSeek = INVALID_TIME;

		m_pFile->Seek(curpos);
		Header hdr;
		while (ReadHeader(hdr)) {
			if (hdr.sync == VIDEO) {
				rtSeek = hdr.rt;
				break;
			}
		}

		if (rtmin <= rtSeek && rtSeek <= rtmax) {
			m_pFile->Seek(m_pFile->GetPos() - HeaderSize);
			return;
		}

		REFERENCE_TIME dt = rtSeek - rtmax;
		if (rtSeek < 0) {
			dt = UNITS / div;
		}
		dt /= div;
		div += 0.05;

		if (div >= 5.0) {
			break;
		}

		curpos -= CalcPos(dt);
		m_pFile->Seek(curpos);
	}

	m_pFile->Seek(CalcPos(rt));
}

bool CDVRSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	REFERENCE_TIME last_rt = 0;
	CAutoPtr<CPacket> vp;

	while (SUCCEEDED(hr) && !CheckRequest(nullptr) && m_pFile->GetRemaining()) {
		Header hdr;
		if (ReadHeader(hdr)) {
			if (hdr.sync == VIDEO) {
				// video packet
				if (vp && last_rt != hdr.rt) {
					hr = DeliverPacket(vp);
				}

				if (!vp) {
					vp.Attach(DNew CPacket());
					vp->rtStart = hdr.rt - m_rtOffsetVideo;
					vp->rtStop = vp->rtStart + m_AvgTimePerFrame;
					last_rt = hdr.rt;

					vp->resize(hdr.size);
					hr = m_pFile->ByteRead(vp->data(), hdr.size);
				} else {
					const size_t old_size = vp->size();
					vp->resize(old_size + hdr.size);
					hr = m_pFile->ByteRead(vp->data() + old_size, hdr.size);
				}
			} else {
				// audio packet
				if (GetOutputPin(1)) {
					CAutoPtr<CPacket> p(DNew CPacket());
					p->TrackNumber = 1;
					p->rtStart = hdr.rt - m_rtOffsetAudio;
					p->rtStop = p->rtStart + llMulDiv(hdr.size, UNITS, 8000, 0);

					p->resize(hdr.size);
					if ((hr = m_pFile->ByteRead(p->data(), hdr.size)) == S_OK) {
						hr = DeliverPacket(p);
					}
				} else {
					m_pFile->Skip(hdr.size);
				}
			}
		} else {
			break;
		}
	}

	if (vp) {
		DeliverPacket(vp);
	}

	return true;
}

bool CDVRSplitterFilter::Sync()
{
	const __int64 start = m_pFile->GetPos();
	DWORD sync;
	for (__int64 i = 0, j = m_endpos - start - HeaderSize - 4;
			i <= MEGABYTE && i < j && S_OK == m_pFile->ByteRead((BYTE*)&sync, sizeof(sync));
			i++, m_pFile->Seek(start + i)) {
		if (sync == VIDEO || sync == AUDIO) {
			m_pFile->Seek(start + i);
			return true;
		}
	}

	m_pFile->Seek(start);
	return false;
}

bool CDVRSplitterFilter::ReadHeader(Header& hdr)
{
	const auto ret = Sync() && S_OK == m_pFile->ByteRead((BYTE*)&hdr, HeaderSize);
	if (ret) {
		hdr.rt = hdr.pts * 10000ll;
		hdr.key_frame = hdr.dummy == 1;

		if (hdr.sync == AUDIO) {
			hdr.size -= 4;
			m_pFile->Skip(4);
		}

		if (hdr.key_frame) {
			const auto pos = m_pFile->GetPos() - HeaderSize;

			if (m_sps.empty() || (hdr.rt > m_sps.back().rt)) {
				const SyncPoint sp = {hdr.rt, pos};
				m_sps.emplace_back(sp);
			} else if (hdr.rt < m_sps.back().rt) {
				const auto exists = std::any_of(m_sps.begin(), m_sps.end(), [&](const SyncPoint& sp) {
					return sp.rt == hdr.rt;
				});
				if (!exists) {
					const SyncPoint sp = {hdr.rt, pos};
					m_sps.emplace_back(sp);
					std::sort(m_sps.begin(), m_sps.end(), [](const SyncPoint& a, const SyncPoint& b) {
						return a.rt < b.rt;
					});
				}
			}
		}
	}

	return ret;
}

// CBaseFilter

STDMETHODIMP CDVRSplitterFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	wcscpy_s(pInfo->achName, DVRSplitterName);

	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

//
// CDVRSourceFilter
//

CDVRSourceFilter::CDVRSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CDVRSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.Free();
}

STDMETHODIMP CDVRSourceFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	wcscpy_s(pInfo->achName, DVRSourceName);

	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

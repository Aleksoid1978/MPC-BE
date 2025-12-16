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
#include <Shlwapi.h>
#include <ks.h>
#include <ksmedia.h>
#include <wmcodecdsp.h>
#include <moreuuids.h>
#include "DSUtil/DSUtil.h"
#include "DSUtil/PixelUtils.h"
#include "Subtitles/SubtitleInputPin.h"
#include "RealMediaSplitter.h"

template<typename T>
static void bswap(T& var)
{
	BYTE* s = (BYTE*)&var;
	for (BYTE* d = s + sizeof(var)-1; s < d; s++, d--) {
		*s ^= *d, *d ^= *s, *s ^= *d;
	}
}

void rvinfo::bswap()
{
	::bswap(dwSize);
	::bswap(w);
	::bswap(h);
	::bswap(bpp);
	::bswap(unk1);
	::bswap(fps);
	::bswap(type1);
	::bswap(type2);
}

void rainfo3::bswap()
{
	::bswap(fourcc);
	::bswap(version);
	::bswap(header_size);
	::bswap(data_size);
}

void rainfo::bswap()
{
	::bswap(version1);
	::bswap(version2);
	::bswap(header_size);
	::bswap(flavor);
	::bswap(coded_frame_size);
	::bswap(sub_packet_h);
	::bswap(frame_size);
	::bswap(sub_packet_size);
}

void rainfo4::bswap()
{
	__super::bswap();
	::bswap(sample_rate);
	::bswap(sample_size);
	::bswap(channels);
}

void rainfo5::bswap()
{
	__super::bswap();
	::bswap(sample_rate);
	::bswap(sample_size);
	::bswap(channels);
}

using namespace RMFF;

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] = {
	{(LPWSTR)L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, nullptr, std::size(sudPinTypesIn), sudPinTypesIn},
	{(LPWSTR)L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, 0, nullptr}
};

const AMOVIESETUP_MEDIATYPE sudPinTypesIn2[] = {
	{&MEDIATYPE_Video, &MEDIASUBTYPE_RV20},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_RV30},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_RV40},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_RV41},
};

const AMOVIESETUP_MEDIATYPE sudPinTypesOut2[] = {
	{&MEDIATYPE_Video, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins2[] = {
	{(LPWSTR)L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, nullptr, std::size(sudPinTypesIn2), sudPinTypesIn2},
	{(LPWSTR)L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, std::size(sudPinTypesOut2), sudPinTypesOut2}
};

const AMOVIESETUP_MEDIATYPE sudPinTypesIn3[] = {
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_14_4},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_28_8},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_ATRC},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_COOK},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_DNET},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_SIPR},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_SIPR_WAVE},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_RAW_AAC1},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_RAAC},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_RACP},
};

const AMOVIESETUP_MEDIATYPE sudPinTypesOut3[] = {
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_PCM},
};

const AMOVIESETUP_PIN sudpPins3[] = {
	{(LPWSTR)L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, nullptr, std::size(sudPinTypesIn3), sudPinTypesIn3},
	{(LPWSTR)L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, std::size(sudPinTypesOut3), sudPinTypesOut3}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CRealMediaSplitterFilter), RMSplitterName, MERIT_NORMAL, std::size(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CRealMediaSourceFilter), RMSourceName, MERIT_NORMAL, 0, nullptr, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CRealMediaSplitterFilter>, nullptr, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CRealMediaSourceFilter>, nullptr, &sudFilter[1]},
};

int g_cTemplates = std::size(g_Templates);

STDAPI DllRegisterServer()
{
	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_RealMedia, L"0,4,,2E524D46", L".rm", L".rmvb", L".ram", nullptr);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	UnRegisterSourceFilter(MEDIASUBTYPE_RealMedia);

	return AMovieDllRegisterServer2(FALSE);
}

#include "filters/filters/Filters.h"

CFilterApp theApp;

#endif

//
// CRealMediaSplitterFilter
//

CRealMediaSplitterFilter::CRealMediaSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(L"CRealMediaSplitterFilter", pUnk, phr, __uuidof(this))
{
	m_nFlag |= SOURCE_SUPPORT_URL;
}

CRealMediaSplitterFilter::~CRealMediaSplitterFilter()
{
}

STDMETHODIMP CRealMediaSplitterFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	if (m_pName && m_pName[0]==L'M' && m_pName[1]==L'P' && m_pName[2]==L'C') {
		(void)StringCchCopyW(pInfo->achName, NUMELMS(pInfo->achName), m_pName);
	} else {
		wcscpy_s(pInfo->achName, RMSourceName);
	}
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

HRESULT CRealMediaSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	{
		DWORD dw;
		if (FAILED(pAsyncReader->SyncRead(0, 4, (BYTE*)&dw)) || dw != 'FMR.') {
			return E_FAIL;
		}
	}

	HRESULT hr = E_FAIL;

	m_pFile.reset(DNew CRMFile(pAsyncReader, hr));
	if (!m_pFile) {
		return E_OUTOFMEMORY;
	}
	if (FAILED(hr)) {
		m_pFile.reset();
		return hr;
	}
	m_pFile->SetBreakHandle(GetRequestHandle());

	m_rtNewStart = m_rtCurrent = m_rtNewStop = 0;

	m_rtStop = 10000i64*m_pFile->m_p.tDuration;

	for (const auto& pmp : m_pFile->m_mps) {
		CStringW name;
		name.Format(L"Output %02d", pmp->stream);
		if (!pmp->name.IsEmpty()) {
			name += L" (" + CStringW(pmp->name) + L')';
		}

		std::vector<CMediaType> mts;

		CMediaType mt;
		mt.SetSampleSize(std::max(pmp->maxPacketSize*16, 1u));

		if (pmp->mime == "video/x-pn-realvideo") {
			mt.majortype = MEDIATYPE_Video;
			mt.formattype = FORMAT_VideoInfo;

			VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + pmp->typeSpecData.size());
			memset(mt.Format(), 0, mt.FormatLength());
			memcpy(pvih + 1, pmp->typeSpecData.data(), pmp->typeSpecData.size());

			rvinfo rvi = *(rvinfo*)pmp->typeSpecData.data();
			rvi.bswap();

			ASSERT(rvi.dwSize >= FIELD_OFFSET(rvinfo, morewh));
			ASSERT(rvi.fcc1 == 'ODIV');

			mt.subtype = FOURCCMap(rvi.fcc2);
			if (rvi.fps > 0x10000) {
				pvih->AvgTimePerFrame = REFERENCE_TIME(10000000i64 / ((float)rvi.fps/0x10000));
			}
			pvih->dwBitRate = pmp->avgBitRate;
			pvih->bmiHeader.biSize = sizeof(pvih->bmiHeader);
			pvih->bmiHeader.biWidth = rvi.w;
			pvih->bmiHeader.biHeight = rvi.h;
			pvih->bmiHeader.biPlanes = 3;
			pvih->bmiHeader.biBitCount = rvi.bpp;
			pvih->bmiHeader.biCompression = rvi.fcc2;
			pvih->bmiHeader.biSizeImage = rvi.w*rvi.h*3/2;
			mts.push_back(mt);

			BYTE* extra		= pmp->typeSpecData.data();
			int extralen	= pmp->typeSpecData.size();

			if (extralen > 26) {
				extra		+= 26;
				extralen	-= 26;
				VIDEOINFOHEADER* pvih2 = (VIDEOINFOHEADER*)mt.ReallocFormatBuffer(sizeof(VIDEOINFOHEADER) + extralen);
				memcpy(pvih2 + 1, extra, extralen);
				mts.insert(mts.cbegin(), mt);
			}

			if (pmp->width > 0 && pmp->height > 0) {
				BITMAPINFOHEADER bmi = pvih->bmiHeader;
				mt.formattype = FORMAT_VideoInfo2;
				VIDEOINFOHEADER2* pvih2 = (VIDEOINFOHEADER2*)mt.ReallocFormatBuffer(sizeof(VIDEOINFOHEADER2) + pmp->typeSpecData.size());
				memset(mt.Format() + FIELD_OFFSET(VIDEOINFOHEADER2, dwInterlaceFlags), 0, mt.FormatLength() - FIELD_OFFSET(VIDEOINFOHEADER2, dwInterlaceFlags));
				memcpy(pvih2 + 1, pmp->typeSpecData.data(), pmp->typeSpecData.size());
				pvih2->bmiHeader = bmi;
				pvih2->bmiHeader.biWidth = (DWORD)pmp->width;
				pvih2->bmiHeader.biHeight = (DWORD)pmp->height;
				pvih2->dwPictAspectRatioX = rvi.w;
				pvih2->dwPictAspectRatioY = rvi.h;

				mts.insert(mts.cbegin(), mt);
			}
		} else if (pmp->mime == "audio/x-pn-realaudio") {
			mt.majortype = MEDIATYPE_Audio;
			mt.formattype = FORMAT_WaveFormatEx;
			mt.bTemporalCompression = 1;

			WAVEFORMATEX* pwfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX) + pmp->typeSpecData.size());
			memset(mt.Format(), 0, mt.FormatLength());
			memcpy(pwfe + 1, pmp->typeSpecData.data(), pmp->typeSpecData.size());
			pwfe->cbSize = (WORD)pmp->typeSpecData.size();

			union {
				DWORD fcc;
				char fccstr[5];
			};

			fcc = 0;
			fccstr[4] = 0;

			BYTE* fmt = pmp->typeSpecData.data();
			for (size_t i = 0; i < pmp->typeSpecData.size()-4; i++, fmt++) {
				if (*(DWORD*)fmt == MAKEFOURCC('.' ,'r' ,'a' ,0xfd)) {
					break;
				}
			}

			BYTE* extra = nullptr;

			WORD ver = fmt[4] << 8 | fmt[5];
			if (ver == 3) {
				rainfo3 rai3 = *(rainfo3*)fmt;
				rai3.bswap();
				fcc = '4_41';
				pwfe->nChannels       = 1;
				pwfe->nSamplesPerSec  = 8000;
				pwfe->nAvgBytesPerSec = 1000;
				pwfe->nBlockAlign     = 2;
				pwfe->wBitsPerSample  = 16;
			} else {
				rainfo rai = *(rainfo*)fmt;
				rai.bswap();

				if (rai.version2 == 4) {
					rainfo4 rai4 = *(rainfo4*)fmt;
					rai4.bswap();
					pwfe->nChannels = rai4.channels;
					pwfe->wBitsPerSample = rai4.sample_size;
					pwfe->nSamplesPerSec = rai4.sample_rate;
					pwfe->nBlockAlign = rai4.frame_size;
					BYTE* p = (BYTE*)((rainfo4*)fmt + 1);
					int len = *p++;
					p += len;
					len = *p++;
					ASSERT(len == 4);
					if (len == 4) {
						fcc = MAKEFOURCC(p[0], p[1], p[2], p[3]);
					}
					extra = p + len + 3;
				} else if (rai.version2 == 5) {
					rainfo5 rai5 = *(rainfo5*)fmt;
					rai5.bswap();
					pwfe->nChannels = rai5.channels;
					pwfe->wBitsPerSample = rai5.sample_size;
					pwfe->nSamplesPerSec = rai5.sample_rate;
					pwfe->nBlockAlign = rai5.frame_size;
					fcc = rai5.fourcc3;
					extra = fmt + sizeof(rainfo5) + 4;
				} else {
					continue;
				}
			}

			_strupr_s(fccstr);

			mt.subtype = FOURCCMap(fcc);

			bswap(fcc);

			switch (fcc) {
				case '14_4':
					pwfe->wFormatTag = WAVE_FORMAT_14_4;
					break;
				case '28_8':
					pwfe->wFormatTag = WAVE_FORMAT_28_8;
					break;
				case 'ATRC':
					pwfe->wFormatTag = WAVE_FORMAT_ATRC;
					break;
				case 'COOK':
					pwfe->wFormatTag = WAVE_FORMAT_COOK;
					break;
				case 'DNET':
					pwfe->wFormatTag = WAVE_FORMAT_DNET;
					break;
				case 'SIPR':
					pwfe->wFormatTag = WAVE_FORMAT_SIPR;
					break;
				case 'RAAC':
					pwfe->wFormatTag = WAVE_FORMAT_RAAC;
					break;
				case 'RACP':
					pwfe->wFormatTag = WAVE_FORMAT_RACP;
					break;
			}

			if (pwfe->wFormatTag) {
				mts.push_back(mt);

				if (fcc == 'DNET') {
					mt.subtype = FOURCCMap(pwfe->wFormatTag = WAVE_FORMAT_DOLBY_AC3);
					mts.insert(mts.cbegin(), mt);
				} else if (fcc == 'RAAC' || fcc == 'RACP') {
					mt.subtype = FOURCCMap(pwfe->wFormatTag = WAVE_FORMAT_RAW_AAC1);
					int extralen = *(DWORD*)extra;
					extra += 4;
					::bswap(extralen);
					ASSERT(*extra == 2);	// always 2? why? what does it mean?
					if (*extra == 2) {
						extra++;
						extralen--;
						WAVEFORMATEX* pwfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + extralen);
						pwfe->cbSize = extralen;
						memcpy(pwfe + 1, extra, extralen);
					} else {
						WAVEFORMATEX* pwfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + 5);
						pwfe->cbSize = MakeAACInitData((BYTE*)(pwfe+1), 0, pwfe->nSamplesPerSec, pwfe->nChannels);
					}
					mts.insert(mts.cbegin(), mt);
				}
			}
		} else if (pmp->mime == "logical-fileinfo") {
			std::map<CStringA, CStringA> lfi;
			CStringA key, value;

			BYTE* p = pmp->typeSpecData.data();
			BYTE* end = p + pmp->typeSpecData.size();
			p += 8;

			DWORD cnt = p <= end-4 ? *(DWORD*)p : 0;
			bswap(cnt);
			p += 4;

			if (cnt > 0xffff) { // different format?
				p += 2;
				cnt = p <= end-4 ? *(DWORD*)p : 0;
				bswap(cnt);
				p += 4;
			}

			while (p < end-4 && cnt-- > 0) {
				BYTE* base = p;
				DWORD len = *(DWORD*)p;
				bswap(len);
				p += 4;
				if (base + len > end) {
					break;
				}

				p++;
				WORD keylen = *(WORD*)p;
				bswap(keylen);
				p += 2;
				memcpy(key.GetBufferSetLength(keylen), p, keylen);
				p += keylen;

				p+=4;
				WORD valuelen = *(WORD*)p;
				bswap(valuelen);
				p += 2;
				memcpy(value.GetBufferSetLength(valuelen), p, valuelen);
				p += valuelen;

				ASSERT(p == base + len);
				p = base + len;

				lfi[key] = value;
			}

			for (const auto& item : lfi) {
				key = item.first;
				value = item.second;

				int n = 0;
				if (key.Find("CHAPTER") == 0 && key.Find("TIME") == key.GetLength()-4
						&& (n = strtol(key.Mid(7), nullptr, 10)) > 0) {
					int h, m, s, ms;
					char c;
					if (7 != sscanf_s(value, "%d%c%d%c%d%c%d",
							&h, &c, 1,
							&m, &c, 1,
							&s, &c, 1, &ms)) {
						continue;
					}

					key.Format("CHAPTER%02dNAME", n);
					const auto it = lfi.find(key);

					if (it == lfi.end() || (*it).second.IsEmpty()) {
						value.Format("Chapter %d", n);
					}

					ChapAppend(
						((((REFERENCE_TIME)h*60+m)*60+s)*1000+ms)*10000,
						CString(value));
				}
			}
		}

		if (mts.empty()) {
			DLog(L"Unsupported RealMedia stream (%u): %hs", (UINT32)pmp->stream, pmp->mime);
			continue;
		}

		HRESULT hr;

		std::unique_ptr<CBaseSplitterOutputPin> pPinOut(DNew CRealMediaSplitterOutputPin(mts, name, this, this, &hr));
		if (SUCCEEDED(AddOutputPin((DWORD)pmp->stream, pPinOut))) {
			if (!m_rtStop) {
				m_pFile->m_p.tDuration = std::max(m_pFile->m_p.tDuration, pmp->tDuration);
			}
		}
	}

	auto it = m_pFile->m_subs.begin();
	for (DWORD stream = 0; it != m_pFile->m_subs.end(); stream++) {
		CRMFile::subtitle& s = *it++;

		CStringW name;
		name.Format(L"Subtitle %02u", stream);
		if (!s.name.IsEmpty()) {
			name += L" (" + CString(s.name) + L')';
		}

		CMediaType mt;
		mt.SetSampleSize(1);
		mt.majortype = MEDIATYPE_Text;

		std::vector<CMediaType> mts;
		mts.push_back(mt);

		HRESULT hr;

		std::unique_ptr<CBaseSplitterOutputPin> pPinOut(DNew CRealMediaSplitterOutputPin(mts, name, this, this, &hr));
		AddOutputPin((DWORD)~stream, pPinOut);
	}

	m_rtDuration = m_rtNewStop = m_rtStop = 10000i64*m_pFile->m_p.tDuration;

	SetProperty(L"TITL", CStringW(m_pFile->m_cd.title));
	SetProperty(L"AUTH", CStringW(m_pFile->m_cd.author));
	SetProperty(L"CPYR", CStringW(m_pFile->m_cd.copyright));
	SetProperty(L"DESC", CStringW(m_pFile->m_cd.comment));

	return m_pOutputs.size() > 0 ? S_OK : E_FAIL;
}

bool CRealMediaSplitterFilter::DemuxInit()
{
	SetThreadName((DWORD)-1, "CRealMediaSplitterFilter");

	if (!m_pFile) {
		return false;
	}

	// reindex if needed
	if (m_pFile->m_irs.size() == 0) {
		m_nOpenProgress = 0;

		int stream = m_pFile->GetMasterStream();

		UINT32 tLastStart = (UINT32)-1;
		UINT32 nPacket = 0;

		for (const auto& pdc : m_pFile->m_dcs) {
			if (m_fAbort) {
				break;
			}

			m_pFile->Seek(pdc->pos);

			for (UINT32 i = 0; i < pdc->nPackets && !m_fAbort; i++, nPacket++) {
				UINT64 filepos = m_pFile->GetPos();

				HRESULT hr;

				MediaPacketHeader mph;
				if (S_OK != (hr = m_pFile->Read(mph, false))) {
					break;
				}

				if (mph.stream == stream) {
					m_rtDuration = std::max((__int64)(10000i64*mph.tStart), m_rtDuration);

					if (mph.flags&MediaPacketHeader::PN_KEYFRAME_FLAG && tLastStart != mph.tStart) {
						std::unique_ptr<IndexRecord> pir(DNew IndexRecord);
						pir->tStart = mph.tStart;
						pir->ptrFilePos = (UINT32)filepos;
						pir->packet = nPacket;
						m_pFile->m_irs.emplace_back(std::move(pir));

						tLastStart = mph.tStart;
					}
				}

				m_nOpenProgress = m_pFile->GetPos()*100/m_pFile->GetLength();

				DWORD cmd;
				if (CheckRequest(&cmd)) {
					if (cmd == CMD_EXIT) {
						m_fAbort = true;
					} else {
						Reply(S_OK);
					}
				}
			}
		}

		m_nOpenProgress = 100;

		if (m_fAbort) {
			m_pFile->m_irs.clear();
		}

		m_fAbort = false;
	}

	m_seekdc      = UINT32_MAX;
	m_seekpacket  = 0;
	m_seekfilepos = 0;

	return true;
}

void CRealMediaSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	if (rt <= 0) {
		m_seekdc      = 0;
		m_seekpacket  = 0;
		m_seekfilepos = m_pFile->m_dcs.front()->pos;
	} else {
		m_seekdc = UINT32_MAX;
		auto CheckDCsIdx = [&](const UINT32& indx) {
			return (indx < m_pFile->m_dcs.size());
		};

		for (auto it = m_pFile->m_irs.crbegin(); it != m_pFile->m_irs.crend(); ++it) {
			auto& pir = *it;
			if (pir->tStart < rt/10000) {
				m_seekpacket = pir->packet;

				UINT32 idxR = m_pFile->m_dcs.size();
				while (idxR) {
					idxR--;
					if (m_pFile->m_dcs[idxR]->pos <= pir->ptrFilePos) {
						m_seekdc = idxR;
						m_seekfilepos = pir->ptrFilePos;

						UINT32 idxL = 0;
						while (idxL < m_seekdc) {
							m_seekpacket -= m_pFile->m_dcs[idxL]->nPackets;
							idxL++;
						}
						break;
					}
				}

				// search the closest keyframe to the seek time (commented out 'cause rm seems to index all of its keyframes...)
				/*
				if(m_seekpos)
				{
					DataChunk* pdc = m_pFile->m_dcs.GetAt(m_seekpos);

					m_pFile->Seek(m_seekfilepos);

					REFERENCE_TIME seektime = -1;
					UINT32 seekstream = -1;

					for(UINT32 i = m_seekpacket; i < pdc->nPackets; i++)
					{
						UINT64 filepos = m_pFile->GetPos();

						MediaPacketHeader mph;
						if(S_OK != m_pFile->Read(mph, false))
							break;

						if(seekstream == -1) seekstream = mph.stream;
						if(seekstream != mph.stream) continue;

						if(seektime == 10000i64*mph.tStart) continue;
						if(rt < 10000i64*mph.tStart) break;

						if((mph.flags&MediaPacketHeader::PN_KEYFRAME_FLAG))
						{
							m_seekpacket = i;
							m_seekfilepos = filepos;
							seektime = 10000i64*mph.tStart;
						}
					}
				}
				*/
			}
			if (m_seekdc < m_pFile->m_dcs.size()) {
				break;
			}
		}

		if (m_seekdc == UINT32_MAX) {
			m_seekdc      = 0;
			m_seekpacket  = 0;
			m_seekfilepos = m_pFile->m_dcs.front()->pos;
		}
	}
}

bool CRealMediaSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	auto it = m_pFile->m_subs.begin();
	for (DWORD stream = 0; it != m_pFile->m_subs.end() && SUCCEEDED(hr) && !CheckRequest(nullptr); stream++) {
		CRMFile::subtitle& s = *it++;

		std::unique_ptr<CPacket> p(DNew CPacket);

		p->TrackNumber = ~stream;
		p->bSyncPoint = TRUE;
		p->rtStart = 0;
		p->rtStop = 1;

		size_t count = (4+1) + (2+4+(s.name.GetLength()+1)*2) + (2+4+s.data.GetLength());
		p->resize(count);
		BYTE* ptr = p->data();

		strcpy_s((char*)ptr, count, "GAB2");
		ptr += 4+1;
		count -= 4+1;

		*(WORD*)ptr = 2;
		ptr += 2;
		count -= 2;
		*(DWORD*)ptr = (s.name.GetLength()+1)*2;
		ptr += 4;
		count -= 4;
		wcscpy_s((WCHAR*)ptr, count / 2, CStringW(s.name));
		ptr += (s.name.GetLength()+1)*2;

		*(WORD*)ptr = 4;
		ptr += 2;
		*(DWORD*)ptr = s.data.GetLength();
		ptr += 4;
		memcpy((char*)ptr, s.data, s.data.GetLength());
		ptr += s.name.GetLength();

		hr = DeliverPacket(std::move(p));
	}

	UINT32 idxdc = m_seekdc;
	while (idxdc < m_pFile->m_dcs.size() && SUCCEEDED(hr) && !CheckRequest(nullptr)) {
		const auto& pdc = m_pFile->m_dcs[idxdc];
		idxdc++;

		m_pFile->Seek(m_seekfilepos > 0 ? m_seekfilepos : pdc->pos);

		for (UINT32 i = m_seekpacket; (pdc->nPackets ? i < pdc->nPackets : TRUE) && SUCCEEDED(hr) && !CheckRequest(nullptr); i++) {
			MediaPacketHeader mph;
			if (S_OK != (hr = m_pFile->Read(mph))) {
				break;
			}

			std::unique_ptr<CPacket> p(DNew CPacket);
			p->TrackNumber	= mph.stream;
			p->bSyncPoint	= !!(mph.flags & MediaPacketHeader::PN_KEYFRAME_FLAG);
			p->rtStart		= 10000i64 * mph.tStart;
			p->rtStop		= p->rtStart + 1;
			p->SetData(mph.pData.data(), mph.pData.size());
			hr				= DeliverPacket(std::move(p));
		}

		m_seekpacket = 0;
		m_seekfilepos = 0;
	}

	return true;
}

// IKeyFrameInfo

STDMETHODIMP CRealMediaSplitterFilter::GetKeyFrameCount(UINT& nKFs)
{
	if (!m_pFile) {
		return E_UNEXPECTED;
	}
	nKFs = m_pFile->m_irs.size();
	return S_OK;
}

STDMETHODIMP CRealMediaSplitterFilter::GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs)
{
	CheckPointer(pFormat, E_POINTER);
	CheckPointer(pKFs, E_POINTER);

	if (!m_pFile) {
		return E_UNEXPECTED;
	}
	if (*pFormat != TIME_FORMAT_MEDIA_TIME) {
		return E_INVALIDARG;
	}

	UINT nKFsTmp = 0;
	for (auto it = m_pFile->m_irs.cbegin(); it != m_pFile->m_irs.cend() && nKFsTmp < nKFs; ++it) {
		pKFs[nKFsTmp++] = 10000i64 * (*it)->tStart;
	}
	nKFs = nKFsTmp;

	return S_OK;
}

//
// CRealMediaSplitterOutputPin
//

CRealMediaSplitterOutputPin::CRealMediaSplitterOutputPin(std::vector<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseSplitterOutputPin(mts, pName, pFilter, pLock, phr)
{
}

CRealMediaSplitterOutputPin::~CRealMediaSplitterOutputPin()
{
}

HRESULT CRealMediaSplitterOutputPin::DeliverEndFlush()
{
	{
		CAutoLock cAutoLock(&m_csQueue);
		m_segments.Clear();
	}

	return __super::DeliverEndFlush();
}

HRESULT CRealMediaSplitterOutputPin::DeliverSegments()
{
	if (m_segments.empty()) {
		m_segments.Clear();
		return S_OK;
	}

	std::unique_ptr<CPacket> p(DNew CPacket());

	p->TrackNumber		= DWORD_MAX;
	p->bDiscontinuity	= m_segments.fDiscontinuity;
	p->bSyncPoint		= m_segments.fSyncPoint;
	p->rtStart			= m_segments.rtStart;
	p->rtStop			= m_segments.rtStart + 1;

	DWORD len = 0, total = 0;
	for (const auto& s : m_segments) {
		len = std::max(len, s->offset + (DWORD)s->data.size());
		total += s->data.size();
	}
	ASSERT(len == total);

	len += 1 + 2 * 4 * (!m_segments.fMerged ? m_segments.size() : 1);

	p->resize(len);

	BYTE* pData = p->data();

	*pData++ = m_segments.fMerged ? 0 : (BYTE)(m_segments.size() - 1);

	if (m_segments.fMerged) {
		*((DWORD*)pData) = 1;
		pData += 4;
		*((DWORD*)pData) = 0;
		pData += 4;
	} else {
		for (const auto& s : m_segments) {
			*((DWORD*)pData) = 1;
			pData += 4;
			*((DWORD*)pData) = s->offset;
			pData += 4;
		}
	}

	for (const auto& s : m_segments) {
		memcpy(pData + s->offset, s->data.data(), s->data.size());
	}
	m_segments.Clear();

	return __super::DeliverPacket(std::move(p));
}

HRESULT CRealMediaSplitterOutputPin::DeliverPacket(std::unique_ptr<CPacket> p)
{
	HRESULT hr = S_OK;

	ASSERT(p->rtStart < p->rtStop);

	if (m_mt.subtype == MEDIASUBTYPE_WAVE_DOLBY_AC3) {
		WORD* s = (WORD*)p->data();
		WORD* e = s + p->size()/2;
		while (s < e) {
			bswap(*s++);
		}
	}

	if (m_mt.subtype == MEDIASUBTYPE_RV10 || m_mt.subtype == MEDIASUBTYPE_RV20
			|| m_mt.subtype == MEDIASUBTYPE_RV30 || m_mt.subtype == MEDIASUBTYPE_RV40
			|| m_mt.subtype == MEDIASUBTYPE_RV41) {
		CAutoLock cAutoLock(&m_csQueue);

		int len = p->size();
		BYTE* pIn = p->data();
		BYTE* pInOrg = pIn;

		if (m_segments.rtStart != p->rtStart) {
			if (S_OK != (hr = DeliverSegments())) {
				return hr;
			}
		}

		if (!m_segments.fDiscontinuity && p->bDiscontinuity) {
			m_segments.fDiscontinuity = true;
		}
		m_segments.fSyncPoint = !!p->bSyncPoint;
		m_segments.rtStart = p->rtStart;

		while (pIn - pInOrg < len) {
			BYTE hdr = *pIn++;
			DWORD packetlen = 0, packetoffset = 0;

			if ((hdr & 0xc0) == 0x40) {
				pIn++;
				packetlen = len - (pIn - pInOrg);
			} else {
				if ((hdr & 0x40) == 0) {
					pIn++; //BYTE subseq = (*pIn++)&0x7f;
				}

#define GetWORD(var) var = (var<<8)|(*pIn++); var = (var<<8)|(*pIn++);

				GetWORD(packetlen);
				if (packetlen & 0x8000) {
					m_segments.fMerged = true;
				}
				if ((packetlen & 0x4000) == 0) {
					GetWORD(packetlen);
					packetlen &= 0x3fffffff;
				} else {
					packetlen &= 0x3fff;
				}

				GetWORD(packetoffset);
				if ((packetoffset & 0x4000) == 0) {
					GetWORD(packetoffset);
					packetoffset &= 0x3fffffff;
				} else {
					packetoffset &= 0x3fff;
				}

#undef GetWORD

				if ((hdr&0xc0) == 0xc0) {
					m_segments.rtStart = 10000i64 * packetoffset - m_rtStart, packetoffset = 0;
				} else if ((hdr&0xc0) == 0x80) {
					packetoffset = packetlen - packetoffset;
				}

				pIn++; //BYTE seqnum = *pIn++;
			}

			int len2 = std::min(len - (int)(pIn - pInOrg), (int)packetlen - (int)packetoffset);
			if (len2 <= 0) {
				return E_FAIL;
			}
			if (m_segments.empty()) {
				packetoffset = 0;
			}

			std::unique_ptr<segment> s(DNew segment);
			s->offset = packetoffset;
			s->data.resize(len2);
			memcpy(s->data.data(), pIn, len2);
			m_segments.emplace_back(std::move(s));

			pIn += len2;

			if ((hdr&0x80) || packetoffset + len2 >= packetlen) {
				if (S_OK != (hr = DeliverSegments())) {
					return hr;
				}
			}
		}
	} else if (m_mt.subtype == MEDIASUBTYPE_RAAC || m_mt.subtype == MEDIASUBTYPE_RACP
			   || m_mt.subtype == MEDIASUBTYPE_RAW_AAC1) {
		BYTE* ptr = p->data()+2;

		int total = 0;
		int remaining = p->size()-2;
		int expected = *(ptr-1)>>4;

		std::vector<WORD> sizes;
		sizes.reserve(expected);

		while (total < remaining) {
			int size = (ptr[0]<<8)|(ptr[1]);
			sizes.push_back(size);
			total += size;
			ptr += 2;
			remaining -= 2;
			expected--;
		}

		ASSERT(total == remaining);
		ASSERT(expected == 0);

		WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_mt.pbFormat;
		REFERENCE_TIME rtDur = 10240000000i64/wfe->nSamplesPerSec * (wfe->cbSize>2?2:1);
		REFERENCE_TIME rtStart = p->rtStart;
		BOOL bDiscontinuity = p->bDiscontinuity;

		for (const auto& size : sizes) {
			std::unique_ptr<CPacket> p(DNew CPacket);
			p->bDiscontinuity = bDiscontinuity;
			p->bSyncPoint = true;
			p->rtStart = rtStart;
			p->rtStop = rtStart += rtDur;
			p->SetData(ptr, size);
			ptr += size;
			bDiscontinuity = false;
			hr = __super::DeliverPacket(std::move(p));
			if (S_OK != hr) {
				break;
			}
		}
	} else {
		hr = __super::DeliverPacket(std::move(p));
	}

	return hr;
}

//
// CRealMediaSourceFilter
//

CRealMediaSourceFilter::CRealMediaSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CRealMediaSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.reset();
}

//
// CRMFile
//

CRMFile::CRMFile(IAsyncReader* pAsyncReader, HRESULT& hr)
	: CBaseSplitterFile(pAsyncReader, hr, FM_FILE)
{
	if (FAILED(hr)) {
		return;
	}
	hr = Init();
}

template<typename T>
HRESULT CRMFile::Read(T& var)
{
	HRESULT hr = ByteRead((BYTE*)&var, sizeof(var));
	bswap(var);
	return hr;
}

HRESULT CRMFile::Read(ChunkHdr& hdr)
{
	memset(&hdr, 0, sizeof(hdr));
	HRESULT hr;
	if (S_OK != (hr = Read(hdr.object_id))
			|| S_OK != (hr = Read(hdr.size))
			|| S_OK != (hr = Read(hdr.object_version))) {
		return hr;
	}
	return S_OK;
}

HRESULT CRMFile::Read(MediaPacketHeader& mph, bool fFull)
{
	memset(&mph, 0, FIELD_OFFSET(MediaPacketHeader, pData));
	mph.stream = (UINT16)-1;

	HRESULT hr;

	UINT16 object_version;
	if (S_OK != (hr = Read(object_version))) {
		return hr;
	}
	if (object_version != 0 && object_version != 1) {
		return S_OK;
	}

	UINT8 flags;
	if (S_OK != (hr = Read(mph.len))
			|| S_OK != (hr = Read(mph.stream))
			|| S_OK != (hr = Read(mph.tStart))
			|| S_OK != (hr = Read(mph.reserved))
			|| S_OK != (hr = Read(flags))) {
		return hr;
	}

	mph.flags = (MediaPacketHeader::flag_t)flags;

	LONG len = mph.len;
	len -= sizeof(object_version);
	len -= FIELD_OFFSET(MediaPacketHeader, flags);
	len -= sizeof(flags);
	//ASSERT(len >= 0);
	if (len < 0) {
		len = 0;
	}

	if (fFull) {
		mph.pData.resize(len);
		if (mph.len > 0 && S_OK != (hr = ByteRead(mph.pData.data(), len))) {
			return hr;
		}
	} else {
		Seek(GetPos() + len);
	}

	return S_OK;
}

HRESULT CRMFile::Init()
{
	Seek(0);

	bool fFirstChunk = true;

	HRESULT hr;

	ChunkHdr hdr;
	while (GetRemaining() && S_OK == (hr = Read(hdr))) {
		__int64 pos = GetPos() - sizeof(hdr);

		if (fFirstChunk && hdr.object_id != '.RMF') {
			return E_FAIL;
		}

		fFirstChunk = false;

		if (pos + hdr.size > GetLength() && hdr.object_id != 'DATA') {	// truncated?
			break;
		}

		if (hdr.object_id == 0x2E7261FD) {	// '.ra+0xFD'
			return E_FAIL;
		}

		if (hdr.object_version == 0) {
			switch (hdr.object_id) {
				case '.RMF':
					if (S_OK != (hr = Read(m_fh.version))) {
						return hr;
					}
					if (hdr.size == 0x10) {
						WORD w = 0;
						if (S_OK != (hr = Read(w))) {
							return hr;
						}
						m_fh.nHeaders = w;
					} else if (S_OK != (hr = Read(m_fh.nHeaders))) {
						return hr;
					}
					break;
				case 'CONT':
					UINT16 slen;
					if (S_OK != (hr = Read(slen))) {
						return hr;
					}
					if (slen > 0 && S_OK != (hr = ByteRead((BYTE*)m_cd.title.GetBufferSetLength(slen), slen))) {
						return hr;
					}
					if (S_OK != (hr = Read(slen))) {
						return hr;
					}
					if (slen > 0 && S_OK != (hr = ByteRead((BYTE*)m_cd.author.GetBufferSetLength(slen), slen))) {
						return hr;
					}
					if (S_OK != (hr = Read(slen))) {
						return hr;
					}
					if (slen > 0 && S_OK != (hr = ByteRead((BYTE*)m_cd.copyright.GetBufferSetLength(slen), slen))) {
						return hr;
					}
					if (S_OK != (hr = Read(slen))) {
						return hr;
					}
					if (slen > 0 && S_OK != (hr = ByteRead((BYTE*)m_cd.comment.GetBufferSetLength(slen), slen))) {
						return hr;
					}
					break;
				case 'PROP':
					if (S_OK != (hr = Read(m_p.maxBitRate))) {
						return hr;
					}
					if (S_OK != (hr = Read(m_p.avgBitRate))) {
						return hr;
					}
					if (S_OK != (hr = Read(m_p.maxPacketSize))) {
						return hr;
					}
					if (S_OK != (hr = Read(m_p.avgPacketSize))) {
						return hr;
					}
					if (S_OK != (hr = Read(m_p.nPackets))) {
						return hr;
					}
					if (S_OK != (hr = Read(m_p.tDuration))) {
						return hr;
					}
					if (S_OK != (hr = Read(m_p.tPreroll))) {
						return hr;
					}
					if (S_OK != (hr = Read(m_p.ptrIndex))) {
						return hr;
					}
					if (S_OK != (hr = Read(m_p.ptrData))) {
						return hr;
					}
					if (S_OK != (hr = Read(m_p.nStreams))) {
						return hr;
					}
					UINT16 flags;
					if (S_OK != (hr = Read(flags))) {
						return hr;
					}
					m_p.flags = (Properies::flags_t)flags;
					break;
				case 'MDPR': {
					std::unique_ptr<MediaProperies> mp(DNew MediaProperies);
					if (S_OK != (hr = Read(mp->stream))) {
						return hr;
					}
					if (S_OK != (hr = Read(mp->maxBitRate))) {
						return hr;
					}
					if (S_OK != (hr = Read(mp->avgBitRate))) {
						return hr;
					}
					if (S_OK != (hr = Read(mp->maxPacketSize))) {
						return hr;
					}
					if (S_OK != (hr = Read(mp->avgPacketSize))) {
						return hr;
					}
					if (S_OK != (hr = Read(mp->tStart))) {
						return hr;
					}
					if (S_OK != (hr = Read(mp->tPreroll))) {
						return hr;
					}
					if (S_OK != (hr = Read(mp->tDuration))) {
						return hr;
					}
					UINT8 slen;
					if (S_OK != (hr = Read(slen))) {
						return hr;
					}
					if (slen > 0 && S_OK != (hr = ByteRead((BYTE*)mp->name.GetBufferSetLength(slen), slen))) {
						return hr;
					}
					if (S_OK != (hr = Read(slen))) {
						return hr;
					}
					if (slen > 0 && S_OK != (hr = ByteRead((BYTE*)mp->mime.GetBufferSetLength(slen), slen))) {
						return hr;
					}
					UINT32 tsdlen;
					if (S_OK != (hr = Read(tsdlen))) {
						return hr;
					}
					mp->typeSpecData.resize(tsdlen);
					if (tsdlen > 0 && S_OK != (hr = ByteRead(mp->typeSpecData.data(), tsdlen))) {
						return hr;
					}
					mp->width = mp->height = 0;
					mp->interlaced = mp->top_field_first = false;
					m_mps.emplace_back(std::move(mp));
					break;
				}
				case 'DATA': {
					std::unique_ptr<DataChunk> dc(DNew DataChunk);
					if (S_OK != (hr = Read(dc->nPackets))) {
						return hr;
					}
					if (S_OK != (hr = Read(dc->ptrNext))) {
						return hr;
					}
					dc->pos = GetPos();
					m_dcs.emplace_back(std::move(dc));
					GetDimensions();
					break;
				}
				case 'INDX': {
					IndexChunkHeader ich;
					if (S_OK != (hr = Read(ich.nIndices))) {
						return hr;
					}
					if (S_OK != (hr = Read(ich.stream))) {
						return hr;
					}
					if (S_OK != (hr = Read(ich.ptrNext))) {
						return hr;
					}
					int stream = GetMasterStream();
					while (ich.nIndices-- > 0) {
						UINT16 object_version;
						if (S_OK != (hr = Read(object_version))) {
							return hr;
						}
						if (object_version == 0) {
							std::unique_ptr<IndexRecord> ir(DNew IndexRecord);
							if (S_OK != (hr = Read(ir->tStart))) {
								return hr;
							}
							if (S_OK != (hr = Read(ir->ptrFilePos))) {
								return hr;
							}
							if (S_OK != (hr = Read(ir->packet))) {
								return hr;
							}
							if (ich.stream == stream) {
								m_irs.emplace_back(std::move(ir));
							}
						}
					}
					break;
				}
				case '.SUB':
					if (hdr.size > sizeof(hdr)) {
						int size = hdr.size - sizeof(hdr);
						std::unique_ptr<char[]> buff(new(std::nothrow) char[size]);
						if (!buff) {
							return E_OUTOFMEMORY;
						}

						char* p = buff.get();
						if (S_OK != (hr = ByteRead((BYTE*)p, size))) {
							return hr;
						}
						for (char* end = p + size; p < end; ) {
							subtitle s;
							s.name = p;
							p += s.name.GetLength()+1;
							CStringA len(p);
							p += len.GetLength()+1;
							s.data = CStringA(p, strtol(len, nullptr, 10));
							p += s.data.GetLength();
							m_subs.emplace_back(s);
						}
					}
					break;
			}
		}

		if (hdr.object_id == 'CONT' && BitRead(32, true) == 'DATA') {
			hdr.size = GetPos() - pos;
		}

		ASSERT(hdr.object_id == 'DATA' || hdr.object_id == 'INDX'
			   || GetPos() == pos + hdr.size
			   || GetPos() == pos + sizeof(hdr));

		pos += hdr.size;
		if (pos > GetPos()) {
			Seek(pos);
		}
	}

	return S_OK;
}

#define GetBits(n) GetBits2(n, p, bit_offset, bit_buffer)
static unsigned int GetBits2(int n, unsigned char*& p, unsigned int& bit_offset, unsigned int& bit_buffer)
{
	unsigned int ret = ((unsigned int)bit_buffer >> (32-(n)));

	bit_offset += n;
	bit_buffer <<= n;
	if (bit_offset > (32-16)) {
		p += bit_offset >> 3;
		bit_offset &= 7;
		bit_buffer = (unsigned int)p[0] << 24;
		bit_buffer |= (unsigned int)p[1] << 16;
		bit_buffer |= (unsigned int)p[2] << 8;
		bit_buffer |= (unsigned int)p[3];
		bit_buffer <<= bit_offset;
	}

	return ret;
}

static void GetDimensions(unsigned char* p, unsigned int* wi, unsigned int* hi)
{
	unsigned int w, h, c;

	const unsigned int cw[8] = {160, 176, 240, 320, 352, 640, 704, 0};
	const unsigned int ch1[8] = {120, 132, 144, 240, 288, 480, 0, 0};
	const unsigned int ch2[4] = {180, 360, 576, 0};

	unsigned int bit_offset = 0;
	unsigned int bit_buffer = *(unsigned int*)p;
	bswap(bit_buffer);

	GetBits(13);

	GetBits(13);

	w = cw[GetBits(3)];
	if (w == 0) {
		do {
			c = GetBits(8);
			w += (c << 2);
		} while (c == 255);
	}

	c = GetBits(3);

	h = ch1[c];
	if (h == 0) {
		c = ((c << 1) | GetBits(1)) & 3;

		h = ch2[c];
		if (h == 0) {
			do {
				c = GetBits(8);
				h += (c << 2);
			} while (c == 255);
		}
	}

	*wi = w;
	*hi = h;
}

static void GetDimensions_X10(unsigned char* p, unsigned int* wi, unsigned int* hi,
							  bool *interlaced, bool *top_field_first, bool *repeat_field)
{
	unsigned int w, h, c;

	const unsigned int cw[8] = {160, 176, 240, 320, 352, 640, 704, 0};
	const unsigned int ch1[8] = {120, 132, 144, 240, 288, 480, 0, 0};
	const unsigned int ch2[4] = {180, 360, 576, 0};

	unsigned int bit_offset = 0;
	unsigned int bit_buffer = *(unsigned int*)p;
	bswap(bit_buffer);

	GetBits(9);

	*interlaced = false;
	*top_field_first = false;
	*repeat_field = false;
	c = GetBits(1);
	if (c) {
		c = GetBits(1);
		if (c) {
			*interlaced = true;
		}
		c = GetBits(1);
		if (c) {
			*top_field_first = true;
		}
		c = GetBits(1);
		if (c) {
			*repeat_field = true;
		}

		GetBits(1);
		c = GetBits(1);
		if (c) {
			GetBits(2);
		}
	}

	GetBits(16);

	w = cw[GetBits(3)];
	if (w == 0) {
		do {
			c = GetBits(8);
			w += (c << 2);
		} while (c == 255);
	}

	c = GetBits(3);

	h = ch1[c];
	if (h == 0) {
		c = ((c << 1) | GetBits(1)) & 3;

		h = ch2[c];
		if (h == 0) {
			do {
				c = GetBits(8);
				h += (c << 2);
			} while (c == 255);
		}
	}

	*wi = w;
	*hi = h;
}

void CRMFile::GetDimensions()
{
	for (const auto& pmp : m_mps) {
		UINT64 filepos = GetPos();

		if (pmp->mime == "video/x-pn-realvideo") {
			pmp->width = pmp->height = 0;

			rvinfo rvi = *(rvinfo*)pmp->typeSpecData.data();
			rvi.bswap();

			if (rvi.fcc2 != '04VR' && rvi.fcc2 != '14VR') {
				continue;
			}

			MediaPacketHeader mph;
			while (S_OK == Read(mph)) {
				if (mph.stream != pmp->stream || mph.len == 0
						|| !(mph.flags&MediaPacketHeader::PN_KEYFRAME_FLAG)) {
					continue;
				}

				BYTE* p = mph.pData.data();
				BYTE* p0 = p;
				int len = mph.pData.size();

				BYTE hdr = *p++;
				DWORD packetlen = 0, packetoffset = 0;

				if ((hdr&0xc0) == 0x40) {
					packetlen = len - (++p - p0);
				} else {
					if ((hdr&0x40) == 0) {
						p++;
					}

#define GetWORD(var) var = (var<<8)|(*p++); var = (var<<8)|(*p++);

					GetWORD(packetlen);
					if ((packetlen&0x4000) == 0) {
						GetWORD(packetlen);
						packetlen &= 0x3fffffff;
					} else {
						packetlen &= 0x3fff;
					}

					GetWORD(packetoffset);
					if ((packetoffset&0x4000) == 0) {
						GetWORD(packetoffset);
						packetoffset &= 0x3fffffff;
					} else {
						packetoffset &= 0x3fff;
					}

#undef GetWORD

					if ((hdr&0xc0) == 0xc0) {
						packetoffset = 0;
					} else if ((hdr&0xc0) == 0x80) {
						packetoffset = packetlen - packetoffset;
					}

					p++;
				}

				len = std::min(len - (int)(p - p0), (int)packetlen - (int)packetoffset);

				if (len > 0) {
					bool repeat_field;
					if (rvi.fcc2 == '14VR') {
						::GetDimensions_X10(p, &pmp->width, &pmp->height, &pmp->interlaced, &pmp->top_field_first, &repeat_field);
					} else {
						::GetDimensions(p, &pmp->width, &pmp->height);
					}

					if (rvi.w == pmp->width && rvi.h == pmp->height) {
						pmp->width = pmp->height = 0;
					}

					break;
				}
			}
		}

		Seek(filepos);
	}
}

int CRMFile::GetMasterStream()
{
	int s1 = -1, s2 = -1;

	for (const auto& pmp : m_mps) {
		if (pmp->mime == "video/x-pn-realvideo") {
			s1 = pmp->stream;
			break;
		} else if (pmp->mime == "audio/x-pn-realaudio" && s2 == -1) {
			s2 = pmp->stream;
		}
	}

	if (s1 == -1) {
		s1 = s2;
	}

	return s1;
}

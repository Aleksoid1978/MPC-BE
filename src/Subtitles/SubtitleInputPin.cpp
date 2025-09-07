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

#include "stdafx.h"
#include "SubtitleInputPin.h"
#include "VobSubFile.h"
#include "RTS.h"
#include "RenderedHdmvSubtitle.h"
#include "XSUBSubtitle.h"
#include <moreuuids.h>
#include <basestruct.h>

// our first format id
#define __GAB1__ "GAB1"

// our tags for __GAB1__ (ushort) + size (ushort)

// "lang" + '0'
#define __GAB1_LANGUAGE__ 0
// (int)start+(int)stop+(char*)line+'0'
#define __GAB1_ENTRY__ 1
// L"lang" + '0'
#define __GAB1_LANGUAGE_UNICODE__ 2
// (int)start+(int)stop+(WCHAR*)line+'0'
#define __GAB1_ENTRY_UNICODE__ 3

// same as __GAB1__, but the size is (uint) and only __GAB1_LANGUAGE_UNICODE__ is valid
#define __GAB2__ "GAB2"

// (BYTE*)
#define __GAB1_RAWTEXTSUBTITLE__ 4

static const bool IsHdmvSub(const CMediaType* pmt)
{
	return (pmt->subtype == MEDIASUBTYPE_DVB_SUBTITLES
			|| (pmt->majortype == MEDIATYPE_Subtitle && pmt->subtype == MEDIASUBTYPE_HDMVSUB)
			|| (pmt->subtype == MEDIASUBTYPE_NULL && pmt->formattype == FORMAT_SubtitleInfo)) // Workaround : support for Haali PGS
			? true
			: false;
}

CSubtitleInputPin::CSubtitleInputPin(CBaseFilter* pFilter, CCritSec* pLock, CCritSec* pSubLock, HRESULT* phr)
	: CBaseInputPin(L"CSubtitleInputPin", pFilter, pLock, phr, L"Input")
	, m_pSubLock(pSubLock)
	, m_bExitDecodingThread(false)
	, m_bStopDecoding(false)
{
	m_bCanReconnectWhenActive = true;
	m_decodeThread = std::thread([this]() {
		DecodeSamples();
	});
}

CSubtitleInputPin::~CSubtitleInputPin()
{
	m_bExitDecodingThread = m_bStopDecoding = true;
	m_condQueueReady.notify_one();
	if (m_decodeThread.joinable()) {
		m_decodeThread.join();
	}
}

HRESULT CSubtitleInputPin::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Text && (pmt->subtype == MEDIASUBTYPE_NULL || pmt->subtype == FOURCCMap((DWORD)0))
			|| pmt->majortype == MEDIATYPE_Subtitle &&
				(pmt->subtype == MEDIASUBTYPE_UTF8
				|| pmt->subtype == MEDIASUBTYPE_SSA
				|| pmt->subtype == MEDIASUBTYPE_ASS
				|| pmt->subtype == MEDIASUBTYPE_ASS2
				|| pmt->subtype == MEDIASUBTYPE_WEBVTT
				|| pmt->subtype == MEDIASUBTYPE_VOBSUB)
			|| pmt->majortype == MEDIATYPE_Video && (pmt->subtype == MEDIASUBTYPE_DVD_SUBPICTURE || pmt->subtype == MEDIASUBTYPE_XSUB)
			|| IsHdmvSub(pmt)
			? S_OK
			: E_FAIL;
}

HRESULT CSubtitleInputPin::CompleteConnect(IPin* pReceivePin)
{
	InvalidateSamples();

	if (m_mt.majortype == MEDIATYPE_Text) {
		if (!(m_pSubStream = DNew CRenderedTextSubtitle(m_pSubLock))) {
			return E_FAIL;
		}
		CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)m_pSubStream.p;
		pRTS->SetName(CString(GetPinName(pReceivePin)) + L" (embeded)");
		pRTS->m_dstScreenSize = DEFSCREENSIZE;
		pRTS->CreateDefaultStyle(DEFAULT_CHARSET);
	} else if (IsHdmvSub(&m_mt)
			|| m_mt.majortype == MEDIATYPE_Subtitle
			|| (m_mt.majortype == MEDIATYPE_Video && (m_mt.subtype == MEDIASUBTYPE_DVD_SUBPICTURE || m_mt.subtype == MEDIASUBTYPE_XSUB))) {

		SUBTITLEINFO* psi = (SUBTITLEINFO*)m_mt.pbFormat;
		DWORD         dwOffset = 0;
		CString       name;
		LCID          lcid = 0;

		if (m_mt.subtype == MEDIASUBTYPE_XSUB) {
			name = CString(GetPinName(pReceivePin));
		} else {
			if (psi != NULL) {
				dwOffset = psi->dwOffset;

				name = ISO6392ToLanguage(psi->IsoLang);
				CStringW lng(psi->IsoLang);
				if (lng.CompareNoCase(name.Left(lng.GetLength())) != 0) {
					name.AppendFormat(L" (%s)", lng);
				}
				lcid = ISO6392ToLcid(psi->IsoLang);

				CString trackName(psi->TrackName);
				trackName.Trim();
				if (!trackName.IsEmpty()) {
					if (!name.IsEmpty()) {
						if (trackName[0] != L'(' && trackName[0] != L'[') {
							name += L",";
						}
						name += L" ";
					}
					name += trackName;
				}
				if (name.IsEmpty()) {
					name = L"Unknown";
				}
			}
		}

		if (m_mt.subtype == MEDIASUBTYPE_UTF8
				/*|| m_mt.subtype == MEDIASUBTYPE_USF*/
				|| m_mt.subtype == MEDIASUBTYPE_SSA
				|| m_mt.subtype == MEDIASUBTYPE_ASS
				|| m_mt.subtype == MEDIASUBTYPE_ASS2
				|| m_mt.subtype == MEDIASUBTYPE_WEBVTT) {
			if (!(m_pSubStream = DNew CRenderedTextSubtitle(m_pSubLock))) {
				return E_FAIL;
			}
			CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)m_pSubStream.p;
			pRTS->SetSubtitleTypeFromGUID(m_mt.subtype);
			pRTS->SetName(name);
			pRTS->m_lcid = lcid;
			pRTS->m_dstScreenSize = DEFSCREENSIZE;
			pRTS->CreateDefaultStyle(DEFAULT_CHARSET);

			if (dwOffset > 0 && m_mt.cbFormat != dwOffset) {
				CMediaType mt = m_mt;
				if (mt.pbFormat[dwOffset+0] != 0xef
						&& mt.pbFormat[dwOffset+1] != 0xbb
						&& mt.pbFormat[dwOffset+2] != 0xfb) {
					dwOffset -= 3;
					mt.pbFormat[dwOffset+0] = 0xef;
					mt.pbFormat[dwOffset+1] = 0xbb;
					mt.pbFormat[dwOffset+2] = 0xbf;
				}

				pRTS->Open(mt.pbFormat + dwOffset, mt.cbFormat - dwOffset, DEFAULT_CHARSET, pRTS->m_name);
			}
		} else if (m_mt.subtype == MEDIASUBTYPE_VOBSUB) {
			if (!(m_pSubStream = DNew CVobSubStream(m_pSubLock))) {
				return E_FAIL;
			}
			CVobSubStream* pVSS = (CVobSubStream*)m_pSubStream.p;
			pVSS->Open(name, m_mt.pbFormat + dwOffset, m_mt.cbFormat - dwOffset);
		} else if (IsHdmvSub(&m_mt)) {
			if (!(m_pSubStream = DNew CRenderedHdmvSubtitle(m_pSubLock, (m_mt.subtype == MEDIASUBTYPE_DVB_SUBTITLES) ? ST_DVB : ST_HDMV, name, lcid))) {
				return E_FAIL;
			}
		} else if (m_mt.subtype == MEDIASUBTYPE_DVD_SUBPICTURE) {
			if (!(m_pSubStream = DNew CVobSubStream(m_pSubLock))) {
				return E_FAIL;
			}
			CVobSubStream* pVSS = (CVobSubStream*)m_pSubStream.p;

			int x, y, arx, ary;
			ExtractDim(&m_mt, x, y, arx, ary);
			UNREFERENCED_PARAMETER(arx);
			UNREFERENCED_PARAMETER(ary);

			CStringA hdr = VobSubDefHeader(x ? x : 720, y ? y : 576);
			pVSS->Open(name, (BYTE*)(LPCSTR)hdr, hdr.GetLength());
		} else if (m_mt.subtype == MEDIASUBTYPE_XSUB) {
			int x, y, arx, ary;
			ExtractDim(&m_mt, x, y, arx, ary);
			if (!x || !y) {
				return E_FAIL;
			}
			SIZE size = {x, y};

			if (!(m_pSubStream = DNew CXSUBSubtitle(m_pSubLock, name, lcid, size))) {
				return E_FAIL;
			}
		}
	}

	AddSubStream(m_pSubStream);

	return __super::CompleteConnect(pReceivePin);
}

HRESULT CSubtitleInputPin::BreakConnect()
{
	InvalidateSamples();

	RemoveSubStream(m_pSubStream);
	m_pSubStream = NULL;

	ASSERT(IsStopped());

	return __super::BreakConnect();
}

STDMETHODIMP CSubtitleInputPin::ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt)
{
	if (m_Connected) {
		InvalidateSamples();

		RemoveSubStream(m_pSubStream);
		m_pSubStream = NULL;

		m_Connected->Release();
		m_Connected = NULL;
	}

	return __super::ReceiveConnection(pConnector, pmt);
}

STDMETHODIMP CSubtitleInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);

	InvalidateSamples();

	if (m_mt.majortype == MEDIATYPE_Text
			|| (m_mt.majortype == MEDIATYPE_Subtitle
			&& (m_mt.subtype == MEDIASUBTYPE_UTF8
				/*|| m_mt.subtype == MEDIASUBTYPE_USF*/
				|| m_mt.subtype == MEDIASUBTYPE_SSA
				|| m_mt.subtype == MEDIASUBTYPE_ASS
				|| m_mt.subtype == MEDIASUBTYPE_ASS2
				|| m_mt.subtype == MEDIASUBTYPE_WEBVTT))) {
		CAutoLock cAutoLock2(m_pSubLock);
		CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)m_pSubStream.p;
		if (pRTS->m_webvtt_allow_clear || pRTS->m_subtitleType != Subtitle::VTT) {
			pRTS->RemoveAll();
			pRTS->CreateSegments();
		}
		// WebVTT can be read as one big blob of data during pin connection, instead of as samples during playback.
		// This depends on how it is being demuxed. So clear only if we previously got data through samples.
	} else if ((m_mt.majortype == MEDIATYPE_Subtitle && m_mt.subtype == MEDIASUBTYPE_VOBSUB)
				|| (m_mt.majortype == MEDIATYPE_Video && m_mt.subtype == MEDIASUBTYPE_DVD_SUBPICTURE)) {
		CAutoLock cAutoLock2(m_pSubLock);
		CVobSubStream* pVSS = (CVobSubStream*)m_pSubStream.p;
		pVSS->RemoveAll();
	} else if (IsHdmvSub(&m_mt)) {
		CAutoLock cAutoLock2(m_pSubLock);
		CRenderedHdmvSubtitle* pHdmvSubtitle = (CRenderedHdmvSubtitle*)m_pSubStream.p;
		pHdmvSubtitle->NewSegment(tStart, tStop, dRate);
	} else if (m_mt.subtype == MEDIASUBTYPE_XSUB) {
		CAutoLock cAutoLock2(m_pSubLock);
		CXSUBSubtitle* pXSUBSubtitle = (CXSUBSubtitle*)m_pSubStream.p;
		pXSUBSubtitle->NewSegment(tStart, tStop, dRate);
	}

	InvalidateSubtitle(tStart, m_pSubStream);

	return __super::NewSegment(tStart, tStop, dRate);
}

STDMETHODIMP CSubtitleInputPin::EndOfStream()
{
	HRESULT hr = __super::EndOfStream();

	if (SUCCEEDED(hr)) {
		std::unique_lock<std::mutex> lock(m_mutexQueue);
		m_sampleQueue.emplace_back(nullptr); // nullptr means end of stream
		lock.unlock();
		m_condQueueReady.notify_one();
	}

	return hr;
}

STDMETHODIMP CSubtitleInputPin::Receive(IMediaSample* pSample)
{
	HRESULT hr = __super::Receive(pSample);
	if (FAILED(hr)) {
		return hr;
	}

	CAutoLock cAutoLock(&m_csReceive);

	REFERENCE_TIME tStart, tStop;
	hr = pSample->GetTime(&tStart, &tStop);

	switch (hr) {
		case S_OK:
			tStart += m_tStart;
			tStop += m_tStart;
			break;
		case VFW_S_NO_STOP_TIME:
			tStart += m_tStart;
			tStop = INVALID_TIME;
			break;
		case VFW_E_SAMPLE_TIME_NOT_SET:
			tStart = tStop = INVALID_TIME;
			break;
		default:
			ASSERT(FALSE);
			return hr;
	}

	if ((tStart == INVALID_TIME || tStop == INVALID_TIME) && !IsHdmvSub(&m_mt)) {
		ASSERT(FALSE);
	} else {
		BYTE* pData = NULL;
		hr = pSample->GetPointer(&pData);
		long len = pSample->GetActualDataLength();
		if (FAILED(hr) || pData == NULL || len <= 0) {
			return hr;
		}

		{
			std::unique_lock<std::mutex> lock(m_mutexQueue);
			m_sampleQueue.emplace_back(DNew SubtitleSample(tStart, tStop, pData, (size_t)len));
			lock.unlock();
			m_condQueueReady.notify_one();
		}
	}

	return S_OK;
}

void  CSubtitleInputPin::DecodeSamples()
{
	SetThreadName((DWORD)-1, "Subtitle Input Pin Thread");

	while(!m_bExitDecodingThread) {
		std::unique_lock<std::mutex> lock(m_mutexQueue);

		auto needStopProcessing = [this]() {
			return m_bStopDecoding || m_bExitDecodingThread;
		};

		auto isQueueReady = [&]() {
			return !m_sampleQueue.empty() || needStopProcessing();
		};

		m_condQueueReady.wait(lock, isQueueReady);
		lock.unlock(); // Release this lock until we can acquire the other one

		REFERENCE_TIME rtInvalidate = -1;

		if (!needStopProcessing()) {
			CAutoLock cAutoLock(m_pSubLock);
			lock.lock(); // Reacquire the lock

			while (!m_sampleQueue.empty() && !needStopProcessing()) {
				const auto& pSample = m_sampleQueue.front();

				if (pSample) {
					const REFERENCE_TIME rtSampleInvalidate = DecodeSample(pSample);
					if (rtSampleInvalidate >= 0 && (rtSampleInvalidate < rtInvalidate || rtInvalidate < 0)) {
						rtInvalidate = rtSampleInvalidate;
					}
				} else { // marker for end of stream
					if (IsHdmvSub(&m_mt)) {
						CRenderedHdmvSubtitle* pHdmvSubtitle = (CRenderedHdmvSubtitle*)m_pSubStream.p;
						pHdmvSubtitle->EndOfStream();
					}
				}

				m_sampleQueue.pop_front();
			}
		}

		if (rtInvalidate >= 0) {
#if (FALSE)
			DLog(L"InvalidateSubtitle() : %I64d", rtInvalidate);
#endif
			// IMPORTANT: m_pSubLock must not be locked when calling this
			InvalidateSubtitle(rtInvalidate, m_pSubStream);
		}
	}
}

REFERENCE_TIME CSubtitleInputPin::DecodeSample(const std::unique_ptr<SubtitleSample>& pSample)
{
	bool bInvalidate = false;

	BYTE* pData = pSample->data.data();
	const size_t nLen = pSample->data.size();
	const REFERENCE_TIME tStart = pSample->rtStart;
	const REFERENCE_TIME tStop = pSample->rtStop;

	if (m_mt.majortype == MEDIATYPE_Text) {
		CAutoLock cAutoLock(m_pSubLock);
		CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)m_pSubStream.p;

		if (!strncmp((char*)pData, __GAB1__, strlen(__GAB1__))) {
			char* ptr = (char*)&pData[strlen(__GAB1__)+1];
			char* end = (char*)&pData[nLen];

			while (ptr < end) {
				WORD tag = *((WORD*)(ptr));
				ptr += 2;
				WORD size = *((WORD*)(ptr));
				ptr += 2;

				if (tag == __GAB1_LANGUAGE__) {
					pRTS->m_name = ptr;
				} else if (tag == __GAB1_ENTRY__) {
					CStringW wstr = ConvertToWStr(ptr+8, CP_ACP); // TODO: code page?
					pRTS->Add(wstr, *(int*)ptr, *(int*)(ptr+4));
					bInvalidate = true;
				} else if (tag == __GAB1_LANGUAGE_UNICODE__) {
					pRTS->m_name = (WCHAR*)ptr;
				} else if (tag == __GAB1_ENTRY_UNICODE__) {
					pRTS->Add((WCHAR*)(ptr+8), *(int*)ptr, *(int*)(ptr+4));
					bInvalidate = true;
				}

				ptr += size;
			}
		} else if (!strncmp((char*)pData, __GAB2__, strlen(__GAB2__))) {
			char* ptr = (char*)&pData[strlen(__GAB2__)+1];
			char* end = (char*)&pData[nLen];

			while (ptr < end) {
				WORD tag = *((WORD*)(ptr));
				ptr += 2;
				DWORD size = *((DWORD*)(ptr));
				ptr += 4;

				if (tag == __GAB1_LANGUAGE_UNICODE__) {
					pRTS->m_name = (WCHAR*)ptr;
				} else if (tag == __GAB1_RAWTEXTSUBTITLE__) {
					pRTS->Open((BYTE*)ptr, size, DEFAULT_CHARSET, pRTS->m_name);
					bInvalidate = true;
				}

				ptr += size;
			}
		} else if (pData != 0 && nLen > 1 && *pData != 0) {
			CStringA str((char*)pData, nLen);

			str.Replace("\r\n", "\n");
			str.Trim();

			if (!str.IsEmpty()) {
				CStringW wstr = ConvertToWStr(str, CP_ACP); // TODO: code page?
				pRTS->Add(wstr, (int)(tStart / 10000), (int)(tStop / 10000));
				bInvalidate = true;
			}
		}
	} else if (IsHdmvSub(&m_mt)
			|| m_mt.majortype == MEDIATYPE_Subtitle
			|| (m_mt.majortype == MEDIATYPE_Video && (m_mt.subtype == MEDIASUBTYPE_DVD_SUBPICTURE || m_mt.subtype == MEDIASUBTYPE_XSUB))) {

		CAutoLock cAutoLock(m_pSubLock);

		if (m_mt.subtype == MEDIASUBTYPE_UTF8 || m_mt.subtype == MEDIASUBTYPE_WEBVTT) {
			CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)m_pSubStream.p;

			CStringW str = UTF8ToWStr(CStringA((LPCSTR)pData, nLen));
			FastTrim(str);
			if (!str.IsEmpty()) {
				pRTS->Add(str, (int)(tStart / 10000), (int)(tStop / 10000));
				bInvalidate = true;
				if (pRTS->m_subtitleType == Subtitle::VTT) {
					pRTS->m_webvtt_allow_clear = true;
				}
			}
		} else if (m_mt.subtype == MEDIASUBTYPE_SSA || m_mt.subtype == MEDIASUBTYPE_ASS || m_mt.subtype == MEDIASUBTYPE_ASS2) {
			CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)m_pSubStream.p;

			CStringW str = UTF8ToWStr(CStringA((LPCSTR)pData, nLen)).Trim();
			if (!str.IsEmpty()) {
				STSEntry stse;

				int fields = m_mt.subtype == MEDIASUBTYPE_ASS2 ? 10 : 9;

				std::list<CStringW> sl;
				Explode(str, sl, L',', fields);
				if (sl.size() == (size_t)fields) {
					auto it = sl.cbegin();

					stse.readorder = wcstol(*it++, NULL, 10);
					stse.layer = wcstol(*it++, NULL, 10);
					stse.style = *it++;
					stse.actor = *it++;
					stse.marginRect.left = wcstol(*it++, NULL, 10);
					stse.marginRect.right = wcstol(*it++, NULL, 10);
					stse.marginRect.top = stse.marginRect.bottom = wcstol(*it++, NULL, 10);
					if (fields == 10) {
						stse.marginRect.bottom = wcstol(*it++, NULL, 10);
					}
					stse.effect = *it++;
					stse.str = *it++;
				}

				if (!stse.str.IsEmpty()) {
					pRTS->Add(stse.str, (int)(tStart / 10000), (int)(tStop / 10000),
							  stse.style, stse.actor, stse.effect, stse.marginRect, stse.layer, stse.readorder);
					bInvalidate = true;
				}
			}
		} else if (m_mt.subtype == MEDIASUBTYPE_VOBSUB
				   || (m_mt.majortype == MEDIATYPE_Video && m_mt.subtype == MEDIASUBTYPE_DVD_SUBPICTURE)) {
			CVobSubStream* pVSS = (CVobSubStream*)m_pSubStream.p;
			pVSS->Add(tStart, tStop, pData, nLen);
		} else if (IsHdmvSub(&m_mt)) {
			CRenderedHdmvSubtitle* pHdmvSubtitle = (CRenderedHdmvSubtitle*)m_pSubStream.p;
			pHdmvSubtitle->ParseSample(pData, nLen, tStart, tStop);
		} else if (m_mt.subtype == MEDIASUBTYPE_XSUB) {
			CXSUBSubtitle* pXSUBSubtitle = (CXSUBSubtitle*)m_pSubStream.p;
			pXSUBSubtitle->ParseSample(pData, nLen);
		}
	}

	return bInvalidate ? tStart : -1;
}

void CSubtitleInputPin::InvalidateSamples()
{
	m_bStopDecoding = true;
	{
		std::lock_guard<std::mutex> lock(m_mutexQueue);
		m_sampleQueue.clear();
		m_bStopDecoding = false;
	}
}

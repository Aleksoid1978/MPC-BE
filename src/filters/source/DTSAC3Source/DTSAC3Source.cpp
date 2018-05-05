/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2018 see Authors.txt
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
#include <MMReg.h>
#include <ks.h>
#include <aviriff.h>
#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include <uuids.h>
#include <moreuuids.h>
#include "DTSAC3Source.h"
#include "../../../DSUtil/AudioParser.h"
#include <atlpath.h>
#include <stdint.h>
#include <mpc_defines.h>
#include "../../../DSUtil/MediaDescription.h"

enum {
	unknown,
	AC3,
	EAC3,
	TrueHD,
	//TrueHDAC3,
	MLP,
	DTS,
	DTSHD,
	DTSPaded,
	DTSExpress
};

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] = {
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_DTS},
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_DTS},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_DOLBY_AC3},
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_DOLBY_AC3},
};

const AMOVIESETUP_PIN sudOpPin[] = {
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, _countof(sudPinTypesOut), sudPinTypesOut}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CDTSAC3Source), DTSAC3SourceName, MERIT_NORMAL, _countof(sudOpPin), sudOpPin, CLSID_LegacyAmFilterCategory}
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CDTSAC3Source>, nullptr, &sudFilter[0]}
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	SetRegKeyValue(
		L"Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}", L"{B4A7BE85-551D-4594-BDC7-832B09185041}",
		L"0", L"0,4,,7FFE8001"); // DTS

	SetRegKeyValue(
		L"Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}", L"{B4A7BE85-551D-4594-BDC7-832B09185041}",
		L"0", L"0,4,,fE7f0180"); // DTS LE

	SetRegKeyValue(
		L"Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}", L"{B4A7BE85-551D-4594-BDC7-832B09185041}",
		L"0", L"0,4,,64582025"); // DTS Substream

	SetRegKeyValue(
		L"Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}", L"{B4A7BE85-551D-4594-BDC7-832B09185041}",
		L"1", L"0,2,,0B77"); // AC3, E-AC3

	SetRegKeyValue(
		L"Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}", L"{B4A7BE85-551D-4594-BDC7-832B09185041}",
		L"0", L"4,4,,F8726FBB"); // MLP

	SetRegKeyValue(
		L"Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}", L"{B4A7BE85-551D-4594-BDC7-832B09185041}",
		L"Source Filter", L"{B4A7BE85-551D-4594-BDC7-832B09185041}");

	SetRegKeyValue(
		L"Media Type\\Extensions", L".dts",
		L"Source Filter", L"{B4A7BE85-551D-4594-BDC7-832B09185041}");

	SetRegKeyValue(
		L"Media Type\\Extensions", L".dtshd",
		L"Source Filter", L"{B4A7BE85-551D-4594-BDC7-832B09185041}");

	SetRegKeyValue(
		L"Media Type\\Extensions", L".ac3",
		L"Source Filter", L"{B4A7BE85-551D-4594-BDC7-832B09185041}");

	SetRegKeyValue(
		L"Media Type\\Extensions", L".eac3",
		L"Source Filter", L"{B4A7BE85-551D-4594-BDC7-832B09185041}");

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	DeleteRegKey(L"Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}", L"{B4A7BE85-551D-4594-BDC7-832B09185041}");
	DeleteRegKey(L"Media Type\\Extensions", L".dts");
	DeleteRegKey(L"Media Type\\Extensions", L".dtshd");
	DeleteRegKey(L"Media Type\\Extensions", L".ac3");
	DeleteRegKey(L"Media Type\\Extensions", L".eac3");

	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

//
// CDTSAC3Source
//

CDTSAC3Source::CDTSAC3Source(LPUNKNOWN lpunk, HRESULT* phr)
	: CBaseSource<CDTSAC3Stream>(L"CDTSAC3Source", lpunk, phr, __uuidof(this))
{
}

CDTSAC3Source::~CDTSAC3Source()
{
}

STDMETHODIMP CDTSAC3Source::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));
	wcscpy_s(pInfo->achName, DTSAC3SourceName);
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

// CDTSAC3Stream

CDTSAC3Stream::CDTSAC3Stream(const WCHAR* wfn, CSource* pParent, HRESULT* phr)
	: CBaseStream(L"CDTSAC3Stream", pParent, phr)
	, m_dataStart(0)
	, m_dataEnd(0)
	, m_subtype(GUID_NULL)
	, m_wFormatTag(WAVE_FORMAT_UNKNOWN)
	, m_channels(0)
	, m_samplerate(0)
	, m_bitrate(0)
	, m_framesize(0)
	, m_fixedframesize(true)
	, m_framelength(0)
	, m_bitdepth(0)
	, m_streamtype(unknown)
{
	CAutoLock cAutoLock(&m_cSharedState);
	CString fn(wfn);
	CFileException ex;
	HRESULT hr = E_FAIL;
	m_AvgTimePerFrame = 0;

	do {
		if (!m_file.Open(fn, CFile::modeRead|CFile::shareDenyNone, &ex)) {
			hr = AmHresultFromWin32 (ex.m_lOsError);
			break;
		}

		BYTE data[8];
		if (m_file.Read(&data, sizeof(data)) != sizeof(data)) {
			break;
		}
		if (GETDWORD(data) == FCC('RIFF') ||		// ignore RIFF files
			GETQWORD(data) == 0x5244484448535444) {	// ignore DTS-HD files ('DTSHDHDR')
			break;
		}

		const CString path = m_file.GetFilePath();
		const CString ext = CPath(m_file.GetFileName()).GetExtension().MakeLower();

		m_file.Seek(0, CFile::begin);
		m_dataStart = 0;
		m_dataEnd   = m_file.GetLength();

		DWORD id = 0;
		if (m_file.Read(&id, sizeof(id)) != sizeof(id)) {
			break;
		}

		if ((__int64)m_file.GetLength() < m_dataEnd) {
			m_dataEnd = m_file.GetLength();
		}

		{ // search first audio frame
			bool deepsearch = false;
			if (ext == L".dts" || ext == L".dtshd" || ext == L".ac3" || ext == L".eac3" || ext == L".thd") { //check only specific extensions
				deepsearch = true; // deep search for specific extensions only
			}

			UINT buflen = (UINT)std::min(64LL * 1024, m_dataEnd - m_dataStart);
			buflen -= (UINT)(m_dataStart % 4096); // tiny optimization
			BYTE* buffer = DNew BYTE[buflen];

			m_file.Seek(m_dataStart, CFile::begin);
			buflen = m_file.Read(buffer, buflen);

			audioframe_t aframe = { 0 };
			m_streamtype = unknown;
			UINT i;
			for (i = 0; i + 12 < buflen; i++) { // looking for DTS or AC3 sync
				if (ParseDTSHeader(buffer + i)) {
					m_streamtype = DTS;
					break;
				} else if (ParseAC3Header(buffer + i)) {
					m_streamtype = AC3;
					break;
				} else if (ParseEAC3Header(buffer + i, &aframe) && aframe.param1 == EAC3_FRAME_TYPE_INDEPENDENT) {
					m_streamtype = EAC3;
					break;
				} else if (ParseMLPHeader(buffer + i)) {
					m_streamtype = MLP;
					break;
				} else if (ParseDTSHDHeader(buffer + i, buflen - i, &aframe) && aframe.param2 == DCA_PROFILE_EXPRESS) {
					m_streamtype = DTSExpress;
					break;
				}

				if (!deepsearch) {
					break;
				}
			}
			delete [] buffer;
			if (m_streamtype == unknown) {
				break;
			}
			m_dataStart += i;
		}

		m_file.Seek(m_dataStart, CFile::begin);
		BYTE buf[40] = { 0 };
		if (m_file.Read(&buf, 40) != 40) {
			break;
		}
		audioframe_t aframe;

		// DTS & DTS-HD
		BOOL x96k = FALSE;
		if (m_streamtype == DTS && ParseDTSHeader(buf, &aframe)) {
			// DTS header
			int fsize     = aframe.size;
			m_samplerate  = aframe.samplerate;
			m_channels    = aframe.channels;
			m_framelength = aframe.samples;
			x96k          = aframe.param2;

			int core_samplerate = m_samplerate;

			// DTS-HD header and zero padded
			DWORD sync = -1;
			int HD_size = 0;
			int zero_bytes = 0;

			m_file.Seek(m_dataStart + fsize, CFile::begin);
			if (m_file.Read(&buf, _countof(buf)) == _countof(buf)) {
				sync = GETDWORD(buf);
				HD_size = ParseDTSHDHeader(buf, _countof(buf), &aframe);
				if (HD_size) {
					m_samplerate = aframe.samplerate;
					m_channels   = aframe.channels;
					m_bitdepth   = aframe.param1;
				}
			}

			if (HD_size) {
				//TODO: get more information about DTS-HD
				m_streamtype = DTSHD;
				m_fixedframesize = false;
			} else if (sync == 0 && fsize < 2048) { // zero padded?
				m_file.Seek(m_dataStart + 2048, CFile::begin);
				m_file.Read(&sync, sizeof(sync));
				if (sync == id) {
					zero_bytes = 2048 - fsize;
					m_streamtype = DTSPaded;
				}
			}

			// calculate actual bitrate
			m_framesize = fsize + zero_bytes;
			m_bitrate   = int(m_framesize * 8i64 * core_samplerate / m_framelength);

			if (HD_size) {
				m_framesize += HD_size;
				if (aframe.param2 == DCA_PROFILE_HD_HRA) {
					m_bitrate += CalcBitrate(aframe);
				} else {
					m_bitrate = 0;
				}
			}

			// calculate framesize to support a sonic audio decoder 4.3 (TODO: make otherwise)
			// sonicDTSminsize = fsize + HD_size + 4096
			int k = (m_framesize + 4096 + m_framesize - 1) / m_framesize;
			m_framesize   *= k;
			m_framelength *= k;

			m_wFormatTag = WAVE_FORMAT_DTS;
			m_subtype = MEDIASUBTYPE_DTS;
		}
		// DTS Express
		else if (m_streamtype == DTSExpress && ParseDTSHDHeader(buf, _countof(buf), &aframe)) {
			m_samplerate  = aframe.samplerate;
			m_channels    = aframe.channels;
			m_bitdepth    = aframe.param1;
			m_framesize   = aframe.size;
			m_framelength = aframe.samples;
			m_bitrate     = CalcBitrate(aframe);

			m_wFormatTag  = WAVE_FORMAT_DTS;
			m_subtype     = MEDIASUBTYPE_DTS;
		}
		// AC3 & E-AC3
		else if (m_streamtype == AC3 && ParseAC3Header(buf, &aframe)) {
			int fsize     = aframe.size;
			m_samplerate  = aframe.samplerate;
			m_channels    = aframe.channels;
			m_framelength = aframe.samples;
			m_bitrate     = aframe.param1;

			bool bAC3 = true;
			m_file.Seek(m_dataStart + fsize, CFile::begin);
			if (m_file.Read(&buf, 8) == 8) {
				int fsize2 = ParseEAC3Header(buf, &aframe);
				if (fsize2 > 0 && aframe.param1 == EAC3_FRAME_TYPE_DEPENDENT) {
					fsize += fsize2;
					m_channels += aframe.channels / 2; // TODO
					m_bitrate = (int)(fsize * 8i64 * m_samplerate / m_framelength);
					bAC3 = false;
				}
			}

			// calculate framesize to support a sonic audio decoder 4.3 (TODO: make otherwise)
			// sonicAC3minsize = framesize + 64
			m_framesize = fsize * 2;
			m_framelength *= 2;

			m_wFormatTag = WAVE_FORMAT_UNKNOWN;
			m_subtype = bAC3 ? MEDIASUBTYPE_DOLBY_AC3 : MEDIASUBTYPE_DOLBY_DDPLUS;
		}
		// E-AC3
		else if (m_streamtype == EAC3 && ParseEAC3Header(buf, &aframe)) {
			int fsize     = aframe.size;
			m_samplerate  = aframe.samplerate;
			m_channels    = aframe.channels;
			m_framelength = aframe.samples;

			m_file.Seek(m_dataStart + fsize, CFile::begin);
			if (m_file.Read(&buf, 8) == 8) {
				int fsize2 = ParseEAC3Header(buf, &aframe);
				if (fsize2 > 0 && aframe.param1 == EAC3_FRAME_TYPE_DEPENDENT) {
					fsize += fsize2;
					m_channels += aframe.channels / 2; // TODO
				}
			}
			m_bitrate = (int)(fsize * 8i64 * m_samplerate / m_framelength);

			// calculate framesize to support a sonic audio decoder 4.3 (TODO: make otherwise)
			// sonicAC3minsize = framesize + 64
			m_framesize = fsize * 2;
			m_framelength *= 2;

			m_wFormatTag = WAVE_FORMAT_UNKNOWN;
			m_subtype = MEDIASUBTYPE_DOLBY_DDPLUS;
		}
		// MLP
		else if (m_streamtype == MLP && ParseMLPHeader(buf, &aframe)) {
			m_framesize   = std::max(aframe.size, 1024);
			m_samplerate  = aframe.samplerate;
			m_channels    = aframe.channels;
			m_framelength = aframe.samples;
			m_bitdepth    = aframe.param1;
			if (aframe.param2) {
				m_streamtype = TrueHD;
				m_subtype    = MEDIASUBTYPE_DOLBY_TRUEHD;
			} else {
				m_streamtype = MLP;
				m_subtype    = MEDIASUBTYPE_MLP;
			}
			m_fixedframesize = false;

			m_bitrate = (int)(aframe.size * 8i64 * m_samplerate / m_framelength); // inaccurate, because framesize is not constant

			m_wFormatTag = WAVE_FORMAT_UNKNOWN;
		} else {
			break;
		}

		if (m_samplerate > 0) {
			m_AvgTimePerFrame = (10000000i64 * m_framelength / m_samplerate) << x96k;
		}

		m_rtDuration = m_AvgTimePerFrame * (m_dataEnd - m_dataStart) / m_framesize;
		m_rtStop = m_rtDuration;

		hr = S_OK;

		CMediaType mt;
		if (SUCCEEDED(GetMediaType(0, &mt))) {
			SetName(GetMediaTypeDesc(&mt, L"Output"));
		}
	} while (false);

	if (phr) {
		*phr = hr;
	}
}

CDTSAC3Stream::~CDTSAC3Stream()
{
}

HRESULT CDTSAC3Stream::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
	ASSERT(pAlloc);
	ASSERT(pProperties);

	HRESULT hr = NOERROR;

	pProperties->cBuffers = 1;
	pProperties->cbBuffer = m_framesize;

	ALLOCATOR_PROPERTIES Actual;
	if (FAILED(hr = pAlloc->SetProperties(pProperties, &Actual))) {
		return hr;
	}

	if (Actual.cbBuffer < pProperties->cbBuffer) {
		return E_FAIL;
	}
	ASSERT(Actual.cBuffers == pProperties->cBuffers);

	return NOERROR;
}

HRESULT CDTSAC3Stream::FillBuffer(IMediaSample* pSample, int nFrame, BYTE* pOut, long& len)
{
	BYTE* pOutOrg = pOut;

	const GUID* majortype = &m_mt.majortype;
	const GUID* subtype = &m_mt.subtype;
	UNREFERENCED_PARAMETER(subtype);

	if (*majortype == MEDIATYPE_Audio) {
		m_file.Seek(m_dataStart + (__int64)nFrame * m_framesize, CFile::begin);
		if ((int)m_file.Read(pOut, m_framesize) < m_framesize) {
			return S_FALSE;
		}
		pOut += m_framesize;
	}

	len = (long)(pOut - pOutOrg);

	return S_OK;
}

HRESULT CDTSAC3Stream::GetMediaType(int iPosition, CMediaType* pmt)
{
	CAutoLock cAutoLock(m_pFilter->pStateLock());

	if (iPosition == 0) {
		pmt->majortype = MEDIATYPE_Audio;
		pmt->subtype = m_subtype;
		pmt->formattype = FORMAT_WaveFormatEx;

		WAVEFORMATEX* wfe = (WAVEFORMATEX*)pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX));
		wfe->wFormatTag      = m_wFormatTag;
		wfe->nChannels       = m_channels;
		wfe->nSamplesPerSec  = m_samplerate;
		wfe->nAvgBytesPerSec = (m_bitrate + 4) / 8;
		wfe->nBlockAlign     = m_framesize < WORD_MAX ? m_framesize : WORD_MAX;
		wfe->wBitsPerSample  = m_bitdepth;
		wfe->cbSize = 0;

		if (m_streamtype == MLP || m_streamtype == TrueHD) {
			pmt->SetSampleSize(0);
		}

	} else {
		return VFW_S_NO_MORE_ITEMS;
	}

	pmt->SetTemporalCompression(FALSE);

	return S_OK;
}

HRESULT CDTSAC3Stream::CheckMediaType(const CMediaType* pmt)
{
	if (pmt->majortype == MEDIATYPE_Audio && pmt->formattype == FORMAT_WaveFormatEx) {
		WORD& wFmtTag = ((WAVEFORMATEX*)pmt->pbFormat)->wFormatTag;

		if (pmt->subtype == MEDIASUBTYPE_DTS && wFmtTag == WAVE_FORMAT_DTS) {
			return S_OK;
		} else if (pmt->subtype == MEDIASUBTYPE_DTS2 && wFmtTag == WAVE_FORMAT_DTS2) {
			return S_OK;
		} else if (pmt->subtype == MEDIASUBTYPE_DOLBY_AC3 && (wFmtTag == WAVE_FORMAT_UNKNOWN || wFmtTag == WAVE_FORMAT_DOLBY_AC3)) {
			return S_OK;
		} else if (pmt->subtype == MEDIASUBTYPE_DOLBY_DDPLUS && wFmtTag == WAVE_FORMAT_UNKNOWN) {
			return S_OK;
		} else if (pmt->subtype == MEDIASUBTYPE_MLP && wFmtTag == WAVE_FORMAT_UNKNOWN) {
			return S_OK;
		} else if (pmt->subtype == MEDIASUBTYPE_DOLBY_TRUEHD && wFmtTag == WAVE_FORMAT_UNKNOWN) {
		    return S_OK;
		}
	}
	return E_INVALIDARG;
}

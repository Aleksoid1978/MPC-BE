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
#include "BaseMuxerOutputPin.h"

#include <aviriff.h>
#include "DSUtil/FileHandle.h"

#include <wmcodecdsp.h>
#include <moreuuids.h>

//
// CBaseMuxerOutputPin
//

CBaseMuxerOutputPin::CBaseMuxerOutputPin(LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseOutputPin(L"CBaseMuxerOutputPin", pFilter, pLock, phr, pName)
{
}

IBitStream* CBaseMuxerOutputPin::GetBitStream()
{
	if (!m_pBitStream) {
		if (CComQIPtr<IStream> pStream = GetConnected()) {
			m_pBitStream = DNew CBitStream(pStream, true);
		}
	}

	return m_pBitStream;
}

HRESULT CBaseMuxerOutputPin::BreakConnect()
{
	m_pBitStream.Release();

	return __super::BreakConnect();
}

HRESULT CBaseMuxerOutputPin::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
	ASSERT(pAlloc);
	ASSERT(pProperties);

	HRESULT hr = NOERROR;

	pProperties->cBuffers = 1;
	pProperties->cbBuffer = 1;

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

HRESULT CBaseMuxerOutputPin::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Stream && pmt->subtype == MEDIASUBTYPE_NULL
		   ? S_OK
		   : E_INVALIDARG;
}

HRESULT CBaseMuxerOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
	CAutoLock cAutoLock(m_pLock);

	if (iPosition < 0) {
		return E_INVALIDARG;
	}
	if (iPosition > 0) {
		return VFW_S_NO_MORE_ITEMS;
	}

	pmt->ResetFormatBuffer();
	pmt->InitMediaType();
	pmt->majortype = MEDIATYPE_Stream;
	pmt->subtype = MEDIASUBTYPE_NULL;
	pmt->formattype = FORMAT_None;

	return S_OK;
}

HRESULT CBaseMuxerOutputPin::DeliverEndOfStream()
{
	m_pBitStream.Release();

	return __super::DeliverEndOfStream();
}

STDMETHODIMP CBaseMuxerOutputPin::Notify(IBaseFilter* pSender, Quality q)
{
	return E_NOTIMPL;
}

//
// CBaseMuxerRawOutputPin
//

CBaseMuxerRawOutputPin::CBaseMuxerRawOutputPin(LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseMuxerOutputPin(pName, pFilter, pLock, phr)
{
}

STDMETHODIMP CBaseMuxerRawOutputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		QI(IBaseMuxerRelatedPin)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

void CBaseMuxerRawOutputPin::MuxHeader(const CMediaType& mt)
{
	CComQIPtr<IBitStream> pBitStream = GetBitStream();
	if (!pBitStream) {
		return;
	}

	const BYTE utf8bom[3] = {0xef, 0xbb, 0xbf};

	if ((mt.subtype == FOURCCMap('1CVA') || mt.subtype == FOURCCMap('1cva')) && mt.formattype == FORMAT_MPEG2_VIDEO) {
		MPEG2VIDEOINFO* m2vi = (MPEG2VIDEOINFO*)mt.Format();

		for (DWORD i = 0; i < m2vi->cbSequenceHeader - 2; i += 2) {
			pBitStream->BitWrite(0x00000001, 32);
			WORD size = (((BYTE*)m2vi->dwSequenceHeader)[i + 0] << 8) | ((BYTE*)m2vi->dwSequenceHeader)[i + 1];
			pBitStream->ByteWrite(&((BYTE*)m2vi->dwSequenceHeader)[i + 2], size);
			i += size;
		}
	} else if (mt.subtype == MEDIASUBTYPE_UTF8) {
		pBitStream->ByteWrite(utf8bom, sizeof(utf8bom));
	} else if (mt.subtype == MEDIASUBTYPE_SSA || mt.subtype == MEDIASUBTYPE_ASS || mt.subtype == MEDIASUBTYPE_ASS2) {
		SUBTITLEINFO* si = (SUBTITLEINFO*)mt.Format();
		BYTE* p = (BYTE*)si + si->dwOffset;

		if (memcmp(utf8bom, p, 3) != 0) {
			pBitStream->ByteWrite(utf8bom, sizeof(utf8bom));
		}

		CStringA str((char*)p, mt.FormatLength() - (ULONG)(p - mt.Format()));
		pBitStream->StrWrite(str + '\n', true);

		if (str.Find("[Events]") < 0) {
			pBitStream->StrWrite("\n\n[Events]\n", true);
		}
	} else if (mt.subtype == MEDIASUBTYPE_VOBSUB) {
		m_idx.clear();
	} else if (mt.majortype == MEDIATYPE_Audio
			   && (mt.subtype == MEDIASUBTYPE_PCM
				   || mt.subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO
				   || mt.subtype == FOURCCMap(WAVE_FORMAT_EXTENSIBLE)
				   || mt.subtype == FOURCCMap(WAVE_FORMAT_IEEE_FLOAT))
			   && mt.formattype == FORMAT_WaveFormatEx) {
		pBitStream->BitWrite('RIFF', 32);
		pBitStream->BitWrite(0, 32); // file length - 8, set later
		pBitStream->BitWrite('WAVE', 32);

		pBitStream->BitWrite('fmt ', 32);
		pBitStream->ByteWrite(&mt.cbFormat, 4);
		pBitStream->ByteWrite(mt.pbFormat, mt.cbFormat);

		pBitStream->BitWrite('data', 32);
		pBitStream->BitWrite(0, 32); // data length, set later
	}
}

void CBaseMuxerRawOutputPin::MuxPacket(const CMediaType& mt, const MuxerPacket* pPacket)
{
	CComQIPtr<IBitStream> pBitStream = GetBitStream();
	if (!pBitStream) {
		return;
	}

	const BYTE* pData = (BYTE*)pPacket->data.get();
	const int DataSize = (int)pPacket->size;

	if (mt.subtype == MEDIASUBTYPE_RAW_AAC1 && mt.formattype == FORMAT_WaveFormatEx) {
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();

		int profile = 0;

		int srate_idx = 11;
		if (92017 <= wfe->nSamplesPerSec) {
			srate_idx = 0;
		} else if (75132 <= wfe->nSamplesPerSec) {
			srate_idx = 1;
		} else if (55426 <= wfe->nSamplesPerSec) {
			srate_idx = 2;
		} else if (46009 <= wfe->nSamplesPerSec) {
			srate_idx = 3;
		} else if (37566 <= wfe->nSamplesPerSec) {
			srate_idx = 4;
		} else if (27713 <= wfe->nSamplesPerSec) {
			srate_idx = 5;
		} else if (23004 <= wfe->nSamplesPerSec) {
			srate_idx = 6;
		} else if (18783 <= wfe->nSamplesPerSec) {
			srate_idx = 7;
		} else if (13856 <= wfe->nSamplesPerSec) {
			srate_idx = 8;
		} else if (11502 <= wfe->nSamplesPerSec) {
			srate_idx = 9;
		} else if (9391 <= wfe->nSamplesPerSec) {
			srate_idx = 10;
		}

		int channels = wfe->nChannels;

		if (wfe->cbSize >= 2) {
			BYTE* p = (BYTE*)(wfe + 1);
			profile = (p[0] >> 3) - 1;
			srate_idx = ((p[0] & 7) << 1) | ((p[1] & 0x80) >> 7);
			channels = (p[1] >> 3) & 15;
		}

		int len = (DataSize + 7) & 0x1fff;

		BYTE hdr[7] = {0xff, 0xf9};
		hdr[2] = BYTE((profile << 6) | (srate_idx << 2) | ((channels & 4) >> 2));
		hdr[3] = BYTE(((channels & 3) << 6) | (len >> 11));
		hdr[4] = (len >> 3) & 0xff;
		hdr[5] = ((len & 7) << 5) | 0x1f;
		hdr[6] = 0xfc;

		pBitStream->ByteWrite(hdr, sizeof(hdr));
	} else if ((mt.subtype == FOURCCMap('1CVA') || mt.subtype == FOURCCMap('1cva')) && mt.formattype == FORMAT_MPEG2_VIDEO) {
		const BYTE* p = pData;
		int i = DataSize;

		while (i >= 4) {
			DWORD len = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];

			i -= len + 4;
			p += len + 4;
		}

		if (i == 0) {
			p = pData;
			i = DataSize;

			while (i >= 4) {
				DWORD len = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];

				pBitStream->BitWrite(0x00000001, 32);

				p += 4;
				i -= 4;

				if (len > (DWORD)i || len == 1) {
					len = i;
					ASSERT(0);
				}

				pBitStream->ByteWrite(p, len);

				p += len;
				i -= len;
			}

			return;
		}
	} else if (mt.subtype == MEDIASUBTYPE_UTF8 || mt.majortype == MEDIATYPE_Text) {
		CStringA str((char*)pData, DataSize);
		str.Trim();
		if (str.IsEmpty()) {
			return;
		}

		TimeCode_t start = ReftimeToTimecode(pPacket->rtStart);
		TimeCode_t stop  = ReftimeToTimecode(pPacket->rtStop);

		str.Format("%d\n%02d:%02d:%02d,%03d --> %02d:%02d:%02d,%03d\n%s\n\n",
				   pPacket->index + 1,
				   start.Hours, start.Minutes, start.Seconds, start.Milliseconds,
				   stop.Hours, stop.Minutes, stop.Seconds, start.Milliseconds,
				   CStringA(str));

		pBitStream->StrWrite(str, true);

		return;
	} else if (mt.subtype == MEDIASUBTYPE_SSA || mt.subtype == MEDIASUBTYPE_ASS || mt.subtype == MEDIASUBTYPE_ASS2) {
		CStringA str((char*)pData, DataSize);
		str.Trim();
		if (str.IsEmpty()) {
			return;
		}

		size_t fields = mt.subtype == MEDIASUBTYPE_ASS2 ? 10 : 9;

		std::list<CStringA> sl;
		Explode(str, sl, ',', fields);
		if (sl.size() < fields) {
			return;
		}

		auto it = sl.begin();

		CStringA readorder = *it++; // TODO
		CStringA layer = *it++;
		CStringA style = *it++;
		CStringA actor = *it++;
		CStringA left  = *it++;
		CStringA right = *it++;
		CStringA top   = *it++;
		if (fields == 10) {
			top += ',' + *it++;    // bottom
		}
		CStringA effect = *it++;
		str = *it++;

		if (mt.subtype == MEDIASUBTYPE_SSA) {
			layer = "Marked=0";
		}

		TimeCode_t start = ReftimeToTimecode(pPacket->rtStart);
		TimeCode_t stop = ReftimeToTimecode(pPacket->rtStop);

		str.Format("Dialogue: %s,%d:%02d:%02d.%02d,%d:%02d:%02d.%02d,%s,%s,%s,%s,%s,%s,%s\n",
				   layer,
				   start.Hours, start.Minutes, start.Seconds, start.Milliseconds,
				   stop.Hours, stop.Minutes, stop.Seconds, stop.Milliseconds,
				   style, actor, left, right, top, effect,
				   CStringA(str));

		pBitStream->StrWrite(str, true);

		return;
	} else if (mt.subtype == MEDIASUBTYPE_VOBSUB) {
		bool fTimeValid = pPacket->IsTimeValid();

		if (fTimeValid) {
			idx_t i;
			i.rt = pPacket->rtStart;
			i.fp = pBitStream->GetPos();
			m_idx.emplace_back(i);
		}

		int DataSizeLeft = DataSize;

		while (DataSizeLeft > 0) {
			int BytesAvail = 0x7ec - (fTimeValid ? 9 : 4);
			int Size = std::min(BytesAvail, DataSizeLeft);
			int Padding = 0x800 - Size - 20 - (fTimeValid ? 9 : 4);

			pBitStream->BitWrite(0x000001ba, 32);
			pBitStream->BitWrite(0x440004000401ui64, 48);
			pBitStream->BitWrite(0x000003f8, 32);
			pBitStream->BitWrite(0x000001bd, 32);

			if (fTimeValid) {
				pBitStream->BitWrite(Size + 9, 16);
				pBitStream->BitWrite(0x8180052100010001ui64, 64);
			} else {
				pBitStream->BitWrite(Size + 4, 16);
				pBitStream->BitWrite(0x810000, 24);
			}

			pBitStream->BitWrite(0x20, 8);

			pBitStream->ByteWrite(pData, Size);

			pData += Size;
			DataSizeLeft -= Size;

			if (Padding > 0) {
				Padding -= 6;
				ASSERT(Padding >= 0);
				pBitStream->BitWrite(0x000001be, 32);
				pBitStream->BitWrite(Padding, 16);
				while (Padding-- > 0) {
					pBitStream->BitWrite(0xff, 8);
				}
			}

			fTimeValid = false;
		}

		return;
	} else if (mt.subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO) {
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();

		// This code is probably totally broken for anything but 16 bits
		for (int i = 0, bps = wfe->wBitsPerSample / 8; i < DataSize; i += bps)
			for (int j = bps - 1; j >= 0; j--) {
				pBitStream->BitWrite(pData[i + j], 8);
			}

		return;
	}
	// else // TODO: restore more streams (vorbis to ogg)

	pBitStream->ByteWrite(pData, DataSize);
}

void CBaseMuxerRawOutputPin::MuxFooter(const CMediaType& mt)
{
	CComQIPtr<IBitStream> pBitStream = GetBitStream();
	if (!pBitStream) {
		return;
	}

	if (mt.majortype == MEDIATYPE_Audio
			&& (mt.subtype == MEDIASUBTYPE_PCM
				|| mt.subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO
				|| mt.subtype == FOURCCMap(WAVE_FORMAT_EXTENSIBLE)
				|| mt.subtype == FOURCCMap(WAVE_FORMAT_IEEE_FLOAT))
			&& mt.formattype == FORMAT_WaveFormatEx) {
		pBitStream->BitFlush();

		ASSERT(pBitStream->GetPos() <= 0xffffffff);
		UINT32 size = (UINT32)pBitStream->GetPos();

		size -= 8;
		pBitStream->Seek(4);
		pBitStream->ByteWrite(&size, 4);

		size -= sizeof(RIFFLIST) + sizeof(RIFFCHUNK) + mt.FormatLength();
		pBitStream->Seek(sizeof(RIFFLIST) + sizeof(RIFFCHUNK) + mt.FormatLength() + 4);
		pBitStream->ByteWrite(&size, 4);
	} else if (mt.subtype == MEDIASUBTYPE_VOBSUB) {
		if (CComQIPtr<IFileSinkFilter> pFSF = GetFilterFromPin(GetConnected())) {
			WCHAR* fn = nullptr;
			if (SUCCEEDED(pFSF->GetCurFile(&fn, nullptr))) {
				const CStringW path = GetRenameFileExt(fn, L".idx");
				CoTaskMemFree(fn);

				FILE* f;
				if (!_wfopen_s(&f, path, L"w")) {
					SUBTITLEINFO* si = (SUBTITLEINFO*)mt.Format();

					fwprintf_s(f, L"%s\n", L"# VobSub index file, v7 (do not modify this line!)");

					fwrite(mt.Format() + si->dwOffset, mt.FormatLength() - si->dwOffset, 1, f);

					CStringA iso6391(ISO6392To6391(si->IsoLang));
					if (iso6391.IsEmpty()) {
						iso6391 = "--";
					}
					fwprintf_s(f, L"\nlangidx: 0\n\nid: %S, index: 0\n", iso6391.GetString());

					CStringW alt = si->TrackName;
					if (alt.GetLength()) {
						fwprintf_s(f, L"alt: %s\n", alt.GetString());
					}

					for (const auto& item : m_idx) {
						TimeCode_t start = ReftimeToTimecode(item.rt);
						fwprintf_s(f, L"timestamp: %02d:%02d:%02d:%03d, filepos: %09I64x\n",
									start.Hours, start.Minutes, start.Seconds, start.Milliseconds,
									item.fp);
					}

					fclose(f);
				}
			}
		}
	}
}

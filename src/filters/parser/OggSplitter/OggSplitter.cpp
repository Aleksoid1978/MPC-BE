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
#include <mmreg.h>
#include "OggSplitter.h"
#include "../../../DSUtil/CUE.h"
#include "../../../DSUtil/DSUtil.h"
#include "../../../DSUtil/GolombBuffer.h"
#include "../../../DSUtil/VideoParser.h"

#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include <moreuuids.h>
#include <basestruct.h>

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, nullptr, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, 0, nullptr}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(COggSplitterFilter), OggSplitterName, MERIT_NORMAL+1, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(COggSourceFilter), OggSourceName, MERIT_NORMAL+1, 0, nullptr, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<COggSplitterFilter>, nullptr, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<COggSourceFilter>, nullptr, &sudFilter[1]},
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	RegisterSourceFilter(
		CLSID_AsyncReader,
		MEDIASUBTYPE_Ogg,
		L"0,4,,4F676753", // OggS
		L".ogg", L".ogm", nullptr);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	UnRegisterSourceFilter(MEDIASUBTYPE_Ogg);

	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

//
// bitstream
//

class bitstream
{
	BYTE* m_p;
	int m_len, m_pos;
public:
	bitstream(BYTE* p, int len, bool rev = false) : m_p(p), m_len(len*8) {
		m_pos = !rev ? 0 : len*8;
	}
	bool hasbits(int cnt) {
		int pos = m_pos+cnt;
		return(pos >= 0 && pos <= m_len);
	}
	unsigned int showbits(int cnt) { // a bit unclean, but works and can read backwards too! :P
		if (!hasbits(cnt)) {
			//ASSERT(0);
			return 0;
		}
		unsigned int ret = 0, off = 0;
		BYTE* p = m_p;
		if (cnt < 0) {
			p += (m_pos+cnt)>>3;
			off = (m_pos+cnt)&7;
			cnt = abs(cnt);
			ret = (*p++&(~0<<off))>>off;
			off = 8 - off;
			cnt -= off;
		} else {
			p += m_pos>>3;
			off = m_pos&7;
			ret = (*p++>>off)&((1<<std::min(cnt, 8))-1);
			off = 0;
			cnt -= 8 - off;
		}
		while (cnt > 0) {
			ret |= (*p++&((1<<std::min(cnt, 8))-1)) << off;
			off += 8;
			cnt -= 8;
		}
		return ret;
	}
	unsigned int getbits(int cnt) {
		unsigned int ret = showbits(cnt);
		m_pos += cnt;
		return ret;
	}
};

//
// COggSplitterFilter
//

COggSplitterFilter::COggSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(L"COggSplitterFilter", pUnk, phr, __uuidof(this))
{
	m_nFlag |= SOURCE_SUPPORT_URL;
}

COggSplitterFilter::~COggSplitterFilter()
{
}

STDMETHODIMP COggSplitterFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	if (m_pName && m_pName[0]==L'M' && m_pName[1]==L'P' && m_pName[2]==L'C') {
		(void)StringCchCopyW(pInfo->achName, NUMELMS(pInfo->achName), m_pName);
	} else {
		wcscpy_s(pInfo->achName, OggSourceName);
	}
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

#define PinNotExist !GetOutputPin(page.m_hdr.bitstream_serial_number)

HRESULT COggSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hres = E_FAIL;

	m_pFile.Free();

	m_pFile.Attach(DNew COggFile(pAsyncReader, hres));
	if (!m_pFile) {
		return E_OUTOFMEMORY;
	}
	if (FAILED(hres)) {
		m_pFile.Free();
		return hres;
	}
	m_pFile->SetBreakHandle(GetRequestHandle());

	m_rtDuration = m_rtNewStart = m_rtCurrent = m_rtNewStop = m_rtStop = 0;

	__int64 start_pos = 0;

start:

	m_pFile->Seek(start_pos);

	std::map<WORD, BOOL> streamMoreInit;
	int streamId = 0;

	OggPage page;
	for (int i = 0; m_pFile->ReadPages(page), i < 30; i++) {
		BYTE* p = page.data();
		if (!p) {
			continue;
		}

		if (!(page.m_hdr.header_type_flag & OggPageHeader::continued)) {
			if (!memcmp(p, "fishead", 7) || !memcmp(p, "fisbone", 7)) {
				continue;
			}

			BYTE type = *p++;

			CStringW name;

			HRESULT hr;

			if (type >= 0x80 && type <= 0x82 && !memcmp(p, "theora", 6)) {
				if (type == 0x80 && PinNotExist) {
					name.Format(L"Theora %d", streamId++);
					CAutoPtr<CBaseSplitterOutputPin> pPinOut;
					pPinOut.Attach(DNew COggTheoraOutputPin(page, name, this, this, &hr));
					AddOutputPin(page.m_hdr.bitstream_serial_number, pPinOut);
					streamMoreInit[page.m_hdr.bitstream_serial_number] = TRUE;
				} else if (type == 0x81) {
					if (COggTheoraOutputPin* pOggPin = dynamic_cast<COggTheoraOutputPin*>(GetOutputPin(page.m_hdr.bitstream_serial_number))) {
						pOggPin->AddComment(page.data() + 7, page.size() - 7);
					}
				}
			} else if (type == 1 && (page.m_hdr.header_type_flag & OggPageHeader::first)) {
				if (PinNotExist) {
					CAutoPtr<CBaseSplitterOutputPin> pPinOut;

					if (!memcmp(p, "vorbis", 6)) {
						if ((*(OggVorbisIdHeader*)(p + 6)).audio_sample_rate == 0) {
							return E_FAIL; // fix crash on broken files
						}
						name.Format(L"Vorbis %d", streamId++);
						pPinOut.Attach(DNew COggVorbisOutputPin((OggVorbisIdHeader*)(p + 6), name, this, this, &hr));
						m_bitstream_serial_number_start = m_bitstream_serial_number_last = page.m_hdr.bitstream_serial_number;
						streamMoreInit[page.m_hdr.bitstream_serial_number] = TRUE;
					} else if (!memcmp(p, "video", 5)) {
						name.Format(L"Video %d", streamId++);
						pPinOut.Attach(DNew COggVideoOutputPin((OggStreamHeader*)p, name, this, this, &hr));
						m_bitstream_serial_number_Video = page.m_hdr.bitstream_serial_number;
					} else if (!memcmp(p, "audio", 5)) {
						name.Format(L"Audio %d", streamId++);
						pPinOut.Attach(DNew COggAudioOutputPin((OggStreamHeader*)p, name, this, this, &hr));
					} else if (!memcmp(p, "text", 4)) {
						name.Format(L"Text %d", streamId++);
						pPinOut.Attach(DNew COggTextOutputPin((OggStreamHeader*)p, name, this, this, &hr));
					} else if (!memcmp(p, "Direct Show Samples embedded in Ogg", 35)) {
						name.Format(L"DirectShow %d", streamId++);
						AM_MEDIA_TYPE* pmt = (AM_MEDIA_TYPE*)(p + 35 + sizeof(GUID));
						pPinOut.Attach(DNew COggDirectShowOutputPin(pmt, name, this, this, &hr));
						if (pmt->majortype == MEDIATYPE_Video) {
							m_bitstream_serial_number_Video = page.m_hdr.bitstream_serial_number;
						}
					}

					AddOutputPin(page.m_hdr.bitstream_serial_number, pPinOut);
				}
			} else if (type == 3 && !memcmp(p, "vorbis", 6)) {
				if (COggSplitterOutputPin* pOggPin = dynamic_cast<COggSplitterOutputPin*>(GetOutputPin(page.m_hdr.bitstream_serial_number))) {
					pOggPin->AddComment(page.data() + 7, page.size() - 7);
				}
			} else if (type == 0x7F && page.size() > 12 && GETUINT32(p + 8) == FCC('fLaC')) {	// Flac
				if (PinNotExist) {
					// Ogg Flac : method 1
					CAutoPtr<CBaseSplitterOutputPin> pPinOut;
					name.Format(L"FLAC %d", streamId++);
					pPinOut.Attach(DNew COggFlacOutputPin(p + 12, page.size() - 14, name, this, this, &hr));
					AddOutputPin(page.m_hdr.bitstream_serial_number, pPinOut);
				}
			} else if (GETUINT32(p-1) == FCC('fLaC')) {
				// Ogg Flac : method 2
				if (PinNotExist && m_pFile->Read(page)) {
					CAutoPtr<CBaseSplitterOutputPin> pPinOut;
					name.Format(L"FLAC %d", streamId++);
					p = page.data();
					pPinOut.Attach(DNew COggFlacOutputPin(p, page.size(), name, this, this, &hr));
					AddOutputPin(page.m_hdr.bitstream_serial_number, pPinOut);
				}
			} else if (!memcmp(page.data(), "BBCD\x00", 5) || !memcmp(page.data(), "KW-DIRAC\x00", 9)) {
				if (PinNotExist) {
					name.Format(L"Dirac %d", streamId++);
					CAutoPtr<CBaseSplitterOutputPin> pPinOut;
					pPinOut.Attach(DNew COggDiracOutputPin(page.data(), page.size(), name, this, this, &hr));
					AddOutputPin(page.m_hdr.bitstream_serial_number, pPinOut);
					streamMoreInit[page.m_hdr.bitstream_serial_number] = TRUE;
				}
			} else if (!memcmp(page.data(), "OpusHead", 8) && page.size() > 8) {
				if (PinNotExist) {
					name.Format(L"Opus %d", streamId++);
					CAutoPtr<CBaseSplitterOutputPin> pPinOut;
					pPinOut.Attach(DNew COggOpusOutputPin(page.data(), page.size(), name, this, this, &hr));
					AddOutputPin(page.m_hdr.bitstream_serial_number, pPinOut);
				}
			} else if (!memcmp(page.data(), "OpusTags", 8) && page.size() > 8) {
				if (COggSplitterOutputPin* pOggPin = dynamic_cast<COggSplitterOutputPin*>(GetOutputPin(page.m_hdr.bitstream_serial_number))) {
					pOggPin->AddComment(page.data() + 8, page.size() - 8);
				}
			} else if (!memcmp(page.data(), "Speex   ", 8) && page.size() > 8) {
				if (PinNotExist) {
					name.Format(L"Speex %d", streamId++);
					CAutoPtr<CBaseSplitterOutputPin> pPinOut;
					pPinOut.Attach(DNew COggSpeexOutputPin(page.data(), page.size(), name, this, this, &hr));
					AddOutputPin(page.m_hdr.bitstream_serial_number, pPinOut);
				}
			} else if (!memcmp(page.data(), "\x80kate\x00\x00\x00", 8) && page.size() == 64) {
				if (PinNotExist) {
					CStringA lang;
					memcpy(lang.GetBuffer(16), page.data() + 32, 16);
					lang.ReleaseBuffer();

					name.Format(L"Kate %d (%hS)", streamId++, lang);
					CAutoPtr<CBaseSplitterOutputPin> pPinOut;
					pPinOut.Attach(DNew COggKateOutputPin((OggStreamHeader*)p, name, this, this, &hr));
					// TODO
					AddOutputPin(page.m_hdr.bitstream_serial_number, pPinOut);
				}
			} else if (type == 0x4F && !memcmp(p, "VP80", 4)) {
				switch (p[4]) {
					case 0x01:
						if (PinNotExist
								&& page.size() >= 26
								&& p[5] == 0x01) {
							name.Format(L"VP8 %d", streamId++);
							CAutoPtr<CBaseSplitterOutputPin> pPinOut;
							pPinOut.Attach(DNew COggVP8OutputPin(page.data(), page.size(), name, this, this, &hr));
							AddOutputPin(page.m_hdr.bitstream_serial_number, pPinOut);

							m_bitstream_serial_number_Video = page.m_hdr.bitstream_serial_number;
						}
						break;
					case 0x02:
						if (p[5] == 0x20) {
							if (COggVP8OutputPin* pOggPin = dynamic_cast<COggVP8OutputPin*>(GetOutputPin(page.m_hdr.bitstream_serial_number))) {
								pOggPin->AddComment(page.data() + 7, page.size() - 7);
							}
						}
						break;
				}
			} else if (type == 0x84) {
				if (COggFlacOutputPin* pOggPin = dynamic_cast<COggFlacOutputPin*>(GetOutputPin(page.m_hdr.bitstream_serial_number))) {
					pOggPin->AddComment(page.data() + 4, page.size() - 4);
				}
			} else if (!(type & 1) && streamMoreInit.empty()) {
				break;
			}
		}

		if (COggTheoraOutputPin* pPin = dynamic_cast<COggTheoraOutputPin*>(GetOutputPin(page.m_hdr.bitstream_serial_number))) {
			if (!pPin->IsInitialized()) {
				pPin->UnpackInitPage(page);
				if (pPin->IsInitialized()) {
					streamMoreInit.erase(page.m_hdr.bitstream_serial_number);
					m_bitstream_serial_number_Video = page.m_hdr.bitstream_serial_number;
				}
			}
		}

		if (COggVorbisOutputPin* pPin = dynamic_cast<COggVorbisOutputPin*>(GetOutputPin(page.m_hdr.bitstream_serial_number))) {
			if (!pPin->IsInitialized()) {
				pPin->UnpackInitPage(page);
				if (pPin->IsInitialized()) {
					streamMoreInit.erase(page.m_hdr.bitstream_serial_number);
				}
			}
		}

		if (COggDiracOutputPin* pPin = dynamic_cast<COggDiracOutputPin*>(GetOutputPin(page.m_hdr.bitstream_serial_number))) {
			if (!pPin->IsInitialized()) {
				pPin->UnpackInitPage(page);
				if (pPin->IsInitialized()) {
					streamMoreInit.erase(page.m_hdr.bitstream_serial_number);
				}
			}
		}
	}

	if (m_pOutputs.IsEmpty()) {
		return E_FAIL;
	}

	if (m_pOutputs.GetCount() > 1) {
		m_bitstream_serial_number_start = m_bitstream_serial_number_last = 0;
	}

	// verify that stream contain data, not only header
	if (m_bitstream_serial_number_start) {
		__int64 start_pos2 = start_pos;
		m_pFile->Seek(start_pos2);
		for (int i = 0; m_pFile->Read(page), i < 10; i++) {
			COggSplitterOutputPin* pOggPin = dynamic_cast<COggSplitterOutputPin*>(GetOutputPin(page.m_hdr.bitstream_serial_number));
			if (!pOggPin) {
				BYTE* p = page.data();
				if (!p || !memcmp(p, "fishead", 7) || !memcmp(p, "fisbone", 7)) {
					start_pos2 = m_pFile->GetPos();
					continue;
				}
				DeleteOutputs();
				start_pos = start_pos2;
				goto start;
			}
			start_pos2 = m_pFile->GetPos();
		}
	}

	// get max pts to calculate duration
	if (m_pFile->IsRandomAccess()) {
		m_pFile->Seek(std::max(m_pFile->GetLength() - MAX_PAGE_SIZE, 0LL));

		OggPage page2;
		while (m_pFile->Read(page2)) {
			COggSplitterOutputPin* pOggPin = dynamic_cast<COggSplitterOutputPin*>(GetOutputPin(page2.m_hdr.bitstream_serial_number));
			if (!pOggPin || page2.m_hdr.granule_position == -1) {
				continue;
			}
			REFERENCE_TIME rt = pOggPin->GetRefTime(page2.m_hdr.granule_position);
			m_rtDuration = std::max(rt, m_rtDuration);
		}

		// get min pts to calculate duration
		REFERENCE_TIME rtMin = 0;
		m_pFile->Seek(start_pos);
		for (int i = 0; m_pFile->Read(page2), i < 10; i++) {
			COggSplitterOutputPin* pOggPin = dynamic_cast<COggSplitterOutputPin*>(GetOutputPin(page2.m_hdr.bitstream_serial_number));
			if (!pOggPin || page2.m_hdr.granule_position == -1 || page2.m_hdr.header_type_flag & OggPageHeader::first) {
				continue;
			}
			REFERENCE_TIME rt = pOggPin->GetRefTime(page2.m_hdr.granule_position);
			if (rt > 0) {
				if ((rt - rtMin) > 10 * UNITS) {
					rtMin = rt;
				} else {
					break;
				}
			}
		}

		m_rtDuration -= rtMin;
		m_rtDuration = std::max(0LL, m_rtDuration);

		m_rtOffset = rtMin;
	}

	m_pFile->Seek(start_pos);

	m_rtNewStop = m_rtStop = m_rtDuration;

	// comments
	{
		std::map<CStringW, CStringW> tagmap = {
			{ L"TITLE",                  L"TITL"  },
			{ L"ARTIST",                 L"AUTH"  },
			{ L"COPYRIGHT",              L"CPYR"  },
			{ L"DESCRIPTION",            L"DESC"  },
			{ L"ENCODER",                L"DESC"  },
			{ L"ALBUM",                  L"ALBUM" },
			{ L"METADATA_BLOCK_PICTURE", L""      }
		};

		for (const auto& [oggtag, dsmtag] : tagmap) {
			POSITION pos = m_pOutputs.GetHeadPosition();
			while (pos) {
				COggSplitterOutputPin* pOggPin = dynamic_cast<COggSplitterOutputPin*>((CBaseOutputPin*)m_pOutputs.GetNext(pos));
				if (!pOggPin) {
					continue;
				}

				CStringW value = pOggPin->GetComment(oggtag);
				if (!value.IsEmpty()) {
					if (oggtag == L"METADATA_BLOCK_PICTURE") {
						// Flac format, METADATA_BLOCK_PICTURE
						if (!value.IsEmpty()) {
							const CStringA base64(value);
							int byteLength = Base64DecodeGetRequiredLength(base64.GetLength());
							std::vector<BYTE> pData(byteLength);
							if (Base64Decode(base64.GetString(), base64.GetLength(), pData.data(), &byteLength)) {
								CGolombBuffer gb(pData.data(), byteLength);
								for (;;) {
									gb.SkipBytes(4); // picture type

									const auto mime_len = gb.ReadDword(); // MIME type size in bytes.
									if (mime_len >= 64) {
										break;
									}
									CStringA mimeA;
									gb.ReadBuffer((BYTE*)mimeA.GetBufferSetLength(mime_len), mime_len); // MIME type

									const auto desc_len = gb.ReadDword(); // description size in bytes.
									if (desc_len) {
										gb.SkipBytes(desc_len); // description of the picture
									}

									gb.SkipBytes(4); // width of the picture
									gb.SkipBytes(4); // height of the picture
									gb.SkipBytes(4); // color depth of the picture
									gb.SkipBytes(4); // number of colors used

									const auto picture_len = gb.ReadDword(); // picture data size in bytes.
									if (picture_len) {
										std::vector<BYTE> picture_data(picture_len);
										gb.ReadBuffer(picture_data.data(), picture_len); // binary picture data.

										CStringW fname;
										CStringW mime(mimeA);
										if (mime == L"image/jpeg" || mime == L"image/jpg") {
											fname = L"cover.jpg";
										} else if (mime == L"image/png") {
											fname = L"cover.png";
										} else {
											fname = mime;
											fname.Replace(L"image/", L"cover.");
										}

										ResAppend(fname, L"cover", mime.GetString(), picture_data.data(), picture_len, 0);
									}

									break;
								}
							}
						}
					} else {
						SetProperty(dsmtag, value);
					}
					break;
				}
			}
		}

		POSITION pos = m_pOutputs.GetHeadPosition();
		while (pos && !ChapGetCount()) {
			COggSplitterOutputPin* pOggPin = dynamic_cast<COggSplitterOutputPin*>((CBaseOutputPin*)m_pOutputs.GetNext(pos));
			if (!pOggPin) {
				continue;
			}

			for (int i = 1; pOggPin; i++) {
				CStringW key;
				key.Format(L"CHAPTER%02d", i);
				CStringW time = pOggPin->GetComment(key);
				if (time.IsEmpty()) {
					break;
				}
				key.Format(L"CHAPTER%02dNAME", i);
				CStringW name = pOggPin->GetComment(key);
				if (name.IsEmpty()) {
					name.Format(L"Chapter %d", i);
				}
				int h, m, s, ms;
				WCHAR c;
//				if (7 != swscanf(time, L"%d%c%d%c%d%c%d", &h, &c, &m, &c, &s, &c, &ms)) {	// temp rem SZL - 4728
				if (7 != swscanf_s(time, L"%d%c%d%c%d%c%d", &h, &c, sizeof(WCHAR),			// temp ins SZL - 4728
								   &m, &c, sizeof(WCHAR), &s, &c, sizeof(WCHAR), &ms)) {	// temp ins SZL - 4728
					break;
				}
				REFERENCE_TIME rt = ((((REFERENCE_TIME)h*60+m)*60+s)*1000+ms)*10000;
				ChapAppend(rt, name);
			}
		}

		if (!ChapGetCount()) {
			pos = m_pOutputs.GetHeadPosition();
			while (pos) {
				COggSplitterOutputPin* pOggPin = dynamic_cast<COggSplitterOutputPin*>((CBaseOutputPin*)m_pOutputs.GetNext(pos));
				if (!pOggPin) {
					continue;
				}

				CStringW value = pOggPin->GetComment(L"CUESHEET");
				if (!value.IsEmpty()) {
					std::list<Chapters> ChaptersList;
					CString Title, Performer;
					if (ParseCUESheet(value, ChaptersList, Title, Performer)) {
						if (!Title.IsEmpty()) {
							CComBSTR value;
							GetProperty(L"TITL", &value);
							if (!value.Length()) {
								SetProperty(L"TITL", Title);
							}
						}
						if (!Performer.IsEmpty()) {
							CComBSTR value;
							GetProperty(L"AUTH", &value);
							if (!value.Length()) {
								SetProperty(L"AUTH", Performer);
							}
						}

						ChapRemoveAll();
						for (const auto& cp : ChaptersList) {
							ChapAppend(cp.rt, cp.name);
						}
					}
					break;
				}
			}
		}
	}

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

bool COggSplitterFilter::DemuxInit()
{
	SetThreadName((DWORD)-1, "COggSplitterFilter");
	if (!m_pFile) {
		return false;
	}

	return true;
}

#define CalcPos(rt) (__int64)(1.0 * rt / m_rtDuration * len)

void COggSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	if (rt <= 0) {
		m_pFile->Seek(0);
	} else if (m_rtDuration > 0) {
		rt += m_rtOffset;

		const __int64 len = m_pFile->GetLength();
		__int64 seekpos   = CalcPos(rt);

		const REFERENCE_TIME rtmax = rt - UNITS * (m_bitstream_serial_number_Video != DWORD_MAX ? 4 : 0);
		const REFERENCE_TIME rtmin = rtmax - UNITS / 2;

		__int64 curpos = seekpos;
		double div = 1.0;
		for (;;) {
			REFERENCE_TIME rtSeek = INVALID_TIME;

			m_pFile->Seek(curpos);
			OggPage page;
			while (m_pFile->Read(page, false)) {
				if (page.m_hdr.granule_position == -1) {
					continue;
				}
				if (!page.bComplete) {
					continue;
				}

				COggSplitterOutputPin* pOggPin = dynamic_cast<COggSplitterOutputPin*>(GetOutputPin(page.m_hdr.bitstream_serial_number));
				if (!pOggPin) {
					continue;
				}

				if (m_bitstream_serial_number_Video != DWORD_MAX
						&& m_bitstream_serial_number_Video != page.m_hdr.bitstream_serial_number) {
					continue;
				}

				rtSeek = pOggPin->GetRefTime(page.m_hdr.granule_position) + pOggPin->GetOffset();
				break;
			}

			if (rtSeek == INVALID_TIME) {
				break;
			}

			if (rtmin <= rtSeek && rtSeek <= rtmax) {
				m_pFile->Seek(page.pos);
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

		m_pFile->Seek(0);
	}
}

bool COggSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	OggPage page;
	while (SUCCEEDED(hr) && !CheckRequest(nullptr)) {

		if(!m_pFile->Read(page, true, GetRequestHandle())) {
			break;
		}

		if (m_pOutputs.GetCount() == 1 && m_bitstream_serial_number_start && m_bitstream_serial_number_start != page.m_hdr.bitstream_serial_number) {
			m_bitstream_serial_number_last		= page.m_hdr.bitstream_serial_number;
			page.m_hdr.bitstream_serial_number	= m_bitstream_serial_number_start;
		}

		COggSplitterOutputPin* pOggPin = dynamic_cast<COggSplitterOutputPin*>(GetOutputPin(page.m_hdr.bitstream_serial_number));
		if (!pOggPin) {
			continue;
		}
		if (!pOggPin->IsConnected()) {
			continue;
		}
		if (FAILED(hr = pOggPin->UnpackPage(page))) {
			ASSERT(0);
			break;
		}

		CAutoPtr<CPacket> p;
		while (!CheckRequest(nullptr) && SUCCEEDED(hr) && (p = pOggPin->GetPacket())) {
			hr = DeliverPacket(p);
		}
	}

	return true;
}

//
// COggSourceFilter
//

COggSourceFilter::COggSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: COggSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.Free();
}

//
// COggSplitterOutputPin
//

COggSplitterOutputPin::COggSplitterOutputPin(LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseSplitterOutputPin(pName, pFilter, pLock, phr)
	, m_pFilter(dynamic_cast<COggSplitterFilter*>(pFilter))
	, m_rtLast(INVALID_TIME)
	, m_bSetKeyFrame(false)
{
	ResetState();
}

void COggSplitterOutputPin::AddComment(BYTE* p, int len)
{
	CGolombBuffer gb(p, len);
	gb.SkipBytes(gb.ReadDwordLE());

	const DWORD cnt = gb.ReadDwordLE();
	for (DWORD n = cnt; gb.RemainingSize() >= 4 && n-- > 0; ) {
		const DWORD size = gb.ReadDwordLE();
		CStringA str;
		gb.ReadBuffer((BYTE*)str.GetBufferSetLength(size), size);

		const int sepPos = str.Find('=');
		if (sepPos <= 0 || sepPos == str.GetLength() - 1) {
			continue;
		}

		CStringA TagKey   = str.Left(sepPos);
		CStringA TagValue = str.Mid(sepPos + 1);

		const CStringW commentKey   = UTF8ToWStr(TagKey).MakeUpper();
		const CStringW commentValue = UTF8ToWStr(TagValue);

		if (commentKey == L"LANGUAGE") {
			CString lang = ISO6392ToLanguage(TagValue), iso6392 = LanguageToISO6392(commentValue);
			if (commentValue.GetLength() == 3 && !lang.IsEmpty()) {
				SetName(GetMediaTypeDesc(m_mts, lang, m_pFilter));
				SetProperty(L"LANG", commentValue);
			} else if (!iso6392.IsEmpty()) {
				SetName(GetMediaTypeDesc(m_mts, commentValue, m_pFilter));
				SetProperty(L"LANG", iso6392);
			} else {
				SetName(GetMediaTypeDesc(m_mts, commentValue, m_pFilter));
				SetProperty(L"NAME", commentValue);
			}
		}

		m_pComments[commentKey] = commentValue;
	}
}

CStringW COggSplitterOutputPin::GetComment(CStringW key)
{
	key.MakeUpper();
	std::list<CStringW> sl;
	for (const auto& [_key, _value] : m_pComments) {
		if (_key == key) {
			sl.push_back(_value);
		}
	}
	return Implode(sl, ';');
}

void COggSplitterOutputPin::ResetState(DWORD seqnum/* = DWORD_MAX*/)
{
	m_queue.RemoveAll();
	m_lastPacketData.clear();
	m_lastseqnum = seqnum;
}

void COggSplitterOutputPin::HandlePacket(DWORD TrackNumber, BYTE* pData, int len)
{
	if (m_rtLast != INVALID_TIME) {
		CAutoPtr<CPacket> p(DNew CPacket());
		p->TrackNumber = TrackNumber;
		if (S_OK == UnpackPacket(p, pData, len)) {
			m_rtLast = p->rtStop;

			m_queue.Add(p);
		}
	}
}

HRESULT COggSplitterOutputPin::UnpackPage(OggPage& page)
{
	if (m_lastseqnum != page.m_hdr.page_sequence_number - 1) {
		ResetState(page.m_hdr.page_sequence_number);
	} else {
		m_lastseqnum = page.m_hdr.page_sequence_number;
	}

	size_t last_segment = -1;
	for (size_t segment = page.m_lens.size() - 1; (int)segment >= 0; segment--) {
		if (page.m_lens[segment] < 255) {
			last_segment = segment;
			break;
		}
	}

	BYTE* start = page.data();

	unsigned size = 0;
	for (size_t segment = 0; segment < page.m_lens.size(); segment++) {
		const auto& len = page.m_lens[segment];
		size += len;

		if (len < 255) {
			if (last_segment == segment && page.m_hdr.granule_position != -1) {
				const REFERENCE_TIME rt = GetRefTime(page.m_hdr.granule_position) - m_pFilter->m_rtOffset;
				if (rt >= 0) {
					m_rtLast = rt;
				}
			}

			if (!m_lastPacketData.empty()) {
				const size_t old_size = m_lastPacketData.size();
				m_lastPacketData.resize(old_size + size);
				memcpy(m_lastPacketData.data() + old_size, start, size);

				HandlePacket(page.m_hdr.bitstream_serial_number, m_lastPacketData.data(), m_lastPacketData.size());
				m_lastPacketData.clear();
			} else {
				HandlePacket(page.m_hdr.bitstream_serial_number, start, size);
			}

			start += size;
			size = 0;
		}
	}

	if (size) {
		const size_t old_size = m_lastPacketData.size();
		m_lastPacketData.resize(old_size + size);
		memcpy(m_lastPacketData.data() + old_size, start, size);
	}

	return S_OK;
}

CAutoPtr<CPacket> COggSplitterOutputPin::GetPacket()
{
	CAutoPtr<CPacket> p;
	size_t count;
	m_queue.RemoveSafe(p, count);
	return p;
}

HRESULT COggSplitterOutputPin::DeliverEndFlush()
{
	ResetState();
	return __super::DeliverEndFlush();
}

HRESULT COggSplitterOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	ResetState();
	m_rtLast = INVALID_TIME;
	m_bSetKeyFrame = false;
	return __super::DeliverNewSegment(tStart, tStop, dRate);
}

//
// COggVorbisOutputPin
//

COggVorbisOutputPin::COggVorbisOutputPin(OggVorbisIdHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggSplitterOutputPin(pName, pFilter, pLock, phr)
{
	m_audio_sample_rate = h->audio_sample_rate;
	m_blocksize[0] = 1 << h->blocksize_0;
	m_blocksize[1] = 1 << h->blocksize_1;
	m_lastblocksize = 0;

	CMediaType mt;

	mt.InitMediaType();
	mt.majortype  = MEDIATYPE_Audio;
	mt.subtype    = MEDIASUBTYPE_Vorbis;
	mt.formattype = FORMAT_VorbisFormat;

	VORBISFORMAT* vf = (VORBISFORMAT*)mt.AllocFormatBuffer(sizeof(VORBISFORMAT));
	memset(mt.Format(), 0, mt.FormatLength());

	vf->nChannels      = h->audio_channels;
	vf->nSamplesPerSec = h->audio_sample_rate;
	vf->nAvgBitsPerSec = h->bitrate_nominal;
	vf->nMinBitsPerSec = h->bitrate_minimum;
	vf->nMaxBitsPerSec = h->bitrate_maximum;
	vf->fQuality       = -1;
	mt.SetSampleSize(8192);
	m_mts.push_back(mt);

	mt.InitMediaType();
	mt.majortype  = MEDIATYPE_Audio;
	mt.subtype    = MEDIASUBTYPE_Vorbis2;
	mt.formattype = FORMAT_VorbisFormat2;

	VORBISFORMAT2* vf2 = (VORBISFORMAT2*)mt.AllocFormatBuffer(sizeof(VORBISFORMAT2));
	memset(mt.Format(), 0, mt.FormatLength());

	vf2->Channels      = h->audio_channels;
	vf2->SamplesPerSec = h->audio_sample_rate;
	mt.SetSampleSize(8192);
	m_mts.insert(m_mts.cbegin(), mt);

	SetName(GetMediaTypeDesc(m_mts, pName, pFilter));
}

HRESULT COggVorbisOutputPin::UnpackInitPage(OggPage& page)
{
	HRESULT hr = __super::UnpackPage(page);

	while (m_queue.GetCount()) {
		CAutoPtr<CPacket> p = m_queue.Remove();

		if (p->size() >= 6 && p->at(0) == 0x05) {
			// yeah, right, we are going to be parsing this backwards! :P
			bitstream bs(p->data(), p->size(), true);
			while (bs.hasbits(-1) && bs.getbits(-1) != 1) {
				;
			}
			for (int cnt = 0; bs.hasbits(-8-16-16-1-6); cnt++) {
				unsigned int modes = bs.showbits(-6) + 1;

				unsigned int mapping       = bs.getbits(-8);
				unsigned int transformtype = bs.getbits(-16);
				unsigned int windowtype    = bs.getbits(-16);
				unsigned int blockflag     = bs.getbits(-1);
				UNREFERENCED_PARAMETER(mapping);

				if (transformtype != 0 || windowtype != 0) {
					ASSERT(modes == cnt);
					UNREFERENCED_PARAMETER(modes);
					break;
				}

				m_blockflags.insert(m_blockflags.begin(), !!blockflag);
			}
		}

		int cnt = m_initpackets.size();
		if (cnt < 3 && (p->size() >= 6 && p->at(0) == 1 + cnt * 2)) {
			VORBISFORMAT2* vf2   = (VORBISFORMAT2*)m_mts[0].Format();
			vf2->HeaderSize[cnt] = p->size();
			int len              = m_mts[0].FormatLength();
			memcpy(m_mts[0].ReallocFormatBuffer(len + p->size()) + len, p->data(), p->size());
		}

		m_initpackets.emplace_back(p);
	}

	return hr;
}

REFERENCE_TIME COggVorbisOutputPin::GetRefTime(__int64 granule_position)
{
	REFERENCE_TIME rt = (granule_position * UNITS) / m_audio_sample_rate;
	return rt;
}

HRESULT COggVorbisOutputPin::UnpackPacket(CAutoPtr<CPacket>& p, BYTE* pData, int len)
{
	if (len >= 7 && !memcmp(pData + 1, "vorbis", 6)) {
		if (IsInitialized()) {
			return E_FAIL; // skip Vorbis header packets ...
		}
	}

	REFERENCE_TIME duration = 1;
	if (len > 0 && m_blockflags.size()) {
		bitstream bs(pData, len);
		if (bs.getbits(1) == 0) {
			int x = m_blockflags.size() - 1, n = 0;
			while (x) {
				n++;
				x >>= 1;
			}
			DWORD blocksize = m_blocksize[m_blockflags[bs.getbits(n)] ? 1 : 0];
			if (m_lastblocksize) {
				duration = GetRefTime((m_lastblocksize + blocksize) >> 2);
			}
			m_lastblocksize = blocksize;
		}
	}

	p->bSyncPoint = TRUE;
	p->rtStart    = m_rtLast;
	p->rtStop     = m_rtLast + duration;
	p->SetData(pData, len);

	return S_OK;
}

HRESULT COggVorbisOutputPin::DeliverPacket(CAutoPtr<CPacket> p)
{
	if (p->size() > 0 && (p->at(0) & 1)) {
		return S_OK;
	}

	return __super::DeliverPacket(p);
}

HRESULT COggVorbisOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	HRESULT hr = __super::DeliverNewSegment(tStart, tStop, dRate);

	m_lastblocksize = 0;

	if (m_mt.subtype == MEDIASUBTYPE_Vorbis) {
		for (const auto& packet : m_initpackets) {
			CAutoPtr<CPacket> p(DNew CPacket());
			p->TrackNumber = packet->TrackNumber;
			p->SetData(*packet);
			__super::DeliverPacket(p);
		}
	}

	return hr;
}

//
// COggFlacOutputPin
//

COggFlacOutputPin::COggFlacOutputPin(BYTE* h, int nCount, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggSplitterOutputPin(pName, pFilter, pLock, phr)
{
	CGolombBuffer Buffer(h, nCount);

	Buffer.BitRead(1);		// Last-metadata-block flag

	if (Buffer.BitRead(7) != 0) {	// Should be a STREAMINFO block
		if (phr) {
			*phr = VFW_E_INVALID_FILE_FORMAT;
		}
		return;
	}

	Buffer.BitRead(24); // Length (in bytes) of metadata to follow
	Buffer.ReadShort(); // m_nMinBlocksize
	Buffer.ReadShort(); // m_nMaxBlocksize
	Buffer.BitRead(24); // m_nMinFrameSize
	Buffer.BitRead(24); // m_nMaxFrameSize
	m_nSamplesPerSec    = (int)Buffer.BitRead(20);
	m_nChannels         = (int)Buffer.BitRead(3)  + 1;
	m_wBitsPerSample    = (WORD)Buffer.BitRead(5) + 1;
	Buffer.BitRead(36); // m_i64TotalNumSamples
	m_nAvgBytesPerSec   = (m_nChannels * (m_wBitsPerSample >> 3)) * m_nSamplesPerSec;

	CMediaType mt;

	mt.majortype  = MEDIATYPE_Audio;
	mt.subtype    = MEDIASUBTYPE_FLAC_FRAMED;
	mt.formattype = FORMAT_WaveFormatEx;

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX));
	memset(wfe, 0, sizeof(WAVEFORMATEX));
	wfe->wFormatTag      = WAVE_FORMAT_FLAC;
	wfe->nSamplesPerSec  = m_nSamplesPerSec;
	wfe->nAvgBytesPerSec = m_nAvgBytesPerSec;
	wfe->nChannels       = m_nChannels;
	wfe->nBlockAlign     = 1;
	wfe->wBitsPerSample  = m_wBitsPerSample;

	m_mts.insert(m_mts.cbegin(), mt);

	SetName(GetMediaTypeDesc(m_mts, pName, pFilter));
	*phr = S_OK;
}

REFERENCE_TIME COggFlacOutputPin::GetRefTime(__int64 granule_position)
{
	REFERENCE_TIME rt = (granule_position * UNITS) / m_nSamplesPerSec;
	return rt;
}

HRESULT COggFlacOutputPin::UnpackPacket(CAutoPtr<CPacket>& p, BYTE* pData, int len)
{
	if (pData[0] != 0xFF || (pData[1] & 0xFE) != 0xF8) {
		return S_FALSE;
	}

	p->bSyncPoint = TRUE;
	p->rtStart    = m_rtLast;
	p->rtStop     = m_rtLast + 1; // TODO : find packet duration !
	p->SetData(pData, len);

	return S_OK;
}

//
// COggDirectShowOutputPin
//

COggDirectShowOutputPin::COggDirectShowOutputPin(AM_MEDIA_TYPE* pmt, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggSplitterOutputPin(pName, pFilter, pLock, phr)
{
	CMediaType mt;
	memcpy((AM_MEDIA_TYPE*)&mt, pmt, FIELD_OFFSET(AM_MEDIA_TYPE, pUnk));
	mt.SetFormat((BYTE*)(pmt + 1), pmt->cbFormat);
	mt.SetSampleSize(1);
	if (mt.majortype == MEDIATYPE_Video) { // TODO: find samples for audio and find out what to return in GetRefTime...
		m_mts.push_back(mt);
	}

	SetName(GetMediaTypeDesc(m_mts, pName, pFilter));
}

REFERENCE_TIME COggDirectShowOutputPin::GetRefTime(__int64 granule_position)
{
	REFERENCE_TIME rt = 0;

	if (m_mt.majortype == MEDIATYPE_Video) {
		rt = granule_position * ((VIDEOINFOHEADER*)m_mt.Format())->AvgTimePerFrame;
	} else if (m_mt.majortype == MEDIATYPE_Audio) {
		rt = granule_position; // ((WAVEFORMATEX*)m_mt.Format())-> // TODO
	}

	return rt;
}

HRESULT COggDirectShowOutputPin::UnpackPacket(CAutoPtr<CPacket>& p, BYTE* pData, int len)
{
	int i = 0;

	BYTE hdr = pData[i++];

	if (!(hdr & 1)) {
		// TODO: verify if this was still present in the old format (haven't found one sample yet)
		BYTE nLenBytes = (hdr >> 6) | ((hdr & 2) << 1);
		__int64 Length = 0;
		for (int j = 0; j < nLenBytes; j++) {
			Length |= (__int64)pData[i++] << (j << 3);
		}

		if (len < i) {
			ASSERT(0);
			return E_FAIL;
		}

		bool bKeyFrame = !!(hdr & 8);
		if (!m_bSetKeyFrame) {
			m_bSetKeyFrame = bKeyFrame;
		}

		if (m_mt.majortype == MEDIATYPE_Video) {
			if (!m_bSetKeyFrame) {
				DLog(L"COggDirectShowOutputPin::UnpackPacket() : KeyFrame not found !!!");
				return E_FAIL; // waiting for a key frame after seeking
			}
		}

		p->bSyncPoint = bKeyFrame;
		p->rtStart    = m_rtLast;
		p->rtStop     = m_rtLast + (nLenBytes ? GetRefTime(Length) : GetRefTime(1));
		p->SetData(&pData[i], len - i);

		return S_OK;
	}

	return S_FALSE;
}

//
// COggStreamOutputPin
//

COggStreamOutputPin::COggStreamOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggSplitterOutputPin(pName, pFilter, pLock, phr)
	, m_time_unit(h->time_unit)
	, m_samples_per_unit(h->samples_per_unit)
	, m_default_len(h->default_len)
{
}

REFERENCE_TIME COggStreamOutputPin::GetRefTime(__int64 granule_position)
{
	return granule_position * m_time_unit / m_samples_per_unit;
}

HRESULT COggStreamOutputPin::UnpackPacket(CAutoPtr<CPacket>& p, BYTE* pData, int len)
{
	int i = 0;

	BYTE hdr = pData[i++];

	if (!(hdr&1)) {
		BYTE nLenBytes = (hdr >> 6) | ((hdr & 2) << 1);
		__int64 Length = 0;
		for (int j = 0; j < nLenBytes; j++) {
			Length |= (__int64)pData[i++] << (j << 3);
		}

		if (len < i) {
			ASSERT(0);
			return E_FAIL;
		}

		bool bKeyFrame = !!(hdr & 8);
		if (!m_bSetKeyFrame) {
			m_bSetKeyFrame = bKeyFrame;
		}

		if (m_mt.majortype == MEDIATYPE_Video) {
			if (!m_bSetKeyFrame) {
				DLog(L"COggStreamOutputPin::UnpackPacket() : KeyFrame not found !!!");
				return E_FAIL; // waiting for a key frame after seeking
			}
		}

		p->bSyncPoint = bKeyFrame;
		p->rtStart    = m_rtLast;
		p->rtStop     = m_rtLast + (nLenBytes ? GetRefTime(Length) : GetRefTime(m_default_len));
		p->SetData(&pData[i], len - i);

		return S_OK;
	}

	return S_FALSE;
}

//
// COggVideoOutputPin
//

COggVideoOutputPin::COggVideoOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggStreamOutputPin(h, pName, pFilter, pLock, phr)
{
	int extra = (int)h->size - sizeof(OggStreamHeader);
	extra = std::max(extra, 0);

	CMediaType mt;
	mt.majortype  = MEDIATYPE_Video;
	mt.formattype = FORMAT_VideoInfo;
	mt.subtype    = FOURCCMap(MAKEFOURCC(h->subtype[0], h->subtype[1], h->subtype[2], h->subtype[3]));

	VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + extra);
	memset(mt.Format(), 0, mt.FormatLength());
	memcpy(mt.Format() + sizeof(VIDEOINFOHEADER), h + 1, extra);

	pvih->AvgTimePerFrame         = h->time_unit / h->samples_per_unit;
	pvih->bmiHeader.biWidth       = h->v.w;
	pvih->bmiHeader.biHeight      = h->v.h;
	pvih->bmiHeader.biBitCount    = (WORD)h->bps;
	pvih->bmiHeader.biSizeImage   = DIBSIZE(pvih->bmiHeader);
	pvih->bmiHeader.biCompression = mt.subtype.Data1;
	switch (pvih->bmiHeader.biCompression) {
		case BI_RGB:
		case BI_BITFIELDS:
			mt.subtype =
				pvih->bmiHeader.biBitCount == 1 ? MEDIASUBTYPE_RGB1 :
				pvih->bmiHeader.biBitCount == 4 ? MEDIASUBTYPE_RGB4 :
				pvih->bmiHeader.biBitCount == 8 ? MEDIASUBTYPE_RGB8 :
				pvih->bmiHeader.biBitCount == 16 ? MEDIASUBTYPE_RGB565 :
				pvih->bmiHeader.biBitCount == 24 ? MEDIASUBTYPE_RGB24 :
				pvih->bmiHeader.biBitCount == 32 ? MEDIASUBTYPE_RGB32 :
				MEDIASUBTYPE_NULL;
			break;
		case BI_RLE8:
			mt.subtype = MEDIASUBTYPE_RGB8;
			break;
		case BI_RLE4:
			mt.subtype = MEDIASUBTYPE_RGB4;
			break;
		case FCC('MPEG'): // MediaInfo: Chromatic MPEG 1 Video I Frame
			pvih->bmiHeader.biCompression = 0;
			mt.subtype = MEDIASUBTYPE_MPEG1Payload;
			break;
	}
	mt.SetSampleSize(std::max(h->buffersize, 1uL));
	m_mts.push_back(mt);

	SetName(GetMediaTypeDesc(m_mts, pName, pFilter));
}

//
// COggAudioOutputPin
//

COggAudioOutputPin::COggAudioOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggStreamOutputPin(h, pName, pFilter, pLock, phr)
{
	int extra = (int)h->size - sizeof(OggStreamHeader);
	extra = std::max(extra, 0);

	CMediaType mt;
	mt.majortype   = MEDIATYPE_Audio;
	mt.subtype    = FOURCCMap(strtol(CStringA(h->subtype, 4), nullptr, 16));
	mt.formattype = FORMAT_WaveFormatEx;

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX) + extra);
	memset(mt.Format(), 0, mt.FormatLength());
	memcpy(mt.Format() + sizeof(WAVEFORMATEX), h + 1, extra);

	wfe->cbSize          = extra;
	wfe->wFormatTag      = (WORD)mt.subtype.Data1;
	wfe->nChannels       = h->a.nChannels;
	wfe->nSamplesPerSec  = (DWORD)(10000000i64 * h->samples_per_unit / h->time_unit);
	wfe->wBitsPerSample  = (WORD)h->bps;
	wfe->nAvgBytesPerSec = h->a.nAvgBytesPerSec; // TODO: verify for PCM
	wfe->nBlockAlign     = h->a.nBlockAlign; // TODO: verify for PCM
	mt.SetSampleSize(std::max(h->buffersize, 1uL));
	m_mts.push_back(mt);

	SetName(GetMediaTypeDesc(m_mts, pName, pFilter));
}

//
// COggTextOutputPin
//

COggTextOutputPin::COggTextOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggStreamOutputPin(h, pName, pFilter, pLock, phr)
{
	CMediaType mt;
	mt.majortype  = MEDIATYPE_Text;
	mt.subtype    = MEDIASUBTYPE_NULL;
	mt.formattype = FORMAT_None;
	mt.SetSampleSize(1);
	m_mts.push_back(mt);
}

//
// COggKateOutputPin
//

COggKateOutputPin::COggKateOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggStreamOutputPin(h, pName, pFilter, pLock, phr)
{
	CMediaType mt;
	mt.majortype  = MEDIATYPE_Subtitle;
	mt.subtype    = FOURCCMap(FCC('KATE')); // TODO: use the correct subtype for KATE subtitles
	mt.formattype = FORMAT_SubtitleInfo;

	SUBTITLEINFO* psi = (SUBTITLEINFO*)mt.AllocFormatBuffer(sizeof(SUBTITLEINFO));
	memset(psi, 0, mt.FormatLength());

	m_mts.push_back(mt);
}

//
// COggTheoraOutputPin
//

COggTheoraOutputPin::COggTheoraOutputPin(OggPage& page, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggSplitterOutputPin(pName, pFilter, pLock, phr)
	, m_KfgShift(0)
	, m_KfgMask(0)
	, m_nVersion(0)
	, m_rtAvgTimePerFrame(0)
{
	CGolombBuffer gb(page.data(), page.size());
	gb.SkipBytes(7);

	m_nVersion  = (UINT)gb.BitRead(24);
	LONG width  = gb.ReadShort() << 4;
	LONG height = gb.ReadShort() << 4;
	if (m_nVersion > 0x030400) {
		gb.BitRead(50); gb.BitRead(50);
	}

	if (m_nVersion >= 0x030200) {
		LONG visible_width  = gb.BitRead(24);
		LONG visible_height = gb.BitRead(24);
		if (visible_width <= width && visible_width > width - 16
				&& visible_height <= height && visible_height > height - 16) {
			width  = visible_width;
			height = visible_height;
		}
		gb.BitRead(16);
	}

	int nFpsNum = gb.ReadDword();
	int nFpsDen = gb.ReadDword();
	if (!nFpsNum || !nFpsDen) {
		DLog(L"COggTheoraOutputPin::COggTheoraOutputPin() : Invalid time base, use 25 fps");
		nFpsNum = 1;
		nFpsDen = 25;
	}
	m_rtAvgTimePerFrame = (REFERENCE_TIME)(10000000.0 * nFpsDen / nFpsNum);

	int nARnum = gb.BitRead(24);
	int nARden = gb.BitRead(24);
	CSize Aspect(width, height);
	if (nARnum && nARden) {
		Aspect.cx *= nARnum;
		Aspect.cy *= nARden;
	}
	ReduceDim(Aspect);

	if (m_nVersion >= 0x030200) {
		gb.BitRead(38);
	}
	if (m_nVersion >= 0x304000) {
		gb.BitRead(2);
	}

	m_KfgShift = gb.BitRead(5);
	if (!m_KfgShift) {
		m_KfgShift = 6; // Is it really default value ?
	}
	m_KfgMask = (1 << m_KfgShift) - 1;

	CMediaType mt;
	mt.majortype  = MEDIATYPE_Video;
	mt.formattype = FORMAT_MPEG2_VIDEO;
	mt.subtype    = FOURCCMap(FCC('THEO'));

	MPEG2VIDEOINFO* vih	= (MPEG2VIDEOINFO*)mt.AllocFormatBuffer(sizeof(MPEG2VIDEOINFO));
	memset(mt.Format(), 0, mt.FormatLength());

	vih->hdr.bmiHeader.biSize        = sizeof(vih->hdr.bmiHeader);
	vih->hdr.bmiHeader.biWidth       = width;
	vih->hdr.bmiHeader.biHeight      = height;
	vih->hdr.bmiHeader.biCompression = mt.subtype.Data1;
	vih->hdr.bmiHeader.biPlanes      = 1;
	vih->hdr.bmiHeader.biBitCount    = 24;
	vih->hdr.bmiHeader.biSizeImage   = DIBSIZE(vih->hdr.bmiHeader);
	vih->hdr.AvgTimePerFrame         = m_rtAvgTimePerFrame;
	vih->hdr.dwPictAspectRatioX      = Aspect.cx;
	vih->hdr.dwPictAspectRatioY      = Aspect.cy;

	mt.bFixedSizeSamples = 0;
	m_mts.push_back(mt);

	SetName(GetMediaTypeDesc(m_mts, pName, pFilter));
}

HRESULT COggTheoraOutputPin::UnpackInitPage(OggPage& page)
{
	HRESULT hr = __super::UnpackPage(page);

	while (m_queue.GetCount()) {
		CAutoPtr<CPacket> p = m_queue.Remove();

		if (p->size() == 0) {
			continue;
		}

		BYTE* data = p->data();
		BYTE type = *data++;

		if (type >= 0x80 && type <= 0x82 && !memcmp(data, "theora", 6)) {

			CMediaType& mt = m_mts[0];
			int size = p->size();
			//ASSERT(size <= 0xffff);
			MPEG2VIDEOINFO* vih = (MPEG2VIDEOINFO*)mt.ReallocFormatBuffer(
								   FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader) +
								   ((MPEG2VIDEOINFO*)mt.Format())->cbSequenceHeader +
								   2 + size);
			*(WORD*)((BYTE*)vih->dwSequenceHeader + vih->cbSequenceHeader) = (size >> 8) | (size << 8);
			memcpy((BYTE*)vih->dwSequenceHeader + vih->cbSequenceHeader + 2, p->data(), size);
			vih->cbSequenceHeader += 2 + size;

			m_initpackets.emplace_back(p);
		}
	}

	return hr;
}

REFERENCE_TIME COggTheoraOutputPin::GetRefTime(__int64 granule_position)
{
	LONGLONG iframe = (granule_position >> m_KfgShift);
	if (m_nVersion < 0x030201) {
		iframe++;
	}

	LONGLONG pframe = granule_position & m_KfgMask;
	/*3.2.0 streams store the frame index in the granule position.
	  3.2.1 and later store the frame count.
	  We return the index, so adjust the value if we have a 3.2.1 or later
	   stream.*/

	REFERENCE_TIME rt = (iframe + pframe) * m_rtAvgTimePerFrame;
	return rt;
}

HRESULT COggTheoraOutputPin::UnpackPacket(CAutoPtr<CPacket>& p, BYTE* pData, int len)
{
	if (!pData) {
		return E_FAIL;
	}

	if (len >= 7 && !memcmp(pData + 1, "theora", 6)) {
		if (IsInitialized()) {
			return E_FAIL; // skip Theora header packets ...
		}
	}

	bool bKeyFrame = (!(*pData & 0x40) && !(*pData & 0x80));
	if (!m_bSetKeyFrame) {
		m_bSetKeyFrame = bKeyFrame;
	}
	if (!m_bSetKeyFrame && IsInitialized()) {
		DLog(L"COggTheoraOutputPin::UnpackPacket() : KeyFrame not found !!!");
		return E_FAIL; // waiting for a key frame after seeking
	}

	p->bSyncPoint = bKeyFrame;
	p->rtStart    = m_rtLast;
	p->rtStop     = m_rtLast + m_rtAvgTimePerFrame;
	p->SetData(pData, len);

	if (!(*pData & 0x80) && m_mt.majortype == MEDIATYPE_Video) {
		p->rtStop = p->rtStart + ((MPEG2VIDEOINFO*)m_mt.Format())->hdr.AvgTimePerFrame;
	}

	return S_OK;
}

//
// COggDiracOutputPin
//

COggDiracOutputPin::COggDiracOutputPin(BYTE* p, int nCount, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggSplitterOutputPin(pName, pFilter, pLock, phr)
	, m_rtAvgTimePerFrame(0)
	, m_IsInitialized(false)
	, m_bOldDirac(!memcmp(p, "KW-DIRAC\x00", 9))
{
}

HRESULT COggDiracOutputPin::UnpackInitPage(OggPage& page)
{
	HRESULT hr = __super::UnpackPage(page);

	while (m_queue.GetCount() && !m_IsInitialized) {
		CAutoPtr<CPacket> p = m_queue.Remove();
		if (p->size() > 13) {
			BYTE* buf = p->data();

			if (!memcmp(buf, "BBCD\x00", 5)) {
				m_IsInitialized = SUCCEEDED(InitDirac(buf, p->size()));
			}
		}
	}
	m_queue.RemoveAll();

	return hr;
}

HRESULT COggDiracOutputPin::InitDirac(BYTE* p, int nCount)
{
	if (nCount <= 13) {
		return S_FALSE;
	}

	vc_params_t params;
	if(!ParseDiracHeader(p + 13, nCount - 13, params)) {
		return S_FALSE;
	}

	m_rtAvgTimePerFrame = std::max(1LL, params.AvgTimePerFrame);

	CMediaType mt;

	mt.majortype  = MEDIATYPE_Video;
	mt.formattype = FORMAT_VideoInfo;
	mt.subtype    = FOURCCMap('card');

	VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
	memset(mt.Format(), 0, mt.FormatLength());
	pvih->AvgTimePerFrame         = m_rtAvgTimePerFrame;
	pvih->bmiHeader.biSize        = sizeof(pvih->bmiHeader);
	pvih->bmiHeader.biWidth       = params.width;
	pvih->bmiHeader.biHeight      = params.height;
	pvih->bmiHeader.biPlanes      = 1;
	pvih->bmiHeader.biBitCount    = 12;
	pvih->bmiHeader.biCompression = mt.subtype.Data1;
	pvih->bmiHeader.biSizeImage   = DIBSIZE(pvih->bmiHeader);

	mt.bFixedSizeSamples = 0;
	m_mts.push_back(mt);

	SetName(GetMediaTypeDesc(m_mts, m_pName, m_pFilter));

	m_IsInitialized = true;

	return S_OK;
}

REFERENCE_TIME COggDiracOutputPin::GetRefTime(__int64 granule_position)
{
	REFERENCE_TIME pts_out = 0;

	if (m_bOldDirac) {
		const REFERENCE_TIME iframe = granule_position >> 30;
		const REFERENCE_TIME pframe = granule_position & 0x3fffffff;
		pts_out                     = (iframe + pframe) * m_rtAvgTimePerFrame;
	} else {
		const REFERENCE_TIME dts    = granule_position >> 31;
		const REFERENCE_TIME pts    = dts + ((granule_position >> 9) & 0x1fff);
		pts_out                     = pts * m_rtAvgTimePerFrame / 2;
	}

	return pts_out;
}

HRESULT COggDiracOutputPin::UnpackPacket(CAutoPtr<CPacket>& p, BYTE* pData, int len)
{
	if (!pData) {
		return E_FAIL;
	}

	if (m_IsInitialized && GETUINT32(pData) != FCC('BBCD')) { // not found Dirac SYNC 'BBCD'
		return E_FAIL;
	}

	p->bSyncPoint = TRUE;
	p->rtStart    = m_rtLast;
	p->rtStop     = m_rtLast + m_rtAvgTimePerFrame;
	p->SetData(pData, len);

	return S_OK;
}

//
// COggOpusOutputPin
//

COggOpusOutputPin::COggOpusOutputPin(BYTE* h, int nCount, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggSplitterOutputPin(pName, pFilter, pLock, phr)
{
	// http://wiki.xiph.org/OggOpus
	CGolombBuffer Buffer(h + 8, nCount - 8); // skip "OpusHead"

	BYTE version  = Buffer.ReadByte();
	BYTE channels = Buffer.ReadByte();
	m_Preskip     = Buffer.ReadShortLE();
	Buffer.SkipBytes(4); // Input sample rate
	Buffer.SkipBytes(2); // Output gain

	m_SampleRate	= 48000;

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)DNew BYTE[sizeof(WAVEFORMATEX) + nCount];
	memset(wfe, 0, sizeof(WAVEFORMATEX));
	wfe->wFormatTag      = WAVE_FORMAT_OPUS;
	wfe->nChannels       = channels;
	wfe->nSamplesPerSec  = m_SampleRate;
	wfe->wBitsPerSample  = 16;
	wfe->nBlockAlign     = 1;
	wfe->nAvgBytesPerSec = 0;
	wfe->cbSize          = nCount;
	memcpy((BYTE*)(wfe+1), h, nCount);

	CMediaType mt;
	ZeroMemory(&mt, sizeof(CMediaType));

	mt.majortype  = MEDIATYPE_Audio;
	mt.subtype    = MEDIASUBTYPE_OPUS;
	mt.formattype = FORMAT_WaveFormatEx;
	mt.SetFormat((BYTE*)wfe, sizeof(WAVEFORMATEX)+wfe->cbSize);

	delete [] wfe;

	m_mts.insert(m_mts.cbegin(), mt);

	SetName(GetMediaTypeDesc(m_mts, pName, pFilter));

	*phr = S_OK;
}

REFERENCE_TIME COggOpusOutputPin::GetRefTime(__int64 granule_position)
{
	REFERENCE_TIME rt = granule_position * UNITS / m_SampleRate;
	return rt;
}

HRESULT COggOpusOutputPin::UnpackPacket(CAutoPtr<CPacket>& p, BYTE* pData, int len)
{
	if (len > 8 && !memcmp(pData, "Opus", 4)) {
		return E_FAIL; // skip Opus header packets ...
	}

	// code from ffmpeg
	// calculate packet duration
	unsigned int toc, toc_config, toc_count, frame_size, nb_frames = 1;

	toc = *pData;
	toc_config = toc >> 3;
	toc_count  = toc & 3;
	frame_size = toc_config < 12 ? std::max(480u, 960u * (toc_config & 3)) :
				 toc_config < 16 ? 480 << (toc_config & 1) :
								   120 << (toc_config & 3);
	if (toc_count == 3) {
		if (len < 2) {
			return E_FAIL;
		}
		nb_frames = pData[1] & 0x3F;
	} else if (toc_count) {
		nb_frames = 2;
	}
	__int64 pduration = (frame_size * nb_frames * UNITS) / m_SampleRate;

	const REFERENCE_TIME preSkip = m_Preskip * UNITS / m_SampleRate;
	const REFERENCE_TIME pts = m_rtLast - preSkip;

	p->bSyncPoint = TRUE;
	p->rtStart    = pts;
	p->rtStop     = pts + pduration;
	p->SetData(pData, len);

	return S_OK;
}

//
// COggSpeexOutputPin
//
COggSpeexOutputPin::COggSpeexOutputPin(BYTE* h, int nCount, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggSplitterOutputPin(pName, pFilter, pLock, phr)
{
	// http://wiki.xiph.org/OggSpeex
	// http://www.speex.org/docs/manual/speex-manual/node8.html

	CGolombBuffer Buffer(h + (8 + 20), nCount - (8 + 20)); // 8 + 20 = speex_string + speex_version

	int speex_version_id       = Buffer.ReadDwordLE();
	int header_size            = Buffer.ReadDwordLE();
	int rate                   = Buffer.ReadDwordLE();
	int mode                   = Buffer.ReadDwordLE();
	int mode_bitstream_version = Buffer.ReadDwordLE();
	int nb_channels            = Buffer.ReadDwordLE();
	int bitrate                = Buffer.ReadDwordLE();
	int frame_size             = Buffer.ReadDwordLE();
	int vbr                    = Buffer.ReadDwordLE();
	int frames_per_packet      = Buffer.ReadDwordLE();
	int extra_headers          = Buffer.ReadDwordLE();
	int reserved1              = Buffer.ReadDwordLE();
	int reserved2              = Buffer.ReadDwordLE();

	m_SampleRate = rate;

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)DNew BYTE[sizeof(WAVEFORMATEX) + nCount];
	memset(wfe, 0, sizeof(WAVEFORMATEX));
	wfe->wFormatTag      = WAVE_FORMAT_SPEEX;
	wfe->nChannels       = nb_channels;
	wfe->nSamplesPerSec  = rate;
	wfe->wBitsPerSample  = 16;
	wfe->nBlockAlign     = frame_size;
	wfe->nAvgBytesPerSec = 0;
	wfe->cbSize          = nCount;
	memcpy((BYTE*)(wfe+1), h, nCount);

	CMediaType mt;
	ZeroMemory(&mt, sizeof(CMediaType));

	mt.majortype  = MEDIATYPE_Audio;
	mt.subtype    = MEDIASUBTYPE_SPEEX;
	mt.formattype = FORMAT_WaveFormatEx;
	mt.SetFormat((BYTE*)wfe, sizeof(WAVEFORMATEX) + wfe->cbSize);

	delete [] wfe;

	m_mts.insert(m_mts.cbegin(), mt);

	SetName(GetMediaTypeDesc(m_mts, pName, pFilter));

	*phr = S_OK;
}

REFERENCE_TIME COggSpeexOutputPin::GetRefTime(__int64 granule_position)
{
	REFERENCE_TIME rt = (granule_position * UNITS) / m_SampleRate;
	return rt;
}

HRESULT COggSpeexOutputPin::UnpackPacket(CAutoPtr<CPacket>& p, BYTE* pData, int len)
{
	p->bSyncPoint = TRUE;
	p->rtStart    = m_rtLast;
	p->rtStop     = m_rtLast + 1; // TODO : find packet duration !
	p->SetData(pData, len);

	return S_OK;
}

//
// COggVP8OutputPin
//

COggVP8OutputPin::COggVP8OutputPin(BYTE* h, int nCount, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggSplitterOutputPin(pName, pFilter, pLock, phr)
	, m_rtAvgTimePerFrame(0)
{
	CGolombBuffer gb(h + 8, nCount - 8); // skip VP8 header

	int width  = gb.BitRead(16);
	int height = gb.BitRead(16);
	int nARnum = gb.BitRead(24);
	int nARden = gb.BitRead(24);
	CSize Aspect(width, height);
	if (nARnum && nARden) {
		Aspect.cx *= nARnum;
		Aspect.cy *= nARden;
	}
	ReduceDim(Aspect);
	int nFpsNum = gb.BitRead(32);
	int nFpsDen = gb.BitRead(32);
	if (!nFpsNum || !nFpsDen) {
		DLog(L"COggVP8OutputPin::COggVP8OutputPin() : Invalid time base, use 25 fps");
		nFpsNum = 1;
		nFpsDen = 25;
	}
	m_rtAvgTimePerFrame = (REFERENCE_TIME)(10000000.0 * nFpsDen / nFpsNum);

	CMediaType mt;
	mt.majortype  = MEDIATYPE_Video;
	mt.formattype = FORMAT_VIDEOINFO2;
	mt.subtype    = FOURCCMap(FCC('VP80'));

	VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER2));
	memset(mt.Format(), 0, mt.FormatLength());

	vih2->AvgTimePerFrame         = m_rtAvgTimePerFrame;
	vih2->dwPictAspectRatioX      = Aspect.cx;
	vih2->dwPictAspectRatioY      = Aspect.cy;
	vih2->bmiHeader.biSize        = sizeof(vih2->bmiHeader);
	vih2->bmiHeader.biWidth       = width;
	vih2->bmiHeader.biHeight      = height;
	vih2->bmiHeader.biCompression = mt.subtype.Data1;
	vih2->bmiHeader.biPlanes      = 1;
	vih2->bmiHeader.biBitCount    = 24;
	vih2->bmiHeader.biSizeImage   = DIBSIZE(vih2->bmiHeader);

	mt.bFixedSizeSamples = 0;
	m_mts.push_back(mt);

	SetName(GetMediaTypeDesc(m_mts, pName, pFilter));

	*phr = S_OK;
}

REFERENCE_TIME COggVP8OutputPin::GetRefTime(__int64 granule_position)
{
	return (granule_position >> 32) * m_rtAvgTimePerFrame;
}

HRESULT COggVP8OutputPin::UnpackPacket(CAutoPtr<CPacket>& p, BYTE* pData, int len)
{
	CheckPointer(pData, E_FAIL);

	if (pData[0] == 0x4F) {
		return E_FAIL; // skip VP8 header
	}

	bool bKeyFrame = !(*pData & 1);
	if (!m_bSetKeyFrame) {
		m_bSetKeyFrame = bKeyFrame;
	}
	if (!m_bSetKeyFrame) {
		DLog(L"COggVP8OutputPin::UnpackPacket() : KeyFrame not found !!!");
		return E_FAIL; // waiting for a key frame after seeking
	}

	p->bSyncPoint = bKeyFrame;
	p->rtStart    = m_rtLast;
	p->rtStop     = m_rtLast + m_rtAvgTimePerFrame;
	p->SetData(pData, len);

	return S_OK;
}

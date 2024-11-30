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
#include <wmcodecdsp.h>
#include <moreuuids.h>
#include "AviFile.h"
#include "AviSplitter.h"

// option names
#define OPT_REGKEY_AVISplit  L"Software\\MPC-BE Filters\\AVI Splitter"
#define OPT_SECTION_AVISplit L"Filters\\AVI Splitter"
#define OPT_BadInterleaved   L"BadInterleavedSuport"
#define OPT_NeededReindex    L"NeededReindex"

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_Avi},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] = {
	{(LPWSTR)L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, nullptr, std::size(sudPinTypesIn), sudPinTypesIn},
	{(LPWSTR)L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, 0, nullptr}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CAviSplitterFilter), AviSplitterName, MERIT_NORMAL+1, std::size(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CAviSourceFilter), AviSourceName, MERIT_NORMAL+1, 0, nullptr, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CAviSplitterFilter>, nullptr, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CAviSourceFilter>, nullptr, &sudFilter[1]},
	{L"CAviSplitterPropertyPage", &__uuidof(CAviSplitterSettingsWnd), CreateInstance<CInternalPropertyPageTempl<CAviSplitterSettingsWnd>>},
};

int g_cTemplates = std::size(g_Templates);

STDAPI DllRegisterServer()
{
	const std::list<CString> chkbytes = {
		L"0,4,,52494646,8,4,,41564920", // 'RIFF....AVI '
		L"0,4,,52494646,8,4,,41564958", // 'RIFF....AVIX'
		L"0,4,,52494646,8,4,,414D5620", // 'RIFF....AMV '
	};

	RegisterSourceFilter(
		CLSID_AsyncReader,
		MEDIASUBTYPE_Avi,
		chkbytes,
		nullptr);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	UnRegisterSourceFilter(MEDIASUBTYPE_Avi);

	return AMovieDllRegisterServer2(FALSE);
}

#include "filters/filters/Filters.h"

CFilterApp theApp;

#else

#include "DSUtil/Profile.h"

#endif

//
// CAviSplitterFilter
//

CAviSplitterFilter::CAviSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(L"CAviSplitterFilter", pUnk, phr, __uuidof(this))
	, m_maxTimeStamp(INVALID_TIME)
	, m_bBadInterleavedSuport(true)
	, m_bSetReindex(true)
{
	m_nFlag |= SOURCE_SUPPORT_URL;
#ifdef REGISTER_FILTER
	CRegKey key;

	if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, OPT_REGKEY_AVISplit, KEY_READ)) {
		DWORD dw;

		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_BadInterleaved, dw)) {
			m_bBadInterleavedSuport = !!dw;
		}

		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_NeededReindex, dw)) {
			m_bSetReindex = !!dw;
		}
	}
#else
	CProfile& profile = AfxGetProfile();
	profile.ReadBool(OPT_SECTION_AVISplit, OPT_BadInterleaved, m_bBadInterleavedSuport);
	profile.ReadBool(OPT_SECTION_AVISplit, OPT_NeededReindex, m_bSetReindex);
#endif
}

STDMETHODIMP CAviSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	*ppv = nullptr;

	return
		QI(IAviSplitterFilter)
		QI(ISpecifyPropertyPages)
		QI(ISpecifyPropertyPages2)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CAviSplitterFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	if (m_pName && m_pName[0]==L'M' && m_pName[1]==L'P' && m_pName[2]==L'C') {
		(void)StringCchCopyW(pInfo->achName, NUMELMS(pInfo->achName), m_pName);
	} else {
		wcscpy_s(pInfo->achName, AviSourceName);
	}
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

template<typename T>
static void ParseHeader(T& var, CAviFile* pFile, CAviFile::strm_t* s, std::vector<CMediaType>& mts, REFERENCE_TIME AvgTimePerFrame, CSize headerAspect)
{
	if (s->cs.size()) {
		__int64 pos = pFile->GetPos();
		for (size_t i = 0; i < s->cs.size() - 1; i++) {
			if (s->cs[i].orgsize) {
				pFile->Seek(s->cs[i].filepos);
				CMediaType mt2;
				if (pFile->Read(var, s->cs[i].orgsize, &mt2)) {
					VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)mt2.pbFormat;
					pvih->AvgTimePerFrame = AvgTimePerFrame;
					if (headerAspect.cx && headerAspect.cy) {
						VIDEOINFOHEADER2* pvih2 = (VIDEOINFOHEADER2*)mt2.pbFormat;
						pvih2->dwPictAspectRatioX = headerAspect.cx;
						pvih2->dwPictAspectRatioY = headerAspect.cy;
					}
					mts.insert(mts.cbegin(), mt2);
				}
				break;
			}
		}
		pFile->Seek(pos);
	}
}

HRESULT CAviSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	m_tFrame.clear();

	m_pFile.reset(DNew CAviFile(pAsyncReader, hr));
	if (!m_pFile) {
		return E_OUTOFMEMORY;
	}
	if (FAILED(hr)) {
		m_pFile.reset();
		return hr;
	}
	m_pFile->SetBreakHandle(GetRequestHandle());

	if (!m_bBadInterleavedSuport && !m_pFile->IsInterleaved(!!(::GetKeyState(VK_SHIFT)&0x8000))) {
		m_pFile.reset();
		hr = E_FAIL;
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = m_pFile->GetTotalTime();

	{
		// reindex if needed

		int fReIndex = 0;

		for (DWORD track = 0; track < m_pFile->m_avih.dwStreams; track++) {
			if (!m_pFile->m_strms[track]->cs.size()) {
				fReIndex++;
			}
		}

		if (fReIndex && m_bSetReindex) {
			if (fReIndex == m_pFile->m_avih.dwStreams) {
				m_rtDuration = 0;
			}

			for (DWORD track = 0; track < m_pFile->m_avih.dwStreams; track++) {
				if (!m_pFile->m_strms[track]->cs.size()) {

					m_pFile->EmptyIndex(track);

					m_fAbort = false;

					__int64 pos = m_pFile->GetPos();
					m_pFile->Seek(0);
					UINT64 Size = 0;
					ReIndex(m_pFile->GetLength(), Size, track);

					if (m_fAbort) {
						m_pFile->EmptyIndex(track);
					}
					m_pFile->Seek(pos);

					m_fAbort = false;
					m_nOpenProgress = 100;
				}
			}
		}
	}

	bool fHasIndex = false;

	for (size_t i = 0; !fHasIndex && i < m_pFile->m_strms.size(); i++)
		if (m_pFile->m_strms[i]->cs.size() > 0) {
			fHasIndex = true;
		}

	for (size_t i = 0; i < m_pFile->m_strms.size(); i++) {
		CAviFile::strm_t* s = m_pFile->m_strms[i].get();

		if (fHasIndex && s->cs.size() == 0) {
			continue;
		}

		CMediaType mt;
		std::vector<CMediaType> mts;

		CStringW name, label;

		if (s->strh.fccType == FCC('vids')) {
			label = L"Video";

			ASSERT(s->strf.size() >= sizeof(BITMAPINFOHEADER));

			BITMAPINFOHEADER* pbmi = &((BITMAPINFO*)s->strf.data())->bmiHeader;
			RECT rc = {0, 0, pbmi->biWidth, abs(pbmi->biHeight)};

			REFERENCE_TIME AvgTimePerFrame = s->strh.dwRate > 0 ? 10000000ui64 * s->strh.dwScale / s->strh.dwRate : 0;

			DWORD dwBitRate = 0;
			if (s->cs.size() && AvgTimePerFrame > 0) {
				UINT64 size = 0;
				for (size_t j = 0; j < s->cs.size(); j++) {
					size += s->cs[j].orgsize;
				}
				dwBitRate = (DWORD)(10000000.0 * size * 8 / (s->cs.size() * AvgTimePerFrame) + 0.5);
				// need calculate in double, because the (10000000ui64 * size * 8) can give overflow
			}

			CSize headerAspect;
			if (m_pFile->m_vprp.dwFrameHeightInLines && m_pFile->m_vprp.dwFrameWidthInPixels
					&& m_pFile->m_vprp.FrameAspectRatioDen && m_pFile->m_vprp.FrameAspectRatioNum) {
				headerAspect = CSize(m_pFile->m_vprp.FrameAspectRatioDen, m_pFile->m_vprp.FrameAspectRatioNum);
				ReduceDim(headerAspect);
			}

			// building a basic media type
			mt.majortype = MEDIATYPE_Video;
			switch (pbmi->biCompression) {
				case BI_RGB:
				case BI_BITFIELDS:
					mt.subtype =
						pbmi->biBitCount == 1 ? MEDIASUBTYPE_RGB1 :
						pbmi->biBitCount == 4 ? MEDIASUBTYPE_RGB4 :
						pbmi->biBitCount == 8 ? MEDIASUBTYPE_RGB8 :
						pbmi->biBitCount == 16 ? pbmi->biCompression == BI_RGB ? MEDIASUBTYPE_RGB555 : MEDIASUBTYPE_RGB565 :
						pbmi->biBitCount == 24 ? MEDIASUBTYPE_RGB24 :
						pbmi->biBitCount == 32 ? MEDIASUBTYPE_ARGB32 :
						MEDIASUBTYPE_NULL;
					break;
				//case BI_RLE8: mt.subtype = MEDIASUBTYPE_RGB8; break;
				//case BI_RLE4: mt.subtype = MEDIASUBTYPE_RGB4; break;
				case FCC('V422'): // uncommon fourcc
					mt.subtype = FOURCCMap(pbmi->biCompression = FCC('YUY2'));
					break;
				case FCC('HDYC'): // UYVY with BT709
				case FCC('UYNV'): // uncommon fourcc
				case FCC('UYNY'): // uncommon fourcc
					mt.subtype = FOURCCMap(pbmi->biCompression = FCC('UYVY'));
					break;
				case FCC('P422'):
					mt.subtype = FOURCCMap(pbmi->biCompression = FCC('Y42B'));
					break;
				case FCC('RV24'): // uncommon fourcc
					pbmi->biCompression = BI_RGB;
					mt.subtype = MEDIASUBTYPE_RGB24;
					pbmi->biHeight = -pbmi->biHeight;
					break;
				case FCC('AVRn'): // uncommon fourcc
				case FCC('JPGL'): // uncommon fourcc
				case FCC('MSC2'): // uncommon fourcc
					mt.subtype = MEDIASUBTYPE_MJPG;
					break;
				case FCC('azpr'): // uncommon fourcc
					mt.subtype = FOURCCMap(pbmi->biCompression = FCC('rpza'));
					break;
				case FCC('mpg2'):
				case FCC('MPG2'):
				case FCC('MMES'): // Matrox MPEG-2 elementary video stream
				case FCC('M701'): // Matrox MPEG-2 I-frame codec
				case FCC('M702'):
				case FCC('M703'):
				case FCC('M704'):
				case FCC('M705'):
					mt.subtype = MEDIASUBTYPE_MPEG2_VIDEO;
					break;
				case FCC('mpg1'):
				case FCC('MPEG'):
					mt.subtype = MEDIASUBTYPE_MPEG1Payload;
					break;
				case FCC('H264'):
				case FCC('h264'):
				case FCC('X264'):
				case FCC('x264'):
				case FCC('AVC1'):
				case FCC('avc1'):
				case FCC('DAVC'):
				case FCC('SMV2'):
				case FCC('VSSH'):
				case FCC('Q264'):
				case FCC('V264'):
				case FCC('GAVC'):
				case FCC('UMSV'):
				case FCC('UNMC'):
					mt.subtype = FOURCCMap(pbmi->biCompression = FCC('H264'));
					break;
				case FCC('MAGY'):
				case FCC('M8RG'):
				case FCC('M8RA'):
				case FCC('M8G0'):
				case FCC('M8Y0'):
				case FCC('M8Y2'):
				case FCC('M8Y4'):
				case FCC('M8YA'):
				case FCC('M0RG'):
				case FCC('M0RA'):
				case FCC('M0G0'):
				case FCC('M0Y2'):
				case FCC('M2RG'):
				case FCC('M2RA'):
				//case FCC('M4RG'):
				//case FCC('M4RA'):
					mt.subtype = FOURCCMap(FCC('MAGY'));
					break;
				case FCC('av01'):
					mt.subtype = FOURCCMap(pbmi->biCompression = FCC('AV01'));
					break;
				case FCC('MPNG'):
					mt.subtype = MEDIASUBTYPE_PNG;
					break;
				case FCC('DXSB'):
				case FCC('DXSA'):
					label = L"XSub";
				default:
					mt.subtype = FOURCCMap(pbmi->biCompression);
			}

			mt.formattype			= FORMAT_VideoInfo;
			VIDEOINFOHEADER* pvih	= (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + (ULONG)s->strf.size() - sizeof(BITMAPINFOHEADER));
			memset(mt.Format(), 0, mt.FormatLength());
			memcpy(&pvih->bmiHeader, s->strf.data(), s->strf.size());
			pvih->AvgTimePerFrame	= AvgTimePerFrame;
			pvih->dwBitRate			= dwBitRate;
			pvih->rcSource			= pvih->rcTarget = rc;

			mt.SetSampleSize(s->strh.dwSuggestedBufferSize > 0
							 ? s->strh.dwSuggestedBufferSize*3/2
							 : (pvih->bmiHeader.biWidth*pvih->bmiHeader.biHeight*4));
			mts.push_back(mt);

			// building a special media type
			switch (pbmi->biCompression) {
				case FCC('HM10'):
					{
						CBaseSplitterFileEx::hevchdr h;
						ParseHeader(h, m_pFile.get(), s, mts, AvgTimePerFrame, headerAspect);
					}
					break;
				case FCC('mpg2'):
				case FCC('MPG2'):
				case FCC('MMES'):
				case FCC('M701'):
				case FCC('M702'):
				case FCC('M703'):
				case FCC('M704'):
				case FCC('M705'):
					{
						CBaseSplitterFileEx::seqhdr h;
						ParseHeader(h, m_pFile.get(), s, mts, AvgTimePerFrame, headerAspect);
					}
					break;
				case FCC('H264'):
				case FCC('h264'):
				case FCC('X264'):
				case FCC('x264'):
				case FCC('AVC1'):
				case FCC('avc1'):
				case FCC('DAVC'):
				case FCC('SMV2'):
				case FCC('VSSH'):
				case FCC('Q264'):
				case FCC('V264'):
				case FCC('GAVC'):
				case FCC('UMSV'):
				case FCC('UNMC'):
					if (s->strf.size() > sizeof(BITMAPINFOHEADER)) {
						size_t extralen	= s->strf.size() - sizeof(BITMAPINFOHEADER);
						BYTE* extra		= s->strf.data() + (s->strf.size() - extralen);

						CSize aspect(pbmi->biWidth, pbmi->biHeight);
						ReduceDim(aspect);
						if (headerAspect.cx && headerAspect.cy) {
							aspect = headerAspect;
						}

						pbmi->biCompression = FCC('AVC1');
						CMediaType mt2;
						if (SUCCEEDED(CreateMPEG2VIfromAVC(&mt2, pbmi, AvgTimePerFrame, aspect, extra, extralen))) {
							mts.insert(mts.cbegin(), mt2);
						}
					}
					if (mts.size() == 1) {
						CBaseSplitterFileEx::avchdr h;
						ParseHeader(h, m_pFile.get(), s, mts, AvgTimePerFrame, headerAspect);
					}
					break;
			}
		} else if (s->strh.fccType == FCC('auds') || s->strh.fccType == FCC('amva')) {
			label = L"Audio";

			ASSERT(s->strf.size() >= sizeof(WAVEFORMATEX)
				   || s->strf.size() == sizeof(PCMWAVEFORMAT));

			WAVEFORMATEX* pwfe = (WAVEFORMATEX*)s->strf.data();

			if (pwfe->nBlockAlign == 0) {
				continue;
			}

			if (pwfe->wFormatTag == WAVE_FORMAT_FAAD_AAC) {
				pwfe->wFormatTag = WAVE_FORMAT_RAW_AAC1;
			}

			mt.majortype = MEDIATYPE_Audio;
			if (m_pFile->m_isamv) {
				mt.subtype = FOURCCMap(MAKEFOURCC('A','M','V','A'));
			} else {
				mt.subtype = FOURCCMap(pwfe->wFormatTag);
			}
			mt.formattype = FORMAT_WaveFormatEx;
			if (nullptr == mt.AllocFormatBuffer(std::max(s->strf.size(), sizeof(WAVEFORMATEX)))) {
				continue;
			}
			memcpy(mt.Format(), s->strf.data(), s->strf.size());
			pwfe = (WAVEFORMATEX*)mt.Format();
			if (s->strf.size() == sizeof(PCMWAVEFORMAT)) {
				pwfe->cbSize = 0;
			}
			if (pwfe->wFormatTag == WAVE_FORMAT_PCM) {
				pwfe->nBlockAlign = pwfe->nChannels*pwfe->wBitsPerSample>>3;
			}
			if (pwfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
				mt.subtype = ((WAVEFORMATEXTENSIBLE*)pwfe)->SubFormat;
			}
			if (!pwfe->nChannels) {
				pwfe->nChannels = 2;
			}

			if (mt.subtype == MEDIASUBTYPE_RAW_AAC1) {
				pwfe->wFormatTag = WAVE_FORMAT_RAW_AAC1;
				WAVEFORMATEX* pwfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + 5);
				pwfe->cbSize = MakeAACInitData((BYTE*)(pwfe + 1), 0, pwfe->nSamplesPerSec, pwfe->nChannels);
			}

			mt.SetSampleSize(s->strh.dwSuggestedBufferSize > 0
							 ? s->strh.dwSuggestedBufferSize*3/2
							 : (pwfe->nChannels*pwfe->nSamplesPerSec*32>>3));
			mts.push_back(mt);
		} else if (s->strh.fccType == FCC('mids')) {
			label = L"Midi";

			mt.majortype = MEDIATYPE_Midi;
			mt.subtype = MEDIASUBTYPE_NULL;
			mt.formattype = FORMAT_None;
			mt.SetSampleSize(s->strh.dwSuggestedBufferSize > 0
							 ? s->strh.dwSuggestedBufferSize*3/2
							 : (1024*1024));
			mts.push_back(mt);
		} else if (s->strh.fccType == FCC('txts')) {
			label = L"Text";

			mt.majortype = MEDIATYPE_Text;
			mt.subtype = MEDIASUBTYPE_NULL;
			mt.formattype = FORMAT_None;
			mt.SetSampleSize(s->strh.dwSuggestedBufferSize > 0
							 ? s->strh.dwSuggestedBufferSize*3/2
							 : (1024*1024));
			mts.push_back(mt);
		} else if (s->strh.fccType == FCC('iavs')) {
			label = L"Interleaved";

			ASSERT(s->strh.fccHandler == FCC('dvsd'));

			mt.majortype = MEDIATYPE_Interleaved;
			mt.subtype = FOURCCMap(s->strh.fccHandler);
			mt.formattype = FORMAT_DvInfo;
			mt.SetFormat(s->strf.data(), std::max((ULONG)s->strf.size(), (ULONG)sizeof(DVINFO)));
			mt.SetSampleSize(s->strh.dwSuggestedBufferSize > 0
							 ? s->strh.dwSuggestedBufferSize*3/2
							 : (1024*1024));
			mts.push_back(mt);
		}

		if (mts.empty()) {
			DLog(L"CAviSourceFilter: Unsupported stream (%u)", (unsigned)i);
			continue;
		}

		//Put filename at front sometime(eg. ~temp.avi) will cause filter graph
		//stop check this pin. Not sure the reason exactly. but it happens.
		//If you know why, please emailto: tomasen@gmail.com
		if (s->strn.IsEmpty()) {
			name.Format(L"%s %u", label, i);
		} else {
			name.Format(L"%s (%s %u)", UTF8orLocalToWStr(s->strn), label, i);
		}

		HRESULT hr;

		std::unique_ptr<CBaseSplitterOutputPin> pPinOut(DNew CAviSplitterOutputPin(mts, name, this, this, &hr));
		AddOutputPin(i, pPinOut);
	}

	for (const auto info : m_pFile->m_info) {
		switch (info.first) {
			case FCC('INAM'):
				SetProperty(L"TITL", CStringW(info.second));
				break;
			case FCC('IART'):
				SetProperty(L"AUTH", CStringW(info.second));
				break;
			case FCC('ICOP'):
				SetProperty(L"CPYR", CStringW(info.second));
				break;
			case FCC('ISBJ'):
				SetProperty(L"DESC", CStringW(info.second));
				break;
		}
	}

	m_tFrame.resize(m_pFile->m_avih.dwStreams);

	return m_pOutputs.size() > 0 ? S_OK : E_FAIL;
}

bool CAviSplitterFilter::DemuxInit()
{
	SetThreadName((DWORD)-1, "CAviSplitterFilter");

	if (!m_pFile) {
		return false;
	}

	return true;
}

HRESULT CAviSplitterFilter::ReIndex(__int64 end, UINT64& Size, DWORD TrackNumber)
{
	HRESULT hr = S_OK;

	if (TrackNumber >= m_pFile->m_avih.dwStreams) {
		return E_FAIL;
	}

	while (S_OK == hr && m_pFile->GetPos() < end && !m_fAbort) {
		__int64 pos = m_pFile->GetPos();

		DWORD id = 0, size;
		if (S_OK != m_pFile->ReadAvi(id) || id == 0) {
			return E_FAIL;
		}

		if (id == FCC('RIFF') || id == FCC('LIST')) {
			if (S_OK != m_pFile->ReadAvi(size) || S_OK != m_pFile->ReadAvi(id)) {
				return E_FAIL;
			}

			size += (size&1) + 8;

			if (id == FCC('AVI ') || id == FCC('AVIX') || id == FCC('movi') || id == FCC('rec ')) {
				hr = ReIndex(pos + size, Size, TrackNumber);
			}
		} else {
			if (S_OK != m_pFile->ReadAvi(size)) {
				return E_FAIL;
			}

			DWORD nTrackNumber = TRACKNUM(id);

			if (nTrackNumber == TrackNumber) {
				CAviFile::strm_t* s = m_pFile->m_strms[nTrackNumber].get();

				WORD type = TRACKTYPE(id);

				if (type == 'db' || type == 'dc' || /*type == 'pc' ||*/ type == 'wb'
						|| type == 'iv' || type == '__' || type == 'xx') {
					CAviFile::strm_t::chunk c;
					c.filepos	= pos;
					c.size		= Size;
					c.orgsize	= size;

					if (s->cs.empty()) {
						c.fKeyFrame = true; // force the first frame as a keyframe
					}
					else if (size == 0) {
						c.fKeyFrame = false; // drop frame
					}
					else {
						c.fKeyFrame = true;

						if (s->strh.fccType == FCC('vids') && size >= 4 && (type == 'db' || type == 'dc' || type == 'wb')) {
							DWORD frametype;
							if (S_OK == m_pFile->ReadAvi(frametype)) {
								// Xvid   : 0xb0010000 - keyframe, 0xb6010000 - delta frame.
								// DivX 5 : 0x00010000 - keyframe, 0xb6010000 - delta frame.
								if (frametype == 0xb6010000) {
									c.fKeyFrame = false; // delta frame for Xvid and DivX 5
								}
							}
						}
					}

					c.fChunkHdr	= true;
					s->cs.push_back(c);

					Size += s->GetChunkSize(size);
					s->totalsize = Size;

					REFERENCE_TIME rt	= s->GetRefTime((DWORD)s->cs.size()-1, Size);
					m_rtDuration		= std::max(rt, m_rtDuration);
				}
			}

			size += (size&1) + 8;
		}

		m_pFile->Seek(pos + size);

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

	return hr;
}

void CAviSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	std::fill(m_tFrame.begin(), m_tFrame.end(), 0);
	m_pFile->Seek(0);

	if (rt > 0) {
		for (DWORD track = 0; track < m_pFile->m_avih.dwStreams; track++) {
			CAviFile::strm_t* s = m_pFile->m_strms[track].get();

			if (s->IsRawSubtitleStream() || s->cs.empty()) {
				continue;
			}

			//ASSERT(s->GetFrame(rt) == s->GetKeyFrame(rt)); // fast seek test
			m_tFrame[track] = s->GetKeyFrame(rt);
		}
	}
}

bool CAviSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	std::vector<BOOL> fDiscontinuity;
	fDiscontinuity.resize(m_pFile->m_avih.dwStreams, FALSE);

	while (SUCCEEDED(hr) && !CheckRequest(nullptr)) {
		DWORD curTrack = DWORD_MAX;
		UINT64 minpos = 0;
		REFERENCE_TIME minTime = INT64_MAX;
		for (DWORD track = 0; track < m_pFile->m_avih.dwStreams; track++) {
			CAviFile::strm_t* s = m_pFile->m_strms[track].get();
			DWORD f = m_tFrame[track];

			if (f >= (DWORD)s->cs.size()) {
				continue;
			}

			if (s->IsRawSubtitleStream()) {
				// TODO: get subtitle time from index
				minTime  = 0;
				curTrack = track;
				break; // read all subtitles at once
			}

			const REFERENCE_TIME rt = s->GetRefTime(f, s->cs[f].size);
			if (curTrack == DWORD_MAX
					|| (llabs(minTime - rt) <= UNITS && s->cs[f].filepos < minpos)
					|| (llabs(minTime - rt) > UNITS && rt < minTime)) {
				curTrack = track;
				minTime  = rt;
				minpos   = s->cs[f].filepos;
			}
		}

		if (minTime == INT64_MAX) {
			return true;
		}

		do {
			CAviFile::strm_t* s = m_pFile->m_strms[curTrack].get();
			DWORD f = m_tFrame[curTrack];

			m_pFile->Seek(s->cs[f].filepos);
			DWORD size = 0;

			if (s->cs[f].fChunkHdr) {
				DWORD id = 0;
				if (S_OK != m_pFile->ReadAvi(id) || id == 0 || curTrack != TRACKNUM(id) || S_OK != m_pFile->ReadAvi(size)) {
					fDiscontinuity[curTrack] = TRUE;
					break;
				}

				if (size != s->cs[f].orgsize) {
					DLog(L"CAviSplitterFilter::DemuxLoop() : incorrect chunk size. TrackNum: %d, by index: %u, by header: %u", TRACKNUM(id), s->cs[f].orgsize, size);
					const DWORD maxChunkSize = (f + 1 < s->cs.size() ? s->cs[f + 1].filepos : m_pFile->GetLength()) - m_pFile->GetPos();
					if (size > maxChunkSize) {
						DLog(L"    using from index");
						size = s->cs[f].orgsize;
					}
					fDiscontinuity[curTrack] = TRUE;
					//break; // Why so, why break ??? If anyone knows - please describe ...
				}
			} else {
				size = s->cs[f].orgsize;
			}

			std::unique_ptr<CPacket> p(DNew CPacket());

			p->TrackNumber		= (DWORD)curTrack;
			p->bSyncPoint		= (BOOL)s->cs[f].fKeyFrame;
			p->bDiscontinuity	= fDiscontinuity[curTrack];
			p->rtStart			= s->GetRefTime(f, s->cs[f].size);
			p->rtStop			= s->GetRefTime(f + 1, f + 1 < (DWORD)s->cs.size() ? s->cs[f + 1].size : s->totalsize);
			p->resize(size);
			if (S_OK != (hr = m_pFile->ByteRead(p->data(), p->size()))) {
				return true;
			}
#if defined(DEBUG_OR_LOG) && 0
			DLog(L"%d (%d): %I64d - %I64d, %I64d - %I64d (size = %u)",
					curTrack, (int)p->bSyncPoint,
					(p->rtStart) / 10000, (p->rtStop) / 10000,
					(p->rtStart - m_rtStart) / 10000, (p->rtStop - m_rtStart) / 10000,
					size);
#endif
			m_maxTimeStamp = std::max(m_maxTimeStamp, p->rtStart);

			hr = DeliverPacket(std::move(p));

			fDiscontinuity[curTrack] = false;
		} while (0);

		m_tFrame[curTrack]++;
	}

	if (m_maxTimeStamp != INVALID_TIME) {
		m_rtCurrent = m_maxTimeStamp;
	}

	return true;
}

// IKeyFrameInfo

STDMETHODIMP CAviSplitterFilter::GetKeyFrameCount(UINT& nKFs)
{
	if (!m_pFile) {
		return E_UNEXPECTED;
	}

	HRESULT hr = S_OK;

	nKFs = 0;

	for (const auto& s : m_pFile->m_strms) {
		if (s->strh.fccType != FCC('vids')) {
			continue;
		}

		for (size_t j = 0; j < s->cs.size(); j++) {
			CAviFile::strm_t::chunk& c = s->cs[j];
			if (c.fKeyFrame) {
				++nKFs;
			}
		}

		if (nKFs == s->cs.size()) {
			hr = S_FALSE;
		}

		break;
	}

	return hr;
}

STDMETHODIMP CAviSplitterFilter::GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs)
{
	CheckPointer(pFormat, E_POINTER);
	CheckPointer(pKFs, E_POINTER);

	if (!m_pFile) {
		return E_UNEXPECTED;
	}
	if (*pFormat != TIME_FORMAT_MEDIA_TIME && *pFormat != TIME_FORMAT_FRAME) {
		return E_INVALIDARG;
	}

	for (const auto& s : m_pFile->m_strms) {
		if (s->strh.fccType != FCC('vids')) {
			continue;
		}
		bool fConvertToRefTime = !!(*pFormat == TIME_FORMAT_MEDIA_TIME);

		UINT nKFsTmp = 0;

		for (size_t j = 0; j < s->cs.size() && nKFsTmp < nKFs; j++) {
			if (s->cs[j].fKeyFrame) {
				pKFs[nKFsTmp++] = fConvertToRefTime ? s->GetRefTime(j, s->cs[j].size) : j;
			}
		}
		nKFs = nKFsTmp;

		return S_OK;
	}

	return E_FAIL;
}

// ISpecifyPropertyPages2

STDMETHODIMP CAviSplitterFilter::GetPages(CAUUID* pPages)
{
	CheckPointer(pPages, E_POINTER);

	pPages->cElems = 1;
	pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID) * pPages->cElems);
	pPages->pElems[0] = __uuidof(CAviSplitterSettingsWnd);

	return S_OK;
}

STDMETHODIMP CAviSplitterFilter::CreatePage(const GUID& guid, IPropertyPage** ppPage)
{
	CheckPointer(ppPage, E_POINTER);

	if (*ppPage != nullptr) {
		return E_INVALIDARG;
	}

	HRESULT hr;

	if (guid == __uuidof(CAviSplitterSettingsWnd)) {
		(*ppPage = DNew CInternalPropertyPageTempl<CAviSplitterSettingsWnd>(nullptr, &hr))->AddRef();
	}

	return *ppPage ? S_OK : E_FAIL;
}

// IAviSplitterFilter
STDMETHODIMP CAviSplitterFilter::Apply()
{
#ifdef REGISTER_FILTER
	CRegKey key;
	if (ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, OPT_REGKEY_AVISplit)) {
		key.SetDWORDValue(OPT_BadInterleaved, m_bBadInterleavedSuport);
		key.SetDWORDValue(OPT_NeededReindex, m_bSetReindex);
	}
#else
	CProfile& profile = AfxGetProfile();
	profile.WriteBool(OPT_SECTION_AVISplit, OPT_BadInterleaved, m_bBadInterleavedSuport);
	profile.WriteBool(OPT_SECTION_AVISplit, OPT_NeededReindex, m_bSetReindex);
#endif
	return S_OK;
}

STDMETHODIMP CAviSplitterFilter::SetBadInterleavedSuport(BOOL nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_bBadInterleavedSuport = !!nValue;
	return S_OK;
}

STDMETHODIMP_(BOOL) CAviSplitterFilter::GetBadInterleavedSuport()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_bBadInterleavedSuport;
}

STDMETHODIMP CAviSplitterFilter::SetReindex(BOOL nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_bSetReindex = !!nValue;
	return S_OK;
}

STDMETHODIMP_(BOOL) CAviSplitterFilter::GetReindex()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_bSetReindex;
}

//
// CAviSourceFilter
//

CAviSourceFilter::CAviSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CAviSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.reset();
}

//
// CAviSplitterOutputPin
//

CAviSplitterOutputPin::CAviSplitterOutputPin(std::vector<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseSplitterOutputPin(mts, pName, pFilter, pLock, phr)
{
}

HRESULT CAviSplitterOutputPin::CheckConnect(IPin* pPin)
{
	int iPosition = 0;
	CMediaType mt;
	while (S_OK == GetMediaType(iPosition++, &mt)) {
		if (mt.majortype == MEDIATYPE_Video
				&& (mt.subtype == FOURCCMap(FCC('IV32'))
					|| mt.subtype == FOURCCMap(FCC('IV31'))
					|| mt.subtype == FOURCCMap(FCC('IF09')))) {
			CLSID clsid = GetCLSID(GetFilterFromPin(pPin));
			if (clsid == CLSID_VideoMixingRenderer || clsid == CLSID_OverlayMixer) {
				return E_FAIL;
			}
		}

		mt.InitMediaType();
	}

	return __super::CheckConnect(pPin);
}

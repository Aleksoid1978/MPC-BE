/*
 * (C) 2016-2018 see Authors.txt
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
#include "BinkSplitter.h"
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
	{&__uuidof(CBinkSplitterFilter), BinkSplitterName, MERIT_NORMAL+1, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CBinkSourceFilter), BinkSourceName, MERIT_NORMAL+1, 0, nullptr, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] =
{
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CBinkSplitterFilter>, nullptr, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CBinkSourceFilter>, nullptr, &sudFilter[1]},
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_NULL, L"0,3,,42494B");

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

enum BinkAudFlags {
	BINK_AUD_16BITS = 0x4000, ///< prefer 16-bit output
	BINK_AUD_STEREO = 0x2000,
	BINK_AUD_USEDCT = 0x1000,
};

//
// CBinkSplitterFilter
//

CBinkSplitterFilter::CBinkSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CBinkSplitterFilter"), pUnk, phr, __uuidof(this))
{
}

// https://wiki.multimedia.cx/index.php?title=Bink_Container
// https://github.com/FFmpeg/FFmpeg/blob/master/libavformat/bink.c

HRESULT CBinkSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;
	m_pFile.Free();

	m_pFile.Attach(DNew CBaseSplitterFile(pAsyncReader, hr, FM_FILE | FM_FILE_DL));
	if (!m_pFile) {
		return E_OUTOFMEMORY;
	}
	if (FAILED(hr)) {
		m_pFile.Free();
		return hr;
	}
	if (m_pFile->IsStreaming()) {
		m_pFile.Free();
		return E_FAIL;
	}
	m_pFile->SetBreakHandle(GetRequestHandle());

	UINT32 codec_tag = 0;
	m_pFile->ByteRead((BYTE*)&codec_tag, 4);
	if (codec_tag != FCC('BIKb') && codec_tag != FCC('BIKi')) {
		return E_FAIL;
	}

	UINT32 file_size		= 0;
	UINT32 num_frames		= 0;
	UINT32 larges_framesize = 0;
	UINT32 width			= 0;
	UINT32 height			= 0;
	UINT32 fps_num			= 0;
	UINT32 fps_den			= 0;
	UINT32 video_flags		= 0;

	m_pFile->ByteRead((BYTE*)&file_size, 4);
	file_size += 8;
	m_pFile->ByteRead((BYTE*)&num_frames, 4);
	if (num_frames > 1000000) {
		DLog(L"CBinkSplitter: invalid header: more than 1000000 frames");
		return E_FAIL;
	}
	m_pFile->ByteRead((BYTE*)&larges_framesize, 4);
	if (larges_framesize > file_size) {
		DLog(L"CBinkSplitter: invalid header: largest frame size greater than file size");
		return E_FAIL;
	}
	m_pFile->Skip(4);
	m_pFile->ByteRead((BYTE*)&width, 4);
	m_pFile->ByteRead((BYTE*)&height, 4);
	m_pFile->ByteRead((BYTE*)&fps_num, 4);
	m_pFile->ByteRead((BYTE*)&fps_den, 4);
	if (fps_num == 0 || fps_den == 0) {
		DLog(L"CBinkSplitter: invalid header: invalid fps (%u / %u)", fps_num, fps_den);
		return E_FAIL;
	}
	m_pFile->ByteRead((BYTE*)&video_flags, 4);
	m_pFile->ByteRead((BYTE*)&num_audio_tracks, 4);
	if (num_audio_tracks > 256) {
		DLog(L"CBinkSplitter: invalid header: more than 256 audio tracks (%u)", num_audio_tracks);
		return E_FAIL;
	}

	m_fps.num = fps_num;
	m_fps.den = fps_den;

	std::vector<CMediaType> mts;
	CMediaType mt;
	{
		mt.InitMediaType();
		mt.majortype = MEDIATYPE_Video;
		mt.subtype = FOURCCMap(codec_tag);
		mt.formattype = FORMAT_VideoInfo;
		VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + sizeof(video_flags));
		memset(mt.Format(), 0, mt.FormatLength());
		pvih->AvgTimePerFrame = 10000000i64 * fps_den / fps_num;
		pvih->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		pvih->bmiHeader.biWidth = width;
		pvih->bmiHeader.biHeight = height;
		pvih->bmiHeader.biPlanes = 1;
		pvih->bmiHeader.biBitCount = 12;
		pvih->bmiHeader.biCompression = codec_tag;
		pvih->bmiHeader.biSizeImage = width * height * 12 / 8;
		memcpy(pvih + 1, &video_flags, sizeof(video_flags));
		mt.SetSampleSize(0);
		mt.SetTemporalCompression(FALSE);
		mts.push_back(mt);
		CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CBaseSplitterOutputPin(mts, L"Video", this, this, &hr));
		AddOutputPin(0, pPinOut);
	}

	if (num_audio_tracks) {
		struct audiotrack_t {
			UINT16 sample_rate = 0;
			UINT16 audio_flags = 0;
			UINT32 track_ID = 0;
		};
		std::vector<audiotrack_t> audiotracks(num_audio_tracks);

		m_pFile->Skip(4 * num_audio_tracks);

		for (unsigned i = 0; i < num_audio_tracks; i++) {
			m_pFile->ByteRead((BYTE*)&audiotracks[i].sample_rate, 2);
			m_pFile->ByteRead((BYTE*)&audiotracks[i].audio_flags, 2);

			mts.clear();
			mt.InitMediaType();
			mt.majortype = MEDIATYPE_Audio;
			mt.subtype = (audiotracks[i].audio_flags & BINK_AUD_USEDCT) ? MEDIASUBTYPE_BINKA_DCT : MEDIASUBTYPE_BINKA_RDFT;
			mt.formattype = FORMAT_WaveFormatEx;
			WAVEFORMATEX wfe;
			memset(&wfe, 0, sizeof(wfe));
			wfe.wFormatTag = 0x4142;
			wfe.nChannels = (audiotracks[i].audio_flags & BINK_AUD_STEREO) ? 2 : 1;
			wfe.nSamplesPerSec = audiotracks[i].sample_rate;
			wfe.wBitsPerSample = 16;
			mt.SetFormat((BYTE*)&wfe, sizeof(wfe));
			mt.lSampleSize = 1;
			mts.push_back(mt);
			CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CBaseSplitterOutputPin(mts, L"Audio", this, this, &hr));
			AddOutputPin(i + 1, pPinOut);
		}

		for (unsigned i = 0; i < num_audio_tracks; i++) {
			m_pFile->ByteRead((BYTE*)&audiotracks[i].track_ID, 4);
		}
	}

	/* frame index table */
	m_seektable.clear();
	m_seektable.reserve(num_frames);
	UINT32 next_pos = 0;
	m_pFile->ByteRead((BYTE*)&next_pos, 4);
	for (unsigned i = 0; i < num_frames; i++) {
		UINT32 pos = next_pos;
		UINT32 keyframe;

		if (i == num_frames - 1) {
			next_pos = file_size;
			keyframe = 0;
		} else {
			m_pFile->ByteRead((BYTE*)&next_pos, 4);
			keyframe = pos & 1;
		}
		pos &= ~1;
		next_pos &= ~1;

		int size = next_pos - pos;
		if (size < 0) {
			DLog(L"CBinkSplitter: invalid frame index table");
			return E_FAIL;
		}

		frame_t frame = { pos, keyframe, size };
		m_seektable.push_back(frame);
	}

	if (m_seektable.size()) {
		m_pFile->Seek(m_seektable[0].pos);
	} else {
		DLog(L"CBinkSplitter: files without index not supported!");
		return E_FAIL;
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = 10000000i64 * num_frames * fps_den / fps_num;

	CString time = ReftimeToString(m_rtDuration);

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

bool CBinkSplitterFilter::DemuxInit()
{
	SetThreadName((DWORD)-1, "CBinkSplitterFilter");
	m_indexpos = 0;

	return(true);
}

void CBinkSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	if (rt <= 0 || m_seektable.empty()) {
		m_indexpos = 0;
	}
	else if (rt >= m_rtDuration) {
		m_indexpos = m_seektable.size();
	}
	else {
		m_indexpos = llMulDiv(rt, m_fps.num, m_fps.den * 10000000i64, 0);
		// found keyframe
		while (m_indexpos > 0 && !m_seektable[m_indexpos].keyframe) {
			m_indexpos--;
		}
	}
}

bool CBinkSplitterFilter::DemuxLoop()
{
	while (m_indexpos < m_seektable.size() && !CheckRequest(nullptr)) {
		HRESULT hr = S_OK;

		REFERENCE_TIME rtstart = 10000000i64 * m_indexpos * m_fps.den / m_fps.num;
		REFERENCE_TIME rtstop = 10000000i64 * (m_indexpos + 1) * m_fps.den / m_fps.num;

		CString timestart = ReftimeToString(rtstart);
		CString timestop = ReftimeToString(rtstop);

		m_pFile->Seek(m_seektable[m_indexpos].pos);

		if (num_audio_tracks) {
			for (unsigned i = 0; i < num_audio_tracks; i++) {
				UINT32 packet_size = 0;
				m_pFile->ByteRead((BYTE*)&packet_size, 4);
				if (packet_size >= 4) {
					CAutoPtr<CPacket> pa(DNew CPacket());
					if (pa->SetCount(packet_size)) {
						pa->TrackNumber = i + 1;
						pa->bSyncPoint = TRUE;
						pa->rtStart = rtstart;
						pa->rtStop = rtstop;

						m_pFile->ByteRead(pa->data(), packet_size);

						hr = DeliverPacket(pa);
					}
				}
				else {
					m_pFile->Skip(packet_size);
				}
			}
		}

		int packet_size = m_seektable[m_indexpos].pos + m_seektable[m_indexpos].size - m_pFile->GetPos();
		ASSERT(packet_size > 0);

		CAutoPtr<CPacket> pv(DNew CPacket());
		if (pv->SetCount(packet_size)) {
			pv->TrackNumber = 0;
			pv->bSyncPoint = m_seektable[m_indexpos].keyframe;
			pv->rtStart = rtstart;
			pv->rtStop = rtstop;

			m_pFile->ByteRead(pv->data(), packet_size);

			hr = DeliverPacket(pv);
		}

		m_indexpos++;
	}

	return true;
}

// CBaseFilter

STDMETHODIMP CBinkSplitterFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	wcscpy_s(pInfo->achName, BinkSplitterName);

	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

// IKeyFrameInfo

STDMETHODIMP CBinkSplitterFilter::GetKeyFrameCount(UINT& nKFs)
{
	nKFs = 0;

	if (m_seektable.empty()) {
		return E_UNEXPECTED;
	}

	UINT count = 0;
	for (auto frame : m_seektable) {
		if (frame.keyframe) {
			count++;
		}
	}

	nKFs = count;

	return (m_seektable.size() == count) ? S_FALSE : S_OK;
}

STDMETHODIMP CBinkSplitterFilter::GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs)
{
	CheckPointer(pFormat, E_POINTER);
	CheckPointer(pKFs, E_POINTER);

	if (m_seektable.empty()) {
		return E_UNEXPECTED;
	}
	if (*pFormat != TIME_FORMAT_MEDIA_TIME && *pFormat != TIME_FORMAT_FRAME) {
		return E_INVALIDARG;
	}

	bool bRefTime = (*pFormat == TIME_FORMAT_MEDIA_TIME);

	UINT count = 0;
	for (size_t i = 0; i < m_seektable.size() && count < nKFs; i++) {
		if (m_seektable[i].keyframe) {
			pKFs[count++] = bRefTime ? (10000000i64 * i * m_fps.den / m_fps.num) : i;
		}
	}

	nKFs = count;

	return S_OK;
}

//
// CBinkSourceFilter
//

CBinkSourceFilter::CBinkSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBinkSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.Free();
}

STDMETHODIMP CBinkSourceFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	wcscpy_s(pInfo->achName, BinkSourceName);

	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

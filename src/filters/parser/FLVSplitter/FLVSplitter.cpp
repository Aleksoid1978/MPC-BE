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
#include "FLVSplitter.h"
#include "../BaseSplitter/FrameDuration.h"
#include "../../../DSUtil/DSUtil.h"
#include "../../../DSUtil/VideoParser.h"

#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include <moreuuids.h>

#define FLV_AUDIODATA     8
#define FLV_VIDEODATA     9
#define FLV_SCRIPTDATA    18

#define FLV_AUDIO_PCM     0 // Linear PCM, platform endian
#define FLV_AUDIO_ADPCM   1 // ADPCM
#define FLV_AUDIO_MP3     2 // MP3
#define FLV_AUDIO_PCMLE   3 // Linear PCM, little endian
#define FLV_AUDIO_NELLY16 4 // Nellymoser 16 kHz mono
#define FLV_AUDIO_NELLY8  5 // Nellymoser 8 kHz mono
#define FLV_AUDIO_NELLY   6 // Nellymoser
// 7 = G.711 A-law logarithmic PCM (reserved)
// 8 = G.711 mu-law logarithmic PCM (reserved)
// 9 = reserved
#define FLV_AUDIO_AAC     10 // AAC
#define FLV_AUDIO_SPEEX   11 // Speex
// 14 = MP3 8 kHz (reserved)
// 15 = Device-specific sound (reserved)

//#define FLV_VIDEO_JPEG   1 // non-standard? need samples
#define FLV_VIDEO_H263     2 // Sorenson H.263
#define FLV_VIDEO_SCREEN   3 // Screen video
#define FLV_VIDEO_VP6      4 // On2 VP6
#define FLV_VIDEO_VP6A     5 // On2 VP6 with alpha channel
#define FLV_VIDEO_SCREEN2  6 // Screen video version 2
#define FLV_VIDEO_AVC      7 // AVC
#define FLV_VIDEO_HM62    11 // HM6.2
#define FLV_VIDEO_HM91    12 // HM9.1
#define FLV_VIDEO_HM10    13 // HM10.0
#define FLV_VIDEO_HEVC    14 // HEVC (HM version write to MetaData "HM compatibility")

#define AMF_END_OF_OBJECT			0x09

#define KEYFRAMES_TAG				L"keyframes"
#define KEYFRAMES_TIMESTAMP_TAG		L"times"
#define KEYFRAMES_BYTEOFFSET_TAG	L"filepositions"

#define IsValidTag(TagType)			(TagType == FLV_AUDIODATA || TagType == FLV_VIDEODATA || TagType == FLV_SCRIPTDATA)
#define IsAVCCodec(CodecID)			(CodecID == FLV_VIDEO_AVC || CodecID == FLV_VIDEO_HM91 || CodecID == FLV_VIDEO_HM10 || CodecID == FLV_VIDEO_HEVC)

#define ValidateTag(t)				(t.DataSize || t.PreviousTagSize)


#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_FLV},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, nullptr, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, 0, nullptr}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CFLVSplitterFilter), FlvSplitterName, MERIT_NORMAL, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CFLVSourceFilter), FlvSourceName, MERIT_NORMAL, 0, nullptr, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CFLVSplitterFilter>, nullptr, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CFLVSourceFilter>, nullptr, &sudFilter[1]},
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	DeleteRegKey(L"Media Type\\Extensions\\", L".flv");

	RegisterSourceFilter(CLSID_AsyncReader, MEDIASUBTYPE_FLV, L"0,4,,464C5601", nullptr);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	UnRegisterSourceFilter(MEDIASUBTYPE_FLV);

	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

//
// CFLVSplitterFilter
//

CFLVSplitterFilter::CFLVSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CFLVSplitterFilter"), pUnk, phr, __uuidof(this))
	, m_DataOffset(0)
	, m_TimeStampOffset(0)
	, m_DetectWrongTimeStamp(true)
{
	m_nFlag |= SOURCE_SUPPORT_URL;
	//memset(&meta, 0, sizeof(meta));
}

STDMETHODIMP CFLVSplitterFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	if (m_pName && m_pName[0]==L'M' && m_pName[1]==L'P' && m_pName[2]==L'C') {
		(void)StringCchCopyW(pInfo->achName, NUMELMS(pInfo->achName), m_pName);
	} else {
		wcscpy_s(pInfo->achName, FlvSourceName);
	}
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

static double int64toDouble(__int64 value)
{
	union
	{
		__int64	i;
		double	f;
	} intfloat64;

	intfloat64.i = value;

	return intfloat64.f;
}

CString CFLVSplitterFilter::AMF0GetString(UINT64 end)
{
	char name[256] = {0};

	SHORT length = m_pFile->BitRead(16);
	if (UINT64(m_pFile->GetPos() + length) > end) {
		return L"";
	}

	if (length > sizeof(name)) {
		m_pFile->Seek(m_pFile->GetPos() + length);
		return L"";
	}

	m_pFile->ByteRead((BYTE*)name, length);

	return CString(name);
}

bool CFLVSplitterFilter::ParseAMF0(UINT64 end, const CString key, std::vector<AMF0> &AMF0Array)
{
	if (UINT64(m_pFile->GetPos()) >= (end - 2)) {
		return false;
	}

	AMF0 amf0;

	AMF_DATA_TYPE amf_type = (AMF_DATA_TYPE)m_pFile->BitRead(8);

	switch (amf_type) {
		case AMF_DATA_TYPE_NUMBER:
			{
				UINT64 value	= m_pFile->BitRead(64);

				amf0.type		= amf_type;
				amf0.name		= key;
				amf0.value_d	= int64toDouble(value);
			}
			break;
		case AMF_DATA_TYPE_BOOL:
			{
				BYTE value		= m_pFile->BitRead(8);

				amf0.type		= amf_type;
				amf0.name		= key;
				amf0.value_b	= !!value;
			}
			break;
		case AMF_DATA_TYPE_STRING:
			{
				amf0.type		= amf_type;
				amf0.name		= key;
				amf0.value_s	= AMF0GetString(end);
			}
			break;
		case AMF_DATA_TYPE_OBJECT:
			{
				if (key == KEYFRAMES_TAG && m_sps.empty()) {
					std::vector<double> times;
					std::vector<double> filepositions;

					for (;;) {
						CString name = AMF0GetString(end);
						if (name.IsEmpty()) {
							break;
						}
						BYTE value = m_pFile->BitRead(8);
						if (value != AMF_DATA_TYPE_ARRAY) {
							break;
						}
						unsigned arraylen = m_pFile->BitRead(32);
						if (arraylen >> 28) {
							break;
						}
						std::vector<double>* array = nullptr;

						if (name == KEYFRAMES_BYTEOFFSET_TAG) {
							array = &filepositions;
						} else if (name == KEYFRAMES_TIMESTAMP_TAG) {
							array = &times;
						} else {
							break;
						}

						for (unsigned i = 0; i < arraylen; i++) {
							BYTE value = m_pFile->BitRead(8);
							if (value != AMF_DATA_TYPE_NUMBER) {
								break;
							}
							array->push_back(int64toDouble(m_pFile->BitRead(64)));
						}
					}

					if (times.size() == filepositions.size()) {
						for (size_t i = 0; i < times.size(); i++) {
							SyncPoint sp = {REFERENCE_TIME(times[i] * UNITS), __int64(filepositions[i])};
							m_sps.push_back(sp);
						}
					}
				}
			}
			break;
		case AMF_DATA_TYPE_NULL:
		case AMF_DATA_TYPE_UNDEFINED:
		case AMF_DATA_TYPE_UNSUPPORTED:
			return true;
		case AMF_DATA_TYPE_MIXEDARRAY:
			{
				m_pFile->BitRead(32);
				for (;;) {
					CString name = AMF0GetString(end);
					if (name.IsEmpty()) {
						return false;
					}
					if (ParseAMF0(end, name, AMF0Array) == false) {
						return false;
					}
				}

				return (m_pFile->BitRead(8) == AMF_END_OF_OBJECT);
			}
		case AMF_DATA_TYPE_ARRAY:
			{
				DWORD arraylen = m_pFile->BitRead(32);
				for (DWORD i = 0; i < arraylen; i++) {
					if (ParseAMF0(end, L"", AMF0Array) == false) {
						return false;
					}
				}
			}
			break; // TODO ...
		case AMF_DATA_TYPE_DATE:
			m_pFile->Seek(m_pFile->GetPos() + 8 + 2);
			return true;
	}

	if (amf0.type != AMF_DATA_TYPE_EMPTY) {
		AMF0Array.push_back(amf0);
	}

	return true;
}

bool CFLVSplitterFilter::ReadTag(Tag& t)
{
	if (!m_pFile->IsStreaming() && m_pFile->GetRemaining() < 15) {
		return false;
	}

	t.PreviousTagSize	= (UINT32)m_pFile->BitRead(32);
	t.TagType			= (BYTE)m_pFile->BitRead(8);
	t.DataSize			= (UINT32)m_pFile->BitRead(24);
	t.TimeStamp			= (UINT32)m_pFile->BitRead(24);
	t.TimeStamp		   |= (UINT32)m_pFile->BitRead(8) << 24;
	t.StreamID			= (UINT32)m_pFile->BitRead(24);

	if (m_DetectWrongTimeStamp && (t.TagType == FLV_AUDIODATA || t.TagType == FLV_VIDEODATA)) {
		if (t.TimeStamp > 0) {
			m_TimeStampOffset = t.TimeStamp;
		}
		m_DetectWrongTimeStamp = false;
	}

	if (m_TimeStampOffset > 0) {
		t.TimeStamp -= m_TimeStampOffset;
		DLog(L"CFLVSplitterFilter::ReadTag() : Detect wrong TimeStamp offset, corrected [%u -> %u]",  (t.TimeStamp + m_TimeStampOffset), t.TimeStamp);
	}

	return !m_pFile->IsStreaming() ? (m_pFile->GetRemaining() >= t.DataSize) : true;
}

bool CFLVSplitterFilter::ReadTag(AudioTag& at)
{
	if (!m_pFile->IsStreaming() && !m_pFile->GetRemaining()) {
		return false;
	}

	at.SoundFormat	= (BYTE)m_pFile->BitRead(4);
	at.SoundRate	= (BYTE)m_pFile->BitRead(2);
	at.SoundSize	= (BYTE)m_pFile->BitRead(1);
	at.SoundType	= (BYTE)m_pFile->BitRead(1);

	return true;
}

bool CFLVSplitterFilter::ReadTag(VideoTag& vt)
{
	if (!m_pFile->IsStreaming() && !m_pFile->GetRemaining()) {
		return false;
	}

	vt.FrameType		= (BYTE)m_pFile->BitRead(4);
	vt.CodecID			= (BYTE)m_pFile->BitRead(4);
	vt.AVCPacketType	= 0;
	vt.tsOffset			= 0;

	if (IsAVCCodec(vt.CodecID)) {
		if (!m_pFile->IsStreaming() && m_pFile->GetRemaining() < 3) {
			return false;
		}

		vt.AVCPacketType	= (BYTE)m_pFile->BitRead(8);
		if (vt.AVCPacketType == 1) {
			vt.tsOffset		= (UINT32)m_pFile->BitRead(24);
			vt.tsOffset		= (vt.tsOffset + 0xff800000) ^ 0xff800000; // sign extension
		} else {
			m_pFile->BitRead(24);
		}
	}

	return true;
}

#ifndef NOVIDEOTWEAK
bool CFLVSplitterFilter::ReadTag(VideoTweak& vt)
{
	vt.x = (BYTE)m_pFile->BitRead(4);
	vt.y = (BYTE)m_pFile->BitRead(4);

	return true;
}
#endif

#define RESYNC_BUFFER_SIZE (1 << 20)
#define AV_RB24(p) ((((BYTE*)(p))[0] << 16) | (((BYTE*)(p))[1] << 8)  | ((BYTE*)(p))[2])
#define AV_RB32(p) ((((BYTE*)(p))[0] << 24) | (((BYTE*)(p))[1] << 16) | (((BYTE*)(p))[2] <<  8) | ((BYTE*)(p))[3])

bool CFLVSplitterFilter::Sync(__int64& pos)
{
	static BYTE resync_buffer[2 * RESYNC_BUFFER_SIZE] = { 0 };

	m_pFile->Seek(pos);
	if (m_pFile->IsURL()) {
		for (UINT32 i = 0; m_pFile->GetRemaining() && SUCCEEDED(m_pFile->GetLastReadError()); i++) {
			const int j  = i & (RESYNC_BUFFER_SIZE - 1);
			const int j1 = j + RESYNC_BUFFER_SIZE;
			resync_buffer[j ] =
			resync_buffer[j1] = m_pFile->BitRead(8);

			if (i > 22) {
				UINT32 lsize2 = AV_RB32(resync_buffer + j1 - 4);
				if (lsize2 >= 11 && lsize2 + 8LL < std::min(i, (UINT32)RESYNC_BUFFER_SIZE)) {
					UINT32  size2 = AV_RB24(resync_buffer + j1 - lsize2 + 1 - 4);
					UINT32 lsize1 = AV_RB32(resync_buffer + j1 - lsize2 - 8);
					if (lsize1 >= 11 && lsize1 + 8LL + lsize2 < std::min(i, (UINT32)RESYNC_BUFFER_SIZE)) {
						UINT32 size1 = AV_RB24(resync_buffer + j1 - lsize1 + 1 - lsize2 - 8);
						if (size1 == lsize1 - 11 && size2  == lsize2 - 11) {
							pos = pos + i - lsize1 - lsize2 - 8 - 4;
							m_pFile->Seek(pos);
							return true;
						}
					}
				}
			}
		}
	} else {
		m_pFile->Seek(pos);
		while (m_pFile->GetRemaining() >= 15 && SUCCEEDED(m_pFile->GetLastReadError())) {
			__int64 limit = m_pFile->GetRemaining();
			while (true) {
				BYTE b = (BYTE)m_pFile->BitRead(8);
				if (IsValidTag(b)) {
					break;
				}
				if (--limit < 15) {
					return false;
				}
			}

			pos = m_pFile->GetPos() - 5;
			m_pFile->Seek(pos);

			Tag ct;
			if (ReadTag(ct) && IsValidTag(ct.TagType)) {
				__int64 next = m_pFile->GetPos() + ct.DataSize;
				if (next == m_pFile->GetAvailable() - 4) {
					m_pFile->Seek(pos);
					return true;
				} else if (next <= m_pFile->GetAvailable() - 19) {
					m_pFile->Seek(next);
					Tag nt;
					if (ReadTag(nt) && IsValidTag(nt.TagType)) {
						if ((nt.PreviousTagSize == ct.DataSize + 11) ||
								(nt.TimeStamp >= ct.TimeStamp)) {
							m_pFile->Seek(pos);
							return true;
						}
					}
				}
			}

			m_pFile->Seek(pos + 5);
		}
	}

	return false;
}

HRESULT CFLVSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;
	m_pFile.Free();

	m_pFile.Attach(DNew CBaseSplitterFileEx(pAsyncReader, hr, FM_FILE | FM_FILE_DL | FM_STREAM));
	if (!m_pFile) {
		return E_OUTOFMEMORY;
	}
	if (FAILED(hr)) {
		m_pFile.Free();
		return hr;
	}
	m_pFile->SetBreakHandle(GetRequestHandle());

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = 0;

	if (m_pFile->BitRead(32) != 'FLV\01') {
		return E_FAIL;
	}

	EXECUTE_ASSERT(m_pFile->BitRead(5) == 0); // TypeFlagsReserved
	bool fTypeFlagsAudio = !!m_pFile->BitRead(1);
	EXECUTE_ASSERT(m_pFile->BitRead(1) == 0); // TypeFlagsReserved
	bool fTypeFlagsVideo = !!m_pFile->BitRead(1);
	m_DataOffset = (UINT32)m_pFile->BitRead(32);

	// doh, these flags aren't always telling the truth
	fTypeFlagsAudio = fTypeFlagsVideo = true;

	Tag t;
	AudioTag at;
	VideoTag vt;

	m_pFile->Seek(m_DataOffset);

	REFERENCE_TIME AvgTimePerFrame	= 0;
	BOOL bAvgTimePerFrameSet		= FALSE;

	REFERENCE_TIME metaDataDuration	= 0;
	DWORD nSamplesPerSec			= 0;
	int   metaHM_compatibility		= 0;

	LONG vWidth   = 0;
	LONG vHeight  = 0;
	BYTE vCodecId = 0;

	m_sps.clear();

	BOOL bVideoMetadataExists = FALSE;
	BOOL bAudioMetadataExists = FALSE;
	BOOL bVideoFound          = FALSE;
	BOOL bAudioFound          = FALSE;

	auto CheckMetadataStreams = [&]() {
		return (bVideoMetadataExists + bAudioMetadataExists) != (bVideoFound + bAudioFound) && m_pFile->GetPos() <= 10 * MEGABYTE;
	};

	CMediaType mt;
	CMediaType mtAAC;

	struct {
		CString title, name;
		CString album;
		CString artist;
		CString comment;
		CString date;
	} metaData;

	// read up to 180 tags (actual maximum was 168)
	UINT i = 0;
	while (((i++ <= 180 && (fTypeFlagsVideo || fTypeFlagsAudio)) || CheckMetadataStreams())
			&& ReadTag(t)
			&& SUCCEEDED(m_pFile->GetLastReadError())) {
		if (!t.DataSize) {
			continue; // skip empty Tag
		}

		UINT64 next = m_pFile->GetPos() + t.DataSize;

		if (!IsValidTag(t.TagType)) {
			m_pFile->Seek(next);
			continue;
		}

		CString name;
		CMediaType mt;

		if (t.TagType == FLV_SCRIPTDATA && t.DataSize) {
			BYTE type = m_pFile->BitRead(8);
			SHORT length = m_pFile->BitRead(16);
			if (type == AMF_DATA_TYPE_STRING && length <= 11) {
				char name[11];
				memset(name, 0, 11);
				m_pFile->ByteRead((BYTE*)name, length);
				if (!strncmp(name, "onTextData", length) || (!strncmp(name, "onMetaData", length))) {
					std::vector<AMF0> AMF0Array;
					ParseAMF0(next, CString(name), AMF0Array);

					for (size_t i = 0; i < AMF0Array.size(); i++) {
						if (AMF0Array[i].type == AMF_DATA_TYPE_NUMBER) {
							if (AMF0Array[i].name == L"width") {
								vWidth = (LONG)((double)AMF0Array[i]);
							} else if (AMF0Array[i].name == L"height") {
								vHeight = (LONG)((double)AMF0Array[i]);
							} else if (AMF0Array[i].name == L"videocodecid") {
								vCodecId = (BYTE)((double)AMF0Array[i]);
								bVideoMetadataExists = TRUE;
							} else if (AMF0Array[i].name == L"audiocodecid") {
								bAudioMetadataExists = TRUE;
							} else if (AMF0Array[i].name == L"duration") {
								metaDataDuration = (REFERENCE_TIME)(UNITS * (double)AMF0Array[i]);
							} else if (AMF0Array[i].name == L"framerate" || AMF0Array[i].name == L"videoframerate") {
								double value = AMF0Array[i];
								if (value > 0 && value < 120) {
									AvgTimePerFrame		= (REFERENCE_TIME)(10000000.0 / value);
									bAvgTimePerFrameSet	= TRUE;
								}
							} else if (AMF0Array[i].name == L"audiosamplerate") {
								double value = AMF0Array[i];
								if (value != 0) {
									nSamplesPerSec = value;
								}
							}
						} else if (AMF0Array[i].type == AMF_DATA_TYPE_STRING) {
							if (AMF0Array[i].name == L"HM compatibility") {
								metaHM_compatibility = (int)(_wtof(AMF0Array[i].value_s) * 10.0);
							} else if (AMF0Array[i].name == L"videocodecid") {
								bVideoMetadataExists = TRUE;
							} else if (AMF0Array[i].name == L"audiocodecid") {
								bAudioMetadataExists = TRUE;
							} else if (AMF0Array[i].name.CompareNoCase(L"title") == 0) {
								metaData.title = AMF0Array[i];
							} else if (AMF0Array[i].name.CompareNoCase(L"name") == 0) {
								metaData.name = AMF0Array[i];
							} else if (AMF0Array[i].name.CompareNoCase(L"album") == 0) {
								metaData.album = AMF0Array[i];
							} else if (AMF0Array[i].name.CompareNoCase(L"artist") == 0) {
								metaData.artist = AMF0Array[i];
							} else if (AMF0Array[i].name.CompareNoCase(L"comment") == 0) {
								metaData.comment = AMF0Array[i];
							} else if (AMF0Array[i].name.CompareNoCase(L"date") == 0) {
								metaData.date = AMF0Array[i];
							}
						} else if (AMF0Array[i].type == AMF_DATA_TYPE_BOOL) {
							if (AMF0Array[i].name == L"hasVideo") {
								bVideoMetadataExists = (bool)AMF0Array[i];
							} else if (AMF0Array[i].name == L"hasAudio") {
								bAudioMetadataExists = (bool)AMF0Array[i];
							}
						}
					}
				}
			}
		} else if (t.TagType == FLV_AUDIODATA && t.DataSize != 0 && fTypeFlagsAudio) {
			UNREFERENCED_PARAMETER(at);
			AudioTag at;
			name = L"Audio";

			if (ReadTag(at)) {
				int dataSize = t.DataSize - 1;

				if (bAudioMetadataExists) {
					bAudioFound = TRUE;
				}

				fTypeFlagsAudio = false;

				mt.majortype			= MEDIATYPE_Audio;
				mt.formattype			= FORMAT_WaveFormatEx;
				WAVEFORMATEX* wfe		= (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX));
				memset(wfe, 0, sizeof(WAVEFORMATEX));
				wfe->nSamplesPerSec		= nSamplesPerSec ? nSamplesPerSec : 44100*(1<<at.SoundRate)/8;
				wfe->wBitsPerSample		= 8 * (at.SoundSize + 1);
				wfe->nChannels			= at.SoundType + 1;

				switch (at.SoundFormat) {
					case FLV_AUDIO_PCM:
					case FLV_AUDIO_PCMLE:
						mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_PCM);
						name += L" PCM";
						break;
					case FLV_AUDIO_ADPCM:
						mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_ADPCM_SWF);
						name += L" ADPCM";
						break;
					case FLV_AUDIO_MP3:
						mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_MPEGLAYER3);
						name += L" MP3";
						{
							CBaseSplitterFileEx::mpahdr h;
							CMediaType mt2;
							if (m_pFile->Read(h, 4, &mt2)) {
								mt = mt2;
							}
						}
						break;
					case FLV_AUDIO_NELLY16:
						mt.subtype = FOURCCMap(MAKEFOURCC('N','E','L','L'));
						wfe->nSamplesPerSec = 16000;
						name += L" Nellimoser";
						break;
					case FLV_AUDIO_NELLY8:
						mt.subtype = FOURCCMap(MAKEFOURCC('N','E','L','L'));
						wfe->nSamplesPerSec = 8000;
						name += L" Nellimoser";
						break;
					case FLV_AUDIO_NELLY:
						mt.subtype = FOURCCMap(MAKEFOURCC('N','E','L','L'));
						name += L" Nellimoser";
						break;
					case FLV_AUDIO_SPEEX:
						mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_SPEEX);
						wfe->nSamplesPerSec = 16000;
						name += L" Speex";
						break;
					case FLV_AUDIO_AAC: {
						if (dataSize < 1 || m_pFile->BitRead(8) != 0) { // packet type 0 == aac header
							fTypeFlagsAudio = true;
							break;
						}

						FreeMediaType(mtAAC);
						mtAAC.InitMediaType();
						mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_RAW_AAC1);
						name += L" AAC";

						__int64 configOffset = m_pFile->GetPos();
						UINT32 configSize = dataSize - 1;
						if (configSize < 2) {
							break;
						}

						// Might break depending on the AAC profile, see ff_mpeg4audio_get_config in ffmpeg's mpeg4audio.c
						m_pFile->BitRead(5);
						int nSampleRate = (int)m_pFile->BitRead(4);
						int nChannels   = (int)m_pFile->BitRead(4);
						if (nSampleRate > 12 || nChannels > 7) {
							break;
						}

						static const int sampleRates[] = { 96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0, 0, 0 };
						static const int channels[] = { 0, 1, 2, 3, 4, 5, 6, 8 };

						wfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX) + configSize);
						memset(wfe, 0, mt.FormatLength());
						wfe->wFormatTag     = WAVE_FORMAT_RAW_AAC1;
						wfe->nSamplesPerSec = sampleRates[nSampleRate];
						wfe->wBitsPerSample = 16;
						wfe->nChannels      = channels[nChannels];
						wfe->cbSize         = configSize;

						m_pFile->Seek(configOffset);
						m_pFile->ByteRead((BYTE*)(wfe+1), configSize);
					}
				}
				wfe->nBlockAlign     = wfe->nChannels * wfe->wBitsPerSample / 8;
				wfe->nAvgBytesPerSec = wfe->nSamplesPerSec * wfe->nBlockAlign;

				mt.SetSampleSize(wfe->wBitsPerSample * wfe->nChannels / 8);

				if (at.SoundFormat == FLV_AUDIO_AAC && mt.subtype == GUID_NULL && mtAAC.subtype == GUID_NULL) {
					mtAAC = mt;
					WAVEFORMATEX* wfeAAC = (WAVEFORMATEX*)mtAAC.pbFormat;
					mtAAC.subtype = FOURCCMap(wfeAAC->wFormatTag = WAVE_FORMAT_RAW_AAC1);
				}
			}
		} else if (t.TagType == FLV_VIDEODATA && t.DataSize != 0 && fTypeFlagsVideo) {
			UNREFERENCED_PARAMETER(vt);
			VideoTag vt;
			if (ReadTag(vt) && vt.FrameType == 1) {
				int dataSize = t.DataSize - 1;

				if (bVideoMetadataExists) {
					bVideoFound = TRUE;
				}

				fTypeFlagsVideo = false;
				name = L"Video";

				mt.majortype = MEDIATYPE_Video;
				mt.formattype = FORMAT_VideoInfo;
				VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
				memset(vih, 0, sizeof(VIDEOINFOHEADER));

				BITMAPINFOHEADER* bih = &vih->bmiHeader;

				vih->AvgTimePerFrame = AvgTimePerFrame;

				switch (vt.CodecID) {
					case FLV_VIDEO_H263:   // H.263
						if (m_pFile->BitRead(17) != 1) {
							break;
						}

						m_pFile->BitRead(13); // Version (5), TemporalReference (8)

						switch (BYTE PictureSize = (BYTE)m_pFile->BitRead(3)) { // w00t
							case 0:
							case 1:
								vih->bmiHeader.biWidth = (WORD)m_pFile->BitRead(8*(PictureSize+1));
								vih->bmiHeader.biHeight = (WORD)m_pFile->BitRead(8*(PictureSize+1));
								break;
							case 2:
							case 3:
							case 4:
								vih->bmiHeader.biWidth = 704 / PictureSize;
								vih->bmiHeader.biHeight = 576 / PictureSize;
								break;
							case 5:
							case 6:
								PictureSize -= 3;
								vih->bmiHeader.biWidth = 640 / PictureSize;
								vih->bmiHeader.biHeight = 480 / PictureSize;
								break;
						}

						if (!vih->bmiHeader.biWidth || !vih->bmiHeader.biHeight) {
							break;
						}

						mt.subtype = FOURCCMap(vih->bmiHeader.biCompression = '1VLF');
						name += L" H.263";

						break;
					case FLV_VIDEO_SCREEN: {
						m_pFile->BitRead(4);
						vih->bmiHeader.biWidth  = (LONG)m_pFile->BitRead(12);
						m_pFile->BitRead(4);
						vih->bmiHeader.biHeight = (LONG)m_pFile->BitRead(12);

						if (!vih->bmiHeader.biWidth || !vih->bmiHeader.biHeight) {
							break;
						}

						vih->bmiHeader.biSize = sizeof(vih->bmiHeader);
						vih->bmiHeader.biPlanes = 1;
						vih->bmiHeader.biBitCount = 24;
						vih->bmiHeader.biSizeImage = DIBSIZE(vih->bmiHeader);

						mt.subtype = FOURCCMap(vih->bmiHeader.biCompression = '1VSF');
						name += L" Screen";

						break;
					}
					case FLV_VIDEO_VP6A:  // VP6 with alpha
						m_pFile->BitRead(24);
					case FLV_VIDEO_VP6: { // VP6
						int w, h, arx, ary;
#ifdef NOVIDEOTWEAK
						m_pFile->BitRead(8);
#else
						VideoTweak fudge;
						ReadTag(fudge);
#endif

						if (m_pFile->BitRead(1)) {
							// Delta (inter) frame
							fTypeFlagsVideo = true;
							break;
						}
						m_pFile->BitRead(6);
						bool fSeparatedCoeff = !!m_pFile->BitRead(1);
						m_pFile->BitRead(5);
						int filterHeader = (int)m_pFile->BitRead(2);
						m_pFile->BitRead(1);
						if (fSeparatedCoeff || !filterHeader) {
							m_pFile->BitRead(16);
						}

						h = (int)m_pFile->BitRead(8) * 16;
						w = (int)m_pFile->BitRead(8) * 16;

						ary = (int)m_pFile->BitRead(8) * 16;
						arx = (int)m_pFile->BitRead(8) * 16;

						if (arx && arx != w || ary && ary != h) {
							VIDEOINFOHEADER2* vih2		= (VIDEOINFOHEADER2*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER2));
							memset(vih2, 0, sizeof(VIDEOINFOHEADER2));
							vih2->dwPictAspectRatioX	= arx;
							vih2->dwPictAspectRatioY	= ary;
							vih2->AvgTimePerFrame		= AvgTimePerFrame;
							bih = &vih2->bmiHeader;
							mt.formattype = FORMAT_VideoInfo2;
							vih = (VIDEOINFOHEADER *)vih2;
						}

						bih->biWidth = w;
						bih->biHeight = h;
#ifndef NOVIDEOTWEAK
						SetRect(&vih->rcSource, 0, 0, w - fudge.x, h - fudge.y);
						SetRect(&vih->rcTarget, 0, 0, w - fudge.x, h - fudge.y);
#endif

						mt.subtype = FOURCCMap(bih->biCompression = '4VLF');
						name += L" VP6";

						break;
					}
					case FLV_VIDEO_AVC: { // H.264
						if (dataSize < 4 || vt.AVCPacketType != 0) {
							fTypeFlagsVideo = true;
							break;
						}

						__int64 headerOffset = m_pFile->GetPos();
						UINT32 headerSize = dataSize - 4;

						BOOL bIsAVC = FALSE;
						if (headerSize > 9) {
							BYTE *headerData = DNew BYTE[headerSize];
							m_pFile->ByteRead(headerData, headerSize);

							vc_params_t params;
							if (AVCParser::ParseSequenceParameterSet(headerData + 9, headerSize - 9, params)) {
								BITMAPINFOHEADER pbmi;
								memset(&pbmi, 0, sizeof(BITMAPINFOHEADER));
								pbmi.biSize			= sizeof(pbmi);
								pbmi.biWidth		= params.width;
								pbmi.biHeight		= params.height;
								pbmi.biCompression	= FCC('AVC1');
								pbmi.biPlanes		= 1;
								pbmi.biBitCount		= 24;
								pbmi.biSizeImage	= DIBSIZE(pbmi);

								if (!bAvgTimePerFrameSet && params.AvgTimePerFrame) {
									AvgTimePerFrame = params.AvgTimePerFrame;
									vih->AvgTimePerFrame = AvgTimePerFrame;
								}

								CSize aspect(params.width * params.sar.num, params.height * params.sar.den);
								ReduceDim(aspect);
								CreateMPEG2VIfromAVC(&mt, &pbmi, AvgTimePerFrame, aspect, headerData, headerSize);

								bIsAVC = TRUE;
							}

							delete [] headerData;
						}

						if (!bIsAVC && vCodecId == vt.CodecID) {
							bih->biSize			= sizeof(vih->bmiHeader);
							bih->biWidth		= vWidth;
							bih->biHeight		= vHeight;
							bih->biPlanes		= 1;
							bih->biBitCount		= 24;
							bih->biSizeImage	= DIBSIZE(*bih);

							mt.subtype			= FOURCCMap(bih->biCompression = FCC('H264'));
						}

						name += L" H.264";

						break;
					}
					case FLV_VIDEO_HM62: { // HEVC HM6.2
						// Source code is provided by Deng James from Strongene Ltd.
						// check is avc header
						if (dataSize < 4 || vt.AVCPacketType != 0) {
							fTypeFlagsVideo = true;
							break;
						}

						__int64 headerOffset = m_pFile->GetPos();
						UINT32 headerSize = dataSize - 4;
						BYTE * headerData = DNew BYTE[headerSize];
						m_pFile->ByteRead(headerData, headerSize);
						m_pFile->Seek(headerOffset + 9);

						mt.formattype					= FORMAT_MPEG2Video;
						MPEG2VIDEOINFO* vih				= (MPEG2VIDEOINFO*)mt.AllocFormatBuffer(FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader) + headerSize);
						memset(vih, 0, mt.FormatLength());
						vih->hdr.bmiHeader.biSize		= sizeof(vih->hdr.bmiHeader);
						vih->hdr.bmiHeader.biPlanes		= 1;
						vih->hdr.bmiHeader.biBitCount	= 24;
						vih->dwFlags					= (headerData[4] & 0x03) + 1; // nal length size

						vih->dwProfile					= (BYTE)m_pFile->BitRead(8); // profile
						m_pFile->BitRead(8); // compatibility
						vih->dwLevel					= (BYTE)m_pFile->BitRead(8); // level

						// parse SPS
						UINT ue;
						ue = (UINT)m_pFile->UExpGolombRead(); // seq_parameter_set_id
						ue = (UINT)m_pFile->UExpGolombRead(); // ???
						ue = (UINT)m_pFile->BitRead(3); // max_tid_minus_1

						UINT nWidth  = (UINT)m_pFile->UExpGolombRead(); // video width
						UINT nHeight = (UINT)m_pFile->UExpGolombRead(); // video height

						INT bit = (INT)m_pFile->BitRead(1);
						if(bit == 1) {
							ue = (UINT)m_pFile->UExpGolombRead();
							ue = (UINT)m_pFile->UExpGolombRead();
							ue = (UINT)m_pFile->UExpGolombRead();
							ue = (UINT)m_pFile->UExpGolombRead();
						}
						ue = (UINT)m_pFile->UExpGolombRead();
						ue = (UINT)m_pFile->UExpGolombRead();

						bit = (INT)m_pFile->BitRead(1);
						bit = (INT)m_pFile->BitRead(1);

						ue = (UINT)m_pFile->UExpGolombRead(); // log2_poc_minus_4

						// Fill media type
						CSize aspect(nWidth, nHeight);
						ReduceDim(aspect);

						vih->hdr.bmiHeader.biWidth		= nWidth;
						vih->hdr.bmiHeader.biHeight		= nHeight;
						vih->hdr.bmiHeader.biSizeImage	= DIBSIZE(vih->hdr.bmiHeader);
						vih->hdr.dwPictAspectRatioX		= aspect.cx;
						vih->hdr.dwPictAspectRatioY		= aspect.cy;
						vih->hdr.AvgTimePerFrame		= AvgTimePerFrame;

						HEVCParser::CreateSequenceHeaderAVC(headerData, headerSize, vih->dwSequenceHeader, vih->cbSequenceHeader);

						delete [] headerData;

						mt.subtype = FOURCCMap(vih->hdr.bmiHeader.biCompression = 'CVEH');
						break;
					}
					case FLV_VIDEO_HM91:   // HEVC HM9.1
					case FLV_VIDEO_HM10:   // HEVC HM10.0
					case FLV_VIDEO_HEVC: { // HEVC HM11.0 & HM12.0 ...
						if (dataSize < 4 || vt.AVCPacketType != 0) {
							fTypeFlagsVideo = true;
							break;
						}

						__int64 headerOffset = m_pFile->GetPos();
						UINT32 headerSize = dataSize - 4;
						BYTE* headerData = DNew BYTE[headerSize]; // this is AVCDecoderConfigurationRecord struct

						m_pFile->ByteRead(headerData, headerSize);

						DWORD fourcc = MAKEFOURCC('H','E','V','C');
						switch (vt.CodecID) {
							case FLV_VIDEO_HM91:
								fourcc = MAKEFOURCC('H','M','9','1');
								metaHM_compatibility = 91;
								break;
							case FLV_VIDEO_HM10:
								fourcc = MAKEFOURCC('H','M','1','0');
								metaHM_compatibility = 100;
								break;
							case FLV_VIDEO_HEVC:
								if (metaHM_compatibility >= 90 && metaHM_compatibility < 100) {
									fourcc = MAKEFOURCC('H','M','9','1');
								} else if (metaHM_compatibility >= 100 && metaHM_compatibility < 110) {
									fourcc = MAKEFOURCC('H','M','1','0');
								} else if (metaHM_compatibility >= 110 && metaHM_compatibility < 120) {
									fourcc = MAKEFOURCC('H','M','1','1');
								} else if (metaHM_compatibility >= 120 && metaHM_compatibility < 130) {
									fourcc = MAKEFOURCC('H','M','1','2');
								}
								break;
						}

						vc_params_t params;
						if (!HEVCParser::ParseAVCDecoderConfigurationRecord(headerData, headerSize, params, metaHM_compatibility)) {
							fTypeFlagsVideo = true;
							break;
						}

						CSize aspect(params.width, params.height);
						ReduceDim(aspect);

						// format type
						mt.formattype					= FORMAT_MPEG2Video;
						MPEG2VIDEOINFO* vih				= (MPEG2VIDEOINFO*)mt.AllocFormatBuffer(FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader) + headerSize);
						memset(vih, 0, mt.FormatLength());
						vih->hdr.bmiHeader.biSize		= sizeof(vih->hdr.bmiHeader);
						vih->hdr.bmiHeader.biPlanes		= 1;
						vih->hdr.bmiHeader.biBitCount	= 24;
						vih->hdr.bmiHeader.biWidth		= params.width;
						vih->hdr.bmiHeader.biHeight		= params.height;
						vih->hdr.bmiHeader.biSizeImage	= DIBSIZE(vih->hdr.bmiHeader);
						vih->hdr.dwPictAspectRatioX		= aspect.cx;
						vih->hdr.dwPictAspectRatioY		= aspect.cy;
						vih->hdr.AvgTimePerFrame		= AvgTimePerFrame;
						vih->dwFlags					= params.nal_length_size;
						vih->dwProfile					= params.profile;
						vih->dwLevel					= params.level;

						HEVCParser::CreateSequenceHeaderAVC(headerData, headerSize, vih->dwSequenceHeader, vih->cbSequenceHeader);
						delete [] headerData;

						mt.subtype = FOURCCMap(vih->hdr.bmiHeader.biCompression = fourcc);

						name += L" HEVC";
						if (vt.CodecID == FLV_VIDEO_HM91) {
							name += L" HM9.1";
						} else if (vt.CodecID == FLV_VIDEO_HM10) {
							name += L" HM10.0";
						}
						break;
					}
					default:
						fTypeFlagsVideo = true;
				}

				if (mt.subtype != GUID_NULL) {
					REFERENCE_TIME rtAvgTimePerFrame = 1;
					ExtractAvgTimePerFrame(&mt, rtAvgTimePerFrame);

					// calculate video fps
					if (rtAvgTimePerFrame < 50000) {
						const __int64 pos = m_pFile->GetPos();
						__int64 sync_pos = m_DataOffset;
						if (Sync(sync_pos)) {
							std::vector<REFERENCE_TIME> timecodes;
							timecodes.reserve(FrameDuration::DefaultFrameNum);

							Tag tag;
							VideoTag vtag;

							while (ReadTag(tag) && !CheckRequest(nullptr) && m_pFile->GetRemaining()) {
								__int64 _next = m_pFile->GetPos() + tag.DataSize;
								if ((tag.DataSize > 0) && (tag.TagType == FLV_VIDEODATA && ReadTag(vtag) && tag.TimeStamp > 0)) {
									if (IsAVCCodec(vtag.CodecID)) {
										if (vtag.AVCPacketType != 1) {
											continue;
										}
										tag.TimeStamp += vtag.tsOffset;
									}

									timecodes.push_back(tag.TimeStamp);
									if (timecodes.size() >= FrameDuration::DefaultFrameNum) {
										break;
									}
								}

								m_pFile->Seek(_next);
								if (!Sync(_next)) {
									break;
								}
							}

							rtAvgTimePerFrame = FrameDuration::Calculate(timecodes, 10000);

							if (mt.formattype == FORMAT_MPEG2_VIDEO) {
								((MPEG2VIDEOINFO*)mt.pbFormat)->hdr.AvgTimePerFrame = rtAvgTimePerFrame;
							} else if (mt.formattype == FORMAT_MPEGVideo) {
								((MPEG1VIDEOINFO*)mt.pbFormat)->hdr.AvgTimePerFrame = rtAvgTimePerFrame;
							} else if (mt.formattype == FORMAT_VIDEOINFO2) {
								((VIDEOINFOHEADER2*)mt.pbFormat)->AvgTimePerFrame   = rtAvgTimePerFrame;
							} else if (mt.formattype == FORMAT_VideoInfo) {
								((VIDEOINFOHEADER*)mt.pbFormat)->AvgTimePerFrame    = rtAvgTimePerFrame;
							}
						}
						m_pFile->Seek(pos);
					}
				}
			}
		}

		if (mt.subtype != GUID_NULL) {
			std::vector<CMediaType> mts;
			mts.push_back(mt);
			CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CBaseSplitterOutputPin(mts, name, this, this, &hr));
			EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(t.TagType, pPinOut)));
		}

		m_pFile->Seek(next);
	}

	if (mtAAC.subtype != GUID_NULL) {
		std::vector<CMediaType> mts;
		mts.push_back(mtAAC);
		CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CBaseSplitterOutputPin(mts, L"Audio AAC", this, this, &hr));
		EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(FLV_AUDIODATA, pPinOut)));
	}

	CString title;
	if (!metaData.title.IsEmpty()) {
		title = metaData.title;
	} else if (!metaData.name.IsEmpty()) {
		title = metaData.name;
	}
	if (!metaData.date.IsEmpty() && !title.IsEmpty()) {
		title += L" (" + metaData.date + L")";
	}
	if (!title.IsEmpty()) {
		SetProperty(L"TITL", title);
	}

	if (!metaData.album.IsEmpty()){
		SetProperty(L"ALBUM", metaData.album);
	}
	if (!metaData.artist.IsEmpty()){
		SetProperty(L"AUTH", metaData.artist);
	}
	if (!metaData.comment.IsEmpty()){
		SetProperty(L"DESC", metaData.comment);
	}

	m_rtDuration = metaDataDuration;
	m_rtNewStop = m_rtStop = m_rtDuration;

	m_pFile->Seek(m_DataOffset);

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

bool CFLVSplitterFilter::DemuxInit()
{
	SetThreadName((DWORD)-1, "CFLVSplitterFilter");

	if (m_pFile->IsRandomAccess()) {
		__int64 pos = std::max((__int64)m_DataOffset, m_pFile->GetAvailable() - 256 * KILOBYTE);

		if (Sync(pos)) {
			Tag t;
			AudioTag at;
			VideoTag vt;

			while (ReadTag(t) && m_pFile->GetRemaining()) {
				UINT64 next = m_pFile->GetPos() + t.DataSize;

				CBaseSplitterOutputPin* pOutPin = dynamic_cast<CBaseSplitterOutputPin*>(GetOutputPin(t.TagType));
				if (!pOutPin) {
					m_pFile->Seek(next);
					continue;
				}

				if ((t.TagType == FLV_AUDIODATA && ReadTag(at)) || (t.TagType == FLV_VIDEODATA && ReadTag(vt))) {
					m_rtDuration = std::max(m_rtDuration, 10000i64 * t.TimeStamp + pOutPin->GetOffset());
				}

				m_pFile->Seek(next);
			}
		}

		m_pFile->Seek(m_DataOffset);
	}

	return true;
}

void CFLVSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	if (m_pFile->IsStreaming() || m_rtDuration <= 0) {
		return;
	}

	if (rt <= 0) {
		m_pFile->Seek(m_DataOffset);
		return;
	}

	const BYTE masterTagType = GetOutputPin(FLV_VIDEODATA) ? FLV_VIDEODATA : FLV_AUDIODATA;

	__int64 estimPos = 0;

	if (m_sps.size() > 1 && rt <= m_sps.back().rt) {
		const int i = range_bsearch(m_sps, rt);
		if (i >= 0) {
			estimPos = m_sps[i].fp - 4;
			m_pFile->Seek(estimPos);
			return;
		}
	}

	estimPos = m_DataOffset + (__int64)(double(m_pFile->GetLength() - m_DataOffset) * rt / m_rtDuration);

	if (estimPos > m_pFile->GetAvailable()) {
		return;
	}

	Tag t;
	if (!Sync(estimPos) || !ReadTag(t)) {
		m_pFile->Seek(m_DataOffset);
		return;
	}

	if (t.TagType == masterTagType) {
		if (10000i64 * t.TimeStamp == rt) {
			m_pFile->Seek(m_pFile->GetPos() - 15);
			return;
		}
	}

	while (true) {
		__int64 bestPos  = 0;
		while (ReadTag(t) && t.TimeStamp * 10000i64 <= rt) {
			const __int64 cur  = m_pFile->GetPos() - 15;
			const __int64 next = m_pFile->GetPos() + t.DataSize;

			if (t.TagType == masterTagType) {
				AudioTag at;
				VideoTag vt;
				if ((t.TagType == FLV_AUDIODATA && ReadTag(at))
						|| (t.TagType == FLV_VIDEODATA && ReadTag(vt) && vt.FrameType == 1)) {
					bestPos = cur;
				}
			}

			m_pFile->Seek(next);
		}

		if (bestPos) {
			m_pFile->Seek(bestPos);
			return;
		}

		if (estimPos == m_DataOffset) {
			m_pFile->Seek(m_DataOffset);
			return;
		}

		estimPos -= MEGABYTE;
		if (estimPos < m_DataOffset) {
			estimPos = m_DataOffset;
		}

		bestPos = estimPos;
		if (!Sync(bestPos)) {
			m_pFile->Seek(m_DataOffset);
			return;
		}
	}
}

bool CFLVSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	CAutoPtr<CPacket> p;

	Tag t;
	AudioTag at;
	VideoTag vt;

	while (SUCCEEDED(hr) && !CheckRequest(nullptr)) {
		if (!m_pFile->IsStreaming() && !m_pFile->GetRemaining()) {
			break;
		}

		if (!ReadTag(t)) {
			break;
		}

		if (!ValidateTag(t)) {
			__int64 pos = m_pFile->GetPos();
			if (!Sync(pos)) {
				break;
			}
			continue;
		}

		__int64 next = m_pFile->GetPos() + t.DataSize;

		if ((t.DataSize > 0) && (t.TagType == FLV_AUDIODATA && ReadTag(at) || t.TagType == FLV_VIDEODATA && ReadTag(vt))) {
			if (t.TagType == FLV_VIDEODATA) {
				if (vt.FrameType == 5) {
					goto NextTag;    // video info/command frame
				}
				if (vt.CodecID == FLV_VIDEO_VP6) {
					m_pFile->BitRead(8);
				} else if (vt.CodecID == FLV_VIDEO_VP6A) {
					m_pFile->BitRead(32);
				} else if (IsAVCCodec(vt.CodecID)) {
					if (vt.AVCPacketType != 1) {
						goto NextTag;
					}

					t.TimeStamp += vt.tsOffset;
				}
			}
			if (t.TagType == FLV_AUDIODATA && at.SoundFormat == FLV_AUDIO_AAC) {
				if (m_pFile->BitRead(8) != 1) {
					goto NextTag;
				}
			}
			__int64 dataSize = next - m_pFile->GetPos();
			if (dataSize <= 0) {
				goto NextTag;
			}

			p.Attach(DNew CPacket());
			p->TrackNumber	= t.TagType;
			p->rtStart		= 10000i64 * t.TimeStamp;
			p->rtStop		= p->rtStart + 1;
			p->bSyncPoint	= t.TagType == FLV_VIDEODATA ? vt.FrameType == 1 : true;

			p->SetCount((size_t)dataSize);
			if (S_OK != (hr = m_pFile->ByteRead(p->GetData(), p->GetCount()))) {
				return true;
			}

			hr = DeliverPacket(p);
		}
NextTag:
		m_pFile->Seek(next);
	}

	return true;
}

// IKeyFrameInfo

STDMETHODIMP CFLVSplitterFilter::GetKeyFrameCount(UINT& nKFs)
{
	CheckPointer(m_pFile, E_UNEXPECTED);
	nKFs = m_sps.size();
	return S_OK;
}

STDMETHODIMP CFLVSplitterFilter::GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs)
{
	CheckPointer(pFormat, E_POINTER);
	CheckPointer(pKFs, E_POINTER);
	CheckPointer(m_pFile, E_UNEXPECTED);

	if (*pFormat != TIME_FORMAT_MEDIA_TIME) {
		return E_INVALIDARG;
	}

	for (nKFs = 0; nKFs < m_sps.size(); nKFs++) {
		pKFs[nKFs] = m_sps[nKFs].rt;
	}

	return S_OK;
}

//
// CFLVSourceFilter
//

CFLVSourceFilter::CFLVSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CFLVSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.Free();
}

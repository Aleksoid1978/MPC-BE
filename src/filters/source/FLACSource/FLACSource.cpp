/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2014 see Authors.txt
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
#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include <uuids.h>
#include <moreuuids.h>
#include "FLACSource.h"
#include "../../../DSUtil/DSUtil.h"
#include "../../../DSUtil/CUE.h"
#include "../../../DSUtil/ID3Tag.h"
#include <libflac/src/libflac/include/protected/stream_decoder.h>

#define FLAC_DECODER (FLAC__StreamDecoder*)m_pDecoder

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] = {
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_FLAC_FRAMED}
};

const AMOVIESETUP_PIN sudOpPin[] = {
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesOut), sudPinTypesOut}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CFLACSource), FlacSourceName, MERIT_NORMAL, _countof(sudOpPin), sudOpPin, CLSID_LegacyAmFilterCategory}
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CFLACSource>, NULL, &sudFilter[0]}
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	SetRegKeyValue(
		_T("Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}"), _T("{1930D8FF-4739-4e42-9199-3B2EDEAA3BF2}"),
		_T("0"), _T("0,4,,664C6143"));

	SetRegKeyValue(
		_T("Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}"), _T("{1930D8FF-4739-4e42-9199-3B2EDEAA3BF2}"),
		_T("Source Filter"), _T("{1930D8FF-4739-4e42-9199-3B2EDEAA3BF2}"));

	SetRegKeyValue(
		_T("Media Type\\Extensions"), _T(".flac"),
		_T("Source Filter"), _T("{1930D8FF-4739-4e42-9199-3B2EDEAA3BF2}"));

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	DeleteRegKey(_T("Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}"), _T("{1930D8FF-4739-4e42-9199-3B2EDEAA3BF2}"));
	DeleteRegKey(_T("Media Type\\Extensions"), _T(".flac"));

	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif


// Declaration for FLAC callbacks
static FLAC__StreamDecoderReadStatus	StreamDecoderRead(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data);
static FLAC__StreamDecoderSeekStatus	StreamDecoderSeek(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data);
static FLAC__StreamDecoderTellStatus	StreamDecoderTell(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data);
static FLAC__StreamDecoderLengthStatus	StreamDecoderLength(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data);
static FLAC__bool						StreamDecoderEof(const FLAC__StreamDecoder *decoder, void *client_data);
static FLAC__StreamDecoderWriteStatus	StreamDecoderWrite(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
static void								StreamDecoderError(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);
static void								StreamDecoderMetadata(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);


//
// CFLACSource
//

CFLACSource::CFLACSource(LPUNKNOWN lpunk, HRESULT* phr)
	: CBaseSource<CFLACStream>(NAME("CFLACSource"), lpunk, phr, __uuidof(this))
{
}

CFLACSource::~CFLACSource()
{
}

STDMETHODIMP CFLACSource::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		QI2(IAMMediaContent)
		QI(IPropertyBag)
		QI(IPropertyBag2)
		QI(IDSMResourceBag)
		QI(IDSMChapterBag)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IAMMediaContent

STDMETHODIMP CFLACSource::get_AuthorName(BSTR* pbstrAuthorName)
{
	return GetProperty(L"AUTH", pbstrAuthorName);
}

STDMETHODIMP CFLACSource::get_Title(BSTR* pbstrTitle)
{
	return GetProperty(L"TITL", pbstrTitle);
}

STDMETHODIMP CFLACSource::get_Description(BSTR* pbstrDescription)
{
	return GetProperty(L"DESC", pbstrDescription);
}

STDMETHODIMP CFLACSource::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));
	wcscpy_s(pInfo->achName, FlacSourceName);
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

// CFLACStream

CFLACStream::CFLACStream(const WCHAR* wfn, CSource* pParent, HRESULT* phr)
	: CBaseStream(NAME("CFLACStream"), pParent, phr)
	, m_bIsEOF(false)
	, m_pDecoder(NULL)
{
	CAutoLock		cAutoLock(&m_cSharedState);
	CString			fn(wfn);
	CFileException	ex;
	HRESULT			hr = E_FAIL;

	file_info.got_vorbis_comments = false;

	do {
		if (!m_file.Open(fn, CFile::modeRead | CFile::shareDenyNone, &ex)) {
			hr = AmHresultFromWin32 (ex.m_lOsError);
			break;
		}

		m_pDecoder = FLAC__stream_decoder_new();
		if (!m_pDecoder) {
			break;
		}

		FLAC__stream_decoder_set_metadata_respond(FLAC_DECODER, FLAC__METADATA_TYPE_VORBIS_COMMENT);
		FLAC__stream_decoder_set_metadata_respond(FLAC_DECODER, FLAC__METADATA_TYPE_PICTURE);
		FLAC__stream_decoder_set_metadata_respond(FLAC_DECODER, FLAC__METADATA_TYPE_CUESHEET);

		if (FLAC__STREAM_DECODER_INIT_STATUS_OK != FLAC__stream_decoder_init_stream (FLAC_DECODER,
				StreamDecoderRead,
				StreamDecoderSeek,
				StreamDecoderTell,
				StreamDecoderLength,
				StreamDecoderEof,
				StreamDecoderWrite,
				StreamDecoderMetadata,
				StreamDecoderError,
				this)) {
			break;
		}

		if (!FLAC__stream_decoder_process_until_end_of_metadata(FLAC_DECODER)) {
			break;
		}
		
		FLAC__uint64 metadataend = 0;
		FLAC__stream_decoder_get_decode_position(FLAC_DECODER, &metadataend);
		
		if (!FLAC__stream_decoder_seek_absolute(FLAC_DECODER, 0)) {
			break;
		}

		FLAC__stream_decoder_get_decode_position(FLAC_DECODER, &m_llOffset);

		CID3Tag* pID3Tag = NULL;

		ULONGLONG pos = m_file.GetPosition();
		m_file.Seek(0, CFile::begin);
		DWORD id = 0;
		if ((3 == m_file.Read(&id, 3)) && (id == FCC('ID3\0'))) {
			BYTE major = 0;
			m_file.Read(&major, sizeof(major));
			BYTE revision = 0;
			m_file.Read(&revision, sizeof(revision));
			UNREFERENCED_PARAMETER(revision);
			BYTE flags = 0;
			m_file.Read(&flags, sizeof(flags));

			DWORD size;
			m_file.Read(&size, sizeof(size));
			size = FCC(size);
			size = (((size & 0x7F000000) >> 0x03) |
					((size & 0x007F0000) >> 0x02) |
					((size & 0x00007F00) >> 0x01) |
					((size & 0x0000007F)		));

			if (major <= 4) {
				BYTE* buf = DNew BYTE[size];
				m_file.Read(buf, size);

				pID3Tag = DNew CID3Tag(major, flags);
				pID3Tag->ReadTagsV2(buf, size);
				delete [] buf;
			} else {
				m_file.Seek(size, CFile::current);
			}
		} else {
			m_file.Seek(0, CFile::begin);
		}

		if (metadataend) {
			ULONGLONG startpos = m_file.GetPosition();
			metadataend -= startpos;
			char* data = DNew char[metadataend];
			m_file.Read(data, metadataend);
			char* end = data + metadataend;
			char* begin = strstr(data, "fLaC");
			if (begin) {
				size_t size = end - begin;
				m_extradata.SetCount(size);
				memcpy(m_extradata.GetData(), begin, size);
			}
			delete [] data;
		}

		m_file.Seek(pos, CFile::begin);

		if (file_info.got_vorbis_comments) {
			CString Title	= file_info.title;
			CString Year	= file_info.year;
			if (!Title.IsEmpty() && !Year.IsEmpty()) {
				Title += _T(" (") + Year + _T(")");
			}

			((CFLACSource*)m_pFilter)->SetProperty(L"TITL", Title);
			((CFLACSource*)m_pFilter)->SetProperty(L"AUTH", file_info.artist);
			((CFLACSource*)m_pFilter)->SetProperty(L"DESC", file_info.comment);
			((CFLACSource*)m_pFilter)->SetProperty(L"ALBUM", file_info.album);
		}

		if (m_Cover.GetCount()) {
			((CFLACSource*)m_pFilter)->ResAppend(L"cover.jpg", L"cover", m_CoverMime, m_Cover.GetData(), (DWORD)m_Cover.GetCount(), 0);
		} else if (pID3Tag) {
			BOOL bResAppend = FALSE;

			POSITION pos = pID3Tag->TagItems.GetHeadPosition();
			while (pos && !bResAppend) {
				CID3TagItem* item = pID3Tag->TagItems.GetNext(pos);
				if (item->GetType() == ID3Type::ID3_TYPE_BINARY && item->GetDataLen()) {
					CString mime = item->GetMime();
					CString fname;
					if (mime == L"image/jpeg") {
						fname = L"cover.jpg";
					} else if (mime == L"image/png") {
						fname = L"cover.png";
					} else {
						fname.Replace(L"image/", L"cover.");
					}

					HRESULT hr2 = ((CFLACSource*)m_pFilter)->ResAppend(fname, L"cover", mime, (BYTE*)item->GetData(), (DWORD)item->GetDataLen(), 0);
					bResAppend = SUCCEEDED(hr2);
				}
			}
		}

		if (pID3Tag) {
			delete pID3Tag;
		}

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

CFLACStream::~CFLACStream()
{
	if (m_pDecoder) {
		FLAC__stream_decoder_delete(FLAC_DECODER);
		m_pDecoder = NULL;
	}
}

HRESULT CFLACStream::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
	ASSERT(pAlloc);
	ASSERT(pProperties);

	HRESULT hr = NOERROR;

	pProperties->cBuffers = 1;
	pProperties->cbBuffer = m_nMaxFrameSize;

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

HRESULT CFLACStream::FillBuffer(IMediaSample* pSample, int nFrame, BYTE* pOut, long& len)
{
	FLAC__uint64	llCurPos;
	FLAC__uint64	llNextPos;

	if (m_bDiscontinuity) {
		FLAC__stream_decoder_seek_absolute(FLAC_DECODER, FLAC__uint64((1.0 * m_rtPosition * m_i64TotalNumSamples) / m_rtDuration));
		// need calculate in double, because uint64 can give overflow
	}

	FLAC__stream_decoder_get_decode_position(FLAC_DECODER, &llCurPos);

	FLAC__stream_decoder_skip_single_frame(FLAC_DECODER);

	FLAC__stream_decoder_get_decode_position(FLAC_DECODER, &llNextPos);

	FLAC__uint64 llCurFile = m_file.GetPosition();
	len = (long)(llNextPos - llCurPos);
	ASSERT(len > 0);
	if (len <= 0) {
		return S_FALSE;
	}

	m_file.Seek(llCurPos, CFile::begin);
	try {
		m_file.Read(pOut, len);
	} catch (CFileException* e) {
		// TODO - Handle exception
		e->Delete();
		len = 0;
	}
	m_file.Seek(llCurFile, CFile::begin);

	if (len) {
		if ((FLAC_DECODER)->protected_->blocksize > 0 && (FLAC_DECODER)->protected_->sample_rate > 0) {
			m_AvgTimePerFrame = (FLAC_DECODER)->protected_->blocksize * UNITS / (FLAC_DECODER)->protected_->sample_rate;
		} else {
			m_AvgTimePerFrame = m_rtDuration * len / (m_llFileSize - m_llOffset);
		}
	}

	return S_OK;
}

HRESULT CFLACStream::GetMediaType(int iPosition, CMediaType* pmt)
{
	CAutoLock cAutoLock(m_pFilter->pStateLock());

	if (iPosition == 0) {
		pmt->majortype			= MEDIATYPE_Audio;
		pmt->subtype			= MEDIASUBTYPE_FLAC_FRAMED;
		pmt->formattype			= FORMAT_WaveFormatEx;
		WAVEFORMATEX* wfe		= (WAVEFORMATEX*)pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX) + m_extradata.GetCount());
		memset(wfe, 0, sizeof(WAVEFORMATEX));
		wfe->wFormatTag			= WAVE_FORMAT_FLAC;
		wfe->nChannels			= m_nChannels;
		wfe->nSamplesPerSec		= m_nSamplesPerSec;
		wfe->nAvgBytesPerSec	= m_nAvgBytesPerSec;
		wfe->nBlockAlign		= 1;
		wfe->wBitsPerSample		= m_wBitsPerSample;
		wfe->cbSize				= m_extradata.GetCount();
		memcpy(wfe + 1, m_extradata.GetData(), m_extradata.GetCount());
	} else {
		return VFW_S_NO_MORE_ITEMS;
	}

	pmt->SetTemporalCompression(FALSE);

	return S_OK;
}

HRESULT CFLACStream::CheckMediaType(const CMediaType* pmt)
{
	if (pmt->majortype		== MEDIATYPE_Audio
		&& pmt->subtype		== MEDIASUBTYPE_FLAC_FRAMED
		&& pmt->formattype	== FORMAT_WaveFormatEx
		&& ((WAVEFORMATEX*)pmt->pbFormat)->wFormatTag == WAVE_FORMAT_FLAC) {
		return S_OK;
	} else {
		return E_INVALIDARG;
	}
}

static bool ParseVorbisTag(const CString field_name, const CString VorbisTag, CString* TagValue) {
	TagValue->Empty();

	CString vorbis_data = VorbisTag;
	vorbis_data.MakeLower().Trim();
	if (vorbis_data.Find(field_name + '=') != 0) {
		return false;
	}

	vorbis_data = VorbisTag;
	vorbis_data.Delete(0, vorbis_data.Find('=') + 1);
	*TagValue = vorbis_data;

	return true;
}

/*
 * Calculate an estimate for the maximum frame size based on verbatim mode.
 * get from ffmpeg
 */
static int CalculateFLACFrameSize(UINT max_blocksize, UINT channels, UINT bits_per_sample)
{
	int count;

	count = 16;										/* frame header */
	count += channels * ((7+bits_per_sample+7)/8);	/* subframe headers */
	if (channels == 2) {
		/* for stereo, need to account for using decorrelation */
		count += (( 2*bits_per_sample+1) * max_blocksize + 7) / 8;
	} else {
		count += ( channels*bits_per_sample * max_blocksize + 7) / 8;
	}
	count += 2;										/* frame footer */

	return count;
}

void CFLACStream::UpdateFromMetadata (void* pBuffer)
{
	const FLAC__StreamMetadata* pMetadata = (const FLAC__StreamMetadata*)pBuffer;

	if (pMetadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		const FLAC__StreamMetadata_StreamInfo *si = &pMetadata->data.stream_info;

		m_nMaxFrameSize			= si->max_framesize;
		if (!m_nMaxFrameSize) {
			m_nMaxFrameSize		= CalculateFLACFrameSize(si->max_blocksize,
														 si->channels,
														 si->bits_per_sample);
		}
		m_nSamplesPerSec		= si->sample_rate;
		m_nChannels				= si->channels;
		m_wBitsPerSample		= si->bits_per_sample;
		m_i64TotalNumSamples	= si->total_samples;
		m_nAvgBytesPerSec		= (m_nChannels * (m_wBitsPerSample >> 3)) * m_nSamplesPerSec;

		// === Init members from base classes
		GetFileSizeEx (m_file.m_hFile, (LARGE_INTEGER*)&m_llFileSize);
		m_rtDuration			= (m_i64TotalNumSamples * UNITS) / m_nSamplesPerSec;
		m_rtStop				= m_rtDuration;
		m_AvgTimePerFrame		= (m_nMaxFrameSize + si->min_framesize) * m_rtDuration / 2 / m_llFileSize;
	} else if (pMetadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
		const FLAC__StreamMetadata_VorbisComment *vc = &pMetadata->data.vorbis_comment;

		file_info.got_vorbis_comments = (vc->num_comments > 0);
		for(unsigned i = 0; i < vc->num_comments; i++) {
			CString TagValue;
			CString VorbisTag = AltUTF8To16((LPCSTR)vc->comments[i].entry);
			if (VorbisTag.GetLength() > 0) {
				if (ParseVorbisTag(L"artist", VorbisTag, &TagValue)) {
					file_info.artist = TagValue;
				} else if (ParseVorbisTag(L"title", VorbisTag, &TagValue)) {
					file_info.title = TagValue;
				} else if (ParseVorbisTag(L"description", VorbisTag, &TagValue)) {
					file_info.comment = TagValue;
				} else if (ParseVorbisTag(L"comment", VorbisTag, &TagValue)) {
					file_info.comment = TagValue;
				} else if (ParseVorbisTag(L"date", VorbisTag, &TagValue)) {
					file_info.year = TagValue;
				} else if (ParseVorbisTag(L"album", VorbisTag, &TagValue)) {
					file_info.album = TagValue;
				} else if (ParseVorbisTag(L"cuesheet", VorbisTag, &TagValue)) {
					CAtlList<Chapters> ChaptersList;
					CString Title, Performer;
					if (ParseCUESheet(TagValue, ChaptersList, Title, Performer)) {
						if (Title.GetLength() > 0 && file_info.title.IsEmpty()) {
							file_info.title = Title;
						}
						if (Performer.GetLength() > 0 && file_info.artist.IsEmpty()) {
							file_info.artist = Performer;
						}

						((CFLACSource*)m_pFilter)->ChapRemoveAll();
						while (ChaptersList.GetCount()) {
							Chapters cp = ChaptersList.RemoveHead();
							((CFLACSource*)m_pFilter)->ChapAppend(cp.rt, cp.name);
						}
					}
				}
			}
		}
	} else if (pMetadata->type == FLAC__METADATA_TYPE_PICTURE) {
		const FLAC__StreamMetadata_Picture *pic = &pMetadata->data.picture;

		if (!m_Cover.GetCount() && pic->data_length) {
			m_CoverMime = pic->mime_type;
			m_Cover.SetCount(pic->data_length);
			memcpy(m_Cover.GetData(), pic->data, pic->data_length);
		}
	} else if (pMetadata->type == FLAC__METADATA_TYPE_CUESHEET) {
		if (((CFLACSource*)m_pFilter)->ChapGetCount()) {
			return;
		}

		const FLAC__StreamMetadata_CueSheet *cs = &pMetadata->data.cue_sheet;

		for(size_t i = 0; i < cs->num_tracks; ++i) {
			FLAC__StreamMetadata_CueSheet_Track *track = &cs->tracks[i];
			if (track->type || track->offset >= (FLAC__uint64)m_i64TotalNumSamples) {
				continue;
			}

			REFERENCE_TIME rt = MILLISECONDS_TO_100NS_UNITS(1000*track->offset/m_nSamplesPerSec);
			CString s;
			s.Format(_T("Track %02d"), i+1);
			((CFLACSource*)m_pFilter)->ChapAppend(rt, s);

			if (track->num_indices > 1) {
				for (int j = 0; j < track->num_indices; ++j) {
					FLAC__StreamMetadata_CueSheet_Index *index = &track->indices[j];
					s.Format(_T("+ INDEX %02d"), index->number);
					REFERENCE_TIME r = rt + MILLISECONDS_TO_100NS_UNITS(1000*index->offset/m_nSamplesPerSec);
					((CFLACSource*)m_pFilter)->ChapAppend(r, s);
				}
			}
		}
	}
}

file_info_struct CFLACStream::GetInfo()
{
	return file_info;
}

FLAC__StreamDecoderReadStatus StreamDecoderRead(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
{
	CFLACStream*	pThis = static_cast<CFLACStream*>(client_data);
	UINT			nRead = pThis->GetFile()->Read(buffer, (UINT)*bytes);

	pThis->m_bIsEOF	= (nRead != *bytes);
	*bytes			= nRead;

	return (*bytes == 0) ? FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM : FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

FLAC__StreamDecoderSeekStatus StreamDecoderSeek(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
	CFLACStream*	pThis = static_cast<CFLACStream*>(client_data);

	pThis->m_bIsEOF	= false;
	pThis->GetFile()->Seek(absolute_byte_offset, CFile::begin);
	return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__StreamDecoderTellStatus StreamDecoderTell(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	CFLACStream*	pThis = static_cast<CFLACStream*>(client_data);
	*absolute_byte_offset = pThis->GetFile()->GetPosition();
	return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus StreamDecoderLength(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
{
	CFLACStream*	pThis = static_cast<CFLACStream*>(client_data);
	CFile*			pFile = pThis->GetFile();

	if (pFile == NULL) {
		return FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;
	} else {
		*stream_length = pFile->GetLength();
		return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
	}
}

FLAC__bool StreamDecoderEof(const FLAC__StreamDecoder *decoder, void *client_data)
{
	CFLACStream*	pThis = static_cast<CFLACStream*>(client_data);

	return pThis->m_bIsEOF;
}

FLAC__StreamDecoderWriteStatus StreamDecoderWrite(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	CFLACStream* pThis = static_cast<CFLACStream*>(client_data);
	UNREFERENCED_PARAMETER(pThis);

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void StreamDecoderError(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	TRACE(_T("FLAC::StreamDecoderError() : %s\n"), FLAC__StreamDecoderErrorStatusString[status]);
}

void StreamDecoderMetadata(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	CFLACStream*	pThis = static_cast<CFLACStream*>(client_data);

	if (pThis) {
		pThis->UpdateFromMetadata((void*)metadata);
	}
}

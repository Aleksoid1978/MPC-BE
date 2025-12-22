/*
 * (C) 2020-2025 see Authors.txt
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
#include "FLACFile.h"
#include "DSUtil/ID3V2PictureType.h"
#include <ExtLib/libflac/src/libflac/include/protected/stream_decoder.h>

#define FLAC_DECODER (FLAC__StreamDecoder*)m_pDecoder

 // Declaration for FLAC callbacks
static FLAC__StreamDecoderReadStatus   StreamDecoderRead(const FLAC__StreamDecoder* decoder, FLAC__byte buffer[], size_t* bytes, void* client_data);
static FLAC__StreamDecoderSeekStatus   StreamDecoderSeek(const FLAC__StreamDecoder* decoder, FLAC__uint64 absolute_byte_offset, void* client_data);
static FLAC__StreamDecoderTellStatus   StreamDecoderTell(const FLAC__StreamDecoder* decoder, FLAC__uint64* absolute_byte_offset, void* client_data);
static FLAC__StreamDecoderLengthStatus StreamDecoderLength(const FLAC__StreamDecoder* decoder, FLAC__uint64* stream_length, void* client_data);
static FLAC__bool                      StreamDecoderEof(const FLAC__StreamDecoder* decoder, void* client_data);
static FLAC__StreamDecoderWriteStatus  StreamDecoderWrite(const FLAC__StreamDecoder* decoder, const FLAC__Frame* frame, const FLAC__int32* const buffer[], void* client_data);
static void                            StreamDecoderError(const FLAC__StreamDecoder* decoder, FLAC__StreamDecoderErrorStatus status, void* client_data);
static void                            StreamDecoderMetadata(const FLAC__StreamDecoder* decoder, const FLAC__StreamMetadata* metadata, void* client_data);

//
// CFLACFile
//

CFLACFile::CFLACFile()
	: CAudioFile()
{
	m_subtype = MEDIASUBTYPE_FLAC_FRAMED;
	m_wFormatTag = WAVE_FORMAT_FLAC;
}

CFLACFile::~CFLACFile()
{
	if (m_pDecoder) {
		FLAC__stream_decoder_delete(FLAC_DECODER);
		m_pDecoder = nullptr;
	}
}

void CFLACFile::SetProperties(IBaseFilter* pBF)
{
	if (m_info.got_vorbis_comments) {
		if (CComQIPtr<IDSMPropertyBag> pPB = pBF) {
			CString title = m_info.title;
			if (!title.IsEmpty() && !m_info.year.IsEmpty()) {
				title += L" (" + m_info.year + L")";
			}

			pPB->SetProperty(L"TITL", title);
			pPB->SetProperty(L"AUTH", m_info.artist);
			pPB->SetProperty(L"DESC", m_info.comment);
			pPB->SetProperty(L"ALBUM", m_info.album);
		}
	}

	if (!m_chapters.empty()) {
		if (CComQIPtr<IDSMChapterBag> pCB = pBF) {
			pCB->ChapRemoveAll();
			for (const auto& chap : m_chapters) {
				pCB->ChapAppend(chap.rt, chap.name);
			}
		}
	}

	if (m_covers.size()) {
		if (CComQIPtr<IDSMResourceBag> pRB = pBF) {
			for (auto& cover : m_covers) {
				pRB->ResAppend(cover.name, cover.desc, cover.mime, cover.picture.data(), (DWORD)cover.picture.size(), 0);
			}
		}
	}

	__super::SetProperties(pBF);
}

HRESULT CFLACFile::Open(CBaseSplitterFile* pFile)
{
	m_pFile = pFile;
	m_pFile->Seek(0);
	__int64 start_pos = 0;

	if (ReadID3Tag(m_pFile->GetRemaining())) {
		start_pos = m_pFile->GetPos();
	}
	m_pFile->Seek(start_pos);

	DWORD id = 0;
	if (m_pFile->ByteRead((BYTE*)&id, 4) != S_OK || id != FCC('fLaC')) {
		return E_FAIL;
	}

	m_offset = m_pFile->GetPos();

	m_pFile->Seek(start_pos);

	m_pDecoder = FLAC__stream_decoder_new();
	CheckPointer(m_pDecoder, E_FAIL);

	FLAC__stream_decoder_set_metadata_respond(FLAC_DECODER, FLAC__METADATA_TYPE_VORBIS_COMMENT);
	FLAC__stream_decoder_set_metadata_respond(FLAC_DECODER, FLAC__METADATA_TYPE_PICTURE);
	FLAC__stream_decoder_set_metadata_respond(FLAC_DECODER, FLAC__METADATA_TYPE_CUESHEET);

	if (FLAC__STREAM_DECODER_INIT_STATUS_OK != FLAC__stream_decoder_init_stream(FLAC_DECODER,
			StreamDecoderRead,
			StreamDecoderSeek,
			StreamDecoderTell,
			StreamDecoderLength,
			StreamDecoderEof,
			StreamDecoderWrite,
			StreamDecoderMetadata,
			StreamDecoderError,
			this)) {
		return E_FAIL;
	}

	if (!FLAC__stream_decoder_process_until_end_of_metadata(FLAC_DECODER)) {
		return E_FAIL;
	}

	FLAC__uint64 metadataend = 0;
	if (!FLAC__stream_decoder_get_decode_position(FLAC_DECODER, &metadataend)) {
		return E_FAIL;
	}
	if (metadataend) {
		m_extradata.SetSize(metadataend);
		m_pFile->Seek(start_pos);
		if (m_pFile->ByteRead(m_extradata.Data(), m_extradata.Bytes()) != S_OK) {
			return E_FAIL;
		}

		m_offset = metadataend;
	}

	m_curPos = m_offset;

	return S_OK;
}

REFERENCE_TIME CFLACFile::Seek(REFERENCE_TIME rt)
{
	if (rt <= 0) {
		FLAC__stream_decoder_seek_absolute(FLAC_DECODER, 0);
		m_curPos = m_offset;
		return 0;
	}

	if (m_rtduration) {
		auto sample = static_cast<FLAC__uint64>(llMulDiv(rt, m_totalsamples, m_rtduration, 0));
		if (sample > 0) {
			sample--;
		}
		FLAC__stream_decoder_seek_absolute(FLAC_DECODER, sample);

		FLAC__stream_decoder_get_decode_position(FLAC_DECODER, &m_curPos);
		FLAC__stream_decoder_skip_single_frame(FLAC_DECODER);
	}

	return rt;
}

int CFLACFile::GetAudioFrame(CPacket* packet, REFERENCE_TIME rtStart)
{
	FLAC__uint64 nextpos;
	if (!FLAC__stream_decoder_get_decode_position(FLAC_DECODER, &nextpos)) {
		return 0;
	}

	if (nextpos <= m_curPos) {
		return 0;
	}

	auto pos = m_pFile->GetPos();
	const auto size = nextpos - m_curPos;
	m_pFile->Seek(m_curPos);
	if (!packet->SetCount(size) || m_pFile->ByteRead(packet->data(), size) != S_OK) {
		return 0;
	}

	packet->rtStart = rtStart;
	REFERENCE_TIME nAvgTimePerFrame;
	const auto protected_ = (FLAC_DECODER)->protected_;
	if (protected_->blocksize > 0 && protected_->sample_rate > 0) {
		nAvgTimePerFrame = llMulDiv(protected_->blocksize, UNITS, protected_->sample_rate, 0);
	} else {
		nAvgTimePerFrame = std::max(1LL, llMulDiv(m_rtduration, size, m_pFile->GetLength() - m_offset, 0));
	}
	packet->rtStop = rtStart + nAvgTimePerFrame;

	m_pFile->Seek(pos);

	FLAC__stream_decoder_skip_single_frame(FLAC_DECODER);
	m_curPos = nextpos;

	return size;
}

void CFLACFile::UpdateFromMetadata(void* pBuffer)
{
	auto pMetadata = (const FLAC__StreamMetadata*)pBuffer;

	if (pMetadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		const auto& si = pMetadata->data.stream_info;

		m_samplerate   = si.sample_rate;
		m_channels     = si.channels;
		m_bitdepth     = si.bits_per_sample;
		m_totalsamples = si.total_samples;
		m_rtduration   = (m_totalsamples * UNITS) / m_samplerate;
	} else if (pMetadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
		const auto ParseVorbisTag = [](const CString& field_name, const CString& VorbisTag, CString& TagValue) {
			TagValue.Empty();

			CString vorbis_data(VorbisTag);
			vorbis_data.MakeLower().Trim();
			if (vorbis_data.Find(field_name + L'=') != 0) {
				return false;
			}

			vorbis_data = VorbisTag;
			vorbis_data.Delete(0, vorbis_data.Find(L'=') + 1);
			TagValue = vorbis_data;

			return true;
		};

		const auto& vc = pMetadata->data.vorbis_comment;

		m_info.got_vorbis_comments = (vc.num_comments > 0);
		for (unsigned i = 0; i < vc.num_comments; i++) {
			CString TagValue;
			CString VorbisTag = AltUTF8ToWStr((LPCSTR)vc.comments[i].entry);
			if (VorbisTag.GetLength() > 0) {
				if (ParseVorbisTag(L"artist", VorbisTag, TagValue)) {
					if (m_info.artist.IsEmpty()) {
						m_info.artist = TagValue;
					} else {
						m_info.artist += L" / " + TagValue;
					}
				} else if (ParseVorbisTag(L"title", VorbisTag, TagValue)) {
					m_info.title = TagValue;
				} else if (ParseVorbisTag(L"description", VorbisTag, TagValue)) {
					m_info.comment = TagValue;
				} else if (ParseVorbisTag(L"comment", VorbisTag, TagValue)) {
					m_info.comment = TagValue;
				} else if (ParseVorbisTag(L"date", VorbisTag, TagValue)) {
					m_info.year = TagValue;
				} else if (ParseVorbisTag(L"album", VorbisTag, TagValue)) {
					m_info.album = TagValue;
				} else if (ParseVorbisTag(L"cuesheet", VorbisTag, TagValue)) {
					CString Title, Performer;
					std::list<Chapters> chapters;
					if (ParseCUESheet(TagValue, chapters, Title, Performer)) {
						m_chapters = chapters;

						if (!Title.IsEmpty() && m_info.title.IsEmpty()) {
							m_info.title = Title;
						}
						if (!Performer.IsEmpty() && m_info.artist.IsEmpty()) {
							m_info.artist = Performer;
						}
					}
				}
			}
		}
	} else if (pMetadata->type == FLAC__METADATA_TYPE_PICTURE) {
		const auto& pic = pMetadata->data.picture;
		if (pic.data_length && pic.mime_type) {
			CoverInfo_t cover;
			cover.name = ID3v2PictureTypeToStr(pic.type);
			if (pic.description) {
				cover.desc = AltUTF8ToWStr((char*)pic.description);
			}
			cover.mime = pic.mime_type;
			cover.picture.resize(pic.data_length);
			memcpy(cover.picture.data(), pic.data, pic.data_length);
			m_covers.emplace_back(cover);
		}
	} else if (pMetadata->type == FLAC__METADATA_TYPE_CUESHEET) {
		if (!m_chapters.empty()) {
			return;
		}

		const auto& cs = pMetadata->data.cue_sheet;

		for (uint32_t i = 0; i < cs.num_tracks; ++i) {
			const auto& track = cs.tracks[i];
			if (track.type || track.offset >= m_totalsamples) {
				continue;
			}

			REFERENCE_TIME rt = MILLISECONDS_TO_100NS_UNITS(1000 * track.offset / m_samplerate);
			CString s;
			s.Format(L"Track %02u", i + 1);
			m_chapters.emplace_back(s, rt);

			if (track.num_indices > 1) {
				for (int j = 0; j < track.num_indices; ++j) {
					const auto& index = track.indices[j];
					s.Format(L"+ INDEX %02d", index.number);
					const auto rtIndex = rt + MILLISECONDS_TO_100NS_UNITS(1000 * index.offset / m_samplerate);
					m_chapters.emplace_back(s, rtIndex);
				}
			}
		}
	}
}

FLAC__StreamDecoderReadStatus StreamDecoderRead(const FLAC__StreamDecoder* decoder, FLAC__byte buffer[], size_t* bytes, void* client_data)
{
	if (*bytes > 0) {
		auto pThis = static_cast<CFLACFile*>(client_data);
		auto pFile = pThis->GetFile();
		const auto len = std::min(pFile->GetRemaining(), (__int64)*bytes);
		const auto hr = pFile->ByteRead(buffer, len);

		pThis->m_bIsEOF = (FAILED(hr) || *bytes != len);
		*bytes = FAILED(hr) ? 0 : len;

		if (FAILED(hr)) {
			return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
		} else if (pThis->m_bIsEOF) {
			return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
		}
		return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
	}

	return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
}

FLAC__StreamDecoderSeekStatus StreamDecoderSeek(const FLAC__StreamDecoder* decoder, FLAC__uint64 absolute_byte_offset, void* client_data)
{
	auto pThis = static_cast<CFLACFile*>(client_data);
	pThis->m_bIsEOF = false;
	pThis->GetFile()->Seek(absolute_byte_offset);

	return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__StreamDecoderTellStatus StreamDecoderTell(const FLAC__StreamDecoder* decoder, FLAC__uint64* absolute_byte_offset, void* client_data)
{
	auto pThis = static_cast<CFLACFile*>(client_data);
	*absolute_byte_offset = pThis->GetFile()->GetPos();

	return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus StreamDecoderLength(const FLAC__StreamDecoder* decoder, FLAC__uint64* stream_length, void* client_data)
{
	auto pThis = static_cast<CFLACFile*>(client_data);
	auto pFile = pThis->GetFile();

	*stream_length = pFile->GetLength();
	return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

FLAC__bool StreamDecoderEof(const FLAC__StreamDecoder* decoder, void* client_data)
{
	auto pThis = static_cast<CFLACFile*>(client_data);

	return pThis->m_bIsEOF;
}

FLAC__StreamDecoderWriteStatus StreamDecoderWrite(const FLAC__StreamDecoder* decoder, const FLAC__Frame* frame, const FLAC__int32* const buffer[], void* client_data)
{
	auto pThis = static_cast<CFLACFile*>(client_data);
	UNREFERENCED_PARAMETER(pThis);

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void StreamDecoderError(const FLAC__StreamDecoder* decoder, FLAC__StreamDecoderErrorStatus status, void* client_data)
{
	DLog(L"StreamDecoderError() : %hs", FLAC__StreamDecoderErrorStatusString[status]);
}

void StreamDecoderMetadata(const FLAC__StreamDecoder* decoder, const FLAC__StreamMetadata* metadata, void* client_data)
{
	auto pThis = static_cast<CFLACFile*>(client_data);
	pThis->UpdateFromMetadata((void*)metadata);
}


/*
 * (C) 2014-2016 see Authors.txt
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
#include "../../../DSUtil/AudioParser.h"
#include "DFFFile.h"

//
// CDFFFile
//


CDFFFile::CDFFFile()
	: CAudioFile()
{
	m_subtype = MEDIASUBTYPE_DSDM;
}

CDFFFile::~CDFFFile()
{
	SAFE_DELETE(m_ID3Tag);
}

bool CDFFFile::ReadDFFChunk(dffchunk_t& chunk)
{
	if (m_pFile->ByteRead((BYTE*)&chunk.id, 4) != S_OK) {
		return false;
	}

	if (m_pFile->ByteRead((BYTE*)&chunk.size, 8) != S_OK) {
		return false;
	}
	chunk.size = _byteswap_uint64(chunk.size);

	return true;
}

bool CDFFFile::parse_dsd_prop(__int64 eof)
{
	dffchunk_t chunk;

	uint16_t channels;
	uint32_t sample_rate;
	uint32_t tag;

	while (m_pFile->GetPos() + 12 < eof && ReadDFFChunk(chunk)) {
		__int64 pos = m_pFile->GetPos();

		switch (chunk.id) {
		case FCC('ABSS'):
			if (chunk.size < 8) {
				return false;
			}
			//hour = avio_rb16(pb);
			//min  = avio_r8(pb);
			//sec  = avio_r8(pb);
			//snprintf(abss, sizeof(abss), "%02dh:%02dm:%02ds:%d", hour, min, sec, avio_rb32(pb));
			//av_dict_set(&st->metadata, "absolute_start_time", abss, 0);
			break;

		case FCC('CHNL'):
			if (chunk.size < 2 || m_pFile->ByteRead((BYTE*)&channels, 2) != S_OK) {
				return false;
			}
			channels = _byteswap_ushort(channels);
			if (chunk.size < 2 + channels * 4) {
				return false;
			}
			m_channels = channels;
			m_layout = 0;

			if (m_channels > 6) {
				DLog(L"CDFFFile::parse_dsd_prop() - invalid channels count : %d", m_channels);
				break;
			}

			for (int i = 0; i < m_channels; i++) {
				uint32_t dsd_layout;
				if (m_pFile->ByteRead((BYTE*)&dsd_layout, 4) != S_OK) {
					return false;
				}

				switch (dsd_layout) {
				case FCC('SLFT'):
				case FCC('MLFT'):
					m_layout |= SPEAKER_FRONT_LEFT;
					break;
				case FCC('SRGT'):
				case FCC('MRGT'):
					m_layout |= SPEAKER_FRONT_RIGHT;
					break;
				case FCC('C   '):
					m_layout |= SPEAKER_FRONT_CENTER;
					break;
				case FCC('LS  '):
					m_layout |= SPEAKER_SIDE_LEFT;
					break;
				case FCC('RS  '):
					m_layout |= SPEAKER_SIDE_RIGHT;
					break;
				case FCC('LFE '):
					m_layout |= SPEAKER_LOW_FREQUENCY;
					break;
				}
			}

			if (CountBits(m_layout) != m_channels) {
				m_layout = 0;
			}
			break;
		case FCC('CMPR'):
			if (chunk.size < 4 || m_pFile->ByteRead((BYTE*)&tag, 4) != S_OK) {
				return false;
			}
			switch (tag) {
				case FCC('DSD '): m_subtype = MEDIASUBTYPE_DSDM; break;
				case FCC('DST '): m_subtype = MEDIASUBTYPE_DST ; break;
				default: return false;
			}
			break;
		case FCC('FS  '):
			if (chunk.size < 4 || m_pFile->ByteRead((BYTE*)&sample_rate, 4) != S_OK) {
				return false;
			}
			m_samplerate = sample_rate = _byteswap_ulong(sample_rate);
			break;
		case FCC('ID3 '):
			//    id3v2_extra_meta = NULL;
			//    ff_id3v2_read(s, ID3v2_DEFAULT_MAGIC, &id3v2_extra_meta, size);
			//    if (id3v2_extra_meta) {
			//        if ((ret = ff_id3v2_parse_apic(s, &id3v2_extra_meta)) < 0) {
			//            ff_id3v2_free_extra_meta(&id3v2_extra_meta);
			//            return ret;
			//        }
			//        ff_id3v2_free_extra_meta(&id3v2_extra_meta);
			//    }
			//
			//    if (size < avio_tell(pb) - orig_pos) {
			//        av_log(s, AV_LOG_ERROR, "id3 exceeds chunk size\n");
			//        return AVERROR_INVALIDDATA;
			//    }
			break;
		case FCC('LSCO'):
			if (chunk.size < 2) {
				return false;
			}
			//    config = avio_rb16(pb);
			//    if (config != 0xFFFF) {
			//        if (config < FF_ARRAY_ELEMS(dsd_loudspeaker_config))
			//            st->codec->channel_layout = dsd_loudspeaker_config[config];
			//        if (!st->codec->channel_layout)
			//            avpriv_request_sample(s, "loudspeaker configuration %d", config);
			//    }
			break;
		}

		m_pFile->Seek(pos + chunk.size);
	}

	return true;
}

bool CDFFFile::parse_dsd_diin(__int64 eof)
{
	dffchunk_t chunk;

	while (m_pFile->GetPos() + 12 < eof && ReadDFFChunk(chunk)) {
		__int64 pos = m_pFile->GetPos();

		if (chunk.size > 4 && (chunk.id == FCC('DIAR') || chunk.id == FCC('DITI'))) {
			uint32_t size;
			CStringA value;

			if (S_OK != m_pFile->ByteRead((BYTE*)&size, 4)) {
				break;
			}
			size = _byteswap_ulong(size);
			if (size + 4 > chunk.size || S_OK != m_pFile->ByteRead((BYTE*)value.GetBufferSetLength(size), size)) {
				break;
			}

			m_info[chunk.id] = value;
		}

		m_pFile->Seek(pos + chunk.size);
	}

	return true;
}

bool CDFFFile::parse_dsd_dst(__int64 eof)
{
	dffchunk_t chunk;

	while (m_pFile->GetPos() + 12 < eof && ReadDFFChunk(chunk)) {
		__int64 pos = m_pFile->GetPos();

		if (chunk.size >= 4 && chunk.id == FCC('FRTE')) {
			uint32_t count;
			if (S_OK != m_pFile->ByteRead((BYTE*)&count, 4)) {
				break;
			}
			count = _byteswap_ulong(count);
			const int fixed_samplerate = m_samplerate / 8;
			m_rtduration = SCALE64(count, 588LL * fixed_samplerate, 44100);
			m_rtduration = SCALE64(m_rtduration, UNITS, fixed_samplerate);

			break;
		}

		m_pFile->Seek(pos + chunk.size);
	}

	return true;
}

#define ABORT									\
	DLog(L"CDFFFile::Open() : broken file!");	\
	return E_ABORT;								\

HRESULT CDFFFile::Open(CBaseSplitterFile* pFile)
{
	m_pFile = pFile;
	m_startpos = 0;

	m_pFile->Seek(0);
	dffchunk_t Chunk;
	DWORD id = 0;
	DWORD version = 0;

	if (!ReadDFFChunk(Chunk)
			|| Chunk.id != FCC('FRM8')
			|| Chunk.size < 16
			|| m_pFile->ByteRead((BYTE*)&id, sizeof(id)) != S_OK
			|| id != FCC('DSD ')) {
		return E_FAIL;
	}

	__int64 end = min((__int64)Chunk.size + 12, m_pFile->GetLength());

	m_samplerate = 0;
	m_channels = 1;
	m_layout   = SPEAKER_FRONT_CENTER;

	while (ReadDFFChunk(Chunk) && m_pFile->GetPos() < end) {
		__int64 pos = m_pFile->GetPos();

		DLog(L"CDFFFile::Open() : found '%c%c%c%c' chunk.",
				WCHAR((Chunk.id>>0)&0xff),
				WCHAR((Chunk.id>>8)&0xff),
				WCHAR((Chunk.id>>16)&0xff),
				WCHAR((Chunk.id>>24)&0xff));

		switch (Chunk.id) {
		case FCC('FVER'):
			if (Chunk.size < 4 || m_pFile->ByteRead((BYTE*)&version, 4) != S_OK) {
				ABORT;
			}
			DLog(L"CDFFFile::Open() : DSIFF v%u.%u.%u.%u", version >> 24, (version >> 16) & 0xFF, (version >> 8) & 0xFF, version & 0xFF);
			break;
		case FCC('PROP'):
			if (Chunk.size < 4 || m_pFile->ByteRead((BYTE*)&id, 4) != S_OK) {
				ABORT;
			}
			if (id != FCC('SND ')) {
				DLog(L"CDFFFile::Open() : unknown property type!");
				break;
			}
			if (!parse_dsd_prop(pos + Chunk.size)) {
				return E_ABORT;
			}
			break;
		case FCC('DSD '):
		case FCC('DST '):
			m_startpos = pos;
			m_length = Chunk.size;
			m_endpos = m_startpos + m_length;
			if (Chunk.id == FCC('DST ')
					&& !parse_dsd_dst(pos + Chunk.size)) {
				return E_ABORT;
			}
			break;
		case FCC('COMT'):
			if (Chunk.size < 2) {
				ABORT;
			}
			{
				__int64 endpos = m_pFile->GetPos() + Chunk.size;
				uint16_t nb_comments;
				if (m_pFile->ByteRead((BYTE*)&nb_comments, 2) != S_OK) {
					ABORT;
				}
				nb_comments = _byteswap_ushort(nb_comments);
				for (int i = 0; i < nb_comments; i++) {
					m_pFile->Skip(6); // skip comment time
					uint16_t type;
					if (m_pFile->ByteRead((BYTE*)&type, 2) != S_OK) {
						ABORT;
					}
					type = _byteswap_ushort(type);
					m_pFile->Skip(2);
					uint32_t metadata_size;
					if (m_pFile->ByteRead((BYTE*)&metadata_size, 4) != S_OK) {
						ABORT;
					}
					metadata_size = _byteswap_ulong(metadata_size);
					switch (type) {
						case 1: // 'channel_comment'
						case 2: // 'source_comment'
						case 3: // 'file_history'
							m_pFile->Skip(metadata_size);
							break;
						default:
							CStringA value;
							if ((m_pFile->GetPos() + metadata_size) > endpos ||
									S_OK != m_pFile->ByteRead((BYTE*)value.GetBufferSetLength(metadata_size), metadata_size)) {
								break;
							}

							m_info[Chunk.id] = value;
							break;
					}
				}
			}
			break;
		case FCC('DIIN'):
			if (!parse_dsd_diin(pos + Chunk.size)) {
				return E_ABORT;
			}
			break;
		default:
			for (int i = 0; i < sizeof(Chunk.id); i++) {
				BYTE ch = ((BYTE*)&Chunk.id)[i];
				if (ch != 0x20 && (ch < '0' || ch > '9') && (ch < 'A' || ch > 'Z') && (ch < 'a' || ch > 'z')) {
					ABORT;
				}
			}
			break;
		}

		m_pFile->Seek(pos + Chunk.size);
	}

	m_bitdepth        = 8;  // hack for ffmpeg decoder
	m_samplerate      /= 8; // hack for ffmpeg decoder
	m_block_size      = m_channels;
	int max_blocks    = m_samplerate / 20; // 50 ms
	m_max_blocksize   = m_block_size * max_blocks;
	m_nAvgBytesPerSec = m_samplerate * m_channels;

	m_length          -= m_length % m_block_size;
	m_endpos          = m_startpos + m_length;

	if (m_subtype == MEDIASUBTYPE_DSDM) {
		m_rtduration = 10000000i64 * m_length / m_nAvgBytesPerSec;
	}

	return S_OK;
}

bool CDFFFile::SetMediaType(CMediaType& mt)
{
	mt.majortype			= MEDIATYPE_Audio;
	mt.formattype			= FORMAT_WaveFormatEx;
	mt.subtype				= m_subtype;
	mt.SetSampleSize(256000);

	WAVEFORMATEX* wfe		= (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX));
	memset(wfe, 0, sizeof(WAVEFORMATEX));
	//wfe->wFormatTag			= m_wFormatTag;
	wfe->nChannels			= m_channels;
	wfe->nSamplesPerSec		= m_samplerate;
	wfe->wBitsPerSample		= m_bitdepth;
	wfe->nBlockAlign		= m_block_size;
	wfe->nAvgBytesPerSec	= m_nAvgBytesPerSec;

	return true;
}

void CDFFFile::SetProperties(IBaseFilter* pBF)
{
	if (m_info.GetCount() > 0) {
		if (CComQIPtr<IDSMPropertyBag> pPB = pBF) {
			POSITION pos = m_info.GetStartPosition();
			while (pos) {
				DWORD fcc;
				CStringA value;
				m_info.GetNextAssoc(pos, fcc, value);

				switch (fcc) {
				case FCC('DITI'):
					pPB->SetProperty(L"TITL", CString(value));
					break;
				case FCC('DIAR'):
					pPB->SetProperty(L"AUTH", CString(value));
					break;
				case FCC('COMT'):
					pPB->SetProperty(L"DESC", CString(value));
				}
			}
		}
	}

	if (m_ID3Tag) {
		SetID3TagProperties(pBF, m_ID3Tag);
	}
}


REFERENCE_TIME CDFFFile::Seek(REFERENCE_TIME rt)
{
	if (rt <= 0) {
		m_pFile->Seek(m_startpos);
		return 0;
	}

	if (m_subtype == MEDIASUBTYPE_DST) {
		__int64 len = SCALE64(m_length, rt , m_rtduration);
		m_pFile->Seek(m_startpos + len);
		return rt;
	}

	__int64 len = SCALE64(m_length, rt , m_rtduration);
	len -= len % m_block_size;
	m_pFile->Seek(m_startpos + len);

	rt = 10000000i64 * len / m_nAvgBytesPerSec;

	return rt;
}

int CDFFFile::GetAudioFrame(CPacket* packet, REFERENCE_TIME rtStart)
{
	if (m_subtype == MEDIASUBTYPE_DST) {
		const __int64 start = m_pFile->GetPos();
		DWORD dw;
		for (__int64 i = 0, j = m_endpos - start - sizeof(dffchunk_t);
				i <= 10 * MEGABYTE && i < j && S_OK == m_pFile->ByteRead((BYTE*)&dw, sizeof(dw));
				i++, m_pFile->Seek(start + i)) {
			if (dw == FCC('DSTF')) {
				// sync
				m_pFile->Seek(start + i);
				dffchunk_t Chunk;
				if (ReadDFFChunk(Chunk) && ((__int64)Chunk.size + m_pFile->GetPos()) <= m_endpos) {
					const int size = Chunk.size;
					if (!packet->SetCount(size) || m_pFile->ByteRead(packet->GetData(), size) != S_OK) {
						return 0;
					}

					REFERENCE_TIME duration = (588 * m_samplerate / 44100);
					duration = SCALE64(duration, UNITS, m_samplerate);
					packet->rtStart	= rtStart;
					packet->rtStop	= rtStart + duration;

					return size;
				}
			}
		}

		return 0;
	}

	if (m_pFile->GetPos() + m_block_size > m_endpos) {
		return 0;
	}

	int size = min(m_max_blocksize, m_endpos - m_pFile->GetPos());
	if (!packet->SetCount(size) || m_pFile->ByteRead(packet->GetData(), size) != S_OK) {
		return 0;
	}

	__int64 len = m_pFile->GetPos() - m_startpos;
	packet->rtStart	= SCALE64(m_rtduration, len, m_length);
	packet->rtStop	= SCALE64(m_rtduration, (len + size), m_length);

	return size;
}

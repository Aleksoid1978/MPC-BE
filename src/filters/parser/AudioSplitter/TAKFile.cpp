/*
 * (C) 2014-2015 see Authors.txt
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
#include "TAKFile.h"

enum TAKMetaDataType {
	TAK_METADATA_END = 0,
	TAK_METADATA_STREAMINFO,
	TAK_METADATA_SEEKTABLE,
	TAK_METADATA_SIMPLE_WAVE_DATA,
	TAK_METADATA_ENCODER,
	TAK_METADATA_PADDING,
	TAK_METADATA_MD5,
	TAK_METADATA_LAST_FRAME,
};

enum TAKCodecType {
	TAK_CODEC_Integer24bit_TAK10 = 0,
	TAK_CODEC_Experimental,
	TAK_CODEC_Integer24bit_TAK20,
	TAK_CODEC_LossyWav_TAK21Beta,
	TAK_CODEC_Integer24bit_MC_TAK22
};

enum TAKFrameType {
	TAK_FRAME_94ms = 0,
	TAK_FRAME_125ms,
	TAK_FRAME_188ms,
	TAK_FRAME_250ms,
	TAK_FRAME_4096,
	TAK_FRAME_8192,
	TAK_FRAME_16384,
	TAK_FRAME_512,
	TAK_FRAME_1024,
	TAK_FRAME_2048,
};

#define BSWAP24(x) { x = (((x & 0xff) << 16) | (x >> 16) | (x & 0xff00)); }

#define CRC24_INIT 0xb704ceL
#define CRC24_POLY 0x1864cfbL

inline long crc_octets(BYTE *octets, size_t len)
{
	long crc = CRC24_INIT;

	while (len--) {
		crc ^= (*octets++) << 16;
		for (int i = 0; i < 8; i++) {
			crc <<= 1;
			if (crc & 0x1000000) {
				crc ^= CRC24_POLY;
			}
		}
	}

	return crc & 0xffffffL;
}

static int GetTAKFrameNumber(BYTE* buf, int size)
{
	if (size < 32 || *(WORD*)buf != 0xA0FF) { // sync
		return -1;
	}

	uint8_t  LastFrame   = buf[2] & 0x1;
	uint8_t  HasInfo     = buf[2] >> 1 & 0x1;
	uint32_t FrameNumber = *(uint32_t*)(buf + 1) >> 11;

	int shiftbytes = 5; // (16 + 1 + 1 + 1 + 21) bits

	if (LastFrame) {
		shiftbytes += 2; // (14 + 2) bits
	}

	if (HasInfo) {
		uint8_t  FrameSizeType  = buf[shiftbytes + 1] >> 2 & 0xF;
		uint8_t  DataType       = (buf[shiftbytes + 6] >> 1 & 0x7); // 0 - PCM
		if (FrameSizeType > TAK_FRAME_2048 || DataType != 0) {
			return -1;
		}

		int infobits = (6 + 4) + (4 + 35) + (3 + 18 + 5 + 4 + 1);
		uint8_t ChannelNum   = (buf[shiftbytes + 9] >> 3 & 0xF) + 1;
		uint8_t HasExtension = buf[shiftbytes + 9] >> 7;
		if (HasExtension) {
			infobits += (5 + 1);
			uint8_t HasSpeakerAssignment = buf[shiftbytes + 10] >> 5 & 0x1;
			if (HasSpeakerAssignment) {
				infobits += ChannelNum * 6;
			}
		}
		uint8_t PrevFramePos = buf[shiftbytes + infobits/8] >> (infobits % 8) & 0x1;
		infobits += (1 + 5); // Extra info
		if (PrevFramePos) {
			infobits += 25;
		}

		shiftbytes += (infobits + 7) / 8; // 0..7 bits for padding
	}

	int crc = int(*(uint32_t*)(buf + shiftbytes - 1) >> 8);
	if (crc != crc_octets(buf, shiftbytes)) {
		return -1;
	}

	return (int)FrameNumber;
}

//
// CTAKFile
//

CTAKFile::CTAKFile()
	: CAudioFile()
	, m_samples(0)
	, m_framelen(0)
	, m_totalframes(0)
	, m_APETag(NULL)
{
	m_subtype = MEDIASUBTYPE_TAK;
}

CTAKFile::~CTAKFile()
{
	SAFE_DELETE(m_APETag);
}

void CTAKFile::SetProperties(IBaseFilter* pBF)
{
	if (m_APETag) {
		SetAPETagProperties(pBF, m_APETag);
	}
}

bool CTAKFile::ParseTAKStreamInfo(BYTE* buf, int size)
{
	static const uint16_t frame_duration_type_quants[] = { 3, 4, 6, 8, 4096, 8192, 16384, 512, 1024, 2048 };
	static const DWORD tak_channels[] = {
		0,
		SPEAKER_FRONT_LEFT,
		SPEAKER_FRONT_RIGHT,
		SPEAKER_FRONT_CENTER,
		SPEAKER_LOW_FREQUENCY,
		SPEAKER_BACK_LEFT,
		SPEAKER_BACK_RIGHT,
		SPEAKER_FRONT_LEFT_OF_CENTER,
		SPEAKER_FRONT_RIGHT_OF_CENTER,
		SPEAKER_BACK_CENTER,
		SPEAKER_SIDE_LEFT,
		SPEAKER_SIDE_RIGHT,
		SPEAKER_TOP_CENTER,
		SPEAKER_TOP_FRONT_LEFT,
		SPEAKER_TOP_FRONT_CENTER,
		SPEAKER_TOP_FRONT_RIGHT,
		SPEAKER_TOP_BACK_LEFT,
		SPEAKER_TOP_BACK_CENTER,
		SPEAKER_TOP_BACK_RIGHT,
	};

	if (size < 10) {
		return false;
	}

	// Encoder info
	uint8_t  Codec          = buf[0] & 0x3F;
	uint16_t Profile        = *(uint16_t*)&buf[0] >> 6 & 0xF;

	// Frame size information
	uint8_t  FrameSizeType  = buf[1] >> 2 & 0xF;
	uint64_t SampleNum      = *(uint64_t*)&buf[1] >> 6 & 0x1FFFFFFFFF;
	if (FrameSizeType > TAK_FRAME_2048) {
		return false;
	}

	// Audio format
	uint8_t  DataType       = (buf[6] >> 1 & 0x7); // 0 - PCM
	uint32_t SampleRate     = (*(uint32_t*)&buf[6] >> 4 & 0x3FFFF) + 6000;
	uint16_t SampleBits     = (*(uint16_t*)&buf[8] >> 6 & 0x1F) + 8;
	uint8_t  ChannelNum     = (buf[9] >> 3 & 0xF) + 1;
	uint8_t  HasExtension   = buf[9] >> 7;
	if (DataType != 0) {
		return false;
	}
	DWORD   ChannelMask = 0;
	if (HasExtension && size >= 11) {
		uint8_t ValidBitsPerSample   = (buf[10] & 0x1F) + 1;
		uint8_t HasSpeakerAssignment = (buf[10] >> 5 & 0x1);
		if (HasSpeakerAssignment && size >= (80 + ChannelNum * 6 + 7) / 8) {
			int ch = 1;
			int bytepos = 10;
			do {
				ChannelMask |= (ch++ <= ChannelNum) ? tak_channels[*(uint16_t*)&buf[bytepos + 0] >> 6 & 0x3F] : 0;
				ChannelMask |= (ch++ <= ChannelNum) ? tak_channels[*(uint16_t*)&buf[bytepos + 1] >> 4 & 0x3F] : 0;
				ChannelMask |= (ch++ <= ChannelNum) ? tak_channels[buf[bytepos + 2] >> 2 & 0x3F]              : 0;
				ChannelMask |= (ch++ <= ChannelNum) ? tak_channels[buf[bytepos + 3] & 0x3F]                   : 0;
				bytepos += 3;
			} while (ch <= ChannelNum);
		}
	}
	//if (ChannelMask == 0) {
	//	ChannelMask = GetDefChannelMask(ChannelNum);
	//}

	int nb_samples, max_nb_samples;
	if (FrameSizeType <= TAK_FRAME_250ms) {
		nb_samples     = SampleRate * frame_duration_type_quants[FrameSizeType] >> 5;
		max_nb_samples = 16384;
	} else if (FrameSizeType < _countof(frame_duration_type_quants)) {
		nb_samples     = frame_duration_type_quants[FrameSizeType];
		max_nb_samples = SampleRate * frame_duration_type_quants[TAK_FRAME_250ms] >> 5;
	} else {
		return false;
	}

	m_samplerate  = SampleRate;
	m_bitdepth    = SampleBits;
	m_channels    = ChannelNum;
	m_layout      = ChannelMask;
	m_samples     = SampleNum;
	m_framelen    = nb_samples;
	m_totalframes = m_samples / m_framelen;

	return true;
}

HRESULT CTAKFile::Open(CBaseSplitterFile* pFile)
{
	m_pFile = pFile;

	m_pFile->Seek(0);
	DWORD id = 0;
	if (m_pFile->ByteRead((BYTE*)&id, 4) != S_OK || id != FCC('tBaK')) {
		return E_FAIL;
	}

	BOOL bLastFrame	= FALSE;
	BOOL bIsEnd		= FALSE;
	while ((m_pFile->GetRemaining() >= 4) && !bIsEnd) {
		TAKMetaDataType type	= (TAKMetaDataType)(m_pFile->BitRead(8) & 0x7f);
		int size				= m_pFile->BitRead(24);
		BSWAP24(size);

		m_startpos				= min(m_pFile->GetPos() + size, m_pFile->GetLength());

		BYTE* buffer			= NULL;

		switch (type) {
			case TAK_METADATA_STREAMINFO:
			case TAK_METADATA_LAST_FRAME:
				{
					buffer = DNew BYTE[size];
					if (!buffer) {
						return E_FAIL;
					}
					if (m_pFile->ByteRead(buffer, size) != S_OK) {
						delete [] buffer;
						return E_FAIL;
					}

					// check crc
					m_pFile->Seek(m_pFile->GetPos() - 3);
					int crc		= m_pFile->BitRead(24);
					BSWAP24(crc);
					if (crc != crc_octets(buffer, size - 3)) {
						delete [] buffer;
						return E_FAIL;
					}
				}
				break;
			case TAK_METADATA_END:
				{
					m_endpos += m_pFile->GetPos();

					// parse APE Tag Header
					BYTE buf[APE_TAG_FOOTER_BYTES];
					memset(buf, 0, sizeof(buf));
					__int64 cur_pos = m_pFile->GetPos();
					__int64 file_size = m_pFile->GetLength();

					CAtlArray<BYTE>		CoverData;
					CString				CoverMime;
					CString				CoverFileName;

					if (cur_pos + APE_TAG_FOOTER_BYTES <= file_size) {
						m_pFile->Seek(file_size - APE_TAG_FOOTER_BYTES);
						if (m_pFile->ByteRead(buf, APE_TAG_FOOTER_BYTES) == S_OK) {
							m_APETag = DNew CAPETag;
							if (m_APETag->ReadFooter(buf, APE_TAG_FOOTER_BYTES) && m_APETag->GetTagSize()) {
								size_t tag_size = m_APETag->GetTagSize();
								m_pFile->Seek(file_size - tag_size);
								BYTE *p = DNew BYTE[tag_size];
								if (m_pFile->ByteRead(p, tag_size) == S_OK) {
									m_APETag->ReadTags(p, tag_size);
								}

								delete [] p;
							}

							if (m_APETag->TagItems.IsEmpty()) {
								SAFE_DELETE(m_APETag);
							}
						}

						m_pFile->Seek(cur_pos);
					}

					bIsEnd = TRUE;
				}
				break;
			default:
				m_pFile->Seek(m_pFile->GetPos() + size);
		}

		if (type == TAK_METADATA_STREAMINFO) {
			if (!ParseTAKStreamInfo(buffer, size - 3)) {
				delete [] buffer;
				return E_FAIL;
			}
			m_rtduration = m_samples * UNITS / m_samplerate;

			if (m_extradata) {
				m_extradata = (BYTE*)realloc(m_extradata, size);
			} else {
				m_extradata = (BYTE*)malloc(size);
			}
			memcpy(m_extradata, buffer, size);
			m_extrasize = size;
		} else if (type == TAK_METADATA_LAST_FRAME) {
			bLastFrame				= TRUE;
			uint64_t LastFramePos	= *(uint64_t*)buffer & 0xFFFFFFFFFF;
			uint64_t LastFrameSize	= *(uint32_t*)&buffer[4] >> 8;

			m_endpos				= LastFramePos + LastFrameSize;
		}

		SAFE_DELETE_ARRAY(buffer);
	}

	if (!bLastFrame) {
		m_endpos = m_pFile->GetLength();
	}

	if (m_startpos > m_endpos) {
		ASSERT(0);
		m_startpos = m_endpos;
	}

	m_pFile->Seek(m_startpos);

	return S_OK;
}

REFERENCE_TIME CTAKFile::Seek(REFERENCE_TIME rt)
{
#define BUFSIZE 32
#define BACKSTEP (12288 - BUFSIZE)

	if (rt <= 0) {
		m_pFile->Seek(m_startpos);
		return 0;
	}

	const int FrameNumber = (int)SCALE64(m_totalframes, rt, m_rtduration);
	__int64 pos		= m_startpos + SCALE64(m_endpos - m_startpos, rt , m_rtduration);
	__int64 start	= pos;
	__int64 end		= m_endpos - BUFSIZE;

	BYTE buf[BUFSIZE];
	int direction		= 0;
	int CurFrmNum		= -1;
	__int64 CurFrmPos	= m_endpos;

	while (pos >= m_startpos && pos < end) {
		m_pFile->Seek(pos);
		WORD sync = 0;
		m_pFile->ByteRead((BYTE*)&sync, sizeof(sync));
		if (sync == 0xA0FF) {
			m_pFile->Seek(pos);
			m_pFile->ByteRead(buf, BUFSIZE);

			int ret = GetTAKFrameNumber(buf, sizeof(buf));
			if (ret >= 0) {
				CurFrmNum = ret;
				CurFrmPos = pos;
				if (direction <= 0 && CurFrmNum > FrameNumber) {
					direction = -1;
					end = start;
					start = max(m_startpos, start - BACKSTEP);
					pos = start;
					continue;
				}
				if (direction >= 0 && CurFrmNum < FrameNumber) {
					direction = +1;
					pos = min(end, pos + 32 * (FrameNumber - CurFrmNum));
					continue;
				}
				break; // CurFrmNum == FrameNumber or direction changed
			}
		}
		pos++;
		if (pos == end && direction == -1) {
			end = start;
			start = max(m_startpos, start - BACKSTEP);
			pos = start;
		}
	}

	TRACE(L"CTAKFile::Seek() : Seek to frame number %d (%d)\n", CurFrmNum, FrameNumber);

	m_pFile->Seek(CurFrmPos);

	rt = CurFrmNum < 0 ? m_rtduration : (10000000i64 * CurFrmNum * m_framelen / m_samplerate);
	return rt;
}

int CTAKFile::GetAudioFrame(CPacket* packet, REFERENCE_TIME rtStart)
{
	if (m_pFile->GetPos() >= m_endpos) {
		return 0;
	}
	int size = min(1024, m_endpos - m_pFile->GetPos());
	packet->SetCount(size);
	m_pFile->ByteRead(packet->GetData(), size);

	packet->rtStart	= rtStart;
	packet->rtStop	= rtStart + m_nAvgBytesPerSec;

	return size;
}

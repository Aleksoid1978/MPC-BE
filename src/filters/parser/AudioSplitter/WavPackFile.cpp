/*
 * (C) 2014-2017 see Authors.txt
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
#include "WavPackFile.h"

#define WV_HEADER_SIZE 32

#define WV_FLAG_INITIAL_BLOCK (1 << 11)
#define WV_FLAG_FINAL_BLOCK   (1 << 12)

// specs say that maximum block size is 1Mb
#define WV_BLOCK_LIMIT 1048576

enum WV_FLAGS {
	WV_MONO   = 0x0004,
	WV_HYBRID = 0x0008,
	WV_JOINT  = 0x0010,
	WV_CROSSD = 0x0020,
	WV_HSHAPE = 0x0040,
	WV_FLOAT  = 0x0080,
	WV_INT32  = 0x0100,
	WV_HBR    = 0x0200,
	WV_HBAL   = 0x0400,
	WV_MCINIT = 0x0800,
	WV_MCEND  = 0x1000,
};

static const int wv_rates[16] = {
	 6000,  8000,  9600, 11025, 12000, 16000,  22050, 24000,
	32000, 44100, 48000, 64000, 88200, 96000, 192000,    -1
};

struct wv_header_t {
	uint32_t blocksize;     //< size of the block data (excluding the header)
	uint16_t version;       //< bitstream version
	uint32_t total_samples; //< total number of samples in the stream
	uint32_t block_idx;     //< index of the first sample in this block
	uint32_t samples;       //< number of samples in this block
	uint32_t flags;
	uint32_t crc;

	int initial, final;
};

struct wv_context_t {
	uint8_t block_header[WV_HEADER_SIZE];
	wv_header_t header;
	int rate, chan, bpp;
	uint32_t chmask;
	int multichannel;
	int block_parsed;
	int64_t pos;

	int64_t apetag_start;
};

bool ff_wv_parse_header(wv_header_t* wvh, const uint8_t* data)
{
	memset(wvh, 0, sizeof(*wvh));

	if (GETDWORD(data) != FCC('wvpk')) {
		return false;
	}

	wvh->blocksize     = *(uint32_t*)(data + 4);
	if (wvh->blocksize < 24 || wvh->blocksize > WV_BLOCK_LIMIT) {
		return false;
	}

	wvh->blocksize -= 24;

	wvh->version       = *(uint16_t*)(data + 8);
	wvh->total_samples = *(uint32_t*)(data + 12);
	wvh->block_idx     = *(uint32_t*)(data + 16);
	wvh->samples       = *(uint32_t*)(data + 20);
	wvh->flags         = *(uint32_t*)(data + 24);
	wvh->crc           = *(uint32_t*)(data + 28);

	wvh->initial = !!(wvh->flags & WV_FLAG_INITIAL_BLOCK);
	wvh->final   = !!(wvh->flags & WV_FLAG_FINAL_BLOCK);

	return true;
}

HRESULT wv_read_block_header(wv_context_t* wvc, CBaseSplitterFile* pFile)
{
	int rate, bpp, chan;
	uint32_t chmask, flags;

	wvc->pos = pFile->GetPos();

	// don't return bogus packets with the ape tag data
	if (wvc->apetag_start && wvc->pos >= wvc->apetag_start) {
		return E_FAIL;
	}

	if (pFile->ByteRead(wvc->block_header, WV_HEADER_SIZE) != S_OK) {
		return E_FAIL;
	}

	if (!ff_wv_parse_header(&wvc->header, wvc->block_header)) {
		DLog(L"wv_read_block_header : Invalid block header.");
		return E_FAIL;
	}

	if (wvc->header.version < 0x402 || wvc->header.version > 0x410) {
		DLog(L"wv_read_block_header : Unsupported version %03X", wvc->header.version);
		return E_FAIL;
	}

	// Blocks with zero samples don't contain actual audio information and should be ignored
	if (!wvc->header.samples) {
		return S_FALSE;
	}

	// parse flags
	flags  = wvc->header.flags;
	bpp    = ((flags & 3) + 1) << 3;
	chan   = 1 + !(flags & WV_MONO);
	chmask = flags & WV_MONO ? SPEAKER_FRONT_CENTER : SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
	rate   = wv_rates[(flags >> 23) & 0xF];
	wvc->multichannel = !(wvc->header.initial && wvc->header.final);
	if (wvc->multichannel) {
		chan   = wvc->chan;
		chmask = wvc->chmask;
	}
	if ((rate == -1 || !chan) && !wvc->block_parsed) {
		int64_t block_end = pFile->GetPos() + wvc->header.blocksize;
		while (pFile->GetPos() < block_end) {
			uint8_t id;
			int size = 0;
			pFile->ByteRead(&id, 1);
			if (id & 0x80) {
				pFile->ByteRead((BYTE*)&size, 3);
			} else {
				pFile->ByteRead((BYTE*)&size, 1);
			}
			size <<= 1;
			if (id & 0x40) {
				size--;
			}
			switch (id & 0x3F) {
			case 0xD:
				if (size <= 1) {
					DLog(L"wv_read_block_header : Insufficient channel information");
					return E_FAIL;
				}
				chan = 0;
				chmask = 0;
				pFile->ByteRead((BYTE*)&chan, 1);
				switch (size - 2) {
				case 0:
					pFile->ByteRead((BYTE*)&chmask, 1);
					break;
				case 1:
					pFile->ByteRead((BYTE*)&chmask, 2);
					break;
				case 2:
					pFile->ByteRead((BYTE*)&chmask, 3);
					break;
				case 3:
					pFile->ByteRead((BYTE*)&chmask, 4);
					break;
				case 5:
					pFile->Seek(pFile->GetPos() + 1);
					pFile->ByteRead((BYTE*)&chan + 1, 1);
					pFile->ByteRead((BYTE*)&chmask, 3);
					break;
				default:
					DLog(L"wv_read_block_header : Invalid channel info size %d", size);
					return E_FAIL;
				}
				break;
			case 0x27:
				rate = 0;
				pFile->ByteRead((BYTE*)&rate, 3);
				break;
			default:
				pFile->Seek(pFile->GetPos() + size);
			}
			if (id & 0x40) {
				pFile->Seek(pFile->GetPos() + 1);
			}
		}
		if (rate == -1) {
			DLog(L"wv_read_block_header : Cannot determine custom sampling rate");
			return E_FAIL;
		}
		pFile->Seek(block_end - wvc->header.blocksize);
	}
	if (!wvc->bpp)
		wvc->bpp    = bpp;
	if (!wvc->chan)
		wvc->chan   = chan;
	if (!wvc->chmask)
		wvc->chmask = chmask;
	if (!wvc->rate)
		wvc->rate   = rate;

	if (flags && bpp != wvc->bpp) {
		DLog(L"wv_read_block_header : Bits per sample differ, this block: %d, header block: %d", bpp, wvc->bpp);
		return E_FAIL;
	}
	if (flags && !wvc->multichannel && chan != wvc->chan) {
		DLog(L"wv_read_block_header : Channels differ, this block: %d, header block: %d", chan, wvc->chan);
		return E_FAIL;
	}
	if (flags && rate != -1 && rate != wvc->rate) {
		DLog(L"wv_read_block_header : Sampling rate differ, this block: %d, header block: %d", rate, wvc->rate);
		return E_FAIL;
	}

	return S_OK;
}

//
// CWavPackFile
//

CWavPackFile::CWavPackFile()
	: CAudioFile()
	, m_block_idx_start(0)
	, m_block_idx_end(0)
	, m_APETag(nullptr)
{
	m_subtype = MEDIASUBTYPE_WAVPACK4;
	m_wFormatTag = 0x5756;
}

CWavPackFile::~CWavPackFile()
{
	SAFE_DELETE(m_APETag);
}

void CWavPackFile::SetProperties(IBaseFilter* pBF)
{
	if (m_APETag) {
		SetAPETagProperties(pBF, m_APETag);
	}
}

HRESULT CWavPackFile::Open(CBaseSplitterFile* pFile)
{
	m_pFile = pFile;
	m_pFile->Seek(0);

	wv_context_t wv_ctx;
	memset(&wv_ctx, 0, sizeof(wv_ctx));

	wv_ctx.block_parsed = 0;
	for (;;) {
		HRESULT hr = wv_read_block_header(&wv_ctx, m_pFile);
		if (FAILED(hr)) {
			return hr;
		}
		if (hr == S_OK) {
			break;
		}
		m_pFile->Seek(m_pFile->GetPos() + wv_ctx.header.blocksize);
	}

	m_startpos = wv_ctx.pos;
	m_block_idx_start = wv_ctx.header.block_idx;
	m_block_idx_end = wv_ctx.header.total_samples;

	m_endpos = m_pFile->GetLength();

	if (m_pFile->IsRandomAccess()) {
		if (m_startpos + 128 <= m_endpos) {
			m_pFile->Seek(m_endpos - 128);
			BYTE buf[128];
			if (m_pFile->ByteRead(buf, 128) == S_OK
				&& (memcmp(buf + 96, "APETAGEX", 8) || memcmp(buf + 120, "\0\0\0\0\0\0\0\0", 8))
				&& memcmp(buf, "TAG", 3) == 0) {
				// ignore ID3v1/1.1 tag
				m_endpos -= 128;
			}
		}

		if (m_startpos + WV_HEADER_SIZE + APE_TAG_FOOTER_BYTES < m_endpos) {
			BYTE buf[APE_TAG_FOOTER_BYTES];
			memset(buf, 0, sizeof(buf));

			m_pFile->Seek(m_endpos - APE_TAG_FOOTER_BYTES);
			if (m_pFile->ByteRead(buf, APE_TAG_FOOTER_BYTES) == S_OK) {
				m_APETag = DNew CAPETag;
				size_t tag_size = 0;
				if (m_APETag->ReadFooter(buf, APE_TAG_FOOTER_BYTES) && m_APETag->GetTagSize()) {
					tag_size = m_APETag->GetTagSize();
					m_pFile->Seek(m_endpos - tag_size);
					BYTE *p = DNew BYTE[tag_size];
					if (m_pFile->ByteRead(p, tag_size) == S_OK) {
						m_APETag->ReadTags(p, tag_size);
					}
					delete[] p;
				}

				m_endpos -= tag_size;

				if (m_APETag->TagItems.empty()) {
					SAFE_DELETE(m_APETag);
				}
			}
		}

		m_pFile->Seek(std::max(m_startpos + WV_HEADER_SIZE, m_endpos - WV_BLOCK_LIMIT));

		wv_header_t wv_header;
		uint8_t block_header[WV_HEADER_SIZE];
		__int64 pos = m_pFile->GetPos();
		while (pos < m_endpos && pFile->ByteRead(block_header, WV_HEADER_SIZE) == S_OK) {
			if (ff_wv_parse_header(&wv_header, block_header)) {
				if (wv_header.samples && wv_header.final) {
					m_block_idx_end = wv_header.block_idx + wv_header.samples;
				}
				pos = m_pFile->GetPos() + wv_header.blocksize;
			}
			else {
				pos++;
			}
			m_pFile->Seek(pos);
		}

		if (m_block_idx_end == m_block_idx_start) {
			m_block_idx_end = m_block_idx_start + wv_ctx.header.samples;
			// TODO: make it more correct
		}
	}

	if (m_block_idx_start >= m_block_idx_end) {
		return E_FAIL;
	}

	m_samplerate = wv_ctx.rate;
	m_bitdepth   = wv_ctx.bpp;
	m_channels   = wv_ctx.chan;
	m_layout     = wv_ctx.chmask;

	m_rtduration = 10000000i64 * (m_block_idx_end - m_block_idx_start) / m_samplerate;

	m_extrasize = sizeof(uint16_t);
	m_extradata = (BYTE*)malloc(m_extrasize);
	memcpy(m_extradata, &wv_ctx.header.version, m_extrasize);

	m_pFile->Seek(m_startpos);

	return S_OK;
}

REFERENCE_TIME CWavPackFile::Seek(REFERENCE_TIME rt)
{
#define BUFSIZE WV_HEADER_SIZE
#define BACKSTEP (12288 - BUFSIZE)

	if (rt <= 0) {
		m_pFile->Seek(m_startpos);
		return 0;
	}

	const uint32_t BlockIndex = (int)SCALE64(m_block_idx_end - m_block_idx_start, rt, m_rtduration);
	__int64 pos		= m_startpos + SCALE64(m_endpos - m_startpos, rt , m_rtduration);
	__int64 start	= pos;
	__int64 end		= m_endpos - BUFSIZE;

	BYTE buf[BUFSIZE];
	int direction			= 0;
	uint32_t CurBlockIdx	= -1;
	__int64 CurFrmPos		= m_endpos;

	while (pos >= m_startpos && pos < end) {
		m_pFile->Seek(pos);
		DWORD sync = 0;
		m_pFile->ByteRead((BYTE*)&sync, sizeof(sync));
		if (sync == FCC('wvpk')) {
			m_pFile->Seek(pos);
			m_pFile->ByteRead(buf, BUFSIZE);

			wv_header_t wv_header;
			if (ff_wv_parse_header(&wv_header, buf)) {
				CurFrmPos = pos;
				if (direction <= 0 && wv_header.block_idx > BlockIndex) {
					direction = -1;
					end = start;
					start = std::max(m_startpos, start - BACKSTEP);
					pos = start;
					continue;
				}
				if (direction >= 0 && (wv_header.block_idx + wv_header.samples) < BlockIndex) {
					direction = +1;
					pos = std::min(end, pos + WV_HEADER_SIZE + wv_header.blocksize);
					continue;
				}
				CurBlockIdx = wv_header.block_idx;
				break; // CurFrmNum == FrameNumber or direction changed
			}
		}
		pos++;
		if (pos == end && direction == -1) {
			end = start;
			start = std::max(m_startpos, start - BACKSTEP);
			pos = start;
		}
	}

	if (CurBlockIdx == -1 || CurBlockIdx < m_block_idx_start) {
		m_pFile->Seek(m_startpos);
		return 0;
	}

	DLog(L"CWavPackFile::Seek() : Seek to frame index %u (%u)", CurBlockIdx, BlockIndex);

	m_pFile->Seek(CurFrmPos);

	rt = 10000000i64 * (CurBlockIdx - m_block_idx_start) / m_samplerate;
	return rt;
}

int CWavPackFile::GetAudioFrame(CPacket* packet, REFERENCE_TIME rtStart)
{
	if (m_pFile->GetPos() >= m_endpos) {
		return 0;
	}

	wv_header_t wv_header;
	BYTE buf[WV_HEADER_SIZE];
	packet->SetCount(0);

	do {
		wv_header.samples = 0;

		while (m_pFile->ByteRead(buf, WV_HEADER_SIZE) == S_OK && ff_wv_parse_header(&wv_header, buf) && wv_header.samples == 0) {
			m_pFile->Seek(m_pFile->GetPos() + wv_header.blocksize);
		}

		size_t offset = packet->GetCount();

		if (wv_header.samples
				&& packet->SetCount(offset + WV_HEADER_SIZE + wv_header.blocksize)
				&& m_pFile->ByteRead(packet->GetData() + offset + WV_HEADER_SIZE, wv_header.blocksize) == S_OK) {
			memcpy(packet->GetData() + offset, buf, WV_HEADER_SIZE);
		} else {
			return 0;
		}
	} while (!wv_header.final);

	packet->rtStart	= (wv_header.block_idx - m_block_idx_start) *  10000000i64 / m_samplerate;
	packet->rtStop	= (wv_header.block_idx - m_block_idx_start + wv_header.samples) * 10000000i64 / m_samplerate;

	return packet->GetCount();
}

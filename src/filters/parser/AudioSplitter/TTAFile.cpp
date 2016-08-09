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
#include "TTAFile.h"

struct tta_header_t {
	uint16_t flags;
	uint16_t channels;
	uint16_t bps;
	uint32_t samplerate;
	uint32_t nb_samples;
	uint32_t crc;
};

//
// CTTAFile
//

CTTAFile::CTTAFile()
	: CAudioFile()
	, m_totalframes(0)
	, m_currentframe(0)
	, m_framesamples(0)
	, m_last_framesamples(0)
	, m_APETag(NULL)
{
	m_subtype = MEDIASUBTYPE_TTA1;
	m_wFormatTag = 0x77a1;
}

CTTAFile::~CTTAFile()
{
	SAFE_DELETE(m_APETag);
}

void CTTAFile::SetProperties(IBaseFilter* pBF)
{
	if (m_APETag) {
		SetAPETagProperties(pBF, m_APETag);
	}
}

HRESULT CTTAFile::Open(CBaseSplitterFile* pFile)
{
	m_pFile = pFile;
	m_pFile->Seek(0);

	BYTE data[10];
	if (m_pFile->ByteRead(data, sizeof(data)) != S_OK) {
		return NULL;
	}
	int start_offset = id3v2_match_len(data);
	m_pFile->Seek(start_offset);

	DWORD id = 0;
	if (m_pFile->ByteRead((BYTE*)&id, 4) != S_OK || id != FCC('TTA1')) {
		return E_FAIL;
	}

	tta_header_t tta;
	memset(&tta, 0, sizeof(tta));

	m_pFile->ByteRead((BYTE*)&tta.flags,      2);
	m_pFile->ByteRead((BYTE*)&tta.channels,   2);
	m_pFile->ByteRead((BYTE*)&tta.bps,        2);
	m_pFile->ByteRead((BYTE*)&tta.samplerate, 4);
	m_pFile->ByteRead((BYTE*)&tta.nb_samples, 4);
	m_pFile->ByteRead((BYTE*)&tta.crc,        4);

	if (tta.samplerate == 0 || tta.samplerate > 1000000 || !tta.nb_samples) {
		return E_FAIL;
	}

	m_framesamples		= tta.samplerate * 256 / 245;
	m_last_framesamples	= tta.nb_samples % m_framesamples;
	if (!m_last_framesamples) {
		m_last_framesamples = m_framesamples;
	}
	m_totalframes		= tta.nb_samples / m_framesamples + (m_last_framesamples < m_framesamples);
	m_currentframe		= 0;

	if(m_totalframes >= UINT_MAX/sizeof(uint32_t) || m_totalframes <= 0) {
		DLog(L"CTTAile::Open() : totalframes %d invalid", m_totalframes);
		return E_FAIL;
	}

	if (!m_index.SetCount(m_totalframes)) {
		return E_OUTOFMEMORY;
	}

	__int64 framepos = m_pFile->GetPos() + 4 * m_totalframes + 4;

	m_extrasize = m_pFile->GetPos() - start_offset;
	m_extradata = (BYTE*)malloc(m_extrasize);
	m_pFile->Seek(start_offset);
	if (m_pFile->ByteRead(m_extradata, m_extrasize) != S_OK) {
		return E_FAIL;
	}

	for (int i = 0; i < m_totalframes; i++) {
		uint32_t size;
		if (m_pFile->ByteRead((BYTE*)&size, 4) != S_OK) {
			return E_FAIL;
		}
		m_index[i] = framepos;
		framepos += size;
	}

	m_startpos = m_pFile->GetPos();
	m_endpos   = min(m_index[m_totalframes - 1], m_pFile->GetLength());

	const __int64 file_size = m_pFile->GetLength();
	if (m_endpos + APE_TAG_FOOTER_BYTES < file_size) {
		BYTE buf[APE_TAG_FOOTER_BYTES];
		memset(buf, 0, sizeof(buf));

		m_pFile->Seek(file_size - APE_TAG_FOOTER_BYTES);
		if (m_pFile->ByteRead(buf, APE_TAG_FOOTER_BYTES) == S_OK) {
			m_APETag = DNew CAPETag;
			size_t tag_size = 0;
			if (m_APETag->ReadFooter(buf, APE_TAG_FOOTER_BYTES) && m_APETag->GetTagSize()) {
				tag_size = m_APETag->GetTagSize();
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
	}

	m_samplerate	= tta.samplerate;
	m_bitdepth		= tta.bps;
	m_channels		= tta.channels;
	if (m_channels == 1) {
		m_layout = SPEAKER_FRONT_CENTER;
	} else if (m_channels == 2) {
		m_layout = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
	}
	m_rtduration	= 10000000i64 * tta.nb_samples / m_samplerate;


	m_pFile->Seek(m_startpos);

	return S_OK;
}

REFERENCE_TIME CTTAFile::Seek(REFERENCE_TIME rt)
{
	if (rt <= 0) {
		m_currentframe = 0;
		m_pFile->Seek(m_index[0]);
		return 0;
	}

	int64_t pts = rt * m_samplerate / 10000000;

	m_currentframe = pts / m_framesamples;
	pts = m_currentframe * m_framesamples;

	m_pFile->Seek(m_index[m_currentframe]);
	rt = pts * 10000000 / m_samplerate;

	return rt;
}

int CTTAFile::GetAudioFrame(CPacket* packet, REFERENCE_TIME rtStart)
{
	if (m_currentframe >= m_totalframes) {
		return 0;
	}

	packet->rtStart = 10000000i64 * m_currentframe * m_framesamples / m_samplerate;

	int size;
	if (m_currentframe == m_totalframes - 1) {
		size = m_endpos - m_index[m_currentframe];
		packet->rtStop  = 10000000i64 * (m_currentframe * m_framesamples + m_last_framesamples) / m_samplerate;
	} else {
		size = m_index[m_currentframe + 1] -  m_index[m_currentframe];
		packet->rtStop  = 10000000i64 * (m_currentframe + 1) * m_framesamples / m_samplerate;
	}

	m_pFile->Seek(m_index[m_currentframe]);
	m_currentframe++;

	if (!packet->SetCount(size) || m_pFile->ByteRead(packet->GetData(), size) != S_OK) {
		return 0;
	}

	return size;
}

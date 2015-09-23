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
#include "DSFFile.h"

//
// CDSFFile
//


CDSFFile::CDSFFile()
	: CAudioFile()
{
}

CDSFFile::~CDSFFile()
{
	SAFE_DELETE(m_ID3Tag);
}

HRESULT CDSFFile::Open(CBaseSplitterFile* pFile)
{
	m_pFile = pFile;

	m_pFile->Seek(0);
	UINT32 type32 = 0;
	UINT64 type64 = 0;
	if (m_pFile->ByteRead((BYTE*)&type32, 4) != S_OK || type32 != FCC('DSD ')) {
		return E_FAIL;
	}
	if (m_pFile->ByteRead((BYTE*)&type64, 8) != S_OK || type64 != 0x1C) {
		return E_FAIL;
	}

	m_pFile->Skip(8);

	type64 = 0;
	if (m_pFile->ByteRead((BYTE*)&type64, 8) != S_OK) {
		return E_FAIL;
	}
	UINT64 id3pos = type64;
	if (id3pos && m_pFile->IsRandomAccess()) {
		__int64 pos = m_pFile->GetPos();
		m_pFile->Seek(id3pos);

		if (m_pFile->BitRead(24) == 'ID3') {
			BYTE major = (BYTE)m_pFile->BitRead(8);
			BYTE revision = (BYTE)m_pFile->BitRead(8);
			UNREFERENCED_PARAMETER(revision);

			BYTE flags = (BYTE)m_pFile->BitRead(8);

			DWORD size = m_pFile->BitRead(32);
			size = (((size & 0x7F000000) >> 0x03) |
					((size & 0x007F0000) >> 0x02) |
					((size & 0x00007F00) >> 0x01) |
					((size & 0x0000007F)		));
			if (major <= 4) {
				BYTE* buf = DNew BYTE[size];
				m_pFile->ByteRead(buf, size);

				m_ID3Tag = DNew CID3Tag(major, flags);
				m_ID3Tag->ReadTagsV2(buf, size);
				delete [] buf;
			}
		}

		m_pFile->Seek(pos);
	}

	type32 = 0;
	if (m_pFile->ByteRead((BYTE*)&type32, 4) != S_OK || type32 != FCC('fmt ')) {
		return E_FAIL;
	}

	type64 = 0;
	if (m_pFile->ByteRead((BYTE*)&type64, 8) != S_OK || type64 != 0x34) {
		return E_FAIL;
	}

	type32 = 0;
	if (m_pFile->ByteRead((BYTE*)&type32, 4) != S_OK || type32 != 1) {
		return E_FAIL;
	}

	type32 = 0;
	if (m_pFile->ByteRead((BYTE*)&type32, 4) != S_OK || type32) {
		return E_FAIL;
	}

	type32 = 0;
	if (m_pFile->ByteRead((BYTE*)&type32, 4) != S_OK) {
		return E_FAIL;
	}

	UINT32 channel_type = type32;

	type32 = 0;
	if (m_pFile->ByteRead((BYTE*)&type32, 4) != S_OK) {
		return E_FAIL;
	}
	m_channels = type32;

	type32 = 0;
	if (m_pFile->ByteRead((BYTE*)&type32, 4) != S_OK) {
		return E_FAIL;
	}
	m_samplerate = type32 / 8; // hack for ffmpeg decoder
	m_bitdepth = 8; // hack for ffmpeg decoder

	type32 = 0;
	if (m_pFile->ByteRead((BYTE*)&type32, 4) != S_OK) {
		return E_FAIL;
	}

	UINT32 codec = type32;
	switch(codec) {
		case 1:
			m_subtype = MEDIASUBTYPE_DSD1;
			break;
		case 8:
			m_subtype = MEDIASUBTYPE_DSD8;
			break;
		default:
			return E_FAIL;
	}

	m_pFile->Skip(8);

	type32 = 0;
	if (m_pFile->ByteRead((BYTE*)&type32, 4) != S_OK || type32 > INT_MAX / (UINT32)m_channels) {
		return E_FAIL;
	}
	m_block_size = type32 * m_channels;
	m_nAvgBytesPerSec = m_samplerate * m_channels;

	m_pFile->Skip(4);

	type32 = 0;
	if (m_pFile->ByteRead((BYTE*)&type32, 4) != S_OK || type32 != FCC('data')) {
		return E_FAIL;
	}

	type64 = 0;
	if (m_pFile->ByteRead((BYTE*)&type64, 8) != S_OK) {
		return E_FAIL;
	}
	m_length = type64;

	m_length		-= m_length % m_block_size;
	m_startpos		= m_pFile->GetPos();
	m_endpos		= m_startpos + m_length;
	m_rtduration	= 10000000i64 * m_length / m_nAvgBytesPerSec;

	return S_OK;
}

bool CDSFFile::SetMediaType(CMediaType& mt)
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

void CDSFFile::SetProperties(IBaseFilter* pBF)
{
	if (m_ID3Tag) {
		SetID3TagProperties(pBF, m_ID3Tag);
	}
}


REFERENCE_TIME CDSFFile::Seek(REFERENCE_TIME rt)
{
	if (rt <= 0) {
		m_pFile->Seek(m_startpos);
		return 0;
	}

	__int64 len = SCALE64(m_length, rt , m_rtduration);
	len -= len % m_block_size;
	m_pFile->Seek(m_startpos + len);

	rt = 10000000i64 * len / m_nAvgBytesPerSec;

	return rt;
}

int CDSFFile::GetAudioFrame(CPacket* packet, REFERENCE_TIME rtStart)
{
	if (m_pFile->GetPos() + m_block_size > m_endpos) {
		return 0;
	}

	int size = min(m_block_size, m_endpos - m_pFile->GetPos());
	if (!packet->SetCount(size) || m_pFile->ByteRead(packet->GetData(), size) != S_OK) {
		return 0;
	}

	__int64 len = m_pFile->GetPos() - m_startpos;
	packet->rtStart	= SCALE64(m_rtduration, len, m_length);
	packet->rtStop	= SCALE64(m_rtduration, (len + size), m_length);

	return size;
}

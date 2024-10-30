/*
 * (C) 2014-2023 see Authors.txt
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
#include "DSUtil/AudioParser.h"
#include "WAVFile.h"

#pragma pack(push, 1)
typedef union {
	struct {
		DWORD id;
		DWORD size;
	};
	BYTE data[8];
} chunk_t;
#pragma pack(pop)

//
// CWAVFile
//

CWAVFile::~CWAVFile()
{
}

bool CWAVFile::ProcessWAVEFORMATEX()
{
	if (!m_fmtdata || m_fmtsize < sizeof(WAVEFORMATEX)) {
		return false;
	}

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_fmtdata.get();
	if (wfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE && m_fmtsize >= sizeof(WAVEFORMATEXTENSIBLE)) {
		WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)m_fmtdata.get();
		m_subtype = wfex->SubFormat;
		if (CountBits(wfex->dwChannelMask) != wfex->Format.nChannels) {
			// fix incorrect dwChannelMask
			wfex->dwChannelMask = GetDefChannelMask(wfex->Format.nChannels);
		}
	} else {
		m_subtype = FOURCCMap(wfe->wFormatTag);
	}

	if (wfe->wFormatTag == WAVE_FORMAT_PCM && (wfe->nChannels > 2 || wfe->wBitsPerSample != 8 && wfe->wBitsPerSample != 16)
			|| wfe->wFormatTag == WAVE_FORMAT_IEEE_FLOAT && wfe->nChannels > 2) {
		// convert incorrect WAVEFORMATEX to WAVEFORMATEXTENSIBLE

		WAVEFORMATEXTENSIBLE wfex;
		wfex.Format.wFormatTag           = WAVE_FORMAT_EXTENSIBLE;
		wfex.Format.nChannels            = wfe->nChannels;
		wfex.Format.nSamplesPerSec       = wfe->nSamplesPerSec;
		wfex.Format.wBitsPerSample       = (wfe->wBitsPerSample + 7) & ~7;
		wfex.Format.nBlockAlign          = wfex.Format.nChannels * wfex.Format.wBitsPerSample / 8;
		wfex.Format.nAvgBytesPerSec      = wfex.Format.nSamplesPerSec * wfex.Format.nBlockAlign;
		wfex.Format.cbSize               = 22;
		wfex.Samples.wValidBitsPerSample = wfe->wBitsPerSample;
		wfex.dwChannelMask               = GetDefChannelMask(wfe->nChannels);
		wfex.SubFormat                   = m_subtype;

		m_fmtsize = sizeof(WAVEFORMATEXTENSIBLE);
		m_fmtdata.reset(DNew BYTE[m_fmtsize]);
		memcpy(m_fmtdata.get(), &wfex, m_fmtsize);

		wfe = (WAVEFORMATEX*)m_fmtdata.get();
	}

	m_samplerate		= wfe->nSamplesPerSec;
	m_bitdepth			= wfe->wBitsPerSample;
	m_channels			= wfe->nChannels;
	if (wfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE && m_fmtsize >= sizeof(WAVEFORMATEXTENSIBLE)) {
		m_layout = ((WAVEFORMATEXTENSIBLE*)wfe)->dwChannelMask;
	} else {
		m_layout = GetDefChannelMask(wfe->nChannels);
	}
	m_nAvgBytesPerSec	= wfe->nAvgBytesPerSec;
	m_nBlockAlign		= wfe->nBlockAlign;

	m_blocksize			= std::max((DWORD)m_nBlockAlign, m_nAvgBytesPerSec / 16); // 62.5 ms
	m_blocksize			-= m_blocksize % m_nBlockAlign;

	return true;
}

bool CWAVFile::CheckDTSAC3CD()
{
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_fmtdata.get();
	if (m_fmtsize != sizeof(WAVEFORMATEX)
			|| wfe->wFormatTag != WAVE_FORMAT_PCM
			|| wfe->nChannels != 2
			|| wfe->nSamplesPerSec != 44100 && wfe->nSamplesPerSec != 48000
			|| wfe->nBlockAlign != 4
			|| wfe->nAvgBytesPerSec != wfe->nSamplesPerSec * wfe->nBlockAlign
			|| wfe->wBitsPerSample != 16
			|| wfe->cbSize != 0) {
		return false;
	}

	size_t buflen = std::min(64LL * 1024, m_length);
	BYTE* buffer = DNew BYTE[buflen];

	m_pFile->Seek(m_startpos);
	if (m_pFile->ByteRead(buffer, buflen) == S_OK) {
		audioframe_t aframe;

		for (size_t i = 0; i + 16 < buflen; i++) {
			if (ParseDTSHeader(buffer + i, &aframe)) {
				m_subtype		= MEDIASUBTYPE_DTS2;
				m_samplerate	= aframe.samplerate;
				m_channels		= aframe.channels;
				m_nBlockAlign	= aframe.size;
				m_blocksize		= aframe.size;
				break;
			}
			if (int ret = ParseAC3IEC61937Header(buffer + i)) {
				m_subtype = MEDIASUBTYPE_DOLBY_AC3_SPDIF;
				m_blocksize		= ret;
				break;
			}
		}
	}

	SAFE_DELETE_ARRAY(buffer);

	if (m_subtype == MEDIASUBTYPE_DTS2) {
		wfe->wFormatTag = WAVE_FORMAT_DTS2;
		wfe->nChannels		= m_channels;
		wfe->nSamplesPerSec	= m_samplerate;
		wfe->nBlockAlign	= m_nBlockAlign;
	} else if (m_subtype == MEDIASUBTYPE_DOLBY_AC3_SPDIF) {
		wfe->wFormatTag = WAVE_FORMAT_DOLBY_AC3_SPDIF;
	} else {
		return false;
	}

	return true;
}

bool CWAVFile::SetMediaType(CMediaType& mt)
{
	if (!m_fmtdata || !m_fmtsize) {
		return false;
	}

	mt.majortype	= MEDIATYPE_Audio;
	mt.formattype	= FORMAT_WaveFormatEx;
	mt.subtype		= m_subtype;
	mt.SetSampleSize(m_blocksize);

	memcpy(mt.AllocFormatBuffer(m_fmtsize), m_fmtdata.get(), m_fmtsize);

	return true;
}

void CWAVFile::SetProperties(IBaseFilter* pBF)
{
	if (m_info.size() > 0) {
		if (CComQIPtr<IDSMPropertyBag> pPB = pBF) {
			for (const auto& [tag, value] : m_info) {
				switch (tag) {
				case FCC('INAM'):
					pPB->SetProperty(L"TITL", CString(value));
					break;
				case FCC('IART'):
					pPB->SetProperty(L"AUTH", CString(value));
					break;
				case FCC('ICOP'):
					pPB->SetProperty(L"CPYR", CString(value));
					break;
				case FCC('ISBJ'):
					pPB->SetProperty(L"DESC", CString(value));
					break;
				}
			}
		}
	}

	__super::SetProperties(pBF);

	if (!m_chapters.empty()) {
		if (CComQIPtr<IDSMChapterBag> pCB = pBF) {
			pCB->ChapRemoveAll();
			for (const auto& chap : m_chapters) {
				pCB->ChapAppend(chap.rt, chap.name);
			}
		}
	}
}

HRESULT CWAVFile::Open(CBaseSplitterFile* pFile)
{
	m_pFile = pFile;
	m_startpos = 0;

	m_pFile->Seek(0);
	BYTE data[12];
	if (m_pFile->ByteRead(data, sizeof(data)) != S_OK
			|| GETU32(data+0) != FCC('RIFF')
			|| GETU32(data+4) < (4 + 8 + sizeof(PCMWAVEFORMAT) + 8) // 'WAVE' + ('fmt ' + fmt.size) + sizeof(PCMWAVEFORMAT) + (data + data.size)
			|| GETU32(data+8) != FCC('WAVE')) {
		return E_FAIL;
	}
	__int64 end = std::min((__int64)GETU32(data + 4) + 8, m_pFile->GetLength());

	chunk_t Chunk;

	while (m_pFile->ByteRead(Chunk.data, sizeof(Chunk)) == S_OK && m_pFile->GetPos() < end) {
		__int64 pos = m_pFile->GetPos();

		DLog(L"CWAVFile::Open() : found '%c%c%c%c' chunk.",
			WCHAR((Chunk.id>>0)&0xff),
			WCHAR((Chunk.id>>8)&0xff),
			WCHAR((Chunk.id>>16)&0xff),
			WCHAR((Chunk.id>>24)&0xff));

		switch (Chunk.id) {
		case FCC('fmt '):
			if (m_fmtdata || Chunk.size < sizeof(PCMWAVEFORMAT) || Chunk.size > 65536) {
				DLog(L"CWAVFile::Open() : bad format");
				return E_FAIL;
			}
			m_fmtsize = std::max(Chunk.size, (DWORD)sizeof(WAVEFORMATEX)); // PCMWAVEFORMAT to WAVEFORMATEX
			m_fmtdata.reset(DNew BYTE[m_fmtsize]);
			memset(m_fmtdata.get(), 0, m_fmtsize);
			if (m_pFile->ByteRead(m_fmtdata.get(), Chunk.size) != S_OK) {
				DLog(L"CWAVFile::Open() : format can not be read.");
				return E_FAIL;
			}
			break;
		case FCC('data'):
			if (m_endpos) {
				DLog(L"CWAVFile::Open() : bad format");
				return E_FAIL;
			}
			m_startpos	= pos;
			m_length	= std::min((__int64)Chunk.size, m_pFile->GetLength() - m_startpos);
			if (!m_pFile->IsRandomAccess()) {
				goto stop;
			}
			break;
		case FCC('LIST'):
			ReadRIFFINFO(Chunk.size);
			break;
		case FCC('wavl'): // not supported
		case FCC('slnt'): // not supported
			DLog(L"CWAVFile::Open() : WAVE file is not supported!");
			return E_FAIL;
		case FCC('ID3 '):
		case FCC('id3 '):
			ReadID3Tag(Chunk.size);
			break;
		case FCC('list'):
			ReadADTLTag(Chunk.size);
			break;
		case FCC('cue '):
			ReadCueTag(Chunk.size);
			break;
		default:
			for (int i = 0; i < sizeof(Chunk.id); i++) {
				BYTE ch = Chunk.data[i];
				if (ch != 0x20 && (ch < '0' || ch > '9') && (ch < 'A' || ch > 'Z') && (ch < 'a' || ch > 'z') && ch != '_') {
					DLog(L"CWAVFile::Open() : broken file!");
					return E_FAIL;
				}
			}
			// Known chunks:
			// 'fact'
			// 'JUNK'
			// 'PAD '
			// 'plst'
			// 'PEAK'
			// 'smpl'
			// 'inst'
			// 'bext'
			// 'minf'
			// 'elm1'
			break;
		}

		if (Chunk.id == FCC('data')) {
			// HACK: fix end of data chunk size for find correct offset of the next chunk.
			// need the official information about this
			if ((pos + Chunk.size) & 1) {
				// next chunk will start with an even offset
				Chunk.size++;
			}
		}

		m_pFile->Seek(pos + Chunk.size);
	}
	stop:

	if (!m_startpos || !ProcessWAVEFORMATEX()) {
		return E_FAIL;
	}

	if (!m_pFile->IsStreaming()) {
		m_length -= m_length % m_nBlockAlign;
		m_endpos = m_startpos + m_length;
		m_rtduration = 10000000i64 * m_length / m_nAvgBytesPerSec;
	} else {
		m_frameDuration = 10000000i64 * m_blocksize / m_nAvgBytesPerSec;
	}

	CheckDTSAC3CD();

	return S_OK;
}

REFERENCE_TIME CWAVFile::Seek(REFERENCE_TIME rt)
{
	if (m_pFile->IsStreaming()) {
		return rt;
	}

	if (rt <= 0) {
		m_pFile->Seek(m_startpos);
		return 0;
	}

	__int64 len = SCALE64(m_length, rt , m_rtduration);
	len -= len % m_nBlockAlign;
	m_pFile->Seek(m_startpos + len);

	rt = 10000000i64 * len / m_nAvgBytesPerSec;

	return rt;
}

int CWAVFile::GetAudioFrame(CPacket* packet, REFERENCE_TIME rtStart)
{
	if (m_pFile->IsStreaming()) {
		if (!packet->SetCount(m_blocksize) || m_pFile->ByteRead(packet->data(), m_blocksize) != S_OK) {
			return 0;
		}

		packet->rtStart = m_rtPrev;
		packet->rtStop = m_rtPrev + m_frameDuration;

		m_rtPrev = packet->rtStop;

		return m_blocksize;
	}

	const __int64 start = m_pFile->GetPos();

	if (start + m_nBlockAlign > m_endpos) {
		return 0;
	}

	int size = (int)std::min((__int64)m_blocksize, m_endpos - start);
	if (!packet->SetCount(size) || m_pFile->ByteRead(packet->data(), size) != S_OK) {
		return 0;
	}

	__int64 len = start - m_startpos;
	packet->rtStart = SCALE64(m_rtduration, len, m_length);
	packet->rtStop  = SCALE64(m_rtduration, (len + size), m_length);

	return size;
}

HRESULT CWAVFile::ReadRIFFINFO(const DWORD chunk_size)
{
	m_info.clear();
	DWORD id = 0;

	if (chunk_size < 4 || m_pFile->ByteRead((BYTE*)&id, 4) != S_OK || id != FCC('INFO')) {
		return E_FAIL;
	}

	const __int64 chunk_pos = m_pFile->GetPos();
	const __int64 end = chunk_pos + chunk_size;
	chunk_t Chunk;

	while (m_pFile->GetPos() + 8 < end && m_pFile->ByteRead(Chunk.data, sizeof(Chunk)) == S_OK && m_pFile->GetPos() + Chunk.size <= end) {
		__int64 pos = m_pFile->GetPos();

		if (Chunk.size > 0 && (Chunk.id == FCC('INAM') || Chunk.id == FCC('IART') || Chunk.id == FCC('ICOP') || Chunk.id == FCC('ISBJ'))) {
			CStringA value;
			if (S_OK != m_pFile->ByteRead((BYTE*)value.GetBufferSetLength(Chunk.size), Chunk.size)) {
				return S_FALSE;
			}
			m_info[Chunk.id] = value;
		}

		m_pFile->Seek(pos + Chunk.size);
	}

	return S_OK;
}

HRESULT CWAVFile::ReadADTLTag(const DWORD chunk_size)
{
	DWORD id = 0;

	if (m_chapters.empty() || chunk_size < 4 || m_pFile->ByteRead((BYTE*)&id, 4) != S_OK || id != FCC('adtl')) {
		return E_FAIL;
	}

	const __int64 chunk_pos = m_pFile->GetPos();
	const __int64 end = chunk_pos + chunk_size;
	chunk_t Chunk;

	while (m_pFile->GetPos() + 8 < end && m_pFile->ByteRead(Chunk.data, sizeof(Chunk)) == S_OK && m_pFile->GetPos() + Chunk.size <= end) {
		__int64 pos = m_pFile->GetPos();

		if (Chunk.size >= 5 && Chunk.id == FCC('labl')) {
			if (m_pFile->ByteRead((BYTE*)&id, 4) != S_OK) {
				return S_FALSE;
			}
			CStringA value;
			if (S_OK != m_pFile->ByteRead((BYTE*)value.GetBufferSetLength(Chunk.size - 4), Chunk.size - 4)) {
				return S_FALSE;
			}
			m_pFile->Skip(m_pFile->GetPos() & 1);

			for (auto& chap : m_chapters) {
				if (chap.id == id) {
					chap.name = UTF8ToWStr(value);
				}
			}
		}

		m_pFile->Seek(pos + Chunk.size);
	}

	return S_OK;
}

HRESULT CWAVFile::ReadCueTag(const DWORD chunk_size)
{
	if (!m_fmtdata || m_fmtsize < sizeof(WAVEFORMATEX)) {
		return E_FAIL;
	}

	const auto wfe = (WAVEFORMATEX*)m_fmtdata.get();
	if (chunk_size > 4 && wfe->nSamplesPerSec) {
		const auto& samplerate = wfe->nSamplesPerSec;
		unsigned cues;
		if (m_pFile->ByteRead((BYTE*)&cues, 4) != S_OK) {
			return S_FALSE;
		}

		if (chunk_size >= cues * 24UL + 4UL) {
			for (unsigned i = 0; i < cues; i++) {
				DWORD id;
				if (m_pFile->ByteRead((BYTE*)&id, 4) != S_OK) {
					return S_FALSE;
				}
				m_pFile->Skip(16);
				DWORD offset;
				if (m_pFile->ByteRead((BYTE*)&offset, 4) != S_OK) {
					return S_FALSE;
				}

				const auto rt = llMulDiv(offset, UNITS, samplerate, 0);
				m_chapters.emplace_back(L"", rt, id);
			}

			return S_OK;
		}
	}

	return E_FAIL;
}

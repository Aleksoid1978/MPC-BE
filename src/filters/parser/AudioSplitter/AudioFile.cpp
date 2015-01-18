/*
 * (C) 2014 see Authors.txt
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
#include "AudioFile.h"
#include "AMRFile.h"
#include "APEFile.h"
#include "DFFFile.h"
#include "DSFFile.h"
#include "TAKFile.h"
#include "TTAFile.h"
#include "WavPackFile.h"
#include "WAVFile.h"
#include "Wave64File.h"

//
// CAudioFile
//

CAudioFile::CAudioFile()
	: m_pFile(NULL)
	, m_startpos(0)
	, m_endpos(0)
	, m_samplerate(0)
	, m_bitdepth(0)
	, m_channels(0)
	, m_layout(0)
	, m_rtduration(0)
	, m_subtype(GUID_NULL)
	, m_wFormatTag(0)
	, m_extradata(NULL)
	, m_extrasize(0)
	, m_nAvgBytesPerSec(0)
{
}

CAudioFile::~CAudioFile()
{
	if (m_extradata) {
		free(m_extradata);
	}
	m_pFile = NULL;
}

CAudioFile* CAudioFile::CreateFilter(CBaseSplitterFile* m_pFile)
{
	CAudioFile* pAudioFile = NULL;
	BYTE data[40];

	m_pFile->Seek(0);
	if (m_pFile->ByteRead(data, sizeof(data)) != S_OK) {
		return NULL;
	}

	DWORD* id = (DWORD*)data;
	if (*id == FCC('MAC ')) {
		pAudioFile = DNew CAPEFile();
	}
	else if (*id == FCC('tBaK')) {
		pAudioFile = DNew CTAKFile();
	}
	else if (*id == FCC('TTA1')) {
		pAudioFile = DNew CTTAFile();
	}
	else if (*id == FCC('wvpk')) {
		pAudioFile = DNew CWavPackFile();
	}
	else if (memcmp(data, AMR_header, 6) == 0 || memcmp(data, AMRWB_header, 9) == 0) {
		pAudioFile = DNew CAMRFile();
	}
	else if (*id == FCC('RIFF') && *(DWORD*)(data+8) == FCC('WAVE')) {
		pAudioFile = DNew CWAVFile();
	}
	else if (memcmp(data, w64_guid_riff, 16) == 0 &&  memcmp(data+24, w64_guid_wave, 16) == 0) {
		pAudioFile = DNew CWave64File();
	}
	else if (*id == FCC('FRM8') && *(DWORD*)(data+12) == FCC('DSD ')) {
		pAudioFile = DNew CDFFFile();
	}
	else if (*id == FCC('DSD ') && data[4] == 0x1C) {
		pAudioFile = DNew CDSFFile();
	}
	else if (int id3v2_size = id3v2_match_len(data)) {
		// skip ID3V2 metadata for formats that can contain it
		m_pFile->Seek(id3v2_size);
		if (m_pFile->ByteRead(data, 4) != S_OK) {
			return NULL;
		}
		if (*id == FCC('TTA1')) {
			pAudioFile = DNew CTTAFile();
		} else {
			return NULL;
		}
	}
	else {
		return NULL;
	}

	if (FAILED(pAudioFile->Open(m_pFile))) {
		SAFE_DELETE(pAudioFile);
	}

	return pAudioFile;
}

bool CAudioFile::SetMediaType(CMediaType& mt)
{
	mt.majortype			= MEDIATYPE_Audio;
	mt.formattype			= FORMAT_WaveFormatEx;
	mt.subtype				= m_subtype;
	mt.SetSampleSize(256000);

	WAVEFORMATEX* wfe		= (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX) + m_extrasize);
	wfe->wFormatTag			= m_wFormatTag;
	wfe->nChannels			= m_channels;
	wfe->nSamplesPerSec		= m_samplerate;
	wfe->wBitsPerSample		= m_bitdepth;
	wfe->nBlockAlign		= m_channels * m_bitdepth / 8;
	wfe->nAvgBytesPerSec	= wfe->nSamplesPerSec * wfe->nBlockAlign;
	wfe->cbSize				= m_extrasize;
	memcpy(wfe + 1, m_extradata, m_extrasize);

	m_nAvgBytesPerSec		= wfe->nAvgBytesPerSec;

	return true;
}

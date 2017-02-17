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

#pragma once

#include "AudioFile.h"

class CDFFFile : public CAudioFile
{
	struct dffchunk_t {
		DWORD id;
		UINT64 size;
	};

	__int64 m_length	= 0;
	int m_block_size	= 0;
	int m_max_blocksize	= 0;

	CID3Tag* m_ID3Tag	= NULL;

	bool ReadDFFChunk(dffchunk_t& chunk);
	bool parse_dsd_prop(__int64 eof);
	bool parse_dsd_diin(__int64 eof);
	bool parse_dsd_dst(__int64 eof);

public:
	CDFFFile();
	~CDFFFile();

	bool SetMediaType(CMediaType& mt);
	void SetProperties(IBaseFilter* pBF);

	HRESULT Open(CBaseSplitterFile* pFile);
	REFERENCE_TIME Seek(REFERENCE_TIME rt);
	int GetAudioFrame(CPacket* packet, REFERENCE_TIME rtStart);
	CString GetName() const { return m_subtype == MEDIASUBTYPE_DSDM ? L"DSD-IFF" : L"DST-IFF"; };

protected:
	CAtlMap<DWORD, CStringA> m_info;
};

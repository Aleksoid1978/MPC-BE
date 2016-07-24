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

#pragma once

#include "AudioFile.h"

class CWAVFile : public CAudioFile
{
protected:
	__int64  m_length      = 0;

	BYTE*    m_fmtdata     = NULL;
	DWORD    m_fmtsize     = 0;

	WORD     m_nBlockAlign = 0;
	int      m_blocksize   = 0;

	CID3Tag* m_ID3Tag      = NULL;

	CAtlMap<DWORD, CStringA> m_info;

	bool ProcessWAVEFORMATEX();
	HRESULT ReadRIFFINFO(const DWORD chunk_size);
	HRESULT ReadID3Tag(const DWORD chunk_size);
	bool CheckDTSAC3CD();

public:
	CWAVFile();
	virtual ~CWAVFile();

	bool SetMediaType(CMediaType& mt);
	void SetProperties(IBaseFilter* pBF);

	HRESULT Open(CBaseSplitterFile* pFile);
	REFERENCE_TIME Seek(REFERENCE_TIME rt);
	int GetAudioFrame(CPacket* packet, REFERENCE_TIME rtStart);
	CString GetName() const { return L"WAVE"; };
};

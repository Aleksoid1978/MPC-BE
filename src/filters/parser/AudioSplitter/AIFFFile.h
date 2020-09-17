/*
 * (C) 2020 see Authors.txt
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

class CAIFFFile : public CAudioFile
{
protected:
	__int64 m_length = 0;

	WORD m_block_align = 0;
	int  m_block_size = 0;

	HRESULT ReadCommonTag(const DWORD chunk_size);
public:
	CAIFFFile();
	virtual ~CAIFFFile() = default;

	HRESULT Open(CBaseSplitterFile* pFile);
	REFERENCE_TIME Seek(REFERENCE_TIME rt);
	int GetAudioFrame(CPacket* packet, REFERENCE_TIME rtStart);
	CString GetName() const { return L"AIFF"; };
};

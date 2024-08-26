/*
 * (C) 2014-2024 see Authors.txt
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

#include "../BaseSplitter/BaseSplitter.h"
#include "DSUtil/ApeTag.h"
#include "DSUtil/ID3Tag.h"
#include "DSUtil/SimpleBuffer.h"
#include <wmcodecdsp.h>
#include <moreuuids.h>
#include <stdint.h>

class CAudioFile
{
protected:
	CBaseSplitterFile* m_pFile = nullptr;

	__int64 m_startpos = 0;
	__int64 m_endpos = 0;

	int   m_samplerate = 0;
	int   m_bitdepth = 0;
	int   m_channels = 0;
	DWORD m_layout = 0;
	GUID  m_subtype = GUID_NULL;
	WORD  m_wFormatTag = 0;

	REFERENCE_TIME m_rtduration = 0;

	CSimpleBlock<BYTE> m_extradata;

	DWORD m_nAvgBytesPerSec = 0;

	CAPETag* m_pAPETag = nullptr;
	CID3Tag* m_pID3Tag = nullptr;

public:
	CAudioFile() = default;
	virtual ~CAudioFile();

	static CAudioFile* CreateFilter(CBaseSplitterFile* pFile);

	__int64 GetStartPos() const { return m_startpos; }
	__int64 GetEndPos() const { return m_endpos; }
	REFERENCE_TIME GetDuration() const { return m_rtduration;}
	virtual bool SetMediaType(CMediaType& mt);
	virtual void SetProperties(IBaseFilter* pBF);

	virtual HRESULT Open(CBaseSplitterFile* pFile) PURE;
	virtual REFERENCE_TIME Seek(REFERENCE_TIME rt) PURE;
	virtual int GetAudioFrame(CPacket* packet, REFERENCE_TIME rtStart) PURE;
	virtual CString GetName() const PURE;

	bool ReadApeTag(size_t& size);
	bool ReadID3Tag(const int64_t size);
};

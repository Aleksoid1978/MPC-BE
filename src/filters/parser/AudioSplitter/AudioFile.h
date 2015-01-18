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

#include "../BaseSplitter/BaseSplitter.h"
#include "../DSUtil/ApeTag.h"
#include "../DSUtil/ID3Tag.h"
#include <MMReg.h>
#include <moreuuids.h>

#pragma warning(disable: 4005 4244)
extern "C" {
	#include <stdint.h>
}

class CAudioFile
{
protected:
	CBaseSplitterFile* m_pFile;

	__int64			m_startpos;
	__int64			m_endpos;

	int				m_samplerate;
	int				m_bitdepth;
	int				m_channels;
	DWORD			m_layout;

	//__int64			m_totalsamples;
	REFERENCE_TIME	m_rtduration;

	GUID			m_subtype;
	WORD			m_wFormatTag;
	BYTE*			m_extradata;
	int				m_extrasize;

	DWORD			m_nAvgBytesPerSec;

public:
	CAudioFile();
	virtual ~CAudioFile();

	static CAudioFile* CreateFilter(CBaseSplitterFile* m_pFile);

	__int64 GetStartPos() const { return m_startpos; }
	__int64 GetEndPos() const { return m_endpos; }
	REFERENCE_TIME GetDuration() const { return m_rtduration;}
	virtual bool SetMediaType(CMediaType& mt);
	virtual void SetProperties(IBaseFilter* pBF) {};

	virtual HRESULT Open(CBaseSplitterFile* pFile) PURE;
	virtual REFERENCE_TIME Seek(REFERENCE_TIME rt) PURE;
	virtual int GetAudioFrame(CPacket* packet, REFERENCE_TIME rtStart) PURE;
	virtual CString GetName() const PURE;
};

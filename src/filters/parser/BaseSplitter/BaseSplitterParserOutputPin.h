/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2015 see Authors.txt
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

#include "../../../DSUtil/Packet.h"
#include "BaseSplitterOutputPin.h"

struct MpegParseContext {
	bool	bFrameStartFound = FALSE;
	UINT64	state64			 = 0;
};

class CBaseSplitterParserOutputPin : public CBaseSplitterOutputPin, protected CCritSec
{
	class CH264Packet : public CPacket
	{
		public:
			BOOL bSliceExist = FALSE;
	};

	CAutoPtr<CPacket>			m_p;
	CAutoPtrList<CH264Packet>	m_pl;
	MpegParseContext			m_ParseContext;

	bool	m_bHasAccessUnitDelimiters;
	bool	m_bFlushed;
	int		m_truehd_framelength;
	bool	m_bEndOfStream;

	WORD	m_nChannels;
	DWORD	m_nSamplesPerSec;
	WORD	m_wBitsPerSample;

	int		m_adx_block_size;

protected:
	HRESULT DeliverPacket(CAutoPtr<CPacket> p);
	HRESULT DeliverEndFlush();

	HRESULT Flush();

	void InitPacket(CPacket* pSource);
	void InitAudioParams();

	HRESULT ParseAAC(CAutoPtr<CPacket> p);
	HRESULT ParseAnnexB(CAutoPtr<CPacket> p, bool bConvertToAVCC);
	HRESULT ParseHEVC(CAutoPtr<CPacket> p);
	HRESULT ParseVC1(CAutoPtr<CPacket> p);
	HRESULT ParseHDMVLPCM(CAutoPtr<CPacket> p);
	HRESULT ParseAC3(CAutoPtr<CPacket> p);
	HRESULT ParseTrueHD(CAutoPtr<CPacket> p, BOOL bCheckAC3 = TRUE);
	HRESULT ParseDirac(CAutoPtr<CPacket> p);
	HRESULT ParseVobSub(CAutoPtr<CPacket> p);
	HRESULT ParseAdxADPCM(CAutoPtr<CPacket> p);

public:
	CBaseSplitterParserOutputPin(CAtlArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr, int factor = 1);
	virtual ~CBaseSplitterParserOutputPin();

	HRESULT DeliverEndOfStream();
};

/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2024 see Authors.txt
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

#include "DSUtil/Packet.h"
#include "BaseSplitterOutputPin.h"
#include "Teletext.h"

struct MpegParseContext {
	bool   bFrameStartFound = false;
	UINT64 state64          = 0;
};

class CBaseSplitterParserOutputPin : public CBaseSplitterOutputPin, protected CCritSec
{
	class CH264Packet : public CPacket
	{
		public:
			BOOL bDataExists = FALSE;
	};

	std::unique_ptr<CPacket> m_p;
	std::list<std::unique_ptr<CH264Packet>> m_pl;

	MpegParseContext m_ParseContext;
	CTeletext        m_teletext;

	bool  m_bHasAccessUnitDelimiters = false;
	bool  m_bFlushed                 = false;
	bool  m_bEndOfStream             = false;
	int   m_truehd_framelength       = 0;

	WORD  m_nChannels      = 0;
	DWORD m_nSamplesPerSec = 0;
	WORD  m_wBitsPerSample = 0;

	int   m_adx_block_size = 0;

	UINT32 m_packetFlag     = 0;
	BYTE  m_DTSHDProfile   = 0;

	HRESULT DeliverParsed(const BYTE* start, const size_t size);
	void HandlePacket(std::unique_ptr<CPacket>& p);

protected:
	HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	HRESULT DeliverPacket(std::unique_ptr<CPacket> p);

	HRESULT Flush();

private:
	void InitPacket(CPacket* pSource);

	HRESULT ParseAAC(std::unique_ptr<CPacket>& p);
	HRESULT ParseAACLATM(std::unique_ptr<CPacket>& p);
	HRESULT ParseAnnexB(std::unique_ptr<CPacket>& p, bool bConvertToAVCC);
	HRESULT ParseHEVC(std::unique_ptr<CPacket>& p);
	HRESULT ParseVVC(std::unique_ptr<CPacket>& p);
	HRESULT ParseVC1(std::unique_ptr<CPacket>& p);
	HRESULT ParseHDMVLPCM(std::unique_ptr<CPacket>& p);
	HRESULT ParseAC3(std::unique_ptr<CPacket>& p);
	HRESULT ParseTrueHD(std::unique_ptr<CPacket>& p, BOOL bCheckAC3 = TRUE);
	HRESULT ParseDirac(std::unique_ptr<CPacket>& p);
	HRESULT ParseVobSub(std::unique_ptr<CPacket>& p);
	HRESULT ParseAdxADPCM(std::unique_ptr<CPacket>& p);
	HRESULT ParseDTS(std::unique_ptr<CPacket>& p);
	HRESULT ParseTeletext(std::unique_ptr<CPacket>& p);
	HRESULT ParseMpegVideo(std::unique_ptr<CPacket>& p);

public:
	CBaseSplitterParserOutputPin(std::vector<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
	virtual ~CBaseSplitterParserOutputPin();

	HRESULT DeliverEndOfStream();
};

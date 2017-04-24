/*
 * (C) 2015-2017 see Authors.txt
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
 * Based on telxcc.c from ccextractor source code
 * http://ccextractor.sourceforge.net/
 */

#pragma once

#include <vector>
#include <stdint.h>
#include <mpc_defines.h>

struct TeletextData {
	REFERENCE_TIME rtStart = INVALID_TIME;
	REFERENCE_TIME rtStop  = INVALID_TIME;
	CString str;
};

class CTeletext
{
	#pragma pack(push, 1)
	struct teletext_packet_payload {
		uint8_t _clock_in;     // clock run in
		uint8_t _framing_code; // framing code, not needed, ETSI 300 706: const 0xe4
		uint8_t address[2];
		uint8_t data[40];
	};
	#pragma pack(pop)

	struct {
		REFERENCE_TIME rtStart;
		REFERENCE_TIME rtStop;
		uint16_t text[25][40]; // 25 lines x 40 cols (1 screen/page) of wide chars
		uint8_t tainted;       // 1 = text variable contains any data
	} m_page_buffer;

	BOOL m_bReceivingData = FALSE;
	uint16_t m_nSuitablePage = 0;

	std::vector<TeletextData> m_output;

	void ProcessTeletextPage();
	void ProcessTeletextPacket(teletext_packet_payload* packet, REFERENCE_TIME rtStart);
public:
	CTeletext();

	void ProcessData(uint8_t* buffer, uint16_t size, REFERENCE_TIME rtStart);
	void Flush();
	void SetLCID(const LCID lcid);

	BOOL IsOutputPresent() const { return !m_output.empty(); }
	void GetOutput(std::vector<TeletextData>& output);
	void EraseOutput();

	BOOL ProcessRemainingData();
};

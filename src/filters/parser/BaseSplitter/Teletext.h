/*
 * (C) 2015-2020 see Authors.txt
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
 * https://github.com/CCExtractor/ccextractor
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

	enum transmission_mode {
		TRANSMISSION_MODE_PARALLEL = 0,
		TRANSMISSION_MODE_SERIAL = 1
	};

	enum data_unit {
		DATA_UNIT_EBU_TELETEXT_NONSUBTITLE = 0x02,
		DATA_UNIT_EBU_TELETEXT_SUBTITLE = 0x03,
		DATA_UNIT_EBU_TELETEXT_INVERTED = 0x0c,
		DATA_UNIT_VPS = 0xc3,
		DATA_UNIT_CLOSED_CAPTIONS = 0xc5
	};

	BOOL m_bReceivingData = FALSE;
	uint16_t m_nSuitablePage = 0;
	int m_packetCntFlagOn = 0; // a keeps count of packets with flag subtitle ON and data packets

	std::vector<TeletextData> m_output;

	void ProcessTeletextPage();
	void ProcessTeletextPacket(teletext_packet_payload* packet, REFERENCE_TIME rtStart, data_unit data_unit_id);
public:
	CTeletext();

	void ProcessData(uint8_t* buffer, uint16_t size, REFERENCE_TIME rtStart, WORD tlxPage);
	void Flush();
	void SetLCID(const LCID lcid);

	BOOL IsOutputPresent() const { return !m_output.empty(); }
	const std::vector<TeletextData>& GetOutput() const { return m_output; }
	void ClearOutput() { m_output.clear(); }

	void ProcessRemainingData();
};

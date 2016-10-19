/*
 * (C) 2012-2016 see Authors.txt
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

#include <basestruct.h>

static const byte pixel_aspect[17][2]= {
	{  0,   1 },
	{  1,   1 },
	{ 12,  11 },
	{ 10,  11 },
	{ 16,  11 },
	{ 40,  33 },
	{ 24,  11 },
	{ 20,  11 },
	{ 32,  11 },
	{ 80,  33 },
	{ 18,  11 },
	{ 15,  11 },
	{ 64,  33 },
	{ 160, 99 },
	{  4,   3 },
	{  3,   2 },
	{  2,   1 },
};

struct timing {
	UINT32 num_units_in_tick;
	UINT32 time_scale;
};
struct vc_params_t {
	LONG width, height;

	DWORD profile, level;
	DWORD nal_length_size;

	BOOL interlaced;

	timing vps_timing;
	timing vui_timing;

	REFERENCE_TIME AvgTimePerFrame;

	fraction_t sar;

	void clear() {
		memset(this, 0, sizeof(*this));
	}
};

bool ParseDiracHeader(BYTE* data, int size, vc_params_t& params);

namespace AVCParser {
	bool ParseSequenceParameterSet(BYTE* data, int size, vc_params_t& params);
} // namespace H264Parser

namespace HEVCParser {
	void CreateSequenceHeaderAVC(BYTE* data, int size, DWORD* dwSequenceHeader, DWORD& cbSequenceHeader);
	void CreateSequenceHeaderHEVC(BYTE* data, int size, DWORD* dwSequenceHeader, DWORD& cbSequenceHeader);

	bool ParseSequenceParameterSet(BYTE* data, int size, vc_params_t& params);
	bool ParseVideoParameterSet(BYTE* data, int size, vc_params_t& params);

	bool ParseAVCDecoderConfigurationRecord(BYTE* data, int size, vc_params_t& params, int flv_hm = 0);
	bool ParseHEVCDecoderConfigurationRecord(BYTE* data, int size, vc_params_t& params, bool parseSPS);
} // namespace HEVCParser

/*
 * (C) 2012-2020 see Authors.txt
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
#include <vector>

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

bool ParseDiracHeader(const BYTE* data, const int size, vc_params_t& params);

namespace AVCParser {
	bool ParseSequenceParameterSet(const BYTE* data, const int size, vc_params_t& params);
} // namespace AVCParser

namespace HEVCParser {
	void CreateSequenceHeaderAVC(const BYTE* data, const int size, DWORD* dwSequenceHeader, DWORD& cbSequenceHeader);
	void CreateSequenceHeaderHEVC(const BYTE* data, const int size, DWORD* dwSequenceHeader, DWORD& cbSequenceHeader);

	bool ParseSequenceParameterSet(const BYTE* data, const int size, vc_params_t& params);
	bool ParseVideoParameterSet(const BYTE* data, const int size, vc_params_t& params);

	bool ParseAVCDecoderConfigurationRecord(const BYTE* data, const int size, vc_params_t& params, const int flv_hm = 0);
	bool ParseHEVCDecoderConfigurationRecord(const BYTE* data, const int size, vc_params_t& params, const bool parseSPS);
} // namespace HEVCParser

namespace AV1Parser {
	inline enum AV1_OBU_Type {
		// 0 reserved.
		AV1_OBU_SEQUENCE_HEADER = 1,
		AV1_OBU_TEMPORAL_DELIMITER = 2,
		AV1_OBU_FRAME_HEADER = 3,
		AV1_OBU_TILE_GROUP = 4,
		AV1_OBU_METADATA = 5,
		AV1_OBU_FRAME = 6,
		AV1_OBU_REDUNDANT_FRAME_HEADER = 7,
		AV1_OBU_TILE_LIST = 8,
		// 9-14 reserved.
		AV1_OBU_PADDING = 15,
	} AV1_OBU_Type;

	constexpr auto MAX_OBU_HEADER_SIZE = 2 + 8;

	struct AV1SequenceParameters {
		uint8_t profile                        = 0;
		uint8_t level                          = 0;
		uint8_t tier                           = 0;
		uint8_t bitdepth                       = 0;
		uint8_t monochrome                     = 0;
		uint8_t chroma_subsampling_x           = 0;
		uint8_t chroma_subsampling_y           = 0;
		uint8_t chroma_sample_position         = 0;
		uint8_t color_description_present_flag = 0;
		uint8_t color_primaries                = 0;
		uint8_t transfer_characteristics       = 0;
		uint8_t matrix_coefficients            = 0;
		uint8_t color_range                    = 0;
		uint32_t width                         = 0;
		uint32_t height                        = 0;
	};

	int64_t ParseOBUHeaderSize(const BYTE* buf, const int buf_size, uint8_t& obu_type);
	bool ParseOBU(const BYTE* data, int size, AV1SequenceParameters& seq_params, std::vector<uint8_t>& obu_sequence_header);
} // namespace AV1Parser
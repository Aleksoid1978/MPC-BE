/*
 * Copyright (C) 2012 Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
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

#include "stdafx.h"
#include "VideoParser.h"
#include "H264Nalu.h"

struct dirac_source_params {
	unsigned width;
	unsigned height;
	WORD chroma_format;          ///< 0: 444  1: 422  2: 420

	BYTE interlaced;
	BYTE top_field_first;

	BYTE frame_rate_index;       ///< index into dirac_frame_rate[]
	BYTE aspect_ratio_index;     ///< index into dirac_aspect_ratio[]

	WORD clean_width;
	WORD clean_height;
	WORD clean_left_offset;
	WORD clean_right_offset;

	BYTE pixel_range_index;      ///< index into dirac_pixel_range_presets[]
	BYTE color_spec_index;       ///< index into dirac_color_spec_presets[]
};

static const dirac_source_params dirac_source_parameters_defaults[] = {
	{ 640,  480,  2, 0, 0, 1,  1, 640,  480,  0, 0, 1, 0 },
	{ 176,  120,  2, 0, 0, 9,  2, 176,  120,  0, 0, 1, 1 },
	{ 176,  144,  2, 0, 1, 10, 3, 176,  144,  0, 0, 1, 2 },
	{ 352,  240,  2, 0, 0, 9,  2, 352,  240,  0, 0, 1, 1 },
	{ 352,  288,  2, 0, 1, 10, 3, 352,  288,  0, 0, 1, 2 },
	{ 704,  480,  2, 0, 0, 9,  2, 704,  480,  0, 0, 1, 1 },
	{ 704,  576,  2, 0, 1, 10, 3, 704,  576,  0, 0, 1, 2 },
	{ 720,  480,  1, 1, 0, 4,  2, 704,  480,  8, 0, 3, 1 },
	{ 720,  576,  1, 1, 1, 3,  3, 704,  576,  8, 0, 3, 2 },
	{ 1280, 720,  1, 0, 1, 7,  1, 1280, 720,  0, 0, 3, 3 },
	{ 1280, 720,  1, 0, 1, 6,  1, 1280, 720,  0, 0, 3, 3 },
	{ 1920, 1080, 1, 1, 1, 4,  1, 1920, 1080, 0, 0, 3, 3 },
	{ 1920, 1080, 1, 1, 1, 3,  1, 1920, 1080, 0, 0, 3, 3 },
	{ 1920, 1080, 1, 0, 1, 7,  1, 1920, 1080, 0, 0, 3, 3 },
	{ 1920, 1080, 1, 0, 1, 6,  1, 1920, 1080, 0, 0, 3, 3 },
	{ 2048, 1080, 0, 0, 1, 2,  1, 2048, 1080, 0, 0, 4, 4 },
	{ 4096, 2160, 0, 0, 1, 2,  1, 4096, 2160, 0, 0, 4, 4 },
	{ 3840, 2160, 1, 0, 1, 7,  1, 3840, 2160, 0, 0, 3, 3 },
	{ 3840, 2160, 1, 0, 1, 6,  1, 3840, 2160, 0, 0, 3, 3 },
	{ 7680, 4320, 1, 0, 1, 7,  1, 3840, 2160, 0, 0, 3, 3 },
	{ 7680, 4320, 1, 0, 1, 6,  1, 3840, 2160, 0, 0, 3, 3 },
};

static const AV_Rational avpriv_frame_rate_tab[16] = {
	{    0,    0},
	{24000, 1001},
	{   24,    1},
	{   25,    1},
	{30000, 1001},
	{   30,    1},
	{   50,    1},
	{60000, 1001},
	{   60,    1},
	// Xing's 15fps: (9)
	{   15,    1},
	// libmpeg3's "Unofficial economy rates": (10-13)
	{    5,    1},
	{   10,    1},
	{   12,    1},
	{   15,    1},
	{    0,    0},
};

static const AV_Rational dirac_frame_rate[] = {
	{15000, 1001},
	{25, 2},
};

bool ParseDiracHeader(BYTE* data, int size, vc_params_t& params)
{
	memset(&params, 0, sizeof(params));

	CGolombBuffer gb(data, size);

	unsigned int version_major = gb.UintGolombRead();
	if (version_major < 2) {
		return false;
	}
	gb.UintGolombRead(); /* version_minor */
	gb.UintGolombRead(); /* profile */
	gb.UintGolombRead(); /* level */
	unsigned int video_format = gb.UintGolombRead();

	dirac_source_params source = dirac_source_parameters_defaults[video_format];

	if (gb.BitRead(1)) {
		source.width	= gb.UintGolombRead();
		source.height	= gb.UintGolombRead();
	}
	if (!source.width || !source.height) {
		return false;
	}

	if (gb.BitRead(1)) {
		source.chroma_format = gb.UintGolombRead();
	}
	if (source.chroma_format > 2) {
		return false;
	}

	if (gb.BitRead(1)) {
		source.interlaced = gb.UintGolombRead();
	}
	if (source.interlaced > 1) {
		return false;
	}

	AV_Rational frame_rate = {0,0};
	if (gb.BitRead(1)) {
		source.frame_rate_index = gb.UintGolombRead();
		if (source.frame_rate_index > 10) {
			return false;
		}
		if (!source.frame_rate_index) {
			frame_rate.num = gb.UintGolombRead();
			frame_rate.den = gb.UintGolombRead();
		}
	}
	if (source.frame_rate_index > 0) {
		if (source.frame_rate_index <= 8) {
			frame_rate = avpriv_frame_rate_tab[source.frame_rate_index];
		} else {
			frame_rate = dirac_frame_rate[source.frame_rate_index - 9];
		}
	}
	if (!frame_rate.num || !frame_rate.den) {
		return false;
	}

	params.width			= source.width;
	params.height			= source.height;
	params.AvgTimePerFrame	= (REFERENCE_TIME)(10000000.0 * frame_rate.den / frame_rate.num);

	return true;
}

namespace AVCParser {
	static bool ParseHrdParameters(CGolombBuffer& gb)
	{
		UINT64 cnt = gb.UExpGolombRead();	// cpb_cnt_minus1
		if (cnt > 32U) {
			return false;
		}
		gb.BitRead(4);							// bit_rate_scale
		gb.BitRead(4);							// cpb_size_scale

		for (unsigned int i = 0; i <= cnt; i++) {
			gb.UExpGolombRead();				// bit_rate_value_minus1
			gb.UExpGolombRead();				// cpb_size_value_minus1
			gb.BitRead(1);						// cbr_flag
		}

		gb.BitRead(5);							// initial_cpb_removal_delay_length_minus1
		gb.BitRead(5);							// cpb_removal_delay_length_minus1
		gb.BitRead(5);							// dpb_output_delay_length_minus1
		gb.BitRead(5);							// time_offset_length

		return true;
	}

	static inline bool MatchValue(BYTE array[], size_t size, BYTE value)
	{
		for (size_t i = 0; i < size; i++) {
			if (value == array[i]) {
				return true;
			}
		}

		return false;
	}

#define H264_PROFILE_MULTIVIEW_HIGH	118
#define H264_PROFILE_STEREO_HIGH	128
	bool ParseSequenceParameterSet(BYTE* data, int size, vc_params_t& params)
	{
		static BYTE profiles[] = { 44, 66, 77, 88, 100, 110, 118, 122, 128, 144, 244 };
		static BYTE levels[] = { 10, 11, 12, 13, 20, 21, 22, 30, 31, 32, 40, 41, 42, 50, 51, 52 };

		CGolombBuffer gb(data, size, true);

		memset(&params, 0, sizeof(params));

		params.profile = gb.BitRead(8);
		if (!MatchValue(profiles, _countof(profiles), params.profile)) {
			goto error;
		}

		gb.BitRead(8);
		params.level = gb.BitRead(8);
		if (!MatchValue(levels, _countof(levels), params.level)) {
			goto error;
		}

		UINT64 sps_id = gb.UExpGolombRead();	// seq_parameter_set_id
		if (sps_id >= 32) {
			goto error;
		}

		UINT64 chroma_format_idc = 0;
		if (params.profile >= 100) {					// high profile
			chroma_format_idc = gb.UExpGolombRead();
			if (chroma_format_idc == 3) {		// chroma_format_idc
				gb.BitRead(1);					// residue_transform_flag
			}

			gb.UExpGolombRead();				// bit_depth_luma_minus8
			gb.UExpGolombRead();				// bit_depth_chroma_minus8

			gb.BitRead(1);						// qpprime_y_zero_transform_bypass_flag

			if (gb.BitRead(1)) {				// seq_scaling_matrix_present_flag
				for (int i = 0; i < 8; i++) {
					if (gb.BitRead(1)) {		// seq_scaling_list_present_flag
						for (int j = 0, size = i < 6 ? 16 : 64, next = 8; j < size && next != 0; ++j) {
							next = (next + gb.SExpGolombRead() + 256) & 255;
						}
					}
				}
			}
		}

		gb.UExpGolombRead();					// log2_max_frame_num_minus4

		UINT64 pic_order_cnt_type = gb.UExpGolombRead();

		if (pic_order_cnt_type == 0) {
			gb.UExpGolombRead();				// log2_max_pic_order_cnt_lsb_minus4
		} else if (pic_order_cnt_type == 1) {
			gb.BitRead(1);						// delta_pic_order_always_zero_flag
			gb.SExpGolombRead();				// offset_for_non_ref_pic
			gb.SExpGolombRead();				// offset_for_top_to_bottom_field
			UINT64 num_ref_frames_in_pic_order_cnt_cycle = gb.UExpGolombRead();
			if (num_ref_frames_in_pic_order_cnt_cycle >= 256) {
				goto error;
			}
			for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++) {
				gb.SExpGolombRead();			// offset_for_ref_frame[i]
			}
		} else if (pic_order_cnt_type != 2) {
			goto error;
		}

		UINT64 ref_frame_count = gb.UExpGolombRead();	// num_ref_frames
		if (ref_frame_count > 30) {
			goto error;
		}
		gb.BitRead(1);									// gaps_in_frame_num_value_allowed_flag

		UINT64 pic_width_in_mbs_minus1 = gb.UExpGolombRead();
		UINT64 pic_height_in_map_units_minus1 = gb.UExpGolombRead();
		params.interlaced = !gb.BitRead(1);

		if (params.interlaced) {
			gb.BitRead(1);								// mb_adaptive_frame_field_flag
		}

		BYTE direct_8x8_inference_flag = (BYTE)gb.BitRead(1); // direct_8x8_inference_flag
		if (params.interlaced && !direct_8x8_inference_flag) {
			goto error;
		}

		UINT crop_left, crop_right, crop_top, crop_bottom;
		crop_left = crop_right = crop_top = crop_bottom = 0;
		if (gb.BitRead(1)) {					// frame_cropping_flag
			crop_left	= gb.UExpGolombRead();	// frame_cropping_rect_left_offset
			crop_right	= gb.UExpGolombRead();	// frame_cropping_rect_right_offset
			crop_top	= gb.UExpGolombRead();	// frame_cropping_rect_top_offset
			crop_bottom	= gb.UExpGolombRead();	// frame_cropping_rect_bottom_offset
		}

		if (gb.BitRead(1)) {							// vui_parameters_present_flag
			if (gb.BitRead(1)) {						// aspect_ratio_info_present_flag
				BYTE aspect_ratio_idc = (BYTE)gb.BitRead(8); // aspect_ratio_idc
				if (255 == aspect_ratio_idc) {
					params.sar.num = (WORD)gb.BitRead(16);	// sar_width
					params.sar.den = (WORD)gb.BitRead(16);	// sar_height
				}
				else if (aspect_ratio_idc < _countof(pixel_aspect)) {
					params.sar.num = pixel_aspect[aspect_ratio_idc][0];
					params.sar.den = pixel_aspect[aspect_ratio_idc][1];
				}
				else {
					goto error;
				}
			}

			if (gb.BitRead(1)) {				// overscan_info_present_flag
				gb.BitRead(1);					// overscan_appropriate_flag
			}

			if (gb.BitRead(1)) {				// video_signal_type_present_flag
				gb.BitRead(3);					// video_format
				gb.BitRead(1);					// video_full_range_flag
				if (gb.BitRead(1)) {			// colour_description_present_flag
					gb.BitRead(8);				// colour_primaries
					gb.BitRead(8);				// transfer_characteristics
					gb.BitRead(8);				// matrix_coefficients
				}
			}
			if (gb.BitRead(1)) {				// chroma_location_info_present_flag
				gb.UExpGolombRead();			// chroma_sample_loc_type_top_field
				gb.UExpGolombRead();			// chroma_sample_loc_type_bottom_field
			}
			if (gb.BitRead(1)) {				// timing_info_present_flag
				UINT32 num_units_in_tick = gb.BitRead(32);
				UINT32 time_scale = gb.BitRead(32);
				BYTE fixed_frame_rate_flag = gb.BitRead(1);

				if (num_units_in_tick && time_scale) {
					params.AvgTimePerFrame = (REFERENCE_TIME)(10000000.0 * num_units_in_tick * 2 / time_scale);
				}
			}

			bool nalflag = !!gb.BitRead(1);		// nal_hrd_parameters_present_flag
			if (nalflag) {
				if (!ParseHrdParameters(gb)) {
					goto error;
				}
			}
			bool vlcflag = !!gb.BitRead(1);		// vlc_hrd_parameters_present_flag
			if (vlcflag) {
				if (!ParseHrdParameters(gb)) {
					goto error;
				}
			}
			if (nalflag || vlcflag) {
				gb.BitRead(1);					// low_delay_hrd_flag
			}

			gb.BitRead(1);						// pic_struct_present_flag
			if (gb.BitRead(1)) {				// bitstream_restriction_flag
				gb.BitRead(1);					// motion_vectors_over_pic_boundaries_flag
				gb.UExpGolombRead();			// max_bytes_per_pic_denom
				gb.UExpGolombRead();			// max_bits_per_mb_denom
				gb.UExpGolombRead();			// log2_max_mv_length_horizontal
				gb.UExpGolombRead();			// log2_max_mv_length_vertical
				UINT64 num_reorder_frames = gb.UExpGolombRead(); // num_reorder_frames
				gb.UExpGolombRead();			// max_dec_frame_buffering

				if (gb.GetSize() < gb.GetPos()) {
					num_reorder_frames = 0;
				}
				if (num_reorder_frames > 16U) {
					goto error;
				}
			}
		}

		if (params.profile == H264_PROFILE_MULTIVIEW_HIGH ||
			params.profile == H264_PROFILE_STEREO_HIGH) {
			UINT8 bit_equal_to_one = gb.BitRead(1);	// bit_equal_to_one
			if (!bit_equal_to_one) {
				goto error;
			}
			gb.UExpGolombRead();					// num_views_minus1
		}

		if (!params.sar.num)
			params.sar.num = 1;
		if (!params.sar.den)
			params.sar.den = 1;

		unsigned int mb_Width = (unsigned int)pic_width_in_mbs_minus1 + 1;
		unsigned int mb_Height = ((unsigned int)pic_height_in_map_units_minus1 + 1) * (2 - !params.interlaced);
		BYTE CHROMA444 = (chroma_format_idc == 3);

		params.width = 16 * mb_Width - (2u >> CHROMA444) * min(crop_right, (8u << CHROMA444) - 1);
		if (!params.interlaced) {
			params.height = 16 * mb_Height - (2u >> CHROMA444) * min(crop_bottom, (8u << CHROMA444) - 1);
		} else {
			params.height = 16 * mb_Height - (4u >> CHROMA444) * min(crop_bottom, (8u << CHROMA444) - 1);
		}

		if (params.height < 100 || params.width < 100) {
			goto error;
		}

		if (params.height == 1088) {
			params.height = 1080;	// Prevent blur lines
		}

		return true;

	error:
		memset(&params, 0, sizeof(params));
		return false;
	}
} // namespace AVCParser

namespace HEVCParser {
	void CreateSequenceHeaderAVC(BYTE* data, int size, DWORD* dwSequenceHeader, DWORD& cbSequenceHeader)
	{
		// copy VideoParameterSets(VPS), SequenceParameterSets(SPS) and PictureParameterSets(PPS) from AVCDecoderConfigurationRecord

		cbSequenceHeader = 0;
		if (size < 7 || (data[5] & 0xe0) != 0xe0) {
			return;
		}

		BYTE* src = data + 5;
		BYTE* dst = (BYTE*)dwSequenceHeader;
		BYTE* src_end = data + size;
		int spsCount = *(src++) & 0x1F;
		int ppsCount = -1;

		while (src + 2 < src_end) {
			if (spsCount == 0) {
				spsCount = -1;
				ppsCount = *(src++);
				continue;
			}

			if (spsCount > 0) {
				spsCount--;
			}
			else if (ppsCount > 0) {
				ppsCount--;
			}
			else {
				break;
			}

			int len = (src[0] << 8 | src[1]) + 2;
			if (src + len > src_end) {
				ASSERT(0);
				break;
			}
			memcpy(dst, src, len);
			src += len;
			dst += len;
			cbSequenceHeader += len;
		}
	}

	void CreateSequenceHeaderHEVC(BYTE* data, int size, DWORD* dwSequenceHeader, DWORD& cbSequenceHeader)
	{
		// copy NAL units from HEVCDecoderConfigurationRecord

		cbSequenceHeader = 0;
		if (size < 23) {
			return;
		}

		int numOfArrays = data[22];

		BYTE* src = data + 23;
		BYTE* dst = (BYTE*)dwSequenceHeader;
		BYTE* src_end = data + size;

		for (int j = 0; j < numOfArrays; j++) {
			if (src + 3 > src_end) {
				ASSERT(0);
				break;
			}
			int NAL_unit_type = src[0] & 0x3f;
			int numNalus = src[1] << 8 | src[2];
			src += 3;
			for (int i = 0; i < numNalus; i++) {
				int len = (src[0] << 8 | src[1]) + 2;
				if (src + len > src_end) {
					ASSERT(0);
					break;
				}
				memcpy(dst, src, len);
				src += len;
				dst += len;
				cbSequenceHeader += len;
			}
		}
	}

	static bool ParsePtl(CGolombBuffer& gb, int max_sub_layers_minus1, vc_params_t& params)
	{
		gb.BitRead(2);					// general_profile_space
		gb.BitRead(1);					// general_tier_flag
		params.profile = gb.BitRead(5);	// general_profile_idc
		gb.BitRead(32);					// general_profile_compatibility_flag[32]
		gb.BitRead(1);					// general_progressive_source_flag
		gb.BitRead(1);					// general_interlaced_source_flag
		gb.BitRead(1);					// general_non_packed_constraint_flag
		gb.BitRead(1);					// general_frame_only_constraint_flag
		gb.BitRead(44);					// general_reserved_zero_44bits
		params.level = gb.BitRead(8);	// general_level_idc
		uint8 sub_layer_profile_present_flag[6] = { 0 };
		uint8 sub_layer_level_present_flag[6] = { 0 };
		for (int i = 0; i < max_sub_layers_minus1; i++) {
			sub_layer_profile_present_flag[i] = gb.BitRead(1);
			sub_layer_level_present_flag[i] = gb.BitRead(1);
		}
		if (max_sub_layers_minus1 > 0) {
			for (int i = max_sub_layers_minus1; i < 8; i++) {
				uint8 reserved_zero_2bits = gb.BitRead(2);
			}
		}
		for (int i = 0; i < max_sub_layers_minus1; i++) {
			if (sub_layer_profile_present_flag[i]) {
				gb.BitRead(2);	// sub_layer_profile_space[i]
				gb.BitRead(1);	// sub_layer_tier_flag[i]
				gb.BitRead(5);	// sub_layer_profile_idc[i]
				gb.BitRead(32);	// sub_layer_profile_compatibility_flag[i][32]
				gb.BitRead(1);	// sub_layer_progressive_source_flag[i]
				gb.BitRead(1);	// sub_layer_interlaced_source_flag[i]
				gb.BitRead(1);	// sub_layer_non_packed_constraint_flag[i]
				gb.BitRead(1);	// sub_layer_frame_only_constraint_flag[i]
				gb.BitRead(44);	// sub_layer_reserved_zero_44bits[i]
			}
			if (sub_layer_level_present_flag[i]) {
				gb.BitRead(8);	// sub_layer_level_idc[i]
			}
		}

		return true;
	}

#define MAX_SUB_LAYERS 6
#define MAX_VPS_COUNT 16
#define MAX_SPS_COUNT 32

#define MAX_REFS 16
#define MAX_SHORT_TERM_RPS_COUNT 64

	struct ShortTermRPS {
		unsigned int num_negative_pics;
		int num_delta_pocs;
		int32 delta_poc[32];
		uint8 used[32];
	};
	ShortTermRPS st_rps[MAX_SHORT_TERM_RPS_COUNT];

	static bool ParseShortTermRps(CGolombBuffer& gb, ShortTermRPS *rps)
	{
		uint8 rps_predict = 0;
		int delta_poc;
		int k0 = 0;
		int k1 = 0;
		int k = 0;
		int i;

		if (st_rps != rps) {
			rps_predict = gb.BitRead(1);
		}

		if (rps_predict) {
			const ShortTermRPS *rps_ridx;
			int delta_rps;
			unsigned abs_delta_rps;
			uint8 use_delta_flag = 0;
			uint8 delta_rps_sign;

			rps_ridx = &st_rps[rps - st_rps - 1];

			delta_rps_sign = gb.BitRead(1);
			abs_delta_rps = gb.UExpGolombRead() + 1;
			if (abs_delta_rps < 1 || abs_delta_rps > 32768) {
				return false;
			}
			delta_rps = (1 - (delta_rps_sign << 1)) * abs_delta_rps;
			for (i = 0; i <= rps_ridx->num_delta_pocs; i++) {
				int used = rps->used[k] = gb.BitRead(1);

				if (!used)
					use_delta_flag = gb.BitRead(1);

				if (used || use_delta_flag) {
					if (i < rps_ridx->num_delta_pocs)
						delta_poc = delta_rps + rps_ridx->delta_poc[i];
					else
						delta_poc = delta_rps;
					rps->delta_poc[k] = delta_poc;
					if (delta_poc < 0)
						k0++;
					else
						k1++;
					k++;
				}
			}

			rps->num_delta_pocs = k;
			rps->num_negative_pics = k0;
			// sort in increasing order (smallest first)
			if (rps->num_delta_pocs != 0) {
				int used, tmp;
				for (i = 1; i < rps->num_delta_pocs; i++) {
					delta_poc = rps->delta_poc[i];
					used = rps->used[i];
					for (k = i - 1; k >= 0; k--) {
						tmp = rps->delta_poc[k];
						if (delta_poc < tmp) {
							rps->delta_poc[k + 1] = tmp;
							rps->used[k + 1] = rps->used[k];
							rps->delta_poc[k] = delta_poc;
							rps->used[k] = used;
						}
					}
				}
			}
			if ((rps->num_negative_pics >> 1) != 0) {
				int used;
				k = rps->num_negative_pics - 1;
				// flip the negative values to largest first
				for (unsigned int i = 0; i < rps->num_negative_pics >> 1; i++) {
					delta_poc = rps->delta_poc[i];
					used = rps->used[i];
					rps->delta_poc[i] = rps->delta_poc[k];
					rps->used[i] = rps->used[k];
					rps->delta_poc[k] = delta_poc;
					rps->used[k] = used;
					k--;
				}
			}
		}
		else {
			int delta_poc = 0;
			unsigned int prev, nb_positive_pics;
			rps->num_negative_pics = gb.UExpGolombRead();
			nb_positive_pics = gb.UExpGolombRead();

			if (rps->num_negative_pics >= MAX_REFS ||
				nb_positive_pics >= MAX_REFS) {
				return false;
			}

			rps->num_delta_pocs = rps->num_negative_pics + nb_positive_pics;
			if (rps->num_delta_pocs) {
				prev = 0;
				for (unsigned int i = 0; i < rps->num_negative_pics; i++) {
					delta_poc = gb.UExpGolombRead() + 1;
					prev -= delta_poc;
					rps->delta_poc[i] = prev;
					rps->used[i] = gb.BitRead(1);
				}
				prev = 0;
				for (unsigned int i = 0; i < nb_positive_pics; i++) {
					delta_poc = gb.UExpGolombRead() + 1;
					prev += delta_poc;
					rps->delta_poc[rps->num_negative_pics + i] = prev;
					rps->used[rps->num_negative_pics + i] = gb.BitRead(1);
				}
			}
		}

		return true;
	}

	bool ParseSequenceParameterSet(BYTE* data, int size, vc_params_t& params)
	{
		// Recommendation H.265 (04/13) ( http://www.itu.int/rec/T-REC-H.265-201304-I )
		// 7.3.2.2  Sequence parameter set RBSP syntax
		// 7.3.3  Profile, tier and level syntax
		if (size < 20) { // 8 + 12
			return false;
		}

		CGolombBuffer gb(data, size, true);

		// seq_parameter_set_rbsp()
		int vps_id = gb.BitRead(4);		// sps_video_parameter_set_id
		if (vps_id >= MAX_VPS_COUNT) {
			return false;
		}
		int sps_max_sub_layers_minus1 = gb.BitRead(3); // "The value of sps_max_sub_layers_minus1 shall be in the range of 0 to 6, inclusive."
		if (sps_max_sub_layers_minus1 > MAX_SUB_LAYERS) {
			return false;
		}
		gb.BitRead(1);		// sps_temporal_id_nesting_flag

		// profile_tier_level( sps_max_sub_layers_minus1 )
		ParsePtl(gb, sps_max_sub_layers_minus1, params);

		if (gb.IsEOF()) {
			return false;
		}

		uint32 sps_seq_parameter_set_id = gb.UExpGolombRead(); // "The  value  of sps_seq_parameter_set_id shall be in the range of 0 to 15, inclusive."
		if (sps_seq_parameter_set_id > MAX_SPS_COUNT) {
			return false;
		}
		uint32 chroma_format_idc = gb.UExpGolombRead(); // "The value of chroma_format_idc shall be in the range of 0 to 3, inclusive."
		if (!(chroma_format_idc == 1 || chroma_format_idc == 2 || chroma_format_idc == 3)) {
			return false;
		}
		if (chroma_format_idc == 3) {
			gb.BitRead(1);				// separate_colour_plane_flag
		}
		uint32 width = gb.UExpGolombRead();	// pic_width_in_luma_samples
		uint32 height = gb.UExpGolombRead();	// pic_height_in_luma_samples
		uint32 pic_conf_win_left_offset = 0;
		uint32 pic_conf_win_right_offset = 0;
		uint32 pic_conf_win_top_offset = 0;
		uint32 pic_conf_win_bottom_offset = 0;

		if (gb.BitRead(1)) {			// conformance_window_flag
			pic_conf_win_left_offset = gb.UExpGolombRead() * 2;
			pic_conf_win_right_offset = gb.UExpGolombRead() * 2;
			pic_conf_win_top_offset = gb.UExpGolombRead() * 2;
			pic_conf_win_bottom_offset = gb.UExpGolombRead() * 2;
		}

		params.width = width - (pic_conf_win_left_offset + pic_conf_win_right_offset);
		params.height = height - (pic_conf_win_top_offset + pic_conf_win_bottom_offset);
		if (params.width <= 0 || params.height <= 0) {
			params.width = width;
			params.height = height;
		}

		if (!params.width || !params.height) {
			return false;
		}

		uint32 bit_depth_luma_minus8 = gb.UExpGolombRead();
		uint32 bit_depth_chroma_minus8 = gb.UExpGolombRead();
		if (bit_depth_luma_minus8 != bit_depth_chroma_minus8) {
			return false;
		}

		uint32 log2_max_poc_lsb = gb.UExpGolombRead() + 4;
		if (log2_max_poc_lsb > 16) {
			return false;
		}

		uint8 sublayer_ordering_info = gb.BitRead(1);
		int start = sublayer_ordering_info ? 0 : sps_max_sub_layers_minus1;
		for (int i = start; i <= sps_max_sub_layers_minus1; i++) {
			uint32 max_dec_pic_buffering_minus1 = gb.UExpGolombRead();
			if (max_dec_pic_buffering_minus1 >= 16) {
				return false;
			}
			gb.UExpGolombRead(); // num_reorder_pics
			gb.UExpGolombRead(); // max_latency_increase
		}

		if (gb.IsEOF()) {
			return false;
		}

		uint32 log2_min_cb_size = gb.UExpGolombRead() + 3;
		gb.UExpGolombRead(); // log2_diff_max_min_coding_block_size
		uint32 log2_min_tb_size = gb.UExpGolombRead() + 2;
		gb.UExpGolombRead(); // log2_diff_max_min_transform_block_size
		if (log2_min_tb_size >= log2_min_cb_size) {
			return false;
		}

		gb.UExpGolombRead(); // max_transform_hierarchy_depth_inter
		gb.UExpGolombRead(); // max_transform_hierarchy_depth_intra

		if (gb.BitRead(1)) {     // scaling_list_enable_flag
			if (gb.BitRead(1)) { // scaling_list_data
				uint8 scaling_list_pred_mode_flag = 0;
				int coef_num, i;
				for (int size_id = 0; size_id < 4; size_id++) {
					for (uint32 matrix_id = 0; matrix_id < 6; matrix_id += ((size_id == 3) ? 3 : 1)) {
						scaling_list_pred_mode_flag = gb.BitRead(1);
						if (!scaling_list_pred_mode_flag) {
							uint32 delta = gb.UExpGolombRead();
							if (matrix_id < delta) {
								return false;
							}
						} else {
							coef_num = min(64, 1 << (4 + (size_id << 1)));
							if (size_id > 1) {
								gb.SExpGolombRead();
							}
							for (i = 0; i < coef_num; i++) {
								gb.SExpGolombRead();
							}
						}
					}
				}
			}
		}

		if (gb.IsEOF()) {
			return false;
		}

		gb.BitRead(1); // amp_enabled_flag
		gb.BitRead(1); // sao_enabled

		uint8 pcm_enabled_flag = gb.BitRead(1);
		if (pcm_enabled_flag) {
			uint8 bit_depth_minus1 = gb.BitRead(4);
			gb.BitRead(4);	// bit_depth_chroma_minus1
			gb.UExpGolombRead();		// log2_min_pcm_cb_size
			gb.UExpGolombRead();		// log2_max_pcm_cb_size
			if ((bit_depth_minus1 + 1u) > (bit_depth_luma_minus8 + 8u)) {
				return false;
			}
		}

		uint32 nb_st_rps = gb.UExpGolombRead();
		if (nb_st_rps > MAX_SHORT_TERM_RPS_COUNT) {
			return false;
		}

		memset(st_rps, 0, sizeof(st_rps));
		for (uint32 i = 0; i < nb_st_rps; i++) {
			if (!ParseShortTermRps(gb, &st_rps[i])) {
				return false;
			}
		}

		if (gb.IsEOF()) {
			return false;
		}

		uint8 long_term_ref_pics_present_flag = gb.BitRead(1);
		if (long_term_ref_pics_present_flag) {
			uint32 num_long_term_ref_pics_sps = gb.UExpGolombRead();
			if (num_long_term_ref_pics_sps > 31U) {
				return false;
			}
			for (uint32 i = 0; i < num_long_term_ref_pics_sps; i++) {
				gb.BitRead(log2_max_poc_lsb);	// lt_ref_pic_poc_lsb_sps
				gb.BitRead(1);					// used_by_curr_pic_lt_sps_flag
			}
		}

		gb.BitRead(1); // sps_temporal_mvp_enabled_flag
		gb.BitRead(1); // sps_strong_intra_smoothing_enable_flag

		uint32 vui_present = gb.BitRead(1);
		if (vui_present) {
			uint8 sar_present = gb.BitRead(1);
			if (sar_present) {
				uint8 sar_idx = gb.ReadByte();
				if (255 == sar_idx) {
					params.sar.num = gb.BitRead(16);	// sar_width
					params.sar.den = gb.BitRead(16);	// sar_height
				}
				else if (sar_idx < _countof(pixel_aspect)) {
					params.sar.num = pixel_aspect[sar_idx][0];
					params.sar.den = pixel_aspect[sar_idx][1];
				}
				else {
					;
				}
			}

			uint8 overscan_info_present_flag = gb.BitRead(1);
			if (overscan_info_present_flag) {
				gb.BitRead(1); // overscan_appropriate_flag
			}

			uint8 video_signal_type_present_flag = gb.BitRead(1);
			if (video_signal_type_present_flag) {
				gb.BitRead(3);	// video_format
				gb.BitRead(1);	// video_full_range_flag
				uint8 colour_description_present_flag = gb.BitRead(1);
				if (colour_description_present_flag) {
					gb.ReadByte(); // colour_primaries
					gb.ReadByte(); // transfer_characteristic
					gb.ReadByte(); // matrix_coeffs
				}
			}

			uint8 chroma_loc_info_present_flag = gb.BitRead(1);
			if (chroma_loc_info_present_flag) {
				gb.BitRead(1); // chroma_sample_loc_type_top_field
				gb.BitRead(1); // chroma_sample_loc_type_bottom_field
			}

			gb.BitRead(1); // neutra_chroma_indication_flag
			gb.BitRead(1); // field_seq_flag
			gb.BitRead(1); // frame_field_info_present_flag

			uint8 default_display_window_flag = gb.BitRead(1);
			if (default_display_window_flag) {
				gb.UExpGolombRead(); // def_disp_win_left_offset
				gb.UExpGolombRead(); // def_disp_win_right_offset
				gb.UExpGolombRead(); // def_disp_win_top_offset
				gb.UExpGolombRead(); // def_disp_win_bottom_offset
			}

			uint8 vui_timing_info_present_flag = gb.BitRead(1);
			if (vui_timing_info_present_flag) {
				params.vui_timing.num_units_in_tick = gb.BitRead(32);
				params.vui_timing.time_scale = gb.BitRead(32);
			}
		}

		return true;
	}

	bool ParseVideoParameterSet(BYTE* data, int size, vc_params_t& params)
	{
		CGolombBuffer gb(data, size, true);

		int vps_id = gb.BitRead(4);
		if (vps_id >= MAX_VPS_COUNT) {
			return false;
		}
		if (gb.BitRead(2) != 3) { // vps_reserved_three_2bits
			return false;
		}
		gb.BitRead(6); // vps_max_layers_minus1
		int vps_max_sub_layers_minus1 = gb.BitRead(3);
		if (vps_max_sub_layers_minus1 > MAX_SUB_LAYERS) {
			return false;
		}
		gb.BitRead(1);   // vps_temporal_id_nesting_flag

		if (gb.BitRead(16) != 0xffff) { // vps_reserved_ffff_16bits
			return false;
		}

		// profile_tier_level( vps_max_sub_layers_minus1 )
		ParsePtl(gb, vps_max_sub_layers_minus1, params);

		int vps_sub_layer_ordering_info_present_flag = gb.BitRead(1);
		int i = vps_sub_layer_ordering_info_present_flag ? 0 : vps_max_sub_layers_minus1;
		for (; i <= vps_max_sub_layers_minus1; i++) {
			gb.UExpGolombRead();
			gb.UExpGolombRead();
			gb.UExpGolombRead();
		}

		int vps_max_layer_id = gb.BitRead(6);
		int vps_num_layer_sets_minus1 = gb.UExpGolombRead();
		for (i = 1; i <= vps_num_layer_sets_minus1; i++) {
			for (int j = 0; j <= vps_max_layer_id; j++) {
				gb.BitRead(1); // layer_id_included_flag[i][j]
			}
		}

		uint8 vps_timing_info_present_flag = gb.BitRead(1);
		if (vps_timing_info_present_flag) {
			params.vps_timing.num_units_in_tick = gb.BitRead(32);
			params.vps_timing.time_scale = gb.BitRead(32);
		}

		return true;
	}

	bool ParseSequenceParameterSetHM91(BYTE* data, int size, vc_params_t& params)
	{
		// decode SPS
		CGolombBuffer gb(data, size, true);
		gb.BitRead(4);						// video_parameter_set_id
		int sps_max_sub_layers_minus1 = (int)gb.BitRead(3);

		gb.BitRead(1);						// sps_temporal_id_nesting_flag

		// profile_tier_level( 1, sps_max_sub_layers_minus1 )
		{
			int i;

			gb.BitRead(2);					// XXX_profile_space[]
			gb.BitRead(1);					// XXX_tier_flag[]
			params.profile = gb.BitRead(5);	// XXX_profile_idc[]
			gb.BitRead(32);					// XXX_profile_compatibility_flag[][32]

			// HM9.1
			gb.BitRead(16);					// XXX_reserved_zero_16bits[]

			params.level = gb.BitRead(8);	// general_level_idc

			// HM9.1
			for (i = 0; i < sps_max_sub_layers_minus1; i++) {
				int sub_layer_profile_present_flag, sub_layer_level_present_flag;
				sub_layer_profile_present_flag = (int)gb.BitRead(1);	// sub_layer_profile_present_flag[i]
				sub_layer_level_present_flag = (int)gb.BitRead(1);		// sub_layer_level_present_flag[i]

				if (sub_layer_profile_present_flag) {
					gb.BitRead(2);			// XXX_profile_space[]
					gb.BitRead(1);			// XXX_tier_flag[]
					gb.BitRead(5);			// XXX_profile_idc[]
					gb.BitRead(32);			// XXX_profile_compatibility_flag[][32]
					gb.BitRead(16);			// XXX_reserved_zero_16bits[]
				}
				if (sub_layer_level_present_flag) {
					gb.BitRead(8);			// sub_layer_level_idc[i]
				}
			}
		}

		gb.UExpGolombRead();							// seq_parameter_set_id

		int chroma_format_idc = (int)gb.UExpGolombRead(); // chroma_format_idc
		if (chroma_format_idc == 3) {
			gb.BitRead(1);					// separate_colour_plane_flag
		}

		params.width = gb.UExpGolombRead();
		params.height = gb.UExpGolombRead();

		return true;
	}

	bool ParseAVCDecoderConfigurationRecord(BYTE* data, int size, vc_params_t& params, int flv_hm)
	{
		params.clear();
		if (size < 7) {
			return false;
		}
		CGolombBuffer gb(data, size);
		if (gb.BitRead(8) != 1) {		// configurationVersion = 1
			return false;
		}
		gb.BitRead(8);					// AVCProfileIndication
		gb.BitRead(8);					// profile_compatibility
		gb.BitRead(8);					// AVCLevelIndication;
		if (gb.BitRead(6) != 63) {		// reserved = ‘111111’b
			return false;
		}
		params.nal_length_size = gb.BitRead(2) + 1;	// lengthSizeMinusOne
		if (gb.BitRead(3) != 7) {		// reserved = ‘111’b
			return false;
		}
		int numOfSequenceParameterSets = gb.BitRead(5);
		for (int i = 0; i < numOfSequenceParameterSets; i++) {
			int sps_len = gb.BitRead(16);	// sequenceParameterSetLength

			//
			if (flv_hm >= 90 && sps_len > 2) {
				BYTE* sps_data = gb.GetBufferPos();
				if ((*sps_data >> 1 & 0x3f) == (BYTE)NAL_TYPE_HEVC_SPS) {
					if (flv_hm >= 100) {
						return ParseSequenceParameterSet(sps_data + 2, sps_len - 2, params);
					}
					else if (flv_hm >= 90) {
						return ParseSequenceParameterSetHM91(sps_data + 2, sps_len - 2, params);
					}
				}
			}

			gb.SkipBytes(sps_len);			// sequenceParameterSetNALUnit
		}
		int numOfPictureParameterSets = gb.BitRead(8);
		for (int i = 0; i < numOfPictureParameterSets; i++) {
			int pps_len = gb.BitRead(16);	// pictureParameterSetLength
			gb.SkipBytes(pps_len);			// pictureParameterSetNALUnit
		}
		//  if( profile_idc  ==  100  ||  profile_idc  ==  110  ||
		//      profile_idc  ==  122  ||  profile_idc  ==  144 )
		//  {
		//    bit(6) reserved = ‘111111’b;
		//    unsigned int(2) chroma_format;
		//    bit(5) reserved = ‘11111’b;
		//    unsigned int(3) bit_depth_luma_minus8;
		//    bit(5) reserved = ‘11111’b;
		//    unsigned int(3) bit_depth_chroma_minus8;
		//    unsigned int(8) numOfSequenceParameterSetExt;
		//    for (i=0; i< numOfSequenceParameterSetExt; i++) {
		//      unsigned int(16) sequenceParameterSetExtLength;
		//      bit(8*sequenceParameterSetExtLength) sequenceParameterSetExtNALUnit;
		//    }
		//  }

		return true;
	}

	bool ParseHEVCDecoderConfigurationRecord(BYTE* data, int size, vc_params_t& params, bool parseSPS)
	{
		// ISO/IEC 14496-15 Third edition (2013-xx-xx)
		// 8.3.3.1  HEVC decoder configuration record

		params.clear();
		if (size < 23) {
			return false;
		}
		CGolombBuffer gb(data, size);

		// HEVCDecoderConfigurationRecord
		uint8 configurationVersion = gb.BitRead(8); // configurationVersion = 1 (or 0 for beta MKV DivX HEVC)
		TRACE(L"%s", configurationVersion == 0 ? L"WARNING: beta MKV DivX HEVC\n" : L"");
		if (configurationVersion > 1) {
			return false;
		}
		gb.BitRead(2);					// general_profile_space
		gb.BitRead(1);					// general_tier_flag
		params.profile = gb.BitRead(5);	// general_profile_idc
		gb.BitRead(32);					// general_profile_compatibility_flags
		gb.BitRead(48);					// general_constraint_indicator_flags
		params.level = gb.BitRead(8);	// general_level_idc
		if (gb.BitRead(4) != 15) {		// reserved = ‘1111’b
			return false;
		}
		gb.BitRead(12);				// min_spatial_segmentation_idc
		if (gb.BitRead(6) != 63) {	// reserved = ‘111111’b
			return false;
		}
		gb.BitRead(2);				// parallelismType
		if (gb.BitRead(6) != 63) {	// reserved = ‘111111’b
			return false;
		}
		uint8 chromaFormat = gb.BitRead(2); // 0 = monochrome, 1 = 4:2:0, 2 = 4:2:2, 3 = 4:4:4
		if (gb.BitRead(5) != 31) {	// reserved = ‘11111’b
			return false;
		}
		uint8 bitDepthLumaMinus8 = gb.BitRead(3);
		if (gb.BitRead(5) != 31) {	// reserved = ‘11111’b
			return false;
		}
		uint8 bitDepthChromaMinus8 = gb.BitRead(3);
		gb.BitRead(16);				// avgFrameRate
		gb.BitRead(2);				// constantFrameRate
		gb.BitRead(3);				// numTemporalLayers
		gb.BitRead(1);				// temporalIdNested
		params.nal_length_size = gb.BitRead(2) + 1;	// lengthSizeMinusOne
		int numOfArrays = gb.BitRead(8);

		if (parseSPS) {
			int sps_len = 0;

			for (int j = 0; j < numOfArrays; j++) {
				gb.BitRead(1);						// array_completeness
				uint8 reserved = gb.BitRead(1);	// reserved = 0 (or 1 for MKV DivX HEVC)
				int NAL_unit_type = gb.BitRead(6);
				int numNalus = gb.BitRead(16);
				if (NAL_unit_type == NAL_TYPE_HEVC_SPS && numNalus > 0) {
					sps_len = gb.BitRead(16);
					break;
				}
				for (int i = 0; i < numNalus; i++) {
					int nalUnitLength = gb.BitRead(16);
					gb.SkipBytes(nalUnitLength);
				}
			}

			if (sps_len) {
				return ParseSequenceParameterSet(gb.GetBufferPos(), sps_len, params);
			}

			return false;
		}

		return true;
	}
} // namespace HEVCParser

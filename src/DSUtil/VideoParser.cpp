/*
 * (C) 2012-2025 see Authors.txt
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

#include <stdint.h>
#include "H264Nalu.h"
#include "GolombBuffer.h"
#include "VideoParser.h"
#include "Log.h"

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

static const fraction_t avpriv_frame_rate_tab[16] = {
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

static const fraction_t dirac_frame_rate[] = {
	{15000, 1001},
	{25, 2},
};

bool ParseDiracHeader(const BYTE* data, const int size, vc_params_t& params)
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

	fraction_t frame_rate = {0,0};
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

#define H264_PROFILE_MULTIVIEW_HIGH	118
#define H264_PROFILE_STEREO_HIGH	128
	bool ParseSequenceParameterSet(const BYTE* data, const int size, vc_params_t& params)
	{
		static BYTE profiles[] = { 44, 66, 77, 88, 100, 110, 118, 122, 128, 144, 244 };
		static BYTE levels[] = { 10, 11, 12, 13, 20, 21, 22, 30, 31, 32, 40, 41, 42, 50, 51, 52, 60, 61, 62 };

		CGolombBuffer gb(data, size, true);
		params.clear();

		do {
			params.profile = gb.BitRead(8);
			if (std::find(std::cbegin(profiles), std::cend(profiles), params.profile) == std::cend(profiles)) {
				break;
			}

			gb.BitRead(8);
			params.level = gb.BitRead(8);
			if (std::find(std::cbegin(levels), std::cend(levels), params.level) == std::cend(levels)) {
				break;
			}

			UINT64 sps_id = gb.UExpGolombRead();	// seq_parameter_set_id
			if (sps_id >= 32) {
				break;
			}

			UINT64 chroma_format_idc = 1;
			if (params.profile >= 100) {			// high profile
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
			}
			else if (pic_order_cnt_type == 1) {
				gb.BitRead(1);						// delta_pic_order_always_zero_flag
				gb.SExpGolombRead();				// offset_for_non_ref_pic
				gb.SExpGolombRead();				// offset_for_top_to_bottom_field
				UINT64 num_ref_frames_in_pic_order_cnt_cycle = gb.UExpGolombRead();
				if (num_ref_frames_in_pic_order_cnt_cycle >= 256) {
					break;
				}
				for (unsigned i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++) {
					gb.SExpGolombRead();			// offset_for_ref_frame[i]
				}
			}
			else if (pic_order_cnt_type != 2) {
				break;
			}

			UINT64 ref_frame_count = gb.UExpGolombRead(); // num_ref_frames
			if (ref_frame_count > 30) {
				break;
			}
			gb.BitRead(1);							// gaps_in_frame_num_value_allowed_flag

			UINT64 pic_width_in_mbs_minus1 = gb.UExpGolombRead();
			UINT64 pic_height_in_map_units_minus1 = gb.UExpGolombRead();
			params.interlaced = !gb.BitRead(1);

			if (params.interlaced) {				// !frame_mbs_only_flag
				gb.BitRead(1);						// mb_adaptive_frame_field_flag
			}

			BYTE direct_8x8_inference_flag = (BYTE)gb.BitRead(1); // direct_8x8_inference_flag
			if (params.interlaced && !direct_8x8_inference_flag) {
				break;
			}

			UINT crop_left, crop_right, crop_top, crop_bottom;
			const bool frame_cropping_flag = gb.BitRead(1);
			if (frame_cropping_flag) {				// frame_cropping_flag
				crop_left   = gb.UExpGolombRead();	// frame_cropping_rect_left_offset
				crop_right  = gb.UExpGolombRead();	// frame_cropping_rect_right_offset
				crop_top    = gb.UExpGolombRead();	// frame_cropping_rect_top_offset
				crop_bottom = gb.UExpGolombRead();	// frame_cropping_rect_bottom_offset
			} else {
				crop_left = crop_right = crop_top = crop_bottom = 0;
			}

			if (gb.BitRead(1)) {					// vui_parameters_present_flag
				if (gb.BitRead(1)) {				// aspect_ratio_info_present_flag
					BYTE aspect_ratio_idc = (BYTE)gb.BitRead(8); // aspect_ratio_idc
					if (255 == aspect_ratio_idc) {
						params.sar.num = (WORD)gb.BitRead(16); // sar_width
						params.sar.den = (WORD)gb.BitRead(16); // sar_height
					}
					else if (aspect_ratio_idc < std::size(pixel_aspect)) {
						params.sar.num = pixel_aspect[aspect_ratio_idc][0];
						params.sar.den = pixel_aspect[aspect_ratio_idc][1];
					} else {
						break;
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

					if (fixed_frame_rate_flag
						&& num_units_in_tick && time_scale) {
						params.AvgTimePerFrame = (REFERENCE_TIME)(10000000.0 * num_units_in_tick * 2 / time_scale);
					}
				}

				bool nalflag = !!gb.BitRead(1);		// nal_hrd_parameters_present_flag
				if (nalflag) {
					if (!ParseHrdParameters(gb)) {
						break;
					}
				}
				bool vlcflag = !!gb.BitRead(1);		// vlc_hrd_parameters_present_flag
				if (vlcflag) {
					if (!ParseHrdParameters(gb)) {
						break;
					}
				}
				if (nalflag || vlcflag) {
					gb.BitRead(1);					// low_delay_hrd_flag
				}

				gb.BitRead(1);						// pic_struct_present_flag
				if (!gb.IsEOF() && gb.BitRead(1)) {	// bitstream_restriction_flag
					gb.BitRead(1);					// motion_vectors_over_pic_boundaries_flag
					gb.UExpGolombRead();			// max_bytes_per_pic_denom
					gb.UExpGolombRead();			// max_bits_per_mb_denom
					gb.UExpGolombRead();			// log2_max_mv_length_horizontal
					gb.UExpGolombRead();			// log2_max_mv_length_vertical

					UINT64 num_reorder_frames = 0;
					if (!gb.IsEOF()) {
						num_reorder_frames = gb.UExpGolombRead(); // num_reorder_frames
						gb.UExpGolombRead();		// max_dec_frame_buffering
					}
					if (num_reorder_frames > 16U) {
						break;
					}
				}
			}

			if (!gb.IsEOF() &&
				(params.profile == H264_PROFILE_MULTIVIEW_HIGH ||
					params.profile == H264_PROFILE_STEREO_HIGH)) {
				UINT8 bit_equal_to_one = gb.BitRead(1);	// bit_equal_to_one
				if (!bit_equal_to_one) {
					break;
				}
				gb.UExpGolombRead();					// num_views_minus1
			}

			if (!params.sar.num || !params.sar.den) {
				params.sar.num = params.sar.den = 1;
			}

			params.width = (pic_width_in_mbs_minus1 + 1) * 16;
			params.height = ((pic_height_in_map_units_minus1 + 1) * (2 - !params.interlaced)) * 16;

			if (frame_cropping_flag) {
				const auto vsub = (chroma_format_idc == 1) ? 1 : 0;
				const auto hsub = (chroma_format_idc == 1 || chroma_format_idc == 2) ? 1 : 0;
				const auto step_x = 1 << hsub;
				const auto step_y = (2 - !params.interlaced) << vsub;

				params.width -= (crop_left + crop_right) * step_x;
				params.height -= (crop_top + crop_bottom) * step_y;
			}

			if (params.height < 100 || params.width < 100) {
				break;
			}

			if (params.height == 1088) {
				params.height = 1080;	// Prevent blur lines
			}

			return true;
		} while (0);

		params.clear();
		return false;
	}
} // namespace AVCParser

namespace HEVCParser {
	void CreateSequenceHeaderAVC(const BYTE* data, const int size, DWORD* dwSequenceHeader, DWORD& cbSequenceHeader)
	{
		// copy VideoParameterSets(VPS), SequenceParameterSets(SPS) and PictureParameterSets(PPS) from AVCDecoderConfigurationRecord

		cbSequenceHeader = 0;
		if (size < 7 || (data[5] & 0xe0) != 0xe0) {
			return;
		}

		BYTE* src = (BYTE*)data + 5;
		BYTE* dst = (BYTE*)dwSequenceHeader;
		const BYTE* src_end = data + size;
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

	void CreateSequenceHeaderHEVC(const BYTE* data, const int size, DWORD* dwSequenceHeader, DWORD& cbSequenceHeader)
	{
		// copy NAL units from HEVCDecoderConfigurationRecord

		cbSequenceHeader = 0;
		if (size < 23) {
			return;
		}

		int numOfArrays = data[22];

		BYTE* src = (BYTE*)data + 23;
		BYTE* dst = (BYTE*)dwSequenceHeader;
		const BYTE* src_end = data + size;

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

	static bool ParsePtl(CGolombBuffer& gb, const int max_sub_layers_minus1, vc_params_t& params)
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
		uint8_t sub_layer_profile_present_flag[6] = { 0 };
		uint8_t sub_layer_level_present_flag[6] = { 0 };
		for (int i = 0; i < max_sub_layers_minus1; i++) {
			sub_layer_profile_present_flag[i] = gb.BitRead(1);
			sub_layer_level_present_flag[i] = gb.BitRead(1);
		}
		if (max_sub_layers_minus1 > 0) {
			for (int i = max_sub_layers_minus1; i < 8; i++) {
				uint8_t reserved_zero_2bits = gb.BitRead(2);
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
#define MAX_DPB_SIZE 16
#define MAX_SHORT_TERM_RPS_COUNT 64

	struct ShortTermRPS {
		unsigned int num_negative_pics;
		int num_delta_pocs;
		int32_t delta_poc[32];
		uint8_t used[32];
	};
	ShortTermRPS st_rps[MAX_SHORT_TERM_RPS_COUNT];

	static bool ParseShortTermRps(CGolombBuffer& gb, ShortTermRPS *rps)
	{
		uint8_t rps_predict = 0;
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
			uint8_t use_delta_flag = 0;
			uint8_t delta_rps_sign;

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
					int delta_poc;
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
					int delta_poc = rps->delta_poc[i];
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
				for (unsigned int j = 0; j < rps->num_negative_pics >> 1; j++) {
					int delta_poc = rps->delta_poc[j];
					used = rps->used[j];
					rps->delta_poc[j] = rps->delta_poc[k];
					rps->used[j] = rps->used[k];
					rps->delta_poc[k] = delta_poc;
					rps->used[k] = used;
					k--;
				}
			}
		}
		else {
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
				for (unsigned int j = 0; j < rps->num_negative_pics; j++) {
					int delta_poc = gb.UExpGolombRead() + 1;
					prev -= delta_poc;
					rps->delta_poc[j] = prev;
					rps->used[j] = gb.BitRead(1);
				}
				prev = 0;
				for (unsigned int j = 0; j < nb_positive_pics; j++) {
					int delta_poc = gb.UExpGolombRead() + 1;
					prev += delta_poc;
					rps->delta_poc[rps->num_negative_pics + j] = prev;
					rps->used[rps->num_negative_pics + j] = gb.BitRead(1);
				}
			}
		}

		return true;
	}

	bool ParseSequenceParameterSet(const BYTE* data, const int size, vc_params_t& params)
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

		uint32_t sps_seq_parameter_set_id = gb.UExpGolombRead(); // "The  value  of sps_seq_parameter_set_id shall be in the range of 0 to 15, inclusive."
		if (sps_seq_parameter_set_id > MAX_SPS_COUNT) {
			return false;
		}
		uint32_t chroma_format_idc = gb.UExpGolombRead(); // "The value of chroma_format_idc shall be in the range of 0 to 3, inclusive."
		if (!(chroma_format_idc == 1 || chroma_format_idc == 2 || chroma_format_idc == 3)) {
			return false;
		}
		if (chroma_format_idc == 3) {
			gb.BitRead(1);				// separate_colour_plane_flag
		}
		uint32_t width = gb.UExpGolombRead();	// pic_width_in_luma_samples
		uint32_t height = gb.UExpGolombRead();	// pic_height_in_luma_samples
		uint32_t pic_conf_win_left_offset = 0;
		uint32_t pic_conf_win_right_offset = 0;
		uint32_t pic_conf_win_top_offset = 0;
		uint32_t pic_conf_win_bottom_offset = 0;

		if (gb.BitRead(1)) {			// conformance_window_flag
			int vert_mult = 1 + (chroma_format_idc < 2);
			int horiz_mult = 1 + (chroma_format_idc < 3);
			pic_conf_win_left_offset = gb.UExpGolombRead() * horiz_mult;
			pic_conf_win_right_offset = gb.UExpGolombRead() * horiz_mult;
			pic_conf_win_top_offset = gb.UExpGolombRead() * vert_mult;
			pic_conf_win_bottom_offset = gb.UExpGolombRead() * vert_mult;
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

		uint32_t bit_depth_luma_minus8 = gb.UExpGolombRead();
		uint32_t bit_depth_chroma_minus8 = gb.UExpGolombRead();
		if (bit_depth_luma_minus8 != bit_depth_chroma_minus8) {
			return false;
		}

		uint32_t log2_max_poc_lsb = gb.UExpGolombRead() + 4;
		if (log2_max_poc_lsb > 16) {
			return false;
		}

		uint8_t sublayer_ordering_info = gb.BitRead(1);
		int start = sublayer_ordering_info ? 0 : sps_max_sub_layers_minus1;
		for (int i = start; i <= sps_max_sub_layers_minus1; i++) {
			uint32_t max_dec_pic_buffering_minus1 = gb.UExpGolombRead();
			if (max_dec_pic_buffering_minus1 >= MAX_DPB_SIZE) {
				return false;
			}
			uint32_t num_reorder_pics = gb.UExpGolombRead();
			if (num_reorder_pics >= MAX_DPB_SIZE) {
				return false;
			}
			gb.UExpGolombRead(); // max_latency_increase
		}

		if (gb.IsEOF()) {
			return false;
		}

		uint32_t log2_min_cb_size = gb.UExpGolombRead() + 3;
		gb.UExpGolombRead(); // log2_diff_max_min_coding_block_size
		uint32_t log2_min_tb_size = gb.UExpGolombRead() + 2;
		gb.UExpGolombRead(); // log2_diff_max_min_transform_block_size
		if (log2_min_tb_size >= log2_min_cb_size) {
			return false;
		}

		gb.UExpGolombRead(); // max_transform_hierarchy_depth_inter
		gb.UExpGolombRead(); // max_transform_hierarchy_depth_intra

		if (gb.BitRead(1)) {     // scaling_list_enable_flag
			if (gb.BitRead(1)) { // scaling_list_data
				uint8_t scaling_list_pred_mode_flag = 0;
				int coef_num, i;
				for (int size_id = 0; size_id < 4; size_id++) {
					for (uint32_t matrix_id = 0; matrix_id < 6; matrix_id += ((size_id == 3) ? 3 : 1)) {
						scaling_list_pred_mode_flag = gb.BitRead(1);
						if (!scaling_list_pred_mode_flag) {
							uint32_t delta = gb.UExpGolombRead();
							if (matrix_id < delta) {
								return false;
							}
						} else {
							coef_num = std::min(64, 1 << (4 + (size_id << 1)));
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

		uint8_t pcm_enabled_flag = gb.BitRead(1);
		if (pcm_enabled_flag) {
			uint8_t bit_depth_minus1 = gb.BitRead(4);
			gb.BitRead(4);       // bit_depth_chroma_minus1
			gb.UExpGolombRead(); // log2_min_pcm_cb_size
			gb.UExpGolombRead(); // log2_max_pcm_cb_size
			if ((bit_depth_minus1 + 1u) > (bit_depth_luma_minus8 + 8u)) {
				return false;
			}
			gb.BitRead(1);       // loop_filter_disable_flag
		}

		uint32_t nb_st_rps = gb.UExpGolombRead();
		if (nb_st_rps > MAX_SHORT_TERM_RPS_COUNT) {
			return false;
		}

		memset(st_rps, 0, sizeof(st_rps));
		for (uint32_t i = 0; i < nb_st_rps; i++) {
			if (!ParseShortTermRps(gb, &st_rps[i])) {
				return false;
			}
		}

		if (gb.IsEOF()) {
			return false;
		}

		uint8_t long_term_ref_pics_present_flag = gb.BitRead(1);
		if (long_term_ref_pics_present_flag) {
			uint32_t num_long_term_ref_pics_sps = gb.UExpGolombRead();
			if (num_long_term_ref_pics_sps > 31U) {
				return false;
			}
			for (uint32_t i = 0; i < num_long_term_ref_pics_sps; i++) {
				gb.BitRead(log2_max_poc_lsb);	// lt_ref_pic_poc_lsb_sps
				gb.BitRead(1);					// used_by_curr_pic_lt_sps_flag
			}
		}

		gb.BitRead(1); // sps_temporal_mvp_enabled_flag
		gb.BitRead(1); // sps_strong_intra_smoothing_enable_flag

		uint32_t vui_present = gb.BitRead(1);
		if (vui_present) {
			uint8_t sar_present = gb.BitRead(1);
			if (sar_present) {
				uint8_t sar_idx = gb.ReadByte();
				if (255 == sar_idx) {
					params.sar.num = gb.BitRead(16);	// sar_width
					params.sar.den = gb.BitRead(16);	// sar_height
				} else if (sar_idx < std::size(pixel_aspect)) {
					params.sar.num = pixel_aspect[sar_idx][0];
					params.sar.den = pixel_aspect[sar_idx][1];
				}
			}

			uint8_t overscan_info_present_flag = gb.BitRead(1);
			if (overscan_info_present_flag) {
				gb.BitRead(1); // overscan_appropriate_flag
			}

			uint8_t video_signal_type_present_flag = gb.BitRead(1);
			if (video_signal_type_present_flag) {
				gb.BitRead(3);	// video_format
				gb.BitRead(1);	// video_full_range_flag
				uint8_t colour_description_present_flag = gb.BitRead(1);
				if (colour_description_present_flag) {
					gb.ReadByte(); // colour_primaries
					gb.ReadByte(); // transfer_characteristic
					gb.ReadByte(); // matrix_coeffs
				}
			}

			uint8_t chroma_loc_info_present_flag = gb.BitRead(1);
			if (chroma_loc_info_present_flag) {
				gb.BitRead(1); // chroma_sample_loc_type_top_field
				gb.BitRead(1); // chroma_sample_loc_type_bottom_field
			}

			gb.BitRead(1); // neutra_chroma_indication_flag
			gb.BitRead(1); // field_seq_flag
			gb.BitRead(1); // frame_field_info_present_flag

			uint8_t default_display_window_flag = gb.BitRead(1);
			if (default_display_window_flag) {
				gb.UExpGolombRead(); // def_disp_win_left_offset
				gb.UExpGolombRead(); // def_disp_win_right_offset
				gb.UExpGolombRead(); // def_disp_win_top_offset
				gb.UExpGolombRead(); // def_disp_win_bottom_offset
			}

			uint8_t vui_timing_info_present_flag = gb.BitRead(1);
			if (vui_timing_info_present_flag) {
				params.vui_timing.num_units_in_tick = gb.BitRead(32);
				params.vui_timing.time_scale = gb.BitRead(32);
			}
		}

		if (!params.sar.num || !params.sar.den) {
			params.sar.num = params.sar.den = 1;
		}

		return true;
	}

	bool ParseVideoParameterSet(const BYTE* data, const int size, vc_params_t& params)
	{
		if (size < 15) {
			return false;
		}

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

		uint8_t vps_timing_info_present_flag = gb.BitRead(1);
		if (vps_timing_info_present_flag) {
			params.vps_timing.num_units_in_tick = gb.BitRead(32);
			params.vps_timing.time_scale = gb.BitRead(32);
		}

		return true;
	}

	bool ParseSequenceParameterSetHM91(const BYTE* data, const int size, vc_params_t& params)
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

	bool ParseAVCDecoderConfigurationRecord(const BYTE* data, const int size, vc_params_t& params, const int flv_hm/* = 0*/)
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
				const BYTE* sps_data = gb.GetBufferPos();
				if ((*sps_data >> 1 & 0x3f) == (BYTE)NALU_TYPE_HEVC_SPS) {
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

	bool ParseHEVCDecoderConfigurationRecord(const BYTE* data, const int size, vc_params_t& params, const bool parseSPS)
	{
		// ISO/IEC 14496-15 Third edition (2013-xx-xx)
		// 8.3.3.1  HEVC decoder configuration record

		params.clear();
		if (size < 23) {
			return false;
		}
		CGolombBuffer gb(data, size);

		// HEVCDecoderConfigurationRecord
		uint8_t configurationVersion = gb.BitRead(8); // configurationVersion = 1 (or 0 for beta MKV DivX HEVC)
		DLogIf(configurationVersion == 0, L"WARNING: beta MKV DivX HEVC");
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
		uint8_t chromaFormat = gb.BitRead(2); // 0 = monochrome, 1 = 4:2:0, 2 = 4:2:2, 3 = 4:4:4
		if (gb.BitRead(5) != 31) {	// reserved = ‘11111’b
			return false;
		}
		uint8_t bitDepthLumaMinus8 = gb.BitRead(3);
		if (gb.BitRead(5) != 31) {	// reserved = ‘11111’b
			return false;
		}
		uint8_t bitDepthChromaMinus8 = gb.BitRead(3);
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
				uint8_t reserved = gb.BitRead(1);	// reserved = 0 (or 1 for MKV DivX HEVC)
				int NAL_unit_type = gb.BitRead(6);
				int numNalus = gb.BitRead(16);
				if (NAL_unit_type == NALU_TYPE_HEVC_SPS && numNalus > 0) {
					sps_len = gb.BitRead(16);
					if (sps_len < 2) {
						return false;
					}
					break;
				}
				for (int i = 0; i < numNalus; i++) {
					int nalUnitLength = gb.BitRead(16);
					if (nalUnitLength < 2) {
						return false;
					}
					gb.SkipBytes(nalUnitLength);
				}
			}

			if (sps_len) {
				uint8_t marker_bit = gb.BitRead(1);            // market_bit
				if (marker_bit) {
					return false;
				}
				gb.BitRead(6);                                 // nal_unit_type
				gb.BitRead(6);                                 // nuh_layer_id
				uint8_t nuh_temporal_id_plus1 = gb.BitRead(3); // nuh_temporal_id_plus1
				if (nuh_temporal_id_plus1 == 0) {
					return false;
				}
				return ParseSequenceParameterSet(gb.GetBufferPos(), sps_len - 2, params);
			}

			return false;
		}

		return true;
	}

	bool ReconstructHEVCDecoderConfigurationRecord(const BYTE* raw_data, const size_t raw_size, const int nal_length_size,
												   const BYTE* record_data, const size_t record_size,
												   std::vector<BYTE>& new_record)
	{
		std::vector<uint8_t> vps;
		std::vector<uint8_t> sps;
		std::vector<uint8_t> pps;

		CH265Nalu Nalu;
		Nalu.SetBuffer(raw_data, raw_size, nal_length_size);
		while (!(!vps.empty() && !pps.empty() && !sps.empty())
			   && Nalu.ReadNext()) {
			const auto nalu_type = Nalu.GetType();
			switch (nalu_type) {
			case NALU_TYPE_HEVC_VPS:
				if (!vps.empty()) continue;
				vps.assign(Nalu.GetDataBuffer(), Nalu.GetDataBuffer() + Nalu.GetDataLength());
				break;
			case NALU_TYPE_HEVC_SPS:
				if (!sps.empty()) continue;
				sps.assign(Nalu.GetDataBuffer(), Nalu.GetDataBuffer() + Nalu.GetDataLength());
				break;
			case NALU_TYPE_HEVC_PPS:
				if (!pps.empty()) continue;
				pps.assign(Nalu.GetDataBuffer(), Nalu.GetDataBuffer() + Nalu.GetDataLength());
				break;
			}
		}

		if (!vps.empty() && !sps.empty() && !pps.empty()) {
			size_t new_record_size = record_size + 3 * 5 + vps.size() + sps.size() + pps.size();
			new_record.resize(new_record_size);
			memcpy(new_record.data(), record_data, record_size);
			new_record[22] = 3; // num_of_arrays
			size_t write_pos = 23;

			uint8_t array_completeness = 1;

			auto WriteNalUnits = [&](NALU_TYPE nalu_type, std::vector<uint8_t>& nal_unit) {
				/*
				 * bit(1) array_completeness;
				 * unsigned int(1) reserved = 0;
				 * unsigned int(6) nal_unit_type;
				 */
				new_record[write_pos] = array_completeness << 7 | nalu_type & 0x3f;
				write_pos++;

				new_record[write_pos + 1] = 1; // num_nalus
				write_pos += 2;

				const auto nal_unit_size = _byteswap_ushort(static_cast<unsigned short>(nal_unit.size()));
				memcpy(&new_record[write_pos], &nal_unit_size, 2); // nal_unit_length
				write_pos += 2;

				memcpy(&new_record[write_pos], nal_unit.data(), nal_unit.size());
				write_pos += nal_unit.size();
			};

			WriteNalUnits(NALU_TYPE_HEVC_VPS, vps);
			WriteNalUnits(NALU_TYPE_HEVC_SPS, sps);
			WriteNalUnits(NALU_TYPE_HEVC_PPS, pps);

			return true;
		}

		return false;
	}
} // namespace HEVCParser


namespace AV1Parser {
	constexpr auto PROFILE_AV1_MAIN         = 0;
	constexpr auto PROFILE_AV1_HIGH         = 1;
	constexpr auto PROFILE_AV1_PROFESSIONAL = 2;

	constexpr auto AVCOL_PRI_BT709          = 1;
	constexpr auto AVCOL_PRI_UNSPECIFIED    = 2;

	constexpr auto AVCOL_TRC_UNSPECIFIED    = 2;
	constexpr auto AVCOL_TRC_IEC61966_2_1   = 13;

	constexpr auto AVCOL_SPC_RGB            = 0;
	constexpr auto AVCOL_SPC_UNSPECIFIED    = 2;

	static bool ParseSequenceHeader(AV1SequenceParameters& seq_params, const BYTE* buf, const int buf_size) {
		auto uvlc = [](CGolombBuffer& gb) {
			auto leading_zeros = 0u;

			while (!gb.BitRead(1)) {
				++leading_zeros;
			}

			if (leading_zeros >= 32) {
				return (1llu << 32) - 1;
			}

			auto value = gb.BitRead(leading_zeros);
			return value + (1llu << leading_zeros) - 1;
		};

		auto ParseColorConfig = [](CGolombBuffer& gb, AV1SequenceParameters& seq_params) {
			uint8_t twelve_bit = 0;
			uint8_t high_bitdepth = gb.BitRead(1);
			if (seq_params.profile == PROFILE_AV1_PROFESSIONAL && high_bitdepth) {
				twelve_bit = gb.BitRead(1);
			}

			seq_params.bitdepth = 8 + (high_bitdepth * 2) + (twelve_bit * 2);

			if (seq_params.profile != PROFILE_AV1_HIGH) {
				seq_params.monochrome = gb.BitRead(1);
			}

			seq_params.color_description_present_flag = gb.BitRead(1);
			if (seq_params.color_description_present_flag) {
				seq_params.color_primaries = gb.BitRead(8);
				seq_params.transfer_characteristics = gb.BitRead(8);
				seq_params.matrix_coefficients = gb.BitRead(8);
			} else {
				seq_params.color_primaries = AVCOL_PRI_UNSPECIFIED;
				seq_params.transfer_characteristics = AVCOL_TRC_UNSPECIFIED;
				seq_params.matrix_coefficients = AVCOL_SPC_UNSPECIFIED;
			}

			if (seq_params.monochrome) {
				seq_params.color_range = gb.BitRead(1);
				seq_params.chroma_subsampling_x = 1;
				seq_params.chroma_subsampling_y = 1;
				seq_params.chroma_sample_position = 0;
				return;
			} else if (seq_params.color_primaries == AVCOL_PRI_BT709 &&
				seq_params.transfer_characteristics == AVCOL_TRC_IEC61966_2_1 &&
				seq_params.matrix_coefficients == AVCOL_SPC_RGB) {
			} else {
				seq_params.color_range = gb.BitRead(1);

				if (seq_params.profile == PROFILE_AV1_MAIN) {
					seq_params.chroma_subsampling_x = 1;
					seq_params.chroma_subsampling_y = 1;
				} else if (seq_params.profile == PROFILE_AV1_HIGH) {
					seq_params.chroma_subsampling_x = 0;
					seq_params.chroma_subsampling_y = 0;
				} else {
					if (twelve_bit) {
						seq_params.chroma_subsampling_x = gb.BitRead(1);
						if (seq_params.chroma_subsampling_x) {
							seq_params.chroma_subsampling_y = gb.BitRead(1);
						} else {
							seq_params.chroma_subsampling_y = 0;
						}
					} else {
						seq_params.chroma_subsampling_x = 1;
						seq_params.chroma_subsampling_y = 0;
					}
				}
				if (seq_params.chroma_subsampling_x && seq_params.chroma_subsampling_y) {
					seq_params.chroma_sample_position = gb.BitRead(2);
				}
			}

			gb.BitRead(1); // separate_uv_delta_q
		};

		CGolombBuffer gb(buf, buf_size);

		seq_params.profile = gb.BitRead(3);
		gb.BitRead(1); // still_picture
		bool reduced_still_picture_header = gb.BitRead(1);

		if (reduced_still_picture_header) {
			seq_params.level = gb.BitRead(5);
		} else {
			uint8_t buffer_delay_length_minus_1;
			bool decoder_model_info_present_flag = false;

			if (gb.BitRead(1)) { // timing_info_present_flag
				gb.BitRead(32); // num_units_in_display_tick
				gb.BitRead(32); // time_scale

				if (gb.BitRead(1)) { // equal_picture_interval
					uvlc(gb); // num_ticks_per_picture_minus_1
				}

				decoder_model_info_present_flag = gb.BitRead(1);
				if (decoder_model_info_present_flag) {
					buffer_delay_length_minus_1 = gb.BitRead(5);
					gb.BitRead(32); // num_units_in_decoding_tick
					gb.BitRead(10); // buffer_removal_time_length_minus_1 (5)
									// frame_presentation_time_length_minus_1 (5)
				}
			}

			bool initial_display_delay_present_flag = gb.BitRead(1);

			uint8_t operating_points_cnt_minus_1 = gb.BitRead(5);
			for (uint8_t i = 0; i <= operating_points_cnt_minus_1; i++) {
				gb.BitRead(12); // operating_point_idc
				uint8_t seq_level_idx = gb.BitRead(5);
				uint8_t seq_tier = seq_level_idx > 7 ? gb.BitRead(1) : 0;

				if (decoder_model_info_present_flag) {
					if (gb.BitRead(1)) { // decoder_model_present_for_this_op
						gb.BitRead(buffer_delay_length_minus_1 + 1); // decoder_buffer_delay
						gb.BitRead(buffer_delay_length_minus_1 + 1); // encoder_buffer_delay
						gb.BitRead(1); // low_delay_mode_flag
					}
				}

				if (initial_display_delay_present_flag) {
					if (gb.BitRead(1)) // initial_display_delay_present_for_this_op
						gb.BitRead(4); // initial_display_delay_minus_1
				}

				if (i == 0) {
					seq_params.level = seq_level_idx;
					seq_params.tier = seq_tier;
				}
			}
		}

		uint8_t frame_width_bits_minus_1 = gb.BitRead(4);
		uint8_t frame_height_bits_minus_1 = gb.BitRead(4);

		seq_params.width = gb.BitRead(frame_width_bits_minus_1 + 1) + 1;
		seq_params.height = gb.BitRead(frame_height_bits_minus_1 + 1) + 1;

		if (!seq_params.width || !seq_params.height) {
			return false;
		}

		if (!reduced_still_picture_header) {
			if (gb.BitRead(1)) { // frame_id_numbers_present_flag
				gb.BitRead(7); // delta_frame_id_length_minus_2 (4), additional_frame_id_length_minus_1 (3)
			}
		}

		gb.BitRead(3); // use_128x128_superblock (1), enable_filter_intra (1), enable_intra_edge_filter (1)

		if (!reduced_still_picture_header) {
			gb.BitRead(4); // enable_interintra_compound (1), enable_masked_compound (1)
						   // enable_warped_motion (1), enable_dual_filter (1)

			bool enable_order_hint = gb.BitRead(1);
			if (enable_order_hint) {
				gb.BitRead(2); // enable_jnt_comp (1), enable_ref_frame_mvs (1)
			}

			uint8_t seq_force_screen_content_tools = gb.BitRead(1) ? 2 : gb.BitRead(1);

			if (seq_force_screen_content_tools) {
				if (!gb.BitRead(1)) { // seq_choose_integer_mv
					gb.BitRead(1); // seq_force_integer_mv
				}
			}

			if (enable_order_hint) {
				gb.BitRead(3); // order_hint_bits_minus_1
			}
		}

		gb.BitRead(3); // enable_superres (1), enable_cdef (1), enable_restoration (1)

		ParseColorConfig(gb, seq_params);

		gb.BitRead(1); // film_grain_params_present

		auto remainingSize = gb.RemainingSize();
		return (remainingSize == 0 || remainingSize == 1);
	};

	static int64_t ParseOBUHeader(const BYTE* buf, const int buf_size, int64_t& obu_size, int& start_pos, uint8_t& obu_type)
	{
		auto leb128 = [](CGolombBuffer& gb) {
			int64_t ret = 0;

			for (int i = 0; i < 8; i++) {
				uint8_t byte = gb.BitRead(8);
				ret |= (int64_t)(byte & 0x7f) << (i * 7);
				if (!(byte & 0x80)) {
					break;
				}
			}
			return ret;
		};

		CGolombBuffer gb(buf, std::min(buf_size, MAX_OBU_HEADER_SIZE));
		if (gb.BitRead(1) != 0) { // obu_forbidden_bit
			return -1;
		}
		obu_type = gb.BitRead(4);
		bool extension_flag = gb.BitRead(1);
		bool has_size_flag = gb.BitRead(1);
		if (!has_size_flag) {
			return -1;
		}
		gb.BitRead(1); // obu_reserved_1bit

		if (extension_flag) {
			gb.BitRead(3); // temporal_id
			gb.BitRead(2); // spatial_id
			gb.BitRead(3); // extension_header_reserved_3bits
		}

		obu_size = leb128(gb);
		start_pos = gb.GetPos();

		return obu_size + start_pos;
	};

	int64_t ParseOBUHeaderSize(const BYTE* buf, const int buf_size, uint8_t& obu_type)
	{
		int64_t obu_size;
		int start_pos;
		return ParseOBUHeader(buf, buf_size, obu_size, start_pos, obu_type);
	}

	bool ParseOBU(const BYTE* data, int size, AV1SequenceParameters& seq_params, std::vector<uint8_t>& obu_sequence_header)
	{
		const BYTE* seq = nullptr;
		int seq_size = 0;
		bool b_frame_found = false;

		const BYTE* seq_obu = nullptr;
		int seq_obu_size = 0;

		auto buf = data;
		while (size > 0) {
			int64_t obu_size = 0;
			int start_pos = 0;
			uint8_t type = 0;
			auto len = ParseOBUHeader(buf, size, obu_size, start_pos, type);
			if (len < 0) {
				break;
			}

			if (type == AV1_OBU_FRAME_HEADER || type == AV1_OBU_FRAME) {
				b_frame_found = true;
			}

			if (len > size) {
				break;
			}

			if (type == AV1_OBU_SEQUENCE_HEADER) {
				seq = buf + start_pos;
				seq_size = obu_size;

				seq_obu = buf;
				seq_obu_size = len;
			}

			if (seq && b_frame_found) {
				break;
			}

			size -= len;
			buf += len;
		}

		if (seq && b_frame_found) {
			auto ret = ParseSequenceHeader(seq_params, seq, seq_size);
			if (ret) {
				obu_sequence_header.resize(seq_obu_size);
				obu_sequence_header.assign(seq_obu, seq_obu + seq_obu_size);
				return ret;
			}
		}

		return false;
	}
} // namespace AV1Parser

namespace AVS3Parser {
	static constexpr BYTE SEQ_START_CODE = 0xB0;
	static constexpr BYTE PROFILE_BASELINE_MAIN = 0x20;
	static constexpr BYTE PROFILE_BASELINE_MAIN10 = 0x22;

	static constexpr fraction_t frame_rates[16] = {
		{ 0    , 0   }, // forbid
		{ 24000, 1001},
		{ 24   , 1   },
		{ 25   , 1   },
		{ 30000, 1001},
		{ 30   , 1   },
		{ 50   , 1   },
		{ 60000, 1001},
		{ 60   , 1   },
		{ 100  , 1   },
		{ 120  , 1   },
		{ 200  , 1   },
		{ 240  , 1   },
		{ 300  , 1   },
		{ 0    , 0   }, // reserved
		{ 0    , 0   }  // reserved
	};

	bool ParseSequenceHeader(const BYTE* data, const int size, AVS3SequenceHeader& seq_header)
	{
		CGolombBuffer gb(data, size);

		BYTE id = {};
		if (!gb.NextMpegStartCode(id) || id != SEQ_START_CODE) {
			return false;
		}

		seq_header.profile = gb.BitRead(8);
		if (seq_header.profile != PROFILE_BASELINE_MAIN && seq_header.profile != PROFILE_BASELINE_MAIN10) {
			return false;
		}

		gb.BitRead(8); // level_id
		gb.BitRead(1); // progressive_sequence
		gb.BitRead(1); // field_coded_sequence

		BYTE library_stream_flag = gb.BitRead(1);
		if (!library_stream_flag) {
			BYTE library_picture_enable_flag = gb.BitRead(1);
			if (library_picture_enable_flag) {
				gb.BitRead(1); // duplicate_sequence_header_flag
			}
		}

		gb.BitRead(1); // marker_bit
		seq_header.width = gb.BitRead(14);
		gb.BitRead(1); // marker_bit
		seq_header.height = gb.BitRead(14);
		if (!seq_header.width || !seq_header.height) {
			return false;
		}

		gb.BitRead(2); // chroma_format
		gb.BitRead(3); // sample_precision

		if (seq_header.profile == PROFILE_BASELINE_MAIN10) {
			BYTE encoding_precision = gb.BitRead(3);
			if (encoding_precision == 1) {
				seq_header.bitdepth = 8;
			} else if (encoding_precision == 2) {
				seq_header.bitdepth = 10;
			} else {
				return false;
			}
		} else {
			seq_header.bitdepth = 8;
		}

		gb.BitRead(1); // marker_bit
		gb.BitRead(4); // aspect_ratio

		BYTE frame_rate_code = gb.BitRead(4);

		auto& frame_rate = frame_rates[frame_rate_code];
		if (!frame_rate.num || !frame_rate.den) {
			return false;
		}

		seq_header.AvgTimePerFrame = llMulDiv(UNITS, frame_rate.den, frame_rate.num, 0);

		return true;
	}
} // namespace AVS3Parser

namespace VVCParser {
	#define VVC_MAX_SLICES 1000
	#define VVC_MAX_SAMPLE_ARRAYS 3
	#define VVC_MAX_POINTS_IN_QP_TABLE 111
	#define VVC_MAX_REF_PIC_LISTS 64
	#define VVC_MAX_SUBLAYERS 7
	#define VVC_MAX_SUB_PROFILES 256
	#define VVC_MAX_DPB_SIZE 16
	#define VVC_MAX_REF_ENTRIES (VVC_MAX_DPB_SIZE + 13)
	#define VVC_MAX_CPB_CNT 32
	#define VVC_MAX_WIDTH 25332
	#define VVC_MAX_HEIGHT 25332

	struct H266RawNALUnitHeader {
		uint8_t nuh_layer_id;
		uint8_t nal_unit_type;
		uint8_t nuh_temporal_id_plus1;
		uint8_t nuh_reserved_zero_bit;
	};

	struct H266GeneralConstraintsInfo {
		uint8_t gci_present_flag;
		/* general */
		uint8_t gci_intra_only_constraint_flag;
		uint8_t gci_all_layers_independent_constraint_flag;
		uint8_t gci_one_au_only_constraint_flag;

		/* picture format */
		uint8_t gci_sixteen_minus_max_bitdepth_constraint_idc;
		uint8_t gci_three_minus_max_chroma_format_constraint_idc;

		/* NAL unit type related */
		uint8_t gci_no_mixed_nalu_types_in_pic_constraint_flag;
		uint8_t gci_no_trail_constraint_flag;
		uint8_t gci_no_stsa_constraint_flag;
		uint8_t gci_no_rasl_constraint_flag;
		uint8_t gci_no_radl_constraint_flag;
		uint8_t gci_no_idr_constraint_flag;
		uint8_t gci_no_cra_constraint_flag;
		uint8_t gci_no_gdr_constraint_flag;
		uint8_t gci_no_aps_constraint_flag;
		uint8_t gci_no_idr_rpl_constraint_flag;

		/* tile, slice, subpicture partitioning */
		uint8_t gci_one_tile_per_pic_constraint_flag;
		uint8_t gci_pic_header_in_slice_header_constraint_flag;
		uint8_t gci_one_slice_per_pic_constraint_flag;
		uint8_t gci_no_rectangular_slice_constraint_flag;
		uint8_t gci_one_slice_per_subpic_constraint_flag;
		uint8_t gci_no_subpic_info_constraint_flag;

		/* CTU and block partitioning */
		uint8_t gci_three_minus_max_log2_ctu_size_constraint_idc;
		uint8_t gci_no_partition_constraints_override_constraint_flag;
		uint8_t gci_no_mtt_constraint_flag;
		uint8_t gci_no_qtbtt_dual_tree_intra_constraint_flag;

		/* intra */
		uint8_t gci_no_palette_constraint_flag;
		uint8_t gci_no_ibc_constraint_flag;
		uint8_t gci_no_isp_constraint_flag;
		uint8_t gci_no_mrl_constraint_flag;
		uint8_t gci_no_mip_constraint_flag;
		uint8_t gci_no_cclm_constraint_flag;

		/* inter */
		uint8_t gci_no_ref_pic_resampling_constraint_flag;
		uint8_t gci_no_res_change_in_clvs_constraint_flag;
		uint8_t gci_no_weighted_prediction_constraint_flag;
		uint8_t gci_no_ref_wraparound_constraint_flag;
		uint8_t gci_no_temporal_mvp_constraint_flag;
		uint8_t gci_no_sbtmvp_constraint_flag;
		uint8_t gci_no_amvr_constraint_flag;
		uint8_t gci_no_bdof_constraint_flag;
		uint8_t gci_no_smvd_constraint_flag;
		uint8_t gci_no_dmvr_constraint_flag;
		uint8_t gci_no_mmvd_constraint_flag;
		uint8_t gci_no_affine_motion_constraint_flag;
		uint8_t gci_no_prof_constraint_flag;
		uint8_t gci_no_bcw_constraint_flag;
		uint8_t gci_no_ciip_constraint_flag;
		uint8_t gci_no_gpm_constraint_flag;

		/* transform, quantization, residual */
		uint8_t gci_no_luma_transform_size_64_constraint_flag;
		uint8_t gci_no_transform_skip_constraint_flag;
		uint8_t gci_no_bdpcm_constraint_flag;
		uint8_t gci_no_mts_constraint_flag;
		uint8_t gci_no_lfnst_constraint_flag;
		uint8_t gci_no_joint_cbcr_constraint_flag;
		uint8_t gci_no_sbt_constraint_flag;
		uint8_t gci_no_act_constraint_flag;
		uint8_t gci_no_explicit_scaling_list_constraint_flag;
		uint8_t gci_no_dep_quant_constraint_flag;
		uint8_t gci_no_sign_data_hiding_constraint_flag;
		uint8_t gci_no_cu_qp_delta_constraint_flag;
		uint8_t gci_no_chroma_qp_offset_constraint_flag;

		/* loop filter */
		uint8_t gci_no_sao_constraint_flag;
		uint8_t gci_no_alf_constraint_flag;
		uint8_t gci_no_ccalf_constraint_flag;
		uint8_t gci_no_lmcs_constraint_flag;
		uint8_t gci_no_ladf_constraint_flag;
		uint8_t gci_no_virtual_boundaries_constraint_flag;

		uint8_t gci_num_additional_bits;
		uint8_t gci_reserved_bit[255];

		uint8_t gci_all_rap_pictures_constraint_flag;
		uint8_t gci_no_extended_precision_processing_constraint_flag;
		uint8_t gci_no_ts_residual_coding_rice_constraint_flag;
		uint8_t gci_no_rrc_rice_extension_constraint_flag;
		uint8_t gci_no_persistent_rice_adaptation_constraint_flag;
		uint8_t gci_no_reverse_last_sig_coeff_constraint_flag;
	};

	struct H266RawProfileTierLevel {
		uint8_t  general_profile_idc;
		uint8_t  general_tier_flag;
		uint8_t  general_level_idc;
		uint8_t  ptl_frame_only_constraint_flag;
		uint8_t  ptl_multilayer_enabled_flag;
		H266GeneralConstraintsInfo general_constraints_info;
		uint8_t  ptl_sublayer_level_present_flag[VVC_MAX_SUBLAYERS - 1];
		uint8_t  sublayer_level_idc[VVC_MAX_SUBLAYERS - 1];
		uint8_t  ptl_num_sub_profiles;
		uint32_t general_sub_profile_idc[VVC_MAX_SUB_PROFILES];

		uint8_t  ptl_reserved_zero_bit;
	};

	struct H266DpbParameters {
		uint8_t dpb_max_dec_pic_buffering_minus1[VVC_MAX_SUBLAYERS];
		uint8_t dpb_max_num_reorder_pics[VVC_MAX_SUBLAYERS];
		uint8_t dpb_max_latency_increase_plus1[VVC_MAX_SUBLAYERS];
	};

	struct H266RefPicListStruct {
		uint8_t num_ref_entries;
		uint8_t ltrp_in_header_flag;
		uint8_t inter_layer_ref_pic_flag[VVC_MAX_REF_ENTRIES];
		uint8_t st_ref_pic_flag[VVC_MAX_REF_ENTRIES];
		uint8_t abs_delta_poc_st[VVC_MAX_REF_ENTRIES];
		uint8_t strp_entry_sign_flag[VVC_MAX_REF_ENTRIES];
		uint8_t rpls_poc_lsb_lt[VVC_MAX_REF_ENTRIES];
		uint8_t ilrp_idx[VVC_MAX_REF_ENTRIES];
	};

	struct H266RawGeneralTimingHrdParameters {
		uint32_t num_units_in_tick;
		uint32_t time_scale;
		uint8_t  general_nal_hrd_params_present_flag;
		uint8_t  general_vcl_hrd_params_present_flag;
		uint8_t  general_same_pic_timing_in_all_ols_flag;
		uint8_t  general_du_hrd_params_present_flag;
		uint8_t  tick_divisor_minus2;
		uint8_t  bit_rate_scale;
		uint8_t  cpb_size_scale;
		uint8_t  cpb_size_du_scale;
		uint8_t  hrd_cpb_cnt_minus1;
	};

	struct H266RawSubLayerHRDParameters {
		uint32_t bit_rate_value_minus1[VVC_MAX_SUBLAYERS][VVC_MAX_CPB_CNT];
		uint32_t cpb_size_value_minus1[VVC_MAX_SUBLAYERS][VVC_MAX_CPB_CNT];
		uint32_t cpb_size_du_value_minus1[VVC_MAX_SUBLAYERS][VVC_MAX_CPB_CNT];
		uint32_t bit_rate_du_value_minus1[VVC_MAX_SUBLAYERS][VVC_MAX_CPB_CNT];
		uint8_t  cbr_flag[VVC_MAX_SUBLAYERS][VVC_MAX_CPB_CNT];
	};

	struct H266RawOlsTimingHrdParameters {
		uint8_t  fixed_pic_rate_general_flag[VVC_MAX_SUBLAYERS];
		uint8_t  fixed_pic_rate_within_cvs_flag[VVC_MAX_SUBLAYERS];
		uint16_t elemental_duration_in_tc_minus1[VVC_MAX_SUBLAYERS];
		uint8_t  low_delay_hrd_flag[VVC_MAX_SUBLAYERS];
		H266RawSubLayerHRDParameters nal_sub_layer_hrd_parameters;
		H266RawSubLayerHRDParameters vcl_sub_layer_hrd_parameters;
	};

	struct H266RawVUI {
		uint8_t  vui_progressive_source_flag;
		uint8_t  vui_interlaced_source_flag;
		uint8_t  vui_non_packed_constraint_flag;
		uint8_t  vui_non_projected_constraint_flag;

		uint8_t  vui_aspect_ratio_info_present_flag;
		uint8_t  vui_aspect_ratio_constant_flag;
		uint8_t  vui_aspect_ratio_idc;

		uint16_t vui_sar_width;
		uint16_t vui_sar_height;

		uint8_t  vui_overscan_info_present_flag;
		uint8_t  vui_overscan_appropriate_flag;

		uint8_t  vui_colour_description_present_flag;
		uint8_t  vui_colour_primaries;

		uint8_t  vui_transfer_characteristics;
		uint8_t  vui_matrix_coeffs;
		uint8_t  vui_full_range_flag;

		uint8_t  vui_chroma_loc_info_present_flag;
		uint8_t  vui_chroma_sample_loc_type_frame;
		uint8_t  vui_chroma_sample_loc_type_top_field;
		uint8_t  vui_chroma_sample_loc_type_bottom_field;
	};

	struct H266RawSPS {
		H266RawNALUnitHeader nal_unit_header;

		uint8_t  sps_seq_parameter_set_id;
		uint8_t  sps_video_parameter_set_id;
		uint8_t  sps_max_sublayers_minus1;
		uint8_t  sps_chroma_format_idc;
		uint8_t  sps_log2_ctu_size_minus5;
		uint8_t  sps_ptl_dpb_hrd_params_present_flag;
		H266RawProfileTierLevel profile_tier_level;
		uint8_t  sps_gdr_enabled_flag;
		uint8_t  sps_ref_pic_resampling_enabled_flag;
		uint8_t  sps_res_change_in_clvs_allowed_flag;

		uint16_t sps_pic_width_max_in_luma_samples;
		uint16_t sps_pic_height_max_in_luma_samples;

		uint8_t  sps_conformance_window_flag;
		uint16_t sps_conf_win_left_offset;
		uint16_t sps_conf_win_right_offset;
		uint16_t sps_conf_win_top_offset;
		uint16_t sps_conf_win_bottom_offset;

		uint8_t  sps_subpic_info_present_flag;
		uint16_t sps_num_subpics_minus1;
		uint8_t  sps_independent_subpics_flag;
		uint8_t  sps_subpic_same_size_flag;
		uint16_t sps_subpic_ctu_top_left_x[VVC_MAX_SLICES];
		uint16_t sps_subpic_ctu_top_left_y[VVC_MAX_SLICES];
		uint16_t sps_subpic_width_minus1[VVC_MAX_SLICES];
		uint16_t sps_subpic_height_minus1[VVC_MAX_SLICES];
		uint8_t  sps_subpic_treated_as_pic_flag[VVC_MAX_SLICES];
		uint8_t  sps_loop_filter_across_subpic_enabled_flag[VVC_MAX_SLICES];
		uint8_t  sps_subpic_id_len_minus1;
		uint8_t  sps_subpic_id_mapping_explicitly_signalled_flag;
		uint8_t  sps_subpic_id_mapping_present_flag;
		uint32_t sps_subpic_id[VVC_MAX_SLICES];


		uint8_t  sps_bitdepth_minus8;
		uint8_t  sps_entropy_coding_sync_enabled_flag;
		uint8_t  sps_entry_point_offsets_present_flag;

		uint8_t  sps_log2_max_pic_order_cnt_lsb_minus4;
		uint8_t  sps_poc_msb_cycle_flag;
		uint8_t  sps_poc_msb_cycle_len_minus1;

		uint8_t  sps_num_extra_ph_bytes;
		uint8_t  sps_extra_ph_bit_present_flag[16];

		uint8_t  sps_num_extra_sh_bytes;
		uint8_t  sps_extra_sh_bit_present_flag[16];

		uint8_t  sps_sublayer_dpb_params_flag;
		H266DpbParameters sps_dpb_params;

		uint8_t  sps_log2_min_luma_coding_block_size_minus2;
		uint8_t  sps_partition_constraints_override_enabled_flag;
		uint8_t  sps_log2_diff_min_qt_min_cb_intra_slice_luma;
		uint8_t  sps_max_mtt_hierarchy_depth_intra_slice_luma;
		uint8_t  sps_log2_diff_max_bt_min_qt_intra_slice_luma;
		uint8_t  sps_log2_diff_max_tt_min_qt_intra_slice_luma;

		uint8_t  sps_qtbtt_dual_tree_intra_flag;
		uint8_t  sps_log2_diff_min_qt_min_cb_intra_slice_chroma;
		uint8_t  sps_max_mtt_hierarchy_depth_intra_slice_chroma;
		uint8_t  sps_log2_diff_max_bt_min_qt_intra_slice_chroma;
		uint8_t  sps_log2_diff_max_tt_min_qt_intra_slice_chroma;

		uint8_t  sps_log2_diff_min_qt_min_cb_inter_slice;
		uint8_t  sps_max_mtt_hierarchy_depth_inter_slice;
		uint8_t  sps_log2_diff_max_bt_min_qt_inter_slice;
		uint8_t  sps_log2_diff_max_tt_min_qt_inter_slice;

		uint8_t  sps_max_luma_transform_size_64_flag;

		uint8_t  sps_transform_skip_enabled_flag;
		uint8_t  sps_log2_transform_skip_max_size_minus2;
		uint8_t  sps_bdpcm_enabled_flag;

		uint8_t  sps_mts_enabled_flag;
		uint8_t  sps_explicit_mts_intra_enabled_flag;
		uint8_t  sps_explicit_mts_inter_enabled_flag;

		uint8_t  sps_lfnst_enabled_flag;

		uint8_t  sps_joint_cbcr_enabled_flag;
		uint8_t  sps_same_qp_table_for_chroma_flag;

		int8_t   sps_qp_table_start_minus26[VVC_MAX_SAMPLE_ARRAYS];
		uint8_t  sps_num_points_in_qp_table_minus1[VVC_MAX_SAMPLE_ARRAYS];
		uint8_t  sps_delta_qp_in_val_minus1[VVC_MAX_SAMPLE_ARRAYS][VVC_MAX_POINTS_IN_QP_TABLE];
		uint8_t  sps_delta_qp_diff_val[VVC_MAX_SAMPLE_ARRAYS][VVC_MAX_POINTS_IN_QP_TABLE];

		uint8_t  sps_sao_enabled_flag;
		uint8_t  sps_alf_enabled_flag;
		uint8_t  sps_ccalf_enabled_flag;
		uint8_t  sps_lmcs_enabled_flag;
		uint8_t  sps_weighted_pred_flag;
		uint8_t  sps_weighted_bipred_flag;
		uint8_t  sps_long_term_ref_pics_flag;
		uint8_t  sps_inter_layer_prediction_enabled_flag;
		uint8_t  sps_idr_rpl_present_flag;
		uint8_t  sps_rpl1_same_as_rpl0_flag;

		uint8_t  sps_num_ref_pic_lists[2];
		H266RefPicListStruct sps_ref_pic_list_struct[2][VVC_MAX_REF_PIC_LISTS];

		uint8_t  sps_ref_wraparound_enabled_flag;
		uint8_t  sps_temporal_mvp_enabled_flag;
		uint8_t  sps_sbtmvp_enabled_flag;
		uint8_t  sps_amvr_enabled_flag;
		uint8_t  sps_bdof_enabled_flag;
		uint8_t  sps_bdof_control_present_in_ph_flag;
		uint8_t  sps_smvd_enabled_flag;
		uint8_t  sps_dmvr_enabled_flag;
		uint8_t  sps_dmvr_control_present_in_ph_flag;
		uint8_t  sps_mmvd_enabled_flag;
		uint8_t  sps_mmvd_fullpel_only_enabled_flag;
		uint8_t  sps_six_minus_max_num_merge_cand;
		uint8_t  sps_sbt_enabled_flag;
		uint8_t  sps_affine_enabled_flag;
		uint8_t  sps_five_minus_max_num_subblock_merge_cand;
		uint8_t  sps_6param_affine_enabled_flag;
		uint8_t  sps_affine_amvr_enabled_flag;
		uint8_t  sps_affine_prof_enabled_flag;
		uint8_t  sps_prof_control_present_in_ph_flag;
		uint8_t  sps_bcw_enabled_flag;
		uint8_t  sps_ciip_enabled_flag;
		uint8_t  sps_gpm_enabled_flag;
		uint8_t  sps_max_num_merge_cand_minus_max_num_gpm_cand;
		uint8_t  sps_log2_parallel_merge_level_minus2;
		uint8_t  sps_isp_enabled_flag;
		uint8_t  sps_mrl_enabled_flag;
		uint8_t  sps_mip_enabled_flag;
		uint8_t  sps_cclm_enabled_flag;
		uint8_t  sps_chroma_horizontal_collocated_flag;
		uint8_t  sps_chroma_vertical_collocated_flag;
		uint8_t  sps_palette_enabled_flag;
		uint8_t  sps_act_enabled_flag;
		uint8_t  sps_min_qp_prime_ts;
		uint8_t  sps_ibc_enabled_flag;
		uint8_t  sps_six_minus_max_num_ibc_merge_cand;
		uint8_t  sps_ladf_enabled_flag;
		uint8_t  sps_num_ladf_intervals_minus2;
		int8_t   sps_ladf_lowest_interval_qp_offset;
		int8_t   sps_ladf_qp_offset[4];
		uint16_t sps_ladf_delta_threshold_minus1[4];

		uint8_t  sps_explicit_scaling_list_enabled_flag;
		uint8_t  sps_scaling_matrix_for_lfnst_disabled_flag;
		uint8_t  sps_scaling_matrix_for_alternative_colour_space_disabled_flag;
		uint8_t  sps_scaling_matrix_designated_colour_space_flag;
		uint8_t  sps_dep_quant_enabled_flag;
		uint8_t  sps_sign_data_hiding_enabled_flag;

		uint8_t  sps_virtual_boundaries_enabled_flag;
		uint8_t  sps_virtual_boundaries_present_flag;
		uint8_t  sps_num_ver_virtual_boundaries;
		uint16_t sps_virtual_boundary_pos_x_minus1[3];
		uint8_t  sps_num_hor_virtual_boundaries;
		uint16_t sps_virtual_boundary_pos_y_minus1[3];

		uint8_t  sps_timing_hrd_params_present_flag;
		uint8_t  sps_sublayer_cpb_params_present_flag;
		H266RawGeneralTimingHrdParameters sps_general_timing_hrd_parameters;
		H266RawOlsTimingHrdParameters sps_ols_timing_hrd_parameters;

		uint8_t  sps_field_seq_flag;
		uint8_t  sps_vui_parameters_present_flag;
		uint16_t sps_vui_payload_size_minus1;
		H266RawVUI vui;

		uint8_t  sps_extension_flag;

		uint8_t  sps_range_extension_flag;
		uint8_t  sps_extension_7bits;

		uint8_t  sps_extended_precision_flag;
		uint8_t  sps_ts_residual_coding_rice_present_in_sh_flag;
		uint8_t  sps_rrc_rice_extension_flag;
		uint8_t  sps_persistent_rice_adaptation_enabled_flag;
		uint8_t  sps_reverse_last_sig_coeff_enabled_flag;
	};

	struct H266RawPPS {
		H266RawNALUnitHeader nal_unit_header;

		uint8_t  pps_pic_parameter_set_id;
		uint8_t  pps_seq_parameter_set_id;
		uint8_t  pps_mixed_nalu_types_in_pic_flag;
		uint16_t pps_pic_width_in_luma_samples;
		uint16_t pps_pic_height_in_luma_samples;

		uint8_t  pps_conformance_window_flag;
		uint16_t pps_conf_win_left_offset;
		uint16_t pps_conf_win_right_offset;
		uint16_t pps_conf_win_top_offset;
		uint16_t pps_conf_win_bottom_offset;
	};

#define bit_position(gb) (gb.GetBitsPos())
#define byte_alignment(gb) (gb.GetBitsPos() % 8)
#define CHECK(call) \
	{ \
		auto ret = (call); \
		if (!ret) return false; \
	}
#define ub(width, name) \
	{ \
		current->name = gb.BitRead(width); \
	}
#define u(width, name, range_min, range_max) \
	{ \
		current->name = gb.BitRead(width); \
		if (current->name < (range_min) || current->name > (range_max)) return false; \
	}
#define ue(name, range_min, range_max) \
	{ \
		current->name = gb.UExpGolombRead(); \
		if (current->name < (range_min) || current->name > (range_max)) return false; \
	}
#define ses(name, range_min, range_max, ...) \
	{ \
		current->name = gb.SExpGolombRead(); \
		if (current->name < (range_min) || current->name > (range_max)) return false; \
	}
#define se(name, range_min, range_max) \
	{ \
		current->name = gb.SExpGolombRead(); \
		if (current->name < (range_min) || current->name > (range_max)) return false; \
	}
#define ues(name, range_min, range_max, ...) \
		ue(name, range_min, range_max)
#define ubs(width, name, range_min, range_max) \
		u(width, name, range_min, range_max)
#define flag(name) \
	{ \
		current->name = gb.BitRead(1); \
	}
#define flags(name, ...) \
        u(1, name, 0, 1)
#define fixed(width, name, value) \
	{ \
		uint32_t name = gb.BitRead(width); \
		if (name > (value)) return false; \
	}
#define infer(name, value) \
	{ \
		current->name = value; \
	}

#define AV_CEIL_RSHIFT(a,b) (-((-(a)) >> (b)))
#define MAX_UINT_BITS(length) ((UINT64_C(1) << (length)) - 1)

	static const int av_ceil_log2(int x)
	{
		return log2((x - 1U) << 1);
	}

	static bool general_constraints_info(CGolombBuffer& gb, H266GeneralConstraintsInfo* current)
	{
		flag(gci_present_flag);
		if (current->gci_present_flag) {
			int num_additional_bits_used = 0;

			/* general */
			flag(gci_intra_only_constraint_flag);
			flag(gci_all_layers_independent_constraint_flag);
			flag(gci_one_au_only_constraint_flag);

			/* picture format */
			u(4, gci_sixteen_minus_max_bitdepth_constraint_idc, 0, 8);
			ub(2, gci_three_minus_max_chroma_format_constraint_idc);

			/* NAL unit type related */
			flag(gci_no_mixed_nalu_types_in_pic_constraint_flag);
			flag(gci_no_trail_constraint_flag);
			flag(gci_no_stsa_constraint_flag);
			flag(gci_no_rasl_constraint_flag);
			flag(gci_no_radl_constraint_flag);
			flag(gci_no_idr_constraint_flag);
			flag(gci_no_cra_constraint_flag);
			flag(gci_no_gdr_constraint_flag);
			flag(gci_no_aps_constraint_flag);
			flag(gci_no_idr_rpl_constraint_flag);

			/* tile, slice, subpicture partitioning */
			flag(gci_one_tile_per_pic_constraint_flag);
			flag(gci_pic_header_in_slice_header_constraint_flag);
			flag(gci_one_slice_per_pic_constraint_flag);
			flag(gci_no_rectangular_slice_constraint_flag);
			flag(gci_one_slice_per_subpic_constraint_flag);
			flag(gci_no_subpic_info_constraint_flag);

			/* CTU and block partitioning */
			ub(2, gci_three_minus_max_log2_ctu_size_constraint_idc);
			flag(gci_no_partition_constraints_override_constraint_flag);
			flag(gci_no_mtt_constraint_flag);
			flag(gci_no_qtbtt_dual_tree_intra_constraint_flag);

			/* intra */
			flag(gci_no_palette_constraint_flag);
			flag(gci_no_ibc_constraint_flag);
			flag(gci_no_isp_constraint_flag);
			flag(gci_no_mrl_constraint_flag);
			flag(gci_no_mip_constraint_flag);
			flag(gci_no_cclm_constraint_flag);

			/* inter */
			flag(gci_no_ref_pic_resampling_constraint_flag);
			flag(gci_no_res_change_in_clvs_constraint_flag);
			flag(gci_no_weighted_prediction_constraint_flag);
			flag(gci_no_ref_wraparound_constraint_flag);
			flag(gci_no_temporal_mvp_constraint_flag);
			flag(gci_no_sbtmvp_constraint_flag);
			flag(gci_no_amvr_constraint_flag);
			flag(gci_no_bdof_constraint_flag);
			flag(gci_no_smvd_constraint_flag);
			flag(gci_no_dmvr_constraint_flag);
			flag(gci_no_mmvd_constraint_flag);
			flag(gci_no_affine_motion_constraint_flag);
			flag(gci_no_prof_constraint_flag);
			flag(gci_no_bcw_constraint_flag);
			flag(gci_no_ciip_constraint_flag);
			flag(gci_no_gpm_constraint_flag);

			/* transform, quantization, residual */
			flag(gci_no_luma_transform_size_64_constraint_flag);
			flag(gci_no_transform_skip_constraint_flag);
			flag(gci_no_bdpcm_constraint_flag);
			flag(gci_no_mts_constraint_flag);
			flag(gci_no_lfnst_constraint_flag);
			flag(gci_no_joint_cbcr_constraint_flag);
			flag(gci_no_sbt_constraint_flag);
			flag(gci_no_act_constraint_flag);
			flag(gci_no_explicit_scaling_list_constraint_flag);
			flag(gci_no_dep_quant_constraint_flag);
			flag(gci_no_sign_data_hiding_constraint_flag);
			flag(gci_no_cu_qp_delta_constraint_flag);
			flag(gci_no_chroma_qp_offset_constraint_flag);

			/* loop filter */
			flag(gci_no_sao_constraint_flag);
			flag(gci_no_alf_constraint_flag);
			flag(gci_no_ccalf_constraint_flag);
			flag(gci_no_lmcs_constraint_flag);
			flag(gci_no_ladf_constraint_flag);
			flag(gci_no_virtual_boundaries_constraint_flag);
			ub(8, gci_num_additional_bits);
			if (current->gci_num_additional_bits > 5) {
				flag(gci_all_rap_pictures_constraint_flag);
				flag(gci_no_extended_precision_processing_constraint_flag);
				flag(gci_no_ts_residual_coding_rice_constraint_flag);
				flag(gci_no_rrc_rice_extension_constraint_flag);
				flag(gci_no_persistent_rice_adaptation_constraint_flag);
				flag(gci_no_reverse_last_sig_coeff_constraint_flag);
				num_additional_bits_used = 6;
			}

			for (int i = 0; i < current->gci_num_additional_bits - num_additional_bits_used; i++)
				flags(gci_reserved_bit[i], 1, i);
		}
		while (byte_alignment(gb) != 0)
			fixed(1, gci_alignment_zero_bit, 0);

		return true;
	}

	static bool profile_tier_level(CGolombBuffer& gb, H266RawProfileTierLevel* current,
								   int profile_tier_present_flag,
								   int max_num_sub_layers_minus1)
	{
		if (profile_tier_present_flag) {
			ub(7, general_profile_idc);
			flag(general_tier_flag);
		}
		ub(8, general_level_idc);
		flag(ptl_frame_only_constraint_flag);
		flag(ptl_multilayer_enabled_flag);
		if (profile_tier_present_flag) {
			CHECK(general_constraints_info(gb,&current->general_constraints_info));
		}
		for (int i = max_num_sub_layers_minus1 - 1; i >= 0; i--)
			flags(ptl_sublayer_level_present_flag[i], 1, i);
		while (byte_alignment(gb) != 0)
			flag(ptl_reserved_zero_bit);
		for (int i = max_num_sub_layers_minus1 - 1; i >= 0; i--)
			if (current->ptl_sublayer_level_present_flag[i])
				ubs(8, sublayer_level_idc[i], 1, i);
		if (profile_tier_present_flag) {
			ub(8, ptl_num_sub_profiles);
			for (int i = 0; i < current->ptl_num_sub_profiles; i++)
				ubs(32, general_sub_profile_idc[i], 1, static_cast<uint32_t>(i));
		}

		return true;
	}

	static bool dpb_parameters(CGolombBuffer& gb, H266DpbParameters* current,
							   uint8_t max_sublayers_minus1,
							   uint8_t sublayer_info_flag)
	{
		for (int i = (sublayer_info_flag ? 0 : max_sublayers_minus1);
			i <= max_sublayers_minus1; i++) {
			ues(dpb_max_dec_pic_buffering_minus1[i], 0, VVC_MAX_DPB_SIZE - 1, 1, i);
			ues(dpb_max_num_reorder_pics[i],
				0, current->dpb_max_dec_pic_buffering_minus1[i], 1, i);
			ues(dpb_max_latency_increase_plus1[i], 0, UINT32_MAX - 1, 1, i);
		}

		return true;
	}

	static bool ref_pic_list_struct(CGolombBuffer& gb,
									H266RefPicListStruct* current,
									uint8_t list_idx, uint8_t rpls_idx,
									const H266RawSPS* sps)
	{
		ue(num_ref_entries, 0, VVC_MAX_REF_ENTRIES);
		if (sps->sps_long_term_ref_pics_flag &&
			rpls_idx < sps->sps_num_ref_pic_lists[list_idx] &&
			current->num_ref_entries > 0)
			flag(ltrp_in_header_flag);
		if (sps->sps_long_term_ref_pics_flag &&
			rpls_idx == sps->sps_num_ref_pic_lists[list_idx])
			infer(ltrp_in_header_flag, 1);
		for (int i = 0, j = 0; i < current->num_ref_entries; i++) {
			if (sps->sps_inter_layer_prediction_enabled_flag) {
				flags(inter_layer_ref_pic_flag[i], 1, i);
			}
			else
				infer(inter_layer_ref_pic_flag[i], 0);

			if (!current->inter_layer_ref_pic_flag[i]) {
				if (sps->sps_long_term_ref_pics_flag) {
					flags(st_ref_pic_flag[i], 1, i);
				}
				else
					infer(st_ref_pic_flag[i], 1);
				if (current->st_ref_pic_flag[i]) {
					int abs_delta_poc_st;
					ues(abs_delta_poc_st[i], 0, MAX_UINT_BITS(15), 1, i);
					if ((sps->sps_weighted_pred_flag ||
						 sps->sps_weighted_bipred_flag) && i != 0)
						abs_delta_poc_st = current->abs_delta_poc_st[i];
					else
						abs_delta_poc_st = current->abs_delta_poc_st[i] + 1;
					if (abs_delta_poc_st > 0)
						flags(strp_entry_sign_flag[i], 1, i);
				} else {
					if (!current->ltrp_in_header_flag) {
						uint8_t bits = sps->sps_log2_max_pic_order_cnt_lsb_minus4 + 4;
						ubs(bits, rpls_poc_lsb_lt[j], 1, j);
						j++;
					}
				}
			} else {
				ues(ilrp_idx[i], 0, 255, 1, i);
			}
		}

		return true;
	}

	static bool general_timing_hrd_parameters(CGolombBuffer& gb, H266RawGeneralTimingHrdParameters* current)
	{
		ub(32, num_units_in_tick);
		u(32, time_scale, 1, MAX_UINT_BITS(32));
		flag(general_nal_hrd_params_present_flag);
		flag(general_vcl_hrd_params_present_flag);

		if (current->general_nal_hrd_params_present_flag ||
			current->general_vcl_hrd_params_present_flag) {
			flag(general_same_pic_timing_in_all_ols_flag);
			flag(general_du_hrd_params_present_flag);
			if (current->general_du_hrd_params_present_flag)
				ub(8, tick_divisor_minus2);
			ub(4, bit_rate_scale);
			ub(4, cpb_size_scale);
			if (current->general_du_hrd_params_present_flag)
				ub(4, cpb_size_du_scale);
			ue(hrd_cpb_cnt_minus1, 0, 31);
		} else {
			//infer general_same_pic_timing_in_all_ols_flag?
			infer(general_du_hrd_params_present_flag, 0);
		}

		return true;
	}

	static bool sublayer_hrd_parameters(CGolombBuffer& gb, H266RawSubLayerHRDParameters* current,
										int sublayer_id,
										const H266RawGeneralTimingHrdParameters* general)
	{
		for (int i = 0; i <= general->hrd_cpb_cnt_minus1; i++) {
			ues(bit_rate_value_minus1[sublayer_id][i], 0, UINT32_MAX - 1, 2,
				sublayer_id, i);
			ues(cpb_size_value_minus1[sublayer_id][i], 0, UINT32_MAX - 1, 2,
				sublayer_id, i);
			if (general->general_du_hrd_params_present_flag) {
				ues(cpb_size_du_value_minus1[sublayer_id][i],
					0, UINT32_MAX - 1, 2, sublayer_id, i);
				ues(bit_rate_du_value_minus1[sublayer_id][i],
					0, UINT32_MAX - 1, 2, sublayer_id, i);
			}
			flags(cbr_flag[sublayer_id][i], 2, sublayer_id, i);
		}

		return true;
	}


	static bool ols_timing_hrd_parameters(CGolombBuffer& gb, H266RawOlsTimingHrdParameters* current,
										  uint8_t first_sublayer, uint8_t max_sublayers_minus1,
										  const H266RawGeneralTimingHrdParameters* general)
	{
		for (int i = first_sublayer; i <= max_sublayers_minus1; i++) {
			flags(fixed_pic_rate_general_flag[i], 1, i);
			if (!current->fixed_pic_rate_general_flag[i]) {
				flags(fixed_pic_rate_within_cvs_flag[i], 1, i);
			}
			else
				infer(fixed_pic_rate_within_cvs_flag[i], 1);
			if (current->fixed_pic_rate_within_cvs_flag[i]) {
				ues(elemental_duration_in_tc_minus1[i], 0, 2047, 1, i);
				infer(low_delay_hrd_flag[i], 0);
			} else if ((general->general_nal_hrd_params_present_flag ||
						general->general_vcl_hrd_params_present_flag) &&
					   general->hrd_cpb_cnt_minus1 == 0) {
				flags(low_delay_hrd_flag[i], 1, i);
			} else {
				infer(low_delay_hrd_flag[i], 0);
			}
			if (general->general_nal_hrd_params_present_flag)
				CHECK(sublayer_hrd_parameters(gb,
											  &current->nal_sub_layer_hrd_parameters,
											  i, general));
			if (general->general_vcl_hrd_params_present_flag)
				CHECK(sublayer_hrd_parameters(gb,
											  &current->nal_sub_layer_hrd_parameters,
											  i, general));
		}

		return true;
	}

	static bool vui_parameters_default(H266RawVUI* current)
	{
		//defined in D.8
		infer(vui_progressive_source_flag, 0);
		infer(vui_interlaced_source_flag, 0);

		infer(vui_non_packed_constraint_flag, 0);
		infer(vui_non_projected_constraint_flag, 0);

		infer(vui_aspect_ratio_constant_flag, 0);
		infer(vui_aspect_ratio_idc, 0);

		infer(vui_overscan_info_present_flag, 0);

		infer(vui_colour_primaries, 2);
		infer(vui_transfer_characteristics, 2);
		infer(vui_matrix_coeffs, 2);
		infer(vui_full_range_flag, 0);

		infer(vui_chroma_sample_loc_type_frame, 6);
		infer(vui_chroma_sample_loc_type_top_field, 6);
		infer(vui_chroma_sample_loc_type_bottom_field, 6);

		return true;
	}

	static bool vui_parameters(CGolombBuffer& gb,
							   H266RawVUI* current,
							   uint8_t chroma_format_idc)
	{
		flag(vui_progressive_source_flag);
		flag(vui_interlaced_source_flag);
		flag(vui_non_packed_constraint_flag);
		flag(vui_non_projected_constraint_flag);
		flag(vui_aspect_ratio_info_present_flag);
		if (current->vui_aspect_ratio_info_present_flag) {
			flag(vui_aspect_ratio_constant_flag);
			ub(8, vui_aspect_ratio_idc);
			if (current->vui_aspect_ratio_idc == 255) {
				ub(16, vui_sar_width);
				ub(16, vui_sar_height);
			}
		} else {
			infer(vui_aspect_ratio_constant_flag, 0);
			infer(vui_aspect_ratio_idc, 0);
		}
		flag(vui_overscan_info_present_flag);
		if (current->vui_overscan_info_present_flag)
			flag(vui_overscan_appropriate_flag);
		flag(vui_colour_description_present_flag);
		if (current->vui_colour_description_present_flag) {
			ub(8, vui_colour_primaries);
			ub(8, vui_transfer_characteristics);
			ub(8, vui_matrix_coeffs);
			flag(vui_full_range_flag);
		} else {
			infer(vui_colour_primaries, 2);
			infer(vui_transfer_characteristics, 2);
			infer(vui_matrix_coeffs, 2);
			infer(vui_full_range_flag, 0);
		}
		flag(vui_chroma_loc_info_present_flag);
		if (chroma_format_idc != 1 && current->vui_chroma_loc_info_present_flag) {
			return false;
		}
		if (current->vui_chroma_loc_info_present_flag) {
			if (current->vui_progressive_source_flag &&
				!current->vui_interlaced_source_flag) {
				ue(vui_chroma_sample_loc_type_frame, 0, 6);
			} else {
				ue(vui_chroma_sample_loc_type_top_field, 0, 6);
				ue(vui_chroma_sample_loc_type_bottom_field, 0, 6);
			}
		} else {
			if (chroma_format_idc == 1) {
				infer(vui_chroma_sample_loc_type_frame, 6);
				infer(vui_chroma_sample_loc_type_top_field,
					  current->vui_chroma_sample_loc_type_frame);
				infer(vui_chroma_sample_loc_type_bottom_field,
					  current->vui_chroma_sample_loc_type_frame);
			}
		}

		return true;
	}

	static bool vui_payload(CGolombBuffer& gb,
							H266RawVUI* current, uint16_t vui_payload_size,
							uint8_t chroma_format_idc)
	{
		CHECK(vui_parameters(gb, current, chroma_format_idc));

		return true;
	}

	static bool sps_range_extension(CGolombBuffer& gb,
									H266RawSPS* current)
	{
		flag(sps_extended_precision_flag);
		if (current->sps_transform_skip_enabled_flag) {
			flag(sps_ts_residual_coding_rice_present_in_sh_flag);
		}
		else
			infer(sps_ts_residual_coding_rice_present_in_sh_flag, 0);
		flag(sps_rrc_rice_extension_flag);
		flag(sps_persistent_rice_adaptation_enabled_flag);
		flag(sps_reverse_last_sig_coeff_enabled_flag);

		return 0;
	}

	constexpr uint8_t h266_sub_width_c[] = {
		1, 2, 2, 1
	};
	constexpr uint8_t h266_sub_height_c[] = {
		1, 2, 1, 1
	};

	bool ParseSequenceParameterSet(const BYTE* data, const int size, vc_params_vvc_t& params)
	{
		CGolombBuffer gb(data, size, true);
		auto rawSPS = std::make_unique<H266RawSPS>();
		auto current = rawSPS.get();

		ub(4, sps_seq_parameter_set_id);
		ub(4, sps_video_parameter_set_id);

		u(3, sps_max_sublayers_minus1, 0, VVC_MAX_SUBLAYERS - 1);
		u(2, sps_chroma_format_idc, 0, 3);
		u(2, sps_log2_ctu_size_minus5, 0, 3);
		uint32_t ctb_log2_size_y = current->sps_log2_ctu_size_minus5 + 5;
		uint32_t ctb_size_y = 1 << ctb_log2_size_y;

		flag(sps_ptl_dpb_hrd_params_present_flag);
		if (current->sps_ptl_dpb_hrd_params_present_flag) {
			CHECK(profile_tier_level(gb, &current->profile_tier_level,
									 1, current->sps_max_sublayers_minus1));
		}

		flag(sps_gdr_enabled_flag);
		flag(sps_ref_pic_resampling_enabled_flag);
		if (current->sps_ref_pic_resampling_enabled_flag)
			flag(sps_res_change_in_clvs_allowed_flag);

		ue(sps_pic_width_max_in_luma_samples, 1, VVC_MAX_WIDTH);
		ue(sps_pic_height_max_in_luma_samples, 1, VVC_MAX_HEIGHT);

		flag(sps_conformance_window_flag);
		if (current->sps_conformance_window_flag) {
			uint8_t sub_width_c = h266_sub_width_c[current->sps_chroma_format_idc];
			uint8_t sub_height_c = h266_sub_height_c[current->sps_chroma_format_idc];
			uint16_t width = current->sps_pic_width_max_in_luma_samples / sub_width_c;
			uint16_t height = current->sps_pic_height_max_in_luma_samples / sub_height_c;
			ue(sps_conf_win_left_offset, 0, width);
			ue(sps_conf_win_right_offset, 0, width - current->sps_conf_win_left_offset);
			ue(sps_conf_win_top_offset, 0, height);
			ue(sps_conf_win_bottom_offset, 0, height - current->sps_conf_win_top_offset);
		}

		uint32_t tmp_width_val = AV_CEIL_RSHIFT(current->sps_pic_width_max_in_luma_samples,
												ctb_log2_size_y);
		uint32_t tmp_height_val = AV_CEIL_RSHIFT(current->sps_pic_height_max_in_luma_samples,
												 ctb_log2_size_y);

		flag(sps_subpic_info_present_flag);
		if (current->sps_subpic_info_present_flag) {
			ue(sps_num_subpics_minus1, 0, VVC_MAX_SLICES - 1);
			if (current->sps_num_subpics_minus1 > 0) {
				flag(sps_independent_subpics_flag);
				flag(sps_subpic_same_size_flag);
			}

			if (current->sps_num_subpics_minus1 > 0) {
				int wlen = av_ceil_log2(tmp_width_val);
				int hlen = av_ceil_log2(tmp_height_val);
				infer(sps_subpic_ctu_top_left_x[0], 0);
				infer(sps_subpic_ctu_top_left_y[0], 0);
				if (current->sps_pic_width_max_in_luma_samples > ctb_size_y) {
					ubs(wlen, sps_subpic_width_minus1[0], 1, 0);
				}
				else
					infer(sps_subpic_width_minus1[0], tmp_width_val - 1);
				if (current->sps_pic_height_max_in_luma_samples > ctb_size_y) {
					ubs(hlen, sps_subpic_height_minus1[0], 1, 0);
				}
				else
					infer(sps_subpic_height_minus1[0], tmp_height_val - 1);
				if (!current->sps_independent_subpics_flag) {
					flags(sps_subpic_treated_as_pic_flag[0], 1, 0);
					flags(sps_loop_filter_across_subpic_enabled_flag[0], 1, 0);
				} else {
					infer(sps_subpic_treated_as_pic_flag[0], 1);
					infer(sps_loop_filter_across_subpic_enabled_flag[0], 1);
				}
				for (int i = 1; i <= current->sps_num_subpics_minus1; i++) {
					if (!current->sps_subpic_same_size_flag) {
						if (current->sps_pic_width_max_in_luma_samples > ctb_size_y) {
							ubs(wlen, sps_subpic_ctu_top_left_x[i], 1, i);
						}
						else
							infer(sps_subpic_ctu_top_left_x[i], 0);
						if (current->sps_pic_height_max_in_luma_samples >
							ctb_size_y) {
							ubs(hlen, sps_subpic_ctu_top_left_y[i], 1, i);
						}
						else
							infer(sps_subpic_ctu_top_left_y[i], 0);
						if (i < current->sps_num_subpics_minus1 &&
							current->sps_pic_width_max_in_luma_samples >
							ctb_size_y) {
							ubs(wlen, sps_subpic_width_minus1[i], 1, i);
						} else {
							infer(sps_subpic_width_minus1[i],
								  tmp_width_val -
								  current->sps_subpic_ctu_top_left_x[i] - 1);
						}
						if (i < current->sps_num_subpics_minus1 &&
							current->sps_pic_height_max_in_luma_samples >
							ctb_size_y) {
							ubs(hlen, sps_subpic_height_minus1[i], 1, i);
						} else {
							infer(sps_subpic_height_minus1[i],
								  tmp_height_val -
								  current->sps_subpic_ctu_top_left_y[i] - 1);
						}
					} else {
						int num_subpic_cols = tmp_width_val /
							(current->sps_subpic_width_minus1[0] + 1);
						if (tmp_width_val % (current->sps_subpic_width_minus1[0] + 1) ||
							tmp_height_val % (current->sps_subpic_width_minus1[0] + 1) ||
							current->sps_num_subpics_minus1 !=
							(num_subpic_cols * tmp_height_val /
							 (current->sps_subpic_height_minus1[0] + 1) - 1))
							return false;
						infer(sps_subpic_ctu_top_left_x[i],
							  (i % num_subpic_cols) *
							  (current->sps_subpic_width_minus1[0] + 1));
						infer(sps_subpic_ctu_top_left_y[i],
							  (i / num_subpic_cols) *
							  (current->sps_subpic_height_minus1[0] + 1));
						infer(sps_subpic_width_minus1[i],
							  current->sps_subpic_width_minus1[0]);
						infer(sps_subpic_height_minus1[i],
							  current->sps_subpic_height_minus1[0]);
					}
					if (!current->sps_independent_subpics_flag) {
						flags(sps_subpic_treated_as_pic_flag[i], 1, i);
						flags(sps_loop_filter_across_subpic_enabled_flag[i], 1, i);
					} else {
						infer(sps_subpic_treated_as_pic_flag[i], 1);
						infer(sps_loop_filter_across_subpic_enabled_flag[i], 0);
					}
				}
			} else {
				infer(sps_subpic_ctu_top_left_x[0], 0);
				infer(sps_subpic_ctu_top_left_y[0], 0);
				infer(sps_subpic_width_minus1[0], tmp_width_val - 1);
				infer(sps_subpic_height_minus1[0], tmp_height_val - 1);
			}
			ue(sps_subpic_id_len_minus1, 0, 15);
			if ((1 << (current->sps_subpic_id_len_minus1 + 1)) <
				current->sps_num_subpics_minus1 + 1) {
				return false;
			}
			flag(sps_subpic_id_mapping_explicitly_signalled_flag);
			if (current->sps_subpic_id_mapping_explicitly_signalled_flag) {
				flag(sps_subpic_id_mapping_present_flag);
				if (current->sps_subpic_id_mapping_present_flag) {
					for (int i = 0; i <= current->sps_num_subpics_minus1; i++) {
						ubs(current->sps_subpic_id_len_minus1 + 1,
							sps_subpic_id[i], 1, static_cast<uint32_t>(i));
					}
				}
			}
		} else {
			infer(sps_num_subpics_minus1, 0);
			infer(sps_independent_subpics_flag, 1);
			infer(sps_subpic_same_size_flag, 0);
			infer(sps_subpic_id_mapping_explicitly_signalled_flag, 0);
			infer(sps_subpic_ctu_top_left_x[0], 0);
			infer(sps_subpic_ctu_top_left_y[0], 0);
			infer(sps_subpic_width_minus1[0], tmp_width_val - 1);
			infer(sps_subpic_height_minus1[0], tmp_height_val - 1);
		}

		ue(sps_bitdepth_minus8, 0, 8);
		uint8_t qp_bd_offset = 6 * current->sps_bitdepth_minus8;

		flag(sps_entropy_coding_sync_enabled_flag);
		flag(sps_entry_point_offsets_present_flag);

		u(4, sps_log2_max_pic_order_cnt_lsb_minus4, 0, 12);
		flag(sps_poc_msb_cycle_flag);
		if (current->sps_poc_msb_cycle_flag)
			ue(sps_poc_msb_cycle_len_minus1,
			   0, 32 - current->sps_log2_max_pic_order_cnt_lsb_minus4 - 5);

		u(2, sps_num_extra_ph_bytes, 0, 2);
		for (int i = 0; i < (current->sps_num_extra_ph_bytes * 8); i++) {
			flags(sps_extra_ph_bit_present_flag[i], 1, i);
		}

		u(2, sps_num_extra_sh_bytes, 0, 2);
		for (int i = 0; i < (current->sps_num_extra_sh_bytes * 8); i++) {
			flags(sps_extra_sh_bit_present_flag[i], 1, i);
		}

		if (current->sps_ptl_dpb_hrd_params_present_flag) {
			if (current->sps_max_sublayers_minus1 > 0) {
				flag(sps_sublayer_dpb_params_flag);
			}
			else
				infer(sps_sublayer_dpb_params_flag, 0);
			CHECK(dpb_parameters(gb, &current->sps_dpb_params,
								 current->sps_max_sublayers_minus1,
								 current->sps_sublayer_dpb_params_flag));
		}

		ue(sps_log2_min_luma_coding_block_size_minus2,
		   0, std::min(4, current->sps_log2_ctu_size_minus5 + 3));
		uint32_t min_cb_log2_size_y =
			current->sps_log2_min_luma_coding_block_size_minus2 + 2;

		flag(sps_partition_constraints_override_enabled_flag);

		ue(sps_log2_diff_min_qt_min_cb_intra_slice_luma,
		   0, std::min(6u, ctb_log2_size_y) - min_cb_log2_size_y);
		uint32_t min_qt_log2_size_intra_y =
			current->sps_log2_diff_min_qt_min_cb_intra_slice_luma +
			min_cb_log2_size_y;

		ue(sps_max_mtt_hierarchy_depth_intra_slice_luma,
		   0, 2 * (ctb_log2_size_y - min_cb_log2_size_y));

		if (current->sps_max_mtt_hierarchy_depth_intra_slice_luma != 0) {
			ue(sps_log2_diff_max_bt_min_qt_intra_slice_luma,
			   0, ctb_log2_size_y - min_qt_log2_size_intra_y);
			ue(sps_log2_diff_max_tt_min_qt_intra_slice_luma,
			   0, std::min(6u, ctb_log2_size_y) - min_qt_log2_size_intra_y);
		} else {
			infer(sps_log2_diff_max_bt_min_qt_intra_slice_luma, 0);
			infer(sps_log2_diff_max_tt_min_qt_intra_slice_luma, 0);
		}

		if (current->sps_chroma_format_idc != 0) {
			flag(sps_qtbtt_dual_tree_intra_flag);
		} else {
			infer(sps_qtbtt_dual_tree_intra_flag, 0);
		}

		if (current->sps_qtbtt_dual_tree_intra_flag) {
			ue(sps_log2_diff_min_qt_min_cb_intra_slice_chroma,
			   0, std::min(6u, ctb_log2_size_y) - min_cb_log2_size_y);
			ue(sps_max_mtt_hierarchy_depth_intra_slice_chroma,
			   0, 2 * (ctb_log2_size_y - min_cb_log2_size_y));
			if (current->sps_max_mtt_hierarchy_depth_intra_slice_chroma != 0) {
				unsigned int min_qt_log2_size_intra_c =
					current->sps_log2_diff_min_qt_min_cb_intra_slice_chroma +
					min_cb_log2_size_y;
				ue(sps_log2_diff_max_bt_min_qt_intra_slice_chroma,
				   0, std::min(6u, ctb_log2_size_y) - min_qt_log2_size_intra_c);
				ue(sps_log2_diff_max_tt_min_qt_intra_slice_chroma,
				   0, std::min(6u, ctb_log2_size_y) - min_qt_log2_size_intra_c);
			}
		} else {
			infer(sps_log2_diff_min_qt_min_cb_intra_slice_chroma, 0);
			infer(sps_max_mtt_hierarchy_depth_intra_slice_chroma, 0);
		}
		if (current->sps_max_mtt_hierarchy_depth_intra_slice_chroma == 0) {
			infer(sps_log2_diff_max_bt_min_qt_intra_slice_chroma, 0);
			infer(sps_log2_diff_max_tt_min_qt_intra_slice_chroma, 0);
		}

		ue(sps_log2_diff_min_qt_min_cb_inter_slice,
		   0, std::min(6u, ctb_log2_size_y) - min_cb_log2_size_y);
		uint32_t min_qt_log2_size_inter_y =
			current->sps_log2_diff_min_qt_min_cb_inter_slice + min_cb_log2_size_y;

		ue(sps_max_mtt_hierarchy_depth_inter_slice,
		   0, 2 * (ctb_log2_size_y - min_cb_log2_size_y));
		if (current->sps_max_mtt_hierarchy_depth_inter_slice != 0) {
			ue(sps_log2_diff_max_bt_min_qt_inter_slice,
			   0, ctb_log2_size_y - min_qt_log2_size_inter_y);
			ue(sps_log2_diff_max_tt_min_qt_inter_slice,
			   0, std::min(6u, ctb_log2_size_y) - min_qt_log2_size_inter_y);
		} else {
			infer(sps_log2_diff_max_bt_min_qt_inter_slice, 0);
			infer(sps_log2_diff_max_tt_min_qt_inter_slice, 0);
		}

		if (ctb_size_y > 32) {
			flag(sps_max_luma_transform_size_64_flag);
		}
		else
			infer(sps_max_luma_transform_size_64_flag, 0);

		flag(sps_transform_skip_enabled_flag);
		if (current->sps_transform_skip_enabled_flag) {
			ue(sps_log2_transform_skip_max_size_minus2, 0, 3);
			flag(sps_bdpcm_enabled_flag);
		}

		flag(sps_mts_enabled_flag);
		if (current->sps_mts_enabled_flag) {
			flag(sps_explicit_mts_intra_enabled_flag);
			flag(sps_explicit_mts_inter_enabled_flag);
		} else {
			infer(sps_explicit_mts_intra_enabled_flag, 0);
			infer(sps_explicit_mts_inter_enabled_flag, 0);
		}

		flag(sps_lfnst_enabled_flag);

		if (current->sps_chroma_format_idc != 0) {
			uint8_t num_qp_tables;
			flag(sps_joint_cbcr_enabled_flag);
			flag(sps_same_qp_table_for_chroma_flag);
			num_qp_tables = current->sps_same_qp_table_for_chroma_flag ?
				1 : (current->sps_joint_cbcr_enabled_flag ? 3 : 2);
			for (int i = 0; i < num_qp_tables; i++) {
				ses(sps_qp_table_start_minus26[i], -26 - qp_bd_offset, 36, 1, i);
				ues(sps_num_points_in_qp_table_minus1[i],
					0, 36 - current->sps_qp_table_start_minus26[i], 1, i);
				for (int j = 0; j <= current->sps_num_points_in_qp_table_minus1[i]; j++) {
					uint8_t max = MAX_UINT_BITS(8);
					ues(sps_delta_qp_in_val_minus1[i][j], 0, max, 2, i, j);
					ues(sps_delta_qp_diff_val[i][j], 0, max, 2, i, j);
				}
			}
		} else {
			infer(sps_joint_cbcr_enabled_flag, 0);
			infer(sps_same_qp_table_for_chroma_flag, 0);
		}

		flag(sps_sao_enabled_flag);
		flag(sps_alf_enabled_flag);
		if (current->sps_alf_enabled_flag && current->sps_chroma_format_idc) {
			flag(sps_ccalf_enabled_flag);
		}
		else
			infer(sps_ccalf_enabled_flag, 0);
		flag(sps_lmcs_enabled_flag);
		flag(sps_weighted_pred_flag);
		flag(sps_weighted_bipred_flag);
		flag(sps_long_term_ref_pics_flag);
		if (current->sps_video_parameter_set_id > 0) {
			flag(sps_inter_layer_prediction_enabled_flag);
		}
		else
			infer(sps_inter_layer_prediction_enabled_flag, 0);
		flag(sps_idr_rpl_present_flag);
		flag(sps_rpl1_same_as_rpl0_flag);

		for (int i = 0; i < (current->sps_rpl1_same_as_rpl0_flag ? 1 : 2); i++) {
			ues(sps_num_ref_pic_lists[i], 0, VVC_MAX_REF_PIC_LISTS, 1, i);
			for (int j = 0; j < current->sps_num_ref_pic_lists[i]; j++)
				CHECK(ref_pic_list_struct(gb, &current->sps_ref_pic_list_struct[i][j], i,
										  j, current));
		}

		if (current->sps_rpl1_same_as_rpl0_flag) {
			current->sps_num_ref_pic_lists[1] = current->sps_num_ref_pic_lists[0];
			for (int j = 0; j < current->sps_num_ref_pic_lists[0]; j++)
				memcpy(&current->sps_ref_pic_list_struct[1][j],
					   &current->sps_ref_pic_list_struct[0][j],
					   sizeof(current->sps_ref_pic_list_struct[0][j]));
		}

		flag(sps_ref_wraparound_enabled_flag);

		flag(sps_temporal_mvp_enabled_flag);
		if (current->sps_temporal_mvp_enabled_flag) {
			flag(sps_sbtmvp_enabled_flag);
		}
		else
			infer(sps_sbtmvp_enabled_flag, 0);

		flag(sps_amvr_enabled_flag);
		flag(sps_bdof_enabled_flag);
		if (current->sps_bdof_enabled_flag) {
			flag(sps_bdof_control_present_in_ph_flag);
		}
		else
			infer(sps_bdof_control_present_in_ph_flag, 0);

		flag(sps_smvd_enabled_flag);
		flag(sps_dmvr_enabled_flag);
		if (current->sps_dmvr_enabled_flag) {
			flag(sps_dmvr_control_present_in_ph_flag);
		}
		else
			infer(sps_dmvr_control_present_in_ph_flag, 0);

		flag(sps_mmvd_enabled_flag);
		if (current->sps_mmvd_enabled_flag) {
			flag(sps_mmvd_fullpel_only_enabled_flag);
		}
		else
			infer(sps_mmvd_fullpel_only_enabled_flag, 0);

		ue(sps_six_minus_max_num_merge_cand, 0, 5);
		uint32_t max_num_merge_cand = 6 - current->sps_six_minus_max_num_merge_cand;

		flag(sps_sbt_enabled_flag);

		flag(sps_affine_enabled_flag);
		if (current->sps_affine_enabled_flag) {
			ue(sps_five_minus_max_num_subblock_merge_cand,
			   0, 5 - current->sps_sbtmvp_enabled_flag);
			flag(sps_6param_affine_enabled_flag);
			if (current->sps_amvr_enabled_flag) {
				flag(sps_affine_amvr_enabled_flag);
			}
			else
				infer(sps_affine_amvr_enabled_flag, 0);
			flag(sps_affine_prof_enabled_flag);
			if (current->sps_affine_prof_enabled_flag) {
				flag(sps_prof_control_present_in_ph_flag);
			}
			else
				infer(sps_prof_control_present_in_ph_flag, 0);
		} else {
			infer(sps_6param_affine_enabled_flag, 0);
			infer(sps_affine_amvr_enabled_flag, 0);
			infer(sps_affine_prof_enabled_flag, 0);
			infer(sps_prof_control_present_in_ph_flag, 0);
		}

		flag(sps_bcw_enabled_flag);
		flag(sps_ciip_enabled_flag);

		if (max_num_merge_cand >= 2) {
			flag(sps_gpm_enabled_flag);
			if (current->sps_gpm_enabled_flag && max_num_merge_cand >= 3)
				ue(sps_max_num_merge_cand_minus_max_num_gpm_cand,
				   0, max_num_merge_cand - 2);
		} else {
			infer(sps_gpm_enabled_flag, 0);
		}

		ue(sps_log2_parallel_merge_level_minus2, 0, ctb_log2_size_y - 2);

		flag(sps_isp_enabled_flag);
		flag(sps_mrl_enabled_flag);
		flag(sps_mip_enabled_flag);

		if (current->sps_chroma_format_idc != 0) {
			flag(sps_cclm_enabled_flag);
		}
		else
			infer(sps_cclm_enabled_flag, 0);
		if (current->sps_chroma_format_idc == 1) {
			flag(sps_chroma_horizontal_collocated_flag);
			flag(sps_chroma_vertical_collocated_flag);
		} else {
			infer(sps_chroma_horizontal_collocated_flag, 1);
			infer(sps_chroma_vertical_collocated_flag, 1);
		}

		flag(sps_palette_enabled_flag);
		if (current->sps_chroma_format_idc == 3 &&
			!current->sps_max_luma_transform_size_64_flag) {
			flag(sps_act_enabled_flag);
		}
		else
			infer(sps_act_enabled_flag, 0);
		if (current->sps_transform_skip_enabled_flag ||
			current->sps_palette_enabled_flag)
			ue(sps_min_qp_prime_ts, 0, 8);

		flag(sps_ibc_enabled_flag);
		if (current->sps_ibc_enabled_flag)
			ue(sps_six_minus_max_num_ibc_merge_cand, 0, 5);

		flag(sps_ladf_enabled_flag);
		if (current->sps_ladf_enabled_flag) {
			ub(2, sps_num_ladf_intervals_minus2);
			se(sps_ladf_lowest_interval_qp_offset, -63, 63);
			for (int i = 0; i < current->sps_num_ladf_intervals_minus2 + 1; i++) {
				ses(sps_ladf_qp_offset[i], -63, 63, 1, i);
				ues(sps_ladf_delta_threshold_minus1[i],
					0, (2 << (8 + current->sps_bitdepth_minus8)) - 3, 1, i);
			}
		}

		flag(sps_explicit_scaling_list_enabled_flag);
		if (current->sps_lfnst_enabled_flag &&
			current->sps_explicit_scaling_list_enabled_flag)
			flag(sps_scaling_matrix_for_lfnst_disabled_flag);

		if (current->sps_act_enabled_flag &&
			current->sps_explicit_scaling_list_enabled_flag) {
			flag(sps_scaling_matrix_for_alternative_colour_space_disabled_flag);
		}
		else
			infer(sps_scaling_matrix_for_alternative_colour_space_disabled_flag, 0);
		if (current->sps_scaling_matrix_for_alternative_colour_space_disabled_flag)
			flag(sps_scaling_matrix_designated_colour_space_flag);

		flag(sps_dep_quant_enabled_flag);
		flag(sps_sign_data_hiding_enabled_flag);

		flag(sps_virtual_boundaries_enabled_flag);
		if (current->sps_virtual_boundaries_enabled_flag) {
			flag(sps_virtual_boundaries_present_flag);
			if (current->sps_virtual_boundaries_present_flag) {
				ue(sps_num_ver_virtual_boundaries,
				   0, current->sps_pic_width_max_in_luma_samples <= 8 ? 0 : 3);
				for (int i = 0; i < current->sps_num_ver_virtual_boundaries; i++)
					ues(sps_virtual_boundary_pos_x_minus1[i],
						0, (current->sps_pic_width_max_in_luma_samples + 7) / 8 - 2,
						1, i);
				ue(sps_num_hor_virtual_boundaries,
				   0, current->sps_pic_height_max_in_luma_samples <= 8 ? 0 : 3);
				for (int i = 0; i < current->sps_num_hor_virtual_boundaries; i++)
					ues(sps_virtual_boundary_pos_y_minus1[i],
						0, (current->sps_pic_height_max_in_luma_samples + 7) /
						8 - 2, 1, i);
			}
		} else {
			infer(sps_virtual_boundaries_present_flag, 0);
			infer(sps_num_ver_virtual_boundaries, 0);
			infer(sps_num_hor_virtual_boundaries, 0);
		}

		if (current->sps_ptl_dpb_hrd_params_present_flag) {
			flag(sps_timing_hrd_params_present_flag);
			if (current->sps_timing_hrd_params_present_flag) {
				uint8_t first_sublayer;
				CHECK(general_timing_hrd_parameters(gb,
													&current->sps_general_timing_hrd_parameters));
				if (current->sps_max_sublayers_minus1 > 0) {
					flag(sps_sublayer_cpb_params_present_flag);
				}
				else
					infer(sps_sublayer_cpb_params_present_flag, 0);
				first_sublayer = current->sps_sublayer_cpb_params_present_flag ?
					0 : current->sps_max_sublayers_minus1;
				CHECK(ols_timing_hrd_parameters(gb,
												&current->sps_ols_timing_hrd_parameters, first_sublayer,
												current->sps_max_sublayers_minus1,
												&current->sps_general_timing_hrd_parameters));
			}
		}

		flag(sps_field_seq_flag);
		flag(sps_vui_parameters_present_flag);
		if (current->sps_vui_parameters_present_flag) {
			ue(sps_vui_payload_size_minus1, 0, 1023);
			while (byte_alignment(gb) != 0)
				fixed(1, sps_vui_alignment_zero_bit, 0);
			CHECK(vui_payload(gb, &current->vui,
							  current->sps_vui_payload_size_minus1 + 1,
							  current->sps_chroma_format_idc));
		} else {
			CHECK(vui_parameters_default(&current->vui));
		}

		auto sps = rawSPS.get();

		params.sps_pic_width_max_in_luma_samples = sps->sps_pic_width_max_in_luma_samples;
		params.sps_pic_height_max_in_luma_samples = sps->sps_pic_height_max_in_luma_samples;
		params.sps_log2_min_luma_coding_block_size_minus2 = sps->sps_log2_min_luma_coding_block_size_minus2;
		params.sps_chroma_format_idc = sps->sps_chroma_format_idc;
		params.sps_res_change_in_clvs_allowed_flag = sps->sps_res_change_in_clvs_allowed_flag;
		params.sps_log2_ctu_size_minus5 = sps->sps_log2_ctu_size_minus5;
		params.sps_ref_wraparound_enabled_flag = sps->sps_ref_wraparound_enabled_flag;

		params.sps_conf_win_left_offset = sps->sps_conf_win_left_offset;
		params.sps_conf_win_right_offset = sps->sps_conf_win_right_offset;
		params.sps_conf_win_top_offset = sps->sps_conf_win_top_offset;
		params.sps_conf_win_bottom_offset = sps->sps_conf_win_bottom_offset;

		params.profile = sps->profile_tier_level.general_profile_idc;
		params.level = sps->profile_tier_level.general_level_idc;

		params.sar.num = params.sar.num = 1;
		if (sps->vui.vui_sar_width && sps->vui.vui_sar_height) {
			params.sar.num = sps->vui.vui_sar_width;
			params.sar.den = sps->vui.vui_sar_height;
		}

		if (sps->sps_ptl_dpb_hrd_params_present_flag && sps->sps_timing_hrd_params_present_flag) {
			params.sps_timing.num_units_in_tick = sps->sps_general_timing_hrd_parameters.num_units_in_tick;
			params.sps_timing.time_scale = sps->sps_general_timing_hrd_parameters.time_scale;
		}

		return true;
	}

	bool ParsePictureParameterSet(const BYTE* data, const int size, vc_params_vvc_t& params)
	{
		CGolombBuffer gb(data, size, true);
		H266RawPPS pps = {};
		auto current = &pps;

		ub(6, pps_pic_parameter_set_id);
		ub(4, pps_seq_parameter_set_id);

		flag(pps_mixed_nalu_types_in_pic_flag);
		ue(pps_pic_width_in_luma_samples,
		   1, params.sps_pic_width_max_in_luma_samples);
		ue(pps_pic_height_in_luma_samples,
		   1, params.sps_pic_height_max_in_luma_samples);

		uint32_t min_cb_size_y = 1 << (params.sps_log2_min_luma_coding_block_size_minus2 + 2);
		uint32_t divisor = std::max(min_cb_size_y, 8u);
		if (current->pps_pic_width_in_luma_samples % divisor ||
			current->pps_pic_height_in_luma_samples % divisor) {
			return false;
		}
		if (!params.sps_res_change_in_clvs_allowed_flag &&
			(current->pps_pic_width_in_luma_samples !=
			 params.sps_pic_width_max_in_luma_samples ||
			 current->pps_pic_height_in_luma_samples !=
			 params.sps_pic_height_max_in_luma_samples)) {
			return false;
		}

		uint32_t ctb_size_y = 1 << (params.sps_log2_ctu_size_minus5 + 5);
		if (params.sps_ref_wraparound_enabled_flag) {
			if ((ctb_size_y / min_cb_size_y + 1) >
				(current->pps_pic_width_in_luma_samples / min_cb_size_y - 1)) {
				return false;
			}
		}

		flag(pps_conformance_window_flag);
		if (current->pps_pic_width_in_luma_samples ==
			params.sps_pic_width_max_in_luma_samples &&
			current->pps_pic_height_in_luma_samples ==
			params.sps_pic_height_max_in_luma_samples &&
			current->pps_conformance_window_flag) {
			return false;
		}

		uint8_t sub_width_c = h266_sub_width_c[params.sps_chroma_format_idc];
		uint8_t sub_height_c = h266_sub_height_c[params.sps_chroma_format_idc];
		if (current->pps_conformance_window_flag) {
			ue(pps_conf_win_left_offset, 0, current->pps_pic_width_in_luma_samples);
			ue(pps_conf_win_right_offset,
			   0, current->pps_pic_width_in_luma_samples);
			ue(pps_conf_win_top_offset, 0, current->pps_pic_height_in_luma_samples);
			ue(pps_conf_win_bottom_offset,
			   0, current->pps_pic_height_in_luma_samples);
			if (sub_width_c *
				(current->pps_conf_win_left_offset +
				 current->pps_conf_win_right_offset) >=
				current->pps_pic_width_in_luma_samples ||
				sub_height_c *
				(current->pps_conf_win_top_offset +
				 current->pps_conf_win_bottom_offset) >=
				current->pps_pic_height_in_luma_samples) {
				return false;
			}
		} else {
			if (current->pps_pic_width_in_luma_samples ==
				params.sps_pic_width_max_in_luma_samples &&
				current->pps_pic_height_in_luma_samples ==
				params.sps_pic_height_max_in_luma_samples) {
				infer(pps_conf_win_left_offset, params.sps_conf_win_left_offset);
				infer(pps_conf_win_right_offset, params.sps_conf_win_right_offset);
				infer(pps_conf_win_top_offset, params.sps_conf_win_top_offset);
				infer(pps_conf_win_bottom_offset, params.sps_conf_win_bottom_offset);
			} else {
				infer(pps_conf_win_left_offset, 0);
				infer(pps_conf_win_right_offset, 0);
				infer(pps_conf_win_top_offset, 0);
				infer(pps_conf_win_bottom_offset, 0);
			}
		}

		params.width = pps.pps_pic_width_in_luma_samples -
			(pps.pps_conf_win_left_offset + pps.pps_conf_win_right_offset) *
			h266_sub_width_c[params.sps_chroma_format_idc];
		params.height = pps.pps_pic_height_in_luma_samples -
			(pps.pps_conf_win_top_offset + pps.pps_conf_win_bottom_offset) *
			h266_sub_height_c[params.sps_chroma_format_idc];

		return true;
	}
} // namespace VVCParser

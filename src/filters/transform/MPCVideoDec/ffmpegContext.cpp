/*
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

#define HAVE_AV_CONFIG_H

#include <Windows.h>
#include <time.h> // for the _time64 workaround
#include <algorithm>
#include <mpc_defines.h>
#include "DSUtil/FileVersion.h"
#include "ffmpegContext.h"

#pragma warning(push)
#pragma warning(disable: 4005)
#pragma warning(disable: 5033)
extern "C" {
	#include <ExtLib/ffmpeg/libavutil/pixdesc.h>
// This is kind of an hack but it avoids using a C++ keyword as a struct member name
#define class classFFMPEG
	#include <ExtLib/ffmpeg/libavcodec/h264dec.h>
	#include <ExtLib/ffmpeg/libavcodec/ffv1.h>
#undef class
	//#include <ExtLib/ffmpeg/libavcodec/vvc/dec.h>
}
#pragma warning(pop)

static const UINT16 PCID_ATI_UVD [] = {
	0x94C7, // ATI Radeon HD 2350
	0x94C1, // ATI Radeon HD 2400 XT
	0x94CC, // ATI Radeon HD 2400 Series
	0x958A, // ATI Radeon HD 2600 X2 Series
	0x9588, // ATI Radeon HD 2600 XT
	0x9405, // ATI Radeon HD 2900 GT
	0x9400, // ATI Radeon HD 2900 XT
	0x9611, // ATI Radeon 3100 Graphics
	0x9610, // ATI Radeon HD 3200 Graphics
	0x9614, // ATI Radeon HD 3300 Graphics
	0x95C0, // ATI Radeon HD 3400 Series (and others)
	0x95C5, // ATI Radeon HD 3400 Series (and others)
	0x95C4, // ATI Radeon HD 3400 Series (and others)
	0x94C3, // ATI Radeon HD 3410
	0x9589, // ATI Radeon HD 3600 Series (and others)
	0x9598, // ATI Radeon HD 3600 Series (and others)
	0x9591, // ATI Radeon HD 3600 Series (and others)
	0x9501, // ATI Radeon HD 3800 Series (and others)
	0x9505, // ATI Radeon HD 3800 Series (and others)
	0x9507, // ATI Radeon HD 3830
	0x9513, // ATI Radeon HD 3850 X2
	0x9515, // ATI Radeon HD 3850 AGP
	0x950F, // ATI Radeon HD 3850 X2
};

static bool CheckPCID(UINT pcid, const UINT16* pPCIDs, size_t count)
{
	UINT16 wPCID = (UINT16)pcid;
	for (size_t i = 0; i < count; i++) {
		if (wPCID == pPCIDs[i]) {
			return true;
		}
	}

	return false;
}

// === H264 functions

int FFH264CheckCompatibility(int nWidth, int nHeight, struct AVCodecContext* pAVCtx,
							 UINT nPCIVendor, UINT nPCIDevice, UINT64 VideoDriverVersion)
{
	const H264Context* h = (H264Context*)pAVCtx->priv_data;
	const SPS* sps = h264_getSPS(h);
	int flags = 0;

	if (sps) {
		int video_is_level51 = 0;
		int no_level51_support = 1;
		int too_much_ref_frames = 0;
		const int max_ref_frames_dpb41 = std::min(11, 8388608 / (nWidth * nHeight));

		if (sps->bit_depth_luma > 8 || sps->chroma_format_idc > 1) {
			return DXVA_HIGH_BIT;
		}

		if (sps->profile_idc > 100) {
			return DXVA_PROFILE_HIGHER_THAN_HIGH;
		}

		video_is_level51   = sps->level_idc >= 51 ? 1 : 0;
		int max_ref_frames = max_ref_frames_dpb41; // default value is calculate

		if (nPCIVendor == PCIV_nVidia) {
			// nVidia cards support level 5.1 since drivers v7.15.11.7800 for Vista/7
			if (VideoDriverVersion >= FileVersion::Ver(7, 15, 11, 7800).value) {
				no_level51_support = 0;
				max_ref_frames = 16;
			}
		} else if (nPCIVendor == PCIV_ATI && !CheckPCID(nPCIDevice, PCID_ATI_UVD, std::size(PCID_ATI_UVD))) {
			WCHAR path[MAX_PATH] = { 0 };
			GetSystemDirectoryW(path, MAX_PATH);
			wcscat(path, L"\\drivers\\atikmdag.sys\0");
			UINT64 atikmdag_ver = FileVersion::GetVer(path).value;

			if (atikmdag_ver) {
				// file version 8.1.1.1016 - Catalyst 10.4, WinVista & Win7
				if (atikmdag_ver >= FileVersion::Ver(8, 1, 1, 1016).value) {
					no_level51_support = 0;
					max_ref_frames = 16;
				}
			} else {
				// driver version 8.14.1.6105 - Catalyst 10.4; TODO - verify this information
				if (VideoDriverVersion >= FileVersion::Ver(8, 14, 1, 6105).value) {
					no_level51_support = 0;
					max_ref_frames = 16;
				}
			}
		} else if (nPCIVendor == PCIV_S3_Graphics || nPCIVendor == PCIV_Intel || nPCIVendor == PCIV_Qualcomm) {
			no_level51_support = 0;
			max_ref_frames = 16;
		}

		// Check maximum allowed number reference frames
		if (sps->ref_frame_count > max_ref_frames) {
			too_much_ref_frames = 1;
		}

		if (video_is_level51 * no_level51_support) {
			flags |= DXVA_UNSUPPORTED_LEVEL;
		}
		if (too_much_ref_frames) {
			flags |= DXVA_TOO_MANY_REF_FRAMES;
		}
	}

	return flags;
}

void FillAVCodecProps(struct AVCodecContext* pAVCtx, BITMAPINFOHEADER* pBMI)
{
	// fill "Pixel format" properties
	if (pAVCtx->pix_fmt == AV_PIX_FMT_NONE) {
		switch (pAVCtx->codec_id) {
		case AV_CODEC_ID_MPEG4:
			switch (pAVCtx->bits_per_coded_sample) {
			case 20:
				pAVCtx->pix_fmt = AV_PIX_FMT_YUV422P10LE;
				break;
			case 30:
				pAVCtx->pix_fmt = AV_PIX_FMT_GBRP10LE;
				break;
			default:
				pAVCtx->pix_fmt = AV_PIX_FMT_YUV420P;
			}
			break;
		case AV_CODEC_ID_LAGARITH:
			if (pAVCtx->extradata_size >= 4) {
				switch (GETU32(pAVCtx->extradata)) { // "lossy_option"
				case 0:
					switch (pAVCtx->bits_per_coded_sample) {
					case 32: pAVCtx->pix_fmt = AV_PIX_FMT_RGBA; break;
					default:
					case 24: pAVCtx->pix_fmt = AV_PIX_FMT_RGB24; break;
					case 16: pAVCtx->pix_fmt = AV_PIX_FMT_YUV422P; break;
					case 12: pAVCtx->pix_fmt = AV_PIX_FMT_YUV420P; break;
					}
					break;
				case 1:
					pAVCtx->pix_fmt = AV_PIX_FMT_YUV422P;
					break;
				case 2:
					pAVCtx->pix_fmt = AV_PIX_FMT_YUV420P;
					break;
				}
			}
			break;
		case AV_CODEC_ID_PRORES:
			switch (pAVCtx->codec_tag) {
			case FCC('apch'): // Apple ProRes 422 High Quality
			case FCC('apcn'): // Apple ProRes 422 Standard Definition
			case FCC('apcs'): // Apple ProRes 422 LT
			case FCC('apco'): // Apple ProRes 422 Proxy
			case FCC('icpf'): // unknown type after matroska splitter
				pAVCtx->pix_fmt = AV_PIX_FMT_YUV422P10LE;
				break;
			case FCC('ap4h'): // Apple ProRes 4444
			case FCC('ap4x'): // Apple ProRes 4444 XQ
				pAVCtx->pix_fmt = AV_PIX_FMT_YUV444P12LE;
				break;
			}
			break;
		case AV_CODEC_ID_PNG:
		case AV_CODEC_ID_JPEG2000:
			if (pBMI->biBitCount > 32) {
				pAVCtx->pix_fmt = AV_PIX_FMT_RGB48BE;
			} else {
				pAVCtx->pix_fmt = AV_PIX_FMT_RGBA; // and other RGB formats, but it is not important here
			}
			break;
		case AV_CODEC_ID_HQ_HQA:
			pAVCtx->pix_fmt = AV_PIX_FMT_YUV422P;
			break;
		case AV_CODEC_ID_HQX:
			pAVCtx->pix_fmt = AV_PIX_FMT_YUV422P16; // or AV_PIX_FMT_YUV444P16
			break;
		case AV_CODEC_ID_CFHD:
			pAVCtx->pix_fmt = AV_PIX_FMT_YUV422P10; // most common format
			break;
		case AV_CODEC_ID_MJPEG:
			pAVCtx->pix_fmt = AV_PIX_FMT_YUV422P; // most common format
			pAVCtx->color_range = AVCOL_RANGE_JPEG;
			break;
		case AV_CODEC_ID_DNXHD:
			pAVCtx->pix_fmt = AV_PIX_FMT_YUV422P; // most common format
			break;
		case AV_CODEC_ID_MAGICYUV:
			if (pAVCtx->extradata_size >= 32 && *(DWORD*)pAVCtx->extradata == FCC('MAGY')) {
				int hsize = GETU32(pAVCtx->extradata + 4);
				if (hsize >= 32 && pAVCtx->extradata[8] == 7) {
					switch (pAVCtx->extradata[9]) {
					case 0x65: pAVCtx->pix_fmt = AV_PIX_FMT_GBRP;      break;
					case 0x66: pAVCtx->pix_fmt = AV_PIX_FMT_GBRAP;     break;
					case 0x67: pAVCtx->pix_fmt = AV_PIX_FMT_YUV444P;   break;
					case 0x68: pAVCtx->pix_fmt = AV_PIX_FMT_YUV422P;   break;
					case 0x69: pAVCtx->pix_fmt = AV_PIX_FMT_YUV420P;   break;
					case 0x6a: pAVCtx->pix_fmt = AV_PIX_FMT_YUVA444P;  break;
					case 0x6b: pAVCtx->pix_fmt = AV_PIX_FMT_GRAY8;     break;
					case 0x6c: pAVCtx->pix_fmt = AV_PIX_FMT_YUV422P10; break;
					case 0x6d: pAVCtx->pix_fmt = AV_PIX_FMT_GBRP10;    break;
					case 0x6e: pAVCtx->pix_fmt = AV_PIX_FMT_GBRAP10;   break;
					case 0x6f: pAVCtx->pix_fmt = AV_PIX_FMT_GBRP12;    break;
					case 0x70: pAVCtx->pix_fmt = AV_PIX_FMT_GBRAP12;   break;
					case 0x73: pAVCtx->pix_fmt = AV_PIX_FMT_GRAY10;    break;
					}
				}
			}
			else if (pAVCtx->extradata_size >= 8) {
				switch (GETU32(pAVCtx->extradata + 4)) {
				case FCC('M8RG'): pAVCtx->pix_fmt = AV_PIX_FMT_GBRP;      break;
				case FCC('M8RA'): pAVCtx->pix_fmt = AV_PIX_FMT_GBRAP;     break;
				case FCC('M8Y4'): pAVCtx->pix_fmt = AV_PIX_FMT_YUV444P;   break;
				case FCC('M8Y2'): pAVCtx->pix_fmt = AV_PIX_FMT_YUV422P;   break;
				case FCC('M8Y0'): pAVCtx->pix_fmt = AV_PIX_FMT_YUV420P;   break;
				case FCC('M8YA'): pAVCtx->pix_fmt = AV_PIX_FMT_YUVA444P;  break;
				case FCC('M8G0'): pAVCtx->pix_fmt = AV_PIX_FMT_GRAY8;     break;
				case FCC('M0Y2'): pAVCtx->pix_fmt = AV_PIX_FMT_YUV422P10; break;
				case FCC('M0RG'): pAVCtx->pix_fmt = AV_PIX_FMT_GBRP10;    break;
				case FCC('M0RA'): pAVCtx->pix_fmt = AV_PIX_FMT_GBRAP10;   break;
				case FCC('M2RG'): pAVCtx->pix_fmt = AV_PIX_FMT_GBRP12;    break;
				case FCC('M2RA'): pAVCtx->pix_fmt = AV_PIX_FMT_GBRAP12;   break;
				case FCC('M0R0'): pAVCtx->pix_fmt = AV_PIX_FMT_GRAY10;    break;
				}
			}
			break;
		case AV_CODEC_ID_FFV1:
			if (pAVCtx->priv_data) {
				auto f = static_cast<FFV1Context*>(pAVCtx->priv_data);
				if (!f->version) {
					if (ff_ffv1_parse_extra_data(pAVCtx) < 0) {
						break;
					}
				}
				if (f->version > 0) { // extra data has been parse
					if (f->colorspace == 0) {
						auto chorma_shift = 16 * f->chroma_h_shift + f->chroma_v_shift;

						if (pAVCtx->bits_per_raw_sample <= 8) { // <=8 bit and transparency
							switch (chorma_shift) {
							case 0x00: pAVCtx->pix_fmt = AV_PIX_FMT_YUV444P; break;
							case 0x01: pAVCtx->pix_fmt = AV_PIX_FMT_YUV440P; break;
							case 0x10: pAVCtx->pix_fmt = AV_PIX_FMT_YUV422P; break;
							case 0x11: pAVCtx->pix_fmt = AV_PIX_FMT_YUV420P; break;
							case 0x20: pAVCtx->pix_fmt = AV_PIX_FMT_YUV411P; break;
							case 0x22: pAVCtx->pix_fmt = AV_PIX_FMT_YUV410P; break;
							}
						}
						else if (pAVCtx->bits_per_raw_sample <= 10) { // 9, 10 bit and transparency
							switch (chorma_shift) {
							case 0x00: pAVCtx->pix_fmt = AV_PIX_FMT_YUV444P10; break;
							case 0x10: pAVCtx->pix_fmt = AV_PIX_FMT_YUV422P10; break;
							case 0x11: pAVCtx->pix_fmt = AV_PIX_FMT_YUV420P10; break;
							}
						}
						else if (pAVCtx->bits_per_raw_sample <= 16) { // 12, 14, 16 bit and transparency
							switch (chorma_shift) {
							case 0x00: pAVCtx->pix_fmt = AV_PIX_FMT_YUV444P16; break;
							case 0x10: pAVCtx->pix_fmt = AV_PIX_FMT_YUV422P16; break;
							case 0x11: pAVCtx->pix_fmt = AV_PIX_FMT_YUV420P16; break;
							}
						}
					}
					else if (f->colorspace == 1) {
						if (pAVCtx->bits_per_raw_sample <= 8) {
							pAVCtx->pix_fmt = AV_PIX_FMT_RGBA;
						}
						else {
							pAVCtx->pix_fmt = AV_PIX_FMT_RGB48;
						}
					}
				}
			}
			break;
		case AV_CODEC_ID_RPZA:
		case AV_CODEC_ID_CINEPAK:
		case AV_CODEC_ID_MSRLE:
		case AV_CODEC_ID_MSVIDEO1:
		case AV_CODEC_ID_8BPS:
		case AV_CODEC_ID_QTRLE:
		case AV_CODEC_ID_TSCC:
		case AV_CODEC_ID_CSCD:
		case AV_CODEC_ID_VMNC:
		case AV_CODEC_ID_FLASHSV2:
		case AV_CODEC_ID_MSS1:
		case AV_CODEC_ID_G2M:
		case AV_CODEC_ID_DXTORY:
			pAVCtx->pix_fmt = AV_PIX_FMT_RGB24; // and other RGB formats, but it is not important here
			break;
		/*
		case AV_CODEC_ID_VVC:
			{
				auto s = reinterpret_cast<VVCContext*>(pAVCtx->priv_data);
				if (s->cbc && s->cbc->priv_data) {
					auto h266 = reinterpret_cast<CodedBitstreamH266Context*>(s->cbc->priv_data);
					auto pps = h266->pps[0];
					if (pps) {
						auto sps = h266->sps[pps->pps_seq_parameter_set_id];
						if (sps) {
							auto get_format = [](const H266RawSPS* sps) {
								static const AVPixelFormat pix_fmts_8bit[] = {
									AV_PIX_FMT_GRAY8, AV_PIX_FMT_YUV420P,
									AV_PIX_FMT_YUV422P, AV_PIX_FMT_YUV444P
								};

								static const AVPixelFormat pix_fmts_10bit[] = {
									AV_PIX_FMT_GRAY10, AV_PIX_FMT_YUV420P10,
									AV_PIX_FMT_YUV422P10, AV_PIX_FMT_YUV444P10
								};

								switch (sps->sps_bitdepth_minus8) {
									case 0:
										return pix_fmts_8bit[sps->sps_chroma_format_idc];
									case 2:
										return pix_fmts_10bit[sps->sps_chroma_format_idc];
								}

								return AV_PIX_FMT_NONE;
							};

							pAVCtx->pix_fmt = get_format(sps);
							pAVCtx->profile = sps->profile_tier_level.general_profile_idc;
							pAVCtx->level = sps->profile_tier_level.general_level_idc;
							pAVCtx->colorspace = static_cast<AVColorSpace>(sps->vui.vui_matrix_coeffs);
							pAVCtx->color_primaries = static_cast<AVColorPrimaries>(sps->vui.vui_colour_primaries);
							pAVCtx->color_trc = static_cast<AVColorTransferCharacteristic>(sps->vui.vui_transfer_characteristics);
							pAVCtx->color_range = sps->vui.vui_full_range_flag ? AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG;

							if (sps->sps_ptl_dpb_hrd_params_present_flag && sps->sps_timing_hrd_params_present_flag) {
								int64_t num = sps->sps_general_timing_hrd_parameters.num_units_in_tick;
								int64_t den = sps->sps_general_timing_hrd_parameters.time_scale;

								if (num != 0 && den != 0) {
									av_reduce(&pAVCtx->framerate.den, &pAVCtx->framerate.num, num, den, 1LL << 30);
								}
							}
						}
					}
				}
			}
			break;
		*/
		}

		if (pAVCtx->pix_fmt == AV_PIX_FMT_NONE) {
			pAVCtx->pix_fmt = AV_PIX_FMT_YUV420P; // most common format
			av_log(nullptr, AV_LOG_WARNING, "FillAVCodecProps: Unknown pixel format for %s, use AV_PIX_FMT_YUV420P.\n", pAVCtx->codec->name);
		}
	}
}

bool IsATIUVD(UINT nPCIVendor, UINT nPCIDevice)
{
	return (nPCIVendor == PCIV_ATI && CheckPCID(nPCIDevice, PCID_ATI_UVD, std::size(PCID_ATI_UVD)));
}

BOOL DXVACheckFramesize(int width, int height, UINT nPCIVendor, UINT nPCIDevice, UINT64 VideoDriverVersion)
{
	width = (width + 15) & ~15; // (width + 15) / 16 * 16;
	height = (height + 15) & ~15; // (height + 15) / 16 * 16;

	if (nPCIVendor == PCIV_nVidia) {
		if (VideoDriverVersion >= FileVersion::Ver(9, 18, 13, 2018).value) {
			// For Nvidia graphics cards with support for 4k, you must install the driver v320.18 or newer.
			return TRUE;
		}
		if (width <= 2032 && height <= 2032 && width * height <= 8190 * 16 * 16) {
			// tested H.264, VC-1 and MPEG-2 on VP4 (feature set C) (G210M, GT220)
			return TRUE;
		}
	}
	else if (nPCIVendor == PCIV_ATI) {
		if (VideoDriverVersion >= FileVersion::Ver(8, 17, 10, 1404).value) { // aticfx32.dll/aticfx64.dll (v8.17.10.1404)
			// For AMD graphics cards with support for 4k, you must install the Catalyst v15.7.1 or newer
			return TRUE;
		}
		if (width <= 2048 && height <= 2304 && width * height <= 2048 * 2048) {
			// tested H.264 on UVD 2.2 (HD5670, HD5770, HD5850)
			// it may also work if width = 2064, but unstable
			return TRUE;
		}
	}
	else if (nPCIVendor == PCIV_Intel && VideoDriverVersion >= FileVersion::Ver(10, 18, 10, 4061).value) {
		// For Intel graphics cards with support for 4k, you must install the driver v15.33.32.4061 or newer.
		return TRUE;
	}
	else if (nPCIVendor == PCIV_Qualcomm) {
		return TRUE;
	}
	else if ((width <= 1920 && height <= 1088)
			|| (width <= 720 && height <= 1280)) {
		return TRUE;
	}

	return FALSE;
}

void FixFrameSize(enum AVPixelFormat pixfmt, int& width, int& height)
{
	const AVPixFmtDescriptor* av_pfdesc = av_pix_fmt_desc_get(pixfmt);
	if (av_pfdesc) {
		if (av_pfdesc->log2_chroma_w == 1 && (width & 1)) {
			width += 1;
		}
		if (av_pfdesc->log2_chroma_h == 1 && (height & 1)) {
			height -= 1;
		}
	}
}

void FixFrameSize(struct AVCodecContext* pAVCtx, int& width, int& height)
{
	const AVPixelFormat pixfmt = pAVCtx->sw_pix_fmt != AV_PIX_FMT_NONE ? pAVCtx->sw_pix_fmt : pAVCtx->pix_fmt;
	FixFrameSize(pixfmt, width, height);
}

/*
 * (C) 2006-2017 see Authors.txt
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
#include <atlbase.h>
#include <MMReg.h>
#include "../../../DSUtil/ff_log.h"

#ifdef REGISTER_FILTER
	#include <InitGuid.h>
#endif

#include "MPCVideoDec.h"
#include "DXVAAllocator.h"
#include "FfmpegContext.h"

#include "../../../DSUtil/CPUInfo.h"
#include "../../../DSUtil/D3D9Helper.h"
#include "../../../DSUtil/DSUtil.h"
#include "../../../DSUtil/SysVersion.h"
#include "../../../DSUtil/WinAPIUtils.h"
#include "../../parser/MpegSplitter/MpegSplitter.h"
#include "../../Lock.h"
#include <moreuuids.h>

#include "Version.h"

#pragma warning(push)
#pragma warning(disable: 4005)
extern "C" {
	#include <ffmpeg/libavcodec/avcodec.h>
	#include <ffmpeg/libavutil/mastering_display_metadata.h>
	#include <ffmpeg/libavutil/pixdesc.h>
	#include <ffmpeg/libavutil/imgutils.h>
}
#pragma warning(pop)

#pragma comment(lib, "libmfx.lib")

// option names
#define OPT_REGKEY_VideoDec  L"Software\\MPC-BE Filters\\MPC Video Decoder"
#define OPT_SECTION_VideoDec L"Filters\\MPC Video Decoder"
#define OPT_ThreadNumber     L"ThreadNumber"
#define OPT_DiscardMode      L"DiscardMode"
#define OPT_Deinterlacing    L"Deinterlacing"
#define OPT_ARMode           L"ARMode"
#define OPT_DXVACheck        L"DXVACheckCompatibility"
#define OPT_DisableDXVA_SD   L"DisableDXVA_SD"
#define OPT_SW_prefix        L"Sw_"
#define OPT_SwRGBLevels      L"SwRGBLevels"

#define MAX_AUTO_THREADS 16

#pragma region any_constants

#ifdef REGISTER_FILTER
#define OPT_REGKEY_VCodecs   L"Software\\MPC-BE Filters\\MPC Video Decoder\\Codecs"

static const struct vcodec_t {
	const LPCWSTR          opt_name;
	const unsigned __int64 flag;
}
vcodecs[] = {
	{L"h264",			CODEC_H264		},
	{L"h264_mvc",		CODEC_H264_MVC	},
	{L"mpeg1",			CODEC_MPEG1		},
	{L"mpeg3",			CODEC_MPEG2		},
	{L"vc1",			CODEC_VC1		},
	{L"msmpeg4",		CODEC_MSMPEG4	},
	{L"xvid",			CODEC_XVID		},
	{L"divx",			CODEC_DIVX		},
	{L"wmv",			CODEC_WMV		},
	{L"hevc",			CODEC_HEVC		},
	{L"vp356",			CODEC_VP356		},
	{L"vp89",			CODEC_VP89		},
	{L"theora",			CODEC_THEORA	},
	{L"mjpeg",			CODEC_MJPEG		},
	{L"dv",				CODEC_DV		},
	{L"lossless",		CODEC_LOSSLESS	},
	{L"prores",			CODEC_PRORES	},
	{L"canopus",		CODEC_CANOPUS	},
	{L"screc",			CODEC_SCREC		},
	{L"indeo",			CODEC_INDEO		},
	{L"h263",			CODEC_H263		},
	{L"svq3",			CODEC_SVQ3		},
	{L"realv",			CODEC_REALV		},
	{L"dirac",			CODEC_DIRAC		},
	{L"binkv",			CODEC_BINKV		},
	{L"amvv",			CODEC_AMVV		},
	{L"flash",			CODEC_FLASH		},
	{L"utvd",			CODEC_UTVD		},
	{L"png",			CODEC_PNG		},
	{L"uncompressed",	CODEC_UNCOMPRESSED},
	{L"dnxhd",			CODEC_DNXHD		},
	{L"cinepak",		CODEC_CINEPAK	},
	{L"quicktime",		CODEC_QT		},
	{L"cineform",		CODEC_CINEFORM	},
	// dxva codecs
	{L"h264_dxva",		CODEC_H264_DXVA	},
	{L"hevc_dxva",		CODEC_HEVC_DXVA	},
	{L"mpeg2_dxva",		CODEC_MPEG2_DXVA},
	{L"vc1_dxva",		CODEC_VC1_DXVA	},
	{L"wmv3_dxva",		CODEC_WMV3_DXVA	},
	{L"vp9_dxva",		CODEC_VP9_DXVA	}
};
#endif

struct FFMPEG_CODECS {
	const CLSID*         clsMinorType;
	const enum AVCodecID nFFCodec;

	const int            FFMPEGCode;
	const int            DXVACode;
};

struct {
	const enum AVCodecID nCodecId;
	const GUID           decoderGUID;
	const bool           bHighBitdepth;
} DXVAModes [] = {
	// H.264
	{ AV_CODEC_ID_H264, DXVA2_ModeH264_E, false },
	{ AV_CODEC_ID_H264, DXVA2_ModeH264_F, false },
	{ AV_CODEC_ID_H264, DXVA_Intel_H264_ClearVideo, false },
	// HEVC
	{ AV_CODEC_ID_HEVC, DXVA2_ModeHEVC_VLD_Main10, true },
	{ AV_CODEC_ID_HEVC, DXVA2_ModeHEVC_VLD_Main, false },
	// MPEG2
	{ AV_CODEC_ID_MPEG2VIDEO, DXVA2_ModeMPEG2_VLD, false },
	// VC1
	{ AV_CODEC_ID_VC1, DXVA2_ModeVC1_D2010, false },
	{ AV_CODEC_ID_VC1, DXVA2_ModeVC1_D, false },
	// WMV3
	{ AV_CODEC_ID_WMV3, DXVA2_ModeVC1_D2010, false },
	{ AV_CODEC_ID_WMV3, DXVA2_ModeVC1_D, false },
	// VP9
	{ AV_CODEC_ID_VP9, DXVA_ModeVP9_VLD_10bit_Profile2, true },
	{ AV_CODEC_ID_VP9, DXVA_ModeVP9_VLD_Profile0, false },
	{ AV_CODEC_ID_VP9, DXVA_VP9_VLD_Intel, false },
};

FFMPEG_CODECS ffCodecs[] = {
	// Flash video
	{ &MEDIASUBTYPE_FLV1, AV_CODEC_ID_FLV1, VDEC_FLV, -1 },
	{ &MEDIASUBTYPE_flv1, AV_CODEC_ID_FLV1, VDEC_FLV, -1 },
	{ &MEDIASUBTYPE_FLV4, AV_CODEC_ID_VP6F, VDEC_FLV, -1 },
	{ &MEDIASUBTYPE_flv4, AV_CODEC_ID_VP6F, VDEC_FLV, -1 },
	{ &MEDIASUBTYPE_VP6F, AV_CODEC_ID_VP6F, VDEC_FLV, -1 },
	{ &MEDIASUBTYPE_vp6f, AV_CODEC_ID_VP6F, VDEC_FLV, -1 },

	// VP3
	{ &MEDIASUBTYPE_VP30, AV_CODEC_ID_VP3,  VDEC_VP356, -1 },
	{ &MEDIASUBTYPE_VP31, AV_CODEC_ID_VP3,  VDEC_VP356, -1 },

	// VP5
	{ &MEDIASUBTYPE_VP50, AV_CODEC_ID_VP5,  VDEC_VP356, -1 },
	{ &MEDIASUBTYPE_vp50, AV_CODEC_ID_VP5,  VDEC_VP356, -1 },

	// VP6
	{ &MEDIASUBTYPE_VP60, AV_CODEC_ID_VP6,  VDEC_VP356, -1 },
	{ &MEDIASUBTYPE_vp60, AV_CODEC_ID_VP6,  VDEC_VP356, -1 },
	{ &MEDIASUBTYPE_VP61, AV_CODEC_ID_VP6,  VDEC_VP356, -1 },
	{ &MEDIASUBTYPE_vp61, AV_CODEC_ID_VP6,  VDEC_VP356, -1 },
	{ &MEDIASUBTYPE_VP62, AV_CODEC_ID_VP6,  VDEC_VP356, -1 },
	{ &MEDIASUBTYPE_vp62, AV_CODEC_ID_VP6,  VDEC_VP356, -1 },
	{ &MEDIASUBTYPE_VP6A, AV_CODEC_ID_VP6A, VDEC_VP356, -1 },
	{ &MEDIASUBTYPE_vp6a, AV_CODEC_ID_VP6A, VDEC_VP356, -1 },

	// VP7
	{ &MEDIASUBTYPE_VP70, AV_CODEC_ID_VP7, VDEC_VP789, -1 },

	// VP8
	{ &MEDIASUBTYPE_VP80, AV_CODEC_ID_VP8, VDEC_VP789, -1 },

	// VP9
	{ &MEDIASUBTYPE_VP90, AV_CODEC_ID_VP9, VDEC_VP789, VDEC_DXVA_VP9 },

	// Xvid
	{ &MEDIASUBTYPE_XVID, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_xvid, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_XVIX, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_xvix, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },

	// DivX
	{ &MEDIASUBTYPE_DX50, AV_CODEC_ID_MPEG4, VDEC_DIVX, -1 },
	{ &MEDIASUBTYPE_dx50, AV_CODEC_ID_MPEG4, VDEC_DIVX, -1 },
	{ &MEDIASUBTYPE_DIVX, AV_CODEC_ID_MPEG4, VDEC_DIVX, -1 },
	{ &MEDIASUBTYPE_divx, AV_CODEC_ID_MPEG4, VDEC_DIVX, -1 },
	{ &MEDIASUBTYPE_Divx, AV_CODEC_ID_MPEG4, VDEC_DIVX, -1 },

	// WMV1/2/3
	{ &MEDIASUBTYPE_WMV1, AV_CODEC_ID_WMV1, VDEC_WMV, -1 },
	{ &MEDIASUBTYPE_wmv1, AV_CODEC_ID_WMV1, VDEC_WMV, -1 },
	{ &MEDIASUBTYPE_WMV2, AV_CODEC_ID_WMV2, VDEC_WMV, -1 },
	{ &MEDIASUBTYPE_wmv2, AV_CODEC_ID_WMV2, VDEC_WMV, -1 },
	{ &MEDIASUBTYPE_WMV3, AV_CODEC_ID_WMV3, VDEC_WMV, VDEC_DXVA_WMV3 },
	{ &MEDIASUBTYPE_wmv3, AV_CODEC_ID_WMV3, VDEC_WMV, VDEC_DXVA_WMV3 },
	// WMVP
	{ &MEDIASUBTYPE_WMVP, AV_CODEC_ID_WMV3IMAGE, VDEC_WMV, -1 },

	// MPEG-2
	{ &MEDIASUBTYPE_MPEG2_VIDEO, AV_CODEC_ID_MPEG2VIDEO, VDEC_MPEG2, VDEC_DXVA_MPEG2 },
	{ &MEDIASUBTYPE_MPG2,        AV_CODEC_ID_MPEG2VIDEO, VDEC_MPEG2, VDEC_DXVA_MPEG2 },

	// MPEG-1
	{ &MEDIASUBTYPE_MPEG1Packet,  AV_CODEC_ID_MPEG1VIDEO, VDEC_MPEG1, -1 },
	{ &MEDIASUBTYPE_MPEG1Payload, AV_CODEC_ID_MPEG1VIDEO, VDEC_MPEG1, -1 },

	// MSMPEG-4
	{ &MEDIASUBTYPE_DIV3, AV_CODEC_ID_MSMPEG4V3, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_div3, AV_CODEC_ID_MSMPEG4V3, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_DVX3, AV_CODEC_ID_MSMPEG4V3, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_dvx3, AV_CODEC_ID_MSMPEG4V3, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_MP43, AV_CODEC_ID_MSMPEG4V3, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_mp43, AV_CODEC_ID_MSMPEG4V3, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_COL1, AV_CODEC_ID_MSMPEG4V3, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_col1, AV_CODEC_ID_MSMPEG4V3, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_DIV4, AV_CODEC_ID_MSMPEG4V3, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_div4, AV_CODEC_ID_MSMPEG4V3, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_DIV5, AV_CODEC_ID_MSMPEG4V3, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_div5, AV_CODEC_ID_MSMPEG4V3, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_DIV6, AV_CODEC_ID_MSMPEG4V3, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_div6, AV_CODEC_ID_MSMPEG4V3, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_AP41, AV_CODEC_ID_MSMPEG4V3, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_ap41, AV_CODEC_ID_MSMPEG4V3, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_MPG3, AV_CODEC_ID_MSMPEG4V3, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_mpg3, AV_CODEC_ID_MSMPEG4V3, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_DIV2, AV_CODEC_ID_MSMPEG4V2, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_div2, AV_CODEC_ID_MSMPEG4V2, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_MP42, AV_CODEC_ID_MSMPEG4V2, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_mp42, AV_CODEC_ID_MSMPEG4V2, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_MPG4, AV_CODEC_ID_MSMPEG4V1, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_mpg4, AV_CODEC_ID_MSMPEG4V1, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_DIV1, AV_CODEC_ID_MSMPEG4V1, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_div1, AV_CODEC_ID_MSMPEG4V1, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_MP41, AV_CODEC_ID_MSMPEG4V1, VDEC_MSMPEG4, -1 },
	{ &MEDIASUBTYPE_mp41, AV_CODEC_ID_MSMPEG4V1, VDEC_MSMPEG4, -1 },

	// AMV Video
	{ &MEDIASUBTYPE_AMVV, AV_CODEC_ID_AMV, VDEC_AMV, -1 },

	// MJPEG
	{ &MEDIASUBTYPE_MJPG,   AV_CODEC_ID_MJPEG,    VDEC_MJPEG, -1 },
	{ &MEDIASUBTYPE_QTJpeg, AV_CODEC_ID_MJPEG,    VDEC_MJPEG, -1 },
	{ &MEDIASUBTYPE_MJPA,   AV_CODEC_ID_MJPEG,    VDEC_MJPEG, -1 },
	{ &MEDIASUBTYPE_MJPB,   AV_CODEC_ID_MJPEGB,   VDEC_MJPEG, -1 },
	{ &MEDIASUBTYPE_MJP2,   AV_CODEC_ID_JPEG2000, VDEC_MJPEG, -1 },
	{ &MEDIASUBTYPE_MJ2C,   AV_CODEC_ID_JPEG2000, VDEC_MJPEG, -1 },

	// Cinepak
	{ &MEDIASUBTYPE_CVID, AV_CODEC_ID_CINEPAK, VDEC_CINEPAK, -1 },

	// DV VIDEO
	{ &MEDIASUBTYPE_dvsl, AV_CODEC_ID_DVVIDEO, VDEC_DV, -1 },
	{ &MEDIASUBTYPE_dvsd, AV_CODEC_ID_DVVIDEO, VDEC_DV, -1 },
	{ &MEDIASUBTYPE_dvhd, AV_CODEC_ID_DVVIDEO, VDEC_DV, -1 },
	{ &MEDIASUBTYPE_dv25, AV_CODEC_ID_DVVIDEO, VDEC_DV, -1 },
	{ &MEDIASUBTYPE_dv50, AV_CODEC_ID_DVVIDEO, VDEC_DV, -1 },
	{ &MEDIASUBTYPE_dvh1, AV_CODEC_ID_DVVIDEO, VDEC_DV, -1 },
	{ &MEDIASUBTYPE_CDVH, AV_CODEC_ID_DVVIDEO, VDEC_DV, -1 },
	{ &MEDIASUBTYPE_CDVC, AV_CODEC_ID_DVVIDEO, VDEC_DV, -1 },
	// Quicktime DV sybtypes (used in LAV Splitter)
	{ &MEDIASUBTYPE_DVCP, AV_CODEC_ID_DVVIDEO, VDEC_DV, -1 },
	{ &MEDIASUBTYPE_DVPP, AV_CODEC_ID_DVVIDEO, VDEC_DV, -1 },
	{ &MEDIASUBTYPE_DV5P, AV_CODEC_ID_DVVIDEO, VDEC_DV, -1 },
	{ &MEDIASUBTYPE_DVC,  AV_CODEC_ID_DVVIDEO, VDEC_DV, -1 },

	// Quicktime
	{ &MEDIASUBTYPE_8BPS,   AV_CODEC_ID_8BPS,  VDEC_QT, -1 },
	{ &MEDIASUBTYPE_QTRle,  AV_CODEC_ID_QTRLE, VDEC_QT, -1 },
	{ &MEDIASUBTYPE_QTRpza, AV_CODEC_ID_RPZA,  VDEC_QT, -1 },

	// Screen recorder
	{ &MEDIASUBTYPE_CSCD,     AV_CODEC_ID_CSCD,     VDEC_SCREEN, -1 },
	{ &MEDIASUBTYPE_TSCC,     AV_CODEC_ID_TSCC,     VDEC_SCREEN, -1 },
	{ &MEDIASUBTYPE_TSCC2,    AV_CODEC_ID_TSCC2,    VDEC_SCREEN, -1 },
	{ &MEDIASUBTYPE_VMnc,     AV_CODEC_ID_VMNC,     VDEC_SCREEN, -1 },
	{ &MEDIASUBTYPE_FLASHSV1, AV_CODEC_ID_FLASHSV,  VDEC_SCREEN, -1 },
//	{ &MEDIASUBTYPE_FLASHSV2, AV_CODEC_ID_FLASHSV2, VDEC_SCREEN, -1 },
	{ &MEDIASUBTYPE_FPS1,     AV_CODEC_ID_FRAPS,    VDEC_SCREEN, -1 },
	{ &MEDIASUBTYPE_MSS1,     AV_CODEC_ID_MSS1,     VDEC_SCREEN, -1 },
	{ &MEDIASUBTYPE_MSS2,     AV_CODEC_ID_MSS2,     VDEC_SCREEN, -1 },
	{ &MEDIASUBTYPE_MSA1,     AV_CODEC_ID_MSA1,     VDEC_SCREEN, -1 },
	{ &MEDIASUBTYPE_MTS2,     AV_CODEC_ID_MTS2,     VDEC_SCREEN, -1 },
	{ &MEDIASUBTYPE_G2M2,     AV_CODEC_ID_G2M,      VDEC_SCREEN, -1 },
	{ &MEDIASUBTYPE_G2M3,     AV_CODEC_ID_G2M,      VDEC_SCREEN, -1 },
	{ &MEDIASUBTYPE_G2M4,     AV_CODEC_ID_G2M,      VDEC_SCREEN, -1 },
	{ &MEDIASUBTYPE_CRAM,     AV_CODEC_ID_MSVIDEO1, VDEC_SCREEN, -1 }, // CRAM - Microsoft Video 1

	// UtVideo
	{ &MEDIASUBTYPE_Ut_ULRG, AV_CODEC_ID_UTVIDEO, VDEC_UT, -1 },
	{ &MEDIASUBTYPE_Ut_ULRA, AV_CODEC_ID_UTVIDEO, VDEC_UT, -1 },
	{ &MEDIASUBTYPE_Ut_ULY0, AV_CODEC_ID_UTVIDEO, VDEC_UT, -1 },
	{ &MEDIASUBTYPE_Ut_ULY2, AV_CODEC_ID_UTVIDEO, VDEC_UT, -1 },
	{ &MEDIASUBTYPE_Ut_UQY2, AV_CODEC_ID_UTVIDEO, VDEC_UT, -1 },
	{ &MEDIASUBTYPE_Ut_UQRG, AV_CODEC_ID_UTVIDEO, VDEC_UT, -1 },
	{ &MEDIASUBTYPE_Ut_UQRA, AV_CODEC_ID_UTVIDEO, VDEC_UT, -1 },
	{ &MEDIASUBTYPE_Ut_ULH0, AV_CODEC_ID_UTVIDEO, VDEC_UT, -1 },
	{ &MEDIASUBTYPE_Ut_ULH2, AV_CODEC_ID_UTVIDEO, VDEC_UT, -1 },

	// DIRAC
	{ &MEDIASUBTYPE_DRAC, AV_CODEC_ID_DIRAC, VDEC_DIRAC, -1 },

	// Lossless Video
	{ &MEDIASUBTYPE_HuffYUV,  AV_CODEC_ID_HUFFYUV,  VDEC_LOSSLESS, -1 },
	{ &MEDIASUBTYPE_Lagarith, AV_CODEC_ID_LAGARITH, VDEC_LOSSLESS, -1 },
	{ &MEDIASUBTYPE_FFVH,     AV_CODEC_ID_FFVHUFF,  VDEC_LOSSLESS, -1 },
	{ &MEDIASUBTYPE_FFV1,     AV_CODEC_ID_FFV1,     VDEC_LOSSLESS, -1 },
	{ &MEDIASUBTYPE_MAGICYUV, AV_CODEC_ID_MAGICYUV, VDEC_LOSSLESS, -1 },

	// Indeo 3/4/5
	{ &MEDIASUBTYPE_IV31, AV_CODEC_ID_INDEO3, VDEC_INDEO, -1 },
	{ &MEDIASUBTYPE_IV32, AV_CODEC_ID_INDEO3, VDEC_INDEO, -1 },
	{ &MEDIASUBTYPE_IV41, AV_CODEC_ID_INDEO4, VDEC_INDEO, -1 },
	{ &MEDIASUBTYPE_IV50, AV_CODEC_ID_INDEO5, VDEC_INDEO, -1 },

	// H264/AVC
	{ &MEDIASUBTYPE_H264,     AV_CODEC_ID_H264, VDEC_H264, VDEC_DXVA_H264 },
	{ &MEDIASUBTYPE_h264,     AV_CODEC_ID_H264, VDEC_H264, VDEC_DXVA_H264 },
	{ &MEDIASUBTYPE_X264,     AV_CODEC_ID_H264, VDEC_H264, VDEC_DXVA_H264 },
	{ &MEDIASUBTYPE_x264,     AV_CODEC_ID_H264, VDEC_H264, VDEC_DXVA_H264 },
	{ &MEDIASUBTYPE_VSSH,     AV_CODEC_ID_H264, VDEC_H264, VDEC_DXVA_H264 },
	{ &MEDIASUBTYPE_vssh,     AV_CODEC_ID_H264, VDEC_H264, VDEC_DXVA_H264 },
	{ &MEDIASUBTYPE_DAVC,     AV_CODEC_ID_H264, VDEC_H264, VDEC_DXVA_H264 },
	{ &MEDIASUBTYPE_davc,     AV_CODEC_ID_H264, VDEC_H264, VDEC_DXVA_H264 },
	{ &MEDIASUBTYPE_PAVC,     AV_CODEC_ID_H264, VDEC_H264, VDEC_DXVA_H264 },
	{ &MEDIASUBTYPE_pavc,     AV_CODEC_ID_H264, VDEC_H264, VDEC_DXVA_H264 },
	{ &MEDIASUBTYPE_AVC1,     AV_CODEC_ID_H264, VDEC_H264, VDEC_DXVA_H264 },
	{ &MEDIASUBTYPE_avc1,     AV_CODEC_ID_H264, VDEC_H264, VDEC_DXVA_H264 },
	{ &MEDIASUBTYPE_H264_bis, AV_CODEC_ID_H264, VDEC_H264, VDEC_DXVA_H264 },

	// H264 MVC
	{ &MEDIASUBTYPE_AMVC, AV_CODEC_ID_H264, VDEC_H264_MVC, -1 },
	{ &MEDIASUBTYPE_MVC1, AV_CODEC_ID_H264, VDEC_H264_MVC, -1 },

	// SVQ3
	{ &MEDIASUBTYPE_SVQ3, AV_CODEC_ID_SVQ3, VDEC_SVQ, -1 },
	// SVQ1
	{ &MEDIASUBTYPE_SVQ1, AV_CODEC_ID_SVQ1, VDEC_SVQ, -1 },

	// H263
	{ &MEDIASUBTYPE_H263, AV_CODEC_ID_H263, VDEC_H263, -1 },
	{ &MEDIASUBTYPE_h263, AV_CODEC_ID_H263, VDEC_H263, -1 },
	{ &MEDIASUBTYPE_S263, AV_CODEC_ID_H263, VDEC_H263, -1 },
	{ &MEDIASUBTYPE_s263, AV_CODEC_ID_H263, VDEC_H263, -1 },

	// Real Video
	{ &MEDIASUBTYPE_RV10, AV_CODEC_ID_RV10, VDEC_REAL, -1 },
	{ &MEDIASUBTYPE_RV20, AV_CODEC_ID_RV20, VDEC_REAL, -1 },
	{ &MEDIASUBTYPE_RV30, AV_CODEC_ID_RV30, VDEC_REAL, -1 },
	{ &MEDIASUBTYPE_RV40, AV_CODEC_ID_RV40, VDEC_REAL, -1 },

	// Theora
	{ &MEDIASUBTYPE_THEORA, AV_CODEC_ID_THEORA, VDEC_THEORA, -1 },
	{ &MEDIASUBTYPE_theora, AV_CODEC_ID_THEORA, VDEC_THEORA, -1 },

	// WVC1
	{ &MEDIASUBTYPE_WVC1, AV_CODEC_ID_VC1, VDEC_VC1, VDEC_DXVA_VC1 },
	{ &MEDIASUBTYPE_wvc1, AV_CODEC_ID_VC1, VDEC_VC1, VDEC_DXVA_VC1 },

	// WMVA
	{ &MEDIASUBTYPE_WMVA, AV_CODEC_ID_VC1, VDEC_VC1, VDEC_DXVA_VC1 },

	// WVP2
	{ &MEDIASUBTYPE_WVP2, AV_CODEC_ID_VC1IMAGE, VDEC_VC1, -1 },

	// Apple ProRes
	{ &MEDIASUBTYPE_apch, AV_CODEC_ID_PRORES, VDEC_PRORES, -1 },
	{ &MEDIASUBTYPE_apcn, AV_CODEC_ID_PRORES, VDEC_PRORES, -1 },
	{ &MEDIASUBTYPE_apcs, AV_CODEC_ID_PRORES, VDEC_PRORES, -1 },
	{ &MEDIASUBTYPE_apco, AV_CODEC_ID_PRORES, VDEC_PRORES, -1 },
	{ &MEDIASUBTYPE_ap4h, AV_CODEC_ID_PRORES, VDEC_PRORES, -1 },
	{ &MEDIASUBTYPE_icpf, AV_CODEC_ID_PRORES, VDEC_PRORES, -1 },
	{ &MEDIASUBTYPE_icod, AV_CODEC_ID_AIC,    VDEC_PRORES, -1 },

	// Bink Video
	{ &MEDIASUBTYPE_BINKVI, AV_CODEC_ID_BINKVIDEO, VDEC_BINK, -1 },
	{ &MEDIASUBTYPE_BINKVB, AV_CODEC_ID_BINKVIDEO, VDEC_BINK, -1 },

	// PNG
	{ &MEDIASUBTYPE_PNG, AV_CODEC_ID_PNG, VDEC_PNG, -1 },

	// Canopus
	{ &MEDIASUBTYPE_CLLC, AV_CODEC_ID_CLLC,   VDEC_CANOPUS, -1 },
	{ &MEDIASUBTYPE_CUVC, AV_CODEC_ID_HQ_HQA, VDEC_CANOPUS, -1 },
	{ &MEDIASUBTYPE_CHQX, AV_CODEC_ID_HQX,    VDEC_CANOPUS, -1 },

	// CineForm
	{ &MEDIASUBTYPE_CFHD, AV_CODEC_ID_CFHD, VDEC_CINEFORM, -1 },

	// HEVC
	{ &MEDIASUBTYPE_HEVC, AV_CODEC_ID_HEVC, VDEC_HEVC, VDEC_DXVA_HEVC },
	{ &MEDIASUBTYPE_HVC1, AV_CODEC_ID_HEVC, VDEC_HEVC, VDEC_DXVA_HEVC },
//	{ &MEDIASUBTYPE_HM91, AV_CODEC_ID_HEVC, VDEC_HEVC, -1 },
	{ &MEDIASUBTYPE_HM10, AV_CODEC_ID_HEVC, VDEC_HEVC, VDEC_DXVA_HEVC },
//	{ &MEDIASUBTYPE_HM12, AV_CODEC_ID_HEVC, VDEC_HEVC, -1 },

	// Avid DNxHD
	{ &MEDIASUBTYPE_AVdn, AV_CODEC_ID_DNXHD, VDEC_DNXHD, -1 },

	// Other MPEG-4
	{ &MEDIASUBTYPE_MP4V, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_mp4v, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_M4S2, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_m4s2, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_MP4S, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_mp4s, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_3IV1, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_3iv1, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_3IV2, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_3iv2, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_3IVX, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_3ivx, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_BLZ0, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_blz0, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_DM4V, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_dm4v, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_FFDS, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_ffds, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_FVFW, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_fvfw, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_DXGM, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_dxgm, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_FMP4, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_fmp4, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_HDX4, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_hdx4, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_LMP4, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_lmp4, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_NDIG, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_ndig, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_RMP4, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_rmp4, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_SMP4, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_smp4, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_SEDG, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_sedg, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_UMP4, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_ump4, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_WV1F, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },
	{ &MEDIASUBTYPE_wv1f, AV_CODEC_ID_MPEG4, VDEC_XVID, -1 },

	// uncompressed video
	{ &MEDIASUBTYPE_v210, AV_CODEC_ID_V210, VDEC_UNCOMPRESSED, -1 },
	{ &MEDIASUBTYPE_V410, AV_CODEC_ID_V410, VDEC_UNCOMPRESSED, -1 },
	{ &MEDIASUBTYPE_r210, AV_CODEC_ID_R210, VDEC_UNCOMPRESSED, -1 },
	{ &MEDIASUBTYPE_R10g, AV_CODEC_ID_R10K, VDEC_UNCOMPRESSED, -1 },
	{ &MEDIASUBTYPE_R10k, AV_CODEC_ID_R10K, VDEC_UNCOMPRESSED, -1 },

	{ &MEDIASUBTYPE_Y800, AV_CODEC_ID_RAWVIDEO, VDEC_UNCOMPRESSED, -1 },
	{ &MEDIASUBTYPE_I420, AV_CODEC_ID_RAWVIDEO, VDEC_UNCOMPRESSED, -1 },
	{ &MEDIASUBTYPE_Y41B, AV_CODEC_ID_RAWVIDEO, VDEC_UNCOMPRESSED, -1 },
	{ &MEDIASUBTYPE_Y42B, AV_CODEC_ID_RAWVIDEO, VDEC_UNCOMPRESSED, -1 },
	{ &MEDIASUBTYPE_444P, AV_CODEC_ID_RAWVIDEO, VDEC_UNCOMPRESSED, -1 },
	{ &MEDIASUBTYPE_cyuv, AV_CODEC_ID_RAWVIDEO, VDEC_UNCOMPRESSED, -1 },

	{ &MEDIASUBTYPE_YVU9, AV_CODEC_ID_RAWVIDEO, VDEC_UNCOMPRESSED, -1 },
	{ &MEDIASUBTYPE_IYUV, AV_CODEC_ID_RAWVIDEO, VDEC_UNCOMPRESSED, -1 },
	{ &MEDIASUBTYPE_UYVY, AV_CODEC_ID_RAWVIDEO, VDEC_UNCOMPRESSED, -1 },
	{ &MEDIASUBTYPE_YUY2, AV_CODEC_ID_RAWVIDEO, VDEC_UNCOMPRESSED, -1 },

	{ &MEDIASUBTYPE_NV12, AV_CODEC_ID_RAWVIDEO, VDEC_UNCOMPRESSED, -1 },
	{ &MEDIASUBTYPE_YV12, AV_CODEC_ID_RAWVIDEO, VDEC_UNCOMPRESSED, -1 },
	{ &MEDIASUBTYPE_YV16, AV_CODEC_ID_RAWVIDEO, VDEC_UNCOMPRESSED, -1 },
	{ &MEDIASUBTYPE_YV24, AV_CODEC_ID_RAWVIDEO, VDEC_UNCOMPRESSED, -1 },

	{ &MEDIASUBTYPE_b64a, AV_CODEC_ID_RAWVIDEO, VDEC_UNCOMPRESSED, -1 },

	{ &MEDIASUBTYPE_LAV_RAWVIDEO, AV_CODEC_ID_RAWVIDEO, VDEC_UNCOMPRESSED, -1 }
};

/* Important: the order should be exactly the same as in ffCodecs[] */
const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	// Flash video
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_FLV1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_flv1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_FLV4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_flv4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_VP6F },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_vp6f },

	// VP3
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_VP30 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_VP31 },

	// VP5
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_VP50 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_vp50 },

	// VP6
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_VP60 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_vp60 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_VP61 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_vp61 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_VP62 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_vp62 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_VP6A },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_vp6a },

	// VP7
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_VP70 },

	// VP8
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_VP80 },

	// VP9
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_VP90 },

	// Xvid
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_XVID },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_xvid },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_XVIX },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_xvix },

	// DivX
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DX50 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_dx50 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DIVX },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_divx },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_Divx },

	// WMV1/2/3
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_WMV1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_wmv1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_WMV2 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_wmv2 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_WMV3 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_wmv3 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_WMVP },

	// MPEG-2
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MPEG2_VIDEO },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MPG2        },

	// MPEG-1
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MPEG1Packet  },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MPEG1Payload },

	// MSMPEG-4
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DIV3 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_div3 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DVX3 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_dvx3 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MP43 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_mp43 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_COL1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_col1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DIV4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_div4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DIV5 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_div5 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DIV6 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_div6 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_AP41 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_ap41 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MPG3 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_mpg3 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DIV2 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_div2 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MP42 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_mp42 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MPG4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_mpg4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DIV1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_div1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MP41 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_mp41 },

	// AMV Video
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_AMVV },

	// MJPEG
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MJPG   },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_QTJpeg },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MJPA   },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MJPB   },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MJP2   },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MJ2C   },

	// CINEPAK
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_CVID },

	// DV VIDEO
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_dvsl },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_dvsd },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_dvhd },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_dv25 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_dv50 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_dvh1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_CDVH },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_CDVC },
	// Quicktime DV sybtypes (used in LAV Splitter)
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DVCP },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DVPP },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DV5P },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DVC  },


	// QuickTime video
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_8BPS   },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_QTRle  },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_QTRpza },

	// Screen recorder
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_CSCD },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_TSCC },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_TSCC2 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_VMnc },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_FLASHSV1 },
//	{ &MEDIATYPE_Video, &MEDIASUBTYPE_FLASHSV2 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_FPS1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MSS1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MSS2 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MSA1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MTS2 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_G2M2 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_G2M3 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_G2M4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_CRAM }, // CRAM - Microsoft Video 1

	// UtVideo
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_Ut_ULRG },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_Ut_ULRA },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_Ut_ULY0 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_Ut_ULY2 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_Ut_UQY2 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_Ut_UQRG },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_Ut_UQRA },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_Ut_ULH0 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_Ut_ULH2 },

	// DIRAC
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DRAC },

	// Lossless Video
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_HuffYUV  },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_Lagarith },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_FFVH     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_FFV1     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MAGICYUV },

	// Indeo 3/4/5
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_IV31 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_IV32 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_IV41 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_IV50 },

	// H264/AVC
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_H264     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_h264     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_X264     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_x264     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_VSSH     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_vssh     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DAVC     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_davc     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_PAVC     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_pavc     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_AVC1     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_avc1     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_H264_bis },

	// H264 MVC
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_AMVC     },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MVC1     },

	// SVQ3
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_SVQ3 },

	// SVQ1
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_SVQ1 },

	// H263
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_H263 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_h263 },

	{ &MEDIATYPE_Video, &MEDIASUBTYPE_S263 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_s263 },

	// Real video
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_RV10 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_RV20 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_RV30 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_RV40 },

	// Theora
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_THEORA },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_theora },

	// VC1
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_WVC1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_wvc1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_WMVA },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_WVP2 },

	// Apple ProRes
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_apch },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_apcn },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_apcs },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_apco },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_ap4h },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_icpf },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_icod },

	// Bink Video
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_BINKVI },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_BINKVB },

	// PNG
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_PNG },

	// Canopus
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_CLLC },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_CUVC },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_CHQX },

	// CineForm
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_CFHD },

	// HEVC
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_HEVC },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_HVC1 },
//	{ &MEDIATYPE_Video, &MEDIASUBTYPE_HM91 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_HM10 },
//	{ &MEDIATYPE_Video, &MEDIASUBTYPE_HM12 },

	// Avid DNxHD
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_AVdn },

	// Other MPEG-4
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MP4V },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_mp4v },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_M4S2 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_m4s2 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MP4S },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_mp4s },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_3IV1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_3iv1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_3IV2 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_3iv2 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_3IVX },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_3ivx },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_BLZ0 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_blz0 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DM4V },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_dm4v },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_FFDS },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_ffds },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_FVFW },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_fvfw },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DXGM },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_dxgm },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_FMP4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_fmp4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_HDX4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_hdx4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_LMP4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_lmp4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_NDIG },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_ndig },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_RMP4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_rmp4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_SMP4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_smp4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_SEDG },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_sedg },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_UMP4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_ump4 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_WV1F },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_wv1f },
};

const AMOVIESETUP_MEDIATYPE sudPinTypesInUncompressed[] = {
	// uncompressed video
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_v210 }, // YUV 4:2:2 10-bit
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_V410 }, // YUV 4:4:4 10-bit
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_r210 }, // RGB30
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_R10g }, // RGB30
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_R10k }, // RGB30

	{ &MEDIATYPE_Video, &MEDIASUBTYPE_Y800 }, // Y 8-bit (monochrome)
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_I420 }, // YUV 4:2:0 Planar
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_Y41B }, // YUV 4:1:1 Planar
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_Y42B }, // YUV 4:2:2 Planar
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_444P }, // YUV 4:4:4 Planar
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_cyuv }, // UYVY flipped vertically

	{ &MEDIATYPE_Video, &MEDIASUBTYPE_YVU9 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_IYUV },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_UYVY },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_YUY2 },

	{ &MEDIATYPE_Video, &MEDIASUBTYPE_NV12 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_YV12 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_YV16 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_YV24 },

	{ &MEDIATYPE_Video, &MEDIASUBTYPE_b64a },

	{ &MEDIATYPE_Video, &MEDIASUBTYPE_LAV_RAWVIDEO },
};

#pragma endregion any_constants

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] = {
	{&MEDIATYPE_Video, &MEDIASUBTYPE_NV12},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_YV12},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_YUY2},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_YV16},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_AYUV},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_YV24},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_P010},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_P210},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_Y410},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_P016},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_P216},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_Y416},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_RGB32},
};

VIDEO_OUTPUT_FORMATS DXVAFormats[]		= { // DXVA2 8bit
	{&MEDIASUBTYPE_NV12, 1, 12, FCC('dxva')},
};
VIDEO_OUTPUT_FORMATS DXVAFormats10bit[]	= { // DXVA2 10bit
	{&MEDIASUBTYPE_P010, 1, 24, FCC('dxva')},
};

#ifdef REGISTER_FILTER

#include "../../filters/ffmpeg_fix.cpp"

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesIn),  sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesOut), sudPinTypesOut}
};

const AMOVIESETUP_PIN sudpPinsUncompressed[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesInUncompressed),  sudPinTypesInUncompressed},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesOut), sudPinTypesOut}
};

CLSID Converter_clsID = GUIDFromCString(L"{0B7FA55E-FA38-4671-A2F2-B8F300C955C4}");

const AMOVIESETUP_FILTER sudFilters[] = {
	{&__uuidof(CMPCVideoDecFilter), MPCVideoDecName, MERIT_NORMAL + 1, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&Converter_clsID, MPCVideoConvName, MERIT_NORMAL + 1, _countof(sudpPinsUncompressed), sudpPinsUncompressed, CLSID_LegacyAmFilterCategory}
	// merit of video converter must be lower than merit of video renderers
};

CFactoryTemplate g_Templates[] = {
	{sudFilters[0].strName, sudFilters[0].clsID, CreateInstance<CMPCVideoDecFilter>, NULL, &sudFilters[0]},
	{sudFilters[1].strName, sudFilters[1].clsID, CreateInstance<CMPCVideoDecFilter>, NULL, &sudFilters[1]},
	{L"CMPCVideoDecPropertyPage", &__uuidof(CMPCVideoDecSettingsWnd), CreateInstance<CInternalPropertyPageTempl<CMPCVideoDecSettingsWnd> >},
	{L"CMPCVideoDecPropertyPage2", &__uuidof(CMPCVideoDecCodecWnd), CreateInstance<CInternalPropertyPageTempl<CMPCVideoDecCodecWnd> >},
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

BOOL CALLBACK EnumFindProcessWnd (HWND hwnd, LPARAM lParam)
{
	DWORD	procid = 0;
	WCHAR	WindowClass[40];
	GetWindowThreadProcessId(hwnd, &procid);
	GetClassName(hwnd, WindowClass, _countof(WindowClass));

	if (procid == GetCurrentProcessId() && wcscmp(WindowClass, _T(MPC_WND_CLASS_NAME)) == 0) {
		HWND* pWnd = (HWND*) lParam;
		*pWnd = hwnd;
		return FALSE;
	}
	return TRUE;
}

// CMPCVideoDecFilter

CMPCVideoDecFilter::CMPCVideoDecFilter(LPUNKNOWN lpunk, HRESULT* phr)
	: CBaseVideoFilter(NAME("MPC - Video decoder"), lpunk, phr, __uuidof(this))
	, m_nThreadNumber(0)
	, m_nDiscardMode(AVDISCARD_DEFAULT)
	, m_nDeinterlacing(AUTO)
	, m_nARMode(2)
	, m_nDXVACheckCompatibility(1)
	, m_nDXVA_SD(0)
	, m_nSwRGBLevels(0)
	, m_pAVCodec(NULL)
	, m_pAVCtx(NULL)
	, m_pFrame(NULL)
	, m_pParser(NULL)
	, m_nCodecNb(-1)
	, m_nCodecId(AV_CODEC_ID_NONE)
	, m_bCalculateStopTime(false)
	, m_bWaitKeyFrame(false)
	, m_DXVADecoderGUID(GUID_NULL)
	, m_nActiveCodecs(CODECS_ALL & ~CODEC_H264_MVC)
	, m_rtAvrTimePerFrame(0)
	, m_rtLastStart(INVALID_TIME)
	, m_rtLastStop(0)
	, m_rtStartCache(INVALID_TIME)
	, m_nWorkaroundBug(FF_BUG_AUTODETECT)
	, m_nErrorConcealment(FF_EC_DEBLOCK | FF_EC_GUESS_MVS)
	, m_bDXVACompatible(true)
	, m_nARX(0)
	, m_nARY(0)
	, m_bUseDXVA(true)
	, m_bUseFFmpeg(true)
	, m_pDXVADecoder(NULL)
	, m_pVideoOutputFormat(NULL)
	, m_nVideoOutputCount(0)
	, m_hDevice(INVALID_HANDLE_VALUE)
	, m_bWaitingForKeyFrame(TRUE)
	, m_bRVDropBFrameTimings(FALSE)
	, m_PixelFormat(AV_PIX_FMT_NONE)
	, m_bInterlaced(FALSE)
	, m_fSYNC(0)
	, m_bDecodingStart(FALSE)
	, m_bHighBitdepth(FALSE)
	, m_dRate(1.0)
	, m_pMSDKDecoder(NULL)
	, m_iMvcOutputMode(MVC_OUTPUT_Auto)
	, m_bMvcSwapLR(false)
	, m_MVC_Base_View_R_flag(FALSE)
{
	if (phr) {
		*phr = S_OK;
	}

	if (m_pOutput)	{
		delete m_pOutput;
	}
	m_pOutput = DNew CVideoDecOutputPin(NAME("CVideoDecOutputPin"), this, phr, L"Output");
	if (!m_pOutput) {
		*phr = E_OUTOFMEMORY;
		return;
	}

	for (int i = 0; i < PixFmt_count; i++) {
		if (i == PixFmt_AYUV) {
			m_fPixFmts[i] = false;
		} else {
			m_fPixFmts[i] = true;
		}
	}

	memset(&m_DDPixelFormat, 0, sizeof(m_DDPixelFormat));

	memset(&m_DXVAFilters, false, sizeof(m_DXVAFilters));
	memset(&m_VideoFilters, false, sizeof(m_VideoFilters));
	m_VideoFilters[VDEC_UNCOMPRESSED] = true;

#ifdef REGISTER_FILTER
	CRegKey key;
	ULONG len = 255;
	if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, OPT_REGKEY_VideoDec, KEY_READ)) {
		DWORD dw;
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_ThreadNumber, dw)) {
			m_nThreadNumber = dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_DiscardMode, dw)) {
			m_nDiscardMode = dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_Deinterlacing, dw)) {
			m_nDeinterlacing = (MPC_DEINTERLACING_FLAGS)dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_ARMode, dw)) {
			m_nARMode = dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_DXVACheck, dw)) {
			m_nDXVACheckCompatibility = dw;
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_DisableDXVA_SD, dw)) {
			m_nDXVA_SD = dw;
		}

		for (int i = 0; i < PixFmt_count; i++) {
			CString optname = OPT_SW_prefix;
			optname += GetSWOF(i)->name;
			if (ERROR_SUCCESS == key.QueryDWORDValue(optname, dw)) {
				m_fPixFmts[i] = !!dw;
			}
		}
		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_SwRGBLevels, dw)) {
			m_nSwRGBLevels = dw;
		}
	}
	if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, OPT_REGKEY_VCodecs, KEY_READ)) {
		m_nActiveCodecs = 0;
		for (size_t i = 0; i < _countof(vcodecs); i++) {
			DWORD dw = 1;
			key.QueryDWORDValue(vcodecs[i].opt_name, dw);
			if (dw) {
				m_nActiveCodecs |= vcodecs[i].flag;
			}
		}
	}
#else
	m_nThreadNumber				= AfxGetApp()->GetProfileInt(OPT_SECTION_VideoDec, OPT_ThreadNumber, m_nThreadNumber);
	m_nDiscardMode				= AfxGetApp()->GetProfileInt(OPT_SECTION_VideoDec, OPT_DiscardMode, m_nDiscardMode);
	m_nDeinterlacing			= (MPC_DEINTERLACING_FLAGS)AfxGetApp()->GetProfileInt(OPT_SECTION_VideoDec, OPT_Deinterlacing, m_nDeinterlacing);
	m_nARMode					= AfxGetApp()->GetProfileInt(OPT_SECTION_VideoDec, OPT_ARMode, m_nARMode);

	m_nDXVACheckCompatibility	= AfxGetApp()->GetProfileInt(OPT_SECTION_VideoDec, OPT_DXVACheck, m_nDXVACheckCompatibility);
	m_nDXVA_SD					= AfxGetApp()->GetProfileInt(OPT_SECTION_VideoDec, OPT_DisableDXVA_SD, m_nDXVA_SD);

	for (int i = 0; i < PixFmt_count; i++) {
		CString optname = OPT_SW_prefix;
		optname += GetSWOF(i)->name;
		m_fPixFmts[i] = !!AfxGetApp()->GetProfileInt(OPT_SECTION_VideoDec, optname, m_fPixFmts[i]);
	}
	m_nSwRGBLevels				= AfxGetApp()->GetProfileInt(OPT_SECTION_VideoDec, OPT_SwRGBLevels, m_nSwRGBLevels);
#endif

	m_nDXVACheckCompatibility = max(0, min(m_nDXVACheckCompatibility, 3));

	if (m_nDeinterlacing > PROGRESSIVE) {
		m_nDeinterlacing = AUTO;
	}
	if (m_nSwRGBLevels != 1) {
		m_nSwRGBLevels = 0;
	}

	avcodec_register_all();
	av_log_set_callback(ff_log);
	m_FormatConverter.SetOptions(m_nSwRGBLevels);

	HWND hWnd = NULL;
	EnumWindows(EnumFindProcessWnd, (LPARAM)&hWnd);
	DetectVideoCard(hWnd);

#ifdef _DEBUG
	// Check codec definition table
	size_t nCodecs		= _countof(ffCodecs);
	size_t nPinTypes	= _countof(sudPinTypesIn);
	size_t nPinTypesUncompressed = _countof(sudPinTypesInUncompressed);
	ASSERT(nCodecs == nPinTypes + nPinTypesUncompressed );
	for (size_t i = 0; i < nPinTypes; i++) {
		ASSERT(ffCodecs[i].clsMinorType == sudPinTypesIn[i].clsMinorType);
	}
	for (size_t i = 0; i < nPinTypesUncompressed ; i++) {
		ASSERT(ffCodecs[nPinTypes + i].clsMinorType == sudPinTypesInUncompressed[i].clsMinorType);
	}
#endif
}

CMPCVideoDecFilter::~CMPCVideoDecFilter()
{
	Cleanup();
}

void CMPCVideoDecFilter::DetectVideoCard(HWND hWnd)
{
	m_nPCIVendor         = 0;
	m_nPCIDevice         = 0;
	m_VideoDriverVersion = 0;

	CComPtr<IDirect3D9> pD3D9 = D3D9Helper::Direct3DCreate9();
	if (pD3D9) {
		D3DADAPTER_IDENTIFIER9 AdapID9 = {};
		if (pD3D9->GetAdapterIdentifier(D3D9Helper::GetAdapter(pD3D9, hWnd), 0, &AdapID9) == S_OK) {
			m_nPCIVendor         = AdapID9.VendorId;
			m_nPCIDevice         = AdapID9.DeviceId;
			m_VideoDriverVersion = AdapID9.DriverVersion.QuadPart;
			if (IsWin81orLater() && (m_VideoDriverVersion & 0xffff00000000) == 0 && (m_VideoDriverVersion & 0xffff) == 0) {
				// fix bug in GetAdapterIdentifier()
				m_VideoDriverVersion = (m_VideoDriverVersion & 0xffff000000000000) | ((m_VideoDriverVersion & 0xffff0000) << 16) | 0xffffffff;
			}
			m_strDeviceDescription.Format(L"%S (%04X:%04X)", AdapID9.Description, m_nPCIVendor, m_nPCIDevice);
		}
	}
}

REFERENCE_TIME CMPCVideoDecFilter::GetFrameDuration()
{
	REFERENCE_TIME AvgTimePerFrame = m_rtAvrTimePerFrame;
	if (m_nCodecId == AV_CODEC_ID_MPEG2VIDEO || m_nCodecId == AV_CODEC_ID_MPEG1VIDEO) {
		if (m_pAVCtx->time_base.den && m_pAVCtx->time_base.num) {
			AvgTimePerFrame = (UNITS * m_pAVCtx->time_base.num / m_pAVCtx->time_base.den) * m_pAVCtx->ticks_per_frame;
		}
	}

	return AvgTimePerFrame;
}

void CMPCVideoDecFilter::UpdateFrameTime(REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop)
{
	const REFERENCE_TIME AvgTimePerFrame = GetFrameDuration();

	if (rtStart == INVALID_TIME || (m_rtLastStart != INVALID_TIME && rtStart < m_rtLastStart)) {
		rtStart = m_rtLastStop;
		rtStop = INVALID_TIME;
	}

	if (m_bCalculateStopTime) {
		rtStop = INVALID_TIME;
	}

	if (rtStop == INVALID_TIME) {
		const REFERENCE_TIME rtFrameDuration = AvgTimePerFrame * (m_pFrame && m_pFrame->repeat_pict ? 3 : 2) / 2;
		rtStop = rtStart + (rtFrameDuration / m_dRate);
	}

	m_rtLastStart = rtStart;
	m_rtLastStop = rtStop;
}

void CMPCVideoDecFilter::GetFrameTimeStamp(AVFrame* pFrame, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop)
{
	rtStart = av_frame_get_best_effort_timestamp(pFrame);
	const int64_t pkt_duration = av_frame_get_pkt_duration(pFrame);
	if (pkt_duration) {
		rtStop = rtStart + pkt_duration;
	} else {
		rtStop = INVALID_TIME;
	}
}

#define CALC_HDR_VALUE(value) (value.num * (1.0 / value.den))
bool CMPCVideoDecFilter::AddFrameSideData(IMediaSample* pSample, AVFrame* pFrame)
{
	CheckPointer(pSample, false);
	CheckPointer(pFrame, false);

	CComPtr<IMediaSideData> pMediaSideData;
	if (SUCCEEDED(pSample->QueryInterface(&pMediaSideData))) {
		if (AVFrameSideData* sd = av_frame_get_side_data(pFrame, AV_FRAME_DATA_MASTERING_DISPLAY_METADATA)) {
			if (sd->size == sizeof(AVMasteringDisplayMetadata)) {
				AVMasteringDisplayMetadata* metadataHDR = (AVMasteringDisplayMetadata*)sd->data;
				MediaSideDataHDR hdr = { 0 };

				if (metadataHDR->has_primaries) {
					// export the display primaries in GBR order
					hdr.display_primaries_x[0] = CALC_HDR_VALUE(metadataHDR->display_primaries[1][0]);
					hdr.display_primaries_y[0] = CALC_HDR_VALUE(metadataHDR->display_primaries[1][1]);
					hdr.display_primaries_x[1] = CALC_HDR_VALUE(metadataHDR->display_primaries[2][0]);
					hdr.display_primaries_y[1] = CALC_HDR_VALUE(metadataHDR->display_primaries[2][1]);
					hdr.display_primaries_x[2] = CALC_HDR_VALUE(metadataHDR->display_primaries[0][0]);
					hdr.display_primaries_y[2] = CALC_HDR_VALUE(metadataHDR->display_primaries[0][1]);

					hdr.white_point_x = CALC_HDR_VALUE(metadataHDR->white_point[0]);
					hdr.white_point_y = CALC_HDR_VALUE(metadataHDR->white_point[1]);
				}

				if (metadataHDR->has_luminance) {
					hdr.max_display_mastering_luminance = CALC_HDR_VALUE(metadataHDR->max_luminance);
					hdr.min_display_mastering_luminance = CALC_HDR_VALUE(metadataHDR->min_luminance);
				}

				pMediaSideData->SetSideData(IID_MediaSideDataHDR, (const BYTE*)&hdr, sizeof(hdr));

				return true;
			} else {
				DLog(L"CMPCVideoDecFilter::AddFrameSideData(): Found HDR data of an unexpected size (%d)", sd->size);
			}
		} else if (m_baseFilterInfo.masterDataHDR) {
			pMediaSideData->SetSideData(IID_MediaSideDataHDR, (const BYTE*)m_baseFilterInfo.masterDataHDR, sizeof(MediaSideDataHDR));
			SAFE_DELETE(m_baseFilterInfo.masterDataHDR);

			return true;
		}
	}

	return false;
}

// some codecs can reset the values width/height on initialization
int CMPCVideoDecFilter::PictWidth()
{
	return m_pAVCtx->width ? m_pAVCtx->width : m_pAVCtx->coded_width;
}

int CMPCVideoDecFilter::PictHeight()
{
	return m_pAVCtx->height ? m_pAVCtx->height : m_pAVCtx->coded_height;
}

int CMPCVideoDecFilter::PictWidthAligned()
{
	return FFALIGN(m_pAVCtx->coded_width, m_nAlign);
}

int CMPCVideoDecFilter::PictHeightAligned()
{
	return FFALIGN(m_pAVCtx->coded_height, m_nAlign);
}

static bool IsFFMPEGEnabled(FFMPEG_CODECS ffcodec, const bool FFmpegFilters[VDEC_LAST])
{
	if (ffcodec.FFMPEGCode < 0 || ffcodec.FFMPEGCode >= VDEC_LAST) {
		return false;
	}

	return FFmpegFilters[ffcodec.FFMPEGCode];
}

static bool IsDXVAEnabled(FFMPEG_CODECS ffcodec, const bool DXVAFilters[VDEC_DXVA_LAST])
{
	if (ffcodec.DXVACode < 0 || ffcodec.DXVACode >= VDEC_DXVA_LAST) {
		return false;
	}

	return DXVAFilters[ffcodec.DXVACode];
}

int CMPCVideoDecFilter::FindCodec(const CMediaType* mtIn, BOOL bForced/* = FALSE*/)
{
	m_bUseFFmpeg	= false;
	m_bUseDXVA		= false;
	for (size_t i = 0; i < _countof(ffCodecs); i++)
		if (mtIn->subtype == *ffCodecs[i].clsMinorType) {
#ifndef REGISTER_FILTER
			m_bUseFFmpeg	= bForced || IsFFMPEGEnabled(ffCodecs[i], m_VideoFilters);
			m_bUseDXVA		= bForced || IsDXVAEnabled(ffCodecs[i], m_DXVAFilters);
			return ((m_bUseDXVA || m_bUseFFmpeg) ? i : -1);
#else
			bool bCodecActivated = false;
			m_bUseFFmpeg         = true;
			switch (ffCodecs[i].nFFCodec) {
				case AV_CODEC_ID_FLV1 :
				case AV_CODEC_ID_VP6F :
					bCodecActivated = (m_nActiveCodecs & CODEC_FLASH) != 0;
					break;
				case AV_CODEC_ID_MPEG4 :
					if ((*ffCodecs[i].clsMinorType == MEDIASUBTYPE_DX50) ||		// DivX
							(*ffCodecs[i].clsMinorType == MEDIASUBTYPE_dx50) ||
							(*ffCodecs[i].clsMinorType == MEDIASUBTYPE_DIVX) ||
							(*ffCodecs[i].clsMinorType == MEDIASUBTYPE_divx) ||
							(*ffCodecs[i].clsMinorType == MEDIASUBTYPE_Divx) ) {
						bCodecActivated = (m_nActiveCodecs & CODEC_DIVX) != 0;
					} else {
						bCodecActivated = (m_nActiveCodecs & CODEC_XVID) != 0;	// Xvid/MPEG-4
					}
					break;
				case AV_CODEC_ID_WMV1 :
				case AV_CODEC_ID_WMV2 :
				case AV_CODEC_ID_WMV3IMAGE :
					bCodecActivated = (m_nActiveCodecs & CODEC_WMV) != 0;
					break;
				case AV_CODEC_ID_WMV3 :
					m_bUseDXVA		= (m_nActiveCodecs & CODEC_WMV3_DXVA) != 0;
					m_bUseFFmpeg	= (m_nActiveCodecs & CODEC_WMV) != 0;
					bCodecActivated	= m_bUseDXVA || m_bUseFFmpeg;
					break;
				case AV_CODEC_ID_MSMPEG4V3 :
				case AV_CODEC_ID_MSMPEG4V2 :
				case AV_CODEC_ID_MSMPEG4V1 :
					bCodecActivated = (m_nActiveCodecs & CODEC_MSMPEG4) != 0;
					break;
				case AV_CODEC_ID_H264 :
					if ((*ffCodecs[i].clsMinorType == MEDIASUBTYPE_MVC1) ||
							(*ffCodecs[i].clsMinorType == MEDIASUBTYPE_AMVC)) {
						bCodecActivated = (m_nActiveCodecs & CODEC_H264_MVC) != 0;
					} else {
						m_bUseDXVA		= (m_nActiveCodecs & CODEC_H264_DXVA) != 0;
						m_bUseFFmpeg	= (m_nActiveCodecs & CODEC_H264) != 0;
						bCodecActivated	= m_bUseDXVA || m_bUseFFmpeg;
					}
					break;
				case AV_CODEC_ID_HEVC :
					m_bUseDXVA		= (m_nActiveCodecs & CODEC_HEVC_DXVA) != 0;
					m_bUseFFmpeg	= (m_nActiveCodecs & CODEC_HEVC) != 0;
					bCodecActivated	= m_bUseDXVA || m_bUseFFmpeg;
					break;
				case AV_CODEC_ID_SVQ3 :
				case AV_CODEC_ID_SVQ1 :
					bCodecActivated = (m_nActiveCodecs & CODEC_SVQ3) != 0;
					break;
				case AV_CODEC_ID_H263 :
					bCodecActivated = (m_nActiveCodecs & CODEC_H263) != 0;
					break;
				case AV_CODEC_ID_DIRAC  :
					bCodecActivated = (m_nActiveCodecs & CODEC_DIRAC) != 0;
					break;
				case AV_CODEC_ID_DVVIDEO  :
					bCodecActivated = (m_nActiveCodecs & CODEC_DV) != 0;
					break;
				case AV_CODEC_ID_THEORA :
					bCodecActivated = (m_nActiveCodecs & CODEC_THEORA) != 0;
					break;
				case AV_CODEC_ID_VC1 :
					m_bUseDXVA		= (m_nActiveCodecs & CODEC_VC1_DXVA) != 0;
					m_bUseFFmpeg	= (m_nActiveCodecs & CODEC_VC1) != 0;
					bCodecActivated	= m_bUseDXVA || m_bUseFFmpeg;
					break;
				case AV_CODEC_ID_VC1IMAGE :
					bCodecActivated = (m_nActiveCodecs & CODEC_VC1) != 0;
					break;
				case AV_CODEC_ID_AMV :
					bCodecActivated = (m_nActiveCodecs & CODEC_AMVV) != 0;
					break;
				case AV_CODEC_ID_LAGARITH :
					bCodecActivated = (m_nActiveCodecs & CODEC_LOSSLESS) != 0;
					break;
				case AV_CODEC_ID_VP3  :
				case AV_CODEC_ID_VP5  :
				case AV_CODEC_ID_VP6  :
				case AV_CODEC_ID_VP6A :
					bCodecActivated = (m_nActiveCodecs & CODEC_VP356) != 0;
					break;
				case AV_CODEC_ID_VP7  :
				case AV_CODEC_ID_VP8  :
					bCodecActivated = (m_nActiveCodecs & CODEC_VP89) != 0;
					break;
				case AV_CODEC_ID_VP9  :
					m_bUseDXVA		= (m_nActiveCodecs & CODEC_VP9_DXVA) != 0;
					m_bUseFFmpeg	= (m_nActiveCodecs & CODEC_VP89) != 0;
					bCodecActivated	= m_bUseDXVA || m_bUseFFmpeg;
					break;
				case AV_CODEC_ID_MJPEG  :
				case AV_CODEC_ID_MJPEGB :
					bCodecActivated = (m_nActiveCodecs & CODEC_MJPEG) != 0;
					break;
				case AV_CODEC_ID_INDEO3 :
				case AV_CODEC_ID_INDEO4 :
				case AV_CODEC_ID_INDEO5 :
					bCodecActivated = (m_nActiveCodecs & CODEC_INDEO) != 0;
					break;
				case AV_CODEC_ID_UTVIDEO :
					bCodecActivated = (m_nActiveCodecs & CODEC_UTVD) != 0;
					break;
				case AV_CODEC_ID_CSCD    :
				case AV_CODEC_ID_TSCC    :
				case AV_CODEC_ID_TSCC2   :
				case AV_CODEC_ID_VMNC    :
					bCodecActivated = (m_nActiveCodecs & CODEC_SCREC) != 0;
					break;
				case AV_CODEC_ID_RV10 :
				case AV_CODEC_ID_RV20 :
				case AV_CODEC_ID_RV30 :
				case AV_CODEC_ID_RV40 :
					bCodecActivated = (m_nActiveCodecs & CODEC_REALV) != 0;
					break;
				case AV_CODEC_ID_MPEG2VIDEO :
					m_bUseDXVA		= (m_nActiveCodecs & CODEC_MPEG2_DXVA) != 0;
					m_bUseFFmpeg	= (m_nActiveCodecs & CODEC_MPEG2) != 0;
					bCodecActivated	= m_bUseDXVA || m_bUseFFmpeg;
					break;
				case AV_CODEC_ID_MPEG1VIDEO :
					bCodecActivated = (m_nActiveCodecs & CODEC_MPEG1) != 0;
					break;
				case AV_CODEC_ID_PRORES :
					bCodecActivated = (m_nActiveCodecs & CODEC_PRORES) != 0;
					break;
				case AV_CODEC_ID_BINKVIDEO :
					bCodecActivated = (m_nActiveCodecs & CODEC_BINKV) != 0;
					break;
				case AV_CODEC_ID_PNG :
					bCodecActivated = (m_nActiveCodecs & CODEC_PNG) != 0;
					break;
				case AV_CODEC_ID_CLLC :
				case AV_CODEC_ID_HQ_HQA :
				case AV_CODEC_ID_HQX :
					bCodecActivated = (m_nActiveCodecs & CODEC_CANOPUS) != 0;
					break;
				case AV_CODEC_ID_CFHD :
					bCodecActivated = (m_nActiveCodecs & VDEC_CINEFORM) != 0;
					break;
				case AV_CODEC_ID_V210 :
				case AV_CODEC_ID_V410 :
				case AV_CODEC_ID_R210 :
				case AV_CODEC_ID_R10K :
				case AV_CODEC_ID_RAWVIDEO :
					bCodecActivated = (m_nActiveCodecs & CODEC_UNCOMPRESSED) != 0;
					break;
				case AV_CODEC_ID_DNXHD :
					bCodecActivated = (m_nActiveCodecs & CODEC_DNXHD) != 0;
					break;
				case AV_CODEC_ID_CINEPAK :
					bCodecActivated = (m_nActiveCodecs & CODEC_CINEPAK) != 0;
					break;
				case AV_CODEC_ID_8BPS  :
				case AV_CODEC_ID_QTRLE :
				case AV_CODEC_ID_RPZA  :
					bCodecActivated = (m_nActiveCodecs & CODEC_QT) != 0;
					break;
			}

			if (!bCodecActivated && !bForced) {
				m_bUseFFmpeg = false;
			}
			return ((bForced || bCodecActivated) ? i : -1);
#endif
		}

	return -1;
}

void CMPCVideoDecFilter::Cleanup()
{
	CAutoLock cAutoLock(&m_csReceive);

	ffmpegCleanup();

	SAFE_DELETE(m_pMSDKDecoder);
	SAFE_DELETE(m_pDXVADecoder);
	SAFE_DELETE_ARRAY(m_pVideoOutputFormat);

	if (m_hDevice != INVALID_HANDLE_VALUE) {
		m_pDeviceManager->CloseDeviceHandle(m_hDevice);
		m_hDevice = INVALID_HANDLE_VALUE;
	}

	m_pDeviceManager.Release();
	m_pDecoderService.Release();

	m_baseFilterInfo.Clear();
}

void CMPCVideoDecFilter::ffmpegCleanup()
{
	m_pAVCodec = NULL;

	av_parser_close(m_pParser);
	m_pParser = NULL;

	avcodec_free_context(&m_pAVCtx);

	av_frame_free(&m_pFrame);

	m_FormatConverter.Cleanup();

	m_nCodecNb	= -1;
	m_nCodecId	= AV_CODEC_ID_NONE;
}

STDMETHODIMP CMPCVideoDecFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IMPCVideoDecFilter)
		QI(ISpecifyPropertyPages)
		QI(ISpecifyPropertyPages2)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CMPCVideoDecFilter::CheckInputType(const CMediaType* mtIn)
{
	for (size_t i = 0; i < _countof(sudPinTypesIn); i++) {
		if ((mtIn->majortype == *sudPinTypesIn[i].clsMajorType) &&
				(mtIn->subtype == *sudPinTypesIn[i].clsMinorType)) {
			return S_OK;
		}
	}

	for (size_t i = 0; i < _countof(sudPinTypesInUncompressed); i++) {
		if ((mtIn->majortype == *sudPinTypesInUncompressed[i].clsMajorType) &&
				(mtIn->subtype == *sudPinTypesInUncompressed[i].clsMinorType)) {
			return S_OK;
		}
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CMPCVideoDecFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	return CheckInputType(mtIn); // TODO - add check output MediaType
}

bool CMPCVideoDecFilter::IsAVI()
{
	if (m_fSYNC == MAKEFOURCC('R','I','F','F')) {
		return true;
	} else if (m_fSYNC != 0) {
		return false;
	}

	CString fname;

	BeginEnumFilters(m_pGraph, pEF, pBF) {
		CComQIPtr<IFileSourceFilter> pFSF = pBF;
		if (pFSF) {
			LPOLESTR pFN = NULL;
			AM_MEDIA_TYPE mt;
			if (SUCCEEDED(pFSF->GetCurFile(&pFN, &mt)) && pFN && *pFN) {
				fname = CString(pFN);
				CoTaskMemFree(pFN);
			}
			break;
		}
	}
	EndEnumFilters

	if (!fname.IsEmpty() && ::PathFileExists(fname)) {
		CFile f;
		CFileException fileException;
		if (!f.Open(fname, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone, &fileException)) {
			DLog(L"CMPCVideoDecFilter::IsAVI() : Can't open file '%s', error = %u", fname, fileException.m_cause);
			return false;
		}

		if (f.Read(&m_fSYNC, sizeof(m_fSYNC)) != sizeof(m_fSYNC)) {
			DLog(L"CMPCVideoDecFilter::IsAVI() : Can't read SYNC from file '%s'", fname);
			return false;
		}

		if (m_fSYNC == MAKEFOURCC('R','I','F','F')) {
			DLog(L"CMPCVideoDecFilter::IsAVI() : '%s' is a valid AVI file", fname);
			return true;
		}
	}

	m_fSYNC = -1; // set non-zero value to avoid recheck of the file
	return false;
}

HRESULT CMPCVideoDecFilter::SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt)
{
	if (direction == PINDIR_INPUT) {
		HRESULT hr = InitDecoder(pmt);
		if (FAILED(hr)) {
			return hr;
		}

		if (m_nDecoderMode == MODE_DXVA2
				&& (m_pCurrentMediaType != *pmt)) {
			hr = ReinitDXVA2Decoder();
			if (FAILED(hr)) {
				return hr;
			}
		}

		m_bDecodingStart    = FALSE;
		m_pCurrentMediaType = *pmt;
	} else if (direction == PINDIR_OUTPUT) {
		BITMAPINFOHEADER bihOut;
		if (!ExtractBIH(&m_pOutput->CurrentMediaType(), &bihOut)) {
			return E_FAIL;
		}
		m_FormatConverter.UpdateOutput2(bihOut.biCompression, bihOut.biWidth, bihOut.biHeight);
	}

	return __super::SetMediaType(direction, pmt);
}

bool CMPCVideoDecFilter::IsDXVASupported()
{
	if (m_nCodecId != AV_CODEC_ID_NONE) {
		// Enabled by user ?
		if (m_bUseDXVA) {
			// is the file compatible ?
			if (m_bDXVACompatible) {
				// Does the codec suppport DXVA ?
				for (int i = 0; i < _countof(DXVAModes); i++) {
					if (m_nCodecId == DXVAModes[i].nCodecId) {
						return true;
					}
				}
			}
		}
	}

	return false;
}

HRESULT CMPCVideoDecFilter::FindDecoderConfiguration()
{
	DLog(L"CMPCVideoDecFilter::FindDecoderConfiguration(DXVA2)");

	HRESULT hr = E_FAIL;

	m_DXVADecoderGUID = GUID_NULL;

	if (m_pDecoderService) {
		UINT cDecoderGuids               = 0;
		GUID* pDecoderGuids              = NULL;
		GUID decoderGuid                 = GUID_NULL;
		BOOL bFoundDXVA2Configuration    = FALSE;
		DXVA2_ConfigPictureDecode config = { 0 };

		if (SUCCEEDED(hr = m_pDecoderService->GetDecoderDeviceGuids(&cDecoderGuids, &pDecoderGuids)) && cDecoderGuids) {

			std::vector<GUID> supportedDecoderGuids;
			DLog(L"    => Enumerating supported DXVA2 modes:");
			for (UINT iGuid = 0; iGuid < cDecoderGuids; iGuid++) {
				CString msg;
				msg.Format(L"        %s", GetGUIDString(pDecoderGuids[iGuid]));
				if (IsSupportedDecoderMode(pDecoderGuids[iGuid])) {
					msg.Append(L" - supported");
					supportedDecoderGuids.emplace_back(pDecoderGuids[iGuid]);
				}
				DLog(msg);
			}

			if (!supportedDecoderGuids.empty()) {
				for (auto guid = supportedDecoderGuids.begin(); guid != supportedDecoderGuids.end(); guid++) {
					DLog(L"    => Attempt : %s", GetGUIDString(*guid));

					// Find a configuration that we support.
					if (FAILED(hr = FindDXVA2DecoderConfiguration(m_pDecoderService, *guid, &config, &bFoundDXVA2Configuration))) {
						break;
					}

					if (bFoundDXVA2Configuration) {
						// Found a good configuration. Save the GUID.
						decoderGuid = *guid;
						DLog(L"    => Use : %s", GetGUIDString(decoderGuid));
						break;
					}
				}
			}
		}

		if (pDecoderGuids) {
			CoTaskMemFree(pDecoderGuids);
		}
		if (!bFoundDXVA2Configuration) {
			hr = E_FAIL; // Unable to find a configuration.
		}

		if (SUCCEEDED(hr)) {
			m_DXVA2Config     = config;
			m_DXVADecoderGUID = decoderGuid;
		}
	}

	return hr;
}

HRESULT CMPCVideoDecFilter::InitDecoder(const CMediaType *pmt)
{
	DLog(L"CMPCVideoDecFilter::InitDecoder()");

	const BOOL bReinit = (m_pAVCtx != NULL);
	const BOOL bChangeType = (m_pCurrentMediaType != *pmt);

	ffmpegCleanup();

	const int nNewCodec = FindCodec(pmt, bReinit);
	if (nNewCodec == -1) {
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	// Prevent connection to the video decoder - need to support decoding of uncompressed video (v210, V410, Y800, I420)
	CComPtr<IBaseFilter> pFilter = GetFilterFromPin(m_pInput->GetConnected());
	if (pFilter && IsVideoDecoder(pFilter, true)) {
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	m_nCodecNb = nNewCodec;

	if (pmt->subtype == MEDIASUBTYPE_AMVC || pmt->subtype == MEDIASUBTYPE_MVC1) {
		if (!m_pMSDKDecoder) {
			m_pMSDKDecoder = DNew CMSDKDecoder(this);
		}
		HRESULT hr = m_pMSDKDecoder->InitDecoder(pmt);

		if (hr != S_OK) {
			SAFE_DELETE(m_pMSDKDecoder);
		}

		if (m_pMSDKDecoder) {
			ExtractAvgTimePerFrame(&m_pInput->CurrentMediaType(), m_rtAvrTimePerFrame);
			int wout, hout;
			ExtractDim(&m_pInput->CurrentMediaType(), wout, hout, m_nARX, m_nARY);
			UNREFERENCED_PARAMETER(wout);
			UNREFERENCED_PARAMETER(hout);

			BuildOutputFormat();
			return S_OK;
		}

		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	m_nCodecId = ffCodecs[nNewCodec].nFFCodec;
	m_pAVCodec = avcodec_find_decoder(m_nCodecId);
	CheckPointer(m_pAVCodec, VFW_E_UNSUPPORTED_VIDEO);

	if (bChangeType) {
		const CLSID clsidInput = GetCLSID(m_pInput->GetConnected());
		const BOOL bNotTrustSourceTimeStamp = (clsidInput == GUIDFromCString(L"{A2E7EDBB-DCDD-4C32-A2A9-0CFBBE6154B4}") // Daum PotPlayer's MKV Source
											   || clsidInput == CLSID_WMAsfReader); // WM ASF Reader

		m_bCalculateStopTime = (m_nCodecId == AV_CODEC_ID_H264 ||
								m_nCodecId == AV_CODEC_ID_DIRAC ||
								(m_nCodecId == AV_CODEC_ID_MPEG4 && pmt->formattype == FORMAT_MPEG2Video)
								|| bNotTrustSourceTimeStamp);

		m_bRVDropBFrameTimings = (m_nCodecId == AV_CODEC_ID_RV10 || m_nCodecId == AV_CODEC_ID_RV20 || m_nCodecId == AV_CODEC_ID_RV30 || m_nCodecId == AV_CODEC_ID_RV40);
	}

	m_pAVCtx = avcodec_alloc_context3(m_pAVCodec);
	CheckPointer(m_pAVCtx, E_POINTER);

	if (m_nCodecId == AV_CODEC_ID_MPEG2VIDEO
			|| m_nCodecId == AV_CODEC_ID_MPEG1VIDEO
			|| pmt->subtype == MEDIASUBTYPE_H264
			|| pmt->subtype == MEDIASUBTYPE_h264
			|| pmt->subtype == MEDIASUBTYPE_X264
			|| pmt->subtype == MEDIASUBTYPE_x264
			|| pmt->subtype == MEDIASUBTYPE_H264_bis
			|| pmt->subtype == MEDIASUBTYPE_HEVC) {
		m_pParser = av_parser_init(m_nCodecId);
	}

	if (bReinit && m_nDecoderMode == MODE_SOFTWARE) {
		m_bUseDXVA = false;
	}

	SetThreadCount();

	m_pFrame = av_frame_alloc();
	CheckPointer(m_pFrame, E_POINTER);

	BITMAPINFOHEADER *pBMI = NULL;
	if (pmt->formattype == FORMAT_VideoInfo) {
		VIDEOINFOHEADER* vih	= (VIDEOINFOHEADER*)pmt->pbFormat;
		pBMI					= &vih->bmiHeader;
	} else if (pmt->formattype == FORMAT_VideoInfo2) {
		VIDEOINFOHEADER2* vih2	= (VIDEOINFOHEADER2*)pmt->pbFormat;
		pBMI					= &vih2->bmiHeader;
	} else if (pmt->formattype == FORMAT_MPEGVideo) {
		MPEG1VIDEOINFO* mpgv	= (MPEG1VIDEOINFO*)pmt->pbFormat;
		pBMI					= &mpgv->hdr.bmiHeader;
	} else if (pmt->formattype == FORMAT_MPEG2Video) {
		MPEG2VIDEOINFO* mpg2v	= (MPEG2VIDEOINFO*)pmt->pbFormat;
		pBMI					= &mpg2v->hdr.bmiHeader;
	} else {
		return VFW_E_INVALIDMEDIATYPE;
	}

	if (bChangeType) {
		m_bWaitKeyFrame =	m_nCodecId == AV_CODEC_ID_VC1
						 || m_nCodecId == AV_CODEC_ID_VC1IMAGE
						 || m_nCodecId == AV_CODEC_ID_WMV3
						 || m_nCodecId == AV_CODEC_ID_WMV3IMAGE
						 || m_nCodecId == AV_CODEC_ID_MPEG2VIDEO
						 || m_nCodecId == AV_CODEC_ID_RV30
						 || m_nCodecId == AV_CODEC_ID_RV40
						 || m_nCodecId == AV_CODEC_ID_VP3
						 || m_nCodecId == AV_CODEC_ID_THEORA
						 || m_nCodecId == AV_CODEC_ID_MPEG4
						 || m_nCodecId == AV_CODEC_ID_HEVC;
	}

	m_pAVCtx->codec_id              = m_nCodecId;
	m_pAVCtx->codec_tag             = pBMI->biCompression ? pBMI->biCompression : pmt->subtype.Data1;
	m_pAVCtx->coded_width           = pBMI->biWidth;
	m_pAVCtx->coded_height          = abs(pBMI->biHeight);
	m_pAVCtx->bits_per_coded_sample = pBMI->biBitCount;
	m_pAVCtx->workaround_bugs       = m_nWorkaroundBug;
	m_pAVCtx->error_concealment     = m_nErrorConcealment;
	m_pAVCtx->err_recognition       = AV_EF_CAREFUL;
	m_pAVCtx->idct_algo             = FF_IDCT_AUTO;
	m_pAVCtx->skip_loop_filter      = (AVDiscard)m_nDiscardMode;
	m_pAVCtx->refcounted_frames		= 1;
	m_pAVCtx->opaque				= this;
	m_pAVCtx->using_dxva			= IsDXVASupported();
	if (m_pAVCtx->codec_tag == MAKEFOURCC('m','p','g','2')) {
		m_pAVCtx->codec_tag = MAKEFOURCC('M','P','E','G');
	}

	if (m_pAVCtx->using_dxva) {
		m_pAVCtx->get_format		= av_get_format;
		m_pAVCtx->get_buffer2		= av_get_buffer;
	}

	AllocExtradata(m_pAVCtx, pmt);

	if (bChangeType) {
		ExtractAvgTimePerFrame(&m_pInput->CurrentMediaType(), m_rtAvrTimePerFrame);
		int wout, hout;
		ExtractDim(&m_pInput->CurrentMediaType(), wout, hout, m_nARX, m_nARY);
		UNREFERENCED_PARAMETER(wout);
		UNREFERENCED_PARAMETER(hout);
	}

	avcodec_lock;
	const int ret = avcodec_open2(m_pAVCtx, m_pAVCodec, NULL);
	avcodec_unlock;
	if (ret < 0) {
		return VFW_E_INVALIDMEDIATYPE;
	}

	FillAVCodecProps(m_pAVCtx);

	if (bChangeType && pFilter) {
		m_baseFilterInfo.Clear();

		CComPtr<IBaseFilterInfo> pBaseFilterInfo;
		if (SUCCEEDED(pFilter->QueryInterface(&pBaseFilterInfo))) {
			int value;
			if (SUCCEEDED(pBaseFilterInfo->GetInt("VIDEO_PROFILE", &value))) {
				m_baseFilterInfo.profile = value;
			}
			if (SUCCEEDED(pBaseFilterInfo->GetInt("VIDEO_PIXEL_FORMAT", &value))) {
				m_baseFilterInfo.pix_fmt = value;
			}

			size_t size = 0;
			LPVOID pData = NULL;
			if (SUCCEEDED(pBaseFilterInfo->GetBin("VIDEO_COLOR_SPACE", &pData, &size))) {
				if (size == sizeof(ColorSpace)) {
					m_baseFilterInfo.colorSpace = DNew ColorSpace;
					memcpy(m_baseFilterInfo.colorSpace, pData, sizeof(ColorSpace));
				}
				LocalFree(pData);
			}
			if (SUCCEEDED(pBaseFilterInfo->GetBin("HDR_MASTERING_METADATA", &pData, &size))) {
				if (size == sizeof(MediaSideDataHDR)) {
					m_baseFilterInfo.masterDataHDR = DNew MediaSideDataHDR;
					memcpy(m_baseFilterInfo.masterDataHDR, pData, sizeof(MediaSideDataHDR));
				}
				LocalFree(pData);
			}
		}
	}

	if (m_baseFilterInfo.profile != -1) {
		m_pAVCtx->profile = m_baseFilterInfo.profile;
	}
	if (m_baseFilterInfo.pix_fmt != AV_PIX_FMT_NONE) {
		m_pAVCtx->pix_fmt = (AVPixelFormat)m_baseFilterInfo.pix_fmt;
	}
	if (m_baseFilterInfo.colorSpace) {
		m_pAVCtx->colorspace             = (AVColorSpace)m_baseFilterInfo.colorSpace->MatrixCoefficients;
		m_pAVCtx->color_primaries        = (AVColorPrimaries)m_baseFilterInfo.colorSpace->Primaries;
		m_pAVCtx->color_range            = (AVColorRange)m_baseFilterInfo.colorSpace->Range;
		m_pAVCtx->color_trc              = (AVColorTransferCharacteristic)m_baseFilterInfo.colorSpace->TransferCharacteristics;
		m_pAVCtx->chroma_sample_location = (AVChromaLocation)m_baseFilterInfo.colorSpace->ChromaLocation;
	}

	m_PixelFormat = m_pAVCtx->pix_fmt;

	m_nAlign = 16;
	if (m_nCodecId == AV_CODEC_ID_MPEG2VIDEO) {
		m_nAlign <<= 1;
	} else if (m_nCodecId == AV_CODEC_ID_HEVC) {
		m_nAlign = 128;
	}

	m_nSurfaceWidth  = m_pAVCtx->coded_width;
	m_nSurfaceHeight = m_pAVCtx->coded_height;

	const int depth = GetLumaBits(m_pAVCtx->pix_fmt);
	m_bHighBitdepth = (depth == 10) && ((m_nCodecId == AV_CODEC_ID_HEVC && m_pAVCtx->profile == FF_PROFILE_HEVC_MAIN_10)
										|| (m_nCodecId == AV_CODEC_ID_VP9 && m_pAVCtx->profile == FF_PROFILE_VP9_2));

	m_dxvaExtFormat = GetDXVA2ExtendedFormat(m_pAVCtx, m_pFrame);

	if (bChangeType && IsDXVASupported()) {
		do {
			m_bDXVACompatible = false;

			if (!DXVACheckFramesize(m_nCodecId, PictWidth(), PictHeight(), m_nPCIVendor, m_nPCIDevice, m_VideoDriverVersion)) { // check frame size
				break;
			}

			if (m_nCodecId == AV_CODEC_ID_H264) {
				if (m_nDXVA_SD && PictWidthAligned() < 1280) { // check "Disable DXVA for SD" option
					break;
				}

				const int nCompat = FFH264CheckCompatibility(PictWidthAligned(), PictHeightAligned(), m_pAVCtx, m_nPCIVendor, m_nPCIDevice, m_VideoDriverVersion);

				if ((nCompat & DXVA_PROFILE_HIGHER_THAN_HIGH) || (nCompat & DXVA_HIGH_BIT)) { // DXVA unsupported
					break;
				}

				if (nCompat) {
					bool bDXVACompatible = true;
					switch (m_nDXVACheckCompatibility) {
						case 0:
							bDXVACompatible = false;
							break;
						case 1:
							bDXVACompatible = (nCompat == DXVA_UNSUPPORTED_LEVEL);
							break;
						case 2:
							bDXVACompatible = (nCompat == DXVA_TOO_MANY_REF_FRAMES);
							break;
					}
					if (!bDXVACompatible) {
						break;
					}
				}
			}
			else if (m_nCodecId == AV_CODEC_ID_MPEG2VIDEO) {
				if (!MPEG2CheckCompatibility(m_pAVCtx)) {
					break;
				}
			}
			else if (m_nCodecId == AV_CODEC_ID_WMV3 || m_nCodecId == AV_CODEC_ID_VC1) {
				if (m_pAVCtx->profile == FF_PROFILE_VC1_COMPLEX) {
					break;
				}
			}
			else if (m_nCodecId == AV_CODEC_ID_HEVC) {
				if (m_pAVCtx->profile > FF_PROFILE_HEVC_MAIN_10) {
					break;
				}

				// additional checks for older versions of x265
				const CString chroma = GetChromaSubsamplingStr(m_pAVCtx->pix_fmt);
				if (depth != 8 && depth != 10 || chroma != L"4:2:0") {
					break;
				}
			}
			else if (m_nCodecId == AV_CODEC_ID_VP9) {
				if (!(m_pAVCtx->profile == FF_PROFILE_VP9_0 || m_pAVCtx->profile == FF_PROFILE_VP9_2)) {
					break;
				}
			}

			m_bDXVACompatible = true;
		} while (false);

		if (!m_bDXVACompatible) {
			HRESULT hr = ReopenVideo();
			if FAILED(hr) {
				return hr;
			}
		}
	}

	av_frame_unref(m_pFrame);

	BuildOutputFormat();

	if (m_nDecoderMode == MODE_DXVA2 && m_pDXVADecoder && !m_pAVCtx->dxva_context) {
		m_pDXVADecoder->UpdateDXVA2Context();
	}

	return S_OK;
}

void CMPCVideoDecFilter::BuildOutputFormat()
{
	SAFE_DELETE_ARRAY(m_pVideoOutputFormat);

	// === New swscaler options
	int nSwIndex[PixFmt_count] = { 0 };
	int nSwCount = 0;

	const enum AVPixelFormat pix_fmt = m_pMSDKDecoder ? AV_PIX_FMT_NV12 : m_pAVCtx->pix_fmt;

	if (pix_fmt != AV_PIX_FMT_NONE) {
		const AVPixFmtDescriptor* av_pfdesc = av_pix_fmt_desc_get(pix_fmt);
		if (av_pfdesc) {
			int lumabits = av_pfdesc->comp->depth;

			const MPCPixelFormat* OutList = NULL;

			if (av_pfdesc->nb_components <= 2) { // greyscale formats with and without alfa channel
				if (lumabits <= 8) {
					OutList = YUV420_8;
				}
				else if (lumabits <= 10) {
					OutList = YUV420_10;
				}
				else {
					OutList = YUV420_16;
				}
			}
			else if (av_pfdesc->flags & (AV_PIX_FMT_FLAG_RGB | AV_PIX_FMT_FLAG_PAL)) {
				OutList = RGB_8;
			}
			else if (av_pfdesc->nb_components >= 3) {
				if (av_pfdesc->log2_chroma_w == 1 && av_pfdesc->log2_chroma_h == 1) { // 4:2:0
					if (lumabits <= 8) {
						OutList = YUV420_8;
					}
					else if (lumabits <= 10) {
						OutList = YUV420_10;
					}
					else {
						OutList = YUV420_16;
					}
				}
				else if (av_pfdesc->log2_chroma_w == 1 && av_pfdesc->log2_chroma_h == 0) { // 4:2:2
					if (lumabits <= 8) {
						OutList = YUV422_8;
					}
					else if (lumabits <= 10) {
						OutList = YUV422_10;
					}
					else {
						OutList = YUV422_16;
					}
				}
				else if (av_pfdesc->log2_chroma_w == 0 && av_pfdesc->log2_chroma_h == 0) { // 4:4:4
					if (lumabits <= 8) {
						OutList = YUV444_8;
					}
					else if (lumabits <= 10) {
						OutList = YUV444_10;
					}
					else {
						OutList = YUV444_16;
					}
				}
			}

			if (OutList == NULL) {
				OutList = YUV420_8;
			}

			// swap NV12 and YV12 for the Intel GPU.
			MPCPixelFormat IntelOutList[PixFmt_count];
			if (m_nPCIVendor == PCIV_Intel) {
				memcpy(IntelOutList, OutList, sizeof(IntelOutList));
				for (int i = 1; i < PixFmt_count; i++) {
					if (IntelOutList[i-1] == PixFmt_NV12 && IntelOutList[i] == PixFmt_YV12) {
						IntelOutList[i-1] = PixFmt_YV12;
						IntelOutList[i] = PixFmt_NV12;
						OutList = IntelOutList;
						break;
					}
				}
			}

			for (int i = 0; i < PixFmt_count; i++) {
				int index = OutList[i];
				if (m_fPixFmts[index]) {
					nSwIndex[nSwCount++] = index;
				}
			}
		}
	}

	if (!m_fPixFmts[PixFmt_YUY2] || nSwCount == 0) {
		// if YUY2 has not been added yet, then add it to the end of the list
		nSwIndex[nSwCount++] = PixFmt_YUY2;
	}

	m_nVideoOutputCount = m_bUseFFmpeg ? nSwCount : 0;
	if (IsDXVASupported()) {
		m_nVideoOutputCount += m_bHighBitdepth ? _countof(DXVAFormats10bit) : _countof(DXVAFormats);
	}

	m_pVideoOutputFormat = DNew VIDEO_OUTPUT_FORMATS[m_nVideoOutputCount];

	int nPos = 0;
	if (IsDXVASupported()) {
		if (m_bHighBitdepth) {
			memcpy(&m_pVideoOutputFormat[nPos], DXVAFormats10bit, sizeof(DXVAFormats10bit));
			nPos += _countof(DXVAFormats10bit);
		} else {
			memcpy(&m_pVideoOutputFormat[nPos], DXVAFormats, sizeof(DXVAFormats));
			nPos += _countof(DXVAFormats);
		}
	}

	// Software rendering
	if (m_bUseFFmpeg) {
		for (int i = 0; i < nSwCount; i++) {
			const SW_OUT_FMT* swof = GetSWOF(nSwIndex[i]);
			m_pVideoOutputFormat[nPos + i].subtype			= swof->subtype;
			m_pVideoOutputFormat[nPos + i].biCompression	= swof->biCompression;
			m_pVideoOutputFormat[nPos + i].biBitCount		= swof->bpp;
			m_pVideoOutputFormat[nPos + i].biPlanes			= 1; // This value must be set to 1.
		}
	}
}

void CMPCVideoDecFilter::GetOutputFormats(int& nNumber, VIDEO_OUTPUT_FORMATS** ppFormats)
{
	nNumber    = m_nVideoOutputCount;
	*ppFormats = m_pVideoOutputFormat;
}

void CMPCVideoDecFilter::AllocExtradata(AVCodecContext* pAVCtx, const CMediaType* pmt)
{
	// code from LAV ...
	// Process Extradata
	BYTE *extra = NULL;
	unsigned extralen = 0;
	getExtraData((const BYTE *)pmt->Format(), pmt->FormatType(), pmt->FormatLength(), NULL, &extralen);

	BOOL bH264avc = FALSE;
	if (pmt->formattype == FORMAT_MPEG2Video && (m_pAVCtx->codec_tag == MAKEFOURCC('a','v','c','1') || m_pAVCtx->codec_tag == MAKEFOURCC('A','V','C','1') || m_pAVCtx->codec_tag == MAKEFOURCC('C','C','V','1'))) {
		DLog(L"CMPCVideoDecFilter::AllocExtradata() : processing AVC1 extradata of %d bytes", extralen);
		// Reconstruct AVC1 extradata format
		MPEG2VIDEOINFO *mp2vi = (MPEG2VIDEOINFO *)pmt->Format();
		extralen += 7;
		extra = (uint8_t *)av_mallocz(extralen + AV_INPUT_BUFFER_PADDING_SIZE);
		extra[0] = 1;
		extra[1] = (BYTE)mp2vi->dwProfile;
		extra[2] = 0;
		extra[3] = (BYTE)mp2vi->dwLevel;
		extra[4] = (BYTE)(mp2vi->dwFlags ? mp2vi->dwFlags : 4) - 1;

		// only process extradata if available
		uint8_t ps_count = 0;
		if (extralen > 7) {
			// Actually copy the metadata into our new buffer
			unsigned actual_len;
			getExtraData((const BYTE *)pmt->Format(), pmt->FormatType(), pmt->FormatLength(), extra + 6, &actual_len);

			// Count the number of SPS/PPS in them and set the length
			// We'll put them all into one block and add a second block with 0 elements afterwards
			// The parsing logic does not care what type they are, it just expects 2 blocks.
			BYTE *p = extra + 6, *end = extra + 6 + actual_len;
			BOOL bSPS = FALSE, bPPS = FALSE;
			while (p + 1 < end) {
				unsigned len = (((unsigned)p[0] << 8) | p[1]) + 2;
				if (p + len > end) {
					break;
				}
				if ((p[2] & 0x1F) == 7)
					bSPS = TRUE;
				if ((p[2] & 0x1F) == 8)
					bPPS = TRUE;
				ps_count++;
				p += len;
			}
		}
		extra[5] = ps_count;
		extra[extralen - 1] = 0;

		bH264avc = TRUE;
	} else if (extralen > 0) {
		DLog(L"CMPCVideoDecFilter::AllocExtradata() : processing extradata of %d bytes", extralen);
		// Just copy extradata for other formats
		extra = (uint8_t *)av_mallocz(extralen + AV_INPUT_BUFFER_PADDING_SIZE);
		getExtraData((const BYTE *)pmt->Format(), pmt->FormatType(), pmt->FormatLength(), extra, NULL);

		if (m_nCodecId == AV_CODEC_ID_HEVC) {
			// try Reconstruct NAL units sequence into NAL Units in Byte-Stream Format
			BYTE* src		= extra;
			BYTE* src_end	= extra + extralen;
			BYTE* dst		= NULL;
			int dst_len		= 0;

			int NALCount	= 0;
			while (src + 2 < src_end) {
				int len = (src[0] << 8 | src[1]);
				if (len == 0) {
					break;
				}
				if ((len <= 2) || (src + len + 2 > src_end)) {
					NALCount = 0;
					break;
				}
				src += 2;
				int nat = (src[0] >> 1) & 0x3F;
				if (nat < NALU_TYPE_HEVC_VPS || nat > NALU_TYPE_HEVC_PPS) {
					NALCount = 0;
					break;
				}

				dst = (BYTE *)av_realloc_f(dst, dst_len + len + 3 + AV_INPUT_BUFFER_PADDING_SIZE, 1);
				// put startcode 0x000001
				dst[dst_len]		= 0x00;
				dst[dst_len + 1]	= 0x00;
				dst[dst_len + 2]	= 0x01;
				memcpy(dst + dst_len + 3, src, len);

				dst_len += len + 3;

				src += len;
				NALCount++;
			}

			if (NALCount > 1) {
				av_freep(&extra);
				extra		= dst;
				extralen	= dst_len;
			} else {
				av_freep(&dst);
			}
		}
	}

	// Hack to discard invalid MP4 metadata with AnnexB style video
	if (m_nCodecId == AV_CODEC_ID_H264 && !bH264avc && extra && extra[0] == 1) {
		av_freep(&extra);
		extralen = 0;
	}

	m_pAVCtx->extradata			= extra;
	m_pAVCtx->extradata_size	= (int)extralen;
}

#define DXVA2_MAX_SURFACES 64
HRESULT CMPCVideoDecFilter::CompleteConnect(PIN_DIRECTION direction, IPin* pReceivePin)
{
	if (direction == PINDIR_OUTPUT) {
		if (IsDXVASupported() && SUCCEEDED(ConfigureDXVA2(pReceivePin))) {
			HRESULT hr = E_FAIL;
			for (;;) {
				CComPtr<IDirectXVideoDecoderService> pDXVA2Service;
				hr = m_pDeviceManager->GetVideoService(m_hDevice, IID_PPV_ARGS(&pDXVA2Service));
				if (FAILED(hr)) {
					DLog(L"CMPCVideoDecFilter::CompleteConnect() : IDirect3DDeviceManager9::GetVideoService() - FAILED (0x%08x)", hr);
					break;
				}
				if (!pDXVA2Service) {
					break;
				}

				UINT numSurfaces = max(m_DXVA2Config.ConfigMinRenderTargetBuffCount, 1);
				LPDIRECT3DSURFACE9 pSurfaces[DXVA2_MAX_SURFACES] = { 0 };
				hr = pDXVA2Service->CreateSurface(
						PictWidthAligned(),
						PictHeightAligned(),
						numSurfaces,
						m_VideoDesc.Format,
						D3DPOOL_DEFAULT,
						0,
						DXVA2_VideoDecoderRenderTarget,
						pSurfaces,
						NULL);
				if (FAILED(hr)) {
					DLog(L"CMPCVideoDecFilter::CompleteConnect() : IDirectXVideoDecoderService::CreateSurface() - FAILED (0x%08x)", hr);
					break;
				}

				CComPtr<IDirectXVideoDecoder> pDirectXVideoDec;
				hr = m_pDecoderService->CreateVideoDecoder(m_DXVADecoderGUID, &m_VideoDesc, &m_DXVA2Config, pSurfaces, numSurfaces, &pDirectXVideoDec);
				if (FAILED(hr)) {
					DLog(L"CMPCVideoDecFilter::CompleteConnect() : IDirectXVideoDecoder::CreateVideoDecoder() - FAILED (0x%08x)", hr);
				}

				for (UINT i = 0; i < DXVA2_MAX_SURFACES; i++) {
					SAFE_RELEASE(pSurfaces[i]);
				}

				if (SUCCEEDED(hr) && SUCCEEDED(SetEVRForDXVA2(pReceivePin))) {
					m_nDecoderMode = MODE_DXVA2;
				}

				break;
			}

			if (FAILED(hr)) {
				m_DXVADecoderGUID = GUID_NULL;
				ZeroMemory(&m_DXVA2Config, sizeof(m_DXVA2Config));
			}
		}

		if (m_nDecoderMode == MODE_SOFTWARE) {
			if (!m_bUseFFmpeg) {
				return VFW_E_INVALIDMEDIATYPE;
			}

			if (IsDXVASupported()) {
				HRESULT hr;
				if (FAILED(hr = ReopenVideo())) {
					return hr;
				}

				ChangeOutputMediaFormat(2);
			}
		}

		DetectVideoCard_EVR(pReceivePin);

		if (m_pMSDKDecoder) {
			m_MVC_Base_View_R_flag = FALSE;
			BeginEnumFilters(m_pGraph, pEF, pBF) {
				if (CComQIPtr<IPropertyBag> pPB = pBF) {
					CComVariant var;
					if (SUCCEEDED(pPB->Read(L"STEREOSCOPIC3DMODE", &var, NULL)) && var.vt == VT_BSTR) {
						CString mode(var.bstrVal); mode.MakeLower();
						m_MVC_Base_View_R_flag = mode == L"mvc_rl";
						break;
					}
				}
			}
			EndEnumFilters;
		}
	}

	return __super::CompleteConnect (direction, pReceivePin);
}

HRESULT CMPCVideoDecFilter::DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties)
{
	if (UseDXVA2()) {
		HRESULT					hr;
		ALLOCATOR_PROPERTIES	Actual;

		if (m_pInput->IsConnected() == FALSE) {
			return E_UNEXPECTED;
		}

		pProperties->cBuffers = 24;

		if (FAILED(hr = pAllocator->SetProperties(pProperties, &Actual))) {
			return hr;
		}

		return pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer
			   ? E_FAIL
			   : NOERROR;
	} else {
		return __super::DecideBufferSize (pAllocator, pProperties);
	}
}

HRESULT CMPCVideoDecFilter::BeginFlush()
{
	return __super::BeginFlush();
}

HRESULT CMPCVideoDecFilter::EndFlush()
{
	CAutoLock cAutoLock(&m_csReceive);
	HRESULT hr =  __super::EndFlush();

	if (m_pAVCtx && avcodec_is_open(m_pAVCtx)) {
		avcodec_flush_buffers(m_pAVCtx);
	}

	return hr;
}

HRESULT CMPCVideoDecFilter::NewSegment(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double dRate)
{
	DLog(L"CMPCVideoDecFilter::NewSegment()");

	CAutoLock cAutoLock(&m_csReceive);

	if (m_pAVCtx) {
		avcodec_flush_buffers(m_pAVCtx);
	}

	if (m_pParser) {
		av_parser_close(m_pParser);
		m_pParser = av_parser_init(m_nCodecId);
	}

	if (m_pDXVADecoder) {
		m_pDXVADecoder->Flush();
	}

	if (m_pMSDKDecoder) {
		m_pMSDKDecoder->Flush();
	}

	m_dRate	= dRate;

	m_bWaitingForKeyFrame = TRUE;

	m_rtStartCache	= INVALID_TIME;

	m_rtLastStart	= INVALID_TIME;
	m_rtLastStop	= 0;

	if (m_bDecodingStart) {
		if (m_nCodecId == AV_CODEC_ID_H264 || m_nCodecId == AV_CODEC_ID_MPEG2VIDEO) {
			InitDecoder(&m_pInput->CurrentMediaType());
		}

		if (m_nDecoderMode == MODE_DXVA2
				&& (m_nCodecId == AV_CODEC_ID_H264 && m_nPCIVendor == PCIV_ATI && m_bInterlaced)) {
			HRESULT hr = ReinitDXVA2Decoder();
			if (FAILED(hr)) {
				return hr;
			}
		}
	}

	return __super::NewSegment(rtStart, rtStop, dRate);
}

HRESULT CMPCVideoDecFilter::EndOfStream()
{
	CAutoLock cAutoLock(&m_csReceive);
	m_pMSDKDecoder
		? m_pMSDKDecoder->EndOfStream()
		: Decode(NULL, 0, INVALID_TIME, INVALID_TIME);

	return __super::EndOfStream();
}

HRESULT CMPCVideoDecFilter::BreakConnect(PIN_DIRECTION dir)
{
	if (dir == PINDIR_INPUT) {
		Cleanup();
	}

	return __super::BreakConnect (dir);
}

void CMPCVideoDecFilter::SetTypeSpecificFlags(IMediaSample* pMS)
{
	if (CComQIPtr<IMediaSample2> pMS2 = pMS) {
		AM_SAMPLE2_PROPERTIES props;
		if (SUCCEEDED(pMS2->GetProperties(sizeof(props), (BYTE*)&props))) {
			props.dwTypeSpecificFlags &= ~0x7f;

			switch (m_nDeinterlacing) {
				case AUTO :
					if (!m_pFrame->interlaced_frame) {
						props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_WEAVE;
					}
					if (m_pFrame->top_field_first) {
						props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_FIELD1FIRST;
					}
					if (m_pFrame->repeat_pict) {
						props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_REPEAT_FIELD;
					}
					break;
				case PROGRESSIVE :
					props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_WEAVE;
					break;
				case TOPFIELD :
					props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_FIELD1FIRST;
					break;
			}

			switch (m_pFrame->pict_type) {
				case AV_PICTURE_TYPE_I :
				case AV_PICTURE_TYPE_SI :
					props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_I_SAMPLE;
					break;
				case AV_PICTURE_TYPE_P :
				case AV_PICTURE_TYPE_SP :
					props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_P_SAMPLE;
					break;
				default :
					props.dwTypeSpecificFlags |= AM_VIDEO_FLAG_B_SAMPLE;
					break;
			}

			pMS2->SetProperties(sizeof(props), (BYTE*)&props);
		}
	}

	m_bInterlaced = m_pFrame->interlaced_frame;
}

// from LAVVideo
DXVA2_ExtendedFormat CMPCVideoDecFilter::GetDXVA2ExtendedFormat(AVCodecContext *ctx, AVFrame *frame)
{
	DXVA2_ExtendedFormat fmt = { 0 };

	if (m_FormatConverter.GetOutPixFormat() == PixFmt_RGB32) {
		return fmt;
	}

	// Color Primaries
	switch(ctx->color_primaries) {
		case AVCOL_PRI_BT709:
			fmt.VideoPrimaries = DXVA2_VideoPrimaries_BT709;
			break;
		case AVCOL_PRI_BT470M:
			fmt.VideoPrimaries = DXVA2_VideoPrimaries_BT470_2_SysM;
			break;
		case AVCOL_PRI_BT470BG:
			fmt.VideoPrimaries = DXVA2_VideoPrimaries_BT470_2_SysBG;
			break;
		case AVCOL_PRI_SMPTE170M:
			fmt.VideoPrimaries = DXVA2_VideoPrimaries_SMPTE170M;
			break;
		case AVCOL_PRI_SMPTE240M:
			fmt.VideoPrimaries = DXVA2_VideoPrimaries_SMPTE240M;
			break;
		case AVCOL_PRI_BT2020:
			fmt.VideoPrimaries = 9;
			break;
	}

	// Color Space / Transfer Matrix
	switch (ctx->colorspace) {
		case AVCOL_SPC_BT709:
			fmt.VideoTransferMatrix = DXVA2_VideoTransferMatrix_BT709;
			break;
		case AVCOL_SPC_BT470BG:
		case AVCOL_SPC_SMPTE170M:
			fmt.VideoTransferMatrix = DXVA2_VideoTransferMatrix_BT601;
			break;
		case AVCOL_SPC_SMPTE240M:
			fmt.VideoTransferMatrix = DXVA2_VideoTransferMatrix_SMPTE240M;
			break;
		// Custom values, not official standard, but understood by madVR
		case AVCOL_SPC_BT2020_CL:
		case AVCOL_SPC_BT2020_NCL:
			fmt.VideoTransferMatrix = 4;
			break;
		case AVCOL_SPC_FCC:
			fmt.VideoTransferMatrix = 6;
			break;
		case AVCOL_SPC_YCGCO:
			fmt.VideoTransferMatrix = 7;
			break;
	}

	// Color Transfer Function
	switch(ctx->color_trc) {
		case AVCOL_TRC_BT709:
		case AVCOL_TRC_SMPTE170M:
		case AVCOL_TRC_BT2020_10:
		case AVCOL_TRC_BT2020_12:
			fmt.VideoTransferFunction = DXVA2_VideoTransFunc_709;
			break;
		case AVCOL_TRC_GAMMA22:
			fmt.VideoTransferFunction = DXVA2_VideoTransFunc_22;
			break;
		case AVCOL_TRC_GAMMA28:
			fmt.VideoTransferFunction = DXVA2_VideoTransFunc_28;
			break;
		case AVCOL_TRC_SMPTE240M:
			fmt.VideoTransferFunction = DXVA2_VideoTransFunc_240M;
			break;
		case AVCOL_TRC_LOG:
			fmt.VideoTransferFunction = MFVideoTransFunc_Log_100;
			break;
		case AVCOL_TRC_LOG_SQRT:
			fmt.VideoTransferFunction = MFVideoTransFunc_Log_316;
			break;
		case AVCOL_TRC_SMPTEST2084:
			// Custom values, not official standard, but understood by madVR
			fmt.VideoTransferFunction = 16;
			break;
		case AVCOL_TRC_ARIB_STD_B67:
			// Custom values, not official standard, but understood by EVR Custom
			fmt.VideoTransferFunction = 18;
			break;
	}

	if (frame->format == AV_PIX_FMT_XYZ12LE || frame->format == AV_PIX_FMT_XYZ12BE) {
		fmt.VideoPrimaries = DXVA2_VideoPrimaries_BT709;
	}

	// Chroma location
	switch(ctx->chroma_sample_location) {
		case AVCHROMA_LOC_LEFT:
			fmt.VideoChromaSubsampling = DXVA2_VideoChromaSubsampling_MPEG2;
			break;
		case AVCHROMA_LOC_CENTER:
			fmt.VideoChromaSubsampling = DXVA2_VideoChromaSubsampling_MPEG1;
			break;
		case AVCHROMA_LOC_TOPLEFT:
			fmt.VideoChromaSubsampling = DXVA2_VideoChromaSubsampling_DV_PAL;
			break;
	}

	// Color Range, 0-255 or 16-235
	if (ctx->color_range == AVCOL_RANGE_JPEG
			|| frame->format == AV_PIX_FMT_YUVJ420P
			|| frame->format == AV_PIX_FMT_YUVJ422P
			|| frame->format == AV_PIX_FMT_YUVJ444P
			|| frame->format == AV_PIX_FMT_YUVJ440P
			|| frame->format == AV_PIX_FMT_YUVJ411P) {
		fmt.NominalRange = DXVA2_NominalRange_0_255;
	} else {
		fmt.NominalRange = DXVA2_NominalRange_16_235;
	}

	// HACK: 1280 is the value when only chroma location is set to MPEG2, do not bother to send this information, as its the same for basically every clip
	if ((fmt.value & ~0xff) != 0 && (fmt.value & ~0xff) != 1280) {
		fmt.SampleFormat = AMCONTROL_USED | AMCONTROL_COLORINFO_PRESENT;
	} else {
		fmt.value = 0;
	}

	return fmt;
}

static inline BOOL GOPFound(BYTE *buf, int len)
{
	if (buf && len > 0) {
		CGolombBuffer gb(buf, len);
		BYTE state = 0x00;
		while (gb.NextMpegStartCode(state)) {
			if (state == 0xb8) { // GOP
				return TRUE;
			}
		}
	}

	return FALSE;
}

static inline HRESULT FillAVPacket(AVPacket *avpkt, const BYTE *buffer, int buflen)
{
	if (av_new_packet(avpkt, buflen) < 0) {
		return E_OUTOFMEMORY;
	}
	memcpy(avpkt->data, buffer, buflen);

	return S_OK;
}

#define Continue { av_frame_unref(m_pFrame); continue; }
HRESULT CMPCVideoDecFilter::DecodeInternal(AVPacket *avpkt, REFERENCE_TIME rtStartIn, REFERENCE_TIME rtStopIn, BOOL bPreroll/* = FALSE*/)
{
	if (avpkt) {
		if (m_bWaitingForKeyFrame) {
			if (m_nCodecId == AV_CODEC_ID_MPEG2VIDEO
					&& GOPFound(avpkt->data, avpkt->size)) {
				m_bWaitingForKeyFrame = FALSE;
			}

			if (m_nCodecId == AV_CODEC_ID_VP8 || m_nCodecId == AV_CODEC_ID_VP9) {
				const BOOL bKeyFrame = m_nCodecId == AV_CODEC_ID_VP8 ? !(avpkt->data[0] & 1) : !(avpkt->data[0] & 4);
				if (bKeyFrame) {
					DLog(L"CMPCVideoDecFilter::DecodeInternal(): Found VP8/9 key-frame, resuming decoding");
					m_bWaitingForKeyFrame = FALSE;
				} else {
					return S_OK;
				}
			}
		}
	}

	int ret = avcodec_send_packet(m_pAVCtx, avpkt);
	if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
		return E_FAIL;
	}

	HRESULT hr = S_OK;
	BOOL bDXVADeliverFrame = FALSE;
	for (;;) {
		ret = avcodec_receive_frame(m_pAVCtx, m_pFrame);
		if (ret < 0 && ret != AVERROR(EAGAIN)) {
			av_frame_unref(m_pFrame);
			return S_FALSE;
		}

		if (m_bWaitKeyFrame) {
			if (m_bWaitingForKeyFrame && (ret >= 0)) {
				if (m_pFrame->key_frame) {
					DLog(L"CMPCVideoDecFilter::DecodeInternal(): Found key-frame, resuming decoding");
					m_bWaitingForKeyFrame = FALSE;
				} else {
					ret = AVERROR(EAGAIN);
				}
			}
		}

		UpdateAspectRatio();

		if (m_nDecoderMode == MODE_DXVA2 && ((ret >= 0) || !bDXVADeliverFrame)) {
			hr = m_pDXVADecoder->DeliverFrame((ret >= 0), rtStartIn, rtStopIn);
			bDXVADeliverFrame = TRUE;
			Continue;
		}

		if (ret < 0) {
			av_frame_unref(m_pFrame);
			break;
		}

		GetFrameTimeStamp(m_pFrame, rtStartIn, rtStopIn);
		if (m_bRVDropBFrameTimings && m_pFrame->pict_type == AV_PICTURE_TYPE_B) {
			rtStartIn = m_rtLastStop;
			rtStopIn = INVALID_TIME;
		}

		UpdateFrameTime(rtStartIn, rtStopIn);

		if (bPreroll || rtStartIn < 0) {
			Continue;
		}

		CComPtr<IMediaSample> pOut;
		BYTE* pDataOut = NULL;
		DXVA2_ExtendedFormat dxvaExtFormat = GetDXVA2ExtendedFormat(m_pAVCtx, m_pFrame);

		if (FAILED(hr = GetDeliveryBuffer(m_pAVCtx->width, m_pAVCtx->height, &pOut, GetFrameDuration(), &dxvaExtFormat)) || FAILED(hr = pOut->GetPointer(&pDataOut))) {
			Continue;
		}

		// Check alignment on rawvideo, which can be off depending on the source file
		AVFrame* pTmpFrame = NULL;
		if (m_nCodecId == AV_CODEC_ID_RAWVIDEO) {
			for (size_t i = 0; i < 4; i++) {
				if ((intptr_t)m_pFrame->data[i] % 16u || m_pFrame->linesize[i] % 16u) {
					// copy the frame, its not aligned properly and would crash later
					pTmpFrame = av_frame_alloc();
					pTmpFrame->format      = m_pFrame->format;
					pTmpFrame->width       = m_pFrame->width;
					pTmpFrame->height      = m_pFrame->height;
					pTmpFrame->colorspace  = m_pFrame->colorspace;
					pTmpFrame->color_range = m_pFrame->color_range;
					av_frame_get_buffer(pTmpFrame, AV_INPUT_BUFFER_PADDING_SIZE);
					av_image_copy(pTmpFrame->data, pTmpFrame->linesize, (const uint8_t**)m_pFrame->data, m_pFrame->linesize, (AVPixelFormat)m_pFrame->format, m_pFrame->width, m_pFrame->height);
					break;
				}
			}
		}
		if (pTmpFrame) {
			m_FormatConverter.Converting(pDataOut, pTmpFrame);
			av_frame_free(&pTmpFrame);
		} else {
			m_FormatConverter.Converting(pDataOut, m_pFrame);
		}

		pOut->SetTime(&rtStartIn, &rtStopIn);
		pOut->SetMediaTime(NULL, NULL);
		SetTypeSpecificFlags(pOut);

		AddFrameSideData(pOut, m_pFrame);

		hr = m_pOutput->Deliver(pOut);

		av_frame_unref(m_pFrame);
	}

	return hr;
}

HRESULT CMPCVideoDecFilter::ParseInternal(const BYTE *buffer, int buflen, REFERENCE_TIME rtStartIn, REFERENCE_TIME rtStopIn, BOOL bPreroll)
{
	BOOL bFlush = (buffer == NULL);
	BYTE* pDataBuffer = (BYTE*)buffer;
	HRESULT hr = S_OK;

	while (buflen > 0 || bFlush) {
		REFERENCE_TIME rtStart = rtStartIn, rtStop = rtStopIn;

		BYTE *pOutBuffer = NULL;
		int pOutLen = 0;

		int used_bytes = av_parser_parse2(m_pParser, m_pAVCtx, &pOutBuffer, &pOutLen, pDataBuffer, buflen, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
		if (used_bytes == 0 && pOutLen == 0 && !bFlush) {
			DLog(L"CMPCVideoDecFilter::ParseInternal() - could not process buffer, starving?");
			break;
		} else if (used_bytes > 0) {
			buflen -= used_bytes;
			pDataBuffer += used_bytes;
		}

		// Update start time cache
		// If more data was read then output, update the cache (incomplete frame)
		// If output is bigger or equal, a frame was completed, update the actual rtStart with the cached value, and then overwrite the cache
		if (used_bytes > pOutLen) {
			if (rtStartIn != INVALID_TIME) {
				m_rtStartCache = rtStartIn;
			}
		/*
		} else if (used_bytes == pOutLen || ((used_bytes + 9) == pOutLen)) {
			// Why +9 above?
			// Well, apparently there are some broken MKV muxers that like to mux the MPEG-2 PICTURE_START_CODE block (which is 9 bytes) in the package with the previous frame
			// This would cause the frame timestamps to be delayed by one frame exactly, and cause timestamp reordering to go wrong.
			// So instead of failing on those samples, lets just assume that 9 bytes are that case exactly.
			m_rtStartCache = rtStartIn = INVALID_TIME;
		} else if (pOut_size > used_bytes) {
		*/
		} else {
			rtStart        = m_rtStartCache;
			m_rtStartCache = rtStartIn;
			// The value was used once, don't use it for multiple frames, that ends up in weird timings
			rtStartIn      = INVALID_TIME;
		}

		if (pOutLen > 0) {
			AVPacket *avpkt = av_packet_alloc();
			if (FAILED(hr = FillAVPacket(avpkt, pOutBuffer, pOutLen))) {
				break;
			}

			avpkt->pts = rtStart;

			hr = DecodeInternal(avpkt, rtStartIn, rtStopIn, bPreroll);

			av_packet_free(&avpkt);

			if (FAILED(hr)) {
				break;
			}
		} else if (bFlush) {
			hr = DecodeInternal(NULL, INVALID_TIME, INVALID_TIME);
			break;
		}
	}

	return hr;
}

HRESULT CMPCVideoDecFilter::Decode(const BYTE *buffer, int buflen, REFERENCE_TIME rtStartIn, REFERENCE_TIME rtStopIn, BOOL bSyncPoint/* = FALSE*/, BOOL bPreroll/* = FALSE*/)
{
	HRESULT hr = S_OK;

	if (m_pParser) {
		hr = ParseInternal(buffer, buflen, rtStartIn, rtStopIn, bPreroll);
	} else {
		if (!buffer) {
			return DecodeInternal(NULL, INVALID_TIME, INVALID_TIME);
		}

		AVPacket *avpkt = av_packet_alloc();
		if (FAILED(FillAVPacket(avpkt, buffer, buflen))) {
			return E_OUTOFMEMORY;
		}

		avpkt->pts  = rtStartIn;
		if (rtStartIn != INVALID_TIME && rtStopIn != INVALID_TIME) {
			avpkt->duration = rtStopIn - rtStartIn;
		}
		avpkt->flags = bSyncPoint ? AV_PKT_FLAG_KEY : 0;

		hr = DecodeInternal(avpkt, rtStartIn, rtStopIn, bPreroll);

		av_packet_free(&avpkt);
	}

	return hr;
}

// change colorspace details/output media format
// 1 - change swscaler colorspace details
// 2 - change output media format
HRESULT CMPCVideoDecFilter::ChangeOutputMediaFormat(int nType)
{
	HRESULT hr = S_OK;

	if (!m_pOutput || !m_pOutput->IsConnected()) {
		return hr;
	}

	// change swscaler colorspace details
	if (nType >= 1) {
		m_FormatConverter.SetOptions(m_nSwRGBLevels);
	}

	// change output media format
	if (nType == 2) {
		CAutoLock cObjectLock(m_pLock);
		BuildOutputFormat();

		IPin* pPin = m_pOutput->GetConnected();
		if (IsVideoRenderer(GetFilterFromPin(pPin))) {
			hr = NotifyEvent(EC_DISPLAY_CHANGED, (LONG_PTR)pPin, NULL);
			if (S_OK != hr) {
				hr = E_FAIL;
			}
		} else {
			int nNumber;
			VIDEO_OUTPUT_FORMATS* pFormats;
			GetOutputFormats(nNumber, &pFormats);
			for (int i = 0; i < nNumber * 2; i++) {
				CMediaType mt;
				if (SUCCEEDED(GetMediaType(i, &mt))) {
					hr = pPin->QueryAccept(&mt);
					if (hr == S_OK) {
						hr = ReconnectPin(pPin, &mt);
						if (hr == S_OK) {
							return hr;
						}
					}
				}
			}

			return E_FAIL;
		}
	}

	return hr;
}

// reopen video codec - reset the threads count and dxva flag
HRESULT CMPCVideoDecFilter::ReopenVideo()
{
	if (m_pAVCtx) {
		m_bUseDXVA = false;
		avcodec_close(m_pAVCtx);
		m_pAVCtx->using_dxva = false;
		m_pAVCtx->get_format = avcodec_default_get_format;
		m_pAVCtx->get_buffer2 = avcodec_default_get_buffer2;

		SetThreadCount();

		avcodec_lock;
		int ret = avcodec_open2(m_pAVCtx, m_pAVCodec, NULL);
		avcodec_unlock;
		if (ret < 0) {
			return VFW_E_INVALIDMEDIATYPE;
		}
	}

	return S_OK;
}

void CMPCVideoDecFilter::SetThreadCount()
{
	if (m_pAVCtx) {
		int nThreadNumber = m_nThreadNumber ? m_nThreadNumber : CPUInfo::GetProcessorNumber() * 3/2;
		m_pAVCtx->thread_count = max(1, min((IsDXVASupported() || m_nCodecId == AV_CODEC_ID_MPEG4) ? 1 : nThreadNumber, MAX_AUTO_THREADS));
	}
}

HRESULT CMPCVideoDecFilter::Transform(IMediaSample* pIn)
{
	HRESULT			hr;
	BYTE*			buffer;
	int				buflen;
	REFERENCE_TIME	rtStart	= INVALID_TIME;
	REFERENCE_TIME	rtStop	= INVALID_TIME;

	if (FAILED(hr = pIn->GetPointer(&buffer))) {
		return hr;
	}

	buflen = pIn->GetActualDataLength();
	if (buflen == 0) {
		return S_OK;
	}

	hr = pIn->GetTime(&rtStart, &rtStop);
	if (FAILED(hr)) {
		rtStart = rtStop = INVALID_TIME;
	} else if (hr == VFW_S_NO_STOP_TIME || rtStop - 1 <= rtStart) {
		rtStop = INVALID_TIME;
	}

	if (m_nDecoderMode == MODE_DXVA2) {
		CheckPointer(m_pDXVA2Allocator, E_UNEXPECTED);
	}

	hr = m_pMSDKDecoder
		? m_pMSDKDecoder->Decode(buffer, buflen, rtStart, rtStop)
		: Decode(buffer, buflen, rtStart, rtStop, pIn->IsSyncPoint() == S_OK, pIn->IsPreroll() == S_OK);

	m_bDecodingStart = TRUE;

	return S_OK;
}

void CMPCVideoDecFilter::UpdateAspectRatio()
{
	if (m_nARMode) {
		bool bSetAR = true;
		if (m_nARMode == 2) {
			CMediaType& mt = m_pInput->CurrentMediaType();
			if (mt.formattype == FORMAT_VideoInfo2 || mt.formattype == FORMAT_MPEG2_VIDEO || mt.formattype == FORMAT_DiracVideoInfo) {
				VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)mt.pbFormat;
				bSetAR = (!vih2->dwPictAspectRatioX && !vih2->dwPictAspectRatioY);
			}
			if (!bSetAR && (m_nARX && m_nARY)) {
				CSize aspect(m_nARX, m_nARY);
				ReduceDim(aspect);
				SetAspect(aspect);
			}
		}

		if (bSetAR) {
			if (m_pAVCtx && (m_pAVCtx->sample_aspect_ratio.num > 0) && (m_pAVCtx->sample_aspect_ratio.den > 0)) {
				CSize aspect(m_pAVCtx->sample_aspect_ratio.num * m_pAVCtx->width, m_pAVCtx->sample_aspect_ratio.den * m_pAVCtx->height);
				ReduceDim(aspect);
				SetAspect(aspect);
			}
		}
	} else if (m_nARX && m_nARY) {
		CSize aspect(m_nARX, m_nARY);
		ReduceDim(aspect);
		SetAspect(aspect);
	}
}

void CMPCVideoDecFilter::FlushDXVADecoder()	{
	if (m_pDXVADecoder) {
		CAutoLock cAutoLock(&m_csReceive);
		if (m_pAVCtx && avcodec_is_open(m_pAVCtx)) {
			avcodec_flush_buffers(m_pAVCtx);
		}
		m_pDXVADecoder->Flush();
	}
}

void CMPCVideoDecFilter::FillInVideoDescription(DXVA2_VideoDesc& videoDesc, D3DFORMAT Format/* = D3DFMT_A8R8G8B8*/)
{
	memset(&videoDesc, 0, sizeof(videoDesc));
	videoDesc.SampleWidth        = PictWidthAligned();
	videoDesc.SampleHeight       = PictHeightAligned();
	videoDesc.Format             = Format;
	videoDesc.UABProtectionLevel = 1;
}

BOOL CMPCVideoDecFilter::IsSupportedDecoderMode(const GUID& decoderGUID)
{
	if (IsDXVASupported()) {
		for (int i = 0; i < _countof(DXVAModes); i++) {
			if (DXVAModes[i].nCodecId == m_nCodecId
					&& DXVAModes[i].decoderGUID == decoderGUID
					&& DXVAModes[i].bHighBitdepth == !!m_bHighBitdepth) {
				return true;
			}
		}
	}

	return FALSE;
}

BOOL CMPCVideoDecFilter::IsSupportedDecoderConfig(const D3DFORMAT& nD3DFormat, const DXVA2_ConfigPictureDecode& config, bool& bIsPrefered)
{
	bIsPrefered = (config.ConfigBitstreamRaw == (m_nCodecId == AV_CODEC_ID_H264 ? 2 : 1));
	return (m_bHighBitdepth && nD3DFormat == MAKEFOURCC('P', '0', '1', '0')
			|| (!m_bHighBitdepth && (nD3DFormat == MAKEFOURCC('N', 'V', '1', '2') || nD3DFormat == MAKEFOURCC('I', 'M', 'C', '3'))));
}

HRESULT CMPCVideoDecFilter::FindDXVA2DecoderConfiguration(IDirectXVideoDecoderService *pDecoderService,
														  const GUID& guidDecoder,
														  DXVA2_ConfigPictureDecode *pSelectedConfig,
														  BOOL *pbFoundDXVA2Configuration)
{
	HRESULT hr = S_OK;
	UINT cFormats = 0;
	UINT cConfigurations = 0;
	bool bIsPrefered = false;

	D3DFORMAT                   *pFormats	= NULL;			// size = cFormats
	DXVA2_ConfigPictureDecode   *pConfig	= NULL;			// size = cConfigurations

	// Find the valid render target formats for this decoder GUID.
	hr = pDecoderService->GetDecoderRenderTargets(guidDecoder, &cFormats, &pFormats);

	if (SUCCEEDED(hr)) {
		// Look for a format that matches our output format.
		for (UINT iFormat = 0; iFormat < cFormats;  iFormat++) {

			// Fill in the video description. Set the width, height, format, and frame rate.
			DXVA2_VideoDesc VideoDesc;
			FillInVideoDescription(VideoDesc, pFormats[iFormat]);

			// Get the available configurations.
			hr = pDecoderService->GetDecoderConfigurations(guidDecoder, &VideoDesc, NULL, &cConfigurations, &pConfig);

			if (FAILED(hr)) {
				continue;
			}

			// Find a supported configuration.
			for (UINT iConfig = 0; iConfig < cConfigurations; iConfig++) {
				if (IsSupportedDecoderConfig(pFormats[iFormat], pConfig[iConfig], bIsPrefered)) {
					// This configuration is good.
					if (bIsPrefered || !*pbFoundDXVA2Configuration) {
						*pbFoundDXVA2Configuration	= TRUE;
						*pSelectedConfig			= pConfig[iConfig];

						FillInVideoDescription(m_VideoDesc, pFormats[iFormat]);
					}

					if (bIsPrefered) {
						break;
					}
				}
			}

			CoTaskMemFree(pConfig);
		} // End of formats loop.
	}

	CoTaskMemFree(pFormats);

	// Note: It is possible to return S_OK without finding a configuration.
	return hr;
}

HRESULT CMPCVideoDecFilter::ConfigureDXVA2(IPin *pPin)
{
	HRESULT hr = S_OK;

	CComPtr<IMFGetService>					pGetService;
	CComPtr<IDirect3DDeviceManager9>		pDeviceManager;
	CComPtr<IDirectXVideoDecoderService>	pDecoderService;
	HANDLE									hDevice = INVALID_HANDLE_VALUE;

	// Query the pin for IMFGetService.
	hr = pPin->QueryInterface(IID_PPV_ARGS(&pGetService));

	// Get the Direct3D device manager.
	if (SUCCEEDED(hr)) {
		hr = pGetService->GetService(MR_VIDEO_ACCELERATION_SERVICE, IID_PPV_ARGS(&pDeviceManager));
	}

	// Open a new device handle.
	if (SUCCEEDED(hr)) {
		hr = pDeviceManager->OpenDeviceHandle(&hDevice);
	}

	// Get the video decoder service.
	if (SUCCEEDED(hr)) {
		hr = pDeviceManager->GetVideoService(hDevice, IID_PPV_ARGS(&pDecoderService));
	}

	if (SUCCEEDED(hr)) {
		m_pDeviceManager	= pDeviceManager;
		m_pDecoderService	= pDecoderService;
		m_hDevice			= hDevice;
		hr					= FindDecoderConfiguration();
	}

	if (FAILED(hr)) {
		if (hDevice != INVALID_HANDLE_VALUE) {
			pDeviceManager->CloseDeviceHandle(hDevice);
		}
		m_pDeviceManager	= NULL;
		m_pDecoderService	= NULL;
		m_hDevice			= INVALID_HANDLE_VALUE;
	}

	return hr;
}

HRESULT CMPCVideoDecFilter::SetEVRForDXVA2(IPin *pPin)
{
    IMFGetService* pGetService;
    HRESULT hr = pPin->QueryInterface(IID_PPV_ARGS(&pGetService));
	if (SUCCEEDED(hr)) {
		IDirectXVideoMemoryConfiguration* pVideoConfig;
		hr = pGetService->GetService(MR_VIDEO_ACCELERATION_SERVICE, IID_PPV_ARGS(&pVideoConfig));
		if (SUCCEEDED(hr)) {
			// Notify the EVR.
			DXVA2_SurfaceType surfaceType;
			DWORD dwTypeIndex = 0;
			for (;;) {
				hr = pVideoConfig->GetAvailableSurfaceTypeByIndex(dwTypeIndex, &surfaceType);
				if (FAILED(hr)) {
					break;
				}
				if (surfaceType == DXVA2_SurfaceType_DecoderRenderTarget) {
					hr = pVideoConfig->SetSurfaceType(DXVA2_SurfaceType_DecoderRenderTarget);
					break;
				}
				++dwTypeIndex;
			}
			pVideoConfig->Release();
		}
		pGetService->Release();
    }
	return hr;
}

HRESULT CMPCVideoDecFilter::CreateDXVA2Decoder(UINT nNumRenderTargets, IDirect3DSurface9** pDecoderRenderTargets)
{
	DLog(L"CMPCVideoDecFilter::CreateDXVA2Decoder()");

	HRESULT							hr;
	CComPtr<IDirectXVideoDecoder>	pDirectXVideoDec;

	hr = m_pDecoderService->CreateVideoDecoder(m_DXVADecoderGUID, &m_VideoDesc, &m_DXVA2Config,
											   pDecoderRenderTargets, nNumRenderTargets, &pDirectXVideoDec);

	if (SUCCEEDED(hr)) {
		// need recreate dxva decoder after "stop" on Intel HD Graphics
		SAFE_DELETE(m_pDXVADecoder);
		m_pDXVADecoder = CDXVA2Decoder::CreateDXVA2Decoder(this, pDirectXVideoDec, &m_DXVADecoderGUID, &m_DXVA2Config);
		if (!m_pDXVADecoder) {
			hr = E_FAIL;
		}
	}

	if (FAILED(hr)) {
		m_DXVADecoderGUID = GUID_NULL;
		ZeroMemory(&m_DXVA2Config, sizeof(m_DXVA2Config));
	}

	return hr;
}

HRESULT CMPCVideoDecFilter::ReinitDXVA2Decoder()
{
	HRESULT hr = E_FAIL;

	SAFE_DELETE(m_pDXVADecoder);
	if (m_pDXVA2Allocator && IsDXVASupported() && SUCCEEDED(FindDecoderConfiguration())) {
		hr = RecommitAllocator();
	}

	return hr;
}

HRESULT CMPCVideoDecFilter::InitAllocator(IMemAllocator **ppAlloc)
{
	HRESULT hr = S_FALSE;
	m_pDXVA2Allocator = DNew CVideoDecDXVAAllocator(this, &hr);
	if (!m_pDXVA2Allocator) {
		return E_OUTOFMEMORY;
	}
	if (FAILED(hr)) {
		SAFE_DELETE(m_pDXVA2Allocator);
		return hr;
	}

	// Return the IMemAllocator interface.
	return m_pDXVA2Allocator->QueryInterface(IID_PPV_ARGS(ppAlloc));
}

HRESULT CMPCVideoDecFilter::RecommitAllocator()
{
	HRESULT hr = S_OK;

	if (m_pDXVA2Allocator) {
		// Re-Commit the allocator (creates surfaces and new decoder)
		hr = m_pDXVA2Allocator->Decommit();
		if (m_pDXVA2Allocator->DecommitInProgress()) {
			DLog(L"CMPCVideoDecFilter::RecommitAllocator() : WARNING! DXVA2 Allocator is still busy, trying to flush downstream");
			GetOutputPin()->GetConnected()->BeginFlush();
			GetOutputPin()->GetConnected()->EndFlush();
			if (m_pDXVA2Allocator->DecommitInProgress()) {
				DLog(L"CMPCVideoDecFilter::RecommitAllocator() : WARNING! Flush had no effect, decommit of the allocator still not complete");
			} else {
				DLog(L"CMPCVideoDecFilter::RecommitAllocator() : Flush was successfull, decommit completed!");
			}
		}
		hr = m_pDXVA2Allocator->Commit();
	}

	return hr;
}

// ISpecifyPropertyPages2

STDMETHODIMP CMPCVideoDecFilter::GetPages(CAUUID* pPages)
{
	CheckPointer(pPages, E_POINTER);

#ifdef REGISTER_FILTER
	pPages->cElems    = 2;
#else
	pPages->cElems    = 1;
#endif
	pPages->pElems    = (GUID*)CoTaskMemAlloc(sizeof(GUID) * pPages->cElems);
	pPages->pElems[0] = __uuidof(CMPCVideoDecSettingsWnd);
#ifdef REGISTER_FILTER
	pPages->pElems[1] = __uuidof(CMPCVideoDecCodecWnd);
#endif

	return S_OK;
}

STDMETHODIMP CMPCVideoDecFilter::CreatePage(const GUID& guid, IPropertyPage** ppPage)
{
	CheckPointer(ppPage, E_POINTER);

	if (*ppPage != NULL) {
		return E_INVALIDARG;
	}

	HRESULT hr;

	if (guid == __uuidof(CMPCVideoDecSettingsWnd)) {
		(*ppPage = DNew CInternalPropertyPageTempl<CMPCVideoDecSettingsWnd>(NULL, &hr))->AddRef();
	}
#ifdef REGISTER_FILTER
	else if (guid == __uuidof(CMPCVideoDecCodecWnd)) {
		(*ppPage = DNew CInternalPropertyPageTempl<CMPCVideoDecCodecWnd>(NULL, &hr))->AddRef();
	}
#endif

	return *ppPage ? S_OK : E_FAIL;
}

// EVR functions
HRESULT CMPCVideoDecFilter::DetectVideoCard_EVR(IPin *pPin)
{
	IMFGetService* pGetService;
	HRESULT hr = pPin->QueryInterface(IID_PPV_ARGS(&pGetService));
	if (SUCCEEDED(hr)) {
		// Try to get the adapter description of the active DirectX 9 device.
		IDirect3DDeviceManager9* pDevMan9;
		hr = pGetService->GetService(MR_VIDEO_ACCELERATION_SERVICE, IID_PPV_ARGS(&pDevMan9));
		if (SUCCEEDED(hr)) {
			HANDLE hDevice;
			hr = pDevMan9->OpenDeviceHandle(&hDevice);
			if (SUCCEEDED(hr)) {
				IDirect3DDevice9* pD3DDev9;
				hr = pDevMan9->LockDevice(hDevice, &pD3DDev9, TRUE);
				if (hr == DXVA2_E_NEW_VIDEO_DEVICE) {
					// Invalid device handle. Try to open a new device handle.
					hr = pDevMan9->CloseDeviceHandle(hDevice);
					if (SUCCEEDED(hr)) {
						hr = pDevMan9->OpenDeviceHandle(&hDevice);
						// Try to lock the device again.
						if (SUCCEEDED(hr)) {
							hr = pDevMan9->LockDevice(hDevice, &pD3DDev9, TRUE);
						}
					}
				}
				if (SUCCEEDED(hr)) {
					D3DDEVICE_CREATION_PARAMETERS DevPar9;
					hr = pD3DDev9->GetCreationParameters(&DevPar9);
					if (SUCCEEDED(hr)) {
						IDirect3D9* pD3D9;
						hr = pD3DDev9->GetDirect3D(&pD3D9);
						if (SUCCEEDED(hr)) {
							D3DADAPTER_IDENTIFIER9 AdapID9;
							hr = pD3D9->GetAdapterIdentifier(DevPar9.AdapterOrdinal, 0, &AdapID9);
							if (SUCCEEDED(hr)) {
								// copy adapter description
								m_nPCIVendor         = AdapID9.VendorId;
								m_nPCIDevice         = AdapID9.DeviceId;
								m_VideoDriverVersion = AdapID9.DriverVersion.QuadPart;
								if (IsWin81orLater() && (m_VideoDriverVersion & 0xffff00000000) == 0 && (m_VideoDriverVersion & 0xffff) == 0) {
									// fix bug in GetAdapterIdentifier()
									m_VideoDriverVersion = (m_VideoDriverVersion & 0xffff000000000000) | ((m_VideoDriverVersion & 0xffff0000) << 16) | 0xffffffff;
								}
								m_strDeviceDescription.Format(L"%S (%04X:%04X)", AdapID9.Description, m_nPCIVendor, m_nPCIDevice);
							}
						}
						pD3D9->Release();
					}
					pD3DDev9->Release();
					pDevMan9->UnlockDevice(hDevice, FALSE);
				}
				pDevMan9->CloseDeviceHandle(hDevice);
			}
			pDevMan9->Release();
		}
		pGetService->Release();
	}
	return hr;
}

HRESULT CMPCVideoDecFilter::SetFFMpegCodec(int nCodec, bool bEnabled)
{
	CAutoLock cAutoLock(&m_csProps);

	if (nCodec < 0 || nCodec >= VDEC_LAST) {
		return E_FAIL;
	}

	m_VideoFilters[nCodec] = bEnabled;
	return S_OK;
}

HRESULT CMPCVideoDecFilter::SetDXVACodec(int nCodec, bool bEnabled)
{
	CAutoLock cAutoLock(&m_csProps);

	if (nCodec < 0 || nCodec >= VDEC_DXVA_LAST) {
		return E_FAIL;
	}

	m_DXVAFilters[nCodec] = bEnabled;
	return S_OK;
}

// IFFmpegDecFilter
STDMETHODIMP CMPCVideoDecFilter::SaveSettings()
{
#ifdef REGISTER_FILTER
	CRegKey key;
	if (ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, OPT_REGKEY_VideoDec)) {
		key.SetDWORDValue(OPT_ThreadNumber, m_nThreadNumber);
		key.SetDWORDValue(OPT_DiscardMode, m_nDiscardMode);
		key.SetDWORDValue(OPT_Deinterlacing, (int)m_nDeinterlacing);
		key.SetDWORDValue(OPT_ARMode, m_nARMode);
		key.SetDWORDValue(OPT_DXVACheck, m_nDXVACheckCompatibility);
		key.SetDWORDValue(OPT_DisableDXVA_SD, m_nDXVA_SD);

		// === New swscaler options
		for (int i = 0; i < PixFmt_count; i++) {
			CString optname = OPT_SW_prefix;
			optname += GetSWOF(i)->name;
			key.SetDWORDValue(optname, m_fPixFmts[i]);
		}
		key.SetDWORDValue(OPT_SwRGBLevels, m_nSwRGBLevels);
		//
	}
	if (ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, OPT_REGKEY_VCodecs)) {
		for (size_t i = 0; i < _countof(vcodecs); i++) {
			DWORD dw = m_nActiveCodecs & vcodecs[i].flag ? 1 : 0;
			key.SetDWORDValue(vcodecs[i].opt_name, dw);
		}
	}
#else
	AfxGetApp()->WriteProfileInt(OPT_SECTION_VideoDec, OPT_ThreadNumber, m_nThreadNumber);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_VideoDec, OPT_DiscardMode, m_nDiscardMode);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_VideoDec, OPT_Deinterlacing, (int)m_nDeinterlacing);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_VideoDec, OPT_ARMode, m_nARMode);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_VideoDec, OPT_DXVACheck, m_nDXVACheckCompatibility);
	AfxGetApp()->WriteProfileInt(OPT_SECTION_VideoDec, OPT_DisableDXVA_SD, m_nDXVA_SD);

	// === New swscaler options
	for (int i = 0; i < PixFmt_count; i++) {
		CString optname = OPT_SW_prefix;
		optname += GetSWOF(i)->name;
		AfxGetApp()->WriteProfileInt(OPT_SECTION_VideoDec, optname, m_fPixFmts[i]);
	}
	AfxGetApp()->WriteProfileInt(OPT_SECTION_VideoDec, OPT_SwRGBLevels, m_nSwRGBLevels);
	//
#endif

	return S_OK;
}

// === IMPCVideoDecFilter

STDMETHODIMP CMPCVideoDecFilter::SetThreadNumber(int nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_nThreadNumber = nValue;
	return S_OK;
}

STDMETHODIMP_(int) CMPCVideoDecFilter::GetThreadNumber()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_nThreadNumber;
}

STDMETHODIMP CMPCVideoDecFilter::SetDiscardMode(int nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_nDiscardMode = nValue;
	return S_OK;
}

STDMETHODIMP_(int) CMPCVideoDecFilter::GetDiscardMode()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_nDiscardMode;
}

STDMETHODIMP CMPCVideoDecFilter::SetDeinterlacing(MPC_DEINTERLACING_FLAGS nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_nDeinterlacing = nValue;
	return S_OK;
}

STDMETHODIMP_(MPC_DEINTERLACING_FLAGS) CMPCVideoDecFilter::GetDeinterlacing()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_nDeinterlacing;
}

STDMETHODIMP_(GUID*) CMPCVideoDecFilter::GetDXVADecoderGuid()
{
	return m_pGraph && m_pDXVADecoder ? &m_DXVADecoderGUID : NULL;
}

STDMETHODIMP CMPCVideoDecFilter::SetActiveCodecs(ULONGLONG nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_nActiveCodecs = nValue;
	return S_OK;
}

STDMETHODIMP_(ULONGLONG) CMPCVideoDecFilter::GetActiveCodecs()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_nActiveCodecs;
}

STDMETHODIMP CMPCVideoDecFilter::SetARMode(int nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_nARMode = nValue;
	return S_OK;
}

STDMETHODIMP_(int) CMPCVideoDecFilter::GetARMode()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_nARMode;
}

STDMETHODIMP CMPCVideoDecFilter::SetDXVACheckCompatibility(int nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_nDXVACheckCompatibility = nValue;
	return S_OK;
}

STDMETHODIMP_(int) CMPCVideoDecFilter::GetDXVACheckCompatibility()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_nDXVACheckCompatibility;
}

STDMETHODIMP CMPCVideoDecFilter::SetDXVA_SD(int nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_nDXVA_SD = nValue;
	return S_OK;
}

STDMETHODIMP_(int) CMPCVideoDecFilter::GetDXVA_SD()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_nDXVA_SD;
}

// === New swscaler options
STDMETHODIMP CMPCVideoDecFilter::SetSwRefresh(int nValue)
{
	CAutoLock cAutoLock(&m_csProps);

	if (nValue &&
			((m_pAVCtx && m_nDecoderMode == MODE_SOFTWARE) || m_pMSDKDecoder)) {
		ChangeOutputMediaFormat(nValue);
	}
	return S_OK;
}

STDMETHODIMP CMPCVideoDecFilter::SetSwPixelFormat(MPCPixelFormat pf, bool enable)
{
	CAutoLock cAutoLock(&m_csProps);
	if (pf < 0 || pf >= PixFmt_count) {
		return E_INVALIDARG;
	}

	m_fPixFmts[pf] = enable;
	return S_OK;
}

STDMETHODIMP_(bool) CMPCVideoDecFilter::GetSwPixelFormat(MPCPixelFormat pf)
{
	CAutoLock cAutoLock(&m_csProps);

	if (pf < 0 || pf >= PixFmt_count) {
		return false;
	}

	return m_fPixFmts[pf];
}

STDMETHODIMP CMPCVideoDecFilter::SetSwRGBLevels(int nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_nSwRGBLevels = nValue;
	return S_OK;
}
STDMETHODIMP_(int) CMPCVideoDecFilter::GetSwRGBLevels()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_nSwRGBLevels;
}

STDMETHODIMP_(int) CMPCVideoDecFilter::GetColorSpaceConversion()
{
	CAutoLock cAutoLock(&m_csProps);

	if (!m_pAVCtx) {
		return -1; // no decoder
	}

	if (m_nDecoderMode != MODE_SOFTWARE || m_pAVCtx->pix_fmt == AV_PIX_FMT_NONE || m_FormatConverter.GetOutPixFormat() == PixFmt_None) {
		return -2; // no conversion
	}

	const AVPixFmtDescriptor* av_pfdesc = av_pix_fmt_desc_get(m_pAVCtx->pix_fmt);
	if (!av_pfdesc) {
		return -2;
	}
	bool in_rgb		= !!(av_pfdesc->flags & (AV_PIX_FMT_FLAG_RGB|AV_PIX_FMT_FLAG_PAL));
	bool out_rgb	= (m_FormatConverter.GetOutPixFormat() == PixFmt_RGB32);
	if (in_rgb < out_rgb) {
		return 1; // YUV->RGB conversion
	}
	if (in_rgb > out_rgb) {
		return 2; // RGB->YUV conversion
	}

	return 0; // YUV->YUV or RGB->RGB conversion
}

STDMETHODIMP CMPCVideoDecFilter::SetMvcOutputMode(int nMode, bool bSwapLR)
{
	CAutoLock cAutoLock(&m_csProps);
	if (nMode < 0 || nMode > MVC_OUTPUT_TopBottom) {
		return E_INVALIDARG;
	}
	m_iMvcOutputMode = nMode;
	m_bMvcSwapLR = bSwapLR;

	if (m_pMSDKDecoder) {
		m_pMSDKDecoder->SetOutputMode(nMode, bSwapLR);
	}

	return S_OK;
}

STDMETHODIMP_(int) CMPCVideoDecFilter::GetMvcActive()
{
	return m_pMSDKDecoder != NULL;
}

STDMETHODIMP_(CString) CMPCVideoDecFilter::GetInformation(MPCInfo index)
{
	CAutoLock cAutoLock(&m_csProps);
	CString infostr;

	switch (index) {
		case INFO_MPCVersion:
			infostr.Format(L"v%s (build %d)", _T(MPC_VERSION_STR), MPC_VERSION_REV);
			break;
		case INFO_InputFormat:
			if (m_pAVCtx) {
				infostr = m_pAVCtx->codec_descriptor->name;
				if (const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(m_pAVCtx->pix_fmt)) {
					if (desc->flags & AV_PIX_FMT_FLAG_PAL) {
						infostr.Append(L", palettized RGB");
					}
					else if (desc->nb_components == 1 || desc->nb_components == 2) {
						infostr.AppendFormat(L", Gray %d-bit", GetLumaBits(m_pAVCtx->pix_fmt));
					}
					else if(desc->flags & AV_PIX_FMT_FLAG_RGB) {
						int bidepth = 0;
						for (int i = 0; i < desc->nb_components; i++) {
							bidepth += desc->comp[i].depth;
						}
						infostr.Append(desc->flags & AV_PIX_FMT_FLAG_ALPHA ? L", RGBA" : L", RGB");
						infostr.AppendFormat(L" %dbpp", bidepth);
					}
					else if (desc->nb_components == 0) {
						// unknown
					} else {
						infostr.Append(desc->flags & AV_PIX_FMT_FLAG_ALPHA ? L", YUVA" : L", YUV");
						infostr.AppendFormat(L" %d-bit %s", GetLumaBits(m_pAVCtx->pix_fmt), GetChromaSubsamplingStr(m_pAVCtx->pix_fmt));
						if (desc->name && !strncmp(desc->name, "yuvj", 4)) {
							infostr.Append(L" full range");
						}
					}
				}
			} else if (m_pMSDKDecoder) {
				infostr = L"h264(MVC 3D), YUV 8-bit, 4:2:0";
			}
			break;
		case INFO_FrameSize:
			if (m_w && m_h) {
				LONG sarx = m_arx * m_h;
				LONG sary = m_ary * m_w;
				ReduceDim(sarx, sary);
				infostr.Format(L"%dx%d, SAR %ld:%ld, DAR %d:%d", m_w, m_h, sarx, sary, m_arx, m_ary);
			}
			break;
		case INFO_OutputFormat:
			if (GUID* DxvaGuid = GetDXVADecoderGuid()) {
				if (*DxvaGuid != GUID_NULL) {
					infostr.Format(L"DXVA (%s)", GetDXVAMode(DxvaGuid));
					break;
				}
			}
			if (const SW_OUT_FMT* swof = GetSWOF(m_FormatConverter.GetOutPixFormat())) {
				infostr.Format(L"%s (%d-bit %s)", swof->name, swof->luma_bits, GetChromaSubsamplingStr(swof->av_pix_fmt));
			}
			break;
		case INFO_GraphicsAdapter:
			infostr = m_strDeviceDescription;
			break;
	}

	return infostr;
}

#define H264_CHECK_PROFILE(profile) \
	(((profile) & ~FF_PROFILE_H264_CONSTRAINED) <= FF_PROFILE_H264_HIGH)

int CMPCVideoDecFilter::av_get_buffer(struct AVCodecContext *c, AVFrame *pic, int flags)
{
	CMPCVideoDecFilter* pFilter = static_cast<CMPCVideoDecFilter*>(c->opaque);
	if (pFilter->m_pDXVADecoder) {
		if ((c->codec_id == AV_CODEC_ID_H264 && !H264_CHECK_PROFILE(c->profile)) ||
			(c->codec_id == AV_CODEC_ID_HEVC && c->profile > FF_PROFILE_HEVC_MAIN_10) ||
			(c->codec_id == AV_CODEC_ID_VP9  && !(c->profile == FF_PROFILE_VP9_0 || c->profile == FF_PROFILE_VP9_2))) {
			return -1;
		}

		return pFilter->m_pDXVADecoder->get_buffer_dxva(pic);
	}

	return avcodec_default_get_buffer2(c, pic, flags);
}

enum AVPixelFormat CMPCVideoDecFilter::av_get_format(struct AVCodecContext *c, const enum AVPixelFormat * pix_fmts)
{
	CMPCVideoDecFilter* pFilter = static_cast<CMPCVideoDecFilter*>(c->opaque);
	if (pFilter->m_pDXVADecoder) {
		if (pFilter->m_nSurfaceWidth != c->coded_width
				|| pFilter->m_nSurfaceHeight != c->coded_height) {
			avcodec_flush_buffers(c);
			if (SUCCEEDED(pFilter->FindDecoderConfiguration())) {
				pFilter->RecommitAllocator();
			}

			pFilter->m_nSurfaceWidth  = c->coded_width;
			pFilter->m_nSurfaceHeight = c->coded_height;
		}
	}

	return avcodec_default_get_format(c, pix_fmts);
}

CVideoDecOutputPin::CVideoDecOutputPin(TCHAR* pObjectName, CBaseVideoFilter* pFilter, HRESULT* phr, LPCWSTR pName)
	: CBaseVideoOutputPin(pObjectName, pFilter, phr, pName)
	, m_pVideoDecFilter(static_cast<CMPCVideoDecFilter*>(pFilter))
{
}

CVideoDecOutputPin::~CVideoDecOutputPin()
{
}

HRESULT CVideoDecOutputPin::InitAllocator(IMemAllocator **ppAlloc)
{
	if (m_pVideoDecFilter && m_pVideoDecFilter->UseDXVA2()) {
		return m_pVideoDecFilter->InitAllocator(ppAlloc);
	}

	return __super::InitAllocator(ppAlloc);
}

namespace MPCVideoDec {
	void GetSupportedFormatList(FORMATS& fmts)
	{
		fmts.RemoveAll();

		for (size_t i = 0; i < _countof(sudPinTypesIn); i++) {
			FORMAT fmt = { sudPinTypesIn[i].clsMajorType, ffCodecs[i].clsMinorType, ffCodecs[i].FFMPEGCode, ffCodecs[i].DXVACode };
			fmts.AddTail(fmt);
		}
	}
} // namespace MPCVideoDec
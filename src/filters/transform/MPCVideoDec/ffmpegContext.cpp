/*
 * (C) 2006-2014 see Authors.txt
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
#include "../../../DSUtil/SysVersion.h"
#include "ffmpegContext.h"

extern "C" {
	#include <ffmpeg/libavcodec/avcodec.h>
// This is kind of an hack but it avoids using a C++ keyword as a struct member name
#define class classFFMPEG
	#include <ffmpeg/libavcodec/mpegvideo.h>
#undef class
// hack since "h264.h" is using "new" as a variable
#define new newFFMPEG
	#include <ffmpeg/libavcodec/h264.h>
#undef new
	#include <ffmpeg/libavcodec/hevc.h>
	#include <ffmpeg/libavcodec/vc1.h>
	#include <ffmpeg/libavcodec/mpeg12.h>
}

static const WORD PCID_NVIDIA_VP567[] = {
	// http://us.download.nvidia.com/XFree86/Linux-x86_64/346.35/README/supportedchips.html
	// http://pci-ids.ucw.cz/read/PC/10de
	// VP5, Nvidia VDPAU Feature Set D: GF117, GF119, GK104, GK106, GK107, GK110, GK208.
	0x0FC2, // GeForce GT 630 (GK107) (not officially supported or typo, 4k tested)
	0x0FC6, // GeForce GTX 650
	0x0FC8, // GeForce GT 740
	0x0FCD, // GeForce GT 755M
	0x0FD1, // GeForce GT 650M
	0x0FD2, // GeForce GT 640M
	0x0FD4, // GeForce GTX 660M
	0x0FD5, // GeForce GT 650M
	0x0FD8, // GeForce GT 640M
	0x0FD9, // GeForce GT 645M
	0x0FDF, // GeForce GT 740M
	0x0FE0, // GeForce GTX 660M
	0x0FE1, // GeForce GT 730M
	0x0FE2, // GeForce GT 745M
	0x0FE3, // GeForce GT 745M
	0x0FE4, // GeForce GT 750M
	0x0FE9, // GeForce GT 750M
	0x0FEA, // GeForce GT 755M
	0x0FEF, // GRID K340
	0x0FF2, // GRID K1
	0x0FF3, // Quadro K420
	0x0FF6, // Quadro K1100M
	0x0FF8, // Quadro K500M
	0x0FF9, // Quadro K2000D
	0x0FFA, // Quadro K600
	0x0FFB, // Quadro K2000M
	0x0FFC, // Quadro K1000M
	0x0FFD, // NVS 510
	0x0FFE, // Quadro K2000
	0x0FFF, // Quadro 410
	0x1001, // GeForce GTX TITAN Z
	0x1004, // GeForce GTX 780
	0x1005, // GeForce GTX TITAN
	0x1007, // GeForce GTX 780
	0x1008, // GeForce GTX 780 Ti
	0x100A, // GeForce GTX 780 Ti
	0x100C, // GeForce GTX TITAN Black
	0x1021, // Tesla K20Xm
	0x1022, // Tesla K20c
	0x1023, // Tesla K40m
	0x1024, // Tesla K40c
	0x1026, // Tesla K20s
	0x1027, // Tesla K40st
	0x1028, // Tesla K20m
	0x1029, // Tesla K40s
	0x103A, // Quadro K6000
	0x103C, // Quadro K5200
	0x1040, // GeForce GT 520 (GF119) (not officially supported or typo, 4k tested)
	0x1042, // GeForce 510
	0x1048, // GeForce 605
	0x104A, // GeForce GT 610 ((not officially supported or typo, fully tested)
	0x104B, // GeForce GT 625 (OEM)
	0x104C, // GeForce GT 705
	0x1050, // GeForce GT 520M (GF119) (not officially supported or typo)
	0x1051, // GeForce GT 520MX
	0x1052, // GeForce GT 520M (GF119) (not officially supported or typo)
	0x1054, // GeForce 410M
	0x1055, // GeForce 410M
	0x1056, // NVS 4200M
	0x1057, // NVS 4200M
	0x107C, // NVS 315
	0x107D, // NVS 310
	0x1180, // GeForce GTX 680
	0x1183, // GeForce GTX 660 Ti (fully tested)
	0x1184, // GeForce GTX 770
	0x1185, // GeForce GTX 660
	0x1187, // GeForce GTX 760
	0x1188, // GeForce GTX 690
	0x1189, // GeForce GTX 670
	0x118A, // GRID K520
	0x118E, // GeForce GTX 760 (192-bit)
	0x118F, // Tesla K10
	0x1193, // GeForce GTX 760 Ti OEM
	0x1194, // Tesla K8
	0x1195, // GeForce GTX 660
	0x1198, // GeForce GTX 880M
	0x1199, // GeForce GTX 870M
	0x119A, // GeForce GTX 860M
	0x119D, // GeForce GTX 775M
	0x119E, // GeForce GTX 780M
	0x119F, // GeForce GTX 780M
	0x11A0, // GeForce GTX 680M
	0x11A1, // GeForce GTX 670MX
	0x11A2, // GeForce GTX 675MX
	0x11A3, // GeForce GTX 680MX
	0x11A7, // GeForce GTX 675MX
	0x11B4, // Quadro K4200
	0x11B6, // Quadro K3100M
	0x11B7, // Quadro K4100M
	0x11B8, // Quadro K5100M
	0x11BA, // Quadro K5000
	0x11BC, // Quadro K5000M
	0x11BD, // Quadro K4000M
	0x11BE, // Quadro K3000M
	0x11BF, // GRID K2
	0x11C0, // GeForce GTX 660
	0x11C2, // GeForce GTX 650 Ti BOOST
	0x11C3, // GeForce GTX 650 Ti
	0x11C4, // GeForce GTX 645
	0x11C5, // GeForce GT 740
	0x11C6, // GeForce GTX 650 Ti
	0x11C8, // GeForce GTX 650
	0x11E0, // GeForce GTX 770M
	0x11E1, // GeForce GTX 765M
	0x11E2, // GeForce GTX 765M
	0x11E3, // GeForce GTX 760M
	0x11FA, // Quadro K4000
	0x11FC, // Quadro K2100M
	0x1280, // GeForce GT 635
	0x1281, // GeForce GT 710
	0x1282, // GeForce GT 640 rev. 2 (not officially supported or typo, fully tested)
	0x1284, // GeForce GT 630 rev. 2 (not officially supported or typo, fully tested)
	0x1286, // GeForce GT 720
	0x1288, // GeForce GT 720
	0x1290, // GeForce GT 730M
	0x1291, // GeForce GT 735M
	0x1292, // GeForce GT 740M
	0x1293, // GeForce GT 730M
	0x1295, // GeForce 710M
	0x1296, // GeForce 825M
	0x12B9, // Quadro K610M
	0x12BA, // Quadro K510M

	// VP6, Nvidia VDPAU Feature Set E: GM107, GM108, GM204.
	0x1340, // GeForce 830M
	0x1341, // GeForce 840M
	0x1380, // GeForce GTX 750 Ti
	0x1381, // GeForce GTX 750
	0x1382, // GeForce GTX 745
	0x1390, // GeForce 845M
	0x1391, // GeForce GTX 850M
	0x1392, // GeForce GTX 860M
	0x1393, // GeForce 840M
	0x13BA, // Quadro K2200
	0x13BB, // Quadro K620
	0x13C0, // GeForce GTX 980
	0x13C2, // GeForce GTX 970
	0x13D7, // GeForce GTX 980M
	0x13D8, // GeForce GTX 970M

	// VP7, Nvidia VDPAU Feature Set F: GM206.
	0x1401, // GeForce GTX 960
};

static const WORD PCID_ATI_UVD [] = {
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

static const WORD PCID_INTEL_4K [] = {
	// IvyBridge
	0x0152, // Intel HD Graphics 2500        (4k tested)
	0x0156, // Intel HD Graphics 2500 Mobile
	0x015A, // Intel HD Graphics P2500
	0x0162, // Intel HD Graphics 4000        (fully tested)
	0x0166, // Intel HD Graphics 4000 Mobile
	0x016A, // Intel HD Graphics P4000
	// Haswell
	0x0402, // Intel HD Graphics
	0x0406, // Intel HD Graphics Mobile
	0x0412, // Intel HD Graphics 4600
	0x0416, // Intel HD Graphics 4600 Mobile
	0x041A, // Intel HD Graphics P4600/P4700
	0x041E, // Intel HD Graphics 4400"
	0x0A06, // Intel HD Graphics
	0x0A0E, // Intel HD Graphics
	0x0A16, // Intel HD Graphics Family
	0x0A1E, // Intel HD Graphics Family
	0x0A26, // Intel HD Graphics 5000
	0x0A2E, // Intel Iris Graphics 5100
	0x0D22, // Intel Iris Graphics 5200
	0x0D26, // Intel Iris Graphics 5200
	// Atom Z3700 Series
	0x0F31, // Intel HD Graphics
	// Broadwell
	0x1606, // Intel HD Graphics
	0x1616, // Intel HD Graphics 5500
	0x160E, // Intel HD Graphics
	0x161E, // Intel HD Graphics 5300
	0x1626, // Intel HD Graphics 6000
	0x162B, // Intel HD Graphics 6100
};

static bool CheckPCID(DWORD pcid, const WORD* pPCIDs, size_t count)
{
	WORD wPCID = (WORD)pcid;
	for (size_t i = 0; i < count; i++) {
		if (wPCID == pPCIDs[i]) {
			return true;
		}
	}

	return false;
}

bool IsATIUVD(DWORD nPCIVendor, DWORD nPCIDevice)
{
	return (nPCIVendor == PCIV_ATI && CheckPCID(nPCIDevice, PCID_ATI_UVD, _countof(PCID_ATI_UVD)));
}

// === H264 functions
// returns TRUE if version is equal to or higher than A.B.C.D, returns FALSE otherwise
static BOOL DriverVersionCheck(LARGE_INTEGER VideoDriverVersion, int A, int B, int C, int D)
{
	if (HIWORD(VideoDriverVersion.HighPart) > A) {
		return TRUE;
	} else if (HIWORD(VideoDriverVersion.HighPart) == A) {
		if (LOWORD(VideoDriverVersion.HighPart) > B) {
			return TRUE;
		} else if (LOWORD(VideoDriverVersion.HighPart) == B) {
			if (HIWORD(VideoDriverVersion.LowPart) > C) {
				return TRUE;
			} else if (HIWORD(VideoDriverVersion.LowPart) == C) {
				if (LOWORD(VideoDriverVersion.LowPart) >= D) {
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

static unsigned __int64 GetFileVersion(LPCTSTR lptstrFilename)
{
	unsigned __int64 ret = 0;

	DWORD buff[4] = { 0 };
	VS_FIXEDFILEINFO* pvsf = (VS_FIXEDFILEINFO*)buff;
	DWORD len = GetFileVersionInfoSize((LPTSTR)lptstrFilename, NULL);

	if (len) {
		TCHAR* b1 = new TCHAR[len];
		if (b1) {
			UINT uLen;
			if (GetFileVersionInfo((LPTSTR)lptstrFilename, 0, len, b1) && VerQueryValue(b1, L"\\", (void**)&pvsf, &uLen)) {
				ret = ((unsigned __int64)pvsf->dwFileVersionMS << 32) | pvsf->dwFileVersionLS;
			}

			delete [] b1;
		}
	}

	return ret;
}

int FFH264CheckCompatibility(int nWidth, int nHeight, struct AVCodecContext* pAVCtx,
							 DWORD nPCIVendor, DWORD nPCIDevice, LARGE_INTEGER VideoDriverVersion, bool nIsAtiDXVACompatible)
{
	H264Context* h				= (H264Context*)pAVCtx->priv_data;
	SPS* cur_sps				= &h->sps;

	int video_is_level51		= 0;
	int no_level51_support		= 1;
	int too_much_ref_frames		= 0;
	int max_ref_frames_dpb41	= min(11, 8388608/(nWidth * nHeight) );

	if (cur_sps != NULL) {
		if (cur_sps->bit_depth_luma > 8 || cur_sps->chroma_format_idc > 1) {
			return DXVA_HIGH_BIT;
		}

		if (cur_sps->profile_idc > 100) {
			return DXVA_PROFILE_HIGHER_THAN_HIGH;
		}

		video_is_level51			= cur_sps->level_idc >= 51 ? 1 : 0;
		int max_ref_frames			= max_ref_frames_dpb41; // default value is calculate

		if (nPCIVendor == PCIV_nVidia) {
			// nVidia cards support level 5.1 since drivers v6.14.11.7800 for XP and drivers v7.15.11.7800 for Vista/7
			if (IsWinVistaOrLater()) {
				if (DriverVersionCheck(VideoDriverVersion, 7, 15, 11, 7800)) {
					no_level51_support = 0;

					// max ref frames is 16 for HD and 11 otherwise
					// max_ref_frames = (nWidth >= 1280) ? 16 : 11;
					max_ref_frames = 16;
				}
			} else {
				if (DriverVersionCheck(VideoDriverVersion, 6, 14, 11, 7800)) {
					no_level51_support = 0;

					// max ref frames is 14
					max_ref_frames = 14;
				}
			}
		} else if (nPCIVendor == PCIV_S3_Graphics || nPCIVendor == PCIV_Intel) {
			no_level51_support = 0;
			max_ref_frames = 16;
		} else if (nPCIVendor == PCIV_ATI && nIsAtiDXVACompatible) {
			TCHAR path[MAX_PATH] = { 0 };
			GetSystemDirectory(path, MAX_PATH);
			wcscat(path, L"\\drivers\\atikmdag.sys\0");
			unsigned __int64 f_version = GetFileVersion(path);

			if (f_version) {
				LARGE_INTEGER VideoDriverVersion;
				VideoDriverVersion.QuadPart = f_version;

				if (IsWinVistaOrLater()) {
					// file version 8.1.1.1016 - Catalyst 10.4, WinVista & Win7
					if (DriverVersionCheck(VideoDriverVersion, 8, 1, 1, 1016)) {
						no_level51_support = 0;
						max_ref_frames = 16;
					}
				} else {
					// TODO - need file version for Catalyst 10.4 under WinXP
				}
			} else {
				// driver version 8.14.1.6105 - Catalyst 10.4; TODO - verify this information
				if (DriverVersionCheck(VideoDriverVersion, 8, 14, 1, 6105)) {
					no_level51_support = 0;
					max_ref_frames = 16;
				}
			}
		}

		// Check maximum allowed number reference frames
		if (cur_sps->ref_frame_count > max_ref_frames) {
			too_much_ref_frames = 1;
		}
	}

	int Flags = 0;
	if (video_is_level51 * no_level51_support) {
		Flags |= DXVA_UNSUPPORTED_LEVEL;
	}
	if (too_much_ref_frames) {
		Flags |= DXVA_TOO_MANY_REF_FRAMES;
	}

	return Flags;
}

void FFH264SetDxvaParams(struct AVCodecContext* pAVCtx, void* pDXVA_Context)
{
	H264Context* h		= (H264Context*)pAVCtx->priv_data;
	h->dxva_context		= pDXVA_Context;
}

// === VC1 functions
void FFVC1SetDxvaParams(struct AVCodecContext* pAVCtx, void* pDXVA_Context)
{
	VC1Context* vc1		= (VC1Context*)pAVCtx->priv_data;
	vc1->dxva_context	= pDXVA_Context;
}

// === Mpeg2 functions
int	MPEG2CheckCompatibility(struct AVCodecContext* pAVCtx)
{
	MpegEncContext*	s = (MpegEncContext*)pAVCtx->priv_data;

	return (s->chroma_format < 2);
}

void FFMPEG2SetDxvaParams(struct AVCodecContext* pAVCtx, void* pDXVA_Context)
{
	MpegEncContext* s	= (MpegEncContext*)pAVCtx->priv_data;
	s->dxva_context		= pDXVA_Context;
}

// === HEVC functions
void FFHEVCSetDxvaParams(struct AVCodecContext* pAVCtx, void* pDXVA_Context)
{
	HEVCContext* s		= (HEVCContext*)pAVCtx->priv_data;
	s->dxva_context		= pDXVA_Context;
}

// === Common functions
#define INVALID_TIME _I64_MIN
HRESULT FFDecodeFrame(struct AVCodecContext* pAVCtx, struct AVFrame* pFrame,
					  BYTE* pBuffer, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop,
					  int* got_picture_ptr, AVFrame** ppFrameOut)
{
	AVPacket avpkt;
	av_init_packet(&avpkt);
	if (pBuffer) {
		avpkt.data	= pBuffer;
		avpkt.size	= nSize;
		avpkt.pts	= rtStart;
		avpkt.dts	= rtStop;
		if (rtStart != INVALID_TIME && rtStop != INVALID_TIME) {
			avpkt.duration = (int)(rtStop - rtStart);
		} else {
			avpkt.duration = 0;
		}
		avpkt.flags	= AV_PKT_FLAG_KEY;
	} else {
		avpkt.data	= NULL;
		avpkt.size	= 0;
	}

	int used_bytes	= avcodec_decode_video2(pAVCtx, pFrame, got_picture_ptr, &avpkt);

#if defined(_DEBUG) && 0
	av_log(pAVCtx, AV_LOG_INFO, "FFDecodeFrame() : %d, %d\n", used_bytes, got_picture);
#endif

	if (used_bytes < 0) {
		return E_FAIL;
	}

	if (!(*got_picture_ptr) && !avpkt.size) {
		return E_FAIL;
	}

	switch (pAVCtx->codec_id) {
		case AV_CODEC_ID_H264:
			{
				H264Context* h		= (H264Context*)pAVCtx->priv_data;
				*ppFrameOut			= &h->cur_pic.f;
			}
			break;
		case AV_CODEC_ID_HEVC:
			{
				HEVCContext *h		= (HEVCContext*)pAVCtx->priv_data;
				*ppFrameOut			= h->ref->frame;
			}
			break;
		case AV_CODEC_ID_VC1:
		case AV_CODEC_ID_WMV3:
			{
				VC1Context* v		= (VC1Context*)pAVCtx->priv_data;
				*ppFrameOut			= v->s.current_picture_ptr->f;
			}
			break;
		case AV_CODEC_ID_MPEG2VIDEO:
			{
				MpegEncContext *s	= (MpegEncContext*)pAVCtx->priv_data;
				*ppFrameOut			= s->current_picture.f;
			}
			break;
		default:
			return E_FAIL;
	}

	return S_OK;
}

BOOL FFGetAlternateScan(struct AVCodecContext* pAVCtx)
{
	MpegEncContext* s = (MpegEncContext*)pAVCtx->priv_data;

	return s ? s->alternate_scan : FALSE;
}

UINT FFGetMBCount(struct AVCodecContext* pAVCtx)
{
	UINT MBCount = 0;
	switch (pAVCtx->codec_id) {
		case AV_CODEC_ID_H264 :
			{
				H264Context* h		= (H264Context*)pAVCtx->priv_data;
				MBCount				= h->mb_width * h->mb_height;
			}
			break;
		case AV_CODEC_ID_MPEG2VIDEO:
			{
				MpegEncContext* s	= (MpegEncContext*)pAVCtx->priv_data;
				const int is_field	= s->picture_structure != PICT_FRAME;
				MBCount				= s->mb_width * (s->mb_height >> is_field);
			}
			break;
	}

	return MBCount;
}

void FFGetFrameProps(struct AVCodecContext* pAVCtx, struct AVFrame* pFrame, int& width, int& height)
{
	switch (pAVCtx->codec_id) {
	case AV_CODEC_ID_H264:
		{
			H264Context* h	= (H264Context*)pAVCtx->priv_data;
			SPS* cur_sps	= &h->sps;
			if (cur_sps) {
				width		= cur_sps->mb_width * 16;
				height		= cur_sps->mb_height * (2 - cur_sps->frame_mbs_only_flag) * 16;
			}
		}
		break;
	case AV_CODEC_ID_MPEG1VIDEO:
	case AV_CODEC_ID_MPEG2VIDEO:
		{
			MpegEncContext*	s = (MpegEncContext*)pAVCtx->priv_data;

			width	= FFALIGN(s->width, 16);
			height	= FFALIGN(s->height, 16);
			if (!s->progressive_sequence) {
				height = int((s->height + 31) / 32 * 2) * 16;
			}

			if (pAVCtx->pix_fmt == AV_PIX_FMT_NONE) {
				if(s->chroma_format < 2) {
					pAVCtx->pix_fmt = AV_PIX_FMT_YUV420P;
				} else if(s->chroma_format == 2) {
					pAVCtx->pix_fmt = AV_PIX_FMT_YUV422P;
				} else {
					pAVCtx->pix_fmt = AV_PIX_FMT_YUV444P;
				}
			}
		}
		break;
	case AV_CODEC_ID_LAGARITH:
		if (pAVCtx->pix_fmt == AV_PIX_FMT_NONE && pAVCtx->extradata_size >= 4) {
			switch (*(DWORD*)pAVCtx->extradata) {
			case 0:
				if (pAVCtx->bits_per_coded_sample == 32) {
					pAVCtx->pix_fmt = AV_PIX_FMT_RGBA;
				} else if (pAVCtx->bits_per_coded_sample == 24) {
					pAVCtx->pix_fmt = AV_PIX_FMT_RGB24;
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
		if (pAVCtx->pix_fmt == AV_PIX_FMT_NONE && pAVCtx->extradata_size >= 8) {
			switch (*(DWORD*)(pAVCtx->extradata + 4)) {
			case 'hcpa': // Apple ProRes 422 High Quality
			case 'ncpa': // Apple ProRes 422 Standard Definition
			case 'scpa': // Apple ProRes 422 LT
			case 'ocpa': // Apple ProRes 422 Proxy
				pAVCtx->pix_fmt = AV_PIX_FMT_YUV422P10LE;
				break;
			case 'h4pa': // Apple ProRes 4444
				pAVCtx->pix_fmt = pAVCtx->bits_per_coded_sample == 32 ? AV_PIX_FMT_YUVA444P10LE : AV_PIX_FMT_YUV444P10LE;
				break;
			}
		}
		break;
	case AV_CODEC_ID_MJPEG:
	case AV_CODEC_ID_DNXHD:
		if (pAVCtx->pix_fmt == AV_PIX_FMT_NONE) {
			av_log(pAVCtx, AV_LOG_INFO, "WARNING! : pAVCtx->pix_fmt == AV_PIX_FMT_NONE\n");
			pAVCtx->pix_fmt = AV_PIX_FMT_YUV422P; // bad hack
		}
		break;
	case AV_CODEC_ID_HEVC: // TODO
	default:
		if (pAVCtx->pix_fmt == AV_PIX_FMT_NONE) {
			av_log(pAVCtx, AV_LOG_INFO, "WARNING! : pAVCtx->pix_fmt == AV_PIX_FMT_NONE\n");
			pAVCtx->pix_fmt = AV_PIX_FMT_YUV420P; // bad hack
		}
	}
}

#define CHECK_AVC_L52_SIZE(w, h) ((w) <= 4096 && (h) <= 4096 && (w) * (h) <= 36864 * 16 * 16)

BOOL DXVACheckFramesize(enum AVCodecID nCodecId, int width, int height, DWORD nPCIVendor, DWORD nPCIDevice)
{
	width = (width + 15) & ~15; // (width + 15) / 16 * 16;
	height = (height + 15) & ~15; // (height + 15) / 16 * 16;

	//if (nCodecId == AV_CODEC_ID_WMV3 && width <= 720) {
	//	// fixes color problem for some wmv files (profile <= MP@ML)
	//	return FALSE;
	//}

	if (nPCIVendor == PCIV_nVidia) {
		if (CheckPCID(nPCIDevice, PCID_NVIDIA_VP567, _countof(PCID_NVIDIA_VP567)) && width <= 4096 && height <= 4096 && width * height <= 4080 * 4080) {
			// tested H.264 on VP5 (GT 610, GTX 660 Ti)
			// 4080x4080 = 65025 macroblocks
			return TRUE;
		} else if (width <= 2032 && height <= 2032 && width * height <= 8190 * 16 * 16) {
			// tested H.264, VC-1 and MPEG-2 on VP4 (feature set C) (G210M, GT220)
			return TRUE;
		}
	} else if (nPCIVendor == PCIV_ATI) {
		if (width <= 2048 && height <= 2304 && width * height <= 2048 * 2048) {
			// tested H.264 on UVD 2.2 (HD5670, HD5770, HD5850)
			// it may also work if width = 2064, but unstable
			return TRUE;
		}
	} else if (nPCIVendor == PCIV_Intel && nPCIDevice == PCID_Intel_HD4000) {
		if (width <= 4096 && height <= 4096) { // driver >= v.9.17.10.2867
			// complete test was performed
			return TRUE;
		}
	} else if (nPCIVendor == PCIV_Intel && CheckPCID(nPCIDevice, PCID_INTEL_4K, _countof(PCID_INTEL_4K))) {
		if (CHECK_AVC_L52_SIZE(width, height)) {
			// tested some media files with AVC Livel 5.1
			// complete test was NOT performed
			return TRUE;
		}
	} else if (width <= 1920 && height <= 1088) {
		return TRUE;
	}

	return FALSE;
}

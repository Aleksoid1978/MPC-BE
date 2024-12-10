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

#pragma once

// Internal codec list (use to enable/disable codec in standalone mode)

#define CODEC_H264			(1ULL << 0)
#define CODEC_MPEG1			(1ULL << 1)
#define CODEC_MPEG2			(1ULL << 2)
#define CODEC_VC1			(1ULL << 3)
#define CODEC_MSMPEG4		(1ULL << 4)
#define CODEC_XVID			(1ULL << 5)
#define CODEC_DIVX			(1ULL << 6)
#define CODEC_WMV			(1ULL << 7)
#define CODEC_HEVC			(1ULL << 8)
#define CODEC_VP356			(1ULL << 9)
#define CODEC_VP89			(1ULL << 10)
#define CODEC_THEORA		(1ULL << 11)
#define CODEC_MJPEG			(1ULL << 12)
#define CODEC_DV			(1ULL << 13)
#define CODEC_LOSSLESS		(1ULL << 14)
#define CODEC_PRORES		(1ULL << 15)
#define CODEC_CANOPUS		(1ULL << 16)
#define CODEC_SCREC			(1ULL << 17)
#define CODEC_INDEO			(1ULL << 18)
#define CODEC_H263			(1ULL << 19)
#define CODEC_SVQ3			(1ULL << 20)
#define CODEC_REALV			(1ULL << 21)
#define CODEC_DIRAC			(1ULL << 22)
#define CODEC_BINKV			(1ULL << 23)
#define CODEC_AMVV			(1ULL << 24)
#define CODEC_FLASH			(1ULL << 25)
#define CODEC_UTVD			(1ULL << 26)
#define CODEC_PNG			(1ULL << 27)
#define CODEC_UNCOMPRESSED	(1ULL << 28)
#define CODEC_DNXHD			(1ULL << 29)
#define CODEC_CINEPAK		(1ULL << 30)
#define CODEC_QT			(1ULL << 31)
#define CODEC_CINEFORM		(1ULL << 32)
#define CODEC_H264_MVC		(1ULL << 33)
#define CODEC_HAP			(1ULL << 34)
#define CODEC_AV1			(1ULL << 34)
#define CODEC_SHQ			(1ULL << 35)
#define CODEC_AVS3			(1ULL << 36)
#define CODEC_VVC			(1ULL << 37)

#define CODECS_ALL (CODEC_H264|CODEC_MPEG1|CODEC_MPEG2|CODEC_VC1|CODEC_MSMPEG4|CODEC_XVID|CODEC_DIVX|CODEC_WMV|CODEC_HEVC| \
					CODEC_VP356|CODEC_VP89|CODEC_THEORA|CODEC_MJPEG|CODEC_DV|CODEC_LOSSLESS|CODEC_PRORES|CODEC_CANOPUS|CODEC_SCREC|CODEC_INDEO| \
					CODEC_H263|CODEC_SVQ3|CODEC_REALV|CODEC_DIRAC|CODEC_BINKV|CODEC_AMVV|CODEC_FLASH|CODEC_UTVD|CODEC_PNG|CODEC_UNCOMPRESSED| \
					CODEC_DNXHD|CODEC_CINEPAK|CODEC_QT|CODEC_CINEFORM|CODEC_H264_MVC|CODEC_HAP|CODEC_AV1|CODEC_SHQ|CODEC_AVS3|CODEC_VVC)

enum MPC_SCAN_TYPE : int {
	SCAN_AUTO = 0,
	SCAN_TOPFIELD,
	SCAN_BOTTOMFIELD,
	SCAN_PROGRESSIVE
};

enum MPCHwCodec {
	HWCodec_None = -1,
	HWCodec_MPEG2,
	HWCodec_WMV3,
	HWCodec_VC1,
	HWCodec_H264,
	HWCodec_HEVC,
	HWCodec_VP9,
	HWCodec_AV1,
	HWCodec_count
};

enum MPCHwDecoder {
	HWDec_DXVA2 = 0,
	HWDec_D3D11,
	HWDec_D3D11cb,
	HWDec_D3D12cb,
	HWDec_NVDEC,
	HWDec_count
};

enum MPCPixelFormat {
	PixFmt_None = -1,
	// YUV 8 bit
	PixFmt_NV12,  // 4:2:0, 12 bit
	PixFmt_YV12,  // 4:2:0, 12 bit
	PixFmt_YUY2,  // 4:2:2, 16 bit
	PixFmt_YV16,  // 4:2:2, 16 bit
	PixFmt_AYUV,  // 4:4:4, 24(32) bit
	PixFmt_YV24,  // 4:4:4, 24 bit
	// YUV 10 bit
	PixFmt_P010,  // 4:2:0, 15(24) bit
	PixFmt_P210,  // 4:2:2, 20(32) bit
	PixFmt_Y410,  // 4:4:4, 30(32) bit
	// YUV 16 bit
	PixFmt_P016,  // 4:2:0, 24 bit
	PixFmt_P216,  // 4:2:2, 32 bit
	PixFmt_Y416,  // 4:4:4, 48(64) bit
	PixFmt_YUV444P16, // 4:4:4, 48 bit
	// RGB
	PixFmt_RGB32, // 24(32) bit
	PixFmt_RGB48, // 48 bit
	PixFmt_count
};

enum MPCInfo {
	INFO_MPCVersion,
	INFO_InputFormat,
	INFO_FrameSize,
	INFO_OutputFormat,
	INFO_GraphicsAdapter
};

struct MPC_ADAPTER_ID {
	UINT VendorId;
	UINT DeviceId;
};

interface __declspec(uuid("CDC3B5B3-A8B0-4c70-A805-9FC80CDEF262"))
IMPCVideoDecFilter :
public IUnknown {

	STDMETHOD(SetThreadNumber(int nValue)) PURE;
	STDMETHOD_(int, GetThreadNumber()) PURE;
	STDMETHOD(SetDiscardMode(int nValue)) PURE;
	STDMETHOD_(int, GetDiscardMode()) PURE;
	STDMETHOD(SetScanType(MPC_SCAN_TYPE nValue)) PURE;
	STDMETHOD_(MPC_SCAN_TYPE, GetScanType()) PURE;
	STDMETHOD(SetARMode(int nValue)) PURE;
	STDMETHOD_(int, GetARMode()) PURE;

	STDMETHOD(SetHwCodec(MPCHwCodec hwcodec, bool enable)) PURE;
	STDMETHOD_(bool, GetHwCodec(MPCHwCodec hwcodec)) PURE;
	STDMETHOD(SetHwDecoder(int value)) PURE;
	STDMETHOD_(int, GetHwDecoder()) PURE;
	STDMETHOD(SetDXVACheckCompatibility(int nValue)) PURE;
	STDMETHOD_(int, GetDXVACheckCompatibility()) PURE;
	STDMETHOD(SetDXVA_SD(int nValue)) PURE;
	STDMETHOD_(int, GetDXVA_SD()) PURE;

	STDMETHOD(SetSwRefresh(int nValue)) PURE;
	STDMETHOD(SetSwPixelFormat(MPCPixelFormat pf, bool enable)) PURE;
	STDMETHOD_(bool, GetSwPixelFormat(MPCPixelFormat pf)) PURE;
	STDMETHOD(SetSwConvertToRGB(bool enable)) PURE;
	STDMETHOD_(bool, GetSwConvertToRGB()) PURE;
	STDMETHOD(SetSwRGBLevels(int nValue)) PURE;
	STDMETHOD_(int, GetSwRGBLevels()) PURE;

	STDMETHOD(SetActiveCodecs(ULONGLONG nValue)) PURE;
	STDMETHOD_(ULONGLONG, GetActiveCodecs()) PURE;

	STDMETHOD(SaveSettings()) PURE;

	STDMETHOD_(CString, GetInformation(MPCInfo index)) PURE;

	STDMETHOD_(GUID*, GetDXVADecoderGuid()) PURE;
	STDMETHOD_(int, GetColorSpaceConversion()) PURE;

	STDMETHOD(GetD3D11Adapter(MPC_ADAPTER_ID* pAdapterId)) PURE;
	STDMETHOD(SetD3D11Adapter(UINT VendorId, UINT DeviceId)) PURE;
};

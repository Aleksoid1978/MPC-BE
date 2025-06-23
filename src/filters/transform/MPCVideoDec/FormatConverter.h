/*
 * (C) 2014-2025 see Authors.txt
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

#include "IMPCVideoDec.h"
#include <stdint.h>

const MPCPixelFormat YUV420_8[]  = {PixFmt_NV12, PixFmt_YV12, PixFmt_YUY2, PixFmt_YV16, PixFmt_YV24, PixFmt_AYUV, PixFmt_P010, PixFmt_P016, PixFmt_P210, PixFmt_P216, PixFmt_Y410, PixFmt_YUV444P16, PixFmt_Y416, PixFmt_None};
const MPCPixelFormat YUV422_8[]  = {PixFmt_YUY2, PixFmt_YV16, PixFmt_YV24, PixFmt_AYUV, PixFmt_NV12, PixFmt_YV12, PixFmt_P210, PixFmt_P216, PixFmt_Y410, PixFmt_P010, PixFmt_P016, PixFmt_YUV444P16, PixFmt_Y416, PixFmt_None};
const MPCPixelFormat YUV444_8[]  = {PixFmt_YV24, PixFmt_AYUV, PixFmt_YUY2, PixFmt_YV16, PixFmt_NV12, PixFmt_YV12, PixFmt_Y410, PixFmt_P210, PixFmt_P216, PixFmt_P010, PixFmt_P016, PixFmt_YUV444P16, PixFmt_Y416, PixFmt_None};

const MPCPixelFormat YUV420_10[] = {PixFmt_P010, PixFmt_P016, PixFmt_P210, PixFmt_P216, PixFmt_Y410, PixFmt_YUV444P16, PixFmt_Y416, PixFmt_NV12, PixFmt_YV12, PixFmt_YUY2, PixFmt_YV16, PixFmt_YV24, PixFmt_AYUV, PixFmt_None};
const MPCPixelFormat YUV422_10[] = {PixFmt_P210, PixFmt_P216, PixFmt_YUY2, PixFmt_YV16, PixFmt_Y410, PixFmt_P010, PixFmt_P016, PixFmt_YUV444P16, PixFmt_Y416, PixFmt_YV24, PixFmt_AYUV, PixFmt_NV12, PixFmt_YV12, PixFmt_None};
const MPCPixelFormat YUV444_10[] = {PixFmt_Y410, PixFmt_YV24, PixFmt_AYUV, PixFmt_YUV444P16, PixFmt_Y416, PixFmt_P210, PixFmt_P216, PixFmt_P010, PixFmt_P016, PixFmt_YUY2, PixFmt_YV16, PixFmt_NV12, PixFmt_YV12, PixFmt_None};

const MPCPixelFormat YUV420_16[] = {PixFmt_P016, PixFmt_P010, PixFmt_P216, PixFmt_YUV444P16, PixFmt_Y416, PixFmt_P210, PixFmt_Y410, PixFmt_NV12, PixFmt_YV12, PixFmt_YUY2, PixFmt_YV16, PixFmt_YV24, PixFmt_AYUV, PixFmt_None};
const MPCPixelFormat YUV422_16[] = {PixFmt_P216, PixFmt_P210, PixFmt_YUY2, PixFmt_YV16, PixFmt_YUV444P16, PixFmt_Y416, PixFmt_P016, PixFmt_Y410, PixFmt_P010, PixFmt_YV24, PixFmt_AYUV, PixFmt_NV12, PixFmt_YV12, PixFmt_None};
const MPCPixelFormat YUV444_16[] = {PixFmt_YUV444P16, PixFmt_Y416, PixFmt_Y410, PixFmt_YV24, PixFmt_AYUV, PixFmt_P216, PixFmt_P016, PixFmt_P210, PixFmt_P010, PixFmt_YUY2, PixFmt_YV16, PixFmt_NV12, PixFmt_YV12, PixFmt_None};

const MPCPixelFormat RGB_8[]     = {PixFmt_RGB32, PixFmt_RGB48, PixFmt_None};
const MPCPixelFormat RGB_16[]    = {PixFmt_RGB48, PixFmt_RGB32, PixFmt_None};

struct SW_OUT_FMT {
	const LPCWSTR				name;
	const DWORD					biCompression;
	const GUID*					subtype;
	const int					bpp;
	const int					codedbytes;
	const int					planes;
	const int					planeHeight[4];
	const int					planeWidth[4];
	const enum AVPixelFormat	av_pix_fmt;
	const uint8_t				chroma_w;
	const uint8_t				chroma_h;
	const int					actual_bpp;
	const int					luma_bits;
};

extern SW_OUT_FMT s_sw_formats[];

const SW_OUT_FMT* GetSWOF(int pixfmt);
LPCWSTR GetChromaSubsamplingStr(enum AVPixelFormat av_pix_fmt);
int GetLumaBits(enum AVPixelFormat av_pix_fmt);

// CFormatConverter

struct AVFrame;
struct SwsContext;

enum MPCPixFmtType {
	PFType_unspecified,
	PFType_RGB,
	PFType_YUV420,   // YUV 4:2:0, 8 bit
	PFType_YUV422,   // YUV 4:2:2, 8 bit
	PFType_YUV444,   // YUV 4:2:2, 8 bit
	PFType_YUV420Px, // YUV 4:2:0, 9-16 bit
	PFType_YUV422Px, // YUV 4:2:2, 9-16 bit
	PFType_YUV444Px, // YUV 4:4:4, 9-16 bit
	PFType_NV12,     // YUV 4:2:0, U/V interleaved
	PFType_P01x,     // YUV 4:2:0, 10 to 16-bit, U/V interleaved, MSB aligned
	PFType_Y21x,     // YUV 4:2:0, 10 to 16-bit
};

struct FrameProps {
	// basic properties
	enum AVPixelFormat	avpixfmt;
	int					width;
	int					height;
	// additional properties
	int					lumabits;
	MPCPixFmtType		pftype;
	enum AVColorSpace	colorspace;
	enum AVColorRange	colorrange;
};

MPCPixFmtType GetPixFmtType(enum AVPixelFormat av_pix_fmt);

typedef struct _RGBCoeffs {
	__m128i Ysub;
	__m128i CbCr_center;
	__m128i rgb_add;
	__m128i cy;
	__m128i cR_Cr;
	__m128i cG_Cb_cG_Cr;
	__m128i cB_Cb;
} RGBCoeffs;

typedef int (__stdcall *YUVRGBConversionFunc)(const uint8_t *srcY, const uint8_t *srcU, const uint8_t *srcV, uint8_t *dst, int width, int height, ptrdiff_t srcStrideY, ptrdiff_t srcStrideUV, ptrdiff_t dstStride, ptrdiff_t sliceYStart, ptrdiff_t sliceYEnd, const RGBCoeffs *coeffs, const uint16_t *dithers);

#define CONV_FUNC_PARAMS const uint8_t* const src[4], const ptrdiff_t srcStride[4], uint8_t* dst[], int width, int height, const ptrdiff_t dstStride[]

class CFormatConverter
{

protected:
	SwsContext*		m_pSwsContext = nullptr;
	FrameProps		m_FProps;

	MPCPixelFormat	m_out_pixfmt = PixFmt_None;

	int				m_dstRGBRange = 0;

	int				m_dstStride   = 0;
	int				m_planeHeight = 0;
	int				m_OutHeight   = 0;

	size_t			m_nAlignedBufferSize = 0;
	uint8_t*		m_pAlignedBuffer = nullptr;

	int				m_nCPUFlag = 0;

	unsigned		m_RequiredAlignment = 0;

	int				m_NumThreads = 1;

	bool InitSWSContext();
	void UpdateSWSContext();

	int m_swsWidth  = 0;
	int m_swsHeight = 0;

	RGBCoeffs *m_rgbCoeffs = nullptr;
	BOOL m_bRGBConverter   = FALSE;
	BOOL m_bRGBConvInit    = FALSE;

	// [out32][dithermode][ycgco][format][shift]
	YUVRGBConversionFunc m_RGBConvFuncs[2][1][2][PixFmt_count][9];

	void SetConvertFunc();

	// Conversion function pointer
	typedef HRESULT (CFormatConverter::*ConverterFn)(CONV_FUNC_PARAMS);
	ConverterFn m_pConvertFn = nullptr;
	BOOL m_bDirect = FALSE;

	// from LAV Filters
	HRESULT ConvertGeneric(CONV_FUNC_PARAMS);
	HRESULT ConvertToAYUV(CONV_FUNC_PARAMS);
	HRESULT ConvertToPX1X(CONV_FUNC_PARAMS, int chromaVertical);
	HRESULT ConvertToY410(CONV_FUNC_PARAMS);
	HRESULT ConvertToY416(CONV_FUNC_PARAMS);

	// optimized function
	HRESULT plane_copy_sse2(CONV_FUNC_PARAMS);
	HRESULT convert_p010_nv12_sse2(CONV_FUNC_PARAMS);
	HRESULT convert_y210_p210_sse4(CONV_FUNC_PARAMS);
	HRESULT convert_yuy2_yv16_sse2(CONV_FUNC_PARAMS);

	// optimized direct function
	HRESULT plane_copy_direct_sse4(CONV_FUNC_PARAMS);
	HRESULT plane_copy_direct_nv12_sse4(CONV_FUNC_PARAMS);
	HRESULT convert_nv12_yv12_direct_sse4(CONV_FUNC_PARAMS);
	HRESULT convert_p010_nv12_direct_sse4(CONV_FUNC_PARAMS);
	HRESULT convert_y210_p210_direct_sse4(CONV_FUNC_PARAMS);
	HRESULT convert_yuy2_yv16_direct_sse4(CONV_FUNC_PARAMS);

	HRESULT convert_yuv_yv_nv12_dither_le(CONV_FUNC_PARAMS);
	HRESULT convert_yuv420_px1x_le(CONV_FUNC_PARAMS);
	HRESULT convert_yuv_yv(CONV_FUNC_PARAMS);
	HRESULT convert_yuv420_nv12(CONV_FUNC_PARAMS);
	HRESULT convert_yuv422_yuy2_uyvy_dither_le(CONV_FUNC_PARAMS);
	HRESULT convert_nv12_yv12(CONV_FUNC_PARAMS);

	HRESULT convert_yuv444_y410(CONV_FUNC_PARAMS);

	HRESULT convert_yuv444_ayuv(CONV_FUNC_PARAMS);
	HRESULT convert_yuv444_ayuv_dither_le(CONV_FUNC_PARAMS);

	HRESULT convert_yuv420_yuy2(CONV_FUNC_PARAMS);

	HRESULT convert_yuv_rgb(CONV_FUNC_PARAMS);

	void InitRGBConvDispatcher();
	const RGBCoeffs* getRGBCoeffs(int width, int height);

public:
	CFormatConverter();
	~CFormatConverter();

	void UpdateOutput(MPCPixelFormat out_pixfmt, int dstStride, int planeHeight);
	void UpdateOutput2(DWORD biCompression, LONG biWidth, LONG biHeight);
	void SetOptions(int rgblevels);

	MPCPixelFormat GetOutPixFormat() { return m_out_pixfmt; }

	bool Converting(BYTE* dst, AVFrame* pFrame);
	void SetDirect(BOOL bDirect) { m_bDirect = bDirect; }

	void Cleanup();

	bool FormatChanged(AVPixelFormat* fmt1, AVPixelFormat* fmt2);

	bool DirectCopyPossible(AVPixelFormat avformat);

	int GetDstStride() const { return m_dstStride; }

	void Clear();
};

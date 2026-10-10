/*
 * (C) 2014-2026 see Authors.txt
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
#include <mpc_defines.h>
#include <moreuuids.h>
#include "FormatConverter.h"
#include "DSUtil/CPUInfo.h"
#include "DSUtil/Utils.h"

#pragma warning(push)
#pragma warning(disable: 4005)
#pragma warning(disable: 5033)
extern "C" {
	#include <ExtLib/ffmpeg/libavcodec/defs.h>
	#include <ExtLib/ffmpeg/libswscale/swscale.h>
	#include <ExtLib/ffmpeg/libavutil/pixdesc.h>
}
#pragma warning(pop)

FrameProps::FrameProps()
	: avpixfmt(AV_PIX_FMT_NONE)
	, colorspace(AVCOL_SPC_UNSPECIFIED)
	, colorrange(AVCOL_RANGE_UNSPECIFIED)
{
}

const SW_OUT_FMT s_sw_formats[] = {
	//name             bpp planeWidth planeHeight  av_pix_fmt   chroma_w chroma_h
	// YUV 8 bit
	{ VFormat_NV12,      12, {1,1},   {1,2},   AV_PIX_FMT_NV12,        1, 1 }, // PixFmt_NV12
	{ VFormat_YV12,      12, {1,2,2}, {1,2,2}, AV_PIX_FMT_YUV420P,     1, 1 }, // PixFmt_YV12
	{ VFormat_YUY2,      16, {1},     {1},     AV_PIX_FMT_YUYV422,     1, 0 }, // PixFmt_YUY2
	{ VFormat_YV16,      16, {1,2,2}, {1,1,1}, AV_PIX_FMT_YUV422P,     1, 0 }, // PixFmt_YV16
	{ VFormat_AYUV,      32, {1},     {1},     AV_PIX_FMT_YUV444P,     0, 0 }, // PixFmt_AYUV
	{ VFormat_YV24,      24, {1,1,1}, {1,1,1}, AV_PIX_FMT_YUV444P,     0, 0 }, // PixFmt_YV24
	// YUV 10 bit
	{ VFormat_P010,      24, {1,1},   {1,2},   AV_PIX_FMT_YUV420P16LE, 1, 1 }, // PixFmt_P010
	{ VFormat_P210,      32, {1,1},   {1,1},   AV_PIX_FMT_YUV422P16LE, 1, 0 }, // PixFmt_P210
	{ VFormat_Y410,      32, {1},     {1},     AV_PIX_FMT_YUV444P10LE, 0, 0 }, // PixFmt_Y410
	// YUV 16 bit
	{ VFormat_P016,      24, {1,1},   {1,2},   AV_PIX_FMT_YUV420P16LE, 1, 1 }, // PixFmt_P016
	{ VFormat_P216,      32, {1,1},   {1,1},   AV_PIX_FMT_YUV422P16LE, 1, 0 }, // PixFmt_P216
	{ VFormat_Y416,      64, {1},     {1},     AV_PIX_FMT_YUV444P16LE, 0, 0 }, // PixFmt_Y416
	{ VFormat_YUV444P16, 48, {1,1,1}, {1,1,1}, AV_PIX_FMT_YUV444P16LE, 0, 0 }, // PixFmt_YUV444P16
	// RGB
	{ VFormat_RGB32,     32, {1},     {1},     AV_PIX_FMT_BGRA,        0, 0 }, // PixFmt_RGB32
	{ VFormat_RGB48,     48, {1},     {1},     AV_PIX_FMT_RGB48LE,     0, 0 }, // PixFmt_RGB48
	// PS:
	// AV_PIX_FMT_YUV444P not equal to AYUV, but is used as an intermediate format.
	// AV_PIX_FMT_YUV420P16LE not equal to P010, but is used as an intermediate format.
	// AV_PIX_FMT_YUV422P16LE not equal to P210, but is used as an intermediate format.
};

static_assert(std::size(s_sw_formats) == PixFmt_count);

const SW_OUT_FMT* GetSWOF(const int pixfmt)
{
	if (pixfmt < 0 || pixfmt >= PixFmt_count) {
		return nullptr;
	}
	return &s_sw_formats[pixfmt];
}

LPCWSTR GetChromaSubsamplingStr(const AVPixelFormat av_pix_fmt)
{
	const AVPixFmtDescriptor* pfdesc = av_pix_fmt_desc_get(av_pix_fmt);
	if (pfdesc && pfdesc->nb_components >= 3) {
		unsigned chroma_sub_sample = ((unsigned)pfdesc->log2_chroma_w << 8) + pfdesc->log2_chroma_h;

		switch (chroma_sub_sample) {
		case 0x0000: return L"4:4:4";
		case 0x0001: return L"4:4:0";
		case 0x0100: return L"4:2:2";
		case 0x0101: return L"4:2:0";
		case 0x0200: return L"4:1:1";
		case 0x0202: return L"4:1:0";
		}
	}

	return L"";
}

static int GetLumaBits(const AVPixFmtDescriptor* avpfdesc)
{
	return (avpfdesc ? avpfdesc->comp[0].depth : 0);
}

int GetLumaBits(const AVPixelFormat av_pix_fmt)
{
	const AVPixFmtDescriptor* avpfdesc = av_pix_fmt_desc_get(av_pix_fmt);

	return GetLumaBits(avpfdesc);
}

static MPCPixelFormat GetPixFormat(const GUID& subtype)
{
	for (int i = 0; i < PixFmt_count; i++) {
		if (*s_sw_formats[i].desc.subtype == subtype) {
			return (MPCPixelFormat)i;
		}
	}

	return PixFmt_None;
}

static MPCPixelFormat GetPixFormat(const AVPixelFormat av_pix_fmt)
{
	for (int i = 0; i < PixFmt_count; i++) {
		if (s_sw_formats[i].av_pix_fmt == av_pix_fmt) {
			return (MPCPixelFormat)i;
		}
	}

	return PixFmt_None;
}

static MPCPixelFormat GetPixFormat(const DWORD biCompression)
{
	for (int i = 0; i < PixFmt_count; i++) {
		if (s_sw_formats[i].desc.fourcc == biCompression) {
			return (MPCPixelFormat)i;
		}
	}

	return PixFmt_None;
}

MPCPixFmtType GetPixFmtType(const AVPixelFormat av_pix_fmt, const AVPixFmtDescriptor* avpfdesc)
{
	switch (av_pix_fmt) {
	case AV_PIX_FMT_YUV420P:
	case AV_PIX_FMT_YUVJ420P:
		return PFType_YUV420;

	case AV_PIX_FMT_YUV422P:
	case AV_PIX_FMT_YUVJ422P:
		return PFType_YUV422;

	case AV_PIX_FMT_YUV444P:
	case AV_PIX_FMT_YUVJ444P:
		return PFType_YUV444;

	case AV_PIX_FMT_YUV420P9LE:
	case AV_PIX_FMT_YUV420P10LE:
	case AV_PIX_FMT_YUV420P12LE:
	case AV_PIX_FMT_YUV420P14LE:
	case AV_PIX_FMT_YUV420P16LE:
	case AV_PIX_FMT_YUVA420P9LE:
	case AV_PIX_FMT_YUVA420P10LE:
	case AV_PIX_FMT_YUVA420P16LE:
		return PFType_YUV420Px;

	case AV_PIX_FMT_YUV422P9LE:
	case AV_PIX_FMT_YUV422P10LE:
	case AV_PIX_FMT_YUV422P12LE:
	case AV_PIX_FMT_YUV422P14LE:
	case AV_PIX_FMT_YUV422P16LE:
	case AV_PIX_FMT_YUVA422P9LE:
	case AV_PIX_FMT_YUVA422P10LE:
	case AV_PIX_FMT_YUVA422P12LE:
	case AV_PIX_FMT_YUVA422P16LE:
		return PFType_YUV422Px;

	case AV_PIX_FMT_YUV444P9LE:
	case AV_PIX_FMT_YUV444P10LE:
	case AV_PIX_FMT_YUV444P12LE:
	case AV_PIX_FMT_YUV444P14LE:
	case AV_PIX_FMT_YUV444P16LE:
	case AV_PIX_FMT_YUVA444P9LE:
	case AV_PIX_FMT_YUVA444P10LE:
	case AV_PIX_FMT_YUVA444P12LE:
	case AV_PIX_FMT_YUVA444P16LE:
		return PFType_YUV444Px;

	case AV_PIX_FMT_NV12:
		return PFType_NV12;

	case AV_PIX_FMT_P010LE:
	case AV_PIX_FMT_P012LE:
	case AV_PIX_FMT_P016LE:
		return PFType_P01x;

	case AV_PIX_FMT_P210LE:
	case AV_PIX_FMT_P212LE:
	case AV_PIX_FMT_P216LE:
		return PFType_P21x;

	case AV_PIX_FMT_P410LE:
	case AV_PIX_FMT_P412LE:
	case AV_PIX_FMT_P416LE:
		return PFType_P41x;

	case AV_PIX_FMT_Y210LE:
	case AV_PIX_FMT_Y212LE:
	case AV_PIX_FMT_Y216LE:
		return PFType_Y21x;
	}

	if (avpfdesc) {
		if (avpfdesc->flags & (AV_PIX_FMT_FLAG_RGB | AV_PIX_FMT_FLAG_PAL)) {
			return PFType_RGB;
		}
	}

	return PFType_unspecified;
}

// CFormatConverter

CFormatConverter::CFormatConverter()
{
	ASSERT(PixFmt_count == std::size(s_sw_formats));

	m_NumThreads = std::clamp(CPUInfo::GetProcessorNumber() / 2, 1uL, 8uL);
}

CFormatConverter::~CFormatConverter()
{
	Cleanup();
}

void* CFormatConverter::GetTempBuffer(const size_t size)
{
	if (size > m_nTempBufferSize) {
		void* pTmpBuffer = av_realloc(m_pTempBuffer, size + AV_INPUT_BUFFER_PADDING_SIZE);
		if (pTmpBuffer == nullptr) {
			return nullptr;
		}
		m_pTempBuffer = pTmpBuffer;
		m_nTempBufferSize = size;
	}
	return m_pTempBuffer;
}

bool CFormatConverter::InitSWSContext()
{
	if (m_FProps.avpixfmt == AV_PIX_FMT_NONE) {
		DLog(L"FormatConverter::InitSWSContext() - incorrect source format");
		return false;
	}
	if (m_out_pixfmt == PixFmt_None) {
		DLog(L"FormatConverter::InitSWSContext() - incorrect output format");
		return false;
	}

	const SW_OUT_FMT& swof = s_sw_formats[m_out_pixfmt];

	m_pSwsContext = sws_getContext(
						m_FProps.width,
						m_FProps.height,
						m_FProps.avpixfmt,
						m_FProps.width,
						m_FProps.height,
						swof.av_pix_fmt,
						SWS_BILINEAR | SWS_FULL_CHR_H_INT | SWS_PRINT_INFO,
						nullptr,
						nullptr,
						nullptr);

	if (m_pSwsContext == nullptr) {
		DLog(L"FormatConverter::InitSWSContext() - sws_getCachedContext() failed");
		return false;
	}

	UpdateSWSContext();

	return true;
}

void CFormatConverter::UpdateSWSContext()
{
	if (m_pSwsContext && m_FProps.pftype != PFType_RGB && (m_out_pixfmt == PixFmt_RGB32 || m_out_pixfmt == PixFmt_RGB48)) {
		// needed for correct YUV to RGB conversion
		int *inv_tbl = nullptr, *tbl = nullptr;
		int srcRange, dstRange, brightness, contrast, saturation;
		int ret = sws_getColorspaceDetails(m_pSwsContext, &inv_tbl, &srcRange, &tbl, &dstRange, &brightness, &contrast, &saturation);
		if (ret >= 0) {
			srcRange = (m_FProps.colorrange == AVCOL_RANGE_JPEG) ? 1 : 0;
			dstRange = m_dstRGBRange;

			inv_tbl = (int*)sws_getCoefficients(m_FProps.colorspace);
			tbl = (int*)sws_getCoefficients(AVCOL_SPC_RGB);

			ret = sws_setColorspaceDetails(m_pSwsContext, inv_tbl, srcRange, tbl, dstRange, brightness, contrast, saturation);
		}
	}
}

void CFormatConverter::SetConvertFunc()
{
	m_pConvertFn = nullptr;
	m_RequiredAlignment = 16;

	if (m_FProps.avpixfmt == AV_PIX_FMT_NONE || m_out_pixfmt == PixFmt_None) {
		return;
	}

#ifdef DEBUG
	{
		auto swof = GetSWOF(m_out_pixfmt);
		if (m_FProps.avpfdesc && swof) {
			DLog(L"CFormatConverter::SetConvertFunc : %hs -> %s", m_FProps.avpfdesc->name, swof->desc.name);
		}
	}
#endif // DEBUG

	// optimized direct function
	if (m_bDirect && CPUInfo::HaveSSE4()) {
		if (m_FProps.pftype == PFType_NV12) {
			if (m_out_pixfmt == PixFmt_NV12) {
				m_pConvertFn = &CFormatConverter::plane_copy_direct_nv12_sse4;
			}
			else if (m_out_pixfmt == PixFmt_YV12) {
				m_pConvertFn = &CFormatConverter::convert_nv12_yv12_direct_sse4;
				m_RequiredAlignment = 32;
			}
		}
		else if (m_FProps.pftype == PFType_P01x) {
			if (m_out_pixfmt == PixFmt_P010 || m_out_pixfmt == PixFmt_P016) {
				m_pConvertFn = &CFormatConverter::plane_copy_direct_nv12_sse4;
			}
			else if (m_out_pixfmt == PixFmt_NV12) {
				m_pConvertFn = &CFormatConverter::convert_p010_nv12_direct_sse4;
			}
		}
		else if (m_FProps.pftype == PFType_Y21x) {
			if (m_out_pixfmt == PixFmt_P210 || m_out_pixfmt == PixFmt_P216) {
				m_pConvertFn = &CFormatConverter::convert_y210_p210_direct_sse4;
			}
		}
		else if (m_FProps.avpixfmt == AV_PIX_FMT_YUYV422) {
			if (m_out_pixfmt == PixFmt_YUY2) {
				m_pConvertFn = &CFormatConverter::plane_copy_direct_sse4;
			}
			else if (m_out_pixfmt == PixFmt_YV16) {
				//m_pConvertFn = &CFormatConverter::convert_yuy2_yv16_direct_sse4;
			}
		}
		else if (m_FProps.avpixfmt == AV_PIX_FMT_VUYX && m_out_pixfmt == PixFmt_AYUV) {
			m_pConvertFn = &CFormatConverter::plane_copy_direct_sse4;
		}
		else if (m_FProps.avpixfmt == AV_PIX_FMT_XV30 && m_out_pixfmt == PixFmt_Y410) {
			m_pConvertFn = &CFormatConverter::plane_copy_direct_sse4;
		}
		else if (m_FProps.avpixfmt == AV_PIX_FMT_XV36 && m_out_pixfmt == PixFmt_Y416) {
			m_pConvertFn = &CFormatConverter::plane_copy_direct_sse4;
		}
	}

	if (m_pConvertFn) {
		DLog("CFormatConverter::SetConvertFunc : SSE4 direct function has been selected");
		return;
	}

	// optimized function
	switch (m_out_pixfmt) {
	case PixFmt_NV12:
		if (m_FProps.pftype == PFType_NV12) {
			m_pConvertFn = &CFormatConverter::plane_copy_sse2;
			m_RequiredAlignment = 0;
		}
		else if (m_FProps.pftype == PFType_YUV420) {
			m_pConvertFn = &CFormatConverter::convert_yuv420_nv12;
			m_RequiredAlignment = 32;
		}
		else if (m_FProps.pftype == PFType_P01x) {
			m_pConvertFn = &CFormatConverter::convert_p010_nv12_sse2;
		}
		else if (m_FProps.pftype == PFType_YUV420Px) {
			m_pConvertFn = &CFormatConverter::convert_yuv_yv_nv12_dither_le;
			m_RequiredAlignment = 32;
		}
		break;
	case PixFmt_YV12:
		if (m_FProps.pftype == PFType_NV12) {
			m_pConvertFn = &CFormatConverter::convert_nv12_yv12;
			m_RequiredAlignment = 32;
		}
#if (0) // disabled because not increase performance
		else if (m_FProps.pftype == PFType_YUV420) {
			pConvertFn = &CFormatConverter::convert_yuv_yv;
			m_RequiredAlignment = 0;
		}
#endif
		else if (m_FProps.pftype == PFType_YUV420Px) {
			m_pConvertFn = &CFormatConverter::convert_yuv_yv_nv12_dither_le;
			m_RequiredAlignment = 32;
		}
		break;
	case PixFmt_YUY2:
		if (m_FProps.avpixfmt == AV_PIX_FMT_YUYV422) {
			m_pConvertFn = &CFormatConverter::plane_copy_sse2;
			m_RequiredAlignment = 0;
		}
		else if (m_FProps.pftype == PFType_YUV422Px) {
			m_pConvertFn = &CFormatConverter::convert_yuv422_yuy2_uyvy_dither_le;
			m_RequiredAlignment = 8;
		}
		else if (m_FProps.pftype == PFType_YUV420
			|| (m_FProps.pftype == PFType_YUV420Px && m_FProps.lumabits <= 14)
			|| m_FProps.pftype == PFType_NV12) {
			m_pConvertFn = &CFormatConverter::convert_yuv420_yuy2;
			m_RequiredAlignment = 8;
		}
		break;
	case PixFmt_YV16:
		if (m_FProps.pftype == PFType_YUV422Px) {
			m_pConvertFn = &CFormatConverter::convert_yuv_yv_nv12_dither_le;
			m_RequiredAlignment = 32;
		}
#if (0) // disabled because not increase performance
		else if (m_FProps.pftype == PFType_YUV422) {
			m_pConvertFn = &CFormatConverter::convert_yuv_yv;
			m_RequiredAlignment = 0;
		}
#endif
		else if (m_FProps.avpixfmt == AV_PIX_FMT_YUYV422) {
			m_pConvertFn = &CFormatConverter::convert_yuy2_yv16_sse2;
		}
		break;
	case PixFmt_AYUV:
		if (m_FProps.avpixfmt == AV_PIX_FMT_VUYX) {
			m_pConvertFn = &CFormatConverter::plane_copy_sse2;
			m_RequiredAlignment = 0;
		}
		else if (m_FProps.pftype == PFType_YUV444Px) {
			m_pConvertFn = &CFormatConverter::convert_yuv444_ayuv_dither_le;
		}
		break;
	case PixFmt_YV24:
		if (m_FProps.pftype == PFType_YUV444Px) {
			m_pConvertFn = &CFormatConverter::convert_yuv_yv_nv12_dither_le;
			m_RequiredAlignment = 32;
		}
		else if (m_FProps.pftype == PFType_YUV444) {
			m_pConvertFn = &CFormatConverter::convert_yuv_yv;
			m_RequiredAlignment = 0;
		}
		break;
	case PixFmt_P010:
	case PixFmt_P016:
		if (m_FProps.pftype == PFType_P01x) {
			m_pConvertFn = &CFormatConverter::plane_copy_sse2;
			m_RequiredAlignment = 0;
		}
		else if (m_FProps.pftype == PFType_YUV420Px) {
			m_pConvertFn = &CFormatConverter::convert_yuv420_px1x_le;
		}
		break;
	case PixFmt_P210:
	case PixFmt_P216:
		if (m_FProps.pftype == PFType_Y21x && CPUInfo::HaveSSE4()) {
			m_pConvertFn = &CFormatConverter::convert_y210_p210_sse4;
			m_RequiredAlignment = 4;
		}
		else if (m_FProps.pftype == PFType_YUV422Px) {
			m_pConvertFn = &CFormatConverter::convert_yuv420_px1x_le;
		}
		break;
	case PixFmt_Y410:
		if (m_FProps.avpixfmt == AV_PIX_FMT_XV30) {
			m_pConvertFn = &CFormatConverter::plane_copy_sse2;
			m_RequiredAlignment = 0;
		}
		else if (m_FProps.pftype == PFType_YUV444Px && m_FProps.lumabits <= 10) {
			m_pConvertFn = &CFormatConverter::convert_yuv444_y410;
		}
		break;
	case PixFmt_Y416:
		if (m_FProps.avpixfmt == AV_PIX_FMT_XV36) {
			m_pConvertFn = &CFormatConverter::plane_copy_sse2;
			m_RequiredAlignment = 0;
		}
		break;
	case PixFmt_RGB32:
		switch (m_FProps.pftype) {
		case PFType_YUV420:
		case PFType_YUV420Px:
		case PFType_YUV422:
		case PFType_YUV422Px:
		case PFType_YUV444:
		case PFType_YUV444Px:
		case PFType_NV12:
		case PFType_P01x:
			m_pConvertFn = &CFormatConverter::convert_yuv_rgb;
			m_RequiredAlignment = 4;
		}
		break;
	}

	if (m_pConvertFn) {
		DLog("CFormatConverter::SetConvertFunc : optimized function has been selected");
		return;
	}

	m_pConvertFn = &CFormatConverter::ConvertGeneric;

	DLog("CFormatConverter::SetConvertFunc : swscale has been selected");
}

void CFormatConverter::UpdateInput(const AVFrame* pFrame)
{
	if (m_FProps.avpixfmt != (AVPixelFormat)pFrame->format|| pFrame->width != m_FProps.width || pFrame->height != m_FProps.height) {
		// update the basic properties
		m_FProps.avpixfmt = (AVPixelFormat)pFrame->format;
		m_FProps.width = pFrame->width;
		m_FProps.height = pFrame->height;

		// update the additional properties (updated only when changing basic properties)
		m_FProps.avpfdesc = av_pix_fmt_desc_get(m_FProps.avpixfmt);
		m_FProps.lumabits = GetLumaBits(m_FProps.avpfdesc);
		m_FProps.pftype   = GetPixFmtType(m_FProps.avpixfmt, m_FProps.avpfdesc);
		m_FProps.colorspace = pFrame->colorspace;
		m_FProps.colorrange = pFrame->color_range;

		Cleanup();
		SetConvertFunc();
	}
}

void CFormatConverter::UpdateOutput(const GUID& subtype, const BITMAPINFOHEADER* pBIH)
{
	MPCPixelFormat out_pixfmt = GetPixFormat(subtype);
	if (out_pixfmt != m_out_pixfmt) {
		m_out_pixfmt = out_pixfmt;

		Cleanup();
		SetConvertFunc();
	}

	m_dstStride   = pBIH->biWidth;
	m_planeHeight = abs(pBIH->biHeight);
	m_OutHeight   = pBIH->biHeight;
}

void CFormatConverter::SetOptions(const int rgblevels)
{
	m_dstRGBRange = (rgblevels == 1) ? 0 : 1;

	UpdateSWSContext();
}

bool CFormatConverter::Converting(BYTE* dst, const AVFrame* pFrame)
{
	UpdateInput(pFrame);

	ptrdiff_t srcStride[4];
	for (int i = 0; i < 4; i++) {
		srcStride[i] = pFrame->linesize[i];
	}

	const uint8_t* srcData[4];
	for (int i = 0; i < 4; i++) {
		srcData[i] = pFrame->data[i];
	}

	return Converting(dst, srcData, srcStride);
}

bool CFormatConverter::Converting(BYTE* dst, const uint8_t* (&srcData)[4], const ptrdiff_t(&srcStride)[4])
{
	if (!m_pConvertFn) {
		return false;
	}

	const SW_OUT_FMT& swof = s_sw_formats[m_out_pixfmt];

	// From LAVVideo...
	uint8_t *out = dst;
	int outStride = m_dstStride;
	// Check if we have proper pixel alignment and the dst memory is actually aligned
	if (m_RequiredAlignment && FFALIGN(m_dstStride, m_RequiredAlignment) != m_dstStride || ((uintptr_t)dst % 16u)) {
		outStride = FFALIGN(outStride, m_RequiredAlignment);
		size_t requiredSize = (outStride * m_planeHeight * swof.bpp) >> 3;

		uint8_t* pTmpBuffer = (uint8_t*)GetTempBuffer(requiredSize);
		if (pTmpBuffer == nullptr) {
			return false;
		}
		out = pTmpBuffer;
	}

	uint8_t*  dstArray[4]       = { nullptr };
	ptrdiff_t dstStrideArray[4] = { 0 };
	ptrdiff_t byteStride        = outStride * swof.desc.packsize;

	dstArray[0] = out;
	dstStrideArray[0] = byteStride;
	for (int i = 1; i < swof.desc.planes; ++i) {
		dstArray[i] = dstArray[i - 1] + dstStrideArray[i - 1] * (m_planeHeight / swof.planeHeight[i - 1]);
		dstStrideArray[i] = byteStride / swof.planeWidth[i];
	}

	(this->*m_pConvertFn)(srcData, srcStride, dstArray, m_FProps.width, m_FProps.height, dstStrideArray);

	if (out != dst) {
		int line = 0;

		// Copy first plane
		const size_t widthBytes        = m_FProps.width * swof.desc.packsize;
		const ptrdiff_t srcStrideBytes = outStride * swof.desc.packsize;
		const ptrdiff_t dstStrideBytes = m_dstStride * swof.desc.packsize;
		for (line = 0; line < m_FProps.height; ++line) {
			memcpy(dst, out, widthBytes);
			out += srcStrideBytes;
			dst += dstStrideBytes;
		}
		dst += (m_planeHeight - m_FProps.height) * dstStrideBytes;

		for (int plane = 1; plane < swof.desc.planes; ++plane) {
			const size_t planeWidth        = widthBytes      / swof.planeWidth[plane];
			const int activePlaneHeight    = m_FProps.height / swof.planeHeight[plane];
			const int totalPlaneHeight     = m_planeHeight   / swof.planeHeight[plane];
			const ptrdiff_t srcPlaneStride = srcStrideBytes  / swof.planeWidth[plane];
			const ptrdiff_t dstPlaneStride = dstStrideBytes  / swof.planeWidth[plane];
			for (line = 0; line < activePlaneHeight; ++line) {
				memcpy(dst, out, planeWidth);
				out += srcPlaneStride;
				dst += dstPlaneStride;
			}
			dst += (totalPlaneHeight - activePlaneHeight) * dstPlaneStride;
		}
	}

	return true;
}

void CFormatConverter::Cleanup()
{
	if (m_pSwsContext) {
		sws_freeContext(m_pSwsContext);
		m_pSwsContext = nullptr;
	}

	av_freep(&m_pTempBuffer);
	m_nTempBufferSize = 0;

	if (m_rgbCoeffs) {
		_aligned_free(m_rgbCoeffs);
		m_rgbCoeffs = nullptr;
	}

	m_pConvertFn = nullptr;
}

void CFormatConverter::Clear()
{
	m_FProps = {};
}

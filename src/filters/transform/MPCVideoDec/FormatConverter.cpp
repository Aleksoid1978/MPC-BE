/*
 * (C) 2014-2024 see Authors.txt
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

SW_OUT_FMT s_sw_formats[] = {
	//name         biCompression       subtype                                            av_pix_fmt   chroma_w chroma_h actual_bpp luma_bits
	// YUV 8 bit
	{L"NV12",      FCC('NV12'),      &MEDIASUBTYPE_NV12,      12, 1, 2, {1,2},   {1,1},   AV_PIX_FMT_NV12,        1, 1, 12,  8}, // PixFmt_NV12
	{L"YV12",      FCC('YV12'),      &MEDIASUBTYPE_YV12,      12, 1, 3, {1,2,2}, {1,2,2}, AV_PIX_FMT_YUV420P,     1, 1, 12,  8}, // PixFmt_YV12
	{L"YUY2",      FCC('YUY2'),      &MEDIASUBTYPE_YUY2,      16, 2, 0, {1},     {1},     AV_PIX_FMT_YUYV422,     1, 0, 16,  8}, // PixFmt_YUY2
	{L"YV16",      FCC('YV16'),      &MEDIASUBTYPE_YV16,      16, 1, 3, {1,1,1}, {1,2,2}, AV_PIX_FMT_YUV422P,     1, 0, 16,  8}, // PixFmt_YV16
	{L"AYUV",      FCC('AYUV'),      &MEDIASUBTYPE_AYUV,      32, 4, 0, {1},     {1},     AV_PIX_FMT_YUV444P,     0, 0, 24,  8}, // PixFmt_AYUV
	{L"YV24",      FCC('YV24'),      &MEDIASUBTYPE_YV24,      24, 1, 3, {1,1,1}, {1,1,1}, AV_PIX_FMT_YUV444P,     0, 0, 24,  8}, // PixFmt_YV24
	// YUV 10 bit
	{L"P010",      FCC('P010'),      &MEDIASUBTYPE_P010,      24, 2, 2, {1,2},   {1,1},   AV_PIX_FMT_YUV420P16LE, 1, 1, 15, 10}, // PixFmt_P010
	{L"P210",      FCC('P210'),      &MEDIASUBTYPE_P210,      32, 2, 2, {1,1},   {1,1},   AV_PIX_FMT_YUV422P16LE, 1, 0, 20, 10}, // PixFmt_P210
	{L"Y410",      FCC('Y410'),      &MEDIASUBTYPE_Y410,      32, 4, 0, {1},     {1},     AV_PIX_FMT_YUV444P10LE, 0, 0, 30, 10}, // PixFmt_Y410
	// YUV 16 bit
	{L"P016",      FCC('P016'),      &MEDIASUBTYPE_P016,      24, 2, 2, {1,2},   {1,1},   AV_PIX_FMT_YUV420P16LE, 1, 1, 24, 16}, // PixFmt_P016
	{L"P216",      FCC('P216'),      &MEDIASUBTYPE_P216,      32, 2, 2, {1,1},   {1,1},   AV_PIX_FMT_YUV422P16LE, 1, 0, 32, 16}, // PixFmt_P216
	{L"Y416",      FCC('Y416'),      &MEDIASUBTYPE_Y416,      64, 8, 0, {1},     {1},     AV_PIX_FMT_YUV444P16LE, 0, 0, 48, 16}, // PixFmt_Y416
	{L"YUV444P16", FOURCC_YUV444P16, &MEDIASUBTYPE_YUV444P16, 48, 2, 3, {1,1,1}, {1,1,1}, AV_PIX_FMT_YUV444P16LE, 0, 0, 48, 16}, // PixFmt_YUV444P16
	// RGB
	{L"RGB32",     BI_RGB,           &MEDIASUBTYPE_RGB32,     32, 4, 0, {1},     {1},     AV_PIX_FMT_BGRA,        0, 0, 24,  8}, // PixFmt_RGB32
	{L"RGB48",     FOURCC_RGB48,     &MEDIASUBTYPE_RGB48,     48, 6, 0, {1},     {1},     AV_PIX_FMT_RGB48LE,     0, 0, 48, 16}, // PixFmt_RGB48
	// PS:
	// AV_PIX_FMT_YUV444P not equal to AYUV, but is used as an intermediate format.
	// AV_PIX_FMT_YUV420P16LE not equal to P010, but is used as an intermediate format.
	// AV_PIX_FMT_YUV422P16LE not equal to P210, but is used as an intermediate format.
};

const SW_OUT_FMT* GetSWOF(int pixfmt)
{
	if (pixfmt < 0 || pixfmt >= PixFmt_count) {
		return nullptr;
	}
	return &s_sw_formats[pixfmt];
}

LPCWSTR GetChromaSubsamplingStr(AVPixelFormat av_pix_fmt)
{
	int h_shift, v_shift;

	if (0 == av_pix_fmt_get_chroma_sub_sample(av_pix_fmt, &h_shift, &v_shift)) {
		if (h_shift == 0 && v_shift == 0) {
			return L"4:4:4";
		} else if (h_shift == 0 && v_shift == 1) {
			return L"4:4:0";
		} else if (h_shift == 1 && v_shift == 0) {
			return L"4:2:2";
		} else if (h_shift == 1 && v_shift == 1) {
			return L"4:2:0";
		} else if (h_shift == 2 && v_shift == 0) {
			return L"4:1:1";
		} else if (h_shift == 2 && v_shift == 2) {
			return L"4:1:0";
		}
	}

	return L"";
}

int GetLumaBits(AVPixelFormat av_pix_fmt)
{
	const AVPixFmtDescriptor* pfdesc = av_pix_fmt_desc_get(av_pix_fmt);

	return (pfdesc ? pfdesc->comp[0].depth : 0);
}

MPCPixelFormat GetPixFormat(GUID& subtype)
{
	for (int i = 0; i < PixFmt_count; i++) {
		if (*s_sw_formats[i].subtype == subtype) {
			return (MPCPixelFormat)i;
		}
	}

	return PixFmt_None;
}

MPCPixelFormat GetPixFormat(AVPixelFormat av_pix_fmt)
{
	for (int i = 0; i < PixFmt_count; i++) {
		if (s_sw_formats[i].av_pix_fmt == av_pix_fmt) {
			return (MPCPixelFormat)i;
		}
	}

	return PixFmt_None;
}

MPCPixelFormat GetPixFormat(DWORD biCompression)
{
	for (int i = 0; i < PixFmt_count; i++) {
		if (s_sw_formats[i].biCompression == biCompression) {
			return (MPCPixelFormat)i;
		}
	}

	return PixFmt_None;
}

MPCPixFmtType GetPixFmtType(AVPixelFormat av_pix_fmt)
{
	const AVPixFmtDescriptor* pfdesc = av_pix_fmt_desc_get(av_pix_fmt);
	if (!pfdesc) {
		return PFType_unspecified;
	}

	if (av_pix_fmt == AV_PIX_FMT_NV12) {
		return PFType_NV12;
	} else if (av_pix_fmt == AV_PIX_FMT_P010 || av_pix_fmt == AV_PIX_FMT_P016) {
		return PFType_P01x;
	}

	int lumabits = pfdesc->comp[0].depth;

	if (pfdesc->flags & (AV_PIX_FMT_FLAG_RGB|AV_PIX_FMT_FLAG_PAL)) {
		return PFType_RGB;
	}

	if (lumabits < 8 || lumabits > 16 || pfdesc->nb_components != 3 + (pfdesc->flags & AV_PIX_FMT_FLAG_ALPHA ? 1 : 0)) {
		return PFType_unspecified;
	}

	if ((pfdesc->flags & ~AV_PIX_FMT_FLAG_ALPHA) == AV_PIX_FMT_FLAG_PLANAR) {
		// must be planar type, ignore alpha channel, other flags are forbidden

		if (pfdesc->log2_chroma_w == 1 && pfdesc->log2_chroma_h == 1) {
			return lumabits == 8 ? PFType_YUV420 : PFType_YUV420Px;
		}

		if (pfdesc->log2_chroma_w == 1 && pfdesc->log2_chroma_h == 0) {
			return lumabits == 8 ? PFType_YUV422 : PFType_YUV422Px;
		}

		if (pfdesc->log2_chroma_w == 0 && pfdesc->log2_chroma_h == 0) {
			return lumabits == 8 ? PFType_YUV444 : PFType_YUV444Px;
		}
	}

	return PFType_unspecified;
}

// CFormatConverter

CFormatConverter::CFormatConverter()
{
	ASSERT(PixFmt_count == std::size(s_sw_formats));

	m_FProps.avpixfmt   = AV_PIX_FMT_NONE;
	m_FProps.width      = 0;
	m_FProps.height     = 0;
	m_FProps.lumabits   = 0;
	m_FProps.pftype     = PFType_unspecified;
	m_FProps.colorspace = AVCOL_SPC_UNSPECIFIED;
	m_FProps.colorrange = AVCOL_RANGE_UNSPECIFIED;

	m_NumThreads = std::clamp(CPUInfo::GetProcessorNumber() / 2, 1uL, 8uL);
}

CFormatConverter::~CFormatConverter()
{
	Cleanup();
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

	m_pSwsContext = sws_getCachedContext(
						nullptr,
						m_FProps.width,
						m_FProps.height,
						m_FProps.avpixfmt,
						m_FProps.width,
						m_FProps.height,
						swof.av_pix_fmt,
						SWS_BILINEAR | SWS_ACCURATE_RND | SWS_FULL_CHR_H_INP | SWS_PRINT_INFO,
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
	if (m_pSwsContext) {
		int *inv_tbl = nullptr, *tbl = nullptr;
		int srcRange, dstRange, brightness, contrast, saturation;
		int ret = sws_getColorspaceDetails(m_pSwsContext, &inv_tbl, &srcRange, &tbl, &dstRange, &brightness, &contrast, &saturation);
		if (ret >= 0) {
			if (m_FProps.pftype == PFType_RGB || m_FProps.colorrange == AVCOL_RANGE_JPEG) {
				srcRange = 1;
			}

			if (m_out_pixfmt == PixFmt_RGB32 || m_out_pixfmt == PixFmt_RGB48) {
				dstRange = m_dstRGBRange;
			}
			else if (m_FProps.colorrange == AVCOL_RANGE_JPEG) {
				dstRange = 1;
			}

			auto isAnyRGB = [](enum AVPixelFormat pix_fmt) {
				const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(pix_fmt);
				ASSERT(desc);
				return (desc->flags & AV_PIX_FMT_FLAG_RGB) || pix_fmt == AV_PIX_FMT_MONOBLACK || pix_fmt == AV_PIX_FMT_MONOWHITE;
			};

			if (isAnyRGB(m_FProps.avpixfmt) || isAnyRGB(s_sw_formats[m_out_pixfmt].av_pix_fmt)) {
				// SWS_CS_* does not fully comply with the AVCOL_SPC_*, but it is well handled in the libswscale.
				inv_tbl = (int *)sws_getCoefficients(m_FProps.colorspace);
			}

			ret = sws_setColorspaceDetails(m_pSwsContext, inv_tbl, srcRange, tbl, dstRange, brightness, contrast, saturation);
		}
	}
}

void CFormatConverter::SetConvertFunc()
{
	m_pConvertFn = &CFormatConverter::ConvertGeneric;
	m_RequiredAlignment = 16;

	switch (m_out_pixfmt) {
		case PixFmt_AYUV:
			if (m_FProps.pftype == PFType_YUV444Px) {
				m_pConvertFn = &CFormatConverter::convert_yuv444_ayuv_dither_le;
			}
			break;
		case PixFmt_P010:
		case PixFmt_P016:
			if (m_FProps.pftype == PFType_YUV420Px) {
				m_pConvertFn = &CFormatConverter::convert_yuv420_px1x_le;
			}
			break;
		case PixFmt_Y410:
			if (m_FProps.pftype == PFType_YUV444Px && m_FProps.lumabits <= 10) {
				m_pConvertFn = &CFormatConverter::convert_yuv444_y410;
			}
			break;
		case PixFmt_P210:
		case PixFmt_P216:
			if (m_FProps.pftype == PFType_YUV422Px) {
				m_pConvertFn = &CFormatConverter::convert_yuv420_px1x_le;
			}
			break;
		case PixFmt_YUY2:
			if (m_FProps.pftype == PFType_YUV422Px) {
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
		case PixFmt_NV12:
			if (m_FProps.pftype == PFType_NV12) {
				m_pConvertFn = &CFormatConverter::plane_copy_sse2;
				break;
			}
			else if (m_FProps.pftype == PFType_YUV420) {
				m_pConvertFn = &CFormatConverter::convert_yuv420_nv12;
				m_RequiredAlignment = 32;
			}
			else if (m_FProps.pftype == PFType_P01x) {
				m_pConvertFn = &CFormatConverter::convert_p010_nv12_sse2;
			}
		case PixFmt_YV12:
			if (m_FProps.pftype == PFType_YUV420Px) {
				m_pConvertFn = &CFormatConverter::convert_yuv_yv_nv12_dither_le;
				m_RequiredAlignment = 32;
			}
			else if (m_FProps.pftype == PFType_NV12) {
				m_pConvertFn = &CFormatConverter::convert_nv12_yv12;
				m_RequiredAlignment = 32;
			}
#if (0) // disabled because not increase performance
			else if (m_FProps.pftype == PFType_YUV420) {
				pConvertFn = &CFormatConverter::convert_yuv_yv;
			}
#endif
			break;
		case PixFmt_YV16:
			if (m_FProps.pftype == PFType_YUV422Px) {
				m_pConvertFn = &CFormatConverter::convert_yuv_yv_nv12_dither_le;
				m_RequiredAlignment = 32;
			}
#if (0) // disabled because not increase performance
			else if (m_FProps.pftype == PFType_YUV422) {
				pConvertFn = &CFormatConverter::convert_yuv_yv;
			}
#endif
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

	if (CPUInfo::HaveSSE4()) {
		if (m_FProps.pftype == PFType_NV12) {
			if (m_out_pixfmt == PixFmt_NV12) {
				m_pConvertFn = &CFormatConverter::plane_copy_direct_sse4;
			}
			else if (m_out_pixfmt == PixFmt_YV12) {
				m_pConvertFn = &CFormatConverter::convert_nv12_yv12_direct_sse4;
			}
		}
		else if (m_FProps.pftype == PFType_P01x) {
			if (m_out_pixfmt == PixFmt_P010 || m_out_pixfmt == PixFmt_P016) {
				m_pConvertFn = &CFormatConverter::plane_copy_direct_sse4;
			}
			else if (m_out_pixfmt == PixFmt_NV12) {
				m_pConvertFn = &CFormatConverter::convert_p010_nv12_direct_sse4;
			}
		}
	}
}

void CFormatConverter::UpdateOutput(MPCPixelFormat out_pixfmt, int dstStride, int planeHeight)
{
	if (out_pixfmt != m_out_pixfmt) {
		Cleanup();
		m_out_pixfmt = out_pixfmt;
	}

	m_dstStride   = dstStride;
	m_planeHeight = planeHeight;
}

void CFormatConverter::UpdateOutput2(DWORD biCompression, LONG biWidth, LONG biHeight)
{
	UpdateOutput(GetPixFormat(biCompression), biWidth, abs(biHeight));

	m_OutHeight = biHeight;
}

void CFormatConverter::SetOptions(int rgblevels)
{
	m_dstRGBRange = (rgblevels == 1) ? 0 : 1;

	UpdateSWSContext();
}

bool CFormatConverter::Converting(BYTE* dst, AVFrame* pFrame)
{
	if (FormatChanged(&m_FProps.avpixfmt, (AVPixelFormat*)&pFrame->format)
			|| pFrame->width != m_FProps.width || pFrame->height != m_FProps.height) {
		// update the basic properties
		m_FProps.avpixfmt   = (AVPixelFormat)pFrame->format;
		m_FProps.width      = pFrame->width;
		m_FProps.height     = pFrame->height;

		// update the additional properties (updated only when changing basic properties)
		m_FProps.lumabits   = GetLumaBits(m_FProps.avpixfmt);
		m_FProps.pftype     = GetPixFmtType(m_FProps.avpixfmt);
		m_FProps.colorspace = pFrame->colorspace;
		m_FProps.colorrange = pFrame->color_range;

		Cleanup();
	}

	if (!m_pConvertFn) {
		SetConvertFunc();
	}

	const SW_OUT_FMT& swof = s_sw_formats[m_out_pixfmt];

	// From LAVVideo...
	uint8_t *out = dst;
	int outStride = m_dstStride;
	// Check if we have proper pixel alignment and the dst memory is actually aligned
	if (m_RequiredAlignment && FFALIGN(m_dstStride, m_RequiredAlignment) != m_dstStride || ((uintptr_t)dst % 16u)) {
		outStride = FFALIGN(outStride, m_RequiredAlignment);
		size_t requiredSize = (outStride * m_planeHeight * swof.bpp) >> 3;
		if (requiredSize > m_nAlignedBufferSize) {
			av_freep(&m_pAlignedBuffer);
			m_nAlignedBufferSize = requiredSize;
			m_pAlignedBuffer = (uint8_t*)av_malloc(m_nAlignedBufferSize + AV_INPUT_BUFFER_PADDING_SIZE);
		}
		out = m_pAlignedBuffer;
	}

	uint8_t*  dstArray[4]       = { nullptr };
	ptrdiff_t dstStrideArray[4] = { 0 };
	ptrdiff_t byteStride        = outStride * swof.codedbytes;

	dstArray[0] = out;
	dstStrideArray[0] = byteStride;
	for (int i = 1; i < swof.planes; ++i) {
		dstArray[i] = dstArray[i - 1] + dstStrideArray[i - 1] * (m_planeHeight / swof.planeHeight[i - 1]);
		dstStrideArray[i] = byteStride / swof.planeWidth[i];
	}

	ptrdiff_t srcStride[4];
	for (int i = 0; i < 4; i++) {
		srcStride[i] = pFrame->linesize[i];
	}

	(this->*m_pConvertFn)(pFrame->data, srcStride, dstArray, m_FProps.width, m_FProps.height, dstStrideArray);

	if (out != dst) {
		int line = 0;

		// Copy first plane
		const size_t widthBytes        = m_FProps.width * swof.codedbytes;
		const ptrdiff_t srcStrideBytes = outStride * swof.codedbytes;
		const ptrdiff_t dstStrideBytes = m_dstStride * swof.codedbytes;
		for (line = 0; line < m_FProps.height; ++line) {
			memcpy(dst, out, widthBytes);
			out += srcStrideBytes;
			dst += dstStrideBytes;
		}
		dst += (m_planeHeight - m_FProps.height) * dstStrideBytes;

		for (int plane = 1; plane < swof.planes; ++plane) {
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

	av_freep(&m_pAlignedBuffer);
	m_nAlignedBufferSize = 0;

	if (m_rgbCoeffs) {
		_aligned_free(m_rgbCoeffs);
		m_rgbCoeffs = nullptr;
	}

	m_pConvertFn = nullptr;
}

bool CFormatConverter::FormatChanged(AVPixelFormat* fmt1, AVPixelFormat* fmt2)
{
	if (*fmt1 == AV_PIX_FMT_NONE || *fmt2 == AV_PIX_FMT_NONE) {
		return true;
	}
	const AVPixFmtDescriptor* av_pfdesc_fmt1 = av_pix_fmt_desc_get(*fmt1);
	const AVPixFmtDescriptor* av_pfdesc_fmt2 = av_pix_fmt_desc_get(*fmt2);
	if (!av_pfdesc_fmt1 || !av_pfdesc_fmt2) {
		return false;
	}
	return av_pfdesc_fmt1->log2_chroma_h != av_pfdesc_fmt2->log2_chroma_h
			|| av_pfdesc_fmt1->log2_chroma_w != av_pfdesc_fmt2->log2_chroma_w
			|| av_pfdesc_fmt1->nb_components != av_pfdesc_fmt2->nb_components
			|| av_pfdesc_fmt1->comp[0].depth != av_pfdesc_fmt2->comp[0].depth;
}

bool CFormatConverter::DirectCopyPossible(const AVPixelFormat avformat)
{
	return avformat == AV_PIX_FMT_NV12      && m_out_pixfmt == PixFmt_NV12 ||
		   avformat == AV_PIX_FMT_P010      && m_out_pixfmt == PixFmt_P010 ||
		   avformat == AV_PIX_FMT_P016      && m_out_pixfmt == PixFmt_P016 ||
		   avformat == AV_PIX_FMT_YUV444P   && m_out_pixfmt == PixFmt_YV24 ||
		   avformat == AV_PIX_FMT_YUV444P16 && m_out_pixfmt == PixFmt_YUV444P16;
}

void CFormatConverter::Clear()
{
	m_FProps = {};
}

/*
 * (C) 2012-2024 see Authors.txt
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

#include <ExtLib/libpng/png.h>
#include "WicUtils.h"

inline UINT CalcDibRowPitch(const UINT width, const UINT bitcount)
{
	// see DIBWIDTHBYTES in amvideo.h
	return ALIGN(width * bitcount, 32) / 8;
}

static bool SaveDIB_libpng(LPCWSTR filename, BYTE* pData, int level)
{
	BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)pData;
	if (bih->biCompression != BI_RGB || bih->biWidth <= 0 || bih->biHeight == 0) {
		return false;
	}

	int bit_depth = 0; // bits per channel

	switch (bih->biBitCount) {
	case 24:
	case 32:
		bit_depth = 8;
		break;
	case 48:
	case 64:
		bit_depth = 16;
		break;
	default:
		return false;
	}

	FILE* fp;
	if (_wfopen_s(&fp, filename, L"wb") == 0) {
		const unsigned width        = bih->biWidth;
		const unsigned height       = abs(bih->biHeight);
		const unsigned src_linesize = CalcDibRowPitch(width, bih->biBitCount);
		const unsigned dst_linesize = width * 3 * bit_depth / 8;

		png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		png_infop info_ptr = png_create_info_struct(png_ptr);
		png_init_io(png_ptr, fp);

		png_set_bgr(png_ptr);
		png_set_compression_level(png_ptr, level);
		png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, 0, 0);
		png_write_info(png_ptr, info_ptr);
		png_set_swap(png_ptr); // should be after png_write_info

		BYTE* src = pData + sizeof(BITMAPINFOHEADER);
		int src_pitch = src_linesize;

		if (bih->biHeight > 0) {
			// bottom-up bitmap
			src += src_linesize * (height - 1);
			src_pitch = -(int)src_linesize;
		}

		if (bih->biBitCount == 24 || bih->biBitCount == 48) {
			for (unsigned y = 0; y < height; ++y) {
				png_write_row(png_ptr, src);
				src += src_pitch;
			}
		}
		else {
			std::unique_ptr<BYTE[]> row(new(std::nothrow) BYTE[dst_linesize]);
			if (row) {
				if (bih->biBitCount == 32) {
					for (unsigned y = 0; y < height; ++y) {
						uint8_t* src8 = (uint8_t*)src;
						uint8_t* dst8 = (uint8_t*)row.get();
						for (unsigned x = 0; x < width; ++x) {
							*dst8++ = *src8++;
							*dst8++ = *src8++;
							*dst8++ = *src8++;
							src8++;
						}
						png_write_row(png_ptr, row.get());
						src += src_pitch;
					}
				}
				else { // if (bih->biBitCount == 64)
					for (unsigned y = 0; y < height; ++y) {
						uint16_t* src16 = (uint16_t*)src;
						uint16_t* dst16 = (uint16_t*)row.get();
						for (unsigned x = 0; x < width; ++x) {
							*dst16++ = *src16++;
							*dst16++ = *src16++;
							*dst16++ = *src16++;
							src16++;
						}
						png_write_row(png_ptr, row.get());
						src += src_pitch;
					}
				}
			}
		}

		png_write_end(png_ptr, info_ptr);
		fclose(fp);
		png_destroy_write_struct(&png_ptr, &info_ptr);

		return true;
	}

	return false;
}

static bool SaveDIB_WIC(LPCWSTR filename, BYTE* pData, int quality, BYTE* output, size_t& outLen)
{
	BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)pData;
	if (bih->biCompression != BI_RGB || bih->biWidth <= 0 || bih->biHeight == 0) {
		return false;
	}

	WICPixelFormatGUID format = {};

	switch (bih->biBitCount) {
	case 24: format = GUID_WICPixelFormat24bppBGR; break;
	case 32: format = GUID_WICPixelFormat32bppBGR; break;
	case 48: format = GUID_WICPixelFormat48bppBGR; break;
	case 64: format = GUID_WICPixelFormat64bppBGRA; break;
	default:
		return false;
	}

	const UINT dib_pitch = CalcDibRowPitch(bih->biWidth, bih->biBitCount);
	BYTE* src = pData + sizeof(BITMAPINFOHEADER) + bih->biClrUsed * 4;

	if (bih->biHeight > 0) {
		// bottom-up RGB bitmap
		const UINT dib_size = dib_pitch * bih->biHeight;
		BYTE* const data = new(std::nothrow) BYTE[dib_size];
		if (!data) {
			return false;
		}

		src += dib_size - dib_pitch;
		BYTE* dst = data;
		UINT y = bih->biHeight;

		while (y--) {
			memcpy(dst, src, dib_pitch);
			src -= dib_pitch;
			dst += dib_pitch;
		}

		src = data;
	}

	HRESULT hr = WicSaveImage(
		src, dib_pitch,
		bih->biWidth, abs(bih->biHeight),
		format, quality,
		filename,
		output, outLen
	);

	if (bih->biHeight > 0) {
		delete[] src;
	}

	return SUCCEEDED(hr);
}

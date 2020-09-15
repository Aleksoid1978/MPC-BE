/*
 * (C) 2012-2020 see Authors.txt
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

#include <libpng/png.h>
#include "../../../DSUtil/vd.h"
#include "WicUtils.h"

static bool BMPDIB(LPCWSTR filename, BYTE* pData)
{
	BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)pData;

	int width = bih->biWidth;
	int height = abs(bih->biHeight);
	int bpp = bih->biBitCount / 8;
	int sih = sizeof(BITMAPINFOHEADER);
	BYTE *src = pData + sih;

	int bit = 24;
	int stride = (width * bit + 31) / 32 * bpp;
	DWORD len = stride * height;

	std::unique_ptr<BYTE[]> rgb(new(std::nothrow) BYTE[len]);
	if (!rgb) {
		return false;
	}

	BitBltFromRGBToRGB(width, height, rgb.get(), stride, bit, src, width * bpp, bih->biBitCount);

	BITMAPFILEHEADER bfh;
	bfh.bfType = 0x4d42;
	bfh.bfOffBits = sizeof(bfh) + sih;
	bfh.bfSize = bfh.bfOffBits + len;
	bfh.bfReserved1 = bfh.bfReserved2 = 0;

	BITMAPINFO bi = {{sih, width, height, 1, bit, BI_RGB, 0, 0, 0, 0, 0}};

	FILE* fp;
	if (_wfopen_s(&fp, filename, L"wb") == 0) {
		fwrite(&bfh, sizeof(bfh), 1, fp);
		fwrite(&bi.bmiHeader, sih, 1, fp);
		fwrite(rgb.get(), len, 1, fp);
		fclose(fp);

		return true;
	}

	return false;
}

static bool PNGDIB(LPCWSTR filename, BYTE* pData, int level)
{
	BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)pData;
	if (bih->biCompression != BI_RGB || bih->biWidth <= 0 || abs(bih->biHeight) == 0 || bih->biBitCount % 8) {
		return false;
	}

	FILE* fp;
	if (_wfopen_s(&fp, filename, L"wb") == 0) {
		const unsigned width = bih->biWidth;
		const unsigned height = abs(bih->biHeight);
		const int bit_depth = (bih->biBitCount == 48 || bih->biBitCount == 64) ? 16 : 8; // bits per channel
		const unsigned src_bpp = bih->biBitCount / 8;
		const unsigned dst_bpp = 3 * bit_depth / 8;

		png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		png_infop info_ptr = png_create_info_struct(png_ptr);
		png_init_io(png_ptr, fp);

		png_set_compression_level(png_ptr, level);
		png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, 0, 0);
		png_write_info(png_ptr, info_ptr);

		BYTE* row_ptr = (png_bytep)malloc(width * dst_bpp);
		const unsigned src_pitch = width * src_bpp;
		BYTE* src = pData + sizeof(BITMAPINFOHEADER) + src_pitch * (height - 1);

		for (unsigned y = 0; y < height; ++y) {
			if (bit_depth == 16) {
				uint16_t* src16 = (uint16_t*)src;
				uint16_t* dst16 = (uint16_t*)row_ptr;
				for (unsigned x = 0; x < width; ++x) {
					*dst16++ = _byteswap_ushort(src16[2]);
					*dst16++ = _byteswap_ushort(src16[1]);
					*dst16++ = _byteswap_ushort(src16[0]);
					src16 += 4;
				}
			}
			else {
				uint8_t* src8 = (uint8_t*)src;
				uint8_t* dst8 = (uint8_t*)row_ptr;
				for (unsigned x = 0; x < width; ++x) {
					*dst8++ = src8[2];
					*dst8++ = src8[1];
					*dst8++ = src8[0];
					src8 += 4;
				}
			}
			png_write_row(png_ptr, row_ptr);
			src -= src_pitch;
		}
		png_write_end(png_ptr, info_ptr);

		fclose(fp);
		free(row_ptr);
		png_destroy_write_struct(&png_ptr, &info_ptr);

		return true;
	}

	return false;
}

static bool WICDIB(LPCWSTR filename, BYTE* pData, int quality, BYTE* output, size_t& outLen)
{
	BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)pData;

	const WICPixelFormatGUID format = (bih->biBitCount == 32) ? GUID_WICPixelFormat32bppBGR : GUID_WICPixelFormat24bppBGR;
	const UINT pitch = bih->biWidth * bih->biBitCount / 8;
	BYTE* src = pData + sizeof(BITMAPINFOHEADER) + bih->biClrUsed * 4;

	if (bih->biHeight > 0) {
		const UINT len = pitch * bih->biHeight;
		BYTE* const data = new(std::nothrow) BYTE[len];
		if (!data) {
			return false;
		}

		src += len - pitch;
		BYTE* dst = data;
		UINT y = bih->biHeight;

		while (y--) {
			memcpy(dst, src, pitch);
			src -= pitch;
			dst += pitch;
		}

		src = data;
	}

	HRESULT hr = WicSaveImage(
		src, pitch, 
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

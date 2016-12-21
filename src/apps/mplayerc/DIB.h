/*
 * (C) 2012-2016 see Authors.txt
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

using namespace Gdiplus;

static int GetEncoderClsid(CStringW format, CLSID* pClsid)
{
	UINT num = 0, size = 0;
	GetImageEncodersSize(&num, &size);
	if (size == 0) {
		return -1;
	}
	ImageCodecInfo* pImageCodecInfo = (ImageCodecInfo*)malloc(size);
	GetImageEncoders(num, size, pImageCodecInfo);
	for (UINT j = 0; j < num; j++) {
		if (!wcscmp(pImageCodecInfo[j].MimeType, format)) {
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;
		}
	}
	free(pImageCodecInfo);
	return -1;
}

static void GdiplusConvert(Bitmap* bm, CStringW fn, CStringW format, ULONG quality, bool type, BYTE** pBuf, size_t* pSize)
{
	CLSID encoderClsid = CLSID_NULL;
	GetEncoderClsid(format, &encoderClsid);

	EncoderParameters encoderParameters;
	encoderParameters.Count = 1;
	encoderParameters.Parameter[0].NumberOfValues = 1;
	encoderParameters.Parameter[0].Value = &quality;
	encoderParameters.Parameter[0].Guid = EncoderQuality;
	encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;

	if (type) {
		LARGE_INTEGER lOfs;
		ULARGE_INTEGER lSize;
		IStream *pS;
		::CreateStreamOnHGlobal(0, 1, &pS);
		bm->Save(pS, &encoderClsid, &encoderParameters);

		lOfs.QuadPart = 0;
		pS->Seek(lOfs, STREAM_SEEK_END, &lSize);

		lOfs.QuadPart = 0;
		pS->Seek(lOfs, STREAM_SEEK_SET, 0);

		*pSize = (ULONG)((DWORD_PTR)lSize.QuadPart);
		*pBuf = (BYTE*)malloc(*pSize);

		pS->Read(*pBuf, (ULONG)*pSize, 0);
		pS->Release();
	} else {
		bm->Save(fn, &encoderClsid, &encoderParameters);
	}
}

static bool BMPDIB(LPCWSTR fn, BYTE* pData, CStringW format, ULONG quality, bool type, BYTE** pBuf, size_t* pSize)
{
	BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)pData;

	int bit = 24, width = bih->biWidth, height = abs(bih->biHeight), bpp = bih->biBitCount / 8;
	int stride = (width * bit + 31) / 32 * bpp, sih = sizeof(BITMAPINFOHEADER);
	DWORD len = stride * height;

	BYTE *src = pData + sih, *rgb = (BYTE*)malloc(len);

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			memcpy(rgb + (3 * x) + (stride * y), src + (width * bpp * y) + (bpp * x), 3);
		}
	}

	BITMAPFILEHEADER bfh;
	bfh.bfType = 0x4d42;
	bfh.bfOffBits = sizeof(bfh) + sih;
	bfh.bfSize = bfh.bfOffBits + len;
	bfh.bfReserved1 = bfh.bfReserved2 = 0;

	BITMAPINFO bi = {{sih, width, height, 1, bit, BI_RGB, 0, 0, 0, 0, 0}};

	if (format == L"") {
		FILE* fp;
		_wfopen_s(&fp, fn, L"wb");
		if (fp) {
			fwrite(&bfh, sizeof(bfh), 1, fp);
			fwrite(&bi.bmiHeader, sih, 1, fp);
			fwrite(rgb, len, 1, fp);
			fclose(fp);
		}
	} else {
		HGLOBAL hG = ::GlobalAlloc(GMEM_MOVEABLE, bfh.bfOffBits + len);
		BYTE* lpBits = (BYTE*)::GlobalLock(hG);

		memcpy(lpBits, &bfh, sizeof(bfh));
		memcpy(lpBits + sizeof(bfh), &bi.bmiHeader, sih);
		memcpy(lpBits + bfh.bfOffBits, rgb, len);

		IStream *s;
		::CreateStreamOnHGlobal(hG, 1, &s);

		ULONG_PTR gdiplusToken;
		GdiplusStartupInput gdiplusStartupInput;
		GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, 0);
		Bitmap *bm = new Bitmap(s);

		GdiplusConvert(bm, fn, format, quality, type, pBuf, pSize);

		delete bm;
		GdiplusShutdown(gdiplusToken);

		s->Release();
		::GlobalUnlock(hG);
		::GlobalFree(hG);
	}

	free(rgb);
	return 1;
}

static void PNGDIB(LPCWSTR fn, BYTE* pData, int level)
{
	FILE* fp;
	_wfopen_s(&fp, fn, L"wb");
	if (fp) {
		BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)pData;

		int line, width = bih->biWidth, height = abs(bih->biHeight), bpp = bih->biBitCount / 8;

		png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		png_infop info_ptr = png_create_info_struct(png_ptr);
		png_init_io(png_ptr, fp);

		png_set_compression_level(png_ptr, level);
		png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, 0, 0);
		png_write_info(png_ptr, info_ptr);

		png_bytep row_ptr = (png_bytep)malloc(width * 3);
		BYTE *p, *src = pData + sizeof(BITMAPINFOHEADER);

		for (int y = height - 1; y >= 0; y--) {
			for (int x = 0; x < width; x++) {
				line = (3 * x);
				p = src + (width * bpp * y) + (bpp * x);
				row_ptr[line] = (png_byte)p[2];
				row_ptr[line + 1] = (png_byte)p[1];
				row_ptr[line + 2] = (png_byte)p[0];
			}
			png_write_row(png_ptr, row_ptr);
		}
		png_write_end(png_ptr, info_ptr);

		fclose(fp);
		free(row_ptr);
		png_destroy_write_struct(&png_ptr, &info_ptr);
	}
}

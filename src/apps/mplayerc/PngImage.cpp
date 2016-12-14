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

#include "stdafx.h"
#include "../../DSUtil/FileHandle.h"
#include "PngImage.h"

static void read_data_fn(png_structp png_ptr, png_bytep data, png_size_t length)
{
	struct png_t* png = (struct png_t*)png_get_progressive_ptr(png_ptr);
	memcpy(data, &png->data[png->pos], length);
	png->pos += length;
}

CMPCPngImage::CMPCPngImage()
	: CImage()
	, m_pExtGradientDATA(NULL)
	, m_ExtGradientHB(NULL)
	, m_width(0)
	, m_height(0)
	, m_bpp(0)
	, m_type(IMG_TYPE::UNDEF)
{
}

CMPCPngImage::~CMPCPngImage()
{
	if (m_ExtGradientHB) {
		DeleteObject(m_ExtGradientHB);
	}
}

bool CMPCPngImage::DecompressPNG(struct png_t* png)
{
	if (png_sig_cmp(png->data, 0, 8) == 0) {
		png->pos = 8;
	}

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);

	if (!png_ptr) {
		return 0;
	}

	png_set_read_fn(png_ptr, (png_voidp)png, read_data_fn);
	png_infop info_ptr = png_create_info_struct(png_ptr);

	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		return 0;
	}

	png_set_sig_bytes(png_ptr, 8);
	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_BGR, 0);
	png_bytep *row_pointers = png_get_rows(png_ptr, info_ptr);

	unsigned int w = png_get_image_width(png_ptr, info_ptr), h = png_get_image_height(png_ptr, info_ptr);
	unsigned int b = png_get_channels(png_ptr, info_ptr);
	unsigned int len = w * b;
	unsigned char *row, *pic = (unsigned char*)malloc(len * h);

	bool ret = false;

	if (Create(w, -(int)h, b * 8)) {
		for (unsigned y = 0; y < h; y++) {
			row = &pic[len * y];
			for (unsigned x = 0; x < len; row += b) {
				for (unsigned c = 0; c < b; c++) {
					row[c] = row_pointers[y][x++];
				}
			}
			memcpy(GetPixelAddress(0, y), &pic[len * y], len);
		}
		ret = true;
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, 0);

	free(pic);

	return ret;
}

bool CMPCPngImage::LoadFromResource(UINT id) {
	bool ret = false;

	CStringA str;
	if (LoadResource(id, str, L"FILE") || LoadResource(id, str, L"PNG")) {
		struct png_t png;
		png.data = (unsigned char*)(LPCSTR)str;
		png.size = str.GetLength();

		ret = DecompressPNG(&png);
	}

	return ret;
}

bool CMPCPngImage::FileExists(CString& fn, bool bInclJPEG)
{
	CString path = GetProgramDir();

	if (::PathFileExists(path + fn + L".png")) {
		fn = path + fn + L".png";
		return true;
	} else if (::PathFileExists(path + fn + L".bmp")) {
		fn = path + fn + L".bmp";
		return true;
	} else if (bInclJPEG && ::PathFileExists(path + fn + L".jpg")) {
		fn = path + fn + L".jpg";
		return true;
	}

	return false;
}

BYTE* CMPCPngImage::BrightnessRGB(IMG_TYPE type, BYTE* lpBits, int width, int height, int bpp, int br, int rc, int gc, int bc)
{
	int k = bpp / 8;
	int size = width * height * k;
	double R, G, B;
	/*
	int kbr = 100;
	double brn, rcn, gcn, bcn;

	if (br >= 0 && rc >= 0 && gc >= 0 && bc >= 0) {

		brn = (double)((br * 10) / (kbr + (br * 4)) * 0.8);
		rcn = (double)(rc / (70 + kbr - br) + brn);
		gcn = (double)(gc / (75 + kbr - br) + brn);
		bcn = (double)(bc / (80 + kbr - br) + brn);

		if (rcn < 0.5) {
			rcn = 0.5;
		}
		if (gcn < 0.5) {
			gcn = 0.5;
		}
		if (bcn < 0.5) {
			bcn = 0.5;
		}
	}
	*/

	for (int i = 0; i < size; i += k) {

		R = lpBits[i];
		G = lpBits[i + 1];
		B = lpBits[i + 2];
		/*
		if (br >= 0 && rc >= 0 && gc >= 0 && bc >= 0) {

			R *= rcn;
			G *= gcn;
			B *= bcn;

			if (R > 255) {
				R = 255;
			}
			if (G > 255) {
				G = 255;
			}
			if (B > 255) {
				B = 255;
			}
		}
		*/
		if (type == IMG_TYPE::BMP) {
			lpBits[i] = R;
			lpBits[i + 2] = B;
		} else if (type == IMG_TYPE::PNG) {
			lpBits[i] = B;
			lpBits[i + 2] = R;
		}
		lpBits[i + 1] = G;
	}

	return lpBits;
}

HBITMAP CMPCPngImage::TypeLoadImage(IMG_TYPE type, BYTE** pData, int* width, int* height, int* bpp, FILE* fp, int resid, int br/* = -1*/, int rc/* = -1*/, int gc/* = -1*/, int bc/* = -1*/)
{
	HBITMAP hbm = NULL;
	BYTE* bmp;

	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep* row_pointers;

	if (type == IMG_TYPE::BMP) {

		fseek(fp, 0, SEEK_END);
		DWORD size = ftell(fp);
		rewind(fp);
		bmp = (BYTE*)malloc(size);
		fread(bmp, size, 1, fp);

		BITMAPINFO& bi = (BITMAPINFO&)bmp[sizeof(BITMAPFILEHEADER)];

		*width = bi.bmiHeader.biWidth;
		*height = abs(bi.bmiHeader.biHeight);
		*bpp = bi.bmiHeader.biBitCount;

		hbm = CreateDIBSection(0, &bi, DIB_RGB_COLORS, (void**)pData, 0, 0);

	} else if (type == IMG_TYPE::PNG) {

		png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);

		if (fp) {
			png_init_io(png_ptr, fp);
		} else {
			HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(resid), L"PNG");
			HANDLE lRes = LoadResource(NULL, hRes);

			struct png_t png;
			png.data = (unsigned char*)LockResource(lRes);
			png.size = SizeofResource(NULL, hRes);
			png.pos = 8;
			png_set_read_fn(png_ptr, (png_voidp)&png, read_data_fn);
			png_set_sig_bytes(png_ptr, 8);

			UnlockResource(lRes);
			FreeResource(lRes);
		}

		info_ptr = png_create_info_struct(png_ptr);

		png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND, 0);

		row_pointers = png_get_rows(png_ptr, info_ptr);

		*width = png_get_image_width(png_ptr, info_ptr);
		*height = png_get_image_height(png_ptr, info_ptr);
		*bpp = png_get_channels(png_ptr, info_ptr) * 8;

		BITMAPINFO bmi = {{sizeof(BITMAPINFOHEADER), *width, -(*height), 1, *bpp, BI_RGB, 0, 0, 0, 0, 0}};

		hbm = CreateDIBSection(0, &bmi, DIB_RGB_COLORS, (void**)pData, 0, 0);
	}

	int memWidth = (*width) * (*bpp) / 8;

	if (type == IMG_TYPE::BMP) {

		int hsize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		memcpy(*pData, BrightnessRGB(type, bmp, *width, *height, *bpp, br, rc, gc, bc) + hsize, memWidth * (*height));
		free(bmp);

	} else if (type == IMG_TYPE::PNG) {

		for (int i = 0; i < *height; i++) {
			memcpy((*pData) + memWidth * i, BrightnessRGB(type, (BYTE*)row_pointers[i], *width, 1, *bpp, br, rc, gc, bc), memWidth);
		}
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
	}

	if (fp) {
		fclose(fp);
	}

	return hbm;
}

HBITMAP CMPCPngImage::LoadExternalImage(CString fn, int resid, IMG_TYPE type, int br/* = -1*/, int rc/* = -1*/, int gc/* = -1*/, int bc/* = -1*/)
{
	CString path = GetProgramDir();

	BYTE* pData;
	int width, height, bpp;

	FILE* fp = NULL;
	if (fn.GetLength() > 0) {
		_wfopen_s(&fp, path + fn + L".png", L"rb");
	}
	if (fp) {
		return TypeLoadImage(IMG_TYPE::PNG, &pData, &width, &height, &bpp, fp, 0, br, rc, gc, bc);
	} else {
		if (fn.GetLength() > 0) {
			_wfopen_s(&fp, path + fn + L".bmp", L"rb");
		}
		if (fp) {
			return TypeLoadImage(IMG_TYPE::BMP, &pData, &width, &height, &bpp, fp, 0, br, rc, gc, bc);
		} else {
			if (resid && ((int)AfxGetAppSettings().bUseDarkTheme == !!type || type == IMG_TYPE::UNDEF)) {
				return TypeLoadImage(IMG_TYPE::PNG, &pData, &width, &height, &bpp, NULL, resid, br, rc, gc, bc);
			}
		}
	}

	return NULL;
}

bool CMPCPngImage::LoadExternalGradient(CString fn)
{
	CString path = GetProgramDir();

	FILE* fp = NULL;
	_wfopen_s(&fp, path + fn + L".png", L"rb");

	if (m_ExtGradientHB) {
		DeleteObject(m_ExtGradientHB);
	}
	m_ExtGradientHB = NULL;

	if (fp) {
		m_ExtGradientHB = TypeLoadImage(IMG_TYPE::PNG, &m_pExtGradientDATA, &m_width, &m_height, &m_bpp, fp, 0);
		m_type = IMG_TYPE::PNG;
	} else {
		_wfopen_s(&fp, path + fn + L".bmp", L"rb");
		if (fp) {
			m_ExtGradientHB = TypeLoadImage(IMG_TYPE::BMP, &m_pExtGradientDATA, &m_width, &m_height, &m_bpp, fp, 0);
			m_type = IMG_TYPE::BMP;
		}
	}

	if (fp) {
		fclose(fp);
	}

	return m_ExtGradientHB != NULL;
}

bool CMPCPngImage::PaintExternalGradient(CDC* dc, CRect r, int ptop, int br/* = -1*/, int rc/* = -1*/, int gc/* = -1*/, int bc/* = -1*/)
{
	if (IsExtGradiendLoading()) {
		size_t size = m_width * m_height * m_bpp >> 3;
		BYTE* pData = DNew BYTE[size];
		memcpy(pData, BrightnessRGB(IMG_TYPE::BMP, m_pExtGradientDATA, m_width, m_height, m_bpp, br, rc, gc, bc), size);

		GRADIENT_RECT gr[1] = {{0, 1}};

		int bt = m_bpp >> 3, st = 2, pa = 255 * 256;
		int sp = bt * ptop, hs = bt * st;

		if (m_width > m_height) {
			for (int k = sp, t = 0; t < r.right; k += hs, t += st) {
				TRIVERTEX tv[2] = {
					{t, 0, pData[k + 2] * 256, pData[k + 1] * 256, pData[k] * 256, pa},
					{t + st, 1, pData[k + hs + 2] * 256, pData[k + hs + 1] * 256, pData[k + hs] * 256, pa},
				};
				dc->GradientFill(tv, 2, gr, 1, GRADIENT_FILL_RECT_H);
			}
		} else {
			for (int k = sp, t = 0; t < r.bottom; k += hs, t += st) {
				TRIVERTEX tv[2] = {
					{r.left, t, pData[k + 2] * 256, pData[k + 1] * 256, pData[k] * 256, pa},
					{r.right, t + st, pData[k + hs + 2] * 256, pData[k + hs + 1] * 256, pData[k + hs] * 256, pa},
				};
				dc->GradientFill(tv, 2, gr, 1, GRADIENT_FILL_RECT_V);
			}
		}

		delete [] pData;

		return true;
	}

	return false;
}

CSize CMPCPngImage::GetSize() const
{
	CSize size;
	if (!IsNull()) {
		size.SetSize(GetWidth(), GetHeight());
	}

	return size;
}
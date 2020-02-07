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

#include <atlimage.h>
#include <libpng/png.h>

struct png_t {
	unsigned char* data;
	png_size_t size, pos;
};

enum IMG_TYPE {
	UNDEF = -1,
	BMP,
	PNG
};

class CMPCPngImage : public CImage
{
	// External Gradient
	BYTE*    m_pExtGradientDATA;
	HBITMAP  m_ExtGradientHB;
	int      m_width, m_height, m_bpp;
	IMG_TYPE m_type;

	bool DecompressPNG(struct png_t* png);

	BYTE* BrightnessRGB(IMG_TYPE type, BYTE* lpBits, int width, int height, int bpp, int br, int rc, int gc, int bc);
	static BYTE* SwapPNG(BYTE* lpBits, int width, int height, int bpp);
public:
	CMPCPngImage();
	~CMPCPngImage();

	bool LoadFromResource(UINT id);

	static bool FileExists(CString& fn);
	static bool FileExists(const CString& fn);

	static HBITMAP TypeLoadImage(IMG_TYPE type, BYTE** pData, int* width, int* height, int* bpp, FILE* fp, int resid, int br = -1, int rc = -1, int gc = -1, int bc = -1);
	static HBITMAP LoadExternalImage(const CString& fn, int resid, IMG_TYPE type, int br = -1, int rc = -1, int gc = -1, int bc = -1);

	bool LoadExternalGradient(const CString& fn);
	bool PaintExternalGradient(CDC* dc, CRect r, int ptop, int br = -1, int rc = -1, int gc = -1, int bc = -1);

	const bool IsExtGradiendLoading() { return m_ExtGradientHB && m_width && m_height && m_bpp; };

	CSize GetSize() const;
};

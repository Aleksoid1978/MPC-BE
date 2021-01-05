/*
 * (C) 2020-2021 see Authors.txt
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

#define NANOSVG_IMPLEMENTATION
#include "../../ExtLib/nanosvg/src/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "../../ExtLib/nanosvg/src/nanosvgrast.h"

#include "SvgHelper.h"
#include "Misc.h"

CSvgImage::~CSvgImage()
{
	Clear();
}

float CSvgImage::CalcScale(int& w, int& h)
{
	float scale = 1.0f;

	auto svgimage = ((NSVGimage*)m_pSvgImage);

	if (!w && !h) {
		w = (int)std::round(svgimage->width);
		h = (int)std::round(svgimage->height);
	}
	else if (!w) {
		scale = h / svgimage->height;
		w = (int)std::round(svgimage->width * scale);
	}
	else if (!h) {
		scale = w / svgimage->width;
		h = (int)std::round(svgimage->height * scale);
	}
	else {
		scale = std::min(w / svgimage->width, h / svgimage->height);
	}

	return scale;
}

void CSvgImage::Clear()
{
	if (m_pRasterizer) {
		nsvgDeleteRasterizer((NSVGrasterizer*)m_pRasterizer);
		m_pRasterizer = nullptr;
	}

	if (m_pSvgImage) {
		nsvgDelete((NSVGimage*)m_pSvgImage);
		m_pSvgImage = nullptr;
	}
}

bool CSvgImage::Load(char* svgData)
{
	Clear();

	m_pSvgImage = nsvgParse(svgData, "px", 96.0f);
	if (m_pSvgImage) {
		m_pRasterizer = nsvgCreateRasterizer();
	}

	if (!m_pSvgImage || !m_pRasterizer) {
		Clear();
		return false;
	}

	return true;
}

bool CSvgImage::Load(const wchar_t* filename)
{
	FILE* file = nullptr;

	if (_wfopen_s(&file, filename, L"rb") == 0) {
		fseek(file, 0, SEEK_END);
		const size_t len = ftell(file);
		fseek(file, 0, SEEK_SET);

		std::unique_ptr<char[]> svgData(new(std::nothrow) char[len + 1]);
		if (svgData) {
			const size_t ret = fread(svgData.get(), 1, len, file);

			if (ret == len) {
				fclose(file);
				svgData.get()[len] = '\0'; // Must be null terminated.

				return Load(svgData.get());
			}
		}
		fclose(file);
	}

	return false;
}

bool CSvgImage::Load(UINT resid)
{
	CStringA svgData;
	if (LoadResource(resid, svgData, L"FILE")) {
		return Load(svgData.GetBuffer());
	}

	return false;
}

bool CSvgImage::GetOriginalSize(int& w, int& h)
{
	if (m_pSvgImage) {
		auto svgimage = ((NSVGimage*)m_pSvgImage);
		w = (int)std::round(svgimage->width);
		h = (int)std::round(svgimage->height);
		return true;
	}
	return false;
}

HBITMAP CSvgImage::Rasterize(int& w, int& h)
{
	HBITMAP hBitmap = nullptr;

	if (m_pSvgImage && m_pRasterizer) {
		float scale = CalcScale(w, h);

		std::unique_ptr<BYTE[]> buffer(new(std::nothrow) BYTE[w * h * 4]);
		if (buffer) {
			nsvgRasterize((NSVGrasterizer*)m_pRasterizer, (NSVGimage*)m_pSvgImage, 0.0f, 0.0f, scale, buffer.get(), w, h, w * 4);

			hBitmap = CreateBitmap(w, h, 1, 32, buffer.get());
		}
	}

	return hBitmap;
}

bool CSvgImage::Rasterize(CBitmap& bitmap, int& w, int& h) // not tested
{
	if (m_pSvgImage && m_pRasterizer) {
		float scale = CalcScale(w, h);

		std::unique_ptr<BYTE[]> buffer(new(std::nothrow) BYTE[w * h * 4]);
		if (buffer) {
			nsvgRasterize((NSVGrasterizer*)m_pRasterizer, (NSVGimage*)m_pSvgImage, 0.0f, 0.0f, scale, buffer.get(), w, h, w * 4);
			bitmap.CreateBitmap(w, h, 1, 32, buffer.get());

			return true;
		}
	}

	return false;
}

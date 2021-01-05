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

#pragma once

class CSvgImage
{
	void* m_pSvgImage = nullptr;
	void* m_pRasterizer = nullptr;

public:
	~CSvgImage();

private:
	float CalcScale(int& w, int& h);

public:
	void Clear();
	bool Load(char* svgData);
	bool Load(const wchar_t* filename);
	bool Load(UINT resid);
	bool GetOriginalSize(int& w, int& h);
	HBITMAP Rasterize(int& w, int& h); // if the dimension is zero, then it will be set automatically
	bool Rasterize(CBitmap& bitmap, int& w, int& h); // if the dimension is zero, then it will be set automatically

	bool IsLoad() { return m_pSvgImage && m_pRasterizer; }
};

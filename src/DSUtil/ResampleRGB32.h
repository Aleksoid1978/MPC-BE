/*
* (C) 2017 see Authors.txt
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

struct filter_t;

class CResampleRGB32
{
public:
	enum : int {
		FILTER_BOX = 1,
		FILTER_BILINEAR,
		FILTER_HAMMING,
		FILTER_BICUBIC,
		FILTER_LANCZOS
	};

private:
	bool m_actual = false;
	int  m_srcW   = 0;
	int  m_srcH   = 0;
	int  m_destW  = 0;
	int  m_destH  = 0;
	int  m_filter = 0;
	bool m_alpha  = false;

	filter_t* m_pFilter = nullptr;

	bool m_bResampleHor = false;
	bool m_bResampleVer = false;

	BYTE*  m_pTemp      = nullptr;

	int*   m_boundsHor  = nullptr;
	INT32* m_kkHor      = nullptr;
	int    m_kmaxHor    = 0;

	int*   m_boundsVer  = nullptr;
	INT32* m_kkVer      = nullptr;
	int    m_kmaxVer    = 0;

	void ResampleHorizontal(BYTE* dest, int destW, int H, const BYTE* const src, int srcW);
	void ResampleVertical(BYTE* dest, int W, int destH, const BYTE* const src, int srcH);

	void FreeData();
	HRESULT Init();

public:
	~CResampleRGB32();

	HRESULT SetParameters(const int destW, const int destH, const int srcW, const int srcH, const int filter, const bool alpha);
	HRESULT Process(BYTE* const dest, const BYTE* const src);
};

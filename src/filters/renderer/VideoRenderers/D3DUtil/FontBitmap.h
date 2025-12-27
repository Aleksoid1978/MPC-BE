/*
* (C) 2019-2025 see Authors.txt
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

#include "D3DCommon.h"

#define DUMP_BITMAP 0

#if _DEBUG && DUMP_BITMAP
#include "apps/mplayerc/WicUtils.h"
#endif

struct Grid_t {
	UINT stepX = 0;
	UINT stepY = 0;
	UINT columns = 0;
	UINT lines = 0;
};

inline uint16_t A8R8G8B8toA8L8(uint32_t pix)
{
	uint32_t a = (pix & 0xff000000) >> 24;
	uint32_t r = (pix & 0x00ff0000) >> 16;
	uint32_t g = (pix & 0x0000ff00) >> 8;
	uint32_t b = (pix & 0x000000ff);
	uint32_t l = ((r * 1063 + g * 3576 + b * 361) / 5000);

	return (uint16_t)((a << 8) + l);
}

class CFontBitmapGDI
{
private:
	HBITMAP m_hBitmap = nullptr;
	DWORD* m_pBitmapBits = nullptr;
	UINT m_bmWidth = 0;
	UINT m_bmHeight = 0;
	std::vector<RECT> m_charCoords;
	SIZE m_MaxCharMetric = {};

public:
	CFontBitmapGDI() = default;

	~CFontBitmapGDI()
	{
		DeleteObject(m_hBitmap);
	}

	HRESULT Initialize(const WCHAR* fontName, const int fontHeight, const int fontWidth, UINT fontFlags, const WCHAR* chars, UINT lenght)
	{
		DeleteObject(m_hBitmap);
		m_pBitmapBits = nullptr;
		m_charCoords.clear();

		HRESULT hr = S_OK;

		// Create a font. By specifying ANTIALIASED_QUALITY, we might get an
		// antialiased font, but this is not guaranteed.
		HFONT hFont = CreateFontW(
			fontHeight, fontWidth,
			0, 0,
			(fontFlags & D3DFONT_BOLD)   ? FW_BOLD : FW_NORMAL,
			(fontFlags & D3DFONT_ITALIC) ? TRUE    : FALSE,
			FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
			DEFAULT_PITCH, fontName);

		HDC hDC = CreateCompatibleDC(nullptr);
		SetMapMode(hDC, MM_TEXT);
		HFONT hFontOld = (HFONT)SelectObject(hDC, hFont);

		std::vector<SIZE> charSizes;
		charSizes.reserve(lenght);
		SIZE size;
		LONG maxWidth = 0;
		LONG maxHeight = 0;
		for (UINT i = 0; i < lenght; i++) {
			if (GetTextExtentPoint32W(hDC, &chars[i], 1, &size) == FALSE) {
				hr = E_FAIL;
				break;
			}
			charSizes.emplace_back(size);

			if (size.cx > maxWidth) {
				maxWidth = size.cx;
			}
			if (size.cy > maxHeight) {
				maxHeight = size.cy;
			}
		}

		if (S_OK == hr) {
			m_MaxCharMetric = { maxWidth, maxHeight };
			UINT stepX = m_MaxCharMetric.cx + 2;
			UINT stepY = m_MaxCharMetric.cy;
			UINT bmWidth = 128;
			UINT bmHeight = 128;
			UINT columns = bmWidth / stepX;
			UINT lines = bmHeight / stepY;

			while (lenght > lines * columns) {
				if (bmWidth <= bmHeight) {
					bmWidth *= 2;
				} else {
					bmHeight += 128;
				}
				columns = bmWidth / stepX;
				lines = bmHeight / stepY;
			};

			// Prepare to create a bitmap
			BITMAPINFO bmi = {};
			bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
			bmi.bmiHeader.biWidth       =  (LONG)bmWidth;
			bmi.bmiHeader.biHeight      = -(LONG)bmHeight;
			bmi.bmiHeader.biPlanes      = 1;
			bmi.bmiHeader.biCompression = BI_RGB;
			bmi.bmiHeader.biBitCount    = 32;

			// Create a bitmap for the font
			m_hBitmap = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, (void**)&m_pBitmapBits, nullptr, 0);
			m_bmWidth = bmWidth;
			m_bmHeight = bmHeight;

			HGDIOBJ m_hBitmapOld = SelectObject(hDC, m_hBitmap);

			SetTextColor(hDC, RGB(255, 255, 255));
			SetBkColor(hDC, 0x00000000);
			SetTextAlign(hDC, TA_TOP);

			UINT idx = 0;
			for (UINT y = 0; y < lines; y++) {
				for (UINT x = 0; x < columns; x++) {
					if (idx >= lenght) {
						break;
					}
					UINT X = x * stepX + 1;
					UINT Y = y * stepY;
					if (ExtTextOutW(hDC, X, Y, ETO_OPAQUE, nullptr, &chars[idx], 1, nullptr) == FALSE) {
						hr = E_FAIL;
						break;
					}
					RECT rect = {
						X,
						Y,
						X + charSizes[idx].cx,
						Y + charSizes[idx].cy
					};
					m_charCoords.emplace_back(rect);
					idx++;
				}
			}
			GdiFlush();

			SelectObject(hDC, m_hBitmapOld);
		}

		SelectObject(hDC, hFontOld);
		DeleteObject(hFont);
		DeleteDC(hDC);

		// set transparency
		const UINT nPixels = m_bmWidth * m_bmHeight;
		for (UINT i = 0; i < nPixels; i++) {
			uint32_t pix = m_pBitmapBits[i];
			uint32_t r = (pix & 0x00ff0000) >> 16;
			uint32_t g = (pix & 0x0000ff00) >> 8;
			uint32_t b = (pix & 0x000000ff);
			uint32_t l = ((r * 1063 + g * 3576 + b * 361) / 5000);
			m_pBitmapBits[i] = (l << 24) | (pix & 0x00ffffff); // the darker the more transparent
		}

#if _DEBUG && DUMP_BITMAP
		if (S_OK == hr) {
			size_t outlen = 0;
			WicSaveImage((BYTE*)m_pBitmapBits, m_bmWidth * 4,
				m_bmWidth, m_bmHeight, GUID_WICPixelFormat32bppPBGRA,
				0, L"c:\\temp\\font_gdi_bitmap.png",
				nullptr, outlen);
		}
#endif

		return hr;
	}

	UINT GetWidth()
	{
		return m_bmWidth;
	}

	UINT GetHeight()
	{
		return m_bmHeight;
	}

	SIZE GetMaxCharMetric()
	{
		return m_MaxCharMetric;
	}

	HRESULT GetFloatCoords(FloatRect* pTexCoords, const UINT lenght)
	{
		ASSERT(pTexCoords);

		if (!m_hBitmap || !m_pBitmapBits || lenght != m_charCoords.size()) {
			return E_ABORT;
		}

		for (const auto coord : m_charCoords) {
			*pTexCoords++ = {
				(float)coord.left   / m_bmWidth,
				(float)coord.top    / m_bmHeight,
				(float)coord.right  / m_bmWidth,
				(float)coord.bottom / m_bmHeight,
			};
		}

		return S_OK;
	}

	HRESULT Lock(BYTE** ppData, UINT& uStride)
	{
		if (!m_hBitmap || !m_pBitmapBits) {
			return E_ABORT;
		}

		*ppData = (BYTE*)m_pBitmapBits;
		uStride = m_bmWidth * 4;

		return S_OK;
	}

	void Unlock()
	{
		// nothing
	}
};

typedef CFontBitmapGDI CFontBitmap;

/*
 * (C) 2016-2023 see Authors.txt
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

#include <d3d9.h>

bool RetrieveBitmapData(unsigned w, unsigned h, unsigned bpp, BYTE* dst, BYTE* src, int srcpitch);

HRESULT DumpDX9Surface(IDirect3DDevice9* pD3DDev, IDirect3DSurface9* pSurface, wchar_t* filename);
HRESULT DumpDX9Texture(IDirect3DDevice9* pD3DDev, IDirect3DTexture9* pTexture, wchar_t* filename);
HRESULT DumpDX9RenderTarget(IDirect3DDevice9* pD3DDev, wchar_t* filename);

HRESULT SaveRAWVideoAsBMP(BYTE* data, DWORD format, unsigned pitch, unsigned width, unsigned height, wchar_t* filename);

struct DisplayConfig_t {
	UINT32 width;
	UINT32 height;
	UINT32 bitsPerChannel;
	DISPLAYCONFIG_COLOR_ENCODING colorEncoding;
	union {
		struct {
			UINT32 advancedColorSupported : 1;    // A type of advanced color is supported
			UINT32 advancedColorEnabled : 1;    // A type of advanced color is enabled
			UINT32 wideColorEnforced : 1;    // Wide color gamut is enabled
			UINT32 advancedColorForceDisabled : 1;    // Advanced color is force disabled due to system/OS policy
			UINT32 reserved : 28;
		};
		UINT32 value;
	} advancedColor;
	DISPLAYCONFIG_RATIONAL                refreshRate;
	DISPLAYCONFIG_SCANLINE_ORDERING       scanLineOrdering;
	DISPLAYCONFIG_VIDEO_OUTPUT_TECHNOLOGY outputTechnology;
	WCHAR displayName[CCHDEVICENAME];
	WCHAR monitorName[64];
	DISPLAYCONFIG_MODE_INFO modeTarget;
};

// DisplayConfig

bool GetDisplayConfig(const wchar_t* displayName, DisplayConfig_t& displayConfig);

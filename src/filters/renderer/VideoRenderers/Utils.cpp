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

#include "stdafx.h"
#include <memory>

#include "Utils.h"

double get_refresh_rate(const DISPLAYCONFIG_PATH_INFO& path, DISPLAYCONFIG_MODE_INFO* modes)
{
	double freq = 0.0;

	if (path.targetInfo.modeInfoIdx != DISPLAYCONFIG_PATH_MODE_IDX_INVALID) {
		DISPLAYCONFIG_MODE_INFO* mode = &modes[path.targetInfo.modeInfoIdx];
		if (mode->infoType == DISPLAYCONFIG_MODE_INFO_TYPE_TARGET) {
			DISPLAYCONFIG_RATIONAL* vSyncFreq = &mode->targetMode.targetVideoSignalInfo.vSyncFreq;
			if (vSyncFreq->Denominator != 0 && vSyncFreq->Numerator / vSyncFreq->Denominator > 1) {
				freq = (double)vSyncFreq->Numerator / (double)vSyncFreq->Denominator;
			}
		}
	}

	if (freq == 0.0) {
		const DISPLAYCONFIG_RATIONAL* refreshRate = &path.targetInfo.refreshRate;
		if (refreshRate->Denominator != 0 && refreshRate->Numerator / refreshRate->Denominator > 1) {
			freq = (double)refreshRate->Numerator / (double)refreshRate->Denominator;
		}
	}

	return freq;
};


double GetRefreshRate(const wchar_t* displayName)
{
	UINT32 num_paths;
	UINT32 num_modes;
	std::vector<DISPLAYCONFIG_PATH_INFO> paths;
	std::vector<DISPLAYCONFIG_MODE_INFO> modes;
	LONG res;

	// The display configuration could change between the call to
	// GetDisplayConfigBufferSizes and the call to QueryDisplayConfig, so call
	// them in a loop until the correct buffer size is chosen
	do {
		res = GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &num_paths, &num_modes);
		if (res == ERROR_SUCCESS) {
			paths.resize(num_paths);
			modes.resize(num_modes);
			res = QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &num_paths, paths.data(), &num_modes, modes.data(), nullptr);
		}
	} while (res == ERROR_INSUFFICIENT_BUFFER);

	if (res == ERROR_SUCCESS) {
		// num_paths and num_modes could decrease in a loop
		paths.resize(num_paths);
		modes.resize(num_modes);

		for (const auto& path : paths) {
			// Send a GET_SOURCE_NAME request
			DISPLAYCONFIG_SOURCE_DEVICE_NAME source = {
				{DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME, sizeof(source), path.sourceInfo.adapterId, path.sourceInfo.id}, {},
			};
			if (DisplayConfigGetDeviceInfo(&source.header) == ERROR_SUCCESS) {
				if (wcscmp(displayName, source.viewGdiDeviceName) == 0) {
					return get_refresh_rate(path, modes.data());
				}
			}
		}
	}

	return 0.0;
}

bool RetrieveBitmapData(unsigned w, unsigned h, unsigned bpp, BYTE* dst, BYTE* src, int srcpitch)
{
	unsigned linesize = w * bpp / 8;
	if ((int)linesize > srcpitch) {
		return false;
	}

	src += srcpitch * (h - 1);

	for (unsigned y = 0; y < h; ++y) {
		memcpy(dst, src, linesize);
		src -= srcpitch;
		dst += linesize;
	}

	return true;
}

HRESULT DumpDX9Surface(IDirect3DDevice9* pD3DDev, IDirect3DSurface9* pSurface, wchar_t* filename)
{
	CheckPointer(pD3DDev, E_POINTER);
	CheckPointer(pSurface, E_POINTER);

	HRESULT hr;
	D3DSURFACE_DESC desc = {};

	if (FAILED(hr = pSurface->GetDesc(&desc))) {
		return hr;
	};

	CComPtr<IDirect3DSurface9> pTarget;
	if (FAILED(hr = pD3DDev->CreateRenderTarget(desc.Width, desc.Height, D3DFMT_X8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &pTarget, nullptr))
		|| FAILED(hr = pD3DDev->StretchRect(pSurface, nullptr, pTarget, nullptr, D3DTEXF_NONE))) {
		return hr;
	}

	unsigned len = desc.Width * desc.Height * 4;
	std::unique_ptr<BYTE[]> dib(new(std::nothrow) BYTE[sizeof(BITMAPINFOHEADER) + len]);
	if (!dib) {
		return E_OUTOFMEMORY;
	}

	BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)dib.get();
	memset(bih, 0, sizeof(BITMAPINFOHEADER));
	bih->biSize = sizeof(BITMAPINFOHEADER);
	bih->biWidth = desc.Width;
	bih->biHeight = desc.Height;
	bih->biBitCount = 32;
	bih->biPlanes = 1;
	bih->biSizeImage = DIBSIZE(*bih);

	D3DLOCKED_RECT r;
	hr = pTarget->LockRect(&r, nullptr, D3DLOCK_READONLY);

	RetrieveBitmapData(desc.Width, desc.Height, 32, (BYTE*)(bih + 1), (BYTE*)r.pBits, r.Pitch);

	pTarget->UnlockRect();

	BITMAPFILEHEADER bfh;
	bfh.bfType = 0x4d42;
	bfh.bfOffBits = sizeof(bfh) + sizeof(BITMAPINFOHEADER);
	bfh.bfSize = bfh.bfOffBits + len;
	bfh.bfReserved1 = bfh.bfReserved2 = 0;

	FILE* fp;
	if (_wfopen_s(&fp, filename, L"wb") == 0) {
		fwrite(&bfh, sizeof(bfh), 1, fp);
		fwrite(dib.get(), sizeof(BITMAPINFOHEADER) + len, 1, fp);
		fclose(fp);
	}

	return hr;
}

HRESULT DumpDX9Texture(IDirect3DDevice9* pD3DDev, IDirect3DTexture9* pTexture, wchar_t* filename)
{
	CheckPointer(pTexture, E_POINTER);

	CComPtr<IDirect3DSurface9> pSurface;
	pTexture->GetSurfaceLevel(0, &pSurface);

	return DumpDX9Surface(pD3DDev, pSurface, filename);
}

HRESULT DumpDX9RenderTarget(IDirect3DDevice9* pD3DDev, wchar_t* filename)
{
	CheckPointer(pD3DDev, E_POINTER);

	CComPtr<IDirect3DSurface9> pSurface;
	pD3DDev->GetRenderTarget(0, &pSurface);

	return DumpDX9Surface(pD3DDev, pSurface, filename);
}

HRESULT SaveRAWVideoAsBMP(BYTE* data, DWORD format, unsigned pitch, unsigned width, unsigned height, wchar_t* filename)
{
	CheckPointer(data, E_POINTER);

	unsigned pseudobitdepth = 8;

	switch (format) {
	case FCC('NV12'):
	case FCC('YV12'):
		height = height * 3 / 2;
		break;
	case D3DFMT_UYVY:
	case D3DFMT_YUY2:
		width *= 2;
		break;
	case FCC('YV16'):
		height *= 2;
		break;
	case FCC('YV24'):
		height *= 3;
		break;
	case FCC('AYUV'):
	case D3DFMT_A8R8G8B8:
	case D3DFMT_X8R8G8B8:
		pseudobitdepth = 32;
		break;
	case FCC('P010'):
	case FCC('P016'):
		width *= 2;
		height = height * 3 / 2;
		break;
	case FCC('P210'):
	case FCC('P216'):
		width *= 2;
		height *= 2;
		break;
	default:
		return E_INVALIDARG;
	}

	if (!pitch) {
		pitch = width * pseudobitdepth / 8;
	}

	unsigned tablecolors = (pseudobitdepth == 8) ? 256 : 0;

	unsigned len = width * height * pseudobitdepth / 8;
	std::unique_ptr<BYTE[]> dib(new(std::nothrow) BYTE[sizeof(BITMAPINFOHEADER) + tablecolors * 4 + len]);
	if (!dib) {
		return E_OUTOFMEMORY;
	}

	BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)dib.get();
	memset(bih, 0, sizeof(BITMAPINFOHEADER));
	bih->biSize = sizeof(BITMAPINFOHEADER);
	bih->biWidth = width;
	bih->biHeight = height;
	bih->biBitCount = pseudobitdepth;
	bih->biPlanes = 1;
	bih->biSizeImage = DIBSIZE(*bih);
	bih->biClrUsed = tablecolors;

	BYTE* p = (BYTE*)(bih + 1);
	for (unsigned i = 0; i < tablecolors; i++) {
		*p++ = (BYTE)i;
		*p++ = (BYTE)i;
		*p++ = (BYTE)i;
		*p++ = 0;
	}

	//memcpy(p, r.pBits, len);
	RetrieveBitmapData(width, height, pseudobitdepth, p, data, pitch);

	BITMAPFILEHEADER bfh;
	bfh.bfType = 0x4d42;
	bfh.bfOffBits = sizeof(bfh) + sizeof(BITMAPINFOHEADER) + tablecolors * 4;
	bfh.bfSize = bfh.bfOffBits + len;
	bfh.bfReserved1 = bfh.bfReserved2 = 0;

	FILE* fp;
	if (_wfopen_s(&fp, filename, L"wb") == 0) {
		fwrite(&bfh, sizeof(bfh), 1, fp);
		fwrite(dib.get(), sizeof(BITMAPINFOHEADER) + tablecolors * 4 + len, 1, fp);
		fclose(fp);
	}

	return S_OK;
}

// DisplayConfig

static bool is_valid_refresh_rate(const DISPLAYCONFIG_RATIONAL& rr)
{
	// DisplayConfig sometimes reports a rate of 1 when the rate is not known
	return rr.Denominator != 0 && rr.Numerator / rr.Denominator > 1;
}

bool GetDisplayConfig(const wchar_t* displayName, DisplayConfig_t& displayConfig)
{
	UINT32 num_paths;
	UINT32 num_modes;
	std::vector<DISPLAYCONFIG_PATH_INFO> paths;
	std::vector<DISPLAYCONFIG_MODE_INFO> modes;
	LONG res;

	// The display configuration could change between the call to
	// GetDisplayConfigBufferSizes and the call to QueryDisplayConfig, so call
	// them in a loop until the correct buffer size is chosen
	do {
		res = GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &num_paths, &num_modes);
		if (ERROR_SUCCESS == res) {
			paths.resize(num_paths);
			modes.resize(num_modes);
			res = QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &num_paths, paths.data(), &num_modes, modes.data(), nullptr);
		}
	} while (ERROR_INSUFFICIENT_BUFFER == res);

	if (res == ERROR_SUCCESS) {
		// num_paths and num_modes could decrease in a loop
		paths.resize(num_paths);
		modes.resize(num_modes);

		for (const auto& path : paths) {
			// Send a GET_SOURCE_NAME request
			DISPLAYCONFIG_SOURCE_DEVICE_NAME source = {
				{DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME, sizeof(source), path.sourceInfo.adapterId, path.sourceInfo.id}, {},
			};
			res = DisplayConfigGetDeviceInfo(&source.header);
			if (ERROR_SUCCESS == res) {
				if (wcscmp(displayName, source.viewGdiDeviceName) == 0) {
					displayConfig = {};

					if (path.sourceInfo.modeInfoIdx != DISPLAYCONFIG_PATH_MODE_IDX_INVALID) {
						const auto& mode = modes[path.sourceInfo.modeInfoIdx];
						if (mode.infoType == DISPLAYCONFIG_MODE_INFO_TYPE_SOURCE) {
							displayConfig.width  = mode.sourceMode.width;;
							displayConfig.height = mode.sourceMode.height;
						}
					}
					if (path.targetInfo.modeInfoIdx != DISPLAYCONFIG_PATH_MODE_IDX_INVALID) {
						const auto& mode = modes[path.targetInfo.modeInfoIdx];
						if (mode.infoType == DISPLAYCONFIG_MODE_INFO_TYPE_TARGET) {
							displayConfig.refreshRate      = mode.targetMode.targetVideoSignalInfo.vSyncFreq;
							displayConfig.scanLineOrdering = mode.targetMode.targetVideoSignalInfo.scanLineOrdering;
						}
						displayConfig.modeTarget = mode;

						DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO color_info = {
							{DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO, sizeof(color_info), mode.adapterId, mode.id}, {}
						};
						res = DisplayConfigGetDeviceInfo(&color_info.header);
						if (ERROR_SUCCESS == res) {
							displayConfig.colorEncoding = color_info.colorEncoding;
							displayConfig.bitsPerChannel = color_info.bitsPerColorChannel;
							displayConfig.advancedColor.value = color_info.value;
						}
					}

					if (!is_valid_refresh_rate(displayConfig.refreshRate)) {
						displayConfig.refreshRate = path.targetInfo.refreshRate;
						displayConfig.scanLineOrdering = path.targetInfo.scanLineOrdering;
						if (!is_valid_refresh_rate(displayConfig.refreshRate)) {
							displayConfig.refreshRate = { 0, 1 };
						}
					}

					displayConfig.outputTechnology = path.targetInfo.outputTechnology;
					memcpy(displayConfig.displayName, source.viewGdiDeviceName, sizeof(displayConfig.displayName));

					DISPLAYCONFIG_TARGET_DEVICE_NAME name= {
						{DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME, sizeof(name), path.sourceInfo.adapterId, path.targetInfo.id}, {},
					};
					res = DisplayConfigGetDeviceInfo(&name.header);
					if (ERROR_SUCCESS == res) {
						memcpy(displayConfig.monitorName, name.monitorFriendlyDeviceName, sizeof(displayConfig.monitorName));
					} else {
						ZeroMemory(displayConfig.monitorName, sizeof(displayConfig.monitorName));
					}

					return true;
				}
			}
		}
	}

	return false;
}

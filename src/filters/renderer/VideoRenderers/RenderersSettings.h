/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2022 see Authors.txt
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

#include "IAllocatorPresenter.h"

#define WM_MYMOUSELAST WM_XBUTTONDBLCLK
#define DXVA2VP 1
#define DXVAHDVP 1

#define RS_EVRBUFFERS_MIN	4
#define RS_EVRBUFFERS_DEF	5
#define RS_EVRBUFFERS_MAX	30

enum : int {
	//VIDRNDT_VMR7 = 0, // obsolete
	//VIDRNDT_VMR9_W = 1, // obsolete
	VIDRNDT_EVR = 2,
	VIDRNDT_EVR_CP,
	VIDRNDT_SYNC,
	VIDRNDT_MPCVR,
	VIDRNDT_DXR,
	VIDRNDT_MADVR,
	VIDRNDT_NULL_ANY = 10,
	VIDRNDT_NULL_UNCOMP,
};

enum :int {
	RESIZER_NEAREST = 0,
	RESIZER_BILINEAR,
	RESIZER_DXVA2,
	RESIZER_SHADER_BICUBIC06,
	RESIZER_SHADER_BICUBIC08,
	RESIZER_SHADER_BICUBIC10,
	RESIZER_SHADER_SMOOTHERSTEP, // no longer used
	RESIZER_SHADER_BSPLINE,
	RESIZER_SHADER_MITCHELL,
	RESIZER_SHADER_CATMULL,
	RESIZER_SHADER_LANCZOS2,
	RESIZER_SHADER_LANCZOS3,
	RESIZER_DXVAHD = 50,

	DOWNSCALER_SIMPLE = 100,
	DOWNSCALER_BOX,
	DOWNSCALER_BILINEAR,
	DOWNSCALER_HAMMING,
	DOWNSCALER_BICUBIC,
	DOWNSCALER_LANCZOS,
};

enum VideoSystem {
	VIDEO_SYSTEM_UNKNOWN = 0,
	VIDEO_SYSTEM_HDTV,
	VIDEO_SYSTEM_SDTV_NTSC,
	VIDEO_SYSTEM_SDTV_PAL,
};

enum AmbientLight {
	AMBIENT_LIGHT_BRIGHT = 0,
	AMBIENT_LIGHT_DIM,
	AMBIENT_LIGHT_DARK,
};

enum ColorRenderingIntent {
	COLOR_RENDERING_INTENT_PERCEPTUAL = 0,
	COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
	COLOR_RENDERING_INTENT_SATURATION,
	COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC,
};

enum {
	SYNCHRONIZE_NEAREST = 0,
	SYNCHRONIZE_VIDEO,
	SYNCHRONIZE_DISPLAY,
};

class CRenderersSettings
{
public:
	int		iVideoRenderer;

	// device
	CString	sD3DRenderDevice;
	bool	bResetDevice;

	bool	bExclusiveFullscreen;
	bool	b10BitOutput;

	// surfaces and resizer
	int		iPresentMode;
	D3DFORMAT iSurfaceFormat;
	int		iResizer;
	int		iDownscaler;

	// frame synchronization
	bool	bVSync;
	bool	bVSyncInternal;
	bool	bEVRFrameTimeCorrection;
	bool	bFlushGPUBeforeVSync;
	bool	bFlushGPUAfterPresent;
	bool	bFlushGPUWait;

	// EVR
	int		iEVROutputRange;
	int		nEVRBuffers;

	bool	bMPCVRFullscreenControl = false; // experimental, changes only in the registry 

	// SyncRenderer settings
	int		iSynchronizeMode;
	int		iLineDelta;
	int		iColumnDelta;
	double	dCycleDelta;
	double	dTargetSyncOffset;
	double	dControlLimit;

	// color management
	bool	bColorManagementEnable;
	int		iColorManagementInput;
	int		iColorManagementAmbientLight;
	int		iColorManagementIntent;

	// subtitles
	SubpicSettings SubpicSets;
	Stereo3DSettings Stereo3DSets;

	bool	bTearingTest;       // not saved
	int		iDisplayStats;      // not saved

	CRenderersSettings();

	void	SetDefault();
	void	Load();
	void	Save();
};

class CAffectingRenderersSettings // used in SettingsNeedResetDevice()
{
public:
	int iPresentMode                = 0;
	D3DFORMAT iSurfaceFormat        = D3DFMT_X8R8G8B8;
	bool b10BitOutput               = false;
	bool bVSync                     = false;

	void Fill(const CRenderersSettings& rs)
	{
		iPresentMode               = rs.iPresentMode;
		iSurfaceFormat             = rs.iSurfaceFormat;
		b10BitOutput               = rs.b10BitOutput;
		bVSync                     = rs.bVSync;
	}
};

extern CRenderersSettings& GetRenderersSettings();

extern bool LoadResource(UINT resid, CStringA& str, LPCWSTR restype);

HINSTANCE GetD3X9Dll();

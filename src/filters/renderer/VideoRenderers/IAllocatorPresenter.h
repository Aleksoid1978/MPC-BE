/*
 * (C) 2022 see Authors.txt
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

#include "SubPic/ISubPic.h"

#define TARGET_FRAME 0
#define TARGET_SCREEN 1

#define RS_EVRBUFFERS_MIN	4
#define RS_EVRBUFFERS_DEF	5
#define RS_EVRBUFFERS_MAX	30

#define RS_SPCSIZE_MIN		0
#define RS_SPCSIZE_DEF		10
#define RS_SPCSIZE_MAX		60

enum {
	SUBPIC_STEREO_NONE = 0,
	SUBPIC_STEREO_SIDEBYSIDE,
	SUBPIC_STEREO_TOPANDBOTTOM,
};

enum {
	STEREO3D_AsIs = 0,
	STEREO3D_HalfOverUnder_to_Interlace,
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

struct SubpicSettings {
	int   iPosRelative       = 0;        // not saved
	POINT ShiftPos           = { 0, 0 }; // not saved
	int   nCount             = RS_SPCSIZE_DEF;
	int   iMaxTexWidth       = 1280;
	bool  bAnimationWhenBuffering = true;
	bool  bAllowDrop         = true;
};

struct Stereo3DSettings {
	int   iMode      = SUBPIC_STEREO_NONE;
	int   iTransform = STEREO3D_AsIs; // not saved
	bool  bSwapLR    = false;         // not saved
};

struct ExtraRendererSettings {
	int       iPresentMode   = 0;
	D3DFORMAT iSurfaceFormat = D3DFMT_X8R8G8B8;
	bool      b10BitOutput   = false;
	bool      bVSync         = false;

	CStringW  sD3DRenderDevice;
	bool      bResetDevice   = false;

	bool      bVSyncInternal = false;
	bool      bFlushGPUBeforeVSync = false;
	bool      bFlushGPUAfterPresent = false;
	bool      bFlushGPUWait  = false;

	int       iResizer       = RESIZER_SHADER_CATMULL;
	int       iDownscaler    = DOWNSCALER_SIMPLE;
	int       iEVROutputRange = 0;
	int       nEVRBuffers    = 5;
	bool      bEVRFrameTimeCorrection = false;

	int       iDisplayStats  = 0;     // not saved
	bool      bTearingTest   = false; // not saved

	// color management
	bool      bColorManagementEnable       = false;
	int       iColorManagementInput        = VIDEO_SYSTEM_UNKNOWN;
	int       iColorManagementAmbientLight = AMBIENT_LIGHT_BRIGHT;
	int       iColorManagementIntent       = COLOR_RENDERING_INTENT_PERCEPTUAL;

	// SyncRenderer settings
	int       iSynchronizeMode  = SYNCHRONIZE_NEAREST;
	int       iLineDelta        = 0;
	int       iColumnDelta      = 0;
	double    dCycleDelta       = 0.0012;
	double    dTargetSyncOffset = 12.0;
	double    dControlLimit     = 2.0;

	bool      bMPCVRFullscreenControl = false;
};

//
// IAllocatorPresenter (MPC-BE internal interface)
//

interface __declspec(uuid("AD863F43-83F9-4B8E-962C-426F2BDBEAEF"))
IAllocatorPresenter :
public IUnknown {
	STDMETHOD(DisableSubPicInitialization) () PURE;
	STDMETHOD(EnablePreviewModeInitialization) () PURE;

	STDMETHOD (CreateRenderer) (IUnknown** ppRenderer) PURE;

	STDMETHOD_(CLSID, GetAPCLSID) () PURE;

	STDMETHOD_(SIZE, GetVideoSize) () PURE;
	STDMETHOD_(SIZE, GetVideoSizeAR) () PURE;
	STDMETHOD_(void, SetPosition) (RECT w, RECT v) PURE;
	STDMETHOD (SetRotation) (int rotation) PURE;
	STDMETHOD_(int, GetRotation) () PURE;
	STDMETHOD (SetFlip) (bool flip) PURE;
	STDMETHOD_(bool, GetFlip) () PURE;
	STDMETHOD_(bool, Paint) (bool fAll) PURE;

	STDMETHOD_(void, SetTime) (REFERENCE_TIME rtNow) PURE;
	STDMETHOD_(void, SetSubtitleDelay) (int delay_ms) PURE;
	STDMETHOD_(int, GetSubtitleDelay) () PURE;
	STDMETHOD_(double, GetFPS) () PURE;

	STDMETHOD_(void, SetSubPicProvider) (ISubPicProvider* pSubPicProvider) PURE;
	STDMETHOD_(void, Invalidate) (REFERENCE_TIME rtInvalidate = -1) PURE;

	STDMETHOD (GetDIB) (BYTE* lpDib, DWORD* size) PURE; // may be deleted in the future
	STDMETHOD (GetVideoFrame) (BYTE* lpDib, DWORD* size) PURE;
	STDMETHOD (GetDisplayedImage) (LPVOID* dibImage) PURE;

	STDMETHOD_(int, GetPixelShaderMode) () PURE;
	STDMETHOD (ClearPixelShaders) (int target) PURE;
	STDMETHOD (AddPixelShader) (int target, LPCWSTR name, LPCSTR profile, LPCSTR sourceCode) PURE;

	STDMETHOD_(bool, ResizeDevice) () PURE;
	STDMETHOD_(bool, ResetDevice) () PURE;
	STDMETHOD_(bool, DisplayChange) () PURE;
	STDMETHOD_(void, ResetStats) () PURE;

	STDMETHOD_(bool, IsRendering)() PURE;

	STDMETHOD_(void, SetSubpicSettings) (SubpicSettings* pSubpicSets) PURE;
	STDMETHOD_(void, SetStereo3DSettings) (Stereo3DSettings* pStereo3DSets) PURE;
	STDMETHOD_(void, SetExtraSettings) (ExtraRendererSettings* pExtraSets) PURE;
};

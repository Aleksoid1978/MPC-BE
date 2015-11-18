/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2015 see Authors.txt
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

#define WM_MYMOUSELAST WM_XBUTTONDBLCLK
#define ENABLE_2PASS_RESIZE 0
#define DXVAVP 1

#define RS_EVRBUFFERS_MIN	4
#define RS_EVRBUFFERS_DEF	5
#define RS_EVRBUFFERS_MAX	60
#define RS_SPCSIZE_MIN		0
#define RS_SPCSIZE_DEF		10
#define RS_SPCSIZE_MAX		60


enum {
	WM_REARRANGERENDERLESS = WM_APP+1,
	WM_RESET_DEVICE,
};

enum {
	VIDRNDT_AP_SURFACE,
	VIDRNDT_AP_TEXTURE2D,
	VIDRNDT_AP_TEXTURE3D,
};

enum :int {
	RESIZER_UNKNOWN = -1,
	RESIZER_NEAREST = 0,
	RESIZER_BILINEAR,
	RESIZER_DXVA2,
	RESIZER_SHADER_BICUBIC06,
	RESIZER_SHADER_BICUBIC08,
	RESIZER_SHADER_BICUBIC10,
	RESIZER_SHADER_SMOOTHERSTEP,
	RESIZER_SHADER_BSPLINE4,
	RESIZER_SHADER_MITCHELL4,
	RESIZER_SHADER_CATMULL4,
	RESIZER_SHADER_LANCZOS2,
	RESIZER_SHADER_LANCZOS3,

	RESIZER_SHADER_AVERAGE = 100, // for statistical purposes only. not for settings
};

enum VideoSystem {
	VIDEO_SYSTEM_UNKNOWN,
	VIDEO_SYSTEM_HDTV,
	VIDEO_SYSTEM_SDTV_NTSC,
	VIDEO_SYSTEM_SDTV_PAL,
};

enum AmbientLight {
	AMBIENT_LIGHT_BRIGHT,
	AMBIENT_LIGHT_DIM,
	AMBIENT_LIGHT_DARK,
};

enum ColorRenderingIntent {
	COLOR_RENDERING_INTENT_PERCEPTUAL,
	COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
	COLOR_RENDERING_INTENT_SATURATION,
	COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC,
};

class CRenderersSettings
{

public:
	bool fResetDevice;

	// Subtitle position settings
	int bPositionRelative;

	// Stereoscopic Subtitles
	BOOL bStereoDisabled;
	BOOL bSideBySide;
	BOOL bTopAndBottom;

	// Subtitle position
	CPoint nShiftPos;

	class CAdvRendererSettings
	{
	public:
		CAdvRendererSettings() {SetDefault();}

		bool   fVMR9AlterativeVSync;
		int    iVMR9VSyncOffset;
		bool   iVMR9VSyncAccurate;
		bool   iVMR9VSync;
		bool   iVMR9ColorManagementEnable;
		int    iVMR9ColorManagementInput;
		int    iVMR9ColorManagementAmbientLight;
		int    iVMR9ColorManagementIntent;
		bool   iVMRDisableDesktopComposition;
		int    iVMRFlushGPUBeforeVSync;
		int    iVMRFlushGPUAfterPresent;
		int    iVMRFlushGPUWait;
		int    iDX9SurfaceFormat;
		bool   b10BitOutput;

		// EVR
		bool   iEVREnableFrameTimeCorrection;
		int    iEVROutputRange;

		// SyncRenderer settings
		int    bSynchronizeVideo;
		int    bSynchronizeDisplay;
		int    bSynchronizeNearest;
		int    iLineDelta;
		int    iColumnDelta;
		double fCycleDelta;
		double fTargetSyncOffset;
		double fControlLimit;

		void SetDefault();
	};

	CAdvRendererSettings m_AdvRendSets;

	int			iAPSurfaceUsage;
	int			iDX9Resizer;
	bool		fVMRMixerMode;
	bool		fVMRMixerYUV;

	int			iEvrBuffers;

	int			nSPCSize;
	int			nSPMaxTexRes;
	bool		bSPCAllowAnimationWhenBuffering;
	bool		bSPAllowDropSubPic;

	CString		D3D9RenderDevice;
	void		SaveRenderers();
	void		LoadRenderers();
};

class CRenderersData
{
	HINSTANCE	m_hD3DX9Dll;
	HINSTANCE	m_hD3DCompilerDll;

public:
	CRenderersData();

	// Casimir666
	bool		m_fTearingTest;
	int			m_fDisplayStats;
	bool		m_bResetStats; // Set to reset the presentation statistics
	CString		m_strD3DX9Version;

	// Hardware feature support
	bool		m_bFP16Support;
	bool		m_b10bitSupport;

	HINSTANCE	GetD3X9Dll();
	HINSTANCE	GetD3DCompilerDll();
};

extern CRenderersData*		GetRenderersData();
extern CRenderersSettings&	GetRenderersSettings();

extern bool LoadResource(UINT resid, CStringA& str, LPCTSTR restype);

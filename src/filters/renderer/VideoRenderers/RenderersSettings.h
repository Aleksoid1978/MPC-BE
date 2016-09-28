/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2016 see Authors.txt
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
#define DXVAVP 1

#define RS_EVRBUFFERS_MIN	4
#define RS_EVRBUFFERS_DEF	5
#define RS_EVRBUFFERS_MAX	30
#define RS_SPCSIZE_MIN		0
#define RS_SPCSIZE_DEF		10
#define RS_SPCSIZE_MAX		60


enum {
	WM_REARRANGERENDERLESS = WM_APP+1,
	WM_RESET_DEVICE,
};

enum {
	SURFACE_OFFSCREEN,
	SURFACE_TEXTURE2D,
	SURFACE_TEXTURE3D,
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

enum {
	SYNCHRONIZE_NEAREST,
	SYNCHRONIZE_VIDEO,
	SYNCHRONIZE_DISPLAY,
};

enum {
	SUBPIC_STEREO_NONE,
	SUBPIC_STEREO_SIDEBYSIDE,
	SUBPIC_STEREO_TOPANDBOTTOM,
};

enum {
	STEREO3D_AsIs = 0,
	STEREO3D_HalfOverUnder_to_Interlace,
};

class CRenderersSettings
{
public:
	// device
	CString	sD3DRenderDevice;
	bool	bResetDevice;

	// surfaces and resizer
	int		iSurfaceType;
	int		iSurfaceFormat;
	bool	b10BitOutput;
	int		iResizer;

	// frame synchronization
	bool	bVSync;
	bool	bVSyncAccurate;
	bool	bAlterativeVSync;
	int		iVSyncOffset;
	bool	bDisableDesktopComposition;
	bool	bEVRFrameTimeCorrection;
	bool	bFlushGPUBeforeVSync;
	bool	bFlushGPUAfterPresent;
	bool	bFlushGPUWait;

	// VMR
	bool	bVMRMixerMode;
	bool	bVMRMixerYUV;

	// EVR
	int		iEVROutputRange;
	int		nEVRBuffers;

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
	int		bSubpicPosRelative;
	CPoint	SubpicShiftPos;
	int		nSubpicCount;
	int		iSubpicMaxTexWidth;
	bool	bSubpicAnimationWhenBuffering;
	bool	bSubpicAllowDrop;
	int		iSubpicStereoMode;

	CRenderersSettings();

	void	SetDefault();
	void	SaveRenderers();
	//void	LoadRenderers();
};

class CAffectingRenderersSettings // used in SettingsNeedResetDevice()
{
public:
	int iSurfaceFormat		= D3DFMT_X8R8G8B8;
	bool b10BitOutput		= false;

	bool bVSyncAccurate		= false;
	bool bAlterativeVSync	= false;
	bool bDisableDesktopComposition = false;

	void Fill(CRenderersSettings& rs)
	{
		iSurfaceFormat		= rs.iSurfaceFormat;
		b10BitOutput		= rs.b10BitOutput;
		bVSyncAccurate		= rs.bVSyncAccurate;
		bAlterativeVSync	= rs.bAlterativeVSync;
	}
};

class CRenderersData
{
public:
	CRenderersData();

	bool		m_bTearingTest;
	int			m_iDisplayStats;
	bool		m_bResetStats; // Set to reset the presentation statistics
	int			m_iStereo3DTransform;

	// Hardware feature support
	bool		m_bFP16Support;
	bool		m_b10bitSupport;
};

extern CRenderersSettings&	GetRenderersSettings();
extern CRenderersData*		GetRenderersData();

extern bool LoadResource(UINT resid, CStringA& str, LPCTSTR restype);

HINSTANCE GetD3X9Dll();

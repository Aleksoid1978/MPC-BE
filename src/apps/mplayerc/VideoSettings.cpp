/*
 * (C) 2022-2024 see Authors.txt
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
#include <clsids.h>
#include "VideoSettings.h"

#define IDS_R_VIDEO					L"Video"
#define IDS_RS_COLOR_BRIGHTNESS		L"ColorBrightness"
#define IDS_RS_COLOR_CONTRAST		L"ColorContrast"
#define IDS_RS_COLOR_HUE			L"ColorHue"
#define IDS_RS_COLOR_SATURATION		L"ColorSaturation"

#define IDS_RS_VIDEORENDERER		L"VideoRenderer"

#define IDS_RS_RENDERDEVICE			L"RenderDevice"
#define IDS_RS_RESETDEVICE			L"ResetDevice"

#define IDS_RS_EXCLUSIVEFULLSCREEN	L"ExclusiveFullscreen"
#define IDS_RS_OUTPUT10BIT			L"Output10Bit"

#define IDS_RS_PRESENTMODE			L"PresentMode"
#define IDS_RS_SURFACEFORMAT		L"SurfaceFormat"
#define IDS_RS_RESIZER				L"Resizer"
#define IDS_RS_DOWNSCALER			L"Downscaler"

#define IDS_RS_VSYNC				L"VSync"
#define IDS_RS_VSYNC_INTERNAL		L"VSyncInternal"
#define IDS_RS_FRAMETIMECORRECTION	L"FrameTimeCorrection"
#define IDS_RS_FLUSHGPUBEFOREVSYNC	L"FlushGPUBeforeVSync"
#define IDS_RS_FLUSHGPUAFTERPRESENT	L"FlushGPUAfterPresent"
#define IDS_RS_FLUSHGPUWAIT			L"FlushGPUWait"

#define IDS_RS_EVR_OUTPUTRANGE		L"EVROutputRange"
#define IDS_RS_EVR_BUFFERS			L"EVRBuffers"

#define IDS_RS_MPCVR_FSCONTROL		L"MPCVRFullscreenControl"

#define IDS_RS_SYNC_MODE			L"SyncMode"
#define IDS_RS_SYNC_CYCLEDELTA		L"SyncCycleDelta"
#define IDS_RS_SYNC_TARGETOFFSET	L"SyncTargetOffset"
#define IDS_RS_SYNC_CONTROLLIMIT	L"SyncControlLimit"

#define IDS_RS_COLMAN				L"ColorManagement"
#define IDS_RS_COLMAN_INPUT			L"ColorManagementInput"
#define IDS_RS_COLMAN_AMBIENTLIGHT	L"ColorManagementAmbientLight"
#define IDS_RS_COLMAN_INTENT		L"ColorManagementIntent"

#define IDS_RS_SUBPIC_COUNT			L"SubpicCount"
#define IDS_RS_SUBPIC_MAXTEXWIDTH	L"SubpicMaxTexWidth"
#define IDS_RS_SUBPIC_ANIMBUFFERING	L"SubpicAnimationWhenBuffering"
#define IDS_RS_SUBPIC_ALLOWDROP		L"SubpicAllowDrop"
#define IDS_RS_SUBPIC_STEREOMODE	L"SubpicStereoMode"

void CRenderersSettings::SetDefault()
{
	HRESULT hr = CheckFilterCLSID(CLSID_MPCVR);
	if (S_OK == hr) {
		iVideoRenderer = VIDRNDT_MPCVR;
	} else {
		iVideoRenderer = VIDRNDT_EVR_CP;
	};

	bExclusiveFullscreen = false;

	SubpicSets   = {};
	Stereo3DSets = {};
	ExtraSets    = {};
}

void CRenderersSettings::Load()
{
	CProfile& profile = AfxGetProfile();

	profile.ReadInt(IDS_R_VIDEO, IDS_RS_VIDEORENDERER, iVideoRenderer);
	iVideoRenderer = discard(iVideoRenderer, (int)VIDRNDT_EVR_CP,
		{VIDRNDT_EVR,
		VIDRNDT_EVR_CP,
		VIDRNDT_SYNC,
		VIDRNDT_MPCVR,
		VIDRNDT_DXR,
		VIDRNDT_MADVR,
		VIDRNDT_NULL_ANY,
		VIDRNDT_NULL_UNCOMP,
		});
	profile.ReadBool(IDS_R_VIDEO, IDS_RS_EXCLUSIVEFULLSCREEN, bExclusiveFullscreen);

	profile.ReadInt(IDS_R_VIDEO, IDS_RS_COLOR_BRIGHTNESS, iBrightness, -100, 100);
	profile.ReadInt(IDS_R_VIDEO, IDS_RS_COLOR_CONTRAST, iContrast, -100, 100);
	profile.ReadInt(IDS_R_VIDEO, IDS_RS_COLOR_HUE, iHue, -180, 180);
	profile.ReadInt(IDS_R_VIDEO, IDS_RS_COLOR_SATURATION, iSaturation, -100, 100);

	profile.ReadString(IDS_R_VIDEO, IDS_RS_RENDERDEVICE, ExtraSets.sD3DRenderDevice);
	profile.ReadBool(IDS_R_VIDEO, IDS_RS_RESETDEVICE, ExtraSets.bResetDevice);

	profile.ReadBool(IDS_R_VIDEO, IDS_RS_OUTPUT10BIT, ExtraSets.b10BitOutput);
	profile.ReadInt(IDS_R_VIDEO, IDS_RS_PRESENTMODE, ExtraSets.iPresentMode);
	int val;
	profile.ReadInt(IDS_R_VIDEO, IDS_RS_SURFACEFORMAT, val);
	if (val == D3DFMT_A32B32G32R32F) { // is no longer supported, because it is very redundant.
		ExtraSets.iSurfaceFormat = D3DFMT_A16B16G16R16F;
	} else {
		ExtraSets.iSurfaceFormat = discard((D3DFORMAT)val, D3DFMT_X8R8G8B8, { D3DFMT_A2R10G10B10 , D3DFMT_A16B16G16R16F });
	}
	profile.ReadInt(IDS_R_VIDEO, IDS_RS_RESIZER, ExtraSets.iResizer);
	profile.ReadInt(IDS_R_VIDEO, IDS_RS_DOWNSCALER, ExtraSets.iDownscaler);

	profile.ReadBool(IDS_R_VIDEO, IDS_RS_VSYNC, ExtraSets.bVSync);
	profile.ReadBool(IDS_R_VIDEO, IDS_RS_VSYNC_INTERNAL, ExtraSets.bVSyncInternal);
	profile.ReadBool(IDS_R_VIDEO, IDS_RS_FRAMETIMECORRECTION, ExtraSets.bEVRFrameTimeCorrection);
	profile.ReadBool(IDS_R_VIDEO, IDS_RS_FLUSHGPUBEFOREVSYNC, ExtraSets.bFlushGPUBeforeVSync);
	profile.ReadBool(IDS_R_VIDEO, IDS_RS_FLUSHGPUAFTERPRESENT, ExtraSets.bFlushGPUAfterPresent);
	profile.ReadBool(IDS_R_VIDEO, IDS_RS_FLUSHGPUWAIT, ExtraSets.bFlushGPUWait);

	profile.ReadInt(IDS_R_VIDEO, IDS_RS_EVR_OUTPUTRANGE, ExtraSets.iEVROutputRange);
	profile.ReadInt(IDS_R_VIDEO, IDS_RS_EVR_BUFFERS, ExtraSets.nEVRBuffers);

	profile.ReadBool(IDS_R_VIDEO, IDS_RS_MPCVR_FSCONTROL, ExtraSets.bMPCVRFullscreenControl);

	profile.ReadInt(IDS_R_VIDEO, IDS_RS_SYNC_MODE, ExtraSets.iSynchronizeMode);
	profile.ReadDouble(IDS_R_VIDEO, IDS_RS_SYNC_CYCLEDELTA, ExtraSets.dCycleDelta);
	profile.ReadDouble(IDS_R_VIDEO, IDS_RS_SYNC_TARGETOFFSET, ExtraSets.dTargetSyncOffset);
	profile.ReadDouble(IDS_R_VIDEO, IDS_RS_SYNC_CONTROLLIMIT, ExtraSets.dControlLimit);

	profile.ReadBool(IDS_R_VIDEO, IDS_RS_COLMAN, ExtraSets.bColorManagementEnable);
	profile.ReadInt(IDS_R_VIDEO, IDS_RS_COLMAN_INPUT, ExtraSets.iColorManagementInput);
	profile.ReadInt(IDS_R_VIDEO, IDS_RS_COLMAN_AMBIENTLIGHT, ExtraSets.iColorManagementAmbientLight);
	profile.ReadInt(IDS_R_VIDEO, IDS_RS_COLMAN_INTENT, ExtraSets.iColorManagementIntent);

	profile.ReadInt(IDS_R_VIDEO, IDS_RS_SUBPIC_COUNT, SubpicSets.nCount);
	profile.ReadInt(IDS_R_VIDEO, IDS_RS_SUBPIC_MAXTEXWIDTH, SubpicSets.iMaxTexWidth);
	profile.ReadBool(IDS_R_VIDEO, IDS_RS_SUBPIC_ANIMBUFFERING, SubpicSets.bAnimationWhenBuffering);
	profile.ReadBool(IDS_R_VIDEO, IDS_RS_SUBPIC_ALLOWDROP, SubpicSets.bAllowDrop);
	profile.ReadInt(IDS_R_VIDEO, IDS_RS_SUBPIC_STEREOMODE, Stereo3DSets.iMode);

	SubpicSets.iPosRelative = 0;
}

void CRenderersSettings::Save()
{
	CProfile& profile = AfxGetProfile();

	profile.WriteInt(IDS_R_VIDEO, IDS_RS_VIDEORENDERER, iVideoRenderer);
	profile.WriteBool(IDS_R_VIDEO, IDS_RS_EXCLUSIVEFULLSCREEN, bExclusiveFullscreen);

	profile.WriteInt(IDS_R_VIDEO, IDS_RS_COLOR_BRIGHTNESS, iBrightness);
	profile.WriteInt(IDS_R_VIDEO, IDS_RS_COLOR_CONTRAST, iContrast);
	profile.WriteInt(IDS_R_VIDEO, IDS_RS_COLOR_HUE, iHue);
	profile.WriteInt(IDS_R_VIDEO, IDS_RS_COLOR_SATURATION, iSaturation);

	profile.WriteString(IDS_R_VIDEO, IDS_RS_RENDERDEVICE, ExtraSets.sD3DRenderDevice);
	profile.WriteBool(IDS_R_VIDEO, IDS_RS_RESETDEVICE, ExtraSets.bResetDevice);

	profile.WriteBool(IDS_R_VIDEO, IDS_RS_OUTPUT10BIT, ExtraSets.b10BitOutput);
	profile.WriteInt(IDS_R_VIDEO, IDS_RS_PRESENTMODE, ExtraSets.iPresentMode);
	profile.WriteInt(IDS_R_VIDEO, IDS_RS_SURFACEFORMAT, ExtraSets.iSurfaceFormat);
	profile.WriteInt(IDS_R_VIDEO, IDS_RS_RESIZER, ExtraSets.iResizer);
	profile.WriteInt(IDS_R_VIDEO, IDS_RS_DOWNSCALER, ExtraSets.iDownscaler);

	profile.WriteBool(IDS_R_VIDEO, IDS_RS_VSYNC, ExtraSets.bVSync);
	profile.WriteBool(IDS_R_VIDEO, IDS_RS_VSYNC_INTERNAL, ExtraSets.bVSyncInternal);
	profile.WriteBool(IDS_R_VIDEO, IDS_RS_FRAMETIMECORRECTION, ExtraSets.bEVRFrameTimeCorrection);
	profile.WriteBool(IDS_R_VIDEO, IDS_RS_FLUSHGPUBEFOREVSYNC, ExtraSets.bFlushGPUBeforeVSync);
	profile.WriteBool(IDS_R_VIDEO, IDS_RS_FLUSHGPUAFTERPRESENT, ExtraSets.bFlushGPUAfterPresent);
	profile.WriteBool(IDS_R_VIDEO, IDS_RS_FLUSHGPUWAIT, ExtraSets.bFlushGPUWait);

	profile.WriteInt(IDS_R_VIDEO, IDS_RS_EVR_OUTPUTRANGE, ExtraSets.iEVROutputRange);
	profile.WriteInt(IDS_R_VIDEO, IDS_RS_EVR_BUFFERS, ExtraSets.nEVRBuffers);

	profile.WriteBool(IDS_R_VIDEO, IDS_RS_MPCVR_FSCONTROL, ExtraSets.bMPCVRFullscreenControl);

	profile.WriteInt(IDS_R_VIDEO, IDS_RS_SYNC_MODE, ExtraSets.iSynchronizeMode);
	profile.WriteDouble(IDS_R_VIDEO, IDS_RS_SYNC_CYCLEDELTA, ExtraSets.dCycleDelta);
	profile.WriteDouble(IDS_R_VIDEO, IDS_RS_SYNC_TARGETOFFSET, ExtraSets.dTargetSyncOffset);
	profile.WriteDouble(IDS_R_VIDEO, IDS_RS_SYNC_CONTROLLIMIT, ExtraSets.dControlLimit);

	profile.WriteBool(IDS_R_VIDEO, IDS_RS_COLMAN, ExtraSets.bColorManagementEnable);
	profile.WriteInt(IDS_R_VIDEO, IDS_RS_COLMAN_INPUT, ExtraSets.iColorManagementInput);
	profile.WriteInt(IDS_R_VIDEO, IDS_RS_COLMAN_AMBIENTLIGHT, ExtraSets.iColorManagementAmbientLight);
	profile.WriteInt(IDS_R_VIDEO, IDS_RS_COLMAN_INTENT, ExtraSets.iColorManagementIntent);

	profile.WriteInt(IDS_R_VIDEO, IDS_RS_SUBPIC_COUNT, SubpicSets.nCount);
	profile.WriteInt(IDS_R_VIDEO, IDS_RS_SUBPIC_MAXTEXWIDTH, SubpicSets.iMaxTexWidth);
	profile.WriteBool(IDS_R_VIDEO, IDS_RS_SUBPIC_ANIMBUFFERING, SubpicSets.bAnimationWhenBuffering);
	profile.WriteBool(IDS_R_VIDEO, IDS_RS_SUBPIC_ALLOWDROP, SubpicSets.bAllowDrop);
	profile.WriteInt(IDS_R_VIDEO, IDS_RS_SUBPIC_STEREOMODE, Stereo3DSets.iMode);
}

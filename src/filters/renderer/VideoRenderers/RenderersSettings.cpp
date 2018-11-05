/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2018 see Authors.txt
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
#include "RenderersSettings.h"
#include "../../../apps/mplayerc/mplayerc.h"

#define IDS_R_VIDEO					L"Settings\\Video"

#define IDS_RS_VIDEORENDERER		L"VideoRenderer"

#define IDS_RS_RENDERDEVICE			L"RenderDevice"
#define IDS_RS_RESETDEVICE			L"ResetDevice"

#define IDS_RS_SURFACEFORMAT		L"SurfaceFormat"
#define IDS_RS_OUTPUT10BIT			L"Output10Bit"
#define IDS_RS_RESIZER				L"Resizer"
#define IDS_RS_DOWNSCALER			L"Downscaler"

#define IDS_RS_VSYNC				L"VSync"
#define IDS_RS_VSYNC_ACCURATE		L"VSyncAccurate"
#define IDS_RS_VSYNC_ALTERNATE		L"VSyncAlternate"
#define IDS_RS_VSYNC_OFFSET			L"VSyncOffset"
#define IDS_RS_DISABLEDESKCOMP		L"DisableDesktopComposition"
#define IDS_RS_FRAMETIMECORRECTION	L"FrameTimeCorrection"
#define IDS_RS_FLUSHGPUBEFOREVSYNC	L"FlushGPUBeforeVSync"
#define IDS_RS_FLUSHGPUAFTERPRESENT	L"FlushGPUAfterPresent"
#define IDS_RS_FLUSHGPUWAIT			L"FlushGPUWait"

#define IDS_RS_VMR_MIXERMODE		L"VMRMixerMode"
#define IDS_RS_VMR_MIXERYUV			L"VMRMixerYUV"

#define IDS_RS_EVR_OUTPUTRANGE		L"EVROutputRange"
#define IDS_RS_EVR_BUFFERS			L"EVRBuffers"

#define IDS_RS_SYNC_MODE			L"SyncMode"
#define IDS_RS_SYNC_LINEDELTA		L"SyncLineDelta"
#define IDS_RS_SYNC_COLUMNDELTA		L"SyncColumnDelta"
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

CRenderersSettings::CRenderersSettings()
{
	SetDefault();
}

void CRenderersSettings::SetDefault()
{
	iVideoRenderer					= VIDRNDT_EVR_CUSTOM;

	sD3DRenderDevice.Empty();
	bResetDevice					= false;

	iSurfaceFormat					= D3DFMT_X8R8G8B8;
	b10BitOutput					= false;
	iResizer						= RESIZER_SHADER_CATMULL;
	iDownscaler						= DOWNSCALER_SIMPLE;

	bVSync							= false;
	bVSyncAccurate					= false;
	bAlterativeVSync				= false;
	iVSyncOffset					= 0;
	bDisableDesktopComposition		= false;
	bEVRFrameTimeCorrection			= false;
	bFlushGPUBeforeVSync			= false;
	bFlushGPUAfterPresent			= false;
	bFlushGPUWait					= false;

	bVMRMixerMode					= true;
	bVMRMixerYUV					= true;

	iEVROutputRange					= 0;
	nEVRBuffers						= 5;

	iSynchronizeMode				= SYNCHRONIZE_NEAREST;
	iLineDelta						= 0;
	iColumnDelta					= 0;
	dCycleDelta						= 0.0012;
	dTargetSyncOffset				= 12.0;
	dControlLimit					= 2.0;

	bColorManagementEnable			= false;
	iColorManagementInput			= VIDEO_SYSTEM_UNKNOWN;
	iColorManagementAmbientLight	= AMBIENT_LIGHT_BRIGHT;
	iColorManagementIntent			= COLOR_RENDERING_INTENT_PERCEPTUAL;

	iSubpicPosRelative				= 0;
	SubpicShiftPos.SetPoint(0, 0);
	nSubpicCount					= RS_SPCSIZE_DEF;
	iSubpicMaxTexWidth				= 1280;
	bSubpicAnimationWhenBuffering	= true;
	bSubpicAllowDrop				= true;
	iSubpicStereoMode				= 0;

	bTearingTest					= false;
	iDisplayStats					= 0;
	bResetStats						= false;
	iStereo3DTransform				= STEREO3D_AsIs;
	bStereo3DSwapLR					= false;
}

void CRenderersSettings::Load()
{
	CProfile& profile = AfxGetProfile();

	profile.ReadInt(IDS_R_VIDEO, IDS_RS_VIDEORENDERER, iVideoRenderer);

	profile.ReadString(IDS_R_VIDEO, IDS_RS_RENDERDEVICE, sD3DRenderDevice);
	profile.ReadBool(IDS_R_VIDEO, IDS_RS_RESETDEVICE, bResetDevice);

	int val;
	profile.ReadInt(IDS_R_VIDEO, IDS_RS_SURFACEFORMAT, val);
	if (val == D3DFMT_A32B32G32R32F) { // is no longer supported, because it is very redundant.
		iSurfaceFormat = D3DFMT_A16B16G16R16F;
	} else {
		iSurfaceFormat = discard((D3DFORMAT)val, D3DFMT_X8R8G8B8, { D3DFMT_A2R10G10B10 , D3DFMT_A16B16G16R16F });
	}

	profile.ReadBool(IDS_R_VIDEO, IDS_RS_OUTPUT10BIT, b10BitOutput);
	profile.ReadInt(IDS_R_VIDEO, IDS_RS_RESIZER, iResizer);
	profile.ReadInt(IDS_R_VIDEO, IDS_RS_DOWNSCALER, iDownscaler);

	profile.ReadBool(IDS_R_VIDEO, IDS_RS_VSYNC, bVSync);
	profile.ReadBool(IDS_R_VIDEO, IDS_RS_VSYNC_ACCURATE, bVSyncAccurate);
	profile.ReadBool(IDS_R_VIDEO, IDS_RS_VSYNC_ALTERNATE, bAlterativeVSync);
	profile.ReadInt(IDS_R_VIDEO, IDS_RS_VSYNC_OFFSET, iVSyncOffset);
	profile.ReadBool(IDS_R_VIDEO, IDS_RS_DISABLEDESKCOMP, bDisableDesktopComposition);
	profile.ReadBool(IDS_R_VIDEO, IDS_RS_FRAMETIMECORRECTION, bEVRFrameTimeCorrection);
	profile.ReadBool(IDS_R_VIDEO, IDS_RS_FLUSHGPUBEFOREVSYNC, bFlushGPUBeforeVSync);
	profile.ReadBool(IDS_R_VIDEO, IDS_RS_FLUSHGPUAFTERPRESENT, bFlushGPUAfterPresent);
	profile.ReadBool(IDS_R_VIDEO, IDS_RS_FLUSHGPUWAIT, bFlushGPUWait);

	profile.ReadBool(IDS_R_VIDEO, IDS_RS_VMR_MIXERMODE, bVMRMixerMode);
	profile.ReadBool(IDS_R_VIDEO, IDS_RS_VMR_MIXERYUV, bVMRMixerYUV);

	profile.ReadInt(IDS_R_VIDEO, IDS_RS_EVR_OUTPUTRANGE, iEVROutputRange);
	profile.ReadInt(IDS_R_VIDEO, IDS_RS_EVR_BUFFERS, nEVRBuffers);

	profile.ReadInt(IDS_R_VIDEO, IDS_RS_SYNC_MODE, iSynchronizeMode);
	profile.ReadInt(IDS_R_VIDEO, IDS_RS_SYNC_LINEDELTA, iLineDelta);
	profile.ReadInt(IDS_R_VIDEO, IDS_RS_SYNC_COLUMNDELTA, iColumnDelta);
	double* dPtr;
	UINT dSize;
	if (profile.ReadBinary(IDS_R_VIDEO, IDS_RS_SYNC_CYCLEDELTA, (BYTE**)&dPtr, dSize) && dSize == 8) {
		dCycleDelta = *dPtr;
		delete[] dPtr;
	}
	if (profile.ReadBinary(IDS_R_VIDEO, IDS_RS_SYNC_TARGETOFFSET, (BYTE**)&dPtr, dSize) && dSize == 8) {
		dTargetSyncOffset = *dPtr;
		delete[] dPtr;
	}
	if (profile.ReadBinary(IDS_R_VIDEO, IDS_RS_SYNC_CONTROLLIMIT, (BYTE**)&dPtr, dSize) && dSize == 8) {
		dControlLimit = *dPtr;
		delete[] dPtr;
	}

	profile.ReadBool(IDS_R_VIDEO, IDS_RS_COLMAN, bColorManagementEnable);
	profile.ReadInt(IDS_R_VIDEO, IDS_RS_COLMAN_INPUT, iColorManagementInput);
	profile.ReadInt(IDS_R_VIDEO, IDS_RS_COLMAN_AMBIENTLIGHT, iColorManagementAmbientLight);
	profile.ReadInt(IDS_R_VIDEO, IDS_RS_COLMAN_INTENT, iColorManagementIntent);

	profile.ReadInt(IDS_R_VIDEO, IDS_RS_SUBPIC_COUNT, nSubpicCount);
	profile.ReadInt(IDS_R_VIDEO, IDS_RS_SUBPIC_MAXTEXWIDTH, iSubpicMaxTexWidth);
	profile.ReadBool(IDS_R_VIDEO, IDS_RS_SUBPIC_ANIMBUFFERING, bSubpicAnimationWhenBuffering);
	profile.ReadBool(IDS_R_VIDEO, IDS_RS_SUBPIC_ALLOWDROP, bSubpicAllowDrop);
	profile.ReadInt(IDS_R_VIDEO, IDS_RS_SUBPIC_STEREOMODE, iSubpicStereoMode);

	iSubpicPosRelative = 0;
}

void CRenderersSettings::Save()
{
	CProfile& profile = AfxGetProfile();

	profile.WriteInt(IDS_R_VIDEO, IDS_RS_VIDEORENDERER, iVideoRenderer);

	profile.WriteString(IDS_R_VIDEO, IDS_RS_RENDERDEVICE, sD3DRenderDevice);
	profile.WriteBool(IDS_R_VIDEO, IDS_RS_RESETDEVICE, bResetDevice);

	profile.WriteInt(IDS_R_VIDEO, IDS_RS_SURFACEFORMAT, iSurfaceFormat);
	profile.WriteBool(IDS_R_VIDEO, IDS_RS_OUTPUT10BIT, b10BitOutput);
	profile.WriteInt(IDS_R_VIDEO, IDS_RS_RESIZER, iResizer);
	profile.WriteInt(IDS_R_VIDEO, IDS_RS_DOWNSCALER, iDownscaler);

	profile.WriteBool(IDS_R_VIDEO, IDS_RS_VSYNC, bVSync);
	profile.WriteBool(IDS_R_VIDEO, IDS_RS_VSYNC_ACCURATE, bVSyncAccurate);
	profile.WriteBool(IDS_R_VIDEO, IDS_RS_VSYNC_ALTERNATE, bAlterativeVSync);
	profile.WriteInt(IDS_R_VIDEO, IDS_RS_VSYNC_OFFSET, iVSyncOffset);
	profile.WriteBool(IDS_R_VIDEO, IDS_RS_DISABLEDESKCOMP, bDisableDesktopComposition);
	profile.WriteBool(IDS_R_VIDEO, IDS_RS_FRAMETIMECORRECTION, bEVRFrameTimeCorrection);
	profile.WriteBool(IDS_R_VIDEO, IDS_RS_FLUSHGPUBEFOREVSYNC, bFlushGPUBeforeVSync);
	profile.WriteBool(IDS_R_VIDEO, IDS_RS_FLUSHGPUAFTERPRESENT, bFlushGPUAfterPresent);
	profile.WriteBool(IDS_R_VIDEO, IDS_RS_FLUSHGPUWAIT, bFlushGPUWait);

	profile.WriteBool(IDS_R_VIDEO, IDS_RS_VMR_MIXERMODE, bVMRMixerMode);
	profile.WriteBool(IDS_R_VIDEO, IDS_RS_VMR_MIXERYUV, bVMRMixerYUV);

	profile.WriteInt(IDS_R_VIDEO, IDS_RS_EVR_OUTPUTRANGE, iEVROutputRange);
	profile.WriteInt(IDS_R_VIDEO, IDS_RS_EVR_BUFFERS, nEVRBuffers);

	profile.WriteInt(IDS_R_VIDEO, IDS_RS_SYNC_MODE, iSynchronizeMode);
	profile.WriteInt(IDS_R_VIDEO, IDS_RS_SYNC_LINEDELTA, iLineDelta);
	profile.WriteInt(IDS_R_VIDEO, IDS_RS_SYNC_COLUMNDELTA, iColumnDelta);
	profile.WriteBinary(IDS_R_VIDEO, IDS_RS_SYNC_CYCLEDELTA, (LPBYTE)&(dCycleDelta), sizeof(dCycleDelta));
	profile.WriteBinary(IDS_R_VIDEO, IDS_RS_SYNC_TARGETOFFSET, (LPBYTE)&(dTargetSyncOffset), sizeof(dTargetSyncOffset));
	profile.WriteBinary(IDS_R_VIDEO, IDS_RS_SYNC_CONTROLLIMIT, (LPBYTE)&(dControlLimit), sizeof(dControlLimit));

	profile.WriteBool(IDS_R_VIDEO, IDS_RS_COLMAN, bColorManagementEnable);
	profile.WriteInt(IDS_R_VIDEO, IDS_RS_COLMAN_INPUT, iColorManagementInput);
	profile.WriteInt(IDS_R_VIDEO, IDS_RS_COLMAN_AMBIENTLIGHT, iColorManagementAmbientLight);
	profile.WriteInt(IDS_R_VIDEO, IDS_RS_COLMAN_INTENT, iColorManagementIntent);

	profile.WriteInt(IDS_R_VIDEO, IDS_RS_SUBPIC_COUNT, nSubpicCount);
	profile.WriteInt(IDS_R_VIDEO, IDS_RS_SUBPIC_MAXTEXWIDTH, iSubpicMaxTexWidth);
	profile.WriteBool(IDS_R_VIDEO, IDS_RS_SUBPIC_ANIMBUFFERING, bSubpicAnimationWhenBuffering);
	profile.WriteBool(IDS_R_VIDEO, IDS_RS_SUBPIC_ALLOWDROP, bSubpicAllowDrop);
	profile.WriteInt(IDS_R_VIDEO, IDS_RS_SUBPIC_STEREOMODE, iSubpicStereoMode);
}

/////////////////////////////////////////////////////////////////////////////

HINSTANCE GetD3X9Dll()
{
	static HINSTANCE s_hD3DX9Dll = LoadLibraryW(L"d3dx9_43.dll"); // load latest compatible version of the DLL that is available

	return s_hD3DX9Dll;
}

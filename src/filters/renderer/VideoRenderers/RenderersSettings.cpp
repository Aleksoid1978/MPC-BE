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

#include "stdafx.h"
#include "RenderersSettings.h"
#include "../../../apps/mplayerc/mplayerc.h"
#include <version.h>

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
	bResetDevice					= true;

	iSurfaceFormat					= D3DFMT_X8R8G8B8;
	b10BitOutput					= false;
	iResizer						= 1;

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

	bSubpicPosRelative				= 0;
	SubpicShiftPos.SetPoint(0, 0);
	nSubpicCount					= RS_SPCSIZE_DEF;
	iSubpicMaxTexWidth				= 1280;
	bSubpicAnimationWhenBuffering	= true;
	bSubpicAllowDrop				= true;
	iSubpicStereoMode				= 0;
}

void CRenderersSettings::Load()
{
	CMPlayerCApp* pApp = AfxGetMyApp();

#define GET_OPTION_INT(value, name) value = pApp->GetProfileInt(IDS_R_VIDEO, name, value)
#define GET_OPTION_BOOL(value, name) value = !!pApp->GetProfileInt(IDS_R_VIDEO, name, value)

	GET_OPTION_INT(iVideoRenderer, IDS_RS_VIDEORENDERER);

	sD3DRenderDevice = pApp->GetProfileString(IDS_R_VIDEO, IDS_RS_RENDERDEVICE);
	GET_OPTION_BOOL(bResetDevice, IDS_RS_RESETDEVICE);

	GET_OPTION_INT(iSurfaceFormat, IDS_RS_SURFACEFORMAT);
	GET_OPTION_BOOL(b10BitOutput, IDS_RS_OUTPUT10BIT);
	GET_OPTION_INT(iResizer, IDS_RS_RESIZER);
	GET_OPTION_INT(iDownscaler, IDS_RS_DOWNSCALER);

	GET_OPTION_BOOL(bVSync, IDS_RS_VSYNC);
	GET_OPTION_BOOL(bVSyncAccurate, IDS_RS_VSYNC_ACCURATE);
	GET_OPTION_BOOL(bAlterativeVSync, IDS_RS_VSYNC_ALTERNATE);
	GET_OPTION_INT(iVSyncOffset, IDS_RS_VSYNC_OFFSET);
	GET_OPTION_BOOL(bDisableDesktopComposition, IDS_RS_DISABLEDESKCOMP);
	GET_OPTION_BOOL(bEVRFrameTimeCorrection, IDS_RS_FRAMETIMECORRECTION);
	GET_OPTION_BOOL(bFlushGPUBeforeVSync, IDS_RS_FLUSHGPUBEFOREVSYNC);
	GET_OPTION_BOOL(bFlushGPUAfterPresent, IDS_RS_FLUSHGPUAFTERPRESENT);
	GET_OPTION_BOOL(bFlushGPUWait, IDS_RS_FLUSHGPUWAIT);

	GET_OPTION_BOOL(bVMRMixerMode, IDS_RS_VMR_MIXERMODE);
	GET_OPTION_BOOL(bVMRMixerYUV, IDS_RS_VMR_MIXERYUV);

	GET_OPTION_INT(iEVROutputRange, IDS_RS_EVR_OUTPUTRANGE);
	GET_OPTION_INT(nEVRBuffers, IDS_RS_EVR_BUFFERS);

	GET_OPTION_INT(iSynchronizeMode, IDS_RS_SYNC_MODE);
	GET_OPTION_INT(iLineDelta, IDS_RS_SYNC_LINEDELTA);
	GET_OPTION_INT(iColumnDelta, IDS_RS_SYNC_COLUMNDELTA);
	double* dPtr;
	UINT dSize;
	if (pApp->GetProfileBinary(IDS_R_SETTINGS, IDS_RS_SYNC_CYCLEDELTA, (LPBYTE*)&dPtr, &dSize)) {
		dCycleDelta = *dPtr;
		delete[] dPtr;
	}
	if (pApp->GetProfileBinary(IDS_R_SETTINGS, IDS_RS_SYNC_TARGETOFFSET, (LPBYTE*)&dPtr, &dSize)) {
		dTargetSyncOffset = *dPtr;
		delete[] dPtr;
	}
	if (pApp->GetProfileBinary(IDS_R_SETTINGS, IDS_RS_SYNC_CONTROLLIMIT, (LPBYTE*)&dPtr, &dSize)) {
		dControlLimit = *dPtr;
		delete[] dPtr;
	}

	GET_OPTION_BOOL(bColorManagementEnable, IDS_RS_COLMAN);
	GET_OPTION_INT(iColorManagementInput, IDS_RS_COLMAN_INPUT);
	GET_OPTION_INT(iColorManagementAmbientLight, IDS_RS_COLMAN_AMBIENTLIGHT);
	GET_OPTION_INT(iColorManagementIntent, IDS_RS_COLMAN_INTENT);

	GET_OPTION_INT(nSubpicCount, IDS_RS_SUBPIC_COUNT);
	GET_OPTION_INT(iSubpicMaxTexWidth, IDS_RS_SUBPIC_MAXTEXWIDTH);
	GET_OPTION_BOOL(bSubpicAnimationWhenBuffering, IDS_RS_SUBPIC_ANIMBUFFERING);
	GET_OPTION_BOOL(bSubpicAllowDrop, IDS_RS_SUBPIC_ALLOWDROP);
	GET_OPTION_INT(iSubpicStereoMode, IDS_RS_SUBPIC_STEREOMODE);

	bSubpicPosRelative = false;

#undef GET_OPTION_INT
#undef GET_OPTION_BOOL
}

void CRenderersSettings::Save()
{
	CMPlayerCApp* pApp = AfxGetMyApp();

#define WRITE_OPTION_INT(value, name) pApp->WriteProfileInt(IDS_R_VIDEO, name, value)

	WRITE_OPTION_INT(iVideoRenderer, IDS_RS_VIDEORENDERER);

	pApp->WriteProfileString(IDS_R_VIDEO, IDS_RS_RENDERDEVICE, sD3DRenderDevice);
	WRITE_OPTION_INT(bResetDevice, IDS_RS_RESETDEVICE);

	WRITE_OPTION_INT(iSurfaceFormat, IDS_RS_SURFACEFORMAT);
	WRITE_OPTION_INT(b10BitOutput, IDS_RS_OUTPUT10BIT);
	WRITE_OPTION_INT(iResizer, IDS_RS_RESIZER);
	WRITE_OPTION_INT(iDownscaler, IDS_RS_DOWNSCALER);

	WRITE_OPTION_INT(bVSync, IDS_RS_VSYNC);
	WRITE_OPTION_INT(bVSyncAccurate, IDS_RS_VSYNC_ACCURATE);
	WRITE_OPTION_INT(bAlterativeVSync, IDS_RS_VSYNC_ALTERNATE);
	WRITE_OPTION_INT(iVSyncOffset, IDS_RS_VSYNC_OFFSET);
	WRITE_OPTION_INT(bDisableDesktopComposition, IDS_RS_DISABLEDESKCOMP);
	WRITE_OPTION_INT(bEVRFrameTimeCorrection, IDS_RS_FRAMETIMECORRECTION);
	WRITE_OPTION_INT(bFlushGPUBeforeVSync, IDS_RS_FLUSHGPUBEFOREVSYNC);
	WRITE_OPTION_INT(bFlushGPUAfterPresent, IDS_RS_FLUSHGPUAFTERPRESENT);
	WRITE_OPTION_INT(bFlushGPUWait, IDS_RS_FLUSHGPUWAIT);

	WRITE_OPTION_INT(bVMRMixerMode, IDS_RS_VMR_MIXERMODE);
	WRITE_OPTION_INT(bVMRMixerYUV, IDS_RS_VMR_MIXERYUV);

	WRITE_OPTION_INT(iEVROutputRange, IDS_RS_EVR_OUTPUTRANGE);
	WRITE_OPTION_INT(nEVRBuffers, IDS_RS_EVR_BUFFERS);

	WRITE_OPTION_INT(iSynchronizeMode, IDS_RS_SYNC_MODE);
	WRITE_OPTION_INT(iLineDelta, IDS_RS_SYNC_LINEDELTA);
	WRITE_OPTION_INT(iColumnDelta, IDS_RS_SYNC_COLUMNDELTA);
	pApp->WriteProfileBinary(IDS_R_SETTINGS, IDS_RS_SYNC_CYCLEDELTA, (LPBYTE)&(dCycleDelta), sizeof(dCycleDelta));
	pApp->WriteProfileBinary(IDS_R_SETTINGS, IDS_RS_SYNC_TARGETOFFSET, (LPBYTE)&(dTargetSyncOffset), sizeof(dTargetSyncOffset));
	pApp->WriteProfileBinary(IDS_R_SETTINGS, IDS_RS_SYNC_CONTROLLIMIT, (LPBYTE)&(dControlLimit), sizeof(dControlLimit));

	WRITE_OPTION_INT(bColorManagementEnable, IDS_RS_COLMAN);
	WRITE_OPTION_INT(iColorManagementInput, IDS_RS_COLMAN_INPUT);
	WRITE_OPTION_INT(iColorManagementAmbientLight, IDS_RS_COLMAN_AMBIENTLIGHT);
	WRITE_OPTION_INT(iColorManagementIntent, IDS_RS_COLMAN_INTENT);

	WRITE_OPTION_INT(nSubpicCount, IDS_RS_SUBPIC_COUNT);
	WRITE_OPTION_INT(iSubpicMaxTexWidth, IDS_RS_SUBPIC_MAXTEXWIDTH);
	WRITE_OPTION_INT(bSubpicAnimationWhenBuffering, IDS_RS_SUBPIC_ANIMBUFFERING);
	WRITE_OPTION_INT(bSubpicAllowDrop, IDS_RS_SUBPIC_ALLOWDROP);
	WRITE_OPTION_INT(iSubpicStereoMode, IDS_RS_SUBPIC_STEREOMODE);

#undef WRITE_OPTION_INT
}

/////////////////////////////////////////////////////////////////////////////
// CRenderersData construction

CRenderersData::CRenderersData()
	: m_bTearingTest(false)
	, m_iDisplayStats(0)
	, m_bResetStats(false)
	, m_iStereo3DTransform(STEREO3D_AsIs)
{
}


HINSTANCE GetD3X9Dll()
{
	static HINSTANCE s_hD3DX9Dll = LoadLibraryW(L"d3dx9_43.dll"); // load latest compatible version of the DLL that is available

	return s_hD3DX9Dll;
}

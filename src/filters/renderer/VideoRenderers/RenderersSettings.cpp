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

CRenderersSettings::CRenderersSettings()
{
	SetDefault();
}

void CRenderersSettings::SetDefault()
{
	sD3DRenderDevice.Empty();
	bResetDevice					= true;

	iSurfaceType					= SURFACE_TEXTURE3D;
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

void CRenderersSettings::SaveRenderers()
{
	AfxGetAppSettings().SaveRenderers();
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

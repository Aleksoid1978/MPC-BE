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

#include "stdafx.h"
#include "RenderersSettings.h"
#include "../../../apps/mplayerc/mplayerc.h"
#include <version.h>
#include <dx/d3dx9.h>

void CRenderersSettings::UpdateData(bool fSave)
{
	AfxGetAppSettings().UpdateRenderersData(fSave);
}

void CRenderersSettings::CAdvRendererSettings::SetDefault()
{
	fVMR9AlterativeVSync              = 0;
	iVMR9VSyncOffset                  = 0;
	iVMR9VSyncAccurate                = 0;
	iVMR9VSync                        = 0;
	iVMR9FullFloatingPointProcessing  = 0;
	iVMR9HalfFloatingPointProcessing  = 0;
	iVMR9ColorManagementEnable        = 0;
	iVMR9ColorManagementInput         = VIDEO_SYSTEM_UNKNOWN;
	iVMR9ColorManagementAmbientLight  = AMBIENT_LIGHT_BRIGHT;
	iVMR9ColorManagementIntent        = COLOR_RENDERING_INTENT_PERCEPTUAL;
	iVMRDisableDesktopComposition     = 0;
	iVMRFlushGPUBeforeVSync           = 0;
	iVMRFlushGPUAfterPresent          = 0;
	iVMRFlushGPUWait                  = 0;
	iEVRHighColorResolution           = 0;
	iEVRForceInputHighColorResolution = 0;
	iEVREnableFrameTimeCorrection     = 0;
	iEVROutputRange                   = 0;
	bSynchronizeVideo                 = 0;
	bSynchronizeDisplay               = 0;
	bSynchronizeNearest               = 1;
	iLineDelta                        = 0;
	iColumnDelta                      = 0;
	fCycleDelta                       = 0.0012;
	fTargetSyncOffset                 = 12.0;
	fControlLimit                     = 2.0;
}

void CRenderersSettings::CAdvRendererSettings::SetOptimal()
{
	fVMR9AlterativeVSync              = 0;
	iVMR9VSyncOffset                  = 0;
	iVMR9VSyncAccurate                = 0;
	iVMR9VSync                        = 0;
	iVMR9FullFloatingPointProcessing  = 1;
	iVMR9HalfFloatingPointProcessing  = 0;
	iVMR9ColorManagementEnable        = 0;
	iVMR9ColorManagementInput         = VIDEO_SYSTEM_UNKNOWN;
	iVMR9ColorManagementAmbientLight  = AMBIENT_LIGHT_BRIGHT;
	iVMR9ColorManagementIntent        = COLOR_RENDERING_INTENT_PERCEPTUAL;
	iVMRDisableDesktopComposition     = 1;
	iVMRFlushGPUBeforeVSync           = 0;
	iVMRFlushGPUAfterPresent          = 0;
	iVMRFlushGPUWait                  = 0;
	iEVRHighColorResolution           = 0;
	iEVRForceInputHighColorResolution = 0;
	iEVREnableFrameTimeCorrection     = 0;
	iEVROutputRange                   = 0;
	bSynchronizeVideo                 = 0;
	bSynchronizeDisplay               = 0;
	bSynchronizeNearest               = 1;
	iLineDelta                        = 0;
	iColumnDelta                      = 0;
	fCycleDelta                       = 0.0012;
	fTargetSyncOffset                 = 12.0;
	fControlLimit                     = 2.0;
}

/////////////////////////////////////////////////////////////////////////////
// CRenderersData construction

CRenderersData::CRenderersData()
{
	m_fTearingTest  = false;
	m_fDisplayStats = false;
	m_bResetStats   = false;
	m_hD3DX9Dll     = NULL;

	// Don't disable hardware features before initializing a renderer
	m_bFP16Support  = true;
	m_b10bitSupport = true;
}

LONGLONG CRenderersData::GetPerfCounter()
{
	LARGE_INTEGER i64Ticks100ns;
	LARGE_INTEGER llPerfFrequency;

	QueryPerformanceFrequency(&llPerfFrequency);
	if (llPerfFrequency.QuadPart != 0) {
		QueryPerformanceCounter(&i64Ticks100ns);
		return llMulDiv(i64Ticks100ns.QuadPart, 10000000, llPerfFrequency.QuadPart, 0);
	} else {
		// ms to 100ns units
		return timeGetTime() * 10000;
	}
}

HINSTANCE CRenderersData::GetD3X9Dll()
{
#if D3DX_SDK_VERSION < 43
#error DirectX SDK June 2010 (v43) or newer is required to build MPC-BE
#endif

	if (m_hD3DX9Dll == NULL) {
		// load latest compatible version of the DLL that is available
		m_hD3DX9Dll = LoadLibrary(L"d3dx9_43.dll");
	}

	return m_hD3DX9Dll;
}

HINSTANCE CRenderersData::GetD3DCompilerDll()
{
	if (m_hD3DCompilerDll == NULL) {
		// load latest compatible version of the DLL that is available
		m_hD3DCompilerDll = LoadLibrary(L"D3DCompiler_43.dll");
	}

	return m_hD3DCompilerDll;
}

/*
 *
 * (C) 2013-2016 see Authors.txt
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
 *
 */

#pragma once
#include <adl/adl_sdk.h>

class CGPUUsage
{
	typedef int (*ADL_MAIN_CONTROL_CREATE)(ADL_MAIN_MALLOC_CALLBACK, int);
	typedef int (*ADL_MAIN_CONTROL_DESTROY)();
	typedef int (*ADL_ADAPTER_NUMBEROFADAPTERS_GET)(int*);
	typedef int (*ADL_ADAPTER_ADAPTERINFO_GET)(LPAdapterInfo, int);
	typedef int (*ADL_ADAPTER_ACTIVE_GET)(int, int*);
	typedef int (*ADL_OVERDRIVE_CAPS)(int iAdapterIndex, int *iSupported, int *iEnabled, int *iVersion);

	typedef int (*ADL_OVERDRIVE5_ODPARAMETERS_GET)(int iAdapterIndex,  ADLODParameters *lpOdParameters);
	typedef int (*ADL_OVERDRIVE5_CURRENTACTIVITY_GET)(int iAdapterIndex, ADLPMActivity *lpActivity);

	typedef int (*ADL_OVERDRIVE6_CAPABILITIES_GET)(int iAdapterIndex, ADLOD6Capabilities *lpODCapabilities);
	typedef int	(*ADL_OVERDRIVE6_CURRENTSTATUS_GET)(int iAdapterIndex, ADLOD6CurrentStatus *lpCurrentStatus);

	#define OK                         0
	#define NVAPI_MAX_PHYSICAL_GPUS   64
	#define NVAPI_MAX_USAGES_PER_GPU  33
	#define NVAPI_MAX_PSTATES_PER_GPU  8
	struct gpuUsages {
		UINT version;
		UINT usage[NVAPI_MAX_USAGES_PER_GPU];
	};

	#define NVAPI_DOMAIN_GPU 0
	#define NVAPI_DOMAIN_VID 2
	struct gpuPStates {
		UINT version;
		UINT flag;
		struct {
			UINT present:1;
			UINT percent;
		} pstates[NVAPI_MAX_PSTATES_PER_GPU];
	};

	typedef char NvAPI_ShortString[64];

	typedef int *(*NvAPI_QueryInterface_t)(unsigned int offset);
	typedef int (*NvAPI_Initialize_t)();
	typedef int (*NvAPI_EnumPhysicalGPUs_t)(int **handles, int *count);
	typedef int (*NvAPI_GPU_GetUsages_t)(int *handle, gpuUsages *gpuUsages);
	typedef int (*NvAPI_GPU_GetPStates_t)(int *handle, gpuPStates *gpuPStates);
	typedef int (*NvAPI_GPU_GetFullName_t)(int *handle, NvAPI_ShortString gpuName);

public:
	enum GPUType {
		ATI_GPU,
		NVIDIA_GPU,
		UNKNOWN_GPU
	};

	CGPUUsage();
	~CGPUUsage();
	HRESULT Init(CString DeviceName, CString Device);

	const DWORD		GetUsage();
	const GPUType	GetType() { return m_GPUType; }

private:
	bool EnoughTimePassed();

	DWORD m_nGPUUsage;
	DWORD m_dwLastRun;

	GPUType m_GPUType;

	volatile LONG m_lRunCount;

	struct {
		HMODULE                            hAtiADL;
		int                                iAdapterId;
		int                                iOverdriveVersion;

		ADL_MAIN_CONTROL_DESTROY           ADL_Main_Control_Destroy;
		ADL_OVERDRIVE5_CURRENTACTIVITY_GET ADL_Overdrive5_CurrentActivity_Get;
		ADL_OVERDRIVE6_CURRENTSTATUS_GET   ADL_Overdrive6_CurrentStatus_Get;
	} ATIData;

	struct {
		HMODULE                hNVApi;
		int*                   gpuHandles[NVAPI_MAX_PHYSICAL_GPUS];
		gpuUsages              gpuUsages;
		gpuPStates             gpuPStates;
		int                    gpuSelected;

		NvAPI_GPU_GetUsages_t  NvAPI_GPU_GetUsages;
		NvAPI_GPU_GetPStates_t NvAPI_GPU_GetPStates;
	} NVData;

	void Clean();
};

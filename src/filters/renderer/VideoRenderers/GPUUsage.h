/*
 *
 * Copyright (C) 2013 Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
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

	#define NVAPI_MAX_PHYSICAL_GPUS		64
	#define NVAPI_MAX_USAGES_PER_GPU	34
	typedef int *(*NvAPI_QueryInterface_t)(unsigned int offset);
	typedef int (*NvAPI_Initialize_t)();
	typedef int (*NvAPI_EnumPhysicalGPUs_t)(int **handles, int *count);
	typedef int (*NvAPI_GPU_GetUsages_t)(int *handle, unsigned int *usages);

public:
	enum GPUType { ATI_GPU, NVIDIA_GPU, UNKNOWN_GPU };

	CGPUUsage();
	~CGPUUsage();
	HRESULT Init(CString DeviceName);

	const short		GetUsage();
	const GPUType	GetType() { return m_GPUType; }

private:
	bool EnoughTimePassed();

	short m_nGPUUsage;
	DWORD m_dwLastRun;

	GPUType m_GPUType;

	volatile LONG m_lRunCount;

	struct {
		HMODULE								hAtiADL;
		int									iAdapterId;
		ADL_MAIN_CONTROL_DESTROY			ADL_Main_Control_Destroy;
		ADL_OVERDRIVE5_CURRENTACTIVITY_GET	ADL_Overdrive5_CurrentActivity_Get;
		ADL_OVERDRIVE6_CURRENTSTATUS_GET	ADL_Overdrive6_CurrentStatus_Get;

		int									iOverdriveVersion;
	} ATIData;

	struct {
		HMODULE								hNVApi;
		int									*gpuHandles[NVAPI_MAX_PHYSICAL_GPUS];
		unsigned int						gpuUsages[NVAPI_MAX_USAGES_PER_GPU];
		NvAPI_GPU_GetUsages_t				NvAPI_GPU_GetUsages;

	} NVData;

	void Clean();
};

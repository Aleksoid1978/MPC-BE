/*
 * (C) 2013-2018 see Authors.txt
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
#include "include/d3dkmt.h"

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

	typedef int (*ADL2_OVERDRIVEN_CAPABILITIES_GET)(ADL_CONTEXT_HANDLE, int, ADLODNCapabilities*);
	typedef int (*ADL2_OVERDRIVEN_PERFORMANCESTATUS_GET)(ADL_CONTEXT_HANDLE, int, ADLODNPerformanceStatus*);

	#define OK                            0
	#define NVAPI_MAX_PHYSICAL_GPUS      64
	#define NVAPI_MAX_USAGES_PER_GPU     33
	#define NVAPI_MAX_PSTATES_PER_GPU     8
	#define NVAPI_MAX_CLOCKS_PER_GPU  0x120
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

	struct gpuClocks {
		UINT version;
		UINT clock[NVAPI_MAX_CLOCKS_PER_GPU];
	};

	typedef char NvAPI_ShortString[64];

	typedef int *(*NvAPI_QueryInterface_t)(unsigned int offset);
	typedef int (*NvAPI_Initialize_t)();
	typedef int (*NvAPI_EnumPhysicalGPUs_t)(int **handles, int *count);
	typedef int (*NvAPI_GPU_GetUsages_t)(int *handle, gpuUsages *gpuUsages);
	typedef int (*NvAPI_GPU_GetPStates_t)(int *handle, gpuPStates *gpuPStates);
	typedef int (*NvAPI_GPU_GetFullName_t)(int *handle, NvAPI_ShortString gpuName);
	typedef int (*NvAPI_GPU_GetAllClocks_t)(int *handle, gpuClocks *gpuClocks);

public:
	enum GPUType {
		ATI_GPU,
		NVIDIA_GPU,
		INTEL_GPU,
		OTHER_GPU,
		UNKNOWN_GPU
	};

	struct statistic {
		BYTE gpu = 0;
		BYTE decode = 0;
		BYTE processing = 0;
		UINT clock = 0;
		UINT64 memUsageCurrent = 0;
		UINT64 memUsageTotal = 0;
	};

	CGPUUsage();
	~CGPUUsage();
	HRESULT Init(const CString& DeviceName, const CString& Device);

	void    GetUsage(statistic& gpu_statistic);
	GPUType GetType() const { return m_GPUType; }

	bool    IsUseStatistics() const { return !gpuTimeStatistics.empty(); }

private:
	bool EnoughTimePassed();

	DWORD         m_dwLastRun = 0;
	volatile LONG m_lRunCount = 0;

	GPUType m_GPUType = UNKNOWN_GPU;
	statistic m_statistic = {};

	struct {
		HMODULE                               hAtiADL;
		int                                   iAdapterId;
		int                                   iOverdriveVersion;

		ADL_MAIN_CONTROL_DESTROY              ADL_Main_Control_Destroy;
		ADL_OVERDRIVE5_CURRENTACTIVITY_GET    ADL_Overdrive5_CurrentActivity_Get;
		ADL_OVERDRIVE6_CURRENTSTATUS_GET      ADL_Overdrive6_CurrentStatus_Get;
		ADL2_OVERDRIVEN_PERFORMANCESTATUS_GET ADL2_OverdriveN_PerformanceStatus_Get;
	} ATIData;

	struct {
		HMODULE                  hNVApi;
		int*                     gpuHandles[NVAPI_MAX_PHYSICAL_GPUS];
		gpuUsages                gpuUsages;
		gpuPStates               gpuPStates;
		gpuClocks                gpuClocks;
		int                      gpuSelected;

		NvAPI_GPU_GetUsages_t    NvAPI_GPU_GetUsages;
		NvAPI_GPU_GetPStates_t   NvAPI_GPU_GetPStates;
		NvAPI_GPU_GetAllClocks_t NvAPI_GPU_GetAllClocks;
	} NVData;

	PFND3DKMT_OPENADAPTERFROMHDC pD3DKMTOpenAdapterFromHdc = nullptr;
	PFND3DKMT_CLOSEADAPTER pD3DKMTCloseAdapter = nullptr;
	PFND3DKMT_QUERYSTATISTICS pD3DKMTQueryStatistics = nullptr;

	D3DKMT_HANDLE AdapterHandle = 0;
	LUID AdapterLuid = {};

	HANDLE processHandle = nullptr;

	struct LONGLONG_DELTA {
		LONGLONG Value;
		LONGLONG Delta;
	};
	struct gpuTimeStatistic {
		LONGLONG_DELTA gputotalRunningTime;
		LONGLONG runningTime;
	};
	std::vector <gpuTimeStatistic> gpuTimeStatistics;

	LONGLONG_DELTA totalRunning = {};

	void Clean();
};

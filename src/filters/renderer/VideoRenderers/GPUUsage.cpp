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

#include "StdAfx.h"
#include "GPUUsage.h"

// Memory allocation function
static void* __stdcall ADL_Main_Memory_Alloc(int iSize)
{
	void* lpBuffer = malloc(iSize);
	return lpBuffer;
}

CGPUUsage::CGPUUsage()
	: m_nGPUUsage(0)
	, m_dwLastRun(0)
	, m_lRunCount(0)
	, m_GPUType(UNKNOWN_GPU)
{
	Clean();
}

CGPUUsage::~CGPUUsage()
{
	Clean();
}

void CGPUUsage::Clean()
{
	if (m_GPUType == ATI_GPU) {
		if (ATIData.ADL_Main_Control_Destroy) {
			ATIData.ADL_Main_Control_Destroy();
		}

		if (ATIData.hAtiADL) {
			FreeLibrary(ATIData.hAtiADL);
		}
	} else if (m_GPUType == NVIDIA_GPU && NVData.hNVApi) {
		FreeLibrary(NVData.hNVApi);
	}

	m_GPUType = UNKNOWN_GPU;

	ATIData.iAdapterId							= -1;
	ATIData.iOverdriveVersion					= 0;
	ATIData.hAtiADL								= NULL;
	ATIData.ADL_Main_Control_Destroy			= NULL;
	ATIData.ADL_Overdrive5_CurrentActivity_Get	= NULL;
	ATIData.ADL_Overdrive6_CurrentStatus_Get	= NULL;

	ZeroMemory(&NVData.gpuHandles, sizeof(NVData.gpuHandles));
	ZeroMemory(&NVData.gpuUsages, sizeof(NVData.gpuUsages));
	NVData.gpuUsages[0]							= (NVAPI_MAX_USAGES_PER_GPU * 4) | 0x10000;
	NVData.hNVApi								= NULL;
	NVData.NvAPI_GPU_GetUsages					= NULL;
}


HRESULT CGPUUsage::Init(CString DeviceName)
{
	Clean();

	{
		// ATI OverDrive
		ATIData.hAtiADL = LoadLibrary(L"atiadlxx.dll");
		if (ATIData.hAtiADL == NULL) {
			ATIData.hAtiADL = LoadLibrary(L"atiadlxy.dll");
		}

		if (ATIData.hAtiADL) {
			ADL_MAIN_CONTROL_CREATE				ADL_Main_Control_Create;
			ADL_ADAPTER_NUMBEROFADAPTERS_GET	ADL_Adapter_NumberOfAdapters_Get;
			ADL_ADAPTER_ADAPTERINFO_GET			ADL_Adapter_AdapterInfo_Get;
			ADL_ADAPTER_ACTIVE_GET				ADL_Adapter_Active_Get;
			ADL_OVERDRIVE_CAPS					ADL_Overdrive_Caps;

			ADL_Main_Control_Create				= (ADL_MAIN_CONTROL_CREATE)GetProcAddress(ATIData.hAtiADL,"ADL_Main_Control_Create");
			ATIData.ADL_Main_Control_Destroy	= (ADL_MAIN_CONTROL_DESTROY)GetProcAddress(ATIData.hAtiADL,"ADL_Main_Control_Destroy");
			ADL_Adapter_NumberOfAdapters_Get	= (ADL_ADAPTER_NUMBEROFADAPTERS_GET)GetProcAddress(ATIData.hAtiADL,"ADL_Adapter_NumberOfAdapters_Get");
			ADL_Adapter_AdapterInfo_Get			= (ADL_ADAPTER_ADAPTERINFO_GET)GetProcAddress(ATIData.hAtiADL,"ADL_Adapter_AdapterInfo_Get");
			ADL_Adapter_Active_Get				= (ADL_ADAPTER_ACTIVE_GET)GetProcAddress(ATIData.hAtiADL, "ADL_Adapter_Active_Get");
			ADL_Overdrive_Caps					= (ADL_OVERDRIVE_CAPS)GetProcAddress(ATIData.hAtiADL, "ADL_Overdrive_Caps");

			if (NULL == ADL_Main_Control_Create ||
					NULL == ATIData.ADL_Main_Control_Destroy ||
					NULL == ADL_Adapter_NumberOfAdapters_Get ||
					NULL == ADL_Adapter_AdapterInfo_Get ||
					NULL == ADL_Adapter_Active_Get ||
					NULL == ADL_Overdrive_Caps) {
				Clean();
			}

			if (ATIData.hAtiADL) {
				int iNumberAdapters = 0;
				if (ADL_OK == ADL_Main_Control_Create(ADL_Main_Memory_Alloc, 1) && ADL_OK == ADL_Adapter_NumberOfAdapters_Get(&iNumberAdapters) && iNumberAdapters) {
					LPAdapterInfo lpAdapterInfo = DNew AdapterInfo[iNumberAdapters];
					if (lpAdapterInfo) {
						memset(lpAdapterInfo, 0, sizeof(AdapterInfo) * iNumberAdapters);

						// Get the AdapterInfo structure for all adapters in the system
						ADL_Adapter_AdapterInfo_Get(lpAdapterInfo, sizeof(AdapterInfo) * iNumberAdapters);
						for (int i = 0; i < iNumberAdapters; i++ ) {
							int adapterActive = 0;
							AdapterInfo adapterInfo = lpAdapterInfo[i];
							ADL_Adapter_Active_Get(adapterInfo.iAdapterIndex, &adapterActive);
							if (adapterActive && adapterInfo.iAdapterIndex >= 0 && adapterInfo.iVendorID == 1002) {
								if (DeviceName.GetLength() && DeviceName != CString(adapterInfo.strDisplayName)) {
									continue;
								}
								int iOverdriveSupported	= 0;
								int iOverdriveEnabled	= 0;
								if (ADL_OK != ADL_Overdrive_Caps(adapterInfo.iAdapterIndex, &iOverdriveSupported, &iOverdriveEnabled, &ATIData.iOverdriveVersion) || !iOverdriveSupported) {
									break;
								}

								if (ATIData.iOverdriveVersion == 5) {
									ADL_OVERDRIVE5_ODPARAMETERS_GET ADL_Overdrive5_ODParameters_Get;

									ADL_Overdrive5_ODParameters_Get				= (ADL_OVERDRIVE5_ODPARAMETERS_GET)GetProcAddress(ATIData.hAtiADL, "ADL_Overdrive5_ODParameters_Get");
									ATIData.ADL_Overdrive5_CurrentActivity_Get	= (ADL_OVERDRIVE5_CURRENTACTIVITY_GET)GetProcAddress(ATIData.hAtiADL, "ADL_Overdrive5_CurrentActivity_Get");
									if (NULL == ADL_Overdrive5_ODParameters_Get || NULL == ATIData.ADL_Overdrive5_CurrentActivity_Get) {
										break;
									}

									ADLODParameters overdriveParameters = {0};
									overdriveParameters.iSize = sizeof (ADLODParameters);
									if (ADL_OK != ADL_Overdrive5_ODParameters_Get(adapterInfo.iAdapterIndex, &overdriveParameters) || !overdriveParameters.iActivityReportingSupported) {
										break;
									}

									ATIData.iAdapterId = adapterInfo.iAdapterIndex;
								} else if (ATIData.iOverdriveVersion == 6) {
									ADL_OVERDRIVE6_CAPABILITIES_GET ADL_Overdrive6_Capabilities_Get;
									ADL_Overdrive6_Capabilities_Get				= (ADL_OVERDRIVE6_CAPABILITIES_GET)GetProcAddress(ATIData.hAtiADL, "ADL_Overdrive6_Capabilities_Get");
									ATIData.ADL_Overdrive6_CurrentStatus_Get	= (ADL_OVERDRIVE6_CURRENTSTATUS_GET)GetProcAddress(ATIData.hAtiADL, "ADL_Overdrive6_CurrentStatus_Get");
									if (NULL == ADL_Overdrive6_Capabilities_Get || NULL == ATIData.ADL_Overdrive6_CurrentStatus_Get) {
										break;
									}

									ADLOD6Capabilities od6Capabilities = {0};
									if (ADL_OK != ADL_Overdrive6_Capabilities_Get(adapterInfo.iAdapterIndex, &od6Capabilities) || (od6Capabilities.iCapabilities & ADL_OD6_CAPABILITY_GPU_ACTIVITY_MONITOR) != ADL_OD6_CAPABILITY_GPU_ACTIVITY_MONITOR) {
										break;
									}

									ATIData.iAdapterId = adapterInfo.iAdapterIndex;
								}

								break;
							}
						}

						delete [] lpAdapterInfo;
					}
				}
			}
		}

		if (ATIData.iAdapterId == -1 && ATIData.hAtiADL) {
			Clean();
		}

		if (ATIData.iAdapterId >= 0 && ATIData.hAtiADL) {
			m_GPUType = ATI_GPU;
		}
	}

	if (m_GPUType == UNKNOWN_GPU) {
		// NVApi
		NVData.hNVApi = LoadLibrary(L"nvapi64.dll");
		if (NVData.hNVApi == NULL) {
			NVData.hNVApi = LoadLibrary(L"nvapi.dll");
		}

		if (NVData.hNVApi) {
			NvAPI_QueryInterface_t		NvAPI_QueryInterface	= (NvAPI_QueryInterface_t)GetProcAddress(NVData.hNVApi, "nvapi_QueryInterface");
			NvAPI_Initialize_t			NvAPI_Initialize		= NULL;
			NvAPI_EnumPhysicalGPUs_t	NvAPI_EnumPhysicalGPUs	= NULL;
			if (NvAPI_QueryInterface) {
				NvAPI_Initialize			= (NvAPI_Initialize_t)(NvAPI_QueryInterface)(0x0150E828);
				NvAPI_EnumPhysicalGPUs		= (NvAPI_EnumPhysicalGPUs_t)(NvAPI_QueryInterface)(0xE5AC921F);
				NVData.NvAPI_GPU_GetUsages	= (NvAPI_GPU_GetUsages_t)(NvAPI_QueryInterface)(0x189A1FDF);
			}
			if (NULL == NvAPI_QueryInterface ||
					NULL == NvAPI_Initialize ||
					NULL == NvAPI_EnumPhysicalGPUs ||
					NULL == NVData.NvAPI_GPU_GetUsages) {
				Clean();
			}

			if (NVData.hNVApi) {
				NvAPI_Initialize();

				int gpuCount = 0;
				NvAPI_EnumPhysicalGPUs(NVData.gpuHandles, &gpuCount);
				if (!gpuCount) {
					Clean();
				}
			}
		}

		if (NVData.hNVApi && NVData.NvAPI_GPU_GetUsages) {
			m_GPUType = NVIDIA_GPU;
		}
	}

	return m_GPUType == UNKNOWN_GPU ? E_FAIL : S_OK;
}

const short CGPUUsage::GetUsage()
{
	short nGPUCopy = m_nGPUUsage;
	if (m_GPUType != UNKNOWN_GPU && ::InterlockedIncrement(&m_lRunCount) == 1) {
		if (!EnoughTimePassed()) {
			::InterlockedDecrement(&m_lRunCount);
			return nGPUCopy;
		}

		if (m_GPUType == ATI_GPU) {
			if (ATIData.iOverdriveVersion == 5) {
				ADLPMActivity activity = {0};
				activity.iSize = sizeof(ADLPMActivity);
				if (ADL_OK == ATIData.ADL_Overdrive5_CurrentActivity_Get(ATIData.iAdapterId, &activity)) {
					m_nGPUUsage = activity.iActivityPercent;
				}
			} else if (ATIData.iOverdriveVersion == 6) {
				ADLOD6CurrentStatus currentStatus = {0};
				if (ADL_OK == ATIData.ADL_Overdrive6_CurrentStatus_Get(ATIData.iAdapterId, &currentStatus)) {
					m_nGPUUsage = currentStatus.iActivityPercent;
				}
			}
		} else if (m_GPUType == NVIDIA_GPU) {
			NVData.NvAPI_GPU_GetUsages(NVData.gpuHandles[0], NVData.gpuUsages);
			m_nGPUUsage = NVData.gpuUsages[3];
		}

		m_dwLastRun	= GetTickCount();
		nGPUCopy	= m_nGPUUsage;
	}

	::InterlockedDecrement(&m_lRunCount);

	return nGPUCopy;
}

bool CGPUUsage::EnoughTimePassed()
{
	const int minElapsedMS = 1000;
	return (GetTickCount() - m_dwLastRun) >= minElapsedMS;
}

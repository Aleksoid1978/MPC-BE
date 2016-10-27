/*
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

#include "StdAfx.h"
#include "GPUUsage.h"
#include "../../../DSUtil/SysVersion.h"

// Memory allocation function
static void* __stdcall ADL_Main_Memory_Alloc(int iSize)
{
	return malloc(iSize);
}

CGPUUsage::CGPUUsage()
	: m_iGPUUsage(0)
	, m_iGPUClock(0)
	, m_llGPUdedicatedBytesUsedTotal(0)
	, m_llGPUdedicatedBytesUsedCurrent(0)
	, m_dwLastRun(0)
	, m_lRunCount(0)
	, m_GPUType(UNKNOWN_GPU)
{
	Clean();

	if (IsWin7orLater()) {
		gdi32Handle = LoadLibrary(L"gdi32.dll");
		if (gdi32Handle) {
			pD3DKMTQueryStatistics = (PFND3DKMT_QUERYSTATISTICS)GetProcAddress(gdi32Handle, "D3DKMTQueryStatistics");

			dxgiHandle = LoadLibrary(L"dxgi.dll");
			if (dxgiHandle) {
				pCreateDXGIFactory = (CreateDXGIFactory_t)GetProcAddress(dxgiHandle, "CreateDXGIFactory");
			}
		}

		processHandle = GetCurrentProcess();
	}
}

CGPUUsage::~CGPUUsage()
{
	Clean();

	if (gdi32Handle) {
		FreeLibrary(gdi32Handle);
	}

	if (dxgiHandle) {
		FreeLibrary(dxgiHandle);
	}
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
	NVData.gpuUsages.version					= sizeof(gpuUsages) | 0x10000;
	NVData.NvAPI_GPU_GetUsages					= NULL;

	ZeroMemory(&NVData.gpuPStates, sizeof(NVData.gpuPStates));
	NVData.gpuPStates.version					= sizeof(gpuPStates) | 0x10000;
	NVData.NvAPI_GPU_GetPStates					= NULL;

	ZeroMemory(&NVData.gpuClocks, sizeof(NVData.gpuClocks));
	NVData.gpuClocks.version					= sizeof(gpuClocks) | 0x20000;
	NVData.NvAPI_GPU_GetAllClocks				= NULL;

	NVData.gpuSelected							= -1;

	NVData.hNVApi								= NULL;

	ZeroMemory(&dxgiAdapterDesc, sizeof(dxgiAdapterDesc));
}


HRESULT CGPUUsage::Init(CString DeviceName, CString Device)
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

	if (m_GPUType == UNKNOWN_GPU && Device.Find(L"NVIDIA") == 0) {
		// NVApi
		NVData.hNVApi = LoadLibrary(L"nvapi64.dll");
		if (NVData.hNVApi == NULL) {
			NVData.hNVApi = LoadLibrary(L"nvapi.dll");
		}

		if (NVData.hNVApi) {
			NvAPI_QueryInterface_t   NvAPI_QueryInterface   = (NvAPI_QueryInterface_t)GetProcAddress(NVData.hNVApi, "nvapi_QueryInterface");
			NvAPI_Initialize_t       NvAPI_Initialize       = NULL;
			NvAPI_EnumPhysicalGPUs_t NvAPI_EnumPhysicalGPUs = NULL;
			NvAPI_GPU_GetFullName_t  NvAPI_GPU_GetFullName  = NULL;
			if (NvAPI_QueryInterface) {
				NvAPI_Initialize              = (NvAPI_Initialize_t)(NvAPI_QueryInterface)(0x0150E828);
				NvAPI_EnumPhysicalGPUs        = (NvAPI_EnumPhysicalGPUs_t)(NvAPI_QueryInterface)(0xE5AC921F);
				NvAPI_GPU_GetFullName         = (NvAPI_GPU_GetFullName_t)(NvAPI_QueryInterface)(0xCEEE8E9F);
				NVData.NvAPI_GPU_GetUsages    = (NvAPI_GPU_GetUsages_t)(NvAPI_QueryInterface)(0x189A1FDF);
				NVData.NvAPI_GPU_GetPStates   = (NvAPI_GPU_GetPStates_t)(NvAPI_QueryInterface)(0x60DED2ED);
				NVData.NvAPI_GPU_GetAllClocks = (NvAPI_GPU_GetAllClocks_t)(NvAPI_QueryInterface)(0x1BD69F49);
			}
			if (NULL == NvAPI_QueryInterface ||
					NULL == NvAPI_Initialize ||
					NULL == NvAPI_EnumPhysicalGPUs ||
					NULL == NvAPI_GPU_GetFullName ||
					NULL == NVData.NvAPI_GPU_GetUsages) {
				Clean();
			}

			if (NVData.hNVApi) {
				if (NvAPI_Initialize() != OK) {
					Clean();
				} else {
					int gpuCount = 0;
					if (NvAPI_EnumPhysicalGPUs(NVData.gpuHandles, &gpuCount) != OK || !gpuCount) {
						Clean();
					}

					for (int i = 0; i < gpuCount; i++) {
						NvAPI_ShortString gpuName = { 0 };
						if (NvAPI_GPU_GetFullName(NVData.gpuHandles[i], gpuName) == OK) {
							CString nvidiaGpuName = L"NVIDIA " + CString(gpuName);
							if (Device == nvidiaGpuName) {
								NVData.gpuSelected = i;
								break;
							}
						}
					}

					if (NVData.gpuSelected == -1) {
						Clean();
					}
				}
			}
		}

		if (NVData.hNVApi && NVData.NvAPI_GPU_GetUsages) {
			m_GPUType = NVIDIA_GPU;
		}
	}

	if (m_GPUType != UNKNOWN_GPU && !Device.IsEmpty() && pCreateDXGIFactory) {
		IDXGIFactory* pDXGIFactory = NULL;
		if (SUCCEEDED(pCreateDXGIFactory(IID_PPV_ARGS(&pDXGIFactory)))) {
			IDXGIAdapter *pDXGIAdapter = NULL;
			for (UINT i = 0; pDXGIFactory->EnumAdapters(i, &pDXGIAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
				DXGI_ADAPTER_DESC adapterDesc = { 0 };
				pDXGIAdapter->GetDesc(&adapterDesc);
				pDXGIAdapter->Release();

				const CString Description = CString(adapterDesc.Description).Trim();
				if (Description == Device) {
					memcpy(&dxgiAdapterDesc, &adapterDesc, sizeof(adapterDesc));
					break;
				}
			}

			pDXGIFactory->Release();
		}
	}

	return m_GPUType == UNKNOWN_GPU ? E_FAIL : S_OK;
}

void CGPUUsage::GetUsage(UINT& gpu_usage, UINT& gpu_clock, UINT64& gpu_mem_usage_total, UINT64& gpu_mem_usage_current)
{
	gpu_usage             = m_iGPUUsage;
	gpu_clock             = m_iGPUClock;
	gpu_mem_usage_total   = m_llGPUdedicatedBytesUsedTotal;
	gpu_mem_usage_current = m_llGPUdedicatedBytesUsedCurrent;

	if (m_GPUType != UNKNOWN_GPU && ::InterlockedIncrement(&m_lRunCount) == 1) {
		if (!EnoughTimePassed()) {
			::InterlockedDecrement(&m_lRunCount);
			return;
		}

		if (m_GPUType == ATI_GPU) {
			if (ATIData.iOverdriveVersion == 5) {
				ADLPMActivity activity = {0};
				activity.iSize = sizeof(ADLPMActivity);
				if (ADL_OK == ATIData.ADL_Overdrive5_CurrentActivity_Get(ATIData.iAdapterId, &activity)) {
					m_iGPUUsage = activity.iActivityPercent;
					m_iGPUClock = activity.iEngineClock / 100;
				}
			} else if (ATIData.iOverdriveVersion == 6) {
				ADLOD6CurrentStatus currentStatus = {0};
				if (ADL_OK == ATIData.ADL_Overdrive6_CurrentStatus_Get(ATIData.iAdapterId, &currentStatus)) {
					m_iGPUUsage = currentStatus.iActivityPercent;
					m_iGPUClock = currentStatus.iEngineClock / 100;
				}
			}
		} else if (m_GPUType == NVIDIA_GPU) {
			const int idx = NVData.gpuSelected;
			if (NVData.NvAPI_GPU_GetPStates) {
				if (NVData.NvAPI_GPU_GetPStates(NVData.gpuHandles[idx], &NVData.gpuPStates) == OK) {
					UINT state_gpu = NVData.gpuPStates.pstates[NVAPI_DOMAIN_GPU].present ? NVData.gpuPStates.pstates[NVAPI_DOMAIN_GPU].percent : 0;
					UINT state_vid = NVData.gpuPStates.pstates[NVAPI_DOMAIN_VID].present ? NVData.gpuPStates.pstates[NVAPI_DOMAIN_VID].percent : 0;
					m_iGPUUsage = state_vid << 16 | state_gpu;
				}
			} else {
				if (NVData.NvAPI_GPU_GetUsages(NVData.gpuHandles[idx], &NVData.gpuUsages) == OK) {
					m_iGPUUsage = NVData.gpuUsages.usage[10] << 16 | NVData.gpuUsages.usage[2];
				}
			}

			if (NVData.NvAPI_GPU_GetAllClocks
					&& NVData.NvAPI_GPU_GetAllClocks(NVData.gpuHandles[idx], &NVData.gpuClocks) == OK) {
				if (NVData.gpuClocks.clock[30] != 0) {
					m_iGPUClock = NVData.gpuClocks.clock[30] / 2000;
				} else {
					m_iGPUClock = NVData.gpuClocks.clock[0] / 1000;
				}
			}
		}

		if (dxgiAdapterDesc.AdapterLuid.LowPart) {
			m_llGPUdedicatedBytesUsedTotal = 0;
			m_llGPUdedicatedBytesUsedCurrent = 0;

			D3DKMT_QUERYSTATISTICS queryStatistics = {};
			queryStatistics.Type = D3DKMT_QUERYSTATISTICS_ADAPTER;
			queryStatistics.AdapterLuid = dxgiAdapterDesc.AdapterLuid;
			if (NT_SUCCESS(pD3DKMTQueryStatistics(&queryStatistics))) {
				const ULONG segmentCount = queryStatistics.QueryResult.AdapterInformation.NbSegments;
				for (ULONG i = 0; i < segmentCount; i++) {
					ZeroMemory(&queryStatistics, sizeof(D3DKMT_QUERYSTATISTICS));
					queryStatistics.Type = D3DKMT_QUERYSTATISTICS_SEGMENT;
					queryStatistics.AdapterLuid = dxgiAdapterDesc.AdapterLuid;
					queryStatistics.QuerySegment.SegmentId = i;

					if (NT_SUCCESS(pD3DKMTQueryStatistics(&queryStatistics))) {
						ULONG aperture;
						ULONG64 commitLimit;

						if (IsWin81orLater()) {
							aperture = queryStatistics.QueryResult.SegmentInformation.Aperture;
							commitLimit = queryStatistics.QueryResult.SegmentInformation.BytesResident;
						} else {
							aperture = queryStatistics.QueryResult.SegmentInformationV1.Aperture;
							commitLimit = queryStatistics.QueryResult.SegmentInformationV1.BytesResident;
						}

						if (!aperture) {
							m_llGPUdedicatedBytesUsedTotal += commitLimit;

							ZeroMemory(&queryStatistics, sizeof(D3DKMT_QUERYSTATISTICS));
							queryStatistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT;
							queryStatistics.hProcess = processHandle;
							queryStatistics.AdapterLuid = dxgiAdapterDesc.AdapterLuid;
							queryStatistics.QuerySegment.SegmentId = i;
							if (NT_SUCCESS(pD3DKMTQueryStatistics(&queryStatistics))) {
								if (IsWin81orLater()) {
									m_llGPUdedicatedBytesUsedCurrent += queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;
								} else {
									m_llGPUdedicatedBytesUsedCurrent += (ULONG)queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;
								}
							}
						}
					}
				}
			}
		}

		gpu_usage             = m_iGPUUsage;
		gpu_clock             = m_iGPUClock;
		gpu_mem_usage_total   = m_llGPUdedicatedBytesUsedTotal;
		gpu_mem_usage_current = m_llGPUdedicatedBytesUsedCurrent;

		m_dwLastRun = GetTickCount();
	}

	::InterlockedDecrement(&m_lRunCount);
}

bool CGPUUsage::EnoughTimePassed()
{
	const int minElapsedMS = 1000;
	return (GetTickCount() - m_dwLastRun) >= minElapsedMS;
}

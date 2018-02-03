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

#include "StdAfx.h"
#include "GPUUsage.h"
#include "../../../DSUtil/SysVersion.h"

// Memory allocation function
static void* __stdcall ADL_Main_Memory_Alloc(int iSize)
{
	return malloc(iSize);
}

CGPUUsage::CGPUUsage()
{
	Clean();

	if (SysVersion::IsWin7orLater()) {
		gdi32Handle = LoadLibraryW(L"gdi32.dll");
		if (gdi32Handle) {
			pD3DKMTQueryStatistics = (PFND3DKMT_QUERYSTATISTICS)GetProcAddress(gdi32Handle, "D3DKMTQueryStatistics");

			dxgiHandle = LoadLibraryW(L"dxgi.dll");
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

	ATIData.iAdapterId                            = -1;
	ATIData.iOverdriveVersion                     = 0;
	ATIData.hAtiADL                               = nullptr;
	ATIData.ADL_Main_Control_Destroy              = nullptr;
	ATIData.ADL_Overdrive5_CurrentActivity_Get    = nullptr;
	ATIData.ADL_Overdrive6_CurrentStatus_Get      = nullptr;
	ATIData.ADL2_OverdriveN_PerformanceStatus_Get = nullptr;

	ZeroMemory(&NVData.gpuHandles, sizeof(NVData.gpuHandles));

	ZeroMemory(&NVData.gpuUsages, sizeof(NVData.gpuUsages));
	NVData.gpuUsages.version					= sizeof(gpuUsages) | 0x10000;
	NVData.NvAPI_GPU_GetUsages					= nullptr;

	ZeroMemory(&NVData.gpuPStates, sizeof(NVData.gpuPStates));
	NVData.gpuPStates.version					= sizeof(gpuPStates) | 0x10000;
	NVData.NvAPI_GPU_GetPStates					= nullptr;

	ZeroMemory(&NVData.gpuClocks, sizeof(NVData.gpuClocks));
	NVData.gpuClocks.version					= sizeof(gpuClocks) | 0x20000;
	NVData.NvAPI_GPU_GetAllClocks				= nullptr;

	NVData.gpuSelected							= -1;

	NVData.hNVApi								= nullptr;

	ZeroMemory(&dxgiAdapterDesc, sizeof(dxgiAdapterDesc));

	m_statistic = {};
	m_dwLastRun = 0;
	m_lRunCount = 0;

	gpuTimeStatistics.clear();
	totalRunning = {0, 0};
}

HRESULT CGPUUsage::Init(const CString& DeviceName, const CString& Device)
{
	Clean();

	if (DeviceName.IsEmpty() || Device.IsEmpty()) {
		return E_FAIL;
	}

	{
		// ATI OverDrive
		ATIData.hAtiADL = LoadLibraryW(L"atiadlxx.dll");
		if (ATIData.hAtiADL == nullptr) {
			ATIData.hAtiADL = LoadLibraryW(L"atiadlxy.dll");
		}

		if (ATIData.hAtiADL) {
			ADL_MAIN_CONTROL_CREATE          ADL_Main_Control_Create;
			ADL_ADAPTER_NUMBEROFADAPTERS_GET ADL_Adapter_NumberOfAdapters_Get;
			ADL_ADAPTER_ADAPTERINFO_GET      ADL_Adapter_AdapterInfo_Get;
			ADL_ADAPTER_ACTIVE_GET           ADL_Adapter_Active_Get;
			ADL_OVERDRIVE_CAPS               ADL_Overdrive_Caps;

			ADL_Main_Control_Create          = (ADL_MAIN_CONTROL_CREATE)GetProcAddress(ATIData.hAtiADL,"ADL_Main_Control_Create");
			ATIData.ADL_Main_Control_Destroy = (ADL_MAIN_CONTROL_DESTROY)GetProcAddress(ATIData.hAtiADL,"ADL_Main_Control_Destroy");
			ADL_Adapter_NumberOfAdapters_Get = (ADL_ADAPTER_NUMBEROFADAPTERS_GET)GetProcAddress(ATIData.hAtiADL,"ADL_Adapter_NumberOfAdapters_Get");
			ADL_Adapter_AdapterInfo_Get      = (ADL_ADAPTER_ADAPTERINFO_GET)GetProcAddress(ATIData.hAtiADL,"ADL_Adapter_AdapterInfo_Get");
			ADL_Adapter_Active_Get           = (ADL_ADAPTER_ACTIVE_GET)GetProcAddress(ATIData.hAtiADL, "ADL_Adapter_Active_Get");
			ADL_Overdrive_Caps               = (ADL_OVERDRIVE_CAPS)GetProcAddress(ATIData.hAtiADL, "ADL_Overdrive_Caps");

			if (nullptr == ADL_Main_Control_Create ||
					nullptr == ATIData.ADL_Main_Control_Destroy ||
					nullptr == ADL_Adapter_NumberOfAdapters_Get ||
					nullptr == ADL_Adapter_AdapterInfo_Get ||
					nullptr == ADL_Adapter_Active_Get) {
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
								if (DeviceName != CString(adapterInfo.strDisplayName)) {
									continue;
								}

								int iOverdriveVersions[] = {5, 6, 7};
								int iOverdriveSupported  = 0;
								int iOverdriveEnabled    = 0;
								int iOverdriveVersion    = 0;
								if (ADL_Overdrive_Caps
										&& ADL_OK == ADL_Overdrive_Caps(adapterInfo.iAdapterIndex, &iOverdriveSupported, &iOverdriveEnabled, &iOverdriveVersion)
										&& iOverdriveSupported) {
									iOverdriveVersions[0] = iOverdriveVersion;
									iOverdriveVersions[1] = 0;
									iOverdriveVersions[2] = 0;
								}

								for (unsigned i = 0; i < _countof(iOverdriveVersions) && iOverdriveVersions[i] != 0; i++) {
									switch (iOverdriveVersions[i]) {
										case 5:
											{
												ADL_OVERDRIVE5_ODPARAMETERS_GET ADL_Overdrive5_ODParameters_Get;

												ADL_Overdrive5_ODParameters_Get            = (ADL_OVERDRIVE5_ODPARAMETERS_GET)GetProcAddress(ATIData.hAtiADL, "ADL_Overdrive5_ODParameters_Get");
												ATIData.ADL_Overdrive5_CurrentActivity_Get = (ADL_OVERDRIVE5_CURRENTACTIVITY_GET)GetProcAddress(ATIData.hAtiADL, "ADL_Overdrive5_CurrentActivity_Get");
												if (nullptr == ADL_Overdrive5_ODParameters_Get || nullptr == ATIData.ADL_Overdrive5_CurrentActivity_Get) {
													continue;
												}

												ADLODParameters overdriveParameters = { sizeof(overdriveParameters) };
												if (ADL_OK != ADL_Overdrive5_ODParameters_Get(adapterInfo.iAdapterIndex, &overdriveParameters) || !overdriveParameters.iActivityReportingSupported) {
													continue;
												}

												ATIData.iAdapterId = adapterInfo.iAdapterIndex;
												ATIData.iOverdriveVersion = iOverdriveVersions[i];
											}
											break;
										case 6:
											{
												ADL_OVERDRIVE6_CAPABILITIES_GET ADL_Overdrive6_Capabilities_Get;

												ADL_Overdrive6_Capabilities_Get          = (ADL_OVERDRIVE6_CAPABILITIES_GET)GetProcAddress(ATIData.hAtiADL, "ADL_Overdrive6_Capabilities_Get");
												ATIData.ADL_Overdrive6_CurrentStatus_Get = (ADL_OVERDRIVE6_CURRENTSTATUS_GET)GetProcAddress(ATIData.hAtiADL, "ADL_Overdrive6_CurrentStatus_Get");
												if (nullptr == ADL_Overdrive6_Capabilities_Get || nullptr == ATIData.ADL_Overdrive6_CurrentStatus_Get) {
													continue;
												}

												ADLOD6Capabilities od6Capabilities = { 0 };
												if (ADL_OK != ADL_Overdrive6_Capabilities_Get(adapterInfo.iAdapterIndex, &od6Capabilities) || (od6Capabilities.iCapabilities & ADL_OD6_CAPABILITY_GPU_ACTIVITY_MONITOR) != ADL_OD6_CAPABILITY_GPU_ACTIVITY_MONITOR) {
													continue;
												}

												ATIData.iAdapterId = adapterInfo.iAdapterIndex;
												ATIData.iOverdriveVersion = iOverdriveVersions[i];
											}
											break;
										case 7:
											{
												ADL2_OVERDRIVEN_CAPABILITIES_GET ADL2_OverdriveN_Capabilities_Get;

												ADL2_OverdriveN_Capabilities_Get              = (ADL2_OVERDRIVEN_CAPABILITIES_GET)GetProcAddress(ATIData.hAtiADL, "ADL2_OverdriveN_Capabilities_Get");
												ATIData.ADL2_OverdriveN_PerformanceStatus_Get = (ADL2_OVERDRIVEN_PERFORMANCESTATUS_GET)GetProcAddress(ATIData.hAtiADL,"ADL2_OverdriveN_PerformanceStatus_Get");
												if (nullptr == ADL2_OverdriveN_Capabilities_Get || nullptr == ATIData.ADL2_OverdriveN_PerformanceStatus_Get) {
													continue;
												}

												ADLODNCapabilities overdriveCapabilities = { 0 };
												if (ADL_OK != ADL2_OverdriveN_Capabilities_Get(nullptr, adapterInfo.iAdapterIndex, &overdriveCapabilities)) {
													// since Crimson 17.7.2 ADL2_OverdriveN_Capabilities_Get() are no longer working at all, it's replaced with ADL2_OverdriveN_CapabilitiesX2_Get()
													// continue;
												}

												ATIData.iAdapterId = adapterInfo.iAdapterIndex;
												ATIData.iOverdriveVersion = iOverdriveVersions[i];
											}
											break;
									}
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
		NVData.hNVApi = LoadLibraryW(L"nvapi64.dll");
		if (NVData.hNVApi == nullptr) {
			NVData.hNVApi = LoadLibraryW(L"nvapi.dll");
		}

		if (NVData.hNVApi) {
			NvAPI_QueryInterface_t   NvAPI_QueryInterface   = (NvAPI_QueryInterface_t)GetProcAddress(NVData.hNVApi, "nvapi_QueryInterface");
			NvAPI_Initialize_t       NvAPI_Initialize       = nullptr;
			NvAPI_EnumPhysicalGPUs_t NvAPI_EnumPhysicalGPUs = nullptr;
			NvAPI_GPU_GetFullName_t  NvAPI_GPU_GetFullName  = nullptr;
			if (NvAPI_QueryInterface) {
				NvAPI_Initialize              = (NvAPI_Initialize_t)(NvAPI_QueryInterface)(0x0150E828);
				NvAPI_EnumPhysicalGPUs        = (NvAPI_EnumPhysicalGPUs_t)(NvAPI_QueryInterface)(0xE5AC921F);
				NvAPI_GPU_GetFullName         = (NvAPI_GPU_GetFullName_t)(NvAPI_QueryInterface)(0xCEEE8E9F);
				NVData.NvAPI_GPU_GetUsages    = (NvAPI_GPU_GetUsages_t)(NvAPI_QueryInterface)(0x189A1FDF);
				NVData.NvAPI_GPU_GetPStates   = (NvAPI_GPU_GetPStates_t)(NvAPI_QueryInterface)(0x60DED2ED);
				NVData.NvAPI_GPU_GetAllClocks = (NvAPI_GPU_GetAllClocks_t)(NvAPI_QueryInterface)(0x1BD69F49);
			}
			if (nullptr == NvAPI_QueryInterface ||
					nullptr == NvAPI_Initialize ||
					nullptr == NvAPI_EnumPhysicalGPUs ||
					nullptr == NvAPI_GPU_GetFullName ||
					nullptr == NVData.NvAPI_GPU_GetUsages) {
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

	if (!Device.IsEmpty() && pCreateDXGIFactory) {
		IDXGIFactory* pDXGIFactory = nullptr;
		if (SUCCEEDED(pCreateDXGIFactory(IID_PPV_ARGS(&pDXGIFactory)))) {
			IDXGIAdapter *pDXGIAdapter = nullptr;
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

	if (m_GPUType == UNKNOWN_GPU && dxgiAdapterDesc.AdapterLuid.LowPart) {
		m_GPUType = Device.Find(L"Intel") == 0 ? INTEL_GPU : OTHER_GPU;
	}

	return m_GPUType == UNKNOWN_GPU ? E_FAIL : S_OK;
}

#define UpdateDelta(Var, NewValue) { if (Var.Value) { Var.Delta = NewValue - Var.Value; } Var.Value = NewValue; }

void CGPUUsage::GetUsage(statistic& gpu_statistic)
{
	gpu_statistic = m_statistic;

	if (m_GPUType != UNKNOWN_GPU && ::InterlockedIncrement(&m_lRunCount) == 1) {
		if (!EnoughTimePassed()) {
			::InterlockedDecrement(&m_lRunCount);
			return;
		}

		if (m_GPUType == ATI_GPU) {
			switch (ATIData.iOverdriveVersion) {
				case 5:
					{
						ADLPMActivity activity = { sizeof(activity) };
						if (ADL_OK == ATIData.ADL_Overdrive5_CurrentActivity_Get(ATIData.iAdapterId, &activity)) {
							m_statistic.gpu   = activity.iActivityPercent;
							m_statistic.clock = activity.iEngineClock / 100;
						}
					}
					break;
				case 6:
					{
						ADLOD6CurrentStatus currentStatus = { 0 };
						if (ADL_OK == ATIData.ADL_Overdrive6_CurrentStatus_Get(ATIData.iAdapterId, &currentStatus)) {
							m_statistic.gpu   = currentStatus.iActivityPercent;
							m_statistic.clock = currentStatus.iEngineClock / 100;
						}
					}
					break;
				case 7:
					{
						ADLODNPerformanceStatus odNPerformanceStatus = { 0 };
						if (ADL_OK == ATIData.ADL2_OverdriveN_PerformanceStatus_Get(nullptr, ATIData.iAdapterId, &odNPerformanceStatus)) {
							m_statistic.gpu   = odNPerformanceStatus.iGPUActivityPercent;
							m_statistic.clock = odNPerformanceStatus.iCoreClock / 100;
						}
					}
					break;
			}
		} else if (m_GPUType == NVIDIA_GPU) {
			const int idx = NVData.gpuSelected;
			if (NVData.NvAPI_GPU_GetPStates) {
				if (NVData.NvAPI_GPU_GetPStates(NVData.gpuHandles[idx], &NVData.gpuPStates) == OK) {
					m_statistic.gpu    = NVData.gpuPStates.pstates[NVAPI_DOMAIN_GPU].present ? NVData.gpuPStates.pstates[NVAPI_DOMAIN_GPU].percent : 0;
					m_statistic.decode = NVData.gpuPStates.pstates[NVAPI_DOMAIN_VID].present ? NVData.gpuPStates.pstates[NVAPI_DOMAIN_VID].percent : 0;
				}
			} else {
				if (NVData.NvAPI_GPU_GetUsages(NVData.gpuHandles[idx], &NVData.gpuUsages) == OK) {
					m_statistic.gpu = NVData.gpuUsages.usage[10] << 16 | NVData.gpuUsages.usage[2];
				}
			}

			if (NVData.NvAPI_GPU_GetAllClocks
					&& NVData.NvAPI_GPU_GetAllClocks(NVData.gpuHandles[idx], &NVData.gpuClocks) == OK) {
				if (NVData.gpuClocks.clock[30] != 0) {
					m_statistic.clock = NVData.gpuClocks.clock[30] / 2000;
				} else {
					m_statistic.clock = NVData.gpuClocks.clock[0] / 1000;
				}
			}
		}

		if (dxgiAdapterDesc.AdapterLuid.LowPart) {
			UINT64 llGPUDedicatedBytesUsedTotal = 0;
			UINT64 llGPUDedicatedBytesUsedCurrent = 0;
			UINT64 llGPUSharedBytesUsedTotal = 0;
			UINT64 llGPUSharedBytesUsedCurrent = 0;

			ULONG nodeCount = 0;

			D3DKMT_QUERYSTATISTICS queryStatistics = {};
			queryStatistics.Type = D3DKMT_QUERYSTATISTICS_ADAPTER;
			queryStatistics.AdapterLuid = dxgiAdapterDesc.AdapterLuid;
			if (NT_SUCCESS(pD3DKMTQueryStatistics(&queryStatistics))) {
				nodeCount = queryStatistics.QueryResult.AdapterInformation.NodeCount;
				const ULONG segmentCount = queryStatistics.QueryResult.AdapterInformation.NbSegments;
				for (ULONG i = 0; i < segmentCount; i++) {
					ZeroMemory(&queryStatistics, sizeof(D3DKMT_QUERYSTATISTICS));
					queryStatistics.Type = D3DKMT_QUERYSTATISTICS_SEGMENT;
					queryStatistics.AdapterLuid = dxgiAdapterDesc.AdapterLuid;
					queryStatistics.QuerySegment.SegmentId = i;

					if (NT_SUCCESS(pD3DKMTQueryStatistics(&queryStatistics))) {
						ULONG aperture;
						ULONG64 commitLimit;

						if (SysVersion::IsWin81orLater()) {
							aperture = queryStatistics.QueryResult.SegmentInformation.Aperture;
							commitLimit = queryStatistics.QueryResult.SegmentInformation.BytesResident;
						} else {
							aperture = queryStatistics.QueryResult.SegmentInformationV1.Aperture;
							commitLimit = queryStatistics.QueryResult.SegmentInformationV1.BytesResident;
						}

						if (aperture) {
							llGPUSharedBytesUsedTotal += commitLimit;
						} else {
							llGPUDedicatedBytesUsedTotal += commitLimit;
						}

						ZeroMemory(&queryStatistics, sizeof(D3DKMT_QUERYSTATISTICS));
						queryStatistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT;
						queryStatistics.hProcess = processHandle;
						queryStatistics.AdapterLuid = dxgiAdapterDesc.AdapterLuid;
						queryStatistics.QuerySegment.SegmentId = i;
						if (NT_SUCCESS(pD3DKMTQueryStatistics(&queryStatistics))) {
							if (SysVersion::IsWin81orLater()) {
								if (aperture) {
									llGPUSharedBytesUsedCurrent += queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;
								} else {
									llGPUDedicatedBytesUsedCurrent += queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;
								}
							} else {
								if (aperture) {
									llGPUSharedBytesUsedCurrent += (ULONG)queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;
								} else {
									llGPUDedicatedBytesUsedCurrent += (ULONG)queryStatistics.QueryResult.ProcessSegmentInformation.BytesCommitted;
								}
							}
						}
					}
				}

				m_statistic.memUsageTotal   = (llGPUDedicatedBytesUsedTotal ? llGPUDedicatedBytesUsedTotal : llGPUSharedBytesUsedTotal) / MEGABYTE;
				m_statistic.memUsageCurrent = (llGPUDedicatedBytesUsedCurrent ? llGPUDedicatedBytesUsedCurrent : llGPUSharedBytesUsedCurrent) / MEGABYTE;
			}

			if (nodeCount && m_GPUType != NVIDIA_GPU) {
				gpuTimeStatistics.resize(nodeCount);

				ZeroMemory(&queryStatistics, sizeof(queryStatistics));
				queryStatistics.Type = D3DKMT_QUERYSTATISTICS_NODE;
				queryStatistics.AdapterLuid = dxgiAdapterDesc.AdapterLuid;
				for (ULONG i = 0; i < nodeCount; i++) {
					queryStatistics.QueryProcessNode.NodeId = i;
					if (NT_SUCCESS(pD3DKMTQueryStatistics(&queryStatistics))
							&& queryStatistics.QueryResult.NodeInformation.GlobalInformation.RunningTime.QuadPart) {
						gpuTimeStatistics[i].runningTime = queryStatistics.QueryResult.NodeInformation.GlobalInformation.RunningTime.QuadPart;
					}
				}

				LARGE_INTEGER performanceCounter = {};
				QueryPerformanceCounter(&performanceCounter);
				LARGE_INTEGER performanceFrequency = {};
				QueryPerformanceFrequency(&performanceFrequency);

				if (performanceCounter.QuadPart && performanceFrequency.QuadPart) {
					UpdateDelta(totalRunning, performanceCounter.QuadPart);
					if (totalRunning.Delta) {
						m_statistic.gpu = 0;
						m_statistic.decode = 0;
						m_statistic.processing = 0;
						const auto elapsedTime = llMulDiv(totalRunning.Delta, 10000000, performanceFrequency.QuadPart, 0);

						if (m_GPUType == INTEL_GPU && gpuTimeStatistics.size() >= 4) {
							auto& item0 = gpuTimeStatistics[0];
							UpdateDelta(item0.gputotalRunningTime, item0.runningTime);
							if (item0.gputotalRunningTime.Delta) {
								m_statistic.gpu = llMulDiv(item0.gputotalRunningTime.Delta, 100, elapsedTime, 0);
							}

							auto& item1 = gpuTimeStatistics[1];
							UpdateDelta(item1.gputotalRunningTime, item1.runningTime);
							if (item1.gputotalRunningTime.Delta) {
								m_statistic.decode = llMulDiv(item1.gputotalRunningTime.Delta, 100, elapsedTime, 0);
							}

							auto& item3 = gpuTimeStatistics[3];
							UpdateDelta(item3.gputotalRunningTime, item3.runningTime);
							if (item3.gputotalRunningTime.Delta) {
								m_statistic.processing = llMulDiv(item3.gputotalRunningTime.Delta, 100, elapsedTime, 0);
							}
						} else {
							for (auto& item : gpuTimeStatistics) {
								if (item.runningTime) {
									UpdateDelta(item.gputotalRunningTime, item.runningTime);
									if (item.gputotalRunningTime.Delta) {
										const BYTE gpuUsage = llMulDiv(item.gputotalRunningTime.Delta, 100, elapsedTime, 0);
										m_statistic.gpu = std::max(m_statistic.gpu, gpuUsage);
									}
								}
							}
						}
					}
				}
			}
		}
		
		gpu_statistic = m_statistic;
		m_dwLastRun = GetTickCount();
	}

	::InterlockedDecrement(&m_lRunCount);
}

bool CGPUUsage::EnoughTimePassed()
{
	const int minElapsedMS = 1000;
	return (GetTickCount() - m_dwLastRun) >= minElapsedMS;
}

/*
 * (C) 2017-2019 see Authors.txt
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

#include "stdafx.h"
#include <detours/detours.h>
#include "D3DHook.h"
#include "include/d3dumddi.h"
#include "../../../DSUtil/Log.h"
#include "../../../DSUtil/SysVersion.h"

namespace D3DHook {
	UINT m_refreshRate = 0;
	CString m_Device;

	static bool HookFunc(PVOID& origFuncPtr, PVOID newFuncPtr)
	{
		DetourTransactionBegin();
		const bool attachSuccessful = (NO_ERROR == DetourAttach(&origFuncPtr, newFuncPtr));
		const bool commitSuccessful = (NO_ERROR == DetourTransactionCommit());

		return attachSuccessful && commitSuccessful;
	}

	template<typename T>
	static void fixRefreshRate(T& pResource, const double refreshRate)
	{
		if ((refreshRate > 59.970 && refreshRate < 60.5) && (m_refreshRate == 59)) {
			DLog(L"    => applied refreshRate fix -> 59.940");
			pResource->RefreshRate.Numerator = 60000;
			pResource->RefreshRate.Denominator = 1001;
		} else if ((refreshRate > 59.5 && refreshRate < 59.970) && (m_refreshRate == 60)) {
			DLog(L"    => applied refreshRate fix -> 60.000");
			pResource->RefreshRate.Numerator = 60000;
			pResource->RefreshRate.Denominator = 1000;
		} else if ((refreshRate > 29.985 && refreshRate < 30.25) && (m_refreshRate == 29)) {
			DLog(L"    => applied refreshRate fix -> 29.970");
			pResource->RefreshRate.Numerator = 30000;
			pResource->RefreshRate.Denominator = 1001;
		} else if ((refreshRate > 29.75 && refreshRate < 29.985) && (m_refreshRate == 30)) {
			DLog(L"    => applied refreshRate fix -> 30.000");
			pResource->RefreshRate.Numerator = 30000;
			pResource->RefreshRate.Denominator = 1000;
		} else if ((refreshRate > 23.988 && refreshRate < 24.2) && (m_refreshRate == 23)) {
			DLog(L"    => applied refreshRate fix -> 23.976");
			pResource->RefreshRate.Numerator = 24000;
			pResource->RefreshRate.Denominator = 1001;
		} else if ((refreshRate > 23.80 && refreshRate < 23.988) && (m_refreshRate == 24)) {
			DLog(L"    => applied refreshRate fix -> 24.000");
			pResource->RefreshRate.Numerator = 24000;
			pResource->RefreshRate.Denominator = 1000;
		}
	}

	PFND3DDDI_CREATERESOURCE pOrigCreateResource = nullptr;
	HRESULT APIENTRY pNewCreateResource(HANDLE hDevice, D3DDDIARG_CREATERESOURCE *pResource)
	{
		if (pResource && pResource->RefreshRate.Denominator) {
			const double refreshRate = (double)pResource->RefreshRate.Numerator / pResource->RefreshRate.Denominator;
			DLog(L"D3DHook() : hook CreateResource() : resolution: %d:%d, refreshRate: %d / %d = %.4f", pResource->pSurfList->Width, pResource->pSurfList->Height, pResource->RefreshRate.Numerator, pResource->RefreshRate.Denominator, refreshRate);

			fixRefreshRate(pResource, refreshRate);
		}

		return pOrigCreateResource(hDevice, pResource);
	}

	PFND3DDDI_CREATERESOURCE2 pOrigCreateResource2 = nullptr;
	HRESULT APIENTRY pNewCreateResource2(HANDLE hDevice, D3DDDIARG_CREATERESOURCE2 *pResource)
	{
		if (pResource && pResource->RefreshRate.Denominator) {
			const double refreshRate = (double)pResource->RefreshRate.Numerator / pResource->RefreshRate.Denominator;
			DLog(L"D3DHook() : hook CreateResource2() : resolution: %d:%d, refreshRate: %d / %d = %.4f", pResource->pSurfList->Width, pResource->pSurfList->Height, pResource->RefreshRate.Numerator, pResource->RefreshRate.Denominator, refreshRate);

			fixRefreshRate(pResource, refreshRate);
		}

		return pOrigCreateResource2(hDevice, pResource);
	}

	PFND3DDDI_CREATEDEVICE pOrigCreateDevice = nullptr;
	HRESULT APIENTRY pNewCreateDevice(HANDLE hAdapter, D3DDDIARG_CREATEDEVICE* pCreateData)
	{
		const HRESULT hr = pOrigCreateDevice(hAdapter, pCreateData);
		if (SUCCEEDED(hr) && pCreateData && pCreateData->Interface == 9) {
			if (!pOrigCreateResource
					&& pCreateData->pDeviceFuncs && pCreateData->pDeviceFuncs->pfnCreateResource) {
				pOrigCreateResource = pCreateData->pDeviceFuncs->pfnCreateResource;

				HookFunc((PVOID&)pOrigCreateResource, &pNewCreateResource);
			}

			if (!pOrigCreateResource2
					&& pCreateData->pDeviceFuncs && pCreateData->pDeviceFuncs->pfnCreateResource2) {
				pOrigCreateResource2 = pCreateData->pDeviceFuncs->pfnCreateResource2;

				HookFunc((PVOID&)pOrigCreateResource2, &pNewCreateResource2);
			}
		}

		return hr;
	}

	PFND3DDDI_OPENADAPTER pOrigOpenAdapter = nullptr;
	HRESULT APIENTRY pNewOpenAdapter(D3DDDIARG_OPENADAPTER* pOpenData)
	{
		const HRESULT hr = pOrigOpenAdapter(pOpenData);
		if (SUCCEEDED(hr) && !pOrigCreateDevice
				&& pOpenData && pOpenData->pAdapterFuncs && pOpenData->pAdapterFuncs->pfnCreateDevice) {
			pOrigCreateDevice = pOpenData->pAdapterFuncs->pfnCreateDevice;

			HookFunc((PVOID&)pOrigCreateDevice, &pNewCreateDevice);
		}

		return hr;
	}

	const bool Hook(const CString& device, const UINT refreshRate)
	{
		DLog(L"D3DHook::Hook() : refreshRate : %u", refreshRate);

		if (m_Device == device) {
			DLog(L"D3DHook::Hook() : hook already installed for the device '%s'", m_Device.GetString());
			m_refreshRate = refreshRate;
			return true;
		}

		UnHook();

		DLog(L"D3DHook::Hook() : try to install hook for the device '%s'", device.GetString());

		CString driverDll;
		DISPLAY_DEVICE dd = { sizeof(DISPLAY_DEVICE) };
		DWORD iDevNum = 0;
		while (EnumDisplayDevices(0, iDevNum, &dd, 0)) {
			DLog(L"D3DHook::Hook() : EnumDisplayDevices() : DeviceName - '%s', DeviceString - '%s', DeviceKey - '%s'", dd.DeviceName, dd.DeviceString, dd.DeviceKey);
			if (device == dd.DeviceString) {
				CString lpszKeyName = dd.DeviceKey; lpszKeyName.Replace(L"\\Registry\\Machine\\", L""); lpszKeyName.Trim();
				if (!lpszKeyName.IsEmpty()) {
					CRegKey regkey;
					if (ERROR_SUCCESS == regkey.Open(HKEY_LOCAL_MACHINE, lpszKeyName, KEY_READ)) {
						LPCTSTR pszValueName = L"UserModeDriverName";
#ifndef _WIN64
						if (SysVersion::IsW64()) {
							pszValueName = L"UserModeDriverNameWoW";
						}
#endif
						ULONG nChars = 0;
						if (ERROR_SUCCESS == regkey.QueryMultiStringValue(pszValueName, nullptr, &nChars) && nChars > 0) {
							LPTSTR pszValue = DNew WCHAR[nChars];
							if (ERROR_SUCCESS == regkey.QueryMultiStringValue(pszValueName, pszValue, &nChars)) {
								// we need only the first value
								driverDll = pszValue; driverDll.Trim();
							}

							delete [] pszValue;
						}

						regkey.Close();
					}
				}

				break;
			}

			ZeroMemory(&dd, sizeof(dd));
			dd.cb = sizeof(dd);
			iDevNum++;
		}

		if (!driverDll.IsEmpty()) {
			DLog(L"D3DHook::Hook() : use the driver library '%s'", driverDll.GetString());
			pOrigOpenAdapter = (PFND3DDDI_OPENADAPTER)GetProcAddress(GetModuleHandleW(driverDll), "OpenAdapter");
			if (pOrigOpenAdapter) {
				const bool ret = HookFunc((PVOID&)pOrigOpenAdapter, &pNewOpenAdapter);
				if (ret) {
					DLog(L"D3DHook::Hook() : successfully installed hook for the device '%s'", device.GetString());
					m_refreshRate = refreshRate;
					m_Device = device;

					return true;
				}

				pOrigOpenAdapter = nullptr;
			}
		}

		DLog(L"D3DHook::Hook() : unable install hook for the device '%s'", device.GetString());
		return false;
	}

	void UnHook()
	{
		if (pOrigOpenAdapter) {
			DetourTransactionBegin();

			if (pOrigCreateResource) {
				DetourDetach(&(PVOID&)pOrigCreateResource, &pNewCreateResource);
			}
			if (pOrigCreateResource2) {
				DetourDetach(&(PVOID&)pOrigCreateResource2, &pNewCreateResource2);
			}
			if (pOrigCreateDevice) {
				DetourDetach(&(PVOID&)pOrigCreateDevice, &pNewCreateDevice);
			}

			DetourDetach(&(PVOID&)pOrigOpenAdapter, pNewOpenAdapter);
			DetourTransactionCommit();

			pOrigCreateResource = nullptr;
			pOrigCreateResource2 = nullptr;
			pOrigCreateDevice = nullptr;
			pOrigOpenAdapter = nullptr;

			m_Device.Empty();
			m_refreshRate = 0;
		}
	}
}

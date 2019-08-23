/*
 * (C) 2015-2019 see Authors.txt
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
#include "AudioDevice.h"
#include <FunctionDiscoveryKeys_devpkey.h>
#include "../../../DSUtil/DSUtil.h"

static BOOL CALLBACK DSEnumCallback(LPGUID lpGUID,
	LPCTSTR lpszDesc,
	LPCTSTR lpszDrvName,
	LPVOID lpContext)
{
	auto device = (AudioDevices::device_t*)lpContext;
	ASSERT(device);

	if (lpGUID == nullptr) { // add only "Primary Sound Driver"
		*device = {lpszDesc, L""};
	}

	return TRUE;
}

static HMODULE hModuleDSound = nullptr;
HRESULT(__stdcall * pDirectSoundEnumerate)(__in LPDSENUMCALLBACKW pDSEnumCallback, __in_opt LPVOID pContext) = nullptr;
static void InitDSound()
{
	if (!hModuleDSound) {
		hModuleDSound = LoadLibraryW(L"dsound.dll");
	}

	if (hModuleDSound && !pDirectSoundEnumerate) {
		(FARPROC &)pDirectSoundEnumerate = GetProcAddress(hModuleDSound, "DirectSoundEnumerateW");
	}
}

namespace AudioDevices
{
	HRESULT GetActiveAudioDevices(IMMDeviceEnumerator *pMMDeviceEnumerator, deviceList_t* deviceList, UINT* devicesCount, BOOL bIncludeDefault)
	{
		if (deviceList) {
			deviceList->clear();
		}

		if (bIncludeDefault) {
			InitDSound();
			if (pDirectSoundEnumerate) {
				device_t device;
				pDirectSoundEnumerate((LPDSENUMCALLBACK)DSEnumCallback, (LPVOID)&device);

				if (!device.deviceName.IsEmpty()) {
					if (deviceList) {
						deviceList->emplace_back(device);
					}

					if (devicesCount) {
						*devicesCount = 1;
					}
				}
			}
		}

		IMMDeviceEnumerator* deviceEnumerator = pMMDeviceEnumerator;
		if (!deviceEnumerator && FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&deviceEnumerator)))) {
			return E_FAIL;
		}

		CComPtr<IMMDeviceCollection> deviceCollection;
		deviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &deviceCollection);

		UINT count = 0;
		if (!deviceCollection || FAILED(deviceCollection->GetCount(&count))) {
			if (!pMMDeviceEnumerator) {
				deviceEnumerator->Release();
			}
			return E_FAIL;
		}

		if (devicesCount) {
			*devicesCount += count;
		}

		if (deviceList) {
			IMMDevice* endpoint = nullptr;
			IPropertyStore* pProps = nullptr;
			LPWSTR pwszID = nullptr;
			PROPVARIANT varName;
			PropVariantInit(&varName);

			for (UINT i = 0; i < count; i++) {
				if (SUCCEEDED(deviceCollection->Item(i, &endpoint))
						&& SUCCEEDED(endpoint->GetId(&pwszID))
						&& SUCCEEDED(endpoint->OpenPropertyStore(STGM_READ, &pProps))
						&& SUCCEEDED(pProps->GetValue(PKEY_Device_FriendlyName, &varName))) {
					deviceList->push_back({ varName.pwszVal, pwszID });
					PropVariantClear(&varName);
				}

				SAFE_RELEASE(endpoint);
				SAFE_RELEASE(pProps);
				if (pwszID) {
					CoTaskMemFree(pwszID);
					pwszID = nullptr;
				}
			}

			if (!pMMDeviceEnumerator) {
				deviceEnumerator->Release();
			}

			return deviceList->empty() ? E_FAIL : S_OK;
		}

		return S_OK;
	}

	HRESULT GetDefaultAudioDevice(IMMDeviceEnumerator *pMMDeviceEnumerator, device_t& device)
	{
		HRESULT hr = E_FAIL;
		IMMDevice* endpoint = nullptr;
		IPropertyStore* pProps = nullptr;
		LPWSTR pwszID = nullptr;
		PROPVARIANT varName;
		PropVariantInit(&varName);

		if (SUCCEEDED(pMMDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &endpoint))
				&& SUCCEEDED(endpoint->GetId(&pwszID))
				&& SUCCEEDED(endpoint->OpenPropertyStore(STGM_READ, &pProps))
				&& SUCCEEDED(pProps->GetValue(PKEY_Device_FriendlyName, &varName))) {
			device = { varName.pwszVal, pwszID };
			PropVariantClear(&varName);

			hr = S_OK;
		}

		SAFE_RELEASE(endpoint);
		SAFE_RELEASE(pProps);
		if (pwszID) {
			CoTaskMemFree(pwszID);
		}

		return hr;
	}
}

/*
 * (C) 2015-2018 see Authors.txt
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
#include <mmdeviceapi.h>
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
	HRESULT GetActiveAudioDevices(deviceList_t* deviceList/* = NULL*/, UINT* devicesCount/* = NULL*/, BOOL bIncludeDefault/* = TRUE*/)
	{
		HRESULT hr = E_FAIL;

		if (deviceList) {
			deviceList->clear();
		}

		if (bIncludeDefault) {
			InitDSound();
			if (pDirectSoundEnumerate) {
				device_t device;
				pDirectSoundEnumerate((LPDSENUMCALLBACK)DSEnumCallback, (LPVOID)&device);

				if (!device.first.IsEmpty()) {
					if (deviceList) {
						deviceList->emplace_back(device);
					}

					if (devicesCount) {
						*devicesCount = 1;
					}
				}
			}
		}

		for (;;) {
			CComPtr<IMMDeviceEnumerator> enumerator;
			hr = enumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator));
			if (FAILED(hr)) {
				break;
			}
			CComPtr<IMMDeviceCollection> devices;
			if (FAILED(hr = enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devices))) {
				break;
			}

			UINT count = 0;
			if (FAILED(hr = devices->GetCount(&count))) {
				break;
			}

			if (devicesCount) {
				*devicesCount += count;
			}

			if (deviceList) {
				IMMDevice* endpoint = nullptr;
				IPropertyStore* pProps = nullptr;
				LPWSTR pwszID = nullptr;

				for (UINT i = 0; i < count; i++) {
					if (SUCCEEDED(hr = devices->Item(i, &endpoint))
							&& SUCCEEDED(endpoint->GetId(&pwszID))
							&& SUCCEEDED(hr = endpoint->OpenPropertyStore(STGM_READ, &pProps))) {
						PROPVARIANT varName;
						PropVariantInit(&varName);
						if (SUCCEEDED(hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName))) {
							deviceList->emplace_back(varName.pwszVal, pwszID);

							PropVariantClear(&varName);
						}
					}

					SAFE_RELEASE(endpoint);
					SAFE_RELEASE(pProps);
					if (pwszID) {
						CoTaskMemFree(pwszID);
						pwszID = nullptr;
					}
				}
			}

			hr = S_OK;
			break;
		}

		return hr;
	}

	HRESULT GetDefaultAudioDevice(device_t& device)
	{
		HRESULT hr = E_FAIL;

		for (;;) {
			CComPtr<IMMDeviceEnumerator> enumerator;
			if (FAILED(hr = enumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator)))) {
				break;
			}

			CComPtr<IMMDevice> pMMDevice;
			if (FAILED(hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pMMDevice))) {
				break;
			}

			LPWSTR pwszID = nullptr;
			if ((hr = pMMDevice->GetId(&pwszID)) == S_OK) {
				CComPtr<IPropertyStore> pProps;
				if ((hr = pMMDevice->OpenPropertyStore(STGM_READ, &pProps)) == S_OK) {
					PROPVARIANT varName;
					PropVariantInit(&varName);
					if ((hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName)) == S_OK) {
						device = {varName.pwszVal, pwszID};
					}

					PropVariantClear(&varName);
				}
			}

			if (pwszID) {
				CoTaskMemFree(pwszID);
			}

			break;
		}

		return hr;
	}
}

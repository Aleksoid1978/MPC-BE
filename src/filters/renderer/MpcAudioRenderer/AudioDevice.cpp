/*
 * (C) 2015-2016 see Authors.txt
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
	CStringArray* pArray = (CStringArray*)lpContext;
	ASSERT(pArray);

	if (lpGUID == NULL) { // add only "Primary Sound Driver"
		pArray->Add(lpszDesc);
	}

	return TRUE;
}

static HMODULE hModuleDSound = NULL;
HRESULT(__stdcall * pDirectSoundEnumerate)(__in LPDSENUMCALLBACKW pDSEnumCallback, __in_opt LPVOID pContext) = NULL;
static void InitDSound()
{
	if (!hModuleDSound) {
		hModuleDSound = LoadLibrary(L"dsound.dll");
	}

	if (hModuleDSound && !pDirectSoundEnumerate) {
		(FARPROC &)pDirectSoundEnumerate = GetProcAddress(hModuleDSound, "DirectSoundEnumerateW");
	}
}

namespace AudioDevices
{
	HRESULT GetActiveAudioDevices(CStringArray& deviceNameList, CStringArray& deviceIdList, BOOL bIncludeDefault/* = TRUE*/)
	{
		HRESULT hr = E_FAIL;

		deviceNameList.RemoveAll();
		deviceIdList.RemoveAll();

		if (bIncludeDefault) {
			InitDSound();
			if (pDirectSoundEnumerate) {
				pDirectSoundEnumerate((LPDSENUMCALLBACK)DSEnumCallback, (LPVOID)&deviceNameList);
			}

			if (deviceNameList.GetCount() == 1) {
				deviceIdList.Add(L"");
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

			IMMDevice* endpoint = NULL;
			IPropertyStore* pProps = NULL;
			LPWSTR pwszID = NULL;

			for (UINT i = 0; i < count; i++) {
				if (SUCCEEDED(hr = devices->Item(i, &endpoint))
						&& SUCCEEDED(endpoint->GetId(&pwszID))
						&& SUCCEEDED(hr = endpoint->OpenPropertyStore(STGM_READ, &pProps))) {
					PROPVARIANT varName;
					PropVariantInit(&varName);
					if (SUCCEEDED(hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName))) {
						deviceIdList.Add(pwszID);
						deviceNameList.Add(varName.pwszVal);

						PropVariantClear(&varName);
					}
				}

				SAFE_RELEASE(endpoint);
				SAFE_RELEASE(pProps);
				if (pwszID) {
					CoTaskMemFree(pwszID);
					pwszID = NULL;
				}
			}

			hr = S_OK;
			break;
		}

		return hr;
	}

	HRESULT GetActiveAudioDevicesCount(UINT& deviceCount, BOOL bIncludeDefault/* = TRUE*/)
	{
		HRESULT hr = E_FAIL;

		deviceCount = 0;

		if (bIncludeDefault) {
			InitDSound();
			CStringArray deviceNameList;
			if (pDirectSoundEnumerate) {
				pDirectSoundEnumerate((LPDSENUMCALLBACK)DSEnumCallback, (LPVOID)&deviceNameList);
			}

			if (deviceNameList.GetCount() == 1) {
				deviceCount = 1;
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

			deviceCount += count;

			break;
		}

		return hr;
	}

	HRESULT GetDefaultAudioDevice(CString& deviceName, CString& deviceId)
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

			LPWSTR pwszID = NULL;
			if ((hr = pMMDevice->GetId(&pwszID)) == S_OK) {
				deviceId = pwszID;

				CComPtr<IPropertyStore> pProps;
				if ((hr = pMMDevice->OpenPropertyStore(STGM_READ, &pProps)) == S_OK) {
					PROPVARIANT varName;
					PropVariantInit(&varName);
					if ((hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName)) == S_OK) {
						deviceName = varName.pwszVal;
					}

					PropVariantClear(&varName);
				}
			}

			CoTaskMemFree(pwszID);

			break;
		}

		return hr;
	}
}

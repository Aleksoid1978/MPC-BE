/*
 * (C) 2022 see Authors.txt
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

#include "DSUtil/SysVersion.h"
#include "D3D11Decoder/D3D11Decoder.h"

#include "Utils.h"

static IDXGIFactory1* GetDXGIFactory1()
{
	if (!SysVersion::IsWin8orLater()) {
		return nullptr;
	}

	HMODULE hDxgiLib = LoadLibraryW(L"dxgi.dll");
	if (!hDxgiLib) {
		return nullptr;
	}

	auto pfnCreateDXGIFactory1 = reinterpret_cast<PFN_CREATE_DXGI_FACTORY1>(GetProcAddress(hDxgiLib, "CreateDXGIFactory1"));
	if (!pfnCreateDXGIFactory1) {
		return nullptr;
	}

	IDXGIFactory1* pDXGIFactory1 = nullptr;
	HRESULT hr = pfnCreateDXGIFactory1(IID_IDXGIFactory1, reinterpret_cast<void**>(&pDXGIFactory1));
	if (FAILED(hr)) {
		return nullptr;
	}

	return pDXGIFactory1;
}

HRESULT GetDxgiAdapters(std::list<DXGI_ADAPTER_DESC>& dxgi_adapters)
{
	static CComPtr<IDXGIFactory1> pDXGIFactory1 = GetDXGIFactory1();
	if (!pDXGIFactory1) {
		return E_FAIL;
	}

	dxgi_adapters.clear();

	UINT index = 0;
	HRESULT hr = S_OK;
	CComPtr<IDXGIAdapter> pDXGIAdapter;
	while (SUCCEEDED(hr = pDXGIFactory1->EnumAdapters(index++, &pDXGIAdapter))) {
		DXGI_ADAPTER_DESC desc = {};
		if (SUCCEEDED(pDXGIAdapter->GetDesc(&desc))) {
			if (desc.VendorId == 0x1414 && desc.DeviceId == 0x8c) {
				// skip "Microsoft Basic Render Driver" from end
				break;
			}
			dxgi_adapters.emplace_back(desc);
		}

		pDXGIAdapter.Release();
	}

	return dxgi_adapters.size() ? S_OK : E_FAIL;
}

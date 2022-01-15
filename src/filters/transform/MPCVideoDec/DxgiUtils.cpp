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
#include "DxgiUtils.h"

// CDXGIFactory1

typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY1)(REFIID riid, void** ppFactory);

std::unique_ptr<CDXGIFactory1> CDXGIFactory1::m_pInstance;

CDXGIFactory1::CDXGIFactory1()
	: m_pDXGIFactory1(nullptr)
{
	if (SysVersion::IsWin8orLater()) {
		HMODULE hDxgiLib = LoadLibraryW(L"dxgi.dll");
		if (hDxgiLib) {
			PFN_CREATE_DXGI_FACTORY1 pfnCreateDXGIFactory1 = (PFN_CREATE_DXGI_FACTORY1)GetProcAddress(hDxgiLib, "CreateDXGIFactory1");
			if (pfnCreateDXGIFactory1) {
				HRESULT hr = pfnCreateDXGIFactory1(IID_IDXGIFactory1, (void**)&m_pDXGIFactory1);
			}
		}
	}
	ASSERT(m_pDXGIFactory1);
}

IDXGIFactory1* CDXGIFactory1::GetFactory()  const
{
	ASSERT(m_pDXGIFactory1);
	return m_pDXGIFactory1;
}

////////////////

HRESULT GetDxgiAdapters(std::list<DXGI_ADAPTER_DESC>& dxgi_adapters)
{
	IDXGIFactory1* pDXGIFactory1 = CDXGIFactory1::GetInstance().GetFactory();
	if (!pDXGIFactory1) {
		return E_ABORT;
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

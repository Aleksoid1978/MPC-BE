/*
 * (C) 2016-2017 see Authors.txt
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
#include <d3d9.h>
#include "D3D9Helper.h"

UINT D3D9Helper::GetAdapter(IDirect3D9* pD3D, HWND hWnd)
{
	CheckPointer(hWnd, D3DADAPTER_DEFAULT);
	CheckPointer(pD3D, D3DADAPTER_DEFAULT);

	const HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
	if (hMonitor == nullptr) {
		return D3DADAPTER_DEFAULT;
	}

	for (UINT adp = 0, num_adp = pD3D->GetAdapterCount(); adp < num_adp; ++adp) {
		const HMONITOR hAdpMon = pD3D->GetAdapterMonitor(adp);
		if (hAdpMon == hMonitor) {
			return adp;
		}
	}

	return D3DADAPTER_DEFAULT;
}

IDirect3D9* D3D9Helper::Direct3DCreate9()
{
	typedef IDirect3D9* (WINAPI *tpDirect3DCreate9)(__in UINT SDKVersion);

	static HMODULE hModule = LoadLibrary(L"d3d9.dll");
	static tpDirect3DCreate9 pDirect3DCreate9 = hModule ? (tpDirect3DCreate9)GetProcAddress(hModule, "Direct3DCreate9") : nullptr;
	if (pDirect3DCreate9) {
		IDirect3D9* pD3D9 = pDirect3DCreate9(D3D_SDK_VERSION);
		if (!pD3D9) {
			pD3D9 = pDirect3DCreate9(D3D9b_SDK_VERSION);
		}

		return pD3D9;
	}

	return nullptr;
}

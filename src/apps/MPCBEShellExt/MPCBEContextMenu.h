/*
 * (C) 2012-2018 see Authors.txt
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

#pragma once

#include "resource.h"
#include <vector>
#include <Shobjidl.h>
#include "MPCBEShellExt_i.h"

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

// CMPCBEContextMenu

class ATL_NO_VTABLE CMPCBEContextMenu :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMPCBEContextMenu, &CLSID_MPCBEContextMenu>,
	public IContextMenu,
	public IShellExtInit
{
public:

	CMPCBEContextMenu();
	~CMPCBEContextMenu();

	// IShellExtInit
	STDMETHODIMP Initialize(LPCITEMIDLIST, LPDATAOBJECT, HKEY);
	
	// IContextMenu
	STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT, UINT*, LPSTR, UINT) { return E_NOTIMPL; }
	STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO);
	STDMETHODIMP QueryContextMenu(HMENU, UINT, UINT, UINT, UINT);

	DECLARE_REGISTRY_RESOURCEID(IDR_MPCBECONTEXTMENU)
	DECLARE_NOT_AGGREGATABLE(CMPCBEContextMenu)

	BEGIN_COM_MAP(CMPCBEContextMenu)
		COM_INTERFACE_ENTRY(IContextMenu)
		COM_INTERFACE_ENTRY(IShellExtInit)
	END_COM_MAP()

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

private:
	HBITMAP m_hPlayBmp, m_hAddBmp;
	std::vector<CString> m_fileNames;

	void SendData(bool add_pl = false);
};

OBJECT_ENTRY_AUTO(__uuidof(MPCBEContextMenu), CMPCBEContextMenu)

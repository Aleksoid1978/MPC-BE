/*
 * Copyright (C) 2012 Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
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

#include "../../filters/InternalPropertyPage.h"
#include "IAviSplitter.h"
#include "resource.h"

class __declspec(uuid("299503E5-F29B-4C63-92B4-EB5CF67BBDA0"))
	CAviSplitterSettingsWnd : public CInternalPropertyPageWnd
{
private :
	CComQIPtr<IAviSplitterFilter> m_pMSF;

	CButton	m_cbBadInterleavedSuport;
	CButton	m_cbSetReindex;

	enum {
		IDC_PP_INTERLEAVED_SUPPORT = 10000,
		IDC_PP_SET_REINDEX,
	};

public:
	CAviSplitterSettingsWnd(void);

	bool OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks);
	void OnDisconnect();
	bool OnActivate();
	void OnDeactivate();
	bool OnApply();

	static LPCWSTR GetWindowTitle() { return MAKEINTRESOURCE(IDS_FILTER_SETTINGS_CAPTION); }
	static CSize GetWindowSize() { return CSize(270, 53); }

	DECLARE_MESSAGE_MAP()
};

/*
 * (C) 2006-2014 see Authors.txt
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
#include "IVTSReader.h"
#include "resource.h"

class __declspec(uuid("BA18CEF2-319A-4A1F-B2CB-BCB53F6ABDE5"))
	CVTSReaderSettingsWnd : public CInternalPropertyPageWnd
{
private :
	CComQIPtr<IVTSReader> m_pMSF;

	CButton m_cbEnableTitleSelection;

public:
	CVTSReaderSettingsWnd(void);

	bool OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks);
	void OnDisconnect();
	bool OnActivate();
	void OnDeactivate();
	bool OnApply();

	static LPCTSTR GetWindowTitle() { return MAKEINTRESOURCE(IDS_FILTER_SETTINGS_CAPTION); }
	static CSize GetWindowSize() { return CSize(320, 250); }

	DECLARE_MESSAGE_MAP()
};

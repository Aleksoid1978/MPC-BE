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
#include "IMpegSplitter.h"
#include "resource.h"

class __declspec(uuid("44FCB62D-3AEB-401C-A7E1-8A984C017923"))
	CMpegSplitterSettingsWnd : public CInternalPropertyPageWnd
{
private :
	CComQIPtr<IMpegSplitterFilter> m_pMSF;

	CButton		m_cbForcedSub;
	CStatic		m_txtAudioLanguageOrder;
	CEdit		m_edtAudioLanguageOrder;
	CStatic		m_txtSubtitlesLanguageOrder;
	CEdit		m_edtSubtitlesLanguageOrder;

	CButton		m_grpTrueHD;
	CButton		m_cbTrueHD;
	CButton		m_cbAC3Core;

	CButton		m_cbSubEmptyPin;

	enum {
		IDC_PP_SUBTITLE_FORCED = 10000,
		IDC_PP_AUDIO_LANGUAGE_ORDER,
		IDC_PP_SUBTITLES_LANGUAGE_ORDER,
		IDC_PP_TRUEHD,
		IDC_PP_AC3CORE,
		IDC_PP_ENABLE_SUB_EMPTY_PIN
	};

public:
	CMpegSplitterSettingsWnd(void);

	bool OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks);
	void OnDisconnect();
	bool OnActivate();
	void OnDeactivate();
	bool OnApply();

	static LPCTSTR GetWindowTitle() { return MAKEINTRESOURCE(IDS_FILTER_SETTINGS_CAPTION); }
	static CSize GetWindowSize() { return CSize(325, 250); }

	DECLARE_MESSAGE_MAP()
};

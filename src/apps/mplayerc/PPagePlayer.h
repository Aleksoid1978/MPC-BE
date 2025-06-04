/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2025 see Authors.txt
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

#include "PPageBase.h"
#include "controls/FloatEdit.h"

// CPPagePlayer dialog

class CPPagePlayer : public CPPageBase
{
	DECLARE_DYNAMIC(CPPagePlayer)

public:
	CPPagePlayer();
	virtual ~CPPagePlayer() = default;

	int  m_iCurSetsLocation = 0;
	int  m_iSetsLocation = 0;

	int  m_iMultipleInst = 1;

	BOOL m_bKeepHistory     = FALSE;
	BOOL m_bRememberDVDPos  = FALSE;
	BOOL m_bRememberFilePos = FALSE;
	BOOL m_bSavePnSZoom     = FALSE;
	BOOL m_bRememberPlaylistItems = FALSE;
	BOOL m_bRecentFilesShowUrlTitle = FALSE;

	BOOL m_bTrayIcon    = FALSE;
	BOOL m_bHideCDROMsSubMenu = FALSE;
	BOOL m_bPriority = FALSE;

	CComboBox m_cbTitleBarPrefix;
	CComboBox m_cbSeekBarText;

	CIntEdit m_edtHistoryEntriesMax;
	CSpinButtonCtrl m_spnHistoryEntriesMax;
	CIntEdit m_edtRecentFiles;
	CSpinButtonCtrl m_spnRecentFiles;

	CIntEdit m_edtNetworkTimeout;
	CSpinButtonCtrl m_spnNetworkTimeout;
	CIntEdit m_edtNetworkReceiveTimeout;
	CSpinButtonCtrl m_spnNetworkReceiveTimeout;

	enum { IDD = IDD_PPAGEPLAYER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnUpdateKeepHistory(CCmdUI* pCmdUI);
};

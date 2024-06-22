/*
 * (C) 2006-2024 see Authors.txt
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
#include "controls/StaticLink.h"


class CPPageSync: public CPPageBase
{
	DECLARE_DYNAMIC(CPPageSync)

public:
	CPPageSync();
	virtual ~CPPageSync();

	enum {IDD = IDD_PPAGESYNC};

	CButton m_chkVSync;
	CButton m_chkVSyncInternal;
	CButton m_chkEnableFrameTimeCorrection;
	CButton m_chkFlushGPUBeforeVSync;
	CButton m_chkFlushGPUAfterPresent;
	CButton m_chkFlushGPUWait;

	int m_iSyncMode = 0;
	CFloatEdit m_edtCycleDelta;

	CIntEdit m_edtTargetSyncOffset;
	CIntEdit m_edtControlLimit;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnSetActive();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnSyncModeClicked(UINT nID);

private:
	void InitDialogPrivate();
};

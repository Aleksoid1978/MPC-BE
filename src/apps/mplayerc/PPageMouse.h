/*
 * (C) 2020 see Authors.txt
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


// CPPageMouse dialog

class CPPageMouse : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageMouse)

	CComboBox m_cmbLeftBottonClick;
	CComboBox m_cmbLeftBottonDblClick;
	CComboBox m_cmbMiddleBotton;
	CComboBox m_cmbRightBotton;
	CComboBox m_cmbXButton1;
	CComboBox m_cmbXButton2;
	CComboBox m_cmbWheelUp;
	CComboBox m_cmbWheelDown;
	CComboBox m_cmbWheelLeft;
	CComboBox m_cmbWheelRight;

public:
	CPPageMouse();
	virtual ~CPPageMouse();

	enum { IDD = IDD_PPAGEMOUSE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnWheelUpChange();
	afx_msg void OnWheelLeftChange();
};

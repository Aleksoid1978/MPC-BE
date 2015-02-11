/*
 * (C) 2003-2006 Gabest
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

#include "PPageBase.h"


// CPPageVideo dialog

class CPPageVideo : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageVideo)

private:
	enum { IDD = IDD_PPAGEVIDEO };

	CStringArray m_D3D9GUIDNames;

	CComboBox m_cbVideoRenderer;
	CComboBox m_cbD3D9RenderDevice;
	CComboBox m_cbDX9SurfaceFormat;
	CComboBox m_cbDX9Resizer;
	CButton   m_chkD3DFullscreen;
	CButton   m_chk10bitOutput;

	int m_iVideoRendererType_store;
	int m_iVideoRendererType;

	BOOL m_bD3D9RenderDevice;
	int  m_iD3D9RenderDevice;
	BOOL m_bVMR9AlterativeVSync;
	BOOL m_bVMRMixerMode;
	BOOL m_bVMRMixerYUV;
	BOOL m_bResetDevice;
	//BOOL m_bDisableAero;

	int m_iAPSurfaceUsage;
	//int m_iOutputRange;
	CString m_iEvrBuffers;

	//BOOL m_bColorManagment;
	//int  m_iCMInputType;
	//int  m_iCMAmbientLight;
	//int  m_iCMRenderingIntent;

public:
	CPPageVideo();
	virtual ~CPPageVideo();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	void UpdateSurfaceFormatList(int select);
	void UpdateResizerList(int select);

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnUpdateMixerYUV(CCmdUI* pCmdUI);
	afx_msg void OnSurfaceChange();
	afx_msg void OnDSRendererChange();
	afx_msg void OnFullscreenCheck();
	afx_msg void OnD3D9DeviceCheck();
};

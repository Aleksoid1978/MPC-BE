/*
 * (C) 2003-2006 Gabest
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


// CPPageVideo dialog

class CPPageVideo : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageVideo)

private:
	enum { IDD = IDD_PPAGEVIDEO };

	CStringArray m_D3D9GUIDNames;

	CComboBox m_cbVideoRenderer;
	CComboBox m_cbD3D9RenderDevice;
	CComboBox m_cbDX9PresentMode;
	CComboBox m_cbDX9SurfaceFormat;
	CComboBox m_cbDX9Resizer;
	CComboBox m_cbDownscaler;
	CComboBox m_cbEVROutputRange;
	CComboBox m_cbFrameMode;
	CButton   m_chkD3DFullscreen;
	CButton   m_chk10bitOutput;
	CButton   m_chkMPCVRFullscreenControl;
	CButton   m_chkNoSmallUpscale;
	CButton   m_chkNoSmallDownscale;

	int m_iEvrBuffers = RS_EVRBUFFERS_DEF;
	CSpinButtonCtrl m_spnEvrBuffers;

	int m_iVideoRendererType_store = VIDRNDT_EVR;
	int m_iVideoRendererType = VIDRNDT_EVR;

	BOOL m_bD3D9RenderDevice = FALSE;
	int  m_iD3D9RenderDevice = -1;
	BOOL m_bResetDevice = FALSE;

public:
	CPPageVideo();
	virtual ~CPPageVideo();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	void UpdateResizerList(int select);
	void UpdateDownscalerList(int select);

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnDSRendererChange();
	afx_msg void OnResetDevice();
	afx_msg void OnFullscreenCheck();
	afx_msg void OnD3D9DeviceCheck();
	afx_msg void OnSurfaceFormatChange();
	afx_msg void OnFrameModeChange();
	afx_msg void OnBnClickedDefault();

	afx_msg void OnVideoRenderPropClick();
};

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

#include "stdafx.h"
#include <d3d9.h>
#include <clsids.h>
#include <mvrInterfaces.h>
#include "DSUtil/D3D9Helper.h"
#include "DSUtil/SysVersion.h"
#include "MultiMonitor.h"
#include "MainFrm.h"
#include "PPageVideo.h"
#include "ComPropertySheet.h"
#include "filters/renderer/VideoRenderers/EVRAllocatorPresenter.h"

// CPPageVideo dialog

static HRESULT IsRendererAvailable(int VideoRendererType)
{
	switch (VideoRendererType) {
		case VIDRNDT_EVR:
		case VIDRNDT_EVR_CP:
		case VIDRNDT_SYNC:
			return CheckFilterCLSID(CLSID_EnhancedVideoRenderer);
		case VIDRNDT_MPCVR:
			return CheckFilterCLSID(CLSID_MPCVR);
		case VIDRNDT_DXR:
			return CheckFilterCLSID(CLSID_DXR);
		case VIDRNDT_MADVR:
			return CheckFilterCLSID(CLSID_madVR);
		default:
		return S_OK;
	}
}

IMPLEMENT_DYNAMIC(CPPageVideo, CPPageBase)
CPPageVideo::CPPageVideo()
	: CPPageBase(CPPageVideo::IDD, CPPageVideo::IDD)
{
}

CPPageVideo::~CPPageVideo()
{
}

void CPPageVideo::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_VIDRND_COMBO, m_cbVideoRenderer);
	DDX_Control(pDX, IDC_D3D9DEVICE_COMBO, m_cbD3D9RenderDevice);
	DDX_Control(pDX, IDC_COMBO3, m_cbDX9PresentMode);
	DDX_Control(pDX, IDC_COMBO1, m_cbDX9SurfaceFormat);
	DDX_Control(pDX, IDC_DX9RESIZER_COMBO, m_cbDX9Resizer);
	DDX_Control(pDX, IDC_COMBO7, m_cbDownscaler);
	DDX_CBIndex(pDX, IDC_D3D9DEVICE_COMBO, m_iD3D9RenderDevice);
	DDX_Check(pDX, IDC_D3D9DEVICE_CHECK, m_bD3D9RenderDevice);
	DDX_Check(pDX, IDC_RESETDEVICE, m_bResetDevice);
	DDX_Control(pDX, IDC_EXCLUSIVE_FULLSCREEN_CHECK, m_chkD3DFullscreen);
	DDX_Control(pDX, IDC_CHECK1, m_chk10bitOutput);
	DDX_Control(pDX, IDC_COMBO2, m_cbEVROutputRange);
	DDX_Text(pDX, IDC_EVR_BUFFERS, m_iEvrBuffers);
	DDX_Control(pDX, IDC_SPIN1, m_spnEvrBuffers);
	DDX_Control(pDX, IDC_CHECK2, m_chkMPCVRFullscreenControl);
	DDX_Control(pDX, IDC_COMBO8, m_cbFrameMode);
	DDX_Control(pDX, IDC_CHECK7, m_chkNoSmallUpscale);
	DDX_Control(pDX, IDC_CHECK8, m_chkNoSmallDownscale);

	m_iEvrBuffers = std::clamp(m_iEvrBuffers, RS_EVRBUFFERS_MIN, RS_EVRBUFFERS_MAX);
}

BEGIN_MESSAGE_MAP(CPPageVideo, CPPageBase)
	ON_CBN_SELCHANGE(IDC_VIDRND_COMBO, OnVideoRendererChange)
	ON_BN_CLICKED(IDC_D3D9DEVICE_CHECK, OnD3D9DeviceCheck)
	ON_BN_CLICKED(IDC_RESETDEVICE, OnResetDevice)
	ON_BN_CLICKED(IDC_EXCLUSIVE_FULLSCREEN_CHECK, OnFullscreenCheck)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnSurfaceFormatChange)
	ON_CBN_SELCHANGE(IDC_COMBO8, OnFrameModeChange)
	ON_BN_CLICKED(IDC_BUTTON3, OnBnClickedDefault)
	ON_BN_CLICKED(IDC_BUTTON1, OnVideoRenderPropClick)
END_MESSAGE_MAP()

// CPPageVideo message handlers

BOOL CPPageVideo::OnInitDialog()
{
	__super::OnInitDialog();

	SetCursor(m_hWnd, IDC_VIDRND_COMBO, IDC_HAND);
	SetCursor(m_hWnd, IDC_D3D9DEVICE_COMBO, IDC_HAND);
	SetCursor(m_hWnd, IDC_DX9RESIZER_COMBO, IDC_HAND);
	SetCursor(m_hWnd, IDC_COMBO7, IDC_HAND);
	SetCursor(m_hWnd, IDC_COMBO1, IDC_HAND);
	SetCursor(m_hWnd, IDC_COMBO2, IDC_HAND);

	CAppSettings& s = AfxGetAppSettings();
	CRenderersSettings& rs = s.m_VRSettings;

	m_iVideoRendererType   = rs.iVideoRenderer;

	m_chkD3DFullscreen.SetCheck(rs.bExclusiveFullscreen);
	m_chk10bitOutput.EnableWindow(rs.bExclusiveFullscreen);
	m_chk10bitOutput.SetCheck(rs.ExtraSets.b10BitOutput);

	m_cbEVROutputRange.AddString(L"0-255");
	m_cbEVROutputRange.AddString(L"16-235");
	m_cbEVROutputRange.SetCurSel(rs.ExtraSets.iEVROutputRange);

	m_iEvrBuffers = rs.ExtraSets.nEVRBuffers;
	m_spnEvrBuffers.SetRange(RS_EVRBUFFERS_MIN, RS_EVRBUFFERS_MAX);

	m_bResetDevice = s.m_VRSettings.ExtraSets.bResetDevice;

	auto pD3D9 = D3D9Helper::Direct3DCreate9();
	if (pD3D9) {
		WCHAR strGUID[50] = {};
		CString cstrGUID;
		CString d3ddevice_str;
		D3DADAPTER_IDENTIFIER9 adapterIdentifier = {};

		for (UINT adp = 0; adp < pD3D9->GetAdapterCount(); ++adp) {
			if (SUCCEEDED(pD3D9->GetAdapterIdentifier(adp, 0, &adapterIdentifier))) {
				d3ddevice_str = adapterIdentifier.Description;
				cstrGUID.Empty();
				if (::StringFromGUID2(adapterIdentifier.DeviceIdentifier, strGUID, 50) > 0) {
					cstrGUID = strGUID;
				}
				if (!cstrGUID.IsEmpty()) {
					BOOL bExists = FALSE;
					for (INT_PTR i = 0; !bExists && i < m_D3D9GUIDNames.GetCount(); i++) {
						if (m_D3D9GUIDNames.GetAt(i) == cstrGUID) {
							bExists = TRUE;
						}
					}
					if (!bExists) {
						m_cbD3D9RenderDevice.AddString(d3ddevice_str);
						m_D3D9GUIDNames.Add(cstrGUID);
						if (rs.ExtraSets.sD3DRenderDevice == cstrGUID) {
							m_iD3D9RenderDevice = m_cbD3D9RenderDevice.GetCount() - 1;
						}
					}
				}
			}
		}

		pD3D9->Release();
	}

	CorrectComboListWidth(m_cbD3D9RenderDevice);

	auto addRenderer = [&](int iVR, UINT nID) {
		switch (iVR) {
		case VIDRNDT_EVR:
		case VIDRNDT_EVR_CP:
		case VIDRNDT_SYNC:
		case VIDRNDT_MPCVR:
		case VIDRNDT_DXR:
		case VIDRNDT_MADVR:
		case VIDRNDT_NULL_ANY:
		case VIDRNDT_NULL_UNCOMP:
			break;
		default:
			ASSERT(FALSE);
			return;
		}
		CString sName = ResStr(nID);

		HRESULT hr = IsRendererAvailable(iVR);
		if (S_FALSE == hr) {
			sName.AppendFormat(L" %s", ResStr(IDS_REND_NOT_AVAILABLE));
		} else if (FAILED(hr)) {
			sName.AppendFormat(L" %s", ResStr(IDS_REND_NOT_INSTALLED));
		}

		AddStringData(m_cbVideoRenderer, sName, iVR);
	};


	CComboBox& m_iDSVRTC = m_cbVideoRenderer;
	m_iDSVRTC.SetRedraw(FALSE);
	addRenderer(VIDRNDT_EVR,         IDS_PPAGE_OUTPUT_EVR);
	addRenderer(VIDRNDT_EVR_CP,      IDS_PPAGE_OUTPUT_EVR_CUSTOM);
	addRenderer(VIDRNDT_SYNC,        IDS_PPAGE_OUTPUT_SYNC);
	addRenderer(VIDRNDT_MPCVR,       IDS_PPAGE_OUTPUT_MPCVR);
	addRenderer(VIDRNDT_DXR,         IDS_PPAGE_OUTPUT_DXR);
	addRenderer(VIDRNDT_MADVR,       IDS_PPAGE_OUTPUT_MADVR);
	addRenderer(VIDRNDT_NULL_ANY,    IDS_PPAGE_OUTPUT_NULL_ANY);
	addRenderer(VIDRNDT_NULL_UNCOMP, IDS_PPAGE_OUTPUT_NULL_UNCOMP);

	SelectByItemData(m_cbVideoRenderer, m_iVideoRendererType);

	m_iDSVRTC.SetRedraw(TRUE);
	m_iDSVRTC.Invalidate();
	m_iDSVRTC.UpdateWindow();

	UpdateData(FALSE);

	CreateToolTip();
	m_wndToolTip.AddTool(GetDlgItem(IDC_VIDRND_COMBO), L"");

	OnVideoRendererChange();

	AddStringData(m_cbDX9PresentMode, L"Copy", 0);
	AddStringData(m_cbDX9PresentMode, L"Flip/FlipEx", 1);
	m_cbDX9PresentMode.SetCurSel(0); // default
	SelectByItemData(m_cbDX9PresentMode, rs.ExtraSets.iPresentMode);
	CorrectComboListWidth(m_cbDX9PresentMode);

	AddStringData(m_cbDX9SurfaceFormat, L"8-bit Integer", D3DFMT_X8R8G8B8);
	AddStringData(m_cbDX9SurfaceFormat, L"10-bit Integer", D3DFMT_A2R10G10B10);
	AddStringData(m_cbDX9SurfaceFormat, L"16-bit Floating Point", D3DFMT_A16B16G16R16F);
	m_cbDX9SurfaceFormat.SetCurSel(0); // default
	SelectByItemData(m_cbDX9SurfaceFormat, rs.ExtraSets.iSurfaceFormat);
	CorrectComboListWidth(m_cbDX9SurfaceFormat);

	UpdateResizerList(rs.ExtraSets.iResizer);
	UpdateDownscalerList(rs.ExtraSets.iDownscaler);

	CheckDlgButton(IDC_D3D9DEVICE_CHECK, BST_UNCHECKED);
	GetDlgItem(IDC_D3D9DEVICE_CHECK)->EnableWindow(FALSE);
	m_cbD3D9RenderDevice.EnableWindow(FALSE);

	switch (m_iVideoRendererType) {
		case VIDRNDT_EVR_CP:
			if (m_cbD3D9RenderDevice.GetCount() > 1) {
					GetDlgItem(IDC_D3D9DEVICE_CHECK)->EnableWindow(TRUE);
					m_cbD3D9RenderDevice.EnableWindow(FALSE);
					CheckDlgButton(IDC_D3D9DEVICE_CHECK, BST_UNCHECKED);
					if (m_iD3D9RenderDevice != -1) {
						CheckDlgButton(IDC_D3D9DEVICE_CHECK, BST_CHECKED);
						m_cbD3D9RenderDevice.EnableWindow(TRUE);
					}
			}
			break;
		default:
			GetDlgItem(IDC_D3D9DEVICE_CHECK)->EnableWindow(FALSE);
			m_cbD3D9RenderDevice.EnableWindow(FALSE);
	}

	OnSurfaceFormatChange();

	m_chkMPCVRFullscreenControl.SetCheck(rs.ExtraSets.bMPCVRFullscreenControl);

	m_cbFrameMode.AddString(ResStr(IDS_FRAME_HALF));
	m_cbFrameMode.AddString(ResStr(IDS_FRAME_NORMAL));
	m_cbFrameMode.AddString(ResStr(IDS_FRAME_DOUBLE));
	m_cbFrameMode.AddString(ResStr(IDS_FRAME_STRETCH));
	m_cbFrameMode.AddString(ResStr(IDS_FRAME_FROMINSIDE));
	m_cbFrameMode.AddString(ResStr(IDS_FRAME_FROMOUTSIDE));
	m_cbFrameMode.AddString(ResStr(IDS_FRAME_ZOOM1));
	m_cbFrameMode.AddString(ResStr(IDS_FRAME_ZOOM2));
	m_cbFrameMode.SetCurSel(s.iDefaultVideoSize);

	m_chkNoSmallUpscale.SetCheck(s.bNoSmallUpscale);
	m_chkNoSmallDownscale.SetCheck(s.bNoSmallDownscale);
	OnFrameModeChange();

	UpdateData(TRUE);
	SetModified(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
}

BOOL CPPageVideo::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();
	CRenderersSettings& rs = s.m_VRSettings;

	rs.iVideoRenderer = m_iVideoRendererType = m_iVideoRendererType_store = GetCurItemData(m_cbVideoRenderer);
	rs.ExtraSets.iResizer        = GetCurItemData(m_cbDX9Resizer);
	rs.ExtraSets.iDownscaler     = GetCurItemData(m_cbDownscaler);
	rs.bExclusiveFullscreen      = !!m_chkD3DFullscreen.GetCheck();
	rs.ExtraSets.bResetDevice    = !!m_bResetDevice;

	rs.ExtraSets.iPresentMode    = (int)GetCurItemData(m_cbDX9PresentMode);
	rs.ExtraSets.iSurfaceFormat  = (D3DFORMAT)GetCurItemData(m_cbDX9SurfaceFormat);
	rs.ExtraSets.b10BitOutput    = !!m_chk10bitOutput.GetCheck();
	rs.ExtraSets.iEVROutputRange = m_cbEVROutputRange.GetCurSel();

	rs.ExtraSets.nEVRBuffers = m_iEvrBuffers;

	if (m_bD3D9RenderDevice) {
		rs.ExtraSets.sD3DRenderDevice = m_D3D9GUIDNames[m_iD3D9RenderDevice];
	} else {
		rs.ExtraSets.sD3DRenderDevice.Empty();
	}

	rs.ExtraSets.bMPCVRFullscreenControl = !!m_chkMPCVRFullscreenControl.GetCheck();

	s.iDefaultVideoSize = m_cbFrameMode.GetCurSel();
	s.bNoSmallUpscale = !!m_chkNoSmallUpscale.GetCheck();
	s.bNoSmallDownscale = !!m_chkNoSmallDownscale.GetCheck();

	AfxGetMainFrame()->ApplyExraRendererSettings();

	return __super::OnApply();
}

void CPPageVideo::UpdateResizerList(int select)
{
	m_cbDX9Resizer.SetRedraw(FALSE);
	m_cbDX9Resizer.ResetContent();

	AddStringData(m_cbDX9Resizer, L"Nearest neighbor", RESIZER_NEAREST);
	AddStringData(m_cbDX9Resizer, L"Bilinear", RESIZER_BILINEAR);
#if DXVA2VP
	if ((D3DFORMAT)GetCurItemData(m_cbDX9SurfaceFormat) == D3DFMT_X8R8G8B8 && GetCurItemData(m_cbVideoRenderer) != VIDRNDT_SYNC) {
		AddStringData(m_cbDX9Resizer, L"DXVA2 (Intel GPU only)", RESIZER_DXVA2);
	}
#endif
#if DXVAHDVP
	if ((D3DFORMAT)GetCurItemData(m_cbDX9SurfaceFormat) == D3DFMT_X8R8G8B8 && GetCurItemData(m_cbVideoRenderer) != VIDRNDT_SYNC) {
		AddStringData(m_cbDX9Resizer, L"DXVA-HD (Intel GPU only)", RESIZER_DXVAHD);
	}
#endif
	AddStringData(m_cbDX9Resizer, L"PS: B-spline", RESIZER_SHADER_BSPLINE);
	AddStringData(m_cbDX9Resizer, L"PS: Mitchell-Netravali", RESIZER_SHADER_MITCHELL);
	AddStringData(m_cbDX9Resizer, L"PS: Catmull-Rom", RESIZER_SHADER_CATMULL);
	AddStringData(m_cbDX9Resizer, L"PS: Bicubic A=-0.6", RESIZER_SHADER_BICUBIC06);
	AddStringData(m_cbDX9Resizer, L"PS: Bicubic A=-0.8", RESIZER_SHADER_BICUBIC08);
	AddStringData(m_cbDX9Resizer, L"PS: Bicubic A=-1.0", RESIZER_SHADER_BICUBIC10);
	AddStringData(m_cbDX9Resizer, L"PS: Lanczos2", RESIZER_SHADER_LANCZOS2);
	AddStringData(m_cbDX9Resizer, L"PS: Lanczos3", RESIZER_SHADER_LANCZOS3);

	m_cbDX9Resizer.SetCurSel(1); // default
	SelectByItemData(m_cbDX9Resizer, select);
	m_cbDX9Resizer.SetRedraw(TRUE);
}

void CPPageVideo::UpdateDownscalerList(int select)
{
	m_cbDownscaler.SetRedraw(FALSE);
	m_cbDownscaler.ResetContent();

	AddStringData(m_cbDownscaler, L"Nearest neighbor", RESIZER_NEAREST);
	AddStringData(m_cbDownscaler, L"Bilinear", RESIZER_BILINEAR);
#if DXVA2VP
	if ((D3DFORMAT)GetCurItemData(m_cbDX9SurfaceFormat) == D3DFMT_X8R8G8B8 && GetCurItemData(m_cbVideoRenderer) != VIDRNDT_SYNC) {
		AddStringData(m_cbDownscaler, L"DXVA2 (Intel GPU only)", RESIZER_DXVA2);
	}
#endif
#if DXVAHDVP
	if ((D3DFORMAT)GetCurItemData(m_cbDX9SurfaceFormat) == D3DFMT_X8R8G8B8 && GetCurItemData(m_cbVideoRenderer) != VIDRNDT_SYNC) {
		AddStringData(m_cbDownscaler, L"DXVA-HD (Intel GPU only)", RESIZER_DXVAHD);
	}
#endif
	AddStringData(m_cbDownscaler, L"PS: Simple averaging", DOWNSCALER_SIMPLE);
	AddStringData(m_cbDownscaler, L"PS: Box", DOWNSCALER_BOX);
	AddStringData(m_cbDownscaler, L"PS: Bilinear", DOWNSCALER_BILINEAR);
	AddStringData(m_cbDownscaler, L"PS: Hamming", DOWNSCALER_HAMMING);
	AddStringData(m_cbDownscaler, L"PS: Bicubic", DOWNSCALER_BICUBIC);
	AddStringData(m_cbDownscaler, L"PS: Lanczos", DOWNSCALER_LANCZOS);

	m_cbDownscaler.SetCurSel(1); // default
	SelectByItemData(m_cbDownscaler, select);
	m_cbDownscaler.SetRedraw(TRUE);
}

void CPPageVideo::OnVideoRendererChange()
{
	int CurrentVR = (int)GetCurItemData(m_cbVideoRenderer);

	CRenderersSettings& rs = GetRenderersSettings();

	//m_cbAPSurfaceUsage.EnableWindow(FALSE);
	m_cbDX9PresentMode.EnableWindow(FALSE);
	m_cbDX9SurfaceFormat.EnableWindow(FALSE);
	m_cbDX9Resizer.EnableWindow(FALSE);
	m_cbDownscaler.EnableWindow(FALSE);
	m_chkD3DFullscreen.EnableWindow(FALSE);
	m_chk10bitOutput.EnableWindow(FALSE);
	GetDlgItem(IDC_RESETDEVICE)->EnableWindow(FALSE);
	GetDlgItem(IDC_EVR_BUFFERS)->EnableWindow(FALSE);
	m_spnEvrBuffers.EnableWindow(FALSE);
	GetDlgItem(IDC_D3D9DEVICE_CHECK)->EnableWindow(FALSE);
	m_cbD3D9RenderDevice.EnableWindow(FALSE);
	m_cbEVROutputRange.EnableWindow(FALSE);
	GetDlgItem(IDC_STATIC6)->EnableWindow(FALSE);
	GetDlgItem(IDC_STATIC2)->EnableWindow(FALSE);
	GetDlgItem(IDC_STATIC3)->EnableWindow(FALSE);
	GetDlgItem(IDC_STATIC10)->EnableWindow(FALSE);
	GetDlgItem(IDC_STATIC4)->EnableWindow(FALSE);
	GetDlgItem(IDC_STATIC5)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON1)->EnableWindow(FALSE);
	m_chkMPCVRFullscreenControl.EnableWindow(FALSE);

	switch (CurrentVR) {
		case VIDRNDT_EVR:
			m_wndToolTip.UpdateTipText(ResStr(IDS_DESC_EVR), &m_cbVideoRenderer);
			break;
		case VIDRNDT_EVR_CP:
			if (m_cbD3D9RenderDevice.GetCount() > 1) {
				GetDlgItem(IDC_D3D9DEVICE_CHECK)->EnableWindow(TRUE);
				m_cbD3D9RenderDevice.EnableWindow(IsDlgButtonChecked(IDC_D3D9DEVICE_CHECK));
			}

			UpdateResizerList(rs.ExtraSets.iResizer);
			UpdateDownscalerList(rs.ExtraSets.iDownscaler);

			GetDlgItem(IDC_STATIC6)->EnableWindow(TRUE);
			m_cbDX9PresentMode.EnableWindow(TRUE);
			GetDlgItem(IDC_STATIC2)->EnableWindow(TRUE);
			m_cbDX9SurfaceFormat.EnableWindow(TRUE);
			GetDlgItem(IDC_STATIC3)->EnableWindow(TRUE);
			GetDlgItem(IDC_STATIC10)->EnableWindow(TRUE);
			m_cbDX9Resizer.EnableWindow(TRUE);
			m_cbDownscaler.EnableWindow(TRUE);
			m_chkD3DFullscreen.EnableWindow(TRUE);
			m_chk10bitOutput.EnableWindow(m_chkD3DFullscreen.GetCheck() == BST_CHECKED);
			GetDlgItem(IDC_RESETDEVICE)->EnableWindow(TRUE);
			GetDlgItem(IDC_EVR_BUFFERS)->EnableWindow(TRUE);
			GetDlgItem(IDC_STATIC5)->EnableWindow(TRUE);
			m_spnEvrBuffers.EnableWindow(TRUE);

			GetDlgItem(IDC_STATIC4)->EnableWindow(TRUE);
			m_cbEVROutputRange.EnableWindow(TRUE);

			m_wndToolTip.UpdateTipText(ResStr(IDS_DESC_EVR_CP), &m_cbVideoRenderer);
			break;
		case VIDRNDT_SYNC:
			UpdateResizerList(GetCurItemData(m_cbDX9Resizer));

			GetDlgItem(IDC_STATIC5)->EnableWindow(TRUE);
			GetDlgItem(IDC_EVR_BUFFERS)->EnableWindow(TRUE);
			m_spnEvrBuffers.EnableWindow(TRUE);
			GetDlgItem(IDC_STATIC3)->EnableWindow(TRUE);
			m_cbDX9Resizer.EnableWindow(TRUE);
			GetDlgItem(IDC_RESETDEVICE)->EnableWindow(TRUE);

			GetDlgItem(IDC_STATIC4)->EnableWindow(TRUE);
			m_cbEVROutputRange.EnableWindow(TRUE);

			m_wndToolTip.UpdateTipText(ResStr(IDS_DESC_EVR_SYNC), &m_cbVideoRenderer);
			break;
		case VIDRNDT_DXR:
			m_wndToolTip.UpdateTipText(ResStr(IDS_DESC_HAALI_VR), &m_cbVideoRenderer);
			GetDlgItem(IDC_BUTTON1)->EnableWindow(IsRendererAvailable(CurrentVR) == S_OK ? TRUE : FALSE);
			break;
		case VIDRNDT_NULL_ANY:
			m_wndToolTip.UpdateTipText(ResStr(IDS_DESC_NULLVR_ANY), &m_cbVideoRenderer);
			break;
		case VIDRNDT_NULL_UNCOMP:
			m_wndToolTip.UpdateTipText(ResStr(IDS_DESC_NULLVR_UNCOMP), &m_cbVideoRenderer);
			break;
		case VIDRNDT_MADVR:
			m_wndToolTip.UpdateTipText(ResStr(IDS_DESC_MADVR), &m_cbVideoRenderer);
			GetDlgItem(IDC_BUTTON1)->EnableWindow(IsRendererAvailable(CurrentVR) == S_OK ? TRUE : FALSE);
			break;
		case VIDRNDT_MPCVR:
			m_wndToolTip.UpdateTipText(ResStr(IDS_DESC_MPC_VR), &m_cbVideoRenderer);
			m_chkMPCVRFullscreenControl.EnableWindow(TRUE);
			GetDlgItem(IDC_BUTTON1)->EnableWindow(IsRendererAvailable(CurrentVR) == S_OK ? TRUE : FALSE);
			break;
		default:
			m_wndToolTip.UpdateTipText(L"", &m_cbVideoRenderer);
	}

	AfxGetAppSettings().iSelectedVideoRenderer = CurrentVR;

	SetModified();
}

void CPPageVideo::OnResetDevice()
{
	SetModified();
}

void CPPageVideo::OnFullscreenCheck()
{
	if (m_chkD3DFullscreen.GetCheck() == BST_CHECKED && (MessageBoxW(ResStr(IDS_EXCLUSIVE_FS_WARNING), nullptr, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES)) {
		m_chk10bitOutput.EnableWindow(TRUE);
	} else {
		m_chkD3DFullscreen.SetCheck(BST_UNCHECKED);
		m_chk10bitOutput.EnableWindow(FALSE);
	}
	SetModified();
}

void CPPageVideo::OnD3D9DeviceCheck()
{
	UpdateData();
	m_cbD3D9RenderDevice.EnableWindow(m_bD3D9RenderDevice);
	SetModified();
}

void CPPageVideo::OnSurfaceFormatChange()
{
	D3DFORMAT surfmt = (D3DFORMAT)GetCurItemData(m_cbDX9SurfaceFormat);
	UINT CurrentVR = GetCurItemData(m_cbVideoRenderer);

	UpdateResizerList(GetCurItemData(m_cbDX9Resizer));
	UpdateDownscalerList(GetCurItemData(m_cbDownscaler));

	SetModified();
}

void CPPageVideo::OnFrameModeChange()
{
	if (m_cbFrameMode.GetCurSel() == ID_VIEW_VF_FROMINSIDE - ID_VIEW_VF_HALF) {
		m_chkNoSmallUpscale.EnableWindow(TRUE);
		m_chkNoSmallDownscale.EnableWindow(TRUE);
	} else {
		m_chkNoSmallUpscale.EnableWindow(FALSE);
		m_chkNoSmallDownscale.EnableWindow(FALSE);
	}

	SetModified();
}

void CPPageVideo::OnBnClickedDefault()
{
	m_bD3D9RenderDevice = FALSE;
	m_bResetDevice = FALSE;
	m_iEvrBuffers = RS_EVRBUFFERS_DEF;

	UpdateData(FALSE);

	HRESULT hr = CheckFilterCLSID(CLSID_MPCVR);
	SelectByItemData(m_cbVideoRenderer, (S_OK == hr) ? VIDRNDT_MPCVR : VIDRNDT_EVR_CP);

	m_cbEVROutputRange.SetCurSel(0);

	m_chkD3DFullscreen.SetCheck(BST_UNCHECKED);
	m_chk10bitOutput.SetCheck(BST_UNCHECKED);

	SelectByItemData(m_cbDX9PresentMode, 0);
	SelectByItemData(m_cbDX9SurfaceFormat, D3DFMT_X8R8G8B8);
	UpdateResizerList(RESIZER_SHADER_CATMULL);
	UpdateDownscalerList(DOWNSCALER_SIMPLE);

	OnFullscreenCheck();
	OnSurfaceFormatChange();
	OnVideoRendererChange();

	m_chkMPCVRFullscreenControl.SetCheck(BST_UNCHECKED);

	m_cbFrameMode.SetCurSel(ID_VIEW_VF_FROMINSIDE - ID_VIEW_VF_HALF);
	m_chkNoSmallUpscale.SetCheck(BST_UNCHECKED);
	m_chkNoSmallUpscale.EnableWindow(TRUE);
	m_chkNoSmallDownscale.SetCheck(BST_UNCHECKED);
	m_chkNoSmallDownscale.EnableWindow(TRUE);

	Invalidate();
	SetModified();
}

void CPPageVideo::OnVideoRenderPropClick()
{
	const auto& s = AfxGetAppSettings();

	CLSID clsid;
	switch (s.iSelectedVideoRenderer) {
	case VIDRNDT_MPCVR: clsid = CLSID_MPCVR; break;
	case VIDRNDT_DXR:   clsid = CLSID_DXR;   break;
	case VIDRNDT_MADVR: clsid = CLSID_madVR; break;
	default:
		return;
	}

	CComPtr<IBaseFilter> pBF;
	CComQIPtr<ISpecifyPropertyPages> pSPP = FindFilter(clsid, AfxGetMainFrame()->m_pGB);
	if (!pSPP) {
		pBF.CoCreateInstance(clsid); pSPP = pBF;
	}
	if (pSPP) {
		CComPropertySheet ps(ResStr(IDS_PROPSHEET_PROPERTIES), this);
		ps.AddPages(pSPP);
		ps.DoModal();
	}
}

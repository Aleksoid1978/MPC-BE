/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2018 see Authors.txt
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
#include <moreuuids.h>
#include "../../DSUtil/D3D9Helper.h"
#include "../../DSUtil/SysVersion.h"
#include "MultiMonitor.h"
#include "PPageVideo.h"

// CPPageVideo dialog

static bool IsRenderTypeAvailable(int VideoRendererType, HWND hwnd)
{
	switch (VideoRendererType) {
		case VIDRNDT_EVR:
		case VIDRNDT_EVR_CUSTOM:
		case VIDRNDT_SYNC:
			return IsCLSIDRegistered(CLSID_EnhancedVideoRenderer);
		case VIDRNDT_DXR:
			return IsCLSIDRegistered(CLSID_DXR);
		case VIDRNDT_MADVR:
			return IsCLSIDRegistered(CLSID_madVR);
#if MPCVR
		case VIDRNDT_MPCVR:
			return IsCLSIDRegistered(CLSID_MPCVR);
#endif
		default:
		return true;
	}
}

IMPLEMENT_DYNAMIC(CPPageVideo, CPPageBase)
CPPageVideo::CPPageVideo()
	: CPPageBase(CPPageVideo::IDD, CPPageVideo::IDD)
	, m_iVideoRendererType(VIDRNDT_SYSDEFAULT)
	, m_iVideoRendererType_store(VIDRNDT_SYSDEFAULT)
	, m_bResetDevice(FALSE)
	, m_iEvrBuffers(RS_EVRBUFFERS_DEF)
	, m_bD3D9RenderDevice(FALSE)
	, m_iD3D9RenderDevice(-1)
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
	DDX_Control(pDX, IDC_DSVMRLOADMIXER, m_chkVMRMixerMode);
	DDX_Control(pDX, IDC_DSVMRYUVMIXER, m_chkVMRMixerYUV);
	DDX_Control(pDX, IDC_COMBO2, m_cbEVROutputRange);
	DDX_Text(pDX, IDC_EVR_BUFFERS, m_iEvrBuffers);
	DDX_Control(pDX, IDC_SPIN1, m_spnEvrBuffers);
	DDX_Control(pDX, IDC_COMBO8, m_cbFrameMode);
	DDX_Control(pDX, IDC_CHECK7, m_chkNoSmallUpscale);
	DDX_Control(pDX, IDC_CHECK8, m_chkNoSmallDownscale);

	m_iEvrBuffers = std::clamp(m_iEvrBuffers, RS_EVRBUFFERS_MIN, RS_EVRBUFFERS_MAX);
}

BEGIN_MESSAGE_MAP(CPPageVideo, CPPageBase)
	ON_CBN_SELCHANGE(IDC_VIDRND_COMBO, OnDSRendererChange)
	ON_BN_CLICKED(IDC_D3D9DEVICE_CHECK, OnD3D9DeviceCheck)
	ON_BN_CLICKED(IDC_RESETDEVICE, OnResetDevice)
	ON_BN_CLICKED(IDC_EXCLUSIVE_FULLSCREEN_CHECK, OnFullscreenCheck)
	ON_UPDATE_COMMAND_UI(IDC_DSVMRYUVMIXER, OnUpdateMixerYUV)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnSurfaceFormatChange)
	ON_CBN_SELCHANGE(IDC_COMBO8, OnFrameModeChange)
	ON_BN_CLICKED(IDC_BUTTON3, OnBnClickedDefault)
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

	m_chkD3DFullscreen.SetCheck(s.fD3DFullscreen);
	m_chk10bitOutput.EnableWindow(s.fD3DFullscreen);
	m_chk10bitOutput.SetCheck(rs.b10BitOutput);
	m_chkVMRMixerMode.SetCheck(rs.bVMRMixerMode);
	m_chkVMRMixerYUV.SetCheck(rs.bVMRMixerYUV);

	m_cbEVROutputRange.AddString(L"0-255");
	m_cbEVROutputRange.AddString(L"16-235");
	m_cbEVROutputRange.SetCurSel(rs.iEVROutputRange);

	m_iEvrBuffers = rs.nEVRBuffers;
	m_spnEvrBuffers.SetRange(RS_EVRBUFFERS_MIN, RS_EVRBUFFERS_MAX);

	m_bResetDevice = s.m_VRSettings.bResetDevice;

	CComPtr<IDirect3D9> pD3D9 = D3D9Helper::Direct3DCreate9();
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
						if (rs.sD3DRenderDevice == cstrGUID) {
							m_iD3D9RenderDevice = m_cbD3D9RenderDevice.GetCount() - 1;
						}
					}
				}
			}
		}
	}

	CorrectComboListWidth(m_cbD3D9RenderDevice);

	auto addRenderer = [&](int iVR, UINT nID) {
		switch (iVR) {
		case VIDRNDT_SYSDEFAULT:
		case VIDRNDT_VMR9WINDOWED:
		case VIDRNDT_EVR:
		case VIDRNDT_EVR_CUSTOM:
		case VIDRNDT_SYNC:
		case VIDRNDT_DXR:
		case VIDRNDT_MADVR:
		case VIDRNDT_NULL_COMP:
		case VIDRNDT_NULL_UNCOMP:
			break;
		default:
			ASSERT(FALSE);
			return;
		}
		CString sName = ResStr(nID);

		if (!IsRenderTypeAvailable(iVR, m_hWnd)) {
			sName.AppendFormat(L" %s", ResStr(IDS_REND_NOT_AVAILABLE));
		}

		AddStringData(m_cbVideoRenderer, sName, iVR);
	};


	CComboBox& m_iDSVRTC = m_cbVideoRenderer;
	m_iDSVRTC.SetRedraw(FALSE);
	addRenderer(VIDRNDT_SYSDEFAULT,   IDS_PPAGE_OUTPUT_SYS_DEF);
	addRenderer(VIDRNDT_VMR9WINDOWED, IDS_PPAGE_OUTPUT_VMR9WINDOWED);
	addRenderer(VIDRNDT_EVR,          IDS_PPAGE_OUTPUT_EVR);
	addRenderer(VIDRNDT_EVR_CUSTOM,   IDS_PPAGE_OUTPUT_EVR_CUSTOM);
	addRenderer(VIDRNDT_SYNC,         IDS_PPAGE_OUTPUT_SYNC);
	addRenderer(VIDRNDT_DXR,          IDS_PPAGE_OUTPUT_DXR);
	addRenderer(VIDRNDT_MADVR,        IDS_PPAGE_OUTPUT_MADVR);
	addRenderer(VIDRNDT_NULL_COMP,    IDS_PPAGE_OUTPUT_NULL_COMP);
	addRenderer(VIDRNDT_NULL_UNCOMP,  IDS_PPAGE_OUTPUT_NULL_UNCOMP);

#if MPCVR
	CString sName = L"Experimental MPC Video Renderer";
	if (!IsRenderTypeAvailable(VIDRNDT_MPCVR, m_hWnd)) {
		sName.AppendFormat(L" %s", ResStr(IDS_REND_NOT_AVAILABLE));
	}
	AddStringData(m_cbVideoRenderer, sName, VIDRNDT_MPCVR);
#endif

	for (int i = 0; i < m_iDSVRTC.GetCount(); ++i) {
		if (m_iVideoRendererType == m_iDSVRTC.GetItemData(i)) {
			if (IsRenderTypeAvailable(m_iVideoRendererType, m_hWnd)) {
				m_iDSVRTC.SetCurSel(i);
				m_iVideoRendererType_store = m_iVideoRendererType;
			} else {
				m_iDSVRTC.SetCurSel(0);
			}
			break;
		}
	}

	m_iDSVRTC.SetRedraw(TRUE);
	m_iDSVRTC.Invalidate();
	m_iDSVRTC.UpdateWindow();

	UpdateData(FALSE);

	CreateToolTip();
	m_wndToolTip.AddTool(GetDlgItem(IDC_VIDRND_COMBO), L"");

	OnDSRendererChange();

	AddStringData(m_cbDX9PresentMode, L"Copy", 0);
	AddStringData(m_cbDX9PresentMode, L"Flip/FlipEx", 1);
	m_cbDX9PresentMode.SetCurSel(0); // default
	SelectByItemData(m_cbDX9PresentMode, rs.iPresentMode);
	CorrectComboListWidth(m_cbDX9PresentMode);

	AddStringData(m_cbDX9SurfaceFormat, L"8-bit Integer", D3DFMT_X8R8G8B8);
	AddStringData(m_cbDX9SurfaceFormat, L"10-bit Integer", D3DFMT_A2R10G10B10);
	AddStringData(m_cbDX9SurfaceFormat, L"16-bit Floating Point", D3DFMT_A16B16G16R16F);
	m_cbDX9SurfaceFormat.SetCurSel(0); // default
	SelectByItemData(m_cbDX9SurfaceFormat, rs.iSurfaceFormat);
	CorrectComboListWidth(m_cbDX9SurfaceFormat);

	UpdateResizerList(rs.iResizer);
	UpdateDownscalerList(rs.iDownscaler);

	CheckDlgButton(IDC_D3D9DEVICE_CHECK, BST_UNCHECKED);
	GetDlgItem(IDC_D3D9DEVICE_CHECK)->EnableWindow(FALSE);
	m_cbD3D9RenderDevice.EnableWindow(FALSE);

	switch (m_iVideoRendererType) {
		case VIDRNDT_EVR_CUSTOM:
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

	rs.iVideoRenderer	= m_iVideoRendererType = m_iVideoRendererType_store = GetCurItemData(m_cbVideoRenderer);
	rs.iResizer			= GetCurItemData(m_cbDX9Resizer);
	rs.iDownscaler		= GetCurItemData(m_cbDownscaler);
	rs.bVMRMixerMode	= !!m_chkVMRMixerMode.GetCheck();
	rs.bVMRMixerYUV		= !!m_chkVMRMixerYUV.GetCheck();
	s.fD3DFullscreen	= !!m_chkD3DFullscreen.GetCheck();
	rs.bResetDevice		= !!m_bResetDevice;

	rs.iPresentMode		= (int)GetCurItemData(m_cbDX9PresentMode);
	rs.iSurfaceFormat	= (D3DFORMAT)GetCurItemData(m_cbDX9SurfaceFormat);
	rs.b10BitOutput		= !!m_chk10bitOutput.GetCheck();
	rs.iEVROutputRange	= m_cbEVROutputRange.GetCurSel();

	rs.nEVRBuffers = m_iEvrBuffers;

	rs.sD3DRenderDevice = m_bD3D9RenderDevice ? m_D3D9GUIDNames[m_iD3D9RenderDevice] : L"";

	s.iDefaultVideoSize = m_cbFrameMode.GetCurSel();
	s.bNoSmallUpscale = !!m_chkNoSmallUpscale.GetCheck();
	s.bNoSmallDownscale = !!m_chkNoSmallDownscale.GetCheck();

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
	AddStringData(m_cbDX9Resizer, L"PS 3.0: Lanczos3", RESIZER_SHADER_LANCZOS3);

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
	AddStringData(m_cbDownscaler, L"PS 3.0: Bilinear", DOWNSCALER_BILINEAR);
	AddStringData(m_cbDownscaler, L"PS 3.0: Hamming", DOWNSCALER_HAMMING);
	AddStringData(m_cbDownscaler, L"PS 3.0: Bicubic", DOWNSCALER_BICUBIC);
	AddStringData(m_cbDownscaler, L"PS 3.0: Lanczos", DOWNSCALER_LANCZOS);

	m_cbDownscaler.SetCurSel(1); // default
	SelectByItemData(m_cbDownscaler, select);
	m_cbDownscaler.SetRedraw(TRUE);
}

void CPPageVideo::OnUpdateMixerYUV(CCmdUI* pCmdUI)
{
	int vrenderer = GetCurItemData(m_cbVideoRenderer);

	pCmdUI->Enable(!!IsDlgButtonChecked(IDC_DSVMRLOADMIXER) && vrenderer == VIDRNDT_VMR9WINDOWED);
}

void CPPageVideo::OnDSRendererChange()
{
	int CurrentVR = (int)GetCurItemData(m_cbVideoRenderer);

	if (!IsRenderTypeAvailable(CurrentVR, m_hWnd)) {
		AfxMessageBox(IDS_PPAGE_OUTPUT_UNAVAILABLEMSG, MB_ICONEXCLAMATION | MB_OK, 0);

		// revert to the last saved renderer
		m_cbVideoRenderer.SetCurSel(0);
		for (int i = 0; i < m_cbVideoRenderer.GetCount(); ++i) {
			if (m_iVideoRendererType_store == m_cbVideoRenderer.GetItemData(i)) {
				m_cbVideoRenderer.SetCurSel(i);
				CurrentVR = (int)m_cbVideoRenderer.GetItemData(i);
				break;
			}
		}
	}

	CRenderersSettings& rs = GetRenderersSettings();

	//m_cbAPSurfaceUsage.EnableWindow(FALSE);
	m_cbDX9PresentMode.EnableWindow(FALSE);
	m_cbDX9SurfaceFormat.EnableWindow(FALSE);
	m_cbDX9Resizer.EnableWindow(FALSE);
	m_cbDownscaler.EnableWindow(FALSE);
	m_chkD3DFullscreen.EnableWindow(FALSE);
	m_chk10bitOutput.EnableWindow(FALSE);
	m_chkVMRMixerMode.EnableWindow(FALSE);
	m_chkVMRMixerYUV.EnableWindow(FALSE);
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

	switch (CurrentVR) {
		case VIDRNDT_SYSDEFAULT:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSSYSDEF), &m_cbVideoRenderer);
			break;
		case VIDRNDT_VMR9WINDOWED:
			m_chkVMRMixerMode.EnableWindow(TRUE);
			m_chkVMRMixerYUV.EnableWindow(TRUE);

			m_wndToolTip.UpdateTipText(ResStr(IDC_DSVMR9WIN), &m_cbVideoRenderer);
			break;
		case VIDRNDT_EVR:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSEVR), &m_cbVideoRenderer);
			break;
		case VIDRNDT_EVR_CUSTOM:
			if (m_cbD3D9RenderDevice.GetCount() > 1) {
				GetDlgItem(IDC_D3D9DEVICE_CHECK)->EnableWindow(TRUE);
				m_cbD3D9RenderDevice.EnableWindow(IsDlgButtonChecked(IDC_D3D9DEVICE_CHECK));
			}

			UpdateResizerList(rs.iResizer);
			UpdateDownscalerList(rs.iDownscaler);

			//GetDlgItem(IDC_STATIC6)->EnableWindow(TRUE);
			//m_cbDX9PresentMode.EnableWindow(TRUE);
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

			m_wndToolTip.UpdateTipText(ResStr(IDC_DSEVR_CUSTOM), &m_cbVideoRenderer);
			break;
		case VIDRNDT_SYNC:
			UpdateResizerList(GetCurItemData(m_cbDX9Resizer));

			GetDlgItem(IDC_STATIC5)->EnableWindow(TRUE);
			GetDlgItem(IDC_EVR_BUFFERS)->EnableWindow(TRUE);
			m_spnEvrBuffers.EnableWindow(TRUE);
			GetDlgItem(IDC_STATIC3)->EnableWindow(TRUE);
			m_cbDX9Resizer.EnableWindow(TRUE);
			m_chkD3DFullscreen.EnableWindow(TRUE);
			m_chk10bitOutput.EnableWindow(m_chkD3DFullscreen.GetCheck() == BST_CHECKED);
			GetDlgItem(IDC_RESETDEVICE)->EnableWindow(TRUE);

			GetDlgItem(IDC_STATIC4)->EnableWindow(TRUE);
			m_cbEVROutputRange.EnableWindow(TRUE);

			m_wndToolTip.UpdateTipText(ResStr(IDC_DSSYNC), &m_cbVideoRenderer);
			break;
		case VIDRNDT_DXR:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSDXR), &m_cbVideoRenderer);
			break;
		case VIDRNDT_NULL_COMP:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSNULL_COMP), &m_cbVideoRenderer);
			break;
		case VIDRNDT_NULL_UNCOMP:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSNULL_UNCOMP), &m_cbVideoRenderer);
			break;
		case VIDRNDT_MADVR:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSMADVR), &m_cbVideoRenderer);
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

	m_cbEVROutputRange.SetCurSel(0);
	m_chkVMRMixerMode.SetCheck(BST_CHECKED);
	m_chkVMRMixerYUV.SetCheck(BST_CHECKED);

	m_chkD3DFullscreen.SetCheck(BST_UNCHECKED);
	m_chk10bitOutput.SetCheck(BST_UNCHECKED);

	SelectByItemData(m_cbDX9PresentMode, 0);
	SelectByItemData(m_cbDX9SurfaceFormat, D3DFMT_X8R8G8B8);
	UpdateResizerList(RESIZER_SHADER_CATMULL);
	UpdateDownscalerList(DOWNSCALER_SIMPLE);

	OnFullscreenCheck();
	OnSurfaceFormatChange();

	m_cbFrameMode.SetCurSel(ID_VIEW_VF_FROMINSIDE - ID_VIEW_VF_HALF);
	m_chkNoSmallUpscale.SetCheck(BST_UNCHECKED);
	m_chkNoSmallUpscale.EnableWindow(TRUE);
	m_chkNoSmallDownscale.SetCheck(BST_UNCHECKED);
	m_chkNoSmallDownscale.EnableWindow(TRUE);

	Invalidate();
	SetModified();
}

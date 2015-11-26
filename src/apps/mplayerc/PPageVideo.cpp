/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2015 see Authors.txt
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
#include "../../DSUtil/SysVersion.h"
#include "MultiMonitor.h"
#include "PPageVideo.h"

// CPPageVideo dialog

static bool IsRenderTypeAvailable(UINT VideoRendererType, HWND hwnd)
{
	switch (VideoRendererType) {
		case VIDRNDT_DS_EVR:
		case VIDRNDT_DS_EVR_CUSTOM:
		case VIDRNDT_DS_SYNC:
			return IsCLSIDRegistered(CLSID_EnhancedVideoRenderer);
		case VIDRNDT_DS_DXR:
			return IsCLSIDRegistered(CLSID_DXR);
		case VIDRNDT_DS_MADVR:
			return IsCLSIDRegistered(CLSID_madVR);
		case VIDRNDT_DS_VMR7RENDERLESS:
			#ifdef _WIN64
			return false;
			#endif
			{
				MONITORINFO mi;
				mi.cbSize = sizeof(mi);
				if (GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi)) {
					CRect rc(mi.rcMonitor);
					return rc.Width() < 2048 && rc.Height() < 2048;
				}
			}
		default:
		return true;
	}
}

IMPLEMENT_DYNAMIC(CPPageVideo, CPPageBase)
CPPageVideo::CPPageVideo()
	: CPPageBase(CPPageVideo::IDD, CPPageVideo::IDD)
	, m_iVideoRendererType(VIDRNDT_DS_DEFAULT)
	, m_iVideoRendererType_store(VIDRNDT_DS_DEFAULT)
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
	DDX_Control(pDX, IDC_DX_SURFACE, m_cbAPSurfaceUsage);
	DDX_Control(pDX, IDC_COMBO1, m_cbDX9SurfaceFormat);
	DDX_Control(pDX, IDC_DX9RESIZER_COMBO, m_cbDX9Resizer);
	DDX_CBIndex(pDX, IDC_D3D9DEVICE_COMBO, m_iD3D9RenderDevice);
	DDX_Check(pDX, IDC_D3D9DEVICE, m_bD3D9RenderDevice);
	DDX_Check(pDX, IDC_RESETDEVICE, m_bResetDevice);
	DDX_Control(pDX, IDC_FULLSCREEN_MONITOR_CHECK, m_chkD3DFullscreen);
	DDX_Control(pDX, IDC_CHECK1, m_chk10bitOutput);
	DDX_Control(pDX, IDC_DSVMRLOADMIXER, m_chkVMRMixerMode);
	DDX_Control(pDX, IDC_DSVMRYUVMIXER, m_chkVMRMixerYUV);
	DDX_Control(pDX, IDC_COMBO2, m_cbEVROutputRange);
	DDX_Text(pDX, IDC_EVR_BUFFERS, m_iEvrBuffers);
	DDX_Control(pDX, IDC_SPIN1, m_spnEvrBuffers);

	DDX_Control(pDX, IDC_CHECK3, m_chkColorManagment);
	DDX_Control(pDX, IDC_COMBO3, m_cbCMInputType);
	DDX_Control(pDX, IDC_COMBO4, m_cbCMAmbientLight);
	DDX_Control(pDX, IDC_COMBO5, m_cbCMRenderingIntent);

	m_iEvrBuffers = min(max(RS_EVRBUFFERS_MIN, m_iEvrBuffers), RS_EVRBUFFERS_MAX);
}

BEGIN_MESSAGE_MAP(CPPageVideo, CPPageBase)
	ON_CBN_SELCHANGE(IDC_VIDRND_COMBO, OnDSRendererChange)
	ON_CBN_SELCHANGE(IDC_DX_SURFACE, OnSurfaceChange)
	ON_BN_CLICKED(IDC_D3D9DEVICE, OnD3D9DeviceCheck)
	ON_BN_CLICKED(IDC_RESETDEVICE, OnResetDevice)
	ON_BN_CLICKED(IDC_FULLSCREEN_MONITOR_CHECK, OnFullscreenCheck)
	ON_UPDATE_COMMAND_UI(IDC_DSVMRYUVMIXER, OnUpdateMixerYUV)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnSurfaceFormatChange)
	ON_BN_CLICKED(IDC_CHECK3, OnColorManagmentCheck)
	ON_BN_CLICKED(IDC_BUTTON3, OnBnClickedDefault)
END_MESSAGE_MAP()

// CPPageVideo message handlers

BOOL CPPageVideo::OnInitDialog()
{
	__super::OnInitDialog();

	SetCursor(m_hWnd, IDC_VIDRND_COMBO, IDC_HAND);
	SetCursor(m_hWnd, IDC_D3D9DEVICE_COMBO, IDC_HAND);
	SetCursor(m_hWnd, IDC_DX_SURFACE, IDC_HAND);
	SetCursor(m_hWnd, IDC_DX9RESIZER_COMBO, IDC_HAND);
	SetCursor(m_hWnd, IDC_COMBO1, IDC_HAND);
	SetCursor(m_hWnd, IDC_COMBO2, IDC_HAND);
	SetCursor(m_hWnd, IDC_COMBO3, IDC_HAND);
	SetCursor(m_hWnd, IDC_COMBO4, IDC_HAND);
	SetCursor(m_hWnd, IDC_COMBO5, IDC_HAND);

	CAppSettings& s = AfxGetAppSettings();

	CRenderersSettings& rs = s.m_RenderersSettings;
	m_iVideoRendererType   = s.iDSVideoRendererType;

	m_chkD3DFullscreen.SetCheck(s.fD3DFullscreen);
	m_chk10bitOutput.EnableWindow(s.fD3DFullscreen);
	m_chk10bitOutput.SetCheck(rs.m_AdvRendSets.b10BitOutput);
	m_chkVMRMixerMode.SetCheck(rs.fVMRMixerMode);
	m_chkVMRMixerYUV.SetCheck(rs.fVMRMixerYUV);

	m_cbAPSurfaceUsage.AddString(ResStr(IDS_PPAGE_OUTPUT_SURF_OFFSCREEN));
	m_cbAPSurfaceUsage.AddString(ResStr(IDS_PPAGE_OUTPUT_SURF_2D));
	m_cbAPSurfaceUsage.AddString(ResStr(IDS_PPAGE_OUTPUT_SURF_3D));
	m_cbAPSurfaceUsage.SetCurSel(rs.iAPSurfaceType);

	m_cbEVROutputRange.AddString(L"0-255");
	m_cbEVROutputRange.AddString(L"16-235");
	m_cbEVROutputRange.SetCurSel(rs.m_AdvRendSets.iEVROutputRange);

	m_iEvrBuffers = rs.iEvrBuffers;
	m_spnEvrBuffers.SetRange(RS_EVRBUFFERS_MIN, RS_EVRBUFFERS_MAX);

	m_bResetDevice = s.m_RenderersSettings.fResetDevice;

	IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (pD3D) {
		TCHAR strGUID[50] = { 0 };
		CString cstrGUID;
		CString d3ddevice_str;
		D3DADAPTER_IDENTIFIER9 adapterIdentifier = { 0 };

		for (UINT adp = 0; adp < pD3D->GetAdapterCount(); ++adp) {
			if (SUCCEEDED(pD3D->GetAdapterIdentifier(adp, 0, &adapterIdentifier))) {
				d3ddevice_str = adapterIdentifier.Description;
				d3ddevice_str += _T(" - ");
				d3ddevice_str += adapterIdentifier.DeviceName;
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
						if (rs.D3D9RenderDevice == cstrGUID) {
							m_iD3D9RenderDevice = m_cbD3D9RenderDevice.GetCount() - 1;
						}
					}
				}
			}
		}
		pD3D->Release();
	}

	CorrectComboListWidth(m_cbD3D9RenderDevice);

	auto addRenderer = [&](int nID) {
		CString sName;

		switch (nID) {
			case VIDRNDT_DS_DEFAULT:
				sName = ResStr(IDS_PPAGE_OUTPUT_SYS_DEF);
				break;
			case VIDRNDT_DS_OVERLAYMIXER:
				if (!IsWinXP()) {
					return;
				}
				sName = ResStr(IDS_PPAGE_OUTPUT_OVERLAYMIXER);
				break;
			case VIDRNDT_DS_VMR7WINDOWED:
				sName = ResStr(IDS_PPAGE_OUTPUT_VMR7WINDOWED);
				break;
			case VIDRNDT_DS_VMR9WINDOWED:
				sName = ResStr(IDS_PPAGE_OUTPUT_VMR9WINDOWED);
				break;
			case VIDRNDT_DS_VMR7RENDERLESS:
				sName = ResStr(IDS_PPAGE_OUTPUT_VMR7RENDERLESS);
				break;
			case VIDRNDT_DS_VMR9RENDERLESS:
				sName = ResStr(IDS_PPAGE_OUTPUT_VMR9RENDERLESS);
				break;
			case VIDRNDT_DS_DXR:
				sName = ResStr(IDS_PPAGE_OUTPUT_DXR);
				break;
			case VIDRNDT_DS_NULL_COMP:
				sName = ResStr(IDS_PPAGE_OUTPUT_NULL_COMP);
				break;
			case VIDRNDT_DS_NULL_UNCOMP:
				sName = ResStr(IDS_PPAGE_OUTPUT_NULL_UNCOMP);
				break;
			case VIDRNDT_DS_EVR:
				sName = ResStr(IDS_PPAGE_OUTPUT_EVR);
				break;
			case VIDRNDT_DS_EVR_CUSTOM:
				sName = ResStr(IDS_PPAGE_OUTPUT_EVR_CUSTOM);
				break;
			case VIDRNDT_DS_MADVR:
				sName = ResStr(IDS_PPAGE_OUTPUT_MADVR);
				break;
			case VIDRNDT_DS_SYNC:
				sName = ResStr(IDS_PPAGE_OUTPUT_SYNC);
				break;
			default:
				ASSERT(FALSE);
				return;
		}

		if (!IsRenderTypeAvailable(nID, m_hWnd)) {
			sName.AppendFormat(L" %s", ResStr(IDS_REND_NOT_AVAILABLE));
		}

		m_cbVideoRenderer.SetItemData(m_cbVideoRenderer.AddString(sName), nID);
	};


	CComboBox& m_iDSVRTC = m_cbVideoRenderer;
	m_iDSVRTC.SetRedraw(FALSE);
	addRenderer(VIDRNDT_DS_DEFAULT);
	addRenderer(VIDRNDT_DS_OVERLAYMIXER);
	addRenderer(VIDRNDT_DS_VMR7WINDOWED);
	addRenderer(VIDRNDT_DS_VMR9WINDOWED);
	addRenderer(VIDRNDT_DS_VMR7RENDERLESS);
	addRenderer(VIDRNDT_DS_VMR9RENDERLESS);
	addRenderer(VIDRNDT_DS_EVR);
	addRenderer(VIDRNDT_DS_EVR_CUSTOM);
	addRenderer(VIDRNDT_DS_SYNC);
	addRenderer(VIDRNDT_DS_DXR);
	addRenderer(VIDRNDT_DS_MADVR);
	addRenderer(VIDRNDT_DS_NULL_COMP);
	addRenderer(VIDRNDT_DS_NULL_UNCOMP);

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
	m_wndToolTip.AddTool(&m_cbAPSurfaceUsage, L"");

	OnDSRendererChange();
	UpdateSurfaceFormatList(rs.m_AdvRendSets.iDX9SurfaceFormat);
	OnSurfaceChange();
	UpdateResizerList(rs.iDX9Resizer);

	CheckDlgButton(IDC_D3D9DEVICE, BST_UNCHECKED);
	GetDlgItem(IDC_D3D9DEVICE)->EnableWindow(FALSE);
	m_cbD3D9RenderDevice.EnableWindow(FALSE);

	switch (m_iVideoRendererType) {
		case VIDRNDT_DS_VMR9RENDERLESS:
		case VIDRNDT_DS_EVR_CUSTOM:
			if (m_cbD3D9RenderDevice.GetCount() > 1) {
					GetDlgItem(IDC_D3D9DEVICE)->EnableWindow(TRUE);
					m_cbD3D9RenderDevice.EnableWindow(FALSE);
					CheckDlgButton(IDC_D3D9DEVICE, BST_UNCHECKED);
					if (m_iD3D9RenderDevice != -1) {
						CheckDlgButton(IDC_D3D9DEVICE, BST_CHECKED);
						m_cbD3D9RenderDevice.EnableWindow(TRUE);
					}
			}
			break;
		default:
			GetDlgItem(IDC_D3D9DEVICE)->EnableWindow(FALSE);
			m_cbD3D9RenderDevice.EnableWindow(FALSE);
	}

	// Color Managment
	CorrectCWndWidth(&m_chkColorManagment);
	m_chkColorManagment.SetCheck(rs.m_AdvRendSets.iVMR9ColorManagementEnable != 0);
	m_cbCMInputType.AddString(ResStr(IDS_CM_INPUT_AUTO));
	m_cbCMInputType.AddString(L"HDTV");
	m_cbCMInputType.AddString(L"SDTV NTSC");
	m_cbCMInputType.AddString(L"SDTV PAL");
	m_cbCMInputType.SetCurSel(rs.m_AdvRendSets.iVMR9ColorManagementInput);
	m_cbCMAmbientLight.AddString(ResStr(IDS_CM_AMBIENTLIGHT_BRIGHT));
	m_cbCMAmbientLight.AddString(ResStr(IDS_CM_AMBIENTLIGHT_DIM));
	m_cbCMAmbientLight.AddString(ResStr(IDS_CM_AMBIENTLIGHT_DARK));
	m_cbCMAmbientLight.SetCurSel(rs.m_AdvRendSets.iVMR9ColorManagementAmbientLight);
	m_cbCMRenderingIntent.AddString(ResStr(IDS_CM_INTENT_PERCEPTUAL));
	m_cbCMRenderingIntent.AddString(ResStr(IDS_CM_INTENT_RELATIVECM));
	m_cbCMRenderingIntent.AddString(ResStr(IDS_CM_INTENT_SATURATION));
	m_cbCMRenderingIntent.AddString(ResStr(IDS_CM_INTENT_ABSOLUTECM));
	m_cbCMRenderingIntent.SetCurSel(rs.m_AdvRendSets.iVMR9ColorManagementIntent);
	CorrectComboListWidth(m_cbCMInputType);
	CorrectComboListWidth(m_cbCMAmbientLight);
	CorrectComboListWidth(m_cbCMRenderingIntent);

	OnSurfaceFormatChange();
	OnColorManagmentCheck();

	UpdateData(TRUE);
	SetModified(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
}

BOOL CPPageVideo::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();
	CRenderersSettings& rs = s.m_RenderersSettings;

	s.iDSVideoRendererType	= m_iVideoRendererType = m_iVideoRendererType_store = m_cbVideoRenderer.GetItemData(m_cbVideoRenderer.GetCurSel());
	rs.iAPSurfaceType		= m_cbAPSurfaceUsage.GetCurSel();
	rs.iDX9Resizer			= (int)m_cbDX9Resizer.GetItemData(m_cbDX9Resizer.GetCurSel());
	rs.fVMRMixerMode		= !!m_chkVMRMixerMode.GetCheck();
	rs.fVMRMixerYUV			= !!m_chkVMRMixerYUV.GetCheck();
	s.fD3DFullscreen		= !!m_chkD3DFullscreen.GetCheck();
	rs.fResetDevice			= !!m_bResetDevice;

	rs.m_AdvRendSets.iDX9SurfaceFormat		= m_cbDX9SurfaceFormat.GetItemData(m_cbDX9SurfaceFormat.GetCurSel());
	rs.m_AdvRendSets.b10BitOutput			= !!m_chk10bitOutput.GetCheck();
	rs.m_AdvRendSets.iEVROutputRange		= m_cbEVROutputRange.GetCurSel();

	rs.iEvrBuffers = m_iEvrBuffers;

	rs.D3D9RenderDevice = m_bD3D9RenderDevice ? m_D3D9GUIDNames[m_iD3D9RenderDevice] : L"";

	// Color Managment
	rs.m_AdvRendSets.iVMR9ColorManagementEnable = !!m_chkColorManagment.GetCheck();
	rs.m_AdvRendSets.iVMR9ColorManagementInput = m_cbCMInputType.GetCurSel();
	rs.m_AdvRendSets.iVMR9ColorManagementAmbientLight = m_cbCMAmbientLight.GetCurSel();
	rs.m_AdvRendSets.iVMR9ColorManagementIntent = m_cbCMRenderingIntent.GetCurSel();

	return __super::OnApply();
}

void CPPageVideo::UpdateSurfaceFormatList(int select)
{
	for (int i = m_cbDX9SurfaceFormat.GetCount() - 1; i >= 0; i--) {
		m_cbDX9SurfaceFormat.DeleteString(i);
	}

	int videoRenderer = m_cbVideoRenderer.GetItemData(m_cbVideoRenderer.GetCurSel());

	m_cbDX9SurfaceFormat.SetItemData(m_cbDX9SurfaceFormat.AddString(L"8-bit Integer Surfaces"), D3DFMT_X8R8G8B8);

	if ((videoRenderer == VIDRNDT_DS_VMR9RENDERLESS || videoRenderer == VIDRNDT_DS_EVR_CUSTOM) && m_cbAPSurfaceUsage.GetCurSel() == VIDRNDT_AP_TEXTURE3D) {
		m_cbDX9SurfaceFormat.SetItemData(m_cbDX9SurfaceFormat.AddString(L"10-bit Integer Surfaces"), D3DFMT_A2R10G10B10);
		m_cbDX9SurfaceFormat.SetItemData(m_cbDX9SurfaceFormat.AddString(L"16-bit Floating Point Surfaces"), D3DFMT_A16B16G16R16F);
		m_cbDX9SurfaceFormat.SetItemData(m_cbDX9SurfaceFormat.AddString(L"32-bit Floating Point Surfaces"), D3DFMT_A32B32G32R32F);
	}

	m_cbDX9SurfaceFormat.SetCurSel(0); // default
	for (int i = 0; i < m_cbDX9SurfaceFormat.GetCount(); i++) {
		if ((int)m_cbDX9SurfaceFormat.GetItemData(i) == select) {
			m_cbDX9SurfaceFormat.SetCurSel(i);
			break;
		}
	}
}

void CPPageVideo::UpdateResizerList(int select)
{
	for (int i = m_cbDX9Resizer.GetCount() - 1; i >= 0; i--) {
		m_cbDX9Resizer.DeleteString(i);
	}

	m_cbDX9Resizer.SetItemData(m_cbDX9Resizer.AddString(L"Nearest neighbor"), RESIZER_NEAREST);
	m_cbDX9Resizer.SetItemData(m_cbDX9Resizer.AddString(L"Bilinear"), RESIZER_BILINEAR);

#if DXVAVP
	if ((D3DFORMAT)m_cbDX9SurfaceFormat.GetItemData(m_cbDX9SurfaceFormat.GetCurSel()) == D3DFMT_X8R8G8B8) {
		m_cbDX9Resizer.SetItemData(m_cbDX9Resizer.AddString(L"DXVA2"), RESIZER_DXVA2);
	}
#endif

	if (m_cbAPSurfaceUsage.GetCurSel() == VIDRNDT_AP_TEXTURE3D) {
		m_cbDX9Resizer.SetItemData(m_cbDX9Resizer.AddString(L"Perlin Smootherstep (PS 2.0)"), RESIZER_SHADER_SMOOTHERSTEP);
		m_cbDX9Resizer.SetItemData(m_cbDX9Resizer.AddString(L"B-spline4 (PS 2.0)"), RESIZER_SHADER_BSPLINE4);
		m_cbDX9Resizer.SetItemData(m_cbDX9Resizer.AddString(L"Mitchell-Netravali spline4 (PS 2.0)"), RESIZER_SHADER_MITCHELL4);
		m_cbDX9Resizer.SetItemData(m_cbDX9Resizer.AddString(L"Catmull-Rom spline4 (PS 2.0)"), RESIZER_SHADER_CATMULL4);
		m_cbDX9Resizer.SetItemData(m_cbDX9Resizer.AddString(L"Bicubic A=-0.6 (PS 2.0)"), RESIZER_SHADER_BICUBIC06);
		m_cbDX9Resizer.SetItemData(m_cbDX9Resizer.AddString(L"Bicubic A=-0.8 (PS 2.0)"), RESIZER_SHADER_BICUBIC08);
		m_cbDX9Resizer.SetItemData(m_cbDX9Resizer.AddString(L"Bicubic A=-1.0 (PS 2.0)"), RESIZER_SHADER_BICUBIC10);
#if ENABLE_2PASS_RESIZE
		m_cbDX9Resizer.SetItemData(m_cbDX9Resizer.AddString(L"Lanczos2 (PS 2.0)"), RESIZER_SHADER_LANCZOS2);
		m_cbDX9Resizer.SetItemData(m_cbDX9Resizer.AddString(L"Lanczos3 (PS 2.x)"), RESIZER_SHADER_LANCZOS3);
#endif
	}

	m_cbDX9Resizer.SetCurSel(1); // default
	for (int i = 0; i < m_cbDX9Resizer.GetCount(); i++) {
		if ((int)m_cbDX9Resizer.GetItemData(i) == select) {
			m_cbDX9Resizer.SetCurSel(i);
			break;
		}
	}
}

void CPPageVideo::OnUpdateMixerYUV(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!!IsDlgButtonChecked(IDC_DSVMRLOADMIXER)
					&& (m_cbVideoRenderer.GetItemData(m_cbVideoRenderer.GetCurSel()) == VIDRNDT_DS_VMR7WINDOWED
						|| m_cbVideoRenderer.GetItemData(m_cbVideoRenderer.GetCurSel()) == VIDRNDT_DS_VMR9WINDOWED
						|| m_cbVideoRenderer.GetItemData(m_cbVideoRenderer.GetCurSel()) == VIDRNDT_DS_VMR7RENDERLESS
						|| m_cbVideoRenderer.GetItemData(m_cbVideoRenderer.GetCurSel()) == VIDRNDT_DS_VMR9RENDERLESS));
}

void CPPageVideo::OnSurfaceChange()
{
	UpdateData();

	switch (m_cbAPSurfaceUsage.GetCurSel()) {
		case VIDRNDT_AP_SURFACE:
			m_wndToolTip.UpdateTipText(ResStr(IDC_REGULARSURF), &m_cbAPSurfaceUsage);
			break;
		case VIDRNDT_AP_TEXTURE2D:
			m_wndToolTip.UpdateTipText(ResStr(IDC_TEXTURESURF2D), &m_cbAPSurfaceUsage);
			break;
		case VIDRNDT_AP_TEXTURE3D:
			m_wndToolTip.UpdateTipText(ResStr(IDC_TEXTURESURF3D), &m_cbAPSurfaceUsage);
			break;
	}

	UpdateSurfaceFormatList((int)m_cbDX9SurfaceFormat.GetItemData(m_cbDX9SurfaceFormat.GetCurSel()));
	UpdateResizerList((int)m_cbDX9Resizer.GetItemData(m_cbDX9Resizer.GetCurSel()));

	SetModified();
}

void CPPageVideo::OnDSRendererChange()
{
	UINT CurrentVR = m_cbVideoRenderer.GetItemData(m_cbVideoRenderer.GetCurSel());

	if (!IsRenderTypeAvailable(CurrentVR, m_hWnd)) {
		AfxMessageBox(IDS_PPAGE_OUTPUT_UNAVAILABLEMSG, MB_ICONEXCLAMATION | MB_OK, 0);

		// revert to the last saved renderer
		m_cbVideoRenderer.SetCurSel(0);
		for (int i = 0; i < m_cbVideoRenderer.GetCount(); ++i) {
			if (m_iVideoRendererType_store == m_cbVideoRenderer.GetItemData(i)) {
				m_cbVideoRenderer.SetCurSel(i);
				CurrentVR = m_cbVideoRenderer.GetItemData(m_cbVideoRenderer.GetCurSel());
				break;
			}
		}
	}

	m_cbAPSurfaceUsage.EnableWindow(FALSE);
	m_cbDX9SurfaceFormat.EnableWindow(FALSE);
	m_cbDX9Resizer.EnableWindow(FALSE);
	GetDlgItem(IDC_FULLSCREEN_MONITOR_CHECK)->EnableWindow(FALSE);
	GetDlgItem(IDC_CHECK1)->EnableWindow(FALSE);
	m_chkVMRMixerMode.EnableWindow(FALSE);
	m_chkVMRMixerYUV.EnableWindow(FALSE);
	GetDlgItem(IDC_RESETDEVICE)->EnableWindow(FALSE);
	GetDlgItem(IDC_EVR_BUFFERS)->EnableWindow(FALSE);
	m_spnEvrBuffers.EnableWindow(FALSE);
	GetDlgItem(IDC_D3D9DEVICE)->EnableWindow(FALSE);
	m_cbD3D9RenderDevice.EnableWindow(FALSE);
	m_cbEVROutputRange.EnableWindow(FALSE);
	GetDlgItem(IDC_STATIC1)->EnableWindow(FALSE);
	GetDlgItem(IDC_STATIC2)->EnableWindow(FALSE);
	GetDlgItem(IDC_STATIC3)->EnableWindow(FALSE);
	GetDlgItem(IDC_STATIC4)->EnableWindow(FALSE);
	GetDlgItem(IDC_STATIC5)->EnableWindow(FALSE);

	// Color Managment
	m_chkColorManagment.ShowWindow(SW_HIDE);
	m_cbCMInputType.ShowWindow(SW_HIDE);
	m_cbCMAmbientLight.ShowWindow(SW_HIDE);
	m_cbCMRenderingIntent.ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC6)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC7)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC8)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC9)->ShowWindow(SW_HIDE);

	switch (CurrentVR) {
		case VIDRNDT_DS_DEFAULT:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSSYSDEF), &m_cbVideoRenderer);
			break;
		case VIDRNDT_DS_OVERLAYMIXER:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSOVERLAYMIXER), &m_cbVideoRenderer);
			break;
		case VIDRNDT_DS_VMR7WINDOWED:
			m_chkVMRMixerMode.EnableWindow(TRUE);
			m_chkVMRMixerYUV.EnableWindow(TRUE);

			m_wndToolTip.UpdateTipText(ResStr(IDC_DSVMR7WIN), &m_cbVideoRenderer);
			break;
		case VIDRNDT_DS_VMR9WINDOWED:
			m_chkVMRMixerMode.EnableWindow(TRUE);
			m_chkVMRMixerYUV.EnableWindow(TRUE);

			m_wndToolTip.UpdateTipText(ResStr(IDC_DSVMR9WIN), &m_cbVideoRenderer);
			break;
		case VIDRNDT_DS_VMR7RENDERLESS:
			GetDlgItem(IDC_STATIC1)->EnableWindow(TRUE);
			m_cbAPSurfaceUsage.EnableWindow(TRUE);
			m_chkVMRMixerMode.EnableWindow(TRUE);
			m_chkVMRMixerYUV.EnableWindow(TRUE);

			m_wndToolTip.UpdateTipText(ResStr(IDC_DSVMR7REN), &m_cbVideoRenderer);
			break;
		case VIDRNDT_DS_VMR9RENDERLESS:
			if (m_cbD3D9RenderDevice.GetCount() > 1) {
				GetDlgItem(IDC_D3D9DEVICE)->EnableWindow(TRUE);
				m_cbD3D9RenderDevice.EnableWindow(IsDlgButtonChecked(IDC_D3D9DEVICE));
			}
			m_chkVMRMixerMode.EnableWindow(TRUE);
			m_chkVMRMixerYUV.EnableWindow(TRUE);
			GetDlgItem(IDC_RESETDEVICE)->EnableWindow(TRUE);
			GetDlgItem(IDC_STATIC1)->EnableWindow(TRUE);
			m_cbAPSurfaceUsage.EnableWindow(TRUE);
			GetDlgItem(IDC_STATIC2)->EnableWindow(TRUE);
			m_cbDX9SurfaceFormat.EnableWindow(TRUE);
			GetDlgItem(IDC_STATIC3)->EnableWindow(TRUE);
			m_cbDX9Resizer.EnableWindow(TRUE);
			GetDlgItem(IDC_FULLSCREEN_MONITOR_CHECK)->EnableWindow(TRUE);
			GetDlgItem(IDC_CHECK1)->EnableWindow(m_chkD3DFullscreen.GetCheck() == BST_CHECKED);
			GetDlgItem(IDC_RESETDEVICE)->EnableWindow(TRUE);
			if (m_cbAPSurfaceUsage.GetCurSel() == VIDRNDT_AP_TEXTURE3D) {
				D3DFORMAT surfmt = (D3DFORMAT)m_cbDX9SurfaceFormat.GetItemData(m_cbDX9SurfaceFormat.GetCurSel());
				if (surfmt == D3DFMT_A16B16G16R16F || surfmt == D3DFMT_A32B32G32R32F) {
					// Color Managment
					m_chkColorManagment.ShowWindow(SW_SHOW);
					m_cbCMInputType.ShowWindow(SW_SHOW);
					m_cbCMAmbientLight.ShowWindow(SW_SHOW);
					m_cbCMRenderingIntent.ShowWindow(SW_SHOW);
					GetDlgItem(IDC_STATIC6)->ShowWindow(SW_SHOW);
					GetDlgItem(IDC_STATIC7)->ShowWindow(SW_SHOW);
					GetDlgItem(IDC_STATIC8)->ShowWindow(SW_SHOW);
					GetDlgItem(IDC_STATIC9)->ShowWindow(SW_SHOW);
				}
			}

			m_wndToolTip.UpdateTipText(ResStr(IDC_DSVMR9REN), &m_cbVideoRenderer);
			break;
		case VIDRNDT_DS_EVR:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSEVR), &m_cbVideoRenderer);
			break;
		case VIDRNDT_DS_EVR_CUSTOM:
			if (m_cbD3D9RenderDevice.GetCount() > 1) {
				GetDlgItem(IDC_D3D9DEVICE)->EnableWindow(TRUE);
				m_cbD3D9RenderDevice.EnableWindow(IsDlgButtonChecked(IDC_D3D9DEVICE));
			}
			GetDlgItem(IDC_STATIC2)->EnableWindow(TRUE);
			m_cbDX9SurfaceFormat.EnableWindow(TRUE);
			GetDlgItem(IDC_STATIC3)->EnableWindow(TRUE);
			m_cbDX9Resizer.EnableWindow(TRUE);
			GetDlgItem(IDC_FULLSCREEN_MONITOR_CHECK)->EnableWindow(TRUE);
			GetDlgItem(IDC_CHECK1)->EnableWindow(m_chkD3DFullscreen.GetCheck() == BST_CHECKED);
			GetDlgItem(IDC_RESETDEVICE)->EnableWindow(TRUE);
			GetDlgItem(IDC_EVR_BUFFERS)->EnableWindow(TRUE);
			GetDlgItem(IDC_STATIC5)->EnableWindow(TRUE);
			m_spnEvrBuffers.EnableWindow(TRUE);

			// Force 3D surface with EVR Custom
			m_cbAPSurfaceUsage.SetCurSel(2);
			OnSurfaceChange();

			GetDlgItem(IDC_STATIC4)->EnableWindow(TRUE);
			m_cbEVROutputRange.EnableWindow(TRUE);

			{
				D3DFORMAT surfmt = (D3DFORMAT)m_cbDX9SurfaceFormat.GetItemData(m_cbDX9SurfaceFormat.GetCurSel());
				if (surfmt == D3DFMT_A16B16G16R16F || surfmt == D3DFMT_A32B32G32R32F) {
					// Color Managment
					m_chkColorManagment.ShowWindow(SW_SHOW);
					m_cbCMInputType.ShowWindow(SW_SHOW);
					m_cbCMAmbientLight.ShowWindow(SW_SHOW);
					m_cbCMRenderingIntent.ShowWindow(SW_SHOW);
					GetDlgItem(IDC_STATIC6)->ShowWindow(SW_SHOW);
					GetDlgItem(IDC_STATIC7)->ShowWindow(SW_SHOW);
					GetDlgItem(IDC_STATIC8)->ShowWindow(SW_SHOW);
					GetDlgItem(IDC_STATIC9)->ShowWindow(SW_SHOW);
				}
			}

			m_wndToolTip.UpdateTipText(ResStr(IDC_DSEVR_CUSTOM), &m_cbVideoRenderer);
			break;
		case VIDRNDT_DS_SYNC:
			GetDlgItem(IDC_STATIC5)->EnableWindow(TRUE);
			GetDlgItem(IDC_EVR_BUFFERS)->EnableWindow(TRUE);
			m_spnEvrBuffers.EnableWindow(TRUE);
			GetDlgItem(IDC_STATIC3)->EnableWindow(TRUE);
			m_cbDX9Resizer.EnableWindow(TRUE);
			GetDlgItem(IDC_FULLSCREEN_MONITOR_CHECK)->EnableWindow(TRUE);
			GetDlgItem(IDC_CHECK1)->EnableWindow(m_chkD3DFullscreen.GetCheck() == BST_CHECKED);
			GetDlgItem(IDC_RESETDEVICE)->EnableWindow(TRUE);
			// Force 3D surface with EVR Sync
			m_cbAPSurfaceUsage.SetCurSel(2);
			OnSurfaceChange();

			GetDlgItem(IDC_STATIC4)->EnableWindow(TRUE);
			m_cbEVROutputRange.EnableWindow(TRUE);

			m_wndToolTip.UpdateTipText(ResStr(IDC_DSSYNC), &m_cbVideoRenderer);
			break;
		case VIDRNDT_DS_DXR:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSDXR), &m_cbVideoRenderer);
			break;
		case VIDRNDT_DS_NULL_COMP:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSNULL_COMP), &m_cbVideoRenderer);
			break;
		case VIDRNDT_DS_NULL_UNCOMP:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSNULL_UNCOMP), &m_cbVideoRenderer);
			break;
		case VIDRNDT_DS_MADVR:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSMADVR), &m_cbVideoRenderer);
			break;
		default:
			m_wndToolTip.UpdateTipText(L"", &m_cbVideoRenderer);
	}

	AfxGetAppSettings().iSelectedDSVideoRendererType = CurrentVR;

	SetModified();
}

void CPPageVideo::OnResetDevice()
{
	SetModified();
}

void CPPageVideo::OnFullscreenCheck()
{
	if (m_chkD3DFullscreen.GetCheck() == BST_CHECKED && (MessageBox(ResStr(IDS_D3DFS_WARNING), NULL, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES)) {
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
	D3DFORMAT surfmt = (D3DFORMAT)m_cbDX9SurfaceFormat.GetItemData(m_cbDX9SurfaceFormat.GetCurSel());
	UINT CurrentVR = m_cbVideoRenderer.GetItemData(m_cbVideoRenderer.GetCurSel());

	if ((CurrentVR == VIDRNDT_DS_VMR9RENDERLESS || CurrentVR == VIDRNDT_DS_EVR_CUSTOM) && (surfmt == D3DFMT_A16B16G16R16F || surfmt == D3DFMT_A32B32G32R32F)) {
		// Color Managment
		m_chkColorManagment.ShowWindow(SW_SHOW);
		m_cbCMInputType.ShowWindow(SW_SHOW);
		m_cbCMAmbientLight.ShowWindow(SW_SHOW);
		m_cbCMRenderingIntent.ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATIC6)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATIC7)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATIC8)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATIC9)->ShowWindow(SW_SHOW);
	}
	else {
		// Color Managment
		m_chkColorManagment.ShowWindow(SW_HIDE);
		m_cbCMInputType.ShowWindow(SW_HIDE);
		m_cbCMAmbientLight.ShowWindow(SW_HIDE);
		m_cbCMRenderingIntent.ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STATIC6)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STATIC7)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STATIC8)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STATIC9)->ShowWindow(SW_HIDE);
	}

	UpdateResizerList((int)m_cbDX9Resizer.GetItemData(m_cbDX9Resizer.GetCurSel()));

	SetModified();
}

void CPPageVideo::OnColorManagmentCheck()
{
	if (m_chkColorManagment.GetCheck() == BST_CHECKED) {
		m_cbCMInputType.EnableWindow(TRUE);
		m_cbCMAmbientLight.EnableWindow(TRUE);
		m_cbCMRenderingIntent.EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC7)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC8)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC9)->EnableWindow(TRUE);
	}
	else {
		m_cbCMInputType.EnableWindow(FALSE);
		m_cbCMAmbientLight.EnableWindow(FALSE);
		m_cbCMRenderingIntent.EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC7)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC8)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC9)->EnableWindow(FALSE);
	}

	SetModified();
}

void CPPageVideo::OnBnClickedDefault()
{
	m_bD3D9RenderDevice = FALSE;
	m_bResetDevice = TRUE;
	m_iEvrBuffers = 5;

	UpdateData(FALSE);

	m_cbAPSurfaceUsage.SetCurSel(IsWinVistaOrLater() ? VIDRNDT_AP_TEXTURE3D : VIDRNDT_AP_TEXTURE2D);
	m_cbEVROutputRange.SetCurSel(0);
	m_chkVMRMixerMode.SetCheck(IsWinVistaOrLater() ? BST_CHECKED : BST_UNCHECKED);
	m_chkVMRMixerYUV.SetCheck(IsWinVistaOrLater() ? BST_CHECKED : BST_UNCHECKED);

	m_chkD3DFullscreen.SetCheck(BST_UNCHECKED);
	m_chk10bitOutput.SetCheck(BST_UNCHECKED);
	m_cbAPSurfaceUsage.SetCurSel(IsWinVistaOrLater() ? VIDRNDT_AP_TEXTURE3D : VIDRNDT_AP_TEXTURE2D);

	UpdateSurfaceFormatList(D3DFMT_X8R8G8B8);
	UpdateResizerList(RESIZER_BILINEAR);

	m_chkColorManagment.SetCheck(BST_UNCHECKED);
	m_cbCMInputType.SetCurSel(0);
	m_cbCMAmbientLight.SetCurSel(0);
	m_cbCMRenderingIntent.SetCurSel(0);

	OnFullscreenCheck();
	OnSurfaceChange();
	OnSurfaceFormatChange();
	OnColorManagmentCheck();

	SetModified();
}

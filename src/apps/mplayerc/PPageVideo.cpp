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
	, m_iAPSurfaceUsage(0)
	, m_bVMRMixerMode(FALSE)
	, m_bVMRMixerYUV(FALSE)
	, m_bVMR9AlterativeVSync(FALSE)
	, m_bResetDevice(FALSE)
	, m_iEvrBuffers(L"5")
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
	DDX_CBIndex(pDX, IDC_DX_SURFACE, m_iAPSurfaceUsage);
	DDX_Control(pDX, IDC_COMBO1, m_cbDX9SurfaceFormat);
	DDX_Control(pDX, IDC_DX9RESIZER_COMBO, m_cbDX9Resizer);
	DDX_CBIndex(pDX, IDC_D3D9DEVICE_COMBO, m_iD3D9RenderDevice);
	DDX_Check(pDX, IDC_D3D9DEVICE, m_bD3D9RenderDevice);
	DDX_Check(pDX, IDC_RESETDEVICE, m_bResetDevice);
	DDX_Control(pDX, IDC_FULLSCREEN_MONITOR_CHECK, m_chkD3DFullscreen);
	DDX_Control(pDX, IDC_CHECK1, m_chk10bitOutput);
	DDX_Check(pDX, IDC_DSVMR9ALTERNATIVEVSYNC, m_bVMR9AlterativeVSync);
	DDX_Check(pDX, IDC_DSVMRLOADMIXER, m_bVMRMixerMode);
	DDX_Check(pDX, IDC_DSVMRYUVMIXER, m_bVMRMixerYUV);
	DDX_CBString(pDX, IDC_EVR_BUFFERS, m_iEvrBuffers);
	DDX_Control(pDX, IDC_COMBO2, m_cbEVROutputRange);
	DDX_Control(pDX, IDC_CHECK2, m_chkDisableAero);
}

BEGIN_MESSAGE_MAP(CPPageVideo, CPPageBase)
	ON_CBN_SELCHANGE(IDC_VIDRND_COMBO, OnDSRendererChange)
	ON_CBN_SELCHANGE(IDC_DX_SURFACE, OnSurfaceChange)
	ON_BN_CLICKED(IDC_D3D9DEVICE, OnD3D9DeviceCheck)
	ON_BN_CLICKED(IDC_FULLSCREEN_MONITOR_CHECK, OnFullscreenCheck)
	ON_UPDATE_COMMAND_UI(IDC_DSVMRYUVMIXER, OnUpdateMixerYUV)
END_MESSAGE_MAP()

// CPPageVideo message handlers

BOOL CPPageVideo::OnInitDialog()
{
	__super::OnInitDialog();

	SetHandCursor(m_hWnd, IDC_VIDRND_COMBO);
	SetHandCursor(m_hWnd, IDC_D3D9DEVICE_COMBO);
	SetHandCursor(m_hWnd, IDC_DX_SURFACE);
	SetHandCursor(m_hWnd, IDC_DX9RESIZER_COMBO);
	SetHandCursor(m_hWnd, IDC_EVR_BUFFERS);
	SetHandCursor(m_hWnd, IDC_COMBO1);
	SetHandCursor(m_hWnd, IDC_COMBO2);
	SetHandCursor(m_hWnd, IDC_COMBO3);
	SetHandCursor(m_hWnd, IDC_COMBO4);
	SetHandCursor(m_hWnd, IDC_COMBO5);

	AppSettings& s = AfxGetAppSettings();

	CRenderersSettings& rs = s.m_RenderersSettings;
	m_iVideoRendererType	= s.iDSVideoRendererType;
	m_iAPSurfaceUsage		= rs.iAPSurfaceUsage;
	m_bVMRMixerMode			= rs.fVMRMixerMode;
	m_bVMRMixerYUV			= rs.fVMRMixerYUV;
	m_bVMR9AlterativeVSync	= rs.m_AdvRendSets.fVMR9AlterativeVSync;

	m_chkD3DFullscreen.SetCheck(s.fD3DFullscreen);
	m_chk10bitOutput.SetCheck(rs.m_AdvRendSets.b10BitOutput);
	m_chk10bitOutput.EnableWindow(s.fD3DFullscreen);

	m_chkDisableAero.SetCheck(rs.m_AdvRendSets.iVMRDisableDesktopComposition);

	m_cbEVROutputRange.AddString(L"0-255");
	m_cbEVROutputRange.AddString(L"16-235");
	m_cbEVROutputRange.SetCurSel(rs.m_AdvRendSets.iEVROutputRange);

	m_iEvrBuffers.Format(L"%d", rs.iEvrBuffers);

	UpdateSurfaceFormatList(rs.m_AdvRendSets.iDX9SurfaceFormat);
	UpdateResizerList(rs.iDX9Resizer);

	m_bResetDevice = s.m_RenderersSettings.fResetDevice;

	IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (pD3D) {
		TCHAR strGUID[50] = {0};
		CString cstrGUID;
		CString d3ddevice_str;
		CStringArray adapterList;

		D3DADAPTER_IDENTIFIER9 adapterIdentifier;

		for (UINT adp = 0; adp < pD3D->GetAdapterCount(); ++adp) {
			if (SUCCEEDED(pD3D->GetAdapterIdentifier(adp, 0, &adapterIdentifier))) {
				d3ddevice_str = adapterIdentifier.Description;
				d3ddevice_str += _T(" - ");
				d3ddevice_str += adapterIdentifier.DeviceName;
				cstrGUID.Empty();
				if (::StringFromGUID2(adapterIdentifier.DeviceIdentifier, strGUID, 50) > 0) {
					cstrGUID = strGUID;
				}
				if (cstrGUID.GetLength() > 0) {
					boolean m_find = false;
					for (INT_PTR i = 0; (!m_find) && (i < m_D3D9GUIDNames.GetCount()); i++) {
						if (m_D3D9GUIDNames.GetAt(i) == cstrGUID) {
							m_find = true;
						}
					}
					if (!m_find) {
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
	m_wndToolTip.AddTool(GetDlgItem(IDC_DX_SURFACE), L"");

	OnDSRendererChange();
	OnSurfaceChange();

	CheckDlgButton(IDC_D3D9DEVICE, BST_UNCHECKED);
	GetDlgItem(IDC_D3D9DEVICE)->EnableWindow(FALSE);
	GetDlgItem(IDC_D3D9DEVICE_COMBO)->EnableWindow(FALSE);

	switch (m_iVideoRendererType) {
		case VIDRNDT_DS_VMR9RENDERLESS:
		case VIDRNDT_DS_EVR_CUSTOM:
			if (m_cbD3D9RenderDevice.GetCount() > 1) {
					GetDlgItem(IDC_D3D9DEVICE)->EnableWindow(TRUE);
					GetDlgItem(IDC_D3D9DEVICE_COMBO)->EnableWindow(FALSE);
					CheckDlgButton(IDC_D3D9DEVICE, BST_UNCHECKED);
					if (m_iD3D9RenderDevice != -1) {
						CheckDlgButton(IDC_D3D9DEVICE, BST_CHECKED);
						GetDlgItem(IDC_D3D9DEVICE_COMBO)->EnableWindow(TRUE);
					}
			}
			break;
		default:
			GetDlgItem(IDC_D3D9DEVICE)->EnableWindow(FALSE);
			GetDlgItem(IDC_D3D9DEVICE_COMBO)->EnableWindow(FALSE);
 	}
	UpdateData(TRUE);

	return TRUE;  // return TRUE unless you set the focus to a control
}

BOOL CPPageVideo::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();
	CRenderersSettings& rs = s.m_RenderersSettings;

	s.iDSVideoRendererType	= m_iVideoRendererType = m_iVideoRendererType_store = m_cbVideoRenderer.GetItemData(m_cbVideoRenderer.GetCurSel());
	rs.iAPSurfaceUsage		= m_iAPSurfaceUsage;
	rs.iDX9Resizer			= (int)m_cbDX9Resizer.GetItemData(m_cbDX9Resizer.GetCurSel());
	rs.fVMRMixerMode		= !!m_bVMRMixerMode;
	rs.fVMRMixerYUV			= !!m_bVMRMixerYUV;
	s.fD3DFullscreen		= !!m_chkD3DFullscreen.GetCheck();
	rs.fResetDevice			= !!m_bResetDevice;

	rs.m_AdvRendSets.iDX9SurfaceFormat		= m_cbDX9SurfaceFormat.GetItemData(m_cbDX9SurfaceFormat.GetCurSel());
	rs.m_AdvRendSets.b10BitOutput			= !!m_chk10bitOutput.GetCheck();
	rs.m_AdvRendSets.iVMRDisableDesktopComposition = !!m_chkDisableAero.GetCheck();
	rs.m_AdvRendSets.iEVROutputRange		= m_cbEVROutputRange.GetCurSel();

	rs.m_AdvRendSets.fVMR9AlterativeVSync	= m_bVMR9AlterativeVSync != 0;

	if (!m_iEvrBuffers.IsEmpty()) {
		int Temp = 5;
		swscanf_s(m_iEvrBuffers.GetBuffer(), L"%d", &Temp);
		rs.iEvrBuffers = Temp;
	} else {
		rs.iEvrBuffers = 5;
	}

	rs.D3D9RenderDevice = m_bD3D9RenderDevice ? m_D3D9GUIDNames[m_iD3D9RenderDevice] : L"";

	return __super::OnApply();
}

void CPPageVideo::UpdateSurfaceFormatList(int select)
{
	for (int i = m_cbDX9SurfaceFormat.GetCount() - 1; i >= 0; i--) {
		m_cbDX9SurfaceFormat.DeleteString(i);
	}

	int videoRenderer = m_cbVideoRenderer.GetItemData(m_cbVideoRenderer.GetCurSel());

	m_cbDX9SurfaceFormat.SetItemData(m_cbDX9SurfaceFormat.AddString(L"8-bit Integer Surfaces"), D3DFMT_X8R8G8B8);

	if ((videoRenderer == VIDRNDT_DS_VMR9RENDERLESS || videoRenderer == VIDRNDT_DS_EVR_CUSTOM) && m_iAPSurfaceUsage == VIDRNDT_AP_TEXTURE3D) {
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

	if (m_iAPSurfaceUsage == VIDRNDT_AP_TEXTURE3D) {
		m_cbDX9Resizer.SetItemData(m_cbDX9Resizer.AddString(L"Perlin Smootherstep (PS 2.0)"), RESIZER_SHADER_SMOOTHERSTEP);
		m_cbDX9Resizer.SetItemData(m_cbDX9Resizer.AddString(L"Bicubic A=-0.6 (PS 2.0)"), RESIZER_SHADER_BICUBIC06);
		m_cbDX9Resizer.SetItemData(m_cbDX9Resizer.AddString(L"Bicubic A=-0.8 (PS 2.0)"), RESIZER_SHADER_BICUBIC08);
		m_cbDX9Resizer.SetItemData(m_cbDX9Resizer.AddString(L"Bicubic A=-1.0 (PS 2.0)"), RESIZER_SHADER_BICUBIC10);
		m_cbDX9Resizer.SetItemData(m_cbDX9Resizer.AddString(L"B-spline4 (PS 2.0)"), RESIZER_SHADER_BSPLINE4);
		m_cbDX9Resizer.SetItemData(m_cbDX9Resizer.AddString(L"Mitchell-Netravali spline4 (PS 2.0)"), RESIZER_SHADER_MITCHELL4);
		m_cbDX9Resizer.SetItemData(m_cbDX9Resizer.AddString(L"Catmull-Rom spline4 (PS 2.0)"), RESIZER_SHADER_CATMULL4);
#if ENABLE_2PASS_RESIZE
		m_cbDX9Resizer.SetItemData(m_cbDX9Resizer.AddString(L"Lanczos2 (PS 2.0)"), RESIZER_SHADER_LANCZOS2);
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

	switch (m_iAPSurfaceUsage) {
		case VIDRNDT_AP_SURFACE:
			m_wndToolTip.UpdateTipText(ResStr(IDC_REGULARSURF), GetDlgItem(IDC_DX_SURFACE));
			break;
		case VIDRNDT_AP_TEXTURE2D:
			m_wndToolTip.UpdateTipText(ResStr(IDC_TEXTURESURF2D), GetDlgItem(IDC_DX_SURFACE));
			break;
		case VIDRNDT_AP_TEXTURE3D:
			m_wndToolTip.UpdateTipText(ResStr(IDC_TEXTURESURF3D), GetDlgItem(IDC_DX_SURFACE));
			break;
	}

	UpdateSurfaceFormatList(AfxGetAppSettings().m_RenderersSettings.m_AdvRendSets.iDX9SurfaceFormat);
	UpdateResizerList(AfxGetAppSettings().m_RenderersSettings.iDX9Resizer);

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

	GetDlgItem(IDC_DX_SURFACE)->EnableWindow(FALSE);
	m_cbDX9SurfaceFormat.EnableWindow(FALSE);
	m_cbDX9Resizer.EnableWindow(FALSE);
	GetDlgItem(IDC_FULLSCREEN_MONITOR_CHECK)->EnableWindow(FALSE);
	GetDlgItem(IDC_DSVMRLOADMIXER)->EnableWindow(FALSE);
	GetDlgItem(IDC_DSVMRYUVMIXER)->EnableWindow(FALSE);
	GetDlgItem(IDC_DSVMR9ALTERNATIVEVSYNC)->EnableWindow(FALSE);
	GetDlgItem(IDC_RESETDEVICE)->EnableWindow(FALSE);
	GetDlgItem(IDC_EVR_BUFFERS)->EnableWindow(FALSE);
	GetDlgItem(IDC_EVR_BUFFERS_TXT)->EnableWindow(FALSE);
	GetDlgItem(IDC_D3D9DEVICE)->EnableWindow(FALSE);
	GetDlgItem(IDC_D3D9DEVICE_COMBO)->EnableWindow(FALSE);
	m_chkDisableAero.EnableWindow(FALSE);
	m_cbEVROutputRange.EnableWindow(FALSE);

	switch (CurrentVR) {
		case VIDRNDT_DS_DEFAULT:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSSYSDEF), GetDlgItem(IDC_VIDRND_COMBO));
			break;
		case VIDRNDT_DS_OVERLAYMIXER:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSOVERLAYMIXER), GetDlgItem(IDC_VIDRND_COMBO));
			break;
		case VIDRNDT_DS_VMR7WINDOWED:
			GetDlgItem(IDC_DSVMRLOADMIXER)->EnableWindow(TRUE);
			GetDlgItem(IDC_DSVMRYUVMIXER)->EnableWindow(TRUE);

			m_wndToolTip.UpdateTipText(ResStr(IDC_DSVMR7WIN), GetDlgItem(IDC_VIDRND_COMBO));
			break;
		case VIDRNDT_DS_VMR9WINDOWED:
			GetDlgItem(IDC_DSVMRLOADMIXER)->EnableWindow(TRUE);
			GetDlgItem(IDC_DSVMRYUVMIXER)->EnableWindow(TRUE);

			m_wndToolTip.UpdateTipText(ResStr(IDC_DSVMR9WIN), GetDlgItem(IDC_VIDRND_COMBO));
			break;
		case VIDRNDT_DS_VMR7RENDERLESS:
			GetDlgItem(IDC_DX_SURFACE)->EnableWindow(TRUE);
			GetDlgItem(IDC_DSVMRLOADMIXER)->EnableWindow(TRUE);
			GetDlgItem(IDC_DSVMRYUVMIXER)->EnableWindow(TRUE);

			m_wndToolTip.UpdateTipText(ResStr(IDC_DSVMR7REN), GetDlgItem(IDC_VIDRND_COMBO));
			break;
		case VIDRNDT_DS_VMR9RENDERLESS:
			if (m_cbD3D9RenderDevice.GetCount() > 1) {
				GetDlgItem(IDC_D3D9DEVICE)->EnableWindow(TRUE);
				GetDlgItem(IDC_D3D9DEVICE_COMBO)->EnableWindow(IsDlgButtonChecked(IDC_D3D9DEVICE));
			}
			GetDlgItem(IDC_DSVMRLOADMIXER)->EnableWindow(TRUE);
			GetDlgItem(IDC_DSVMRYUVMIXER)->EnableWindow(TRUE);
			GetDlgItem(IDC_DSVMR9ALTERNATIVEVSYNC)->EnableWindow(TRUE);
			GetDlgItem(IDC_RESETDEVICE)->EnableWindow(TRUE);
			m_cbDX9SurfaceFormat.EnableWindow(TRUE);
			m_cbDX9Resizer.EnableWindow(TRUE);
			GetDlgItem(IDC_FULLSCREEN_MONITOR_CHECK)->EnableWindow(TRUE);
			GetDlgItem(IDC_DSVMR9ALTERNATIVEVSYNC)->EnableWindow(TRUE);
			GetDlgItem(IDC_RESETDEVICE)->EnableWindow(TRUE);
			GetDlgItem(IDC_DX_SURFACE)->EnableWindow(TRUE);
			if (IsWinVista() || IsWinSeven() && m_iAPSurfaceUsage == VIDRNDT_AP_TEXTURE3D) {
				m_chkDisableAero.EnableWindow(TRUE);
			}

			m_wndToolTip.UpdateTipText(ResStr(IDC_DSVMR9REN), GetDlgItem(IDC_VIDRND_COMBO));
			break;
		case VIDRNDT_DS_EVR:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSEVR), GetDlgItem(IDC_VIDRND_COMBO));
			break;
		case VIDRNDT_DS_EVR_CUSTOM:
			if (m_cbD3D9RenderDevice.GetCount() > 1) {
				GetDlgItem(IDC_D3D9DEVICE)->EnableWindow(TRUE);
				GetDlgItem(IDC_D3D9DEVICE_COMBO)->EnableWindow(IsDlgButtonChecked(IDC_D3D9DEVICE));
			}
			m_cbDX9SurfaceFormat.EnableWindow(TRUE);
			m_cbDX9Resizer.EnableWindow(TRUE);
			GetDlgItem(IDC_FULLSCREEN_MONITOR_CHECK)->EnableWindow(TRUE);
			GetDlgItem(IDC_DSVMR9ALTERNATIVEVSYNC)->EnableWindow(TRUE);
			GetDlgItem(IDC_RESETDEVICE)->EnableWindow(TRUE);
			GetDlgItem(IDC_EVR_BUFFERS)->EnableWindow(TRUE);
			GetDlgItem(IDC_EVR_BUFFERS_TXT)->EnableWindow(TRUE);

			// Force 3D surface with EVR Custom
			GetDlgItem(IDC_DX_SURFACE)->EnableWindow(FALSE);
			((CComboBox*)GetDlgItem(IDC_DX_SURFACE))->SetCurSel(2);
			OnSurfaceChange();

			if (IsWinVista() || IsWinSeven()) {
				m_chkDisableAero.EnableWindow(TRUE);
			}
			m_cbEVROutputRange.EnableWindow(TRUE);

			m_wndToolTip.UpdateTipText(ResStr(IDC_DSEVR_CUSTOM), GetDlgItem(IDC_VIDRND_COMBO));
			break;
		case VIDRNDT_DS_SYNC:
			GetDlgItem(IDC_EVR_BUFFERS)->EnableWindow(TRUE);
			GetDlgItem(IDC_EVR_BUFFERS_TXT)->EnableWindow(TRUE);
			m_cbDX9Resizer.EnableWindow(TRUE);
			GetDlgItem(IDC_FULLSCREEN_MONITOR_CHECK)->EnableWindow(TRUE);
			GetDlgItem(IDC_RESETDEVICE)->EnableWindow(TRUE);
			GetDlgItem(IDC_DX_SURFACE)->EnableWindow(FALSE);
			((CComboBox*)GetDlgItem(IDC_DX_SURFACE))->SetCurSel(2);
			OnSurfaceChange();

			if (IsWinVista() || IsWinSeven()) {
				m_chkDisableAero.EnableWindow(TRUE);
			}
			m_cbEVROutputRange.EnableWindow(TRUE);

			m_wndToolTip.UpdateTipText(ResStr(IDC_DSSYNC), GetDlgItem(IDC_VIDRND_COMBO));
			break;
		case VIDRNDT_DS_DXR:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSDXR), GetDlgItem(IDC_VIDRND_COMBO));
			break;
		case VIDRNDT_DS_NULL_COMP:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSNULL_COMP), GetDlgItem(IDC_VIDRND_COMBO));
			break;
		case VIDRNDT_DS_NULL_UNCOMP:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSNULL_UNCOMP), GetDlgItem(IDC_VIDRND_COMBO));
			break;
		case VIDRNDT_DS_MADVR:
			m_wndToolTip.UpdateTipText(ResStr(IDC_DSMADVR), GetDlgItem(IDC_VIDRND_COMBO));
			break;
		default:
			m_wndToolTip.UpdateTipText(L"", GetDlgItem(IDC_VIDRND_COMBO));
	}

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
	GetDlgItem(IDC_D3D9DEVICE_COMBO)->EnableWindow(m_bD3D9RenderDevice);
	SetModified();
}

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
#include "MainFrm.h"
#include "PPageFullscreen.h"
#include "../../DSUtil/WinAPIUtils.h"
#include "../../DSUtil/SysVersion.h"
#include "MultiMonitor.h"

static CString FormatModeString(dispmode dmod)
{
	CString strMode;
	strMode.Format(L"[ %d ]  @ %dx%d %s", dmod.freq, dmod.size.cx, dmod.size.cy, dmod.dmDisplayFlags & DM_INTERLACED ? L"i" : L"p");

	return strMode;
}

// CPPagePlayer dialog

IMPLEMENT_DYNAMIC(CPPageFullscreen, CPPageBase)
CPPageFullscreen::CPPageFullscreen()
	: CPPageBase(CPPageFullscreen::IDD, CPPageFullscreen::IDD)
	, m_bLaunchFullScreen(FALSE)
	, m_bEnableAutoMode(FALSE)
	, m_bBeforePlayback(FALSE)
	, m_bSetDefault(FALSE)
	, m_bShowBarsWhenFullScreen(FALSE)
	, m_bExitFullScreenAtTheEnd(FALSE)
	, m_bExitFullScreenAtFocusLost(FALSE)
	, m_bRestoreResAfterExit(TRUE)
	, m_nCurMon(0)
	, m_list(0)
{
}

CPPageFullscreen::~CPPageFullscreen()
{
}

void CPPageFullscreen::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK1, m_bLaunchFullScreen);
	DDX_Check(pDX, IDC_CHECK4, m_bShowBarsWhenFullScreen);
	DDX_Control(pDX, IDC_EDIT1, m_edtTimeOut);
	DDX_Control(pDX, IDC_SPIN1, m_nTimeOutCtrl);
	DDX_Check(pDX, IDC_CHECK5, m_bExitFullScreenAtTheEnd);
	DDX_Check(pDX, IDC_CHECK6, m_bExitFullScreenAtFocusLost);
	DDX_CBIndex(pDX, IDC_COMBO1, m_iMonitorType);
	DDX_Control(pDX, IDC_COMBO1, m_iMonitorTypeCtrl);
	DDX_Check(pDX, IDC_CHECK2, m_bEnableAutoMode);
	DDX_Check(pDX, IDC_CHECK7, m_bBeforePlayback);
	DDX_Control(pDX, IDC_LIST1, m_list);
	DDX_Control(pDX, IDC_EDIT2, m_edDMChangeDelay);
	DDX_Control(pDX, IDC_SPIN2, m_spnDMChangeDelay);
	DDX_Check(pDX, IDC_CHECK3, m_bSetDefault);
	DDX_Check(pDX, IDC_RESTORERESCHECK, m_bRestoreResAfterExit);
}

BEGIN_MESSAGE_MAP(CPPageFullscreen, CPPageBase)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnUpdateFullScrCombo)
	ON_NOTIFY(LVN_DOLABELEDIT, IDC_LIST1, OnDolabeleditList)
	ON_NOTIFY(LVN_BEGINLABELEDIT, IDC_LIST1, OnBeginlabeleditList)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_LIST1, OnEndlabeleditList)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST1, OnCustomdrawList)
	ON_CLBN_CHKCHANGE(IDC_LIST1, OnCheckChangeList)

	ON_UPDATE_COMMAND_UI(IDC_LIST1, OnUpdateFullscreenRes)
	ON_UPDATE_COMMAND_UI(IDC_CHECK7, OnUpdateFullscreenRes)
	ON_UPDATE_COMMAND_UI(IDC_CHECK3, OnUpdateFullscreenRes)
	ON_UPDATE_COMMAND_UI(IDC_RESTORERESCHECK, OnUpdateFullscreenRes)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON1, OnUpdateFullscreenRes)
	ON_UPDATE_COMMAND_UI(IDC_EDIT2, OnUpdateFullscreenRes)
	ON_UPDATE_COMMAND_UI(IDC_SPIN2, OnUpdateFullscreenRes)

	ON_COMMAND(IDC_CHECK2, OnUpdateSetFullscreenRes)
	ON_UPDATE_COMMAND_UI(IDC_CHECK4, OnUpdateShowBarsWhenFullScreen)
	ON_UPDATE_COMMAND_UI(IDC_EDIT1, OnUpdateShowBarsWhenFullScreenTimeOut)
	ON_UPDATE_COMMAND_UI(IDC_COMBO1, OnUpdateFullScrComboCtrl)

	ON_UPDATE_COMMAND_UI(IDC_STATIC1, OnUpdateStatic1)
	ON_UPDATE_COMMAND_UI(IDC_STATIC2, OnUpdateStatic2)

	ON_UPDATE_COMMAND_UI(IDC_SPIN1, OnUpdateTimeout)
	ON_UPDATE_COMMAND_UI(IDC_EDIT1, OnUpdateTimeout)

	ON_BN_CLICKED(IDC_BUTTON2, OnRemove)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON2, OnUpdateRemove)
	ON_BN_CLICKED(IDC_BUTTON1, OnAdd)

	ON_BN_CLICKED(IDC_BUTTON3, OnMoveUp)
	ON_BN_CLICKED(IDC_BUTTON4, OnMoveDown)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON3, OnUpdateUp)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON4, OnUpdateDown)

END_MESSAGE_MAP()

// CPPagePlayer message handlers

BOOL CPPageFullscreen::OnInitDialog()
{
	__super::OnInitDialog();

	SetCursor(m_hWnd, IDC_COMBO1, IDC_HAND);

	CAppSettings& s = AfxGetAppSettings();

	m_bLaunchFullScreen					= s.fLaunchfullscreen;
	m_AutoChangeFullscrRes				= s.AutoChangeFullscrRes;
	m_bSetDefault						= s.AutoChangeFullscrRes.bApplyDefault;
	m_f_hmonitor						= s.strFullScreenMonitor;
	m_f_hmonitorID						= s.strFullScreenMonitorID;
	m_bShowBarsWhenFullScreen			= s.fShowBarsWhenFullScreen;
	m_edtTimeOut						= s.nShowBarsWhenFullScreenTimeOut;
	m_edtTimeOut.SetRange(-1, 10);
	m_nTimeOutCtrl.SetRange(-1, 10);
	m_bExitFullScreenAtTheEnd			= s.fExitFullScreenAtTheEnd;
	m_bExitFullScreenAtFocusLost		= s.fExitFullScreenAtFocusLost;
	m_edDMChangeDelay					= s.iDMChangeDelay;
	m_edDMChangeDelay.SetRange(0, 9);
	m_spnDMChangeDelay.SetRange(0, 9);


	m_bRestoreResAfterExit				= s.fRestoreResAfterExit;

	CString str;
	m_iMonitorType = 0;
	CMonitor curmonitor;
	CMonitors monitors;
	CString strCurMon;

	m_iMonitorTypeCtrl.AddString(ResStr(IDS_FULLSCREENMONITOR_CURRENT));
	m_MonitorDisplayNames.Add(L"Current");
	m_MonitorDeviceName.Add(L"Current");
	curmonitor = monitors.GetNearestMonitor(AfxGetApp()->m_pMainWnd);
	curmonitor.GetName(strCurMon);
	if(m_f_hmonitor.IsEmpty()) {
		m_f_hmonitor = L"Current";
		m_iMonitorType = 0;
	}

	DISPLAY_DEVICE dd;
	dd.cb = sizeof(dd);
	DWORD dev = 0; // device index
	int id = 0;
	CString str2;
	CString DeviceID;
	CString strMonID;
	while (EnumDisplayDevices(0, dev, &dd, 0)) {
		DISPLAY_DEVICE ddMon;
		ZeroMemory(&ddMon, sizeof(ddMon));
		ddMon.cb = sizeof(ddMon);
		DWORD devMon = 0;

		while (EnumDisplayDevices(dd.DeviceName, devMon, &ddMon, 0)) {
			if (ddMon.StateFlags & DISPLAY_DEVICE_ACTIVE && !(ddMon.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)) {
				DeviceID.Format (L"%s", ddMon.DeviceID);
				strMonID = DeviceID = DeviceID.Mid (8, DeviceID.Find (L"\\", 9) - 8);
				str.Format(L"%s", ddMon.DeviceName);
				str = str.Left(12);
				if (str == strCurMon) {
					m_iMonitorTypeCtrl.AddString(str.Mid(4, 7) + L"( " + str.Right(1) + L" ) " + L"- [id: " + DeviceID + L" *" +  ResStr(IDS_FULLSCREENMONITOR_CURRENT) + L"] - " + ddMon.DeviceString);
					m_MonitorDisplayNames[0] = L"Current" + strMonID;
					m_MonitorDeviceName[0] = str;

					if(m_f_hmonitor == L"Current" && m_AutoChangeFullscrRes.bEnabled > 0) {
						m_iMonitorType = m_iMonitorTypeCtrl.GetCount()-1;
						m_f_hmonitor = strCurMon;
					}
					m_nCurMon = m_iMonitorTypeCtrl.GetCount() - 1;
				} else {
					m_iMonitorTypeCtrl.AddString(str.Mid(4, 7) + L"( " + str.Right(1) + L" ) " + L"- [id: " + DeviceID + L"] - " + ddMon.DeviceString);
				}
				m_MonitorDisplayNames.Add(str + strMonID);
				m_MonitorDeviceName.Add(str);
				if(m_iMonitorType == 0 && m_f_hmonitor == str) {
					m_iMonitorType = m_iMonitorTypeCtrl.GetCount()-1;
				}

				if (m_f_hmonitorID == strMonID  && m_f_hmonitor != L"Current") {
					id = m_iMonitorType = m_iMonitorTypeCtrl.GetCount()-1;
					str2 = str;
				}
			}
			devMon++;
			ZeroMemory(&ddMon, sizeof(ddMon));
			ddMon.cb = sizeof(ddMon);
		}
		ZeroMemory(&dd, sizeof(dd));
		dd.cb = sizeof(dd);
		dev++;
	}

	if(m_iMonitorTypeCtrl.GetCount() > 2) {
		if (m_MonitorDisplayNames[m_iMonitorType] != m_f_hmonitor + m_f_hmonitorID) {
			if ( id > 0 ) {
				m_iMonitorType = id;
				m_f_hmonitor = str2;
			}
		}
		GetDlgItem(IDC_COMBO1)->EnableWindow(TRUE);
	} else {
		if(m_AutoChangeFullscrRes.bEnabled == false)  {
			m_iMonitorType = 0;
		} else {
			m_iMonitorType = 1;
		}
		GetDlgItem(IDC_COMBO1)->EnableWindow(FALSE);
	}

	if (m_AutoChangeFullscrRes.bEnabled == false && (m_MonitorDisplayNames[m_iMonitorType]).Left(7) == L"Current"){
		m_f_hmonitor = L"Current";
	}

	m_list.SetExtendedStyle(m_list.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER
							| LVS_EX_GRIDLINES | LVS_EX_BORDERSELECT | LVS_EX_ONECLICKACTIVATE | LVS_EX_CHECKBOXES | LVS_EX_FLATSB);
	m_list.InsertColumn(COL_Z, ResStr(IDS_PPAGE_FS_CLN_ON_OFF), LVCFMT_LEFT, 60, -1, -1);
	m_list.InsertColumn(COL_VFR_F, ResStr(IDS_PPAGE_FS_CLN_FROM_FPS), LVCFMT_RIGHT, 60, -1, -1);
	m_list.InsertColumn(COL_VFR_T, ResStr(IDS_PPAGE_FS_CLN_TO_FPS), LVCFMT_RIGHT, 60, -1, -1);
	m_list.InsertColumn(COL_SRR, ResStr(IDS_PPAGE_FS_CLN_DISPLAY_MODE), LVCFMT_LEFT, 135, -1, -1);

	CorrectCWndWidth(GetDlgItem(IDC_CHECK2));

	ModesUpdate();
	UpdateData(FALSE);

	return TRUE;
}

void CPPageFullscreen::OnCustomdrawList(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );
	*pResult = CDRF_DODEFAULT;

	if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage ) {
		*pResult = CDRF_NOTIFYITEMDRAW;
	} else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage ) {
		*pResult = CDRF_NOTIFYSUBITEMDRAW;
	} else if ( (CDDS_ITEMPREPAINT | CDDS_SUBITEM) == pLVCD->nmcd.dwDrawStage ) {
		COLORREF crText, crBkgnd;

		m_AutoChangeFullscrRes.bEnabled = m_bEnableAutoMode ? m_bEnableAutoMode + m_bBeforePlayback : 0;

		if (m_AutoChangeFullscrRes.bEnabled == false) {
			crText = RGB(128,128,128);
			crBkgnd = RGB(240, 240, 240);
		} else {
			crText = RGB(0,0,0);
			crBkgnd = RGB(255,255,255);
		}
		if (m_list.GetCheck(pLVCD->nmcd.dwItemSpec) == false) {
			crText = RGB(128,128,128);
		}
		pLVCD->clrText = crText;
		pLVCD->clrTextBk = crBkgnd;
		*pResult = CDRF_DODEFAULT;
	}
}

BOOL CPPageFullscreen::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	m_AutoChangeFullscrRes.bEnabled = m_bEnableAutoMode ? m_bEnableAutoMode + m_bBeforePlayback : 0;

	for (int nItem = 0; nItem < MaxFpsCount; nItem++) {
		if (nItem < m_list.GetItemCount()) {
			m_AutoChangeFullscrRes.dmFullscreenRes[nItem].dmFSRes = m_dms[m_list.GetItemData(nItem)];
			m_AutoChangeFullscrRes.dmFullscreenRes[nItem].bChecked = !!m_list.GetCheck(nItem);
			m_AutoChangeFullscrRes.dmFullscreenRes[nItem].bValid = true;
			m_AutoChangeFullscrRes.dmFullscreenRes[nItem].vfr_from = wcstod(m_list.GetItemText(nItem, COL_VFR_F), nullptr);
			m_AutoChangeFullscrRes.dmFullscreenRes[nItem].vfr_to = wcstod(m_list.GetItemText(nItem, COL_VFR_T), nullptr);
		} else {
			m_AutoChangeFullscrRes.dmFullscreenRes[nItem].Empty();
		}
	}

	m_AutoChangeFullscrRes.bApplyDefault	= !!m_bSetDefault;
	s.AutoChangeFullscrRes					= m_AutoChangeFullscrRes;
	s.fLaunchfullscreen						= !!m_bLaunchFullScreen;

	CString str;
	CString strCurMon;
	str = m_MonitorDisplayNames[m_iMonitorType];
	if (str.GetLength() == 14) { m_f_hmonitor = str.Left(7); }
	if (str.GetLength() == 19) { m_f_hmonitor = str.Left(12); }
	m_f_hmonitorID = str.Right(7);
	if (m_AutoChangeFullscrRes.bEnabled > 0 && m_f_hmonitor == L"Current") {
		CMonitors monitors;
		CMonitor curmonitor;
		curmonitor = monitors.GetNearestMonitor(AfxGetApp()->m_pMainWnd);
		curmonitor.GetName(strCurMon);
		m_f_hmonitor = strCurMon;
	}
	if (m_AutoChangeFullscrRes.bEnabled == false && m_f_hmonitor == L"Current") {
		m_f_hmonitor = L"Current";
	}
	s.strFullScreenMonitor					= m_f_hmonitor;
	s.strFullScreenMonitorID				= m_f_hmonitorID;

	s.fShowBarsWhenFullScreen				= !!m_bShowBarsWhenFullScreen;
	s.nShowBarsWhenFullScreenTimeOut		= m_edtTimeOut;
	s.fExitFullScreenAtTheEnd				= !!m_bExitFullScreenAtTheEnd;
	s.fExitFullScreenAtFocusLost			= !!m_bExitFullScreenAtFocusLost;
	s.fRestoreResAfterExit					= !!m_bRestoreResAfterExit;
	s.iDMChangeDelay						= m_edDMChangeDelay;

	return __super::OnApply();
}

void CPPageFullscreen::OnBeginlabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	LV_ITEM* pItem = &pDispInfo->item;
	*pResult = FALSE;
	if (pItem->iItem < 0) {
		return;
	}
	*pResult = TRUE;
}

void CPPageFullscreen::OnDolabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	LV_ITEM* pItem = &pDispInfo->item;

	*pResult = FALSE;
	if (pItem->iItem < 0) {
		return;
	}

	switch (pItem->iSubItem) {
		case COL_SRR:
			m_list.ShowInPlaceComboBox(pItem->iItem, pItem->iSubItem, m_displayModesString, (int)m_list.GetItemData(pItem->iItem));
			break;
		case COL_VFR_F:
		case COL_VFR_T:
			if (pItem->iItem > 0) {
				m_list.ShowInPlaceFloatEdit(pItem->iItem, pItem->iSubItem);
			}
			break;
	}

	m_list.RedrawWindow();
	*pResult = TRUE;
}

void CPPageFullscreen::OnEndlabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	LV_ITEM* pItem = &pDispInfo->item;

	*pResult = FALSE;
	if (!m_list.m_fInPlaceDirty) {
		return;
	}
	if (pItem->iItem < 0) {
		return;
	}

	switch (pItem->iSubItem) {
		case COL_SRR:
			if (pItem->lParam >= 0) {
				VERIFY(m_list.SetItemData(pItem->iItem, (DWORD_PTR)(int)pItem->lParam));
				m_list.SetItemText(pItem->iItem, pItem->iSubItem, pItem->pszText);
			}
			break;
		case COL_VFR_F:
		case COL_VFR_T:
			if (pItem->pszText) {
				CString str = pItem->pszText;
				double dFR = wcstod(str, nullptr);
				dFR = std::clamp(dFR, 1.0, 125.999);
				str.Format(L"%.3f", dFR);
				m_list.SetItemText(pItem->iItem, pItem->iSubItem, str);
			}
			break;
	}

	*pResult = TRUE;

	SetModified();
}

void CPPageFullscreen::OnCheckChangeList()
{
	SetModified();
}

void CPPageFullscreen::OnUpdateFullscreenRes(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_bEnableAutoMode);
}

void CPPageFullscreen::OnUpdateFullScrComboCtrl(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!m_bEnableAutoMode);
}

void CPPageFullscreen::OnUpdateSetFullscreenRes()
{
	CMonitor curmonitor;
	CMonitors monitors;
	CString strCurMon;
	curmonitor = monitors.GetNearestMonitor(AfxGetApp()->m_pMainWnd);
	curmonitor.GetName(strCurMon);
	if (m_iMonitorTypeCtrl.GetCurSel() == 0) {
		m_iMonitorTypeCtrl.SetCurSel(m_nCurMon);
		m_f_hmonitor = strCurMon;
	}

	SetModified();
}

void CPPageFullscreen::OnUpdateShowBarsWhenFullScreen(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!AfxGetAppSettings().IsD3DFullscreen());
}

void CPPageFullscreen::OnUpdateShowBarsWhenFullScreenTimeOut(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!AfxGetAppSettings().IsD3DFullscreen());
}

void CPPageFullscreen::OnUpdateStatic1(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!AfxGetAppSettings().IsD3DFullscreen());
}

void CPPageFullscreen::OnUpdateStatic2(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!AfxGetAppSettings().IsD3DFullscreen());
}

void CPPageFullscreen::OnUpdateFullScrCombo()
{
	CMonitors monitors;
	CMonitor curmonitor;
	CString str, strCurMonID, strCurMon;
	str = m_MonitorDisplayNames[m_iMonitorTypeCtrl.GetCurSel()];
	if (str.GetLength() == 14) { m_f_hmonitor = str.Left(7); }
	if (str.GetLength() == 19) { m_f_hmonitor = str.Left(12); }
	strCurMonID = str.Right(7);
	curmonitor = monitors.GetNearestMonitor(AfxGetApp()->m_pMainWnd);
	curmonitor.GetName(strCurMon);

	if (m_f_hmonitor == L"Current" && m_AutoChangeFullscrRes.bEnabled > 0) {
		m_f_hmonitor = strCurMon;
	}

	if (m_f_hmonitor != L"Current" && m_f_hmonitor != strCurMon) {
		m_AutoChangeFullscrRes.bEnabled = false;
	}

	ModesUpdate();
	SetModified();
}

void CPPageFullscreen::OnUpdateTimeout(CCmdUI* pCmdUI)
{
	UpdateData();
	pCmdUI->Enable(m_bShowBarsWhenFullScreen);
}

void CPPageFullscreen::ModesUpdate()
{
	CAppSettings& s = AfxGetAppSettings();

	m_list.SetRedraw(FALSE);

	m_bEnableAutoMode = m_AutoChangeFullscrRes.bEnabled > 0;
	m_bBeforePlayback = m_AutoChangeFullscrRes.bEnabled == 2;

	/*
	CString strDevice = m_MonitorDeviceName[m_iMonitorType];
	CString MonitorName;
	UINT16 MonitorHorRes, MonitorVerRes;
	ReadDisplay(strDevice, &MonitorName, &MonitorHorRes, &MonitorVerRes);
	*/

	CString str;
	str = m_MonitorDisplayNames[m_iMonitorType];
	if (str.GetLength() == 14) { m_f_hmonitor = str.Left(7); }
	if (str.GetLength() == 19) { m_f_hmonitor = str.Left(12); }
	m_f_hmonitorID = str.Right(7);
	if (s.strFullScreenMonitorID != m_f_hmonitorID) {
		m_AutoChangeFullscrRes.bEnabled = false;
	}

	m_list.DeleteAllItems();
	m_dms.clear();
	m_displayModesString.clear();

	dispmode dm;
	for (int i = 0, m = 0, ModeExist = true;  ; i++) {
		if (!CMainFrame::GetDispMode(i, dm, m_f_hmonitor)) {
			break;
		}
		if (dm.bpp != 32 || dm.size.cx < 720 || dm.size.cy < 480) {
			continue; // skip low resolution and non 32bpp mode
		}

		int j = 0;
		while (j < m) {
			if (dm == m_dms[j]) {
				break;
			}
			j++;
		}
		if (j < m) {
			continue;
		}
		m_dms.push_back(dm);
		m++;
	}

	// sort display modes
	std::sort(m_dms.begin(), m_dms.end());

	CString strCur, strModes;
	GetCurDispModeString(strCur);

	dispmode dCurMod;
	CMainFrame::GetCurDispMode(dCurMod, m_f_hmonitor);

	int curModeIdx = m_dms.size() - 1;
	for (size_t i = 0; i < m_dms.size(); i++) {
		m_displayModesString.push_back(FormatModeString(m_dms[i]));
		if (dCurMod == m_dms[i]) {
			curModeIdx = i;
		}
	}

	int nItem = 0;
	if (m_AutoChangeFullscrRes.dmFullscreenRes[0].bValid
			&& m_AutoChangeFullscrRes.dmFullscreenRes[0].vfr_from != 0.000) {
		m_list.InsertItem(nItem, L"Default");
		m_list.SetItemText(nItem, COL_VFR_F, L"-");
		m_list.SetItemText(nItem, COL_VFR_T, L"-");
		m_list.SetItemText(nItem, COL_SRR, strCur);
		m_list.SetItemData(nItem, (DWORD_PTR)curModeIdx);
		m_list.SetCheck(nItem, TRUE);
		nItem++;
	}

	auto findDisplayMode = [this](const dispmode& dm, const int& curModeIdx) {
		for (size_t i = 0; i < m_dms.size(); i++) {
			if (dm == m_dms[i]) {
				return (int)i;
			}
		}

		return curModeIdx;
	};

	for (int n = 0; n < MaxFpsCount; n++) {
		if (m_AutoChangeFullscrRes.dmFullscreenRes[n].bValid) {
			int dmIdx = findDisplayMode(m_AutoChangeFullscrRes.dmFullscreenRes[n].dmFSRes, curModeIdx);

			CString str;
			nItem > 0 ? str.Format(L"%02d", n) : str = L"Default";
			m_list.InsertItem(nItem, str);
			m_list.SetItemText(nItem, COL_SRR, FormatModeString(m_dms[dmIdx]));
			nItem > 0 ? str.Format(L"%.3f", m_AutoChangeFullscrRes.dmFullscreenRes[n].vfr_from) : str = L"-";
			m_list.SetItemText(nItem, COL_VFR_F, str);
			nItem > 0 ? str.Format(L"%.3f", m_AutoChangeFullscrRes.dmFullscreenRes[n].vfr_to) : str = L"-";
			m_list.SetItemText(nItem, COL_VFR_T, str);
			m_list.SetItemData(nItem, (DWORD_PTR)dmIdx);
			m_list.SetCheck(nItem, m_AutoChangeFullscrRes.dmFullscreenRes[n].bChecked);
			nItem++;
		}
	}

	if (m_list.GetItemCount() < 1) {
		nItem = 0;
		m_list.InsertItem(nItem, L"Default");
		m_list.SetItemText(nItem, COL_VFR_F, L"-");
		m_list.SetItemText(nItem, COL_VFR_T, L"-");
		m_list.SetItemText(nItem, COL_SRR, strCur);
		m_list.SetItemData(nItem, (DWORD_PTR)curModeIdx);
		m_list.SetCheck(nItem, TRUE);

		static const struct {
			double vfr_from;
			double vfr_to;
		} vfr [8] = {
			{ 23.400, 23.981 },
			{ 23.982, 24.499 },
			{ 24.500, 25.499 },
			{ 29.500, 29.981 },
			{ 29.982, 30.499 },
			{ 49.500, 50.499 },
			{ 59.500, 59.945 },
			{ 59.946, 60.499 },
		};

		for (nItem = 0; nItem < _countof(vfr); nItem++) {
			str.Format(L"%02d", nItem + 1);
			m_list.InsertItem(nItem + 1, str);

			str.Format(L"%.3f", vfr[nItem].vfr_from);
			m_list.SetItemText(nItem + 1, COL_VFR_F, str);
			str.Format(L"%.3f", vfr[nItem].vfr_to);
			m_list.SetItemText(nItem + 1, COL_VFR_T, str);
			m_list.SetItemText(nItem + 1, COL_SRR, strCur);
			m_list.SetItemData(nItem, (DWORD_PTR)curModeIdx);
			m_list.SetCheck(nItem + 1, TRUE);
		}
	}

	for (int nCol = 0; nCol <= COL_SRR; nCol++) {
		m_list.SetColumnWidth(nCol, LVSCW_AUTOSIZE);
		int nColumnWidth = m_list.GetColumnWidth(nCol);
		m_list.SetColumnWidth(nCol, LVSCW_AUTOSIZE_USEHEADER);
		int nHeaderWidth = m_list.GetColumnWidth(nCol);
		int nNewWidth = std::max(nColumnWidth, nHeaderWidth);
		m_list.SetColumnWidth(nCol, nNewWidth);
		{
			LVCOLUMN col;
			col.mask = LVCF_MINWIDTH;
			col.cxMin = nNewWidth;
			m_list.SetColumn(nCol, &col);
		}
	}

	m_list.SetExtendedStyle(m_list.GetExtendedStyle() | LVS_EX_COLUMNSNAPPOINTS);
	m_list.SetRedraw(TRUE);
}

void CPPageFullscreen::OnRemove()
{
	if (POSITION pos = m_list.GetFirstSelectedItemPosition()) {
		int nItem = m_list.GetNextSelectedItem(pos);
		if (nItem <= 0 || nItem >= m_list.GetItemCount()) {
			return;
		}
		m_list.DeleteItem(nItem);
		nItem = std::min(nItem, m_list.GetItemCount() - 1);
		m_list.SetSelectionMark(nItem);
		m_list.SetFocus();
		m_list.SetItemState(nItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		ReindexList();

		SetModified();
	}
}

void CPPageFullscreen::OnUpdateRemove(CCmdUI* pCmdUI)
{
	POSITION pos = m_list.GetFirstSelectedItemPosition();
	int nItem = pos ? m_list.GetNextSelectedItem(pos) : -1;
	pCmdUI->Enable(m_bEnableAutoMode && nItem > 0);
}

void CPPageFullscreen::OnAdd()
{
	POSITION pos = m_list.GetFirstSelectedItemPosition();
	int nItem = pos ? (m_list.GetNextSelectedItem(pos) + 1) : 0;
	if (nItem <= 0) {
		nItem = m_list.GetItemCount();
	}

	if (m_list.GetItemCount() <= MaxFpsCount) {
		CString str, strCur;
		str.Format(L"%02d", nItem);
		m_list.InsertItem(nItem, str);
		m_list.SetItemText(nItem, COL_VFR_F, L"1.000");
		m_list.SetItemText(nItem, COL_VFR_T, L"1.000");
		GetCurDispModeString(strCur);
		m_list.SetItemText(nItem, COL_SRR, strCur);
		m_list.SetCheck(nItem, FALSE);
		m_list.SetFocus();
		m_list.SetItemState(nItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		ReindexList();

		SetModified();
	}
}

void CPPageFullscreen::OnMoveUp()
{
	if (POSITION pos = m_list.GetFirstSelectedItemPosition()) {
		int nItem = m_list.GetNextSelectedItem(pos);
		if (nItem <= 1 || nItem >= m_list.GetItemCount()) {
			return;
		}

		DWORD_PTR data = m_list.GetItemData(nItem);
		int nCheckCur = m_list.GetCheck(nItem);
		CString strN = m_list.GetItemText(nItem, 0);
		CString strF = m_list.GetItemText(nItem, 1);
		CString strT = m_list.GetItemText(nItem, 2);
		CString strDM = m_list.GetItemText(nItem, 3);
		m_list.DeleteItem(nItem);

		nItem--;
		m_list.InsertItem(nItem, strN);
		m_list.SetItemData(nItem, data);
		m_list.SetItemText(nItem, 1, strF);
		m_list.SetItemText(nItem, 2, strT);
		m_list.SetItemText(nItem, 3, strDM);
		m_list.SetCheck(nItem, nCheckCur);
		m_list.SetFocus();
		m_list.SetSelectionMark(nItem);
		m_list.SetItemState(nItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		ReindexList();

		SetModified();
	}
}

void CPPageFullscreen::OnMoveDown()
{
	if (POSITION pos = m_list.GetFirstSelectedItemPosition()) {
		int nItem = m_list.GetNextSelectedItem(pos);
		if (nItem <= 0 || nItem >= m_list.GetItemCount() - 1) {
			return;
		}

		DWORD_PTR data = m_list.GetItemData(nItem);
		int nCheckCur = m_list.GetCheck(nItem);
		CString strN = m_list.GetItemText(nItem, 0);
		CString strF = m_list.GetItemText(nItem, 1);
		CString strT = m_list.GetItemText(nItem, 2);
		CString strDM = m_list.GetItemText(nItem, 3);
		m_list.DeleteItem(nItem);

		nItem++;

		m_list.InsertItem(nItem, strN);
		m_list.SetItemData(nItem, data);
		m_list.SetItemText(nItem, 1, strF);
		m_list.SetItemText(nItem, 2, strT);
		m_list.SetItemText(nItem, 3, strDM);
		m_list.SetCheck(nItem, nCheckCur);
		m_list.SetFocus();
		m_list.SetSelectionMark(nItem);
		m_list.SetItemState(nItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		ReindexList();

		SetModified();
	}
}

void CPPageFullscreen::OnUpdateUp(CCmdUI* pCmdUI)
{
	POSITION pos = m_list.GetFirstSelectedItemPosition();
	int nItem = pos ? m_list.GetNextSelectedItem(pos) : -1;
	pCmdUI->Enable(m_bEnableAutoMode && nItem > 1);
}

void CPPageFullscreen::OnUpdateDown(CCmdUI* pCmdUI)
{
	POSITION pos = m_list.GetFirstSelectedItemPosition();
	int nItem = pos ? m_list.GetNextSelectedItem(pos) : -1;
	pCmdUI->Enable(m_bEnableAutoMode && nItem > 0 && nItem < m_list.GetItemCount() - 1);
}

void CPPageFullscreen::ReindexList()
{
	if (m_list.GetItemCount() > 0) {
		CString str;
		for (int i = 0; i < m_list.GetItemCount(); i++) {
			i > 0 ? str.Format(L"%02d", i) : str = L"Default";
			m_list.SetItemText(i, 0, str);
		}
	}
}

void CPPageFullscreen::GetCurDispModeString(CString& strCur)
{
	dispmode dmod;
	CMainFrame::GetCurDispMode(dmod, m_f_hmonitor);

	strCur = FormatModeString(dmod);
}

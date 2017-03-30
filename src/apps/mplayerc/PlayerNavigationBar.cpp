/*
 * (C) 2006-2016 see Authors.txt
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
#include "PlayerNavigationBar.h"
#include <afxwin.h>
#include <moreuuids.h>

// CPlayerCaptureBar

IMPLEMENT_DYNAMIC(CPlayerNavigationBar, CPlayerBar)

CPlayerNavigationBar::CPlayerNavigationBar()
{
}

CPlayerNavigationBar::~CPlayerNavigationBar()
{
}

BOOL CPlayerNavigationBar::Create(CWnd* pParentWnd, UINT defDockBarID)
{
	if (!CPlayerBar::Create(ResStr(IDS_NAVIGATION_BAR), pParentWnd, ID_VIEW_NAVIGATION, defDockBarID, L"Navigation Bar")) {
		return FALSE;
	}

	m_pParent = pParentWnd;
	m_navdlg.Create(this);
	m_navdlg.ShowWindow(SW_SHOWNORMAL);

	CRect r;
	m_navdlg.GetWindowRect(r);
	m_szMinVert = m_szVert = r.Size();
	m_szMinHorz = m_szHorz = r.Size();
	m_szMinFloat = m_szFloat = r.Size();
	m_bFixedFloat = true;
	m_szFixedFloat = r.Size();

	return TRUE;
}

BOOL CPlayerNavigationBar::PreTranslateMessage(MSG* pMsg)
{
	if (IsWindow(pMsg->hwnd) && IsVisible() && pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST) {
		if (IsDialogMessage(pMsg)) {
			return TRUE;
		}
	}

	return __super::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(CPlayerNavigationBar, CPlayerBar)
	ON_WM_SIZE()
	ON_WM_NCLBUTTONUP()
END_MESSAGE_MAP()

// CPlayerNavigationBar message handlers

void CPlayerNavigationBar::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	if (::IsWindow(m_navdlg.m_hWnd)) {
		CRect r, rectComboAudio, rectButtonInfo, rectButtonScan;
		LONG totalsize, separation, sizeComboAudio, sizeButtonInfo, sizeButtonScan;
		GetClientRect(r);
		m_navdlg.MoveWindow(r);
		r.DeflateRect(8,8,8,50);
		m_navdlg.m_ChannelList.MoveWindow(r);

		m_navdlg.m_ComboAudio.GetClientRect(rectComboAudio);
		m_navdlg.m_ButtonInfo.GetClientRect(rectButtonInfo);
		m_navdlg.m_ButtonScan.GetClientRect(rectButtonScan);
		sizeComboAudio = rectComboAudio.right - rectComboAudio.left;
		sizeButtonInfo = rectButtonInfo.right - rectButtonInfo.left;
		sizeButtonScan = rectButtonScan.right - rectButtonScan.left;
		totalsize = r.right - r.left;
		separation = (totalsize - sizeComboAudio - sizeButtonInfo - sizeButtonScan) / 2;

		if (separation < 0) {
			separation = 0;
		}

		m_navdlg.m_ComboAudio.SetWindowPos(NULL, r.left, r.bottom+6, 0,0, SWP_NOSIZE | SWP_NOZORDER);
		m_navdlg.m_ButtonInfo.SetWindowPos(NULL, r.left + sizeComboAudio + separation, r.bottom +5, 0,0, SWP_NOSIZE | SWP_NOZORDER);
		m_navdlg.m_ButtonScan.SetWindowPos(NULL, r.left + sizeComboAudio + sizeButtonInfo + 2 * separation, r.bottom +5, 0,0, SWP_NOSIZE | SWP_NOZORDER);
		m_navdlg.m_ButtonFilterStations.SetWindowPos(NULL, r.left,r.bottom +30, totalsize, 20, SWP_NOZORDER);
	}
}

void CPlayerNavigationBar::OnNcLButtonUp(UINT nHitTest, CPoint point)
{
	__super::OnNcLButtonUp(nHitTest, point);

	if (nHitTest == HTCLOSE) {
		AfxGetAppSettings().fHideNavigation = 1;
	}
}

void CPlayerNavigationBar::ShowControls(CWnd* pMainfrm, bool bShow)
{
	CSize s = this->CalcFixedLayout(FALSE, TRUE);
	((CMainFrame*)pMainfrm)->ShowControlBar(this, bShow, TRUE);

	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);
	GetWindowPlacement(&wp);

	((CMainFrame*)pMainfrm)->RecalcLayout();
}

// CPlayerNavigationDialog dialog

//IMPLEMENT_DYNAMIC(CPlayerNavigationDialog, CResizableDialog)

CPlayerNavigationDialog::CPlayerNavigationDialog()
	: CResizableDialog(CPlayerNavigationDialog::IDD, NULL)
{
}

CPlayerNavigationDialog::~CPlayerNavigationDialog()
{
}

BOOL CPlayerNavigationDialog::Create(CWnd* pParent)
{
	if (!__super::Create(IDD, pParent)) {
		return FALSE;
	}

	m_pParent = pParent;

	return TRUE;
}

void CPlayerNavigationDialog::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_LISTCHANNELS, m_ChannelList);
	DDX_Control(pDX, IDC_NAVIGATION_AUDIO, m_ComboAudio);
	DDX_Control(pDX, IDC_NAVIGATION_INFO, m_ButtonInfo);
	DDX_Control(pDX, IDC_NAVIGATION_SCAN, m_ButtonScan);
	DDX_Control(pDX, IDC_NAVIGATION_FILTERSTATIONS, m_ButtonFilterStations);
}

BOOL CPlayerNavigationDialog::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN) {
		if (pMsg->wParam == VK_RETURN) {
			CWnd* pFocused = GetFocus();

			if (pFocused && pFocused->m_hWnd == m_ChannelList.m_hWnd) {
				return TRUE;
			}
		}
	}

	return __super::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(CPlayerNavigationDialog, CResizableDialog)
	ON_WM_DESTROY()
	ON_LBN_SELCHANGE(IDC_LISTCHANNELS, OnChangeChannel)
	ON_CBN_SELCHANGE(IDC_NAVIGATION_AUDIO, OnSelChangeComboAudio)
	ON_BN_CLICKED (IDC_NAVIGATION_INFO, OnButtonInfo)
	ON_BN_CLICKED(IDC_NAVIGATION_SCAN, OnTunerScan)
	ON_BN_CLICKED(IDC_NAVIGATION_FILTERSTATIONS, OnTvRadioStations)
END_MESSAGE_MAP()

// CPlayerNavigationDialog message handlers

BOOL CPlayerNavigationDialog::OnInitDialog()
{
	__super::OnInitDialog();

	m_bTVStations = true;
	m_ButtonFilterStations.SetWindowText(ResStr(IDS_DVB_TVNAV_SEERADIO));

	return TRUE;
}

void CPlayerNavigationDialog::OnDestroy()
{
	m_ChannelList.ResetContent();

	__super::OnDestroy();
}

void CPlayerNavigationDialog::OnChangeChannel()
{
	CWnd* TempWnd;
	int nItem;

	TempWnd = static_cast<CPlayerNavigationBar*> (m_pParent) -> m_pParent;
	nItem = p_nItems[m_ChannelList.GetCurSel()] + ID_NAVIGATE_CHAP_SUBITEM_START;
	static_cast<CMainFrame*> (TempWnd) -> OnNavigateChapters(nItem);

	SetupAudioSwitcherSubMenu();
}

void CPlayerNavigationDialog::SetupAudioSwitcherSubMenu(CDVBChannel* pChannel)
{
	bool bFound = (pChannel != NULL);
	int nCurrentChannel;
	CAppSettings& s = AfxGetAppSettings();

	if (!bFound) {
		nCurrentChannel = s.nDVBLastChannel;
		POSITION	pos = s.m_DVBChannels.GetHeadPosition();

		while (pos && !bFound) {
			pChannel = &s.m_DVBChannels.GetNext(pos);

			if (nCurrentChannel == pChannel->GetPrefNumber()) {
				bFound = true;
			}
		}
	}

	if (bFound) {
		m_ButtonInfo.EnableWindow(pChannel->GetNowNextFlag());
		m_ComboAudio.ResetContent();

		for (int i=0; i < pChannel->GetAudioCount(); i++) {
			m_ComboAudio.AddString(pChannel->GetAudio(i)->Language);
			m_audios[i].PID = pChannel->GetAudio(i)-> PID;
			m_audios[i].Type = pChannel->GetAudio(i)->Type;
			m_audios[i].PesType = pChannel->GetAudio(i) -> PesType;
			m_audios[i].Language = pChannel->GetAudio(i) -> Language;
		}

		m_ComboAudio.SetCurSel(pChannel->GetDefaultAudio());
	}
}

void CPlayerNavigationDialog::UpdateElementList()
{
	CAppSettings& s = AfxGetAppSettings();

	if (s.iDefaultCaptureDevice == 1) {
		m_ChannelList.ResetContent();

		int nCurrentChannel = s.nDVBLastChannel;

		POSITION	pos = s.m_DVBChannels.GetHeadPosition();

		while (pos) {
			CDVBChannel&	Channel = s.m_DVBChannels.GetNext(pos);

			if ((m_bTVStations && (Channel.GetVideoPID() != 0)) ||
					(!m_bTVStations && (Channel.GetAudioCount() > 0)) && (Channel.GetVideoPID() == 0)) {
				int nItem = m_ChannelList.AddString (Channel.GetName());

				if (nItem < MAX_CHANNELS_ALLOWED) {
					p_nItems [nItem] = Channel.GetPrefNumber();
				}

				if (nCurrentChannel == Channel.GetPrefNumber()) {
					m_ChannelList.SetCurSel(nItem);
					SetupAudioSwitcherSubMenu(&Channel);
				}
			}
		}
	}
}

void CPlayerNavigationDialog::UpdatePos(int nID)
{
	for (int i=0; i < MAX_CHANNELS_ALLOWED; i++) {
		if (p_nItems [i] == nID) {
			m_ChannelList.SetCurSel(i);

			break;
		}
	}
}

void CPlayerNavigationDialog::OnTunerScan()
{
	CWnd* TempWnd;

	TempWnd = static_cast<CPlayerNavigationBar*> (m_pParent) -> m_pParent;
	static_cast<CMainFrame*> (TempWnd) -> OnTunerScan();

	UpdateElementList();
}

void CPlayerNavigationDialog::OnSelChangeComboAudio()
{
	UINT nID;
	CWnd* TempWnd;
	CAppSettings& s = AfxGetAppSettings();
	CDVBChannel*	 pChannel = s.FindChannelByPref(s.nDVBLastChannel);

	nID = m_ComboAudio.GetCurSel() + ID_NAVIGATE_AUDIO_SUBITEM_START;

	TempWnd = static_cast<CPlayerNavigationBar*> (m_pParent) -> m_pParent;
	static_cast<CMainFrame*>(TempWnd)->OnNavigateAudio(nID);

	pChannel->SetDefaultAudio(m_ComboAudio.GetCurSel());
	pChannel->ToString();
}

void CPlayerNavigationDialog::OnButtonInfo()
{
	CWnd* TempWnd;

	TempWnd = static_cast<CPlayerNavigationBar*> (m_pParent) -> m_pParent;
	static_cast<CMainFrame*> (TempWnd) -> DisplayCurrentChannelInfo();
}

void CPlayerNavigationDialog::OnTvRadioStations()
{
	m_bTVStations = !m_bTVStations;
	UpdateElementList();

	if (m_bTVStations) {
		m_ButtonFilterStations.SetWindowText(ResStr(IDS_DVB_TVNAV_SEERADIO));
	} else {
		m_ButtonFilterStations.SetWindowText(ResStr(IDS_DVB_TVNAV_SEETV));
	}
}

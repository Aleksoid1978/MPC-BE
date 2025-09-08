/*
 * (C) 2006-2025 see Authors.txt
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
#include <clsids.h>
#include "MainFrm.h"
#include "PPageSubtitles.h"

const static uint16_t codepages[] = {
	1250,
	1251,
	1253,
	1252,
	1254,
	1255,
	1256,
	1257,
	1258,
	1361,
	874,
	932,
	936,
	949,
	950,
	54936,
};

// CPPageSubtitles dialog

IMPLEMENT_DYNAMIC(CPPageSubtitles, CPPageBase)

CPPageSubtitles::CPPageSubtitles()
	: CPPageBase(CPPageSubtitles::IDD, CPPageSubtitles::IDD)
{
}

CPPageSubtitles::~CPPageSubtitles()
{
}

void CPPageSubtitles::DoDataExchange(CDataExchange* pDX)
{
	CPPageBase::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_COMBO2, m_cbSubtitleRenderer);
	DDX_Check(pDX, IDC_CHECK1, m_fPrioritizeExternalSubtitles);
	DDX_Check(pDX, IDC_CHECK2, m_fDisableInternalSubtitles);
	DDX_Check(pDX, IDC_CHECK3, m_fAutoReloadExtSubtitles);
	DDX_Check(pDX, IDC_CHECK_SUBRESYNC, m_fUseSybresync);
	DDX_Text(pDX, IDC_EDIT1, m_szAutoloadPaths);
	DDX_Control(pDX, IDC_COMBO3, m_cbDefaultEncoding);
	DDX_Check(pDX, IDC_CHECK5, m_bAutoDetectCodePage);
}

BOOL CPPageSubtitles::OnInitDialog()
{
	__super::OnInitDialog();

	SetCursor(m_hWnd, IDC_COMBO2, IDC_HAND);

	const CAppSettings& s = AfxGetAppSettings();

	UpdateSubRenderersList(s.iSubtitleRenderer);

	m_fPrioritizeExternalSubtitles = s.fPrioritizeExternalSubtitles;
	m_fDisableInternalSubtitles    = s.fDisableInternalSubtitles;
	m_fAutoReloadExtSubtitles      = s.fAutoReloadExtSubtitles;
	m_fUseSybresync                = s.fUseSybresync;
	m_szAutoloadPaths              = s.strSubtitlePaths;

	CStringW str;
	str.Format(L"System code page - %u", GetACP());
	AddStringData(m_cbDefaultEncoding, str, CP_ACP);

	CPINFOEX cpinfoex;
	for (const auto& codepage : codepages) {
		if (GetCPInfoExW(codepage, 0, &cpinfoex)) {
			AddStringData(m_cbDefaultEncoding, cpinfoex.CodePageName, codepage);
		}
	}
	SelectByItemData(m_cbDefaultEncoding, s.iSubtitleDefaultCodePage);

	m_bAutoDetectCodePage = s.bSubtitleAutoDetectCodePage;

	UpdateData(FALSE);

	CreateToolTip();

	return TRUE;
}

BOOL CPPageSubtitles::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	const bool fAutoReloadExtSubtitles = s.fAutoReloadExtSubtitles;

	s.iSubtitleRenderer            = m_cbSubtitleRenderer.GetCurSel();
	s.fPrioritizeExternalSubtitles = !!m_fPrioritizeExternalSubtitles;
	s.fDisableInternalSubtitles    = !!m_fDisableInternalSubtitles;
	s.fAutoReloadExtSubtitles      = !!m_fAutoReloadExtSubtitles;
	s.fUseSybresync                = !!m_fUseSybresync;
	s.strSubtitlePaths             = m_szAutoloadPaths;
	s.iSubtitleDefaultCodePage     = GetCurItemData(m_cbDefaultEncoding);
	s.bSubtitleAutoDetectCodePage  = !!m_bAutoDetectCodePage;

	if (fAutoReloadExtSubtitles != s.fAutoReloadExtSubtitles) {
		auto pFrame = AfxGetMainFrame();
		if (s.fAutoReloadExtSubtitles) {
			if (!pFrame->subChangeNotifyThread.joinable()) {
				pFrame->subChangeNotifyThreadStart();
			}
		} else {
			if (pFrame->subChangeNotifyThread.joinable()) {
				pFrame->m_EventSubChangeRefreshNotify.Reset();
				pFrame->m_EventSubChangeStopNotify.Set();
				pFrame->subChangeNotifyThread.join();
			}
		}
	}

	return __super::OnApply();
}

void CPPageSubtitles::UpdateSubRenderersList(int select)
{
	m_cbSubtitleRenderer.ResetContent();

	CAppSettings& s = AfxGetAppSettings();
	CRenderersSettings& rs = s.m_VRSettings;

	m_cbSubtitleRenderer.AddString(ResStr(IDS_SUB_NOT_USE)); // SUBRNDT_NONE

	CString str = ResStr(IDS_SUB_USE_INTERNAL);
	int iVideoRenderer = rs.iVideoRenderer;
	if (s.iSelectedVideoRenderer != -1) {
		rs.iVideoRenderer = s.iSelectedVideoRenderer;
	}
	if (!s.IsISRSelect()) {
		str += L" " + ResStr(IDS_REND_NOT_AVAILABLE);
	}
	m_cbSubtitleRenderer.AddString(str); // SUBRNDT_ISR

	bool VRwithSR =
		rs.iVideoRenderer == VIDRNDT_MADVR  ||
		rs.iVideoRenderer == VIDRNDT_EVR_CP ||
		rs.iVideoRenderer == VIDRNDT_SYNC   ||
		rs.iVideoRenderer == VIDRNDT_MPCVR;

	str = L"VSFilter/xy-VSFilter";
	if (!IsCLSIDRegistered(CLSID_VSFilter_autoloading)) {
		str += L" " + ResStr(IDS_REND_NOT_INSTALLED);
	}
	m_cbSubtitleRenderer.AddString(str); // SUBRNDT_VSFILTER

	str = L"XySubFilter";
	if (!IsCLSIDRegistered(CLSID_XySubFilter_AutoLoader)) {
		str += L" " + ResStr(IDS_REND_NOT_INSTALLED);
	} else if (!VRwithSR) {
		str += L" " + ResStr(IDS_REND_NOT_AVAILABLE);
	}
	m_cbSubtitleRenderer.AddString(str); // SUBRNDT_XYSUBFILTER

#if ENABLE_ASSFILTERMOD
	str = L"AssFilterMod (" + ResStr(IDS_AG_NOTRECOMMENDED) + ")";
	if (!IsCLSIDRegistered(CLSID_AssFilterMod)) {
		str += L" " + ResStr(IDS_REND_NOT_INSTALLED);
	}
	else if (!VRwithSR) {
		str += L" " + ResStr(IDS_REND_NOT_AVAILABLE);
	}
	m_cbSubtitleRenderer.AddString(str); // SUBRNDT_ASSFILTERMOD
#endif

	if (m_cbSubtitleRenderer.SetCurSel(select) == CB_ERR) {
		m_cbSubtitleRenderer.SetCurSel(1);
	}

	rs.iVideoRenderer = iVideoRenderer;
}

BEGIN_MESSAGE_MAP(CPPageSubtitles, CPPageBase)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
	ON_CBN_SELCHANGE(IDC_COMBO2, OnSubRendModified)
	ON_UPDATE_COMMAND_UI(IDC_CHECK2, OnUpdateISRSelect)
	ON_UPDATE_COMMAND_UI(IDC_CHECK3, OnUpdateISRSelect)
	ON_UPDATE_COMMAND_UI(IDC_CHECK_SUBRESYNC, OnUpdateISRSelect)
END_MESSAGE_MAP()

void CPPageSubtitles::OnBnClickedButton1()
{
	m_szAutoloadPaths = DEFAULT_SUBTITLE_PATHS;

	UpdateData(FALSE);

	SetModified();
}

void CPPageSubtitles::OnSubRendModified()
{
	SetModified();
}

void CPPageSubtitles::OnUpdateISRSelect(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_cbSubtitleRenderer.GetCurSel() == SUBRNDT_ISR);
}

BOOL CPPageSubtitles::OnSetActive()
{
	UpdateSubRenderersList(m_cbSubtitleRenderer.GetCurSel());

	return CPPageBase::OnSetActive();
}

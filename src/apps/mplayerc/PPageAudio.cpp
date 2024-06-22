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
#include "filters/renderer/MpcAudioRenderer/MpcAudioRenderer.h"
#include "ComPropertySheet.h"
#include "PPageAudio.h"
#include "MainFrm.h"

// CPPageAudio dialog

IMPLEMENT_DYNAMIC(CPPageAudio, CPPageBase)
CPPageAudio::CPPageAudio()
	: CPPageBase(CPPageAudio::IDD, CPPageAudio::IDD)
{
}

CPPageAudio::~CPPageAudio()
{
}

void CPPageAudio::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_AUDRND_COMBO, m_iAudioRendererTypeCtrl);
	DDX_Control(pDX, IDC_COMBO1, m_iSecAudioRendererTypeCtrl);
	DDX_CBIndex(pDX, IDC_AUDRND_COMBO, m_iAudioRendererType);
	DDX_CBIndex(pDX, IDC_COMBO1, m_iSecAudioRendererType);
	DDX_Control(pDX, IDC_BUTTON1, m_audRendPropButton);
	DDX_Control(pDX, IDC_CHECK1, m_DualAudioOutput);

	DDX_Control(pDX, IDC_SLIDER1, m_volumectrl);
	DDX_Control(pDX, IDC_SLIDER2, m_balancectrl);
	DDX_Slider(pDX, IDC_SLIDER1, m_nVolume);
	DDX_Slider(pDX, IDC_SLIDER2, m_nBalance);

	DDX_Check(pDX, IDC_CHECK2, m_fAutoloadAudio);
	DDX_Text(pDX, IDC_EDIT4, m_sAudioPaths);
	DDX_Check(pDX, IDC_CHECK3, m_fPrioritizeExternalAudio);
}

BEGIN_MESSAGE_MAP(CPPageAudio, CPPageBase)
	ON_WM_HSCROLL()
	ON_CBN_SELCHANGE(IDC_AUDRND_COMBO, OnAudioRendererChange)
	ON_BN_CLICKED(IDC_BUTTON1, OnAudioRenderPropClick)
	ON_BN_CLICKED(IDC_CHECK1, OnDualAudioOutputCheck)
	ON_STN_DBLCLK(IDC_STATIC_BALANCE, OnBalanceTextDblClk)
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedResetAudioPaths)
	ON_NOTIFY_EX(TTN_NEEDTEXTW, 0, OnToolTipNotify)
END_MESSAGE_MAP()

// CPPageAudio message handlers

BOOL CPPageAudio::OnInitDialog()
{
	__super::OnInitDialog();

	SetCursor(m_hWnd, IDC_AUDRND_COMBO, IDC_HAND);

	CAppSettings& s = AfxGetAppSettings();
	CString str;

	m_iAudioRendererTypeCtrl.SetRedraw(FALSE);
	m_iSecAudioRendererTypeCtrl.SetRedraw(FALSE);

	// MPC Audio Renderer
	m_AudioRendererDisplayNames.Add(AUDRNDT_MPC);
	str.Format(L"0. %s", AUDRNDT_MPC);
	m_iAudioRendererTypeCtrl.AddString(str);
	m_iSecAudioRendererTypeCtrl.AddString(str);

	// Default DirectSound Device
	m_AudioRendererDisplayNames.Add(L"");
	str.Format(L"1. DirectSound: %s", ResStr(IDS_PPAGE_OUTPUT_SYS_DEF));
	m_iAudioRendererTypeCtrl.AddString(str);
	m_iSecAudioRendererTypeCtrl.AddString(str);

	int i = 2;

	// other DirectSound devices
	BeginEnumSysDev(CLSID_AudioRendererCategory, pMoniker) {
		LPOLESTR olestr = nullptr;
		if (FAILED(pMoniker->GetDisplayName(0, 0, &olestr))) {
			continue;
		}

		CComPtr<IPropertyBag> pPB;
		if (SUCCEEDED(pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB))) {
			CComVariant var;
			if (pPB->Read(CComBSTR(L"FriendlyName"), &var, nullptr) == S_OK) {
				CStringW fname = var.bstrVal;

				// skip WaveOut
				var.Clear();
				if (pPB->Read(CComBSTR(L"WaveOutId"), &var, nullptr) == S_OK) {
					CoTaskMemFree(olestr);
					continue;
				}

				// skip Default DirectSound Device
				var.Clear();
				if (pPB->Read(CComBSTR(L"DSGuid"), &var, nullptr) == S_OK
						&& CString(var.bstrVal) == "{00000000-0000-0000-0000-000000000000}") {
					CoTaskMemFree(olestr);
					continue;
				}

				m_AudioRendererDisplayNames.Add(CString(olestr));
				str.Format(L"%d. %s", i++, fname);
				m_iAudioRendererTypeCtrl.AddString(str);
				m_iSecAudioRendererTypeCtrl.AddString(str);
			}
		}
		CoTaskMemFree(olestr);
	}
	EndEnumSysDev;

	// Null Audio Renderer (Any)
	m_AudioRendererDisplayNames.Add(AUDRNDT_NULL_COMP);
	str.Format(L"%d. %s", i++, AUDRNDT_NULL_COMP);
	m_iAudioRendererTypeCtrl.AddString(str);
	m_iSecAudioRendererTypeCtrl.AddString(str);

	// Null Audio Renderer (Uncompressed)
	m_AudioRendererDisplayNames.Add(AUDRNDT_NULL_UNCOMP);
	str.Format(L"%d. %s", i++, AUDRNDT_NULL_UNCOMP);
	m_iAudioRendererTypeCtrl.AddString(str);
	m_iSecAudioRendererTypeCtrl.AddString(str);

	m_iAudioRendererType = 0;
	m_iSecAudioRendererType = 1;
	for (INT_PTR idx = 0; idx < m_AudioRendererDisplayNames.GetCount(); idx++) {
		if (s.strAudioRendererDisplayName == m_AudioRendererDisplayNames[idx]) {
			m_iAudioRendererType = idx;
		}
		if (s.strAudioRendererDisplayName2 == m_AudioRendererDisplayNames[idx]) {
			m_iSecAudioRendererType = idx;
		}
	}

	CorrectComboListWidth(m_iAudioRendererTypeCtrl);
	m_iAudioRendererTypeCtrl.SetRedraw(TRUE);
	m_iAudioRendererTypeCtrl.Invalidate();
	m_iAudioRendererTypeCtrl.UpdateWindow();

	CorrectComboListWidth(m_iSecAudioRendererTypeCtrl);
	m_iSecAudioRendererTypeCtrl.SetRedraw(TRUE);
	m_iSecAudioRendererTypeCtrl.Invalidate();
	m_iSecAudioRendererTypeCtrl.UpdateWindow();

	m_DualAudioOutput.SetCheck(s.fDualAudioOutput);
	OnDualAudioOutputCheck();

	m_volumectrl.SetRange(0, 100);
	m_volumectrl.SetTicFreq(10);
	m_balancectrl.SetRange(-100, 100);
	m_balancectrl.SetLineSize(2);
	m_balancectrl.SetPageSize(2);
	m_balancectrl.SetTicFreq(20);
	m_nVolume = m_oldVolume = s.nVolume;
	m_nBalance = s.nBalance;

	m_fAutoloadAudio           = s.fAutoloadAudio;
	m_fPrioritizeExternalAudio = s.fPrioritizeExternalAudio;
	m_sAudioPaths              = s.strAudioPaths;

	UpdateData(FALSE);

	OnAudioRendererChange();

	CreateToolTip();

	return TRUE;
}

BOOL CPPageAudio::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	s.strAudioRendererDisplayName  = m_AudioRendererDisplayNames[m_iAudioRendererType];
	if (m_iSecAudioRendererType == -1) {
		s.strAudioRendererDisplayName.Empty();
	} else {
		s.strAudioRendererDisplayName2 = m_AudioRendererDisplayNames[m_iSecAudioRendererType];
	}
	s.fDualAudioOutput             = !!m_DualAudioOutput.GetCheck();

	s.nVolume = m_oldVolume = m_nVolume;
	s.nBalance = m_nBalance;

	s.fAutoloadAudio = !!m_fAutoloadAudio;
	s.fPrioritizeExternalAudio = !!m_fPrioritizeExternalAudio;
	s.strAudioPaths = m_sAudioPaths;

	return __super::OnApply();
}

void CPPageAudio::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (*pScrollBar == m_volumectrl) {
		UpdateData();
		AfxGetMainFrame()->m_wndToolBar.Volume = m_nVolume;
	}
	else if (*pScrollBar == m_balancectrl) {
		UpdateData();
		AfxGetMainFrame()->SetBalance(m_nBalance);
	}

	SetModified();

	__super::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CPPageAudio::ShowPPage(CUnknown* (WINAPI * CreateInstance)(LPUNKNOWN lpunk, HRESULT* phr))
{
	if (!CreateInstance) {
		return;
	}

	HRESULT hr;
	CUnknown* pObj = CreateInstance(nullptr, &hr);

	if (!pObj) {
		return;
	}

	CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)pObj;

	if (SUCCEEDED(hr)) {
		if (CComQIPtr<ISpecifyPropertyPages> pSPP = pUnk.p) {
			CComPropertySheet ps(ResStr(IDS_PROPSHEET_PROPERTIES), this);
			ps.AddPages(pSPP);
			ps.DoModal();
		}
	}
}

void CPPageAudio::OnAudioRendererChange()
{
	UpdateData();

	BOOL flag = FALSE;
	CString str_audio = m_AudioRendererDisplayNames[m_iAudioRendererType];
	if (str_audio == AUDRNDT_MPC) {
		flag = TRUE;
	} else {
		BeginEnumSysDev(CLSID_AudioRendererCategory, pMoniker) {
			LPOLESTR olestr = nullptr;
			if (FAILED(pMoniker->GetDisplayName(0, 0, &olestr))) {
				continue;
			}

			CStringW str(olestr);
			CoTaskMemFree(olestr);

			if (str == m_AudioRendererDisplayNames[m_iAudioRendererType]) {
				CComPtr<IBaseFilter> pBF;
				HRESULT hr = pMoniker->BindToObject(nullptr, nullptr, IID_PPV_ARGS(&pBF));
				if (SUCCEEDED(hr)) {
					if (CComQIPtr<ISpecifyPropertyPages> pSPP = pBF.p) {
						flag = TRUE;
						break;
					}
				}
			}
		}
		EndEnumSysDev
	}

	m_audRendPropButton.EnableWindow(flag);

	SetModified();
}

void CPPageAudio::OnAudioRenderPropClick()
{
	CString str_audio = m_AudioRendererDisplayNames[m_iAudioRendererType];

	if (str_audio == AUDRNDT_MPC) {
		ShowPPage(CreateInstance<CMpcAudioRenderer>);
	} else {
		BeginEnumSysDev(CLSID_AudioRendererCategory, pMoniker) {
			LPOLESTR olestr = nullptr;
			if (FAILED(pMoniker->GetDisplayName(0, 0, &olestr))) {
				continue;
			}

			CStringW str(olestr);
			CoTaskMemFree(olestr);

			if (str == str_audio) {
				CComPtr<IBaseFilter> pBF;
				HRESULT hr = pMoniker->BindToObject(nullptr, nullptr, IID_PPV_ARGS(&pBF));
				if (SUCCEEDED(hr)) {
					ISpecifyPropertyPages *pProp = nullptr;
					hr = pBF->QueryInterface(IID_PPV_ARGS(&pProp));
					if (SUCCEEDED(hr)) {
						// Get the filter's name and IUnknown pointer.
						FILTER_INFO FilterInfo;
						hr = pBF->QueryFilterInfo(&FilterInfo);
						if (SUCCEEDED(hr)) {
							IUnknown *pFilterUnk;
							hr = pBF->QueryInterface(IID_PPV_ARGS(&pFilterUnk));
							if (SUCCEEDED(hr)) {

								// Show the page.
								CAUUID caGUID;
								pProp->GetPages(&caGUID);
								pProp->Release();

								OleCreatePropertyFrame(
									this->m_hWnd,			// Parent window
									0, 0,					// Reserved
									FilterInfo.achName,		// Caption for the dialog box
									1,						// Number of objects (just the filter)
									&pFilterUnk,			// Array of object pointers.
									caGUID.cElems,			// Number of property pages
									caGUID.pElems,			// Array of property page CLSIDs
									0,						// Locale identifier
									0, nullptr				// Reserved
								);

								// Clean up.
								CoTaskMemFree(caGUID.pElems);
								pFilterUnk->Release();
							}
							if (FilterInfo.pGraph) {
								FilterInfo.pGraph->Release();
							}
						}
					}
				}
				break;
			}
		}
		EndEnumSysDev
	}
}

void CPPageAudio::OnDualAudioOutputCheck()
{
	m_iSecAudioRendererTypeCtrl.EnableWindow(!!m_DualAudioOutput.GetCheck());

	SetModified();
}

void CPPageAudio::OnBalanceTextDblClk()
{
	// double click on text "Balance" resets the balance to zero
	m_nBalance = 0;
	m_balancectrl.SetPos(m_nBalance);

	AfxGetMainFrame()->SetBalance(m_nBalance);

	SetModified();
}

void CPPageAudio::OnBnClickedResetAudioPaths()
{
	m_sAudioPaths = DEFAULT_AUDIO_PATHS;

	UpdateData(FALSE);

	SetModified();
}

void CPPageAudio::OnCancel()
{
	CAppSettings& s = AfxGetAppSettings();

	if (m_nVolume != m_oldVolume) {
		AfxGetMainFrame()->m_wndToolBar.Volume = m_oldVolume;    //not very nice solution
	}

	if (m_nBalance != s.nBalance) {
		AfxGetMainFrame()->SetBalance(s.nBalance);
	}

	__super::OnCancel();
}

BOOL CPPageAudio::OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
	TOOLTIPTEXTW* pTTT = (TOOLTIPTEXTW*)pNMHDR;

	UINT_PTR nID = pNMHDR->idFrom;

	if (pTTT->uFlags & TTF_IDISHWND) {
		nID = ::GetDlgCtrlID((HWND)nID);
	}

	if (nID == 0) {
		return FALSE;
	}

	static CString strTipText;

	if (nID == IDC_SLIDER1) {
		strTipText.Format(L"%d%%", m_nVolume);
	}
	else if (nID == IDC_SLIDER2) {
		if (m_nBalance > 0) {
			strTipText.Format(L"R +%d%%", m_nBalance);
		}
		else if (m_nBalance < 0) {
			strTipText.Format(L"L +%d%%", -m_nBalance);
		}
		else { //if (m_nBalance == 0)
			strTipText = L"L = R";
		}
	}
	else {
		return FALSE;
	}

	pTTT->lpszText = (LPWSTR)(LPCWSTR)strTipText;

	*pResult = 0;

	return TRUE;
}


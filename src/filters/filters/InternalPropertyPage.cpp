/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2017 see Authors.txt
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
#include "InternalPropertyPage.h"
#include "../../DSUtil/DSUtil.h"
#include "../../apps/mplayerc/mplayerc.h"

#define DEFAULT_FONTSIZE 13

//
// CInternalPropertyPageWnd
//

CInternalPropertyPageWnd::CInternalPropertyPageWnd()
	: m_fDirty(false)
	, m_fontheight(DEFAULT_FONTSIZE)
{

}

BOOL CInternalPropertyPageWnd::Create(IPropertyPageSite* pPageSite, LPCRECT pRect, CWnd* pParentWnd)
{
	if (!pPageSite || !pRect) {
		return FALSE;
	}

	m_pPageSite = pPageSite;

	LPCTSTR wc = AfxRegisterWndClass(CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS, 0, (HBRUSH)(COLOR_BTNFACE + 1));
	if (!CreateEx(0, wc, L"CInternalPropertyPageWnd", WS_CHILDWINDOW, *pRect, pParentWnd, 0)) {
		return FALSE;
	}

	UseCurentMonitorDPI(m_hWnd);

	if (!m_font.m_hObject) {
		CString face;
		WORD height;
		extern BOOL AFXAPI AfxGetPropSheetFont(CString& strFace, WORD& wSize, BOOL bWizard); // yay
		if (!AfxGetPropSheetFont(face, height, FALSE)) {
			return FALSE;
		}

		LOGFONT lf = { 0 };
		wcscpy_s(lf.lfFaceName, face);
		lf.lfHeight = -PointsToPixels(height);
		lf.lfWeight = FW_NORMAL;
		lf.lfCharSet = DEFAULT_CHARSET;
		if (!m_font.CreateFontIndirect(&lf)) {
			return FALSE;
		}

		lf.lfHeight -= -1;
		wcscpy_s(lf.lfFaceName, L"Lucida Console");
		if (!m_monospacefont.CreateFontIndirect(&lf)) {
			wcscpy_s(lf.lfFaceName, L"Courier New");
			if (!m_monospacefont.CreateFontIndirect(&lf)) {
				return FALSE;
			}
		}

		HDC hDC = ::GetDC(nullptr);
		HFONT hFontOld = (HFONT)SelectObject(hDC, m_font.m_hObject);
		CSize size;
		GetTextExtentPoint32(hDC, L"x", 1, &size);
		SelectObject(hDC, hFontOld);
		::ReleaseDC(nullptr, hDC);

		m_fontheight = size.cy;
	}

	SetFont(&m_font);

	return TRUE;
}

BOOL CInternalPropertyPageWnd::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	if (message == WM_COMMAND || message == WM_HSCROLL || message == WM_VSCROLL) {
		WORD notify = HIWORD(wParam);
		// check only notifications that change the state of a control, otherwise false positives are possible.
		if (notify == BN_CLICKED
				|| notify == CBN_SELCHANGE
				|| notify == EN_CHANGE
				|| notify == CLBN_CHKCHANGE) {
			SetDirty(true);
		}
	}

	return __super::OnWndMsg(message, wParam, lParam, pResult);
}

BEGIN_MESSAGE_MAP(CInternalPropertyPageWnd, CWnd)
END_MESSAGE_MAP()

//
// CInternalPropertyPage
//

CInternalPropertyPage::CInternalPropertyPage(LPUNKNOWN lpunk, HRESULT* phr)
	: CUnknown(L"CInternalPropertyPage", lpunk)
	, m_pWnd(nullptr)
{
	if (phr) {
		*phr = S_OK;
	}
}

CInternalPropertyPage::~CInternalPropertyPage()
{
	if (m_pWnd) {
		if (m_pWnd->m_hWnd) {
			ASSERT(0);
			m_pWnd->DestroyWindow();
		}
		delete m_pWnd;
		m_pWnd = nullptr;
	}
}

STDMETHODIMP CInternalPropertyPage::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI2(IPropertyPage)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IPropertyPage

STDMETHODIMP CInternalPropertyPage::SetPageSite(IPropertyPageSite* pPageSite)
{
	CAutoLock cAutoLock(this);

	if (pPageSite && m_pPageSite || !pPageSite && !m_pPageSite) {
		return E_UNEXPECTED;
	}

	m_pPageSite = pPageSite;

	return S_OK;
}

STDMETHODIMP CInternalPropertyPage::Activate(HWND hwndParent, LPCRECT pRect, BOOL fModal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CAutoLock cAutoLock(this);

	CheckPointer(pRect, E_POINTER);

	if (!m_pWnd || m_pWnd->m_hWnd || m_pUnks.IsEmpty()) {
		return E_UNEXPECTED;
	}

	if (!m_pWnd->Create(m_pPageSite, pRect, CWnd::FromHandle(hwndParent))) {
		return E_OUTOFMEMORY;
	}

	if (!m_pWnd->OnActivate()) {
		m_pWnd->DestroyWindow();
		return E_FAIL;
	}

	m_pWnd->ModifyStyleEx(WS_EX_DLGMODALFRAME, WS_EX_CONTROLPARENT);
	m_pWnd->ShowWindow(SW_SHOWNORMAL);

	return S_OK;
}

STDMETHODIMP CInternalPropertyPage::Deactivate()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CAutoLock cAutoLock(this);

	if (!m_pWnd || !m_pWnd->m_hWnd) {
		return E_UNEXPECTED;
	}

	m_pWnd->OnDeactivate();

	m_pWnd->ModifyStyleEx(WS_EX_CONTROLPARENT, 0);
	m_pWnd->DestroyWindow();

	return S_OK;
}

STDMETHODIMP CInternalPropertyPage::GetPageInfo(PROPPAGEINFO* pPageInfo)
{
	CAutoLock cAutoLock(this);

	CheckPointer(pPageInfo, E_POINTER);

	LPOLESTR pszTitle;
	HRESULT hr = AMGetWideString(CStringW(GetWindowTitle()), &pszTitle);
	if (FAILED(hr)) {
		return hr;
	}

	pPageInfo->cb = sizeof(PROPPAGEINFO);
	pPageInfo->pszTitle = pszTitle;
	pPageInfo->pszDocString = nullptr;
	pPageInfo->pszHelpFile = nullptr;
	pPageInfo->dwHelpContext = 0;
	pPageInfo->size = GetWindowSize();

	return S_OK;
}

STDMETHODIMP CInternalPropertyPage::SetObjects(ULONG cObjects, LPUNKNOWN* ppUnk)
{
	CAutoLock cAutoLock(this);

	if (cObjects && m_pWnd || !cObjects && !m_pWnd) {
		return E_UNEXPECTED;
	}

	m_pUnks.RemoveAll();

	if (cObjects > 0) {
		CheckPointer(ppUnk, E_POINTER);

		for (ULONG i = 0; i < cObjects; i++) {
			m_pUnks.AddTail(ppUnk[i]);
		}

		m_pWnd = GetWindow();
		if (!m_pWnd) {
			return E_OUTOFMEMORY;
		}

		if (!m_pWnd->OnConnect(m_pUnks)) {
			delete m_pWnd;
			m_pWnd = nullptr;

			return E_FAIL;
		}
	} else {
		m_pWnd->OnDisconnect();

		m_pWnd->DestroyWindow();
		delete m_pWnd;
		m_pWnd = nullptr;
	}

	return S_OK;
}

STDMETHODIMP CInternalPropertyPage::Show(UINT nCmdShow)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CAutoLock cAutoLock(this);

	if (!m_pWnd) {
		return E_UNEXPECTED;
	}

	if ((nCmdShow != SW_SHOW) && (nCmdShow != SW_SHOWNORMAL) && (nCmdShow != SW_HIDE)) {
		return E_INVALIDARG;
	}

	m_pWnd->ShowWindow(nCmdShow);
	m_pWnd->Invalidate();

	return S_OK;
}

STDMETHODIMP CInternalPropertyPage::Move(LPCRECT pRect)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CAutoLock cAutoLock(this);

	CheckPointer(pRect, E_POINTER);

	if (!m_pWnd) {
		return E_UNEXPECTED;
	}

	m_pWnd->MoveWindow(pRect, TRUE);

	return S_OK;
}

STDMETHODIMP CInternalPropertyPage::IsPageDirty()
{
	CAutoLock cAutoLock(this);

	return m_pWnd && m_pWnd->GetDirty() ? S_OK : S_FALSE;
}

STDMETHODIMP CInternalPropertyPage::Apply()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CAutoLock cAutoLock(this);

	if (!m_pWnd || m_pUnks.IsEmpty() || !m_pPageSite) {
		return E_UNEXPECTED;
	}

	if (m_pWnd->GetDirty() && m_pWnd->OnApply()) {
		m_pWnd->SetDirty(false);
	}

	return S_OK;
}

STDMETHODIMP CInternalPropertyPage::Help(LPCWSTR lpszHelpDir)
{
	CAutoLock cAutoLock(this);

	return E_NOTIMPL;
}

STDMETHODIMP CInternalPropertyPage::TranslateAccelerator(LPMSG lpMsg)
{
	CAutoLock cAutoLock(this);

	return E_NOTIMPL;
}

//
// CPinInfoWnd
//

CAtlMap<CLSID, CString> CPinInfoWnd::m_CachedRegistryFilters;

CPinInfoWnd::CPinInfoWnd()
{
	CAppSettings& s = AfxGetAppSettings();
	POSITION pos = s.m_filters.GetHeadPosition();
	while (pos) {
		CAutoPtr<FilterOverride> f(DNew FilterOverride(s.m_filters.GetNext(pos)));
		if (::PathFileExistsW(f->path)) {
			m_CachedExternalFilters[f->clsid] = f->path;
		}
	}
}

bool CPinInfoWnd::OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks)
{
	ASSERT(!m_pBF);

	m_pBF.Release();

	POSITION pos = pUnks.GetHeadPosition();
	while (pos && !(m_pBF = pUnks.GetNext(pos))) {
		;
	}

	if (!m_pBF) {
		return false;
	}

	return true;
}

void CPinInfoWnd::OnDisconnect()
{
	m_pBF.Release();
}

static WNDPROC OldControlProc;

static LRESULT CALLBACK ControlProc(HWND control, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_KEYDOWN) {
		if (LOWORD(wParam) == VK_ESCAPE) {
			return 0;    // just ignore ESCAPE in edit control
		}
		if ((LOWORD(wParam)== 'A' || LOWORD(wParam) == 'a')
				&&(GetKeyState(VK_CONTROL) < 0)) {
			CEdit *pEdit = (CEdit*)CWnd::FromHandle(control);
			pEdit->SetSel(0, pEdit->GetWindowTextLength(),TRUE);
			return 0;
		}
	}

	return CallWindowProc(OldControlProc, control, message, wParam, lParam); // call edit control's own windowproc
}

bool CPinInfoWnd::OnActivate()
{
	DWORD dwStyle = WS_VISIBLE | WS_CHILD | WS_TABSTOP;

	CPoint p(ScaleX(10), ScaleY(10));

	m_pin_static.Create(L"Pin:",
						dwStyle,
						CRect(p + CPoint(0, ScaleY(3)), CSize(ScaleX(30), m_fontheight)),
						this);
	m_pin_combo.Create(dwStyle | CBS_DROPDOWNLIST,
					   CRect(p + CPoint(ScaleX(30), 0), CSize(ScaleX(500), ScaleY(200))),
					   this,
					   IDC_PP_COMBO1);
	BeginEnumPins(m_pBF, pEP, pPin) {
		CPinInfo pi;
		if (FAILED(pPin->QueryPinInfo(&pi))) {
			continue;
		}
		CString str = CString(pi.achName);
		if (!str.Find(L"Apple")) {
			str.Delete(0, 1);
		}
		CString dir = L"[?] ";
		if (pi.dir == PINDIR_INPUT) {
			dir = L"[IN] ";
		} else if (pi.dir == PINDIR_OUTPUT) {
			dir = L"[OUT] ";
		}
		m_pin_combo.SetItemDataPtr(m_pin_combo.AddString(dir + str), pPin);
	}
	EndEnumPins
	m_pin_combo.SetCurSel(0);

	p.y += m_fontheight + ScaleX(20);

	m_info_edit.CreateEx(WS_EX_CLIENTEDGE,
						 L"EDIT",
						 L"",
						 dwStyle | WS_BORDER | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOHSCROLL | ES_READONLY,
						 CRect(p, CSize(ScaleX(530), m_fontheight * 23)),
						 this,
						 IDC_PP_EDIT1);
	m_info_edit.SetLimitText(60000);

	for (CWnd* pWnd = GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetNextWindow()) {
		pWnd->SetFont(&m_font, FALSE);
	}

	m_info_edit.SetFont(&m_monospacefont);

	// subclass the edit control
	OldControlProc = (WNDPROC)SetWindowLongPtrW(m_info_edit.m_hWnd, GWLP_WNDPROC, (LONG_PTR)ControlProc);

	OnCbnSelchangeCombo1();

	return true;
}

void CPinInfoWnd::OnDeactivate()
{
}

bool CPinInfoWnd::OnApply()
{
	OnDeactivate();

	if (m_pBF) {
	}

	return true;
}

BOOL CPinInfoWnd::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	if (message == WM_WINDOWPOSCHANGING) {
		OnCbnSelchangeCombo1();
	}
	SetDirty(false);

	return __super::OnWndMsg(message, wParam, lParam, pResult);
}

BEGIN_MESSAGE_MAP(CPinInfoWnd, CInternalPropertyPageWnd)
	ON_CBN_SELCHANGE(IDC_PP_COMBO1, OnCbnSelchangeCombo1)
END_MESSAGE_MAP()

void CPinInfoWnd::OnCbnSelchangeCombo1()
{
	m_info_edit.SetWindowTextW(L"");
	CString infoStr;

	int i = m_pin_combo.GetCurSel();
	if (i < 0) {
		return;
	}

	CComPtr<IPin> pPin = (IPin*)m_pin_combo.GetItemDataPtr(i);
	if (!pPin) {
		return;
	}

	PIN_INFO PinInfo;
	if (SUCCEEDED (pPin->QueryPinInfo(&PinInfo))) {
		FILTER_INFO FilterInfo;

		if (SUCCEEDED (PinInfo.pFilter->QueryFilterInfo(&FilterInfo)) && FilterInfo.pGraph) {
			CString strName;

			CLSID FilterClsid;
			PinInfo.pFilter->GetClassID(&FilterClsid);
			CString clsid = CStringFromGUID(FilterClsid);

			WCHAR buff[MAX_PATH] = { 0 };
			ULONG len = _countof(buff);
			CRegKey key;
			if (ERROR_SUCCESS == key.Open (HKEY_CLASSES_ROOT, L"CLSID\\{083863F1-70DE-11D0-BD40-00A0C911CE86}\\Instance\\" + clsid, KEY_READ)
					&& ERROR_SUCCESS == key.QueryStringValue(L"FriendlyName", buff, &len)) {
				strName = CString(buff);
				key.Close();
			} else {
				strName = FilterInfo.achName;
			}
			infoStr.AppendFormat(L"Filter : %s - CLSID : %s\r\n", strName, CStringFromGUID(FilterClsid));
			FilterInfo.pGraph->Release();

			{
				CString module;

				CachedFilters::CPair* pRegPair = m_CachedRegistryFilters.Lookup(FilterClsid);
				if (pRegPair && ::PathFileExistsW(pRegPair->m_value)) {
					module = pRegPair->m_value;
				} else {
					len = _countof(buff);
					memset(buff, 0, len);
					if (ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, L"CLSID\\" + clsid + L"\\InprocServer32", KEY_READ)
							&& ERROR_SUCCESS == key.QueryStringValue(nullptr, buff, &len)
							&& ::PathFileExistsW(buff)) {
						module = CString(buff);
						m_CachedRegistryFilters[FilterClsid] = module;
						key.Close();
					} else if (CachedFilters::CPair* pExtPair = m_CachedExternalFilters.Lookup(FilterClsid)) {
						module = pExtPair->m_value;
					}
				}

				if (!module.IsEmpty()) {
					infoStr.AppendFormat(L"Module : %s\r\n", module);
				}
			}

			infoStr.Append(L"\r\n");
		}
		PinInfo.pFilter->Release();
	}

	CMediaTypeEx cmt;

	CComPtr<IPin> pPinTo;
	if (SUCCEEDED(pPin->ConnectedTo(&pPinTo)) && pPinTo) {
		infoStr.AppendFormat(L"- Connected to:\r\n\r\nCLSID: %s\r\nFilter: %s\r\nPin: %s\r\n\r\n",
				   CString(CStringFromGUID(GetCLSID(pPinTo))),
				   CString(GetFilterName(GetFilterFromPin(pPinTo))),
				   CString(GetPinName(pPinTo)));

		infoStr.Append(L"- Connection media type:\r\n\r\n");

		if (SUCCEEDED(pPin->ConnectionMediaType(&cmt))) {
			std::list<CString> sl;
			cmt.Dump(sl);
			for (const auto& line : sl) {
				infoStr.Append(line + L"\r\n");
			}
		}
	} else {
		infoStr.Append(L"- Not connected\r\n\r\n");
	}

	int iMT = 0;

	BeginEnumMediaTypes(pPin, pEMT, pmt) {
		CMediaTypeEx mt(*pmt);

		infoStr.AppendFormat(L"- Enumerated media type %d:\r\n\r\n", iMT++);

		if (cmt.majortype != GUID_NULL && mt == cmt) {
			infoStr.AppendFormat(L"Set as the current media type\r\n\r\n");
		} else {
			std::list<CString> sl;
			mt.Dump(sl);
			for (const auto& line : sl) {
				infoStr.Append(line + L"\r\n");
			}
		}
	}
	EndEnumMediaTypes(pmt);

	m_info_edit.SetWindowTextW(infoStr);
}

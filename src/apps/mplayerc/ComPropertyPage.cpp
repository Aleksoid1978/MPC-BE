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
#include "ComPropertyPage.h"
#include "../../filters/filters/InternalPropertyPage.h"

// CComPropertyPage dialog

IMPLEMENT_DYNAMIC(CComPropertyPage, CPropertyPage)
CComPropertyPage::CComPropertyPage(IPropertyPage* pPage)
	: CPropertyPage(CComPropertyPage::IDD), m_pPage(pPage)
{
	PROPPAGEINFO ppi;
	m_pPage->GetPageInfo(&ppi);
	m_pPSP->pszTitle = (m_strCaption = ppi.pszTitle);
	m_psp.dwFlags |= PSP_USETITLE;
}

CComPropertyPage::~CComPropertyPage()
{
}

void CComPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

BOOL CComPropertyPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	CRect r;
	PROPPAGEINFO ppi;
	m_pPage->GetPageInfo(&ppi);
	r = CRect(CPoint(0,0), ppi.size);
	m_pPage->Activate(m_hWnd, r, FALSE);
	m_pPage->Show(SW_SHOW);

	return TRUE;
}

void CComPropertyPage::OnDestroy()
{
	CPropertyPage::OnDestroy();

	m_pPage->Deactivate();
}

BOOL CComPropertyPage::OnSetActive()
{
	SetModified(S_OK == m_pPage->IsPageDirty());

	CWnd* pParent = GetParent();

	if (pParent->IsKindOf(RUNTIME_CLASS(CComPropertySheet))) {
		CComPropertySheet* pSheet = static_cast<CComPropertySheet*>(pParent);
		pSheet->OnActivated(this);
	}

	return CPropertyPage::OnSetActive();
}

BOOL CComPropertyPage::OnKillActive()
{
	SetModified(FALSE);

	return CPropertyPage::OnKillActive();
}

BEGIN_MESSAGE_MAP(CComPropertyPage, CPropertyPage)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

// CComPropertyPage message handlers

void CComPropertyPage::OnOK()
{
	if (S_OK == m_pPage->IsPageDirty()) {
		m_pPage->Apply();
	}

	SetModified(FALSE);

	CPropertyPage::OnOK();
}

// CComPropertyPageSite

class CComPropertyPageSite : public CUnknown, public IPropertyPageSite
{
	IComPropertyPageDirty* m_pPPD;

public:
	CComPropertyPageSite(IComPropertyPageDirty* pPPD) : CUnknown(NAME("CComPropertyPageSite"), nullptr), m_pPPD(pPPD) {}

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv) {
		return
			QI(IPropertyPageSite)
			__super::NonDelegatingQueryInterface(riid, ppv);
	}

	// IPropertyPageSite
	STDMETHODIMP OnStatusChange(DWORD flags) {
		if (m_pPPD) {
			if (flags&PROPPAGESTATUS_DIRTY) {
				m_pPPD->OnSetDirty(true);
			}

			if (flags&PROPPAGESTATUS_CLEAN) {
				m_pPPD->OnSetDirty(false);
			}
		}

		return S_OK;
	}

	STDMETHODIMP GetLocaleID(LCID* pLocaleID) {
		CheckPointer(pLocaleID, E_POINTER);
		*pLocaleID = ::GetUserDefaultLCID();

		return S_OK;
	}

	STDMETHODIMP GetPageContainer(IUnknown** ppUnk) {
		return E_NOTIMPL;
	}

	STDMETHODIMP TranslateAccelerator(LPMSG pMsg) {
		return E_NOTIMPL;
	}
};

// CComPropertySheet

IMPLEMENT_DYNAMIC(CComPropertySheet, CPropertySheet)
CComPropertySheet::CComPropertySheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	: CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
	m_pSite = DNew CComPropertyPageSite(this);
	m_size.SetSize(0, 0);
}

CComPropertySheet::CComPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	: CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
	m_pSite = DNew CComPropertyPageSite(this);
	m_size.SetSize(0, 0);
}

CComPropertySheet::~CComPropertySheet()
{
}

int CComPropertySheet::AddPages(ISpecifyPropertyPages* pSPP)
{
	if (!pSPP) {
		return(0);
	}

	CAUUID caGUID;
	caGUID.pElems = nullptr;

	if (FAILED(pSPP->GetPages(&caGUID)) || caGUID.pElems == nullptr) {
		return(0);
	}

	IUnknown* lpUnk = nullptr;

	if (FAILED(pSPP->QueryInterface(&lpUnk))) {
		return(0);
	}

	m_spp.AddTail(pSPP);

	CComQIPtr<ISpecifyPropertyPages2> pSPP2 = pSPP;
	CComQIPtr<IPersist> pPersist = pSPP;

	ULONG nPages = 0;

	for (ULONG i = 0; i < caGUID.cElems; i++) {
		CComPtr<IPropertyPage> pPage;

		HRESULT hr = E_FAIL;

		if (!pPage && pSPP2) {
			hr = pSPP2->CreatePage(caGUID.pElems[i], &pPage);
		}

		if (FAILED(hr) && !pPage) {
			hr = pPage.CoCreateInstance(caGUID.pElems[i]);
		}

		if (FAILED(hr) && !pPage && pPersist) {
			hr = LoadExternalPropertyPage(pPersist, caGUID.pElems[i], &pPage);
		}

		if (SUCCEEDED(hr)) {
			if (AddPage(pPage, lpUnk)) {
				nPages++;
			}
		}
	}

	if (caGUID.pElems) {
		CoTaskMemFree(caGUID.pElems);
	}

	lpUnk->Release();

	return(nPages);
}

bool CComPropertySheet::AddPage(IPropertyPage* pPage, IUnknown* pUnk)
{
	if (!pPage || !pUnk) {
		return false;
	}

	HRESULT hr;
	hr = pPage->SetPageSite(m_pSite);
	hr = pPage->SetObjects(1, &pUnk);
	PROPPAGEINFO ppi;
	hr = pPage->GetPageInfo(&ppi);
	m_size.cx = max(m_size.cx, ppi.size.cx);
	m_size.cy = max(m_size.cy, ppi.size.cy);
	CAutoPtr<CComPropertyPage> p(DNew CComPropertyPage(pPage));
	__super::AddPage(p);
	m_pages.AddTail(p);

	return true;
}

void CComPropertySheet::OnActivated(CPropertyPage* pPage)
{
	if (!pPage) {
		return;
	}

	CRect bounds(30000,30000,-30000,-30000);

	CRect wr, cr;
	GetWindowRect(wr);
	GetClientRect(cr);
	CSize ws = wr.Size();

	CRect twr, tcr;
	CTabCtrl* pTC = (CTabCtrl*)GetDlgItem(AFX_IDC_TAB_CONTROL);
	pTC->GetWindowRect(twr);
	pTC->GetClientRect(tcr);
	CSize tws = twr.Size();

	if (CWnd* pChild = pPage->GetWindow(GW_CHILD)) {
		pChild->ModifyStyle(WS_CAPTION | WS_THICKFRAME, 0);
		pChild->ModifyStyleEx(WS_EX_DLGMODALFRAME, WS_EX_CONTROLPARENT);

		for (CWnd* pGrandChild = pChild->GetWindow(GW_CHILD); pGrandChild; pGrandChild = pGrandChild->GetNextWindow()) {
			if (!(pGrandChild->GetStyle()&WS_VISIBLE)) {
				continue;
			}

			CRect r;
			pGrandChild->GetWindowRect(&r);
			pChild->ScreenToClient(r);
			bounds |= r;
		}
	}

	bounds |= CRect(0,0,0,0);
	bounds.SetRect(0, 0, bounds.right + max(bounds.left, 4), bounds.bottom + max(bounds.top, 4));

	CRect r = CRect(CPoint(0,0), bounds.Size());
	pTC->AdjustRect(TRUE, r);
	r.SetRect(twr.TopLeft(), twr.TopLeft() + r.Size());
	ScreenToClient(r);
	pTC->MoveWindow(r);
	pTC->ModifyStyle(TCS_MULTILINE, TCS_SINGLELINE);

	CSize diff = r.Size() - tws;

	if (!bounds.IsRectEmpty()) {
		if (CWnd* pChild = pPage->GetWindow(GW_CHILD)) {
			pChild->MoveWindow(bounds);
		}

		CRect r = twr;
		pTC->AdjustRect(FALSE, r);
		ScreenToClient(r);
		pPage->MoveWindow(CRect(r.TopLeft(), bounds.Size()));
	}

	int _afxPropSheetButtons[] = { IDOK, IDCANCEL, ID_APPLY_NOW, IDHELP };

	for (int i = 0; i < _countof(_afxPropSheetButtons); i++) {
		if (CWnd* pWnd = GetDlgItem(_afxPropSheetButtons[i])) {
			pWnd->GetWindowRect(r);
			ScreenToClient(r);
			pWnd->MoveWindow(CRect(r.TopLeft() + diff, r.Size()));
		}
	}

	MoveWindow(CRect(wr.TopLeft(), ws + diff));

	Invalidate();
}

BEGIN_MESSAGE_MAP(CComPropertySheet, CPropertySheet)
END_MESSAGE_MAP()

// CComPropertySheet message handlers

BOOL CComPropertySheet::OnInitDialog()
{
	BOOL bResult = (BOOL)Default();//CPropertySheet::OnInitDialog();

	if (!(GetStyle() & WS_CHILD)) {
		CenterWindow();
	}

	return bResult;
}

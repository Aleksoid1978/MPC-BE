/*
 * (C) 2003-2006 Gabest
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

#pragma once

#include <atlcoll.h>
#include <afxcmn.h>

#include <HighDPI.h>

interface __declspec(uuid("03481710-D73E-4674-839F-03EDE2D60ED8"))
ISpecifyPropertyPages2 :
public ISpecifyPropertyPages {
	STDMETHOD (CreatePage) (const GUID& guid, IPropertyPage** ppPage) PURE;
};

class CInternalPropertyPageWnd : public CWnd, public CDPI
{
	bool m_fDirty;
	CComPtr<IPropertyPageSite> m_pPageSite;

protected:
	CFont m_font, m_monospacefont;
	int m_fontheight;

	virtual BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);

public:
	CInternalPropertyPageWnd();

	void SetDirty(bool fDirty = true) {
		m_fDirty = fDirty;
		if (m_pPageSite) {
			if (fDirty) {
				m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
			} else {
				m_pPageSite->OnStatusChange(PROPPAGESTATUS_CLEAN);
			}
		}
	}
	bool GetDirty() { return m_fDirty; }

	virtual BOOL Create(IPropertyPageSite* pPageSite, LPCRECT pRect, CWnd* pParentWnd);

	virtual bool OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks) { return true; }
	virtual void OnDisconnect() {}
	virtual bool OnActivate() { return true; }
	virtual void OnDeactivate() {}
	virtual bool OnApply() { return true; }

	DECLARE_MESSAGE_MAP()
};

class CInternalPropertyPage
	: public CUnknown
	, public IPropertyPage
	, public CCritSec
{
	CComPtr<IPropertyPageSite> m_pPageSite;
	CInterfaceList<IUnknown, &IID_IUnknown> m_pUnks;
	CInternalPropertyPageWnd* m_pWnd;

protected:
	virtual CInternalPropertyPageWnd* GetWindow() PURE;
	virtual LPCTSTR GetWindowTitle() PURE;
	virtual CSize GetWindowSize() PURE;

public:
	CInternalPropertyPage(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CInternalPropertyPage();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IPropertyPage

	STDMETHODIMP SetPageSite(IPropertyPageSite* pPageSite);
	STDMETHODIMP Activate(HWND hwndParent, LPCRECT pRect, BOOL fModal);
	STDMETHODIMP Deactivate();
	STDMETHODIMP GetPageInfo(PROPPAGEINFO* pPageInfo);
	STDMETHODIMP SetObjects(ULONG cObjects, LPUNKNOWN* ppUnk);
	STDMETHODIMP Show(UINT nCmdShow);
	STDMETHODIMP Move(LPCRECT prect);
	STDMETHODIMP IsPageDirty();
	STDMETHODIMP Apply();
	STDMETHODIMP Help(LPCWSTR lpszHelpDir);
	STDMETHODIMP TranslateAccelerator(LPMSG lpMsg);
};

template<class WndClass>
class CInternalPropertyPageTempl : public CInternalPropertyPage
{
	virtual CInternalPropertyPageWnd* GetWindow() { return DNew WndClass(); }
	virtual LPCTSTR GetWindowTitle() { return WndClass::GetWindowTitle(); }
	virtual CSize GetWindowSize() { return WndClass::GetWindowSize(); }

public:
	CInternalPropertyPageTempl(LPUNKNOWN lpunk, HRESULT* phr)
		: CInternalPropertyPage(lpunk, phr) {
	}
};

class __declspec(uuid("A1EB391C-6089-4A87-9988-BE50872317D4"))
	CPinInfoWnd : public CInternalPropertyPageWnd
{
	CComQIPtr<IBaseFilter> m_pBF;

	enum {
		IDC_PP_COMBO1 = 10000,
		IDC_PP_EDIT1,
	};

	CStatic m_pin_static;
	CComboBox m_pin_combo;
	CEdit m_info_edit;

	typedef CAtlMap<CLSID, CString> CachedFilters;
	CachedFilters m_CachedExternalFilters;
	static CachedFilters m_CachedRegistryFilters;
public:
	CPinInfoWnd();

	bool OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks);
	void OnDisconnect();
	bool OnActivate();
	void OnDeactivate();
	bool OnApply();

	static LPCTSTR GetWindowTitle() { return L"Pin Info"; }
	static CSize GetWindowSize() { return CSize(0, 0); }

	DECLARE_MESSAGE_MAP()

	void OnCbnSelchangeCombo1();

protected:
	virtual BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
};

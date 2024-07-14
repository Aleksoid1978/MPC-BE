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

#pragma once

#include "IDSMResourceBag.h"

#include <map>
#include <unordered_map>
#include <mutex>

// IDSMPropertyBag

interface __declspec(uuid("232FD5D2-4954-41E7-BF9B-09E1257B1A95"))
IDSMPropertyBag :
public IPropertyBag2 {
	STDMETHOD(SetProperty) (LPCWSTR key, LPCWSTR value) PURE;
	STDMETHOD(SetProperty) (LPCWSTR key, VARIANT* var) PURE;
	STDMETHOD(GetProperty) (LPCWSTR key, BSTR* value) PURE;
	STDMETHOD(DelAllProperties) () PURE;
	STDMETHOD(DelProperty) (LPCWSTR key) PURE;
};

class IDSMPropertyBagImpl : public IDSMPropertyBag, public IPropertyBag
{
	std::mutex m_mutex;

protected:
	std::unordered_map<std::wstring, CStringW> m_properties; // unordered_map because no need to sort by key here

public:
	IDSMPropertyBagImpl() = default;
	~IDSMPropertyBagImpl() = default;

	// IPropertyBag

	STDMETHODIMP Read(LPCOLESTR pszPropName, VARIANT* pVar, IErrorLog* pErrorLog) override;
	STDMETHODIMP Write(LPCOLESTR pszPropName, VARIANT* pVar) override;

	// IPropertyBag2

	STDMETHODIMP Read(ULONG cProperties, PROPBAG2* pPropBag, IErrorLog* pErrLog, VARIANT* pvarValue, HRESULT* phrError) override;
	STDMETHODIMP Write(ULONG cProperties, PROPBAG2* pPropBag, VARIANT* pvarValue) override;
	STDMETHODIMP CountProperties(ULONG* pcProperties) override;
	STDMETHODIMP GetPropertyInfo(ULONG iProperty, ULONG cProperties, PROPBAG2* pPropBag, ULONG* pcProperties) override;
	STDMETHODIMP LoadObject(LPCOLESTR pstrName, DWORD dwHint, IUnknown* pUnkObject, IErrorLog* pErrLog) override;

	// IDSMPropertyBag

	STDMETHODIMP SetProperty(LPCWSTR key, LPCWSTR value) override;
	STDMETHODIMP SetProperty(LPCWSTR key, VARIANT* var) override;
	STDMETHODIMP GetProperty(LPCWSTR key, BSTR* value) override;
	STDMETHODIMP DelAllProperties() override;
	STDMETHODIMP DelProperty(LPCWSTR key) override;
};

class CDSMResource
{
public:
	DWORD_PTR tag;
	CStringW name, desc, mime;
	std::vector<BYTE> data;
	CDSMResource();
	CDSMResource(const CDSMResource& r);
	CDSMResource(LPCWSTR name, LPCWSTR desc, LPCWSTR mime, BYTE* pData, int len, DWORD_PTR tag = 0);
	virtual ~CDSMResource();
	CDSMResource& operator = (const CDSMResource& r);

	// global access to all resources
	static CCritSec m_csResources;
	static std::map<uintptr_t, CDSMResource*> m_resources;
};

class IDSMResourceBagImpl : public IDSMResourceBag
{
protected:
	std::vector<CDSMResource> m_resources;

public:
	IDSMResourceBagImpl();

	void operator += (const CDSMResource& r) { m_resources.emplace_back(r); }

	// IDSMResourceBag

	STDMETHODIMP_(DWORD) ResGetCount() override;
	STDMETHODIMP ResGet(DWORD iIndex, BSTR* ppName, BSTR* ppDesc, BSTR* ppMime, BYTE** ppData, DWORD* pDataLen, DWORD_PTR* pTag = nullptr) override;
	STDMETHODIMP ResSet(DWORD iIndex, LPCWSTR pName, LPCWSTR pDesc, LPCWSTR pMime, BYTE* pData, DWORD len, DWORD_PTR tag = 0) override;
	STDMETHODIMP ResAppend(LPCWSTR pName, LPCWSTR pDesc, LPCWSTR pMime, BYTE* pData, DWORD len, DWORD_PTR tag = 0) override;
	STDMETHODIMP ResRemoveAt(DWORD iIndex) override;
	STDMETHODIMP ResRemoveAll(DWORD_PTR tag = 0) override;
};

// IDSMChapterBag

interface __declspec(uuid("2D0EBE73-BA82-4E90-859B-C7C48ED3650F"))
IDSMChapterBag :
public IUnknown {
	STDMETHOD_(DWORD, ChapGetCount) () PURE;
	STDMETHOD(ChapGet) (DWORD iIndex, REFERENCE_TIME* prt, BSTR* ppName) PURE;
	STDMETHOD(ChapSet) (DWORD iIndex, REFERENCE_TIME rt, LPCWSTR pName) PURE;
	STDMETHOD(ChapAppend) (REFERENCE_TIME rt, LPCWSTR pName) PURE;
	STDMETHOD(ChapRemoveAt) (DWORD iIndex) PURE;
	STDMETHOD(ChapRemoveAll) () PURE;
	STDMETHOD_(long, ChapLookup) (REFERENCE_TIME* prt, BSTR* ppName) PURE;
	STDMETHOD(ChapSort) () PURE;
};

class CDSMChapter
{
	static int counter;
	int order;

public:
	REFERENCE_TIME rt;
	CStringW name;
	CDSMChapter();
	CDSMChapter(REFERENCE_TIME rt, LPCWSTR name);
	//CDSMChapter& operator = (const CDSMChapter& c);
	static int Compare(const void* a, const void* b);
};

class IDSMChapterBagImpl : public IDSMChapterBag
{
protected:
	std::vector<CDSMChapter> m_chapters;
	bool m_fSorted;

public:
	IDSMChapterBagImpl();

	void operator += (const CDSMChapter& c) { m_chapters.emplace_back(c); m_fSorted = false; }

	// IDSMChapterBag

	STDMETHODIMP_(DWORD) ChapGetCount() override;
	STDMETHODIMP ChapGet(DWORD iIndex, REFERENCE_TIME* prt, BSTR* ppName = nullptr) override;
	STDMETHODIMP ChapSet(DWORD iIndex, REFERENCE_TIME rt, LPCWSTR pName) override;
	STDMETHODIMP ChapAppend(REFERENCE_TIME rt, LPCWSTR pName) override;
	STDMETHODIMP ChapRemoveAt(DWORD iIndex) override;
	STDMETHODIMP ChapRemoveAll() override;
	STDMETHODIMP_(long) ChapLookup(REFERENCE_TIME* prt, BSTR* ppName = nullptr) override;
	STDMETHODIMP ChapSort() override;
};

class CDSMChapterBag : public CUnknown, public IDSMChapterBagImpl
{
public:
	CDSMChapterBag(LPUNKNOWN pUnk, HRESULT* phr);

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv) override;
};

template<class T>
int range_bsearch(const std::vector<T>& array, REFERENCE_TIME rt)
{
	int i = 0, j = array.size() - 1, ret = -1;
	if (j >= 0 && rt >= array[j].rt) {
		return j;
	}
	while (i < j) {
		int mid = (i + j) >> 1;
		REFERENCE_TIME midrt = array[mid].rt;
		if (rt == midrt) {
			ret = mid;
			break;
		} else if (rt < midrt) {
			ret = -1;
			if (j == mid) {
				mid--;
			}
			j = mid;
		} else if (rt > midrt) {
			ret = mid;
			if (i == mid) {
				mid++;
			}
			i = mid;
		}
	}
	return ret;
}

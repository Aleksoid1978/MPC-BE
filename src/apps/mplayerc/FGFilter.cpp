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
#include <mpconfig.h>
#include "FGFilter.h"
#include "MainFrm.h"
#include <AllocatorCommon.h>
#include <SyncAllocatorPresenter.h>
#include <moreuuids.h>

//
// CFGFilter
//

CFGFilter::CFGFilter(const CLSID& clsid, CStringW name, UINT64 merit)
	: m_clsid(clsid)
	, m_name(name)
{
	m_merit.val = merit;
}

const std::list<GUID>& CFGFilter::GetTypes() const
{
	return m_types;
}

void CFGFilter::SetTypes(const std::list<GUID>& types)
{
	//m_types.clear();
	m_types = types;
}

void CFGFilter::SetMerit(UINT64 merit)
{
	m_merit.val = merit;
}

void CFGFilter::SetName(CString name)
{
	m_name = name;
}

void CFGFilter::AddType(const GUID& majortype, const GUID& subtype)
{
	m_types.push_back(majortype);
	m_types.push_back(subtype);
}

bool CFGFilter::CheckTypes(const std::vector<GUID>& types, bool fExactMatch)
{
	auto it = m_types.cbegin();
	while (it != m_types.cend() && std::next(it) != m_types.cend()) {
		const GUID& majortype = *it++;
		const GUID& subtype   = *it++;

		for (int i = 0, len = types.size() & ~1; i < len; i += 2) {
			if (fExactMatch) {
				if (majortype == types[i] && majortype != GUID_NULL
						&& subtype == types[i+1] && subtype != GUID_NULL) {
					return true;
				}
			} else {
				if ((majortype == GUID_NULL || types[i] == GUID_NULL || majortype == types[i])
						&& (subtype == GUID_NULL || types[i+1] == GUID_NULL || subtype == types[i+1])) {
					return true;
				}
			}
		}
	}

	return false;
}

//
// CFGFilterRegistry
//

CFGFilterRegistry::CFGFilterRegistry(IMoniker* pMoniker, UINT64 merit)
	: CFGFilter(GUID_NULL, L"", merit)
	, m_pMoniker(pMoniker)
{
	if (!m_pMoniker) {
		return;
	}

	LPOLESTR str = nullptr;
	if (FAILED(m_pMoniker->GetDisplayName(0, 0, &str))) {
		return;
	}
	m_DisplayName = m_name = str;
	CoTaskMemFree(str), str = nullptr;

	QueryProperties();

	if (merit != MERIT64_DO_USE) {
		m_merit.val = merit;
	}
}

CFGFilterRegistry::CFGFilterRegistry(CStringW DisplayName, UINT64 merit)
	: CFGFilter(GUID_NULL, L"", merit)
	, m_DisplayName(DisplayName)
{
	if (m_DisplayName.IsEmpty()) {
		return;
	}

	CComPtr<IBindCtx> pBC;
	CreateBindCtx(0, &pBC);

	ULONG chEaten;
	if (S_OK != MkParseDisplayName(pBC, CComBSTR(m_DisplayName), &chEaten, &m_pMoniker)) {
		return;
	}

	QueryProperties();

	if (merit != MERIT64_DO_USE) {
		m_merit.val = merit;
	}
}

void CFGFilterRegistry::QueryProperties()
{
	ASSERT(m_pMoniker);
	CComPtr<IPropertyBag> pPB;
	if (SUCCEEDED(m_pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB))) {
		CComVariant var;
		if (SUCCEEDED(pPB->Read(CComBSTR(L"FriendlyName"), &var, nullptr))) {
			m_name = var.bstrVal;
			var.Clear();
		}

		if (SUCCEEDED(pPB->Read(CComBSTR(L"CLSID"), &var, nullptr))) {
			CLSIDFromString(var.bstrVal, &m_clsid);
			var.Clear();
		}

		if (SUCCEEDED(pPB->Read(CComBSTR(L"FilterData"), &var, nullptr))) {
			BSTR* pstr;
			if (SUCCEEDED(SafeArrayAccessData(var.parray, (void**)&pstr))) {
				ExtractFilterData((BYTE*)pstr, var.parray->cbElements*(var.parray->rgsabound[0].cElements));
				SafeArrayUnaccessData(var.parray);
			}

			var.Clear();
		}
	}
}

CFGFilterRegistry::CFGFilterRegistry(const CLSID& clsid, UINT64 merit)
	: CFGFilter(clsid, L"", merit)
{
	if (m_clsid == GUID_NULL) {
		return;
	}

	CString guid = CStringFromGUID(m_clsid);

	CRegKey key;

	if (ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, L"CLSID\\" + guid, KEY_READ)) {
		ULONG nChars = 0;
		if (ERROR_SUCCESS == key.QueryStringValue(nullptr, nullptr, &nChars)) {
			CString name;
			if (ERROR_SUCCESS == key.QueryStringValue(nullptr, name.GetBuffer(nChars), &nChars)) {
				name.ReleaseBuffer(nChars);
				m_name = name;
			}
		}

		key.Close();
	}

	CRegKey catkey;

	if (ERROR_SUCCESS == catkey.Open(HKEY_CLASSES_ROOT, L"CLSID\\{083863F1-70DE-11d0-BD40-00A0C911CE86}\\Instance", KEY_READ)) {
		if (ERROR_SUCCESS != key.Open(catkey, guid, KEY_READ)) {
			// illiminable pack uses the name of the filter and not the clsid, have to enum all keys to find it...

			FILETIME ft;
			WCHAR buff[256];
			DWORD len = _countof(buff);
			for (DWORD i = 0; ERROR_SUCCESS == catkey.EnumKey(i, buff, &len, &ft); i++, len = _countof(buff)) {
				if (ERROR_SUCCESS == key.Open(catkey, buff, KEY_READ)) {
					WCHAR clsid[256];
					len = _countof(clsid);
					if (ERROR_SUCCESS == key.QueryStringValue(L"CLSID", clsid, &len) && GUIDFromCString(clsid) == m_clsid) {
						break;
					}

					key.Close();
				}
			}
		}

		if (key) {
			ULONG nChars = 0;
			if (ERROR_SUCCESS == key.QueryStringValue(L"FriendlyName", nullptr, &nChars)) {
				CString name;
				if (ERROR_SUCCESS == key.QueryStringValue(L"FriendlyName", name.GetBuffer(nChars), &nChars)) {
					name.ReleaseBuffer(nChars);
					m_name = name;
				}
			}

			ULONG nBytes = 0;
			if (ERROR_SUCCESS == key.QueryBinaryValue(L"FilterData", nullptr, &nBytes)) {
				CAutoVectorPtr<BYTE> buff;
				if (buff.Allocate(nBytes) && ERROR_SUCCESS == key.QueryBinaryValue(L"FilterData", buff, &nBytes)) {
					ExtractFilterData(buff, nBytes);
				}
			}

			key.Close();
		}
	}

	if (merit != MERIT64_DO_USE) {
		m_merit.val = merit;
	}
}

HRESULT CFGFilterRegistry::Create(IBaseFilter** ppBF, CInterfaceList<IUnknown, &IID_IUnknown>& pUnks)
{
	CheckPointer(ppBF, E_POINTER);

	HRESULT hr = E_FAIL;

	if (m_pMoniker) {
		if (SUCCEEDED(hr = m_pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)ppBF))) {
			m_clsid = ::GetCLSID(*ppBF);
		}
	} else if (m_clsid != GUID_NULL) {
		CComQIPtr<IBaseFilter> pBF;

		if (FAILED(pBF.CoCreateInstance(m_clsid))) {
			return E_FAIL;
		}

		*ppBF = pBF.Detach();

		hr = S_OK;
	}

	return hr;
};

interface __declspec(uuid("97f7c4d4-547b-4a5f-8332-536430ad2e4d"))
IAMFilterData :
public IUnknown {
	STDMETHOD (ParseFilterData) (BYTE* rgbFilterData, ULONG cb, BYTE** prgbRegFilter2) PURE;
	STDMETHOD (CreateFilterData) (REGFILTER2* prf2, BYTE** prgbFilterData, ULONG* pcb) PURE;
};

void CFGFilterRegistry::ExtractFilterData(BYTE* p, UINT len)
{
	CComPtr<IAMFilterData> pFD;
	BYTE* ptr = nullptr;

	if (SUCCEEDED(pFD.CoCreateInstance(CLSID_FilterMapper2))
			&& SUCCEEDED(pFD->ParseFilterData(p, len, (BYTE**)&ptr))) {
		REGFILTER2* prf = (REGFILTER2*)*(WPARAM*)ptr; // this is f*cked up

		m_merit.mid = prf->dwMerit;

		if (prf->dwVersion == 1) {
			for (UINT i = 0; i < prf->cPins; i++) {
				if (prf->rgPins[i].bOutput) {
					continue;
				}

				for (UINT j = 0; j < prf->rgPins[i].nMediaTypes; j++) {
					if (!prf->rgPins[i].lpMediaType[j].clsMajorType || !prf->rgPins[i].lpMediaType[j].clsMinorType) {
						break;
					}

					const REGPINTYPES& rpt = prf->rgPins[i].lpMediaType[j];
					AddType(*rpt.clsMajorType, *rpt.clsMinorType);
				}
			}
		} else if (prf->dwVersion == 2) {
			for (UINT i = 0; i < prf->cPins2; i++) {
				if (prf->rgPins2[i].dwFlags&REG_PINFLAG_B_OUTPUT) {
					continue;
				}

				for (UINT j = 0; j < prf->rgPins2[i].nMediaTypes; j++) {
					if (!prf->rgPins2[i].lpMediaType[j].clsMajorType || !prf->rgPins2[i].lpMediaType[j].clsMinorType) {
						break;
					}

					const REGPINTYPES& rpt = prf->rgPins2[i].lpMediaType[j];
					AddType(*rpt.clsMajorType, *rpt.clsMinorType);
				}
			}
		}

		CoTaskMemFree(prf);
	} else {
		BYTE* base = p;

#define ChkLen(size) if (p - base + size > (int)len) return;

		ChkLen(4)
		if (*(DWORD*)p != 0x00000002) {
			return;    // only version 2 supported, no samples found for 1
		}
		p += 4;

		ChkLen(4)
		m_merit.mid = *(DWORD*)p;
		p += 4;

		m_types.clear();

		ChkLen(8)
		DWORD nPins = *(DWORD*)p;
		p += 8;
		while (nPins-- > 0) {
			ChkLen(1)
			BYTE n = *p-0x30;
			p++;
			UNREFERENCED_PARAMETER(n);

			ChkLen(2)
			WORD pi = *(WORD*)p;
			p += 2;
			ASSERT(pi == 'ip');
			UNREFERENCED_PARAMETER(pi);

			ChkLen(1)
			BYTE x33 = *p;
			p++;
			ASSERT(x33 == 0x33);
			UNREFERENCED_PARAMETER(x33);

			ChkLen(8)
			bool fOutput = !!(*p&REG_PINFLAG_B_OUTPUT);
			p += 8;

			ChkLen(12)
			DWORD nTypes = *(DWORD*)p;
			p += 12;
			while (nTypes-- > 0) {
				ChkLen(1)
				BYTE n = *p-0x30;
				p++;
				UNREFERENCED_PARAMETER(n);

				ChkLen(2)
				WORD ty = *(WORD*)p;
				p += 2;
				ASSERT(ty == 'yt');
				UNREFERENCED_PARAMETER(ty);

				ChkLen(5)
				BYTE x33 = *p;
				p++;
				ASSERT(x33 == 0x33);
				UNREFERENCED_PARAMETER(x33);
				p += 4;

				ChkLen(8)
				if (*(DWORD*)p < (DWORD)(p-base+8) || *(DWORD*)p >= len
						|| *(DWORD*)(p+4) < (DWORD)(p-base+8) || *(DWORD*)(p+4) >= len) {
					p += 8;
					continue;
				}

				GUID majortype, subtype;
				memcpy(&majortype, &base[*(DWORD*)p], sizeof(GUID));
				p += 4;
				if (!fOutput) {
					AddType(majortype, subtype);
				}
			}
		}

#undef ChkLen
	}
}

//
// CFGFilterFile
//

CFGFilterFile::CFGFilterFile(const CLSID& clsid, CString path, CStringW name, UINT64 merit)
	: CFGFilter(clsid, name, merit)
	, m_path(path)
	, m_hInst(nullptr)
{
}

HRESULT CFGFilterFile::Create(IBaseFilter** ppBF, CInterfaceList<IUnknown, &IID_IUnknown>& pUnks)
{
	CheckPointer(ppBF, E_POINTER);

	return LoadExternalFilter(m_path, m_clsid, ppBF);
}

//
// CFGFilterVideoRenderer
//

CFGFilterVideoRenderer::CFGFilterVideoRenderer(HWND hWnd, const CLSID& clsid, CStringW name, UINT64 merit)
	: CFGFilter(clsid, name, merit)
	, m_hWnd(hWnd)
{
	AddType(MEDIATYPE_Video, MEDIASUBTYPE_NULL);
}

HRESULT CFGFilterVideoRenderer::Create(IBaseFilter** ppBF, CInterfaceList<IUnknown, &IID_IUnknown>& pUnks)
{
	DLog(L"CFGFilterVideoRenderer::Create() on thread: %d", GetCurrentThreadId());
	CheckPointer(ppBF, E_POINTER);

	HRESULT hr = S_OK;
	CComPtr<ISubPicAllocatorPresenter3> pCAP;

	auto pMainFrame = (CMainFrame *)(AfxGetApp()->m_pMainWnd);
	const bool bFullscreen = pMainFrame && pMainFrame->IsD3DFullScreenMode();

	if (m_clsid == CLSID_EVRAllocatorPresenter) {
		hr = CreateEVR(m_clsid, m_hWnd, bFullscreen, &pCAP);
	} else if (m_clsid == CLSID_SyncAllocatorPresenter) {
		hr = CreateSyncRenderer(m_clsid, m_hWnd, bFullscreen, &pCAP);
	} else if (m_clsid == CLSID_DXRAllocatorPresenter) {
		hr = CreateAP9(m_clsid, m_hWnd, bFullscreen, &pCAP);
	} else if (m_clsid == CLSID_madVRAllocatorPresenter) {
		hr = CreateAP9(m_clsid, m_hWnd, bFullscreen, &pCAP);
	} else {
		CComPtr<IBaseFilter> pBF;
		hr = pBF.CoCreateInstance(m_clsid);
		EXIT_ON_ERROR(hr);

		if (m_clsid == CLSID_EnhancedVideoRenderer) {
			if (CComQIPtr<IEVRFilterConfig> pConfig = pBF) {
				VERIFY(SUCCEEDED(pConfig->SetNumberOfStreams(3)));
			}

			if (CComQIPtr<IMFGetService> pMFGS = pBF) {
				CComPtr<IMFVideoDisplayControl> pMFVDC;
				if (SUCCEEDED(pMFGS->GetService(MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&pMFVDC)))) {
					VERIFY(SUCCEEDED(pMFVDC->SetVideoWindow(m_hWnd)));
				}
			}
		}

		BeginEnumPins(pBF, pEP, pPin) {
			if (CComQIPtr<IMixerPinConfig, &IID_IMixerPinConfig> pMPC = pPin) {
				pUnks.AddTail(pMPC);
				break;
			}
		}
		EndEnumPins;

		const CRenderersSettings& rs = GetRenderersSettings();
		if (GetRenderersSettings().bVMRMixerMode) {
			// VMR9
			if (CComQIPtr<IVMRFilterConfig9> pConfig = pBF) {
				VERIFY(SUCCEEDED(pConfig->SetNumberOfStreams(4)));

				if (CComQIPtr<IVMRMixerControl9> pVMRMC9 = pBF) {
					DWORD dwPrefs;
					VERIFY(SUCCEEDED(pVMRMC9->GetMixingPrefs(&dwPrefs)));

					// See http://msdn.microsoft.com/en-us/library/dd390928(VS.85).aspx
					dwPrefs |= MixerPref9_NonSquareMixing;
					dwPrefs |= MixerPref9_NoDecimation;
					if (rs.bVMRMixerYUV) {
						dwPrefs &= ~MixerPref9_RenderTargetMask;
						dwPrefs |= MixerPref9_RenderTargetYUV;
					}
					VERIFY(SUCCEEDED(pVMRMC9->SetMixingPrefs(dwPrefs)));
				}
			}
		}

		*ppBF = pBF.Detach();
	}

	if (pCAP) {
		CComPtr<IUnknown> pRenderer;
		if (SUCCEEDED(hr = pCAP->CreateRenderer(&pRenderer))) {
			*ppBF = CComQIPtr<IBaseFilter>(pRenderer).Detach();
			pUnks.AddTail(pCAP);

			if (m_clsid == CLSID_madVRAllocatorPresenter) {
				if (CComQIPtr<IMadVRSubclassReplacement> pMVRSR = *ppBF) {
					VERIFY(SUCCEEDED(pMVRSR->DisableSubclassing()));
				}
				// madVR supports calling IVideoWindow::put_Owner before the pins are connected
				if (CComQIPtr<IVideoWindow> pVW = *ppBF) {
					VERIFY(SUCCEEDED(pVW->put_Owner((OAHWND)m_hWnd)));
				}
			}
		}
	}

	CheckPointer(*ppBF, E_FAIL);

	return hr;
}

//
// CFGFilterList
//

CFGFilterList::CFGFilterList()
{
}

CFGFilterList::~CFGFilterList()
{
	RemoveAll();
}

void CFGFilterList::RemoveAll()
{
	while (!m_filters.empty()) {
		const filter_t& f = m_filters.front();
		if (f.autodelete) {
			delete f.pFGF;
		}
		m_filters.pop_front();
	}

	m_sortedfilters.clear();
}

void CFGFilterList::Insert(CFGFilter* pFGF, int group, bool exactmatch, bool autodelete)
{
	if (CFGFilterRegistry* f1r = dynamic_cast<CFGFilterRegistry*>(pFGF)) {
		for (const auto& f2 : m_filters) {
			if (group != f2.group) {
				continue;
			}

			if (CFGFilterRegistry* f2r = dynamic_cast<CFGFilterRegistry*>(f2.pFGF)) {
				if (f1r->GetMoniker() && f2r->GetMoniker() && S_OK == f1r->GetMoniker()->IsEqual(f2r->GetMoniker())
						|| f1r->GetCLSID() != GUID_NULL && f1r->GetCLSID() == f2r->GetCLSID()) {
					DLog(L"FGM: Inserting %d %d %016I64x '%s' NOT!",
						  group, exactmatch, pFGF->GetMerit(),
						  pFGF->GetName().IsEmpty() ? CStringFromGUID(pFGF->GetCLSID()) : CString(pFGF->GetName()));

					if (autodelete) {
						delete pFGF;
					}
					return;
				}
			}
		}
	}

	for (const auto& f : m_filters) {
		CFGFilter* pFGF2 = f.pFGF;
		if (pFGF2->GetCLSID() == pFGF->GetCLSID() && ((pFGF2->GetMerit() >= pFGF->GetMerit()) || (f.group < group))) {
			if (pFGF->GetType() != pFGF2->GetType()) {
				continue;
			}
			if (!pFGF->GetName().IsEmpty() && !pFGF2->GetName().IsEmpty()) {
				if (pFGF2->GetName() != pFGF->GetName()) {
					continue;
				}
			}

			DLog(L"FGM: Inserting %d %d %016I64x '%s', type = '%s' DUP!",
				  group, exactmatch, pFGF->GetMerit(),
				  pFGF->GetName().IsEmpty() ? CStringFromGUID(pFGF->GetCLSID()) : CString(pFGF->GetName()),
				  pFGF->GetType());

			if (autodelete) {
				delete pFGF;
			}
			return;
		}
	}

	DLog(L"FGM: Inserting %d %d %016I64x '%s', type = '%s'",
		  group, exactmatch, pFGF->GetMerit(),
		  pFGF->GetName().IsEmpty() ? CStringFromGUID(pFGF->GetCLSID()) : CString(pFGF->GetName()),
		  pFGF->GetType());

	CString name = pFGF->GetName();
	if (name.Find(L"MPC ") == 0 && pFGF->GetType() != L"CFGFilterInternal") {
		CString external;
		external.Format(L" (%s)", ResStr(IDS_EXTERNAL));

		if (name.Find(external) < 0) {
			name.Append(external);
			pFGF->SetName(name);
		}
	}

	filter_t f = {m_filters.size(), pFGF, group, exactmatch, autodelete};
	m_filters.emplace_back(f);

	m_sortedfilters.clear();
}

unsigned CFGFilterList::GetSortedSize()
{
	if (m_sortedfilters.empty()) {
		std::vector<filter_t> flts;
		flts.reserve(m_filters.size());

		for (const auto& flt : m_filters) {
			if (flt.pFGF->GetMerit() >= MERIT64_DO_USE) {
				flts.emplace_back(flt);
			}
		}

		std::sort(flts.begin(), flts.end(), [](const filter_t& a, const filter_t& b) {
			return (filter_cmp(a, b) < 0);
		});

		m_sortedfilters.reserve(flts.size());
		for (const auto& flt : flts) {
			m_sortedfilters.push_back(flt.pFGF);
		}

#ifdef DEBUG_OR_LOG
		DLog(L"FGM: Sorting filters");
		for (const auto& pFGF : m_sortedfilters) {
			DLog(L"FGM:    %016I64x '%s', type = '%s'",
				pFGF->GetMerit(),
				pFGF->GetName().IsEmpty() ? CStringFromGUID(pFGF->GetCLSID()) : CString(pFGF->GetName()),
				pFGF->GetType());
		}
#endif
	}

	return (unsigned)m_sortedfilters.size();
}

CFGFilter* CFGFilterList::GetFilter(unsigned pos)
{
	return pos < m_sortedfilters.size() ? m_sortedfilters[pos] : nullptr;
}

int CFGFilterList::filter_cmp(const filter_t& a, const filter_t& b)
{
	if (a.group < b.group) {
		return -1;
	}
	if (a.group > b.group) {
		return +1;
	}

	if (a.pFGF->GetMerit() > b.pFGF->GetMerit()) {
		return -1;
	}
	if (a.pFGF->GetMerit() < b.pFGF->GetMerit()) {
		return +1;
	}

	if (a.pFGF->GetCLSID() == b.pFGF->GetCLSID()) {
		CFGFilterFile* fgfa = dynamic_cast<CFGFilterFile*>(a.pFGF);
		CFGFilterFile* fgfb = dynamic_cast<CFGFilterFile*>(b.pFGF);

		if (fgfa && !fgfb) {
			return -1;
		}
		if (!fgfa && fgfb) {
			return +1;
		}
	}

	if (a.exactmatch && !b.exactmatch) {
		return -1;
	}
	if (!a.exactmatch && b.exactmatch) {
		return +1;
	}

	if (a.index < b.index) {
		return -1;
	}
	if (a.index > b.index) {
		return +1;
	}

	return 0;
}

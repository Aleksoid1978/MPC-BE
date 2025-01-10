/*
 * (C) 2003-2006 Gabest
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
#include <mpconfig.h>
#include "FGManager.h"
#include "filters/AllFilters.h"
#include "filters/DeinterlacerFilter.h"
#include "DSUtil/SysVersion.h"
#include "DSUtil/FileVersion.h"
#include "DSUtil/std_helper.h"
#include "DSUtil/NullRenderers.h"
#include "DSUtil/FileHandle.h"
#include "filters/transform/DeCSSFilter/VobFile.h"
#include <dmodshow.h>
#include <evr.h>
#include <evr9.h>
#include <vmr9.h>
#include <ksproxy.h>
#include <clsids.h>
#include <moreuuids.h>
#include <mvrInterfaces.h>
#include "MediaFormats.h"
#include "Content.h"
#include <filters/renderer/VideoRenderers/IPinHook.h>
#include <IURLSourceFilterLAV.h>

class CFGMPCVideoDecoderInternal : public CFGFilterInternal<CMPCVideoDecFilter>
{
	bool	m_bIsPreview;
	UINT64	m_merit;

public:
	CFGMPCVideoDecoderInternal(CStringW name = L"", UINT64 merit = MERIT64_DO_USE, bool IsPreview = false)
		: CFGFilterInternal<CMPCVideoDecFilter>(name, merit)
		, m_bIsPreview(IsPreview)
		, m_merit(merit) {

		CAppSettings& s = AfxGetAppSettings();

		// Get supported MEDIASUBTYPE_ from decoder
		MPCVideoDec::FORMATS fmts;
		MPCVideoDec::GetSupportedFormatList(fmts);

		if (m_merit >= MERIT64_ABOVE_DSHOW) {
			// High merit MPC Video Decoder
			for (const auto& fmt : fmts) {
				if (m_bIsPreview || s.VideoFilters[fmt.FFMPEGCode]) {
					AddType(*fmt.clsMajorType, *fmt.clsMinorType);
				}
			}
		} else if (!m_bIsPreview) {
			// Low merit MPC Video Decoder
			for (const auto& fmt : fmts) {
				AddType(*fmt.clsMajorType, *fmt.clsMinorType);
			}
		}
	}

	HRESULT Create(IBaseFilter** ppBF, std::list<CComQIPtr<IUnknown, &IID_IUnknown>>& pUnks) {
		CheckPointer(ppBF, E_POINTER);

		HRESULT hr = S_OK;
		CComPtr<CMPCVideoDecFilter> pBF = DNew CMPCVideoDecFilter(nullptr, &hr);
		if (FAILED(hr)) {
			return hr;
		}

		CAppSettings& s = AfxGetAppSettings();

		bool video_filters[VDEC_COUNT];
		memcpy(&video_filters, &s.VideoFilters, sizeof(s.VideoFilters));

		if (m_merit == MERIT64_DO_USE) {
			memset(&video_filters, true, sizeof(video_filters));
		}

		if (m_bIsPreview) {
			memset(&video_filters, true, sizeof(video_filters));
			for (int i = 0; i < PixFmt_count; i++) {
				pBF->SetSwPixelFormat((MPCPixelFormat)i, false);
			}
			pBF->SetSwPixelFormat(PixFmt_NV12, true);
			pBF->SetSwPixelFormat(PixFmt_YV12, true);
			pBF->SetSwPixelFormat(PixFmt_YUY2, true);
			pBF->SetSwPixelFormat(PixFmt_RGB32, true);

			if (CComQIPtr<IExFilterConfig> pEFC = pBF.p) {
				pEFC->Flt_SetBool("hw_decoding", false);
			}
		}
		video_filters[VDEC_UNCOMPRESSED] = false;

		for (size_t i = 0; i < VDEC_COUNT; i++) {
			pBF->SetFFMpegCodec(i, video_filters[i]);
		}

		if (CComQIPtr<IExFilterConfig> pEFC = pBF.p) {
			int iMvcOutputMode = MVC_OUTPUT_Auto;
			switch (s.iStereo3DMode) {
				case STEREO3D_MONO:              iMvcOutputMode = MVC_OUTPUT_Mono;          break;
				case STEREO3D_ROWINTERLEAVED:    iMvcOutputMode = MVC_OUTPUT_HalfTopBottom; break;
				case STEREO3D_ROWINTERLEAVED_2X: iMvcOutputMode = MVC_OUTPUT_TopBottom;     break;
				case STEREO3D_HALFOVERUNDER:     iMvcOutputMode = MVC_OUTPUT_HalfTopBottom; break;
				case STEREO3D_OVERUNDER:         iMvcOutputMode = MVC_OUTPUT_TopBottom;     break;
			}
			const int mvc_mode_value = (iMvcOutputMode << 16) | (s.bStereo3DSwapLR ? 1 : 0);

			pEFC->Flt_SetInt("mvc_mode", mvc_mode_value);
		}

		*ppBF = pBF.Detach();

		return hr;
	}
};

//
// CFGManager
//

CFGManager::CFGManager(LPCWSTR pName, LPUNKNOWN pUnk, HWND hWnd, bool IsPreview)
	: CUnknown(pName, pUnk)
	, m_hWnd(hWnd)
	, m_bIsPreview(IsPreview)
{
	m_pUnkInner.CoCreateInstance(CLSID_FilterGraph, GetOwner());
	m_pFM.CoCreateInstance(CLSID_FilterMapper2);
}

CFGManager::~CFGManager()
{
	CAutoLock cAutoLock(this);
	while (!m_source.empty()) {
		delete m_source.front();
		m_source.pop_front();
	}
	while (!m_transform.empty()) {
		delete m_transform.front();
		m_transform.pop_front();
	}
	while (!m_override.empty()) {
		delete m_override.front();
		m_override.pop_front();
	}
	m_pUnks.clear();
	m_pUnkInner.Release();
}

STDMETHODIMP CFGManager::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		QI(IFilterGraph)
		QI(IGraphBuilder)
		QI(IFilterGraph2)
		QI(IGraphBuilder2)
		QI(IGraphBuilderDeadEnd)
		QI(IGraphBuilderSub)
		QI(IGraphBuilderAudio)
		m_pUnkInner && (riid != IID_IUnknown && SUCCEEDED(m_pUnkInner->QueryInterface(riid, ppv))) ? S_OK :
		__super::NonDelegatingQueryInterface(riid, ppv);
}

//

void CFGManager::CStreamPath::Append(IBaseFilter* pBF, IPin* pPin)
{
	path_t p;
	p.clsid = GetCLSID(pBF);
	p.filter = GetFilterName(pBF);
	p.pin = GetPinName(pPin);
	emplace_back(p);
}

bool CFGManager::CStreamPath::Compare(const CStreamPath& path)
{
	auto it1 = begin();
	auto it2 = path.begin();

	while (it1 != end() && it2 != path.end()) {
		const path_t& p1 = *it1++;
		const path_t& p2 = *it2++;

		if (p1.filter != p2.filter) {
			return true;
		} else if (p1.pin != p2.pin) {
			return false;
		}
	}

	return true;
}

//

bool CFGManager::CheckBytes(HANDLE hFile, CString chkbytes)
{
	std::list<CString> sl;
	Explode(chkbytes, sl, L',');

	if (sl.size() < 4) {
		return false;
	}

	ASSERT(!(sl.size()&3));

	LARGE_INTEGER size = {0, 0};
	GetFileSizeEx(hFile, &size);

	while (sl.size() >= 4) {
		CString offsetstr = sl.front();
		sl.pop_front();
		CString cbstr = sl.front();
		sl.pop_front();
		CString maskstr = sl.front();
		sl.pop_front();
		CString valstr = sl.front();
		sl.pop_front();

		long cb = _wtol(cbstr);

		if (offsetstr.IsEmpty() || cbstr.IsEmpty()
				|| valstr.IsEmpty() || (valstr.GetLength() & 1)
				|| cb*2 != valstr.GetLength()) {
			return false;
		}

		LARGE_INTEGER offset;
		offset.QuadPart = _wtoi64(offsetstr);
		if (offset.QuadPart < 0) {
			offset.QuadPart = size.QuadPart - offset.QuadPart;
		}
		SetFilePointerEx(hFile, offset, &offset, FILE_BEGIN);

		// LAME
		while (maskstr.GetLength() < valstr.GetLength()) {
			maskstr += 'F';
		}

		std::vector<BYTE> mask, val;
		CStringToBin(maskstr, mask);
		CStringToBin(valstr, val);

		for (size_t i = 0; i < val.size(); i++) {
			BYTE b;
			DWORD r;
			if (!ReadFile(hFile, &b, 1, &r, nullptr) || (b & mask[i]) != val[i]) {
				return false;
			}
		}
	}

	return sl.empty();
}

bool CFGManager::CheckBytes(PBYTE buf, DWORD size, CString chkbytes)
{
	std::list<CString> sl;
	Explode(chkbytes, sl, L',');

	if (sl.size() < 4) {
		return false;
	}

	ASSERT(!(sl.size()&3));

	CGolombBuffer gb(buf, size);

	while (sl.size() >= 4) {
		CString offsetstr = sl.front();
		sl.pop_front();
		CString cbstr = sl.front();
		sl.pop_front();
		CString maskstr = sl.front();
		sl.pop_front();
		CString valstr = sl.front();
		sl.pop_front();

		long cb = _wtol(cbstr);

		if (offsetstr.IsEmpty() || cbstr.IsEmpty()
				|| valstr.IsEmpty() || (valstr.GetLength() & 1)
				|| cb*2 != valstr.GetLength()) {
			return false;
		}

		__int64 offset = _wtoi64(offsetstr);
		if (offset + valstr.GetLength() > size) {
			return false;
		}

		gb.Seek(offset);

		// LAME
		while (maskstr.GetLength() < valstr.GetLength()) {
			maskstr += 'F';
		}

		std::vector<BYTE> mask, val;
		CStringToBin(maskstr, mask);
		CStringToBin(valstr, val);

		for (size_t i = 0; i < val.size(); i++) {
			BYTE b = gb.ReadByte();
			if ((b & mask[i]) != val[i]) {
				return false;
			}
		}
	}

	return sl.empty();
}

CFGFilter *LookupFilterRegistry(const GUID &guid, std::list<CFGFilter*> &list, UINT64 fallback_merit = MERIT64_DO_USE)
{
	CFGFilter *pFilter = nullptr;
	for (const auto& pFGF : list) {
		if (pFGF->GetCLSID() == guid) {
			pFilter = pFGF;
			break;
		}
	}
	if (pFilter) {
		return DNew CFGFilterRegistry(guid, pFilter->GetMerit());
	} else {
		return DNew CFGFilterRegistry(guid, fallback_merit);
	}
}

HRESULT CFGManager::EnumSourceFilters(LPCWSTR lpcwstrFileName, CFGFilterList& fl)
{
	CheckPointer(lpcwstrFileName, E_POINTER);

	fl.RemoveAll();

	CString fn = CString(lpcwstrFileName).TrimLeft();
	CString protocol = fn.Left(fn.Find(':')).MakeLower();
	CString ext = GetFileExt(fn).MakeLower();

	HANDLE hFile = INVALID_HANDLE_VALUE;
	std::vector<BYTE> httpbuf;

	if ((protocol.GetLength() <= 1 || protocol == L"file" || StartsWith(protocol, EXTENDED_PATH_PREFIX)) && (ext.Compare(L".cda") != 0)) {
		hFile = CreateFileW(fn, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

		if (hFile == INVALID_HANDLE_VALUE) {
			return VFW_E_NOT_FOUND;
		}

		LARGE_INTEGER size = {};
		GetFileSizeEx(hFile, &size);
		if (!size.QuadPart) {
			CloseHandle(hFile);
			return VFW_E_CANNOT_RENDER;
		}
	} else {
		Content::Online::GetRaw(lpcwstrFileName, httpbuf);
	}

	// internal filters
	{
		if (hFile == INVALID_HANDLE_VALUE) {
			// check bytes for http
			if (httpbuf.size()) {
				for (const auto& pFGF : m_source) {
					for (const auto& bytestring : pFGF->m_chkbytes) {
						if (CheckBytes(httpbuf.data(), httpbuf.size(), bytestring)) {
							fl.Insert(pFGF, 0, false, false);
							break;
						}
					}
				}
			}

			// protocol
			for (const auto& pFGF : m_source) {
				if (Contains(pFGF->m_protocols, protocol)) {
					fl.Insert(pFGF, 0, false, false);
				}
			}
		} else {
			// check bytes
			for (const auto& pFGF : m_source) {
				for (const auto& bytestring : pFGF->m_chkbytes) {
					if (CheckBytes(hFile, bytestring)) {
						fl.Insert(pFGF, 1, false, false);
						break;
					}
				}
			}
		}

		if (!ext.IsEmpty()) {
			// file extension

			for (const auto& pFGF : m_source) {
				if (Contains(pFGF->m_extensions, ext)) {
					fl.Insert(pFGF, 2, false, false);
				}
			}
		}

		{
			// the rest
			for (const auto& pFGF : m_source) {
				if (pFGF->m_protocols.empty() && pFGF->m_chkbytes.empty() && pFGF->m_extensions.empty()) {
					fl.Insert(pFGF, 3, false, false);
				}
			}
		}
	}

	{
		// add an external preferred filter
		for (const auto& pFGF : m_override) {
			if (pFGF->GetMerit() >= MERIT64_EXT_FILTERS_PREFER) { // FilterOverride::PREFERRED
				fl.Insert(pFGF, 0, false, false);
			}
		}
	}

	{
		// Filters Priority
		CAppSettings& s = AfxGetAppSettings();

		CStringW type;
		if (httpbuf.size()) {
			type = L"http";
		}
		else if (protocol == L"udp") {
			type = L"udp";
		}
		else if (CMediaFormatCategory* mfc = s.m_Formats.FindMediaByExt(ext)) {
			type = mfc->GetLabel();
		}

		if (type.GetLength()) {
			if (const auto it = s.FiltersPriority.values.find(type); it != s.FiltersPriority.values.cend() && it->second != CLSID_NULL) {
				const auto& clsid_value = it->second;

				for (const auto& pFGF : m_override) {
					if (pFGF->GetCLSID() == clsid_value) {
						const std::list<GUID>& types = pFGF->GetTypes();
						if (types.size() && !httpbuf.size()) {
							bool bIsSplitter = false;
							auto it = types.cbegin();
							while (it != types.cend() && std::next(it) != types.cend()) {
								CLSID major = *it++;
								CLSID sub   = *it++;

								if (major == MEDIATYPE_Stream) {
									bIsSplitter = true;

									std::list<GUID> typesNew;
									typesNew.emplace_back(major);
									typesNew.emplace_back(sub);
									pFGF->SetTypes(typesNew);

									break;
								}
							}
							if (bIsSplitter) {
								CFGFilter* pFGFAsync = LookupFilterRegistry(CLSID_AsyncReader, m_override, MERIT64_HIGH + 1);
								fl.Insert(pFGFAsync, 0);
							}
						}

						pFGF->SetMerit(MERIT64_HIGH);
						fl.Insert(pFGF, 0, false, false);

						break;
					}
				}
			}
		}
	}

	// external
	{
		// hack for StreamBufferSource - we need override merit from registry.
		if (ext == L".dvr-ms" || ext == L".wtv") {
			BOOL bIsBlocked = FALSE;
			for (const auto& pFGF : m_override) {
				if (pFGF->GetCLSID() == CLSID_StreamBufferSource && pFGF->GetMerit() == MERIT64_DO_NOT_USE) {
					bIsBlocked = TRUE;
					break;
				}
			}

			if (!bIsBlocked) {
				CFGFilter* pFGF = LookupFilterRegistry(CLSID_StreamBufferSource, m_override);
				pFGF->SetMerit(MERIT64_DO_USE);
				fl.Insert(pFGF, 9);
			}
		}
		// add MPC Script Source
		else if (ext == L".avs" || ext == L".vpy") {
			CFGFilter* pFGF = LookupFilterRegistry(CLSID_MPCScriptSource, m_override);
			if (pFGF) {
				fl.Insert(pFGF, 9);
			}
		}

		WCHAR buff[256] = {}, buff2[256] = {};
		ULONG len, len2;

		if (hFile == INVALID_HANDLE_VALUE) {
			// protocol

			CRegKey key;
			if (ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, protocol, KEY_READ)) {
				CRegKey exts;
				if (ERROR_SUCCESS == exts.Open(key, L"Extensions", KEY_READ)) {
					len = std::size(buff);
					if (ERROR_SUCCESS == exts.QueryStringValue(ext, buff, &len)) {
						fl.Insert(LookupFilterRegistry(GUIDFromCString(buff), m_override), 4);
					}
				}

				len = std::size(buff);
				if (ERROR_SUCCESS == key.QueryStringValue(L"Source Filter", buff, &len)) {
					fl.Insert(LookupFilterRegistry(GUIDFromCString(buff), m_override), 5);
				}
			}

			BOOL bIsBlocked = FALSE;
			for (const auto& pFGF : m_override) {
				if (pFGF->GetCLSID() == CLSID_URLReader && pFGF->GetMerit() == MERIT64_DO_NOT_USE) {
					bIsBlocked = TRUE;
					break;
				}
			}

			if (!bIsBlocked) {
				fl.Insert(DNew CFGFilterRegistry(CLSID_URLReader), 6);
			}
		} else {
			// check bytes

			CRegKey key;
			if (ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, L"Media Type", KEY_READ)) {
				FILETIME ft;
				len = std::size(buff);
				for (DWORD i = 0; ERROR_SUCCESS == key.EnumKey(i, buff, &len, &ft); i++, len = std::size(buff)) {
					GUID majortype;
					if (FAILED(GUIDFromCString(buff, majortype))) {
						continue;
					}

					CRegKey majorkey;
					if (ERROR_SUCCESS == majorkey.Open(key, buff, KEY_READ)) {
						len = std::size(buff);
						for (DWORD j = 0; ERROR_SUCCESS == majorkey.EnumKey(j, buff, &len, &ft); j++, len = std::size(buff)) {
							GUID subtype;
							if (FAILED(GUIDFromCString(buff, subtype))) {
								continue;
							}

							CRegKey subkey;
							if (ERROR_SUCCESS == subkey.Open(majorkey, buff, KEY_READ)) {
								len = std::size(buff);
								if (ERROR_SUCCESS != subkey.QueryStringValue(L"Source Filter", buff, &len)) {
									continue;
								}

								GUID clsid = GUIDFromCString(buff);

								DWORD size = sizeof(buff);
								len2 = std::size(buff2);
								for (DWORD k = 0, type;
										clsid != GUID_NULL && ERROR_SUCCESS == RegEnumValueW(subkey, k, buff2, &len2, 0, &type, (BYTE*)buff, &size);
										k++, size = sizeof(buff), len2 = std::size(buff2)) {
									if (CheckBytes(hFile, CString(buff))) {
										CFGFilter* pFGF = LookupFilterRegistry(clsid, m_override);
										pFGF->AddType(majortype, subtype);
										fl.Insert(pFGF, 9);
										break;
									}
								}
							}
						}
					}
				}
			}
		}

		if (!ext.IsEmpty()) {
			// file extension

			CRegKey key;
			if (ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, L"Media Type\\Extensions\\" + CString(ext), KEY_READ)) {
				ULONG len = std::size(buff);
				memset(buff, 0, sizeof(buff));
				LONG ret = key.QueryStringValue(L"Source Filter", buff, &len); // QueryStringValue can return ERROR_INVALID_DATA on bogus strings (radlight mpc v1003, fixed in v1004)
				if (ERROR_SUCCESS == ret || (ERROR_INVALID_DATA == ret && GUIDFromCString(buff) != GUID_NULL)) {
					GUID clsid = GUIDFromCString(buff);
					GUID majortype = GUID_NULL;
					GUID subtype = GUID_NULL;

					len = std::size(buff);
					if (ERROR_SUCCESS == key.QueryStringValue(L"Media Type", buff, &len)) {
						majortype = GUIDFromCString(buff);
					}

					len = std::size(buff);
					if (ERROR_SUCCESS == key.QueryStringValue(L"Subtype", buff, &len)) {
						subtype = GUIDFromCString(buff);
					}

					CFGFilter* pFGF = LookupFilterRegistry(clsid, m_override);
					pFGF->AddType(majortype, subtype);
					fl.Insert(pFGF, 9);
				}
			}
		}
	}

	if (hFile != INVALID_HANDLE_VALUE) {
		BOOL bIsBlocked = FALSE;
		for (const auto& pFGF : m_override) {
			if (pFGF->GetCLSID() == CLSID_AsyncReader && pFGF->GetMerit() == MERIT64_DO_NOT_USE) {
				bIsBlocked = TRUE;
				break;
			}
		}

		if (!bIsBlocked) {
			CFGFilter* pFGF = LookupFilterRegistry(CLSID_AsyncReader, m_override);
			pFGF->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_NULL);
			fl.Insert(pFGF, 9);
		}

		CloseHandle(hFile);
	}

	return S_OK;
}

HRESULT CFGManager::AddSourceFilterInternal(CFGFilter* pFGF, LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrFilterName, IBaseFilter** ppBF)
{
	if (m_bOpeningAborted) {
		return E_ABORT;
	}

	DLog(L"FGM: AddSourceFilterInternal() trying '%s'", pFGF->GetName().IsEmpty() ? CStringFromGUID(pFGF->GetCLSID()) : CString(pFGF->GetName())/*CStringFromGUID(pFGF->GetCLSID())*/);

	CheckPointer(lpcwstrFileName, E_POINTER);
	CheckPointer(ppBF, E_POINTER);

	ASSERT(*ppBF == nullptr);

	CComPtr<IBaseFilter> pBF;
	std::list<CComQIPtr<IUnknown, &IID_IUnknown>> pUnks;

	HRESULT hr = pFGF->Create(&pBF, pUnks);
	if (FAILED(hr)) {
		return hr;
	}

	CComQIPtr<IFileSourceFilter> pFSF = pBF.p;
	if (!pFSF) {
		return E_NOINTERFACE;
	}

	hr = AddFilter(pBF, lpcwstrFilterName);
	if (FAILED(hr)) {
		return hr;
	}

	const AM_MEDIA_TYPE* pmt = nullptr;

	hr = E_NOT_VALID_STATE;
	if (::PathIsURLW(lpcwstrFileName)) {
		CComQIPtr<IURLSourceFilterLAV> pUSFLAV = pBF.p;
		if (pUSFLAV) {
			hr = pUSFLAV->LoadURL(lpcwstrFileName, m_userAgent, m_referrer);
		}
	}

	if (FAILED(hr)) {
		CMediaType mt;
		const std::list<GUID>& types = pFGF->GetTypes();
		if (types.size() == 2 && (types.front() != GUID_NULL || types.back() != GUID_NULL)) {
			mt.majortype = types.front();
			mt.subtype = types.back();
			pmt = &mt;
		}

		hr = pFSF->Load(lpcwstrFileName, pmt);
		if (FAILED(hr) || m_bOpeningAborted) { // sometimes looping with AviSynth
			RemoveFilter(pBF);
			return m_bOpeningAborted ? E_ABORT : hr;
		}

		// doh :P
		BeginEnumMediaTypes(GetFirstPin(pBF, PINDIR_OUTPUT), pEMT, pmt) {
			static const GUID guid1 =
			{ 0x640999A0, 0xA946, 0x11D0, { 0xA5, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }; // ASX file Parser
			static const GUID guid2 =
			{ 0x640999A1, 0xA946, 0x11D0, { 0xA5, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }; // ASX v.2 file Parser
			static const GUID guid3 =
			{ 0xD51BD5AE, 0x7548, 0x11CF, { 0xA5, 0x20, 0x00, 0x80, 0xC7, 0x7E, 0xF5, 0x8A } }; // XML Playlist

			if (pmt->subtype == guid1 || pmt->subtype == guid2 || pmt->subtype == guid3) {
				RemoveFilter(pBF);
				pFGF = DNew CFGFilterRegistry(CLSID_NetShowSource);
				hr = AddSourceFilterInternal(pFGF, lpcwstrFileName, lpcwstrFilterName, ppBF);
				delete pFGF;
				SAFE_DELETE(pmt);
				return hr;
			}
		}
		EndEnumMediaTypes(pmt)
	}

	*ppBF = pBF.Detach();

	m_pUnks.insert(m_pUnks.cend(), pUnks.cbegin(), pUnks.cend());

	return S_OK;
}

// IFilterGraph

STDMETHODIMP CFGManager::AddFilter(IBaseFilter* pFilter, LPCWSTR pName)
{
	if (!m_pUnkInner) {
		return E_UNEXPECTED;
	}

	CAutoLock cAutoLock(this);

	HRESULT hr;

	if (FAILED(hr = CComQIPtr<IFilterGraph2>(m_pUnkInner)->AddFilter(pFilter, pName))) {
		return hr;
	}

	// TODO
	hr = pFilter->JoinFilterGraph(nullptr, nullptr);
	hr = pFilter->JoinFilterGraph(this, pName);

	return hr;
}

STDMETHODIMP CFGManager::RemoveFilter(IBaseFilter* pFilter)
{
	if (!m_pUnkInner) {
		return E_UNEXPECTED;
	}

	CAutoLock cAutoLock(this);

	return CComQIPtr<IFilterGraph2>(m_pUnkInner)->RemoveFilter(pFilter);
}

STDMETHODIMP CFGManager::EnumFilters(IEnumFilters** ppEnum)
{
	if (!m_pUnkInner) {
		return E_UNEXPECTED;
	}

	// Not locking here fixes a deadlock involving ReClock
	//CAutoLock cAutoLock(this);

	return CComQIPtr<IFilterGraph2>(m_pUnkInner)->EnumFilters(ppEnum);
}

STDMETHODIMP CFGManager::FindFilterByName(LPCWSTR pName, IBaseFilter** ppFilter)
{
	if (!m_pUnkInner) {
		return E_UNEXPECTED;
	}

	CAutoLock cAutoLock(this);

	return CComQIPtr<IFilterGraph2>(m_pUnkInner)->FindFilterByName(pName, ppFilter);
}

STDMETHODIMP CFGManager::ConnectDirect(IPin* pPinOut, IPin* pPinIn, const AM_MEDIA_TYPE* pmt)
{
	if (!m_pUnkInner) {
		return E_UNEXPECTED;
	}

	CAutoLock cAutoLock(this);

	CComPtr<IBaseFilter> pBF = GetFilterFromPin(pPinIn);
	CLSID clsid = GetCLSID(pBF);

	// TODO: GetUpStreamFilter goes up on the first input pin only
	for (CComPtr<IBaseFilter> pBFUS = GetFilterFromPin(pPinOut); pBFUS; pBFUS = GetUpStreamFilter(pBFUS)) {
		if (pBFUS == pBF) {
			return VFW_E_CIRCULAR_GRAPH;
		}
		if (clsid!=CLSID_Proxy && GetCLSID(pBFUS) == clsid) {
			return VFW_E_CANNOT_CONNECT;
		}
	}

	return CComQIPtr<IFilterGraph2>(m_pUnkInner)->ConnectDirect(pPinOut, pPinIn, pmt);
}

STDMETHODIMP CFGManager::Reconnect(IPin* ppin)
{
	if (!m_pUnkInner) {
		return E_UNEXPECTED;
	}

	CAutoLock cAutoLock(this);

	return CComQIPtr<IFilterGraph2>(m_pUnkInner)->Reconnect(ppin);
}

STDMETHODIMP CFGManager::Disconnect(IPin* ppin)
{
	if (!m_pUnkInner) {
		return E_UNEXPECTED;
	}

	CAutoLock cAutoLock(this);

	return CComQIPtr<IFilterGraph2>(m_pUnkInner)->Disconnect(ppin);
}

STDMETHODIMP CFGManager::SetDefaultSyncSource()
{
	if (!m_pUnkInner) {
		return E_UNEXPECTED;
	}

	CAutoLock cAutoLock(this);

	return CComQIPtr<IFilterGraph2>(m_pUnkInner)->SetDefaultSyncSource();
}

// IGraphBuilder

STDMETHODIMP CFGManager::Connect(IPin* pPinOut, IPin* pPinIn)
{
	return ConnectInternal(pPinOut, pPinIn, true);
}

HRESULT CFGManager::ConnectInternal(IPin* pPinOut, IPin* pPinIn, bool bContinueRender)
{
	CAutoLock cAutoLock(this);

	CheckPointer(pPinOut, E_POINTER);

	HRESULT hr;

	if (S_OK != IsPinDirection(pPinOut, PINDIR_OUTPUT)
			|| pPinIn && S_OK != IsPinDirection(pPinIn, PINDIR_INPUT)) {
		return VFW_E_INVALID_DIRECTION;
	}

	if (S_OK == IsPinConnected(pPinOut)
			|| pPinIn && S_OK == IsPinConnected(pPinIn)) {
		return VFW_E_ALREADY_CONNECTED;
	}

	BeginEnumMediaTypes(pPinOut, pEM, pmt) {
		// skip Audio output Pin for preview mode;
		if (m_bIsPreview) {
			// Allow only video
			if (pmt->majortype == MEDIATYPE_Audio
				|| pmt->majortype == MEDIATYPE_AUXLine21Data
				|| pmt->majortype == MEDIATYPE_Midi
				|| CMediaTypeEx(*pmt).ValidateSubtitle()) {
				return S_FALSE;
			}
			// DVD
			if (pmt->majortype == MEDIATYPE_DVD_ENCRYPTED_PACK && pmt->subtype != MEDIASUBTYPE_MPEG2_VIDEO) {
				return S_FALSE;
			}
		}

		// DVR-MS Caption (WTV Subtitle)/MPEG2_SECTIONS pin - disable
		if (pmt->majortype == MEDIATYPE_MSTVCaption || pmt->majortype == MEDIATYPE_MPEG2_SECTIONS) {
			return S_FALSE;
		}

		if (m_bOnlySub && !CMediaTypeEx(*pmt).ValidateSubtitle()) {
			return S_FALSE;
		}

		if (m_bOnlyAudio
				&& pmt->majortype != MEDIATYPE_Audio
				&& pmt->majortype != MEDIATYPE_Stream
				&& !CMediaTypeEx(*pmt).ValidateSubtitle()) {
			return S_FALSE;
		}
	}
	EndEnumMediaTypes(pmt)

	bool fDeadEnd = true;

	if (pPinIn) {
		// 1. Try a direct connection between the filters, with no intermediate filters

		if (SUCCEEDED(hr = ConnectDirect(pPinOut, pPinIn, nullptr))) {
			return hr;
		}
	} else {
		// 1. Use IStreamBuilder

		if (CComQIPtr<IStreamBuilder> pSB = pPinOut) {
			if (SUCCEEDED(hr = pSB->Render(pPinOut, this))) {
				return hr;
			}

			pSB->Backout(pPinOut, this);
		}
	}

	// 2. Try cached filters

	if (CComQIPtr<IGraphConfig> pGC = (IGraphBuilder2*)this) {
		BeginEnumCachedFilters(pGC, pEF, pBF) {
			if (pPinIn && GetFilterFromPin(pPinIn) == pBF) {
				continue;
			}

			hr = pGC->RemoveFilterFromCache(pBF);

			// does RemoveFilterFromCache call AddFilter like AddFilterToCache calls RemoveFilter ?

			if (SUCCEEDED(hr = ConnectFilterDirect(pPinOut, pBF, nullptr))) {
				if (!IsStreamEnd(pBF)) {
					fDeadEnd = false;
				}

				if (SUCCEEDED(hr = ConnectFilter(pBF, pPinIn))) {
					return hr;
				}
			}

			hr = pGC->AddFilterToCache(pBF);
		}
		EndEnumCachedFilters
	}

	// 3. Try filters in the graph

	{
		std::list<CComQIPtr<IBaseFilter>> pBFs;

		BeginEnumFilters(this, pEF, pBF) {
			if (pPinIn && GetFilterFromPin(pPinIn) == pBF
					|| GetFilterFromPin(pPinOut) == pBF) {
				continue;
			}

			// HACK: ffdshow - audio capture filter
			if (GetCLSID(pPinOut) == GUIDFromCString(L"{04FE9017-F873-410E-871E-AB91661A4EF7}")
					&& GetCLSID(pBF) == GUIDFromCString(L"{E30629D2-27E5-11CE-875D-00608CB78066}")) {
				continue;
			}

			pBFs.emplace_back(pBF);
		}
		EndEnumFilters;

		for (auto& pBF : pBFs) {
			if (SUCCEEDED(hr = ConnectFilterDirect(pPinOut, pBF, nullptr))) {
				if (!IsStreamEnd(pBF)) {
					fDeadEnd = false;
				}

				if (SUCCEEDED(hr = ConnectFilter(pBF, pPinIn))) {
					return hr;
				}
			}

			EXECUTE_ASSERT(SUCCEEDED(Disconnect(pPinOut)));
		}
	}

	// 4. Look up filters in the registry

	{
		CFGFilterList fl;

		std::vector<GUID> types;
		ExtractMediaTypes(pPinOut, types);

		for (const auto& pFGF : m_transform) {
			if (pFGF->GetMerit() < MERIT64_DO_USE || pFGF->CheckTypes(types, false)) {
				fl.Insert(pFGF, 0, pFGF->CheckTypes(types, true), false);
			}
		}

		for (const auto& pFGF : m_override) {
			if (pFGF->GetMerit() < MERIT64_DO_USE || pFGF->CheckTypes(types, false)) {
				fl.Insert(pFGF, 0, pFGF->CheckTypes(types, true), false);
			}
		}

		CComPtr<IEnumMoniker> pEM;
		if (!types.empty()
				&& SUCCEEDED(m_pFM->EnumMatchingFilters(
								 &pEM, 0, FALSE, MERIT_DO_NOT_USE+1,
								 TRUE, types.size()/2, types.data(), nullptr, nullptr, FALSE,
								 !!pPinIn, 0, nullptr, nullptr, nullptr))) {
			for (CComPtr<IMoniker> pMoniker; S_OK == pEM->Next(1, &pMoniker, nullptr); pMoniker = nullptr) {
				CFGFilterRegistry* pFGF = DNew CFGFilterRegistry(pMoniker);
				fl.Insert(pFGF, 0, pFGF->CheckTypes(types, true));
			}
		}

		// let's check whether the madVR allocator presenter is in our list
		// it should be if madVR is selected as the video renderer
		CFGFilter* pMadVRAllocatorPresenter = nullptr;

		for (unsigned pos = 0, fltnum = fl.GetSortedSize(); pos < fltnum; pos++) {
			CFGFilter* pFGF = fl.GetFilter(pos);
			if (pFGF->GetCLSID() == CLSID_madVRAllocatorPresenter) {
				// found it!
				pMadVRAllocatorPresenter = pFGF;
				break;
			}
		}

		for (unsigned pos = 0, fltnum = fl.GetSortedSize(); pos < fltnum; pos++) {
			CFGFilter* pFGF = fl.GetFilter(pos);

			// Checks if any Video Renderer is already in the graph to avoid trying to connect a second instance
			if (IsVideoRenderer(pFGF->GetCLSID())) {
				CString fname = pFGF->GetName();
				if (!fname.IsEmpty()) {
					CComPtr<IBaseFilter> pBFVR;
					if (SUCCEEDED(FindFilterByName(fname, &pBFVR)) && pBFVR) {
						DLog(L"FGM: Skip '%s' - already in graph", pFGF->GetName());
						continue;
					}
				}
			}

			if ((pMadVRAllocatorPresenter) && (pFGF->GetCLSID() == CLSID_madVR)) {
				// the pure madVR filter was selected (without the allocator presenter)
				// subtitles, OSD etc don't work correcty without the allocator presenter
				// so we prefer the allocator presenter over the pure filter
				pFGF = pMadVRAllocatorPresenter;
			}

			DLog(L"FGM: Connecting '%s'", pFGF->GetName());

			CComPtr<IBaseFilter> pBF;
			std::list<CComQIPtr<IUnknown, &IID_IUnknown>> pUnks;
			if (FAILED(pFGF->Create(&pBF, pUnks))) {
				continue;
			}

			if (FAILED(hr = AddFilter(pBF, pFGF->GetName()))) {
				pBF.Release();
				continue;
			}

			auto hookDirectXVideoDecoderService = [](CComQIPtr<IMFGetService> pMFGS) {
				CComPtr<IDirectXVideoDecoderService> pDecoderService;
				CComPtr<IDirect3DDeviceManager9>     pDeviceManager;
				HANDLE                               hDevice = INVALID_HANDLE_VALUE;

				if (SUCCEEDED(pMFGS->GetService(MR_VIDEO_ACCELERATION_SERVICE, IID_PPV_ARGS(&pDeviceManager)))
						&& SUCCEEDED(pDeviceManager->OpenDeviceHandle(&hDevice))
						&& SUCCEEDED(pDeviceManager->GetVideoService(hDevice, IID_PPV_ARGS(&pDecoderService)))) {
					HookDirectXVideoDecoderService(pDecoderService);
					pDeviceManager->CloseDeviceHandle(hDevice);
				}
			};

			if (!m_bIsPreview && !pMadVRAllocatorPresenter) {
				if (CComQIPtr<IMFGetService> pMFGS = pBF.p) {
					// hook IDirectXVideoDecoderService to get DXVA status & logging;
					// why before ConnectFilterDirect() - some decoder, like ArcSoft & Cyberlink, init DXVA2 decoder while connect to the renderer ...
					// madVR crash on call ::GetService() before connect
					hookDirectXVideoDecoderService(pMFGS);
				}
			}

			hr = ConnectFilterDirect(pPinOut, pBF, nullptr);

			if (SUCCEEDED(hr)) {
				if (!IsStreamEnd(pBF)) {
					fDeadEnd = false;
				}

				if (bContinueRender) {
					hr = ConnectFilter(pBF, pPinIn);
				}

				if (SUCCEEDED(hr)) {
					m_pUnks.insert(m_pUnks.cend(), pUnks.cbegin(), pUnks.cend());

					// maybe the application should do this...

					for (auto& pUnk : pUnks) {
						if (CComQIPtr<IMixerPinConfig, &IID_IMixerPinConfig> pMPC = pUnk.p) {
							pMPC->SetAspectRatioMode(AM_ARMODE_STRETCHED);
						}
					}

					if (CComQIPtr<IVMRAspectRatioControl> pARC = pBF.p) {
						pARC->SetAspectRatioMode(VMR_ARMODE_NONE);
					}

					if (CComQIPtr<IVMRAspectRatioControl9> pARC = pBF.p) {
						pARC->SetAspectRatioMode(VMR_ARMODE_NONE);
					}

					if (CComQIPtr<IVMRMixerControl9> pVMRMC9 = pBF.p) {
						m_pUnks.emplace_back(pVMRMC9);
					}

					CComQIPtr<IMFVideoMixerBitmap> pMFVMB = pBF.p; // get custom EVR-CP or EVR-Sync interface
					if (pMFVMB) {
						m_pUnks.emplace_back(pMFVMB);
					}

					if (CComQIPtr<IMFGetService> pMFGS = pBF.p) {
						CComPtr<IMFVideoDisplayControl>	pMFVDC;
						CComPtr<IMFVideoProcessor>		pMFVP;

						if (SUCCEEDED(pMFGS->GetService(MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&pMFVDC)))) {
							m_pUnks.emplace_back(pMFVDC);
						}

						if (!pMFVMB && SUCCEEDED(pMFGS->GetService(MR_VIDEO_MIXER_SERVICE, IID_PPV_ARGS(&pMFVMB)))) {
							m_pUnks.emplace_back(pMFVMB);
						}

						if (SUCCEEDED(pMFGS->GetService(MR_VIDEO_MIXER_SERVICE, IID_PPV_ARGS(&pMFVP)))) {
							m_pUnks.emplace_back(pMFVP);
						}

						if (!m_bIsPreview && pMadVRAllocatorPresenter) {
							// hook IDirectXVideoDecoderService to get DXVA status & logging;
							// madVR crash on call ::GetService() before connect - so set Hook after ConnectFilterDirect()
							hookDirectXVideoDecoderService(pMFGS);
						}
					}

					DLog(L"FGM: '%s' Successfully connected", pFGF->GetName());

					return hr;
				}
			}

			DLog(L"FGM: Connecting '%s' FAILED!", pFGF->GetName());

			EXECUTE_ASSERT(SUCCEEDED(RemoveFilter(pBF)));
			pUnks.clear();
			pBF.Release();
		}
	}

	if (fDeadEnd) {
		auto psde = std::make_unique<CStreamDeadEnd>();
		psde->insert(psde->end(), m_streampath.begin(), m_streampath.end());
		int skip = 0;
		BeginEnumMediaTypes(pPinOut, pEM, pmt) {
			if (pmt->majortype == MEDIATYPE_Stream && pmt->subtype == MEDIASUBTYPE_NULL) {
				skip++;
			}
			psde->mts.emplace_back(*pmt);
		}
		EndEnumMediaTypes(pmt)
		if (skip < (int)psde->mts.size()) {
			m_deadends.emplace_back(std::move(psde));
		}
	}

	return pPinIn ? VFW_E_CANNOT_CONNECT : VFW_E_CANNOT_RENDER;
}

STDMETHODIMP CFGManager::Render(IPin* pPinOut)
{
	CAutoLock cAutoLock(this);

	return RenderEx(pPinOut, 0, nullptr);
}

STDMETHODIMP CFGManager::RenderFile(LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrPlayList)
{
	DLog(L"CFGManager::RenderFile() on thread: %u", GetCurrentThreadId());

	std::unique_lock<std::mutex> lock(m_mutexRender);

	CAutoLock cAutoLock(this);

	m_streampath.clear();
	m_deadends.clear();

	HRESULT hr;

	/*
	CComPtr<IBaseFilter> pBF;
	if (FAILED(hr = AddSourceFilter(lpcwstrFile, lpcwstrFile, &pBF)))
		return hr;

	return ConnectFilter(pBF, nullptr);
	*/

	CFGFilterList fl;
	if (FAILED(hr = EnumSourceFilters(lpcwstrFileName, fl))) {
		return hr;
	}

	std::vector<std::unique_ptr<CStreamDeadEnd>> deadends;

	hr = VFW_E_CANNOT_RENDER;

	for (unsigned pos = 0, fltnum = fl.GetSortedSize(); pos < fltnum; pos++) {
		CFGFilter* pFGF = fl.GetFilter(pos);
		CComPtr<IBaseFilter> pBF;

		if (SUCCEEDED(hr = AddSourceFilterInternal(pFGF, lpcwstrFileName, pFGF->GetName(), &pBF))) {
			m_streampath.clear();
			m_deadends.clear();

			hr = ConnectFilter(pBF, nullptr);
			if (SUCCEEDED(hr)) {
				m_bOpeningAborted = false;
				return hr;
			}

			NukeDownstream(pBF);
			RemoveFilter(pBF);

			for (auto& de : m_deadends) {
				deadends.emplace_back(std::move(de));
			}
		} else if (hr == E_ABORT) {
			m_bOpeningAborted = false;
			return hr;
		}
	}

	m_deadends = std::move(deadends);
	m_bOpeningAborted = false;

	return hr;
}

STDMETHODIMP CFGManager::AddSourceFilter(LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrFilterName, IBaseFilter** ppFilter)
{
	CAutoLock cAutoLock(this);

	HRESULT hr;

	CFGFilterList fl;
	if (FAILED(hr = EnumSourceFilters(lpcwstrFileName, fl))) {
		return hr;
	}

	for (unsigned pos = 0, fltnum = fl.GetSortedSize(); pos < fltnum; pos++) {
		CFGFilter* pFGF = fl.GetFilter(pos);
		if (SUCCEEDED(hr = AddSourceFilterInternal(pFGF, lpcwstrFileName, lpcwstrFilterName, ppFilter))) {
			return hr;
		}
	}

	return VFW_E_CANNOT_LOAD_SOURCE_FILTER;
}

STDMETHODIMP CFGManager::SetLogFile(DWORD_PTR hFile)
{
	if (!m_pUnkInner) {
		return E_UNEXPECTED;
	}

	CAutoLock cAutoLock(this);

	return CComQIPtr<IFilterGraph2>(m_pUnkInner)->SetLogFile(hFile);
}

STDMETHODIMP CFGManager::Abort()
{
	if (!m_pUnkInner) {
		return E_UNEXPECTED;
	}

	m_bOpeningAborted = true;
	std::unique_lock<std::mutex> lock(m_mutexRender);

	CAutoLock cAutoLock(this);

	return CComQIPtr<IFilterGraph2>(m_pUnkInner)->Abort();
}

STDMETHODIMP CFGManager::ShouldOperationContinue()
{
	if (!m_pUnkInner) {
		return E_UNEXPECTED;
	}

	CAutoLock cAutoLock(this);

	return CComQIPtr<IFilterGraph2>(m_pUnkInner)->ShouldOperationContinue();
}

// IFilterGraph2

STDMETHODIMP CFGManager::AddSourceFilterForMoniker(IMoniker* pMoniker, IBindCtx* pCtx, LPCWSTR lpcwstrFilterName, IBaseFilter** ppFilter)
{
	if (!m_pUnkInner) {
		return E_UNEXPECTED;
	}

	CAutoLock cAutoLock(this);

	return CComQIPtr<IFilterGraph2>(m_pUnkInner)->AddSourceFilterForMoniker(pMoniker, pCtx, lpcwstrFilterName, ppFilter);
}

STDMETHODIMP CFGManager::ReconnectEx(IPin* ppin, const AM_MEDIA_TYPE* pmt)
{
	if (!m_pUnkInner) {
		return E_UNEXPECTED;
	}

	CAutoLock cAutoLock(this);

	return CComQIPtr<IFilterGraph2>(m_pUnkInner)->ReconnectEx(ppin, pmt);
}

STDMETHODIMP CFGManager::RenderEx(IPin* pPinOut, DWORD dwFlags, DWORD* pvContext)
{
	CAutoLock cAutoLock(this);

	m_streampath.clear();
	m_deadends.clear();

	if (!pPinOut || dwFlags > AM_RENDEREX_RENDERTOEXISTINGRENDERERS || pvContext) {
		return E_INVALIDARG;
	}

	if (dwFlags & AM_RENDEREX_RENDERTOEXISTINGRENDERERS) {
		std::list<CComQIPtr<IBaseFilter>> pBFs;

		BeginEnumFilters(this, pEF, pBF) {
			if (CComQIPtr<IAMFilterMiscFlags> pAMMF = pBF.p) {
				if (pAMMF->GetMiscFlags() & AM_FILTER_MISC_FLAGS_IS_RENDERER) {
					pBFs.emplace_back(pBF);
				}
			} else {
				BeginEnumPins(pBF, pEP, pPin) {
					CComPtr<IPin> pPinIn;
					DWORD size = 1;
					if (SUCCEEDED(pPin->QueryInternalConnections(&pPinIn, &size)) && size == 0) {
						pBFs.emplace_back(pBF);
						break;
					}
				}
				EndEnumPins;
			}
		}
		EndEnumFilters;

		while (pBFs.size()) {
			HRESULT hr = ConnectFilter(pPinOut, pBFs.front());
			pBFs.pop_front();
			if (SUCCEEDED(hr)) {
				return hr;
			}
		}

		return VFW_E_CANNOT_RENDER;
	}

	return Connect(pPinOut, (IPin*)nullptr);
}

// IGraphBuilder2

STDMETHODIMP CFGManager::IsPinDirection(IPin* pPin, PIN_DIRECTION dir1)
{
	CAutoLock cAutoLock(this);

	CheckPointer(pPin, E_POINTER);

	PIN_DIRECTION dir2;
	if (FAILED(pPin->QueryDirection(&dir2))) {
		return E_FAIL;
	}

	return dir1 == dir2 ? S_OK : S_FALSE;
}

STDMETHODIMP CFGManager::IsPinConnected(IPin* pPin)
{
	CAutoLock cAutoLock(this);

	CheckPointer(pPin, E_POINTER);

	CComPtr<IPin> pPinTo;
	return SUCCEEDED(pPin->ConnectedTo(&pPinTo)) && pPinTo ? S_OK : S_FALSE;
}

static bool FindMT(IPin* pPin, const GUID majortype)
{
	BeginEnumMediaTypes(pPin, pEM, pmt) {
		if (pmt->majortype == majortype) {
			return true;
		}
	}
	EndEnumMediaTypes(pmt)

	return false;
}

HRESULT CFGManager::ConnectFilterDirect(IPin* pPinOut, CFGFilter* pFGF)
{
	HRESULT hr = S_OK;

	CComPtr<IBaseFilter> pBF;
	std::list<CComQIPtr<IUnknown, &IID_IUnknown>> pUnks;
	if (FAILED(hr = pFGF->Create(&pBF, pUnks))) {
		return hr;
	}

	if (FAILED(hr = AddFilter(pBF, pFGF->GetName()))) {
		pBF.Release();
		return hr;
	}

	hr = ConnectFilterDirect(pPinOut, pBF, nullptr);

	if (FAILED(hr)) {
		EXECUTE_ASSERT(SUCCEEDED(RemoveFilter(pBF)));
		DLog(L"FGM: Connecting '%s' FAILED!", pFGF->GetName());
		pBF.Release();
	}

	return hr;
}

STDMETHODIMP CFGManager::ConnectFilter(IBaseFilter* pBF, IPin* pPinIn)
{
	CAutoLock cAutoLock(this);

	CheckPointer(pBF, E_POINTER);

	if (pPinIn && S_OK != IsPinDirection(pPinIn, PINDIR_INPUT)) {
		return VFW_E_INVALID_DIRECTION;
	}

	int nTotal = 0, nRendered = 0;

	CAppSettings& s = AfxGetAppSettings();
	const CRenderersSettings& rs = s.m_VRSettings;

	BeginEnumPins(pBF, pEP, pPin) {
		if (S_OK == IsPinDirection(pPin, PINDIR_OUTPUT) && S_OK != IsPinConnected(pPin)) {

			if (GetPinName(pPin)[0] == '~') {
				if (rs.iVideoRenderer != VIDRNDT_EVR_CP
					&& rs.iVideoRenderer != VIDRNDT_EVR
					&& rs.iVideoRenderer != VIDRNDT_SYNC) {
					continue;
				}
				if (FindMT(pPin, MEDIATYPE_AUXLine21Data)) {
					CLSID clsid;
					pBF->GetClassID(&clsid);

					// Disable MEDIATYPE_AUXLine21Data - prevent connect Line 21 Decoder
					if (clsid == CLSID_NvidiaVideoDecoder || clsid == CLSID_SonicCinemasterVideoDecoder) {
						continue;
					}

					// HACK: block any Line21 connections, if Line 21 Decoder is blocked
					// TODO: understand why lock in the filter does not work
					bool bBlockLine21Decoder2 = false;
					for (const auto& pFGF : m_override) {
						if (pFGF->GetCLSID() == CLSID_Line21Decoder2) {
							if (pFGF->GetMerit() == MERIT64_DO_NOT_USE) {
								bBlockLine21Decoder2 = true;
							}
							break;
						}
					}
					if (bBlockLine21Decoder2) {
						continue;
					}
				}
			}

			m_streampath.Append(pBF, pPin);
			HRESULT hr = S_OK;

			BOOL bInfPinTeeConnected = FALSE;
			if (s.fDualAudioOutput) {
				if (CComQIPtr<IAudioSwitcherFilter> pASF = pBF) {
					BeginEnumMediaTypes(pPin, pEM, pmt) {
						// Find the Audio out pin
						if (pmt->majortype == MEDIATYPE_Audio && pPinIn == nullptr) {
							// Add infinite Pin Tee Filter
							CComPtr<IBaseFilter> pInfPinTee;
							pInfPinTee.CoCreateInstance(CLSID_InfTee);
							AddFilter(pInfPinTee, L"Infinite Pin Tee");

							hr = ConnectFilterDirect(pPin, pInfPinTee, nullptr);
							if (SUCCEEDED(hr)) {
								bInfPinTeeConnected = TRUE;
								CString SelAudioRenderer = s.SelectedAudioRenderer();
								for (int ar = 0; ar < 2; ar++) {
									IPin *infTeeFilterOutPin = GetFirstDisconnectedPin(pInfPinTee, PINDIR_OUTPUT);

									BOOL bIsConnected = FALSE;

									if (!SelAudioRenderer.IsEmpty()) {

										// looking at the list of filters
										for (const auto& pFGF : m_transform) {
											if (SelAudioRenderer == pFGF->GetName()) {
												hr = ConnectFilterDirect(infTeeFilterOutPin, pFGF);
												if (SUCCEEDED(hr)) {
													DLog(L"FGM: Connect Direct to '%s'", pFGF->GetName());
													bIsConnected = TRUE;
													break;
												}
											}
										}

										if (!bIsConnected) {

											// looking at the list of AudioRenderer
											BeginEnumSysDev(CLSID_AudioRendererCategory, pMoniker) {
												CFGFilterRegistry f(pMoniker);

												if (SelAudioRenderer == f.GetDisplayName()) {
													hr = ConnectFilterDirect(infTeeFilterOutPin, &f);
													if (SUCCEEDED(hr)) {
														DLog(L"FGM: Connect Direct to '%s'", f.GetName());
														bIsConnected = TRUE;
														break;
													}
												}
											}
											EndEnumSysDev
										}
									} else {

										// connect to 'Default DirectSound Device'
										CComPtr<IEnumMoniker> pEM;
										GUID guids[] = {MEDIATYPE_Audio, MEDIASUBTYPE_NULL};

										if (SUCCEEDED(m_pFM->EnumMatchingFilters(&pEM, 0, FALSE, MERIT_DO_NOT_USE+1,
													  TRUE, 1, guids, nullptr, nullptr, TRUE, FALSE, 0, nullptr, nullptr, nullptr))) {
											for (CComPtr<IMoniker> pMoniker; S_OK == pEM->Next(1, &pMoniker, nullptr); pMoniker = nullptr) {
												CFGFilterRegistry f(pMoniker);

												if (f.GetName() == L"Default DirectSound Device") {
													hr = ConnectFilterDirect(infTeeFilterOutPin, &f);
													if (SUCCEEDED(hr)) {
														DLog(L"FGM: Connect Direct to '%s'", f.GetName());
														bIsConnected = TRUE;
														break;
													}
												}
											}
										}


									}

									if (!bIsConnected) {
										hr = Connect(infTeeFilterOutPin, pPinIn);
									}

									SelAudioRenderer = s.strAudioRendererDisplayName2;
								}
							}
							break;
						}
					}
					EndEnumMediaTypes(pmt)
				}
			}

			if (!bInfPinTeeConnected) {
				hr = Connect(pPin, pPinIn);
			}

			if (SUCCEEDED(hr)) {
				for (ptrdiff_t i = m_deadends.size() - 1; i >= 0; i--) {
					if (m_deadends[i]->Compare(m_streampath)) {
						auto it = m_deadends.begin();
						std::advance(it, i);
						m_deadends.erase(it);
					}
				}
				nRendered++;
			}

			nTotal++;

			m_streampath.pop_back();

			if (SUCCEEDED(hr) && pPinIn) {
				return S_OK;
			}
		}
	}
	EndEnumPins;

	return
		nRendered == nTotal ? (nRendered > 0 ? S_OK : S_FALSE) :
			nRendered > 0 ? VFW_S_PARTIAL_RENDER :
			VFW_E_CANNOT_RENDER;
}

STDMETHODIMP CFGManager::ConnectFilter(IPin* pPinOut, IBaseFilter* pBF)
{
	CAutoLock cAutoLock(this);

	CheckPointer(pPinOut, E_POINTER);
	CheckPointer(pBF, E_POINTER);

	if (S_OK != IsPinDirection(pPinOut, PINDIR_OUTPUT)) {
		return VFW_E_INVALID_DIRECTION;
	}

	BeginEnumPins(pBF, pEP, pPin) {
		if (S_OK == IsPinDirection(pPin, PINDIR_INPUT) && S_OK != IsPinConnected(pPin)) {
			HRESULT hr = Connect(pPinOut, pPin);
			if (SUCCEEDED(hr)) {
				return hr;
			}
		}
	}
	EndEnumPins;

	return VFW_E_CANNOT_CONNECT;
}

STDMETHODIMP CFGManager::ConnectFilterDirect(IPin* pPinOut, IBaseFilter* pBF, const AM_MEDIA_TYPE* pmt)
{
	CAutoLock cAutoLock(this);

	CheckPointer(pPinOut, E_POINTER);
	CheckPointer(pBF, E_POINTER);

	if (S_OK != IsPinDirection(pPinOut, PINDIR_OUTPUT)) {
		return VFW_E_INVALID_DIRECTION;
	}

	BeginEnumPins(pBF, pEP, pPin) {
		if (S_OK == IsPinDirection(pPin, PINDIR_INPUT) && S_OK != IsPinConnected(pPin)) {

			CLSID clsid;
			pBF->GetClassID(&clsid);
			// Disable Line21 Decoder when not in DVD playback mode
			if (clsid == CLSID_Line21Decoder || clsid == CLSID_Line21Decoder2) {
				if (!FindFilter(CLSID_DVDNavigator, this)) {
					continue;
				}
			}

			HRESULT hr = ConnectDirect(pPinOut, pPin, pmt);
			if (SUCCEEDED(hr)) {
				return hr;
			}
		}
	}
	EndEnumPins;

	return VFW_E_CANNOT_CONNECT;
}

STDMETHODIMP CFGManager::NukeDownstream(IUnknown* pUnk)
{
	CAutoLock cAutoLock(this);

	if (CComQIPtr<IBaseFilter> pBF = pUnk) {
		BeginEnumPins(pBF, pEP, pPin) {
			NukeDownstream(pPin);
		}
		EndEnumPins;
	} else if (CComQIPtr<IPin> pPin = pUnk) {
		CComPtr<IPin> pPinTo;
		if (S_OK == IsPinDirection(pPin, PINDIR_OUTPUT)
				&& SUCCEEDED(pPin->ConnectedTo(&pPinTo)) && pPinTo) {
			if (pBF = GetFilterFromPin(pPinTo)) {
				if (GetCLSID(pBF) == CLSID_EnhancedVideoRenderer) {
					// GetFilterFromPin() returns pointer to the Base EVR,
					// but we need to remove Outer EVR from the graph.
					CComPtr<IBaseFilter> pOuterEVR;
					if (SUCCEEDED(pBF->QueryInterface(&pOuterEVR))) {
						pBF = pOuterEVR;
					}
				}
				NukeDownstream(pBF);
				Disconnect(pPinTo);
				Disconnect(pPin);
				RemoveFilter(pBF);
			}
		}
	} else {
		return E_INVALIDARG;
	}

	return S_OK;
}

STDMETHODIMP CFGManager::FindInterface(REFIID iid, void** ppv, BOOL bRemove)
{
	CAutoLock cAutoLock(this);

	CheckPointer(ppv, E_POINTER);

	for (auto it = m_pUnks.cbegin(); it != m_pUnks.cend(); ++it) {
		if (SUCCEEDED((*it)->QueryInterface(iid, ppv))) {
			if (bRemove) {
				m_pUnks.erase(it);
			}
			return S_OK;
		}
	}

	return E_NOINTERFACE;
}

STDMETHODIMP CFGManager::AddToROT()
{
	CAutoLock cAutoLock(this);

	HRESULT hr;

	if (m_dwRegister) {
		return S_FALSE;
	}

	CComPtr<IRunningObjectTable> pROT;
	CComPtr<IMoniker> pMoniker;
	WCHAR wsz[256];
	swprintf_s(wsz, std::size(wsz), L"FilterGraph %p pid %08x (MPC)", this, GetCurrentProcessId());
	if (SUCCEEDED(hr = GetRunningObjectTable(0, &pROT))
			&& SUCCEEDED(hr = CreateItemMoniker(L"!", wsz, &pMoniker))) {
		hr = pROT->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, (IGraphBuilder2*)this, pMoniker, &m_dwRegister);
	}

	return hr;
}

STDMETHODIMP CFGManager::RemoveFromROT()
{
	CAutoLock cAutoLock(this);

	HRESULT hr;

	if (!m_dwRegister) {
		return S_FALSE;
	}

	CComPtr<IRunningObjectTable> pROT;
	if (SUCCEEDED(hr = GetRunningObjectTable(0, &pROT))
			&& SUCCEEDED(hr = pROT->Revoke(m_dwRegister))) {
		m_dwRegister = 0;
	}

	return hr;
}

// IGraphBuilderDeadEnd

STDMETHODIMP_(size_t) CFGManager::GetCount()
{
	CAutoLock cAutoLock(this);

	return m_deadends.size();
}

STDMETHODIMP CFGManager::GetDeadEnd(int iIndex, std::list<CStringW>& path, std::list<CMediaType>& mts)
{
	CAutoLock cAutoLock(this);

	if (iIndex < 0 || iIndex >= (int)m_deadends.size()) {
		return E_FAIL;
	}

	path.clear();
	mts.clear();

	auto& deadend = *m_deadends[iIndex];

	for (const auto& item : deadend) {
		CStringW str;
		str.Format(L"%s::%s", item.filter, item.pin);
		path.emplace_back(str);
	}

	mts.insert(mts.end(), deadend.mts.begin(), deadend.mts.end());

	return S_OK;
}

// IGraphBuilderSub

STDMETHODIMP CFGManager::RenderSubFile(LPCWSTR lpcwstrFileName)
{
	DLog(L"CFGManager::RenderSubFile() on thread: %u", GetCurrentThreadId());
	CAutoLock cAutoLock(this);

	HRESULT hr = VFW_E_CANNOT_RENDER;

	m_bOnlySub = TRUE;

	// support only .mks - use internal MatroskaSource
	CFGFilter* pFG = DNew CFGFilterInternal<CMatroskaSourceFilter>();

	CComPtr<IBaseFilter> pBF;
	if (SUCCEEDED(hr = AddSourceFilterInternal(pFG, lpcwstrFileName, pFG->GetName(), &pBF))) {
		m_streampath.clear();
		m_deadends.clear();

		hr = ConnectFilter(pBF, nullptr);
	}
	m_bOnlySub = FALSE;

	delete pFG;

	return hr;
}

// IGraphBuilderAudio

STDMETHODIMP CFGManager::RenderAudioFile(LPCWSTR lpcwstrFileName)
{
	m_bOnlyAudio = TRUE;
	HRESULT hr = RenderFile(lpcwstrFileName, nullptr);
	m_bOnlyAudio = FALSE;

	return hr;
}

//
// CFGManagerCustom
//

CFGManagerCustom::CFGManagerCustom(LPCWSTR pName, LPUNKNOWN pUnk, HWND hWnd, bool IsPreview)
	: CFGManager(pName, pUnk, hWnd, IsPreview)
{
	CAppSettings& s = AfxGetAppSettings();
	const CRenderersSettings& rs = s.m_VRSettings;

	CFGFilter* pFGF;

	bool *src   = s.SrcFilters;
	bool *video = s.VideoFilters;
	bool *audio = s.AudioFilters;

	// Source filters

	if (src[SRC_SHOUTCAST] && !IsPreview) {
		pFGF = DNew CFGFilterInternal<CShoutcastSource>(ShoutcastSourceName);
		pFGF->m_protocols.emplace_back(L"http");
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_UDP] && !IsPreview) {
		pFGF = DNew CFGFilterInternal<CUDPReader>(StreamReaderName);
		pFGF->m_protocols.emplace_back(L"udp");
		pFGF->m_protocols.emplace_back(L"http");
		pFGF->m_protocols.emplace_back(L"https");
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_STDINPUT] && !IsPreview) {
		pFGF = DNew CFGFilterInternal<CUDPReader>(StreamReaderName);
		pFGF->m_protocols.emplace_back(L"pipe");
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_AVI] || IsPreview) {
		pFGF = DNew CFGFilterInternal<CAviSourceFilter>(AviSourceName);
		pFGF->m_chkbytes.emplace_back(L"0,4,,52494646,8,4,,41564920"); // 'RIFF....AVI '
		pFGF->m_chkbytes.emplace_back(L"0,4,,52494646,8,4,,41564958"); // 'RIFF....AVIX'
		pFGF->m_chkbytes.emplace_back(L"0,4,,52494646,8,4,,414D5620"); // 'RIFF....AMV '
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_MP4] || IsPreview) {
		pFGF = DNew CFGFilterInternal<CMP4SourceFilter>(MP4SourceName);
		// mov, mp4
		pFGF->m_chkbytes.emplace_back(L"4,4,,66747970"); // '....ftyp'
		pFGF->m_chkbytes.emplace_back(L"4,4,,6d6f6f76"); // '....moov'
		pFGF->m_chkbytes.emplace_back(L"4,4,,6d646174"); // '....mdat'
		pFGF->m_chkbytes.emplace_back(L"4,4,,77696465"); // '....wide'
		pFGF->m_chkbytes.emplace_back(L"4,4,,736b6970"); // '....skip'
		pFGF->m_chkbytes.emplace_back(L"4,4,,66726565"); // '....free'
		pFGF->m_chkbytes.emplace_back(L"4,4,,706e6f74"); // '....pnot'
		pFGF->m_extensions.emplace_back(L".mov");
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_FLV] || IsPreview) {
		pFGF = DNew CFGFilterInternal<CFLVSourceFilter>(FlvSourceName);
		pFGF->m_chkbytes.emplace_back(L"0,4,,464C5601");   // FLV\01
		pFGF->m_chkbytes.emplace_back(L"0,4,,584C5646");   // XLVF
		pFGF->m_chkbytes.emplace_back(L"0,5,,4B444B0000"); // KDK\00\00
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_MATROSKA] || IsPreview) {
		pFGF = DNew CFGFilterInternal<CMatroskaSourceFilter>(MatroskaSourceName);
		pFGF->m_chkbytes.emplace_back(L"0,4,,1A45DFA3");
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_REAL] || IsPreview) {
		pFGF = DNew CFGFilterInternal<CRealMediaSourceFilter>(RMSourceName);
		pFGF->m_chkbytes.emplace_back(L"0,4,,2E524D46");
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_ROQ] || IsPreview) {
		pFGF = DNew CFGFilterInternal<CRoQSourceFilter>(RoQSourceName);
		pFGF->m_chkbytes.emplace_back(L"0,8,,8410FFFFFFFF1E00");
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_DSM] || IsPreview) {
		pFGF = DNew CFGFilterInternal<CDSMSourceFilter>(DSMSourceName);
		pFGF->m_chkbytes.emplace_back(L"0,4,,44534D53");
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_BINK] || IsPreview) {
		pFGF = DNew CFGFilterInternal<CBinkSourceFilter>(BinkSourceName);
		pFGF->m_chkbytes.emplace_back(L"0,3,,42494B");
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_FLIC] || IsPreview) {
		pFGF = DNew CFGFilterInternal<CFLICSource>(FlicSourceName);
		pFGF->m_chkbytes.emplace_back(L"4,2,,11AF");
		pFGF->m_chkbytes.emplace_back(L"4,2,,12AF");
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_FLAC] && !IsPreview) {
		pFGF = DNew CFGFilterInternal<CAudioSourceFilter>(AudioSourceName);
		pFGF->m_chkbytes.emplace_back(L"0,4,,664C6143");
		pFGF->m_extensions.emplace_back(L".flac");
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_CDDA] && !IsPreview) {
		pFGF = DNew CFGFilterInternal<CCDDAReader>(CCDDAReaderName);
		pFGF->m_extensions.emplace_back(L".cda");
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_CDXA] || IsPreview) {
		pFGF = DNew CFGFilterInternal<CCDXAReader>(CCDXAReaderName);
		pFGF->m_chkbytes.emplace_back(L"0,4,,52494646,8,4,,43445841");
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_VTS] || IsPreview) {
		pFGF = DNew CFGFilterInternal<CVTSReader>(VTSReaderName);
		pFGF->m_chkbytes.emplace_back(L"0,12,,445644564944454F2D565453");
		pFGF->m_chkbytes.emplace_back(L"0,12,,445644415544494F2D415453");
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_OGG] || IsPreview) {
		pFGF = DNew CFGFilterInternal<COggSourceFilter>(OggSourceName);
		pFGF->m_chkbytes.emplace_back(L"0,4,,4F676753");
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_MPA] && !IsPreview) {
		pFGF = DNew CFGFilterInternal<CMpaSourceFilter>(MpaSourceName);
		pFGF->m_chkbytes.emplace_back(L"0,2,FFE0,FFE0");               // Mpeg Audio
		pFGF->m_chkbytes.emplace_back(L"0,10,FFFFFF00000080808080,49443300000000000000");
		pFGF->m_chkbytes.emplace_back(L"0,2,FFE0,56E0");               // AAC LATM
		pFGF->m_extensions.emplace_back(L".mp3");
		pFGF->m_extensions.emplace_back(L".aac");
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_AC3] && !IsPreview) {
		pFGF = DNew CFGFilterInternal<CDTSAC3Source>(DTSAC3SourceName);
		pFGF->m_chkbytes.emplace_back(L"0,2,,0B77");                   // AC3, E-AC3
		pFGF->m_chkbytes.emplace_back(L"0,2,,770B");                   // AC3 LE
		pFGF->m_chkbytes.emplace_back(L"4,4,,F8726FBB");               // MLP
		pFGF->m_chkbytes.emplace_back(L"4,4,,F8726FBA");               // TrueHD
		pFGF->m_extensions.emplace_back(L".ac3");
		pFGF->m_extensions.emplace_back(L".ec3");
		pFGF->m_extensions.emplace_back(L".eac3");
		pFGF->m_extensions.emplace_back(L".thd");
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_DTS] && !IsPreview) {
		pFGF = DNew CFGFilterInternal<CDTSAC3Source>(DTSAC3SourceName);
		pFGF->m_chkbytes.emplace_back(L"0,4,,7FFE8001");               // DTS
		pFGF->m_chkbytes.emplace_back(L"0,4,,fE7f0180");               // DTS LE
		pFGF->m_chkbytes.emplace_back(L"0,4,,64582025");               // DTS Substream
		pFGF->m_extensions.emplace_back(L".dts");
		pFGF->m_extensions.emplace_back(L".dtshd");
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_AMR] && !IsPreview) {
		pFGF = DNew CFGFilterInternal<CAudioSourceFilter>(AudioSourceName);
		pFGF->m_chkbytes.emplace_back(L"0,6,,2321414D520A");           // '#!AMR\n'
		pFGF->m_chkbytes.emplace_back(L"0,9,,2321414D522D57420A");     // '#!AMR-WB\n'
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_APE] && !IsPreview) {
		pFGF = DNew CFGFilterInternal<CAudioSourceFilter>(AudioSourceName);
		pFGF->m_chkbytes.emplace_back(L"0,4,,4D414320");               // 'MAC '
		pFGF->m_chkbytes.emplace_back(L"0,3,,494433");                 // 'ID3'
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_TAK] && !IsPreview) {
		pFGF = DNew CFGFilterInternal<CAudioSourceFilter>(AudioSourceName);
		pFGF->m_chkbytes.emplace_back(L"0,4,,7442614B");               // 'tBaK'
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_TTA] && !IsPreview) {
		pFGF = DNew CFGFilterInternal<CAudioSourceFilter>(AudioSourceName);
		pFGF->m_chkbytes.emplace_back(L"0,4,,54544131");               // 'TTA1'
		pFGF->m_chkbytes.emplace_back(L"0,3,,494433");                 // 'ID3'
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_WAVPACK] && !IsPreview) {
		pFGF = DNew CFGFilterInternal<CAudioSourceFilter>(AudioSourceName);
		pFGF->m_chkbytes.emplace_back(L"0,4,,7776706B");               // 'wvpk'
		pFGF->m_chkbytes.emplace_back(L"0,3,,494433");                 // 'ID3'
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_WAV] && !IsPreview) {
		pFGF = DNew CFGFilterInternal<CAudioSourceFilter>(AudioSourceName);
		pFGF->m_chkbytes.emplace_back(L"0,4,,52494646,8,4,,57415645"); // 'RIFF....WAVE'
		pFGF->m_chkbytes.emplace_back(L"0,16,,726966662E91CF11A5D628DB04C10000,24,16,,77617665F3ACD3118CD100C04F8EDB8A"); // Wave64
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_DSD] && !IsPreview) {
		pFGF = DNew CFGFilterInternal<CAudioSourceFilter>(AudioSourceName);
		pFGF->m_chkbytes.emplace_back(L"0,12,,445344201C00000000000000"); // 'DSD..."
		pFGF->m_chkbytes.emplace_back(L"0,4,,46524D38,12,4,,44534420");   // 'FRM8........DSD '
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_DTS] && !IsPreview) {
		pFGF = DNew CFGFilterInternal<CAudioSourceFilter>(AudioSourceName);
		pFGF->m_chkbytes.emplace_back(L"0,8,,4454534844484452"); // DTSHDHDR
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_MUSEPACK] && !IsPreview) {
		pFGF = DNew CFGFilterInternal<CAudioSourceFilter>(AudioSourceName);
		pFGF->m_chkbytes.emplace_back(L"0,4,,4D50434B"); // MPCK
		pFGF->m_chkbytes.emplace_back(L"0,3,,4D502B");   // MP+
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_DVR] || IsPreview) {
		pFGF = DNew CFGFilterInternal<CDVRSourceFilter>(DVRSourceName);
		pFGF->m_chkbytes.emplace_back(L"0,4,,48585653,16,4,,48585646"); // 'HXVS............HXVF'
		pFGF->m_chkbytes.emplace_back(L"0,4,,44484156");                // 'DHAV'
		pFGF->m_chkbytes.emplace_back(L"0,4,,6C756F20");                // 'luo '
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_AIFF] && !IsPreview) {
		pFGF = DNew CFGFilterInternal<CAudioSourceFilter>(AudioSourceName);
		pFGF->m_chkbytes.emplace_back(L"0,4,,464F524D,8,4,,41494646"); // 'FORM....AIFF'
		m_source.emplace_back(pFGF);
	}

	// add CMpegSourceFilter last since it can parse the stream for a long time
	if (src[SRC_MPEG] || IsPreview) {
		pFGF = DNew CFGFilterInternal<CMpegSourceFilter>(MpegSourceName);
		m_source.emplace_back(pFGF);
	}

	if (src[SRC_RAWVIDEO] || IsPreview) {
		pFGF = DNew CFGFilterInternal<CRawVideoSourceFilter>(RawVideoSourceName);
		pFGF->m_chkbytes.emplace_back(L"0,9,,595556344D50454732");      // YUV4MPEG2
		pFGF->m_chkbytes.emplace_back(L"0,6,,444B49460000");            // 'DKIF\0\0'
		pFGF->m_chkbytes.emplace_back(L"0,3,,000001");                  // MPEG1/2, VC-1
		pFGF->m_chkbytes.emplace_back(L"0,4,,00000001");                // H.264/AVC, H.265/HEVC
		pFGF->m_chkbytes.emplace_back(L"0,4,,434D5331,20,4,,50445652"); // 'CMS1................PDVR'
		pFGF->m_chkbytes.emplace_back(L"0,5,,3236344456");              // '264DV'
		pFGF->m_chkbytes.emplace_back(L"0,4,,FFFFFF88");
		pFGF->m_extensions.emplace_back(L".obu");
		m_source.emplace_back(pFGF);
	}

	// hmmm, shouldn't there be an option in the GUI to enable/disable this filter?
	pFGF = DNew CFGFilterInternal<CAVI2AC3Filter>(AVI2AC3FilterName, MERIT64(0x00680000) + 1);
	pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_WAVE_DOLBY_AC3);
	pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_DTS2);
	m_transform.emplace_back(pFGF);

	if (src[SRC_MATROSKA] || IsPreview) {
		pFGF = DNew CFGFilterInternal<CMatroskaSplitterFilter>(MatroskaSplitterName, MERIT64_ABOVE_DSHOW);
	} else {
		pFGF = DNew CFGFilterInternal<CMatroskaSplitterFilter>(LowMerit(MatroskaSplitterName), MERIT64_DO_USE);
	}
	pFGF->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_Matroska);
	pFGF->AddType(MEDIATYPE_Stream, GUID_NULL);
	m_transform.emplace_back(pFGF);

	if (src[SRC_REAL] || IsPreview) {
		pFGF = DNew CFGFilterInternal<CRealMediaSplitterFilter>(RMSplitterName, MERIT64_ABOVE_DSHOW);
	} else {
		pFGF = DNew CFGFilterInternal<CRealMediaSplitterFilter>(LowMerit(RMSplitterName), MERIT64_DO_USE);
	}
	pFGF->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_RealMedia);
	pFGF->AddType(MEDIATYPE_Stream, GUID_NULL);
	m_transform.emplace_back(pFGF);

	if (src[SRC_ROQ] || IsPreview) {
		pFGF = DNew CFGFilterInternal<CRoQSplitterFilter>(RoQSplitterName, MERIT64_ABOVE_DSHOW);
	} else {
		pFGF = DNew CFGFilterInternal<CRoQSplitterFilter>(LowMerit(RoQSplitterName), MERIT64_DO_USE);
	}
	pFGF->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_RoQ);
	pFGF->AddType(MEDIATYPE_Stream, GUID_NULL);
	m_transform.emplace_back(pFGF);

	if (src[SRC_AVI] || IsPreview) {
		pFGF = DNew CFGFilterInternal<CAviSplitterFilter>(AviSplitterName, MERIT64_ABOVE_DSHOW);
	} else {
		pFGF = DNew CFGFilterInternal<CAviSplitterFilter>(LowMerit(AviSplitterName), MERIT64_DO_USE);
	}
	pFGF->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_Avi);
	pFGF->AddType(MEDIATYPE_Stream, GUID_NULL);
	m_transform.emplace_back(pFGF);

	if (src[SRC_OGG] || IsPreview) {
		pFGF = DNew CFGFilterInternal<COggSplitterFilter>(OggSplitterName, MERIT64_ABOVE_DSHOW);
	} else {
		pFGF = DNew CFGFilterInternal<COggSplitterFilter>(LowMerit(OggSplitterName), MERIT64_DO_USE);
	}
	pFGF->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_Ogg);
	pFGF->AddType(MEDIATYPE_Stream, GUID_NULL);
	m_transform.emplace_back(pFGF);

	if (!IsPreview) {
		if (src[SRC_MPA]) {
			pFGF = DNew CFGFilterInternal<CMpaSplitterFilter>(MpaSplitterName, MERIT64_ABOVE_DSHOW);
		} else {
			pFGF = DNew CFGFilterInternal<CMpaSplitterFilter>(LowMerit(MpaSplitterName), MERIT64_DO_USE);
		}
		pFGF->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_MPEG1Audio);
		pFGF->AddType(MEDIATYPE_Stream, GUID_NULL);
		m_transform.emplace_back(pFGF);
	}

	if (src[SRC_DSM] || IsPreview) {
		pFGF = DNew CFGFilterInternal<CDSMSplitterFilter>(DSMSplitterName, MERIT64_ABOVE_DSHOW);
	} else {
		pFGF = DNew CFGFilterInternal<CDSMSplitterFilter>(LowMerit(DSMSplitterName), MERIT64_DO_USE);
	}
	pFGF->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_DirectShowMedia);
	pFGF->AddType(MEDIATYPE_Stream, GUID_NULL);
	m_transform.emplace_back(pFGF);

	if (src[SRC_MP4] || IsPreview) {
		pFGF = DNew CFGFilterInternal<CMP4SplitterFilter>(MP4SplitterName, MERIT64_ABOVE_DSHOW);
	} else {
		pFGF = DNew CFGFilterInternal<CMP4SplitterFilter>(LowMerit(MP4SplitterName), MERIT64_DO_USE);
	}
	pFGF->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_MP4);
	pFGF->AddType(MEDIATYPE_Stream, GUID_NULL);
	m_transform.emplace_back(pFGF);

	if (src[SRC_FLV] || IsPreview) {
		pFGF = DNew CFGFilterInternal<CFLVSplitterFilter>(FlvSplitterName, MERIT64_ABOVE_DSHOW);
	} else {
		pFGF = DNew CFGFilterInternal<CFLVSplitterFilter>(LowMerit(FlvSplitterName), MERIT64_DO_USE);
	}
	pFGF->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_FLV);
	pFGF->AddType(MEDIATYPE_Stream, GUID_NULL);
	m_transform.emplace_back(pFGF);

	if (src[SRC_BINK] || IsPreview) {
		pFGF = DNew CFGFilterInternal<CBinkSplitterFilter>(BinkSplitterName, MERIT64_ABOVE_DSHOW);
	} else {
		pFGF = DNew CFGFilterInternal<CBinkSplitterFilter>(LowMerit(BinkSplitterName), MERIT64_DO_USE);
	}
	pFGF->AddType(MEDIATYPE_Stream, GUID_NULL);
	m_transform.emplace_back(pFGF);

	if (!IsPreview) {
		pFGF = DNew CFGFilterInternal<CAudioSplitterFilter>(
					(src[SRC_WAV]) ? AudioSplitterName : LowMerit(AudioSplitterName),
					(src[SRC_WAV]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_WAVE);
		m_transform.emplace_back(pFGF);

		pFGF = DNew CFGFilterInternal<CAudioSplitterFilter>(
					(src[SRC_AMR] && src[SRC_APE] && src[SRC_TAK] && src[SRC_TTA] && src[SRC_WAVPACK] && src[SRC_WAV]) ? AudioSplitterName : LowMerit(AudioSplitterName),
					(src[SRC_AMR] && src[SRC_APE] && src[SRC_TAK] && src[SRC_TTA] && src[SRC_WAVPACK] && src[SRC_WAV]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Stream, GUID_NULL);
		m_transform.emplace_back(pFGF);
	}

	if (src[SRC_RAWVIDEO] || IsPreview) {
		pFGF = DNew CFGFilterInternal<CRawVideoSplitterFilter>(RawVideoSplitterName, MERIT64_ABOVE_DSHOW);
	} else {
		pFGF = DNew CFGFilterInternal<CRawVideoSplitterFilter>(LowMerit(RawVideoSplitterName), MERIT64_DO_USE);
	}
	pFGF->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_MPEG1Video);
	pFGF->AddType(MEDIATYPE_Stream, GUID_NULL);
	m_transform.emplace_back(pFGF);

	if (src[SRC_DVR] || IsPreview) {
		pFGF = DNew CFGFilterInternal<CDVRSplitterFilter>(DVRSplitterName, MERIT64_ABOVE_DSHOW);
	} else {
		pFGF = DNew CFGFilterInternal<CDVRSplitterFilter>(LowMerit(DVRSplitterName), MERIT64_DO_USE);
	}
	pFGF->AddType(MEDIATYPE_Stream, GUID_NULL);
	m_transform.emplace_back(pFGF);

	// add CMpegSplitterFilter last since it can parse the stream for a long time
	if (src[SRC_MPEG] || IsPreview) {
		pFGF = DNew CFGFilterInternal<CMpegSplitterFilter>(MpegSplitterName, MERIT64_ABOVE_DSHOW);
	} else {
		pFGF = DNew CFGFilterInternal<CMpegSplitterFilter>(LowMerit(MpegSplitterName), MERIT64_DO_USE);
	}
	pFGF->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_MPEG1System);
	pFGF->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_MPEG2_PROGRAM);
	pFGF->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_MPEG2_TRANSPORT);
	pFGF->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_MPEG2_PVA);
	pFGF->AddType(MEDIATYPE_Stream, GUID_NULL);
	m_transform.emplace_back(pFGF);

	// Transform filters
	if (!IsPreview) {
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_MPA]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_MPA]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_MP3);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_MPEG1AudioPayload);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_MPEG1Payload);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_MPEG1Packet);
		m_transform.emplace_back(pFGF);

		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_MPA]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_MPA]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_DVD_ENCRYPTED_PACK, MEDIASUBTYPE_MPEG2_AUDIO);
		pFGF->AddType(MEDIATYPE_MPEG2_PACK, MEDIASUBTYPE_MPEG2_AUDIO);
		pFGF->AddType(MEDIATYPE_MPEG2_PES, MEDIASUBTYPE_MPEG2_AUDIO);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_MPEG2_AUDIO);
		m_transform.emplace_back(pFGF);

		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_AMR]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_AMR]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_AMR);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_SAMR);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_SAWB);
		m_transform.emplace_back(pFGF);

		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_LPCM]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_LPCM]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_DVD_ENCRYPTED_PACK, MEDIASUBTYPE_DVD_LPCM_AUDIO);
		pFGF->AddType(MEDIATYPE_MPEG2_PACK, MEDIASUBTYPE_DVD_LPCM_AUDIO);
		pFGF->AddType(MEDIATYPE_MPEG2_PES, MEDIASUBTYPE_DVD_LPCM_AUDIO);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_DVD_LPCM_AUDIO);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_HDMV_LPCM_AUDIO);
		m_transform.emplace_back(pFGF);

		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_AC3]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_AC3]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_DVD_ENCRYPTED_PACK, MEDIASUBTYPE_DOLBY_AC3);
		pFGF->AddType(MEDIATYPE_MPEG2_PACK, MEDIASUBTYPE_DOLBY_AC3);
		pFGF->AddType(MEDIATYPE_MPEG2_PES, MEDIASUBTYPE_DOLBY_AC3);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_DOLBY_AC3);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_WAVE_DOLBY_AC3);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_DOLBY_TRUEHD);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_DOLBY_DDPLUS);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_MLP);
		m_transform.emplace_back(pFGF);

		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_DTS]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_DTS]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_DVD_ENCRYPTED_PACK, MEDIASUBTYPE_DTS);
		pFGF->AddType(MEDIATYPE_MPEG2_PACK, MEDIASUBTYPE_DTS);
		pFGF->AddType(MEDIATYPE_MPEG2_PES, MEDIASUBTYPE_DTS);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_DTS);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_DTS2);
		m_transform.emplace_back(pFGF);

		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_AAC]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_AAC]) ? MERIT64_ABOVE_DSHOW + 1 : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_RAW_AAC1);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_LATM_AAC);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_AAC_ADTS);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_MP4A);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_mp4a);
		m_transform.emplace_back(pFGF);

		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_PS2]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_PS2]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_PS2_PCM);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_PS2_ADPCM);
		m_transform.emplace_back(pFGF);
	}

	if (!IsPreview) {
		// RealAudio
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_REAL]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_REAL]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_14_4);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_28_8);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_ATRC);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_COOK);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_DNET);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_SIPR);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_SIPR_WAVE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_RAAC);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_RACP);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_RALF);
		m_transform.emplace_back(pFGF);

		// Vorbis
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_VORBIS]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_VORBIS]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_Vorbis2);
		m_transform.emplace_back(pFGF);

		// FLAC
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_FLAC]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_FLAC]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_FLAC_FRAMED);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_FLAC);
		m_transform.emplace_back(pFGF);

		// Nelly Moser
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_NELLY]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_NELLY]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_NELLYMOSER);
		m_transform.emplace_back(pFGF);

		// Apple Lossless Audio Coding
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_ALAC]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_ALAC]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_ALAC);
		m_transform.emplace_back(pFGF);

		// MPEG-4 Audio Lossless Coding (ALS)
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_ALS]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_ALS]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_ALS);
		m_transform.emplace_back(pFGF);

		// PCM, ADPCM
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_PCM_ADPCM]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_PCM_ADPCM]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_PCM_NONE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_PCM_RAW);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_PCM_TWOS);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_PCM_SOWT);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_PCM_IN24);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_PCM_IN32);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_PCM_FL32);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_PCM_FL64);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_IEEE_FLOAT); // only for 64-bit float PCM
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_ALAW);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_MULAW);
		/* todo: this should not depend on PCM */
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_IMA4);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_ADPCM_SWF);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_IMA_AMV);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_ADX_ADPCM);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_G726_ADPCM);
		// AES3
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_AES3);
		m_transform.emplace_back(pFGF);

		// QDesign Music Codec
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_QDMC]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_QDMC]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_QDMC);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_QDM2);
		m_transform.emplace_back(pFGF);

		// WavPack
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_WAVPACK]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_WAVPACK]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_WAVPACK4);
		m_transform.emplace_back(pFGF);

		// Musepack
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_MUSEPACK]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_MUSEPACK]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_MPC7);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_MPC8);
		m_transform.emplace_back(pFGF);

		// Monkey's Audio (APE)
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_APE]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_APE]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_APE);
		m_transform.emplace_back(pFGF);

		// TAK
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_TAK]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_TAK]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_TAK);
		m_transform.emplace_back(pFGF);

		// TTA
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_TTA]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_TTA]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_TTA1);
		m_transform.emplace_back(pFGF);

		// Shorten
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_SHORTEN]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_SHORTEN]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_Shorten);
		m_transform.emplace_back(pFGF);

		// DSP Group TrueSpeech
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_TRUESPEECH]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_TRUESPEECH]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_TRUESPEECH);
		m_transform.emplace_back(pFGF);

		// Voxware MetaSound
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_VOXWARE]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_VOXWARE]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_VOXWARE_RT29);
		m_transform.emplace_back(pFGF);

		// Bink Audio
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_BINK]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_BINK]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_BINKA_DCT);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_BINKA_RDFT);
		m_transform.emplace_back(pFGF);

		// Windows Media Audio 9 Professional
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_WMA9]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_WMA9]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_WMAUDIO3);
		m_transform.emplace_back(pFGF);

		// Windows Media Audio Lossless
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_WMALOSSLESS]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_WMALOSSLESS]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_WMAUDIO_LOSSLESS);
		m_transform.emplace_back(pFGF);

		// Windows Media Audio 1, 2
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_WMA]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_WMA]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_MSAUDIO1);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_WMAUDIO2);
		m_transform.emplace_back(pFGF);

		// Windows Media Audio Voice
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_WMAVOICE]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_WMAVOICE]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_WMSP1);
		m_transform.emplace_back(pFGF);

		// Indeo Audio
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_INDEO]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_INDEO]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_INDEO_AUDIO);
		m_transform.emplace_back(pFGF);

		// Opus
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_OPUS]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_OPUS]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_OPUS);
		m_transform.emplace_back(pFGF);

		// Speex
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
					(audio[ADEC_SPEEX]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
					(audio[ADEC_SPEEX]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_SPEEX);
		m_transform.emplace_back(pFGF);

		// DSD
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(
			(audio[ADEC_DSD]) ? MPCAudioDecName : LowMerit(MPCAudioDecName),
			(audio[ADEC_DSD]) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_DSD);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_DSDL);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_DSDM);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_DSD1);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_DSD8);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_DST);
		m_transform.emplace_back(pFGF);

		// other audio
		pFGF = DNew CFGFilterInternal<CMpaDecFilter>(MPCAudioDecName, MERIT64_ABOVE_DSHOW);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_ATRAC3);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_ATRAC3plus);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_ATRAC9);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_INTEL_MUSIC);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_ON2VP7_AUDIO);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_ON2VP6_AUDIO);
		m_transform.emplace_back(pFGF);

		// subtitles
		pFGF = DNew CFGFilterInternal<CNullTextRenderer>(L"NullTextRenderer", MERIT64_DO_USE);
		pFGF->AddType(MEDIATYPE_Text, MEDIASUBTYPE_NULL);
		pFGF->AddType(MEDIATYPE_ScriptCommand, MEDIASUBTYPE_NULL);
		pFGF->AddType(MEDIATYPE_Subtitle, MEDIASUBTYPE_NULL);
		pFGF->AddType(MEDIATYPE_Text, MEDIASUBTYPE_NULL);
		pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_DVD_SUBPICTURE);
		pFGF->AddType(MEDIATYPE_NULL, MEDIASUBTYPE_CVD_SUBPICTURE);
		pFGF->AddType(MEDIATYPE_NULL, MEDIASUBTYPE_SVCD_SUBPICTURE);
		pFGF->AddType(MEDIATYPE_NULL, MEDIASUBTYPE_XSUB);
		pFGF->AddType(MEDIATYPE_NULL, MEDIASUBTYPE_DVB_SUBTITLES);
		m_transform.emplace_back(pFGF);
	}

	// High merit MPC Video Decoder
	pFGF = DNew CFGMPCVideoDecoderInternal(MPCVideoDecName, MERIT64_ABOVE_DSHOW + 1, IsPreview);
	m_transform.emplace_back(pFGF);

	// Low merit MPC Video Decoder
	if (!IsPreview) { // do not need for Preview mode.
		pFGF = DNew CFGMPCVideoDecoderInternal(LowMerit(MPCVideoDecName));
		m_transform.emplace_back(pFGF);
	}

	pFGF = DNew CFGFilterInternal<CMPCVideoDecFilter>(
				(video[VDEC_UNCOMPRESSED] || IsPreview) ? MPCVideoConvName : LowMerit(MPCVideoConvName),
				(video[VDEC_UNCOMPRESSED] || IsPreview) ? (MERIT64_PREFERRED - 0x100) : MERIT64_DO_USE); // merit of video converter must be lower than merit of video renderers
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_v210);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_V410);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_r210);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_R10g);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_R10k);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_AVrp);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_Y8);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_Y800);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_Y16);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_I420);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_Y41B);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_Y42B);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_444P);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_cyuv);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_YVU9);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_IYUV);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_UYVY);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_YUY2);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_NV12);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_YV12);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_YV16);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_YV24);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_BGR48);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_BGRA64);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_b48r);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_b64a);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_LAV_RAWVIDEO);
	m_transform.emplace_back(pFGF);

	// Keep Mpeg2DecFilter after DXVA/ffmpeg decoder !
	pFGF = DNew CFGFilterInternal<CMpeg2DecFilter>(
				(video[VDEC_DVD] || IsPreview) ? DvdVideoDecoderName : LowMerit(DvdVideoDecoderName),
				(video[VDEC_DVD] || IsPreview) ? MERIT64_ABOVE_DSHOW : MERIT64_DO_USE);
	// MPC-BE uses this filter for DVD-Video only
	pFGF->AddType(MEDIATYPE_DVD_ENCRYPTED_PACK, MEDIASUBTYPE_MPEG2_VIDEO); // used by for MPEG-2 and MPEG-1
	//pFGF->AddType(MEDIATYPE_MPEG2_PES, MEDIASUBTYPE_MPEG2_VIDEO);
	m_transform.emplace_back(pFGF);

	// TODO - make optional RoQ A/V decoder
	pFGF = DNew CFGFilterInternal<CRoQVideoDecoder>(RoQVideoDecoderName, MERIT64_ABOVE_DSHOW);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_RoQV);
	m_transform.emplace_back(pFGF);

	if (!IsPreview) {
		pFGF = DNew CFGFilterInternal<CRoQAudioDecoder>(RoQAudioDecoderName, MERIT64_ABOVE_DSHOW);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_RoQA);
		m_transform.emplace_back(pFGF);
	}

	// Blocked filters

	// "Subtitle Mixer" makes an access violation around the
	// 11-12th media type when enumerating them on its output.
	m_transform.emplace_back(DNew CFGFilterRegistry(GUIDFromCString(L"{00A95963-3BE5-48C0-AD9F-3356D67EA09D}"), MERIT64_DO_NOT_USE));

	// DiracSplitter.ax is crashing MPC-BE when opening invalid files...
	m_transform.emplace_back(DNew CFGFilterRegistry(GUIDFromCString(L"{09E7F58E-71A1-419D-B0A0-E524AE1454A9}"), MERIT64_DO_NOT_USE));
	m_transform.emplace_back(DNew CFGFilterRegistry(GUIDFromCString(L"{5899CFB9-948F-4869-A999-5544ECB38BA5}"), MERIT64_DO_NOT_USE));
	m_transform.emplace_back(DNew CFGFilterRegistry(GUIDFromCString(L"{F78CF248-180E-4713-B107-B13F7B5C31E1}"), MERIT64_DO_NOT_USE));

	// ISCR suxx
	m_transform.emplace_back(DNew CFGFilterRegistry(GUIDFromCString(L"{48025243-2D39-11CE-875D-00608CB78066}"), MERIT64_DO_NOT_USE));

	// Samsung's "mpeg-4 demultiplexor" can even open matroska files, amazing...
	m_transform.emplace_back(DNew CFGFilterRegistry(GUIDFromCString(L"{99EC0C72-4D1B-411B-AB1F-D561EE049D94}"), MERIT64_DO_NOT_USE));

	// LG Video Renderer (lgvid.ax) just crashes when trying to connect it
	m_transform.emplace_back(DNew CFGFilterRegistry(GUIDFromCString(L"{9F711C60-0668-11D0-94D4-0000C02BA972}"), MERIT64_DO_NOT_USE));

	// palm demuxer crashes (even crashes graphedit when dropping an .ac3 onto it)
	m_transform.emplace_back(DNew CFGFilterRegistry(GUIDFromCString(L"{BE2CF8A7-08CE-4A2C-9A25-FD726A999196}"), MERIT64_DO_NOT_USE));

	{ // DCDSPFilter (early versions crash mpc)
		CRegKey key;
		WCHAR buff[256] = { 0 };
		ULONG len = std::size(buff);
		CString clsid = L"{B38C58A0-1809-11D6-A458-EDAE78F1DF12}";

		if (ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, L"CLSID\\" + clsid + L"\\InprocServer32", KEY_READ)
				&& ERROR_SUCCESS == key.QueryStringValue(nullptr, buff, &len)
				&& FileVersion::GetVer(buff).value < FileVersion::Ver(1,0,3,0).value) {
			m_transform.emplace_back(DNew CFGFilterRegistry(GUIDFromCString(clsid), MERIT64_DO_NOT_USE));
		}
	}

	// mainconcept color space converter
	m_transform.emplace_back(DNew CFGFilterRegistry(GUIDFromCString(L"{272D77A0-A852-4851-ADA4-9091FEAD4C86}"), MERIT64_DO_NOT_USE));

	// Accusoft PICVideo M-JPEG Codec 2.1 since causes a DEP crash
	m_transform.emplace_back(DNew CFGFilterRegistry(GUIDFromCString(L"{4C4CD9E1-F876-11D2-962F-00500471FDDC}"), MERIT64_DO_NOT_USE));

	// Morgan Stream Switcher (mmswitch.ax)
	m_transform.emplace_back(DNew CFGFilterRegistry(GUIDFromCString(L"{D3CD7858-971A-4838-ACEC-40CA5D529DC8}"), MERIT64_DO_NOT_USE));

	// block default video renderer renderer
	m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_VideoRendererDefault, MERIT64_DO_NOT_USE));
	m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_VideoRenderer, MERIT64_DO_NOT_USE));

	// Subtitle renderers

	bool VRwithSR =
		rs.iVideoRenderer == VIDRNDT_MADVR  ||
		rs.iVideoRenderer == VIDRNDT_EVR_CP ||
		rs.iVideoRenderer == VIDRNDT_SYNC   ||
		rs.iVideoRenderer == VIDRNDT_MPCVR;

	switch (s.iSubtitleRenderer) {
		case SUBRNDT_NONE:
		case SUBRNDT_ISR:
			m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_VSFilter, MERIT64_DO_NOT_USE));
			m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_VSFilter_autoloading, MERIT64_DO_NOT_USE));
			m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_XySubFilter, MERIT64_DO_NOT_USE));
			m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_XySubFilter_AutoLoader, MERIT64_DO_NOT_USE));
			m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_AssFilterMod, MERIT64_DO_NOT_USE));
			m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_AssFilterModAutoLoad, MERIT64_DO_NOT_USE));
			break;
		case SUBRNDT_VSFILTER:
			m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_VSFilter_autoloading, MERIT64_ABOVE_DSHOW));
			m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_XySubFilter, MERIT64_DO_NOT_USE));
			m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_XySubFilter_AutoLoader, MERIT64_DO_NOT_USE));
			m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_AssFilterMod, MERIT64_DO_NOT_USE));
			m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_AssFilterModAutoLoad, MERIT64_DO_NOT_USE));
			break;
		case SUBRNDT_XYSUBFILTER:
			m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_VSFilter, MERIT64_DO_NOT_USE));
			m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_VSFilter_autoloading, MERIT64_DO_NOT_USE));
			m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_AssFilterMod, MERIT64_DO_NOT_USE));
			m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_AssFilterModAutoLoad, MERIT64_DO_NOT_USE));
			if (VRwithSR) {
				m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_XySubFilter_AutoLoader, MERIT64_ABOVE_DSHOW));
			} else {
				// Prevent XySubFilter from connecting while renderer is not compatible
				m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_XySubFilter, MERIT64_DO_NOT_USE));
				m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_XySubFilter_AutoLoader, MERIT64_DO_NOT_USE));
			}
#if ENABLE_ASSFILTERMOD
		case SUBRNDT_ASSFILTERMOD:
			m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_VSFilter, MERIT64_DO_NOT_USE));
			m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_VSFilter_autoloading, MERIT64_DO_NOT_USE));
			m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_XySubFilter, MERIT64_DO_NOT_USE));
			m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_XySubFilter_AutoLoader, MERIT64_DO_NOT_USE));
			if (VRwithSR) {
				m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_AssFilterMod, MERIT64_ABOVE_DSHOW));
			} else {
				m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_AssFilterMod, MERIT64_DO_NOT_USE));
				m_transform.emplace_back(DNew CFGFilterRegistry(CLSID_AssFilterModAutoLoad, MERIT64_DO_NOT_USE));
			}
			break;
#endif
	}

	// Overrides
	WORD merit_low = 1;

	for (const auto& fo : s.m_ExternalFilters) {
		if (fo->fDisabled || (fo->type == FilterOverride::EXTERNAL && !::PathFileExistsW(MakeFullPath(fo->path)))) {
			continue;
		}

		ULONGLONG merit =
			IsPreview ? MERIT64_DO_USE :
			fo->iLoadType == FilterOverride::PREFERRED ? MERIT64_EXT_FILTERS_PREFER :
			fo->iLoadType == FilterOverride::MERIT ? MERIT64(fo->dwMerit) :
			MERIT64_DO_NOT_USE; // fo->iLoadType == FilterOverride::BLOCKED

		if (merit != MERIT64_DO_NOT_USE) {
			merit += merit_low++;
		}

		CFGFilter* pFGF = nullptr;

		if (fo->type == FilterOverride::REGISTERED) {
			pFGF = DNew CFGFilterRegistry(fo->dispname, merit);
		} else if (fo->type == FilterOverride::EXTERNAL) {
			pFGF = DNew CFGFilterFile(fo->clsid, fo->path, CStringW(fo->name), merit);
		}

		if (pFGF) {
			pFGF->SetTypes(fo->guids);
			m_override.emplace_back(pFGF);
		}
	}
}

STDMETHODIMP CFGManagerCustom::AddFilter(IBaseFilter* pBF, LPCWSTR pName)
{
	CAutoLock cAutoLock(this);

	HRESULT hr;

	if (FAILED(hr = __super::AddFilter(pBF, pName))) {
		return hr;
	}

	CAppSettings& s = AfxGetAppSettings();

	if (GetCLSID(pBF) == CLSID_DMOWrapperFilter) {
		if (CComQIPtr<IPropertyBag> pPB = pBF) {
			CComVariant var(true);
			pPB->Write(CComBSTR(L"_HIRESOUTPUT"), &var);
		}
	}

	if (CComQIPtr<IExFilterConfig> pEFC = pBF) {
		pEFC->Flt_SetBool("stereodownmix", s.bAudioMixer && s.nAudioMixerLayout == SPK_STEREO && s.bAudioStereoFromDecoder);
	}

	if (CComQIPtr<IAudioSwitcherFilter> pASF = pBF) {
		pASF->SetChannelMixer(s.bAudioMixer, s.nAudioMixerLayout);
		pASF->SetBassRedirect(s.bAudioBassRedirect);
		pASF->SetLevels(s.dAudioCenter_dB, s.dAudioSurround_dB);
		pASF->SetAudioGain(s.dAudioGain_dB);
		pASF->SetAutoVolumeControl(s.bAudioAutoVolumeControl, s.bAudioNormBoost, s.iAudioNormLevel, s.iAudioNormRealeaseTime);
		pASF->SetOutputFormats(s.iAudioSampleFormats);
		pASF->SetAudioTimeShift(s.bAudioTimeShift ? 10000i64 * s.iAudioTimeShift : 0);
		if (s.bAudioFilters) {
			pASF->SetAudioFilter1(s.strAudioFilter1);
		}
		pASF->SetAudioFiltersNotForStereo(s.bAudioFiltersNotForStereo);
	}

	return hr;
}

//
// CFGManagerPlayer
//

CFGManagerPlayer::CFGManagerPlayer(LPCWSTR pName, LPUNKNOWN pUnk, HWND hWnd, int preview)
	: CFGManagerCustom(pName, pUnk, hWnd, (preview > 0))
{
	DLog(L"CFGManagerPlayer::CFGManagerPlayer() on thread: %u", GetCurrentThreadId());
	CFGFilter* pFGF;

	CAppSettings& s = AfxGetAppSettings();
	const CRenderersSettings& rs = s.m_VRSettings;

	if (!m_bIsPreview) {
		pFGF = DNew CFGFilterInternal<CAudioSwitcherFilter>(L"Audio Switcher", MERIT64_HIGHEST);
		pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_NULL);
		m_transform.emplace_back(pFGF);
	}

	// Renderers
	if (!m_bIsPreview) {
		const UINT64 vrmerit = MERIT64_RENDERER;

		 auto CheckAddRenderer = [&](const CLSID& clsidVR, const CLSID& clsidAP, const WCHAR* name) {
			if (S_OK != CheckFilterCLSID(clsidVR)) {
				CString msg_str;
				msg_str.Format(ResStr(IDS_REND_UNAVAILABLEMSG), name, L"Enhanced Video Renderer");
				AfxMessageBox(msg_str, MB_ICONEXCLAMATION | MB_OK);
				m_transform.emplace_back(DNew CFGFilterVideoRenderer(m_hWnd, CLSID_EnhancedVideoRenderer, L"Enhanced Video Renderer", vrmerit));
			} else {
				m_transform.emplace_back(DNew CFGFilterVideoRenderer(m_hWnd, clsidAP, name, vrmerit));
			}
		};

		switch (rs.iVideoRenderer) {
			case VIDRNDT_EVR:
				m_transform.emplace_back(DNew CFGFilterVideoRenderer(m_hWnd, CLSID_EnhancedVideoRenderer, L"Enhanced Video Renderer", vrmerit));
				break;
			case VIDRNDT_EVR_CP:
				m_transform.emplace_back(DNew CFGFilterVideoRenderer(m_hWnd, CLSID_EVRAllocatorPresenter, L"Enhanced Video Renderer (custom presenter)", vrmerit));
				break;
			case VIDRNDT_SYNC:
				m_transform.emplace_back(DNew CFGFilterVideoRenderer(m_hWnd, CLSID_SyncAllocatorPresenter, L"EVR Sync", vrmerit));
				break;
			case VIDRNDT_MPCVR:
				CheckAddRenderer(CLSID_MPCVR, CLSID_MPCVRAllocatorPresenter, L"MPC Video Renderer");
				break;
			case VIDRNDT_DXR:
				CheckAddRenderer(CLSID_DXR, CLSID_DXRAllocatorPresenter, L"Haali's Video Renderer");
				break;
			case VIDRNDT_MADVR:
				CheckAddRenderer(CLSID_madVR, CLSID_madVRAllocatorPresenter, L"madVR Renderer");
				break;
			case VIDRNDT_NULL_ANY:
				pFGF = DNew CFGFilterInternal<CNullVideoRenderer>(L"Null Video Renderer (Any)", MERIT64_ABOVE_DSHOW + 2);
				pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_NULL);
				m_transform.emplace_back(pFGF);
				break;
			case VIDRNDT_NULL_UNCOMP:
				pFGF = DNew CFGFilterInternal<CNullUVideoRenderer>(L"Null Video Renderer (Uncompressed)", MERIT64_ABOVE_DSHOW + 2);
				pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_NULL);
				m_transform.emplace_back(pFGF);
				break;
		}
	} else {
		if (preview == 1) {
			m_transform.emplace_back(DNew CFGFilterVideoRenderer(m_hWnd, CLSID_EnhancedVideoRenderer, L"EVR - Preview Window", MERIT64_ABOVE_DSHOW + 2, true));
		}
		else if (preview == 2) {
			m_transform.emplace_back(DNew CFGFilterVideoRenderer(m_hWnd, CLSID_EVRAllocatorPresenter, L"EVR-CP - Preview Window", MERIT64_ABOVE_DSHOW + 2, true));
		}
	}

	if (!m_bIsPreview) {
		UINT64 armerit = MERIT64_RENDERER;

		BeginEnumSysDev(CLSID_AudioRendererCategory, pMoniker) {
			CFGFilterRegistry f(pMoniker);
			if (f.GetMerit() > armerit) {
				armerit = f.GetMerit() + 0x100;
			}
		}
		EndEnumSysDev

		CString SelAudioRenderer = s.SelectedAudioRenderer();
		armerit += 0x100; // for the first audio output give higher priority

		for (int ar = 0; ar < (s.fDualAudioOutput ? 2 : 1); ar++) {
			if (SelAudioRenderer == AUDRNDT_NULL_COMP) {
				pFGF = DNew CFGFilterInternal<CNullAudioRenderer>(AUDRNDT_NULL_COMP, armerit);
				pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_NULL);
				m_transform.emplace_back(pFGF);
			} else if (SelAudioRenderer == AUDRNDT_NULL_UNCOMP) {
				pFGF = DNew CFGFilterInternal<CNullUAudioRenderer>(AUDRNDT_NULL_UNCOMP, armerit);
				pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_NULL);
				m_transform.emplace_back(pFGF);
			} else if (SelAudioRenderer == AUDRNDT_MPC) {
				pFGF = DNew CFGFilterInternal<CMpcAudioRenderer>(AUDRNDT_MPC, armerit);
				pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_PCM);
				pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_IEEE_FLOAT);
				m_transform.emplace_back(pFGF);
			} else if (SelAudioRenderer.GetLength() > 0) {
				pFGF = DNew CFGFilterRegistry(SelAudioRenderer, armerit);
				pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_NULL);
				m_transform.emplace_back(pFGF);
			}

			// second audio output
			SelAudioRenderer = s.strAudioRendererDisplayName2;
			armerit -= 0x100;
		}
	}
}

STDMETHODIMP CFGManagerPlayer::ConnectDirect(IPin* pPinOut, IPin* pPinIn, const AM_MEDIA_TYPE* pmt)
{
	CAutoLock cAutoLock(this);

	if (GetCLSID(pPinOut) == CLSID_MPEG2Demultiplexer) {
		CComQIPtr<IMediaSeeking> pMS = pPinOut;
		REFERENCE_TIME rtDur = 0;
		if (!pMS || FAILED(pMS->GetDuration(&rtDur)) || rtDur <= 0) {
			return E_FAIL;
		}
	}

	return __super::ConnectDirect(pPinOut, pPinIn, pmt);
}

//
// CFGManagerDVD
//

CFGManagerDVD::CFGManagerDVD(LPCWSTR pName, LPUNKNOWN pUnk, HWND hWnd, bool IsPreview)
	: CFGManagerPlayer(pName, pUnk, hWnd, IsPreview)
{
	// elecard's decoder isn't suited for dvd playback (atm)
	m_transform.emplace_back(DNew CFGFilterRegistry(GUIDFromCString(L"{F50B3F13-19C4-11CF-AA9A-02608C9BABA2}"), MERIT64_DO_NOT_USE));
}

class CResetDVD : public CDVDSession
{
public:
	CResetDVD(LPCWSTR path) {
		if (Open(path)) {
			if (BeginSession()) {
				Authenticate(); /*GetDiscKey();*/
				EndSession();
			}
			Close();
		}
	}
};

STDMETHODIMP CFGManagerDVD::RenderFile(LPCWSTR lpcwstrFile, LPCWSTR lpcwstrPlayList)
{
	std::unique_lock<std::mutex> lock(m_mutexRender);

	CAutoLock cAutoLock(this);

	HRESULT hr;

	CComPtr<IBaseFilter> pBF;
	if (FAILED(hr = AddSourceFilter(lpcwstrFile, lpcwstrFile, &pBF))) {
		m_bOpeningAborted = false;
		return hr;
	}

	hr = ConnectFilter(pBF, nullptr);
	m_bOpeningAborted = false;

	return hr;
}

STDMETHODIMP CFGManagerDVD::AddSourceFilter(LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrFilterName, IBaseFilter** ppFilter)
{
	CAutoLock cAutoLock(this);

	CheckPointer(lpcwstrFileName, E_POINTER);
	CheckPointer(ppFilter, E_POINTER);

	HRESULT hr;
	CStringW fn = CStringW(lpcwstrFileName).TrimLeft();
	GUID clsid = CLSID_DVDNavigator;

	CComPtr<IBaseFilter> pBF;
	if (FAILED(hr = pBF.CoCreateInstance(clsid))
			|| FAILED(hr = AddFilter(pBF, L"DVD Navigator"))) {
		return VFW_E_CANNOT_LOAD_SOURCE_FILTER;
	}

	CComQIPtr<IDvdControl2> pDVDC;
	CComQIPtr<IDvdInfo2> pDVDI;

	if (!((pDVDC = pBF) && (pDVDI = pBF))) {
		return E_NOINTERFACE;
	}

	WCHAR buff[MAX_PATH];
	ULONG len;
	if (fn.IsEmpty()
			|| FAILED(hr = pDVDC->SetDVDDirectory(fn))
			|| FAILED(hr = pDVDI->GetDVDDirectory(buff, std::size(buff), &len))
			|| len == 0) {
		return E_INVALIDARG;
	}

	pDVDC->SetOption(DVD_EnablePortableBookmarks, TRUE); // DVD bookmarks can be used on another computer

	pDVDC->SetOption(DVD_ResetOnStop, FALSE);
	pDVDC->SetOption(DVD_HMSF_TimeCodeEvents, TRUE);

	if (clsid == CLSID_DVDNavigator) {
		CResetDVD(CString(buff));
	}

	*ppFilter = pBF.Detach();

	return S_OK;
}

//
// CFGManagerCapture
//

CFGManagerCapture::CFGManagerCapture(LPCWSTR pName, LPUNKNOWN pUnk, HWND hWnd)
	: CFGManagerPlayer(pName, pUnk, hWnd)
{
	CFGFilter* pFGF = DNew CFGFilterInternal<CDeinterlacerFilter>(L"Deinterlacer", MERIT64_DO_USE);
	pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_NULL);
	m_transform.emplace_back(pFGF);
}

//
// CFGManagerMuxer
//

CFGManagerMuxer::CFGManagerMuxer(LPCWSTR pName, LPUNKNOWN pUnk)
	: CFGManagerCustom(pName, pUnk)
{
	m_source.emplace_back(DNew CFGFilterInternal<CSubtitleSourceASS>());
}

//
// CFGAggregator
//

CFGAggregator::CFGAggregator(const CLSID& clsid, LPCWSTR pName, LPUNKNOWN pUnk, HRESULT& hr)
	: CUnknown(pName, pUnk)
{
	hr = m_pUnkInner.CoCreateInstance(clsid, GetOwner());
}

CFGAggregator::~CFGAggregator()
{
	m_pUnkInner.Release();
}

STDMETHODIMP CFGAggregator::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		m_pUnkInner && (riid != IID_IUnknown && SUCCEEDED(m_pUnkInner->QueryInterface(riid, ppv))) ? S_OK :
		__super::NonDelegatingQueryInterface(riid, ppv);
}

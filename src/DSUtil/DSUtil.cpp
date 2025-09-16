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
#include <Vfw.h>
#include "DSUtil.h"
#include "Mpeg2Def.h"
#include "AudioParser.h"
#include "NullRenderers.h"
#include "std_helper.h"
#include "FileHandle.h"
#include <clsids.h>
#include <wmcodecdsp.h>
#include <moreuuids.h>
#include <dxva2_guids.h>
#include <basestruct.h>
#include <emmintrin.h>
#include <d3d9.h>
#include <dxva2api.h>
#include <mvrInterfaces.h>

int CountPins(IBaseFilter* pBF, int& nIn, int& nOut, int& nInC, int& nOutC)
{
	nIn = nOut = 0;
	nInC = nOutC = 0;

	BeginEnumPins(pBF, pEP, pPin) {
		PIN_DIRECTION dir;
		if (SUCCEEDED(pPin->QueryDirection(&dir))) {
			CComPtr<IPin> pPinConnectedTo;
			pPin->ConnectedTo(&pPinConnectedTo);

			if (dir == PINDIR_INPUT) {
				nIn++;
				if (pPinConnectedTo) {
					nInC++;
				}
			} else if (dir == PINDIR_OUTPUT) {
				nOut++;
				if (pPinConnectedTo) {
					nOutC++;
				}
			}
		}
	}
	EndEnumPins

	return (nIn + nOut);
}

bool IsSplitter(IBaseFilter* pBF, bool fCountConnectedOnly)
{
	int nIn, nOut, nInC, nOutC;
	CountPins(pBF, nIn, nOut, nInC, nOutC);
	return (fCountConnectedOnly ? nOutC > 1 : nOut > 1);
}

bool IsMultiplexer(IBaseFilter* pBF, bool fCountConnectedOnly)
{
	int nIn, nOut, nInC, nOutC;
	CountPins(pBF, nIn, nOut, nInC, nOutC);
	return (fCountConnectedOnly ? nInC > 1 : nIn > 1);
}

bool IsStreamStart(IBaseFilter* pBF)
{
	CComQIPtr<IAMFilterMiscFlags> pAMMF(pBF);
	if (pAMMF && pAMMF->GetMiscFlags()&AM_FILTER_MISC_FLAGS_IS_SOURCE) {
		return true;
	}

	int nIn, nOut, nInC, nOutC;
	CountPins(pBF, nIn, nOut, nInC, nOutC);
	CMediaType mt;
	CComPtr<IPin> pIn = GetFirstPin(pBF);
	return ((nOut > 1)
		   || (nOut > 0 && nIn == 1 && pIn && SUCCEEDED(pIn->ConnectionMediaType(&mt)) && mt.majortype == MEDIATYPE_Stream));
}

bool IsStreamEnd(IBaseFilter* pBF)
{
	int nIn, nOut, nInC, nOutC;
	CountPins(pBF, nIn, nOut, nInC, nOutC);
	return (nOut == 0);
}

bool IsVideoDecoder(IBaseFilter* pBF, bool fCountConnectedOnly)
{
	const CLSID clsid = GetCLSID(pBF);
	if (clsid == CLSID_SmartTee) {
		return false;
	}

	const CString filterName = GetFilterName(pBF).MakeLower();

	if (StartsWith(filterName, L"directvobsub")) {
		return true;
	}
	if (filterName.Find(L"video") < 0) {
		// All popular video decoders includes in the name the "video"
		return false;
	}

	int nIn, nOut, nInC, nOutC;
	nIn = nInC = 0;
	nOut = nOutC = 0;

	BeginEnumPins(pBF, pEP, pPin) {

		PIN_DIRECTION dir;
		if (SUCCEEDED(pPin->QueryDirection(&dir))) {
			CComPtr<IPin> pPinConnectedTo;
			pPin->ConnectedTo(&pPinConnectedTo);

			if (dir == PINDIR_INPUT) {
				AM_MEDIA_TYPE mt;
				if (S_OK != pPin->ConnectionMediaType(&mt)) {
					continue;
				}
				FreeMediaType(mt);

				if (mt.majortype == MEDIATYPE_Video || mt.majortype == MEDIATYPE_DVD_ENCRYPTED_PACK) {
					nIn++;
					if (pPinConnectedTo) {
						nInC++;
					}
				}
			} else if (dir == PINDIR_OUTPUT) {
				AM_MEDIA_TYPE mt;
				if (S_OK != pPin->ConnectionMediaType(&mt)) {
					continue;
				}
				FreeMediaType(mt);

				if (mt.majortype == MEDIATYPE_Video) {
					nOut++;
					if (pPinConnectedTo) {
						nOutC++;
					}
				}
			}
		}
	}
	EndEnumPins

	return fCountConnectedOnly ? (nInC && nOutC) : (nIn && nOut);
}

bool IsVideoRenderer(IBaseFilter* pBF)
{
	int nIn, nOut, nInC, nOutC;
	CountPins(pBF, nIn, nOut, nInC, nOutC);

	if (nInC > 0 && nOut == 0) {
		BeginEnumPins(pBF, pEP, pPin) {
			AM_MEDIA_TYPE mt;
			if (S_OK != pPin->ConnectionMediaType(&mt)) {
				continue;
			}

			FreeMediaType(mt);

			return !!(mt.majortype == MEDIATYPE_Video);
		}
		EndEnumPins
	}

	return IsVideoRenderer(GetCLSID(pBF));
}

bool IsVideoRenderer(const CLSID clsid)
{
	if (clsid == CLSID_VideoRendererDefault
		|| clsid == CLSID_VideoRenderer
		|| clsid == CLSID_EnhancedVideoRenderer   // EVR
		|| clsid == CLSID_EVRAllocatorPresenter   // EVR-CP
		|| clsid == CLSID_SyncAllocatorPresenter  // Sync VR
		|| clsid == CLSID_VideoMixingRenderer     // VMR-7
		|| clsid == CLSID_VideoMixingRenderer9    // VMR-9
		|| clsid == CLSID_DXR                     // Haali VR
		|| clsid == CLSID_DXRAllocatorPresenter   // AP for Haali VR
		|| clsid == CLSID_madVR                   // madVR
		|| clsid == CLSID_madVRAllocatorPresenter // AP for madVR
		|| clsid == CLSID_MPCVR                   // MPC VR
		|| clsid == CLSID_MPCVRAllocatorPresenter // AP for MPC VR
		|| clsid == CLSID_OverlayMixer) {
		return true;
	}

	return false;
}

bool IsAudioWaveRenderer(IBaseFilter* pBF)
{
	int nIn, nOut, nInC, nOutC;
	CountPins(pBF, nIn, nOut, nInC, nOutC);

	if (nInC > 0 && nOut == 0 && CComQIPtr<IBasicAudio>(pBF)) {
		BeginEnumPins(pBF, pEP, pPin) {
			AM_MEDIA_TYPE mt;
			if (S_OK != pPin->ConnectionMediaType(&mt)) {
				continue;
			}
			FreeMediaType(mt);

			return !!(mt.majortype == MEDIATYPE_Audio);
		}
		EndEnumPins
	}

	CLSID clsid = GUID_NULL;
	pBF->GetClassID(&clsid);

	return (// system
			clsid == CLSID_DSoundRender ||
			clsid == CLSID_AudioRender ||
			// internal
			clsid == CLSID_MpcAudioRenderer ||
			clsid == __uuidof(CNullAudioRenderer) ||
			clsid == __uuidof(CNullUAudioRenderer) ||
			// external
			clsid == CLSID_ReClock ||
			clsid == CLSID_SanearAudioRenderer ||
			clsid == GUIDFromCString(L"{EC9ED6FC-7B03-4cb6-8C01-4EABE109F26B}") || // MediaPortal Audio Renderer
			clsid == GUIDFromCString(L"{50063380-2B2F-4855-9A1E-40FCA344C7AC}") || // Surodev ASIO Renderer
			clsid == GUIDFromCString(L"{8DE31E85-10FC-4088-8861-E0EC8E70744A}") || // MultiChannel ASIO Renderer
			clsid == GUIDFromCString(L"{205F9417-8EEF-40B4-91CF-C7C6A96936EF}")    // MBSE MultiChannel ASIO Renderer
	);
}

IBaseFilter* GetUpStreamFilter(IBaseFilter* pBF, IPin* pInputPin)
{
	return GetFilterFromPin(GetUpStreamPin(pBF, pInputPin));
}

IPin* GetUpStreamPin(IBaseFilter* pBF, IPin* pInputPin)
{
	BeginEnumPins(pBF, pEP, pPin) {
		if (pInputPin && pInputPin != pPin) {
			continue;
		}

		PIN_DIRECTION dir;
		CComPtr<IPin> pPinConnectedTo;
		if (SUCCEEDED(pPin->QueryDirection(&dir)) && dir == PINDIR_INPUT
				&& SUCCEEDED(pPin->ConnectedTo(&pPinConnectedTo))) {
			IPin* pRet = pPinConnectedTo.Detach();
			pRet->Release();
			return pRet;
		}
	}
	EndEnumPins

	return nullptr;
}

IBaseFilter* GetDownStreamFilter(IBaseFilter* pBF, IPin* pInputPin)
{
	return GetFilterFromPin(GetDownStreamPin(pBF, pInputPin));
}

IPin* GetDownStreamPin(IBaseFilter* pBF, IPin* pInputPin)
{
	BeginEnumPins(pBF, pEP, pPin) {
		if (!pInputPin || (pInputPin == pPin)) {
			PIN_DIRECTION dir;
			CComPtr<IPin> pPinConnectedTo;
			if (SUCCEEDED(pPin->QueryDirection(&dir)) && dir == PINDIR_OUTPUT
					&& SUCCEEDED(pPin->ConnectedTo(&pPinConnectedTo))) {
				IPin* pRet = pPinConnectedTo.Detach();
				pRet->Release();
				return pRet;
			}
		}
	}
	EndEnumPins

	return nullptr;
}

IPin* GetFirstPin(IBaseFilter* pBF, PIN_DIRECTION dir)
{
	if (!pBF) {
		return nullptr;
	}

	BeginEnumPins(pBF, pEP, pPin) {
		PIN_DIRECTION dir2;
		pPin->QueryDirection(&dir2);
		if (dir == dir2) {
			IPin* pRet = pPin.Detach();
			pRet->Release();
			return pRet;
		}
	}
	EndEnumPins

	return nullptr;
}

IPin* GetFirstDisconnectedPin(IBaseFilter* pBF, PIN_DIRECTION dir)
{
	if (!pBF) {
		return nullptr;
	}

	BeginEnumPins(pBF, pEP, pPin) {
		PIN_DIRECTION dir2;
		pPin->QueryDirection(&dir2);
		CComPtr<IPin> pPinTo;
		if (dir == dir2 && (S_OK != pPin->ConnectedTo(&pPinTo))) {
			IPin* pRet = pPin.Detach();
			pRet->Release();
			return pRet;
		}
	}
	EndEnumPins

	return nullptr;
}

IBaseFilter* FindFilter(LPCWSTR clsid, IFilterGraph* pFG)
{
	CLSID clsid2;
	CLSIDFromString(CComBSTR(clsid), &clsid2);
	return FindFilter(clsid2, pFG);
}

IBaseFilter* FindFilter(const CLSID& clsid, IFilterGraph* pFG)
{
	BeginEnumFilters(pFG, pEF, pBF) {
		CLSID clsid2;
		if (SUCCEEDED(pBF->GetClassID(&clsid2)) && clsid == clsid2) {
			return pBF;
		}
	}
	EndEnumFilters

	return nullptr;
}

IPin* FindPin(IBaseFilter* pBF, PIN_DIRECTION direction, const AM_MEDIA_TYPE* pRequestedMT)
{
	if (!pBF) {
		return nullptr;
	}

	PIN_DIRECTION	pindir;
	BeginEnumPins(pBF, pEP, pPin) {
		CComPtr<IPin>		pFellow;

		if (SUCCEEDED (pPin->QueryDirection(&pindir)) &&
				pindir == direction &&
				pPin->ConnectedTo(&pFellow) == VFW_E_NOT_CONNECTED) {
			BeginEnumMediaTypes(pPin, pEM, pmt) {
				if (pmt->majortype == pRequestedMT->majortype && pmt->subtype == pRequestedMT->subtype) {
					return (pPin);
				}
			}
			EndEnumMediaTypes(pmt)
		}
	}
	EndEnumPins

	return nullptr;
}

IPin* FindPin(IBaseFilter* pBF, PIN_DIRECTION direction, const GUID majortype)
{
	if (!pBF) {
		return nullptr;
	}

	PIN_DIRECTION	pindir;
	BeginEnumPins(pBF, pEP, pPin) {

		if (SUCCEEDED (pPin->QueryDirection(&pindir)) && pindir == direction) {
			BeginEnumMediaTypes(pPin, pEM, pmt) {
				if (pmt->majortype == majortype) {
					return (pPin);
				}
			}
			EndEnumMediaTypes(pmt)
		}
	}
	EndEnumPins

	return nullptr;
}

CString GetFilterName(IBaseFilter* pBF)
{
	CString name;

	if (pBF) {
		CFilterInfo fi;
		if (SUCCEEDED(pBF->QueryFilterInfo(&fi))) {
			name = fi.achName;
		}
	}

	return name;
}

CString GetPinName(IPin* pPin)
{
	CString name;
	CPinInfo pi;
	if (pPin && SUCCEEDED(pPin->QueryPinInfo(&pi))) {
		name = pi.achName;
	}

	return name;
}

IFilterGraph* GetGraphFromFilter(IBaseFilter* pBF)
{
	if (pBF) {
		CFilterInfo fi;
		if (SUCCEEDED(pBF->QueryFilterInfo(&fi))) {
			return fi.pGraph;
		}
	}
	return nullptr;
}

IBaseFilter* GetFilterFromPin(IPin* pPin)
{
	if (pPin) {
		CPinInfo pi;
		if (SUCCEEDED(pPin->QueryPinInfo(&pi))) {
			return pi.pFilter;
		}
	}
	return nullptr;
}

IPin* AppendFilter(IPin* pPin, CString DisplayName, IGraphBuilder* pGB)
{
	IPin* pRet = pPin;

	std::list<CComPtr<IBaseFilter>> pFilters;

	do {
		if (!pPin || DisplayName.IsEmpty() || !pGB) {
			break;
		}

		CComPtr<IPin> pPinTo;
		PIN_DIRECTION dir;
		if (FAILED(pPin->QueryDirection(&dir)) || dir != PINDIR_OUTPUT || SUCCEEDED(pPin->ConnectedTo(&pPinTo))) {
			break;
		}

		CComPtr<IBindCtx> pBindCtx;
		CreateBindCtx(0, &pBindCtx);

		CComPtr<IMoniker> pMoniker;
		ULONG chEaten;
		if (S_OK != MkParseDisplayName(pBindCtx, CComBSTR(DisplayName), &chEaten, &pMoniker)) {
			break;
		}

		CComPtr<IBaseFilter> pBF;
		if (FAILED(pMoniker->BindToObject(pBindCtx, 0, IID_IBaseFilter, (void**)&pBF)) || !pBF) {
			break;
		}

		CComPtr<IPropertyBag> pPB;
		if (FAILED(pMoniker->BindToStorage(pBindCtx, 0, IID_IPropertyBag, (void**)&pPB))) {
			break;
		}

		CComVariant var;
		if (FAILED(pPB->Read(CComBSTR(L"FriendlyName"), &var, nullptr))) {
			break;
		}

		pFilters.emplace_back(pBF);
		BeginEnumFilters(pGB, pEnum, pBF2)
		pFilters.emplace_back(pBF2);
		EndEnumFilters

		if (FAILED(pGB->AddFilter(pBF, CStringW(var.bstrVal)))) {
			break;
		}

		BeginEnumFilters(pGB, pEnum, pBF2)
		if (!Contains(pFilters, pBF2) && SUCCEEDED(pGB->RemoveFilter(pBF2))) {
			pEnum->Reset();
		}
		EndEnumFilters

		pPinTo = GetFirstPin(pBF, PINDIR_INPUT);
		if (!pPinTo) {
			pGB->RemoveFilter(pBF);
			break;
		}

		HRESULT hr;
		if (FAILED(hr = pGB->ConnectDirect(pPin, pPinTo, nullptr))) {
			hr = pGB->Connect(pPin, pPinTo);
			pGB->RemoveFilter(pBF);
			break;
		}

		BeginEnumFilters(pGB, pEnum, pBF2)
		if (!Contains(pFilters, pBF2) && SUCCEEDED(pGB->RemoveFilter(pBF2))) {
			pEnum->Reset();
		}
		EndEnumFilters

		pRet = GetFirstPin(pBF, PINDIR_OUTPUT);
		if (!pRet) {
			pRet = pPin;
			pGB->RemoveFilter(pBF);
			break;
		}
	} while (false);

	return pRet;
}

IPin* InsertFilter(IPin* pPin, CString DisplayName, IGraphBuilder* pGB)
{
	do {
		if (!pPin || DisplayName.IsEmpty() || !pGB) {
			break;
		}

		PIN_DIRECTION dir;
		if (FAILED(pPin->QueryDirection(&dir))) {
			break;
		}

		CComPtr<IPin> pFrom, pTo;

		if (dir == PINDIR_INPUT) {
			pPin->ConnectedTo(&pFrom);
			pTo = pPin;
		} else if (dir == PINDIR_OUTPUT) {
			pFrom = pPin;
			pPin->ConnectedTo(&pTo);
		}

		if (!pFrom || !pTo) {
			break;
		}

		CComPtr<IBindCtx> pBindCtx;
		CreateBindCtx(0, &pBindCtx);

		CComPtr<IMoniker> pMoniker;
		ULONG chEaten;
		if (S_OK != MkParseDisplayName(pBindCtx, CComBSTR(DisplayName), &chEaten, &pMoniker)) {
			break;
		}

		CComPtr<IBaseFilter> pBF;
		if (FAILED(pMoniker->BindToObject(pBindCtx, 0, IID_IBaseFilter, (void**)&pBF)) || !pBF) {
			break;
		}

		CComPtr<IPropertyBag> pPB;
		if (FAILED(pMoniker->BindToStorage(pBindCtx, 0, IID_IPropertyBag, (void**)&pPB))) {
			break;
		}

		CComVariant var;
		if (FAILED(pPB->Read(CComBSTR(L"FriendlyName"), &var, nullptr))) {
			break;
		}

		if (FAILED(pGB->AddFilter(pBF, CStringW(var.bstrVal)))) {
			break;
		}

		CComPtr<IPin> pFromTo = GetFirstPin(pBF, PINDIR_INPUT);
		if (!pFromTo) {
			pGB->RemoveFilter(pBF);
			break;
		}

		if (FAILED(pGB->Disconnect(pFrom)) || FAILED(pGB->Disconnect(pTo))) {
			pGB->RemoveFilter(pBF);
			pGB->ConnectDirect(pFrom, pTo, nullptr);
			break;
		}

		HRESULT hr;
		if (FAILED(hr = pGB->ConnectDirect(pFrom, pFromTo, nullptr))) {
			pGB->RemoveFilter(pBF);
			pGB->ConnectDirect(pFrom, pTo, nullptr);
			break;
		}

		CComPtr<IPin> pToFrom = GetFirstPin(pBF, PINDIR_OUTPUT);
		if (!pToFrom) {
			pGB->RemoveFilter(pBF);
			pGB->ConnectDirect(pFrom, pTo, nullptr);
			break;
		}

		if (FAILED(pGB->ConnectDirect(pToFrom, pTo, nullptr))) {
			pGB->RemoveFilter(pBF);
			pGB->ConnectDirect(pFrom, pTo, nullptr);
			break;
		}

		pPin = pToFrom;
	} while (false);

	return pPin;
}

void ExtractMediaTypes(IPin* pPin, std::vector<GUID>& types)
{
	types.clear();

	BeginEnumMediaTypes(pPin, pEM, pmt) {
		bool fFound = false;

		for (size_t i = 0; i < types.size(); i += 2) {
			if (types[i] == pmt->majortype && types[i+1] == pmt->subtype) {
				fFound = true;
				break;
			}
		}

		if (!fFound) {
			types.push_back(pmt->majortype);
			types.push_back(pmt->subtype);
		}
	}
	EndEnumMediaTypes(pmt)
}

void ExtractMediaTypes(IPin* pPin, std::list<PinType>& types)
{
	types.clear();

	BeginEnumMediaTypes(pPin, pEM, pmt) {
		bool fFound = false;

		for (const auto& type : types) {
			if (type.major == pmt->majortype && type.sub == pmt->subtype) {
				fFound = true;
				break;
			}
		}

		if (!fFound) {
			types.push_back({ pmt->majortype, pmt->subtype });
		}
	}
	EndEnumMediaTypes(pmt)
}

void ExtractMediaTypes(IPin* pPin, std::list<CMediaType>& mts)
{
	mts.clear();

	BeginEnumMediaTypes(pPin, pEM, pmt) {
		bool fFound = false;

		for (const auto& mt : mts) {
			if (mt.majortype == pmt->majortype && mt.subtype == pmt->subtype) {
				fFound = true;
				break;
			}
		}

		if (!fFound) {
			mts.emplace_back(*pmt);
		}
	}
	EndEnumMediaTypes(pmt)
}

CLSID GetCLSID(IBaseFilter* pBF)
{
	CLSID clsid = GUID_NULL;
	if (pBF) {
		pBF->GetClassID(&clsid);
	}
	return clsid;
}

CLSID GetCLSID(IPin* pPin)
{
	return GetCLSID(GetFilterFromPin(pPin));
}

bool IsCLSIDRegistered(LPCWSTR clsid)
{
	CString rootkey1(L"CLSID\\");
	CString rootkey2(L"CLSID\\{083863F1-70DE-11d0-BD40-00A0C911CE86}\\Instance\\");

	return ERROR_SUCCESS == CRegKey().Open(HKEY_CLASSES_ROOT, rootkey1 + clsid, KEY_READ)
		   || ERROR_SUCCESS == CRegKey().Open(HKEY_CLASSES_ROOT, rootkey2 + clsid, KEY_READ);
}

bool IsCLSIDRegistered(const CLSID& clsid)
{
	bool fRet = false;

	LPOLESTR pStr = nullptr;
	if (S_OK == StringFromCLSID(clsid, &pStr) && pStr) {
		fRet = IsCLSIDRegistered(CString(pStr));
		CoTaskMemFree(pStr);
	}

	return fRet;
}

HRESULT CheckFilterCLSID(const CLSID& clsid)
{
	LPOLESTR pStr = nullptr;
	if (S_OK == StringFromCLSID(clsid, &pStr) && pStr) {
		CString str_clsid(pStr);
		CoTaskMemFree(pStr);

		CRegKey key;
		if (ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, L"CLSID\\{083863F1-70DE-11D0-BD40-00A0C911CE86}\\Instance\\" + str_clsid, KEY_READ)) {
			WCHAR buf[MAX_PATH] = {};
			ULONG len = MAX_PATH;
			if (ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, L"CLSID\\" + str_clsid + L"\\InprocServer32", KEY_READ)
					&& ERROR_SUCCESS == key.QueryStringValue(nullptr, buf, &len)
					&& ::PathFileExistsW(buf)) {

				return S_OK; // installed and available
			}

			return S_FALSE; // installed but not available
		}
	}

	return E_FAIL; // not installed
}

void CStringToBin(CString str, std::vector<BYTE>& data)
{
	str.Trim();
	ASSERT((str.GetLength()&1) == 0);
	data.reserve(str.GetLength()/2);

	BYTE b = 0;
	for (int i = 0, len = str.GetLength(); i < len; i++) {
		WCHAR ch = str[i];
		if (ch >= '0' && ch <= '9') {
			ch -= '0';
		}
		else if (ch >= 'A' && ch <= 'F') {
			ch -= ('A'-10);
		}
		else if (ch >= 'a' && ch <= 'f') {
			ch -= ('a'-10);
		}
		else {
			break;
		}

		if (i & 1) {
			b |= (BYTE)ch;
			data.push_back(b);
		} else {
			b = (BYTE)ch << 4;
		}
	}
}

CString BinToCString(const BYTE* ptr, size_t len)
{
	CString ret;
	WCHAR high, low;

	while (len-- > 0) {
		high = *ptr >> 4;
		high += (high >= 10) ? ('A'-10) : '0';
		low = *ptr & 0xf;
		low += (low >= 10) ? ('A'-10) : '0';

		ret.AppendChar(high);
		ret.AppendChar(low);

		ptr++;
	}

	return ret;
}

TimeCode_t ReftimeToTimecode(REFERENCE_TIME rt)
{
	TimeCode_t timecode;

	lldiv_t d = { rt / 10000, 0 };

	d = std::lldiv(d.quot, 1000);
	timecode.Milliseconds = (int16_t)d.rem;

	d = std::lldiv(d.quot, 60);
	timecode.Seconds = (int8_t)d.rem;

	d = std::lldiv(d.quot, 60);
	timecode.Hours = (int32_t)d.quot;
	timecode.Minutes = (int8_t)d.rem;

	return timecode;
}

TimeCode_t ReftimeToHMS(REFERENCE_TIME rt)
{
	TimeCode_t timecode;

	lldiv_t d = { (rt + 5000000) / 10000000, 0 };

	d = std::lldiv(d.quot, 60);
	timecode.Seconds = (int8_t)d.rem;
	timecode.Milliseconds = 0;

	d = std::lldiv(d.quot, 60);
	timecode.Hours = (int32_t)d.quot;
	timecode.Minutes = (int8_t)d.rem;

	return timecode;
}

REFERENCE_TIME TimecodeToReftime(TimeCode_t tc)
{
	return ((((REFERENCE_TIME)tc.Hours * 60 + tc.Minutes) * 60 + tc.Seconds) * 1000 + tc.Milliseconds) * 10000;
}

// hh:mm::ss.millisec
CStringW ReftimeToString(REFERENCE_TIME rt, bool showZeroHours /* = true*/)
{
	if (rt == INVALID_TIME) {
		return L"INVALID TIME";
	}

	TimeCode_t tc = ReftimeToTimecode(rt);
	CStringW str;
	if (tc.Hours || showZeroHours) {
		str.Format(L"%02d:%02d:%02d.%03d", tc.Hours, tc.Minutes, tc.Seconds, tc.Milliseconds);
	} else {
		str.Format(L"%02d:%02d.%03d", tc.Minutes, tc.Seconds, tc.Milliseconds);
	}

	return str;
}

// hour, minute, second (round)
CStringW ReftimeToString2(REFERENCE_TIME rt, bool showZeroHours /* = true*/)
{
	if (rt == INVALID_TIME) {
		return L"INVALID TIME";
	}

	TimeCode_t tc = ReftimeToHMS(rt);
	CStringW str;
	if (tc.Hours || showZeroHours) {
		str.Format(L"%02d:%02d:%02d", tc.Hours, tc.Minutes, tc.Seconds);
	} else {
		str.Format(L"%02d:%02d", tc.Minutes, tc.Seconds);
	}

	return str;
}

DVD_HMSF_TIMECODE RT2HMSF(REFERENCE_TIME rt, double fps) // use to remember the current position
{
	DVD_HMSF_TIMECODE hmsf = {
		(BYTE)((rt/10000000/60/60)),
		(BYTE)((rt/10000000/60)%60),
		(BYTE)((rt/10000000)%60),
		(BYTE)(1.0*((rt/10000)%1000) * fps / 1000)
	};

	return hmsf;
}

DVD_HMSF_TIMECODE RT2HMS_r(REFERENCE_TIME rt) // use only for information (for display on the screen)
{
	rt = (rt + 5000000) / 10000000;
	DVD_HMSF_TIMECODE hmsf = {
		(BYTE)(rt / 3600),
		(BYTE)(rt / 60 % 60),
		(BYTE)(rt % 60),
		0
	};

	return hmsf;
}

REFERENCE_TIME HMSF2RT(DVD_HMSF_TIMECODE hmsf, double fps)
{
	if (fps == 0) {
		hmsf.bFrames = 0;
		fps = 1;
	}
	return (REFERENCE_TIME)((((REFERENCE_TIME)hmsf.bHours*60+hmsf.bMinutes)*60+hmsf.bSeconds)*1000+1.0*hmsf.bFrames*1000/fps)*10000;
}

bool ExtractBIH(const AM_MEDIA_TYPE* pmt, BITMAPINFOHEADER* bih)
{
	if (pmt && bih) {
		memset(bih, 0, sizeof(*bih));

		if (pmt->formattype == FORMAT_VideoInfo2) {
			VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)pmt->pbFormat;
			memcpy(bih, &vih2->bmiHeader, sizeof(BITMAPINFOHEADER));
			return true;
		} else if (pmt->formattype == FORMAT_VideoInfo) {
			VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pmt->pbFormat;
			memcpy(bih, &vih->bmiHeader, sizeof(BITMAPINFOHEADER));
			return true;
		} else if (pmt->formattype == FORMAT_MPEGVideo) {
			VIDEOINFOHEADER* vih = &((MPEG1VIDEOINFO*)pmt->pbFormat)->hdr;
			memcpy(bih, &vih->bmiHeader, sizeof(BITMAPINFOHEADER));
			return true;
		} else if (pmt->formattype == FORMAT_MPEG2_VIDEO) {
			VIDEOINFOHEADER2* vih2 = &((MPEG2VIDEOINFO*)pmt->pbFormat)->hdr;
			memcpy(bih, &vih2->bmiHeader, sizeof(BITMAPINFOHEADER));
			return true;
		} else if (pmt->formattype == FORMAT_DiracVideoInfo) {
			VIDEOINFOHEADER2* vih2 = &((DIRACINFOHEADER*)pmt->pbFormat)->hdr;
			memcpy(bih, &vih2->bmiHeader, sizeof(BITMAPINFOHEADER));
			return true;
		}
	}

	return false;
}

bool ExtractAvgTimePerFrame(const AM_MEDIA_TYPE* pmt, REFERENCE_TIME& rtAvgTimePerFrame)
{
	if (pmt->formattype==FORMAT_VideoInfo) {
		rtAvgTimePerFrame = ((VIDEOINFOHEADER*)pmt->pbFormat)->AvgTimePerFrame;
	} else if (pmt->formattype==FORMAT_VideoInfo2) {
		rtAvgTimePerFrame = ((VIDEOINFOHEADER2*)pmt->pbFormat)->AvgTimePerFrame;
	} else if (pmt->formattype==FORMAT_MPEGVideo) {
		rtAvgTimePerFrame = ((MPEG1VIDEOINFO*)pmt->pbFormat)->hdr.AvgTimePerFrame;
	} else if (pmt->formattype==FORMAT_MPEG2Video) {
		rtAvgTimePerFrame = ((MPEG2VIDEOINFO*)pmt->pbFormat)->hdr.AvgTimePerFrame;
	} else {
		return false;
	}

	if (rtAvgTimePerFrame < 1) { // invalid value
		rtAvgTimePerFrame = 1;
	}

	return true;
}

bool ExtractBIH(IMediaSample* pMS, BITMAPINFOHEADER* bih)
{
	AM_MEDIA_TYPE* pmt = nullptr;
	pMS->GetMediaType(&pmt);
	if (pmt) {
		bool fRet = ExtractBIH(pmt, bih);
		DeleteMediaType(pmt);
		return fRet;
	}

	return false;
}

bool ExtractDim(const AM_MEDIA_TYPE* pmt, int& w, int& h, int& arx, int& ary)
{
	w = h = arx = ary = 0;

	if (pmt->formattype == FORMAT_VideoInfo || pmt->formattype == FORMAT_MPEGVideo) {
		VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pmt->pbFormat;
		w = vih->bmiHeader.biWidth;
		h = abs(vih->bmiHeader.biHeight);
		arx = w * vih->bmiHeader.biYPelsPerMeter;
		ary = h * vih->bmiHeader.biXPelsPerMeter;
	} else if (pmt->formattype == FORMAT_VideoInfo2 || pmt->formattype == FORMAT_MPEG2_VIDEO || pmt->formattype == FORMAT_DiracVideoInfo) {
		VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)pmt->pbFormat;
		w = vih->bmiHeader.biWidth;
		h = abs(vih->bmiHeader.biHeight);
		arx = vih->dwPictAspectRatioX;
		ary = vih->dwPictAspectRatioY;
	} else {
		return false;
	}

	if (!arx || !ary) {
		BYTE* ptr = nullptr;
		DWORD len = 0;

		if (pmt->formattype == FORMAT_MPEGVideo) {
			ptr = ((MPEG1VIDEOINFO*)pmt->pbFormat)->bSequenceHeader;
			len = ((MPEG1VIDEOINFO*)pmt->pbFormat)->cbSequenceHeader;

			if (ptr && len >= 8 && GETU32(ptr) == 0xb3010000) {
				w = (ptr[4]<<4)|(ptr[5]>>4);
				h = ((ptr[5]&0xf)<<8)|ptr[6];
				float ar[] = {
					1.0000f,1.0000f,0.6735f,0.7031f,
					0.7615f,0.8055f,0.8437f,0.8935f,
					0.9157f,0.9815f,1.0255f,1.0695f,
					1.0950f,1.1575f,1.2015f,1.0000f,
				};
				arx = (int)((float)w / ar[ptr[7]>>4] + 0.5);
				ary = h;
			}
		} else if (pmt->formattype == FORMAT_MPEG2_VIDEO) {
			ptr = (BYTE*)((MPEG2VIDEOINFO*)pmt->pbFormat)->dwSequenceHeader;
			len = ((MPEG2VIDEOINFO*)pmt->pbFormat)->cbSequenceHeader;

			if (ptr && len >= 8 && GETU32(ptr) == 0xb3010000) {
				w = (ptr[4]<<4)|(ptr[5]>>4);
				h = ((ptr[5]&0xf)<<8)|ptr[6];
				struct {
					int x, y;
				} ar[] = {{w,h},{4,3},{16,9},{221,100},{w,h}};
				int i = std::clamp(ptr[7]>>4, 1, 5) - 1;
				arx = ar[i].x;
				ary = ar[i].y;
			}
		}
	}

	if (!arx || !ary) {
		arx = w;
		ary = h;
	}

	ReduceDim(arx, ary);

	return true;
}

bool MakeMPEG2MediaType(CMediaType& mt, BYTE* seqhdr, DWORD len, int w, int h)
{
	mt = CMediaType();

	if (len < 4 || GETU32(seqhdr) != 0xb3010000) {
		mt.majortype					= MEDIATYPE_Video;
		mt.subtype						= MEDIASUBTYPE_MPEG2_VIDEO;
		mt.formattype					= FORMAT_MPEG2Video;

		MPEG2VIDEOINFO* vih				= (MPEG2VIDEOINFO*)mt.AllocFormatBuffer(FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader));
		memset(mt.Format(), 0, mt.FormatLength());
		vih->hdr.bmiHeader.biSize		= sizeof(vih->hdr.bmiHeader);
		vih->hdr.bmiHeader.biWidth		= w;
		vih->hdr.bmiHeader.biHeight		= h;
		vih->hdr.bmiHeader.biPlanes		= 1;
		vih->hdr.bmiHeader.biBitCount	= 12;
		vih->hdr.bmiHeader.biSizeImage	= DIBSIZE(vih->hdr.bmiHeader);

		vih->cbSequenceHeader		= 0;

		return true;
	}

	BYTE* seqhdr_ext = nullptr;
	BYTE* seqhdr_end = seqhdr + 7;

	while (seqhdr_end < (seqhdr + len - 6)) {
		if (GETU32(seqhdr_end) == 0xb5010000) {
			seqhdr_ext = seqhdr_end;
			seqhdr_end += 10;
			len = (DWORD)(seqhdr_end - seqhdr);
			break;
		}
		seqhdr_end++;
	}

	mt.majortype					= MEDIATYPE_Video;
	mt.subtype						= MEDIASUBTYPE_MPEG2_VIDEO;
	mt.formattype					= FORMAT_MPEG2Video;

	MPEG2VIDEOINFO* vih				= (MPEG2VIDEOINFO*)mt.AllocFormatBuffer(FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader) + len);
	memset(mt.Format(), 0, mt.FormatLength());
	vih->hdr.bmiHeader.biSize		= sizeof(vih->hdr.bmiHeader);
	vih->hdr.bmiHeader.biWidth		= w;
	vih->hdr.bmiHeader.biHeight		= h;
	vih->hdr.bmiHeader.biPlanes		= 1;
	vih->hdr.bmiHeader.biBitCount	= 12;
	vih->hdr.bmiHeader.biSizeImage	= DIBSIZE(vih->hdr.bmiHeader);


	BYTE* pSequenceHeader		= (BYTE*)vih->dwSequenceHeader;
	memcpy(pSequenceHeader, seqhdr, len);
	vih->cbSequenceHeader		= len;

	static DWORD profile[8] = {
		0, AM_MPEG2Profile_High, AM_MPEG2Profile_SpatiallyScalable, AM_MPEG2Profile_SNRScalable,
		AM_MPEG2Profile_Main, AM_MPEG2Profile_Simple, 0, 0
	};

	static DWORD level[16] = {
		0, 0, 0, 0,
		AM_MPEG2Level_High, 0, AM_MPEG2Level_High1440, 0,
		AM_MPEG2Level_Main, 0, AM_MPEG2Level_Low, 0,
		0, 0, 0, 0
	};

	if (seqhdr_ext && (seqhdr_ext[4] & 0xf0) == 0x10) {
		vih->dwProfile	= profile[seqhdr_ext[4] & 0x07];
		vih->dwLevel	= level[seqhdr_ext[5] >> 4];
	}

	return true;
}

bool CreateFilter(CString DisplayName, IBaseFilter** ppBF, CString& FriendlyName)
{
	if (!ppBF) {
		return false;
	}

	*ppBF = nullptr;
	FriendlyName.Empty();

	CComPtr<IBindCtx> pBindCtx;
	CreateBindCtx(0, &pBindCtx);

	CComPtr<IMoniker> pMoniker;
	ULONG chEaten;
	if (S_OK != MkParseDisplayName(pBindCtx, CComBSTR(DisplayName), &chEaten, &pMoniker)) {
		return false;
	}

	if (FAILED(pMoniker->BindToObject(pBindCtx, 0, IID_IBaseFilter, (void**)ppBF)) || !*ppBF) {
		return false;
	}

	CComPtr<IPropertyBag> pPB;
	CComVariant var;
	if (SUCCEEDED(pMoniker->BindToStorage(pBindCtx, 0, IID_IPropertyBag, (void**)&pPB))
			&& SUCCEEDED(pPB->Read(CComBSTR(L"FriendlyName"), &var, nullptr))) {
		FriendlyName = var.bstrVal;
	}

	return true;
}

bool HasMediaType(IFilterGraph *pFilterGraph, const GUID &mediaType)
{
	CheckPointer(pFilterGraph, false);
	bool bFound = false;

	BeginEnumFilters(pFilterGraph, pEF, pBF)
		BeginEnumPins(pBF, pEP, pPin)
			PIN_DIRECTION dir;
			if (SUCCEEDED(pPin->QueryDirection(&dir)) && dir == PINDIR_OUTPUT) {
				CComPtr<IPin> pPinConnectedTo;
				if (SUCCEEDED(pPin->ConnectedTo(&pPinConnectedTo))) {
					BeginEnumMediaTypes(pPin, pEM, pmt)
						if (pmt->majortype == mediaType || pmt->subtype == mediaType) {
							bFound = true;
							break;
						}
					EndEnumMediaTypes(pmt)
				}
			}
			if (bFound) {
				break;
			}
		EndEnumPins
		if (bFound) {
			break;
		}
	EndEnumFilters

	return bFound;
}

IBaseFilter* AppendFilter(IPin* pPin, IMoniker* pMoniker, IGraphBuilder* pGB)
{
	do {
		if (!pPin || !pMoniker || !pGB) {
			break;
		}

		{
			CComPtr<IPin> pPinTo;
			PIN_DIRECTION dir;
			if (FAILED(pPin->QueryDirection(&dir)) || dir != PINDIR_OUTPUT || SUCCEEDED(pPin->ConnectedTo(&pPinTo))) {
				break;
			}
		}

		CComPtr<IBindCtx> pBindCtx;
		CreateBindCtx(0, &pBindCtx);

		CComPtr<IPropertyBag> pPB;
		if (FAILED(pMoniker->BindToStorage(pBindCtx, 0, IID_IPropertyBag, (void**)&pPB))) {
			break;
		}

		CComVariant var;
		if (FAILED(pPB->Read(CComBSTR(L"FriendlyName"), &var, nullptr))) {
			break;
		}

		CComPtr<IBaseFilter> pBF;
		if (FAILED(pMoniker->BindToObject(pBindCtx, 0, IID_IBaseFilter, (void**)&pBF)) || !pBF) {
			break;
		}

		if (FAILED(pGB->AddFilter(pBF, CStringW(var.bstrVal)))) {
			break;
		}

		BeginEnumPins(pBF, pEP, pPinTo) {
			PIN_DIRECTION dir;
			if (FAILED(pPinTo->QueryDirection(&dir)) || dir != PINDIR_INPUT) {
				continue;
			}

			if (SUCCEEDED(pGB->ConnectDirect(pPin, pPinTo, nullptr))) {
				return pBF;
			}
		}
		EndEnumFilters

		pGB->RemoveFilter(pBF);
	} while (false);

	return nullptr;
}

CString GetFriendlyName(CString DisplayName)
{
	CString FriendlyName;

	CComPtr<IBindCtx> pBindCtx;
	CreateBindCtx(0, &pBindCtx);

	CComPtr<IMoniker> pMoniker;
	ULONG chEaten;
	if (S_OK != MkParseDisplayName(pBindCtx, CComBSTR(DisplayName), &chEaten, &pMoniker)) {
		return L"";
	}

	CComPtr<IPropertyBag> pPB;
	CComVariant var;
	if (SUCCEEDED(pMoniker->BindToStorage(pBindCtx, 0, IID_IPropertyBag, (void**)&pPB))
			&& SUCCEEDED(pPB->Read(CComBSTR(L"FriendlyName"), &var, nullptr))) {
		FriendlyName = var.bstrVal;
	}

	return FriendlyName;
}

struct ExternalObject {
	CString path;
	HINSTANCE hInst;
	CLSID clsid;
};

static std::list<ExternalObject> s_extobjs;

HRESULT LoadExternalObject(LPCWSTR path, REFCLSID clsid, REFIID iid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	CString fullpath = MakeFullPath(path);

	HINSTANCE hInst = nullptr;
	bool fFound = false;

	for (const auto& eo : s_extobjs) {
		if (!eo.path.CompareNoCase(fullpath)) {
			hInst = eo.hInst;
			fFound = true;
			break;
		}
	}

	HRESULT hr = E_FAIL;

	if (!hInst) {
		hInst = CoLoadLibrary(CComBSTR(fullpath), TRUE);
	}
	if (hInst) {
		typedef HRESULT (__stdcall * PDllGetClassObject)(REFCLSID rclsid, REFIID riid, LPVOID* ppv);
		PDllGetClassObject p = (PDllGetClassObject)GetProcAddress(hInst, "DllGetClassObject");

		if (p && FAILED(hr = p(clsid, iid, ppv))) {
			CComPtr<IClassFactory> pCF;
			if (SUCCEEDED(hr = p(clsid, __uuidof(IClassFactory), (void**)&pCF))) {
				hr = pCF->CreateInstance(nullptr, iid, ppv);
			}
		}
	}

	if (FAILED(hr) && hInst && !fFound) {
		CoFreeLibrary(hInst);
		return hr;
	}

	if (hInst && !fFound) {
		ExternalObject eo;
		eo.path = fullpath;
		eo.hInst = hInst;
		eo.clsid = clsid;
		s_extobjs.push_back(eo);
	}

	return hr;
}

HRESULT LoadExternalFilter(LPCWSTR path, REFCLSID clsid, IBaseFilter** ppBF)
{
	return LoadExternalObject(path, clsid, __uuidof(IBaseFilter), (void**)ppBF);
}

HRESULT LoadExternalPropertyPage(IPersist* pP, REFCLSID clsid, IPropertyPage** ppPP)
{
	CLSID clsid2 = GUID_NULL;
	if (FAILED(pP->GetClassID(&clsid2))) {
		return E_FAIL;
	}

	for (const auto& eo : s_extobjs) {
		if (eo.clsid == clsid2) {
			return LoadExternalObject(eo.path, clsid, __uuidof(IPropertyPage), (void**)ppPP);
		}
	}

	return E_FAIL;
}

void UnloadExternalObjects()
{
	for (auto& eo : s_extobjs) {
		CoFreeLibrary(eo.hInst);
	}
	s_extobjs.clear();
}

CStringW MakeFullPath(LPCWSTR path)
{
	CStringW full(path);
	full.Replace('/', '\\');

	CStringW base = GetProgramPath();

	if (full.GetLength() >= 2 && full[0] == '\\' && full[1] != '\\') {
		StripToRoot(base);
		full = base + full.Mid(1);
	} else if (full.Find(L":\\") < 0) {
		RemoveFileSpec(base);
		full = base + L"\\" + full;
	}

	CStringW c = GetFullCannonFilePath(full);
	return c;
}


bool IsLikelyFilePath(const CStringW& str) // simple file system path detector
{
	auto IsLatin = [](wchar_t ch) { return (ch >= 'A' && ch <= 'Z' || ch >= 'a' && ch <= 'z'); };

	if (str.GetLength() >= 4) {
		// local file path 'x:\'
		if (str[1] == ':' && str[2] == '\\' && IsLatin(str[0])) {
			return true;
		}

		if (str.GetLength() >= 7 && str[0] == '\\' && str[1] == '\\') {
			// net file path '\\servername'
			if (IsLatin(str[2]) || str[2] == '-' || str[2] >= '0' && str[2] <= '9') {
				return true;
			}
			// local file path with prefix '\\?\x:\'
			if (str[2] == '?' && str[3] == '\\' && str[5] == ':' && str[6] == '\\' && IsLatin(str[4])) {
				return true;
			}
		}
	}

	return false;
}

//

GUID GUIDFromCString(CString str)
{
	GUID guid = GUID_NULL;
	HRESULT hr = CLSIDFromString(CComBSTR(str), &guid);
	//ASSERT(SUCCEEDED(hr));
	UNREFERENCED_PARAMETER(hr);
	return guid;
}

HRESULT GUIDFromCString(CString str, GUID& guid)
{
	guid = GUID_NULL;
	return CLSIDFromString(CComBSTR(str), &guid);
}

CStringW CStringFromGUID(const GUID& guid)
{
	CStringW str;
	int ret = StringFromGUID2(guid, str.GetBuffer(39), 39);
	if (ret) {
		str.ReleaseBufferSetLength(ret - 1);
	} else {
		str.Empty();
	}
	return str;
}

int MakeAACInitData(BYTE* pData, int profile, int freq, int channels)
{
	int srate_idx;

	if (92017 <= freq) {
		srate_idx = 0;
	} else if (75132 <= freq) {
		srate_idx = 1;
	} else if (55426 <= freq) {
		srate_idx = 2;
	} else if (46009 <= freq) {
		srate_idx = 3;
	} else if (37566 <= freq) {
		srate_idx = 4;
	} else if (27713 <= freq) {
		srate_idx = 5;
	} else if (23004 <= freq) {
		srate_idx = 6;
	} else if (18783 <= freq) {
		srate_idx = 7;
	} else if (13856 <= freq) {
		srate_idx = 8;
	} else if (11502 <= freq) {
		srate_idx = 9;
	} else if (9391 <= freq) {
		srate_idx = 10;
	} else {
		srate_idx = 11;
	}

	pData[0] = ((abs(profile) + 1) << 3) | ((srate_idx & 0xe) >> 1);
	pData[1] = ((srate_idx & 0x1) << 7) | (channels << 3);

	int ret = 2;

	if (profile < 0) {
		freq *= 2;

		if (92017 <= freq) {
			srate_idx = 0;
		} else if (75132 <= freq) {
			srate_idx = 1;
		} else if (55426 <= freq) {
			srate_idx = 2;
		} else if (46009 <= freq) {
			srate_idx = 3;
		} else if (37566 <= freq) {
			srate_idx = 4;
		} else if (27713 <= freq) {
			srate_idx = 5;
		} else if (23004 <= freq) {
			srate_idx = 6;
		} else if (18783 <= freq) {
			srate_idx = 7;
		} else if (13856 <= freq) {
			srate_idx = 8;
		} else if (11502 <= freq) {
			srate_idx = 9;
		} else if (9391 <= freq) {
			srate_idx = 10;
		} else {
			srate_idx = 11;
		}

		pData[2] = 0x2B7>>3;
		pData[3] = (BYTE)((0x2B7<<5) | 5);
		pData[4] = (1<<7) | (srate_idx<<3);

		ret = 5;
	}

	return ret;
}

// filter registration helpers

bool DeleteRegKey(LPCWSTR pszKey, LPCWSTR pszSubkey)
{
	bool bOK = false;

	HKEY hKey;
	LONG ec = ::RegOpenKeyExW(HKEY_CLASSES_ROOT, pszKey, 0, KEY_ALL_ACCESS, &hKey);
	if (ec == ERROR_SUCCESS) {
		if (pszSubkey != 0) {
			ec = ::RegDeleteKeyW(hKey, pszSubkey);
		}

		bOK = (ec == ERROR_SUCCESS);

		::RegCloseKey(hKey);
	}

	return bOK;
}

bool SetRegKeyValue(LPCWSTR pszKey, LPCWSTR pszSubkey, LPCWSTR pszValueName, LPCWSTR pszValue)
{
	bool bOK = false;

	CString szKey(pszKey);
	if (pszSubkey != 0) {
		szKey += CString(L"\\") + pszSubkey;
	}

	HKEY hKey;
	LONG ec = ::RegCreateKeyExW(HKEY_CLASSES_ROOT, szKey, 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &hKey, 0);
	if (ec == ERROR_SUCCESS) {
		if (pszValue != 0) {
			ec = ::RegSetValueExW(hKey, pszValueName, 0, REG_SZ,
								 reinterpret_cast<BYTE*>(const_cast<LPWSTR>(pszValue)),
								 (DWORD)(wcslen(pszValue) + 1) * sizeof(WCHAR));
		}

		bOK = (ec == ERROR_SUCCESS);

		::RegCloseKey(hKey);
	}

	return bOK;
}

bool SetRegKeyValue(LPCWSTR pszKey, LPCWSTR pszSubkey, LPCWSTR pszValue)
{
	return SetRegKeyValue(pszKey, pszSubkey, 0, pszValue);
}

void RegisterSourceFilter(const CLSID& clsid, const GUID& subtype2, LPCWSTR chkbytes, LPCWSTR ext, ...)
{
	CString null = CStringFromGUID(GUID_NULL);
	CString majortype = CStringFromGUID(MEDIATYPE_Stream);
	CString subtype = CStringFromGUID(subtype2);

	SetRegKeyValue(L"Media Type\\" + majortype, subtype, L"0", chkbytes);
	SetRegKeyValue(L"Media Type\\" + majortype, subtype, L"Source Filter", CStringFromGUID(clsid));

	DeleteRegKey(L"Media Type\\" + null, subtype);

	va_list marker;
	va_start(marker, ext);
	for (; ext; ext = va_arg(marker, LPCWSTR)) {
		DeleteRegKey(L"Media Type\\Extensions", ext);
	}
	va_end(marker);
}

void RegisterSourceFilter(const CLSID& clsid, const GUID& subtype2, const std::list<CString>& chkbytes, LPCWSTR ext, ...)
{
	CString null = CStringFromGUID(GUID_NULL);
	CString majortype = CStringFromGUID(MEDIATYPE_Stream);
	CString subtype = CStringFromGUID(subtype2);

	auto it = chkbytes.begin();
	for (int i = 0; it != chkbytes.end(); i++) {
		CString idx;
		idx.Format(L"%d", i);
		SetRegKeyValue(L"Media Type\\" + majortype, subtype, idx, *it++);
	}

	SetRegKeyValue(L"Media Type\\" + majortype, subtype, L"Source Filter", CStringFromGUID(clsid));

	DeleteRegKey(L"Media Type\\" + null, subtype);

	va_list marker;
	va_start(marker, ext);
	for (; ext; ext = va_arg(marker, LPCWSTR)) {
		DeleteRegKey(L"Media Type\\Extensions", ext);
	}
	va_end(marker);
}


void UnRegisterSourceFilter(const GUID& subtype)
{
	DeleteRegKey(L"Media Type\\" + CStringFromGUID(MEDIATYPE_Stream), CStringFromGUID(subtype));
}

static const struct {
	const GUID*  Guid;
	const WCHAR* Description;
} s_dxva2_vld_decoders[] = {
	// MPEG1
	{&DXVA2_ModeMPEG1_VLD,							L"MPEG-1"},
	// MPEG2
	{&DXVA2_ModeMPEG2_VLD,							L"MPEG-2"},
	{&DXVA2_ModeMPEG2and1_VLD,						L"MPEG-2/MPEG-1"},
	// MPEG4
	{&DXVA2_ModeMPEG4pt2_VLD_Simple,				L"MPEG-4 SP"},
	{&DXVA2_ModeMPEG4pt2_VLD_AdvSimple_NoGMC,		L"MPEG-4 ASP, no GMC"},
	{&DXVA2_ModeMPEG4pt2_VLD_AdvSimple_GMC,			L"MPEG-4 ASP, GMC"},
	{&DXVA2_MPEG4pt2_VLD_AdvSimple_Nvidia,			L"MPEG-4 ASP (Nvidia)"},
	// VC-1
	{&DXVA2_ModeVC1_D,/*DXVA2_ModeVC1_VLD*/			L"VC-1"},
	{&DXVA2_ModeVC1_D2010,							L"VC-1 (2010)"},
	{&DXVA2_VC1_VLD_Intel,							L"VC-1 (Intel)"},
	{&DXVA2_VC1_VLD_2_Intel,						L"VC-1 (Intel, 2)"},
	// H.264
	{&DXVA2_ModeH264_E,/*DXVA2_ModeH264_VLD_NoFGT*/	L"H.264, no FGT"},
	{&DXVA2_ModeH264_F,/*DXVA2_ModeH264_VLD_FGT*/	L"H.264, FGT"},
	{&DXVA2_ModeH264_VLD_WithFMOASO_NoFGT,			L"H.264, no FGT, with FMOASO"},
	{&DXVA2_H264_VLD_Intel,							L"H.264 (Intel)"},
	{&DXVA2_H264_VLD_NoFGT_AMD,						L"H.264 (AMD)"},
	// H.264 stereo
	{&DXVA2_ModeH264_VLD_Stereo_Progressive_NoFGT,	L"H.264 stereo progressive, no FGT"},
	{&DXVA2_ModeH264_VLD_Stereo_NoFGT,				L"H.264 stereo, no FGT"},
	{&DXVA2_ModeH264_VLD_Multiview_NoFGT,			L"H.264 multiview, no FGT"},
	// HEVC
	{&DXVA2_ModeHEVC_VLD_Main,						L"HEVC"},
	{&DXVA2_ModeHEVC_VLD_Main10,					L"HEVC 10-bit"},

	{&D3D11_DECODER_PROFILE_HEVC_VLD_MAIN12,		L"HEVC 12-bit"},
	{&D3D11_DECODER_PROFILE_HEVC_VLD_MAIN10_422,	L"HEVC 422 10-bit"},
	{&D3D11_DECODER_PROFILE_HEVC_VLD_MAIN12_422,	L"HEVC 422 12-bit"},
	{&D3D11_DECODER_PROFILE_HEVC_VLD_MAIN_444,		L"HEVC 444 8-bit"},
	{&D3D11_DECODER_PROFILE_HEVC_VLD_MAIN10_444,	L"HEVC 444 10-bit"},
	{&D3D11_DECODER_PROFILE_HEVC_VLD_MAIN12_444,	L"HEVC 444 12-bit"},
	// VP8
	{&DXVA2_ModeVP8_VLD,							L"VP8"},
	// VP9
	{&DXVA2_ModeVP9_VLD_Profile0,					L"VP9"},
	{&DXVA2_ModeVP9_VLD_10bit_Profile2,				L"VP9 10-bit"},
	{&DXVA2_VP9_VLD_Intel,							L"VP9 Intel"},
	// AV1
	{&DXVA2_ModeAV1_VLD_Profile0,					L"AV1 profile 0"},
	// HEVC Intel
	{&DXVA2_HEVC_VLD_Main12_Intel,					L"HEVC 12-bit Intel"},
	{&DXVA2_HEVC_VLD_Main422_10_Intel,				L"HEVC 422 10-bit Intel"},
	{&DXVA2_HEVC_VLD_Main422_12_Intel,				L"HEVC 422 12-bit Intel"},
	{&DXVA2_HEVC_VLD_Main444_Intel,					L"HEVC 444 8-bit Intel"},
	{&DXVA2_HEVC_VLD_Main444_10_Intel,				L"HEVC 444 10-bit Intel"},
	{&DXVA2_HEVC_VLD_Main444_12_Intel,				L"HEVC 444 12-bit Intel"},
};

CStringW GetDXVAModeString(const GUID& guidDecoder)
{
	if (guidDecoder == GUID_NULL) {
		return L"Not using DXVA";
	}

	for (const auto& decoder : s_dxva2_vld_decoders) {
		if (guidDecoder == *decoder.Guid) {
			return decoder.Description;
		}
	}

	return CStringFromGUID(guidDecoder);
}

CStringW GetDXVAModeStringAndName(const GUID& guidDecoder)
{
	CStringW str = CStringFromGUID(guidDecoder) + L" ";

	if (guidDecoder == GUID_NULL) {
		str.Append(L"Not using DXVA");
		return str;
	}

	for (const auto& decoder : s_dxva2_vld_decoders) {
		if (guidDecoder == *decoder.Guid) {
			str.Append(decoder.Description);
			return str;
		}
	}

	str.Append(L"Unknown");
	return str;
}

CStringW DVDtimeToString(const DVD_HMSF_TIMECODE dvd_tc, bool showZeroHours)
{
	CStringW str;
	if (dvd_tc.bHours > 0 || showZeroHours) {
		str.Format(L"%02d:%02d:%02d", dvd_tc.bHours, dvd_tc.bMinutes, dvd_tc.bSeconds);
	} else {
		str.Format(L"%02d:%02d", dvd_tc.bMinutes, dvd_tc.bSeconds);
	}
	return str;
}

REFERENCE_TIME StringToReftime(LPCWSTR strVal)
{
	REFERENCE_TIME	rt			= 0;
	int				lHour		= 0;
	int				lMinute		= 0;
	int				lSecond		= 0;
	int				lMillisec	= 0;

	if (swscanf_s(strVal, L"%02d:%02d:%02d.%03d", &lHour, &lMinute, &lSecond, &lMillisec) == 4
			|| swscanf_s(strVal, L"%02d:%02d:%02d,%03d", &lHour, &lMinute, &lSecond, &lMillisec) == 4) {
		rt = ((((lHour * 60) + lMinute) * 60 + lSecond) * MILLISECONDS + lMillisec) * (UNITS/MILLISECONDS);
	}

	return rt;
}

REFERENCE_TIME StringToReftime2(LPCWSTR strVal)
{
	REFERENCE_TIME	rt			= 0;
	int				lHour		= 0;
	int				lMinute		= 0;
	int				lSecond		= 0;

	if (swscanf_s (strVal, L"%02d:%02d:%02d", &lHour, &lMinute, &lSecond) == 3) {
		rt = (((lHour * 60) + lMinute) * 60 + lSecond) * UNITS;
	}

	return rt;
}

void TraceFilterInfo(IBaseFilter* pBF)
{
	FILTER_INFO Info;
	if (SUCCEEDED (pBF->QueryFilterInfo(&Info))) {
		DLog(L" === Filter info : %s", Info.achName);
		BeginEnumPins(pBF, pEnum, pPin) {
			TracePinInfo(pPin);
		}

		EndEnumPins
		Info.pGraph->Release();
	}
}

void TracePinInfo(IPin* pPin)
{
	PIN_INFO		PinInfo;
	FILTER_INFO		ConnectedFilterInfo;
	PIN_INFO		ConnectedInfo;
	CComPtr<IPin>	pConnected;
	memset (&ConnectedInfo, 0, sizeof(ConnectedInfo));
	memset (&ConnectedFilterInfo, 0, sizeof(ConnectedFilterInfo));
	if (SUCCEEDED (pPin->ConnectedTo  (&pConnected))) {
		pConnected->QueryPinInfo (&ConnectedInfo);
		ConnectedInfo.pFilter->QueryFilterInfo(&ConnectedFilterInfo);
		ConnectedInfo.pFilter->Release();
		ConnectedFilterInfo.pGraph->Release();
	}
	pPin->QueryPinInfo (&PinInfo);
	DLog(L"        %s (%s) -> %s (Filter %s)",
		  PinInfo.achName,
		  PinInfo.dir == PINDIR_OUTPUT ? L"Out" : L"In",
		  ConnectedInfo.achName,
		  ConnectedFilterInfo.achName);
	PinInfo.pFilter->Release();
}

const wchar_t *StreamTypeToName(PES_STREAM_TYPE _Type)
{
	switch (_Type) {
		case VIDEO_STREAM_MPEG1:
				return L"MPEG-1";
		case VIDEO_STREAM_MPEG2:
				return L"MPEG-2";
		case AUDIO_STREAM_MPEG1:
				return L"MPEG-1";
		case AUDIO_STREAM_MPEG2:
				return L"MPEG-2";
		case VIDEO_STREAM_H264:
				return L"H.264";
		case AUDIO_STREAM_LPCM:
				return L"LPCM";
		case AUDIO_STREAM_AC3:
				return L"Dolby Digital";
		case AUDIO_STREAM_DTS:
				return L"DTS";
		case AUDIO_STREAM_AC3_TRUE_HD:
				return L"Dolby TrueHD";
		case AUDIO_STREAM_AC3_PLUS:
				return L"Dolby Digital Plus";
		case AUDIO_STREAM_DTS_HD:
				return L"DTS-HD HRA";
		case AUDIO_STREAM_DTS_HD_MASTER_AUDIO:
				return L"DTS-HD MA";
		case PRESENTATION_GRAPHICS_STREAM:
				return L"PGS";
		case INTERACTIVE_GRAPHICS_STREAM:
				return L"Interactive Graphics Stream";
		case SUBTITLE_STREAM:
				return L"Subtitle";
		case SECONDARY_AUDIO_AC3_PLUS:
				return L"Secondary Dolby Digital Plus";
		case SECONDARY_AUDIO_DTS_HD:
				return L"DTS Express";
		case VIDEO_STREAM_VC1:
				return L"VC-1";
	}
	return nullptr;
}

unsigned int lav_xiphlacing(unsigned char *s, unsigned int v)
{
	unsigned int n = 0;

	while (v >= 0xff) {
		*s++ = 0xff;
		v -= 0xff;
		n++;
	}
	*s = v;
	n++;
	return n;
}

void getExtraData(const BYTE *format, const GUID *formattype, const ULONG formatlen, BYTE *extra, unsigned int *extralen)
{
	// code from LAV ...
	const BYTE *extraposition = nullptr;
	ULONG extralength = 0;
	if (*formattype == FORMAT_WaveFormatEx) {
		const auto wfex = (WAVEFORMATEX *)format;
		if (wfex->wFormatTag == WAVE_FORMAT_EXTENSIBLE && formatlen >= sizeof(WAVEFORMATEXTENSIBLE)) {
			extraposition = format + sizeof(WAVEFORMATEXTENSIBLE);
			// Protected against over-reads
			extralength = formatlen - sizeof(WAVEFORMATEXTENSIBLE);
		} else {
			extraposition = format + sizeof(WAVEFORMATEX);
			// Protected against over-reads
			extralength = formatlen - sizeof(WAVEFORMATEX);
		}
	} else if (*formattype == FORMAT_VorbisFormat2) {
		VORBISFORMAT2 *vf2 = (VORBISFORMAT2 *)format;
		unsigned offset = 1;
		if (extra) {
			*extra = 2;
			offset += lav_xiphlacing(extra+offset, vf2->HeaderSize[0]);
			offset += lav_xiphlacing(extra+offset, vf2->HeaderSize[1]);
			extra += offset;
		} else {
			offset += vf2->HeaderSize[0] / 255 + 1;
			offset += vf2->HeaderSize[1] / 255 + 1;
		}
		extralength = vf2->HeaderSize[0] + vf2->HeaderSize[1] + vf2->HeaderSize[2];
		extralength = std::min(extralength, formatlen - (ULONG)sizeof(VORBISFORMAT2));

		if (extra && extralength)
			memcpy(extra, format + sizeof(VORBISFORMAT2), extralength);
		if (extralen)
			*extralen = (unsigned int)extralength + offset;

		return;
	} else if (*formattype == FORMAT_VideoInfo) {
		extraposition = format + sizeof(VIDEOINFOHEADER);
		extralength   = formatlen - sizeof(VIDEOINFOHEADER);
	} else if (*formattype == FORMAT_VideoInfo2) {
		extraposition = format + sizeof(VIDEOINFOHEADER2);
		extralength   = formatlen - sizeof(VIDEOINFOHEADER2);
	} else if (*formattype == FORMAT_MPEGVideo) {
		MPEG1VIDEOINFO *mp1vi = (MPEG1VIDEOINFO *)format;
		extraposition = (BYTE *)mp1vi->bSequenceHeader;
		extralength   =  std::min(mp1vi->cbSequenceHeader, formatlen - FIELD_OFFSET(MPEG1VIDEOINFO, bSequenceHeader[0]));
	} else if (*formattype == FORMAT_MPEG2Video) {
		MPEG2VIDEOINFO *mp2vi = (MPEG2VIDEOINFO *)format;
		extraposition = (BYTE *)mp2vi->dwSequenceHeader;
		extralength   = std::min(mp2vi->cbSequenceHeader, formatlen - FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader[0]));
	}

	if (extra && extralength)
		memcpy(extra, extraposition, extralength);
	if (extralen)
		*extralen = extralength;
}

HRESULT CreateMPEG2VIfromAVC(CMediaType* mt, BITMAPINFOHEADER* pbmi, REFERENCE_TIME AvgTimePerFrame, CSize pictAR, BYTE* extra, size_t extralen)
{
	RECT rc = {0, 0, pbmi->biWidth, abs(pbmi->biHeight)};

	mt->majortype					= MEDIATYPE_Video;
	mt->formattype					= FORMAT_MPEG2Video;

	MPEG2VIDEOINFO* pm2vi			= (MPEG2VIDEOINFO*)mt->AllocFormatBuffer(FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader) + extralen);
	memset(pm2vi, 0, mt->FormatLength());
	memcpy(&pm2vi->hdr.bmiHeader, pbmi, sizeof(BITMAPINFOHEADER));

	pm2vi->hdr.dwPictAspectRatioX	= pictAR.cx;
	pm2vi->hdr.dwPictAspectRatioY	= pictAR.cy;
	pm2vi->hdr.AvgTimePerFrame		= AvgTimePerFrame;
	pm2vi->hdr.rcSource				= pm2vi->hdr.rcTarget = rc;
	pm2vi->cbSequenceHeader			= 0;

	HRESULT hr = S_OK;

	if (extralen > 5 && extra && extra[0] == 1) {
		pm2vi->dwProfile	= extra[1];
		pm2vi->dwLevel		= extra[3];
		pm2vi->dwFlags		= (extra[4] & 3) + 1;

		BYTE* src = (BYTE*)extra + 5;
		BYTE* dst = (BYTE*)pm2vi->dwSequenceHeader;

		BYTE* src_end = (BYTE*)extra + extralen;
		BYTE* dst_end = (BYTE*)pm2vi->dwSequenceHeader + extralen;

		for (int i = 0; i < 2; ++i) {
			for (int n = *src++ & 0x1f; n > 0; --n) {
				int len = ((src[0] << 8) | src[1]) + 2;
				if (src + len > src_end || dst + len > dst_end) {
					ASSERT(0);
					break;
				}
				memcpy(dst, src, len);
				src += len;
				dst += len;
				pm2vi->cbSequenceHeader += len;
			}
		}
	} else {
		hr = E_FAIL;

		pm2vi->cbSequenceHeader = extralen;
		memcpy(&pm2vi->dwSequenceHeader[0], extra, extralen);
	}

	mt->subtype = FOURCCMap(pm2vi->hdr.bmiHeader.biCompression);
	mt->SetSampleSize(pbmi->biWidth * pbmi-> biHeight * 4);

	if (pm2vi->cbSequenceHeader < extralen) {
		mt->ReallocFormatBuffer(FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader) + pm2vi->cbSequenceHeader);
	}

	return hr;
}

HRESULT CreateMPEG2VIfromMVC(CMediaType* mt, BITMAPINFOHEADER* pbmi, REFERENCE_TIME AvgTimePerFrame, CSize pictAR, BYTE* extra, size_t extralen)
{
	// code from LAV Source ... thanks to it's author
	if (extralen > 8 && extra && extra[0] == 1) {
		// Find "mvcC" atom
		DWORD state = 0;
		size_t i = 0;
		for (; i < extralen - 4; i++) {
			state = (state << 8) | extra[i];
			if (state == 'mvcC') {
				break;
			}
		}

		if (i == extralen || i < 8) {
			return E_FAIL;
		}

		// Update pointers to the start of the mvcC atom
		extra = extra + i - 7;
		extralen = extralen - i + 7;
		DWORD atomSize = GETU32(extra); atomSize = FCC(atomSize);

		// verify size atom and actual size
		if ((atomSize + 4) > extralen || extralen < 14) {
			return E_FAIL;
		}

		// Skip atom headers
		extra += 8;
		extralen -= 8;

		pbmi->biCompression = FCC('MVC1');
		return CreateMPEG2VIfromAVC(mt, pbmi, AvgTimePerFrame, pictAR, extra, extralen);
	}

	return E_FAIL;
}

HRESULT CreateMPEG2VISimple(CMediaType* mt, BITMAPINFOHEADER* pbmi, REFERENCE_TIME AvgTimePerFrame, CSize pictAR, BYTE* extra, size_t extralen, DWORD dwProfile/* = 0*/, DWORD dwLevel/* = 0*/, DWORD dwFlags/* = 0*/)
{
	CheckPointer(mt, E_FAIL);

	RECT rc = {0, 0, pbmi->biWidth, abs(pbmi->biHeight)};

	mt->majortype					= MEDIATYPE_Video;
	mt->formattype					= FORMAT_MPEG2Video;

	MPEG2VIDEOINFO* pm2vi			= (MPEG2VIDEOINFO*)mt->AllocFormatBuffer(FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader) + extralen);
	memset(pm2vi, 0, mt->FormatLength());
	memcpy(&pm2vi->hdr.bmiHeader, pbmi, sizeof(BITMAPINFOHEADER));

	pm2vi->hdr.dwPictAspectRatioX	= pictAR.cx;
	pm2vi->hdr.dwPictAspectRatioY	= pictAR.cy;
	pm2vi->hdr.AvgTimePerFrame		= AvgTimePerFrame;
	pm2vi->hdr.rcSource				= pm2vi->hdr.rcTarget = rc;

	pm2vi->dwProfile				= dwProfile;
	pm2vi->dwLevel					= dwLevel;
	pm2vi->dwFlags					= dwFlags;
	pm2vi->cbSequenceHeader			= extralen;
	memcpy(&pm2vi->dwSequenceHeader[0], extra, extralen);

	mt->subtype = FOURCCMap(pm2vi->hdr.bmiHeader.biCompression);

	return S_OK;
}

HRESULT CreateAVCfromH264(CMediaType* mt)
{
	CheckPointer(mt, E_FAIL);

	if (mt->formattype != FORMAT_MPEG2_VIDEO) {
		return E_FAIL;
	}

	MPEG2VIDEOINFO* pm2vi = (MPEG2VIDEOINFO*)mt->pbFormat;
	if (!pm2vi->cbSequenceHeader) {
		return E_FAIL;
	}

	const BYTE* extra     = (BYTE*)&pm2vi->dwSequenceHeader[0];
	const DWORD extrasize = pm2vi->cbSequenceHeader;

	std::unique_ptr<BYTE[]> dst_ptr(new(std::nothrow) BYTE[extrasize]);
	if (!dst_ptr) {
		return E_FAIL;
	}

	auto dst = dst_ptr.get();
	DWORD dstSize = 0;

	CH264Nalu Nalu;
	Nalu.SetBuffer(extra, extrasize);
	while (Nalu.ReadNext()) {
		if (Nalu.GetType() == NALU_TYPE_SPS || Nalu.GetType() == NALU_TYPE_PPS) {
			const size_t nalLength = Nalu.GetDataLength();
			*(dst + dstSize++) = (BYTE)(nalLength >> 8);
			*(dst + dstSize++) = nalLength & 0xff;

			memcpy(dst + dstSize, Nalu.GetDataBuffer(), nalLength);
			dstSize += nalLength;
		}
	}

	if (dstSize) {
		pm2vi = (MPEG2VIDEOINFO*)mt->ReallocFormatBuffer(FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader) + dstSize);

		mt->subtype = MEDIASUBTYPE_AVC1;
		pm2vi->hdr.bmiHeader.biCompression = mt->subtype.Data1;

		if (!pm2vi->dwFlags) {
			pm2vi->dwFlags = 4;
		}

		pm2vi->cbSequenceHeader = dstSize;
		memcpy(&pm2vi->dwSequenceHeader[0], dst, dstSize);

		return S_OK;
	}

	return E_FAIL;
}

void CreateVorbisMediaType(CMediaType& mt, std::vector<CMediaType>& mts, DWORD Channels, DWORD SamplesPerSec, DWORD BitsPerSample, const BYTE* pData, size_t Count)
{
	const BYTE* p = pData;
	std::vector<int> sizes;
	int totalsize = 0;
	for (BYTE n = *p++; n > 0; n--) {
		int size = 0;
		do {
			size += *p;
		} while (*p++ == 0xff);
		sizes.push_back(size);
		totalsize += size;
	}
	sizes.push_back(Count - (p - pData) - totalsize);
	totalsize += sizes[sizes.size()-1];

	if (sizes.size() == 3) {
		mt.subtype          = MEDIASUBTYPE_Vorbis2;
		mt.formattype       = FORMAT_VorbisFormat2;
		VORBISFORMAT2* pvf2 = (VORBISFORMAT2*)mt.AllocFormatBuffer(sizeof(VORBISFORMAT2) + totalsize);
		memset(pvf2, 0, mt.FormatLength());
		pvf2->Channels      = Channels;
		pvf2->SamplesPerSec = SamplesPerSec;
		pvf2->BitsPerSample = BitsPerSample;
		BYTE* p2 = mt.Format() + sizeof(VORBISFORMAT2);
		for (size_t i = 0; i < sizes.size(); p += sizes[i], p2 += sizes[i], i++) {
			memcpy(p2, p, pvf2->HeaderSize[i] = sizes[i]);
		}

		mts.push_back(mt);
	}
	else {
		mt.subtype    = MEDIASUBTYPE_Vorbis;
		mt.formattype = FORMAT_VorbisFormat;
		VORBISFORMAT* vf = (VORBISFORMAT*)mt.AllocFormatBuffer(sizeof(VORBISFORMAT));
		memset(vf, 0, mt.FormatLength());
		vf->nChannels      = Channels;
		vf->nSamplesPerSec = SamplesPerSec;
		vf->nMinBitsPerSec = vf->nMaxBitsPerSec = vf->nAvgBitsPerSec = DWORD_MAX;
		mts.push_back(mt);
	}
}

CStringA VobSubDefHeader(int w, int h, CStringA palette)
{
	CStringA hdr;
	hdr.Format(
		"# VobSub index file, v7 (do not modify this line!)\n"
		"size: %dx%d\n",
		w, h);

	if (palette.IsEmpty()) {
		hdr.Append("custom colors: ON, tridx: 1000, colors: 00000, e0e0e0, 808080, 202020\n");
	} else {
		hdr.AppendFormat("palette: %s\n", palette);
	}

	return hdr;
}

void CorrectWaveFormatEx(CMediaType& mt)
{
	if (mt.majortype == MEDIATYPE_Audio && mt.formattype == FORMAT_WaveFormatEx) {
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.pbFormat;
		WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)wfe;

		if (wfe->wFormatTag == WAVE_FORMAT_PCM && (wfe->nChannels > 2 || wfe->wBitsPerSample != 8 && wfe->wBitsPerSample != 16)
			|| wfe->wFormatTag == WAVE_FORMAT_IEEE_FLOAT && wfe->nChannels > 2) {
			// convert incorrect WAVEFORMATEX to WAVEFORMATEXTENSIBLE

			wfe = (WAVEFORMATEX*)mt.ReallocFormatBuffer(sizeof(WAVEFORMATEXTENSIBLE));
			wfex = (WAVEFORMATEXTENSIBLE*)wfe;

			wfex->Format.wFormatTag				= WAVE_FORMAT_EXTENSIBLE;
			wfex->Format.cbSize					= 22;
			wfex->Samples.wValidBitsPerSample	= wfe->wBitsPerSample;
			wfex->dwChannelMask					= GetDefChannelMask(wfe->nChannels);
			wfex->SubFormat						= mt.subtype;
		} else if (wfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE && wfex->dwChannelMask == 0) {
			// fix empty dwChannelMask
			wfex->dwChannelMask = GetDefChannelMask(wfe->nChannels);
		}
	}
}

inline const LONGLONG GetPerfCounter()
{
	auto GetPerfFrequency = [] {
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		return freq.QuadPart;
	};
	static const LONGLONG llPerfFrequency = GetPerfFrequency();
	if (llPerfFrequency) {
		LARGE_INTEGER llPerfCounter;
		QueryPerformanceCounter(&llPerfCounter);
		return llMulDiv(llPerfCounter.QuadPart, 10000000LL, llPerfFrequency, 0);
	} else {
		// ms to 100ns units
		return timeGetTime() * 10000;
	}
}

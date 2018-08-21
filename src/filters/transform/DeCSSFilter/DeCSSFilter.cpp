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
#include <atlbase.h>
#include <BaseClasses/streams.h>
#include <dvdmedia.h>
#include <ks.h>
#include <ksmedia.h>
#include "DeCSSFilter.h"
#include "../../../DSUtil/DSUtil.h"
#include "CSSauth.h"
#include "CSSscramble.h"
#include <moreuuids.h>

#ifdef REGISTER_FILTER
#include <InitGuid.h>

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] = {
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_NULL},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, nullptr, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, _countof(sudPinTypesOut), sudPinTypesOut}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CDeCSSFilter), DeCSSFilterName, MERIT_DO_NOT_USE, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CDeCSSFilter>, nullptr, &sudFilter[0]},
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

//
// CDeCSSFilter
//

class CKsPSInputPin : public CDeCSSInputPin
{
public:
	CKsPSInputPin(TCHAR* pObjectName, CTransformFilter* pFilter, HRESULT* phr, LPWSTR pName)
		: CDeCSSInputPin(pObjectName, pFilter, phr, pName) {
	}

	// IKsPropertySet
	STDMETHODIMP Set(REFGUID PropSet, ULONG Id, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength) {
		if (CComQIPtr<IKsPropertySet> pKsPS = (static_cast<CDeCSSFilter*>(m_pFilter))->m_pOutput->GetConnected()) {
			return pKsPS->Set(PropSet, Id, InstanceData, InstanceLength, PropertyData, DataLength);
		}
		return E_NOTIMPL;
	}
	STDMETHODIMP Get(REFGUID PropSet, ULONG Id, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength, ULONG* pBytesReturned) {
		if (CComQIPtr<IKsPropertySet> pKsPS = (static_cast<CDeCSSFilter*>(m_pFilter))->m_pOutput->GetConnected()) {
			return pKsPS->Get(PropSet, Id, InstanceData, InstanceLength, PropertyData, DataLength, pBytesReturned);
		}
		return E_NOTIMPL;
	}
	STDMETHODIMP QuerySupported(REFGUID PropSet, ULONG Id, ULONG* pTypeSupport) {
		if (CComQIPtr<IKsPropertySet> pKsPS = (static_cast<CDeCSSFilter*>(m_pFilter))->m_pOutput->GetConnected()) {
			return pKsPS->QuerySupported(PropSet, Id, pTypeSupport);
		}
		return E_NOTIMPL;
	}
};

CDeCSSFilter::CDeCSSFilter(LPUNKNOWN lpunk, HRESULT* phr)
	: CTransformFilter(L"CDeCSSFilter", lpunk, __uuidof(this))
{
	if (phr) {
		*phr = S_OK;
	}

	m_pInput = DNew CKsPSInputPin(L"CKsPSInputPin", this, phr, L"In");
	if (!m_pInput) {
		*phr = E_OUTOFMEMORY;
	}
	if (FAILED(*phr)) {
		return;
	}

	m_pOutput = DNew CTransformOutputPin(L"CTransformOutputPin", this, phr, L"Out");
	if (!m_pOutput) {
		*phr = E_OUTOFMEMORY;
	}
	if (FAILED(*phr))  {
		delete m_pInput, m_pInput = nullptr;
		return;
	}
}

CDeCSSFilter::~CDeCSSFilter()
{
}

HRESULT CDeCSSFilter::Transform(IMediaSample* pIn, IMediaSample* pOut)
{
	AM_MEDIA_TYPE* pmt;
	if (SUCCEEDED(pIn->GetMediaType(&pmt)) && pmt) {
		CMediaType mt = *pmt;
		m_pInput->SetMediaType(&mt);
		mt.majortype = m_pOutput->CurrentMediaType().majortype;
		m_pOutput->SetMediaType(&mt);
		pOut->SetMediaType(&mt);
		DeleteMediaType(pmt);
	}

	BYTE* pDataIn = nullptr;
	BYTE* pDataOut = nullptr;

	pIn->GetPointer(&pDataIn);
	pOut->GetPointer(&pDataOut);

	long len = pIn->GetActualDataLength();
	long size = pOut->GetSize();

	if (len == 0 || pDataIn == nullptr) { // format changes do not carry any data
		pOut->SetActualDataLength(0);
		return S_OK;
	}

	if (m_pOutput->CurrentMediaType().majortype == MEDIATYPE_MPEG2_PES) {
		if (GETU32(pDataIn) == 0xBA010000) {
			len -= 14;
			pDataIn += 14;
			if (int stuffing = (pDataIn[-1]&7)) {
				len -= stuffing;
				pDataIn += stuffing;
			}
		}
		if (len <= 0) {
			return S_FALSE;
		}
		if (GETU32(pDataIn) == 0xBB010000) {
			len -= 4;
			pDataIn += 4;
			int hdrlen = ((pDataIn[0]<<8)|pDataIn[1]) + 2;
			len -= hdrlen;
			pDataIn += hdrlen;
		}
		if (len <= 0) {
			return S_FALSE;
		}
	}

	if (!pDataIn || !pDataOut || len > size || len < 0) {
		return S_FALSE;
	}

	memcpy(pDataOut, pDataIn, std::min(len, size));
	pOut->SetActualDataLength(std::min(len, size));

	return S_OK;
}

HRESULT CDeCSSFilter::CheckInputType(const CMediaType* mtIn)
{
	return mtIn->majortype == MEDIATYPE_DVD_ENCRYPTED_PACK
		   ? S_OK
		   : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CDeCSSFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	return SUCCEEDED(CheckInputType(mtIn))
		   && mtOut->majortype == MEDIATYPE_MPEG2_PACK || mtOut->majortype == MEDIATYPE_MPEG2_PES
		   ? S_OK
		   : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CDeCSSFilter::DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties)
{
	if (m_pInput->IsConnected() == FALSE) {
		return E_UNEXPECTED;
	}

	pProperties->cbAlign = 1;
	pProperties->cBuffers = 1;
	pProperties->cbBuffer = 2048;
	pProperties->cbPrefix = 0;

	HRESULT hr;
	ALLOCATOR_PROPERTIES Actual;
	if (FAILED(hr = pAllocator->SetProperties(pProperties, &Actual))) {
		return hr;
	}

	return(pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer
		   ? E_FAIL
		   : NOERROR);
}

HRESULT CDeCSSFilter::GetMediaType(int iPosition, CMediaType* pmt)
{
	if (m_pInput->IsConnected() == FALSE) {
		return E_UNEXPECTED;
	}

	if (iPosition < 0) {
		return E_INVALIDARG;
	}
	if (iPosition > 1) {
		return VFW_S_NO_MORE_ITEMS;
	}

	CopyMediaType(pmt, &m_pInput->CurrentMediaType());
	if (iPosition == 0) {
		pmt->majortype = MEDIATYPE_MPEG2_PACK;
	}
	if (iPosition == 1) {
		pmt->majortype = MEDIATYPE_MPEG2_PES;
	}

	return S_OK;
}

//
// CDeCSSInputPin
//

CDeCSSInputPin::CDeCSSInputPin(TCHAR* pObjectName, CTransformFilter* pFilter, HRESULT* phr, LPCWSTR pName)
	: CTransformInputPin(pObjectName, pFilter, phr, pName)
{
	m_varient = -1;
	memset(m_Challenge, 0, sizeof(m_Challenge));
	memset(m_KeyCheck, 0, sizeof(m_KeyCheck));
	memset(m_DiscKey, 0, sizeof(m_DiscKey));
	memset(m_TitleKey, 0, sizeof(m_TitleKey));
}

STDMETHODIMP CDeCSSInputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		QI(IKsPropertySet)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IMemInputPin

STDMETHODIMP CDeCSSInputPin::Receive(IMediaSample* pSample)
{
	long len = pSample->GetActualDataLength();

	BYTE* p = nullptr;
	if(SUCCEEDED(pSample->GetPointer(&p)) && len > 0) {
		if(m_mt.majortype == MEDIATYPE_DVD_ENCRYPTED_PACK && len == 2048 && (p[0x14]&0x30)) {
			CSSdescramble(p, m_TitleKey);
			p[0x14] &= ~0x30;

			if(CComQIPtr<IMediaSample2> pMS2 = pSample) {
				AM_SAMPLE2_PROPERTIES props;
				memset(&props, 0, sizeof(props));
				if(SUCCEEDED(pMS2->GetProperties(sizeof(props), (BYTE*)&props))
						&& (props.dwTypeSpecificFlags & AM_UseNewCSSKey)) {
					props.dwTypeSpecificFlags &= ~AM_UseNewCSSKey;
					pMS2->SetProperties(sizeof(props), (BYTE*)&props);
				}
			}
		}
	}

	HRESULT hr = Transform(pSample);

	return hr == S_OK ? __super::Receive(pSample) :
		   hr == S_FALSE ? S_OK : hr;
}

void CDeCSSInputPin::StripPacket(BYTE*& p, long& len)
{
	GUID majortype = m_mt.majortype;

	if(majortype == MEDIATYPE_MPEG2_PACK || majortype == MEDIATYPE_DVD_ENCRYPTED_PACK)
		if(len > 0 && GETU32(p) == 0xba010000) { // MEDIATYPE_*_PACK
			len -= 14;
			p += 14;
			if(int stuffing = (p[-1]&7)) {
				len -= stuffing;
				p += stuffing;
			}
			majortype = MEDIATYPE_MPEG2_PES;
		}

	if(majortype == MEDIATYPE_MPEG2_PES)
		if(len > 0 && GETU32(p) == 0xbb010000) {
			len -= 4;
			p += 4;
			int hdrlen = ((p[0]<<8)|p[1]) + 2;
			len -= hdrlen;
			p += hdrlen;
		}

	if(majortype == MEDIATYPE_MPEG2_PES)
		if(len > 0
				&& ((GETU32(p)&0xf0ffffff) == 0xe0010000
					|| (GETU32(p)&0xe0ffffff) == 0xc0010000
					|| (GETU32(p)&0xbdffffff) == 0xbd010000)) { // PES
			bool ps1 = (GETU32(p)&0xbdffffff) == 0xbd010000;

			len -= 4;
			p += 4;
			long expected = ((p[0]<<8)|p[1]);
			len -= 2;
			p += 2;
			BYTE* p0 = p;

			for(int i = 0; i < 16 && *p == 0xff; i++, len--, p++) {
				;
			}

			if((*p&0xc0) == 0x80) { // mpeg2
				len -= 2;
				p += 2;
				len -= *p+1;
				p += *p+1;
			} else { // mpeg1
				if((*p&0xc0) == 0x40) {
					len -= 2;
					p += 2;
				}

				if((*p&0x30) == 0x30 || (*p&0x30) == 0x20) {
					bool pts = !!(*p&0x20), dts = !!(*p&0x10);
					if(pts) {
						len -= 5;
					}
					p += 5;
					if(dts) {
						ASSERT((*p&0xf0) == 0x10);
						len -= 5;
						p += 5;
					}
				} else {
					len--;
					p++;
				}
			}

			if(ps1) {
				len--;
				p++;
				if(m_mt.subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO) {
					len -= 6;
					p += 6;
				} else if(m_mt.subtype == MEDIASUBTYPE_DOLBY_AC3 || m_mt.subtype == MEDIASUBTYPE_WAVE_DOLBY_AC3
						  || m_mt.subtype == MEDIASUBTYPE_DTS || m_mt.subtype == MEDIASUBTYPE_DTS2) {
					len -= 3;
					p += 3;
				}
			}

			if(expected > 0) {
				expected -= (int)(p - p0);
				len = std::min(expected, len);
			}
		}

	if(len < 0) {
		ASSERT(0);
		len = 0;
	}
}

// IKsPropertySet

STDMETHODIMP CDeCSSInputPin::Set(REFGUID PropSet, ULONG Id, LPVOID pInstanceData, ULONG InstanceLength, LPVOID pPropertyData, ULONG DataLength)
{
	if(PropSet != AM_KSPROPSETID_CopyProt) {
		return E_NOTIMPL;
	}

	switch(Id) {
		case AM_PROPERTY_COPY_MACROVISION:
			break;
		case AM_PROPERTY_DVDCOPY_CHLG_KEY: { // 3. auth: receive drive nonce word, also store and encrypt the buskey made up of the two nonce words
			AM_DVDCOPY_CHLGKEY* pChlgKey = (AM_DVDCOPY_CHLGKEY*)pPropertyData;
			for(int i = 0; i < 10; i++) {
				m_Challenge[i] = pChlgKey->ChlgKey[9-i];
			}

			CSSkey2(m_varient, m_Challenge, &m_Key[5]);

			CSSbuskey(m_varient, m_Key, m_KeyCheck);
		}
		break;
		case AM_PROPERTY_DVDCOPY_DISC_KEY: { // 5. receive the disckey
			AM_DVDCOPY_DISCKEY* pDiscKey = (AM_DVDCOPY_DISCKEY*)pPropertyData; // pDiscKey->DiscKey holds the disckey encrypted with itself and the 408 disckeys encrypted with the playerkeys

			bool fSuccess = false;

			for(int j = 0; j < g_nPlayerKeys; j++) {
				for(int k = 1; k < 409; k++) {
					BYTE DiscKey[6];
					for(int i = 0; i < 5; i++) {
						DiscKey[i] = pDiscKey->DiscKey[k*5+i] ^ m_KeyCheck[4-i];
					}
					DiscKey[5] = 0;

					CSSdisckey(DiscKey, g_PlayerKeys[j]);

					BYTE Hash[6];
					for(int i = 0; i < 5; i++) {
						Hash[i] = pDiscKey->DiscKey[i] ^ m_KeyCheck[4-i];
					}
					Hash[5] = 0;

					CSSdisckey(Hash, DiscKey);

					if(!memcmp(Hash, DiscKey, 6)) {
						memcpy(m_DiscKey, DiscKey, 6);
						j = g_nPlayerKeys;
						fSuccess = true;
						break;
					}
				}
			}

			if(!fSuccess) {
				return E_FAIL;
			}
		}
		break;
		case AM_PROPERTY_DVDCOPY_DVD_KEY1: { // 2. auth: receive our drive-encrypted nonce word and decrypt it for verification
			AM_DVDCOPY_BUSKEY* pKey1 = (AM_DVDCOPY_BUSKEY*)pPropertyData;
			for(int i = 0; i < 5; i++) {
				m_Key[i] =  pKey1->BusKey[4-i];
			}

			m_varient = -1;

			for(int i = 31; i >= 0; i--) {
				CSSkey1(i, m_Challenge, m_KeyCheck);

				if(memcmp(m_KeyCheck, &m_Key[0], 5) == 0) {
					m_varient = i;
				}
			}
		}
		break;
		case AM_PROPERTY_DVDCOPY_REGION:
			break;
		case AM_PROPERTY_DVDCOPY_SET_COPY_STATE:
			break;
		case AM_PROPERTY_DVDCOPY_TITLE_KEY: { // 6. receive the title key and decrypt it with the disc key
			AM_DVDCOPY_TITLEKEY* pTitleKey = (AM_DVDCOPY_TITLEKEY*)pPropertyData;
			for(int i = 0; i < 5; i++) {
				m_TitleKey[i] = pTitleKey->TitleKey[i] ^ m_KeyCheck[4-i];
			}
			m_TitleKey[5] = 0;
			CSStitlekey(m_TitleKey, m_DiscKey);
		}
		break;
		default:
			return E_PROP_ID_UNSUPPORTED;
	}

	return S_OK;
}

STDMETHODIMP CDeCSSInputPin::Get(REFGUID PropSet, ULONG Id, LPVOID pInstanceData, ULONG InstanceLength, LPVOID pPropertyData, ULONG DataLength, ULONG* pBytesReturned)
{
	if(PropSet != AM_KSPROPSETID_CopyProt) {
		return E_NOTIMPL;
	}

	switch(Id) {
		case AM_PROPERTY_DVDCOPY_CHLG_KEY: { // 1. auth: send our nonce word
			AM_DVDCOPY_CHLGKEY* pChlgKey = (AM_DVDCOPY_CHLGKEY*)pPropertyData;
			for(int i = 0; i < 10; i++) {
				pChlgKey->ChlgKey[i] = 9 - (m_Challenge[i] = i);
			}
			*pBytesReturned = sizeof(AM_DVDCOPY_CHLGKEY);
		}
		break;
		case AM_PROPERTY_DVDCOPY_DEC_KEY2: { // 4. auth: send back the encrypted drive nonce word to finish the authentication
			AM_DVDCOPY_BUSKEY* pKey2 = (AM_DVDCOPY_BUSKEY*)pPropertyData;
			for(int i = 0; i < 5; i++) {
				pKey2->BusKey[4-i] = m_Key[5+i];
			}
			*pBytesReturned = sizeof(AM_DVDCOPY_BUSKEY);
		}
		break;
		case AM_PROPERTY_DVDCOPY_REGION: {
			DVD_REGION* pRegion = (DVD_REGION*)pPropertyData;
			pRegion->RegionData = 0;
			pRegion->SystemRegion = 0;
			*pBytesReturned = sizeof(DVD_REGION);
		}
		break;
		case AM_PROPERTY_DVDCOPY_SET_COPY_STATE: {
			AM_DVDCOPY_SET_COPY_STATE* pState = (AM_DVDCOPY_SET_COPY_STATE*)pPropertyData;
			pState->DVDCopyState = AM_DVDCOPYSTATE_AUTHENTICATION_REQUIRED;
			*pBytesReturned = sizeof(AM_DVDCOPY_SET_COPY_STATE);
		}
		break;
		default:
			return E_PROP_ID_UNSUPPORTED;
	}

	return S_OK;
}

STDMETHODIMP CDeCSSInputPin::QuerySupported(REFGUID PropSet, ULONG Id, ULONG* pTypeSupport)
{
	if(PropSet != AM_KSPROPSETID_CopyProt) {
		return E_NOTIMPL;
	}

	switch(Id) {
		case AM_PROPERTY_COPY_MACROVISION:
			*pTypeSupport = KSPROPERTY_SUPPORT_SET;
			break;
		case AM_PROPERTY_DVDCOPY_CHLG_KEY:
			*pTypeSupport = KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET;
			break;
		case AM_PROPERTY_DVDCOPY_DEC_KEY2:
			*pTypeSupport = KSPROPERTY_SUPPORT_GET;
			break;
		case AM_PROPERTY_DVDCOPY_DISC_KEY:
			*pTypeSupport = KSPROPERTY_SUPPORT_SET;
			break;
		case AM_PROPERTY_DVDCOPY_DVD_KEY1:
			*pTypeSupport = KSPROPERTY_SUPPORT_SET;
			break;
		case AM_PROPERTY_DVDCOPY_REGION:
			*pTypeSupport = KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET;
			break;
		case AM_PROPERTY_DVDCOPY_SET_COPY_STATE:
			*pTypeSupport = KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET;
			break;
		case AM_PROPERTY_DVDCOPY_TITLE_KEY:
			*pTypeSupport = KSPROPERTY_SUPPORT_SET;
			break;
		default:
			return E_PROP_ID_UNSUPPORTED;
	}

	return S_OK;
}

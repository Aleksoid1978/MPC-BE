/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2014 see Authors.txt
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

#define DeCSSFilterName L"MPC DeCSSFilter"

class __declspec(uuid("7B3BD419-FE03-4820-BE94-A22A4F844895"))
	CDeCSSFilter : public CTransformFilter
{
	friend class CKsPSInputPin;

public:
	CDeCSSFilter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CDeCSSFilter();

	HRESULT Transform(IMediaSample* pIn, IMediaSample* pOut);
	HRESULT CheckInputType(const CMediaType* mtIn);
	HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
	HRESULT DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties);
	HRESULT GetMediaType(int iPosition, CMediaType* pmt);
};

class CDeCSSInputPin : public CTransformInputPin, public IKsPropertySet
{
	int m_varient;
	BYTE m_Challenge[10], m_KeyCheck[5], m_Key[10];
	BYTE m_DiscKey[6], m_TitleKey[6];

protected:
	// return S_FALSE here if you don't want the base class
	// to call CTransformFilter::Receive with this sample
	virtual HRESULT Transform(IMediaSample* pSample) {
		return S_OK;
	}

public:
	CDeCSSInputPin(TCHAR* pObjectName, CTransformFilter* pFilter, HRESULT* phr, LPCWSTR pName);

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	void StripPacket(BYTE*& p, long& len);

	// IMemInputPin
	STDMETHODIMP Receive(IMediaSample* pSample);

	// IKsPropertySet
	STDMETHODIMP Set(REFGUID PropSet, ULONG Id, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength);
	STDMETHODIMP Get(REFGUID PropSet, ULONG Id, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength, ULONG* pBytesReturned);
	STDMETHODIMP QuerySupported(REFGUID PropSet, ULONG Id, ULONG* pTypeSupport);
};

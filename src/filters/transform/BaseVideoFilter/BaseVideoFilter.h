/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2026 see Authors.txt
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

#include <d3d9.h>
#include <dxva2api.h>
#include "VideoFormats.h"

enum DECODER_MODE {
	MODE_NONE = 0,
	MODE_SOFTWARE,
	MODE_DXVA2,
	MODE_D3D11
};

class CBaseVideoFilter : public CTransformFilter
{
protected:
	int m_win = 0;
	int m_hin = 0;
	int m_arxin = 0;
	int m_aryin = 0;

	int m_wout = 0;
	int m_hout = 0;
	int m_arxout = 0;
	int m_aryout = 0;

	int m_arx = 0;
	int m_ary = 0;

	DECODER_MODE m_nDecoderMode = MODE_NONE;
	DXVA2_ExtendedFormat m_dxvaExtFormat = {};

	BOOL m_bMVC_Output_TopBottom = FALSE;
	bool m_bSendMediaType = false;

	long m_cBuffers;

	HRESULT Receive(IMediaSample* pIn);
	HRESULT GetDeliveryBuffer(int w, int h, IMediaSample** ppOut, REFERENCE_TIME AvgTimePerFrame = 0, DXVA2_ExtendedFormat* dxvaExtFormat = nullptr);

	virtual void GetOutputSize(int& w, int& h, int& arx, int& ary) {}
	virtual HRESULT Transform(IMediaSample* pIn) PURE;
	virtual bool IsVideoInterlaced() { return true; }
	virtual void GetOutputFormats(int& nNumber, VFormatDesc** ppFormats) PURE;

public:
	CBaseVideoFilter(LPCWSTR pName, LPUNKNOWN lpunk, HRESULT* phr, REFCLSID clsid, long cBuffers = 1);
	virtual ~CBaseVideoFilter();

	HRESULT ReconnectOutput(int width, int height, bool bForce = false, REFERENCE_TIME AvgTimePerFrame = 0, DXVA2_ExtendedFormat* dxvaExtFormat = nullptr);
	int GetPinCount();
	CBasePin* GetPin(int n);

	HRESULT CheckInputType(const CMediaType* mtIn) override;
	HRESULT CheckOutputType(const CMediaType& mtOut);
	HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut) override;
	HRESULT DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties) override;
	virtual HRESULT GetMediaType(int iPosition, CMediaType* pmt) override;
	HRESULT SetMediaType(PIN_DIRECTION dir, const CMediaType* pmt) override;

	void SetAspect(CSize aspect);
};

class CBaseVideoInputAllocator : public CMemAllocator
{
	CMediaType m_mt;

public:
	CBaseVideoInputAllocator(HRESULT* phr);
	void SetMediaType(const CMediaType& mt);
	STDMETHODIMP GetBuffer(IMediaSample** ppBuffer, REFERENCE_TIME* pStartTime, REFERENCE_TIME* pEndTime, DWORD dwFlags);
};

class CBaseVideoInputPin : public CTransformInputPin
{
	CBaseVideoInputAllocator* m_pAllocator;

public:
	CBaseVideoInputPin(LPCWSTR pObjectName, CBaseVideoFilter* pFilter, HRESULT* phr, LPCWSTR pName);
	~CBaseVideoInputPin();

	STDMETHODIMP GetAllocator(IMemAllocator** ppAllocator);
	STDMETHODIMP ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt);
};

class CBaseVideoOutputPin : public CTransformOutputPin
{
public:
	CBaseVideoOutputPin(LPCWSTR pObjectName, CBaseVideoFilter* pFilter, HRESULT* phr, LPCWSTR pName);

	HRESULT CheckMediaType(const CMediaType* mtOut);
};

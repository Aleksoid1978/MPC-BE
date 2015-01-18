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

bool BitBltFromP016ToP016(size_t w, size_t h, BYTE* dstY, BYTE* dstUV, int dstPitch, BYTE* srcY, BYTE* srcUV, int srcPitch);

struct VIDEO_OUTPUT_FORMATS {
	const GUID*		subtype;
	WORD			biPlanes;
	WORD			biBitCount;
	DWORD			biCompression;

	bool operator == (const struct VIDEO_OUTPUT_FORMATS& fmt) const {
		return (subtype == fmt.subtype
				&& biPlanes == fmt.biPlanes
				&& biBitCount == fmt.biBitCount
				&& biCompression == fmt.biCompression);
	}
};

enum DECODER_MODE {
	MODE_SOFTWARE,
	MODE_DXVA1,
	MODE_DXVA2
};

class CBaseVideoFilter : public CTransformFilter
{
private:
	HRESULT Receive(IMediaSample* pIn);

	// these are private for a reason, don't bother them
	int m_win, m_hin, m_arxin, m_aryin;
	int m_wout, m_hout, m_arxout, m_aryout;
	bool m_bSendMediaType;

	long m_cBuffers;

protected:
	CCritSec m_csReceive;

	int m_w, m_h, m_arx, m_ary;

	DECODER_MODE m_nDecoderMode;

	HRESULT GetDeliveryBuffer(int w, int h, IMediaSample** ppOut, REFERENCE_TIME AvgTimePerFrame = 0);
	HRESULT CopyBuffer(BYTE* pOut, BYTE* pIn, int w, int h, int pitchIn, const GUID& subtype, bool fInterlaced = false);
	HRESULT CopyBuffer(BYTE* pOut, BYTE** ppIn, int w, int h, int pitchIn, const GUID& subtype, bool fInterlaced = false);

	virtual void GetOutputSize(int& w, int& h, int& arx, int& ary, int &RealWidth, int &RealHeight, int& vsfilter) {}
	virtual HRESULT Transform(IMediaSample* pIn) PURE;
	virtual bool IsVideoInterlaced() { return true; }
	virtual void GetOutputFormats(int& nNumber, VIDEO_OUTPUT_FORMATS** ppFormats) PURE;

public:
	CBaseVideoFilter(TCHAR* pName, LPUNKNOWN lpunk, HRESULT* phr, REFCLSID clsid, long cBuffers = 1);
	virtual ~CBaseVideoFilter();

	HRESULT ReconnectOutput(int w, int h, bool bSendSample = true, bool bForce = false, REFERENCE_TIME AvgTimePerFrame = 0, int RealWidth = -1, int RealHeight = -1);
	int GetPinCount();
	CBasePin* GetPin(int n);

	HRESULT CheckInputType(const CMediaType* mtIn);
	HRESULT CheckOutputType(const CMediaType& mtOut);
	HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
	HRESULT DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties);
	HRESULT GetMediaType(int iPosition, CMediaType* pMediaType);
	HRESULT SetMediaType(PIN_DIRECTION dir, const CMediaType* pmt);

	void SetAspect(CSize aspect);

	bool GetSendMediaType()				{ return m_bSendMediaType;  }
	void SetSendMediaType(bool value)	{ m_bSendMediaType = value; }
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
	CBaseVideoInputPin(TCHAR* pObjectName, CBaseVideoFilter* pFilter, HRESULT* phr, LPCWSTR pName);
	~CBaseVideoInputPin();

	STDMETHODIMP GetAllocator(IMemAllocator** ppAllocator);
	STDMETHODIMP ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt);
};

class CBaseVideoOutputPin : public CTransformOutputPin
{
public:
	CBaseVideoOutputPin(TCHAR* pObjectName, CBaseVideoFilter* pFilter, HRESULT* phr, LPCWSTR pName);

	HRESULT CheckMediaType(const CMediaType* mtOut);
};

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

#include "../DeCSSFilter/DeCSSFilter.h"
#include "../BaseVideoFilter/BaseVideoFilter.h"
#include "IMpcDvdVideoDec.h"
#include "SettingsWnd.h"

#define DvdVideoDecoderName L"MPC DVD Video Decoder"

class CSubpicInputPin;
class CClosedCaptionOutputPin;
class CMpeg2Dec;

class __declspec(uuid("39F81046-93AC-486D-882F-4ADD1CB068C6"))
	CMpeg2DecFilter
	: public CBaseVideoFilter
	, public IMpcDvdVideoDec
	, public ISpecifyPropertyPages2
{
	CSubpicInputPin* m_pSubpicInput;
	CClosedCaptionOutputPin* m_pClosedCaptionOutput;

	std::unique_ptr<CMpeg2Dec> m_dec;

	REFERENCE_TIME m_AvgTimePerFrame;
	bool m_fWaitForKeyFrame = true;

	struct framebuf {
		int w = 0;
		int h = 0;
		int pitch = 0;
		BYTE* buf_base = nullptr;;
		BYTE* buf[6] = {};
		REFERENCE_TIME rtStart = 0;
		REFERENCE_TIME rtStop = 0;
		uint32_t flags = 0;
		ditype di = DIAuto;
		~framebuf() {
			Free();
		}
		void Alloc(int _w, int _h, int _pitch) {
			Free();
			w = _w;
			h = _h;
			pitch = _pitch;
			const unsigned size = pitch*h;
			buf_base = (BYTE*)_aligned_malloc(size * 3 + 6 * 32, 32);
			BYTE* p = buf_base;
			buf[0] = p;
			p += (size + 31) & ~31;
			buf[3] = p;
			p += (size + 31) & ~31;
			buf[1] = p;
			p += (size/4 + 31) & ~31;
			buf[4] = p;
			p += (size/4 + 31) & ~31;
			buf[2] = p;
			p += (size/4 + 31) & ~31;
			buf[5] = p;
			p += (size/4 + 31) & ~31;
		}
		void Free() {
			if (buf_base) {
				_aligned_free(buf_base);
			}
			buf_base = nullptr;
		}
	} m_fb;

	bool m_fFilm;
	void SetDeinterlaceMethod();
	void SetTypeSpecificFlags(IMediaSample* pMS);

	AM_SimpleRateChange m_rate = { 0, 10000 };

	LONGLONG m_llLastDecodeTime = 0;

protected:
	void InputTypeChanged();

	virtual void GetOutputSize(int& w, int& h, int& arx, int& ary) override;
	virtual HRESULT Transform(IMediaSample* pIn) override;
	virtual bool IsVideoInterlaced() override { return IsInterlacedEnabled(); }
	virtual void GetOutputFormats(int& nNumber, VFormatDesc** ppFormats) override;

public:
	CMpeg2DecFilter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CMpeg2DecFilter();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	HRESULT Deliver();
	HRESULT DeliverToRenderer();

	int GetPinCount();
	CBasePin* GetPin(int n);

	HRESULT EndOfStream();
	HRESULT BeginFlush();
	HRESULT EndFlush();
	HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

	HRESULT CheckConnect(PIN_DIRECTION dir, IPin* pPin);
	HRESULT CheckInputType(const CMediaType* mtIn);
	HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);

	HRESULT StartStreaming();
	HRESULT StopStreaming();

	bool m_fDropFrames;
	HRESULT AlterQuality(Quality q);

	bool IsGraphRunning() const;
	bool IsNeedDeliverToRenderer() const;
	bool IsInterlaced() const;

protected:
	CCritSec m_csProps;
	ditype m_ditype = DIAuto;
	int m_bright    = 0;
	int m_cont      = 100;
	int m_hue       = 0;
	int m_sat       = 100;
	BYTE m_YTbl[256], m_UTbl[256*256], m_VTbl[256*256];
	bool m_fForcedSubs       = true;
	bool m_fInterlaced       = true;

	void ApplyBrContHueSat(BYTE* srcy, BYTE* srcu, BYTE* srcv, int w, int h, int pitch);

public:

	// ISpecifyPropertyPages2

	STDMETHODIMP GetPages(CAUUID* pPages);
	STDMETHODIMP CreatePage(const GUID& guid, IPropertyPage** ppPage);

	// IMpcDvdVideoDec

	STDMETHODIMP SetDeinterlaceMethod(ditype di);
	STDMETHODIMP_(ditype) GetDeinterlaceMethod();

	STDMETHODIMP SetBrightness(int bright);
	STDMETHODIMP SetContrast(int cont);
	STDMETHODIMP SetHue(int hue);
	STDMETHODIMP SetSaturation(int sat);
	STDMETHODIMP_(int) GetBrightness();
	STDMETHODIMP_(int) GetContrast();
	STDMETHODIMP_(int) GetHue();
	STDMETHODIMP_(int) GetSaturation();

	STDMETHODIMP EnableForcedSubtitles(bool fEnable);
	STDMETHODIMP_(bool) IsForcedSubtitlesEnabled();
	STDMETHODIMP EnableInterlaced(bool fEnable);
	STDMETHODIMP_(bool) IsInterlacedEnabled();

	STDMETHODIMP Apply();

private:
	enum {
		CNTRL_EXIT,
		CNTRL_REDRAW
	};
	HRESULT ControlCmd(DWORD cmd) {
		return m_ControlThread->CallWorker(cmd);
	}

	friend class CMpeg2DecControlThread;
	friend class CSubpicInputPin;
	CAMThread *m_ControlThread = nullptr;
};

class CMpeg2DecInputPin : public CDeCSSInputPin
{
	LONG m_CorrectTS = 0;

public:
	CMpeg2DecInputPin(CTransformFilter* pFilter, HRESULT* phr, LPCWSTR pName);

	CCritSec m_csRateLock;
	AM_SimpleRateChange m_ratechange = { _I64_MAX, 10000 };

	// IKsPropertySet
	STDMETHODIMP Set(REFGUID PropSet, ULONG Id, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength);
	STDMETHODIMP Get(REFGUID PropSet, ULONG Id, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength, ULONG* pBytesReturned);
	STDMETHODIMP QuerySupported(REFGUID PropSet, ULONG Id, ULONG* pTypeSupport);
};

class CSubpicInputPin : public CMpeg2DecInputPin
{
	CCritSec m_csReceive;

	AM_PROPERTY_COMPOSIT_ON m_spon = TRUE;
	AM_DVD_YUV m_sppal[16];
	bool m_fsppal = false;
	std::unique_ptr<AM_PROPERTY_SPHLI> m_sphli; // temp

	class spu : public std::vector<BYTE>
	{
	public:
		bool m_fForced = false;
		REFERENCE_TIME m_rtStart = 0;
		REFERENCE_TIME m_rtStop = 0;
		uint32_t m_offset[2] = {};
		AM_PROPERTY_SPHLI m_sphli = {}; // parsed
		std::unique_ptr<AM_PROPERTY_SPHLI> m_psphli; // for the menu (optional)
		virtual ~spu() {}
		virtual bool Parse() PURE;
		virtual void Render(REFERENCE_TIME rt, BYTE** p, int w, int h, AM_DVD_YUV* sppal, bool fsppal) PURE;
	};

	class dvdspu : public spu
	{
	public:
		struct offset_t {
			REFERENCE_TIME rt;
			AM_PROPERTY_SPHLI sphli;
		};
		std::list<offset_t> m_offsets;
		bool Parse() override;
		void Render(REFERENCE_TIME rt, BYTE** p, int w, int h, AM_DVD_YUV* sppal, bool fsppal) override;
	};

	class cvdspu : public spu
	{
	public:
		AM_DVD_YUV m_sppal[2][4];
		cvdspu() {
			memset(m_sppal, 0, sizeof(m_sppal));
		}
		bool Parse() override;
		void Render(REFERENCE_TIME rt, BYTE** p, int w, int h, AM_DVD_YUV* sppal, bool fsppal) override;
	};

	class svcdspu : public spu
	{
	public:
		AM_DVD_YUV m_sppal[4];
		svcdspu() {
			memset(m_sppal, 0, sizeof(m_sppal));
		}
		bool Parse() override;
		void Render(REFERENCE_TIME rt, BYTE** p, int w, int h, AM_DVD_YUV* sppal, bool fsppal) override;
	};

	std::list<std::unique_ptr<spu>> m_sps;

protected:
	HRESULT Transform(IMediaSample* pSample) override;

public:
	CSubpicInputPin(CTransformFilter* pFilter, HRESULT* phr);

	bool HasAnythingToRender(REFERENCE_TIME rt);
	void RenderSubpics(REFERENCE_TIME rt, BYTE** p, int w, int h);

	HRESULT CheckMediaType(const CMediaType* mtIn);
	HRESULT SetMediaType(const CMediaType* mtIn);

	// we shouldn't pass these to the filter from this pin
	STDMETHODIMP EndOfStream() {
		return S_OK;
	}
	STDMETHODIMP BeginFlush() {
		return S_OK;
	}
	STDMETHODIMP EndFlush();
	STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate) {
		return S_OK;
	}

	// IKsPropertySet
	STDMETHODIMP Set(REFGUID PropSet, ULONG Id, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength);
	STDMETHODIMP QuerySupported(REFGUID PropSet, ULONG Id, ULONG* pTypeSupport);
};

class CClosedCaptionOutputPin : public CBaseOutputPin
{
public:
	CClosedCaptionOutputPin(CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);

	HRESULT CheckMediaType(const CMediaType* mtOut);
	HRESULT GetMediaType(int iPosition, CMediaType* pmt);
	HRESULT DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties);

	CMediaType& CurrentMediaType() {
		return m_mt;
	}

	HRESULT Deliver(const void* ptr, int len);
};

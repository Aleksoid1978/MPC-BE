/*
* (C) 2012-2017 see Authors.txt
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

#include "../../parser/BaseSplitter/BaseSplitter.h"

#include "mpc_file.h"

#define MusePackSplitterName	L"MPC MusePack Source"

// {47A759C8-CCD7-471A-81D3-A92870431979}
static const GUID CLSID_MusePackSplitter =
	{ 0x47A759C8, 0xCCD7, 0x471A, { 0x81, 0xD3, 0xA9, 0x28, 0x70, 0x43, 0x19, 0x79 } };

class CMusePackSplitter;

//-----------------------------------------------------------------------------
//
//	CMusePackInputPin class
//
//-----------------------------------------------------------------------------
class CMusePackInputPin : public CBaseInputPin
{
public:
	// parent splitter
	CMusePackSplitter	*demux;
	IAsyncReader		*reader;

public:
	CMusePackInputPin(TCHAR *pObjectName, CMusePackSplitter *pDemux, HRESULT *phr, LPCWSTR pName);
	virtual ~CMusePackInputPin();

	// Get hold of IAsyncReader interface
	HRESULT CompleteConnect(IPin *pReceivePin);
	HRESULT CheckConnect(IPin *pPin);
	HRESULT BreakConnect();
	HRESULT CheckMediaType(const CMediaType *pmt);
	HRESULT SetMediaType(const CMediaType *pmt);

	// IMemInputPIn
	STDMETHODIMP Receive(IMediaSample *pSample);
	STDMETHODIMP EndOfStream(void);
	STDMETHODIMP BeginFlush();
	STDMETHODIMP EndFlush();
	STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double rate);

	HRESULT Inactive();
public:
	// Helpers
	CMediaType &CurrentMediaType() { return m_mt; }
	IAsyncReader *Reader() { return reader; }
};

//-----------------------------------------------------------------------------
//
//	DataPacketMPC class
//
//-----------------------------------------------------------------------------
class DataPacketMPC
{
public:
	int		type;

	enum {
		PACKET_TYPE_EOS	= 0,
		PACKET_TYPE_DATA = 1,
		PACKET_TYPE_NEW_SEGMENT = 2
	};

	REFERENCE_TIME	rtStart, rtStop;
	double			rate;
	bool			has_time;
	BOOL			sync_point;
	BOOL			discontinuity;
	BYTE			*buf;
	int				size;

public:
	DataPacketMPC();
	virtual ~DataPacketMPC();
};

//-----------------------------------------------------------------------------
//
//	CMusePackOutputPin class
//
//-----------------------------------------------------------------------------
class CMusePackOutputPin :
	public CBaseOutputPin,
	public IMediaSeeking,
	public CAMThread
{
public:
	enum {CMD_EXIT, CMD_STOP, CMD_RUN};

	// parser
	CMusePackSplitter		*demux;
	int						stream_index;
	CAtlArray<CMediaType>	mt_types;

	// buffer queue
	int						buffers;
	CList<DataPacketMPC*>	queue;
	CCritSec				lock_queue;
	CAMEvent				ev_can_read;
	CAMEvent				ev_can_write;
	CAMEvent				ev_abort;
	int						buffer_time_ms;

	// time stamps
	REFERENCE_TIME			rtStart;
	REFERENCE_TIME			rtStop;
	double					rate;
	bool					active;
	bool					discontinuity;
	bool					eos_delivered;

public:
	CMusePackOutputPin(TCHAR *pObjectName, CMusePackSplitter *pDemux, HRESULT *phr, LPCWSTR pName, int iBuffers);
	virtual ~CMusePackOutputPin();

	// IUNKNOWN
	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

	// MediaType stuff
	HRESULT CheckMediaType(const CMediaType *pmt);
	HRESULT SetMediaType(const CMediaType *pmt);
	HRESULT GetMediaType(int iPosition, CMediaType *pmt);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp);
	HRESULT BreakConnect();
	HRESULT CompleteConnect(IPin *pReceivePin);

	// Activation/Deactivation
	HRESULT Active();
	HRESULT Inactive();
	HRESULT DoNewSegment(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double dRate);

	STDMETHODIMP Notify(IBaseFilter *pSender, Quality q);

	// packet delivery mechanism
	HRESULT DeliverPacket(CMPCPacket &packet);
	HRESULT DeliverDataPacketMPC(DataPacketMPC &packet);
	HRESULT DoEndOfStream();

	// delivery thread
	virtual DWORD ThreadProc();
	void FlushQueue();
	int GetDataPacketMPC(DataPacketMPC **packet);

	// IMediaSeeking
	STDMETHODIMP GetCapabilities(DWORD* pCapabilities);
	STDMETHODIMP CheckCapabilities(DWORD* pCapabilities);
	STDMETHODIMP IsFormatSupported(const GUID* pFormat);
	STDMETHODIMP QueryPreferredFormat(GUID* pFormat);
	STDMETHODIMP GetTimeFormat(GUID* pFormat);
	STDMETHODIMP IsUsingTimeFormat(const GUID* pFormat);
	STDMETHODIMP SetTimeFormat(const GUID* pFormat);
	STDMETHODIMP GetDuration(LONGLONG* pDuration);
	STDMETHODIMP GetStopPosition(LONGLONG* pStop);
	STDMETHODIMP GetCurrentPosition(LONGLONG* pCurrent);
	STDMETHODIMP ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat);
	STDMETHODIMP SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags);
	STDMETHODIMP GetPositions(LONGLONG* pCurrent, LONGLONG* pStop);
	STDMETHODIMP GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest);
	STDMETHODIMP SetRate(double dRate);
	STDMETHODIMP GetRate(double* pdRate);
	STDMETHODIMP GetPreroll(LONGLONG* pllPreroll);

public:
	CMediaType &CurrentMediaType() { return m_mt; }
	IMemAllocator *Allocator() { return m_pAllocator; }
	REFERENCE_TIME CurrentStart() { return rtStart; }
	REFERENCE_TIME CurrentStop() { return rtStop; }
	double CurrentRate() { return rate; }
	__int64 GetBufferTime_MS();
};

//-----------------------------------------------------------------------------
//
//	CMusePackReader class
//
//	Todo: caching...
//
//-----------------------------------------------------------------------------

class CMusePackReader
{
protected:
	IAsyncReader	*reader;
	__int64			position;

public:
	CMusePackReader(IAsyncReader *rd);
	virtual ~CMusePackReader();

	virtual int GetSize(__int64 *avail, __int64 *total);
	virtual int GetPosition(__int64 *pos, __int64 *avail);
	virtual int Seek(__int64 pos);
	virtual int Read(void *buf, int size);
	virtual int ReadSwapped(void *buf, int size);

	void BeginFlush() { if (reader) reader->BeginFlush(); }
	void EndFlush() { if (reader) reader->EndFlush(); }

	// reading syntax elements - in network byte order
	int GetMagick(uint32_t &elm);
	int GetKey(uint16_t &key);
	int GetSizeElm(int64_t &size, int32_t &size_len);
	bool KeyValid(uint16_t key);
};

//-----------------------------------------------------------------------------
//
//	CMusePackSplitter class
//
//-----------------------------------------------------------------------------
class __declspec(uuid("47A759C8-CCD7-471A-81D3-A92870431979"))
    CMusePackSplitter
	: public CBaseFilter
	, public CAMThread
	, public IMediaSeeking
	, public IDSMResourceBagImpl
	, public IDSMChapterBagImpl
	, public IDSMPropertyBagImpl
	, public IAMMediaContent
{
public:
	enum {CMD_EXIT, CMD_STOP, CMD_RUN};

	CCritSec						lock_filter;
	CMusePackInputPin				*input;
	CMusePackOutputPin*				output;
	CAtlArray<CMusePackOutputPin*>	retired;
	CMusePackReader					*reader;
	CMPCFile						*file;

	HWND						wnd_prop;
	CAMEvent					ev_abort;

	// times
	REFERENCE_TIME				rtCurrent;
	REFERENCE_TIME				rtStop;
	double						rate;

public:
	// constructor
	CMusePackSplitter(LPUNKNOWN pUnk, HRESULT *phr);
	virtual ~CMusePackSplitter();
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

	// override this to publicise our interfaces
	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

	// CBaseFilter
	virtual int GetPinCount();
	virtual CBasePin *GetPin(int n);

	// Output pins
	HRESULT SetOutputPin(CMusePackOutputPin *pPin);
	virtual HRESULT RemoveOutputPin();
	CMusePackOutputPin *FindStream(int stream_no);

	// check that we can support this input type
	virtual HRESULT CheckInputType(const CMediaType* mtIn);
	virtual HRESULT CheckConnect(PIN_DIRECTION Dir, IPin *pPin);
	virtual HRESULT BreakConnect(PIN_DIRECTION Dir, CBasePin *pCaller);
	virtual HRESULT CompleteConnect(PIN_DIRECTION Dir, CBasePin *pCaller, IPin *pReceivePin);
	virtual HRESULT ConfigureMediaType(CMusePackOutputPin *pin);

	// Demuxing thread
	virtual DWORD ThreadProc();

	// activate / deactivate filter
	STDMETHODIMP Pause();
	STDMETHODIMP Stop();

	// IMediaSeeking
	STDMETHODIMP GetCapabilities(DWORD* pCapabilities);
	STDMETHODIMP CheckCapabilities(DWORD* pCapabilities);
	STDMETHODIMP IsFormatSupported(const GUID* pFormat);
	STDMETHODIMP QueryPreferredFormat(GUID* pFormat);
	STDMETHODIMP GetTimeFormat(GUID* pFormat);
	STDMETHODIMP IsUsingTimeFormat(const GUID* pFormat);
	STDMETHODIMP SetTimeFormat(const GUID* pFormat);
	STDMETHODIMP GetDuration(LONGLONG* pDuration);
	STDMETHODIMP GetStopPosition(LONGLONG* pStop);
	STDMETHODIMP GetCurrentPosition(LONGLONG* pCurrent);
	STDMETHODIMP ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat);
	STDMETHODIMP SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags);
	STDMETHODIMP GetPositions(LONGLONG* pCurrent, LONGLONG* pStop);
	STDMETHODIMP GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest);
	STDMETHODIMP SetRate(double dRate);
	STDMETHODIMP GetRate(double* pdRate);
	STDMETHODIMP GetPreroll(LONGLONG* pllPreroll);

	STDMETHODIMP SetPositionsInternal(int iD, LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags);

	virtual HRESULT DoNewSeek();

	// IDispatch
	STDMETHODIMP GetTypeInfoCount(UINT* pctinfo) {return E_NOTIMPL;}
	STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo) {return E_NOTIMPL;}
	STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid) {return E_NOTIMPL;}
	STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr) {return E_NOTIMPL;}

	// IAMMediaContent
	STDMETHODIMP get_AuthorName(BSTR* pbstrAuthorName);
	STDMETHODIMP get_Title(BSTR* pbstrTitle);
	STDMETHODIMP get_Rating(BSTR* pbstrRating);
	STDMETHODIMP get_Description(BSTR* pbstrDescription);
	STDMETHODIMP get_Copyright(BSTR* pbstrCopyright);
	STDMETHODIMP get_BaseURL(BSTR* pbstrBaseURL) {return E_NOTIMPL;}
	STDMETHODIMP get_LogoURL(BSTR* pbstrLogoURL) {return E_NOTIMPL;}
	STDMETHODIMP get_LogoIconURL(BSTR* pbstrLogoURL) {return E_NOTIMPL;}
	STDMETHODIMP get_WatermarkURL(BSTR* pbstrWatermarkURL) {return E_NOTIMPL;}
	STDMETHODIMP get_MoreInfoURL(BSTR* pbstrMoreInfoURL) {return E_NOTIMPL;}
	STDMETHODIMP get_MoreInfoBannerImage(BSTR* pbstrMoreInfoBannerImage) {return E_NOTIMPL;}
	STDMETHODIMP get_MoreInfoBannerURL(BSTR* pbstrMoreInfoBannerURL) {return E_NOTIMPL;}
	STDMETHODIMP get_MoreInfoText(BSTR* pbstrMoreInfoText) {return E_NOTIMPL;}
};

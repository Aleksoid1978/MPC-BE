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

#pragma once

#include <qnetwork.h>
#include <ExtLib/AsyncReader/asyncio.h>
#include <ExtLib/AsyncReader/asyncrdr.h>
#include "CDDAReaderSettingsWnd.h"

#define CCDDAReaderName L"MPC CDDA Reader"

struct ChunkHeader {
	UINT chunkID;
	long chunkSize;
};

#define RIFFID 'FFIR'
#define WAVEID 'EVAW'
struct RIFFChunk {
	ChunkHeader hdr;
	UINT WAVE;
};

#define FormatID ' tmf'
struct FormatChunk {
	ChunkHeader hdr;
	PCMWAVEFORMAT pcm;
};

#define DataID 'atad'
struct DataChunk {
	ChunkHeader hdr;
};

struct WAVEChunck {
	RIFFChunk riff;
	FormatChunk frm;
	DataChunk data;
};

class CCDDAStream : public CAsyncStream
{
private:
	CCritSec m_csLock;

	LONGLONG m_llPosition = 0;
	LONGLONG m_llLength   = 0;

	HANDLE m_hDrive       = INVALID_HANDLE_VALUE;
	UINT m_nStartSector   = 0;
	UINT m_nStopSector    = 0;

	WAVEChunck m_header;

	std::vector<BYTE> m_buff;
	size_t m_buff_pos     = 0;

public:
	CCDDAStream();
	virtual ~CCDDAStream();

	CStringW m_discTitle;
	CStringW m_trackTitle;
	CStringW m_discArtist;
	CStringW m_trackArtist;

	bool Load(const WCHAR* fnw, bool bReadTextInfo);

	// overrides
	HRESULT SetPointer(LONGLONG llPos);
	HRESULT Read(PBYTE pbBuffer, DWORD dwBytesToRead, BOOL bAlign, LPDWORD pdwBytesRead);
	LONGLONG Size(LONGLONG* pSizeAvailable);
	DWORD Alignment();
	void Lock();
	void Unlock();
};

class __declspec(uuid("54A35221-2C8D-4a31-A5DF-6D809847E393"))
	CCDDAReader
	: public CAsyncReader
	, public IFileSourceFilter
	, public IAMMediaContent
	, public ISpecifyPropertyPages2
	, public ICDDAReaderFilter
{
	CCritSec m_csProps;
	bool m_bReadTextInfo;

	CCDDAStream m_stream;
	CStringW m_fn;

public:
	CCDDAReader(IUnknown* pUnk, HRESULT* phr);
	~CCDDAReader();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// CBaseFilter
	STDMETHODIMP QueryFilterInfo(FILTER_INFO* pInfo);

	// IFileSourceFilter
	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt);

	// IAMMediaContent
	STDMETHODIMP GetTypeInfoCount(UINT* pctinfo);
	STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
	STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid);
	STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr);

	STDMETHODIMP get_AuthorName(BSTR* pbstrAuthorName);
	STDMETHODIMP get_Title(BSTR* pbstrTitle);
	STDMETHODIMP get_Rating(BSTR* pbstrRating);
	STDMETHODIMP get_Description(BSTR* pbstrDescription);
	STDMETHODIMP get_Copyright(BSTR* pbstrCopyright);
	STDMETHODIMP get_BaseURL(BSTR* pbstrBaseURL);
	STDMETHODIMP get_LogoURL(BSTR* pbstrLogoURL);
	STDMETHODIMP get_LogoIconURL(BSTR* pbstrLogoURL);
	STDMETHODIMP get_WatermarkURL(BSTR* pbstrWatermarkURL);
	STDMETHODIMP get_MoreInfoURL(BSTR* pbstrMoreInfoURL);
	STDMETHODIMP get_MoreInfoBannerImage(BSTR* pbstrMoreInfoBannerImage);
	STDMETHODIMP get_MoreInfoBannerURL(BSTR* pbstrMoreInfoBannerURL);
	STDMETHODIMP get_MoreInfoText(BSTR* pbstrMoreInfoText);

	// ISpecifyPropertyPages2
	STDMETHODIMP GetPages(CAUUID* pPages);
	STDMETHODIMP CreatePage(const GUID& guid, IPropertyPage** ppPage);

	// ICDDAReaderFilter
	STDMETHODIMP Apply();
	STDMETHODIMP SetReadTextInfo(BOOL nValue);
	STDMETHODIMP_(BOOL) GetReadTextInfo();
};

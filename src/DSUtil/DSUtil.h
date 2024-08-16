/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2024 see Authors.txt
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

#include <ExtLib/BaseClasses/streams.h>
#include "H264Nalu.h"
#include "MediaTypeEx.h"
#include "MFCHelper.h"
#include "text.h"
#include "Log.h"
#include <basestruct.h>
#include <mpc_defines.h>
#include "ds_defines.h"
#include "Utils.h"
#include "ISOLang.h"

extern int				CountPins(IBaseFilter* pBF, int& nIn, int& nOut, int& nInC, int& nOutC);
extern bool				IsSplitter(IBaseFilter* pBF, bool fCountConnectedOnly = false);
extern bool				IsMultiplexer(IBaseFilter* pBF, bool fCountConnectedOnly = false);
extern bool				IsStreamStart(IBaseFilter* pBF);
extern bool				IsStreamEnd(IBaseFilter* pBF);
extern bool				IsVideoDecoder(IBaseFilter* pBF, bool fCountConnectedOnly = false);
extern bool				IsVideoRenderer(IBaseFilter* pBF);
extern bool				IsVideoRenderer(const CLSID clsid);
extern bool				IsAudioWaveRenderer(IBaseFilter* pBF);

extern IBaseFilter*		GetUpStreamFilter(IBaseFilter* pBF, IPin* pInputPin = nullptr);
extern IPin*			GetUpStreamPin(IBaseFilter* pBF, IPin* pInputPin = nullptr);
extern IBaseFilter*		GetDownStreamFilter(IBaseFilter* pBF, IPin* pInputPin = nullptr);
extern IPin*			GetDownStreamPin(IBaseFilter* pBF, IPin* pInputPin = nullptr);
extern IPin*			GetFirstPin(IBaseFilter* pBF, PIN_DIRECTION dir = PINDIR_INPUT);
extern IPin*			GetFirstDisconnectedPin(IBaseFilter* pBF, PIN_DIRECTION dir);
extern void				NukeDownstream(IBaseFilter* pBF, IFilterGraph* pFG);
extern void				NukeDownstream(IPin* pPin, IFilterGraph* pFG);
extern IBaseFilter*		FindFilter(LPCWSTR clsid, IFilterGraph* pFG);
extern IBaseFilter*		FindFilter(const CLSID& clsid, IFilterGraph* pFG);
extern IPin*			FindPin(IBaseFilter* pBF, PIN_DIRECTION direction, const AM_MEDIA_TYPE* pRequestedMT);
extern IPin*			FindPin(IBaseFilter* pBF, PIN_DIRECTION direction, const GUID majortype);
extern CString			GetFilterName(IBaseFilter* pBF);
extern CString			GetPinName(IPin* pPin);
extern IFilterGraph*	GetGraphFromFilter(IBaseFilter* pBF);
extern IBaseFilter*		GetFilterFromPin(IPin* pPin);
extern IPin*			AppendFilter(IPin* pPin, CString DisplayName, IGraphBuilder* pGB);
extern IBaseFilter*		AppendFilter(IPin* pPin, IMoniker* pMoniker, IGraphBuilder* pGB);
extern IPin*			InsertFilter(IPin* pPin, CString DisplayName, IGraphBuilder* pGB);
extern bool				CreateFilter(CString DisplayName, IBaseFilter** ppBF, CString& FriendlyName);
extern bool				HasMediaType(IFilterGraph *pFilterGraph, const GUID &mediaType);

extern void				ExtractMediaTypes(IPin* pPin, std::vector<GUID>& types);
extern void				ExtractMediaTypes(IPin* pPin, std::list<PinType>& types);
extern void				ExtractMediaTypes(IPin* pPin, std::list<CMediaType>& mts);
extern bool				ExtractBIH(const AM_MEDIA_TYPE* pmt, BITMAPINFOHEADER* bih);
extern bool				ExtractBIH(IMediaSample* pMS, BITMAPINFOHEADER* bih);
extern bool				ExtractAvgTimePerFrame(const AM_MEDIA_TYPE* pmt, REFERENCE_TIME& rtAvgTimePerFrame);
extern bool				ExtractDim(const AM_MEDIA_TYPE* pmt, int& w, int& h, int& arx, int& ary);

extern CLSID			GetCLSID(IBaseFilter* pBF);
extern CLSID			GetCLSID(IPin* pPin);

extern void				ShowPPage(CString DisplayName, HWND hParentWnd);
extern void				ShowPPage(IUnknown* pUnknown, HWND hParentWnd);

extern bool				IsCLSIDRegistered(LPCWSTR clsid);
extern bool				IsCLSIDRegistered(const CLSID& clsid);

// return S_OK if installed and available, S_FALSE if installed but not available, E_FAIL if not installed
extern HRESULT			CheckFilterCLSID(const CLSID& clsid);

extern void				CStringToBin(CString str, std::vector<BYTE>& data);
extern CString			BinToCString(const BYTE* ptr, size_t len);

enum cdrom_t {
	CDROM_NotFound,
	CDROM_Audio,
	CDROM_VideoCD,
	CDROM_DVDVideo,
	CDROM_BDVideo,
	CDROM_DVDAudio,
	CDROM_Unknown
};
extern cdrom_t			GetCDROMType(WCHAR drive, std::list<CString>& files);
extern CString			GetDriveLabel(WCHAR drive);

inline bool				HourOrMore(const REFERENCE_TIME rt) { return (rt > UNITS * 3600); };
TimeCode_t				ReftimeToTimecode(const REFERENCE_TIME rt);
TimeCode_t				ReftimeToHMS(const REFERENCE_TIME rt); // seconds rounded to the nearest value
REFERENCE_TIME			TimecodeToReftime(const TimeCode_t tc);
CStringW				ReftimeToString(const REFERENCE_TIME rt, bool showZeroHours = true);  // hh:mm::ss.millisec
CStringW				ReftimeToString2(const REFERENCE_TIME rt, bool showZeroHours = true); // hh:mm::ss (round)

extern DVD_HMSF_TIMECODE	RT2HMSF(REFERENCE_TIME rt, double fps = 0); // use to remember the current position
extern DVD_HMSF_TIMECODE	RT2HMS_r(REFERENCE_TIME rt);                // use only for information (for display on the screen)
extern REFERENCE_TIME		HMSF2RT(DVD_HMSF_TIMECODE hmsf, double fps = 0);
extern CStringW				DVDtimeToString(const DVD_HMSF_TIMECODE dvd_tc, bool showZeroHours =false);
extern REFERENCE_TIME		StringToReftime(LPCWSTR strVal);
extern REFERENCE_TIME		StringToReftime2(LPCWSTR strVal);

extern CString			GetFriendlyName(CString DisplayName);
extern HRESULT			LoadExternalObject(LPCWSTR path, REFCLSID clsid, REFIID iid, void** ppv);
extern HRESULT			LoadExternalFilter(LPCWSTR path, REFCLSID clsid, IBaseFilter** ppBF);
extern HRESULT			LoadExternalPropertyPage(IPersist* pP, REFCLSID clsid, IPropertyPage** ppPP);
extern void				UnloadExternalObjects();

extern CString			MakeFullPath(LPCWSTR path);
// simple file system path detector
extern bool				IsLikelyFilePath(const CString &str);

extern GUID				GUIDFromCString(CString str);
extern HRESULT			GUIDFromCString(CString str, GUID& guid);
extern CStringW			CStringFromGUID(const GUID& guid);

extern bool				DeleteRegKey(LPCWSTR pszKey, LPCWSTR pszSubkey);
extern bool				SetRegKeyValue(LPCWSTR pszKey, LPCWSTR pszSubkey, LPCWSTR pszValueName, LPCWSTR pszValue);
extern bool				SetRegKeyValue(LPCWSTR pszKey, LPCWSTR pszSubkey, LPCWSTR pszValue);

extern void				RegisterSourceFilter(const CLSID& clsid, const GUID& subtype2, LPCWSTR chkbytes, LPCWSTR ext = nullptr, ...);
extern void				RegisterSourceFilter(const CLSID& clsid, const GUID& subtype2, const std::list<CString>& chkbytes, LPCWSTR ext = nullptr, ...);
extern void				UnRegisterSourceFilter(const GUID& subtype);

extern CStringW			GetDXVAModeString(const GUID& guidDecoder);
extern CStringW			GetDXVAModeStringAndName(const GUID& guidDecoder);

extern void				TraceFilterInfo(IBaseFilter* pBF);
extern void				TracePinInfo(IPin* pPin);

extern void				getExtraData(const BYTE *format, const GUID *formattype, const ULONG formatlen, BYTE *extra, unsigned int *extralen);

extern int				MakeAACInitData(BYTE* pData, int profile, int freq, int channels);
extern bool				MakeMPEG2MediaType(CMediaType& mt, BYTE* seqhdr, DWORD len, int w, int h);
extern HRESULT			CreateMPEG2VIfromAVC(CMediaType* mt, BITMAPINFOHEADER* pbmi, REFERENCE_TIME AvgTimePerFrame, CSize pictAR, BYTE* extra, size_t extralen);
extern HRESULT			CreateMPEG2VIfromMVC(CMediaType* mt, BITMAPINFOHEADER* pbmi, REFERENCE_TIME AvgTimePerFrame, CSize pictAR, BYTE* extra, size_t extralen);
extern HRESULT			CreateMPEG2VISimple(CMediaType* mt, BITMAPINFOHEADER* pbmi, REFERENCE_TIME AvgTimePerFrame, CSize pictAR, BYTE* extra, size_t extralen, DWORD dwProfile = 0, DWORD dwLevel = 0, DWORD dwFlags = 0);
extern HRESULT			CreateAVCfromH264(CMediaType* mt);

extern void				CreateVorbisMediaType(CMediaType& mt, std::vector<CMediaType>& mts, DWORD Channels, DWORD SamplesPerSec, DWORD BitsPerSample, const BYTE* pData, size_t Count);

extern CStringA			VobSubDefHeader(int w, int h, CStringA palette = "");
extern void				CorrectWaveFormatEx(CMediaType& mt);

extern inline const LONGLONG GetPerfCounter();

class CPinInfo : public PIN_INFO
{
public:
	CPinInfo() { pFilter = nullptr; }
	~CPinInfo() { if (pFilter) { pFilter->Release(); } }
};

class CFilterInfo : public FILTER_INFO
{
public:
	CFilterInfo() { pGraph = nullptr; }
	~CFilterInfo() { if (pGraph) { pGraph->Release(); } }
};

template <class T>
static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
	*phr = S_OK;
	CUnknown* punk = new(std::nothrow) T(lpunk, phr);
	if (punk == nullptr) {
		*phr = E_OUTOFMEMORY;
	}
	return punk;
}

namespace CStringUtils
{
	struct IgnoreCaseLess {
		bool operator()(const CString& str1, const CString& str2) const {
			return str1.CompareNoCase(str2) < 0;
		}
	};
	struct LogicalLess {
		bool operator()(const CString& str1, const CString& str2) const {
			return StrCmpLogicalW(str1, str2) < 0;
		}
	};
}

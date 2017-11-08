/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2017 see Authors.txt
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
#include <winddk/devioctl.h>
#include <winddk/ntddcdrm.h>

#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include "CDDAReader.h"
#include "../../../DSUtil/DSUtil.h"

 // option names
#define OPT_REGKEY_CDDAReader	L"Software\\MPC-BE Filters\\CDDAReader"
#define OPT_SECTION_CDDAReader	L"Filters\\CDDAReader"
#define OPT_ReadTextInfo		L"ReadTextInfo"

#define RAW_SECTOR_SIZE 2352
#define MSF2UINT(hgs) ((hgs[1]*4500)+(hgs[2]*75)+(hgs[3]))

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] = {
	{&MEDIATYPE_Stream,	&MEDIASUBTYPE_WAVE},
};

const AMOVIESETUP_PIN sudOpPin[] = {
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, _countof(sudPinTypesOut), sudPinTypesOut},
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CCDDAReader), CCDDAReaderName, MERIT_NORMAL, _countof(sudOpPin), sudOpPin, CLSID_LegacyAmFilterCategory}
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CCDDAReader>, nullptr, &sudFilter[0]}
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	SetRegKeyValue(
		L"Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}", L"{54A35221-2C8D-4a31-A5DF-6D809847E393}",
		L"0", L"0,4,,52494646,8,4,,43444441"); // "RIFFxxxxCDDA"

	SetRegKeyValue(
		L"Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}", L"{54A35221-2C8D-4a31-A5DF-6D809847E393}",
		L"Source Filter", L"{54A35221-2C8D-4a31-A5DF-6D809847E393}");

	SetRegKeyValue(
		L"Media Type\\Extensions", L".cda",
		L"Source Filter", L"{54A35221-2C8D-4a31-A5DF-6D809847E393}");

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	DeleteRegKey(L"Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}", L"{54A35221-2C8D-4a31-A5DF-6D809847E393}");
	DeleteRegKey(L"Media Type\\Extensions", L".cda");

	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

//
// CCDDAReader
//

CCDDAReader::CCDDAReader(IUnknown* pUnk, HRESULT* phr)
	: CAsyncReader(NAME("CCDDAReader"), pUnk, &m_stream, phr, __uuidof(this))
	, m_bReadTextInfo(true)
{
	if (phr) {
		*phr = S_OK;
	}

#ifdef REGISTER_FILTER
	CRegKey key;

	if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, OPT_REGKEY_CDDAReader, KEY_READ)) {
		DWORD dw;

		if (ERROR_SUCCESS == key.QueryDWORDValue(OPT_ReadTextInfo, dw)) {
			m_bReadTextInfo = !!dw;
		}
	}
#else
	m_bReadTextInfo = !!AfxGetApp()->GetProfileInt(OPT_SECTION_CDDAReader, OPT_ReadTextInfo, m_bReadTextInfo);
#endif
}

CCDDAReader::~CCDDAReader()
{
}

STDMETHODIMP CCDDAReader::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		QI(IFileSourceFilter)
		QI2(IAMMediaContent)
		QI(ICDDAReaderFilter)
		QI(ISpecifyPropertyPages)
		QI(ISpecifyPropertyPages2)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CCDDAReader::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	wcscpy_s(pInfo->achName, CCDDAReaderName);
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

// IFileSourceFilter

STDMETHODIMP CCDDAReader::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt)
{
	if (!m_stream.Load(pszFileName, m_bReadTextInfo)) {
		return E_FAIL;
	}

	m_fn = pszFileName;

	CMediaType mt;
	mt.majortype = MEDIATYPE_Stream;
	mt.subtype = MEDIASUBTYPE_WAVE;
	m_mt = mt;

	return S_OK;
}

STDMETHODIMP CCDDAReader::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
{
	CheckPointer(ppszFileName, E_POINTER);

	size_t nCount = m_fn.GetLength() + 1;
	*ppszFileName = (LPOLESTR)CoTaskMemAlloc(nCount * sizeof(WCHAR));
	CheckPointer(*ppszFileName, E_OUTOFMEMORY);

	wcscpy_s(*ppszFileName, nCount, m_fn);
	return S_OK;
}

// IAMMediaContent

STDMETHODIMP CCDDAReader::GetTypeInfoCount(UINT* pctinfo)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCDDAReader::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCDDAReader::GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCDDAReader::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCDDAReader::get_AuthorName(BSTR* pbstrAuthorName)
{
	CheckPointer(pbstrAuthorName, E_POINTER);
	CString str = m_stream.m_trackArtist;
	if (str.IsEmpty()) {
		str = m_stream.m_discArtist;
	}
	*pbstrAuthorName = str.AllocSysString();
	return S_OK;
}

STDMETHODIMP CCDDAReader::get_Title(BSTR* pbstrTitle)
{
	CheckPointer(pbstrTitle, E_POINTER);
	CString str = m_stream.m_trackTitle;
	if (str.IsEmpty()) {
		str = m_stream.m_discTitle;
	}
	*pbstrTitle = str.AllocSysString();
	return S_OK;
}

STDMETHODIMP CCDDAReader::get_Rating(BSTR* pbstrRating)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCDDAReader::get_Description(BSTR* pbstrDescription)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCDDAReader::get_Copyright(BSTR* pbstrCopyright)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCDDAReader::get_BaseURL(BSTR* pbstrBaseURL)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCDDAReader::get_LogoURL(BSTR* pbstrLogoURL)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCDDAReader::get_LogoIconURL(BSTR* pbstrLogoURL)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCDDAReader::get_WatermarkURL(BSTR* pbstrWatermarkURL)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCDDAReader::get_MoreInfoURL(BSTR* pbstrMoreInfoURL)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCDDAReader::get_MoreInfoBannerImage(BSTR* pbstrMoreInfoBannerImage)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCDDAReader::get_MoreInfoBannerURL(BSTR* pbstrMoreInfoBannerURL)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCDDAReader::get_MoreInfoText(BSTR* pbstrMoreInfoText)
{
	return E_NOTIMPL;
}

// ISpecifyPropertyPages2

STDMETHODIMP CCDDAReader::GetPages(CAUUID* pPages)
{
	CheckPointer(pPages, E_POINTER);

	pPages->cElems = 1;
	pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID) * pPages->cElems);
	pPages->pElems[0] = __uuidof(CCDDAReaderSettingsWnd);

	return S_OK;
}

STDMETHODIMP CCDDAReader::CreatePage(const GUID& guid, IPropertyPage** ppPage)
{
	CheckPointer(ppPage, E_POINTER);

	if (*ppPage != nullptr) {
		return E_INVALIDARG;
	}

	HRESULT hr;

	if (guid == __uuidof(CCDDAReaderSettingsWnd)) {
		(*ppPage = DNew CInternalPropertyPageTempl<CCDDAReaderSettingsWnd>(nullptr, &hr))->AddRef();
	}

	return *ppPage ? S_OK : E_FAIL;
}

// ICDDAReaderFilter

STDMETHODIMP CCDDAReader::Apply()
{
#ifdef REGISTER_FILTER
	CRegKey key;
	if (ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, OPT_REGKEY_CDDAReader)) {
		key.SetDWORDValue(OPT_ReadTextInfo, m_bReadTextInfo);
	}
#else
	AfxGetApp()->WriteProfileInt(OPT_SECTION_CDDAReader, OPT_ReadTextInfo, m_bReadTextInfo);
#endif

	return S_OK;
}

STDMETHODIMP CCDDAReader::SetReadTextInfo(BOOL nValue)
{
	CAutoLock cAutoLock(&m_csProps);
	m_bReadTextInfo = !!nValue;
	return S_OK;
}

STDMETHODIMP_(BOOL) CCDDAReader::GetReadTextInfo()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_bReadTextInfo;
}

// CCDDAStream

CCDDAStream::CCDDAStream()
{
	ZeroMemory(&m_header, sizeof(m_header));
	m_header.riff.hdr.chunkID = RIFFID;
	m_header.riff.WAVE = WAVEID;
	m_header.frm.hdr.chunkID = FormatID;
	m_header.frm.hdr.chunkSize = sizeof(m_header.frm.pcm);
	m_header.frm.pcm.wf.wFormatTag = WAVE_FORMAT_PCM;
	m_header.frm.pcm.wf.nSamplesPerSec = 44100;
	m_header.frm.pcm.wf.nChannels = 2;
	m_header.frm.pcm.wBitsPerSample = 16;
	m_header.frm.pcm.wf.nBlockAlign = m_header.frm.pcm.wf.nChannels * m_header.frm.pcm.wBitsPerSample / 8;
	m_header.frm.pcm.wf.nAvgBytesPerSec = m_header.frm.pcm.wf.nSamplesPerSec * m_header.frm.pcm.wf.nBlockAlign;
	m_header.data.hdr.chunkID = DataID;
}

CCDDAStream::~CCDDAStream()
{
	if (m_hDrive != INVALID_HANDLE_VALUE) {
		CloseHandle(m_hDrive);
		m_hDrive = INVALID_HANDLE_VALUE;
	}
}

enum PacketTypes {
	CD_TEXT_TITLE,
	CD_TEXT_PERFORMER,
	CD_TEXT_SONGWRITER,
	CD_TEXT_COMPOSER,
	CD_TEXT_ARRANGER,
	CD_TEXT_MESSAGES,
	CD_TEXT_DISC_ID,
	CD_TEXT_GENRE,
	CD_TEXT_TOC_INFO,
	CD_TEXT_TOC_INFO2,
	CD_TEXT_RESERVED_1,
	CD_TEXT_RESERVED_2,
	CD_TEXT_RESERVED_3,
	CD_TEXT_RESERVED_4,
	CD_TEXT_CODE,
	CD_TEXT_SIZE,
	CD_TEXT_NUM_PACKS
};

#define CD_TEXT_FIELD_LENGTH         12
#define CD_TEXT_FIELD_LENGTH_UNICODE  6

bool CCDDAStream::Load(const WCHAR* fnw, bool bReadTextInfo)
{
	const CString path(fnw);

	int iDriveLetter = path.Find(L":\\") - 1;
	int iTrackIndex = CString(path).MakeLower().Find(L".cda") - 1;
	if (iDriveLetter < 0 || iTrackIndex <= iDriveLetter) {
		return false;
	}

	while (iTrackIndex > 0 && _istdigit(path[iTrackIndex - 1])) {
		iTrackIndex--;
	}
	if (1 != swscanf_s(path.Mid(iTrackIndex), L"%d", &iTrackIndex)) {
		return false;
	}

	if (m_hDrive != INVALID_HANDLE_VALUE) {
		CloseHandle(m_hDrive);
		m_hDrive = INVALID_HANDLE_VALUE;
	}

	CString drive = CString(L"\\\\.\\") + path[iDriveLetter] + L":";

	m_hDrive = CreateFile(drive, GENERIC_READ, FILE_SHARE_READ, nullptr,
						  OPEN_EXISTING, FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
	if (m_hDrive == INVALID_HANDLE_VALUE) {
		return false;
	}

	CDROM_TOC cdrom_TOC = { 0 };
	DWORD BytesReturned;
	if (!DeviceIoControl(m_hDrive, IOCTL_CDROM_READ_TOC, nullptr, 0, &cdrom_TOC, sizeof(cdrom_TOC), &BytesReturned, 0)
			|| !(cdrom_TOC.FirstTrack <= iTrackIndex && iTrackIndex <= cdrom_TOC.LastTrack)) {
		CloseHandle(m_hDrive);
		m_hDrive = INVALID_HANDLE_VALUE;
		return false;
	}

	// MMC-3 Draft Revision 10g: Table 222 - Q Sub-channel control field
	cdrom_TOC.TrackData[iTrackIndex - 1].Control &= 5;
	if (!(cdrom_TOC.TrackData[iTrackIndex - 1].Control == 0 || cdrom_TOC.TrackData[iTrackIndex - 1].Control == 1)) {
		CloseHandle(m_hDrive);
		m_hDrive = INVALID_HANDLE_VALUE;
		return false;
	}

	if (cdrom_TOC.TrackData[iTrackIndex - 1].Control & 8) {
		m_header.frm.pcm.wf.nChannels = 4;
	}

	m_nStartSector = MSF2UINT(cdrom_TOC.TrackData[iTrackIndex - 1].Address) - 150;
	m_nStopSector = MSF2UINT(cdrom_TOC.TrackData[iTrackIndex].Address) - 150;

	m_llLength = LONGLONG(m_nStopSector - m_nStartSector) * RAW_SECTOR_SIZE;

	m_header.riff.hdr.chunkSize = (long)(m_llLength + sizeof(m_header) - 8);
	m_header.data.hdr.chunkSize = (long)(m_llLength);

	if (bReadTextInfo) {
		do {
			CDROM_READ_TOC_EX TOCEx = { 0 };
			TOCEx.Format = CDROM_READ_TOC_EX_FORMAT_CDTEXT;
			TOCEx.SessionTrack = iTrackIndex;
			WORD size = 0;
			ASSERT(MINIMUM_CDROM_READ_TOC_EX_SIZE == sizeof(size));
			if (!DeviceIoControl(m_hDrive, IOCTL_CDROM_READ_TOC_EX, &TOCEx, sizeof(TOCEx), &size, sizeof(size), &BytesReturned, 0)) {
				break;
			}

			size = _byteswap_ushort(size);
			if (size <= MINIMUM_CDROM_READ_TOC_EX_SIZE) { // No cd-text information
				break;
			}

			size += 2;

			std::vector<BYTE> pCDTextData(size);

			if (!DeviceIoControl(m_hDrive, IOCTL_CDROM_READ_TOC_EX, &TOCEx, sizeof(TOCEx), pCDTextData.data(), size, &BytesReturned, 0)) {
				break;
			}

			size = (WORD)(BytesReturned - sizeof(CDROM_TOC_CD_TEXT_DATA));
			CDROM_TOC_CD_TEXT_DATA_BLOCK* pCDTextBlock = ((CDROM_TOC_CD_TEXT_DATA*)pCDTextData.data())->Descriptors;

			CStringArray CDTextStrings[CD_TEXT_NUM_PACKS];
			for (int i = 0; i < CD_TEXT_NUM_PACKS; i++) {
				CDTextStrings[i].SetSize(1 + cdrom_TOC.LastTrack);
			}

			for (; size >= sizeof(CDROM_TOC_CD_TEXT_DATA_BLOCK); size -= sizeof(CDROM_TOC_CD_TEXT_DATA_BLOCK), pCDTextBlock++) {
				if (pCDTextBlock->TrackNumber > cdrom_TOC.LastTrack) {
					continue;
				}

				if (pCDTextBlock->PackType < 0x80 || pCDTextBlock->PackType >= 0x80 + 0x10) {
					continue;
				}
				pCDTextBlock->PackType -= 0x80;

				int nLengthRemaining = pCDTextBlock->Unicode ? CD_TEXT_FIELD_LENGTH_UNICODE : CD_TEXT_FIELD_LENGTH;
				UINT nMaxLength = nLengthRemaining;
				UINT nOffset = 0;
				UCHAR nTrack = pCDTextBlock->TrackNumber;
				while (nTrack <= cdrom_TOC.LastTrack && nLengthRemaining > 0 && nOffset < nMaxLength) {
					CString Text = pCDTextBlock->Unicode
						? CString((WCHAR*)pCDTextBlock->WText + nOffset, nLengthRemaining)
						: CString(CStringA((CHAR*)pCDTextBlock->Text + nOffset, nLengthRemaining));
					CDTextStrings[pCDTextBlock->PackType][nTrack] += Text;
					nOffset += Text.GetLength() + 1;
					nLengthRemaining -= (Text.GetLength() + 1);
					++nTrack;
				}
			}

			m_discTitle   = CDTextStrings[CD_TEXT_TITLE][0];
			m_trackTitle  = CDTextStrings[CD_TEXT_TITLE][iTrackIndex];
			m_discArtist  = CDTextStrings[CD_TEXT_PERFORMER][0];
			m_trackArtist = CDTextStrings[CD_TEXT_PERFORMER][iTrackIndex];
		} while (0);
	}

	return true;
}

HRESULT CCDDAStream::SetPointer(LONGLONG llPos)
{
	if (llPos < 0 || llPos > m_llLength) {
		return S_FALSE;
	}

	if (m_llPosition != llPos) {
		m_buff_pos   = 0;
		m_llPosition = llPos;
	}

	return S_OK;
}

HRESULT CCDDAStream::Read(PBYTE pbBuffer, DWORD dwBytesToRead, BOOL bAlign, LPDWORD pdwBytesRead)
{
	CAutoLock lck(&m_csLock);

	const PBYTE pbBufferOrg = pbBuffer;
	LONGLONG pos = m_llPosition;
	DWORD len = dwBytesToRead;

	if (pos < sizeof(m_header) && len > 0) {
		const DWORD size = std::min(len, (DWORD)(sizeof(m_header) - pos));
		memcpy(pbBuffer, &((BYTE*)&m_header)[pos], size);
		pbBuffer += size;
		pos += size;
		len -= size;
	}
	pos -= sizeof(m_header);

	if (m_buff_pos && m_buff_pos < m_buff.size()) {
		const DWORD size = std::min(len, (DWORD)(m_buff.size() - m_buff_pos));
		memcpy(pbBuffer, &m_buff[m_buff_pos], size);

		m_buff_pos += size;
		pbBuffer += size;
		pos += size;
		len -= size;
	}

	while (pos >= 0 && pos < m_llLength && len > 0) {
		RAW_READ_INFO rawreadinfo;
		rawreadinfo.TrackMode = CDDA;

		const UINT sector = m_nStartSector + UINT(pos / RAW_SECTOR_SIZE);
		const UINT offset = pos % RAW_SECTOR_SIZE;

		// Reading 20 sectors at once seems to be a good trade-off between performance and compatibility
		rawreadinfo.SectorCount = std::min(20u, m_nStopSector - sector);

		if (m_buff.size() != rawreadinfo.SectorCount * RAW_SECTOR_SIZE) {
			m_buff.resize(rawreadinfo.SectorCount * RAW_SECTOR_SIZE);
		}

		rawreadinfo.DiskOffset.QuadPart = sector * 2048;
		DWORD dwBytesReturned = 0;
		VERIFY(DeviceIoControl(m_hDrive, IOCTL_CDROM_RAW_READ,
							   &rawreadinfo, sizeof(rawreadinfo),
							   m_buff.data(), m_buff.size(),
							   &dwBytesReturned, 0));

		const DWORD size = (DWORD)std::min((LONGLONG)std::min(len, (dwBytesReturned - offset)), m_llLength - pos);
		memcpy(pbBuffer, &m_buff[offset], size);

		pbBuffer += size;
		pos += size;
		len -= size;

		m_buff_pos = size;
	}

	if (pdwBytesRead) {
		*pdwBytesRead = pbBuffer - pbBufferOrg;
	}
	m_llPosition += pbBuffer - pbBufferOrg;

	return S_OK;
}

LONGLONG CCDDAStream::Size(LONGLONG* pSizeAvailable)
{
	const LONGLONG size = sizeof(m_header) + m_llLength;
	if (pSizeAvailable) {
		*pSizeAvailable = size;
	}
	return size;
}

DWORD CCDDAStream::Alignment()
{
	return 1;
}

void CCDDAStream::Lock()
{
	m_csLock.Lock();
}

void CCDDAStream::Unlock()
{
	m_csLock.Unlock();
}

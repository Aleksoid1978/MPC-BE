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
#include <regex>
#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include "ShoutcastSource.h"
#include "../../../DSUtil/GolombBuffer.h"
#include "../../../DSUtil/AudioParser.h"
#include <MMReg.h>
#include <moreuuids.h>

#define MAXFRAMESIZE	((144 * 320000 / 8000) + 1)
#define BUFFERS			2
#define MINBUFFERLENGTH	  1000000i64
#define AVGBUFFERLENGTH	 30000000i64
#define MAXBUFFERLENGTH	100000000i64

#define MPA_HEADER_SIZE		4
#define ADTS_HEADER_SIZE	9

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] = {
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_MP3},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_RAW_AAC1},
};

const AMOVIESETUP_PIN sudOpPin[] = {
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, nullptr, _countof(sudPinTypesOut), sudPinTypesOut}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CShoutcastSource), ShoutcastSourceName, MERIT_NORMAL, _countof(sudOpPin), sudOpPin, CLSID_LegacyAmFilterCategory}
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CShoutcastSource>, nullptr, &sudFilter[0]}
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

class CShoutcastSourceApp : public CFilterApp
{
public:
	BOOL InitInstance() {
		if (!__super::InitInstance()) {
			return FALSE;
		}
		/*
				if (!AfxWinInit(::GetModuleHandleW(nullptr), nullptr, ::GetCommandLineW(), 0))
				{
					AfxMessageBox(L"AfxWinInit failed!");
					return FALSE;
				}
		*/
		if (!AfxSocketInit(nullptr)) {
			AfxMessageBox(L"AfxSocketInit failed!");
			return FALSE;
		}

		return TRUE;
	}
};

CShoutcastSourceApp theApp;

#endif

//
// CShoutcastSource
//

CShoutcastSource::CShoutcastSource(LPUNKNOWN lpunk, HRESULT* phr)
	: CSource(L"CShoutcastSource", lpunk, __uuidof(this))
{
#ifndef REGISTER_FILTER
	AfxSocketInit();
#endif
}

CShoutcastSource::~CShoutcastSource()
{
}

STDMETHODIMP CShoutcastSource::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		QI(IFileSourceFilter)
		QI(IAMFilterMiscFlags)
		QI(IAMOpenProgress)
		QI2(IAMMediaContent)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IFileSourceFilter

STDMETHODIMP CShoutcastSource::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt)
{
	if (GetPinCount() > 0) {
		return VFW_E_ALREADY_CONNECTED;
	}

	HRESULT hr = E_OUTOFMEMORY;

	if (!(DNew CShoutcastStream(pszFileName, this, &hr)) || FAILED(hr)) {
		return hr;
	}

	m_fn = pszFileName;

	return S_OK;
}

STDMETHODIMP CShoutcastSource::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
{
	CheckPointer(ppszFileName, E_POINTER);

	size_t nCount = m_fn.GetLength() + 1;
	*ppszFileName = (LPOLESTR)CoTaskMemAlloc(nCount * sizeof(WCHAR));
	CheckPointer(*ppszFileName, E_OUTOFMEMORY);

	wcscpy_s(*ppszFileName, nCount, m_fn);
	return S_OK;
}

// IAMFilterMiscFlags

ULONG CShoutcastSource::GetMiscFlags()
{
	return AM_FILTER_MISC_FLAGS_IS_SOURCE;
}

// IAMOpenProgress

STDMETHODIMP CShoutcastSource::QueryProgress(LONGLONG* pllTotal, LONGLONG* pllCurrent)
{
	if (m_iPins == 1) {
		if (pllTotal) {
			*pllTotal = 100;
		}
		if (pllCurrent) {
			*pllCurrent = (static_cast<CShoutcastStream*>(m_paStreams[0]))->GetBufferFullness();
		}
		return S_OK;
	}

	return E_UNEXPECTED;
}

STDMETHODIMP CShoutcastSource::AbortOperation()
{
	return E_NOTIMPL;
}

// IAMMediaContent

STDMETHODIMP CShoutcastSource::get_Title(BSTR* pbstrTitle)
{
	CheckPointer(pbstrTitle, E_POINTER);

	if (m_iPins == 1) {
		*pbstrTitle = (static_cast<CShoutcastStream*>(m_paStreams[0]))->GetTitle().AllocSysString();
		return S_OK;
	}

	return E_UNEXPECTED;
}

STDMETHODIMP CShoutcastSource::get_Description(BSTR* pbstrDescription)
{
	CheckPointer(pbstrDescription, E_POINTER);

	if (m_iPins == 1) {
		*pbstrDescription = (static_cast<CShoutcastStream*>(m_paStreams[0]))->GetDescription().AllocSysString();
		return S_OK;
	}

	return E_UNEXPECTED;
}

STDMETHODIMP CShoutcastSource::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));
	wcscpy_s(pInfo->achName, ShoutcastSourceName);
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

// CShoutcastStream

CShoutcastStream::CShoutcastStream(const WCHAR* wfn, CShoutcastSource* pParent, HRESULT* phr)
	: CSourceStream(L"ShoutcastStream", phr, pParent, L"Output")
	, m_fBuffering(false)
	, m_hSocket(INVALID_SOCKET)
{
	ASSERT(phr);

	*phr = S_OK;

	CString fn(wfn);
	if (fn.Find(L"://") < 0) {
		fn = L"http://" + fn;
	}

	int redirectTry = 0;
redirect:

	if (!m_url.CrackUrl(fn)) {
		*phr = E_FAIL;
		return;
	}

	if (!m_socket.Create()) {
		*phr = E_FAIL;
		return;
	}

	m_socket.SetUserAgent("MPC ShoutCast Source");

	CString redirectUrl;
	if (!m_socket.Connect(m_url, redirectUrl)) {
		int nError = GetLastError();
		if (nError == WSAEINTR) {
			DLog(L"CShoutcastStream(): failed connect for TimeOut!");
		}

		m_socket.Close();

		if (!redirectUrl.IsEmpty() && redirectTry < 5) {
			redirectTry++;
			if (redirectUrl[1] == '/') {
				fn += redirectUrl;
			}
			fn = redirectUrl;
			DLog(L"CShoutcastStream(): redirect to \"%s\"", fn);
			goto redirect;
		}
		*phr = E_FAIL;
		return;
	}

	CMediaType mt;
	if (SUCCEEDED(GetMediaType(0, &mt))) {
		SetName(GetMediaTypeDesc(&mt, L"Output"));
	}

	m_hSocket = m_socket.Detach();
}

CShoutcastStream::~CShoutcastStream()
{
	if (m_hSocket != INVALID_SOCKET) {
		m_socket.Attach(m_hSocket);
	}
	m_socket.Close();
}

void CShoutcastStream::EmptyBuffer()
{
	CAutoLock cAutoLock(&m_queue);
	m_queue.RemoveAll();
}

LONGLONG CShoutcastStream::GetBufferFullness()
{
	CAutoLock cAutoLock(&m_queue);
	if (!m_fBuffering) {
		return 100;
	}
	if (m_queue.IsEmpty()) {
		return 0;
	}
	LONGLONG ret = 100i64*(m_queue.GetDuration()) / AVGBUFFERLENGTH;
	return std::min(ret, 100LL);
}

CString CShoutcastStream::GetTitle()
{
	CAutoLock cAutoLock(&m_queue);
	return m_title;
}

CString CShoutcastStream::GetDescription()
{
	CAutoLock cAutoLock(&m_queue);
	return m_description;
}

HRESULT CShoutcastStream::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
	ASSERT(pAlloc);
	ASSERT(pProperties);

	HRESULT hr = S_OK;

	pProperties->cBuffers = BUFFERS;
	pProperties->cbBuffer = MAXFRAMESIZE;

	ALLOCATOR_PROPERTIES Actual;
	if (FAILED(hr = pAlloc->SetProperties(pProperties, &Actual))) {
		return hr;
	}

	if (Actual.cbBuffer < pProperties->cbBuffer) {
		return E_FAIL;
	}
	ASSERT(Actual.cBuffers == pProperties->cBuffers);

	return hr;
}

HRESULT CShoutcastStream::FillBuffer(IMediaSample* pSample)
{
	HRESULT hr;

	BYTE* pData = nullptr;
	if (FAILED(hr = pSample->GetPointer(&pData)) || !pData) {
		return S_FALSE;
	}

	do {
		// do we have to refill our buffer?
		{
			CAutoLock cAutoLock(&m_queue);
			if (m_queue.GetDuration() > MINBUFFERLENGTH) {
				break;    // nope, that's great
			}
		}

		DLog(L"CShoutcastStream(): START BUFFERING");
		m_fBuffering = true;

		for (;;) {
			if (fExitThread) { // playback stopped?
				return S_FALSE;
			}

			Sleep(50);

			CAutoLock cAutoLock(&m_queue);
			if (m_queue.GetDuration() > AVGBUFFERLENGTH) {
				break;    // this is enough
			}
		}

		pSample->SetDiscontinuity(TRUE);

		DeliverBeginFlush();
		DeliverEndFlush();

		DeliverNewSegment(0, ~0, 1.0);

		DLog(L"CShoutcastStream(): END BUFFERING");
		m_fBuffering = false;
	} while (false);

	{
		CAutoLock cAutoLock(&m_queue);
		ASSERT(!m_queue.IsEmpty());
		if (!m_queue.IsEmpty()) {
			CAutoPtr<CShoutCastPacket> p = m_queue.RemoveHead();
			long len = std::min(pSample->GetSize(), (long)p->size());
			memcpy(pData, p->data(), len);
			pSample->SetActualDataLength(len);
			pSample->SetTime(&p->rtStart, &p->rtStop);
			m_title = p->title;
		}
	}

	pSample->SetSyncPoint(TRUE);

	return S_OK;
}

HRESULT CShoutcastStream::GetMediaType(int iPosition, CMediaType* pmt)
{
	CAutoLock cAutoLock(m_pFilter->pStateLock());

	if (iPosition < 0) {
		return E_INVALIDARG;
	}
	if (iPosition > 0) {
		return VFW_S_NO_MORE_ITEMS;
	}

	if (m_socket.m_Format == AUDIO_MPEG) {
		pmt->SetType(&MEDIATYPE_Audio);
		pmt->SetSubtype(&MEDIASUBTYPE_MP3);
		pmt->SetFormatType(&FORMAT_WaveFormatEx);

		WAVEFORMATEX* wfe		= (WAVEFORMATEX*)pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX));
		memset(wfe, 0, sizeof(WAVEFORMATEX));
		wfe->wFormatTag			= (WORD)MEDIASUBTYPE_MP3.Data1;
		wfe->nChannels			= (WORD)m_socket.m_channels;
		wfe->nSamplesPerSec		= m_socket.m_samplerate;
		wfe->nAvgBytesPerSec	= m_socket.m_bitrate / 8;
		wfe->nBlockAlign		= 1;
		wfe->wBitsPerSample		= 0;
	} else if (m_socket.m_Format == AUDIO_AAC) {
		pmt->SetType(&MEDIATYPE_Audio);
		pmt->SetSubtype(&MEDIASUBTYPE_RAW_AAC1);
		pmt->SetFormatType(&FORMAT_WaveFormatEx);

		WAVEFORMATEX* wfe		= (WAVEFORMATEX*)DNew BYTE[sizeof(WAVEFORMATEX) + 5];
		memset(wfe, 0, sizeof(WAVEFORMATEX) + 5);
		wfe->wFormatTag			= WAVE_FORMAT_RAW_AAC1;
		wfe->nChannels			= (WORD)m_socket.m_channels;
		wfe->nSamplesPerSec		= m_socket.m_samplerate;
		wfe->nBlockAlign		= m_socket.m_framesize;
		wfe->nAvgBytesPerSec	= m_socket.m_bitrate / 8;
		wfe->cbSize				= MakeAACInitData((BYTE*)(wfe + 1), m_socket.m_aacprofile, wfe->nSamplesPerSec, wfe->nChannels);

		pmt->SetFormat((BYTE*)wfe, sizeof(WAVEFORMATEX) + wfe->cbSize);

		delete [] wfe;
	} else {
		return VFW_E_INVALID_MEDIA_TYPE;
	}

	return S_OK;
}

HRESULT CShoutcastStream::CheckMediaType(const CMediaType* pmt)
{
	if (pmt->majortype == MEDIATYPE_Audio
			&& ((pmt->subtype == MEDIASUBTYPE_MP3 && m_socket.m_Format == AUDIO_MPEG) ||
				(pmt->subtype == MEDIASUBTYPE_RAW_AAC1 && m_socket.m_Format == AUDIO_AAC))
			&& pmt->formattype == FORMAT_WaveFormatEx) {
		return S_OK;
	}

	return E_INVALIDARG;
}

static UINT SocketThreadProc(LPVOID pParam)
{
	return (static_cast<CShoutcastStream*>(pParam))->SocketThreadProc();
}

UINT CShoutcastStream::SocketThreadProc()
{
	fExitThread = false;

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	AfxSocketInit();

	CShoutcastSocket soc;

	CAutoVectorPtr<BYTE> pData;
	int nSize = std::max((int)m_socket.m_metaint, MAXFRAMESIZE);
	if (!pData.Allocate(nSize) || !soc.Create()) {
		return 1;
	}

	if (m_hSocket == INVALID_SOCKET) {
		CString redirectUrl;
		if (!m_socket.Create() || !m_socket.Connect(m_url, redirectUrl)) {
			return 1;
		}

		m_hSocket = m_socket.Detach();
	}

	soc.Attach(m_hSocket);
	soc = m_socket;

	m_title			= soc.m_title;
	m_description	= soc.m_description;

	REFERENCE_TIME m_rtSampleTime = 0;
	std::vector<NoInitByte> buffer;

	while (!fExitThread) {
		{
			if (m_queue.GetDuration() > MAXBUFFERLENGTH || m_queue.GetCount() >= 500) {
				// Buffer is full
				Sleep(100);
				continue;
			}
		}

		int len = soc.Receive(pData, nSize);
		if (len == 0) {
			break;
		}
		if (len == SOCKET_ERROR) {
			soc.Close();
			CString redirectUrl;
			if (!soc.Create() || !soc.Connect(m_url, redirectUrl)) {
				break;
			}

			len = soc.Receive(pData, nSize);
			if (len <= 0) {
				break;
			}
		}

		if (m_socket.m_Format != AUDIO_MPEG && m_socket.m_Format != AUDIO_AAC) {
			continue; // Hmm.
		}

		size_t old_size = buffer.size();
		buffer.resize(old_size + len);
		memcpy(buffer.data() + old_size, pData, (size_t)len);

		BYTE* pos = &buffer.front().value;
		const BYTE* end = pos + buffer.size();

		if (m_socket.m_Format == AUDIO_MPEG) {
			for(;;) {
				while (pos + 2 <= end && ((GETUINT16(pos) & MPA_SYNCWORD) != MPA_SYNCWORD)) {
					pos++;
				}

				if (pos + MPA_HEADER_SIZE > end) {
					break;
				}

				int size = ParseMPAHeader(pos);
				if (size == 0) {
					pos++;
					continue;
				}
				if (pos + size > end) {
					break;
				}

				if (pos + size + MPA_HEADER_SIZE <= end) {
					int size2 = ParseMPAHeader(pos + size);
					if (size2 == 0) {
						pos++;
						continue;
					}
				}

				CAutoPtr<CShoutCastPacket> p2(DNew CShoutCastPacket());
				p2->SetData(pos, size);
				p2->rtStart = m_rtSampleTime;
				p2->rtStop  = m_rtSampleTime + (10000000i64 * size * 8/soc.m_bitrate);
				m_rtSampleTime = p2->rtStop;
				p2->title = !soc.m_title.IsEmpty() ? soc.m_title : soc.m_url;

				{
					CAutoLock cAutoLock(&m_queue);
					m_queue.AddTail(p2);
				}

				pos += size;
			}
		}
		else if (m_socket.m_Format == AUDIO_AAC) {
			for(;;) {
				while (pos + 2 <= end && ((GETUINT16(pos) & AAC_ADTS_SYNCWORD) != AAC_ADTS_SYNCWORD)) {
					pos++;
				}

				if (pos + ADTS_HEADER_SIZE > end) {
					break;
				}

				audioframe_t aframe;
				int size = ParseADTSAACHeader(pos, &aframe);
				if (size == 0) {
					pos++;
					continue;
				}
				if (pos + size > end) {
					break;
				}

				if (pos + size + ADTS_HEADER_SIZE <= end) {
					int size2 = ParseADTSAACHeader(pos + size, &aframe);
					if (size2 == 0) {
						pos++;
						continue;
					}
				}

				CAutoPtr<CShoutCastPacket> p2(DNew CShoutCastPacket());
				p2->SetData(pos + aframe.param1, size - aframe.param1);
				p2->rtStart = m_rtSampleTime;
				p2->rtStop  = m_rtSampleTime + (10000000i64 * size * 8/soc.m_bitrate);
				m_rtSampleTime = p2->rtStop;
				p2->title = !soc.m_title.IsEmpty() ? soc.m_title : soc.m_url;

				{
					CAutoLock cAutoLock(&m_queue);
					m_queue.AddTail(p2);
				}

				pos += size;
			}
		}

		if (pos > &buffer.front().value) {
			size_t unused_size = end - pos;
			memmove(buffer.data(), pos, unused_size);
			buffer.resize(unused_size);
		}
	}

	soc.Close();
	m_hSocket = INVALID_SOCKET;

	fExitThread = true;

	return 0;
}

HRESULT CShoutcastStream::OnThreadCreate()
{
	EmptyBuffer();

	fExitThread = true;
	m_hSocketThread = AfxBeginThread(::SocketThreadProc, this)->m_hThread;

	while (fExitThread) {
		Sleep(10);
	}

	return S_OK;
}

HRESULT CShoutcastStream::OnThreadDestroy()
{
	EmptyBuffer();

	fExitThread = true;
	m_socket.CancelBlockingCall();
	WaitForSingleObject(m_hSocketThread, INFINITE);

	return S_OK;
}

HRESULT CShoutcastStream::Inactive()
{
	fExitThread = true;
	return __super::Inactive();
}

HRESULT CShoutcastStream::SetName(LPCWSTR pName)
{
	CheckPointer(pName, E_POINTER);
	if (m_pName) {
		delete [] m_pName;
	}

	size_t len = wcslen(pName) + 1;
	m_pName = DNew WCHAR[len];
	CheckPointer(m_pName, E_OUTOFMEMORY);
	wcscpy_s(m_pName, len, pName);

	return S_OK;
}

//

int CShoutcastStream::CShoutcastSocket::Receive(void* lpBuf, int nBufLen, int nFlags)
{
	if (nFlags & MSG_PEEK) {
		return __super::Receive(lpBuf, nBufLen, nFlags);
	}

	if (m_metaint > 0 && m_nBytesRead + nBufLen > m_metaint) {
		nBufLen = m_metaint - m_nBytesRead;
	}

	int len = __super::Receive(lpBuf, nBufLen, nFlags);
	if (len <= 0) {
		return len;
	}

	if ((m_nBytesRead += len) == m_metaint) {
		m_nBytesRead = 0;

		static BYTE buff[255*16], b = 0;
		memset(buff, 0, sizeof(buff));
		if (1 == __super::Receive(&b, 1) && b && b*16 == __super::Receive(buff, b*16)) {
			CString str = UTF8orLocalToWStr((LPCSTR)buff);

			DLog(L"CShoutcastStream(): Metainfo: %s", str);

			int i = str.Find(L"StreamTitle='");
			if (i >= 0) {
				i += 13;
				int j = str.Find(L"';", i);
				if (!j) {
					j = str.ReverseFind('\'');
				}
				if (j > i) {
					m_title = str.Mid(i, j - i);
				}
			} else {
				DLog(L"CShoutcastStream(): StreamTitle is missing");
			}

			i = str.Find(L"StreamUrl='");
			if (i >= 0) {
				i += 11;
				int j = str.Find(L"';", i);
				if (!j) {
					j = str.ReverseFind('\'');
				}
				if (j > i) {
					str = str.Mid(i, j - i);
					if (!str.IsEmpty()) {
						m_url = str;
					}
				}
			}
		}
	} else if (m_metaint > 0) {
		char* p = (char*)lpBuf;
		char* p0 = p;
		char* pend = p + len - 13;
		for (; p < pend; p++) {
			if (strncmp(p, "StreamTitle='", 13)) {
				continue;
			}

			DLog(L"CShoutcastStream(): !!!!!!!!!StreamTitle found inside mp3 data!!!!!!!!! offset=%Id", p - p0);
			DLog(L"resyncing...");
			while (p-- > p0) {
				if ((BYTE)*p >= 0x20) {
					continue;
				}

				DLog(L"CShoutcastStream(): found possible length byte: %d, skipping %d bytes", *p, 1 + *p*16);
				p += 1 + *p*16;
				len = (int)(p0 + len - p);
				DLog(L"CShoutcastStream(): returning the remaining bytes in the packet: %d", len);
				if (len <= 0) {
					DLog(L"CShoutcastStream(): nothing to return, reading a bit more in");
					if (len < 0) {
						__super::Receive(lpBuf, -len, nFlags);
					}

					int len = __super::Receive(lpBuf, nBufLen, nFlags);
					if (len <= 0) {
						return len;
					}
				}

				m_nBytesRead = len;
				memcpy(lpBuf, p, len);

				break;
			}

			break;
		}
	}

	return len;
}

bool CShoutcastStream::CShoutcastSocket::Connect(CUrl& url, CString& redirectUrl)
{
	redirectUrl.Empty();

	ClearHeaderParams();
	AddHeaderParams("Icy-MetaData:1");

	if (!__super::Connect(url)) {
		return false;
	}

	bool fOK = false;
	bool fTryAgain = false;
	int metaint = 0;
	int ContentLength = 0;

	do {
		m_nBytesRead = 0;
		m_metaint = metaint = 0;
		m_bitrate = 0;

		CStringA hdr = GetHeader();
		DLog(L"CShoutcastSocket::Connect() - HTTP hdr:\n%S", hdr);

		std::list<CStringA> sl;
		Explode(hdr, sl, '\n');
		for (auto& hdrline : sl) {

			CStringA param, value;
			int k = hdrline.Find(':');
			if (k > 0 && k + 1 < hdrline.GetLength()) {
				param = hdrline.Left(k).Trim().MakeLower();
				value = hdrline.Mid(k + 1).Trim();
			}

			hdrline.MakeLower();
			if (hdrline.Find("200 ok") > 0 && (hdrline.Find("icy") == 0 || hdrline.Find("http/") == 0)) {
				fOK = true;
			}
			else if (param == "content-type") {
				value.MakeLower();
				if (value == "audio/mpeg") {
					m_Format = AUDIO_MPEG;
					DLog(L"CShoutcastSocket::Connect() - detected MPEG Audio format");
				}
				else if (value == "audio/aac" || value == "audio/aacp") {
					m_Format = AUDIO_AAC;
					DLog(L"CShoutcastSocket::Connect() - detected AAC Audio format");
				}
				else if (value == "audio/x-scpls") {
					m_Format = AUDIO_PLS;
					DLog(L"CShoutcastSocket::Connect() - detected PLS playlist format");
				}
				else if (value == "audio/x-mpegurl") {
					m_Format = AUDIO_M3U;
					DLog(L"CShoutcastSocket::Connect() - detected M3U playlist format");
				}
				else if (value == "application/xspf+xml") {
					m_Format = AUDIO_XSPF;
					DLog(L"CShoutcastSocket::Connect() - detected XSPF playlist format");
				}
				else if (value == "application/ogg") {
					// not supported yet
					DLog(L"CShoutcastSocket::Connect() - detected Ogg format");
				}
			}
			else if (param == "content-length") {
				ContentLength = atoi(value);
			}
			else if (param == "location") {
				redirectUrl = value;
			}
			else if (param == "content-disposition") {
				if (int pos = value.Find("filename=") >= 0) {
					redirectUrl = L"/" + CString(value.Mid(pos + 9).Trim());
				}
			}
			else if (param == "icy-br") {
				m_bitrate = atoi(value) * 1000;
			}
			else if (param == "icy-metaint") {
				metaint = atoi(value);
			}
			else if (param == "icy-name") {
				m_title = UTF8orLocalToWStr(value);
			}
			else if (param == "icy-url") {
				m_url = value;
			}
			else if (param == "icy-description") {
				m_description = UTF8orLocalToWStr(value);
			}
		}

		if (!fOK && GetLastError() == WSAECONNRESET && !fTryAgain) {
			SendRequest();
			fTryAgain = true;
		} else {
			fTryAgain = false;
		}
	} while (fTryAgain);

	if (!fOK || (m_bitrate == 0 && metaint == 0 && m_title.IsEmpty())) {
		if (ContentLength && m_Format >= AUDIO_PLS) {
			char* buf = DNew char[ContentLength + 1];

			if (ContentLength == Receive((void*)buf, ContentLength)) {
				buf[ContentLength] = 0;

				if (m_Format == AUDIO_PLS) {
					const char* reg_esp = "File\\d[ \\t]*=[ \\t]*\"*(http\\://[^\\n\"]+)";
					std::regex rgx(reg_esp/*, std::regex_constants::icase*/);
					std::cmatch match;

					if (std::regex_search(buf, match, rgx) && match.size() == 2) {
						redirectUrl = CString(match[1].first, match[1].length());
						redirectUrl.Trim();
					}
				}
				else if (m_Format == AUDIO_M3U) {
					const char* reg_esp = "(^|\\n)(http\\://[^\\n]+)";
					std::regex rgx(reg_esp);
					std::cmatch match;

					if (std::regex_search(buf, match, rgx) && match.size() == 3) {
						redirectUrl = CString(match[2].first, match[2].length());
						redirectUrl.Trim();
					}
				}
				else if (m_Format == AUDIO_XSPF) {
					const char* reg_esp = "<location>(http\\://[^<]+)</location>";
					std::regex rgx(reg_esp);
					std::cmatch match;

					if (std::regex_search(buf, match, rgx) && match.size() == 2) {
						redirectUrl = CString(match[1].first, match[1].length());
						redirectUrl.Trim();
					}
				}
			}

			delete [] buf;
		}

		Close();
		return false;
	}

	m_metaint = metaint;

	return FindSync();
}

bool CShoutcastStream::CShoutcastSocket::FindSync()
{
	m_nBytesRead = 0;

	if (m_Format == AUDIO_NONE || m_Format >= AUDIO_PLS) {
		return false; // not supported
	}

	m_samplerate = (DWORD)-1;
	m_channels = (DWORD)-1;

	BYTE buff[10];
	BYTE b;

	if (m_Format == AUDIO_MPEG) {
		for (int i = MAXFRAMESIZE; i > 0; i--, Receive(&b, 1)) {
			if (4 == Receive(buff, 4, MSG_PEEK) && ParseMPAHeader(buff)) {
				audioframe_t aframe;
				m_framesize = ParseMPAHeader(buff, &aframe);
				m_samplerate	= aframe.samplerate;
				m_channels		= aframe.channels;
				if (aframe.param1 > 1) {
					m_bitrate = aframe.param1;
				}

				return true;
			}
		}
	} else if (m_Format == AUDIO_AAC) {
		for (int i = MAXFRAMESIZE; i > 0; i--, Receive(&b, 1)) {
			if (9 == Receive(buff, 9, MSG_PEEK) && ParseADTSAACHeader(buff)) {
				audioframe_t aframe;
				m_framesize		= ParseADTSAACHeader(buff, &aframe);
				m_samplerate	= aframe.samplerate;
				m_channels		= aframe.channels;
				m_aacprofile	= aframe.param2;

				return true;
			}
		}
	}

	return false;
}

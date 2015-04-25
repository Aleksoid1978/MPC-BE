/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2015 see Authors.txt
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
#include <atl/atlrx.h>
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
#define MINBUFFERLENGTH	1000000i64
#define AVGBUFFERLENGTH	30000000i64
#define MAXBUFFERLENGTH	100000000i64

#define MPA_FRAME_SIZE	4
#define ADTS_FRAME_SIZE	9

static const DWORD s_bitrate[2][16] = {
	{1,8,16,24,32,40,48,56,64,80,96,112,128,144,160,0},
	{1,32,40,48,56,64,80,96,112,128,160,192,224,256,320,0}
};

static const DWORD s_freq[4][4] = {
	{11025,12000,8000,0},
	{0,0,0,0},
	{22050,24000,16000,0},
	{44100,48000,32000,0}
};

static const BYTE s_channels[4] = {
	2,2,2,1 // stereo, joint stereo, dual, mono
};

struct mp3hdr {
	WORD sync;
	BYTE version;
	BYTE layer;
	DWORD bitrate;
	DWORD freq;
	BYTE channels;
	DWORD framesize;

	bool ExtractHeader(CSocket& socket) {
		BYTE buff[4];
		if (4 != socket.Receive(buff, 4, MSG_PEEK)) {
			return false;
		}

		sync = (buff[0]<<4)|(buff[1]>>4)|1;
		version = (buff[1]>>3)&3;
		layer = 4 - ((buff[1]>>1)&3);
		bitrate = s_bitrate[version&1][buff[2]>>4]*1000;
		freq = s_freq[version][(buff[2]>>2)&3];
		channels = s_channels[(buff[3]>>6)&3];
		framesize = freq ? ((((version&1)?144:72) * bitrate / freq) + ((buff[2]>>1)&1)) : 0;

		return (sync == 0xfff && layer == 3 && bitrate != 0 && freq != 0);
	}

	aachdr m_aachdr;

	bool ExtractAACHeader(CSocket& socket) {
		BYTE buff[ADTS_FRAME_SIZE];
		if (ADTS_FRAME_SIZE != socket.Receive(buff, ADTS_FRAME_SIZE, MSG_PEEK)) {
			return false;
		}

		CGolombBuffer gb(buff, ADTS_FRAME_SIZE);

		WORD sync = gb.BitRead(12);
		if (sync != 0xfff) {
			return false;
		}

		m_aachdr.sync = sync;
		m_aachdr.version = gb.BitRead(1);
		m_aachdr.layer = gb.BitRead(2);
		m_aachdr.fcrc = gb.BitRead(1);
		m_aachdr.profile = gb.BitRead(2);
		m_aachdr.freq = gb.BitRead(4);
		m_aachdr.privatebit = gb.BitRead(1);
		m_aachdr.channels = gb.BitRead(3);
		m_aachdr.original = gb.BitRead(1);
		m_aachdr.home = gb.BitRead(1);

		m_aachdr.copyright_id_bit = gb.BitRead(1);
		m_aachdr.copyright_id_start = gb.BitRead(1);
		m_aachdr.aac_frame_length = gb.BitRead(13);
		m_aachdr.adts_buffer_fullness = gb.BitRead(11);
		m_aachdr.no_raw_data_blocks_in_frame = gb.BitRead(2);

		if (m_aachdr.fcrc == 0) {
			m_aachdr.crc = (WORD)gb.BitRead(16);
		}

		if (m_aachdr.layer != 0 || m_aachdr.freq > 12 || m_aachdr.aac_frame_length <= (m_aachdr.fcrc == 0 ? 9 : 7) || !m_aachdr.channels) {
			return false;
		}

		m_aachdr.FrameSize = m_aachdr.aac_frame_length - (m_aachdr.fcrc == 0 ? 9 : 7);
		m_aachdr.nBytesPerSec = m_aachdr.aac_frame_length * aacfreq[m_aachdr.freq] / 1024; // ok?
		m_aachdr.rtDuration = 10000000i64 * 1024 / aacfreq[m_aachdr.freq]; // ok?

		return true;
	}
};

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] = {
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_MP3},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_RAW_AAC1},
};

const AMOVIESETUP_PIN sudOpPin[] = {
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesOut), sudPinTypesOut}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CShoutcastSource), ShoutcastSourceName, MERIT_NORMAL, _countof(sudOpPin), sudOpPin, CLSID_LegacyAmFilterCategory}
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CShoutcastSource>, NULL, &sudFilter[0]}
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
				if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
				{
					AfxMessageBox(_T("AfxWinInit failed!"));
					return FALSE;
				}
		*/
		if (!AfxSocketInit(NULL)) {
			AfxMessageBox(_T("AfxSocketInit failed!"));
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
	: CSource(NAME("CShoutcastSource"), lpunk, __uuidof(this))
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
	: CSourceStream(NAME("ShoutcastStream"), phr, pParent, L"Output")
	, m_fBuffering(false)
	, m_hSocket(INVALID_SOCKET)
{
	ASSERT(phr);

	*phr = S_OK;

	CString fn(wfn);
	if (fn.Find(_T("://")) < 0) {
		fn = _T("http://") + fn;
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
			DbgLog((LOG_TRACE, 3, L"CShoutcastStream(): failed connect for TimeOut!"));
		}

		m_socket.Close();

		if (!redirectUrl.IsEmpty() && redirectTry < 5) {
			redirectTry++;
			if (redirectUrl[1] == '/') {
				fn += redirectUrl;
			}
			fn = redirectUrl;
			DbgLog((LOG_TRACE, 3, L"CShoutcastStream(): redirect to \"%s\"", fn));
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
	LONGLONG ret = 100i64*(m_queue.GetTail()->rtStart - m_queue.GetHead()->rtStart) / AVGBUFFERLENGTH;
	return min(ret, 100);
}

CString CShoutcastStream::GetTitle()
{
	CAutoLock cAutoLock(&m_queue);
	return m_title;
}

CString CShoutcastStream::GetDescription()
{
	CAutoLock cAutoLock(&m_queue);
	return m_Description;
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

	BYTE* pData = NULL;
	if (FAILED(hr = pSample->GetPointer(&pData)) || !pData) {
		return S_FALSE;
	}

	do {
		// do we have to refill our buffer?
		{
			CAutoLock cAutoLock(&m_queue);
			if (!m_queue.IsEmpty() && m_queue.GetHead()->rtStart < m_queue.GetTail()->rtStart - MINBUFFERLENGTH) {
				break;    // nope, that's great
			}
		}

		TRACE(_T("CShoutcastStream(): START BUFFERING\n"));
		m_fBuffering = true;

		for (;;) {
			if (fExitThread) { // playback stopped?
				return S_FALSE;
			}

			Sleep(50);

			CAutoLock cAutoLock(&m_queue);
			if (!m_queue.IsEmpty() && m_queue.GetHead()->rtStart < m_queue.GetTail()->rtStart - AVGBUFFERLENGTH) {
				break;    // this is enough
			}
		}

		pSample->SetDiscontinuity(TRUE);

		DeliverBeginFlush();
		DeliverEndFlush();

		DeliverNewSegment(0, ~0, 1.0);

		TRACE(_T("CShoutcastStream(): END BUFFERING\n"));
		m_fBuffering = false;
	} while (false);

	{
		CAutoLock cAutoLock(&m_queue);
		ASSERT(!m_queue.IsEmpty());
		if (!m_queue.IsEmpty()) {
			CAutoPtr<CShoutCastPacket> p = m_queue.RemoveHead();
			DWORD len = min((DWORD)pSample->GetSize(), p->GetCount());
			memcpy(pData, p->GetData(), len);
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
		wfe->nSamplesPerSec		= m_socket.m_freq;
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
		wfe->nChannels			= m_socket.m_aachdr.channels <= 6 ? m_socket.m_aachdr.channels : 2;
		wfe->nSamplesPerSec		= aacfreq[m_socket.m_aachdr.freq];
		wfe->nBlockAlign		= m_socket.m_aachdr.aac_frame_length;
		wfe->nAvgBytesPerSec	= m_socket.m_aachdr.nBytesPerSec;
		wfe->cbSize				= MakeAACInitData((BYTE*)(wfe + 1), m_socket.m_aachdr.profile, wfe->nSamplesPerSec, wfe->nChannels);

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

#define MOVE_TO_MPA_START_CODE(b, e)	while(b <= e - MPA_FRAME_SIZE  && ((*(WORD*)b & 0xe0ff) != 0xe0ff)) b++;
#define MOVE_TO_AAC_START_CODE(b, e)	while(b <= e - ADTS_FRAME_SIZE && ((*(WORD*)b & 0xf0ff) != 0xf0ff)) b++;
UINT CShoutcastStream::SocketThreadProc()
{
	fExitThread = false;

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	AfxSocketInit();

	CShoutcastSocket soc;

	CAutoVectorPtr<BYTE> pData;
	size_t nSize = max(m_socket.m_metaint, MAXFRAMESIZE);
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
	m_Description	= soc.m_Description;

	REFERENCE_TIME m_rtSampleTime = 0;

	DWORD MinQueuePackets = max(10, min(MAXQUEUEPACKETS, AfxGetApp()->GetProfileInt(IDS_R_SETTINGS IDS_R_PERFOMANCE, IDS_RS_PERFOMANCE_MINQUEUEPACKETS, MINQUEUEPACKETS)));
	DWORD MaxQueuePackets = max(MinQueuePackets * 2, min(MAXQUEUEPACKETS * 10, AfxGetApp()->GetProfileInt(IDS_R_SETTINGS IDS_R_PERFOMANCE, IDS_RS_PERFOMANCE_MAXQUEUEPACKETS, MAXQUEUEPACKETS)));

	CAtlArray<BYTE> m_p;
	while (!fExitThread) {
		{
			if (m_queue.GetCount() >= MaxQueuePackets) {
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

		if (m_socket.m_Format == AUDIO_MPEG) {
			size_t nSize = m_p.GetCount();
			m_p.SetCount(nSize + len, 1024);
			memcpy(m_p.GetData() + nSize, pData, (size_t)len);

			if (m_p.GetCount() > MPA_FRAME_SIZE) {
				BYTE* start	= m_p.GetData();
				BYTE* end	= start + m_p.GetCount();

				for(;;) {
					MOVE_TO_MPA_START_CODE(start, end);
					if (start <= end - MPA_FRAME_SIZE) {
						audioframe_t aframe;
						int size = ParseMPAHeader(start, &aframe);
						if (size == 0) {
							start++;
							continue;
						}
						if (start + size > end) {
							break;
						}

						if (start + size + MPA_FRAME_SIZE <= end) {
							int size2 = ParseMPAHeader(start + size, &aframe);
							if (size2 == 0) {
								start++;
								continue;
							}
						}

						CAutoPtr<CShoutCastPacket> p2(DNew CShoutCastPacket());
						p2->SetData(start, size);
						p2->rtStop = (p2->rtStart = m_rtSampleTime) + (10000000i64 * size * 8/soc.m_bitrate);
						p2->title = !soc.m_title.IsEmpty() ? soc.m_title : soc.m_url;
						m_rtSampleTime = p2->rtStop;

						{
							CAutoLock cAutoLock(&m_queue);
							m_queue.AddTail(p2);
						}

						start += size;
					} else {
						break;
					}
				}

				if (start > m_p.GetData()) {
					m_p.RemoveAt(0, start - m_p.GetData());
				}
			}
		} else if (m_socket.m_Format == AUDIO_AAC) {
			size_t nSize = m_p.GetCount();
			m_p.SetCount(nSize + len, 1024);
			memcpy(m_p.GetData() + nSize, pData, (size_t)len);

			if (m_p.GetCount() > ADTS_FRAME_SIZE) {
				BYTE* start	= m_p.GetData();
				BYTE* end	= start + m_p.GetCount();

				for(;;) {
					MOVE_TO_AAC_START_CODE(start, end);
					if (start <= end - ADTS_FRAME_SIZE) {
						audioframe_t aframe;
						int size = ParseADTSAACHeader(start, &aframe);
						if (size == 0) {
							start++;
							continue;
						}
						if (start + size > end) {
							break;
						}

						if (start + size + ADTS_FRAME_SIZE <= end) {
							int size2 = ParseADTSAACHeader(start + size, &aframe);
							if (size2 == 0) {
								start++;
								continue;
							}
						}

						CAutoPtr<CShoutCastPacket> p2(DNew CShoutCastPacket());
						p2->SetData(start, size);
						p2->rtStop = (p2->rtStart = m_rtSampleTime) + (10000000i64 * size * 8/soc.m_bitrate);
						p2->title = !soc.m_title.IsEmpty() ? soc.m_title : soc.m_url;
						m_rtSampleTime = p2->rtStop;

						{
							CAutoLock cAutoLock(&m_queue);
							m_queue.AddTail(p2);
						}

						start += size;
					} else {
						break;
					}
				}

				if (start > m_p.GetData()) {
					m_p.RemoveAt(0, start - m_p.GetData());
				}
			}
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

static CString ConvertStr(LPCSTR S)
{
	CString str = AltUTF8To16(S);
	if (str.IsEmpty()) {
		str = ConvertToUTF16(S, CP_ACP); //Trying Local...
	}

	return str;
}

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
			CString str = ConvertStr((LPCSTR)buff);

			DbgLog((LOG_TRACE, 3, L"CShoutcastStream(): Metainfo: %s", str));

			CString title("StreamTitle='"), url("StreamUrl='");

			int i = str.Find(title);
			if (i >= 0) {
				i += title.GetLength();
				int j = str.Find(L"\';", i);
				if (!j) {
					j = str.ReverseFind('\'');
				}
				if (j > i) {
					m_title = str.Mid(i, j - i);
				}
			} else {
				DbgLog((LOG_TRACE, 3, L"CShoutcastStream(): StreamTitle is missing"));
			}

			i = str.Find(url);
			if (i >= 0) {
				i += url.GetLength();
				int j = str.Find(L"\';", i);
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

			TRACE(_T("CShoutcastStream(): !!!!!!!!!StreamTitle found inside mp3 data!!!!!!!!! offset=%d\n"), p - p0);
			TRACE(_T("resyncing...\n"));
			while (p-- > p0) {
				if ((BYTE)*p >= 0x20) {
					continue;
				}

				TRACE(_T("CShoutcastStream(): found possible length byte: %d, skipping %d bytes\n"), *p, 1 + *p*16);
				p += 1 + *p*16;
				len = (int)(p0 + len - p);
				TRACE(_T("CShoutcastStream(): returning the remaining bytes in the packet: %d\n"), len);
				if (len <= 0) {
					TRACE(_T("CShoutcastStream(): nothing to return, reading a bit more in\n"));
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

		BYTE cur = 0, prev = 0;

		CStringA hdr = GetHeader();
		DbgLog((LOG_TRACE, 3, L"\nCShoutcastSocket::Connect() - HTTP hdr:\n%s", ConvertStr(hdr)));

		CAtlList<CStringA> sl;
		Explode(hdr, sl, '\n');
		POSITION pos = sl.GetHeadPosition();
		while (pos) {
			CStringA& hdrline = sl.GetNext(pos);

			CStringA dup(hdrline);
			hdrline.MakeLower();
			if (hdrline.Find("200 ok") > 0 && (hdrline.Find("icy") == 0 || hdrline.Find("http/") == 0)) {
				fOK = true;
			} else if (hdrline.Left(13) == "content-type:") {
				hdrline = hdrline.Mid(13).Trim();
				if (hdrline.Find("audio/mpeg") == 0) {
					m_Format = AUDIO_MPEG;
					DbgLog((LOG_TRACE, 3, L"CShoutcastSocket::Connect() - detected MPEG Audio format"));
				} else if (hdrline.Find("audio/aacp") == 0) {
					m_Format = AUDIO_AAC;
					DbgLog((LOG_TRACE, 3, L"CShoutcastSocket::Connect() - detected AAC Audio format"));
				} else if (hdrline.Find("audio/x-scpls") == 0) {
					m_Format = AUDIO_PLAYLIST;
					DbgLog((LOG_TRACE, 3, L"CShoutcastSocket::Connect() - detected Playlist format"));
				}
			} else if (1 == sscanf_s(hdrline, "icy-br:%d", &m_bitrate)) {
				m_bitrate *= 1000;
			} else if (1 == sscanf_s(hdrline, "icy-metaint:%d", &metaint)) {
				metaint = metaint;
			} else if (hdrline.Left(9) == "icy-name:") {
				m_title = ConvertStr(dup.Mid(9).Trim());
			} else if (hdrline.Left(8) == "icy-url:") {
				m_url = dup.Mid(8).Trim();
			} else if (1 == sscanf_s(hdrline, "content-length:%d", &ContentLength)) {
				ContentLength = ContentLength;
			} else if (hdrline.Left(9) == "location:") {
				redirectUrl = hdrline.Mid(9).Trim();
			} else if (hdrline.Find("content-disposition:") >= 0 && hdrline.Find("filename=") > 0) {
				int pos = hdrline.Find("filename=");
				redirectUrl = _T("/") + CString(hdrline.Mid(pos + 9).Trim());
			} else if (hdrline.Left(16) == "icy-description:") {
				m_Description = ConvertStr(dup.Mid(16).Trim());
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
		if (m_Format == AUDIO_PLAYLIST && ContentLength) {

			char* buf = DNew char[ContentLength + 1];
			memset(buf, 0, ContentLength + 1);
			if (ContentLength == Receive((void*)buf, ContentLength)) {
				typedef CAtlRegExp<CAtlRECharTraits> CAtlRegExpT;
				typedef CAtlREMatchContext<CAtlRECharTraits> CAtlREMatchContextT;
				CString body(buf);
				CAutoPtr<CAtlRegExpT> re;

				re.Attach(DNew CAtlRegExp<>());
				if (re && REPARSE_ERROR_OK == re->Parse(_T("file\\z\\b*=\\b*[\"]*{[^\n\"]+}"), FALSE)) {
					CAtlREMatchContextT mc;
					const CAtlREMatchContextT::RECHAR* s = (LPCTSTR)body;
					const CAtlREMatchContextT::RECHAR* e = NULL;
					for (; s && re->Match(s, &mc, &e); s = e) {
						const CAtlREMatchContextT::RECHAR* szStart = 0;
						const CAtlREMatchContextT::RECHAR* szEnd = 0;
						mc.GetMatch(0, &szStart, &szEnd);

						redirectUrl.Format(_T("%.*s"), szEnd - szStart, szStart);
						redirectUrl.Trim();

						break;
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

	if (m_Format == AUDIO_NONE || m_Format == AUDIO_PLAYLIST) {
		return false; // not supported
	}

	m_freq = (DWORD)-1;
	m_channels = (DWORD)-1;

	BYTE b;

	if (m_Format == AUDIO_MPEG) {
		for (int i = MAXFRAMESIZE; i > 0; i--, Receive(&b, 1)) {
			mp3hdr h;
			if (h.ExtractHeader(*this)) {
				if (h.bitrate > 1) {
					m_bitrate = h.bitrate;
				}
				m_freq = h.freq;
				m_channels = h.channels;

				return true;
			}
		}
	} else if (m_Format == AUDIO_AAC) {
		for (int i = MAXFRAMESIZE; i > 0; i--, Receive(&b, 1)) {
			mp3hdr h;
			if (h.ExtractAACHeader(*this)) {
				m_freq = aacfreq[h.m_aachdr.freq];
				m_channels = h.m_aachdr.channels;
				m_aachdr = h.m_aachdr;

				return true;
			}
		}
	}

	return false;
}

/*
 * (C) 2013-2015 see Authors.txt
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
#include "../../../DSUtil/CUE.h"
#include <atlpath.h>
#include <MMReg.h>
#ifdef REGISTER_FILTER
#include <InitGuid.h>
#endif
#include <moreuuids.h>
#include "AudioSplitter.h"

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] = {
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_WAVE},
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] = {
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] = {
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesIn), sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, _countof(sudPinTypesOut), sudPinTypesOut}
};

const AMOVIESETUP_FILTER sudFilter[] = {
	{&__uuidof(CAudioSplitterFilter), AudioSplitterName, MERIT_NORMAL+1, _countof(sudpPins), sudpPins, CLSID_LegacyAmFilterCategory},
	{&__uuidof(CAudioSourceFilter), AudioSourceName, MERIT_NORMAL+1, 0, NULL, CLSID_LegacyAmFilterCategory},
};

CFactoryTemplate g_Templates[] = {
	{sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CAudioSplitterFilter>, NULL, &sudFilter[0]},
	{sudFilter[1].strName, sudFilter[1].clsID, CreateInstance<CAudioSourceFilter>, NULL, &sudFilter[1]},
};

int g_cTemplates = _countof(g_Templates);

STDAPI DllRegisterServer()
{
	CAtlList<CString>chkbytes;
	chkbytes.AddTail(_T("0,4,,4D414320"));               // 'MAC '
	chkbytes.AddTail(_T("0,4,,7442614B"));               // 'tBaK'
	chkbytes.AddTail(_T("0,4,,54544131"));               // 'TTA1'
	chkbytes.AddTail(_T("0,4,,7776706B"));               // 'wvpk'
	chkbytes.AddTail(_T("0,6,,2321414D520A"));           // '#!AMR\n'
	chkbytes.AddTail(_T("0,9,,2321414D522D57420A"));     // '#!AMR-WB\n'
	chkbytes.AddTail(_T("0,4,,52494646,8,4,,57415645")); // 'RIFF....WAVE'
	chkbytes.AddTail(_T("0,16,,726966662E91CF11A5D628DB04C10000,24,16,,77617665F3ACD3118CD100C04F8EDB8A")); // Wave64
	chkbytes.AddTail(_T("0,3,,494433"));                 // 'ID3' for TTA
	chkbytes.AddTail(_T("0,12,,445344201C00000000000000")); // 'DSD ...'
	chkbytes.AddTail(_T("0,4,,46524D38,12,4,,44534420")); // 'FRM8........DSD '

	RegisterSourceFilter(
		CLSID_AsyncReader,
		MEDIASUBTYPE_WAVE,
		chkbytes,
		NULL);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	UnRegisterSourceFilter(MEDIASUBTYPE_WAVE);

	return AMovieDllRegisterServer2(FALSE);
}

#include "../../filters/Filters.h"

CFilterApp theApp;

#endif

//
// CAudioSplitterFilter
//

CAudioSplitterFilter::CAudioSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CAudioSplitterFilter"), pUnk, phr, __uuidof(this))
	, m_pAudioFile(NULL)
	, m_rtime(0)
{
}

CAudioSplitterFilter::~CAudioSplitterFilter()
{
	SAFE_DELETE(m_pAudioFile);
}

STDMETHODIMP CAudioSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return
		__super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CAudioSplitterFilter::QueryFilterInfo(FILTER_INFO* pInfo)
{
	CheckPointer(pInfo, E_POINTER);
	ValidateReadWritePtr(pInfo, sizeof(FILTER_INFO));

	if (m_pName && m_pName[0]==L'M' && m_pName[1]==L'P' && m_pName[2]==L'C') {
		(void)StringCchCopyW(pInfo->achName, NUMELMS(pInfo->achName), m_pName);
	} else {
		wcscpy_s(pInfo->achName, AudioSourceName);
	}
	pInfo->pGraph = m_pGraph;
	if (m_pGraph) {
		m_pGraph->AddRef();
	}

	return S_OK;
}

HRESULT CAudioSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	m_pFile.Free();
	m_pFile.Attach(DNew CBaseSplitterFile(pAsyncReader, hr));
	if (!m_pFile) {
		return E_OUTOFMEMORY;
	}
	if (FAILED(hr)) {
		m_pFile.Free();
		return hr;
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = 0;

	m_pAudioFile = CAudioFile::CreateFilter(m_pFile);
	if (m_pAudioFile) {
		CMediaType mt;
		if (m_pAudioFile->SetMediaType(mt)) {
			m_rtDuration = m_pAudioFile->GetDuration();

			m_pAudioFile->SetProperties(this);

			CAtlArray<CMediaType> mts;
			mts.Add(mt);
			CAutoPtr<CBaseSplitterOutputPin> pPinOut(DNew CBaseSplitterOutputPin(mts, m_pAudioFile->GetName() + L" Audio Output", this, this, &hr));
			EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(0, pPinOut)));
		}
	}

	if (m_pOutputs.GetCount() == 0) {
		return E_FAIL;
	}

#if(0)
	// read cue file
	CPath path = GetPartFilename(pAsyncReader);
	path.RenameExtension(L".cue");
	if (path.FileExists()) {
		CFile cuefile(path, CFile::modeRead|CFile::shareDenyNone);

		if (cuefile.GetLength() <= 16384) {
			int size = (int)cuefile.GetLength();
			CStringA cuetextA;
			cuefile.Read(cuetextA.GetBufferSetLength(size), size);

			CStringW cuetextW = UTF8To16(cuetextA);
			CAtlList<Chapters> ChaptersList;
			CString sTitle, sPerformer;

			if (ParseCUESheet(cuetextW, ChaptersList, sTitle, sPerformer)) {
				if (CComQIPtr<IDSMPropertyBag> pPB = this) {
					if (sTitle.GetLength() > 0) {
						pPB->SetProperty(L"TITL", sTitle);
					}
					if (sPerformer.GetLength() > 0) {
						pPB->SetProperty(L"AUTH", sPerformer);
					}
				}

				if (CComQIPtr<IDSMChapterBag> pCB = this) {
					pCB->ChapRemoveAll();
					while (ChaptersList.GetCount()) {
						Chapters cp = ChaptersList.RemoveHead();
						pCB->ChapAppend(cp.rt, cp.name);
					}
				}
			}
		}
	}
#endif

	return S_OK;
}

bool CAudioSplitterFilter::DemuxInit()
{
	SetThreadName((DWORD)-1, "CAudioSplitterFilter");
	if (!m_pFile || !m_pAudioFile) {
		return false;
	}

	return true;
}

void CAudioSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	m_rtime = m_pAudioFile ? m_pAudioFile->Seek(rt) : 0;
}

bool CAudioSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	while (m_pAudioFile && SUCCEEDED(hr) && !CheckRequest(NULL)) {
		CAutoPtr<CPacket> p(DNew CPacket());
		p->bSyncPoint = TRUE;

		if (!m_pAudioFile->GetAudioFrame(p, m_rtime)) {
			break;
		}

		m_rtime = p->rtStop;

		hr = DeliverPacket(p);
	}

	return true;
}

//
// CAudioSourceFilter
//

CAudioSourceFilter::CAudioSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CAudioSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.Free();
}

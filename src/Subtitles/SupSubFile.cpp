/*
 * Copyright (C) 2012 Alexandr Vodiannikov aka "Aleksoid1978" (Aleksoid1978@mail.ru)
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
#include "SupSubFile.h"
#include "SubtitleHelpers.h"

CSupSubFile::CSupSubFile(CCritSec* pLock)
	: CSubPicProviderImpl(pLock)
	, m_Thread(NULL)
	, m_pSub(NULL)
{
}

CSupSubFile::~CSupSubFile()
{
	if (m_Thread) {
		if (WaitForSingleObject(m_Thread->m_hThread, 1000) == WAIT_TIMEOUT) {
			TerminateThread(m_Thread->m_hThread, 0xDEAD);
		}
	}

	if (m_pSub) {
		m_pSub->Reset();
		delete m_pSub;
	}
}

static UINT64 ReadByte(CFile* mfile, UINT count = 1)
{
	if (count <= 0 || count >= 64) {
		return 0;
	}
	UINT64 ret = 0;
	BYTE buf[64];
	if (mfile->Read(buf, count) != count) {
		return 0;
	}
	for(UINT i = 0; i < count; i++) {
		ret = (ret << 8) + (buf[i] & 0xff);
	}

	return ret;
}

static CString StripPath(CString path)
{
	CString p = path;
	p.Replace('\\', '/');
	p = p.Mid(p.ReverseFind('/') + 1);
	return (p.IsEmpty() ? path : p);
}

static UINT ThreadProc(LPVOID pParam)
{
	return (static_cast<CSupSubFile*>(pParam))->ThreadProc();
}

bool CSupSubFile::Open(CString fn, CString name/* = L""*/, CString videoName/* = L""*/)
{
	CFile f;
	if (!f.Open(fn, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone)) {
		return false;
	}

	WORD sync = (WORD)ReadByte(&f, 2);
	if (sync != 'PG') {
		return false;
	}

	m_fname			= fn;
    if (name.IsEmpty()) {
        m_Subname	= Subtitle::GuessSubtitleName(fn, videoName);
    } else {
        m_Subname	= name;
    }
	m_pSub			= DNew CHdmvSub();

	m_Thread = AfxBeginThread(::ThreadProc, static_cast<LPVOID>(this));

	return (m_Thread) ? true : false;
}

UINT CSupSubFile::ThreadProc()
{
	CFile f;
	if (!f.Open(m_fname, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone)) {
		return 1;
	}

	CAutoLock cAutoLock(&m_csCritSec);

	f.SeekToBegin();

	CMemFile m_sub;
	m_sub.SetLength(f.GetLength());
	m_sub.SeekToBegin();

	int len;
	BYTE buff[65536] = { 0 };
	while ((len = f.Read(buff, sizeof(buff))) > 0) {
		m_sub.Write(buff, len);
	}
	m_sub.SeekToBegin();

	WORD sync				= 0;
	USHORT size				= 0;
	REFERENCE_TIME rtStart	= 0;

	while (m_sub.GetPosition() < (m_sub.GetLength() - 10)) {
		sync = (WORD)ReadByte(&m_sub, 2);
		if (sync == 'PG') {
			rtStart = UINT64(ReadByte(&m_sub, 4) * 1000 / 9);
			m_sub.Seek(4 + 1, CFile::current);	// rtStop + Segment type
			size = ReadByte(&m_sub, 2) + 3;		// Segment size
			m_sub.Seek(-3, CFile::current);
			m_sub.Read(buff, size);
			m_pSub->ParseSample(buff, size, rtStart, 0);
		} else {
			break;
		}
	}

	m_sub.Close();

	return 0;
}

STDMETHODIMP CSupSubFile::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);
	*ppv = NULL;

	return
		QI(IPersist)
		QI(ISubStream)
		QI(ISubPicProvider)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// ISubPicProvider

STDMETHODIMP_(POSITION) CSupSubFile::GetStartPosition(REFERENCE_TIME rt, double fps, bool CleanOld)
{
	CAutoLock cAutoLock(&m_csCritSec);
	return m_pSub->GetStartPosition(rt, fps);
}

STDMETHODIMP_(POSITION) CSupSubFile::GetNext(POSITION pos)
{
	CAutoLock cAutoLock(&m_csCritSec);
	return m_pSub->GetNext(pos);
}

STDMETHODIMP_(REFERENCE_TIME) CSupSubFile::GetStart(POSITION pos, double fps)
{
	CAutoLock cAutoLock(&m_csCritSec);
	return m_pSub->GetStart(pos);
}

STDMETHODIMP_(REFERENCE_TIME) CSupSubFile::GetStop(POSITION pos, double fps)
{
	CAutoLock cAutoLock(&m_csCritSec);
	return m_pSub->GetStop(pos);
}

STDMETHODIMP_(bool) CSupSubFile::IsAnimated(POSITION pos)
{
	return false;
}

STDMETHODIMP CSupSubFile::Render(SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox)
{
	CAutoLock cAutoLock(&m_csCritSec);
	return m_pSub->Render(spd, rt, bbox);
}

STDMETHODIMP CSupSubFile::GetTextureSize(POSITION pos, SIZE& MaxTextureSize, SIZE& VideoSize, POINT& VideoTopLeft)
{
	CAutoLock cAutoLock(&m_csCritSec);
	return m_pSub->GetTextureSize(pos, MaxTextureSize, VideoSize, VideoTopLeft);
}

// IPersist

STDMETHODIMP CSupSubFile::GetClassID(CLSID* pClassID)
{
	return pClassID ? *pClassID = __uuidof(this), S_OK : E_POINTER;
}

// ISubStream

STDMETHODIMP_(int) CSupSubFile::GetStreamCount()
{
	return 1;
}

STDMETHODIMP CSupSubFile::GetStreamInfo(int iStream, WCHAR** ppName, LCID* pLCID)
{
	if (iStream != 0) {
		return E_INVALIDARG;
	}

	if (ppName) {
		*ppName = (WCHAR*)CoTaskMemAlloc((m_Subname.GetLength() + 1) * sizeof(WCHAR));
		if (!(*ppName)) {
			return E_OUTOFMEMORY;
		}

		wcscpy_s(*ppName, m_Subname.GetLength() + 1, CStringW(m_Subname));
	}

	if (pLCID) {
		*pLCID = 0;
	}

	return S_OK;
}

STDMETHODIMP_(int) CSupSubFile::GetStream()
{
	return 0;
}

STDMETHODIMP CSupSubFile::SetStream(int iStream)
{
	return iStream == 0 ? S_OK : E_FAIL;
}

STDMETHODIMP CSupSubFile::Reload()
{
	return S_OK;
}

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

#include "stdafx.h"
#include "BaseSplitterFile.h"
#include "../../../DSUtil/DSUtil.h"
#include "../apps/mplayerc/SettingsDefines.h"

//
// CBaseSplitterFile
//

CBaseSplitterFile::CBaseSplitterFile(IAsyncReader* pAsyncReader, HRESULT& hr, bool fRandomAccess, bool fStreaming, bool fStreamingDetect)
	: m_pAsyncReader(pAsyncReader)
	, m_fStreaming(false)
	, m_fRandomAccess(false)
	, m_pos(0), m_len(0)
	, m_bitbuff(0), m_bitlen(0)
	, m_cachepos(0), m_cachelen(0)
	, m_available(0)
	, m_hThread(NULL)
	, m_hThread_Duration(NULL)
	, m_evUpdate_Duration_Set(TRUE)
{
	if (!m_pAsyncReader) {
		hr = E_UNEXPECTED;
		return;
	}

	LONGLONG total = 0, available;
	hr = m_pAsyncReader->Length(&total, &available);

	m_fStreaming	= (total == 0 && available > 0);
	m_fRandomAccess	= (total > 0 && total == available);

	m_len		= total;
	m_available	= available;

	if (FAILED(hr) || (fRandomAccess && !m_fRandomAccess) || (!fStreaming && m_fStreaming) || m_len < 0) {
		hr = E_FAIL;
		return;
	}

	if (!m_fStreaming && fStreamingDetect) {
		m_evStop.Reset();
		DWORD ThreadId = 0;
		m_hThread = ::CreateThread(NULL, 0, StaticThreadProc, (LPVOID)this, CREATE_SUSPENDED, &ThreadId);
		UNREFERENCED_PARAMETER(ThreadId);
		SetThreadPriority(m_hThread, THREAD_PRIORITY_BELOW_NORMAL);
		ResumeThread(m_hThread);
	}

	size_t cachelen = AfxGetApp()->GetProfileInt(IDS_R_SETTINGS, IDS_RS_PERFOMANCE_CACHE_LENGTH, DEFAULT_CACHE_LENGTH);
	cachelen = CLAMP(cachelen, 16, 1024) * KILOBYTE;
	if (!SetCacheSize(cachelen)) {
		hr = E_OUTOFMEMORY;
		return;
	}

	hr = S_OK;
}

CBaseSplitterFile::~CBaseSplitterFile()
{
	if (m_hThread != NULL) {
		m_evStop.Set();
		if (WaitForSingleObject(m_hThread, 500) == WAIT_TIMEOUT) {
			TerminateThread(m_hThread, 0xDEAD);
		}
	}

	if (m_hThread_Duration != NULL) {
		m_evStop_Duration.Set();
		if (WaitForSingleObject(m_hThread_Duration, 1000) == WAIT_TIMEOUT) {
			TerminateThread(m_hThread_Duration, 0xDEAD);
		}
	}
}

DWORD WINAPI CBaseSplitterFile::StaticThreadProc(LPVOID lpParam)
{
	return ((CBaseSplitterFile*)lpParam)->ThreadProc();
}

DWORD CBaseSplitterFile::ThreadProc()
{
	HANDLE hEvts[] = {m_evStop};

	for(int i = 0; i < 20; i++) {
		DWORD dwObject = WaitForMultipleObjects(_countof(hEvts), hEvts, FALSE, 100);
		if (dwObject == WAIT_OBJECT_0) {
			return 0;
		} else {
			LONGLONG total, available = 0;
			m_pAsyncReader->Length(&total, &available);
			UNREFERENCED_PARAMETER(total);

			if (m_available && m_available < available) {
				ForceMode(Streaming);
				return 0;
			}

			m_available = available;
		}
	}

	return 0;
}

DWORD WINAPI CBaseSplitterFile::StaticThreadProc_Duration(LPVOID lpParam)
{
	return ((CBaseSplitterFile*)lpParam)->ThreadProc_Duration();
}

DWORD CBaseSplitterFile::ThreadProc_Duration()
{
	HANDLE hEvts[] = {m_evStop_Duration};
	m_evUpdate_Duration_Set.Set();

	for (;;) {
		DWORD dwObject = WaitForMultipleObjects(_countof(hEvts), hEvts, FALSE, 1000);
		if (dwObject == WAIT_OBJECT_0) {
			return 0;
		} else if (m_evUpdate_Duration_Set.Check()) {
			LONGLONG total, available = 0;
			m_pAsyncReader->Length(&total, &available);
			UNREFERENCED_PARAMETER(total);

			/*
			Do not remove these lines - leave for the future
			if (m_available && m_available < available) {
				m_evUpdate_Duration.Set();
			}
			*/

			m_len = m_available = available;
		}
	}

	return 0;
}

bool CBaseSplitterFile::SetCacheSize(size_t cachelen)
{
	m_pCache.Free();
	m_cachetotal = 0;
	m_pCache.Allocate(cachelen);
	if (!m_pCache) {
		return false;
	}
	m_cachetotal = cachelen;
	m_cachelen = 0;
	return true;
}

__int64 CBaseSplitterFile::GetPos()
{
	return m_pos - (m_bitlen>>3);
}

__int64 CBaseSplitterFile::GetAvailable()
{
	if (!m_fRandomAccess) {
		LONGLONG total;
		m_pAsyncReader->Length(&total, &m_available);
		UNREFERENCED_PARAMETER(total);
	}

	return m_available;
}

__int64 CBaseSplitterFile::GetLength(bool fUpdate)
{
	if (m_fStreaming) {
		m_len = GetAvailable();
	} else if (fUpdate) {
		LONGLONG total = 0, available;
		m_pAsyncReader->Length(&total, &available);
		m_len = total;
	}

	return m_len;
}

void CBaseSplitterFile::Seek(__int64 pos)
{
	__int64 len = GetLength();
	m_pos = min(max(pos, 0), len);
	BitFlush();
}

void CBaseSplitterFile::Skip(__int64 offset)
{
	ASSERT(offset >= 0);
	if (offset) {
		Seek(GetPos() + offset);
	}
}

HRESULT CBaseSplitterFile::Read(BYTE* pData, __int64 len)
{
	CheckPointer(m_pAsyncReader, E_NOINTERFACE);

	/*
	Do not remove these lines - leave for the future
	UpdateDuration();
	*/

	HRESULT hr = S_OK;

	if (m_cachetotal == 0 || !m_pCache) {
		hr = m_pAsyncReader->SyncRead(m_pos, (long)len, pData);
		m_pos += len;
		return hr;
	}

	BYTE* pCache = m_pCache;

	if (m_cachepos <= m_pos && m_pos < m_cachepos + m_cachelen) {
		__int64 minlen = min(len, m_cachelen - (m_pos - m_cachepos));

		memcpy(pData, &pCache[m_pos - m_cachepos], (size_t)minlen);

		len -= minlen;
		m_pos += minlen;
		pData += minlen;
	}

	while (len > m_cachetotal) {
		hr = m_pAsyncReader->SyncRead(m_pos, (long)m_cachetotal, pData);
		if (S_OK != hr) {
			return hr;
		}

		len -= m_cachetotal;
		m_pos += m_cachetotal;
		pData += m_cachetotal;
	}

	while (len > 0) {
		__int64 tmplen = GetLength();
		__int64 maxlen = min(tmplen - m_pos, m_cachetotal);
		__int64 minlen = min(len, maxlen);
		if (minlen <= 0) {
			return S_FALSE;
		}

		hr = m_pAsyncReader->SyncRead(m_pos, (long)maxlen, pCache);
		if (S_OK != hr) {
			return hr;
		}

		m_cachepos = m_pos;
		m_cachelen = maxlen;

		memcpy(pData, pCache, (size_t)minlen);

		len -= minlen;
		m_pos += minlen;
		pData += minlen;
	}

	return hr;
}

UINT64 CBaseSplitterFile::BitRead(int nBits, bool fPeek)
{
	ASSERT(nBits >= 0 && nBits <= 64);

	while (m_bitlen < nBits) {
		m_bitbuff <<= 8;
		if (S_OK != Read((BYTE*)&m_bitbuff, 1)) {
			return 0;   // EOF? // ASSERT(0);
		}
		m_bitlen += 8;
	}

	int bitlen = m_bitlen - nBits;

	UINT64 ret;
	// The shift to 64 bits can give incorrect results.
	// "The behavior is undefined if the right operand is negative, or greater than or equal to the length in bits of the promoted left operand."
	if (nBits == 64) {
		ret = m_bitbuff;
	} else {
		ret = (m_bitbuff >> bitlen) & ((1ui64 << nBits) - 1);
	}

	if (!fPeek) {
		m_bitbuff &= ((1ui64 << bitlen) - 1);
		m_bitlen = bitlen;
	}

	return ret;
}

void CBaseSplitterFile::BitByteAlign()
{
	m_bitlen &= ~7;
}

void CBaseSplitterFile::BitFlush()
{
	m_bitlen = 0;
}

HRESULT CBaseSplitterFile::ByteRead(BYTE* pData, __int64 len)
{
	Seek(GetPos());
	return Read(pData, len);
}

UINT64 CBaseSplitterFile::UExpGolombRead()
{
	int n = -1;
	for (BYTE b = 0; !b; n++) {
		b = (BYTE)BitRead(1);
	}
	return (1ui64 << n) - 1 + BitRead(n);
}

INT64 CBaseSplitterFile::SExpGolombRead()
{
	UINT64 k = UExpGolombRead();
	return ((k & 1) ? 1 : -1) * ((k + 1) >> 1);
}

HRESULT CBaseSplitterFile::HasMoreData(__int64 len, DWORD ms)
{
	__int64 available = GetLength() - GetPos();

	if (!m_fStreaming) {
		return available < 1 ? E_FAIL : S_OK;
	}

	if (available < len) {
		if (ms > 0) {
			Sleep(ms);
		}
		return S_FALSE;
	}

	return S_OK;
}

HRESULT CBaseSplitterFile::WaitAvailable(DWORD dwMilliseconds/* = 1500*/, __int64 AvailBytes/* = 1*/, HANDLE hBreak/* = NULL*/)
{
	if (m_fRandomAccess) {
		return HasMoreData(AvailBytes, dwMilliseconds);
	}

	if (GetAvailable() < GetLength()) {
		AvailBytes = min(AvailBytes, GetLength() - GetAvailable());
	}

	for (int i = 0; i < int(dwMilliseconds/100) && (!hBreak || WaitForSingleObject(hBreak, 0) != WAIT_OBJECT_0); i++) {
		if (GetRemaining() >= AvailBytes) {
			return S_OK;
		}
		Sleep(100);
	}

	if (GetRemaining() >= AvailBytes) {
		return S_OK;
	}

	return E_FAIL;
}

void CBaseSplitterFile::ForceMode(MODE mode) {
	if (mode == Streaming) {
		m_len = 0;

		m_fStreaming	= true;
		m_fRandomAccess	= false;
	} else if (mode == RandomAccess) {
		LONGLONG total = 0, available;
		m_pAsyncReader->Length(&total, &available);
		if (total < available) {
			total = available;
		}
		m_len = total;

		m_fStreaming	= false;
		m_fRandomAccess	= true;
	}
}

void CBaseSplitterFile::StartStreamingDetect()
{
	if (m_hThread_Duration == NULL) {
		m_evStop_Duration.Reset();
		DWORD ThreadId = 0;
		m_hThread_Duration = ::CreateThread(NULL, 0, StaticThreadProc_Duration, (LPVOID)this, CREATE_SUSPENDED, &ThreadId);
		UNREFERENCED_PARAMETER(ThreadId);
		SetThreadPriority(m_hThread_Duration, THREAD_PRIORITY_BELOW_NORMAL);
		ResumeThread(m_hThread_Duration);
	}
}

void CBaseSplitterFile::StopStreamingDetect()
{
	if (m_hThread_Duration != NULL) {
		m_evStop_Duration.Set();
	}
}

void CBaseSplitterFile::UpdateDuration()
{
	if (m_evUpdate_Duration.Check()) {
		m_evUpdate_Duration_Set.Reset();
		OnUpdateDuration();
		m_evUpdate_Duration_Set.Set();
	}
}
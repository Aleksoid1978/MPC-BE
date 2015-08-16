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

//
// CBaseSplitterFile
//

CBaseSplitterFile::CBaseSplitterFile(IAsyncReader* pAsyncReader, HRESULT& hr, bool fRandomAccess, bool fStreaming, bool fStreamingDetect)
	: m_pAsyncReader(pAsyncReader)
	, m_hThread(NULL)
{
	if (!m_pAsyncReader) {
		hr = E_UNEXPECTED;
		return;
	}

	LONGLONG total = 0, available = 0;
	hr = m_pAsyncReader->Length(&total, &available);
	if (FAILED(hr)) {
		return;
	}

	m_fStreaming	= (total == 0 && available > 0);
	m_fRandomAccess	= (total > 0 && total == available);

	if ((fRandomAccess && !m_fRandomAccess) || (!fStreaming && m_fStreaming) || total < 0) {
		hr = E_FAIL;
		return;
	}

	m_len		= m_fStreaming ? available : total;
	m_available	= available;

	if (m_fRandomAccess && !m_fStreaming && fStreamingDetect) {
		m_evStop.Reset();
		DWORD ThreadId = 0;
		m_hThread = ::CreateThread(NULL, 0, StaticThreadProc, (LPVOID)this, CREATE_SUSPENDED, &ThreadId);
		UNREFERENCED_PARAMETER(ThreadId);
		SetThreadPriority(m_hThread, THREAD_PRIORITY_BELOW_NORMAL);
		ResumeThread(m_hThread);
	}

	int cachelen = 64 * KILOBYTE; // TODO: specify when streaming
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
}

DWORD WINAPI CBaseSplitterFile::StaticThreadProc(LPVOID lpParam)
{
	return ((CBaseSplitterFile*)lpParam)->ThreadProc();
}

DWORD CBaseSplitterFile::ThreadProc()
{
	HANDLE hEvts[] = { m_evStop };

	for (int i = 0; i < 20; i++) {
		DWORD dwObject = WaitForMultipleObjects(_countof(hEvts), hEvts, FALSE, 100);
		if (dwObject == WAIT_OBJECT_0) {
			return 0;
		} else {
			LONGLONG total, available = 0;
			m_pAsyncReader->Length(&total, &available);
			if (m_available && m_available < available) {
				// local file whose size increases
				m_fStreaming = true;

				return 0;
			}
		}
	}

	return 0;
}

bool CBaseSplitterFile::SetCacheSize(int cachelen)
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

void CBaseSplitterFile::UpdateLength()
{
	if (m_fRandomAccess && !m_fStreaming) {
		return;
	}

	__int64 total = 0;
	__int64 available = 0;
	HRESULT hr = m_pAsyncReader->Length(&total, &available);

	m_available = available;

	if (m_fStreaming) {
		m_len = available;
	} else {
		m_len = total;
		if (total == available) {
			m_fRandomAccess = true;
		}
	}
}

HRESULT CBaseSplitterFile::WaitData(__int64 pos)
{
	UpdateLength();

	if (pos <= m_available) {
		return S_OK;
	}
	if (pos > m_len) {
		return E_FAIL;
	}

	int n = 0;
	while (pos > m_available) {
		__int64 available = m_available;
		Sleep(1000); // wait for 1 second until the data is loaded (TODO: specify size of the buffer when streaming)
		UpdateLength();
		
		if (available == m_available) {
			if (++n >= 10) {
				return E_FAIL;
			}
		} else {
			n = 0;
		}

		if (m_hBreak && WaitForSingleObject(m_hBreak, 0) == WAIT_OBJECT_0) {
			return E_FAIL; // 
		}
	}

	return S_OK;
}

__int64 CBaseSplitterFile::GetAvailable()
{
	UpdateLength();

	return m_available;
}

__int64 CBaseSplitterFile::GetLength()
{
	UpdateLength();

	return m_len;
}

void CBaseSplitterFile::Seek(__int64 pos)
{
	UpdateLength();

	m_pos = min(max(0, pos), m_len);
	BitFlush();
}

void CBaseSplitterFile::Skip(__int64 offset)
{
	ASSERT(offset >= 0);
	if (offset) {
		Seek(GetPos() + offset);
	}
}

HRESULT CBaseSplitterFile::Read(BYTE* pData, int len)
{
	CheckPointer(m_pAsyncReader, E_NOINTERFACE);

	HRESULT hr = WaitData(m_pos + len);
	if (S_OK != hr) {
		return hr;
	}

	if (m_cachetotal == 0 || !m_pCache) {
		hr = m_pAsyncReader->SyncRead(m_pos, len, pData);
		m_pos += len;
		return hr;
	}

	BYTE* pCache = m_pCache;

	if (m_cachepos <= m_pos && m_pos < m_cachepos + m_cachelen) {
		int minlen = min(len, m_cachelen - (int)(m_pos - m_cachepos));

		memcpy(pData, &pCache[m_pos - m_cachepos], minlen);

		len -= minlen;
		m_pos += minlen;
		pData += minlen;
	}

	while (len > m_cachetotal) {
		hr = m_pAsyncReader->SyncRead(m_pos, m_cachetotal, pData);
		if (S_OK != hr) {
			return hr;
		}

		len -= m_cachetotal;
		m_pos += m_cachetotal;
		pData += m_cachetotal;
	}

	while (len > 0) {
		__int64 tmplen = m_available - m_pos;
		int maxlen = min(tmplen, m_cachetotal);
		int minlen = min(len, maxlen);
		if (minlen <= 0) {
			return S_FALSE;
		}

		hr = m_pAsyncReader->SyncRead(m_pos, maxlen, pCache);
		if (S_OK != hr) {
			return hr;
		}

		m_cachepos = m_pos;
		m_cachelen = maxlen;

		memcpy(pData, pCache, minlen);

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
	return Read(pData, (int)len);
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

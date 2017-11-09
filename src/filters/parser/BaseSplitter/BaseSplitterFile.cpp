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
#include "BaseSplitterFile.h"

//
// CBaseSplitterFile
//

CBaseSplitterFile::CBaseSplitterFile(IAsyncReader* pAsyncReader, HRESULT& hr, int fmode)
	: m_pAsyncReader(pAsyncReader)
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

	if (!available && (fmode & FM_STREAM)) {
		DLog(L"BaseSplitter : no data available in streaming mode, wait and try again.");
		Sleep(500);
		hr = m_pAsyncReader->Length(&total, &available);
		if (FAILED(hr)) {
			return;
		}
	}

	if (total > 0 && total == available) {
		m_fmode = FM_FILE;
	}
	else if (total > 0 && available < total) {
		m_fmode = FM_FILE_DL;
	}
	else if (total == 0 && available > 0) {
		m_fmode = FM_STREAM;
	}

	if (!(m_fmode & (fmode^FM_FILE_VAR))) {
		hr = E_FAIL;
		return;
	}

	m_len		= m_fmode == FM_STREAM ? available : total;
	m_available	= available;

	int cachelen = 64 * KILOBYTE; // TODO: specify when streaming
	if (!SetCacheSize(cachelen)) {
		hr = E_OUTOFMEMORY;
		return;
	}

	if (m_fmode == FM_FILE_DL
			|| m_fmode == FM_STREAM
			|| (m_fmode == FM_FILE && (fmode & FM_FILE_VAR))) {
		m_evStopThreadLength.Reset();
		m_ThreadLength = std::thread([this] { ThreadUpdateLength(); });
		::SetThreadPriority(m_ThreadLength.native_handle(), THREAD_PRIORITY_LOWEST);
	}

	m_pSyncReader = m_pAsyncReader;

	hr = S_OK;
}

CBaseSplitterFile::~CBaseSplitterFile()
{
	if (m_ThreadLength.joinable()) {
		m_evStopThreadLength.Set();
		m_ThreadLength.join();
	}
}

HRESULT CBaseSplitterFile::Refresh()
{
	CheckPointer(m_pAsyncReader, E_FAIL);

	LONGLONG total = 0, available = 0;
	HRESULT hr = m_pAsyncReader->Length(&total, &available);
	if (SUCCEEDED(hr)) {
		m_len = total;
		m_available = available;
	}

	return hr;
}

void CBaseSplitterFile::ThreadUpdateLength()
{
	HANDLE hEvts[] = { m_evStopThreadLength };

	int count = 0;
	for (;;) {
		DWORD dwObject = WaitForMultipleObjects(1, hEvts, FALSE, 1000);
		if (dwObject == WAIT_OBJECT_0
				|| (m_fmode == FM_FILE && count >= 10)) {
			return;
		} else {
			LONGLONG total, available = 0;
			m_pAsyncReader->Length(&total, &available);
			if (m_fmode & (FM_FILE_VAR | FM_STREAM)) {
				m_len = available;
			} else if (m_fmode == FM_FILE) {
				if (m_available && m_available < available) {
					m_fmode = FM_FILE_VAR;
				}
				count++;
			} else {
				m_len = total;
				if (total == available) {
					m_fmode = FM_FILE;
					m_available = available;
					return;
				}
			}

			m_available = available;
		}
	}

	return;
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
	return m_pos - (m_bitlen >> 3);
}

bool CBaseSplitterFile::WaitData(__int64 pos)
{
	int n = 0;
	while (pos > m_available) {
		__int64 available = m_available;
		Sleep(1000); // wait for 1 second until the data is loaded (TODO: specify size of the buffer when streaming)

		if (available == m_available) {
			if (++n >= 10) {
				DLog(L"BaseSplitter : no new data more than 10 seconds, most likely loss of connection.");
				m_bConnectionLost = true;
				return false;
			}
		} else {
			n = 0;
		}

		if (m_hBreak && WaitForSingleObject(m_hBreak, 0) == WAIT_OBJECT_0) {
			return false;
		}
	}

	return true;
}

__int64 CBaseSplitterFile::GetAvailable()
{
	return m_available;
}

__int64 CBaseSplitterFile::GetLength()
{
	return m_len;
}

__int64 CBaseSplitterFile::GetRemaining()
{
	return m_bConnectionLost ? 0 : m_len - GetPos();
}

void CBaseSplitterFile::Seek(__int64 pos)
{
	if (m_pos != pos) {
		if (IsStreaming()) {
			m_pos = pos;
		} else {
			m_pos = clamp(pos, 0LL, m_len);
		}
	}

	BitFlush();
}

void CBaseSplitterFile::Skip(__int64 offset)
{
	ASSERT(offset >= 0);
	if (offset > 0) {
		Seek(GetPos() + offset);
	}
}

HRESULT CBaseSplitterFile::SyncRead(BYTE* pData, int& len)
{
	HRESULT hr = m_pAsyncReader->SyncRead(m_pos, len, pData);
	if (FAILED(hr)) {
		return hr;
	}

	if (hr == S_FALSE && IsStreaming()) {
		DLog(L"CBaseSplitterFile::SyncRead() - we reached the end of data (pos: %I64d), but the size of the data changes, trying reading manually", m_pos);

		int read = 0;
		__int64 pos = m_pos;
		do {
			hr = m_pAsyncReader->SyncRead(pos++, 1, pData + read);
		} while (hr == S_OK && (++read) < len);
		DLog(L"    -> Read %d bytes", read);

		len = read;
	}

	return hr;
}

#define Exit(hr) { m_hrLastReadError = hr; return hr; }
HRESULT CBaseSplitterFile::Read(BYTE* pData, int len)
{
	CheckPointer(m_pAsyncReader, E_NOINTERFACE);

	__int64 new_pos = m_pos + len;
	if (!IsStreaming() && (new_pos > m_len || m_bConnectionLost)) {
		Exit(E_FAIL);
	}
	if (m_fmode == FM_FILE_DL && new_pos > m_available && !WaitData(new_pos)) {
		Exit(E_FAIL);
	}

	HRESULT hr = S_OK;
	if (m_cachetotal == 0 || !m_pCache) {
		hr = SyncRead(pData, len);
		m_pos += len;
		Exit(hr);
	}

	BYTE* pCache = m_pCache;

	if (m_cachepos <= m_pos && m_pos < m_cachepos + m_cachelen) {
		int minlen = std::min(len, m_cachelen - (int)(m_pos - m_cachepos));

		memcpy(pData, &pCache[m_pos - m_cachepos], minlen);

		len -= minlen;
		m_pos += minlen;
		pData += minlen;
	}

	while (len > m_cachetotal) {
		hr = SyncRead(pData, m_cachetotal);
		if (S_OK != hr) {
			Exit(hr);
		}

		len -= m_cachetotal;
		m_pos += m_cachetotal;
		pData += m_cachetotal;
	}

	while (len > 0) {
		const __int64 tmplen = IsStreaming() ? m_cachetotal: m_available - m_pos;
		int maxlen = (int)std::min(tmplen, (__int64)m_cachetotal);
		int minlen = std::min(len, maxlen);
		if (minlen <= 0) {
			Exit(S_FALSE);
		}

		hr = SyncRead(pCache, maxlen);
		if (S_OK != hr) {
			Exit(hr);
		}

		m_cachepos = m_pos;
		m_cachelen = maxlen;

		memcpy(pData, pCache, minlen);

		len -= minlen;
		m_pos += minlen;
		pData += minlen;
	}

	Exit(hr);
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

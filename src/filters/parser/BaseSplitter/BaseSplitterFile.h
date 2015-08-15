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

#pragma once

#include <atlcoll.h>

class CBaseSplitterFile
{
	CComPtr<IAsyncReader> m_pAsyncReader;
	CAutoVectorPtr<BYTE> m_pCache;
	__int64 m_cachepos, m_cachelen, m_cachetotal;
	UINT64 m_bitbuff;
	int m_bitlen;

	bool m_fStreaming, m_fRandomAccess;
	__int64 m_pos, m_len;
	__int64 m_available;

	DWORD m_lentick_prev;
	DWORD m_lentick_actual;

	HRESULT UpdateLength();
	HRESULT WaitData(__int64 pos);

	virtual HRESULT Read(BYTE* pData, __int64 len); // use ByteRead
	virtual void OnUpdateDuration() {};

protected:
	DWORD ThreadProc();
	static DWORD WINAPI StaticThreadProc(LPVOID lpParam);
	HANDLE m_hThread;
	CAMEvent m_evStop;

	DWORD ThreadProc_Duration();
	static DWORD WINAPI StaticThreadProc_Duration(LPVOID lpParam);
	HANDLE m_hThread_Duration;
	CAMEvent m_evStop_Duration;
	CAMEvent m_evUpdate_Duration;
	CAMEvent m_evUpdate_Duration_Set;

public:
	CBaseSplitterFile(IAsyncReader* pReader, HRESULT& hr, bool fRandomAccess = true, bool fStreaming = false, bool fStreamingDetect = false);
	~CBaseSplitterFile();

	bool SetCacheSize(size_t cachelen);

	__int64 GetPos();
	__int64 GetAvailable();
	__int64 GetLength();
	__int64 GetRemaining() {
		return GetLength() - GetPos();
	}
	virtual void Seek(__int64 pos);
	void Skip(__int64 offset);

	UINT64 UExpGolombRead();
	INT64 SExpGolombRead();

	UINT64 BitRead(int nBits, bool fPeek = false);
	void BitByteAlign(), BitFlush();
	HRESULT ByteRead(BYTE* pData, __int64 len);

	bool IsStreaming()		const {
		return m_fStreaming;
	}
	bool IsRandomAccess()	const {
		return m_fRandomAccess;
	}

	enum MODE {
		Streaming,
		RandomAccess,
	};
	void ForceMode(MODE mode);

	void StartStreamingDetect();
	void StopStreamingDetect();
	void UpdateDuration();
};

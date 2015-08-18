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

#define FM_FILE     1 // complete file or stream of known size (local file, VTS Reader, File Source (Async.))
#define FM_FILE_DL  2 // downloading stream of known size (File Source (URL) for files < 4 GB)
#define FM_FILE_VAR 4 // local file whose size increases
#define FM_STREAM   8 // downloading stream of unknown size

class CBaseSplitterFile
{
	CComPtr<IAsyncReader> m_pAsyncReader;
	__int64 m_pos             = 0;
	__int64 m_len             = 0;
	__int64 m_available       = 0;
	bool    m_bConnectionLost = false;

	CAutoVectorPtr<BYTE> m_pCache;
	__int64 m_cachepos        = 0;
	int     m_cachelen        = 0;
	int     m_cachetotal      = 0;
	UINT64  m_bitbuff         = 0;
	int     m_bitlen          = 0;

	int     m_fmode           = 0;

	HANDLE  m_hBreak          = NULL;

	void UpdateLength();
	bool WaitData(__int64 pos);

	HRESULT Read(BYTE* pData, int len);

	// thread to determine the local file whose size increases
	DWORD ThreadProc();
	static DWORD WINAPI StaticThreadProc(LPVOID lpParam);
	HANDLE m_hThread;
	CAMEvent m_evStop;

public:
	CBaseSplitterFile(IAsyncReader* pReader, HRESULT& hr, int fmode = FM_FILE);
	~CBaseSplitterFile();

	bool SetCacheSize(int cachelen);

	__int64 GetPos();
	__int64 GetAvailable();
	__int64 GetLength();
	__int64 GetRemaining();

	virtual void Seek(__int64 pos);
	void Skip(__int64 offset);

	UINT64 UExpGolombRead();
	INT64 SExpGolombRead();

	void BitByteAlign();
	void BitFlush();
	UINT64 BitRead(int nBits, bool fPeek = false);
	HRESULT ByteRead(BYTE* pData, __int64 len);

	bool IsStreaming() const { return m_fmode == FM_STREAM; }
	bool IsRandomAccess() const { return m_fmode == FM_FILE || m_fmode == FM_FILE_VAR; }
	bool IsVariableSize() const { return m_fmode == FM_FILE_VAR; }

	void SetBreakHandle(HANDLE hBreak) { m_hBreak = hBreak; }
};

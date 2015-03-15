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
#include "..\DSUtil\SysVersion.h"
#include "..\DSUtil\GolombBuffer.h"
#include <math.h>
#include <zlib/zlib.h>
#include <Audiopolicy.h>
#include <Mmdeviceapi.h>
#include "ShockwaveGraph.h"

using namespace DSObjects;


CShockwaveGraph::CShockwaveGraph(HWND hParent, HRESULT& hr)
	: m_fs(State_Stopped)
	, vsize(0, 0)
	, m_fInitialVolume(1)
{
	hr = S_OK;

	if (!m_wndWindowFrame.Create(NULL, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
								 CRect(0, 0, 0, 0), CWnd::FromHandle(hParent), 0, NULL)) {
		hr = E_FAIL;
		return;
	}

	if (!m_wndDestFrame.Create(NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
							   CRect(0, 0, 0, 0), &m_wndWindowFrame, 0)) {
		hr = E_FAIL;
		return;
	}
	m_wndDestFrame.put_BackgroundColor(0);

	if (IsWinVistaOrLater()) {
		CComPtr<IMMDeviceEnumerator> pDeviceEnumerator;
		if (SUCCEEDED(pDeviceEnumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator)))) {
			CComPtr<IMMDevice> pDevice;
			if (SUCCEEDED(pDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice))) {
				CComPtr<IAudioSessionManager> pSessionManager;
				if (SUCCEEDED(pDevice->Activate(__uuidof(IAudioSessionManager), CLSCTX_ALL, nullptr,
												reinterpret_cast<void**>(&pSessionManager)))) {
					if (SUCCEEDED(pSessionManager->GetSimpleAudioVolume(nullptr, FALSE, &m_pSimpleVolume))) {
						VERIFY(SUCCEEDED(m_pSimpleVolume->GetMasterVolume(&m_fInitialVolume)));
					}
				}
			}
		}
		ASSERT(m_pSimpleVolume);
	}
}

CShockwaveGraph::~CShockwaveGraph()
{
	m_wndDestFrame.DestroyWindow();
	m_wndWindowFrame.DestroyWindow();
	if (m_pSimpleVolume) {
		VERIFY(SUCCEEDED(m_pSimpleVolume->SetMasterVolume(m_fInitialVolume, nullptr)));
	}
}

static DWORD ReadBuffer(HANDLE hFile, BYTE* pBuff, DWORD nLen)
{
	DWORD dwRead;
	ReadFile(hFile, pBuff, nLen, &dwRead, NULL);

	return dwRead;
}

// IGraphBuilder
STDMETHODIMP CShockwaveGraph::RenderFile(LPCWSTR lpcwstrFile, LPCWSTR lpcwstrPlayList)
{
	try {
		m_wndDestFrame.LoadMovie(0, CString(lpcwstrFile));
	} catch (CException* e) {
		e->Delete();
		return E_FAIL;
	}

	if (!::PathIsURL(lpcwstrFile)) {
		// handle only local files
		HANDLE m_hFile = CreateFile(lpcwstrFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
												 OPEN_EXISTING, FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

		if (m_hFile != INVALID_HANDLE_VALUE) {
			BYTE Buff[128] = {0};
			ReadBuffer(m_hFile, Buff, 3);	// Signature
			if (memcmp(Buff, "CWS", 3) == 0 || memcmp(Buff, "FWS", 3) == 0) {
				CGolombBuffer gb(NULL, 0);

				LARGE_INTEGER size = {0};
				GetFileSizeEx(m_hFile, &size);

				BYTE ver = 0;
				ReadBuffer(m_hFile, &ver, 1);
				DWORD flen = 0;
				ReadBuffer(m_hFile, (BYTE*)(&flen), sizeof(flen));
				flen -= 8;

				CAtlArray<BYTE> DecompData;

				if (memcmp(Buff, "CWS", 3) == 0) {
					if (size.QuadPart < 5 * MEGABYTE) {

						DecompData.SetCount(size.QuadPart - 8);
						DWORD size = ReadBuffer(m_hFile, DecompData.GetData(), DecompData.GetCount());

						if (size == DecompData.GetCount()) {
							// decompress
							for (;;) {
								int res;
								z_stream d_stream;

								d_stream.zalloc	= (alloc_func)NULL;
								d_stream.zfree	= (free_func)NULL;
								d_stream.opaque	= (voidpf)NULL;

								if (Z_OK != (res = inflateInit(&d_stream))) {
									DecompData.RemoveAll();
									break;
								}

								d_stream.next_in	= DecompData.GetData();
								d_stream.avail_in	= (uInt)DecompData.GetCount();

								BYTE* dst = NULL;
								int n = 0;
								do {
									dst = (BYTE*)realloc(dst, ++n * 1000);
									d_stream.next_out	= &dst[(n - 1) * 1000];
									d_stream.avail_out	= 1000;
									if (Z_OK != (res = inflate(&d_stream, Z_NO_FLUSH)) && Z_STREAM_END != res) {
										DecompData.RemoveAll();
										free(dst);
										break;
									}
								} while (0 == d_stream.avail_out && 0 != d_stream.avail_in && Z_STREAM_END != res);

								inflateEnd(&d_stream);

								DecompData.SetCount(d_stream.total_out);
								memcpy(DecompData.GetData(), dst, DecompData.GetCount());

								free(dst);

								break;
							}
						}

						if (flen == DecompData.GetCount()) {
							gb.Reset(DecompData.GetData(), min(_countof(Buff), DecompData.GetCount()));
						}
					}

				} else if (memcmp(Buff, "FWS", 3) == 0) {
					DWORD dwRead = ReadBuffer(m_hFile, Buff, min(_countof(Buff), size.QuadPart));
					if (dwRead) {
						gb.Reset(Buff, dwRead);
					}
				}

				if (gb.GetSize() > 1) {
					int Nbits	= (int)gb.BitRead(5);
					UINT64 Xmin = gb.BitRead(Nbits);
					UINT64 Xmax = gb.BitRead(Nbits);
					UINT64 Ymin = gb.BitRead(Nbits);
					UINT64 Ymax = gb.BitRead(Nbits);

					vsize = CSize((Xmax-Xmin)/20, (Ymax-Ymin)/20);
				}
			}

			CloseHandle(m_hFile);
		}
	}

	// do not trust this value :)
	if (vsize.cx == 0 || vsize.cy == 0) {
		vsize.cx = m_wndDestFrame.TGetPropertyAsNumber(L"/", 8);
		vsize.cy = m_wndDestFrame.TGetPropertyAsNumber(L"/", 9);
	}

	// default value ...
	if (vsize.cx == 0 || vsize.cy == 0) {
		vsize.cx = 640;
		vsize.cy = 480;
	}

	return S_OK;
}

// IMediaControl
STDMETHODIMP CShockwaveGraph::Run()
{
	try {
		if (m_fs != State_Running) {
			m_wndDestFrame.Play();
		}
	} catch (CException* e) {
		e->Delete();
		return E_FAIL;
	}
	m_fs = State_Running;
	m_wndWindowFrame.EnableWindow();
	// m_wndDestFrame.EnableWindow();

	return S_OK;
}

STDMETHODIMP CShockwaveGraph::Pause()
{
	try {
		if (m_fs == State_Running) {
			m_wndDestFrame.Stop();
		}
	} catch (CException* e) {
		e->Delete();
		return E_FAIL;
	}

	m_fs = State_Paused;
	return S_OK;
}

STDMETHODIMP CShockwaveGraph::Stop()
{
	try {
		m_wndDestFrame.Stop();
	} catch (CException* e) {
		e->Delete();
		return E_FAIL;
	}

	m_fs = State_Stopped;
	return S_OK;
}

STDMETHODIMP CShockwaveGraph::GetState(LONG msTimeout, OAFilterState* pfs)
{
	OAFilterState fs = m_fs;

	try {
		if (m_wndDestFrame.IsPlaying() && m_fs == State_Stopped) {
			m_fs = State_Running;
		} else if (!m_wndDestFrame.IsPlaying() && m_fs == State_Running) {
			m_fs = State_Stopped;
		}
		fs = m_fs;
	} catch (CException* e) {
		e->Delete();
		return E_FAIL;
	}

	return pfs ? *pfs = fs, S_OK : E_POINTER;
}

// IMediaSeeking
STDMETHODIMP CShockwaveGraph::IsFormatSupported(const GUID* pFormat)
{
	return !pFormat ? E_POINTER : *pFormat == TIME_FORMAT_FRAME ? S_OK : S_FALSE;
}

STDMETHODIMP CShockwaveGraph::GetTimeFormat(GUID* pFormat)
{
	return pFormat ? *pFormat = TIME_FORMAT_FRAME, S_OK : E_POINTER;
}

STDMETHODIMP CShockwaveGraph::GetDuration(LONGLONG* pDuration)
{
	CheckPointer(pDuration, E_POINTER);

	*pDuration = 0;
	try {
		if (m_wndDestFrame.get_ReadyState() >= READYSTATE_COMPLETE) {
			*pDuration = m_wndDestFrame.get_TotalFrames();
		}
	} catch (CException* e) {
		e->Delete();
		return E_FAIL;
	}

	return S_OK;
}

STDMETHODIMP CShockwaveGraph::GetCurrentPosition(LONGLONG* pCurrent)
{
	CheckPointer(pCurrent, E_POINTER);

	*pCurrent = 0;
	try {
		if (m_wndDestFrame.get_ReadyState() >= READYSTATE_COMPLETE) {
			*pCurrent = m_wndDestFrame.get_FrameNum();
		}
	} catch (CException* e) {
		e->Delete();
		return E_FAIL;
	}

	return S_OK;
}

STDMETHODIMP CShockwaveGraph::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	if (dwCurrentFlags&AM_SEEKING_AbsolutePositioning) {
		m_wndDestFrame.put_FrameNum(*pCurrent);

		if (m_fs == State_Running && !m_wndDestFrame.IsPlaying()) {
			m_wndDestFrame.Play();
		} else if ((m_fs == State_Paused || m_fs == State_Stopped) && m_wndDestFrame.IsPlaying()) {
			m_wndDestFrame.Stop();
		}

		m_wndDestFrame.put_Quality(1); // 0=Low, 1=High, 2=AutoLow, 3=AutoHigh

		return S_OK;
	}

	return E_INVALIDARG;
}

// IVideoWindow
STDMETHODIMP CShockwaveGraph::put_Visible(long Visible)
{
	if (IsWindow(m_wndDestFrame.m_hWnd)) {
		m_wndDestFrame.ShowWindow(Visible == OATRUE ? SW_SHOWNORMAL : SW_HIDE);
	}

	return S_OK;
}

STDMETHODIMP CShockwaveGraph::get_Visible(long* pVisible)
{
	return pVisible ? *pVisible = (m_wndDestFrame.IsWindowVisible() ? OATRUE : OAFALSE), S_OK : E_POINTER;
}

STDMETHODIMP CShockwaveGraph::SetWindowPosition(long Left, long Top, long Width, long Height)
{
	if (IsWindow(m_wndWindowFrame.m_hWnd)) {
		m_wndWindowFrame.MoveWindow(Left, Top, Width, Height);
	}

	return S_OK;
}

// IBasicVideo
STDMETHODIMP CShockwaveGraph::SetDestinationPosition(long Left, long Top, long Width, long Height)// {return E_NOTIMPL;}
{
	if (IsWindow(m_wndDestFrame.m_hWnd)) {
		m_wndDestFrame.MoveWindow(Left, Top, Width, Height);
	}

	return S_OK;
}

STDMETHODIMP CShockwaveGraph::GetVideoSize(long* pWidth, long* pHeight)
{
	CheckPointer(pWidth, E_POINTER);
	CheckPointer(pHeight, E_POINTER);

	*pWidth		= vsize.cx;
	*pHeight	= vsize.cy;

	CRect r;
	m_wndWindowFrame.GetWindowRect(r);
	if (r.IsRectEmpty()) {
		NotifyEvent(EC_BG_AUDIO_CHANGED, 2, 0);
	}

	return S_OK;
}

// IBasicAudio
STDMETHODIMP CShockwaveGraph::put_Volume(long lVolume)
{
	HRESULT hr = S_OK;

	if (m_pSimpleVolume) {
		float fVolume = (lVolume <= -10000) ? 0 : (lVolume >= 0) ? 1 : pow(10.f, lVolume / 4000.f);
		hr = m_pSimpleVolume->SetMasterVolume(fVolume, nullptr);
	} else {
		lVolume = (lVolume <= -10000) ? 0 : (long)(pow(10.0, lVolume / 4000.0) * 100);
		lVolume = lVolume * 0x10000 / 100;
		lVolume = max(min(lVolume, 0xffff), 0);
		waveOutSetVolume(0, (lVolume << 16) | lVolume);
	}

	return hr;
}

STDMETHODIMP CShockwaveGraph::get_Volume(long* plVolume)
{
	CheckPointer(plVolume, E_POINTER);
	HRESULT hr = S_OK;

	if (m_pSimpleVolume) {
		float fVolume;
		hr = m_pSimpleVolume->GetMasterVolume(&fVolume);
		if (SUCCEEDED(hr)) {
			*plVolume = (fVolume == 0) ? -10000 : long(4000 * log10(fVolume));
		}
	} else {
		waveOutGetVolume(0, (DWORD*)plVolume);
		*plVolume = (*plVolume & 0xffff + ((*plVolume >> 16) & 0xffff)) / 2 * 100 / 0x10000;
		if (*plVolume > 0) {
			*plVolume = min((long)(4000 * log10(*plVolume / 100.0f)), 0);
		}
		else {
			*plVolume = -10000;
		}
	}

	return hr;
}

// IAMOpenProgress
STDMETHODIMP CShockwaveGraph::QueryProgress(LONGLONG* pllTotal, LONGLONG* pllCurrent)
{
	CheckPointer(pllTotal, E_POINTER);
	CheckPointer(pllCurrent, E_POINTER);

	*pllTotal = 100;
	*pllCurrent = m_wndDestFrame.PercentLoaded();

	return S_OK;
}

// IGraphEngine
STDMETHODIMP_(engine_t) CShockwaveGraph::GetEngine()
{
	return ShockWave;
}

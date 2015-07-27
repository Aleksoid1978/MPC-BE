/*
 * (C) 2015 see Authors.txt
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

#include "stdafx.h"
#include "../../../../DSUtil/DSUtil.h"
#include <stdint.h>
#include <ffmpeg/libavcodec/dxva_internal.h>

#define DBGLOG_LEVEL 0

#if defined(_DEBUG) && DBGLOG_LEVEL > 0
	#define CHECK_HR_FALSE(x)	hr = ##x; if (FAILED(hr)) { DbgLog((LOG_TRACE, 3, L"DXVA Error : 0x%08x, %s : %i", hr, CString(__FILE__), __LINE__)); return S_FALSE; }
#else
	#define CHECK_HR_FALSE(x)	hr = ##x; if (FAILED(hr)) return S_FALSE;
#endif

#define MAX_RETRY_ON_PENDING		50
#define DO_DXVA_PENDING_LOOP(x)		nTry = 0; \
									while (FAILED(hr = x) && nTry < MAX_RETRY_ON_PENDING) \
									{ \
										if (hr != E_PENDING) break; \
										Sleep(3); \
										nTry++; \
									}

class CMPCVideoDecFilter;
struct AVFrame;

class CDXVADecoder
{
public:
	virtual			~CDXVADecoder() {};

	virtual void	Flush() { m_dxva_context.report_id = 0; };
	virtual HRESULT	DecodeFrame(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop) { return S_OK; };
	virtual HRESULT	DeliverFrame(int got_picture, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop) { return S_OK; };

protected:
	UINT			m_nFieldNum;
	dxva_context	m_dxva_context;
};

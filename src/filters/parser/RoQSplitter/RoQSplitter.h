/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2016 see Authors.txt
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

#include "../BaseSplitter/BaseSplitter.h"

#define RoQSplitterName			L"MPC RoQ Splitter"
#define RoQSourceName			L"MPC RoQ Source"
#define RoQVideoDecoderName		L"MPC RoQ Video Decoder"
#define RoQAudioDecoderName		L"MPC RoQ Audio Decoder"

class __declspec(uuid("C73DF7C1-21F2-44C7-A430-D35FB9BB298F"))
CRoQSplitterFilter : public CBaseSplitterFilter
{
	CComPtr<IAsyncReader> m_pAsyncReader;

	struct index {REFERENCE_TIME rtv, rta; __int64 fp;};
	std::list<index> m_index;
	std::list<index>::const_iterator m_indexpos;

protected:
	HRESULT CreateOutputs(IAsyncReader* pAsyncReader);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

public:
	CRoQSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);

	STDMETHODIMP_(HRESULT) QueryFilterInfo(FILTER_INFO* pInfo);
};

class __declspec(uuid("02B8E5C2-4E1F-45D3-9A8E-B8F1EDE6DE09"))
CRoQSourceFilter : public CRoQSplitterFilter
{
public:
	CRoQSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);

	STDMETHODIMP_(HRESULT) QueryFilterInfo(FILTER_INFO* pInfo);
};

class __declspec(uuid("FBEFC5EC-ABA0-4E6C-ACA3-D05FDFEFB853"))
CRoQVideoDecoder : public CTransformFilter
{
	CCritSec m_csReceive;

	REFERENCE_TIME m_rtStart = 0;

	BYTE* m_y[2] = { 0 };
	BYTE* m_u[2] = { 0 };
	BYTE* m_v[2] = { 0 };
	int m_pitch = 0;

	void Copy(BYTE* pOut, BYTE* pIn, DWORD w, DWORD h);

	#pragma pack(push, 1)
	struct roq_cell {BYTE y0, y1, y2, y3, u, v;} m_cells[256];
	struct roq_qcell {roq_cell* idx[4];} m_qcells[256];
	#pragma pack(pop)
	void apply_vector_2x2(int x, int y, roq_cell* cell);
	void apply_vector_4x4(int x, int y, roq_cell* cell);
	void apply_motion_4x4(int x, int y, unsigned char mv, char mean_x, char mean_y);
	void apply_motion_8x8(int x, int y, unsigned char mv, char mean_x, char mean_y);

public:
	CRoQVideoDecoder(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CRoQVideoDecoder();

	HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

	HRESULT Transform(IMediaSample* pIn, IMediaSample* pOut);
	HRESULT CheckInputType(const CMediaType* mtIn);
	HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
	HRESULT DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties);
	HRESULT GetMediaType(int iPosition, CMediaType* pMediaType);

	HRESULT StartStreaming();
	HRESULT StopStreaming();
};

class __declspec(uuid("226FAF85-E358-4502-8C98-F4224BE76953"))
CRoQAudioDecoder : public CTransformFilter
{
public:
	CRoQAudioDecoder(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CRoQAudioDecoder();

	HRESULT Transform(IMediaSample* pIn, IMediaSample* pOut);
	HRESULT CheckInputType(const CMediaType* mtIn);
	HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
	HRESULT DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties);
	HRESULT GetMediaType(int iPosition, CMediaType* pMediaType);
};

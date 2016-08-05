/*
 * (C) 2016 see Authors.txt
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

#define BinkSplitterName	L"MPC Bink Splitter"
#define BinkSourceName		L"MPC Bink Source"

class __declspec(uuid("C14684E8-CCA6-468D-9ABC-1CED631CC31C"))
CBinkSplitterFilter : public CBaseSplitterFilter
{
	CComPtr<IAsyncReader> m_pAsyncReader;
	INT64 m_pos = 0;

	fraction_t m_fps = { 1, 1 };
	UINT32 num_audio_tracks = 0;

	std::vector<BYTE> m_framebuffer;
	size_t m_indexpos = 0;

	struct frame_t {
		UINT32 pos;
		UINT32 keyframe: 1, size : 31;
	};
	std::vector<frame_t> m_seektable;

protected:
	HRESULT CreateOutputs(IAsyncReader* pAsyncReader);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

public:
	CBinkSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);

	STDMETHODIMP_(HRESULT) QueryFilterInfo(FILTER_INFO* pInfo);
};

class __declspec(uuid("1BB4BD04-D9D2-4136-A3B6-75B1A10E11BF"))
CBinkSourceFilter : public CBinkSplitterFilter
{
public:
	CBinkSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);

	STDMETHODIMP_(HRESULT) QueryFilterInfo(FILTER_INFO* pInfo);
};

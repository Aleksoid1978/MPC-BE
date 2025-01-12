/*
 * (C) 2014-2025 see Authors.txt
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

#include <list>
#include "AudioTools/SampleFormat.h"
#include "DSUtil/Packet.h"
#include "DSUtil/SimpleBuffer.h"

struct AVFilterGraph;
struct AVFilterContext;
struct AVFrame;
enum   AVSampleFormat;

class CAudioFilter final
{
	CCritSec        m_csFilter;

private:
	AVFilterGraph   *m_pFilterGraph      = nullptr;
	AVFilterContext *m_pFilterBufferSrc  = nullptr;
	AVFilterContext *m_pFilterBufferSink = nullptr;

	AVFrame         *m_pInputFrame       = nullptr;
	AVFrame         *m_pOutputFrame      = nullptr;

	SampleFormat    m_inSampleFmt        = SAMPLE_FMT_NONE;
	AVSampleFormat  m_inAvSampleFmt      = (AVSampleFormat)-1;
	uint64_t        m_inLayout           = 0;
	int             m_inChannels         = 0;
	int             m_inSamplerate       = 0;

	SampleFormat    m_outSampleFmt       = SAMPLE_FMT_NONE;
	AVSampleFormat  m_outAvSampleFmt     = (AVSampleFormat)-1;
	uint64_t        m_outLayout          = 0;
	int             m_outChannels        = 0;
	int             m_outSamplerate      = 0;

	fraction_t      m_time_base          = { 0, 0 };

public:
	CAudioFilter();
	~CAudioFilter();

private:
	int InitFilterBufferSrc();
	int InitFilterBufferSink();

public:
	HRESULT Initialize(
		const SampleFormat in_format, const uint32_t in_layout, const int in_samplerate,
		const SampleFormat out_format, const uint32_t out_layout, const int out_samplerate,
		const bool autoconvert,
		const std::list<std::pair<CStringA, CStringA>>& filters);

	HRESULT Push(const std::unique_ptr<CPacket>& p);
	HRESULT Push(const REFERENCE_TIME time_start, BYTE* pData, const size_t size);
	void PushEnd();

	HRESULT Pull(std::unique_ptr<CPacket>& p);
	HRESULT Pull(REFERENCE_TIME& time_start, CSimpleBuffer<float>& simpleBuffer, unsigned& allsamples);

	BOOL IsInitialized() const { return m_pFilterGraph != nullptr; }

	void Flush();
};

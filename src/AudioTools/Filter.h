/*
 * (C) 2014-2021 see Authors.txt
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

	AVFrame         *m_pFrame            = nullptr;

	AVSampleFormat  m_av_sample_fmt      = (AVSampleFormat)-1;
	SampleFormat    m_sample_fmt         = SAMPLE_FMT_NONE;

	DWORD           m_layout             = 0;
	DWORD           m_SamplesPerSec      = 0;
	WORD            m_Channels           = 0;

	fraction_t      m_time_base          = { 0, 0 };

public:
	CAudioFilter();
	~CAudioFilter();

	HRESULT Init(const WAVEFORMATEX* wfe, const char* flt_name, const char* flt_args, const bool autoconvert);

	HRESULT Push(const CAutoPtr<CPacket>& p);
	HRESULT Push(const REFERENCE_TIME time_start, BYTE* pData, const size_t size);

	HRESULT Pull(CAutoPtr<CPacket>& p);
	HRESULT Pull(REFERENCE_TIME& time_start, CSimpleBuffer<float>& simpleBuffer, unsigned& allsamples);

	BOOL IsInitialized() const { return m_pFilterGraph != nullptr; }

	void Flush();
};

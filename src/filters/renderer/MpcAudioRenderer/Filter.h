/*
  * (C) 2014-2015  see Authors.txt
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

#include "SampleFormat.h"
#include "../../../DSUtil/Packet.h"

struct AVFilterGraph;
struct AVFilterContext;
struct AVFrame;
enum AVSampleFormat;

class CFilter final
{
	CCritSec		m_csFilter;

private:
	AVFilterGraph	*m_pFilterGraph;
	AVFilterContext	*m_pFilterBufferSrc;
	AVFilterContext	*m_pFilterBufferSink;

	AVSampleFormat	m_av_sample_fmt;
	SampleFormat	m_sample_fmt;

	DWORD			m_layout;
	DWORD			m_SamplesPerSec;
	WORD			m_Channels;

	double			m_dRate;

public:
	CFilter();
	~CFilter();

	HRESULT Init(double dRate, const WAVEFORMATEX* wfe);
	HRESULT Push(CAutoPtr<CPacket> p);
	HRESULT Pull(CAutoPtr<CPacket>& p);

	BOOL IsInitialized() const { return m_pFilterGraph != NULL; }

	void Flush();
};

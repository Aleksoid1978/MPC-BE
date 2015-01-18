/*
 * 
 * (C) 2014 see Authors.txt
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
#include "PaddedArray.h"

struct AVCodec;
struct AVCodecContext;
struct AVFrame;

class CAC3Encoder
{
protected:
	AVCodec*        m_pAVCodec;
	AVCodecContext* m_pAVCtx;
	AVFrame*        m_pFrame;

	float*          m_pSamples;
	int             m_buffersize;
	int             m_framesize;

public:
	CAC3Encoder();

	bool    Init(int sample_rate, DWORD channel_layout);
	HRESULT Encode(CAtlArray<float>& BuffIn, CAtlArray<BYTE>& BuffOut);
	void    FlushBuffers();
	void    StreamFinish();

	DWORD   SelectLayout(DWORD layout);
	DWORD   SelectSamplerate(DWORD samplerate);

	bool OK() { return (m_pAVCtx != NULL); }
};

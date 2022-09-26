/*
 * (C) 2014-2022 see Authors.txt
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

struct AVCodec;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;

class CAC3Encoder
{
private:
	const AVCodec*  m_pAVCodec = nullptr;
	AVCodecContext* m_pAVCtx   = nullptr;
	AVFrame*        m_pFrame   = nullptr;
	AVPacket*       m_pPacket  = nullptr;

	float*          m_pSamples = nullptr;
	int             m_buffersize = 0;
	int             m_framesize  = 0;

public:
	CAC3Encoder();
	~CAC3Encoder();

	bool    Init(int sample_rate, DWORD channel_layout);
	HRESULT Encode(std::vector<float>& BuffIn, std::vector<BYTE>& BuffOut);
	void    FlushBuffers();
	void    StreamFinish();

	DWORD   SelectLayout(DWORD layout);
	DWORD   SelectSamplerate(DWORD samplerate);

	bool OK() const { return (m_pAVCtx != nullptr); }
};

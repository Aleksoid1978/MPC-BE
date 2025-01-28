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

#include "PaddedBuffer.h"
#include "AudioTools/SampleFormat.h"
#include "AudioTools/Mixer.h"

struct AVCodec;
struct AVCodecContext;
struct AVCodecParserContext;
struct AVFrame;
struct AVPacket;

enum AVCodecID FindCodec(const GUID subtype);
const char* GetCodecDescriptorName(enum AVCodecID codec_id);

class CMpaDecFilter;

// CFFAudioDecoder

class CFFAudioDecoder
{
private:
	const AVCodec*        m_pAVCodec = nullptr;
	AVCodecContext*       m_pAVCtx   = nullptr;
	AVCodecParserContext* m_pParser  = nullptr;
	AVFrame*              m_pFrame   = nullptr;
	AVPacket*             m_pPacket  = nullptr;

	struct {
		int flavor;
		int coded_frame_size;
		int audio_framesize;
		int sub_packet_h;
		int sub_packet_size;
		unsigned int deint_id;
	} m_raData = {};

	HRESULT ParseRealAudioHeader(const BYTE* extra, const int extralen);

	bool m_bNeedSyncpoint = false;
	bool m_bStereoDownmix = false;
	bool m_bNeedReinit    = false;

	bool m_bNeedMix       = false;
	int m_MixerChannels   = 0;
	uint64_t m_MixerChannelLayout = 0;
	CMixer m_Mixer;

	CMpaDecFilter* m_pFilter;

public:
	CFFAudioDecoder(CMpaDecFilter* pFilter);

	bool    Init(enum AVCodecID codecID, CMediaType* mediaType);
	void    SetDRC(bool bDRC);
	void    SetStereoDownmix(bool bStereoDownmix);

	HRESULT RealPrepare(BYTE* p, int buffsize, CPaddedBuffer& BuffOut);
	HRESULT SendData(BYTE* p, int size, int* out_size = nullptr);
	HRESULT ReceiveData(std::vector<BYTE>& BuffOut, size_t& outputSize, SampleFormat& samplefmt, REFERENCE_TIME& rtStart);
	void    FlushBuffers();
	void    StreamFinish();

	// info
	enum AVCodecID GetCodecId(); // safe
	const char* GetCodecName();  // unsafe
	SampleFormat GetSampleFmt(); // unsafe
	DWORD GetSampleRate();       // unsafe
	WORD  GetChannels();         // unsafe
	DWORD GetChannelMask();      // unsafe
	WORD  GetCoddedBitdepth();   // unsafe

	bool NeedReinit()    const { return m_bNeedReinit;    }
	bool NeedSyncPoint() const { return m_bNeedSyncpoint; }
};

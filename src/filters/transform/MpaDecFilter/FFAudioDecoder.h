/*
 * (C) 2014-2016 see Authors.txt
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
#include "SampleFormat.h"

struct AVCodec;
struct AVCodecContext;
struct AVCodecParserContext;
struct AVFrame;

enum AVCodecID FindCodec(const GUID subtype);
const char* GetCodecDescriptorName(enum AVCodecID codec_id);

// CFFAudioDecoder

class CFFAudioDecoder
{
protected:
	AVCodec*              m_pAVCodec;
	AVCodecContext*       m_pAVCtx;
	AVCodecParserContext* m_pParser;
	AVFrame*              m_pFrame;

	struct {
		int flavor;
		int coded_frame_size;
		int audio_framesize;
		int sub_packet_h;
		int sub_packet_size;
		unsigned int deint_id;
	} m_raData;

	HRESULT ParseRealAudioHeader(const BYTE* extra, const int extralen);

	bool m_bNeedSyncpoint;
	bool m_bStereoDownmix;
	bool m_bNeedReinit;

public:
	CFFAudioDecoder();

	bool    Init(enum AVCodecID codecID, CMediaType* mediaType);
	void    SetDRC(bool fDRC);
	void    SetStereoDownmix(bool stereodownmix);

	HRESULT RealPrepare(BYTE* p, int buffsize, CPaddedArray& BuffOut);
	HRESULT SendData(BYTE* p, int size, int* out_size = NULL);
	HRESULT ReceiveData(CAtlArray<BYTE>& BuffOut, SampleFormat& samplefmt);
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

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

#include "stdafx.h"
#include "AC3Encoder.h"

#pragma warning(disable: 4005 4244)
extern "C" {
#include "ffmpeg/libavcodec/avcodec.h"
}
#pragma warning(default: 4005 4244)

#include "../../../DSUtil/DSUtil.h"
#include "../../../DSUtil/ff_log.h"

#include "../../Lock.h"

// CFFAudioEncoder

CAC3Encoder::CAC3Encoder()
	: m_pAVCodec(NULL)
	, m_pAVCtx(NULL)
	, m_pFrame(NULL)
	, m_pSamples(NULL)
	, m_buffersize(0)
	, m_framesize(0)
{
	avcodec_register_all();
	av_log_set_callback(ff_log);
	m_pAVCodec = avcodec_find_encoder(AV_CODEC_ID_AC3); // AC-3
}

bool CAC3Encoder::Init(int sample_rate, DWORD channel_layout)
{
	StreamFinish();
	int ret;

	m_pAVCtx = avcodec_alloc_context3(m_pAVCodec);

	m_pAVCtx->bit_rate       = 640000;
	m_pAVCtx->sample_fmt     = AV_SAMPLE_FMT_FLTP;
	m_pAVCtx->sample_rate    = SelectSamplerate(sample_rate);
	m_pAVCtx->channel_layout = channel_layout;
	m_pAVCtx->channels       = av_popcount(channel_layout);

	avcodec_lock;
	ret = avcodec_open2(m_pAVCtx, m_pAVCodec, NULL);
	avcodec_unlock;
	if (ret < 0) {
		DbgLog((LOG_TRACE, 3, L"CAC3Encoder::Init() : avcodec_open2() failed"));
		return false;
	}

	m_pFrame = av_frame_alloc();
	if (!m_pFrame) {
		DbgLog((LOG_TRACE, 3, L"CAC3Encoder::Init() : avcodec_alloc_frame() failed"));
		return false;
	}
	m_pFrame->nb_samples     = m_pAVCtx->frame_size;
	m_pFrame->format         = m_pAVCtx->sample_fmt;
	m_pFrame->channel_layout = m_pAVCtx->channel_layout;

	// the codec gives us the frame size, in samples,
	// we calculate the size of the samples buffer in bytes
	m_framesize  = m_pAVCtx->frame_size * m_pAVCtx->channels * sizeof(float);
	m_buffersize = av_samples_get_buffer_size(NULL, m_pAVCtx->channels, m_pAVCtx->frame_size, m_pAVCtx->sample_fmt, 0);

	m_pSamples = (float*)av_malloc(m_buffersize);
	if (!m_pSamples) {
		DbgLog((LOG_TRACE, 3, L"CAC3Encoder::Init() : av_malloc(%d) failed", m_buffersize));
		return false;
	}
	/* setup the data pointers in the AVFrame */
	ret = avcodec_fill_audio_frame(m_pFrame, m_pAVCtx->channels, m_pAVCtx->sample_fmt, (const uint8_t*)m_pSamples, m_buffersize, 0);
	if (ret < 0) {
		DbgLog((LOG_TRACE, 3, L"CAC3Encoder::Init() : avcodec_fill_audio_frame() failed"));
		return false;
	}

	return true;
}

static inline int encode(AVCodecContext *avctx, AVFrame *frame, int *got_packet, AVPacket *pkt)
{
	*got_packet = 0;
	int ret;

	ret = avcodec_send_frame(avctx, frame);
	if (ret < 0) {
		return ret;
	}

	ret = avcodec_receive_packet(avctx, pkt);
	if (!ret) {
		*got_packet = 1;
	}
	if (ret == AVERROR(EAGAIN)) {
		ret = 0;
	}

	return ret;
}

HRESULT CAC3Encoder::Encode(CAtlArray<float>& BuffIn, CAtlArray<BYTE>& BuffOut)
{
	int buffsamples = BuffIn.GetCount() / m_pAVCtx->channels;
	if (buffsamples < m_pAVCtx->frame_size) {
		return E_ABORT;
	}

	float* pEnc  = m_pSamples;
	float* pIn   = BuffIn.GetData();
	int channels = m_pAVCtx->channels;
	int samples  = m_pAVCtx->frame_size;

	for (int ch = 0; ch < channels; ++ch) {
		for (int i = 0; i < samples; ++i) {
			*pEnc++ = pIn[channels * i + ch];
		}
	}

	int ret;
	AVPacket avpkt;
	int got_packet;

	av_init_packet(&avpkt);
	avpkt.data = NULL; // packet data will be allocated by the encoder
	avpkt.size = 0;

	ret = encode(m_pAVCtx, m_pFrame, &got_packet, &avpkt);
	if (ret < 0) {
		av_packet_unref(&avpkt);
		return E_FAIL;
	}
	if (got_packet) {
		BuffOut.SetCount(avpkt.size);
		memcpy(BuffOut.GetData(), avpkt.data, avpkt.size);
	}
	av_packet_unref(&avpkt);

	size_t old_size  = BuffIn.GetCount() * sizeof(float);
	size_t new_size  = BuffIn.GetCount() * sizeof(float) - m_framesize;
	size_t new_count = new_size / sizeof(float);

	memmove(pIn, (BYTE*)pIn + m_framesize, new_size);
	BuffIn.SetCount(new_count);

	return S_OK;
}

void CAC3Encoder::FlushBuffers()
{
	if (m_pAVCtx && avcodec_is_open(m_pAVCtx)) {
		avcodec_flush_buffers(m_pAVCtx);
	}
}

void CAC3Encoder::StreamFinish()
{
	if (m_pAVCtx) {
		avcodec_close(m_pAVCtx);
		av_freep(&m_pAVCtx->extradata);
		av_freep(&m_pAVCtx);
	}

	av_frame_free(&m_pFrame);

	av_freep(&m_pSamples);
	m_buffersize = 0;
}

DWORD CAC3Encoder::SelectLayout(DWORD layout)
{
	// check supported layouts
	if (m_pAVCodec && m_pAVCodec->channel_layouts) {
		for (size_t i = 0; m_pAVCodec->channel_layouts[i] != 0; i++) {
			if (layout == (DWORD)m_pAVCodec->channel_layouts[i]) {
				return layout;
			}
		}
	}

	// select the suitable format
	int channels = av_popcount(layout & ~AV_CH_LOW_FREQUENCY); // number of channels without lfe
	DWORD new_layout = layout & AV_CH_LOW_FREQUENCY;
	if (channels >= 5) {
		new_layout |= AV_CH_LAYOUT_5POINT0;
	} else if (channels == 4) {
		new_layout |= AV_CH_LAYOUT_QUAD;
	} else if (channels == 3) {
		new_layout |= AV_CH_LAYOUT_2_1;
	} else if (channels == 2) {
		new_layout |= AV_CH_LAYOUT_STEREO;
	} else if (channels == 1) {
		new_layout |= AV_CH_LAYOUT_MONO;
	}

	return new_layout;
}

DWORD CAC3Encoder::SelectSamplerate(DWORD samplerate)
{
	/* // this code does not work, because supported_samplerates is always NULL.
	if (m_pAVCodec && m_pAVCodec->supported_samplerates) {
		for (size_t i = 0; m_pAVCodec->supported_samplerates[i] != 0; i++) {
			if (samplerate == (DWORD)m_pAVCodec->supported_samplerates[i]) {
				return samplerate;
			}
		}
	}*/

	if (samplerate % 11025 == 0) {
		return 44100;
	} else {
		return 48000;
	}
}

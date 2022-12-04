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

#include "stdafx.h"
#include "AC3Encoder.h"

#pragma warning(push)
#pragma warning(disable: 4005)
extern "C" {
#include "ExtLib/ffmpeg/libavcodec/avcodec.h"
#include "ExtLib/ffmpeg/libavutil/channel_layout.h"
}
#pragma warning(pop)

#include "DSUtil/ffmpeg_log.h"

#include "filters/Lock.h"

// CFFAudioEncoder

CAC3Encoder::CAC3Encoder()
{
#ifdef DEBUG_OR_LOG
	av_log_set_callback(ff_log);
#else
	av_log_set_callback(nullptr);
#endif

	m_pAVCodec = avcodec_find_encoder(AV_CODEC_ID_AC3); // AC-3
}

CAC3Encoder::~CAC3Encoder()
{
	StreamFinish();
}

bool CAC3Encoder::Init(int sample_rate, DWORD channel_layout)
{
	StreamFinish();
	int ret;

	m_pAVCtx = avcodec_alloc_context3(m_pAVCodec);

	m_pAVCtx->bit_rate    = 640000;
	m_pAVCtx->sample_fmt  = AV_SAMPLE_FMT_FLTP;
	m_pAVCtx->sample_rate = SelectSamplerate(sample_rate);
	av_channel_layout_from_mask(&m_pAVCtx->ch_layout, channel_layout);

	avcodec_lock;
	ret = avcodec_open2(m_pAVCtx, m_pAVCodec, nullptr);
	avcodec_unlock;
	if (ret < 0) {
		DLog(L"CAC3Encoder::Init() : avcodec_open2() failed");
		return false;
	}

	m_pFrame = av_frame_alloc();
	m_pPacket = av_packet_alloc();

	if (!m_pFrame || !m_pPacket) {
		DLog(L"CAC3Encoder::Init() : avcodec_alloc_frame() or av_packet_alloc() failed");
		StreamFinish();
		return false;
	}

	m_pFrame->nb_samples = m_pAVCtx->frame_size;
	m_pFrame->format     = m_pAVCtx->sample_fmt;
	av_channel_layout_copy(&m_pFrame->ch_layout, &m_pAVCtx->ch_layout);

	// the codec gives us the frame size, in samples,
	// we calculate the size of the samples buffer in bytes
	m_framesize  = m_pAVCtx->frame_size * m_pAVCtx->ch_layout.nb_channels * sizeof(float);
	m_buffersize = av_samples_get_buffer_size(nullptr, m_pAVCtx->ch_layout.nb_channels, m_pAVCtx->frame_size, m_pAVCtx->sample_fmt, 0);

	m_pSamples = (float*)av_malloc(m_buffersize);
	if (!m_pSamples) {
		DLog(L"CAC3Encoder::Init() : av_malloc(%d) failed", m_buffersize);
		return false;
	}
	/* setup the data pointers in the AVFrame */
	ret = av_frame_get_buffer(m_pFrame, 0);
	if (ret < 0) {
		DLog(L"CAC3Encoder::Init() : av_frame_get_buffer() failed");
		return false;
	}
	ret = avcodec_fill_audio_frame(m_pFrame, m_pAVCtx->ch_layout.nb_channels, m_pAVCtx->sample_fmt, (const uint8_t*)m_pSamples, m_buffersize, 0);
	if (ret < 0) {
		DLog(L"CAC3Encoder::Init() : avcodec_fill_audio_frame() failed");
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

HRESULT CAC3Encoder::Encode(std::vector<float>& BuffIn, std::vector<BYTE>& BuffOut)
{
	int buffsamples = BuffIn.size() / m_pAVCtx->ch_layout.nb_channels;
	if (buffsamples < m_pAVCtx->frame_size) {
		return E_ABORT;
	}

	float* pEnc  = m_pSamples;
	float* pIn   = BuffIn.data();
	int channels = m_pAVCtx->ch_layout.nb_channels;
	int samples  = m_pAVCtx->frame_size;

	for (int ch = 0; ch < channels; ++ch) {
		for (int i = 0; i < samples; ++i) {
			*pEnc++ = pIn[channels * i + ch];
		}
	}

	int got_packet = 0;
	int ret = encode(m_pAVCtx, m_pFrame, &got_packet, m_pPacket);
	if (ret < 0) {
		av_packet_unref(m_pPacket);
		return E_FAIL;
	}
	if (got_packet) {
		BuffOut.resize(m_pPacket->size);
		memcpy(BuffOut.data(), m_pPacket->data, m_pPacket->size);
	}
	av_packet_unref(m_pPacket);

	size_t old_size = BuffIn.size() * sizeof(float);
	size_t new_size = old_size - m_framesize;
	size_t new_count = new_size / sizeof(float);

	memmove(pIn, (BYTE*)pIn + m_framesize, new_size);
	BuffIn.resize(new_count);

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
	avcodec_free_context(&m_pAVCtx);

	av_frame_free(&m_pFrame);
	av_packet_free(&m_pPacket);

	av_freep(&m_pSamples);
	m_buffersize = 0;
}

DWORD CAC3Encoder::SelectLayout(DWORD layout)
{
	// check supported layouts
	if (m_pAVCtx &&
			m_pAVCtx->ch_layout.order == AV_CHANNEL_ORDER_NATIVE && m_pAVCtx->ch_layout.u.mask == static_cast<uint64_t>(layout)) {
		return layout;
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

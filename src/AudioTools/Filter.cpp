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

#include "stdafx.h"
#include <basestruct.h>
#include "Filter.h"
#include "DSUtil/AudioParser.h"
#include "DSUtil/ffmpeg_log.h"
#include "AudioTools/AudioHelper.h"

extern "C"
{
	#include <libavcodec/avcodec.h>
	#include <libavfilter/buffersink.h>
	#include <libavfilter/buffersrc.h>
}

CAudioFilter::CAudioFilter()
{
#ifdef DEBUG_OR_LOG
	av_log_set_callback(ff_log);
#else
	av_log_set_callback(nullptr);
#endif

	m_pFrame = av_frame_alloc();
}

CAudioFilter::~CAudioFilter()
{
	av_frame_free(&m_pFrame);
	avfilter_graph_free(&m_pFilterGraph);
}

HRESULT CAudioFilter::Init(const WAVEFORMATEX* wfe, const char* flt_name, const char* flt_args, const bool autoconvert)
{
	CAutoLock cAutoLock(&m_csFilter);

	Flush();

	if (!wfe || !flt_name) {
		return E_POINTER;
	}

	if (flt_name[0] == 0) {
		return E_INVALIDARG;
	}

	const AVFilter *buffersrc = avfilter_get_by_name("abuffer");
	CheckPointer(buffersrc, E_FAIL);
	const AVFilter *buffersink = avfilter_get_by_name("abuffersink");
	CheckPointer(buffersink, E_FAIL);

	m_pFilterGraph = avfilter_graph_alloc();
	CheckPointer(m_pFilterGraph, E_FAIL);
	avfilter_graph_set_auto_convert(m_pFilterGraph, autoconvert ? AVFILTER_AUTO_CONVERT_ALL : AVFILTER_AUTO_CONVERT_NONE);

	SampleFormat sample_fmt = GetSampleFormat(wfe);
	AVSampleFormat av_sample_fmt = AV_SAMPLE_FMT_NONE;

	switch (sample_fmt) {
	case SAMPLE_FMT_U8:
		av_sample_fmt = AV_SAMPLE_FMT_U8;
		break;
	case SAMPLE_FMT_S16:
		av_sample_fmt = AV_SAMPLE_FMT_S16;
		break;
	case SAMPLE_FMT_S24:
	case SAMPLE_FMT_S32:
		av_sample_fmt = AV_SAMPLE_FMT_S32;
		break;
	case SAMPLE_FMT_FLT:
		av_sample_fmt = AV_SAMPLE_FMT_FLT;
		break;
	case SAMPLE_FMT_DBL:
		av_sample_fmt = AV_SAMPLE_FMT_DBL;
		break;
	default:
		ASSERT(FALSE);
		return E_FAIL;
	}

	DWORD layout = 0;
	if (wfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE && wfe->cbSize == 22) {
		layout = ((WAVEFORMATEXTENSIBLE*)wfe)->dwChannelMask;
	} else {
		layout = GetDefChannelMask(wfe->nChannels);
	}

	int ret = 0;
	do {
		char args[256] = { 0 };
		_snprintf_s(args, sizeof(args), "time_base=1/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%x",
			wfe->nSamplesPerSec,
			wfe->nSamplesPerSec,
			av_get_sample_fmt_name(av_sample_fmt),
			layout);
		ret = avfilter_graph_create_filter(&m_pFilterBufferSrc,
			buffersrc,
			"in",
			args,
			nullptr,
			m_pFilterGraph);
		if (ret < 0) { break; }

		ret = avfilter_graph_create_filter(&m_pFilterBufferSink,
			buffersink,
			"out",
			nullptr,
			nullptr,
			m_pFilterGraph);
		if (ret < 0) { break; }

		const AVFilter* filter = avfilter_get_by_name(flt_name);
		AVFilterContext* filter_ctx = avfilter_graph_alloc_filter(m_pFilterGraph, filter, flt_name);
		ret = avfilter_init_str(filter_ctx, flt_args);
		if (ret < 0) { break; }

		ret = avfilter_link(m_pFilterBufferSrc, 0, filter_ctx, 0);
		if (ret < 0) { break; }
		ret = avfilter_link(filter_ctx, 0, m_pFilterBufferSink, 0);
		if (ret < 0) { break; }

		ret = avfilter_graph_config(m_pFilterGraph, nullptr);
	} while (0);

	if (ret < 0) {
		Flush();
		return E_FAIL;
	}

	m_av_sample_fmt = av_sample_fmt;
	m_sample_fmt = sample_fmt;

	m_SamplesPerSec = wfe->nSamplesPerSec;
	m_Channels = wfe->nChannels;
	m_layout = layout;

	AVRational time_base = av_buffersink_get_time_base(m_pFilterBufferSink);
	m_time_base = { time_base.num, time_base.den };

	return S_OK;
}

HRESULT CAudioFilter::Push(const CAutoPtr<CPacket>& p)
{
	return Push(p->rtStart, p->data(), p->size());
}

HRESULT CAudioFilter::Push(const REFERENCE_TIME time_start, BYTE* pData, const size_t size)
{
	CheckPointer(m_pFilterGraph, E_FAIL);
	CheckPointer(m_pFrame, E_FAIL);

	BYTE* pTmpBuf = nullptr;

	const int nSamples = size / (m_Channels * get_bytes_per_sample(m_sample_fmt));

	if (m_av_sample_fmt == AV_SAMPLE_FMT_S32 && m_sample_fmt == SAMPLE_FMT_S24) {
		DWORD pSize = nSamples * m_Channels * sizeof(int32_t);
		pTmpBuf = DNew BYTE[pSize];
		convert_to_int32(SAMPLE_FMT_S24, m_Channels, nSamples, pData, (int32_t*)pTmpBuf);
		pData = &pTmpBuf[0];
	}

	m_pFrame->nb_samples     = nSamples;
	m_pFrame->format         = m_av_sample_fmt;
	m_pFrame->channels       = m_Channels;
	m_pFrame->channel_layout = m_layout;
	m_pFrame->sample_rate    = m_SamplesPerSec;
	m_pFrame->pts            = av_rescale(time_start, m_time_base.den, m_time_base.num * UNITS);

	const int buffersize = av_samples_get_buffer_size(nullptr, m_pFrame->channels, m_pFrame->nb_samples, m_av_sample_fmt, 1);
	int ret = avcodec_fill_audio_frame(m_pFrame, m_pFrame->channels, m_av_sample_fmt, pData, buffersize, 1);
	if (ret >= 0) {
		ret = av_buffersrc_write_frame(m_pFilterBufferSrc, m_pFrame);
	}

	SAFE_DELETE_ARRAY(pTmpBuf);
	av_frame_unref(m_pFrame);

	return ret < 0 ? E_FAIL : S_OK;
}

HRESULT CAudioFilter::Pull(CAutoPtr<CPacket>& p)
{
	CheckPointer(m_pFilterGraph, E_FAIL);
	CheckPointer(m_pFrame, E_FAIL);

	if (!p) {
		p.Attach(DNew CPacket());
	}
	CheckPointer(p, E_FAIL);

	const int ret = av_buffersink_get_frame(m_pFilterBufferSink, m_pFrame);
	if (ret >= 0) {
		p->rtStart = av_rescale(m_pFrame->pts, m_time_base.num * UNITS, m_time_base.den);
		p->rtStop  = p->rtStart + llMulDiv(UNITS, m_pFrame->nb_samples, m_pFrame->sample_rate, 0);

		if (m_av_sample_fmt == AV_SAMPLE_FMT_S32 && m_sample_fmt == SAMPLE_FMT_S24) {
			const DWORD pSize = m_pFrame->nb_samples * m_pFrame->channels * sizeof(BYTE) * 3;
			BYTE* pTmpBuf = DNew BYTE[pSize];
			convert_to_int24(SAMPLE_FMT_S32, m_Channels, m_pFrame->nb_samples, m_pFrame->data[0], pTmpBuf);

			p->SetData(pTmpBuf, pSize);
			delete [] pTmpBuf;
		} else {
			const int buffersize = av_samples_get_buffer_size(nullptr, m_pFrame->channels, m_pFrame->nb_samples, m_av_sample_fmt, 1);
			p->SetData(m_pFrame->data[0], buffersize);
		}
	}
	av_frame_unref(m_pFrame);

	return
		ret >= 0 ? S_OK :
		ret == AVERROR(EAGAIN) ? E_PENDING :
		E_FAIL;
}

HRESULT CAudioFilter::Pull(REFERENCE_TIME& time_start, CSimpleBuffer<float>& simpleBuffer, unsigned& allsamples)
{
	if (m_av_sample_fmt != AV_SAMPLE_FMT_FLT || !m_pFilterGraph || !m_pFrame) {
		return E_ABORT;
	}

	const int ret = av_buffersink_get_frame(m_pFilterBufferSink, m_pFrame);
	if (ret >= 0) {
		time_start = av_rescale(m_pFrame->pts, m_time_base.num * UNITS, m_time_base.den);
		allsamples = m_pFrame->nb_samples * m_pFrame->channels;
		simpleBuffer.WriteData(0, (float*)m_pFrame->data[0], allsamples);
	}
	av_frame_unref(m_pFrame);

	return
		ret >= 0 ? S_OK :
		ret == AVERROR(EAGAIN) ? E_PENDING :
		E_FAIL;
}

void CAudioFilter::Flush()
{
	CAutoLock cAutoLock(&m_csFilter);

	avfilter_graph_free(&m_pFilterGraph);
	m_pFilterBufferSrc	= nullptr;
	m_pFilterBufferSink	= nullptr;
}

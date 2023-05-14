/*
 * (C) 2014-2023 see Authors.txt
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
#include "DSUtil/Utils.h"
#include "AudioTools/AudioHelper.h"

extern "C"
{
	#include <libavfilter/buffersink.h>
	#include <libavfilter/buffersrc.h>
	#include "libavutil/bprint.h"
	#include <libavutil/opt.h>
}

CStringW AvError2Str(const int averror)
{
	CStringW str;

	if (averror >= 0) {
		str = L"OK";
	} else {
#define UNPACK_VALUE(VALUE) case VALUE: str = L#VALUE; break;
		switch (averror) {
			UNPACK_VALUE(AVERROR(EIO))
			UNPACK_VALUE(AVERROR(EAGAIN))
			UNPACK_VALUE(AVERROR(ENOMEM))
			UNPACK_VALUE(AVERROR(EINVAL))
			UNPACK_VALUE(AVERROR(ERANGE))
			UNPACK_VALUE(AVERROR(ENOSYS))
			UNPACK_VALUE(AVERROR_BUG)
			UNPACK_VALUE(AVERROR_EOF)
			UNPACK_VALUE(AVERROR_OPTION_NOT_FOUND)
			UNPACK_VALUE(AVERROR_PATCHWELCOME)
		default:
			str.Format(L"AVERROR(%d)", averror);
		};
#undef UNPACK_VALUE
	}

	return str;
}

AVSampleFormat MpcToAvSampleFormat(const SampleFormat sample_fmt)
{
	switch (sample_fmt) {
	case SAMPLE_FMT_U8:  return AV_SAMPLE_FMT_U8;
	case SAMPLE_FMT_S16: return AV_SAMPLE_FMT_S16;
	case SAMPLE_FMT_S24: // will be converted to a temporary buffer
	case SAMPLE_FMT_S32: return AV_SAMPLE_FMT_S32;
	case SAMPLE_FMT_FLT: return AV_SAMPLE_FMT_FLT;
	case SAMPLE_FMT_DBL: return AV_SAMPLE_FMT_DBL;
	default:
		ASSERT(FALSE);
		return AV_SAMPLE_FMT_NONE;
	}
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

HRESULT CAudioFilter::Initialize(
	const SampleFormat in_format, const uint32_t in_layout, const int in_samplerate,
	const SampleFormat out_format, const uint32_t out_layout, const int out_samplerate,
	const bool autoconvert,
	const std::list<std::pair<CStringA, CStringA>>& filters)
{
	CAutoLock cAutoLock(&m_csFilter);

	Flush();

	if (!in_layout || in_samplerate <= 0|| !out_layout || out_samplerate <= 0) {
		return E_INVALIDARG;
	}

	m_inAvSampleFmt = MpcToAvSampleFormat(in_format);
	if (AV_SAMPLE_FMT_NONE == m_inAvSampleFmt) {
		return E_INVALIDARG;
	}
	m_outAvSampleFmt = MpcToAvSampleFormat(out_format);
	if (AV_SAMPLE_FMT_NONE == m_outAvSampleFmt) {
		return E_INVALIDARG;
	}

	m_inSampleFmt   = in_format;
	m_inLayout      = in_layout;
	m_inChannels    = CountBits(in_layout);
	m_inSamplerate  = in_samplerate;
	m_outSampleFmt  = out_format;
	m_outLayout     = out_layout;
	m_outChannels   = CountBits(out_layout);
	m_outSamplerate = out_samplerate;

	const AVFilter *buffersrc = avfilter_get_by_name("abuffer");
	CheckPointer(buffersrc, E_FAIL);
	const AVFilter *buffersink = avfilter_get_by_name("abuffersink");
	CheckPointer(buffersink, E_FAIL);

	m_pFilterGraph = avfilter_graph_alloc();
	CheckPointer(m_pFilterGraph, E_FAIL);
	avfilter_graph_set_auto_convert(m_pFilterGraph, autoconvert ? AVFILTER_AUTO_CONVERT_ALL : AVFILTER_AUTO_CONVERT_NONE);

	int ret = 0;
	do {
		AVBPrint bp = {};
		av_bprint_init(&bp, 0, AV_BPRINT_SIZE_AUTOMATIC);
		AVChannelLayout ch_layout = { AV_CHANNEL_ORDER_NATIVE, m_inChannels, m_inLayout };
		av_channel_layout_describe_bprint(&ch_layout, &bp);

		char args[256] = { 0 };
		_snprintf_s(args, sizeof(args), "time_base=1/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%s",
			m_inSamplerate,
			m_inSamplerate,
			av_get_sample_fmt_name(m_inAvSampleFmt),
			bp.str);
		av_bprint_finalize(&bp, nullptr);
		ret = avfilter_graph_create_filter(&m_pFilterBufferSrc,
			buffersrc,
			"in",
			args,
			nullptr,
			m_pFilterGraph);
		if (ret < 0) {
			break;
		}

		ret = avfilter_graph_create_filter(&m_pFilterBufferSink,
			buffersink,
			"out",
			nullptr,
			nullptr,
			m_pFilterGraph);
		if (ret < 0) {
			break;
		}

		ret = av_opt_set_bin(m_pFilterBufferSink, "sample_fmts",
			(uint8_t*)&m_outAvSampleFmt, sizeof(m_outAvSampleFmt),
			AV_OPT_SEARCH_CHILDREN);
		if (ret < 0) {
			break;
		}

		av_bprint_init(&bp, 0, AV_BPRINT_SIZE_AUTOMATIC);
		ch_layout = { AV_CHANNEL_ORDER_NATIVE, m_outChannels, m_outLayout };
		av_channel_layout_describe_bprint(&ch_layout, &bp);
		ret = av_opt_set(m_pFilterBufferSink, "ch_layouts",
			bp.str, AV_OPT_SEARCH_CHILDREN);
		av_bprint_finalize(&bp, nullptr);
		if (ret < 0) {
			break;
		}

		ret = av_opt_set_bin(m_pFilterBufferSink, "sample_rates",
			(uint8_t*)&m_outSamplerate, sizeof(m_outSamplerate),
			AV_OPT_SEARCH_CHILDREN);
		if (ret < 0) {
			break;
		}

		AVFilterContext* endFilterCtx = m_pFilterBufferSrc;

		for (const auto&[flt_name, flt_args] : filters) {
			const AVFilter* filter = avfilter_get_by_name(flt_name);
			AVFilterContext* filter_ctx = avfilter_graph_alloc_filter(m_pFilterGraph, filter, flt_name);
			ret = avfilter_init_str(filter_ctx, flt_args);
			if (ret < 0) {
				break;
			}

			ret = avfilter_link(endFilterCtx, 0, filter_ctx, 0);
			if (ret < 0) {
				break;
			}

			endFilterCtx = filter_ctx;
		}
		if (ret < 0) {
			break;
		}

		ret = avfilter_link(endFilterCtx, 0, m_pFilterBufferSink, 0);
		if (ret < 0) {
			break;
		}

		ret = avfilter_graph_config(m_pFilterGraph, nullptr);
	} while (0);

	if (ret < 0) {
		DLog(L"CAudioFilter::Initialize failed with %s", AvError2Str(ret));
		Flush();
		return E_FAIL;
	}

	AVRational time_base = av_buffersink_get_time_base(m_pFilterBufferSink);
	m_time_base = { time_base.num, time_base.den };

	return S_OK;
}

HRESULT CAudioFilter::Push(const std::unique_ptr<CPacket>& p)
{
	return Push(p->rtStart, p->data(), p->size());
}

HRESULT CAudioFilter::Push(const REFERENCE_TIME time_start, BYTE* pData, const size_t size)
{
	if (!m_pFilterBufferSrc || !m_pFrame) {
		return E_ABORT;
	}
	ASSERT(av_sample_fmt_is_planar(m_inAvSampleFmt) == 0);

	const int nSamples = size / (m_inChannels * get_bytes_per_sample(m_inSampleFmt));

	m_pFrame->nb_samples     = nSamples;
	m_pFrame->format         = m_inAvSampleFmt;
	m_pFrame->ch_layout      = { AV_CHANNEL_ORDER_NATIVE, m_inChannels, m_inLayout };
	m_pFrame->sample_rate    = m_inSamplerate;
	m_pFrame->pts            = av_rescale(time_start, m_time_base.den, m_time_base.num * UNITS);

	int ret = av_frame_get_buffer(m_pFrame, 0);
	if (ret < 0) {
		return E_OUTOFMEMORY;
	}

	if (m_inSampleFmt == SAMPLE_FMT_S24 && m_inAvSampleFmt == AV_SAMPLE_FMT_S32) {
		convert_int24_to_int32((int32_t*)m_pFrame->data[0], pData, nSamples * m_inChannels);
	} else {
		memcpy(m_pFrame->data[0], pData, size);
	}

	ret = av_buffersrc_write_frame(m_pFilterBufferSrc, m_pFrame);

	av_frame_unref(m_pFrame);
	DLogIf(ret < 0, L"CAudioFilter::Push failed with %s", AvError2Str(ret));

	return ret < 0 ? E_FAIL : S_OK;
}

void CAudioFilter::PushEnd()
{
	if (m_pFilterBufferSink) {
		int ret = av_buffersrc_add_frame(m_pFilterBufferSrc, nullptr);
	}
}

HRESULT CAudioFilter::Pull(std::unique_ptr<CPacket>& p)
{
	if (!m_pFilterBufferSink || !m_pFrame) {
		return E_ABORT;
	}
	ASSERT(av_sample_fmt_is_planar(m_outAvSampleFmt) == 0);

	if (!p) {
		p.reset(DNew CPacket());
	}
	CheckPointer(p, E_FAIL);

	const int ret = av_buffersink_get_frame(m_pFilterBufferSink, m_pFrame);
	if (ret >= 0) {
		ASSERT(m_pFrame->format == m_outAvSampleFmt && m_pFrame->ch_layout.nb_channels == m_outChannels);

		p->rtStart = av_rescale(m_pFrame->pts, m_time_base.num * UNITS, m_time_base.den);
		p->rtStop  = p->rtStart + llMulDiv(UNITS, m_pFrame->nb_samples, m_pFrame->sample_rate, 0);

		if (m_outSampleFmt == SAMPLE_FMT_S24 && m_outAvSampleFmt == AV_SAMPLE_FMT_S32) {
			const size_t samples = m_pFrame->nb_samples * m_pFrame->ch_layout.nb_channels;
			p->resize(samples * 3);
			convert_int32_to_int24(p->data(), (int32_t*)m_pFrame->data[0], samples);
		} else {
			const int buffersize = av_samples_get_buffer_size(nullptr, m_pFrame->ch_layout.nb_channels, m_pFrame->nb_samples, m_outAvSampleFmt, 1);
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
	if (m_outAvSampleFmt != AV_SAMPLE_FMT_FLT || !m_pFilterBufferSink || !m_pFrame) {
		return E_ABORT;
	}

	const int ret = av_buffersink_get_frame(m_pFilterBufferSink, m_pFrame);
	if (ret >= 0) {
		ASSERT(m_pFrame->format == m_outAvSampleFmt && m_pFrame->ch_layout.nb_channels == m_outChannels);

		time_start = av_rescale(m_pFrame->pts, m_time_base.num * UNITS, m_time_base.den);
		allsamples = m_pFrame->nb_samples * m_pFrame->ch_layout.nb_channels;
		simpleBuffer.ExtendSize(allsamples);
		memcpy(simpleBuffer.Data(), m_pFrame->data[0], allsamples * sizeof(float));
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
	m_pFilterBufferSrc  = nullptr;
	m_pFilterBufferSink = nullptr;
}

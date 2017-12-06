/*
 * (C) 2014-2017 see Authors.txt
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
#include "AudioHelper.h"
#include "Mixer.h"

#pragma warning(push)
#pragma warning(disable: 4005)
extern "C" {
	#include "ffmpeg/libswresample/swresample.h"
	#include "ffmpeg/libswresample/swresample_internal.h"
	#include "ffmpeg/libavutil/samplefmt.h"
	#include "ffmpeg/libavutil/opt.h"
}
#pragma warning(pop)

CMixer::CMixer()
	: m_pSWRCxt(nullptr)
	, m_matrix_dbl(nullptr)
	, m_normalize_matrix(false)
	, m_ActualContext(false)
	, m_in_sf(SAMPLE_FMT_NONE)
	, m_out_sf(SAMPLE_FMT_NONE)
	, m_in_layout(0)
	, m_out_layout(0)
	, m_in_samplerate(0)
	, m_out_samplerate(0)
	, m_in_avsf(AV_SAMPLE_FMT_NONE)
	, m_out_avsf(AV_SAMPLE_FMT_NONE)
{
	// Allocate SWR Context
	m_pSWRCxt = swr_alloc();
}

CMixer::~CMixer()
{
	swr_free(&m_pSWRCxt);

	av_free(m_matrix_dbl); // If ptr is a NULL pointer, this function simply performs no actions.
}

bool CMixer::Init()
{
	if (sample_fmt_is_planar(m_out_sf)) {
		DLog(L"CMixer::Init() : planar formats are not supported in the output.");
		return false;
	}

	m_in_avsf  = m_in_sf  == SAMPLE_FMT_S24 ? AV_SAMPLE_FMT_S32 : (AVSampleFormat)m_in_sf;
	m_out_avsf = m_out_sf == SAMPLE_FMT_S24 ? AV_SAMPLE_FMT_S32 : (AVSampleFormat)m_out_sf;

	av_freep(&m_matrix_dbl);
	int ret = 0;

	// Close SWR Context
	swr_close(m_pSWRCxt);

	// Set options
	av_opt_set_int(m_pSWRCxt, "in_sample_fmt",      m_in_avsf,        0);
	av_opt_set_int(m_pSWRCxt, "out_sample_fmt",     m_out_avsf,       0);
	av_opt_set_int(m_pSWRCxt, "in_channel_layout",  m_in_layout,      0);
	av_opt_set_int(m_pSWRCxt, "out_channel_layout", m_out_layout,     0);
	av_opt_set_int(m_pSWRCxt, "in_sample_rate",     m_in_samplerate,  0);
	av_opt_set_int(m_pSWRCxt, "out_sample_rate",    m_out_samplerate, 0);

	av_opt_set    (m_pSWRCxt, "resampler",          "soxr",           0); // use soxr library
	//av_opt_set_int(m_pSWRCxt, "precision",          28,               0); // SOXR_VHQ

	// Create Matrix
	const int in_ch  = av_popcount(m_in_layout);
	const int out_ch = av_popcount(m_out_layout);
	m_matrix_dbl = (double*)av_mallocz(in_ch * out_ch * sizeof(*m_matrix_dbl));

	// expand mono to front left and front right channels
	if (m_in_layout == AV_CH_LAYOUT_MONO && m_out_layout & (AV_CH_FRONT_LEFT|AV_CH_FRONT_RIGHT)) {
		int i = 0;
		m_matrix_dbl[i++] = 1.0;
		m_matrix_dbl[i++] = 1.0;
		while (i < out_ch) {
			m_matrix_dbl[i++] = 0.0;
		}
	}
	// expand stereo
	else if (m_in_layout == AV_CH_LAYOUT_STEREO && out_ch >= 4
			&& (m_out_layout & ~(AV_CH_FRONT_LEFT|AV_CH_FRONT_RIGHT|AV_CH_FRONT_CENTER|AV_CH_LOW_FREQUENCY|AV_CH_BACK_LEFT|AV_CH_BACK_RIGHT|AV_CH_SIDE_LEFT|AV_CH_SIDE_RIGHT)) == 0) {
		int i = 0;
		if (m_out_layout & (AV_CH_FRONT_LEFT | AV_CH_FRONT_RIGHT)) {
			m_matrix_dbl[i++] = 1.0;
			m_matrix_dbl[i++] = 0.0;
			m_matrix_dbl[i++] = 0.0;
			m_matrix_dbl[i++] = 1.0;
		}
		if (m_out_layout & AV_CH_FRONT_CENTER) {
			m_matrix_dbl[i++] = 0.5;
			m_matrix_dbl[i++] = 0.5;
		}
		if (m_out_layout & AV_CH_LOW_FREQUENCY) {
			m_matrix_dbl[i++] = 0.0;
			m_matrix_dbl[i++] = 0.0;
		}
		if (m_out_layout & (AV_CH_BACK_LEFT | AV_CH_BACK_RIGHT)) {
			m_matrix_dbl[i++] = 0.6666;
			m_matrix_dbl[i++] = (-0.2222);
			m_matrix_dbl[i++] = (-0.2222);
			m_matrix_dbl[i++] = 0.6666;
		}
		if (m_out_layout & (AV_CH_SIDE_LEFT | AV_CH_SIDE_RIGHT)) {
			m_matrix_dbl[i++] =  0.6666;
			m_matrix_dbl[i++] = (-0.2222);
			m_matrix_dbl[i++] = (-0.2222);
			m_matrix_dbl[i++] = 0.6666;
		}
	} else {
		const double center_mix_level   = M_SQRT1_2;
		const double surround_mix_level = 1.0;
		const double lfe_mix_level      = 1.0;
		const double rematrix_maxval    = INT_MAX; // matrix coefficients will not be normalized
		const double rematrix_volume    = 0.0; // not to do a rematrix.
		ret = swr_build_matrix(
			m_in_layout, m_out_layout,
			center_mix_level, surround_mix_level, lfe_mix_level,
			rematrix_maxval, rematrix_volume,
			m_matrix_dbl, in_ch,
			AV_MATRIX_ENCODING_NONE, nullptr);
		if (ret < 0) {
			DLog(L"CMixer::Init() : swr_build_matrix failed");
			av_freep(&m_matrix_dbl);
			return false;
		}

		// if back channels do not have sound, then divide side channels for the back and side
		if (m_out_layout == AV_CH_LAYOUT_7POINT1) {
			bool back_no_sound = true;
			for (int i = 0; i < in_ch * 2; i++) {
				if (m_matrix_dbl[4 * in_ch + i] != 0.0) {
					back_no_sound = false;
				}
			}
			if (back_no_sound) {
				for (int i = 0; i < in_ch * 2; i++) {
					m_matrix_dbl[4 * in_ch + i] = (m_matrix_dbl[6 * in_ch + i] *= M_SQRT1_2);
				}
			}
		}
	}

	if (m_normalize_matrix) {
		double peekmax = 0.0;

		for (int j = 0; j < out_ch; j++) {
			double peek = 0.0;
			for (int i = 0; i < in_ch; i++) {
				peek += m_matrix_dbl[j * in_ch + i];
			}
			if (peek > peekmax) {
				peekmax = peek;
			}
		}

		if (fabs(peekmax - 1.0) > 0.0001) {
			for (int j = 0; j < out_ch; j++) {
				for (int i = 0; i < in_ch; i++) {
					m_matrix_dbl[j * in_ch + i] /= peekmax;
				}
			}
		}
	}

#ifdef DEBUG_OR_LOG
	CString matrix_str = L"CMixer::Init() : matrix";
	double k = 0.0;
	for (int j = 0; j < out_ch; j++) {
		matrix_str.AppendFormat(L"\n    %d:", j + 1);
		for (int i = 0; i < in_ch; i++) {
			k = m_matrix_dbl[j * in_ch + i];
			matrix_str.AppendFormat(L" %.4f", k);
		}
	}
	DLog(matrix_str);
#endif

	// Set Matrix on the context
	ret = swr_set_matrix(m_pSWRCxt, m_matrix_dbl, in_ch);
	if (ret < 0) {
		DLog(L"CMixer::Init() : swr_set_matrix failed");
		av_freep(&m_matrix_dbl);
		return false;
	}

	// init SWR Context
	ret = swr_init(m_pSWRCxt);
	if (ret < 0) {
		// try again ...
		av_opt_set_int(m_pSWRCxt, "internal_sample_fmt", AV_SAMPLE_FMT_FLTP, 0);
		ret = swr_init(m_pSWRCxt);

		if (ret < 0) {
			DLog(L"CMixer::Init() : swr_init failed");
			return false;
		}
	}

	m_ActualContext = true;
	return true;
}

void CMixer::SetOptions(bool normalize_matrix)
{
	if (normalize_matrix != m_normalize_matrix) {
		m_normalize_matrix = normalize_matrix;
		m_ActualContext = false;
	}
}

void CMixer::UpdateInput(SampleFormat in_sf, DWORD in_layout, int in_samplerate)
{
	if (in_sf != m_in_sf || in_layout != m_in_layout || in_samplerate != m_in_samplerate) {
		m_in_layout     = in_layout;
		m_in_sf         = in_sf;
		m_in_samplerate = in_samplerate;
		m_ActualContext = false;
	}
}

void CMixer::UpdateOutput(SampleFormat out_sf, DWORD out_layout, int out_samplerate)
{
	if (out_sf != m_out_sf || out_layout != m_out_layout || out_samplerate != m_out_samplerate) {
		m_out_layout     = out_layout;
		m_out_sf         = out_sf;
		m_out_samplerate = out_samplerate;
		m_ActualContext  = false;
	}
}

int CMixer::Mixing(BYTE* pOutput, int out_samples, BYTE* pInput, int in_samples)
{
	if (!m_ActualContext && !Init()) {
		DLog(L"CMixer::Mixing() : Init failed");
		return 0;
	}

	const int in_ch  = av_popcount(m_in_layout);
	const int out_ch = av_popcount(m_out_layout);

	int32_t* buf1 = nullptr;
	if (m_in_sf == SAMPLE_FMT_S24) {
		ASSERT(m_in_avsf == AV_SAMPLE_FMT_S32);
		buf1 = DNew int32_t[in_samples * in_ch];
		convert_int24_to_int32(buf1, pInput, in_samples * in_ch);
		pInput = (BYTE*)buf1;
	}

	BYTE* output;
	int32_t* buf2 = nullptr;
	if (m_out_sf == SAMPLE_FMT_S24) {
		ASSERT(m_out_avsf == AV_SAMPLE_FMT_S32);
		buf2 = DNew int32_t[out_samples * out_ch];
		output = (BYTE*)buf2;
	} else {
		output = pOutput;
	}

	int in_plane_nb   = av_sample_fmt_is_planar(m_in_avsf) ? in_ch : 1;
	int in_plane_size = in_samples * (av_sample_fmt_is_planar(m_in_avsf) ? 1 : in_ch) * av_get_bytes_per_sample(m_in_avsf);

	static BYTE* ppInput[SWR_CH_MAX];
	for (int i = 0; i < in_plane_nb; i++) {
		ppInput[i] = pInput + i * in_plane_size;
	}

	out_samples = swr_convert(m_pSWRCxt, &output, out_samples, (const uint8_t**)ppInput, in_samples);
	if (out_samples < 0) {
		DLog(L"CMixer::Mixing() : swr_convert failed");
		out_samples = 0;
	}

	if (buf1) {
		delete [] buf1;
	}
	if (buf2) {
		convert_int32_to_int24(pOutput, buf2, out_samples * out_ch);
		delete [] buf2;
	}

	return out_samples;
}

int CMixer::Receive(BYTE* pOutput, int out_samples)
{
	if (!m_ActualContext && !Init()) {
		DLog(L"CMixer::Receive() : Init failed");
		return 0;
	}

	const int out_ch = av_popcount(m_out_layout);

	BYTE* output;
	int32_t* buf = nullptr;
	if (m_out_sf == SAMPLE_FMT_S24) {
		ASSERT(m_out_avsf == AV_SAMPLE_FMT_S32);
		buf = DNew int32_t[out_samples * out_ch];
		output = (BYTE*)buf;
	} else {
		output = pOutput;
	}

	out_samples = swr_convert(m_pSWRCxt, &output, out_samples, nullptr, 0);
	if (out_samples < 0) {
		DLog(L"CMixer::Receive() : swr_convert failed");
		out_samples = 0;
	}

	if (buf) {
		convert_int32_to_int24(pOutput, buf, out_samples * out_ch);
		delete [] buf;
	}

	return out_samples;
}

int64_t CMixer::GetDelay()
{
	return swr_get_delay(m_pSWRCxt, UNITS);
}

int CMixer::CalcOutSamples(int in_samples)
{
	if (!m_ActualContext && !Init()) {
		DLog(L"Mixer::CalcOutSamples() : Init failed");
		return 0;
	}

	if (m_in_samplerate == m_out_samplerate) {
		return in_samples;
	} else {
		return swr_get_out_samples(m_pSWRCxt, in_samples) + swr_get_delay(m_pSWRCxt, m_in_samplerate);
	}
}

void CMixer::FlushBuffers()
{
	if (m_in_samplerate != m_out_samplerate) {
		// Close SWR Context
		swr_close(m_pSWRCxt);

		// Set Matrix on the context
		int ret = swr_set_matrix(m_pSWRCxt, m_matrix_dbl, av_popcount(m_in_layout));
		if (ret < 0) {
			DLog(L"CMixer::FlushBuffers() : swr_set_matrix failed");
			return;
		}

		// init SWR Context
		ret = swr_init(m_pSWRCxt);
		if (ret < 0) {
			DLog(L"CMixer::FlushBuffers() : swr_init failed");
			return;
		}
	}
}

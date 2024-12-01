/*
 * (C) 2014-2024 see Authors.txt
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
#include "DSUtil/Utils.h"
#include "Mixer.h"

#pragma warning(push)
#pragma warning(disable: 4005)
extern "C" {
	#include "ExtLib/ffmpeg/libswresample/swresample.h"
	#include "ExtLib/ffmpeg/libavutil/opt.h"
}
#pragma warning(pop)

CMixer::CMixer()
	: m_pSWRCxt(nullptr)
	, m_matrix_dbl(nullptr)
	, m_center_level(1.0)
	, m_surround_level(1.0)
	, m_normalize_matrix(false)
	, m_dummy_channels(false)
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

	m_Buffer1.SetSize(0);
	m_Buffer2.SetSize(0);

	m_in_avsf  = m_in_sf  == SAMPLE_FMT_S24 ? AV_SAMPLE_FMT_S32 : (AVSampleFormat)m_in_sf;
	m_out_avsf = m_out_sf == SAMPLE_FMT_S24 ? AV_SAMPLE_FMT_S32 : (AVSampleFormat)m_out_sf;

	av_freep(&m_matrix_dbl);
	int ret = 0;

	const int in_ch = av_popcount64(m_in_layout);
	const int out_ch = av_popcount(m_out_layout);
	const AVChannelLayout in_ch_layout = { AV_CHANNEL_ORDER_NATIVE, in_ch, m_in_layout };
	const AVChannelLayout out_ch_layout = { AV_CHANNEL_ORDER_NATIVE, out_ch, m_out_layout };

	// Close SWR Context
	swr_close(m_pSWRCxt);

	// Set options
	av_opt_set_int(m_pSWRCxt,      "in_sample_fmt",   m_in_avsf,        0);
	av_opt_set_int(m_pSWRCxt,      "out_sample_fmt",  m_out_avsf,       0);
	av_opt_set_chlayout(m_pSWRCxt, "in_chlayout",     &in_ch_layout,    0);
	av_opt_set_chlayout(m_pSWRCxt, "out_chlayout",    &out_ch_layout,   0);
	av_opt_set_int(m_pSWRCxt,      "in_sample_rate",  m_in_samplerate,  0);
	av_opt_set_int(m_pSWRCxt,      "out_sample_rate", m_out_samplerate, 0);

	av_opt_set    (m_pSWRCxt,      "resampler",       "soxr",           0); // use soxr library
	//av_opt_set_int(m_pSWRCxt,      "precision",       28,               0); // SOXR_VHQ

	// Create Matrix
	m_matrix_dbl = (double*)av_mallocz(in_ch * out_ch * sizeof(*m_matrix_dbl));

	// special mode that adds empty channels to existing channels
	if (m_dummy_channels && (m_out_layout & m_in_layout) == m_in_layout) {
		int olayout = m_out_layout;
		for (int j = 0; j < out_ch; j++) {
			const int och = olayout & (-olayout);
			int ilayout = m_in_layout;
			for (int i = 0; i < in_ch; i++) {
				const int ich = ilayout & (-ilayout);
				m_matrix_dbl[j * in_ch + i] = (ich == och) ? 1.0 : 0.0;
				ilayout &= ~ich;
			}
			olayout &= ~och;
		}
	}
	// expand mono to front left and front right channels
	else if (m_in_layout == AV_CH_LAYOUT_MONO && m_out_layout & (AV_CH_FRONT_LEFT|AV_CH_FRONT_RIGHT)) {
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
			m_matrix_dbl[i++] = 0.5; // m_center_level is not needed here
			m_matrix_dbl[i++] = 0.5; // m_center_level is not needed here
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
	}
	// no mixing
	else if (m_in_layout == m_out_layout) {
		for (int j = 0; j < out_ch; j++) {
			for (int i = 0; i < in_ch; i++) {
				m_matrix_dbl[j * in_ch + i] = (i == j) ? 1.0 : 0.0;
			}
		}

		if (m_in_layout&AV_CH_FRONT_CENTER && m_center_level != 1.0) {
			m_matrix_dbl[BitNum(m_in_layout, AV_CH_FRONT_CENTER) * (in_ch + 1)] *= m_center_level;
		}

		if (m_surround_level != 1.0) {
			if (m_in_layout&AV_CH_BACK_LEFT) {
				m_matrix_dbl[BitNum(m_in_layout, AV_CH_BACK_LEFT) * (in_ch + 1)] *= m_surround_level;
			}
			if (m_in_layout&AV_CH_BACK_RIGHT) {
				m_matrix_dbl[BitNum(m_in_layout, AV_CH_BACK_RIGHT) * (in_ch + 1)] *= m_surround_level;
			}
			if (m_in_layout&AV_CH_BACK_CENTER) {
				m_matrix_dbl[BitNum(m_in_layout, AV_CH_BACK_CENTER) * (in_ch + 1)] *= m_surround_level;
			}
			if (m_in_layout&AV_CH_SIDE_LEFT) {
				m_matrix_dbl[BitNum(m_out_layout, AV_CH_SIDE_LEFT) * (in_ch + 1)] *= m_surround_level;
			}
			if (m_in_layout&AV_CH_SIDE_RIGHT) {
				m_matrix_dbl[BitNum(m_in_layout, AV_CH_SIDE_RIGHT) * (in_ch + 1)] *= m_surround_level;
			}
		}
	}
	else {
		const double center_mix_level   = M_SQRT1_2 * m_center_level;
		const double surround_mix_level = M_SQRT1_2 * m_surround_level;
		const double lfe_mix_level      = 1.0;
		const double rematrix_maxval    = INT_MAX; // matrix coefficients will not be normalized
		const double rematrix_volume    = 0.0; // not to do a rematrix.

		ret = swr_build_matrix2(
			&in_ch_layout, &out_ch_layout,
			center_mix_level, surround_mix_level, lfe_mix_level,
			rematrix_maxval, rematrix_volume,
			m_matrix_dbl, in_ch,
			AV_MATRIX_ENCODING_NONE, nullptr);
		if (ret < 0) {
			DLog(L"CMixer::Init() : swr_build_matrix2 failed");
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

void CMixer::SetOptions(double center_level, double suround_level, bool normalize_matrix, bool dummy_channels)
{
	if (center_level != m_center_level || suround_level != m_surround_level
			|| normalize_matrix != m_normalize_matrix || dummy_channels != m_dummy_channels) {
		m_center_level = center_level;
		m_surround_level = suround_level;
		m_normalize_matrix = normalize_matrix;
		m_dummy_channels = dummy_channels;
		m_ActualContext = false;
	}
}

void CMixer::UpdateInput(SampleFormat in_sf, uint64_t in_layout, int in_samplerate)
{
	if (in_sf != m_in_sf || in_layout != m_in_layout || in_samplerate != m_in_samplerate) {
		m_in_layout     = in_layout;
		m_in_sf         = in_sf;
		m_in_samplerate = in_samplerate;
		m_ActualContext = false;
	}
}

void CMixer::UpdateOutput(SampleFormat out_sf, uint32_t out_layout, int out_samplerate)
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

	const int in_ch  = av_popcount64(m_in_layout);
	const int out_ch = av_popcount(m_out_layout);

	if (m_in_sf == SAMPLE_FMT_S24) {
		ASSERT(m_in_avsf == AV_SAMPLE_FMT_S32);
		size_t allsamples = in_samples * in_ch;
		m_Buffer1.ExtendSize(allsamples);
		convert_int24_to_int32(m_Buffer1.Data(), pInput, allsamples);
		pInput = (BYTE*)m_Buffer1.Data();
	}

	BYTE* output;
	if (m_out_sf == SAMPLE_FMT_S24) {
		ASSERT(m_out_avsf == AV_SAMPLE_FMT_S32);
		m_Buffer2.ExtendSize(out_samples * out_ch);
		output = (BYTE*)m_Buffer2.Data();
	} else {
		output = pOutput;
	}

	int in_plane_nb   = av_sample_fmt_is_planar(m_in_avsf) ? in_ch : 1;
	int in_plane_size = in_samples * (av_sample_fmt_is_planar(m_in_avsf) ? 1 : in_ch) * av_get_bytes_per_sample(m_in_avsf);

	static BYTE* ppInput[64/*SWR_CH_MAX*/];
	for (int i = 0; i < in_plane_nb; i++) {
		ppInput[i] = pInput + i * in_plane_size;
	}

	out_samples = swr_convert(m_pSWRCxt, &output, out_samples, (const uint8_t**)ppInput, in_samples);
	if (out_samples < 0) {
		DLog(L"CMixer::Mixing() : swr_convert failed");
		out_samples = 0;
	}

	if (output == (BYTE*)m_Buffer2.Data()) {
		convert_int32_to_int24(pOutput, m_Buffer2.Data(), out_samples * out_ch);
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
	if (m_out_sf == SAMPLE_FMT_S24) {
		ASSERT(m_out_avsf == AV_SAMPLE_FMT_S32);
		m_Buffer2.ExtendSize(out_samples * out_ch);
		output = (BYTE*)m_Buffer2.Data();
	} else {
		output = pOutput;
	}

	out_samples = swr_convert(m_pSWRCxt, &output, out_samples, nullptr, 0);
	if (out_samples < 0) {
		DLog(L"CMixer::Receive() : swr_convert failed");
		out_samples = 0;
	}

	if (output == (BYTE*)m_Buffer2.Data()) {
		convert_int32_to_int24(pOutput, m_Buffer2.Data(), out_samples * out_ch);
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

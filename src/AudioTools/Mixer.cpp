/*
 *
 * (C) 2014-2015 see Authors.txt
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
#include "Mixer.h"

#pragma warning(disable: 4005)
extern "C" {
	#include "ffmpeg/libavresample/avresample.h"
	#include "ffmpeg/libavutil/samplefmt.h"
	#include "ffmpeg/libavutil/opt.h"
}
#pragma warning(default: 4005)
#include "AudioHelper.h"

CMixer::CMixer()
	: m_pAVRCxt(NULL)
	, m_matrix_dbl(NULL)
	, m_ActualContext(false)
	, m_in_sf(SAMPLE_FMT_NONE)
	, m_out_sf(SAMPLE_FMT_NONE)
	, m_in_layout(0)
	, m_out_layout(0)
	, m_in_samplerate(0)
	, m_out_samplerate(0)
	, m_matrix_norm(0.0f)
	, m_in_avsf(AV_SAMPLE_FMT_NONE)
	, m_out_avsf(AV_SAMPLE_FMT_NONE)
{
	// Allocate Resample Context
	m_pAVRCxt = avresample_alloc_context();
}

CMixer::~CMixer()
{
	avresample_free(&m_pAVRCxt);

	av_free(m_matrix_dbl); // If ptr is a NULL pointer, this function simply performs no actions.
}

bool CMixer::Init()
{
	if (sample_fmt_is_planar(m_out_sf)) {
		TRACE(_T("Mixer: planar formats are not supported in the output.\n"));
		return false;
	}

	m_in_avsf  = m_in_sf == SAMPLE_FMT_S24 ? AV_SAMPLE_FMT_S32 : (AVSampleFormat)m_in_sf;
	m_out_avsf = m_out_sf == SAMPLE_FMT_S24 ? AV_SAMPLE_FMT_S32 : (AVSampleFormat)m_out_sf;

	if (m_matrix_dbl) {
		av_free(m_matrix_dbl);
		m_matrix_dbl = NULL;
	}

	// Close Resample Context
	avresample_close(m_pAVRCxt);

	int ret = 0;
	// Set options
	av_opt_set_int(m_pAVRCxt, "in_sample_fmt",      m_in_avsf,        0);
	av_opt_set_int(m_pAVRCxt, "out_sample_fmt",     m_out_avsf,       0);
	av_opt_set_int(m_pAVRCxt, "in_channel_layout",  m_in_layout,      0);
	av_opt_set_int(m_pAVRCxt, "out_channel_layout", m_out_layout,     0);
	av_opt_set_int(m_pAVRCxt, "in_sample_rate",     m_in_samplerate,  0);
	av_opt_set_int(m_pAVRCxt, "out_sample_rate",    m_out_samplerate, 0);

	// Open Resample Context
	ret = avresample_open(m_pAVRCxt);
	if (ret < 0) {
		// try again ...
		av_opt_set_int(m_pAVRCxt, "internal_sample_fmt", AV_SAMPLE_FMT_FLTP, 0);
		ret = avresample_open(m_pAVRCxt);

		if (ret < 0) {
			TRACE(_T("Mixer: avresample_open failed\n"));
			return false;
		}
	}

	// Create Matrix
	int in_ch  = av_popcount(m_in_layout);
	int out_ch = av_popcount(m_out_layout);
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
			m_matrix_dbl[i++] =  (-0.2222);
			m_matrix_dbl[i++] = (-0.2222);
			m_matrix_dbl[i++] = 0.6666;
		}
	} else {
		const double center_mix_level   = M_SQRT1_2;
		const double surround_mix_level = 1.0;
		const double lfe_mix_level      = 1.0;
		const int normalize = 0;
		ret = avresample_build_matrix(m_in_layout, m_out_layout, center_mix_level, surround_mix_level, lfe_mix_level, normalize, m_matrix_dbl, in_ch, AV_MATRIX_ENCODING_NONE);
		if (ret < 0) {
			TRACE(_T("Mixer: avresample_build_matrix failed\n"));
			av_free(m_matrix_dbl);
			m_matrix_dbl = NULL;
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

	if (m_matrix_norm > 0.0f && m_matrix_norm <= 1.0f) { // 0.0 - normalize off; 1.0 - full normalize matrix
		double max_peak = 0;
		for (int j = 0; j < out_ch; j++) {
			double peak = 0;
			for (int i = 0; i < in_ch; i++) {
				peak += fabs(m_matrix_dbl[j * in_ch + i]);
			}
			if (peak > max_peak) {
				max_peak = peak;
			}
		}
		if (max_peak > 1.0) {
			double g = ((max_peak - 1.0) * (1.0 - m_matrix_norm) + 1.0) / max_peak;
			for (int i = 0, n = in_ch * out_ch; i < n; i++) {
				m_matrix_dbl[i] *= g;
			}
		}
	}

#ifdef _DEBUG
	CString matrix_str = _T("Mixer: matrix\n");;
	for (int j = 0; j < out_ch; j++) {
		matrix_str.AppendFormat(_T("%d:"), j + 1);
		for (int i = 0; i < in_ch; i++) {
			double k = m_matrix_dbl[j * in_ch + i];
			matrix_str.AppendFormat(_T(" %.4f"), k);
		}
		matrix_str += _T("\n");
	}
	TRACE(matrix_str);
#endif

	// Set Matrix on the context
	ret = avresample_set_matrix(m_pAVRCxt, m_matrix_dbl, in_ch);
	if (ret < 0) {
		TRACE(_T("Mixer: avresample_set_matrix failed\n"));
		av_free(m_matrix_dbl);
		m_matrix_dbl = NULL;
		return false;
	}

	m_ActualContext = true;
	return true;
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

void CMixer::SetOptions(float matrix_norm)
{
	if (matrix_norm != m_matrix_norm) {
		m_matrix_norm   = matrix_norm;
		m_ActualContext = false;
	}
}

int CMixer::Mixing(BYTE* pOutput, int out_samples, BYTE* pInput, int in_samples)
{
	if (!m_ActualContext && !Init()) {
		TRACE(_T("Mixer: Init() failed\n"));
		return 0;
	}

	int in_ch  = av_popcount(m_in_layout);
	int out_ch = av_popcount(m_out_layout);

	int32_t* buf1 = NULL;
	if (m_in_sf == SAMPLE_FMT_S24) {
		ASSERT(m_in_avsf == AV_SAMPLE_FMT_S32);
		buf1 = new int32_t[in_samples * in_ch];
		convert_int24_to_int32(in_samples * in_ch, pInput, buf1);
		pInput = (BYTE*)buf1;
	}

	BYTE* output;
	int32_t* buf2 = NULL;
	if (m_out_sf == SAMPLE_FMT_S24) {
		ASSERT(m_out_avsf == AV_SAMPLE_FMT_S32);
		buf2 = new int32_t[out_samples * out_ch];
		output = (BYTE*)buf2;
	} else {
		output = pOutput;
	}

	int in_plane_nb   = av_sample_fmt_is_planar(m_in_avsf) ? in_ch : 1;
	int in_plane_size = in_samples * (av_sample_fmt_is_planar(m_in_avsf) ? 1 : in_ch) * av_get_bytes_per_sample(m_in_avsf);

	static BYTE* ppInput[AVRESAMPLE_MAX_CHANNELS];
	for (int i = 0; i < in_plane_nb; i++) {
		ppInput[i] = pInput + i * in_plane_size;
	}

	int out_plane_size = out_samples * out_ch * av_get_bytes_per_sample(m_out_avsf);

	out_samples = avresample_convert(m_pAVRCxt, (uint8_t**)&output, out_plane_size, out_samples, ppInput, in_plane_size, in_samples);
	if (out_samples < 0) {
		TRACE(_T("Mixer: avresample_convert failed\n"));
		out_samples = 0;
	}

	if (buf1) {
		delete[] buf1;
	}
	if (buf2) {
		convert_int32_to_int24(out_samples * out_ch, buf2, pOutput);
		delete[] buf2;
	}

	return out_samples;
}

int CMixer::GetInputDelay()
{
	return avresample_get_delay(m_pAVRCxt);
}

int CMixer::CalcOutSamples(int in_samples)
{
	if (!m_ActualContext && !Init()) {
		TRACE(_T("Mixer: Init() failed\n"));
		return 0;
	}

	if (m_in_samplerate == m_out_samplerate) {
		return in_samples;
	} else {
		return avresample_available(m_pAVRCxt) + (int)((__int64)(avresample_get_delay(m_pAVRCxt) + in_samples) * m_out_samplerate / m_in_samplerate);
	}
}

void CMixer::FlushBuffers()
{
	if (m_in_samplerate != m_out_samplerate) {
		// Close Resample Context
		avresample_close(m_pAVRCxt);

		int ret = 0;
		// Open Resample Context
		ret = avresample_open(m_pAVRCxt);
		if (ret < 0) {
			TRACE(_T("Mixer: avresample_open failed\n"));
			return;
		}

		// Set Matrix on the context
		ret = avresample_set_matrix(m_pAVRCxt, m_matrix_dbl, av_popcount(m_in_layout));
		if (ret < 0) {
			TRACE(_T("Mixer: avresample_set_matrix failed\n"));
			return;
		}
	}
}

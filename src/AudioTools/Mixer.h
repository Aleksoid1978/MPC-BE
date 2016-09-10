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

#include "SampleFormat.h"
#include "AudioHelper.h"

struct AVAudioResampleContext;

/*   Algorithms: Recursive single pole low pass filter
 *   Reference: The Scientist and Engineer's Guide to Digital Signal Processing
 *
 *   low-pass: output[N] = input[N] * A + output[N-1] * B
 *     X = exp(-2.0 * pi * Fc)
 *     A = 1 - X
 *     B = X
 *     Fc = cutoff freq / sample rate
 *
 *     Mimics an RC low-pass filter:
 *
 *     ---/\/\/\/\----------->
 *                   |
 *                  --- C
 *                  ---
 *                   |
 *                   |
 *                   V
 */

class LowPassFilter
{
	float A = 0.0f;
	float B = 0.0f;
	int m_channels = 0;
	SampleFormat m_sampleFormat = SAMPLE_FMT_NONE;

public:
	void SetParams(const int channels, const int samplerate, const SampleFormat sampleFormat, const float freq) {
		const float Fc = freq / samplerate;
		const float X  = exp(-2.0f * M_PI * Fc);
		A = 1.0f - X;
		B = X;

		m_channels = channels;
		m_sampleFormat = sampleFormat;
	}

	#define LFE(p) (*(p + 3))

	template <typename T>
	inline void Process(BYTE** ppData, const int samples) {
		if (m_sampleFormat != SAMPLE_FMT_NONE) {
			float input, output, prev = 0.0f;

			T* p = (T*)*ppData;
			T sample;
			for (int i = 0; i < samples; i++) {
				sample = LFE(p);

				switch (m_sampleFormat) {
					case SAMPLE_FMT_U8:
						input = SAMPLE_uint8_to_float((uint8_t)sample);
						break;
					case SAMPLE_FMT_S16:
						input = SAMPLE_int16_to_float(sample);
						break;
					case SAMPLE_FMT_S24:
					case SAMPLE_FMT_S32:
						input = SAMPLE_int32_to_float(sample);
						break;
					default:
						input = (float)sample;
				}

				output = input * A + prev * B;
				prev = output;

				switch (m_sampleFormat) {
					case SAMPLE_FMT_U8:
						sample = SAMPLE_float_to_uint8(output);
						break;
					case SAMPLE_FMT_S16:
						sample = SAMPLE_float_to_int16(output);
						break;
					case SAMPLE_FMT_S24:
					case SAMPLE_FMT_S32:
						sample = SAMPLE_float_to_int32(output);
						break;
					default:
						sample = (T)output;
				}

				LFE(p) = sample;

				p += m_channels;
			}
		}
	}
};

class CMixer
{
protected:
	AVAudioResampleContext* m_pAVRCxt;
	double* m_matrix_dbl;
	bool    m_ActualContext;

	SampleFormat m_in_sf;
	SampleFormat m_out_sf;
	DWORD   m_in_layout;
	DWORD   m_out_layout;
	int     m_in_samplerate;
	int     m_out_samplerate;
	float   m_matrix_norm;

	enum AVSampleFormat m_in_avsf;
	enum AVSampleFormat m_out_avsf;

	LowPassFilter m_LowPassFilter;

	bool Init();

public:
	CMixer();
	~CMixer();

	void UpdateInput (SampleFormat  in_sf, DWORD  in_layout, int  in_samplerate = 48000);
	void UpdateOutput(SampleFormat out_sf, DWORD out_layout, int out_samplerate = 48000);
	void SetOptions(float matrix_norm = 0.0f);

	int  Mixing(BYTE* pOutput, int out_samples, BYTE* pInput, int in_samples);

	int  GetInputDelay(); // needed when using resampling
	int  CalcOutSamples(int in_samples); // needed when using resampling
	void FlushBuffers(); // needed when using resampling
};

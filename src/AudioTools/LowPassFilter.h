/*
 * (C) 2016 see Authors.txt
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

class CLowPassFilter
{
	float A = 0.0f;
	float B = 0.0f;
	unsigned m_shift = 0;
	unsigned m_step = 1;
	float m_sample = 0.0f;
	SampleFormat m_sf = SAMPLE_FMT_NONE;

	void Process_uint8(uint8_t* p, const int samples);
	void Process_int16(int16_t* p, const int samples);
	void Process_int32(int32_t* p, const int samples);
	void Process_float(float* p, const int samples);
	void Process_double(double* p, const int samples);

public:
	void SetParams(SampleFormat sampleFormat, int shift, int step, int samplerate, int freq);
	void Process(BYTE* p, const int samples);
};

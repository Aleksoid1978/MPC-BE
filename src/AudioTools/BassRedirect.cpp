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

#include "stdafx.h"
#include "AudioHelper.h"
#include "../DSUtil/AudioParser.h" // for CountBits()
#include "BassRedirect.h"

int get_channel_num(DWORD layout, DWORD channel)
{
	ASSERT(layout & channel);
	ASSERT(CountBits(channel) == 1);

	int num = 0;
	for (unsigned i = 0; i < 32; i++) {
		DWORD mask = (1 << i);
		if (channel == mask) {
			break;
		}
		if (layout & mask) {
			num++;
		}
	}

	return num;
}

// CBassRedirect

void CBassRedirect::CalcAB()
{
	if (m_samplerate > 0) {
		double Fc = (double)m_cutoff_freq / m_samplerate;
		float X = exp(-2.0 * M_PI * Fc);
		A = 1.0f - X;
		B = X;
	}

	m_sample = 0.0f;
}

#define LOWPASSPROCESS(fmt) \
void CBassRedirect::Process_##fmt## (##fmt##_t* p, const int samples) \
{ \
    for (int i = 0; i < samples; i++) { \
        float sample = 0.5 * (SAMPLE_##fmt##_to_float(p[0]) + SAMPLE_##fmt##_to_float(p[1])); \
        m_sample = sample * A + m_sample * B; \
        p[m_lfepos] = SAMPLE_float_to_##fmt##(m_sample); \
        p += m_chanels; \
    } \
} \
 

LOWPASSPROCESS(uint8)
LOWPASSPROCESS(int16)
LOWPASSPROCESS(int32)
LOWPASSPROCESS(float)
LOWPASSPROCESS(double)

void CBassRedirect::Process_int24(BYTE* p, const int samples)
{
	for (int i = 0; i < samples; i++) {
		int32_t L = (uint32_t)p[0] << 8 |(uint32_t)p[1] << 16 | p[2] << 24;
		int32_t R = (uint32_t)p[3] << 8 | (uint32_t)p[4] << 16 | p[5] << 24;

		float sample = 0.5 * (SAMPLE_int32_to_float(L) + SAMPLE_int32_to_float(R));
		m_sample = sample * A + m_sample * B;

		int32_t LFE = SAMPLE_float_to_int32(m_sample);
		BYTE* pLFE = (BYTE*)LFE;
		p[3*m_lfepos] = pLFE[1];
		p[3*m_lfepos + 1] = pLFE[2];
		p[3*m_lfepos + 2] = pLFE[3];

		p += m_chanels;
	}
}

void CBassRedirect::SetOptions(int cutoff_freq)
{
	if (cutoff_freq != m_cutoff_freq) {
		m_cutoff_freq = cutoff_freq;
		CalcAB();
	}
}

void CBassRedirect::UpdateInput(SampleFormat sf, DWORD layout, int samplerate)
{
	m_sf = sf;

	if (layout != m_layout) {
		m_layout = layout;
		m_chanels = CountBits(layout);
		m_lfepos = get_channel_num(layout, SPEAKER_LOW_FREQUENCY);
		m_sample = 0.0f;
	}

	if (samplerate != m_samplerate) {
		m_samplerate = samplerate;
		CalcAB();
	}
}

void CBassRedirect::Process(BYTE* p, const int samples)
{
	if (CHL_CONTAINS_ALL(m_layout, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY)) {
		switch (m_sf) {
		case SAMPLE_FMT_U8:
			Process_uint8((uint8_t*)p, samples);
			break;
		case SAMPLE_FMT_S16:
			Process_int16((int16_t*)p, samples);
			break;
		case SAMPLE_FMT_S24:
			Process_int24(p, samples);
			break;
		case SAMPLE_FMT_S32:
			Process_int32((int32_t*)p, samples);
			break;
		case SAMPLE_FMT_FLT:
			Process_float((float*)p, samples);
			break;
		case SAMPLE_FMT_DBL:
			Process_double((double*)p, samples);
			break;
		}
	}
}

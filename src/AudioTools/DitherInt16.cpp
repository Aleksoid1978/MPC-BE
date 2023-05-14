/*
 * (C) 2021-2023 see Authors.txt
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
#include "DSUtil/SimpleBuffer.h"
#include "DitherInt16.h"

// used code from Sanear
// https://github.com/alexmarsev/sanear/blob/master/src/DspDither.cpp

// DitherInt16

CDitherInt16::CDitherInt16()
{
	for (auto& distributor : m_distributor) {
		// unnecessary initialization with the range (0; 1). it's here to just show the range explicitly.
		distributor = std::uniform_real_distribution<float>(0.0f, 1.0f);
	}
}

void CDitherInt16::Initialize()
{
	m_simpleBuffer.SetSize(0);

	m_previous.fill(0.0f);
	unsigned seed = 12345;
	for (auto& generator : m_generator) {
		generator.seed(seed++);
	}
}

void CDitherInt16::UpdateInput(const SampleFormat sf, const int chanels)
{
	if (sf != m_sf || chanels != m_chanels) {
		m_sf = sf;
		m_chanels = chanels;

		Initialize();
	}
}

void CDitherInt16::ProcessFloat(int16_t* pDst, float* pSrc, const int samples)
{
	for (int frame = 0; frame < samples; frame++) {
		for (int channel = 0; channel < m_chanels; channel++) {
			float inputSample = pSrc[frame * m_chanels + channel] * (INT16_MAX - 1);

			// High-pass TPDF, 2 LSB amplitude.
			float r = m_distributor[channel](m_generator[channel]);
			float noise = r - m_previous[channel];
			m_previous[channel] = r;

			float outputSample = std::round(inputSample + noise);
			ASSERT(outputSample >= INT16_MIN && outputSample <= INT16_MAX);
			pDst[frame * m_chanels + channel] = (int16_t)outputSample;
		}
	}
}

void CDitherInt16::Process(int16_t* pDst, BYTE* pSrc, const int samples)
{
	switch (m_sf) {
	case SAMPLE_FMT_FLT:
		ProcessFloat(pDst, (float*)pSrc, samples);
		break;
	case SAMPLE_FMT_S32:
	case SAMPLE_FMT_DBL:
	case SAMPLE_FMT_S32P:
	case SAMPLE_FMT_FLTP:
	case SAMPLE_FMT_DBLP:
	case SAMPLE_FMT_S64:
	case SAMPLE_FMT_S64P:
	case SAMPLE_FMT_S24:
		m_simpleBuffer.ExtendSize(samples * m_chanels);
		convert_to_float(m_sf, m_chanels, samples, pSrc, m_simpleBuffer.Data());
		ProcessFloat(pDst, m_simpleBuffer.Data(), samples);
		break;
	default:
		convert_to_int16(m_sf, m_chanels, samples, pSrc, pDst);
		break;
	}
}

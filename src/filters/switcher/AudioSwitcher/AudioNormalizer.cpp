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
#include <math.h>
#include "AudioNormalizer.h"

//
// CAudioNormalizer
//

CAudioNormalizer::CAudioNormalizer()
{
	memset(m_prediction, 7, sizeof(m_prediction));
}

CAudioNormalizer::~CAudioNormalizer()
{
}

int CAudioNormalizer::ProcessInternal(float *samples, unsigned numsamples, unsigned nch)
{
	if (m_vol <= 0) {
		m_vol = 1;
	}

	const size_t allsamples = numsamples * nch;

	float max_peak = 0.0f;
	for (size_t k = 0; k < allsamples; k++) {
		max_peak = std::max(max_peak , std::abs(samples[k]));
	}

	int tries = 0;

redo:
	const double factor = m_vol != m_stepping_vol ? (double)m_vol / (double)(m_stepping_vol) : 1.0;
	const double highest = (double)max_peak * factor * 32768;

	if (highest > 30000.0) {
		if (m_vol > m_stepping) {
			m_vol -= m_stepping;
		} else {
			m_vol = 1;
		}
		tries++;
		if (tries < 1024) {
			goto redo;
		} else {
			return numsamples;
		}
	}

	if (highest > 10.0) {
		const double t = (m_level << 15) / 100.0;
		if (highest > t) {
			if (m_prediction[m_predictor] > 0) {
				m_prediction[m_predictor] = m_prediction[m_predictor] - 1;
			} else {
				m_prediction[m_predictor] = 0;
			}
			const int predict = m_prediction[m_predictor];
			m_predictor >>= 1;
			m_rising = 0;
			if (predict <= 7 && m_vol > 1) {
				m_vol--;
			}
		} else { // highest <= t
			if (m_prediction[m_predictor] < 15) {
				m_prediction[m_predictor] = m_prediction[m_predictor] + 1;
			} else {
				m_prediction[m_predictor] = 15;
			}
			const int predict = m_prediction[m_predictor];
			m_predictor = (m_predictor >> 1) | 0x800;
			if (predict >= 8) {
				m_vol += (++m_rising < 128) ? 1 : m_stepping;
			}
		}
	}

	for (size_t k = 0; k < allsamples; k++) {
		samples[k] *= factor;
	}

	if (m_boost == false && m_vol > m_stepping_vol) {
		m_vol = m_stepping_vol;
	}

	return numsamples;
}

int CAudioNormalizer::Process(float *samples, unsigned numsamples, unsigned nch)
{
	int ret = 0;

	while (numsamples > 0) {
		const unsigned process = std::min(numsamples, 512u);

		ret += ProcessInternal(samples, process, nch);
		numsamples -= process;
		samples += (process * nch);
	}

	return ret;
}

void CAudioNormalizer::SetParam(int Level, bool Boost, int Steping)
{
	m_level = Level;
	m_boost = Boost;
	if (m_stepping != Steping) {
		m_vol = (m_vol << Steping) / (1 << m_stepping);

		m_stepping = Steping;
		m_stepping_vol = 1 << m_stepping;
	}
}

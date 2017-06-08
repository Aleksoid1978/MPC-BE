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
#include <math.h>
#include <algorithm>
#include "AudioNormalizer.h"

//
// CAudioNormalizer
//

CAudioNormalizer::CAudioNormalizer(void)
{
	m_predictor = 0;
	memset(m_prediction, 7, sizeof(m_prediction));

	m_level = 75;
	m_stepping = 8;
	m_boost = true;
	m_vol = 1 << m_stepping;

	m_rising = 0;
}

CAudioNormalizer::~CAudioNormalizer(void)
{
}

int CAudioNormalizer::ProcessInternal(float *samples, unsigned numsamples, unsigned nch)
{
	if (m_vol <= 0) {
		m_vol = 1;
	}

	const size_t allsamples = numsamples * nch;
	if (allsamples > m_bufHQ.GetCount()) {
		m_bufHQ.SetCount(allsamples);
		m_smpHQ.SetCount(allsamples);
	}
	double *bufHQ = m_bufHQ.GetData();
	double *smpHQ = m_smpHQ.GetData();

	size_t k;
	for (k = 0; k < allsamples; k++) {
		smpHQ[k] = (double)samples[k] * 32768;
	}

	int tries = 0;

redo:
	const double factor = (double)m_vol / (double)(1 << m_stepping);
	for (k = 0; k < allsamples; k++) {
		bufHQ[k] = smpHQ[k] * factor;
	}

	double highest = 0.0;
	for (k = 0; k < allsamples; k++) {
		highest = (std::max)(highest, fabs(bufHQ[k]));
	}

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
		} else if (highest <= t) {
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

	for (k = 0; k < allsamples; k++) {
		samples[k] = (float)((bufHQ[k] + 0.5) / 32768);
	}

	if (m_boost == false) {
		if (m_vol >= (1 << m_stepping)) m_vol = 1 << m_stepping;
	}

	return numsamples;
}

int CAudioNormalizer::Process(float *samples, unsigned numsamples, unsigned nch)
{
	int ret = 0;

	while (numsamples > 0) {
		const unsigned process = min(numsamples, 512);

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
		const int tmp = m_stepping;

		m_stepping = Steping;
		m_vol = (m_vol << m_stepping) / (1 << tmp);
	}
}

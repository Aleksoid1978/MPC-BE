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

#include "stdafx.h"
#include <math.h>
#include "AudioNormalizer.h"

template<class T>
static inline const T &limit(const T &val,const T &min,const T &max)
{
	if(val < min) return min;
	else if(val > max) return max;
	else return val;
}

//
// CAudioNormalizer
//

CAudioNormalizer::CAudioNormalizer(void)
{
	m_drangehead = 0;
	m_vol_head = 0;
	m_SMP = 0;
	m_predictor = 0;
	m_pred_good = 0;
	m_pred_bad = 0;

	ZeroMemory(m_vol_level, sizeof(m_vol_level));
	ZeroMemory(m_drange, sizeof(m_drange));
	ZeroMemory(m_drange, sizeof(m_drange));
	memset(m_prediction, 7, sizeof(m_prediction));

	m_level = 75;
	m_stepping = 8;
	m_boost = true;
	m_vol = 1 << m_stepping;

	m_rising = 0;

#if AUTOVOLUME
	m_normalize_level = 0.25;
	m_silence_level = 0.01;
	m_max_mult = 5.0;
	m_do_compress = false;
	m_cutoff = 13000.0;
	m_degree = 2.0;
	ZeroMemory(m_smooth, sizeof(m_smooth));
	for (size_t i = 0; i < _countof(m_smooth); i++)
	{
		m_smooth[i] = SmoothNew(100);
	}
#endif
}

CAudioNormalizer::~CAudioNormalizer(void)
{
#if AUTOVOLUME
	for (size_t i = 0; i < _countof(m_smooth); i++)
	{
		if (m_smooth[i]) SmoothDelete(m_smooth[i]);
	}
#endif
}

int CAudioNormalizer::MSteadyLQ(void *samples, int numsamples, int nch, bool IsFloat)
{
	int x, y, t, lowest, dhighest, highest, tries;
	short *bufLQ;
	int *smpLQ;
	float *samples32 = (float *)samples;
	short *samples16 = (short *)samples;

	tries = 0;
	if (m_vol <= 0) m_vol = 1;

	y = numsamples * nch;
	if ((int)m_bufLQ.GetCount() < y)
	{
		m_bufLQ.SetCount(y);
		m_smpLQ.SetCount(y);
	}
	bufLQ = m_bufLQ.GetData();
	smpLQ = m_smpLQ.GetData();

	if (IsFloat)
	{
		for (x = 0; x < y; x++) smpLQ[x] = (int)(samples32[x] * 32768);
	}

redo:
	if (IsFloat)
	{
		for (x = 0; x < y; x++) bufLQ[x] = (short int)((((int)smpLQ[x] * (int)m_vol) + (1 << (m_stepping - 1))) >> m_stepping);
	}
	else
	{
		for (x = 0; x < y; x++) bufLQ[x] = (short int)((((int)samples16[x] * (int)m_vol) + (1 << (m_stepping - 1))) >> m_stepping);
	}

	lowest = 32767;
	dhighest = 0;
	highest = 0;
	for (x = 0; x < y; x++)
	{
		int samp;

		if (IsFloat) samp = samples16[x];
		else samp = smpLQ[x];

		if (lowest > samp) lowest = samp;
		if (dhighest < samp) dhighest = samp;
		if (bufLQ[x] < 0)
		{
			if (-bufLQ[x] > highest) highest = -bufLQ[x];
		}
		if (bufLQ[x] > highest) highest = bufLQ[x];
	}

	if (highest > 30000)
	{
		if (m_vol > m_stepping) m_vol -= m_stepping;
		else m_vol = 1;
		tries++;
		if (tries < 1024) goto redo;
		else return numsamples;
	}
	m_drange[m_drangehead] = (dhighest - lowest) / 65536.0;
	m_drangehead++;
	m_drangehead = m_drangehead % (sizeof(m_drange) / sizeof(m_drange[0]));

	m_vol_level[m_vol_head] = (highest * 100) / 32767;
	m_vol_head = (m_vol_head + 1) % (sizeof(m_vol_level) / sizeof(m_vol_level[0]));

	if (highest > 10)
	{
		x = (m_level << 15) / 100;
		if (highest > x)
		{
			if (m_prediction[m_predictor] > 0) m_prediction[m_predictor] = m_prediction[m_predictor] - 1;
			else m_prediction[m_predictor] = 0;
			t = m_prediction[m_predictor];
			m_predictor >>= 1;
			m_rising = 0;
			if (t <= 7 && m_vol > 1)
			{
				m_vol--;
				m_pred_good++;
			}
			else m_pred_bad++;
		}
		else if (highest <= x)
		{
			if (m_prediction[m_predictor] < 15) m_prediction[m_predictor] = m_prediction[m_predictor] + 1;
			else m_prediction[m_predictor] = 15;
			t = m_prediction[m_predictor];
			m_predictor = (m_predictor >> 1) | 0x800;
			if (t >= 8)
			{
				m_vol += (++m_rising < 128) ? 1 : m_stepping;
				++m_pred_good;
			}
			else m_pred_bad++;
		}
	}

	if (IsFloat)
	{
		for (x = 0; x < y; x++) samples32[x] = (float)(bufLQ[x] / 32768.0);
	}
	else
	{
		CopyMemory(samples, bufLQ, y * sizeof(short));
	}
	if (m_boost == false)
	{
		if (m_vol >= (1 << m_stepping)) m_vol = 1 << m_stepping;
	}
	return numsamples;
}

int CAudioNormalizer::MSteadyHQ(void *samples, int numsamples, int nch, bool IsFloat)
{
	int x, y, tt, tries, lowest, dhighest;
	double highest, t, tmp;
	double *bufHQ;
	double *smpHQ;
	float *samples32 = (float *)samples;
	short *samples16 = (short *)samples;

	tries = 0;

	if (m_vol <= 0) m_vol = 1;

	y = numsamples * nch;
	if ((int)m_bufHQ.GetCount() < y)
	{
		m_bufHQ.SetCount(y);
		m_smpHQ.SetCount(y);
	}
	bufHQ = m_bufHQ.GetData();
	smpHQ = m_smpHQ.GetData();

	if (IsFloat)
	{
		for (x = 0; x < y; x++) smpHQ[x] = samples32[x] * 32768;
	}
	else
	{
		for (x = 0; x < y; x++) smpHQ[x] = samples16[x];
	}

redo:
	tmp = (double)m_vol / (double)(1 << m_stepping);
	for (x = 0; x < y; x++) bufHQ[x] = smpHQ[x] * tmp;

	lowest = 32767;
	dhighest = 0;
	highest = 0;
	for (x = 0; x < y; x++)
	{
		int samp;

		//		if(IsFloat) samp = (int)(samples32[x] * 32768);
		//		else samp = samples16[x];
		samp = (int)smpHQ[x];
		if (samp > dhighest) dhighest = samp;
		if (samp < lowest) lowest = samp;
		if (bufHQ[x] < 0)
		{
			if (-bufHQ[x] > highest) highest = -bufHQ[x];
		}
		else if (bufHQ[x] > highest) highest = bufHQ[x];
	}

	if (highest > 30000)
	{
		if (m_vol > m_stepping) m_vol -= m_stepping;
		else m_vol = 1;
		tries++;
		if (tries < 1024) goto redo;
		else return numsamples;
	}

	m_drange[m_drangehead] = (dhighest - lowest) / 65536.0;
	m_drangehead++;
	m_drangehead = m_drangehead % (sizeof(m_drange) / sizeof(m_drange[0]));

	m_vol_level[m_vol_head] = (int)((highest * 100) / 32767);
	m_vol_head = (m_vol_head + 1) % (sizeof(m_vol_level) / sizeof(m_vol_level[0]));

	if (highest > 10)
	{
		t = (m_level << 15) / 100.0;
		if (highest > t)
		{
			if (m_prediction[m_predictor] > 0) m_prediction[m_predictor] = m_prediction[m_predictor] - 1;
			else m_prediction[m_predictor] = 0;
			tt = m_prediction[m_predictor];
			m_predictor >>= 1;
			m_rising = 0;
			if (tt <= 7 && m_vol > 1)
			{
				m_vol--;
				m_pred_good++;
			}
			else m_pred_bad++;
		}
		else if (highest <= t)
		{
			if (m_prediction[m_predictor] < 15) m_prediction[m_predictor] = m_prediction[m_predictor] + 1;
			else m_prediction[m_predictor] = 15;
			tt = m_prediction[m_predictor];
			m_predictor = (m_predictor >> 1) | 0x800;
			if (tt >= 8)
			{
				m_vol += (++m_rising < 128) ? 1 : m_stepping;
				++m_pred_good;
			}
			else m_pred_bad++;
		}
	}

	if (IsFloat)
	{
		for (x = 0; x < y; x++) samples32[x] = (float)((bufHQ[x] + 0.5) / 32768);
	}
	else
	{
		for (x = 0; x < y; x++) samples16[x] = (short int)(floor(bufHQ[x] + 0.5));
	}
	if (m_boost == false)
	{
		if (m_vol >= (1 << m_stepping)) m_vol = 1 << m_stepping;
	}
	return numsamples;
}

int CAudioNormalizer::MSteadyLQ16(short *samples, int numsamples, int nch)
{
	int ret = 0;

	while (numsamples > 0)
	{
		int process = numsamples;

		if (process > 512) process = 512;
		ret += MSteadyLQ(samples, process, nch, false);
		numsamples -= process;
		samples += (process * nch);
	}

	return ret;
}

int CAudioNormalizer::MSteadyLQ32(float *samples, int numsamples, int nch)
{
	int ret = 0;

	while (numsamples > 0)
	{
		int process = numsamples;

		if (process > 512) process = 512;
		ret += MSteadyLQ(samples, process, nch, true);
		numsamples -= process;
		samples += (process * nch);
	}

	return ret;
}

int CAudioNormalizer::MSteadyHQ16(short *samples, int numsamples, int nch)
{
	int ret = 0;

	while (numsamples > 0)
	{
		int process = numsamples;

		if (process > 512) process = 512;
		ret += MSteadyHQ(samples, process, nch, false);
		numsamples -= process;
		samples += (process * nch);
	}

	return ret;
}

int CAudioNormalizer::MSteadyHQ32(float *samples, int numsamples, int nch)
{
	int ret = 0;

	while (numsamples > 0)
	{
		int process = numsamples;

		if (process > 512) process = 512;
		ret += MSteadyHQ(samples, process, nch, true);
		numsamples -= process;
		samples += (process * nch);
	}

	return ret;
}

void CAudioNormalizer::SetParam(int Level, bool Boost, int Steping)
{
	m_level = Level;
	m_boost = Boost;
	if (m_stepping != Steping)
	{
		int x = m_stepping;

		m_stepping = Steping;
		m_vol = (m_vol << m_stepping) / (1 << x);
	}
}

#if AUTOVOLUME
int CAudioNormalizer::AutoVolume(short *samples, int numsamples, int nch)
{
	double level = -1.0;

	calc_power_level(samples, numsamples, nch);
	{
		int channel = 0;

		level = -1.0;
		for (channel = 0; channel < nch; ++channel)
		{
			double channel_level = SmoothGetMax(m_smooth[channel]);

			if (channel_level > level) level = channel_level;
		}
	}

	if (level > m_silence_level)
	{
		double gain = m_normalize_level / level;

		if (gain > m_max_mult) gain = m_max_mult;

		adjust_gain(samples, numsamples, nch, gain);
	}

	return numsamples;
}

void CAudioNormalizer::calc_power_level(short *samples, int numsamples, int nch)
{
	int channel = 0;
	int i = 0;
	double sum[8];
	short *data = samples;

	for (channel = 0; channel < nch; ++channel)
	{
		sum[channel] = 0.0;
	}

	for (i = 0, channel = 0; i < numsamples * nch; ++i, ++data)
	{
		double sample = *data;
		double temp = 0.0;

		if (m_do_compress)
		{
			if (sample > m_cutoff) sample = m_cutoff + (sample - m_cutoff) / m_degree;
		}

		temp = sample*sample;

		sum[channel] += temp;

		++channel;
		channel = channel % nch;
	}

	{
		static const double NORMAL = 1.0 / ((double)MAXSHORT);
		static const double NORMAL_SQUARED = NORMAL * NORMAL;
		double channel_length = 2.0 / (numsamples * nch);

		for (channel = 0; channel < nch; ++channel)
		{
			double level = sum[channel] * channel_length * NORMAL_SQUARED;

			SmoothAddSample(m_smooth[channel], sqrt(level));
		}
	}
}

void CAudioNormalizer::adjust_gain(short *samples, int numsamples, int nch, double gain)
{
	short *data = samples;
	int i = 0;
#define NO_GAIN 0.01

	if (gain >= 1.0 - NO_GAIN && gain <= 1.0 + NO_GAIN) return;

	for (i = 0; i < numsamples * nch; ++i, ++data)
	{
		double samp = (double)*data;

		if (m_do_compress)
		{
			if (samp > m_cutoff) samp = m_cutoff + (samp - m_cutoff) / m_degree;
		}
		samp *= gain;
		*data = (short)limit(samp, (double)(short)MINSHORT, (double)(short)MAXSHORT);
	}
}

CAudioNormalizer::smooth_t *CAudioNormalizer::SmoothNew(int size)
{
	smooth_t * sm = (smooth_t *)malloc(sizeof(smooth_t));
	if (sm == NULL) return NULL;

	ZeroMemory(sm, sizeof(smooth_t));

	sm->data = (double *)malloc(size * sizeof(double));
	if (sm->data == NULL)
	{
		free(sm);
		return NULL;
	}
	ZeroMemory(sm->data, size * sizeof(double));

	sm->size = size;
	sm->current = sm->used = 0;
	sm->max = 0.0;
	return sm;
}

void CAudioNormalizer::SmoothDelete(smooth_t * del)
{
	if (del == NULL)
		return;

	if (del->data != NULL) free(del->data);

	free(del);
}

void CAudioNormalizer::SmoothAddSample(smooth_t * sm, double sample)
{
	if (sm == NULL)	return;

	sm->data[sm->current] = sample;

	++sm->current;

	if (sm->current > sm->used)	++sm->used;
	if (sm->current >= sm->size) sm->current %= sm->size;
}

double CAudioNormalizer::SmoothGetMax(smooth_t * sm)
{
	if (sm == NULL) return -1.0;

	{
		int i = 0;
		double smoothed = 0.0;

		for (i = 0; i < sm->used; ++i) smoothed += sm->data[i];
		smoothed = smoothed / sm->used;

		if (sm->used < sm->size) return (smoothed * sm->used + m_normalize_level * (sm->size - sm->used)) / sm->size;

		if (sm->max < smoothed) sm->max = smoothed;
	}

	return sm->max;
}
#endif

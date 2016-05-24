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

#include <atlcoll.h>

class CAudioNormalizer
{
protected:
	int m_level;
	bool m_boost;
	int m_stepping;
	int m_rising;
	int m_drangehead;
	int m_vol_level[64];
	int m_vol_head;

	CAtlArray<short> m_bufLQ;
	CAtlArray<int> m_smpLQ;

	CAtlArray<double> m_bufHQ;
	CAtlArray<double> m_smpHQ;
	double m_drange[64];
	double m_SMP;

	BYTE m_prediction[4096];
	DWORD m_predictor;
	DWORD m_pred_good;
	DWORD m_pred_bad;
	int m_vol;

	struct smooth_t {
		double * data;
		double max;
		int size;
		int used;
		int current;
	};

	smooth_t *m_smooth[10];
	double m_normalize_level;
	double m_silence_level;
	double m_max_mult;
	bool m_do_compress;
	double m_cutoff;
	double m_degree;

	void calc_power_level(short *samples, int numsamples, int nch);
	void adjust_gain(short *samples, int numsamples, int nch, double gain);
	smooth_t *SmoothNew(int size);
	void SmoothDelete(smooth_t * del);
	void SmoothAddSample(smooth_t * sm, double sample);
	double SmoothGetMax(smooth_t * sm);

	int MSteadyLQ(void *samples, int numsamples, int nch, bool IsFloat);
	int MSteadyHQ(void *samples, int numsamples, int nch, bool IsFloat);

public:
	void SetParam(int Level, bool Boost, int Steping);
	int MSteadyLQ16(short *samples, int numsamples, int nch);
	int MSteadyLQ32(float *samples, int numsamples, int nch);
	int MSteadyHQ16(short *samples, int numsamples, int nch);
	int MSteadyHQ32(float *samples, int numsamples, int nch);

	int AutoVolume(short *samples, int numsamples, int nch);

public:
	CAudioNormalizer(void);
	virtual ~CAudioNormalizer(void);
};

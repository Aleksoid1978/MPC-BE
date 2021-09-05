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

#pragma once

class CAudioNormalizer
{
protected:
	int  m_level = 75;
	bool m_boost =true;
	int  m_stepping = 8;
	int  m_stepping_vol = 1 << m_stepping;
	int  m_rising = 0;

	int m_vol = m_stepping_vol;

	unsigned m_predictor = 0;
	BYTE m_prediction[4096];

	int ProcessInternal(float *samples, unsigned numsamples, unsigned nch);

public:
	CAudioNormalizer();
	virtual ~CAudioNormalizer();

	void SetParam(int Level, bool Boost, int Steping);
	int Process(float *samples, unsigned numsamples, unsigned nch);
};

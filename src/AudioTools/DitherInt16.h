/*
 * (C) 2021 see Authors.txt
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

#include <array>
#include <random>

// converts to Int16 with dither if necessary
class CDitherInt16
{
	SampleFormat m_sf      = SAMPLE_FMT_NONE;
	uint32_t     m_layout  = 0;
	int          m_chanels = 0;

	CSimpleBuffer<float> m_simpleBuffer;

	std::array<float, 18> m_previous;
	std::array<std::minstd_rand, 18> m_generator;
	std::array<std::uniform_real_distribution<float>, 18> m_distributor;

	void Initialize();

public:
	void UpdateInput(const SampleFormat sf, const int chanels);

	void ProcessFloat(int16_t* pDst, float* pSrc, const int samples);
	void Process(int16_t* pDst, BYTE* pSrc, const int samples);
};

/*
 * (C) 2016-2017 see Authors.txt
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

#include "stdafx.h"

namespace TimecodeAnalyzer
{
	// A sufficient number of frames for calculating the average fps in most cases.
	const unsigned DefaultFrameNum = 120;


	bool GetMonotoneInterval(std::vector<int64_t>& timecodes, int64_t& interval, unsigned& num);

	REFERENCE_TIME CalculateFrameTime(std::vector<int64_t>& timecodes, const int64_t timecodescaleRF);
	double CalculateFPS(std::vector<int64_t>& timecodes, const int64_t timecodescale);
}

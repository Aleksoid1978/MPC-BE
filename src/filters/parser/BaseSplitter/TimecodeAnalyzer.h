/*
 * (C) 2016-2022 see Authors.txt
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
	double FrameRate_RoundToStandard(double FrameRate);
	REFERENCE_TIME FrameDuration_RoundToStandard(REFERENCE_TIME FrameDuration);

	// A sufficient number of frames for calculating the average fps in most cases.
	const unsigned DefaultFrameNum = 120;

	bool GetMonotoneInterval(std::vector<int64_t>& timecodes, uint64_t& interval, unsigned& num, const int feeze = 1);

	REFERENCE_TIME CalculateFrameTime(std::vector<int64_t>& timecodes, const unsigned timecodescaleRF, const int feeze = 1);
	double CalculateFPS(std::vector<int64_t>& timecodes, const unsigned timecodespersecond, const int feeze = 1);
}

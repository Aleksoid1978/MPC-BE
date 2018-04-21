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

#include "stdafx.h"
#include <vector>
#include "TimecodeAnalyzer.h"

static double Video_FrameRate_Rounding(double FrameRate)
{
	// rounded up to the standard values if the difference is not more than 0.05%
	     if (FrameRate > 14.993 && FrameRate <  15.008) FrameRate = 15;
	else if (FrameRate > 23.964 && FrameRate <  23.988) FrameRate = 24/1.001;
	else if (FrameRate > 23.988 && FrameRate <  24.012) FrameRate = 24;
	else if (FrameRate > 24.988 && FrameRate <  25.013) FrameRate = 25;
	else if (FrameRate > 29.955 && FrameRate <  29.985) FrameRate = 30/1.001;
	else if (FrameRate > 29.985 && FrameRate <  30.015) FrameRate = 30;
	else if (FrameRate > 47.928 && FrameRate <  47.976) FrameRate = 48/1.001;
	else if (FrameRate > 47.976 && FrameRate <  48.024) FrameRate = 48;
	else if (FrameRate > 49.975 && FrameRate <  50.025) FrameRate = 50;
	else if (FrameRate > 59.910 && FrameRate <  59.970) FrameRate = 60/1.001;
	else if (FrameRate > 59.970 && FrameRate <  60.030) FrameRate = 60;
	else if (FrameRate > 99.950 && FrameRate < 100.050) FrameRate = 100;
	else if (FrameRate >119.820 && FrameRate < 119.940) FrameRate = 120/1.001;
	else if (FrameRate >119.940 && FrameRate < 120.060) FrameRate = 120;

	return FrameRate;
}

static REFERENCE_TIME Video_FrameDuration_Rounding(REFERENCE_TIME FrameDuration)
{
	// rounded up to the standard values if the difference is not more than 0.05%
	     if (FrameDuration > 666333 && FrameDuration < 667000) FrameDuration = UNITS / 15;
	else if (FrameDuration > 416874 && FrameDuration < 417291) FrameDuration = UNITS * 1001 / 24000;
	else if (FrameDuration > 416458 && FrameDuration < 416874) FrameDuration = UNITS / 24;
	else if (FrameDuration > 399800 && FrameDuration < 400200) FrameDuration = UNITS / 25;
	else if (FrameDuration > 333499 && FrameDuration < 333833) FrameDuration = UNITS * 1001 / 30000;
	else if (FrameDuration > 333166 && FrameDuration < 333499) FrameDuration = UNITS / 30;
	else if (FrameDuration > 208437 && FrameDuration < 208645) FrameDuration = UNITS * 1001 / 48000;
	else if (FrameDuration > 208229 && FrameDuration < 208437) FrameDuration = UNITS / 48;
	else if (FrameDuration > 199900 && FrameDuration < 200100) FrameDuration = UNITS / 50;
	else if (FrameDuration > 166749 && FrameDuration < 166916) FrameDuration = UNITS * 1001 / 60000;
	else if (FrameDuration > 166583 && FrameDuration < 166749) FrameDuration = UNITS / 60;
	else if (FrameDuration >  99950 && FrameDuration < 100050) FrameDuration = UNITS / 100;
	else if (FrameDuration >  83374 && FrameDuration <  83458) FrameDuration = UNITS * 1001 / 120000;
	else if (FrameDuration >  83291 && FrameDuration <  83374) FrameDuration = UNITS / 120;

	return FrameDuration;
}

bool TimecodeAnalyzer::GetMonotoneInterval(std::vector<int64_t>& timecodes, int64_t& interval, unsigned& num)
{
	if (timecodes.size() < 2) {
		return false;
	}

	std::sort(timecodes.begin(), timecodes.end());

	std::vector<int> durations;
	durations.reserve(timecodes.size() - 1);

	// calculate durations, discard the zero durations, correct the ranges with duration equal to 1.
	unsigned k = 0;
	for (size_t i = 1; i < timecodes.size(); i++) {
		const int64_t diff = timecodes[i] - timecodes[i - 1];
		if (diff > 0 && diff < INT_MAX) {
			if (diff == 1) {
				// count values equal to 1
				k++;
			}
			else if (k > 0 && k < durations.size()) {
				// fill values equal to 1 due to the previous value
				size_t j = durations.size() - k - 1;
				int d = durations[j] + k;
				k += 1;
				while (k) {
					durations[j] = d / k--;
					d -= durations[j++];
				}
			}

			durations.push_back((int)diff);
		}
	}

	if (durations.empty()) {
		return false;
	}

	int64_t longsum = 0;
	int longcount = 0;

	int64_t sum = durations[0];
	int count = 1;
	for (size_t i = 1; i < durations.size(); i++) {
		if (abs(durations[i - 1] - durations[i]) <= 1) {
			sum += durations[i];
			count++;

			if (count > longcount) {
				longsum = sum;
				longcount = count;
			}
		}
		else {
			sum = durations[i];
			count = 1;
		}
	}

	interval = longsum;
	num = longcount;

	return true;
}

REFERENCE_TIME TimecodeAnalyzer::CalculateFrameTime(std::vector<int64_t>& timecodes, const int64_t timecodescaleRF)
{
	int64_t interval;
	unsigned num;

	if (GetMonotoneInterval(timecodes, interval, num) && num >= 10) {
		return Video_FrameDuration_Rounding((interval * timecodescaleRF) / num);
	}

	return 417083;
}

double TimecodeAnalyzer::CalculateFPS(std::vector<int64_t>& timecodes, const int64_t timecodescale)
{
	int64_t interval;
	unsigned num;

	if (GetMonotoneInterval(timecodes, interval, num) && num >= 10) {
		return Video_FrameRate_Rounding((double)num * timecodescale / interval);
	}

	return 24/1.001;
}

/*
 * (C) 2016 see Authors.txt
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
#include <algorithm>
#include "MatroskaSplitter.h"

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

//
// CMatroskaSplitterFilter
//

#define MAXTESTEDFRAMES 120

REFERENCE_TIME CMatroskaSplitterFilter::CalcFrameDuration(CUInt trackNumber)
{
	DbgLog((LOG_TRACE, 3, L"CMatroskaSplitterFilter::CalcFrameDuration() : calculate AvgTimePerFrame"));
	REFERENCE_TIME FrameDuration = 0;

	CMatroskaNode Root(m_pFile);
	m_pSegment = Root.Child(MATROSKA_ID_SEGMENT);
	m_pCluster = m_pSegment->Child(MATROSKA_ID_CLUSTER);

	std::vector<INT64> timecodes;
	timecodes.reserve(MAXTESTEDFRAMES);

	bool readmore = true;

	do {
		Cluster c;
		c.ParseTimeCode(m_pCluster);

		if (CAutoPtr<CMatroskaNode> pBlock = m_pCluster->GetFirstBlock()) {
			do {
				CBlockGroupNode bgn;

				if (pBlock->m_id == MATROSKA_ID_BLOCKGROUP) {
					bgn.Parse(pBlock, true);
				}
				else if (pBlock->m_id == MATROSKA_ID_SIMPLEBLOCK) {
					CAutoPtr<BlockGroup> bg(DNew BlockGroup());
					bg->Block.Parse(pBlock, true);
					bgn.AddTail(bg);
				}

				POSITION pos4 = bgn.GetHeadPosition();
				while (pos4) {
					BlockGroup* bg = bgn.GetNext(pos4);
					if (bg->Block.TrackNumber != trackNumber) {
						continue;
					}
					INT64 tc = c.TimeCode + bg->Block.TimeCode;

					DbgLog((LOG_TRACE, 3, L"	=> Frame: %3d, TimeCode: %5I64d = %10I64d", timecodes.size(), tc, m_pFile->m_segment.GetRefTime(tc)));
					timecodes.push_back(tc);

					if (timecodes.size() >= MAXTESTEDFRAMES) {
						readmore = false;
						break;
					}
				}
			} while (readmore && pBlock->NextBlock());
		}
	} while (readmore && m_pCluster->Next(true));

	m_pCluster.Free();

	if (timecodes.size()) {
		std::sort(timecodes.begin(), timecodes.end());

		// calculate the average fps
		double fps = 1000000000.0 * (timecodes.size() - 1) / (m_pFile->m_segment.SegmentInfo.TimeCodeScale * (timecodes.back() - timecodes.front()));

		std::vector<int> frametimes;
		frametimes.reserve(MAXTESTEDFRAMES - 1);

		unsigned k = 0;
		for (size_t i = 1; i < timecodes.size(); i++) {
			INT64 diff = timecodes[i] - timecodes[i-1];
			if (diff > 0 && diff < INT_MAX) {

				if (diff == 1) {
					// calculate values equal to 1
					k++;
				}
				else if (k > 0 && k < frametimes.size()) {
					// fill values equal to 1 due to the previous value
					size_t j = frametimes.size() - k - 1;
					int d = frametimes[j] + k;
					k += 1;
					while (k) {
						frametimes[j] = d / k--;
						d -= frametimes[j++];
					}
				}

				frametimes.push_back((int)diff);
			}
		}

		if (frametimes.size()) {
			int longsum = 0;
			int longcount = 0;

			int sum = frametimes[0];
			int count = 1;
			for (size_t i = 1; i < frametimes.size(); i++) {
				if (abs(frametimes[i-1] - frametimes[i]) <= 1) {
					sum += frametimes[i];
					count++;

					if (count > longcount) {
						longsum = sum;
						longcount = count;
					}
				} else {
					sum = frametimes[i];
					count = 1;
				}
			}

			if (longsum && longcount >= 10) {
				fps = 1000000000.0 * longcount / (m_pFile->m_segment.SegmentInfo.TimeCodeScale * longsum);
			}
		}

		fps = Video_FrameRate_Rounding(fps);

		FrameDuration = 10000000.0 / fps;
	}

	return FrameDuration;
}

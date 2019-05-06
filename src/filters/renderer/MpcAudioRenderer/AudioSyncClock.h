/*
 * (C) 2018-2019 see Authors.txt
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

#include <AudioClient.h>

#define OneMillisecond 10000

class CAudioSyncClock: public CBaseReferenceClock
{
public:
	CAudioSyncClock(LPUNKNOWN pUnk, HRESULT* phr);

	DECLARE_IUNKNOWN

	REFERENCE_TIME GetPrivateTime() override;

	void Slave(IAudioClock* pAudioClock, const REFERENCE_TIME audioStart);
	void UnSlave();
	const bool IsSlave() const { return m_pAudioClock != nullptr; }
	void OffsetAudioClock(const REFERENCE_TIME offsetTime);

private:
	IAudioClock* m_pAudioClock = nullptr;
	REFERENCE_TIME m_audioStart = 0;
	REFERENCE_TIME m_audioOffset = 0;
	REFERENCE_TIME m_counterOffset = 0;
	UINT64 m_audioStartPosition = 0;

	HRESULT GetAudioClockTime(REFERENCE_TIME* pAudioTime, REFERENCE_TIME* pCounterTime);
};

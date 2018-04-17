/*
 * (C) 2018 see Authors.txt
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
#include "AudioSyncClock.h"
#include "../../../DSUtil/DSUtil.h"

CAudioSyncClock::CAudioSyncClock(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseReferenceClock(L"MPC Audio Sync Clock", pUnk, phr)
{
}

REFERENCE_TIME CAudioSyncClock::GetPrivateTime()
{
	CAutoLock lock(this);

#ifdef DEBUG
	const REFERENCE_TIME oldCounterOffset = m_counterOffset;
#endif

	REFERENCE_TIME audioClockTime, counterTime, clockTime;
	if (SUCCEEDED(GetAudioClockTime(&audioClockTime, &counterTime))) {
		clockTime = audioClockTime;
		m_counterOffset = audioClockTime - counterTime;
	} else {
		clockTime = m_counterOffset + GetPerfCounter();
	}

#ifndef NDEBUG
	const REFERENCE_TIME counterOffsetDiff = m_counterOffset - oldCounterOffset;
	if (std::abs(counterOffsetDiff) > (UNITS / MILLISECONDS) * 5) {
		DLog(L"CAudioSyncClock::GetPrivateTime() : jitter %.2f ms", counterOffsetDiff / 10000.0);
	}
#endif

	return clockTime;
}

void CAudioSyncClock::Slave(IAudioClock* pAudioClock, const REFERENCE_TIME audioStart)
{
	ASSERT(pAudioClock);

	CAutoLock lock(this);

	m_pAudioClock = pAudioClock;
	m_audioStart = audioStart;
	m_audioOffset = 0;
	m_audioStartPosition = 0;
	m_pAudioClock->GetPosition(&m_audioStartPosition, nullptr);
}

void CAudioSyncClock::UnSlave()
{
	CAutoLock lock(this);

	m_pAudioClock = nullptr;
}

void CAudioSyncClock::OffsetAudioClock(const REFERENCE_TIME offsetTime)
{
	CAutoLock lock(this);

	m_audioOffset += offsetTime;
}

HRESULT CAudioSyncClock::GetAudioClockTime(REFERENCE_TIME* pAudioTime, REFERENCE_TIME* pCounterTime)
{
    CheckPointer(pAudioTime, E_POINTER);

    CAutoLock lock(this);

	if (m_pAudioClock) {
		UINT64 audioFrequency, audioPosition, audioTime;
		if (SUCCEEDED(m_pAudioClock->GetFrequency(&audioFrequency))
				&& SUCCEEDED(m_pAudioClock->GetPosition(&audioPosition, &audioTime))
				&& audioPosition > m_audioStartPosition) {
			const LONGLONG counterTime = GetPerfCounter();
			const LONGLONG clockTime   = llMulDiv(audioPosition - m_audioStartPosition, UNITS, audioFrequency, 0)
										 + m_audioStart + m_audioOffset + counterTime - audioTime;

			*pAudioTime = clockTime;

			if (pCounterTime)
				*pCounterTime = counterTime;

			return S_OK;
		}

		return E_FAIL;
	}

	return E_FAIL;
}

/*
 * (C) 2006-2021 see Authors.txt
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

#include <deque>
#include <mutex>
#include <mpc_defines.h>

#define PACKET_AAC_RAW 0x0001

 // CPacket

class CPacket : public std::vector<BYTE>
{
public:
	DWORD TrackNumber      = 0;
	BOOL bDiscontinuity    = FALSE;
	BOOL bSyncPoint        = FALSE;
	REFERENCE_TIME rtStart = INVALID_TIME;
	REFERENCE_TIME rtStop  = INVALID_TIME;
	AM_MEDIA_TYPE* pmt     = nullptr;

	UINT32 Flag            = 0;

	~CPacket();

	bool SetCount(const size_t newsize);
	void SetData(const CPacket& packet);
	void SetData(const void* ptr, const size_t size);
	void AppendData(const CPacket& packet);
	void AppendData(const void* ptr, const size_t size);
	void RemoveHead(const size_t size);
};

// CPacketQueue

class CPacketQueue
{
	std::mutex m_mutex;
	size_t m_size = 0;
	std::deque<CAutoPtr<CPacket>> m_deque;

public:
	void Add(CAutoPtr<CPacket>& p);
	CAutoPtr<CPacket> Remove();
	void RemoveSafe(CAutoPtr<CPacket>& p, size_t& count);
	void RemoveAll();
	const size_t GetCount();
	const size_t GetSize();
	const REFERENCE_TIME GetDuration();
};

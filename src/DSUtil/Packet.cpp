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

#include "stdafx.h"
#include "Packet.h"

//
// CPacket
//

CPacket::~CPacket()
{
	DeleteMediaType(pmt);
}

bool CPacket::SetCount(const size_t newsize)
{
	try {
		resize(newsize);
	}
	catch (...) {
		return false;
	}
	return true;
}

void CPacket::SetData(const CPacket& packet)
{
	*this = packet;
}

void CPacket::SetData(const void* ptr, const size_t size)
{
	resize(size);
	memcpy(data(), ptr, size);
}

void CPacket::AppendData(const CPacket& packet)
{
	insert(cend(), packet.cbegin(), packet.cend());
}

void CPacket::AppendData(const void* ptr, const size_t size)
{
	const size_t oldsize = this->size();
	resize(oldsize + size);
	memcpy(data() + oldsize, ptr, size);
}

void CPacket::RemoveHead(const size_t size)
{
	erase(begin(), begin() + size);
}

//
// CPacketQueue
//

void CPacketQueue::Add(CAutoPtr<CPacket>& p)
{
	std::unique_lock<std::mutex> lock(m_mutex);

	if (p) {
		m_size += p->size();
	}
	m_deque.emplace_back(p);
}

CAutoPtr<CPacket> CPacketQueue::Remove()
{
	std::unique_lock<std::mutex> lock(m_mutex);

	ASSERT(!m_deque.empty());
	CAutoPtr<CPacket> p = m_deque.front();
	m_deque.pop_front();
	if (p) {
		m_size -= p->size();
	}
	return p;
}

void CPacketQueue::RemoveSafe(CAutoPtr<CPacket>& p, size_t& count)
{
	std::unique_lock<std::mutex> lock(m_mutex);

	count = m_deque.size();
	if (count) {
		p = m_deque.front();
		m_deque.pop_front();
		if (p) {
			m_size -= p->size();
		}
	}
}

void CPacketQueue::RemoveAll()
{
	std::unique_lock<std::mutex> lock(m_mutex);

	m_size = 0;
	m_deque.clear();
}

const size_t CPacketQueue::GetCount()
{
	std::unique_lock<std::mutex> lock(m_mutex);

	return m_deque.size();
}

const size_t CPacketQueue::GetSize()
{
	std::unique_lock<std::mutex> lock(m_mutex);

	return m_size;
}

const REFERENCE_TIME CPacketQueue::GetDuration()
{
	std::unique_lock<std::mutex> lock(m_mutex);

	if (m_deque.size()) {
		return (m_deque.back()->rtStop - m_deque.front()->rtStart);
	}
	return 0;
}

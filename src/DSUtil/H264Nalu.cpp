/*
 * (C) 2006-2024 see Authors.txt
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
#include "H264Nalu.h"

constexpr uint32_t NALU_START_CODE      = 0x000001;
constexpr size_t   NALU_START_CODE_SIZE = 3;

void CH264Nalu::SetBuffer(const BYTE* pBuffer, const size_t nSize, const int nNALSize/* = 0*/)
{
	m_pBuffer      = pBuffer;
	m_nSize        = nSize;
	m_nNALSize     = nNALSize;
	m_nCurPos      = 0;
	m_nNextRTP     = 0;

	m_nNALStartPos = 0;
	m_nNALDataPos  = 0;

	if (!nNALSize && nSize) {
		MoveToNextAnnexBStartcode();
	}
}

bool CH264Nalu::MoveToNextAnnexBStartcode()
{
	m_nNALStartCodeSize = NALU_START_CODE_SIZE;

	if (m_nCurPos + NALU_START_CODE_SIZE <= m_nSize) {
		const auto nBuffEnd = m_nSize - NALU_START_CODE_SIZE;
		for (size_t i = m_nCurPos; i < nBuffEnd; i++) {
			auto CheckNaluStartCode = [](const BYTE* pBuffer) {
				return (static_cast<uint32_t>((pBuffer[0] << 16) | (pBuffer[1] << 8) | pBuffer[2])) == NALU_START_CODE;
			};

			if (CheckNaluStartCode(m_pBuffer + i)) {
				if (i > m_nCurPos && m_pBuffer[i - 1] == 0x00) {
					// 00 00 00 01
					m_nNALStartCodeSize = 4;
					i--;
				}
				m_nCurPos = i;
				return true;
			}
		}
	}

	m_nCurPos = m_nSize;
	return false;
}

bool CH264Nalu::MoveToNextRTPStartcode()
{
	if (m_nNextRTP < m_nSize) {
		m_nCurPos = m_nNextRTP;
		return true;
	}

	m_nCurPos = m_nSize;
	return false;
}

bool CH264Nalu::ReadNext()
{
	if (m_nCurPos >= m_nSize) {
		return false;
	}

	if ((m_nNALSize != 0) && (m_nCurPos == m_nNextRTP)) {
		if (m_nCurPos + m_nNALSize >= m_nSize) {
			return false;
		}

		// RTP Nalu type : (XX XX) XX XX NAL..., with XX XX XX XX or XX XX equal to NAL size
		m_nNALStartPos = m_nCurPos;
		m_nNALDataPos  = m_nCurPos + m_nNALSize;
		size_t nTemp = 0;
		for (int i = 0; i < m_nNALSize; i++) {
			nTemp = (nTemp << 8) + m_pBuffer[m_nCurPos++];
		}
		m_nNextRTP += nTemp + m_nNALSize;
		MoveToNextRTPStartcode();
	} else {
		if (m_nCurPos + m_nNALStartCodeSize >= m_nSize) {
			return false;
		}

		// AnnexB Nalu : 00 00 01 NAL...
		m_nNALStartPos  = m_nCurPos;
		m_nCurPos      += m_nNALStartCodeSize;
		m_nNALDataPos   = m_nCurPos;
		MoveToNextAnnexBStartcode();
	}

	forbidden_bit     = (m_pBuffer[m_nNALDataPos] >> 7) & 1;
	nal_reference_idc = (m_pBuffer[m_nNALDataPos] >> 5) & 3;
	nal_unit_type     = static_cast<NALU_TYPE>(m_pBuffer[m_nNALDataPos] & 0x1f);

	return true;
}

bool CH265Nalu::ReadNext()
{
	if (__super::ReadNext()) {
		nal_unit_type = static_cast<NALU_TYPE>((m_pBuffer[m_nNALDataPos] >> 1) & 0x3F);
		return true;
	}

	return false;
}

bool CH266Nalu::ReadNext()
{
	if (__super::ReadNext()) {
		nal_unit_type = static_cast<NALU_TYPE>((m_pBuffer[m_nNALDataPos + 1] >> 3) & 0x1f);
		return true;
	}

	return false;
}

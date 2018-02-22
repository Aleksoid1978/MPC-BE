/*
 * (C) 2006-2018 see Authors.txt
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
#include "GolombBuffer.h"
#include <mpc_defines.h>

static void RemoveMpegEscapeCode(BYTE* dst, const BYTE* src, int& length)
{
	memset(dst, 0, length);
	int si = 0;
	int di = 0;
	while (si + 2 < length) {
		if (src[si + 2] > 3) {
			dst[di++] = src[si++];
			dst[di++] = src[si++];
		} else if (src[si] == 0 && src[si + 1] == 0) {
			dst[di++] = 0;
			dst[di++] = 0;
			if (src[si + 2] == 3) { // escape
				si += 3;
			} else {
				si += 2;
			}
			continue;
		}

		dst[di++] = src[si++];
	}
	while (si < length) {
		dst[di++] = src[si++];
	}

	length = di;
}

CGolombBuffer::CGolombBuffer(const BYTE* pBuffer, int nSize, const bool bRemoveMpegEscapes/* = false*/)
	: m_bRemoveMpegEscapes(bRemoveMpegEscapes)
	, m_pTmpBuffer(nullptr)
{
	Reset(pBuffer, nSize);
}

CGolombBuffer::~CGolombBuffer()
{
	SAFE_DELETE_ARRAY(m_pTmpBuffer);
}

UINT64 CGolombBuffer::BitRead(const int nBits, const bool bPeek/* = false*/)
{
	//ASSERT(nBits >= 0 && nBits <= 64);
	const INT64 tmp_bitbuff = m_bitbuff;
	const int tmp_nBitPos   = m_nBitPos;
	const int tmp_bitlen    = m_bitlen;

	while (m_bitlen < nBits) {
		m_bitbuff <<= 8;

		if (m_nBitPos >= m_nSize) {
			return 0;
		}

		*(BYTE*)&m_bitbuff = m_pBuffer[m_nBitPos++];
		m_bitlen += 8;
	}

	const int bitlen = m_bitlen - nBits;

	UINT64 ret;
	// The shift to 64 bits can give incorrect results.
	// "The behavior is undefined if the right operand is negative, or greater than or equal to the length in bits of the promoted left operand."
	if (nBits == 64) {
		ret = m_bitbuff;
	} else {
		ret = (m_bitbuff >> bitlen) & ((1ui64 << nBits) - 1);
	}

	if (!bPeek) {
		m_bitbuff &= ((1ui64 << bitlen) - 1);
		m_bitlen  = bitlen;
	} else {
		m_bitbuff = tmp_bitbuff;
		m_nBitPos = tmp_nBitPos;
		m_bitlen  = tmp_bitlen;
	}

	return ret;
}

UINT64 CGolombBuffer::UExpGolombRead()
{
	int n = -1;
	for (BYTE b = 0; !b && !IsEOF(); n++) {
		b = (BYTE)BitRead(1);
	}
	return (1ui64 << n) - 1 + BitRead(n);
}

unsigned int CGolombBuffer::UintGolombRead()
{
	unsigned int value = 0, count = 0;
	while (!BitRead(1) && !IsEOF()) {
		count++;
		value <<= 1;
		value |= BitRead(1);
	}

	return (1 << count) - 1 + value;
}

INT64 CGolombBuffer::SExpGolombRead()
{
	UINT64 k = UExpGolombRead();
	return ((k&1) ? 1 : -1) * ((k + 1) >> 1);
}

void CGolombBuffer::BitByteAlign()
{
	m_bitlen &= ~7;
}

int CGolombBuffer::GetPos() const
{
	return m_nBitPos - (m_bitlen >> 3);
}

void CGolombBuffer::ReadBuffer(BYTE* pDest, int nSize)
{
	ASSERT(m_nBitPos + nSize <= m_nSize);
	ASSERT(m_bitlen == 0);
	nSize = std::min(nSize, m_nSize - m_nBitPos);

	memcpy(pDest, m_pBuffer + m_nBitPos, nSize);
	m_nBitPos += nSize;
}

void CGolombBuffer::Reset()
{
	m_nBitPos = 0;
	m_bitlen  = 0;
	m_bitbuff = 0;
}

void CGolombBuffer::Reset(const BYTE* pNewBuffer, int nNewSize)
{
	if (m_bRemoveMpegEscapes) {
		SAFE_DELETE_ARRAY(m_pTmpBuffer);
		m_pTmpBuffer = DNew BYTE[nNewSize];

		RemoveMpegEscapeCode(m_pTmpBuffer, pNewBuffer, nNewSize);
		m_pBuffer = m_pTmpBuffer;
		m_nSize   = nNewSize;
	} else {
		m_pBuffer = pNewBuffer;
		m_nSize   = nNewSize;
	}

	Reset();
}

void CGolombBuffer::SkipBytes(const int nCount)
{
	m_nBitPos += nCount;
	m_bitlen   = 0;
	m_bitbuff  = 0;
}

void CGolombBuffer::Seek(const int nCount)
{
	m_nBitPos = nCount;
	m_bitlen  = 0;
	m_bitbuff = 0;
}

bool CGolombBuffer::NextMpegStartCode(BYTE& code)
{
	BitByteAlign();
	DWORD dw = DWORD_MAX;
	do {
		if (IsEOF()) {
			return false;
		}
		dw = (dw << 8) | (BYTE)BitRead(8);
	} while ((dw & 0xffffff00) != 0x00000100);
	code = (BYTE)(dw & 0xff);

	return true;
}

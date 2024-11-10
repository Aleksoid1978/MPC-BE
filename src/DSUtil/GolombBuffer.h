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

#pragma once

#include <memory>

class CGolombBuffer
{
public:
	CGolombBuffer(const BYTE* pBuffer, int nSize, const bool bRemoveMpegEscapes = false);
	~CGolombBuffer() = default;

	UINT64       BitRead(const int nBits, const bool bPeek = false);
	UINT64       UExpGolombRead();
	unsigned int UintGolombRead();
	INT64        SExpGolombRead();
	void         BitByteAlign();

	inline BYTE  ReadByte()    { return (BYTE)BitRead(8); }
	inline SHORT ReadShort()   { return (SHORT)BitRead(16); }
	inline DWORD ReadDword()   { return (DWORD)BitRead(32); }
	inline SHORT ReadShortLE() { return _byteswap_ushort((SHORT)BitRead(16)); }
	inline DWORD ReadDwordLE() { return _byteswap_ulong((DWORD)BitRead(32)); }

	void         ReadBuffer(BYTE* pDest, int nSize);

	void         Reset();
	void         Reset(const BYTE* pNewBuffer, int nNewSize);

	void         SetSize(const int nValue) { m_nSize = nValue; }
	int          GetSize() const { return m_nSize; }
	int          RemainingSize() const { return m_nSize - m_nBitPos; }
	int          BitsLeft() const { return 8 * RemainingSize() + m_bitlen; }
	bool         IsEOF() const { return !m_bitlen && m_nBitPos >= m_nSize; }
	int          GetPos() const;
	int          GetBitsPos() const;
	const BYTE*  GetBufferPos() const { return m_pBuffer + m_nBitPos; }

	void         SkipBytes(const int nCount);
	void         Seek(const int nPos);

	bool         NextMpegStartCode(BYTE& code);

private :
	const BYTE*  m_pBuffer;
	int          m_nSize;
	int          m_nBitPos;
	int          m_bitlen;
	INT64        m_bitbuff;

	std::unique_ptr<BYTE[]> m_pTmpBuffer;
	bool         m_bRemoveMpegEscapes;
};

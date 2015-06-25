/*
 * (C) 2006-2014 see Authors.txt
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

class CGolombBuffer
{
public:
	CGolombBuffer(BYTE* pBuffer, int nSize, bool bRemoveMpegEscapes = false);
	~CGolombBuffer();

	UINT64			BitRead(int nBits, bool fPeek = false);
	UINT64			UExpGolombRead();
	unsigned int	UintGolombRead();
	INT64			SExpGolombRead();
	void			BitByteAlign();

	inline BYTE		ReadByte()		{ return (BYTE)BitRead (8); };
	inline SHORT	ReadShort()		{ return (SHORT)BitRead (16); };
	inline DWORD	ReadDword()		{ return (DWORD)BitRead (32); };
	inline SHORT	ReadShortLE()	{ return (SHORT)ReadByte() | (SHORT)ReadByte() << 8; };
	inline DWORD	ReadDwordLE()	{ return (DWORD)(ReadByte() | ReadByte() << 8 | ReadByte() << 16 | ReadByte() << 24); };
	void			ReadBuffer(BYTE* pDest, int nSize);

	void			Reset();
	void			Reset(BYTE* pNewBuffer, int nNewSize);

	void			SetSize(int nValue) { m_nSize = nValue; };
	int				GetSize() const     { return m_nSize; };
	int				RemainingSize() const { return m_nSize - m_nBitPos; };
	bool			IsEOF() const { return m_nBitPos >= m_nSize; };
	int				GetPos();
	BYTE*			GetBufferPos() { return m_pBuffer + m_nBitPos; };

	void			SkipBytes(int nCount);
	void			Seek(int nPos);

	bool			NextMpegStartCode(BYTE& code);

private :
	BYTE*			m_pBuffer;
	int				m_nSize;
	int				m_nBitPos;
	int				m_bitlen;
	INT64			m_bitbuff;

	BYTE*			m_pTmpBuffer;
	bool			m_bRemoveMpegEscapes;
};

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

#include "../../../DSUtil/DSUtil.h"

//-------------------------------------------------------------------------
//
//	Bitstream class
//
//-------------------------------------------------------------------------

class Bitstream
{
public:
	uint32	bitbuf;				// bitbuffer
	uint8	*buf;				// byte buffer
	int32	bits;				// bits in bitbuffer.

public:
	static const int32 EXP_GOLOMB_MAP[2][48];
	static const int32 EXP_GOLOMB_MAP_INV[2][48];
	static const int32 EXP_GOLOMB_SIZE[255];

public:
	Bitstream() : bitbuf(0), buf(NULL), bits(0) { };
	Bitstream(uint8 *b) : bitbuf(0), buf(b), bits(0) { };
	Bitstream(const Bitstream &b) : bitbuf(b.bitbuf), buf(b.buf), bits(b.bits) { };

	Bitstream &operator =(const Bitstream &b) { bitbuf = b.bitbuf; buf = b.buf; bits = b.bits; return *this; };
	Bitstream &operator =(const uint8 *b) { bitbuf = 0; buf = (uint8*)b; bits = 0; return *this; };
	Bitstream &operator =(uint8 *b) { bitbuf = 0; buf = b; bits = 0; return *this; };

	inline void Init(const uint8 *b) { bitbuf = 0; buf = (uint8*)b; bits = 0; };
	inline void Init(uint8 *b) { bitbuf = 0; buf = b; bits = 0; };

	inline int32 BitsLeft() { return bits; };
	inline uint32 BitBuf() { return bitbuf; };
	inline uint8 *Position() { return buf - (bits/8); };

	inline void DumpBits(int32 n) { bitbuf <<= n; bits -= n; };
	inline uint32 UBits(int32 n) { return (uint32)(bitbuf >> (32-n)); };
	inline uint32 UGetBits(int32 n) { uint32 val = (uint32)(bitbuf >> (32-n)); bitbuf <<= n; bits -= n; return val; };
	inline int32 SBits(int32 n) { return (int32)(bitbuf >> (32-n)); };
	inline int32 SGetBits(int32 n) { int32 val = (int32)(bitbuf >> (32-n)); bitbuf <<= n; bits -= n; return val; };
	inline void Markerbit() { DumpBits(1); }

	inline int64 GetMpcSize() {
		int64 ret=0;
		uint8 tmp;
		do {
			NeedBits();
			tmp = UGetBits(8);
			ret = (ret<<7) | (tmp&0x7f);
		} while (tmp&0x80);
		return ret;
	}

	inline int32 IsByteAligned() { return !(bits&0x07); };
	inline void ByteAlign() { if (bits&0x07) DumpBits(bits&0x07); };

	// Exp-Golomb Codes
	uint32 Get_UE();
	int32 Get_SE();
	int32 Get_ME(int32 mode);
	int32 Get_TE(int32 range);
	int32 Get_Golomb(int k);

	inline int32 Size_UE(uint32 val)
	{
		if (val<255) return EXP_GOLOMB_SIZE[val];
		int32 isize=0;
		val++;
		if (val >= 0x10000) { isize+= 32;	val = (val >> 16)-1; }
		if (val >= 0x100)	{ isize+= 16;	val = (val >> 8)-1;  }
		return EXP_GOLOMB_SIZE[val] + isize;
	}

	inline int32 Size_SE(int32 val)				{ return Size_UE(val <= 0 ? -val*2 : val*2 - 1); }
	inline int32 Size_TE(int32 range, int32 v)  { if (range == 1) return 1; return Size_UE(v);	}

	inline void NeedBits() { if (bits < 16) { bitbuf |= ((buf[0] << 8) | (buf[1])) << (16-bits); bits += 16; buf += 2; } };
	inline void NeedBits24() { while (bits<24) { bitbuf |= (buf[0] << (24-bits)); buf++; bits+= 8; } };
	inline void NeedBits32() { while (bits<32) { bitbuf |= (buf[0] << (24-bits)); buf++; bits+= 8; } };

	// Musepack SV7 32-bit words
	inline void NeedBits32_MPC()
	{
		NeedBits32();
		uint8	tmp[4];
		tmp[0] = (bitbuf>> 0) & 0xff;
		tmp[1] = (bitbuf>> 8) & 0xff;
		tmp[2] = (bitbuf>>16) & 0xff;
		tmp[3] = (bitbuf>>24) & 0xff;
		bitbuf = (tmp[0] << 24) | (tmp[1] << 16) | (tmp[2] << 8) | (tmp[3]);
	}

	inline void PutBits(int32 val, int32 num) {
		bits += num;
		if (num < 32) val &= (1<<num)-1;
		if (bits > 32) {
			bits -= 32;
			bitbuf |= val >> (bits);
			*buf++ = ( bitbuf >> 24 ) & 0xff;
			*buf++ = ( bitbuf >> 16 ) & 0xff;
			*buf++ = ( bitbuf >>  8 ) & 0xff;
			*buf++ = ( bitbuf >>  0 ) & 0xff;
			bitbuf = val << (32 - bits);
		} else
		bitbuf |= val << (32 - bits);
	}
	inline void WriteBits() {
		while (bits >= 8) {
			*buf++ = (bitbuf >> 24) & 0xff;
			bitbuf <<= 8;
			bits -= 8;
		}
	}
	inline void Put_ByteAlign_Zero() { int32 bl=(bits)&0x07; if (bl<8) { PutBits(0,8-bl); } WriteBits(); }
	inline void Put_ByteAlign_One() { int32 bl=(bits)&0x07; if (bl<8) {	PutBits(0xffffff,8-bl); } WriteBits(); }
};

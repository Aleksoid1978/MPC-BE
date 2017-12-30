/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2017 see Authors.txt
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

#include <dvdmedia.h>

struct DIRACINFOHEADER {
	VIDEOINFOHEADER2 hdr;
	DWORD cbSequenceHeader;
	DWORD dwSequenceHeader[1];
};

struct WAVEFORMATEXPS2 : public WAVEFORMATEX {
	DWORD dwInterleave;

	struct WAVEFORMATEXPS2() {
		memset(this, 0, sizeof(*this));
		cbSize = sizeof(WAVEFORMATEXPS2) - sizeof(WAVEFORMATEX);
	}
};

typedef struct tagVORBISFORMAT {
	WORD nChannels;
	DWORD nSamplesPerSec;
	DWORD nMinBitsPerSec;
	DWORD nAvgBitsPerSec;
	DWORD nMaxBitsPerSec;
	float fQuality;
} VORBISFORMAT, *PVORBISFORMAT, FAR *LPVORBISFORMAT;

typedef struct tagVORBISFORMAT2 {
	DWORD Channels;
	DWORD SamplesPerSec;
	DWORD BitsPerSample;
	DWORD HeaderSize[3]; // 0: Identification, 1: Comment, 2: Setup
} VORBISFORMAT2, *PVORBISFORMAT2, FAR *LPVORBISFORMAT2;

#pragma pack(push, 1)
typedef struct {
	DWORD dwOffset;
	CHAR IsoLang[4]; // three letter lang code + terminating zero
	WCHAR TrackName[256]; // 256 chars ought to be enough for everyone :)
} SUBTITLEINFO;
#pragma pack(pop)

struct WAVEFORMATEX_HDMV_LPCM : public WAVEFORMATEX {
	BYTE channel_conf;

	struct WAVEFORMATEX_HDMV_LPCM() {
		memset(this, 0, sizeof(*this));
		cbSize = sizeof(WAVEFORMATEX_HDMV_LPCM) - sizeof(WAVEFORMATEX);
	}
};

#pragma pack(push, 1)
struct DVDALPCMFORMAT
{
	WAVEFORMATEX wfe;
	WORD  GroupAssignment;
	DWORD nSamplesPerSec2;
	WORD  wBitsPerSample2;
};
#pragma pack(pop)

struct WAVEFORMATEXFFMPEG
{
	int nCodecId;
	WAVEFORMATEX wfex;

	struct WAVEFORMATEXFFMPEG() {
		memset(this, 0, sizeof(*this));
	}
};

typedef struct tagDOLBYAC3WAVEFORMAT {
	WAVEFORMATEX	wfx;
	BYTE			bBigEndian;		// TRUE = Big Endian, FALSE little endian
	BYTE			bsid;
	BYTE			lfeon;
	BYTE			copyrightb;
	BYTE			nAuxBitsCode;	//  Aux bits per frame
} DOLBYAC3WAVEFORMAT;

struct fraction_t {
	int num;
	int den;
};

struct SyncPoint {
	REFERENCE_TIME rt;
	__int64 fp;
};

struct ColorSpace {
	BYTE MatrixCoefficients;
	BYTE Primaries;
	BYTE Range;
	BYTE TransferCharacteristics;
	BYTE ChromaLocation;
};

// A byte that is not initialized to std::vector when using the resize method.
struct NoInitByte
{
	uint8_t value;
	NoInitByte() {
		// do nothing
		static_assert(sizeof(*this) == sizeof (value), "invalid size");
		//static_assert(__alignof(*this) == __alignof(value), "invalid alignment");
	}
};

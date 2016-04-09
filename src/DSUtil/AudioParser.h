/*
 * (C) 2011-2015 see Authors.txt
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

#include <WinDef.h>
#include <MMReg.h>

#define RIFF_DWORD          0x46464952

#define AC3_SYNC_WORD           0x770b
#define AAC_LATM_SYNC_WORD       0x2b7
#define TRUEHD_SYNC_WORD    0xba6f72f8
#define MLP_SYNC_WORD       0xbb6f72f8
#define IEC61937_SYNC_WORD  0x4e1ff872

#define DTS_SYNC_WORD       0x0180fe7f
#define DTSHD_SYNC_WORD     0x25205864

#define EAC3_FRAME_TYPE_INDEPENDENT  0
#define EAC3_FRAME_TYPE_DEPENDENT    1
#define EAC3_FRAME_TYPE_AC3_CONVERT  2
#define EAC3_FRAME_TYPE_RESERVED     3

bool ParseAACLatmHeader(const BYTE* buf, int len, int& samplerate, int& channels, BYTE* extra, unsigned int& extralen);

DWORD GetDefChannelMask(WORD nChannels);
DWORD GetVorbisChannelMask(WORD nChannels);
DWORD CountBits(DWORD v);

struct audioframe_t {
	int size;
	int samplerate;
	int channels;
	int samples;
	int param1;
	int param2;

	void clear() {
		memset(this, 0, sizeof(*this));
	}
};

int ParseAC3IEC61937Header	(const BYTE* buf);											// need >= 8 bytes
int ParseMPAHeader			(const BYTE* buf, audioframe_t* audioframe = NULL);			// need >= 4 bytes,  param1 = bitrate, param2 = MP3 flag
int ParseMPEG1Header		(const BYTE* buf, MPEG1WAVEFORMAT* mpeg1wf);				// need >= 4 bytes
int ParseMP3Header			(const BYTE* buf, MPEGLAYER3WAVEFORMAT* mp3wf);				// need >= 4 bytes (experimental)
int ParseAC3Header			(const BYTE* buf, audioframe_t* audioframe = NULL);			// need >= 7 bytes,  param1 = bitrate
int ParseEAC3Header			(const BYTE* buf, audioframe_t* audioframe = NULL);			// need >= 6 bytes,  param1 = eac3 frame type
int ParseMLPHeader			(const BYTE* buf, audioframe_t* audioframe = NULL);			// need >= 12 bytes, param1 = bitdepth, param2 = TrueHD flag
int ParseDTSHeader			(const BYTE* buf, audioframe_t* audioframe = NULL);			// need >= 10 bytes, param1 = transmission bitrate, param2 = x96k extension flag
int ParseDTSHDHeader		(const BYTE* buf, const int buffsize = 0, audioframe_t* audioframe = NULL); // need >= 20 bytes, param1 = bitdepth
int ParseHdmvLPCMHeader		(const BYTE* buf, audioframe_t* audioframe = NULL);			// need >= 4 bytes,  param1 = bitdepth, param2 = bytes per frame
int ParseADTSAACHeader		(const BYTE* buf, audioframe_t* audioframe = NULL);			// need >= 7 bytes,  param1 = header size, param2 = MPEG-4 Audio Object Type

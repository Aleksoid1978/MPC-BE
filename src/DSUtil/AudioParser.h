/*
 * (C) 2011-2016 see Authors.txt
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

#define RIFF_SYNCWORD            0x46464952

#define AC3_SYNCWORD                 0x770B
#define AAC_ADTS_SYNCWORD            0xF0FF
#define AAC_LATM_SYNCWORD             0x2B7

#define MPA_SYNCWORD                 0xE0FF

#define TRUEHD_SYNCWORD          0xBA6F72F8
#define MLP_SYNCWORD             0xBB6F72F8

#define IEC61937_SYNCWORD        0x4E1FF872

#define DTS_SYNCWORD_CORE_BE     0x0180FE7F
#define DTS_SYNCWORD_CORE_LE     0x80017FFE
#define DTS_SYNCWORD_CORE_14B_BE 0x00E8FF1F
#define DTS_SYNCWORD_CORE_14B_LE 0xE8001FFF
#define DTS_SYNCWORD_SUBSTREAM   0x25205864

#define EAC3_FRAME_TYPE_INDEPENDENT 0
#define EAC3_FRAME_TYPE_DEPENDENT   1
#define EAC3_FRAME_TYPE_AC3_CONVERT 2
#define EAC3_FRAME_TYPE_RESERVED    3

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

	void Empty() {
		memset(this, 0, sizeof(*this));
	}
};

int CalcBitrate(const audioframe_t& audioframe);

#define DCA_PROFILE_HD_HRA  0x01 // High Resolution Audio
#define DCA_PROFILE_HD_MA   0x02 // Master Audio
#define DCA_PROFILE_EXPRESS 0x03 // Express

// need >= 8 bytes
int ParseAC3IEC61937Header (const BYTE* buf);

// need >= 4 bytes, param1 = bitrate, param2 = MP3 flag
int ParseMPAHeader         (const BYTE* buf, audioframe_t* audioframe = NULL);

// need >= 4 bytes
int ParseMPEG1Header       (const BYTE* buf, MPEG1WAVEFORMAT* mpeg1wf);

// need >= 4 bytes (experimental)
int ParseMP3Header         (const BYTE* buf, MPEGLAYER3WAVEFORMAT* mp3wf);

// need >= 7 bytes, param1 = bitrate
int ParseAC3Header         (const BYTE* buf, audioframe_t* audioframe = NULL);

// need >= 6 bytes, param1 = eac3 frame type
int ParseEAC3Header        (const BYTE* buf, audioframe_t* audioframe = NULL);

// need >= 12 bytes, param1 = bitdepth, param2 = TrueHD flag
int ParseMLPHeader         (const BYTE* buf, audioframe_t* audioframe = NULL);

// need >= 10 bytes, param2 = x96k extension flag
int ParseDTSHeader         (const BYTE* buf, audioframe_t* audioframe = NULL);

// need >= 40 bytes, param1 = bitdepth, param2 = profile
int ParseDTSHDHeader       (const BYTE* buf, const int buffsize = 0, audioframe_t* audioframe = NULL);

// need >= 4 bytes, param1 = bitdepth, param2 = bytes per frame
int ParseHdmvLPCMHeader    (const BYTE* buf, audioframe_t* audioframe = NULL);

// need >= 7 bytes, param1 = header size, param2 = MPEG-4 Audio Object Type
int ParseADTSAACHeader     (const BYTE* buf, audioframe_t* audioframe = NULL);

bool ParseAACLatmHeader(const BYTE* buf, int len, int& samplerate, int& channels, BYTE* extra, unsigned int& extralen);

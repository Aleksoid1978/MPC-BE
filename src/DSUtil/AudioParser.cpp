/*
 * (C) 2011-2026 see Authors.txt
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
#include "AudioParser.h"
#include "GolombBuffer.h"
#include <mpc_defines.h>
#include "Utils.h"
#include "MP4AudioDecoderConfig.h"
#include <basestruct.h>

#include <vector>

#define AC3_CHANNEL                  0
#define AC3_MONO                     1
#define AC3_STEREO                   2
#define AC3_3F                       3
#define AC3_2F1R                     4
#define AC3_3F1R                     5
#define AC3_2F2R                     6
#define AC3_3F2R                     7
#define AC3_CHANNEL1                 8
#define AC3_CHANNEL2                 9
#define AC3_DOLBY                   10
#define AC3_CHANNEL_MASK            15
#define AC3_LFE                     16

// Channel masks

DWORD GetDefChannelMask(WORD nChannels)
{
	switch (nChannels) {
		case 1: // 1.0 Mono (KSAUDIO_SPEAKER_MONO)
			return SPEAKER_FRONT_CENTER;
		case 2: // 2.0 Stereo (KSAUDIO_SPEAKER_STEREO)
			return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
		case 3: // 2.1 Stereo
			return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY;
		case 4: // 4.0 Quad (KSAUDIO_SPEAKER_QUAD)
			return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT
				   | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
		case 5: // 5.0
			return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER
				   | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
		case 6: // 5.1 Side (KSAUDIO_SPEAKER_5POINT1_SURROUND)
			return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER
				   | SPEAKER_LOW_FREQUENCY | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT;
		case 7: // 6.1 Side
			return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER
				   | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_CENTER
				   | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT;
		case 8: // 7.1 Surround (KSAUDIO_SPEAKER_7POINT1_SURROUND)
			return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER
				   | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT
				   | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT;
		case 10: // 9.1 Surround
			return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER
				   | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT
				   | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT
				   | SPEAKER_TOP_FRONT_LEFT | SPEAKER_TOP_FRONT_RIGHT;
		case 12: // 11.1
			return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER
				   | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT
				   | SPEAKER_FRONT_LEFT_OF_CENTER | SPEAKER_FRONT_RIGHT_OF_CENTER
				   | SPEAKER_TOP_FRONT_LEFT | SPEAKER_TOP_FRONT_RIGHT
				   | SPEAKER_TOP_BACK_LEFT | SPEAKER_TOP_BACK_RIGHT;
		default:
			return 0;
	}
}

DWORD GetVorbisChannelMask(WORD nChannels)
{
	// for Vorbis and FLAC
	// http://xiph.org/vorbis/doc/Vorbis_I_spec.html#x1-800004.3.9
	// http://flac.sourceforge.net/format.html#frame_header
	switch (nChannels) {
		case 1: // 1.0 Mono (KSAUDIO_SPEAKER_MONO)
			return SPEAKER_FRONT_CENTER;
		case 2: // 2.0 Stereo (KSAUDIO_SPEAKER_STEREO)
			return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
		case 3: // 3.0
			return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT;
		case 4: // 4.0 Quad (KSAUDIO_SPEAKER_QUAD)
			return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT
				   | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
		case 5: // 5.0
			return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT
				   | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
		case 6: // 5.1 (KSAUDIO_SPEAKER_5POINT1)
			return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT
				   | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT
				   | SPEAKER_LOW_FREQUENCY;
		case 7: // 6.1 Side
			return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT
				   | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT
				   | SPEAKER_BACK_CENTER
				   | SPEAKER_LOW_FREQUENCY;
		case 8: // 7.1 Surround (KSAUDIO_SPEAKER_7POINT1_SURROUND)
			return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT
				   | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT
				   | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT
				   | SPEAKER_LOW_FREQUENCY;
		default:
			return 0;
	}
}

int CalcBitrate(const audioframe_t& audioframe)
{
	return audioframe.samples ? (int)(8i64 * audioframe.size * audioframe.samplerate / audioframe.samples) : 0;
}

// S/PDIF AC3

int ParseAC3IEC61937Header(const BYTE* buf)
{
	WORD* wbuf = (WORD*)buf;
	if (GETU32(buf) != IEC61937_SYNCWORD
			|| wbuf[2] !=  0x0001
			|| wbuf[3] == 0
			|| wbuf[3] >= (6144-8)*8
			|| wbuf[4] != 0x0B77 ) {
		return 0;
	}

	return 6144;
}

// MPEG Audio

// http://mpgedit.org/mpgedit/mpeg_format/mpeghdr.htm
// http://www.codeproject.com/Articles/8295/MPEG-Audio-Frame-Header

static const short mpeg1_rates[3][16] = {
	{ 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0 }, // MPEG 1 Layer 1
	{ 0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384, 0 }, // MPEG 1 Layer 2
	{ 0, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 0 }, // MPEG 1 Layer 3
};
static const short mpeg2_rates[2][16] = {
	{ 0, 32, 48, 56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256, 0 }, // MPEG 2/2.5 Layer 1
	{ 0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, 0 }, // MPEG 2/2.5 Layer 2/3
};
static const int mpeg1_samplerates[] = { 44100, 48000, 32000, 0 };

#define MPEG_V1			0x3
#define MPEG_V2			0x2
#define MPEG_INVALID	0x1
#define MPEG_V25		0x0

#define LAYER1			0x3
#define LAYER2			0x2
#define LAYER3			0x1
#define LAYER_INVALID	0x0

int ParseMPAHeader(const BYTE* buf, audioframe_t* audioframe)
{
	if ((GETU16(buf) & MPA_SYNCWORD) != MPA_SYNCWORD) { // sync
		return 0;
	}

	BYTE mpaver_id        = (buf[1] & 0x18) >> 3;
	BYTE layer_desc       = (buf[1] & 0x06) >> 1;
	BYTE bitrate_index    = buf[2] >> 4;
	BYTE samplerate_index = (buf[2] & 0x0c) >> 2;
	if (mpaver_id == MPEG_INVALID || layer_desc == LAYER_INVALID || bitrate_index == 0 || bitrate_index == 15 || samplerate_index == 3) {
		return 0;
	}
	BYTE pading           = (buf[2] & 0x02) >> 1;

	int bitrate;
	if (mpaver_id == MPEG_V1) { // MPEG Version 1
		bitrate = mpeg1_rates[3 - layer_desc][bitrate_index];
	} else { // MPEG Version 2, MPEG Version 2.5
		if (layer_desc == LAYER1) { // Layer 1
			bitrate = mpeg2_rates[0][bitrate_index];
		} else { // Layer 2, Layer 3
			bitrate = mpeg2_rates[1][bitrate_index];
		}
	}
	bitrate *= 1000;

	int samplerate = mpeg1_samplerates[samplerate_index];
	if (mpaver_id == MPEG_V2) { // MPEG Version 2
		samplerate /= 2;
	} else if (mpaver_id == MPEG_V25) { // MPEG Version 2.5
		samplerate /= 4;
	}

	int frame_size;
	if (layer_desc == LAYER1) { // Layer 1
		frame_size = (12 * bitrate / samplerate + pading) * 4;
	}
	else if (mpaver_id != MPEG_V1 && layer_desc == LAYER3) { // MPEG Version 2/2.5 Layer 3
		frame_size = 72 * bitrate / samplerate + pading;
	}
	else {
		frame_size = 144 * bitrate / samplerate + pading;
	}

	if (audioframe) {
		audioframe->size       = frame_size;
		audioframe->samplerate = samplerate;

		if ((buf[3] & 0xc0) == 0xc0) {
			audioframe->channels = 1;
		} else {
			audioframe->channels = 2;
		}

		if (layer_desc == LAYER1) { // Layer 1
			audioframe->samples = 384;
		}
		else if (mpaver_id != MPEG_V1 && layer_desc == LAYER3) { // MPEG Version 2/2.5 Layer 3
			audioframe->samples = 576;
		}
		else {
			audioframe->samples = 1152;
		}

		audioframe->param1 = bitrate;
		audioframe->param2 = (mpaver_id == MPEG_V1 && layer_desc == LAYER3) ? 1 : 0;
	}

	return frame_size;
}

int ParseMPEG1Header(const BYTE* buf, MPEG1WAVEFORMAT* mpeg1wf)
{
	// http://msdn.microsoft.com/en-us/library/windows/desktop/dd390701%28v=vs.85%29.aspx

	if ((GETU16(buf) & 0xf8ff) != 0xf8ff) { // sync + (mpaver_id = MPEG Version 1)
		return 0;
	}

	BYTE layer_desc       = (buf[1] & 0x06) >> 1;
	BYTE protection_bit   = buf[1] & 0x01;
	BYTE bitrate_index    = buf[2] >> 4;
	BYTE samplerate_index = (buf[2] & 0x0c) >> 2;
	if (layer_desc == 0x0 || bitrate_index == 0 || bitrate_index == 15 || samplerate_index == 3) {
		return 0;
	}
	BYTE pading_bit       = (buf[2] & 0x02) >> 1;
	BYTE private_bit      = buf[2] & 0x01;
	BYTE channel_mode     = (buf[3] & 0xc0) >> 6;
	BYTE mode_extension   = (buf[3] & 0x30) >> 4;
	BYTE copyright_bit    = (buf[3] & 0x08) >> 3;
	BYTE original_bit     = (buf[3] & 0x04) >> 2;
	BYTE emphasis         = buf[3] & 0x03;

	mpeg1wf->fwHeadLayer   = 8 >> layer_desc;
	mpeg1wf->dwHeadBitrate = (DWORD)mpeg1_rates[3 - layer_desc][bitrate_index] * 1000;
	mpeg1wf->fwHeadMode    = 1 << channel_mode;
	mpeg1wf->fwHeadModeExt = 1 << mode_extension;
	mpeg1wf->wHeadEmphasis = emphasis + 1;
	mpeg1wf->fwHeadFlags   = ACM_MPEG_ID_MPEG1;
	if (private_bit)    { mpeg1wf->fwHeadFlags |= ACM_MPEG_PRIVATEBIT;    }
	if (copyright_bit)  { mpeg1wf->fwHeadFlags |= ACM_MPEG_COPYRIGHT;     }
	if (original_bit)   { mpeg1wf->fwHeadFlags |= ACM_MPEG_ORIGINALHOME;  }
	if (protection_bit) { mpeg1wf->fwHeadFlags |= ACM_MPEG_PROTECTIONBIT; }
	mpeg1wf->dwPTSLow      = 0;
	mpeg1wf->dwPTSHigh     = 0;

	mpeg1wf->wfx.wFormatTag      = WAVE_FORMAT_MPEG;
	mpeg1wf->wfx.nChannels       = channel_mode == 0x3 ? 1 : 2;
	mpeg1wf->wfx.nSamplesPerSec  = mpeg1_samplerates[samplerate_index];
	mpeg1wf->wfx.nAvgBytesPerSec = mpeg1wf->dwHeadBitrate / 8;
	if (layer_desc == LAYER1) { // Layer 1
		mpeg1wf->wfx.nBlockAlign = (12 * mpeg1wf->dwHeadBitrate / mpeg1wf->wfx.nSamplesPerSec) * 4; // without pading_bit
	} else { // Layer 2, Layer 3
		mpeg1wf->wfx.nBlockAlign = 144 * mpeg1wf->dwHeadBitrate / mpeg1wf->wfx.nSamplesPerSec; // without pading_bit
	}
	mpeg1wf->wfx.wBitsPerSample  = 0;
	mpeg1wf->wfx.cbSize          = 22;

	return (mpeg1wf->wfx.nBlockAlign + pading_bit);
}

int ParseMP3Header(const BYTE* buf, MPEGLAYER3WAVEFORMAT* mp3wf) // experimental
{
	// http://msdn.microsoft.com/en-us/library/windows/desktop/dd390710%28v=vs.85%29.aspx

	if ((GETU16(buf) & 0xfeff) != 0xfaff) { // sync + (mpaver_id = MPEG Version 1 = 11b) + (layer_desc = Layer 3 = 01b)
		return 0;
	}

	BYTE bitrate_index    = buf[2] >> 4;
	BYTE samplerate_index = (buf[2] & 0x0c) >> 2;
	if (bitrate_index == 0 || bitrate_index == 15 || samplerate_index == 3) {
		return 0;
	}
	BYTE pading_bit       = (buf[2] & 0x02) >> 1;
	BYTE channel_mode     = (buf[3] & 0xc0) >> 6;

	DWORD bitrate = (DWORD)mpeg1_rates[2][bitrate_index] * 1000;

	mp3wf->wfx.wFormatTag      = WAVE_FORMAT_MPEGLAYER3;
	mp3wf->wfx.nChannels       = channel_mode == 0x3 ? 1 : 2;
	mp3wf->wfx.nSamplesPerSec  = mpeg1_samplerates[samplerate_index];
	mp3wf->wfx.nAvgBytesPerSec = bitrate / 8;
	mp3wf->wfx.nBlockAlign     = 144 * bitrate / mp3wf->wfx.nSamplesPerSec; // without pading_bit ?
	mp3wf->wfx.wBitsPerSample  = 0;
	mp3wf->wfx.cbSize          = MPEGLAYER3_WFX_EXTRA_BYTES;

	mp3wf->wID             = MPEGLAYER3_ID_MPEG;
	mp3wf->fdwFlags        = MPEGLAYER3_FLAG_PADDING_ISO; // clarify on average bitrate.
	mp3wf->nBlockSize      = 144 * bitrate / mp3wf->wfx.nSamplesPerSec + pading_bit;
	mp3wf->nFramesPerBlock = 1152;
	mp3wf->nCodecDelay     = 0;

	return mp3wf->nBlockSize;
}

// AC-3

int ParseAC3Header(const BYTE* buf, audioframe_t* audioframe)
{
	static const short ac3_rates[19]    = { 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 576, 640 };
	static const BYTE  ac3_halfrate[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3 };
	static const BYTE  ac3_lfeon[8]     = { 0x10, 0x10, 0x04, 0x04, 0x04, 0x01, 0x04, 0x01 };

	const auto sync = GETU16(buf);
	if (sync != AC3_SYNCWORD && sync != AC3_SYNCWORD_LE) { // syncword
		return 0;
	}

	BYTE bsid = buf[sync == AC3_SYNCWORD ? 5 : 4] >> 3; // bsid
	if (bsid > 10) {
		return 0;
	}

	int frmsizecod = buf[sync == AC3_SYNCWORD ? 4 : 5] & 0x3F;
	if (frmsizecod >= 38) {
		return 0;
	}

	int frame_size;
	int samplerate;
	int half = ac3_halfrate[bsid];
	int rate = ac3_rates[frmsizecod >> 1];
	switch (buf[sync == AC3_SYNCWORD ? 4 : 5] & 0xc0) {
		case 0:
			samplerate = 48000 >> half;
			frame_size  = 4 * rate;
			break;
		case 0x40:
			samplerate = 44100 >> half;
			frame_size  = 2 * (320 * rate / 147 + (frmsizecod & 1));
			break;
		case 0x80:
			samplerate = 32000 >> half;
			frame_size  = 6 * rate;
			break;
		default:
			return 0;
	}

	if (audioframe) {
		audioframe->size       = frame_size;
		audioframe->samplerate = samplerate;

		const auto buf6 = buf[sync == AC3_SYNCWORD ? 6 : 7];
		BYTE acmod = buf6 >> 5;
		BYTE flags = ((((buf6 & 0xf8) == 0x50) ? AC3_DOLBY : acmod) | ((buf6 & ac3_lfeon[acmod]) ? AC3_LFE : 0));
		switch (flags & AC3_CHANNEL_MASK) {
			case AC3_MONO:
				audioframe->channels = 1;
				break;
			case AC3_CHANNEL:
			case AC3_STEREO:
			case AC3_CHANNEL1:
			case AC3_CHANNEL2:
			case AC3_DOLBY:
				audioframe->channels = 2;
				break;
			case AC3_3F:
			case AC3_2F1R:
				audioframe->channels = 3;
				break;
			case AC3_3F1R:
			case AC3_2F2R:
				audioframe->channels = 4;
				break;
			case AC3_3F2R:
				audioframe->channels = 5;
				break;
		}
		if (flags & AC3_LFE) {
			(audioframe->channels)++;
		}

		audioframe->samples = 1536;
		audioframe->param1  = (rate * 1000) >> half; // bitrate
		audioframe->param2  = 0;
	}

	return frame_size;
}

// E-AC3

int ParseEAC3Header(const BYTE* buf, audioframe_t* audioframe, const int buffsize)
{
	static const int   eac3_samplerates[6] = { 48000, 44100, 32000, 24000, 22050, 16000 };
	static const BYTE  eac3_channels[8]    = { 2, 1, 2, 3, 3, 4, 4, 5 };
	static const short eac3_samples_tbl[4] = { 256, 512, 768, 1536 };

	if (GETU16(buf) != AC3_SYNCWORD) { // syncword
		return 0;
	}

	const int bsid = buf[5] >> 3; // bsid
	if (bsid < 11 || bsid > 16) {
		return 0;
	}

	const int frame_size = (((buf[2] & 0x07) << 8) + buf[3] + 1) * 2;

	const int fscod      =  buf[4] >> 6;
	const int fscod2     = (buf[4] >> 4) & 0x03;
	const int frame_type = (buf[2] >> 6) & 0x03;

	if ((fscod == 0x03 && fscod2 == 0x03) || frame_type == EAC3_FRAME_TYPE_RESERVED) {
		return 0;
	}

	if (audioframe) {
		const int sub_stream_id = (buf[2] >> 3) & 0x07;
		const int acmod         = (buf[4] >> 1) & 0x07;
		const int lfeon         =  buf[4] & 0x01;

		audioframe->size       = frame_size;
		audioframe->samplerate = eac3_samplerates[fscod == 0x03 ? 3 + fscod2 : fscod];
		audioframe->channels   = eac3_channels[acmod] + lfeon;
		audioframe->samples    = (fscod == 0x03) ? 1536 : eac3_samples_tbl[fscod2];
		audioframe->param1     = frame_type;
		audioframe->param2     = 0;

		if (buffsize >= 16 && !sub_stream_id) {
			int num_blocks = 6;
			if (fscod != 0x03) {
				static int eac3_blocks[] = { 1, 2, 3, 6 };
				num_blocks = eac3_blocks[fscod2];
			}

			// skip AC3 header - 5 bytes
			bool atmos_flag = {};
			ParseEAC3HeaderForAtmosDetect(buf + 5, buffsize - 5,
										  frame_type, fscod, num_blocks, acmod, lfeon,
										  atmos_flag);
			if (atmos_flag) {
				audioframe->param2 = 1;
			}
		}
	}

	return frame_size;
}

void ParseEAC3HeaderForAtmosDetect(const BYTE* buf, const int buffsize,
								   int frame_type, int fscod, int num_blocks, int acmod, int lfeon,
								   bool& atmos_flag)
{
	atmos_flag = false;

	CGolombBuffer gb(buf, buffsize);

	gb.BitRead(5); // bitstream id
	for (int i = 0; i < (acmod ? 1 : 2); i++) {
		gb.BitRead(5); // dialog normalization

		if (gb.BitRead(1)) {
			gb.BitRead(8); // heavy dynamic range
		}
	}

	if (frame_type == EAC3_FRAME_TYPE_DEPENDENT) {
		if (gb.BitRead(1)) {
			gb.BitRead(16); // channel map
		}
	}

	/* mixing metadata */
	if (gb.BitRead(1)) {
		/* center and surround mix levels */
		if (acmod > AC3_STEREO) {
			gb.BitRead(2); // preferred downmix
			if (acmod & 1) {
				gb.BitRead(3); // center mix level ltrt
				gb.BitRead(3); // center mix level
			}
			if (acmod & 4) {
				gb.BitRead(3); // surround mix level ltrt
				gb.BitRead(3); // surround mix level
			}
		}

		/* lfe mix level */
		if (lfeon && gb.BitRead(1)) {
			gb.BitRead(5); // lfe mix level
		}

		/* info for mixing with other streams and substreams */
		if (frame_type == EAC3_FRAME_TYPE_INDEPENDENT) {
			for (int i = 0; i < (acmod ? 1 : 2); i++) {
				if (gb.BitRead(1)) {
					gb.BitRead(6); // program scale factor
				}
			}
			if (gb.BitRead(1)) {
				gb.BitRead(6); // external program scale factor
			}

			/* mixing parameter data */
			switch (gb.BitRead(2)) {
				case 1: gb.BitRead(5);  break;
				case 2: gb.BitRead(12); break;
				case 3: {
					int mix_data_size = (gb.BitRead(5) + 2) << 3;
					gb.BitRead(mix_data_size);
					break;
				}
			}

			/* pan information for mono or dual mono source */
			if (acmod < AC3_STEREO) {
				for (int i = 0; i < (acmod ? 1 : 2); i++) {
					if (gb.BitRead(1)) {
						gb.BitRead(8);  // pan mean direction index
						gb.BitRead(6);  // reserved paninfo bits
					}
				}
			}

			/* mixing configuration information */
			if (gb.BitRead(1)) {
				for (int blk = 0; blk < num_blocks; blk++) {
					if (num_blocks == 1 || gb.BitRead(1)) {
						gb.BitRead(5);
					}
				}
			}
		}
	}

	/* informational metadata */
	if (gb.BitRead(1)) {
		gb.BitRead(3); // bitstream mode
		gb.BitRead(2); // copyright bit and original bitstream bit
		if (acmod == AC3_STEREO) {
			gb.BitRead(2); // dolby surround mode
			gb.BitRead(2); // dolby headphone mode
		}
		if (acmod >= AC3_2F2R) {
			gb.BitRead(2); // dolby surround ex mode
		}
		for (int i = 0; i < (acmod ? 1 : 2); i++) {
			if (gb.BitRead(1)) {
				gb.BitRead(8); // mix level, room type, and A/D converter type
			}
		}
		if (fscod != 3) {
			gb.BitRead(1); // source sample rate code
		}
	}

	/* converter synchronization flag */
	if (frame_type == EAC3_FRAME_TYPE_INDEPENDENT && num_blocks != 6) {
		gb.BitRead(1); // converter synchronization flag
	}

	/* original frame size code if this stream was converted from AC-3 */
	if (frame_type == EAC3_FRAME_TYPE_AC3_CONVERT &&
			(num_blocks == 6 || gb.BitRead(1))) {
		gb.BitRead(6); // frame size code
	}

	/* additional bitstream info */
	if (gb.BitRead(1)) {
		gb.BitRead(6); // addbsil

		/* In this 8 bit chunk, the LSB is equal to flag_ec3_extension_type_a
			which can be used to detect Atmos presence */
		gb.BitRead(7);
		if (gb.BitRead(1)) {
			atmos_flag = true;
		}
	}
}

// MLP and TrueHD

int ParseMLPHeader(const BYTE* buf, audioframe_t* audioframe)
{
	static const int  mlp_samplerates[16] = { 48000, 96000, 192000, 0, 0, 0, 0, 0, 44100, 88200, 176400, 0, 0, 0, 0, 0 };
	static const BYTE mlp_bitdepth[16]    = { 16, 20, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	static const BYTE mlp_channels[32] = {
		1, 2, 3, 4, 3, 4, 5, 3, 4, 5, 4, 5, 6, 4, 5, 4,
		5, 6, 5, 5, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	static const BYTE thd_channel_count[13] = {
		//LR    C  LFE  LRs LRvh  LRc LRrs   Cs   Ts LRsd  LRw  Cvh LFE2
		   2,   1,   1,   2,   2,   2,   2,   1,   1,   2,   2,   1,   1
	};

	DWORD sync = GETU32(buf+4);
	bool isTrueHD = false;;
	if (sync == TRUEHD_SYNCWORD) {
		isTrueHD = true;
	} else if (sync != MLP_SYNCWORD) {
		return 0;
	}

	int frame_size  = (((buf[0] << 8) | buf[1]) & 0xfff) * 2;

	if (audioframe) {
		audioframe->size = frame_size;
		audioframe->param3 = 0;
		if (isTrueHD) {
			audioframe->param1     = 24; // bitdepth
			audioframe->samplerate = mlp_samplerates[buf[8] >> 4];
			audioframe->samples    =  40 << ((buf[8] >> 4) & 0x07);

			int chanmap_substream_1 = ((buf[ 9] & 0x0f) << 1) | (buf[10] >> 7);
			int chanmap_substream_2 = ((buf[10] & 0x1f) << 8) |  buf[11];
			int channel_map         = chanmap_substream_2 ? chanmap_substream_2 : chanmap_substream_1;
			audioframe->channels = 0;
			for (int i = 0; i < 13; ++i) {
				audioframe->channels += thd_channel_count[i] * ((channel_map >> i) & 1);
			}
			audioframe->param2 = 1; // TrueHD flag

			BYTE num_substreams = buf[20] >> 4;
			BYTE substream_info = buf[21];
			if (num_substreams == 4 && substream_info >> 7 == 1) {
				audioframe->param3 = 1; // TrueHD Atmos flag
			}
		} else {
			audioframe->param1     = mlp_bitdepth[buf[8] >> 4]; // bitdepth
			audioframe->samplerate = mlp_samplerates[buf[9] >> 4];
			audioframe->samples    = 40 << ((buf[9] >> 4) & 0x07);
			audioframe->channels   = mlp_channels[buf[11] & 0x1f];
			audioframe->param2     = 0; // TrueHD flag
		}
	}

	return frame_size;
}

// DTS

void dts14be_to_dts16be(const BYTE* source, BYTE* destination, int size)
{
	unsigned short* src = (unsigned short*)source;
	unsigned short* dst = (unsigned short*)destination;

	for (int i = 0, n = size / 16; i < n; i++) {
		unsigned short src_0 = (src[0] >> 8) | (src[0] << 8);
		unsigned short src_1 = (src[1] >> 8) | (src[1] << 8);
		unsigned short src_2 = (src[2] >> 8) | (src[2] << 8);
		unsigned short src_3 = (src[3] >> 8) | (src[3] << 8);
		unsigned short src_4 = (src[4] >> 8) | (src[4] << 8);
		unsigned short src_5 = (src[5] >> 8) | (src[5] << 8);
		unsigned short src_6 = (src[6] >> 8) | (src[6] << 8);
		unsigned short src_7 = (src[7] >> 8) | (src[7] << 8);

		dst[0] = (src_0 << 2)  | ((src_1 & 0x3fff) >> 12); // 14 + 2
		dst[1] = (src_1 << 4)  | ((src_2 & 0x3fff) >> 10); // 12 + 4
		dst[2] = (src_2 << 6)  | ((src_3 & 0x3fff) >> 8);  // 10 + 6
		dst[3] = (src_3 << 8)  | ((src_4 & 0x3fff) >> 6);  // 8  + 8
		dst[4] = (src_4 << 10) | ((src_5 & 0x3fff) >> 4);  // 6  + 10
		dst[5] = (src_5 << 12) | ((src_6 & 0x3fff) >> 2);  // 4  + 12
		dst[6] = (src_6 << 14) |  (src_7 & 0x3fff);        // 2  + 14

		dst[0] = (dst[0] >> 8) | (dst[0] << 8);
		dst[1] = (dst[1] >> 8) | (dst[1] << 8);
		dst[2] = (dst[2] >> 8) | (dst[2] << 8);
		dst[3] = (dst[3] >> 8) | (dst[3] << 8);
		dst[4] = (dst[4] >> 8) | (dst[4] << 8);
		dst[5] = (dst[5] >> 8) | (dst[5] << 8);
		dst[6] = (dst[6] >> 8) | (dst[6] << 8);

		src += 8;
		dst += 7;
	}
}

void dts14le_to_dts16be(const BYTE* source, BYTE* destination, int size)
{
	unsigned short* src = (unsigned short*)source;
	unsigned short* dst = (unsigned short*)destination;

	for (int i = 0, n = size / 16; i < n; i++) {
		dst[0] = (src[0] << 2)  | ((src[1] & 0x3fff) >> 12); // 14 + 2
		dst[1] = (src[1] << 4)  | ((src[2] & 0x3fff) >> 10); // 12 + 4
		dst[2] = (src[2] << 6)  | ((src[3] & 0x3fff) >> 8);  // 10 + 6
		dst[3] = (src[3] << 8)  | ((src[4] & 0x3fff) >> 6);  // 8  + 8
		dst[4] = (src[4] << 10) | ((src[5] & 0x3fff) >> 4);  // 6  + 10
		dst[5] = (src[5] << 12) | ((src[6] & 0x3fff) >> 2);  // 4  + 12
		dst[6] = (src[6] << 14) |  (src[7] & 0x3fff);        // 2  + 14

		dst[0] = (dst[0] >> 8) | (dst[0] << 8);
		dst[1] = (dst[1] >> 8) | (dst[1] << 8);
		dst[2] = (dst[2] >> 8) | (dst[2] << 8);
		dst[3] = (dst[3] >> 8) | (dst[3] << 8);
		dst[4] = (dst[4] >> 8) | (dst[4] << 8);
		dst[5] = (dst[5] >> 8) | (dst[5] << 8);
		dst[6] = (dst[6] >> 8) | (dst[6] << 8);

		src += 8;
		dst += 7;
	}
}

int ParseDTSHeader(const BYTE* buf, audioframe_t* audioframe)
{
	// ETSI TS 102 114 V1.3.1 (2011-08)
	static const int  dts_samplerates[16] = { 0, 8000, 16000, 32000, 0, 0, 11025, 22050, 44100, 0, 0, 12000, 24000, 48000, 96000, 192000 };
	static const BYTE dts_channels[16]    = { 1, 2, 2, 2, 2, 3, 3, 4, 4, 5, 6, 6, 6, 7, 8, 8 };
	static const int  dts_transmission_bitrates[32] = {
		  32000,   56000,   64000,   96000,
		 112000,  128000,  192000,  224000,
		 256000,  320000,  384000,  448000,
		 512000,  576000,  640000,  768000,
		 960000, 1024000, 1152000, 1280000,
		1344000, 1408000, 1411200, 1472000,
		1536000, 1920000, 2048000, 3072000,
		3840000, 0, 0, 0 //open, variable, lossless
		// [15]  768000 is actually 754500 for DVD
		// [24] 1536000 is actually 1509000 for ???
		// [24] 1536000 is actually 1509750 for DVD
		// [22] 1411200 is actually 1234800 for 14-bit DTS-CD audio
	};

	int frame_size = 0;
	DWORD sync = GETU32(buf);
	switch (sync) {
		case DTS_SYNCWORD_CORE_BE:    // '7FFE8001' 16 bits and big endian bitstream
			frame_size = ((buf[5] & 3) << 12 | buf[6] << 4 | (buf[7] & 0xf0) >> 4) + 1;
			break;
		case DTS_SYNCWORD_CORE_LE:    // 'FE7F0180' 16 bits and little endian bitstream
			frame_size = ((buf[4] & 3) << 12 | buf[7] << 4 | (buf[6] & 0xf0) >> 4) + 1;
			break;
		case DTS_SYNCWORD_CORE_14B_BE:    // '1FFFE800 07Fx' 14 bits and big endian bitstream
			if (buf[4] == 0x07 && (buf[5] & 0xf0) == 0xf0) {
				frame_size = ((buf[6] & 3) << 12 | buf[7] << 4 | (buf[8] & 0x3C) >> 2) + 1;
				frame_size = frame_size * 16 / 14;
			}
			break;
		case DTS_SYNCWORD_CORE_14B_LE:    // 'FF1F00E8 Fx07' 14 bits and little endian bitstream
			if ((buf[4] & 0xf0) == 0xf0 && buf[5] == 0x07) { //
				frame_size = ((buf[7] & 3) << 12 | buf[6] << 4 | (buf[9] & 0x3C) >> 2) + 1;
				frame_size = frame_size * 16 / 14;
			}
			break;
	}

	if (frame_size < 96) {
		return 0;
	}

	if (audioframe) {
		// minimum buffer size is 16
		BYTE hdr[14]; //minimum header size is 14
		switch (sync) {
			case 0x0180fe7f: // '7FFE8001' 16 bits and big endian bitstream
				memcpy(hdr, buf, 14);
				break;
			case 0x80017ffe: // 'FE7F0180' 16 bits and little endian bitstream
				_swab((char*)buf, (char*)hdr, 14);
				break;
			case 0x00e8ff1f: // '1FFFE800' 14 bits and big endian bitstream
				// not tested, need samples.
				dts14be_to_dts16be(buf, (BYTE*)hdr, 16);
				break;
			case 0xe8001fff: // 'FF1F00E8' 14 bits and little endian bitstream
				dts14le_to_dts16be(buf, (BYTE*)hdr, 16);
				break;
		}
		ASSERT(GETU32(hdr) == 0x0180fe7f);

		audioframe->size = frame_size;

		audioframe->samples = (((hdr[4] & 1) << 6 | (hdr[5] & 0xfc) >> 2) + 1) * 32;
		BYTE channel_index = (hdr[7] & 0x0f) << 2 | (hdr[8] & 0xc0) >> 6;
		if (channel_index >= 16) {
			return 0;
		}
		audioframe->channels   = dts_channels[channel_index];
		audioframe->samplerate = dts_samplerates[(hdr[8] & 0x3c) >> 2];
		//audioframe->param1     = dts_transmission_bitrates[(hdr[8] & 3) << 3 | (hdr[9] & 0xe0) >> 5]; // transmission bitrate
		if ((hdr[10] & 6) >> 1) {
			(audioframe->channels)++; // LFE
		}
		audioframe->param2 = 0;

		if (audioframe->samples < 6 * 32 || audioframe->samplerate == 0) {
			return 0;
		}

		BYTE ext_audio_id = (hdr[10] & 0xE0) >> 5;
		BYTE ext_audio_coding = (hdr[10] & 0x10) >> 4;
		if (ext_audio_id == 2 && ext_audio_coding == 1) {
			audioframe->param2 = 1;
			audioframe->samplerate <<= 1;
		}
	}

	return frame_size;
}

#define SPEAKER_PAIR_ALL_2 0xae66
static inline int count_chs_for_mask(int mask)
{
	return CountBits(mask) + CountBits(mask & SPEAKER_PAIR_ALL_2);
}

// Core/extension mask
enum ExtensionMask {
	EXSS_XBR  = 0x020,
	EXSS_XXCH = 0x040,
	EXSS_X96  = 0x080,
	EXSS_LBR  = 0x100,
	EXSS_XLL  = 0x200,
};

int ParseDTSHDHeader(const BYTE* buf, const int buffsize /* = 0*/, audioframe_t* audioframe /* = nullptr*/)
{
	if (GETU32(buf) != DTS_SYNCWORD_SUBSTREAM) { // syncword
		return 0;
	}

	static const DWORD exss_sample_rates[16] = { 8000, 16000, 32000, 64000, 128000, 22050, 44100, 88200, 176400, 352800, 12000, 24000, 48000, 96000, 192000, 384000 };

	int hd_frame_size = 0;

	const BYTE isBlownUpHeader = (buf[5] >> 5) & 1;
	if (isBlownUpHeader) {
		hd_frame_size = ((buf[6] & 1) << 19 | buf[7] << 11 | buf[8] << 3 | buf[9] >> 5) + 1;
	} else {
		hd_frame_size = ((buf[6] & 31) << 11 | buf[7] << 3 | buf[8] >> 5) + 1;
	}

	if (!buffsize || !audioframe) {
		return hd_frame_size;
	}

	audioframe->Empty();
	audioframe->size = hd_frame_size;

	CGolombBuffer gb((BYTE*)buf + 4, buffsize - 4); // skip DTSHD_SYNC_WORD

	UINT num_audiop = 1;
	UINT num_assets = 1;

	BYTE mix_metadata_enabled = 0;

	BYTE embedded_stereo = 0;
	BYTE embedded_6ch = 0;
	BYTE spkr_mask_enabled = 0;
	UINT spkr_mask = 0;

	UINT nmixoutconfigs = 0;
	UINT nmixoutchs[4] = { 0 };

	BYTE exss_frame_duration_code = 0;
	UINT exss_sample_rates_index = 0;

	gb.BitRead(8);
	UINT ss_index = gb.BitRead(2);
	gb.BitRead(1);
	gb.BitRead(8 + 4 * isBlownUpHeader);
	gb.BitRead(16 + 4 * isBlownUpHeader);

	const BYTE static_fields = gb.BitRead(1);
	if (static_fields) {
		gb.BitRead(2);
		exss_frame_duration_code = gb.BitRead(3) + 1;
		if (gb.BitRead(1)) {
			gb.BitRead(36);
		}
		num_audiop = gb.BitRead(3) + 1;
		num_assets = gb.BitRead(3) + 1;

		int active_ss_mask[8] = { 0 };
		for (UINT i = 0; i < num_audiop; i++) {
			active_ss_mask[i] = gb.BitRead(ss_index + 1);
		}

		for (UINT i = 0; i < num_audiop; i++) {
			for (UINT j = 0; j <= ss_index; j++) {
				if (active_ss_mask[i] & (1 << j)) {
					gb.BitRead(8); // active asset mask
				}
			}
		}

		mix_metadata_enabled = gb.BitRead(1);
		if (mix_metadata_enabled) {
			gb.BitRead(2);
			const UINT bits = (gb.BitRead(2) + 1) << 2;
			nmixoutconfigs = gb.BitRead(2) + 1;
			for (UINT i = 0; i < nmixoutconfigs; i++) {
				nmixoutchs[i] = count_chs_for_mask(gb.BitRead(bits));
			}
		}
	}

	for (UINT a = 0; a < num_assets; a++) {
		gb.BitRead(16 + 4 * isBlownUpHeader);
	}

	for (UINT a = 0; a < num_assets; a++) {
		gb.BitRead(9);
		gb.BitRead(3);
		if (static_fields) {
			if (gb.BitRead(1)) {
				gb.BitRead(4);
			}
			if (gb.BitRead(1)) {
				gb.BitRead(24);
			}
			if (gb.BitRead(1)) {
				const UINT bytes = gb.BitRead(10) + 1;
				for (UINT i = 0; i < bytes; i++) {
					gb.BitRead(8);
				}
			}

			audioframe->param1 = gb.BitRead(5) + 1;
			exss_sample_rates_index = gb.BitRead(4);
			if (exss_sample_rates_index < std::size(exss_sample_rates)) {
				audioframe->samplerate = exss_sample_rates[exss_sample_rates_index];
			}
			audioframe->channels = gb.BitRead(8) + 1;

			BYTE one_to_one_map_ch_to_spkr = gb.BitRead(1);
			if (one_to_one_map_ch_to_spkr) {
				if (audioframe->channels > 2) {
					embedded_stereo = gb.BitRead(1);
				}
				if (audioframe->channels > 6) {
					embedded_6ch = gb.BitRead(1);
				}

				spkr_mask_enabled = gb.BitRead(1);
				int spkr_mask_nbits = 0;
				if (spkr_mask_enabled) {
					spkr_mask_nbits = (gb.BitRead(2) + 1) << 2;
					spkr_mask = gb.BitRead(spkr_mask_nbits);
				}

				int nspeakers[8] = { 0 };
				const UINT spkr_remap_nsets = gb.BitRead(3);
				for (UINT i = 0; i < spkr_remap_nsets; i++) {
					nspeakers[i] = gb.BitRead(spkr_mask_nbits);
				}
				for (UINT i = 0; i < spkr_remap_nsets; i++) {
					const UINT nch_for_remaps = gb.BitRead(5) + 1;
					for (int j = 0; j < nspeakers[i]; j++) {
						int remap_ch_mask = gb.BitRead(nch_for_remaps);
						int ncodes = CountBits(remap_ch_mask);
						for (int k = 0; k < ncodes * 5; k++) {
							gb.BitRead(1);
						}
					}
				}
			} else {
				gb.BitRead(3);
			}
		}

		const BYTE drc_present = gb.BitRead(1);
		if (drc_present) {
			gb.BitRead(8);
		}

		if (gb.BitRead(1)) {
			gb.BitRead(5);
		}

		if (drc_present && embedded_stereo) {
			gb.BitRead(8);
		}

		if (mix_metadata_enabled && gb.BitRead(1)) {
			gb.BitRead(1);
			gb.BitRead(6);

			if (gb.BitRead(2) == 3) {
				gb.BitRead(8);
			} else {
				gb.BitRead(3);
			}

			if (gb.BitRead(1)) {
				for (UINT i = 0; i < nmixoutconfigs; i++) {
					gb.BitRead(6 * nmixoutchs[i]);
				}
			} else {
				gb.BitRead(6 * nmixoutconfigs);
			}

			UINT nchannels_dmix = audioframe->channels;
			if (embedded_6ch) {
				nchannels_dmix += 6;
			}
			if (embedded_stereo) {
				nchannels_dmix += 2;
			}
			for (UINT i = 0; i < nmixoutconfigs; i++) {
				for (UINT j = 0; j < nchannels_dmix; j++) {
					if (!nmixoutchs[i]) {
						return 0;
					}
					const UINT mix_map_mask = gb.BitRead(nmixoutchs[i]);
					int nmixcoefs = CountBits(mix_map_mask);
					gb.BitRead(6 * nmixcoefs);
				}
			}
		}

		if (gb.RemainingSize() < 2) {
			break;
		}

		UINT extension_mask = 0;
		const BYTE coding_mode = gb.BitRead(2);
		switch (coding_mode) {
			case 0:
				extension_mask = gb.BitRead(12);
				break;
			case 1:
				extension_mask = EXSS_XLL;
				break;
			case 2:
				extension_mask = EXSS_LBR;
				break;
		}

		if (extension_mask & (EXSS_XBR | EXSS_XXCH | EXSS_X96)) {
			audioframe->param2 = DCA_PROFILE_HD_HRA;
		} else if (extension_mask & EXSS_XLL) {
			audioframe->param2 = DCA_PROFILE_HD_MA;
		} else if (extension_mask & EXSS_LBR) {
			audioframe->param2 = DCA_PROFILE_EXPRESS;
		}

		if (audioframe->param2 == DCA_PROFILE_HD_HRA || audioframe->param2 == DCA_PROFILE_EXPRESS) {
			UINT factor = 0;
			switch (exss_sample_rates_index) {
				case  0 : //  8000
				case 10 : // 12000
					factor = 128;
					break;
				case  1 : // 16000
				case  5 : // 22050
				case 11 : // 24000
					factor = 256;
					break;
				case  2 : // 32000
				case  6 : // 44100
				case 12 : // 48000
					factor = 512;
					break;
				case  3 : // 64000
				case  7 : // 88200
				case 13 : // 96000
					factor = 1024;
					break;
				case  4 : //128000
				case  8 : //176400
				case 14 : //192000
					factor = 2048;
					break;
				case  9 : //352800
				case 15 : //384000
					factor = 4096;
					break;
			}
			audioframe->samples = exss_frame_duration_code * factor;
		}

		break;
	}

	if (audioframe->samplerate && audioframe->channels) {
		if ((audioframe->param2 == DCA_PROFILE_HD_MA || audioframe->param2 == DCA_PROFILE_HD_HRA) && gb.RemainingSize() > 4) {
			auto start = gb.GetBufferPos();
			auto end = start + gb.RemainingSize();
			while (end - start >= 4) {
				auto sync = GETU32(start++);
				if (sync == 0x50080002 || sync == 0xD10040F1 || sync == 0xD40040F1) {
					audioframe->param2 = audioframe->param2 == DCA_PROFILE_HD_MA ? DCA_PROFILE_HD_MA_X : DCA_PROFILE_HD_HRA_X;
					break;
				} else if (sync == 0xD00040F1) {
					audioframe->param2 = audioframe->param2 == DCA_PROFILE_HD_MA ? DCA_PROFILE_HD_MA_X_IMAX : DCA_PROFILE_HD_HRA_X_IMAX ;
					break;
				}
			}
		}

		return hd_frame_size;
	}

	return 0;
}

void GetDTSHDDescription(BYTE profile, CStringW& description)
{
	switch (profile) {
		case DCA_PROFILE_HD_HRA:
			description = L"DTS-HD HRA";
			break;
		case DCA_PROFILE_HD_MA:
			description = L"DTS-HD MA";
			break;
		case DCA_PROFILE_EXPRESS:
			description = L"DTS Express";
			break;
		case DCA_PROFILE_HD_MA_X:
			description = L"DTS-HD MA + DTS:X";
			break;
		case DCA_PROFILE_HD_MA_X_IMAX:
			description = L"DTS-HD MA + DTS:X IMAX";
			break;
		case DCA_PROFILE_HD_HRA_X:
			description = L"DTS-HD HRA + DTS:X";
			break;
		case DCA_PROFILE_HD_HRA_X_IMAX:
			description = L"DTS-HD HRA + DTS:X IMAX";
			break;
	}
}

// HDMV LPCM

int ParseHdmvLPCMHeader(const BYTE* buf, audioframe_t* audioframe)
{
	static const int  hdmvlpcm_samplerates[6] = { 0, 48000, 0, 0, 96000, 192000 };
	static const BYTE hdmvlpcm_channels[16]   = { 0, 1, 0, 2, 3, 3, 4, 4, 5, 6, 7, 8, 0, 0, 0, 0 };
	static const BYTE hdmvlpcm_bitdepth[4]    = { 0, 16, 20, 24 };

	int frame_size = buf[0] << 8 | buf[1];
	frame_size += 4; // add header size;

	if (audioframe) {
		audioframe->size       = frame_size;
		BYTE samplerate_index  = buf[2] & 0x0f;
		audioframe->samplerate = samplerate_index < 6 ? hdmvlpcm_samplerates[samplerate_index] : 0;
		audioframe->channels   = hdmvlpcm_channels[buf[2] >> 4];
		audioframe->param1     = hdmvlpcm_bitdepth[buf[3] >> 6]; // bitdepth
		if (audioframe->channels == 0 || audioframe->samplerate == 0 || audioframe->param1 == 0) {
			return 0;
		}
		audioframe->param2  = ((audioframe->channels + 1) & ~1) * ((audioframe->param1 + 7) / 8); // bytes per frame
		audioframe->samples = (frame_size - 4) / audioframe->param2;
	}

	return frame_size;
}

// ADTS AAC

int ParseADTSAACHeader(const BYTE* buf, audioframe_t* audioframe)
{
	static const int  mp4a_samplerates[16] = { 96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0, 0, 0 };
	static const BYTE mp4a_channels[8]     = { 0, 1, 2, 3, 4, 5, 6, 8 };

	if ((GETU16(buf) & AAC_ADTS_SYNCWORD) != AAC_ADTS_SYNCWORD) { // syncword
		return 0;
	}

	if ((buf[1] & 0x06) !=  0) { // Layer: always 0
		return 0;
	}

	int headersize = (buf[1] & 0x01) == 1 ? 7 : 9;
	int frame_size = ((buf[3] & 0x03) << 11) | (buf[4] << 3) | ((buf[5] & 0xe0) >> 5);
	if (frame_size < headersize) {
		return 0;
	}

	if (audioframe) {
		audioframe->size       = frame_size;
		audioframe->param1     = headersize; // header size
		audioframe->param2     = ((buf[2] & 0xc0) >> 6) + 1;
		audioframe->samplerate = mp4a_samplerates[(buf[2] & 0x3c) >> 2];
		BYTE channel_index = ((buf[2] & 0x01) << 2) | ((buf[3] & 0xc0) >> 6);
		if (audioframe->samplerate == 0/* || channel_index == 0*/ || channel_index >= 8) {
			// Channel Configurations - 0: Defined in AOT Specific Config, so do not ignore it;
			return 0;
		}
		audioframe->channels   = mp4a_channels[channel_index];
		audioframe->samples    = ((buf[6] & 0x03) + 1) * 1024;
	}

	return frame_size;
}

// LATM AAC
static inline UINT64 LatmGetValue(CGolombBuffer& gb) {
	int length = gb.BitRead(2);
	UINT64 value = 0;

	for (int i = 0; i <= length; i++) {
		value <<= 8;
		value |= gb.BitRead(8);
	}

	return value;
}

static bool StreamMuxConfig(CGolombBuffer& gb, int& samplingFrequency, int& channels, int& nExtraPos)
{
	nExtraPos = 0;

	BYTE audio_mux_version_A = 0;
	BYTE audio_mux_version = gb.BitRead(1);
	if (audio_mux_version == 1) {
		audio_mux_version_A = gb.BitRead(1);
	}

	if (!audio_mux_version_A) {
		if (audio_mux_version == 1) {
			LatmGetValue(gb);	// taraFullness
		}
		gb.BitRead(1);			// all_same_framing
		gb.BitRead(6);			// numSubFrames
		if (gb.BitRead(4)) {	// numProgram
			return false;		// Multiple programs don't support
		}
		if (gb.BitRead(3)) {	// int numLayer
			return false;		// Multiple layers don't support
		}

		if (!audio_mux_version) {
			nExtraPos = gb.GetPos();

			CMP4AudioDecoderConfig MP4AudioDecoderConfig;
			bool bRet = MP4AudioDecoderConfig.Parse(gb);
			if (bRet) {
				samplingFrequency = MP4AudioDecoderConfig.m_SamplingFrequency;
				channels = MP4AudioDecoderConfig.m_ChannelCount;
			}
			return bRet;
		}
	}

	return false;
}

int ParseAACLatmHeader(const BYTE* buf, int len, audioframe_t* audioframe/* = nullptr*/, std::vector<BYTE>* extra/* = nullptr*/)
{
	if ((GETU16(buf) & 0xE0FF) != 0xE056) {
		return 0;
	}

	int samplerate = 0, channels = 0;
	int nExtraPos = 0;

	CGolombBuffer gb(buf, len);
	gb.BitRead(11); // sync
	int muxlength = gb.BitRead(13) + 3;
	BYTE use_same_mux = gb.BitRead(1);
	if (!use_same_mux) {
		bool ret = StreamMuxConfig(gb, samplerate, channels, nExtraPos);
		if (!ret) {
			return ret;
		}
	} else {
		return 0;
	}

	if (samplerate < 8000 || samplerate > 96000 || channels < 1 || (channels > 8 && channels != 24)) {
		return 0;
	}

	if (audioframe) {
		audioframe->size = muxlength;
		audioframe->channels = channels;
		audioframe->samplerate = samplerate;
	}

	if (extra && nExtraPos) {
		const int extralen = gb.GetPos() - nExtraPos;
		gb.Reset();
		gb.SkipBytes(nExtraPos);

		extra->resize(extralen);
		gb.ReadBuffer(extra->data(), extra->size());
	}

	return muxlength;
}


namespace AC4 {
	using Result = int;
	const int SUCCESS = 0;
	const int FAILURE = -1;

	const unsigned int CH_MODE_LENGTH          = 16;   /* AC-4 ch_mode length  */
	const unsigned int AC4_FRAME_RATE_NUM      = 14;   /* AC-4 frame rate number  */
	const unsigned int AC4_SUBSTREAM_GROUP_NUM = 3;    /* AC-4 Maximum substream group if presentation_config < 5 */

	const unsigned char SUPER_SET_CH_MODE[CH_MODE_LENGTH][CH_MODE_LENGTH] = {
		{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,11,12,13,14,15},
		{1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,11,12,13,14,15},
		{2, 2, 2, 3, 4, 5, 6, 7, 8, 9, 10,11,12,13,14,15},
		{3, 3, 3, 3, 4, 5, 6, 7, 8, 9, 10,11,12,13,14,15},
		{4, 4, 4, 4, 4, 6, 6, 8, 8, 10,10,12,12,14,14,15},
		{5, 5, 5, 5, 6, 5, 6, 7, 8, 9, 10,11,12,13,14,15},
		{6, 6, 6, 6, 6, 6, 6, 6, 8, 6, 10,12,12,14,14,15},
		{7, 7, 7, 7, 8, 7, 6, 7, 8, 9, 10,12,12,13,14,15},
		{8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 10,11,12,14,14,15},
		{9, 9, 9, 9, 10,9, 10,9, 9, 9, 10,11,12,13,14,15},
		{10,10,10,10,10,10,10,10,10,10,10,10,12,13,14,15},
		{11,11,11,11,12,11,12,11,12,11,12,11,13,13,14,15},
		{12,12,12,12,12,12,12,12,12,12,12,12,12,13,14,15},
		{13,13,13,13,14,13,14,13,14,13,14,13,14,13,14,15},
		{14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,15},
		{15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15}
	};

	// ch_mode - TS 103 190-2 table 78
	const int CH_MODE_MONO     = 0;
	const int CH_MODE_STEREO   = 1;
	const int CH_MODE_3_0      = 2;
	const int CH_MODE_5_0      = 3;
	const int CH_MODE_5_1      = 4;
	const int CH_MODE_70_34    = 5;
	const int CH_MODE_71_34    = 6;
	const int CH_MODE_70_52    = 7;
	const int CH_MODE_71_52    = 8;
	const int CH_MODE_70_322   = 9;
	const int CH_MODE_71_322   = 10;
	const int CH_MODE_7_0_4    = 11;
	const int CH_MODE_7_1_4    = 12;
	const int CH_MODE_9_0_4    = 13;
	const int CH_MODE_9_1_4    = 14;
	const int CH_MODE_22_2     = 15;
	const int CH_MODE_RESERVED = 16;

	// speaker group index mask, indexed by ch_mode - TS 103 190-2 A.27
	const int AC4_SPEAKER_GROUP_INDEX_MASK_BY_CH_MODE[] = {
		2,        // 0b10 - 1.0
		1,        // 0b01 - 2.0
		3,        // 0b11 - 3.0
		7,        // 0b0000111 - 5.0
		71,       // 0b1000111 - 5.1
		15,       // 0b0001111 - 7.0: 3/4/0
		79,       // 0b1001111 - 7.1: 3/4/0.1
		131079,   // 0b100000000000000111 - 7.0: 5/2/0
		131143,   // 0b100000000001000111 - 7.1: 5/2/0.1
		262151,   // 0b1000000000000000111 - 7.0: 3/2/2
		262215,   // 0b1000000000001000111 - 7.1: 3/2/2.1
		63,       // 0b0111111 - 7.0.4
		127,      // 0b1111111 - 7.1.4
		65599,    // 0b10000000000111111 - 9.0.4
		65663,    // 0b10000000001111111 - 9.1.4
		196479,   // 0b101111111101111111 - 22.2
		0         // reserved
	};

	static uint32_t Ac4VariableBits(CGolombBuffer& data, int nBits)
	{
		uint32_t value = 0;
		uint32_t b_moreBits;
		do {
			value += data.BitRead(nBits);
			b_moreBits = data.BitRead(1);
			if (b_moreBits == 1) {
				value <<= nBits;
				value += (1 << nBits);
			}
		} while (b_moreBits == 1);
		return value;
	}

	static Result Ac4ChannelCountFromSpeakerGroupIndexMask(unsigned int speakerGroupIndexMask)
	{

		unsigned int channelCount = 0;
		if ((speakerGroupIndexMask & 1) != 0) { // 0: L,R 0b1
			channelCount += 2;
		}
		if ((speakerGroupIndexMask & 2) != 0) { // 1: C 0b10
			channelCount += 1;
		}
		if ((speakerGroupIndexMask & 4) != 0) { // 2: Ls,Rs 0b100
			channelCount += 2;
		}
		if ((speakerGroupIndexMask & 8) != 0) { // 3: Lb,Rb 0b1000
			channelCount += 2;
		}
		if ((speakerGroupIndexMask & 16) != 0) { // 4: Tfl,Tfr 0b10000
			channelCount += 2;
		}
		if ((speakerGroupIndexMask & 32) != 0) { // 5: Tbl,Tbr 0b100000
			channelCount += 2;
		}
		if ((speakerGroupIndexMask & 64) != 0) { // 6: LFE 0b1000000
			channelCount += 1;
		}
		if ((speakerGroupIndexMask & 128) != 0) { // 7: TL,TR 0b10000000
			channelCount += 2;
		}
		if ((speakerGroupIndexMask & 256) != 0) { // 8: Tsl,Tsr 0b100000000
			channelCount += 2;
		}
		if ((speakerGroupIndexMask & 512) != 0) { // 9: Tfc
			channelCount += 1;
		}
		if ((speakerGroupIndexMask & 1024) != 0) { // 10: Tbc
			channelCount += 1;
		}
		if ((speakerGroupIndexMask & 2048) != 0) { // 11: Tc
			channelCount += 1;
		}
		if ((speakerGroupIndexMask & 4096) != 0) { // 12: LFE2
			channelCount += 1;
		}
		if ((speakerGroupIndexMask & 8192) != 0) { // 13: Bfl,Bfr
			channelCount += 2;
		}
		if ((speakerGroupIndexMask & 16384) != 0) { // 14: Bfc
			channelCount += 1;
		}
		if ((speakerGroupIndexMask & 32768) != 0) { // 15: Cb
			channelCount += 1;
		}
		if ((speakerGroupIndexMask & 65536) != 0) { // 16: Lscr,Rscr
			channelCount += 2;
		}
		if ((speakerGroupIndexMask & 131072) != 0) { // 17: Lw,Rw
			channelCount += 2;
		}
		if ((speakerGroupIndexMask & 262144) != 0) { // 18: Vhl,Vhr
			channelCount += 2;
		}
		return channelCount;
	}

	static uint32_t SetMaxGroupIndex(unsigned int groupIndex, unsigned int maxGroupIndex)
	{
		if (groupIndex > maxGroupIndex) { maxGroupIndex = groupIndex; }
		return maxGroupIndex;
	}

	static Result Ac4SuperSet(int lvalue, int rvalue)
	{
		if ((lvalue == -1) || (lvalue > 15)) return rvalue;
		if ((rvalue == -1) || (rvalue > 15)) return lvalue;
		return SUPER_SET_CH_MODE[lvalue][rvalue];
	}

	struct Ac4EmdfInfo {
		uint8_t emdf_version;
		uint16_t key_id;
		uint8_t b_emdf_payloads_substream_info;
		uint16_t substream_index;
		uint8_t protectionLengthPrimary;
		uint8_t protectionLengthSecondary;
		uint8_t protection_bits_primary[16];
		uint8_t protection_bits_Secondary[16];
	};

	struct Ac4Dsi {
		struct SubStream {
			uint8_t b_4_back_channels_present;
			uint8_t b_centre_present;
			uint8_t top_channels_present;
			uint8_t b_lfe;
			uint8_t dsi_sf_multiplier;
			uint8_t b_substream_bitrate_indicator;
			uint8_t substream_bitrate_indicator;
			uint8_t ch_mode; // auxiliary information, used to calcuate pres_ch_mode
			uint32_t dsi_substream_channel_mask;
			uint8_t b_ajoc;
			uint8_t b_static_dmx;
			uint8_t n_dmx_objects_minus1;
			uint8_t n_umx_objects_minus1;
			uint8_t b_substream_contains_bed_objects;
			uint8_t b_substream_contains_dynamic_objects;
			uint8_t b_substream_contains_ISF_objects;

			// methods
			Result ParseSubstreamInfoChan(CGolombBuffer& bits,
				unsigned int  presentation_version,
				unsigned char defalut_presentation_flag,
				unsigned int  fs_idx,
				unsigned int& speaker_index_mask,
				unsigned int  frame_rate_factor,
				unsigned int  b_substreams_present,
				unsigned char& dolby_atmos_indicator);

			Result ParseSubStreamInfoAjoc(CGolombBuffer& bits,
				unsigned int& channel_count,
				unsigned char defalut_presentation_flag,
				unsigned int  fs_idx,
				unsigned int  frame_rate_factor,
				unsigned int  b_substreams_present);

			Result ParseSubstreamInfoObj(CGolombBuffer& bits,
				unsigned int& channel_count,
				unsigned char defalut_presentation_flag,
				unsigned int  fs_idx,
				unsigned int  frame_rate_factor,
				unsigned int  b_substreams_present);
			Result GetChModeCore(unsigned char b_channel_coded);
		private:
			Result ParseChMode(CGolombBuffer& bits, int presentationVersion, unsigned char& dolby_atmos_indicator);
			Result ParseDsiSfMutiplier(CGolombBuffer& bits, unsigned int fs_idx);
			Result BedDynObjAssignment(CGolombBuffer& bits, unsigned int nSignals, bool is_upmix);
			Result ParseSubstreamIdxInfo(CGolombBuffer& bits, unsigned int b_substreams_present);

			Result ParseBitrateIndicator(CGolombBuffer& bits);
			Result ParseOamdCommonData(CGolombBuffer& bits);
			Result Trim(CGolombBuffer& bits);
			Result BedRendeInfo(CGolombBuffer& bits);
			uint32_t ObjNumFromIsfConfig(unsigned char isf_config);
			uint32_t BedNumFromAssignCode(unsigned char assign_code);
			uint32_t BedNumFromNonStdMask(unsigned int  non_std_mask);
			uint32_t BedNumFromStdMask(unsigned int  std_mask);
		};
		struct SubStreamGroupV1 {
			struct v1 {
				uint8_t b_substreams_present;
				uint8_t b_hsf_ext;
				uint8_t b_channel_coded;
				uint8_t n_substreams;
				std::vector<SubStream> substreams;
				uint8_t b_content_type;
				uint8_t content_classifier;
				uint8_t b_language_indicator;
				uint8_t n_language_tag_bytes;
				uint8_t language_tag_bytes[64]; // n_language_tag_bytes is 6 bits
				uint8_t dolby_atmos_indicator;
			}v1;
			Result ParseSubstreamGroupInfo(CGolombBuffer& bits,
				unsigned int  bitstream_version,
				unsigned int  presentation_version,
				unsigned char defalut_presentation_flag,
				unsigned int  frame_rate_factor,
				unsigned int  fs_idx,
				unsigned int& channel_count,
				unsigned int& speaker_index_mask,
				unsigned int& b_obj_or_Ajoc);
		private:
			Result ParseOamdSubstreamInfo(CGolombBuffer& bits);
			Result ParseHsfExtSubstreamInfo(CGolombBuffer& bits);
			Result ParseContentType(CGolombBuffer& bits);
		};
		struct PresentationV1 {
			uint8_t presentation_version;
			struct v1 {
				uint8_t presentation_config_v1;
				uint8_t mdcompat;
				uint8_t b_presentation_id;
				uint8_t presentation_id;
				uint8_t dsi_frame_rate_multiply_info;
				uint8_t dsi_frame_rate_fraction_info;
				uint8_t presentation_emdf_version;
				uint16_t presentation_key_id;
				uint8_t b_presentation_channel_coded;
				uint8_t dsi_presentation_ch_mode;
				uint8_t pres_b_4_back_channels_present;
				uint8_t pres_top_channel_pairs;
				uint32_t presentation_channel_mask_v1;
				uint8_t b_presentation_core_differs;
				uint8_t b_presentation_core_channel_coded;
				uint8_t dsi_presentation_channel_mode_core;
				uint8_t b_presentation_filter;
				uint8_t b_enable_presentation;
				uint8_t n_filter_bytes;

				uint8_t b_multi_pid;
				uint8_t n_substream_groups;
				std::vector<SubStreamGroupV1> substream_groups;
				std::vector<uint32_t> substream_group_indexs; // auxiliary information, not exist in DSI
				uint8_t n_skip_bytes;

				uint8_t b_pre_virtualized;
				uint8_t b_add_emdf_substreams;
				uint8_t n_add_emdf_substreams;
				uint8_t substream_emdf_version[128]; // n_add_emdf_substreams - 7 bits
				uint16_t substream_key_id[128];

				uint8_t b_presentation_bitrate_info;
				uint8_t b_alternative;
				uint8_t de_indicator;
				uint8_t dolby_atmos_indicator;
				uint8_t b_extended_presentation_id;
				uint16_t extended_presentation_id;
			} v1;
			Result ParsePresentationV1Info(CGolombBuffer& bits,
				unsigned int  bitstream_version,
				unsigned int  frame_rate_idx,
				unsigned int  pres_idx,
				unsigned int& max_group_index,
				unsigned int** first_pres_sg_index,
				unsigned int& first_pres_sg_num);
		private:
			Result ParsePresentationVersion(CGolombBuffer& bits, unsigned int bitstream_version);
			Result ParsePresentationConfigExtInfo(CGolombBuffer& bits, unsigned int bitstream_version);
			uint32_t   ParseAc4SgiSpecifier(CGolombBuffer& bits, unsigned int bitstream_version);
			Result ParseDSIFrameRateMultiplyInfo(CGolombBuffer& bits, unsigned int frame_rate_idx);
			Result ParseDSIFrameRateFractionsInfo(CGolombBuffer& bits, unsigned int frame_rate_idx);
			Result ParseEmdInfo(CGolombBuffer& bits, Ac4EmdfInfo& emdf_info);
			Result ParsePresentationSubstreamInfo(CGolombBuffer& bits);
			Result GetPresentationChMode();
			Result GetPresentationChannelMask();
			Result GetPresB4BackChannelsPresent();
			Result GetPresTopChannelPairs();
			Result GetBPresentationCoreDiffers();
		};

		uint8_t ac4_dsi_version;
		struct v1 {
			uint8_t bitstream_version;
			uint8_t fs_index;
			uint32_t fs;
			uint8_t frame_rate_index;
			uint8_t b_program_id;
			uint16_t short_program_id;
			uint8_t b_uuid;
			uint8_t program_uuid[16];
			uint16_t n_presentations;
			PresentationV1* presentations;
		} v1;
	};

	Result Ac4Dsi::SubStream::ParseSubstreamInfoChan(CGolombBuffer& bits,
		unsigned int  presentation_version,
		unsigned char defalut_presentation_flag,
		unsigned int  fs_idx,
		unsigned int& speaker_index_mask,
		unsigned int  frame_rate_factor,
		unsigned int  b_substreams_present,
		unsigned char& dolby_atmos_indicator)
	{
		ch_mode = ParseChMode(bits, presentation_version, dolby_atmos_indicator);
		int substreamSpeakerGroupIndexMask = AC4_SPEAKER_GROUP_INDEX_MASK_BY_CH_MODE[ch_mode];
		if ((ch_mode >= CH_MODE_7_0_4) && (ch_mode <= CH_MODE_9_1_4)) {
			if (!(b_4_back_channels_present = bits.BitRead(1))) {    // b_4_back_channels_present false
				substreamSpeakerGroupIndexMask &= ~0x8;             // Remove back channels (Lb,Rb) from mask
			}
			if (!(b_centre_present = bits.BitRead(1))) {             // b_centre_present false
				substreamSpeakerGroupIndexMask &= ~0x2;             // Remove centre channel (C) from mask
			}
			switch (top_channels_present = bits.BitRead(2)) {      // top_channels_present
			case 0:
				substreamSpeakerGroupIndexMask &= ~0x30;        // Remove top channels (Tfl,Tfr,Tbl,Tbr) from mask
				break;
			case 1:
			case 2:
				substreamSpeakerGroupIndexMask &= ~0x30;        // Remove top channels (Tfl,Tfr,Tbl,Tbr) from mask
				substreamSpeakerGroupIndexMask |= 0x80;        // Add top channels (Tl, Tr) from mask;
				break;
			}
		}
		dsi_substream_channel_mask = substreamSpeakerGroupIndexMask;
		// Only combine channel masks of substream groups that are part of the first/default presentation
		if (defalut_presentation_flag) {
			speaker_index_mask |= substreamSpeakerGroupIndexMask;
		}

		ParseDsiSfMutiplier(bits, fs_idx);

		b_substream_bitrate_indicator = bits.BitRead(1);
		if (b_substream_bitrate_indicator) {    // b_bitrate_info
			// bitrate_indicator()
			ParseBitrateIndicator(bits);
		}

		if (ch_mode >= CH_MODE_70_52 && ch_mode <= CH_MODE_71_322) {
			bits.BitRead(1);                     // add_ch_base
		}
		for (unsigned int i = 0; i < frame_rate_factor; i++) {
			bits.BitRead(1);                     // b_audio_ndot
		}

		ParseSubstreamIdxInfo(bits, b_substreams_present);

		return SUCCESS;
	}

	Result Ac4Dsi::SubStream::ParseSubStreamInfoAjoc(CGolombBuffer& bits,
		unsigned int& channel_count,
		unsigned char defalut_presentation_flag,
		unsigned int  fs_idx,
		unsigned int  frame_rate_factor,
		unsigned int  b_substreams_present)
	{
		b_lfe = bits.BitRead(1);     // b_lfe
		b_static_dmx = bits.BitRead(1);
		if (b_static_dmx) {         // b_static_dmx
			if (defalut_presentation_flag) {
				channel_count += 5;
			}
		}
		else {
			n_dmx_objects_minus1 = bits.BitRead(4);
			unsigned int nFullbandDmxSignals = n_dmx_objects_minus1 + 1;
			BedDynObjAssignment(bits, nFullbandDmxSignals, false);
			if (defalut_presentation_flag) {
				channel_count += nFullbandDmxSignals;
			}
		}
		if (bits.BitRead(1)) {       // b_oamd_common_data_present
			// oamd_common_data()
			ParseOamdCommonData(bits);
		}
		int nFullbandUpmixSignalsMinus = bits.BitRead(4);
		int nFullbandUpmixSignals = nFullbandUpmixSignalsMinus + 1;
		if (nFullbandUpmixSignals == 16) {
			nFullbandUpmixSignals += Ac4VariableBits(bits, 3);
		}
		n_umx_objects_minus1 = nFullbandUpmixSignals - 1;

		BedDynObjAssignment(bits, nFullbandUpmixSignals, true);
		ParseDsiSfMutiplier(bits, fs_idx);

		b_substream_bitrate_indicator = bits.BitRead(1);
		if (b_substream_bitrate_indicator) {    // b_bitrate_info
			ParseBitrateIndicator(bits);
		}
		for (unsigned int i = 0; i < frame_rate_factor; i++) {
			bits.BitRead(1);                     // b_audio_ndot
		}
		ParseSubstreamIdxInfo(bits, b_substreams_present);
		return SUCCESS;
	}

	Result Ac4Dsi::SubStream::ParseSubstreamInfoObj(CGolombBuffer& bits,
		unsigned int& channel_count,
		unsigned char defalut_presentation_flag,
		unsigned int  fs_idx,
		unsigned int  frame_rate_factor,
		unsigned int  b_substreams_present)
	{
		int nObjectsCode = bits.BitRead(3);
		if (defalut_presentation_flag) {
			switch (nObjectsCode) {
			case 0:
			case 1:
			case 2:
			case 3:
				channel_count += nObjectsCode;
				break;
			case 4:
				channel_count += 5;
				break;
			default:
				break;
				//TODO: Error
			}
		}
		if (bits.BitRead(1)) {                       // b_dynamic_objects
			b_substream_contains_dynamic_objects = 1;
			unsigned int b_lfe = bits.BitRead(1);    // b_lfe
			if (defalut_presentation_flag && b_lfe) { channel_count += 1; }
		}
		else {
			if (bits.BitRead(1)) {                   // b_bed_objects
				b_substream_contains_bed_objects = 1;
				if (bits.BitRead(1)) {               // b_bed_start
					if (bits.BitRead(1)) {           // b_ch_assign_code
						bits.BitRead(3);           // bed_chan_assign_code
					}
					else {
						if (bits.BitRead(1)) {       // b_nonstd_bed_channel_assignment
							bits.BitRead(17);      // nonstd_bed_channel_assignment_mask
						}
						else {
							bits.BitRead(10);      // std_bed_channel_assignment_mask
						}
					}
				}
			}
			else {
				if (bits.BitRead(1)) {               // b_isf
					b_substream_contains_ISF_objects = 1;
					if (bits.BitRead(1)) {           // b_isf_start
						bits.BitRead(3);           // isf_config
					}
				}
				else {
					int resBytes = bits.BitRead(4);
					bits.BitRead(resBytes * 8);
				}
			}
		}

		ParseDsiSfMutiplier(bits, fs_idx);

		b_substream_bitrate_indicator = bits.BitRead(1);
		if (b_substream_bitrate_indicator) {        // b_bitrate_info
			ParseBitrateIndicator(bits);
		}
		for (unsigned int i = 0; i < frame_rate_factor; i++) {
			bits.BitRead(1);                         // b_audio_ndot
		}
		ParseSubstreamIdxInfo(bits, b_substreams_present);
		return SUCCESS;
	}

	Result Ac4Dsi::SubStream::ParseChMode(CGolombBuffer& bits, int presentationVersion, unsigned char& dolby_atmos_indicator)
	{
		int channel_mode_code = 0;

		channel_mode_code = bits.BitRead(1);
		if (channel_mode_code == 0) {   // Mono 0b0
			return CH_MODE_MONO;
		}
		channel_mode_code = (channel_mode_code << 1) | (int)bits.BitRead(1);
		if (channel_mode_code == 2) {   // Stereo  0b10
			return CH_MODE_STEREO;
		}
		channel_mode_code = (channel_mode_code << 2) | (int)bits.BitRead(2);
		switch (channel_mode_code) {
		case 12:                    // 3.0 0b1100
			return CH_MODE_3_0;
		case 13:                    // 5.0 0b1101
			return CH_MODE_5_0;
		case 14:                    // 5.1 0b1110
			return CH_MODE_5_1;
		}
		channel_mode_code = (channel_mode_code << 3) | (int)bits.BitRead(3);
		switch (channel_mode_code) {
		case 120:                   // 0b1111000
			if (presentationVersion == 2) { // IMS (all content)
				return CH_MODE_STEREO;
			}
			else {                  // 7.0: 3/4/0
				return CH_MODE_70_34;
			}
		case 121:                   // 0b1111001
			if (presentationVersion == 2) { // IMS (Atmos content)
				dolby_atmos_indicator |= 1;
				return CH_MODE_STEREO;
			}
			else {                  // 7.1: 3/4/0.1
				return CH_MODE_71_34;
			}
		case 122:                   // 7.0: 5/2/0   0b1111010
			return CH_MODE_70_52;
		case 123:                   // 7.1: 5/2/0.1 0b1111011
			return CH_MODE_71_52;
		case 124:                   // 7.0: 3/2/2   0b1111100
			return CH_MODE_70_322;
		case 125:                   // 7.1: 3/2/2.1 0b1111101
			return CH_MODE_71_322;
		}
		channel_mode_code = (channel_mode_code << 1) | (int)bits.BitRead(1);
		switch (channel_mode_code) {
		case 252:                   // 7.0.4 0b11111100
			return CH_MODE_7_0_4;
		case 253:                   // 7.1.4 0b11111101
			return CH_MODE_7_1_4;
		}
		channel_mode_code = (channel_mode_code << 1) | (int)bits.BitRead(1);
		switch (channel_mode_code) {
		case 508:                   // 9.0.4 0b111111100
			return CH_MODE_9_0_4;
		case 509:                   // 9.1.4 0b111111101
			return CH_MODE_9_1_4;
		case 510:                   // 22.2 0b111111110
			return CH_MODE_22_2;
		case 511:                   // Reserved, escape value 0b111111111
		default:
			Ac4VariableBits(bits, 2);
			return CH_MODE_RESERVED;
		}
	}

	Result Ac4Dsi::SubStream::ParseDsiSfMutiplier(CGolombBuffer& bits,
		unsigned int  fs_idx)
	{
		if (fs_idx == 1) {
			if (bits.BitRead(1)) {                       // b_sf_miultiplier
				// 96 kHz or 192 kHz
				dsi_sf_multiplier = bits.BitRead(1) + 1; // sf_multiplier
			}
			else {
				// 48 kHz
				dsi_sf_multiplier = 0;
			}
		}
		return SUCCESS;
	}

	Result Ac4Dsi::SubStream::BedDynObjAssignment(CGolombBuffer& bits,
		unsigned int  nSignals,
		bool          is_upmix)
	{
		if (!bits.BitRead(1)) {      // b_dyn_objects_only
			if (bits.BitRead(1)) {   // b_isf
				unsigned char isf_config = bits.BitRead(3);
				if (is_upmix) {
					b_substream_contains_ISF_objects |= 1;
					if (nSignals > ObjNumFromIsfConfig(isf_config)) {
						b_substream_contains_dynamic_objects |= 1;
					}
				}
			}
			else {
				if (bits.BitRead(1)) {           // b_ch_assign_code
					unsigned char bed_chan_assign_code = bits.BitRead(3);
					if (is_upmix) {
						b_substream_contains_bed_objects |= 1;
						if (nSignals > BedNumFromAssignCode(bed_chan_assign_code)) {
							b_substream_contains_dynamic_objects |= 1;
						}
					}
				}
				else {
					if (bits.BitRead(1)) {       // b_chan_assign_mask
						if (bits.BitRead(1)) {   // b_nonstd_bed_channel_assignment
							unsigned int nonstd_bed_channel_assignment_mask = bits.BitRead(17);
							if (is_upmix) {
								unsigned int bed_num = BedNumFromNonStdMask(nonstd_bed_channel_assignment_mask);
								if (bed_num > 0) { b_substream_contains_bed_objects |= 1; }
								if (nSignals > bed_num) {
									b_substream_contains_dynamic_objects |= 1;
								}
							}
						}
						else {
							unsigned int std_bed_channel_assignment_mask = bits.BitRead(10);
							if (is_upmix) {
								unsigned int bed_num = BedNumFromStdMask(std_bed_channel_assignment_mask);
								if (bed_num > 0) { b_substream_contains_bed_objects |= 1; }
								if (nSignals > bed_num) {
									b_substream_contains_dynamic_objects |= 1;
								}
							}
						}
					}
					else {
						unsigned int nBedSignals = 0;
						if (nSignals > 1) {
							int bedChBits = (int)ceil(log((float)nSignals) / log((float)2));
							nBedSignals = bits.BitRead(bedChBits) + 1;
						}
						else {
							nBedSignals = 1;
						}
						for (unsigned int b = 0; b < nBedSignals; b++) {
							bits.BitRead(4);   // nonstd_bed_channel_assignment
						}
						if (is_upmix) {
							b_substream_contains_bed_objects |= 1;
							if (nSignals > nBedSignals) { b_substream_contains_dynamic_objects |= 1; }
						}
					}
				}
			}
		}
		else {
			if (is_upmix) {
				b_substream_contains_dynamic_objects |= 1;
				b_substream_contains_bed_objects |= 0;
				b_substream_contains_ISF_objects |= 0;
			}
		}
		return SUCCESS;
	}

	Result Ac4Dsi::SubStream::ParseSubstreamIdxInfo(CGolombBuffer& bits, unsigned int b_substreams_present)
	{
		if (b_substreams_present == 1) {
			if (bits.BitRead(2) == 3) {    // substream_index
				Ac4VariableBits(bits, 2);
			}
		}
		return SUCCESS;
	}

	Result Ac4Dsi::SubStream::ParseBitrateIndicator(CGolombBuffer& bits)
	{
		substream_bitrate_indicator = bits.BitRead(3);
		if ((substream_bitrate_indicator & 0x1) == 1) { // bitrate_indicator
			substream_bitrate_indicator = (substream_bitrate_indicator << 2) + (int)bits.BitRead(2);
		}
		return SUCCESS;
	}

	Result Ac4Dsi::SubStream::ParseOamdCommonData(CGolombBuffer& bits)
	{
		if (bits.BitRead(1) == 0) {  // b_default_screen_size_ratio
			bits.BitRead(5);       // master_screen_size_ratio_code
		}
		bits.BitRead(1);             // b_bed_object_chan_distribute
		if (bits.BitRead(1)) {       // b_additional_data
			int addDataBytes = bits.BitRead(1) + 1;
			if (addDataBytes == 2) {
				addDataBytes += Ac4VariableBits(bits, 2);
			}
			unsigned int bits_used = Trim(bits);
			bits_used += BedRendeInfo(bits);
			bits.BitRead(addDataBytes * 8 - bits_used);
		}
		return SUCCESS;
	}

	Result Ac4Dsi::SubStream::Trim(CGolombBuffer& bits)
	{
		return SUCCESS;
	}

	Result Ac4Dsi::SubStream::BedRendeInfo(CGolombBuffer& bits)
	{
		return SUCCESS;
	}

	uint32_t Ac4Dsi::SubStream::ObjNumFromIsfConfig(unsigned char isf_config)
	{
		unsigned int obj_num = 0;
		switch (isf_config) {
		case 0: obj_num = 4; break;
		case 1: obj_num = 8; break;
		case 2: obj_num = 10; break;
		case 3: obj_num = 14; break;
		case 4: obj_num = 15; break;
		case 5: obj_num = 30; break;
		default: obj_num = 0;
		}
		return obj_num;
	}

	uint32_t Ac4Dsi::SubStream::BedNumFromAssignCode(unsigned char assign_code)
	{
		unsigned int bed_num = 0;
		switch (assign_code) {
		case 0: bed_num = 2; break;
		case 1: bed_num = 3; break;
		case 2: bed_num = 6; break;
		case 3: bed_num = 8; break;
		case 4: bed_num = 10; break;
		case 5: bed_num = 8; break;
		case 6: bed_num = 10; break;
		case 7: bed_num = 12; break;
		default: bed_num = 0;
		}
		return bed_num;
	}

	uint32_t Ac4Dsi::SubStream::BedNumFromNonStdMask(unsigned int non_std_mask)
	{
		unsigned int bed_num = 0;
		// Table 85: nonstd_bed_channel_assignment AC-4 part-2 v1.2.1
		for (unsigned int idx = 0; idx < 17; idx++) {
			if ((non_std_mask >> idx) & 0x1) {
				bed_num++;
			}
		}
		return bed_num;
	}

	uint32_t Ac4Dsi::SubStream::BedNumFromStdMask(unsigned int std_mask)
	{
		unsigned int bed_num = 0;
		// Table 86 std_bed_channel_assignment_flag[] AC-4 part-2 v1.2.1
		for (unsigned int idx = 0; idx < 10; idx++) {
			if ((std_mask >> idx) & 0x1) {
				if ((idx == 1) || (idx == 2) || (idx == 9)) { bed_num++; }
				else { bed_num += 2; }
			}
		}
		return bed_num;
	}

	Result Ac4Dsi::SubStream::GetChModeCore(unsigned char b_channel_coded)
	{
		int ch_mode_core = -1;
		if (b_channel_coded == 0 &&
			b_ajoc == 1 &&
			b_static_dmx == 1 &&
			b_lfe == 0) {
			ch_mode_core = 3;
		}
		else if (b_channel_coded == 0 &&
			b_ajoc == 1 &&
			b_static_dmx == 1 &&
			b_lfe == 1) {
			ch_mode_core = 4;
		}
		else if ((b_channel_coded == 1) &&
			(ch_mode == 11 || ch_mode == 13)) {
			ch_mode_core = 5;
		}
		else if ((b_channel_coded == 1) &&
			(ch_mode == 12 || ch_mode == 14)) {
			ch_mode_core = 6;
		}
		return ch_mode_core;
	}

	Result Ac4Dsi::SubStreamGroupV1::ParseSubstreamGroupInfo(CGolombBuffer& bits,
		unsigned int   bitstream_version,
		unsigned int   presentation_version,
		unsigned char  defalut_presentation_flag,
		unsigned int   frame_rate_factor,
		unsigned int   fs_idx,
		unsigned int& channel_count,
		unsigned int& speaker_index_mask,
		unsigned int& b_obj_or_Ajoc)
	{
		v1.b_substreams_present = bits.BitRead(1);
		v1.b_hsf_ext = bits.BitRead(1);
		if (bits.BitRead(1)) {
			v1.n_substreams = 1;
		}
		else {
			v1.n_substreams = bits.BitRead(2) + 2;
			if (v1.n_substreams == 5) {
				v1.n_substreams += Ac4VariableBits(bits, 2);
			}
		}
		v1.substreams.resize(v1.n_substreams);
		v1.b_channel_coded = bits.BitRead(1);
		if (v1.b_channel_coded) {
			for (unsigned int sus = 0; sus < v1.n_substreams; sus++) {
				Ac4Dsi::SubStream& substream = v1.substreams[sus];
				if (bitstream_version == 1) {
					bits.BitRead(1); // sus_ver
				}

				// ac4_substream_info_chan()
				substream.ParseSubstreamInfoChan(bits,
					presentation_version,
					defalut_presentation_flag,
					fs_idx,
					speaker_index_mask,
					frame_rate_factor,
					v1.b_substreams_present,
					v1.dolby_atmos_indicator);

				if (v1.b_hsf_ext) {
					// ac4_hsf_ext_substream_info()
					ParseHsfExtSubstreamInfo(bits);
				}
			}
		}
		else {     // b_channel_coded == 0
			b_obj_or_Ajoc = 1;
			if (bits.BitRead(1)) {       // b_oamd_substream
				//oamd_substream_info()
				ParseOamdSubstreamInfo(bits);
			}
			for (int sus = 0; sus < v1.n_substreams; sus++) {
				Ac4Dsi::SubStream& substream = v1.substreams[sus];
				substream.b_ajoc = bits.BitRead(1);
				unsigned int local_channel_count = 0;
				if (substream.b_ajoc) { // b_ajoc
					// ac4_substream_info_ajoc()
					substream.ParseSubStreamInfoAjoc(bits,
						local_channel_count,
						defalut_presentation_flag,
						fs_idx,
						frame_rate_factor,
						v1.b_substreams_present);

					if (v1.b_hsf_ext) {
						// ac4_hsf_ext_substream_info()
						ParseHsfExtSubstreamInfo(bits);
					}
				}
				else {
					// ac4_substream_info_obj()
					substream.ParseSubstreamInfoObj(bits,
						local_channel_count,
						defalut_presentation_flag,
						fs_idx,
						frame_rate_factor,
						v1.b_substreams_present);

					if (v1.b_hsf_ext) {
						// ac4_hsf_ext_substream_info()
						ParseHsfExtSubstreamInfo(bits);
					}
				}
				if (channel_count < local_channel_count) { channel_count = local_channel_count; }
			}
		}

		v1.b_content_type = bits.BitRead(1);
		if (v1.b_content_type) {
			// content_type()
			ParseContentType(bits);
		}
		return SUCCESS;
	}

	Result Ac4Dsi::SubStreamGroupV1::ParseOamdSubstreamInfo(CGolombBuffer& bits)
	{
		bits.BitRead(1);                     // b_oamd_ndot
		if (v1.b_substreams_present == 1) {
			if (bits.BitRead(2) == 3) {    // substream_index
				Ac4VariableBits(bits, 2);
			}
		}
		return SUCCESS;
	}

	Result Ac4Dsi::SubStreamGroupV1::ParseHsfExtSubstreamInfo(CGolombBuffer& bits)
	{
		if (v1.b_substreams_present == 1) {
			if (bits.BitRead(2) == 3) {    // substream_index
				Ac4VariableBits(bits, 2);
			}
		}
		return SUCCESS;
	}

	Result Ac4Dsi::SubStreamGroupV1::ParseContentType(CGolombBuffer& bits)
	{
		v1.content_classifier = bits.BitRead(3); // content_classifier
		v1.b_language_indicator = bits.BitRead(1);
		if (v1.b_language_indicator == 1) {       // b_language_indicator
			if (bits.BitRead(1)) {                   // b_serialized_language_tag
				bits.BitRead(17);                  // b_start_tag, language_tag_chunk
			}
			else {
				v1.n_language_tag_bytes = bits.BitRead(6);
				for (unsigned int l = 0; l < v1.n_language_tag_bytes; l++) {
					v1.language_tag_bytes[l] = bits.BitRead(8);
				}
			}
		}
		return SUCCESS;
	}

	Result Ac4Dsi::PresentationV1::ParsePresentationV1Info(CGolombBuffer& bits,
		unsigned int  bitstream_version,
		unsigned int  frame_rate_idx,
		unsigned int  pres_idx,
		unsigned int& max_group_index,
		unsigned int** first_pres_sg_index,
		unsigned int& first_pres_sg_num)
	{
		unsigned int b_single_substream_group = bits.BitRead(1);
		if (b_single_substream_group != 1) {
			v1.presentation_config_v1 = bits.BitRead(3);
			if (v1.presentation_config_v1 == 7) {
				v1.presentation_config_v1 += Ac4VariableBits(bits, 2);
			}
		}
		else {
			v1.presentation_config_v1 = 0x1f;
		}

		// presentation_version()
		ParsePresentationVersion(bits, bitstream_version);

		Ac4EmdfInfo tmp_emdf_info;
		if (b_single_substream_group != 1 && v1.presentation_config_v1 == 6) {
			v1.b_add_emdf_substreams = 1;
		}
		else {
			if (bitstream_version != 1) {
				v1.mdcompat = bits.BitRead(3);
			}
			v1.b_presentation_id = bits.BitRead(1);
			if (v1.b_presentation_id) {
				v1.presentation_id = Ac4VariableBits(bits, 2);
			}
			// frame_rate_multiply_info()
			ParseDSIFrameRateMultiplyInfo(bits, frame_rate_idx);

			// frame_rate_fractions_info()
			ParseDSIFrameRateFractionsInfo(bits, frame_rate_idx);

			// emdf_info()
			ParseEmdInfo(bits, tmp_emdf_info);
			v1.presentation_emdf_version = tmp_emdf_info.emdf_version;
			v1.presentation_key_id = tmp_emdf_info.key_id;

			// b_presentation_filter
			v1.b_presentation_filter = bits.BitRead(1);
			if (v1.b_presentation_filter == 1) {
				v1.b_enable_presentation = bits.BitRead(1);
			}
			if (b_single_substream_group == 1) {
				v1.n_substream_groups = 1;
				v1.substream_group_indexs.resize(v1.n_substream_groups);
				v1.substream_group_indexs[0] = ParseAc4SgiSpecifier(bits, bitstream_version);
				max_group_index = SetMaxGroupIndex(v1.substream_group_indexs[0], max_group_index);
			}
			else {
				v1.b_multi_pid = bits.BitRead(1);
				switch (v1.presentation_config_v1) {
				case 0: // M&E + D
				case 2: // Main + Assoc
					v1.n_substream_groups = 2;
					v1.substream_group_indexs.resize(v1.n_substream_groups);
					v1.substream_group_indexs[0] = ParseAc4SgiSpecifier(bits, bitstream_version);
					v1.substream_group_indexs[1] = ParseAc4SgiSpecifier(bits, bitstream_version);
					max_group_index = SetMaxGroupIndex(v1.substream_group_indexs[0], max_group_index);
					max_group_index = SetMaxGroupIndex(v1.substream_group_indexs[1], max_group_index);
					break;
				case 1: // Main + DE
					// shall return same substream group index, need stream to verify
					v1.n_substream_groups = 2;
					v1.substream_group_indexs.resize(v1.n_substream_groups);
					v1.substream_group_indexs[0] = ParseAc4SgiSpecifier(bits, bitstream_version);
					v1.substream_group_indexs[1] = ParseAc4SgiSpecifier(bits, bitstream_version);
					max_group_index = SetMaxGroupIndex(v1.substream_group_indexs[0], max_group_index);
					max_group_index = SetMaxGroupIndex(v1.substream_group_indexs[1], max_group_index);
					break;
				case 3: // M&E + D + Assoc
					v1.n_substream_groups = 3;
					v1.substream_group_indexs.resize(v1.n_substream_groups);
					v1.substream_group_indexs[0] = ParseAc4SgiSpecifier(bits, bitstream_version);
					v1.substream_group_indexs[1] = ParseAc4SgiSpecifier(bits, bitstream_version);
					v1.substream_group_indexs[2] = ParseAc4SgiSpecifier(bits, bitstream_version);
					max_group_index = SetMaxGroupIndex(v1.substream_group_indexs[0], max_group_index);
					max_group_index = SetMaxGroupIndex(v1.substream_group_indexs[1], max_group_index);
					max_group_index = SetMaxGroupIndex(v1.substream_group_indexs[2], max_group_index);
					break;
				case 4: // Main + DE + Assoc
					// shall return only two substream group index, need stream to verify
					v1.n_substream_groups = 3;
					v1.substream_group_indexs.resize(v1.n_substream_groups);
					v1.substream_group_indexs[0] = ParseAc4SgiSpecifier(bits, bitstream_version);
					v1.substream_group_indexs[1] = ParseAc4SgiSpecifier(bits, bitstream_version);
					v1.substream_group_indexs[2] = ParseAc4SgiSpecifier(bits, bitstream_version);
					max_group_index = SetMaxGroupIndex(v1.substream_group_indexs[0], max_group_index);
					max_group_index = SetMaxGroupIndex(v1.substream_group_indexs[1], max_group_index);
					max_group_index = SetMaxGroupIndex(v1.substream_group_indexs[2], max_group_index);
					break;
				case 5:
					v1.n_substream_groups = bits.BitRead(2) + 2;
					if (v1.n_substream_groups == 5) {
						v1.n_substream_groups += Ac4VariableBits(bits, 2);
					}
					v1.substream_group_indexs.resize(v1.n_substream_groups);
					for (unsigned int sg = 0; sg < v1.n_substream_groups; sg++) {
						v1.substream_group_indexs[sg] = ParseAc4SgiSpecifier(bits, bitstream_version);
						max_group_index = SetMaxGroupIndex(v1.substream_group_indexs[sg], max_group_index);
					}
					break;
				default:
					// presentation_config_ext_info()
					ParsePresentationConfigExtInfo(bits, bitstream_version);
					break;
				}
			}
			// b_pre_virtualized
			v1.b_pre_virtualized = bits.BitRead(1);
			v1.b_add_emdf_substreams = bits.BitRead(1);
			// ac4_presentation_substream_info()
			ParsePresentationSubstreamInfo(bits);
		}

		if (v1.b_add_emdf_substreams) {
			v1.n_add_emdf_substreams = bits.BitRead(2);
			if (v1.n_add_emdf_substreams == 0) {
				v1.n_add_emdf_substreams = Ac4VariableBits(bits, 2) + 4;
			}
			for (unsigned int cnt = 0; cnt < v1.n_add_emdf_substreams; cnt++) {
				ParseEmdInfo(bits, tmp_emdf_info);
				v1.substream_emdf_version[cnt] = tmp_emdf_info.emdf_version;
				v1.substream_key_id[cnt] = tmp_emdf_info.key_id;
			}
		}
		if (pres_idx == 0) {
			*first_pres_sg_index = v1.substream_group_indexs.data();
			first_pres_sg_num = v1.n_substream_groups;
		}
		return SUCCESS;
	}

	Result Ac4Dsi::PresentationV1::ParsePresentationVersion(CGolombBuffer& bits, unsigned int bitstream_version)
	{
		presentation_version = 0;
		if (bitstream_version != 1) {
			while (bits.BitRead(1) == 1) {
				presentation_version++;
			}
		}
		return SUCCESS;
	}

	Result Ac4Dsi::PresentationV1::ParsePresentationConfigExtInfo(CGolombBuffer& bits, unsigned int bitstream_version)
	{
		unsigned int nSkipBytes = bits.BitRead(5);
		if (bits.BitRead(1)) {  // b_more_skip_bytes
			nSkipBytes += (Ac4VariableBits(bits, 2) << 5);
		}
		if (bitstream_version == 1 && v1.presentation_config_v1 == 7) {
			// TODO: refer to chapte 6.2.1.5 - TS 103 190-2
		}
		bits.BitRead(nSkipBytes * 8);
		return SUCCESS;
	}

	uint32_t Ac4Dsi::PresentationV1::ParseAc4SgiSpecifier(CGolombBuffer& bits, unsigned int bitstream_version)
	{
		if (bitstream_version == 1) {
			// Error
			return 0;
		}
		else {
			unsigned int groupIndex = bits.BitRead(3);
			if (groupIndex == 7) {
				groupIndex += Ac4VariableBits(bits, 2);
			}
			return groupIndex;
		}
	}

	Result Ac4Dsi::PresentationV1::ParseDSIFrameRateMultiplyInfo(CGolombBuffer& bits, unsigned int frame_rate_idx)
	{
		switch (frame_rate_idx) {
		case 2:
		case 3:
		case 4:
			if (bits.BitRead(1)) {                               // b_multiplier
				unsigned int multiplier_bit = bits.BitRead(1);   //multiplier_bit
				v1.dsi_frame_rate_multiply_info = (multiplier_bit == 0) ? 1 : 2;
			}
			else {
				v1.dsi_frame_rate_multiply_info = 0;
			}
			break;
		case 0:
		case 1:
		case 7:
		case 8:
		case 9:
			if (bits.BitRead(1)) {
				v1.dsi_frame_rate_multiply_info = 1;
			}
			else {
				v1.dsi_frame_rate_multiply_info = 0;
			}
			break;
		default:
			v1.dsi_frame_rate_multiply_info = 0;
			break;
		}
		return SUCCESS;
	}

	Result Ac4Dsi::PresentationV1::ParseDSIFrameRateFractionsInfo(CGolombBuffer& bits, unsigned int frame_rate_idx)
	{
		if (frame_rate_idx >= 5 && frame_rate_idx <= 9) {
			if (bits.BitRead(1) == 1) {      // b_frame_rate_fraction
				v1.dsi_frame_rate_fraction_info = 1;
			}
			else {
				v1.dsi_frame_rate_fraction_info = 0;
			}
		}
		else if (frame_rate_idx >= 10 && frame_rate_idx <= 12) {
			if (bits.BitRead(1) == 1) {      // b_frame_rate_fraction
				if (bits.BitRead(1) == 1) {  // b_frame_rate_fraction_is_4
					v1.dsi_frame_rate_fraction_info = 2;
				}
				else {
					v1.dsi_frame_rate_fraction_info = 1;
				}

			}
			else {
				v1.dsi_frame_rate_fraction_info = 0;
			}
		}
		return SUCCESS;
	}

	Result Ac4Dsi::PresentationV1::ParseEmdInfo(CGolombBuffer& bits, Ac4EmdfInfo& emdf_info)
	{
		emdf_info.emdf_version = bits.BitRead(2);
		if (emdf_info.emdf_version == 3) {
			emdf_info.emdf_version += Ac4VariableBits(bits, 2);
		}
		emdf_info.key_id = bits.BitRead(3);
		if (emdf_info.key_id == 7) {
			emdf_info.key_id += Ac4VariableBits(bits, 3);
		}
		emdf_info.b_emdf_payloads_substream_info = bits.BitRead(1);
		if (emdf_info.b_emdf_payloads_substream_info == 1) {
			// emdf_payloads_substream_info ()
			if (bits.BitRead(2) == 3) {    // substream_index
				Ac4VariableBits(bits, 2);
			}
		}
		emdf_info.protectionLengthPrimary = bits.BitRead(2);
		emdf_info.protectionLengthSecondary = bits.BitRead(2);
		// protection_bits_primary
		switch (emdf_info.protectionLengthPrimary) {
		case 1:
			emdf_info.protection_bits_primary[0] = bits.BitRead(8);
			break;
		case 2:
			for (unsigned idx = 0; idx < 4; idx++) { emdf_info.protection_bits_primary[idx] = bits.BitRead(8); }
			break;
		case 3:
			for (unsigned idx = 0; idx < 16; idx++) { emdf_info.protection_bits_primary[idx] = bits.BitRead(8); }
			break;
		default:
			// Error
			break;
		}
		// protection_bits_secondary
		switch (emdf_info.protectionLengthSecondary) {
		case 0:
			break;
		case 1:
			emdf_info.protection_bits_Secondary[0] = bits.BitRead(8);
			break;
		case 2:
			for (unsigned idx = 0; idx < 4; idx++) { emdf_info.protection_bits_Secondary[idx] = bits.BitRead(8); }
			break;
		case 3:
			for (unsigned idx = 0; idx < 16; idx++) { emdf_info.protection_bits_Secondary[idx] = bits.BitRead(8); }
			break;
		default:
			// TODO: Error
			break;
		}
		return SUCCESS;
	}

	Result Ac4Dsi::PresentationV1::ParsePresentationSubstreamInfo(CGolombBuffer& bits)
	{
		v1.b_alternative = bits.BitRead(1);
		/* unsigned int b_pres_ndot = */ bits.BitRead(1);          // b_pres_ndot;
		unsigned int substream_index = bits.BitRead(2);    //substream_index
		if (substream_index == 3) {
			Ac4VariableBits(bits, 2);
		}
		return SUCCESS;
	}

	Result Ac4Dsi::PresentationV1::GetPresentationChMode()
	{
		int pres_ch_mode = -1;
		char b_obj_or_ajoc = 0;
		// TODO: n_substream_groups
		for (unsigned int sg = 0; sg < v1.n_substream_groups; sg++) {
			Ac4Dsi::SubStreamGroupV1& substream_group = v1.substream_groups[sg];
			unsigned int n_substreams = v1.substream_groups[sg].v1.n_substreams;
			for (unsigned int sus = 0; sus < n_substreams; sus++) {
				Ac4Dsi::SubStream& substream = substream_group.v1.substreams[sus];
				if (substream_group.v1.b_channel_coded) {
					pres_ch_mode = Ac4SuperSet(pres_ch_mode, substream.ch_mode);
				}
				else {
					b_obj_or_ajoc = 1;
				}
			}
		}
		if (b_obj_or_ajoc == 1) { pres_ch_mode = -1; }
		return pres_ch_mode;
	}

	Result Ac4Dsi::PresentationV1::GetPresentationChannelMask()
	{
		unsigned int channel_mask = 0;
		char b_obj_or_ajoc = 0;
		// TODO: n_substream_groups
		for (unsigned int sg = 0; sg < v1.n_substream_groups; sg++) {
			Ac4Dsi::SubStreamGroupV1& substream_group = v1.substream_groups[sg];
			unsigned int n_substreams = v1.substream_groups[sg].v1.n_substreams;
			for (unsigned int sus = 0; sus < n_substreams; sus++) {
				Ac4Dsi::SubStream& substream = substream_group.v1.substreams[sus];
				if (substream_group.v1.b_channel_coded) {
					channel_mask |= substream.dsi_substream_channel_mask;
				}
				else {
					b_obj_or_ajoc = 1;
				}
			}
		}
		//Modify the mask for headphone presentation with b_pre_virtualized = 1
		/*
		if (presentation_version == 2 || v1.b_pre_virtualized == 1){
			if (channel_mask == 0x03) { channel_mask = 0x01;}
		}
		*/
		// TODO: temporary solution according to Dolby's internal discussion
		if (channel_mask == 0x03) { channel_mask = 0x01; }

		// If one substream contains Tfl, Tfr, Tbl, Tbr, Tl and Tr shall be removed.
		if ((channel_mask & 0x30) && (channel_mask & 0x80)) { channel_mask &= ~0x80; }

		// objective channel mask
		if (b_obj_or_ajoc == 1) { channel_mask = 0x800000; }
		return channel_mask;
	}

	Result Ac4Dsi::PresentationV1::GetPresB4BackChannelsPresent()
	{
		for (unsigned int sg = 0; sg < v1.n_substream_groups; sg++) {
			Ac4Dsi::SubStreamGroupV1& substream_group = v1.substream_groups[sg];
			for (unsigned int sus = 0; sus < substream_group.v1.n_substreams; sus++) {
				v1.pres_b_4_back_channels_present |= substream_group.v1.substreams[sus].b_4_back_channels_present;
			}
		}
		return SUCCESS;
	}

	Result Ac4Dsi::PresentationV1::GetPresTopChannelPairs()
	{
		unsigned char tmp_pres_top_channel_pairs = 0;
		for (unsigned int sg = 0; sg < v1.n_substream_groups; sg++) {
			Ac4Dsi::SubStreamGroupV1& substream_group = v1.substream_groups[sg];
			for (unsigned int sus = 0; sus < substream_group.v1.n_substreams; sus++) {
				if (tmp_pres_top_channel_pairs < substream_group.v1.substreams[sus].top_channels_present) {
					tmp_pres_top_channel_pairs = substream_group.v1.substreams[sus].top_channels_present;
				}
			}
		}
		switch (tmp_pres_top_channel_pairs) {
		case 0:
			v1.pres_top_channel_pairs = 0; break;
		case 1:
		case 2:
			v1.pres_top_channel_pairs = 1; break;
		case 3:
			v1.pres_top_channel_pairs = 2; break;
		default:
			v1.pres_top_channel_pairs = 0; break;
		}
		return SUCCESS;
	}

	Result Ac4Dsi::PresentationV1::GetBPresentationCoreDiffers()
	{
		int pres_ch_mode_core = -1;
		char b_obj_or_ajoc_adaptive = 0;
		for (unsigned int sg = 0; sg < v1.n_substream_groups; sg++) {
			Ac4Dsi::SubStreamGroupV1& substream_group = v1.substream_groups[sg];
			unsigned int n_substreams = v1.substream_groups[sg].v1.n_substreams;
			for (unsigned int sus = 0; sus < n_substreams; sus++) {
				Ac4Dsi::SubStream& substream = substream_group.v1.substreams[sus];
				if (substream_group.v1.b_channel_coded) {
					pres_ch_mode_core = Ac4SuperSet(pres_ch_mode_core, substream.GetChModeCore(substream_group.v1.b_channel_coded));
				}
				else {
					if (substream.b_ajoc) {
						if (substream.b_static_dmx) {
							pres_ch_mode_core = Ac4SuperSet(pres_ch_mode_core, substream.GetChModeCore(substream_group.v1.b_channel_coded));
						}
						else {
							b_obj_or_ajoc_adaptive = 1;
						}
					}
					else {
						b_obj_or_ajoc_adaptive = 1;
					}
				}
			}
		}
		if (b_obj_or_ajoc_adaptive) {
			pres_ch_mode_core = -1;
		}
		if (pres_ch_mode_core == GetPresentationChMode()) {
			pres_ch_mode_core = -1;
		}
		return pres_ch_mode_core;
	}
}

#define READ_B16(buf) (((buf)[0] << 8) | (buf)[1])
#define READ_B24(buf) (((buf)[0] << 16) | ((buf)[1] << 8) | (buf)[2])

uint32_t ParseAC4Header(const BYTE* buf, int len, audioframe_t* audioframe)
{
	if (len < 8) {
		return 0;
	}

	uint16_t syncWord = READ_B16(buf);
	if (syncWord != AC4_SYNC_WORD && syncWord != AC4_SYNC_WORD_CRC) {
		return 0;
	}

	uint32_t frameSize = READ_B16(buf + 2);
	uint16_t headerSize = 4;
	if (frameSize == 0xFFFF) {
		frameSize = READ_B24(buf + 4);
		headerSize += 3;
	}
	frameSize += headerSize;
	if (syncWord == AC4_SYNC_WORD_CRC) {
		frameSize += 2;
	}

	if (audioframe) {
		*audioframe = {};

		CGolombBuffer bits(buf + headerSize, len - headerSize);

		uint32_t bitstreamVersion = bits.BitRead(2);
		if (bitstreamVersion == 3) {
			bitstreamVersion = AC4::Ac4VariableBits(bits, 2);
		}

		bits.BitRead(10);

		if (bits.BitRead(1)) {
			if (bits.BitRead(3) > 0) {
				bits.BitRead(2);
			}
		}

		uint8_t fsIndex = bits.BitRead(1);
		uint8_t frameRateIndex = bits.BitRead(4);

		audioframe->size = frameSize;
		audioframe->samplerate = (fsIndex == 0) ? 44100 : 48000;
		audioframe->channels = 2;

		static const int FrameSamples[] {
			2002,
			2000,
			1920,
			1601,
			1600,
			1001,
			1000,
			960,
			800,
			800,
			480,
			400,
			400,
			2048
		};

		if (fsIndex == 0) {
			if (frameRateIndex == 13) {
				audioframe->samples = 2048;
			}
		} else if (frameRateIndex < std::size(FrameSamples)) {
			audioframe->samples = FrameSamples[frameRateIndex];
		}

		if (bitstreamVersion <= 1) {
			// deprecated
			return frameSize;
		}

		bits.BitRead(1);

		uint32_t nPresentations = 0;
		if (bits.BitRead(1)) {
			nPresentations = 1;
		}
		else {
			if (bits.BitRead(1)) {
				nPresentations = AC4::Ac4VariableBits(bits, 2) + 2;
			}
			else {
				nPresentations = 0;
			}
		}

		if (!nPresentations) {
			return 0;
		}

		if (bits.BitRead(1)) {
			uint32_t payloadBase = bits.BitRead(5) + 1;
			if (payloadBase == 0x20) {
				payloadBase = AC4::Ac4VariableBits(bits, 3);
			}
		}

		if (bits.BitRead(1)) {
			bits.BitRead(16);
			if (bits.BitRead(1)) {
				for (int cnt = 0; cnt < 16; cnt++) {
					bits.BitRead(8);
				}
			}
		}

		unsigned int maxGroupIndex = 0;
		unsigned int* firstPresentationSubstreamGroupIndexes = NULL;
		unsigned int firstPresentationNSubstreamGroups = 0;

		std::vector<AC4::Ac4Dsi::PresentationV1> PresentationV1(nPresentations);
		for (unsigned int pres_idx = 0; pres_idx < nPresentations; pres_idx++) {
			AC4::Ac4Dsi::PresentationV1& presentation = PresentationV1[pres_idx];
			presentation.ParsePresentationV1Info(bits,
				bitstreamVersion,
				frameRateIndex,
				pres_idx,
				maxGroupIndex,
				&firstPresentationSubstreamGroupIndexes,
				firstPresentationNSubstreamGroups);
		}

		auto GetPresentationIndexBySGIndex = [&](unsigned int substream_group_index) -> AC4::Result {
			for (unsigned int idx = 0; idx < nPresentations; idx++) {
				for (unsigned int sg = 0; sg < PresentationV1[idx].v1.n_substream_groups; sg++) {
					if (substream_group_index == PresentationV1[idx].v1.substream_group_indexs[sg]) {
						return idx;
					}
				}
			}
			return AC4::FAILURE;
			};
		auto GetPresentationVersionBySGIndex = [&](unsigned int substream_group_index) -> AC4::Result {
			for (unsigned int idx = 0; idx < nPresentations; idx++) {
				for (unsigned int sg = 0; sg < PresentationV1[idx].v1.n_substream_groups; sg++) {
					if (substream_group_index == PresentationV1[idx].v1.substream_group_indexs[sg]) {
						return PresentationV1[idx].presentation_version;
					}
				}
			}
			return AC4::FAILURE;
			};
		auto Ac4SubstreamGroupPartOfDefaultPresentation = [](unsigned int substreamGroupIndex, unsigned int presSubstreamGroupIndexes[], unsigned int presNSubstreamGroups) -> AC4::Result {
			int partOf = false;
			for (unsigned int idx = 0; idx < presNSubstreamGroups; idx++) {
				if (presSubstreamGroupIndexes[idx] == substreamGroupIndex) { partOf = true; }
			}
			return partOf;
			};

		unsigned int bObjorAjoc = 0;
		unsigned int channelCount = 0;
		unsigned int speakerGroupIndexMask = 0;
		std::vector<AC4::Ac4Dsi::SubStreamGroupV1> substream_groups(maxGroupIndex + 1);
		for (unsigned int sg = 0; (sg < (maxGroupIndex + 1)) && (nPresentations > 0); sg++) {
			AC4::Ac4Dsi::SubStreamGroupV1& substream_group = substream_groups[sg];
			AC4::Result pres_index = GetPresentationIndexBySGIndex(sg);
			if (pres_index == AC4::FAILURE) {
				break;
			}
			unsigned int localChannelCount = 0;
			unsigned int frame_rate_factor = (PresentationV1[pres_index].v1.dsi_frame_rate_multiply_info == 0) ? 1 : (PresentationV1[pres_index].v1.dsi_frame_rate_multiply_info * 2);
			substream_group.ParseSubstreamGroupInfo(bits,
				bitstreamVersion,
				GetPresentationVersionBySGIndex(sg),
				Ac4SubstreamGroupPartOfDefaultPresentation(sg, firstPresentationSubstreamGroupIndexes, firstPresentationNSubstreamGroups),
				frame_rate_factor,
				fsIndex,
				localChannelCount,
				speakerGroupIndexMask,
				bObjorAjoc);
			if (channelCount < localChannelCount) { channelCount = localChannelCount; }
		}

		if (bObjorAjoc == 0) {
			audioframe->channels = AC4::Ac4ChannelCountFromSpeakerGroupIndexMask(speakerGroupIndexMask);
		}
		else {
			audioframe->param1 = 1;
			audioframe->channels = channelCount;
		}

		static const fraction_t resampling_ratios[] = {
			{25025, 24000},
			{25, 24},
			{15, 16},
			{25025, 24000},
			{25, 24},
			{25025, 24000},
			{25, 24},
			{15, 16},
			{25025, 24000},
			{25, 24},
			{15, 16},
			{25025, 24000},
			{25, 24},
			{1, 1},
			{1, 1},
			{1, 1},
		};
		if (fsIndex) {
			audioframe->samplerate = RescaleU64x32(audioframe->samplerate, resampling_ratios[frameRateIndex].den, resampling_ratios[frameRateIndex].num);
		}
	}

	return frameSize;
}

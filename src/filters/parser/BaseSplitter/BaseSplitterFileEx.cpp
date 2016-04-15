/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2016 see Authors.txt
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
#include "BaseSplitterFileEx.h"
#include <MMReg.h>
#include "../../../DSUtil/AudioParser.h"
#include <InitGuid.h>
#include <moreuuids.h>
#include <basestruct.h>

//
// CBaseSplitterFileEx
//

CBaseSplitterFileEx::CBaseSplitterFileEx(IAsyncReader* pReader, HRESULT& hr, int fmode)
	: CBaseSplitterFile(pReader, hr, fmode)
{
}

CBaseSplitterFileEx::~CBaseSplitterFileEx()
{
}

//

bool CBaseSplitterFileEx::NextMpegStartCode(BYTE& code, __int64 len)
{
	BitByteAlign();
	DWORD dw = DWORD_MAX;
	do {
		if (len-- == 0 || !GetRemaining()) {
			return false;
		}
		dw = (dw << 8) | (BYTE)BitRead(8);
	} while ((dw & 0xffffff00) != 0x00000100);
	code = (BYTE)(dw & 0xff);
	return true;
}

//

bool CBaseSplitterFileEx::Read(seqhdr& h, int len, CMediaType* pmt, bool find_sync)
{
	size_t nSize = min(len, GetRemaining());
	CAtlArray<BYTE> pData;
	pData.SetCount(nSize);
	ByteRead(pData.GetData(), nSize);

	return Read(h, pData, pmt, find_sync);
}

#define MARKERGB if (gb.BitRead(1) != 1) {DEBUG_ASSERT(0);/* return false;*/}
bool CBaseSplitterFileEx::Read(seqhdr& h, CAtlArray<BYTE>& buf, CMediaType* pmt, bool find_sync)
{
	BYTE id = 0;

	CGolombBuffer gb(buf.GetData(), buf.GetCount());
	while (!gb.IsEOF() && id != 0xb3) {
		if (!gb.NextMpegStartCode(id)) {
			return false;
		}

		if (!find_sync) {
			break;
		}
	}

	if (id != 0xb3) {
		return false;
	}

	__int64 shpos = gb.GetPos() - 4;

	h.width = (WORD)gb.BitRead(12);
	h.height = (WORD)gb.BitRead(12);
	h.ar = gb.BitRead(4);
	static int ifps[16] = {0, 1126125, 1125000, 1080000, 900900, 900000, 540000, 450450, 450000, 0, 0, 0, 0, 0, 0, 0};
	h.ifps = ifps[gb.BitRead(4)];
	h.bitrate = (DWORD)gb.BitRead(18);
	MARKERGB;
	h.vbv = (DWORD)gb.BitRead(10);
	h.constrained = gb.BitRead(1);

	h.fiqm = gb.BitRead(1);
	if (h.fiqm)
		for (int i = 0; i < _countof(h.iqm); i++) {
			h.iqm[i] = (BYTE)gb.BitRead(8);
		}

	h.fniqm = gb.BitRead(1);
	if (h.fniqm)
		for (int i = 0; i < _countof(h.niqm); i++) {
			h.niqm[i] = (BYTE)gb.BitRead(8);
		}

	__int64 shlen = gb.GetPos() - shpos;

	static float ar[] = {
		1.0000f, 1.0000f, 0.6735f, 0.7031f, 0.7615f, 0.8055f, 0.8437f, 0.8935f,
		0.9157f, 0.9815f, 1.0255f, 1.0695f, 1.0950f, 1.1575f, 1.2015f, 1.0000f
	};

	h.arx = (int)((float)h.width / ar[h.ar] + 0.5);
	h.ary = h.height;

	mpeg_t type = mpeg1;

	__int64 shextpos = 0, shextlen = 0;

	if (gb.NextMpegStartCode(id) && id == 0xb5) { // sequence header ext
		shextpos = gb.GetPos() - 4;

		h.startcodeid = gb.BitRead(4);
		h.profile_levelescape = gb.BitRead(1); // reserved, should be 0
		h.profile = gb.BitRead(3);
		h.level = gb.BitRead(4);
		h.progressive = gb.BitRead(1);
		h.chroma = gb.BitRead(2);
		h.width |= (gb.BitRead(2) << 12);
		h.height |= (gb.BitRead(2) << 12);
		h.bitrate |= (gb.BitRead(12) << 18);
		MARKERGB;
		h.vbv |= (gb.BitRead(8) << 10);
		h.lowdelay = gb.BitRead(1);
		h.ifps = (DWORD)(h.ifps * (gb.BitRead(2) + 1) / (gb.BitRead(5) + 1));

		shextlen = gb.GetPos() - shextpos;

		struct {
			DWORD x, y;
		} ar[] = {{h.width, h.height}, {4, 3}, {16, 9}, {221, 100}, {h.width, h.height}};
		int i = min(max(h.ar, 1), 5) - 1;
		h.arx = ar[i].x;
		h.ary = ar[i].y;

		type = mpeg2;

		while (!gb.IsEOF()) {
			if (gb.NextMpegStartCode(id)) {
				if (id != 0xb5) {
					continue;
				}

				if (gb.RemainingSize() >= 5) {

					BYTE startcodeid = gb.BitRead(4);
					if (startcodeid == 0x02) {
						BYTE video_format = gb.BitRead(3);
						BYTE color_description = gb.BitRead(1);
						if (color_description) {
							BYTE color_primaries = gb.BitRead(8);
							BYTE color_trc = gb.BitRead(8);
							BYTE colorspace = gb.BitRead(8);
						}

						WORD panscan_width = (WORD)gb.BitRead(14);
						MARKERGB;
						WORD panscan_height = (WORD)gb.BitRead(14);

						if (panscan_width && panscan_height) {
							CSize aspect(h.width  * panscan_height, h.height * panscan_width);
							ReduceDim(aspect);

							h.arx *= aspect.cx;
							h.ary *= aspect.cy;
						}

						break;
					}
				}
			}
		}
	}

	h.ifps = 10 * h.ifps / 27;
	h.bitrate = h.bitrate == (1 << 30) - 1 ? 0 : h.bitrate * 400;

	CSize aspect(h.arx, h.ary);
	ReduceDim(aspect);
	h.arx = aspect.cx;
	h.ary = aspect.cy;

	if (pmt) {
		pmt->majortype = MEDIATYPE_Video;

		if (type == mpeg1) {
			pmt->subtype						= MEDIASUBTYPE_MPEG1Payload;
			pmt->formattype						= FORMAT_MPEGVideo;
			int len								= FIELD_OFFSET(MPEG1VIDEOINFO, bSequenceHeader) + int(shlen + shextlen);
			MPEG1VIDEOINFO* vi					= (MPEG1VIDEOINFO*)DNew BYTE[len];
			memset(vi, 0, len);
			vi->hdr.dwBitRate					= h.bitrate;
			vi->hdr.AvgTimePerFrame				= h.ifps;
			vi->hdr.bmiHeader.biSize			= sizeof(vi->hdr.bmiHeader);
			vi->hdr.bmiHeader.biWidth			= h.width;
			vi->hdr.bmiHeader.biHeight			= h.height;
			vi->hdr.bmiHeader.biPlanes			= 1;
			vi->hdr.bmiHeader.biBitCount		= 12;
			vi->hdr.bmiHeader.biSizeImage		= DIBSIZE(vi->hdr.bmiHeader);
			vi->hdr.bmiHeader.biXPelsPerMeter	= h.width * h.ary;
			vi->hdr.bmiHeader.biYPelsPerMeter	= h.height * h.arx;
			vi->cbSequenceHeader				= DWORD(shlen + shextlen);
			gb.Reset();
			gb.SkipBytes(shextpos);
			gb.ReadBuffer((BYTE*)&vi->bSequenceHeader[0], shlen);
			if (shextpos && shextlen) {
				gb.Reset();
				gb.SkipBytes(shextpos);
			}
			gb.ReadBuffer((BYTE*)&vi->bSequenceHeader[0] + shlen, shextlen);
			pmt->SetFormat((BYTE*)vi, len);
			pmt->SetTemporalCompression(TRUE);
			pmt->SetVariableSize();

			delete [] vi;
		} else if (type == mpeg2) {
			pmt->subtype					= MEDIASUBTYPE_MPEG2_VIDEO;
			pmt->formattype					= FORMAT_MPEG2_VIDEO;
			int len							= FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader) + int(shlen + shextlen);
			MPEG2VIDEOINFO* vi				= (MPEG2VIDEOINFO*)DNew BYTE[len];
			memset(vi, 0, len);
			vi->hdr.dwBitRate				= h.bitrate;
			vi->hdr.AvgTimePerFrame			= h.ifps;
			vi->hdr.dwPictAspectRatioX		= h.arx;
			vi->hdr.dwPictAspectRatioY		= h.ary;
			vi->hdr.bmiHeader.biSize		= sizeof(vi->hdr.bmiHeader);
			vi->hdr.bmiHeader.biWidth		= h.width;
			vi->hdr.bmiHeader.biHeight		= h.height;
			vi->hdr.bmiHeader.biPlanes		= 1;
			vi->hdr.bmiHeader.biBitCount	= 12;
			vi->hdr.bmiHeader.biSizeImage	= DIBSIZE(vi->hdr.bmiHeader);
			vi->dwProfile					= h.profile;
			vi->dwLevel						= h.level;
			vi->cbSequenceHeader			= DWORD(shlen + shextlen);
			gb.Reset();
			gb.SkipBytes(shpos);
			gb.ReadBuffer((BYTE*)&vi->dwSequenceHeader[0], shlen);
			if (shextpos && shextlen) {
				gb.Reset();
				gb.SkipBytes(shextpos);
			}
			gb.ReadBuffer((BYTE*)&vi->dwSequenceHeader[0] + shlen, shextlen);
			pmt->SetFormat((BYTE*)vi, len);
			pmt->SetTemporalCompression(TRUE);
			pmt->SetVariableSize();

			delete [] vi;
		}
	}

	return true;
}

#define AGAIN_OR_EXIT						\
	if (find_sync) {						\
		Seek(pos); BitRead(8); continue;	\
	} else {								\
		return false;						\
	}										\

bool CBaseSplitterFileEx::Read(mpahdr& h, int len, CMediaType* pmt/* = NULL*/, bool fAllowV25/* = false*/, bool find_sync/* = false*/)
{
	memset(&h, 0, sizeof(h));

	int syncbits = fAllowV25 ? 11 : 12;
	int bitrate = 0;

	for (;;) {
		if (!find_sync && (BitRead(syncbits, true) != (1 << syncbits) - 1)) {
			return false;
		}

		for (; len >= 4 && BitRead(syncbits, true) != (1 << syncbits) - 1; len--) {
			BitRead(8);
		}

		if (len < 4) {
			return false;
		}

		const __int64 pos = GetPos();

		h.sync = BitRead(11);
		h.version = BitRead(2);
		h.layer = BitRead(2);
		h.crc = BitRead(1);
		h.bitrate = BitRead(4);
		h.freq = BitRead(2);
		h.padding = BitRead(1);
		h.privatebit = BitRead(1);
		h.channels = BitRead(2);
		h.modeext = BitRead(2);
		h.copyright = BitRead(1);
		h.original = BitRead(1);
		h.emphasis = BitRead(2);

		if (h.version == 1 || h.layer == 0 || h.freq == 3 || h.bitrate == 15 || h.emphasis == 2) {
			AGAIN_OR_EXIT
		}

		if (h.version == 3 && h.layer == 2) {
			if (h.channels == 3) {
				if (h.bitrate >= 11 && h.bitrate <= 14) {
					AGAIN_OR_EXIT
				}
			} else {
				if (h.bitrate == 1 || h.bitrate == 2 || h.bitrate == 3 || h.bitrate == 5) {
					AGAIN_OR_EXIT
				}
			}
		}

		h.layer = 4 - h.layer;

		static int brtbl[][5] = {
			{  0,   0,   0,   0,   0},
			{ 32,  32,  32,  32,   8},
			{ 64,  48,  40,  48,  16},
			{ 96,  56,  48,  56,  24},
			{128,  64,  56,  64,  32},
			{160,  80,  64,  80,  40},
			{192,  96,  80,  96,  48},
			{224, 112,  96, 112,  56},
			{256, 128, 112, 128,  64},
			{288, 160, 128, 144,  80},
			{320, 192, 160, 160,  96},
			{352, 224, 192, 176, 112},
			{384, 256, 224, 192, 128},
			{416, 320, 256, 224, 144},
			{448, 384, 320, 256, 160},
			{  0,   0,   0,   0,   0},
		};

		static int brtblcol[][4] = {
			{0, 3, 4, 4},
			{0, 0, 1, 2}
		};
		bitrate = 1000 * brtbl[h.bitrate][brtblcol[h.version & 1][h.layer]];
		if (bitrate == 0) {
			AGAIN_OR_EXIT
		}

		break;
	}

	static int freq[][4] = {
		{11025, 0, 22050, 44100},
		{12000, 0, 24000, 48000},
		{ 8000, 0, 16000, 32000}
	};

	bool l3ext = h.layer == 3 && !(h.version&1);

	h.Samplerate = freq[h.freq][h.version];
	h.FrameSize = h.layer == 1
				  ? (12 * bitrate / h.Samplerate + h.padding) * 4
				  : (l3ext ? 72 : 144) * bitrate / h.Samplerate + h.padding;
	h.FrameSamples = h.layer == 1 ? 384 : l3ext ? 576 : 1152;
	h.rtDuration = 10000000i64 * h.FrameSamples / h.Samplerate;

	if (pmt) {
		size_t size = h.layer == 3
			? sizeof(WAVEFORMATEX/*MPEGLAYER3WAVEFORMAT*/) // no need to overcomplicate this...
			: sizeof(MPEG1WAVEFORMAT);
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)DNew BYTE[size];
		memset(wfe, 0, size);
		wfe->cbSize = WORD(size - sizeof(WAVEFORMATEX));

		if (h.layer == 3) {
			wfe->wFormatTag = WAVE_FORMAT_MPEGLAYER3;

			/*		MPEGLAYER3WAVEFORMAT* f = (MPEGLAYER3WAVEFORMAT*)wfe;
					f->wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
					f->wID = MPEGLAYER3_ID_UNKNOWN;
					f->fdwFlags = h.padding ? MPEGLAYER3_FLAG_PADDING_ON : MPEGLAYER3_FLAG_PADDING_OFF; // _OFF or _ISO ?
					*/
		} else {
			MPEG1WAVEFORMAT* f = (MPEG1WAVEFORMAT*)wfe;
			f->wfx.wFormatTag = WAVE_FORMAT_MPEG;
			f->fwHeadMode = 1 << h.channels;
			f->fwHeadModeExt = 1 << h.modeext;
			f->wHeadEmphasis = h.emphasis + 1;
			if (h.privatebit) {
				f->fwHeadFlags |= ACM_MPEG_PRIVATEBIT;
			}
			if (h.copyright) {
				f->fwHeadFlags |= ACM_MPEG_COPYRIGHT;
			}
			if (h.original) {
				f->fwHeadFlags |= ACM_MPEG_ORIGINALHOME;
			}
			if (h.crc == 0) {
				f->fwHeadFlags |= ACM_MPEG_PROTECTIONBIT;
			}
			if (h.version == 3) {
				f->fwHeadFlags |= ACM_MPEG_ID_MPEG1;
			}
			f->fwHeadLayer = 1 << (h.layer - 1);
			f->dwHeadBitrate = bitrate;
		}

		wfe->nChannels			= h.channels == 3 ? 1 : 2;
		wfe->nSamplesPerSec		= h.Samplerate;
		wfe->nBlockAlign		= h.FrameSize;
		wfe->nAvgBytesPerSec	= bitrate / 8;

		pmt->majortype			= MEDIATYPE_Audio;
		pmt->subtype			= FOURCCMap(wfe->wFormatTag);
		pmt->formattype			= FORMAT_WaveFormatEx;
		pmt->SetFormat((BYTE*)wfe, sizeof(WAVEFORMATEX) + wfe->cbSize);

		delete [] wfe;
	}

	return true;
}

bool CBaseSplitterFileEx::Read(latm_aachdr& h, int len, CMediaType* pmt)
{
	memset(&h, 0, sizeof(h));

	for (; len >= 7 && BitRead(11, true) != AAC_LATM_SYNCWORD; len--) {
		BitRead(8);
	}

	if (len < 7) {
		return false;
	}

	BYTE buffer[64] = { 0 };
	ByteRead(buffer, min(len, 64));

	BYTE extra[64] = { 0 };
	unsigned int extralen = 0;
	if (!ParseAACLatmHeader(buffer, min(len, 64), h.samplerate, h.channels, extra, extralen)) {
		return false;
	}

	if (pmt) {
		WAVEFORMATEX* wfe		= (WAVEFORMATEX*)DNew BYTE[sizeof(WAVEFORMATEX) + extralen];
		memset(wfe, 0, sizeof(WAVEFORMATEX));
		wfe->wFormatTag			= WAVE_FORMAT_LATM_AAC;
		wfe->nChannels			= h.channels;
		wfe->nSamplesPerSec		= h.samplerate;
		wfe->nBlockAlign		= 1;
		wfe->nAvgBytesPerSec	= 0;
		wfe->cbSize				= extralen;
		if (extralen) {
			memcpy((BYTE*)(wfe + 1), extra, extralen);
		}

		pmt->majortype			= MEDIATYPE_Audio;
		pmt->subtype			= MEDIASUBTYPE_LATM_AAC;
		pmt->formattype			= FORMAT_WaveFormatEx;
		pmt->SetFormat((BYTE*)wfe, sizeof(WAVEFORMATEX) + wfe->cbSize);

		delete [] wfe;
	}

	return true;
}

bool CBaseSplitterFileEx::Read(aachdr& h, int len, CMediaType* pmt, bool find_sync)
{
	memset(&h, 0, sizeof(h));

	for (;;) {
		if (!find_sync && (BitRead(12, true) != 0xfff)) {
			return false;
		}

		for (; len >= 7 && BitRead(12, true) != 0xfff; len--) {
			BitRead(8);
		}

		if (len < 7) {
			return false;
		}

		const __int64 pos = GetPos();

		h.sync = BitRead(12);
		h.version = BitRead(1);
		h.layer = BitRead(2);
		h.fcrc = BitRead(1);
		h.profile = BitRead(2);
		h.freq = BitRead(4);
		h.privatebit = BitRead(1);
		h.channels = BitRead(3);
		h.original = BitRead(1);
		h.home = BitRead(1);

		h.copyright_id_bit = BitRead(1);
		h.copyright_id_start = BitRead(1);
		h.aac_frame_length = BitRead(13);
		h.adts_buffer_fullness = BitRead(11);
		h.no_raw_data_blocks_in_frame = BitRead(2);

		if (h.fcrc == 0) {
			h.crc = (WORD)BitRead(16);
		}

		if (h.layer != 0 || h.freq > 12 || h.aac_frame_length <= (h.fcrc == 0 ? 9 : 7) || h.channels < 1) {
			AGAIN_OR_EXIT
		}

		break;
	}

	static int freq[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350};
	h.Samplerate = freq[h.freq];
	h.FrameSize = h.aac_frame_length - (h.fcrc == 0 ? 9 : 7);
	h.FrameSamples = 1024; // ok?
	h.rtDuration = 10000000i64 * h.FrameSamples / h.Samplerate;

	if (pmt) {
		WAVEFORMATEX* wfe		= (WAVEFORMATEX*)DNew BYTE[sizeof(WAVEFORMATEX) + 5];
		memset(wfe, 0, sizeof(WAVEFORMATEX) + 5);
		wfe->wFormatTag			= WAVE_FORMAT_RAW_AAC1;
		wfe->nChannels			= h.channels <= 6 ? h.channels : 2;
		wfe->nSamplesPerSec		= freq[h.freq];
		wfe->nBlockAlign		= h.aac_frame_length;
		wfe->nAvgBytesPerSec	= h.aac_frame_length * h.Samplerate / h.FrameSamples;
		wfe->cbSize				= MakeAACInitData((BYTE*)(wfe + 1), h.profile, wfe->nSamplesPerSec, wfe->nChannels);

		pmt->majortype			= MEDIATYPE_Audio;
		pmt->subtype			= MEDIASUBTYPE_RAW_AAC1;
		pmt->formattype			= FORMAT_WaveFormatEx;
		pmt->SetFormat((BYTE*)wfe, sizeof(WAVEFORMATEX) + wfe->cbSize);

		delete [] wfe;
	}

	return true;
}

bool CBaseSplitterFileEx::Read(ac3hdr& h, int len, CMediaType* pmt, bool find_sync, bool AC3CoreOnly)
{
	static int freq[] = {48000, 44100, 32000, 0};

	bool e_ac3 = false;

	__int64 startpos = GetPos();

	memset(&h, 0, sizeof(h));

	// Parse TrueHD and MLP header
	if (!AC3CoreOnly) {
		BYTE buf[20];

		int fsize = 0;
		ByteRead(buf, 20);

		audioframe_t aframe;
		fsize = ParseMLPHeader(buf, &aframe);
		if (fsize) {

			if (pmt) {
				int bitrate = (int)(fsize * 8i64 * aframe.samplerate / aframe.samples); // inaccurate, because fsize is not constant

				pmt->majortype			= MEDIATYPE_Audio;
				pmt->subtype			= aframe.param2 ? MEDIASUBTYPE_DOLBY_TRUEHD : MEDIASUBTYPE_MLP;
				pmt->formattype			= FORMAT_WaveFormatEx;

				WAVEFORMATEX* wfe		= (WAVEFORMATEX*)pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX));
				memset(wfe, 0, sizeof(WAVEFORMATEX));
				wfe->wFormatTag			= WAVE_FORMAT_UNKNOWN;
				wfe->nChannels			= aframe.channels;
				wfe->nSamplesPerSec		= aframe.samplerate;
				wfe->nAvgBytesPerSec	= (bitrate + 4) / 8;
				wfe->nBlockAlign		= fsize < WORD_MAX ? fsize : WORD_MAX;
				wfe->wBitsPerSample		= aframe.param1;

				pmt->SetSampleSize(0);
			}
			return true;
		}
	}

	Seek(startpos);

	if (find_sync) {
		for (; len >= 7 && BitRead(16, true) != 0x0b77; len--) {
			BitRead(8);
		}
	}

	if (len < 7) {
		return false;
	}

	h.sync = (WORD)BitRead(16);
	if (h.sync != 0x0B77) {
		return false;
	}

	_int64 pos = GetPos();
	h.crc1 = (WORD)BitRead(16);
	h.fscod = BitRead(2);
	h.frmsizecod = BitRead(6);
	h.bsid = BitRead(5);
	if (h.bsid > 16) {
		return false;
	}
	if (h.bsid <= 10) {
		/* Normal AC-3 */
		if (h.fscod == 3) {
			return false;
		}
		if (h.frmsizecod > 37) {
			return false;
		}
		h.bsmod = BitRead(3);
		h.acmod = BitRead(3);
		if (h.acmod == 2) {
			h.dsurmod = BitRead(2);
		}
		if ((h.acmod & 1) && h.acmod != 1) {
			h.cmixlev = BitRead(2);
		}
		if (h.acmod & 4) {
			h.surmixlev = BitRead(2);
		}
		h.lfeon = BitRead(1);
		h.sr_shift = max(h.bsid, 8) - 8;
	} else {
		/* Enhanced AC-3 */
		e_ac3 = true;
		Seek(pos);
		h.frame_type = (BYTE)BitRead(2);
		h.substreamid = (BYTE)BitRead(3);
		if (h.frame_type || h.substreamid) {
			return false;
		}
		h.frame_size = ((WORD)BitRead(11) + 1) << 1;
		if (h.frame_size < 7) {
			return false;
		}
		h.sr_code = (BYTE)BitRead(2);
		if (h.sr_code == 3) {
			BYTE sr_code2 = (BYTE)BitRead(2);
			if (sr_code2 == 3) {
				return false;
			}
			h.sample_rate = freq[sr_code2] / 2;
			h.sr_shift = 1;
		} else {
			static int eac3_blocks[4] = {1, 2, 3, 6};
			h.num_blocks = eac3_blocks[BitRead(2)];
			h.sample_rate = freq[h.sr_code];
			h.sr_shift = 0;
		}
		h.acmod = BitRead(3);
		h.lfeon = BitRead(1);
	}

	if (pmt) {
		WAVEFORMATEX wfe;
		memset(&wfe, 0, sizeof(wfe));
		wfe.wFormatTag = WAVE_FORMAT_DOLBY_AC3;

		static int channels[] = { 2, 1, 2, 3, 3, 4, 4, 5 };
		wfe.nChannels = channels[h.acmod] + h.lfeon;

		if (e_ac3) {
			wfe.nSamplesPerSec = h.sample_rate;
			wfe.nAvgBytesPerSec = h.frame_size * h.sample_rate / (h.num_blocks * 256);
		} else {
			wfe.nSamplesPerSec = freq[h.fscod] >> h.sr_shift;
			static int rate[] = { 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 576, 640, 768, 896, 1024, 1152, 1280 };
			wfe.nAvgBytesPerSec = ((rate[h.frmsizecod >> 1] * 1000) >> h.sr_shift) / 8;
		}

		wfe.nBlockAlign = (WORD)(1536 * wfe.nAvgBytesPerSec / wfe.nSamplesPerSec);

		pmt->majortype = MEDIATYPE_Audio;
		if (e_ac3) {
			pmt->subtype = MEDIASUBTYPE_DOLBY_DDPLUS;
		} else {
			pmt->subtype = MEDIASUBTYPE_DOLBY_AC3;
		}
		pmt->formattype = FORMAT_WaveFormatEx;
		pmt->SetFormat((BYTE*)&wfe, sizeof(wfe));
	}

	return true;
}

bool CBaseSplitterFileEx::Read(dtshdr& h, int len, CMediaType* pmt, bool find_sync)
{
	memset(&h, 0, sizeof(h));

	if (find_sync) {
		for (; len >= 10 && BitRead(32, true) != 0x7ffe8001; len--) {
			BitRead(8);
		}
	}

	if (len < 10) {
		return false;
	}

	h.sync = (DWORD)BitRead(32);
	if (h.sync != 0x7ffe8001) {
		return false;
	}

	h.frametype = BitRead(1);
	h.deficitsamplecount = BitRead(5);
	h.fcrc = BitRead(1);
	h.nblocks = BitRead(7)+1;
	h.framebytes = (WORD)BitRead(14)+1;
	h.amode = BitRead(6);
	h.sfreq = BitRead(4);
	h.rate = BitRead(5);

	h.downmix = BitRead(1);
	h.dynrange = BitRead(1);
	h.timestamp = BitRead(1);
	h.aux_data = BitRead(1);
	h.hdcd = BitRead(1);
	h.ext_descr = BitRead(3);
	h.ext_coding = BitRead(1);
	h.aspf = BitRead(1);
	h.lfe = BitRead(2);
	h.predictor_history = BitRead(1);

	if (h.fcrc) {
		BitRead(16);
	}
	BitRead(1); // Multirate Interpolator
	BitRead(4); // Encoder Software Revision
	BitRead(2); // Copy History
	h.bits_per_sample = BitRead(2); // Source PCM Resolution

	if (pmt) {
		WAVEFORMATEX wfe;
		memset(&wfe, 0, sizeof(wfe));
		wfe.wFormatTag = WAVE_FORMAT_DTS2;

		static WORD channels[] = { 1, 2, 2, 2, 2, 3, 3, 4, 4, 5, 6, 6, 6, 7, 8, 8 };
		if (h.amode < _countof(channels)) {
			wfe.nChannels = channels[h.amode];
			if (h.lfe > 0) {
				++wfe.nChannels;
			}
		}

		static DWORD freq[] = { 0, 8000, 16000, 32000, 0, 0, 11025, 22050, 44100, 0, 0, 12000, 24000, 48000, 0, 0 };
		if (h.sfreq < _countof(freq)) {
			wfe.nSamplesPerSec = freq[h.sfreq];
		}

		BOOL x96k = (h.ext_descr == 2 && h.ext_coding == 1);
		if (x96k) {
			// x96k extension present
			wfe.nSamplesPerSec <<= 1;
		}

		/*static int rate[] = {
			  32000,   56000,   64000,   96000,
			  112000,  128000,  192000,  224000,
			  256000,  320000,  384000,  448000,
			  512000,  576000,  640000,  768000,
			  960000, 1024000, 1152000, 1280000,
			  1344000, 1408000, 1411200, 1472000,
			  1536000, 1920000, 2048000, 3072000,
			  3840000, 0, 0, 0 //open, variable, lossless
		};
		int nom_bitrate = rate[h.rate];*/

		unsigned int bitrate = (unsigned int)(8ui64 * h.framebytes * wfe.nSamplesPerSec / (h.nblocks * 32)) >> x96k;

		wfe.nAvgBytesPerSec	= (bitrate + 4) / 8;
		wfe.nBlockAlign		= h.framebytes;

		static const WORD bits_per_sample[] = { 16, 20, 24, 24 };
		if (h.bits_per_sample < _countof(bits_per_sample)) {
			wfe.wBitsPerSample	= bits_per_sample[h.bits_per_sample];
		}

		if (!wfe.nChannels || !wfe.nSamplesPerSec) {
			return false;
		}

		pmt->majortype		= MEDIATYPE_Audio;
		pmt->subtype		= MEDIASUBTYPE_DTS;
		pmt->formattype		= FORMAT_WaveFormatEx;
		pmt->SetFormat((BYTE*)&wfe, sizeof(wfe));
	}

	return true;
}

bool CBaseSplitterFileEx::Read(lpcmhdr& h, CMediaType* pmt)
{
	memset(&h, 0, sizeof(h));

	h.emphasis = BitRead(1);
	h.mute = BitRead(1);
	h.reserved1 = BitRead(1);
	h.framenum = BitRead(5);
	h.quantwordlen = BitRead(2);
	h.freq = BitRead(2);
	h.reserved2 = BitRead(1);
	h.channels = BitRead(3);
	h.drc = (BYTE)BitRead(8);

	if (h.quantwordlen == 3 || h.reserved1 || h.reserved2) {
		return false;
	}

	if (pmt) {
		WAVEFORMATEX wfe;
		memset(&wfe, 0, sizeof(wfe));
		wfe.wFormatTag = WAVE_FORMAT_PCM;
		wfe.nChannels = h.channels + 1;
		static int freq[] = { 48000, 96000, 44100, 32000 };
		wfe.nSamplesPerSec = freq[h.freq];
		switch (h.quantwordlen) {
			case 0:
				wfe.wBitsPerSample = 16;
				break;
			case 1:
				wfe.wBitsPerSample = 20;
				break;
			case 2:
				wfe.wBitsPerSample = 24;
				break;
		}
		wfe.nBlockAlign		= (wfe.nChannels * 2 * wfe.wBitsPerSample) / 8;
		wfe.nAvgBytesPerSec	= (wfe.nBlockAlign*wfe.nSamplesPerSec) / 2;

		pmt->majortype		= MEDIATYPE_Audio;
		pmt->subtype		= MEDIASUBTYPE_DVD_LPCM_AUDIO;
		pmt->formattype		= FORMAT_WaveFormatEx;
		pmt->SetFormat((BYTE*)&wfe, sizeof(wfe));
	}

	return true;
}

bool CBaseSplitterFileEx::Read(dvdalpcmhdr& h, int len, CMediaType* pmt)
{
	memset(&h, 0, sizeof(h));
	if (len < 8) return false;

	h.firstaudioframe = (WORD)BitRead(16);// Byte pointer to start of first audio frame.
	h.unknown1        = (BYTE)BitRead(8); // Unknown - e.g. 0x10 for stereo, 0x00 for surround
	if (h.unknown1 != 0x10 && h.unknown1 != 0x00)
		return false; // this is not the aob. maybe this is a vob.

	h.bitpersample1   = (BYTE)BitRead(4);
	h.bitpersample2   = (BYTE)BitRead(4);
	h.samplerate1     = (BYTE)BitRead(4);
	h.samplerate2     = (BYTE)BitRead(4);
	h.unknown2        = (BYTE)BitRead(8); // Unknown - e.g. 0x00
	h.groupassignment = (BYTE)BitRead(8); // Channel Group Assignment
	h.unknown3        = (BYTE)BitRead(8); // Unknown - e.g. 0x80

	if (h.bitpersample1 > 2 || (h.samplerate1 & 7) > 2 || h.groupassignment > 20 ||
			h.unknown2 != 0x00 || h.unknown3 != 0x80) {
		return false; // poor parameters or this is a vob.
	}
	// decoder limitations
	if (h.groupassignment > 1 &&
			(h.bitpersample2 != h.bitpersample1 || h.samplerate2 != h.samplerate1)) {
		return false; // decoder does not support different bit depth and frequency.
	}

	if (pmt) {
		WAVEFORMATEX wfe;
		memset(&wfe, 0, sizeof(wfe));
		wfe.wFormatTag = WAVE_FORMAT_UNKNOWN;
		static const WORD depth[] = { 16, 20, 24 };
		static const DWORD freq[] = { 48000, 96000, 192000, 0, 0, 0, 0, 0, 44100, 88200, 1764000 };
		// 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20
		static const WORD channels1[] = { 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4 };
		static const WORD channels2[] = { 0, 0, 1, 2, 1, 2, 3, 1, 2, 3, 2, 3, 4, 1, 2, 1, 2, 3, 1, 1, 2 };
		wfe.wBitsPerSample = depth[h.bitpersample1];
		wfe.nSamplesPerSec = freq[h.samplerate1];
		wfe.nChannels = channels1[h.groupassignment] + channels2[h.groupassignment];

		if (wfe.nChannels > 2) {
			wfe.nBlockAlign = (depth[h.bitpersample1] * channels1[h.groupassignment] * (WORD)(freq[h.samplerate1] / freq[h.samplerate2]) +
				depth[h.bitpersample2] * channels2[h.groupassignment]) * 2 / 8;
		} else {
			wfe.nBlockAlign = depth[h.bitpersample1] * channels1[h.groupassignment] * 2 / 8;
		}
		wfe.nAvgBytesPerSec = wfe.nBlockAlign * wfe.nSamplesPerSec / 2;

		pmt->majortype	= MEDIATYPE_Audio;
		pmt->subtype	= MEDIASUBTYPE_DVD_LPCM_AUDIO;
		pmt->formattype	= FORMAT_WaveFormatEx;
		pmt->SetFormat((BYTE*)&wfe, sizeof(wfe));
	}

	return true;
}

bool CBaseSplitterFileEx::Read(hdmvlpcmhdr& h, CMediaType* pmt)
{
	memset(&h, 0, sizeof(h));

	h.size			= (WORD)BitRead(16);
	h.channels		= (BYTE)BitRead(4);
	h.samplerate	= (BYTE)BitRead(4);
	h.bitpersample	= (BYTE)BitRead(2);

	if (h.channels > 11
			|| h.samplerate > 5
			|| h.bitpersample > 3) {
		return false;
	}

	if (pmt) {
		WAVEFORMATEX_HDMV_LPCM wfe;
		wfe.wFormatTag = WAVE_FORMAT_PCM;

		static int channels[] = { 0, 1, 0, 2, 3, 3, 4, 4, 5, 6, 7, 8 };
		wfe.nChannels = channels[h.channels];
		if (wfe.nChannels == 0) {
			return false;
		}
		wfe.channel_conf = h.channels;

		static int freq[] = { 0, 48000, 0, 0, 96000, 192000 };
		wfe.nSamplesPerSec	= freq[h.samplerate];
		if (wfe.nSamplesPerSec == 0) {
			return false;
		}

		static int bitspersample[] = { 0, 16, 20, 24 };
		wfe.wBitsPerSample	= bitspersample[h.bitpersample];
		if (wfe.wBitsPerSample == 0) {
			return false;
		}

		wfe.nBlockAlign		= wfe.nChannels*wfe.wBitsPerSample >> 3;
		wfe.nAvgBytesPerSec	= wfe.nBlockAlign*wfe.nSamplesPerSec;

		pmt->majortype		= MEDIATYPE_Audio;
		pmt->subtype		= MEDIASUBTYPE_HDMV_LPCM_AUDIO;
		pmt->formattype		= FORMAT_WaveFormatEx;
		pmt->SetFormat((BYTE*)&wfe, sizeof(wfe));
	}

	return true;
}

bool CBaseSplitterFileEx::Read(mlphdr& h, int len, CMediaType* pmt, bool find_sync)
{
	memset(&h, 0, sizeof(h));
	if (len < 20) return false;

	__int64 startpos = GetPos();

	audioframe_t aframe;
	int fsize = 0;

	BYTE buf[20];
	int k = find_sync ? len - 20 : 1;
	int i = 0;
	while (i < k) {
		Seek(startpos+i);
		ByteRead(buf, 20);
		fsize = ParseMLPHeader(buf, &aframe);
		if (fsize) {
			break;
		}
		++i;
	}

	if (fsize && aframe.param2 == 0) {
		h.size = fsize;

		if (pmt) {
			int bitrate = (int)(fsize * 8i64 * aframe.samplerate / aframe.samples); // inaccurate, because fsize is not constant
			pmt->majortype			= MEDIATYPE_Audio;
			pmt->subtype			= MEDIASUBTYPE_MLP;
			pmt->formattype			= FORMAT_WaveFormatEx;

			WAVEFORMATEX* wfe		= (WAVEFORMATEX*)pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX));
			memset(wfe, 0, sizeof(WAVEFORMATEX));
			wfe->wFormatTag			= WAVE_FORMAT_UNKNOWN;
			wfe->nChannels			= aframe.channels;
			wfe->nSamplesPerSec		= aframe.samplerate;
			wfe->nAvgBytesPerSec	= (bitrate + 4) / 8;
			wfe->nBlockAlign		= fsize < WORD_MAX ? fsize : WORD_MAX;
			wfe->wBitsPerSample		= aframe.param1;

			pmt->SetSampleSize(0);

			Seek(startpos + i);
		}
		return true;
	}

	return false;
}

bool CBaseSplitterFileEx::Read(dvdspuhdr& h, CMediaType* pmt)
{
	if (pmt) {
		pmt->majortype	= MEDIATYPE_Video;
		pmt->subtype	= MEDIASUBTYPE_DVD_SUBPICTURE;
		pmt->formattype	= FORMAT_None;
	}

	return true;
}

bool CBaseSplitterFileEx::Read(hdmvsubhdr& h, CMediaType* pmt, const char* language_code)
{
	if (pmt) {
		pmt->majortype	= MEDIATYPE_Subtitle;
		pmt->subtype	= MEDIASUBTYPE_HDMVSUB;
		pmt->formattype	= FORMAT_None;

		SUBTITLEINFO* psi = (SUBTITLEINFO*)pmt->AllocFormatBuffer(sizeof(SUBTITLEINFO));
		if (psi) {
			memset(psi, 0, pmt->FormatLength());
			strcpy_s(psi->IsoLang, language_code ? language_code : "eng");
		}
	}

	return true;
}

bool CBaseSplitterFileEx::Read(svcdspuhdr& h, CMediaType* pmt)
{
	if (pmt) {
		pmt->majortype	= MEDIATYPE_Video;
		pmt->subtype	= MEDIASUBTYPE_SVCD_SUBPICTURE;
		pmt->formattype	= FORMAT_None;
	}

	return true;
}

bool CBaseSplitterFileEx::Read(cvdspuhdr& h, CMediaType* pmt)
{
	if (pmt) {
		pmt->majortype	= MEDIATYPE_Video;
		pmt->subtype	= MEDIASUBTYPE_CVD_SUBPICTURE;
		pmt->formattype	= FORMAT_None;
	}

	return true;
}

bool CBaseSplitterFileEx::Read(ps2audhdr& h, CMediaType* pmt)
{
	memset(&h, 0, sizeof(h));

	if (BitRead(16, true) != 'SS') {
		return false;
	}

	__int64 pos = GetPos();

	while (BitRead(16, true) == 'SS') {
		DWORD tag = (DWORD)BitRead(32, true);
		DWORD size = 0;

		if (tag == 'SShd') {
			BitRead(32);
			ByteRead((BYTE*)&size, sizeof(size));
			ASSERT(size == 0x18);
			Seek(GetPos());
			ByteRead((BYTE*)&h, sizeof(h));
		} else if (tag == 'SSbd') {
			BitRead(32);
			ByteRead((BYTE*)&size, sizeof(size));
			break;
		}
	}

	Seek(pos);

	if (pmt) {
		WAVEFORMATEXPS2 wfe;
		wfe.wFormatTag =
			h.unk1 == 0x01 ? WAVE_FORMAT_PS2_PCM :
			h.unk1 == 0x10 ? WAVE_FORMAT_PS2_ADPCM :
			WAVE_FORMAT_UNKNOWN;
		wfe.nChannels		= (WORD)h.channels;
		wfe.nSamplesPerSec	= h.freq;
		wfe.wBitsPerSample	= 16; // always?
		wfe.nBlockAlign		= wfe.nChannels*wfe.wBitsPerSample >> 3;
		wfe.nAvgBytesPerSec	= wfe.nBlockAlign*wfe.nSamplesPerSec;
		wfe.dwInterleave	= h.interleave;

		pmt->majortype		= MEDIATYPE_Audio;
		pmt->subtype		= FOURCCMap(wfe.wFormatTag);
		pmt->formattype		= FORMAT_WaveFormatEx;
		pmt->SetFormat((BYTE*)&wfe, sizeof(wfe));
	}

	return true;
}

bool CBaseSplitterFileEx::Read(ps2subhdr& h, CMediaType* pmt)
{
	if (pmt) {
		pmt->majortype	= MEDIATYPE_Subtitle;
		pmt->subtype	= MEDIASUBTYPE_PS2_SUB;
		pmt->formattype	= FORMAT_None;
	}

	return true;
}

bool CBaseSplitterFileEx::Read(vc1hdr& h, int len, CMediaType* pmt)
{
	memset(&h, 0, sizeof(h));

	__int64 endpos = GetPos() + len; // - sequence header length
	__int64 extrapos = 0, extralen = 0;
	int		nFrameRateNum = 0, nFrameRateDen = 1;

	if (GetPos() < endpos + 4 && BitRead(32, true) == 0x0000010F) {
		extrapos = GetPos();

		BitRead(32);

		h.profile			= (BYTE)BitRead(2);

		// Check if advanced profile
		if (h.profile != 3) {
			return false;
		}

		h.level				= (BYTE)BitRead(3);
		h.chromaformat		= (BYTE)BitRead(2);

		// (fps-2)/4 (->30)
		h.frmrtq_postproc	= (BYTE)BitRead(3); //common
		// (bitrate-32kbps)/64kbps
		h.bitrtq_postproc	= (BYTE)BitRead(5); //common
		h.postprocflag		= (BYTE)BitRead(1); //common

		h.width				= ((unsigned int)BitRead(12) + 1) << 1;
		h.height			= ((unsigned int)BitRead(12) + 1) << 1;

		h.broadcast			= (BYTE)BitRead(1);
		h.interlace			= (BYTE)BitRead(1);
		h.tfcntrflag		= (BYTE)BitRead(1);
		h.finterpflag		= (BYTE)BitRead(1);
		BitRead(1); // reserved
		h.psf				= (BYTE)BitRead(1);
		if (BitRead(1)) {
			int ar = 0;
			BitRead(14);
			BitRead(14);
			if (BitRead(1)) {
				ar = (int)BitRead(4);
			}
			if (ar && ar < 14) {
				h.sar.num = pixel_aspect[ar][0];
				h.sar.den = pixel_aspect[ar][1];
			} else if (ar == 15) {
				h.sar.num = (BYTE)BitRead(8);
				h.sar.den = (BYTE)BitRead(8);
			}

			// Read framerate
			const int ff_vc1_fps_nr[5] = { 24, 25, 30, 50, 60 };
			const int ff_vc1_fps_dr[2] = { 1000, 1001 };

			if (BitRead(1)) {
				if (BitRead(1)) {
					nFrameRateNum = 32;
					nFrameRateDen = (int)BitRead(16) + 1;
				} else {
					int nr, dr;
					nr = (int)BitRead(8);
					dr = (int)BitRead(4);
					if (nr && nr < 8 && dr && dr < 3) {
						nFrameRateNum = ff_vc1_fps_dr[dr - 1];
						nFrameRateDen = ff_vc1_fps_nr[nr - 1] * 1000;
					}
				}
			}

		}

		Seek(extrapos + 4);
		extralen	= 0;
		int parse	= 0; // really need a signed type? may be unsigned will be better

		while (GetPos() < endpos + 4 && ((parse == 0x0000010E) || (parse & 0xFFFFFF00) != 0x00000100)) {
			parse = (parse <<8 ) | (int)BitRead(8);
			extralen++;
		}
	}

	if (!extralen) {
		return false;
	}

	if (pmt) {
		if (!h.sar.num) {
			h.sar.num = 1;
		}
		if (!h.sar.den) {
			h.sar.den = 1;
		}
		CSize aspect = CSize(h.width * h.sar.num, h.height * h.sar.den);
		if (h.width == h.sar.num && h.height == h.sar.den) {
			aspect = CSize(h.width, h.height);
		}
		ReduceDim(aspect);

		pmt->majortype				= MEDIATYPE_Video;
		pmt->subtype				= FOURCCMap(FCC('WVC1'));
		pmt->formattype				= FORMAT_VIDEOINFO2;

		int vi_len					= sizeof(VIDEOINFOHEADER2) + (int)extralen + 1;
		VIDEOINFOHEADER2* vi		= (VIDEOINFOHEADER2*)DNew BYTE[vi_len];
		memset(vi, 0, vi_len);

		vi->AvgTimePerFrame			= 10000000I64 * nFrameRateNum / nFrameRateDen;
		vi->dwPictAspectRatioX		= aspect.cx;
		vi->dwPictAspectRatioY		= aspect.cy;
		vi->bmiHeader.biSize		= sizeof(vi->bmiHeader);
		vi->bmiHeader.biWidth		= h.width;
		vi->bmiHeader.biHeight		= h.height;
		vi->bmiHeader.biCompression	= pmt->subtype.Data1;
		vi->bmiHeader.biPlanes		= 1;
		vi->bmiHeader.biBitCount	= 24;
		vi->bmiHeader.biSizeImage	= DIBSIZE(vi->bmiHeader);


		BYTE* p = (BYTE*)vi + sizeof(VIDEOINFOHEADER2);
		*p++ = 0;
		Seek(extrapos);
		ByteRead(p, extralen);
		pmt->SetFormat((BYTE*)vi, vi_len);
		pmt->SetTemporalCompression(TRUE);
		pmt->SetVariableSize();

		delete [] vi;
	}

	return true;
}

#define pc_seq_header 0x00
bool CBaseSplitterFileEx::Read(dirachdr& h, int len, CMediaType* pmt)
{
	if (len <= 13) {
		return false;
	}

	if (BitRead(32) == 'BBCD' && BitRead(8) == pc_seq_header) {
		len -= 5;
		DWORD unit_size = (DWORD)BitRead(32, true) - 5;
		if ((int)unit_size <= len) {
			Skip(8);
			unit_size -= 8;
			CAtlArray<BYTE> pData;
			pData.SetCount(unit_size);
			if (S_OK == ByteRead(pData.GetData(), unit_size)) {
				vc_params_t params;
				if (!ParseDiracHeader(pData.GetData(), unit_size, params)) {
					return false;
				}

				if (pmt) {
					pmt->majortype					= MEDIATYPE_Video;
					pmt->formattype					= FORMAT_VideoInfo;
					pmt->subtype					= FOURCCMap(FCC('drac'));

					VIDEOINFOHEADER* pvih			= (VIDEOINFOHEADER*)pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
					memset(pmt->Format(), 0, pmt->FormatLength());

					pvih->AvgTimePerFrame			= params.AvgTimePerFrame;
					pvih->bmiHeader.biSize			= sizeof(pvih->bmiHeader);
					pvih->bmiHeader.biWidth			= params.width;
					pvih->bmiHeader.biHeight		= params.height;
					pvih->bmiHeader.biPlanes		= 1;
					pvih->bmiHeader.biBitCount		= 12;
					pvih->bmiHeader.biCompression	= pmt->subtype.Data1;
					pvih->bmiHeader.biSizeImage		= DIBSIZE(pvih->bmiHeader);

					pmt->SetTemporalCompression(TRUE);
					pmt->SetVariableSize();
				}

				return true;
			}
		}
	}

	return false;
}

bool CBaseSplitterFileEx::Read(dvbsub& h, int len, CMediaType* pmt, bool bSimpleAdd)
{
	if (bSimpleAdd || (len > 4 && (BitRead(32, true) & 0xFFFFFF00) == 0x20000f00)) {
		if (pmt) {
			static const SUBTITLEINFO SubFormat = { 0, "", L"" };

			pmt->majortype	= MEDIATYPE_Subtitle;
			pmt->subtype	= MEDIASUBTYPE_DVB_SUBTITLES;
			pmt->formattype	= FORMAT_None;
			pmt->SetFormat((BYTE*)&SubFormat, sizeof(SUBTITLEINFO));
		}

		return true;
	}

	return false;
}

bool CBaseSplitterFileEx::Read(teletextsub& h, int len, CMediaType* pmt, bool bSimpleAdd)
{
	DWORD sync = 0;
	if (!bSimpleAdd && len > 4) {
		sync = BitRead(32, true) & 0xFFFFFF00;
	}
	if (bSimpleAdd
			|| sync == 0x10022C00 || sync == 0x10032C00) {
		if (pmt) {
			static const SUBTITLEINFO SubFormat = { 0, "", L"" };

			pmt->majortype	= MEDIATYPE_Subtitle;
			pmt->subtype	= MEDIASUBTYPE_UTF8;
			pmt->formattype	= FORMAT_None;
			pmt->SetFormat((BYTE*)&SubFormat, sizeof(SUBTITLEINFO));
		}

		return true;
	}

	return false;
}

static bool ParseAvc(CAtlArray<BYTE>& pData, CMediaType* pmt)
{
	NALU_TYPE nalu_type = NALU_TYPE_UNKNOWN;
	CH264Nalu Nalu;

	{
		int sps_present = 0;
		int pps_present = 0;

		Nalu.SetBuffer(pData.GetData(), pData.GetCount());
		Nalu.ReadNext();
		while (!(sps_present && pps_present)) {
			NALU_TYPE nalu_type = Nalu.GetType();
			if (!Nalu.ReadNext()) {
				break;
			}
			switch (nalu_type) {
				case NALU_TYPE_SPS:
				case NALU_TYPE_SUBSET_SPS:
					sps_present++;
					break;
				case NALU_TYPE_PPS:
					pps_present++;
					break;
			}
		}

		if (!(sps_present && pps_present)) {
			return false;
		}
	}

	Nalu.SetBuffer(pData.GetData(), pData.GetCount());
	nalu_type = NALU_TYPE_UNKNOWN;
	while (!(nalu_type == NALU_TYPE_SPS || nalu_type == NALU_TYPE_SUBSET_SPS)
			&& Nalu.ReadNext()) {
		nalu_type = Nalu.GetType();
	}

	vc_params_t params = { 0 };
	if (AVCParser::ParseSequenceParameterSet(Nalu.GetDataBuffer() + 1, Nalu.GetDataLength() - 1, params)) {
		if (pmt) {
			BITMAPINFOHEADER bmi = { 0 };
			bmi.biSize			= sizeof(bmi);
			bmi.biWidth			= params.width;
			bmi.biHeight		= params.height;
			bmi.biCompression	= nalu_type == NALU_TYPE_SUBSET_SPS ? FCC('AMVC') : FCC('H264');
			bmi.biPlanes		= 1;
			bmi.biBitCount		= 24;
			bmi.biSizeImage		= DIBSIZE(bmi);

			CSize aspect(params.width * params.sar.num, params.height * params.sar.den);
			ReduceDim(aspect);

			BYTE* extradata		= NULL;
			size_t extrasize	= 0;

			{
				int sps_present = 0;
				int pps_present = 0;

				Nalu.SetBuffer(pData.GetData(), pData.GetCount());
				while (!(sps_present && pps_present)
					   && Nalu.ReadNext()) {
					nalu_type = Nalu.GetType();
					switch (nalu_type) {
						case NALU_TYPE_SPS:
						case NALU_TYPE_SUBSET_SPS:
						case NALU_TYPE_PPS:
							if (nalu_type == NALU_TYPE_SPS || nalu_type == NALU_TYPE_SUBSET_SPS) {
								if (sps_present) continue;
								sps_present++;
							} else if (nalu_type == NALU_TYPE_PPS) {
								if (pps_present) continue;
								pps_present++;
							}

							static const BYTE start_code[4] = { 0, 0, 0, 1 };
							static const UINT start_code_size = sizeof(start_code);

							BYTE* new_extradata = (BYTE*)realloc(extradata, extrasize + Nalu.GetDataLength() + start_code_size);
							ASSERT(new_extradata);
							if (new_extradata) {
								extradata = new_extradata;
								memcpy(extradata + extrasize, start_code, start_code_size);
								extrasize += start_code_size;
								memcpy(extradata + extrasize, Nalu.GetDataBuffer(), Nalu.GetDataLength());
								extrasize += Nalu.GetDataLength();
							}
					}
				}
			}

			CreateMPEG2VISimple(pmt, &bmi, params.AvgTimePerFrame, aspect, extradata, extrasize, params.profile, params.level, 4);
			pmt->SetTemporalCompression(TRUE);
			pmt->SetVariableSize();

			free(extradata);
		}

		return true;
	}

	pData.RemoveAll();
	return false;
}

bool CBaseSplitterFileEx::Read(avchdr& h, int len, CMediaType* pmt/* = NULL*/)
{
	CAtlArray<BYTE> pData;
	pData.SetCount(len);
	ByteRead(pData.GetData(), len);

	return ParseAvc(pData, pmt);
}

bool CBaseSplitterFileEx::Read(avchdr& h, int len, CAtlArray<BYTE>& pData, CMediaType* pmt/* = NULL*/)
{
	if (pData.IsEmpty()) {
		CAtlArray<BYTE> pTmpData;
		pTmpData.SetCount(len);
		ByteRead(pTmpData.GetData(), len);

		NALU_TYPE nalu_type = NALU_TYPE_UNKNOWN;
		CH264Nalu Nalu;
		Nalu.SetBuffer(pTmpData.GetData(), pTmpData.GetCount());
		while (!(nalu_type == NALU_TYPE_SPS || nalu_type == NALU_TYPE_SUBSET_SPS)
				&& Nalu.ReadNext()) {
			nalu_type = Nalu.GetType();
		}

		if (nalu_type != NALU_TYPE_SPS && nalu_type != NALU_TYPE_SUBSET_SPS) {
			return false;
		}

		pData.SetCount(len);
		memcpy(pData.GetData(), pTmpData.GetData(), len);
	} else {
		size_t dataLen = pData.GetCount();
		pData.SetCount(dataLen + len);
		ByteRead(pData.GetData() + dataLen, len);
	}

	return ParseAvc(pData, pmt);
}

static bool ParseHevc(CAtlArray<BYTE>& pData, CMediaType* pmt)
{
	NALU_TYPE nalu_type = NALU_TYPE_UNKNOWN;
	CH265Nalu Nalu;

	{
		int vps_present = 0;
		int sps_present = 0;

		Nalu.SetBuffer(pData.GetData(), pData.GetCount());
		Nalu.ReadNext();
		while (!(vps_present && sps_present)) {
			NALU_TYPE nalu_type = Nalu.GetType();
			if (!Nalu.ReadNext()) {
				break;
			}
			switch (nalu_type) {
				case NALU_TYPE_HEVC_VPS:
					vps_present++;
					break;
				case NALU_TYPE_HEVC_SPS:
					sps_present++;
					break;
			}
		}

		if (!(vps_present && sps_present)) {
			return false;
		}
	}

	Nalu.SetBuffer(pData.GetData(), pData.GetCount());
	nalu_type = NALU_TYPE_UNKNOWN;
	while (nalu_type != NALU_TYPE_HEVC_SPS && Nalu.ReadNext()) {
		nalu_type = Nalu.GetType();
	}

	vc_params_t params = { 0 };
	if (HEVCParser::ParseSequenceParameterSet(Nalu.GetDataBuffer() + 2, Nalu.GetDataLength() - 2, params)) {
		if (pmt) {
			BITMAPINFOHEADER bmi = { 0 };
			bmi.biSize			= sizeof(bmi);
			bmi.biWidth			= params.width;
			bmi.biHeight		= params.height;
			bmi.biCompression	= FCC('HEVC');
			bmi.biPlanes		= 1;
			bmi.biBitCount		= 24;
			bmi.biSizeImage		= DIBSIZE(bmi);

			CSize aspect(params.width * params.sar.num, params.height * params.sar.den);
			ReduceDim(aspect);

			BYTE* extradata		= NULL;
			size_t extrasize	= 0;

			{
				int vps_present = 0;
				int sps_present = 0;
				int pps_present = 0;

				Nalu.SetBuffer(pData.GetData(), pData.GetCount());
				while (!(vps_present && sps_present && pps_present)
					   && Nalu.ReadNext()) {
					nalu_type = Nalu.GetType();
					switch (nalu_type) {
						case NALU_TYPE_HEVC_VPS:
						case NALU_TYPE_HEVC_SPS:
						case NALU_TYPE_HEVC_PPS:
							if (nalu_type == NALU_TYPE_HEVC_VPS) {
								if (vps_present) continue;
								vps_present++;
								if (!HEVCParser::ParseVideoParameterSet(Nalu.GetDataBuffer() + 2, Nalu.GetDataLength() - 2, params)) {
									return false;
								}
							} else if (nalu_type == NALU_TYPE_HEVC_SPS) {
								if (sps_present) continue;
								sps_present++;
							} else if (nalu_type == NALU_TYPE_HEVC_PPS) {
								if (pps_present) continue;
								pps_present++;
							}

							static const BYTE start_code[3] = { 0, 0, 1 };
							static const UINT start_code_size = sizeof(start_code);

							BYTE* new_extradata = (BYTE*)realloc(extradata, extrasize + Nalu.GetDataLength() + start_code_size);
							ASSERT(new_extradata);
							if (new_extradata) {
								extradata = new_extradata;
								memcpy(extradata + extrasize, start_code, start_code_size);
								extrasize += start_code_size;
								memcpy(extradata + extrasize, Nalu.GetDataBuffer(), Nalu.GetDataLength());
								extrasize += Nalu.GetDataLength();
							}
					}
				}
			}

			REFERENCE_TIME AvgTimePerFrame = 0;
			if (params.vps_timing.num_units_in_tick && params.vps_timing.time_scale) {
				AvgTimePerFrame = (REFERENCE_TIME)(10000000.0 * params.vps_timing.num_units_in_tick / params.vps_timing.time_scale);
			} else if (params.vui_timing.num_units_in_tick && params.vui_timing.time_scale) {
				AvgTimePerFrame = (REFERENCE_TIME)(10000000.0 * params.vui_timing.num_units_in_tick / params.vui_timing.time_scale);
			}

			CreateMPEG2VISimple(pmt, &bmi, AvgTimePerFrame, aspect, extradata, extrasize, params.profile, params.level, params.nal_length_size);
			pmt->SetTemporalCompression(TRUE);
			pmt->SetVariableSize();

			free(extradata);
		}

		return true;
	}

	pData.RemoveAll();
	return false;
}

bool CBaseSplitterFileEx::Read(hevchdr& h, int len, CMediaType* pmt/* = NULL*/)
{
	CAtlArray<BYTE> pData;
	pData.SetCount(len);
	ByteRead(pData.GetData(), len);

	return ParseHevc(pData, pmt);
}

bool CBaseSplitterFileEx::Read(hevchdr& h, int len, CAtlArray<BYTE>& pData, CMediaType* pmt/* = NULL*/)
{
	if (pData.IsEmpty()) {
		CAtlArray<BYTE> pTmpData;
		pTmpData.SetCount(len);
		ByteRead(pTmpData.GetData(), len);

		NALU_TYPE nalu_type = NALU_TYPE_UNKNOWN;
		CH265Nalu Nalu;
		Nalu.SetBuffer(pTmpData.GetData(), pTmpData.GetCount());
		while (nalu_type != NALU_TYPE_HEVC_VPS && Nalu.ReadNext()) {
			nalu_type = Nalu.GetType();
		}

		if (nalu_type != NALU_TYPE_HEVC_VPS) {
			return false;
		}

		pData.SetCount(len);
		memcpy(pData.GetData(), pTmpData.GetData(), len);
	} else {
		size_t dataLen = pData.GetCount();
		pData.SetCount(dataLen + len);
		ByteRead(pData.GetData() + dataLen, len);
	}

	return ParseHevc(pData, pmt);
}

bool CBaseSplitterFileEx::Read(adx_adpcm_hdr& h, int len, CMediaType* pmt)
{
	memset(&h, 0, sizeof(h));
	__int64 start = GetPos();

	if (BitRead(16) != 0x8000) {
		return false;
	}
	SHORT offset = BitRead(16);
	if (offset < 6) {
		return false;
	}

	if (len > offset) {
		BYTE copyright_string[6] = { 0 };
		Seek(start + offset - 6 + 4);
		ByteRead(copyright_string, 6);
		if (memcmp(copyright_string, "(c)CRI", 6)) {
			return false;
		}
		Seek(start + 2 + 2);
	}

	BYTE encoding		= BitRead(8);
	BYTE block_size		= BitRead(8);
	BYTE sample_size	= BitRead(8);
	h.channels			= BitRead(8);
	h.samplerate		= BitRead(32);
	if (encoding != 3 || block_size != 18 || sample_size != 4) {
		return false;
	}
	if (h.channels <= 0 || h.channels > 2) {
		return false;
	}
	if (h.samplerate < 1 || h.samplerate > INT_MAX / (h.channels * 18 * 8)) {
		return false;
	}

	if (pmt) {
		pmt->majortype			= MEDIATYPE_Audio;
		pmt->subtype			= MEDIASUBTYPE_ADX_ADPCM;
		pmt->formattype			= FORMAT_WaveFormatEx;

		WAVEFORMATEX* wfe		= (WAVEFORMATEX*)pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX));
		memset(wfe, 0, sizeof(WAVEFORMATEX));
		wfe->wFormatTag			= WAVE_FORMAT_ADX_ADPCM;
		wfe->nChannels			= h.channels;
		wfe->nSamplesPerSec		= h.samplerate;
		wfe->wBitsPerSample		= 16;
		wfe->nBlockAlign		= wfe->nChannels * wfe->wBitsPerSample >> 3;
		wfe->nAvgBytesPerSec	= wfe->nBlockAlign * wfe->nSamplesPerSec;
	}

	return true;
}

bool CBaseSplitterFileEx::Read(pcm_law_hdr& h, int len, bool bAlaw, CMediaType* pmt)
{
	if (pmt) {
		pmt->majortype			= MEDIATYPE_Audio;
		pmt->subtype			= bAlaw ? MEDIASUBTYPE_ALAW : MEDIASUBTYPE_MULAW;
		pmt->formattype			= FORMAT_WaveFormatEx;

		WAVEFORMATEX* wfe		= (WAVEFORMATEX*)pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX));
		memset(wfe, 0, sizeof(WAVEFORMATEX));
		wfe->wFormatTag			= bAlaw ? WAVE_FORMAT_ALAW : WAVE_FORMAT_MULAW;
		wfe->nChannels			= 1;
		wfe->nSamplesPerSec		= 8000;
		wfe->wBitsPerSample		= 8;
		wfe->nBlockAlign		= wfe->nChannels * wfe->wBitsPerSample >> 3;
		wfe->nAvgBytesPerSec	= wfe->nBlockAlign * wfe->nSamplesPerSec;
	}

	return true;
}

bool CBaseSplitterFileEx::Read(opus_ts_hdr& h, int len, CAtlArray<BYTE>& extradata, CMediaType* pmt/* = NULL*/)
{
	if (len < 2 || extradata.GetCount() != 30) {
		return false;
	}

	if ((BitRead(16) & 0xFFE0) != 0x7FE0) {
		return false;
	}

	if (pmt) {
		BYTE* buf = extradata.GetData();
		const size_t nCount = extradata.GetCount();
		CGolombBuffer Buffer(buf + 8, nCount - 8); // skip "OpusHead"

		Buffer.ReadByte();
		const BYTE channels = Buffer.ReadByte();
		Buffer.SkipBytes(2);
		Buffer.SkipBytes(4);
		Buffer.SkipBytes(2);

		pmt->majortype       = MEDIATYPE_Audio;
		pmt->subtype         = MEDIASUBTYPE_OPUS;
		pmt->formattype      = FORMAT_WaveFormatEx;

		WAVEFORMATEX* wfe    = (WAVEFORMATEX*)pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX) + nCount);
		memset(wfe, 0, sizeof(WAVEFORMATEX));
		wfe->wFormatTag      = WAVE_FORMAT_OPUS;
		wfe->nChannels       = channels;
		wfe->nSamplesPerSec  = 48000;
		wfe->wBitsPerSample  = 16;
		wfe->nBlockAlign     = 1;
		wfe->nAvgBytesPerSec = 0;
		wfe->cbSize          = nCount;
		memcpy((BYTE*)(wfe + 1), buf, nCount);
	}
	
	return true;
}
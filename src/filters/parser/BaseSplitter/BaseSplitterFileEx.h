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

#include "BaseSplitterFile.h"
#include "../../../DSUtil/Mpeg2Def.h"
#include "../../../DSUtil/VideoParser.h"

#if (0)
	#define DEBUG_ASSERT ASSERT
#else
	#define DEBUG_ASSERT __noop
#endif

class CBaseSplitterFileEx : public CBaseSplitterFile
{
public:
	CBaseSplitterFileEx(IAsyncReader* pReader, HRESULT& hr, int fmode = FM_FILE);
	virtual ~CBaseSplitterFileEx();

	bool NextMpegStartCode(BYTE& b, __int64 len = 65536);

	enum mpeg_t {
		mpegunk,
		mpeg1,
		mpeg2
	};

#pragma pack(push, 1)

	struct seqhdr {
		WORD width;
		WORD height;
		BYTE ar:4;
		DWORD ifps;
		DWORD bitrate;
		DWORD vbv;
		BYTE constrained:1;
		BYTE fiqm:1;
		BYTE iqm[64];
		BYTE fniqm:1;
		BYTE niqm[64];
		// ext
		BYTE startcodeid:4;
		BYTE profile_levelescape:1;
		BYTE profile:3;
		BYTE level:4;
		BYTE progressive:1;
		BYTE chroma:2;
		BYTE lowdelay:1;
		// misc
		LONG arx, ary;
		//
		CAtlArray<BYTE> data;

		seqhdr() {
			memset(this, 0, sizeof(*this));
		}
	};

	struct mpahdr
	{
		WORD sync:11;
		WORD version:2;
		WORD layer:2;
		WORD crc:1;
		WORD bitrate:4;
		WORD freq:2;
		WORD padding:1;
		WORD privatebit:1;
		WORD channels:2;
		WORD modeext:2;
		WORD copyright:1;
		WORD original:1;
		WORD emphasis:2;

		int Samplerate, FrameSize, FrameSamples;
		REFERENCE_TIME rtDuration;

		bool operator == (const struct mpahdr& h) const {
			return (sync == h.sync
					&& version == h.version
					&& layer == h.layer
					&& freq == h.freq
					&& channels == h.channels);
		}
	};

	struct aachdr
	{
		WORD sync:12;
		WORD version:1;
		WORD layer:2;
		WORD fcrc:1;
		WORD profile:2;
		WORD freq:4;
		WORD privatebit:1;
		WORD channels:3;
		WORD original:1;
		WORD home:1; // ?

		WORD copyright_id_bit:1;
		WORD copyright_id_start:1;
		WORD aac_frame_length:13;
		WORD adts_buffer_fullness:11;
		WORD no_raw_data_blocks_in_frame:2;

		WORD crc;

		int Samplerate, FrameSize, FrameSamples;
		REFERENCE_TIME rtDuration;

		bool operator == (const struct aachdr& h) const {
			return (sync == h.sync
					&& version == h.version
					&& profile == h.profile
					&& freq == h.freq
					&& channels == h.channels);
		}
	};

	struct latm_aachdr
	{
		int samplerate;
		int channels;

		bool operator == (const struct latm_aachdr& h) const {
			return (samplerate == h.samplerate
					&& channels == h.channels);
		}
	};

	struct ac3hdr
	{
		WORD sync;
		WORD crc1;
		WORD frame_size;
		WORD sample_rate;
		BYTE fscod:2;
		BYTE frmsizecod:6;
		BYTE bsid:5;
		BYTE bsmod:3;
		BYTE acmod:3;
		BYTE cmixlev:2;
		BYTE surmixlev:2;
		BYTE dsurmod:2;
		BYTE lfeon:1;
		BYTE sr_shift;
		// E-AC3 header
		BYTE frame_type;
		BYTE substreamid;
		BYTE sr_code;
		BYTE num_blocks;
		// the rest is unimportant for us

		bool operator == (const struct ac3hdr& h) const {
			return (frame_size == h.frame_size
					&& fscod == h.fscod
					&& frmsizecod == h.frmsizecod
					&& acmod == h.acmod
					&& lfeon == h.lfeon);
		}
	};

	struct dtshdr
	{
		DWORD sync;
		BYTE frametype:1;
		BYTE deficitsamplecount:5;
		BYTE fcrc:1;
		BYTE nblocks:7;
		WORD framebytes;
		BYTE amode:6;
		BYTE sfreq:4;
		BYTE rate:5;

		BYTE downmix:1;
		BYTE dynrange:1;
		BYTE timestamp:1;
		BYTE aux_data:1;
		BYTE hdcd:1;
		BYTE ext_descr:3;
		BYTE ext_coding:1;
		BYTE aspf:1;
		BYTE lfe:2;
		BYTE predictor_history:1;
		BYTE bits_per_sample:2;
	};

	struct dtslbr_hdr {
	};

	struct lpcmhdr
	{
		BYTE emphasis:1;
		BYTE mute:1;
		BYTE reserved1:1;
		BYTE framenum:5;
		BYTE quantwordlen:2;
		BYTE freq:2; // 48, 96, 44.1, 32
		BYTE reserved2:1;
		BYTE channels:3; // +1
		BYTE drc; // 0x80: off
	};

	struct dvdalpcmhdr
	{
		// http://dvd-audio.sourceforge.net/spec/aob.shtml
		WORD firstaudioframe;
		BYTE unknown1;
		BYTE bitpersample1:4;
		BYTE bitpersample2:4;
		BYTE samplerate1:4;
		BYTE samplerate2:4;
		BYTE unknown2;
		BYTE groupassignment;
		BYTE unknown3;
	};

	struct hdmvlpcmhdr
	{
		WORD size;
		BYTE channels:4;
		BYTE samplerate:4;
		BYTE bitpersample:2;
	};

	struct mlphdr
	{
		DWORD size;
		//DWORD samplerate;
		//WORD bitdepth;
		//WORD channels;
	};

	struct dvdspuhdr {
	};

	struct svcdspuhdr {
	};

	struct cvdspuhdr {
	};

	struct ps2audhdr
	{
		// 'SShd' + len (0x18)
		DWORD unk1;
		DWORD freq;
		DWORD channels;
		DWORD interleave; // bytes per channel
		// padding: FF .. FF
		// 'SSbd' + len
		// pcm or adpcm data
	};

	struct ps2subhdr {
	};

	struct vc1hdr
	{
		BYTE profile;
		BYTE level;
		BYTE chromaformat;
		BYTE frmrtq_postproc;
		BYTE bitrtq_postproc;
		BYTE postprocflag;
		BYTE broadcast;
		BYTE interlace;
		BYTE tfcntrflag;
		BYTE finterpflag;
		BYTE psf;
		unsigned int width, height;

		fraction_t sar;
	};

	struct dirachdr {
	};

	struct hdmvsubhdr {
	};

	struct dvbsubhdr {
	};

	struct teletextsubhdr {
	};

	struct avchdr {
	};

	struct hevchdr {
	};

	struct adx_adpcm_hdr {
		DWORD channels;
		DWORD samplerate;
	};

	struct pcm_law_hdr {
	};

	struct opus_ts_hdr {
	};

#pragma pack(pop)

	bool Read(seqhdr& h, int len, CMediaType* pmt = nullptr, bool find_sync = true);
	bool Read(seqhdr& h, CAtlArray<BYTE>& buf, CMediaType* pmt = nullptr, bool find_sync = true);
	bool Read(mpahdr& h, int len, CMediaType* pmt = nullptr, bool fAllowV25 = false, bool find_sync = false);
	bool Read(aachdr& h, int len, CMediaType* pmt = nullptr, bool find_sync = true);
	bool Read(latm_aachdr& h, int len, CMediaType* pmt = nullptr);
	bool Read(ac3hdr& h, int len, CMediaType* pmt = nullptr, bool find_sync = true, bool AC3CoreOnly = true);
	bool Read(dtshdr& h, int len, CMediaType* pmt = nullptr, bool find_sync = true);
	bool Read(dtslbr_hdr& h, int len, CMediaType* pmt = nullptr);
	bool Read(lpcmhdr& h, CMediaType* pmt = nullptr);
	bool Read(dvdalpcmhdr& h, int len, CMediaType* pmt = nullptr);
	bool Read(hdmvlpcmhdr& h, CMediaType* pmt = nullptr);
	bool Read(mlphdr& h, int len, CMediaType* pmt = nullptr, bool find_sync = false);
	bool Read(dvdspuhdr& h, CMediaType* pmt = nullptr);
	bool Read(svcdspuhdr& h, CMediaType* pmt = nullptr);
	bool Read(cvdspuhdr& h, CMediaType* pmt = nullptr);
	bool Read(ps2audhdr& h, CMediaType* pmt = nullptr);
	bool Read(ps2subhdr& h, CMediaType* pmt = nullptr);
	bool Read(vc1hdr& h, int len, CMediaType* pmt = nullptr);
	bool Read(dirachdr& h, int len, CMediaType* pmt = nullptr);

	bool Read(hdmvsubhdr& h, CMediaType* pmt, LPCSTR language_code);
	bool Read(dvbsubhdr& h, int len, CMediaType* pmt, LPCSTR language_code, bool bCheckFormat = true);
	bool Read(teletextsubhdr& h, int len, CMediaType* pmt, LPCSTR language_code, bool bCheckFormat = true);
	
	bool Read(avchdr& h, CAtlArray<BYTE>& pData, CMediaType* pmt = nullptr);
	bool Read(avchdr& h, int len, CMediaType* pmt = nullptr);
	bool Read(avchdr& h, int len, CAtlArray<BYTE>& pData, CMediaType* pmt = nullptr);
	
	bool Read(hevchdr& h, CAtlArray<BYTE>& pData, CMediaType* pmt = nullptr);
	bool Read(hevchdr& h, int len, CMediaType* pmt = nullptr);
	bool Read(hevchdr& h, int len, CAtlArray<BYTE>& pData, CMediaType* pmt = nullptr);
	
	bool Read(adx_adpcm_hdr& h, int len, CMediaType* pmt = nullptr);
	bool Read(pcm_law_hdr& h, int len, bool bAlaw, CMediaType* pmt = nullptr);
	bool Read(opus_ts_hdr& h, int len, CAtlArray<BYTE>& extradata, CMediaType* pmt = nullptr);
};

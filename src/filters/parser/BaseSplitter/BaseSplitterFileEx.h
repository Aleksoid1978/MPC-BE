/*
 * (C) 2003-2006 Gabest
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

#pragma once

#include "BaseSplitterFile.h"
#include "../../../DSUtil/Mpeg2Def.h"
#include "../../../DSUtil/SimpleBuffer.h"
#include "../../../DSUtil/VideoParser.h"

#if (0)
	#define DEBUG_ASSERT ASSERT
#else
	#define DEBUG_ASSERT __noop
#endif

class CBaseSplitterFileEx : public CBaseSplitterFile
{
	CSimpleBuffer<BYTE> m_tmpBuffer;

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
		struct {;
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
		} hdr;
		//
		std::vector<BYTE> data;

		seqhdr() {
			memset(&hdr, 0, sizeof(hdr));
		}
	};

	struct mpahdr
	{
		WORD sync:11;
		BYTE version:2;
		BYTE layer:2;
		BYTE crc:1;
		BYTE bitrate:4;
		BYTE freq:2;
		BYTE padding:1;
		BYTE privatebit:1;
		BYTE channels:2;
		BYTE modeext:2;
		BYTE copyright:1;
		BYTE original:1;
		BYTE emphasis:2;

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
		BYTE version:1;
		BYTE layer:2;
		BYTE fcrc:1;
		BYTE profile:2;
		BYTE freq:4;
		BYTE privatebit:1;
		BYTE channel_index:3;
		BYTE original:1;
		BYTE home:1; // ?

		BYTE copyright_id_bit:1;
		BYTE copyright_id_start:1;
		WORD aac_frame_length:13;
		WORD adts_buffer_fullness:11;
		BYTE no_raw_data_blocks_in_frame:2;

		BYTE channels;
		WORD crc;

		int Samplerate, FrameSize, FrameSamples;
		REFERENCE_TIME rtDuration;

		bool operator == (const struct aachdr& h) const {
			return (sync == h.sync
					&& version == h.version
					&& profile == h.profile
					&& freq == h.freq
					&& channel_index == h.channel_index);
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
		bool bMixedMVC = false;
	};

	struct hevchdr {
	};

	struct mpeg4videohdr {
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
	bool Read(seqhdr& h, std::vector<BYTE>& buf, CMediaType* pmt = nullptr, bool find_sync = true);
	bool Read(mpahdr& h, int len, CMediaType* pmt = nullptr, bool fAllowV25 = false, bool find_sync = false);
	bool Read(aachdr& h, int len, CMediaType* pmt = nullptr, bool find_sync = true);
	bool Read(latm_aachdr& h, int len, CMediaType* pmt = nullptr);
	bool Read(ac3hdr& h, int len, CMediaType* pmt = nullptr, bool find_sync = true, bool AC3CoreOnly = true);
	bool Read(dtshdr& h, int len, CMediaType* pmt = nullptr, bool find_sync = true);
	bool Read(dtslbr_hdr& h, int len, CMediaType* pmt = nullptr);
	bool Read(mlphdr& h, int len, CMediaType* pmt = nullptr, bool find_sync = false);
	bool Read(dvdspuhdr& h, CMediaType* pmt = nullptr);
	bool Read(svcdspuhdr& h, CMediaType* pmt = nullptr);
	bool Read(cvdspuhdr& h, CMediaType* pmt = nullptr);
	bool Read(ps2audhdr& h, CMediaType* pmt = nullptr);
	bool Read(ps2subhdr& h, CMediaType* pmt = nullptr);
	bool Read(vc1hdr& h, int len, CMediaType* pmt = nullptr);
	bool Read(dirachdr& h, int len, CMediaType* pmt = nullptr);

	bool Read(hdmvsubhdr& h, CMediaType* pmt, LPCSTR ISO_639_codes);
	bool Read(dvbsubhdr& h, int len, CMediaType* pmt, LPCSTR ISO_639_codes, bool bCheckFormat = true);
	bool Read(teletextsubhdr& h, int len, CMediaType* pmt, LPCSTR ISO_639_codes, bool bCheckFormat = true);

	bool Read(avchdr& h, std::vector<BYTE>& pData, CMediaType* pmt = nullptr);
	bool Read(avchdr& h, int len, CMediaType* pmt = nullptr);
	bool Read(avchdr& h, int len, std::vector<BYTE>& pData, CMediaType* pmt = nullptr);

	bool Read(hevchdr& h, std::vector<BYTE>& pData, CMediaType* pmt = nullptr);
	bool Read(hevchdr& h, int len, CMediaType* pmt = nullptr);
	bool Read(hevchdr& h, int len, std::vector<BYTE>& pData, CMediaType* pmt = nullptr);

	bool Read(mpeg4videohdr& h, int len, CMediaType* pmt = nullptr);

	bool Read(adx_adpcm_hdr& h, int len, CMediaType* pmt = nullptr);
	bool Read(pcm_law_hdr& h, bool bAlaw, CMediaType* pmt = nullptr);
	bool Read(opus_ts_hdr& h, int len, std::vector<BYTE>& extradata, CMediaType* pmt = nullptr);

	// LPCM
	bool ReadDVDLPCMHdr(CMediaType* pmt = nullptr);
	bool ReadDVDALPCMHdr(CMediaType* pmt = nullptr);
	bool ReadHDMVLPCMHdr(CMediaType* pmt = nullptr);
};

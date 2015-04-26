/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2015 see Authors.txt
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
#include "../../../DSUtil/DSUtil.h"
#include "../../../DSUtil/VideoParser.h"

#define MAX_SPSPPS			256			// Max size for a SPS/PPS packet
class CGolombBuffer;

class CBaseSplitterFileEx : public CBaseSplitterFile
{
	int m_tslen; // transport stream packet length (188 or 192 bytes, auto-detected)

protected :
	REFERENCE_TIME m_rtPTSOffset;

public:
	CBaseSplitterFileEx(IAsyncReader* pReader, HRESULT& hr, bool fRandomAccess = true, bool fStreaming = false, bool fStreamingDetect = false);
	virtual ~CBaseSplitterFileEx();

	bool NextMpegStartCode(BYTE& b, __int64 len = 65536);

#pragma pack(push, 1)

	enum mpeg_t {mpegunk, mpeg1, mpeg2};

	struct pshdr {
		mpeg_t type;
		UINT64 scr, bitrate;
	};

	struct pssyshdr {
		DWORD rate_bound;
		BYTE video_bound, audio_bound;
		bool fixed_rate, csps;
		bool sys_video_loc_flag, sys_audio_loc_flag;
	};

	struct peshdr {
		WORD len;

		BYTE type:2, fpts:1, fdts:1;
		REFERENCE_TIME pts, dts;

		// mpeg1 stuff
		UINT64 std_buff_size;

		// mpeg2 stuff
		BYTE scrambling:2, priority:1, alignment:1, copyright:1, original:1;
		BYTE escr:1, esrate:1, dsmtrickmode:1, morecopyright:1, crc:1, extension:1;
		BYTE hdrlen;

		BYTE id_ext;

		struct peshdr() {
			memset(this, 0, sizeof(*this));
		}
	};

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

		void Init() {
			memset(this, 0, sizeof(*this));
		}

		seqhdr() {
			Init();
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

	struct dvdspuhdr
	{
		// nothing ;)
	};

	struct hdmvsubhdr
	{
		// nothing ;)
	};

	struct svcdspuhdr
	{
		// nothing ;)
	};

	struct cvdspuhdr
	{
		// nothing ;)
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

	struct ps2subhdr
	{
		// nothing ;)
	};

	struct trhdr
	{
		__int64 PCR;
		__int64 next;
		int bytes;
		WORD pid:13;
		BYTE sync; // 0x47
		BYTE error:1;
		BYTE payloadstart:1;
		BYTE transportpriority:1;
		BYTE scrambling:2;
		BYTE adapfield:1;
		BYTE payload:1;
		BYTE counter:4;
		// if adapfield set
		BYTE length;
		BYTE discontinuity:1;
		BYTE randomaccess:1;
		BYTE priority:1;
		BYTE fPCR:1;
		BYTE OPCR:1;
		BYTE splicingpoint:1;
		BYTE privatedata:1;
		BYTE extension:1;
		// TODO: add more fields here when the flags above are set (they aren't very interesting...)
	};

	struct trsechdr
	{
		WORD section_syntax_indicator:1;
		WORD zero:1;
		WORD reserved1:2;
		WORD section_length:12;
		WORD transport_stream_id;
		BYTE table_id;
		BYTE reserved2:2;
		BYTE version_number:5;
		BYTE current_next_indicator:1;
		BYTE section_number;
		BYTE last_section_number;
	};

	// http://multimedia.cx/mirror/av_format_v1.pdf
	struct pvahdr
	{
		WORD sync; // 'VA'
		BYTE streamid; // 1 - video, 2 - audio
		BYTE counter;
		BYTE res1; // 0x55
		BYTE res2:3;
		BYTE fpts:1;
		BYTE postbytes:2;
		BYTE prebytes:2;
		WORD length;
		REFERENCE_TIME pts;
	};

	enum spsppsindex {
		index_unknown   = -1,
		index_subsetsps = 0,
		index_sps       = 1,
		index_pps1      = 2,
		index_pps2      = 3,
	};

	struct spsppsdata
	{
		BYTE buffer[MAX_SPSPPS];
		unsigned int size;
		bool complete;
	};

	struct avchdr
	{
		vc_params_t params;

		spsppsdata spspps[4];
		BYTE lastid;

		avchdr() {
			memset(this, 0, sizeof(*this));
		}
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

		AV_Rational sar;
	};

	struct dirachdr {
	};

	struct dvbsub {
	};

	struct hevchdr {
	};

	struct adx_adpcm_hdr {
		DWORD channels;
		DWORD samplerate;
	};

	struct pcm_law_hdr {
	};

#pragma pack(pop)

	bool Read(pshdr& h);
	bool Read(pssyshdr& h);
	bool Read(peshdr& h, BYTE code);
	bool Read(seqhdr& h, int len, CMediaType* pmt = NULL, bool find_sync = true);
	bool Read(seqhdr& h, CAtlArray<BYTE>& buf, CMediaType* pmt = NULL, bool find_sync = true);
	bool Read(mpahdr& h, int len, CMediaType* pmt = NULL, bool fAllowV25 = false, bool find_sync = false);
	bool Read(aachdr& h, int len, CMediaType* pmt = NULL, bool find_sync = true);
	bool Read(latm_aachdr& h, int len, CMediaType* pmt = NULL);
	bool Read(ac3hdr& h, int len, CMediaType* pmt = NULL, bool find_sync = true, bool AC3CoreOnly = true);
	bool Read(dtshdr& h, int len, CMediaType* pmt = NULL, bool find_sync = true);
	bool Read(lpcmhdr& h, CMediaType* pmt = NULL);
	bool Read(dvdalpcmhdr& h, int len, CMediaType* pmt = NULL);
	bool Read(hdmvlpcmhdr& h, CMediaType* pmt = NULL);
	bool Read(mlphdr& h, int len, CMediaType* pmt = NULL, bool find_sync = false);
	bool Read(dvdspuhdr& h, CMediaType* pmt = NULL);
	bool Read(hdmvsubhdr& h, CMediaType* pmt = NULL, const char* language_code = NULL);
	bool Read(svcdspuhdr& h, CMediaType* pmt = NULL);
	bool Read(cvdspuhdr& h, CMediaType* pmt = NULL);
	bool Read(ps2audhdr& h, CMediaType* pmt = NULL);
	bool Read(ps2subhdr& h, CMediaType* pmt = NULL);
	__int64 Read(trhdr& h, bool fSync = true);
	bool Read(trsechdr& h);
	bool Read(pvahdr& h, bool fSync = true);
	bool Read(avchdr& h, int len, CMediaType* pmt = NULL);
	bool Read(vc1hdr& h, int len, CMediaType* pmt = NULL);
	bool Read(dirachdr& h, int len, CMediaType* pmt = NULL);
	bool Read(dvbsub& h, int len, CMediaType* pmt = NULL, bool bSimpleAdd = false);
	bool Read(avchdr& h, spsppsindex index);
	bool Read(hevchdr& h, int len, CMediaType* pmt = NULL);
	bool Read(adx_adpcm_hdr& h, int len, CMediaType* pmt = NULL);
	bool Read(pcm_law_hdr& h, int len, bool bAlaw, CMediaType* pmt = NULL);
};

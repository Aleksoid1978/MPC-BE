/*
 * (C) 2012-2017 see Authors.txt
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
#include "mpc_file.h"
#include "MusepackSplitter.h"
#include <math.h>

#define OLD_GAIN_REF		64.82
#define MPC_KEY(k1,k2)		((k1<<8)|(k2))

static int freq[] = {44100, 48000, 37800, 32000};

//-----------------------------------------------------------------------------
//
//	CMPCFile class
//
//-----------------------------------------------------------------------------

CMPCFile::CMPCFile() :
	duration_10mhz(0),
	reader(nullptr),
	seek_table(nullptr),
	seek_table_position(0)
{
	bits_to_skip		= 0;

	gain_album_db		= 0;
	gain_album_peak_db	= 0;
	gain_title_db		= 0;
	gain_title_peak_db	= 0;
}

CMPCFile::~CMPCFile()
{
	if (seek_table) {
		free(seek_table);
		seek_table = nullptr;
	}
}

int CMPCFile::Open_SV8()
{
	bool	done;
	int		ret;

	// means no seek table
	seek_table_position	= 0;
	seek_table_size		= 0;

	done = false;

	// now loop through a few packets
	CMPCPacket packet;
	do {
		ret = packet.Load(reader);
		if (ret < 0) {
			return -1;			// end of file reached before initialization is done ?
		}

		switch (packet.key) {
			case MPC_KEY('S','H'):
				ret = ReadStreamHeader(&packet);
				break;
			case MPC_KEY('R','G'):
				ret = ReadReplaygain(&packet);
				break;
			case MPC_KEY('S','O'):
				ret = ReadSeekOffset(&packet);
				break;
			case MPC_KEY('E','I'):
				break;

				// audio packet - initialization is over
			case MPC_KEY('A','P'):
				done = true;
				break;
		}

		// if parsing failed we quit
		if (ret < 0) {
			return ret;
		}
	} while (!done);

	// if there was a seek table, load it
	if (seek_table_position != 0) {
		int64_t first_ap_pos, avail;
		reader->GetPosition(&first_ap_pos, &avail);

		reader->Seek(seek_table_position/8);
		ret = packet.Load(reader);
		ret = ReadSeekTable(&packet);
		if (ret < 0) {
			return ret;
		}

		// seek back to first AP
		reader->Seek(first_ap_pos);
	}

	// we're at start
	current_sample = 0;
	return 0;
}

int CMPCFile::Open_SV7()
{
	int		ret;
	bool	done = false;

	// means no seek table
	seek_table_position	= 0;
	seek_table_size		= 0;

	uint8_t hdr[8*4];

	__int64 c, a;
	reader->GetPosition(&c, &a);
	reader->Seek(c + 6);
	ret = reader->Read(extradata, 16);
	extradata_size = 16;

	reader->Seek(c);
	ret = reader->ReadSwapped(hdr, 6*4);
	Bitstream b(hdr);

	b.NeedBits32();
	int audio_frames = (b.UGetBits(16)<<16) | b.UGetBits(16);

	b.NeedBits32();
	b.UGetBits(1);				// intensity stereo, should be 0
	b.UGetBits(1);				// mid-side
	b.UGetBits(6);				// max-band
	b.UGetBits(4);				// profile
	b.UGetBits(2);				// link

	uint8_t samplerateidx = b.UGetBits(2);			// samplerate
	sample_rate = (samplerateidx <= 3) ? freq[samplerateidx] : 0;

	channels	= 2;
	block_pwr	= 0;
	b.UGetBits(16);				// max-level

	b.NeedBits32();
	gain_title_db	= ((int16_t)b.SGetBits(16)) / 100.0;		// title gain
	int title_peak	= b.UGetBits(16);
	if (title_peak != 0) {
		gain_title_peak_db = (title_peak / 32767.0);
	} else {
		gain_title_peak_db = 0;
	}

	b.NeedBits32();
	gain_album_db	= ((int16_t)b.SGetBits(16)) / 100.0;		// album gain
	int album_peak	= b.UGetBits(16);
	if (album_peak != 0) {
		gain_album_peak_db = (album_peak / 32767.0);
	} else {
		gain_album_peak_db = 0;
	}

	b.NeedBits32();
	true_gapless		= b.UGetBits(1);			// true gapless
	int last_frame		= b.UGetBits(11);		// last-frame samples
	audio_block_frames	= 1;
	total_samples		= (audio_frames * 1152);
	if (true_gapless) {
		total_samples -= (1152 - last_frame);
	} else {
		total_samples -= 481;		// synth delay
	}
	duration_10mhz = (total_samples * 10000000) / sample_rate;

	fast_seeking = b.UGetBits(1);
	b.UGetBits(16);				// 19 zero bits
	b.UGetBits(3);

	b.NeedBits32();
	int ver = b.UGetBits(8);

	seek_pwr = 6;
	if (block_pwr > seek_pwr) {
		seek_pwr = block_pwr;
	}

	// calculate size as seen in mpc_demux.c
	int64_t tmp = 2+total_samples / (1152 << seek_pwr);
	if (tmp < seek_table_size) {
		tmp = seek_table_size;
	}

	// alloc memory for seek table
	seek_table = (uint64_t*)malloc(tmp * sizeof(uint64_t));
	if (!seek_table) {
		return -1;
	}

	int64_t pos, av;
	reader->GetPosition(&pos, &av);
	seek_table[0] = pos*8 - b.BitsLeft();		// current position in bits
	seek_table_size = 1;

	// loop through file to get complete seeking table
	CMPCPacket packet;
	int64_t seek_frames = 0;
	Seek(0);
	do {
		if (seek_frames*1152 >= total_samples) {
			break;
		}

		// new entry in seeking table
		if (seek_frames == (seek_table_size << seek_pwr)) {
			__int64 c, a;
			reader->GetPosition(&c, &a);
			seek_table[seek_table_size] = (c*8) + bits_to_skip;
			seek_table_size++;
		}

		ret = packet.Load_SV7(reader, bits_to_skip, true);
		if (ret<0) {
			break;
		}
		seek_frames += 1;
	} while (true);

	// seek to begin
	Seek(0);

	header_position = 0;
	bits_to_skip = 0;
	return 0;
}

// I/O for MPC file
int CMPCFile::Open(CMusePackReader *reader)
{
	int		ret = 0;
	__int64	size, avail;
	int		tagsize = 0;

	reader->GetSize(&avail, &size);

	// keep a local copy of the reader
	this->reader = reader;

	// check ID3 tag
	uint32_t ID3size = 0;
	reader->Seek(0);
	char buf[3];
	if (!reader->Read(buf, 3) && !strncmp(buf, "ID3", 3)) {
		reader->Seek(3+3); // ID3 tag, Version, Revision, Flags
		if (!reader->GetMagick(ID3size)) {
			ID3size = (((ID3size & 0x7F000000) >> 0x03) |
					   ((ID3size & 0x007F0000) >> 0x02) |
					   ((ID3size & 0x00007F00) >> 0x01) |
					   ((ID3size & 0x0000007F)		  ));
			ID3size += 10;
		}
	}
	reader->Seek(ID3size);

	// According to stream specification the first 4 bytes should be 'MPCK'
	uint32_t magick;
	ret = reader->GetMagick(magick);
	if (ret < 0) {
		return ret;
	}

	if (magick == 0x4d50434b) {					// MPCK
		return Open_SV8();
	}
#if (0)	// Disable support MPC7 until write ReadAudioPacket working with ffmpeg MPC7 decoder
	else if ((magick&0xffffff00) == 0x4d502b00) {		// MP+
		// stream version... only 7 is supported
		stream_version = magick & 0x0f;
		if (stream_version != 7) {
			return -1;
		}
		return Open_SV7();
	}
#endif
	return -1;
}

// parsing packets
int CMPCFile::ReadReplaygain(CMPCPacket *packet)
{
	Bitstream b(packet->payload);
	b.NeedBits();

	int version = b.UGetBits(8);
	if (version != 1) return 0;		// unsupported RG version. not critical to us...

	int16_t	val;
	b.NeedBits();	val = b.SGetBits(16);	gain_title_db = val / 256.0;
	b.NeedBits();	val = b.UGetBits(16);	gain_title_peak_db = val;
	b.NeedBits();	val = b.SGetBits(16);	gain_album_db = val / 256.0;
	b.NeedBits();	val = b.UGetBits(16);	gain_album_peak_db = val;

	if (gain_title_db != 0) {
		gain_title_db = OLD_GAIN_REF - gain_title_db;
	}
	if (gain_album_db != 0) {
		gain_album_db = OLD_GAIN_REF - gain_album_db;
	}
	if (gain_title_peak_db != 0) {
		gain_title_peak_db = pow((double)10.0, (double)(gain_title_peak_db / (20*256))) / (double)(1 << 15);
	}
	if (gain_album_peak_db != 0) {
		gain_album_peak_db = pow((double)10.0, (double)(gain_album_peak_db / (20*256))) / (double)(1 << 15);
	}

	return S_OK;
}

int CMPCFile::ReadSeekOffset(CMPCPacket *packet)
{
	Bitstream b(packet->payload);

	seek_table_position = b.GetMpcSize() * 8;
	seek_table_position += packet->file_position;

	// success
	return S_OK;
}

int CMPCFile::ReadSeekTable(CMPCPacket *packet)
{
	Bitstream b(packet->payload);

	// calculate size as seen in mpc_demux.c
	int64_t tmp		= b.GetMpcSize();	b.NeedBits();
	seek_table_size	= tmp;
	seek_pwr		= block_pwr + b.UGetBits(4);
	tmp				= 2+total_samples / (1152 << seek_pwr);
	if (tmp < seek_table_size) {
		tmp = seek_table_size;
	}

	// alloc memory for seek table
	seek_table = (uint64_t*)malloc(tmp * sizeof(uint64_t));

	uint64_t *table = seek_table;

	tmp			= b.GetMpcSize();
	table[0]	= (uint32_t)(tmp*8 + header_position);
	if (seek_table_size == 1) {
		return 0;
	}

	tmp			= b.GetMpcSize();
	table[1]	= (uint64_t)(tmp*8 + header_position);
	for (int i=2; i<seek_table_size; i++) {
		int code	= b.Get_Golomb(12);
		if (code&1) {
			code = -(code&(-1 << 1));
		}
		code		<<= 2;
		table[i]	= code + 2*table[i-1] - table[i-2];
	}

	return S_OK;
}

int CMPCFile::ReadStreamHeader(CMPCPacket *packet)
{
	Bitstream b(packet->payload);
	b.NeedBits();

	// let's do some reading
	uint32_t crc;
	crc	= b.UGetBits(16);
	b.NeedBits();
	crc	<<= 16;
	crc	|= b.UGetBits(16);

	// TODO: CRC checking.
	b.NeedBits();
	stream_version = b.UGetBits(8);

	switch (stream_version) {
		case 8:
			{
				total_samples = b.GetMpcSize();
				int64_t silence = b.GetMpcSize();

				memset(extradata, 0, 16);
				Bitstream o(extradata);

				b.NeedBits();
				uint8_t samplerateidx = b.UGetBits(3);
				o.PutBits(samplerateidx, 3);
				sample_rate = (samplerateidx <= 3) ? freq[samplerateidx] : 0;

				// let's calculate duration
				duration_10mhz = (total_samples * 10000000) / sample_rate;

				o.PutBits(b.UGetBits(5), 5);		// bands
				channels = b.UGetBits(4);			// channels
				o.PutBits(channels, 4);
				channels += 1;

				b.NeedBits();
				o.PutBits(b.UGetBits(1), 1);		// mid side
				block_pwr = b.UGetBits(3);
				o.PutBits(block_pwr, 3);
				block_pwr *= 2;
				audio_block_frames = 1 << (block_pwr);

				// store absolute position of stream header
				header_position = packet->file_position - 4*8;

				o.WriteBits();
				extradata_size = 2;
			}
			break;
		default:
			// stream version not supported
			return -2;
	}

	// everything is okay
	return S_OK;
}

// parsing out packets
int CMPCFile::ReadAudioPacket(CMPCPacket *packet, int64_t *cur_sample)
{
	// we just load packets until we face the SE packet. Then we return -1
	int ret;

	// end of file ?
	if (current_sample >= total_samples) {
		return -2;
	}

	do {
		switch (stream_version) {
			case 7:
				ret = packet->Load_SV7(reader, bits_to_skip);
				break;
			case 8:
				ret = packet->Load(reader);
				break;
			default:
				ret = -2;
				break;
		}
		if (ret < 0) {
			return ret;
		}

		// keep track of samples...
		if (cur_sample) {
			*cur_sample = current_sample;
		}
		if (stream_version == 8) {
			switch (packet->key) {
				case MPC_KEY('A','P'):
					current_sample += (1152*audio_block_frames);
					return 0;			// we got one
				case MPC_KEY('S','E'):
					return -2;
			}
		} else {
			current_sample += (1152*audio_block_frames);
			return 0;
		}

		// skip other packets
	} while (1);

	// unexpected...
	return S_OK;
}

int CMPCFile::Seek(int64_t seek_sample)
{
	// cannot seek
	if (seek_table == nullptr || seek_table_size == 0) {
		return -1;
	}

	int packet_num = seek_sample / (1152*audio_block_frames);

	// we need to seek back a little to avoid artifacts
	if (stream_version == 7) {
		if (packet_num > 32) {
			packet_num -= 32;
		} else {
			packet_num = 0;
		}
	}

	int i = (packet_num >> (seek_pwr - block_pwr));
	if (i >= seek_table_size) {
		i = seek_table_size-1;
	}


	// seek to position
	uint64_t pos		= seek_table[i] >> 3;
	bits_to_skip	= seek_table[i] & 0x07;
	reader->Seek(pos);

	// shift back
	i = i << (seek_pwr - block_pwr);
	current_sample = i*1152*audio_block_frames;

	return S_OK;
}

//-----------------------------------------------------------------------------
//
//	CMPCPacket
//
//-----------------------------------------------------------------------------

CMPCPacket::CMPCPacket() :
	file_position(0),
	packet(nullptr),
	payload(nullptr),
	packet_size(0),
	payload_size(0),
	key(0)
{
}

CMPCPacket::~CMPCPacket()
{
	// just let it go
	Release();
}

void CMPCPacket::Release()
{
	if (packet) {
		free(packet); packet = nullptr;
	}
	payload			= nullptr;
	packet_size		= 0;
	payload_size	= 0;
	key				= 0;
	file_position	= 0;
}

int CMPCPacket::Load_SV7(CMusePackReader *reader, int &bits_to_skip, bool only_parse)
{
	uint8_t	temp[14*1024];
	uint8_t	outtemp[14*1024];
	__int64	cur, av;
	int		t;
	int		total_bits;

	Release();

	reader->GetPosition(&cur, &av);
	int ret = reader->ReadSwapped(temp, std::min((__int64)sizeof(temp), (av-cur)));
	if (ret < 0) {
		return 0;
	}

	Bitstream b(temp);
	Bitstream o(outtemp);
	b.NeedBits32();

	// skip bits from previous frame
	if (bits_to_skip > 0) {
		b.DumpBits(bits_to_skip);
	}
	file_position = (cur*8) + bits_to_skip;

	// 20-bit frame length
	b.NeedBits24();
	int	frame_len_bits = b.UGetBits(20);
	o.PutBits(frame_len_bits, 20);

	if (!only_parse) {
		// load data
		int left_bits = frame_len_bits;
		while (left_bits > 16) {
			b.NeedBits();
			t = b.UGetBits(16);
			o.PutBits(t, 16);
			left_bits -= 16;
		}
		b.NeedBits();
		t = b.UGetBits(left_bits);
		o.PutBits(t, left_bits);

		// zero-align 32-bit word
		total_bits		= (20 + frame_len_bits + 31) &~ 31;
		int zero_bits	= total_bits - frame_len_bits;
		o.PutBits(0, zero_bits);
		o.WriteBits();
	}

	// bits to skip in the next frame
	__int64 finalpos	= file_position + 20 + frame_len_bits;
	bits_to_skip		= (finalpos & 7);
	reader->Seek(finalpos >> 3);

	// copy data
	if (!only_parse) {
		packet_size		= total_bits >> 3;
		payload_size	= packet_size;
		packet			= (uint8_t*)malloc(packet_size);
		payload			= packet;				// pointer to packet payload

		// copy data
		uint8_t *src	= outtemp;
		uint8_t *dst	= packet;
		int left	= packet_size;
		while (left > 3) {
			dst[0] = src[3];
			dst[1] = src[2];
			dst[2] = src[1];
			dst[3] = src[0];
			src += 4;
			dst += 4;
			left-= 4;
		}
	}

	return S_OK;
}

int CMPCPacket::Load(CMusePackReader *reader)
{
	uint16_t	key_val;
	int64_t	size_val, avail;
	int32_t	size_len;
	int		ret;
	Release();
	reader->GetPosition(&file_position, &avail);

	// end of stream
	if (file_position >= avail) {
		return -2;
	}
	file_position *= 8;

	ret = reader->GetKey(key_val);
	if (ret < 0) {
		return ret;
	}

	ret = reader->GetSizeElm(size_val, size_len);
	if (ret < 0) {
		return ret;
	}

	// if the key is not valid, quit
	if (!reader->KeyValid(key_val)) {
		return -1;
	}
	key = key_val;

	// now load the packet
	packet_size		= size_val;
	payload_size	= size_val - 2 - size_len;
	packet			= (uint8_t*)malloc(packet_size);
	payload			= packet + 2 + size_len;				// pointer to packet payload

	// roll back the bytes
	reader->Seek(file_position/8);
	ret = reader->Read(packet, packet_size);
	if (ret < 0) {
		return ret;
	}

	return S_OK;
}

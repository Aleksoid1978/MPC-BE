/*
 *
 * Adaptation for MPC-BE (C) 2012 Dmitry "Vortex" Koteroff (vortex@light-alloy.ru, http://light-alloy.ru)
 *
 * This file is part of MPC-BE and Light Alloy.
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

#include <vector>
#include "bits.h"

class CMusePackReader;

//-----------------------------------------------------------------------------
//
//	CMPCPacket
//
//-----------------------------------------------------------------------------

class CMPCPacket
{
public:
	int64	file_position;				// absolute file position (in bits)

	uint8	*packet;					// we own this one
	uint8	*payload;					// just a pointer
	int32	packet_size;				// whole packet size
	int32	payload_size;				// only the payload

	uint16	key;						// parsed key value

	REFERENCE_TIME	tStart, tStop;			// to be used later

public:
	CMPCPacket();
	virtual ~CMPCPacket();

	// loading packets
	int Load_SV7(CMusePackReader *reader, int &bits_to_skip, bool only_parse = false);
	int Load(CMusePackReader *reader);
	void Release();
};

//-----------------------------------------------------------------------------
//
//	CMPCFile class
//
//-----------------------------------------------------------------------------

class CMPCFile
{
public:
	// stream header
	__int64		duration_10mhz;			// total file duration
	int			stream_version;			// 7, 8 are supported
	int			sample_rate;
	int			channels;
	int			audio_block_frames;
	int			block_pwr;
	int			seek_pwr;

	uint8		true_gapless;
	uint8		fast_seeking;

	// replay gain
	float		gain_title_db;
	float		gain_title_peak_db;
	float		gain_album_db;
	float		gain_album_peak_db;

	// seeking table
	int64		seek_table_position;		// position of seeking table in file (in bits)
	int64		header_position;			// (in bits)
	uint64		*seek_table;
	int64		seek_table_size;

	// current position
	int64		total_samples;
	int64		current_sample;

	// internals
	CMusePackReader	*reader;				// file reader interface
	int			bits_to_skip;			// after seeking

	// buffer for ffmpeg extradata
	uint8		extradata[16];
	int			extradata_size;

	int Open_SV8();
	int Open_SV7();

public:
	CMPCFile();
	virtual ~CMPCFile();

	// I/O for MPC file
	int Open(CMusePackReader *reader);

	// parsing packets
	int ReadStreamHeader(CMPCPacket *packet);
	int ReadReplaygain(CMPCPacket *packet);
	int ReadSeekOffset(CMPCPacket *packet);
	int ReadSeekTable(CMPCPacket *packet);

	// parsing out packets
	int ReadAudioPacket(CMPCPacket *packet, int64 *cur_sample);
	int Seek(int64 seek_sample);
};

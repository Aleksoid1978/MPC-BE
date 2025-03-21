/* libFLAC - Free Lossless Audio Codec
 * Copyright (C) 2002-2009  Josh Coalson
 * Copyright (C) 2011-2025  Xiph.Org Foundation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Xiph.org Foundation nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FLAC__PRIVATE__OGG_DECODER_ASPECT_H
#define FLAC__PRIVATE__OGG_DECODER_ASPECT_H

#include <ogg/ogg.h>

#include "FLAC/ordinals.h"
#include "FLAC/stream_decoder.h" /* for FLAC__StreamDecoderReadStatus */
#include "share/compat.h"

typedef struct FLAC__OggDecoderAspect_LinkDetails {
	long serial_number;
	FLAC__off_t start_byte;
	FLAC__off_t end_byte;
	uint64_t samples;
	uint32_t number_of_other_streams;
	long * other_serial_numbers;
	FLAC__bool is_last;
} FLAC__OggDecoderAspect_LinkDetails;

typedef struct FLAC__OggDecoderAspect_TargetLink {
	long serial_number;
	FLAC__off_t start_byte;
	FLAC__off_t end_byte;
	uint64_t samples_in_preceding_links;
	uint64_t samples_this_link;
	uint32_t linknumber;
} FLAC__OggDecoderAspect_TargetLink;

typedef struct FLAC__OggDecoderAspect {
	/* these are storage for values that can be set through the API */
	FLAC__bool use_first_serial_number;
	long serial_number;

	/* these are for internal state related to Ogg decoding */
	ogg_stream_state stream_state;
	ogg_sync_state sync_state;
	uint32_t version_major, version_minor;
	FLAC__bool need_serial_number;
	FLAC__bool beginning_of_link;
	FLAC__bool bos_flag_seen;
	FLAC__bool end_of_stream;
	FLAC__bool end_of_link;
	FLAC__bool decode_chained_stream;
	FLAC__bool have_working_page; /* only if true will the following vars be valid */
	ogg_page working_page;
	FLAC__bool have_working_packet; /* only if true will the following vars be valid */
	ogg_packet working_packet; /* as we work through the packet we will move working_packet.packet forward and working_packet.bytes down */
	FLAC__OggDecoderAspect_LinkDetails *linkdetails;
	FLAC__OggDecoderAspect_TargetLink target_link; /* to pass data to the seek routine */
	uint32_t number_of_links_detected;
	uint32_t number_of_links_indexed;
	uint32_t number_of_links_allocated;
	uint32_t current_linknumber; /* The linknumber the FLAC parser is in */
	uint32_t current_linknumber_advance_read; /* The linknumber the ogg parser is in. The name 'advance read' is because it reads ahead, to see whether there is another link */
	FLAC__bool is_seeking;
} FLAC__OggDecoderAspect;

void FLAC__ogg_decoder_aspect_set_serial_number(FLAC__OggDecoderAspect *aspect, long value);
void FLAC__ogg_decoder_aspect_set_defaults(FLAC__OggDecoderAspect *aspect);
FLAC__bool FLAC__ogg_decoder_aspect_init(FLAC__OggDecoderAspect *aspect);
void FLAC__ogg_decoder_aspect_finish(FLAC__OggDecoderAspect *aspect);
void FLAC__ogg_decoder_aspect_flush(FLAC__OggDecoderAspect *aspect);
void FLAC__ogg_decoder_aspect_reset(FLAC__OggDecoderAspect* aspect);
void FLAC__ogg_decoder_aspect_next_link(FLAC__OggDecoderAspect* aspect);
FLAC__OggDecoderAspect_TargetLink * FLAC__ogg_decoder_aspect_get_target_link(FLAC__OggDecoderAspect* aspect, FLAC__uint64 target_sample);
void FLAC__ogg_decoder_aspect_set_decode_chained_stream(FLAC__OggDecoderAspect* aspect, FLAC__bool value);
FLAC__bool FLAC__ogg_decoder_aspect_get_decode_chained_stream(FLAC__OggDecoderAspect* aspect);
void FLAC__ogg_decoder_aspect_set_seek_parameters(FLAC__OggDecoderAspect *aspect, FLAC__OggDecoderAspect_TargetLink *target_link);

typedef enum {
	FLAC__OGG_DECODER_ASPECT_READ_STATUS_OK = 0,
	FLAC__OGG_DECODER_ASPECT_READ_STATUS_END_OF_STREAM,
	FLAC__OGG_DECODER_ASPECT_READ_STATUS_END_OF_LINK,
	FLAC__OGG_DECODER_ASPECT_READ_STATUS_LOST_SYNC,
	FLAC__OGG_DECODER_ASPECT_READ_STATUS_NOT_FLAC,
	FLAC__OGG_DECODER_ASPECT_READ_STATUS_UNSUPPORTED_MAPPING_VERSION,
	FLAC__OGG_DECODER_ASPECT_READ_STATUS_ABORT,
	FLAC__OGG_DECODER_ASPECT_READ_STATUS_ERROR,
	FLAC__OGG_DECODER_ASPECT_READ_STATUS_MEMORY_ALLOCATION_ERROR,
	FLAC__OGG_DECODER_ASPECT_READ_STATUS_CALLBACKS_NONFUNCTIONAL
} FLAC__OggDecoderAspectReadStatus;

typedef FLAC__OggDecoderAspectReadStatus (*FLAC__OggDecoderAspectReadCallbackProxy)(const void *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data);

FLAC__OggDecoderAspectReadStatus FLAC__ogg_decoder_aspect_read_callback_wrapper(FLAC__OggDecoderAspect *aspect, FLAC__byte buffer[], size_t *bytes, FLAC__OggDecoderAspectReadCallbackProxy read_callback, FLAC__StreamDecoderTellCallback tell_callback, const FLAC__StreamDecoder *decoder, void *client_data);
FLAC__OggDecoderAspectReadStatus FLAC__ogg_decoder_aspect_skip_link(FLAC__OggDecoderAspect *aspect, FLAC__OggDecoderAspectReadCallbackProxy read_callback, FLAC__StreamDecoderSeekCallback seek_callback, FLAC__StreamDecoderTellCallback tell_callback, FLAC__StreamDecoderLengthCallback length_callback, const FLAC__StreamDecoder *decoder, void *client_data);
#endif

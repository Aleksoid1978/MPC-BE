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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h> /* for memcpy() */
#include "FLAC/assert.h"
#include "share/alloc.h" /* for free() */
#include "private/ogg_decoder_aspect.h"
#include "private/ogg_mapping.h"
#include "private/macros.h"

static FLAC__OggDecoderAspectReadStatus read_more_data_(FLAC__OggDecoderAspect *aspect, FLAC__OggDecoderAspectReadCallbackProxy read_callback, size_t bytes_requested, const FLAC__StreamDecoder *decoder, void *client_data);

/***********************************************************************
 *
 * Public class methods
 *
 ***********************************************************************/

static FLAC__OggDecoderAspectReadStatus read_more_data_(FLAC__OggDecoderAspect *aspect, FLAC__OggDecoderAspectReadCallbackProxy read_callback, size_t bytes_requested, const FLAC__StreamDecoder *decoder, void *client_data)
{
	static const size_t OGG_BYTES_CHUNK = 8192;
	const size_t ogg_bytes_to_read = flac_max(bytes_requested, OGG_BYTES_CHUNK);
	char *oggbuf = ogg_sync_buffer(&aspect->sync_state, ogg_bytes_to_read);

	if(0 == oggbuf) {
		return FLAC__OGG_DECODER_ASPECT_READ_STATUS_MEMORY_ALLOCATION_ERROR;
	}
	else {
		size_t ogg_bytes_read = ogg_bytes_to_read;

		switch(read_callback(decoder, (FLAC__byte*)oggbuf, &ogg_bytes_read, client_data)) {
			case FLAC__OGG_DECODER_ASPECT_READ_STATUS_OK:
				break;
			case FLAC__OGG_DECODER_ASPECT_READ_STATUS_END_OF_STREAM:
				aspect->end_of_stream = true;
				break;
			case FLAC__OGG_DECODER_ASPECT_READ_STATUS_ABORT:
				return FLAC__OGG_DECODER_ASPECT_READ_STATUS_ABORT;
			default:
				FLAC__ASSERT(0);
		}

		if(ogg_sync_wrote(&aspect->sync_state, ogg_bytes_read) < 0) {
			/* double protection; this will happen if the read callback returns more bytes than the max requested, which would overflow Ogg's internal buffer */
			FLAC__ASSERT(0);
			return FLAC__OGG_DECODER_ASPECT_READ_STATUS_ERROR;
		}
	}
	return FLAC__OGG_DECODER_ASPECT_READ_STATUS_OK;
}

static FLAC__OggDecoderAspectReadStatus process_page_(FLAC__OggDecoderAspect *aspect, FLAC__StreamDecoderTellCallback tell_callback, const FLAC__StreamDecoder *decoder, void *client_data)
{
	/* got a page, grab the serial number if necessary */
	if(aspect->need_serial_number) {
		/* Check whether not FLAC. The next if is somewhat confusing: check
		 * whether the length of the next page body agrees with the length
		 * of a FLAC 'header' possibly contained in that page */
		if(aspect->working_page.body_len > (long)(1 + FLAC__OGG_MAPPING_MAGIC_LENGTH) &&
		   aspect->working_page.body[0] == FLAC__OGG_MAPPING_FIRST_HEADER_PACKET_TYPE &&
		   memcmp((&aspect->working_page.body) + 1, FLAC__OGG_MAPPING_MAGIC, FLAC__OGG_MAPPING_MAGIC_LENGTH)) {
			aspect->bos_flag_seen = true;
			aspect->serial_number = ogg_page_serialno(&aspect->working_page);
			ogg_stream_reset_serialno(&aspect->stream_state, aspect->serial_number);
			aspect->need_serial_number = false;

			if(aspect->current_linknumber_advance_read >= aspect->number_of_links_detected) {
				FLAC__uint64 tell_offset;
				aspect->number_of_links_detected = aspect->current_linknumber_advance_read + 1;
				aspect->linkdetails[aspect->current_linknumber_advance_read].serial_number = aspect->serial_number;
				if(tell_callback != 0) {
					if(tell_callback(decoder, &tell_offset, client_data) == FLAC__STREAM_DECODER_TELL_STATUS_OK)
						aspect->linkdetails[aspect->current_linknumber_advance_read].start_byte = tell_offset - aspect->sync_state.fill + aspect->sync_state.returned
						                                                                          - aspect->working_page.header_len - aspect->working_page.body_len;
				}
			}
		}
	}
	if(aspect->beginning_of_link) {
		if(aspect->bos_flag_seen && !ogg_page_bos(&aspect->working_page)) {
			/* Page does not have BOS flag, which means we're done scanning for other serial numbers */
			aspect->beginning_of_link = false;
		}
	}
	if(ogg_stream_pagein(&aspect->stream_state, &aspect->working_page) == 0) {
		aspect->have_working_page = true;
		aspect->have_working_packet = false;
	}
	else if(aspect->beginning_of_link) {
		/* At the beginning of a link, store the serial numbers of all other streams, to make
		 * finding the end of a link through seeking possible */
		if(ogg_page_bos(&aspect->working_page)) {
			aspect->bos_flag_seen = true;
			if(aspect->current_linknumber_advance_read >= aspect->number_of_links_indexed) {
				FLAC__OggDecoderAspect_LinkDetails * current_link = &aspect->linkdetails[aspect->current_linknumber_advance_read];
				/* Reallocate in chunks of 4 */
				if((current_link->number_of_other_streams) % 4 == 0) {
					long * tmpptr = NULL;
					if(NULL == (tmpptr = safe_realloc_nofree_mul_2op_(current_link->other_serial_numbers, 4+current_link->number_of_other_streams, sizeof(long)))) {
						return FLAC__OGG_DECODER_ASPECT_READ_STATUS_MEMORY_ALLOCATION_ERROR;
					}
					current_link->other_serial_numbers = tmpptr;
				}
				current_link->other_serial_numbers[current_link->number_of_other_streams] = ogg_page_serialno(&aspect->working_page);
				current_link->number_of_other_streams++;
			}
		}
		/* No BOS flag seen yet, these pages might be still from previous link */
	}
	/* else do nothing, could be a page from another stream */
	return FLAC__OGG_DECODER_ASPECT_READ_STATUS_OK;
}

static FLAC__bool check_size_of_link_allocation_(FLAC__OggDecoderAspect *aspect)
{
	/* double on reallocating */
	if(aspect->current_linknumber >= aspect->number_of_links_allocated || aspect->current_linknumber_advance_read >= aspect->number_of_links_allocated) {
		FLAC__OggDecoderAspect_LinkDetails * tmpptr = NULL;
		if(NULL == (tmpptr = safe_realloc_nofree_mul_2op_(aspect->linkdetails,2*aspect->number_of_links_allocated,sizeof(FLAC__OggDecoderAspect_LinkDetails)))) {
			return false;
		}
		aspect->linkdetails = tmpptr;
		memset(aspect->linkdetails + aspect->number_of_links_allocated, 0, aspect->number_of_links_allocated * sizeof(FLAC__OggDecoderAspect_LinkDetails));
		aspect->number_of_links_allocated *= 2;
	}
	return true;
}

FLAC__bool FLAC__ogg_decoder_aspect_init(FLAC__OggDecoderAspect *aspect)
{
	/* we will determine the serial number later if necessary */
	if(ogg_stream_init(&aspect->stream_state, aspect->serial_number) != 0)
		return false;

	if(ogg_sync_init(&aspect->sync_state) != 0)
		return false;

	aspect->version_major = ~(0u);
	aspect->version_minor = ~(0u);

	aspect->need_serial_number = aspect->use_first_serial_number || aspect->decode_chained_stream;

	aspect->end_of_stream = false;
	aspect->have_working_page = false;
	aspect->end_of_link = false;

	aspect->current_linknumber = 0;
	aspect->current_linknumber_advance_read = 0;
	aspect->number_of_links_indexed = 0;
	aspect->number_of_links_detected = 0;
	aspect->number_of_links_allocated = 0;

	if(NULL == (aspect->linkdetails = safe_realloc_mul_2op_(NULL,4,sizeof(FLAC__OggDecoderAspect_LinkDetails))))
		return false;
	memset(aspect->linkdetails, 0, 4 * sizeof(FLAC__OggDecoderAspect_LinkDetails));

	aspect->number_of_links_allocated = 4;

	return true;
}

void FLAC__ogg_decoder_aspect_finish(FLAC__OggDecoderAspect *aspect)
{
	uint32_t i;
	(void)ogg_sync_clear(&aspect->sync_state);
	(void)ogg_stream_clear(&aspect->stream_state);
	if(NULL != aspect->linkdetails) {
		for(i = 0; i < aspect->number_of_links_allocated; i++)
			free(aspect->linkdetails[i].other_serial_numbers);
		free(aspect->linkdetails);
	}
	aspect->linkdetails = NULL;
}

void FLAC__ogg_decoder_aspect_set_serial_number(FLAC__OggDecoderAspect *aspect, long value)
{
	aspect->use_first_serial_number = false;
	aspect->serial_number = value;
}

void FLAC__ogg_decoder_aspect_set_defaults(FLAC__OggDecoderAspect *aspect)
{
	aspect->use_first_serial_number = true;
	aspect->decode_chained_stream = false;
}

void FLAC__ogg_decoder_aspect_flush(FLAC__OggDecoderAspect *aspect)
{
	(void)ogg_stream_reset(&aspect->stream_state);
	(void)ogg_sync_reset(&aspect->sync_state);
	aspect->end_of_stream = false;
	aspect->have_working_page = false;
	aspect->end_of_link = false;
}

void FLAC__ogg_decoder_aspect_reset(FLAC__OggDecoderAspect *aspect)
{
	FLAC__ogg_decoder_aspect_flush(aspect);
	aspect->current_linknumber = 0;
	aspect->current_linknumber_advance_read = 0;

	if(aspect->use_first_serial_number || aspect->decode_chained_stream)
		aspect->need_serial_number = true;

	aspect->beginning_of_link = true;
	aspect->bos_flag_seen = false;
}

void FLAC__ogg_decoder_aspect_next_link(FLAC__OggDecoderAspect* aspect)
{
	aspect->end_of_link = false;
	aspect->current_linknumber++;
	aspect->beginning_of_link = true;
	aspect->bos_flag_seen = false;
}

void FLAC__ogg_decoder_aspect_set_decode_chained_stream(FLAC__OggDecoderAspect* aspect, FLAC__bool value)
{
	aspect->decode_chained_stream = value;
}

FLAC__bool FLAC__ogg_decoder_aspect_get_decode_chained_stream(FLAC__OggDecoderAspect* aspect)
{
	return aspect->decode_chained_stream;
}

FLAC__OggDecoderAspect_TargetLink * FLAC__ogg_decoder_aspect_get_target_link(FLAC__OggDecoderAspect* aspect, FLAC__uint64 target_sample)
{
	/* This returns the link containing the seek target if known. In
	 * effect, this function always returns NULL if no links have been
	 * indexed */

	uint32_t current_link = 0;
	uint32_t total_samples = 0;

	while(current_link < aspect->number_of_links_indexed) {
		total_samples += aspect->linkdetails[current_link].samples;
		if(target_sample < total_samples) {
			aspect->target_link.serial_number = aspect->linkdetails[current_link].serial_number;
			aspect->target_link.start_byte = aspect->linkdetails[current_link].start_byte;
			aspect->target_link.samples_in_preceding_links = total_samples - aspect->linkdetails[current_link].samples;
			aspect->target_link.end_byte = aspect->linkdetails[current_link].end_byte;
			aspect->target_link.samples_this_link = aspect->linkdetails[current_link].samples;
			aspect->target_link.linknumber = current_link;
			return &aspect->target_link;
		}
		current_link++;
	}
	return NULL;
}

void FLAC__ogg_decoder_aspect_set_seek_parameters(FLAC__OggDecoderAspect *aspect, FLAC__OggDecoderAspect_TargetLink *target_link)
{
	if(target_link == 0) {
		aspect->is_seeking = false;
	}
	else {
		aspect->need_serial_number = false;
		aspect->current_linknumber = target_link->linknumber;
		aspect->current_linknumber_advance_read = target_link->linknumber;
		aspect->serial_number = target_link->serial_number;
		ogg_stream_reset_serialno(&aspect->stream_state, aspect->serial_number);
		aspect->is_seeking = true;
	}
}


FLAC__OggDecoderAspectReadStatus FLAC__ogg_decoder_aspect_read_callback_wrapper(FLAC__OggDecoderAspect *aspect, FLAC__byte buffer[], size_t *bytes, FLAC__OggDecoderAspectReadCallbackProxy read_callback, FLAC__StreamDecoderTellCallback tell_callback, const FLAC__StreamDecoder *decoder, void *client_data)
{
	const size_t bytes_requested = *bytes;

	const uint32_t header_length =
		FLAC__OGG_MAPPING_PACKET_TYPE_LENGTH +
		FLAC__OGG_MAPPING_MAGIC_LENGTH +
		FLAC__OGG_MAPPING_VERSION_MAJOR_LENGTH +
		FLAC__OGG_MAPPING_VERSION_MINOR_LENGTH +
		FLAC__OGG_MAPPING_NUM_HEADERS_LENGTH;

	/*
	 * The FLAC decoding API uses pull-based reads, whereas Ogg decoding
	 * is push-based.  In libFLAC, when you ask to decode a frame, the
	 * decoder will eventually call the read callback to supply some data,
	 * but how much it asks for depends on how much free space it has in
	 * its internal buffer.  It does not try to grow its internal buffer
	 * to accommodate a whole frame because then the internal buffer size
	 * could not be limited, which is necessary in embedded applications.
	 *
	 * Ogg however grows its internal buffer until a whole page is present;
	 * only then can you get decoded data out.  So we can't just ask for
	 * the same number of bytes from Ogg, then pass what's decoded down to
	 * libFLAC.  If what libFLAC is asking for will not contain a whole
	 * page, then we will get no data from ogg_sync_pageout(), and at the
	 * same time cannot just read more data from the client for the purpose
	 * of getting a whole decoded page because the decoded size might be
	 * larger than libFLAC's internal buffer.
	 *
	 * Instead, whenever this read callback wrapper is called, we will
	 * continually request data from the client until we have at least one
	 * page, and manage pages internally so that we can send pieces of
	 * pages down to libFLAC in such a way that we obey its size
	 * requirement.  To limit the amount of callbacks, we will always try
	 * to read in enough pages to return the full number of bytes
	 * requested.
	 */
	*bytes = 0;
	while (*bytes < bytes_requested && !aspect->end_of_stream) {
		if (aspect->end_of_link && aspect->have_working_page) {
			/* we've now consumed all packets of this link and have checked that a new page follows it */
			if(*bytes > 0)
				return FLAC__OGG_DECODER_ASPECT_READ_STATUS_OK;
			else
				return FLAC__OGG_DECODER_ASPECT_READ_STATUS_END_OF_LINK;
		}
		else if (aspect->have_working_page) {
			if (aspect->have_working_packet) {
				size_t n = bytes_requested - *bytes;
				if ((size_t)aspect->working_packet.bytes <= n) {
					/* the rest of the packet will fit in the buffer */
					n = aspect->working_packet.bytes;
					memcpy(buffer, aspect->working_packet.packet, n);
					*bytes += n;
					buffer += n;
					aspect->have_working_packet = false;
					if(aspect->working_packet.e_o_s) {
						if(!aspect->decode_chained_stream)
							aspect->end_of_stream = true;
						else {
							aspect->end_of_link = true;
							aspect->current_linknumber_advance_read = aspect->current_linknumber + 1;
							if(!check_size_of_link_allocation_(aspect))
								return FLAC__OGG_DECODER_ASPECT_READ_STATUS_MEMORY_ALLOCATION_ERROR;
							if(aspect->current_linknumber >= aspect->number_of_links_indexed) {
								FLAC__uint64 tell_offset;
								FLAC__ASSERT(aspect->current_linknumber == aspect->number_of_links_indexed);
								aspect->linkdetails[aspect->current_linknumber].samples = aspect->working_packet.granulepos;
								if(tell_callback != 0) {
									if(tell_callback(decoder, &tell_offset, client_data) == FLAC__STREAM_DECODER_TELL_STATUS_OK)
										aspect->linkdetails[aspect->current_linknumber].end_byte = tell_offset - aspect->sync_state.fill + aspect->sync_state.returned;
								}
								aspect->number_of_links_indexed++;
								aspect->need_serial_number = true;
							}
							if(!aspect->is_seeking)
								aspect->need_serial_number = true;
							aspect->have_working_page = false; /* e-o-s packet ends page */
						}
					}
				}
				else {
					/* only n bytes of the packet will fit in the buffer */
					memcpy(buffer, aspect->working_packet.packet, n);
					*bytes += n;
					buffer += n;
					aspect->working_packet.packet += n;
					aspect->working_packet.bytes -= n;
				}
			}
			else {
				/* try and get another packet */
				const int ret = ogg_stream_packetout(&aspect->stream_state, &aspect->working_packet);
				if (ret > 0) {
					aspect->have_working_packet = true;
					/* if it is the first header packet, check for magic and a supported Ogg FLAC mapping version */
					if (aspect->working_packet.bytes > 0 && aspect->working_packet.packet[0] == FLAC__OGG_MAPPING_FIRST_HEADER_PACKET_TYPE) {
						const FLAC__byte *b = aspect->working_packet.packet;
						if (aspect->working_packet.bytes < (long)header_length)
							return FLAC__OGG_DECODER_ASPECT_READ_STATUS_NOT_FLAC;
						b += FLAC__OGG_MAPPING_PACKET_TYPE_LENGTH;
						if (memcmp(b, FLAC__OGG_MAPPING_MAGIC, FLAC__OGG_MAPPING_MAGIC_LENGTH))
							return FLAC__OGG_DECODER_ASPECT_READ_STATUS_NOT_FLAC;
						b += FLAC__OGG_MAPPING_MAGIC_LENGTH;
						aspect->version_major = (uint32_t)(*b);
						b += FLAC__OGG_MAPPING_VERSION_MAJOR_LENGTH;
						aspect->version_minor = (uint32_t)(*b);
						if (aspect->version_major != 1)
							return FLAC__OGG_DECODER_ASPECT_READ_STATUS_UNSUPPORTED_MAPPING_VERSION;
						aspect->working_packet.packet += header_length;
						aspect->working_packet.bytes -= header_length;
					}
				}
				else if (ret == 0) {
					aspect->have_working_page = false;
				}
				else { /* ret < 0 */
					/* lost sync, we'll leave the working page for the next call */
					return FLAC__OGG_DECODER_ASPECT_READ_STATUS_LOST_SYNC;
				}
			}
		}
		else {
			/* try and get another page */
			const int ret = ogg_sync_pageout(&aspect->sync_state, &aspect->working_page);
			if (ret > 0) {
				FLAC__OggDecoderAspectReadStatus status = process_page_(aspect, tell_callback, decoder, client_data);
				if(status != FLAC__OGG_DECODER_ASPECT_READ_STATUS_OK)
					return status;
			}
			else if (ret == 0) {
				/* need more data */
				FLAC__OggDecoderAspectReadStatus status = read_more_data_(aspect, read_callback, bytes_requested - *bytes, decoder, client_data);
				if(status != FLAC__OGG_DECODER_ASPECT_READ_STATUS_OK)
					return status;
			}
			else { /* ret < 0 */
				/* lost sync, bail out */
				return FLAC__OGG_DECODER_ASPECT_READ_STATUS_LOST_SYNC;
			}
		}
	}

	if (aspect->end_of_stream && *bytes == 0) {
		aspect->linkdetails[aspect->current_linknumber].is_last = true;
		return FLAC__OGG_DECODER_ASPECT_READ_STATUS_END_OF_STREAM;
	}

	return FLAC__OGG_DECODER_ASPECT_READ_STATUS_OK;
}

FLAC__OggDecoderAspectReadStatus FLAC__ogg_decoder_aspect_skip_link(FLAC__OggDecoderAspect *aspect, FLAC__OggDecoderAspectReadCallbackProxy read_callback, FLAC__StreamDecoderSeekCallback seek_callback, FLAC__StreamDecoderTellCallback tell_callback, FLAC__StreamDecoderLengthCallback length_callback, const FLAC__StreamDecoder *decoder, void *client_data)
{
	if(seek_callback == NULL || tell_callback == NULL || length_callback == NULL)
		return FLAC__OGG_DECODER_ASPECT_READ_STATUS_CALLBACKS_NONFUNCTIONAL;

	/* This extra check is here, because allocation failures while reading cannot always be
	 * properly passed down the chain with the current API. So, instead, check again */
	if(!check_size_of_link_allocation_(aspect))
		return FLAC__OGG_DECODER_ASPECT_READ_STATUS_MEMORY_ALLOCATION_ERROR;

	if(aspect->current_linknumber < aspect->number_of_links_indexed) {
		if(aspect->linkdetails[aspect->current_linknumber].is_last) {
			/* Seek to end of stream */
			FLAC__StreamDecoderLengthStatus lstatus;
			FLAC__StreamDecoderSeekStatus sstatus;
			uint64_t stream_length = 0;

			lstatus = length_callback(decoder, &stream_length, client_data);
			if(lstatus == FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED)
				return FLAC__OGG_DECODER_ASPECT_READ_STATUS_CALLBACKS_NONFUNCTIONAL;
			if(lstatus == FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR)
				return FLAC__OGG_DECODER_ASPECT_READ_STATUS_ERROR;

			sstatus = seek_callback(decoder, stream_length, client_data);
			if(sstatus == FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED)
				return FLAC__OGG_DECODER_ASPECT_READ_STATUS_CALLBACKS_NONFUNCTIONAL;
			if(sstatus == FLAC__STREAM_DECODER_SEEK_STATUS_ERROR)
				return FLAC__OGG_DECODER_ASPECT_READ_STATUS_ERROR;

			return FLAC__OGG_DECODER_ASPECT_READ_STATUS_END_OF_STREAM;

		}
		else {
			FLAC__StreamDecoderSeekStatus status;
			status = seek_callback(decoder, aspect->linkdetails[aspect->current_linknumber].end_byte,client_data);
			if(status == FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED)
				return FLAC__OGG_DECODER_ASPECT_READ_STATUS_CALLBACKS_NONFUNCTIONAL;
			if(status == FLAC__STREAM_DECODER_SEEK_STATUS_ERROR)
				return FLAC__OGG_DECODER_ASPECT_READ_STATUS_ERROR;
			FLAC__ogg_decoder_aspect_flush(aspect);
			aspect->beginning_of_link = true;
			aspect->need_serial_number = true;
			aspect->bos_flag_seen = false;
			aspect->current_linknumber++;
			aspect->current_linknumber_advance_read = aspect->current_linknumber;
			if(!check_size_of_link_allocation_(aspect))
				return FLAC__OGG_DECODER_ASPECT_READ_STATUS_MEMORY_ALLOCATION_ERROR;
			aspect->have_working_page = false;
			return FLAC__OGG_DECODER_ASPECT_READ_STATUS_OK;
		}
	}
	else
	{
		/* End of current link is unknown, go search for it */
		const uint32_t max_page_size = 65307;
		uint64_t stream_length = 0;
		uint64_t current_pos = 0;
		uint64_t page_pos = 0;
		uint64_t target_pos = 0;
		uint64_t left_pos = 0;
		uint64_t right_pos = 0;
		FLAC__bool did_a_seek;
		FLAC__bool seek_to_left_pos = false;
		FLAC__bool keep_reading = false;
		FLAC__bool find_bos_twice = aspect->need_serial_number;
		int ret = 0;

		{
			FLAC__StreamDecoderLengthStatus lstatus;
			FLAC__StreamDecoderTellStatus tstatus;

			lstatus = length_callback(decoder, &stream_length, client_data);
			if(lstatus == FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED)
				return FLAC__OGG_DECODER_ASPECT_READ_STATUS_CALLBACKS_NONFUNCTIONAL;
			if(lstatus == FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR)
				return FLAC__OGG_DECODER_ASPECT_READ_STATUS_ERROR;

			tstatus = tell_callback(decoder, &current_pos, client_data);
			if(tstatus == FLAC__STREAM_DECODER_TELL_STATUS_UNSUPPORTED)
				return FLAC__OGG_DECODER_ASPECT_READ_STATUS_CALLBACKS_NONFUNCTIONAL;
			if(tstatus == FLAC__STREAM_DECODER_TELL_STATUS_ERROR)
				return FLAC__OGG_DECODER_ASPECT_READ_STATUS_ERROR;
		}


		current_pos = current_pos - aspect->sync_state.fill + aspect->sync_state.returned;
		left_pos = current_pos;
		right_pos = stream_length;

		while(1){
			FLAC__bool seek_was_to_current_link = true;

			if(right_pos <= left_pos || right_pos - left_pos < 9) {
				/* FLAC frame is at least 9 byte in size */
				return FLAC__OGG_DECODER_ASPECT_READ_STATUS_ERROR;
			}
			target_pos = left_pos + (right_pos - left_pos)/2;

			if(keep_reading) {
				/* To not end up in a loop, we need to keep reading until are able to change left_pos */
				did_a_seek = false;
			}
			else if(current_pos < target_pos && current_pos + aspect->sync_state.fill - aspect->sync_state.returned > target_pos) {
				/* Target location is already in buffer, keep reading */
				did_a_seek = false;
			}
			else if(current_pos < target_pos && current_pos + max_page_size > target_pos) {
				/* Target is very close to current location, just reading is probably faster than seeking */
				did_a_seek = false;
			}
			else if(aspect->beginning_of_link) {
				/* Still need to save all serial numbers at start of stream, so don't seek yet */
				did_a_seek = false;
			}
			else {
				if(seek_to_left_pos || target_pos - left_pos < max_page_size) {
					/* Seek to start of bisect area */
					target_pos = left_pos;
					keep_reading = true;
					seek_to_left_pos = false;
				}
				/* Seek */
				if(seek_callback(decoder, target_pos, client_data) != FLAC__STREAM_DECODER_SEEK_STATUS_OK)
					return false;
				did_a_seek = true;
				FLAC__ASSERT(tell_callback(decoder, &current_pos, client_data) == FLAC__STREAM_DECODER_TELL_STATUS_OK);
				FLAC__ASSERT(current_pos == target_pos);
				current_pos = target_pos;
				(void)ogg_stream_reset(&aspect->stream_state);
				(void)ogg_sync_reset(&aspect->sync_state);
			}

			/* Get a page, resynchronize if necessary */
			while((ret = ogg_sync_pageseek(&aspect->sync_state, &aspect->working_page)) <= 0 && !aspect->end_of_stream) {
				if(ret < 0)
					current_pos -= ret;
				else {
					/* need more data */
					FLAC__OggDecoderAspectReadStatus status = read_more_data_(aspect, read_callback, 0, decoder, client_data);
					if(status != FLAC__OGG_DECODER_ASPECT_READ_STATUS_OK)
						return status;
				}
			}

			page_pos = current_pos;
			current_pos += aspect->working_page.header_len + aspect->working_page.body_len;

			/* Check whether the page serial number belongs to this link or another link */
			if (ret > 0) {
				if(!aspect->beginning_of_link) {
					/* got a page, check the serial number */
					long serial_number = ogg_page_serialno(&aspect->working_page);
					uint32_t i;
					seek_was_to_current_link = false;
					if(serial_number == aspect->linkdetails[aspect->current_linknumber].serial_number) {
						/* This page belongs to current link */
						seek_was_to_current_link = true;
					}
					for(i = 0; i < aspect->linkdetails[aspect->current_linknumber].number_of_other_streams; i++) {
						if(serial_number == aspect->linkdetails[aspect->current_linknumber].other_serial_numbers[i]) {
							/* This page belongs to the current link */
							seek_was_to_current_link = true;
						}
					}
					if(ogg_page_serialno(&aspect->working_page) == aspect->linkdetails[aspect->current_linknumber].serial_number && ogg_page_eos(&aspect->working_page)) {
						/* Found EOS */
						aspect->linkdetails[aspect->current_linknumber].end_byte = current_pos;
						aspect->linkdetails[aspect->current_linknumber].samples = ogg_page_granulepos(&aspect->working_page);

						aspect->number_of_links_indexed++;
						aspect->current_linknumber_advance_read = aspect->current_linknumber + 1;
						if(!check_size_of_link_allocation_(aspect))
							return FLAC__OGG_DECODER_ASPECT_READ_STATUS_MEMORY_ALLOCATION_ERROR;

						aspect->need_serial_number = true;

						/* Now, continue loop to check whether we are at end of stream or another link follows */
						FLAC__ogg_decoder_aspect_next_link(aspect);
						continue;
					}
					if(seek_was_to_current_link) {
						/* If the seek landed on a page with the serial number of the stream we're interested in, we can be sure
						 * the EOS page will be later in the stream. If the seek landed on a stream with a different serial number
						 * however, we cannot be sure. It could be the EOS page of that stream has already been seen. In that case
						 * something else needs to be done, else we could end up in an endless loop */
						if(ogg_page_serialno(&aspect->working_page) == aspect->linkdetails[aspect->current_linknumber].serial_number) {
							left_pos = current_pos;
							keep_reading = false;
						}
						else {
							seek_to_left_pos = true;
						}
					}
					else if(keep_reading) {
						/* We read from the left_pos but found nothing interesting, so we can move left_pos up */
						left_pos = current_pos;
					}
					else if(did_a_seek) {
						if(right_pos <= page_pos) {
							/* Ended up somewhere we've already been */
							seek_to_left_pos = true;
						}
						else
							right_pos = page_pos;
					}
					else {
						/* Read forward but found an unknown serial number */
						return FLAC__OGG_DECODER_ASPECT_READ_STATUS_ERROR;
					}
				}
				else { /* aspect->beginning_of_link == true */
					if(aspect->end_of_stream) {
						if(aspect->current_linknumber == 0)
							return FLAC__OGG_DECODER_ASPECT_READ_STATUS_LOST_SYNC;
						aspect->current_linknumber--;
						aspect->linkdetails[aspect->current_linknumber].is_last = true;
						return FLAC__OGG_DECODER_ASPECT_READ_STATUS_END_OF_STREAM;
					}
					else {
						/* We can end up here for three reasons:
						 * 1) We've found the end of the link and are looking whether a new one follows it. If that is the case,
						 *    we need to finish after we found it
						 * 2) We've just started skipping this link, and need to look for other BOS pages first. If that is the case,
						 *    we need to continue after we found them
						 * 3) We've don't know anything about the link that needs to be skipped yet, and we need to process the BOS
						 *    pages first, and process the BOS pages of the next link */
						FLAC__bool need_to_finish = aspect->need_serial_number && !find_bos_twice;
						FLAC__OggDecoderAspectReadStatus status = process_page_(aspect, tell_callback, decoder, client_data);
						if(status != FLAC__OGG_DECODER_ASPECT_READ_STATUS_OK)
							return status;
						if(!aspect->need_serial_number) {
							if(need_to_finish) {
								/* Found start of next link, we're done */
								return FLAC__OGG_DECODER_ASPECT_READ_STATUS_OK;
							}
							find_bos_twice = false;
						}
						if(!aspect->beginning_of_link) {
							/* Done scanning BOS pages, move up left_pos */
							left_pos = page_pos;
						}
					}
				}
			}
			else if(aspect->end_of_stream) {
				if(aspect->beginning_of_link && !aspect->bos_flag_seen) {
					/* We were looking for the next link, but found end of stream instead */
					if(aspect->current_linknumber == 0)
						return FLAC__OGG_DECODER_ASPECT_READ_STATUS_LOST_SYNC;
					aspect->current_linknumber--;
					aspect->linkdetails[aspect->current_linknumber].is_last = true;
					return FLAC__OGG_DECODER_ASPECT_READ_STATUS_END_OF_STREAM;
				}
				else if(did_a_seek) {
					/* Seeking to target_pos did no result in finding a page, set right_pos to that value */
					right_pos = target_pos;
				}
				else {
					/* We expected to find the EOS page without seeking, but ended up at the end of the stream */
					return FLAC__OGG_DECODER_ASPECT_READ_STATUS_ERROR;
				}
			}
			else if(ret == 0) {
				/* ogg error */
				return FLAC__OGG_DECODER_ASPECT_READ_STATUS_LOST_SYNC;
			}
		}
	}
}

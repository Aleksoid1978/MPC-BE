/*
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVCODEC_BSF_FILTERS_H
#define AVCODEC_BSF_FILTERS_H

#include "libavutil/log.h"
#include "libavutil/rational.h"

#include "libavcodec/bsf.h"
#include "libavcodec/codec_par.h"
#include "libavcodec/packet.h"

/**
 * Special return code when activate() did not do anything.
 */
#define FFERROR_BSF_NOT_READY FFERRTAG('N','R','D','Y')
#define FFERROR_SOURCE_EMPTY FFERRTAG('M','P','T','Y')

struct AVBitStreamFilterPad {
    /**
     * Pad name. The name is unique among inputs and among outputs, but an
     * input may have the same name as an output. This may be NULL if this
     * pad has no need to ever be referenced by name.
     */
    const char *name;

    /**
     * A list of codec ids supported by the pad, terminated by
     * AV_CODEC_ID_NONE.
     * May be NULL, in that case the pad works with any codec id.
     */
    const enum AVCodecID *codec_ids;

    /**
     * The filter expects writable packets from its input link,
     * duplicating data buffers if needed.
     *
     * input pads only.
     */
#define FF_BSF_PAD_FLAG_NEEDS_WRITABLE                  (1 << 0)

    /**
     * The pad's name is allocated and should be freed generically.
     */
#define FF_BSF_PAD_FLAG_FREE_NAME                       (1 << 1)

    /**
     * A combination of FF_BSF_PAD_FLAG_* flags.
     */
    int flags;

    /**
     * Filtering callback. This is where a filter receives a packet with
     * audio/video data and should do its processing.
     *
     * Input pads only.
     *
     * @return >= 0 on success, a negative AVERROR on error. This function
     * must ensure that packet is properly unreferenced on error if it
     * hasn't been passed on to another filter.
     */
    int (*filter)(AVBitStreamFilterLink *link, AVPacket *pkt);

    /**
     * Packet request callback. A call to this should result in some progress
     * towards producing output over the given link. This should return zero
     * on success, and another value on error.
     *
     * Output pads only.
     */
    int (*request_packet)(AVBitStreamFilterLink *link);

    /**
     * Link configuration callback.
     *
     * For output pads, this should set the link properties such as
     * width/height.
     *
     * For input pads, this should check the properties of the link, and update
     * the filter's internal state as necessary.
     *
     * For both input and output filters, this should return zero on success,
     * and another value on error.
     */
    int (*config_props)(AVBitStreamFilterLink *link);
};

/**
 * Link properties exposed to filter code, but not external callers.
 */
typedef struct AVBitStreamFilterLink {
    AVBitStreamFilterContext *src; ///< source filter
    AVBitStreamFilterPad  *srcpad; ///< output pad on the source filter

    AVBitStreamFilterContext *dst; ///< dest filter
    AVBitStreamFilterPad  *dstpad; ///< input pad on the dest filter

    /**
     * Graph the filter belongs to.
     */
    struct AVBitStreamFilterGraph *graph;


    AVCodecParameters *par;

    AVRational time_base;

    /**
     * Current timestamp of the link, as defined by the most recent
     * packet(s), in link time_base units.
     */
    int64_t current_pts;

    /**
     * Current timestamp of the link, as defined by the most recent
     * packet(s), in AV_TIME_BASE units.
     */
    int64_t current_pts_us;

    /**
     * Number of past packets sent through the link.
     */
    int64_t packet_count_in, packet_count_out;
} AVBitStreamFilterLink;

#define BSFILTER_INOUTPADS(inout, array) \
       .inout        = array, \
       .nb_ ## inout = FF_ARRAY_ELEMS(array)
#define BSFILTER_INPUTS(array)  BSFILTER_INOUTPADS(inputs,  (array))
#define BSFILTER_OUTPUTS(array) BSFILTER_INOUTPADS(outputs, (array))

extern const AVBitStreamFilterPad ff_default_bsf_pad[1];

#define BSF_DEFINE_CLASS_EXT(name, desc, options) \
    static const AVClass name##_class = {       \
        .class_name = desc,                     \
        .item_name  = av_default_item_name,     \
        .option     = options,                  \
        .version    = LIBAVUTIL_VERSION_INT,    \
        .category   = AV_CLASS_CATEGORY_BITSTREAM_FILTER, \
    }
#define BSF_DEFINE_CLASS(fname) \
    BSF_DEFINE_CLASS_EXT(fname, #fname, fname##_options)

/**
 * Mark a filter ready and schedule it for activation.
 *
 * This is automatically done when something happens to the filter (queued
 * packet, status change, request on output).
 * Filters implementing the activate callback can call it directly to
 * perform one more round of processing later.
 * It is also useful for filters reacting to external or asynchronous
 * events.
 */
void ff_bsf_set_ready(AVBitStreamFilterContext *filter, unsigned priority);

/**
 * Send a packet of data to the next filter.
 *
 * @param link   the output link over which the data is being sent
 * @param pkt   a reference to the buffer of data being sent. The
 *              receiving filter will free this reference when it no longer
 *              needs it or pass it on to the next filter.
 *
 * @return >= 0 on success, a negative AVERROR on error. The receiving filter
 * is responsible for unreferencing pkt in case of error.
 */
int ff_bsf_filter_packet(AVBitStreamFilterLink *link, AVPacket *pkt);

int ff_bsf_request_packet(AVBitStreamFilterLink *link);

int ff_bsf_inlink_consume_packet(AVBitStreamFilterLink *link, AVPacket **pkt);

void ff_bsf_inlink_request_packet(AVBitStreamFilterLink *link);

int ff_bsf_inlink_acknowledge_status(AVBitStreamFilterLink *link, int *rstatus, int64_t *rpts);

size_t ff_bsf_inlink_queued_packets(AVBitStreamFilterLink *link);

int ff_bsf_inlink_check_available_packet(AVBitStreamFilterLink *link);

void ff_bsf_inlink_set_status(AVBitStreamFilterLink *link, int status);

void ff_bsf_link_set_in_status(AVBitStreamFilterLink *link, int status, int64_t pts);

int ff_bsf_outlink_get_status(AVBitStreamFilterLink *link);

int ff_bsf_outlink_packet_wanted(AVBitStreamFilterLink *link);

/**
 * Get the number of failed requests.
 *
 * A failed request is when the request_packet method is called while no
 * packet is present in the buffer.
 * The number is reset when a packet is added.
 */
unsigned ff_bsf_source_get_nb_failed_requests(const AVBitStreamFilterContext *src);

#endif /* AVCODEC_BSF_FILTERS_H */

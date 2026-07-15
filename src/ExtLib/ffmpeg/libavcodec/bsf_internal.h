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

#ifndef AVCODEC_BSF_INTERNAL_H
#define AVCODEC_BSF_INTERNAL_H

#include "libavutil/container_fifo.h"
#include "libavutil/log.h"

#include "bsf.h"
#include "packet.h"
#include "bsf/filters.h"

typedef struct FFBitStreamFilter {
    /**
     * The public AVBitStreamFilter. See bsf.h for it.
     */
    AVBitStreamFilter p;

    int priv_data_size;
    int (*init)(AVBSFContext *ctx);
    int (*filter)(AVBSFContext *ctx, AVPacket *pkt);
    void (*close)(AVBSFContext *ctx);
    void (*flush)(AVBSFContext *ctx);

    // Graph based API

    /**
     * List of static inputs.
     */
    const struct AVBitStreamFilterPad *inputs;

    /**
     * List of static outputs.
     */
    const struct AVBitStreamFilterPad *outputs;

    /**
     * The number of entries in the list of inputs.
     */
    uint8_t nb_inputs;

    /**
     * The number of entries in the list of outputs.
     */
    uint8_t nb_outputs;

    int (*preinit)(AVBitStreamFilterContext *ctx);
    int (*init2)(AVBitStreamFilterContext *ctx);
    void (*uninit)(AVBitStreamFilterContext *ctx);

    /**
     * Filter activation function.
     *
     * Called when any processing is needed from the filter, instead of any
     * filter_packet and request_packet on pads.
     *
     * The function must examine inlinks and outlinks and perform a single
     * step of processing. If there is nothing to do, the function must do
     * nothing and not return an error. If more steps are or may be
     * possible, it must use ff_filter_set_ready() to schedule another
     * activation.
     */
    int (*activate)(AVBitStreamFilterContext *ctx);
} FFBitStreamFilter;

static av_always_inline const FFBitStreamFilter *ff_bsf(const AVBitStreamFilter *bsf)
{
    return (const FFBitStreamFilter*)bsf;
}

/**
 * Called by the bitstream filters to get the next packet for filtering.
 * The filter is responsible for either freeing the packet or passing it to the
 * caller.
 */
int ff_bsf_get_packet(AVBSFContext *ctx, AVPacket **pkt);

/**
 * Called by bitstream filters to get packet for filtering.
 * The reference to packet is moved to provided packet structure.
 *
 * @param ctx pointer to AVBSFContext of filter
 * @param pkt pointer to packet to move reference to
 *
 * @return 0 on success, negative AVERROR in case of failure
 */
int ff_bsf_get_packet_ref(AVBSFContext *ctx, AVPacket *pkt);

const AVClass *ff_bsf_child_class_iterate(void **opaque);


// Graph based API

typedef struct BitStreamFilterLinkInternal {
    AVBitStreamFilterLink l;

    /**
     * Queue of packets waiting to be filtered.
     */
    AVContainerFifo *fifo;

    /**
     * If set, the source filter can not generate a packet as is.
     * The goal is to avoid repeatedly calling the request_packet() method on
     * the same link.
     */
    int packet_blocked_in;

    /**
     * Link input status.
     * If not zero, all attempts of filter_packet will fail with the
     * corresponding code.
     */
    int status_in;

    /**
     * Timestamp of the input status change.
     */
    int64_t status_in_pts;

    /**
     * Link output status.
     * If not zero, all attempts of request_packet will fail with the
     * corresponding code.
     */
    int status_out;

    /**
     * True if a packet is currently wanted on the output of this filter.
     * Set when ff_request_packet() is called by the output,
     * cleared when a packet is filtered.
     */
    int packet_wanted_out;

    /**
     * Index in the age array.
     */
    int age_index;

    /** stage of the initialization of the link properties (dimensions, etc) */
    enum {
        AVLINK_UNINIT = 0,      ///< not started
        AVLINK_STARTINIT,       ///< started, but incomplete
        AVLINK_INIT             ///< complete
    } init_state;
} BitStreamFilterLinkInternal;

static inline BitStreamFilterLinkInternal *ff_link_internal(AVBitStreamFilterLink *link)
{
    return (BitStreamFilterLinkInternal*)link;
}

int ff_bsf_config_links(AVBitStreamFilterContext *filter);

/**
 * Run one round of processing on a filter graph.
 */
int ff_bsf_graph_run_once(AVBitStreamFilterGraph *graph);

int ff_bsf_activate(AVBitStreamFilterContext *ctx);

void ff_bsf_graph_update_heap(AVBitStreamFilterGraph *graph, BitStreamFilterLinkInternal *li);

typedef struct FFBitStreamFilterContext {
    /**
     * The public AVBitStreamFilterContext. See bsf.h for it.
     */
    AVBitStreamFilterContext p;

    // AV_CLASS_STATE_FLAG_*
    unsigned state_flags;

    /**
     * Ready status of the filter.
     * A non-0 value means that the filter needs activating;
     * a higher value suggests a more urgent activation.
     */
    unsigned ready;
} FFBitStreamFilterContext;

static inline FFBitStreamFilterContext *ffbsfctx(AVBitStreamFilterContext *ctx)
{
    return (FFBitStreamFilterContext*)ctx;
}

/**
 * Free a filter context. This will also remove the filter from its
 * filtergraph's list of filters.
 *
 * @param filter the filter to free
 */
void ff_bsf_free(AVBitStreamFilterContext *filter);

typedef struct FFBitStreamFilterGraph {
    /**
     * The public AVBitStreamFilterGraph. See bsf.h for it.
     */
    AVBitStreamFilterGraph p;

    struct BitStreamFilterLinkInternal **sink_links;
    struct BitStreamFilterLinkInternal **source_links;
    int sink_links_count;
    int source_links_count;

    size_t max_packet_queue;
    size_t packets_queued;
} FFBitStreamFilterGraph;

static inline FFBitStreamFilterGraph *ffbsffiltergraph(AVBitStreamFilterGraph *graph)
{
    return (FFBitStreamFilterGraph*)graph;
}

static inline const FFBitStreamFilterGraph *cffbsffiltergraph(const AVBitStreamFilterGraph *graph)
{
    return (const FFBitStreamFilterGraph*)graph;
}

/**
 * Allocate a new filter context and return it.
 *
 * @param[in]  filter what filter to create an instance of
 * @param[in]  inst_name name to give to the new filter context
 * @param[out] ctx a pointer into which the pointer to the newly-allocated context
 *                 will be written
 *
 * @return 0 on success, an AVERROR code on failure
 */
int ff_bsf_alloc(const AVBitStreamFilter *filter, const char *inst_name, AVBitStreamFilterContext **ctx);

/**
 * Remove a filter from a graph;
 */
void ff_bsf_graph_remove_filter(AVBitStreamFilterGraph *graph, AVBitStreamFilterContext *filter);

#endif /* AVCODEC_BSF_INTERNAL_H */

/*
 * filter graphs
 * Copyright (c) 2008 Vitor Sessak
 * Copyright (c) 2007 Bobby Bingham
 *
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

#include "config.h"

#include <string.h>

#include "libavutil/avassert.h"
#include "libavutil/bprint.h"
#include "libavutil/channel_layout.h"
#include "libavutil/imgutils.h"
#include "libavutil/mem.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"


#include "avfilter.h"
#include "avfilter_internal.h"
#include "buffersink.h"
#include "filters.h"
#include "formats.h"
#include "framequeue.h"
#include "video.h"

#define OFFSET(x) offsetof(AVFilterGraph, x)
#define F AV_OPT_FLAG_FILTERING_PARAM
#define V AV_OPT_FLAG_VIDEO_PARAM
#define A AV_OPT_FLAG_AUDIO_PARAM
static const AVOption filtergraph_options[] = {
    { "thread_type", "Allowed thread types", OFFSET(thread_type), AV_OPT_TYPE_FLAGS,
        { .i64 = AVFILTER_THREAD_SLICE }, 0, INT_MAX, F|V|A, .unit = "thread_type" },
        { "slice", NULL, 0, AV_OPT_TYPE_CONST, { .i64 = AVFILTER_THREAD_SLICE }, .flags = F|V|A, .unit = "thread_type" },
    { "threads",     "Maximum number of threads", OFFSET(nb_threads), AV_OPT_TYPE_INT,
        { .i64 = 0 }, 0, INT_MAX, F|V|A, .unit = "threads"},
        {"auto", "autodetect a suitable number of threads to use", 0, AV_OPT_TYPE_CONST, {.i64 = 0 }, .flags = F|V|A, .unit = "threads"},
    {"scale_sws_opts"       , "default scale filter options"        , OFFSET(scale_sws_opts)        ,
        AV_OPT_TYPE_STRING, {.str = NULL}, 0, 0, F|V },
    {"aresample_swr_opts"   , "default aresample filter options"    , OFFSET(aresample_swr_opts)    ,
        AV_OPT_TYPE_STRING, {.str = NULL}, 0, 0, F|A },
    { NULL },
};

static const AVClass filtergraph_class = {
    .class_name = "AVFilterGraph",
    .item_name  = av_default_item_name,
    .version    = LIBAVUTIL_VERSION_INT,
    .option     = filtergraph_options,
    .category   = AV_CLASS_CATEGORY_FILTER,
};

#if !HAVE_THREADS
void ff_graph_thread_free(FFFilterGraph *graph)
{
}

int ff_graph_thread_init(FFFilterGraph *graph)
{
    graph->p.thread_type = 0;
    graph->p.nb_threads  = 1;
    return 0;
}
#endif

AVFilterGraph *avfilter_graph_alloc(void)
{
    FFFilterGraph *graph = av_mallocz(sizeof(*graph));
    AVFilterGraph *ret;

    if (!graph)
        return NULL;

    ret = &graph->p;
    ret->av_class = &filtergraph_class;
    av_opt_set_defaults(ret);
    ff_framequeue_global_init(&graph->frame_queues);

    return ret;
}

void ff_filter_graph_remove_filter(AVFilterGraph *graph, AVFilterContext *filter)
{
    int i, j;
    for (i = 0; i < graph->nb_filters; i++) {
        if (graph->filters[i] == filter) {
            FFSWAP(AVFilterContext*, graph->filters[i],
                   graph->filters[graph->nb_filters - 1]);
            graph->nb_filters--;
            filter->graph = NULL;
            for (j = 0; j<filter->nb_outputs; j++)
                if (filter->outputs[j])
                    ff_filter_link(filter->outputs[j])->graph = NULL;

            return;
        }
    }
}

void avfilter_graph_free(AVFilterGraph **graphp)
{
    AVFilterGraph *graph = *graphp;
    FFFilterGraph *graphi = fffiltergraph(graph);

    if (!graph)
        return;

    while (graph->nb_filters)
        avfilter_free(graph->filters[0]);

    ff_graph_thread_free(graphi);

    av_freep(&graphi->sink_links);

    av_opt_free(graph);

    av_freep(&graph->filters);
    av_freep(graphp);
}

int avfilter_graph_create_filter(AVFilterContext **filt_ctx, const AVFilter *filt,
                                 const char *name, const char *args, void *opaque,
                                 AVFilterGraph *graph_ctx)
{
    int ret;

    *filt_ctx = avfilter_graph_alloc_filter(graph_ctx, filt, name);
    if (!*filt_ctx)
        return AVERROR(ENOMEM);

    ret = avfilter_init_str(*filt_ctx, args);
    if (ret < 0)
        goto fail;

    return 0;

fail:
    avfilter_free(*filt_ctx);
    *filt_ctx = NULL;
    return ret;
}

void avfilter_graph_set_auto_convert(AVFilterGraph *graph, unsigned flags)
{
    fffiltergraph(graph)->disable_auto_convert = flags;
}

AVFilterContext *avfilter_graph_alloc_filter(AVFilterGraph *graph,
                                             const AVFilter *filter,
                                             const char *name)
{
    AVFilterContext **filters, *s;
    FFFilterGraph *graphi = fffiltergraph(graph);

    if (graph->thread_type && !graphi->thread_execute) {
        if (graph->execute) {
            graphi->thread_execute = graph->execute;
        } else {
            int ret = ff_graph_thread_init(graphi);
            if (ret < 0) {
                av_log(graph, AV_LOG_ERROR, "Error initializing threading: %s.\n", av_err2str(ret));
                return NULL;
            }
        }
    }

    filters = av_realloc_array(graph->filters, graph->nb_filters + 1, sizeof(*filters));
    if (!filters)
        return NULL;
    graph->filters = filters;

    s = ff_filter_alloc(filter, name);
    if (!s)
        return NULL;

    graph->filters[graph->nb_filters++] = s;

    s->graph = graph;

    return s;
}

/**
 * Check for the validity of graph.
 *
 * A graph is considered valid if all its input and output pads are
 * connected.
 *
 * @return >= 0 in case of success, a negative value otherwise
 */
static int graph_check_validity(AVFilterGraph *graph, void *log_ctx)
{
    AVFilterContext *filt;
    int i, j;

    for (i = 0; i < graph->nb_filters; i++) {
        const AVFilterPad *pad;
        filt = graph->filters[i];

        for (j = 0; j < filt->nb_inputs; j++) {
            if (!filt->inputs[j] || !filt->inputs[j]->src) {
                pad = &filt->input_pads[j];
                av_log(log_ctx, AV_LOG_ERROR,
                       "Input pad \"%s\" with type %s of the filter instance \"%s\" of %s not connected to any source\n",
                       pad->name, av_get_media_type_string(pad->type), filt->name, filt->filter->name);
                return AVERROR(EINVAL);
            }
        }

        for (j = 0; j < filt->nb_outputs; j++) {
            if (!filt->outputs[j] || !filt->outputs[j]->dst) {
                pad = &filt->output_pads[j];
                av_log(log_ctx, AV_LOG_ERROR,
                       "Output pad \"%s\" with type %s of the filter instance \"%s\" of %s not connected to any destination\n",
                       pad->name, av_get_media_type_string(pad->type), filt->name, filt->filter->name);
                return AVERROR(EINVAL);
            }
        }
    }

    return 0;
}

/**
 * Configure all the links of graphctx.
 *
 * @return >= 0 in case of success, a negative value otherwise
 */
static int graph_config_links(AVFilterGraph *graph, void *log_ctx)
{
    AVFilterContext *filt;
    int i, ret;

    for (i = 0; i < graph->nb_filters; i++) {
        filt = graph->filters[i];

        if (!filt->nb_outputs) {
            if ((ret = ff_filter_config_links(filt)))
                return ret;
        }
    }

    return 0;
}

static int graph_check_links(AVFilterGraph *graph, void *log_ctx)
{
    AVFilterContext *f;
    AVFilterLink *l;
    unsigned i, j;
    int ret;

    for (i = 0; i < graph->nb_filters; i++) {
        f = graph->filters[i];
        for (j = 0; j < f->nb_outputs; j++) {
            l = f->outputs[j];
            if (l->type == AVMEDIA_TYPE_VIDEO) {
                ret = av_image_check_size2(l->w, l->h, INT64_MAX, l->format, 0, f);
                if (ret < 0)
                    return ret;
            }
        }
    }
    return 0;
}

AVFilterContext *avfilter_graph_get_filter(AVFilterGraph *graph, const char *name)
{
    int i;

    for (i = 0; i < graph->nb_filters; i++)
        if (graph->filters[i]->name && !strcmp(name, graph->filters[i]->name))
            return graph->filters[i];

    return NULL;
}

static int filter_link_check_formats(void *log, AVFilterLink *link, AVFilterFormatsConfig *cfg)
{
    int ret;

    switch (link->type) {

    case AVMEDIA_TYPE_VIDEO:
        if ((ret = ff_formats_check_pixel_formats(log, cfg->formats)) < 0 ||
            (ret = ff_formats_check_color_spaces(log, cfg->color_spaces)) < 0 ||
            (ret = ff_formats_check_color_ranges(log, cfg->color_ranges)) < 0)
            return ret;
        break;

    case AVMEDIA_TYPE_AUDIO:
        if ((ret = ff_formats_check_sample_formats(log, cfg->formats)) < 0 ||
            (ret = ff_formats_check_sample_rates(log, cfg->samplerates)) < 0 ||
            (ret = ff_formats_check_channel_layouts(log, cfg->channel_layouts)) < 0)
            return ret;
        break;

    default:
        av_assert0(!"reached");
    }
    return 0;
}

/**
 * Check the validity of the formats / etc. lists set by query_formats().
 *
 * In particular, check they do not contain any redundant element.
 */
static int filter_check_formats(AVFilterContext *ctx)
{
    unsigned i;
    int ret;

    for (i = 0; i < ctx->nb_inputs; i++) {
        ret = filter_link_check_formats(ctx, ctx->inputs[i], &ctx->inputs[i]->outcfg);
        if (ret < 0)
            return ret;
    }
    for (i = 0; i < ctx->nb_outputs; i++) {
        ret = filter_link_check_formats(ctx, ctx->outputs[i], &ctx->outputs[i]->incfg);
        if (ret < 0)
            return ret;
    }
    return 0;
}

static int filter_query_formats(AVFilterContext *ctx)
{
    const FFFilter *const filter = fffilter(ctx->filter);
    int ret;

    if (filter->formats_state == FF_FILTER_FORMATS_QUERY_FUNC) {
        if ((ret = filter->formats.query_func(ctx)) < 0) {
            if (ret != AVERROR(EAGAIN))
                av_log(ctx, AV_LOG_ERROR, "Query format failed for '%s': %s\n",
                       ctx->name, av_err2str(ret));
            return ret;
        }
    } else if (filter->formats_state == FF_FILTER_FORMATS_QUERY_FUNC2) {
        AVFilterFormatsConfig *cfg_in_stack[64], *cfg_out_stack[64];
        AVFilterFormatsConfig **cfg_in_dyn = NULL, **cfg_out_dyn = NULL;
        AVFilterFormatsConfig **cfg_in, **cfg_out;

        if (ctx->nb_inputs > FF_ARRAY_ELEMS(cfg_in_stack)) {
            cfg_in_dyn = av_malloc_array(ctx->nb_inputs, sizeof(*cfg_in_dyn));
            if (!cfg_in_dyn)
                return AVERROR(ENOMEM);
            cfg_in = cfg_in_dyn;
        } else
            cfg_in = ctx->nb_inputs ? cfg_in_stack : NULL;

        for (unsigned i = 0; i < ctx->nb_inputs; i++) {
            AVFilterLink *l = ctx->inputs[i];
            cfg_in[i] = &l->outcfg;
        }

        if (ctx->nb_outputs > FF_ARRAY_ELEMS(cfg_out_stack)) {
            cfg_out_dyn = av_malloc_array(ctx->nb_outputs, sizeof(*cfg_out_dyn));
            if (!cfg_out_dyn) {
                av_freep(&cfg_in_dyn);
                return AVERROR(ENOMEM);
            }
            cfg_out = cfg_out_dyn;
        } else
            cfg_out = ctx->nb_outputs ? cfg_out_stack : NULL;

        for (unsigned i = 0; i < ctx->nb_outputs; i++) {
            AVFilterLink *l = ctx->outputs[i];
            cfg_out[i] = &l->incfg;
        }

        ret = filter->formats.query_func2(ctx, cfg_in, cfg_out);
        av_freep(&cfg_in_dyn);
        av_freep(&cfg_out_dyn);
        if (ret < 0) {
            if (ret != AVERROR(EAGAIN))
                av_log(ctx, AV_LOG_ERROR, "Query format failed for '%s': %s\n",
                       ctx->name, av_err2str(ret));
            return ret;
        }
    }

    if (filter->formats_state == FF_FILTER_FORMATS_QUERY_FUNC ||
        filter->formats_state == FF_FILTER_FORMATS_QUERY_FUNC2) {
        ret = filter_check_formats(ctx);
        if (ret < 0)
            return ret;
    }

    return ff_default_query_formats(ctx);
}

static int formats_declared(AVFilterContext *f)
{
    int i;

    for (i = 0; i < f->nb_inputs; i++) {
        if (!f->inputs[i]->outcfg.formats)
            return 0;
        if (f->inputs[i]->type == AVMEDIA_TYPE_VIDEO &&
            !(f->inputs[i]->outcfg.color_ranges &&
              f->inputs[i]->outcfg.color_spaces))
            return 0;
        if (f->inputs[i]->type == AVMEDIA_TYPE_AUDIO &&
            !(f->inputs[i]->outcfg.samplerates &&
              f->inputs[i]->outcfg.channel_layouts))
            return 0;
    }
    for (i = 0; i < f->nb_outputs; i++) {
        if (!f->outputs[i]->incfg.formats)
            return 0;
        if (f->outputs[i]->type == AVMEDIA_TYPE_VIDEO &&
            !(f->outputs[i]->incfg.color_ranges &&
              f->outputs[i]->incfg.color_spaces))
            return 0;
        if (f->outputs[i]->type == AVMEDIA_TYPE_AUDIO &&
            !(f->outputs[i]->incfg.samplerates &&
              f->outputs[i]->incfg.channel_layouts))
            return 0;
    }
    return 1;
}

/**
 * Perform one round of query_formats() and merging formats lists on the
 * filter graph.
 * @return  >=0 if all links formats lists could be queried and merged;
 *          AVERROR(EAGAIN) some progress was made in the queries or merging
 *          and a later call may succeed;
 *          AVERROR(EIO) (may be changed) plus a log message if no progress
 *          was made and the negotiation is stuck;
 *          a negative error code if some other error happened
 */
static int query_formats(AVFilterGraph *graph, void *log_ctx)
{
    int i, j, ret;
    int converter_count = 0;
    int count_queried = 0;        /* successful calls to query_formats() */
    int count_merged = 0;         /* successful merge of formats lists */
    int count_already_merged = 0; /* lists already merged */
    int count_delayed = 0;        /* lists that need to be merged later */

    for (i = 0; i < graph->nb_filters; i++) {
        AVFilterContext *f = graph->filters[i];
        if (formats_declared(f))
            continue;
        ret = filter_query_formats(f);
        if (ret < 0 && ret != AVERROR(EAGAIN))
            return ret;
        /* note: EAGAIN could indicate a partial success, not counted yet */
        count_queried += ret >= 0;
    }

    /* go through and merge as many format lists as possible */
    for (i = 0; i < graph->nb_filters; i++) {
        AVFilterContext *filter = graph->filters[i];

        for (j = 0; j < filter->nb_inputs; j++) {
            AVFilterLink *link = filter->inputs[j];
            const AVFilterNegotiation *neg;
            unsigned neg_step;
            int convert_needed = 0;

            if (!link)
                continue;

            neg = ff_filter_get_negotiation(link);
            av_assert0(neg);
            for (neg_step = 0; neg_step < neg->nb_mergers; neg_step++) {
                const AVFilterFormatsMerger *m = &neg->mergers[neg_step];
                void *a = FF_FIELD_AT(void *, m->offset, link->incfg);
                void *b = FF_FIELD_AT(void *, m->offset, link->outcfg);
                if (a && b && a != b && !m->can_merge(a, b)) {
                    convert_needed = 1;
                    break;
                }
            }
            for (neg_step = 0; neg_step < neg->nb_mergers; neg_step++) {
                const AVFilterFormatsMerger *m = &neg->mergers[neg_step];
                void *a = FF_FIELD_AT(void *, m->offset, link->incfg);
                void *b = FF_FIELD_AT(void *, m->offset, link->outcfg);
                if (!(a && b)) {
                    count_delayed++;
                } else if (a == b) {
                    count_already_merged++;
                } else if (!convert_needed) {
                    count_merged++;
                    ret = m->merge(a, b);
                    if (ret < 0)
                        return ret;
                    if (!ret)
                        convert_needed = 1;
                }
            }

            if (convert_needed) {
                AVFilterContext *convert;
                const AVFilter *filter;
                AVFilterLink *inlink, *outlink;
                char inst_name[30];
                const char *opts;

                if (fffiltergraph(graph)->disable_auto_convert) {
                    av_log(log_ctx, AV_LOG_ERROR,
                           "The filters '%s' and '%s' do not have a common format "
                           "and automatic conversion is disabled.\n",
                           link->src->name, link->dst->name);
                    return AVERROR(EINVAL);
                }

                /* couldn't merge format lists. auto-insert conversion filter */
                if (!(filter = avfilter_get_by_name(neg->conversion_filter))) {
                    av_log(log_ctx, AV_LOG_ERROR,
                           "'%s' filter not present, cannot convert formats.\n",
                           neg->conversion_filter);
                    return AVERROR(EINVAL);
                }
                snprintf(inst_name, sizeof(inst_name), "auto_%s_%d",
                         neg->conversion_filter, converter_count++);
                opts = FF_FIELD_AT(char *, neg->conversion_opts_offset, *graph);
                ret = avfilter_graph_create_filter(&convert, filter, inst_name, opts, NULL, graph);
                if (ret < 0)
                    return ret;
                if ((ret = avfilter_insert_filter(link, convert, 0, 0)) < 0)
                    return ret;

                if ((ret = filter_query_formats(convert)) < 0)
                    return ret;

                inlink  = convert->inputs[0];
                outlink = convert->outputs[0];
                av_assert0( inlink->incfg.formats->refcount > 0);
                av_assert0( inlink->outcfg.formats->refcount > 0);
                av_assert0(outlink->incfg.formats->refcount > 0);
                av_assert0(outlink->outcfg.formats->refcount > 0);
                if (outlink->type == AVMEDIA_TYPE_VIDEO) {
                    av_assert0( inlink-> incfg.color_spaces->refcount > 0);
                    av_assert0( inlink->outcfg.color_spaces->refcount > 0);
                    av_assert0(outlink-> incfg.color_spaces->refcount > 0);
                    av_assert0(outlink->outcfg.color_spaces->refcount > 0);
                    av_assert0( inlink-> incfg.color_ranges->refcount > 0);
                    av_assert0( inlink->outcfg.color_ranges->refcount > 0);
                    av_assert0(outlink-> incfg.color_ranges->refcount > 0);
                    av_assert0(outlink->outcfg.color_ranges->refcount > 0);
                } else if (outlink->type == AVMEDIA_TYPE_AUDIO) {
                    av_assert0( inlink-> incfg.samplerates->refcount > 0);
                    av_assert0( inlink->outcfg.samplerates->refcount > 0);
                    av_assert0(outlink-> incfg.samplerates->refcount > 0);
                    av_assert0(outlink->outcfg.samplerates->refcount > 0);
                    av_assert0( inlink-> incfg.channel_layouts->refcount > 0);
                    av_assert0( inlink->outcfg.channel_layouts->refcount > 0);
                    av_assert0(outlink-> incfg.channel_layouts->refcount > 0);
                    av_assert0(outlink->outcfg.channel_layouts->refcount > 0);
                }
#define MERGE(merger, link)                                                  \
    ((merger)->merge(FF_FIELD_AT(void *, (merger)->offset, (link)->incfg),   \
                     FF_FIELD_AT(void *, (merger)->offset, (link)->outcfg)))
                for (neg_step = 0; neg_step < neg->nb_mergers; neg_step++) {
                    const AVFilterFormatsMerger *m = &neg->mergers[neg_step];
                    if ((ret = MERGE(m,  inlink)) <= 0 ||
                        (ret = MERGE(m, outlink)) <= 0) {
                        if (ret < 0)
                            return ret;
                        av_log(log_ctx, AV_LOG_ERROR,
                               "Impossible to convert between the formats supported by the filter "
                               "'%s' and the filter '%s'\n", link->src->name, link->dst->name);
                        return AVERROR(ENOSYS);
                    }
                }
            }
        }
    }

    av_log(graph, AV_LOG_DEBUG, "query_formats: "
           "%d queried, %d merged, %d already done, %d delayed\n",
           count_queried, count_merged, count_already_merged, count_delayed);
    if (count_delayed) {
        AVBPrint bp;

        /* if count_queried > 0, one filter at least did set its formats,
           that will give additional information to its neighbour;
           if count_merged > 0, one pair of formats lists at least was merged,
           that will give additional information to all connected filters;
           in both cases, progress was made and a new round must be done */
        if (count_queried || count_merged)
            return AVERROR(EAGAIN);
        av_bprint_init(&bp, 0, AV_BPRINT_SIZE_AUTOMATIC);
        for (i = 0; i < graph->nb_filters; i++)
            if (!formats_declared(graph->filters[i]))
                av_bprintf(&bp, "%s%s", bp.len ? ", " : "",
                          graph->filters[i]->name);
        av_log(graph, AV_LOG_ERROR,
               "The following filters could not choose their formats: %s\n"
               "Consider inserting the (a)format filter near their input or "
               "output.\n", bp.str);
        return AVERROR(EIO);
    }
    return 0;
}

static int get_fmt_score(enum AVSampleFormat dst_fmt, enum AVSampleFormat src_fmt)
{
    int score = 0;

    if (av_sample_fmt_is_planar(dst_fmt) != av_sample_fmt_is_planar(src_fmt))
        score ++;

    if (av_get_bytes_per_sample(dst_fmt) < av_get_bytes_per_sample(src_fmt)) {
        score += 100 * (av_get_bytes_per_sample(src_fmt) - av_get_bytes_per_sample(dst_fmt));
    }else
        score += 10  * (av_get_bytes_per_sample(dst_fmt) - av_get_bytes_per_sample(src_fmt));

    if (av_get_packed_sample_fmt(dst_fmt) == AV_SAMPLE_FMT_S32 &&
        av_get_packed_sample_fmt(src_fmt) == AV_SAMPLE_FMT_FLT)
        score += 20;

    if (av_get_packed_sample_fmt(dst_fmt) == AV_SAMPLE_FMT_FLT &&
        av_get_packed_sample_fmt(src_fmt) == AV_SAMPLE_FMT_S32)
        score += 2;

    return score;
}

static enum AVSampleFormat find_best_sample_fmt_of_2(enum AVSampleFormat dst_fmt1, enum AVSampleFormat dst_fmt2,
                                                     enum AVSampleFormat src_fmt)
{
    int score1, score2;

    score1 = get_fmt_score(dst_fmt1, src_fmt);
    score2 = get_fmt_score(dst_fmt2, src_fmt);

    return score1 < score2 ? dst_fmt1 : dst_fmt2;
}

int ff_fmt_is_regular_yuv(enum AVPixelFormat fmt)
{
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(fmt);
    if (!desc)
        return 0;
    if (desc->nb_components < 3)
        return 0; /* Grayscale is explicitly full-range in swscale */
    av_assert1(!(desc->flags & AV_PIX_FMT_FLAG_HWACCEL));
    return !(desc->flags & (AV_PIX_FMT_FLAG_RGB | AV_PIX_FMT_FLAG_PAL |
                            AV_PIX_FMT_FLAG_XYZ | AV_PIX_FMT_FLAG_FLOAT));
}


int ff_fmt_is_forced_full_range(enum AVPixelFormat fmt)
{
    switch (fmt) {
    case AV_PIX_FMT_YUVJ420P:
    case AV_PIX_FMT_YUVJ422P:
    case AV_PIX_FMT_YUVJ444P:
    case AV_PIX_FMT_YUVJ440P:
    case AV_PIX_FMT_YUVJ411P:
        return 1;
    default:
        return 0;
    }
}

static int pick_format(AVFilterLink *link, AVFilterLink *ref)
{
    if (!link || !link->incfg.formats)
        return 0;

    if (link->type == AVMEDIA_TYPE_VIDEO) {
        if(ref && ref->type == AVMEDIA_TYPE_VIDEO){
            //FIXME: This should check for AV_PIX_FMT_FLAG_ALPHA after PAL8 pixel format without alpha is implemented
            int has_alpha= av_pix_fmt_desc_get(ref->format)->nb_components % 2 == 0;
            enum AVPixelFormat best= AV_PIX_FMT_NONE;
            int i;
            for (i = 0; i < link->incfg.formats->nb_formats; i++) {
                enum AVPixelFormat p = link->incfg.formats->formats[i];
                best= av_find_best_pix_fmt_of_2(best, p, ref->format, has_alpha, NULL);
            }
            av_log(link->src,AV_LOG_DEBUG, "picking %s out of %d ref:%s alpha:%d\n",
                   av_get_pix_fmt_name(best), link->incfg.formats->nb_formats,
                   av_get_pix_fmt_name(ref->format), has_alpha);
            link->incfg.formats->formats[0] = best;
        }
    } else if (link->type == AVMEDIA_TYPE_AUDIO) {
        if(ref && ref->type == AVMEDIA_TYPE_AUDIO){
            enum AVSampleFormat best= AV_SAMPLE_FMT_NONE;
            int i;
            for (i = 0; i < link->incfg.formats->nb_formats; i++) {
                enum AVSampleFormat p = link->incfg.formats->formats[i];
                best = find_best_sample_fmt_of_2(best, p, ref->format);
            }
            av_log(link->src,AV_LOG_DEBUG, "picking %s out of %d ref:%s\n",
                   av_get_sample_fmt_name(best), link->incfg.formats->nb_formats,
                   av_get_sample_fmt_name(ref->format));
            link->incfg.formats->formats[0] = best;
        }
    }

    link->incfg.formats->nb_formats = 1;
    link->format = link->incfg.formats->formats[0];

    if (link->type == AVMEDIA_TYPE_VIDEO) {
        enum AVPixelFormat swfmt = link->format;
        if (av_pix_fmt_desc_get(swfmt)->flags & AV_PIX_FMT_FLAG_HWACCEL) {
            // FIXME: this is a hack - we'd like to use the sw_format of
            // link->hw_frames_ctx here, but it is not yet available.
            // To make this work properly we will need to either reorder
            // things so that it is available here or somehow negotiate
            // sw_format separately.
            swfmt = AV_PIX_FMT_YUV420P;
        }

        if (!ff_fmt_is_regular_yuv(swfmt)) {
            const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(swfmt);
            /* These fields are explicitly documented as affecting YUV only,
             * so set them to sane values for other formats. */
            if (desc->flags & AV_PIX_FMT_FLAG_FLOAT)
                link->color_range = AVCOL_RANGE_UNSPECIFIED;
            else
                link->color_range = AVCOL_RANGE_JPEG;
            if (desc->flags & (AV_PIX_FMT_FLAG_RGB | AV_PIX_FMT_FLAG_XYZ)) {
                link->colorspace = AVCOL_SPC_RGB;
            } else {
                link->colorspace = AVCOL_SPC_UNSPECIFIED;
            }
        } else {
            if (!link->incfg.color_spaces->nb_formats) {
                av_log(link->src, AV_LOG_ERROR, "Cannot select color space for"
                       " the link between filters %s and %s.\n", link->src->name,
                       link->dst->name);
                return AVERROR(EINVAL);
            }
            link->incfg.color_spaces->nb_formats = 1;
            link->colorspace = link->incfg.color_spaces->formats[0];

            if (ff_fmt_is_forced_full_range(swfmt)) {
                link->color_range = AVCOL_RANGE_JPEG;
            } else {
                if (!link->incfg.color_ranges->nb_formats) {
                    av_log(link->src, AV_LOG_ERROR, "Cannot select color range for"
                           " the link between filters %s and %s.\n", link->src->name,
                           link->dst->name);
                    return AVERROR(EINVAL);
                }
                link->incfg.color_ranges->nb_formats = 1;
                link->color_range = link->incfg.color_ranges->formats[0];
            }
        }
    } else if (link->type == AVMEDIA_TYPE_AUDIO) {
        int ret;

        if (!link->incfg.samplerates->nb_formats) {
            av_log(link->src, AV_LOG_ERROR, "Cannot select sample rate for"
                   " the link between filters %s and %s.\n", link->src->name,
                   link->dst->name);
            return AVERROR(EINVAL);
        }
        link->incfg.samplerates->nb_formats = 1;
        link->sample_rate = link->incfg.samplerates->formats[0];

        if (link->incfg.channel_layouts->all_layouts) {
            av_log(link->src, AV_LOG_ERROR, "Cannot select channel layout for"
                   " the link between filters %s and %s.\n", link->src->name,
                   link->dst->name);
            if (!link->incfg.channel_layouts->all_counts)
                av_log(link->src, AV_LOG_ERROR, "Unknown channel layouts not "
                       "supported, try specifying a channel layout using "
                       "'aformat=channel_layouts=something'.\n");
            return AVERROR(EINVAL);
        }
        link->incfg.channel_layouts->nb_channel_layouts = 1;
        ret = av_channel_layout_copy(&link->ch_layout, &link->incfg.channel_layouts->channel_layouts[0]);
        if (ret < 0)
            return ret;
    }

    ff_formats_unref(&link->incfg.formats);
    ff_formats_unref(&link->outcfg.formats);
    ff_formats_unref(&link->incfg.samplerates);
    ff_formats_unref(&link->outcfg.samplerates);
    ff_channel_layouts_unref(&link->incfg.channel_layouts);
    ff_channel_layouts_unref(&link->outcfg.channel_layouts);
    ff_formats_unref(&link->incfg.color_spaces);
    ff_formats_unref(&link->outcfg.color_spaces);
    ff_formats_unref(&link->incfg.color_ranges);
    ff_formats_unref(&link->outcfg.color_ranges);

    return 0;
}

#define REDUCE_FORMATS(fmt_type, list_type, list, var, nb, add_format) \
do {                                                                   \
    for (i = 0; i < filter->nb_inputs; i++) {                          \
        AVFilterLink *link = filter->inputs[i];                        \
        fmt_type fmt;                                                  \
                                                                       \
        if (!link->outcfg.list || link->outcfg.list->nb != 1)          \
            continue;                                                  \
        fmt = link->outcfg.list->var[0];                               \
                                                                       \
        for (j = 0; j < filter->nb_outputs; j++) {                     \
            AVFilterLink *out_link = filter->outputs[j];               \
            list_type *fmts;                                           \
                                                                       \
            if (link->type != out_link->type ||                        \
                out_link->incfg.list->nb == 1)                         \
                continue;                                              \
            fmts = out_link->incfg.list;                               \
                                                                       \
            if (!out_link->incfg.list->nb) {                           \
                if ((ret = add_format(&out_link->incfg.list, fmt)) < 0)\
                    return ret;                                        \
                ret = 1;                                               \
                break;                                                 \
            }                                                          \
                                                                       \
            for (k = 0; k < out_link->incfg.list->nb; k++)             \
                if (fmts->var[k] == fmt) {                             \
                    fmts->var[0]  = fmt;                               \
                    fmts->nb = 1;                                      \
                    ret = 1;                                           \
                    break;                                             \
                }                                                      \
        }                                                              \
    }                                                                  \
} while (0)

static int reduce_formats_on_filter(AVFilterContext *filter)
{
    int i, j, k, ret = 0;

    REDUCE_FORMATS(int,      AVFilterFormats,        formats,         formats,
                   nb_formats, ff_add_format);
    REDUCE_FORMATS(int,      AVFilterFormats,        samplerates,     formats,
                   nb_formats, ff_add_format);
    REDUCE_FORMATS(int,      AVFilterFormats,        color_spaces,    formats,
                   nb_formats, ff_add_format);
    REDUCE_FORMATS(int,      AVFilterFormats,        color_ranges,    formats,
                   nb_formats, ff_add_format);

    /* reduce channel layouts */
    for (i = 0; i < filter->nb_inputs; i++) {
        AVFilterLink *inlink = filter->inputs[i];
        const AVChannelLayout *fmt;

        if (!inlink->outcfg.channel_layouts ||
            inlink->outcfg.channel_layouts->nb_channel_layouts != 1)
            continue;
        fmt = &inlink->outcfg.channel_layouts->channel_layouts[0];

        for (j = 0; j < filter->nb_outputs; j++) {
            AVFilterLink *outlink = filter->outputs[j];
            AVFilterChannelLayouts *fmts;

            fmts = outlink->incfg.channel_layouts;
            if (inlink->type != outlink->type || fmts->nb_channel_layouts == 1)
                continue;

            if (fmts->all_layouts &&
                (KNOWN(fmt) || fmts->all_counts)) {
                /* Turn the infinite list into a singleton */
                fmts->all_layouts = fmts->all_counts  = 0;
                ret = ff_add_channel_layout(&outlink->incfg.channel_layouts, fmt);
                if (ret < 0)
                    return ret;
                ret = 1;
                break;
            }

            for (k = 0; k < outlink->incfg.channel_layouts->nb_channel_layouts; k++) {
                if (!av_channel_layout_compare(&fmts->channel_layouts[k], fmt)) {
                    ret = av_channel_layout_copy(&fmts->channel_layouts[0], fmt);
                    if (ret < 0)
                        return ret;
                    fmts->nb_channel_layouts = 1;
                    ret = 1;
                    break;
                }
            }
        }
    }

    return ret;
}

static int reduce_formats(AVFilterGraph *graph)
{
    int i, reduced, ret;

    do {
        reduced = 0;

        for (i = 0; i < graph->nb_filters; i++) {
            if ((ret = reduce_formats_on_filter(graph->filters[i])) < 0)
                return ret;
            reduced |= ret;
        }
    } while (reduced);

    return 0;
}

static void swap_samplerates_on_filter(AVFilterContext *filter)
{
    AVFilterLink *link = NULL;
    int sample_rate;
    int i, j;

    for (i = 0; i < filter->nb_inputs; i++) {
        link = filter->inputs[i];

        if (link->type == AVMEDIA_TYPE_AUDIO &&
            link->outcfg.samplerates->nb_formats== 1)
            break;
    }
    if (i == filter->nb_inputs)
        return;

    sample_rate = link->outcfg.samplerates->formats[0];

    for (i = 0; i < filter->nb_outputs; i++) {
        AVFilterLink *outlink = filter->outputs[i];
        int best_idx, best_diff = INT_MAX;

        if (outlink->type != AVMEDIA_TYPE_AUDIO ||
            outlink->incfg.samplerates->nb_formats < 2)
            continue;

        for (j = 0; j < outlink->incfg.samplerates->nb_formats; j++) {
            int diff = abs(sample_rate - outlink->incfg.samplerates->formats[j]);

            av_assert0(diff < INT_MAX); // This would lead to the use of uninitialized best_diff but is only possible with invalid sample rates

            if (diff < best_diff) {
                best_diff = diff;
                best_idx  = j;
            }
        }
        FFSWAP(int, outlink->incfg.samplerates->formats[0],
               outlink->incfg.samplerates->formats[best_idx]);
    }
}

static void swap_samplerates(AVFilterGraph *graph)
{
    int i;

    for (i = 0; i < graph->nb_filters; i++)
        swap_samplerates_on_filter(graph->filters[i]);
}

#define CH_CENTER_PAIR (AV_CH_FRONT_LEFT_OF_CENTER | AV_CH_FRONT_RIGHT_OF_CENTER)
#define CH_FRONT_PAIR  (AV_CH_FRONT_LEFT           | AV_CH_FRONT_RIGHT)
#define CH_STEREO_PAIR (AV_CH_STEREO_LEFT          | AV_CH_STEREO_RIGHT)
#define CH_WIDE_PAIR   (AV_CH_WIDE_LEFT            | AV_CH_WIDE_RIGHT)
#define CH_SIDE_PAIR   (AV_CH_SIDE_LEFT            | AV_CH_SIDE_RIGHT)
#define CH_DIRECT_PAIR (AV_CH_SURROUND_DIRECT_LEFT | AV_CH_SURROUND_DIRECT_RIGHT)
#define CH_BACK_PAIR   (AV_CH_BACK_LEFT            | AV_CH_BACK_RIGHT)

/* allowable substitutions for channel pairs when comparing layouts,
 * ordered by priority for both values */
static const uint64_t ch_subst[][2] = {
    { CH_FRONT_PAIR,      CH_CENTER_PAIR     },
    { CH_FRONT_PAIR,      CH_WIDE_PAIR       },
    { CH_FRONT_PAIR,      AV_CH_FRONT_CENTER },
    { CH_CENTER_PAIR,     CH_FRONT_PAIR      },
    { CH_CENTER_PAIR,     CH_WIDE_PAIR       },
    { CH_CENTER_PAIR,     AV_CH_FRONT_CENTER },
    { CH_WIDE_PAIR,       CH_FRONT_PAIR      },
    { CH_WIDE_PAIR,       CH_CENTER_PAIR     },
    { CH_WIDE_PAIR,       AV_CH_FRONT_CENTER },
    { AV_CH_FRONT_CENTER, CH_FRONT_PAIR      },
    { AV_CH_FRONT_CENTER, CH_CENTER_PAIR     },
    { AV_CH_FRONT_CENTER, CH_WIDE_PAIR       },
    { CH_SIDE_PAIR,       CH_DIRECT_PAIR     },
    { CH_SIDE_PAIR,       CH_BACK_PAIR       },
    { CH_SIDE_PAIR,       AV_CH_BACK_CENTER  },
    { CH_BACK_PAIR,       CH_DIRECT_PAIR     },
    { CH_BACK_PAIR,       CH_SIDE_PAIR       },
    { CH_BACK_PAIR,       AV_CH_BACK_CENTER  },
    { AV_CH_BACK_CENTER,  CH_BACK_PAIR       },
    { AV_CH_BACK_CENTER,  CH_DIRECT_PAIR     },
    { AV_CH_BACK_CENTER,  CH_SIDE_PAIR       },
};

static void swap_channel_layouts_on_filter(AVFilterContext *filter)
{
    AVFilterLink *link = NULL;
    int i, j, k;

    for (i = 0; i < filter->nb_inputs; i++) {
        link = filter->inputs[i];

        if (link->type == AVMEDIA_TYPE_AUDIO &&
            link->outcfg.channel_layouts->nb_channel_layouts == 1)
            break;
    }
    if (i == filter->nb_inputs)
        return;

    for (i = 0; i < filter->nb_outputs; i++) {
        AVFilterLink *outlink = filter->outputs[i];
        int best_idx = -1, best_score = INT_MIN, best_count_diff = INT_MAX;

        if (outlink->type != AVMEDIA_TYPE_AUDIO ||
            outlink->incfg.channel_layouts->nb_channel_layouts < 2)
            continue;

        for (j = 0; j < outlink->incfg.channel_layouts->nb_channel_layouts; j++) {
            AVChannelLayout in_chlayout = { 0 }, out_chlayout = { 0 };
            int  in_channels;
            int out_channels;
            int count_diff;
            int matched_channels, extra_channels;
            int score = 100000;

            av_channel_layout_copy(&in_chlayout, &link->outcfg.channel_layouts->channel_layouts[0]);
            av_channel_layout_copy(&out_chlayout, &outlink->incfg.channel_layouts->channel_layouts[j]);
            in_channels            = in_chlayout.nb_channels;
            out_channels           = out_chlayout.nb_channels;
            count_diff             = out_channels - in_channels;
            if (!KNOWN(&in_chlayout) || !KNOWN(&out_chlayout)) {
                /* Compute score in case the input or output layout encodes
                   a channel count; in this case the score is not altered by
                   the computation afterwards, as in_chlayout and
                   out_chlayout have both been set to 0 */
                if (!KNOWN(&in_chlayout))
                    in_channels = FF_LAYOUT2COUNT(&in_chlayout);
                if (!KNOWN(&out_chlayout))
                    out_channels = FF_LAYOUT2COUNT(&out_chlayout);
                score -= 10000 + FFABS(out_channels - in_channels) +
                         (in_channels > out_channels ? 10000 : 0);
                av_channel_layout_uninit(&in_chlayout);
                av_channel_layout_uninit(&out_chlayout);
                /* Let the remaining computation run, even if the score
                   value is not altered */
            }

            /* channel substitution */
            for (k = 0; k < FF_ARRAY_ELEMS(ch_subst); k++) {
                uint64_t cmp0 = ch_subst[k][0];
                uint64_t cmp1 = ch_subst[k][1];
                if ( av_channel_layout_subset(& in_chlayout, cmp0) &&
                    !av_channel_layout_subset(&out_chlayout, cmp0) &&
                     av_channel_layout_subset(&out_chlayout, cmp1) &&
                    !av_channel_layout_subset(& in_chlayout, cmp1)) {
                    av_channel_layout_from_mask(&in_chlayout, av_channel_layout_subset(& in_chlayout, ~cmp0));
                    av_channel_layout_from_mask(&out_chlayout, av_channel_layout_subset(&out_chlayout, ~cmp1));
                    /* add score for channel match, minus a deduction for
                       having to do the substitution */
                    score += 10 * av_popcount64(cmp1) - 2;
                }
            }

            /* no penalty for LFE channel mismatch */
            if (av_channel_layout_channel_from_index(&in_chlayout,  AV_CHAN_LOW_FREQUENCY) >= 0 &&
                av_channel_layout_channel_from_index(&out_chlayout, AV_CHAN_LOW_FREQUENCY) >= 0)
                score += 10;
            av_channel_layout_from_mask(&in_chlayout, av_channel_layout_subset(&in_chlayout, ~AV_CH_LOW_FREQUENCY));
            av_channel_layout_from_mask(&out_chlayout, av_channel_layout_subset(&out_chlayout, ~AV_CH_LOW_FREQUENCY));

            matched_channels = av_popcount64(in_chlayout.u.mask & out_chlayout.u.mask);
            extra_channels   = av_popcount64(out_chlayout.u.mask & (~in_chlayout.u.mask));
            score += 10 * matched_channels - 5 * extra_channels;

            if (score > best_score ||
                (count_diff < best_count_diff && score == best_score)) {
                best_score = score;
                best_idx   = j;
                best_count_diff = count_diff;
            }
        }
        av_assert0(best_idx >= 0);
        FFSWAP(AVChannelLayout, outlink->incfg.channel_layouts->channel_layouts[0],
               outlink->incfg.channel_layouts->channel_layouts[best_idx]);
    }

}

static void swap_channel_layouts(AVFilterGraph *graph)
{
    int i;

    for (i = 0; i < graph->nb_filters; i++)
        swap_channel_layouts_on_filter(graph->filters[i]);
}

static void swap_sample_fmts_on_filter(AVFilterContext *filter)
{
    AVFilterLink *link = NULL;
    int format, bps;
    int i, j;

    for (i = 0; i < filter->nb_inputs; i++) {
        link = filter->inputs[i];

        if (link->type == AVMEDIA_TYPE_AUDIO &&
            link->outcfg.formats->nb_formats == 1)
            break;
    }
    if (i == filter->nb_inputs)
        return;

    format = link->outcfg.formats->formats[0];
    bps    = av_get_bytes_per_sample(format);

    for (i = 0; i < filter->nb_outputs; i++) {
        AVFilterLink *outlink = filter->outputs[i];
        int best_idx = -1, best_score = INT_MIN;

        if (outlink->type != AVMEDIA_TYPE_AUDIO ||
            outlink->incfg.formats->nb_formats < 2)
            continue;

        for (j = 0; j < outlink->incfg.formats->nb_formats; j++) {
            int out_format = outlink->incfg.formats->formats[j];
            int out_bps    = av_get_bytes_per_sample(out_format);
            int score;

            if (av_get_packed_sample_fmt(out_format) == format ||
                av_get_planar_sample_fmt(out_format) == format) {
                best_idx   = j;
                break;
            }

            /* for s32 and float prefer double to prevent loss of information */
            if (bps == 4 && out_bps == 8) {
                best_idx = j;
                break;
            }

            /* prefer closest higher or equal bps */
            score = -abs(out_bps - bps);
            if (out_bps >= bps)
                score += INT_MAX/2;

            if (score > best_score) {
                best_score = score;
                best_idx   = j;
            }
        }
        av_assert0(best_idx >= 0);
        FFSWAP(int, outlink->incfg.formats->formats[0],
               outlink->incfg.formats->formats[best_idx]);
    }
}

static void swap_sample_fmts(AVFilterGraph *graph)
{
    int i;

    for (i = 0; i < graph->nb_filters; i++)
        swap_sample_fmts_on_filter(graph->filters[i]);

}

static int pick_formats(AVFilterGraph *graph)
{
    int i, j, ret;
    int change;

    do{
        change = 0;
        for (i = 0; i < graph->nb_filters; i++) {
            AVFilterContext *filter = graph->filters[i];
            if (filter->nb_inputs){
                for (j = 0; j < filter->nb_inputs; j++){
                    if (filter->inputs[j]->incfg.formats && filter->inputs[j]->incfg.formats->nb_formats == 1) {
                        if ((ret = pick_format(filter->inputs[j], NULL)) < 0)
                            return ret;
                        change = 1;
                    }
                }
            }
            if (filter->nb_outputs){
                for (j = 0; j < filter->nb_outputs; j++){
                    if (filter->outputs[j]->incfg.formats && filter->outputs[j]->incfg.formats->nb_formats == 1) {
                        if ((ret = pick_format(filter->outputs[j], NULL)) < 0)
                            return ret;
                        change = 1;
                    }
                }
            }
            if (filter->nb_inputs && filter->nb_outputs && filter->inputs[0]->format>=0) {
                for (j = 0; j < filter->nb_outputs; j++) {
                    if (filter->outputs[j]->format<0) {
                        if ((ret = pick_format(filter->outputs[j], filter->inputs[0])) < 0)
                            return ret;
                        change = 1;
                    }
                }
            }
        }
    }while(change);

    for (i = 0; i < graph->nb_filters; i++) {
        AVFilterContext *filter = graph->filters[i];

        for (j = 0; j < filter->nb_inputs; j++)
            if ((ret = pick_format(filter->inputs[j], NULL)) < 0)
                return ret;
        for (j = 0; j < filter->nb_outputs; j++)
            if ((ret = pick_format(filter->outputs[j], NULL)) < 0)
                return ret;
    }
    return 0;
}

/**
 * Configure the formats of all the links in the graph.
 */
static int graph_config_formats(AVFilterGraph *graph, void *log_ctx)
{
    int ret;

    /* find supported formats from sub-filters, and merge along links */
    while ((ret = query_formats(graph, log_ctx)) == AVERROR(EAGAIN))
        av_log(graph, AV_LOG_DEBUG, "query_formats not finished\n");
    if (ret < 0)
        return ret;

    /* Once everything is merged, it's possible that we'll still have
     * multiple valid media format choices. We try to minimize the amount
     * of format conversion inside filters */
    if ((ret = reduce_formats(graph)) < 0)
        return ret;

    /* for audio filters, ensure the best format, sample rate and channel layout
     * is selected */
    swap_sample_fmts(graph);
    swap_samplerates(graph);
    swap_channel_layouts(graph);

    if ((ret = pick_formats(graph)) < 0)
        return ret;

    return 0;
}

static int graph_config_pointers(AVFilterGraph *graph, void *log_ctx)
{
    unsigned i, j;
    int sink_links_count = 0, n = 0;
    AVFilterContext *f;
    FilterLinkInternal **sinks;

    for (i = 0; i < graph->nb_filters; i++) {
        f = graph->filters[i];
        for (j = 0; j < f->nb_inputs; j++) {
            ff_link_internal(f->inputs[j])->age_index  = -1;
        }
        for (j = 0; j < f->nb_outputs; j++) {
            ff_link_internal(f->outputs[j])->age_index = -1;
        }
        if (!f->nb_outputs) {
            if (f->nb_inputs > INT_MAX - sink_links_count)
                return AVERROR(EINVAL);
            sink_links_count += f->nb_inputs;
        }
    }
    sinks = av_calloc(sink_links_count, sizeof(*sinks));
    if (!sinks)
        return AVERROR(ENOMEM);
    for (i = 0; i < graph->nb_filters; i++) {
        f = graph->filters[i];
        if (!f->nb_outputs) {
            for (j = 0; j < f->nb_inputs; j++) {
                sinks[n] = ff_link_internal(f->inputs[j]);
                sinks[n]->age_index = n;
                n++;
            }
        }
    }
    av_assert0(n == sink_links_count);
    fffiltergraph(graph)->sink_links       = sinks;
    fffiltergraph(graph)->sink_links_count = sink_links_count;
    return 0;
}

int avfilter_graph_config(AVFilterGraph *graphctx, void *log_ctx)
{
    int ret;

    if ((ret = graph_check_validity(graphctx, log_ctx)))
        return ret;
    if ((ret = graph_config_formats(graphctx, log_ctx)))
        return ret;
    if ((ret = graph_config_links(graphctx, log_ctx)))
        return ret;
    if ((ret = graph_check_links(graphctx, log_ctx)))
        return ret;
    if ((ret = graph_config_pointers(graphctx, log_ctx)))
        return ret;

    return 0;
}

int avfilter_graph_send_command(AVFilterGraph *graph, const char *target, const char *cmd, const char *arg, char *res, int res_len, int flags)
{
    int i, r = AVERROR(ENOSYS);

    if (!graph)
        return r;

    if ((flags & AVFILTER_CMD_FLAG_ONE) && !(flags & AVFILTER_CMD_FLAG_FAST)) {
        r = avfilter_graph_send_command(graph, target, cmd, arg, res, res_len, flags | AVFILTER_CMD_FLAG_FAST);
        if (r != AVERROR(ENOSYS))
            return r;
    }

    if (res_len && res)
        res[0] = 0;

    for (i = 0; i < graph->nb_filters; i++) {
        AVFilterContext *filter = graph->filters[i];
        if (!strcmp(target, "all") || (filter->name && !strcmp(target, filter->name)) || !strcmp(target, filter->filter->name)) {
            r = avfilter_process_command(filter, cmd, arg, res, res_len, flags);
            if (r != AVERROR(ENOSYS)) {
                if ((flags & AVFILTER_CMD_FLAG_ONE) || r < 0)
                    return r;
            }
        }
    }

    return r;
}

int avfilter_graph_queue_command(AVFilterGraph *graph, const char *target, const char *command, const char *arg, int flags, double ts)
{
    int i;

    if(!graph)
        return 0;

    for (i = 0; i < graph->nb_filters; i++) {
        AVFilterContext *filter = graph->filters[i];
        FFFilterContext *ctxi   = fffilterctx(filter);
        if(filter && (!strcmp(target, "all") || !strcmp(target, filter->name) || !strcmp(target, filter->filter->name))){
            AVFilterCommand **queue = &ctxi->command_queue, *next;
            while (*queue && (*queue)->time <= ts)
                queue = &(*queue)->next;
            next = *queue;
            *queue = av_mallocz(sizeof(AVFilterCommand));
            if (!*queue)
                return AVERROR(ENOMEM);

            (*queue)->command = av_strdup(command);
            (*queue)->arg     = av_strdup(arg);
            (*queue)->time    = ts;
            (*queue)->flags   = flags;
            (*queue)->next    = next;
            if(flags & AVFILTER_CMD_FLAG_ONE)
                return 0;
        }
    }

    return 0;
}

static void heap_bubble_up(FFFilterGraph *graph,
                           FilterLinkInternal *li, int index)
{
    FilterLinkInternal **links = graph->sink_links;

    av_assert0(index >= 0);

    while (index) {
        int parent = (index - 1) >> 1;
        if (links[parent]->l.current_pts_us >= li->l.current_pts_us)
            break;
        links[index] = links[parent];
        links[index]->age_index = index;
        index = parent;
    }
    links[index] = li;
    li->age_index = index;
}

static void heap_bubble_down(FFFilterGraph *graph,
                             FilterLinkInternal *li, int index)
{
    FilterLinkInternal **links = graph->sink_links;

    av_assert0(index >= 0);

    while (1) {
        int child = 2 * index + 1;
        if (child >= graph->sink_links_count)
            break;
        if (child + 1 < graph->sink_links_count &&
            links[child + 1]->l.current_pts_us < links[child]->l.current_pts_us)
            child++;
        if (li->l.current_pts_us < links[child]->l.current_pts_us)
            break;
        links[index] = links[child];
        links[index]->age_index = index;
        index = child;
    }
    links[index] = li;
    li->age_index = index;
}

void ff_avfilter_graph_update_heap(AVFilterGraph *graph, FilterLinkInternal *li)
{
    FFFilterGraph  *graphi = fffiltergraph(graph);

    heap_bubble_up  (graphi, li, li->age_index);
    heap_bubble_down(graphi, li, li->age_index);
}

int avfilter_graph_request_oldest(AVFilterGraph *graph)
{
    FFFilterGraph *graphi = fffiltergraph(graph);
    FilterLinkInternal *oldesti = graphi->sink_links[0];
    AVFilterLink *oldest = &oldesti->l.pub;
    int64_t frame_count;
    int r;

    while (graphi->sink_links_count) {
        oldesti = graphi->sink_links[0];
        oldest  = &oldesti->l.pub;
        if (fffilter(oldest->dst->filter)->activate) {
            r = av_buffersink_get_frame_flags(oldest->dst, NULL,
                                              AV_BUFFERSINK_FLAG_PEEK);
            if (r != AVERROR_EOF)
                return r;
        } else {
            r = ff_request_frame(oldest);
        }
        if (r != AVERROR_EOF)
            break;
        av_log(oldest->dst, AV_LOG_DEBUG, "EOF on sink link %s:%s.\n",
               oldest->dst->name,
               oldest->dstpad->name);
        /* EOF: remove the link from the heap */
        if (oldesti->age_index < --graphi->sink_links_count)
            heap_bubble_down(graphi, graphi->sink_links[graphi->sink_links_count],
                             oldesti->age_index);
        oldesti->age_index = -1;
    }
    if (!graphi->sink_links_count)
        return AVERROR_EOF;
    av_assert1(!fffilter(oldest->dst->filter)->activate);
    av_assert1(oldesti->age_index >= 0);
    frame_count = oldesti->l.frame_count_out;
    while (frame_count == oldesti->l.frame_count_out) {
        r = ff_filter_graph_run_once(graph);
        if (r == AVERROR(EAGAIN) &&
            !oldesti->frame_wanted_out && !oldesti->frame_blocked_in &&
            !oldesti->status_in)
            (void)ff_request_frame(oldest);
        else if (r < 0)
            return r;
    }
    return 0;
}

int ff_filter_graph_run_once(AVFilterGraph *graph)
{
    FFFilterContext *ctxi;
    unsigned i;

    av_assert0(graph->nb_filters);
    ctxi = fffilterctx(graph->filters[0]);
    for (i = 1; i < graph->nb_filters; i++) {
        FFFilterContext *ctxi_other = fffilterctx(graph->filters[i]);

        if (ctxi_other->ready > ctxi->ready)
            ctxi = ctxi_other;
    }

    if (!ctxi->ready)
        return AVERROR(EAGAIN);
    return ff_filter_activate(&ctxi->p);
}

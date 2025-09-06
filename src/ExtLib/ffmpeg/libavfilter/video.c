/*
 * Copyright 2007 Bobby Bingham
 * Copyright Stefano Sabatini <stefasab gmail com>
 * Copyright Vitor Sessak <vitor1001 gmail com>
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

#include <string.h>
#include <stdio.h>

#include "libavutil/buffer.h"
#include "libavutil/cpu.h"
#include "libavutil/hwcontext.h"
#include "libavutil/pixfmt.h"

#include "avfilter.h"
#include "avfilter_internal.h"
#include "filters.h"
#include "framepool.h"
#include "video.h"

const AVFilterPad ff_video_default_filterpad[1] = {
    {
        .name = "default",
        .type = AVMEDIA_TYPE_VIDEO,
    }
};

AVFrame *ff_null_get_video_buffer(AVFilterLink *link, int w, int h)
{
    return ff_get_video_buffer(link->dst->outputs[0], w, h);
}

AVFrame *ff_default_get_video_buffer2(AVFilterLink *link, int w, int h, int align)
{
    FilterLinkInternal *const li = ff_link_internal(link);
    AVFrame *frame = NULL;
    int pool_width = 0;
    int pool_height = 0;
    int pool_align = 0;
    enum AVPixelFormat pool_format = AV_PIX_FMT_NONE;

    if (li->l.hw_frames_ctx &&
        ((AVHWFramesContext*)li->l.hw_frames_ctx->data)->format == link->format) {
        int ret;
        frame = av_frame_alloc();

        if (!frame)
            return NULL;

        ret = av_hwframe_get_buffer(li->l.hw_frames_ctx, frame, 0);
        if (ret < 0)
            av_frame_free(&frame);

        return frame;
    }

    if (!li->frame_pool) {
        li->frame_pool = ff_frame_pool_video_init(CONFIG_MEMORY_POISONING
                                                     ? NULL
                                                     : av_buffer_allocz,
                                                  w, h, link->format, align);
        if (!li->frame_pool)
            return NULL;
    } else {
        if (ff_frame_pool_get_video_config(li->frame_pool,
                                           &pool_width, &pool_height,
                                           &pool_format, &pool_align) < 0) {
            return NULL;
        }

        if (pool_width != w || pool_height != h ||
            pool_format != link->format || pool_align != align) {

            ff_frame_pool_uninit(&li->frame_pool);
            li->frame_pool = ff_frame_pool_video_init(CONFIG_MEMORY_POISONING
                                                         ? NULL
                                                         : av_buffer_allocz,
                                                      w, h, link->format, align);
            if (!li->frame_pool)
                return NULL;
        }
    }

    frame = ff_frame_pool_get(li->frame_pool);
    if (!frame)
        return NULL;

    frame->sample_aspect_ratio = link->sample_aspect_ratio;
    frame->colorspace  = link->colorspace;
    frame->color_range = link->color_range;
    frame->alpha_mode  = link->alpha_mode;

    return frame;
}

AVFrame *ff_default_get_video_buffer(AVFilterLink *link, int w, int h)
{
    return ff_default_get_video_buffer2(link, w, h, av_cpu_max_align());
}

AVFrame *ff_get_video_buffer(AVFilterLink *link, int w, int h)
{
    AVFrame *ret = NULL;

    FF_TPRINTF_START(NULL, get_video_buffer); ff_tlog_link(NULL, link, 1);

    if (link->dstpad->get_buffer.video)
        ret = link->dstpad->get_buffer.video(link, w, h);

    if (!ret)
        ret = ff_default_get_video_buffer(link, w, h);

    return ret;
}

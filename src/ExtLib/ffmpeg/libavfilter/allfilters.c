/*
 * filter registration
 * Copyright (c) 2008 Vitor Sessak
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

#include "libavutil/thread.h"
#include "avfilter.h"
#include "config.h"

extern AVFilter ff_af_aresample;
extern AVFilter ff_af_atempo;
// extern AVFilter ff_af_lowpass;

extern AVFilter ff_asrc_abuffer;
extern AVFilter ff_asink_abuffer;

#include "libavfilter/filter_list.c"


const AVFilter *av_filter_iterate(void **opaque)
{
    uintptr_t i = (uintptr_t)*opaque;
    const AVFilter *f = filter_list[i];

    if (f)
        *opaque = (void*)(i + 1);

    return f;
}

const AVFilter *avfilter_get_by_name(const char *name)
{
    const AVFilter *f = NULL;
    void *opaque = 0;

    if (!name)
        return NULL;

    while ((f = av_filter_iterate(&opaque)))
        if (!strcmp(f->name, name))
            return (AVFilter *)f;

    return NULL;
}


#if FF_API_NEXT
FF_DISABLE_DEPRECATION_WARNINGS
static AVOnce av_filter_next_init = AV_ONCE_INIT;

static void av_filter_init_next(void)
{
    AVFilter *prev = NULL, *p;
    void *i = 0;
    while ((p = (AVFilter*)av_filter_iterate(&i))) {
        if (prev)
            prev->next = p;
        prev = p;
    }
}

void avfilter_register_all(void)
{
    ff_thread_once(&av_filter_next_init, av_filter_init_next);
}

int avfilter_register(AVFilter *filter)
{
    ff_thread_once(&av_filter_next_init, av_filter_init_next);

    return 0;
}

const AVFilter *avfilter_next(const AVFilter *prev)
{
    ff_thread_once(&av_filter_next_init, av_filter_init_next);

    return prev ? prev->next : filter_list[0];
}

FF_ENABLE_DEPRECATION_WARNINGS
#endif

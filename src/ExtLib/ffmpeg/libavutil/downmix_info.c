/*
 * Copyright (c) 2014 Tim Walker <tdskywalker@gmail.com>
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

#include "downmix_info.h"
#include "frame.h"
#include "mem.h"

AVDownmixInfo *av_downmix_info_update_side_data(AVFrame *frame)
{
    AVFrameSideData *side_data;

    side_data = av_frame_get_side_data(frame, AV_FRAME_DATA_DOWNMIX_INFO);

    if (!side_data) {
        side_data = av_frame_new_side_data(frame, AV_FRAME_DATA_DOWNMIX_INFO,
                                           sizeof(AVDownmixInfo));
        if (side_data)
            memset(side_data->data, 0, sizeof(AVDownmixInfo));
    }

    if (!side_data)
        return NULL;

    return (AVDownmixInfo*)side_data->data;
}

#define MAX_CHANNELS 64

static const int type_map[] = {
    [AV_DOWNMIX_TYPE_UNKNOWN] = 0,
    [AV_DOWNMIX_TYPE_LORO]    = 2,
    [AV_DOWNMIX_TYPE_LTRT]    = 2,
    [AV_DOWNMIX_TYPE_DPLII]   = 2,
};

AVDownmixMatrix *av_downmix_matrix_alloc(enum AVDownmixType type,
                                         int in_ch_count, size_t *out_size)
{
    struct TestStruct {
        AVDownmixMatrix p;
        AVDownmixCoeff  b;
    };
    const size_t coeffs_offset = offsetof(struct TestStruct, b);
    size_t size = coeffs_offset;
    unsigned int nb_coeffs;
    AVDownmixMatrix *dc;

    if ((unsigned)type >= FF_ARRAY_ELEMS(type_map) ||
        (unsigned)in_ch_count > MAX_CHANNELS)
        return NULL;

    nb_coeffs = type_map[type] * in_ch_count;
    size += sizeof(AVDownmixCoeff) * nb_coeffs;

    dc = av_mallocz(size);
    if (!dc)
        return NULL;

    dc->downmix_type  = type;
    dc->nb_coeffs     = nb_coeffs;
    dc->in_ch_count   = in_ch_count;
    dc->coeffs_offset = coeffs_offset;

    if (out_size)
        *out_size = size;

    return dc;
}

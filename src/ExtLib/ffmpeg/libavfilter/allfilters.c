// file much modified for MPC-BE

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

#include "avfilter.h"
#include "config.h"


#define REGISTER_FILTER(X, x, y)                                        \
    {                                                                   \
        extern AVFilter ff_##y##_##x;                                   \
        if (CONFIG_##X##_FILTER)                                        \
            avfilter_register(&ff_##y##_##x);                           \
    }

#define REGISTER_FILTER_UNCONDITIONAL(x)                                \
    {                                                                   \
        extern AVFilter ff_##x;                                         \
        avfilter_register(&ff_##x);                                     \
    }

void avfilter_register_all(void)
{
    static int initialized;

    if (initialized)
        return;
    initialized = 1;

    REGISTER_FILTER(ATEMPO,         atempo,         af);
    REGISTER_FILTER(LOWPASS,        lowpass,        af);

    /* those filters are part of public or internal API => registered
     * unconditionally */
    REGISTER_FILTER_UNCONDITIONAL(asrc_abuffer);
    REGISTER_FILTER_UNCONDITIONAL(asink_abuffer);
}

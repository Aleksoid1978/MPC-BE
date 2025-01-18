/*
 * Copyright (c) 2013 Paul B Mahol
 * Copyright (c) 2006-2008 Rob Sykes <robs@users.sourceforge.net>
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

/*
 * 2-pole filters designed by Robert Bristow-Johnson <rbj@audioimagination.com>
 *   see http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
 *
 * 1-pole filters based on code (c) 2000 Chris Bagwell <cbagwell@sprynet.com>
 *   Algorithms: Recursive single pole low/high pass filter
 *   Reference: The Scientist and Engineer's Guide to Digital Signal Processing
 *
 *   low-pass: output[N] = input[N] * A + output[N-1] * B
 *     X = exp(-2.0 * pi * Fc)
 *     A = 1 - X
 *     B = X
 *     Fc = cutoff freq / sample rate
 *
 *     Mimics an RC low-pass filter:
 *
 *     ---/\/\/\/\----------->
 *                   |
 *                  --- C
 *                  ---
 *                   |
 *                   |
 *                   V
 *
 *   high-pass: output[N] = A0 * input[N] + A1 * input[N-1] + B1 * output[N-1]
 *     X  = exp(-2.0 * pi * Fc)
 *     A0 = (1 + X) / 2
 *     A1 = -(1 + X) / 2
 *     B1 = X
 *     Fc = cutoff freq / sample rate
 *
 *     Mimics an RC high-pass filter:
 *
 *         || C
 *     ----||--------->
 *         ||    |
 *               <
 *               > R
 *               <
 *               |
 *               V
 */

#include "config_components.h"

#include "libavutil/avassert.h"
#include "libavutil/channel_layout.h"
#include "libavutil/ffmath.h"
#include "libavutil/mem.h"
#include "libavutil/opt.h"
#include "audio.h"
#include "avfilter.h"
#include "filters.h"
#include "formats.h"

enum FilterType {
    biquad,
    equalizer,
    bass,
    treble,
    bandpass,
    bandreject,
    allpass,
    highpass,
    lowpass,
    lowshelf,
    highshelf,
    tiltshelf,
};

enum WidthType {
    NONE,
    HERTZ,
    OCTAVE,
    QFACTOR,
    SLOPE,
    KHERTZ,
    NB_WTYPE,
};

enum TransformType {
    DI,
    DII,
    TDI,
    TDII,
    LATT,
    SVF,
    ZDF,
    NB_TTYPE,
};

typedef struct BiquadsContext {
    const AVClass *class;

    enum FilterType filter_type;
    int width_type;
    int poles;
    int csg;
    int transform_type;
    int precision;
    int block_samples;

    int bypass;

    double gain;
    double frequency;
    double width;
    double mix;
    char *ch_layout_str;
    AVChannelLayout ch_layout;
    int normalize;
    int order;

    double a_double[3];
    double b_double[3];

    float a_float[3];
    float b_float[3];

    double oa[3];
    double ob[3];

    AVFrame *block[3];

    int *clip;
    AVFrame *cache[2];
    int block_align;

    int64_t pts;
    int nb_samples;

    void (*filter)(struct BiquadsContext *s, const void *ibuf, void *obuf, int len,
                   void *cache, int *clip, int disabled);
} BiquadsContext;

static int query_formats(const AVFilterContext *ctx,
                         AVFilterFormatsConfig **cfg_in,
                         AVFilterFormatsConfig **cfg_out)
{
    const BiquadsContext *s = ctx->priv;
    static const enum AVSampleFormat auto_sample_fmts[] = {
        AV_SAMPLE_FMT_S16P,
        AV_SAMPLE_FMT_S32P,
        AV_SAMPLE_FMT_FLTP,
        AV_SAMPLE_FMT_DBLP,
        AV_SAMPLE_FMT_NONE
    };
    enum AVSampleFormat sample_fmts[] = {
        AV_SAMPLE_FMT_S16P,
        AV_SAMPLE_FMT_NONE
    };
    const enum AVSampleFormat *sample_fmts_list = sample_fmts;
    int ret;

    switch (s->precision) {
    case 0:
        sample_fmts[0] = AV_SAMPLE_FMT_S16P;
        break;
    case 1:
        sample_fmts[0] = AV_SAMPLE_FMT_S32P;
        break;
    case 2:
        sample_fmts[0] = AV_SAMPLE_FMT_FLTP;
        break;
    case 3:
        sample_fmts[0] = AV_SAMPLE_FMT_DBLP;
        break;
    default:
        sample_fmts_list = auto_sample_fmts;
        break;
    }
    ret = ff_set_common_formats_from_list2(ctx, cfg_in, cfg_out, sample_fmts_list);
    if (ret < 0)
        return ret;

    return 0;
}

#define BIQUAD_FILTER(name, type, ftype, min, max, need_clipping)             \
static void biquad_## name (BiquadsContext *s,                                \
                            const void *input, void *output, int len,         \
                            void *cache, int *clippings, int disabled)        \
{                                                                             \
    const type *ibuf = input;                                                 \
    type *obuf = output;                                                      \
    ftype *fcache = cache;                                                    \
    ftype i1 = fcache[0], i2 = fcache[1], o1 = fcache[2], o2 = fcache[3];     \
    ftype *a = s->a_##ftype;                                                  \
    ftype *b = s->b_##ftype;                                                  \
    ftype a1 = -a[1];                                                         \
    ftype a2 = -a[2];                                                         \
    ftype b0 = b[0];                                                          \
    ftype b1 = b[1];                                                          \
    ftype b2 = b[2];                                                          \
    ftype wet = s->mix;                                                       \
    ftype dry = 1. - wet;                                                     \
    ftype out;                                                                \
    int i;                                                                    \
                                                                              \
    for (i = 0; i+1 < len; i++) {                                             \
        o2 = i2 * b2 + i1 * b1 + ibuf[i] * b0 + o2 * a2 + o1 * a1;            \
        i2 = ibuf[i];                                                         \
        out = o2 * wet + i2 * dry;                                            \
        if (disabled) {                                                       \
            obuf[i] = i2;                                                     \
        } else if (need_clipping && out < min) {                              \
            (*clippings)++;                                                   \
            obuf[i] = min;                                                    \
        } else if (need_clipping && out > max) {                              \
            (*clippings)++;                                                   \
            obuf[i] = max;                                                    \
        } else {                                                              \
            obuf[i] = out;                                                    \
        }                                                                     \
        i++;                                                                  \
        o1 = i1 * b2 + i2 * b1 + ibuf[i] * b0 + o1 * a2 + o2 * a1;            \
        i1 = ibuf[i];                                                         \
        out = o1 * wet + i1 * dry;                                            \
        if (disabled) {                                                       \
            obuf[i] = i1;                                                     \
        } else if (need_clipping && out < min) {                              \
            (*clippings)++;                                                   \
            obuf[i] = min;                                                    \
        } else if (need_clipping && out > max) {                              \
            (*clippings)++;                                                   \
            obuf[i] = max;                                                    \
        } else {                                                              \
            obuf[i] = out;                                                    \
        }                                                                     \
    }                                                                         \
    if (i < len) {                                                            \
        ftype o0 = ibuf[i] * b0 + i1 * b1 + i2 * b2 + o1 * a1 + o2 * a2;      \
        i2 = i1;                                                              \
        i1 = ibuf[i];                                                         \
        o2 = o1;                                                              \
        o1 = o0;                                                              \
        out = o0 * wet + i1 * dry;                                            \
        if (disabled) {                                                       \
            obuf[i] = i1;                                                     \
        } else if (need_clipping && out < min) {                              \
            (*clippings)++;                                                   \
            obuf[i] = min;                                                    \
        } else if (need_clipping && out > max) {                              \
            (*clippings)++;                                                   \
            obuf[i] = max;                                                    \
        } else {                                                              \
            obuf[i] = out;                                                    \
        }                                                                     \
    }                                                                         \
    fcache[0] = i1;                                                           \
    fcache[1] = i2;                                                           \
    fcache[2] = o1;                                                           \
    fcache[3] = o2;                                                           \
}

BIQUAD_FILTER(s16, int16_t, float,  INT16_MIN, INT16_MAX, 1)
BIQUAD_FILTER(s32, int32_t, double, INT32_MIN, INT32_MAX, 1)
BIQUAD_FILTER(flt, float,   float,  -1.f, 1.f, 0)
BIQUAD_FILTER(dbl, double,  double, -1.,  1.,  0)

#define BIQUAD_DII_FILTER(name, type, ftype, min, max, need_clipping)         \
static void biquad_dii_## name (BiquadsContext *s,                            \
                            const void *input, void *output, int len,         \
                            void *cache, int *clippings, int disabled)        \
{                                                                             \
    const type *ibuf = input;                                                 \
    type *obuf = output;                                                      \
    ftype *fcache = cache;                                                    \
    ftype *a = s->a_##ftype;                                                  \
    ftype *b = s->b_##ftype;                                                  \
    ftype a1 = -a[1];                                                         \
    ftype a2 = -a[2];                                                         \
    ftype b0 = b[0];                                                          \
    ftype b1 = b[1];                                                          \
    ftype b2 = b[2];                                                          \
    ftype w1 = fcache[0];                                                     \
    ftype w2 = fcache[1];                                                     \
    ftype wet = s->mix;                                                       \
    ftype dry = 1. - wet;                                                     \
    ftype in, out, w0;                                                        \
                                                                              \
    for (int i = 0; i < len; i++) {                                           \
        in = ibuf[i];                                                         \
        w0 = in + a1 * w1 + a2 * w2;                                          \
        out = b0 * w0 + b1 * w1 + b2 * w2;                                    \
        w2 = w1;                                                              \
        w1 = w0;                                                              \
        out = out * wet + in * dry;                                           \
        if (disabled) {                                                       \
            obuf[i] = in;                                                     \
        } else if (need_clipping && out < min) {                              \
            (*clippings)++;                                                   \
            obuf[i] = min;                                                    \
        } else if (need_clipping && out > max) {                              \
            (*clippings)++;                                                   \
            obuf[i] = max;                                                    \
        } else {                                                              \
            obuf[i] = out;                                                    \
        }                                                                     \
    }                                                                         \
    fcache[0] = w1;                                                           \
    fcache[1] = w2;                                                           \
}

BIQUAD_DII_FILTER(s16, int16_t, float,  INT16_MIN, INT16_MAX, 1)
BIQUAD_DII_FILTER(s32, int32_t, double, INT32_MIN, INT32_MAX, 1)
BIQUAD_DII_FILTER(flt, float,   float,  -1.f, 1.f, 0)
BIQUAD_DII_FILTER(dbl, double,  double, -1.,  1.,  0)

#define BIQUAD_TDI_FILTER(name, type, ftype, min, max, need_clipping)         \
static void biquad_tdi_## name (BiquadsContext *s,                            \
                            const void *input, void *output, int len,         \
                            void *cache, int *clippings, int disabled)        \
{                                                                             \
    const type *ibuf = input;                                                 \
    type *obuf = output;                                                      \
    ftype *fcache = cache;                                                    \
    ftype *a = s->a_##ftype;                                                  \
    ftype *b = s->b_##ftype;                                                  \
    ftype a1 = -a[1];                                                         \
    ftype a2 = -a[2];                                                         \
    ftype b0 = b[0];                                                          \
    ftype b1 = b[1];                                                          \
    ftype b2 = b[2];                                                          \
    ftype s1 = fcache[0];                                                     \
    ftype s2 = fcache[1];                                                     \
    ftype s3 = fcache[2];                                                     \
    ftype s4 = fcache[3];                                                     \
    ftype wet = s->mix;                                                       \
    ftype dry = 1. - wet;                                                     \
    ftype in, out;                                                            \
                                                                              \
    for (int i = 0; i < len; i++) {                                           \
        ftype t1, t2, t3, t4;                                                 \
        in = ibuf[i] + s1;                                                    \
        t1 = in * a1 + s2;                                                    \
        t2 = in * a2;                                                         \
        t3 = in * b1 + s4;                                                    \
        t4 = in * b2;                                                         \
        out = b0 * in + s3;                                                   \
        out = out * wet + in * dry;                                           \
        s1 = t1; s2 = t2; s3 = t3; s4 = t4;                                   \
        if (disabled) {                                                       \
            obuf[i] = in;                                                     \
        } else if (need_clipping && out < min) {                              \
            (*clippings)++;                                                   \
            obuf[i] = min;                                                    \
        } else if (need_clipping && out > max) {                              \
            (*clippings)++;                                                   \
            obuf[i] = max;                                                    \
        } else {                                                              \
            obuf[i] = out;                                                    \
        }                                                                     \
    }                                                                         \
                                                                              \
    fcache[0] = s1;                                                           \
    fcache[1] = s2;                                                           \
    fcache[2] = s3;                                                           \
    fcache[3] = s4;                                                           \
}

BIQUAD_TDI_FILTER(s16, int16_t, float,  INT16_MIN, INT16_MAX, 1)
BIQUAD_TDI_FILTER(s32, int32_t, double, INT32_MIN, INT32_MAX, 1)
BIQUAD_TDI_FILTER(flt, float,   float,  -1.f, 1.f, 0)
BIQUAD_TDI_FILTER(dbl, double,  double, -1.,  1.,  0)

#define BIQUAD_TDII_FILTER(name, type, ftype, min, max, need_clipping)        \
static void biquad_tdii_## name (BiquadsContext *s,                           \
                            const void *input, void *output, int len,         \
                            void *cache, int *clippings, int disabled)        \
{                                                                             \
    const type *ibuf = input;                                                 \
    type *obuf = output;                                                      \
    ftype *fcache = cache;                                                    \
    ftype *a = s->a_##ftype;                                                  \
    ftype *b = s->b_##ftype;                                                  \
    ftype a1 = -a[1];                                                         \
    ftype a2 = -a[2];                                                         \
    ftype b0 = b[0];                                                          \
    ftype b1 = b[1];                                                          \
    ftype b2 = b[2];                                                          \
    ftype w1 = fcache[0];                                                     \
    ftype w2 = fcache[1];                                                     \
    ftype wet = s->mix;                                                       \
    ftype dry = 1. - wet;                                                     \
    ftype in, out;                                                            \
                                                                              \
    for (int i = 0; i < len; i++) {                                           \
        in = ibuf[i];                                                         \
        out = b0 * in + w1;                                                   \
        w1 = b1 * in + w2 + a1 * out;                                         \
        w2 = b2 * in + a2 * out;                                              \
        out = out * wet + in * dry;                                           \
        if (disabled) {                                                       \
            obuf[i] = in;                                                     \
        } else if (need_clipping && out < min) {                              \
            (*clippings)++;                                                   \
            obuf[i] = min;                                                    \
        } else if (need_clipping && out > max) {                              \
            (*clippings)++;                                                   \
            obuf[i] = max;                                                    \
        } else {                                                              \
            obuf[i] = out;                                                    \
        }                                                                     \
    }                                                                         \
    fcache[0] = w1;                                                           \
    fcache[1] = w2;                                                           \
}

BIQUAD_TDII_FILTER(s16, int16_t, float,  INT16_MIN, INT16_MAX, 1)
BIQUAD_TDII_FILTER(s32, int32_t, double, INT32_MIN, INT32_MAX, 1)
BIQUAD_TDII_FILTER(flt, float,   float,  -1.f, 1.f, 0)
BIQUAD_TDII_FILTER(dbl, double,  double, -1.,  1.,  0)

#define BIQUAD_LATT_FILTER(name, type, ftype, min, max, need_clipping)        \
static void biquad_latt_## name (BiquadsContext *s,                           \
                           const void *input, void *output, int len,          \
                           void *cache, int *clippings, int disabled)         \
{                                                                             \
    const type *ibuf = input;                                                 \
    type *obuf = output;                                                      \
    ftype *fcache = cache;                                                    \
    ftype *a = s->a_##ftype;                                                  \
    ftype *b = s->b_##ftype;                                                  \
    ftype k0 = a[1];                                                          \
    ftype k1 = a[2];                                                          \
    ftype v0 = b[0];                                                          \
    ftype v1 = b[1];                                                          \
    ftype v2 = b[2];                                                          \
    ftype s0 = fcache[0];                                                     \
    ftype s1 = fcache[1];                                                     \
    ftype wet = s->mix;                                                       \
    ftype dry = 1. - wet;                                                     \
    ftype in, out;                                                            \
    ftype t0, t1;                                                             \
                                                                              \
    for (int i = 0; i < len; i++) {                                           \
        out  = 0.;                                                            \
        in   = ibuf[i];                                                       \
        t0   = in - k1 * s0;                                                  \
        t1   = t0 * k1 + s0;                                                  \
        out += t1 * v2;                                                       \
                                                                              \
        t0    = t0 - k0 * s1;                                                 \
        t1    = t0 * k0 + s1;                                                 \
        out  += t1 * v1;                                                      \
                                                                              \
        out  += t0 * v0;                                                      \
        s0    = t1;                                                           \
        s1    = t0;                                                           \
                                                                              \
        out = out * wet + in * dry;                                           \
        if (disabled) {                                                       \
            obuf[i] = in;                                                     \
        } else if (need_clipping && out < min) {                              \
            (*clippings)++;                                                   \
            obuf[i] = min;                                                    \
        } else if (need_clipping && out > max) {                              \
            (*clippings)++;                                                   \
            obuf[i] = max;                                                    \
        } else {                                                              \
            obuf[i] = out;                                                    \
        }                                                                     \
    }                                                                         \
    fcache[0] = s0;                                                           \
    fcache[1] = s1;                                                           \
}

BIQUAD_LATT_FILTER(s16, int16_t, float,  INT16_MIN, INT16_MAX, 1)
BIQUAD_LATT_FILTER(s32, int32_t, double, INT32_MIN, INT32_MAX, 1)
BIQUAD_LATT_FILTER(flt, float,   float,  -1.f, 1.f, 0)
BIQUAD_LATT_FILTER(dbl, double,  double, -1.,  1.,  0)

#define BIQUAD_SVF_FILTER(name, type, ftype, min, max, need_clipping)         \
static void biquad_svf_## name (BiquadsContext *s,                            \
                           const void *input, void *output, int len,          \
                           void *cache, int *clippings, int disabled)         \
{                                                                             \
    const type *ibuf = input;                                                 \
    type *obuf = output;                                                      \
    ftype *fcache = cache;                                                    \
    ftype *a = s->a_##ftype;                                                  \
    ftype *b = s->b_##ftype;                                                  \
    ftype a1 = a[1];                                                          \
    ftype a2 = a[2];                                                          \
    ftype b0 = b[0];                                                          \
    ftype b1 = b[1];                                                          \
    ftype b2 = b[2];                                                          \
    ftype s0 = fcache[0];                                                     \
    ftype s1 = fcache[1];                                                     \
    ftype wet = s->mix;                                                       \
    ftype dry = 1. - wet;                                                     \
    ftype in, out;                                                            \
    ftype t0, t1;                                                             \
                                                                              \
    for (int i = 0; i < len; i++) {                                           \
        in   = ibuf[i];                                                       \
        out  = b2 * in + s0;                                                  \
        t0   = b0 * in + a1 * s0 + s1;                                        \
        t1   = b1 * in + a2 * s0;                                             \
        s0   = t0;                                                            \
        s1   = t1;                                                            \
                                                                              \
        out = out * wet + in * dry;                                           \
        if (disabled) {                                                       \
            obuf[i] = in;                                                     \
        } else if (need_clipping && out < min) {                              \
            (*clippings)++;                                                   \
            obuf[i] = min;                                                    \
        } else if (need_clipping && out > max) {                              \
            (*clippings)++;                                                   \
            obuf[i] = max;                                                    \
        } else {                                                              \
            obuf[i] = out;                                                    \
        }                                                                     \
    }                                                                         \
    fcache[0] = s0;                                                           \
    fcache[1] = s1;                                                           \
}

BIQUAD_SVF_FILTER(s16, int16_t, float,  INT16_MIN, INT16_MAX, 1)
BIQUAD_SVF_FILTER(s32, int32_t, double, INT32_MIN, INT32_MAX, 1)
BIQUAD_SVF_FILTER(flt, float,   float,  -1.f, 1.f, 0)
BIQUAD_SVF_FILTER(dbl, double,  double, -1.,  1.,  0)

#define BIQUAD_ZDF_FILTER(name, type, ftype, min, max, need_clipping, two)    \
static void biquad_zdf_## name (BiquadsContext *s,                            \
                           const void *input, void *output, int len,          \
                           void *cache, int *clippings, int disabled)         \
{                                                                             \
    const type *ibuf = input;                                                 \
    type *obuf = output;                                                      \
    ftype *fcache = cache;                                                    \
    ftype *a = s->a_##ftype;                                                  \
    ftype *b = s->b_##ftype;                                                  \
    ftype m0 = b[0];                                                          \
    ftype m1 = b[1];                                                          \
    ftype m2 = b[2];                                                          \
    ftype a0 = a[0];                                                          \
    ftype a1 = a[1];                                                          \
    ftype a2 = a[2];                                                          \
    ftype b0 = fcache[0];                                                     \
    ftype b1 = fcache[1];                                                     \
    ftype wet = s->mix;                                                       \
    ftype dry = 1. - wet;                                                     \
    ftype out;                                                                \
                                                                              \
    for (int i = 0; i < len; i++) {                                           \
        const ftype in = ibuf[i];                                             \
        const ftype v0 = in;                                                  \
        const ftype v3 = v0 - b1;                                             \
        const ftype v1 = a0 * b0 + a1 * v3;                                   \
        const ftype v2 = b1 + a1 * b0 + a2 * v3;                              \
                                                                              \
        b0 = two * v1 - b0;                                                   \
        b1 = two * v2 - b1;                                                   \
                                                                              \
        out = m0 * v0 + m1 * v1 + m2 * v2;                                    \
        out = out * wet + in * dry;                                           \
        if (disabled) {                                                       \
            obuf[i] = in;                                                     \
        } else if (need_clipping && out < min) {                              \
            (*clippings)++;                                                   \
            obuf[i] = min;                                                    \
        } else if (need_clipping && out > max) {                              \
            (*clippings)++;                                                   \
            obuf[i] = max;                                                    \
        } else {                                                              \
            obuf[i] = out;                                                    \
        }                                                                     \
    }                                                                         \
    fcache[0] = b0;                                                           \
    fcache[1] = b1;                                                           \
}

BIQUAD_ZDF_FILTER(s16, int16_t, float,  INT16_MIN, INT16_MAX, 1, 2.f)
BIQUAD_ZDF_FILTER(s32, int32_t, double, INT32_MIN, INT32_MAX, 1, 2.0)
BIQUAD_ZDF_FILTER(flt, float,   float,  -1.f, 1.f, 0, 2.f)
BIQUAD_ZDF_FILTER(dbl, double,  double, -1.,  1.,  0, 2.0)

static void convert_dir2latt(BiquadsContext *s)
{
    double k0, k1, v0, v1, v2;

    k1 = s->a_double[2];
    k0 = s->a_double[1] / (1. + k1);
    v2 = s->b_double[2];
    v1 = s->b_double[1] - v2 * s->a_double[1];
    v0 = s->b_double[0] - v1 * k0 - v2 * k1;

    s->a_double[1] = k0;
    s->a_double[2] = k1;
    s->b_double[0] = v0;
    s->b_double[1] = v1;
    s->b_double[2] = v2;
}

static void convert_dir2svf(BiquadsContext *s)
{
    double a[2];
    double b[3];

    a[0] = -s->a_double[1];
    a[1] = -s->a_double[2];
    b[0] = s->b_double[1] - s->a_double[1] * s->b_double[0];
    b[1] = s->b_double[2] - s->a_double[2] * s->b_double[0];
    b[2] = s->b_double[0];

    s->a_double[1] = a[0];
    s->a_double[2] = a[1];
    s->b_double[0] = b[0];
    s->b_double[1] = b[1];
    s->b_double[2] = b[2];
}

static double convert_width2qfactor(double width,
                                    double frequency,
                                    double gain,
                                    double sample_rate,
                                    int width_type)
{
    double w0 = 2. * M_PI * frequency / sample_rate;
    double A = ff_exp10(gain / 40.);
    double ret;

    switch (width_type) {
    case NONE:
    case QFACTOR:
        ret = width;
        break;
    case HERTZ:
        ret = frequency / width;
        break;
    case KHERTZ:
        ret = frequency / (width * 1000.);
        break;
    case OCTAVE:
        ret = 1. / (2. * sinh(log(2.) / 2. * width * w0 / sin(w0)));
        break;
    case SLOPE:
        ret = 1. / sqrt((A + 1. / A) * (1. / width - 1.) + 2.);
        break;
    default:
        av_assert0(0);
        break;
    }

    return ret;
}

static void convert_dir2zdf(BiquadsContext *s, int sample_rate)
{
    double Q = convert_width2qfactor(s->width, s->frequency, s->gain, sample_rate, s->width_type);
    double g, k, A;
    double a[3];
    double m[3];

    switch (s->filter_type) {
    case biquad:
        a[0] = s->oa[0];
        a[1] = s->oa[1];
        a[2] = s->oa[2];
        m[0] = s->ob[0];
        m[1] = s->ob[1];
        m[2] = s->ob[2];
        break;
    case equalizer:
        A = ff_exp10(s->gain / 40.);
        g = tan(M_PI * s->frequency / sample_rate);
        k = 1. / (Q * A);
        a[0] = 1. / (1. + g * (g + k));
        a[1] = g * a[0];
        a[2] = g * a[1];
        m[0] = 1.;
        m[1] = k * (A * A - 1.);
        m[2] = 0.;
        break;
    case bass:
    case lowshelf:
        A = ff_exp10(s->gain / 40.);
        g = tan(M_PI * s->frequency / sample_rate) / sqrt(A);
        k = 1. / Q;
        a[0] = 1. / (1. + g * (g + k));
        a[1] = g * a[0];
        a[2] = g * a[1];
        m[0] = 1.;
        m[1] = k * (A - 1.);
        m[2] = A * A - 1.;
        break;
    case tiltshelf:
        A = ff_exp10(s->gain / 20.);
        g = tan(M_PI * s->frequency / sample_rate) / sqrt(A);
        k = 1. / Q;
        a[0] = 1. / (1. + g * (g + k));
        a[1] = g * a[0];
        a[2] = g * a[1];
        m[0] = 1./ A;
        m[1] = k * (A - 1.) / A;
        m[2] = (A * A - 1.) / A;
        break;
    case treble:
    case highshelf:
        A = ff_exp10(s->gain / 40.);
        g = tan(M_PI * s->frequency / sample_rate) * sqrt(A);
        k = 1. / Q;
        a[0] = 1. / (1. + g * (g + k));
        a[1] = g * a[0];
        a[2] = g * a[1];
        m[0] = A * A;
        m[1] = k * (1. - A) * A;
        m[2] = 1. - A * A;
        break;
    case bandpass:
        g = tan(M_PI * s->frequency / sample_rate);
        k = 1. / Q;
        a[0] = 1. / (1. + g * (g + k));
        a[1] = g * a[0];
        a[2] = g * a[1];
        m[0] = 0.;
        m[1] = s->csg ? 1. : k;
        m[2] = 0.;
        break;
    case bandreject:
        g = tan(M_PI * s->frequency / sample_rate);
        k = 1. / Q;
        a[0] = 1. / (1. + g * (g + k));
        a[1] = g * a[0];
        a[2] = g * a[1];
        m[0] =  1.;
        m[1] = -k;
        m[2] =  0.;
        break;
    case lowpass:
        g = tan(M_PI * s->frequency / sample_rate);
        k = 1. / Q;
        a[0] = 1. / (1. + g * (g + k));
        a[1] = g * a[0];
        a[2] = g * a[1];
        m[0] = 0.;
        m[1] = 0.;
        m[2] = 1.;
        break;
    case highpass:
        g = tan(M_PI * s->frequency / sample_rate);
        k = 1. / Q;
        a[0] = 1. / (1. + g * (g + k));
        a[1] = g * a[0];
        a[2] = g * a[1];
        m[0] =  1.;
        m[1] = -k;
        m[2] = -1.;
        break;
    case allpass:
        g = tan(M_PI * s->frequency / sample_rate);
        k = 1. / Q;
        a[0] = 1. / (1. + g * (g + k));
        a[1] = g * a[0];
        a[2] = g * a[1];
        m[0] =  1.;
        m[1] = -2. * k;
        m[2] =  0.;
        break;
    default:
        av_assert0(0);
    }

    s->a_double[0] = a[0];
    s->a_double[1] = a[1];
    s->a_double[2] = a[2];
    s->b_double[0] = m[0];
    s->b_double[1] = m[1];
    s->b_double[2] = m[2];
}

static int config_filter(AVFilterLink *outlink, int reset)
{
    AVFilterContext *ctx    = outlink->src;
    BiquadsContext *s       = ctx->priv;
    AVFilterLink *inlink    = ctx->inputs[0];
    double gain = s->gain * ((s->filter_type == tiltshelf) + 1.);
    double A = ff_exp10(gain / 40);
    double w0 = 2 * M_PI * s->frequency / inlink->sample_rate;
    double K = tan(w0 / 2.);
    double alpha, beta;

    s->bypass = (((w0 > M_PI || w0 <= 0.) && reset) || (s->width <= 0.)) && (s->filter_type != biquad);
    if (s->bypass) {
        av_log(ctx, AV_LOG_WARNING, "Invalid frequency and/or width!\n");
        return 0;
    }

    if ((w0 > M_PI || w0 <= 0.) && (s->filter_type != biquad))
        return AVERROR(EINVAL);

    switch (s->width_type) {
    case NONE:
        alpha = 0.0;
        break;
    case HERTZ:
        alpha = sin(w0) / (2 * s->frequency / s->width);
        break;
    case KHERTZ:
        alpha = sin(w0) / (2 * s->frequency / (s->width * 1000));
        break;
    case OCTAVE:
        alpha = sin(w0) * sinh(log(2.) / 2 * s->width * w0 / sin(w0));
        break;
    case QFACTOR:
        alpha = sin(w0) / (2 * s->width);
        break;
    case SLOPE:
        alpha = sin(w0) / 2 * sqrt((A + 1 / A) * (1 / s->width - 1) + 2);
        break;
    default:
        av_assert0(0);
    }

    beta = 2 * sqrt(A);

    switch (s->filter_type) {
    case biquad:
        s->a_double[0] = s->oa[0];
        s->a_double[1] = s->oa[1];
        s->a_double[2] = s->oa[2];
        s->b_double[0] = s->ob[0];
        s->b_double[1] = s->ob[1];
        s->b_double[2] = s->ob[2];
        break;
    case equalizer:
        s->a_double[0] =   1 + alpha / A;
        s->a_double[1] =  -2 * cos(w0);
        s->a_double[2] =   1 - alpha / A;
        s->b_double[0] =   1 + alpha * A;
        s->b_double[1] =  -2 * cos(w0);
        s->b_double[2] =   1 - alpha * A;
        break;
    case bass:
        beta = sqrt((A * A + 1) - (A - 1) * (A - 1));
    case tiltshelf:
    case lowshelf:
        if (s->poles == 1) {
            double A = ff_exp10(gain / 20);
            double ro = -sin(w0 / 2. - M_PI_4) / sin(w0 / 2. + M_PI_4);
            double n = (A + 1) / (A - 1);
            double alpha1 = A == 1. ? 0. : n - FFSIGN(n) * sqrt(n * n - 1);
            double beta0 = ((1 + A) + (1 - A) * alpha1) * 0.5;
            double beta1 = ((1 - A) + (1 + A) * alpha1) * 0.5;

            s->a_double[0] = 1 + ro * alpha1;
            s->a_double[1] = -ro - alpha1;
            s->a_double[2] = 0;
            s->b_double[0] = beta0 + ro * beta1;
            s->b_double[1] = -beta1 - ro * beta0;
            s->b_double[2] = 0;
        } else {
            s->a_double[0] =          (A + 1) + (A - 1) * cos(w0) + beta * alpha;
            s->a_double[1] =    -2 * ((A - 1) + (A + 1) * cos(w0));
            s->a_double[2] =          (A + 1) + (A - 1) * cos(w0) - beta * alpha;
            s->b_double[0] =     A * ((A + 1) - (A - 1) * cos(w0) + beta * alpha);
            s->b_double[1] = 2 * A * ((A - 1) - (A + 1) * cos(w0));
            s->b_double[2] =     A * ((A + 1) - (A - 1) * cos(w0) - beta * alpha);
        }
        break;
    case treble:
        beta = sqrt((A * A + 1) - (A - 1) * (A - 1));
    case highshelf:
        if (s->poles == 1) {
            double A = ff_exp10(gain / 20);
            double ro = sin(w0 / 2. - M_PI_4) / sin(w0 / 2. + M_PI_4);
            double n = (A + 1) / (A - 1);
            double alpha1 = A == 1. ? 0. : n - FFSIGN(n) * sqrt(n * n - 1);
            double beta0 = ((1 + A) + (1 - A) * alpha1) * 0.5;
            double beta1 = ((1 - A) + (1 + A) * alpha1) * 0.5;

            s->a_double[0] = 1 + ro * alpha1;
            s->a_double[1] = ro + alpha1;
            s->a_double[2] = 0;
            s->b_double[0] = beta0 + ro * beta1;
            s->b_double[1] = beta1 + ro * beta0;
            s->b_double[2] = 0;
        } else {
            s->a_double[0] =          (A + 1) - (A - 1) * cos(w0) + beta * alpha;
            s->a_double[1] =     2 * ((A - 1) - (A + 1) * cos(w0));
            s->a_double[2] =          (A + 1) - (A - 1) * cos(w0) - beta * alpha;
            s->b_double[0] =     A * ((A + 1) + (A - 1) * cos(w0) + beta * alpha);
            s->b_double[1] =-2 * A * ((A - 1) + (A + 1) * cos(w0));
            s->b_double[2] =     A * ((A + 1) + (A - 1) * cos(w0) - beta * alpha);
        }
        break;
    case bandpass:
        if (s->csg) {
            s->a_double[0] =  1 + alpha;
            s->a_double[1] = -2 * cos(w0);
            s->a_double[2] =  1 - alpha;
            s->b_double[0] =  sin(w0) / 2;
            s->b_double[1] =  0;
            s->b_double[2] = -sin(w0) / 2;
        } else {
            s->a_double[0] =  1 + alpha;
            s->a_double[1] = -2 * cos(w0);
            s->a_double[2] =  1 - alpha;
            s->b_double[0] =  alpha;
            s->b_double[1] =  0;
            s->b_double[2] = -alpha;
        }
        break;
    case bandreject:
        s->a_double[0] =  1 + alpha;
        s->a_double[1] = -2 * cos(w0);
        s->a_double[2] =  1 - alpha;
        s->b_double[0] =  1;
        s->b_double[1] = -2 * cos(w0);
        s->b_double[2] =  1;
        break;
    case lowpass:
        if (s->poles == 1) {
            s->a_double[0] = 1;
            s->a_double[1] = -exp(-w0);
            s->a_double[2] = 0;
            s->b_double[0] = 1 + s->a_double[1];
            s->b_double[1] = 0;
            s->b_double[2] = 0;
        } else {
            s->a_double[0] =  1 + alpha;
            s->a_double[1] = -2 * cos(w0);
            s->a_double[2] =  1 - alpha;
            s->b_double[0] = (1 - cos(w0)) / 2;
            s->b_double[1] =  1 - cos(w0);
            s->b_double[2] = (1 - cos(w0)) / 2;
        }
        break;
    case highpass:
        if (s->poles == 1) {
            s->a_double[0] = 1;
            s->a_double[1] = -exp(-w0);
            s->a_double[2] = 0;
            s->b_double[0] = (1 - s->a_double[1]) / 2;
            s->b_double[1] = -s->b_double[0];
            s->b_double[2] = 0;
        } else {
            s->a_double[0] =   1 + alpha;
            s->a_double[1] =  -2 * cos(w0);
            s->a_double[2] =   1 - alpha;
            s->b_double[0] =  (1 + cos(w0)) / 2;
            s->b_double[1] = -(1 + cos(w0));
            s->b_double[2] =  (1 + cos(w0)) / 2;
        }
        break;
    case allpass:
        switch (s->order) {
        case 1:
            s->a_double[0] = 1.;
            s->a_double[1] = -(1. - K) / (1. + K);
            s->a_double[2] = 0.;
            s->b_double[0] = s->a_double[1];
            s->b_double[1] = s->a_double[0];
            s->b_double[2] = 0.;
            break;
        case 2:
            s->a_double[0] =  1 + alpha;
            s->a_double[1] = -2 * cos(w0);
            s->a_double[2] =  1 - alpha;
            s->b_double[0] =  1 - alpha;
            s->b_double[1] = -2 * cos(w0);
            s->b_double[2] =  1 + alpha;
        break;
        }
        break;
    default:
        av_assert0(0);
    }

    av_log(ctx, AV_LOG_VERBOSE, "a=%f %f %f:b=%f %f %f\n",
           s->a_double[0], s->a_double[1], s->a_double[2],
           s->b_double[0], s->b_double[1], s->b_double[2]);

    s->a_double[1] /= s->a_double[0];
    s->a_double[2] /= s->a_double[0];
    s->b_double[0] /= s->a_double[0];
    s->b_double[1] /= s->a_double[0];
    s->b_double[2] /= s->a_double[0];
    s->a_double[0] /= s->a_double[0];

    if (s->normalize && fabs(s->b_double[0] + s->b_double[1] + s->b_double[2]) > 1e-6) {
        double factor = (s->a_double[0] + s->a_double[1] + s->a_double[2]) /
                        (s->b_double[0] + s->b_double[1] + s->b_double[2]);

        s->b_double[0] *= factor;
        s->b_double[1] *= factor;
        s->b_double[2] *= factor;
    }

    switch (s->filter_type) {
    case tiltshelf:
        s->b_double[0] /= A;
        s->b_double[1] /= A;
        s->b_double[2] /= A;
        break;
    }

    if (!s->cache[0])
        s->cache[0] = ff_get_audio_buffer(outlink, 4 * sizeof(double));
    if (!s->clip)
        s->clip = av_calloc(outlink->ch_layout.nb_channels, sizeof(*s->clip));
    if (!s->cache[0] || !s->clip)
        return AVERROR(ENOMEM);
    if (reset) {
        av_samples_set_silence(s->cache[0]->extended_data, 0, s->cache[0]->nb_samples,
                               s->cache[0]->ch_layout.nb_channels, s->cache[0]->format);
    }

    if (reset && s->block_samples > 0) {
        if (!s->cache[1])
            s->cache[1] = ff_get_audio_buffer(outlink, 4 * sizeof(double));
        if (!s->cache[1])
            return AVERROR(ENOMEM);
        av_samples_set_silence(s->cache[1]->extended_data, 0, s->cache[1]->nb_samples,
                               s->cache[1]->ch_layout.nb_channels, s->cache[1]->format);
        for (int i = 0; i < 3; i++) {
            if (!s->block[i])
                s->block[i] = ff_get_audio_buffer(outlink, s->block_samples * 2);
            if (!s->block[i])
                return AVERROR(ENOMEM);
            av_samples_set_silence(s->block[i]->extended_data, 0, s->block_samples * 2,
                                   s->block[i]->ch_layout.nb_channels, s->block[i]->format);
        }
    }

    switch (s->transform_type) {
    case DI:
        switch (inlink->format) {
        case AV_SAMPLE_FMT_S16P:
            s->filter = biquad_s16;
            break;
        case AV_SAMPLE_FMT_S32P:
            s->filter = biquad_s32;
            break;
        case AV_SAMPLE_FMT_FLTP:
            s->filter = biquad_flt;
            break;
        case AV_SAMPLE_FMT_DBLP:
            s->filter = biquad_dbl;
            break;
        default: av_assert0(0);
        }
        break;
    case DII:
        switch (inlink->format) {
        case AV_SAMPLE_FMT_S16P:
            s->filter = biquad_dii_s16;
            break;
        case AV_SAMPLE_FMT_S32P:
            s->filter = biquad_dii_s32;
            break;
        case AV_SAMPLE_FMT_FLTP:
            s->filter = biquad_dii_flt;
            break;
        case AV_SAMPLE_FMT_DBLP:
            s->filter = biquad_dii_dbl;
            break;
        default: av_assert0(0);
        }
        break;
    case TDI:
        switch (inlink->format) {
        case AV_SAMPLE_FMT_S16P:
            s->filter = biquad_tdi_s16;
            break;
        case AV_SAMPLE_FMT_S32P:
            s->filter = biquad_tdi_s32;
            break;
        case AV_SAMPLE_FMT_FLTP:
            s->filter = biquad_tdi_flt;
            break;
        case AV_SAMPLE_FMT_DBLP:
            s->filter = biquad_tdi_dbl;
            break;
        default: av_assert0(0);
        }
        break;
    case TDII:
        switch (inlink->format) {
        case AV_SAMPLE_FMT_S16P:
            s->filter = biquad_tdii_s16;
            break;
        case AV_SAMPLE_FMT_S32P:
            s->filter = biquad_tdii_s32;
            break;
        case AV_SAMPLE_FMT_FLTP:
            s->filter = biquad_tdii_flt;
            break;
        case AV_SAMPLE_FMT_DBLP:
            s->filter = biquad_tdii_dbl;
            break;
        default: av_assert0(0);
        }
        break;
    case LATT:
        switch (inlink->format) {
        case AV_SAMPLE_FMT_S16P:
            s->filter = biquad_latt_s16;
            break;
        case AV_SAMPLE_FMT_S32P:
            s->filter = biquad_latt_s32;
            break;
        case AV_SAMPLE_FMT_FLTP:
            s->filter = biquad_latt_flt;
            break;
        case AV_SAMPLE_FMT_DBLP:
            s->filter = biquad_latt_dbl;
            break;
        default: av_assert0(0);
        }
        break;
    case SVF:
        switch (inlink->format) {
        case AV_SAMPLE_FMT_S16P:
            s->filter = biquad_svf_s16;
            break;
        case AV_SAMPLE_FMT_S32P:
            s->filter = biquad_svf_s32;
            break;
        case AV_SAMPLE_FMT_FLTP:
            s->filter = biquad_svf_flt;
            break;
        case AV_SAMPLE_FMT_DBLP:
            s->filter = biquad_svf_dbl;
            break;
        default: av_assert0(0);
        }
        break;
    case ZDF:
        switch (inlink->format) {
        case AV_SAMPLE_FMT_S16P:
            s->filter = biquad_zdf_s16;
            break;
        case AV_SAMPLE_FMT_S32P:
            s->filter = biquad_zdf_s32;
            break;
        case AV_SAMPLE_FMT_FLTP:
            s->filter = biquad_zdf_flt;
            break;
        case AV_SAMPLE_FMT_DBLP:
            s->filter = biquad_zdf_dbl;
            break;
        default: av_assert0(0);
        }
        break;
    default:
        av_assert0(0);
    }

    s->block_align = av_get_bytes_per_sample(inlink->format);

    if (s->transform_type == LATT)
        convert_dir2latt(s);
    else if (s->transform_type == SVF)
        convert_dir2svf(s);
    else if (s->transform_type == ZDF)
        convert_dir2zdf(s, inlink->sample_rate);

    s->a_float[0] = s->a_double[0];
    s->a_float[1] = s->a_double[1];
    s->a_float[2] = s->a_double[2];
    s->b_float[0] = s->b_double[0];
    s->b_float[1] = s->b_double[1];
    s->b_float[2] = s->b_double[2];

    return 0;
}

static int config_output(AVFilterLink *outlink)
{
    return config_filter(outlink, 1);
}

typedef struct ThreadData {
    AVFrame *in, *out;
    int eof;
} ThreadData;

static void reverse_samples(AVFrame *out, AVFrame *in, int p,
                            int oo, int io, int nb_samples)
{
    switch (out->format) {
    case AV_SAMPLE_FMT_S16P: {
        const int16_t *src = ((const int16_t *)in->extended_data[p]) + io;
        int16_t *dst = ((int16_t *)out->extended_data[p]) + oo;
        for (int i = 0, j = nb_samples - 1; i < nb_samples; i++, j--)
            dst[i] = src[j];
    }
        break;
    case AV_SAMPLE_FMT_S32P: {
        const int32_t *src = ((const int32_t *)in->extended_data[p]) + io;
        int32_t *dst = ((int32_t *)out->extended_data[p]) + oo;
        for (int i = 0, j = nb_samples - 1; i < nb_samples; i++, j--)
            dst[i] = src[j];
    }
        break;
    case AV_SAMPLE_FMT_FLTP: {
        const float *src = ((const float *)in->extended_data[p]) + io;
        float *dst = ((float *)out->extended_data[p]) + oo;
        for (int i = 0, j = nb_samples - 1; i < nb_samples; i++, j--)
            dst[i] = src[j];
    }
        break;
    case AV_SAMPLE_FMT_DBLP: {
        const double *src = ((const double *)in->extended_data[p]) + io;
        double *dst = ((double *)out->extended_data[p]) + oo;
        for (int i = 0, j = nb_samples - 1; i < nb_samples; i++, j--)
            dst[i] = src[j];
    }
        break;
    }
}

static int filter_channel(AVFilterContext *ctx, void *arg, int jobnr, int nb_jobs)
{
    AVFilterLink *inlink = ctx->inputs[0];
    ThreadData *td = arg;
    AVFrame *buf = td->in;
    AVFrame *out_buf = td->out;
    BiquadsContext *s = ctx->priv;
    const int start = (buf->ch_layout.nb_channels * jobnr) / nb_jobs;
    const int end = (buf->ch_layout.nb_channels * (jobnr+1)) / nb_jobs;
    int ch;

    for (ch = start; ch < end; ch++) {
        enum AVChannel channel = av_channel_layout_channel_from_index(&inlink->ch_layout, ch);

        if (av_channel_layout_index_from_channel(&s->ch_layout, channel) < 0) {
            if (buf != out_buf)
                memcpy(out_buf->extended_data[ch], buf->extended_data[ch],
                       buf->nb_samples * s->block_align);
            continue;
        }

        if (!s->block_samples) {
            s->filter(s, buf->extended_data[ch], out_buf->extended_data[ch], buf->nb_samples,
                      s->cache[0]->extended_data[ch], s->clip+ch, ctx->is_disabled);
        } else if (td->eof) {
            memcpy(out_buf->extended_data[ch], s->block[1]->extended_data[ch] + s->block_align * s->block_samples,
                   s->nb_samples * s->block_align);
        } else {
            memcpy(s->block[0]->extended_data[ch] + s->block_align * s->block_samples, buf->extended_data[ch],
                   buf->nb_samples * s->block_align);
            memset(s->block[0]->extended_data[ch] + s->block_align * (s->block_samples + buf->nb_samples),
                   0, (s->block_samples - buf->nb_samples) * s->block_align);
            s->filter(s, s->block[0]->extended_data[ch], s->block[1]->extended_data[ch], s->block_samples,
                      s->cache[0]->extended_data[ch], s->clip+ch, ctx->is_disabled);
            av_samples_copy(s->cache[1]->extended_data, s->cache[0]->extended_data, 0, 0,
                            s->cache[0]->nb_samples, s->cache[0]->ch_layout.nb_channels,
                            s->cache[0]->format);
            s->filter(s, s->block[0]->extended_data[ch] + s->block_samples * s->block_align,
                      s->block[1]->extended_data[ch] + s->block_samples * s->block_align,
                      s->block_samples, s->cache[1]->extended_data[ch], s->clip+ch,
                      ctx->is_disabled);
            reverse_samples(s->block[2], s->block[1], ch, 0, 0, 2 * s->block_samples);
            av_samples_set_silence(s->cache[1]->extended_data, 0, s->cache[1]->nb_samples,
                                   s->cache[1]->ch_layout.nb_channels, s->cache[1]->format);
            s->filter(s, s->block[2]->extended_data[ch], s->block[2]->extended_data[ch], 2 * s->block_samples,
                      s->cache[1]->extended_data[ch], s->clip+ch, ctx->is_disabled);
            reverse_samples(s->block[1], s->block[2], ch, 0, 0, 2 * s->block_samples);
            memcpy(out_buf->extended_data[ch], s->block[1]->extended_data[ch],
                   s->block_samples * s->block_align);
            memmove(s->block[0]->extended_data[ch], s->block[0]->extended_data[ch] + s->block_align * s->block_samples,
                    s->block_samples * s->block_align);
        }
    }

    return 0;
}

static int filter_frame(AVFilterLink *inlink, AVFrame *buf, int eof)
{
    AVFilterContext  *ctx = inlink->dst;
    BiquadsContext *s     = ctx->priv;
    AVFilterLink *outlink = ctx->outputs[0];
    AVFrame *out_buf;
    ThreadData td;
    int ch, ret, drop = 0;

    if (s->bypass)
        return ff_filter_frame(outlink, buf);

    ret = av_channel_layout_copy(&s->ch_layout, &inlink->ch_layout);
    if (ret < 0) {
        av_frame_free(&buf);
        return ret;
    }
    if (strcmp(s->ch_layout_str, "all"))
        av_channel_layout_from_string(&s->ch_layout,
                                      s->ch_layout_str);

    if (av_frame_is_writable(buf) && s->block_samples == 0) {
        out_buf = buf;
    } else {
        out_buf = ff_get_audio_buffer(outlink, s->block_samples > 0 ? s->block_samples : buf->nb_samples);
        if (!out_buf) {
            av_frame_free(&buf);
            return AVERROR(ENOMEM);
        }
        av_frame_copy_props(out_buf, buf);
    }

    if (s->block_samples > 0 && s->pts == AV_NOPTS_VALUE)
        drop = 1;
    td.in = buf;
    td.out = out_buf;
    td.eof = eof;
    ff_filter_execute(ctx, filter_channel, &td, NULL,
                      FFMIN(outlink->ch_layout.nb_channels, ff_filter_get_nb_threads(ctx)));

    for (ch = 0; ch < outlink->ch_layout.nb_channels; ch++) {
        if (s->clip[ch] > 0)
            av_log(ctx, AV_LOG_WARNING, "Channel %d clipping %d times. Please reduce gain.\n",
                   ch, s->clip[ch]);
        s->clip[ch] = 0;
    }

    if (s->block_samples > 0) {
        int nb_samples = buf->nb_samples;
        int64_t pts = buf->pts;

        out_buf->pts = s->pts;
        out_buf->nb_samples = s->nb_samples;
        s->pts = pts;
        s->nb_samples = nb_samples;
    }

    if (buf != out_buf)
        av_frame_free(&buf);

    if (!drop)
        return ff_filter_frame(outlink, out_buf);
    else {
        av_frame_free(&out_buf);
        ff_filter_set_ready(ctx, 10);
        return 0;
    }
}

static int activate(AVFilterContext *ctx)
{
    AVFilterLink *inlink = ctx->inputs[0];
    AVFilterLink *outlink = ctx->outputs[0];
    BiquadsContext *s = ctx->priv;
    AVFrame *in = NULL;
    int64_t pts;
    int status;
    int ret;

    FF_FILTER_FORWARD_STATUS_BACK(outlink, inlink);

    if (s->block_samples > 0) {
        ret = ff_inlink_consume_samples(inlink, s->block_samples, s->block_samples, &in);
    } else {
        ret = ff_inlink_consume_frame(inlink, &in);
    }
    if (ret < 0)
        return ret;
    if (ret > 0)
        return filter_frame(inlink, in, 0);

    if (s->block_samples > 0 && ff_inlink_queued_samples(inlink) >= s->block_samples) {
        ff_filter_set_ready(ctx, 10);
        return 0;
    }

    if (ff_inlink_acknowledge_status(inlink, &status, &pts)) {
        if (s->block_samples > 0) {
            AVFrame *in = ff_get_audio_buffer(outlink, s->block_samples);
            if (!in)
                return AVERROR(ENOMEM);

            ret = filter_frame(inlink, in, 1);
        }

        ff_outlink_set_status(outlink, status, pts);

        return ret;
    }

    FF_FILTER_FORWARD_WANTED(outlink, inlink);

    return FFERROR_NOT_READY;
}

static int process_command(AVFilterContext *ctx, const char *cmd, const char *args,
                           char *res, int res_len, int flags)
{
    AVFilterLink *outlink = ctx->outputs[0];
    int ret;

    ret = ff_filter_process_command(ctx, cmd, args, res, res_len, flags);
    if (ret < 0)
        return ret;

    return config_filter(outlink, 0);
}

static av_cold void uninit(AVFilterContext *ctx)
{
    BiquadsContext *s = ctx->priv;

    for (int i = 0; i < 3; i++)
        av_frame_free(&s->block[i]);
    av_frame_free(&s->cache[0]);
    av_frame_free(&s->cache[1]);
    av_freep(&s->clip);
    av_channel_layout_uninit(&s->ch_layout);
}

static const AVFilterPad outputs[] = {
    {
        .name         = "default",
        .type         = AVMEDIA_TYPE_AUDIO,
        .config_props = config_output,
    },
};

#define OFFSET(x) offsetof(BiquadsContext, x)
#define FLAGS AV_OPT_FLAG_AUDIO_PARAM|AV_OPT_FLAG_FILTERING_PARAM|AV_OPT_FLAG_RUNTIME_PARAM
#define AF AV_OPT_FLAG_AUDIO_PARAM|AV_OPT_FLAG_FILTERING_PARAM

#define DEFINE_BIQUAD_FILTER_2(name_, description_, priv_class_)        \
static av_cold int name_##_init(AVFilterContext *ctx)                   \
{                                                                       \
    BiquadsContext *s = ctx->priv;                                      \
    s->filter_type = name_;                                             \
    s->pts = AV_NOPTS_VALUE;                                            \
    return 0;                                                           \
}                                                                       \
                                                         \
const FFFilter ff_af_##name_ = {                         \
    .p.name        = #name_,                             \
    .p.description = NULL_IF_CONFIG_SMALL(description_), \
    .p.priv_class  = &priv_class_##_class,               \
    .p.flags       = AVFILTER_FLAG_SLICE_THREADS |       \
                     AVFILTER_FLAG_SUPPORT_TIMELINE_INTERNAL, \
    .priv_size     = sizeof(BiquadsContext),             \
    .init          = name_##_init,                       \
    .activate      = activate,                           \
    .uninit        = uninit,                             \
    FILTER_INPUTS(ff_audio_default_filterpad),           \
    FILTER_OUTPUTS(outputs),                             \
    FILTER_QUERY_FUNC2(query_formats),                   \
    .process_command = process_command,                  \
}

#define DEFINE_BIQUAD_FILTER(name, description)                         \
    AVFILTER_DEFINE_CLASS(name);                                        \
    DEFINE_BIQUAD_FILTER_2(name, description, name)

#define WIDTH_OPTION(x)                                                                       \
    {"width", "set width", OFFSET(width), AV_OPT_TYPE_DOUBLE, {.dbl=x}, 0, 99999, FLAGS}, \
    {"w",     "set width", OFFSET(width), AV_OPT_TYPE_DOUBLE, {.dbl=x}, 0, 99999, FLAGS}

#define WIDTH_TYPE_OPTION(x)                                                                                                        \
    {"width_type", "set filter-width type", OFFSET(width_type), AV_OPT_TYPE_INT, {.i64=x}, HERTZ, NB_WTYPE-1, FLAGS, .unit = "width_type"}, \
    {"t",          "set filter-width type", OFFSET(width_type), AV_OPT_TYPE_INT, {.i64=x}, HERTZ, NB_WTYPE-1, FLAGS, .unit = "width_type"}, \
    {"h", "Hz", 0, AV_OPT_TYPE_CONST, {.i64=HERTZ}, 0, 0, FLAGS, .unit = "width_type"},                                                     \
    {"q", "Q-Factor", 0, AV_OPT_TYPE_CONST, {.i64=QFACTOR}, 0, 0, FLAGS, .unit = "width_type"},                                             \
    {"o", "octave", 0, AV_OPT_TYPE_CONST, {.i64=OCTAVE}, 0, 0, FLAGS, .unit = "width_type"},                                                \
    {"s", "slope", 0, AV_OPT_TYPE_CONST, {.i64=SLOPE}, 0, 0, FLAGS, .unit = "width_type"},                                                  \
    {"k", "kHz", 0, AV_OPT_TYPE_CONST, {.i64=KHERTZ}, 0, 0, FLAGS, .unit = "width_type"}

#define MIX_CHANNELS_NORMALIZE_OPTION(x, y, z)                                                                \
    {"mix", "set mix", OFFSET(mix), AV_OPT_TYPE_DOUBLE, {.dbl=x}, 0, 1, FLAGS},                               \
    {"m",   "set mix", OFFSET(mix), AV_OPT_TYPE_DOUBLE, {.dbl=x}, 0, 1, FLAGS},                               \
    {"channels", "set channels to filter", OFFSET(ch_layout_str), AV_OPT_TYPE_STRING, {.str=y}, 0, 0, FLAGS}, \
    {"c",        "set channels to filter", OFFSET(ch_layout_str), AV_OPT_TYPE_STRING, {.str=y}, 0, 0, FLAGS}, \
    {"normalize", "normalize coefficients", OFFSET(normalize), AV_OPT_TYPE_BOOL, {.i64=z}, 0, 1, FLAGS},      \
    {"n",         "normalize coefficients", OFFSET(normalize), AV_OPT_TYPE_BOOL, {.i64=z}, 0, 1, FLAGS}

#define TRANSFORM_OPTION(x)                                                                                                      \
    {"transform", "set transform type", OFFSET(transform_type), AV_OPT_TYPE_INT, {.i64=x}, 0, NB_TTYPE-1, AF, .unit = "transform_type"}, \
    {"a",         "set transform type", OFFSET(transform_type), AV_OPT_TYPE_INT, {.i64=x}, 0, NB_TTYPE-1, AF, .unit = "transform_type"}, \
    {"di",   "direct form I",  0, AV_OPT_TYPE_CONST, {.i64=DI}, 0, 0, AF, .unit = "transform_type"},                                     \
    {"dii",  "direct form II", 0, AV_OPT_TYPE_CONST, {.i64=DII}, 0, 0, AF, .unit = "transform_type"},                                    \
    {"tdi",  "transposed direct form I",  0, AV_OPT_TYPE_CONST, {.i64=TDI},  0, 0, AF, .unit = "transform_type"},                        \
    {"tdii", "transposed direct form II", 0, AV_OPT_TYPE_CONST, {.i64=TDII}, 0, 0, AF, .unit = "transform_type"},                        \
    {"latt", "lattice-ladder form", 0, AV_OPT_TYPE_CONST, {.i64=LATT}, 0, 0, AF, .unit = "transform_type"},                              \
    {"svf",  "state variable filter form", 0, AV_OPT_TYPE_CONST, {.i64=SVF}, 0, 0, AF, .unit = "transform_type"},                        \
    {"zdf",  "zero-delay filter form", 0, AV_OPT_TYPE_CONST, {.i64=ZDF}, 0, 0, AF, .unit = "transform_type"}

#define PRECISION_OPTION(x)                                                                                           \
    {"precision", "set filtering precision", OFFSET(precision), AV_OPT_TYPE_INT, {.i64=x}, -1, 3, AF, .unit = "precision"},   \
    {"r",         "set filtering precision", OFFSET(precision), AV_OPT_TYPE_INT, {.i64=x}, -1, 3, AF, .unit = "precision"},   \
    {"auto", "automatic",            0, AV_OPT_TYPE_CONST, {.i64=-1}, 0, 0, AF, .unit = "precision"},                         \
    {"s16", "signed 16-bit",         0, AV_OPT_TYPE_CONST, {.i64=0},  0, 0, AF, .unit = "precision"},                         \
    {"s32", "signed 32-bit",         0, AV_OPT_TYPE_CONST, {.i64=1},  0, 0, AF, .unit = "precision"},                         \
    {"f32", "floating-point single", 0, AV_OPT_TYPE_CONST, {.i64=2},  0, 0, AF, .unit = "precision"},                         \
    {"f64", "floating-point double", 0, AV_OPT_TYPE_CONST, {.i64=3},  0, 0, AF, .unit = "precision"}

#define BLOCKSIZE_OPTION(x)                                                                              \
    {"blocksize", "set the block size", OFFSET(block_samples), AV_OPT_TYPE_INT, {.i64=x}, 0, 32768, AF}, \
    {"b",         "set the block size", OFFSET(block_samples), AV_OPT_TYPE_INT, {.i64=x}, 0, 32768, AF}

#if CONFIG_EQUALIZER_FILTER
static const AVOption equalizer_options[] = {
    {"frequency", "set central frequency", OFFSET(frequency), AV_OPT_TYPE_DOUBLE, {.dbl=0}, 0, 999999, FLAGS},
    {"f",         "set central frequency", OFFSET(frequency), AV_OPT_TYPE_DOUBLE, {.dbl=0}, 0, 999999, FLAGS},
    WIDTH_TYPE_OPTION(QFACTOR),
    WIDTH_OPTION(1.0),
    {"gain", "set gain", OFFSET(gain), AV_OPT_TYPE_DOUBLE, {.dbl=0}, -900, 900, FLAGS},
    {"g",    "set gain", OFFSET(gain), AV_OPT_TYPE_DOUBLE, {.dbl=0}, -900, 900, FLAGS},
    MIX_CHANNELS_NORMALIZE_OPTION(1, "all", 0),
    TRANSFORM_OPTION(DI),
    PRECISION_OPTION(-1),
    BLOCKSIZE_OPTION(0),
    {NULL}
};

DEFINE_BIQUAD_FILTER(equalizer, "Apply two-pole peaking equalization (EQ) filter.");
#endif  /* CONFIG_EQUALIZER_FILTER */
#if CONFIG_BASS_FILTER || CONFIG_LOWSHELF_FILTER
static const AVOption bass_lowshelf_options[] = {
    {"frequency", "set central frequency", OFFSET(frequency), AV_OPT_TYPE_DOUBLE, {.dbl=100}, 0, 999999, FLAGS},
    {"f",         "set central frequency", OFFSET(frequency), AV_OPT_TYPE_DOUBLE, {.dbl=100}, 0, 999999, FLAGS},
    WIDTH_TYPE_OPTION(QFACTOR),
    WIDTH_OPTION(0.5),
    {"gain", "set gain", OFFSET(gain), AV_OPT_TYPE_DOUBLE, {.dbl=0}, -900, 900, FLAGS},
    {"g",    "set gain", OFFSET(gain), AV_OPT_TYPE_DOUBLE, {.dbl=0}, -900, 900, FLAGS},
    {"poles", "set number of poles", OFFSET(poles), AV_OPT_TYPE_INT, {.i64=2}, 1, 2, AF},
    {"p",     "set number of poles", OFFSET(poles), AV_OPT_TYPE_INT, {.i64=2}, 1, 2, AF},
    MIX_CHANNELS_NORMALIZE_OPTION(1, "all", 0),
    TRANSFORM_OPTION(DI),
    PRECISION_OPTION(-1),
    BLOCKSIZE_OPTION(0),
    {NULL}
};

AVFILTER_DEFINE_CLASS_EXT(bass_lowshelf, "bass/lowshelf", bass_lowshelf_options);
#if CONFIG_BASS_FILTER
DEFINE_BIQUAD_FILTER_2(bass, "Boost or cut lower frequencies.", bass_lowshelf);
#endif  /* CONFIG_BASS_FILTER */

#if CONFIG_LOWSHELF_FILTER
DEFINE_BIQUAD_FILTER_2(lowshelf, "Apply a low shelf filter.", bass_lowshelf);
#endif  /* CONFIG_LOWSHELF_FILTER */
#endif  /* CONFIG_BASS_FILTER || CONFIG LOWSHELF_FILTER */
#if CONFIG_TREBLE_FILTER || CONFIG_HIGHSHELF_FILTER || CONFIG_TILTSHELF_FILTER
static const AVOption treble_highshelf_options[] = {
    {"frequency", "set central frequency", OFFSET(frequency), AV_OPT_TYPE_DOUBLE, {.dbl=3000}, 0, 999999, FLAGS},
    {"f",         "set central frequency", OFFSET(frequency), AV_OPT_TYPE_DOUBLE, {.dbl=3000}, 0, 999999, FLAGS},
    WIDTH_TYPE_OPTION(QFACTOR),
    WIDTH_OPTION(0.5),
    {"gain", "set gain", OFFSET(gain), AV_OPT_TYPE_DOUBLE, {.dbl=0}, -900, 900, FLAGS},
    {"g",    "set gain", OFFSET(gain), AV_OPT_TYPE_DOUBLE, {.dbl=0}, -900, 900, FLAGS},
    {"poles", "set number of poles", OFFSET(poles), AV_OPT_TYPE_INT, {.i64=2}, 1, 2, AF},
    {"p",     "set number of poles", OFFSET(poles), AV_OPT_TYPE_INT, {.i64=2}, 1, 2, AF},
    MIX_CHANNELS_NORMALIZE_OPTION(1, "all", 0),
    TRANSFORM_OPTION(DI),
    PRECISION_OPTION(-1),
    BLOCKSIZE_OPTION(0),
    {NULL}
};

AVFILTER_DEFINE_CLASS_EXT(treble_highshelf, "treble/high/tiltshelf",
                          treble_highshelf_options);

#if CONFIG_TREBLE_FILTER
DEFINE_BIQUAD_FILTER_2(treble, "Boost or cut upper frequencies.", treble_highshelf);
#endif  /* CONFIG_TREBLE_FILTER */

#if CONFIG_HIGHSHELF_FILTER
DEFINE_BIQUAD_FILTER_2(highshelf, "Apply a high shelf filter.", treble_highshelf);
#endif  /* CONFIG_HIGHSHELF_FILTER */

#if CONFIG_TILTSHELF_FILTER
DEFINE_BIQUAD_FILTER_2(tiltshelf, "Apply a tilt shelf filter.", treble_highshelf);
#endif
#endif  /* CONFIG_TREBLE_FILTER || CONFIG_HIGHSHELF_FILTER || CONFIG_TILTSHELF_FILTER */

#if CONFIG_BANDPASS_FILTER
static const AVOption bandpass_options[] = {
    {"frequency", "set central frequency", OFFSET(frequency), AV_OPT_TYPE_DOUBLE, {.dbl=3000}, 0, 999999, FLAGS},
    {"f",         "set central frequency", OFFSET(frequency), AV_OPT_TYPE_DOUBLE, {.dbl=3000}, 0, 999999, FLAGS},
    WIDTH_TYPE_OPTION(QFACTOR),
    WIDTH_OPTION(0.5),
    {"csg",   "use constant skirt gain", OFFSET(csg), AV_OPT_TYPE_BOOL, {.i64=0}, 0, 1, FLAGS},
    MIX_CHANNELS_NORMALIZE_OPTION(1, "all", 0),
    TRANSFORM_OPTION(DI),
    PRECISION_OPTION(-1),
    BLOCKSIZE_OPTION(0),
    {NULL}
};

DEFINE_BIQUAD_FILTER(bandpass, "Apply a two-pole Butterworth band-pass filter.");
#endif  /* CONFIG_BANDPASS_FILTER */
#if CONFIG_BANDREJECT_FILTER
static const AVOption bandreject_options[] = {
    {"frequency", "set central frequency", OFFSET(frequency), AV_OPT_TYPE_DOUBLE, {.dbl=3000}, 0, 999999, FLAGS},
    {"f",         "set central frequency", OFFSET(frequency), AV_OPT_TYPE_DOUBLE, {.dbl=3000}, 0, 999999, FLAGS},
    WIDTH_TYPE_OPTION(QFACTOR),
    WIDTH_OPTION(0.5),
    MIX_CHANNELS_NORMALIZE_OPTION(1, "all", 0),
    TRANSFORM_OPTION(DI),
    PRECISION_OPTION(-1),
    BLOCKSIZE_OPTION(0),
    {NULL}
};

DEFINE_BIQUAD_FILTER(bandreject, "Apply a two-pole Butterworth band-reject filter.");
#endif  /* CONFIG_BANDREJECT_FILTER */
#if CONFIG_LOWPASS_FILTER
static const AVOption lowpass_options[] = {
    {"frequency", "set frequency", OFFSET(frequency), AV_OPT_TYPE_DOUBLE, {.dbl=500}, 0, 999999, FLAGS},
    {"f",         "set frequency", OFFSET(frequency), AV_OPT_TYPE_DOUBLE, {.dbl=500}, 0, 999999, FLAGS},
    WIDTH_TYPE_OPTION(QFACTOR),
    WIDTH_OPTION(0.707),
    {"poles", "set number of poles", OFFSET(poles), AV_OPT_TYPE_INT, {.i64=2}, 1, 2, AF},
    {"p",     "set number of poles", OFFSET(poles), AV_OPT_TYPE_INT, {.i64=2}, 1, 2, AF},
    MIX_CHANNELS_NORMALIZE_OPTION(1, "all", 0),
    TRANSFORM_OPTION(DI),
    PRECISION_OPTION(-1),
    BLOCKSIZE_OPTION(0),
    {NULL}
};

DEFINE_BIQUAD_FILTER(lowpass, "Apply a low-pass filter with 3dB point frequency.");
#endif  /* CONFIG_LOWPASS_FILTER */
#if CONFIG_HIGHPASS_FILTER
static const AVOption highpass_options[] = {
    {"frequency", "set frequency", OFFSET(frequency), AV_OPT_TYPE_DOUBLE, {.dbl=3000}, 0, 999999, FLAGS},
    {"f",         "set frequency", OFFSET(frequency), AV_OPT_TYPE_DOUBLE, {.dbl=3000}, 0, 999999, FLAGS},
    WIDTH_TYPE_OPTION(QFACTOR),
    WIDTH_OPTION(0.707),
    {"poles", "set number of poles", OFFSET(poles), AV_OPT_TYPE_INT, {.i64=2}, 1, 2, AF},
    {"p",     "set number of poles", OFFSET(poles), AV_OPT_TYPE_INT, {.i64=2}, 1, 2, AF},
    MIX_CHANNELS_NORMALIZE_OPTION(1, "all", 0),
    TRANSFORM_OPTION(DI),
    PRECISION_OPTION(-1),
    BLOCKSIZE_OPTION(0),
    {NULL}
};

DEFINE_BIQUAD_FILTER(highpass, "Apply a high-pass filter with 3dB point frequency.");
#endif  /* CONFIG_HIGHPASS_FILTER */
#if CONFIG_ALLPASS_FILTER
static const AVOption allpass_options[] = {
    {"frequency", "set central frequency", OFFSET(frequency), AV_OPT_TYPE_DOUBLE, {.dbl=3000}, 0, 999999, FLAGS},
    {"f",         "set central frequency", OFFSET(frequency), AV_OPT_TYPE_DOUBLE, {.dbl=3000}, 0, 999999, FLAGS},
    WIDTH_TYPE_OPTION(QFACTOR),
    WIDTH_OPTION(0.707),
    MIX_CHANNELS_NORMALIZE_OPTION(1, "all", 0),
    {"order", "set filter order", OFFSET(order), AV_OPT_TYPE_INT, {.i64=2}, 1, 2, FLAGS},
    {"o",     "set filter order", OFFSET(order), AV_OPT_TYPE_INT, {.i64=2}, 1, 2, FLAGS},
    TRANSFORM_OPTION(DI),
    PRECISION_OPTION(-1),
    {NULL}
};

DEFINE_BIQUAD_FILTER(allpass, "Apply a two-pole all-pass filter.");
#endif  /* CONFIG_ALLPASS_FILTER */
#if CONFIG_BIQUAD_FILTER
static const AVOption biquad_options[] = {
    {"a0", NULL, OFFSET(oa[0]), AV_OPT_TYPE_DOUBLE, {.dbl=1}, INT32_MIN, INT32_MAX, FLAGS},
    {"a1", NULL, OFFSET(oa[1]), AV_OPT_TYPE_DOUBLE, {.dbl=0}, INT32_MIN, INT32_MAX, FLAGS},
    {"a2", NULL, OFFSET(oa[2]), AV_OPT_TYPE_DOUBLE, {.dbl=0}, INT32_MIN, INT32_MAX, FLAGS},
    {"b0", NULL, OFFSET(ob[0]), AV_OPT_TYPE_DOUBLE, {.dbl=0}, INT32_MIN, INT32_MAX, FLAGS},
    {"b1", NULL, OFFSET(ob[1]), AV_OPT_TYPE_DOUBLE, {.dbl=0}, INT32_MIN, INT32_MAX, FLAGS},
    {"b2", NULL, OFFSET(ob[2]), AV_OPT_TYPE_DOUBLE, {.dbl=0}, INT32_MIN, INT32_MAX, FLAGS},
    MIX_CHANNELS_NORMALIZE_OPTION(1, "all", 0),
    TRANSFORM_OPTION(DI),
    PRECISION_OPTION(-1),
    BLOCKSIZE_OPTION(0),
    {NULL}
};

DEFINE_BIQUAD_FILTER(biquad, "Apply a biquad IIR filter with the given coefficients.");
#endif  /* CONFIG_BIQUAD_FILTER */

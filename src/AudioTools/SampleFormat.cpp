#include "stdafx.h"
#include "SampleFormat.h"

static const struct SampleFmtProp {
    int bits;
    bool planar;
}
sample_fmt_info[SAMPLE_FMT_NB] = {
//   bits planar
    {  8, false }, // SAMPLE_FMT_U8
    { 16, false }, // SAMPLE_FMT_S16
    { 32, false }, // SAMPLE_FMT_S32
    { 32, false }, // SAMPLE_FMT_FLT
    { 64, false }, // SAMPLE_FMT_DBL
    {  8,  true }, // SAMPLE_FMT_U8P
    { 16,  true }, // SAMPLE_FMT_S16P
    { 32,  true }, // SAMPLE_FMT_S32P
    { 32,  true }, // SAMPLE_FMT_FLTP
    { 64,  true }, // SAMPLE_FMT_DBLP
    { 24, false }, // SAMPLE_FMT_S24
//  { 24,  true }  // SAMPLE_FMT_S24P
};

int get_bits_per_sample(const SampleFormat sample_fmt)
{
    return sample_fmt < 0 || sample_fmt >= SAMPLE_FMT_NB ? 0 : sample_fmt_info[sample_fmt].bits;
}

int get_bytes_per_sample(const SampleFormat sample_fmt)
{
    return sample_fmt < 0 || sample_fmt >= SAMPLE_FMT_NB ? 0 : sample_fmt_info[sample_fmt].bits >> 3;
}

bool sample_fmt_is_planar(const SampleFormat sample_fmt)
{
    return sample_fmt < 0 || sample_fmt >= SAMPLE_FMT_NB ? false : sample_fmt_info[sample_fmt].planar;
}

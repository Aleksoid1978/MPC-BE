#pragma once

enum SampleFormat {
    SAMPLE_FMT_NONE = -1,
    SAMPLE_FMT_U8,          ///< unsigned 8 bits
    SAMPLE_FMT_S16,         ///< signed 16 bits
    SAMPLE_FMT_S32,         ///< signed 32 bits
    SAMPLE_FMT_FLT,         ///< float
    SAMPLE_FMT_DBL,         ///< double

    SAMPLE_FMT_U8P,         ///< unsigned 8 bits, planar
    SAMPLE_FMT_S16P,        ///< signed 16 bits, planar
    SAMPLE_FMT_S32P,        ///< signed 32 bits, planar
    SAMPLE_FMT_FLTP,        ///< float, planar
    SAMPLE_FMT_DBLP,        ///< double, planar
    SAMPLE_FMT_S64,         ///< signed 64 bits
    SAMPLE_FMT_S64P,        ///< signed 64 bits, planar

    SAMPLE_FMT_S24,         ///< signed 24 bits
  //SAMPLE_FMT_S24P,        ///< signed 24 bits, planar

    SAMPLE_FMT_NB           ///< Number of sample formats.
};

int get_bits_per_sample(const SampleFormat sample_fmt);
int get_bytes_per_sample(const SampleFormat sample_fmt);
bool sample_fmt_is_planar(const SampleFormat sample_fmt);

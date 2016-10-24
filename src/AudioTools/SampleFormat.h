/*
 * (C) 2014-2016 see Authors.txt
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

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

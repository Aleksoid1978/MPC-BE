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
    { 64, false }, // SAMPLE_FMT_S64
    { 64,  true }, // SAMPLE_FMT_S64P
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

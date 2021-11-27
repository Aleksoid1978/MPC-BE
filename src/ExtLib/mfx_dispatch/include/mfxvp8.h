// Copyright (c) 2017-2019 Intel Corporation
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#ifndef __MFXVP8_H__
#define __MFXVP8_H__

#include "mfxdefs.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    MFX_CODEC_VP8 = MFX_MAKEFOURCC('V','P','8',' '),
};

/* CodecProfile*/
enum {
    MFX_PROFILE_VP8_0                       = 0+1, 
    MFX_PROFILE_VP8_1                       = 1+1,
    MFX_PROFILE_VP8_2                       = 2+1,
    MFX_PROFILE_VP8_3                       = 3+1,
};

/* Extended Buffer Ids */
enum {
    MFX_EXTBUFF_VP8_CODING_OPTION =   MFX_MAKEFOURCC('V','P','8','E'),
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct { 
    mfxExtBuffer    Header;

    mfxU16   Version;
    mfxU16   EnableMultipleSegments;
    mfxU16   LoopFilterType;
    mfxU16   LoopFilterLevel[4];
    mfxU16   SharpnessLevel;
    mfxU16   NumTokenPartitions;
    mfxI16   LoopFilterRefTypeDelta[4];
    mfxI16   LoopFilterMbModeDelta[4];
    mfxI16   SegmentQPDelta[4];
    mfxI16   CoeffTypeQPDelta[5];
    mfxU16   WriteIVFHeaders;
    mfxU32   NumFramesForIVFHeader;
    mfxU16   reserved[223];
} mfxExtVP8CodingOption;
MFX_PACK_END()

#ifdef __cplusplus
} // extern "C"
#endif

#endif


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
#ifndef __MFXLA_H__
#define __MFXLA_H__
#include "mfxdefs.h"
#include "mfxvstructures.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


enum
{
    MFX_DEPRECATED_ENUM_FIELD_INSIDE(MFX_EXTBUFF_LOOKAHEAD_CTRL)  =   MFX_MAKEFOURCC('L','A','C','T'),
    MFX_DEPRECATED_ENUM_FIELD_INSIDE(MFX_EXTBUFF_LOOKAHEAD_STAT)  =   MFX_MAKEFOURCC('L','A','S','T'),
};

MFX_DEPRECATED_ENUM_FIELD_OUTSIDE(MFX_EXTBUFF_LOOKAHEAD_CTRL);
MFX_DEPRECATED_ENUM_FIELD_OUTSIDE(MFX_EXTBUFF_LOOKAHEAD_STAT);

MFX_PACK_BEGIN_USUAL_STRUCT()
MFX_DEPRECATED typedef struct
{
    mfxExtBuffer    Header;
    mfxU16  LookAheadDepth;
    mfxU16  DependencyDepth;
    mfxU16  DownScaleFactor;
    mfxU16  BPyramid;

    mfxU16  reserved1[23];

    mfxU16  NumOutStream;
    struct  mfxStream{
        mfxU16  Width;
        mfxU16  Height;
        mfxU16  reserved2[14];
    } OutStream[16];
}mfxExtLAControl;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_L_TYPE()
MFX_DEPRECATED typedef struct
{
    mfxU16  Width;
    mfxU16  Height;

    mfxU32  FrameType;
    mfxU32  FrameDisplayOrder;
    mfxU32  FrameEncodeOrder;

    mfxU32  IntraCost;
    mfxU32  InterCost;
    mfxU32  DependencyCost; //aggregated cost, how this frame influences subsequent frames
    mfxU16  Layer;
    mfxU16  reserved[23];

    mfxU64 EstimatedRate[52];
}mfxLAFrameInfo;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_PTR()
MFX_DEPRECATED typedef struct  {
    mfxExtBuffer    Header;

    mfxU16  reserved[20];

    mfxU16  NumAlloc; //number of allocated mfxLAFrameInfo structures
    mfxU16  NumStream; //number of resolutions
    mfxU16  NumFrame; //number of frames for each resolution
    mfxLAFrameInfo   *FrameStat; //frame statistics

    mfxFrameSurface1 *OutSurface; //reordered surface

} mfxExtLAFrameStatistics;
MFX_PACK_END()

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */


#endif


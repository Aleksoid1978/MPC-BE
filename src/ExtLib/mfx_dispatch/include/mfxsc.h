// Copyright (c) 2018-2019 Intel Corporation
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
#ifndef __MFXSC_H__
#define __MFXSC_H__
#include "mfxdefs.h"
#include "mfxvstructures.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* Extended Buffer Ids */
enum 
{
    MFX_EXTBUFF_SCREEN_CAPTURE_PARAM = MFX_MAKEFOURCC('S','C','P','A')
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct
{
    mfxExtBuffer    Header;

    mfxU32          DisplayIndex;
    mfxU16          EnableDirtyRect;
    mfxU16          EnableCursorCapture;
    mfxU16          reserved[24];
} mfxExtScreenCaptureParam;
MFX_PACK_END()

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */


#endif


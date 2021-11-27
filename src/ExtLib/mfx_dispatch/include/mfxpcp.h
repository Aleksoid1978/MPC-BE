// Copyright (c) 2019 Intel Corporation
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
#ifndef __MFXPCP_H__
#define __MFXPCP_H__
#include "mfxstructures.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#if MFX_VERSION >= 1030
/* Protected in mfxVideoParam */
enum {
    MFX_PROTECTION_CENC_WV_CLASSIC      = 0x0004,
    MFX_PROTECTION_CENC_WV_GOOGLE_DASH  = 0x0005,
};

/* Extended Buffer Ids */
enum {
    MFX_EXTBUFF_CENC_PARAM          = MFX_MAKEFOURCC('C','E','N','P')
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct _mfxExtCencParam{
    mfxExtBuffer Header;

    mfxU32 StatusReportIndex;
    mfxU32 reserved[15];
} mfxExtCencParam;
MFX_PACK_END()
#endif

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */

#endif


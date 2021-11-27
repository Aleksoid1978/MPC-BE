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
#ifndef __MFXSCD_H__
#define __MFXSCD_H__

#include "mfxenc.h"
#include "mfxplugin.h"

#define MFX_ENC_SCD_PLUGIN_VERSION 1

#ifdef __cplusplus
extern "C" {
#endif

static const mfxPluginUID MFX_PLUGINID_ENC_SCD = {{ 0xdf, 0xc2, 0x15, 0xb3, 0xe3, 0xd3, 0x90, 0x4d, 0x7f, 0xa5, 0x04, 0x12, 0x7e, 0xf5, 0x64, 0xd5 }};

/* SCD Extended Buffer Ids */
enum {
    MFX_EXTBUFF_SCD = MFX_MAKEFOURCC('S','C','D',' ')
};

/* SceneType */
enum {
    MFX_SCD_SCENE_SAME        = 0x00,
    MFX_SCD_SCENE_NEW_FIELD_1 = 0x01,
    MFX_SCD_SCENE_NEW_FIELD_2 = 0x02,
    MFX_SCD_SCENE_NEW_PICTURE = MFX_SCD_SCENE_NEW_FIELD_1 | MFX_SCD_SCENE_NEW_FIELD_2
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxU16 SceneType;
    mfxU16 reserved[27];
} mfxExtSCD;
MFX_PACK_END()

#ifdef __cplusplus
} // extern "C"
#endif

#endif


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
#ifndef __MFXCAMERA_H__
#define __MFXCAMERA_H__
#include "mfxcommon.h"

#if !defined (__GNUC__)
#pragma warning(disable: 4201)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Camera Extended Buffer Ids */
enum {
    MFX_EXTBUF_CAM_GAMMA_CORRECTION             = MFX_MAKEFOURCC('C','G','A','M'),
    MFX_EXTBUF_CAM_WHITE_BALANCE                = MFX_MAKEFOURCC('C','W','B','L'),
    MFX_EXTBUF_CAM_HOT_PIXEL_REMOVAL            = MFX_MAKEFOURCC('C','H','P','R'),
    MFX_EXTBUF_CAM_BLACK_LEVEL_CORRECTION       = MFX_MAKEFOURCC('C','B','L','C'),
    MFX_EXTBUF_CAM_VIGNETTE_CORRECTION          = MFX_MAKEFOURCC('C','V','G','T'),
    MFX_EXTBUF_CAM_BAYER_DENOISE                = MFX_MAKEFOURCC('C','D','N','S'),
    MFX_EXTBUF_CAM_COLOR_CORRECTION_3X3         = MFX_MAKEFOURCC('C','C','3','3'),
    MFX_EXTBUF_CAM_PADDING                      = MFX_MAKEFOURCC('C','P','A','D'),
    MFX_EXTBUF_CAM_PIPECONTROL                  = MFX_MAKEFOURCC('C','P','P','C'),
    MFX_EXTBUF_CAM_FORWARD_GAMMA_CORRECTION     = MFX_MAKEFOURCC('C','F','G','C'),
    MFX_EXTBUF_CAM_LENS_GEOM_DIST_CORRECTION    = MFX_MAKEFOURCC('C','L','G','D'),
    MFX_EXTBUF_CAM_3DLUT                        = MFX_MAKEFOURCC('C','L','U','T'),
    MFX_EXTBUF_CAM_TOTAL_COLOR_CONTROL          = MFX_MAKEFOURCC('C','T','C','C'),
    MFX_EXTBUF_CAM_CSC_YUV_RGB                  = MFX_MAKEFOURCC('C','C','Y','R')
};

typedef enum {
    MFX_CAM_GAMMA_VALUE      = 0x0001,
    MFX_CAM_GAMMA_LUT        = 0x0002,
} mfxCamGammaParam;

MFX_PACK_BEGIN_STRUCT_W_L_TYPE()
typedef struct {
    mfxExtBuffer    Header;
    mfxU16  Mode;
    mfxU16  reserved1;
    mfxF64  GammaValue;

    mfxU16  reserved2[3];
    mfxU16  NumPoints;
    mfxU16  GammaPoint[1024];
    mfxU16  GammaCorrected[1024];
    mfxU32  reserved3[4];
} mfxExtCamGammaCorrection;
MFX_PACK_END()

typedef enum {
    MFX_CAM_WHITE_BALANCE_MANUAL   = 0x0001,
    MFX_CAM_WHITE_BALANCE_AUTO     = 0x0002
} mfxCamWhiteBalanceMode;

MFX_PACK_BEGIN_STRUCT_W_L_TYPE()
typedef struct {
    mfxExtBuffer    Header;
    mfxU32          Mode;
    mfxF64          R;
    mfxF64          G0;
    mfxF64          B;
    mfxF64          G1;
    mfxU32          reserved[8];
} mfxExtCamWhiteBalance;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;
    mfxU16        R;
    mfxU16        G;
    mfxU16        B;
    mfxU16        C;
    mfxU16        M;
    mfxU16        Y;
    mfxU16        reserved[6];
} mfxExtCamTotalColorControl;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxF32       PreOffset[3];
    mfxF32       Matrix[3][3];
    mfxF32       PostOffset[3];
    mfxU16       reserved[30];
} mfxExtCamCscYuvRgb;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer    Header;
    mfxU16          PixelThresholdDifference;
    mfxU16          PixelCountThreshold;
} mfxExtCamHotPixelRemoval;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer    Header;
    mfxU16          R;
    mfxU16          G0;
    mfxU16          B;
    mfxU16          G1;
    mfxU32          reserved[4];
} mfxExtCamBlackLevelCorrection;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxU8 integer;
    mfxU8 mantissa;
} mfxCamVignetteCorrectionElement;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxCamVignetteCorrectionElement R;
    mfxCamVignetteCorrectionElement G0;
    mfxCamVignetteCorrectionElement B;
    mfxCamVignetteCorrectionElement G1;
} mfxCamVignetteCorrectionParam;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_PTR()
typedef struct {
    mfxExtBuffer    Header;

    mfxU32          Width;
    mfxU32          Height;
    mfxU32          Pitch;
    mfxU32          reserved[7];

    mfxCamVignetteCorrectionParam *CorrectionMap;

} mfxExtCamVignetteCorrection;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer    Header;
    mfxU16          Threshold;
    mfxU16          reserved[27];
} mfxExtCamBayerDenoise;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_L_TYPE()
typedef struct {
    mfxExtBuffer    Header;
    mfxF64          CCM[3][3];
    mfxU32          reserved[4];
} mfxExtCamColorCorrection3x3;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer    Header;
    mfxU16 Top;
    mfxU16 Bottom;
    mfxU16 Left;
    mfxU16 Right;
    mfxU32 reserved[4];
} mfxExtCamPadding;
MFX_PACK_END()

typedef enum {
    MFX_CAM_BAYER_BGGR   = 0x0000,
    MFX_CAM_BAYER_RGGB   = 0x0001,
    MFX_CAM_BAYER_GBRG   = 0x0002,
    MFX_CAM_BAYER_GRBG   = 0x0003
} mfxCamBayerFormat;

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer    Header;
    mfxU16          RawFormat;
    mfxU16          reserved1;
    mfxU32          reserved[5];
} mfxExtCamPipeControl;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct{
    mfxU16 Pixel;
    mfxU16 Red;
    mfxU16 Green;
    mfxU16 Blue;
} mfxCamFwdGammaSegment;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_L_TYPE()
typedef struct {
    mfxExtBuffer Header;

    mfxU16       reserved[19];
    mfxU16       NumSegments;
    union {
        mfxCamFwdGammaSegment* Segment;
        mfxU64 reserved1;
    };
} mfxExtCamFwdGamma;
MFX_PACK_END()

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxExtBuffer Header;

    mfxF32       a[3]; // [R, G, B]
    mfxF32       b[3]; // [R, G, B]
    mfxF32       c[3]; // [R, G, B]
    mfxF32       d[3]; // [R, G, B]
    mfxU16       reserved[36];
} mfxExtCamLensGeomDistCorrection;
MFX_PACK_END()

/* LUTSize */
enum {
    MFX_CAM_3DLUT17_SIZE = (17 * 17 * 17),
    MFX_CAM_3DLUT33_SIZE = (33 * 33 * 33),
    MFX_CAM_3DLUT65_SIZE = (65 * 65 * 65)
};

MFX_PACK_BEGIN_USUAL_STRUCT()
typedef struct {
    mfxU16 R;
    mfxU16 G;
    mfxU16 B;
    mfxU16 Reserved;
} mfxCam3DLutEntry;
MFX_PACK_END()

MFX_PACK_BEGIN_STRUCT_W_L_TYPE()
typedef struct {
    mfxExtBuffer Header;

    mfxU16 reserved[10];
    mfxU32 Size;
    union
    {
        mfxCam3DLutEntry* Table;
        mfxU64 reserved1;
    };
} mfxExtCam3DLut;
MFX_PACK_END()

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __MFXCAMERA_H__

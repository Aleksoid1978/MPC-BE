/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
// Pre-compilation
#include "MediaInfo/PreComp.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Setup.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_AAC_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Aac.h"
#include "MediaInfo/Audio/File_Aac_GeneralAudio_Sbr.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include <cmath>
#include <algorithm>
#include <cstring>
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{
//***************************************************************************
// Tables
//***************************************************************************

//---------------------------------------------------------------------------
const int8s t_huffman_env_1_5dB[120][2]=
{
    {   1,    2 },
    { -64,  -65 },
    {   3,    4 },
    { -63,  -66 },
    {   5,    6 },
    { -62,  -67 },
    {   7,    8 },
    { -61,  -68 },
    {   9,   10 },
    { -60,  -69 },
    {  11,   12 },
    { -59,  -70 },
    {  13,   14 },
    { -58,  -71 },
    {  15,   16 },
    { -57,  -72 },
    {  17,   18 },
    { -73,  -56 },
    {  19,   21 },
    { -74,   20 },
    { -55,  -75 },
    {  22,   26 },
    {  23,   24 },
    { -54,  -76 },
    { -77,   25 },
    { -53,  -78 },
    {  27,   34 },
    {  28,   29 },
    { -52,  -79 },
    {  30,   31 },
    { -80,  -51 },
    {  32,   33 },
    { -83,  -82 },
    { -81,  -50 },
    {  35,   57 },
    {  36,   40 },
    {  37,   38 },
    { -88,  -84 },
    { -48,   39 },
    { -90,  -85 },
    {  41,   46 },
    {  42,   43 },
    { -49,  -87 },
    {  44,   45 },
    { -89,  -86 },
    {-124, -123 },
    {  47,   50 },
    {  48,   49 },
    {-122, -121 },
    {-120, -119 },
    {  51,   54 },
    {  52,   53 },
    {-118, -117 },
    {-116, -115 },
    {  55,   56 },
    {-114, -113 },
    {-112, -111 },
    {  58,   89 },
    {  59,   74 },
    {  60,   67 },
    {  61,   64 },
    {  62,   63 },
    {-110, -109 },
    {-108, -107 },
    {  65,   66 },
    {-106, -105 },
    {-104, -103 },
    {  68,   71 },
    {  69,   70 },
    {-102, -101 },
    {-100,  -99 },
    {  72,   73 },
    { -98,  -97 },
    { -96,  -95 },
    {  75,   82 },
    {  76,   79 },
    {  77,   78 },
    { -94,  -93 },
    { -92,  -91 },
    {  80,   81 },
    { -47,  -46 },
    { -45,  -44 },
    {  83,   86 },
    {  84,   85 },
    { -43,  -42 },
    { -41,  -40 },
    {  87,   88 },
    { -39,  -38 },
    { -37,  -36 },
    {  90,  105 },
    {  91,   98 },
    {  92,   95 },
    {  93,   94 },
    { -35,  -34 },
    { -33,  -32 },
    {  96,   97 },
    { -31,  -30 },
    { -29,  -28 },
    {  99,  102 },
    { 100,  101 },
    { -27,  -26 },
    { -25,  -24 },
    { 103,  104 },
    { -23,  -22 },
    { -21,  -20 },
    { 106,  113 },
    { 107,  110 },
    { 108,  109 },
    { -19,  -18 },
    { -17,  -16 },
    { 111,  112 },
    { -15,  -14 },
    { -13,  -12 },
    { 114,  117 },
    { 115,  116 },
    { -11,  -10 },
    {  -9,   -8 },
    { 118,  119 },
    {  -7,   -6 },
    {  -5,   -4 }
};

//---------------------------------------------------------------------------
const int8s f_huffman_env_1_5dB[120][2]=
{
    {   1,    2 },
    { -64,  -65 },
    {   3,    4 },
    { -63,  -66 },
    {   5,    6 },
    { -67,  -62 },
    {   7,    8 },
    { -68,  -61 },
    {   9,   10 },
    { -69,  -60 },
    {  11,   13 },
    { -70,   12 },
    { -59,  -71 },
    {  14,   16 },
    { -58,   15 },
    { -72,  -57 },
    {  17,   19 },
    { -73,   18 },
    { -56,  -74 },
    {  20,   23 },
    {  21,   22 },
    { -55,  -75 },
    { -54,  -53 },
    {  24,   27 },
    {  25,   26 },
    { -76,  -52 },
    { -77,  -51 },
    {  28,   31 },
    {  29,   30 },
    { -50,  -78 },
    { -79,  -49 },
    {  32,   36 },
    {  33,   34 },
    { -48,  -47 },
    { -80,   35 },
    { -81,  -82 },
    {  37,   47 },
    {  38,   41 },
    {  39,   40 },
    { -83,  -46 },
    { -45,  -84 },
    {  42,   44 },
    { -85,   43 },
    { -44,  -43 },
    {  45,   46 },
    { -88,  -87 },
    { -86,  -90 },
    {  48,   66 },
    {  49,   56 },
    {  50,   53 },
    {  51,   52 },
    { -92,  -42 },
    { -41,  -39 },
    {  54,   55 },
    {-105,  -89 },
    { -38,  -37 },
    {  57,   60 },
    {  58,   59 },
    { -94,  -91 },
    { -40,  -36 },
    {  61,   63 },
    { -20,   62 },
    {-115, -110 },
    {  64,   65 },
    {-108, -107 },
    {-101,  -97 },
    {  67,   89 },
    {  68,   75 },
    {  69,   72 },
    {  70,   71 },
    { -95,  -93 },
    { -34,  -27 },
    {  73,   74 },
    { -22,  -17 },
    { -16, -124 },
    {  76,   82 },
    {  77,   79 },
    {-123,   78 },
    {-122, -121 },
    {  80,   81 },
    {-120, -119 },
    {-118, -117 },
    {  83,   86 },
    {  84,   85 },
    {-116, -114 },
    {-113, -112 },
    {  87,   88 },
    {-111, -109 },
    {-106, -104 },
    {  90,  105 },
    {  91,   98 },
    {  92,   95 },
    {  93,   94 },
    {-103, -102 },
    {-100,  -99 },
    {  96,   97 },
    { -98,  -96 },
    { -35,  -33 },
    {  99,  102 },
    { 100,  101 },
    { -32,  -31 },
    { -30,  -29 },
    { 103,  104 },
    { -28,  -26 },
    { -25,  -24 },
    { 106,  113 },
    { 107,  110 },
    { 108,  109 },
    { -23,  -21 },
    { -19,  -18 },
    { 111,  112 },
    { -15,  -14 },
    { -13,  -12 },
    { 114,  117 },
    { 115,  116 },
    { -11,  -10 },
    {  -9,   -8 },
    { 118,  119 },
    {  -7,   -6 },
    {  -5,   -4 }
};

//---------------------------------------------------------------------------
const int8s t_huffman_env_bal_1_5dB[48][2]=
{
    { -64,    1 },
    { -63,    2 },
    { -65,    3 },
    { -62,    4 },
    { -66,    5 },
    { -61,    6 },
    { -67,    7 },
    { -60,    8 },
    { -68,    9 },
    {  10,   11 },
    { -69,  -59 },
    {  12,   13 },
    { -70,  -58 },
    {  14,   28 },
    {  15,   21 },
    {  16,   18 },
    { -57,   17 },
    { -71,  -56 },
    {  19,   20 },
    { -88,  -87 },
    { -86,  -85 },
    {  22,   25 },
    {  23,   24 },
    { -84,  -83 },
    { -82,  -81 },
    {  26,   27 },
    { -80,  -79 },
    { -78,  -77 },
    {  29,   36 },
    {  30,   33 },
    {  31,   32 },
    { -76,  -75 },
    { -74,  -73 },
    {  34,   35 },
    { -72,  -55 },
    { -54,  -53 },
    {  37,   41 },
    {  38,   39 },
    { -52,  -51 },
    { -50,   40 },
    { -49,  -48 },
    {  42,   45 },
    {  43,   44 },
    { -47,  -46 },
    { -45,  -44 },
    {  46,   47 },
    { -43,  -42 },
    { -41,  -40 }
};

//---------------------------------------------------------------------------
const int8s f_huffman_env_bal_1_5dB[48][2]=
{
    { -64,    1 },
    { -65,    2 },
    { -63,    3 },
    { -66,    4 },
    { -62,    5 },
    { -61,    6 },
    { -67,    7 },
    { -68,    8 },
    { -60,    9 },
    {  10,   11 },
    { -69,  -59 },
    { -70,   12 },
    { -58,   13 },
    {  14,   17 },
    { -71,   15 },
    { -57,   16 },
    { -56,  -73 },
    {  18,   32 },
    {  19,   25 },
    {  20,   22 },
    { -72,   21 },
    { -88,  -87 },
    {  23,   24 },
    { -86,  -85 },
    { -84,  -83 },
    {  26,   29 },
    {  27,   28 },
    { -82,  -81 },
    { -80,  -79 },
    {  30,   31 },
    { -78,  -77 },
    { -76,  -75 },
    {  33,   40 },
    {  34,   37 },
    {  35,   36 },
    { -74,  -55 },
    { -54,  -53 },
    {  38,   39 },
    { -52,  -51 },
    { -50,  -49 },
    {  41,   44 },
    {  42,   43 },
    { -48,  -47 },
    { -46,  -45 },
    {  45,   46 },
    { -44,  -43 },
    { -42,   47 },
    { -41,  -40 }
};

//---------------------------------------------------------------------------
const int8s t_huffman_env_3_0dB[62][2]=
{
    { -64,    1 },
    { -65,    2 },
    { -63,    3 },
    { -66,    4 },
    { -62,    5 },
    { -67,    6 },
    { -61,    7 },
    { -68,    8 },
    { -60,    9 },
    {  10,   11 },
    { -69,  -59 },
    {  12,   14 },
    { -70,   13 },
    { -71,  -58 },
    {  15,   18 },
    {  16,   17 },
    { -72,  -57 },
    { -73,  -74 },
    {  19,   22 },
    { -56,   20 },
    { -55,   21 },
    { -54,  -77 },
    {  23,   31 },
    {  24,   25 },
    { -75,  -76 },
    {  26,   27 },
    { -78,  -53 },
    {  28,   29 },
    { -52,  -95 },
    { -94,   30 },
    { -93,  -92 },
    {  32,   47 },
    {  33,   40 },
    {  34,   37 },
    {  35,   36 },
    { -91,  -90 },
    { -89,  -88 },
    {  38,   39 },
    { -87,  -86 },
    { -85,  -84 },
    {  41,   44 },
    {  42,   43 },
    { -83,  -82 },
    { -81,  -80 },
    {  45,   46 },
    { -79,  -51 },
    { -50,  -49 },
    {  48,   55 },
    {  49,   52 },
    {  50,   51 },
    { -48,  -47 },
    { -46,  -45 },
    {  53,   54 },
    { -44,  -43 },
    { -42,  -41 },
    {  56,   59 },
    {  57,   58 },
    { -40,  -39 },
    { -38,  -37 },
    {  60,   61 },
    { -36,  -35 },
    { -34,  -33 }
};

//---------------------------------------------------------------------------
const int8s f_huffman_env_3_0dB[62][2]=
{
    { -64,    1 },
    { -65,    2 },
    { -63,    3 },
    { -66,    4 },
    { -62,    5 },
    { -67,    6 },
    {   7,    8 },
    { -61,  -68 },
    {   9,   10 },
    { -60,  -69 },
    {  11,   12 },
    { -59,  -70 },
    {  13,   14 },
    { -58,  -71 },
    {  15,   16 },
    { -57,  -72 },
    {  17,   19 },
    { -56,   18 },
    { -55,  -73 },
    {  20,   24 },
    {  21,   22 },
    { -74,  -54 },
    { -53,   23 },
    { -75,  -76 },
    {  25,   30 },
    {  26,   27 },
    { -52,  -51 },
    {  28,   29 },
    { -77,  -79 },
    { -50,  -49 },
    {  31,   39 },
    {  32,   35 },
    {  33,   34 },
    { -78,  -46 },
    { -82,  -88 },
    {  36,   37 },
    { -83,  -48 },
    { -47,   38 },
    { -86,  -85 },
    {  40,   47 },
    {  41,   44 },
    {  42,   43 },
    { -80,  -44 },
    { -43,  -42 },
    {  45,   46 },
    { -39,  -87 },
    { -84,  -40 },
    {  48,   55 },
    {  49,   52 },
    {  50,   51 },
    { -95,  -94 },
    { -93,  -92 },
    {  53,   54 },
    { -91,  -90 },
    { -89,  -81 },
    {  56,   59 },
    {  57,   58 },
    { -45,  -41 },
    { -38,  -37 },
    {  60,   61 },
    { -36,  -35 },
    { -34,  -33 }
};

//---------------------------------------------------------------------------
const int8s t_huffman_env_bal_3_0dB[24][2]=
{
    { -64,    1 },
    { -63,    2 },
    { -65,    3 },
    { -66,    4 },
    { -62,    5 },
    { -61,    6 },
    { -67,    7 },
    { -68,    8 },
    { -60,    9 },
    {  10,   16 },
    {  11,   13 },
    { -69,   12 },
    { -76,  -75 },
    {  14,   15 },
    { -74,  -73 },
    { -72,  -71 },
    {  17,   20 },
    {  18,   19 },
    { -70,  -59 },
    { -58,  -57 },
    {  21,   22 },
    { -56,  -55 },
    { -54,   23 },
    { -53,  -52 }
};

//---------------------------------------------------------------------------
const int8s f_huffman_env_bal_3_0dB[24][2]=
{
    { -64,    1 },
    { -65,    2 },
    { -63,    3 },
    { -66,    4 },
    { -62,    5 },
    { -61,    6 },
    { -67,    7 },
    { -68,    8 },
    { -60,    9 },
    {  10,   13 },
    { -69,   11 },
    { -59,   12 },
    { -58,  -76 },
    {  14,   17 },
    {  15,   16 },
    { -75,  -74 },
    { -73,  -72 },
    {  18,   21 },
    {  19,   20 },
    { -71,  -70 },
    { -57,  -56 },
    {  22,   23 },
    { -55,  -54 },
    { -53,  -52 }
};

//---------------------------------------------------------------------------
const int8s t_huffman_noise_3_0dB[62][2]=
{
    { -64,    1 },
    { -63,    2 },
    { -65,    3 },
    { -66,    4 },
    { -62,    5 },
    { -67,    6 },
    {   7,    8 },
    { -61,  -68 },
    {   9,   30 },
    {  10,   15 },
    { -60,   11 },
    { -69,   12 },
    {  13,   14 },
    { -59,  -53 },
    { -95,  -94 },
    {  16,   23 },
    {  17,   20 },
    {  18,   19 },
    { -93,  -92 },
    { -91,  -90 },
    {  21,   22 },
    { -89,  -88 },
    { -87,  -86 },
    {  24,   27 },
    {  25,   26 },
    { -85,  -84 },
    { -83,  -82 },
    {  28,   29 },
    { -81,  -80 },
    { -79,  -78 },
    {  31,   46 },
    {  32,   39 },
    {  33,   36 },
    {  34,   35 },
    { -77,  -76 },
    { -75,  -74 },
    {  37,   38 },
    { -73,  -72 },
    { -71,  -70 },
    {  40,   43 },
    {  41,   42 },
    { -58,  -57 },
    { -56,  -55 },
    {  44,   45 },
    { -54,  -52 },
    { -51,  -50 },
    {  47,   54 },
    {  48,   51 },
    {  49,   50 },
    { -49,  -48 },
    { -47,  -46 },
    {  52,   53 },
    { -45,  -44 },
    { -43,  -42 },
    {  55,   58 },
    {  56,   57 },
    { -41,  -40 },
    { -39,  -38 },
    {  59,   60 },
    { -37,  -36 },
    { -35,   61 },
    { -34,  -33 }
};

//---------------------------------------------------------------------------
const int8s t_huffman_noise_bal_3_0dB[24][2]=
{
    { -64,    1 },
    { -65,    2 },
    { -63,    3 },
    {   4,    9 },
    { -66,    5 },
    { -62,    6 },
    {   7,    8 },
    { -76,  -75 },
    { -74,  -73 },
    {  10,   17 },
    {  11,   14 },
    {  12,   13 },
    { -72,  -71 },
    { -70,  -69 },
    {  15,   16 },
    { -68,  -67 },
    { -61,  -60 },
    {  18,   21 },
    {  19,   20 },
    { -59,  -58 },
    { -57,  -56 },
    {  22,   23 },
    { -55,  -54 },
    { -53,  -52 }
};

//---------------------------------------------------------------------------
// Master frequency band table
// k0 = lower frequency boundary
const int8s Aac_k0_offset[][2][16]=
{
    { //96000
        { -2, -1,  0,  1,  2, 3, 4, 5, 6, 7, 9, 11, 13, 16, 20, 24, },
        { -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 6,  8, 10, 13, 17, 21, },
    },
    { //88200
        { -2, -1,  0,  1,  2, 3, 4, 5, 6, 7, 9, 11, 13, 16, 20, 24, },
        { -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 6,  8, 10, 13, 17, 21, },
    },
    { //64000
        { -4, -2, -1,  0,  1,  2,  3,  4,  5, 6, 7, 9, 11, 13, 16, 20, },
        { -9, -7, -6, -5, -4, -3, -2, -1, -0, 1, 2, 4,  6,  8, 11, 15, },
    },
    { //48000
        {  -4, -2, -1,  0,  1,  2,  3,  4,  5, 6, 7, 9, 11, 13, 16, 20, },
        { -10, -8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 3,  5,  7, 10, 14, },
    },
    { //44100
        {  -4, -2, -1,  0,  1,  2,  3,  4,  5, 6, 7, 9, 11, 13, 16, 20, },
        { -10, -8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 3,  5,  7, 10, 14, },
    },
    { //32000
        {  -6,  -4,  -2, -1,   0,  1,  2,  3,  4,  5,  6,  7, 9, 11, 13, 16, },
        { -14, -12, -10, -9,  -8, -7, -6, -5, -4, -3, -2, -1, 1,  3,  5,  8, },
    },
    { //24000
        {  -5,  -3,  -2, -1,   0,  1,  2,  3,  4,  5,  6,  7, 9, 11, 13, 16, },
        { -13, -11, -10, -9,  -8, -7, -6, -5, -4, -3, -2, -1, 1,  3,  5,  8, },
    },
    { //22050
        {  -5,  -4,  -3, -2, -1,  0,  1,  2,  3,  4,  5,  6, 7, 9, 11, 13, },
        { -13, -12, -11, -10, -9, -8, -7, -6, -5, -4, -3, -2, -1, 1, 3, 5, },
    },
    { //16000
        {  -8,  -7,  -6,  -5,  -4,  -3,  -2,  -1,   0,   1,   2,  3,  4,  5,  6,  7, },
        { -20, -19, -18, -17, -16, -15, -14, -13, -12, -11, -10, -9, -8, -7, -6, -5, },
    },
    { //40000
        { -20, -19, -18, -17, -16, -15, -14, -13, -12, -11, -10,  -8,  -6,  -4, -2,  0, },
        { -27, -26, -25, -24, -23, -22, -21, -20, -19, -18, -17, -15, -13, -11, -9, -7, },
    },

};
const int8u Aac_k0_startMin[10]=
{
     7,  7, 10, 11, 12, 16, 16, 17, 24, 32,// 35, 48,  0,  0,  0,  0,
};

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
extern const char* Aac_audioObjectType(int8u audioObjectType);

//---------------------------------------------------------------------------
int8u Aac_AudioSpecificConfig_sampling_frequency_index(const int64s sampling_frequency, bool usac=false);

//---------------------------------------------------------------------------
// Master frequency band table
// k0 = lower frequency boundary
int8u Aac_k0_Compute(int8u bs_start_freq, int8u extension_sampling_frequency_index, sbr_ratio ratio)
{
    return Aac_k0_startMin[extension_sampling_frequency_index]+Aac_k0_offset[extension_sampling_frequency_index][(int)ratio][bs_start_freq];
}

//---------------------------------------------------------------------------
// Master frequency band table
// k2 = upper frequency boundary
int8u Aac_k2_Compute(int8u bs_stop_freq, int64s sampling_frequency, int8u k0, sbr_ratio ratio)
{
    switch (bs_stop_freq)
    {
        case 14 : return (int8u)min(64, 2*k0);
        case 15 : return (int8u)min(64, 3*k0);
        default : ;
    }

    int stopMin;
    if (sampling_frequency<32000)
        stopMin=(((2*6000*(ratio==DUAL?128:64))/sampling_frequency)+1)>>1;
    else if (sampling_frequency<64000)
        stopMin=(((2*8000*(ratio==DUAL?128:64))/sampling_frequency)+1)>>1;
    else
        stopMin=(((2*10000*(ratio==DUAL?128:64))/sampling_frequency)+1)>>1;

    stopMin=min(stopMin, 64);

    int8u Diff0[13];
    int8u Diff1[14];
    float Power=pow((float)64/(float)stopMin, (float)1/(float)13);
    float Power2=(float)stopMin;
    int8s Temp1=(int8s)(Power2+0.5);
    for (int8u k=0; k<13; k++)
    {
        int8s Temp0=Temp1;
        Power2*=Power;
        Temp1=(int8s)(Power2+0.5);
        Diff0[k]=Temp1-Temp0;
    }
    sort(Diff0, Diff0+13);

    Diff1[0]=stopMin;
    for (int8u Pos=1; Pos<14; Pos++)
        Diff1[Pos]=Diff1[Pos-1]+Diff0[Pos-1];

    return (int8u)min((int8u)64, Diff1[bs_stop_freq]);
}

//---------------------------------------------------------------------------
//Helper
int8u Aac_bands_Compute(bool warp, int8u bands, int8u a0, int8u a1, float quad=1.0)
{
    float div=(float)log(2.0);
    if (warp)
        div*=(float)1.3;

    return (int8u)((bands*log((float)a1/(float)a0)/div+0.5)/quad);
}

//---------------------------------------------------------------------------
// Master frequency band table
// Computing for bs_freq_scale = 0
bool Aac_f_master_Compute_0(int8u &num_env_bands_Master, int8u* f_Master, sbr_handler *sbr, int8u  k0, int8u  k2)
{
    int8u dk, numBands;
    if (sbr->bs_alter_scale)
    {
        dk=1;
        numBands=(((k2-k0+2)>>2)<<1);
    }
    else
    {
        dk=2;
        numBands=(((k2-k0)>>1)<<1);
    }

    int8u k2Achieved=k0+numBands*dk;
    int8s k2Diff=k2-k2Achieved;
    int8s vDk[64] = { 0 };
    for (int8u k=0; k<numBands; k++)
        vDk[k]=dk;

    if (k2Diff)
    {
        int8s  incr;
        int8u  k;
        if (k2Diff>0)
        {
            incr=-1;
            k=numBands-1;
        }
        else
        {
            incr=1;
            k=0;
        }

        while (k2Diff)
        {
            if (k >= 64)
                break;
            vDk[k]-=incr;
            k+=incr;
            k2Diff+=incr;
        }
    }

    f_Master[0]=k0;
    for (int8u k=1; k<=numBands; k++)
        f_Master[k]=f_Master[k-1]+vDk[k-1];

    num_env_bands_Master=numBands;

    return true;
}

//---------------------------------------------------------------------------
// Master frequency band table
// Computing for bs_freq_scale != 0
int int8u_cmp(const void *a, const void *b)
{
    return *(int8u*)a - *(int8u*)b;
}
bool Aac_f_master_Compute(int8u &num_env_bands_Master, int8u* f_Master, sbr_handler *sbr, int8u  k0, int8u  k2)
{
    int8u temp1[]={6, 5, 4 };
    int8u bands=temp1[sbr->bs_freq_scale-1];
    if (sbr->ratio==QUAD && k0<bands)
        bands=floor((float)k0/2);
    float divisor;
    if (sbr->ratio==QUAD && k0<bands*2)
        divisor=1.2;
    else
        divisor=1.0;

    int8u twoRegions, k1;
    if ((float)k2/(float)k0>2.2449)
    {
        twoRegions=1;
        k1=2*k0;
    }
    else
    {
        twoRegions=0;
        k1=k2;
    }

    int8u numBands0=2*Aac_bands_Compute(false, bands, k0, k1, divisor);
    if (numBands0 <= 0 || numBands0 >= 64)
        return false;

    int8u vDk0[64];
    int8u vk0[64];
    float Power=pow((float)k1/(float)k0, (float)1/(float)numBands0);
    float Power2=(float)k0;
    int8s Temp1=(int8s)(Power2+0.5);
    for (int8u k=0; k<=numBands0-1; k++)
    {
        int8s Temp0=Temp1;
        Power2*=Power;
        Temp1=(int8s)(Power2+0.5);
        vDk0[k]=Temp1-Temp0;
    }
    qsort(vDk0, numBands0, sizeof(int8u), int8u_cmp);
    vk0[0]=k0;
    for (int8u k=1; k<=numBands0; k++)
    {
        if (vDk0[k-1]==0)
            return false;
        vk0[k]=vk0[k-1]+vDk0[k-1];
    }

    if (!twoRegions)
    {
        for (int8u k=0; k<=numBands0; k++)
            f_Master[k]=vk0[k];
        num_env_bands_Master=numBands0;
        return true;
    }

    //With twoRegions
    int8u numBands1;
    int8u vDk1[64] = { 0 };
    int8u vk1[64];
    numBands1=2*Aac_bands_Compute(sbr->bs_alter_scale, bands, k1, k2, divisor);
    if (numBands1 == 0 || numBands0 + numBands1 >= 64)
        return false;

    Power=(float)pow((float)k2/(float)k1, (float)1/(float)numBands1);
    Power2=(float)k1;
    Temp1=(int8s)(Power2+0.5);
    for (int8u k=0; k<=numBands1-1; k++)
    {
        int8s Temp0=Temp1;
        Power2*=Power;
        Temp1=(int8s)(Power2+0.5);
        vDk1[k]=Temp1-Temp0;
    }

    if (vDk1[0]<vDk0[numBands0-1])
    {
        qsort(vDk1, numBands1, sizeof(int8u), int8u_cmp);
        int8u change=vDk0[numBands0-1]-vDk1[0];
        vDk1[0]=vDk0[numBands0-1];
        vDk1[numBands1 - 1] = vDk1[numBands1 - 1] - change;
    }

    qsort(vDk1, numBands1, sizeof(int8u), int8u_cmp);
    vk1[0]=k1;
    for (int8u k=1; k<=numBands1; k++)
    {
        if (vDk1[k-1]==0)
            return false;
        vk1[k]=vk1[k-1]+vDk1[k-1];
    }

    num_env_bands_Master=numBands0+numBands1;
    for (int8u k=0; k<=numBands0; k++)
        f_Master[k]=vk0[k];
    for (int8u k=numBands0+1; k <=num_env_bands_Master; k++)
        f_Master[k]=vk1[k-numBands0];

    return true;
}

//---------------------------------------------------------------------------
// Derived frequency border tables
bool Aac_bands_Compute(const int8u &num_env_bands_Master, int8u* f_Master, sbr_handler *sbr, int8u  k2)
{
    sbr->num_env_bands[1]=num_env_bands_Master-sbr->bs_xover_band;
    sbr->num_env_bands[0]=(sbr->num_env_bands[1]>>1)+(sbr->num_env_bands[1]-((sbr->num_env_bands[1]>>1)<<1));

    if (f_Master[sbr->bs_xover_band]>32)
        return false;

    if (sbr->bs_noise_bands==0)
        sbr->num_noise_bands=1;
    else
    {
        sbr->num_noise_bands=Aac_bands_Compute(false, sbr->bs_noise_bands, f_Master[sbr->bs_xover_band], k2);
        if (sbr->num_noise_bands>5)
            return false;
        if (!sbr->num_noise_bands)
            sbr->num_noise_bands++; //Never 0
    }

    return true;
}

//---------------------------------------------------------------------------
bool Aac_Sbr_Compute(sbr_handler *sbr, const int64s sampling_frequency, bool usac)
{
    int8u extension_sampling_frequency_index=Aac_AudioSpecificConfig_sampling_frequency_index((int32u)sampling_frequency, usac);
    if (extension_sampling_frequency_index==17)
        extension_sampling_frequency_index=9; // arrays have 40000 Hz item right after 16000 Hz item
    else if (extension_sampling_frequency_index>9)
        return false; //Not supported

    int8u k0=Aac_k0_Compute(sbr->bs_start_freq, extension_sampling_frequency_index, sbr->ratio);
    int8u k2=Aac_k2_Compute(sbr->bs_stop_freq, sampling_frequency, k0, sbr->ratio);
    if (k2<=k0)
        return false;
    if (sbr->ratio==QUAD)
    {
        if (k2-k0>56)
            return false;
    }
    else
    {
        switch (extension_sampling_frequency_index)
        {
            case  0 :
            case  1 :
            case  2 :
            case  3 : if ((k2-k0)>32) return false; break;
            case  4 : if ((k2-k0)>35) return false; break;
            default : if ((k2-k0)>48) return false; break;
        }
    }

    int8u  num_env_bands_Master;
    int8u  f_Master[64];
    if (sbr->bs_freq_scale==0)
    {
        if (!Aac_f_master_Compute_0(num_env_bands_Master, f_Master, sbr, k0, k2))
            return false;
    }
    else
    {
        if (!Aac_f_master_Compute(num_env_bands_Master, f_Master, sbr, k0, k2))
            return false;
    }
    if (num_env_bands_Master<=sbr->bs_xover_band)
        return false;
    if (!Aac_bands_Compute(num_env_bands_Master, f_Master, sbr, k2))
        return false;

    return true;
}

//---------------------------------------------------------------------------
int16u File_Aac::sbr_huff_dec(const sbr_huffman& Table, const char* Name)
{
    int8u bit;
    int16s index = 0;

    Element_Begin1(Name);
    while (index>=0)
    {
        Get_S1(1, bit,                                          "bit");
        index=Table[index][bit];
    }
    Element_End0();

    return index+64;
}

//---------------------------------------------------------------------------
void File_Aac::sbr_extension_data(size_t End, int8u id_aac, bool crc_flag)
{
    if (raw_data_block_Pos>=sbrs.size())
        sbrs.resize(raw_data_block_Pos+1);

    FILLING_BEGIN();
        if (Infos["Format_Settings_SBR"].empty())
        {
            Infos["Format_Profile"]=__T("HE-AAC");
            Ztring SamplingRate=Infos["SamplingRate"];
            if (SamplingRate.empty() && Frequency_b)
                SamplingRate.From_Number(Frequency_b);
            if (!Frequency_b && !SamplingRate.empty())
                Frequency_b=SamplingRate.To_int64s();
            Infos["SamplingRate"].From_Number((extension_sampling_frequency_index==(int8u)-1)?(Frequency_b*2):extension_sampling_frequency, 10);
            if (MediaInfoLib::Config.LegacyStreamDisplay_Get())
            {
                Infos["Format_Profile"]+=__T(" / LC");
                Infos["SamplingRate"]+=__T(" / ")+SamplingRate;
            }
            Infos["Format_Settings"]=__T("Implicit");
            Infos["Format_Settings_SBR"]=__T("Yes (Implicit)");
            Infos["Codec"]=Ztring().From_UTF8(Aac_audioObjectType(audioObjectType))+__T("-SBR");

            if (Frame_Count_Valid<32)
                Frame_Count_Valid=32; //We need to find the SBR header
        }
    FILLING_END();

    Element_Begin1("sbr_extension_data");
    bool bs_header_flag;
    if (crc_flag)
        Skip_S2(10,                                             "bs_sbr_crc_bits");
    //~ if (sbr_layer != SBR_STEREO_ENHANCE)
    //~ {
        Get_SB(bs_header_flag,                                  "bs_header_flag");
        if (bs_header_flag)
        {
            if (extension_sampling_frequency_index==(int8u)-1)
            {
                extension_sampling_frequency=(int32u)(Frequency_b*2);
                extension_sampling_frequency_index=Aac_AudioSpecificConfig_sampling_frequency_index(extension_sampling_frequency, false);
            }

            delete sbrs[raw_data_block_Pos];
            sbr=new sbr_handler;
            sbrs[raw_data_block_Pos]=sbr;

            sbr_header();

            if (!Aac_Sbr_Compute(sbr, Frequency_b*2, false))
            {
                delete sbrs[raw_data_block_Pos]; sbr=sbrs[raw_data_block_Pos]=NULL;
            }
        }
        else
            sbr=sbrs[raw_data_block_Pos];
    //~ }

    //Parsing
    if (sbr) //only if a header is found
    {
        sbr->bs_amp_res[0]=sbr->bs_amp_res_FromHeader; //Set up with header data
        sbr->bs_amp_res[1]=sbr->bs_amp_res_FromHeader; //Set up with header data
        sbr_data(id_aac);

        FILLING_BEGIN();
            if (Config->ParseSpeed<0.3)
            {
                Frame_Count_Valid=Frame_Count+1;
                if (Frame_Count<8)
                    Frame_Count_Valid+=8-Frame_Count;
            }
        FILLING_END();
    }
    if (Data_BS_Remain()>End)
        Skip_BS(Data_BS_Remain()-End,                           "bs_fill_bits");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Aac::sbr_header()
{
    Element_Begin1("sbr_header");
    Get_S1 (1, sbr->bs_amp_res_FromHeader,                      "bs_amp_res");
    Get_S1 (4, sbr->bs_start_freq,                              "bs_start_freq");
    Get_S1 (4, sbr->bs_stop_freq,                               "bs_stop_freq");
    Get_S1 (3, sbr->bs_xover_band,                              "bs_xover_band");
    Skip_S1(2,                                                  "bs_reserved");
    bool bs_header_extra_1, bs_header_extra_2;
    Get_SB (bs_header_extra_1,                                  "bs_header_extra_1");
    Get_SB (bs_header_extra_2,                                  "bs_header_extra_2");
    if(bs_header_extra_1)
    {
        Get_S1 (2, sbr->bs_freq_scale,                          "bs_freq_scale");
        Get_S1 (1, sbr->bs_alter_scale,                         "bs_alter_scale");
        Get_S1 (2, sbr->bs_noise_bands,                         "bs_noise_bands");
    }
    else
    {
        sbr->bs_freq_scale=2;
        sbr->bs_alter_scale=1;
        sbr->bs_noise_bands=2;
    }
    if(bs_header_extra_2)
    {
        Skip_S1(2,                                              "bs_limiter_bands");
        Skip_S1(2,                                              "bs_limiter_gains");
        Skip_SB(                                                "bs_interpol_freq");
        Skip_SB(                                                "bs_smoothing_mode");
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Aac::sbr_data(int8u id_aac)
{
    Element_Begin1("sbr_data");
    switch (id_aac)
    {
        case  0 : sbr_single_channel_element();     break; //ID_SCE
        case  1 : sbr_channel_pair_element();       break; //ID_CPE
        default : ;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Aac::sbr_single_channel_element()
{
    Element_Begin1("sbr_single_channel_element");
    bool bs_data_extra, bs_add_harmonic_flag, bs_extended_data;
    int8u bs_extension_size, bs_esc_count, bs_extension_id;
    Get_SB (bs_data_extra,                                      "bs_data_extra");
    if (bs_data_extra)
    {
        Skip_S1(4,                                              "bs_reserved");
    }
    sbr_grid(0);
    sbr_dtdf(0);
    sbr_invf(0);
    sbr_envelope(0, 0);
    sbr_noise(0, 0);
    Get_SB (bs_add_harmonic_flag,                               "bs_add_harmonic_flag[0]");
    if (bs_add_harmonic_flag)
        sbr_sinusoidal_coding(0);

    Get_SB (bs_extended_data,                                   "bs_extended_data[0]");
    if (bs_extended_data) {
        Get_S1 (4,bs_extension_size,                            "bs_extension_size");
        size_t cnt=bs_extension_size;
        if (cnt==15)
        {
            Get_S1 (8, bs_esc_count,                            "bs_esc_count");
            cnt+=bs_esc_count;
        }

        if (Data_BS_Remain()>=8*cnt)
        {
            size_t End=Data_BS_Remain()-8*cnt;
            while (Data_BS_Remain()>End+7)
            {
                Get_S1 (2,bs_extension_id,                      "bs_extension_id");
                switch (bs_extension_id)
                {
                    case 2 : ps_data(End); break; //EXTENSION_ID_PS
                    case 3 : esbr_data(End); break; //EXTENSION_ID_ESBR
                    default:
                            if (End<Data_BS_Remain())
                                Skip_BS(Data_BS_Remain()-End,   "(unknown)");
                }
            }
            if (End<Data_BS_Remain())
                Skip_BS(Data_BS_Remain()-End,                   "bs_fill_bits");
        }
        else
            Skip_BS(Data_BS_Remain(),                           "(Error)");

    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Aac::sbr_grid(bool ch)
{
    Element_Begin1("sbr_grid");
    int8u bs_frame_class, bs_num_rel_0, bs_num_rel_1, tmp;
    int ptr_bits;
    Get_S1(2, bs_frame_class,                                   "bs_frame_class");
    switch (bs_frame_class)
    {
        case 0 : //FIXFIX
            Get_S1 (2, tmp,                                     "tmp");
            sbr->bs_num_env[ch]=(int8u)pow(2.0, tmp);
            if (sbr->bs_num_env[ch]==1)
                sbr->bs_amp_res[ch]=0;
            Get_SB (   sbr->bs_freq_res[ch][0],                 "bs_freq_res[ch][0]");
            for (int8u env=1; env<sbr->bs_num_env[ch]; env++)
                sbr->bs_freq_res[ch][env]=sbr->bs_freq_res[ch][0];
            break;
        case 1 : //FIXVAR
            Skip_S1(2,                                          "bs_var_bord_1[ch]");
            Get_S1 (2, bs_num_rel_1,                            "bs_num_rel_1[ch]");
            sbr->bs_num_env[ch]=bs_num_rel_1+1;
            for (int8u rel=0; rel<sbr->bs_num_env[ch]-1; rel++) {
                Skip_S1(2,                                      "tmp");
            }
            ptr_bits=(int8u)ceil(log((double)sbr->bs_num_env[ch]+1)/log((double)2));
            Skip_BS(ptr_bits,                                   "bs_pointer[ch]");
            Element_Begin1("bs_freq_res[ch]");
            for (int8u env=0; env<sbr->bs_num_env[ch]; env++)
                Get_SB (sbr->bs_freq_res[ch][sbr->bs_num_env[ch]-1-env], "bs_freq_res[ch][bs_num_env[ch]-1-env]");
            Element_End0();
            break;
        case 2 : //VARFIX
            Skip_S1(2,                                          "bs_var_bord_0[ch]");
            Get_S1 (2,bs_num_rel_0,                             "bs_num_rel_0[ch]");
            sbr->bs_num_env[ch] = bs_num_rel_0 + 1;
            for (int8u rel=0; rel<sbr->bs_num_env[ch]-1; rel++)
                Skip_S1(2,                                      "tmp");
            ptr_bits=(int8u)ceil(log((double)sbr->bs_num_env[ch]+1)/log((double)2));
            Skip_BS(ptr_bits,                                   "bs_pointer[ch]");
            Element_Begin1("bs_freq_res[ch]");
            for (int8u env = 0; env < sbr->bs_num_env[ch]; env++)
                Get_SB (sbr->bs_freq_res[ch][env],              "bs_freq_res[ch][env]");
            Element_End0();
            break;
        case 3 : //VARVAR
            Skip_S1(2,                                          "bs_var_bord_0[ch]");
            Skip_S1(2,                                          "bs_var_bord_1[ch]");
            Get_S1 (2,bs_num_rel_0,                             "bs_num_rel_0[ch]");
            Get_S1 (2,bs_num_rel_1,                             "bs_num_rel_1[ch]");
            sbr->bs_num_env[ch] = bs_num_rel_0 + bs_num_rel_1 + 1;
            for (int8u rel=0; rel<bs_num_rel_0; rel++)
                Skip_S1(2,                                      "tmp");
            for (int8u rel=0; rel<bs_num_rel_1; rel++)
                Skip_S1(2,                                      "tmp");
            ptr_bits=(int8u)ceil(log((double)(sbr->bs_num_env[ch]+1))/log((double)2));
            Skip_BS(ptr_bits,                                   "bs_pointer[ch]");
            Element_Begin1("bs_freq_res[ch]");
            for (int8u env=0; env<sbr->bs_num_env[ch]; env++)
                Get_SB (sbr->bs_freq_res[ch][env],              "bs_freq_res[ch][env]");
            Element_End0();
            break;
    }
    if (sbr->bs_num_env[ch]>1)
        sbr->bs_num_noise[ch]=2;
    else
        sbr->bs_num_noise[ch]=1;
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Aac::sbr_channel_pair_element()
{
    Element_Begin1("sbr_channel_pair_element");
    bool bs_data_extra,bs_coupling,bs_add_harmonic_flag,bs_extended_data;
    int8u bs_extension_size,bs_esc_count,bs_extension_id;
    Get_SB (bs_data_extra,                                      "bs_data_extra");
    if (bs_data_extra) {
        Skip_S1(4,                                              "bs_reserved");
        Skip_S1(4,                                              "bs_reserved");
    }

    Get_SB (bs_coupling,                                        "bs_coupling");
    sbr_grid(0);
    if (bs_coupling)
    {
        //Coupling
        sbr->bs_num_env    [1]=sbr->bs_num_env    [0];
        sbr->bs_num_noise  [1]=sbr->bs_num_noise  [0];
        sbr->bs_amp_res    [1]=sbr->bs_amp_res    [0];
        for (int8u env=0; env<sbr->bs_num_env[0]; env++)
            sbr->bs_freq_res[1][env]=sbr->bs_freq_res[0][env];
    }
    else
        sbr_grid(1);
    sbr_dtdf(0);
    sbr_dtdf(1);
    sbr_invf(0);
    if (!bs_coupling)
        sbr_invf(1);
    sbr_envelope(0, bs_coupling);
    if (bs_coupling)
    {
        sbr_noise(0, bs_coupling);
        sbr_envelope(1, bs_coupling);
    }
    else
    {
        sbr_envelope(1, bs_coupling);
        sbr_noise(0, bs_coupling);
    }
    sbr_noise(1, bs_coupling);
    Get_SB (bs_add_harmonic_flag,                               "bs_add_harmonic_flag[0]");
    if (bs_add_harmonic_flag)
        sbr_sinusoidal_coding(0);
    Get_SB (bs_add_harmonic_flag,                               "bs_add_harmonic_flag[1]");
    if (bs_add_harmonic_flag)
        sbr_sinusoidal_coding(1);

    Get_SB (bs_extended_data,                                   "bs_extended_data");
    if (bs_extended_data) {
        Get_S1(4,bs_extension_size,                             "bs_extension_size");
        size_t cnt = bs_extension_size;
        if (cnt == 15)
        {
            Get_S1(8,bs_esc_count,                              "bs_esc_count");
            cnt += bs_esc_count;
        }
        if (Data_BS_Remain()>=8*cnt)
        {
            size_t End=Data_BS_Remain()-8*cnt;
            while (Data_BS_Remain()>End+7)
            {
                Get_S1 (2,bs_extension_id,                      "bs_extension_id");
                switch (bs_extension_id)
                {
                    case 2 : ps_data(End); break; //EXTENSION_ID_PS
                    case 3 : esbr_data(End); break; //EXTENSION_ID_ESBR
                    default:
                            if (End<Data_BS_Remain())
                                Skip_BS(Data_BS_Remain()-End,   "(unknown)");
                }
            }
            if (End<Data_BS_Remain())
                Skip_BS(Data_BS_Remain()-End,                   "bs_fill_bits");
        }
        else
            Skip_BS(Data_BS_Remain(),                           "(Error)");

    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Aac::sbr_dtdf(bool ch)
{
    Element_Begin1("sbr_dtdf");
    for (int env=0; env<sbr->bs_num_env[ch]; env++)
        Get_S1 (1, sbr->bs_df_env[ch][env],                     "bs_df_env[ch][env]");
    for (int noise=0; noise<sbr->bs_num_noise[ch]; noise++)
        Get_S1 (1, sbr->bs_df_noise[ch][noise],                 "bs_df_noise[ch][noise]");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Aac::sbr_invf(bool)
{
    Element_Begin1("sbr_invf");
    for (int n = 0; n<sbr->num_noise_bands; n++ )
         Skip_S1(2,                                             "bs_invf_mode[ch][n]");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Aac::sbr_envelope(bool ch, bool bs_coupling)
{
    sbr_huffman t_huff, f_huff;
    Element_Begin1("sbr_envelope");
    if (bs_coupling && ch)
    {
        if (sbr->bs_amp_res[ch])
        {
            t_huff = t_huffman_env_bal_3_0dB;
            f_huff = f_huffman_env_bal_3_0dB;
        }
        else
        {
            t_huff = t_huffman_env_bal_1_5dB;
            f_huff = f_huffman_env_bal_1_5dB;
        }
    }
    else
    {
        if (sbr->bs_amp_res[ch])
        {
            t_huff = t_huffman_env_3_0dB;
            f_huff = f_huffman_env_3_0dB;
        }
        else
        {
            t_huff = t_huffman_env_1_5dB;
            f_huff = f_huffman_env_1_5dB;
        }
    }

    for (int8u env=0; env<sbr->bs_num_env[ch]; env++)
    {
        if (sbr->bs_df_env[ch][env] == 0)
        {
            if (bs_coupling && ch)
                Skip_S1(sbr->bs_amp_res[ch]?5:6,                "bs_env_start_value_balance");
            else
                Skip_S1(sbr->bs_amp_res[ch]?6:7,                "bs_env_start_value_level");
            for (int8u band = 1; band < sbr->num_env_bands[sbr->bs_freq_res[ch][env]]; band++)
                sbr_huff_dec(f_huff,                            "bs_data_env[ch][env][band]");
        }
        else
        {
            for (int8u band = 0; band < sbr->num_env_bands[sbr->bs_freq_res[ch][env]]; band++)
                sbr_huff_dec(t_huff,                            "bs_data_env[ch][env][band]");
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Aac::sbr_noise(bool ch, bool bs_coupling)
{
    sbr_huffman t_huff, f_huff;
    Element_Begin1("sbr_noise");
    if (bs_coupling && ch)
    {
        t_huff = t_huffman_noise_bal_3_0dB;
        f_huff = f_huffman_env_bal_3_0dB;
    }
    else
    {
        t_huff = t_huffman_noise_3_0dB;
        f_huff = f_huffman_env_3_0dB;
    }

    for (int noise=0; noise<sbr->bs_num_noise[ch]; noise++)
    {
        if (sbr->bs_df_noise[ch][noise] == 0)
        {
            Skip_S1(5,                                          (bs_coupling && ch)?"bs_noise_start_value_balance":"bs_noise_start_value_level");
            for (int8u band=1; band<sbr->num_noise_bands; band++)
                sbr_huff_dec(f_huff,                            "bs_data_noise[ch][noise][band]");
        }
        else
        {
            for (int8u band = 0; band < sbr->num_noise_bands; band++)
                sbr_huff_dec(t_huff,                            "bs_data_noise[ch][noise][band]");
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Aac::sbr_sinusoidal_coding(bool)
{
    Element_Begin1("sbr_sinusoidal_coding");
    for (int8u n=0; n<sbr->num_env_bands[1]; n++)
        Skip_SB(                                                "bs_add_harmonic[ch][n]");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Aac::esbr_data(size_t End)
{
    Skip_BS(Data_BS_Remain()-End,                               "(not implemented)");

    FILLING_BEGIN();
        if (Infos["Format_Profile"].find(__T("eSBR"))==string::npos)
        {
            Infos["Format_Profile"]=__T("HE-AAC+eSBR");
        }
    FILLING_END();
}

} //NameSpace

#endif //MEDIAINFO_AAC_YES


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
#if defined(MEDIAINFO_AAC_YES) || defined(MEDIAINFO_MPEGH3DA_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Usac.h"
#include "MediaInfo/Audio/File_Aac_GeneralAudio.h"
#include "MediaInfo/Audio/File_Aac_GeneralAudio_Sbr.h"
#include <algorithm>
#include <cmath>

using namespace std;

#define ARITH_ESCAPE 16

//---------------------------------------------------------------------------

namespace MediaInfoLib
{
//***************************************************************************
// Tables
//***************************************************************************

struct stts_struct
{
    int32u                      SampleCount;
    int32u                      SampleDuration;
};

struct sgpd_prol_struct
{
    int16s                      roll_distance;
};

struct sbgp_struct
{
    int64u                      FirstSample;
    int64u                      LastSample;
    int32u                      group_description_index;

    bool operator==(int64u SamplePos) const
    {
        return SamplePos>=FirstSample && SamplePos<LastSample;
    }
};

//***************************************************************************
//---------------------------------------------------------------------------
static const int16u huffman_scl[][4]=
{
    {0x00f3, 0x00f3, 0x0004, 0x0008},
    {0x00ef, 0x00ef, 0x00f5, 0x00e9},
    {0x00f9, 0x000c, 0x0010, 0x0014},
    {0x00e7, 0x00e7, 0x00ff, 0x00ff},
    {0x00e1, 0x0101, 0x00dd, 0x0105},
    {0x0018, 0x001c, 0x0020, 0x0028},
    {0x010b, 0x010b, 0x00db, 0x00db},
    {0x010f, 0x010f, 0x00d5, 0x0111},
    {0x00d1, 0x0115, 0x00cd, 0x0024},
    {0x011b, 0x011b, 0x00cb, 0x00cb},
    {0x002c, 0x0030, 0x0034, 0x0040},
    {0x00c7, 0x00c7, 0x011f, 0x011f},
    {0x0121, 0x00c1, 0x0125, 0x00bd},
    {0x0129, 0x00b9, 0x0038, 0x003c},
    {0x0133, 0x0133, 0x012f, 0x012f},
    {0x0137, 0x0137, 0x013b, 0x013b},
    {0x0044, 0x0048, 0x004c, 0x0058},
    {0x00b7, 0x00b7, 0x00af, 0x00af},
    {0x00b1, 0x013d, 0x00a9, 0x00a5},
    {0x0141, 0x00a1, 0x0050, 0x0054},
    {0x0147, 0x0147, 0x009f, 0x009f},
    {0x014b, 0x014b, 0x009b, 0x009b},
    {0x005c, 0x0060, 0x0064, 0x0070},
    {0x014f, 0x014f, 0x0095, 0x008d},
    {0x0155, 0x0085, 0x0091, 0x0089},
    {0x0151, 0x0081, 0x0068, 0x006c},
    {0x015f, 0x015f, 0x0167, 0x0167},
    {0x007b, 0x007b, 0x007f, 0x007f},
    {0x0074, 0x0078, 0x0080, 0x00b0},
    {0x0159, 0x0075, 0x0069, 0x006d},
    {0x0071, 0x0061, 0x0161, 0x007c},
    {0x0067, 0x0067, 0x005b, 0x005b},
    {0x0084, 0x0088, 0x008c, 0x009c},
    {0x005f, 0x005f, 0x0169, 0x0055},
    {0x004d, 0x000d, 0x0005, 0x0009},
    {0x0001, 0x0090, 0x0094, 0x0098},
    {0x018b, 0x018b, 0x018f, 0x018f},
    {0x0193, 0x0193, 0x0197, 0x0197},
    {0x019b, 0x019b, 0x01d7, 0x01d7},
    {0x00a0, 0x00a4, 0x00a8, 0x00ac},
    {0x0187, 0x0187, 0x016f, 0x016f},
    {0x0173, 0x0173, 0x0177, 0x0177},
    {0x017b, 0x017b, 0x017f, 0x017f},
    {0x0183, 0x0183, 0x01a3, 0x01a3},
    {0x00b4, 0x00c8, 0x00dc, 0x00f0},
    {0x00b8, 0x00bc, 0x00c0, 0x00c4},
    {0x01bf, 0x01bf, 0x01c3, 0x01c3},
    {0x01c7, 0x01c7, 0x01cb, 0x01cb},
    {0x01cf, 0x01cf, 0x01d3, 0x01d3},
    {0x01bb, 0x01bb, 0x01a7, 0x01a7},
    {0x00cc, 0x00d0, 0x00d4, 0x00d8},
    {0x01ab, 0x01ab, 0x01af, 0x01af},
    {0x01b3, 0x01b3, 0x01b7, 0x01b7},
    {0x01db, 0x01db, 0x001b, 0x001b},
    {0x0023, 0x0023, 0x0027, 0x0027},
    {0x00e0, 0x00e4, 0x00e8, 0x00ec},
    {0x002b, 0x002b, 0x0017, 0x0017},
    {0x019f, 0x019f, 0x01e3, 0x01e3},
    {0x01df, 0x01df, 0x0013, 0x0013},
    {0x001f, 0x001f, 0x003f, 0x003f},
    {0x00f4, 0x00f8, 0x00fc, 0x0100},
    {0x0043, 0x0043, 0x004b, 0x004b},
    {0x0053, 0x0053, 0x0047, 0x0047},
    {0x002f, 0x002f, 0x0033, 0x0033},
    {0x003b, 0x003b, 0x0037, 0x0037}
};

//---------------------------------------------------------------------------
static const int16u arith_lookup_m[]=
{
    0x01,0x34,0x0D,0x13,0x12,0x25,0x00,0x3A,0x05,0x00,0x21,0x13,0x1F,0x1A,0x1D,0x36,
    0x24,0x2B,0x1B,0x33,0x37,0x29,0x1D,0x33,0x37,0x33,0x37,0x33,0x37,0x33,0x2C,0x00,
    0x21,0x13,0x25,0x2A,0x00,0x21,0x24,0x12,0x2C,0x1E,0x37,0x24,0x1F,0x35,0x37,0x24,
    0x35,0x37,0x35,0x37,0x38,0x2D,0x21,0x29,0x1E,0x21,0x13,0x2D,0x36,0x38,0x29,0x36,
    0x37,0x24,0x36,0x38,0x37,0x38,0x00,0x20,0x23,0x20,0x23,0x36,0x38,0x24,0x3B,0x24,
    0x26,0x29,0x1F,0x30,0x2D,0x0D,0x12,0x3F,0x2D,0x21,0x1C,0x2A,0x00,0x21,0x12,0x1E,
    0x36,0x38,0x36,0x37,0x3F,0x1E,0x0D,0x1F,0x2A,0x1E,0x21,0x24,0x12,0x2A,0x3C,0x21,
    0x24,0x1F,0x3C,0x21,0x29,0x36,0x38,0x36,0x37,0x38,0x21,0x1E,0x00,0x3B,0x25,0x1E,
    0x20,0x10,0x1F,0x3C,0x20,0x23,0x29,0x08,0x23,0x12,0x08,0x23,0x21,0x38,0x00,0x20,
    0x13,0x20,0x3B,0x1C,0x20,0x3B,0x29,0x20,0x23,0x24,0x21,0x24,0x21,0x24,0x3B,0x13,
    0x23,0x26,0x23,0x13,0x21,0x24,0x26,0x29,0x12,0x22,0x2B,0x02,0x1E,0x0D,0x1F,0x2D,
    0x00,0x0D,0x12,0x00,0x3C,0x21,0x29,0x3C,0x21,0x2A,0x3C,0x3B,0x22,0x1E,0x20,0x10,
    0x1F,0x3C,0x0D,0x29,0x3C,0x21,0x24,0x08,0x23,0x20,0x38,0x39,0x3C,0x20,0x13,0x3C,
    0x00,0x0D,0x13,0x1F,0x3C,0x09,0x26,0x1F,0x08,0x09,0x26,0x12,0x08,0x23,0x29,0x20,
    0x23,0x21,0x24,0x20,0x13,0x20,0x3B,0x16,0x20,0x3B,0x29,0x20,0x3B,0x29,0x20,0x3B,
    0x13,0x21,0x24,0x29,0x0B,0x13,0x09,0x3B,0x13,0x09,0x3B,0x13,0x21,0x3B,0x13,0x0D,
    0x26,0x29,0x26,0x29,0x3D,0x12,0x22,0x28,0x2E,0x04,0x08,0x13,0x3C,0x3B,0x3C,0x20,
    0x10,0x3C,0x21,0x07,0x08,0x10,0x00,0x08,0x0D,0x29,0x08,0x0D,0x29,0x08,0x09,0x13,
    0x20,0x23,0x39,0x08,0x09,0x13,0x08,0x09,0x16,0x08,0x09,0x10,0x12,0x20,0x3B,0x3D,
    0x09,0x26,0x20,0x3B,0x24,0x39,0x09,0x26,0x20,0x0D,0x13,0x00,0x09,0x13,0x20,0x0D,
    0x26,0x12,0x20,0x3B,0x13,0x21,0x26,0x0B,0x12,0x09,0x3B,0x16,0x09,0x3B,0x3D,0x09,
    0x26,0x0D,0x13,0x26,0x3D,0x1C,0x12,0x1F,0x28,0x2E,0x07,0x0B,0x08,0x09,0x00,0x39,
    0x0B,0x08,0x26,0x08,0x09,0x13,0x20,0x0B,0x39,0x10,0x39,0x0D,0x13,0x20,0x10,0x12,
    0x09,0x13,0x20,0x3B,0x13,0x09,0x26,0x0B,0x09,0x3B,0x1C,0x09,0x3B,0x13,0x20,0x3B,
    0x13,0x09,0x26,0x0B,0x16,0x0D,0x13,0x09,0x13,0x09,0x13,0x26,0x3D,0x1C,0x1F,0x28,
    0x2E,0x07,0x10,0x39,0x0B,0x39,0x39,0x13,0x39,0x0B,0x39,0x0B,0x39,0x26,0x39,0x10,
    0x20,0x3B,0x16,0x20,0x10,0x09,0x26,0x0B,0x13,0x09,0x13,0x26,0x1C,0x0B,0x3D,0x1C,
    0x1F,0x28,0x2B,0x07,0x0C,0x39,0x0B,0x39,0x0B,0x0C,0x0B,0x26,0x0B,0x26,0x3D,0x0D,
    0x1C,0x14,0x28,0x2B,0x39,0x0B,0x0C,0x0E,0x3D,0x1C,0x0D,0x12,0x22,0x2B,0x07,0x0C,
    0x0E,0x3D,0x1C,0x10,0x1F,0x2B,0x0C,0x0E,0x19,0x14,0x10,0x1F,0x28,0x0C,0x0E,0x19,
    0x14,0x26,0x22,0x2B,0x0C,0x0E,0x19,0x14,0x26,0x28,0x0E,0x19,0x14,0x26,0x28,0x0E,
    0x19,0x14,0x28,0x0E,0x19,0x14,0x22,0x28,0x2B,0x0E,0x14,0x2B,0x31,0x00,0x3A,0x3A,
    0x05,0x05,0x1B,0x1D,0x33,0x06,0x35,0x35,0x20,0x21,0x37,0x21,0x24,0x05,0x1B,0x2C,
    0x2C,0x2C,0x06,0x34,0x1E,0x34,0x00,0x08,0x36,0x09,0x21,0x26,0x1C,0x2C,0x00,0x02,
    0x02,0x02,0x3F,0x04,0x04,0x04,0x34,0x39,0x20,0x0A,0x0C,0x39,0x0B,0x0F,0x07,0x07,
    0x07,0x07,0x34,0x39,0x39,0x0A,0x0C,0x39,0x0C,0x0F,0x07,0x07,0x07,0x00,0x39,0x39,
    0x0C,0x0F,0x07,0x07,0x39,0x0C,0x0F,0x07,0x39,0x0C,0x0F,0x39,0x39,0x0C,0x0F,0x39,
    0x0C,0x39,0x0C,0x0F,0x00,0x11,0x27,0x17,0x2F,0x27,0x00,0x27,0x17,0x00,0x11,0x17,
    0x00,0x11,0x17,0x11,0x00,0x27,0x15,0x11,0x17,0x01,0x15,0x11,0x15,0x11,0x15,0x15,
    0x17,0x00,0x27,0x01,0x27,0x27,0x15,0x00,0x27,0x11,0x27,0x15,0x15,0x15,0x27,0x15,
    0x15,0x15,0x15,0x17,0x2F,0x11,0x17,0x27,0x27,0x27,0x11,0x27,0x15,0x27,0x27,0x15,
    0x15,0x27,0x17,0x2F,0x27,0x17,0x2F,0x27,0x17,0x2F,0x27,0x17,0x2F,0x27,0x17,0x2F,
    0x27,0x17,0x2F,0x27,0x17,0x2F,0x27,0x17,0x2F,0x27,0x17,0x2F,0x27,0x17,0x2F,0x27,
    0x17,0x2F,0x27,0x17,0x2F,0x27,0x17,0x2F,0x17,0x2F,0x2B,0x00,0x27,0x00,0x00,0x11,
    0x15,0x00,0x11,0x11,0x27,0x27,0x15,0x17,0x15,0x17,0x15,0x17,0x27,0x17,0x27,0x17,
    0x27,0x17,0x27,0x17,0x27,0x17,0x27,0x17,0x27,0x17,0x27,0x17,0x27,0x17,0x27,0x17,
    0x27,0x15,0x27,0x27,0x15,0x27
};

//---------------------------------------------------------------------------
static const int32u arith_hash_m[]=
{
    0x00000104UL,0x0000030AUL,0x00000510UL,0x00000716UL,0x00000A1FUL,0x00000F2EUL,
    0x00011100UL,0x00111103UL,0x00111306UL,0x00111436UL,0x00111623UL,0x00111929UL,
    0x00111F2EUL,0x0011221BUL,0x00112435UL,0x00112621UL,0x00112D12UL,0x00113130UL,
    0x0011331DUL,0x00113535UL,0x00113938UL,0x0011411BUL,0x00114433UL,0x00114635UL,
    0x00114F29UL,0x00116635UL,0x00116F24UL,0x00117433UL,0x0011FF0FUL,0x00121102UL,
    0x0012132DUL,0x00121436UL,0x00121623UL,0x00121912UL,0x0012213FUL,0x0012232DUL,
    0x00122436UL,0x00122638UL,0x00122A29UL,0x00122F2BUL,0x0012322DUL,0x00123436UL,
    0x00123738UL,0x00123B29UL,0x0012411DUL,0x00124536UL,0x00124938UL,0x00124F12UL,
    0x00125535UL,0x00125F29UL,0x00126535UL,0x0012B837UL,0x0013112AUL,0x0013131EUL,
    0x0013163BUL,0x0013212DUL,0x0013233CUL,0x00132623UL,0x00132F2EUL,0x0013321EUL,
    0x00133521UL,0x00133824UL,0x0013411EUL,0x00134336UL,0x00134838UL,0x00135135UL,
    0x00135537UL,0x00135F12UL,0x00137637UL,0x0013FF29UL,0x00140024UL,0x00142321UL,
    0x00143136UL,0x00143321UL,0x00143F25UL,0x00144321UL,0x00148638UL,0x0014FF29UL,
    0x00154323UL,0x0015FF12UL,0x0016F20CUL,0x0018A529UL,0x00210031UL,0x0021122CUL,
    0x00211408UL,0x00211713UL,0x00211F2EUL,0x0021222AUL,0x00212408UL,0x00212710UL,
    0x00212F2EUL,0x0021331EUL,0x00213436UL,0x00213824UL,0x0021412DUL,0x0021431EUL,
    0x00214536UL,0x00214F1FUL,0x00216637UL,0x00220004UL,0x0022122AUL,0x00221420UL,
    0x00221829UL,0x00221F2EUL,0x0022222DUL,0x00222408UL,0x00222623UL,0x00222929UL,
    0x00222F2BUL,0x0022321EUL,0x00223408UL,0x00223724UL,0x00223A29UL,0x0022411EUL,
    0x00224436UL,0x00224823UL,0x00225134UL,0x00225621UL,0x00225F12UL,0x00226336UL,
    0x00227637UL,0x0022FF29UL,0x0023112DUL,0x0023133CUL,0x00231420UL,0x00231916UL,
    0x0023212DUL,0x0023233CUL,0x00232509UL,0x00232929UL,0x0023312DUL,0x00233308UL,
    0x00233509UL,0x00233724UL,0x0023413CUL,0x00234421UL,0x00234A13UL,0x0023513CUL,
    0x00235421UL,0x00235F1FUL,0x00236421UL,0x0023FF29UL,0x00240024UL,0x0024153BUL,
    0x00242108UL,0x00242409UL,0x00242726UL,0x00243108UL,0x00243409UL,0x00243610UL,
    0x00244136UL,0x00244321UL,0x00244523UL,0x00244F1FUL,0x00245423UL,0x0024610AUL,
    0x00246423UL,0x0024FF29UL,0x00252510UL,0x00253121UL,0x0025343BUL,0x00254121UL,
    0x00254510UL,0x00254F25UL,0x00255221UL,0x0025FF12UL,0x00266513UL,0x0027F529UL,
    0x0029F101UL,0x002CF224UL,0x00310030UL,0x0031122AUL,0x00311420UL,0x00311816UL,
    0x0031212CUL,0x0031231EUL,0x00312408UL,0x00312710UL,0x0031312AUL,0x0031321EUL,
    0x00313408UL,0x00313623UL,0x0031411EUL,0x0031433CUL,0x00320007UL,0x0032122DUL,
    0x00321420UL,0x00321816UL,0x0032212DUL,0x0032233CUL,0x00322509UL,0x00322916UL,
    0x0032312DUL,0x00323420UL,0x00323710UL,0x00323F2BUL,0x00324308UL,0x00324623UL,
    0x00324F25UL,0x00325421UL,0x00325F1FUL,0x00326421UL,0x0032FF29UL,0x00331107UL,
    0x00331308UL,0x0033150DUL,0x0033211EUL,0x00332308UL,0x00332420UL,0x00332610UL,
    0x00332929UL,0x0033311EUL,0x00333308UL,0x0033363BUL,0x00333A29UL,0x0033413CUL,
    0x00334320UL,0x0033463BUL,0x00334A29UL,0x0033510AUL,0x00335320UL,0x00335824UL,
    0x0033610AUL,0x00336321UL,0x00336F12UL,0x00337623UL,0x00341139UL,0x0034153BUL,
    0x00342108UL,0x00342409UL,0x00342610UL,0x00343108UL,0x00343409UL,0x00343610UL,
    0x00344108UL,0x0034440DUL,0x00344610UL,0x0034510AUL,0x00345309UL,0x0034553BUL,
    0x0034610AUL,0x00346309UL,0x0034F824UL,0x00350029UL,0x00352510UL,0x00353120UL,
    0x0035330DUL,0x00353510UL,0x00354120UL,0x0035430DUL,0x00354510UL,0x00354F28UL,
    0x0035530DUL,0x00355510UL,0x00355F1FUL,0x00356410UL,0x00359626UL,0x0035FF12UL,
    0x00366426UL,0x0036FF12UL,0x0037F426UL,0x0039D712UL,0x003BF612UL,0x003DF81FUL,
    0x00410004UL,0x00411207UL,0x0041150DUL,0x0041212AUL,0x00412420UL,0x0041311EUL,
    0x00413308UL,0x00413509UL,0x00413F2BUL,0x00414208UL,0x00420007UL,0x0042123CUL,
    0x00421409UL,0x00422107UL,0x0042223CUL,0x00422409UL,0x00422610UL,0x0042313CUL,
    0x00423409UL,0x0042363BUL,0x0042413CUL,0x00424320UL,0x0042463BUL,0x00425108UL,
    0x00425409UL,0x0042FF29UL,0x00431107UL,0x00431320UL,0x0043153BUL,0x0043213CUL,
    0x00432320UL,0x00432610UL,0x0043313CUL,0x00433320UL,0x0043353BUL,0x00433813UL,
    0x00434108UL,0x00434409UL,0x00434610UL,0x00435108UL,0x0043553BUL,0x00435F25UL,
    0x00436309UL,0x0043753BUL,0x0043FF29UL,0x00441239UL,0x0044143BUL,0x00442139UL,
    0x00442309UL,0x0044253BUL,0x00443108UL,0x00443220UL,0x0044353BUL,0x0044410AUL,
    0x00444309UL,0x0044453BUL,0x00444813UL,0x0044510AUL,0x00445309UL,0x00445510UL,
    0x00445F25UL,0x0044630DUL,0x00450026UL,0x00452713UL,0x00453120UL,0x0045330DUL,
    0x00453510UL,0x00454120UL,0x0045430DUL,0x00454510UL,0x00455120UL,0x0045530DUL,
    0x00456209UL,0x00456410UL,0x0045FF12UL,0x00466513UL,0x0047FF22UL,0x0048FF25UL,
    0x0049F43DUL,0x004BFB25UL,0x004EF825UL,0x004FFF18UL,0x00511339UL,0x00512107UL,
    0x00513409UL,0x00520007UL,0x00521107UL,0x00521320UL,0x00522107UL,0x00522409UL,
    0x0052313CUL,0x00523320UL,0x0052353BUL,0x00524108UL,0x00524320UL,0x00531139UL,
    0x00531309UL,0x00532139UL,0x00532309UL,0x0053253BUL,0x00533108UL,0x0053340DUL,
    0x00533713UL,0x00534108UL,0x0053453BUL,0x00534F2BUL,0x00535309UL,0x00535610UL,
    0x00535F25UL,0x0053643BUL,0x00541139UL,0x00542139UL,0x00542309UL,0x00542613UL,
    0x00543139UL,0x00543309UL,0x00543510UL,0x00543F2BUL,0x00544309UL,0x00544510UL,
    0x00544F28UL,0x0054530DUL,0x0054FF12UL,0x00553613UL,0x00553F2BUL,0x00554410UL,
    0x0055510AUL,0x0055543BUL,0x00555F25UL,0x0055633BUL,0x0055FF12UL,0x00566513UL,
    0x00577413UL,0x0059FF28UL,0x005CC33DUL,0x005EFB28UL,0x005FFF18UL,0x00611339UL,
    0x00612107UL,0x00613320UL,0x0061A724UL,0x00621107UL,0x0062140BUL,0x00622107UL,
    0x00622320UL,0x00623139UL,0x00623320UL,0x00631139UL,0x0063130CUL,0x00632139UL,
    0x00632309UL,0x00633139UL,0x00633309UL,0x00633626UL,0x00633F2BUL,0x00634309UL,
    0x00634F2BUL,0x0063543BUL,0x0063FF12UL,0x0064343BUL,0x00643F2BUL,0x0064443BUL,
    0x00645209UL,0x00665513UL,0x0066610AUL,0x00666526UL,0x0067A616UL,0x0069843DUL,
    0x006CF612UL,0x006EF326UL,0x006FFF18UL,0x0071130CUL,0x00721107UL,0x00722239UL,
    0x0072291CUL,0x0072340BUL,0x00731139UL,0x00732239UL,0x0073630BUL,0x0073FF12UL,
    0x0074430BUL,0x00755426UL,0x00776F28UL,0x00777410UL,0x0078843DUL,0x007CF416UL,
    0x007EF326UL,0x007FFF18UL,0x00822239UL,0x00831139UL,0x0083430BUL,0x0084530BUL,
    0x0087561CUL,0x00887F25UL,0x00888426UL,0x008AF61CUL,0x008F0018UL,0x008FFF18UL,
    0x00911107UL,0x0093230BUL,0x0094530BUL,0x0097743DUL,0x00998C25UL,0x00999616UL,
    0x009EF825UL,0x009FFF18UL,0x00A3430BUL,0x00A4530BUL,0x00A7743DUL,0x00AA9F2BUL,
    0x00AAA616UL,0x00ABD61FUL,0x00AFFF18UL,0x00B3330BUL,0x00B44426UL,0x00B7643DUL,
    0x00BB971FUL,0x00BBB53DUL,0x00BEF512UL,0x00BFFF18UL,0x00C22139UL,0x00C5330EUL,
    0x00C7633DUL,0x00CCAF2EUL,0x00CCC616UL,0x00CFFF18UL,0x00D4440EUL,0x00D6420EUL,
    0x00DDCF2EUL,0x00DDD516UL,0x00DFFF18UL,0x00E4330EUL,0x00E6841CUL,0x00EEE61CUL,
    0x00EFFF18UL,0x00F3320EUL,0x00F55319UL,0x00F8F41CUL,0x00FAFF2EUL,0x00FF002EUL,
    0x00FFF10CUL,0x00FFF33DUL,0x00FFF722UL,0x00FFFF18UL,0x01000232UL,0x0111113EUL,
    0x01112103UL,0x0111311AUL,0x0112111AUL,0x01122130UL,0x01123130UL,0x0112411DUL,
    0x01131102UL,0x01132102UL,0x01133102UL,0x01141108UL,0x01142136UL,0x01143136UL,
    0x01144135UL,0x0115223BUL,0x01211103UL,0x0121211AUL,0x01213130UL,0x01221130UL,
    0x01222130UL,0x01223102UL,0x01231104UL,0x01232104UL,0x01233104UL,0x01241139UL,
    0x01241220UL,0x01242220UL,0x01251109UL,0x0125223BUL,0x0125810AUL,0x01283212UL,
    0x0131111AUL,0x01312130UL,0x0131222CUL,0x0131322AUL,0x0132122AUL,0x0132222DUL,
    0x0132322DUL,0x01331207UL,0x01332234UL,0x01333234UL,0x01341139UL,0x01343134UL,
    0x01344134UL,0x01348134UL,0x0135220BUL,0x0136110BUL,0x01365224UL,0x01411102UL,
    0x01412104UL,0x01431239UL,0x01432239UL,0x0143320AUL,0x01435134UL,0x01443107UL,
    0x01444134UL,0x01446134UL,0x0145220EUL,0x01455134UL,0x0147110EUL,0x01511102UL,
    0x01521239UL,0x01531239UL,0x01532239UL,0x01533107UL,0x0155220EUL,0x01555134UL,
    0x0157110EUL,0x01611107UL,0x01621239UL,0x01631239UL,0x01661139UL,0x01666134UL,
    0x01711107UL,0x01721239UL,0x01745107UL,0x0177110CUL,0x01811107UL,0x01821107UL,
    0x0185110CUL,0x0188210CUL,0x01911107UL,0x01933139UL,0x01A11107UL,0x01A31139UL,
    0x01F5220EUL,0x02000001UL,0x02000127UL,0x02000427UL,0x02000727UL,0x02000E2FUL,
    0x02110000UL,0x02111200UL,0x02111411UL,0x02111827UL,0x02111F2FUL,0x02112411UL,
    0x02112715UL,0x02113200UL,0x02113411UL,0x02113715UL,0x02114200UL,0x02121200UL,
    0x02121301UL,0x02121F2FUL,0x02122200UL,0x02122615UL,0x02122F2FUL,0x02123311UL,
    0x02123F2FUL,0x02124411UL,0x02131211UL,0x02132311UL,0x02133211UL,0x02184415UL,
    0x02211200UL,0x02211311UL,0x02211F2FUL,0x02212311UL,0x02212F2FUL,0x02213211UL,
    0x02221201UL,0x02221311UL,0x02221F2FUL,0x02222311UL,0x02222F2FUL,0x02223211UL,
    0x02223F2FUL,0x02231211UL,0x02232211UL,0x02232F2FUL,0x02233211UL,0x02233F2FUL,
    0x02287515UL,0x022DAB17UL,0x02311211UL,0x02311527UL,0x02312211UL,0x02321211UL,
    0x02322211UL,0x02322F2FUL,0x02323311UL,0x02323F2FUL,0x02331211UL,0x02332211UL,
    0x02332F2FUL,0x02333F2FUL,0x0237FF17UL,0x02385615UL,0x023D9517UL,0x02410027UL,
    0x02487827UL,0x024E3117UL,0x024FFF2FUL,0x02598627UL,0x025DFF2FUL,0x025FFF2FUL,
    0x02687827UL,0x026DFA17UL,0x026FFF2FUL,0x02796427UL,0x027E4217UL,0x027FFF2FUL,
    0x02888727UL,0x028EFF2FUL,0x028FFF2FUL,0x02984327UL,0x029F112FUL,0x029FFF2FUL,
    0x02A76527UL,0x02AEF717UL,0x02AFFF2FUL,0x02B7C827UL,0x02BEF917UL,0x02BFFF2FUL,
    0x02C66527UL,0x02CD5517UL,0x02CFFF2FUL,0x02D63227UL,0x02DDD527UL,0x02DFFF2BUL,
    0x02E84717UL,0x02EEE327UL,0x02EFFF2FUL,0x02F54527UL,0x02FCF817UL,0x02FFEF2BUL,
    0x02FFFA2FUL,0x02FFFE2FUL,0x03000127UL,0x03000201UL,0x03111200UL,0x03122115UL,
    0x03123200UL,0x03133211UL,0x03211200UL,0x03213127UL,0x03221200UL,0x03345215UL,
    0x04000F17UL,0x04122F17UL,0x043F6515UL,0x043FFF17UL,0x044F5527UL,0x044FFF17UL,
    0x045F0017UL,0x045FFF17UL,0x046F6517UL,0x04710027UL,0x047F4427UL,0x04810027UL,
    0x048EFA15UL,0x048FFF2FUL,0x049F4427UL,0x049FFF2FUL,0x04AEA727UL,0x04AFFF2FUL,
    0x04BE9C15UL,0x04BFFF2FUL,0x04CE5427UL,0x04CFFF2FUL,0x04DE3527UL,0x04DFFF17UL,
    0x04EE4627UL,0x04EFFF17UL,0x04FEF327UL,0x04FFFF2FUL,0x06000F27UL,0x069FFF17UL,
    0x06FFFF17UL,0x08110017UL,0x08EFFF15UL,0xFFFFFF00UL
};

//---------------------------------------------------------------------------
static const int16u arith_cf_m[][17]=
{
    {  708,  706,  579,  569,  568,  567,  479,  469,  297,  138,   97,   91,   72,   52,   38,   34,    0},
    { 7619, 6917, 6519, 6412, 5514, 5003, 4683, 4563, 3907, 3297, 3125, 3060, 2904, 2718, 2631, 2590,    0},
    { 7263, 4888, 4810, 4803, 1889,  415,  335,  327,  195,   72,   52,   49,   36,   20,   15,   14,    0},
    { 3626, 2197, 2188, 2187,  582,   57,   47,   46,   30,   12,    9,    8,    6,    4,    3,    2,    0},
    { 7806, 5541, 5451, 5441, 2720,  834,  691,  674,  487,  243,  179,  167,  139,   98,   77,   70,    0},
    { 6684, 4101, 4058, 4055, 1748,  426,  368,  364,  322,  257,  235,  232,  228,  222,  217,  215,    0},
    { 9162, 5964, 5831, 5819, 3269,  866,  658,  638,  535,  348,  258,  244,  234,  214,  195,  186,    0},
    {10638, 8491, 8365, 8351, 4418, 2067, 1859, 1834, 1190,  601,  495,  478,  356,  217,  174,  164,    0},
    {13389,10514,10032, 9961, 7166, 3488, 2655, 2524, 2015, 1140,  760,  672,  585,  426,  325,  283,    0},
    {14861,12788,12115,11952, 9987, 6657, 5323, 4984, 4324, 3001, 2205, 1943, 1764, 1394, 1115,  978,    0},
    {12876,10004, 9661, 9610, 7107, 3435, 2711, 2595, 2257, 1508, 1059,  952,  893,  753,  609,  538,    0},
    {15125,13591,13049,12874,11192, 8543, 7406, 7023, 6291, 4922, 4104, 3769, 3465, 2890, 2486, 2275,    0},
    {14574,13106,12731,12638,10453, 7947, 7233, 7037, 6031, 4618, 4081, 3906, 3465, 2802, 2476, 2349,    0},
    {15070,13179,12517,12351,10742, 7657, 6200, 5825, 5264, 3998, 3014, 2662, 2510, 2153, 1799, 1564,    0},
    {15542,14466,14007,13844,12489,10409, 9481, 9132, 8305, 6940, 6193, 5867, 5458, 4743, 4291, 4047,    0},
    {15165,14384,14084,13934,12911,11485,10844,10513,10002, 8993, 8380, 8051, 7711, 7036, 6514, 6233,    0},
    {15642,14279,13625,13393,12348, 9971, 8405, 7858, 7335, 6119, 4918, 4376, 4185, 3719, 3231, 2860,    0},
    {13408,13407,11471,11218,11217,11216, 9473, 9216, 6480, 3689, 2857, 2690, 2256, 1732, 1405, 1302,    0},
    {16098,15584,15191,14931,14514,13578,12703,12103,11830,11172,10475, 9867, 9695, 9281, 8825, 8389,    0},
    {15844,14873,14277,13996,13230,11535,10205, 9543, 9107, 8086, 7085, 6419, 6214, 5713, 5195, 4731,    0},
    {16131,15720,15443,15276,14848,13971,13314,12910,12591,11874,11225,10788,10573,10077, 9585, 9209,    0},
    {16331,16330,12283,11435,11434,11433, 8725, 8049, 6065, 4138, 3187, 2842, 2529, 2171, 1907, 1745,    0},
    {16011,15292,14782,14528,14008,12767,11556,10921,10591, 9759, 8813, 8043, 7855, 7383, 6863, 6282,    0},
    {16380,16379,15159,14610,14609,14608,12859,12111,11046, 9536, 8348, 7713, 7216, 6533, 5964, 5546,    0},
    {16367,16333,16294,16253,16222,16143,16048,15947,15915,15832,15731,15619,15589,15512,15416,15310,    0},
    {15967,15319,14937,14753,14010,12638,11787,11360,10805, 9706, 8934, 8515, 8166, 7456, 6911, 6575,    0},
    { 4906, 3005, 2985, 2984,  875,  102,   83,   81,   47,   17,   12,   11,    8,    5,    4,    3,    0},
    { 7217, 4346, 4269, 4264, 1924,  428,  340,  332,  280,  203,  179,  175,  171,  164,  159,  157,    0},
    {16010,15415,15032,14805,14228,13043,12168,11634,11265,10419, 9645, 9110, 8892, 8378, 7850, 7437,    0},
    { 8573, 5218, 5046, 5032, 2787,  771,  555,  533,  443,  286,  218,  205,  197,  181,  168,  162,    0},
    {11474, 8095, 7822, 7796, 4632, 1443, 1046, 1004,  748,  351,  218,  194,  167,  121,   93,   83,    0},
    {16152,15764,15463,15264,14925,14189,13536,13070,12846,12314,11763,11277,11131,10777,10383,10011,    0},
    {14187,11654,11043,10919, 8498, 4885, 3778, 3552, 2947, 1835, 1283, 1134,  998,  749,  585,  514,    0},
    {14162,11527,10759,10557, 8601, 5417, 4105, 3753, 3286, 2353, 1708, 1473, 1370, 1148,  959,  840,    0},
    {16205,15902,15669,15498,15213,14601,14068,13674,13463,12970,12471,12061,11916,11564,11183,10841,    0},
    {15043,12972,12092,11792,10265, 7446, 5934, 5379, 4883, 3825, 3036, 2647, 2507, 2185, 1901, 1699,    0},
    {15320,13694,12782,12352,11191, 8936, 7433, 6671, 6255, 5366, 4622, 4158, 4020, 3712, 3420, 3198,    0},
    {16255,16020,15768,15600,15416,14963,14440,14006,13875,13534,13137,12697,12602,12364,12084,11781,    0},
    {15627,14503,13906,13622,12557,10527, 9269, 8661, 8117, 6933, 5994, 5474, 5222, 4664, 4166, 3841,    0},
    {16366,16365,14547,14160,14159,14158,11969,11473, 8735, 6147, 4911, 4530, 3865, 3180, 2710, 2473,    0},
    {16257,16038,15871,15754,15536,15071,14673,14390,14230,13842,13452,13136,13021,12745,12434,12154,    0},
    {15855,14971,14338,13939,13239,11782,10585, 9805, 9444, 8623, 7846, 7254, 7079, 6673, 6262, 5923,    0},
    { 9492, 6318, 6197, 6189, 3004,  652,  489,  477,  333,  143,   96,   90,   78,   60,   50,   47,    0},
    {16313,16191,16063,15968,15851,15590,15303,15082,14968,14704,14427,14177,14095,13899,13674,13457,    0},
    { 8485, 5473, 5389, 5383, 2411,  494,  386,  377,  278,  150,  117,  112,  103,   89,   81,   78,    0},
    {10497, 7154, 6959, 6943, 3788, 1004,  734,  709,  517,  238,  152,  138,  120,   90,   72,   66,    0},
    {16317,16226,16127,16040,15955,15762,15547,15345,15277,15111,14922,14723,14671,14546,14396,14239,    0},
    {16382,16381,15858,15540,15539,15538,14704,14168,13768,13092,12452,11925,11683,11268,10841,10460,    0},
    { 5974, 3798, 3758, 3755, 1275,  205,  166,  162,   95,   35,   26,   24,   18,   11,    8,    7,    0},
    { 3532, 2258, 2246, 2244,  731,  135,  118,  115,   87,   45,   36,   34,   29,   21,   17,   16,    0},
    { 7466, 4882, 4821, 4811, 2476,  886,  788,  771,  688,  531,  469,  457,  437,  400,  369,  361,    0},
    { 9580, 5772, 5291, 5216, 3444, 1496, 1025,  928,  806,  578,  433,  384,  366,  331,  296,  273,    0},
    {10692, 7730, 7543, 7521, 4679, 1746, 1391, 1346, 1128,  692,  495,  458,  424,  353,  291,  268,    0},
    {11040, 7132, 6549, 6452, 4377, 1875, 1253, 1130,  958,  631,  431,  370,  346,  296,  253,  227,    0},
    {12687, 9332, 8701, 8585, 6266, 3093, 2182, 2004, 1683, 1072,  712,  608,  559,  458,  373,  323,    0},
    {13429, 9853, 8860, 8584, 6806, 4039, 2862, 2478, 2239, 1764, 1409, 1224, 1178, 1077,  979,  903,    0},
    {14685,12163,11061,10668, 9101, 6345, 4871, 4263, 3908, 3200, 2668, 2368, 2285, 2106, 1942, 1819,    0},
    {13295,11302,10999,10945, 7947, 5036, 4490, 4385, 3391, 2185, 1836, 1757, 1424,  998,  833,  785,    0},
    { 4992, 2993, 2972, 2970, 1269,  575,  552,  549,  530,  505,  497,  495,  493,  489,  486,  485,    0},
    {15419,13862,13104,12819,11429, 8753, 7220, 6651, 6020, 4667, 3663, 3220, 2995, 2511, 2107, 1871,    0},
    {12468, 9263, 8912, 8873, 5758, 2193, 1625, 1556, 1187,  589,  371,  330,  283,  200,  149,  131,    0},
    {15870,15076,14615,14369,13586,12034,10990,10423, 9953, 8908, 8031, 7488, 7233, 6648, 6101, 5712,    0},
    { 1693,  978,  976,  975,  194,   18,   16,   15,   11,    7,    6,    5,    4,    3,    2,    1,    0},
    { 7992, 5218, 5147, 5143, 2152,  366,  282,  276,  173,   59,   38,   35,   27,   16,   11,   10,    0}
};

//---------------------------------------------------------------------------
static const int16u arith_cf_r[][4]=
{
    {12571, 10569,  3696,     0},
    {12661,  5700,  3751,     0},
    {10827,  6884,  2929,     0}
};

//***************************************************************************
// Info
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_CONFORMANCE
const char* ConformanceLevel_Strings[]=
{
    "Errors",
    "Warnings",
    "Infos",
};
static_assert(sizeof(ConformanceLevel_Strings)/sizeof(const char*)==File_Usac::ConformanceLevel_Max, "");
#endif

//---------------------------------------------------------------------------
extern int8u Aac_AudioSpecificConfig_sampling_frequency_index(const int64s sampling_frequency);
extern const int32u Aac_sampling_frequency[];
extern const int8u Aac_Channels[];
extern string Aac_Channels_GetString(int8u ChannelLayout);
extern string Aac_ChannelConfiguration_GetString(int8u ChannelLayout);
extern string Aac_ChannelConfiguration2_GetString(int8u ChannelLayout);
extern string Aac_ChannelLayout_GetString(int8u ChannelLayout, bool IsMpegh3da=false, bool IsTip=false);
extern string Aac_OutputChannelPosition_GetString(int8u OutputChannelPosition);
extern bool Aac_Sbr_Compute(sbr_handler *sbr, int8u extension_sampling_frequency_index);
extern string Mpeg4_Descriptors_AudioProfileLevelString(const profilelevel_struct& ProfileLevel);

//---------------------------------------------------------------------------
enum usacElementType_Value
{
    ID_USAC_SCE,
    ID_USAC_CPE,
    ID_USAC_LFE,
    ID_USAC_EXT,
    ID_USAC_Max
};
static const char* usacElementType_IdNames[]=
{
    "SCE",
    "CPE",
    "LFE",
    "EXT",
};
static_assert(sizeof(usacElementType_IdNames)/sizeof(const char*)==ID_USAC_Max, "");

//---------------------------------------------------------------------------
enum usacExtElementType_Value
{
    ID_EXT_ELE_FILL,
    ID_EXT_ELE_MPEGS,
    ID_EXT_ELE_SAOC,
    ID_EXT_ELE_AUDIOPREROLL,
    ID_EXT_ELE_UNI_DRC,
    ID_EXT_ELE_Max
};
const char* usacExtElementType_IdNames[]=
{
    "ID_EXT_ELE_FILL",
    "ID_EXT_ELE_MPEGS",
    "ID_EXT_ELE_SAOC",
    "ID_EXT_ELE_AUDIOPREROLL",
    "ID_EXT_ELE_UNI_DRC",
};
static_assert(sizeof(usacExtElementType_IdNames)/sizeof(const char*)==ID_EXT_ELE_Max, "");
const char* usacExtElementType_Names[] =
{
    "Fill",
    "SpatialFrame",
    "SaocFrame",
    "AudioPreRoll",
    "uniDrcGain",
};
static_assert(sizeof(usacExtElementType_Names)/sizeof(const char*)==ID_EXT_ELE_Max, "");
const char* usacExtElementType_ConfigNames[] =
{
    nullptr,
    "SpatialSpecificConfig",
    "SaocSpecificConfig",
    nullptr,
    "uniDrcConfig",
};
static_assert(sizeof(usacExtElementType_ConfigNames)/sizeof(const char*)==ID_EXT_ELE_Max, "");

//---------------------------------------------------------------------------
enum loudnessInfoSetExtType_Value
{
    UNIDRCLOUDEXT_TERM,
    UNIDRCLOUDEXT_EQ,
    UNIDRCLOUDEXT_Max
};
static const char* loudnessInfoSetExtType_IdNames[]=
{
    "UNIDRCLOUDEXT_TERM",
    "UNIDRCLOUDEXT_EQ",
};
static_assert(sizeof(loudnessInfoSetExtType_IdNames)/sizeof(const char*)==UNIDRCLOUDEXT_Max, "");
static const char* loudnessInfoSetExtType_ConfNames[]=
{
    "",
    "V1loudnessInfo",
};
static_assert(sizeof(loudnessInfoSetExtType_ConfNames)/sizeof(const char*)==UNIDRCLOUDEXT_Max, "");

//---------------------------------------------------------------------------
enum uniDrcConfigExtType_Value
{
    UNIDRCCONFEXT_TERM,
    UNIDRCCONFEXT_PARAM_DRC,
    UNIDRCCONFEXT_V1,
    UNIDRCCONFEXT_Max
};
static const char* uniDrcConfigExtType_IdNames[]=
{
    "UNIDRCCONFEXT_TERM",
    "UNIDRCCONFEXT_PARAM_DRC",
    "UNIDRCCONFEXT_V1",
};
static_assert(sizeof(uniDrcConfigExtType_IdNames)/sizeof(const char*)==UNIDRCCONFEXT_Max, "");
static const char* uniDrcConfigExtType_ConfNames[]=
{
    "",
    "ParametricDrc",
    "V1Drc",
};
static_assert(sizeof(uniDrcConfigExtType_ConfNames)/sizeof(const char*)==UNIDRCCONFEXT_Max, "");

//---------------------------------------------------------------------------
enum usacConfigExtType_Value
{
    ID_CONFIG_EXT_FILL,
    ID_CONFIG_EXT_1,
    ID_CONFIG_EXT_LOUDNESS_INFO,
    ID_CONFIG_EXT_3,
    ID_CONFIG_EXT_4,
    ID_CONFIG_EXT_5,
    ID_CONFIG_EXT_6,
    ID_CONFIG_EXT_STREAM_ID,
    ID_CONFIG_EXT_Max
};
static const char* usacConfigExtType_IdNames[]=
{
    "ID_CONFIG_EXT_FILL",
    "",
    "ID_CONFIG_EXT_LOUDNESS_INFO",
    "",
    "",
    "",
    "",
    "ID_CONFIG_EXT_STREAM_ID",
};
static_assert(sizeof(usacConfigExtType_IdNames)/sizeof(const char*)==ID_CONFIG_EXT_Max, "");
static const char* usacConfigExtType_ConfNames[]=
{
    "ConfigExtFill",
    "",
    "loudnessInfoSet",
    "",
    "",
    "",
    "",
    "streamId",
};
static_assert(sizeof(usacConfigExtType_ConfNames)/sizeof(const char*)==ID_CONFIG_EXT_Max, "");

//---------------------------------------------------------------------------
struct coreSbrFrameLengthIndex_mapping
{
    int8u    sbrRatioIndex;
    int16u   coreCoderFrameLength;
    int8u    outputFrameLengthDivided256;
};
const size_t coreSbrFrameLengthIndex_Mapping_Size=5;
coreSbrFrameLengthIndex_mapping coreSbrFrameLengthIndex_Mapping[coreSbrFrameLengthIndex_Mapping_Size]=
{
    { 0, 768,   3 },
    { 0, 1024,  4 },
    { 2, 768,   8 },
    { 3, 1024,  8 },
    { 1, 1024, 16 },
};

//---------------------------------------------------------------------------
static const size_t LoudnessMeaning_Size=9;
static const char* LoudnessMeaning[LoudnessMeaning_Size]=
{
    "Program",
    "Anchor",
    "MaximumOfRange",
    "MaximumMomentary",
    "MaximumShortTerm",
    "Range",
    "ProductionMixingLevel",
    "RoomType",
    "ShortTerm",
};

static Ztring Loudness_Value(int8u methodDefinition, int8u methodValue)
{
    switch (methodDefinition)
    {
        case 0:
                return Ztring();
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
                return Ztring::ToZtring(-57.75+((double)methodValue)/4, 2)+__T(" LKFS");
        case 6: 
                {
                int8u Value;
                if (methodValue==0)
                    Value=0;
                else if (methodValue<=128)
                    Value=methodValue/4;
                else if (methodValue<=204)
                    Value=methodValue/2-32;
                else
                    Value=methodValue-134; 
                return Ztring::ToZtring(Value)+__T(" LU");
                }
        case 7: 
                return Ztring::ToZtring(80+methodValue)+__T(" dB");
        case 8:
                switch(methodValue)
                {
                    case 0: return Ztring();
                    case 1: return __T("Large room");
                    case 2: return __T("Small room");
                    default: return Ztring::ToZtring(methodValue);
                }
        case 9:
                return Ztring::ToZtring(-116+((double)methodValue)/2, 1)+__T(" LKFS");
        default:
                return Ztring().From_Number(methodValue);
    }
}

//---------------------------------------------------------------------------
static const char* measurementSystems[]=
{
    "",
    "EBU R-128",
    "ITU-R BS.1770-4",
    "ITU-R BS.1770-4 with pre-processing",
    "User",
    "Expert/panel",
    "ITU-R BS.1771-1",
    "Reserved Measurement System A",
    "Reserved Measurement System B",
    "Reserved Measurement System C",
    "Reserved Measurement System D",
    "Reserved Measurement System E",
};
auto measurementSystems_Size=sizeof(measurementSystems)/sizeof(const char*);

//---------------------------------------------------------------------------
static const char* reliabilities[]=
{
    "",
    "Unverified",
    "Not to exceed ceiling",
    "Accurate",
};
auto reliabilities_Size=sizeof(reliabilities)/sizeof(const char*);

//---------------------------------------------------------------------------
static const char* drcSetEffect_List[]=
{
    "Night",
    "Noisy",
    "Limited",
    "LowLevel",
    "Dialog",
    "General",
    "Expand",
    "Artistic",
    "Clipping",
    "Fade",
    "DuckOther",
    "DuckSelf",
};
auto drcSetEffect_List_Size=sizeof(drcSetEffect_List)/sizeof(const char*);

//---------------------------------------------------------------------------
#if MEDIAINFO_CONFORMANCE
struct profile_constraints
{
    int8u   MaxChannels;
    int8u   MaxSamplingRate;              // 1 = 24 kHz, 2 = 48 kHz, 3 = 96 kHz
};
static profile_constraints xHEAAC_Constraints[] = // Frame tables in chapter "Extended HE AAC profile" (comments are from table "Levels for the extended HE AAC profile")
{
    {  1, 2},
    {  2, 2},
    {  2, 2},
    {  2, 2}, // {  6, 2},
    {  2, 2}, // {  6, 3},
    {  2, 2}, // {  8, 2},
    {  2, 2}, // {  8, 3},
};
struct measurement_present
{
    int8u                       methodDefinition;
    int8u                       measurementSystem;

    friend bool operator<(const measurement_present& l, const measurement_present& r)
    {
        if (l.methodDefinition < r.methodDefinition)
            return true;
        if (l.methodDefinition != r.methodDefinition)
            return false;
        return l.measurementSystem < r.measurementSystem;
    }
};
#endif


//---------------------------------------------------------------------------
#if MEDIAINFO_CONFORMANCE
static constexpr int8u channelConfiguration_Orders[] =
{
    0, 1, 2, 4, 7, 10, 14, 19, 21, 23, 25, 30, 35, 51, // Offset
    // 1
    ID_USAC_SCE,
    // 2
    ID_USAC_CPE,
    // 3
    ID_USAC_SCE,
    ID_USAC_CPE,
    // 4
    ID_USAC_SCE,
    ID_USAC_CPE,
    ID_USAC_SCE,
    // 5
    ID_USAC_SCE,
    ID_USAC_CPE,
    ID_USAC_CPE,
    // 6
    ID_USAC_SCE,
    ID_USAC_CPE,
    ID_USAC_CPE,
    ID_USAC_LFE,
    // 7
    ID_USAC_SCE,
    ID_USAC_CPE,
    ID_USAC_CPE,
    ID_USAC_CPE,
    ID_USAC_LFE,
    // 8
    ID_USAC_SCE,
    ID_USAC_SCE,
    // 9
    ID_USAC_CPE,
    ID_USAC_SCE,
    // 10
    ID_USAC_CPE,
    ID_USAC_CPE,
    // 11
    ID_USAC_SCE,
    ID_USAC_CPE,
    ID_USAC_CPE,
    ID_USAC_SCE,
    ID_USAC_LFE,
    // 12
    ID_USAC_SCE,
    ID_USAC_CPE,
    ID_USAC_CPE,
    ID_USAC_CPE,
    ID_USAC_LFE,
    // 13
    ID_USAC_SCE,
    ID_USAC_CPE,
    ID_USAC_CPE,
    ID_USAC_CPE,
    ID_USAC_CPE,
    ID_USAC_SCE,
    ID_USAC_LFE,
    ID_USAC_LFE,
    ID_USAC_SCE,
    ID_USAC_CPE,
    ID_USAC_CPE,
    ID_USAC_SCE,
    ID_USAC_CPE,
    ID_USAC_SCE,
    ID_USAC_SCE,
    ID_USAC_CPE,
};
static_assert(sizeof(channelConfiguration_Orders) / sizeof(int8u) == Aac_Channels_Size_Usac + channelConfiguration_Orders[Aac_Channels_Size_Usac - 1], "");
#endif

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Usac::File_Usac()
:File__Analyze()
{
    channelConfiguration=(int8u)-1;
    sampling_frequency_index=(int8u)-1;
    extension_sampling_frequency_index=(int8u)-1;
    IsParsingRaw=0;
    #if MEDIAINFO_CONFORMANCE
        ProfileLevel = { UnknownAudio, 0 };
        Immediate_FramePos=nullptr;
        Immediate_FramePos_IsPresent=nullptr;
        IsCmaf=nullptr;
        outputFrameLength=nullptr;
        FirstOutputtedDecodedSample=nullptr;
        roll_distance_Values=nullptr;
        roll_distance_FramePos=nullptr;
    #endif
}

//---------------------------------------------------------------------------
File_Usac::~File_Usac()
{
}

//***********************************************************************
// Temp
//***********************************************************************

//---------------------------------------------------------------------------
void File_Usac::hcod_sf(const char* Name)
{
    Element_Begin1(Name);
    int16u Pos=0;
    int32u index=0;

    for (;;)
    {
        int8u h;
        Peek_S1(2, h);
        index = huffman_scl[index][h]; /* Expensive memory access */
        if (index & 1)
        {
            if (index & 2)
                Skip_SB(                                        "huffman");
            else
                Skip_S1(2,                                      "huffman");
            break;
        }
        Skip_S1(2,                                              "huffman");
        index >>= 2;

        if (Pos>240)
        {
            Skip_BS(Data_BS_Remain(),                           "Error");
            Element_End0();
            return;
        }
    }
    Element_Info1(huffman_scl[Pos][0]-60);
    Element_End0();
}

//----------------------------------------------------------------------------
int32u File_Usac::arith_decode(int16u& low, int16u& high, int16u& value, const int16u* cf, int32u cfl, size_t* TooMuch)
{
    int32u range=(int32u)(high-low+1), value32=value;
    int32u cm=((((int)(value32-low+1))<<14)-((int)1))/range;
    const int16u* p=cf-1;

     do
     {
         const int16u* q=p+(cfl>>1);
         if (*q>cm)
         {
             p=q;
             cfl++;
         }
         cfl>>=1;
     }
     while (cfl>1);

    int32u symbol=(int32u)(p-cf+1);
    if (symbol)
        high=low+((range*cf[symbol-1])>>14)-1;
    low+=(range*cf[symbol])>>14;

    for (;;)
    {
        if (high&0x8000)
        {
            if (!(low&0x8000))
            {
                 if (low&0x4000 && !(high&0x4000))
                 {
                     low-=0x4000;
                     high-=0x4000;
                     value32-=0x4000;
                 }
                 else
                     break;
            }
        }

        low=low<<1;
        high=(high<<1) | 1;

        value32<<=1;
        if (Data_BS_Remain())
        {
            bool bit;
            Get_SB (bit,                                        "arith_data");
            value32|=(int)bit;
        }
        else
        {
            (*TooMuch)++;
        }
    }

    value=(int16u)value32;
    return symbol;
}

//---------------------------------------------------------------------------
int16u File_Usac::sbr_huff_dec(const int8s(*Table)[2], const char* Name)
{
    int8u bit;
    int8s index=0;

    Element_Begin1(Name);
    while (index>=0)
    {
        Get_S1(1, bit,                                          "bit");
        index=Table[index][bit];
    }
    Element_End0();

    return index+64;
}

//***********************************************************************
// BS_Bookmark
//***********************************************************************

//---------------------------------------------------------------------------
File_Usac::bs_bookmark File_Usac::BS_Bookmark(size_t NewSize)
{
    bs_bookmark B;
    if (Data_BS_Remain()>=NewSize)
        B.End=Data_BS_Remain()-NewSize;
    else
        B.End=Data_BS_Remain();
    B.Element_Offset=Element_Offset;
    B.Trusted=Trusted;
    B.UnTrusted=Element[Element_Level].UnTrusted;
    B.NewSize=NewSize;
    B.BitsNotIncluded=B.End%8;
    if (B.BitsNotIncluded)
        B.NewSize+=B.BitsNotIncluded;
    BS->Resize(B.NewSize);
    #if MEDIAINFO_CONFORMANCE
        for (size_t Level=0; Level<ConformanceLevel_Max; Level++)
            B.ConformanceErrors[Level]=ConformanceErrors[Level];
    #endif
    return B;
}

//---------------------------------------------------------------------------
#if MEDIAINFO_CONFORMANCE
bool File_Usac::BS_Bookmark(File_Usac::bs_bookmark& B, const string& ConformanceFieldName)
#else
bool File_Usac::BS_Bookmark(File_Usac::bs_bookmark& B)
#endif
{
    if (Data_BS_Remain()>B.BitsNotIncluded)
    {
        int8u LastByte;
        auto BitsRemaining=Data_BS_Remain()-B.BitsNotIncluded;
        if (BitsRemaining<8)
        {
            //Peek_S1((int8u)BitsRemaining, LastByte);
            //#if MEDIAINFO_CONFORMANCE
            //    if (LastByte)
            //        Fill_Conformance((ConformanceFieldName+" Coherency").c_str(), "Padding bits are not 0, the bitstream may be malformed", bitset8(), Warning);
            //#endif
            LastByte=0;
        }
        else
        {
            #if MEDIAINFO_CONFORMANCE
                Fill_Conformance((ConformanceFieldName+" Coherency").c_str(), "Extra bytes after the end of the syntax was reached", bitset8(), Warning);
            #endif
            LastByte=1;
        }
        Skip_BS(BitsRemaining,                                  LastByte?"Unknown":"Padding");
    }
    else if (Data_BS_Remain()<B.BitsNotIncluded)
        Trusted_IsNot("Too big");
    bool IsNotValid=!Trusted_Get();
    #if MEDIAINFO_CONFORMANCE
        if (IsNotValid)
        {
            for (size_t Level=0; Level<ConformanceLevel_Max; Level++)
                ConformanceErrors[Level]=B.ConformanceErrors[Level];
            Fill_Conformance((ConformanceFieldName + " Coherency").c_str(), "Bitstream parsing ran out of data to read before the end of the syntax was reached, most probably the bitstream is malformed");
        }
    #endif
    BS->Resize(B.End);
    Element_Offset=B.Element_Offset;
    Trusted=B.Trusted;
    Element[Element_Level].UnTrusted=B.UnTrusted;
    return IsNotValid;
}

//***********************************************************************
// Conformance
//***********************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_CONFORMANCE
void File_Usac::Fill_Conformance(const char* Field, const char* Value, bitset8 Flags, conformance_level Level)
{
    if (strncmp(Field, "UsacConfig loudnessInfoSet", 26) && strncmp(Field, "mpegh3daConfig loudnessInfoSet", 30))
        return; // TODO: remove when all tests are active in production
    if (Level == Warning && Warning_Error)
        Level = Error;
    field_value FieldValue(Field, Value, Flags, (int64u)-1);
    auto& Conformance = ConformanceErrors[Level];
    auto Current = find(Conformance.begin(), Conformance.end(), FieldValue);
    if (Current != Conformance.end())
        return;
    Conformance.emplace_back(FieldValue);
}
#endif

//---------------------------------------------------------------------------
#if MEDIAINFO_CONFORMANCE
void File_Usac::Clear_Conformance()
{
    for (size_t Level = 0; Level < ConformanceLevel_Max; Level++)
    {
        auto& Conformance = ConformanceErrors[Level];
        Conformance.clear();
    }
}
#endif

//---------------------------------------------------------------------------
#if MEDIAINFO_CONFORMANCE
void File_Usac::Merge_Conformance(bool FromConfig)
{
    for (size_t Level = 0; Level < ConformanceLevel_Max; Level++)
    {
        auto& Conformance = ConformanceErrors[Level];
        auto& Conformance_Total = ConformanceErrors_Total[Level];
        for (const auto& FieldValue : Conformance)
        {
            auto Current = find(Conformance_Total.begin(), Conformance_Total.end(), FieldValue);
            if (Current != Conformance_Total.end())
            {
                if (Current->FramePoss.size() < 8)
                {
                    if (FromConfig)
                    {
                        if (Current->FramePoss.empty() || Current->FramePoss[0] != (int64u)-1)
                            Current->FramePoss.insert(Current->FramePoss.begin(), (int64u)-1);
                    }
                    else
                        Current->FramePoss.push_back(Frame_Count_NotParsedIncluded);
                }
                else if (Current->FramePoss.size() == 8)
                    Current->FramePoss.push_back((int64u)-1); //Indicating "..."
                continue;
            }
            if (!CheckIf(FieldValue.Flags))
                continue;
            Conformance_Total.push_back(FieldValue);
            if (!FromConfig)
                Conformance_Total.back().FramePoss.front() = Frame_Count_NotParsedIncluded;
        }
        Conformance.clear();
    }
}
#endif

//---------------------------------------------------------------------------
#if MEDIAINFO_CONFORMANCE
const profilelevel_struct& Mpeg4_Descriptors_ToProfileLevel(int8u AudioProfileLevelIndication);
void File_Usac::SetProfileLevel(int8u AudioProfileLevelIndication)
{
    ProfileLevel = Mpeg4_Descriptors_ToProfileLevel(AudioProfileLevelIndication);
    switch (ProfileLevel.profile)
    {
        case Baseline_USAC                    : ConformanceFlags.set(BaselineUsac); break;
        case Extended_HE_AAC                  : ConformanceFlags.set(xHEAAC); break;
        default:;
    }
}
#endif

//---------------------------------------------------------------------------
#if MEDIAINFO_CONFORMANCE
void File_Usac::Streams_Finish_Conformance_Profile(usac_config& CurrentConf)
{
    if (ProfileLevel.profile == UnknownAudio)
    {
        auto AudioProfileLevelIndication = MediaInfoLib::Config.UsacProfile();
        if (!AudioProfileLevelIndication)
            SetProfileLevel(AudioProfileLevelIndication);
        else if (!IsSub)
            ConformanceFlags.set(xHEAAC); // TODO: remove this check (initially done for LATM without the profile option)
    }

    if (ConformanceFlags[xHEAAC] && ProfileLevel.profile == Extended_HE_AAC && ProfileLevel.level > 1 && ProfileLevel.level <= 5)
    {
        if (CurrentConf.sampling_frequency && xHEAAC_Constraints[ProfileLevel.level].MaxSamplingRate)
        {
            int32u MaxSamplingRate = 24000 << (xHEAAC_Constraints[ProfileLevel.level].MaxSamplingRate - 1);
            if (CurrentConf.sampling_frequency > MaxSamplingRate)
                Fill_Conformance("Crosscheck InitialObjectDescriptor+UsacConfig usacSamplingFrequency", ("InitialObjectDescriptor audioProfileLevelIndication " + Mpeg4_Descriptors_AudioProfileLevelString(ProfileLevel) + " does not permit UsacConfig usacSamplingFrequency " + to_string(CurrentConf.sampling_frequency) + ", max is " + to_string(MaxSamplingRate)).c_str());
        }
        else if (CurrentConf.sampling_frequency > 48000)
            Fill_Conformance("Crosscheck InitialObjectDescriptor+UsacConfig usacSamplingFrequency", ("InitialObjectDescriptor audioProfileLevelIndication " + Mpeg4_Descriptors_AudioProfileLevelString(ProfileLevel) + " does not permit UsacConfig usacSamplingFrequency " + to_string(CurrentConf.sampling_frequency) + ", max is 48000").c_str());
        else if (CurrentConf.sampling_frequency_index < Aac_sampling_frequency_Size && Aac_sampling_frequency[CurrentConf.sampling_frequency_index] == CurrentConf.sampling_frequency
         && ((CurrentConf.sampling_frequency_index < 0x03 || CurrentConf.sampling_frequency_index > 0x0C) && (CurrentConf.sampling_frequency_index < 0x11 || CurrentConf.sampling_frequency_index > 0x1B)))
            Fill_Conformance("Crosscheck InitialObjectDescriptor+UsacConfig usacSamplingFrequencyIndex", ("InitialObjectDescriptor audioProfileLevelIndication " + Mpeg4_Descriptors_AudioProfileLevelString(ProfileLevel) + " does not permit UsacConfig usacSamplingFrequencyIndex " + to_string(CurrentConf.sampling_frequency_index)).c_str());
        if (!CurrentConf.channelConfiguration)
        {
            if (CurrentConf.numOutChannels && CurrentConf.numOutChannels > xHEAAC_Constraints[ProfileLevel.level].MaxChannels)
                Fill_Conformance("Crosscheck InitialObjectDescriptor+UsacConfig numOutChannels", ("InitialObjectDescriptor audioProfileLevelIndication " + Mpeg4_Descriptors_AudioProfileLevelString(ProfileLevel) + " does not permit UsacConfig numOutChannels " + to_string(CurrentConf.numOutChannels) + ", max is " + to_string(xHEAAC_Constraints[ProfileLevel.level].MaxChannels)).c_str());
        }
        else
        {
            switch (CurrentConf.channelConfiguration)
            {
                case 1 :
                case 2 :
                case 8 :
                        break;
                default:
                    Fill_Conformance("Crosscheck InitialObjectDescriptor+UsacConfig channelConfiguration", ("InitialObjectDescriptor audioProfileLevelIndication " + Mpeg4_Descriptors_AudioProfileLevelString(ProfileLevel) + " does not permit UsacConfig channelConfigurationIndex " + to_string(CurrentConf.channelConfiguration)).c_str());
            }
        }
    }
    if (IsCmaf && *IsCmaf && CurrentConf.channelConfiguration != 1 && CurrentConf.channelConfiguration != 2)
        Fill_Conformance("Crosscheck CMAF+UsacConfig channelConfiguration", ("CMAF does not permit channelConfigurationIndex " + to_string(CurrentConf.channelConfiguration) + ", permitted values are 1 and 2").c_str());
}
void File_Usac::Streams_Finish_Conformance()
{
    Streams_Finish_Conformance_Profile(Conf);
    Merge_Conformance(true);

    for (size_t Level = 0; Level < ConformanceLevel_Max; Level++)
    {
        auto& Conformance_Total = ConformanceErrors_Total[Level];
        if (Conformance_Total.empty())
            continue;
        for (size_t i = Conformance_Total.size() - 1; i < Conformance_Total.size(); i--) {
            if (!CheckIf(Conformance_Total[i].Flags)) {
                Conformance_Total.erase(Conformance_Total.begin() + i);
            }
        }
        if (Conformance_Total.empty())
            continue;
        string Conformance_String = "Conformance";
        Conformance_String += ConformanceLevel_Strings[Level];
        Fill(Stream_Audio, 0, Conformance_String.c_str(), Conformance_Total.size());
        Conformance_String += ' ';
        for (const auto& ConformanceError : Conformance_Total) {
            size_t Space = 0;
            for (;;) {
                Space = ConformanceError.Field.find(' ', Space + 1);
                if (Space == string::npos) {
                    break;
                }
                const auto Field = Conformance_String + ConformanceError.Field.substr(0, Space);
                const auto& Value = Retrieve_Const(StreamKind_Last, StreamPos_Last, Field.c_str());
                if (Value.empty()) {
                    Fill(StreamKind_Last, StreamPos_Last, Field.c_str(), "Yes");
                }
            }
            auto Value = ConformanceError.Value;
            if (!ConformanceError.FramePoss.empty() && (ConformanceError.FramePoss.size() != 1 || ConformanceError.FramePoss[0] != (int64u)-1))
            {
                auto HasConfError = ConformanceError.FramePoss[0] == (int64u)-1;
                Value += " (";
                if (HasConfError)
                    Value += "conf & ";
                Value += "frame";
                if (ConformanceError.FramePoss.size() - HasConfError > 1)
                    Value += 's';
                Value += ' ';
                for (size_t i = HasConfError; i < ConformanceError.FramePoss.size(); i++)
                {
                    auto FramePos = ConformanceError.FramePoss[i];
                    if (FramePos == (int64u)-1)
                        Value += "...";
                    else
                        Value += to_string(FramePos);
                    Value += '+';
                }
                Value.back() = ')';
            }
            Fill(Stream_Audio, 0, (Conformance_String + ConformanceError.Field).c_str(), Value);
        }
        Conformance_Total.clear();
    }
}
#endif

//---------------------------------------------------------------------------
#if MEDIAINFO_CONFORMANCE
void File_Usac::numPreRollFrames_Check(usac_config& CurrentConf, int32u numPreRollFrames, const string numPreRollFramesConchString)
{
    string FieldName = numPreRollFramesConchString.substr(numPreRollFramesConchString.rfind(' ') + 1);
    int numPreRollFrames_Max;
    if (CurrentConf.coreSbrFrameLengthIndex >= coreSbrFrameLengthIndex_Mapping_Size || coreSbrFrameLengthIndex_Mapping[CurrentConf.coreSbrFrameLengthIndex].sbrRatioIndex)
    {
        if (CurrentConf.harmonicSBR)
            numPreRollFrames_Max = 3;
        else
            numPreRollFrames_Max = 2;
    }
    else
        numPreRollFrames_Max = 1;
    if (numPreRollFrames != numPreRollFrames_Max)
    {
        auto Value = FieldName + " is " + to_string(numPreRollFrames) + " but ";
        if (numPreRollFrames > numPreRollFrames_Max)
            Value += "<= ";
        Value += to_string(numPreRollFrames_Max) + " is ";
        if (numPreRollFrames > numPreRollFrames_Max)
            Value += "required";
        else
            Value += "recommended";
        if (CurrentConf.coreSbrFrameLengthIndex >= coreSbrFrameLengthIndex_Mapping_Size || coreSbrFrameLengthIndex_Mapping[CurrentConf.coreSbrFrameLengthIndex].sbrRatioIndex)
        {
            if (CurrentConf.harmonicSBR)
            {
                if (numPreRollFrames < numPreRollFrames_Max)
                    Value += " due to SBR with harmonic patching";
            }
            else
                Value += " due to SBR without harmonic patching";
        }
        else
            Value += " due to no SBR";
        Fill_Conformance(numPreRollFramesConchString.c_str(), Value, bitset8(), numPreRollFrames > numPreRollFrames_Max ? Error : Warning);
    }
}
#endif

//***************************************************************************
// Elements - USAC - Config
//***************************************************************************

//---------------------------------------------------------------------------
void File_Usac::UsacConfig(size_t BitsNotIncluded)
{
    // Init
    C = usac_config();
    #if MEDIAINFO_CONFORMANCE
        Warning_Error=MediaInfoLib::Config.WarningError();
        C.Reset();
    #endif

    Element_Begin1("UsacConfig");
    bool usacConfigExtensionPresent;
    Get_S1 (5, C.sampling_frequency_index,                      "usacSamplingFrequencyIndex"); Param_Info1C(C.sampling_frequency_index<Aac_sampling_frequency_Size_Usac && Aac_sampling_frequency[C.sampling_frequency_index], Aac_sampling_frequency[C.sampling_frequency_index]);
    if (C.sampling_frequency_index==0x1F)
    {
        int32u samplingFrequency;
        Get_S3 (24, samplingFrequency,                          "usacSamplingFrequency");
        C.sampling_frequency_index=Aac_AudioSpecificConfig_sampling_frequency_index(samplingFrequency);
        C.sampling_frequency=samplingFrequency;
        #if MEDIAINFO_CONFORMANCE
            if (C.sampling_frequency == Aac_sampling_frequency[C.sampling_frequency_index]) {
                Fill_Conformance("UsacConfig usacSamplingFrequencyIndex", ("usacSamplingFrequency is used but usacSamplingFrequencyIndex " + to_string(C.sampling_frequency_index) + " could be used instead").c_str(), bitset8(), Warning);
            }
        #endif
    }
    else
    {
        C.sampling_frequency=Aac_sampling_frequency[C.sampling_frequency_index];
        #if MEDIAINFO_CONFORMANCE
            if (!C.sampling_frequency) {
                Fill_Conformance("UsacConfig usacSamplingFrequencyIndex", ("Value " + to_string(C.sampling_frequency_index) + " is known as reserved in ISO/IEC 23003-3:2020, bitstream parsing is partial and may be wrong").c_str(), bitset8(), Info);
                if (Frequency_b)
                {
                    for (size_t i = 0; i < Aac_sampling_frequency_Size_Usac; i++)
                        if (Aac_sampling_frequency[i] == Frequency_b)
                            Fill_Conformance("Crosscheck AudioSpecificConfig+UsacConfig samplingFrequency+usacSamplingFrequency", ("AudioSpecificConfig samplingFrequency " + to_string(Frequency_b) + " does not match UsacConfig usacSamplingFrequencyIndex " + to_string(C.sampling_frequency_index)).c_str());
                }
            }
        #endif
    }
    #if MEDIAINFO_CONFORMANCE
        if (Frequency_b && C.sampling_frequency && C.sampling_frequency != Frequency_b)
            Fill_Conformance("Crosscheck AudioSpecificConfig+UsacConfig samplingFrequency+usacSamplingFrequency", ("AudioSpecificConfig samplingFrequency " + to_string(Frequency_b) + " does not match UsacConfig usacSamplingFrequency " + to_string(C.sampling_frequency)).c_str());
    #endif
    Get_S1 (3, C.coreSbrFrameLengthIndex,                       "coreSbrFrameLengthIndex");
    Get_S1 (5, C.channelConfiguration,                          "channelConfiguration"); Param_Info1C(C.channelConfiguration, Aac_ChannelLayout_GetString(C.channelConfiguration));
    #if MEDIAINFO_CONFORMANCE
        if (channelConfiguration && C.channelConfiguration && C.channelConfiguration != channelConfiguration)
            Fill_Conformance("Crosscheck AudioSpecificConfig+UsacConfig channelConfiguration", ("AudioSpecificConfig channelConfiguration " + to_string(channelConfiguration) + Aac_ChannelLayout_GetString(channelConfiguration, false, true) + " does not match UsacConfig channelConfigurationIndex " + to_string(C.channelConfiguration) + Aac_ChannelLayout_GetString(C.channelConfiguration, false, true)).c_str());
    #endif
    if (!C.channelConfiguration)
    {
        escapedValue(C.numOutChannels, 5, 8, 16,                "numOutChannels");
        #if MEDIAINFO_CONFORMANCE
            C.numOutChannels_Lfe.clear();
            string ExpectedOrder;
            if (channelConfiguration)
                ExpectedOrder = Aac_ChannelLayout_GetString(channelConfiguration);
            string ActualOrder;
            set<int32u> bsOutChannelPos_List;
        #endif
        for (int32u i=0; i<C.numOutChannels; i++)
        {
            int8u bsOutputChannelPos;
            Get_S1 (5, bsOutputChannelPos,                         "bsOutputChannelPos"); Param_Info1(Aac_OutputChannelPosition_GetString(bsOutputChannelPos));
            #if MEDIAINFO_CONFORMANCE
                if (bsOutChannelPos_List.find(bsOutputChannelPos) != bsOutChannelPos_List.end())
                {
                    string Value = Aac_OutputChannelPosition_GetString(bsOutputChannelPos);
                    if (Value.empty())
                        Value = to_string(bsOutputChannelPos);
                    else
                        Value = to_string(bsOutputChannelPos) + " (" + Value + ')';
                    Fill_Conformance("bsOutChannelPos Coherency", ("bsOutChannelPos " + Value + " is present 2 times but only 1 instance is allowed").c_str());
                }
                else
                    bsOutChannelPos_List.insert(bsOutputChannelPos);
                size_t ActualOrder_Previous = ActualOrder.size();
                ActualOrder += Aac_OutputChannelPosition_GetString(bsOutputChannelPos);
                if (ActualOrder.find("LFE", ActualOrder_Previous) != string::npos)
                    C.numOutChannels_Lfe.push_back(i);
                ActualOrder += ' ';
            #endif
        }
        #if MEDIAINFO_CONFORMANCE
            if (!C.numOutChannels)
                Fill_Conformance("numOutChannels Coherency", "numOutChannels is 0");
            else if (!ExpectedOrder.empty())
            {
                ActualOrder.pop_back();
                if (ExpectedOrder != ActualOrder)
                    Fill_Conformance("Crosscheck AudioSpecificConfig+UsacConfig channelConfiguration", ("AudioSpecificConfig channelConfiguration " + to_string(channelConfiguration) + " (" + ExpectedOrder + ") does not match UsacConfig channel mapping " + ActualOrder).c_str());
            }
            if (!ActualOrder.empty())
            {
                for (size_t i = 1; i < Aac_Channels_Size_Usac; i++)
                {
                    string PossibleOrder = Aac_ChannelLayout_GetString(i);
                    if (PossibleOrder == ActualOrder)
                        Fill_Conformance("UsacConfig channelConfiguration", ("channelConfiguration is 0 but channelConfiguration " + to_string(i) + " could be used for channel mapping " + ActualOrder).c_str(), bitset8(), Warning);
                }
            }
        #endif
    }
    else if (C.channelConfiguration<Aac_Channels_Size_Usac)
        C.numOutChannels=Aac_Channels[C.channelConfiguration];
    else
    {
        C.numOutChannels=0;
        #if MEDIAINFO_CONFORMANCE
            Fill_Conformance("UsacConfig channelConfiguration", ("Value " + to_string(C.channelConfiguration) + " is known as reserved in ISO/IEC 23003-3:2020, bitstream parsing is partial and may be wrong").c_str(), bitset8(), Info);
        #endif
    }
    channelConfiguration=C.channelConfiguration;
    #if MEDIAINFO_CONFORMANCE
        Streams_Finish_Conformance_Profile(C);
        if (C.coreSbrFrameLengthIndex >= coreSbrFrameLengthIndex_Mapping_Size)
            Fill_Conformance("UsacConfig coreSbrFrameLengthIndex", ("Value " + to_string(C.coreSbrFrameLengthIndex) + " is known as reserved in ISO/IEC 23003-3:2020, bitstream parsing is partial and may be wrong").c_str(), bitset8(), Info);
    #endif
    UsacDecoderConfig();
    Get_SB (usacConfigExtensionPresent,                         "usacConfigExtensionPresent");
    if (usacConfigExtensionPresent)
        UsacConfigExtension();
    Element_End0();

    if (BitsNotIncluded!=(size_t)-1)
    {
        if (Data_BS_Remain()>BitsNotIncluded)
        {
            int8u LastByte;
            auto BitsRemaining=Data_BS_Remain()-BitsNotIncluded;
            if (BitsRemaining<8)
            {
                //Peek_S1((int8u)BitsRemaining, LastByte);
                //#if MEDIAINFO_CONFORMANCE
                //    if (LastByte)
                //        Fill_Conformance((ConformanceFieldName+" Coherency").c_str(), "Padding bits are not 0, the bitstream may be malformed", bitset8(), Warning);
                //#endif
                LastByte=0;
            }
            else
            {
                #if MEDIAINFO_CONFORMANCE
                    Fill_Conformance("UsacConfig Coherency", "Extra bytes after the end of the syntax was reached", bitset8(), Warning);
                #endif
                LastByte=1;
            }
            Skip_BS(BitsRemaining,                              LastByte?"Unknown":"Padding");
        }
        else if (Data_BS_Remain()<BitsNotIncluded)
            Trusted_IsNot("Too big");
    }

    if (Trusted_Get())
    {
        // Filling
        C.IsNotValid=false;
        if (!IsParsingRaw)
        {
            if (C.coreSbrFrameLengthIndex<coreSbrFrameLengthIndex_Mapping_Size)
                Fill(Stream_Audio, 0, Audio_SamplesPerFrame, coreSbrFrameLengthIndex_Mapping[C.coreSbrFrameLengthIndex].outputFrameLengthDivided256<<8, 10, true);
            Fill_DRC();
            Fill_Loudness();
            #if MEDIAINFO_CONFORMANCE
                Merge_Conformance(true);
            #endif
            Conf=C;
        }
    }
    else
    {
        if (!IsParsingRaw)
            Fill_Loudness(); // TODO: remove when all tests are active in production

        #if MEDIAINFO_CONFORMANCE
            if (!IsParsingRaw)
            {
                Clear_Conformance();
                Fill_Conformance("UsacConfig Coherency", "Bitstream parsing ran out of data to read before the end of the syntax was reached, most probably the bitstream is malformed.");
                Merge_Conformance();
            }
        #endif

        C.IsNotValid=true;
        if (!IsParsingRaw)
            Conf.IsNotValid=true;
    }
    #if !MEDIAINFO_CONFORMANCE && MEDIAINFO_TRACE
        if (!Trace_Activated)
            Finish();
    #endif
}

//---------------------------------------------------------------------------
void File_Usac::Fill_DRC(const char* Prefix)
{
    if (!C.drcInstructionsUniDrc_Data.empty())
    {
        string FieldPrefix;
        if (Prefix)
        {
            FieldPrefix+=Prefix;
            FieldPrefix += ' ';
        }

        Fill(Stream_Audio, 0, (FieldPrefix+"DrcSets_Count").c_str(), C.drcInstructionsUniDrc_Data.size());
        Fill_SetOptions(Stream_Audio, 0, (FieldPrefix + "DrcSets_Count").c_str(), "N NI"); // Hidden in text output
        ZtringList Ids, Data;
        for (const auto& Item : C.drcInstructionsUniDrc_Data)
        {
            int8u drcSetId=Item.first.drcSetId;
            int8u downmixId=Item.first.downmixId;
            Ztring Id;
            if (drcSetId || downmixId)
                Id=Ztring::ToZtring(drcSetId)+=__T('-')+Ztring::ToZtring(downmixId);
            Ids.push_back(Id);
            Data.push_back(Ztring().From_UTF8(Item.second.drcSetEffectTotal));
        }
        Fill(Stream_Audio, 0, (FieldPrefix+"DrcSets_Effects").c_str(), Data, Ids);
    }
}

//---------------------------------------------------------------------------
void File_Usac::Fill_Loudness(const char* Prefix, bool NoConCh)
{
    string FieldPrefix;
    if (Prefix)
    {
        FieldPrefix += Prefix;
        FieldPrefix += ' ';
    }
    string FieldSuffix;

    bool DefaultIdPresent = false;
    for (int8u i = 0; i < 2; i++)
    {
        if (i)
            FieldSuffix = "_Album";
        if (!C.loudnessInfo_Data[i].empty())
        {
            Fill(Stream_Audio, 0, (FieldPrefix + "Loudness_Count" + FieldSuffix).c_str(), C.loudnessInfo_Data[i].size());
            Fill_SetOptions(Stream_Audio, 0, (FieldPrefix + "Loudness_Count" + FieldSuffix).c_str(), "N NI"); // Hidden in text output
        }
        vector<drc_id> Ids;
        ZtringList SamplePeakLevel;
        ZtringList TruePeakLevel;
        ZtringList Measurements[16];
        for (const auto& Item : C.loudnessInfo_Data[i])
        {
            Ids.push_back(Item.first);
            SamplePeakLevel.push_back(Item.second.SamplePeakLevel);
            TruePeakLevel.push_back(Item.second.TruePeakLevel);
            for (int8u j = 1; j < 16; j++)
                Measurements[j].push_back(Item.second.Measurements.Values[j]);
        }
        if (Ids.size() >= 1 && Ids.front().empty())
        {
            if (!i)
                DefaultIdPresent = true;
        }
        ZtringList Ids_Ztring;
        for (const auto& Id : Ids)
            Ids_Ztring.emplace_back(Ztring().From_UTF8(Id.to_string()));
        if (Ids_Ztring.empty())
            continue;
        Fill(Stream_Audio, 0, (FieldPrefix + "SamplePeakLevel" + FieldSuffix).c_str(), SamplePeakLevel, Ids_Ztring);
        Fill(Stream_Audio, 0, (FieldPrefix + "TruePeakLevel" + FieldSuffix).c_str(), TruePeakLevel, Ids_Ztring);
        for (int8u j = 0; j < 16; j++)
        {
            string Field;
            if (j && j <= LoudnessMeaning_Size)
                Field = LoudnessMeaning[j - 1];
            else
                Field = Ztring::ToZtring(j).To_UTF8();
            Fill(Stream_Audio, 0, (FieldPrefix + "Loudness_" + Field + FieldSuffix).c_str(), Measurements[j], Ids_Ztring);
        }
    }

    #if MEDIAINFO_CONFORMANCE
        if (NoConCh || C.IsNotValid)
            return;
        auto loudnessInfoSet_Present_Total=C.loudnessInfoSet_Present[0]+C.loudnessInfoSet_Present[1];
        if (C.loudnessInfoSet_HasContent[0] && C.loudnessInfoSet_HasContent[1])
            Fill_Conformance("loudnessInfoSet Coherency", "Mix of v0 and v1");
        if (C.loudnessInfoSet_Present[0]>1)
            Fill_Conformance("loudnessInfoSet Coherency", "loudnessInfoSet is present " + to_string(C.loudnessInfoSet_Present[0]) + " times but only 1 instance is allowed");
        constexpr14 auto CheckFlags = bitset8().set(xHEAAC).set(MpegH);
        if (false)
        {
        }
        else if (!loudnessInfoSet_Present_Total)
        {
            Fill_Conformance((string(C.usacElements.empty() ? "mpegh3daConfig " : "UsacConfig ") + "loudnessInfoSet Coherency").c_str(), "loudnessInfoSet is missing", CheckFlags);
            if (ConformanceFlags & CheckFlags)
            {
                Fill(Stream_Audio, 0, (FieldPrefix + "ConformanceCheck").c_str(), "Invalid: loudnessInfoSet is missing");
                Fill(Stream_Audio, 0, "ConformanceCheck/Short", "Invalid: loudnessInfoSet missing");
            }
        }
        else if (C.loudnessInfo_Data[0].empty())
        {
            Fill_Conformance((string(C.usacElements.empty() ? "mpegh3daConfig " : "UsacConfig ") + "loudnessInfoSet loudnessInfoCount").c_str(), "Is 0", CheckFlags);
            if (ConformanceFlags & CheckFlags)
            {
                Fill(Stream_Audio, 0, (FieldPrefix + "ConformanceCheck").c_str(), "Invalid: loudnessInfoSet is empty");
                Fill(Stream_Audio, 0, "ConformanceCheck/Short", "Invalid: loudnessInfoSet empty");
            }
        }
        else if (!DefaultIdPresent)
        {
            Fill_Conformance((string(C.usacElements.empty() ? "mpegh3daConfig " : "UsacConfig ") + "loudnessInfoSet Coherency").c_str(), "Default loudnessInfo is missing", CheckFlags);
            if (ConformanceFlags & CheckFlags)
            {
                Fill(Stream_Audio, 0, (FieldPrefix + "ConformanceCheck").c_str(), "Invalid: Default loudnessInfo is missing");
                Fill(Stream_Audio, 0, "ConformanceCheck/Short", "Invalid: Default loudnessInfo missing");
            }
        }
        else if (C.loudnessInfo_Data[0].begin()->second.Measurements.Values[1].empty() && C.loudnessInfo_Data[0].begin()->second.Measurements.Values[2].empty()) // TODO: add !C.LoudnessInfoIsNotValid check when all tests are active in production
        {
            Fill_Conformance((string(C.usacElements.empty() ? "mpegh3daConfig " : "UsacConfig ") + "loudnessInfoSet Coherency").c_str(), "None of program loudness or anchor loudness is present in default loudnessInfo", CheckFlags);
            if (ConformanceFlags & CheckFlags)
            {
                Fill(Stream_Audio, 0, (FieldPrefix + "ConformanceCheck").c_str(), "Invalid: None of program loudness or anchor loudness is present in default loudnessInfo");
                Fill(Stream_Audio, 0, "ConformanceCheck/Short", "Invalid: Default loudnessInfo incomplete");
            }
        }
        if (!Retrieve_Const(Stream_Audio, 0, "ConformanceCheck/Short").empty())
        {
            Fill_SetOptions(Stream_Audio, 0, "ConformanceCheck", "N NT"); // Hidden in text output (deprecated)
            Fill_SetOptions(Stream_Audio, 0, "ConformanceCheck/Short", "N NT"); // Hidden in text output
        }
    #endif
}

//---------------------------------------------------------------------------
void File_Usac::UsacDecoderConfig()
{
    #if MEDIAINFO_CONFORMANCE
        usacExtElementType_Present.clear();
        int8u channelConfiguration_Orders_Pos;
        int8u channelConfiguration_Orders_Max;
        int8u CheckChannelConfiguration = (int8u)(C.channelConfiguration && C.channelConfiguration < Aac_Channels_Size_Usac);
        if (CheckChannelConfiguration)
        {
            channelConfiguration_Orders_Pos = channelConfiguration_Orders[C.channelConfiguration - 1];
            channelConfiguration_Orders_Max = channelConfiguration_Orders[C.channelConfiguration];
        }
    #endif

    Element_Begin1("UsacDecoderConfig");
    int32u numElements;
    escapedValue(numElements, 4, 8, 16,                         "numElements minus 1");

    for (int32u elemIdx=0; elemIdx<=numElements; elemIdx++)
    {
        Element_Begin1("usacElement");
        int8u usacElementType;
        Get_S1(2, usacElementType,                              "usacElementType"); Element_Info1(usacElementType_IdNames[usacElementType]);
        #if MEDIAINFO_CONFORMANCE
            if (CheckChannelConfiguration == 1 && usacElementType != ID_USAC_EXT)
            {
                if (channelConfiguration_Orders_Pos == channelConfiguration_Orders_Max || usacElementType != channelConfiguration_Orders[channelConfiguration_Orders_Pos])
                    CheckChannelConfiguration = 2;
                else
                    channelConfiguration_Orders_Pos++;
            }
        #endif
        C.usacElements.push_back({usacElementType, 0});
        switch (usacElementType)
        {
            case ID_USAC_SCE                                    : UsacSingleChannelElementConfig(); break;
            case ID_USAC_CPE                                    : UsacChannelPairElementConfig(); break;
            case ID_USAC_LFE                                    : UsacLfeElementConfig(); break;
            case ID_USAC_EXT                                    : UsacExtElementConfig(); break;
        }
        Element_End0();
    }
    #if MEDIAINFO_CONFORMANCE
        if (!C.channelConfiguration)
        {
            size_t ChannelCount_NonLfe=0;
            vector<size_t> Channels_Lfe;
            bool AccrossCpe = false;
            for (size_t i = 0 ; i < C.usacElements.size(); i++)
            {
                auto usacElement = C.usacElements[i];
                switch (usacElement.usacElementType)
                {
                    case ID_USAC_SCE                          : ChannelCount_NonLfe++; break;
                    case ID_USAC_CPE                          : ChannelCount_NonLfe+=2; break;
                    case ID_USAC_LFE                          : Channels_Lfe.push_back(ChannelCount_NonLfe + Channels_Lfe.size()); break;
                }
                if (usacElement.usacElementType == ID_USAC_CPE && C.numOutChannels == ChannelCount_NonLfe + Channels_Lfe.size() - 1)
                    AccrossCpe = true;
            }
            auto ChannelCount = ChannelCount_NonLfe + Channels_Lfe.size();
            if (ChannelCount < C.numOutChannels)
                Fill_Conformance("UsacConfig numOutChannels", ("numOutChannels is " + to_string(C.numOutChannels) + " but the bitstream contains " + to_string(ChannelCount_NonLfe) + " channels").c_str());
            if (ChannelCount > C.numOutChannels)
            {
                if (AccrossCpe)
                    Fill_Conformance("UsacConfig numOutChannels", ("numOutChannels is " + to_string(C.numOutChannels) + ", it is not recommended that the bitstream contains " + to_string(ChannelCount_NonLfe) + " channels, especially when only one channel of a CPE is included in numOutChannels").c_str(), bitset8(), Warning);
                else
                    Fill_Conformance("UsacConfig numOutChannels", ("numOutChannels is " + to_string(C.numOutChannels) + ", it is not recommended that the bitstream contains " + to_string(ChannelCount_NonLfe) + " channels").c_str(), bitset8(), Warning);
            }
            if (ChannelCount == C.numOutChannels)
            {
                auto C_ChannelCount_NonLfe = C.numOutChannels - C.numOutChannels_Lfe.size();
                if (ChannelCount_NonLfe != C_ChannelCount_NonLfe && Channels_Lfe.size() == C.numOutChannels_Lfe.size())
                    Fill_Conformance("UsacConfig numOutChannels", ("numOutChannels minus LFE channel count is " + to_string(C_ChannelCount_NonLfe) + " but the bitstream contains " + to_string(ChannelCount_NonLfe) + " non LFE channels").c_str());
                if (Channels_Lfe.size() != C.numOutChannels_Lfe.size())
                    Fill_Conformance("UsacConfig numOutChannels", ("non LFE channel count is " + to_string(C_ChannelCount_NonLfe) + " and LFE channel count is " + to_string(C.numOutChannels_Lfe.size()) + " but the bitstream contains " + to_string(ChannelCount_NonLfe) + " non LFE channels and " + to_string(Channels_Lfe.size()) + " LFE channels").c_str());
            }
            if (Channels_Lfe.size() > C.numOutChannels_Lfe.size())
                Channels_Lfe.resize(C.numOutChannels_Lfe.size());
            if (C.numOutChannels_Lfe.size() > Channels_Lfe.size())
                C.numOutChannels_Lfe.resize(Channels_Lfe.size());
            for (size_t i = C.numOutChannels_Lfe.size() - 1; (int)i >= 0; i--)
            {
                auto Lfe = C.numOutChannels_Lfe[i];
                for (size_t j = Channels_Lfe.size() - 1; (int)j >= 0; j--)
                    if (Channels_Lfe[j] == Lfe)
                    {
                        C.numOutChannels_Lfe.erase(C.numOutChannels_Lfe.begin() + j);
                        Channels_Lfe.erase(Channels_Lfe.begin() + i);
                    }
            }
            for (size_t i = C.numOutChannels_Lfe.size() - 1; (int)i >= 0; i--)
                Fill_Conformance("UsacConfig numOutChannels", ("LFE channel is expected at position " + to_string(C.numOutChannels_Lfe[i]) + " but the bitstream contains a LFE channel at position " + to_string(Channels_Lfe[i])).c_str());
        }
        if (C.channelConfiguration >= Aac_Channels_Size_Usac)
        {
            channelConfiguration_Orders_Max = 0;
            for (int8u i = 0; i < Aac_Channels_Size_Usac; i++)
            {
                auto IsNotMatch = false;
                channelConfiguration_Orders_Pos = channelConfiguration_Orders_Max;
                channelConfiguration_Orders_Max = channelConfiguration_Orders[i];
                auto channelConfiguration_Orders_Base = channelConfiguration_Orders + Aac_Channels_Size_Usac;
                for (auto usacElement : C.usacElements)
                {
                    if (usacElement.usacElementType >= ID_USAC_EXT)
                        continue;
                    if (channelConfiguration_Orders_Pos == channelConfiguration_Orders_Max || usacElement.usacElementType != channelConfiguration_Orders_Base[channelConfiguration_Orders_Pos])
                        IsNotMatch = true;
                    else
                        channelConfiguration_Orders_Pos++;
                }
                if (!IsNotMatch)
                {
                    string ActualOrder;
                    for (auto usacElement : C.usacElements)
                    {
                        if (usacElement.usacElementType >= ID_USAC_EXT)
                            continue;
                        ActualOrder += usacElementType_IdNames[usacElement.usacElementType];
                        ActualOrder += ' ';
                    }
                    ActualOrder.pop_back();
                    Fill_Conformance("UsacConfig channelConfiguration", ("channelConfigurationIndex " + to_string(C.channelConfiguration) + " is used but the bitstream contains " + ActualOrder + ", which is the configuration indicated by channelConfigurationIndex " + to_string(i)).c_str(), bitset8(), Warning);
                    break;
                }
            }
        }
        else if (CheckChannelConfiguration == 2)
        {
            channelConfiguration_Orders_Pos = channelConfiguration_Orders[C.channelConfiguration - 1];
            channelConfiguration_Orders_Max = channelConfiguration_Orders[C.channelConfiguration];
            string ExpectedOrder;
            auto channelConfiguration_Orders_Base = channelConfiguration_Orders + Aac_Channels_Size_Usac;
            for (; channelConfiguration_Orders_Pos < channelConfiguration_Orders_Max; channelConfiguration_Orders_Pos++)
            {
                ExpectedOrder += usacElementType_IdNames[channelConfiguration_Orders_Base[channelConfiguration_Orders_Pos]];
                ExpectedOrder += ' ';
            }
            ExpectedOrder.pop_back();
            string ActualOrder;
            for (auto usacElement : C.usacElements)
            {
                if (usacElement.usacElementType >= ID_USAC_EXT)
                    continue;
                ActualOrder += usacElementType_IdNames[usacElement.usacElementType];
                ActualOrder += ' ';
            }
            ActualOrder.pop_back();
            Fill_Conformance("UsacConfig channelConfiguration", ("channelConfigurationIndex " + to_string(C.channelConfiguration) + " implies element order " + ExpectedOrder + " but actual element order is " + ActualOrder).c_str());
        }
    #endif

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::UsacSingleChannelElementConfig()
{
    Element_Begin1("UsacSingleChannelElementConfig");

    UsacCoreConfig();
    if (C.coreSbrFrameLengthIndex>=coreSbrFrameLengthIndex_Mapping_Size || coreSbrFrameLengthIndex_Mapping[C.coreSbrFrameLengthIndex].sbrRatioIndex)
        SbrConfig();

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::UsacChannelPairElementConfig()
{
    Element_Begin1("UsacChannelPairElementConfig");
    C.stereoConfigIndex=0;

    UsacCoreConfig();
    if (C.coreSbrFrameLengthIndex>=coreSbrFrameLengthIndex_Mapping_Size || coreSbrFrameLengthIndex_Mapping[C.coreSbrFrameLengthIndex].sbrRatioIndex)
    {
        SbrConfig();
        Get_S1(2, C.stereoConfigIndex,                           "stereoConfigIndex");
        if (C.stereoConfigIndex)
            Mps212Config(C.stereoConfigIndex);
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::UsacLfeElementConfig()
{
    // Nothing here
}

//---------------------------------------------------------------------------
void File_Usac::UsacExtElementConfig()
{
    Element_Begin1("UsacExtElementConfig");

    int32u usacExtElementType, usacExtElementConfigLength;
    bool usacExtElementDefaultLengthPresent, usacExtElementPayloadFrag;
    escapedValue(usacExtElementType, 4, 8, 16,                  "usacExtElementType"); Element_Level--; Element_Info1C(usacExtElementType<ID_EXT_ELE_Max, usacExtElementType_IdNames[usacExtElementType]); Element_Level++;
    C.usacElements.back().usacElementType+=usacExtElementType<<2;
    #if MEDIAINFO_CONFORMANCE
        if (usacExtElementType_Present.find(usacExtElementType) != usacExtElementType_Present.end() && usacExtElementType < ID_EXT_ELE_Max && usacExtElementType_ConfigNames[usacExtElementType])
        {
            auto FieldName = string(usacExtElementType_ConfigNames[usacExtElementType]);
            Fill_Conformance((FieldName + " Coherency").c_str(), (FieldName + " is present 2 times but only 1 instance is allowed").c_str());
        }
        else
        {
            usacExtElementType_Present.insert(usacExtElementType);
            if (usacExtElementType == ID_EXT_ELE_AUDIOPREROLL && C.usacElements.size() != 1)
                Fill_Conformance("AudioPreRollConfig Location", ("AudioPreRollConfig is present in position "+to_string(C.usacElements.size()-1)+" but only presence in position 0 is allowed").c_str());
        }
    #endif
    escapedValue(usacExtElementConfigLength, 4, 8, 16,          "usacExtElementConfigLength");
    #if MEDIAINFO_CONFORMANCE
        if (usacExtElementType == ID_EXT_ELE_AUDIOPREROLL && usacExtElementConfigLength)
            Fill_Conformance("AudioPreRollConfig usacExtElementConfigLength", ("usacExtElementConfigLength is "+to_string(usacExtElementConfigLength)+" but only value 0 is allowed").c_str());
    #endif
    Get_SB (usacExtElementDefaultLengthPresent,                 "usacExtElementDefaultLengthPresent");
    if (usacExtElementDefaultLengthPresent)
    {
        #if MEDIAINFO_CONFORMANCE
            if (usacExtElementType == ID_EXT_ELE_AUDIOPREROLL)
                Fill_Conformance("AudioPreRollConfig usacExtElementDefaultLengthPresent", "usacExtElementDefaultLengthPresent is 1 but only value 0 is allowed");
        #endif
        int32u usacExtElementDefaultLength;
        escapedValue(usacExtElementDefaultLength, 8, 16, 0,     "usacExtElementDefaultLength");
        C.usacElements.back().usacExtElementDefaultLength=usacExtElementDefaultLength+1;
    }
    else
        C.usacElements.back().usacExtElementDefaultLength=0;
    Get_SB (usacExtElementPayloadFrag,                          "usacExtElementPayloadFlag");
    #if MEDIAINFO_CONFORMANCE
        if (usacExtElementType == ID_EXT_ELE_AUDIOPREROLL && usacExtElementPayloadFrag)
            Fill_Conformance("AudioPreRollConfig usacExtElementConfigLength", "usacExtElementPayloadFrag is 1 but only value 0 is allowed");
    #endif
    C.usacElements.back().usacExtElementPayloadFrag=usacExtElementPayloadFrag;

    if (usacExtElementConfigLength)
    {
        usacExtElementConfigLength*=8;
        if (usacExtElementConfigLength>Data_BS_Remain())
        {
            Trusted_IsNot("Too big");
            Element_End0();
            return;
        }
        auto B=BS_Bookmark(usacExtElementConfigLength);
        switch (usacExtElementType)
        {
            case ID_EXT_ELE_FILL                              : break;
            case ID_EXT_ELE_UNI_DRC                           : uniDrcConfig(); break;
            case ID_EXT_ELE_AUDIOPREROLL                      :
            default                                           : if (usacExtElementConfigLength)
                                                                    Skip_BS(usacExtElementConfigLength, "Unknown");
        }
        BS_Bookmark(B, (usacExtElementType<ID_EXT_ELE_Max?string(usacExtElementType_Names[usacExtElementType]):("usacExtElementType"+to_string(usacExtElementType)))+"Config");
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::UsacCoreConfig()
{
    Element_Begin1("UsacCoreConfig");

    Get_SB (C.tw_mdct,                                          "tw_mdct");
    Get_SB (C.noiseFilling,                                     "noiseFilling");

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::SbrConfig()
{
    Element_Begin1("SbrConfig");

    Get_SB (C.harmonicSBR,                                      "harmonicSBR");
    Get_SB (C.bs_interTes,                                      "bs_interTes");
    Get_SB(C.bs_pvc,                                            "bs_pvc");
    SbrDlftHeader();

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::SbrDlftHeader()
{
    Element_Begin1("SbrDlftHeader");

    bool dflt_header_extra2;
    Get_S1 (4, C.dlftHandler.dflt_start_freq,                   "dflt_start_freq");
    Get_S1 (4, C.dlftHandler.dflt_stop_freq,                    "dflt_stop_freq");
    Get_SB (   C.dlftHandler.dflt_header_extra1,                "dflt_header_extra1");
    Get_SB (   dflt_header_extra2,                              "dflt_header_extra2");
    if (C.dlftHandler.dflt_header_extra1)
    {
        Get_S1 (2, C.dlftHandler.dflt_freq_scale,               "dflt_freq_scale");
        Get_SB (   C.dlftHandler.dflt_alter_scale,              "dflt_alter_scale");
        Get_S1 (2, C.dlftHandler.dflt_noise_bands,              "dflt_noise_bands");
    }
    else
    {
        C.dlftHandler.dflt_freq_scale=2;
        C.dlftHandler.dflt_alter_scale=1;
        C.dlftHandler.dflt_noise_bands=2;
    }
    if (dflt_header_extra2)
    {
        Skip_S1(2,                                              "dflt_limiter_bands");
        Skip_S1(2,                                              "dflt_limiter_gains");
        Skip_SB(                                                "dflt_interpol_freq");
        Skip_SB(                                                "dflt_smoothing_mode");
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::Mps212Config(int8u StereoConfigindex)
{
    Element_Begin1("Mps212Config");

    int8u bsTempShapeConfig;
    bool bsOttBandsPhasePresent;
    Skip_S1(3,                                                  "bsFreqRes");
    Skip_S1(3,                                                  "bsFixedGainDMX");
    Get_S1 (2, bsTempShapeConfig,                               "bsTempShapeConfig");
    Skip_S1(2,                                                  "bsDecorrConfig");
    Skip_SB(                                                    "bsHighRatelMode");
    Skip_SB(                                                    "bsPhaseCoding");
    Get_SB (   bsOttBandsPhasePresent,                          "bsOttBandsPhasePresent");
    if (bsOttBandsPhasePresent)
    {
        Skip_S1(5,                                              "bsOttBandsPhase");
    }
    if (StereoConfigindex>1)
    {
        Skip_S1(5,                                              "bsResidualBands");
        Skip_SB(                                                "bSPseudor");
    }
    if (bsTempShapeConfig==2)
    {
        Skip_SB(                                                "bSEnvOuantMode");
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::uniDrcConfig()
{
    C.downmixInstructions_Data.clear();
    C.drcInstructionsUniDrc_Data.clear();
    C.loudnessInfo_Data[0].clear();
    C.loudnessInfo_Data[1].clear();

    Element_Begin1("uniDrcConfig");

    int8u downmixInstructionsCount, drcCoefficientsBasicCount, drcInstructionsBasicCount, drcCoefficientsUniDrcCount, drcInstructionsUniDrcCount;
    TEST_SB_SKIP(                                               "sampleRatePresent");
        int32u bsSampleRate;
        Get_S3 (18, bsSampleRate ,                              "bsSampleRate"); bsSampleRate+=1000; Param_Info2(bsSampleRate, " Hz");
        #if MEDIAINFO_CONFORMANCE
            if (C.sampling_frequency && bsSampleRate != C.sampling_frequency)
                Fill_Conformance("Crosscheck UsacConfig+uniDrcConfig usacSamplingFrequency+bsSampleRate", ("UsacConfig usacSamplingFrequency " + to_string(C.sampling_frequency) + " does not match uniDrcConfig bsSampleRate " + to_string(bsSampleRate)).c_str());
        #endif
    TEST_SB_END();
    Get_S1 (7, downmixInstructionsCount,                        "downmixInstructionsCount");
    TESTELSE_SB_SKIP(                                           "drcDescriptionBasicPresent");
        Get_S1 (3, drcCoefficientsBasicCount,                   "drcCoefficientsBasicCount");
        Get_S1 (4, drcInstructionsBasicCount,                   "drcInstructionsBasicCount");
    TESTELSE_SB_ELSE(                                           "drcDescriptionBasicPresent");
        drcCoefficientsBasicCount=0;
        drcInstructionsBasicCount=0;
    TESTELSE_SB_END();
    Get_S1 (3, drcCoefficientsUniDrcCount,                      "drcCoefficientsUniDrcCount");
    Get_S1 (6, drcInstructionsUniDrcCount,                      "drcInstructionsUniDrcCount");
    #if MEDIAINFO_CONFORMANCE
        if (downmixInstructionsCount)
            Fill_Conformance("uniDrcConfig downmixInstructionsCount", "Version 0 shall not be used");
        if (drcCoefficientsBasicCount)
            Fill_Conformance("uniDrcConfig drcCoefficientsBasicCount", "Version 0 shall not be used");
        if (drcInstructionsBasicCount)
            Fill_Conformance("uniDrcConfig drcInstructionsBasicCount", "Version 0 shall not be used");
        if (drcCoefficientsUniDrcCount)
            Fill_Conformance("uniDrcConfig drcCoefficientsUniDrcCount", "Version 0 shall not be used");
        if (drcInstructionsUniDrcCount)
            Fill_Conformance("uniDrcConfig drcInstructionsUniDrcCount", "Version 0 shall not be used");
    #endif
    channelLayout();
    for (int8u i=0; i<downmixInstructionsCount; i++)
        downmixInstructions();
    for (int8u i=0; i<drcCoefficientsBasicCount; i++)
        drcCoefficientsBasic();
    for (int8u i=0; i<drcInstructionsBasicCount; i++)
        drcInstructionsBasic();
    for (int8u i=0; i<drcCoefficientsUniDrcCount; i++)
        drcCoefficientsUniDrc();
    for (int8u i=0; i<drcInstructionsUniDrcCount; i++)
        drcInstructionsUniDrc();
    bool uniDrcConfigExtPresent;
    Get_SB (   uniDrcConfigExtPresent,                          "uniDrcConfigExtPresent");
    if (uniDrcConfigExtPresent)
        uniDrcConfigExtension();

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::uniDrcConfigExtension()
{
    for (;;)
    {
        Element_Begin1("uniDrcConfigExtension");
        int32u bitSize;
        int8u uniDrcConfigExtType, extSizeBits;
        Get_S1 (4, uniDrcConfigExtType,                         "uniDrcConfigExtType"); Element_Info1C(uniDrcConfigExtType<UNIDRCCONFEXT_Max, uniDrcConfigExtType_IdNames[uniDrcConfigExtType]);
        if (!uniDrcConfigExtType) // UNIDRCCONFEXT_TERM
        {
            Element_End0();
            break;
        }
        Get_S1 (4, extSizeBits,                                 "bitSizeLen");
        extSizeBits+=4;
        Get_S4 (extSizeBits, bitSize,                           "bitSize");
        bitSize++;
        if (bitSize>Data_BS_Remain())
        {
            Trusted_IsNot("Too big");
            Element_End0();
            break;
        }

        auto B=BS_Bookmark(bitSize);
        switch (uniDrcConfigExtType)
        {
            /*
            case UNIDRCCONFEXT_PARAM_DRC :
                {
                drcCoefficientsParametricDrc();
                int8u parametricDrcInstructionsCount;
                Get_S1 (4, parametricDrcInstructionsCount,      "parametricDrcInstructionsCount");
                for (int8u i=0; i<parametricDrcInstructionsCount; i++)
                    parametricDrcInstructions();
                Skip_BS(Data_BS_Remain(),                       "(Not implemented)");
                }
                break;
            */
            case UNIDRCCONFEXT_V1 :
                {
                TEST_SB_SKIP(                                   "downmixInstructionsV1Present");
                    int8u downmixInstructionsV1Count;
                    Get_S1 (7, downmixInstructionsV1Count,      "downmixInstructionsV1Count");
                    for (int8u i=0; i<downmixInstructionsV1Count; i++)
                        downmixInstructions(true);
                TEST_SB_END();
                TEST_SB_SKIP(                                   "drcCoeffsAndInstructionsUniDrcV1Present");
                    int8u drcCoefficientsUniDrcV1Count;
                    Get_S1 (3, drcCoefficientsUniDrcV1Count,    "drcCoefficientsUniDrcV1Count");
                    for (int8u i=0; i<drcCoefficientsUniDrcV1Count; i++)
                        drcCoefficientsUniDrc(true);
                    int8u drcInstructionsUniDrcV1Count;
                    Get_S1 (6, drcInstructionsUniDrcV1Count,    "drcInstructionsUniDrcV1Count");
                    #if MEDIAINFO_CONFORMANCE
                        C.drcRequired_Present = 0;
                    #endif
                    for (int8u i=0; i<drcInstructionsUniDrcV1Count; i++)
                        drcInstructionsUniDrc(true);
                    #if MEDIAINFO_CONFORMANCE
                        if (C.drcRequired_Present != 0x27)
                        {
                            string Value;
                            for (int8u i = 0; i < 6; i++)
                            {
                                if (!(C.drcRequired_Present & (1 << i)))
                                    Fill_Conformance("drcInstructions drcSetEffect", (string(drcSetEffect_List[i]) + " isn't in at least one DRC").c_str());
                                if (i == 2)
                                    i += 2;
                            }
                        }
                    #endif
                TEST_SB_END();
                bool MustSkip=false;
                TEST_SB_SKIP(                                   "loudEqInstructionsPresent");
                    int8u loudEqInstructionsCount;
                    Get_S1 (4, loudEqInstructionsCount,         "loudEqInstructionsCount");
                    for (int8u i=0; i<loudEqInstructionsCount; i++)
                        MustSkip=true; // Not yet implemented
                    //    loudEqInstructions();
                TEST_SB_END();
                if (!MustSkip)
                TEST_SB_SKIP(                                   "eqPresent");
                    // Not yet implemented
                    //int8u eqInstructionsCount;
                    //eqCoefficients();
                    //Get_S1 (4, eqInstructionsCount,             "eqInstructionsCount");
                    //for (int8u i=0; i<eqInstructionsCount; i++)
                    //    eqInstructions();
                TEST_SB_END();
                if (Data_BS_Remain()>B.BitsNotIncluded)
                    Skip_BS(Data_BS_Remain()-B.BitsNotIncluded, "(Not implemented)");
                }
                break;
            default:
                Skip_BS(bitSize,                                "Unknown");
        }
        BS_Bookmark(B, uniDrcConfigExtType<UNIDRCCONFEXT_Max?string(uniDrcConfigExtType_ConfNames[uniDrcConfigExtType]):("uniDrcConfigExtType"+to_string(uniDrcConfigExtType)));
        Element_End0();
    }
}

//---------------------------------------------------------------------------
void File_Usac::downmixInstructions(bool V1)
{
    Element_Begin1("downmixInstructionsV1");

    bool layoutSignalingPresent;
    int8u downmixId, targetChannelCount;
    Get_S1 (7, downmixId,                                       "downmixId");
    Get_S1 (7, targetChannelCount,                              "targetChannelCount");
    Skip_S1(8,                                                  "targetLayout");
    Get_SB (   layoutSignalingPresent,                          "layoutSignalingPresent");
    if (layoutSignalingPresent)
    {
        if (V1)
            Skip_S1(4,                                          "bsDownmixOffset");
        for (int8u i=0; i<targetChannelCount; i++)
            for (int8u j=0; j<C.baseChannelCount; j++)
                Skip_S1(V1?5:4,                                 V1?"bsDownmixCoefficientV1":"bsDownmixCoefficient");
    }
    C.downmixInstructions_Data[downmixId].targetChannelCount=targetChannelCount;
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::drcCoefficientsBasic()
{
    Element_Begin1("drcCoefficientsBasic");

    Skip_S1(4,                                                  "drcLocation");
    Skip_S1(7,                                                  "drcCharacteristic");

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::drcCoefficientsUniDrc(bool V1)
{
    Element_Begin1(V1?"drcCoefficientsUniDrcV1":"drcCoefficientsUniDrc");

    bool drcFrameSizePresent;
    Skip_S1(4,                                                  "drcLocation");
    Get_SB (   drcFrameSizePresent,                             "drcFrameSizePresent");
    if (drcFrameSizePresent)
        Skip_S2(15,                                             "bsDrcFrameSize");
    if (V1)
    {
        bool drcCharacteristicLeftPresent;
        Get_SB (   drcCharacteristicLeftPresent,                "drcCharacteristicLeftPresent");
        if (drcCharacteristicLeftPresent)
        {
            int8u characteristicLeftCount;
            Get_S1 (4, characteristicLeftCount,                 "characteristicLeftCount");
            for (int8u k=0; k<characteristicLeftCount; k++)
            {
                bool characteristicFormat;
                Get_SB (   characteristicFormat,                "characteristicFormat");
                if (!characteristicFormat)
                {
                    Skip_S1(6,                                  "bsGainLeft");
                    Skip_S1(4,                                  "bsIoRatioLeft");
                    Skip_S1(4,                                  "bsExpLeft");
                    Skip_SB(                                    "flipSignLeft");
                }
                else
                {
                    int8u bsCharNodeCount;
                    Get_S1 (2, bsCharNodeCount,                 "bsCharNodeCount");
                    for (int8u n=0; n<=bsCharNodeCount; n++)
                    {
                        Skip_S1(5,                              "bsNodeLevelDelta");
                        Skip_S1(8,                              "bsNodeGain");
                    }
                }
            }
        }
        bool drcCharacteristicRightPresent;
        Get_SB (   drcCharacteristicRightPresent,               "drcCharacteristicRightPresent");
        if (drcCharacteristicLeftPresent)
        {
            int8u characteristicRightCount;
            Get_S1 (4, characteristicRightCount,                "characteristicRightCount");
            for (int8u k=0; k<characteristicRightCount; k++)
            {
                bool characteristicFormat;
                Get_SB (   characteristicFormat,                "characteristicFormat");
                if (!characteristicFormat)
                {
                    Skip_S1(6,                                  "bsGainLeft");
                    Skip_S1(4,                                  "bsIoRatioLeft");
                    Skip_S1(4,                                  "bsExpLeft");
                    Skip_SB(                                    "flipSignLeft");
                }
                else
                {
                    int8u bsCharNodeCount;
                    Get_S1 (2, bsCharNodeCount,                 "bsCharNodeCount");
                    for (int8u n=0; n<=bsCharNodeCount; n++)
                    {
                        Skip_S1(5,                              "bsNodeLevelDelta");
                        Skip_S1(8,                              "bsNodeGain");
                    }
                }
            }
        }
        bool shapeFiltersPresent;
        Get_SB (   shapeFiltersPresent,                         "shapeFiltersPresent");
        if (shapeFiltersPresent)
        {
            int8u shapeFilterCount;
            Get_S1 (4, shapeFilterCount,                        "shapeFilterCount");
            for (int8u k=0; k<shapeFilterCount; k++)
            {
                TEST_SB_SKIP(                                   "lfCutFilterPresent");
                    Skip_S1(3,                                  "lfCornerFreqIndex");
                    Skip_S1(2,                                  "lfFilterStrengthIndex");
                TEST_SB_END();
                TEST_SB_SKIP(                                   "lfBoostFilterPresent");
                    Skip_S1(3,                                  "lfCornerFreqIndex");
                    Skip_S1(2,                                  "lfFilterStrengthIndex");
                TEST_SB_END();
                TEST_SB_SKIP(                                   "hfCutFilterPresent");
                    Skip_S1(3,                                  "lfCornerFreqIndex");
                    Skip_S1(2,                                  "lfFilterStrengthIndex");
                TEST_SB_END();
                TEST_SB_SKIP(                                   "hfBoostFilterPresent");
                    Skip_S1(3,                                  "lfCornerFreqIndex");
                    Skip_S1(2,                                  "lfFilterStrengthIndex");
                TEST_SB_END();
            }
        }
        Skip_S1(6,                                              "gainSequenceCount");
    }
    int8u gainSetCount;
    Get_S1 (6, gainSetCount,                                    "gainSetCount");
    C.gainSets.clear();
    for (int8u i=0; i<gainSetCount; i++)
    {
        Element_Begin1("gainSet");
        gain_set gainSet;
        int8u gainCodingProfile;
        Get_S1 (2, gainCodingProfile,                           "gainCodingProfile");
        Skip_SB(                                                "gainInterpolationType");
        Skip_SB(                                                "fullFrame");
        Skip_SB(                                                "timeAlignment");
        TEST_SB_SKIP(                                           "timeDeltaMinPresent");
            Skip_S2(11,                                         "bsTimeDeltaMin");
        TEST_SB_END();
        if (gainCodingProfile != 3)
        {
            bool drcBandType;
            Get_S1 (4, gainSet.bandCount,                       "bandCount");
            if (gainSet.bandCount>1)
                Get_SB (drcBandType,                            "drcBandType");
            for (int8u i=0; i<gainSet.bandCount; i++)
            {
                Element_Begin1("bandCount");
                if (V1)
                {
                    TEST_SB_SKIP(                               "indexPresent");
                        Skip_S1(6,                              "bsIndex");
                    TEST_SB_END();
                }
                if (!V1)
                {
                    Skip_S1(7,                                  "drcCharacteristic");
                }
                else //V1
                {
                    TEST_SB_SKIP(                               "drcCharacteristicPresent");
                        bool drcCharacteristicFormatIsCICP;
                        Get_SB (drcCharacteristicFormatIsCICP,  "drcCharacteristicFormatIsCICP");
                        if (drcCharacteristicFormatIsCICP)
                        {
                            Skip_S1(7,                          "drcCharacteristic");
                        }
                        else
                        {
                            Skip_S1(4,                          "drcCharacteristicLeftIndex");
                            Skip_S1(4,                          "drcCharacteristicRightIndex");
                        }
                    TEST_SB_END();
                }
                Element_End0();
            }
            for (int8u i=1; i <gainSet.bandCount; i++)
            {
                if (drcBandType)
                    Skip_S1( 4,                                 "crossoverFreqIndex");
                else
                    Skip_S2(10,                                 "startSubBandIndex");
            }
        }
        else
            gainSet.bandCount=1;
        C.gainSets.push_back(gainSet);
        Element_End0();
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::drcInstructionsBasic()
{
    Element_Begin1("drcInstructionsBasic");

    int16u drcSetEffect;
    Skip_S1(6,                                                  "drcSetId");
    Skip_S1(4,                                                  "drcLocation");
    Skip_S1(7,                                                  "downmixId");
    TEST_SB_SKIP(                                               "additionalDownmixIdPresent");
        int8u additionalDownmixIdCount;
        Get_S1 (3, additionalDownmixIdCount,                    "additionalDownmixIdCount");
        for (int8u i=1; i <additionalDownmixIdCount; i++)
            Skip_S1(7,                                          "additionalDownmixId");
    TEST_SB_END();
    Get_S2 (16, drcSetEffect,                                   "drcSetEffect");
    if ((drcSetEffect & (3<<10)) == 0)
    {
        TEST_SB_SKIP(                                           "limiterPeakTargetPresent");
            Skip_S1(8,                                          "bsLimiterPeakTarget");
        TEST_SB_END();
    }
    TEST_SB_SKIP(                                               "drcSetTargetLoudnessPresent");
        Skip_S1(6,                                              "bsDrcSetTargetLoudnessValueUpper");
        TEST_SB_SKIP(                                           "drcSetTargetLoudnessValueLowerPresent");
            Skip_S1(6,                                          "bsDrcSetTargetLoudnessValueLower");
        TEST_SB_END();
    TEST_SB_END();

    Element_End0();
}

//---------------------------------------------------------------------------
bool File_Usac::drcInstructionsUniDrc(bool V1, bool NoV0)
{
    Element_Begin1(V1?"drcInstructionsUniDrcV1":"drcInstructionsUniDrc");

    int8u channelCount=C.baseChannelCount;
    vector<int8s> gainSetIndex;
    int16u drcSetEffect;
    int8u drcSetId, downmixId;
    bool downmixIdPresent;
    Get_S1 (6, drcSetId,                                        "drcSetId");
    if (V1)
        Skip_S1(4,                                              "drcSetComplexityLevel");
    Skip_S1(4,                                                  "drcLocation");
    if (V1)
        Get_SB (downmixIdPresent,                               "downmixIdPresent");
    else
        downmixIdPresent=true;
    if (downmixIdPresent)
    {
        bool drcApplyToDownmix;
        Get_S1(7, downmixId,                                    "downmixId");
        if (V1)
            Get_SB (   drcApplyToDownmix,                       "drcApplyToDownmix");
        else
            drcApplyToDownmix=downmixId?true:false;
        int8u additionalDownmixIdCount;
        TESTELSE_SB_SKIP(                                       "additionalDownmixIdPresent");
            Get_S1 (3, additionalDownmixIdCount,                "additionalDownmixIdCount");
            for (int8u i=0; i<additionalDownmixIdCount; i++)
                Skip_S1(7,                                      "additionalDownmixId");
        TESTELSE_SB_ELSE(                                       "additionalDownmixIdPresent");
            additionalDownmixIdCount=0;
        TESTELSE_SB_END();
        if ((!V1 || drcApplyToDownmix) && downmixId && downmixId!=0x7F && !additionalDownmixIdCount)
        {
            std::map<int8u, downmix_instruction>::iterator downmixInstruction_Data=C.downmixInstructions_Data.find(downmixId);
            if (downmixInstruction_Data!=C.downmixInstructions_Data.end())
                channelCount=downmixInstruction_Data->second.targetChannelCount;
            else
                channelCount=1;
        }
        else if ((!V1 || drcApplyToDownmix) && (downmixId==0x7F || additionalDownmixIdCount))
            channelCount=1;
    }
    else
        downmixId=0; // 0 is default
    Get_S2 (16, drcSetEffect,                                   "drcSetEffect");
    #if MEDIAINFO_CONFORMANCE
        C.drcRequired_Present |= (drcSetEffect & 0x27); //Bits (1 is LSB) 1, 2, 3, 6;
    #endif
    bool IsNOK=false;
    if ((drcSetEffect & (3<<10)) == 0)
    {
        TESTELSE_SB_SKIP(                                       "limiterPeakTargetPresent");
            Skip_S1(8,                                          "bsLimiterPeakTarget");
        TESTELSE_SB_ELSE(                                       "limiterPeakTargetPresent");
            #if MEDIAINFO_CONFORMANCE
                //Fill_Conformance("drcInstructions limiterPeakTargetPresent", "limiterPeakTargetPresent is 0", bitset8(), Warning);
            #endif
        TESTELSE_SB_END();
    }
    else
        channelCount=C.baseChannelCount; // TEMP
    TEST_SB_SKIP(                                               "drcSetTargetLoudnessPresent");
        Skip_S1(6,                                              "bsDrcSetTargetLoudnessValueUpper");
        TEST_SB_SKIP(                                           "drcSetTargetLoudnessValueLowerPresent");
            Skip_S1(6,                                          "bsDrcSetTargetLoudnessValueLower");
        TEST_SB_END();
    TEST_SB_END();
    TESTELSE_SB_SKIP(                                           "dependsOnDrcSetPresent");
        Skip_S1(6,                                              "dependsOnDrcSet");
    TESTELSE_SB_ELSE(                                           "dependsOnDrcSetPresent");
        Skip_SB(                                                "noIndependentUse");
    TESTELSE_SB_END();
    if (V1)
        Skip_SB(                                                "requiresEq");
    for (int8u c=0; c<channelCount; c++)
    {
        Element_Begin1("channel");
        int8u bsGainSetIndex;
        Get_S1 (6, bsGainSetIndex,                              "bsGainSetIndex");
        gainSetIndex.push_back(bsGainSetIndex);
        if ((drcSetEffect & (3<<10)) != 0)
        {
            TEST_SB_SKIP(                                       "duckingScalingPresent");
                Skip_S1(4,                                      "bsDuckingScaling");
            TEST_SB_END();
        }
        TEST_SB_SKIP(                                           "repeatGainSetIndex");
            int8u bsRepeatGainSetIndexCount;
            Get_S1 (5, bsRepeatGainSetIndexCount,               "bsRepeatGainSetIndexCount");
            bsRepeatGainSetIndexCount++;
            gainSetIndex.resize(gainSetIndex.size()+bsRepeatGainSetIndexCount, bsGainSetIndex);
            c+=bsRepeatGainSetIndexCount;
        TEST_SB_END();
        Element_End0();
    }

    set<int8s> DrcChannelGroups=set<int8s>(gainSetIndex.begin(), gainSetIndex.end());

    for (set<int8s>::iterator DrcChannelGroup=DrcChannelGroups.begin(); DrcChannelGroup!=DrcChannelGroups.end(); ++DrcChannelGroup)
    {
        if (!*DrcChannelGroup || (drcSetEffect & (3<<10)))
            continue; // 0 means not present
        Element_Begin1("DrcChannel");
        int8s gainSetIndex=*DrcChannelGroup-1;
        int8u bandCount=V1?(gainSetIndex<C.gainSets.size()?C.gainSets[gainSetIndex].bandCount:0):1;
        for (int8u k=0; k<bandCount; k++)
        {
            Element_Begin1("band");
            if (V1)
            {
            TEST_SB_SKIP(                                       "targetCharacteristicLeftPresent");
                Skip_S1(4,                                      "targetCharacteristicLeftIndex");
            TEST_SB_END();
            TEST_SB_SKIP(                                       "targetCharacteristicRightPresent");
                Skip_S1(4,                                      "targetCharacteristicRightIndex");
            TEST_SB_END();
            }
            TEST_SB_SKIP(                                       "gainScalingPresent");
                Skip_S1(4,                                      "bsAttenuationScaling");
                Skip_S1(4,                                      "bsAmplificationScaling");
            TEST_SB_END();
            TEST_SB_SKIP(                                       "gainOffsetPresent");
                Skip_SB(                                        "bsGainSign");
                Skip_S1(5,                                      "bsGainOffset");
            TEST_SB_END();
            Element_End0();
        }
        if (V1 && bandCount==1)
        {
            TEST_SB_SKIP(                                       "shapeFilterPresent");
                Skip_S1(4,                                      "shapeFilterIndex");
            TEST_SB_END();
        }
        Element_End0();
    }

    Element_End0();

    if (V1 || NoV0) //We want to display only V1 information
    {
        string Value;
        for (int8u i=0; i<16; i++)
            if (drcSetEffect&(1<<i))
            {
                if (!Value.empty())
                    Value+=" & ";
                if (i<drcSetEffect_List_Size)
                    Value+=drcSetEffect_List[i];
                else
                {
                    if (i>=10)
                        Value+='1';
                    Value+='0'+(i%10);
                }
            }
        drc_id Id={drcSetId, downmixId, 0};
        C.drcInstructionsUniDrc_Data[Id].drcSetEffectTotal=Value;
    }

    return false;
}

//---------------------------------------------------------------------------
void File_Usac::channelLayout()
{
    Element_Begin1("channelLayout");

    bool layoutSignalingPresent;
    Get_S1 (7, C.baseChannelCount,                              "C.baseChannelCount");
    #if MEDIAINFO_CONFORMANCE
        if (C.channelConfiguration && C.channelConfiguration < Aac_Channels_Size_Usac && Aac_Channels[C.channelConfiguration] != C.baseChannelCount)
            Fill_Conformance("Crosscheck UsacConfig+uniDrcConfig numOutChannels+baseChannelCount", ("UsacConfig numOutChannels " + to_string(Aac_Channels[C.channelConfiguration]) + " does not match uniDrcConfig baseChannelCount " + to_string(C.baseChannelCount)).c_str());
    #endif
    Get_SB (   layoutSignalingPresent,                          "layoutSignalingPresent");
    if (layoutSignalingPresent)
    {
        int8u definedLayout;
        Get_S1 (8, definedLayout,                               "definedLayout");
        if (!definedLayout)
        {
            for (int8u i=0; i<C.baseChannelCount; i++)
            {
                Info_S1(7, speakerPosition,                     "speakerPosition"); Param_Info1(Aac_OutputChannelPosition_GetString(speakerPosition));
            }
        }
    }

    Element_End0();
}

//---------------------------------------------------------------------------
static const size_t UsacConfigExtension_usacConfigExtType_Size=8;
static const char* UsacConfigExtension_usacConfigExtType[UsacConfigExtension_usacConfigExtType_Size]=
{
    "FILL",
    NULL,
    "LOUDNESS_INFO",
    NULL,
    NULL,
    NULL,
    NULL,
    "STREAM_ID",
};
void File_Usac::UsacConfigExtension()
{
    Element_Begin1("UsacConfigExtension");

    int32u numConfigExtensions;
    escapedValue(numConfigExtensions, 2, 4, 8,                  "numConfigExtensions minus 1");

    for (int32u confExtIdx=0; confExtIdx<=numConfigExtensions; confExtIdx++) 
    {
        Element_Begin1("usacConfigExtension");
        int32u usacConfigExtType, usacConfigExtLength;
        escapedValue(usacConfigExtType, 4, 8, 16,               "usacConfigExtType"); Param_Info1C(usacConfigExtType<UsacConfigExtension_usacConfigExtType_Size && UsacConfigExtension_usacConfigExtType[usacConfigExtType], UsacConfigExtension_usacConfigExtType[usacConfigExtType]);
        escapedValue(usacConfigExtLength, 4, 8, 16,             "usacExtElementConfigLength");

        if (usacConfigExtLength)
        {
            usacConfigExtLength*=8;
            if (usacConfigExtLength>Data_BS_Remain())
            {
                Trusted_IsNot("Too big");
                Element_End0();
                break;
            }
            auto B=BS_Bookmark(usacConfigExtLength);
            switch (usacConfigExtType)
            {
                case ID_CONFIG_EXT_FILL                       : fill_bytes(usacConfigExtLength); break;
                case ID_CONFIG_EXT_LOUDNESS_INFO              : loudnessInfoSet(); break;
                case ID_CONFIG_EXT_STREAM_ID                  : streamId(); break;
                default                                       : Skip_BS(usacConfigExtLength,                "Unknown");
            }
            if (BS_Bookmark(B, usacConfigExtType<ID_CONFIG_EXT_Max?string(usacConfigExtType_ConfNames[usacConfigExtType]):("usacConfigExtType"+to_string(usacConfigExtType))))
            {
                #if MEDIAINFO_CONFORMANCE
                    if (usacConfigExtType==ID_CONFIG_EXT_LOUDNESS_INFO)
                        C.LoudnessInfoIsNotValid=true;
                #endif
            }
        }
        Element_End0();
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::fill_bytes(size_t usacConfigExtLength)
{
    #if MEDIAINFO_CONFORMANCE
        Element_Begin1("fill_bytes");
        map<int8u, size_t> fill_bytes;
        for (; usacConfigExtLength; usacConfigExtLength -= 8)
        {
            int8u fill_byte;
            Get_S1 (8, fill_byte,                               "fill_byte");
            if (fill_byte != 0b10100101)
                fill_bytes[fill_byte]++;
        }
        if (!fill_bytes.empty())
            Fill_Conformance("fill_byte", "fill_byte is "+(fill_bytes.size()==1?("0b"+Ztring::ToZtring(fill_bytes.begin()->first, 2).To_UTF8()):"with different values")+" but only 0b10100101 is expected", bitset8(), Warning);
        Element_End0();
    #else
        Skip_BS(usacConfigExtLength,                            "0b10100101");
    #endif
}

//---------------------------------------------------------------------------
void File_Usac::loudnessInfoSet(bool V1)
{
    Element_Begin1(V1?"loudnessInfoSetV1":"loudnessInfoSet");

    #if MEDIAINFO_CONFORMANCE
        C.loudnessInfoSet_Present[V1]++;
    #endif

    int8u loudnessInfoAlbumCount, loudnessInfoCount;
    bool loudnessInfoSetExtPresent;
    Get_S1 (6, loudnessInfoAlbumCount,                          "loudnessInfoAlbumCount");
    Get_S1 (6, loudnessInfoCount,                               "loudnessInfoCount");
    #if MEDIAINFO_CONFORMANCE
        if (loudnessInfoAlbumCount || loudnessInfoCount)
            C.loudnessInfoSet_HasContent[V1]=true;
    #endif

    for (int8u i=0; i<loudnessInfoAlbumCount; i++)
        loudnessInfo(true, V1);
    for (int8u i=0; i<loudnessInfoCount; i++)
        loudnessInfo(false, V1);
    if (!V1)
    {
        Get_SB (loudnessInfoSetExtPresent,                      "loudnessInfoSetExtPresent");
        if (loudnessInfoSetExtPresent)
            loudnessInfoSetExtension();
    }

    Element_End0();
}

//---------------------------------------------------------------------------
static const size_t methodDefinition_Format_Size=10;
static const int8u methodDefinition_Format[methodDefinition_Format_Size]=
{
    8, 8, 8, 8, 8, 8, 8, 5, 2, 8,
};
bool File_Usac::loudnessInfo(bool FromAlbum, bool V1)
{
    Element_Begin1(V1?"loudnessInfoV1":"loudnessInfo");

    loudness_info::measurements Measurements;
    int16u bsSamplePeakLevel, bsTruePeakLevel;
    int8u measurementCount;
    bool samplePeakLevelPresent, truePeakLevelPresent;
    int8u drcSetId, eqSetId, downmixId;
    Get_S1 (6, drcSetId,                                        "drcSetId");
    if (V1)
        Get_S1 (6, eqSetId,                                     "eqSetId");
    else
        eqSetId=0;
    Get_S1 (7, downmixId,                                       "downmixId");
    Get_SB (samplePeakLevelPresent,                             "samplePeakLevelPresent");
    if (samplePeakLevelPresent)
    {
        Get_S2(12, bsSamplePeakLevel,                           "bsSamplePeakLevel"); Param_Info1C(bsSamplePeakLevel, Ztring::ToZtring(20-((double)bsSamplePeakLevel)/32)+__T(" dBTP"));
    }
    Get_SB (truePeakLevelPresent,                               "truePeakLevelPresent");
    if (truePeakLevelPresent)
    {
        int8u measurementSystem, reliability;
        Get_S2 (12, bsTruePeakLevel,                            "bsTruePeakLevel"); Param_Info1C(bsTruePeakLevel, Ztring::ToZtring(20-((double)bsTruePeakLevel)/32)+__T(" dBTP"));
        Get_S1 ( 4, measurementSystem,                          "measurementSystem"); Param_Info1C(measurementSystem<measurementSystems_Size, measurementSystems[measurementSystem]);
        Get_S1 ( 2, reliability,                                "reliability"); Param_Info1(reliabilities[reliability]);
            #if MEDIAINFO_CONFORMANCE
            if (measurementSystem == 4)
                Fill_Conformance("loudnessInfo measurementSystem", (to_string(measurementSystem) + (measurementSystem < measurementSystems_Size ? (string(" (") + measurementSystems[measurementSystem] + ')') : string()) + " is incorrect").c_str(), bitset8(), Warning);
            if (reliability == 1)
                Fill_Conformance("loudnessInfo reliability", (to_string(reliability) + " (" + reliabilities[reliability] + ") is incorrect").c_str(), bitset8(), Warning);
        #endif
    }
    #if MEDIAINFO_CONFORMANCE
        if (!drcSetId && !samplePeakLevelPresent && !truePeakLevelPresent)
            Fill_Conformance("loudnessInfo Coherency", "None of samplePeakLevelPresent or truePeakLevelPresent is present", bitset8().set(Usac), Warning);
    #endif
    Get_S1 (4, measurementCount,                                "measurementCount");
    bool IsNOK=false;
    #if MEDIAINFO_CONFORMANCE
        set<measurement_present> Measurements_Present;
    #endif
    for (int8u i=0; i<measurementCount; i++)
    {
        Element_Begin1("measurement");
        int8u methodDefinition, methodValue, measurementSystem, reliability;
        Get_S1 (4, methodDefinition,                            "methodDefinition"); Param_Info1C(methodDefinition && methodDefinition<=LoudnessMeaning_Size, LoudnessMeaning[methodDefinition-1]);
        int8u Size;
        if (methodDefinition>=methodDefinition_Format_Size)
        {
            Param_Info1("(Unsupported)");
            Measurements.Values[methodDefinition].From_UTF8("(Unsupported)");
            IsNOK=true;
            Element_End0();
            break;
        }
        Get_S1 (methodDefinition_Format[methodDefinition], methodValue, "methodValue");
        Ztring measurement=Loudness_Value(methodDefinition, methodValue);
        Param_Info1(measurement);
        Get_S1 (4, measurementSystem,                           "measurementSystem"); Param_Info1C(measurementSystem<measurementSystems_Size, measurementSystems[measurementSystem]);
        Get_S1 (2, reliability,                                 "reliability"); Param_Info1(reliabilities[reliability]);
        #if MEDIAINFO_CONFORMANCE
            if (measurementSystem >= 7 && measurementSystem <= 11) // || measurementSystem == 4
                Fill_Conformance("loudnessInfo measurementSystem", (to_string(measurementSystem) + (measurementSystem < measurementSystems_Size ? (string(" (") + measurementSystems[measurementSystem] + ')') : string()) + " is incorrect").c_str(), bitset8(), Warning);
            //if (reliability == 1)
            //    Fill_Conformance("loudnessInfo reliability", (to_string(reliability) + " (" + reliabilities[reliability] + ") is incorrect").c_str(), bitset8(), Warning);
            measurement_present Measurement_Present = { methodDefinition, measurementSystem };
            if (Measurements_Present.find(Measurement_Present) != Measurements_Present.end())
            {
                string Field = "methodDefinition-measurementSystem " + to_string(methodDefinition) + '-' + to_string(measurementSystem);
                if ((methodDefinition && methodDefinition <= LoudnessMeaning_Size) || measurementSystem < measurementSystems_Size)
                {
                    Field += " (";
                    if (methodDefinition && methodDefinition <= LoudnessMeaning_Size)
                        Field += LoudnessMeaning[methodDefinition - 1];
                    else
                        Field += to_string(methodDefinition);
                    Field += ", ";
                    if (measurementSystem < measurementSystems_Size)
                        Field += measurementSystems[measurementSystem];
                    else
                        Field += to_string(measurementSystem);
                    Field += ')';
                }
                Fill_Conformance("loudnessInfo measurement Coherency", (Field + " is present 2 times but only 1 instance is allowed").c_str());
            }
            else
                Measurements_Present.insert(Measurement_Present);
        #endif

        if (methodDefinition)
        {
            Ztring& Content=Measurements.Values[methodDefinition];
            if (!Content.empty())
                Content+=__T(" & ");
            Content+=measurement;
        }
        Element_End0();
    }

    drc_id Id={drcSetId, downmixId, eqSetId};
    #if MEDIAINFO_CONFORMANCE
        if (C.loudnessInfo_Data[FromAlbum].find(Id) != C.loudnessInfo_Data[FromAlbum].end())
            Fill_Conformance("loudnessInfo Coherency", ((Id.empty() ? string("Default loudness") : Id.to_string()) + " is present 2 times but only 1 instance is allowed").c_str());
    #endif
    C.loudnessInfo_Data[FromAlbum][Id].SamplePeakLevel=((samplePeakLevelPresent && bsSamplePeakLevel)?(Ztring::ToZtring(20-((double)bsSamplePeakLevel)/32)+__T(" dBFS")):Ztring());
    C.loudnessInfo_Data[FromAlbum][Id].TruePeakLevel=((truePeakLevelPresent && bsTruePeakLevel)?(Ztring::ToZtring(20-((double)bsTruePeakLevel)/32)+__T(" dBTP")):Ztring());
    C.loudnessInfo_Data[FromAlbum][Id].Measurements=Measurements;
    Element_End0();

    return IsNOK;
}

//---------------------------------------------------------------------------
void File_Usac::loudnessInfoSetExtension()
{
    for (;;)
    {
        Element_Begin1("loudnessInfoSetExtension");
        int32u bitSize;
        int8u loudnessInfoSetExtType, extSizeBits;
        Get_S1 (4, loudnessInfoSetExtType,                      "loudnessInfoSetExtType"); Element_Info1C(loudnessInfoSetExtType<UNIDRCLOUDEXT_Max, loudnessInfoSetExtType_IdNames[loudnessInfoSetExtType]);
        if (!loudnessInfoSetExtType) // UNIDRCLOUDEXT_TERM
        {
            Element_End0();
            break;
        }
        Get_S1 (4, extSizeBits,                                 "bitSizeLen");
        extSizeBits+=4;
        Get_S4 (extSizeBits, bitSize,                           "bitSize");
        bitSize++;
        if (bitSize>Data_BS_Remain())
        {
            Trusted_IsNot("Too big");
            Element_End0();
            break;
        }

        auto B=BS_Bookmark(bitSize);
        switch (loudnessInfoSetExtType)
        {
            case UNIDRCLOUDEXT_EQ                               : loudnessInfoSet(true); break;
            default:
                Skip_BS(bitSize,                                "Unknown");
        }
        BS_Bookmark(B, loudnessInfoSetExtType<UNIDRCLOUDEXT_Max?string(uniDrcConfigExtType_ConfNames[loudnessInfoSetExtType]):("loudnessInfoSetExtType"+to_string(loudnessInfoSetExtType)));
        Element_End0();
    }
}

//---------------------------------------------------------------------------
void File_Usac::streamId()
{
    Element_Begin1("streamId");

    int16u streamIdentifier;
    Get_S2 (16, streamIdentifier,                               "streamIdentifier");

    if (!IsParsingRaw)
        Fill(Stream_Audio, 0,  "streamIdentifier", streamIdentifier, 10, true);
    Element_End0();
}

//***************************************************************************
// Elements - USAC - Frame
//***************************************************************************
#if MEDIAINFO_TRACE || MEDIAINFO_CONFORMANCE

//---------------------------------------------------------------------------
void File_Usac::UsacFrame(size_t BitsNotIncluded)
{
    #if MEDIAINFO_CONFORMANCE
        if (roll_distance_Values && roll_distance_FramePos && roll_distance_Values && !roll_distance_Values->empty())
        {
            if (find(roll_distance_FramePos->begin(), roll_distance_FramePos->end(), Frame_Count_NotParsedIncluded) != roll_distance_FramePos->end())
            {
                auto numPreRollFrames = (*roll_distance_Values)[0].roll_distance;
                if (numPreRollFrames <= 0)
                    Fill_Conformance("sgpd roll_distance", "roll_distance is " + to_string(numPreRollFrames) + " so <= 0");
                else
                    numPreRollFrames_Check(Conf, (int32u)numPreRollFrames, "Crosscheck Container+UsacFrame roll_distance");
            }
        }
    #endif

    Element_Begin1("UsacFrame");
    Element_Info1(Frame_Count_NotParsedIncluded==(int64u)-1? Frame_Count :Frame_Count_NotParsedIncluded);
    bool usacIndependencyFlag;
    Get_SB (usacIndependencyFlag,                               "usacIndependencyFlag");
    #if MEDIAINFO_CONFORMANCE
        F.numPreRollFrames = (int32u)-1;
        if (IsParsingRaw <= 1)
        {
            if (Immediate_FramePos && roll_distance_FramePos)
            {
                auto ContainerSaysImmediate = (Immediate_FramePos_IsPresent && !*Immediate_FramePos_IsPresent) || find(Immediate_FramePos->begin(), Immediate_FramePos->end(), Frame_Count_NotParsedIncluded) != Immediate_FramePos->end();
                auto ContainerSaysNonImmediate = find(roll_distance_FramePos->begin(), roll_distance_FramePos->end(), Frame_Count_NotParsedIncluded) != roll_distance_FramePos->end();
                if (usacIndependencyFlag)
                {
                    if (!ContainerSaysImmediate && !ContainerSaysNonImmediate)
                        Fill_Conformance("Crosscheck Container+UsacFrame usacIndependencyFlag", "usacIndependencyFlag is 1 but MP4 stts or MP4 sgpd_prol does not indicate this frame is independent");
                }
                else
                {
                    if (ContainerSaysImmediate)
                        Fill_Conformance("Crosscheck Container+UsacFrame usacIndependencyFlag", "usacIndependencyFlag is 0 but MP4 stts indicates this frame is an immediate playout frame (IPF)");
                    if (ContainerSaysNonImmediate)
                        Fill_Conformance("Crosscheck Container+UsacFrame usacIndependencyFlag", "usacIndependencyFlag is 0 but MP4 sgpd_prol indicates this frame is an independent frame (IF)");
                }
            }
            if (!Frame_Count_NotParsedIncluded && !usacIndependencyFlag)
                Fill_Conformance("Crosscheck Container+UsacFrame usacIndependencyFlag", "usacIndependencyFlag is 0 but this is the first frame in this stream so not decodable", bitset8(), Warning);
        }
        if (IsParsingRaw == 2) //If from AudioPreRoll() and first frame
        {
            if (!usacIndependencyFlag)
                Fill_Conformance("AudioPreRoll PreRollFrame usacIndependencyFlag", "usacIndependencyFlag is 0 for first UsacFrame inside AudioPreRoll");
        }
        Merge_Conformance();
    #endif
    if (!IsParsingRaw)
    {
        if (Conf.IsNotValid || !usacIndependencyFlag)
        {
            Skip_BS(Data_BS_Remain(),                           "Data");
            Element_End0();
            #if MEDIAINFO_CONFORMANCE
                if (IsParsingRaw <= 1)
                {
                    if (Immediate_FramePos && roll_distance_FramePos)
                    {
                        if (usacIndependencyFlag)
                        {
                            auto ContainerSaysImmediate = (Immediate_FramePos_IsPresent && !*Immediate_FramePos_IsPresent) || find(Immediate_FramePos->begin(), Immediate_FramePos->end(), Frame_Count_NotParsedIncluded) != Immediate_FramePos->end();
                            auto ContainerSaysNonImmediate = find(roll_distance_FramePos->begin(), roll_distance_FramePos->end(), Frame_Count_NotParsedIncluded) != roll_distance_FramePos->end();
                            if (ContainerSaysImmediate && ContainerSaysNonImmediate)
                                Fill_Conformance("Crosscheck Container+UsacFrame usacIndependencyFlag", "usacIndependencyFlag is 1 and both MP4 stts and MP4 sgpd_prol indicate this frame is independent, only one place is permitted");
                        }
                    }
                }
                Merge_Conformance();
            #endif
            return;
        }
        IsParsingRaw++;
    }

    for (size_t elemIdx=0; elemIdx<C.usacElements.size(); elemIdx++)
    {
        F.NotImplemented=false;
        switch (C.usacElements[elemIdx].usacElementType&3)
        {
            case ID_USAC_SCE:
                UsacSingleChannelElement(usacIndependencyFlag);
                break;
            case ID_USAC_CPE:
                UsacChannelPairElement(usacIndependencyFlag);
                break;
            case ID_USAC_LFE:
                UsacLfeElement();
                break;
            case ID_USAC_EXT:
                UsacExtElement(elemIdx, usacIndependencyFlag);
                break;
            default:
                ; //Not parsed
        }
        if (!Trusted_Get() || F.NotImplemented)
            break;
    }

    auto BitsNotIncluded2=BitsNotIncluded!=(size_t)-1?BitsNotIncluded:0;
    if (F.NotImplemented && Data_BS_Remain()>BitsNotIncluded2)
        Skip_BS(Data_BS_Remain()-BitsNotIncluded2,              "(Not parsed)");

    if (BitsNotIncluded!=(size_t)-1)
    {
        if (Data_BS_Remain()>BitsNotIncluded)
        {
            int8u LastByte;
            auto BitsRemaining=Data_BS_Remain()-BitsNotIncluded;
            if (BitsRemaining<8)
            {
                //Peek_S1((int8u)BitsRemaining, LastByte);
                //#if MEDIAINFO_CONFORMANCE
                //    if (LastByte)
                //        Fill_Conformance((ConformanceFieldName+" Coherency").c_str(), "Padding bits are not 0, the bitstream may be malformed", bitset8(), Warning);
                //#endif
                LastByte=0;
            }
            else
            {
                #if MEDIAINFO_CONFORMANCE
                    Fill_Conformance(IsParsingRaw > 1 ? "AudioPreRoll UsacFrame Coherency" : "UsacFrame Coherency", "Extra bytes after the end of the syntax was reached", bitset8(), Warning);
                #endif
                LastByte=1;
            }
            Skip_BS(BitsRemaining,                              LastByte?"Unknown":"Padding");
        }
        else if (Data_BS_Remain()<BitsNotIncluded)
            Trusted_IsNot("Too big");
    }
    Element_End0();

    if (Trusted_Get())
    {
        #if MEDIAINFO_CONFORMANCE
            if (IsParsingRaw == 1)
                Merge_Conformance();
        #endif
    }
    else
    {
        #if MEDIAINFO_CONFORMANCE
            if (IsParsingRaw == 1)
            {
                Clear_Conformance();
                Fill_Conformance("UsacFrame Coherency", "Bitstream parsing ran out of data to read before the end of the syntax was reached, most probably the bitstream is malformed");
                Merge_Conformance();
            }
        #endif

        if (IsParsingRaw==1)
        {
            IsParsingRaw--;

            //Counting, TODO: remove duplicate of this code due to not executed in case of parsing issue
            Frame_Count++;
            if (Frame_Count_NotParsedIncluded!=(int64u)-1)
                Frame_Count_NotParsedIncluded++;
        }
    }

    #if MEDIAINFO_CONFORMANCE
        if (IsParsingRaw <= 1 && Frame_Count_NotParsedIncluded != (int64u)-1)
        {
            if (C.coreSbrFrameLengthIndex < coreSbrFrameLengthIndex_Mapping_Size && outputFrameLength && !outputFrameLength->empty())
            {
                size_t i = 0;
                int64u outputFrameLength_FrameTotal = 0;
                for (; i < outputFrameLength->size() && Frame_Count_NotParsedIncluded >= outputFrameLength_FrameTotal; i++)
                    outputFrameLength_FrameTotal += (*outputFrameLength)[i].SampleCount;
                if (i <= outputFrameLength->size() && Frame_Count_NotParsedIncluded < outputFrameLength_FrameTotal)
                {
                    auto SampleDuration_FromContainer = (*outputFrameLength)[i - 1].SampleDuration;
                    auto SampleDuration_FromStream = ((int32u)coreSbrFrameLengthIndex_Mapping[C.coreSbrFrameLengthIndex].outputFrameLengthDivided256) * 256;
                    bool IsLastFrame = i == outputFrameLength->size() && Frame_Count_NotParsedIncluded == outputFrameLength_FrameTotal - 1;
                    if (SampleDuration_FromContainer != SampleDuration_FromStream && (!IsLastFrame || SampleDuration_FromContainer > SampleDuration_FromStream))
                        Fill_Conformance("Crosscheck Container+UsacConfig outputFrameLength", ("MP4 stts value " + to_string(SampleDuration_FromContainer) + " does not match UsacConfig coreSbrFrameLengthIndex related value " + to_string(SampleDuration_FromStream)).c_str());
                }
            }
            if (roll_distance_FramePos && usacIndependencyFlag)
            {
                if (!Frame_Count_NotParsedIncluded && F.numPreRollFrames == (int32u)-1)
                {
                    auto ContainerSaysNonImmediate = find(roll_distance_FramePos->begin(), roll_distance_FramePos->end(), Frame_Count_NotParsedIncluded) != roll_distance_FramePos->end();
                    if (ContainerSaysNonImmediate && (!FirstOutputtedDecodedSample || !*FirstOutputtedDecodedSample))
                        Fill_Conformance("Crosscheck Container+UsacFrame AudioPreRoll", "AudioPreRoll is not present and roll_distance is not 0 but this is the first frame in this stream so without valid content", bitset8(), Warning);
                }
                if (Immediate_FramePos)
                {
                    auto ContainerSaysImmediate = (Immediate_FramePos_IsPresent && !*Immediate_FramePos_IsPresent) || find(Immediate_FramePos->begin(), Immediate_FramePos->end(), Frame_Count_NotParsedIncluded) != Immediate_FramePos->end();
                    auto ContainerSaysNonImmediate = find(roll_distance_FramePos->begin(), roll_distance_FramePos->end(), Frame_Count_NotParsedIncluded) != roll_distance_FramePos->end();
                    if (F.numPreRollFrames != (int32u)-1)
                    {
                        //Immediate
                        if (!ContainerSaysImmediate)
                            Fill_Conformance("Crosscheck Container+UsacFrame AudioPreRoll", "AudioPreRoll is present so this frame is an immediate playout frame (IPF) but MP4 stss does not indicate this frame as such");
                        if (ContainerSaysNonImmediate)
                            Fill_Conformance("Crosscheck Container+UsacFrame AudioPreRoll", "AudioPreRoll is present so this frame is an immediate playout frame (IPF) but MP4 sgpd_prol indicates this frame is an independent frame (IF)");
                    }
                    else
                    {
                        //NonImmediate
                        if (ContainerSaysImmediate)
                            Fill_Conformance("Crosscheck Container+UsacFrame AudioPreRoll", "AudioPreRoll is not present so this frame is an independent frame (IF) but MP4 stss indicates this frame is an immediate playout frame (IPF)");
                        if (!ContainerSaysNonImmediate)
                            Fill_Conformance("Crosscheck Container+UsacFrame AudioPreRoll", "AudioPreRoll is not present so this frame is an independent frame (IF) but MP4 sgpd_prol does not indicate this frame as such");
                    }
                }
            }
        }
        Merge_Conformance();
    #endif
}

//---------------------------------------------------------------------------
void File_Usac::UsacSingleChannelElement(bool usacIndependencyFlag)
{
    Element_Begin1("UsacSingleChannelElement");

    UsacCoreCoderData(1, usacIndependencyFlag);
    if (C.coreSbrFrameLengthIndex>=coreSbrFrameLengthIndex_Mapping_Size || coreSbrFrameLengthIndex_Mapping[C.coreSbrFrameLengthIndex].sbrRatioIndex)
        F.NotImplemented=true;

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::UsacChannelPairElement(bool usacIndependencyFlag)
{
    Element_Begin1("UsacChannelPairElement");

    UsacCoreCoderData(C.stereoConfigIndex==1?1:2, usacIndependencyFlag);
    if (C.coreSbrFrameLengthIndex>=coreSbrFrameLengthIndex_Mapping_Size || coreSbrFrameLengthIndex_Mapping[C.coreSbrFrameLengthIndex].sbrRatioIndex)
        F.NotImplemented=true;

    if (C.stereoConfigIndex)
    {
        F.NotImplemented=true;
        //TODO: Mps212Data(indepFlag);
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::twData()
{
    Element_Begin1("tw_data");

    TEST_SB_SKIP(                                               "tw_data_present");
        for (int8u node=0; node<16/*NUM_TW_NODES*/; node++)
            Skip_S1(3,                                          "tw_ratio");
    TEST_SB_END();

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::icsInfo()
{
    Element_Begin1("ics_info");

    int8u window_sequence, scale_factor_grouping;
    Get_S1 (2, window_sequence,                                 "window_sequence");
    Skip_SB(                                                    "window_shape");

    if (window_sequence==2)
    {
        Get_S1 (4, max_sfb,                                     "max_sfb");
        Get_S1 (7, scale_factor_grouping,                       "scale_factor_grouping");
    }
    else
    {
        Get_S1(6, max_sfb,                                      "max_sfb");
    }

    Element_End0();

    //Calculation of windows
    switch (window_sequence)
    {
        case 0 :    //ONLY_LONG_SEQUENCE
        case 1 :    //LONG_START_SEQUENCE
        case 3 :    //LONG_STOP_SEQUENCE
                    num_windows=1;
                    num_window_groups=1;
                    break;
        case 2 :    //EIGHT_SHORT_SEQUENCE
                    num_windows=8;
                    num_window_groups=1;
                    for (int8u win=0; win<num_windows-1; win++)
                    {
                        if (!(scale_factor_grouping&(1<<(6-win))))
                            num_window_groups++;
                    }
                    break;
        default:    ;
    }
}

//---------------------------------------------------------------------------
void File_Usac::scaleFactorData()
{
    Element_Begin1("scale_factor_data");

    #if MEDIAINFO_TRACE
        bool Trace_Activated_Save=Trace_Activated;
        Trace_Activated=false; //It is too big, disabling trace for now for full USAC parsing
    #endif //MEDIAINFO_TRACE

    for (int8u group=0; group<num_window_groups; group++)
    {
        for (int8u sfb=0; sfb<max_sfb; sfb++)
        {
            if (group || sfb)
                hcod_sf(                                        "hcod_sf[dpcm_sf[g][sfb]]");
        }
    }
        #if MEDIAINFO_TRACE
            Trace_Activated=Trace_Activated_Save;
        #endif //MEDIAINFO_TRACE

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::arithData(size_t ch, int16u N, int16u lg, int16u lg_max, bool arith_reset_flag)
{
    if (lg==0)
    {
        // set context
        memset(&C.arithContext[ch].q[1], 1, sizeof(C.arithContext[ch].q[1]));
        C.IsNotValid=true;
        F.NotImplemented=true;
        return;
    }
    else if (lg>1024 || N>4096) // Preserve arrays boundaries
    {
        Trusted_IsNot("lg too high");
        C.IsNotValid=true;
        F.NotImplemented=true;
        return;
    }

    Element_Begin1("arithData");

    // arith_map_context
    {
        if (arith_reset_flag || C.arithContext[ch].previous_window_size==(int16u)-1)
            memset(&C.arithContext[ch].q, 0, sizeof(C.arithContext[ch].q));
        else if (N != C.arithContext[ch].previous_window_size)
        {
            if (!N)
            {
                Trusted_IsNot("arith_map_context");
                C.IsNotValid=true;
                F.NotImplemented=true;
                Element_End0();
                return;
            }

            float ratio=((float)C.arithContext[ch].previous_window_size)/((float)N);
            if (ratio>1)
            {
                for (int pos=0; pos<N/4; pos++)
                    C.arithContext[ch].q[1][pos]=C.arithContext[ch].q[1][(int)((float)pos*ratio)];
            }
            else
            {
                for (int pos=N/4-1; pos>=0; pos--)
                    C.arithContext[ch].q[1][pos]=C.arithContext[ch].q[1][(int)((float)pos*ratio)];
            }

        }
        C.arithContext[ch].previous_window_size=N;
    }

    auto BS_Sav=*BS;
    int16u low=0, high=65535, value, offset;
    size_t TooMuch=0;
    if (Data_BS_Remain()>=16)
        Get_S2(16, value,                                           "initial arith_data");
    else
    {
        TooMuch=16-Data_BS_Remain();
        Get_S2(Data_BS_Remain(), value,                             "initial arith_data");
        value<<=TooMuch;
    }

    int16u context=C.arithContext[ch].q[1][0] << 12;
    int32u state=0;
    vector<int32s> x_ac_dec(lg);

    for (offset=0; offset<lg/2; offset++)
    {
        Element_Begin1(to_string(offset).c_str());
        // arith_get_context
        {
            state=context>>8;
            state=state+((offset>=lg_max/2-1?0:C.arithContext[ch].q[1][offset+1])<<8);
            state=(state<<4)+(offset?C.arithContext[ch].q[1][offset-1]:0);

            context=state;
            if (offset>3 && (C.arithContext[ch].q[1][offset-3]+C.arithContext[ch].q[1][offset-2]+C.arithContext[ch].q[1][offset-1])<5)
                state+=0x10000;
        }

        int32u lev=0, esc_nb=0, m, a, b;
        for (;;)
        {
            int16u pki=(int16u)-1;
            // arith_get_pk
            {
                size_t i_min=-1;
                size_t i_max=sizeof(arith_lookup_m)/sizeof(int16u)-1;
                size_t i=i_min;
                size_t j;
                while (i_max-i_min>1)
                {
                    i=i_min+((i_max-i_min)/2);
                    j=arith_hash_m[i];
                    if (state+(esc_nb<<17)<(j>>8))
                        i_max=i;
                    else if (state+(esc_nb<<17)>(j>>8))
                        i_min=i;
                    else
                    {
                        pki=j&0xFF;
                        break;
                    }
                }
                if (pki==(int16u)-1)
                    pki=arith_lookup_m[i_max];
            }

            Element_Begin1("acod_m");
            m=arith_decode(low, high, value, arith_cf_m[pki], sizeof(arith_cf_m[pki])/sizeof(int16u), &TooMuch);
            Element_End0();

            if (m<ARITH_ESCAPE)
                break;

            if ((esc_nb=++lev)>7)
                esc_nb=7;

            if (lev>23)
            {
                Trusted_IsNot("lev too high");
                C.IsNotValid=true;
                F.NotImplemented=true;
                Element_End0();
                Element_End0();
                return;
            }
        }

        b=m>>2;
        a=m-(b<<2);

        if (m==0)
        {
            if (lev) // ARITH_STOP symbol detected
            {
                Element_End0();
                break;
            }

            C.arithContext[ch].q[1][offset]=((a+b+1)<=0x000F?(a+b+1):0x000F); // arith_update_context
            Element_End0();
            continue;
        }

        for (int32u l=lev; l>0; l--)
        {
            Element_Begin1("acod_l");
            int8u lsbidx=(a==0)?1:((b==0)?0:2);
            int32u r=arith_decode(low, high, value, arith_cf_r[lsbidx], sizeof(arith_cf_r[lsbidx])/sizeof(int16u), &TooMuch);

            a=(a<<1)|(r&1);
            b=(b<<1)|((r>>1)&1);
            Element_End0();
        }

        x_ac_dec[2*offset]=a;
        x_ac_dec[2*offset+1]=b;
        C.arithContext[ch].q[1][offset]=((a+b+1)<=0x000F?(a+b+1):0x000F); // arith_update_context
        Element_End0();
    }

     // arith_finish
     if (offset<sizeof(C.arithContext[ch].q[1]))
         memset(&C.arithContext[ch].q[1][offset], 1, sizeof(C.arithContext[ch].q[1])-offset);

     //Hacky way to rewind by 14
     auto BS_Diff=BS_Sav.Remain()-BS->Remain();
     size_t ToRewind=14-TooMuch;
     if (BS_Diff>=ToRewind)
         BS_Diff-=ToRewind;
     *BS=BS_Sav;
     BS->Skip(BS_Diff);

     for (int16u i=0; i<lg; i++)
     {
         if (x_ac_dec[i]!=0)
         {
             bool s;
             Get_SB (s,                                         "s"); Param_Info1(i/2);
             if (!s)
                 x_ac_dec[i]*=-1;
         }
     }

     Element_End0();
 }

//---------------------------------------------------------------------------
void File_Usac::acSpectralData(size_t ch, bool usacIndependencyFlag)
{
    Element_Begin1("ac_spectral_data");

    #if MEDIAINFO_TRACE
        bool Trace_Activated_Save=Trace_Activated;
        Trace_Activated=false; //It is too big, disabling trace for now for full USAC parsing
    #endif //MEDIAINFO_TRACE

    bool arith_reset_flag=true;
    if (!usacIndependencyFlag)
        Get_SB (arith_reset_flag,                               "arith_reset_flag");

    if (C.IsNotValid || !Aac_sampling_frequency[C.sampling_frequency_index])
    {
        #if MEDIAINFO_TRACE
            Trace_Activated=Trace_Activated_Save;
        #endif //MEDIAINFO_TRACE

        C.IsNotValid=true;
        F.NotImplemented=true;
        Element_End0();
        return;
    }

    int16u N=0, lg=0;
    int8u sampling_frequency_index_swb;
    if (coreSbrFrameLengthIndex_Mapping[C.coreSbrFrameLengthIndex].coreCoderFrameLength==768 || coreSbrFrameLengthIndex_Mapping[C.coreSbrFrameLengthIndex].sbrRatioIndex)
    {
        float64 sampling_frequency_swb=Aac_sampling_frequency[C.sampling_frequency_index];
        if (coreSbrFrameLengthIndex_Mapping[C.coreSbrFrameLengthIndex].coreCoderFrameLength==768)
        {
            //From specs: "For a transform length of 768 samples, the same 1024-based scalefactor band tables are used, but those corresponding to 4 / 3 * sampling frequency."
            sampling_frequency_swb*=4;
            sampling_frequency_swb/=3;
        }
        if (coreSbrFrameLengthIndex_Mapping[C.coreSbrFrameLengthIndex].sbrRatioIndex)
        {
            sampling_frequency_swb*=coreSbrFrameLengthIndex_Mapping[C.coreSbrFrameLengthIndex].coreCoderFrameLength/256;
            sampling_frequency_swb/=coreSbrFrameLengthIndex_Mapping[C.coreSbrFrameLengthIndex].outputFrameLengthDivided256;
        }
        sampling_frequency_index_swb=Aac_AudioSpecificConfig_sampling_frequency_index(float64_int64s(sampling_frequency_swb));
    }
    else
        sampling_frequency_index_swb=C.sampling_frequency_index;
    if (sampling_frequency_index_swb>=13)
    {
        #if MEDIAINFO_TRACE
            Trace_Activated=Trace_Activated_Save;
        #endif //MEDIAINFO_TRACE

        C.IsNotValid=true;
        F.NotImplemented=true;
        Element_End0();
        return;
    }

    if (num_windows==1)
    {
        if (C.coreSbrFrameLengthIndex<coreSbrFrameLengthIndex_Mapping_Size)
            N=2*coreSbrFrameLengthIndex_Mapping[C.coreSbrFrameLengthIndex].coreCoderFrameLength;

        if (max_sfb<=Aac_swb_offset_long_window[sampling_frequency_index_swb]->num_swb)
            lg=Aac_swb_offset_long_window[sampling_frequency_index_swb]->swb_offset[max_sfb];
    }
    else
    {
        if (C.coreSbrFrameLengthIndex<coreSbrFrameLengthIndex_Mapping_Size)
            N=2*coreSbrFrameLengthIndex_Mapping[C.coreSbrFrameLengthIndex].coreCoderFrameLength/8;

        if (max_sfb<Aac_swb_offset_short_window[sampling_frequency_index_swb]->num_swb)
            lg=Aac_swb_offset_short_window[sampling_frequency_index_swb]->swb_offset[max_sfb];
    }
    int16u lg_max=N/2;
    if (lg>lg_max)
        lg=lg_max;

    for (int8u win=0; win<num_windows; win++)
    {
        arithData(ch, N, lg, lg_max, arith_reset_flag&&(win==0));
        if (F.NotImplemented)
            break;
    }

    #if MEDIAINFO_TRACE
        Trace_Activated=Trace_Activated_Save;
    #endif //MEDIAINFO_TRACE

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::tnsData()
{
    Element_Begin1("tns_data");

    #if MEDIAINFO_TRACE
        bool Trace_Activated_Save=Trace_Activated;
        Trace_Activated=false; //It is too big, disabling trace for now for full USAC parsing
    #endif //MEDIAINFO_TRACE

    for (int8u win=0; win<num_windows; win++)
    {
        int8u n_filt, coef_res;
        if (num_windows==1)
            Get_S1 (2, n_filt,                                  "n_filt[w]");
        else
            Get_S1 (1, n_filt,                                  "n_filt[w]");

        if (n_filt)
            Get_S1 (1, coef_res,                                "coef_res[w]");

        for (int8u filt=0; filt<n_filt; filt++)
        {
            int8u order;

            if (num_windows==1)
            {
                Skip_S1(6,                                       "lenght[w][filt]");
                Get_S1 (4, order,                                "order[w][filt]");
            }
            else
            {
                Skip_S1(4,                                       "lenght[w][filt]");
                Get_S1 (3, order,                                "order[w][filt]");
            }

            if (order)
            {
                int8u coef_compress;
                Skip_SB(                                        "direction[w][filt]");
                Get_S1 (1, coef_compress,                       "coef_compress[w][filt]");

                for (int8u i=0; i<order; i++)
                    Skip_S1(coef_res+3-coef_compress,           "coef[w][filt][i]");
            }
        }
    }

    #if MEDIAINFO_TRACE
        Trace_Activated=Trace_Activated_Save;
    #endif //MEDIAINFO_TRACE

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::fdChannelStream(size_t ch, bool commonWindow, bool commonTw, bool tnsDataPresent, bool usacIndependencyFlag)
{
    Element_Begin1("fd_channel_stream");

    Skip_S1(8,                                                  "global_gain");
    if (C.noiseFilling)
    {
        Skip_S1(3,                                              "noise_level");
        Skip_S1(5,                                              "noise_offset");
    }

    if (!commonWindow)
        icsInfo();

    if (C.tw_mdct && !commonTw)
        twData();

    scaleFactorData();

    if (tnsDataPresent)
        tnsData();

    acSpectralData(ch, usacIndependencyFlag);
    if (F.NotImplemented)
    {
        Element_End0();
        return;
    }

    TEST_SB_SKIP(                                               "fac_data_present");
        F.NotImplemented=true;
        //TODO: facData(true, coreSbrFrameLengthIndex_Mapping[C.coreSbrFrameLengthIndex].coreCoderFrameLength/(num_windows==1?8:16));
    TEST_SB_END();

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::cplxPredData(int8u max_sfb_ste, bool usacIndependencyFlag)
{
    Element_Begin1("cplx_pred_data");

    map<int8u, map<int8u, bool>> cplx_pred_used;
    bool complex_coef;

    TESTELSE_SB_SKIP(                                           "cplx_pred_all");
        for (int8u group=0; group<num_window_groups; group++)
            for (int8u sfb=0; sfb<max_sfb_ste; sfb++)
                cplx_pred_used[group][sfb]=true;
    TESTELSE_SB_ELSE(                                           "cplx_pred_all");
        for (int8u group=0; group<num_window_groups; group++)
            for (int8u sfb=0; sfb<max_sfb_ste; sfb+=2)
            {
                Get_SB(cplx_pred_used[group][sfb],              "cplx_pred_used[group][sfb]");
                if ((sfb+1)<max_sfb_ste)
                    cplx_pred_used[group][sfb+1]=cplx_pred_used[group][sfb];
            }
    TESTELSE_SB_END();

    Skip_SB(                                                    "pred_dir");

    TEST_SB_GET(complex_coef,                                   "complex_coef");
        if (!usacIndependencyFlag)
            Skip_SB(                                            "use_prev_frame");
    TEST_SB_END();

    if (!usacIndependencyFlag)
        Skip_SB(                                                "delta_code_time");


    for (int8u group=0; group<num_window_groups; group++)
        for (int8u sfb=0; sfb<max_sfb_ste; sfb+=2)
        {
            if (cplx_pred_used[group][sfb])
            {
                hcod_sf(                                        "dpcm_alpha_q_re[g][sfb]");
                if (complex_coef)
                    hcod_sf(                                    "dpcm_alpha_q_im[g][sfb]");
            }
        }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::StereoCoreToolInfo(bool& tns_data_present0, bool& tns_data_present1, bool core_mode0, bool core_mode1, bool usacIndependencyFlag)
{
    Element_Begin1("StereoCoreToolInfo");

    int8u max_sfb_ste, ms_mask_present;
    bool tns_active;

    if (!core_mode0 && !core_mode1)
    {
        Get_SB(tns_active,                                      "tns_active");
        TEST_SB_GET(C.common_window,                            "common_window");
            icsInfo();
            max_sfb_ste=max_sfb;
            TESTELSE_SB_SKIP(                                   "common_max_sfb");
                max_sfb1=max_sfb;
            TESTELSE_SB_ELSE(                                   "common_max_sfb");
                if (num_windows==1)
                    Get_S1(6, max_sfb1,                         "max_sfb1");
                else //EIGHT_SHORT_SEQUENCE
                    Get_S1(4, max_sfb1,                         "max_sfb1");
                if(max_sfb1>max_sfb)
                    max_sfb_ste=max_sfb1;
            TESTELSE_SB_END();
            Get_S1(2, ms_mask_present,                          "ms_mask_present");
            if (ms_mask_present==1)
            {
                for (int8u group=0; group<num_window_groups; group++)
                    for (int8u sfb=0; sfb<max_sfb_ste; sfb++)
                        Skip_SB(                                "ms_used[g][sfb]");
            }
            else if (ms_mask_present==3 && C.stereoConfigIndex==0)
                cplxPredData(max_sfb_ste, usacIndependencyFlag);
        TEST_SB_END();
        if (C.tw_mdct)
        {
            TEST_SB_GET(C.common_tw,                            "common_tw");
                twData();
            TEST_SB_END();
        }

        if (tns_active)
        {
            bool common_tns=false;
            if (C.common_window)
                Get_SB(common_tns,                              "common_tns");
            Skip_SB(                                            "tns_on_lr");

            if (common_tns)
            {
                tnsData();
                tns_data_present0=false;
                tns_data_present1=false;
            }
            else
            {
                TESTELSE_SB_SKIP(                               "tns_present_both");
                    tns_data_present0=true;
                    tns_data_present1=true;
                TESTELSE_SB_ELSE(                               "tns_present_both");
                    Get_SB(tns_data_present1,                   "tns_data_present[1]");
                    tns_data_present0=!tns_data_present1;
                TESTELSE_SB_END();
            }
        }
        else
        {
            tns_data_present0=0;
            tns_data_present1=0;
        }
    }
    else
    {
        C.common_window=false;
        C.common_tw=false;
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::UsacCoreCoderData(size_t nrChannels, bool usacIndependencyFlag)
{
    Element_Begin1("UsacCoreCoderData");

    bool coreModes[2];
    bool tnsDataPresent[2];

    for (size_t ch=0; ch<nrChannels; ch++)
        Get_SB(coreModes[ch],                                   "core_mode");

    if (nrChannels==2)
        StereoCoreToolInfo(tnsDataPresent[0], tnsDataPresent[1], coreModes[0], coreModes[1], usacIndependencyFlag);

    for (size_t ch=0; ch<nrChannels; ch++)
    {
        if (coreModes[ch])
        {
            C.IsNotValid=true;
            F.NotImplemented=true;
            //TODO: lpd_channel_stream(indepFlag);
        }
        else
        {
            if (nrChannels==1 || coreModes[0]!=coreModes[1])
                Get_SB(tnsDataPresent[ch],                      "tns_data_present");

            fdChannelStream(ch, C.common_window, C.common_tw, tnsDataPresent[ch], usacIndependencyFlag);
        }
        if (F.NotImplemented)
            break;
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::UsacLfeElement()
{
    Element_Begin1("UsacLfeElement");
        F.NotImplemented=true;
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::UsacExtElement(size_t elemIdx, bool usacIndependencyFlag)
{
    Element_Begin1("UsacExtElement");
    int8u usacExtElementType;
    usacExtElementType=Conf.usacElements[elemIdx].usacElementType>>2;
    Element_Info1C(usacExtElementType<ID_EXT_ELE_Max, usacExtElementType_IdNames[usacExtElementType]);
    bool usacExtElementPresent;
    Get_SB (usacExtElementPresent,                              "usacExtElementPresent");
    if (usacExtElementPresent)
    {
        int32u usacExtElementPayloadLength;
        bool usacExtElementUseDefaultLength;;
        Get_SB (usacExtElementUseDefaultLength,                 "usacExtElementUseDefaultLength");
        if (usacExtElementUseDefaultLength)
        {
            #if MEDIAINFO_CONFORMANCE
                if (usacExtElementType == ID_EXT_ELE_AUDIOPREROLL)
                    Fill_Conformance("AudioPreRoll usacExtElementUseDefaultLength", "usacExtElementUseDefaultLength is 1 but only value 0 is allowed");
            #endif
            usacExtElementPayloadLength=Conf.usacElements[elemIdx].usacExtElementDefaultLength;
        }
        else
        {
            Get_S4 (8, usacExtElementPayloadLength,             "usacExtElementPayloadLength");
            if (usacExtElementPayloadLength==0xFF)
            {
                Get_S4 (16, usacExtElementPayloadLength,        "usacExtElementPayloadLength");
                usacExtElementPayloadLength+=255-2;
            }
        }
        if (Conf.usacElements[elemIdx].usacExtElementPayloadFrag)
        {
            Skip_SB(                                            "usacExtElementStart");
            Skip_SB(                                            "usacExtElementStop");
        }

        #if MEDIAINFO_CONFORMANCE
            if (usacExtElementType == ID_EXT_ELE_AUDIOPREROLL)
            {
                if (IsParsingRaw > 1)
                    Fill_Conformance("AudioPreRoll UsacFrame usacExtElementPresent", "usacExtElementPresent is 1 for AudioPreRoll inside AudioPreRoll");
                else if (!usacExtElementPayloadLength)
                {
                    F.numPreRollFrames = 0;
                    if (!Frame_Count && IsParsingRaw <= 1)
                        numPreRollFrames_Check(Conf, 0, "AudioPreRoll numPreRollFrames");
                }
            }
        #endif
        if (usacExtElementPayloadLength)
        {
            usacExtElementPayloadLength*=8;
            if (usacExtElementPayloadLength>Data_BS_Remain())
            {
                Trusted_IsNot("Too big");
                Element_End0();
                return;
            }
            auto B=BS_Bookmark(usacExtElementPayloadLength);
            switch (usacExtElementType)
            {
                case ID_EXT_ELE_AUDIOPREROLL                    : AudioPreRoll(); break;
                default:
                    Skip_BS(usacExtElementPayloadLength,        "Unknown");
            }
            BS_Bookmark(B, usacExtElementType<ID_EXT_ELE_Max?string(usacExtElementType_Names[usacExtElementType]):("usacExtElementType"+to_string(usacExtElementType)));
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::AudioPreRoll()
{
    Element_Begin1("AudioPreRoll");
    int32u configLen;
    escapedValue(configLen, 4, 4, 8,                            "configLen");
    if (configLen)
    {
        configLen*=8;
        if (configLen>Data_BS_Remain())
        {
            Trusted_IsNot("Too big");
            Element_End0();
            return;
        }
        if (IsParsingRaw<=1)
        {
            Element_Begin1("Config");
            auto B=BS_Bookmark(configLen);
            UsacConfig(B.BitsNotIncluded);
            if (!Trusted_Get())
                C=Conf; //Using default conf if new conf has a problem
            BS_Bookmark(B, "AudioPreRoll UsacConfig");
            Element_End0();
        }
        else
        {
            //No nested in nested
            Skip_BS(configLen,                                  "Config");
        }
    }
    else
    {
        if (IsParsingRaw <= 1)
            C = Conf; //Using default conf if there is no replacing conf
        #if MEDIAINFO_CONFORMANCE
            if (IsParsingRaw <= 1)
                Fill_Conformance("AudioPreRoll configLen", "configLen is 0 but preroll config shall not be empty", bitset8(), Warning);
        #endif
    }
    Skip_SB(                                                    "applyCrossfade");
    Skip_SB(                                                    "reserved");
    escapedValue(F.numPreRollFrames, 2, 4, 0,                   "numPreRollFrames");
    #if MEDIAINFO_CONFORMANCE
        numPreRollFrames_Check(C, F.numPreRollFrames, "AudioPreRoll numPreRollFrames");
    #endif
    for (int32u frameIdx=0; frameIdx<F.numPreRollFrames; frameIdx++)
    {
        Element_Begin1("PreRollFrame");
            int32u auLen;
            escapedValue(auLen, 16, 16, 0,                      "auLen");
            auLen*=8;
            if (auLen)
            {
                if (auLen>Data_BS_Remain())
                {
                    Trusted_IsNot("Too big");
                    C=Conf; //Using default conf if new conf has a problem
                    Element_End0();
                    break;
                }
                if (IsParsingRaw<=1)
                {
                    auto FSav=F;
                    IsParsingRaw+=frameIdx+1;
                    Element_Begin1("AccessUnit");
                    auto B=BS_Bookmark(auLen);
                    UsacFrame(B.BitsNotIncluded);
                    BS_Bookmark(B, "AudioPreRoll UsacFrame");
                    Element_End0();
                    IsParsingRaw-=frameIdx+1;
                    F=FSav;
                }
                else
                {
                    //No nested in nested
                    Skip_BS(auLen,                              "AccessUnit");
                }
            }
            else
            {
                #if MEDIAINFO_CONFORMANCE
                    Fill_Conformance("AudioPreRoll auLen", "auLen is 0 but preroll frame shall not be empty");
                #endif
            }
        Element_End0();
    }
    if (!Trusted_Get())
        C=Conf; //Using default conf if new conf has a problem
    Element_End0();
}

//---------------------------------------------------------------------------
#endif //MEDIAINFO_TRACE || MEDIAINFO_CONFORMANCE

//***************************************************************************
// Utils
//***************************************************************************

//---------------------------------------------------------------------------
void File_Usac::escapedValue(int32u &Value, int8u nBits1, int8u nBits2, int8u nBits3, const char* Name)
{
    Element_Begin1(Name);
    Get_S4(nBits1, Value,                                       "nBits1");
    if (Value==((1<<nBits1)-1))
    {
        int32u ValueAdd;
        Get_S4(nBits2, ValueAdd,                                "nBits2");
        Value+=ValueAdd;
        if (nBits3 && ValueAdd==((1<<nBits2)-1))
        {
            Get_S4(nBits3, ValueAdd,                            "nBits3");
            Value+=ValueAdd;
        }
    }
    Element_Info1(Value);
    Element_End0();
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //defined(MEDIAINFO_AAC_YES) || defined(MEDIAINFO_MPEGH3DA_YES)

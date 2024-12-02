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
#include "MediaInfo/TimeCode.h" // For bitset8
#include "ZenLib/Conf.h"
#include <algorithm>
#include <iterator>
#include <limits>
using namespace ZenLib;
using namespace std;
//---------------------------------------------------------------------------

//***************************************************************************
// Constants
//***************************************************************************

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_AVC_YES) || defined(MEDIAINFO_MPEGPS_YES) || defined(MEDIAINFO_MPEGTS_YES) || defined(MEDIAINFO_MPEG7_YES)
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
static constexpr const char* Avc_profile_idc_Names[]= // Time sorted list
{
    "Baseline",                         // 2003
    "Main",
    "Extended",
    "High",                             // 2005
    "High 10",
    "High 4:2:2",
    "High 4:4:4",
    "High 10 Intra",                    // 2007
    "High 4:2:2 Intra",
    "High 4:4:4 Intra",
    "CAVLC 4:4:4 Intra",
    "High 4:4:4 Predictive",
    "Scalable Baseline",                // 2007-11
    "Scalable High",
    "Scalable High Intra",
    "Constrained Baseline",             // 2009
    "Multiview High",
    "Stereo High",                      // 2010
    "Progressive High",                 // 2011
    "Constrained High",                 // 2012
    "Scalable Constrained Baseline",
    "Scalable Constrained High",
    "Multiview Depth High",             // 2013
    "MFC High",                         // 2014
    "Enhanced Multiview Depth High",
    "MFC Depth High",                   // 2016
    "Progressive High 10",              // 2017
};
const char* Avc_profile_idc_Name(size_t Index)
{
    return Avc_profile_idc_Names[Index];
}
static constexpr int8u Avc_profile_idc_Mapping[][2]= // profile_idc sorted list
{
    {  44,  10 },   //  44, CAVLC 4:4:4 Intra (2007)
    {  66,   0 },   //  66, Baseline (2003)
    {  77,   1 },   //  77, Main (2003)
    {  83,  12 },   //  83, Scalable Baseline (2007-11)
    {  86,  13 },   //  86, Scalable High (2007-11)
    {  88,   2 },   //  88, Extended (2003)
    { 100,   3 },   // 100, High (2005)
    { 110,   4 },   // 110, High 10 (2005)
    { 118,  16 },   // 118, Multiview High (2009)
    { 122,   5 },   // 122, High 4:2:2 (2005)
    { 128,  17 },   // 128, Stereo High (2010)
    { 134,  23 },   // 134, MFC High (2014)
    { 135,  25 },   // 135, MFC Depth High (2016)
    { 138,  22 },   // 138, Multiview Depth High (2013)
    { 139,  24 },   // 139, Enhanced Multiview Depth High (2014)
    { 144,  16 },   // 144, High 4:4:4 (2005)
    { 244,  11 },   // 244, High 4:4:4 Predictive (2007)
};
static inline constexpr bool Avc_profile_idc_Names_Size_Cmp(const int8u first[2], const int8u second)
{
    return first[0]<second;
}
static constexpr const int8u Avc_level_idc_Names[]= // Time sorted list
{
    0x10,
    0x11,
    0x12,
    0x13,
    0x20,
    0x21,
    0x22,
    0x30,
    0x31,
    0x32,
    0x40,
    0x41,
    0x42,
    0x50,
    0x51,
    0x09,
    0x52,
    0x60,
    0x61,
    0x62,
};
string Avc_level_idc_Name(size_t Index)
{
    auto Level=Avc_level_idc_Names[Index];
    if (Level==9)
        return "1b";
    string LevelS;
    LevelS+='0'+(Level>>4);
    Level&=0xF;
    if (Level)
    {
        LevelS+='.';
        LevelS+='0'+Level;
    }
    return LevelS;
}
string Avc_profile_level_string(int8u profile_idc, int8u level_idc=0, int8u constraint_set_flags=0)
{
    string Profile;
    bitset8 constraint_setsB(constraint_set_flags);

    //Profile
    if (profile_idc)
    {
        size_t Profile_Pos=(size_t)-1;
        if (constraint_setsB[6]) // constraint_set1_flag
            switch (profile_idc)
            {
                case  66 : Profile_Pos= 15; break; // Constrained Baseline (2009)
            }
        if (constraint_setsB[4]) // constraint_set3_flag
            switch (profile_idc)
            {
                case  86 : Profile_Pos= 14; break; // Scalable High Intra (2007-11)
                case 110 : Profile_Pos=  7; break; // High 10 Intra (2007)
                case 122 : Profile_Pos=  8; break; // High 4:2:2 Intra (2007)
                case 244 : Profile_Pos=  9; break; // High 4:4:4 Intra (2007)
            }
        if (constraint_setsB[3]) // constraint_set4_flag
        {
            if (constraint_setsB[2]) // constraint_set5_flag
                switch (profile_idc)
                {
                    case  83 : Profile_Pos= 20; break; // Scalable Constrained Baseline (2012)
                    case  86 : Profile_Pos= 21; break; // Scalable Constrained High (2012)
                    case 100 : Profile_Pos= 19; break; // Constrained High (2012)
                }
            else
                switch (profile_idc)
                {
                    case 100 : Profile_Pos= 18; break; // Progressive High (2011)
                    case 110 : Profile_Pos= 26; break; // Progressive High 10 (2017)
                }
        }
        if (Profile_Pos==(size_t)-1)
        {
            auto Pos=lower_bound(begin(Avc_profile_idc_Mapping), end(Avc_profile_idc_Mapping), profile_idc, Avc_profile_idc_Names_Size_Cmp);
            if (Pos!=end(Avc_profile_idc_Mapping))
                Profile_Pos=(*Pos)[1];
        }
        if (Profile_Pos==(size_t)-1)
            Profile=to_string(profile_idc);
        else
            Profile=Avc_profile_idc_Names[Profile_Pos];
    }

    //Level
    if (level_idc)
    {
        auto level_idc_level=level_idc/10;
        auto level_idc_sub=level_idc%10;
        if (!Profile.empty())
            Profile+='@';
        Profile+='L';
        bool Is1b=false;
        switch (level_idc)
        {
            case  9:
                    Is1b=true;
                    break;
            case 11:
                if (constraint_setsB[4]) // constraint_set3_flag
                    switch (profile_idc)
                    {
                        case  66 : // Baseline
                        case  77 : // Main
                        case  88 : // Extended
                                    Is1b=true;      
                                    break;
                    }
                break;
        }
        if (Is1b)
            Profile+="1b";
        else
        {
            if (level_idc_level>=10)
            {
                auto level_idc_level_10=level_idc_level/10;
                auto level_idc_level_01=level_idc_level%10;
                level_idc_level=level_idc_level_01;
                Profile+='0'+level_idc_level_10;
            }
            Profile+='0'+level_idc_level;
            if (level_idc_sub && level_idc_sub<10)
            {
                Profile+='.';
                Profile+='0'+level_idc_sub;
            }
        }
    }
    return Profile;
}
size_t Avc_profile_level_Indexes(const string& ProfileLevelS) // Note: 1-based, 0 means not found, 0xPPLL form
{
    auto LevelPos=ProfileLevelS.find('@');
    size_t ProfileLevel;
    string ProfileS;

    // Level
    if (LevelPos!=string::npos)
    {
        if (ProfileLevelS.size()-LevelPos>2 && ProfileLevelS[LevelPos+1]=='L' && ProfileLevelS[LevelPos+2]>='0' && ProfileLevelS[LevelPos+2]<='9')
        {
            int8u Level=ProfileLevelS[LevelPos+2];
            if (Level=='1' && ProfileLevelS.size()-LevelPos==3 && ProfileLevelS[LevelPos+3]=='b')
                Level=9;
            else
            {
                Level-='0';
                Level<<=4;
                if (ProfileLevelS.size()-LevelPos>4 && ProfileLevelS[LevelPos+3]=='.' && ProfileLevelS[LevelPos+4]>='0' && ProfileLevelS[LevelPos+4]<='9')
                    Level+=ProfileLevelS[LevelPos+4]-'0';
            }
            auto Pos=find(begin(Avc_level_idc_Names), end(Avc_level_idc_Names), Level);
            if (Pos==end(Avc_level_idc_Names))
                ProfileLevel=0;
            else
                ProfileLevel=distance(begin(Avc_level_idc_Names), Pos)+1;
        }
        else
        {
            ProfileLevel=0;
        }
        ProfileS=ProfileLevelS.substr(0, LevelPos);
    }
    else
    {
        ProfileLevel=0;
        ProfileS=ProfileLevelS;
    }

    // Profile
    auto Pos=find(begin(Avc_profile_idc_Names), end(Avc_profile_idc_Names), ProfileS);
    if (Pos!=end(Avc_profile_idc_Names))
        ProfileLevel|=(distance(begin(Avc_profile_idc_Names), Pos)+1)<<8;
    return ProfileLevel;
}

//---------------------------------------------------------------------------
} //NameSpace

//---------------------------------------------------------------------------
#endif //...
//---------------------------------------------------------------------------

//***************************************************************************
//
//***************************************************************************

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_AVC_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Video/File_Avc.h"
#if defined(MEDIAINFO_AFDBARDATA_YES)
    #include "MediaInfo/Video/File_AfdBarData.h"
#endif //defined(MEDIAINFO_AFDBARDATA_YES)
#if defined(MEDIAINFO_DTVCCTRANSPORT_YES)
    #include "MediaInfo/Text/File_DtvccTransport.h"
#endif //defined(MEDIAINFO_DTVCCTRANSPORT_YES)
#if MEDIAINFO_ADVANCED2
    #include "ThirdParty/base64/base64.h"
#endif //MEDIAINFO_ADVANCED2
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#if MEDIAINFO_EVENTS
    #include "MediaInfo/MediaInfo_Config_PerPackage.h"
    #include "MediaInfo/MediaInfo_Events.h"
    #include "MediaInfo/MediaInfo_Events_Internal.h"
#endif //MEDIAINFO_EVENTS
#include <cstring>
#include <algorithm>
using namespace std;
using namespace ZenLib;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
const size_t Avc_Errors_MaxCount=32;

//---------------------------------------------------------------------------
struct par
{
    int8u w;
    int8u h;
};
extern const par Avc_PixelAspectRatio[] =
{
    {   1,   1 },
    {  12,  11 },
    {  10,  11 },
    {  16,  11 },
    {  40,  33 },
    {  24,  11 },
    {  20,  11 },
    {  32,  11 },
    {  80,  33 },
    {  18,  11 },
    {  15,  11 },
    {  64,  33 },
    { 160,  99 },
    {   4,   3 },
    {   3,   2 },
    {   2,   1 },
};
extern const auto Avc_PixelAspectRatio_Size = sizeof(Avc_PixelAspectRatio) / sizeof(*Avc_PixelAspectRatio);

//---------------------------------------------------------------------------
const char* Avc_video_format[]=
{
    "Component",
    "PAL",
    "NTSC",
    "SECAM",
    "MAC",
    "",
    "Reserved",
    "Reserved",
};

//---------------------------------------------------------------------------
const char* Avc_video_full_range[]=
{
    "Limited",
    "Full",
};

//---------------------------------------------------------------------------
static const char* Avc_primary_pic_type[]=
{
    "I",
    "I, P",
    "I, P, B",
    "SI",
    "SI, SP",
    "I, SI",
    "I, SI, P, SP",
    "I, SI, P, SP, B",
};

//---------------------------------------------------------------------------
static const char* Avc_slice_type[]=
{
    "P",
    "B",
    "I",
    "SP",
    "SI",
    "P",
    "B",
    "I",
    "SP",
    "SI",
};

//---------------------------------------------------------------------------
const int8u Avc_pic_struct_Size=9;
static const char* Avc_pic_struct[]=
{
    "frame",
    "top field",
    "bottom field",
    "top field, bottom field",
    "bottom field, top field",
    "top field, bottom field, top field repeated",
    "bottom field, top field, bottom field repeated",
    "frame doubling",
    "frame tripling",
};

//---------------------------------------------------------------------------
static const int8u Avc_NumClockTS[]=
{
    1,
    1,
    1,
    2,
    2,
    3,
    3,
    2,
    3,
};

//---------------------------------------------------------------------------
static const char* Avc_ct_type[]=
{
    "Progressive",
    "Interlaced",
    "Unknown",
    "Reserved",
};

//---------------------------------------------------------------------------
static const int8u Avc_SubWidthC[]=
{
    1,
    2,
    2,
    1,
};

//---------------------------------------------------------------------------
static const int8u Avc_SubHeightC[]=
{
    1,
    2,
    1,
    1,
};

//---------------------------------------------------------------------------
const char* Mpegv_colour_primaries(int8u colour_primaries);
const char* Mpegv_transfer_characteristics(int8u transfer_characteristics);
const char* Mpegv_matrix_coefficients(int8u matrix_coefficients);
const char* Mpegv_matrix_coefficients_ColorSpace(int8u matrix_coefficients);

//---------------------------------------------------------------------------
static const char* Avc_ChromaSubsampling_format_idc(int8u chroma_format_idc)
{
    switch (chroma_format_idc)
    {
        case 1: return "4:2:0";
        case 2: return "4:2:2";
        case 3: return "4:4:4";
        default: return "";
    }
}

//---------------------------------------------------------------------------
static const char* Avc_ChromaSubsampling_format_idc_ColorSpace(int8u chroma_format_idc)
{
    switch (chroma_format_idc)
    {
        case 0: return "Y";
        case 1: return "YUV";
        case 2: return "YUV";
        default: return "";
    }
}

//---------------------------------------------------------------------------
const char* Avc_user_data_GA94_cc_type(int8u cc_type)
{
    switch (cc_type)
    {
        case  0 : return "CEA-608 line 21 field 1 closed captions"; //closed caption 3 if this is second field
        case  1 : return "CEA-608 line 21 field 2 closed captions"; //closed caption 4 if this is second field
        case  2 : return "DTVCC Channel Packet Data";
        case  3 : return "DTVCC Channel Packet Start";
        default : return "";
    }
}

//---------------------------------------------------------------------------
int32u Avc_MaxDpbMbs(int8u level)
{
    switch (level)
    {
        case  10 : return    1485;
        case  11 : return    3000;
        case  12 : return    6000;
        case  13 :
        case  20 : return   11880;
        case  21 : return   19800;
        case  22 : return   20250;
        case  30 : return   40500;
        case  31 : return  108000;
        case  32 : return  216000;
        case  40 :
        case  41 : return  245760;
        case  42 : return  522240;
        case  50 : return  589824;
        case  51 : return  983040;
        default  : return       0;
    }
}

//---------------------------------------------------------------------------
namespace AVC_Intra_Headers
{
    //720p50
    static const size_t __720p50__50_Size = 4 * 0x10 + 0xA;
    static const int8u  __720p50__50[__720p50__50_Size] = { 0x00, 0x00, 0x01, 0x67, 0x6E, 0x10, 0x20, 0xA6, 0xD4, 0x20, 0x32, 0x33, 0x0C, 0x71, 0x18, 0x88,
                                                            0x62, 0x10, 0x19, 0x19, 0x86, 0x38, 0x8C, 0x44, 0x30, 0x21, 0x02, 0x56, 0x4E, 0x6F, 0x37, 0xCD,
                                                            0xF9, 0xBF, 0x81, 0x6B, 0xF3, 0x7C, 0xDE, 0x6E, 0x6C, 0xD3, 0x3C, 0x0F, 0x01, 0x6E, 0xFF, 0xC0,
                                                            0x00, 0xC0, 0x01, 0x38, 0xC0, 0x40, 0x40, 0x50, 0x00, 0x00, 0x03, 0x00, 0x10, 0x00, 0x00, 0x06,
                                                            0x48, 0x40, 0x00, 0x00, 0x01, 0x68, 0xEE, 0x31, 0x12, 0x11 };
    static const size_t __720p50_100_Size = 5 * 0x10 + 0x2;
    static const int8u  __720p50_100[__720p50_100_Size] = { 0x00, 0x00, 0x01, 0x67, 0x7A, 0x10, 0x29, 0xB6, 0xD4, 0x20, 0x2A, 0x33, 0x1D, 0xC7, 0x62, 0xA1,
                                                            0x08, 0x40, 0x54, 0x66, 0x3B, 0x8E, 0xC5, 0x42, 0x02, 0x10, 0x25, 0x64, 0x2C, 0x89, 0xE8, 0x85,
                                                            0xE4, 0x21, 0x4B, 0x90, 0x83, 0x06, 0x95, 0xD1, 0x06, 0x46, 0x97, 0x20, 0xC8, 0xD7, 0x43, 0x08,
                                                            0x11, 0xC2, 0x1E, 0x4C, 0x91, 0x0F, 0x01, 0x40, 0x16, 0xEC, 0x07, 0x8C, 0x04, 0x04, 0x05, 0x00,
                                                            0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x03, 0x00, 0x64, 0x84, 0x00, 0x00, 0x01, 0x68, 0xCE, 0x31,
                                                            0x12, 0x11 };
    //720p60
    static const size_t __720p60__50_Size = 4 * 0x10 + 0xB;
    static const int8u  __720p60__50[__720p60__50_Size] = { 0x00, 0x00, 0x01, 0x67, 0x6E, 0x10, 0x20, 0xA6, 0xD4, 0x20, 0x32, 0x33, 0x0C, 0x71, 0x18, 0x88,
                                                            0x62, 0x10, 0x19, 0x19, 0x86, 0x38, 0x8C, 0x44, 0x30, 0x21, 0x02, 0x56, 0x4E, 0x6F, 0x37, 0xCD,
                                                            0xF9, 0xBF, 0x81, 0x6B, 0xF3, 0x7C, 0xDE, 0x6E, 0x6C, 0xD3, 0x3C, 0x0F, 0x01, 0x6E, 0xFF, 0xC0,
                                                            0x00, 0xC0, 0x01, 0x38, 0xC0, 0x40, 0x40, 0x50, 0x00, 0x00, 0x3E, 0x90, 0x00, 0x1D, 0x4C, 0x08,
                                                            0x40, 0x00, 0x00, 0x00, 0x00, 0x01, 0x68, 0xEE, 0x31, 0x12, 0x11 };
    static const size_t __720p60_100_Size = 5 * 0x10 + 0x1;
    static const int8u  __720p60_100[__720p60_100_Size] = { 0x00, 0x00, 0x01, 0x67, 0x7A, 0x10, 0x29, 0xB6, 0xD4, 0x20, 0x2A, 0x33, 0x1D, 0xC7, 0x62, 0xA1,
                                                            0x08, 0x40, 0x54, 0x66, 0x3B, 0x8E, 0xC5, 0x42, 0x02, 0x10, 0x25, 0x64, 0x2C, 0x89, 0xE8, 0x85,
                                                            0xE4, 0x21, 0x4B, 0x90, 0x83, 0x06, 0x95, 0xD1, 0x06, 0x46, 0x97, 0x20, 0xC8, 0xD7, 0x43, 0x08,
                                                            0x11, 0xC2, 0x1E, 0x4C, 0x91, 0x0F, 0x01, 0x40, 0x16, 0xEC, 0x07, 0x8C, 0x04, 0x04, 0x05, 0x00,
                                                            0x00, 0x03, 0x03, 0xE9, 0x00, 0x01, 0xD4, 0xC0, 0x84, 0x00, 0x00, 0x01, 0x68, 0xCE, 0x31, 0x12,
                                                            0x11 };

    //1080i50
    static const size_t _1080i50__50_Size = 5 * 0x10 + 0xE;
    static const int8u  _1080i50__50[_1080i50__50_Size] = { 0x00, 0x00, 0x01, 0x67, 0x6E, 0x10, 0x28, 0xA6, 0xD4, 0x20, 0x32, 0x33, 0x0C, 0x71, 0x18, 0x88,
                                                            0x62, 0x10, 0x19, 0x19, 0x86, 0x38, 0x8C, 0x44, 0x30, 0x21, 0x02, 0x56, 0x4E, 0x6E, 0x61, 0x87,
                                                            0x3E, 0x73, 0x4D, 0x98, 0x0C, 0x03, 0x06, 0x9C, 0x0B, 0x73, 0xE6, 0xC0, 0xB5, 0x18, 0x63, 0x0D,
                                                            0x39, 0xE0, 0x5B, 0x02, 0xD4, 0xC6, 0x19, 0x1A, 0x79, 0x8C, 0x32, 0x34, 0x24, 0xF0, 0x16, 0x81,
                                                            0x13, 0xF7, 0xFF, 0x80, 0x01, 0x80, 0x02, 0x71, 0x80, 0x80, 0x80, 0xA0, 0x00, 0x00, 0x03, 0x00,
                                                            0x20, 0x00, 0x00, 0x06, 0x50, 0x80, 0x00, 0x00, 0x01, 0x68, 0xEE, 0x31, 0x12, 0x11 };
    static const size_t _1080i50_100_Size = 5 * 0x10 + 0xF;
    static const int8u  _1080i50_100[_1080i50_100_Size] = { 0x00, 0x00, 0x01, 0x67, 0x7A, 0x10, 0x29, 0xB6, 0xD4, 0x20, 0x22, 0x33, 0x19, 0xC6, 0x63, 0x23,
                                                            0x21, 0x01, 0x11, 0x98, 0xCE, 0x33, 0x19, 0x18, 0x21, 0x03, 0x3A, 0x46, 0x65, 0x6A, 0x65, 0x24,
                                                            0xAD, 0xE9, 0x12, 0x32, 0x14, 0x1A, 0x26, 0x34, 0xAD, 0xA4, 0x41, 0x82, 0x23, 0x01, 0x50, 0x2B,
                                                            0x1A, 0x24, 0x69, 0x48, 0x30, 0x40, 0x2E, 0x11, 0x12, 0x08, 0xC6, 0x8C, 0x04, 0x41, 0x28, 0x4C,
                                                            0x34, 0xF0, 0x1E, 0x01, 0x13, 0xF2, 0xE0, 0x3C, 0x60, 0x20, 0x20, 0x28, 0x00, 0x00, 0x03, 0x00,
                                                            0x08, 0x00, 0x00, 0x03, 0x01, 0x94, 0x20, 0x00, 0x00, 0x01, 0x68, 0xCE, 0x33, 0x48, 0xD0 };
    //1080i60
    static const size_t _1080i60__50_Size = 5 * 0x10 + 0xD;
    static const int8u  _1080i60__50[_1080i60__50_Size] = { 0x00, 0x00, 0x01, 0x67, 0x6E, 0x10, 0x28, 0xA6, 0xD4, 0x20, 0x32, 0x33, 0x0C, 0x71, 0x18, 0x88,
                                                            0x62, 0x10, 0x19, 0x19, 0x86, 0x38, 0x8C, 0x44, 0x30, 0x21, 0x02, 0x56, 0x4E, 0x6E, 0x61, 0x87,
                                                            0x3E, 0x73, 0x4D, 0x98, 0x0C, 0x03, 0x06, 0x9C, 0x0B, 0x73, 0xE6, 0xC0, 0xB5, 0x18, 0x63, 0x0D,
                                                            0x39, 0xE0, 0x5B, 0x02, 0xD4, 0xC6, 0x19, 0x1A, 0x79, 0x8C, 0x32, 0x34, 0x24, 0xF0, 0x16, 0x81,
                                                            0x13, 0xF7, 0xFF, 0x80, 0x01, 0x80, 0x02, 0x71, 0x80, 0x80, 0x80, 0xA0, 0x00, 0x00, 0x7D, 0x20,
                                                            0x00, 0x1D, 0x4C, 0x10, 0x80, 0x00, 0x00, 0x01, 0x68, 0xEE, 0x31, 0x12, 0x11 };
    static const size_t _1080i60_100_Size = 5 * 0x10 + 0xD;
    static const int8u  _1080i60_100[_1080i60_100_Size] = { 0x00, 0x00, 0x01, 0x67, 0x7A, 0x10, 0x29, 0xB6, 0xD4, 0x20, 0x22, 0x33, 0x19, 0xC6, 0x63, 0x23,
                                                            0x21, 0x01, 0x11, 0x98, 0xCE, 0x33, 0x19, 0x18, 0x21, 0x03, 0x3A, 0x46, 0x65, 0x6A, 0x65, 0x24,
                                                            0xAD, 0xE9, 0x12, 0x32, 0x14, 0x1A, 0x26, 0x34, 0xAD, 0xA4, 0x41, 0x82, 0x23, 0x01, 0x50, 0x2B,
                                                            0x1A, 0x24, 0x69, 0x48, 0x30, 0x40, 0x2E, 0x11, 0x12, 0x08, 0xC6, 0x8C, 0x04, 0x41, 0x28, 0x4C,
                                                            0x34, 0xF0, 0x1E, 0x01, 0x13, 0xF2, 0xE0, 0x3C, 0x60, 0x20, 0x20, 0x28, 0x00, 0x00, 0x1F, 0x48,
                                                            0x00, 0x07, 0x53, 0x04, 0x20, 0x00, 0x00, 0x01, 0x68, 0xCE, 0x33, 0x48, 0xD0 };

    //1080p50
    static const size_t _1080p50_100_Size = 4 * 0x10 + 0xA;
    static const int8u  _1080p50_100[_1080p50_100_Size] = { 0x00, 0x00, 0x01, 0x67, 0x7A, 0x10, 0x2A, 0xB6, 0xD4, 0x20, 0x22, 0x33, 0x19, 0xC6, 0x63, 0x23,
                                                            0x21, 0x01, 0x11, 0x98, 0xCE, 0x33, 0x19, 0x18, 0x21, 0x02, 0x56, 0xB9, 0x3D, 0x7D, 0x7E, 0x4F,
                                                            0xE3, 0x3F, 0x11, 0xF1, 0x9E, 0x08, 0xB8, 0x8C, 0x54, 0x43, 0xC0, 0x78, 0x02, 0x27, 0xE2, 0x70,
                                                            0x1E, 0x30, 0x10, 0x10, 0x14, 0x00, 0x00, 0x03, 0x00, 0x04, 0x00, 0x00, 0x03, 0x01, 0x92, 0x10,
                                                            0x00, 0x00, 0x00, 0x00, 0x01, 0x68, 0xCE, 0x33, 0x48, 0xD0 };
    //1080p60
    static const size_t _1080p60_100_Size = 4 * 0x10 + 0x8;
    static const int8u  _1080p60_100[_1080p60_100_Size] = { 0x00, 0x00, 0x01, 0x67, 0x7A, 0x10, 0x2A, 0xB6, 0xD4, 0x20, 0x22, 0x33, 0x19, 0xC6, 0x63, 0x23,
                                                            0x21, 0x01, 0x11, 0x98, 0xCE, 0x33, 0x19, 0x18, 0x21, 0x02, 0x56, 0xB9, 0x3D, 0x7D, 0x7E, 0x4F,
                                                            0xE3, 0x3F, 0x11, 0xF1, 0x9E, 0x08, 0xB8, 0x8C, 0x54, 0x43, 0xC0, 0x78, 0x02, 0x27, 0xE2, 0x70,
                                                            0x1E, 0x30, 0x10, 0x10, 0x14, 0x00, 0x00, 0x0F, 0xA4, 0x00, 0x07, 0x53, 0x02, 0x10, 0x00, 0x00,
                                                            0x00, 0x00, 0x01, 0x68, 0xCE, 0x33, 0x48, 0xD0 };
};


//---------------------------------------------------------------------------
// Some DV Metadata info: http://www.freepatentsonline.com/20050076039.pdf
static const char* MDPM(int8u ID)
{
    switch (ID)
    {
        case 0x18: return "Date/Time";                          // Is "Text header" in doc?
        case 0x19: return "Date/Time (continue 1)";             // Is "Text" in doc?
        case 0x70: return "Consumer Camera 1";
        case 0x71: return "Consumer Camera 2";
        case 0x73: return "Lens";
        case 0x7F: return "Camera shutter";
        case 0xA1: return "Iris (F)";
        case 0xE0: return "Make Model";
        case 0xE1: return "Rec Info";
        case 0xE4: return "Model Name";
        case 0xE5: return "Model Name (continue 1)";
        case 0xE6: return "Model Name (continue 1)";
        default  : return "";
    }
}

//---------------------------------------------------------------------------
static const char* MDPM_MakeName(int16u Value)
{
    switch (Value)
    {
        case 0x0103: return "Panasonic";
        case 0x0108: return "Sony";
        case 0x1011: return "Canon";
        case 0x1104: return "JVC";
        default    : return "";
    }
}

//---------------------------------------------------------------------------
// From DV
extern const char*  Dv_consumer_camera_1_ae_mode[];
extern const char*  Dv_consumer_camera_1_wb_mode[];
extern const char*  Dv_consumer_camera_1_white_balance(int8u white_balance);
extern const char*  Dv_consumer_camera_1_fcm[];

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Avc::File_Avc()
#if MEDIAINFO_DUPLICATE
:File__Duplicate()
#endif //MEDIAINFO_DUPLICATE
{
    //Config
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_Avc;
        StreamIDs_Width[0]=0;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
    MustSynchronize=true;
    PTS_DTS_Needed=true;
    StreamSource=IsStream;
    Frame_Count_NotParsedIncluded=0;

    //In
    Frame_Count_Valid=0;
    FrameIsAlwaysComplete=false;
    MustParse_SPS_PPS=false;
    SizedBlocks=false;

    //Temporal references
    TemporalReferences_DelayedElement=NULL;

    //Temp
    preferred_transfer_characteristics=2;

    //Text
    #if defined(MEDIAINFO_DTVCCTRANSPORT_YES)
        GA94_03_Parser=NULL;
    #endif //defined(MEDIAINFO_DTVCCTRANSPORT_YES)
}

//---------------------------------------------------------------------------
File_Avc::~File_Avc()
{
    Clean_Temp_References();
    #if defined(MEDIAINFO_DTVCCTRANSPORT_YES)
        delete GA94_03_Parser; //GA94_03_Parser=NULL;
    #endif //defined(MEDIAINFO_DTVCCTRANSPORT_YES)
     Clean_Seq_Parameter();
}

//---------------------------------------------------------------------------
void File_Avc::Clean_Temp_References()
{
    for (size_t Pos = 0; Pos<TemporalReferences.size(); Pos++)
        delete TemporalReferences[Pos]; //TemporalReferences[Pos]=NULL;
    TemporalReferences.clear();
}
//---------------------------------------------------------------------------
void File_Avc::Clean_Seq_Parameter()
{
    for (size_t Pos = 0; Pos<seq_parameter_sets.size(); Pos++)
        delete seq_parameter_sets[Pos]; //TemporalReferences[Pos]=NULL;
    seq_parameter_sets.clear();
    for (size_t Pos = 0; Pos<subset_seq_parameter_sets.size(); Pos++)
        delete subset_seq_parameter_sets[Pos]; //subset_seq_parameter_sets[Pos]=NULL;
    subset_seq_parameter_sets.clear();
    for (size_t Pos = 0; Pos<pic_parameter_sets.size(); Pos++)
        delete pic_parameter_sets[Pos]; //pic_parameter_sets[Pos]=NULL;
    pic_parameter_sets.clear();
}
//***************************************************************************
// AVC-Intra hardcoded headers
//***************************************************************************

//---------------------------------------------------------------------------
File_Avc::avcintra_header File_Avc::AVC_Intra_Headers_Data(int32u CodecID)
{
    switch (CodecID)
    {
        case  0x61693132: //ai12
        case  0x61693232: //ai22
                            return avcintra_header(AVC_Intra_Headers::_1080p50_100, AVC_Intra_Headers::_1080p50_100_Size);  // AVC-Intra 1080p50 class 100/200
        case  0x61693133: //ai13
        case  0x61693233: //ai23
                            return avcintra_header(AVC_Intra_Headers::_1080p60_100, AVC_Intra_Headers::_1080p60_100_Size);  // AVC-Intra 1080p60 class 100/200
        case  0x61693135: //ai15
        case  0x61693235: //ai25
                            return avcintra_header(AVC_Intra_Headers::_1080i50_100, AVC_Intra_Headers::_1080i50_100_Size);  // AVC-Intra 1080i50 class 100/200
        case  0x61693136: //ai16
        case  0x61693236: //ai26
                            return avcintra_header(AVC_Intra_Headers::_1080i60_100, AVC_Intra_Headers::_1080i60_100_Size);  // AVC-Intra 1080i60 class 100/200
        case  0x61693170: //ai1p
        case  0x61693270: //ai2p
                            return avcintra_header(AVC_Intra_Headers::__720p60_100, AVC_Intra_Headers::__720p60_100_Size);  // AVC-Intra  720p60 class 100/200
        case  0x61693171: //ai1q
        case  0x61693271: //ai2q
                            return avcintra_header(AVC_Intra_Headers::__720p50_100, AVC_Intra_Headers::__720p50_100_Size);  // AVC-Intra  720p50 class 100/200
      //case  0x61693532: //ai52
      //                    return avcintra_header(NULL, 0);                                                                // AVC-Intra 1080p25 class  50 (not supported)
      //case  0x61693533: //ai53
      //                    return avcintra_header(NULL, 0);                                                                // AVC-Intra 1080p30 class  50 (not supported)
        case  0x61693535: //ai55
                            return avcintra_header(AVC_Intra_Headers::_1080i50__50, AVC_Intra_Headers::_1080i50__50_Size);  // AVC-Intra 1080i50 class  50
        case  0x61693536: //ai56
                            return avcintra_header(AVC_Intra_Headers::_1080i60__50, AVC_Intra_Headers::_1080i60__50_Size);  // AVC-Intra 1080i60 class  50
        case  0x61693570: //ai5p
                            return avcintra_header(AVC_Intra_Headers::__720p60__50, AVC_Intra_Headers::__720p60__50_Size);  // AVC-Intra  720p60 class  50
        case  0x61693571: //ai5q
                            return avcintra_header(AVC_Intra_Headers::__720p50__50, AVC_Intra_Headers::__720p50__50_Size);  // AVC-Intra  720p50 class  50
        default       :
                            return avcintra_header(NULL, 0);
    }
}

//---------------------------------------------------------------------------
int32u File_Avc::AVC_Intra_CodecID_FromMeta(int32u Width, int32u Height, int32u Fields, int32u SampleDuration, int32u TimeScale, int32u SizePerFrame)
{
    // Computing bitrate
    int64u BitRate=SampleDuration?(((int64u)SizePerFrame)*8*TimeScale/SampleDuration):0;
    int64u SampleRate=SampleDuration?(float64_int64s(((float64)TimeScale)/SampleDuration)):0;
    int32u Class;
    switch (Width)
    {
    case 1920:
        Class=100;
        break;
    case 1440:
    case 1280:
    case 960:
        Class=50;
        break;
    default:
        if (!BitRate)
            return 0x4156696E; //AVin (neutral)
        Class=BitRate<=75000000?50:100; //Arbitrary choosen. TODO: check real maximumm bitrate, check class 200
        break;
    }
    switch (Class)
    {
        case 100 : 
                    switch (Height)
                    {
                        case 1088 :
                        case 1080 :
                                    switch (Fields)
                                    {
                                        case 1: 
                                                    switch (SampleRate)
                                                    {
                                                        case 50: return 0x61693132; //ai12
                                                        case 60: return 0x61693133; //ai13
                                                        default: return 0x4156696E; //AVin (neutral)
                                                    }
                                        case 2: 
                                                    switch (SampleRate)
                                                    {
                                                        case 25: return 0x61693135; //ai15 //TODO: check more files in order to know if it should be 1 or 2 fields per sample
                                                        case 30: return 0x61693136; //ai16 //TODO: check more files in order to know if it should be 1 or 2 fields per sample
                                                        case 50: return 0x61693135; //ai15 //TODO: check more files in order to know if it should be 1 or 2 fields per sample
                                                        case 60: return 0x61693136; //ai16 //TODO: check more files in order to know if it should be 1 or 2 fields per sample
                                                        default: return 0x4156696E; //AVin (neutral)
                                                    }
                                        default:    return 0x4156696E; //AVin (neutral)
                                    }
                        case  720 :
                                    switch (Fields)
                                    {
                                        case 1: 
                                                    switch (SampleRate)
                                                    {
                                                        case 50: return 0x61693170; //ai1p
                                                        case 60: return 0x61693171; //ai1q
                                                        default: return 0x4156696E; //AVin (neutral)
                                                    }
                                        default:    return 0x4156696E; //AVin (neutral)
                                    }
                        default   : return 0x4156696E; //AVin (neutral)
                    }
        case  50 : 
                    switch (Height)
                    {
                        case 1088 :
                        case 1080 :
                                    switch (Fields)
                                    {
                                        case 1: 
                                                    switch (SampleRate)
                                                    {
                                                        case 25: return 0x61693532; //ai52
                                                        case 30: return 0x61693533; //ai53
                                                        default: return 0x4156696E; //AVin (neutral)
                                                    }
                                        case 2: 
                                                    switch (SampleRate)
                                                    {
                                                        case 25: return 0x61693535; //ai55 //TODO: check more files in order to know if it should be 1 or 2 fields per sample
                                                        case 30: return 0x61693536; //ai56 //TODO: check more files in order to know if it should be 1 or 2 fields per sample
                                                        case 50: return 0x61693535; //ai55 //TODO: check more files in order to know if it should be 1 or 2 fields per sample
                                                        case 60: return 0x61693536; //ai56 //TODO: check more files in order to know if it should be 1 or 2 fields per sample
                                                        default: return 0x4156696E; //AVin (neutral)
                                                    }
                                        default:    return 0x4156696E; //AVin (neutral)
                                    }
                        case  720 :
                                    switch (Fields)
                                    {
                                        case 1: 
                                                    switch (SampleRate)
                                                    {
                                                        case 50: return 0x61693570; //ai5p
                                                        case 60: return 0x61693571; //ai5q
                                                        default: return 0x4156696E; //AVin (neutral)
                                                    }
                                        default:    return 0x4156696E; //AVin (neutral)
                                    }
                        default   : return 0x4156696E; //AVin (neutral)
                    }
        default: return 0x4156696E; //AVin (neutral)
    }
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Avc::Streams_Fill()
{
    for (std::vector<seq_parameter_set_struct*>::iterator seq_parameter_set_Item=seq_parameter_sets.begin(); seq_parameter_set_Item!=seq_parameter_sets.end(); ++seq_parameter_set_Item)
        if ((*seq_parameter_set_Item))
            Streams_Fill(seq_parameter_set_Item);
    for (std::vector<seq_parameter_set_struct*>::iterator subset_seq_parameter_set_Item=subset_seq_parameter_sets.begin(); subset_seq_parameter_set_Item!=subset_seq_parameter_sets.end(); ++subset_seq_parameter_set_Item)
        if ((*subset_seq_parameter_set_Item))
        {
            if (seq_parameter_sets.empty())
                Streams_Fill(subset_seq_parameter_set_Item);
            else
                Streams_Fill_subset(subset_seq_parameter_set_Item);
            Fill(Stream_Video, 0, Video_MultiView_Count, (*subset_seq_parameter_set_Item)->num_views_minus1+1);
        }

    #if MEDIAINFO_ADVANCED2
        for (size_t Pos = 0; Pos<Dump_SPS.size(); Pos++)
            Fill(Stream_Video, 0, "Dump_seq_parameter_set", Dump_SPS[Pos].c_str());
        for (size_t Pos = 0; Pos<Dump_PPS.size(); Pos++)
            Fill(Stream_Video, 0, "Dump_pic_parameter_set", Dump_PPS[Pos].c_str());
    #endif //MEDIAINFO_ADVANCED2
}

//---------------------------------------------------------------------------
void File_Avc::Streams_Fill(std::vector<seq_parameter_set_struct*>::iterator seq_parameter_set_Item_)
{
    auto seq_parameter_set_Item = *seq_parameter_set_Item_;

    if (Count_Get(Stream_Video) && !Retrieve_Const(Stream_Video, 0, Video_Format).empty())
        return;
    if (!Count_Get(Stream_Video))
        Stream_Prepare(Stream_Video);
    Fill(Stream_Video, 0, Video_Format, "AVC");
    Fill(Stream_Video, 0, Video_Codec, "AVC");

    //Calculating - Pixels
    int32u Width =(seq_parameter_set_Item->pic_width_in_mbs_minus1       +1)*16;
    int32u Height=(seq_parameter_set_Item->pic_height_in_map_units_minus1+1)*16*(2-seq_parameter_set_Item->frame_mbs_only_flag);
    int8u chromaArrayType = seq_parameter_set_Item->ChromaArrayType();
    if (chromaArrayType >= 4)
        chromaArrayType = 0;
    int32u CropUnitX=Avc_SubWidthC [chromaArrayType];
    int32u CropUnitY=Avc_SubHeightC[chromaArrayType]*(2-seq_parameter_set_Item->frame_mbs_only_flag);
    Width -=(seq_parameter_set_Item->frame_crop_left_offset+seq_parameter_set_Item->frame_crop_right_offset )*CropUnitX;
    Height-=(seq_parameter_set_Item->frame_crop_top_offset +seq_parameter_set_Item->frame_crop_bottom_offset)*CropUnitY;

    //From vui_parameters
    float64 FrameRate=0;
    const auto vui_parameters = seq_parameter_set_Item->vui_parameters;
    if (vui_parameters)
    {
        if (vui_parameters->flags[timing_info_present_flag])
        {
            if (!vui_parameters->flags[fixed_frame_rate_flag])
                Fill(Stream_Video, StreamPos_Last, Video_FrameRate_Mode, "VFR");
            else if (vui_parameters->time_scale && vui_parameters->num_units_in_tick)
            {
                FrameRate = (float32)vui_parameters->time_scale / vui_parameters->num_units_in_tick / (seq_parameter_set_Item->frame_mbs_only_flag ? 2 : ((seq_parameter_set_Item->pic_order_cnt_type == 2 && Structure_Frame / 2 > Structure_Field) ? 1 : 2)) / FrameRate_Divider;
                Fill(Stream_Video, StreamPos_Last, Video_FrameRate, FrameRate);
            }
        }

        if (vui_parameters->sar_width && vui_parameters->sar_height)
        {
            auto PixelAspectRatio = ((float32)vui_parameters->sar_width) / vui_parameters->sar_height;
            Fill(Stream_Video, 0, Video_PixelAspectRatio, PixelAspectRatio, 3, true);
            if (Width && Height)
                Fill(Stream_Video, 0, Video_DisplayAspectRatio, Width*PixelAspectRatio/Height, 3, true); //More precise
        }

        if (vui_parameters->chroma_sample_loc_type_top_field != (int32u)-1)
        {
            Fill(Stream_Video, 0, "ChromaSubsampling_Position", __T("Type ") + Ztring::ToZtring(vui_parameters->chroma_sample_loc_type_top_field));
            if (vui_parameters->chroma_sample_loc_type_bottom_field != (int32u)-1 && vui_parameters->chroma_sample_loc_type_bottom_field != vui_parameters->chroma_sample_loc_type_top_field)
                Fill(Stream_Video, 0, "ChromaSubsampling_Position", __T("Type ") + Ztring::ToZtring(vui_parameters->chroma_sample_loc_type_bottom_field));
        }

        //Colour description
        if (preferred_transfer_characteristics!=2)
            Fill(Stream_Video, 0, Video_transfer_characteristics, Mpegv_transfer_characteristics(preferred_transfer_characteristics));
        if (vui_parameters->flags[video_signal_type_present_flag])
        {
            Fill(Stream_Video, 0, Video_Standard, Avc_video_format[vui_parameters->video_format]);
            Fill(Stream_Video, 0, Video_colour_range, Avc_video_full_range[vui_parameters->flags[video_full_range_flag]]);
            if (vui_parameters->flags[colour_description_present_flag])
            {
                Fill(Stream_Video, 0, Video_colour_description_present, "Yes");
                Fill(Stream_Video, 0, Video_colour_primaries, Mpegv_colour_primaries(vui_parameters->colour_primaries));
                Fill(Stream_Video, 0, Video_transfer_characteristics, Mpegv_transfer_characteristics(vui_parameters->transfer_characteristics));
                Fill(Stream_Video, 0, Video_matrix_coefficients, Mpegv_matrix_coefficients(vui_parameters->matrix_coefficients));
                if (vui_parameters->matrix_coefficients!=2)
                    Fill(Stream_Video, 0, Video_ColorSpace, Mpegv_matrix_coefficients_ColorSpace(vui_parameters->matrix_coefficients), Unlimited, true, true);
            }
        }

        //hrd_parameter_sets
        int64u bit_rate_value=(int64u)-1;
        bool   bit_rate_value_IsValid=true;
        bool   cbr_flag=false;
        bool   cbr_flag_IsSet=false;
        bool   cbr_flag_IsValid=true;
        seq_parameter_set_struct::vui_parameters_struct::xxl* NAL=vui_parameters->NAL;
        if (NAL)
            for (size_t Pos=0; Pos<NAL->SchedSel.size(); Pos++)
            {
                if (NAL->SchedSel[Pos].cpb_size_value!=(int32u)-1)
                    Fill(Stream_Video, 0, Video_BufferSize, NAL->SchedSel[Pos].cpb_size_value);
                if (bit_rate_value!=(int64u)-1 && bit_rate_value!=NAL->SchedSel[Pos].bit_rate_value)
                    bit_rate_value_IsValid=false;
                if (bit_rate_value==(int64u)-1)
                    bit_rate_value=NAL->SchedSel[Pos].bit_rate_value;
                if (cbr_flag_IsSet==true && cbr_flag!=NAL->SchedSel[Pos].cbr_flag)
                    cbr_flag_IsValid=false;
                if (cbr_flag_IsSet==0)
                {
                    cbr_flag=NAL->SchedSel[Pos].cbr_flag;
                    cbr_flag_IsSet=true;
                }
            }
        seq_parameter_set_struct::vui_parameters_struct::xxl* VCL=vui_parameters->VCL;
        if (VCL)
            for (size_t Pos=0; Pos<VCL->SchedSel.size(); Pos++)
            {
                Fill(Stream_Video, 0, Video_BufferSize, VCL->SchedSel[Pos].cpb_size_value);
                if (bit_rate_value!=(int64u)-1 && bit_rate_value!=VCL->SchedSel[Pos].bit_rate_value)
                    bit_rate_value_IsValid=false;
                if (bit_rate_value==(int64u)-1)
                    bit_rate_value=VCL->SchedSel[Pos].bit_rate_value;
                if (cbr_flag_IsSet==true && cbr_flag!=VCL->SchedSel[Pos].cbr_flag)
                    cbr_flag_IsValid=false;
                if (cbr_flag_IsSet==0)
                {
                    cbr_flag=VCL->SchedSel[Pos].cbr_flag;
                    cbr_flag_IsSet=true;
                }
            }
        if (cbr_flag_IsSet && cbr_flag_IsValid)
        {
            Fill(Stream_Video, 0, Video_BitRate_Mode, cbr_flag?"CBR":"VBR");
            if (bit_rate_value!=(int64u)-1 && bit_rate_value_IsValid)
                Fill(Stream_Video, 0, cbr_flag?Video_BitRate_Nominal:Video_BitRate_Maximum, bit_rate_value);
        }
    }

    auto Profile=Avc_profile_level_string(seq_parameter_set_Item->profile_idc, seq_parameter_set_Item->level_idc, seq_parameter_set_Item->constraint_set_flags);
    Fill(Stream_Video, 0, Video_Format_Profile, Profile);
    Fill(Stream_Video, 0, Video_Codec_Profile, Profile);
    Fill(Stream_Video, StreamPos_Last, Video_Width, Width);
    Fill(Stream_Video, StreamPos_Last, Video_Height, Height);
    if (seq_parameter_set_Item->frame_crop_left_offset || seq_parameter_set_Item->frame_crop_right_offset)
        Fill(Stream_Video, StreamPos_Last, Video_Stored_Width, (seq_parameter_set_Item->pic_width_in_mbs_minus1       +1)*16);
    if (seq_parameter_set_Item->frame_crop_top_offset || seq_parameter_set_Item->frame_crop_bottom_offset)
        Fill(Stream_Video, StreamPos_Last, Video_Stored_Height, (seq_parameter_set_Item->pic_height_in_map_units_minus1+1)*16*(2-seq_parameter_set_Item->frame_mbs_only_flag));
    if (FrameRate_Divider==2)
    {
        Fill(Stream_Video, StreamPos_Last, Video_Format_Settings_FrameMode, "Frame doubling");
        Fill(Stream_Video, StreamPos_Last, Video_Format_Settings, "Frame doubling");
    }
    if (FrameRate_Divider==3)
    {
        Fill(Stream_Video, StreamPos_Last, Video_Format_Settings_FrameMode, "Frame tripling");
        Fill(Stream_Video, StreamPos_Last, Video_Format_Settings, "Frame tripling");
    }

    //Interlacement
    if (seq_parameter_set_Item->mb_adaptive_frame_field_flag && Structure_Frame>0) //Interlaced macro-block
    {
        Fill(Stream_Video, 0, Video_ScanType, "MBAFF");
        Fill(Stream_Video, 0, Video_Interlacement, "MBAFF");
    }
    else if (seq_parameter_set_Item->frame_mbs_only_flag || (Structure_Frame>0 && Structure_Field==0)) //No interlaced frame
    {
        switch (seq_parameter_set_Item->pic_struct_FirstDetected)
        {
            case  3 :
                        Fill(Stream_Video, 0, Video_ScanType, "Interlaced");
                        Fill(Stream_Video, 0, Video_Interlacement, "TFF");
                        Fill(Stream_Video, 0, Video_Format_Settings_PictureStructure, "Frame");
                        Fill(Stream_Video, 0, Video_ScanType_StoreMethod, "InterleavedFields");
                        break;
            case  4 :
                        Fill(Stream_Video, 0, Video_ScanType, "Interlaced");
                        Fill(Stream_Video, 0, Video_Interlacement, "BFF");
                        Fill(Stream_Video, 0, Video_Format_Settings_PictureStructure, "Frame");
                        Fill(Stream_Video, 0, Video_ScanType_StoreMethod, "InterleavedFields");
                        break;
            default :
                        Fill(Stream_Video, 0, Video_ScanType, "Progressive");
                        Fill(Stream_Video, 0, Video_Interlacement, "PPF");
        }
    }
    else if (Structure_Field>0)
    {
        Fill(Stream_Video, 0, Video_ScanType, "Interlaced");
        Fill(Stream_Video, 0, Video_Interlacement, "Interlaced");
    }
    std::string ScanOrders, PictureTypes(PictureTypes_PreviousFrames);
    ScanOrders.reserve(TemporalReferences.size());
    for (size_t Pos=0; Pos<TemporalReferences.size(); Pos++)
        if (TemporalReferences[Pos])
        {
            ScanOrders+=TemporalReferences[Pos]->IsTop?'T':'B';
            if ((Pos%2)==0)
                PictureTypes+=Avc_slice_type[TemporalReferences[Pos]->slice_type];
        }
        else if (!PictureTypes.empty()) //Only if stream already started
        {
            ScanOrders+=' ';
            if ((Pos%2)==0)
                PictureTypes+=' ';
        }
    Fill(Stream_Video, 0, Video_ScanOrder, ScanOrder_Detect(ScanOrders));
    { //Legacy
        string Result=ScanOrder_Detect(ScanOrders);
        if (!Result.empty())
        {
            Fill(Stream_Video, 0, Video_Interlacement, Result, true, true);
            Fill(Stream_Video, 0, Video_ScanType_StoreMethod, "SeparatedFields", Unlimited, true, true);
        }
        else
        {
            switch (seq_parameter_set_Item->pic_struct_FirstDetected)
            {
                case  1 :
                            Fill(Stream_Video, 0, Video_ScanOrder, (string) "TFF");
                            Fill(Stream_Video, 0, Video_ScanType_StoreMethod, "SeparatedFields", Unlimited, true, true);
                            break;
                case  2 :
                            Fill(Stream_Video, 0, Video_ScanOrder, (string) "BFF");
                            Fill(Stream_Video, 0, Video_ScanType_StoreMethod, "SeparatedFields", Unlimited, true, true);
                            break;
                case  3 :
                            Fill(Stream_Video, 0, Video_ScanOrder, (string) "TFF");
                            Fill(Stream_Video, 0, Video_ScanType_StoreMethod, "InterleavedFields", Unlimited, true, true);
                            break;
                case  4 :
                            Fill(Stream_Video, 0, Video_ScanOrder, (string) "BFF");
                            Fill(Stream_Video, 0, Video_ScanType_StoreMethod, "InterleavedFields", Unlimited, true, true);
                            break;
                default : ;
            }
        }
    }
    Fill(Stream_Video, 0, Video_Format_Settings_GOP, GOP_Detect(PictureTypes));
    if (Retrieve_Const(Stream_Video, 0, Video_BitRate).empty() && Retrieve_Const(Stream_Video, 0, Video_Format_Settings_GOP)==__T("N=1") && FrameRate && FrameSizes.size()==1)
    {
        Fill(Stream_Video, 0, Video_BitRate, FrameSizes.begin()->first*FrameRate*8, 0);
        Fill(Stream_Video, 0, Video_BitRate_Mode, "CBR");
    }

    Fill(Stream_General, 0, General_Encoded_Library, Encoded_Library);
    Fill(Stream_General, 0, General_Encoded_Library_Name, Encoded_Library_Name);
    Fill(Stream_General, 0, General_Encoded_Library_Version, Encoded_Library_Version);
    Fill(Stream_General, 0, General_Encoded_Library_Settings, Encoded_Library_Settings);
    Fill(Stream_Video, 0, Video_Encoded_Library, Encoded_Library);
    Fill(Stream_Video, 0, Video_Encoded_Library_Name, Encoded_Library_Name);
    Fill(Stream_Video, 0, Video_Encoded_Library_Version, Encoded_Library_Version);
    Fill(Stream_Video, 0, Video_Encoded_Library_Settings, Encoded_Library_Settings);
    if (Retrieve_Const(Stream_Video, 0, Video_BitRate_Nominal).empty()) // SPS has priority over other metadata
        Fill(Stream_Video, 0, Video_BitRate_Nominal, BitRate_Nominal);
    Fill(Stream_Video, 0, Video_MuxingMode, MuxingMode);
    for (std::vector<pic_parameter_set_struct*>::iterator pic_parameter_set_Item=pic_parameter_sets.begin(); pic_parameter_set_Item!=pic_parameter_sets.end(); ++pic_parameter_set_Item)
        if (*pic_parameter_set_Item && (*pic_parameter_set_Item)->seq_parameter_set_id==seq_parameter_set_Item_-(seq_parameter_sets.empty()?subset_seq_parameter_sets.begin():seq_parameter_sets.begin()))
        {
            if ((*pic_parameter_set_Item)->entropy_coding_mode_flag)
            {
                Fill(Stream_Video, 0, Video_Format_Settings, "CABAC");
                Fill(Stream_Video, 0, Video_Format_Settings_CABAC, "Yes");
                Fill(Stream_Video, 0, Video_Codec_Settings, "CABAC");
                Fill(Stream_Video, 0, Video_Codec_Settings_CABAC, "Yes");
            }
            else
            {
                Fill(Stream_Video, 0, Video_Format_Settings_CABAC, "No");
                Fill(Stream_Video, 0, Video_Codec_Settings_CABAC, "No");
            }
            break; //TODO: currently, testing only the first pic_parameter_set
        }
    if (seq_parameter_set_Item->max_num_ref_frames>0)
    {
        Fill(Stream_Video, 0, Video_Format_Settings, Ztring::ToZtring(seq_parameter_set_Item->max_num_ref_frames)+__T(" Ref Frames"));
        Fill(Stream_Video, 0, Video_Codec_Settings, Ztring::ToZtring(seq_parameter_set_Item->max_num_ref_frames)+__T(" Ref Frames"));
        Fill(Stream_Video, 0, Video_Format_Settings_RefFrames, seq_parameter_set_Item->max_num_ref_frames);
        Fill(Stream_Video, 0, Video_Codec_Settings_RefFrames, seq_parameter_set_Item->max_num_ref_frames);
    }
    if (Retrieve(Stream_Video, 0, Video_ColorSpace).empty())
        Fill(Stream_Video, 0, Video_ColorSpace, Avc_ChromaSubsampling_format_idc_ColorSpace(seq_parameter_set_Item->chroma_format_idc));
    Fill(Stream_Video, 0, Video_ChromaSubsampling, Avc_ChromaSubsampling_format_idc(seq_parameter_set_Item->chroma_format_idc));
    if (seq_parameter_set_Item->bit_depth_luma_minus8==seq_parameter_set_Item->bit_depth_chroma_minus8)
        Fill(Stream_Video, 0, Video_BitDepth, seq_parameter_set_Item->bit_depth_luma_minus8+8);
    if (MaxSlicesCount>1)
        Fill(Stream_Video, 0, Video_Format_Settings_SliceCount, MaxSlicesCount);

    hdr::iterator EtsiTs103433 = HDR.find(HdrFormat_EtsiTs103433);
    if (EtsiTs103433 != HDR.end())
    {
        for (std::map<video, Ztring>::iterator Item = EtsiTs103433->second.begin(); Item != EtsiTs103433->second.end(); ++Item)
        {
            Fill(Stream_Video, 0, Item->first, Item->second);
        }
    }
    hdr::iterator SmpteSt209440 = HDR.find(HdrFormat_SmpteSt209440);
    if (SmpteSt209440 != HDR.end())
    {
        for (std::map<video, Ztring>::iterator Item = SmpteSt209440->second.begin(); Item != SmpteSt209440->second.end(); ++Item)
        {
            switch (Item->first)
            {
            case Video_MasteringDisplay_ColorPrimaries:
            case Video_MasteringDisplay_Luminance:
                if (Retrieve_Const(Stream_Video, 0, Item->first) == Item->second)
                    break;
                // Fallthrough
            default:
                Fill(Stream_Video, 0, Item->first, Item->second);
            }
        }
    }
    hdr::iterator SmpteSt2086 = HDR.find(HdrFormat_SmpteSt2086);
    if (SmpteSt2086 != HDR.end())
    {
        for (std::map<video, Ztring>::iterator Item = SmpteSt2086->second.begin(); Item != SmpteSt2086->second.end(); ++Item)
        {
            bool Ignore;
            switch (Item->first)
            {
            case Video_HDR_Format:
                Ignore = !Retrieve_Const(Stream_Video, 0, Item->first).empty();
                break;
            case Video_MasteringDisplay_ColorPrimaries:
            case Video_MasteringDisplay_Luminance:
                Ignore = Retrieve_Const(Stream_Video, 0, Item->first) == Item->second;
                break;
            default:
                Ignore = false;
            }
            if (!Ignore)
                Fill(Stream_Video, 0, Item->first, Item->second);
        }
    }

    if (!maximum_content_light_level.empty())
        Fill(Stream_Video, 0, Video_MaxCLL, maximum_content_light_level);
    if (!maximum_frame_average_light_level.empty())
        Fill(Stream_Video, 0, Video_MaxFALL, maximum_frame_average_light_level);
}

//---------------------------------------------------------------------------
void File_Avc::Streams_Fill_subset(const std::vector<seq_parameter_set_struct*>::iterator seq_parameter_set_Item)
{
    auto Profile=Avc_profile_level_string((*seq_parameter_set_Item)->profile_idc, (*seq_parameter_set_Item)->level_idc, (*seq_parameter_set_Item)->constraint_set_flags);
    Ztring Profile_Base=Retrieve(Stream_Video, 0, Video_Format_Profile);
    Fill(Stream_Video, 0, Video_Format_Profile, Profile, true, true);
    if (!Profile_Base.empty())
        Fill(Stream_Video, 0, Video_Format_Profile, Profile_Base);
}

//---------------------------------------------------------------------------
void File_Avc::Streams_Finish()
{
    if (PTS_End!=(int64u)-1 && (IsSub || File_Offset+Buffer_Offset+Element_Size==File_Size))
    {
        if (PTS_End>PTS_Begin)
            Fill(Stream_Video, 0, Video_Duration, float64_int64s(((float64)(PTS_End-PTS_Begin))/1000000));
    }

    //GA94 captions
    #if defined(MEDIAINFO_DTVCCTRANSPORT_YES)
        if (GA94_03_Parser && GA94_03_Parser->Status[IsAccepted])
        {
            Clear(Stream_Text);

            Finish(GA94_03_Parser);
            Merge(*GA94_03_Parser);

            Ztring LawRating=GA94_03_Parser->Retrieve(Stream_General, 0, General_LawRating);
            if (!LawRating.empty())
                Fill(Stream_General, 0, General_LawRating, LawRating, true);
            Ztring Title=GA94_03_Parser->Retrieve(Stream_General, 0, General_Title);
            if (!Title.empty() && Retrieve(Stream_General, 0, General_Title).empty())
                Fill(Stream_General, 0, General_Title, Title);

            for (size_t Pos=0; Pos<Count_Get(Stream_Text); Pos++)
            {
                Ztring MuxingMode=Retrieve(Stream_Text, Pos, "MuxingMode");
                Fill(Stream_Text, Pos, "MuxingMode", __T("SCTE 128 / ")+MuxingMode, true);
            }
        }
    #endif //defined(MEDIAINFO_DTVCCTRANSPORT_YES)

    #if MEDIAINFO_IBIUSAGE
        if (seq_parameter_sets.size()==1 && (*seq_parameter_sets.begin())->vui_parameters && (*seq_parameter_sets.begin())->vui_parameters->time_scale && (*seq_parameter_sets.begin())->vui_parameters->fixed_frame_rate_flag)
            Ibi_Stream_Finish((*seq_parameter_sets.begin())->vui_parameters->time_scale, (*seq_parameter_sets.begin())->vui_parameters->num_units_in_tick);
    #endif //MEDIAINFO_IBIUSAGE
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Avc::FileHeader_Begin()
{
    if (!File__Analyze::FileHeader_Begin_0x000001())
        return false;

    if (!MustSynchronize)
    {
        Synched_Init();
        Buffer_TotalBytes_FirstSynched=0;
        File_Offset_FirstSynched=File_Offset;
    }

    //All should be OK
    return true;
}

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Avc::Synchronize()
{
    //Synchronizing
    size_t Buffer_Offset_Min=Buffer_Offset;
    while(Buffer_Offset+4<=Buffer_Size && (Buffer[Buffer_Offset  ]!=0x00
                                        || Buffer[Buffer_Offset+1]!=0x00
                                        || Buffer[Buffer_Offset+2]!=0x01))
    {
        Buffer_Offset+=2;
        while(Buffer_Offset<Buffer_Size && Buffer[Buffer_Offset]!=0x00)
            Buffer_Offset+=2;
        if (Buffer_Offset>=Buffer_Size || Buffer[Buffer_Offset-1]==0x00)
            Buffer_Offset--;
    }
    if (Buffer_Offset>Buffer_Offset_Min && Buffer[Buffer_Offset-1]==0x00)
        Buffer_Offset--;

    //Parsing last bytes if needed
    if (Buffer_Offset+4==Buffer_Size && (Buffer[Buffer_Offset  ]!=0x00
                                      || Buffer[Buffer_Offset+1]!=0x00
                                      || Buffer[Buffer_Offset+2]!=0x00
                                      || Buffer[Buffer_Offset+3]!=0x01))
        Buffer_Offset++;
    if (Buffer_Offset+3==Buffer_Size && (Buffer[Buffer_Offset  ]!=0x00
                                      || Buffer[Buffer_Offset+1]!=0x00
                                      || Buffer[Buffer_Offset+2]!=0x01))
        Buffer_Offset++;
    if (Buffer_Offset+2==Buffer_Size && (Buffer[Buffer_Offset  ]!=0x00
                                      || Buffer[Buffer_Offset+1]!=0x00))
        Buffer_Offset++;
    if (Buffer_Offset+1==Buffer_Size &&  Buffer[Buffer_Offset  ]!=0x00)
        Buffer_Offset++;

    if (Buffer_Offset+4>Buffer_Size)
        return false;

    //Synched is OK
    Synched=true;
    return true;
}

//---------------------------------------------------------------------------
bool File_Avc::Synched_Test()
{
    //Must have enough buffer for having header
    if (Buffer_Offset+6>Buffer_Size)
        return false;

    //Quick test of synchro
    if (Buffer[Buffer_Offset  ]!=0x00
     || Buffer[Buffer_Offset+1]!=0x00
     || (Buffer[Buffer_Offset+2]!=0x01 && (Buffer[Buffer_Offset+2]!=0x00 || Buffer[Buffer_Offset+3]!=0x01)))
    {
        Synched=false;
        return true;
    }

    //Quick search
    if (!Header_Parser_QuickSearch())
        return false;

    #if MEDIAINFO_IBIUSAGE
        bool zero_byte=Buffer[Buffer_Offset+2]==0x00;
        bool RandomAccess=(Buffer[Buffer_Offset+(zero_byte?4:3)]&0x1F)==0x07 || ((Buffer[Buffer_Offset+(zero_byte?4:3)]&0x1F)==0x09 && ((Buffer[Buffer_Offset+(zero_byte?5:4)]&0xE0)==0x00 || (Buffer[Buffer_Offset+(zero_byte?5:4)]&0xE0)==0xA0)); //seq_parameter_set or access_unit_delimiter with value=0 or 5 (3 bits)
        if (RandomAccess)
            Ibi_Add();
    #endif //MEDIAINFO_IBIUSAGE

    //We continue
    return true;
}

//***************************************************************************
// Buffer - Demux
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_DEMUX
void File_Avc::Data_Parse_Iso14496()
{
    if (Demux_Avc_Transcode_Iso14496_15_to_Iso14496_10)
    {
        if (Element_Code==0x07)
        {
            std::vector<seq_parameter_set_struct*>::iterator Data_Item=seq_parameter_sets.begin();
            if (Data_Item!=seq_parameter_sets.end() && (*Data_Item))
            {
                (*Data_Item)->Init_Iso14496_10(0x67, Buffer+Buffer_Offset, Element_Size);
            }
        }
        if (Element_Code==0x08)
        {
            std::vector<pic_parameter_set_struct*>::iterator Data_Item=pic_parameter_sets.begin();
            if (Data_Item!=pic_parameter_sets.end() && (*Data_Item))
            {
                (*Data_Item)->Init_Iso14496_10(0x68, Buffer+Buffer_Offset, Element_Size);
            }
        }
        if (Element_Code==0x0F)
        {
            std::vector<seq_parameter_set_struct*>::iterator Data_Item=subset_seq_parameter_sets.begin();
            if (Data_Item!=subset_seq_parameter_sets.end() && (*Data_Item))
            {
                SizeOfNALU_Minus1=0;
                (*Data_Item)->Init_Iso14496_10(0x6F, Buffer+Buffer_Offset, Element_Size);
            }
        }
    }
}

//---------------------------------------------------------------------------
bool File_Avc::Demux_UnpacketizeContainer_Test()
{
    const int8u*    Buffer_Temp=NULL;
    size_t          Buffer_Temp_Size=0;
    bool            RandomAccess=true; //Default, in case of problem

    if ((MustParse_SPS_PPS || SizedBlocks) && Demux_Avc_Transcode_Iso14496_15_to_Iso14496_10)
    {
        if (MustParse_SPS_PPS)
            return true; //Wait for SPS and PPS

        //Random access check
        RandomAccess=false;

        //Computing final size
        size_t TranscodedBuffer_Size=0;
        size_t Buffer_Offset_Save=Buffer_Offset;
        while (Buffer_Offset+SizeOfNALU_Minus1+1+1<=Buffer_Size)
        {
            size_t Size;
            if (Buffer_Offset+SizeOfNALU_Minus1>Buffer_Size)
            {
                Size=0;
                Buffer_Offset=Buffer_Size;
            }
            else
            switch (SizeOfNALU_Minus1)
            {
                case 0: Size=Buffer[Buffer_Offset];
                        TranscodedBuffer_Size+=2;
                        break;
                case 1: Size=BigEndian2int16u(Buffer+Buffer_Offset);
                        TranscodedBuffer_Size++;
                        break;
                case 2: Size=BigEndian2int24u(Buffer+Buffer_Offset);
                        break;
                case 3: Size=BigEndian2int32u(Buffer+Buffer_Offset);
                        TranscodedBuffer_Size--;
                        break;
                default:    return true; //Problem
            }
            Size+=SizeOfNALU_Minus1+1;

            //Coherency checking
            if (Size==0 || Buffer_Offset+Size>Buffer_Size || (Buffer_Offset+Size!=Buffer_Size && Buffer_Offset+Size+SizeOfNALU_Minus1+1>Buffer_Size))
                Size=Buffer_Size-Buffer_Offset;

            //Random access check
            if (!RandomAccess && Buffer_Offset+SizeOfNALU_Minus1+1<Buffer_Size && (Buffer[Buffer_Offset+SizeOfNALU_Minus1+1]&0x1F) && (Buffer[Buffer_Offset+SizeOfNALU_Minus1+1]&0x1F)<=5) //Is a slice
            {
                int32u slice_type;
                Element_Offset=SizeOfNALU_Minus1+1+1;
                Element_Size=Size;
                BS_Begin();
                Skip_UE("first_mb_in_slice");
                Get_UE (slice_type, "slice_type");
                BS_End();
                Element_Offset=0;

                switch (slice_type)
                {
                    case 2 :
                    case 7 :
                                RandomAccess=true;
                }
            }

            TranscodedBuffer_Size+=Size;
            Buffer_Offset+=Size;
        }
        Buffer_Offset=Buffer_Offset_Save;

        //Adding SPS/PPS sizes
        if (RandomAccess)
        {
            for (seq_parameter_set_structs::iterator Data_Item=seq_parameter_sets.begin(); Data_Item!=seq_parameter_sets.end(); ++Data_Item)
                TranscodedBuffer_Size+=(*Data_Item)->Iso14496_10_Buffer_Size;
            for (seq_parameter_set_structs::iterator Data_Item=subset_seq_parameter_sets.begin(); Data_Item!=subset_seq_parameter_sets.end(); ++Data_Item)
                TranscodedBuffer_Size+=(*Data_Item)->Iso14496_10_Buffer_Size;
            for (pic_parameter_set_structs::iterator Data_Item=pic_parameter_sets.begin(); Data_Item!=pic_parameter_sets.end(); ++Data_Item)
                TranscodedBuffer_Size+=(*Data_Item)->Iso14496_10_Buffer_Size;
        }

        //Copying
        int8u* TranscodedBuffer=new int8u[TranscodedBuffer_Size+100];
        size_t TranscodedBuffer_Pos=0;
        if (RandomAccess)
        {
            for (seq_parameter_set_structs::iterator Data_Item=seq_parameter_sets.begin(); Data_Item!=seq_parameter_sets.end(); ++Data_Item)
            {
                std::memcpy(TranscodedBuffer+TranscodedBuffer_Pos, (*Data_Item)->Iso14496_10_Buffer, (*Data_Item)->Iso14496_10_Buffer_Size);
                TranscodedBuffer_Pos+=(*Data_Item)->Iso14496_10_Buffer_Size;
            }
            for (seq_parameter_set_structs::iterator Data_Item=subset_seq_parameter_sets.begin(); Data_Item!=subset_seq_parameter_sets.end(); ++Data_Item)
            {
                std::memcpy(TranscodedBuffer+TranscodedBuffer_Pos, (*Data_Item)->Iso14496_10_Buffer, (*Data_Item)->Iso14496_10_Buffer_Size);
                TranscodedBuffer_Pos+=(*Data_Item)->Iso14496_10_Buffer_Size;
            }
            for (pic_parameter_set_structs::iterator Data_Item=pic_parameter_sets.begin(); Data_Item!=pic_parameter_sets.end(); ++Data_Item)
            {
                std::memcpy(TranscodedBuffer+TranscodedBuffer_Pos, (*Data_Item)->Iso14496_10_Buffer, (*Data_Item)->Iso14496_10_Buffer_Size);
                TranscodedBuffer_Pos+=(*Data_Item)->Iso14496_10_Buffer_Size;
            }
        }
        while (Buffer_Offset<Buffer_Size)
        {
            //Sync layer
            TranscodedBuffer[TranscodedBuffer_Pos]=0x00;
            TranscodedBuffer_Pos++;
            TranscodedBuffer[TranscodedBuffer_Pos]=0x00;
            TranscodedBuffer_Pos++;
            TranscodedBuffer[TranscodedBuffer_Pos]=0x01;
            TranscodedBuffer_Pos++;

            //Block
            size_t Size;
            switch (SizeOfNALU_Minus1)
            {
                case 0: Size=Buffer[Buffer_Offset];
                        Buffer_Offset++;
                        break;
                case 1: Size=BigEndian2int16u(Buffer+Buffer_Offset);
                        Buffer_Offset+=2;
                        break;
                case 2: Size=BigEndian2int24u(Buffer+Buffer_Offset);
                        Buffer_Offset+=3;
                        break;
                case 3: Size=BigEndian2int32u(Buffer+Buffer_Offset);
                        Buffer_Offset+=4;
                        break;
                default: //Problem
                        delete [] TranscodedBuffer;
                        return false;
            }

            //Coherency checking
            if (Size==0 || Buffer_Offset+Size>Buffer_Size || (Buffer_Offset+Size!=Buffer_Size && Buffer_Offset+Size+SizeOfNALU_Minus1+1>Buffer_Size))
                Size=Buffer_Size-Buffer_Offset;

            std::memcpy(TranscodedBuffer+TranscodedBuffer_Pos, Buffer+Buffer_Offset, Size);
            TranscodedBuffer_Pos+=Size;
            Buffer_Offset+=Size;
        }
        Buffer_Offset=0;

        Buffer_Temp=Buffer;
        Buffer=TranscodedBuffer;
        Buffer_Temp_Size=Buffer_Size;
        Buffer_Size=TranscodedBuffer_Size;
        Demux_Offset=Buffer_Size;
    }
    else
    {
        bool zero_byte=Buffer[Buffer_Offset+2]==0x00;
        if (!(((Buffer[Buffer_Offset+(zero_byte?4:3)]&0x1B)==0x01 && (Buffer[Buffer_Offset+(zero_byte?5:4)]&0x80)!=0x80)
           || (Buffer[Buffer_Offset+(zero_byte?4:3)]&0x1F)==0x0C))
        {
            if (Demux_Offset==0)
            {
                Demux_Offset=Buffer_Offset;
                Demux_IntermediateItemFound=false;
            }
            while (Demux_Offset+6<=Buffer_Size)
            {
                //Synchronizing
                while(Demux_Offset+6<=Buffer_Size && (Buffer[Demux_Offset  ]!=0x00
                                                   || Buffer[Demux_Offset+1]!=0x00
                                                   || Buffer[Demux_Offset+2]!=0x01))
                {
                    Demux_Offset+=2;
                    while(Demux_Offset<Buffer_Size && Buffer[Buffer_Offset]!=0x00)
                        Demux_Offset+=2;
                    if (Demux_Offset>=Buffer_Size || Buffer[Demux_Offset-1]==0x00)
                        Demux_Offset--;
                }

                if (Demux_Offset+6>Buffer_Size)
                {
                    if (Config->IsFinishing)
                        Demux_Offset=Buffer_Size;
                    break;
                }

                zero_byte=Buffer[Demux_Offset+2]==0x00;
                if (Demux_IntermediateItemFound)
                {
                    if (!(((Buffer[Demux_Offset+(zero_byte?4:3)]&0x1B)==0x01 && (Buffer[Demux_Offset+(zero_byte?5:4)]&0x80)!=0x80)
                        || (Buffer[Demux_Offset+(zero_byte?4:3)]&0x1F)==0x0C))
                        break;
                }
                else
                {
                    if ((Buffer[Demux_Offset+(zero_byte?4:3)]&0x1B)==0x01 && (Buffer[Demux_Offset+(zero_byte?5:4)]&0x80)==0x80)
                        Demux_IntermediateItemFound=true;
                }

                Demux_Offset++;
            }

            if (Demux_Offset+6>Buffer_Size && !FrameIsAlwaysComplete && !Config->IsFinishing)
                return false; //No complete frame

            if (Demux_Offset && Buffer[Demux_Offset-1]==0x00)
                Demux_Offset--;

            zero_byte=Buffer[Buffer_Offset+2]==0x00;
            size_t Buffer_Offset_Random=Buffer_Offset;
            if ((Buffer[Buffer_Offset_Random+(zero_byte?4:3)]&0x1F)==0x09)
            {
                Buffer_Offset_Random++;
                if (zero_byte)
                    Buffer_Offset_Random++;
                while(Buffer_Offset_Random+6<=Buffer_Size && (Buffer[Buffer_Offset_Random  ]!=0x00
                                                           || Buffer[Buffer_Offset_Random+1]!=0x00
                                                           || Buffer[Buffer_Offset_Random+2]!=0x01))
                    Buffer_Offset_Random++;
                zero_byte=Buffer[Buffer_Offset_Random+2]==0x00;
            }
            RandomAccess=Buffer_Offset_Random+6<=Buffer_Size && (Buffer[Buffer_Offset_Random+(zero_byte?4:3)]&0x1F)==0x07; //seq_parameter_set
        }
    }

    if (!Status[IsAccepted])
    {
        if (Config->Demux_EventWasSent)
            return false;
        File_Avc* MI=new File_Avc;
        Element_Code=(int64u)-1;
        Open_Buffer_Init(MI);
        #ifdef MEDIAINFO_EVENTS
            MediaInfo_Config_PerPackage* Config_PerPackage_Temp=MI->Config->Config_PerPackage;
            MI->Config->Config_PerPackage=NULL;
        #endif //MEDIAINFO_EVENTS
        Open_Buffer_Continue(MI, Buffer, Buffer_Size);
        #ifdef MEDIAINFO_EVENTS
            MI->Config->Config_PerPackage=Config_PerPackage_Temp;
        #endif //MEDIAINFO_EVENTS
        bool IsOk=MI->Status[IsAccepted];
        delete MI;
        if (!IsOk)
            return false;
    }

    if (IFrame_Count || RandomAccess)
    {
        bool Frame_Count_NotParsedIncluded_PlusOne=false;
        int64u PTS_Temp=FrameInfo.PTS;
        if (!IsSub)
            FrameInfo.PTS=(int64u)-1;
        if (Frame_Count_NotParsedIncluded!=(int64u)-1 && Interlaced_Top!=Interlaced_Bottom)
        {
            Frame_Count_NotParsedIncluded--;
            Frame_Count_NotParsedIncluded_PlusOne=true;
        }
        Demux_UnpacketizeContainer_Demux(RandomAccess);
        if (!IsSub)
            FrameInfo.PTS=PTS_Temp;
        if (Frame_Count_NotParsedIncluded_PlusOne)
            Frame_Count_NotParsedIncluded++;
    }
    else
        Demux_UnpacketizeContainer_Demux_Clear();

    if (Buffer_Temp)
    {
        Demux_TotalBytes-=Buffer_Size;
        Demux_TotalBytes+=Buffer_Temp_Size;
        delete[] Buffer;
        Buffer=Buffer_Temp;
        Buffer_Size=Buffer_Temp_Size;
    }

    return true;
}
#endif //MEDIAINFO_DEMUX

//---------------------------------------------------------------------------
void File_Avc::Synched_Init()
{
    if (!Frame_Count_Valid)
        Frame_Count_Valid=Config->ParseSpeed>=0.3?512:(IsSub?1:2);

    //FrameInfo
    PTS_End=0;
    if (!IsSub)
        FrameInfo.DTS=0; //No DTS in container
    DTS_Begin=FrameInfo.DTS;
    DTS_End=FrameInfo.DTS;

    //Temporal references
    TemporalReferences_DelayedElement=NULL;
    TemporalReferences_Min=0;
    TemporalReferences_Max=0;
    TemporalReferences_Reserved=0;
    TemporalReferences_Offset=0;
    TemporalReferences_Offset_pic_order_cnt_lsb_Last=0;
    TemporalReferences_pic_order_cnt_Min=numeric_limits<decltype(TemporalReferences_pic_order_cnt_Min)>::max();

    //Text
    #if defined(MEDIAINFO_DTVCCTRANSPORT_YES)
        GA94_03_IsPresent=false;
    #endif //defined(MEDIAINFO_DTVCCTRANSPORT_YES)

    //File specific
    SizeOfNALU_Minus1=(int8u)-1;

    //Status
    IFrame_Count=0;
    prevPicOrderCntMsb=0;
    prevPicOrderCntLsb=(int32u)-1;
    prevTopFieldOrderCnt=(int32u)-1;
    prevFrameNum=(int32u)-1;
    prevFrameNumOffset=(int32u)-1;

    //Count of a Packets
    Block_Count=0;
    Interlaced_Top=0;
    Interlaced_Bottom=0;
    Structure_Field=0;
    Structure_Frame=0;
    Slices_CountInThisFrame=0;
    MaxSlicesCount=0;

    //Temp
    FrameRate_Divider=1;
    FirstPFrameInGop_IsParsed=false;
    Config_IsRepeated=false;
    tc=0;
    pic_order_cnt_Displayed=numeric_limits<decltype(pic_order_cnt_Displayed)>::max();
    pic_order_cnt_Delta=0;

    //Default values
    Streams.resize(0x100);
    Streams[0x06].Searching_Payload=true; //sei
    Streams[0x07].Searching_Payload=true; //seq_parameter_set
    Streams[0x09].Searching_Payload=true; //access_unit_delimiter
    Streams[0x0F].Searching_Payload=true; //subset_seq_parameter_set
    for (int8u Pos=0xFF; Pos>=0xB9; Pos--)
        Streams[Pos].Searching_Payload=true; //Testing MPEG-PS

    //Options
    Option_Manage();

    //Specific cases
    #if MEDIAINFO_EVENTS
        if (Config->ParseUndecodableFrames_Get())
        {
            Accept(); //In some case, we must accept the stream very quickly and before the sequence header is detected
            Streams[0x01].Searching_Payload=true; //slice_header
            Streams[0x05].Searching_Payload=true; //slice_header
        }
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_DEMUX
        Demux_Avc_Transcode_Iso14496_15_to_Iso14496_10=Config->Demux_Avc_Transcode_Iso14496_15_to_Iso14496_10_Get();
    #endif //MEDIAINFO_DEMUX
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_ADVANCED2
void File_Avc::Read_Buffer_SegmentChange()
{
}
#endif //MEDIAINFO_ADVANCED2

//---------------------------------------------------------------------------
void File_Avc::Read_Buffer_Unsynched()
{
    //Temporal references
    Clean_Temp_References();
    delete TemporalReferences_DelayedElement; TemporalReferences_DelayedElement=NULL;
    TemporalReferences_Min=0;
    TemporalReferences_Max=0;
    TemporalReferences_Reserved=0;
    TemporalReferences_Offset=0;
    TemporalReferences_Offset_pic_order_cnt_lsb_Last=0;
    TemporalReferences_pic_order_cnt_Min=numeric_limits<decltype(TemporalReferences_pic_order_cnt_Min)>::max();
    pic_order_cnt_Displayed=numeric_limits<decltype(pic_order_cnt_Displayed)>::max();
    pic_order_cnt_Delta=0;
    Slices_CountInThisFrame=0;

    //Text
    #if defined(MEDIAINFO_DTVCCTRANSPORT_YES)
        if (GA94_03_Parser)
            GA94_03_Parser->Open_Buffer_Unsynch();
    #endif //defined(MEDIAINFO_DTVCCTRANSPORT_YES)

    //parameter_sets
    if (SizedBlocks || !Config_IsRepeated) //If sized blocks, it is not a broadcasted stream so SPS/PPS are only in container header, we must not disable them.
    {
        //Rebuilding immediatly TemporalReferences
        seq_parameter_set_structs* _seq_parameter_sets=!seq_parameter_sets.empty()?&seq_parameter_sets:&subset_seq_parameter_sets; //Some MVC streams have no seq_parameter_sets. TODO: better management of temporal references
        for (std::vector<seq_parameter_set_struct*>::iterator seq_parameter_set_Item=(*_seq_parameter_sets).begin(); seq_parameter_set_Item!=(*_seq_parameter_sets).end(); ++seq_parameter_set_Item)
            if ((*seq_parameter_set_Item))
            {
                size_t MaxNumber;
                switch ((*seq_parameter_set_Item)->pic_order_cnt_type)
                {
                    case 0 : MaxNumber=(*seq_parameter_set_Item)->MaxPicOrderCntLsb; break;
                    case 2 : MaxNumber=(*seq_parameter_set_Item)->MaxFrameNum*2; break;
                    default: Trusted_IsNot("Not supported"); return;
                }

                TemporalReferences.resize(4*MaxNumber);
                TemporalReferences_Reserved=MaxNumber;
            }
    }
    else
    {
        Clean_Seq_Parameter();
    }

    //Status
    Interlaced_Top=0;
    Interlaced_Bottom=0;
    prevPicOrderCntMsb=0;
    prevPicOrderCntLsb=(int32u)-1;
    prevTopFieldOrderCnt=(int32u)-1;
    prevFrameNum=(int32u)-1;
    prevFrameNumOffset=(int32u)-1;

    //Temp
    FrameRate_Divider=1;
    FirstPFrameInGop_IsParsed=false;
    tc=0;

    //Impossible to know TimeStamps now
    PTS_End=0;
    DTS_End=0;
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Avc::Header_Parse()
{
    //Specific case
    if (MustParse_SPS_PPS)
    {
        Header_Fill_Size(Element_Size);
        Header_Fill_Code((int64u)-1, "Specific");
        return;
    }

    //Parsing
    int8u nal_unit_type;
    if (!SizedBlocks)
    {
        if (Buffer[Buffer_Offset+2]==0x00)
            Skip_B1(                                            "zero_byte");
        Skip_B3(                                                "start_code_prefix_one_3bytes");
        BS_Begin();
        Mark_0 ();
        Get_S1 ( 2, nal_ref_idc,                                "nal_ref_idc");
        Get_S1 ( 5, nal_unit_type,                              "nal_unit_type");
        BS_End();
        if (!Header_Parser_Fill_Size())
        {
            Element_WaitForMoreData();
            return;
        }
    }
    else
    {
        int64u Size;
        switch (SizeOfNALU_Minus1)
        {
            case 0: {
                        int8u Size_;
                        Get_B1 (Size_,                          "size");
                        Size=Size_;
                    }
                    break;
            case 1: {
                        int16u Size_;
                        Get_B2 (Size_,                          "size");
                        Size=Size_;
                    }
                    break;
            case 2: {
                        int32u Size_;
                        Get_B3 (Size_,                          "size");
                        Size=Size_;
                    }
                    break;
            case 3: {
                        int32u Size_;
                        Get_B4 (Size_,                          "size");
                        Size=Size_;
                    }
                    break;
            default:    Trusted_IsNot("No size of NALU defined");
                        Header_Fill_Size(Buffer_Size-Buffer_Offset);
                        return;
        }
        if (Element_Size<(int64u)SizeOfNALU_Minus1+1+1 || Size>Element_Size-Element_Offset)
            return RanOutOfData("AVC");
        Header_Fill_Size(Element_Offset+Size);
        BS_Begin();
        Mark_0 ();
        Get_S1 ( 2, nal_ref_idc,                                "nal_ref_idc");
        Get_S1 ( 5, nal_unit_type,                              "nal_unit_type");
        BS_End();
    }

    //Filling
    #if MEDIAINFO_TRACE
        if (Trace_Activated)
            Header_Fill_Code(nal_unit_type, Ztring().From_CC1(nal_unit_type));
        else
    #endif //MEDIAINFO_TRACE
            Header_Fill_Code(nal_unit_type);
}

//---------------------------------------------------------------------------
bool File_Avc::Header_Parser_Fill_Size()
{
    //Look for next Sync word
    if (Buffer_Offset_Temp==0) //Buffer_Offset_Temp is not 0 if Header_Parse_Fill_Size() has already parsed first frames
        Buffer_Offset_Temp=Buffer_Offset+4;
    while (Buffer_Offset_Temp+5<=Buffer_Size
        && CC3(Buffer+Buffer_Offset_Temp)!=0x000001)
    {
        Buffer_Offset_Temp+=2;
        while(Buffer_Offset_Temp<Buffer_Size && Buffer[Buffer_Offset_Temp]!=0x00)
            Buffer_Offset_Temp+=2;
        if (Buffer_Offset_Temp>=Buffer_Size || Buffer[Buffer_Offset_Temp-1]==0x00)
            Buffer_Offset_Temp--;
    }

    //Must wait more data?
    if (Buffer_Offset_Temp+5>Buffer_Size)
    {
        if (FrameIsAlwaysComplete || Config->IsFinishing)
            Buffer_Offset_Temp=Buffer_Size; //We are sure that the next bytes are a start
        else
            return false;
    }

    if (!FrameIsAlwaysComplete && Buffer[Buffer_Offset_Temp-1]==0x00)
        Buffer_Offset_Temp--;

    //OK, we continue
    Header_Fill_Size(Buffer_Offset_Temp-Buffer_Offset);
    Buffer_Offset_Temp=0;
    return true;
}

//---------------------------------------------------------------------------
bool File_Avc::Header_Parser_QuickSearch()
{
    while (       Buffer_Offset+6<=Buffer_Size
      &&   Buffer[Buffer_Offset  ]==0x00
      &&   Buffer[Buffer_Offset+1]==0x00
      &&  (Buffer[Buffer_Offset+2]==0x01
        || (Buffer[Buffer_Offset+2]==0x00 && Buffer[Buffer_Offset+3]==0x01)))
    {
        //Getting start_code
        int8u start_code;
        if (Buffer[Buffer_Offset+2]==0x00)
            start_code=CC1(Buffer+Buffer_Offset+4)&0x1F;
        else
            start_code=CC1(Buffer+Buffer_Offset+3)&0x1F;

        //Searching start
        if (Streams[start_code].Searching_Payload
         || Streams[start_code].ShouldDuplicate)
            return true;

        //Synchronizing
        Buffer_Offset+=4;
        Synched=false;
        if (!Synchronize())
        {
            UnSynched_IsNotJunk=true;
            return false;
        }

        if (Buffer_Offset+6>Buffer_Size)
        {
            UnSynched_IsNotJunk=true;
            return false;
        }
    }

    Trusted_IsNot("AVC, Synchronisation lost");
    return Synchronize();
}

//---------------------------------------------------------------------------
void File_Avc::Data_Parse()
{
    //Specific case
    if (Element_Code==(int64u)-1)
    {
        SPS_PPS();
        return;
    }

    //Trailing zeroes
    int64u Element_Size_SaveBeforeZeroes=Element_Size;
    if (Element_Size)
    {
        while (Element_Size && Buffer[Buffer_Offset+(size_t)Element_Size-1]==0)
            Element_Size--;
    }

    //Dump of the SPS/PPS - Init
    #if MEDIAINFO_ADVANCED2
        size_t spspps_Size=0;
        if (true) //TODO: add an option for activating this extra piece of information in the output
        {
            switch (Element_Code)
            {
                case 0x07 : //seq_parameter_set();
                            spspps_Size = seq_parameter_sets.size();
                            break;
                case 0x08 : //pic_parameter_set();
                            spspps_Size = pic_parameter_sets.size();
                            break;
                default: ;
            }
        }
    #endif //MEDIAINFO_ADVANCED2

    //svc_extension
    bool svc_extension_flag=false;
    if (Element_Code==0x0E || Element_Code==0x14)
    {
        BS_Begin();
        Get_SB (svc_extension_flag,                             "svc_extension_flag");
        if (svc_extension_flag)
            nal_unit_header_svc_extension();
        else
            nal_unit_header_mvc_extension();
        BS_End();
    }

    //Searching emulation_prevention_three_byte
    int8u* Buffer_3Bytes=NULL;
    const int8u* Save_Buffer=Buffer;
    int64u Save_File_Offset=File_Offset;
    size_t Save_Buffer_Offset=Buffer_Offset;
    int64u Save_Element_Size=Element_Size;
    size_t Element_Offset_3Bytes=(size_t)Element_Offset;
    std::vector<size_t> ThreeByte_List;
    while (Element_Offset_3Bytes+3<=Element_Size)
    {
        if (CC3(Buffer+Buffer_Offset+(size_t)Element_Offset_3Bytes)==0x000003)
            ThreeByte_List.push_back(Element_Offset_3Bytes+2);
        Element_Offset_3Bytes+=2;
        while(Element_Offset_3Bytes<Element_Size && Buffer[Buffer_Offset+(size_t)Element_Offset_3Bytes]!=0x00)
            Element_Offset_3Bytes+=2;
        if (Element_Offset_3Bytes>=Element_Size || Buffer[Buffer_Offset+(size_t)Element_Offset_3Bytes-1]==0x00)
            Element_Offset_3Bytes--;
    }

    if (!ThreeByte_List.empty())
    {
        //We must change the buffer for keeping out
        Element_Size=Save_Element_Size-ThreeByte_List.size();
        File_Offset+=Buffer_Offset;
        Buffer_Offset=0;
        Buffer_3Bytes=new int8u[(size_t)Element_Size];
        for (size_t Pos=0; Pos<=ThreeByte_List.size(); Pos++)
        {
            size_t Pos0=(Pos==ThreeByte_List.size())?(size_t)Save_Element_Size:(ThreeByte_List[Pos]);
            size_t Pos1=(Pos==0)?0:(ThreeByte_List[Pos-1]+1);
            size_t Buffer_3bytes_Begin=Pos1-Pos;
            size_t Save_Buffer_Begin  =Pos1;
            size_t Size=               Pos0-Pos1;
            std::memcpy(Buffer_3Bytes+Buffer_3bytes_Begin, Save_Buffer+Save_Buffer_Offset+Save_Buffer_Begin, Size);
        }
        Buffer=Buffer_3Bytes;
    }

    //Parsing
    switch (Element_Code)
    {
        case 0x00 : Element_Name("unspecified"); Skip_XX(Element_Size-Element_Offset, "Data"); break;
        case 0x01 : slice_layer_without_partitioning_non_IDR(); break;
        case 0x02 : Element_Name("slice_data_partition_a_layer"); Skip_XX(Element_Size-Element_Offset, "Data"); break;
        case 0x03 : Element_Name("slice_data_partition_b_layer"); Skip_XX(Element_Size-Element_Offset, "Data"); break;
        case 0x04 : Element_Name("slice_data_partition_c_layer"); Skip_XX(Element_Size-Element_Offset, "Data"); break;
        case 0x05 : slice_layer_without_partitioning_IDR(); break;
        case 0x06 : sei(); break;
        case 0x07 : seq_parameter_set(); break;
        case 0x08 : pic_parameter_set(); break;
        case 0x09 : access_unit_delimiter(); break;
        case 0x0A : Element_Name("end_of_seq"); Skip_XX(Element_Size-Element_Offset, "Data"); break;
        case 0x0B : Element_Name("end_of_stream"); Skip_XX(Element_Size-Element_Offset, "Data"); break;
        case 0x0C : filler_data(); break;
        case 0x0D : Element_Name("seq_parameter_set_extension"); Skip_XX(Element_Size-Element_Offset, "Data"); break;
        case 0x0E : prefix_nal_unit(svc_extension_flag); break;
        case 0x0F : subset_seq_parameter_set(); break;
        case 0x13 : Element_Name("slice_layer_without_partitioning"); Skip_XX(Element_Size-Element_Offset, "Data"); break;
        case 0x14 : slice_layer_extension(svc_extension_flag); break;
        default :
            if (Element_Code<0x18)
                Element_Name("reserved");
            else
                Element_Name("unspecified");
            Skip_XX(Element_Size-Element_Offset, "Data");
    }

    if (!ThreeByte_List.empty())
    {
        //We must change the buffer for keeping out
        Element_Size=Save_Element_Size;
        File_Offset=Save_File_Offset;
        Buffer_Offset=Save_Buffer_Offset;
        delete[] Buffer; Buffer=Save_Buffer;
        Buffer_3Bytes=NULL; //Same as Buffer...
        Element_Offset+=ThreeByte_List.size();
    }

    //Duplicate
    #if MEDIAINFO_DUPLICATE
        if (!Streams.empty() && Streams[(size_t)Element_Code].ShouldDuplicate)
            File__Duplicate_Write(Element_Code);
    #endif //MEDIAINFO_DUPLICATE

    #if MEDIAINFO_DEMUX
        Data_Parse_Iso14496();
    #endif //MEDIAINFO_DEMUX

    //Dump of the SPS/PPS - Fill
    #if MEDIAINFO_ADVANCED2
        if (false) //TODO: add an option for activating this extra piece of information in the output
        {
            switch (Element_Code)
            {
                case 0x07 : //seq_parameter_set();
                            if (spspps_Size != seq_parameter_sets.size() && !Status[IsFilled])
                            {
                                std::string Data_Raw((const char*)(Buffer+(size_t)(Buffer_Offset-1)), (size_t)(Element_Size+1)); //Including the last byte in the header
                                Dump_SPS.push_back(Base64::encode(Data_Raw));
                            }
                            break;
                case 0x08 : //pic_parameter_set();
                            if (spspps_Size != pic_parameter_sets.size() && !Status[IsFilled])
                            {
                                std::string Data_Raw((const char*)(Buffer+(size_t)(Buffer_Offset-1)), (size_t)(Element_Size+1)); //Including the last byte in the header
                                Dump_PPS.push_back(Base64::encode(Data_Raw));
                            }
                            break;
                default: ;
            }
        }
    #endif //MEDIAINFO_ADVANCED2

    #if MEDIAINFO_DEMUX
        Data_Parse_Iso14496();
    #endif //MEDIAINFO_DEMUX

    //Trailing zeroes
    Element_Size=Element_Size_SaveBeforeZeroes;
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
// Packet "01"
void File_Avc::slice_layer_without_partitioning_non_IDR()
{
    Element_Name("slice_layer_without_partitioning (non-IDR)");

    //Parsing
    BS_Begin();
    slice_header();
    slice_data(true);
    BS_End();
}

//---------------------------------------------------------------------------
// Packet "05"
void File_Avc::slice_layer_without_partitioning_IDR()
{
    Element_Name("slice_layer_without_partitioning (IDR)");

    //Parsing
    BS_Begin();
    slice_header();
    slice_data(true);
    BS_End();

    FILLING_BEGIN_PRECISE();
        //NextCode
        for (int8u Pos=0x01; Pos<=0x05; Pos++)
            NextCode_Add(Pos);
    FILLING_END();
}

//---------------------------------------------------------------------------
//
void File_Avc::slice_header()
{
    //Encryption management
    if (CA_system_ID_MustSkipSlices)
    {
        //Is not decodable
        Skip_BS(Data_BS_Remain(),                               "Data");
        Finish("AVC");
        return;
    }

    Element_Begin1("slice_header");

    //Parsing
    int32u  slice_type, pic_order_cnt_lsb=(int32u)-1;
    int32u  first_mb_in_slice, pic_parameter_set_id, frame_num, num_ref_idx_l0_active_minus1, num_ref_idx_l1_active_minus1, disable_deblocking_filter_idc;

    int32s  delta_pic_order_cnt_bottom=0;
    bool    field_pic_flag=false, bottom_field_flag=false;
    Get_UE (first_mb_in_slice,                                  "first_mb_in_slice");
    Get_UE (slice_type,                                         "slice_type"); Param_Info1C((slice_type<10), Avc_slice_type[slice_type]);
    #if MEDIAINFO_EVENTS
        if (!first_mb_in_slice)
        {
            switch(Element_Code)
            {
                case 5 :    // This is an IDR frame
                            if (Config->Config_PerPackage) // First slice of an IDR frame
                            {
                                // IDR
                                Config->Config_PerPackage->FrameForAlignment(this, true);
                                Config->Config_PerPackage->IsClosedGOP(this);
                            }
                            break;
                default :   ; // This is not an IDR frame
            }

            EVENT_BEGIN (Video, SliceInfo, 0)
                Event.FieldPosition=Field_Count;
                Event.SlicePosition=Element_IsOK()?first_mb_in_slice:(int64u)-1;
                switch (slice_type)
                {
                    case 0 :
                    case 3 :
                    case 5 :
                    case 8 :
                                Event.SliceType=1; break;
                    case 1 :
                    case 6 :
                                Event.SliceType=2; break;
                    case 2 :
                    case 4 :
                    case 7 :
                    case 9 :
                                Event.SliceType=0; break;
                    default:
                                Event.SliceType=(int8u)-1;
                }
                Event.Flags=0;
            EVENT_END   ()
        }
    #endif //MEDIAINFO_EVENTS
    if (slice_type>=10)
    {
        Skip_BS(Data_BS_Remain(),                               "Data");
        Element_End0();
        return;
    }
    Get_UE (pic_parameter_set_id,                               "pic_parameter_set_id");
    std::vector<pic_parameter_set_struct*>::iterator pic_parameter_set_Item;
    if (pic_parameter_set_id>=pic_parameter_sets.size() || (*(pic_parameter_set_Item=pic_parameter_sets.begin()+pic_parameter_set_id))==NULL)
    {
        //Not yet present
        Skip_BS(Data_BS_Remain(),                               "Data (pic_parameter_set is missing)");
        Element_End0();
        return;
    }
    std::vector<seq_parameter_set_struct*>::iterator seq_parameter_set_Item;
    if ((*pic_parameter_set_Item)->seq_parameter_set_id>=seq_parameter_sets.size() || (*(seq_parameter_set_Item=seq_parameter_sets.begin()+(*pic_parameter_set_Item)->seq_parameter_set_id))==NULL)
    {
        if ((*pic_parameter_set_Item)->seq_parameter_set_id>=subset_seq_parameter_sets.size() || (*(seq_parameter_set_Item=subset_seq_parameter_sets.begin()+(*pic_parameter_set_Item)->seq_parameter_set_id))==NULL)
        {
            //Not yet present
            Skip_BS(Data_BS_Remain(),                           "Data (seq_parameter_set is missing)");
            Element_End0();
            return;
        }
    }
    if ((*seq_parameter_set_Item)->separate_colour_plane_flag==1)
        Skip_S1(2,                                              "color_plane_id");
    num_ref_idx_l0_active_minus1=(*pic_parameter_set_Item)->num_ref_idx_l0_default_active_minus1; //Default
    num_ref_idx_l1_active_minus1=(*pic_parameter_set_Item)->num_ref_idx_l1_default_active_minus1; //Default
    Get_BS ((*seq_parameter_set_Item)->log2_max_frame_num_minus4+4, frame_num, "frame_num");
    if (!(*seq_parameter_set_Item)->frame_mbs_only_flag)
    {
        TEST_SB_GET(field_pic_flag,                             "field_pic_flag");
            Get_SB (bottom_field_flag,                          "bottom_field_flag");
        TEST_SB_END();
    }
    if (Element_Code==5) //IdrPicFlag
        Skip_UE(                                                "idr_pic_id");
    if ((*seq_parameter_set_Item)->pic_order_cnt_type==0)
    {
        Get_BS ((*seq_parameter_set_Item)->log2_max_pic_order_cnt_lsb_minus4+4, pic_order_cnt_lsb, "pic_order_cnt_lsb");
        if ((*pic_parameter_set_Item)->bottom_field_pic_order_in_frame_present_flag && !field_pic_flag)
            Get_SE (delta_pic_order_cnt_bottom,                 "delta_pic_order_cnt_bottom");
    }
    if ((*seq_parameter_set_Item)->pic_order_cnt_type==1 && !(*seq_parameter_set_Item)->delta_pic_order_always_zero_flag )
    {
        Skip_SE(                                                "delta_pic_order_cnt[0]");
        if((*pic_parameter_set_Item)->bottom_field_pic_order_in_frame_present_flag && !field_pic_flag)
            Skip_SE(                                            "delta_pic_order_cnt[1]");
    }
    if((*pic_parameter_set_Item)->redundant_pic_cnt_present_flag)
        Skip_UE(                                                "redundant_pic_cnt");
    if (slice_type==1 || slice_type==6) //B-Frame
        Skip_SB(                                                "direct_spatial_mv_pred_flag");
    switch (slice_type)
    {
        case 0 : //P-Frame
        case 1 : //B-Frame
        case 3 : //SP-Frame
        case 5 : //P-Frame
        case 6 : //B-Frame
        case 8 : //SP-Frame
                    TEST_SB_SKIP(                               "num_ref_idx_active_override_flag");
                        Get_UE (num_ref_idx_l0_active_minus1,   "num_ref_idx_l0_active_minus1");
                        switch (slice_type)
                        {
                            case 1 : //B-Frame
                            case 6 : //B-Frame
                                        Get_UE (num_ref_idx_l1_active_minus1, "num_ref_idx_l1_active_minus1");
                                        break;
                            default:    ;
                        }
                    TEST_SB_END();
                    break;
        default:    ;
    }
    ref_pic_list_modification(slice_type, Element_Code==20); //nal_unit_type==20 --> ref_pic_list_mvc_modification()
    if (((*pic_parameter_set_Item)->weighted_pred_flag && (slice_type==0 || slice_type==3 || slice_type==5 || slice_type==8))
     || ((*pic_parameter_set_Item)->weighted_bipred_idc==1 && (slice_type==1 || slice_type==6)))
        pred_weight_table(slice_type, num_ref_idx_l0_active_minus1, num_ref_idx_l1_active_minus1, (*seq_parameter_set_Item)->ChromaArrayType());
    std::vector<int8u> memory_management_control_operations;
    if (nal_ref_idc)
        dec_ref_pic_marking(memory_management_control_operations);

    if ((*pic_parameter_set_Item)->entropy_coding_mode_flag &&
        (slice_type!=2 && slice_type!=7 && //I-Frames
         slice_type!=4 && slice_type!=9))  //SI-Frames
        Skip_UE(                                               "cabac_init_idc");
    Skip_SE(                                                   "slice_qp_delta");
    switch (slice_type)
    {
        case 3 : //SP-Frame
        case 4 : //SI-Frame
        case 8 : //SP-Frame
        case 9 : //SI-Frame
                switch (slice_type)
                {
                    case 3 : //SP-Frame
                    case 8 : //SP-Frame
                            Skip_SB(                           "sp_for_switch_flag");
                            break;
                    default:    ;
                }
                Skip_SE (                                      "slice_qs_delta");
                break;
        default:    ;
    }
    if ((*pic_parameter_set_Item)->deblocking_filter_control_present_flag)
    {
        Get_UE(disable_deblocking_filter_idc,                  "disable_deblocking_filter_idc");
        if (disable_deblocking_filter_idc!=1)
        {
            Skip_SE(                                           "slice_alpha_c0_offset_div2");
            Skip_SE(                                           "slice_beta_offset_div2");
        }
    }

    Element_End0();

    FILLING_BEGIN();
        //Count of I-Frames
        if (first_mb_in_slice==0 && Element_Code!=20 && (slice_type==2 || slice_type==7)) //Not slice_layer_extension, I-Frame
            IFrame_Count++;
        if (!first_mb_in_slice && Slices_CountInThisFrame)
        {
            if (MaxSlicesCount<Slices_CountInThisFrame)
                MaxSlicesCount=Slices_CountInThisFrame;
            Slices_CountInThisFrame=1;
        }
        else
            Slices_CountInThisFrame++;

        //pic_struct
        if (field_pic_flag && (*seq_parameter_set_Item)->pic_struct_FirstDetected==(int8u)-1)
            (*seq_parameter_set_Item)->pic_struct_FirstDetected=bottom_field_flag?2:1; //2=BFF, 1=TFF

        //Saving some info
        if ((*seq_parameter_set_Item)->vui_parameters && (*seq_parameter_set_Item)->vui_parameters->time_scale && (*seq_parameter_set_Item)->vui_parameters->num_units_in_tick)
            tc=float64_int64s(((float64)1000000000)/((float64)(*seq_parameter_set_Item)->vui_parameters->time_scale/(*seq_parameter_set_Item)->vui_parameters->num_units_in_tick/((*seq_parameter_set_Item)->pic_order_cnt_type==2?1:2)/FrameRate_Divider)/((!(*seq_parameter_set_Item)->frame_mbs_only_flag && field_pic_flag)?2:1));

        int32s TemporalReferences_Offset_pic_order_cnt_lsb_Diff=0;
        if ((*seq_parameter_set_Item)->pic_order_cnt_type!=1 && first_mb_in_slice==0 && (Element_Code!=0x14 || seq_parameter_sets.empty()) && TemporalReferences_Reserved) //Not slice_layer_extension except if MVC only
        {
            if (field_pic_flag)
            {
                Structure_Field++;
                if (bottom_field_flag)
                    Interlaced_Bottom++;
                else
                    Interlaced_Top++;
            }
            else
                Structure_Frame++;

            //Frame order detection
            int64s pic_order_cnt=0;
            switch ((*seq_parameter_set_Item)->pic_order_cnt_type)
            {
                case 0 :
                            {
                            if (Element_Code==5) //IDR
                            {
                                prevPicOrderCntMsb=0;
                                prevPicOrderCntLsb=0;
                                TemporalReferences_Offset=TemporalReferences_Max;
                                if (TemporalReferences_Offset%2)
                                    TemporalReferences_Offset++;
                                TemporalReferences_pic_order_cnt_Min=0;
                            }
                            else
                            {
                                const bool Has5 = std::find(memory_management_control_operations.begin(), memory_management_control_operations.end(), 5) != memory_management_control_operations.end();
                                if (Has5)
                                {
                                    prevPicOrderCntMsb=0;
                                    if (bottom_field_flag)
                                        prevPicOrderCntLsb=0;
                                    else
                                        prevPicOrderCntLsb=prevTopFieldOrderCnt;
                                }
                            }
                            int32s PicOrderCntMsb;
                            if (prevPicOrderCntLsb==(int32u)-1)
                            {
                                PicOrderCntMsb=0;
                                if ((int32u)(2*((*seq_parameter_set_Item)->max_num_ref_frames+3))<pic_order_cnt_lsb)
                                    TemporalReferences_Min=pic_order_cnt_lsb-2*((*seq_parameter_set_Item)->max_num_ref_frames+3);
                            }
                            else if (pic_order_cnt_lsb<prevPicOrderCntLsb && prevPicOrderCntLsb-pic_order_cnt_lsb>=(*seq_parameter_set_Item)->MaxPicOrderCntLsb/2)
                                PicOrderCntMsb=prevPicOrderCntMsb+(*seq_parameter_set_Item)->MaxPicOrderCntLsb;
                            else if (pic_order_cnt_lsb>prevPicOrderCntLsb && pic_order_cnt_lsb-prevPicOrderCntLsb>(*seq_parameter_set_Item)->MaxPicOrderCntLsb/2)
                                PicOrderCntMsb=prevPicOrderCntMsb-(*seq_parameter_set_Item)->MaxPicOrderCntLsb;
                            else
                                PicOrderCntMsb=prevPicOrderCntMsb;

                            int32s TopFieldOrderCnt=PicOrderCntMsb+pic_order_cnt_lsb;
                            int32s BottomFieldOrderCnt;
                            if (field_pic_flag)
                                BottomFieldOrderCnt=TopFieldOrderCnt+delta_pic_order_cnt_bottom;
                            else
                                BottomFieldOrderCnt=PicOrderCntMsb+pic_order_cnt_lsb;

                            prevPicOrderCntMsb=PicOrderCntMsb;
                            prevPicOrderCntLsb=pic_order_cnt_lsb;
                            prevTopFieldOrderCnt=TopFieldOrderCnt;

                            pic_order_cnt=bottom_field_flag?BottomFieldOrderCnt:TopFieldOrderCnt;
                            }
                            break;
                case 2 :
                            {
                            const bool Has5 = std::find(memory_management_control_operations.begin(), memory_management_control_operations.end(),5) != memory_management_control_operations.end();
                            if (Has5)
                                prevFrameNumOffset=0;
                            int32u FrameNumOffset;

                            if (Element_Code==5) //IdrPicFlag
                            {
                                TemporalReferences_Offset=TemporalReferences_Max;
                                if (TemporalReferences_Offset%2)
                                    TemporalReferences_Offset++;
                                FrameNumOffset=0;
                            }
                            else if (prevFrameNumOffset==(int32u)-1)
                                FrameNumOffset=0;
                            else if (prevFrameNum>frame_num)
                                FrameNumOffset=prevFrameNumOffset+(*seq_parameter_set_Item)->MaxFrameNum;
                            else
                                FrameNumOffset=prevFrameNumOffset;

                            int32u tempPicOrderCnt;
                            if (Element_Code==5) //IdrPicFlag
                                tempPicOrderCnt=0;
                            else
                            {
                                tempPicOrderCnt=2*(FrameNumOffset+frame_num);
                                if (!nal_ref_idc && tempPicOrderCnt) //Note: if nal_ref_idc is 0, tempPicOrderCnt is not expected to be 0 but it may be the case with invalid streams
                                    tempPicOrderCnt--;
                            }

                            pic_order_cnt=tempPicOrderCnt;

                            prevFrameNum=frame_num;
                            prevFrameNumOffset=FrameNumOffset;

                            pic_order_cnt_lsb=frame_num;
                            }
                            break;
                default:    ;
            }

            if (TemporalReferences_pic_order_cnt_Min==numeric_limits<decltype(TemporalReferences_pic_order_cnt_Min)>::max())
                TemporalReferences_pic_order_cnt_Min=pic_order_cnt;
            if (pic_order_cnt<TemporalReferences_pic_order_cnt_Min)
            {
                if (pic_order_cnt<0)
                {
                    size_t Base=(size_t)(TemporalReferences_Offset+TemporalReferences_pic_order_cnt_Min);
                    size_t ToInsert=(size_t)(TemporalReferences_pic_order_cnt_Min-pic_order_cnt);
                    if (Base+ToInsert>=4*TemporalReferences_Reserved || Base>=4*TemporalReferences_Reserved || ToInsert+TemporalReferences_Max>=4*TemporalReferences_Reserved || TemporalReferences_Max>=4*TemporalReferences_Reserved || TemporalReferences_Max-Base>=4*TemporalReferences_Reserved)
                    {
                        Trusted_IsNot("Problem in temporal references");
                        return;
                    }
                    Element_Info1(__T("Offset of ")+Ztring::ToZtring(ToInsert));
                    TemporalReferences.insert(TemporalReferences.begin()+Base, ToInsert, NULL);
                    TemporalReferences_Offset+=ToInsert;
                    TemporalReferences_Offset_pic_order_cnt_lsb_Last += ToInsert;
                    TemporalReferences_Max+=ToInsert;
                    TemporalReferences_pic_order_cnt_Min=pic_order_cnt;
                }
                else if (TemporalReferences_Min>(size_t)(TemporalReferences_Offset+pic_order_cnt))
                    TemporalReferences_Min=(size_t)(TemporalReferences_Offset+pic_order_cnt);
            }

            if (pic_order_cnt<0 && TemporalReferences_Offset<(size_t)(-pic_order_cnt)) //Found in playreadyEncryptedBlowUp.ts without encryption test
            {
                Trusted_IsNot("Problem in temporal references");
                return;
            }

            if ((size_t)(TemporalReferences_Offset+pic_order_cnt)>=3*TemporalReferences_Reserved)
            {
                size_t Offset=TemporalReferences_Max-TemporalReferences_Offset;
                if (Offset%2)
                    Offset++;
                if (Offset>=TemporalReferences_Reserved && pic_order_cnt>=(int64s)TemporalReferences_Reserved)
                {
                    TemporalReferences_Offset+=TemporalReferences_Reserved;
                    pic_order_cnt-=TemporalReferences_Reserved;
                    TemporalReferences_pic_order_cnt_Min-=TemporalReferences_Reserved/2;
                    switch ((*seq_parameter_set_Item)->pic_order_cnt_type)
                    {
                        case 0 :
                                prevPicOrderCntMsb-=(int32u)TemporalReferences_Reserved;
                                break;
                        case 2 :
                                prevFrameNumOffset-=(int32u)TemporalReferences_Reserved/2;
                                break;
                        default:;
                    }
                }
                while (TemporalReferences_Offset+pic_order_cnt>=3*TemporalReferences_Reserved)
                {
                    for (size_t Pos=0; Pos<TemporalReferences_Reserved; Pos++)
                    {
                        if (TemporalReferences[Pos])
                        {
                            if ((Pos%2)==0)
                                PictureTypes_PreviousFrames+=Avc_slice_type[TemporalReferences[Pos]->slice_type];
                            delete TemporalReferences[Pos];
                            TemporalReferences[Pos] = NULL;
                        }
                        else if (!PictureTypes_PreviousFrames.empty()) //Only if stream already started
                        {
                            if ((Pos%2)==0)
                                PictureTypes_PreviousFrames+=' ';
                        }
                    }
                    if (PictureTypes_PreviousFrames.size()>=8*TemporalReferences.size())
                        PictureTypes_PreviousFrames.erase(PictureTypes_PreviousFrames.begin(), PictureTypes_PreviousFrames.begin()+PictureTypes_PreviousFrames.size()-TemporalReferences.size());
                    TemporalReferences.erase(TemporalReferences.begin(), TemporalReferences.begin()+TemporalReferences_Reserved);
                    TemporalReferences.resize(4*TemporalReferences_Reserved);
                    if (TemporalReferences_Reserved<TemporalReferences_Offset)
                        TemporalReferences_Offset-=TemporalReferences_Reserved;
                    else
                        TemporalReferences_Offset=0;
                    if (TemporalReferences_Reserved<TemporalReferences_Min)
                        TemporalReferences_Min-=TemporalReferences_Reserved;
                    else
                        TemporalReferences_Min=0;
                    if (TemporalReferences_Reserved<TemporalReferences_Max)
                        TemporalReferences_Max-=TemporalReferences_Reserved;
                    else
                        TemporalReferences_Max=0;
                    if (TemporalReferences_Reserved<TemporalReferences_Offset_pic_order_cnt_lsb_Last)
                        TemporalReferences_Offset_pic_order_cnt_lsb_Last-=TemporalReferences_Reserved;
                    else
                        TemporalReferences_Offset_pic_order_cnt_lsb_Last=0;
                }
            }

            TemporalReferences_Offset_pic_order_cnt_lsb_Diff=(int32s)((int32s)(TemporalReferences_Offset+pic_order_cnt)-TemporalReferences_Offset_pic_order_cnt_lsb_Last);
            TemporalReferences_Offset_pic_order_cnt_lsb_Last=(size_t)(TemporalReferences_Offset+pic_order_cnt);
            if (field_pic_flag && (*seq_parameter_set_Item)->pic_order_cnt_type==2 && TemporalReferences_Offset_pic_order_cnt_lsb_Last<TemporalReferences.size() && TemporalReferences[TemporalReferences_Offset_pic_order_cnt_lsb_Last])
                TemporalReferences_Offset_pic_order_cnt_lsb_Last++;
            if (TemporalReferences_Max<=TemporalReferences_Offset_pic_order_cnt_lsb_Last)
                TemporalReferences_Max=TemporalReferences_Offset_pic_order_cnt_lsb_Last+((*seq_parameter_set_Item)->frame_mbs_only_flag?2:1);
            if (TemporalReferences_Min>TemporalReferences_Offset_pic_order_cnt_lsb_Last)
                TemporalReferences_Min=TemporalReferences_Offset_pic_order_cnt_lsb_Last;
            if (TemporalReferences_DelayedElement)
            {
                delete TemporalReferences[TemporalReferences_Offset_pic_order_cnt_lsb_Last]; TemporalReferences[TemporalReferences_Offset_pic_order_cnt_lsb_Last]=TemporalReferences_DelayedElement;
            }
            if (TemporalReferences[TemporalReferences_Offset_pic_order_cnt_lsb_Last]==NULL)
                TemporalReferences[TemporalReferences_Offset_pic_order_cnt_lsb_Last]=new temporal_reference();
            TemporalReferences[TemporalReferences_Offset_pic_order_cnt_lsb_Last]->frame_num=frame_num;
            TemporalReferences[TemporalReferences_Offset_pic_order_cnt_lsb_Last]->slice_type=(int8u)slice_type;
            TemporalReferences[TemporalReferences_Offset_pic_order_cnt_lsb_Last]->IsTop=!bottom_field_flag;
            TemporalReferences[TemporalReferences_Offset_pic_order_cnt_lsb_Last]->IsField=field_pic_flag;
            if (TemporalReferences_DelayedElement)
            {
                TemporalReferences_DelayedElement=NULL;
                sei_message_user_data_registered_itu_t_t35_GA94_03_Delayed((*pic_parameter_set_Item)->seq_parameter_set_id);
            }

            if (first_mb_in_slice==0)
            {
                if (Frame_Count==0)
                {
                    if (FrameInfo.PTS==(int64u)-1)
                        FrameInfo.PTS=FrameInfo.DTS;
                    PTS_Begin=FrameInfo.PTS;
                    PTS_End=FrameInfo.PTS;
                }
                if (IFrame_Count<=1 && TC_Current.IsSet())
                {
                    TimeCode TC_Previous(Retrieve_Const(Stream_Video, 0, Video_TimeCode_FirstFrame).To_UTF8(), TC_Current.GetFramesMax());
                    if (!TC_Previous.IsSet() || TC_Current<TC_Previous)
                        Fill(Stream_Video, 0, Video_TimeCode_FirstFrame, TC_Current.ToString(), true, true);
                    TC_Current=TimeCode();
                }
                if ((*seq_parameter_set_Item)->pic_order_cnt_type != 1 && (Element_Code != 0x14 || seq_parameter_sets.empty())) //Not slice_layer_extension except if MVC only
                {
                    if ((!IsSub || Frame_Count_InThisBlock))
                    {
                        if (!pic_order_cnt_lsb || pic_order_cnt_Displayed==numeric_limits<decltype(pic_order_cnt_Displayed)>::max())
                        {
                            pic_order_cnt_Displayed=pic_order_cnt+pic_order_cnt_Delta;
                            pic_order_cnt_Delta=0;
                            FrameInfo.PTS=PTS_End;
                        }
                        int64s Divisor=field_pic_flag?1:(((*seq_parameter_set_Item)->frame_mbs_only_flag?2:(((*seq_parameter_set_Item)->pic_order_cnt_type==2)?1:2)));
                        if (pic_order_cnt_Displayed!=numeric_limits<decltype(pic_order_cnt_Displayed)>::max())
                        {
                            int64s MissingFieldCount=pic_order_cnt-pic_order_cnt_Displayed;
                            if (MissingFieldCount<0)
                            {
                                pic_order_cnt_Delta=MissingFieldCount;
                                FrameInfo.PTS=FrameInfo.DTS;
                                pic_order_cnt_Displayed=pic_order_cnt;
                                if (IFrame_Count<=1)
                                {
                                    int64s Temp=MissingFieldCount;
                                    Temp*=tc;
                                    Temp/=Divisor;
                                    PTS_Begin-=Temp;
                                }
                            }
                            else
                            {
                                int64s Offset=MissingFieldCount*tc;
                                Offset/=Divisor;
                                FrameInfo.PTS=FrameInfo.DTS+Offset;
                            }
                        }
                        pic_order_cnt_Displayed+=Divisor;
                    }
                }
            }
        }

        if (first_mb_in_slice==0)
        {
            #if MEDIAINFO_ADVANCED2
                if (PTS_Begin_Segment==(int64u)-1 && File_Offset>=Config->File_Current_Offset)
                {
                    PTS_Begin_Segment=FrameInfo.PTS;
                }
            #endif //MEDIAINFO_ADVANCED2
            if (slice_type==2 || slice_type==7) //IFrame
                FirstPFrameInGop_IsParsed=false;
        }
        else
        {
            if (FrameInfo.PTS!=(int64u)-1)
                FrameInfo.PTS-=tc;
            if (FrameInfo.DTS!=(int64u)-1)
                FrameInfo.DTS-=tc;
        }

        //Frame pos
        if (Frame_Count!=(int64u)-1 && Frame_Count && ((!(*seq_parameter_set_Item)->frame_mbs_only_flag && Interlaced_Top==Interlaced_Bottom && field_pic_flag) || first_mb_in_slice!=0 || (Element_Code==0x14 && !seq_parameter_sets.empty())))
        {
            Frame_Count--;
            if (IFrame_Count && Frame_Count_NotParsedIncluded!=(int64u)-1)
                Frame_Count_NotParsedIncluded--;
            Frame_Count_InThisBlock--;
        }
        else if (first_mb_in_slice==0)
        {
            if (!FirstPFrameInGop_IsParsed && (slice_type==0 || slice_type==5)) //P-Frame
            {
                FirstPFrameInGop_IsParsed=true;

                //Testing if we have enough to test GOP
                if (Frame_Count<=Frame_Count_Valid)
                {
                    std::string PictureTypes(PictureTypes_PreviousFrames);
                    PictureTypes.reserve(TemporalReferences.size());
                    for (size_t Pos=0; Pos<TemporalReferences.size(); Pos++)
                        if (TemporalReferences[Pos])
                        {
                            if ((Pos%2)==0)
                                PictureTypes+=Avc_slice_type[TemporalReferences[Pos]->slice_type];
                        }
                        else if (!PictureTypes.empty()) //Only if stream already started
                        {
                            if ((Pos%2)==0)
                                PictureTypes+=' ';
                        }
                        #if defined(MEDIAINFO_DTVCCTRANSPORT_YES)
                            if (!GOP_Detect(PictureTypes).empty() && !GA94_03_IsPresent)
                                Frame_Count_Valid=Frame_Count; //We have enough frames
                        #endif //defined(MEDIAINFO_DTVCCTRANSPORT_YES)
                }
            }
        }

        #if MEDIAINFO_TRACE
            if (Trace_Activated)
            {
                Element_Info1(TemporalReferences_Offset_pic_order_cnt_lsb_Last);
                Element_Info1((((*seq_parameter_set_Item)->frame_mbs_only_flag || !field_pic_flag)?__T("Frame "):(bottom_field_flag?__T("Field (Bottom) "):__T("Field (Top) ")))+Ztring::ToZtring(Frame_Count));
                if (slice_type<9)
                    Element_Info1(__T("slice_type ")+Ztring().From_UTF8(Avc_slice_type[slice_type]));
                Element_Info1(__T("frame_num ")+Ztring::ToZtring(frame_num));
                if ((*seq_parameter_set_Item)->vui_parameters && (*seq_parameter_set_Item)->vui_parameters->flags[fixed_frame_rate_flag])
                {
                    if (FrameInfo.PCR!=(int64u)-1)
                        Element_Info1(__T("PCR ")+Ztring().Duration_From_Milliseconds(float64_int64s(((float64)FrameInfo.PCR)/1000000)));
                    if (FrameInfo.DTS!=(int64u)-1)
                        Element_Info1(__T("DTS ")+Ztring().Duration_From_Milliseconds(float64_int64s(((float64)FrameInfo.DTS)/1000000)));
                    if (FrameInfo.PTS!=(int64u)-1)
                        Element_Info1(__T("PTS ")+Ztring().Duration_From_Milliseconds(float64_int64s(((float64)FrameInfo.PTS)/1000000)));
                }
                if ((*seq_parameter_set_Item)->pic_order_cnt_type==0)
                    Element_Info1(__T("pic_order_cnt_lsb ")+Ztring::ToZtring(pic_order_cnt_lsb));
                if (first_mb_in_slice)
                    Element_Info1(__T("first_mb_in_slice ")+Ztring::ToZtring(first_mb_in_slice));
            }
        #endif //MEDIAINFO_TRACE

        //Counting
        if (Frame_Count!=(int64u)-1)
        {
            if (File_Offset+Buffer_Offset+Element_Size==File_Size)
                Frame_Count_Valid=Frame_Count; //Finish frames in case of there are less than Frame_Count_Valid frames
            Frame_Count++;
            if (IFrame_Count && Frame_Count_NotParsedIncluded!=(int64u)-1)
                Frame_Count_NotParsedIncluded++;
            Frame_Count_InThisBlock++;
        }
        if ((*seq_parameter_set_Item)->pic_order_cnt_type==0 && field_pic_flag)
        {
            Field_Count++;
            Field_Count_InThisBlock++;
        }
        if (FrameInfo.PTS!=(int64u)-1)
            FrameInfo.PTS+=tc;
        if (FrameInfo.DTS!=(int64u)-1)
            FrameInfo.DTS+=tc;
        if (FrameInfo.PTS!=(int64u)-1 && (FrameInfo.PTS>PTS_End || (PTS_End>1000000000 && FrameInfo.PTS<=PTS_End-1000000000))) //More than current PTS_End or less than current PTS_End minus 1 second (there is a problem?)
            PTS_End=FrameInfo.PTS;

        #if MEDIAINFO_DUPLICATE
            if (Streams[(size_t)Element_Code].ShouldDuplicate)
                File__Duplicate_Write(Element_Code, (*seq_parameter_set_Item)->pic_order_cnt_type==0?pic_order_cnt_lsb:frame_num);
        #endif //MEDIAINFO_DUPLICATE

        //Filling only if not already done
        if (Frame_Count==1 && !Status[IsAccepted])
            Accept("AVC");
        if (!Status[IsFilled])
        {
            #if defined(MEDIAINFO_DTVCCTRANSPORT_YES)
                if (!GA94_03_IsPresent && IFrame_Count>=8)
                    Frame_Count_Valid=Frame_Count; //We have enough frames
            #endif //defined(MEDIAINFO_DTVCCTRANSPORT_YES)
            if (Frame_Count>=Frame_Count_Valid)
            {
                Fill("AVC");
                if (!IsSub && !Streams[(size_t)Element_Code].ShouldDuplicate && Config->ParseSpeed<1.0)
                    Finish("AVC");
            }
            if (FrameIsAlwaysComplete && !first_mb_in_slice && FrameSizes.size()<=1)
                FrameSizes[Buffer_Size]++;
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
//
void File_Avc::slice_data(bool AllCategories)
{
    Element_Begin1("slice_data");

    Skip_BS(Data_BS_Remain(),                                   "(ToDo)");

    Element_End0();
}

//---------------------------------------------------------------------------
//
void File_Avc::ref_pic_list_modification(int32u slice_type, bool mvc)
{
    if ((slice_type%5)!=2 && (slice_type%5)!=4)
    {
        TEST_SB_SKIP(                                           "ref_pic_list_modification_flag_l0");
            int32u modification_of_pic_nums_idc;
            do
            {
                Get_UE (modification_of_pic_nums_idc,           "modification_of_pic_nums_idc");
                if (modification_of_pic_nums_idc<2)
                    Skip_UE(                                    "abs_diff_pic_num_minus1");
                else if (modification_of_pic_nums_idc==2)
                    Skip_UE(                                    "long_term_pic_num");
                else if (mvc && (modification_of_pic_nums_idc==4 || modification_of_pic_nums_idc==5)) //ref_pic_list_mvc_modification only
                    Skip_UE(                                    "abs_diff_view_idx_minus1");
                else if (modification_of_pic_nums_idc!=3)
                {
                    Trusted_IsNot("ref_pic_list_modification_flag_l0");
                    Skip_BS(Data_BS_Remain(),                   "(Remaining bits)");
                }
            }
            while (modification_of_pic_nums_idc!=3 && Data_BS_Remain());
        TEST_SB_END();
    }
    if ((slice_type%5)==1)
    {
        TEST_SB_SKIP(                                           "ref_pic_list_modification_flag_l1");
            int32u modification_of_pic_nums_idc;
            do
            {
                Get_UE (modification_of_pic_nums_idc,           "modification_of_pic_nums_idc");
                if (modification_of_pic_nums_idc<2)
                    Skip_UE(                                    "abs_diff_pic_num_minus1");
                else if (modification_of_pic_nums_idc==2)
                    Skip_UE(                                    "long_term_pic_num");
                else if (mvc && (modification_of_pic_nums_idc==4 || modification_of_pic_nums_idc==5)) //ref_pic_list_mvc_modification only
                    Skip_UE(                                    "abs_diff_view_idx_minus1");
                else if (modification_of_pic_nums_idc!=3)
                {
                    Trusted_IsNot("ref_pic_list_modification_flag_l1");
                    Skip_BS(Data_BS_Remain(),                   "(Remaining bits)");
                }
            }
            while (modification_of_pic_nums_idc!=3 && Data_BS_Remain());
        TEST_SB_END();
    }
}

//---------------------------------------------------------------------------
//
void File_Avc::pred_weight_table(int32u  slice_type, int32u num_ref_idx_l0_active_minus1, int32u num_ref_idx_l1_active_minus1, int8u ChromaArrayType)
{
    // 7.3.3.2 Prediction weight table syntax
    Skip_UE(                                                    "luma_log2_weight_denom");
    if (ChromaArrayType)
        Skip_UE(                                                "chroma_log2_weight_denom");
    for(int32u i=0; i<=num_ref_idx_l0_active_minus1; i++)
    {
        TEST_SB_SKIP(                                           "luma_weight_l0_flag");
            Skip_SE(                                            "luma_weight_l0");
            Skip_SE(                                            "luma_offset_l0");
        TEST_SB_END();
		if (ChromaArrayType)
		{
			TEST_SB_SKIP(                                       "chroma_weight_l0_flag");
				Skip_SE(                                        "chroma_weight_l0");
				Skip_SE(                                        "chroma_offset_l0");
				Skip_SE(                                        "chroma_weight_l0");
				Skip_SE(                                        "chroma_offset_l0");
			TEST_SB_END();
		}
    }
    if (slice_type % 5 == 1)
    {
        for (int32u i = 0; i <= num_ref_idx_l1_active_minus1; i++)
        {
            TEST_SB_SKIP("luma_weight_l1_flag");
            Skip_SE("luma_weight_l1");
            Skip_SE("luma_offset_l1");
            TEST_SB_END();
            if (ChromaArrayType)
            {
                TEST_SB_SKIP("chroma_weight_l1_flag");
                Skip_SE("chroma_weight_l1");
                Skip_SE("chroma_offset_l1");
                Skip_SE("chroma_weight_l1");
                Skip_SE("chroma_offset_l1");
                TEST_SB_END();
            }
        }
    }
}

//---------------------------------------------------------------------------
//
void File_Avc::dec_ref_pic_marking(std::vector<int8u> &memory_management_control_operations)
{
    if (Element_Code==5) //IdrPicFlag
    {
        Skip_SB(                                                "no_output_of_prior_pics_flag");
        Skip_SB(                                                "long_term_reference_flag");
    }
    else
    {
        TEST_SB_SKIP(                                           "adaptive_ref_pic_marking_mode_flag");
            int32u memory_management_control_operation;
            do
            {
                Get_UE (memory_management_control_operation,    "memory_management_control_operation");
                switch (memory_management_control_operation)
                {
                    case 1 :
                                Skip_UE(                        "difference_of_pic_nums_minus1");
                                break;
                    case 2 :
                                Skip_UE(                        "long_term_pic_num");
                                break;
                    case 3 :
                                Skip_UE(                        "difference_of_pic_nums_minus1");
                                //break; 3 --> difference_of_pic_nums_minus1 then long_term_frame_idx
                    case 6 :
                                Skip_UE(                        "long_term_frame_idx");
                                break;
                    case 4 :
                                Skip_UE(                        "max_long_term_frame_idx_plus1");
                                break;
                }
                memory_management_control_operations.push_back((int8u)memory_management_control_operation);
            }
            while (Data_BS_Remain() && memory_management_control_operation);
        TEST_SB_END()
    }
}

//---------------------------------------------------------------------------
// Packet "06"
void File_Avc::sei()
{
    Element_Name("sei");

    //Parsing
    int32u seq_parameter_set_id=(int32u)-1;
    while(Element_Offset+1<Element_Size)
    {
        Element_Begin1("sei message");
            sei_message(seq_parameter_set_id);
        Element_End0();
    }
    BS_Begin();
    Mark_1(                                                     );
    BS_End();
}

//---------------------------------------------------------------------------
void File_Avc::sei_message(int32u &seq_parameter_set_id)
{
    //Parsing
    int32u payloadType=0, payloadSize=0;
    int8u payload_type_byte, payload_size_byte;
    Element_Begin1("sei message header");
        do
        {
            Get_B1 (payload_type_byte,                          "payload_type_byte");
            payloadType+=payload_type_byte;
        }
        while(payload_type_byte==0xFF);
        do
        {
            Get_B1 (payload_size_byte,                          "payload_size_byte");
            payloadSize+=payload_size_byte;
        }
        while(payload_size_byte==0xFF);
    Element_End0();

    int64u Element_Offset_Save=Element_Offset+payloadSize;
    if (Element_Offset_Save>Element_Size)
    {
        Trusted_IsNot("Wrong size");
        Skip_XX(Element_Size-Element_Offset,                    "unknown");
        return;
    }
    int64u Element_Size_Save=Element_Size;
    Element_Size=Element_Offset_Save;
    switch (payloadType)
    {
        case  0 :   sei_message_buffering_period(seq_parameter_set_id); break;
        case  1 :   sei_message_pic_timing(payloadSize, seq_parameter_set_id); break;
        case  4 :   sei_message_user_data_registered_itu_t_t35(); break;
        case  5 :   sei_message_user_data_unregistered(payloadSize); break;
        case  6 :   sei_message_recovery_point(); break;
        case 32 :   sei_message_mainconcept(payloadSize); break;
        case 147:   sei_alternative_transfer_characteristics(); break;
        case 137:   sei_message_mastering_display_colour_volume(); break;
        case 144:   sei_message_light_level(); break;
        default :
                    Element_Info1("unknown");
                    Skip_XX(payloadSize,                        "data");
    }
    Element_Offset=Element_Offset_Save; //Positionning in the right place.
    Element_Size=Element_Size_Save; //Positionning in the right place.
}

//---------------------------------------------------------------------------
// SEI - 0
void File_Avc::sei_message_buffering_period(int32u &seq_parameter_set_id)
{
    Element_Info1("buffering_period");

    //Parsing
    if (Element_Offset==Element_Size)
        return; //Nothing to do
    BS_Begin();
    Get_UE (seq_parameter_set_id,                               "seq_parameter_set_id");
    std::vector<seq_parameter_set_struct*>::iterator seq_parameter_set_Item;
    if (seq_parameter_set_id>=seq_parameter_sets.size() || (*(seq_parameter_set_Item=seq_parameter_sets.begin()+seq_parameter_set_id))==NULL)
    {
        //Not yet present
        Skip_BS(Data_BS_Remain(),                               "Data (seq_parameter_set is missing)");
        BS_End();
        return;
    }
    if ((*seq_parameter_set_Item)->NalHrdBpPresentFlag())
        sei_message_buffering_period_xxl((*seq_parameter_set_Item)->vui_parameters->NAL);
    if ((*seq_parameter_set_Item)->VclHrdBpPresentFlag())
        sei_message_buffering_period_xxl((*seq_parameter_set_Item)->vui_parameters->VCL);
    BS_End();
}

void File_Avc::sei_message_buffering_period_xxl(seq_parameter_set_struct::vui_parameters_struct::xxl* xxl)
{
    if (xxl==NULL)
        return;
    for (int32u SchedSelIdx=0; SchedSelIdx<xxl->SchedSel.size(); SchedSelIdx++)
    {
        //Get_S4 (xxl->initial_cpb_removal_delay_length_minus1+1, xxl->SchedSel[SchedSelIdx].initial_cpb_removal_delay, "initial_cpb_removal_delay"); Param_Info2(xxl->SchedSel[SchedSelIdx].initial_cpb_removal_delay/90, " ms");
        //Get_S4 (xxl->initial_cpb_removal_delay_length_minus1+1, xxl->SchedSel[SchedSelIdx].initial_cpb_removal_delay_offset, "initial_cpb_removal_delay_offset"); Param_Info2(xxl->SchedSel[SchedSelIdx].initial_cpb_removal_delay_offset/90, " ms");
        Info_S4 (xxl->initial_cpb_removal_delay_length_minus1+1, initial_cpb_removal_delay, "initial_cpb_removal_delay"); Param_Info2(initial_cpb_removal_delay/90, " ms");
        Info_S4 (xxl->initial_cpb_removal_delay_length_minus1+1, initial_cpb_removal_delay_offset, "initial_cpb_removal_delay_offset"); Param_Info2(initial_cpb_removal_delay_offset/90, " ms");
    }
}

//---------------------------------------------------------------------------
// SEI - 1
void File_Avc::sei_message_pic_timing(int32u /*payloadSize*/, int32u seq_parameter_set_id)
{
    Element_Info1("pic_timing");

    //Testing if we can parsing it now. TODO: handle case seq_parameter_set_id is unknown (buffering of message, decoding in slice parsing)
    if (seq_parameter_set_id==(int32u)-1 && seq_parameter_sets.size()==1)
        seq_parameter_set_id=0;
    std::vector<seq_parameter_set_struct*>::iterator seq_parameter_set_Item;
    if (seq_parameter_set_id>=seq_parameter_sets.size() || (*(seq_parameter_set_Item=seq_parameter_sets.begin()+seq_parameter_set_id))==NULL)
    {
        //Not yet present
        Skip_BS(Data_BS_Remain(),                               "Data (seq_parameter_set is missing)");
        return;
    }

    //Parsing
    int8u   pic_struct=(int8u)-1;
    BS_Begin();
    if ((*seq_parameter_set_Item)->CpbDpbDelaysPresentFlag())
    {
        int8u cpb_removal_delay_length_minus1=(*seq_parameter_set_Item)->vui_parameters->NAL?(*seq_parameter_set_Item)->vui_parameters->NAL->cpb_removal_delay_length_minus1:(*seq_parameter_set_Item)->vui_parameters->VCL->cpb_removal_delay_length_minus1; //Spec is not precise, I am not sure
        int8u dpb_output_delay_length_minus1=(*seq_parameter_set_Item)->vui_parameters->NAL?(*seq_parameter_set_Item)->vui_parameters->NAL->dpb_output_delay_length_minus1:(*seq_parameter_set_Item)->vui_parameters->VCL->dpb_output_delay_length_minus1; //Spec is not precise, I am not sure
        Skip_S4(cpb_removal_delay_length_minus1+1,              "cpb_removal_delay");
        Skip_S4(dpb_output_delay_length_minus1+1,               "dpb_output_delay");
    }
    if ((*seq_parameter_set_Item)->vui_parameters && (*seq_parameter_set_Item)->vui_parameters->flags[pic_struct_present_flag])
    {
        Get_S1 (4, pic_struct,                                  "pic_struct");
        switch (pic_struct)
        {
            case  0 :
            case  1 :
            case  2 :
            case  3 :
            case  4 :
            case  5 :
            case  6 : FrameRate_Divider=1; break;
            case  7 : FrameRate_Divider=2; break;
            case  8 : FrameRate_Divider=3; break;
            default : Param_Info1("Reserved"); return; //NumClockTS is unknown
        }
        Param_Info1(Avc_pic_struct[pic_struct]);
        int8u NumClockTS=Avc_NumClockTS[pic_struct];
        int8u seconds_value=0, minutes_value=0, hours_value=0; //Here because theses values can be reused in later ClockTSs.
        for (int8u i=0; i<NumClockTS; i++)
        {
            Element_Begin1("ClockTS");
            TEST_SB_SKIP(                                       "clock_timestamp_flag");
                int32u time_offset=0;
                int8u counting_type, n_frames;
                bool nuit_field_based_flag, full_timestamp_flag, seconds_flag, minutes_flag, hours_flag;
                Info_S1(2, ct_type,                             "ct_type"); Param_Info1(Avc_ct_type[ct_type]);
                Get_SB (   nuit_field_based_flag,               "nuit_field_based_flag");
                Get_S1 (5, counting_type,                       "counting_type");
                Get_SB (   full_timestamp_flag,                 "full_timestamp_flag");
                Skip_SB(                                        "discontinuity_flag");
                Skip_SB(                                        "cnt_dropped_flag");
                Get_S1 (8, n_frames,                            "n_frames");
                seconds_flag=minutes_flag=hours_flag=full_timestamp_flag;
                if (!full_timestamp_flag)
                    Get_SB (seconds_flag,                       "seconds_flag");
                if (seconds_flag)
                    Get_S1 (6, seconds_value,                   "seconds_value");
                if (!full_timestamp_flag && seconds_flag)
                    Get_SB (minutes_flag,                       "minutes_flag");
                if (minutes_flag)
                    Get_S1 (6, minutes_value,                   "minutes_value");
                if (!full_timestamp_flag && minutes_flag)
                    Get_SB (hours_flag,                         "hours_flag");
                if (hours_flag)
                    Get_S1 (5, hours_value,                     "hours_value");
                if ((*seq_parameter_set_Item)->CpbDpbDelaysPresentFlag())
                {
                    int8u time_offset_length=(*seq_parameter_set_Item)->vui_parameters->NAL?(*seq_parameter_set_Item)->vui_parameters->NAL->time_offset_length:(*seq_parameter_set_Item)->vui_parameters->VCL->time_offset_length; //Spec is not precise, I am not sure
                    if (time_offset_length)
                        Get_S4 (time_offset_length, time_offset,    "time_offset");
                }
                FILLING_BEGIN();
                    if (!i && seconds_flag && minutes_flag && hours_flag && IFrame_Count<=1)
                    {
                        int32u FrameMax;
                        if (counting_type>1 && counting_type!=4)
                        {
                            n_frames=0;
                            FrameMax=0; //Unsupported type
                        }
                        else if ((*seq_parameter_set_Item)->vui_parameters->flags[fixed_frame_rate_flag] && (*seq_parameter_set_Item)->vui_parameters->time_scale && (*seq_parameter_set_Item)->vui_parameters->time_scale && (*seq_parameter_set_Item)->vui_parameters->num_units_in_tick)
                            FrameMax=(int32u)(float64_int64s((float64)(*seq_parameter_set_Item)->vui_parameters->time_scale/(*seq_parameter_set_Item)->vui_parameters->num_units_in_tick/((*seq_parameter_set_Item)->frame_mbs_only_flag?2:(((*seq_parameter_set_Item)->pic_order_cnt_type==2 && Structure_Frame/2>Structure_Field)?1:2))/FrameRate_Divider)-1);
                        else if (n_frames>99)
                            FrameMax=n_frames;
                        else
                            FrameMax=99;

                        TC_Current=TimeCode(hours_value, minutes_value, seconds_value, n_frames, FrameMax, TimeCode::DropFrame(counting_type==4));
                        Element_Info1(TC_Current.ToString());
                    }
                FILLING_END();
            TEST_SB_END();
            Element_End0();
        }
    }
    BS_End();

    FILLING_BEGIN_PRECISE();
        if ((*seq_parameter_set_Item)->pic_struct_FirstDetected==(int8u)-1 && (*seq_parameter_set_Item)->vui_parameters && (*seq_parameter_set_Item)->vui_parameters->flags[pic_struct_present_flag])
            (*seq_parameter_set_Item)->pic_struct_FirstDetected=pic_struct;
    FILLING_END();
}

//---------------------------------------------------------------------------
// SEI - 5
void File_Avc::sei_message_user_data_registered_itu_t_t35()
{
    Element_Info1("user_data_registered_itu_t_t35");

    //Parsing
    int8u itu_t_t35_country_code;
    Get_B1 (itu_t_t35_country_code,                             "itu_t_t35_country_code");
    if (itu_t_t35_country_code==0xFF)
        Skip_B1(                                                "itu_t_t35_country_code_extension_byte");
    if (itu_t_t35_country_code!=0xB5 || Element_Offset+2>=Element_Size)
    {
        if (Element_Size-Element_Offset)
            Skip_XX(Element_Size-Element_Offset,                "Unknown");
        return;
    }

    //United-States
    int16u id;
    Get_B2 (id,                                                 "id?");
    if (id!=0x0031 || Element_Offset+4>=Element_Size)
    {
        if (Element_Size-Element_Offset)
            Skip_XX(Element_Size-Element_Offset,                "Unknown");
        return;
    }

    int32u Identifier;
    Peek_B4(Identifier);
    switch (Identifier)
    {
        case 0x44544731 :   sei_message_user_data_registered_itu_t_t35_DTG1(); return;
        case 0x47413934 :   sei_message_user_data_registered_itu_t_t35_GA94(); return;
        default         :   if (Element_Size-Element_Offset)
                                Skip_XX(Element_Size-Element_Offset, "Unknown");
    }
}

//---------------------------------------------------------------------------
// SEI - 5 - DTG1
void File_Avc::sei_message_user_data_registered_itu_t_t35_DTG1()
{
    Element_Info1("Active Format Description");

    //Parsing
    Skip_C4(                                                    "afd_identifier");
    if (Element_Offset<Element_Size)
    {
        File_AfdBarData DTG1_Parser;
        for (auto seq_parameter_set_Item : seq_parameter_sets)
        {
            if (seq_parameter_set_Item && seq_parameter_set_Item->vui_parameters && seq_parameter_set_Item->vui_parameters->sar_width && seq_parameter_set_Item->vui_parameters->sar_height)
            {
                //TODO: avoid duplicated code
                int32u Width =(seq_parameter_set_Item->pic_width_in_mbs_minus1       +1)*16;
                int32u Height=(seq_parameter_set_Item->pic_height_in_map_units_minus1+1)*16*(2-seq_parameter_set_Item->frame_mbs_only_flag);
                int8u chromaArrayType = seq_parameter_set_Item->ChromaArrayType();
                if (chromaArrayType >= 4)
                    chromaArrayType = 0;
                int32u CropUnitX=Avc_SubWidthC [chromaArrayType];
                int32u CropUnitY=Avc_SubHeightC[chromaArrayType]*(2-seq_parameter_set_Item->frame_mbs_only_flag);
                Width -=(seq_parameter_set_Item->frame_crop_left_offset+seq_parameter_set_Item->frame_crop_right_offset )*CropUnitX;
                Height-=(seq_parameter_set_Item->frame_crop_top_offset +seq_parameter_set_Item->frame_crop_bottom_offset)*CropUnitY;
                if (Height)
                {
                    auto PixelAspectRatio=((float32)seq_parameter_set_Item->vui_parameters->sar_width) / seq_parameter_set_Item->vui_parameters->sar_height;
                    auto DAR=Width*PixelAspectRatio/Height;
                    if (DAR>=4.0/3.0*0.95 && DAR<4.0/3.0*1.05) DTG1_Parser.aspect_ratio_FromContainer=0; //4/3
                    if (DAR>=16.0/9.0*0.95 && DAR<16.0/9.0*1.05) DTG1_Parser.aspect_ratio_FromContainer=1; //16/9
                }
            }
        }
        Open_Buffer_Init(&DTG1_Parser);
        DTG1_Parser.Format=File_AfdBarData::Format_A53_4_DTG1;
        Open_Buffer_Continue(&DTG1_Parser, Buffer+Buffer_Offset+(size_t)Element_Offset, Element_Size-Element_Offset);
        Merge(DTG1_Parser, Stream_Video, 0, 0);
        Element_Offset=Element_Size;
    }
}

//---------------------------------------------------------------------------
// SEI - 5 - GA94
void File_Avc::sei_message_user_data_registered_itu_t_t35_GA94()
{
    //Parsing
    int8u user_data_type_code;
    Skip_B4(                                                    "GA94_identifier");
    Get_B1 (user_data_type_code,                                "user_data_type_code");
    switch (user_data_type_code)
    {
        case 0x03 : sei_message_user_data_registered_itu_t_t35_GA94_03(); break;
        case 0x06 : sei_message_user_data_registered_itu_t_t35_GA94_06(); break;
        default   : Skip_XX(Element_Size-Element_Offset,        "GA94_reserved_user_data");
    }
}

//---------------------------------------------------------------------------
// SEI - 5 - GA94 - 0x03
void File_Avc::sei_message_user_data_registered_itu_t_t35_GA94_03()
{
    #if defined(MEDIAINFO_DTVCCTRANSPORT_YES)
        GA94_03_IsPresent=true;
        MustExtendParsingDuration=true;
        Buffer_TotalBytes_Fill_Max=(int64u)-1; //Disabling this feature for this format, this is done in the parser

        Element_Info1("DTVCC Transport");

        //Coherency
        delete TemporalReferences_DelayedElement; TemporalReferences_DelayedElement=new temporal_reference();

        TemporalReferences_DelayedElement->GA94_03=new buffer_data(Buffer+Buffer_Offset+(size_t)Element_Offset,(size_t)(Element_Size-Element_Offset));

        //Parsing
        Skip_XX(Element_Size-Element_Offset,                    "CC data");
    #else //defined(MEDIAINFO_DTVCCTRANSPORT_YES)
        Skip_XX(Element_Size-Element_Offset,                    "DTVCC Transport data");
    #endif //defined(MEDIAINFO_DTVCCTRANSPORT_YES)
}

void File_Avc::sei_message_user_data_registered_itu_t_t35_GA94_03_Delayed(int32u seq_parameter_set_id)
{
    // Skipping missing frames
    if (TemporalReferences_Max-TemporalReferences_Min>(size_t)(4*(seq_parameter_sets[seq_parameter_set_id]->max_num_ref_frames+3))) // max_num_ref_frames ref frame maximum
    {
        size_t TemporalReferences_Min_New=TemporalReferences_Max-4*(seq_parameter_sets[seq_parameter_set_id]->max_num_ref_frames+3);
        while (TemporalReferences_Min_New>TemporalReferences_Min && TemporalReferences[TemporalReferences_Min_New-1])
            TemporalReferences_Min_New--;
        TemporalReferences_Min=TemporalReferences_Min_New;
        while (TemporalReferences[TemporalReferences_Min]==NULL)
        {
            TemporalReferences_Min++;
            if (TemporalReferences_Min>=TemporalReferences.size())
                return;
        }
    }

    // Parsing captions
    while (TemporalReferences[TemporalReferences_Min] && TemporalReferences_Min+2*seq_parameter_sets[seq_parameter_set_id]->max_num_ref_frames<TemporalReferences_Max)
    {
        #if defined(MEDIAINFO_DTVCCTRANSPORT_YES)
            Element_Begin1("Reordered DTVCC Transport");

            //Parsing
            #if MEDIAINFO_DEMUX
                int64u Element_Code_Old=Element_Code;
                Element_Code=0x4741393400000003LL;
            #endif //MEDIAINFO_DEMUX
            if (GA94_03_Parser==NULL)
            {
                GA94_03_Parser=new File_DtvccTransport;
                Open_Buffer_Init(GA94_03_Parser);
                ((File_DtvccTransport*)GA94_03_Parser)->Format=File_DtvccTransport::Format_A53_4_GA94_03;
            }
            if (((File_DtvccTransport*)GA94_03_Parser)->AspectRatio==0)
            {
                std::vector<seq_parameter_set_struct*>::iterator seq_parameter_set_Item_=seq_parameter_sets.begin();
                for (; seq_parameter_set_Item_!=seq_parameter_sets.end(); ++seq_parameter_set_Item_)
                    if ((*seq_parameter_set_Item_))
                        break;
                if (seq_parameter_set_Item_!=seq_parameter_sets.end())
                {
                    const auto seq_parameter_set_Item=*seq_parameter_set_Item_;
                    const auto vui_parameters=seq_parameter_set_Item->vui_parameters;
                    if (vui_parameters && vui_parameters->sar_width && vui_parameters->sar_height)
                    {
                        const int32u Width =(seq_parameter_set_Item->pic_width_in_mbs_minus1       +1)*16;
                        const int32u Height=(seq_parameter_set_Item->pic_height_in_map_units_minus1+1)*16*(2-seq_parameter_set_Item->frame_mbs_only_flag);
                        if (Height)
                        {
                            auto PixelAspectRatio=((float32)vui_parameters->sar_width)/vui_parameters->sar_height;
                            ((File_DtvccTransport*)GA94_03_Parser)->AspectRatio=Width*PixelAspectRatio/Height;
                        }
                    }
                }
            }
            if (GA94_03_Parser->PTS_DTS_Needed)
            {
                GA94_03_Parser->FrameInfo.PCR=FrameInfo.PCR;
                GA94_03_Parser->FrameInfo.PTS=FrameInfo.PTS;
                GA94_03_Parser->FrameInfo.DTS=FrameInfo.DTS;
            }
            #if MEDIAINFO_DEMUX
                if (TemporalReferences[TemporalReferences_Min]->GA94_03)
                {
                    int8u Demux_Level_Save=Demux_Level;
                    Demux_Level=8; //Ancillary
                    Demux(TemporalReferences[TemporalReferences_Min]->GA94_03->Data, TemporalReferences[TemporalReferences_Min]->GA94_03->Size, ContentType_MainStream);
                    Demux_Level=Demux_Level_Save;
                }
                Element_Code=Element_Code_Old;
            #endif //MEDIAINFO_DEMUX
            if (TemporalReferences[TemporalReferences_Min]->GA94_03)
            {
                #if defined(MEDIAINFO_EIA608_YES) || defined(MEDIAINFO_EIA708_YES)
                    GA94_03_Parser->ServiceDescriptors=ServiceDescriptors;
                #endif
                Open_Buffer_Continue(GA94_03_Parser, TemporalReferences[TemporalReferences_Min]->GA94_03->Data, TemporalReferences[TemporalReferences_Min]->GA94_03->Size);
            }

            Element_End0();
        #endif //defined(MEDIAINFO_DTVCCTRANSPORT_YES)

        TemporalReferences_Min+=((seq_parameter_sets[seq_parameter_set_id]->frame_mbs_only_flag | !TemporalReferences[TemporalReferences_Min]->IsField)?2:1);
    }
}

//---------------------------------------------------------------------------
// SEI - 5 - GA94 - 0x03
void File_Avc::sei_message_user_data_registered_itu_t_t35_GA94_06()
{
    Element_Info1("Bar data");

    //Parsing
    bool   top_bar_flag, bottom_bar_flag, left_bar_flag, right_bar_flag;
    BS_Begin();
    Get_SB (top_bar_flag,                                       "top_bar_flag");
    Get_SB (bottom_bar_flag,                                    "bottom_bar_flag");
    Get_SB (left_bar_flag,                                      "left_bar_flag");
    Get_SB (right_bar_flag,                                     "right_bar_flag");
    Mark_1_NoTrustError();
    Mark_1_NoTrustError();
    Mark_1_NoTrustError();
    Mark_1_NoTrustError();
    BS_End();
    if (top_bar_flag)
    {
        Mark_1();
        Mark_1();
        Skip_S2(14,                                             "line_number_end_of_top_bar");
    }
    if (bottom_bar_flag)
    {
        Mark_1();
        Mark_1();
        Skip_S2(14,                                             "line_number_start_of_bottom_bar");
    }
    if (left_bar_flag)
    {
        Mark_1();
        Mark_1();
        Skip_S2(14,                                             "pixel_number_end_of_left_bar");
    }
    if (right_bar_flag)
    {
        Mark_1();
        Mark_1();
        Skip_S2(14,                                             "pixel_number_start_of_right_bar");
    }
    Mark_1();
    Mark_1();
    Mark_1();
    Mark_1();
    Mark_1();
    Mark_1();
    Mark_1();
    Mark_1();
    BS_End();

    if (Element_Size-Element_Offset)
        Skip_XX(Element_Size-Element_Offset,                    "additional_bar_data");
}

//---------------------------------------------------------------------------
// SEI - 5
void File_Avc::sei_message_user_data_unregistered(int32u payloadSize)
{
    Element_Info1("user_data_unregistered");

    //Parsing
    int128u uuid_iso_iec_11578;
    Get_UUID(uuid_iso_iec_11578,                                "uuid_iso_iec_11578");

    switch (uuid_iso_iec_11578.hi)
    {
        case 0xDC45E9BDE6D948B7LL : Element_Info1("x264");
                                     sei_message_user_data_unregistered_x264(payloadSize-16); break;
        case 0xFB574A60AC924E68LL : Element_Info1("eavc");
                                     sei_message_user_data_unregistered_x264(payloadSize-16); break;
        case 0x17EE8C60F84D11D9LL : Element_Info1("Blu-ray");
                                    sei_message_user_data_unregistered_bluray(payloadSize-16); break;
        default :
                    Element_Info1("unknown");
                    Skip_XX(payloadSize-16,                     "data");
    }
}

//---------------------------------------------------------------------------
// SEI - 5 - x264
void File_Avc::sei_message_user_data_unregistered_x264(int32u payloadSize)
{
    //Parsing
    string Data;
    Peek_String(payloadSize, Data);
    if (Data.size()!=payloadSize && Data.size()+1!=payloadSize)
    {
        Skip_XX(payloadSize,                                    "Unknown");
        return; //This is not a text string
    }
    size_t Data_Pos_Before=0;
    size_t Loop=0;
    do
    {
        size_t Data_Pos=Data.find(" - ", Data_Pos_Before);
        if (Data_Pos==std::string::npos)
            Data_Pos=Data.size();
        if (Data.find("options: ", Data_Pos_Before)==Data_Pos_Before)
        {
            Element_Begin1("options");
            size_t Options_Pos_Before=Data_Pos_Before;
            Encoded_Library_Settings.clear();
            do
            {
                size_t Options_Pos=Data.find(__T(' '), Options_Pos_Before);
                if (Options_Pos==std::string::npos)
                    Options_Pos=Data.size();
                string option;
                Get_String (Options_Pos-Options_Pos_Before, option, "option");
                Options_Pos_Before=Options_Pos;
                do
                {
                    string Separator;
                    Peek_String(1, Separator);
                    if (Separator==" ")
                    {
                        Skip_UTF8(1,                                "separator");
                        Options_Pos_Before+=1;
                    }
                    else
                        break;
                }
                while (Options_Pos_Before!=Data.size());

                //Filling
                if (option!="options:")
                {
                    if (!Encoded_Library_Settings.empty())
                        Encoded_Library_Settings+=__T(" / ");
                    Encoded_Library_Settings+=Ztring().From_UTF8(option.c_str());
                    if (option.find("bitrate=")==0)
                        BitRate_Nominal.From_UTF8(option.substr(8)+"000"); //After "bitrate="
                }
            }
            while (Options_Pos_Before!=Data.size());
            Element_End0();
        }
        else
        {
            string Value;
            Get_String(Data_Pos-Data_Pos_Before, Value,          "data");

            //Saving
            if (Loop==0)
            {
                //Cleaning a little the value
                while (!Value.empty() && Value[0]<0x30)
                    Value.erase(Value.begin());
                while (!Value.empty() && Value[Value.size()-1]<0x30)
                    Value.erase(Value.end()-1);
                Encoded_Library.From_UTF8(Value.c_str());
            }
            if (Loop==1 && Encoded_Library.find(__T("x264"))==0)
            {
                Encoded_Library+=__T(" - ");
                Encoded_Library+=Ztring().From_UTF8(Value.c_str());
            }
        }
        Data_Pos_Before=Data_Pos;
        if (Data_Pos_Before+3<=Data.size())
        {
            Skip_UTF8(3,                                        "separator");
            Data_Pos_Before+=3;
        }

        Loop++;
    }
    while (Data_Pos_Before!=Data.size());

    //Encoded_Library
    if (Encoded_Library.find(__T("eavc "))==0)
    {
        Encoded_Library_Name=__T("eavc");
        Encoded_Library_Version=Encoded_Library.SubString(__T("eavc "), __T(""));
    }
    else if (Encoded_Library.find(__T("x264 - "))==0)
    {
        Encoded_Library_Name=__T("x264");
        Encoded_Library_Version=Encoded_Library.SubString(__T("x264 - "), __T(""));
    }
    else if (Encoded_Library.find(__T("SUPER(C) by eRightSoft "))==0)
    {
        Encoded_Library_Name=__T("SUPER(C) by eRightSoft");
        Encoded_Library_Date=Encoded_Library.SubString(__T("2000-"), __T(" "));
    }
    else
        Encoded_Library_Name=Encoded_Library;
}

//---------------------------------------------------------------------------
// SEI - 5 - x264
void File_Avc::sei_message_user_data_unregistered_bluray(int32u payloadSize)
{
    if (payloadSize<4)
    {
        Skip_XX(payloadSize,                                    "Unknown");
        return;
    }
    int32u Identifier;
    Get_B4 (Identifier,                                         "Identifier");
    switch (Identifier)
    {
        case 0x47413934 :   sei_message_user_data_registered_itu_t_t35_GA94_03(); return;
        case 0x4D44504D :   sei_message_user_data_unregistered_bluray_MDPM(Element_Size-Element_Offset); return;
        default         :   Skip_XX(Element_Size-Element_Offset, "Unknown");
    }
}

//---------------------------------------------------------------------------
// SEI - 5 - bluray - MDPM
void File_Avc::sei_message_user_data_unregistered_bluray_MDPM(int32u payloadSize)
{
    if (payloadSize<1)
    {
        Skip_XX(payloadSize, "Unknown");
        return;
    }

    Element_Info1("Modified Digital Video Pack Metadata");

    Skip_B1(                                                    "Count");
    payloadSize--;
    string DateTime0, DateTime1, DateTime2, Model0, Model1, Model2;
    int16u MakeName=(int16u)-1;
    Ztring IrisFNumber;
    while (payloadSize >= 5)
    {
        Element_Begin0();
        int8u  ID;
        Get_B1(ID,                                              "ID"); Element_Name(MDPM(ID));
        switch (ID)
        {
            case 0x18:
                        {
                        int16u Year;
                        int8u  MM, Zone_Hours;
                        bool   Zone_Sign, Zone_Minutes;
                        BS_Begin();
                        Mark_0();
                        Skip_SB(                                "DST flag");
                        Get_SB (Zone_Sign,                      "Time zone sign");
                        Get_S1 (4, Zone_Hours,                  "Time zone hours");
                        Get_SB (Zone_Minutes,                   "Time zone half-hour flag");
                        BS_End();
                        Get_B2 (Year,                           "Year");
                        Get_B1 (MM,                             "Month");
                        DateTime0+='0'+(Year>>12);
                        DateTime0+='0'+((Year&0xF00)>>8);
                        DateTime0+='0'+((Year&0xF0)>>4);
                        DateTime0+='0'+(Year&0xF);
                        DateTime0+='-';
                        DateTime0+='0'+((MM&0xF0)>>4);
                        DateTime0+='0'+(MM&0xF);
                        DateTime0+='-';
                        Element_Info1(DateTime0);
                        DateTime2+=Zone_Sign?'-':'+';
                        DateTime2+='0'+Zone_Hours/10;
                        DateTime2+='0'+Zone_Hours%10;
                        DateTime2+=':';
                        DateTime2+=Zone_Minutes?'3':'0';
                        DateTime2+='0';
                        Element_Info1(DateTime2);
                        }
                        break;
            case 0x19:
                        {
                        int8u  DD, hh, mm, ss;
                        Get_B1 (DD,                             "Day");
                        Get_B1 (hh,                             "Hour");
                        Get_B1 (mm,                             "Minute");
                        Get_B1 (ss,                             "Second");
                        DateTime1+='0'+(DD>>4);
                        DateTime1+='0'+(DD&0xF);
                        DateTime1+=' ';
                        DateTime1+='0'+(hh>>4);
                        DateTime1+='0'+(hh&0xF);
                        DateTime1+=':';
                        DateTime1+='0'+(mm>>4);
                        DateTime1+='0'+(mm&0xF);
                        DateTime1+=':';
                        DateTime1+='0'+(ss>>4);
                        DateTime1+='0'+(ss&0xF);
                        Element_Info1(DateTime1);
                        }
                        break;
            case 0x70:
                        consumer_camera_1();
                        break;
            case 0x71:
                        consumer_camera_2();
                        break;
            case 0xA1:
                        {
                        int16u D, N;
                        Get_B2 (D,                              "D");
                        Get_B2 (N,                              "N");
                        IrisFNumber.From_Number(((float64)D)/N, 6);
                        Element_Info1(IrisFNumber);
                        }
                        break;
            case 0xE0:
                        {
                        Get_B2 (MakeName,                       "Name");
                        Skip_B2(                                "Category");
                        Element_Info1(MDPM_MakeName(MakeName));
                        }
                        break;
            case 0xE4:
                        Get_String(4, Model0,                   "Data"); Element_Info1(Model0);
                        break;
            case 0xE5:
                        Get_String(4, Model1,                   "Data"); Element_Info1(Model1);
                        break;
            case 0xE6:
                        Get_String(4, Model2,                   "Data");
                        Model2.erase(Model2.find_last_not_of('\0')+1);
                        Element_Info1(Model2);
                        break;
            default: Skip_B4("Data");
        }
        Element_End0();
        payloadSize -= 5;
    }
    if (payloadSize)
        Skip_XX(payloadSize, "Unknown");

    FILLING_BEGIN();
        if (!Frame_Count && !seq_parameter_sets.empty() && !pic_parameter_sets.empty())
        {
            if (!DateTime0.empty() && !DateTime1.empty())
                Fill(Stream_General, 0, General_Recorded_Date, DateTime0+DateTime1+DateTime2);
            if (MDPM_MakeName(MakeName)[0] || !Model0.empty())
            {
                string Model;
                if (MDPM_MakeName(MakeName)[0])
                {
                    Model=MDPM_MakeName(MakeName);
                    Fill(Stream_General, 0, General_Encoded_Application_CompanyName, Model);
                    if (!Model0.empty())
                        Model+=' ';
                }
                Fill(Stream_General, 0, General_Encoded_Application, Model+Model0+Model1+Model2);
                Fill(Stream_General, 0, General_Encoded_Application_Name, Model0+Model1+Model2);
            }
            Fill(Stream_Video, 0, "IrisFNumber", IrisFNumber);
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Avc::consumer_camera_1()
{
    //Parsing
    BS_Begin();
    int8u ae_mode, wb_mode, white_balance, fcm;
    Mark_1_NoTrustError();
    Mark_1_NoTrustError();
    Skip_S1(6,                                                  "iris");
    Get_S1 (4, ae_mode,                                         "ae mode"); Param_Info1(Dv_consumer_camera_1_ae_mode[ae_mode]);
    Skip_S1(4,                                                  "agc(Automatic Gain Control)");
    Get_S1 (3, wb_mode,                                         "wb mode (white balance mode)"); Param_Info1(Dv_consumer_camera_1_wb_mode[wb_mode]);
    Get_S1 (5, white_balance,                                   "white balance"); Param_Info1(Dv_consumer_camera_1_white_balance(white_balance));
    Get_S1 (1, fcm,                                             "fcm (Focus mode)"); Param_Info1(Dv_consumer_camera_1_fcm[fcm]);
    Skip_S1(7,                                                  "focus (focal point)");
    BS_End();

    /* TODO: need some tweaking
    FILLING_BEGIN();
        if (!Frame_Count)
        {
            Ztring Settings;
            if (ae_mode<0x0F) Settings+=__T("ae mode=")+Ztring(Dv_consumer_camera_1_ae_mode[ae_mode])+__T(", ");
            if (wb_mode<0x08) Settings+=__T("wb mode=")+Ztring(Dv_consumer_camera_1_wb_mode[wb_mode])+__T(", ");
            if (wb_mode<0x1F) Settings+=__T("white balance=")+Ztring(Dv_consumer_camera_1_white_balance(white_balance))+__T(", ");
            Settings+=__T("fcm=")+Ztring(Dv_consumer_camera_1_fcm[fcm]);
            Fill(Stream_Video, 0, "Camera_Settings", Settings);
        }
    FILLING_END();
    */
}

//---------------------------------------------------------------------------
void File_Avc::consumer_camera_2()
{
    //Parsing
    BS_Begin();
    Mark_1_NoTrustError();
    Mark_1_NoTrustError();
    Skip_S1(1,                                                  "vpd");
    Skip_S1(5,                                                  "vertical panning speed");
    Skip_S1(1,                                                  "is");
    Skip_S1(1,                                                  "hpd");
    Skip_S1(6,                                                  "horizontal panning speed");
    Skip_S1(8,                                                  "focal length");
    Skip_S1(1,                                                  "zen");
    Info_S1(3, zoom_U,                                          "units of e-zoom");
    Info_S1(4, zoom_D,                                          "1/10 of e-zoom"); /*if (zoom_D!=0xF)*/ Param_Info1(__T("zoom=")+Ztring().From_Number(zoom_U+((float32)zoom_U)/10, 2));
    BS_End();
}

//---------------------------------------------------------------------------
void File_Avc::sei_message_mastering_display_colour_volume()
{
    Element_Info1("mastering_display_colour_volume");

    std::map<video, Ztring>& SmpteSt2086 = HDR[HdrFormat_SmpteSt2086];
    Ztring& HDR_Format = SmpteSt2086[Video_HDR_Format];
    if (HDR_Format.empty())
    {
        HDR_Format = __T("SMPTE ST 2086");
        SmpteSt2086[Video_HDR_Format_Compatibility] = "HDR10";
    }
    Get_MasteringDisplayColorVolume(SmpteSt2086[Video_MasteringDisplay_ColorPrimaries], SmpteSt2086[Video_MasteringDisplay_Luminance]);
}
//---------------------------------------------------------------------------
void File_Avc::sei_message_light_level()
{
    Element_Info1("light_level");

    //Parsing
    Get_LightLevel(maximum_content_light_level, maximum_frame_average_light_level);
}

//---------------------------------------------------------------------------
// SEI - 6
void File_Avc::sei_message_recovery_point()
{
    Element_Info1("recovery_point");

    //Parsing
    BS_Begin();
    Skip_UE(                                                    "recovery_frame_cnt");
    Skip_SB(                                                    "exact_match_flag");
    Skip_SB(                                                    "broken_link_flag");
    Skip_S1(2,                                                  "changing_slice_group_idc");
    BS_End();
}

//---------------------------------------------------------------------------
// SEI - 32
void File_Avc::sei_message_mainconcept(int32u payloadSize)
{
    Element_Info1("MainConcept text");

    //Parsing
    string Text;
    Get_String(payloadSize, Text,                               "text");

    if (Text.find("produced by MainConcept H.264/AVC Codec v")!=std::string::npos)
    {
        Encoded_Library=Ztring().From_UTF8(Text).SubString(__T("produced by "), __T(" MainConcept AG"));
        Encoded_Library_Name=__T("MainConcept H.264/AVC Codec");
        Encoded_Library_Version= Ztring().From_UTF8(Text).SubString(__T("produced by MainConcept H.264/AVC Codec v"), __T(" (c) "));
        Encoded_Library_Date=MediaInfoLib::Config.Library_Get(InfoLibrary_Format_MainConcept_Avc, Encoded_Library_Version, InfoLibrary_Date);
    }
}

//---------------------------------------------------------------------------
void File_Avc::sei_alternative_transfer_characteristics()
{
    Element_Info1("alternative_transfer_characteristics");

    //Parsing
    Get_B1(preferred_transfer_characteristics, "preferred_transfer_characteristics"); Param_Info1(Mpegv_transfer_characteristics(preferred_transfer_characteristics));
}

//---------------------------------------------------------------------------
// Packet "07"
void File_Avc::seq_parameter_set()
{
    Element_Name("seq_parameter_set");

    //parsing
    int32u seq_parameter_set_id;
    seq_parameter_set_struct* Data_Item_New=seq_parameter_set_data(seq_parameter_set_id);
    if (!Data_Item_New)
        return;
    Mark_1(                                                     );
    size_t BS_bits=Data_BS_Remain()%8;
    while (BS_bits)
    {
        Mark_0(                                                 );
        BS_bits--;
    }
    BS_End();

    //Hack for 00003.m2ts: There is a trailing 0x89, why?
    if (Element_Offset+1==Element_Size)
    {
        int8u ToTest;
        Peek_B1(ToTest);
        if (ToTest==0x98)
            Skip_B1(                                            "Unknown");

    }

    //Hack for : There is a trailing data, why?
    if (Element_Offset+4==Element_Size)
    {
        int32u ToTest;
        Peek_B4(ToTest);
        if (ToTest==0xE30633C0)
            Skip_B4(                                            "Unknown");
    }

    //NULL bytes
    while (Element_Offset<Element_Size)
    {
        int8u Null;
        Get_B1 (Null,                                           "NULL byte");
        if (Null)
            Trusted_IsNot("Should be NULL byte");
    }

    FILLING_BEGIN_PRECISE();
        //NextCode
        NextCode_Clear();
        NextCode_Add(0x08);

        //Add
        seq_parameter_set_data_Add(seq_parameter_sets, seq_parameter_set_id, Data_Item_New);

        //Autorisation of other streams
        Streams[0x08].Searching_Payload=true; //pic_parameter_set
        if (Streams[0x07].ShouldDuplicate)
            Streams[0x08].ShouldDuplicate=true; //pic_parameter_set
        Streams[0x0A].Searching_Payload=true; //end_of_seq
        if (Streams[0x07].ShouldDuplicate)
            Streams[0x0A].ShouldDuplicate=true; //end_of_seq
        Streams[0x0B].Searching_Payload=true; //end_of_stream
        if (Streams[0x07].ShouldDuplicate)
            Streams[0x0B].ShouldDuplicate=true; //end_of_stream
    FILLING_ELSE();
        delete Data_Item_New;
    FILLING_END();
}

void File_Avc::seq_parameter_set_data_Add(std::vector<seq_parameter_set_struct*> &Data, const int32u Data_id, seq_parameter_set_struct* Data_Item_New)
{
    //Creating Data
    if (Data_id>=Data.size())
        Data.resize(Data_id+1);
    else
        FirstPFrameInGop_IsParsed=true;
    std::vector<seq_parameter_set_struct*>::iterator Data_Item=Data.begin()+Data_id;
    delete *Data_Item; *Data_Item=Data_Item_New;

    //Computing values (for speed)
    size_t MaxNumber;
    switch (Data_Item_New->pic_order_cnt_type)
    {
        case 0 :
                    MaxNumber=Data_Item_New->MaxPicOrderCntLsb;
                    break;
        case 1 :
        case 2 :
                    MaxNumber=Data_Item_New->MaxFrameNum*2;
                    break;
        default:
                    MaxNumber = 0;
    }

    if (MaxNumber>TemporalReferences_Reserved)
    {
        TemporalReferences.resize(4*MaxNumber);
        TemporalReferences_Reserved=MaxNumber;
    }
}

//---------------------------------------------------------------------------
// Packet "08"
void File_Avc::pic_parameter_set()
{
    Element_Name("pic_parameter_set");

    //Parsing
    int32u  pic_parameter_set_id, seq_parameter_set_id, num_slice_groups_minus1, num_ref_idx_l0_default_active_minus1, num_ref_idx_l1_default_active_minus1, slice_group_map_type=0;
    int8u   weighted_bipred_idc=0;
    bool    entropy_coding_mode_flag,bottom_field_pic_order_in_frame_present_flag, redundant_pic_cnt_present_flag, weighted_pred_flag, deblocking_filter_control_present_flag;
    BS_Begin();
    Get_UE (pic_parameter_set_id,                               "pic_parameter_set_id");
    Get_UE (seq_parameter_set_id,                               "seq_parameter_set_id");
    std::vector<seq_parameter_set_struct*>::iterator seq_parameter_set_Item;
    if (seq_parameter_set_id>=seq_parameter_sets.size() || (*(seq_parameter_set_Item=seq_parameter_sets.begin()+seq_parameter_set_id))==NULL)
    {
        if (seq_parameter_set_id>=subset_seq_parameter_sets.size() || (*(seq_parameter_set_Item=subset_seq_parameter_sets.begin()+seq_parameter_set_id))==NULL)
        {
            //Not yet present
            Skip_BS(Data_BS_Remain(),                               "Data (seq_parameter_set is missing)");
            return;
        }
    }
    Get_SB (entropy_coding_mode_flag,                           "entropy_coding_mode_flag");
    Get_SB (bottom_field_pic_order_in_frame_present_flag,       "bottom_field_pic_order_in_frame_present_flag");
    Get_UE (num_slice_groups_minus1,                            "num_slice_groups_minus1");
    if (num_slice_groups_minus1>7)
    {
        Trusted_IsNot("num_slice_groups_minus1 too high");
        num_slice_groups_minus1=0;
    }
    if (num_slice_groups_minus1>0)
    {
        Get_UE (slice_group_map_type,                           "slice_group_map_type");
        if (slice_group_map_type==0)
        {
            for (int32u Pos=0; Pos<=num_slice_groups_minus1; Pos++)
                Skip_UE(                                        "run_length_minus1");
        }
        else if (slice_group_map_type==2)
        {
            for (int32u Pos=0; Pos<num_slice_groups_minus1; Pos++)
            {
                Skip_UE(                                        "top_left");
                Skip_UE(                                        "bottom_right");
            }
        }
        else if (slice_group_map_type==3
              || slice_group_map_type==4
              || slice_group_map_type==5)
        {
            Skip_SB(                                            "slice_group_change_direction_flag");
            Skip_UE(                                            "slice_group_change_rate_minus1");
        }
        else if (slice_group_map_type==6)
        {
            int32u pic_size_in_map_units_minus1;
            Get_UE (pic_size_in_map_units_minus1,               "pic_size_in_map_units_minus1");
            if(pic_size_in_map_units_minus1>((*seq_parameter_set_Item)->pic_width_in_mbs_minus1+1)*((*seq_parameter_set_Item)->pic_height_in_map_units_minus1+1))
            {
                Trusted_IsNot("pic_size_in_map_units_minus1 too high");
                return;
            }
            #if defined (__mips__)       || defined (__mipsel__)
                int32u slice_group_id_Size=(int32u)(std::ceil(std::log((double)(num_slice_groups_minus1+1))/std::log((double)10))); //std::log is natural logarithm
            #else
                int32u slice_group_id_Size=(int32u)(std::ceil(std::log((float32)(num_slice_groups_minus1+1))/std::log((float32)10))); //std::log is natural logarithm
            #endif
            for (int32u Pos=0; Pos<=pic_size_in_map_units_minus1; Pos++)
                Skip_BS(slice_group_id_Size,                    "slice_group_id");
        }
    }
    Get_UE (num_ref_idx_l0_default_active_minus1,               "num_ref_idx_l0_default_active_minus1");
    Get_UE (num_ref_idx_l1_default_active_minus1,               "num_ref_idx_l1_default_active_minus1");
    Get_SB (weighted_pred_flag,                                 "weighted_pred_flag");
    Get_S1 (2, weighted_bipred_idc,                             "weighted_bipred_idc");
    Skip_SE(                                                    "pic_init_qp_minus26");
    Skip_SE(                                                    "pic_init_qs_minus26");
    Skip_SE(                                                    "chroma_qp_index_offset");
    Get_SB (deblocking_filter_control_present_flag,             "deblocking_filter_control_present_flag");
    Skip_SB(                                                    "constrained_intra_pred_flag");
    Get_SB (redundant_pic_cnt_present_flag,                     "redundant_pic_cnt_present_flag");
    bool more_rbsp_data=false;
    if (Element_Size)
    {
        int64u Offset=Element_Size-1;
        while (Offset && Buffer[Buffer_Offset+(size_t)Offset]==0x00) //Searching if there are NULL bytes at the end of the data
            Offset--;
        size_t Bit_Pos=7;
        while (Bit_Pos && !(Buffer[Buffer_Offset+(size_t)Offset]&(1<<(7-Bit_Pos))))
            Bit_Pos--;
        if (Data_BS_Remain()>1+(7-Bit_Pos)+(Element_Size-Offset-1)*8)
            more_rbsp_data=true;
    }
    if (more_rbsp_data)
    {
        bool transform_8x8_mode_flag;
        Get_SB (transform_8x8_mode_flag,                        "transform_8x8_mode_flag");
        TEST_SB_SKIP(                                           "pic_scaling_matrix_present_flag");
        for (int8u Pos=0; Pos<6+(transform_8x8_mode_flag?((*seq_parameter_set_Item)->chroma_format_idc!=3?2:6):0); Pos++ )
            {
                TEST_SB_SKIP(                                   "pic_scaling_list_present_flag");
                    scaling_list(Pos<6?16:64);
                TEST_SB_END();
            }
        TEST_SB_END();
        Skip_SE(                                                "second_chroma_qp_index_offset");
    }
    Mark_1(                                                     );
    BS_End();

    while (Element_Offset<Element_Size) //Not always removed from the stream, ie in MPEG-4
    {
        int8u Padding;
        Peek_B1(Padding);
        if (!Padding)
            Skip_B1(                                            "Padding");
        else
            break;
    }

    FILLING_BEGIN_PRECISE();
        //Integrity
        if (pic_parameter_set_id>=256)
        {
            Trusted_IsNot("pic_parameter_set_id not valid");
            return; //Problem, not valid
        }
        if (seq_parameter_set_id>=32)
        {
            Trusted_IsNot("seq_parameter_set_id not valid");
            return; //Problem, not valid
        }

        //NextCode
        NextCode_Clear();
        NextCode_Add(0x05);
        NextCode_Add(0x06);
        if (!subset_seq_parameter_sets.empty())
            NextCode_Add(0x14); //slice_layer_extension

        //Filling
        if (pic_parameter_set_id>=pic_parameter_sets.size())
            pic_parameter_sets.resize(pic_parameter_set_id+1);
        std::vector<pic_parameter_set_struct*>::iterator pic_parameter_sets_Item=pic_parameter_sets.begin()+pic_parameter_set_id;
        delete *pic_parameter_sets_Item; *pic_parameter_sets_Item = new pic_parameter_set_struct(
                                                                                                    (int8u)seq_parameter_set_id,
                                                                                                    (int8u)num_ref_idx_l0_default_active_minus1,
                                                                                                    (int8u)num_ref_idx_l1_default_active_minus1,
                                                                                                    weighted_bipred_idc,
                                                                                                    num_slice_groups_minus1,
                                                                                                    slice_group_map_type,
                                                                                                    entropy_coding_mode_flag,
                                                                                                    bottom_field_pic_order_in_frame_present_flag,
                                                                                                    weighted_pred_flag,
                                                                                                    redundant_pic_cnt_present_flag,
                                                                                                    deblocking_filter_control_present_flag
                                                                                                );

        //Autorisation of other streams
        if (!seq_parameter_sets.empty())
        {
            for (int8u Pos=0x01; Pos<=0x06; Pos++)
            {
                Streams[Pos].Searching_Payload=true; //Coded slice...
                if (Streams[0x08].ShouldDuplicate)
                    Streams[Pos].ShouldDuplicate=true;
            }
        }
        if (!subset_seq_parameter_sets.empty())
        {
            Streams[0x14].Searching_Payload=true; //slice_layer_extension
            if (Streams[0x08].ShouldDuplicate)
                Streams[0x14].ShouldDuplicate=true; //slice_layer_extension
        }

        //Setting as OK
        if (!Status[IsAccepted])
            Accept("AVC");
    FILLING_END();
}

//---------------------------------------------------------------------------
// Packet "09"
void File_Avc::access_unit_delimiter()
{
    Element_Name("access_unit_delimiter");

    int8u primary_pic_type;
    BS_Begin();
    Get_S1 ( 3, primary_pic_type,                               "primary_pic_type"); Param_Info1(Avc_primary_pic_type[primary_pic_type]);
    Mark_1_NoTrustError(                                        ); //Found 1 file without this bit
    BS_End();
}

//---------------------------------------------------------------------------
// Packet "09"
void File_Avc::filler_data()
{
    Element_Name("filler_data");

    while (Element_Offset<Element_Size)
    {
        int8u FF;
        Peek_B1(FF);
        if (FF!=0xFF)
            break;
        Element_Offset++;
    }
    BS_Begin();
    Mark_1(                                                     );
    BS_End();
}

//---------------------------------------------------------------------------
// Packet "0E"
void File_Avc::prefix_nal_unit(bool svc_extension_flag)
{
    Element_Name("prefix_nal_unit");

    //Parsing
    if (svc_extension_flag)
    {
        Skip_XX(Element_Size-Element_Offset,                    "prefix_nal_unit_svc");
    }
}

//---------------------------------------------------------------------------
// Packet "0F"
void File_Avc::subset_seq_parameter_set()
{
    Element_Name("subset_seq_parameter_set");

    //Parsing
    int32u subset_seq_parameter_set_id;
    seq_parameter_set_struct* Data_Item_New=seq_parameter_set_data(subset_seq_parameter_set_id);
    if (!Data_Item_New)
        return;
    if (Data_Item_New->profile_idc==83 || Data_Item_New->profile_idc==86)
    {
        //bool svc_vui_parameters_present_flag;
        seq_parameter_set_svc_extension();
        /* The rest is not yet implemented
        Get_SB (svc_vui_parameters_present_flag,                "svc_vui_parameters_present_flag");
        if (svc_vui_parameters_present_flag)
            svc_vui_parameters_extension();
        */
    }
    else if (Data_Item_New->profile_idc==118 || Data_Item_New->profile_idc==128)
    {
        //bool mvc_vui_parameters_present_flag, additional_extension2_flag;
        Mark_1();
        seq_parameter_set_mvc_extension(Data_Item_New);
        /* The rest is not yet implemented
        Get_SB (mvc_vui_parameters_present_flag,                "mvc_vui_parameters_present_flag");
        if (mvc_vui_parameters_present_flag)
            mvc_vui_parameters_extension();
        Get_SB (additional_extension2_flag,                     "additional_extension2_flag");
        if (additional_extension2_flag)
        {
            //Not handled, should skip all bits except 1
            BS_End();
            return;
        }
        */
    }
    /* The rest is not yet implemented
    Mark_1(                                                     );
    */
    BS_End();

    FILLING_BEGIN();
        //NextCode
        NextCode_Clear();
        NextCode_Add(0x08);

        //Add
        seq_parameter_set_data_Add(subset_seq_parameter_sets, subset_seq_parameter_set_id, Data_Item_New);

        //Autorisation of other streams
        Streams[0x08].Searching_Payload=true; //pic_parameter_set
        if (Streams[0x0F].ShouldDuplicate)
            Streams[0x08].ShouldDuplicate=true; //pic_parameter_set
        Streams[0x0A].Searching_Payload=true; //end_of_seq
        if (Streams[0x0F].ShouldDuplicate)
            Streams[0x0A].ShouldDuplicate=true; //end_of_seq
        Streams[0x0B].Searching_Payload=true; //end_of_stream
        if (Streams[0x0F].ShouldDuplicate)
            Streams[0x0B].ShouldDuplicate=true; //end_of_stream
    FILLING_END();
}

//---------------------------------------------------------------------------
// Packet "14"
void File_Avc::slice_layer_extension(bool svc_extension_flag)
{
    Element_Name("slice_layer_extension");

    //Parsing
    if (svc_extension_flag)
    {
        Skip_XX(Element_Size-Element_Offset,                    "slice_header_in_scalable_extension + slice_data_in_scalable_extension");
    }
    else
    {
        BS_Begin();
        slice_header();
        slice_data(true);
        BS_End();
    }
}

//***************************************************************************
// SubElements
//***************************************************************************

//---------------------------------------------------------------------------
File_Avc::seq_parameter_set_struct* File_Avc::seq_parameter_set_data(int32u &Data_id)
{
    //Parsing
    seq_parameter_set_struct::vui_parameters_struct* vui_parameters_Item=NULL;
    int32u  chroma_format_idc=1, bit_depth_luma_minus8=0, bit_depth_chroma_minus8=0, log2_max_frame_num_minus4, pic_order_cnt_type, log2_max_pic_order_cnt_lsb_minus4=(int32u)-1, max_num_ref_frames, pic_width_in_mbs_minus1, pic_height_in_map_units_minus1, frame_crop_left_offset=0, frame_crop_right_offset=0, frame_crop_top_offset=0, frame_crop_bottom_offset=0;
    int8u   profile_idc, constraint_set_flags, level_idc;
    bool    constraint_set1_flag, constraint_set3_flag, separate_colour_plane_flag=false, delta_pic_order_always_zero_flag=false, frame_mbs_only_flag, mb_adaptive_frame_field_flag=false;
    Get_B1 (profile_idc,                                        "profile_idc");
    Get_B1 (constraint_set_flags,                               "constraint_sett_flags");
        Skip_Flags(constraint_set_flags, 7,                     "constraint_sett0_flag");
        Skip_Flags(constraint_set_flags, 6,                     "constraint_sett1_flag");
        Skip_Flags(constraint_set_flags, 5,                     "constraint_sett2_flag");
        Skip_Flags(constraint_set_flags, 4,                     "constraint_sett3_flag");
        Skip_Flags(constraint_set_flags, 3,                     "constraint_sett4_flag");
        Skip_Flags(constraint_set_flags, 2,                     "constraint_sett5_flag");
        Skip_Flags(constraint_set_flags, 1,                     "constraint_sett6_flag");
        Skip_Flags(constraint_set_flags, 0,                     "constraint_sett7_flag");
    Get_B1 (level_idc,                                          "level_idc");
    BS_Begin();
    Get_UE (    Data_id,                                        "seq_parameter_set_id");
    switch (profile_idc)
    {
        case 100 :
        case 110 :
        case 122 :
        case 244 :
        case  44 :
        case  83 :
        case  86 :
        case 118 :
        case 128 :  //High profiles
        case 138 :
                    Element_Begin1("high profile specific");
                    Get_UE (chroma_format_idc,                  "chroma_format_idc"); Param_Info1C((chroma_format_idc<3), Avc_ChromaSubsampling_format_idc(chroma_format_idc));
                    if (chroma_format_idc==3)
                        Get_SB (separate_colour_plane_flag,     "separate_colour_plane_flag");
                    Get_UE (bit_depth_luma_minus8,              "bit_depth_luma_minus8");
                    Get_UE (bit_depth_chroma_minus8,            "bit_depth_chroma_minus8");
                    Skip_SB(                                    "qpprime_y_zero_transform_bypass_flag");
                    TEST_SB_SKIP(                               "seq_scaling_matrix_present_flag");
                        for (int32u Pos=0; Pos<(int32u)((chroma_format_idc!=3) ? 8 : 12); Pos++)
                        {
                            TEST_SB_SKIP(                       "seq_scaling_list_present_flag");
                                scaling_list(Pos<6?16:64);
                            TEST_SB_END();
                        }
                    TEST_SB_END();
                    Element_End0();
                    break;
        default  :  ;
    }
    Get_UE (log2_max_frame_num_minus4,                          "log2_max_frame_num_minus4");
    Get_UE (pic_order_cnt_type,                                 "pic_order_cnt_type");
    if (pic_order_cnt_type==0)
        Get_UE (log2_max_pic_order_cnt_lsb_minus4,              "log2_max_pic_order_cnt_lsb_minus4");
    else if (pic_order_cnt_type==1)
    {
        int32u num_ref_frames_in_pic_order_cnt_cycle;
        Get_SB (delta_pic_order_always_zero_flag,               "delta_pic_order_always_zero_flag");
        Skip_SE(                                                "offset_for_non_ref_pic");
        Skip_SE(                                                "offset_for_top_to_bottom_field");
        Get_UE (num_ref_frames_in_pic_order_cnt_cycle,          "num_ref_frames_in_pic_order_cnt_cycle");
        if (num_ref_frames_in_pic_order_cnt_cycle>=256)
        {
            Trusted_IsNot("num_ref_frames_in_pic_order_cnt_cycle too high");
            return NULL;
        }
        for(int32u Pos=0; Pos<num_ref_frames_in_pic_order_cnt_cycle; Pos++)
            Skip_SE(                                            "offset_for_ref_frame");
    }
    else if (pic_order_cnt_type > 2)
    {
        Trusted_IsNot("pic_order_cnt_type not supported");
        return NULL;
    }
    Get_UE (max_num_ref_frames,                                 "max_num_ref_frames");
    Skip_SB(                                                    "gaps_in_frame_num_value_allowed_flag");
    Get_UE (pic_width_in_mbs_minus1,                            "pic_width_in_mbs_minus1");
    Get_UE (pic_height_in_map_units_minus1,                     "pic_height_in_map_units_minus1");
    Get_SB (frame_mbs_only_flag,                                "frame_mbs_only_flag");
    if (!frame_mbs_only_flag)
        Get_SB (mb_adaptive_frame_field_flag,                   "mb_adaptive_frame_field_flag");
    Skip_SB(                                                    "direct_8x8_inference_flag");
    TEST_SB_SKIP(                                               "frame_cropping_flag");
        Get_UE (frame_crop_left_offset,                         "frame_crop_left_offset");
        Get_UE (frame_crop_right_offset,                        "frame_crop_right_offset");
        Get_UE (frame_crop_top_offset,                          "frame_crop_top_offset");
        Get_UE (frame_crop_bottom_offset,                       "frame_crop_bottom_offset");
    TEST_SB_END();
    TEST_SB_SKIP(                                               "vui_parameters_present_flag");
        vui_parameters(vui_parameters_Item);
    TEST_SB_END();

    FILLING_BEGIN();
        //Integrity
        if (Data_id>=32)
        {
            Trusted_IsNot("seq_parameter_set_id not valid");
            delete (seq_parameter_set_struct::vui_parameters_struct*)vui_parameters_Item;
            return NULL; //Problem, not valid
        }
        if (pic_order_cnt_type==0 && log2_max_pic_order_cnt_lsb_minus4>12)
        {
            Trusted_IsNot("log2_max_pic_order_cnt_lsb_minus4 not valid");
            delete (seq_parameter_set_struct::vui_parameters_struct*)vui_parameters_Item;
            return NULL; //Problem, not valid
        }
        if (log2_max_frame_num_minus4>12)
        {
            Trusted_IsNot("log2_max_frame_num_minus4 not valid");
            delete (seq_parameter_set_struct::vui_parameters_struct*)vui_parameters_Item;
            return NULL; //Problem, not valid
        }

        //Creating Data
        return new seq_parameter_set_struct(
                                                                    vui_parameters_Item,
                                                                    pic_width_in_mbs_minus1,
                                                                    pic_height_in_map_units_minus1,
                                                                    frame_crop_left_offset,
                                                                    frame_crop_right_offset,
                                                                    frame_crop_top_offset,
                                                                    frame_crop_bottom_offset,
                                                                    (int8u)chroma_format_idc,
                                                                    profile_idc,
                                                                    level_idc,
                                                                    (int8u)bit_depth_luma_minus8,
                                                                    (int8u)bit_depth_chroma_minus8,
                                                                    (int8u)log2_max_frame_num_minus4,
                                                                    (int8u)pic_order_cnt_type,
                                                                    (int8u)log2_max_pic_order_cnt_lsb_minus4,
                                                                    (int8u)max_num_ref_frames,
                                                                    constraint_set_flags,
                                                                    separate_colour_plane_flag,
                                                                    delta_pic_order_always_zero_flag,
                                                                    frame_mbs_only_flag,
                                                                    mb_adaptive_frame_field_flag
                                                                  );
    FILLING_ELSE();
        delete vui_parameters_Item; //vui_parameters_Item=NULL;
        return NULL;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Avc::scaling_list(int32u ScalingList_Size)
{
    //From http://mpeg4ip.cvs.sourceforge.net/mpeg4ip/mpeg4ip/util/h264/main.cpp?revision=1.17&view=markup
    int32u lastScale=8, nextScale=8;
    for (int32u Pos=0; Pos<ScalingList_Size; Pos++)
    {
        if (nextScale!=0)
        {
            int32s delta_scale;
            Get_SE (delta_scale,                                "scale_delta");
            nextScale=(lastScale+delta_scale+256)%256;
        }
        if (nextScale)
            lastScale=nextScale;
    }
}

//---------------------------------------------------------------------------
void File_Avc::vui_parameters(seq_parameter_set_struct::vui_parameters_struct* &vui_parameters_Item_)
{
    //Parsing
    seq_parameter_set_struct::vui_parameters_struct::xxl *NAL=NULL, *VCL=NULL;
    bitset<vui_flags_Max> flags;
    int32u  num_units_in_tick=0, time_scale=0, chroma_sample_loc_type_top_field=(int32u)-1, chroma_sample_loc_type_bottom_field=(int32u)-1;
    int16u  sar_width=0, sar_height=0;
    int8u   video_format=5, colour_primaries=2, transfer_characteristics=2, matrix_coefficients=2;
    bool    nal_hrd_parameters_present_flag, vcl_hrd_parameters_present_flag;
    TEST_SB_SKIP(                                               "aspect_ratio_info_present_flag");
        int8u aspect_ratio_idc;
        Get_S1 (8, aspect_ratio_idc,                            "aspect_ratio_idc");
        if (aspect_ratio_idc==0xFF)
        {
            Get_S2 (16, sar_width,                              "sar_width");
            Get_S2 (16, sar_height,                             "sar_height");
        }
        else if (aspect_ratio_idc && aspect_ratio_idc <= Avc_PixelAspectRatio_Size)
        {
            aspect_ratio_idc--;
            const auto& aspect_ratio = Avc_PixelAspectRatio[aspect_ratio_idc];
            Param_Info1(aspect_ratio.w);
            Param_Info1(aspect_ratio.h);
            sar_width = aspect_ratio.w;
            sar_height = aspect_ratio.h;
        }
    TEST_SB_END();
    TEST_SB_SKIP(                                               "overscan_info_present_flag");
        Skip_SB(                                                "overscan_appropriate_flag");
    TEST_SB_END();
    TEST_SB_SKIP(                                               "video_signal_type_present_flag");
        flags[video_signal_type_present_flag]=true;
        bool video_full_range_flag_;
        Get_S1 (3, video_format,                                "video_format"); Param_Info1(Avc_video_format[video_format]);
        Get_SB (   video_full_range_flag_,                      "video_full_range_flag"); Param_Info1(Avc_video_full_range[video_full_range_flag_]);
        flags[video_full_range_flag]=video_full_range_flag_;
        TEST_SB_SKIP(                                           "colour_description_present_flag");
            flags[colour_description_present_flag]=true;
            Get_S1 (8, colour_primaries,                        "colour_primaries"); Param_Info1(Mpegv_colour_primaries(colour_primaries));
            Get_S1 (8, transfer_characteristics,                "transfer_characteristics"); Param_Info1(Mpegv_transfer_characteristics(transfer_characteristics));
            Get_S1 (8, matrix_coefficients,                     "matrix_coefficients"); Param_Info1(Mpegv_matrix_coefficients(matrix_coefficients));
        TEST_SB_END();
    TEST_SB_END();
    TEST_SB_SKIP(                                               "chroma_loc_info_present_flag");
        Get_UE (chroma_sample_loc_type_top_field,               "chroma_sample_loc_type_top_field");
        Get_UE (chroma_sample_loc_type_bottom_field,            "chroma_sample_loc_type_bottom_field");
    TEST_SB_END();
    TEST_SB_SKIP(                                               "timing_info_present_flag");
        flags[timing_info_present_flag]=true;
        bool fixed_frame_rate_flag_;
        Get_S4 (32, num_units_in_tick,                          "num_units_in_tick");
        Get_S4 (32, time_scale,                                 "time_scale");
        Get_SB (    fixed_frame_rate_flag_,                     "fixed_frame_rate_flag");
        flags[fixed_frame_rate_flag]=fixed_frame_rate_flag_;
    TEST_SB_END();
    TEST_SB_GET (nal_hrd_parameters_present_flag,               "nal_hrd_parameters_present_flag");
        hrd_parameters(NAL);
    TEST_SB_END();
    TEST_SB_GET (vcl_hrd_parameters_present_flag,               "vcl_hrd_parameters_present_flag");
        hrd_parameters(VCL);
    TEST_SB_END();
    if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag)
        Skip_SB(                                                "low_delay_hrd_flag");
    bool pic_struct_present_flag_;
    Get_SB (   pic_struct_present_flag_,                        "pic_struct_present_flag");
    flags[pic_struct_present_flag]=pic_struct_present_flag_;
    TEST_SB_SKIP(                                               "bitstream_restriction_flag");
        int32u  max_num_reorder_frames;
        Skip_SB(                                                "motion_vectors_over_pic_boundaries_flag");
        Skip_UE(                                                "max_bytes_per_pic_denom");
        Skip_UE(                                                "max_bits_per_mb_denom");
        Skip_UE(                                                "log2_max_mv_length_horizontal");
        Skip_UE(                                                "log2_max_mv_length_vertical");
        Get_UE (max_num_reorder_frames,                         "max_num_reorder_frames");
        Skip_UE(                                                "max_dec_frame_buffering");
    TEST_SB_END();

    FILLING_BEGIN();
        vui_parameters_Item_=new seq_parameter_set_struct::vui_parameters_struct(
                                                                                    NAL,
                                                                                    VCL,
                                                                                    num_units_in_tick,
                                                                                    time_scale,
                                                                                    chroma_sample_loc_type_top_field,
                                                                                    chroma_sample_loc_type_bottom_field,
                                                                                    sar_width,
                                                                                    sar_height,
                                                                                    video_format,
                                                                                    colour_primaries,
                                                                                    transfer_characteristics,
                                                                                    matrix_coefficients,
                                                                                    flags
                                                                                );
    FILLING_ELSE();
        delete NAL; NAL=NULL;
        delete VCL; VCL=NULL;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Avc::hrd_parameters(seq_parameter_set_struct::vui_parameters_struct::xxl* &hrd_parameters_Item_)
{
    //Parsing
    int32u cpb_cnt_minus1;
    int8u  bit_rate_scale, cpb_size_scale, initial_cpb_removal_delay_length_minus1, cpb_removal_delay_length_minus1, dpb_output_delay_length_minus1, time_offset_length;
    Get_UE (   cpb_cnt_minus1,                                  "cpb_cnt_minus1");
    Get_S1 (4, bit_rate_scale,                                  "bit_rate_scale");
    Get_S1 (4, cpb_size_scale,                                  "cpb_size_scale");
    if (cpb_cnt_minus1>31)
    {
        Trusted_IsNot("cpb_cnt_minus1 too high");
        cpb_cnt_minus1=0;
    }
    vector<seq_parameter_set_struct::vui_parameters_struct::xxl::xxl_data>  SchedSel;
    SchedSel.reserve(cpb_cnt_minus1+1);
    for (int8u SchedSelIdx = 0; SchedSelIdx <= cpb_cnt_minus1; ++SchedSelIdx)
    {
        Element_Begin1("ShedSel");
        int64u bit_rate_value, cpb_size_value;
        int32u bit_rate_value_minus1, cpb_size_value_minus1;
        bool cbr_flag;
        Get_UE (bit_rate_value_minus1,                          "bit_rate_value_minus1");
        bit_rate_value=(int64u)((bit_rate_value_minus1+1)*pow(2.0, 6+bit_rate_scale)); Param_Info2(bit_rate_value, " bps");
        Get_UE (cpb_size_value_minus1,                          "cpb_size_value_minus1");
        cpb_size_value=(int64u)((cpb_size_value_minus1+1)*pow(2.0, 4+cpb_size_scale)); Param_Info2(cpb_size_value, " bits");
        Get_SB (cbr_flag,                                       "cbr_flag");
        Element_End0();

        FILLING_BEGIN();
            SchedSel.push_back(seq_parameter_set_struct::vui_parameters_struct::xxl::xxl_data(
                                                                                                bit_rate_value,
                                                                                                cpb_size_value,
                                                                                                cbr_flag
                                                                                             ));
        FILLING_END();
    }
    Get_S1 (5, initial_cpb_removal_delay_length_minus1,         "initial_cpb_removal_delay_length_minus1");
    Get_S1 (5, cpb_removal_delay_length_minus1,                 "cpb_removal_delay_length_minus1");
    Get_S1 (5, dpb_output_delay_length_minus1,                  "dpb_output_delay_length_minus1");
    Get_S1 (5, time_offset_length,                              "time_offset_length");

    //Validity test
    if (!Element_IsOK() || (SchedSel.size() == 1 && SchedSel[0].bit_rate_value == 64))
    {
        return; //We do not trust this kind of data
    }

    //Filling
    hrd_parameters_Item_=new seq_parameter_set_struct::vui_parameters_struct::xxl(
                                                                                    SchedSel,
                                                                                    initial_cpb_removal_delay_length_minus1,
                                                                                    cpb_removal_delay_length_minus1,
                                                                                    dpb_output_delay_length_minus1,
                                                                                    time_offset_length
                                                                                 );
}

//---------------------------------------------------------------------------
void File_Avc::nal_unit_header_svc_extension()
{
    //Parsing
    Element_Begin1("nal_unit_header_svc_extension");
    Skip_SB(                                                    "idr_flag");
    Skip_S1( 6,                                                 "priority_id");
    Skip_SB(                                                    "no_inter_layer_pred_flag");
    Skip_S1( 3,                                                 "dependency_id");
    Skip_S1( 4,                                                 "quality_id");
    Skip_S1( 3,                                                 "temporal_id");
    Skip_SB(                                                    "use_ref_base_pic_flag");
    Skip_SB(                                                    "discardable_flag");
    Skip_SB(                                                    "output_flag");
    Skip_S1( 2,                                                 "reserved_three_2bits");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Avc::nal_unit_header_mvc_extension()
{
    //Parsing
    Element_Begin1("nal_unit_header_mvc_extension");
    Skip_SB(                                                    "non_idr_flag");
    Skip_S1( 6,                                                 "priority_id");
    Skip_S1(10,                                                 "view_id");
    Skip_S1( 3,                                                 "temporal_id");
    Skip_SB(                                                    "anchor_pic_flag");
    Skip_SB(                                                    "inter_view_flag");
    Skip_SB(                                                    "reserved_one_bit");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Avc::seq_parameter_set_svc_extension()
{
    //Parsing
    Element_Begin1("seq_parameter_set_svc_extension");
    //Skip_SB(                                                    "");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Avc::svc_vui_parameters_extension()
{
    //Parsing
    Element_Begin1("svc_vui_parameters_extension");
    //Skip_SB(                                                    "");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Avc::seq_parameter_set_mvc_extension(seq_parameter_set_struct* Data_Item)
{
    //Parsing
    Element_Begin1("seq_parameter_set_mvc_extension");
    int32u num_views_minus1;
    Get_UE (num_views_minus1,                                   "num_views_minus1");
    //(Not implemented)
    Element_End0();

    FILLING_BEGIN();
        Data_Item->num_views_minus1 = (int16u)num_views_minus1;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Avc::mvc_vui_parameters_extension()
{
    //Parsing
    Element_Begin1("mvc_vui_parameters_extension");
    Skip_SB(                                                    "");
    Element_End0();
}

//***************************************************************************
// Specific
//***************************************************************************

//---------------------------------------------------------------------------
void File_Avc::SPS_PPS()
{
    //Parsing
    int8u Profile, Level, seq_parameter_set_count, pic_parameter_set_count;
    if (SizedBlocks)
        Skip_B1(                                                "configurationVersion");
    Get_B1 (Profile,                                            "AVCProfileIndication");
    Skip_B1(                                                    "profile_compatibility");
    Get_B1 (Level,                                              "AVCLevelIndication");
    BS_Begin();
    Skip_S1(6,                                                  "reserved");
    Get_S1 (2, SizeOfNALU_Minus1,                               "lengthSizeMinusOne");
    Skip_S1(3,                                                  "reserved");
    Get_S1 (5, seq_parameter_set_count,                         "numOfSequenceParameterSets");
    BS_End();
    for (int8u Pos=0; Pos<seq_parameter_set_count; Pos++)
    {
        Element_Begin1("seq_parameter_set");
        int16u Size;
        Get_B2 (Size,                                           "sequenceParameterSetLength");
        if (!Size || Size>Element_Size-Element_Offset)
        {
            Trusted_IsNot("Size is wrong");
            break; //There is an error
        }
        BS_Begin();
        Mark_0 ();
        Skip_S1( 2,                                             "nal_ref_idc");
        Skip_S1( 5,                                             "nal_unit_type");
        BS_End();
        if (!Trusted_Get())
            break;
        int64u Element_Offset_Save=Element_Offset;
        int64u Element_Size_Save=Element_Size;
        Buffer_Offset+=(size_t)Element_Offset_Save;
        Element_Offset=0;
        Element_Size=Size-(Size?1:0);
        Element_Code=0x07; //seq_parameter_set
        Data_Parse();
        Buffer_Offset-=(size_t)Element_Offset_Save;
        Element_Offset=Element_Offset_Save+Size-1;
        Element_Size=Element_Size_Save;
        Element_End0();
    }
    Get_B1 (pic_parameter_set_count,                            "numOfPictureParameterSets");
    for (int8u Pos=0; Pos<pic_parameter_set_count; Pos++)
    {
        Element_Begin1("pic_parameter_set");
        int16u Size;
        Get_B2 (Size,                                           "pictureParameterSetLength");
        if (!Size || Size>Element_Size-Element_Offset)
        {
            Trusted_IsNot("Size is wrong");
            break; //There is an error
        }
        BS_Begin();
        Mark_0 ();
        Skip_S1( 2,                                             "nal_ref_idc");
        Skip_S1( 5,                                             "nal_unit_type");
        BS_End();
        if (!Trusted_Get())
            break;
        int64u Element_Offset_Save=Element_Offset;
        int64u Element_Size_Save=Element_Size;
        Buffer_Offset+=(size_t)Element_Offset_Save;
        Element_Offset=0;
        Element_Size=Size-1;
        Element_Code=0x08; //pic_parameter_set
        Data_Parse();
        Buffer_Offset-=(size_t)Element_Offset_Save;
        Element_Offset=Element_Offset_Save+Size-1;
        Element_Size=Element_Size_Save;
        Element_End0();
    }
    if (Element_Offset<Element_Size)
    {
        switch (Profile)
        {
            case 100:
            case 110:
            case 122:
            case 144:
                        {
                        int8u numOfSequenceParameterSetExt;
                        BS_Begin();
                        Skip_S1( 6,                             "reserved");
                        Skip_S1( 2,                             "chroma_format");
                        Skip_S1( 5,                             "reserved");
                        Skip_S1( 3,                             "bit_depth_luma_minus8");
                        Skip_S1( 5,                             "reserved");
                        Skip_S1( 3,                             "bit_depth_chroma_minus8");
                        BS_End();
                        Get_B1 (numOfSequenceParameterSetExt,   "numOfSequenceParameterSetExt");
                        for (int8u Pos=0; Pos<numOfSequenceParameterSetExt; Pos++)
                        {
                            Element_Begin1("sequenceParameterSetExtNALUnit");
                            int16u Size;
                            Get_B2 (Size,                       "sequenceParameterSetExtLength");
                            BS_Begin();
                            Mark_0 ();
                            Skip_S1( 2,                         "nal_ref_idc");
                            Skip_S1( 5,                         "nal_unit_type");
                            BS_End();
                            int64u Element_Offset_Save=Element_Offset;
                            int64u Element_Size_Save=Element_Size;
                            Buffer_Offset+=(size_t)Element_Offset_Save;
                            Element_Offset=0;
                            Element_Size=Size-1;
                            if (Element_Size>Element_Size_Save-Element_Offset_Save)
                                break; //There is an error
                            Element_Code=0x0F; //subset_seq_parameter_set
                            Data_Parse();
                            Buffer_Offset-=(size_t)Element_Offset_Save;
                            Element_Offset=Element_Offset_Save+Size-1;
                            Element_Size=Element_Size_Save;
                            Element_End0();
                        }
                        }
            default:;
        }
    }
    if (Element_Offset<Element_Size)
        Skip_XX(Element_Size-Element_Offset,                        "Padding?");

    //Filling
    FILLING_BEGIN_PRECISE();
        //Detection of some bugs in the file
        if (!seq_parameter_sets.empty() && seq_parameter_sets[0] && (Profile!=seq_parameter_sets[0]->profile_idc || Level!=seq_parameter_sets[0]->level_idc))
            MuxingMode=Ztring("Container profile=")+Ztring().From_UTF8(Avc_profile_level_string(Profile, Level));

        MustParse_SPS_PPS=false;
        if (!Status[IsAccepted])
            Accept("AVC");
    FILLING_ELSE();
        Frame_Count_NotParsedIncluded--;
        RanOutOfData("AVC");
        Frame_Count_NotParsedIncluded++;
    FILLING_END();
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
std::string File_Avc::GOP_Detect (std::string PictureTypes)
{
    //Finding a string without blanks
    size_t PictureTypes_Limit=PictureTypes.find(' ');
    if (PictureTypes_Limit!=string::npos)
    {
        if (PictureTypes_Limit>PictureTypes.size()/2)
            PictureTypes.resize(PictureTypes_Limit);
        else
        {
            //Trim
            size_t TrimPos;
            TrimPos=PictureTypes.find_first_not_of(' ');
            if (TrimPos!=string::npos)
                PictureTypes.erase(0, TrimPos);
            TrimPos=PictureTypes.find_last_not_of(' ');
            if (TrimPos!=string::npos)
                PictureTypes.erase(TrimPos+1);

            //Finding the longest string
            ZtringList List; List.Separator_Set(0, __T(" "));
            List.Write(Ztring().From_UTF8(PictureTypes));
            size_t MaxLength=0;
            size_t MaxLength_Pos=0;
            for (size_t Pos=0; Pos<List.size(); Pos++)
                if (List[Pos].size()>MaxLength)
                {
                    MaxLength=List[Pos].size();
                    MaxLength_Pos=Pos;
                }
            PictureTypes=List[MaxLength_Pos].To_UTF8();

        }
    }

    //Creating all GOP values
    std::vector<Ztring> GOPs;
    size_t GOP_Frame_Count=0;
    size_t GOP_BFrames_Max=0;
    size_t I_Pos1=PictureTypes.find('I');
    while (I_Pos1!=std::string::npos)
    {
        size_t I_Pos2=PictureTypes.find('I', I_Pos1+1);
        if (I_Pos2!=std::string::npos)
        {
            std::vector<size_t> P_Positions;
            size_t P_Position=I_Pos1;
            do
            {
                P_Position=PictureTypes.find('P', P_Position+1);
                if (P_Position<I_Pos2)
                    P_Positions.push_back(P_Position);
            }
            while (P_Position<I_Pos2);
            if (P_Positions.size()>1 && P_Positions[0]>I_Pos1+1 && P_Positions[P_Positions.size()-1]==I_Pos2-1)
                P_Positions.resize(P_Positions.size()-1); //Removing last P-Frame for next test, this is often a terminating P-Frame replacing a B-Frame
            Ztring GOP;
            bool IsOK=true;
            if (!P_Positions.empty())
            {
                size_t Delta=P_Positions[0]-I_Pos1;
                for (size_t Pos=1; Pos<P_Positions.size(); Pos++)
                    if (P_Positions[Pos]-P_Positions[Pos-1]!=Delta)
                    {
                        IsOK=false;
                        break;
                    }
                if (IsOK)
                {
                    GOP+=__T("M=")+Ztring::ToZtring(P_Positions[0]-I_Pos1)+__T(", ");
                    if (P_Positions[0]-I_Pos1>GOP_BFrames_Max)
                        GOP_BFrames_Max=P_Positions[0]-I_Pos1;
                }
            }
            if (IsOK)
            {
                GOP+=__T("N=")+Ztring::ToZtring(I_Pos2-I_Pos1);
                GOPs.push_back(GOP);
            }
            else
                GOPs.push_back(Ztring()); //There is a problem, blank
            GOP_Frame_Count+=I_Pos2-I_Pos1;
        }
        I_Pos1=I_Pos2;
    }

    //Some clean up
    if (GOP_Frame_Count+GOP_BFrames_Max>Frame_Count && !GOPs.empty())
        GOPs.resize(GOPs.size()-1); //Removing the last one, there may have uncomplete B-frame filling
    if (GOPs.size()>4)
        GOPs.erase(GOPs.begin()); //Removing the first one, it is sometime different and we have enough to deal with

    //Filling
    if (GOPs.size()>=4)
    {
        bool IsOK=true;
        for (size_t Pos=1; Pos<GOPs.size(); Pos++)
            if (GOPs[Pos]!=GOPs[0])
            {
                IsOK=false;
                break;
            }
        if (IsOK)
            return GOPs[0].To_UTF8();
    }

    return string();
}

//---------------------------------------------------------------------------
std::string File_Avc::ScanOrder_Detect (std::string ScanOrders)
{
    //Finding a string without blanks
    size_t ScanOrders_Limit=ScanOrders.find(' ');
    if (ScanOrders_Limit!=string::npos)
    {
        if (ScanOrders_Limit>ScanOrders.size()/2)
            ScanOrders.resize(ScanOrders_Limit);
        else
        {
            //Trim
            size_t TrimPos;
            TrimPos=ScanOrders.find_first_not_of(' ');
            if (TrimPos!=string::npos)
                ScanOrders.erase(0, TrimPos);
            TrimPos=ScanOrders.find_last_not_of(' ');
            if (TrimPos!=string::npos)
                ScanOrders.erase(TrimPos+1);

            //Finding the longest string
            ZtringList List; List.Separator_Set(0, __T(" "));
            List.Write(Ztring().From_UTF8(ScanOrders));
            size_t MaxLength=0;
            size_t MaxLength_Pos=0;
            for (size_t Pos=0; Pos<List.size(); Pos++)
                if (List[Pos].size()>MaxLength)
                {
                    MaxLength=List[Pos].size();
                    MaxLength_Pos=Pos;
                }
            ScanOrders=List[MaxLength_Pos].To_UTF8();

        }
    }

    //Filling
    if (ScanOrders.find("TBTBTBTB")==0)
        return ("TFF");
    if (ScanOrders.find("BTBTBTBT")==0)
        return ("BFF");
    return string();
}

} //NameSpace

#endif //MEDIAINFO_AVC_YES

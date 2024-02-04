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
using namespace ZenLib;
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_MPEGPS_YES) || defined(MEDIAINFO_MPEGTS_YES) || defined(MEDIAINFO_VVC_YES)
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
extern const char* Vvc_tier_flag(bool tier_flag)
{
    return tier_flag ? "High" : "Main";
}

//---------------------------------------------------------------------------
extern const char* Vvc_profile_idc(int32u profile_idc)
{
    switch (profile_idc)
    {
        case   1 : return "Main";
        case   2 : return "Main 10";
        case   3 : return "Main Still";
        case   4 : return "Format Range"; // extensions
        case   5 : return "High Throughput";
        case   6 : return "Multiview Main";
        case   7 : return "Scalable Main"; // can be "Scalable Main 10" depending on general_max_8bit_constraint_flag
        case   8 : return "3D Main";
        case   9 : return "Screen Content"; // coding extensions
        case  10 : return "Scalable Format Range"; // extensions
        default  : return "";
    }
}

} //NameSpace

//---------------------------------------------------------------------------
#endif //...
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_VVC_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Video/File_Vvc.h"
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Info
//***************************************************************************

//---------------------------------------------------------------------------
static const int8u Vvc_profile_idc_Values[] =
{
    1,
    2,
    10,
    17,
    33,
    34,
    35,
    42,
    43,
    49,
    65,
    66,
    97,
    98,
    99,
};
static const char* const Vvc_profile_idc_Names[] =
{
    "Main 10",
    "Main 12",
    "Main 12 Intra",
    "Multilayer Main 10",
    "Main 10 4:4:4",
    "Main 12 4:4:4",
    "Main 16 4:4:4",
    "Main 12 4:4:4 Intra",
    "Main 16 4:4:4 Intra",
    "Multilayer Main 10 4:4:4",
    "Main 10 Still Picture",
    "Main 12 Still Picture",
    "Main 10 4:4:4 Still Picture",
    "Main 12 4:4:4 Still Picture",
    "Main 16 4:4:4 Still Picture",
};
static_assert(sizeof(Vvc_profile_idc_Values)/sizeof(Vvc_profile_idc_Values[0])==sizeof(Vvc_profile_idc_Names)/sizeof(Vvc_profile_idc_Names[0]), "");
string Vvc_profile_idc(int8u profile_idc)
{
    for (size_t i=0; i<sizeof(Vvc_profile_idc_Values)/sizeof(Vvc_profile_idc_Values[0]); i++)
        if (Vvc_profile_idc_Values[i]==profile_idc)
            return Vvc_profile_idc_Names[i];
    return to_string(profile_idc);
}

//---------------------------------------------------------------------------
string Vvc_level_idc(int8u level_idc)
{
    return to_string(level_idc>>4)+'.'+to_string((level_idc&0xF)/3);
}

//---------------------------------------------------------------------------
string Vvc_profile_level_tier_string(int8u profile_idc, int8u level_idc, bool tier_flag)
{
    string Result;
    if (profile_idc && profile_idc!=(int8u)-1)
        Result=Vvc_profile_idc(profile_idc);
    if (level_idc && level_idc!=(int8u)-1)
    {
        if (profile_idc && profile_idc!=(int8u)-1)
            Result+='@';
        Result+='L';
        Result+=Vvc_level_idc(level_idc);
        Result+='@';
        Result+=Vvc_tier_flag(tier_flag);
    }
    return Result;
}

} //NameSpace

#endif //MEDIAINFO_VVC_YES

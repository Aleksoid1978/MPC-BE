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
#if defined(MEDIAINFO_AC4_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Ac4.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include <cfloat>
#include <cmath>
#include <set>

using namespace ZenLib;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

typedef File_Ac4::dmx::cdmx::gain::type gain;
    
//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
// CRC_16_Table
extern const int16u CRC_16_Table[256];

//---------------------------------------------------------------------------
extern const float64 Ac4_frame_rate[2][16]=
{
    { //44.1 kHz
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)11025/(float64)512,
        (float64)0,
        (float64)0,
    },
    { //48 kHz
        (float64)24000/(float64)1001,
        (float64)24,
        (float64)25,
        (float64)30000/(float64)1001,
        (float64)30,
        (float64)48000/(float64)1001,
        (float64)48,
        (float64)50,
        (float64)60000/(float64)1001,
        (float64)60,
        (float64)100,
        (float64)120000/(float64)1001,
        (float64)120,
        (float64)12000/(float64)512,
        (float64)0,
        (float64)0,
    },
};

//---------------------------------------------------------------------------
typedef const float sized_array_float[];
string Value(const sized_array_float Array, size_t Pos, size_t AfterComma=3)
{
    if (Pos++>=(size_t)Array[0] || !Array[Pos])
        return "Index "+Ztring::ToZtring(Pos).To_UTF8();
    if (Array[Pos]==-FLT_MAX)
        return "-inf";
    if (Array[Pos]==FLT_MAX)
        return "inf";
    return Ztring(Ztring::ToZtring(Array[Pos], AfterComma)).To_UTF8();
}
typedef const char* sized_array_string[];
string Value(const sized_array_string Array, size_t Pos)
{
    if (Pos++>=(size_t)Array[0] || !Array[Pos])
        return Ztring::ToZtring(Pos-1).To_UTF8();
    return Array[Pos];
}
bool Value_IsEmpty(const sized_array_string Array, size_t Pos)
{
    if (Pos++>=(size_t)Array[0] || !Array[Pos])
        return false;
    return !Array[Pos][0];
}

static const int16u Ac4_fs_index[2]=
{
    44100,
    48000,
};

static const variable_size Ac4_channel_mode[]=
{
    {13, 0}, // Total size
    {1, 0x00}, // 0b0
    {1, 0x01}, // 0b10
    {2, 0x0C}, // 0b1100
    {0, 0x0D}, // 0b1101
    {0, 0x0E}, // 0b1110
    {3, 0x78}, // 0b1111000
    {0, 0x79}, // 0b1111001
    {0, 0x7A}, // 0b1111010
    {0, 0x7B}, // 0b1111011
    {0, 0x7C}, // 0b1111100
    {0, 0x7D}, // 0b1111101
    {0, 0x7E}, // 0b1111110
    {0, 0x7F}, // 0b1111111
};

static const variable_size Ac4_channel_mode2[]=
{
    {17, 0}, // Total size
    {1, 0x000}, // 0b0
    {1, 0x002}, // 0b10
    {2, 0x00C}, // 0b1100
    {0, 0x00D}, // 0b1101
    {0, 0x00E}, // 0b1110
    {3, 0x078}, // 0b1111000
    {0, 0x079}, // 0b1111001
    {0, 0x07A}, // 0b1111010
    {0, 0x07B}, // 0b1111011
    {0, 0x07C}, // 0b1111100
    {0, 0x07D}, // 0b1111101
    {1, 0x0FC}, // 0b11111100
    {0, 0x0FD}, // 0b11111101
    {1, 0x1FC}, // 0b111111100
    {0, 0x1FD}, // 0b111111101
    {0, 0x1FE}, // 0b111111110
    {0, 0x1FF}, // 0b111111111
};

enum content_classifier
{
    CM,
    ME,
    VI,
    HI,
    D,
    Co,
    E,
    VO,
    A, //Generic "Associate"
};
static const sized_array_string Ac4_content_classifier=
{
    (const char*)9,
    "Main",
    "Music and Effects",
    "Visually Impaired",
    "Hearing Impaired",
    "Dialogue",
    "Commentary",
    "Emergency",
    "Voice Over",
    "Associate", //Generic "Associate"
};

static const sized_array_string Ac4_ch_mode_String=
{
    (const char*)16,
    "Mono",
    "Stereo",
    "3.0",
    "5.0",
    "5.1",
    "7.0 3/4/0",
    "7.1 3/4/0.1",
    "7.0 5/2/0",
    "7.1 5/2/0.1",
    "7.0 3/2/2",
    "7.1 3/2/2.1",
    "7.0.4",
    "7.1.4",
    "9.0.4",
    "9.1.4",
    "22.2",
};
enum ch
{
    L    = 1 << 0,
    R    = 1 << 1,
    C    = 1 << 2,
    LFE  = 1 << 3,
    Ls   = 1 << 4,
    Rs   = 1 << 5,
    Lb   = 1 << 6,
    Rb   = 1 << 7,
    Tfl  = 1 << 8,
    Tfr  = 1 << 9,
    Tl   = 1 << 10,
    Tr   = 1 << 11,
    Tbl  = 1 << 12,
    Tbr  = 1 << 13,
    Lw   = 1 << 14,
    Rw   = 1 << 15,
    LFE2 = 1 << 16,
    // Not in nonstd_bed_channel_assignment
    Cb   = 1 << 17,
    Lscr = 1 << 18,
    Rscr = 1 << 19,
    Tsl  = 1 << 20,
    Tsr  = 1 << 21,
    Tc   = 1 << 22,
    Bfl  = 1 << 23,
    Bfr  = 1 << 24,
    Bfc  = 1 << 25,
    Tfc  = 1 << 26,
    Tbc  = 1 << 27,
    Vhl  = 1 << 26,
    Vhr  = 1 << 27,
    ch_Max = 1 << 31
};

static const size_t Ac4_channel_mask_Size=19;
static ch Ac4_channel_mask[Ac4_channel_mask_Size][2] =
{
    {L,     R},
    {C,     ch_Max},
    {Ls,    Rs},
    {Lb,    Rb},
    {Tfl,   Tfr},
    {Tbl,   Tbr},
    {LFE,   ch_Max},
    {Tl,    Tr},
    {Tsl,   Tsr},
    {Tfc,   ch_Max},
    {Tbc,   ch_Max},
    {Tc,    ch_Max},
    {LFE2,  ch_Max},
    {Bfl,   Bfr},
    {Bfc,   ch_Max},
    {Cb,    ch_Max},
    {Lscr,  Rscr},
    {Lw,    Rw},
    {Vhl,   Vhr},
};

static const sized_array_string Ac4_immersive_stereo_String=
{
    (const char*)2,
    "Multichannel Content",
    "Dolby Atmos Content",
};

static int32u Ac4_ch_mode_2_nonstd_Values[16]=
{
    0,                                          //Mono
    L | R,                                      //Stereo
    L | R | C,                                  //3.0
    L | R | C | Ls | Rs,                        //5.0
    L | R | C | Ls | Rs | LFE,                  //5.1
    L | R | C | Ls | Rs | Lb | Rb,              //7.0 3/4/0
    L | R | C | Ls | Rs | Lb | Rb | LFE,        //7.1 3/4/0
    L | R | C | Ls | Rs | Lw | Rw,              //7.0 5/2/0
    L | R | C | Ls | Rs | Lw | Rw | LFE,        //7.1 5/2/0
    L | R | C | Ls | Rs | Vhl | Vhl,            //7.0 3/2/2
    L | R | C | Ls | Rs | Vhl | Vhl | LFE,      //7.1 3/2/2
    L | R | C | Ls | Rs | Lb | Rb | Tfl | Tfr | Tbl | Tbr,                      //7.0.4
    L | R | C | Ls | Rs | Lb | Rb | Tfl | Tfr | Tbl | Tbr | LFE,                //7.1.4
    L | R | C | Ls | Rs | Lb | Rb | Tfl | Tfr | Tbl | Tbr | Lscr | Rscr,        //9.0.4
    L | R | C | Ls | Rs | Lb | Rb | Tfl | Tfr | Tbl | Tbr | Lscr | Rscr | LFE,  //9.1.4
    L | R | C | Ls | Rs | Lb | Rb | Lw | Rw | Cb | Tfc | Tbc | Tfl | Tfr | Tbl | Tbr | Tsl | Tsr | Tc | Bfl | Bfr | Bfc | LFE | LFE2,  //22.2
};
static int32u Ac4_ch_mode_2_nonstd(int8u ch_mode, bool b_4_back_channels_present=false, bool b_centre_present=false, int8u top_channels_present=0)
{
    if (ch_mode>15)
        return (int32u)-1;
    int32u Value=Ac4_ch_mode_2_nonstd_Values[ch_mode];
    if (ch_mode>=11 && ch_mode<=14)
    {
        if (!b_4_back_channels_present)
            Value&=~(Lb | Rb);
        if (!b_centre_present)
            Value&=~(LFE);
        if (top_channels_present<=1)
            Value&=~(Tbl | Tbr);
        if (!top_channels_present || top_channels_present==2)
            Value&=~(Tfl | Tfr);
    }
    return Value;
}
static string Ac4_nonstd_2_ch_mode_String(int32u nonstd_bed_channel_assignment_mask)
{
    if (!nonstd_bed_channel_assignment_mask)
        return "Mono";
    
    string ToReturn="0.0.0";

    for (int8u i=0; i<17; i++)
    {
        int32u Value=nonstd_bed_channel_assignment_mask&(1<<i);
        if (Value)
        {
            int8u Pos;
            switch (Value)
            {
                case LFE:
                case LFE2:
                    Pos=2;
                    break;
                case Tfl:
                case Tfr:
                case Tbr:
                case Tbl:
                    Pos=4;
                    break;
                default:
                    Pos=0;
            }
            if (ToReturn[Pos]=='9')
                ToReturn[Pos]='A'; 
            else
                ToReturn[Pos]++;
        }
    }

    if (ToReturn[4]=='0')
        ToReturn.resize(3);
    switch (nonstd_bed_channel_assignment_mask&~LFE)
    {
        case L | R | C | Ls | Rs | Lb | Rb : ToReturn+=" 3/4/0"; break;
        case L | R | C | Ls | Rs | Lw | Rw : ToReturn+=" 5/2/0"; break;
        case L | R | C | Ls | Rs | Tl | Tr : ToReturn+=" 3/2/2"; break;
    }
    if (ToReturn.size()==9 && ToReturn[3]==' ' && (nonstd_bed_channel_assignment_mask&LFE))
        ToReturn+=".1";

    return ToReturn;
}

static const sized_array_string Ac4_presentation_config=
{
    (const char*)7,
    "Music and Effects + Dialogue",
    "Main + Dialogue Enhancement",
    "Main + Associate",
    "Music and Effects + Dialogue + Associate",
    "Main + Dialogue Enhancement + Associate",
    "Arbitrary Substream Groups",
    "EMDF Only",
};

static const size_t Ac4_presentation_config_split_Size = 6;
static const int8u Ac4_presentation_config_split[Ac4_presentation_config_split_Size][3] =
{
    {       ME,        D,  (int8u)-1},
    {       CM,        D,  (int8u)-1},
    {       CM,        A,  (int8u)-1},
    {       ME,        D,          A},
    {       CM,        D,          A},
};

const size_t Curve_Profiles_Count=6;
const int8u m1=(int8u)-1;
static File_Ac4::drc_decoder_config_curve Curve_Profiles_Values[Curve_Profiles_Count]=
{
    { 0,  0,  0,  0, m1, m1, m1, m1,  m1, m1, m1, m1, m1},
    { 0,  5,  6, 24, 19,  4,  9, 20,  75,  2, 50, 15, 20},
    {10, 10,  6, 24, 19,  4,  9, 20,  75,  2, 50, 15, 20},
    { 0,  5, 12, 24, 19,  4,  9, 20, 250,  2, 50, 15, 20},
    {10, 10, 12, 15, 29, m1, m1, 20,  75,  2, 50, 15, 20},
    { 0,  5, 15, 24, 19,  4,  9, 20,  25,  2, 10, 10, 10},
};

static const sized_array_string Ac4_drc_eac3_profile=
{
    (const char*)Curve_Profiles_Count,
    "",
    "Film standard",
    "Film light",
    "Music standard",
    "Music light",
    "Speech",
};

static const sized_array_string Ac4_loud_prac_type=
{
    (const char*)16,
    NULL,
    "ATSC A/85",
    "EBU R128",
    "ARIB TR-B32",
    "FreeTV OP-59",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "Manual",
    "Consumer leveler",
};

static const sized_array_string Ac4_loud_dialgate_prac_type=
{
    (const char*)4,
    NULL,
    "Automated C or L+R",
    "Automated individual front channels",
    "Manual",
};

static const sized_array_string Ac4_lra_prac_type=
{
    (const char*)2,
    "EBU Tech 3342 v1",
    "EBU Tech 3342 v2",
};

static const sized_array_string Ac4_drc_decoder_mode_id=
{
    (const char*)4,
    "HomeTheaterAvr",
    "FlatPanelTv",
    "PortableSpeakers",
    "PortableHeadphones",
};

static const sized_array_float Ac4_loro_centre_mixgain=
{
    8,
    3.0,
    1.5,
    0.0,
    -1.5,
    -3.0,
    -4.5,
    -6.0,
    -FLT_MAX,
};

static const sized_array_string Ac4_preferred_dmx_method=
{
    (const char*)4,
    "",
    "LoRo",
    "LtRt",
    "Pro Logic II",
};

static const sized_array_string Ac4_pre_dmixtyp_2ch=
{
    (const char*)4,
    "",
    "LoRo",
    "Pro Logic",
    "Pro Logic II",
};

static const sized_array_string Ac4_phase90_info_2ch=
{
    (const char*)4,
    "",
    NULL,
    "Applied",
    "Not applied",
};

static const sized_array_string Ac4_pre_dmixtyp_5ch=
{
    (const char*)4,
    "Pro Logic IIx",
    "Pro Logic IIx Movie",
    "Pro Logic IIx Music",
    "Pro Logic IIz",
};

static const sized_array_string Ac4_phase90_info_mc=
{
    (const char*)3,
    "",
    "Applied",
    "Not applied",
};

static const sized_array_string Ac4_de_channel_config=
{
    (const char*)8,
    "",
    "C",
    "R",
    "R C",
    "L",
    "L C",
    "L R",
    "L R C",
};

//---------------------------------------------------------------------------
static inline bool Channel_Mode_Contains_Lfe(int8u ch_mode)
{
    return (ch_mode==4 || ch_mode==6 || ch_mode==8 || ch_mode==10 || ch_mode==12 || ch_mode==14 || ch_mode==15)?true:false;
}

//---------------------------------------------------------------------------
static inline bool Channel_Mode_Contains_C(int8u ch_mode)
{
    return (ch_mode==0 || (ch_mode>=2 && ch_mode<=15))?true:false;
}

//---------------------------------------------------------------------------
static inline bool Channel_Mode_Contains_Lr(int8u ch_mode)
{
    return (ch_mode>=1 && ch_mode<=15)?true:false;
}

//---------------------------------------------------------------------------
static inline bool Channel_Mode_Contains_LsRs(int8u ch_mode)
{
    return (ch_mode>=3 && ch_mode<=15)?true:false;
}

//---------------------------------------------------------------------------
static inline bool Channel_Mode_Contains_LrsRrs(int8u ch_mode)
{
    return (ch_mode==5 || ch_mode==6 || (ch_mode>=11 && ch_mode<=15))?true:false;
}

//---------------------------------------------------------------------------
static inline bool Channel_Mode_Contains_LwRw(int8u ch_mode)
{
    return (ch_mode==7 || ch_mode==8 || ch_mode==15)?true:false;
}

//---------------------------------------------------------------------------
static inline bool Channel_Mode_Contains_VhlVhr(int8u ch_mode)
{
    return (ch_mode==9 || ch_mode==10)?true:false;
}

//---------------------------------------------------------------------------
static inline bool Channel_Mode_Contains_LscrRscr(int8u ch_mode)
{
    return (ch_mode == 13 || ch_mode == 14) ? true : false;
}

//---------------------------------------------------------------------------
static inline int8u objs_to_channel_mode(int8u n_objects_code)
{
    if (n_objects_code && n_objects_code<=4)
        return n_objects_code-1;
    return (int8u)-1; //Reserved/Unknown/Problem
}

//---------------------------------------------------------------------------
static inline int8u objs_to_n_objects(int8u n_objects_code, bool b_lfe)
{
    if (n_objects_code<=3)
        return n_objects_code+(b_lfe?1:0);
    if (n_objects_code==4)
        return 5+(b_lfe?1:0);
    return (int8u)-1; //Reserved/Unknown/Problem
}

//---------------------------------------------------------------------------
static const char* AC4_nonstd_bed_channel_assignment_mask_ChannelLayout_List[17+11] =
{
    "L",
    "R",
    "C",
    "LFE",
    "Ls",
    "Rs",
    "Lb",
    "Rb",
    "Tfl",
    "Tfr",
    "Tsl", //Tl
    "Tsr", //Tr
    "Tbl",
    "Tbr",
    "Lw",
    "Rw",
    "LFE2",
    // Not in nonstd_bed_channel_assignment
    "Cb",
    "Lscr",
    "Rscr",
    "Tsl",
    "Tsr",
    "Tc",
    "Bfl",
    "Bfr",
    "Bfc",
    "Tfc",
    "Tbc",
};
static int8s AC4_nonstd_bed_channel_assignment_mask_ChannelLayout_Reordering[17+11] =
{
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    6, // Wide channels before top layer
    6, // Wide channels before top layer
    -2,
    -2,
    -2,
    -2,
    -2,
    -2,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};
static Ztring AC4_nonstd_bed_channel_assignment_mask_ChannelLayout(int32u nonstd_bed_channel_assignment_mask, size_t Groups_Size=0)
{
    if (!nonstd_bed_channel_assignment_mask)
        return Groups_Size==1?__T("M"):__T("C");
    
    Ztring ToReturn;

    for (int8u i=0; i<17+11; i++)
    {
        int8u i2=i+AC4_nonstd_bed_channel_assignment_mask_ChannelLayout_Reordering[i];
        if (nonstd_bed_channel_assignment_mask&(1<<i2))
        {
            ToReturn+=Ztring().From_UTF8(AC4_nonstd_bed_channel_assignment_mask_ChannelLayout_List[i2]);
            ToReturn+=__T(' ');
        }
    }

    if (!ToReturn.empty())
        ToReturn.resize(ToReturn.size()-1);

    return ToReturn;
}

//---------------------------------------------------------------------------
static int8u AC4_nonstd_bed_channel_assignment_mask_2_num_channels_in_bed(int32u nonstd_bed_channel_assignment_mask)
{
    int8u ToReturn=0;

    for (int8u i=0; i<17; i++)
        if (nonstd_bed_channel_assignment_mask&(1<<i))
            ToReturn++;

    return ToReturn;
}

//---------------------------------------------------------------------------
static const int8u AC4_bed_channel_assignment_mask_ChannelLayout_Mapping[10] =
{
    2,
    1,
    1,
    2,
    2,
    2,
    2,
    2,
    2,
    1,
};
static int32u AC4_bed_channel_assignment_mask_2_nonstd(int16u bed_channel_assignment_mask)
{
    int32u ToReturn=0;

    int8u j=0;
    for (int8u i=0; i<10; i++)
    {
        if (bed_channel_assignment_mask&(1<<i))
        {
            ToReturn|=(1<<(j++));
            if (AC4_bed_channel_assignment_mask_ChannelLayout_Mapping[i]>1)
                ToReturn|=(1<<(j++));
        }
        else
            j+=AC4_bed_channel_assignment_mask_ChannelLayout_Mapping[i];
    }

    return ToReturn;
}
static int32u AC4_bed_chan_assign_code_2_nonstd_Values[8]=
{
    L | R,                                      //Stereo
    L | R | C,                                  //3.0
    L | R | C | Ls | Rs | LFE,                  //5.1
    L | R | C | Ls | Rs | Tl | Tr | LFE,                                        //5.1.2
    L | R | C | Ls | Rs | Tfl | Tfr | Tbl | Tbr | LFE,                          //5.1.4
    L | R | C | Ls | Rs | Lb | Rb | LFE,        //7.1 3/4/0
    L | R | C | Ls | Rs | Lb | Rb | Tl | Tr | LFE,                              //7.1.2
    L | R | C | Ls | Rs | Lb | Rb | Tfl | Tfr | Tbl | Tbr | LFE,                //7.1.4
};
static int32u AC4_bed_chan_assign_code_2_nonstd(int8u bed_chan_assign_code)
{
    return AC4_bed_chan_assign_code_2_nonstd_Values[bed_chan_assign_code];
}

//---------------------------------------------------------------------------
static int32u AC4_nonstd_2_desc_Values[17] =
{
    0,
    0,
    0,
    1,
    0,
    0,
    0,
    0,
    2,
    2,
    2,
    2,
    2,
    2,
    1,
    1,
    1,
};
static string AC4_nonstd_2_desc(int32u nonstd_bed_channel_assignment_mask)
{
    string ToReturn="0.0.0";
    for (int8u i=0; i<17; i++)
        if (nonstd_bed_channel_assignment_mask&(1<<i))
            ToReturn[2*AC4_nonstd_2_desc_Values[i]]++;
    return ToReturn;
}

//---------------------------------------------------------------------------
static const char* substream_type_Trace[] =
{
    "ac4_substream_data",
    "ac4_substream",
    "ac4_hsf_ext_substream",
    "emdf_payloads_substream",
    "ac4_presentation_substream",
    "oamd_substream",
};

//---------------------------------------------------------------------------
static const int8s de_hcb_abs_0[31][2] = {
   {  3,   1},
   {  5,   2},
   {-64,   9},
   {  7,   4},
   {-54, -63},
   {  6,  12},
   { 26, -62},
   {  8,  15},
   {-56, -61},
   { 11,  10},
   { 16, -60},
   { 28, -59},
   { 13,  14},
   { 30, -58},
   { 24, -57},
   { 18, -55},
   { 17, -46},
   {-49, -53},
   { 21,  19},
   { 20, -37},
   { 23, -52},
   { 22, -36},
   {-51, -34},
   {-33, -50},
   {-41,  25},
   {-48, -35},
   { 27,  29},
   {-47, -43},
   {-39, -45},
   {-44, -38},
   {-42, -40}
};

static const int8s de_hcb_abs_1[60][2] = {
    {-64,   1},
    {  2,  12},
    { 36,   3},
    {  4,  58},
    {-59,   5},
    { 53,   6},
    {-50,   7},
    {  8,  52},
    {-94,   9},
    { 10, -80},
    {-38,  11},
    {-93, -92},
    { 13,  27},
    { 14, -63},
    { 15, -54},
    {-51,  16},
    { 17,  22},
    {-49,  18},
    { 19, -74},
    {-79,  20},
    {-85,  21},
    {-91, -35},
    {-70,  23},
    { 48,  24},
    { 25, -34},
    { 26, -84},
    {-90, -89},
    { 28,  57},
    { 29,  59},
    {-66,  30},
    { 31, -52},
    { 49,  32},
    { 33, -73},
    { 34, -77},
    { 35, -40},
    {-36, -88},
    { 37,  55},
    { 38, -57},
    {-68,  39},
    { 40,  45},
    { 41,  51},
    { 44,  42},
    {-83,  43},
    {-37, -87},
    {-86, -39},
    {-72,  46},
    { 47, -45},
    {-41, -82},
    {-42, -81},
    { 50, -47},
    {-78, -43},
    {-44, -76},
    {-46, -75},
    {-69,  54},
    {-48, -71},
    { 56, -58},
    {-67, -53},
    {-65, -62},
    {-60, -61},
    {-55, -56}
};

static const int8s de_hcb_diff_0[62][2] = {
    {  1, -64},
    {  2,  32},
    {  3, -63},
    {  4,  22},
    {-61,   5},
    {  6, -68},
    {-57,   7},
    {  8, -71},
    { 54,   9},
    {-74,  10},
    { 11,  17},
    { 12,  14},
    { 13, -76},
    {-95, -94},
    { 15,  16},
    {-93, -92},
    {-90, -91},
    { 18,  20},
    {-42,  19},
    {-89, -88},
    { 21,  61},
    {-87, -35},
    {-67,  23},
    {-60,  24},
    {-58,  25},
    { 26, -55},
    { 27, -72},
    {-52,  28},
    {-48,  29},
    { 58,  30},
    { 31,  49},
    {-37, -86},
    { 33, -65},
    {-66,  34},
    { 35, -62},
    { 36,  59},
    {-69,  37},
    { 38, -56},
    { 39,  44},
    { 40, -53},
    {-49,  41},
    {-46,  42},
    { 43,  50},
    {-38, -85},
    {-73,  45},
    { 46, -50},
    { 47,  51},
    { 48, -44},
    {-40, -84},
    {-83, -82},
    {-39, -81},
    { 53,  52},
    {-80, -41},
    {-79, -78},
    {-51,  55},
    {-47,  56},
    { 57, -45},
    {-77, -33},
    {-75, -43},
    {-59,  60},
    {-54, -70},
    {-34, -36}
};

static const int8s de_hcb_diff_1[120][2] = {
    { -64,    1},
    {  94,    2},
    {  49,    3},
    {   4,  -63},
    {   5,  -62},
    { -68,    6},
    {  13,    7},
    {  19,    8},
    {  45,    9},
    {  10,   58},
    {  11,   62},
    {  12,   61},
    {-124, -102},
    { -56,   14},
    {  67,   15},
    { -78,   16},
    {  24,   17},
    {  18,  118},
    {-122, -123},
    {  20,   34},
    {  29,   21},
    {  26,   22},
    {  23,   28},
    {-121,  -14},
    {  25,  119},
    {-120,   -6},
    {  32,   27},
    {-116, -119},
    {-113, -118},
    { -82,   30},
    {  33,   31},
    {-117,   -5},
    {-114, -115},
    {-110, -112},
    {  40,   35},
    {  36,   38},
    {  37,  -88},
    {-111,  -12},
    { 114,   39},
    { -19, -109},
    {  41,   43},
    {  42,  117},
    {  -9, -108},
    {  44,  116},
    { -20, -107},
    {  46,   86},
    {  47,  111},
    { 115,   48},
    {-106, -105},
    {  50,   69},
    {  51,  -67},
    {  52,  -69},
    { -57,   53},
    {  54,   88},
    {  55,   93},
    {  56,   65},
    {  57,  -42},
    { -97, -104},
    { -48,   59},
    {  60,  -43},
    {-103,  -33},
    {-101,  -32},
    {  64,   63},
    {-100,  -98},
    { -99,  -31},
    {  66,  -85},
    { -96,  -95},
    {  68,  -50},
    { -47,  -94},
    {  70,  101},
    {  71,   77},
    {  72,  -71},
    { -74,   73},
    { -76,   74},
    { 105,   75},
    { -44,   76},
    { -93,  -92},
    {  78,  -58},
    { -55,   79},
    {  91,   80},
    {  81,   84},
    {  82,   83},
    { -39,  -91},
    { -40,  -90},
    { -83,   85},
    { -41,  -89},
    { 108,   87},
    { 109,  -87},
    {  89,  -52},
    { -80,   90},
    { 107,  -86},
    {  92,  -79},
    { -84,  -46},
    { -81,  -49},
    { -65,   95},
    { -66,   96},
    {  97,  -61},
    { -59,   98},
    { -72,   99},
    { 100,  -54},
    { -77,  -51},
    { -60,  102},
    { 103,  -70},
    { -73,  104},
    { -53,  -75},
    { 106,  -45},
    { -37,  -38},
    { -36,  -35},
    { -34,  110},
    { -30,  -29},
    { -28,   -4},
    { 113,  112},
    { -25,  -27},
    { -24,  -26},
    { -17,  -23},
    { -21,  -22},
    { -15,  -18},
    { -16,  -13},
    { -10,  -11},
    {  -7,   -8}
};

//---------------------------------------------------------------------------
static const char* out_ch_config_Values[] =
{
    "5.1",
    "5.1.2",
    "5.1.4",
    "7.1",
    "7.1.2",
};
static constexpr size_t out_ch_config_Size=sizeof(out_ch_config_Values)/sizeof(const char*);

//---------------------------------------------------------------------------
static const float32 gain_f1_Values(int8u code)
{
    if (code>=7)
        return -INFINITY;
    return 1.5*(-((int)code)+2);
};

//---------------------------------------------------------------------------
static const float32 gain_xx_Values(int8u code)
{
    if (code>=7)
        return -INFINITY;
    if (code>=4)
        return 3*(-((int)code)+2);
    return 1.5*(-((int)code));
};

//---------------------------------------------------------------------------
static const char* gain_Text[] =
{
    "ScreenToCenter",
    "ScreenToFront",
    "Back4ToBack2",
    "Top4ToTop2",
    "TopFrontToFront",
    "TopFrontToSide",
    "TopFrontToBack",
    "TopBackToFront",
    "TopBackToSide",
    "TopBackToBack",
};
static constexpr size_t gain_Size=sizeof(gain_Text)/sizeof(const char*);

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Ac4::File_Ac4()
:File__Analyze()
{
    //Configuration
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
    MustSynchronize=true;
    Buffer_TotalBytes_FirstSynched_Max=64*1024;
    Buffer_TotalBytes_Fill_Max=1024*1024;
    PTS_DTS_Needed=true;
    StreamSource=IsStream;
    Frame_Count_NotParsedIncluded=0;

    //In
    Frame_Count_Valid=0;
    MustParse_dac4=false;
}

//---------------------------------------------------------------------------
File_Ac4::~File_Ac4()
{
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ac4::Streams_Fill()
{
    //I-Frames
    Ztring IFrames_Value;
    if (IFrames.size()>1)
    {
        set<size_t> IFrames_IsVariable;
        for (size_t i=1; i<IFrames.size(); i++)
            IFrames_IsVariable.insert(IFrames[i]-IFrames[i-1]);
        if (IFrames_IsVariable.size()==1)
            IFrames_Value=Ztring::ToZtring(*IFrames_IsVariable.begin());
        else if (IFrames_IsVariable.size()==2)
        {
            set<size_t>::iterator It=IFrames_IsVariable.begin();
            size_t Value1=*It;
            It++;
            size_t Value2=*It;
            if (Value1+1==Value2)
            {
                if ((size_t)Ac4_frame_rate[fs_index][frame_rate_index]==Value1) // if Frame rate is in I-frame interval, use frame rate))
                    IFrames_Value=Ztring::ToZtring(Ac4_frame_rate[fs_index][frame_rate_index], 3);
                else
                    IFrames_Value=Ztring::ToZtring(Value1)+__T('/')+Ztring::ToZtring(Value2);
            }
        }
    }

    //Filling
    Fill(Stream_General, 0, General_Format, "AC-4");
    Stream_Prepare(Stream_Audio);
    Fill(Stream_Audio, 0, Audio_Format, "AC-4");
    Fill(Stream_Audio, 0, Audio_Format_Commercial_IfAny, "Dolby AC-4");
    Fill(Stream_Audio, 0, Audio_Format_Version, __T("Version ")+Ztring::ToZtring(bitstream_version));
    Fill(Stream_Audio, 0, "bitstream_version", bitstream_version, 10, true);//TODO remove
    Fill_SetOptions(Stream_Audio, 0, "bitstream_version", "N NTN"); //TODO remove
    Fill(Stream_Audio, 0, Audio_SamplingRate, fs_index?48000:44100);
    Fill(Stream_Audio, 0, Audio_FrameRate, Ac4_frame_rate[fs_index][frame_rate_index]);
    if (!IFrames_Value.empty())
        Fill_Measure(Stream_Audio, 0, "IFrameInterval", IFrames_Value, __T(" frames"));

    //If no frame, use dac4 content
    bool IsUsingDac4;
    if (Presentations.empty() && Groups.empty() && AudioSubstreams.empty())
    {
        Presentations=Presentations_dac4;
        Groups=Groups_dac4;
        IsUsingDac4=true;
    }
    else
        IsUsingDac4=false;

    //Filling
    if (!Presentations.empty())
        Fill(Stream_Audio, 0, "NumberOfPresentations", Presentations.size());
    if (!Groups.empty() && bitstream_version>=2)
        Fill(Stream_Audio, 0, "NumberOfGroups", Groups.size());
    if (!AudioSubstreams.empty())
        Fill(Stream_Audio, 0, "NumberOfSubstreams", AudioSubstreams.size());

    for (size_t p=0; p<Presentations.size(); p++)
    {
        const presentation& Presentation_Current=Presentations[p];
        string ChannelMode;

        //Summary pres_ch_mode
        if (Presentation_Current.pres_ch_mode==1 && Presentation_Current.pres_immersive_stereo!=(int8u)-1)
        {
            ChannelMode="Immersive Stereo";
        }
        else if (Presentation_Current.pres_ch_mode!=(int8u)-1)
        {
            ChannelMode=Value(Ac4_ch_mode_String, Presentation_Current.pres_ch_mode);
            if (Presentation_Current.pres_ch_mode>=11 && Presentation_Current.pres_ch_mode<=14)
            {
                if (!Presentation_Current.b_pres_4_back_channels_present)
                    ChannelMode[0]-=2;
                if (!Presentation_Current.b_pres_centre_present)
                    ChannelMode[0]-=1;
                if (Presentation_Current.pres_top_channel_pairs!=2)
                    ChannelMode[4]-=2*(2-Presentation_Current.pres_top_channel_pairs);
                if (Channel_Mode_Contains_LscrRscr(Presentation_Current.pres_ch_mode))
                    ChannelMode += " (scr)";
            }
        }
        else
        {
            for (size_t i=0; i<Presentation_Current.substream_group_info_specifiers.size(); i++)
                if (!Groups[Presentation_Current.substream_group_info_specifiers[i]].b_channel_coded)
                {
                    ChannelMode="Object Audio";
                    break;
                }
        }
        string Summary=ChannelMode;
        if (!Summary.empty())
            Summary+=' ';

        //Summary presentation_config
        string presentation_config_String;
        bool IsDolbyAtmos=Presentation_Current.dolby_atmos_indicator;
        for (size_t g=0; g<Presentation_Current.substream_group_info_specifiers.size(); g++)
        {
            const group& Group=Groups[Presentation_Current.substream_group_info_specifiers[g]];
            if (g)
                presentation_config_String+=" + ";
            if (Group.ContentInfo.content_classifier!=(int8u)-1)
            {
                int8u content_classifier=Group.ContentInfo.content_classifier;
                if (Presentation_Current.presentation_config!=(int8u)-1 && g<3 && Presentation_Current.presentation_config<Ac4_presentation_config_split_Size && Ac4_presentation_config_split[Presentation_Current.presentation_config][g]==A
                  && (content_classifier==VI ||content_classifier==HI || content_classifier==Co))
                    content_classifier=A;
                presentation_config_String+=Value(Ac4_content_classifier, content_classifier);
                if (Group.ContentInfo.content_classifier==D && (Presentation_Current.presentation_config==1 || Presentation_Current.presentation_config==4) && g==1)
                    presentation_config_String+=" Enhancement";
            }
            else if (Presentation_Current.presentation_config!=(int8u)-1 && g<3 && Presentation_Current.presentation_config<Ac4_presentation_config_split_Size && Ac4_presentation_config_split[Presentation_Current.presentation_config][g]!=(int8u)-1)
            {
                presentation_config_String+=Value(Ac4_content_classifier, Ac4_presentation_config_split[Presentation_Current.presentation_config][g]);
                if (Ac4_presentation_config_split[Presentation_Current.presentation_config][g]==D && (Presentation_Current.presentation_config==1 || Presentation_Current.presentation_config==4) && g==1)
                    presentation_config_String+=" Enhancement";
            }
            else if (Presentation_Current.presentation_config==(int8u)-1)
                presentation_config_String+="Main";
            else
                presentation_config_String+=Ztring::ToZtring(Presentation_Current.presentation_config).To_UTF8();

            if (!IsDolbyAtmos && Presentation_Current.presentation_version>=2)
            {
                for (size_t s=0; s<Group.Substreams.size(); s++)
                {
                    const group_substream& Substream=Group.Substreams[s];
                    if (Substream.immersive_stereo==1)
                    {
                        IsDolbyAtmos=true;
                        break;
                    }
                }
            }
        }
        Summary+=Presentation_Current.presentation_config==(int8u)-1?string("Main"):Value(Ac4_presentation_config, Presentation_Current.presentation_config);

        //Summary language
        if (!Presentation_Current.Language.empty())
        {
            Summary+=" (";
            Summary+=MediaInfoLib::Config.Iso639_Translate(Ztring().From_UTF8(Presentation_Current.Language)).To_UTF8();
            Summary+=')';
        }
        if (Summary.empty())
            Summary='?';

        string P=Ztring(__T("Presentation")+Ztring::ToZtring(p)).To_UTF8();
        Fill(Stream_Audio, 0, P.c_str(), Summary);
        Fill(Stream_Audio, 0, (P+" Pos").c_str(), p);
        Fill_SetOptions(Stream_Audio, 0, (P+" Pos").c_str(), "N NIY");
        if (Presentation_Current.substream_index!=(int8u)-1)
            Fill(Stream_Audio, 0, (P+" Index").c_str(), Presentation_Current.substream_index);
        Fill_SetOptions(Stream_Audio, 0, (P+" Index").c_str(), "N NIY");
        if (Presentation_Current.presentation_id!=(int32u)-1)
            Fill(Stream_Audio, 0, (P+" PresentationID").c_str(), Presentation_Current.presentation_id);
        if (!ChannelMode.empty())
        {
            Fill(Stream_Audio, 0, (P+" ChannelMode").c_str(), ChannelMode);
            Fill_SetOptions(Stream_Audio, 0, (P+" ChannelMode").c_str(), "N NTY");
        }
        if (Presentation_Current.presentation_config!=(int8u)-1)
        {
            Fill(Stream_Audio, 0, (P+" PresentationConfig").c_str(), Value(Ac4_presentation_config, Presentation_Current.presentation_config));
            Fill_SetOptions(Stream_Audio, 0, (P+" PresentationConfig").c_str(), "N NTY");
        }
        if (!presentation_config_String.empty() && presentation_config_String!=(Presentation_Current.presentation_config==(int8u)-1?string("Main"):Value(Ac4_presentation_config, Presentation_Current.presentation_config)))
        {
            Fill(Stream_Audio, 0, (P+" PresentationConfig_ContentClassifier").c_str(), presentation_config_String); //TODO
            Fill_SetOptions(Stream_Audio, 0, (P+" PresentationConfig_ContentClassifier").c_str(), "N NTY");
        }
        if (IsDolbyAtmos)
            Fill(Stream_Audio, 0, (P+" DolbyAtmos").c_str(), "Yes");
        if (Presentation_Current.LoudnessInfo.dialnorm_bits!=(int8u)-1)
            Fill(Stream_Audio, 0, (P+" DialogueNormalization").c_str(), -0.25*Presentation_Current.LoudnessInfo.dialnorm_bits, 2);
        if (!Presentation_Current.Language.empty())
        {
            Fill(Stream_Audio, 0, (P+" Language").c_str(), Presentation_Current.Language);
            Fill(Stream_Audio, 0, (P+" Language/String").c_str(), MediaInfoLib::Config.Iso639_Translate(Ztring().From_UTF8(Presentation_Current.Language)));
            Fill_SetOptions(Stream_Audio, 0, (P+" Language").c_str(), "N NTY");
            Fill_SetOptions(Stream_Audio, 0, (P+" Language/String").c_str(), "Y NTN");
        }
        if (Presentation_Current.b_multi_pid_PresentAndValue!=(int8u)-1)
            Fill(Stream_Audio, 0, (P+" MultipleStream").c_str(), Presentation_Current.b_multi_pid_PresentAndValue?"Yes":"No");
        {
            const loudness_info& L=Presentation_Current.LoudnessInfo;
            if (L.loudspchgat!=(int16u)-1 || L.loudrelgat!=(int16u)-1 || (L.loud_prac_type!=(int8u)-1 && L.loud_prac_type) || L.max_truepk!=(int16u)-1|| L.lra!=(int16u)-1)
            {
                Fill(Stream_Audio, 0, (P+" Loudness").c_str(), "Yes");
                if (L.loudspchgat!=(int16u)-1)
                {
                    string IntegratedLoudness_Speech=Ztring::ToZtring((L.loudspchgat-1024)/10.0, 1).To_UTF8();
                    Fill(Stream_Audio, 0, (P+" Loudness IntegratedLoudness_Speech").c_str(), IntegratedLoudness_Speech);
                    Fill_SetOptions(Stream_Audio, 0, (P+" Loudness IntegratedLoudness_Speech").c_str(), "N NFY");
                    IntegratedLoudness_Speech+=" LKFS";
                    if (L.loudspchgat_dialgate_prac_type)
                    {
                        string Type=Value(Ac4_loud_dialgate_prac_type, L.loudspchgat_dialgate_prac_type);
                        Fill(Stream_Audio, 0, (P+" Loudness IntegratedLoudness_Speech_Type").c_str(), Type);
                        Fill_SetOptions(Stream_Audio, 0, (P+" Loudness IntegratedLoudness_Speech_Type").c_str(), "N NTY");
                        IntegratedLoudness_Speech+=" ("+Type+')';
                    }
                    Fill(Stream_Audio, 0, (P+" Loudness IntegratedLoudness_Speech/String").c_str(), IntegratedLoudness_Speech);
                    Fill_SetOptions(Stream_Audio, 0, (P+" Loudness IntegratedLoudness_Speech/String").c_str(), "Y NTN");
                }
                if (L.loudrelgat!=(int16u)-1)
                    Fill_Measure(Stream_Audio, 0, (P+" Loudness IntegratedLoudness_Level").c_str(), Ztring::ToZtring((L.loudrelgat-1024)/10.0, 1), __T(" LKFS"));
                if (L.loud_prac_type!=(int8u)-1 && L.loud_prac_type)
                {
                    Fill(Stream_Audio, 0, (P+" Loudness AudioLoudnessStandard").c_str(), Value(Ac4_loud_prac_type, L.loud_prac_type));
                    Fill(Stream_Audio, 0, (P+" Loudness RealtimeLoudnessCorrected").c_str(), L.b_loudcorr_type?"Yes":"No");
                    string DialogueCorrected=L.loud_dialgate_prac_type!=(int8u)-1?"Yes":"No";
                    Fill(Stream_Audio, 0, (P+" Loudness DialogueCorrected").c_str(), DialogueCorrected);
                    Fill_SetOptions(Stream_Audio, 0, (P+" Loudness DialogueCorrected").c_str(), "N NTY");
                    if (L.loud_dialgate_prac_type!=(int8u)-1 && L.loud_dialgate_prac_type)
                    {
                        string Type=Value(Ac4_loud_dialgate_prac_type, L.loud_dialgate_prac_type);
                        Fill(Stream_Audio, 0, (P+" Loudness DialogueCorrected_Type").c_str(), Type);
                        Fill_SetOptions(Stream_Audio, 0, (P+" Loudness DialogueCorrected_Type").c_str(), "N NTY");
                        DialogueCorrected+=" ("+ Type +')';
                    }
                    Fill(Stream_Audio, 0, (P+" Loudness DialogueCorrected/String").c_str(), DialogueCorrected);
                    Fill_SetOptions(Stream_Audio, 0, (P+" Loudness DialogueCorrected/String").c_str(), "Y NTN");
                }
                if (L.max_truepk!=(int16u)-1)
                    Fill_Measure(Stream_Audio, 0, (P+" Loudness MaxTruePeak").c_str(), Ztring::ToZtring((L.max_truepk-1024)/10.0, 1), __T(" dBTP"));
                if (L.max_loudmntry!=(int16u)-1)
                    Fill_Measure(Stream_Audio, 0, (P+" Loudness MaximumMomentaryLoudness").c_str(), Ztring::ToZtring((L.max_loudmntry-1024)/10.0, 1), __T(" LUFS"));
                if (L.lra!=(int16u)-1)
                {
                    string Loudness_Range=Ztring::ToZtring(L.lra/10.0, 1).To_UTF8();
                    Fill(Stream_Audio, 0, (P+" Loudness Loudness_Range").c_str(), Loudness_Range);
                    Fill_SetOptions(Stream_Audio, 0, (P+" Loudness Loudness_Range").c_str(), "N NFY");
                    Loudness_Range+=" LU";
                    if (L.loudspchgat_dialgate_prac_type)
                    {
                        string Type=Value(Ac4_lra_prac_type, L.lra_prac_type);
                        Fill(Stream_Audio, 0, (P+" Loudness Loudness_Range_Type").c_str(), Type);
                        Fill_SetOptions(Stream_Audio, 0, (P+" Loudness Loudness_Range_Type").c_str(), "N NTY");
                        Loudness_Range+=" ("+Type+')';
                    }
                    Fill(Stream_Audio, 0, (P+" Loudness Loudness_Range/String").c_str(), Loudness_Range);
                    Fill_SetOptions(Stream_Audio, 0, (P+" Loudness Loudness_Range/String").c_str(), "Y NTN");
                }
            }
        }
        {
            const drc_info& D=Presentation_Current.DrcInfo;
            if (D.drc_eac3_profile!=(int8u)-1 || !D.Decoders.empty())
            {
                Fill(Stream_Audio, 0, (P + " DynamicRangeControl").c_str(), "Yes");
                if (D.drc_eac3_profile!=(int8u)-1)
                    Fill(Stream_Audio, 0, (P+" DynamicRangeControl Eac3DrcProfile").c_str(), Value(Ac4_drc_eac3_profile, D.drc_eac3_profile));
                for (size_t i=0; i<D.Decoders.size(); i++)
                {
                    drc_decoder_config C=D.Decoders[i];
                    string V;
                    if (C.drc_default_profile_flag)
                        V=Value(Ac4_drc_eac3_profile, D.drc_eac3_profile);
                    else if (C.drc_compression_curve_flag)
                    {
                        for (size_t i=0; i<Curve_Profiles_Count; i++)
                            if (C.drc_compression_curve==Curve_Profiles_Values[i])
                                V=Value(Ac4_drc_eac3_profile, i);
                        if (V.empty())
                            V="Custom";
                    }
                    else
                        V="DRC gains";
                    Fill(Stream_Audio, 0, (P+" DynamicRangeControl "+Value(Ac4_drc_decoder_mode_id, C.drc_decoder_mode_id)).c_str(), V);
                }
            }
        }
        {
            const dmx& D=Presentation_Current.Dmx;
            if (D.loro_centre_mixgain!=(int8u)-1 || D.ltrt_centre_mixgain!=(int8u)-1 || D.lfe_mixgain!=(int8u)-1 || D.preferred_dmx_method!=(int8u)-1)
            {
                Fill(Stream_Audio, 0, (P+" Downmix").c_str(), "Yes");
                if (D.loro_centre_mixgain!=(int8u)-1)
                {
                    Fill_Measure(Stream_Audio, 0, (P+" Downmix LoRoCenterMixGain").c_str(), Value(Ac4_loro_centre_mixgain, D.loro_centre_mixgain, 1), " dB");
                    Fill_Measure(Stream_Audio, 0, (P+" Downmix LoRoSurroundMixGain").c_str(), Value(Ac4_loro_centre_mixgain, D.loro_surround_mixgain, 1), " dB");
                }
                if (D.ltrt_centre_mixgain!=(int8u)-1)
                {
                    Fill_Measure(Stream_Audio, 0, (P+" Downmix LtRtCenterMixGain").c_str(), Value(Ac4_loro_centre_mixgain, D.ltrt_centre_mixgain, 1), " dB");
                    Fill_Measure(Stream_Audio, 0, (P+" Downmix LtRtSurroundMixGain").c_str(), Value(Ac4_loro_centre_mixgain, D.ltrt_surround_mixgain, 1), " dB");
                }
                if (D.lfe_mixgain!=(int8u)-1)
                    Fill_Measure(Stream_Audio, 0, (P+" Downmix LfeMixGain").c_str(), Ztring::ToZtring(10-D.lfe_mixgain).To_UTF8(), " dB");
                if (D.preferred_dmx_method!=(int8u)-1)
                    Fill(Stream_Audio, 0, (P+" Downmix PreferredDownmix").c_str(), Value(Ac4_preferred_dmx_method, D.preferred_dmx_method));
            }
            if (!D.Cdmxs.empty())
            {
                Fill(Stream_Audio, 0, (P+" Downmix CustomDownmixTargets").c_str(), "Yes");
                for (size_t c=0; c<D.Cdmxs.size(); c++)
                {
                    const auto& C=D.Cdmxs[c];
                    const auto Pdc=P+" Downmix CustomDownmixTargets "+string(out_ch_config_Values[C.out_ch_config])+"ch";
                    Fill(Stream_Audio, 0, Pdc.c_str(), "Yes");
                    for (size_t g=0; g<C.Gains.size(); g++)
                    {
                        const auto& G=C.Gains[g];
                        Fill_Measure(Stream_Audio, 0, (Pdc+' '+gain_Text[(size_t)G.Type]).c_str(), gain_xx_Values(G.Value), " dB", 1);
                    }
                }
            }
        }

        ZtringList GroupPos, GroupNum;
        for (size_t s=0; s<Presentation_Current.substream_group_info_specifiers.size(); s++)
        {
            GroupPos.push_back(Ztring::ToZtring(Presentation_Current.substream_group_info_specifiers[s]));
            GroupNum.push_back(Ztring::ToZtring(Presentation_Current.substream_group_info_specifiers[s]+1));
        }
        GroupPos.Separator_Set(0, __T(" + "));
        Fill(Stream_Audio, 0, (P + (bitstream_version<2?" LinkedTo_Substream_Pos":" LinkedTo_Group_Pos")).c_str(), GroupPos.Read());
        Fill_SetOptions(Stream_Audio, 0, (P+(bitstream_version<2?" LinkedTo_Substream_Pos":" LinkedTo_Group_Pos")).c_str(), "N NIY");
        GroupNum.Separator_Set(0, __T(" + "));
        Fill(Stream_Audio, 0, (P+(bitstream_version<2?" LinkedTo_Substream_Pos/String":" LinkedTo_Group_Pos/String")).c_str(), GroupNum.Read());
        Fill_SetOptions(Stream_Audio, 0, (P+(bitstream_version<2?" LinkedTo_Substream_Pos/String":" LinkedTo_Group_Pos/String")).c_str(), "Y NIN");
    }
    for (size_t g=0; g<Groups.size(); g++)
    {
        if (bitstream_version<2)
            continue; //Groups are fake

        string G=Ztring(__T("Group")+Ztring::ToZtring(g)).To_UTF8();
        const group& Group=Groups[g];
        set<string> PresentationConfigs;
        if (PresentationConfigs.empty())
        {
            //Check content_classifer from presentations
            for (size_t p=0; p<Presentations.size(); p++)
            {
                const presentation& Presentation_Current=Presentations[p];
                for (size_t s=0; s<Presentation_Current.substream_group_info_specifiers.size(); s++)
                {
                    const size_t& Specifier=Presentation_Current.substream_group_info_specifiers[s];
                    if (Specifier==g)
                    {
                        string Summary2;
                        if (Presentation_Current.presentation_config==(int8u)-1)
                            Summary2="Main";
                        else if (s<3 && Presentation_Current.presentation_config<Ac4_presentation_config_split_Size && Ac4_presentation_config_split[Presentation_Current.presentation_config][s]!=(int8u)-1)
                        {
                            Summary2=Ac4_content_classifier[1+Ac4_presentation_config_split[Presentation_Current.presentation_config][s]];
                            if (Ac4_presentation_config_split[Presentation_Current.presentation_config][s]==D && (Presentation_Current.presentation_config==1 || Presentation_Current.presentation_config==4) && s==1)
                                Summary2+=" Enhancement";
                        }
                        else
                            Summary2=Ztring::ToZtring(Presentation_Current.presentation_config).To_UTF8();
                        PresentationConfigs.insert(Summary2);
                    }
                }
            }
            if (PresentationConfigs.empty() && Group.ContentInfo.content_classifier!=(int8u)-1)
                PresentationConfigs.insert(Value(Ac4_content_classifier, Group.ContentInfo.content_classifier));

            if (PresentationConfigs.empty())
                PresentationConfigs.insert("?");
        }
        string Summary;
        for (set<string>::iterator PresentationConfig=PresentationConfigs.begin(); PresentationConfig!=PresentationConfigs.end(); ++PresentationConfig)
        {
            if (!Summary.empty())
                Summary+=" / ";
            Summary+=*PresentationConfig;
        }
        Fill(Stream_Audio, 0, G.c_str(), Summary);
        Fill(Stream_Audio, 0, (G+" Pos").c_str(), g);
        Fill_SetOptions(Stream_Audio, 0, (G+" Pos").c_str(), "N NIY");
        if (Group.ContentInfo.content_classifier!=(int8u)-1)
            Fill(Stream_Audio, 0, (G+" Classifier").c_str(), Value(Ac4_content_classifier, Group.ContentInfo.content_classifier));
        if (!Group.ContentInfo.language_tag_bytes.empty())
        {
            Fill(Stream_Audio, 0, (G+" Language").c_str(), Group.ContentInfo.language_tag_bytes);
            Fill(Stream_Audio, 0, (G+" Language/String").c_str(), MediaInfoLib::Config.Iso639_Translate(Ztring().From_UTF8(Group.ContentInfo.language_tag_bytes)));
            Fill_SetOptions(Stream_Audio, 0, (G+" Language").c_str(), "N NTY");
            Fill_SetOptions(Stream_Audio, 0, (G+" Language/String").c_str(), "Y NTN");
        }
        Fill(Stream_Audio, 0, (G+" ChannelCoded").c_str(), Group.b_channel_coded?"Yes":"No");
        for (size_t s=0; s<Group.Substreams.size(); s++)
        {
            const group_substream& Substream=Group.Substreams[s];
            if (Substream.immersive_stereo!=(int8u)-1)
            {
                Fill(Stream_Audio, 0, (G+" ImmersiveStereo").c_str(), Value(Ac4_immersive_stereo_String, Substream.immersive_stereo));
                break;
            }
        }
        Fill(Stream_Audio, 0, (G+" NumberOfSubstreams").c_str(), Group.Substreams.size());
        int8u n_objects=0;
        int8u num_channels_in_bed_Total=0;
        int32u nonstd_bed_channel_assignment_mask=0;
        for (size_t s=0; s<Group.Substreams.size(); s++)
        {
            const group_substream& Substream=Group.Substreams[s];
            if (!Group.b_channel_coded && !Substream.b_ajoc && Substream.n_objects_code!=(int8u)-1)
            {
                n_objects+=objs_to_n_objects(Substream.n_objects_code, Substream.b_lfe);
                if (Substream.nonstd_bed_channel_assignment_mask!=(int32u)-1)
                {
                    nonstd_bed_channel_assignment_mask|=Substream.nonstd_bed_channel_assignment_mask;
                    num_channels_in_bed_Total+=AC4_nonstd_bed_channel_assignment_mask_2_num_channels_in_bed(Substream.nonstd_bed_channel_assignment_mask);
                }
            }
        }
        if (nonstd_bed_channel_assignment_mask)
        {
            Ztring BedChannelConfiguration=AC4_nonstd_bed_channel_assignment_mask_ChannelLayout(nonstd_bed_channel_assignment_mask);
            int8u num_channels_in_bed=AC4_nonstd_bed_channel_assignment_mask_2_num_channels_in_bed(nonstd_bed_channel_assignment_mask);
            Ztring Count=Ztring::ToZtring(num_channels_in_bed);
            Fill(Stream_Audio, 0, (G+" BedChannelCount").c_str(), num_channels_in_bed);
            Fill_SetOptions(Stream_Audio, 0, (G+" BedChannelCount").c_str(), "N NIY");
            Fill(Stream_Audio, 0, (G+" BedChannelCount/String").c_str(), MediaInfoLib::Config.Language_Get(Count, __T(" channel")));
            Fill_SetOptions(Stream_Audio, 0, (G+" BedChannelCount/String").c_str(), "Y NIN");
            Fill(Stream_Audio, 0, (G+" BedChannelConfiguration").c_str(), BedChannelConfiguration);
            Fill(Stream_Audio, 0, (G+" ChannelMode").c_str(), Ac4_nonstd_2_ch_mode_String(nonstd_bed_channel_assignment_mask));
        }
        if (n_objects)
        {
            Fill(Stream_Audio, 0, (G+" NumberOfObjects").c_str(), n_objects);
            if (n_objects>=num_channels_in_bed_Total)
                Fill(Stream_Audio, 0, (G+" NumberOfDynamicObjects").c_str(), n_objects-num_channels_in_bed_Total);
        }

        ZtringList SubstreamPos, SubstreamNum;
        for (size_t s=0; s<Group.Substreams.size(); s++)
        {
            const group_substream& GroupInfo=Group.Substreams[s];
            if (GroupInfo.substream_index==(int8u)-1)
                continue;
            size_t AudioSubstream_Pos=0;
            std::map<int8u, substream_type_t>::iterator Substream_Type_Item=Substream_Type.begin();
            for (; Substream_Type_Item!=Substream_Type.end() && Substream_Type_Item->first!=GroupInfo.substream_index; Substream_Type_Item++)
                if (Substream_Type_Item->second==Type_Ac4_Substream)
                    AudioSubstream_Pos++;
            if (Substream_Type_Item==Substream_Type.end())
                continue;
            SubstreamPos.push_back(Ztring::ToZtring(AudioSubstream_Pos));
            SubstreamNum.push_back(Ztring::ToZtring(AudioSubstream_Pos+1));
        }
        SubstreamPos.Separator_Set(0, __T(" + "));
        Fill(Stream_Audio, 0, (G+" LinkedTo_Substream_Pos").c_str(), SubstreamPos.Read());
        Fill_SetOptions(Stream_Audio, 0, (G+" LinkedTo_Substream_Pos").c_str(), "N NIY");
        SubstreamNum.Separator_Set(0, __T(" + "));
        Fill(Stream_Audio, 0, (G+" LinkedTo_Substream_Pos/String").c_str(), SubstreamNum.Read());
        Fill_SetOptions(Stream_Audio, 0, (G+" LinkedTo_Substream_Pos/String").c_str(), "Y NIN");
    }
    for (map<int8u, audio_substream>::iterator Substream_Info=AudioSubstreams.begin(); Substream_Info!=AudioSubstreams.end(); Substream_Info++)
    {
        string ChannelMode, ImmersiveStereo;
        for (size_t g=0; g<Groups.size(); g++)
        {
            const group& Group=Groups[g];
            for (size_t s=0; s<Group.Substreams.size(); s++)
            {
                const group_substream& GroupInfo=Group.Substreams[s];
                string ChannelMode2, ImmersiveStereo2;
                if (GroupInfo.substream_index==Substream_Info->first)
                {
                    if (Groups[g].b_channel_coded)
                    {
                        ChannelMode2=Value(Ac4_ch_mode_String, GroupInfo.ch_mode);
                        if (GroupInfo.immersive_stereo!=(int8u)-1)
                            ImmersiveStereo2=Value(Ac4_immersive_stereo_String, GroupInfo.immersive_stereo);
                        if (GroupInfo.ch_mode>=11 && GroupInfo.ch_mode<=14)
                        {
                            if (!GroupInfo.b_4_back_channels_present)
                                ChannelMode2[0]-=2;
                            if (!GroupInfo.b_centre_present)
                                ChannelMode2[0]-=1;
                            if (GroupInfo.top_channel_pairs!=2)
                                ChannelMode2[4]-=2*(2-GroupInfo.top_channel_pairs);
                            if (Channel_Mode_Contains_LscrRscr(GroupInfo.ch_mode))
                                ChannelMode2 += " (scr)";
                        }
                    }
                    else if (GroupInfo.b_ajoc)
                    {
                        ChannelMode2="A-JOC ";
                        ChannelMode2+=Ztring::ToZtring(GroupInfo.n_fullband_upmix_signals).To_UTF8();
                        ChannelMode2+='.';
                        ChannelMode2+='0'+GroupInfo.b_lfe;
                        ChannelMode2+=" (";
                        ChannelMode2+=Ztring::ToZtring(GroupInfo.n_fullband_dmx_signals).To_UTF8();
                        ChannelMode2+='.';
                        ChannelMode2+='0'+GroupInfo.b_lfe;
                        ChannelMode2+=' ';
                        ChannelMode2+=GroupInfo.b_static_dmx?"channel":"object";
                        ChannelMode2+=" core)";
                    }
                    else if (!GroupInfo.b_ajoc && GroupInfo.n_objects_code!=(int8u)-1)
                    {
                        int8u n=objs_to_n_objects(GroupInfo.n_objects_code, GroupInfo.b_lfe);
                        if (n!=(int8u)-1)
                        {
                            ChannelMode2=Ztring::ToZtring(n-(GroupInfo.b_lfe?1:0)).To_UTF8();
                            if (GroupInfo.b_lfe)
                                ChannelMode2+=".1";
                            ChannelMode2+=" objects";
                        }
                        else
                            ChannelMode2="n_objects_code="+Ztring::ToZtring(GroupInfo.n_objects_code).To_UTF8()+" b_lfe="+(GroupInfo.b_lfe?'1':'0');
                    }
                    if (!ChannelMode2.empty() && ChannelMode2!=ChannelMode)
                    {
                        if (!ChannelMode.empty())
                            ChannelMode+=" + ";
                        ChannelMode+=ChannelMode2;
                    }
                    if (!ImmersiveStereo2.empty() && ImmersiveStereo2!=ImmersiveStereo)
                    {
                        if (!ImmersiveStereo.empty())
                            ImmersiveStereo+=" + ";
                        ImmersiveStereo+=ImmersiveStereo2;
                    }
                }
            }
        }

        string Summary=ChannelMode;
        if (Summary.empty())
        {
            Summary="?";
        }
        size_t AudioSubstream_Pos=0;
        for (std::map<int8u, substream_type_t>::iterator Substream_Type_Item=Substream_Type.begin(); Substream_Type_Item!=Substream_Type.end() && Substream_Type_Item->first!=Substream_Info->first; Substream_Type_Item++)
            if (Substream_Type_Item->second==Type_Ac4_Substream)
                AudioSubstream_Pos++;
        string S=Ztring(__T("Substream")+Ztring::ToZtring(AudioSubstream_Pos)).To_UTF8();
        Fill(Stream_Audio, 0, S.c_str(), Summary);
        Fill(Stream_Audio, 0, (S+" Pos").c_str(), AudioSubstream_Pos);
        Fill_SetOptions(Stream_Audio, 0, (S+" Pos").c_str(), "N NIY");
        Fill(Stream_Audio, 0, (S+" Index").c_str(), Substream_Info->first);
        Fill_SetOptions(Stream_Audio, 0, (S+" Index").c_str(), "N NIY");
        if (!ChannelMode.empty())
        {
            Fill(Stream_Audio, 0, (S+" ChannelMode").c_str(), ChannelMode);
        }
        if (!ImmersiveStereo.empty())
        {
            Fill(Stream_Audio, 0, (S+" ImmersiveStereo").c_str(), ImmersiveStereo);
        }

        //Info from group
        bool b_channel_coded=false;
        bool b_de_data_present=false;
        int8u de_max_gain, de_channel_config;
        for (size_t i=0; i <Groups.size(); i++)
        {
            for (size_t j=0; j<Groups[i].Substreams.size(); j++)
                if (Groups[i].Substreams[j].substream_index==Substream_Info->first)
                {
                    const group_substream& GroupInfo=Groups[i].Substreams[j];
                    if (Groups[i].b_channel_coded)
                    {
                        //Channel based
                        b_channel_coded=true;
                        const de_info& D=Substream_Info->second.DeInfo;
                        //b_de_data_present=D.b_de_data_present;
                        //if (b_de_data_present)
                        //{
                        //    de_max_gain=D.Config.de_max_gain;
                        //    de_channel_config=D.Config.de_channel_config;
                        //}
                        Fill_Dup(Stream_Audio, 0, (S + " ChannelLayout").c_str(), AC4_nonstd_bed_channel_assignment_mask_ChannelLayout(Ac4_ch_mode_2_nonstd(GroupInfo.ch_mode, GroupInfo.b_4_back_channels_present, GroupInfo.b_centre_present, GroupInfo.top_channels_present), Groups.size()));
                    }
                    else if (GroupInfo.b_ajoc || GroupInfo.n_objects_code!=(int8u)-1)
                    {
                        //Object based
                        int8u n_objects;
                        if (!GroupInfo.b_ajoc)
                        {
                            n_objects=objs_to_n_objects(GroupInfo.n_objects_code, GroupInfo.b_lfe);
                            Fill_Dup(Stream_Audio, 0, (S+" NumberOfObjects").c_str(), Ztring::ToZtring(n_objects));
                            //b_de_data_present=Substream_Info->second.b_dialog;
                            //if (b_de_data_present)
                            //{
                            //    de_max_gain=Substream_Info->second.dialog_max_gain;
                            //    de_channel_config=(int8u)-1;
                            //}
                        }
                        if (GroupInfo.nonstd_bed_channel_assignment_mask!=(int32u)-1)
                        {
                            Ztring BedChannelConfiguration=AC4_nonstd_bed_channel_assignment_mask_ChannelLayout(GroupInfo.nonstd_bed_channel_assignment_mask, Groups.size());
                            int8u num_channels_in_bed=AC4_nonstd_bed_channel_assignment_mask_2_num_channels_in_bed(GroupInfo.nonstd_bed_channel_assignment_mask);
                            if (!GroupInfo.b_ajoc && n_objects>num_channels_in_bed)
                                Fill_Dup(Stream_Audio, 0, (S+" NumberOfDynamicObjects").c_str(), Ztring::ToZtring(n_objects-num_channels_in_bed));
                            Fill_Dup(Stream_Audio, 0, (S+" BedChannelCount").c_str(), Ztring::ToZtring(num_channels_in_bed));
                            Fill_SetOptions(Stream_Audio, 0, (S+" BedChannelCount").c_str(), "N NIY");
                            Fill_Dup(Stream_Audio, 0, (S+" BedChannelCount/String").c_str(), MediaInfoLib::Config.Language_Get(Ztring::ToZtring(num_channels_in_bed), __T(" channel")));
                            Fill_SetOptions(Stream_Audio, 0, (S+" BedChannelCount/String").c_str(), "Y NIN");
                            Fill_Dup(Stream_Audio, 0, (S+" BedChannelConfiguration").c_str(), BedChannelConfiguration);
                        }
                    }
                }
        }

        if (Substream_Info->second.LoudnessInfo.dialnorm_bits!=(int8u)-1)
            Fill(Stream_Audio, 0, (S+" dialnorm").c_str(), -0.25*Substream_Info->second.LoudnessInfo.dialnorm_bits, 2);
        {
            const preprocessing& P=Substream_Info->second.Preprocessing;
            bitset<3> Preprocessing_Available;
            Preprocessing_Available[0]=(P.pre_dmixtyp_2ch!=(int8u)-1 && (!Value_IsEmpty(Ac4_pre_dmixtyp_2ch, P.pre_dmixtyp_2ch) || !Value_IsEmpty(Ac4_phase90_info_2ch, P.phase90_info_2ch)));
            Preprocessing_Available[1]=(P.pre_dmixtyp_5ch!=(int8u)-1 && (!Value_IsEmpty(Ac4_pre_dmixtyp_5ch, P.pre_dmixtyp_5ch)));
            Preprocessing_Available[2]=(P.phase90_info_mc!=(int8u)-1 && (!Value_IsEmpty(Ac4_phase90_info_mc, P.phase90_info_mc) || P.b_surround_attenuation_known || P.b_lfe_attenuation_known));
            if (Preprocessing_Available.any())
            {
                Fill(Stream_Audio, 0, (S+" Preprocessing").c_str(), "Yes");
                if (Preprocessing_Available[0])
                {
                    Fill(Stream_Audio, 0, (S+" Preprocessing PreviousMixType2ch").c_str(), Value(Ac4_pre_dmixtyp_2ch, P.pre_dmixtyp_2ch));
                    Fill(Stream_Audio, 0, (S+" Preprocessing Phase90FilterInfo2ch").c_str(), Value(Ac4_phase90_info_2ch, P.phase90_info_2ch));
                }
                if (Preprocessing_Available[1])
                {
                    Fill(Stream_Audio, 0, (S+" Preprocessing PreviousDownmixType5ch").c_str(), Value(Ac4_pre_dmixtyp_5ch, P.pre_dmixtyp_5ch));
                }
                if (Preprocessing_Available[2])
                {
                    Fill(Stream_Audio, 0, (S+" Preprocessing Phase90FilterInfo").c_str(), Value(Ac4_phase90_info_mc, P.phase90_info_mc));
                    Fill(Stream_Audio, 0, (S+" Preprocessing SurroundAttenuationKnown").c_str(), P.b_surround_attenuation_known?"Yes":"No");
                    Fill(Stream_Audio, 0, (S+" Preprocessing LfeAttenuationKnown").c_str(), P.b_lfe_attenuation_known?"Yes":"No");
                }
            }
        }
        {
            const de_info& D=Substream_Info->second.DeInfo;
            b_de_data_present=Substream_Info->second.b_dialog;
            if (b_de_data_present)
            {
                de_max_gain=Substream_Info->second.dialog_max_gain;
                de_channel_config=(int8u)-1;
            }
            else
            {
                b_de_data_present=D.b_de_data_present;
                if (b_de_data_present)
                {
                    de_max_gain=D.Config.de_max_gain;
                    de_channel_config=D.Config.de_channel_config;
                }
            }
            if (b_de_data_present)
            {
                Fill(Stream_Audio, 0, (S+" DialogueEnhancement").c_str(), "Yes");
                Fill(Stream_Audio, 0, (S+" DialogueEnhancement Enabled").c_str(), "Yes");
                if (de_max_gain!=(int8u)-1)
                {
                    Fill_Measure(Stream_Audio, 0, (S+" DialogueEnhancement MaxGain").c_str(), (de_max_gain+1)*3, __T(" dB"));
                    if (de_channel_config!=(int8u)-1)
                        Fill(Stream_Audio, 0, (S+" DialogueEnhancement ChannelConfiguration").c_str(), Value(Ac4_de_channel_config, de_channel_config));
                }
            }
        }
    }
}

//---------------------------------------------------------------------------
void File_Ac4::Streams_Finish()
{
}

//---------------------------------------------------------------------------
void File_Ac4::Read_Buffer_Unsynched()
{
}

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Ac4::Synchronize()
{
    //Synchronizing
    size_t Buffer_Offset_Current;
    while (Buffer_Offset<Buffer_Size)
    {
        Buffer_Offset_Current=Buffer_Offset;
        Synched=true; //For using Synched_Test()
        int8s i=0;
        const int8s count=(Frame_Count_Valid && Frame_Count_Valid<4)?Frame_Count_Valid:4;
        for (; i<count; i++) //4 frames in a row tested
        {
            if (!Synched_Test())
            {
                Buffer_Offset=Buffer_Offset_Current;
                Synched=false;
                return false;
            }
            if (!Synched)
                break;
            Buffer_Offset+=frame_size;
        }
        Buffer_Offset=Buffer_Offset_Current;
        if (i==count)
            break;
        Buffer_Offset++;
    }
    Buffer_Offset=Buffer_Offset_Current;

    //Parsing last bytes if needed
    if (Buffer_Offset+4>Buffer_Size)
    {
        while (Buffer_Offset+2<=Buffer_Size && (BigEndian2int16u(Buffer+Buffer_Offset)>>1)!=(0xAC40>>1))
            Buffer_Offset++;
        if (Buffer_Offset+1==Buffer_Size && Buffer[Buffer_Offset]==0xAC)
            Buffer_Offset++;
        return false;
    }

    //Synched
    return true;
}

//---------------------------------------------------------------------------
void File_Ac4::Synched_Init()
{
    Accept();
    
    if (!Frame_Count_Valid)
        Frame_Count_Valid=Config->ParseSpeed>=0.3?128:(IsSub?1:2);

    //FrameInfo
    PTS_End=0;
    if (!IsSub)
    {
        FrameInfo.DTS=0; //No DTS in container
        FrameInfo.PTS=0; //No PTS in container
    }
    DTS_Begin=FrameInfo.DTS;
    DTS_End=FrameInfo.DTS;
    if (Frame_Count_NotParsedIncluded==(int64u)-1)
        Frame_Count_NotParsedIncluded=0; //No Frame_Count_NotParsedIncluded in the container
}

//---------------------------------------------------------------------------
bool File_Ac4::Synched_Test()
{
    //Must have enough buffer for having header
    if (Buffer_Offset+4>=Buffer_Size)
        return false;

    //sync_word
    sync_word=BigEndian2int16u(Buffer+Buffer_Offset);
    if ((sync_word>>1)!=(0xAC40>>1)) //0xAC40 or 0xAC41
    {
        Synched=false;
        return true;
    }

    //frame_size
    frame_size=BigEndian2int16u(Buffer+Buffer_Offset+2);
    if (frame_size==(int16u)-1)
    {
        if (Buffer_Offset+7>Buffer_Size)
            return false;
        frame_size=BigEndian2int24u(Buffer+Buffer_Offset+4)+7;
    }
    else
        frame_size+=4;

    //crc_word
    if (sync_word&1)
    {
        frame_size+=2;
        if (Buffer_Offset+frame_size>Buffer_Size)
            return false;
        if (!CRC_Compute(frame_size))
            Synched=false;
    }

    //We continue
    return true;
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ac4::Read_Buffer_Continue()
{
    if (MustParse_dac4)
    {
        dac4();
        return;
    }

    if (!MustSynchronize)
    {
        if (!Frame_Count)
            Synched_Init();
        raw_ac4_frame();
        Buffer_Offset=Buffer_Size;
    }
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ac4::Header_Parse()
{
    //Parsing
    //sync_word & frame_size16 were previously calculated in Synched_Test()
    int16u frame_size16;
    Skip_B2 (                                                   "sync_word");
    Get_B2 (frame_size16,                                       "frame_size");
    if (frame_size16==0xFFFF)
        Skip_B3(                                                "frame_size");

    //Filling
    Header_Fill_Size(frame_size);
    Header_Fill_Code(sync_word, "ac4_syncframe");
}

//---------------------------------------------------------------------------
void File_Ac4::Data_Parse()
{
    Element_Info1(Frame_Count);

    //CRC
    if (Element_Code==0xAC41)
        Element_Size-=2;

    //Parsing
    raw_ac4_frame();
    
    //CRC
    Element_Offset=Element_Size;
    if (Element_Code==0xAC41)
    {
        Element_Size+=2;
        Skip_B2(                                                "crc_word");
    }
}

//---------------------------------------------------------------------------
void File_Ac4::raw_ac4_frame()
{
    Element_Begin1("raw_ac4_frame");
    BS_Begin();
    ac4_toc();
    if (Element_Offset!=Element_Size) //Indicates that ac4_toc was not fully parsed
        raw_ac4_frame_substreams();
    Element_End0();

    Frame_Count++;
    FILLING_BEGIN();
        if (!Status[IsFilled] && Frame_Count>=Frame_Count_Valid)
        {
            Fill();
            Finish();
        }
    FILLING_END();

    if (!Presentations_IFrame.empty())
    {
        Presentations=Presentations_IFrame;
        Presentations_IFrame.clear();
        Groups=Groups_IFrame;
        Groups_IFrame.clear();
        for (std::map<int8u, audio_substream>::iterator AudioSubstream=AudioSubstreams_IFrame.begin(); AudioSubstream!=AudioSubstreams_IFrame.end(); ++AudioSubstream)
        {
            AudioSubstreams[AudioSubstream->first]=AudioSubstream->second;
            AudioSubstream->second.Buffer.Data=NULL; //This is a move, Buffer moves from AudioSubstreams_IFrame to AudioSubstreams
        }
        AudioSubstreams_IFrame.clear();
    }
}

//---------------------------------------------------------------------------
void File_Ac4::raw_ac4_frame_substreams()
{
    size_t byte_align=Data_BS_Remain()%8;
    if (byte_align)
        Skip_S1(byte_align,                                     "byte_align");
    BS_End();
    if (payload_base)
    {
        if (payload_base>Element_Size-Element_Offset)
        {
            Skip_XX(Element_Size-Element_Offset,                "?");
            return;
        }
        size_t Max=Buffer_Offset+(size_t)(Element_Offset+payload_base);
        size_t Min=Buffer_Offset+(size_t)Element_Offset;
        size_t End=Min;
        while (End<Max && Buffer[End]>=0x20 && Buffer[End]<0x7F)
            End++;
        if (End!=Min)
        {
            size_t Encoded_Library_Size=(End-Min);
            string Encoded_Library;
            Get_String (Encoded_Library_Size, Encoded_Library,  "Library name (guessed)");
            if (Retrieve_Const(Stream_Audio, 0, Audio_Encoded_Library).empty())
            {
                //Fill(Stream_General, 0, General_Encoded_Library, Encoded_Library);
                //Fill(Stream_Audio, 0, Audio_Encoded_Library, Encoded_Library);
            }
            payload_base-=(int32u)Encoded_Library_Size;
        }
        while (End<Max && !Buffer[End])
            End++;
        Skip_XX(payload_base,                                   End!=Max?"Unknown":"fill_area");
    }
    int64u Substreams_StartOffset=Element_Offset;

    //Check integrity
    if (Substream_Size.empty())
        Substream_Size.push_back(Element_Size-Substreams_StartOffset); // 1 substream only
    size_t Substreams_EndOffset=Substreams_StartOffset;
    for (size_t i=0; i<Substream_Size.size(); i++)
        Substreams_EndOffset+=Substream_Size[i];

    //Parsing presentation substreams first
    if (bitstream_version>=2)
        for (size_t i=0; i<Presentations.size(); i++)
        {
            if (Presentations[i].presentation_version>=1)
            {
                int8u substream_index=Presentations[i].substream_index;
                if (substream_index>=Substream_Size.size())
                {
                    Skip_XX(Element_Size-Element_Offset,        "?");
                    return;
                }
                if (Substream_Size[substream_index])
                {
                    Element_Offset=Substreams_StartOffset;
                    for (size_t i=0; i<substream_index; i++)
                        Element_Offset+=Substream_Size[i];
                    int64u Element_Size_Save=Element_Size;
                    Element_Size=Element_Offset+Substream_Size[substream_index];

                    ac4_presentation_substream(substream_index, i);

                    if (Element_Offset<Element_Size)
                    {
                        Skip_XX(Element_Size-Element_Offset,                "?");
                    }
                    Element_Size=Element_Size_Save;
                }
            }
        }

    //Parsing other substreams
    for (int8u substream_index=0; substream_index<n_substreams; substream_index++)
    {
        Element_Offset=Substreams_StartOffset;
        for (size_t i=0; i<substream_index; i++)
            Element_Offset+=Substream_Size[i];
        int64u Element_Size_Save=Element_Size;
        Element_Size=Element_Offset+Substream_Size[substream_index];

        std::map <int8u, substream_type_t>::iterator SubstreamTypeIt=Substream_Type.find(substream_index);
        substream_type_t SubstreamTypeInfo=SubstreamTypeIt==Substream_Type.end()?Type_Unknown:SubstreamTypeIt->second;
        if (SubstreamTypeInfo>=sizeof(substream_type_Trace)/sizeof(const char*))
            SubstreamTypeInfo=Type_Unknown;
        switch (SubstreamTypeInfo)
        {
            case Type_Ac4_Substream:
                ac4_substream(substream_index);
                break;
            case Type_Ac4_Presentation_Substream:
                Element_Offset=Element_Size; // Previously parsed, skip
                break;
            default:
                Skip_XX(Substream_Size[substream_index],        substream_type_Trace[SubstreamTypeInfo]);
                Param_Info1(substream_index);
        }

        if (Element_Offset<Element_Size)
        {
            Skip_XX(Element_Size-Element_Offset,                "?");
        }
        Element_Size=Element_Size_Save;
    }

    if (Element_Offset<Element_Size)
        Skip_XX(Element_Size-Element_Offset,                    "fill_area");

    Substream_Size.clear();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_toc()
{
    int16u sequence_counter, n_presentations;
    bool b_iframe_global;
    Element_Begin1("raw_ac4_toc");
    Get_S1 (2, bitstream_version,                               "bitstream_version");
    if (bitstream_version==3)
    {
        int32u bitstream_version32; 
        Get_V4 (2, bitstream_version32,                         "bitstream_version");
        bitstream_version32+=3;
        bitstream_version=(int8u)bitstream_version32;
    }
    Get_S2 (10, sequence_counter,                               "sequence_counter");
    TEST_SB_SKIP(                                               "b_wait_frames");
        int8u wait_frames;
        Get_S1 (3, wait_frames,                                 "wait_frames");
        if (wait_frames)
            Skip_S1(2,                                          "br_code");
    TEST_SB_END();
    Get_SB (   fs_index,                                        "fs_index"); Param_Info1(Ztring::ToZtring(Ac4_fs_index[fs_index])+__T(" Hz"));
    Get_S1 (4, frame_rate_index,                                "frame_rate_index"); Param_Info2(Ac4_frame_rate[fs_index][frame_rate_index], " fps");
    Get_SB (   b_iframe_global,                                 "b_iframe_global");
    bool NoSkip=false;
    if (b_iframe_global)
    {
        Element_Level-=2;
        Element_Info1("I");
        Element_Level+=2;
        IFrames.push_back(Frame_Count);
        if (AudioSubstreams.empty())
            NoSkip=true;
    }
    else
    {
        //We parse only Iframes, but also the frames associated to this I-frame
        if (!AudioSubstreams.empty())
            for (map<int8u, audio_substream>::iterator Substream_Info=AudioSubstreams.begin(); Substream_Info!=AudioSubstreams.end(); Substream_Info++)
                if (Substream_Info->second.Buffer_Index)
                    NoSkip=true;
    }
    if (!NoSkip)
    {
        Element_End0();
        BS_End();
        Element_Offset=Element_Size;
        return;
    }
    if (b_iframe_global)
    {
        Presentations.clear();
        Groups.clear();
        AudioSubstreams.clear();
    }
    else
    {
        Presentations_IFrame=Presentations;
        Groups_IFrame=Groups;

        for (map<int8u, audio_substream>::iterator AudioSubstream=AudioSubstreams.begin(); AudioSubstream!=AudioSubstreams.end();)
        {
            if (!AudioSubstream->second.Buffer_Index)
            {
                AudioSubstreams_IFrame[AudioSubstream->first]=AudioSubstream->second;
                AudioSubstream->second.Buffer.Data=NULL; //This is a move, Buffer moves from AudioSubstreams to AudioSubstreams_IFrame
                AudioSubstreams.erase(AudioSubstream++);
            }
            else
                ++AudioSubstream;
        }
    }
    max_group_index=0;

    TESTELSE_SB_SKIP(                                           "b_single_presentation");
        n_presentations=1;
    TESTELSE_SB_ELSE(                                           "b_single_presentation");
        TESTELSE_SB_SKIP(                                       "b_more_presentations");
            int32u n_presentations32;
            Get_V4 (2, n_presentations32,                       "n_presentations_minus2");
            n_presentations32+=2;
            n_presentations=(int8u)n_presentations32;
            Param_Info1(n_presentations);
        TESTELSE_SB_ELSE(                                       "b_more_presentations");
            n_presentations=0;
        TESTELSE_SB_END();
    TESTELSE_SB_END();

    payload_base=0;
    TEST_SB_SKIP(                                               "b_payload_base");
        Get_S4 (5, payload_base,                                "payload_base_minus1");
        payload_base++;
        if (payload_base==32)
        {
            Get_V4 (3, payload_base,                            "payload_base");
            payload_base+=32;
        }
    TEST_SB_END();

    if (bitstream_version<=1)
    {
        Presentations.resize(n_presentations);
        for(int8u Pos=0; Pos<n_presentations; Pos++)
            ac4_presentation_info(Presentations[Pos]);
    }
    else
    {
        TEST_SB_SKIP(                                           "b_program_id");
            int16u short_program_id;
            Get_S2 (16, short_program_id,                       "short_program_id");
            bool FillID=Retrieve_Const(Stream_Audio, 0, Audio_UniqueID).empty();
            if (FillID)
            {
                Fill(Stream_General, 0, General_UniqueID, short_program_id);
                Fill(Stream_Audio, 0, Audio_UniqueID, short_program_id);
            }
            TEST_SB_SKIP(                                       "b_program_uuid_present");
                int128u program_uuid;
                Get_UUID (program_uuid,                         "program_uuid");
                if (FillID)
                {
                    Fill(Stream_General, 0, General_UniqueID, Ztring().From_UUID(program_uuid));
                    Fill(Stream_Audio, 0, Audio_UniqueID, Ztring().From_UUID(program_uuid));
                }
            TEST_SB_END();
        TEST_SB_END();

        Presentations.resize(n_presentations);
        for (int8u Pos=0; Pos<n_presentations; Pos++)
            ac4_presentation_v1_info(Presentations[Pos]);

        size_t max_group_Size=max_group_index+1;
        Groups.resize(max_group_Size);
        for (int8u Pos=0; Pos< max_group_Size; Pos++)
            ac4_substream_group_info(Groups[Pos]);
    }

    substream_index_table();

    size_t byte_align=Data_BS_Remain()%8;
    if (byte_align)
        Skip_S1(byte_align,                                     "byte_align");

    Element_End0();

    ac4_toc_Compute(Presentations, Groups, false);
}

void File_Ac4::ac4_toc_Compute(vector<presentation>& Ps, vector<group>& Gs, bool FromDac4)
{
    //Compute helper values
    for (size_t p=0; p<Ps.size(); p++)
    {
        presentation& P=Ps[p];
        bool b_obj_or_ajoc=false, b_obj_or_ajoc_adaptive=false;
        P.Language.clear();
        for (size_t Pos=0; Pos<P.substream_group_info_specifiers.size(); Pos++)
        {
            int8u Group_Index=P.substream_group_info_specifiers[Pos];
            const group& Group=Gs[Group_Index];
            if (!Group.ContentInfo.language_tag_bytes.empty() && (Group.ContentInfo.content_classifier==0 || Group.ContentInfo.content_classifier==1 || Group.ContentInfo.content_classifier==4))
            {
                if (!P.Language.empty())
                    P.Language+=" / ";
                P.Language+=Group.ContentInfo.language_tag_bytes;
            }
            for (size_t Pos2=0; Pos2<Group.Substreams.size(); Pos2++)
            {
                const group_substream& S=Group.Substreams[Pos2];
                if (S.substream_type==Type_Ac4_Substream)
                {
                    P.n_substreams_in_presentation++;
                    if (FromDac4)
                        continue;

                    // pres_ch_mode && pres_ch_mode_core
                    if (Group.b_channel_coded)
                    {
                        P.pres_ch_mode=Superset(P.pres_ch_mode, S.ch_mode);
                        P.pres_ch_mode_core=Superset(P.pres_ch_mode_core, S.ch_mode_core);
                    }
                    else
                    {
                        b_obj_or_ajoc=true;
                        if (S.b_ajoc && S.b_static_dmx)
                            P.pres_ch_mode_core=Superset(P.pres_ch_mode_core, S.ch_mode_core);
                        else
                            b_obj_or_ajoc_adaptive=1;
                    }

                    // immersive_stereo
                    if (S.immersive_stereo!=(int8u)-1 && P.pres_immersive_stereo==(int8u)-1) // Prioritizing first presentation
                        P.pres_immersive_stereo=S.immersive_stereo;

                    if (S.ch_mode>=11 && S.ch_mode<=14)
                    {
                        // b_pres_4_back_channels_present
                        if (S.b_4_back_channels_present)
                            P.b_pres_4_back_channels_present=true;

                        // b_pres_centre_present
                        if (S.b_centre_present)
                            P.b_pres_centre_present=true;

                        // pres_top_channel_pairs
                        if (P.pres_top_channel_pairs<S.top_channel_pairs)
                            P.pres_top_channel_pairs=S.top_channel_pairs;
                    }
                }
            }
        }
        if (b_obj_or_ajoc)
            P.pres_ch_mode=-1;
        if (b_obj_or_ajoc_adaptive || P.pres_ch_mode_core==P.pres_ch_mode)
            P.pres_ch_mode_core=-1;
    }
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_presentation_info(presentation& P)
{
    P.reset_cumulative();

    bool b_single_substream, b_add_emdf_substreams=false;

    Element_Begin1(                                             "ac4_presentation_info");
    Get_SB (b_single_substream,                                 "b_single_substream");
    if (!b_single_substream)
    {
        Get_S1 (3, P.presentation_config,                       "presentation_config");
        if (P.presentation_config==7) {
            int32u presentation_config32;
            Get_V4 (2, presentation_config32,                   "presentation_config");
            P.presentation_config+=presentation_config32;
        }
        Param_Info1(Value(Ac4_presentation_config, P.presentation_config));
    }
    Get_VB (P.presentation_version,                             "presentation_version");
    if (!b_single_substream && P.presentation_config==6)
    {
        b_add_emdf_substreams=true;
    }
    else
    {
        Skip_S1(3,                                              "mdcompat");

        TEST_SB_SKIP(                                           "b_presentation_id");
            Get_V4 (2, P.presentation_id,                       "presentation_id");
        TEST_SB_END();

        frame_rate_multiply_info();
        P.Substreams.resize(P.Substreams.size()+1);
        emdf_info(P.Substreams.back());

        if (b_single_substream)
        {
            ac4_substream_info(P);
        }
        else
        {
            bool b_hsf_ext;
            Get_SB (b_hsf_ext,                                  "b_hsf_ext");
            switch (P.presentation_config) // TODO: Simplify
            {
            case 0: // Dry main + Dialog
                    ac4_substream_info(P); // Main
                    if (b_hsf_ext)
                        ac4_hsf_ext_substream_info(Groups.back().Substreams[0], true); // Main HSF
                    ac4_substream_info(P); // Dialog
                    break;
            case 1: // Main + DE
                    ac4_substream_info(P); // Main
                    if (b_hsf_ext)
                        ac4_hsf_ext_substream_info(Groups.back().Substreams[0], true); // Main HSF
                    ac4_substream_info(P); // DE
                    break;
            case 2: // Main + Associate
                    ac4_substream_info(P); // Main
                    if (b_hsf_ext)
                        ac4_hsf_ext_substream_info(Groups.back().Substreams[0], true); // Main HSF
                    ac4_substream_info(P); // Associate
                    break;
            case 3: // Dry Main + Dialog + Associate
                    ac4_substream_info(P); // Main
                    if (b_hsf_ext)
                        ac4_hsf_ext_substream_info(Groups.back().Substreams[0], true); // Main HSF
                    ac4_substream_info(P); // Dialog
                    ac4_substream_info(P); // Associate
                    break;
            case 4: // Main + DE Associate
                    ac4_substream_info(P); // Main
                    if (b_hsf_ext)
                        ac4_hsf_ext_substream_info(Groups.back().Substreams[0], true); // Main HSF
                    ac4_substream_info(P); // DE
                    ac4_substream_info(P); // Associate
                    break;
            case 5: // Main + HSF ext
                    ac4_substream_info(P); // Main
                    if (b_hsf_ext)
                        ac4_hsf_ext_substream_info(Groups.back().Substreams[0], true); // Main HSF
                    break;
            default:
                    presentation_config_ext_info(P);
            }
        }
        Skip_SB(                                                "b_pre_virtualized");
        Get_SB (b_add_emdf_substreams,                          "b_add_emdf_substreams");
    }

    if (b_add_emdf_substreams)
    {
        int8u n_add_emdf_substreams;
        Get_S1 (2, n_add_emdf_substreams,                       "n_add_emdf_substreams");
        if (n_add_emdf_substreams==0)
        {
            int32u n_add_emdf_substreams32;
            Get_V4 (2, n_add_emdf_substreams32,                 "n_add_emdf_substreams");
            n_add_emdf_substreams32+=4;
            n_add_emdf_substreams=(int8u)n_add_emdf_substreams32;
        }

        size_t Offset=P.Substreams.size();
        P.Substreams.resize(Offset+n_add_emdf_substreams);
        for (int8u Pos=0; Pos<n_add_emdf_substreams; Pos++)
        {
            emdf_info(P.Substreams[Offset+Pos]);
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_presentation_v1_info(presentation& P)
{
    P.reset_cumulative();

    bool b_single_substream_group, b_add_emdf_substreams = false;
    int8u n_substream_groups=0, b_multi_pid_PresentAndValue=(int8u)-1;

    Element_Begin1(                                             "ac4_presentation_v1_info");
    Get_SB(b_single_substream_group,                            "b_single_substream_group");
    if (!b_single_substream_group)
    {
        Get_S1(3, P.presentation_config,                        "presentation_config");
        if (P.presentation_config==7) {
            int32u presentation_config32;
            Get_V4(2, presentation_config32,                    "presentation_config");
            P.presentation_config+=presentation_config32;
        }
        Param_Info1(Value(Ac4_presentation_config, P.presentation_config));
    }
    if (bitstream_version!=1)
        Get_VB (P.presentation_version,                         "presentation_version");
    else
        P.presentation_version=0;
    if (!b_single_substream_group && P.presentation_config==6)
    {
        b_add_emdf_substreams=true;
    }
    else
    {
        if (bitstream_version!=1)
            Skip_S1(3,                                          "mdcompat");
        TEST_SB_SKIP(                                           "b_presentation_id");
            Get_V4 (2, P.presentation_id,                       "presentation_id");
        TEST_SB_END();
        frame_rate_multiply_info();
        frame_rate_fractions_info(P);
        P.Substreams.resize(P.Substreams.size()+1);
        emdf_info(P.Substreams.back());
        TEST_SB_SKIP(                                           "b_presentation_filter");
            Skip_SB(                                            "b_enable_presentation");
        TEST_SB_END();
        if (b_single_substream_group)
        {
            ac4_sgi_specifier(P);
            n_substream_groups=1;
        }
        else
        {
            bool b_multi_pid;
            Get_SB (b_multi_pid,                                "b_multi_pid");
            b_multi_pid_PresentAndValue=b_multi_pid;
            switch (P.presentation_config) // TODO: Simplify
            {
                case 0: // Music and Effects + Dialogue
                        ac4_sgi_specifier(P);
                        ac4_sgi_specifier(P);
                        n_substream_groups=2;
                        break;
                case 1: // Main + DE
                        ac4_sgi_specifier(P);
                        ac4_sgi_specifier(P);
                        n_substream_groups=1;
                        break;
                case 2: // Main + Associated Audio
                        ac4_sgi_specifier(P);
                        ac4_sgi_specifier(P);
                        n_substream_groups=2;
                        break;
                case 3: // Music and Effects + Dialogue + Associated Audio
                        ac4_sgi_specifier(P);
                        ac4_sgi_specifier(P);
                        ac4_sgi_specifier(P);
                        n_substream_groups=3;
                        break;
                case 4: // Main + DE + Associated Audio
                        ac4_sgi_specifier(P);
                        ac4_sgi_specifier(P);
                        ac4_sgi_specifier(P);
                        n_substream_groups=2;
                        break;
                case 5: // Arbitrary number of roles and substream groups
                        int8u n_substream_groups_minus2;
                        Get_S1 (2, n_substream_groups_minus2,           "n_substream_groups_minus2");
                        n_substream_groups=n_substream_groups_minus2+2;
                        if (n_substream_groups==5)
                        {
                            int32u n_substream_groups32;
                            Get_V4 (2, n_substream_groups32,            "n_substream_groups");
                            n_substream_groups32+=5;
                            n_substream_groups=(int8u)n_substream_groups32;
                        }
                        for (int8u Pos=0; Pos<n_substream_groups; Pos++)
                            ac4_sgi_specifier(P);
                        break;
                default: // EMDF and other data
                        presentation_config_ext_info(P);
            }
        }
        Skip_SB(                                                "b_pre_virtualized");
        Get_SB (b_add_emdf_substreams,                          "b_add_emdf_substreams");
        ac4_presentation_substream_info(P);
    }

    if (b_add_emdf_substreams)
    {
        int8u n_add_emdf_substreams;
        Get_S1 (2, n_add_emdf_substreams,                       "n_add_emdf_substreams");
        if (n_add_emdf_substreams==0)
        {
            int32u n_add_emdf_substreams32;
            Get_V4 (2, n_add_emdf_substreams32,                 "n_add_emdf_substreams");
            n_add_emdf_substreams32+=4;
            n_add_emdf_substreams=(int8u)n_add_emdf_substreams32;
        }
        size_t Offset=P.Substreams.size();
        P.Substreams.resize(Offset+n_add_emdf_substreams);
        for (int8u Pos=0; Pos<n_add_emdf_substreams; Pos++)
        {
            emdf_info(P.Substreams[Offset+Pos]);
        }
    }

    P.n_substream_groups=n_substream_groups;
    P.b_multi_pid_PresentAndValue=b_multi_pid_PresentAndValue;
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_sgi_specifier(presentation& P)
{
    Element_Begin1(                                             "ac4_sgi_specifier");
    if (bitstream_version==1)
    {
        P.substream_group_info_specifiers.push_back(Groups.size());
        Groups.resize(Groups.size()+1);
        ac4_substream_group_info(Groups[Groups.size()-1]);
    }
    else
    {
        int8u group_index;
        Get_S1 (3, group_index,                                 "group_index");
        if (group_index==7)
        {
            int32u group_index32;
            Get_V4 (2, group_index32,                           "group_index");
            group_index+=(int8u)group_index32;
        }
        if (max_group_index<group_index)
            max_group_index=group_index;

        P.substream_group_info_specifiers.push_back(group_index);
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_substream_info(presentation& P)
{
    int8u ch_mode, substream_index;
    Element_Begin1(                                             "ac4_substream_info");
        bool b_content_type;
        content_info ContentInfo;
        Get_V4 (Ac4_channel_mode, ch_mode,                      "channel_mode");
        if (ch_mode==12)
        {
            int32u channel_mode32;
            Get_V4 (2, channel_mode32,                          "channel_mode");
            ch_mode+=(int8u)channel_mode32;
        }
        Param_Info1(Value(Ac4_ch_mode_String, ch_mode));

        if (fs_index)
        {
            TEST_SB_SKIP(                                       "b_sf_multiplier");
                Skip_SB(                                        "sf_multiplier");
            TEST_SB_END();
        }

        TEST_SB_SKIP(                                           "b_bitrate_info");
            Skip_V4(3, 5, 1,                                    "bitrate_indicator");
        TEST_SB_END();
        if (ch_mode>=7 && ch_mode<=10)
            Skip_SB(                                            "add_ch_base");

        TEST_SB_GET(b_content_type,                             "b_content_type");
            content_type(ContentInfo);
        TEST_SB_END();

        vector<bool> b_iframes;
        for (int8u Pos=0; Pos<frame_rate_factor; Pos++)
        {
            bool b_iframe;
            Get_SB (b_iframe,                                   "b_iframe");
            b_iframes.push_back(b_iframe);
        }

        Get_S1 (2, substream_index,                             "substream_index");
        if (substream_index==3)
        {
            int32u substream_index32;
            Get_V4 (2, substream_index32,                       "substream_index");
            substream_index32+=3;
            substream_index=(int8u)substream_index32;
        }
        
        for (size_t i=0; i<frame_rate_factor; i++)
        {
            P.substream_group_info_specifiers.push_back(Groups.size()); //Fake group
            Groups.resize(Groups.size()+1);
            group& G=Groups.back();
            G.b_channel_coded=true;

            G.ContentInfo=ContentInfo;
            G.Substreams.resize(1);
            group_substream& GroupInfo=G.Substreams[0];
            GroupInfo.substream_type=Type_Ac4_Substream;
            GroupInfo.substream_index=substream_index+i;
            GroupInfo.b_iframe=b_iframes[i];
            GroupInfo.sus_ver=false;
            GroupInfo.ch_mode=ch_mode;

            Substream_Type[substream_index+i]=Type_Ac4_Substream;
        }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_substream_group_info(group& G)
{
    bool b_substreams_present, b_single_substream;
    int8u n_lf_substreams;

    Element_Begin1(                                             "ac4_substream_group_info");
    Get_SB (b_substreams_present,                               "b_substreams_present");
    Get_SB (G.b_hsf_ext,                                        "b_hsf_ext");
    TESTELSE_SB_GET(b_single_substream,                         "b_single_substream");
        n_lf_substreams=1;
    TESTELSE_SB_ELSE(                                           "b_single_substream");
        Get_S1 (2, n_lf_substreams,                             "n_lf_substreams_minus2");
        n_lf_substreams+=2;
        if (n_lf_substreams==5)
        {
            int32u n_lf_substreams32;
            Get_V4 (2, n_lf_substreams32,                       "n_lf_substreams");
            n_lf_substreams+=(int8u)n_lf_substreams32;
        }
    TESTELSE_SB_END();
    TESTELSE_SB_GET (G.b_channel_coded,                         "b_channel_coded");
        G.Substreams.resize(n_lf_substreams);
        for (int8u Pos=0; Pos<n_lf_substreams; Pos++)
        {
            group_substream& S=G.Substreams[Pos];
            if (bitstream_version==1)
                Get_SB (S.sus_ver,                              "sus_ver");
            else
                S.sus_ver=true;

            ac4_substream_info_chan(S, Pos, b_substreams_present);
            if (G.b_hsf_ext)
                ac4_hsf_ext_substream_info(S, b_substreams_present);
        }
    TESTELSE_SB_ELSE(                                           "b_channel_coded");
        TEST_SB_SKIP(                                           "b_oamd_substream");
            G.Substreams.resize(1);
            oamd_substream_info(G.Substreams[0],  b_substreams_present);
        TEST_SB_END();
        G.Substreams.resize(n_lf_substreams);
        for (int8u Pos=0; Pos<n_lf_substreams; Pos++)
        {
            group_substream& S=G.Substreams[Pos];
            TESTELSE_SB_GET (S.b_ajoc,                          "b_ajoc");
                ac4_substream_info_ajoc(S, b_substreams_present);
            TESTELSE_SB_ELSE(                                   "b_ajoc");
                ac4_substream_info_obj(S, b_substreams_present);
            TESTELSE_SB_END();
            if (G.b_hsf_ext)
                ac4_hsf_ext_substream_info(S, b_substreams_present);
        }
    TESTELSE_SB_END();
    TEST_SB_SKIP(                                               "b_content_type");
        content_type(G.ContentInfo);
    TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_hsf_ext_substream_info(group_substream& G, bool b_substreams_present)
{
    int8u substream_index;
    Element_Begin1(                                             "ac4_hsf_ext_substream_info");
    if (b_substreams_present)
    {
        Get_S1 (2, substream_index,                             "substream_index");
        if (substream_index==3)
        {
            int32u substream_index32;
            Get_V4 (2, substream_index32,                       "substream_index");
            substream_index32+=3;
            substream_index=(int8u)substream_index32;
        }

        G.hsf_substream_index=substream_index;

        Substream_Type[substream_index]=Type_Ac4_Hsf_Ext_Substream;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_substream_info_chan(group_substream& G, size_t Pos, bool b_substreams_present)
{
    G.substream_type=Type_Ac4_Substream;

    Element_Begin1(                                             "ac4_substream_info_chan");
    Get_V4(Ac4_channel_mode2, G.ch_mode,                        "channel_mode");
    if (G.ch_mode==16)
    {
        int32u channel_mode32;
        Get_V4(2, channel_mode32,                               "channel_mode");
        G.ch_mode+=(int8u)channel_mode32;
    }

    //Testing immersive stereo
    for (size_t p=0; p<Presentations.size(); p++)
    {
        const presentation& Presentation_Current=Presentations[p];
        for (size_t s=0; s<Presentation_Current.substream_group_info_specifiers.size(); s++)
        {
            const size_t& Specifier=Presentation_Current.substream_group_info_specifiers[s];
            if (Specifier==Pos)
            {
                if (Presentation_Current.presentation_version==2)
                {
                    if (G.ch_mode>=5 && G.ch_mode<=10)
                    {
                        //Difference with other versions
                        G.immersive_stereo=G.ch_mode-5;
                        G.ch_mode=1;
                    }
                }
            }
        }
    }


    switch (G.ch_mode)
    {
        case 11:
        case 13:
                    G.ch_mode_core=5;
                    break;
        case 12:
        case 14:
                    G.ch_mode_core=6;
                    break;
    }
    Param_Info1(Value(Ac4_ch_mode_String, G.ch_mode));
    if (G.ch_mode_core!=(int8u)-1)
        Param_Info1(Value(Ac4_ch_mode_String, G.ch_mode_core));
    if (G.immersive_stereo!=(int8u)-1)
        Param_Info1(Value(Ac4_immersive_stereo_String, G.immersive_stereo));
    if (G.ch_mode>=11 && G.ch_mode<=14)
    {
        Get_SB (   G.b_4_back_channels_present,                 "b_4_back_channels_present");
        Get_SB (   G.b_centre_present,                          "b_centre_present");
        Get_S1 (2, G.top_channels_present,                      "top_channels_present");

        // Compute
        G.top_channel_pairs=0;
        switch (G.top_channels_present)
        {
            case 1:
            case 2:
                    if (!G.top_channel_pairs)
                        G.top_channel_pairs=1;
                    break;
            case 3:
                    G.top_channel_pairs=2;
                    break;
        }
    }
    if (fs_index==1)
    {
        TEST_SB_SKIP(                                           "b_sf_multiplier");
            Skip_SB(                                            "sf_multiplier");
        TEST_SB_END();
    }
    TEST_SB_SKIP(                                               "b_bitrate_info");
        Skip_V4(3, 5, 1,                                        "bitrate_indicator");
    TEST_SB_END();
    if (G.ch_mode>=7 && G.ch_mode<=10)
        Skip_SB(                                                "add_ch_base");
    vector<bool> b_iframes;
    for (int8u Pos=0; Pos<frame_rate_factor; Pos++)
    {
        bool b_iframe;
        Get_SB (b_iframe,                                       "b_audio_ndot");
        b_iframes.push_back(b_iframe);
    }
    if (b_substreams_present)
    {
        int8u substream_index;
        Get_S1 (2, substream_index,                             "substream_index");
        if (substream_index==3)
        {
            int32u substream_index32;
            Get_V4 (2, substream_index32,                       "substream_index");
            substream_index32+=3;
            substream_index=(int8u)substream_index32;
        }

        G.substream_index=substream_index;
        G.b_iframe=b_iframes[0]; //TODO frame_rate_factor

        Substream_Type[substream_index]=Type_Ac4_Substream;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_substream_info_ajoc(group_substream& G, bool b_substreams_present)
{
    G.sus_ver=true;
    G.substream_type=Type_Ac4_Substream;

    int8u substream_index;
    Element_Begin1(                                             "ac4_substream_info_ajoc");
    Get_SB (G.b_lfe,                                            "b_lfe");
    TESTELSE_SB_GET (G.b_static_dmx,                            "b_static_dmx");
        G.n_fullband_dmx_signals=5;
    TESTELSE_SB_ELSE(                                           "b_static_dmx");
        Get_S1 (4, G.n_fullband_dmx_signals,                    "n_fullband_dmx_signals_minus1");
        G.n_fullband_dmx_signals++;
        bed_dyn_obj_assignment(G, G.n_fullband_dmx_signals);
    TESTELSE_SB_END();
    TEST_SB_SKIP(                                               "b_oamd_common_data_present");
        oamd_common_data();
    TEST_SB_END();
    Get_S1 (4, G.n_fullband_upmix_signals,                      "n_fullband_upmix_signals_minus1");
    G.n_fullband_upmix_signals++;
    if (G.n_fullband_upmix_signals==16)
    {
        int32u n_fullband_upmix_signals32;
        Get_V4 (3, n_fullband_upmix_signals32,                  "n_fullband_upmix_signals");
        n_fullband_upmix_signals32+=16;
        G.n_fullband_upmix_signals=(int8u)n_fullband_upmix_signals32;
    }
    bed_dyn_obj_assignment(G, G.n_fullband_upmix_signals);
    if (fs_index)
    {
        TEST_SB_SKIP(                                           "b_sf_multiplier");
            Skip_SB(                                            "sf_multiplier");
        TEST_SB_END();
    }
    TEST_SB_SKIP(                                               "b_bitrate_info");
        Skip_V4(3, 5, 1,                                        "bitrate_indicator");
    TEST_SB_END();
    vector<bool> b_iframes;
    for (int8u Pos=0; Pos<frame_rate_factor; Pos++)
    {
        bool b_iframe;
        Get_SB (b_iframe,                                       "b_audio_ndot");
        b_iframes.push_back(b_iframe);
    }
    if (b_substreams_present)
    {
        Get_S1 (2, substream_index,                             "substream_index");
        if (substream_index==3)
        {
            int32u substream_index32;
            Get_V4 (2, substream_index32,                       "substream_index");
            substream_index32+=3;
            substream_index=(int8u)substream_index32;
        }

        G.substream_index=substream_index;
        G.b_iframe=b_iframes[0]; //TODO frame_rate_factor

        Substream_Type[substream_index]=Type_Ac4_Substream;
    }
    Element_End0();

    if (G.b_static_dmx)
        G.ch_mode_core=3+G.b_lfe;
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_substream_info_obj(group_substream& G, bool b_substreams_present)
{
    G.sus_ver=true;
    G.substream_type=Type_Ac4_Substream;

    Element_Begin1(                                             "ac4_substream_info_obj");
    Get_S1 (3, G.n_objects_code,                                "n_objects_code");
    TESTELSE_SB_GET (G.b_dynamic_objects,                       "b_dynamic_objects");
        Get_SB (G.b_lfe,                                        "b_lfe");
    TESTELSE_SB_ELSE(                                           "b_dynamic_objects");
        G.b_lfe=false;
        TESTELSE_SB_SKIP(                                       "b_bed_objects");
            TEST_SB_SKIP(                                       "b_bed_start");
                TESTELSE_SB_SKIP(                               "b_ch_assign_code");
                    int8u bed_chan_assign_code;
                    Get_S1 (3, bed_chan_assign_code,            "bed_chan_assign_code");
                    G.nonstd_bed_channel_assignment_mask=AC4_bed_chan_assign_code_2_nonstd(bed_chan_assign_code);
                TESTELSE_SB_ELSE(                               "b_ch_assign_code");
                    TESTELSE_SB_SKIP(                           "b_nonstd_bed_channel_assignment");
                        Get_S3 (17, G.nonstd_bed_channel_assignment_mask, "nonstd_bed_channel_assignment_mask");
                    TESTELSE_SB_ELSE(                           "b_nonstd_bed_channel_assignment");
                        int16u std_bed_channel_assignment_mask;
                        Get_S2 (10, std_bed_channel_assignment_mask, "std_bed_channel_assignment_mask");
                        G.nonstd_bed_channel_assignment_mask=AC4_bed_channel_assignment_mask_2_nonstd(std_bed_channel_assignment_mask);
                    TESTELSE_SB_END();
                TESTELSE_SB_END();
                if (G.nonstd_bed_channel_assignment_mask!=(int32u)-1)
                {
                    if (G.b_lfe)
                        G.nonstd_bed_channel_assignment_mask|=LFE;
                    else
                        G.b_lfe=G.nonstd_bed_channel_assignment_mask&LFE;
                }
            TEST_SB_END();
        TESTELSE_SB_ELSE(                                       "b_bed_objects");
            TESTELSE_SB_SKIP(                                   "b_isf");
                TEST_SB_SKIP(                                   "b_isf_start");
                    Skip_S1(3,                                  "isf_config");
                TEST_SB_END();
            TESTELSE_SB_ELSE(                                   "b_isf");
                int8u res_bytes;
                Get_S1 (4, res_bytes,                           "res_bytes");
                if (res_bytes)
                    Skip_S8(res_bytes*8,                        "reserved_data");
            TESTELSE_SB_END();
       TESTELSE_SB_END();
    TESTELSE_SB_END();
    if (fs_index)
    {
        TEST_SB_SKIP(                                           "b_sf_multiplier");
            Skip_SB(                                            "sf_multiplier");
        TEST_SB_END();
    }
    TEST_SB_SKIP(                                               "b_bitrate_info");
        Skip_V4(3, 5, 1,                                        "bitrate_indicator");
    TEST_SB_END();
    vector<bool> b_iframes;
    for (int8u Pos=0; Pos<frame_rate_factor; Pos++)
    {
        bool b_iframe;
        Get_SB (b_iframe,                                       "b_audio_ndot");
        b_iframes.push_back(b_iframe);
    }
    if (b_substreams_present)
    {
        int8u substream_index;
        Get_S1 (2, substream_index,                             "substream_index");
        if (substream_index==3)
        {
            int32u substream_index32;
            Get_V4 (2, substream_index32,                       "substream_index");
            substream_index32+=3;
            substream_index=(int8u)substream_index32;
        }

        G.substream_index=substream_index;
        G.b_iframe=b_iframes[0]; //TODO frame_rate_factor

        Substream_Type[substream_index]=Type_Ac4_Substream;
   }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_presentation_substream_info(presentation& P)
{
    Element_Begin1(                                             "ac4_presentation_substream_info");
    Get_SB (P.b_alternative,                                    "b_alternative");
    Get_SB (P.b_pres_ndot,                                      "b_pres_ndot");
    Get_S1 (2, P.substream_index,                               "substream_index");
    if (P.substream_index==3)
    {
        int32u substream_index32;
        Get_V4 (2, substream_index32,                           "substream_index");
        substream_index32+=3;
        P.substream_index=(int8u)substream_index32;
    }
    Element_End0();

    Substream_Type[P.substream_index]=Type_Ac4_Presentation_Substream;
}

//---------------------------------------------------------------------------
void File_Ac4::presentation_config_ext_info(presentation& P)
{
    Element_Begin1(                                             "presentation_config_ext_info");
    int16u n_skip_bytes; // TODO: verify max size

    Get_S2 (5, n_skip_bytes,                                    "n_skip_bytes");
    TEST_SB_SKIP(                                               "b_more_skip_bytes");
        int32u n_skip_bytes32;
        Get_V4 (2, n_skip_bytes32,                              "n_skip_bytes");
        n_skip_bytes=(int8u)n_skip_bytes32<<5;
    TEST_SB_END();

    if (bitstream_version==1 && P.presentation_config==7)
    {
        size_t Pos_Before=Data_BS_Remain();
        ac4_presentation_v1_info(P);
        size_t Pos_After=Data_BS_Remain();
        size_t n_bits_read=Pos_After-Pos_Before;
        if (n_bits_read%8)
        {
            int8u n_skip_bits=8-(n_bits_read%8);
            Skip_S8(n_skip_bits,                                "reserved");
            n_bits_read+=n_skip_bits;
        }
        n_skip_bytes-=(n_bits_read/8);
    }

    Skip_S8(n_skip_bytes*8,                                     "reserved");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::bed_dyn_obj_assignment(group_substream& G, int8u n_signals)
{
    Element_Begin1(                                             "bed_dyn_obj_assignment");
        TESTELSE_SB_SKIP(                                       "b_dyn_objects_only");
        TESTELSE_SB_ELSE(                                       "b_dyn_objects_only");
            TESTELSE_SB_SKIP(                                   "b_isf");
                Skip_S1(3,                                      "isf_config");
            TESTELSE_SB_ELSE(                                   "b_isf");
                TESTELSE_SB_SKIP(                               "b_ch_assign_code");
                    int8u bed_chan_assign_code;
                    Get_S1 (3, bed_chan_assign_code,            "bed_chan_assign_code");
                    G.nonstd_bed_channel_assignment_mask=AC4_bed_chan_assign_code_2_nonstd(bed_chan_assign_code);
                TESTELSE_SB_ELSE(                               "b_ch_assign_code");
                    TESTELSE_SB_SKIP(                           "b_chan_assign_mask");
                        TESTELSE_SB_SKIP(                       "b_nonstd_bed_channel_assignment");
                            Get_S3 (17, G.nonstd_bed_channel_assignment_mask, "nonstd_bed_channel_assignment_mask");
                        TESTELSE_SB_ELSE(                       "b_nonstd_bed_channel_assignment");
                            int16u std_bed_channel_assignment_mask;
                            Get_S2 (10, std_bed_channel_assignment_mask, "std_bed_channel_assignment_mask");
                            G.nonstd_bed_channel_assignment_mask=AC4_bed_channel_assignment_mask_2_nonstd(std_bed_channel_assignment_mask);
                        TESTELSE_SB_END();
                    TESTELSE_SB_ELSE(                           "b_chan_assign_mask");
                        int8u n_bed_signals;
                        if (n_signals>1)
                        {
                            int8u bed_ch_bits=ceil(log((float)n_signals)/log(2.0)); // ceil(log2(n_signals));
                            Get_S1 (bed_ch_bits, n_bed_signals, "n_bed_signals_minus1");
                            n_bed_signals++;
                        }
                        else
                        {
                            n_bed_signals=1;
                        }
                        G.nonstd_bed_channel_assignment_mask=0;
                        for (int8u Pos=0; Pos<n_bed_signals; Pos++)
                        {
                            int8u nonstd_bed_channel_assignment;
                            Get_S1 (4, nonstd_bed_channel_assignment, "nonstd_bed_channel_assignment"); Param_Info1(AC4_nonstd_bed_channel_assignment_mask_ChannelLayout_List[nonstd_bed_channel_assignment]);
                            G.nonstd_bed_channel_assignment_mask|=1<<nonstd_bed_channel_assignment;
                        }
                    TESTELSE_SB_END();
                TESTELSE_SB_END();
                if (G.nonstd_bed_channel_assignment_mask!=(int32u)-1)
                {
                    if (G.b_lfe)
                        G.nonstd_bed_channel_assignment_mask|=LFE;
                    else
                        G.b_lfe=G.nonstd_bed_channel_assignment_mask&LFE;
                }
            TESTELSE_SB_END();
        TESTELSE_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::content_type(content_info& ContentInfo)
{
    Element_Begin1(                                             "content_type");
        int8u content_classifier;
        Get_S1 (3, content_classifier,                          "content_classifier"); Param_Info1(Value(Ac4_content_classifier, content_classifier));
        TEST_SB_SKIP(                                           "b_language_indicator");
            TESTELSE_SB_SKIP(                                   "b_serialized_language_tag");
                Skip_SB(                                        "b_start_tag");
                Skip_S2(16,                                     "language_tag_chunk");
            TESTELSE_SB_ELSE(                                   "b_serialized_language_tag");
                int8u n_language_tag_bytes;
                Get_S1 (6, n_language_tag_bytes,                "n_language_tag_bytes");
                ContentInfo.language_tag_bytes.clear();
                for (int8u Pos=0; Pos<n_language_tag_bytes; Pos++)
                {
                    int8u language_tag_bytes;
                    Get_S1 (8, language_tag_bytes,              "language_tag_bytes");
                    ContentInfo.language_tag_bytes+=(language_tag_bytes<0x80?language_tag_bytes:'?');
                }
            TESTELSE_SB_END();
        TEST_SB_END();

        ContentInfo.content_classifier=content_classifier;
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::frame_rate_multiply_info()
{
    frame_rate_factor=1;
    Element_Begin1(                                             "frame_rate_multiply_info");
    switch (frame_rate_index)
    {
        case 0:
        case 1:
        case 7:
        case 8:
        case 9:
                TEST_SB_SKIP(                                   "b_multiplier");
                    frame_rate_factor=2;
                TEST_SB_END();
        break;
        case 2:
        case 3:
        case 4:
                TEST_SB_SKIP(                                   "b_multiplier");
                    TESTELSE_SB_SKIP(                           "multiplier_bit");
                        frame_rate_factor=4;
                    TESTELSE_SB_ELSE(                           "multiplier_bit");
                        frame_rate_factor=2;
                    TESTELSE_SB_END();
                TEST_SB_END();
        break;
        default:
        break;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::frame_rate_fractions_info(presentation& P)
{
    Element_Begin1(                                             "frame_rate_fractions_info");
    switch(frame_rate_index)
    {
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
                if (frame_rate_factor==1)
                {
                    bool b_frame_rate_fraction;
                    Get_SB (b_frame_rate_fraction,              "b_frame_rate_fraction");
                    if (b_frame_rate_fraction)
                        P.frame_rate_fraction_minus1++;
                }
                break;
        case 10:
        case 11:
        case 12:
                {
                bool b_frame_rate_fraction;
                Get_SB (b_frame_rate_fraction,                  "b_frame_rate_fraction");
                if (b_frame_rate_fraction)
                {
                    P.frame_rate_fraction_minus1++;
                    bool b_frame_rate_fraction_is_4;
                    Get_SB (b_frame_rate_fraction_is_4,         "b_frame_rate_fraction_is_4");
                    if (b_frame_rate_fraction_is_4)
                        P.frame_rate_fraction_minus1+=2;
                }
                }
                break;
        default:;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::emdf_info(presentation_substream& P)
{
    int8u emdf_version, key_id;
    Element_Begin1(                                             "emdf_info");
    Get_S1 (2, emdf_version,                                    "emdf_version");
    if (emdf_version==3)
    Skip_V4(2,                                                  "emdf_version");

    Get_S1 (3, key_id,                                          "key_id");
    if (key_id==7)
        Skip_V4(3,                                              "key_id");

    TEST_SB_SKIP(                                               "b_emdf_payloads_substream_info");
        emdf_payloads_substream_info(P);
    TEST_SB_END();
    emdf_protection();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::emdf_payloads_substream_info(presentation_substream& P)
{
    Element_Begin1(                                             "emdf_payloads_substream_info");
    int8u substream_index;
    Get_S1 (2, substream_index,                                 "substream_index");
    if (substream_index==3)
    {
        int32u substream_index32;
        Get_V4 (2, substream_index32,                           "substream_index");
        substream_index32+=3;
        substream_index=(int8u)substream_index32;
    }

    Substream_Type[substream_index]=Type_Emdf_Payloads_Substream;

    P.substream_type=Type_Emdf_Payloads_Substream;
    P.substream_index=substream_index;
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::emdf_protection()
{
    int8u protection_length_primary, protection_length_secondary;
    Element_Begin1(                                             "emdf_protection");
    Get_S1 (2, protection_length_primary,                       "protection_length_primary");
    Get_S1 (2, protection_length_secondary,                     "protection_length_secondary");
    switch (protection_length_primary)
    {
        case 1:
                Skip_BS(8,                                      "protection_bits_primary");Param_Info1("(8 bits)");
                break;
        case 2:
                Skip_BS(32,                                     "protection_bits_primary");Param_Info1("(32 bits)");
                break;
        case 3:
                Skip_BS(128,                                     "protection_bits_primary");Param_Info1("(128 bits)");
                break;
        default:;
    }

    switch (protection_length_secondary)
    {
        case 1:
                Skip_BS(8,                                      "protection_bits_secondary");Param_Info1("(8 bits)");
                break;
        case 2:
                Skip_BS(32,                                     "protection_bits_secondary");Param_Info1("(32 bits)");
                break;
        case 3:
                Skip_BS(128,                                    "protection_bits_secondary");Param_Info1("(128 bits)");
                break;
        default:;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::substream_index_table()
{
    Element_Begin1(                                             "substream_index_table");
    Get_S1(2, n_substreams,                                     "n_substreams");
    if (n_substreams==0)
    {
        int32u n_substreams32;
        Get_V4 (2, n_substreams32,                              "n_substreams");
        n_substreams32+=4;
        n_substreams=(int8u)n_substreams32;
    }

    bool b_size_present;
    if (n_substreams==1)
        Get_SB(b_size_present,                                  "b_size_present");
    else
        b_size_present=1;

    if (b_size_present)
    {
        for (int8u Pos=0; Pos<n_substreams; Pos++)
        {
            int16u substream_size;
            bool b_more_bits;
            Get_SB (b_more_bits,                                "b_more_bits");
            Get_S2 (10, substream_size,                         "substream_size");
            if (b_more_bits)
            {
                int32u substream_size32;
                Get_V4 (2, substream_size32,                    "substream_size");
                substream_size+=(int16u)substream_size32<<10;
                Param_Info1(substream_size);
            }
            Substream_Size.push_back(substream_size);
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::oamd_substream_info(group_substream& G, bool b_substreams_present)
{
    Element_Begin1(                                             "oamd_substream_info");
    Skip_SB(                                                    "b_oamd_ndot");
    if (b_substreams_present)
    {
        int8u substream_index;
        Get_S1 (2, substream_index,                             "substream_index");
        if (substream_index==3)
        {
            int32u substream_index32;
            Get_V4 (2, substream_index32,                       "substream_index");
            substream_index32+=3;
            substream_index=(int8u)substream_index32;
        }
        
        G.substream_type=Type_Oamd_Substream;
        G.substream_index=substream_index;
        G.ch_mode=(int8u)-1;
        
        Substream_Type[substream_index]=Type_Oamd_Substream;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::oamd_common_data()
{
    int8u add_data_bytes;
    int16u bits_used;

    Element_Begin1(                                             "oamd_common_data");
    TESTELSE_SB_SKIP(                                           "b_default_screen_size_ratio");
    TESTELSE_SB_ELSE(                                           "b_default_screen_size_ratio");
        Skip_S1(5,                                              "master_screen_size_ratio_code");
    TESTELSE_SB_END();

    Skip_SB(                                                    "b_bed_object_chan_distribute");

    TEST_SB_SKIP(                                               "b_additional_data");
        Get_S1 (1, add_data_bytes,                              "add_data_bytes_minus1");
        add_data_bytes++;

        if (add_data_bytes==2)
        {
            int32u add_data_bytes32;
            Get_V4 (2, add_data_bytes32,                        "add_data_bytes");
            add_data_bytes+=(int8u)add_data_bytes32;
        }
        //TODO: bits_used=trim();
        //TODO: bits_used+=bed_render_info();
        Skip_S8(add_data_bytes*8,                               "add_data");
    TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_substream(size_t substream_index)
{
    Element_Begin1(substream_type_Trace[Type_Ac4_Substream]);
    Element_Info1(Ztring::ToZtring(substream_index));

    //Looks for corresponding group info (we take the last one found)
    size_t Group_Pos=(size_t)-1;
    size_t SubStream_Pos;
    for (size_t i=0; i <Groups.size(); i++)
    {
        for (size_t j=0; j<Groups[i].Substreams.size(); j++)
            if (Groups[i].Substreams[j].substream_index==substream_index)
            {
                Group_Pos=i;
                SubStream_Pos=j;
            }
    }
    if (Group_Pos==(size_t)-1)
    {
        Element_End0();
        return; //Problem
    }
    group_substream& GroupInfo=Groups[Group_Pos].Substreams[SubStream_Pos]; //TODO should be const

    std::map<int8u, audio_substream>::iterator AudioSubstream_It=AudioSubstreams.find(substream_index);
    if (AudioSubstream_It==AudioSubstreams.end())
        AudioSubstream_It=AudioSubstreams.insert(std::pair<int8u, audio_substream>((int8u)substream_index, audio_substream(GroupInfo.b_iframe))).first;
    audio_substream& AudioSubstream=AudioSubstream_It->second;

    //Looks for efficient high frame rate (data span over several audio frames)
    for (int8u p=0; p<Presentations.size(); p++)
    {
        presentation& P=Presentations[p];
        if (P.frame_rate_fraction_minus1)
        {
            int8u i;
            for (i=0; i<P.substream_group_info_specifiers.size(); i++)
            {
                if (P.substream_group_info_specifiers[i]==Group_Pos)
                {
                    if (!AudioSubstream.Buffer.Data)
                        AudioSubstream.Buffer.Create((size_t)Element_Size);//*(P.frame_rate_fraction_minus1+1)*2);
                    AudioSubstream.Buffer.Append(Buffer+Buffer_Offset+(size_t)Element_Offset, (size_t)Element_Size-Element_Offset);
                    if (AudioSubstream.Buffer_Index<P.frame_rate_fraction_minus1)
                    {
                        AudioSubstream.Buffer_Index++;
                        Skip_XX(Element_Size-Element_Offset,    "Data (buffered)");
                        Element_End0();
                        return;
                    }
                    break;
                }
            }
            if (i<P.substream_group_info_specifiers.size()) // If found
                break;
        }
    }
    AudioSubstream.Buffer_Index=0;

    int64u Save_Element_Size;
    if (AudioSubstream.Buffer.Data)
    {
        //Using AudioSubstream.Buffer as temporary save
        int8u* Buffer_Temp=(int8u*)Buffer;
        swap(Buffer_Temp, AudioSubstream.Buffer.Data);
        Buffer=Buffer_Temp;
        swap(Buffer_Offset, AudioSubstream.Buffer.Offset);
        swap(Buffer_Size, AudioSubstream.Buffer.Size);

        Save_Element_Size=Element_Size;
        Element_Offset=0;
        Element_Size=Buffer_Offset;
        Buffer_Size=Buffer_Offset;
        Buffer_Offset=0;
    }

    BS_Begin();
    size_t Pos_Before=Data_BS_Remain();
    int32u audio_size;
    Get_S4 (15, audio_size,                                     "audio_size_value");
    TEST_SB_SKIP(                                               "b_more_bits");
        int32u audio_size32;
        Get_V4 (7, audio_size32,                                "audio_size_value");
        audio_size+=audio_size32<<15;
    TEST_SB_END();

    // Skip audio
    const char* audio_data_name;
    int8u ch_mode_Save=GroupInfo.ch_mode;
    if (Groups[Group_Pos].b_channel_coded)
    {
        audio_data_name="audio_data_chan";
    }
    else if (GroupInfo.b_ajoc)
    {
        audio_data_name="audio_data_ajoc";
    }
    else
    {
        audio_data_name="audio_data_objs";
        GroupInfo.ch_mode=objs_to_channel_mode(GroupInfo.n_objects_code);
        if (GroupInfo.ch_mode==3 && GroupInfo.b_lfe)
            GroupInfo.ch_mode++; // ch_mode=4, 5.1
    }
    Skip_BS(audio_size*8,                                       audio_data_name);
    metadata(AudioSubstream, substream_index);
    GroupInfo.ch_mode=ch_mode_Save;

    size_t Pos_After=Data_BS_Remain();
    if (Pos_Before-Pos_After<(Substream_Size[substream_index]*8))
    {
        size_t Bits=(Substream_Size[substream_index]*8)-(Pos_Before-Pos_After);
        bool Align=false;
        if (Bits<8)
        {
            int8u Value;
            Peek_S1((int8u)Bits, Value);
            if (!Value)
                Align=true;
        }
        Skip_BS(Bits, Align?"byte_align":"?");
    }
    BS_End();

    if (AudioSubstream.Buffer.Data)
    {
        Element_Offset=Element_Size=Save_Element_Size;
        int8u* Buffer_Temp=(int8u*)Buffer;
        swap(Buffer_Temp, AudioSubstream.Buffer.Data);
        Buffer=Buffer_Temp;
        Buffer_Offset=AudioSubstream.Buffer.Offset;
        Buffer_Size=AudioSubstream.Buffer.Size;
        AudioSubstream.Buffer.Clear();
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_presentation_substream(size_t substream_index, size_t Substream_Index)
{
    presentation& P=Presentations[Substream_Index];
    loudness_info& L=P.LoudnessInfo;

    int8u name_length=32, n_targets, add_data_bytes;

    // b_pres_has_lfe
    bool b_pres_has_lfe;
    if (P.pres_ch_mode!=(int8u)-1)
        b_pres_has_lfe=Channel_Mode_Contains_Lfe(P.pres_ch_mode);
    else if (P.pres_ch_mode_core==4 || P.pres_ch_mode_core==6)
        b_pres_has_lfe=true;
    else
        b_pres_has_lfe=false;

    Element_Begin1(substream_type_Trace[Type_Ac4_Presentation_Substream]);
    Element_Info1(Ztring::ToZtring(substream_index));
    BS_Begin();
    if (P.b_alternative)
    {
        TEST_SB_SKIP(                                           "b_name_present");
            TEST_SB_SKIP(                                       "b_length");
                Get_S1(5, name_length,                          "name_len");
            TEST_SB_END();
        TEST_SB_END();
        Skip_BS(name_length*8,                                  "presentation_name");

        Get_S1 (2, n_targets,                                   "n_targets_minus1");
        n_targets++;
        if (n_targets==4)
        {
            int32u n_targets32;
            Get_V4 (2, n_targets32,                             "n_targets");
            n_targets+=(int8u)n_targets32;
        }

        for (int8u Pos=0; Pos<n_targets; Pos++)
        {
            Skip_S1(3,                                          "target_level");
            Skip_S1(4,                                          "target_device_category[]");

            TEST_SB_SKIP(                                       "b_tdc_extension");
                Skip_S1(4,                                      "reserved_bits");
            TEST_SB_END();

            TEST_SB_SKIP(                                       "b_ducking_depth_present");
                Skip_S1(6,                                      "max_ducking_depth");
            TEST_SB_END();

            TEST_SB_SKIP(                                       "b_loud_corr_target");
                Skip_S1(5,                                      "loud_corr_target");
            TEST_SB_END();

            for (int8u Pos2=0; Pos2<P.n_substreams_in_presentation; Pos2++)
            {
                TEST_SB_SKIP(                                    "b_active");
                    TEST_SB_SKIP(                                "alt_data_set_index");
                        Skip_V4(2,                               "alt_data_set_index");
                    TEST_SB_END();
                TEST_SB_END();
            }
        }
    }

    TEST_SB_SKIP(                                               "b_additional_data");
        Get_S1 (4, add_data_bytes,                              "add_data_bytes_minus1");
        add_data_bytes++;
        if (add_data_bytes==16)
        {
            int32u add_data_bytes32;
            Get_V4 (2, add_data_bytes32,                        "add_data_bytes32");
            add_data_bytes+=(int8u)add_data_bytes32;
        }
        size_t byte_align=Data_BS_Remain()%8;
        if (byte_align)
            Skip_S1(byte_align,                                 "byte_align");
        Get_SB (P.dolby_atmos_indicator,                        "dolby_atmos_indicator");
        Skip_BS(add_data_bytes*8-1,                             "add_data");
    TEST_SB_END();

    Get_S1 (7, L.dialnorm_bits,                                 "dialnorm_bits");
    TEST_SB_SKIP(                                               "b_further_loudness_info");
        further_loudness_info(P.LoudnessInfo, true, true);
    TEST_SB_END();

    int16u drc_metadata_size_value;
    Get_S2 (5, drc_metadata_size_value,                          "drc_metadata_size_value");
    TEST_SB_SKIP(                                                "b_more_bits");
        int32u drc_metadata_size_value32;
        Get_V4 (3, drc_metadata_size_value32,                    "drc_metadata_size_value");
        drc_metadata_size_value+=(int16u)drc_metadata_size_value32<<5;
    TEST_SB_END();

    size_t Pos_Before=Data_BS_Remain();
    drc_frame(P.DrcInfo, P.b_pres_ndot);
    size_t Pos_After=Data_BS_Remain();
    if (drc_metadata_size_value!=Pos_Before-Pos_After)
    {
        Fill(Stream_Audio, 0, "NOK", "drc_metadata", -1, true, true);//TODO remove
        Element_Info1("Problem");
    }

    if (P.n_substream_groups>1)
    {
        TEST_SB_SKIP(                                           "b_substream_group_gains_present");
            TESTELSE_SB_SKIP(                                   "b_keep");
            TESTELSE_SB_ELSE(                                   "b_keep");
                for (int8u Pos=0; Pos<P.n_substream_groups; Pos++)
                    Skip_S1(6,                                  "sg_gain[sg]");
            TESTELSE_SB_END();
        TEST_SB_END();
    }

    TEST_SB_SKIP(                                               "b_associated");
        TEST_SB_SKIP(                                           "b_scale_main");
            Skip_S1(8,                                          "scale_main");
        TEST_SB_END();

        TEST_SB_SKIP(                                           "b_scale_main_centre");
            Skip_S1(8,                                          "scale_main_centre");
        TEST_SB_END();

        TEST_SB_SKIP(                                           "b_scale_main_front");
            Skip_S1(8,                                          "scale_main_front");
        TEST_SB_END();

        TEST_SB_SKIP(                                           "b_associate_is_mono");
            Skip_S1(8,                                          "pan_associated");
        TEST_SB_END();
    TEST_SB_END();

    custom_dmx_data(P.Dmx, P.pres_ch_mode, P.pres_ch_mode_core, P.b_pres_4_back_channels_present, P.pres_top_channel_pairs, b_pres_has_lfe);
    if (P.pres_ch_mode==(int8u)-1 || P.pres_ch_mode<=4 || Data_BS_Remain()>=4) //TODO: remove this when parsing issue is understood
    {
    loud_corr(P.pres_ch_mode, P.pres_ch_mode_core, false/* TODO: b_objects? */);
    }
    else
    {
        Skip_BS(Data_BS_Remain(), "Problem");
        Fill(Stream_Audio, 0, "NOK", "presentation_substream", -1, true, true);//TODO remove
    }
    size_t byte_align=Data_BS_Remain()%8;
    if (!byte_align && Data_BS_Remain()==8)
        byte_align = Data_BS_Remain(); //TODO: found in 1 stream but spec says 0..7
    if (byte_align)
        Skip_S1(byte_align,                                     "byte_align");
    BS_End();
    Element_End0();
}
//---------------------------------------------------------------------------
void File_Ac4::metadata(audio_substream& AudioSubstream, size_t Substream_Index)
{
    //Looks for corresponding group info (we take the last one found)
    size_t Group_Pos=(size_t)-1;
    size_t SubStream_Pos;
    for (size_t i=0; i <Groups.size(); i++)
    {
        for (size_t j=0; j<Groups[i].Substreams.size(); j++)
            if (Groups[i].Substreams[j].substream_index==Substream_Index)
            {
                Group_Pos=i;
                SubStream_Pos=j;
            }
    }
    if (Group_Pos==(size_t)-1)
        return; //Problem
    const content_info& ContentInfo=Groups[Group_Pos].ContentInfo;
    const group_substream& GroupInfo=Groups[Group_Pos].Substreams[SubStream_Pos];

    bool b_associated=ContentInfo.content_classifier!=(int8u)-1 && ContentInfo.content_classifier>1; //TODO: from presentation_config if content_classifier not present
    AudioSubstream.b_dialog=ContentInfo.content_classifier==4; //TODO: from presentation_config if content_classifier not present

    Element_Begin1("metadata");
    basic_metadata(AudioSubstream.LoudnessInfo, AudioSubstream.Preprocessing, GroupInfo.ch_mode, GroupInfo.sus_ver);
    extended_metadata(AudioSubstream, b_associated, GroupInfo.ch_mode, GroupInfo.sus_ver);

    // TODO:
    // if (b_alternative && !b_ajoc)
    //     oamd_dyndata_single(n_objs, num_obj_info_blocks, b_iframe, b_alternative, obj_type[n_objs], b_lfe[n_objs]);

    int8u tools_metadata_size8;
    Get_S1 (7, tools_metadata_size8,                            "tools_metadata_size");
    int32u tools_metadata_size=tools_metadata_size8;
    TEST_SB_SKIP(                                               "b_more_bits");
        int32u tools_metadata_size32;
        Get_V4 (3, tools_metadata_size32,                       "tools_metadata_size");
        tools_metadata_size+=tools_metadata_size32<<7;
    TEST_SB_END();

    if (false)
    {
        Skip_BS(tools_metadata_size,                            "tools_metadata");
    }
    else
    {
    size_t Pos_Before = Data_BS_Remain();
    if (!GroupInfo.sus_ver)
        drc_frame(AudioSubstream.DrcInfo, AudioSubstream.b_iframe);
    dialog_enhancement(AudioSubstream.DeInfo, GroupInfo.ch_mode, AudioSubstream.b_iframe);
    size_t Pos_After=Data_BS_Remain();
    if (tools_metadata_size!=Pos_Before-Pos_After)
    {
        Fill(Stream_Audio, 0, "NOK", "tools_metadata", -1, true, true);//TODO remove
        Element_Info1("Problem");
        if (tools_metadata_size>Pos_Before-Pos_After)
            Skip_BS(tools_metadata_size-(Pos_Before-Pos_After), "?");
    }
    }

    TEST_SB_SKIP("b_emdf_payloads_substream");
        for (;;)
        {
            Element_Begin1("umd_payload");
            int32u umd_payload_id,  umd_payload_size;
            int8u extSizeBits;
            bool b_smpoffst, b_discard_unknown_payload;
            Get_S4 (5, umd_payload_id,                          "umd_payload_id");
            if (!umd_payload_id)
            {
                Element_End0();
                break;
            }
            if (umd_payload_id==31)
            {
                Get_V4 (5, umd_payload_id,                      "umd_payload_id");
                umd_payload_id+=31;
            }
            Element_Begin1("umd_payload_config");
            TEST_SB_GET(b_smpoffst,                             "b_smpoffst");
                Skip_V4(11,                                     "smpoffst");
            TEST_SB_END();
            TEST_SB_SKIP(                                       "b_duration");
                Skip_V4(11,                                     "duration");
            TEST_SB_END();
            TEST_SB_SKIP(                                       "b_groupid");
                Skip_V4(2,                                      "groupid");
            TEST_SB_END();
            TEST_SB_SKIP(                                       "b_codecdata");
                Skip_V4(8,                                      "b_codecdata");
            TEST_SB_END();
            Get_SB (b_discard_unknown_payload,                  "b_discard_unknown_payload");
            if (!b_discard_unknown_payload)
            {
                bool b_payload_frame_aligned;
                if (!b_smpoffst)
                {
                    TEST_SB_GET(b_payload_frame_aligned,        "b_payload_frame_aligned");
                        Skip_SB(                                "b_create_duplicate");
                        Skip_SB(                                "b_remove_duplicate");
                    TEST_SB_END();
                }
                if (b_smpoffst || b_payload_frame_aligned)
                {
                    Skip_S1(5,                                  "priority");
                    Skip_S1(2,                                  "proc_allowed");
                }
            }
            Element_End0();
            Get_V4 (8, umd_payload_size,                        "umd_payload_size");

            switch (umd_payload_id)
            {
                default:
                    if (umd_payload_size)
                        Skip_BS(umd_payload_size*8,             "(Unknown)");
            }
            Element_End0();
        }
    TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::basic_metadata(loudness_info& L, preprocessing& P, int8u ch_mode, bool sus_ver)
{
    Element_Begin1("basic_metadata");
    if (!sus_ver)
        Get_S1 (7, L.dialnorm_bits,                             "dialnorm_bits");

    TEST_SB_SKIP(                                               "b_more_basic_metadata");
        if (!sus_ver)
        {
            TEST_SB_SKIP(                                       "b_further_loudness_info");
                further_loudness_info(L, sus_ver, false);
            TEST_SB_END();
        }
        else
        {
            TEST_SB_SKIP(                                       "b_substream_loudness_info");
                Skip_S1(8,                                      "substream_loudness_bits");
                TEST_SB_SKIP(                                   "b_further_substream_loudness_info");
                    further_loudness_info(L, sus_ver, false);
                TEST_SB_END();
            TEST_SB_END();
        }

        if (ch_mode==1) // stereo
        {
            TEST_SB_SKIP(                                       "b_prev_dmx_info");
                Get_S1 (3, P.pre_dmixtyp_2ch,                   "pre_dmixtyp_2ch");
                Get_S1 (2, P.phase90_info_2ch,                  "phase90_info_2ch");
            TEST_SB_END();
        }
        else if (ch_mode!=(int8u)-1 && ch_mode>1)
        {
            if (!sus_ver)
            {
                TEST_SB_SKIP(                                   "b_stereo_dmx_coeff");
                    Skip_S1(3,                                  "loro_centre_mixgain");
                    Skip_S1(3,                                  "loro_surround_mixgain");

                    TEST_SB_SKIP(                               "b_loro_dmx_loud_corr");
                        Skip_S1(5,                              "loro_dmx_loud_corr");
                    TEST_SB_END();

                    TEST_SB_SKIP(                               "b_ltrt_mixinfo");
                        Skip_S1(3,                              "ltrt_centre_mixgain");
                        Skip_S1(3,                              "ltrt_surround_mixgain");
                    TEST_SB_END();

                    TEST_SB_SKIP(                               "b_ltrt_dmx_loud_corr");
                        Skip_S1(5,                              "ltrt_dmx_loud_corr");
                    TEST_SB_END();

                    if (Channel_Mode_Contains_Lfe(ch_mode))
                    {
                        TEST_SB_SKIP(                           "b_lfe_mixinfo");
                            Skip_S1(5,                          "lfe_mixgain");
                        TEST_SB_END();
                    }

                    Skip_S1(2,                                  "preferred_dmx_method");
                TEST_SB_END();
            }

            if (ch_mode==3 || ch_mode==4) // 5.x
            {
                TEST_SB_SKIP(                                   "b_predmixtyp_5ch");
                    Get_S1 (3, P.pre_dmixtyp_5ch,               "pre_dmixtyp_5ch");
                TEST_SB_END();

                TEST_SB_SKIP(                                   "b_preupmixtyp_5ch");
                    Skip_S1(4,                                  "pre_upmixtyp_5ch");
                TEST_SB_END();
            }

            if (ch_mode>=5 && ch_mode<=10)
            {
                TEST_SB_SKIP(                                   "b_upmixtyp_7ch");
                    if (ch_mode==5 || ch_mode==6)
                        Skip_S1(2,                              "pre_upmixtyp_3_4");
                    else if (ch_mode==9 || ch_mode==10)
                        Skip_SB(                                "pre_upmixtyp_3_2_2");
                    
                TEST_SB_END();
            }
            Get_S1 (2, P.phase90_info_mc,                       "phase90_info_mc");
            Get_SB (   P.b_surround_attenuation_known,          "b_surround_attenuation_known");
            Get_SB (   P.b_lfe_attenuation_known,               "b_lfe_attenuation_known");
        }

        TEST_SB_SKIP(                                           "b_dc_blocking");
            Skip_SB(                                            "dc_block_on");
        TEST_SB_END();
    TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::extended_metadata(audio_substream& AudioSubstream, bool b_associated, int8u ch_mode, bool sus_ver)
{
    Element_Begin1("extended_metadata");
    if (sus_ver)
    {
        Get_SB (AudioSubstream.b_dialog,                        "b_dialog");
    }
    else if (b_associated)
    {
        TEST_SB_SKIP(                                           "b_scale_main");
            Skip_S1(8,                                          "scale_main");
        TEST_SB_END();

        TEST_SB_SKIP(                                           "b_scale_main_centre");
            Skip_S1(8,                                          "scale_main_centre");
        TEST_SB_END();

        TEST_SB_SKIP(                                           "b_scale_main_front");
            Skip_S1(8,                                          "scale_main_front");
        TEST_SB_END();

        if (ch_mode==0)
            Skip_S1(8,                                          "pan_associated");
    }

    if (AudioSubstream.b_dialog)
    {
        TEST_SB_SKIP(                                           "b_dialog_max_gain");
            Get_S1 (2, AudioSubstream.dialog_max_gain,          "dialog_max_gain");
        TEST_SB_END();

        TEST_SB_SKIP(                                           "b_pan_dialog_present");
            if (ch_mode==0)
            {
                Skip_S1(8,                                      "pan_dialog");
            }
            else
            {
                Skip_S1(8,                                      "pan_dialog[0]");
                Skip_S1(8,                                      "pan_dialog[1]");
                Skip_S1(2,                                      "pan_signal_selector");
            }
        TEST_SB_END();
    }

    TEST_SB_SKIP(                                               "b_channels_classifier");
        if (Channel_Mode_Contains_C(ch_mode))
        {
            TEST_SB_SKIP(                                       "b_c_active");
                Skip_SB(                                        "b_c_has_dialog");
            TEST_SB_END();
        }
        if (Channel_Mode_Contains_Lr(ch_mode))
        {
            TEST_SB_SKIP(                                       "b_l_active");
                Skip_SB(                                        "b_l_has_dialog");
            TEST_SB_END();

            TEST_SB_SKIP(                                       "b_r_active");
                Skip_SB(                                        "b_r_has_dialog");
            TEST_SB_END();
        }
        if (Channel_Mode_Contains_LsRs(ch_mode))
        {
            Skip_SB(                                            "b_ls_active");
            Skip_SB(                                            "b_rs_active");
        }
        if (Channel_Mode_Contains_LrsRrs(ch_mode))
        {
            Skip_SB(                                            "b_lrs_active");
            Skip_SB(                                            "b_rrs_active");
        }
        if (Channel_Mode_Contains_LwRw(ch_mode))
        {
            Skip_SB(                                            "b_lw_active");
            Skip_SB(                                            "b_rw_active");
        }
        if (Channel_Mode_Contains_VhlVhr(ch_mode))
        {
            Skip_SB(                                            "b_vhl_active");
            Skip_SB(                                            "b_vhr_active");
        }
        if (Channel_Mode_Contains_Lfe(ch_mode))
        {
            Skip_SB(                                            "b_lfe_active");
        }
    TEST_SB_END();

    TEST_SB_SKIP(                                               "b_event_probability");
        Skip_S1(4,                                              "event_probability");
    TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::dialog_enhancement(de_info& Info, int8u ch_mode, bool b_iframe)
{
    Element_Begin1("dialog_enhancement");
    TEST_SB_GET (Info.b_de_data_present,                        "b_de_data_present");
        bool b_de_config_flag;
        if (!b_iframe)
            Get_SB (b_de_config_flag,                           "b_de_config_flag");
        else
            b_de_config_flag=b_iframe;
        if (b_de_config_flag)
            dialog_enhancement_config(Info);

        dialog_enhancement_data(Info, b_iframe, 0);
        if (ch_mode==13 || ch_mode==14)
        {
            TEST_SB_SKIP(                                       "b_de_simulcast");
                dialog_enhancement_data(Info, b_iframe, 1);
            TEST_SB_END();
        }
    TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::dialog_enhancement_config(de_info& Info)
{
    Element_Begin1("de_config");
    Get_S1 (2, Info.Config.de_method,                           "de_method");
    Get_S1 (2, Info.Config.de_max_gain,                         "de_max_gain");
    Get_S1 (3, Info.Config.de_channel_config,                   "de_channel_config");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::dialog_enhancement_data(de_info& Info, bool b_iframe, bool b_de_simulcast)
{
    bool de_keep_pos_flag=false, de_keep_data_flag=false, de_ms_proc_flag=false;
    ac4_huffman abs_table=Info.Config.de_method%2==0?de_hcb_abs_0:de_hcb_abs_1;
    ac4_huffman diff_table=Info.Config.de_method%2==0?de_hcb_diff_0:de_hcb_diff_1;

    int8u de_nr_channels=0, de_nr_bands=8;
    switch (Info.Config.de_channel_config)
    {
        case 0x1: // 0b001
        case 0x2: // 0b010
        case 0x4: // 0b100
            de_nr_channels=1; break;
        case 0x3: // 0b011
        case 0x5: // 0b101
        case 0x6: // 0b110
            de_nr_channels=2; break;
        case 0x7: // 0b111
            de_nr_channels=3; break;
    }

    Element_Begin1("de_data");
    if (de_nr_channels>0)
    {
        if (de_nr_channels>1 && (Info.Config.de_method==1 || Info.Config.de_method==3) && !b_de_simulcast)
        {
            if (!b_iframe)
                Get_SB (de_keep_pos_flag,                       "de_keep_pos_flag");

            if (!de_keep_pos_flag)
            {
                Skip_S1(5,                                      "de_mix_coef1_idx");
                if (de_nr_channels==3)
                    Skip_S1(5,                                  "de_mix_coef2_idx");
            }
        }

        if (!b_iframe)
            Get_SB (de_keep_data_flag,                          "de_keep_data_flag");

        if (!de_keep_data_flag)
        {

            if (de_nr_channels==2 && (Info.Config.de_method==0 || Info.Config.de_method==2))
                Skip_SB(                                        "de_ms_proc_flag");

            for (int8u ch=0; ch<de_nr_channels-de_ms_proc_flag; ch++)
            {
                if (b_iframe && ch==0)
                {
                    Huffman_Decode(abs_table, "de_par_code");
                    for (int8u band=1; band<de_nr_bands; band++)
                        Huffman_Decode(diff_table, "de_par_code");
                }
                else
                {
                    for (int8u band=0; band<de_nr_bands; band++)
                        Huffman_Decode(diff_table, "de_par_code");
                }
            }

            if (Info.Config.de_method>=2)
                Skip_S1(5,                                      "de_signal_contribution");
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::custom_dmx_data(dmx& D, int8u pres_ch_mode, int8u pres_ch_mode_core, bool b_pres_4_back_channels_present, int8u pres_top_channel_pairs, bool b_pres_has_lfe)
{
    int8u bs_ch_config=(int8u)-1, n_cdmx_configs;
    if (pres_ch_mode>=11 && pres_ch_mode<=14 && pres_top_channel_pairs)
    {
        if (pres_top_channel_pairs==2)
        {
            if (pres_ch_mode >=13 && b_pres_4_back_channels_present)
                bs_ch_config=0;
            else if (pres_ch_mode<=12 && b_pres_4_back_channels_present)
                bs_ch_config=1;
            else if (pres_ch_mode<=12)
                    bs_ch_config=2;
        }
        else if (pres_top_channel_pairs==1)
        {
            if (pres_ch_mode>=13 && b_pres_4_back_channels_present)
                bs_ch_config=3;
            if (pres_ch_mode<=12 && b_pres_4_back_channels_present)
                bs_ch_config=4;
            else if (pres_ch_mode<=12)
                bs_ch_config = 5;
        }
    }

    Element_Begin1("custom_dmx_data");
    if (bs_ch_config!=(int8u)-1)
    {
        TEST_SB_SKIP(                                           "b_cdmx_data_present");
            Get_S1(2, n_cdmx_configs,                           "n_cdmx_configs_minus1");
            n_cdmx_configs++;

            Presentations.back().Dmx.Cdmxs.reserve(n_cdmx_configs);
            for (int8u Pos=0; Pos<n_cdmx_configs; Pos++)
            {
                Element_Begin1("cdmx_config");
                int8u out_ch_config;
                if (bs_ch_config==2 || bs_ch_config == 5)
                    Get_S1 (1, out_ch_config,                   "out_ch_config");
                else
                    Get_S1 (3, out_ch_config,                   "out_ch_config");
                Param_Info1C(out_ch_config<out_ch_config_Size, out_ch_config_Values[out_ch_config]);

                Presentations.back().Dmx.Cdmxs.resize(Presentations.back().Dmx.Cdmxs.size()+1);
                Presentations.back().Dmx.Cdmxs.back().out_ch_config=out_ch_config;
                cdmx_parameters(bs_ch_config, out_ch_config);
                Element_End0();
            }
        TEST_SB_END();
    }

    if ((pres_ch_mode!=(int8u)-1 && pres_ch_mode>=3) || (pres_ch_mode_core!=(int8u)-1 && pres_ch_mode_core>=3))
    {
        TEST_SB_SKIP(                                           "b_stereo_dmx_coeff");
            Get_S1 (3, D.loro_centre_mixgain,                   "loro_centre_mixgain");
            Get_S1 (3, D.loro_surround_mixgain,                 "loro_surround_mixgain");
            TEST_SB_SKIP(                                       "b_ltrt_mixinfo");
                Get_S1 (3, D.ltrt_centre_mixgain,               "ltrt_centre_mixgain");
                Get_S1 (3, D.ltrt_surround_mixgain,             "ltrt_surround_mixgain");
            TEST_SB_END();
            if (b_pres_has_lfe)
            {
                TEST_SB_SKIP(                                   "b_lfe_mixinfo");
                    Get_S1 (5, D.lfe_mixgain,                   "lfe_mixgain");
                TEST_SB_END();
            }
            Get_S1 (2, D.preferred_dmx_method,                  "preferred_dmx_method");
        TEST_SB_END();
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::cdmx_parameters(int8u bs_ch_config, int8u out_ch_config)
{
    Element_Begin1("cdmx_parameters");
    if (bs_ch_config==0 || bs_ch_config==3)
        tool_scr_to_c_l();

    if (bs_ch_config<2)
    {
        switch (out_ch_config)
        {
            case 0:
                    tool_t4_to_f_s();
                    tool_b4_to_b2();
                    break;
            case 1:
                    tool_t4_to_t2();
                    tool_b4_to_b2();
                    break;
            case 2:
                    tool_b4_to_b2();
                    break;
            case 3:
                    tool_t4_to_f_s_b();
                    break;
            case 4:
                    tool_t4_to_t2();
        break;
        }
    }
    else if (bs_ch_config==2)
    {
        switch (out_ch_config)
        {
            case 0:
                    tool_t4_to_f_s();
                    break;
            case 1:
                    tool_t4_to_t2();
                    break;
        }
    }
    else if (bs_ch_config==3 || bs_ch_config==4)
    {
        switch (out_ch_config)
        {
            case 0:
                    tool_t2_to_f_s();
                    tool_b4_to_b2();
                    break;
            case 1:
                    tool_b4_to_b2();
                    break;
            case 2:
                    tool_b4_to_b2();
                    break;
            case 3:
                    tool_t2_to_f_s_b();
                    break;
            }
    }
    else if (bs_ch_config==5 && out_ch_config==0)
    {
        tool_t2_to_f_s();
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::tool_scr_to_c_l()
{
    Element_Begin1("tool_scr_to_c_l");
        TESTELSE_SB_SKIP(                                       "b_put_screen_to_c");
            Get_Gain(3, gain::f1_code,                          "gain_f1_code");
        TESTELSE_SB_ELSE(                                       "b_put_screen_to_c");
            Get_Gain(3, gain::f2_code,                          "gain_f2_code");
        TESTELSE_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::tool_b4_to_b2()
{
    Element_Begin1("tool_b4_to_b2");
    Get_Gain(3, gain::b_code,                                   "gain_b_code");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::tool_t4_to_t2()
{
    Element_Begin1("tool_t4_to_t2");
    Get_Gain(3, gain::t1_code,                                  "gain_t1_code");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::tool_t4_to_f_s_b()
{
    Element_Begin1("tool_t4_to_f_s_b");
    TESTELSE_SB_SKIP(                                           "b_top_front_to_front");
        Get_Gain(3, gain::t2a_code,                             "gain_t2a_code");
        Get_Gain(0, gain::t2b_code,                             nullptr);
    TESTELSE_SB_ELSE(                                           "b_top_front_to_front");
        TESTELSE_SB_SKIP(                                       "b_top_front_to_side");
            Get_Gain(3, gain::t2b_code,                         "gain_t2b_code");
        TESTELSE_SB_ELSE(                                       "b_top_front_to_side");
            Get_Gain(0, gain::t2b_code,                         nullptr);
            Get_Gain(3, gain::t2c_code,                         "gain_t2c_code");
        TESTELSE_SB_END();
    TESTELSE_SB_END();

    TESTELSE_SB_SKIP(                                           "b_top_back_to_front");
        Get_Gain(3, gain::t2d_code,                              "gain_t2d_code");
        Get_Gain(0, gain::t2e_code,                              nullptr);
    TESTELSE_SB_ELSE(                                           "b_top_back_to_front");
        TESTELSE_SB_SKIP(                                       "b_top_back_to_side");
           Get_Gain(3, gain::t2e_code,                           "gain_t2e_code");
        TESTELSE_SB_ELSE(                                       "b_top_back_to_side");
            Get_Gain(0, gain::t2e_code,                          nullptr);
            Get_Gain(3, gain::t2f_code,                          "gain_t2f_code");
        TESTELSE_SB_END();
    TESTELSE_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::tool_t4_to_f_s()
{
    Element_Begin1("tool_t4_to_f_s");
    TESTELSE_SB_SKIP(                                           "b_top_front_to_front");
        Get_Gain(3, gain::t2a_code,                             "gain_t2a_code");
        Get_Gain(0, gain::t2b_code,                             nullptr);
    TESTELSE_SB_ELSE(                                           "b_top_front_to_front");
        Get_Gain(3, gain::t2b_code,                             "gain_t2b_code");
    TESTELSE_SB_END();

    TESTELSE_SB_SKIP(                                           "b_top_back_to_front");
        Get_Gain(3, gain::t2d_code,                             "gain_t2d_code");
        Get_Gain(0, gain::t2e_code,                             nullptr);
    TESTELSE_SB_ELSE(                                           "b_top_back_to_front");
        Get_Gain(3, gain::t2e_code,                             "gain_t2e_code");
    TESTELSE_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::tool_t2_to_f_s_b()
{
    Element_Begin1("tool_t2_to_f_s_b");
    TESTELSE_SB_SKIP(                                           "b_top_to_front");
        Get_Gain(3, gain::t2a_code,                             "gain_t2a_code");
        Get_Gain(0, gain::t2b_code,                             nullptr);
    TESTELSE_SB_ELSE(                                           "b_top_to_front");
        TESTELSE_SB_SKIP(                                       "b_top_to_side");
            Get_Gain(3, gain::t2b_code,                         "gain_t2b_code");
        TESTELSE_SB_ELSE(                                       "b_top_to_side");
            Get_Gain(0, gain::t2b_code,                         nullptr);
            Get_Gain(3, gain::t2c_code,                         "gain_t2c_code");
        TESTELSE_SB_END();
    TESTELSE_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::tool_t2_to_f_s()
{
    Element_Begin1("tool_t2_to_f_s");
    TESTELSE_SB_SKIP(                                           "b_top_to_front");
        Get_Gain(3, gain::t2a_code,                             "gain_t2a_code");
        Get_Gain(0, gain::t2b_code,                             nullptr);
    TESTELSE_SB_ELSE(                                           "b_top_to_front");
        Get_Gain(3, gain::t2b_code,                             "gain_t2b_code");
    TESTELSE_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::loud_corr(int8u pres_ch_mode, int8u pres_ch_mode_core, bool b_objects)
{
    bool b_obj_loud_corr=false, b_corr_for_immersive_out=false;

    Element_Begin1("loud_corr");
    if (b_objects)
        Get_SB (b_obj_loud_corr,                                "b_obj_loud_corr");

    if ((pres_ch_mode!=(int8u)-1 && pres_ch_mode>4) || b_obj_loud_corr)
        Get_SB (b_corr_for_immersive_out,                       "b_corr_for_immersive_out");

    if ((pres_ch_mode!=(int8u)-1 && pres_ch_mode>1) || b_obj_loud_corr)
    {
        TEST_SB_SKIP(                                           "b_loro_loud_comp");
            Skip_S1(5,                                          "loro_dmx_loud_corr");
        TEST_SB_END();

        TEST_SB_SKIP(                                           "b_ltrt_loud_comp");
            Skip_S1(5,                                          "ltrt_dmx_loud_corr");
        TEST_SB_END();
    }

    if ((pres_ch_mode!=(int8u)-1 && pres_ch_mode>4) || b_obj_loud_corr)
    {
        TEST_SB_SKIP(                                           "b_loud_comp");
            Skip_S1(5,                                          "loud_corr_5_X");
        TEST_SB_END();

        if (b_corr_for_immersive_out)
        {
            TEST_SB_SKIP(                                       "b_loud_comp");
                Skip_S1(5,                                      "loud_corr_5_X_2");
            TEST_SB_END();

            TEST_SB_SKIP(                                       "b_loud_comp");
                Skip_S1(5,                                      "loud_corr_7_X");
            TEST_SB_END();
        }
    }

    if (((pres_ch_mode!=(int8u)-1 && pres_ch_mode>10) || b_obj_loud_corr) && b_corr_for_immersive_out)
    {
        TEST_SB_SKIP(                                           "b_loud_comp");
            Skip_S1(5,                                          "loud_corr_7_X_4");
        TEST_SB_END();

        TEST_SB_SKIP(                                           "b_loud_comp");
            Skip_S1(5,                                          "loud_corr_7_X_2");
        TEST_SB_END();

        TEST_SB_SKIP(                                           "b_loud_comp");
            Skip_S1(5,                                          "loud_corr_5_X_4");
        TEST_SB_END();
    }

    if (pres_ch_mode_core!=(int8u)-1 && pres_ch_mode_core>4)
    {
        TEST_SB_SKIP(                                           "b_loud_comp");
            Skip_S1(5,                                          "loud_corr_5_X_2");
        TEST_SB_END();
    }

    if (pres_ch_mode_core!=(int8u)-1 && pres_ch_mode_core>2)
    {
        TEST_SB_SKIP(                                           "b_loud_comp");
            Skip_S1(5,                                          "loud_corr_5_X");
        TEST_SB_END();

        TEST_SB_SKIP(                                           "b_loud_comp");
            Skip_S1(5,                                          "loud_corr_core_loro");
            Skip_S1(5,                                          "loud_corr_core_ltrt");
        TEST_SB_END();
    }

    if (b_obj_loud_corr)
    {
        TEST_SB_SKIP(                                           "b_loud_comp");
            Skip_S1(5,                                          "loud_corr_9_X_4");
        TEST_SB_END();
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::drc_frame(drc_info& DrcInfo, bool b_iframe)
{
    Element_Begin1("drc_frame");
    TEST_SB_SKIP(                                           "b_drc_present");
        if (b_iframe)
            drc_config(DrcInfo);

        drc_data(DrcInfo);
    TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::drc_config(drc_info& DrcInfo)
{
    int8u drc_decoder_nr_modes;
    Element_Begin1("drc_config");
        Get_S1 (3, drc_decoder_nr_modes,                        "drc_decoder_nr_modes");
        DrcInfo.Decoders.clear();
        for (int8u Pos=0; Pos<=drc_decoder_nr_modes; Pos++)
        {
            DrcInfo.Decoders.resize(DrcInfo.Decoders.size()+1);
            drc_decoder_mode_config(DrcInfo.Decoders.back());
        }

        //Manage drc_repeat_id
        for (int8u i=0; i<=drc_decoder_nr_modes; i++)
        {
            if (DrcInfo.Decoders[i].drc_repeat_id!=(int8u)-1)
            {
                for (int8u j=0; j<=drc_decoder_nr_modes; j++)
                    if (j!=i && DrcInfo.Decoders[j].drc_decoder_mode_id==DrcInfo.Decoders[i].drc_repeat_id)
                    {
                        int8u drc_decoder_mode_id=DrcInfo.Decoders[i].drc_decoder_mode_id;
                        DrcInfo.Decoders[i]=DrcInfo.Decoders[j];
                        DrcInfo.Decoders[i].drc_decoder_mode_id=drc_decoder_mode_id;
                        DrcInfo.Decoders[i].drc_compression_curve_flag=true;
                        break;
                    }
            }
        }

        Get_S1 (3, DrcInfo.drc_eac3_profile,                    "drc_eac3_profile");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::drc_data(drc_info& DrcInfo)
{
    bool curve_present=false;
    int16u drc_gainset_size;
    int8u drc_version;
    size_t Remain_Before, used_bits=0;

    Element_Begin1("drc_data");
    for (int8u Pos=0; Pos<DrcInfo.Decoders.size(); Pos++)
    {
        if (!DrcInfo.Decoders[Pos].drc_compression_curve_flag)
        {
            Get_S2 (6, drc_gainset_size,                        "drc_gainset_size");
            TEST_SB_SKIP(                                       "b_more_bits");
                int32u drc_gainset_size32;
                Get_V4(2, drc_gainset_size32,                   "drc_gainset_size");
                drc_gainset_size+=(int16u)drc_gainset_size32<<6;
            TEST_SB_END();
            Get_S1 (2, drc_version,                             "drc_version");

            if (drc_version<=1)
            {
                Remain_Before=Data_BS_Remain();
                drc_gains(DrcInfo.Decoders[Pos]);
                used_bits=Remain_Before-Data_BS_Remain();
            }

            if (drc_version>=1)
            {
                Skip_BS(drc_gainset_size-2-used_bits,           "drc2_bits");
            }
        }
        else
        {
            curve_present=true;
        }
    }

    if (curve_present)
    {
        Skip_SB(                                                "drc_reset_flag");
        Skip_S1(2,                                              "drc_reserved");
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::drc_gains(drc_decoder_config& D)
{
    Element_Begin1("drc_gains");
    Skip_S1(7,                                                  "drc_gain_val");
    if (D.drc_gains_config>0)
    {
        // TODO:
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::drc_decoder_mode_config(drc_decoder_config& D)
{
    D.drc_compression_curve_flag=false;

    Element_Begin1("drc_decoder_mode_config");
    Get_S1 (3, D.drc_decoder_mode_id,                           "drc_decoder_mode_id[pcount]");
    if (D.drc_decoder_mode_id>3)
    {
        Skip_S1(5,                                              "drc_output_level_from");
        Skip_S1(5,                                              "drc_output_level_to");
    }

    TESTELSE_SB_SKIP(                                           "drc_repeat_profile_flag");
        Get_S1 (3, D.drc_repeat_id,                             "drc_repeat_id");
        D.drc_compression_curve_flag=true;
    TESTELSE_SB_ELSE(                                           "drc_repeat_profile_flag");
        TESTELSE_SB_GET (D.drc_default_profile_flag,            "drc_default_profile_flag");
            D.drc_compression_curve_flag=true;
        TESTELSE_SB_ELSE(                                       "drc_default_profile_flag");
            TESTELSE_SB_GET(D.drc_compression_curve_flag,       "drc_compression_curve_flag[drc_decoder_mode_id[pcount]]");
                drc_compression_curve(D.drc_compression_curve);
            TESTELSE_SB_ELSE(                                   "drc_compression_curve_flag[drc_decoder_mode_id[pcount]]");
                Get_S1 (2, D.drc_gains_config,                  "drc_gains_config[drc_decoder_mode_id[pcount]]");
            TESTELSE_SB_END();
        TESTELSE_SB_END();
    TESTELSE_SB_END();
    Element_End0();
};

void File_Ac4::drc_compression_curve(drc_decoder_config_curve& C)
{
    memset(&C, -1, sizeof(C));

    Element_Begin1("drc_compression_curve");
    Get_S1 (4, C.drc_lev_nullband_low,                          "drc_lev_nullband_low");
    Get_S1 (4, C.drc_lev_nullband_high,                         "drc_lev_nullband_high");

    Get_S1 (4, C.drc_gain_max_boost,                            "drc_gain_max_boost");
    if (C.drc_gain_max_boost)
    {
        Skip_S1(5,                                              "drc_lev_max_boost");
        TEST_SB_SKIP(                                           "drc_nr_boost_sections");
            Skip_S1(4,                                          "drc_gain_section_boost");
            Skip_S1(5,                                          "drc_lev_section_boost");
        TEST_SB_END();
    }

    Get_S1 (5, C.drc_gain_max_cut,                              "drc_gain_max_cut");
    if (C.drc_gain_max_cut)
    {
        Get_S1 (6, C.drc_lev_max_cut,                           "drc_lev_max_cut");
        TEST_SB_SKIP(                                           "drc_nr_cut_sections");
            Get_S1 (5, C.drc_gain_section_cut,                  "drc_gain_section_cut");
            Get_S1 (5, C.drc_lev_section_cut,                   "drc_lev_section_cut");
        TEST_SB_END();
    }

    TESTELSE_SB_SKIP(                                           "drc_tc_default_flag");
    TESTELSE_SB_ELSE(                                           "drc_tc_default_flag");
        Get_S1 (8, C.drc_tc_attack,                             "drc_tc_attack");
        Get_S1 (8, C.drc_tc_release,                            "drc_tc_release");
        Get_S1 (8, C.drc_tc_attack_fast,                        "drc_tc_attack_fast");
        Get_S1 (8, C.drc_tc_release_fast,                       "drc_tc_release_fast");

        TEST_SB_SKIP(                                           "drc_adaptive_smoothing_flag");
            Get_S1 (5, C.drc_attack_threshold,                  "drc_attack_threshold");
            Get_S1 (5, C.drc_release_threshold,                 "drc_release_threshold");
        TEST_SB_END();
    TESTELSE_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::further_loudness_info(loudness_info& L, bool sus_ver, bool b_presentation_ldn)
{
    bool b_loudcorr_type;
    Element_Begin1("further_loudness_info");
    if (b_presentation_ldn || !sus_ver)
    {
        int8u loudness_version;
        Get_S1 (2, loudness_version,                            "loudness_version");
        if (loudness_version==3)
            Skip_S1(4,                                          "extended_loudness_version");

        Get_S1 (4, L.loud_prac_type,                            "loud_prac_type");
        if (L.loud_prac_type)
        {
            TEST_SB_SKIP(                                       "b_loudcorr_dialgate");
                Get_S1 (3, L.loud_dialgate_prac_type,           "dialgate_prac_type");
            TEST_SB_END();
            Get_SB (L.b_loudcorr_type,                          "b_loudcorr_type");
        }
    }
    else
    {
        Skip_SB(                                                "b_loudcorr_dialgate");
    }
    TEST_SB_SKIP(                                               "b_loudrelgat");
        Get_S2 (11, L.loudrelgat,                               "loudrelgat");
    TEST_SB_END();

    TEST_SB_SKIP(                                               "b_loudspchgat");
        Get_S2 (11, L.loudspchgat,                              "loudspchgat");
        Get_S1 (3, L.loudspchgat_dialgate_prac_type,            "dialgate_prac_type");
    TEST_SB_END();
    TEST_SB_SKIP(                                               "b_loudstrm3s");
        Skip_S2(11,                                             "loudstrm3s");
    TEST_SB_END();
    TEST_SB_SKIP(                                               "b_max_loudstrm3s");
        Skip_S2(11,                                             "max_loudstrm3s");
    TEST_SB_END();
    TEST_SB_SKIP(                                               "b_truepk");
        Skip_S2(11,                                             "truepk");
    TEST_SB_END();
    TEST_SB_SKIP(                                               "b_max_truepk");
        Get_S2 (11, L.max_truepk,                               "max_truepk");
    TEST_SB_END();
    if (b_presentation_ldn || !sus_ver)
    {
        TEST_SB_SKIP(                                           "b_prgmbndy");
            Element_Begin1("prgmbndy_bits");
            int32u prgmbndy=1;
            bool prgmbndy_bit=false;
            while(!prgmbndy_bit)
                Get_SB (prgmbndy_bit,                           "prgmbndy_bit");
            Element_Info1(prgmbndy);
            Element_End0();
            Skip_SB(                                            "b_end_or_start");
            TEST_SB_SKIP(                                       "b_prgmbndy_offset");
                Skip_S2(11,                                     "prgmbndy_offset");
            TEST_SB_END();
        TEST_SB_END();
    }
    TEST_SB_SKIP(                                               "b_lra");
        Get_S2 (10, L.lra,                                      "lra");
        Get_S1 ( 3, L.lra_prac_type,                            "lra_prac_type");
    TEST_SB_END();
    TEST_SB_SKIP(                                               "b_loudmntry");
        Skip_S2(11,                                             "loudmntry");
    TEST_SB_END();
    TEST_SB_SKIP(                                               "b_max_loudmntry");
        Get_S2 (11, L.max_loudmntry,                            "max_loudmntry");
    TEST_SB_END();
    if (sus_ver)
    {
        TEST_SB_SKIP(                                           "b_rtllcomp");
            Skip_S1(8,                                          "rtllcomp");
        TEST_SB_END();
    }
    TEST_SB_SKIP(                                               "b_extension");
        int8u e_bits_size;
        Get_S1 (5, e_bits_size,                                 "e_bits_size");
        if (e_bits_size==31)
        {
            int32u e_bits_size32;
            Get_V4 (4, e_bits_size32,                           "e_bits_size");
            e_bits_size32+=31;
            e_bits_size=(int8u)e_bits_size32;
        }
        if (!sus_ver)
        {
            e_bits_size--;
           TEST_SB_SKIP(                                        "b_rtllcomp");
                e_bits_size-=8;
                Skip_S1(8,                                      "rtll_comp");
            TEST_SB_END();
        }
        Skip_BS(e_bits_size,                                    "extensions_bits");
    TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::dac4()
{
    Element_Begin1("ac4_dsi");
    BS_Begin();
    int16u n_presentations;
    int8u ac4_dsi_version;
    Get_S1 (3, ac4_dsi_version,                                 "ac4_dsi_version");
    if (ac4_dsi_version>1)
    {
        Skip_BS(Data_BS_Remain(),                               "Unknown");
        BS_End();
        return;
    }
    Get_S1 (7, bitstream_version,                               "bitstream_version");
    if (bitstream_version>2)
    {
        Skip_BS(Data_BS_Remain(),                               "Unknown");
        BS_End();
        Element_End0();
        return;
    }
    Get_SB (   fs_index,                                        "fs_index");
    Get_S1 (4, frame_rate_index,                                "frame_rate_index"); Param_Info1(Ac4_frame_rate[fs_index][frame_rate_index]);
    Get_S2 (9, n_presentations,                                 "n_presentations");
    if (bitstream_version > 1)
    {
        TEST_SB_SKIP(                                           "b_program_id");
            Skip_S2(16,                                         "short_program_id");
            TEST_SB_SKIP(                                       "b_program_uuid_present");
                Skip_BS(128,                                    "program_uuid");
            TEST_SB_END();
        TEST_SB_END();
    }
    ac4_bitrate_dsi();
    size_t byte_align=Data_BS_Remain()%8;
    if (byte_align)
        Skip_S1(byte_align,                                     "byte_align");
    BS_End();
    Presentations_dac4.resize(n_presentations);
    for (int8u p=0; p<n_presentations; p++)
    {
        Element_Begin1("presentation");
        presentation& P=Presentations_dac4[p];

        int32u pres_bytes;
        int8u  pres_bytes8;
        Get_B1 (P.presentation_version,                         "presentation_version");
        Get_B1 (pres_bytes8,                                    "pres_bytes");
        if (pres_bytes8==0xFF)
        {
            int16u add_pres_bytes;
            Get_B2 (add_pres_bytes,                              "add_pres_bytes");
            pres_bytes=pres_bytes8+add_pres_bytes;
        }
        else
            pres_bytes=pres_bytes8;
        size_t Element_Size_Save=Element_Size;
        Element_Size=Element_Offset+pres_bytes;
        switch (P.presentation_version)
        {
            //case 0 : ac4_presentation_v0_dsi(); break;
            case 1 :
            case 2 : ac4_presentation_v1_dsi(P); break;
        }
        size_t skip_bytes=Element_Size-Element_Offset;
        if (skip_bytes)
            Skip_XX(skip_bytes,                                 "skip_area");
        Element_Size=Element_Size_Save;
        Element_End0();
    }

    Element_End0();

    FILLING_BEGIN();
        Accept();
    FILLING_END();
    Element_Offset=Element_Size;
    MustParse_dac4=false;

    ac4_toc_Compute(Presentations_dac4, Groups_dac4, true);
}

//---------------------------------------------------------------------------
void File_Ac4::alternative_info()
{
    int16u name_len;
    int8u n_targets;
    Element_Begin1("alternative_info");
    Get_S2 (16, name_len,                                       "name_len");
    for (int8u Pos=0; Pos<name_len; Pos++)
        Skip_S1(8,                                              "presentation_name");

    Get_S1 (5, n_targets,                                       "n_targets");
    for (int8u Pos=0; Pos<name_len; Pos++)
    {
        Skip_S1(3,                                              "target_md_compat");
        Skip_S1(8,                                              "target_device_category");
    }
    Element_End0();
}


//---------------------------------------------------------------------------
void File_Ac4::ac4_bitrate_dsi()
{
    Element_Begin1("ac4_bitrate_dsi");
        Skip_S1( 2,                                             "bit_rate_mode");
        Skip_S4(32,                                             "bit_rate");
        Skip_S4(32,                                             "bit_rate_precision");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_presentation_v1_dsi(presentation& P)
{
    Element_Begin1(                                             "ac4_presentation_v1_dsi");
    bool b_add_emdf_substreams=false;
    BS_Begin();
    Get_S1 (5, P.presentation_config,                           "presentation_config_v1");
    Param_Info1(Value(Ac4_presentation_config, P.presentation_config));
    if (P.presentation_config==6)
    {
        b_add_emdf_substreams=true;
    }
    else
    {
        if (P.presentation_config==0x1F)
            P.presentation_config=(int8u)-1; // 0x1F means not present
        int8u dsi_frame_rate_multiply_info, dsi_frame_rate_fraction_info;
        Skip_S1(3,                                              "mdcompat");
        TEST_SB_SKIP(                                           "b_presentation_id");
            Get_S4 (5, P.presentation_id,                       "presentation_id");
        TEST_SB_END();
        Get_S1 (2, dsi_frame_rate_multiply_info,                "dsi_frame_rate_multiply_info"); //TODO
        Get_S1 (2, dsi_frame_rate_fraction_info,                "dsi_frame_rate_fraction_info"); 
        Skip_S1(5,                                              "presentation_emdf_version");
        Skip_S2(10,                                             "presentation_key_id");
        TEST_SB_SKIP(                                           "b_presentation_channel_coded");
            Get_S1 (5, P.pres_ch_mode,                          "dsi_presentation_ch_mode");
            if (P.pres_ch_mode>=11 && P.pres_ch_mode<=14)
            {
                Get_SB (P.b_pres_4_back_channels_present,       "pres_b_4_back_channels_present");
                Get_S1 (2, P.pres_top_channel_pairs,            "pres_top_channel_pairs");
            }
            int32u presentation_channel_mask_v1;
            Get_S3 (24, presentation_channel_mask_v1,             "presentation_channel_mask_v1");
            presentation_channel_mask_v1 &=((int32u)-1)>>(32-Ac4_channel_mask_Size);
            int32u nonstd_bed_channel_assignment_mask=0;
            for (size_t i=0; i<Ac4_channel_mask_Size; i++)
                if (presentation_channel_mask_v1 &(1<<i))
                    for (size_t j=0; j<2; j++)
                        if (Ac4_channel_mask[i][j]!=ch_Max)
                            nonstd_bed_channel_assignment_mask|=Ac4_channel_mask[i][j];
            Param_Info1(AC4_nonstd_bed_channel_assignment_mask_ChannelLayout(nonstd_bed_channel_assignment_mask));
        TEST_SB_END();
        TEST_SB_SKIP(                                           "b_presentation_core_differs");
            TEST_SB_SKIP(                                       "b_presentation_core_channel_coded");
                Get_S1 (2, P.pres_ch_mode_core,                 "dsi_presentation_channel_mode_core");
            TEST_SB_END();
        TEST_SB_END();
        TEST_SB_SKIP(                                           "b_presentation_filter");
            Skip_SB(                                            "b_enable_presentation");
            int8u n_filter_bytes;
            Get_S1 (8, n_filter_bytes,                          "n_filter_bytes");
            if (n_filter_bytes)
                Skip_XX(n_filter_bytes*8,                       "filter_data");
        TEST_SB_END();

        if (P.presentation_config==(int8u)-1)
        {
            ac4_substream_group_dsi(P);
        }
        else
        {
            bool b_multi_pid;
            Get_SB (b_multi_pid,                                "b_multi_pid");
            P.b_multi_pid_PresentAndValue=b_multi_pid;
            if (P.presentation_config<3)
            {
                ac4_substream_group_dsi(P);
                ac4_substream_group_dsi(P);
            }
            else if (P.presentation_config<5)
            {
                ac4_substream_group_dsi(P);
                ac4_substream_group_dsi(P);
                ac4_substream_group_dsi(P);
            }
            else if (P.presentation_config==5)
            {
                int8u n_substream_groups;
                Get_S1 (3, n_substream_groups,                  "n_substream_groups_minus2");
                n_substream_groups+=2;
                for (int8u Pos=0; Pos<n_substream_groups; Pos++)
                    ac4_substream_group_dsi(P);
            }
            if (P.presentation_config>5)
            {
                int8u n_skip_bytes;
                Get_S1 (7, n_skip_bytes,                        "n_skip_bytes");
                if (n_skip_bytes)
                    Skip_XX(n_skip_bytes*8,                     "skip_data");
            }
        }

        Skip_SB(                                                "b_pre_virtualized");
        Get_SB (b_add_emdf_substreams,                          "b_add_emdf_substreams");
    }

    if (b_add_emdf_substreams)
    {
        int8u n_add_emdf_substreams;
        Get_S1 (7, n_add_emdf_substreams,                       "n_add_emdf_substreams");
        for (int8u Pos=0; Pos<n_add_emdf_substreams; Pos++)
        {
            Skip_S1(5,                                          "substream_emdf_version");
            Skip_S2(10,                                         "substream_key_id");
        }
    }

    TEST_SB_SKIP(                                               "b_presentation_bitrate_info");
        ac4_bitrate_dsi();
    TEST_SB_END();

    TEST_SB_GET(P.b_alternative,                                "b_alternative");
        size_t byte_align=Data_BS_Remain()%8;
        if (byte_align)
            Skip_S1(byte_align,                                 "byte_align");
            alternative_info();
    TEST_SB_END();
    size_t byte_align=Data_BS_Remain()%8;
    if (byte_align)
        Skip_S1(byte_align,                                     "byte_align");

    if(Data_BS_Remain()>=8)
    {
        Skip_SB(                                                "de_indicator");
        Skip_S1(5,                                              "reserved");
        TESTELSE_SB_SKIP(                                       "b_extended_presentation_id");
            Skip_S2(9,                                          "extended_presentation_id");
        TESTELSE_SB_ELSE(                                       "b_extended_presentation_id");
            Skip_SB(                                            "reserved");
        TESTELSE_SB_END();
    }

    BS_End();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_substream_group_dsi(presentation& P)
{
    P.substream_group_info_specifiers.push_back(Groups_dac4.size());
    Groups_dac4.resize(Groups_dac4.size()+1);
    group& G=Groups_dac4.back();

    bool b_substreams_present;
    int8u n_substreams;

    Element_Begin1("ac4_substream_group_dsi");
    Get_SB (b_substreams_present,                               "b_substreams_present");
    Get_SB (G.b_hsf_ext,                                        "b_hsf_ext");
    Get_SB (G.b_channel_coded,                                  "b_channel_coded");
    Get_S1 (8, n_substreams,                                    "n_substreams");
    G.Substreams.resize(n_substreams);
    for (int8u Pos=0; Pos<n_substreams; Pos++)
    {
        group_substream& GS=G.Substreams[Pos];
        GS.substream_type=Type_Ac4_Substream;
        Skip_S1(2,                                              "dsi_sf_multiplier");
        TEST_SB_SKIP(                                           "b_substream_bitrate_indicator");
            Skip_S1(5,                                          "substream_bitrate_indicator");
        TEST_SB_END();
        if (G.b_channel_coded)
        {
            int32u dsi_substream_channel_mask;
            Get_S3 (24, dsi_substream_channel_mask,             "dsi_substream_channel_mask");
            dsi_substream_channel_mask&=((int32u)-1)>>(32-Ac4_channel_mask_Size);
            int32u nonstd_bed_channel_assignment_mask=0;
            for (size_t i=0; i<Ac4_channel_mask_Size; i++)
                if (dsi_substream_channel_mask&(1<<i))
                    for (size_t j=0; j<2; j++)
                        if (Ac4_channel_mask[i][j]!=ch_Max)
                            nonstd_bed_channel_assignment_mask|=Ac4_channel_mask[i][j];
            Param_Info1(AC4_nonstd_bed_channel_assignment_mask_ChannelLayout(nonstd_bed_channel_assignment_mask));
        }
        else
        {
            TEST_SB_GET(GS.b_ajoc,                              "b_ajoc")
                Get_SB (GS.b_static_dmx,                        "b_static_dmx");
                if (GS.b_static_dmx==false)
                {
                    Get_S1 (4, GS.n_fullband_dmx_signals,       "n_dmx_objects_minus1");
                    GS.n_fullband_dmx_signals++;
                }
                Get_S1 (6, GS.n_fullband_upmix_signals,         "n_umx_objects_minus1");
                GS.n_fullband_upmix_signals++;
            TEST_SB_END();

            Skip_SB(                                            "b_substream_contains_bed_objects");
            Skip_SB(                                            "b_substream_contains_dynamic_objects");
            Skip_SB(                                            "b_substream_contains_ISF_objects");
            Skip_SB(                                            "reserved");
        }
    }
    TEST_SB_SKIP(                                               "b_content_type");
        Get_S1 (3, G.ContentInfo.content_classifier,            "content_classifier");
        TEST_SB_SKIP(                                           "b_language_indicator");
            int8u n_language_tag_bytes;
            Get_S1 (6, n_language_tag_bytes,                    "n_language_tag_bytes");
            for (int8u Pos=0; Pos<n_language_tag_bytes; Pos++)
            {
                int8u language_tag_bytes;
                Get_S1 (8, language_tag_bytes,                  "language_tag_bytes");
                G.ContentInfo.language_tag_bytes+=(language_tag_bytes<0x80?language_tag_bytes:'?');
            }
        TEST_SB_END();
    TEST_SB_END();
    Element_End0();
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ac4::Get_V4(int8u Bits, int32u& Info, const char* Name)
{
    Info=0;
    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            int8u Count=0;
            for (;;)
            {
                Info+=BS->Get4(Bits);
                Count+=1+Bits;
                if (!BS->GetB())
                    break;
                Info<<=Bits;
                Info+=(1<<Bits);
            }

            Param(Name, Info, Count);
            Param_Info(__T("(")+Ztring::ToZtring(Count)+__T(" bits)"));
        }
        else
    #endif //MEDIAINFO_TRACE
        {
            for (;;)
            {
                Info+=BS->Get4(Bits);
                if (!BS->GetB())
                    break;
                Info<<=Bits;
                Info+=(1<<Bits);
            }
        }
}

//---------------------------------------------------------------------------
void File_Ac4::Skip_V4(int8u  Bits, const char* Name)
{
    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            int32u Info=0;
            int8u Count=0;
            for (;;)
            {
                Info+=BS->Get4(Bits);
                Count+=Bits;
                if (!BS->GetB())
                    break;
                Info<<=Bits;
                Info+=(1<<Bits);
            }

            Param(Name, Info, Count);
            Param_Info(__T("(")+Ztring::ToZtring(Count)+__T(" bits)"));
        }
        else
    #endif //MEDIAINFO_TRACE
        {
            for (;;)
            {
                BS->Skip(Bits);
                if (!BS->GetB())
                    break;
            }
        }
}

//---------------------------------------------------------------------------
void File_Ac4::Get_V4(int8u Bits1, int8u Bits2, int8u Flag_Value, int32u &Info, const char* Name)
{
    Info = 0;
    int8u Count=Bits1;

    Peek_S4(Count, Info);
    if (Info==Flag_Value)
    {
        Count=Bits2;
        Peek_S4(Count, Info);
    }
    BS->Skip(Count);

    #if MEDIAINFO_TRACE
    if (Trace_Activated)
    {
        Param(Name, Info, Count);
        Param_Info(__T("(")+Ztring::ToZtring(Count)+__T(" bits)"));
    }
    #endif
}

//---------------------------------------------------------------------------
void File_Ac4::Skip_V4(int8u Bits1, int8u Bits2, int8u Flag_Value, const char* Name)
{
    int32u Info = 0;
    int8u Count=Bits1;

    Peek_S4(Count, Info);
    if (Info==Flag_Value)
    {
        Count=Bits2;
        Peek_S4(Count, Info);
    }
    BS->Skip(Count);

    #if MEDIAINFO_TRACE
    if (Trace_Activated)
    {
        Param(Name, Info, Count);
        Param_Info(__T("(")+Ztring::ToZtring(Count)+__T(" bits)"));
    }
    #endif
}

//---------------------------------------------------------------------------
void File_Ac4::Get_V4(int8u Bits1, int8u Bits2, int8u Bits3, int8u Bits4, int32u  &Info, const char* Name)
{
    Info = 0;

    int8u Temp;
    int8u Count=Bits1;
    Peek_S1(Bits1, Temp);
    if (Temp==~(~0u<<Bits1))
    {
        Count=Bits2;
        Peek_S1(Bits2, Temp);
        if (Temp==~(~0u<<Bits2))
        {
            Count=Bits3;
            Peek_S1(Bits3, Temp);
            if (Temp==~(~0u<<Bits3))
            {
                Count=Bits4;
                Peek_S1(Bits4, Temp);
            }
        }
    }

    Info=(int32u)Temp;
    BS->Skip(Count);
    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            Param(Name, Info, Count);
            Param_Info(__T("(")+Ztring::ToZtring(Count)+__T(" bits)"));
        }
    #endif //MEDIAINFO_TRACE
}

//---------------------------------------------------------------------------
void File_Ac4::Get_V4(const variable_size* Bits, int8u &Info, const char* Name)
{
    int8u TotalSize=Bits[0].AddedSize;
    Bits++;
    int8u BitSize=0;
    int16u Temp;
    for (int8u i=0; i<TotalSize; i++)
    {
        int8u AddedSize=Bits[i].AddedSize;
        if (AddedSize)
        {
            BitSize+=AddedSize;
            Peek_S2(BitSize, Temp);
        }
        if (Temp==Bits[i].Value)
        {
            Skip_S2(BitSize, Name); Param_Info1(i);
            Info=i;
            return;
        }
    }

    //Problem
    Skip_S2(BitSize, Name);
    Trusted_IsNot("Variable size");
    Info=(int8u)-1;
}

//---------------------------------------------------------------------------
void File_Ac4::Get_VB(int8u  &Info, const char* Name)
{
    Info = 0;

    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            int8u Count=1;
            while (BS->GetB())
            {
                Info++;
                Count++;
            }
            Param(Name, Info, Count);
            Param_Info(__T("(")+Ztring::ToZtring(Count)+__T(" bits)"));
        }
        else
    #endif //MEDIAINFO_TRACE
        {
            while (BS->GetB())
                Info++;
        }
}

//---------------------------------------------------------------------------
void File_Ac4::Skip_VB(const char* Name)
{
    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            int8u Info=0;
            int8u Count=0;
            do
            {
                Info++;
                Count++;
            }
            while (BS->GetB());
            Param(Name, Info, Count);
            Param_Info(__T("(")+Ztring::ToZtring(Count)+__T(" bits)"));
        }
        else
    #endif //MEDIAINFO_TRACE
        {
            while (BS->GetB());
        }
}


//***************************************************************************
// Utils
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Ac4::CRC_Compute(size_t Size)
{
    int16u CRC_16=0x0000;
    const int8u* CRC_16_Buffer=Buffer+Buffer_Offset+2; //After sync_word
    const int8u* CRC_16_Buffer_End=Buffer+Buffer_Offset+Size; //End of frame
    while(CRC_16_Buffer<CRC_16_Buffer_End)
    {
        CRC_16=(CRC_16<<8) ^ CRC_16_Table[(CRC_16>>8)^(*CRC_16_Buffer)];
        CRC_16_Buffer++;
    }

    return (CRC_16==0x0000);
}

//---------------------------------------------------------------------------
int8u File_Ac4::Superset(int8u Ch_Mode1, int8u Ch_Mode2)
{
    if (Ch_Mode1>15 && Ch_Mode2>15)
        return (int8u)-1;
    else if (Ch_Mode1>15)
        return Ch_Mode2;
    else if (Ch_Mode2>15)
        return Ch_Mode1;
        else if (Ch_Mode1==15 || Ch_Mode2==15)
        return 15;

    static int8u Modes[16][3]=
    {
        {1,0,0},
        {2,0,0},
        {3,0,0},
        {5,0,0},
        {5,1,0},
        {7,0,0},
        {7,1,0},
        {7,0,1},
        {7,1,1},
        {7,0,2},
        {7,1,2},
        {7,0,4},
        {7,1,4},
        {9,0,4},
        {9,1,4},
        {22,2,0}
    };

    for (int8u Pos=0; Pos<15; Pos++)
    {
        if (Modes[Ch_Mode1][0]<=Modes[Pos][0] && Modes[Ch_Mode1][1]<=Modes[Pos][1] && Modes[Ch_Mode1][2]<=Modes[Pos][2] &&
            Modes[Ch_Mode2][0]<=Modes[Pos][0] && Modes[Ch_Mode2][1]<=Modes[Pos][1] && Modes[Ch_Mode2][2]<=Modes[Pos][2])
            return Pos;
    }

    return (int8u)-1;
}

//---------------------------------------------------------------------------
int16u File_Ac4::Huffman_Decode(const ac4_huffman& Table, const char* Name)
{
    bool bit;
    int16s index = 0;

    Element_Begin1(Name);
    while (index>=0)
    {
        Get_SB (bit,                                            "bit");
        index=Table[index][bit];
    }
    Element_End0();

    return index+64;
}

void File_Ac4::Get_Gain(int8u Bits, gain Type, const char* Name)
{
    dmx::cdmx::gain Gain;
    Gain.Type=Type;
    if (Bits)
    {
        Get_S1(Bits, Gain.Value,                                Name); Param_Info3(Gain.Type==gain::f1_code?gain_f1_Values(Gain.Value):gain_xx_Values(Gain.Value), " dB", 1);
    }
    else
        Gain.Value=7;
    Presentations.back().Dmx.Cdmxs.back().Gains.push_back(Gain);
}

} //NameSpace

#endif //MEDIAINFO_AC4_YES

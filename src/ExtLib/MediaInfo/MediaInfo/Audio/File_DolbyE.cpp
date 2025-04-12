/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
// https://developer.dolby.com/globalassets/professional/dolby-e/dolby-e-high-level-frame-description.pdf
//---------------------------------------------------------------------------

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
#if defined(MEDIAINFO_DOLBYE_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_DolbyE.h"
#include "MediaInfo/Audio/File_Aac.h"
#include <cmath>

#if !defined(INT8_MAX)
#define INT8_MAX (127)
#endif //!defined(INT8_MAX)
#if !defined(INT8_MIN)
#define INT8_MIN (-128)
#endif //!defined(INT8_MIN)

using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Info
//***************************************************************************

//---------------------------------------------------------------------------
extern const float64 AC3_dynrng[];
extern const float64 AC3_compr[];
extern const int16u AC3_BitRate[];
extern const int8u AC3_Channels[];
extern const char* AC3_Surround[];
extern const char* AC3_Mode[];
extern const char* AC3_Mode_String[];
extern const char* AC3_ChannelPositions[];
extern const char* AC3_ChannelPositions2[];
extern const char* AC3_ChannelLayout_lfeoff[];
extern const char* AC3_ChannelLayout_lfeon[];
extern const char* AC3_roomtyp[];
extern const char* AC3_dmixmod[];
extern string AC3_Level_Value(int8u Index, float Start, float Multiplier);
extern void AC3_Level_Fill(File__Analyze* A, size_t StreamPos, int8u Index, float Start, float Multiplier, const char* Name);
extern string AC3_dynrngprof_Get(int8u Value);

//***************************************************************************
// Utils
//***************************************************************************

//---------------------------------------------------------------------------
//CRC computing, with incomplete first and last bytes
//Inspired by http://zorc.breitbandkatze.de/crc.html
extern const int16u CRC_16_Table[256];
int16u CRC_16_Compute(const int8u* Buffer_Begin, size_t Buffer_Size, int8u SkipBits_Begin, int8u SkipBits_End)
{
    int16u CRC_16=0x0000;
    const int8u* Buffer=Buffer_Begin;
    const int8u* Buffer_End=Buffer+Buffer_Size;
    if (SkipBits_End)
        Buffer_End--; //Not handling completely the last byte

    //First partial byte
    if (SkipBits_Begin)
    {
        for (int8u Mask=(1<<(7-SkipBits_Begin)); Mask; Mask>>=1)
        {
            bool NewBit=(CRC_16&0x8000)?true:false;
            CRC_16<<=1;
            if ((*Buffer)&Mask)
                NewBit=!NewBit;
            if (NewBit)
                CRC_16^=0x8005;
        }

        Buffer++;
    }

    //Complete bytes
    while (Buffer<Buffer_End)
    {
        CRC_16=(CRC_16<<8) ^ CRC_16_Table[(CRC_16>>8)^(*Buffer)];
        Buffer++;
    }

    //Last partial byte
    if (SkipBits_End)
    {
        for (int8u Mask=0x80; Mask>(1<<(SkipBits_End-1)); Mask>>=1)
        {
            bool NewBit=(CRC_16&0x8000)?true:false;
            CRC_16<<=1;
            if ((*Buffer)&Mask)
                NewBit=!NewBit;
            if (NewBit)
                CRC_16^=0x8005;
        }

        Buffer++;
    }

    return CRC_16;
}

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
static const int8u DolbyE_Programs[64]=
{2, 3, 2, 3, 4, 5, 4, 5, 6, 7, 8, 1, 2, 3, 3, 4, 5, 6, 1, 2, 3, 4, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

//---------------------------------------------------------------------------
static const int8u DolbyE_Channels[64]=
{8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 6, 6, 6, 6, 6, 6, 6, 4, 4, 4, 4, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

//---------------------------------------------------------------------------
const int8u DolbyE_Channels_PerProgram(int8u program_config, int8u program)
{
    switch (program_config)
    {
        case  0 :   switch (program)
                    {
                        case  0 :   return 6;
                        default :   return 2;
                    }
        case  1 :   switch (program)
                    {
                        case  0 :   return 6;
                        default :   return 1;
                    }
        case  2 :
        case 18 :   return 4;
        case  3 :
        case 12 :   switch (program)
                    {
                        case  0 :   return 4;
                        default :   return 2;
                    }
        case  4 :   switch (program)
                    {
                        case  0 :   return 4;
                        case  1 :   return 2;
                        default :   return 1;
                    }
        case  5 :
        case 13 :   switch (program)
                    {
                        case  0 :   return 4;
                        default :   return 1;
                    }
        case  6 :
        case 14 :
        case 19 :   return 2;
        case  7 :   switch (program)
                    {
                        case  0 :
                        case  1 :
                        case  2 :   return 2;
                        default :   return 1;
                    }
        case  8 :
        case 15 :   switch (program)
                    {
                        case  0 :
                        case  1 :   return 2;
                        default :   return 1;
                    }
        case  9 :
        case 16 :
        case 20 :   switch (program)
                    {
                        case  0 :   return 2;
                        default :   return 1;
                    }
        case 10 :
        case 17 :
        case 21 :   return 1;
        case 11 :   return 6;
        case 22 :   return 8;
        case 23 :   return 8;
        default :   return 0;
    }
};

//---------------------------------------------------------------------------
const char*  DolbyE_ChannelPositions[64]=
{
    "Front: L C R, Side: L R, LFE / Front: L R",
    "Front: L C R, Side: L R, LFE / Front: C / Front: C",
    "Front: L C R, LFE / Front: L C R, LFE",
    "Front: L C R, LFE / Front: L R / Front: L R",
    "Front: L C R, LFE / Front: L R / Front: C / Front: C",
    "Front: L C R, LFE / Front: C / Front: C / Front: C / Front: C",
    "Front: L R / Front: L R / Front: L R / Front: L R",
    "Front: L R / Front: L R / Front: L R / Front: C / Front: C",
    "Front: L R / Front: L R / Front: C / Front: C / Front: C / Front: C",
    "Front: L R / Front: C / Front: C / Front: C / Front: C / Front: C / Front: C",
    "Front: C / Front: C / Front: C / Front: C / Front: C / Front: C / Front: C / Front: C",
    "Front: L C R, Side: L R, LFE",
    "Front: L C R, LFE / Front: L R",
    "Front: L C R, LFE / Front: C / Front: C",
    "Front: L R / Front: L R / Front: L R",
    "Front: L R / Front: L R / Front: C / Front: C",
    "Front: L R / Front: C / Front: C / Front: C / Front: C",
    "Front: C / Front: C / Front: C / Front: C / Front: C / Front: C",
    "Front: L C R, LFE",
    "Front: L R / Front: L R",
    "Front: L R / Front: C / Front: C",
    "Front: C / Front: C / Front: C / Front: C",
    "Front: L C R, Side: L R, Back: L R, LFE",
    "Front: L C C C R, Side: L R, LFE",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
};

//---------------------------------------------------------------------------
const char*  DolbyE_ChannelPositions_PerProgram(int8u program_config, int8u program)
{
    switch (program_config)
    {
        case  0 :   switch (program)
                    {
                        case  0 :   return "Front: L C R, Side: L R, LFE";
                        default :   return "Front: L R";
                    }
        case  1 :   switch (program)
                    {
                        case  0 :   return "Front: L C R, Side: L R, LFE";
                        default :   return "Front: C";
                    }
        case  2 :
        case 18 :   return "Front: L C R, LFE";
        case  3 :
        case 12 :   switch (program)
                    {
                        case  0 :   return "Front: L C R, LFE";
                        default :   return "Front: L R";
                    }
        case  4 :   switch (program)
                    {
                        case  0 :   return "Front: L C R, LFE";
                        case  1 :   return "Front: L R";
                        default :   return "Front: C";
                    }
        case  5 :
        case 13 :   switch (program)
                    {
                        case  0 :   return "Front: L C R, LFE";
                        default :   return "Front: C";
                    }
        case  6 :
        case 14 :
        case 19 :   return "Front: L R";
        case  7 :   switch (program)
                    {
                        case  0 :
                        case  1 :
                        case  2 :   return "Front: L R";
                        default :   return "Front: C";
                    }
        case  8 :
        case 15 :   switch (program)
                    {
                        case  0 :
                        case  1 :   return "Front: L R";
                        default :   return "Front: C";
                    }
        case  9 :
        case 16 :
        case 20 :   switch (program)
                    {
                        case  0 :   return "Front: L R";
                        default :   return "Front: C";
                    }
        case 10 :
        case 17 :
        case 21 :   return "Front: C";
        case 11 :   return "Front: L C R, Side: L R, LFE";
        case 22 :   return "Front: L C R, Side: L R, Back: L R, LFE";
        case 23 :   return "Front: L C C C R, Side: L R, LFE";
        default :   return "";
    }
};

//---------------------------------------------------------------------------
const char*  DolbyE_ChannelPositions2[64]=
{
    "3/2/0.1 / 2/0/0",
    "3/2/0.1 / 1/0/0 / 1/0/0",
    "3/0/0.1 / 3/0/0.1",
    "3/0/0.1 / 2/0/0 / 2/0/0",
    "3/0/0.1 / 2/0/0 / 1/0/0 / 1/0/0",
    "3/0/0.1 / 1/0/0 / 1/0/0 / 1/0/0 / 1/0/0",
    "2/0/0 / 2/0/0 / 2/0/0 / 2/0/0",
    "2/0/0 / 2/0/0 / 2/0/0 / 1/0/0 / 1/0/0",
    "2/0/0 / 2/0/0 / 1/0/0 / 1/0/0 / 1/0/0 / 1/0/0",
    "2/0/0 / 1/0/0 / 1/0/0 / 1/0/0 / 1/0/0 / 1/0/0 / 1/0/0",
    "1/0/0 / 1/0/0 / 1/0/0 / 1/0/0 / 1/0/0 / 1/0/0 / 1/0/0 / 1/0/0",
    "3/2/0.1",
    "3/0/0.1 / 2/0/0",
    "3/0/0.1 / 1/0/0 / 1/0/0",
    "2/0/0 / 2/0/0 / 2/0/0",
    "2/0/0 / 2/0/0 / 1/0/0 / 1/0/0",
    "2/0/0 / 1/0/0 / 1/0/0 / 1/0/0 / 1/0/0",
    "1/0/0 / 1/0/0 / 1/0/0 / 1/0/0 / 1/0/0 / 1/0/0",
    "3/0/0.1",
    "2/0/0 / 2/0/0",
    "2/0/0 / 1/0/0 / 1/0/0",
    "1/0/0 / 1/0/0 / 1/0/0 / 1/0/0",
    "3/2/2.1",
    "5/2/0.1",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
};

//---------------------------------------------------------------------------
const char*  DolbyE_ChannelPositions2_PerProgram(int8u program_config, int8u program)
{
    switch (program_config)
    {
        case  0 :   switch (program)
                    {
                        case  0 :   return "3/2/0.1";
                        default :   return "2/0/0";
                    }
        case  1 :   switch (program)
                    {
                        case  0 :   return "3/2/0.1";
                        default :   return "1/0/0";
                    }
        case  2 :
        case 18 :   return "3/0/0.1";
        case  3 :
        case 12 :   switch (program)
                    {
                        case  0 :   return "3/0/0.1";
                        default :   return "2/0/0";
                    }
        case  4 :   switch (program)
                    {
                        case  0 :   return "3/0/0.1";
                        case  1 :   return "2/0/0";
                        default :   return "1/0/0";
                    }
        case  5 :
        case 13 :   switch (program)
                    {
                        case  0 :   return "3/0/0.1";
                        default :   return "1/0/0";
                    }
        case  6 :
        case 14 :
        case 19 :   return "Front: L R";
        case  7 :   switch (program)
                    {
                        case  0 :
                        case  1 :
                        case  2 :   return "2/0/0";
                        default :   return "1/0/0";
                    }
        case  8 :
        case 15 :   switch (program)
                    {
                        case  0 :
                        case  1 :   return "2/0/0";
                        default :   return "1/0/0";
                    }
        case  9 :
        case 16 :
        case 20 :   switch (program)
                    {
                        case  0 :   return "2/0/0";
                        default :   return "1/0/0";
                    }
        case 10 :
        case 17 :
        case 21 :   return "1/0/0";
        case 11 :   return "3/2/0.1";
        case 22 :   return "3/2/2.1";
        case 23 :   return "5/2/0.1";
        default :   return "";
    }
};

extern const char*  AC3_Surround[];

//---------------------------------------------------------------------------
const char*  DolbyE_ChannelLayout_PerProgram(int8u program_config, int8u ProgramNumber)
{
    switch (program_config)
    {
        case  0 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L C Ls X R LFE Rs X";
                        default :   return "X X X L X X X R";
                    }
        case  1 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L C Ls X R LFE Rs X";
                        case  1 :   return "X X X C X X X X";
                        default :   return "X X X X X X X C";
                    }
        case  2 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L C X X R S X X";
                        default :   return "X X L C X X R S";
                    }
        case  3 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L C X X R S X X";
                        case  1 :   return "X X L X X X R X";
                        default :   return "X X X L X X X R";
                    }
        case  4 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L C X X R S X X";
                        case  1 :   return "X X L X X X R X";
                        case  2 :   return "X X X C X X X X";
                        default :   return "X X X X X X X C";
                    }
        case  5 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L C X X R S X X";
                        case  1 :   return "X X C X X X X X";
                        case  2 :   return "X X X X X X C X";
                        case  3 :   return "X X X C X X X X";
                        default :   return "X X X X X X X C";
                    }
        case  6 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L X X X R X X X";
                        case  1 :   return "X L X X X R X X";
                        case  2 :   return "X X L X X X R X";
                        default :   return "X X X L X X X R";
                    }
        case  7 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L X X X R X X X";
                        case  1 :   return "X L X X X R X X";
                        case  2 :   return "X X L X X X R X";
                        case  3 :   return "X X X C X X X X";
                        default :   return "X X X X X X X C";
                    }
        case  8 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L X X X R X X X";
                        case  1 :   return "X L X X X R X X";
                        case  2 :   return "X X C X X X X X";
                        case  3 :   return "X X X X X X C X";
                        case  4 :   return "X X X C X X X X";
                        default :   return "X X X X X X X C";
                    }
        case  9 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L X X X R X X X";
                        case  1 :   return "X C X X X X X X";
                        case  2 :   return "X X X X X C X X";
                        case  3 :   return "X X C X X X X X";
                        case  4 :   return "X X X X X X C X";
                        case  5 :   return "X X X C X X X X";
                        default :   return "X X X X X X X C";
                    }
        case 10 :   switch (ProgramNumber)
                    {
                        case  0 :   return "C X X X X X X X";
                        case  1 :   return "X X X X C X X X";
                        case  2 :   return "X C X X X X X X";
                        case  3 :   return "X X X X X C X X";
                        case  4 :   return "X X C X X X X X";
                        case  5 :   return "X X X X X X C X";
                        case  6 :   return "X X X C X X X X";
                        default :   return "X X X X X X X C";
                    }
        case 11 :   return "L C Ls R LFE Rs";
        case 12 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L C X R S X";
                        default :   return "X X L X X R";
                    }
        case 13 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L C X R S X";
                        case  1 :   return "X X C X X X";
                        default :   return "X X X X X C";
                    }
        case 14 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L X X R X X";
                        case  1 :   return "X L X X R X";
                        default :   return "X X L X X R";
                    }
        case 15 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L X X R X X";
                        case  1 :   return "X L X R X";
                        case  2 :   return "X X C X X X";
                        default :   return "X X X X X C";
                    }
        case 16 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L X X R X X";
                        case  1 :   return "X C X X X X";
                        case  2 :   return "X X X X C X";
                        case  3 :   return "X X C X X X";
                        default :   return "X X X X X C";
                    }
        case 17 :   switch (ProgramNumber)
                    {
                        case  0 :   return "C X X X X X";
                        case  1 :   return "X X X C X X";
                        case  2 :   return "X C X X X X";
                        case  3 :   return "X X X X C X";
                        case  4 :   return "X X C X X X";
                        default :   return "X X X X X C";
                    }
        case 18 :   return "L C R S";
        case 19 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L X R X";
                        default :   return "X L X R";
                    }
        case 20 :   switch (ProgramNumber)
                    {
                        case  0 :   return "L X R X";
                        case  1 :   return "X C X X";
                        default :   return "X X X C";
                    }
        case 21 :   switch (ProgramNumber)
                    {
                        case  0 :   return "C X X X";
                        case  1 :   return "X X C X";
                        case  2 :   return "X C X X";
                        default :   return "X X X C";
                    }
        case 22 :   return "L C Ls Lrs R LFE Rs Rrs";
        case 23 :   return "L C Ls Lc R LFE Rs Rc";
        default :   return "";
    }
};

extern const float64 Mpegv_frame_rate[16];

const bool Mpegv_frame_rate_type[16]=
{false, false, false, false, false, false, true, true, true, false, false, false, false, false, false, false};


//---------------------------------------------------------------------------
static const int8u intermediate_spatial_format_object_count[8]=
{
    4,
    8,
    10,
    14,
    15,
    30,
    0,
    0,
};

const char* sound_category_Values[4]=
{
    "",
    "Dialog",
    "",
    "",
};
const char* hp_render_mode_Values[4]=
{
    "Bypassed",
    "Near",
    "Far",
    "",
};
string default_target_device_config_Value(int32u config)
{
    string Value;
    if (config&0x1)
        Value+="Stereo / ";
    if (config&0x2)
        Value+="Surround / ";
    if (config&0x4)
        Value+="Immersive / ";
    if (!Value.empty())
        Value.resize(Value.size()-3);
    return Value;
}
struct pos3d
{
    int8u x;
    int8u y;
    int8u z;
    const char* Value;
};
int16u mgi_6bit_unsigned_to_oari_Q15[0x40]=
{
      0,   529,  1057,  1586,  2114,  2643,  3171,  3700,
   4228,  4757,  5285,  5814,  6342,  6871,  7399,  7928,
   8456,  8985,  9513, 10042, 10570, 11099, 11627, 12156,
  12684, 13213, 13741, 14270, 14798, 15327, 15855, 16384,
  16913, 17441, 17970, 18498, 19027, 19555, 20084, 20612,
  21141, 21669, 22198, 22726, 23255, 23783, 24312, 24840,
  25369, 25897, 26426, 26954, 27483, 28011, 28540, 29068,
  29597, 30125, 30654, 31182, 31711, 32239, 32767, 32767
};

int16u mgi_4bit_unsigned_to_oari_Q15[0x10]=
{
      0,  2185,  4369,  6554,  8738, 10923, 13107, 15292,
  17476, 19661, 21845, 24030, 26214, 28399, 30583, 32767
};

int mgi_bitstream_val_to_Q15(int value, int8u num_bits)
{
    bool sign;
    if (value<0)
    {
        sign=true;
        value=-value;
    }
    else
        sign=false;

    int16u* Table;
    switch (num_bits)
    {
        case 4: Table=mgi_4bit_unsigned_to_oari_Q15; break;
        case 6: Table=mgi_6bit_unsigned_to_oari_Q15; break;
        default: return 0; //Problem
    }
    int decoded=Table[value];

    return sign?-decoded:decoded;
}

int mgi_bitstream_pos_z_to_Q15(bool pos_z_sign, int8u pos_z_bits)
{
    if (pos_z_bits == 0xf) {
        if (pos_z_sign == 1) /* +1 */ {
            return(32767);
        }
        else /* -1 */ {
            return(-32768);
        }
    }
    else {
        /* Negative number */
        if (pos_z_sign == 1) {
            return(mgi_bitstream_val_to_Q15(pos_z_bits, 4));
        }
        else {
            return(mgi_bitstream_val_to_Q15(-pos_z_bits, 4));
        }
    }
}

//---------------------------------------------------------------------------
namespace
{
struct speaker_info
{
    int8u AzimuthAngle; //0 to 180 (right, if AzimuthDirection is false) or 179 (left, if AzimuthDirection is true)
    int8s ElevationDirectionAngle; //-90 (bottom) to 90 (top)
    enum flag
    {
        None                            = 0,
        AzimuthDirection                = 1<<0, // true = right
        isLFE                           = 1<<1,
    };
    int8u Flags;
};
bool operator== (const speaker_info& L, const speaker_info& R)
{
    return L.AzimuthAngle==R.AzimuthAngle
        && L.ElevationDirectionAngle==R.ElevationDirectionAngle
        && L.Flags==R.Flags;
}
}
extern string Aac_ChannelLayout_GetString(const Aac_OutputChannel* const OutputChannels, size_t OutputChannels_Size);
static const size_t SpeakerInfos_Size=43;
static const speaker_info SpeakerInfos[SpeakerInfos_Size] =
{
    {  30,   0, speaker_info::AzimuthDirection },
    {  30,   0, speaker_info::None },
    {   0,   0, speaker_info::None },
    {   0, -15,                                 speaker_info::isLFE },
    { 110,   0, speaker_info::AzimuthDirection },
    { 110,   0, speaker_info::None },
    {  22,   0, speaker_info::AzimuthDirection },
    {  22,   0, speaker_info::None },
    { 135,   0, speaker_info::AzimuthDirection },
    { 135,   0, speaker_info::None },
    { 180,   0, speaker_info::None },
    { 135,   0, speaker_info::AzimuthDirection },
    { 135,   0, speaker_info::None },
    {  90,   0, speaker_info::AzimuthDirection },
    {  90,   0, speaker_info::None },
    {  60,   0, speaker_info::AzimuthDirection },
    {  60,   0, speaker_info::None },
    {  30,  35, speaker_info::AzimuthDirection },
    {  30,  35, speaker_info::None },
    {   0,  35, speaker_info::None },
    { 135,  35, speaker_info::AzimuthDirection },
    { 135,  35, speaker_info::None },
    { 180,  35, speaker_info::None },
    {  90,  35, speaker_info::AzimuthDirection },
    {  90,  35, speaker_info::None },
    {   0,  90, speaker_info::None },
    {  45, -15,                                 speaker_info::isLFE },
    {  45, -15, speaker_info::AzimuthDirection },
    {  45, -15, speaker_info::None },
    {   0, -15, speaker_info::None },
    { 110,  35, speaker_info::AzimuthDirection },
    { 110,  35, speaker_info::None },
    {  45,  35, speaker_info::AzimuthDirection },
    {  45,  35, speaker_info::None },
    {  45,   0, speaker_info::AzimuthDirection },
    {  45,   0, speaker_info::None },
    {  45, -15, speaker_info::None | speaker_info::isLFE },
    {   2,   0, speaker_info::AzimuthDirection },
    {   2,   0, speaker_info::None },
    {   1,   0, speaker_info::AzimuthDirection },
    {   1,   0, speaker_info::None },
    { 150,   0, speaker_info::AzimuthDirection },
    { 150,   0, speaker_info::None }, 
};

struct angles
{
    int Azimuth;
    int Elevation;

    angles()
    {}

    angles(int theta, int phi) :
        Azimuth(theta),
        Elevation(phi)
    {}
};
Aac_OutputChannel AnglesToChannelName(angles Angles)
{
    speaker_info SpeakerInfo;
    if (Angles.Azimuth<0)
    {
        SpeakerInfo.AzimuthAngle=(int8u)-Angles.Azimuth;
        SpeakerInfo.Flags=speaker_info::AzimuthDirection;
    }
    else
    {
        SpeakerInfo.AzimuthAngle=Angles.Azimuth;
        SpeakerInfo.Flags=speaker_info::None;
    }
    SpeakerInfo.ElevationDirectionAngle=Angles.Elevation;

    for (size_t i=0; i<SpeakerInfos_Size; i++)
        if (SpeakerInfos[i]==SpeakerInfo)
            return (Aac_OutputChannel)i;
    return CH_MAX;
}
string ToAngle3Digits(int Value)
{
    string ToReturn=Ztring::ToZtring(Value).To_UTF8();
    ToReturn.insert(0, 3-ToReturn.size(), '0');
    return ToReturn;
}
angles mgi_bitstream_pos_ToAngles(int x_int, int y_int, int z_int)
{
    //Center becomes 0 and edges +/-1
    float x=(((float)x_int)*2-32768)/32768;
    float y=(((float)y_int)*2-32768)/32768;
    float z=((float)z_int)/32768;

    if (!x && !y)
    {
        if (z>0)
            return angles(0, 90);
        else if (z<0)
            return angles(0, -90);
        return angles(0, 0); // Should not happen (is the center in practice)
    }
    else
    {
        angles ToReturn;
        
        float radius=sqrt(x*x+y*y+z*z);
        float theta_float=round(atan2(y, x)*180/3.14159265359/5)*5;
        ToReturn.Azimuth=float32_int32s(theta_float);
        float phi_float=round(acos(z/radius)*180/3.14159265359);
        ToReturn.Elevation=float32_int32s(phi_float);

        //Reference is front of viewer
        if (ToReturn.Azimuth<90)
            ToReturn.Azimuth+=90;
        else
            ToReturn.Azimuth-=270;
        ToReturn.Elevation=90-ToReturn.Elevation;

        return ToReturn;
    }
}

string Angles2String(angles Angles)
{
    string ToReturn;

    //Elevation
    switch (Angles.Elevation)
    {
        case   0:   ToReturn+='M'; break;
        case  90:   ToReturn+='T'; break;
        case -90:   ToReturn+='X'; break; //No standard for that
        default :   ToReturn+=(Angles.Elevation>0?'U':'B');
                    //if (Angles.phi!=35 && Angles.phi!=-15)
                        ToReturn+=ToAngle3Digits(Angles.Elevation);
    }

    ToReturn+='_';

    //Azimuth
    if (Angles.Azimuth<0)
        ToReturn+='L';
    else if (Angles.Azimuth>0 && Angles.Azimuth!=180)
        ToReturn+='R';
    ToReturn+=ToAngle3Digits(Angles.Azimuth<0?-Angles.Azimuth:Angles.Azimuth);

    return ToReturn;
}

string Angles2KnownChannelName(angles& Angles)
{
    //Exception handling
    int Azimuth;
    if (Angles.Azimuth==-180)
        Azimuth=180;
    else
        Azimuth=Angles.Azimuth;
    int Elevation;
    if (Angles.Elevation>=35 && Angles.Elevation<=45)
        Elevation=35;
    else
        Elevation=Angles.Elevation;

    Aac_OutputChannel KnownChannel=AnglesToChannelName(angles(Azimuth, Elevation));
    if (KnownChannel!=CH_MAX)
        return Aac_ChannelLayout_GetString(&KnownChannel, 1);
    else
        return Angles2String(Angles);
}

//---------------------------------------------------------------------------
extern int32u AC3_bed_channel_assignment_mask_2_nonstd(int16u bed_channel_assignment_mask);
extern Ztring AC3_nonstd_bed_channel_assignment_mask_ChannelLayout(int32u nonstd_bed_channel_assignment_mask);
size_t BedChannelConfiguration_ChannelCount(int32u nonstd_bed_channel_assignment_mask)
{
    Ztring BedChannelConfiguration=AC3_nonstd_bed_channel_assignment_mask_ChannelLayout(nonstd_bed_channel_assignment_mask);
    size_t BedChannelCount=0;
    if (!BedChannelConfiguration.empty())
        for (size_t i=0; i<BedChannelConfiguration.size();)
        {
            BedChannelCount++;
            i=BedChannelConfiguration.find(__T(' '), i+1);
        }
    return BedChannelCount;
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_DolbyE::File_DolbyE()
:File__Analyze()
{
    //Configuration
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_DolbyE;
    #endif //MEDIAINFO_EVENTS

    //Configuration
    MustSynchronize=true;
    Buffer_TotalBytes_FirstSynched_Max=64*1024;

    //In
    GuardBand_Before=0;

    //Out
    GuardBand_After=0;

    //ED2
    Guardband_EMDF_PresentAndSize=0;
    num_desc_packets_m1=(int32u)-1;

    //Temp
    SMPTE_time_code_StartTimecode=(int64u)-1;
    frame_rate_code=0;
    FrameInfo.DTS=0;
    program_config=0;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_DolbyE::Streams_Fill()
{
    Fill(Stream_General, 0, General_Format, "Dolby E");

    if (!Presets.empty())
    {
        Streams_Fill_ED2();
        return;
    }

    int8u DolbyE_Audio_Pos=0;
    for (size_t i=0; i<8; i++)
        if (channel_subsegment_sizes[i].size()>1)
            DolbyE_Audio_Pos=(int8u)-1;
    for (int8u program=0; program<DolbyE_Programs[program_config]; program++)
    {
        if (program >= Count_Get(Stream_Audio))
            Stream_Prepare(Stream_Audio);
        Fill(Stream_Audio, program, Audio_Format, "Dolby E");
        if (DolbyE_Programs[program_config]>1)
            Fill(Stream_Audio, program, Audio_ID, program+1);
        Fill(Stream_Audio, program, Audio_Channel_s_, DolbyE_Channels_PerProgram(program_config, program));
        Fill(Stream_Audio, program, Audio_ChannelPositions, DolbyE_ChannelPositions_PerProgram(program_config, program));
        Fill(Stream_Audio, program, Audio_ChannelPositions_String2, DolbyE_ChannelPositions2_PerProgram(program_config, program));
        Fill(Stream_Audio, program, Audio_ChannelLayout, DolbyE_ChannelLayout_PerProgram(program_config, program));
        int32u Program_Size=0;
        if (DolbyE_Audio_Pos!=(int8u)-1)
            for (int8u Pos=0; Pos<DolbyE_Channels_PerProgram(program_config, program); Pos++)
                Program_Size+=channel_subsegment_size[DolbyE_Audio_Pos+Pos];
        if (!Mpegv_frame_rate_type[frame_rate_code])
            Program_Size*=2; //Low bit rate, 2 channel component per block
        Program_Size*=bit_depth;
        Fill(Stream_Audio, program, Audio_BitRate, Program_Size*Mpegv_frame_rate[frame_rate_code], 0);
        if (DolbyE_Audio_Pos!=(int8u)-1)
            DolbyE_Audio_Pos+=DolbyE_Channels_PerProgram(program_config, program);
        Streams_Fill_PerProgram(program);

        if (program<description_text_Values.size())
        {
            Fill(Stream_Audio, program, Audio_Title, description_text_Values[program].Previous);
            Fill(Stream_Audio, program, "Title_FromStream", description_text_Values[program].Previous);
            Fill_SetOptions(Stream_Audio, program, "Title_FromStream", "N NT");
        }
    }
}

//---------------------------------------------------------------------------
void File_DolbyE::Streams_Fill_PerProgram(size_t program)
{
    Fill(Stream_Audio, program, Audio_SamplingRate, 48000);
    Fill(Stream_Audio, program, Audio_BitDepth, bit_depth);

    if (SMPTE_time_code_StartTimecode!=(int64u)-1)
    {
        Fill(Stream_Audio, program, Audio_Delay, SMPTE_time_code_StartTimecode);
        Fill(Stream_Audio, program, Audio_Delay_Source, "Stream");
    }

    Fill(Stream_Audio, program, Audio_FrameRate, Mpegv_frame_rate[frame_rate_code]);
    if (bit_depth)
    {
        float BitRate=(float)(96000*bit_depth);

        if (GuardBand_Before_Initial)
        {
            float GuardBand_Before_Initial_Duration=GuardBand_Before_Initial*8/BitRate;
            Fill(Stream_Audio, program, "GuardBand_Before", GuardBand_Before_Initial_Duration, 9);
            Fill(Stream_Audio, program, "GuardBand_Before/String", Ztring::ToZtring(GuardBand_Before_Initial_Duration*1000000, 0)+Ztring().From_UTF8(" \xC2\xB5s")); //0xC2 0xB5 = micro sign
            Fill_SetOptions(Stream_Audio, program, "GuardBand_Before", "N NT");
            Fill_SetOptions(Stream_Audio, program, "GuardBand_Before/String", "Y NT");

            float GuardBand_After_Initial_Duration=GuardBand_After_Initial*8/BitRate;
            Fill(Stream_Audio, program, "GuardBand_After", GuardBand_After_Initial_Duration, 9);
            Fill(Stream_Audio, program, "GuardBand_After/String", Ztring::ToZtring(GuardBand_After_Initial_Duration*1000000, 0)+Ztring().From_UTF8(" \xC2\xB5s")); //0xC2 0xB5 = micro sign
            Fill_SetOptions(Stream_Audio, program, "GuardBand_After", "N NT");
            Fill_SetOptions(Stream_Audio, program, "GuardBand_After/String", "Y NT");
        }
    }

    if (FrameSizes.size()==1)
    {
        if (program)
            Fill(Stream_Audio, program, Audio_BitRate_Encoded, 0, 0, true);
        else
        {
            Fill(Stream_General, 0, General_OverallBitRate, FrameSizes.begin()->first*8*Mpegv_frame_rate[frame_rate_code], 0);
            Fill(Stream_Audio, 0, Audio_BitRate_Encoded, FrameSizes.begin()->first*8*Mpegv_frame_rate[frame_rate_code], 0);
        }
    }
}

//---------------------------------------------------------------------------
void File_DolbyE::Streams_Fill_ED2()
{
    if (Count_Get(Stream_Audio))
    {
        while (Count_Get(Stream_Audio)>1)
            Stream_Erase(Stream_Audio, Count_Get(Stream_Audio)-1); // We may have several streams due to metadata, we keep one
        StreamPos_Last=0;
    }
    else
        Stream_Prepare(Stream_Audio);
    Fill(Stream_Audio, StreamPos_Last, Audio_Format, "Dolby ED2");
    if (Guardband_EMDF_PresentAndSize)
        Fill(Stream_Audio, StreamPos_Last, Audio_BitRate, Guardband_EMDF_PresentAndSize*8*Mpegv_frame_rate[frame_rate_code], 0);
    size_t ChannelCount=DynObjects.size();
    for (size_t p=0; p<BedInstances.size(); p++)
        if (p<nonstd_bed_channel_assignment_masks.size())
            ChannelCount+=BedChannelConfiguration_ChannelCount(nonstd_bed_channel_assignment_masks[p]);
    if (ChannelCount)
        Fill(Stream_Audio, StreamPos_Last, Audio_Channel_s_, ChannelCount);
    Streams_Fill_PerProgram(StreamPos_Last);

    if (!Presets.empty())
    {
        Fill(Stream_Audio, 0, "NumberOfPresentations", Presets.size());
        Fill_SetOptions(Stream_Audio, 0, "NumberOfPresentations", "N NIY");
    }
    if (!BedInstances.empty())
    {
        Fill(Stream_Audio, 0, "NumberOfBeds", BedInstances.size());
        Fill_SetOptions(Stream_Audio, 0, "NumberOfBeds", "N NIY");
    }
    if (!DynObjects.empty())
    {
        Fill(Stream_Audio, 0, "NumberOfObjects", DynObjects.size());
        Fill_SetOptions(Stream_Audio, 0, "NumberOfObjects", "N NIY");
    }

    for (size_t p=0; p<Presets.size(); p++)
    {
        const preset& Presentation_Current=Presets[p];

        string Title;
        if (p<Presets_More.size() && !Presets_More[p].description.empty())
            Title=Presets_More[p].description;
        
        string Summary=Title;
        if (Summary.empty())
            Summary="Yes";

        string P="Presentation"+Ztring::ToZtring(p).To_UTF8();
        Fill(Stream_Audio, 0, P.c_str(), Summary);
        if (!Title.empty())
        {
            Fill(Stream_Audio, 0, (P+" Title").c_str(), Title);
            Fill_SetOptions(Stream_Audio, 0, (P+" Title").c_str(), "N NTY");
        }
        Fill(Stream_Audio, 0, (P+" DefaultTargetDeviceConfig").c_str(), default_target_device_config_Value(Presentation_Current.default_target_device_config));
        Fill_SetOptions(Stream_Audio, 0, (P+" DefaultTargetDeviceConfig").c_str(), "Y NTY");

        if (!Presentation_Current.target_device_configs.empty())
        {
            Fill(Stream_Audio, 0, (P+" NumberOfTargets").c_str(), Presentation_Current.target_device_configs.size());
            Fill_SetOptions(Stream_Audio, 0, (P+" NumberOfTargets").c_str(), "N NIY");
        }
        for (size_t t=0; t<Presentation_Current.target_device_configs.size(); t++)
        {
            const preset::target_device_config& Target_Current=Presentation_Current.target_device_configs[t];
            
            string Summary2=default_target_device_config_Value(Target_Current.target_devices_mask);
            if (Summary2.empty())
                Summary2="Yes";

            string T=P+" Target"+Ztring::ToZtring(t).To_UTF8();
            Fill(Stream_Audio, 0, T.c_str(), Summary2);

            Fill(Stream_Audio, 0, (T+" TargetDeviceConfig").c_str(), default_target_device_config_Value(Target_Current.target_devices_mask));
            Fill_SetOptions(Stream_Audio, 0, (T+" TargetDeviceConfig").c_str(), "N NTY");

            ZtringList GroupPos[2], GroupNum[2];
            for (size_t i=0; i<Target_Current.md_indexes.size(); i++)
            {
                if (Target_Current.md_indexes[i]!=(int32u)-1)
                {
                    size_t I1, I2;
                    if (i<DynObjects.size())
                    {
                        I1=i;
                        I2=Target_Current.md_indexes[i];
                        if (I2 && I2>DynObjects[I1].Alts.size()) // 0 = OAMD, else Alt - 1
                            I2=0; // There is a problem
                    }
                    else
                    {
                        I1=i-DynObjects.size();
                        I2=Target_Current.md_indexes[i];
                        if (I2 && I2>BedInstances[I1].Alts.size()) // 0 = OAMD, else Alt - 1
                            I2=0; // There is a problem
                    }

                    int j=i<DynObjects.size()?1:0;
                    GroupPos[j].push_back(Ztring::ToZtring(I1));
                    GroupNum[j].push_back(Ztring::ToZtring(I1+1));
                    if (I2 )
                    {
                        GroupPos[j][GroupPos[j].size()-1]+=__T('-');
                        GroupNum[j][GroupNum[j].size()-1]+=__T('-');
                        GroupPos[j][GroupPos[j].size()-1]+=Ztring::ToZtring(I2-1);
                        GroupNum[j][GroupNum[j].size()-1]+=Ztring::ToZtring(I2);
                    }
                }
            }

            for (size_t i=0; i<2; i++)
            {
                GroupPos[i].Separator_Set(0, __T(" + "));
                Fill(Stream_Audio, 0, (T+(!i?" LinkedTo_Bed_Pos":" LinkedTo_Object_Pos")).c_str(), GroupPos[i].Read());
                Fill_SetOptions(Stream_Audio, 0, (T+(!i?" LinkedTo_Bed_Pos":" LinkedTo_Object_Pos")).c_str(), "N NIY");
                GroupNum[i].Separator_Set(0, __T(" + "));
                Fill(Stream_Audio, 0, (T+(!i?" LinkedTo_Bed_Pos/String":" LinkedTo_Object_Pos/String")).c_str(), GroupNum[i].Read());
                Fill_SetOptions(Stream_Audio, 0, (T+(!i?" LinkedTo_Bed_Pos/String":" LinkedTo_Object_Pos/String")).c_str(), "Y NIN");
            }
        }
    }

    // Computing the count of objects in bed instances
    size_t Bed_Object_Count=0;

    // Beds
    for (size_t p=0; p<BedInstances.size(); p++)
    {
        const bed_instance& BedInstance=BedInstances[p];

        string Summary;
        if (Summary.empty())
            Summary=AC3_nonstd_bed_channel_assignment_mask_ChannelLayout(nonstd_bed_channel_assignment_masks[p]).To_UTF8();
        if (Summary.empty())
            Summary="Yes";

        string P=Ztring(__T("Bed")+Ztring::ToZtring(p)).To_UTF8();
        Fill(Stream_Audio, 0, P.c_str(), Summary);

        if (p<nonstd_bed_channel_assignment_masks.size())
        {
            Fill(Stream_Audio, StreamPos_Last, (P+" Channel(s)").c_str(), BedChannelConfiguration_ChannelCount(nonstd_bed_channel_assignment_masks[p]));
            Fill_SetOptions(Stream_Audio, 0, (P+" Channel(s)").c_str(), "Y NTY");
            Fill(Stream_Audio, 0, (P+" ChannelLayout").c_str(), AC3_nonstd_bed_channel_assignment_mask_ChannelLayout(nonstd_bed_channel_assignment_masks[p]));
            Fill_SetOptions(Stream_Audio, 0, (P+" ChannelLayout").c_str(), "Y NTY");

            // From OAMD
            if (Bed_Object_Count<ObjectElements.size() && !ObjectElements[Bed_Object_Count].Alts.empty())
            {
                string A=P;

                // Computing the count of objects in bed instances
                size_t ThisBed_Object_Count=BedChannelConfiguration_ChannelCount(nonstd_bed_channel_assignment_masks[p]);

                if (ThisBed_Object_Count)
                {
                    // Check if all same
                    int8s obj_gain_db_Ref=ObjectElements[Bed_Object_Count].Alts[0].obj_gain_db;
                    for (size_t o=1; o<ThisBed_Object_Count; o++)
                    {
                        int8s obj_gain_db=ObjectElements[Bed_Object_Count+o].Alts[0].obj_gain_db;
                        if (obj_gain_db!=obj_gain_db_Ref)
                        {
                            obj_gain_db_Ref=INT8_MAX;
                            break;
                        }
                    }
                    if (obj_gain_db_Ref!=INT8_MAX)
                    {
                        // Same
                        if (obj_gain_db_Ref==INT8_MIN)
                            Fill_Measure(Stream_Audio, 0, (A+" Gain").c_str(), string("-infinite"), " dB");
                        else
                            Fill_Measure(Stream_Audio, 0, (A+" Gain").c_str(), obj_gain_db_Ref, " dB");
                    }
                    else
                    {
                        // Not same
                        for (size_t o=0; o<ThisBed_Object_Count; o++)
                        {
                            const dyn_object::dyn_object_alt& DynObject_Current=ObjectElements[Bed_Object_Count+o].Alts[0];
                            if (DynObject_Current.obj_gain_db!=INT8_MAX)
                            {
                                if (DynObject_Current.obj_gain_db==INT8_MIN)
                                    Fill_Measure(Stream_Audio, 0, (A+" Gain").c_str(), string("-infinite"), " dB");
                                else
                                    Fill_Measure(Stream_Audio, 0, (A+" Gain").c_str(), DynObject_Current.obj_gain_db, " dB");
                            }
                        }
                    }

                }
                Bed_Object_Count+=ThisBed_Object_Count;
            }
        }

        if (!BedInstance.Alts.empty())
        {
            Fill(Stream_Audio, 0, (P+" NumberOfAlts").c_str(), BedInstance.Alts.size());
            Fill_SetOptions(Stream_Audio, 0, (P+" NumberOfAlts").c_str(), "N NIY");
        }
        for (size_t a=0; a<BedInstance.Alts.size(); a++)
        {
            const bed_instance::bed_alt& BedInstance_Current=BedInstance.Alts[a];

            string Summary2;
            if (Summary2.empty())
                Summary2="Yes";

            string A=P+Ztring(__T(" Alt")+Ztring::ToZtring(a)).To_UTF8();
            Fill(Stream_Audio, 0, A.c_str(), Summary2);
            if (BedInstance_Current.bed_gain_db!=INT8_MAX)
            {
                Fill_Measure(Stream_Audio, 0, (A+" Gain").c_str(), BedInstance_Current.bed_gain_db, " dB");
            }
        }

        const substream_mapping& S=substream_mappings[DynObjects.size()+p];
        {
            Fill(Stream_Audio, 0, (P+" SubtstreamIdChannel").c_str(), Ztring::ToZtring(S.substream_id)+__T('-')+Ztring::ToZtring(S.channel_index));
            Fill_SetOptions(Stream_Audio, 0, (P+" SubtstreamIdChannel").c_str(), "Y NTY");
        }
    }

    // Dynamic objects
    for (size_t p=0; p<DynObjects.size(); p++)
    {
        const dyn_object& DynObject=DynObjects[p];
        string P=Ztring(__T("Object")+Ztring::ToZtring(p)).To_UTF8();

        // From OAMD
        Ztring Position_Cartesian;
        string Position_Polar;
        string ChannelLayout;
        if (Bed_Object_Count<ObjectElements.size() && p<ObjectElements.size()-Bed_Object_Count && !ObjectElements[Bed_Object_Count+p].Alts.empty())
        {
            string A=P;
            const dyn_object::dyn_object_alt& DynObject_Current=ObjectElements[Bed_Object_Count+p].Alts[0];
            if (DynObject_Current.pos3d_x_bits!=(int8u)-1)
            {
                int x_32k=mgi_bitstream_val_to_Q15(DynObject_Current.pos3d_x_bits, 6);
                int y_32k=mgi_bitstream_val_to_Q15(DynObject_Current.pos3d_y_bits, 6);
                int z_32k=mgi_bitstream_pos_z_to_Q15(DynObject_Current.pos3d_z_sig, DynObject_Current.pos3d_z_bits);
                Position_Cartesian=__T("x=")+Ztring::ToZtring((((float)x_32k)*2-32768)/32768, 1)+__T(" y=")+Ztring::ToZtring((((float)y_32k)*2-32768)/32768, 1)+__T(" z=")+Ztring::ToZtring(((float)z_32k)/32768, 1);
                angles Angles=mgi_bitstream_pos_ToAngles(x_32k, y_32k, z_32k);
                Position_Polar=Angles2String(Angles);
                ChannelLayout=Angles2KnownChannelName(Angles);
                if (ChannelLayout==Position_Polar)
                    ChannelLayout.clear();
            }
        }

        string Summary;
        if (Summary.empty())
            Summary=sound_category_Values[DynObject.sound_category];
        /*
        if (Summary.empty())
            Summary=ChannelLayout;
        if (Summary.empty())
            Summary=Position_Polar;
        */
        if (Summary.empty())
            Summary="Yes";

        Fill(Stream_Audio, 0, P.c_str(), Summary);

        if (sound_category_Values[DynObject.sound_category][0])
        {
            Fill(Stream_Audio, 0, (P+" SoundCategory").c_str(), sound_category_Values[DynObject.sound_category]);
            Fill_SetOptions(Stream_Audio, 0, (P+" SoundCategory").c_str(), "Y NTY");
        }

        // From OAMD
        if (Bed_Object_Count<ObjectElements.size() && p<ObjectElements.size()-Bed_Object_Count && !ObjectElements[Bed_Object_Count+p].Alts.empty())
        {
            string A=P;
            const dyn_object::dyn_object_alt& DynObject_Current=ObjectElements[Bed_Object_Count+p].Alts[0];
            /*
            if (!ChannelLayout.empty())
            {
                Fill(Stream_Audio, 0, (A+" ChannelLayout").c_str(), ChannelLayout);
                Fill_SetOptions(Stream_Audio, 0, (A+" ChannelLayout").c_str(), "Y NTY");
            }
            */
            if (!Position_Polar.empty())
            {
                Fill(Stream_Audio, 0, (A+" Position_Polar").c_str(), Position_Polar);
                Fill_SetOptions(Stream_Audio, 0, (A+" Position_Polar").c_str(), "Y NTY");
            }
            if (!Position_Cartesian.empty())
            {
                Fill(Stream_Audio, 0, (A+" Position_Cartesian").c_str(), Position_Cartesian);
                Fill_SetOptions(Stream_Audio, 0, (A+" Position_Cartesian").c_str(), "Y NTY");
            }
            if (DynObject_Current.obj_gain_db!=INT8_MAX)
            {
                if (DynObject_Current.obj_gain_db==INT8_MIN)
                    Fill_Measure(Stream_Audio, 0, (A+" Gain").c_str(), string("-infinite"), " dB");
                else
                    Fill_Measure(Stream_Audio, 0, (A+" Gain").c_str(), DynObject_Current.obj_gain_db, " dB");
            }
        }

        if (!DynObject.Alts.empty())
        {
            Fill(Stream_Audio, 0, (P+" NumberOfAlts").c_str(), DynObject.Alts.size());
            Fill_SetOptions(Stream_Audio, 0, (P+" NumberOfAlts").c_str(), "N NIY");
        }
        for (size_t a=0; a<DynObject.Alts.size(); a++)
        {
            const dyn_object::dyn_object_alt& DynObject_Current=DynObject.Alts[a];
            Ztring Position_Cartesian;
            string Position_Polar;
            string ChannelLayout;
            if (DynObject_Current.pos3d_x_bits!=(int8u)-1)
            {
                int x_32k=mgi_bitstream_val_to_Q15(DynObject_Current.pos3d_x_bits, 6);
                int y_32k=mgi_bitstream_val_to_Q15(DynObject_Current.pos3d_y_bits, 6);
                int z_32k=mgi_bitstream_pos_z_to_Q15(DynObject_Current.pos3d_z_sig, DynObject_Current.pos3d_z_bits);
                Position_Cartesian=__T("x=")+Ztring::ToZtring((((float)x_32k)*2-32768)/32768, 1)+__T(" y=")+Ztring::ToZtring((((float)y_32k)*2-32768)/32768, 1)+__T(" z=")+Ztring::ToZtring(((float)z_32k)/32768, 1);
                angles Angles=mgi_bitstream_pos_ToAngles(x_32k, y_32k, z_32k);
                Position_Polar=Angles2String(Angles);
                ChannelLayout=Angles2KnownChannelName(Angles);
                if (ChannelLayout==Position_Polar)
                    ChannelLayout.clear();
            }
            string Summary2;
            if (Summary2.empty())
                Summary2=sound_category_Values[DynObject.sound_category];
            /*
            if (Summary2.empty())
                Summary2=ChannelLayout;
            if (Summary2.empty())
                Summary2=Position_Polar;
            */
            if (Summary2.empty())
                Summary2="Yes";

            string A=P+Ztring(__T(" Alt")+Ztring::ToZtring(a)).To_UTF8();
            Fill(Stream_Audio, 0, A.c_str(), Summary2);
            /*
            if (!ChannelLayout.empty())
            {
                Fill(Stream_Audio, 0, (A+" ChannelLayout").c_str(), ChannelLayout);
                Fill_SetOptions(Stream_Audio, 0, (A+" ChannelLayout").c_str(), "Y NTY");
            }
            */
            if (!Position_Polar.empty())
            {
                Fill(Stream_Audio, 0, (A+" Position_Polar").c_str(), Position_Polar);
                Fill_SetOptions(Stream_Audio, 0, (A+" Position_Polar").c_str(), "Y NTY");
            }
            if (!Position_Cartesian.empty())
            {
                Fill(Stream_Audio, 0, (A+" Position_Cartesian").c_str(), Position_Cartesian);
                Fill_SetOptions(Stream_Audio, 0, (A+" Position_Cartesian").c_str(), "Y NTY");
            }
            if (DynObject_Current.obj_gain_db!=INT8_MAX)
            {
                if (DynObject_Current.obj_gain_db==INT8_MIN)
                    Fill_Measure(Stream_Audio, 0, (A+" Gain").c_str(), string("-infinite"), " dB");
                else
                    Fill_Measure(Stream_Audio, 0, (A+" Gain").c_str(), DynObject_Current.obj_gain_db, " dB");
            }
            if (DynObject_Current.hp_render_mode!=(int8u)-1)
            {
                Fill(Stream_Audio, 0, (A+" RenderMode").c_str(), hp_render_mode_Values[DynObject_Current.hp_render_mode]);
                Fill_SetOptions(Stream_Audio, 0, (P + " RenderMode").c_str(), "Y NTY");
            }
        }

        const substream_mapping& S=substream_mappings[p];
        {
            Fill(Stream_Audio, 0, (P+" SubtstreamIdChannel").c_str(), Ztring::ToZtring(S.substream_id)+__T('-')+Ztring::ToZtring(S.channel_index));
            Fill_SetOptions(Stream_Audio, 0, (P+" SubtstreamIdChannel").c_str(), "Y NTY");
        }
    }
}

//---------------------------------------------------------------------------
void File_DolbyE::Streams_Finish()
{
    if (FrameInfo.PTS!=(int64u)-1 && FrameInfo.PTS>PTS_Begin)
    {
        int64s Duration=float64_int64s(((float64)(FrameInfo.PTS-PTS_Begin))/1000000);
        int64s FrameCount;
        if (Mpegv_frame_rate[frame_rate_code])
            FrameCount=float64_int64s(((float64)(FrameInfo.PTS-PTS_Begin))/1000000000*Mpegv_frame_rate[frame_rate_code]);
        else
            FrameCount=0;

        for (size_t Pos=0; Pos<Count_Get(Stream_Audio); Pos++)
        {
            Fill(Stream_Audio, Pos, Audio_Duration, Duration);
            if (FrameCount)
                Fill(Stream_Audio, Pos, Audio_FrameCount, FrameCount);
        }
    }
}

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
bool File_DolbyE::Synchronize()
{
    //Synchronizing
    while (Buffer_Offset+3<=Buffer_Size)
    {
        if ((CC2(Buffer+Buffer_Offset_Temp)&0xFFFE)==0x078E) //16-bit
        {
            bit_depth=16;
            key_present=(CC2(Buffer+Buffer_Offset)&0x0001)?true:false;
            break; //while()
        }
        if ((CC3(Buffer+Buffer_Offset)&0xFFFFE0)==0x0788E0) //20-bit
        {
            bit_depth=20;
            key_present=(CC3(Buffer+Buffer_Offset)&0x000010)?true:false;
            break; //while()
        }
        if ((CC3(Buffer+Buffer_Offset)&0xFFFFFE)==0x07888E) //24-bit
        {
            bit_depth=24;
            key_present=(CC3(Buffer+Buffer_Offset)&0x000001)?true:false;
            break; //while()
        }
        Buffer_Offset++;
    }

    //Parsing last bytes if needed
    if (Buffer_Offset+3>Buffer_Size)
        return false;

    //Synched
    return true;
}

//---------------------------------------------------------------------------
bool File_DolbyE::Synched_Test()
{
    //Must have enough buffer for having header
    if (Buffer_Offset+3>Buffer_Size)
        return false;

    //Quick test of synchro
    switch (bit_depth)
    {
        case 16 : if ((CC2(Buffer+Buffer_Offset)&0xFFFE  )!=0x078E  ) {Synched=false; return true;} break;
        case 20 : if ((CC3(Buffer+Buffer_Offset)&0xFFFFE0)!=0x0788E0) {Synched=false; return true;} break;
        case 24 : if ((CC3(Buffer+Buffer_Offset)&0xFFFFFE)!=0x07888E) {Synched=false; return true;} break;
        default : ;
    }

    //We continue
    return true;
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_DolbyE::Read_Buffer_Unsynched()
{
    description_text_Values.clear();
    num_desc_packets_m1=(int32u)-1;
    description_packet_data.clear();
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_DolbyE::Header_Parse()
{
    //Filling
    if (IsSub)
        Header_Fill_Size(Buffer_Size-Buffer_Offset);
    else
    {
        //Looking for synchro
        //Synchronizing
        Buffer_Offset_Temp=Buffer_Offset+3;
        if (bit_depth==16)
            while (Buffer_Offset_Temp+2<=Buffer_Size)
            {
                if ((CC2(Buffer+Buffer_Offset_Temp)&0xFFFE)==0x078E) //16-bit
                    break; //while()
                Buffer_Offset_Temp++;
            }
        if (bit_depth==20)
            while (Buffer_Offset_Temp+3<=Buffer_Size)
            {
                if ((CC3(Buffer+Buffer_Offset_Temp)&0xFFFFE0)==0x0788E0) //20-bit
                    break; //while()
                Buffer_Offset_Temp++;
            }
        if (bit_depth==24)
            while (Buffer_Offset_Temp+3<=Buffer_Size)
            {
                if ((CC3(Buffer+Buffer_Offset_Temp)&0xFFFFFE)==0x07888E) //24-bit
                    break; //while()
                Buffer_Offset_Temp++;
            }

        if (Buffer_Offset_Temp+(bit_depth>16?3:2)>Buffer_Size)
        {
            if (File_Offset+Buffer_Size==File_Size)
                Buffer_Offset_Temp=Buffer_Size;
            else
            {
                Element_WaitForMoreData();
                return;
            }
        }

        Header_Fill_Size(Buffer_Offset_Temp-Buffer_Offset);
    }
    Header_Fill_Code(0, "Dolby_E_frame");
}

//---------------------------------------------------------------------------
void File_DolbyE::Data_Parse()
{
    FrameSizes[Element_Size]++;

    //In case of scrambling
    const int8u*    Save_Buffer=NULL;
    size_t          Save_Buffer_Offset=0;
    int64u          Save_File_Offset=0;
    if (key_present)
    {
        //We must change the buffer,
        Save_Buffer=Buffer;
        Save_Buffer_Offset=Buffer_Offset;
        Save_File_Offset=File_Offset;
        File_Offset+=Buffer_Offset;
        Buffer_Offset=0;
        Descrambled_Buffer=new int8u[(size_t)Element_Size];
        std::memcpy(Descrambled_Buffer, Save_Buffer+Save_Buffer_Offset, (size_t)Element_Size);
        Buffer=Descrambled_Buffer;
    }

    //Parsing
    BS_Begin();
    sync_segment();
    metadata_segment();
    audio_segment();
    metadata_extension_segment();
    audio_extension_segment();
    meter_segment();
    BS_End();

    //Check if there is content in padding
    int16u Padding;
    if (Element_Size-Element_Offset>=2)
    {
        Peek_B2(Padding);
        if (Padding==0x5838)
            guard_band();
    }

    if (Element_Offset<Element_Size)
        Skip_XX(Element_Size-Element_Offset,                    "Unknown");

    //In case of scrambling
    if (key_present)
    {
        delete[] Buffer; Buffer=Save_Buffer;
        Buffer_Offset=Save_Buffer_Offset;
        File_Offset=Save_File_Offset;
    }

    FILLING_BEGIN();
        {
            //Guard band
            if (Mpegv_frame_rate[frame_rate_code])
            {
            int64u BytesPerSecond=96000*bit_depth/8;
            float64 BytesPerFrame=BytesPerSecond/Mpegv_frame_rate[frame_rate_code];
            int64u BytesUpToLastFrame;
            int64u BytesUpToNextFrame;
            for (;;)
            {
                BytesUpToLastFrame=(int64u)(BytesPerFrame*Frame_Count);
                BytesUpToLastFrame/=bit_depth/4;
                BytesUpToLastFrame*=bit_depth/4;
                BytesUpToNextFrame=(int64u)(BytesPerFrame*(Frame_Count+1));
                BytesUpToNextFrame/=bit_depth/4;
                BytesUpToNextFrame*=bit_depth/4;

                if (BytesUpToLastFrame+GuardBand_Before<BytesUpToNextFrame)
                    break;

                // In case previous frame was PCM
                Frame_Count++;
                GuardBand_Before-=BytesUpToNextFrame-BytesUpToLastFrame;
            }
            GuardBand_After=BytesUpToNextFrame-BytesUpToLastFrame;
            int64u ToRemove=GuardBand_Before+(bit_depth>>1)+Element_Size; // Guardband + AES3 header + Dolby E frame
            if (ToRemove<(int64u)GuardBand_After)
                GuardBand_After-=ToRemove;
            else
                GuardBand_After=0;
            GuardBand_After/=bit_depth/4;
            GuardBand_After*=bit_depth/4;

            Element_Info1(GuardBand_Before);
            float64 GuardBand_Before_Duration=((float64)GuardBand_Before)/BytesPerSecond;
            Ztring GuardBand_Before_String=__T("GuardBand_Begin ")+Ztring::ToZtring(GuardBand_Before)+__T(" (")+Ztring::ToZtring(GuardBand_Before_Duration*1000000, 0)+Ztring().From_UTF8(" \xC2\xB5s"); //0xC2 0xB5 = micro sign
            Element_Info1(GuardBand_Before_String);
            }
        }

        if (!Status[IsAccepted])
        {
            Accept("Dolby E");
            PTS_Begin=FrameInfo.PTS;

            //Guard band
            GuardBand_Before_Initial=GuardBand_Before;
            GuardBand_After_Initial=GuardBand_After;
        }
        Frame_Count++;
        if (Frame_Count_NotParsedIncluded!=(int64u)-1)
            Frame_Count_NotParsedIncluded++;
        if (Mpegv_frame_rate[frame_rate_code])
            FrameInfo.DUR=float64_int64s(1000000000/Mpegv_frame_rate[frame_rate_code]);
        else
            FrameInfo.DUR=(int64u)-1;
        if (FrameInfo.DTS!=(int64u)-1)
            FrameInfo.DTS+=FrameInfo.DUR;
        if (FrameInfo.PTS!=(int64u)-1)
            FrameInfo.PTS+=FrameInfo.DUR;
        if (!Status[IsFilled] && ((description_packet_data.empty() && description_text_Values.empty()) || Frame_Count>=32+1+1+32+1)) // max 32 chars (discarded) + ETX (discarded) + STX + max 32 chars + ETX
            Fill("Dolby E");
    FILLING_END();
    if (Frame_Count==0 && Buffer_TotalBytes>Buffer_TotalBytes_FirstSynched_Max)
        Reject("Dolby E");
}

//---------------------------------------------------------------------------
void File_DolbyE::sync_segment()
{
    //Parsing
    Element_Begin1("sync_segment");
    Skip_S3(bit_depth,                                      "sync_word");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_DolbyE::metadata_segment()
{
    //Parsing
    Element_Begin1("metadata_segment");
    if (key_present)
    {
        //We must change the buffer
        switch (bit_depth)
        {
            case 16 :
                        {
                        int16u metadata_key;
                        Get_S2 (16, metadata_key, "metadata_key");
                        int16u metadata_segment_size=((BigEndian2int16u(Buffer+Buffer_Offset+(size_t)Element_Size-Data_BS_Remain()/8)^metadata_key)>>2)&0x3FF;

                        if (Data_BS_Remain()<((size_t)metadata_segment_size+1)*(size_t)bit_depth) //+1 for CRC
                            return; //There is a problem

                        int8u* Temp=Descrambled_Buffer+(size_t)Element_Size-Data_BS_Remain()/8;
                        for (int16u Pos=0; Pos<metadata_segment_size+1; Pos++)
                            int16u2BigEndian(Temp+Pos*2, BigEndian2int16u(Temp+Pos*2)^metadata_key);
                        }
                        break;
            case 20 :
                        {
                        int32u metadata_key;
                        Get_S3 (20, metadata_key, "metadata_key");
                        int16u metadata_segment_size=((BigEndian2int16u(Buffer+Buffer_Offset+(size_t)Element_Size-Data_BS_Remain()/8)^(metadata_key>>4))>>2)&0x3FF;

                        if (Data_BS_Remain()<((size_t)metadata_segment_size+1)*(size_t)bit_depth) //+1 for CRC
                            return; //There is a problem

                        Descramble_20bit(metadata_key, metadata_segment_size);
                        }
                        break;
            case 24 :
                        {
                        int32u metadata_key;
                        Get_S3 (24, metadata_key, "metadata_key");
                        int32u metadata_segment_size=((BigEndian2int16u(Buffer+Buffer_Offset+(size_t)Element_Size-Data_BS_Remain()/8)^metadata_key)>>2)&0x3FF;

                        if (Data_BS_Remain()<((size_t)metadata_segment_size+1)*bit_depth) //+1 for CRC
                            return; //There is a problem

                        int8u* Temp=Descrambled_Buffer+(size_t)Element_Size-Data_BS_Remain()/8;
                        for (int16u Pos=0; Pos<metadata_segment_size+1; Pos++)
                            int24u2BigEndian(Temp+Pos*2, BigEndian2int24u(Temp+Pos*2)^metadata_key);
                        }
                        break;
            default :   ;
        }
    }
    size_t  metadata_segment_BitCountAfter=Data_BS_Remain();
    int16u  metadata_segment_size;
    Skip_S1( 4,                                                 "metadata_revision_id");
    Get_S2 (10, metadata_segment_size,                          "metadata_segment_size");
    metadata_segment_BitCountAfter-=metadata_segment_size*bit_depth;
    Get_S1 ( 6, program_config,                                 "program_config"); Param_Info1(DolbyE_ChannelPositions[program_config]);
    Get_S1 ( 4, frame_rate_code,                                "frame_rate_code"); Param_Info3(Mpegv_frame_rate[frame_rate_code], " fps", 3);
    Info_S1( 4, original_frame_rate_code,                       "original_frame_rate_code"); Param_Info3(Mpegv_frame_rate[original_frame_rate_code], " fps", 3);
    Skip_S2(16,                                                 "frame_count");
    Element_Begin1("SMPTE_time_code");
    int8u Frames_Units, Frames_Tens, Seconds_Units, Seconds_Tens, Minutes_Units, Minutes_Tens, Hours_Units, Hours_Tens;
    bool  DropFrame;

    Skip_S1(4,                                                  "BG8");
    Skip_S1(4,                                                  "BG7");

    Skip_SB(                                                    "BGF2 / Field Phase");
    Skip_SB(                                                    "BGF1");
    Get_S1 (2, Hours_Tens,                                      "Hours (Tens)");
    Get_S1 (4, Hours_Units,                                     "Hours (Units)");

    Skip_S1(4,                                                  "BG6");
    Skip_S1(4,                                                  "BG5");

    Skip_SB(                                                    "BGF0 / BGF2");
    Get_S1 (3, Minutes_Tens,                                    "Minutes (Tens)");
    Get_S1 (4, Minutes_Units,                                   "Minutes (Units)");

    Skip_S1(4,                                                  "BG4");
    Skip_S1(4,                                                  "BG3");

    Skip_SB(                                                    "FP - Field Phase / BGF0");
    Get_S1 (3, Seconds_Tens,                                    "Seconds (Tens)");
    Get_S1 (4, Seconds_Units,                                   "Seconds (Units)");

    Skip_S1(4,                                                  "BG2");
    Skip_S1(4,                                                  "BG1");

    Skip_SB(                                                    "CF - Color fame");
    Get_SB (   DropFrame,                                       "DP - Drop frame");
    Get_S1 (2, Frames_Tens,                                     "Frames (Tens)");
    Get_S1 (4, Frames_Units,                                    "Frames (Units)");

    if (Hours_Tens<3)
    {
        int64u TimeCode=(int64u)(Hours_Tens     *10*60*60*1000
                               + Hours_Units       *60*60*1000
                               + Minutes_Tens      *10*60*1000
                               + Minutes_Units        *60*1000
                               + Seconds_Tens         *10*1000
                               + Seconds_Units           *1000
                               + (Mpegv_frame_rate[frame_rate_code]?float64_int32s((Frames_Tens*10+Frames_Units)*1000/Mpegv_frame_rate[frame_rate_code]):0));

        Element_Info1(Ztring().Duration_From_Milliseconds(TimeCode));

        //TimeCode
        if (SMPTE_time_code_StartTimecode==(int64u)-1)
            SMPTE_time_code_StartTimecode=TimeCode;
    }
    Element_End0();
    bool evolution_data_exists;
    Get_SB (    evolution_data_exists,                          "evolution_data_exists");
    Skip_S1( 7,                                                 "metadata_reserved_bits");
    for (int8u Channel=0; Channel<DolbyE_Channels[program_config]; Channel++)
    {
        Get_S2 (10, channel_subsegment_size[Channel],           "channel_subsegment_size");
        channel_subsegment_sizes[Channel][channel_subsegment_size[Channel]]++;
    }
    if (!Mpegv_frame_rate_type[frame_rate_code])
        Get_S1 ( 8, metadata_extension_segment_size,            "metadata_extension_segment_size");
    else
        metadata_extension_segment_size=0;
    Get_S1 ( 8, meter_segment_size,                             "meter_segment_size");
    for (int8u Program=0; Program<DolbyE_Programs[program_config]; Program++)
    {
        Element_Begin1("per program");
        int8u description_text;
        Get_S1 ( 8, description_text,                           "description_text"); Element_Info1(description_text);
        Info_S1( 2, bandwidth_id,                               "bandwidth_id"); Element_Info1(bandwidth_id);
        if (description_text && Program>=description_text_Values.size())
            description_text_Values.resize(Program+1);
        switch (description_text)
        {
            case 0x00: // No text
                    if (Program<description_text_Values.size())
                    {
                        description_text_Values[Program].Previous.clear();
                        description_text_Values[Program].Current.clear();
                    }
                    break;
            case 0x02: // STX
                    description_text_Values[Program].Current.clear();
                    description_text_Values[Program].Current.push_back(description_text);
                    break;
            case 0x03: // ETX
                    if (!description_text_Values[Program].Current.empty() && description_text_Values[Program].Current[0]==0x02)
                        description_text_Values[Program].Previous= description_text_Values[Program].Current.substr(1);
                    else
                        description_text_Values[Program].Previous.clear();
                    description_text_Values[Program].Current.clear();
                    break;
            default: if (description_text>=0x20 && description_text<=0x7E)
                        description_text_Values[Program].Current.push_back(description_text);
        }
        Element_End0();
    }
    for (int8u Channel=0; Channel<DolbyE_Channels[program_config]; Channel++)
    {
        Element_Begin1("per channel");
        Info_S1( 4, revision_id,                                "revision_id"); Element_Info1(revision_id);
        Info_SB(    bitpool_type,                               "bitpool_type");
        Info_S2(10, begin_gain,                                 "begin_gain"); Element_Info1(begin_gain);
        Info_S2(10, end_gain,                                   "end_gain"); Element_Info1(end_gain);
        Element_End0();
    }
    for(;;)
    {
        Element_Begin1("metadata_subsegment");
        int16u  metadata_subsegment_length;
        int8u   metadata_subsegment_id;
        Get_S1 ( 4, metadata_subsegment_id,                     "metadata_subsegment_id");
        if (metadata_subsegment_id==0)
        {
            Element_End0();
            break;
        }
        Get_S2 (12, metadata_subsegment_length,                 "metadata_subsegment_length");
        size_t End=Data_BS_Remain()-metadata_subsegment_length;
        switch (metadata_subsegment_id)
        {
            case 1 : ac3_metadata_subsegment(true); break;
            case 2 : ac3_metadata_subsegment(false); break;
            default: Skip_BS(metadata_subsegment_length,        "metadata_subsegment (unknown)");
        }
        if (Data_BS_Remain()>End)
            Skip_BS(Data_BS_Remain()-End,                       "unknown");
        Element_End0();
    }
    if (evolution_data_exists)
    {
        for (;;)
        {
            const size_t evolution_data_segment_id_Name_Size=2;
            const char* const evolution_data_segment_id_Name[evolution_data_segment_id_Name_Size]=
            {
                "End",
                "intelligent_loudness_evolution_data_segment",
            };

            Element_Begin1("evolution_data_segment");
            int16u evolution_data_segment_length;
            int8u evolution_data_segment_id;
            Get_S1 ( 4, evolution_data_segment_id,              "evolution_data_segment_id"); Param_Info1C(evolution_data_segment_id<evolution_data_segment_id_Name_Size, evolution_data_segment_id_Name[evolution_data_segment_id]);
            if (!evolution_data_segment_id)
            {
                Element_End0();
                break;
            }
            Get_S2 (12, evolution_data_segment_length,          "evolution_data_segment_length");
            size_t End=Data_BS_Remain()-evolution_data_segment_length;
            switch (evolution_data_segment_id)
            {
                case  1: intelligent_loudness_evolution_data_segment(); break;
                default: Skip_BS(evolution_data_segment_length, "evolution_data_segment (unknown)");
            }
            if (Data_BS_Remain()>End)
                Skip_BS(Data_BS_Remain()-End,                   "unknown");
            Element_End0();
        }
    }
    if (Data_BS_Remain()>metadata_segment_BitCountAfter)
        Skip_BS(Data_BS_Remain()-metadata_segment_BitCountAfter,"reserved_metadata_bits");
    Skip_S3(bit_depth,                                          "metadata_crc");

    {
        //CRC test
        /*
        size_t Pos_End=Buffer_Offset*8+(size_t)Element_Size*8-Data_BS_Remain();
        size_t Pos_Begin=Pos_End-(metadata_segment_size+1)*bit_depth; //+1 for CRC
        int8u BitSkip_Begin=Pos_Begin%8;
        Pos_Begin/=8;
        int8u BitSkip_End=0; // Pos_End%8; Looks like that the last bits must not be in the CRC computing
        Pos_End/=8;
        if (BitSkip_End)
            Pos_End++;

        int16u CRC=CRC_16_Compute(Buffer+Pos_Begin, Pos_End-Pos_Begin, BitSkip_Begin, BitSkip_End);
        if (CRC)
        {
            // CRC is wrong
            Param_Info1("metadata_crc NOK");
        }
        */
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_DolbyE::guard_band()
{
    int8u* NewBuffer=NULL;
    size_t Buffer_Offset_Save;
    size_t Buffer_Size_Save;
    int64u Element_Offset_Save;
    int64u Element_Size_Save;

    Element_Begin1("guard_band (with data)");
    int16u element_length;
    int8u element_id;
    bool escape_code_valid;
    Skip_B2(                                                    "sync_word");
    BS_Begin();
    Skip_S1( 3,                                                 "reserved");
    Get_SB (    escape_code_valid,                              "escape_code_valid");
    if (escape_code_valid)
    {
        int16u escape_code;
        Get_S2 (12, escape_code,                                 "escape_code");
        BS_End();
        for (int64u i=Element_Offset; i+1<Element_Size; i++)
        {
            if ( Buffer[Buffer_Offset+i  ]     ==(escape_code>>4) && (Buffer[Buffer_Offset+i+1]>>4)==(escape_code&0x0F))
            {
                //0xABCD --> 0x078D
                if (!NewBuffer)
                {
                    NewBuffer=new int8u[Element_Size-Element_Offset];
                    memcpy(NewBuffer, Buffer+Buffer_Offset+Element_Offset, Element_Size-Element_Offset);
                }
                NewBuffer[i  -Element_Offset]=0x07;
                NewBuffer[i+1-Element_Offset]=(NewBuffer[i+1-Element_Offset]&0x0F)|0x80;
            }
            if ((Buffer[Buffer_Offset+i  ]&0xF)==(escape_code>>8) &&  Buffer[Buffer_Offset+i+1]    ==(escape_code&0xFF))
            {
                //0xABCD --> 0xA078
                if (!NewBuffer)
                {
                    NewBuffer=new int8u[Element_Size-Element_Offset];
                    memcpy(NewBuffer, Buffer+Buffer_Offset+Element_Offset, Element_Size-Element_Offset);
                }
                NewBuffer[i  -Element_Offset]=(NewBuffer[i  -Element_Offset]&0xF0);
                NewBuffer[i+1-Element_Offset]=0x78;
            }
        }
        if (NewBuffer)
        {
            Buffer=NewBuffer;
            Buffer_Offset_Save=Buffer_Offset;
            Buffer_Size_Save=Buffer_Offset;
            Element_Offset_Save=Element_Offset;
            Element_Size_Save=Element_Size;
            File_Offset+=Buffer_Offset+Element_Offset;
            Buffer_Offset=0;
            Buffer_Size=Element_Size-Element_Offset;
            Element_Offset=0;
            Element_Size=Buffer_Size;
        }
    }
    else
    {
        Skip_S2(12,                                             "escape_code");
        BS_End();
    }
    Get_B1 (    element_id,                                     "element_id");
    Get_B2 (    element_length,                                 "element_length");
    int64u After=Element_Offset+element_length;
    switch (element_id)
    {
        case 0xBB: evo_frame(); break;
        default: Skip_XX(element_length,                        "Unknown");
    }
    if (Element_Offset<After)
        Skip_XX(After-Element_Offset,                           "Unknown");
    else if (Element_Offset>After)
    {
        Param_Info1("Problem");
        Element_Offset=After;
    }
    Skip_B2(                                                    "crc");
    {
        //CRC test
        /*
        size_t Pos_End=Buffer_Offset+Element_Offset;
        size_t Pos_Begin=Pos_End-(element_length+2); //+2 for CRC

        int16u CRC=CRC_16_Compute(Buffer+Pos_Begin, Pos_End-Pos_Begin, 0, 0);
        if (CRC)
        {
            // CRC is wrong
            Param_Info1("crc NOK");
        }
        */
        Element_End0();
        int64u RemainingBytes=Element_Size-Element_Offset;
        if (RemainingBytes && RemainingBytes<bit_depth/4)
        {
            bool HasContent=false;
            size_t Offset=Buffer_Offset+(size_t)Element_Offset;
            size_t Size=Buffer_Offset+(size_t)Element_Size;
            for (; Offset<Size; Offset++)
                if (Buffer[Offset])
                    HasContent=true;
            if (!HasContent)
                Skip_XX(Element_Size-Element_Offset,            "Padding");
        }
    }

    if (NewBuffer)
    {
        delete[] Buffer;
        Buffer_Offset=Buffer_Offset_Save;
        Buffer_Size=Buffer_Offset_Save;
        Element_Offset=Element_Offset_Save;
        Element_Size=Element_Size_Save;
        File_Offset-=Buffer_Offset+Element_Offset;
    }
}

//---------------------------------------------------------------------------
void File_DolbyE::intelligent_loudness_evolution_data_segment()
{
    Element_Begin1("intelligent_loudness_evolution_data_segment");
    for (int8u program=0; program<DolbyE_Programs[program_config]; program++)
    {
        Element_Begin1("per program");
        Skip_S1(4,                                          "loudness_reg_type");
        Skip_SB(                                            "dialogue_corrected");
        Skip_S1(1,                                          "loudness_corr_type");
        Element_End0();
    }
    Element_End0();
}

//---------------------------------------------------------------------------
extern const char* Ac3_emdf_payload_id[16];
void File_DolbyE::evo_frame()
{
    if (!Guardband_EMDF_PresentAndSize)
        Guardband_EMDF_PresentAndSize=Element_Size;

    Element_Begin1("evo_frame");
    BS_Begin();
    int8u evo_version, key_id;
    Get_S1 (2, evo_version,                                     "evo_version");
    if (evo_version==3)
    {
        int32u evo_version32;
        Get_V4 (2, evo_version32,                               "evo_version");
        evo_version32+=3;
        evo_version=(int8u)evo_version32;
    }
    if (evo_version)
    {
        Skip_BS(Data_BS_Remain(),                               "(Unparsed evo_frame data)");
        Element_End0(); 
        return;
    }
    Get_S1 (3, key_id,                                          "key_id");
    if (key_id==7)
        Skip_V4(3,                                              "key_id");

    int32u payload_id = 0;
        
    for(;;)
    {
        Element_Begin1("evo_payload");
        Get_S4 (5, payload_id,                                  "payload_id");
        if (payload_id==0x1F)
        {
            int32u add;
            Get_V4 (5, add,                                     "payload_id");
            payload_id += add;
        }

        if (payload_id<16)
            Element_Info1(Ac3_emdf_payload_id[payload_id]);
        if (!payload_id)
        {
            Element_End0();
            break;
        }

        evo_payload_config();

        int32u payload_size = 0;
        Get_V4 (8, payload_size,                                "payload_size");
        size_t payload_End=(Data_BS_Remain()>payload_size*8)?(Data_BS_Remain()-payload_size*8):0;

        Element_Begin1("payload");
            switch (payload_id)
            {
                case 11: object_audio_metadata_payload(); break;
                case 13: mgi_payload(); break;
                default: Skip_BS(payload_size*8,                "(Unknown)");
            }
            size_t Remaining=Data_BS_Remain()-payload_End;
            if (Remaining && Remaining<8)
            {
                int8u padding;
                Peek_S1(Remaining, padding);
                if (!padding)
                    Skip_S1(Remaining,                          "padding");
            }
            if (Data_BS_Remain()>payload_End)
            {
                Skip_BS(Data_BS_Remain()-payload_End,           "(Unparsed payload bytes)");
            }
            else if (Data_BS_Remain()<payload_End)
            {
                //There is a problem, too many bits were consumed by the parser. //TODO: prevent the parser to consume more bits than count of bits in this element
                if (Data_BS_Remain()>=payload_End)
                    Skip_BS(Data_BS_Remain()-payload_End,       "(Problem during emdf_payload parsing)");
                else
                    Skip_BS(Data_BS_Remain(),                   "(Problem during payload parsing, going to end directly)");
                Element_End0();
                Element_End0();
                break;
            }
        Element_End0();
        Element_End0();
    }

    evo_protection();
    BS_End();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_DolbyE::evo_payload_config()
{
    Element_Begin1("payload_config");
    bool timestamp_present;
    TEST_SB_GET (timestamp_present,                             "timestamp_present");
        Skip_V4(11,                                             "timestamp");
    TEST_SB_END();
    TEST_SB_SKIP(                                               "duration_present");
        Skip_V4(11,                                             "duration");
    TEST_SB_END();
    TEST_SB_SKIP(                                               "group_id_present");
        Skip_V4(2,                                              "group_id");
    TEST_SB_END();
    TEST_SB_SKIP(                                               "codec_specific_id_present");
        Skip_S1(8,                                              "codec_specific_id");
    TEST_SB_END();

    bool dont_transcode;
    Get_SB(dont_transcode,                                      "dont_transcode");
    if (!dont_transcode)
    {
        bool now_or_never = false;
        if (!timestamp_present)
        {
            Get_SB (now_or_never,                               "now_or_never");
            if (now_or_never)
            {
                Skip_SB(                                        "create_duplicate");
                Skip_SB(                                        "remove_duplicate");

            }
        }

        if (timestamp_present || now_or_never)
        {
            Skip_S1(5,                                          "priority");
            Skip_S1(2,                                          "tight_coupling");
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_DolbyE::evo_protection()
{
    Element_Begin1("protection");
    int8u len_primary, len_second;
    Get_S1(2, len_primary,                                      "protection_length_primary");
    Get_S1(2, len_second,                                       "protection_length_secondary");

    switch (len_primary)
    {
        case 0: len_primary = 0; break;
        case 1: len_primary = 8; break;
        case 2: len_primary = 32; break;
        case 3: len_primary = 128; break;
        default:; //Cannot append, read only 2 bits
    };
    switch (len_second)
    {
        case 0: len_second = 0; break;
        case 1: len_second = 8; break;
        case 2: len_second = 32; break;
        case 3: len_second = 128; break;
        default:; //Cannot append, read only 2 bits
    };
    Skip_BS(len_primary,                                        "protection_bits_primary");
    if (len_second)
        Skip_BS(len_primary,                                    "protection_bits_secondary");

    Element_End0();
}


//---------------------------------------------------------------------------
void File_DolbyE::object_audio_metadata_payload()
{
    nonstd_bed_channel_assignment_masks.clear();
    ObjectElements.clear();

    Element_Begin1("object_audio_metadata_payload");
    int8u oa_md_version_bits;
    Get_S1 (2, oa_md_version_bits,                              "oa_md_version_bits");
    if (oa_md_version_bits == 0x3)
    {
        int8u oa_md_version_bits_ext;
        Get_S1 (3, oa_md_version_bits_ext,                      "oa_md_version_bits_ext");
        oa_md_version_bits += oa_md_version_bits_ext;
    }

    int8u object_count_bits;
    Get_S1 (5, object_count_bits,                               "object_count_bits");
    if (object_count_bits==0x1F)
    {
        int8u object_count_bits_ext;
        Get_S1 (7, object_count_bits_ext,                       "object_count_bits_ext");
        object_count_bits=0x1F+object_count_bits_ext;
    }
    object_count=object_count_bits+1;
    Param_Info2(object_count, " objects");

    program_assignment();

    bool b_alternate_object_data_present;
    Get_SB(b_alternate_object_data_present,                     "b_alternate_object_data_present");
    int8u oa_element_count_bits;
    Get_S1(4, oa_element_count_bits,                            "oa_element_count_bits");
    if (oa_element_count_bits==0xF)
    {
        Get_S1(5, oa_element_count_bits,                        "oa_element_count_bits_ext");
        oa_element_count_bits+=0xF;
    }
    for (int8u i=0; i<oa_element_count_bits; i++)
        oa_element_md(b_alternate_object_data_present);

    Element_End0();
}

void File_DolbyE::program_assignment()
{
    Element_Begin1("program_assignment");
    bool b_dyn_object_only_program = false;
    Get_SB (b_dyn_object_only_program,                          "b_dyn_object_only_program");
    if (b_dyn_object_only_program)
    {
        bool b_lfe_present;
        Get_SB (b_lfe_present,                                  "b_lfe_present");
        if (b_lfe_present)
        {
            nonstd_bed_channel_assignment_masks.push_back(1<<3);
            b_object_in_bed_or_isf.push_back(true);
        }
    }
    else
    {
        int8u content_description_mask;
        Get_S1 (4, content_description_mask,                    "content_description_mask");
        if (content_description_mask & 0x1)
        {
            bool b_bed_object_chan_distribute, b_multiple_bed_instances_present;

            Get_SB (b_bed_object_chan_distribute,               "b_bed_object_chan_distribute");
            Get_SB (b_multiple_bed_instances_present,           "b_multiple_bed_instances_present");
            int32u num_bed_instances = 1;
            if (b_multiple_bed_instances_present)
            {
                int8u num_bed_instances_bits = 0;
                Get_S1 (3, num_bed_instances_bits,              "num_bed_instances_bits");
                num_bed_instances = num_bed_instances_bits + 2;
            }

            for (int32u bed = 0; bed < num_bed_instances; ++bed)
            {
                Element_Begin1("Bed");
                bool b_lfe_only = true;
                Get_SB (b_lfe_only,                             "b_lfe_only");
                if (!b_lfe_only)
                {
                    bool b_standard_chan_assign;
                    Get_SB (b_standard_chan_assign,             "b_standard_chan_assign");
                    int32u nonstd_bed_channel_assignment_mask{};
                    if (b_standard_chan_assign)
                    {
                        int16u bed_channel_assignment_mask;
                        Get_S2 (10, bed_channel_assignment_mask, "bed_channel_assignment_mask");
                        nonstd_bed_channel_assignment_mask=AC3_bed_channel_assignment_mask_2_nonstd(bed_channel_assignment_mask);
                    }
                    else
                    {
                        int32u nonstd_bed_channel_assignment_mask;
                        Get_S3 (17, nonstd_bed_channel_assignment_mask, "nonstd_bed_channel_assignment_mask");
                    }
                    Param_Info1(AC3_nonstd_bed_channel_assignment_mask_ChannelLayout(nonstd_bed_channel_assignment_mask));
                    nonstd_bed_channel_assignment_masks.push_back(nonstd_bed_channel_assignment_mask);
                    size_t BedChannelCount=BedChannelConfiguration_ChannelCount(nonstd_bed_channel_assignment_mask);
                    b_object_in_bed_or_isf.resize(b_object_in_bed_or_isf.size()+BedChannelCount, true);
                }
                else
                    b_object_in_bed_or_isf.push_back(true);
                Element_End0();
            }
        }

        if (content_description_mask & 0x2)
        {
            int8u intermediate_spatial_format_idx;
            Get_S1 (3, intermediate_spatial_format_idx,        "intermediate_spatial_format_idx");
            b_object_in_bed_or_isf.resize(b_object_in_bed_or_isf.size()+intermediate_spatial_format_object_count[intermediate_spatial_format_idx], true);
        }

        if (content_description_mask & 0x4)
        {
            int8u num_dynamic_objects_bits;
            Get_S1(5, num_dynamic_objects_bits,                 "num_dynamic_objects_bits");
            if (num_dynamic_objects_bits==0x1F)
            {
                int8u num_dynamic_objects_bits_ext = 0;
                Get_S1 (7, num_dynamic_objects_bits_ext,        "num_dynamic_objects_bits_ext");
                num_dynamic_objects_bits=0x1F+num_dynamic_objects_bits_ext;
            }
            num_dynamic_objects_bits++;
            Param_Info2(object_count-num_dynamic_objects_bits, " static objects");
            Param_Info2(num_dynamic_objects_bits, " dynamic objects");
            b_object_in_bed_or_isf.resize(b_object_in_bed_or_isf.size()+num_dynamic_objects_bits, false);
        }

        if (content_description_mask & 0x8)
        {
            int8u reserved_data_size_bits;
            Get_S1 (4, reserved_data_size_bits,                 "reserved_data_size_bits");
            int8u padding = 8 - (reserved_data_size_bits % 8);
            Skip_S1(reserved_data_size_bits,                    "reserved_data()");
            Skip_S1(padding,                                    "padding");
        }
    }

    Element_End0();
}

void File_DolbyE::oa_element_md(bool b_alternate_object_data_present)
{
    Element_Begin1("oa_element_md");
    int8u oa_element_id_idx;
    int32u oa_element_size_bits;
    Get_S1 (4, oa_element_id_idx,                               "oa_element_id_idx");
    Get_V4 (4, 4, oa_element_size_bits,                         "oa_element_size_bits");
    oa_element_size_bits++;
    oa_element_size_bits*=8;
    int32u b_alternate_object_data_present_Reduced=b_alternate_object_data_present*4+1;
    if (oa_element_size_bits<b_alternate_object_data_present_Reduced || oa_element_size_bits>Data_BS_Remain())
    {
        Skip_BS(oa_element_size_bits,                           "?");
        Element_End0();
        return;
    }
    oa_element_size_bits-=b_alternate_object_data_present_Reduced;
    if (b_alternate_object_data_present)
        Skip_S1(4,                                              "alternate_object_data_id_idx");
    Skip_SB(                                                    "b_discard_unknown_element");
    size_t End=Data_BS_Remain()-oa_element_size_bits;
    switch (oa_element_id_idx)
    {
        case  1: object_element(); break;
        default: Skip_BS(oa_element_size_bits,                  "oa_element");
    }
    if (Data_BS_Remain()>End)
        Skip_BS(Data_BS_Remain()-End,                           "padding");
    Element_End0();
}

void File_DolbyE::object_element()
{
    Element_Begin1("object_element");
    int8u num_obj_info_blocks_bits;
    md_update_info(num_obj_info_blocks_bits);
    bool b_reserved_data_not_present;
    Get_SB (b_reserved_data_not_present,                        "b_reserved_data_not_present");
    if (!b_reserved_data_not_present)
        Skip_S1(5,                                              "reserved");
    for (int8u i=0; i<object_count; i++)
        object_data(i, num_obj_info_blocks_bits);
    Element_End0();
}

void File_DolbyE::md_update_info(int8u& num_obj_info_blocks_bits)
{
    Element_Begin1("md_update_info");
    int8u sample_offset_code;
    Get_S1 (2, sample_offset_code,                              "sample_offset_code");
    switch (sample_offset_code)
    {
        case 1 : Skip_S1(2,                                     "sample_offset_idx"); break;
        case 2 : Skip_S1(5,                                     "sample_offset_bits"); break;
        default:;
    }
    Get_S1 (3, num_obj_info_blocks_bits,                        "num_obj_info_blocks_bits");
    for (int8u blk=0; blk<=num_obj_info_blocks_bits; blk++)
        block_update_info();
    Element_End0();
}

void File_DolbyE::block_update_info()
{
    Element_Begin1("block_update_info");
    int8u block_offset_factor_bits, ramp_duration_code;
    Get_S1 (6, block_offset_factor_bits,                        "block_offset_factor_bits");
    Get_S1 (2, ramp_duration_code,                              "ramp_duration_code");
    switch (ramp_duration_code)
    {
        case 3 : 
                {
                    bool b_use_ramp_duration_idx;
                    Get_SB (b_use_ramp_duration_idx,            "b_use_ramp_duration_idx");
                    if (b_use_ramp_duration_idx)
                        Skip_S1( 4,                             "ramp_duration_idx");
                    else
                        Skip_S1(11,                             "ramp_duration_bits");
                }
                break;
        default:;
    }
    Element_End0();
}

void File_DolbyE::object_data(int8u obj_idx, int8u num_obj_info_blocks_bits)
{
    ObjectElements.resize(ObjectElements.size()+1);
    ObjectElements[ObjectElements.size()-1].Alts.resize(num_obj_info_blocks_bits+1);

    Element_Begin1("object_data");
    for (int8u blk=0; blk<=num_obj_info_blocks_bits; blk++)
        object_info_block(obj_idx, blk);
    Element_End0();
}

void File_DolbyE::object_info_block(int8u obj_idx, int8u blk)
{
    Element_Begin1("object_info_block");
    int8u object_basic_info_status_idx, object_render_info_status_idx;
    bool b_object_not_active;
    Get_SB (b_object_not_active,                                "b_object_not_active");
    if (b_object_not_active)
        object_basic_info_status_idx=0;
    else if (!blk)
        object_basic_info_status_idx=1;
    else
        Get_S1 (2, object_basic_info_status_idx,                "object_basic_info_status_idx");
    if (object_basic_info_status_idx&1) // 1 or 3
        object_basic_info(object_basic_info_status_idx>>1, blk); // Use bit 1
    else
    {
        dyn_object& D=ObjectElements[ObjectElements.size()-1];
        dyn_object::dyn_object_alt& A=D.Alts[blk];
        A.obj_gain_db=INT8_MAX;
    }
    if (b_object_not_active || (obj_idx<b_object_in_bed_or_isf.size() && b_object_in_bed_or_isf[obj_idx]))
        object_render_info_status_idx=0;
    else if (!blk)
        object_render_info_status_idx=1;
    else
        Get_S1 (2, object_render_info_status_idx,               "object_render_info_status_idx");
    if (object_render_info_status_idx&1) // 1 or 3
        object_render_info(object_render_info_status_idx>>1, blk); // Use bit 1
    else
    {
        dyn_object& D=ObjectElements[ObjectElements.size()-1];
        dyn_object::dyn_object_alt& A=D.Alts[blk];
        A.pos3d_x_bits=(int8u)-1;
    }
    bool b_additional_table_data_exists;
    Get_SB (b_additional_table_data_exists,                     "b_additional_table_data_exists");
    if (b_additional_table_data_exists)
    {
        int8u additional_table_data_size_bits;
        Get_S1(4, additional_table_data_size_bits,              "additional_table_data_size_bits");
        additional_table_data_size_bits++;
        additional_table_data_size_bits*=8;
        Skip_BS(additional_table_data_size_bits,                "additional_table_data");
    }
    Element_End0();
}

void File_DolbyE::object_basic_info(int8u object_basic_info_array, int8u blk)
{
    Element_Begin1("object_basic_info");
    if (!object_basic_info_array) // object_basic_info_array is "reuse" info at this point
        object_basic_info_array=3; // 2x 1
    else
        Get_S1 (2, object_basic_info_array,                     "object_basic_info[]");
    dyn_object& D=ObjectElements[ObjectElements.size()-1];
    dyn_object::dyn_object_alt& A=D.Alts[blk];
    if (object_basic_info_array>>1) // bit 1
    {
        int8u object_gain_idx;
        Get_S1 (2, object_gain_idx,                             "object_gain_idx");
        switch (object_gain_idx)
        {
            case 0 :
                    A.obj_gain_db=0;
                    break;
            case 1 :
                    A.obj_gain_db=INT8_MIN;
                    break;
            case 2 :
                    {
                    int8u object_gain_bits;
                    Get_S1 (6, object_gain_bits,                "object_gain_bits");
                    A.obj_gain_db=(object_gain_bits<15?15:14)-object_gain_bits;
                    }
                    break;
            default:
                    if (ObjectElements.size()>=2)
                        A.obj_gain_db=ObjectElements[ObjectElements.size()-2].Alts[blk].obj_gain_db;
                    else
                        A.obj_gain_db=0;
        }
    }
    else
        A.obj_gain_db=INT8_MAX;
    if (object_basic_info_array&1) // bit 0
    {
        bool b_default_object_priority;
        Get_SB (   b_default_object_priority,                   "b_default_object_priority");
        if (!b_default_object_priority)
            Skip_S1(5,                                          "b_default_object_priority");
    }
    Element_End0();
}

void File_DolbyE::object_render_info(int8u object_render_info_array, int8u blk)
{
    Element_Begin1("object_render_info");
    if (!object_render_info_array) // object_render_info_array is "reuse" info at this point
        object_render_info_array=0xF; // 4x 1
    else
        Get_S1 (4, object_render_info_array,                    "object_render_info[]");
    dyn_object& D=ObjectElements[ObjectElements.size()-1];
    dyn_object::dyn_object_alt& A=D.Alts[blk];
    if (object_render_info_array&1) // bit 0
    {
        bool b_differential_position_specified;
        if (!blk)
            b_differential_position_specified=0;
        else
            Get_SB(b_differential_position_specified,           "b_differential_position_specified");
        if (b_differential_position_specified)
        {
            Skip_S1(3,                                          "diff_pos3D_X_bits");
            Skip_S1(3,                                          "diff_pos3D_Y_bits");
            Skip_S1(3,                                          "diff_pos3D_Z_bits");
            A.pos3d_x_bits=(int8u)-1; // Not supported
        }
        else
        {
            bool b_object_distance_specified;
            Get_S1 (6, A.pos3d_x_bits,                          "pos3d_x_bits"); Param_Info3(mgi_bitstream_val_to_Q15(A.pos3d_x_bits, 6)/32768.0*100, "%", 0);
            Get_S1 (6, A.pos3d_y_bits,                          "pos3d_y_bits"); Param_Info3(mgi_bitstream_val_to_Q15(A.pos3d_y_bits, 6)/32768.0*100, "%", 0);
            Get_SB (   A.pos3d_z_sig,                           "pos3d_z_sig");
            Get_S1 (4, A.pos3d_z_bits,                          "pos3d_z_bits"); Param_Info3(mgi_bitstream_pos_z_to_Q15(A.pos3d_z_sig, A.pos3d_z_bits)/32768.0*100, "%", 0);
            Get_SB (   b_object_distance_specified,             "b_object_distance_specified");
            if (b_object_distance_specified)
            {
                bool b_object_at_infinity;
                Get_SB (   b_object_at_infinity,                "b_object_at_infinity");
                if (!b_object_at_infinity)
                    Skip_S1(4,                                  "distance_factor_idx");
            }
        }
    }
    else
        A.pos3d_x_bits=(int8u)-1;
    A.hp_render_mode=(int8u)-1;
    if ((object_render_info_array>>1)&1) // bit 1
    {
        Skip_S1(3,                                              "zone_constraints_idx");
        Skip_SB(                                                "b_enable_elevation");
    }
    if ((object_render_info_array>>2)&1) // bit 2
    {
        int8u object_size_idx;
        Get_S1 (2, object_size_idx,                             "object_size_idx");
        switch (object_size_idx)
        {
            case  1:
                    {
                    Skip_S1(5,                                  "object_size_bits");
                    }
                    break;
            case  2:
                    {
                    Skip_S1(5,                                  "object_width_bits");
                    Skip_S1(5,                                  "object_depth_bits");
                    Skip_S1(5,                                  "object_height_bits");
                    }
                    break;
            default:;
        }
    }
    if (object_render_info_array>>3) // bit 3
    {
        bool b_object_use_screen_ref;
        Get_SB (   b_object_use_screen_ref,                     "b_object_use_screen_ref");
        if (b_object_use_screen_ref)
        {
            Skip_S1(3,                                          "screen_factor_bits");
            Skip_S1(2,                                          "depth_factor_idx");
        }
        Skip_SB(                                                "b_object_snap");
    }
    Element_End0();
}

int8u bits_needed(size_t value)
{
    if (!value)
        return 0;

    int8u res = 0;
    while (value) {
        res += 1;
        value >>= 1;
    }
    return res;
}

void File_DolbyE::mgi_payload()
{
    DynObjects.clear();
    BedInstances.clear();
    Presets.clear();
    substream_mappings.clear();

    Element_Begin1("mgi_payload");
    size_t BS_Start=Data_BS_Remain();
    int8u mgi_version, num_presets_m1, num_dyn_objects, num_bed_instances;
    bool program_id_available, is_substream_mapping_present, mini_mixgraph_extension_present, mini_mixgraph_description_present;
    Get_S1 (2, mgi_version,                                     "mgi_version");
    if (mgi_version ==3)
    {
        int32u mgi_version32;
        Get_V4 (2, mgi_version32,                               "mgi_version");
        mgi_version32+=3;
        mgi_version=(int8u)mgi_version32;
    }
    if (mgi_version!=5)
    {
        Skip_BS(1,                                              "(Unparsed mgi_payload data)"); //TODO: exact count of bits
        Element_End0();
        return;
    }
    Get_SB (program_id_available,                               "program_id_available");
    Get_SB (is_substream_mapping_present,                       "is_substream_mapping_present");
    Get_SB (mini_mixgraph_extension_present,                    "mini_mixgraph_extension_present");
    Get_SB (mini_mixgraph_description_present,                  "mini_mixgraph_description_present");
    if (program_id_available)
    {
        Skip_S1( 3,                                             "program_uuid_segment_number");
        Skip_S2(16,                                             "program_uuid_segment");
    }
    {
        size_t ToPadd=(BS_Start-Data_BS_Remain())%8;
        if (ToPadd<8)
            Skip_BS(8-ToPadd,                                   "byte_align");
    }
    Skip_S2(16,                                                 "short_program_id");
    Get_S1 (4, num_presets_m1,                                  "num_presets_m1");
    Get_S1 (7, num_dyn_objects,                                 "num_dyn_objects");
    Get_S1 (4, num_bed_instances,                               "num_bed_instances");

    DynObjects.resize(num_dyn_objects);
    for (int8u i=0; i<num_dyn_objects; i++)
    {
        dyn_object& D=DynObjects[i];
        Element_Begin1("dyn_object");
        int32u num_dyn_obj_alt_md;
        Get_V4 (2, num_dyn_obj_alt_md,                          "num_dyn_obj_alt_md");
        Get_S1 (2, D.sound_category,                            "sound_category"); Param_Info1(sound_category_Values[D.sound_category]);
        D.Alts.resize(num_dyn_obj_alt_md);
        for (int32u j=0; j<num_dyn_obj_alt_md; j++)
        {
            dyn_object::dyn_object_alt& A=D.Alts[j];
            Element_Begin1("dyn_obj_alt_md");
            int32u object_info_mask, object_info_size_bytes;
            Get_V4 (3, object_info_mask,                        "object_info_mask");
            //Get_V4 (3, object_info_size_bytes,                  "object_info_size_bytes"); //TODO
            Get_S4 (4, object_info_size_bytes,                  "object_info_size_bytes, variable_bits ignored");
            object_info_size_bytes=4;
            size_t Begin=Data_BS_Remain();
            if (object_info_size_bytes)
            {
                if (object_info_mask & 0x1)
                {
                    int8u dyn_obj_gain_db_bits;
                    Get_S1(6, dyn_obj_gain_db_bits,             "dyn_obj_gain_db_bits");
                    if (dyn_obj_gain_db_bits==63)
                        A.obj_gain_db=INT8_MIN;
                    else
                        A.obj_gain_db=15-dyn_obj_gain_db_bits;
                }
                else
                    A.obj_gain_db=INT8_MAX;
                if (object_info_mask & 0x2)
                {
                    Get_S1 (6, A.pos3d_x_bits,                  "pos3d_x_bits"); Param_Info3(mgi_bitstream_val_to_Q15(A.pos3d_x_bits, 6)/32768.0*100, "%", 0);
                    Get_S1 (6, A.pos3d_y_bits,                  "pos3d_y_bits"); Param_Info3(mgi_bitstream_val_to_Q15(A.pos3d_y_bits, 6)/32768.0*100, "%", 0);
                    Get_SB (   A.pos3d_z_sig,                   "pos3d_z_sig");
                    Get_S1 (4, A.pos3d_z_bits,                  "pos3d_z_bits"); Param_Info3(mgi_bitstream_pos_z_to_Q15(A.pos3d_z_sig, A.pos3d_z_bits)/32768.0*100, "%", 0);
                    Element_Level--;
                    Element_Info1(Angles2String(mgi_bitstream_pos_ToAngles(mgi_bitstream_val_to_Q15(A.pos3d_x_bits, 6), mgi_bitstream_val_to_Q15(A.pos3d_y_bits, 6), mgi_bitstream_pos_z_to_Q15(A.pos3d_z_sig, A.pos3d_z_bits))));
                    Element_Level++;
                }
                else
                    A.pos3d_x_bits=(int8u)-1;
                if (object_info_mask & 0x4)
                {
                    Skip_SB(                                    "hp_md_state");
                    Get_S1 (2, A.hp_render_mode,                "hp_render_mode"); Param_Info1(hp_render_mode_Values[A.hp_render_mode]);
                    Skip_SB(                                    "hp_headtrack_state");
                }
                else
                    A.hp_render_mode=(int8u)-1;
                size_t Parsed=Begin-Data_BS_Remain();
                int8u Padding=8-(Parsed%8);
                Skip_BS(Padding,                                "padding");
            }
            Element_End0();
        }
        Element_End0();
    }

    BedInstances.resize(num_bed_instances);
    for (int8u i=0; i<num_bed_instances; i++)
    {
        bed_instance& B=BedInstances[i];
        Element_Begin1("bed_instance");
        int32u num_bed_alt_md;
        Get_V4 (2, num_bed_alt_md,                              "num_bed_alt_md");
        B.Alts.resize(num_bed_alt_md);
        for (int8u j=0; j<num_bed_alt_md; j++)
        {
            bed_instance::bed_alt& A=B.Alts[j];
            Element_Begin1("bed_alt_md");
            int32u bed_info_mask, bed_info_size_bytes;
            Get_V4 (2, bed_info_mask,                           "bed_info_mask");
            Get_V4 (2, bed_info_size_bytes,                     "bed_info_size_bytes");
            size_t End=Data_BS_Remain()-bed_info_size_bytes*8;
            if (bed_info_mask&1)
            {
                int8u bed_gain_db_bits;
                Get_S1(6, bed_gain_db_bits,                     "bed_gain_db_bits");
                A.bed_gain_db=15-bed_gain_db_bits;
            }
            else
                A.bed_gain_db=INT8_MAX;
            if (Data_BS_Remain()>End)
                Skip_BS(Data_BS_Remain()-End,                   "padding");
            Element_End0();
        }
        Element_End0();
    }

    Presets.resize(num_presets_m1 + 1);
    for (int8u i=0; i<=num_presets_m1; i++)
    {
        preset& P=Presets[i];
        Element_Begin1("preset");
        int32u num_target_device_configs_m1;
        Get_V4 (2, num_target_device_configs_m1,                "num_target_device_configs_m1");
        Get_V4 (2, P.default_target_device_config,              "default_target_device_config"); Param_Info1(default_target_device_config_Value(P.default_target_device_config));
        P.target_device_configs.resize(num_target_device_configs_m1+1);
        for (int32u j=0; j<=num_target_device_configs_m1; j++)
        {
            preset::target_device_config& T=P.target_device_configs[j];
            Element_Begin1("target_device_config");
            Get_V4 (3, T.target_devices_mask,                   "target_devices_mask"); Param_Info1(default_target_device_config_Value(T.target_devices_mask));
            for (int32u k=0; k<num_dyn_objects+num_bed_instances; k++)
            {
                Element_Begin1(k<num_dyn_objects?"object":"bed");
                bool active;
                Get_SB (active,                                 k<num_dyn_objects?"object_active":"bed_active");
                if (active)
                {
                    int32u md_index;
                    size_t Size=k<num_dyn_objects?DynObjects[k].Alts.size():BedInstances[k-num_dyn_objects].Alts.size();
                    Get_S4 (bits_needed(Size+1), md_index, k<num_dyn_objects?"obj_md_index":"bed_md_index");
                    T.md_indexes.push_back(md_index);
                }
                else
                    T.md_indexes.push_back((int32u)-1);
                Element_End0();
            }
            Element_End0();
        }
        TEST_SB_SKIP(                                           "preset_extension");
            int32u extension_size;
            Get_V4 (4, extension_size,                          "extension_size");
            Skip_BS(extension_size,                             "extension");
        TEST_SB_END();
        Element_End0();
    }

    substream_mappings.resize(num_dyn_objects+num_bed_instances);
    if (is_substream_mapping_present)
    {
        Element_Begin1("substream_mapping");
        for (int8u i=0; i<substream_mappings.size(); i++)
        {
            substream_mapping& S=substream_mappings[i];
            Element_Begin1(i<num_dyn_objects?"dyn_object":"bed_instance");
            bool standard_index;
            Get_S1(4, S.substream_id,                           "substream_id");
            Get_SB(   standard_index,                           "standard_index");
            if (standard_index)
                Get_S4 (3, S.channel_index,                     "channel_index");
            else
            {
                Get_S4(5, S.channel_index,                      "channel_index");
                if (S.channel_index==0x1F)
                {
                    Get_V4(3, S.channel_index,                  "channel_index");
                    S.channel_index+=0x1F;
                }
            }

            Element_End0();
        }
        Element_End0();
    }
    int8u byte_align_Content=0;
    {
        size_t ToPadd=(BS_Start-Data_BS_Remain())%8;
        if (ToPadd)
            Get_S1(8-ToPadd, byte_align_Content,                "byte_align");
    }
    if (mini_mixgraph_extension_present)
    {
        Element_Begin1("mini_mixgraph_extension");
        TEST_SB_SKIP(                                           "mgi_extension");
            int32u extension_size;
            Get_V4 (4, extension_size,                          "extension_size");
            Skip_BS(extension_size,                             "extension");
        TEST_SB_END();
        Element_End0();
    }
    if (mini_mixgraph_description_present)
    {
        Element_Begin1("mini_mixgraph_description");
        int32u desc_packet_idx, desc_pkt_size_bits;
        if (!byte_align_Content)
        {
            Get_V4 (5, desc_packet_idx,                         "desc_packet_idx");
        }
        else
        {
            Skip_S1(6,                                          "0x3F?");
            desc_packet_idx = 30; //Not in spec. Bug in the stream here?
        }
        if (!desc_packet_idx)
        {
            Get_V4 (5, num_desc_packets_m1,                     "num_desc_packets_m1");
        }
        if (desc_packet_idx==num_desc_packets_m1)
        {
            Get_V4 (7, desc_pkt_size_bits,                      "desc_pkt_size_bits");
        }
        else
        {
            Get_V4 (4, desc_pkt_size_bits,                      "desc_pkt_size_bytes");
            desc_pkt_size_bits<<=3; //bits to bytes
        }
        while (desc_pkt_size_bits)
        {
            int8u bits8=desc_pkt_size_bits>8?8:desc_pkt_size_bits;
            int8u data;
            Get_S1(bits8, data,                                 "description_packet_data");
            if (num_desc_packets_m1!=(int32u)-1)
                description_packet_data.push_back(data<<(8-bits8));
            desc_pkt_size_bits-=bits8;
        }
        if (desc_packet_idx==num_desc_packets_m1 && !description_packet_data.empty())
        {
            Presets_More.clear();
            BitStream_Fast* BS_Save=BS;
            BS=new BitStream_Fast(&*description_packet_data.begin(), description_packet_data.size()); //In bytes, we can not provide a precise count of bits
            Element_Begin1("packet_description");
            int8u preset_description_id_bits;
            Get_S1(3, preset_description_id_bits,           "preset_description_id_bits");
            for (int8u i=0; i <=num_presets_m1; i++)
            {
                Skip_BS(preset_description_id_bits,         "preset_description_id[i]");
            }
            TEST_SB_SKIP("preset_description_text_present");
                Presets_More.resize(num_presets_m1+1);
                for (int8u i=0; i<=num_presets_m1; i++)
                {
                    preset_more& P=Presets_More[i];
                    int8u desc_text_len;
                    Get_S1(5, desc_text_len,                "desc_text_len");
                    Element_Begin1("preset_description");
                    for (int8u j=0; j<desc_text_len; j++)
                    {
                        int8u preset_description_char;
                        Get_S1(8, preset_description_char,  "preset_description_char");
                        P.description+=(char)preset_description_char;
                    }
                    if (Ztring().From_UTF8(P.description).empty())
                        P.description.clear(); // Problem while parsing
                    Element_Info1(P.description);
                    Element_End0();
                }
            TEST_SB_END();
            Element_End0();
            delete BS; BS=BS_Save;
            description_packet_data.clear();
            mini_mixgraph_description_present=false; //Indicates that the stream is ready for data filling
        }
        Element_End0();
        {
            size_t ToPadd=(BS_Start-Data_BS_Remain())%8;
            if (ToPadd<8)
                Skip_BS(8-ToPadd,                               "byte_align");
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_DolbyE::Get_V4(int8u Bits, int8u MaxLoops, int32u& Info, const char* Name)
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
                if (!BS->GetB() || !(--MaxLoops))
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
                if (!BS->GetB() || !(--MaxLoops))
                    break;
                Info<<=Bits;
                Info+=(1<<Bits);
            }
        }
}

//---------------------------------------------------------------------------
void File_DolbyE::Get_V4(int8u Bits, int32u& Info, const char* Name)
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
void File_DolbyE::Skip_V4(int8u  Bits, const char* Name)
{
    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            int32u Info=0;
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
                BS->Skip(Bits);
                if (!BS->GetB())
                    break;
            }
        }
}

//---------------------------------------------------------------------------
void File_DolbyE::audio_segment()
{
    //Parsing
    Element_Begin1("audio_segment");
    #if MEDIAINFO_TRACE
        //CRC test
        size_t Pos_Begin=0;
    #endif //MEDIAINFO_TRACE
    for (int8u Channel=0; Channel<DolbyE_Channels[program_config]; Channel++)
    {
        if ((Channel%(DolbyE_Channels[program_config]/2))==0 && key_present)
        {
            int16u audio_subsegment_size=0;
            for (int8u ChannelForSize=0; ChannelForSize<DolbyE_Channels[program_config]/2; ChannelForSize++)
                audio_subsegment_size+=channel_subsegment_size[((Channel<DolbyE_Channels[program_config]/2)?0:(DolbyE_Channels[program_config]/2))+ChannelForSize];

            if (Data_BS_Remain()<(audio_subsegment_size+1)*(size_t)bit_depth)
                return; //There is a problem

            //We must change the buffer
            switch (bit_depth)
            {
                case 16 :
                            {
                            int16u audio_subsegment_key;
                            Get_S2 (16, audio_subsegment_key, (Channel+1==DolbyE_Channels[program_config])?"audio_subsegment1_key":"audio_subsegment0_key");

                            int8u* Temp=Descrambled_Buffer+(size_t)Element_Size-Data_BS_Remain()/8;
                            for (int16u Pos=0; Pos<audio_subsegment_size+1; Pos++)
                                int16u2BigEndian(Temp+Pos*2, BigEndian2int16u(Temp+Pos*2)^audio_subsegment_key);
                            }
                            break;
                case 20 :
                            {
                            int32u audio_subsegment_key;
                            Get_S3 (20, audio_subsegment_key, (Channel+1==DolbyE_Channels[program_config])?"audio_subsegment1_key":"audio_subsegment0_key");

                            Descramble_20bit(audio_subsegment_key, audio_subsegment_size);
                            }
                            break;
                default :   ;
            }
        }

        #if MEDIAINFO_TRACE
            //CRC test
            if ((Channel%(DolbyE_Channels[program_config]/2))==0)
                Pos_Begin=Buffer_Offset*8+(size_t)Element_Size*8-Data_BS_Remain();
        #endif //MEDIAINFO_TRACE

        Element_Begin1(__T("Channel ")+Ztring::ToZtring(Channel));
        Element_Info1(Ztring::ToZtring(channel_subsegment_size[Channel])+__T(" words"));
        Skip_BS(channel_subsegment_size[Channel]*bit_depth,     "channel_subsegment");
        Element_End0();
        if ((Channel%(DolbyE_Channels[program_config]/2))==DolbyE_Channels[program_config]/2-1)
        {
            Skip_S3(bit_depth,                                  (Channel+1==DolbyE_Channels[program_config])?"audio_subsegment1_crc":"audio_subsegment0_crc");

            #if MEDIAINFO_TRACE
                //CRC test
                /*
                size_t Pos_End=Buffer_Offset*8+(size_t)Element_Size*8-Data_BS_Remain();
                int8u BitSkip_Begin=Pos_Begin%8;
                Pos_Begin/=8;
                int8u BitSkip_End=0; // Pos_End%8; Looks like that the last bits must not be in the CRC computing
                Pos_End/=8;
                if (BitSkip_End)
                    Pos_End++;

                int16u CRC=CRC_16_Compute(Buffer+Pos_Begin, Pos_End-Pos_Begin, BitSkip_Begin, BitSkip_End);
                if (CRC)
                {
                    //CRC is wrong
                    Param_Info1("NOK");
                }
                */
            #endif //MEDIAINFO_TRACE
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_DolbyE::metadata_extension_segment()
{
    //Parsing
    Element_Begin1("metadata_extension_segment");
    if (key_present)
    {
        if (Data_BS_Remain()<((size_t)metadata_extension_segment_size+1)*(size_t)bit_depth) //+1 for CRC
            return; //There is a problem

        //We must change the buffer
        switch (bit_depth)
        {
            case 16 :
                        {
                        int16u metadata_extension_segment_key;
                        Get_S2 (16, metadata_extension_segment_key, "metadata_extension_segment_key");

                        int8u* Temp=Descrambled_Buffer+(size_t)Element_Size-Data_BS_Remain()/8;
                        for (int16u Pos=0; Pos<metadata_extension_segment_size+1; Pos++)
                            int16u2BigEndian(Temp+Pos*2, BigEndian2int16u(Temp+Pos*2)^metadata_extension_segment_key);
                        }
                        break;
            case 20 :
                        {
                        int32u metadata_extension_segment_key;
                        Get_S3 (20, metadata_extension_segment_key, "metadata_extension_segment_key");

                        Descramble_20bit(metadata_extension_segment_key, metadata_extension_segment_size);
                        }
                        break;
            default :   ;
        }
    }

    #if MEDIAINFO_TRACE
        //CRC test
        size_t Pos_Begin=Buffer_Offset*8+(size_t)Element_Size*8-Data_BS_Remain();
    #endif //MEDIAINFO_TRACE

    size_t  metadata_extension_segment_BitCountAfter=Data_BS_Remain();
    metadata_extension_segment_BitCountAfter-=metadata_extension_segment_size*bit_depth;
    if (metadata_extension_segment_size)
    {
        for(;;)
        {
            Element_Begin1("metadata_extension_subsegment");
            int16u  metadata_extension_subsegment_length;
            int8u   metadata_extension_subsegment_id;
            Get_S1 ( 4, metadata_extension_subsegment_id,       "metadata_extension_subsegment_id");
            if (metadata_extension_subsegment_id==0)
            {
                Element_End0();
                break;
            }
            Get_S2 (12, metadata_extension_subsegment_length,   "metadata_extension_subsegment_length");
            switch (metadata_extension_subsegment_id)
            {
                default: Skip_BS(metadata_extension_subsegment_length,"metadata_extension_subsegment (unknown)");
            }
            Element_End0();
        }
        Param_Info1(metadata_extension_segment_BitCountAfter);
        Param_Info1(Data_BS_Remain());
        Param_Info1(Data_BS_Remain()-metadata_extension_segment_BitCountAfter);
        if (Data_BS_Remain()>metadata_extension_segment_BitCountAfter)
            Skip_BS(Data_BS_Remain()-metadata_extension_segment_BitCountAfter,"reserved_metadata_extension_bits");
    }
    Skip_S3(bit_depth,                                          "metadata_extension_crc");

    #if MEDIAINFO_TRACE
        //CRC test
        /*
        size_t Pos_End=Buffer_Offset*8+(size_t)Element_Size*8-Data_BS_Remain();
        int8u BitSkip_Begin=Pos_Begin%8;
        Pos_Begin/=8;
        int8u BitSkip_End=0; // Pos_End%8; Looks like that the last bits must not be in the CRC computing
        Pos_End/=8;
        if (BitSkip_End)
            Pos_End++;

        int16u CRC=CRC_16_Compute(Buffer+Pos_Begin, Pos_End-Pos_Begin, BitSkip_Begin, BitSkip_End);
        if (CRC)
        {
            //CRC is wrong
            Param_Info1("NOK");
        }
        */
    #endif //MEDIAINFO_TRACE

    Element_End0();
}

//---------------------------------------------------------------------------
void File_DolbyE::audio_extension_segment()
{
    //Parsing
    Element_Begin1("audio_extension_segment");
    #if MEDIAINFO_TRACE
        //CRC test
        size_t Pos_Begin=0;
    #endif //MEDIAINFO_TRACE
    for (int8u Channel=0; Channel<DolbyE_Channels[program_config]; Channel++)
    {
        if ((Channel%(DolbyE_Channels[program_config]/2))==0 && key_present)
        {
            int16u audio_extension_subsegment_size=0;
            for (int8u ChannelForSize=0; ChannelForSize<DolbyE_Channels[program_config]/2; ChannelForSize++)
                audio_extension_subsegment_size+=channel_subsegment_size[((Channel<DolbyE_Channels[program_config]/2)?0:(DolbyE_Channels[program_config]/2))+ChannelForSize];

            if (Data_BS_Remain()<((size_t)audio_extension_subsegment_size+1)*(size_t)bit_depth)
                return; //There is a problem

            //We must change the buffer
            switch (bit_depth)
            {
                case 16 :
                            {
                            int16u audio_extension_subsegment_key;
                            Get_S2 (16, audio_extension_subsegment_key, (Channel+1==DolbyE_Channels[program_config])?"audio_extension_subsegment1_key":"audio_extension_subsegment0_key");

                            int8u* Temp=Descrambled_Buffer+(size_t)Element_Size-Data_BS_Remain()/8;
                            for (int16u Pos=0; Pos<audio_extension_subsegment_size+1; Pos++)
                                int16u2BigEndian(Temp+Pos*2, BigEndian2int16u(Temp+Pos*2)^audio_extension_subsegment_key);
                            }
                            break;
                case 20 :
                            {
                            int32u audio_extension_subsegment_key;
                            Get_S3 (20, audio_extension_subsegment_key, (Channel+1==DolbyE_Channels[program_config])?"audio_extension_subsegment1_key":"audio_extension_subsegment0_key");

                            Descramble_20bit(audio_extension_subsegment_key, audio_extension_subsegment_size);
                            }
                            break;
                default :   ;
            }
        }

        #if MEDIAINFO_TRACE
            //CRC test
            if ((Channel%(DolbyE_Channels[program_config]/2))==0)
                Pos_Begin=Buffer_Offset*8+(size_t)Element_Size*8-Data_BS_Remain();
        #endif //MEDIAINFO_TRACE

        Element_Begin1(__T("Channel ")+Ztring::ToZtring(Channel));
        Element_Info1(Ztring::ToZtring(channel_subsegment_size[Channel])+__T(" words"));
        Skip_BS(channel_subsegment_size[Channel]*bit_depth,     "channel_subsegment");
        Element_End0();
        if ((Channel%(DolbyE_Channels[program_config]/2))==DolbyE_Channels[program_config]/2-1)
        {
            Skip_S3(bit_depth,                                  (Channel+1==DolbyE_Channels[program_config])?"audio_extension_subsegment1_crc":"audio_extension_subsegment0_crc");

            #if MEDIAINFO_TRACE
                //CRC test
                /*
                size_t Pos_End=Buffer_Offset*8+(size_t)Element_Size*8-Data_BS_Remain();
                int8u BitSkip_Begin=Pos_Begin%8;
                Pos_Begin/=8;
                int8u BitSkip_End=0; // Pos_End%8; Looks like that the last bits must not be in the CRC computing
                Pos_End/=8;
                if (BitSkip_End)
                    Pos_End++;

                int16u CRC=CRC_16_Compute(Buffer+Pos_Begin, Pos_End-Pos_Begin, BitSkip_Begin, BitSkip_End);
                if (CRC)
                {
                    //CRC is wrong
                    Param_Info1("NOK");
                }
                */
            #endif //MEDIAINFO_TRACE
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_DolbyE::meter_segment()
{
    //Parsing
    Element_Begin1("meter_segment");
    if (key_present)
    {
        if (Data_BS_Remain()<((size_t)meter_segment_size+1)*(size_t)bit_depth) //+1 for CRC
            return; //There is a problem

        //We must change the buffer
        switch (bit_depth)
        {
            case 16 :
                        {
                        int16u meter_segment_key;
                        Get_S2 (16, meter_segment_key, "meter_segment_key");

                        int8u* Temp=Descrambled_Buffer+(size_t)Element_Size-Data_BS_Remain()/8;
                        for (int16u Pos=0; Pos<meter_segment_size+1; Pos++)
                            int16u2BigEndian(Temp+Pos*2, BigEndian2int16u(Temp+Pos*2)^meter_segment_key);
                        }
                        break;
            case 20 :
                        {
                        int32u meter_segment_key;
                        Get_S3 (20, meter_segment_key, "meter_segment_key");

                        Descramble_20bit(meter_segment_key, meter_segment_size);
                        }
                        break;
            default :   ;
        }
    }
    size_t  meter_segment_BitCountAfter=Data_BS_Remain();
    meter_segment_BitCountAfter-=meter_segment_size*bit_depth;
    for (int8u Channel=0; Channel<DolbyE_Channels[program_config]; Channel++)
        Skip_S2(10,                                             "peak_meter");
    for (int8u Channel=0; Channel<DolbyE_Channels[program_config]; Channel++)
        Skip_S2(10,                                             "rms_meter");
    if (Data_BS_Remain()>meter_segment_BitCountAfter)
        Skip_BS(Data_BS_Remain()>meter_segment_BitCountAfter,   "reserved_meter_bits");
    Skip_S3(bit_depth,                                          "meter_crc");

    #if MEDIAINFO_TRACE
        //CRC test
        /*
        size_t Pos_End=Buffer_Offset*8+(size_t)Element_Size*8-Data_BS_Remain();
        size_t Pos_Begin=Pos_End-(meter_segment_size+1)*bit_depth; //+1 for CRC
        int8u BitSkip_Begin=Pos_Begin%8;
        Pos_Begin/=8;
        int8u BitSkip_End=0; // Pos_End%8; Looks like that the last bits must not be in the CRC computing
        Pos_End/=8;
        if (BitSkip_End)
            Pos_End++;

        int16u CRC=CRC_16_Compute(Buffer+Pos_Begin, Pos_End-Pos_Begin, BitSkip_Begin, BitSkip_End);
        if (CRC)
        {
            //CRC is wrong
            Param_Info1("NOK");
        }
        */
    #endif //MEDIAINFO_TRACE

    Element_End0();
}

//---------------------------------------------------------------------------
enum ac3 {
    ac3_datarate,
    ac3_bsmod,
    ac3_acmod,
    ac3_cmixlev,
    ac3_surmixlev,
    ac3_dsurmod,
    ac3_lfeon,
    ac3_dialnorm,
    ac3_langcode,
    ac3_langcod,
    ac3_audprodie,
    ac3_mixlevel,
    ac3_roomtyp,
    ac3_copyrightb,
    ac3_origbs,
    ac3_xbsi1e,
    ac3_dmixmod,
    ac3_ltrtcmixlev,
    ac3_ltrtsurmixlev,
    ac3_lorocmixlev,
    ac3_lorosurmixlev,
    ac3_xbsi2e,
    ac3_dsurexmod,
    ac3_dheadphonmod,
    ac3_adconvtyp,
    ac3_xbsi2,
    ac3_encinfo,
    ac3_hpfon,
    ac3_bwlpfon,
    ac3_lfelpfon,
    ac3_sur90on,
    ac3_suratton,
    ac3_rfpremphon,
    ac3_compre,
    ac3_compr1,
    ac3_dynrnge,
    ac3_dynrng1,
    ac3_dynrng2,
    ac3_dynrng3,
    ac3_dynrng4,
    ac3_max
};
void File_DolbyE::ac3_metadata_subsegment(bool xbsi)
{
    for (int8u program=0; program<DolbyE_Programs[program_config]; program++)
    {
        int8u meta[ac3_max];
        Element_Begin1("per program");
        Get_S1 (5, meta[ac3_datarate],                      "ac3_datarate"); Param_Info2C(meta[ac3_datarate]<19, AC3_BitRate[meta[ac3_datarate]], " kbps");
        Get_S1 (3, meta[ac3_bsmod],                         "ac3_bsmod"); Param_Info1(AC3_Mode_String[meta[ac3_bsmod]]);
        Get_S1 (3, meta[ac3_acmod],                         "ac3_acmod"); Param_Info1(AC3_ChannelLayout_lfeoff[meta[ac3_acmod]]);
        Get_S1 (2, meta[ac3_cmixlev],                       "ac3_cmixlev"); Param_Info2C(meta[ac3_cmixlev]<=2, Ztring::ToZtring(-3 - ((float)meta[ac3_cmixlev]) * 1.5, 1).To_UTF8(), " dB");
        Get_S1 (2, meta[ac3_surmixlev],                     "ac3_surmixlev"); Param_Info2C(meta[ac3_surmixlev]<=2, meta[ac3_surmixlev]==2?string("-inf"):to_string(-3 - (int)meta[ac3_cmixlev] * 3), " dB");
        Get_S1 (2, meta[ac3_dsurmod],                       "ac3_dsurmod"); Param_Info1C(meta[ac3_dsurmod], "Dolby Surround");
        Get_S1 (1, meta[ac3_lfeon],                         "ac3_lfeon"); Param_Info1C(meta[ac3_lfeon], "LFE");
        Get_S1 (5, meta[ac3_dialnorm],                      "ac3_dialnorm"); Param_Info2(Ztring::ToZtring(meta[ac3_dialnorm]==0?-31:-(int)meta[ac3_dialnorm]).To_UTF8(), " dB");
        Get_S1 (1, meta[ac3_langcode],                      "ac3_langcode");
        Get_S1 (8, meta[ac3_langcod],                       "ac3_langcod");
        Get_S1 (1, meta[ac3_audprodie],                     "ac3_audprodie");
        Get_S1 (5, meta[ac3_mixlevel],                      "ac3_mixlevel"); Param_Info2C(meta[ac3_audprodie], 80 + meta[ac3_mixlevel], " dB");
        Get_S1 (2, meta[ac3_roomtyp],                       "ac3_roomtyp"); Param_Info1C(meta[ac3_audprodie] && meta[ac3_roomtyp], AC3_roomtyp[meta[ac3_roomtyp] - 1]);
        Get_S1 (1, meta[ac3_copyrightb],                    "ac3_copyrightb");
        Get_S1 (1, meta[ac3_origbs],                        "ac3_origbs");
        if (xbsi)
        {
            Get_S1 (1, meta[ac3_xbsi1e],                    "ac3_xbsi1e");
            Get_S1 (2, meta[ac3_dmixmod],                   "ac3_dmixmod");
            Get_S1 (3, meta[ac3_ltrtcmixlev],               "ac3_ltrtcmixlev"); Param_Info2C(meta[ac3_xbsi1e], AC3_Level_Value(meta[ac3_ltrtcmixlev], 3, 1.5), " dB");
            Get_S1 (3, meta[ac3_ltrtsurmixlev],             "ac3_ltrtsurmixlev"); Param_Info2C(meta[ac3_xbsi1e], AC3_Level_Value(meta[ac3_ltrtsurmixlev], 3, 1.5), " dB");
            Get_S1 (3, meta[ac3_lorocmixlev],               "ac3_lorocmixlev"); Param_Info2C(meta[ac3_xbsi1e], AC3_Level_Value(meta[ac3_lorocmixlev], 3, 1.5), " dB");
            Get_S1 (3, meta[ac3_lorosurmixlev],             "ac3_lorosurmixlev"); Param_Info2C(meta[ac3_xbsi1e], AC3_Level_Value(meta[ac3_lorosurmixlev], 3, 1.5), " dB");
            Get_S1 (1, meta[ac3_xbsi2e],                    "ac3_xbsi2e");
            Get_S1 (2, meta[ac3_dsurexmod],                 "ac3_dsurexmod"); Param_Info1C(meta[ac3_dsurexmod]>=2, meta[ac3_dsurexmod]==2?"Dolby Surround EX":"Dolby Pro Logic IIz");
            Get_S1 (2, meta[ac3_dheadphonmod],              "ac3_dheadphonmod"); Param_Info1C(meta[ac3_dheadphonmod], "Dolby Headphone");
            Get_S1 (1, meta[ac3_adconvtyp],                 "ac3_adconvtyp"); Param_Info1C(meta[ac3_xbsi2e] && meta[ac3_adconvtyp], "HDCD");
            Get_S1 (8, meta[ac3_xbsi2],                     "ac3_xbsi2");
            Get_S1 (1, meta[ac3_encinfo],                   "ac3_encinfo");
        }
        else
        {
            Skip_S1(1,                                      "ac3_timecode1e");
            Skip_S2(14,                                     "ac3_timecode1");
            Skip_S1(1,                                      "ac3_timecode2e");
            Skip_S2(14,                                     "ac3_timecode2");
        }
        Get_S1 (1, meta[ac3_hpfon],                         "ac3_hpfon");
        Get_S1 (1, meta[ac3_bwlpfon],                       "ac3_bwlpfon");
        Get_S1 (1, meta[ac3_lfelpfon],                      "ac3_lfelpfon");
        Get_S1 (1, meta[ac3_sur90on],                       "ac3_sur90on");
        Get_S1 (1, meta[ac3_suratton],                      "ac3_suratton");
        Get_S1 (1, meta[ac3_rfpremphon],                    "ac3_rfpremphon");
        Get_S1 (1, meta[ac3_compre],                        "ac3_compre");
        Get_S1 (8, meta[ac3_compr1],                        "ac3_compr1"); Param_Info2C(meta[ac3_compre], Ztring::ToZtring(meta[ac3_dynrng1]?(AC3_compr[meta[ac3_compr1]>>4]+20*std::log10(((float)(0x10+(meta[ac3_compr1]&0x0F)))/32)):0, 2), " dB");
        Get_S1 (1, meta[ac3_dynrnge],                       "ac3_dynrnge");
        Get_S1 (8, meta[ac3_dynrng1],                       "ac3_dynrng1"); Param_Info2C(meta[ac3_dynrnge], Ztring::ToZtring(meta[ac3_dynrng1]?(AC3_dynrng[meta[ac3_dynrng1]>>5]+20*std::log10(((float)(0x20+(meta[ac3_dynrng1]&0x1F)))/64)):0, 2), " dB");
        Get_S1 (8, meta[ac3_dynrng2],                       "ac3_dynrng2"); Param_Info2C(meta[ac3_dynrnge], Ztring::ToZtring(meta[ac3_dynrng2]?(AC3_dynrng[meta[ac3_dynrng2]>>5]+20*std::log10(((float)(0x20+(meta[ac3_dynrng2]&0x1F)))/64)):0, 2), " dB");
        Get_S1 (8, meta[ac3_dynrng3],                       "ac3_dynrng3"); Param_Info2C(meta[ac3_dynrnge], Ztring::ToZtring(meta[ac3_dynrng3]?(AC3_dynrng[meta[ac3_dynrng3]>>5]+20*std::log10(((float)(0x20+(meta[ac3_dynrng3]&0x1F)))/64)):0, 2), " dB");
        Get_S1 (8, meta[ac3_dynrng4],                       "ac3_dynrng4"); Param_Info2C(meta[ac3_dynrnge], Ztring::ToZtring(meta[ac3_dynrng4]?(AC3_dynrng[meta[ac3_dynrng4]>>5]+20*std::log10(((float)(0x20+(meta[ac3_dynrng4]&0x1F)))/64)):0, 2), " dB");
        Element_End0();

        FILLING_BEGIN()
            if (!Frame_Count)
            {
                if (program >= Count_Get(Stream_Audio))
                    Stream_Prepare(Stream_Audio);
                Fill(Stream_Audio, program, "AC3_Metadata", "Yes");
                if (meta[ac3_datarate]<19)
                {
                    Fill(Stream_Audio, program, "AC3_Metadata BitRate", AC3_BitRate[meta[ac3_datarate]]*1000);
                    Fill_SetOptions(Stream_Audio, program, "AC3_Metadata BitRate", "N NTY");
                    Fill(Stream_Audio, program, "AC3_Metadata BitRate/String", Ztring::ToZtring(AC3_BitRate[meta[ac3_datarate]])+__T(" kbps"));
                    Fill_SetOptions(Stream_Audio, program, "AC3_Metadata BitRate/String", "Y NTN");
                }
                Fill(Stream_Audio, program, "AC3_Metadata ServiceKind", AC3_Mode[meta[ac3_bsmod]]);
                Fill_SetOptions(Stream_Audio, program, "AC3_Metadata ServiceKind", "N NTY");
                Fill(Stream_Audio, program, "AC3_Metadata ServiceKind/String", AC3_Mode_String[meta[ac3_bsmod]]);
                Fill_SetOptions(Stream_Audio, program, "AC3_Metadata ServiceKind/String", "Y NTN");
                int8u Channels = AC3_Channels[meta[ac3_acmod]];
                Ztring ChannelPositions; ChannelPositions.From_UTF8(AC3_ChannelPositions[meta[ac3_acmod]]);
                Ztring ChannelPositions2; ChannelPositions2.From_UTF8(AC3_ChannelPositions2[meta[ac3_acmod]]);
                Ztring ChannelLayout; ChannelLayout.From_UTF8(meta[ac3_lfeon] ? AC3_ChannelLayout_lfeon[meta[ac3_acmod]] : AC3_ChannelLayout_lfeoff[meta[ac3_acmod]]);
                if (meta[ac3_lfeon])
                {
                    Channels += 1;
                    ChannelPositions += __T(", LFE");
                    ChannelPositions2 += __T(".1");
                }
                Fill(Stream_Audio, program, "AC3_Metadata ChannelLayout", ChannelLayout);

                //Surround
                if (meta[ac3_dsurmod]==2)
                {
                    Fill(Stream_Audio, program, Audio_Format_Settings, "Dolby Surround");
                }
                if (meta[ac3_dsurexmod]==2)
                {
                    Fill(Stream_Audio, program, Audio_Format_Settings, "Dolby Surround EX");
                }
                if (meta[ac3_dsurexmod]==3)
                {
                    Fill(Stream_Audio, program, Audio_Format_Settings, "Dolby Pro Logic IIz");
                }
                if (meta[ac3_dheadphonmod]==2)
                {
                    Fill(Stream_Audio, program, Audio_Format_Settings, "Dolby Headphone");
                }

                // Metadata
                Fill(Stream_Audio, program, "AC3_Metadata dialnorm", meta[ac3_dialnorm]==0?-31:-(int)meta[ac3_dialnorm]);
                Fill_SetOptions(Stream_Audio, program, "AC3_Metadata dialnorm", "N NT");
                Fill(Stream_Audio, program, "AC3_Metadata dialnorm/String", Ztring::ToZtring(meta[ac3_dialnorm]==0?-31:-(int)meta[ac3_dialnorm])+__T(" dB"));
                Fill_SetOptions(Stream_Audio, program, "AC3_Metadata dialnorm/String", "Y NTN");
                if (meta[ac3_compre])
                {
                    float64 Value;
                    if (meta[ac3_compr1]==0)
                        Value=0; //Special case in the formula
                    else
                        Value=AC3_compr[meta[ac3_compr1]>>4]+20*std::log10(((float)(0x10+(meta[ac3_compr1]&0x0F)))/32);
                    Fill(Stream_Audio, program, "AC3_Metadata compr", Value, 2);
                    Fill_SetOptions(Stream_Audio, program, "AC3_Metadata compr", "N NT");
                    Fill(Stream_Audio, program, "AC3_Metadata compr/String", Ztring::ToZtring(Value, 2)+__T(" dB"));
                    Fill_SetOptions(Stream_Audio, program, "AC3_Metadata compr/String", "Y NTN");
                }
                else if (meta[ac3_compr1])
                {
                    Fill(Stream_Audio, program, "AC3_Metadata comprprof", AC3_dynrngprof_Get(meta[ac3_compr1]));
                    Fill_SetOptions(Stream_Audio, program, "AC3_Metadata comprprof", "Y NT");
                }
                if (meta[ac3_dynrnge])
                {
                    float64 Value;
                    if (meta[ac3_dynrng1]==0)
                        Value=0; //Special case in the formula
                    else
                        Value=AC3_dynrng[meta[ac3_dynrng1]>>5]+20*std::log10(((float)(0x20+(meta[ac3_dynrng1]&0x1F)))/64);
                    Fill(Stream_Audio, program, "AC3_Metadata dynrng", Value, 2);
                    Fill_SetOptions(Stream_Audio, program, "AC3_Metadata dynrng", "N NT");
                    Fill(Stream_Audio, program, "AC3_Metadata dynrng/String", Ztring::ToZtring(Value, 2)+__T(" dB"));
                    Fill_SetOptions(Stream_Audio, program, "AC3_Metadata dynrng/String", "Y NTN");
                }
                else if (meta[ac3_dynrng1])
                {
                    Fill(Stream_Audio, program, "AC3_Metadata dynrngprof", AC3_dynrngprof_Get(meta[ac3_dynrng1]));
                    Fill_SetOptions(Stream_Audio, program, "AC3_Metadata dynrngprof", "Y NT");
                }

                // Other metadata
                if (meta[ac3_cmixlev]<=2)
                {
                    string Value = Ztring::ToZtring(-3 - ((float)meta[ac3_cmixlev]) * 1.5, 1).To_UTF8();
                    Fill(Stream_Audio, program, "AC3_Metadata cmixlev", Value);
                    Fill_SetOptions(Stream_Audio, program, "AC3_Metadata cmixlev", "N NT");
                    Fill(Stream_Audio, program, "AC3_Metadata cmixlev/String", Value + " dB");
                    Fill_SetOptions(Stream_Audio, program, "AC3_Metadata cmixlev/String", "Y NTN");
                }
                if (meta[ac3_surmixlev]<=2)
                {
                    string Value = (meta[ac3_surmixlev]==2?string("-inf"):to_string(-3 - (int)meta[ac3_cmixlev] * 3));
                    Fill(Stream_Audio, program, "AC3_Metadata surmixlev", Value + " dB");
                    Fill_SetOptions(Stream_Audio, program, "AC3_Metadata surmixlev", "N NT");
                    Fill(Stream_Audio, program, "AC3_Metadata surmixlev/String", Value + " dB");
                    Fill_SetOptions(Stream_Audio, program, "AC3_Metadata surmixlev/String", "Y NTN");
                }
                if (meta[ac3_audprodie])
                {
                    string Value = to_string(80 + meta[ac3_mixlevel]);
                    Fill(Stream_Audio, program, "AC3_Metadata mixlevel", Value);
                    Fill_SetOptions(Stream_Audio, program, "AC3_Metadata mixlevel", "N NT");
                    Fill(Stream_Audio, program, "AC3_Metadata mixlevel/String", Value + " dB");
                    Fill_SetOptions(Stream_Audio, program, "AC3_Metadata mixlevel/String", "Y NTN");
                    if (meta[ac3_roomtyp]) {
                        Fill(Stream_Audio, program, "AC3_Metadata roomtyp", AC3_roomtyp[meta[ac3_roomtyp] - 1]);
                        Fill_SetOptions(Stream_Audio, program, "AC3_Metadata roomtyp", "Y NTY");
                    }
                }
                if (xbsi) {
                    if (meta[ac3_xbsi1e]) {
                        if (meta[ac3_dmixmod]) {
                            Fill(Stream_Audio, program, "AC3_Metadata dmixmod", AC3_dmixmod[meta[ac3_dmixmod] - 1]);
                            Fill_SetOptions(Stream_Audio, program, "AC3_Metadata dmixmod", "Y NTY");
                        }
                        AC3_Level_Fill(this, program, meta[ac3_ltrtcmixlev], 3, 1.5, "AC3_Metadata ltrtcmixlev");
                        AC3_Level_Fill(this, program, meta[ac3_ltrtsurmixlev], 3, 1.5, "AC3_Metadata ltrtsurmixlev");
                        AC3_Level_Fill(this, program, meta[ac3_lorocmixlev], 3, 1.5, "AC3_Metadata lorocmixlev");
                        AC3_Level_Fill(this, program, meta[ac3_lorosurmixlev], 3, 1.5, "AC3_Metadata lorosurmixlev");
                    }
                    if (meta[ac3_xbsi2e] && meta[ac3_adconvtyp]) {
                        Fill(Stream_Audio, program, "AC3_Metadata adconvtyp", "HDCD");
                        Fill_SetOptions(Stream_Audio, program, "AC3_Metadata adconvtyp", "Y NTY");
                    }
                }
                Fill(Stream_Audio, program, "AC3_Metadata hpfon", meta[ac3_hpfon] ? "Yes" : "No");
                Fill(Stream_Audio, program, "AC3_Metadata bwlpfon", meta[ac3_bwlpfon] ? "Yes" : "No");
                Fill(Stream_Audio, program, "AC3_Metadata lfelpfon", meta[ac3_lfelpfon] ? "Yes" : "No");
                Fill(Stream_Audio, program, "AC3_Metadata sur90on", meta[ac3_sur90on] ? "Yes" : "No");
                Fill(Stream_Audio, program, "AC3_Metadata suratton", meta[ac3_suratton] ? "Yes" : "No");
                Fill(Stream_Audio, program, "AC3_Metadata rfpremphon", meta[ac3_rfpremphon] ? "Yes" : "No");
            }
        FILLING_END()

    }
    for (int8u program=0; program<DolbyE_Programs[program_config]; program++)
    {
        Element_Begin1("per program");
        bool ac3_addbsie;
        Get_SB (   ac3_addbsie,                             "ac3_addbsie");
        if (ac3_addbsie)
        {
            int8u ac3_addbsil;
            Get_S1 (6, ac3_addbsil,                         "ac3_addbsil");
            for (int8u Pos=0; Pos<ac3_addbsil+1; Pos++)
                Skip_S1(8,                                  "ac3_addbsi[x]");
        }
        Element_End0();
    }
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void File_DolbyE::Descramble_20bit (int32u key, int16u size)
{
    int8u* Temp=Descrambled_Buffer+(size_t)Element_Size-Data_BS_Remain()/8;
    int64u keys=(((int64u)key)<<20)|key;
    bool Half;
    if (Data_BS_Remain()%8)
    {
        Temp--;
        int24u2BigEndian(Temp, BigEndian2int24u(Temp)^(key));
        Half=true;
    }
    else
        Half=false;
    for (int16u Pos=0; Pos<size-(Half?1:0); Pos+=2)
        int40u2BigEndian(Temp+(Half?3:0)+Pos*5/2, BigEndian2int40u(Temp+(Half?3:0)+Pos*5/2)^keys);
    if ((size-((size && Half)?1:0))%2==0)
        int24u2BigEndian(Temp+(Half?3:0)+(size-((size && Half)?1:0))*5/2, BigEndian2int24u(Temp+(Half?3:0)+(size-((size && Half)?1:0))*5/2)^(key<<4));
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_DOLBYE_YES

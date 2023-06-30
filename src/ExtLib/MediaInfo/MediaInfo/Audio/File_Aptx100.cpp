/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
// Informal documentation and tests by spoRv
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
#if defined(MEDIAINFO_APTX100_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Aptx100.h"
#include "MediaInfo/TimeCode.h"
#include <algorithm>
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Utils
//***************************************************************************

//---------------------------------------------------------------------------
static unsigned BCD_to_Decimal(unsigned Value)
{
    int Tens=Value>>4;
    int Units=Value&0xF;
    if (Tens>=10 || Units>=10)
        return (unsigned)-1;
    return Tens*10+Units;
}

//---------------------------------------------------------------------------
static char ascii_to_lower(char v)
{
    if (v >= 'A' && v <= 'Z')
        v += ('z' - 'Z');
    return v;
}

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
enum aptx100_commercial
{
    dts_sv,
    dts_stereo,
    dts_70_sv,
    dts_70,
    dts_es,
    dts_6,
    dts_max
};
static const char* Aptx100_Commercial[] = 
{
    "DTS Special Venue",
    "DTS Stereo",
    "DTS 70 mm Special Venue",
    "DTS 70 mm",
    "DTS-ES 35 mm",
    "DTS 35 mm",
};
static_assert(sizeof(Aptx100_Commercial) / sizeof(decltype(*Aptx100_Commercial)) == dts_max, "");

//---------------------------------------------------------------------------
// Usually it is compatible with ISO and IETF BCP 47, but not always, listing exceptions
struct language_mapping
{
    const char* From;
    const char* To;
};
static const language_mapping Aptx100_Languages[] = // Must be sorted
{
    { "ARB" , "ar" },
    { "CAN" , "yue" },
    { "CRO" , "hr" },
    { "CZC" , "cs" },
    { "ENGL", "en" },           // No idea about the difference with ENG
    { "FLM" , "nl-BE" },
    { "FRN" , "fr" },
    { "FRNC", "fr-CA" },
    { "GERS", "de-CH" },
    { "GRK" , "el" },
    { "ITL" , "it" },
    { "JAP" , "ja" },
    { "PORB", "pt-BR" },
    { "SLO" , "sl" },
    { "SLV" , "sk" },
    { "SPNC", "es" },
    { "SPNL", "es-419" },
    { "SWD" , "sv" },
    { "TRK" , "tr" },
    { "VTN" , "vi" },
    { "YUG" , "-YU" },          // Which language from  https://en.wikipedia.org/wiki/Languages_of_Yugoslavia#Official_languages ?
};
static inline bool operator< (const language_mapping& l, const char* r)
{
    return strcmp(l.From, r) < 0;
}
static inline bool operator< (const char* l, const language_mapping& r)
{
    return strcmp(l, r.From) < 0;
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
void File_Aptx100::FileHeader_Parse()
{
    //Parsing
    if (File_Size<0x5C)
    {
        Reject();
        return;
    }
    if (Buffer_Size<0x5C)
    {
        Element_WaitForMoreData();
        return;
    }
    for (int i=0; i<60+8+7; i++)
        if ((Buffer[i] && Buffer[i]<0x20) || Buffer[i]>=0x7F)
        {
            Reject();
            return;
        }
    string Title, Language, Studio;
    int16u Serial, Zero_4C, ReelNumber, ChannelCount;
    int8u DiscNumber, Start_HH, Start_MM, Start_SS, Start_FF, End_HH, End_MM, End_SS, End_FF;
    auto Get_String =[&](size_t MaxSize, string& Value, const char* Name)
    {
        auto IsParsed=false;
        auto Max=Element_Offset+MaxSize;
        for (;;)
        {
            auto Pos=Element_Offset;
            while (Pos<Max && Buffer[(size_t)Pos])
                Pos++;
            auto Size=Pos-Element_Offset;
            if (IsParsed)
                Skip_String(Size,                               "(Phantom string?)");
            else
            {
                IsParsed=true;
                File__Analyze::Get_String (Size, Value,         Name);
            }
            if (Element_Offset>=Max)
                return;
            Element_Offset++;
            while (Element_Offset<Max && !Buffer[(size_t)Element_Offset])
                Element_Offset++;
        }
    };
    Get_String (60, Title,                                      "Title");
    Get_String ( 8, Language,                                   "Language");
    Get_String ( 7, Studio,                                     "Studio");
    Get_L1 (DiscNumber,                                         "Disc number");
    Get_L2 (Zero_4C,                                            "0");
    Get_L2 (ReelNumber,                                         "Reel number");
    Get_L2 (Serial,                                             "Serial");
    Get_L2 (ChannelCount,                                       "Channel count");
    Get_L1 (Start_FF,                                           "Start, deciseconds (BCD)");
    Get_L1 (Start_SS,                                           "Start, seconds (BCD)");
    Get_L1 (Start_MM,                                           "Start, minutes (BCD)");
    Get_L1 (Start_HH,                                           "Start, hours (BCD)");
    Get_L1 (End_FF,                                             "End, deciseconds (BCD)");
    Get_L1 (End_SS,                                             "End, seconds (BCD)");
    Get_L1 (End_MM,                                             "End, minutes (BCD)");
    Get_L1 (End_HH,                                             "End, hours (BCD)");

    //Coherancy
    bool IsNok=false;
    Start_FF=BCD_to_Decimal(Start_FF);
    Start_SS=BCD_to_Decimal(Start_SS>0x60?(Start_SS-0x60):Start_SS);
    Start_MM=BCD_to_Decimal(Start_MM>0x60?(Start_MM-0x60):Start_MM);
    Start_HH=BCD_to_Decimal(Start_HH);
    End_FF=BCD_to_Decimal(End_FF);
    End_SS=BCD_to_Decimal(End_SS>0x60?(End_SS-0x60):End_SS);
    End_MM=BCD_to_Decimal(End_MM>0x60?(End_MM-0x60):End_MM);
    End_HH=BCD_to_Decimal(End_HH);
    switch (DiscNumber)
    {
        case 0x00:
        case 0x01: // Disc A or DVD or USB
        case 0x81: // Disc B
            break;
        default:
            IsNok=true;
    }
    switch (ChannelCount)
    {
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 8:
            break;
        default:
            IsNok=true;
    }
    if (Zero_4C)
        IsNok=true;
    if (Start_FF>99 || Start_SS>59 || Start_MM>59 || Start_HH>23)
        IsNok=true;
    if (End_FF>99 || End_SS>59 || End_MM>59 || End_HH>23)
        IsNok=true;
    auto Start=TimeCode(Start_HH, Start_MM, Start_SS, Start_FF, 99, TimeCode::Timed());
    auto End=TimeCode(End_HH, End_MM, End_SS, End_FF, 99, TimeCode::Timed());
    auto Start_ms=Start.ToMilliseconds();
    auto End_ms=End.ToMilliseconds();
    if (!Start.IsSet() || !End.IsSet() || Start_ms>=End_ms)
        IsNok=true;
    if (IsNok)
    {
        Reject();
        return;
    }
    End--;
    int64s Duration_ms=End_ms-Start_ms;


    //Filling
    Accept();
    Fill(Stream_General, 0, General_Format, "APT-X100");
    Fill(Stream_General, 0, General_Movie, Title);
    Fill(Stream_General, 0, General_Title, Title);
    if (Studio=="none")
        Studio.clear();
    Fill(Stream_General, 0, General_ProductionStudio, Studio);
    if (DiscNumber)
        Fill(Stream_General, 0, General_Part_Position, (DiscNumber>>7)+1);
    if (ReelNumber)
        Fill(Stream_General, 0, General_Reel_Position, ReelNumber);
    if (Serial)
        Fill(Stream_General, 0, General_CatalogNumber, Serial);
    Stream_Prepare(Stream_Audio);
    Fill(Stream_Audio, 0, Audio_Format, "APT-X100");
    Fill(Stream_Audio, 0, Audio_BitRate, 44100*4*ChannelCount); // Must be before addition of matrixed channels. 16-bit, 1:4 compression ratio, so 4-bit/channel, before cross overs and matrix
    string ChannelLayout;
    string Settings;
    auto Commercial=dts_max;
    switch (ChannelCount)
    {
        default:
            Commercial=dts_sv;
            break;
        case 2 :
            ChannelCount+=2; // C S matrixed in L R so +2
            ChannelLayout="L C R S";
            Settings="4:2:4 Matrix";
            Commercial=dts_stereo;
            break;
        case 5 :
            ChannelCount++; // LS RS are crossed over to SW at 80 Hz so +1
            ChannelLayout="L LS C RS R SW";
            if (Serial>=60000)
            {
                ChannelCount++; // BS matrixed in LS RS so +1
                ChannelLayout+=" BS";
                Settings="ES Matrix";
                Commercial=dts_es;
            }
            else if ((Serial==1357 && Language=="*ENG") || Serial==11131 || Serial==12030 || (Serial>12074 && Serial<21000)) // There are some exceptions... Hardcoded in the sidecar software update? List based on feedback from this format fans
                Commercial=dts_70;
            else
                Commercial=dts_6;
            break;
        case 6 :
            ChannelLayout="L LC C RC R S";
            Commercial=dts_70_sv;
            break;
        case 8 :
            ChannelLayout = "L LS C RS R SW LC RC";
            Commercial=dts_70_sv;
            break;
    }
    auto Commercial_String=Aptx100_Commercial[Commercial];
    Fill(Stream_General, 0, General_Format_Commercial_IfAny, Commercial_String);
    Fill(Stream_Audio, 0, Audio_Format_Commercial_IfAny, Commercial_String);
    Fill(Stream_Audio, 0, Audio_Format_Settings, Settings);
    Fill(Stream_Audio, 0, Audio_Channel_s_, ChannelCount);
    Fill(Stream_Audio, 0, Audio_ChannelLayout, ChannelLayout);
    Fill(Stream_Audio, 0, Audio_BitRate_Mode, "CBR");
    Fill(Stream_Audio, 0, Audio_SamplingRate, 44100);
    Fill(Stream_Audio, 0, Audio_Duration, Duration_ms);
    Fill(Stream_Audio, 0, Audio_TimeCode_FirstFrame, Start.ToString());
    Fill(Stream_Audio, 0, Audio_TimeCode_LastFrame, End.ToString());
    if (!Language.empty() && Language[0]=='*')
    {
        auto Language_Test=Language.c_str()+1;
        auto MappedLang=lower_bound(begin(Aptx100_Languages), end(Aptx100_Languages), Language_Test);
        if (MappedLang!=end(Aptx100_Languages) && !(Language_Test<*MappedLang))
            Language=MappedLang->To;
        else
            Language.erase(0, 1);
    }
    Fill(Stream_Audio, 0, Audio_Language, Language);

    if (File_Size!=(int64u)-1 && File_Size>Element_Offset)
        Fill(Stream_Audio, 0, Audio_StreamSize, File_Size-Element_Offset);

    //No more need data
    Finish();
}

} //NameSpace

#endif //MEDIAINFO_APTX100_YES


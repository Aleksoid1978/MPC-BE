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
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/File__MultipleParsing.h"
#include "MediaInfo/MediaInfo_Internal.h"
#include "MediaInfo/MediaInfo_Config.h"
#include "MediaInfo/TimeCode.h"
#include "ThirdParty/base64/base64.h"
#if defined(MEDIAINFO_FILE_YES)
#include "ZenLib/File.h"
#endif //defined(MEDIAINFO_REFERENCES_YES)
#include "ZenLib/FileName.h"
#include "ZenLib/BitStream_LE.h"
#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <type_traits>
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

    //---------------------------------------------------------------------------
    extern MediaInfo_Config Config;
    const char* Mpegv_colour_primaries(int8u colour_primaries);
    const char* Mpegv_transfer_characteristics(int8u transfer_characteristics);
    const char* Mpegv_matrix_coefficients(int8u matrix_coefficients);
    //---------------------------------------------------------------------------

    //***************************************************************************
    // Others, specialized, parsing
    //***************************************************************************

    //---------------------------------------------------------------------------
    bool Location_Adapt(string& Value)
    {
        // Zero padding of degrees/minutes/seconds
        auto AddZeroDMS = [](string& Value, size_t& degree_pos) {
            degree_pos = Value.find("\xC2\xB0", degree_pos);
            if (degree_pos == string::npos) {
                return false;
            }
            ++degree_pos; // degree symbol is 2 bytes
            auto space_pos = Value.find(' ', degree_pos);
            bool Modified = false;

            auto minute_pos = Value.find('\'', degree_pos);
            if (minute_pos >= space_pos) {
                return false;
            }
            auto minute_dot_pos = Value.find('.', degree_pos);
            if (minute_dot_pos > minute_pos) {
                minute_dot_pos = minute_pos;
            }
            if (minute_dot_pos - degree_pos <= 2) {
                Value.insert(degree_pos + 1, 1, '0');
                minute_pos++;
                Modified = true;
            }

            auto second_pos = Value.find('"', minute_pos);
            if (second_pos >= space_pos) {
                return Modified;
            }
            auto second_dot_pos = Value.find('.', minute_pos);
            if (second_dot_pos > second_pos) {
                second_dot_pos = second_pos;
            }
            if (second_dot_pos - minute_pos <= 2) {
                Value.insert(minute_pos + 1, 1, '0');
                second_pos++;
                Modified = true;
            }

            return Modified;
            };
        size_t DMS_Pos = 0;
        bool Modified = false;
        Modified |= AddZeroDMS(Value, DMS_Pos); // Latitude
        DMS_Pos++;
        Modified |= AddZeroDMS(Value, DMS_Pos); // Longitude
        if (Modified) {
            return true;
        }

        // ISO6709 machine to human readable
        auto lat_sign_pos = Value.find_first_of("+-");
        auto lon_sign_pos = Value.find_first_of("+-", lat_sign_pos + 1);
        auto alt_sign_pos = Value.find_first_of("+-", lon_sign_pos + 1);
        auto slash_pos = Value.find('/');

        if (lat_sign_pos || lon_sign_pos == std::string::npos || slash_pos == std::string::npos)
            return false;

        auto lat_sign = Value[lat_sign_pos] == '+' ? 'N' : 'S';
        ++lat_sign_pos;
        while (Value[lat_sign_pos] == '0' && Value[lat_sign_pos + 1] >= '0' && Value[lat_sign_pos + 1] <= '9') {
            ++lat_sign_pos;
        }
        auto lat_without_sign = Value.substr(lat_sign_pos, lon_sign_pos - lat_sign_pos);
        auto lon_sign = Value[lon_sign_pos] == '+' ? 'E' : 'W';
        ++lon_sign_pos;
        while (Value[lon_sign_pos] == '0' && Value[lon_sign_pos + 1] >= '0' && Value[lon_sign_pos + 1] <= '9') {
            ++lon_sign_pos;
        }
        auto lon_without_sign = Value.substr(lon_sign_pos, (alt_sign_pos == std::string::npos ? slash_pos : alt_sign_pos) - lon_sign_pos);
        string alt;
        if (alt_sign_pos != std::string::npos) {
            if (Value[alt_sign_pos] == '+') {
                ++alt_sign_pos; // + sign is implicit
            }
            while (Value[alt_sign_pos] == '0' && Value[alt_sign_pos + 1] >= '0' && Value[alt_sign_pos + 1] <= '9') {
                ++alt_sign_pos;
            }
            alt = Value.substr(alt_sign_pos, slash_pos - alt_sign_pos);
            if (alt.find_first_of('.') != string::npos && alt.find_first_not_of("0.-") == string::npos) {
                alt.clear(); // It is unlikely a real one...
            }
        }
        Value.clear();
        if (lat_without_sign.find_first_not_of("0.") == string::npos && lon_without_sign.find_first_not_of("0.") == string::npos && alt.empty()) {
            return true; // It is unlikely a real one...
        }
        Value += lat_without_sign;
        Value += "\xC2\xB0";
        Value += lat_sign;
        Value += ' ';
        Value += lon_without_sign;
        Value += "\xC2\xB0";
        Value += lon_sign;
        if (!alt.empty()) {
            Value += ' ';
            Value += alt;
            if (alt.find("0123456789.") == string::npos) { // if no CRS identifier
                Value += 'm';
            }
        }
        return true;
    }

//---------------------------------------------------------------------------
static const float digit_to_precision[] = {
    0.5f,
    0.05f,
    0.005f,
    0.0005f,
    0.00005f,
    0.000005f,
    0.0000005f,
};
const size_t digit_to_precision_Size = sizeof(digit_to_precision) / sizeof(*digit_to_precision);
bool Location_Compare(const string& Current, const string& CompareTo, bool& Replace)
{
    auto Split = [](const string& Value, size_t& Start, size_t* Pos3, size_t* Size3) {
        auto degree_pos = Value.find("\xC2\xB0", Start);
        if (degree_pos == string::npos) {
            return true;
        }
        auto degree_dot_pos = Value.find('.', Start);
        if (degree_dot_pos > degree_pos) {
            degree_dot_pos = degree_pos;
        }
        else {
            degree_dot_pos++;
        }
        Pos3[0] = Start;
        Start = Value.find(' ', Start);
        if (Start == string::npos) {
            Start = Value.size();
        }

        auto minute_pos = Value.find('\'', degree_pos);
        if (minute_pos != string::npos) {
            return true; // Not supported
        }

        Size3[0] = degree_pos - degree_dot_pos;
        Pos3[1] = Start;
        Size3[1] = 0;
        Pos3[2] = Start;
        Size3[2] = 0;
        return false;
    };
    size_t Start0 = 0;
    bool Fail = false;
    size_t Pos[2][3][3];
    size_t Size[2][3][3];
    Fail |= Split(Current, Start0, Pos[0][0], Size[0][0]);
    Start0++;
    Fail |= Split(Current, Start0, Pos[0][1], Size[0][1]);
    size_t Start1 = 0;
    Fail |= Split(CompareTo, Start1, Pos[1][0], Size[1][0]);
    Start1++;
    Fail |= Split(CompareTo, Start1, Pos[1][1], Size[1][1]);
    if (Fail) {
        return false;
    }
    string OldLocale;
    auto OldLocale_Temp = setlocale(LC_NUMERIC, nullptr);
    if (OldLocale_Temp && (*OldLocale_Temp != 'C' || *(OldLocale_Temp + 1))) {
        OldLocale = OldLocale_Temp;
        setlocale(LC_NUMERIC, "C");
    }
    auto Current_Deg = strtof(Current.c_str() + Pos[0][0][0], nullptr);
    auto CompareTo_Deg = strtof(CompareTo.c_str() + Pos[0][0][0], nullptr);
    if (!OldLocale.empty()) {
        setlocale(LC_NUMERIC, OldLocale.c_str());
    }
    auto Diff_Deg = CompareTo_Deg - Current_Deg;
    size_t Precision_Deg = max(Size[0][0][0], Size[1][0][0]);
    if (Precision_Deg >= digit_to_precision_Size) {
        return false;
    }
    auto Delta_Deg = digit_to_precision[Precision_Deg];
    bool IsNOk = false;
    if (Diff_Deg > Delta_Deg || Diff_Deg < -Delta_Deg) {
        IsNOk = true;
    }
    if (!IsNOk) {
        Replace = true;
        return CompareTo.size() <= Current.size();
    }
    return false;
}

//---------------------------------------------------------------------------
static const char* add_dec_cache =
"00010203040506070809"
"10111213141516171819"
"20212223242526272829"
"30313233343536373839"
"40414243444546474849"
"50515253545556575859"
"60616263646566676869"
"70717273747576777879"
"80818283848586878889"
"90919293949596979899";
inline void add_dec_2chars(string& In, uint8_t Value)
{
    if (Value>=100)
    {
        auto Value100=div(Value, 100);
        Value=(uint8_t)Value100.rem;
        In+='0'+Value100.quot;
    }
    In.append(add_dec_cache+((size_t)Value<<1), 2);
}

//---------------------------------------------------------------------------
bool DateTime_Adapt_Finalize(string& Value_, string& Value, bool IsUtc)
{
    if (IsUtc)
        Value += " UTC";
    if (Value == Value_)
        return false;
    Value_ = Value;
    return true;
}
bool DateTime_Adapt(string& Value_)
{
    if (Value_.size() < 4)
        return false;
    string Value(Value_);

    // UTC prefix
    bool IsUtc;
    if (Value.rfind("UTC ", 4) != string::npos)
    {
        Value.erase(0, 4);
        IsUtc = true;
    }
    else
        IsUtc = false;
    
    // Unix style
    if (Value.size() < 5)
        return false;
    if (Value[4]!='-')
    {
        if (Value == "unknown")
        {
            Value_.clear();
            return true;
        }

        #if defined(_MSC_VER)
        try
        #endif
        #if !defined(__GNUC__) || __GNUC__>=5
        {
            tm t;
            string ValueT(Value);
            ValueT.erase(std::find_if(ValueT.rbegin(), ValueT.rend(), [](unsigned char ch) {return !std::isspace(ch); }).base(), ValueT.end()); // Trim from end
            stringstream Value2;
            Value2.imbue(locale("C"));
            int HasDateTime = 0;
            if (!HasDateTime)
            {
                t = {};
                Value2.str(Value);
                Value2.clear();
                Value2 >> get_time(&t, "%a %b %d %T %Y");
                if (!Value2.fail() && t.tm_year && t.tm_mday)
                    HasDateTime = 2; // Date and Time
            }
            if (!HasDateTime)
            {
                t = {};
                Value2.str(Value);
                Value2.clear();
                Value2 >> get_time(&t, "%a, %b %d, %Y %r");
                if (!Value2.fail() && t.tm_year && t.tm_mday)
                    HasDateTime = 2; // Date and Time
            }
            if (!HasDateTime)
            {
                t = {};
                Value2.str(Value);
                Value2.clear();
                Value2 >> get_time(&t, "%a, %b %d, %Y");
                if (!Value2.fail() && t.tm_year && t.tm_mday)
                    HasDateTime = 1; // Date only
            }
            if (!HasDateTime && Value.find_first_not_of("0123456789/") == string::npos)
            {
                size_t Slash0 = Value.find('/');
                if (Slash0 != string::npos)
                {
                    Slash0++;
                    size_t Slash1 = Value.find('/', Slash0);
                    if (Slash1 != string::npos)
                    {
                        Slash1++;
                        size_t Slash2 = Value.find('/', Slash1);
                        if (Slash2 == string::npos)
                        {
                            int YY = atoi(Value.c_str() + Slash1);
                            if (YY > 1900)
                                YY -= 1900;
                            if (YY <= numeric_limits<decltype(t.tm_year)>::max())
                            {
                                int DD = atoi(Value.c_str());
                                int MM = atoi(Value.c_str() + Slash0);
                                if (MM >= 12 && DD <= 31)
                                    swap(MM, DD);
                                if (DD && DD <= 31 && MM && MM < 12)
                                {
                                    t.tm_year = YY;
                                    t.tm_mon = MM - 1;
                                    t.tm_mday = DD;
                                    HasDateTime = 1; // Date only
                                }
                            }
                        }
                    }
                }
            }
            if (HasDateTime)
            {
                Value2.str(string());
                Value2.clear();
                Value2 << put_time(&t, (HasDateTime - 1) ? "%F %T" : "%F");
                if (!Value2.fail() && !Value2.str().empty())
                {
                    Value_ = Value2.str();
                    return true;
                }
            }
        }
        #endif
        #if defined(_MSC_VER)
        catch (...)
        {
            return false;
        }
        #endif
    }
    
    // Year
    if (Value[0] < '0' || Value[0] > '9'
     || Value[1] < '0' || Value[1] > '9'
     || Value[2] < '0' || Value[2] > '9'
     || Value[3] < '0' || Value[3] > '9')
        return false;
    if (Value.size() == 4)
        return DateTime_Adapt_Finalize(Value_, Value, IsUtc);

    // Month
    if (Value.size() <= 6)
        return false;
    if ((Value[4] != '-' && Value[4] != ':')
     || Value[5] < '0' || Value[5] > '9'
     || Value[6] < '0' || Value[6] > '9')
        return false;
    if (Value[4] == ':') {
        Value[4] = '-';
    }
    if (Value.size() == 7)
        return DateTime_Adapt_Finalize(Value_, Value, IsUtc);

    // Day
    if (Value.size() <= 9)
        return false;
    if ((Value[7] != '-' && Value[7] != ':')
     || Value[8] < '0' || Value[8] > '9'
     || Value[9] < '0' || Value[9] > '9')
        return false;
    if (Value[7] == ':') {
        Value[7] = '-';
    }
    if (Value.size() == 10)
        return DateTime_Adapt_Finalize(Value_, Value, IsUtc);

    // Separator
    if (Value.size() <= 11)
        return false;
    if (Value[10] == 'T')
        Value[10] = ' ';
    if (Value[10] == ',' && Value[11] == ' ')
        Value.erase(10, 1);

    // Time
    if (Value.size() <= 18)
        return false;
    if (Value[11] < '0' || Value[11] > '9'
     || Value[12] < '0' || Value[12] > '9'
     || Value[13] != ':'
     || Value[14] < '0' || Value[14] > '9'
     || Value[15] < '0' || Value[15] > '9'
     || Value[16] != ':'
     || Value[17] < '0' || Value[17] > '9'
     || Value[18] < '0' || Value[18] > '9')
        return false;
    if (Value.size() == 19)
        return DateTime_Adapt_Finalize(Value_, Value, IsUtc);

    // Fraction
    size_t i = 19;
    if (Value[i] == '.')
    {
        i++;
        while (i < Value.size() && Value[i] >= '0' && Value[i] <= '9')
            i++;
    }
    if (Value.size() == i)
        return DateTime_Adapt_Finalize(Value_, Value, IsUtc);

    // Time zone
    if (Value[i] == 'Z')
    {
        if (Value.size() != i + 1)
            return false;
        Value.pop_back();
        return DateTime_Adapt_Finalize(Value_, Value, true);
    }
    if (Value.size() <= i + 2)
        return false;
    if ((Value[i] != '+' && Value[i] != '-')
     || Value[i + 1] < '0' || Value[i + 1] > '9'
     || Value[i + 2] < '0' || Value[i + 2] > '9')
        return false;
    if (Value.size() == i + 3)
    {
        return DateTime_Adapt_Finalize(Value_, Value, IsUtc);
    }
    if (Value.size() <= i + 4)
        return false;
    if (Value.size() == i + 5
     && Value[i + 3] >= '0' && Value[i + 3] <= '9'
     && Value[i + 4] >= '0' && Value[i + 4] <= '9')
    {
        Value.insert(i + 3, 1, ':');
    }
    if (Value.size() <= i + 5)
        return false;
    if (Value[i + 3] != ':'
     || Value[i + 4] < '0' || Value[i + 4] > '9'
     || Value[i + 5] < '0' || Value[i + 5] > '9')
        return false;
    if (Value.size() == i + 6)
    {
        if (Value[i + 1] == '0'
         && Value[i + 2] == '0'
         && Value[i + 4] == '0'
         && Value[i + 5] == '0')
        {
            Value.resize(i);
            IsUtc = true;
        }
        return DateTime_Adapt_Finalize(Value_, Value, IsUtc);
    }

    return false;
}


//---------------------------------------------------------------------------
void File__Analyze::Attachment(const char* MuxingMode, const Ztring& Description, const Ztring& Type, const Ztring& MimeType, bool IsCover)
{
    if (Element_Offset >= Element_Size) {
        return;
    }

    auto containsWord = [](const std::string& text, const std::string& word) {
        auto pos = text.find(word);
        while (pos != std::string::npos)
        {
            bool leftOk =
                (pos == 0) ||
                !std::isalpha(static_cast<unsigned char>(text[pos - 1]));

            bool rightOk =
                (pos + word.size() >= text.size()) ||
                !std::isalpha(static_cast<unsigned char>(text[pos + word.size()]));
            if (leftOk && rightOk)
                return true;

            pos = text.find(word, pos + 1);
        }

        return false;
    };

    Ztring ModifiedType(Type);
    if (ModifiedType.empty() && (IsCover || MimeType.empty() || MimeType.rfind(__T("image/"), 0) == 0)) {
        auto Description_Lower(Description);
        Description_Lower.MakeLowerCase();
        auto Description_String = Description_Lower.To_UTF8();
        if (containsWord(Description_String, "thumbnail") && Description_String.find("c2pa.thumbnail") == string::npos) {
            IsCover = true;
            ModifiedType = "Thumbnail";
        }
        if (containsWord(Description_String, "cover") || containsWord(Description_String, "front") || containsWord(Description_String, "frontcover")) {
            IsCover = true;
            ModifiedType = "Cover";
        }
        if (containsWord(Description_String, "back") || containsWord(Description_String, "backcover")) {
            IsCover = true;
            ModifiedType = "Cover_Back";
        }
        if (containsWord(Description_String, "cd") && !containsWord(Description_String, "uuid")) {
            IsCover = true;
            ModifiedType = "Cover_Media";
        }
    }
    size_t Data_Size = (size_t)(Element_Size - Element_Offset);
    if (IsCover) {
        auto Type_String = __T("Type_") + ModifiedType;
        auto Type_String2 = MediaInfoLib::Config.Language_Get(Type_String);
        if (Type_String2 == Type_String)
            Type_String2 = ModifiedType;

        Fill(Stream_General, 0, General_Cover, "Yes");
        Fill(Stream_General, 0, General_Cover_Description, Description);
        Fill(Stream_General, 0, General_Cover_Type, Type_String2);
        Fill(Stream_General, 0, General_Cover_Mime, MimeType);
        #if MEDIAINFO_ADVANCED
            if (MediaInfoLib::Config.Flags1_Get(Flags_Cover_Data_base64)) {
                std::string Data_Raw((const char*)(Buffer + (size_t)(Buffer_Offset + Element_Offset)), Data_Size);
                std::string Data_Base64(Base64::encode(Data_Raw));
                Fill(Stream_General, 0, General_Cover_Data, Data_Base64);
            }
        #endif //MEDIAINFO_ADVANCED
    }

    File__MultipleParsing MI;
    Open_Buffer_Init(&MI);
    Open_Buffer_Continue(&MI);
    Open_Buffer_Finalize(&MI);
    Element_Show(); //TODO: why is it needed?
    auto ImageCount = MI.Count_Get(Stream_Image);
    if (IsCover || ImageCount) {
        Stream_Prepare(Stream_Image);
        Fill(Stream_Image, StreamPos_Last, Image_Type, ModifiedType);
        Fill(Stream_Image, StreamPos_Last, Image_Title, Description);
        Fill(Stream_Image, StreamPos_Last, Image_MuxingMode, MuxingMode);
        Fill(Stream_Image, StreamPos_Last, Image_InternetMediaType, MimeType);
        Fill(Stream_Image, StreamPos_Last, Image_StreamSize, Data_Size);
        if (ImageCount) {
            Merge(MI, Stream_Image, 0, StreamPos_Last); // We ignore thumbnails in thumbnails/covers on purpose (not the goal of the cover)
        }
    }
}

//---------------------------------------------------------------------------
// Display aspect ratio with its usual name (e.g. 16:9, 2.39:1)
static Ztring DisplayAspectRatio_String(float DAR)
{
    Ztring DARS;
         if (DAR>=(float)1.23 && DAR<(float)1.27)   DARS=__T("5:4");
    else if (DAR>=(float)1.30 && DAR<(float)1.37)   DARS=__T("4:3");
    else if (DAR>=(float)1.45 && DAR<(float)1.55)   DARS=__T("3:2");
    else if (DAR>=(float)1.55 && DAR<(float)1.65)   DARS=__T("16:10");
    else if (DAR>=(float)1.65 && DAR<(float)1.70)   DARS=__T("5:3");
    else if (DAR>=(float)1.74 && DAR<(float)1.82)   DARS=__T("16:9");
    else if (DAR>=(float)1.82 && DAR<(float)1.88)   DARS=__T("1.85:1");
    else if (DAR>=(float)2.15 && DAR<(float)2.22)   DARS=__T("2.2:1");
    else if (DAR>=(float)2.23 && DAR<(float)2.30)   DARS=__T("2.25:1");
    else if (DAR>=(float)2.30 && DAR<(float)2.37)   DARS=__T("2.35:1");
    else if (DAR>=(float)2.37 && DAR<(float)2.395)  DARS=__T("2.39:1");
    else if (DAR>=(float)2.395 && DAR<(float)2.45)  DARS=__T("2.40:1");
    else if (DAR>=(float)0.54 && DAR<(float)0.58)   DARS=__T("9:16");
    else                                            DARS.From_Number(DAR);
      DARS.FindAndReplace(__T("."), MediaInfoLib::Config.Language_Get(__T("  Config_Text_FloatSeparator")));
    if (MediaInfoLib::Config.Language_Get(__T("  Language_ISO639"))==__T("fr") &&   DARS.find(__T(":1"))==string::npos)
          DARS.FindAndReplace(__T(":"), __T("/"));
    return DARS;
}

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_AV1_YES) || defined(MEDIAINFO_AVC_YES) || defined(MEDIAINFO_HEVC_YES) || defined(MEDIAINFO_MPEG4_YES) || defined(MEDIAINFO_MPEGTS_YES)
void File__Analyze::Get_MasteringDisplayColorVolume(Ztring &MasteringDisplay_ColorPrimaries, Ztring &MasteringDisplay_Luminance, bool FromAV1)
{
    //Parsing
    mastering_metadata_2086 Meta;
    for (size_t c = 0; c < 3; c++)
    {
        Get_B2(Meta.Primaries[c*2  ],                           "display_primaries_x");
        Get_B2(Meta.Primaries[c*2+1],                           "display_primaries_y");
    }
    Get_B2(Meta.Primaries[3*2  ],                               "white_point_x");
    Get_B2(Meta.Primaries[3*2+1],                               "white_point_y");
    Get_B4(Meta.Luminance[1],                                   "max_display_mastering_luminance");
    Get_B4(Meta.Luminance[0],                                   "min_display_mastering_luminance");

    FILLING_BEGIN();
        if (MasteringDisplay_ColorPrimaries.empty())
            Get_MasteringDisplayColorVolume(MasteringDisplay_ColorPrimaries, MasteringDisplay_Luminance, Meta, FromAV1);
    FILLING_END();
}
#endif

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_AV1_YES) || defined(MEDIAINFO_AVC_YES) || defined(MEDIAINFO_HEVC_YES) || defined(MEDIAINFO_MPEG4_YES) || defined(MEDIAINFO_MK_YES) || defined(MEDIAINFO_MXF_YES)
struct masteringdisplaycolorvolume_values
{
    int8u Code; //ISO code
    int16u Values[8]; // G, B, R, W pairs (x values then y values)
};
static const int8u MasteringDisplayColorVolume_Values_Size=4;
static const masteringdisplaycolorvolume_values MasteringDisplayColorVolume_Values[] =
{
    { 1, {15000, 30000,  7500,  3000, 32000, 16500, 15635, 16450}}, // BT.709
    { 9, { 8500, 39850,  6550,  2300, 35400, 14600, 15635, 16450}}, // BT.2020
    {11, {13250, 34500,  7500,  3000, 34000, 16000, 15700, 17550}}, // DCI P3
    {12, {13250, 34500,  7500,  3000, 34000, 16000, 15635, 16450}}, // Display P3
};
Ztring MasteringDisplayColorVolume_Values_Compute(int16u Values[8])
{
    //Reordering to RGB
    size_t G=4, B=4, R=4;
    for (size_t c=0; c<3; c++)
    {
        if (Values[c*2]<17500 && Values[c*2+1]<17500) //x and y
            B=c;
        else if ((int32s)(Values[c*2+1])-(int32s)(Values[c*2])>=0) // y minus x
            G=c;
        else
            R=c;
    }
    if ((R|B|G)>=4)
    {
        //Order not automaticly detected, betting on GBR order
        G=0;
        B=1;
        R=2;
    }

    int8u Code;
    for (int8u i=0; i<MasteringDisplayColorVolume_Values_Size; i++)
    {
        Code=MasteringDisplayColorVolume_Values[i].Code;
        for (int8u j=0; j<2; j++)
        {
            // +/- 0.0005 (3 digits after comma)
            if (Values[G*2+j]<MasteringDisplayColorVolume_Values[i].Values[0*2+j]-25 || (Values[G*2+j]>=MasteringDisplayColorVolume_Values[i].Values[0*2+j]+25))
                Code=0;
            if (Values[B*2+j]<MasteringDisplayColorVolume_Values[i].Values[1*2+j]-25 || (Values[B*2+j]>=MasteringDisplayColorVolume_Values[i].Values[1*2+j]+25))
                Code=0;
            if (Values[R*2+j]<MasteringDisplayColorVolume_Values[i].Values[2*2+j]-25 || (Values[R*2+j]>=MasteringDisplayColorVolume_Values[i].Values[2*2+j]+25))
                Code=0;
            // +/- 0.00005 (4 digits after comma)
            if (Values[3*2+j]<MasteringDisplayColorVolume_Values[i].Values[3*2+j]-2 || (Values[3*2+j]>=MasteringDisplayColorVolume_Values[i].Values[3*2+j]+3))
                Code=0;
        }

        if (Code)
            return Mpegv_colour_primaries(Code);
    }

    return       __T("R: x=")+Ztring::ToZtring(((float64)Values[R*2  ])/50000, 6)
                +__T(  " y=")+Ztring::ToZtring(((float64)Values[R*2+1])/50000, 6)
              +__T(", G: x=")+Ztring::ToZtring(((float64)Values[G*2  ])/50000, 6)
                +__T(  " y=")+Ztring::ToZtring(((float64)Values[G*2+1])/50000, 6)
              +__T(", B: x=")+Ztring::ToZtring(((float64)Values[B*2  ])/50000, 6)
                +__T(  " y=")+Ztring::ToZtring(((float64)Values[B*2+1])/50000, 6)
    +__T(", White point: x=")+Ztring::ToZtring(((float64)Values[3*2  ])/50000, 6)
                +__T(  " y=")+Ztring::ToZtring(((float64)Values[3*2+1])/50000, 6);
}
void File__Analyze::Get_MasteringDisplayColorVolume(Ztring &MasteringDisplay_ColorPrimaries, Ztring &MasteringDisplay_Luminance, mastering_metadata_2086 &Meta, bool FromAV1)
{
    if (!MasteringDisplay_ColorPrimaries.empty())
        return; // Use the first one

    bool IsNotValid=false;
    for (int8u i=0; i<8; i++)
        if (Meta.Primaries[i]==(int16u)-1)
            IsNotValid=true;
        else if (FromAV1)
            Meta.Primaries[i]=(int16u)(((int32u)Meta.Primaries[i]*50000+32768)>>16); // 0.16 fixed-point, MPEG values are x50000
    if (!IsNotValid)
        MasteringDisplay_ColorPrimaries=MasteringDisplayColorVolume_Values_Compute(Meta.Primaries);

    if (Meta.Luminance[0]!=(int32u)-1 && Meta.Luminance[1]!=(int32u)-1)
    {
        float32 Luminance_Min_Ratio=static_cast<float32>(FromAV1?16384:10000); // 18.14 fixed-point, MPEG values are x10000
        float32 Luminance_Max_Ratio=static_cast<float32>(FromAV1?256:10000); // 24.8 fixed-point, MPEG values are x10000
        MasteringDisplay_Luminance=        __T("min: ")+Ztring::ToZtring(((float64)Meta.Luminance[0])/Luminance_Min_Ratio, 4)
                                  +__T(" cd/m2, max: ")+Ztring::ToZtring(((float64)Meta.Luminance[1])/Luminance_Max_Ratio, ((float64)Meta.Luminance[1]/Luminance_Max_Ratio-Meta.Luminance[1]/Luminance_Max_Ratio==0)?0:4)
                                  +__T(" cd/m2");
    }
}
#endif

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_HEVC_YES) || defined(MEDIAINFO_MPEG4_YES)
void File__Analyze::Get_AmbientViewingEnvironment(float64& AmbientViewingEnvironment_Illuminance, Ztring& AmbientViewingEnvironment_Illuminance_string, Ztring& AmbientViewingEnvironment_Chromaticity)
{
    struct ambientviewingenvironmentchromaticity_values {
        int16u Values_x;
        int16u Values_y;
        string Name;        // standard illuminant name
    };
    static const ambientviewingenvironmentchromaticity_values AmbientViewingEnvironmentChromaticity_Values[]{
        { 15635, 16450, "D65" },
        { 14155, 14855, "D93" },
    };
    static const int8u AmbientViewingEnvironmentChromaticity_Values_Size = sizeof(AmbientViewingEnvironmentChromaticity_Values) / sizeof(AmbientViewingEnvironmentChromaticity_Values[0]);

    //Parsing
    int32u ambient_illuminance;
    int16u ambient_light_x, ambient_light_y;
    float64 ambient_illuminance_f;
    Ztring ambient_illuminance_s, ambient_light_x_s, ambient_light_y_s;

    Get_B4(ambient_illuminance,                                 "ambient_illuminance");
    ambient_illuminance_f = ambient_illuminance * 0.0001;
    ambient_illuminance_s = Ztring::ToZtring(ambient_illuminance_f, 4) + __T(" lux");
    Param_Info1(ambient_illuminance_s);

    Get_B2(ambient_light_x,                                     "ambient_light_x");
    ambient_light_x_s = Ztring::ToZtring(ambient_light_x * 0.00002, 5);
    Param_Info1(ambient_light_x_s);

    Get_B2(ambient_light_y,                                     "ambient_light_y");
    ambient_light_y_s = Ztring::ToZtring(ambient_light_y * 0.00002, 5);
    Param_Info1(ambient_light_y_s);

    FILLING_BEGIN_PRECISE();
        AmbientViewingEnvironment_Illuminance = ambient_illuminance_f;
        AmbientViewingEnvironment_Illuminance_string = ambient_illuminance_s;
        for (int8u i = 0; i < AmbientViewingEnvironmentChromaticity_Values_Size; ++i) {
            // +/- 0.00005 (4 digits after comma)
            if (ambient_light_x > AmbientViewingEnvironmentChromaticity_Values[i].Values_x - 3 && ambient_light_x < AmbientViewingEnvironmentChromaticity_Values[i].Values_x + 3
                && ambient_light_y > AmbientViewingEnvironmentChromaticity_Values[i].Values_y - 3 && ambient_light_y < AmbientViewingEnvironmentChromaticity_Values[i].Values_y + 3) {
                AmbientViewingEnvironment_Chromaticity = Ztring().From_UTF8(AmbientViewingEnvironmentChromaticity_Values[i].Name);
            }
        }
        if (AmbientViewingEnvironment_Chromaticity.empty()) {
            AmbientViewingEnvironment_Chromaticity = __T("x=") + ambient_light_x_s + __T(", y=") + ambient_light_y_s;
        }
        Element_Info1(AmbientViewingEnvironment_Illuminance_string + __T(", ") + AmbientViewingEnvironment_Chromaticity);
    FILLING_END();
}
#endif

//---------------------------------------------------------------------------
const char* DolbyVision_Compatibility[] =
{
    "",
    "HDR10",
    "SDR",
    NULL,
    "HLG",
    NULL,
    "Blu-ray",
};
static const size_t DolbyVision_Compatibility_Size = sizeof(DolbyVision_Compatibility) / sizeof(const char*);
size_t DolbyVision_Compatibility_Pos(const Ztring& Value)
{

    for (size_t Pos = 0; Pos < DolbyVision_Compatibility_Size; Pos++)
        if (Ztring(DolbyVision_Compatibility[Pos]) == Value)
            return Pos;
    return (size_t)-1;
}
#if defined(MEDIAINFO_AV1_YES) || defined(MEDIAINFO_AVC_YES) || defined(MEDIAINFO_HEVC_YES) || defined(MEDIAINFO_MPEG4_YES) || defined(MEDIAINFO_MATROSKA_YES) || defined(MEDIAINFO_MXF_YES)
enum class dolbyvision_profile : uint8_t
{
    dav1,
    davc,
    dvav,
    dvh1,
    dvh8,
    dvhe,
    max,
};
static const char DolbyVision_Profiles_Names[] = // dv[BL_codec_type].[number_of_layers][bit_depth][cross-compatibility]
"dav1davcdvavdvh1dvh8dvhe";
static_assert(sizeof(DolbyVision_Profiles_Names)==4*((size_t)dolbyvision_profile::max)+1, "");
static dolbyvision_profile DolbyVision_Profiles[]=
{
    dolbyvision_profile::dvav,
    dolbyvision_profile::dvav,
    dolbyvision_profile::dvhe,
    dolbyvision_profile::dvhe,
    dolbyvision_profile::dvhe,
    dolbyvision_profile::dvhe,
    dolbyvision_profile::dvhe,
    dolbyvision_profile::dvhe,
    dolbyvision_profile::dvhe,
    dolbyvision_profile::dvav,
    dolbyvision_profile::dav1,
    dolbyvision_profile::max,
    dolbyvision_profile::max,
    dolbyvision_profile::max,
    dolbyvision_profile::max,
    dolbyvision_profile::max,
    dolbyvision_profile::max,
    dolbyvision_profile::max,
    dolbyvision_profile::max,
    dolbyvision_profile::max,
    dolbyvision_profile::dvh1,
    dolbyvision_profile::max,
    dolbyvision_profile::max,
    dolbyvision_profile::max,
    dolbyvision_profile::max,
    dolbyvision_profile::max,
    dolbyvision_profile::max,
    dolbyvision_profile::max,
    dolbyvision_profile::max,
    dolbyvision_profile::max,
    dolbyvision_profile::max,
    dolbyvision_profile::max,
    dolbyvision_profile::davc,
    dolbyvision_profile::max,
    dolbyvision_profile::dvh8,
};
static const size_t DolbyVision_Profiles_Size=sizeof(DolbyVision_Profiles)/sizeof(decltype(*DolbyVision_Profiles));
static void DolbyVision_Profiles_Append(string& Profile, int8u i)
{
    dolbyvision_profile j;
    if (i>=DolbyVision_Profiles_Size)
        j=dolbyvision_profile::max;
    else
        j=DolbyVision_Profiles[i];
    if (j>=dolbyvision_profile::max)
        return add_dec_2chars(Profile, i);
    Profile.append(DolbyVision_Profiles_Names+((size_t)j)*4, 4);
}
const char* DolbyVision_Compression[] =
{
    "None",
    "Limited",
    NULL,
    "Extended",
};
void File__Analyze::dvcC(bool has_dependency_pid, std::map<std::string, Ztring>* Infos)
{
    Element_Name("Dolby Vision Configuration");

    //Parsing
    int8u  dv_version_major, dv_version_minor, dv_profile, dv_level, dv_bl_signal_compatibility_id, dv_md_compression;
    bool rpu_present_flag, el_present_flag, bl_present_flag;
    Get_B1 (dv_version_major,                                   "dv_version_major");
    if (dv_version_major && dv_version_major<=3) //Spec says nothing, we hope that a minor version change means that the stream is backward compatible
    {
        Get_B1 (dv_version_minor,                               "dv_version_minor");
        BS_Begin();
        size_t End=Data_BS_Remain();
        if (End>=176)
            End-=176;
        else
            End=0; // Not enough place for reserved bits, but we currently ignore such case, just considered as unknown
        Get_S1 (7, dv_profile,                                  "dv_profile");
        Get_S1 (6, dv_level,                                    "dv_level");
        Get_SB (   rpu_present_flag,                            "rpu_present_flag");
        Get_SB (   el_present_flag,                             "el_present_flag");
        Get_SB (   bl_present_flag,                             "bl_present_flag");
        if (has_dependency_pid && !bl_present_flag)
        {
            Skip_S2(13,                                         "dependency_pid");
            Skip_S1( 3,                                         "reserved");
        }
        if (Data_BS_Remain())
        {
            Get_S1 (4, dv_bl_signal_compatibility_id,           "dv_bl_signal_compatibility_id"); // in dv_version_major 2 only if based on specs but it was confirmed to be seen in dv_version_major 1 too and it does not hurt (value 0 means no new display)
            Get_S1 (2, dv_md_compression,                       "dv_md_compression");
            if (End<Data_BS_Remain())
                Skip_BS(Data_BS_Remain()-End,                   "reserved");
        }
        else
        {
            dv_bl_signal_compatibility_id=0;
            dv_md_compression=0;
        }
        BS_End();
    }
    Skip_XX(Element_Size-Element_Offset,                        "Unknown");

    FILLING_BEGIN();
        if (Infos)
            (*Infos)["HDR_Format"].From_UTF8("Dolby Vision");
        else
            Fill(Stream_Video, StreamPos_Last, Video_HDR_Format, "Dolby Vision");
        if (dv_version_major && dv_version_major<=3)
        {
            Ztring Version=Ztring::ToZtring(dv_version_major)+__T('.')+Ztring::ToZtring(dv_version_minor);
            if (Infos)
                (*Infos)["HDR_Format_Version"]=Version;
            else
                Fill(Stream_Video, StreamPos_Last, Video_HDR_Format_Version, Version);
            string Profile, Level;
            DolbyVision_Profiles_Append(Profile, dv_profile);
            Profile+='.';
            add_dec_2chars(Profile, dv_profile);
            add_dec_2chars(Level, dv_level);
            if (Infos)
            {
                (*Infos)["HDR_Format_Profile"].From_UTF8(Profile);
                (*Infos)["HDR_Format_Level"].From_UTF8(Level);
            }
            else
            {
                Fill(Stream_Video, StreamPos_Last, Video_HDR_Format_Profile, Profile);
                Fill(Stream_Video, StreamPos_Last, Video_HDR_Format_Level, Level);
            }

            string Layers;
            if (rpu_present_flag||el_present_flag||bl_present_flag)
            {
                if (bl_present_flag)
                    Layers +="BL+";
                if (el_present_flag)
                    Layers +="EL+";
                if (rpu_present_flag)
                    Layers +="RPU+";
                Layers.resize(Layers.size()-1);
            }
            if (DolbyVision_Compression[dv_md_compression])
            {
                if (Infos)
                    (*Infos)["HDR_Format_Compression"].From_UTF8(DolbyVision_Compression[dv_md_compression]);
                else
                    Fill(Stream_Video, StreamPos_Last, Video_HDR_Format_Compression, DolbyVision_Compression[dv_md_compression]);
            }
            if (Infos)
                (*Infos)["HDR_Format_Settings"].From_UTF8(Layers);
            else
                Fill(Stream_Video, StreamPos_Last, Video_HDR_Format_Settings, Layers);
            if (dv_bl_signal_compatibility_id)
            {
                string Compatibility;
                if (dv_bl_signal_compatibility_id<DolbyVision_Compatibility_Size && DolbyVision_Compatibility[dv_bl_signal_compatibility_id])
                    Compatibility=DolbyVision_Compatibility[dv_bl_signal_compatibility_id];
                else
                    Compatibility=Ztring().From_Number(dv_bl_signal_compatibility_id).To_UTF8();
                if (Infos)
                    (*Infos)["HDR_Format_Compatibility"].From_UTF8(Compatibility);
                else
                    Fill(Stream_Video, StreamPos_Last, Video_HDR_Format_Compatibility, Compatibility);
            }
            if (dv_profile == 5) {
                // VUI information is optional for a profile 5 bitstream.
                if (Retrieve_Const(Stream_Video, StreamPos_Last, Video_colour_primaries).empty() && Retrieve_Const(Stream_Video, StreamPos_Last, Video_transfer_characteristics).empty() && Retrieve_Const(Stream_Video, StreamPos_Last, Video_matrix_coefficients).empty()) {
                    Fill(Stream_Video, StreamPos_Last, Video_colour_primaries, Mpegv_colour_primaries(9));
                    Fill(Stream_Video, StreamPos_Last, Video_transfer_characteristics, Mpegv_transfer_characteristics(16));
                    Fill(Stream_Video, StreamPos_Last, Video_matrix_coefficients, Mpegv_matrix_coefficients(15));
                }
            }
        }
        else
            if (Infos)
                (*Infos)["HDR_Format_Version"]=Ztring::ToZtring(dv_version_major);
            else
                Fill(Stream_Video, StreamPos_Last, Video_HDR_Format_Version, dv_version_major);
    FILLING_END();
}
#endif

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_HEVC_YES) || defined(MEDIAINFO_AV1_YES) || defined(MEDIAINFO_MPEG4_YES) || defined(MEDIAINFO_MXF_YES)

const char* DV_content_type(int8u content_type)
{
    switch (content_type) {
    case 0  : return "Default";
    case 1  : return "Movies";
    case 2  : return "Game";
    case 3  : return "Sport";
    case 4  : return "User Generated Content";
    default : return "";
    }
}

const char* DV_white_point(int8u white_point)
{
    switch (white_point) {
    case  0: return "6504K, D65";
    case  1: return "6879K";
    case  2: return "7254K";
    case  3: return "7629K";
    case  4: return "8004K";
    case  5: return "8379K";
    case  6: return "8754K";
    case  7: return "9129K";
    case  8: return "9504K, D93";
    case  9: return "9879K";
    case 10: return "10254K";
    case 11: return "10629K";
    case 12: return "11004K";
    case 13: return "11379K";
    case 14: return "11754K";
    case 15: return "12129K";
    default: return "";
    }
}

#endif // defined(MEDIAINFO_HEVC_YES) || defined(MEDIAINFO_AV1_YES) || defined(MEDIAINFO_MPEG4_YES) || defined(MEDIAINFO_MXF_YES)

#if defined(MEDIAINFO_HEVC_YES) || defined(MEDIAINFO_AV1_YES)

// CRC Table for CRC-32/MPEG-2
// Polynomial - 0x04C11DB7
static const int32u CRC_32_MPEG_2_Table[256] =
{
    0x00000000, 0x04C11DB7,  0x09823B6E, 0x0D4326D9,   0x130476DC, 0x17C56B6B,  0x1A864DB2, 0x1E475005,
    0x2608EDB8, 0x22C9F00F,  0x2F8AD6D6, 0x2B4BCB61,   0x350C9B64, 0x31CD86D3,  0x3C8EA00A, 0x384FBDBD,
    0x4C11DB70, 0x48D0C6C7,  0x4593E01E, 0x4152FDA9,   0x5F15ADAC, 0x5BD4B01B,  0x569796C2, 0x52568B75,
    0x6A1936C8, 0x6ED82B7F,  0x639B0DA6, 0x675A1011,   0x791D4014, 0x7DDC5DA3,  0x709F7B7A, 0x745E66CD,
    0x9823B6E0, 0x9CE2AB57,  0x91A18D8E, 0x95609039,   0x8B27C03C, 0x8FE6DD8B,  0x82A5FB52, 0x8664E6E5,
    0xBE2B5B58, 0xBAEA46EF,  0xB7A96036, 0xB3687D81,   0xAD2F2D84, 0xA9EE3033,  0xA4AD16EA, 0xA06C0B5D,
    0xD4326D90, 0xD0F37027,  0xDDB056FE, 0xD9714B49,   0xC7361B4C, 0xC3F706FB,  0xCEB42022, 0xCA753D95,
    0xF23A8028, 0xF6FB9D9F,  0xFBB8BB46, 0xFF79A6F1,   0xE13EF6F4, 0xE5FFEB43,  0xE8BCCD9A, 0xEC7DD02D,
    0x34867077, 0x30476DC0,  0x3D044B19, 0x39C556AE,   0x278206AB, 0x23431B1C,  0x2E003DC5, 0x2AC12072,
    0x128E9DCF, 0x164F8078,  0x1B0CA6A1, 0x1FCDBB16,   0x018AEB13, 0x054BF6A4,  0x0808D07D, 0x0CC9CDCA,
    0x7897AB07, 0x7C56B6B0,  0x71159069, 0x75D48DDE,   0x6B93DDDB, 0x6F52C06C,  0x6211E6B5, 0x66D0FB02,
    0x5E9F46BF, 0x5A5E5B08,  0x571D7DD1, 0x53DC6066,   0x4D9B3063, 0x495A2DD4,  0x44190B0D, 0x40D816BA,
    0xACA5C697, 0xA864DB20,  0xA527FDF9, 0xA1E6E04E,   0xBFA1B04B, 0xBB60ADFC,  0xB6238B25, 0xB2E29692,
    0x8AAD2B2F, 0x8E6C3698,  0x832F1041, 0x87EE0DF6,   0x99A95DF3, 0x9D684044,  0x902B669D, 0x94EA7B2A,
    0xE0B41DE7, 0xE4750050,  0xE9362689, 0xEDF73B3E,   0xF3B06B3B, 0xF771768C,  0xFA325055, 0xFEF34DE2,
    0xC6BCF05F, 0xC27DEDE8,  0xCF3ECB31, 0xCBFFD686,   0xD5B88683, 0xD1799B34,  0xDC3ABDED, 0xD8FBA05A,
    0x690CE0EE, 0x6DCDFD59,  0x608EDB80, 0x644FC637,   0x7A089632, 0x7EC98B85,  0x738AAD5C, 0x774BB0EB,
    0x4F040D56, 0x4BC510E1,  0x46863638, 0x42472B8F,   0x5C007B8A, 0x58C1663D,  0x558240E4, 0x51435D53,
    0x251D3B9E, 0x21DC2629,  0x2C9F00F0, 0x285E1D47,   0x36194D42, 0x32D850F5,  0x3F9B762C, 0x3B5A6B9B,
    0x0315D626, 0x07D4CB91,  0x0A97ED48, 0x0E56F0FF,   0x1011A0FA, 0x14D0BD4D,  0x19939B94, 0x1D528623,
    0xF12F560E, 0xF5EE4BB9,  0xF8AD6D60, 0xFC6C70D7,   0xE22B20D2, 0xE6EA3D65,  0xEBA91BBC, 0xEF68060B,
    0xD727BBB6, 0xD3E6A601,  0xDEA580D8, 0xDA649D6F,   0xC423CD6A, 0xC0E2D0DD,  0xCDA1F604, 0xC960EBB3,
    0xBD3E8D7E, 0xB9FF90C9,  0xB4BCB610, 0xB07DABA7,   0xAE3AFBA2, 0xAAFBE615,  0xA7B8C0CC, 0xA379DD7B,
    0x9B3660C6, 0x9FF77D71,  0x92B45BA8, 0x9675461F,   0x8832161A, 0x8CF30BAD,  0x81B02D74, 0x857130C3,
    0x5D8A9099, 0x594B8D2E,  0x5408ABF7, 0x50C9B640,   0x4E8EE645, 0x4A4FFBF2,  0x470CDD2B, 0x43CDC09C,
    0x7B827D21, 0x7F436096,  0x7200464F, 0x76C15BF8,   0x68860BFD, 0x6C47164A,  0x61043093, 0x65C52D24,
    0x119B4BE9, 0x155A565E,  0x18197087, 0x1CD86D30,   0x029F3D35, 0x065E2082,  0x0B1D065B, 0x0FDC1BEC,
    0x3793A651, 0x3352BBE6,  0x3E119D3F, 0x3AD08088,   0x2497D08D, 0x2056CD3A,  0x2D15EBE3, 0x29D4F654,
    0xC5A92679, 0xC1683BCE,  0xCC2B1D17, 0xC8EA00A0,   0xD6AD50A5, 0xD26C4D12,  0xDF2F6BCB, 0xDBEE767C,
    0xE3A1CBC1, 0xE760D676,  0xEA23F0AF, 0xEEE2ED18,   0xF0A5BD1D, 0xF464A0AA,  0xF9278673, 0xFDE69BC4,
    0x89B8FD09, 0x8D79E0BE,  0x803AC667, 0x84FBDBD0,   0x9ABC8BD5, 0x9E7D9662,  0x933EB0BB, 0x97FFAD0C,
    0xAFB010B1, 0xAB710D06,  0xA6322BDF, 0xA2F33668,   0xBCB4666D, 0xB8757BDA,  0xB5365D03, 0xB1F740B4
};

// CRC32 check for Dolby Vision Rererence Processing Unit (RPU)
// Supports non-byte aligned RPU
// Uses CRC-32/MPEG-2
//   Polynomial - 0x04C11DB7
//   Init       - 0xFFFFFFFF
//   RefIn      - false
//   RefOut     - false
//   XorOut     - 0x00000000
//   Check      - 0x0376E6E7
static bool DV_CRC32_Check(const int8u* Buffer, int64u BufferOffsetBegin, size_t RemainingBitsBegin, size_t RemainingBitsEnd)
{
    const int8u* CRC_32_Buffer = Buffer + BufferOffsetBegin;
    const int32u CRC_Poly{ 0x04C11DB7 };
    int32u CRC_32 = 0xFFFFFFFF;

    const auto first_bits = RemainingBitsBegin & 7;

    for (int8u c = 0; c < first_bits; ++c) {
        CRC_32 ^= ((CRC_32_Buffer[0] >> (first_bits - 1 - c)) & 1) << 31;
        if (CRC_32 & 0x80000000)
            CRC_32 = (CRC_32 << 1) ^ CRC_Poly;
        else
            CRC_32 <<= 1;
    }

    const auto aligned_size = (RemainingBitsBegin - RemainingBitsEnd) + ((8 - first_bits) & 7);
    const auto num_bytes = aligned_size / 8;
    
    for (size_t c = (first_bits != 0); c < num_bytes; ++c) {
        CRC_32 = (CRC_32 << 8) ^ CRC_32_MPEG_2_Table[(CRC_32 >> 24) ^ CRC_32_Buffer[c]];
    }

    for (int8u c = 0; c < (aligned_size & 7); ++c) {
        CRC_32 ^= ((CRC_32_Buffer[num_bytes] >> (7 - c)) & 1) << 31;
        if (CRC_32 & 0x80000000)
            CRC_32 = (CRC_32 << 1) ^ CRC_Poly;
        else
            CRC_32 <<= 1;
    }

    return (CRC_32 != 0);
}

// Count of bits of the fixed part of extension metadata blocks, 0 if not parsed
static size_t DV_ext_block_MinBits(bool IsCMv40, int8u ext_block_level)
{
    if (!IsCMv40) {
        switch (ext_block_level) {
        case   1: return 36;
        case   2: return 85;
        case   4: return 24;
        case   5: return 52;
        case   6: return 64;
        case 255: return 48;
        default : return 0;
        }
    }
    switch (ext_block_level) {
    case   3: return 36;
    case   8: return 80;
    case   9: return 8;
    case  10: return 40;
    case  11: return 32;
    case  15: return 88;
    case  16: return 8;
    case  18: return 56;
    case 254: return 16;
    default : return 0;
    }
}

template<typename T, typename V>
static void Unique_Add(std::vector<T>& List, const V& Value)
{
    if (std::find(List.begin(), List.end(), Value) == List.end())
        List.push_back(Value);
}

// Must be in bitstream before calling this function.
void File__Analyze::Get_DolbyVision_ReferenceProcessingUnit(DV_RPU& data, size_t BitsAfter)
{
    using std::to_string;

    #define UNSUPPORTED() \
        Trusted_IsNot("Unsupported"); \
        return;

    auto Remaining_Bits{ [&]() {
        auto Remain{ Data_BS_Remain() };
        return Remain > BitsAfter ? Remain - BitsAfter : 0;
    } };

    // Values are added to a copy, the copy is kept only if the RPU is fully parsed
    DV_RPU New(data);

    //Parsing
    Element_Begin1("rpu_data");
    auto OffsetBegin{ Element_Offset + BS->Offset_Get() };
    auto RemainingBitsBegin{ Data_BS_Remain() };

    // EDR RPU header
    Element_Begin1("rpu_data_header");
    int8u rpu_type;
    int16u rpu_format;
    Get_S1 ( 6, rpu_type,                                       "rpu_type");
    Get_S2 (11, rpu_format,                                     "rpu_format");
    if (rpu_type != 2) {
        UNSUPPORTED();
    }
    // if (rpu_type == 2) {
    // EDR RPU frame header
    int32u coefficient_log2_denom{}, bl_bit_depth_minus8, el_bit_depth_minus8, vdr_bit_depth_minus8;
    int8u vdr_rpu_profile, vdr_rpu_level, dm_compression, coefficient_data_type, vdr_rpu_normalized_idc{}, ext_mapping_idc{};
    bool BL_video_full_range_flag, el_spatial_resampling_filter_flag, disable_residual_flag, chroma_resampling_explicit_filter_flag{}, spatial_resampling_filter_flag{};
    Get_S1 ( 4, vdr_rpu_profile,                                "vdr_rpu_profile");
    Get_S1 ( 4, vdr_rpu_level,                                  "vdr_rpu_level");
    TESTELSE_SB_SKIP(                                           "vdr_seq_info_present_flag");
        // EDR RPU sequence header
        Get_SB (chroma_resampling_explicit_filter_flag,         "chroma_resampling_explicit_filter_flag");
        Get_S1 (2, coefficient_data_type,                       "coefficient_data_type");
        switch (coefficient_data_type) {
        case 0: //COEFF_FIXED
            Get_UE(coefficient_log2_denom,                      "coefficient_log2_denom");
            if (coefficient_log2_denom > 32) {
                Trusted_IsNot("coefficient_log2_denom is too large");
                return;
            }
            break;
        case 1: //COEFF_FLOAT
            break;
        }
        Get_S1 (2, vdr_rpu_normalized_idc,                      "vdr_rpu_normalized_idc");
        Get_SB (BL_video_full_range_flag,                       "BL_video_full_range_flag");
        if ((rpu_format & 0x700) == 0) {
            // sequence header
            Get_UE (bl_bit_depth_minus8,                        "BL_bit_depth_minus8");
            Get_UE (el_bit_depth_minus8,                        "EL_bit_depth_minus8");
            ext_mapping_idc = (int8u)(el_bit_depth_minus8 >> 8); // ext_mapping_idc is in the upper bits
            el_bit_depth_minus8 &= 0xFF;
            Get_UE (vdr_bit_depth_minus8,                       "vdr_bit_depth_minus8");
            if (bl_bit_depth_minus8 > 8 || el_bit_depth_minus8 > 8 || vdr_bit_depth_minus8 > 8) {
                Trusted_IsNot("bit depth is too large");
                return;
            }
            Get_SB (spatial_resampling_filter_flag,             "spatial_resampling_filter_flag");
            Get_S1 (3, dm_compression,                          "dm_compression");
            Get_SB (el_spatial_resampling_filter_flag,          "el_spatial_resampling_filter_flag");
            Get_SB (disable_residual_flag,                      "disable_residual_flag");
        }
        else {
            UNSUPPORTED();
        }
    TESTELSE_SB_ELSE(                                           "vdr_seq_info_present");
        UNSUPPORTED();
    TESTELSE_SB_END();
    bool use_prev_vdr_rpu_flag, vdr_dm_metadata_present_flag;
    int8u isMEL{ 0xFF };
    Get_SB (vdr_dm_metadata_present_flag,                       "vdr_dm_metadata_present_flag");
    Get_SB (use_prev_vdr_rpu_flag,                              "use_prev_vdr_rpu_flag");
    if (use_prev_vdr_rpu_flag) {
        Skip_UE(                                                "prev_vdr_rpu_id");
    }
    else {
        int32u num_pivots_minus_2[3]{}, mapping_color_space, mapping_chroma_format_idc, num_x_partitions_minus1, num_y_partitions_minus1;
        Skip_UE(                                                "vdr_rpu_id");
        Get_UE (mapping_color_space,                            "mapping_color_space");
        Get_UE (mapping_chroma_format_idc,                      "mapping_chroma_format_idc");
        // pivot points for BL three components
        for (int8u cmp = 0; cmp < 3; ++cmp) {
            Get_UE(num_pivots_minus_2[cmp],                     ("num_pivots_minus_2[" + to_string(cmp) + "]").c_str());
            if (num_pivots_minus_2[cmp] > 7) {
                Trusted_IsNot("num_pivots_minus_2 is too large");
                return;
            }
            for (int32u pivot_idx = 0; pivot_idx < num_pivots_minus_2[cmp] + 2; ++pivot_idx) {
                Skip_BS(static_cast<size_t>(bl_bit_depth_minus8) + 8, ("pred_pivot_value[" + to_string(cmp) + "][" + to_string(pivot_idx) + "]").c_str());
            }
        }
        auto use_nlq{ (rpu_format & 0x700) == 0 && !disable_residual_flag };
        int8u nlq_method_idc{};
        if (use_nlq) {
            // vl.x architecture EL specific
            Get_S1(3, nlq_method_idc,                           "nlq_method_idc");
            Skip_BS(static_cast<size_t>(bl_bit_depth_minus8) + 8, "nlq_pred_pivot_value[0]");
            Skip_BS(static_cast<size_t>(bl_bit_depth_minus8) + 8, "nlq_pred_pivot_value[1]");
        }
        Get_UE (num_x_partitions_minus1,                        "num_x_partitions_minus1");
        Get_UE (num_y_partitions_minus1,                        "num_y_partitions_minus1");
        if (!New.Mapping_Present) {
            New.Mapping_Present = true;
            New.mapping_color_space = mapping_color_space;
            New.mapping_chroma_format_idc = mapping_chroma_format_idc;
            New.nlq_method_idc = nlq_method_idc;
            New.num_x_partitions_minus1 = num_x_partitions_minus1;
            New.num_y_partitions_minus1 = num_y_partitions_minus1;
        }
        // }
        Element_End0(); // rpu_data_header / EDR RPU header

        // if (rpu_type == 2) {
        // EDR RPU data
        Element_Begin1("vdr_rpu_data_payload");
        Element_Begin1("rpu_data_mapping");
        for (int8u cmp = 0; cmp < 3; ++cmp) {
            for (int32u pivot_idx = 0; pivot_idx < num_pivots_minus_2[cmp] + 1; ++pivot_idx) {
                Element_Begin1("rpu_data_mapping_param");
                int32u mapping_idc;
                Get_UE(mapping_idc,                             ("mapping_idc[" + to_string(cmp) + "][" + to_string(pivot_idx) + "]").c_str());
                switch (mapping_idc) {
                case 0: //MAPPING_POLYNOMIAL
                {
                    // Polynomial coefficients
                    int32u poly_order_minus1;
                    bool linear_interp_flag{};
                    Get_UE(poly_order_minus1,                   ("poly_order_minus1[" + to_string(cmp) + "][" + to_string(pivot_idx) + "]").c_str());
                    if (poly_order_minus1 > 1) {
                        Trusted_IsNot("poly_order_minus1 is too large");
                        return;
                    }
                    if (poly_order_minus1 == 0)
                        Get_SB(linear_interp_flag,              ("linear_interp_flag[" + to_string(cmp) + "][" + to_string(pivot_idx) + "]").c_str());
                    if (poly_order_minus1 == 0 && linear_interp_flag) {
                        // Linear interpolation
                        if (coefficient_data_type == 0) {
                            Skip_UE(                            ("pred_linear_interp_value_int[" + to_string(cmp) + "][" + to_string(pivot_idx) + "]").c_str());
                            Skip_BS(coefficient_log2_denom,     ("pred_linear_interp_value[" + to_string(cmp) + "][" + to_string(pivot_idx) + "]").c_str());
                        }
                        else
                            Skip_S4(32,                         ("pred_linear_interp_value[" + to_string(cmp) + "][" + to_string(pivot_idx) + "]").c_str());
                        if (pivot_idx == num_pivots_minus_2[cmp]) {
                            if (coefficient_data_type == 0) {
                                Skip_UE(                        ("pred_linear_interp_value_int[" + to_string(cmp) + "][" + to_string(pivot_idx + 1) + "]").c_str());
                                Skip_BS(coefficient_log2_denom, ("pred_linear_interp_value[" + to_string(cmp) + "][" + to_string(pivot_idx + 1) + "]").c_str());
                            }
                            else
                                Skip_S4(32,                     ("pred_linear_interp_value[" + to_string(cmp) + "][" + to_string(pivot_idx + 1) + "]").c_str());
                        }
                    }
                    else {
                        // Non-linear
                        // the i-th order
                        for (int32u i = 0; i <= poly_order_minus1 + 1; ++i) {
                            if (coefficient_data_type == 0) {
                                Skip_SE(                        ("poly_coef_int[" + to_string(cmp) + "][" + to_string(pivot_idx) + "][" + to_string(i) + "]").c_str());
                                Skip_BS(coefficient_log2_denom, ("poly_coef[" + to_string(cmp) + "][" + to_string(pivot_idx) + "][" + to_string(i) + "]").c_str());
                            }
                            else
                                Skip_S4(32,                     ("poly_coef[" + to_string(cmp) + "][" + to_string(pivot_idx) + "][" + to_string(i) + "]").c_str());
                        }
                    }
                    break;
                }
                case 1: //MAPPING_MMR
                {
                    // MR coefficients
                    int8u mmr_order_minus1;
                    Get_S1(2, mmr_order_minus1,                 ("mmr_order_minus1[" + to_string(cmp) + "][" + to_string(pivot_idx) + "]").c_str());
                    if (coefficient_data_type == 0) {
                        Skip_SE(                                ("mmr_constant_int[" + to_string(cmp) + "][" + to_string(pivot_idx) + "]").c_str());
                        Skip_BS(coefficient_log2_denom,         ("mmr_constant[" + to_string(cmp) + "][" + to_string(pivot_idx) + "]").c_str());
                    }
                    else
                        Skip_S4(32,                             ("mmr_constant[" + to_string(cmp) + "][" + to_string(pivot_idx) + "]").c_str());
                    // the i-th order
                    for (int8u i = 1; i <= mmr_order_minus1 + 1; ++i) {
                        // the j-th coefficients
                        for (int8u j = 0; j < 7; ++j) {
                            if (coefficient_data_type == 0) {
                                Skip_SE(                        ("mmr_coef_int[" + to_string(cmp) + "][" + to_string(pivot_idx) + "][" + to_string(i) + "][" + to_string(j) + "]").c_str());
                                Skip_BS(coefficient_log2_denom, ("mmr_coef[" + to_string(cmp) + "][" + to_string(pivot_idx) + "][" + to_string(i) + "][" + to_string(j) + "]").c_str());
                            }
                            else
                                Skip_S4(32,                     ("mmr_coef[" + to_string(cmp) + "][" + to_string(pivot_idx) + "][" + to_string(i) + "][" + to_string(j) + "]").c_str());
                        }
                    }
                    break;
                }
                }
                Element_End0(); // rpu_data_mapping_param
            }
        }
        Element_End0(); // rpu_data_mapping
        if (use_nlq) {
            Element_Begin1("rpu_data_nlq");
            isMEL = 1;
            // nlq_num_pivots_minus2 == 0 so no pivot_idx loop
            for (int8u cmp = 0; cmp < 3; ++cmp) {
                // Nonlinear Quantization Parameters
                Element_Begin1("rpu_data_nlq_param");
                int16u nlq_offset;
                int32u vdr_in_max_int{}, vdr_in_max{}, linear_deadzone_slope_int{}, linear_deadzone_slope{}, linear_deadzone_threshold_int{}, linear_deadzone_threshold{};
                Get_S2(static_cast<int8u>(el_bit_depth_minus8 + 8), nlq_offset, ("nlq_offset[0][" + to_string(cmp) + "]").c_str());
                if (coefficient_data_type == 0) {
                    Get_UE(vdr_in_max_int,                      ("vdr_in_max_int[0][" + to_string(cmp) + "]").c_str());
                    Get_S4(coefficient_log2_denom, vdr_in_max,  ("vdr_in_max[0][" + to_string(cmp) + "]").c_str());
                }
                else
                    Get_S4(32, vdr_in_max,                      ("vdr_in_max[0][" + to_string(cmp) + "]").c_str());
                switch (nlq_method_idc) {
                case 0: //NLQ_LINEAR_DZ
                    //  Linear dead zone coefficients
                    if (coefficient_data_type == 0) {
                        Get_UE(linear_deadzone_slope_int,       ("linear_deadzone_slope_int[0][" + to_string(cmp) + "]").c_str());
                        Get_S4(coefficient_log2_denom, linear_deadzone_slope, ("linear_deadzone_slope[0][" + to_string(cmp) + "]").c_str());
                    }
                    else
                        Get_S4(32, linear_deadzone_slope,       ("linear_deadzone_slope[0][" + to_string(cmp) + "]").c_str());
                    if (coefficient_data_type == 0) {
                        Get_UE(linear_deadzone_threshold_int,   ("linear_deadzone_threshold_int[0][" + to_string(cmp) + "]").c_str());
                        Get_S4(coefficient_log2_denom, linear_deadzone_threshold, ("linear_deadzone_threshold[0][" + to_string(cmp) + "]").c_str());
                    }
                    else
                        Get_S4(32, linear_deadzone_threshold,   ("linear_deadzone_threshold[0][" + to_string(cmp) + "]").c_str());
                    break;
                }
                if (coefficient_data_type == 0) {
                    if ((nlq_offset | vdr_in_max | linear_deadzone_slope_int | linear_deadzone_slope | linear_deadzone_threshold_int | linear_deadzone_threshold) != 0 || vdr_in_max_int != 1)
                        isMEL = 0;
                }
                else if ((nlq_offset | linear_deadzone_slope | linear_deadzone_threshold) != 0 || vdr_in_max != 0x3F800000) // 1.0 in IEEE 754 single precision
                    isMEL = 0;
                Element_End0(); // rpu_data_nlq_param
            }
            if (nlq_method_idc) {
                isMEL = 0xFF; // Unknown NLQ method, the MEL condition can not be checked
            }
            Element_End0(); // rpu_data_nlq
        }
        
    }
    // }
    Element_End0(); // vdr_rpu_data_payload

    // Display Management
    int32u scene_refresh_flag{};
    if (vdr_dm_metadata_present_flag) {
        Element_Begin1("vdr_dm_data_payload");
        Skip_UE(                                                "affected_dm_metadata_id");
        Skip_UE(                                                "current_dm_metadata_id");
        Get_UE (scene_refresh_flag,                             "scene_refresh_flag");
        if (dm_compression > 1) {
            UNSUPPORTED();
        }
        if (!dm_compression) {
            int32u signal_eotf_param2;
            int16u signal_eotf, signal_eotf_param0, signal_eotf_param1, source_min_PQ, source_max_PQ, source_diagonal;
            int8u signal_bit_depth, signal_color_space, signal_chroma_format, signal_full_range_flag;
            for (int8u i = 0; i < 9; ++i) {
                Skip_S2(16,                                     ("YCCtoRGB_coef" + to_string(i)).c_str());
            }
            for (int8u i = 0; i < 3; ++i) {
                Skip_S4(32,                                     ("YCCtoRGB_offset" + to_string(i)).c_str());
            }
            for (int8u i = 0; i < 9; ++i) {
                Skip_S2(16,                                     ("RGBtoLMS_coef" + to_string(i)).c_str());
            }
            Get_S2 (16, signal_eotf,                            "signal_eotf");
            Get_S2 (16, signal_eotf_param0,                     "signal_eotf_param0");
            Get_S2 (16, signal_eotf_param1,                     "signal_eotf_param1");
            Get_S4 (32, signal_eotf_param2,                     "signal_eotf_param2");
            Get_S1 ( 5, signal_bit_depth,                       "signal_bit_depth");
            Get_S1 ( 2, signal_color_space,                     "signal_color_space");
            Get_S1 ( 2, signal_chroma_format,                   "signal_chroma_format");
            Get_S1 ( 2, signal_full_range_flag,                 "signal_full_range_flag");
            Get_S2 (12, source_min_PQ,                          "source_min_PQ");
            Get_S2 (12, source_max_PQ,                          "source_max_PQ");
            Get_S2 (10, source_diagonal,                        "source_diagonal");
            if (!New.DM_Present) {
                New.DM_Present = true;
                New.signal_eotf = signal_eotf;
                New.signal_eotf_param0 = signal_eotf_param0;
                New.signal_eotf_param1 = signal_eotf_param1;
                New.signal_eotf_param2 = signal_eotf_param2;
                New.signal_bit_depth = signal_bit_depth;
                New.signal_color_space = signal_color_space;
                New.signal_chroma_format = signal_chroma_format;
                New.signal_full_range_flag = signal_full_range_flag;
                New.source_diagonal = source_diagonal;
            }
            Unique_Add(New.Sources, (((int32u)source_min_PQ) << 16) | source_max_PQ);
        }

        // Extension blocks
        // ----------------------------------------------------------------------------------------
        
        // Content Mapping v2.9 (CMv2.9)
        New.Count_CMv29++;
        int32u num_ext_blocks;
        Get_UE(num_ext_blocks,                                  "num_ext_blocks");
        if (num_ext_blocks > 16) { // An arbitrary value as there is currently not so many extension blocks defined
            Trusted_IsNot("num_ext_blocks is larger than expected");
            return;
        }
        if (num_ext_blocks) {
            auto RemainingBitsEnd = Data_BS_Remain();
            auto size = RemainingBitsBegin - RemainingBitsEnd;
            Skip_BS((8 - (size & 7)) & 7,                       "dm_alignment_zero_bit");
            for (int32u i = 0; i < num_ext_blocks; ++i) {
                Element_Begin1("ext_metadata_block");
                int32u ext_block_length;
                int8u ext_block_level;
                Get_UE(ext_block_length,                        "ext_block_length");
                Get_S1(8, ext_block_level,                      "ext_block_level");
                if (ext_block_length > Remaining_Bits() / 8) {
                    Trusted_IsNot("ext_block_length is too large");
                    return;
                }
                Element_Begin1("ext_block_payload");
                size_t ext_block_len_bits{ static_cast<size_t>(ext_block_length) * 8 };
                size_t ext_block_use_bits{ 0 };
                if (ext_block_len_bits < DV_ext_block_MinBits(false, ext_block_level)) {
                    Trusted_IsNot("ext_block_length is too small");
                    ext_block_level = 0; // Skipped as unknown
                }
                switch (ext_block_level) {
                case 1: // ANALYSIS METADATA (DYNAMIC)
                {
                    int16u max_PQ, avg_PQ;
                    Skip_S2(12,                                 "min_PQ");
                    Get_S2 (12, max_PQ,                         "max_PQ");
                    Get_S2 (12, avg_PQ,                         "avg_PQ");
                    ext_block_use_bits += 36;
                    if (New.L1_max_PQ < max_PQ)
                        New.L1_max_PQ = max_PQ;
                    if (New.L1_avg_PQ < avg_PQ)
                        New.L1_avg_PQ = avg_PQ;
                    break;
                }
                case 2: // PER-TARGET TRIM METADATA (DYNAMIC)
                {
                    int16u target_max_PQ;
                    Get_S2 (12, target_max_PQ,                  "target_max_PQ");
                    Skip_S2(12,                                 "trim_slope");
                    Skip_S2(12,                                 "trim_offset");
                    Skip_S2(12,                                 "trim_power");
                    Skip_S2(12,                                 "trim_chroma_weight");
                    Skip_S2(12,                                 "trim_saturation_gain");
                    Skip_S2(13,                                 "ms_weight");
                    ext_block_use_bits += 85;
                    Unique_Add(New.L2_target_max_PQ, target_max_PQ);
                    break;
                }
                case 4:
                    Skip_S2(12,                                 "anchor_pq");
                    Skip_S2(12,                                 "anchor_power");
                    ext_block_use_bits += 24;
                    break;
                case 5: // PER-SHOT ASPECT RATIO (DYNAMIC)
                {
                    int16u active_area[4];
                    Get_S2(13, active_area[0],                  "active_area_left_offset");
                    Get_S2(13, active_area[1],                  "active_area_right_offset");
                    Get_S2(13, active_area[2],                  "active_area_top_offset");
                    Get_S2(13, active_area[3],                  "active_area_bottom_offset");
                    ext_block_use_bits += 52;
                    for (int8u j = 0; j < 4; ++j) {
                        if (!New.L5_Present) {
                            New.L5_First[j] = active_area[j];
                            New.L5_Min[j] = active_area[j];
                            New.L5_Max[j] = active_area[j];
                        }
                        else if (New.L5_Min[j] > active_area[j])
                            New.L5_Min[j] = active_area[j];
                        else if (New.L5_Max[j] < active_area[j])
                            New.L5_Max[j] = active_area[j];
                    }
                    New.L5_Present = true;
                    DV_RPU::l5 L5;
                    memcpy(L5.Offsets, active_area, sizeof(active_area));
                    Unique_Add(New.L5, L5);
                    break;
                }
                case 6: // OPTIONAL HDR10 METADATA (STATIC)
                {
                    DV_RPU::l6 L6{};
                    int16u max_content_light_level, max_frame_average_light_level;
                    Get_S2(16, L6.max_display_mastering_luminance, "max_display_mastering_luminance");
                    Get_S2(16, L6.min_display_mastering_luminance, "min_display_mastering_luminance");
                    Get_S2(16, max_content_light_level,         "max_content_light_level");
                    Get_S2(16, max_frame_average_light_level,   "max_frame_average_light_level");
                    ext_block_use_bits += 64;
                    L6.max_content_light_level = max_content_light_level;
                    L6.max_frame_average_light_level = max_frame_average_light_level;
                    Unique_Add(New.L6, L6);
                    break;
                }
                case 255:
                    Skip_S1(8,                                  "dm_run_mode");
                    Skip_S1(8,                                  "dm_run_version");
                    Skip_S1(8,                                  "dm_debug0");
                    Skip_S1(8,                                  "dm_debug1");
                    Skip_S1(8,                                  "dm_debug2");
                    Skip_S1(8,                                  "dm_debug3");
                    ext_block_use_bits += 48;
                    break;
                default:
                    Skip_BS(ext_block_len_bits,                 "(Unknown)");
                    ext_block_use_bits += ext_block_len_bits;
                    break;
                }
                Skip_BS(ext_block_len_bits - ext_block_use_bits, "ext_dm_alignment_zero_bit");
                Element_End0(); // ext_block_payload
                Element_End0(); // ext_metadata_block
            }
        }

        // Content Mapping v4.0 (CMv4.0)
        if (Data_BS_Remain() >= BitsAfter + 56) {
            int32u num_ext_blocks2;
            Get_UE(num_ext_blocks2,                             "num_ext_blocks");
            if (num_ext_blocks2 > 16) { // An arbitrary value as there is currently not so many extension blocks defined
                Trusted_IsNot("num_ext_blocks is larger than expected");
                return;
            }
            if (num_ext_blocks2) {
                New.Count_CMv40++;
                auto RemainingBitsEnd = Data_BS_Remain();
                auto size = RemainingBitsBegin - RemainingBitsEnd;
                Skip_BS((8 - (size & 7)) & 7,                   "dm_alignment_zero_bit");
                for (int32u i = 0; i < num_ext_blocks2; ++i) {
                    Element_Begin1("ext_metadata_block");
                    int32u ext_block_length;
                    int8u ext_block_level;
                    int8u l16_count{ 0 };
                    Get_UE(ext_block_length,                    "ext_block_length");
                    Get_S1(8, ext_block_level,                  "ext_block_level");
                    if (ext_block_length > Remaining_Bits() / 8) {
                        Trusted_IsNot("ext_block_length is too large");
                        return;
                    }
                    Element_Begin1("ext_block_payload");
                    size_t ext_block_len_bits{ static_cast<size_t>(ext_block_length) * 8 };
                    size_t ext_block_use_bits{ 0 };
                    if (ext_block_len_bits < DV_ext_block_MinBits(true, ext_block_level)) {
                        Trusted_IsNot("ext_block_length is too small");
                        ext_block_level = 0; // Skipped as unknown
                    }
                    switch (ext_block_level) {
                    case 3: // OFFSETS TO L1 (DYNAMIC)
                        Skip_S2(12,                             "min_pq_offset");
                        Skip_S2(12,                             "max_pq_offset");
                        Skip_S2(12,                             "avg_pq_offset");
                        ext_block_use_bits += 36;
                        break;
                    case 8: // PER-TARGET TRIM METADATA (DYNAMIC)
                    {
                        int8u target_display_index;
                        Get_S1 ( 8, target_display_index,       "target_display_index");
                        Skip_S2(12,                             "trim_slope");
                        Skip_S2(12,                             "trim_offset");
                        Skip_S2(12,                             "trim_power");
                        Skip_S2(12,                             "trim_chroma_weight");
                        Skip_S2(12,                             "trim_saturation_gain");
                        Skip_S2(12,                             "ms_weight");
                        ext_block_use_bits += 80;
                        if (ext_block_length > 11) {
                            Skip_S2(12,                         "target_mid_contrast");
                            ext_block_use_bits += 12;
                        }
                        if (ext_block_length > 12) {
                            Skip_S2(12,                         "clip_trim");
                            ext_block_use_bits += 12;
                        }
                        if (ext_block_length > 18) {
                            for (int8u i = 0; i < 6; ++i) {
                                Skip_S1(8,                      ("saturation_vector_field" + to_string(i)).c_str());
                            }
                            ext_block_use_bits += 48;
                        }
                        if (ext_block_length > 24) {
                            for (int8u i = 0; i < 6; ++i) {
                                Skip_S1(8,                      ("hue_vector_field" + to_string(i)).c_str());
                            }
                            ext_block_use_bits += 48;
                        }
                        Unique_Add(New.L8_target_display_index, target_display_index);
                        break;
                    }
                    case 9: // PER-SHOT SOURCE CONTENT PRIMARIES (DYNAMIC)
                    {
                        DV_RPU::l9 L9{};
                        Get_S1 (8, L9.source_primary_index,     "source_primary_index");
                        ext_block_use_bits += 8;
                        if (ext_block_length > 16) {
                            Get_S2(16, L9.Primaries[0],         "source_primary_red_x");
                            Get_S2(16, L9.Primaries[1],         "source_primary_red_y");
                            Get_S2(16, L9.Primaries[2],         "source_primary_green_x");
                            Get_S2(16, L9.Primaries[3],         "source_primary_green_y");
                            Get_S2(16, L9.Primaries[4],         "source_primary_blue_x");
                            Get_S2(16, L9.Primaries[5],         "source_primary_blue_y");
                            Get_S2(16, L9.Primaries[6],         "source_primary_white_x");
                            Get_S2(16, L9.Primaries[7],         "source_primary_white_y");
                            ext_block_use_bits += 128;
                            L9.Explicit = true;
                        }
                        Unique_Add(New.L9, L9);
                        break;
                    }
                    case 10:
                    {
                        DV_RPU::l10 L10{};
                        Get_S1 ( 8, L10.target_display_index,   "target_display_index");
                        Get_S2 (12, L10.target_max_PQ,          "target_max_pq");
                        Get_S2 (12, L10.target_min_PQ,          "target_min_pq");
                        Get_S1 ( 8, L10.target_primary_index,   "target_primary_index");
                        ext_block_use_bits += 40;
                        if (ext_block_length > 20) {
                            Skip_S2(16,                         "target_primary_red_x");
                            Skip_S2(16,                         "target_primary_red_y");
                            Skip_S2(16,                         "target_primary_green_x");
                            Skip_S2(16,                         "target_primary_green_y");
                            Skip_S2(16,                         "target_primary_blue_x");
                            Skip_S2(16,                         "target_primary_blue_y");
                            Skip_S2(16,                         "target_primary_white_x");
                            Skip_S2(16,                         "target_primary_white_y");
                            ext_block_use_bits += 128;
                        }
                        Unique_Add(New.L10, L10);
                        break;
                    }
                    case 11:
                    {   // Automatic Picture/Playback Optimization (APO) / Dolby Vision IQ - Content Type Metadata (L11)
                        DV_RPU::l11 L11{};
                        bool reference_mode_flag;
                        Skip_S1(4,                              "reserved");
                        Get_S1 (4, L11.content_type,            "content_type");            Param_Info1(DV_content_type(L11.content_type));
                        Skip_S1(3,                              "reserved");
                        Get_SB (reference_mode_flag,            "reference_mode_flag");
                        L11.reference_mode = reference_mode_flag;
                        Get_S1 (4, L11.white_point,             "white_point");             Param_Info1(DV_white_point(L11.white_point));
                        Skip_S2(16,                             "(Unknown)"); // Motion control metadata
                        ext_block_use_bits += 32;
                        Unique_Add(New.L11, L11);
                        break;
                    }
                    case 15:
                    {   // Consumer look metadata, Precision Rendering/Detail (Dolby Image Engine, Standard mode)
                        // Source: Dolby Vision Metadata Spec v1.5.1 (L15),
                        // cross-validated against quietvoid/dovi_tool and the
                        // official DV2 TV SDK v6.0 MFV-L15 / CST-DV2 test vectors.
                        // 10 x 8-bit fields + 4-bit revision + 4-bit reserved = 88 bits (11 bytes).
                        Skip_S1( 8,                             "confidence");
                        Skip_S1( 8,                             "precision_rendering_strength");
                        Skip_S1( 8,                             "d_local_contrast");
                        Skip_S1( 8,                             "d_brightness");
                        Skip_S1( 8,                             "d_saturation_plus_one");
                        Skip_S1( 8,                             "d_contrast_plus_one");
                        Skip_S1( 8,                             "confidence_no_pr");
                        Skip_S1( 8,                             "d_brightness_no_pr");
                        Skip_S1( 8,                             "d_saturation_plus_one_no_pr");
                        Skip_S1( 8,                             "d_contrast_plus_one_no_pr");
                        Skip_S1( 4,                             "revision");
                        Skip_S1( 4,                             "reserved");
                        ext_block_use_bits += 88;
                        break;
                    }
                    case 16:
                    {   // Local tone mapping metadata (Reference mode Image Engine)
                        // Source: Dolby Vision Metadata Spec v1.5.1 (L16),
                        // cross-validated against quietvoid/dovi_tool.
                        // Variable length: 4-bit revision + 4-bit count, then count x Level16Params (5 bytes each).
                        Skip_S1( 4,                             "revision");
                        Get_S1 ( 4, l16_count,                  "count");
                        Loop_CheckValue(l16_count, ext_block_len_bits - 8, 40, "count");
                        ext_block_use_bits += 8;
                        for (int8u p = 0; p < l16_count; ++p) {
                            Element_Begin1("Level16Params");
                            Skip_S1( 8,                         "contrast_target");
                            Skip_S1( 8,                         "precision_rendering_strength");
                            Skip_S1( 8,                         "d_local_contrast");
                            Skip_S1( 8,                         "max_d_brightness");
                            Skip_S1( 8,                         "max_d_saturation_plus_one");
                            Element_End0();
                            ext_block_use_bits += 40;
                        }
                        break;
                    }
                    case 17:
                    {   // Up mapping metadata
                        Skip_BS(ext_block_len_bits,             "(Unknown)");
                        ext_block_use_bits += ext_block_len_bits;
                        break;
                    }
                    case 18:
                    {   // Creative environment metadata (Precision Black)
                        // Source: Dolby Vision Metadata Spec v1.5.1 (L18),
                        // cross-validated against quietvoid/dovi_tool and the
                        // official DV2 TV SDK v6.0 MFV-L18 / CST-DV2 test vectors.
                        // 4 x 12-bit PQ-coded luminance + 4-bit revision + 4-bit reserved = 56 bits (7 bytes).
                        Skip_S2(12,                             "surround_luminance_PQ");
                        Skip_S2(12,                             "min_preserved_luminance_PQ");
                        Skip_S2(12,                             "adaptation_luminance_PQ");
                        Skip_S2(12,                             "max_preserved_luminance_PQ");
                        Skip_S1( 4,                             "revision");
                        Skip_S1( 4,                             "reserved");
                        ext_block_use_bits += 56;
                        break;
                    }
                    case 254: // CM version
                    {
                        int8u dm_mode, dm_version_index;
                        Get_S1 (8, dm_mode,                     "dm_mode");
                        Get_S1 (8, dm_version_index,            "dm_version_index");
                        ext_block_use_bits += 16;
                        if (!New.L254_Present) {
                            New.L254_Present = true;
                            New.dm_mode = dm_mode;
                            New.dm_version_index = dm_version_index;
                        }
                        break;
                    }
                    default:
                        Skip_BS(ext_block_len_bits,             "(Unknown)");
                        ext_block_use_bits += ext_block_len_bits;
                        break;
                    }
                    Skip_BS(ext_block_len_bits - ext_block_use_bits, "ext_dm_alignment_zero_bit");
                    Element_End0(); // ext_block_payload
                    Element_End0(); // ext_metadata_block
                }
            }
        }
        Element_End0(); // vdr_dm_data_payload
    }

    auto RemainingBitsEnd = Data_BS_Remain();
    auto size = RemainingBitsBegin - RemainingBitsEnd;
    Skip_BS((8 - (size & 7)) & 7,                               "rpu_alignment_zero_bit");
    if (Remaining_Bits() == 48) {
        // Some encoders add a zero byte before the CRC when the payload is already byte aligned
        int8u Padding;
        Peek_S1(8, Padding);
        if (!Padding) {
            Skip_S1(8,                                          "padding");
        }
    }
    Skip_S4(32,                                                 "rpu_data_crc32");
    auto CRC32_Check = DV_CRC32_Check(Buffer, Buffer_Offset + OffsetBegin, RemainingBitsBegin, Data_BS_Remain());
    Param_Info1(CRC32_Check ? "NOK" : "OK");
    Element_End0(); // rpu_data

    Element_Begin1("rbsp_trailing_bits");
    Mark_1();                                                   // rbsp_stop_one_bit
    for (int8u i = 0; i < 7; ++i) Mark_0();                     // rbsp_alignment_zero_bit
    Element_End0(); // rbsp_trailing_bits

    #undef UNSUPPORTED

    FILLING_BEGIN();
    if (CRC32_Check) {
        Fill_Conformance("DolbyVision rpu_data_crc32", "CRC-32 mismatch", {}, Conformance_Error, Stream_Video, 0);
        return;
    }
    int8u Profile{};
    switch (vdr_rpu_profile) {
    case 0:
        if (BL_video_full_range_flag)
            Profile = 5;
        break;
    case 1:
        if (el_spatial_resampling_filter_flag && !disable_residual_flag)
            Profile = vdr_bit_depth_minus8 == 4 ? 7 : 4;
        else
            Profile = 8;
        break;
    }
    if (!New.Count) {
        New.rpu_format = rpu_format;
        New.vdr_rpu_profile = vdr_rpu_profile;
        New.vdr_rpu_level = vdr_rpu_level;
        New.coefficient_data_type = coefficient_data_type;
        New.coefficient_log2_denom = (int8u)coefficient_log2_denom;
        New.vdr_rpu_normalized_idc = vdr_rpu_normalized_idc;
        New.bl_bit_depth = (int8u)(bl_bit_depth_minus8 + 8);
        New.el_bit_depth = (int8u)(el_bit_depth_minus8 + 8);
        New.vdr_bit_depth = (int8u)(vdr_bit_depth_minus8 + 8);
        New.ext_mapping_idc = ext_mapping_idc;
        New.dm_compression = dm_compression;
        New.Profile_First = Profile;
        New.chroma_resampling_explicit_filter_flag = chroma_resampling_explicit_filter_flag;
        New.BL_video_full_range_flag = BL_video_full_range_flag;
        New.spatial_resampling_filter_flag = spatial_resampling_filter_flag;
        New.el_spatial_resampling_filter_flag = el_spatial_resampling_filter_flag;
        New.disable_residual_flag = disable_residual_flag;
    }
    New.Profiles = (int16u)(New.Profiles | (1 << Profile));
    New.Count++;
    if (scene_refresh_flag)
        New.Count_Shot++;
    switch (isMEL) {
    case 0: New.Count_FEL++; break;
    case 1: New.Count_MEL++; break;
    }
    data = std::move(New);
    FILLING_END();
}

#endif // defined(MEDIAINFO_HEVC_YES) || defined(MEDIAINFO_AV1_YES)

#if defined(MEDIAINFO_HEVC_YES) || defined(MEDIAINFO_AV1_YES) || defined(MEDIAINFO_MPEG4_YES) || defined(MEDIAINFO_MXF_YES)

//---------------------------------------------------------------------------
static const float64 DV_PQ_m1=2610.0/16384;
static const float64 DV_PQ_m2=2523.0/4096*128;
static const float64 DV_PQ_c1=3424.0/4096;
static const float64 DV_PQ_c2=2413.0/4096*32;
static const float64 DV_PQ_c3=2392.0/4096*32;
static float64 DV_PQ_ToNits(float64 PQ)
{
    auto Ep=pow(PQ, 1/DV_PQ_m2);
    auto Num=Ep-DV_PQ_c1;
    if (Num<0)
        Num=0;
    return pow(Num/(DV_PQ_c2-DV_PQ_c3*Ep), 1/DV_PQ_m1)*10000;
}
static float64 DV_Nits_ToPQ(float64 Nits)
{
    auto Ym=pow(Nits/10000, DV_PQ_m1);
    return pow((DV_PQ_c1+DV_PQ_c2*Ym)/(1+DV_PQ_c3*Ym), DV_PQ_m2);
}
int16u File__Analyze::DolbyVision_Nits_ToPQ12(float64 Nits)
{
    if (Nits<=0)
        return 0;
    if (Nits>=10000)
        return 4095;
    return (int16u)float64_int64s(DV_Nits_ToPQ(Nits)*4095);
}
// Simplest luminance value having the same 12-bit PQ code, e.g. 1000 instead of 1001.42 (code 3079)
// For declared values (mastering display, target displays), also 1 code away as encoders do not always round the same way, e.g. 4000 instead of 4008.77 (code 3697, 4000 is 3696.03)
static float64 DV_PQ12_ToNits(int16u Code, bool IsMeasured=false)
{
    if (!Code)
        return 0;
    auto Nits=DV_PQ_ToNits(((float64)Code)/4095);
    auto Magnitude=floor(log10(Nits));
    int Tolerance=IsMeasured?0:1;
    for (int Digits=1; Digits<=6; Digits++)
    {
        auto Scale=pow(10.0, Digits-1-Magnitude);
        auto Candidate=round(Nits*Scale)/Scale;
        auto Candidate_Code=float64_int64s(DV_Nits_ToPQ(Candidate)*4095);
        if (Candidate_Code>=Code-Tolerance && Candidate_Code<=Code+Tolerance)
            return Candidate;
    }
    return Nits;
}
static Ztring DV_Nits_String(float64 Nits)
{
    if (Nits==floor(Nits))
        return Ztring::ToZtring(Nits, 0);
    auto Value=Ztring::ToZtring(Nits, 4);
    while (Value.back()==__T('0'))
        Value.pop_back();
    return Value;
}
static Ztring DV_Luminance_String(float64 Min, float64 Max)
{
    return __T("min: ")+Ztring::ToZtring(Min, 4)+__T(" cd/m2, max: ")+DV_Nits_String(Max)+__T(" cd/m2");
}
static Ztring DV_Primaries_String(const File__Analyze::DV_RPU::l9& L9)
{
    if (L9.Explicit)
    {
        int16u Values[8]; // MPEG units
        for (int8u i=0; i<8; i++)
        {
            int32s Value=(int16s)L9.Primaries[i];
            if (Value<0)
                Value=0;
            Values[i]=(int16u)((Value*50000+16383)/32767);
        }
        return MasteringDisplayColorVolume_Values_Compute(Values);
    }
    switch (L9.source_primary_index)
    {
        case 0: return Mpegv_colour_primaries(12); // Display P3
        case 1: return Mpegv_colour_primaries(1);
        case 2: return Mpegv_colour_primaries(9);
        case 3: return Mpegv_colour_primaries(6);
        case 4: return Mpegv_colour_primaries(5);
        case 5: return Mpegv_colour_primaries(11);
        case 6: return __T("ACES");
        case 7: return __T("S-Gamut");
        case 8: return __T("S-Gamut3.Cine");
        default: return __T("Index ")+Ztring::ToZtring(L9.source_primary_index);
    }
}
// Predefined displays, same IDs in the RPU (target_display_index) and in the XML metadata (ID)
struct dv_display
{
    int8u ID;
    int16u Nits;
    const char* Name;
};
static const dv_display DV_MasteringDisplays[]=
{
    {  7, 4000, "4000-nit, P3, D65, ST.2084, Full" },
    { 20, 1000, "1000-nit, P3, D65, ST.2084, Full" },
    { 21, 1000, "1000-nit, BT.2020, D65, ST.2084, Full" },
};
static const dv_display DV_TargetDisplays[]=
{
    {  1,  100, "100-nit, BT.709, BT.1886, Full" },
    { 16,   48, nullptr },
    { 18,   48, nullptr },
    { 21,   48, nullptr },
    { 24,  300, nullptr },
    { 25,  300, nullptr },
    { 27,  600, "600-nit, P3, D65, ST.2084, Full" },
    { 28,  600, "600-nit, BT.2020, ST.2084, Full" },
    { 37, 2000, "2000-nit, P3, D65, ST.2084, Full" },
    { 38, 2000, "2000-nit, BT.2020, D65, ST.2084, Full" },
    { 42,  108, nullptr },
    { 48, 1000, "1000-nit, P3, D65, ST.2084, Full" },
    { 49, 1000, "1000-nit, BT.2020, ST.2084, Full" },
};
template<size_t Size>
static const dv_display* DV_Display_Get(const dv_display (&Displays)[Size], int8u ID)
{
    for (const auto& Display : Displays)
        if (Display.ID==ID)
            return &Display;
    return nullptr;
}
static const char* DV_ColorRange(int8u Value)
{
    switch (Value)
    {
        case 0: return "Limited";
        case 1: return "Full";
        case 2: return "SDI";
        default: return nullptr;
    }
}
static const char* DV_ColorSpace(int8u Value)
{
    switch (Value)
    {
        case 0: return "YCbCr";
        case 1: return "RGB";
        case 2: return "IPT";
        default: return nullptr;
    }
}
static const char* DV_ChromaFormat(int8u Value)
{
    switch (Value)
    {
        case 0: return "4:2:0";
        case 1: return "4:2:2";
        case 2: return "4:4:4";
        default: return nullptr;
    }
}

//---------------------------------------------------------------------------
void File__Analyze::DolbyVision_Streams_Fill(DV_RPU& Data, dv_source Source)
{
    auto IsRPU=Source!=DV_Source_XML;
    if (IsRPU && !Data.Count)
        return;
    if (Data.IsFilled)
        return;
    Data.IsFilled=true;

    const auto Value_Or_Number=[](const char* Value, int32u Number) {
        return Value?Ztring().From_UTF8(Value):Ztring::ToZtring(Number);
    };
    const auto YesNo=[](bool Value) {
        return Value?"Yes":"No";
    };

    // Profile and layers
    auto Profile=Source==DV_Source_AV1?(int8u)10:Data.Profile_First;
    auto HasEL=Source==DV_Source_HEVC && (Data.rpu_format&0x700)==0 && !Data.disable_residual_flag;
    const char* EL_Type=Data.Count_FEL?"FEL":(Data.Count_MEL?"MEL":nullptr);
    float64 Source_Max=Data.Sources.empty()?0:DV_PQ12_ToNits((int16u)Data.Sources.front());

    if (IsRPU)
    {
        // HDR format, inserted before the other HDR formats of the stream
        Ztring HDR[Video_HDR_Format_Compatibility-Video_HDR_Format+1];
        HDR[0]=__T("Dolby Vision");
        if (Profile)
        {
            string Profile_String;
            DolbyVision_Profiles_Append(Profile_String, Profile);
            Profile_String+='.';
            add_dec_2chars(Profile_String, Profile);
            HDR[Video_HDR_Format_Profile-Video_HDR_Format].From_UTF8(Profile_String);
        }
        string Layers(HasEL && !Data.Count_EL?"":"BL+"); // No enhancement layer NAL unit with the RPUs: the stream is the enhancement layer, the base layer is in another stream
        if (HasEL)
        {
            Layers+=EL_Type?EL_Type:"EL";
            Layers+='+';
        }
        Layers+="RPU";
        HDR[Video_HDR_Format_Settings-Video_HDR_Format].From_UTF8(Layers);
        if (Data.dm_compression<sizeof(DolbyVision_Compression)/sizeof(*DolbyVision_Compression) && DolbyVision_Compression[Data.dm_compression])
            HDR[Video_HDR_Format_Compression-Video_HDR_Format].From_UTF8(DolbyVision_Compression[Data.dm_compression]);
        ZtringList Formats;
        Formats.Separator_Set(0, __T(" / "));
        Formats.Write(Retrieve_Const(Stream_Video, 0, Video_HDR_Format));
        auto Formats_Count=Formats.size();
        for (size_t i=Video_HDR_Format; i<=Video_HDR_Format_Compatibility; i++)
        {
            if (i==Video_HDR_Format_String || i==Video_HDR_Format_Commercial)
                continue;
            ZtringList List;
            List.Separator_Set(0, __T(" / "));
            List.Write(Retrieve_Const(Stream_Video, 0, i));
            List.resize(Formats_Count);
            List.insert(List.begin(), HDR[i-Video_HDR_Format]);
            auto Value=List.Read();
            if (Value.size()>(List.size()-1)*3)
                Fill(Stream_Video, 0, i, Value, true);
        }

        // Mastering display, per HDR format if the other HDR formats have different values, else only if there is no other value
        Ztring MasteringDisplay_ColorPrimaries, MasteringDisplay_Luminance;
        if (!Data.L9.empty())
            MasteringDisplay_ColorPrimaries=DV_Primaries_String(Data.L9.front());
        if (!Data.Sources.empty())
            MasteringDisplay_Luminance=DV_Luminance_String(DV_PQ12_ToNits((int16u)(Data.Sources.front()>>16)), Source_Max);
        const auto MasteringDisplay_Fill=[&](video Parameter, const Ztring& Value) {
            const auto& Current=Retrieve_Const(Stream_Video, 0, Parameter);
            if (Current.find(__T(" / "))!=string::npos)
                Fill(Stream_Video, 0, Parameter, Value+__T(" / ")+Current, true);
            else if (Current.empty() && !Value.empty())
                Fill(Stream_Video, 0, Parameter, Value);
        };
        MasteringDisplay_Fill(Video_MasteringDisplay_ColorPrimaries, MasteringDisplay_ColorPrimaries);
        MasteringDisplay_Fill(Video_MasteringDisplay_Luminance, MasteringDisplay_Luminance);
        if (!Data.L6.empty())
        {
            // Maximum of the values found (0 means unknown), the values may change during the stream
            float64 MaxCLL=0, MaxFALL=0;
            for (const auto& L6 : Data.L6)
            {
                if (MaxCLL<L6.max_content_light_level)
                    MaxCLL=L6.max_content_light_level;
                if (MaxFALL<L6.max_frame_average_light_level)
                    MaxFALL=L6.max_frame_average_light_level;
            }
            if (MaxCLL && Retrieve_Const(Stream_Video, 0, Video_MaxCLL).empty())
                Fill(Stream_Video, 0, Video_MaxCLL, MaxCLL, 0);
            if (MaxFALL && Retrieve_Const(Stream_Video, 0, Video_MaxFALL).empty())
                Fill(Stream_Video, 0, Video_MaxFALL, MaxFALL, 0);
        }

        // Active area
        if (Data.L5_Present && (Data.L5_First[0] || Data.L5_First[1] || Data.L5_First[2] || Data.L5_First[3]) && Retrieve_Const(Stream_Video, 0, Video_Active_Width).empty() && Retrieve_Const(Stream_Video, 0, Video_Active_Height).empty())
        {
            auto Width=Retrieve_Const(Stream_Video, 0, Video_Width).To_int64u();
            auto Height=Retrieve_Const(Stream_Video, 0, Video_Height).To_int64u();
            auto Crop_Width=(int64u)Data.L5_First[0]+Data.L5_First[1];
            auto Crop_Height=(int64u)Data.L5_First[2]+Data.L5_First[3];
            if (Width>Crop_Width && Height>Crop_Height)
            {
                Width-=Crop_Width;
                Height-=Crop_Height;
                Fill(Stream_Video, 0, Video_Active_Width, Width);
                Fill(Stream_Video, 0, Video_Active_Height, Height);
                auto PixelAspectRatio=Retrieve_Const(Stream_Video, 0, Video_PixelAspectRatio).To_float64();
                if (!PixelAspectRatio)
                    PixelAspectRatio=1;
                Fill(Stream_Video, 0, Video_Active_DisplayAspectRatio, ((float64)Width)/Height*PixelAspectRatio, 3);
            }
        }

        // Conformance, same checks as with XML metadata
        if (Data.Count_CMv40 && !Data.L254_Present)
            Fill_Conformance("DolbyVision Level254", "Level254 is missing", {}, Conformance_Warning, Stream_Video, 0);
        if (Source_Max)
            for (const auto& L6 : Data.L6)
                if (L6.max_content_light_level>Source_Max)
                    Fill_Conformance("DolbyVision Level6", ("MaxCLL is more than mastering display peak brightness (actual "+DV_Nits_String(L6.max_content_light_level).To_UTF8()+", expected <= "+DV_Nits_String(Source_Max).To_UTF8()+')').c_str(), {}, Conformance_Warning, Stream_Video, 0);
    }

    // Details
    string Summary;
    ZtringList Profiles;
    Profiles.Separator_Set(0, __T(" / "));
    if (Source==DV_Source_AV1)
        Profiles.push_back(Ztring::ToZtring(Profile));
    else
        for (int8u i=1; i<16; i++)
            if (Data.Profiles&(1<<i))
                Profiles.push_back(Ztring::ToZtring(i));
    if (!Profiles.empty())
        Summary="Profile "+Profiles.Read().To_UTF8();
    if (HasEL && EL_Type)
    {
        if (!Summary.empty())
            Summary+=", ";
        Summary+=EL_Type;
    }
    const dv_display* MasteringDisplay=nullptr;
    if (!Data.MasteringDisplay_ID.empty())
        MasteringDisplay=DV_Display_Get(DV_MasteringDisplays, Data.MasteringDisplay_ID.To_int8u());
    Ztring MasteringDisplay_Name=MasteringDisplay?Ztring().From_UTF8(MasteringDisplay->Name):Data.MasteringDisplay_Name;
    if (Summary.empty())
        Summary=MasteringDisplay_Name.empty()?string("Yes"):MasteringDisplay_Name.To_UTF8();
    Fill(Stream_Video, 0, "DolbyVision", Summary);
    if (!Profiles.empty())
        Fill(Stream_Video, 0, "DolbyVision Profile", Profiles.Read());
    if (HasEL && EL_Type)
    {
        if (Data.Count_MEL && Data.Count_FEL)
            Fill(Stream_Video, 0, "DolbyVision EnhancementLayer", "MEL ("+to_string(Data.Count_MEL)+") / FEL ("+to_string(Data.Count_FEL)+')');
        else
            Fill(Stream_Video, 0, "DolbyVision EnhancementLayer", EL_Type);
    }
    if (IsRPU)
    {
        string BitDepth=to_string(Data.bl_bit_depth)+" (BL) / ";
        if (HasEL)
            BitDepth+=to_string(Data.el_bit_depth)+" (EL) / ";
        BitDepth+=to_string(Data.vdr_bit_depth)+" bits (VDR)";
        Fill(Stream_Video, 0, "DolbyVision BitDepth", BitDepth);
        Fill(Stream_Video, 0, "DolbyVision BaseLayer_ColorRange", Data.BL_video_full_range_flag?"Full":"Limited");
    }

    // Content mapping and display management versions
    auto CMVersion=Data.CMVersion;
    if (CMVersion.empty() && Data.Count_CMv29)
        CMVersion.From_UTF8(!Data.Count_CMv40?"2.9":(Data.Count_CMv40==Data.Count_CMv29?"4.0":"2.9 / 4.0"));
    if (!CMVersion.empty())
        Fill(Stream_Video, 0, "DolbyVision CMVersion", CMVersion);
    if (Data.L254_Present)
        Fill(Stream_Video, 0, "DolbyVision DMVersion", Data.dm_version_index);

    // Mastering display
    if (!MasteringDisplay_Name.empty())
        Fill(Stream_Video, 0, "DolbyVision MasteringDisplay_Name", MasteringDisplay_Name);
    if (!Data.MasteringDisplay_ID.empty())
        Fill(Stream_Video, 0, "DolbyVision MasteringDisplay_ID", Data.MasteringDisplay_ID);
    if (!Data.L9.empty())
    {
        ZtringList List;
        List.Separator_Set(0, __T(" / "));
        for (const auto& L9 : Data.L9)
        {
            auto Value=DV_Primaries_String(L9);
            if (List.Find(Value)==Error)
                List.push_back(Value);
        }
        Fill(Stream_Video, 0, "DolbyVision MasteringDisplay_ColorPrimaries", List.Read());
    }
    if (!Data.Sources.empty())
    {
        ZtringList List;
        List.Separator_Set(0, __T(" / "));
        for (auto Source_Item : Data.Sources)
            List.push_back(DV_Luminance_String(DV_PQ12_ToNits((int16u)(Source_Item>>16)), DV_PQ12_ToNits((int16u)Source_Item)));
        Fill(Stream_Video, 0, "DolbyVision MasteringDisplay_Luminance", List.Read());
    }
    if (Data.source_diagonal)
    {
        Fill(Stream_Video, 0, "DolbyVision MasteringDisplay_Diagonal", to_string(Data.source_diagonal)+" inches");
        Fill_SetOptions(Stream_Video, 0, "DolbyVision MasteringDisplay_Diagonal", "N NT");
    }

    // Target displays, names of the predefined displays if known
    {
        vector<pair<float64, Ztring> > Targets;
        const auto Target_Add=[&](float64 Nits, const Ztring& Name) {
            for (const auto& Target : Targets)
                if (Target.second==Name)
                    return;
            Targets.emplace_back(Nits, Name);
        };
        for (auto target_display_index : Data.L8_target_display_index)
        {
            const auto* Predefined=DV_Display_Get(DV_TargetDisplays, target_display_index);
            float64 Nits=Predefined?Predefined->Nits:0;
            for (const auto& L10 : Data.L10)
                if (L10.target_display_index==target_display_index)
                {
                    Nits=DV_PQ12_ToNits(L10.target_max_PQ);
                    break;
                }
            Ztring Name;
            if (Predefined && Predefined->Name)
                Name.From_UTF8(Predefined->Name);
            else
            {
                for (const auto& TargetDisplay_Name : Data.TargetDisplay_Names)
                    if (TargetDisplay_Name.first==target_display_index)
                        Name=TargetDisplay_Name.second;
                if (Name.empty())
                    Name=Nits?(DV_Nits_String(Nits)+__T("-nit")):(__T("Target display ")+Ztring::ToZtring(target_display_index));
            }
            Target_Add(Nits, Name);
        }
        for (auto target_max_PQ : Data.L2_target_max_PQ)
        {
            auto Nits=DV_PQ12_ToNits(target_max_PQ);
            auto IsKnown=false;
            for (const auto& Target : Targets)
                if (Target.first==Nits)
                    IsKnown=true;
            if (!IsKnown)
                Target_Add(Nits, DV_Nits_String(Nits)+__T("-nit"));
        }
        stable_sort(Targets.begin(), Targets.end(), [](const pair<float64, Ztring>& a, const pair<float64, Ztring>& b) { return a.first<b.first; });
        ZtringList List;
        List.Separator_Set(0, __T(" / "));
        for (const auto& Target : Targets)
            List.push_back(Target.second);
        if (!List.empty())
            Fill(Stream_Video, 0, "DolbyVision TargetDisplays", List.Read());
    }

    // Light levels
    const auto LightLevel_Fill=[&](const char* Parameter, const vector<float64>& Values) {
        // Numeric value for XML/JSON, value with measure for text
        ZtringList List;
        List.Separator_Set(0, __T(" / "));
        for (auto Value : Values)
        {
            auto Value_String=MediaInfoLib::Config.Language_Get(DV_Nits_String(Value), __T(" cd/m2"));
            if (List.Find(Value_String)==Error)
                List.push_back(Value_String);
        }
        auto Name=string("DolbyVision ")+Parameter;
        Fill(Stream_Video, 0, Name.c_str(), DV_Nits_String(Values.front()));
        Fill_SetOptions(Stream_Video, 0, Name.c_str(), "N NFY");
        Name+="/String";
        Fill(Stream_Video, 0, Name.c_str(), List.Read());
        Fill_SetOptions(Stream_Video, 0, Name.c_str(), "Y NTN");
    };
    if (!Data.L6.empty())
    {
        // MaxCLL and MaxFALL are in the video stream, see above
        ZtringList Luminance;
        Luminance.Separator_Set(0, __T(" / "));
        for (const auto& L6 : Data.L6)
        {
            if (L6.max_display_mastering_luminance)
            {
                auto Luminance_Value=DV_Luminance_String(((float64)L6.min_display_mastering_luminance)/10000, L6.max_display_mastering_luminance);
                if (Luminance.Find(Luminance_Value)==Error)
                    Luminance.push_back(Luminance_Value);
            }
        }
        if (!Luminance.empty())
            Fill(Stream_Video, 0, "DolbyVision MasteringDisplay_Luminance_HDR10", Luminance.Read());
    }

    // Active area
    if (Data.L5_Present && (Data.L5_Max[0] || Data.L5_Max[1] || Data.L5_Max[2] || Data.L5_Max[3])) // Not displayed if 0 everywhere (full frame)
    {
        static const char* const Sides[4]={ "left", "right", "top", "bottom" };
        static const int8u Order[4]={ 2, 3, 0, 1 };
        string ActiveArea;
        for (auto i : Order)
        {
            if (!ActiveArea.empty())
                ActiveArea+=", ";
            ActiveArea+=Sides[i];
            ActiveArea+=": ";
            ActiveArea+=to_string(Data.L5_Min[i]);
            if (Data.L5_Max[i]!=Data.L5_Min[i])
                ActiveArea+='-'+to_string(Data.L5_Max[i]);
        }
        Fill(Stream_Video, 0, "DolbyVision ActiveArea", ActiveArea);
    }
    {
        auto AspectRatios=Data.Active_DisplayAspectRatios;
        if (!Data.L5.empty())
        {
            auto Width=Retrieve_Const(Stream_Video, 0, Video_Width).To_int64u();
            auto Height=Retrieve_Const(Stream_Video, 0, Video_Height).To_int64u();
            auto PixelAspectRatio=Retrieve_Const(Stream_Video, 0, Video_PixelAspectRatio).To_float64();
            if (!PixelAspectRatio)
                PixelAspectRatio=1;
            for (const auto& L5 : Data.L5)
            {
                auto Crop_Width=(int64u)L5.Offsets[0]+L5.Offsets[1];
                auto Crop_Height=(int64u)L5.Offsets[2]+L5.Offsets[3];
                if (Width>Crop_Width && Height>Crop_Height)
                    AspectRatios.push_back(((float64)(Width-Crop_Width))/(Height-Crop_Height)*PixelAspectRatio);
            }
        }
        sort(AspectRatios.begin(), AspectRatios.end());
        // Numeric value for XML/JSON, value with its usual name for text
        ZtringList List, List_String;
        List.Separator_Set(0, __T(" / "));
        List_String.Separator_Set(0, __T(" / "));
        for (auto AspectRatio : AspectRatios)
        {
            auto Value=Ztring::ToZtring(AspectRatio, 3);
            if (List.Find(Value)==Error)
                List.push_back(Value);
            auto Value_String=DisplayAspectRatio_String((float)AspectRatio);
            if (List_String.Find(Value_String)==Error)
                List_String.push_back(Value_String);
        }
        if (!List.empty())
        {
            Fill(Stream_Video, 0, "DolbyVision Active_DisplayAspectRatio", List.Read());
            Fill_SetOptions(Stream_Video, 0, "DolbyVision Active_DisplayAspectRatio", "N NFY");
            Fill(Stream_Video, 0, "DolbyVision Active_DisplayAspectRatio/String", List_String.Read());
            Fill_SetOptions(Stream_Video, 0, "DolbyVision Active_DisplayAspectRatio/String", "Y NTN");
        }
    }

    // Content type
    if (!Data.L11.empty())
    {
        ZtringList ContentType, WhitePoint, ReferenceMode;
        ContentType.Separator_Set(0, __T(" / "));
        WhitePoint.Separator_Set(0, __T(" / "));
        ReferenceMode.Separator_Set(0, __T(" / "));
        for (const auto& L11 : Data.L11)
        {
            const char* ContentType_Name=DV_content_type(L11.content_type);
            auto ContentType_Value=Value_Or_Number(*ContentType_Name?ContentType_Name:nullptr, L11.content_type);
            if (ContentType.Find(ContentType_Value)==Error)
                ContentType.push_back(ContentType_Value);
            const char* WhitePoint_Name=DV_white_point(L11.white_point);
            auto WhitePoint_Value=Value_Or_Number(*WhitePoint_Name?WhitePoint_Name:nullptr, L11.white_point);
            if (WhitePoint.Find(WhitePoint_Value)==Error)
                WhitePoint.push_back(WhitePoint_Value);
            if (L11.reference_mode!=(int8u)-1)
            {
                Ztring ReferenceMode_Value; ReferenceMode_Value.From_UTF8(YesNo(L11.reference_mode));
                if (ReferenceMode.Find(ReferenceMode_Value)==Error)
                    ReferenceMode.push_back(ReferenceMode_Value);
            }
        }
        Fill(Stream_Video, 0, "DolbyVision Content_Type", ContentType.Read());
        Fill(Stream_Video, 0, "DolbyVision WhitePoint", WhitePoint.Read());
        if (!ReferenceMode.empty())
            Fill(Stream_Video, 0, "DolbyVision ReferenceMode", ReferenceMode.Read());
    }

    // Values for the whole stream, always available with XML metadata, only when the whole stream is parsed with the RPU
    if (!IsRPU || Config->ParseSpeed>=1.0)
    {
        if (Data.L1_max_PQ)
        {
            LightLevel_Fill("MaxCLL_Analysis", { DV_PQ12_ToNits(Data.L1_max_PQ, true) });
            LightLevel_Fill("MaxFALL_Analysis", { DV_PQ12_ToNits(Data.L1_avg_PQ, true) });
        }
        if (Data.Count_Shot || (IsRPU && Data.Count_CMv29))
            Fill(Stream_Video, 0, "DolbyVision ShotCount", Data.Count_Shot);
    }

    // Low level details
    const auto Fill_Hidden=[&](const char* Parameter, const Ztring& Value) {
        auto Name=string("DolbyVision ")+Parameter;
        Fill(Stream_Video, 0, Name.c_str(), Value);
        Fill_SetOptions(Stream_Video, 0, Name.c_str(), "N NT");
    };
    if (IsRPU)
    {
        Fill_Hidden("RPU_Format", Ztring::ToZtring(Data.rpu_format));
        Fill_Hidden("VDR_RPU_Profile", Ztring::ToZtring(Data.vdr_rpu_profile));
        Fill_Hidden("VDR_RPU_Level", Ztring::ToZtring(Data.vdr_rpu_level));
        Fill_Hidden("VDR_RPU_NormalizedIdc", Ztring::ToZtring(Data.vdr_rpu_normalized_idc));
        Fill_Hidden("CoefficientDataType", Value_Or_Number(Data.coefficient_data_type==0?"Fixed":(Data.coefficient_data_type==1?"Float":nullptr), Data.coefficient_data_type));
        if (!Data.coefficient_data_type)
            Fill_Hidden("CoefficientLog2Denom", Ztring::ToZtring(Data.coefficient_log2_denom));
        Fill_Hidden("ChromaResamplingExplicitFilter", Ztring().From_UTF8(YesNo(Data.chroma_resampling_explicit_filter_flag)));
        Fill_Hidden("SpatialResamplingFilter", Ztring().From_UTF8(YesNo(Data.spatial_resampling_filter_flag)));
        Fill_Hidden("EL_SpatialResamplingFilter", Ztring().From_UTF8(YesNo(Data.el_spatial_resampling_filter_flag)));
        Fill_Hidden("DisableResidual", Ztring().From_UTF8(YesNo(Data.disable_residual_flag)));
        Fill_Hidden("ExtMappingIdc", Ztring::ToZtring(Data.ext_mapping_idc));
        if (Data.Mapping_Present)
        {
            Fill_Hidden("MappingColorSpace", Ztring::ToZtring(Data.mapping_color_space));
            Fill_Hidden("MappingChromaFormat", Ztring::ToZtring(Data.mapping_chroma_format_idc));
            if (HasEL)
                Fill_Hidden("NLQ_Method", Value_Or_Number(Data.nlq_method_idc?nullptr:"Linear dead zone", Data.nlq_method_idc));
            Fill_Hidden("Partitions", Ztring::ToZtring(((int64u)Data.num_x_partitions_minus1)+1)+__T('x')+Ztring::ToZtring(((int64u)Data.num_y_partitions_minus1)+1));
        }
        if (Data.DM_Present)
        {
            Fill_Hidden("Signal_EOTF", Ztring::ToZtring(Data.signal_eotf));
            Fill_Hidden("Signal_EOTF_Parameters", Ztring::ToZtring(Data.signal_eotf_param0)+__T(", ")+Ztring::ToZtring(Data.signal_eotf_param1)+__T(", ")+Ztring::ToZtring(Data.signal_eotf_param2));
            Fill_Hidden("Signal_BitDepth", Ztring::ToZtring(Data.signal_bit_depth));
            Fill_Hidden("Signal_ColorSpace", Value_Or_Number(DV_ColorSpace(Data.signal_color_space), Data.signal_color_space));
            Fill_Hidden("Signal_ChromaFormat", Value_Or_Number(DV_ChromaFormat(Data.signal_chroma_format), Data.signal_chroma_format));
            Fill_Hidden("Signal_ColorRange", Value_Or_Number(DV_ColorRange(Data.signal_full_range_flag), Data.signal_full_range_flag));
        }
    }
    if (Data.L254_Present)
        Fill_Hidden("DMMode", Ztring::ToZtring(Data.dm_mode));
}

#endif // defined(MEDIAINFO_HEVC_YES) || defined(MEDIAINFO_AV1_YES) || defined(MEDIAINFO_MPEG4_YES) || defined(MEDIAINFO_MXF_YES)

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_AV1_YES) || defined(MEDIAINFO_AVC_YES) || defined(MEDIAINFO_HEVC_YES) || defined(MEDIAINFO_MPEG4_YES)
void File__Analyze::Get_LightLevel(Ztring &MaxCLL, Ztring &MaxFALL, int32u Divisor)
{
    //Parsing
    if (Divisor-1)
    {
        int32u maximum_content_light_level, maximum_frame_average_light_level;
        Get_B4 (maximum_content_light_level,                    "maximum_content_light_level");
        Get_B4 (maximum_frame_average_light_level,              "maximum_frame_average_light_level");

        auto Decimals=to_string(Divisor).size()-1;
        if (maximum_content_light_level)
            MaxCLL=Ztring::ToZtring(((float32)maximum_content_light_level)/Divisor, (int8u)Decimals);
        if (maximum_frame_average_light_level)
            MaxFALL=Ztring::ToZtring(((float32)maximum_frame_average_light_level)/Divisor, (int8u)Decimals);
    }
    else
    {
        int16u maximum_content_light_level, maximum_frame_average_light_level;
        Get_B2 (maximum_content_light_level,                    "maximum_content_light_level");
        Get_B2 (maximum_frame_average_light_level,              "maximum_frame_average_light_level");

        if (maximum_content_light_level)
            MaxCLL=Ztring::ToZtring(maximum_content_light_level);
        if (maximum_frame_average_light_level)
            MaxFALL=Ztring::ToZtring(maximum_frame_average_light_level);
    }
}
#endif

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_AV1_YES) || defined(MEDIAINFO_HEVC_YES) || defined(MEDIAINFO_MK_YES)
void File__Analyze::Get_SMPTE_ST_2094_40(int8u& application_version, bool& IsHDRplus, bool& tone_mapping_flag)
{
    Get_B1 (application_version,                                "application_version");
    if (application_version<=1)
    {
        int32u targeted_system_display_maximum_luminance, maxscl[4]{}, distribution_maxrgb_percentiles[16]{};
        int16u fraction_bright_pixels;
        int8u num_distribution_maxrgb_percentiles, distribution_maxrgb_percentages[16]{}, num_windows, num_bezier_curve_anchors;
        bool targeted_system_display_actual_peak_luminance_flag, mastering_display_actual_peak_luminance_flag, color_saturation_mapping_flag;
        BS_Begin();
        Get_S1 ( 2, num_windows,                                "num_windows");

        for (int8u w=1; w<num_windows; w++)
        {
            Element_Begin1("window");
            Skip_S2(16,                                         "window_upper_left_corner_x");
            Skip_S2(16,                                         "window_upper_left_corner_y");
            Skip_S2(16,                                         "window_lower_right_corner_x");
            Skip_S2(16,                                         "window_lower_right_corner_y");
            Skip_S2(16,                                         "center_of_ellipse_x");
            Skip_S2(16,                                         "center_of_ellipse_y");
            Skip_S1( 8,                                         "rotation_angle");
            Skip_S2(16,                                         "semimajor_axis_internal_ellipse");
            Skip_S2(16,                                         "semimajor_axis_external_ellipse");
            Skip_S2(16,                                         "semiminor_axis_external_ellipse");
            Skip_SB(                                            "overlap_process_option");
            Element_End0();
        }

        Get_S4 (27, targeted_system_display_maximum_luminance,  "targeted_system_display_maximum_luminance");
        TEST_SB_GET (targeted_system_display_actual_peak_luminance_flag, "targeted_system_display_actual_peak_luminance_flag");
            int8u num_rows_targeted_system_display_actual_peak_luminance, num_cols_targeted_system_display_actual_peak_luminance;
            Get_S1(5, num_rows_targeted_system_display_actual_peak_luminance, "num_rows_targeted_system_display_actual_peak_luminance");
            Get_S1(5, num_cols_targeted_system_display_actual_peak_luminance, "num_cols_targeted_system_display_actual_peak_luminance");
            for(int8u i=0; i<num_rows_targeted_system_display_actual_peak_luminance; i++)
                for(int8u j=0; j<num_cols_targeted_system_display_actual_peak_luminance; j++)
                    Skip_S1(4,                                   "targeted_system_display_actual_peak_luminance");
        TEST_SB_END();

        for (int8u w=0; w<num_windows; w++)
        {
            Element_Begin1("window");
            for(int8u i=0; i<3; i++)
            {
                Get_S3 (17, maxscl[i],                          "maxscl");          Param_Info2(Ztring::ToZtring(((float)maxscl[i])/10, 1), " cd/m2");
            }
            Get_S3 (17, maxscl[3],                              "average_maxrgb");  Param_Info2(Ztring::ToZtring(((float)maxscl[3])/10, 1), " cd/m2");

            Get_S1(4, num_distribution_maxrgb_percentiles,      "num_distribution_maxrgb_percentiles");
            for (int8u i=0; i< num_distribution_maxrgb_percentiles; i++)
            {
                Element_Begin1(                                 "distribution_maxrgb");
                Get_S1 ( 7, distribution_maxrgb_percentages[i], "distribution_maxrgb_percentages");
                Get_S3 (17, distribution_maxrgb_percentiles[i], "distribution_maxrgb_percentiles");
                Element_Info1(distribution_maxrgb_percentages[i]);
                Element_Info1(distribution_maxrgb_percentiles[i]);
                Element_End0();
            }
            Get_S2 (10, fraction_bright_pixels,                 "fraction_bright_pixels");
            Element_End0();
        }

        TEST_SB_GET (mastering_display_actual_peak_luminance_flag, "mastering_display_actual_peak_luminance_flag");
            int8u num_rows_mastering_display_actual_peak_luminance, num_cols_mastering_display_actual_peak_luminance;
            Get_S1(5, num_rows_mastering_display_actual_peak_luminance, "num_rows_mastering_display_actual_peak_luminance");
            Get_S1(5, num_cols_mastering_display_actual_peak_luminance, "num_cols_mastering_display_actual_peak_luminance");
            for(int8u i=0; i< num_rows_mastering_display_actual_peak_luminance; i++)
                for(int8u j=0; j< num_cols_mastering_display_actual_peak_luminance; j++)
                    Skip_S1(4,                                   "mastering_display_actual_peak_luminance");
        TEST_SB_END();

        for (int8u w=0; w<num_windows; w++)
        {
            Element_Begin1("window");
            TEST_SB_GET (tone_mapping_flag,                     "tone_mapping_flag");
                Skip_S2(12,                                     "knee_point_x");
                Skip_S2(12,                                     "knee_point_y");
                Get_S1(4, num_bezier_curve_anchors,             "num_bezier_curve_anchors");
                for (int8u i = 0; i < num_bezier_curve_anchors; i++)
                    Skip_S2(10,                                 "bezier_curve_anchor");
            TEST_SB_END();
            Element_End0();
        }
        TEST_SB_GET (color_saturation_mapping_flag,             "color_saturation_mapping_flag");
            Info_S1(6, color_saturation_weight,                 "color_saturation_weight"); Param_Info1(((float)color_saturation_weight)/8);
        TEST_SB_END();
        BS_End();

        FILLING_BEGIN();
            IsHDRplus=true;
            if (num_windows!=1 || targeted_system_display_actual_peak_luminance_flag || num_distribution_maxrgb_percentiles!=9 || fraction_bright_pixels || mastering_display_actual_peak_luminance_flag || (distribution_maxrgb_percentages[2]>100 && distribution_maxrgb_percentages[2]!=0xFF) || (!tone_mapping_flag && targeted_system_display_maximum_luminance) || (tone_mapping_flag && num_bezier_curve_anchors>9) || color_saturation_mapping_flag)
                IsHDRplus=false;
            for(int8u i=0; i<4; i++)
                if (maxscl[i]>100000)
                    IsHDRplus=false;
            if (IsHDRplus)
                for(int8u i=0; i<9; i++)
                {
                    static const int8u distribution_maxrgb_percentages_List[9]={1, 5, 10, 25, 50, 75, 90, 95, 99};
                    if (distribution_maxrgb_percentages[i]!=distribution_maxrgb_percentages_List[i])
                        IsHDRplus=false;
                    if (distribution_maxrgb_percentiles[i]>100000)
                        IsHDRplus=false;
                }
        FILLING_END();
    }
}
#endif

//***************************************************************************
// Preparation des streams
//***************************************************************************

//---------------------------------------------------------------------------
size_t File__Analyze::Stream_Prepare (stream_t KindOfStream, size_t StreamPos)
{
    //Integrity
    if (KindOfStream<0 || KindOfStream>Stream_Max)
        return Error;

    //Clear
    if (KindOfStream==Stream_Max)
    {
        StreamKind_Last=Stream_Max;
        StreamPos_Last=(size_t)-1;
        return 0;
    }

    if (StreamPos>=Count_Get(KindOfStream))
    {
        //Add a stream
        (*Stream)[KindOfStream].resize((*Stream)[KindOfStream].size()+1);
        (*Stream_More)[KindOfStream].resize((*Stream_More)[KindOfStream].size()+1);
        StreamKind_Last=KindOfStream;
        StreamPos_Last=(*Stream)[KindOfStream].size()-1;
    }
    else
    {
        //Insert a stream
        (*Stream)[KindOfStream].insert((*Stream)[KindOfStream].begin()+StreamPos, ZtringList());
        (*Stream_More)[KindOfStream].insert((*Stream_More)[KindOfStream].begin()+StreamPos, ZtringListList());
        StreamKind_Last=KindOfStream;
        StreamPos_Last=StreamPos;
    }

    //Filling basic info
    Fill(StreamKind_Last, StreamPos_Last, (size_t)General_Count, Count_Get(StreamKind_Last, StreamPos_Last));
    Fill(StreamKind_Last, StreamPos_Last, General_StreamKind, MediaInfoLib::Config.Info_Get(StreamKind_Last).Read(General_StreamKind, Info_Text));
    Fill(StreamKind_Last, StreamPos_Last, General_StreamKind_String, MediaInfoLib::Config.Language_Get(MediaInfoLib::Config.Info_Get(StreamKind_Last).Read(General_StreamKind, Info_Text)), true);
    size_t Count=Count_Get(StreamKind_Last);
    Stream_Count_Update(StreamKind_Last);
    for (size_t Pos=Count==2?0:StreamPos_Last; Pos<Count; Pos++) //Only the shifted streams, or all streams if the position is no longer alone
    {
        Fill(StreamKind_Last, Pos, General_StreamKindID, Pos, 10, true);
        if (Count>1)
            Fill(StreamKind_Last, Pos, General_StreamKindPos, Pos+1, 10, true);
        else
            Clear(StreamKind_Last, Pos, General_StreamKindPos);
    }

    //Filling Lists & Counts
    if (!IsSub && KindOfStream!=Stream_General)
    {
        const Ztring& StreamKind_Text=Get(KindOfStream, 0, General_StreamKind, Info_Text);
        Fill(Stream_General, 0, Ztring(StreamKind_Text+__T("Count")).To_Local().c_str(), Count_Get(KindOfStream), 10, true);
    }

    //File name and dates
    if (!IsSub && KindOfStream==Stream_General && File_Name.size()>0)
    {
        //File name
        if (File_Name.find(__T("://"))==string::npos)
        {
            Fill (Stream_General, 0, General_CompleteName, File_Name);
            Fill (Stream_General, 0, General_FolderName, FileName::Path_Get(File_Name));
            Fill (Stream_General, 0, General_FileName, FileName::Name_Get(File_Name));
            Fill (Stream_General, 0, General_FileExtension, FileName::Extension_Get(File_Name));
        }
        else
        {
            Ztring FileName_Modified=File_Name;
            size_t Begin=FileName_Modified.find(__T(':'), 6);
            size_t End=FileName_Modified.find(__T('@'));
            if (Begin!=string::npos && End!=string::npos && Begin<End)
                FileName_Modified.erase(Begin, End-Begin);
            Fill (Stream_General, 0, General_CompleteName, FileName_Modified);
            size_t Query=FileName_Modified.find(__T('?'));
            if (Query!=string::npos)
                FileName_Modified.erase(Query);
            size_t FileName_Modified_PathSeparatorOffset=FileName_Modified.find_last_of(__T("/\\"));
            if (FileName_Modified_PathSeparatorOffset!=string::npos)
            {
                Fill (Stream_General, 0, General_FolderName, FileName_Modified.substr(0, FileName_Modified_PathSeparatorOffset));
                size_t FileName_Modified_ExtensionSeparatorOffset=FileName_Modified.find_last_of(__T('.'));
                if (FileName_Modified_ExtensionSeparatorOffset!=string::npos && FileName_Modified_ExtensionSeparatorOffset>FileName_Modified_PathSeparatorOffset)
                {
                    Fill (Stream_General, 0, General_FileName, FileName_Modified.substr(FileName_Modified_PathSeparatorOffset+1, FileName_Modified_ExtensionSeparatorOffset-(FileName_Modified_PathSeparatorOffset+1)));
                    Fill (Stream_General, 0, General_FileExtension, FileName_Modified.substr(FileName_Modified_ExtensionSeparatorOffset+1));
                }
                else
                    Fill (Stream_General, 0, General_FileName, FileName_Modified.substr(FileName_Modified_PathSeparatorOffset+1));
            }
        }
        if (Retrieve(Stream_General, 0, General_FileExtension).empty())
            Fill(Stream_General, 0, General_FileNameExtension, Retrieve(Stream_General, 0, General_FileName));
        else
            Fill(Stream_General, 0, General_FileNameExtension, Retrieve(Stream_General, 0, General_FileName)+__T('.')+Retrieve(Stream_General, 0, General_FileExtension));

        //File dates
        #if defined(MEDIAINFO_FILE_YES)
        File F(File_Name);
        Fill (Stream_General, 0, General_File_Created_Date, F.Created_Get());
        Fill (Stream_General, 0, General_File_Created_Date_Local, F.Created_Local_Get());
        Fill (Stream_General, 0, General_File_Modified_Date, F.Modified_Get());
        Fill (Stream_General, 0, General_File_Modified_Date_Local, F.Modified_Local_Get());
        #endif //defined(MEDIAINFO_FILE_YES)
    }

    //File size
    if (((!IsSub || !File_Name.empty()) && KindOfStream==Stream_General && File_Size!=(int64u)-1))
        Fill (Stream_General, 0, General_FileSize, File_Size);

    //Fill with already ready data
    stream_t Fill_Temp_StreamKind=(Fill_Temp[StreamKind_Last].empty()?Stream_Max:StreamKind_Last);
    for (size_t Pos=0; Pos<Fill_Temp[Fill_Temp_StreamKind].size(); Pos++)
        if (Fill_Temp[Fill_Temp_StreamKind][Pos].Parameter.IsNumber())
            Fill(StreamKind_Last, StreamPos_Last, Fill_Temp[Fill_Temp_StreamKind][Pos].Parameter.To_int32u(), Fill_Temp[Fill_Temp_StreamKind][Pos].Value);
        else
        {
            Fill(StreamKind_Last, StreamPos_Last, Fill_Temp[Fill_Temp_StreamKind][Pos].Parameter.To_UTF8().c_str(), Fill_Temp[Fill_Temp_StreamKind][Pos].Value);
            #if MEDIAINFO_DEMUX
                if (!Retrieve(KindOfStream, StreamPos_Last, "Demux_InitBytes").empty())
                    Fill_SetOptions(KindOfStream, StreamPos_Last, "Demux_InitBytes", "N NT");
            #endif //MEDIAINFO_DEMUX
            map<string, string>::iterator Fill_Temp_Option=Fill_Temp_Options[Fill_Temp_StreamKind].find(Fill_Temp[Fill_Temp_StreamKind][Pos].Parameter.To_UTF8());
            if (Fill_Temp_Option!=Fill_Temp_Options[Fill_Temp_StreamKind].end())
                Fill_SetOptions(KindOfStream, StreamPos_Last, Fill_Temp_Option->first.c_str(), Fill_Temp_Option->second.c_str());
        }
    Fill_Temp[Fill_Temp_StreamKind].clear();
    Fill_Temp_Options[Fill_Temp_StreamKind].clear();

    return StreamPos_Last; //The position in the stream count
}

size_t File__Analyze::Stream_Erase (stream_t KindOfStream, size_t StreamPos)
{
    //Integrity
    if (KindOfStream>Stream_Max || StreamPos>=Count_Get(KindOfStream))
        return Error;

    //Filling Lists & Counts
    if (!IsSub && KindOfStream!=Stream_General)
    {
        const Ztring& StreamKind_Text=Get(KindOfStream, 0, General_StreamKind, Info_Text);
        ZtringList Temp; Temp.Separator_Set(0, __T(" / "));
        Temp.Write(Retrieve(Stream_General, 0, Ztring(StreamKind_Text+__T("_Codec_List")).To_Local().c_str()));
        if (StreamPos<Temp.size())
            Temp.erase(Temp.begin()+StreamPos);
        Fill(Stream_General, 0, Ztring(StreamKind_Text+__T("_Codec_List")).To_Local().c_str(), Temp.Read(), true);
        Temp.Write(Retrieve(Stream_General, 0, Ztring(StreamKind_Text+__T("_Language_List")).To_Local().c_str()));
        if (StreamPos<Temp.size())
            Temp.erase(Temp.begin()+StreamPos);
        Fill(Stream_General, 0, Ztring(StreamKind_Text+__T("_Language_List")).To_Local().c_str(), Temp.Read(), true);
        Temp.Write(Retrieve(Stream_General, 0, Ztring(StreamKind_Text+__T("_Format_List")).To_Local().c_str()));
        if (StreamPos<Temp.size())
            Temp.erase(Temp.begin()+StreamPos);
        Fill(Stream_General, 0, Ztring(StreamKind_Text+__T("_Format_List")).To_Local().c_str(), Temp.Read(), true);
        Temp.Write(Retrieve(Stream_General, 0, Ztring(StreamKind_Text+__T("_Format_WithHint_List")).To_Local().c_str()));
        if (StreamPos<Temp.size())
            Temp.erase(Temp.begin()+StreamPos);
        Fill(Stream_General, 0, Ztring(StreamKind_Text+__T("_Format_WithHint_List")).To_Local().c_str(), Temp.Read(), true);

        Fill(Stream_General, 0, Ztring(StreamKind_Text+__T("Count")).To_Local().c_str(), Count_Get(KindOfStream)-1, 10, true);
    }

    //Insert a stream
    (*Stream)[KindOfStream].erase((*Stream)[KindOfStream].begin()+StreamPos);
    (*Stream_More)[KindOfStream].erase((*Stream_More)[KindOfStream].begin()+StreamPos);

    //Filling basic info
    size_t Count=Count_Get(KindOfStream);
    Stream_Count_Update(KindOfStream);
    for (size_t Pos=Count==1?0:StreamPos; Pos<Count; Pos++) //Only the shifted streams, or all streams if the position is no longer needed
    {
        Fill(KindOfStream, Pos, General_StreamKindID, Pos, 10, true);
        if (Count>1)
            Fill(KindOfStream, Pos, General_StreamKindPos, Pos+1, 10, true);
        else
            Clear(KindOfStream, Pos, General_StreamKindPos);
    }

    StreamKind_Last=Stream_Max;
    StreamPos_Last=(size_t)-1;

    return (*Stream)[KindOfStream].size()-1; //The position in the stream count
}

//---------------------------------------------------------------------------
void File__Analyze::Stream_Count_Update(stream_t KindOfStream)
{
    //Count of streams is in all streams of this kind, the value is set directly because Fill() is too slow for files with thousands of streams
    auto& Streams=(*Stream)[KindOfStream];
    if (Streams.empty())
        return;
    Ztring Count=Ztring::ToZtring(Streams.size());
    for (auto& Stream_Item : Streams)
        Stream_Item(General_StreamCount)=Count;
    Status[IsUpdated]=true;
}


//***************************************************************************
// Filling
//***************************************************************************

//---------------------------------------------------------------------------
size_t ShowSource_IsInList(video Value)
{
    size_t Offset = 0;
    switch (Value)
    {
        case Video_MaxCLL:
        case Video_MaxFALL:
            Offset++;
            [[fallthrough]];
        case Video_colour_description_present:
        case Video_colour_range:
        case Video_colour_primaries:
        case Video_matrix_coefficients:
        case Video_transfer_characteristics:
        case Video_MasteringDisplay_ColorPrimaries:
        case Video_MasteringDisplay_Luminance:
            Offset++;
            [[fallthrough]];
        default:;
    }

    return Offset;
}

//---------------------------------------------------------------------------
void File__Analyze::Fill (stream_t StreamKind, size_t StreamPos, size_t Parameter, const Ztring &Value, bool Replace)
{
    // Sanitize
    if (!Value.empty())
    {
        size_t Value_NotBOM_Pos;
        #if defined(UNICODE) || defined (_UNICODE)
            //Check inverted bytes from UTF BOM
            Value_NotBOM_Pos=Value.find_first_not_of(__T('\xFFFE')); // Avoid deep recursivity
            if (Value_NotBOM_Pos)
            {
                Ztring Value2;
                Value2.reserve(Value.size()-1);
                for (size_t i=0; i<Value.size(); i++)
                {
                    //Swap
                    Char ValueChar=Value[i];
                    ValueChar=((ValueChar<<8 & 0xFFFF) | ((ValueChar>>8) & 0xFF)); // Swap
                    Value2.append(1, ValueChar);
                }
                Value_NotBOM_Pos=Value2.find_first_not_of(__T('\xFEFF')); // Avoid deep recursivity
                if (Value_NotBOM_Pos==string::npos)
                    Value2.clear(); //Only BOMs
                else if (Value_NotBOM_Pos)
                    Value2=Value2.substr(Value_NotBOM_Pos);
                return Fill(StreamKind, StreamPos, Parameter, Value2, Replace);
            }

            Value_NotBOM_Pos=Value.find_first_not_of(__T('\xFEFF')); // Avoid deep recursivity
        #else
            Value_NotBOM_Pos=0;
            while (Value.size()-Value_NotBOM_Pos>=3 // Avoid deep recursivity
             && (unsigned char)Value[Value_NotBOM_Pos  ]==0xEF
             && (unsigned char)Value[Value_NotBOM_Pos+1]==0xBB
             && (unsigned char)Value[Value_NotBOM_Pos+2]==0xBF
                )
                Value_NotBOM_Pos+=3;
        #endif //defined(UNICODE) || defined (_UNICODE)

        if (Value_NotBOM_Pos==string::npos)
            return Fill(StreamKind, StreamPos, Parameter, Ztring(), Replace); //Only BOMs
        if (Value_NotBOM_Pos)
            return Fill(StreamKind, StreamPos, Parameter, Value.substr(Value_NotBOM_Pos), Replace);
    }

    // Replace bad characters
    const auto IsBadChar = [](Char Character) {
        auto c = static_cast<typename std::make_unsigned<Char>::type>(Character);
        return
            ((c < 0x20 &&
              c != 0x09 && c != 0x0A && c != 0x0D)) // C0 Keep TAB, LF, CR
            || (c >= 0x7F && c < 0xA0);             // DEL + C1
    };
    auto it = std::find_if(Value.begin(), Value.end(), IsBadChar);
    if (it != Value.end()) {
        Ztring Cleaned;
        Cleaned.reserve(Value.size());
        Cleaned.append(Value.begin(), it);
        for (auto i = it; i != Value.end(); ++i) {
            Cleaned += IsBadChar(*i) ?
            #if defined(UNICODE) || defined (_UNICODE)
                L'\uFFFD'
            #else
                '?'
            #endif
                : *i;
        }
        return Fill(StreamKind, StreamPos, Parameter, Cleaned, Replace);
    }

    // Ignore useless values
    if (!Value.compare(__T("N/A"))) {
        return;
    }

    // Analysis of some metadata
    if (StreamKind==Stream_General)
    {
        switch (Parameter)
        {
            case General_Description:
                if (Value.size()==38 && Value.rfind(__T("ISAN "), 0)==0)
                    return Fill(Stream_General, StreamPos, General_ISAN, Value.substr(5), Replace);
                break;
            case General_Duration_Start:
            case General_Duration_End:
            case General_Released_Date :
            case General_Original_Released_Date :
            case General_Recorded_Date :
            case General_Encoded_Date :
            case General_Tagged_Date :
            case General_Written_Date :
            case General_Mastered_Date:
            case General_Encoded_Library_Date :
            case General_File_Created_Date:
            case General_File_Created_Date_Local:
            case General_File_Modified_Date :
            case General_File_Modified_Date_Local:
            case General_Added_Date:
            case General_Played_First_Date:
            case General_Played_Last_Date:
            {
                    string Value2(Value.To_UTF8());
                    if (DateTime_Adapt(Value2))
                    {
                        if (Value2.empty())
                            return;
                        return Fill(Stream_General, StreamPos, Parameter, Value2, true, Replace);
                    }
                }
                break;
            case General_Recorded_Location:
                {
                string Value2(Value.To_UTF8());
                if (Location_Adapt(Value2)) {
                    if (Value2.empty()) {
                        return;
                    }
                    string Target = Retrieve_Const(Stream_General, StreamPos, Parameter).To_UTF8();
                    if (Location_Compare(Target, Value2, Replace)) {
                        return;
                    }
                    return Fill(Stream_General, StreamPos, Parameter, Value2, true, Replace);
                }
                }
                break;
        }
    }

    //MergedStreams
    if (FillAllMergedStreams)
    {
        for (size_t i=0; sizeof(generic); i++)
            if (Fill_Parameter(StreamKind, (generic)i)==Parameter)
            {
                generic Parameter2=(generic)i;
                FillAllMergedStreams=false;
                Ztring ID=Retrieve(StreamKind, StreamPos, General_ID); //TODO: find a better to catch content from same stream
                auto ID_Dash_Pos=ID.find(__T("-"));
                if (ID_Dash_Pos!=string::npos)
                    ID.resize(ID_Dash_Pos);
                for (size_t StreamKind2=Stream_Audio; StreamKind2<Stream_Max; StreamKind2++)
                    for (size_t StreamPos2=0; StreamPos2<(*Stream)[StreamKind2].size(); StreamPos2++)
                    {
                        Ztring ID2=Retrieve((stream_t)StreamKind2, StreamPos2, General_ID);
                        auto ID2_Dash_Pos=ID2.find(__T("-"));
                        if (ID2_Dash_Pos!=string::npos && ID2.substr(ID2_Dash_Pos)!=__T("-Material"))
                            ID2.resize(ID2_Dash_Pos);
                        if (ID2==ID)
                        {
                            size_t Parameter3=Fill_Parameter((stream_t)StreamKind2, Parameter2);
                            if (Parameter3!=(size_t)-1)
                                Fill((stream_t)StreamKind2, StreamPos2, Parameter3, Value, Replace);
                        }
                    }
                FillAllMergedStreams=true;
                break;
            }
        return;
    }

    //Integrity
    if (StreamKind>Stream_Max || Parameter==(size_t)-1)
        return;

    //Remove deprecated fields
    if (!MediaInfoLib::Config.Legacy_Get())
    {
        const Ztring& Info=MediaInfoLib::Config.Info_Get(StreamKind, Parameter, Info_Info);
        if (Info==__T("Deprecated"))
            return;
    }

    // Handling sources
    const char* SourceValue[StreamSource_Max] =
    {
        "Container",
        "Stream",
        "ContainerExtra",
    };
    assert(sizeof(SourceValue)==StreamSource_Max*sizeof(const char*));
    if (StreamKind==Stream_Video && StreamSource>=0 && StreamSource<StreamSource_Max)
    {
        auto ShowSource_Offset=ShowSource_IsInList((video)Parameter);
        if (ShowSource_Offset && Retrieve_Const(Stream_Video, StreamPos, Parameter+ShowSource_Offset).empty())
            Fill(Stream_Video, StreamPos, Parameter+ShowSource_Offset, SourceValue[StreamSource]);
    }

    //Format_Profile split (see similar code in MediaInfo_Inform.cpp, dedicated to MIXML)
    #if MEDIAINFO_ADVANCED
        if (Parameter==Fill_Parameter(StreamKind, Generic_Format_Profile) && MediaInfoLib::Config.Format_Profile_Split_Get())
        {
            size_t SeparatorPos=Value.find(__T('@'));
            if (SeparatorPos!=string::npos && Value.find(__T(" / "))==string::npos) //TODO: better support of compatibility modes (e.g. "Multiview") and sequences (e.g. different profiles in different files "BCS@L3 / BCS@L2 / BCS@L3")
            {
                Ztring Value2(Value);
                Ztring Format_Profile_More=Value2.substr(SeparatorPos+1);
                Value2.erase(SeparatorPos);
                if (Format_Profile_More.size()>=2 && Format_Profile_More[0]==__T('L') && Format_Profile_More[1]>=__T('0') && Format_Profile_More[1]<=__T('9'))
                    Format_Profile_More.erase(0, 1);
                size_t SeparatorPos=Format_Profile_More.find(__T('@'));
                Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Format_Profile), Value2, Replace);
                if (SeparatorPos!=string::npos)
                {
                    Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Format_Level), Format_Profile_More.substr(0, SeparatorPos));
                    Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Format_Tier), Format_Profile_More.substr(SeparatorPos+1));
                }
                else
                {
                    Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Format_Level), Format_Profile_More);
                }
                return;
            }
        }
    #endif //MEDIAINFO_ADVANCED

    //Handling values with \r\n inside
    if (Value.find_first_of(__T("\r\n"))!=string::npos)
    {
        Ztring CarriageReturnReplace=MediaInfoLib::Config.CarriageReturnReplace_Get();
        if (!CarriageReturnReplace.empty())
        {
            Ztring NewValue=Value;
            NewValue.FindAndReplace(__T("\r\n"), CarriageReturnReplace, 0, Ztring_Recursive);
            NewValue.FindAndReplace(__T("\r"), CarriageReturnReplace, 0, Ztring_Recursive);
            NewValue.FindAndReplace(__T("\n"), CarriageReturnReplace, 0, Ztring_Recursive);
            if (NewValue.size()>=CarriageReturnReplace.size() && NewValue.rfind(CarriageReturnReplace)==NewValue.size()-CarriageReturnReplace.size())
                NewValue.resize(NewValue.size()-CarriageReturnReplace.size());
            Fill(StreamKind, StreamPos, Parameter, NewValue, Replace);
            return;
        }
    }

    //Handle Value before StreamKind
    if (StreamKind==Stream_Max || StreamPos>=(*Stream)[StreamKind].size())
    {
        size_t StreamKindS=(size_t)StreamKind;
        if (StreamKind>=0 && StreamKind!=Stream_Max)
        {
            //Stream kind is found, moving content
            for (size_t Pos=0; Pos<Fill_Temp[Stream_Max].size(); Pos++)
                Fill_Temp[StreamKind].push_back(Fill_Temp[Stream_Max][Pos]);
            Fill_Temp[Stream_Max].clear();
        }
        else
        {
            //Stream kind is not found, checking if it was found previously
            for (StreamKindS=(size_t)Stream_General+1; StreamKindS<(size_t)Stream_Max; StreamKindS++)
                if (!Fill_Temp[StreamKindS].empty())
                    break;
        }

        const Ztring Parameter_String = Ztring::ToZtring(Parameter);
        if (Replace)
        {
            for (size_t Pos=0; Pos<Fill_Temp[StreamKindS].size(); Pos++)
                if (Fill_Temp[StreamKindS][Pos].Parameter==Parameter_String)
                {
                    Fill_Temp[StreamKindS][Pos].Value=Value;
                    return;
                }
        }
        fill_temp_item NewList;
        NewList.Parameter=Parameter_String;
        NewList.Value=Value;
        Fill_Temp[StreamKindS].push_back(NewList);
        return; //No streams
    }

    //Some defaults
    if (Parameter==Fill_Parameter(StreamKind, Generic_Format_Commercial))
        Replace=true;
    if (Parameter==Fill_Parameter(StreamKind, Generic_Format_Commercial_IfAny))
        Replace=true;

    if (!Replace && Value.empty())
        return;
    if (Replace && Value.empty())
    {
        Clear(StreamKind, StreamPos, Parameter);
        return;
    }

    auto StreamKind_MaxParameterPos=MediaInfoLib::Config.Info_Get(StreamKind).size();
    Ztring &Target=Parameter>StreamKind_MaxParameterPos?(*Stream_More)[StreamKind][StreamPos](Parameter-StreamKind_MaxParameterPos)(Info_Text):(*Stream)[StreamKind][StreamPos](Parameter);
    bool Compare=false;
    switch (StreamKind)
    {
    case Stream_General:
        switch (Parameter)
        {
        case General_Title:
        case General_Encoded_Application:
        case General_Encoded_Application_CompanyName:
        case General_Encoded_Application_Name:
        case General_Encoded_Application_Version:
        case General_Encoded_Library:
        case General_Encoded_Library_CompanyName:
        case General_Encoded_Library_Name:
        case General_Encoded_Library_Version:
        case General_Encoded_OperatingSystem:
        case General_Encoded_OperatingSystem_CompanyName:
        case General_Encoded_OperatingSystem_Name:
        case General_Encoded_OperatingSystem_Version:
        case General_Encoded_Hardware:
        case General_Encoded_Hardware_CompanyName:
        case General_Encoded_Hardware_Name:
        case General_Encoded_Hardware_Model:
        case General_Encoded_Hardware_Version:
        case General_Copyright:
        case General_Comment:
        case General_Description:
            Compare = true;
        }
        break;
    case Stream_Video:
        switch (Parameter)
        {
        case Video_Title:
            Compare = true;
        }
        break;
    case Stream_Audio:
        switch (Parameter)
        {
        case Audio_Title:
            Compare = true;
        }
        break;
    case Stream_Text:
        switch (Parameter)
        {
        case Text_Title:
            Compare = true;
        }
        break;
    case Stream_Image:
        switch (Parameter)
        {
        case Image_Title:
            Compare = true;
        }
        break;
    case Stream_Other:
        switch (Parameter)
        {
        case Other_Title:
            Compare = true;
        }
        break;
    default:
        break;
    }
    if (Compare)
    {
        Ztring Value2(Value);
        Value2.Trim(__T('\n'));
        Value2.Trim(__T('\r'));
        Value2.Trim(__T('\v'));
        Value2.Trim(__T('\t'));
        Value2.Trim(__T(' '));
        if (Value2.empty())
            return;
        if (Value2.size() != Value.size()) {
            return Fill(StreamKind, StreamPos, Parameter, Value2, Replace);
        }
        if (!Target.empty() && Target.size() < Value.size() && Value.rfind(Target, 0) != string::npos)
            Replace = true;
        else
            Compare = Target.size() >= Value.size() && !Target.rfind(Value, 0);
    }
    if (Target.empty() || Replace)
        Target=Value; //First value
    else if (!Compare)
    {
        Target+=MediaInfoLib::Config.TagSeparator_Get();
        Target+=Value;
    }
    Status[IsUpdated]=true;

    //Deprecated
    if (Parameter==Fill_Parameter(StreamKind, Generic_BitDepth))
        Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Resolution), Retrieve(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_BitDepth)), true);
    if (StreamKind==Stream_Video && Parameter==Video_ChromaSubsampling)
        Fill(Stream_Video, StreamPos, Video_Colorimetry, Value, Replace);

    switch (StreamKind)
    {
        case Stream_Video:
                            switch (Parameter)
                            {
                                case Video_Width:   if (StreamSource==IsStream) Fill(Stream_Video, StreamPos, Video_Sampled_Width, Value); break;
                                case Video_Height:  if (StreamSource==IsStream) Fill(Stream_Video, StreamPos, Video_Sampled_Height, Value); break;
                                case Video_DisplayAspectRatio:  DisplayAspectRatio_Fill(Value, Stream_Video, StreamPos, Video_Width, Video_Height, Video_PixelAspectRatio, Video_DisplayAspectRatio); break;
                                case Video_Active_DisplayAspectRatio:  DisplayAspectRatio_Fill(Value, Stream_Video, StreamPos, Video_Active_Width, Video_Active_Height, Video_PixelAspectRatio, Video_Active_DisplayAspectRatio); break;
                                case Video_PixelAspectRatio:    PixelAspectRatio_Fill(Value, Stream_Video, StreamPos, Video_Width, Video_Height, Video_PixelAspectRatio, Video_DisplayAspectRatio);   break;
                                case Video_DisplayAspectRatio_CleanAperture:  DisplayAspectRatio_Fill(Value, Stream_Video, StreamPos, Video_Width_CleanAperture, Video_Height_CleanAperture, Video_PixelAspectRatio_CleanAperture, Video_DisplayAspectRatio_CleanAperture); break;
                                case Video_PixelAspectRatio_CleanAperture:    PixelAspectRatio_Fill(Value, Stream_Video, StreamPos, Video_Width_CleanAperture, Video_Height_CleanAperture, Video_PixelAspectRatio_CleanAperture, Video_DisplayAspectRatio_CleanAperture);   break;
                                case Video_DisplayAspectRatio_Original:  DisplayAspectRatio_Fill(Value, Stream_Video, StreamPos, Video_Width_Original, Video_Height_Original, Video_PixelAspectRatio_Original, Video_DisplayAspectRatio_Original); break;
                                case Video_PixelAspectRatio_Original:    PixelAspectRatio_Fill(Value, Stream_Video, StreamPos, Video_Width_Original, Video_Height_Original, Video_PixelAspectRatio_Original, Video_DisplayAspectRatio_Original);   break;
                            }
                            break;
        case Stream_Audio:
                            switch (Parameter)
                            {
                                case Audio_SamplesPerFrame:
                                case Audio_SamplingRate:
                                    if (Retrieve(Stream_Audio, StreamPos, Audio_FrameRate).empty())
                                    {
                                        float64 SamplesPerFrame=DBL_MAX;
                                        ZtringList SamplesPerFrames;
                                        SamplesPerFrames.Separator_Set(0, " / ");
                                        SamplesPerFrames.Write(Retrieve(Stream_Audio, StreamPos, Audio_SamplesPerFrame));
                                        if (!SamplesPerFrames.empty())
                                        {
                                            size_t i=SamplesPerFrames.size();
                                            do
                                            {
                                                --i;
                                                float64 SamplesPerFrameTemp = SamplesPerFrames[i].To_float64();
                                                if (SamplesPerFrameTemp && SamplesPerFrameTemp<SamplesPerFrame)
                                                    SamplesPerFrame=SamplesPerFrameTemp; // Using the lowest valid one (e.g. AAC doubles sampling rate but the legacy sampling rate is the real frame)
                                            }
                                            while (i);
                                        }
                                        float64 SamplingRate=DBL_MAX;
                                        ZtringList SamplingRates;
                                        SamplingRates.Separator_Set(0, " / ");
                                        SamplingRates.Write(Retrieve(Stream_Audio, StreamPos, Audio_SamplingRate));
                                        if (!SamplingRates.empty())
                                        {
                                            size_t i=SamplingRates.size();
                                            do
                                            {
                                                --i;
                                                float64 SamplingRateTemp = SamplingRates[i].To_float64();
                                                if (SamplingRateTemp && SamplingRateTemp<SamplingRate)
                                                    SamplingRate=SamplingRateTemp; // Using the lowest valid one (e.g. AAC doubles sampling rate but the legacy sampling rate is the real frame)
                                            }
                                            while (i);
                                        }
                                        if (SamplesPerFrame && SamplesPerFrame!=DBL_MAX && SamplingRate && SamplingRate!=DBL_MAX && SamplesPerFrame!=SamplingRate)
                                        {
                                            float64 FrameRate=SamplingRate/SamplesPerFrame;
                                            Fill(Stream_Audio, StreamPos, Audio_FrameRate, FrameRate);
                                        }
                                    }
                            }
                            break;
        case Stream_Text:
                            switch (Parameter)
                            {
                                case Text_DisplayAspectRatio:  DisplayAspectRatio_Fill(Value, Stream_Text, StreamPos, Text_Width, Text_Height, -1, Text_DisplayAspectRatio); break;
                                case Text_DisplayAspectRatio_Original:  DisplayAspectRatio_Fill(Value, Stream_Text, StreamPos, -1, -1, -1, Text_DisplayAspectRatio_Original); break;
                            }
                            break;
        case Stream_Image:
                            switch (Parameter)
                            {
                                case Image_DisplayAspectRatio:  DisplayAspectRatio_Fill(Value, Stream_Image, StreamPos, Image_Width, Image_Height, Image_PixelAspectRatio, Image_DisplayAspectRatio); break;
                                case Image_Active_DisplayAspectRatio:  DisplayAspectRatio_Fill(Value, Stream_Image, StreamPos, Image_Active_Width, Image_Active_Height, Image_PixelAspectRatio, Image_Active_DisplayAspectRatio); break;
                                case Image_PixelAspectRatio:    PixelAspectRatio_Fill(Value, Stream_Image, StreamPos, Image_Width, Image_Height, Image_PixelAspectRatio, Image_DisplayAspectRatio);   break;
                                case Image_DisplayAspectRatio_Original:  DisplayAspectRatio_Fill(Value, Stream_Image, StreamPos, Image_Width_Original, Image_Height_Original, Image_PixelAspectRatio_Original, Image_DisplayAspectRatio_Original); break;
                                case Image_PixelAspectRatio_Original:    PixelAspectRatio_Fill(Value, Stream_Image, StreamPos, Image_Width_Original, Image_Height_Original, Image_PixelAspectRatio_Original, Image_DisplayAspectRatio_Original);   break;
                            }
                            break;
        default:
                            // TODO;
                            break;
    }

    //Commercial name
    if (Parameter==Fill_Parameter(StreamKind, Generic_Format))
        Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Format_Commercial), Value);
    if (Parameter==Fill_Parameter(StreamKind, Generic_Format_Commercial_IfAny))
        Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Format_Commercial), Value, true);

    if (Parameter==Fill_Parameter(StreamKind, Generic_TimeCode_FirstFrame) && Value.size()>=11)
    {
        Char C=Value[8];
        switch (C)
        {
            case __T(':'):
            case __T(';'):
                Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_TimeCode_DropFrame), (C==__T(';'))?__T("Yes"):__T("No"), Unlimited, Replace);
                break;
        }
    }

    if (!IsSub)
    {
        Ztring ParameterName=Retrieve(StreamKind, StreamPos, Parameter, Info_Name);

        //General Format
        if (Parameter==Fill_Parameter(StreamKind, Generic_Format) && Retrieve(Stream_General, 0, General_Format).empty() && !Value.empty() && Count_Get(Stream_Video)+Count_Get(Stream_Audio)+Count_Get(Stream_Text)+Count_Get(Stream_Other)+Count_Get(Stream_Image)==1)
            Fill(Stream_General, 0, General_Format, Value); //If not already filled, we are filling with the stream format

        //ID
        if (Parameter==General_ID)
            Fill(StreamKind, StreamPos, General_ID_String, Value, Replace);

        //Format
        if (Parameter==Fill_Parameter(StreamKind, Generic_Format))
        {
            if ((Replace && !MediaInfoLib::Config.Format_Get(Retrieve(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Format)), InfoFormat_Info).empty()) || Retrieve(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Format_Info)).empty())
                Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Format_Info), MediaInfoLib::Config.Format_Get(Retrieve(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Format)), InfoFormat_Info), true);
            if ((Replace && !MediaInfoLib::Config.Format_Get(Retrieve(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Format)), InfoFormat_Url).empty()) || Retrieve(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Format_Url)).empty())
                Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Format_Url) , MediaInfoLib::Config.Format_Get(Retrieve(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Format)), InfoFormat_Url), true);
            if (StreamKind!=Stream_Menu)
            {
                if ((Replace && !MediaInfoLib::Config.Format_Get(Retrieve(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Format)), InfoFormat_InternetMediaType).empty()) || Retrieve(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_InternetMediaType)).empty())
                    Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_InternetMediaType), MediaInfoLib::Config.Format_Get(Retrieve(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Format)), InfoFormat_InternetMediaType), true);
                if ((Replace && !MediaInfoLib::Config.Format_Get(Retrieve(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Format)), InfoFormat_Compression_Mode).empty()) || Retrieve(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Compression_Mode)).empty())
                    Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Compression_Mode), MediaInfoLib::Config.Format_Get(Retrieve(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Format)), InfoFormat_Compression_Mode), true);
            }
            if (StreamKind==Stream_General)
            {
                Fill(Stream_General, 0, General_Format_Extensions, MediaInfoLib::Config.Format_Get(Value, InfoFormat_Extensions), true);
                Fill(Stream_General, 0, General_Format_String, Value, true);
                Fill(Stream_General, 0, General_Codec, Value, true);
                Fill(Stream_General, 0, General_Codec_String, Value, true);
            }
        }
        if (MediaInfoLib::Config.Legacy_Get())
        {
        if (StreamKind==Stream_General && Parameter==General_Format_Info)
            (*Stream)[Stream_General][0](General_Codec_Info)=Value;
        if (StreamKind==Stream_General && Parameter==General_Format_Url)
            (*Stream)[Stream_General][0](General_Codec_Url)=Value;
        if (StreamKind==Stream_General && Parameter==General_Format_Extensions)
            (*Stream)[Stream_General][0](General_Codec_Extensions)=Value;
        if (StreamKind==Stream_General && Parameter==General_Format_Settings)
            (*Stream)[Stream_General][0](General_Codec_Settings)=Value;

        //Codec
        if (Parameter==Fill_Parameter(StreamKind, Generic_Codec))
        {
            const Ztring &C1=MediaInfoLib::Config.Codec_Get(Retrieve(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Codec)), InfoCodec_Name, (stream_t)StreamKind);
            if (C1.empty())
                Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Codec_String), Value, true);
            else
            {
                Ztring D=Retrieve(StreamKind, StreamPos, "Codec/Family");
                Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Codec_String), C1, true);
                Fill(StreamKind, StreamPos, "Codec/Family", MediaInfoLib::Config.Codec_Get(Retrieve(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Codec)), InfoCodec_KindofCodec, StreamKind), true);
                Ztring B=Retrieve(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Codec));
                Ztring C=MediaInfoLib::Config.Codec_Get(B, InfoCodec_KindofCodec, StreamKind);
                Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Codec_Info)  , MediaInfoLib::Config.Codec_Get(Retrieve(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Codec)), InfoCodec_Description, StreamKind), true);
                Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Codec_Url)   , MediaInfoLib::Config.Codec_Get(Retrieve(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Codec)), InfoCodec_Url,         StreamKind), true);
            }
        }
        }

        //CodecID_Description
        if (Parameter==Fill_Parameter(StreamKind, Generic_CodecID_Info) && Retrieve(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_CodecID_Description))==Value)
            Clear(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_CodecID_Description));

        //BitRate from BitRate_Nominal
        if ((Parameter==Fill_Parameter(StreamKind, Generic_BitRate)
          || Parameter==Fill_Parameter(StreamKind, Generic_BitRate_Nominal))
        #if MEDIAINFO_ADVANCED
         && Config->File_MergeBitRateInfo_Get()
        #endif //MEDIAINFO_ADVANCED
         )
        {
            float32 BitRate=Retrieve(StreamKind, StreamPos, "BitRate").To_float32();
            float32 BitRate_Nominal=Retrieve(StreamKind, StreamPos, "BitRate_Nominal").To_float32();
            if (BitRate_Nominal>BitRate*0.95 && BitRate_Nominal<BitRate*1.05)
            {
                Ztring Temp=Retrieve(StreamKind, StreamPos, "BitRate_Nominal");
                Clear(StreamKind, StreamPos, "BitRate_Nominal");
                Fill(StreamKind, StreamPos, "BitRate", Temp, true);
            }
        }

        //BitRate from BitRate_Maximum
        if ((Parameter==Fill_Parameter(StreamKind, Generic_BitRate)
          || Parameter==Fill_Parameter(StreamKind, Generic_BitRate_Maximum))
        #if MEDIAINFO_ADVANCED
         && Config->File_MergeBitRateInfo_Get()
        #endif //MEDIAINFO_ADVANCED
        )
        {
            float32 BitRate=Retrieve(StreamKind, StreamPos, "BitRate").To_float32();
            float32 BitRate_Maximum=Retrieve(StreamKind, StreamPos, "BitRate_Maximum").To_float32();
            if (BitRate>BitRate_Maximum*0.99 && BitRate<BitRate_Maximum*1.01)
            {
                Ztring Temp=Retrieve(StreamKind, StreamPos, "BitRate_Maximum");
                Clear(StreamKind, StreamPos, "BitRate_Maximum");
                Fill(StreamKind, StreamPos, "BitRate", Temp, true);
            }
        }

        //File size
        if (StreamKind==Stream_General && Parameter==General_FileSize)
        {
            int64u File_Size_Save=File_Size;
            File_Size=Value.To_int64u();
            for (size_t Kind=Stream_Video; Kind<Stream_Menu; Kind++)
                for (size_t Pos=0; Pos<Count_Get((stream_t)Kind); Pos++)
                    FileSize_FileSize123((stream_t)Kind, Pos, Fill_Parameter((stream_t)Kind, Generic_StreamSize));
            File_Size=File_Size_Save;
        }

        //Delay/Video
        if (StreamKind==Stream_Video && StreamPos==0 && Parameter==Video_Delay)
        {
            for (size_t Pos=0; Pos<Count_Get(Stream_Audio); Pos++)
                if (!Retrieve(Stream_Audio, Pos, Audio_Delay).empty())
                {
                    Fill(Stream_Audio, Pos, Audio_Video_Delay, Retrieve(Stream_Audio, Pos, Audio_Delay).To_int64s()-Value.To_int64s(), 10, true);
                    if (Retrieve(Stream_Audio, Pos, Audio_Video_Delay).To_int64u()==0)
                        for (size_t Param_Pos=Audio_Video_Delay+1; Param_Pos<=Audio_Video_Delay+4; Param_Pos++)
                            if (Param_Pos<(*Stream)[Stream_Audio][Pos].size())
                                (*Stream)[Stream_Audio][Pos][Param_Pos].clear();
                }
            for (size_t Pos=0; Pos<Count_Get(Stream_Text); Pos++)
                if (!Retrieve(Stream_Text, Pos, Text_Delay).empty())
                {
                    Fill(Stream_Text, Pos, Text_Video_Delay, Retrieve(Stream_Text, Pos, Text_Delay).To_int64s()-Value.To_int64s(), 10, true);
                    if (Retrieve(Stream_Text, Pos, Text_Video_Delay).To_int64u()==0)
                        for (size_t Param_Pos=Text_Video_Delay+1; Param_Pos<=Text_Video_Delay+4; Param_Pos++)
                            if (Param_Pos<(*Stream)[Stream_Text][Pos].size())
                                (*Stream)[Stream_Text][Pos][Param_Pos].clear();
                }
        }
        if (StreamKind==Stream_Audio && Parameter==Audio_Delay && Count_Get(Stream_Video) && !Retrieve(Stream_Audio, StreamPos, Audio_Delay).empty() && !Retrieve(Stream_Video, 0, Video_Delay).empty())
        {
            if (Replace)
                Clear(Stream_Audio, StreamPos, Audio_Video_Delay);
            ZtringList AudioDelay; AudioDelay.Separator_Set(0, __T(" / ")); AudioDelay.Write(Retrieve(Stream_Audio, StreamPos, Audio_Delay));
            ZtringList VideoDelay; VideoDelay.Separator_Set(0, __T(" / ")); VideoDelay.Write(Retrieve(Stream_Video, 0, Video_Delay));
            if (!AudioDelay.empty() && !VideoDelay.empty() && AudioDelay.size() <= VideoDelay.size())
            {
                Fill(Stream_Audio, StreamPos, Audio_Video_Delay, AudioDelay(AudioDelay.size()-1).To_int64s()-VideoDelay(VideoDelay.size()-1).To_int64s(), 10);
                if (VideoDelay.size()==1 && Retrieve(Stream_Audio, StreamPos, Audio_Video_Delay).To_int64u()==0)
                    for (size_t Pos=Audio_Video_Delay+1; Pos<=Audio_Video_Delay+4; Pos++)
                        if (Pos<(*Stream)[Stream_Audio][StreamPos].size())
                            (*Stream)[Stream_Audio][StreamPos][Pos].clear();
            }
        }
        if (StreamKind==Stream_Text && Parameter==Text_Delay && Count_Get(Stream_Video) && !Retrieve(Stream_Text, StreamPos, Text_Delay).empty() && !Retrieve(Stream_Video, 0, Video_Delay).empty())
        {
            Ztring MuxingMode_MoreInfo=Get(Stream_Text, StreamPos, "MuxingMode_MoreInfo");
            Ztring StreamID=MuxingMode_MoreInfo.SubString(__T("Muxed in Video #"), Ztring());
            size_t StreamID_Int=(size_t)StreamID.To_int64u();
            if (StreamID_Int)
                StreamID_Int--;
            Fill(Stream_Text, StreamPos, Text_Video_Delay, Value.To_int64s()-Retrieve(Stream_Video, StreamID_Int, Video_Delay).To_int64s(), 10, true);
            if (Retrieve(Stream_Text, StreamPos, Text_Video_Delay).To_int64u()==0)
                for (size_t Pos=Text_Video_Delay+1; Pos<=Text_Video_Delay+4; Pos++)
                    if (Pos<(*Stream)[Stream_Text][StreamPos].size())
                        (*Stream)[Stream_Text][StreamPos][Pos].clear();
        }
        if (StreamKind==Stream_Other && Parameter==Other_Delay && Count_Get(Stream_Video) && !Retrieve(Stream_Other, StreamPos, Other_Delay).empty() && !Retrieve(Stream_Video, 0, Video_Delay).empty())
        {
            if (Replace)
                Clear(Stream_Other, StreamPos, Other_Video_Delay);
            ZtringList OtherDelay; OtherDelay.Separator_Set(0, __T(" / ")); OtherDelay.Write(Retrieve(Stream_Other, StreamPos, Other_Delay));
            ZtringList VideoDelay; VideoDelay.Separator_Set(0, __T(" / ")); VideoDelay.Write(Retrieve(Stream_Video, 0, Video_Delay));
            if (!OtherDelay.empty() && !VideoDelay.empty() && OtherDelay.size() <= VideoDelay.size())
            {
                Fill(Stream_Other, StreamPos, Other_Video_Delay, OtherDelay(OtherDelay.size()-1).To_int64s()-VideoDelay(VideoDelay.size()-1).To_int64s(), 10);
                if (VideoDelay.size()==1 && Retrieve(Stream_Other, StreamPos, Other_Video_Delay).To_int64u()==0)
                    for (size_t Pos=Other_Video_Delay+1; Pos<=Other_Video_Delay+4; Pos++)
                        if (Pos<(*Stream)[Stream_Other][StreamPos].size())
                            (*Stream)[Stream_Other][StreamPos][Pos].clear();
            }
        }

        //Delay/Video0
        if (StreamKind==Stream_Video && StreamPos==0 && Parameter==Video_Delay)
        {
            for (size_t Pos=0; Pos<Count_Get(Stream_Audio); Pos++)
                if (!Retrieve(Stream_Audio, Pos, Audio_Delay).empty())
                {
                    Fill(Stream_Audio, Pos, Audio_Video0_Delay, Retrieve(Stream_Audio, Pos, Audio_Delay).To_int64s()-Value.To_int64s(), 10, true);
                    if (Retrieve(Stream_Audio, Pos, Audio_Video0_Delay).To_int64u()==0)
                        for (size_t Param_Pos=Audio_Video0_Delay+1; Param_Pos<=Audio_Video0_Delay+4; Param_Pos++)
                            if (Param_Pos<(*Stream)[Stream_Audio][Pos].size())
                                (*Stream)[Stream_Audio][Pos][Param_Pos].clear();
                }
            for (size_t Pos=0; Pos<Count_Get(Stream_Text); Pos++)
                if (!Retrieve(Stream_Text, Pos, Text_Delay).empty())
                {
                    Fill(Stream_Text, Pos, Text_Video0_Delay, Retrieve(Stream_Text, Pos, Text_Delay).To_int64s()-Value.To_int64s(), 10, true);
                    if (Retrieve(Stream_Text, Pos, Text_Video0_Delay).To_int64u()==0)
                        for (size_t Param_Pos=Text_Video0_Delay+1; Param_Pos<=Text_Video0_Delay+4; Param_Pos++)
                            if (Param_Pos<(*Stream)[Stream_Text][Pos].size())
                                (*Stream)[Stream_Text][Pos][Param_Pos].clear();
                }
        }
        if (StreamKind==Stream_Audio && Parameter==Audio_Delay && Count_Get(Stream_Video) && !Retrieve(Stream_Audio, StreamPos, Audio_Delay).empty() && !Retrieve(Stream_Video, 0, Video_Delay).empty())
        {
            if (Replace)
                Clear(Stream_Audio, StreamPos, Audio_Video0_Delay);
            ZtringList AudioDelay; AudioDelay.Separator_Set(0, __T(" / ")); AudioDelay.Write(Retrieve(Stream_Audio, StreamPos, Audio_Delay));
            ZtringList VideoDelay; VideoDelay.Separator_Set(0, __T(" / ")); VideoDelay.Write(Retrieve(Stream_Video, 0, Video_Delay));
            if (!AudioDelay.empty() && !VideoDelay.empty() && AudioDelay.size() <= VideoDelay.size())
            {
                Fill(Stream_Audio, StreamPos, Audio_Video0_Delay, AudioDelay(AudioDelay.size() - 1).To_int64s() - VideoDelay(VideoDelay.size() - 1).To_int64s(), 10);
                if (VideoDelay.size()==1 && Retrieve(Stream_Audio, StreamPos, Audio_Video0_Delay).To_int64u()==0)
                    for (size_t Pos=Audio_Video0_Delay+1; Pos<=Audio_Video0_Delay+4; Pos++)
                        if (Pos<(*Stream)[Stream_Audio][StreamPos].size())
                            (*Stream)[Stream_Audio][StreamPos][Pos].clear();
            }
        }
        if (StreamKind==Stream_Text && Parameter==Text_Delay && Count_Get(Stream_Video) && !Retrieve(Stream_Text, StreamPos, Text_Delay).empty() && !Retrieve(Stream_Video, 0, Video_Delay).empty())
        {
            Ztring MuxingMode_MoreInfo=Get(Stream_Text, StreamPos, "MuxingMode_MoreInfo");
            Ztring StreamID=MuxingMode_MoreInfo.SubString(__T("Muxed in Video #"), Ztring());
            size_t StreamID_Int=(size_t)StreamID.To_int64u();
            if (StreamID_Int)
                StreamID_Int--;
            Fill(Stream_Text, StreamPos, Text_Video0_Delay, Value.To_int64s()-Retrieve(Stream_Video, StreamID_Int, Video_Delay).To_int64s(), 10, true);
            if (Retrieve(Stream_Text, StreamPos, Text_Video0_Delay).To_int64u()==0)
                for (size_t Pos=Text_Video0_Delay+1; Pos<=Text_Video0_Delay+4; Pos++)
                    if (Pos<(*Stream)[Stream_Text][StreamPos].size())
                        (*Stream)[Stream_Text][StreamPos][Pos].clear();
        }
        if (StreamKind==Stream_Other && Parameter==Text_Delay && Count_Get(Stream_Video) && !Retrieve(Stream_Other, StreamPos, Text_Delay).empty() && !Retrieve(Stream_Video, 0, Video_Delay).empty())
        {
            Ztring MuxingMode_MoreInfo=Get(Stream_Other, StreamPos, "MuxingMode_MoreInfo");
            Ztring StreamID=MuxingMode_MoreInfo.SubString(__T("Muxed in Video #"), Ztring());
            size_t StreamID_Int=(size_t)StreamID.To_int64u();
            if (StreamID_Int)
                StreamID_Int--;
            Fill(Stream_Other, StreamPos, Text_Video0_Delay, Value.To_int64s()-Retrieve(Stream_Video, StreamID_Int, Video_Delay).To_int64s(), 10, true);
            if (Retrieve(Stream_Other, StreamPos, Text_Video0_Delay).To_int64u()==0)
                for (size_t Pos=Text_Video0_Delay+1; Pos<=Text_Video0_Delay+4; Pos++)
                    if (Pos<(*Stream)[Stream_Other][StreamPos].size())
                        (*Stream)[Stream_Other][StreamPos][Pos].clear();
        }

        //Language
        //-Find 2-digit language
        if (Parameter==Fill_Parameter(StreamKind, Generic_Language))
        {
            //Removing old strings
            Clear(StreamKind, StreamPos, Parameter+1); //String
            Clear(StreamKind, StreamPos, Parameter+2); //String1
            Clear(StreamKind, StreamPos, Parameter+3); //String2
            Clear(StreamKind, StreamPos, Parameter+4); //String3
            Clear(StreamKind, StreamPos, Parameter+5); //String4

            ZtringListList Languages;
            Languages.Separator_Set(0, __T(" / "));
            Languages.Separator_Set(1, __T("-"));
            Languages.Write((*Stream)[StreamKind][StreamPos][Parameter]);

            //Canonizing
            for (size_t Pos=0; Pos<Languages.size(); Pos++)
            {
                Ztring Language_Orig;

                //Removing undefined languages
                if (Languages[Pos].size()>=1)
                {
                    Language_Orig=Languages[Pos][0];
                    Languages[Pos][0].MakeLowerCase();
                    if ((Languages[Pos][0].size()==3 && (Languages[Pos][0]==__T("mis")
                                                      || Languages[Pos][0]==__T("und")
                                                      || Languages[Pos][0]==__T("???")
                                                      || Languages[Pos][0]==__T("   ")))
                     || (Languages[Pos][0].size()==2 && Languages[Pos][0]==__T("  ")))
                        Languages[Pos].clear();
                }

                //Finding ISO-639-1 from ISO-639-2 or translated name
                if (Languages[Pos].size()>=1)
                {
                    if (Languages[Pos][0].size()==3 && !MediaInfoLib::Config.Iso639_1_Get(Languages[Pos][0]).empty())
                        Languages[Pos][0]=MediaInfoLib::Config.Iso639_1_Get(Languages[Pos][0]);
                    if (Languages[Pos][0].size()>3 && !MediaInfoLib::Config.Iso639_Find(Languages[Pos][0]).empty())
                        Languages[Pos][0]=MediaInfoLib::Config.Iso639_Find(Languages[Pos][0]);
                    if (Languages[Pos][0].size()>3)
                        Languages[Pos][0]=Language_Orig; //We failed to detect language, using the original version
                }
            }

            if (Languages.Read()!=Value && Languages.Read()!=Retrieve(StreamKind, StreamPos, Parameter))
                Fill(StreamKind, StreamPos, Parameter, Languages.Read(), true);
            else
            {
                ZtringList Language1; Language1.Separator_Set(0, __T(" / "));
                ZtringList Language2; Language2.Separator_Set(0, __T(" / "));
                ZtringList Language3; Language3.Separator_Set(0, __T(" / "));
                ZtringList Language4; Language4.Separator_Set(0, __T(" / "));

                for (size_t Pos=0; Pos<Languages.size(); Pos++)
                {
                    if (Languages[Pos].size()>=1)
                    {
                        Ztring Language_Translated;
                        if (Languages[Pos].size()==2)
                            Language_Translated=MediaInfoLib::Config.Language_Get(__T("Language_")+Languages[Pos].Read()); //Testing in case the langauge file has the complex form
                        if (Language_Translated.find(__T("Language_"))==0)
                            Language_Translated.clear(); //No translation found
                        if (Language_Translated.empty())
                        {
                            const auto& LanguageSplit=Languages[Pos];
                            const auto Language=LanguageSplit.Read();
                            for (const auto C : Language)
                                if (!isalnum(C) && C!='-')
                                {
                                    Language_Translated=Language;
                                    break;
                                }
                        }
                        if (Language_Translated.empty())
                        {
                        Language_Translated=MediaInfoLib::Config.Language_Get(__T("Language_")+Languages[Pos][0]);
                        if (Language_Translated.find(__T("Language_"))==0)
                            Language_Translated=Languages[Pos][0]; //No translation found
                        if (Languages[Pos].size()>=2)
                        {
                            if (Languages[Pos].size()==2)
                            {
                                Language_Translated+=__T(" (");
                                if (Languages[Pos][1].size()==2 && Languages[Pos][1][0]>=__T('a') && Languages[Pos][1][0]<=__T('z') && Languages[Pos][1][1]>=__T('a') && Languages[Pos][1][1]<=__T('z'))
                                    Language_Translated+=Ztring(Languages[Pos][1]).MakeUpperCase(); //Fix some files with countries in lowercase
                                else
                                    Language_Translated+=Languages[Pos][1];
                                Language_Translated+=__T(")");
                            }
                            else
                                for (size_t Pos2=1; Pos2<Languages[Pos].size(); Pos2++)
                                {
                                    Language_Translated+=__T('-'); //As the original string
                                    Language_Translated+=Languages[Pos][Pos2];
                                }
                        }
                        }
                        Language1.push_back(Language_Translated);
                        if (Languages[Pos][0].size()==2)
                        {
                            Language2.push_back(Languages[Pos][0]);
                            Language4.push_back(Languages[Pos].Read());
                        }
                        else
                        {
                            Language2.push_back(Ztring());
                            Language4.push_back(Ztring());
                        }
                        if (Languages[Pos][0].size()==3)
                            Language3.push_back(Languages[Pos][0]);
                        else if (!MediaInfoLib::Config.Iso639_2_Get(Languages[Pos][0]).empty())
                            Language3.push_back(MediaInfoLib::Config.Iso639_2_Get(Languages[Pos][0]));
                        else
                            Language3.push_back(Ztring());
                    }
                    else
                    {
                        Language1.push_back(Ztring());
                        Language2.push_back(Ztring());
                        Language3.push_back(Ztring());
                        Language4.push_back(Ztring());
                    }
                }

                Fill(StreamKind, StreamPos, Parameter+2, Language1.Read()); //String1
                Fill(StreamKind, StreamPos, Parameter+3, Language2.Read()); //String2
                Fill(StreamKind, StreamPos, Parameter+4, Language3.Read()); //String3
                Fill(StreamKind, StreamPos, Parameter+5, Language4.Read()); //String4
                Fill(StreamKind, StreamPos, Parameter+1, Retrieve(StreamKind, StreamPos, Parameter+2)); //String
            }
        }

        //ServiceName / ServiceProvider
        if (Parameter==Fill_Parameter(StreamKind, Generic_ServiceName)
         || Parameter==Fill_Parameter(StreamKind, Generic_ServiceProvider))
        {
            if (Retrieve(StreamKind, StreamPos, Parameter).find(__T(" - "))==string::npos && (Retrieve(StreamKind, StreamPos, Parameter).find(__T(':'))==2 || Retrieve(StreamKind, StreamPos, Parameter).find(__T(':'))==3))
            {
                Ztring Temp=Retrieve(StreamKind, StreamPos, Parameter);
                Temp.erase(0, Retrieve(StreamKind, StreamPos, Parameter).find(__T(':'))+1);
                (*Stream)[StreamKind][StreamPos](Parameter)=Temp;
            }
        }

        //FrameRate Nominal
        if (StreamKind==Stream_Video && (Parameter==Video_FrameRate || Parameter==Video_FrameRate_Nominal))
        {
            float32 FrameRate=Retrieve(Stream_Video, StreamPos, Video_FrameRate).To_float32();
            float32 FrameRate_Nominal=Retrieve(Stream_Video, StreamPos, Video_FrameRate_Nominal).To_float32();
            if (FrameRate_Nominal>FrameRate*0.9995 && FrameRate_Nominal<FrameRate*1.0005)
            {
                Ztring Temp=Retrieve(StreamKind, StreamPos, Video_FrameRate_Nominal);
                Clear(StreamKind, StreamPos, Video_FrameRate_Nominal);
                if (Parameter==Video_FrameRate)
                    Fill(StreamKind, StreamPos, Parameter, Temp, true);
            }
        }

        //Well known framerate values
        if (StreamKind==Stream_Video && (Parameter==Video_FrameRate || Parameter==Video_FrameRate_Nominal || Parameter==Video_FrameRate_Original)
         && Retrieve(Stream_Video, StreamPos, Video_FrameRate_Original_Num).empty()) // Ignoring when there is a num/den with discrepency between container and raw stream
        {
            Video_FrameRate_Rounding(Stream_Video, StreamPos, Parameter);
            if (Retrieve(Stream_Video, StreamPos, Video_FrameRate_Nominal)==Retrieve(Stream_Video, StreamPos, Video_FrameRate))
                Clear(Stream_Video, StreamPos, Video_FrameRate_Nominal);
            if (Parameter!=Video_FrameRate_Original && Retrieve(Stream_Video, StreamPos, Video_FrameRate_Original)==Retrieve(Stream_Video, StreamPos, Video_FrameRate))
                Clear(Stream_Video, StreamPos, Video_FrameRate_Original);
        }

        //Bits/(Pixel*Frame)
        if (StreamKind==Stream_Video && (Parameter==Video_BitRate || Parameter==Video_BitRate_Nominal || Parameter==Video_Width || Parameter==Video_Height || Parameter==Video_FrameRate))
        {
            float32 BitRate=Retrieve(Stream_Video, StreamPos, Video_BitRate).To_float32();
            if (BitRate==0)
                BitRate=Retrieve(Stream_Video, StreamPos, Video_BitRate_Nominal).To_float32();
            float F1=(float)Retrieve(Stream_Video, StreamPos, Video_Width).To_int32s()*(float)Retrieve(Stream_Video, StreamPos, Video_Height).To_int32s()*Retrieve(Stream_Video, StreamPos, Video_FrameRate).To_float32();
            if (BitRate && F1)
                Fill(Stream_Video, StreamPos, Video_Bits__Pixel_Frame_, BitRate/F1, 3, true);
        }

        //Well known bitrate values
        if (StreamKind==Stream_Video && (Parameter==Video_BitRate || Parameter==Video_BitRate_Nominal))
            Video_BitRate_Rounding(StreamPos, (video)Parameter);
        if (StreamKind==Stream_Audio && (Parameter==Audio_BitRate || Parameter==Audio_BitRate_Nominal))
            Audio_BitRate_Rounding(StreamPos, (audio)Parameter);
    }
}

//---------------------------------------------------------------------------
void File__Analyze::Fill (stream_t StreamKind, size_t StreamPos, size_t Parameter, float64 Value, int8u AfterComma, bool Replace)
{
    if (Parameter==Fill_Parameter(StreamKind, Generic_FrameRate))
    {
        size_t FrameRate_Num=Fill_Parameter(StreamKind, Generic_FrameRate_Num);
        size_t FrameRate_Den=Fill_Parameter(StreamKind, Generic_FrameRate_Den);

        Clear(StreamKind, StreamPos, FrameRate_Num);
        Clear(StreamKind, StreamPos, FrameRate_Den);

        if (Value)
        {
            if (float64_int64s(Value) - Value*1.001000 > -0.000002
             && float64_int64s(Value) - Value*1.001000 < +0.000002) // Detection of precise 1.001 (e.g. 24000/1001) taking into account precision of 64-bit float
            {
                Fill(StreamKind, StreamPos, FrameRate_Num, Value*1001,  0, Replace);
                Fill(StreamKind, StreamPos, FrameRate_Den,       1001, 10, Replace);
            }
            if (float64_int64s(Value) - Value*1.001001 > -0.000002
             && float64_int64s(Value) - Value*1.001001 < +0.000002) // Detection of rounded 1.001 (e.g. 23976/1000) taking into account precision of 64-bit float
            {
                Fill(StreamKind, StreamPos, FrameRate_Num,  Value*1000,  0, Replace);
                Fill(StreamKind, StreamPos, FrameRate_Den,        1000, 10, Replace);
            }
            if (!(Value - (int)Value)) // Detection of integer values
            {
                Fill(StreamKind, StreamPos, FrameRate_Num, (int)Value, 10, Replace);
                Fill(StreamKind, StreamPos, FrameRate_Den,          1, 10, Replace);
            }
        }
    }

    Fill(StreamKind, StreamPos, Parameter, Ztring::ToZtring(Value, AfterComma), Replace);
}

//---------------------------------------------------------------------------
void File__Analyze::Fill_Dup(stream_t StreamKind, size_t StreamPos, const char* Parameter, const Ztring& Value, bool Replace)
{
    const Ztring& OldValue=Retrieve_Const(StreamKind, StreamPos, Parameter);
    if (Value!=OldValue)
        Fill(StreamKind, StreamPos, Parameter, Value, Replace);
}

//---------------------------------------------------------------------------
void File__Analyze::Fill_Measure(stream_t StreamKind, size_t StreamPos, const char* Parameter, const Ztring& Value, const Ztring& Measure, bool Replace)
{
    string Parameter_String(Parameter);
    Parameter_String+="/String";
    Fill(StreamKind, StreamPos, Parameter, Value, Replace);
    Fill_SetOptions(StreamKind, StreamPos, Parameter, "N NFY");
    Fill(StreamKind, StreamPos, Parameter_String.c_str(), MediaInfoLib::Config.Language_Get(Value, Measure), Replace);
    Fill_SetOptions(StreamKind, StreamPos, Parameter_String.c_str(), "Y NFN");
}

//---------------------------------------------------------------------------
void File__Analyze::Fill (stream_t StreamKind, size_t StreamPos, const char* Parameter, const Ztring &Value, bool Replace)
{
    //Integrity
    if (StreamKind>Stream_Max || Parameter==NULL || Parameter[0]=='\0')
        return;

    //Handling values with \r\n inside
    if (Value.find_first_of(__T("\r\n"))!=string::npos)
    {
        Ztring CarriageReturnReplace=MediaInfoLib::Config.CarriageReturnReplace_Get();
        if (!CarriageReturnReplace.empty())
        {
            Ztring NewValue=Value;
            NewValue.FindAndReplace(__T("\r\n"), CarriageReturnReplace, 0, Ztring_Recursive);
            NewValue.FindAndReplace(__T("\r"), CarriageReturnReplace, 0, Ztring_Recursive);
            NewValue.FindAndReplace(__T("\n"), CarriageReturnReplace, 0, Ztring_Recursive);
            if (NewValue.size()>=CarriageReturnReplace.size() && NewValue.rfind(CarriageReturnReplace)==NewValue.size()-CarriageReturnReplace.size())
                NewValue.resize(NewValue.size()-CarriageReturnReplace.size());
            Fill(StreamKind, StreamPos, Parameter, NewValue, Replace);
            return;
        }
    }

    // Ignore useless values
    if (!Value.compare(__T("N/A"))) {
        return;
    }

    //Handle Value before StreamKind
    if (StreamKind==Stream_Max || StreamPos>=(*Stream)[StreamKind].size())
    {
        size_t StreamKindS=(size_t)StreamKind;
        if (StreamKind>=0 && StreamKind!=Stream_Max)
        {
            //Stream kind is found, moving content
            for (size_t Pos=0; Pos<Fill_Temp[Stream_Max].size(); Pos++)
                Fill_Temp[StreamKind].push_back(Fill_Temp[Stream_Max][Pos]);
            Fill_Temp[Stream_Max].clear();
        }
        else
        {
            //Stream kind is not found, checking if it was found previously
            for (StreamKindS=(size_t)Stream_General+1; StreamKindS<(size_t)Stream_Max; StreamKindS++)
                if (!Fill_Temp[StreamKindS].empty())
                    break;
        }

        const Ztring Parameter_UTF8 = Ztring().From_UTF8(Parameter);
        if (Replace)
        {
            for (size_t Pos=0; Pos<Fill_Temp[StreamKindS].size(); Pos++)
                if (Fill_Temp[StreamKindS][Pos].Parameter== Parameter_UTF8)
                {
                    Fill_Temp[StreamKindS][Pos].Value=Value;
                    return;
                }
        }
        fill_temp_item NewList;
        NewList.Parameter= Parameter_UTF8;
        NewList.Value=Value;
        Fill_Temp[StreamKindS].push_back(NewList);
        return; //No streams
    }

    //Handling of well known parameters
    const Ztring Parameter_Local = Ztring().From_UTF8(Parameter);
    const size_t Pos=MediaInfoLib::Config.Info_Get(StreamKind).Find(Parameter_Local);
    if (Pos!=Error)
    {
        Fill(StreamKind, StreamPos, Pos, Value, Replace);
        return;
    }

    if (StreamKind==Stream_Other && !strcmp(Parameter, "Codec"))
        return; // "Codec" does not exist in "Other"

    //Handling of unknown parameters
    ZtringListList& Stream_More_Item = (*Stream_More)[StreamKind][StreamPos];
    const Ztring Parameter_ISO = Ztring().From_ISO_8859_1(Parameter);
    if (Value.empty())
    {
        if (Replace)
        {
            const size_t Pos_ToReplace= Stream_More_Find(Stream_More_Item, Parameter_ISO);
            if (Pos_ToReplace!=(size_t)-1)
            {
                Stream_More_Item.erase(Stream_More_Item.begin()+Pos_ToReplace); //Empty value --> remove the line
                Stream_More_Index_Erase(Stream_More_Item, Pos_ToReplace, Parameter_ISO);
            }
        }
    }
    else
    {
        size_t Space=Parameter_ISO.rfind(__T(' '));
        size_t LastFound=(size_t)-1;
        if (Space!=string::npos)
        {
            Ztring ToSearch=Parameter_ISO.substr(0, Space);
            if (Stream_More_Find(Stream_More_Item, Parameter_ISO)==(size_t)-1) // Also updates the index
            {
                // Is there a line with the same prefix
                const auto& Positions=Stream_More_Index.Positions;
                const auto ToSearch_Space=ToSearch+__T(' ');
                const auto Group=Positions.lower_bound(ToSearch_Space);
                if (Positions.find(ToSearch)!=Positions.end() || (Group!=Positions.end() && !Group->first.compare(0, ToSearch_Space.size(), ToSearch_Space)))
                {
                    // Last line with the same prefix, searching from the end as it is usually there
                    for (size_t i=Stream_More_Item.size(); i--;)
                    {
                        const auto& Item=Stream_More_Item[i][Info_Name];
                        if (Item.rfind(ToSearch, 0)==0 && (Item.size()==ToSearch.size() || Item[ToSearch.size()]==__T(' ')))
                        {
                            LastFound=i;
                            break;
                        }
                    }
                }
            }
            if (LastFound!=(size_t)-1)
            {
                auto& Line=*Stream_More_Item.insert(Stream_More_Item.begin()+LastFound+1, ZtringList());
                Line.resize(max(max(Info_Name, Info_Text),max(Info_Options,Info_Name_Text))+1);
                Line[Info_Name]=Parameter_ISO;
                Line[Info_Text]=Value;
                Line[Info_Options]=__T("Y NT");
                Line[Info_Name_Text]=MediaInfoLib::Config.Language_Get(Parameter_Local);
                Stream_More_Index_Insert(Stream_More_Item, LastFound+1);
                return;
            }
        }

        auto Target=Stream_More_Find(Stream_More_Item, Parameter_ISO);
        if (Target==(size_t)-1)
        {
            size_t Pos=Stream_More_Item.size();
            Stream_More_Item.resize(Pos+1);
            auto& Line=Stream_More_Item.back();
            Line.resize(max(max(Info_Name, Info_Text),max(Info_Options,Info_Name_Text))+1);
            Line[Info_Name]=Parameter_ISO;
            Line[Info_Text]=Value;
            Line[Info_Options]=__T("Y NT");
            Line[Info_Name_Text]=MediaInfoLib::Config.Language_Get(Parameter_Local);
            Stream_More_Index_Insert(Stream_More_Item, Pos);
        }
        else
        {
            auto& Item=Stream_More_Item[Target](Info_Text);
            if (Item.empty() || Replace)
            {
                Item=Value;
            }
            else
            {
                Item+=MediaInfoLib::Config.TagSeparator_Get();
                Item+=Value;
            }
        }
    }
    Fill(StreamKind, StreamPos, (size_t)General_Count, Count_Get(StreamKind, StreamPos), 10, true);
}

//---------------------------------------------------------------------------
void File__Analyze::Fill (stream_t StreamKind, size_t StreamPos, const char* Parameter, ZtringList &Value, ZtringList& Id, bool Replace)
{
    //Test if not empty
    size_t Value_Size=Value.size();
    size_t i=0;
    for (; i<Value_Size; i++)
        if (!Value[i].empty())
            break;
    if (i==Value_Size)
        return;

    if (Value.size()!=Id.size())
    {
        Value.Separator_Set(0, __T(" / "));
        Fill(StreamKind, StreamPos, Parameter, Value.Read());
        return;
    }

    ZtringList List;
    List.Separator_Set(0, __T(" / "));
    for (size_t i=0; i<Value.size(); i++)
        if (!Value[i].empty()) // Only if there is a content
            List.push_back(Value[i]+(Id[i].empty()?Ztring():(__T(" (")+Id[i]+__T(')'))));
    Fill(StreamKind, StreamPos, Parameter, List.Read());
}

//---------------------------------------------------------------------------
// Same as Get(StreamKind, StreamPos, Name).empty() but using the index for unknown parameters
bool File__Analyze::Stream_More_IsEmpty(stream_t StreamKind, size_t StreamPos, const Ztring& Name)
{
    if (MediaInfoLib::Config.Info_Get(StreamKind).Find(Name)!=Error)
        return Get(StreamKind, StreamPos, Name).empty();
    auto& Stream_More_Item=(*Stream_More)[StreamKind][StreamPos];
    const auto Target=Stream_More_Find(Stream_More_Item, Name);
    return Target==(size_t)-1 || Stream_More_Item[Target](Info_Text).empty();
}

//---------------------------------------------------------------------------
// Position of the first line with this name, the index is rebuilt if the list changed outside of Fill()
size_t File__Analyze::Stream_More_Find(const ZtringListList& List, const Ztring& Name)
{
    const ZtringList* Data=List.empty()?nullptr:&List[0];
    if (Stream_More_Index.List!=&List || Stream_More_Index.Data!=Data || Stream_More_Index.Size!=List.size())
    {
        Stream_More_Index.List=&List;
        Stream_More_Index.Data=Data;
        Stream_More_Index.Size=List.size();
        Stream_More_Index.Positions.clear();
        Stream_More_Index.Lines.clear();
        for (size_t Pos=0; Pos<List.size(); Pos++)
            if (!List[Pos].empty() && Stream_More_Index.Positions.emplace(List[Pos][0], Stream_More_Index.Lines.size()).second) // First one is kept
                Stream_More_Index.Lines.push_back(Pos);
    }

    auto It=Stream_More_Index.Positions.find(Name);
    if (It==Stream_More_Index.Positions.end())
        return (size_t)-1;
    const auto Line=Stream_More_Index.Lines[It->second];
    if (Line<List.size() && !List[Line].empty() && List[Line][0]==Name)
        return Line;

    // Not coherent, list was modified elsewhere
    Stream_More_Index.List=nullptr;
    return Stream_More_Find(List, Name);
}

//---------------------------------------------------------------------------
// Update of the index after a line was inserted at Pos, the index is rebuilt later if it was not coherent with the list before the insertion
void File__Analyze::Stream_More_Index_Insert(const ZtringListList& List, size_t Pos)
{
    if (Stream_More_Index.List!=&List || Stream_More_Index.Size+1!=List.size())
    {
        Stream_More_Index.List=nullptr;
        return;
    }
    if (Pos<Stream_More_Index.Size)
        for (auto& Line : Stream_More_Index.Lines)
            if (Line>=Pos)
                Line++;
    const auto Position=Stream_More_Index.Positions.emplace(List[Pos][0], Stream_More_Index.Lines.size());
    if (Position.second)
        Stream_More_Index.Lines.push_back(Pos);
    else if (Stream_More_Index.Lines[Position.first->second]>Pos)
        Stream_More_Index.Lines[Position.first->second]=Pos; // First one is kept
    Stream_More_Index.Data=&List[0];
    Stream_More_Index.Size++;
}

//---------------------------------------------------------------------------
// Update of the index after the line at Pos with this name was erased, the index is rebuilt later if it was not coherent with the list before the erasure
void File__Analyze::Stream_More_Index_Erase(const ZtringListList& List, size_t Pos, const Ztring& Name)
{
    if (Stream_More_Index.List!=&List || Stream_More_Index.Size!=List.size()+1)
    {
        Stream_More_Index.List=nullptr;
        return;
    }
    for (auto& Line : Stream_More_Index.Lines)
        if (Line>Pos)
            Line--;
    const auto Position=Stream_More_Index.Positions.find(Name);
    if (Position!=Stream_More_Index.Positions.end() && Stream_More_Index.Lines[Position->second]==Pos)
    {
        // Next line with the same name becomes the first one
        size_t Next=Pos;
        while (Next<List.size() && (List[Next].empty() || List[Next][0]!=Name))
            Next++;
        if (Next<List.size())
            Stream_More_Index.Lines[Position->second]=Next;
        else
            Stream_More_Index.Positions.erase(Position);
    }
    Stream_More_Index.Data=List.empty()?nullptr:&List[0];
    Stream_More_Index.Size--;
}

//---------------------------------------------------------------------------
void File__Analyze::Fill_SetOptions(stream_t StreamKind, size_t StreamPos, const char* Parameter, const char* Options)
{
    //Integrity
    if (StreamKind>Stream_Max || Parameter==NULL || Parameter[0]=='\0')
        return;

    //Handle Value before StreamKind
    if (StreamKind>=0 && (StreamKind==Stream_Max || StreamPos>=(*Stream)[StreamKind].size()))
    {
        Fill_Temp_Options[StreamKind][Parameter]=Options;
        return; //No streams
    }

    //Handling of well known parameters
    size_t Pos=MediaInfoLib::Config.Info_Get(StreamKind).Find(Ztring().From_UTF8(Parameter));
    if (Pos!=Error)
    {
        //We can not change that
        return;
    }

    auto& Stream_More_Item=(*Stream_More)[StreamKind][StreamPos];
    const Ztring Parameter_ISO=Ztring().From_ISO_8859_1(Parameter);
    const auto Target=Stream_More_Find(Stream_More_Item, Parameter_ISO);
    if (Target!=(size_t)-1)
        Stream_More_Item[Target](Info_Options).From_UTF8(Options);
    else
    {
        Stream_More_Item(Parameter_ISO, Info_Options).From_UTF8(Options);
        Stream_More_Index_Insert(Stream_More_Item, Stream_More_Item.size()-1);
    }
}

//---------------------------------------------------------------------------
const Ztring &File__Analyze::Retrieve_Const (stream_t StreamKind, size_t StreamPos, size_t Parameter, info_t KindOfInfo)
{
    //Integrity
    if (StreamKind>=Stream_Max
     || StreamPos>=(*Stream)[StreamKind].size()
     || Parameter>=MediaInfoLib::Config.Info_Get(StreamKind).size()+(*Stream_More)[StreamKind][StreamPos].size())
    {
        if ((size_t)StreamKind<sizeof(Fill_Temp)/sizeof(*Fill_Temp))
        {
            Ztring Parameter_Local;
            Parameter_Local.From_Number(Parameter);
            for (size_t Pos=0; Pos<Fill_Temp[StreamKind].size(); Pos++)
                if (Fill_Temp[StreamKind][Pos].Parameter==Parameter_Local)
                    return Fill_Temp[StreamKind][Pos].Value;
        }

        return MediaInfoLib::Config.EmptyString_Get();
    }

    if (Parameter>=MediaInfoLib::Config.Info_Get(StreamKind).size())
    {
        Parameter-=MediaInfoLib::Config.Info_Get(StreamKind).size();
        if ((size_t)KindOfInfo>=(*Stream_More)[StreamKind][StreamPos][Parameter].size())
            return MediaInfoLib::Config.EmptyString_Get();
        return (*Stream_More)[StreamKind][StreamPos][Parameter][KindOfInfo];
    }

    if (KindOfInfo!=Info_Text)
        return MediaInfoLib::Config.Info_Get(StreamKind, Parameter, KindOfInfo);

    if ((size_t)StreamKind>=(*Stream).size() || StreamPos>=(*Stream)[StreamKind].size() || Parameter>=(*Stream)[StreamKind][StreamPos].size())
        return MediaInfoLib::Config.EmptyString_Get();
    return (*Stream)[StreamKind][StreamPos](Parameter);
}

//---------------------------------------------------------------------------
Ztring File__Analyze::Retrieve (stream_t StreamKind, size_t StreamPos, size_t Parameter, info_t KindOfInfo)
{
    //Integrity
    if (StreamKind>=Stream_Max
     || StreamPos>=(*Stream)[StreamKind].size()
     || Parameter>=MediaInfoLib::Config.Info_Get(StreamKind).size()+(*Stream_More)[StreamKind][StreamPos].size())
        return MediaInfoLib::Config.EmptyString_Get();

    if (Parameter>=MediaInfoLib::Config.Info_Get(StreamKind).size())
    {
        Parameter-=MediaInfoLib::Config.Info_Get(StreamKind).size();
        if ((size_t)KindOfInfo>=(*Stream_More)[StreamKind][StreamPos][Parameter].size())
            return MediaInfoLib::Config.EmptyString_Get();
        return (*Stream_More)[StreamKind][StreamPos][Parameter][KindOfInfo];
    }

    if ((size_t)KindOfInfo!=Info_Text)
        return MediaInfoLib::Config.Info_Get(StreamKind, Parameter, KindOfInfo);

    if ((size_t)StreamKind>=(*Stream).size() || StreamPos>=(*Stream)[StreamKind].size() || Parameter>=(*Stream)[StreamKind][StreamPos].size())
        return MediaInfoLib::Config.EmptyString_Get();
    return (*Stream)[StreamKind][StreamPos](Parameter);
}

//---------------------------------------------------------------------------
const Ztring &File__Analyze::Retrieve_Const (stream_t StreamKind, size_t StreamPos, const char* Parameter, info_t KindOfInfo)
{
    //Integrity
    if (StreamKind>=Stream_Max
     || Parameter==NULL
     || Parameter[0]=='\0')
        return MediaInfoLib::Config.EmptyString_Get();

    if (KindOfInfo!=Info_Text)
        return MediaInfoLib::Config.Info_Get(StreamKind, Parameter, KindOfInfo);
    const Ztring Parameter_Local = Ztring().From_UTF8(Parameter);
    size_t Parameter_Pos=MediaInfoLib::Config.Info_Get(StreamKind).Find(Parameter_Local);
    if (Parameter_Pos==Error)
    {
        if (StreamPos==(*Stream)[StreamKind].size())
        {
            for (size_t Pos=0; Pos<Fill_Temp[StreamKind].size(); Pos++)
                if (Fill_Temp[StreamKind][Pos].Parameter==Parameter_Local)
                    return Fill_Temp[StreamKind][Pos].Value;
        }
        if (StreamPos>=(*Stream)[StreamKind].size())
            return MediaInfoLib::Config.EmptyString_Get();
        Parameter_Pos=Stream_More_Find((*Stream_More)[StreamKind][StreamPos], Parameter_Local);
        if (Parameter_Pos==Error)
            return MediaInfoLib::Config.EmptyString_Get();
        return (*Stream_More)[StreamKind][StreamPos](Parameter_Pos, 1);
    }
    if ((size_t)StreamKind>=(*Stream).size() || StreamPos>=(*Stream)[StreamKind].size() || Parameter_Pos>=(*Stream)[StreamKind][StreamPos].size())
        return MediaInfoLib::Config.EmptyString_Get();
    return (*Stream)[StreamKind][StreamPos](Parameter_Pos);
}

//---------------------------------------------------------------------------
Ztring File__Analyze::Retrieve (stream_t StreamKind, size_t StreamPos, const char* Parameter, info_t KindOfInfo)
{
    //Integrity
    if (StreamKind>=Stream_Max
     || StreamPos>=(*Stream)[StreamKind].size()
     || Parameter==NULL
     || Parameter[0]=='\0')
        return MediaInfoLib::Config.EmptyString_Get();

    if (KindOfInfo!=Info_Text)
        return MediaInfoLib::Config.Info_Get(StreamKind, Parameter, KindOfInfo);
    const Ztring Parameter_Local = Ztring().From_UTF8(Parameter);
    size_t Parameter_Pos=MediaInfoLib::Config.Info_Get(StreamKind).Find(Parameter_Local);
    if (Parameter_Pos==Error)
    {
        Parameter_Pos=Stream_More_Find((*Stream_More)[StreamKind][StreamPos], Parameter_Local);
        if (Parameter_Pos==Error)
            return MediaInfoLib::Config.EmptyString_Get();
        return (*Stream_More)[StreamKind][StreamPos](Parameter_Pos, 1);
    }
    if ((size_t)StreamKind>=(*Stream).size() || StreamPos>=(*Stream)[StreamKind].size() || Parameter_Pos>=(*Stream)[StreamKind][StreamPos].size())
        return MediaInfoLib::Config.EmptyString_Get();
    return (*Stream)[StreamKind][StreamPos](Parameter_Pos);
}

//---------------------------------------------------------------------------
void File__Analyze::Clear (stream_t StreamKind, size_t StreamPos, const char* Parameter)
{
    //Integrity
    if (StreamKind>=Stream_Max || StreamKind<Stream_General
     || Parameter==NULL
     || Parameter[0]=='\0')
        return;

    if (StreamPos>=(*Stream)[StreamKind].size())
    {
        Ztring Parameter_String=Ztring().From_UTF8(Parameter);
        for (size_t Pos=0; Pos<Fill_Temp[StreamKind].size(); Pos++)
            if (Fill_Temp[StreamKind][Pos].Parameter==Parameter_String)
            {
                Fill_Temp[StreamKind].erase(Fill_Temp[StreamKind].begin() + Pos);
                Pos--;
            }
        return;
    }
    const Ztring Parameter_Local = Ztring().From_UTF8(Parameter);
    size_t Parameter_Pos=MediaInfoLib::Config.Info_Get(StreamKind).Find(Parameter_Local);
    if (Parameter_Pos==Error)
    {
        Parameter_Pos=Stream_More_Find((*Stream_More)[StreamKind][StreamPos], Parameter_Local);
        if (Parameter_Pos==Error)
            return;
        (*Stream_More)[StreamKind][StreamPos](Parameter_Pos, 1).clear();
        return;
    }

    Clear(StreamKind, StreamPos, Parameter_Pos);
}

//---------------------------------------------------------------------------
void File__Analyze::Clear (stream_t StreamKind, size_t StreamPos, size_t Parameter)
{
    //Integrity
    if (StreamKind>=Stream_Max
     || StreamPos>=(*Stream)[StreamKind].size())
        return;

    //Normal
    if (Parameter<MediaInfoLib::Config.Info_Get(StreamKind).size())
    {
        //Is something available?
        if (Parameter>=(*Stream)[StreamKind][StreamPos].size())
            return; //Was never filled, no nead to clear it

        //Clearing
        (*Stream)[StreamKind][StreamPos][Parameter].clear();

        //Human readable
        if (MediaInfoLib::Config.ReadByHuman_Get())
        {
            //Strings
            const Ztring &List_Measure_Value=MediaInfoLib::Config.Info_Get(StreamKind).Read(Parameter, Info_Measure);
                 if (List_Measure_Value==__T(" byte"))
            {
                const Ztring &Temp=MediaInfoLib::Config.Info_Get(StreamKind).Read(Parameter, Info_Name);
                size_t List_Size=Temp.find(__T("StreamSize"))==string::npos?5:7; //for /String5, with percentage, and proportion
                for (size_t Pos=Parameter+1; Pos<=Parameter+List_Size; Pos++)
                    if (Pos<(*Stream)[StreamKind][StreamPos].size())
                        (*Stream)[StreamKind][StreamPos][Pos].clear();
            }
            else if (List_Measure_Value==__T(" bps") || List_Measure_Value==__T(" Hz"))
            {
                if (Parameter+1<(*Stream)[StreamKind][StreamPos].size())
                    (*Stream)[StreamKind][StreamPos][Parameter+1].clear();
            }
            else if (List_Measure_Value==__T(" ms"))
            {
                for (size_t Pos=Parameter+1; Pos<=Parameter+6; Pos++)
                    if (Pos<(*Stream)[StreamKind][StreamPos].size())
                        (*Stream)[StreamKind][StreamPos][Pos].clear();
            }
            else if (List_Measure_Value==__T("Yes"))
            {
                if (Parameter+1<(*Stream)[StreamKind][StreamPos].size())
                    (*Stream)[StreamKind][StreamPos][Parameter+1].clear();
            }
            else if (!List_Measure_Value.empty())
            {
                if (Parameter+1<(*Stream)[StreamKind][StreamPos].size())
                    (*Stream)[StreamKind][StreamPos][Parameter+1].clear();
            }
            else if (Parameter+1<(*Stream)[StreamKind][StreamPos].size() && MediaInfoLib::Config.Info_Get(StreamKind).Read(Parameter+1, Info_Name).find(__T("/String"))!=string::npos)
            {
                (*Stream)[StreamKind][StreamPos][Parameter+1].clear();
            }
        }

        return;
    }

    //More
    Parameter-=(*Stream)[StreamKind][StreamPos].size(); //For having Stream_More position
    if (Parameter<(*Stream_More)[StreamKind][StreamPos].size())
    {
        (*Stream_More)[StreamKind][StreamPos].erase((*Stream_More)[StreamKind][StreamPos].begin()+Parameter);
        return;
    }
}

//---------------------------------------------------------------------------
void File__Analyze::Clear (stream_t StreamKind, size_t StreamPos)
{
    //Integrity
    if (StreamKind>=Stream_Max
     || StreamPos>=(*Stream)[StreamKind].size())
        return;

    (*Stream)[StreamKind].erase((*Stream)[StreamKind].begin()+StreamPos);
}

//---------------------------------------------------------------------------
void File__Analyze::Clear (stream_t StreamKind)
{
    //Integrity
    if (StreamKind>=Stream_Max)
        return;

    (*Stream)[StreamKind].clear();
}

//---------------------------------------------------------------------------
void File__Analyze::Fill_Flush()
{
    Stream_Prepare(Stream_Max); //clear filling
    for (size_t StreamKind=(size_t)Stream_General; StreamKind<(size_t)Stream_Max+1; StreamKind++) // +1 because Fill_Temp[Stream_Max] is used when StreamKind is unknown
    {
        Fill_Temp[StreamKind].clear();
        Fill_Temp_Options[StreamKind].clear();
    }
}

//---------------------------------------------------------------------------
size_t File__Analyze::Merge(MediaInfo_Internal &ToAdd, bool)
{
    size_t Count=0;
    for (size_t StreamKind=(size_t)Stream_General; StreamKind<(size_t)Stream_Max; StreamKind++)
    {
        size_t StreamPos_Count=ToAdd.Count_Get((stream_t)StreamKind);
        for (size_t StreamPos=0; StreamPos<StreamPos_Count; StreamPos++)
        {
            //Prepare a new stream
            if (StreamPos>=Count_Get((stream_t)StreamKind))
                Stream_Prepare((stream_t)StreamKind);

            //Merge
            size_t Pos_Count=ToAdd.Count_Get((stream_t)StreamKind, StreamPos);
            for (size_t Pos=0; Pos<Pos_Count; Pos++)
            {
                if (StreamKind!=Stream_General
                 || !(Pos==General_CompleteName
                   || Pos==General_CompleteName_Last
                   || Pos==General_FolderName
                   || Pos==General_FileName
                   || Pos==General_FileExtension
                   || Pos==General_File_Created_Date
                   || Pos==General_Format
                   || Pos==General_Format_String
                   || Pos==General_Format_Extensions
                   || Pos==General_Format_Info
                   || Pos==General_Codec
                   || Pos==General_Codec_String
                   || Pos==General_Codec_Extensions
                   || Pos==General_FileSize
                   || Pos==General_FileSize_String
                   || Pos==General_FileSize_String1
                   || Pos==General_FileSize_String2
                   || Pos==General_FileSize_String3
                   || Pos==General_FileSize_String4
                   || Pos==General_File_Created_Date_Local
                   || Pos==General_File_Modified_Date
                   || Pos==General_File_Modified_Date_Local))
                    Fill((stream_t)StreamKind, StreamPos, Ztring(ToAdd.Get((stream_t)StreamKind, StreamPos, Pos, Info_Name)).To_UTF8().c_str(), ToAdd.Get((stream_t)StreamKind, StreamPos, Pos), true);
            }

            Count++;
        }
    }

    return Count;
}

//---------------------------------------------------------------------------
size_t File__Analyze::Merge(MediaInfo_Internal &ToAdd, stream_t StreamKind, size_t StreamPos_From, size_t StreamPos_To, bool)
{
    size_t Pos_Count=ToAdd.Count_Get(StreamKind, StreamPos_From);
    for (size_t Pos=General_Inform; Pos<Pos_Count; Pos++)
        if (!ToAdd.Get(StreamKind, StreamPos_From, Pos).empty())
            Fill(StreamKind, StreamPos_To, Ztring(ToAdd.Get((stream_t)StreamKind, StreamPos_From, Pos, Info_Name)).To_UTF8().c_str(), ToAdd.Get(StreamKind, StreamPos_From, Pos), true);

    return 1;
}

//---------------------------------------------------------------------------
size_t File__Analyze::Merge(File__Analyze &ToAdd, bool Erase)
{
    size_t Count=0;
    for (size_t StreamKind=(size_t)Stream_General+1; StreamKind<(size_t)Stream_Max; StreamKind++)
        for (size_t StreamPos=0; StreamPos<(*ToAdd.Stream)[StreamKind].size(); StreamPos++)
        {
            //Prepare a new stream
            Stream_Prepare((stream_t)StreamKind);

            //Merge
            Merge(ToAdd, (stream_t)StreamKind, StreamPos, StreamPos_Last, Erase);

            Count++;
        }
    return Count;
}

//---------------------------------------------------------------------------
size_t File__Analyze::Merge(File__Analyze &ToAdd, stream_t StreamKind, size_t StreamPos_From, size_t StreamPos_To, bool Erase)
{
    //Integrity
    if (StreamKind>=Stream_Max || !ToAdd.Stream || StreamPos_From>=(*ToAdd.Stream)[StreamKind].size())
        return 0;

    //Destination
    while (StreamPos_To>=(*Stream)[StreamKind].size())
        Stream_Prepare(StreamKind);

    //Specific stuff
    Ztring Width_Temp, Height_Temp, PixelAspectRatio_Temp, DisplayAspectRatio_Temp, FrameRate_Temp, FrameRate_Num_Temp, FrameRate_Den_Temp, FrameRate_Mode_Temp, ScanType_Temp, ScanOrder_Temp, HDR_Temp[Video_HDR_Format_Compatibility-Video_HDR_Format+1], Channels_Temp[4], Delay_Temp, Delay_DropFrame_Temp, Delay_Source_Temp, Delay_Settings_Temp, Source_Temp, Source_Kind_Temp, Source_Info_Temp, ColorSpace_Temp;
    if (StreamKind==Stream_Video)
    {
        Width_Temp=Retrieve(Stream_Video, StreamPos_To, Video_Width);
        Height_Temp=Retrieve(Stream_Video, StreamPos_To, Video_Height);
        PixelAspectRatio_Temp=Retrieve(Stream_Video, StreamPos_To, Video_PixelAspectRatio); //We want to keep the PixelAspectRatio_Temp of the video stream
        DisplayAspectRatio_Temp=Retrieve(Stream_Video, StreamPos_To, Video_DisplayAspectRatio); //We want to keep the DisplayAspectRatio_Temp of the video stream
        FrameRate_Temp=Retrieve(Stream_Video, StreamPos_To, Video_FrameRate); //We want to keep the FrameRate of AVI 120 fps
        FrameRate_Num_Temp=Retrieve(Stream_Video, StreamPos_To, Video_FrameRate_Num);
        FrameRate_Den_Temp=Retrieve(Stream_Video, StreamPos_To, Video_FrameRate_Den);
        FrameRate_Mode_Temp=Retrieve(Stream_Video, StreamPos_To, Video_FrameRate_Mode); //We want to keep the FrameRate_Mode of AVI 120 fps
        ScanType_Temp=Retrieve(Stream_Video, StreamPos_To, Video_ScanType);
        ScanOrder_Temp=Retrieve(Stream_Video, StreamPos_To, Video_ScanOrder);
        ColorSpace_Temp=Retrieve(Stream_Video, StreamPos_To, Video_ColorSpace);
        for (size_t i=Video_HDR_Format; i<=Video_HDR_Format_Compatibility; i++)
            HDR_Temp[i-Video_HDR_Format]=Retrieve(Stream_Video, StreamPos_To, i);
    }
    if (StreamKind==Stream_Audio)
    {
        Channels_Temp[0]=Retrieve(Stream_Audio, StreamPos_To, Audio_Channel_s_);
        Channels_Temp[2]=Retrieve(Stream_Audio, StreamPos_To, Audio_ChannelPositions);
        Channels_Temp[3]=Retrieve(Stream_Audio, StreamPos_To, Audio_ChannelPositions_String2);
        Channels_Temp[1]=Retrieve(Stream_Audio, StreamPos_To, Audio_ChannelLayout);
    }
    if (StreamKind==Stream_Text)
    {
        FrameRate_Temp=Retrieve(Stream_Text, StreamPos_To, Text_FrameRate);
        FrameRate_Num_Temp=Retrieve(Stream_Text, StreamPos_To, Text_FrameRate_Num);
        FrameRate_Den_Temp=Retrieve(Stream_Text, StreamPos_To, Text_FrameRate_Den);
        FrameRate_Mode_Temp=Retrieve(Stream_Text, StreamPos_To, Text_FrameRate_Mode);
    }
    if (StreamKind==Stream_Image)
    {
        ColorSpace_Temp=Retrieve(Stream_Image, StreamPos_To, Image_ColorSpace);
    }
    if (ToAdd.Retrieve(StreamKind, StreamPos_From, Fill_Parameter(StreamKind, Generic_Delay_Source))==__T("Container"))
    {
        Fill(StreamKind, StreamPos_To, "Delay_Original", Retrieve(StreamKind, StreamPos_To, "Delay"), true);
        Clear(StreamKind, StreamPos_To, "Delay");
        Fill(StreamKind, StreamPos_To, "Delay_Original_DropFrame", Retrieve(StreamKind, StreamPos_To, "Delay_DropFrame"), true);
        Clear(StreamKind, StreamPos_To, "Delay_DropFrame");
        Fill(StreamKind, StreamPos_To, "Delay_Original_Source", Retrieve(StreamKind, StreamPos_To, "Delay_Source"), true);
        Clear(StreamKind, StreamPos_To, "Delay_Source");
        if (!ToAdd.Retrieve(StreamKind, StreamPos_To, "Format").empty()) //Exception: MPEG-4 TimeCode, settings are in the MPEG-4 header
        {
            Fill(StreamKind, StreamPos_To, "Delay_Original_Settings", Retrieve(StreamKind, StreamPos_To, "Delay_Settings"), true);
            Clear(StreamKind, StreamPos_To, "Delay_Settings");
        }
    }
    else
    {
        Delay_Temp=Retrieve(StreamKind, StreamPos_To, "Delay"); //We want to keep the Delay from the stream
        Delay_Settings_Temp=Retrieve(StreamKind, StreamPos_To, "Delay_Settings"); //We want to keep the Delay_Settings from the stream
        Delay_DropFrame_Temp=Retrieve(StreamKind, StreamPos_To, "Delay_DropFrame"); //We want to keep the Delay_Source from the stream
        Delay_Source_Temp=Retrieve(StreamKind, StreamPos_To, "Delay_Source"); //We want to keep the Delay_Source from the stream
    }
    Source_Temp=Retrieve(StreamKind, StreamPos_To, "Source");
    Source_Kind_Temp=Retrieve(StreamKind, StreamPos_To, "Source_Kind");
    Source_Info_Temp=Retrieve(StreamKind, StreamPos_To, "Source_Info");
    Ztring BitRate_Temp=Retrieve(StreamKind, StreamPos_To, "BitRate");
    Ztring CodecID_Temp=Retrieve(StreamKind, StreamPos_To, "CodecID");
    Ztring Title_Temp=Retrieve(StreamKind, StreamPos_To, "Title");

    //Merging
    size_t Size=ToAdd.Count_Get(StreamKind, StreamPos_From);
    size_t Size_NotExtra=MediaInfoLib::Config.Info_Get(StreamKind).size();
    for (size_t Pos=General_Inform; Pos<Size; Pos++)
    {
        const Ztring &ToFill_Value=ToAdd.Get(StreamKind, StreamPos_From, Pos);
        if (StreamKind==Stream_Video && ShowSource_IsInList((video)Pos))
        {
            auto ShowSource_Offset=ShowSource_IsInList((video)Pos);
            auto Pos1=Pos+ShowSource_Offset;
            const Ztring &ToFill_FromContainer=Get(StreamKind, StreamPos_To, Pos);
            if (!ToAdd.Retrieve_Const(StreamKind, StreamPos_From, Pos1).empty())
            {
                if (!Retrieve_Const(StreamKind, StreamPos_To, Pos1).empty())
                {
                    if (ToFill_Value==ToFill_FromContainer)
                    {
                        if (Retrieve_Const(StreamKind, StreamPos_To, Pos1)!=ToAdd.Retrieve_Const(StreamKind, StreamPos_From, Pos1))
                            Fill(StreamKind, StreamPos_To, Pos1, Retrieve_Const(StreamKind, StreamPos_To, Pos1)+MediaInfoLib::Config.TagSeparator_Get()+ToAdd.Retrieve_Const(StreamKind, StreamPos_From, Pos1), true);
                    }
                    else
                    {
                        auto Pos2=Pos1+1;
                        auto Pos3=Pos2+1;
                        Fill(StreamKind, StreamPos_To, Pos3, ToAdd.Retrieve_Const(StreamKind, StreamPos_From, Pos1));
                        Fill(StreamKind, StreamPos_To, Pos2, ToFill_Value);
                    }
                }
                else
                {
                    Fill(StreamKind, StreamPos_To, Pos1, ToAdd.Retrieve_Const(StreamKind, StreamPos_From, Pos1));
                    Fill(StreamKind, StreamPos_To, Pos, ToFill_Value);
                }
            }
        }
        else if (StreamKind==Stream_Video && Pos>1 && (ShowSource_IsInList((video)(Pos-1))==1 || ShowSource_IsInList((video)(Pos-2))==2))
        {
            //Ignore
        }
        else if (!ToFill_Value.empty() && (Erase || (Pos < Size_NotExtra && Get(StreamKind, StreamPos_To, Pos).empty()) || (Pos >= Size_NotExtra && Stream_More_IsEmpty(StreamKind, StreamPos_To, ToAdd.Get(StreamKind, StreamPos_From, Pos, Info_Name)))))
        {
            if (Pos<MediaInfoLib::Config.Info_Get(StreamKind).size())
                Fill(StreamKind, StreamPos_To, Pos, ToFill_Value, true);
            else
            {
                Fill(StreamKind, StreamPos_To, ToAdd.Get(StreamKind, StreamPos_From, Pos, Info_Name).To_UTF8().c_str(), ToFill_Value, true);
                Fill_SetOptions(StreamKind, StreamPos_To, ToAdd.Get(StreamKind, StreamPos_From, Pos, Info_Name).To_UTF8().c_str(), ToAdd.Get(StreamKind, StreamPos_From, Pos, Info_Options).To_UTF8().c_str());
            }
        }
    }

    //Specific stuff
    if (StreamKind==Stream_Video)
    {
        Ztring PixelAspectRatio_Original=Retrieve(Stream_Video, StreamPos_To, Video_PixelAspectRatio);
        Ztring DisplayAspectRatio_Original=Retrieve(Stream_Video, StreamPos_To, Video_DisplayAspectRatio);

        if (!Width_Temp.empty() && Width_Temp!=Retrieve(Stream_Video, StreamPos_To, Video_Width)
            && !(Retrieve(Stream_Video, StreamPos_To, Video_Format) == __T("DV") && Width_Temp == __T("1920") && (Retrieve(Stream_Video, StreamPos_Last, Video_Width) == __T("1280") || Retrieve(Stream_Video, StreamPos_Last, Video_Width) == __T("1440")))) // Exception: DVCPRO HD is really 1440 but lot of containers fill the width value with the marketing width 1920, we ignore it
        {
            Fill(Stream_Video, StreamPos_To, Video_Width_Original, (*Stream)[Stream_Video][StreamPos_To][Video_Width], true);
            Fill(Stream_Video, StreamPos_To, Video_Width, Width_Temp, true);
        }
        if (!Height_Temp.empty() && Height_Temp!=Retrieve(Stream_Video, StreamPos_To, Video_Height))
        {
            Fill(Stream_Video, StreamPos_To, Video_Height_Original, (*Stream)[Stream_Video][StreamPos_To][Video_Height], true);
            Fill(Stream_Video, StreamPos_To, Video_Height, Height_Temp, true);
        }
        if (!PixelAspectRatio_Temp.empty() && PixelAspectRatio_Temp!=Retrieve(Stream_Video, StreamPos_To, Video_PixelAspectRatio))
        {
            Fill(Stream_Video, StreamPos_To, Video_PixelAspectRatio_Original, PixelAspectRatio_Original, true);
            Fill(Stream_Video, StreamPos_To, Video_PixelAspectRatio, PixelAspectRatio_Temp, true);
        }
        if (!DisplayAspectRatio_Temp.empty() && DisplayAspectRatio_Temp!=DisplayAspectRatio_Original)
        {
            Fill(Stream_Video, StreamPos_To, Video_DisplayAspectRatio_Original, DisplayAspectRatio_Original, true);
            Fill(Stream_Video, StreamPos_To, Video_DisplayAspectRatio, DisplayAspectRatio_Temp, true);
        }
        if (!FrameRate_Temp.empty())
        {
            const Ztring& FramesPerContainerBlock=Retrieve(Stream_Video, StreamPos_To, "FramesPerContainerBlock");
            if (!FramesPerContainerBlock.empty())
                FrameRate_Temp.From_Number(FrameRate_Temp.To_float64()*FramesPerContainerBlock.To_float64());
        }
        if ((!FrameRate_Temp.empty() && FrameRate_Temp!=Retrieve(Stream_Video, StreamPos_To, Video_FrameRate))
         || (!FrameRate_Num_Temp.empty() && FrameRate_Num_Temp!=Retrieve(Stream_Video, StreamPos_To, Video_FrameRate_Num))
         || (!FrameRate_Den_Temp.empty() && FrameRate_Den_Temp!=Retrieve(Stream_Video, StreamPos_To, Video_FrameRate_Den)))
        {
            Fill(Stream_Video, StreamPos_To, Video_FrameRate_Original, ToAdd.Retrieve(Stream_Video, StreamPos_To, Video_FrameRate), true);
            Fill(Stream_Video, StreamPos_To, Video_FrameRate_Original_Num, ToAdd.Retrieve(Stream_Video, StreamPos_To, Video_FrameRate_Num), true);
            Fill(Stream_Video, StreamPos_To, Video_FrameRate_Original_Den, ToAdd.Retrieve(Stream_Video, StreamPos_To, Video_FrameRate_Den), true);
            Fill(Stream_Video, StreamPos_To, Video_FrameRate, FrameRate_Temp, true);
            Fill(Stream_Video, StreamPos_To, Video_FrameRate_Num, FrameRate_Num_Temp, true);
            Fill(Stream_Video, StreamPos_To, Video_FrameRate_Den, FrameRate_Den_Temp, true);
        }
        if (!FrameRate_Mode_Temp.empty() && FrameRate_Mode_Temp!=Retrieve(Stream_Video, StreamPos_To, Video_FrameRate_Mode))
        {
            Fill(Stream_Video, StreamPos_To, Video_FrameRate_Mode_Original, (*Stream)[Stream_Video][StreamPos_To][Video_FrameRate_Mode], true);
            Fill(Stream_Video, StreamPos_To, Video_FrameRate_Mode, FrameRate_Mode_Temp, true);
        }
        if (!ScanType_Temp.empty() && (ScanType_Temp!=Retrieve(Stream_Video, StreamPos_To, Video_ScanType) && !(ScanType_Temp==__T("Interlaced") && Retrieve(Stream_Video, StreamPos_To, Video_ScanType)==__T("MBAFF"))))
        {
            Fill(Stream_Video, StreamPos_To, Video_ScanType_Original, (*Stream)[Stream_Video][StreamPos_To][Video_ScanType], true);
            Fill(Stream_Video, StreamPos_To, Video_ScanType, ScanType_Temp, true);
        }
        if (Retrieve(Stream_Video, StreamPos_To, Video_ScanType_Original)!=__T("Progressive") && ((!ScanOrder_Temp.empty() && ScanOrder_Temp!=Retrieve(Stream_Video, StreamPos_To, Video_ScanOrder)) || !Retrieve(Stream_Video, StreamPos_To, Video_ScanType_Original).empty()))
        {
            Fill(Stream_Video, StreamPos_To, Video_ScanOrder_Original, (*Stream)[Stream_Video][StreamPos_To][Video_ScanOrder], true);
            if (ScanOrder_Temp.empty())
            {
                Clear(Stream_Video, StreamPos_To, Video_ScanOrder);
                Clear(Stream_Video, StreamPos_To, Video_ScanOrder_String);
            }
            else
                Fill(Stream_Video, StreamPos_To, Video_ScanOrder, ScanOrder_Temp, true);
        }
        if (!ColorSpace_Temp.empty() && ColorSpace_Temp!=Retrieve(Stream_Video, StreamPos_To, Video_ColorSpace) && ColorSpace_Temp.rfind(Retrieve(Stream_Video, StreamPos_To, Video_ColorSpace), 0) && Retrieve(Stream_Video, StreamPos_To, Video_ColorSpace).rfind(ColorSpace_Temp, 0)) // e.g. RGB ICC for RGBA
        {
            Fill(Stream_Video, StreamPos_To, "ColorSpace_Original", Retrieve(StreamKind, StreamPos_To, Video_ColorSpace), true);
            Fill(Stream_Video, StreamPos_To, Video_ColorSpace, ColorSpace_Temp, true);
        }
        Ztring HDR_ToAdd[Video_HDR_Format_Compatibility-Video_HDR_Format+1];
        for (size_t i=Video_HDR_Format; i<=Video_HDR_Format_Compatibility; i++)
            HDR_ToAdd[i-Video_HDR_Format]=ToAdd.Retrieve_Const(Stream_Video, StreamPos_From, i);
        Merge_HDR(StreamPos_To, HDR_Temp, HDR_ToAdd, ToAdd.Retrieve_Const(Stream_Video, StreamPos_From, Video_colour_range), ToAdd.Retrieve_Const(Stream_Video, StreamPos_From, Video_matrix_coefficients));
    }
    if (StreamKind==Stream_Audio)
    {
        const Ztring& StreamChannelLayout=ToAdd.Retrieve_Const(Stream_Audio, StreamPos_From, Audio_ChannelLayout);
        if (ToAdd.Retrieve_Const(Stream_Audio, StreamPos_From, Audio_Format)==__T("DTS") && StreamChannelLayout.find(__T(" Tfl Tfr Tbl Tbr"))!=string::npos)
        {
            Fill(Stream_Audio, StreamPos_To, Audio_Channel_s_, ToAdd.Retrieve_Const(Stream_Audio, StreamPos_From, Audio_Channel_s_), true);
            Fill(Stream_Audio, StreamPos_To, Audio_ChannelLayout, StreamChannelLayout, true);
            Fill(Stream_Audio, StreamPos_To, Audio_ChannelPositions, ToAdd.Retrieve_Const(Stream_Audio, StreamPos_From, Audio_ChannelPositions), true);
            Fill(Stream_Audio, StreamPos_To, Audio_ChannelPositions_String2, ToAdd.Retrieve_Const(Stream_Audio, StreamPos_From, Audio_ChannelPositions_String2), true);
        }
        else
        {
            bool IsOkGlobal=true;
            static audio AudioField[4]={ Audio_Channel_s_, Audio_ChannelLayout, Audio_ChannelPositions, Audio_ChannelPositions_String2 };
            for (size_t i=0; i<4; i++)
                if (!Channels_Temp[i].empty())
                {
                    //Test with legacy streams information
                    bool IsOk=(Channels_Temp[i]==Retrieve(Stream_Audio, StreamPos_To, AudioField[i]));
                    if (!IsOk)
                    {
                        ZtringList Temp; Temp.Separator_Set(0, __T(" / "));
                        Temp.Write(Retrieve(Stream_Audio, StreamPos_To, AudioField[i]));
                        for (size_t Pos=0; Pos<Temp.size(); Pos++)
                            if (Channels_Temp[i]==Temp[Pos])
                                IsOk=true;
                    }

                    //Special case with AES3: wrong container information is accepted
                    if (!IsOk && Retrieve(Stream_Audio, StreamPos_To, Audio_MuxingMode).find(__T("SMPTE ST 337"))!=string::npos)
                        IsOk=true;

                    if (!IsOk)
                        IsOkGlobal=false;
                }

            if (!IsOkGlobal)
                for (size_t i=0; i<4; i++)
                    if (Channels_Temp[i]!=(*Stream)[Stream_Audio][StreamPos_To][AudioField[i]])
                    {
                        string Original=Retrieve_Const(Stream_Audio, StreamPos_To, AudioField[i], Info_Name).To_UTF8();
                        size_t Original_Insert=Original.find('/');
                        if (Original_Insert==(size_t)-1)
                            Original_Insert=Original.size();
                        Original.insert(Original_Insert, "_Original");
                        Fill(Stream_Audio, StreamPos_To, Original.c_str(), (*Stream)[Stream_Audio][StreamPos_To][AudioField[i]], true);
                        Fill_SetOptions(Stream_Audio, StreamPos_To, Original.c_str(), Retrieve_Const(Stream_Audio, StreamPos_To, AudioField[i], Info_Options).To_UTF8().c_str());
                        Fill(Stream_Audio, StreamPos_To, AudioField[i], Channels_Temp[i], true);
                    }
        }
    }
    if (StreamKind==Stream_Text)
    {
        if (!FrameRate_Temp.empty())
        {
            const Ztring& FramesPerContainerBlock=Retrieve(Stream_Text, StreamPos_To, "FramesPerContainerBlock");
            if (!FramesPerContainerBlock.empty())
                FrameRate_Temp.From_Number(FrameRate_Temp.To_float64()*FramesPerContainerBlock.To_float64());
        }
        if ((!FrameRate_Temp.empty() && FrameRate_Temp!=Retrieve(Stream_Text, StreamPos_To, Text_FrameRate))
         || (!FrameRate_Num_Temp.empty() && FrameRate_Num_Temp!=Retrieve(Stream_Text, StreamPos_To, Text_FrameRate_Num))
         || (!FrameRate_Den_Temp.empty() && FrameRate_Den_Temp!=Retrieve(Stream_Text, StreamPos_To, Text_FrameRate_Den)))
        {
            Fill(Stream_Text, StreamPos_To, Text_FrameRate_Original, ToAdd.Retrieve(Stream_Text, StreamPos_To, Text_FrameRate), true);
            Fill(Stream_Text, StreamPos_To, Text_FrameRate_Original_Num, ToAdd.Retrieve(Stream_Text, StreamPos_To, Text_FrameRate_Num), true);
            Fill(Stream_Text, StreamPos_To, Text_FrameRate_Original_Den, ToAdd.Retrieve(Stream_Text, StreamPos_To, Text_FrameRate_Den), true);
            Fill(Stream_Text, StreamPos_To, Text_FrameRate, FrameRate_Temp, true);
            Fill(Stream_Text, StreamPos_To, Text_FrameRate_Num, FrameRate_Num_Temp, true);
            Fill(Stream_Text, StreamPos_To, Text_FrameRate_Den, FrameRate_Den_Temp, true);
        }
        if (!FrameRate_Mode_Temp.empty() && FrameRate_Mode_Temp!=Retrieve(Stream_Text, StreamPos_To, Text_FrameRate_Mode))
        {
            Fill(Stream_Text, StreamPos_To, Text_FrameRate_Mode_Original, (*Stream)[Stream_Text][StreamPos_To][Text_FrameRate_Mode], true);
            Fill(Stream_Text, StreamPos_To, Text_FrameRate_Mode, FrameRate_Mode_Temp, true);
        }
    }
    if (StreamKind==Stream_Image)
    {
        if (!ColorSpace_Temp.empty() && ColorSpace_Temp!=Retrieve(Stream_Image, StreamPos_To, Image_ColorSpace) && ColorSpace_Temp.rfind(Retrieve(Stream_Image, StreamPos_To, Image_ColorSpace), 0) && Retrieve(Stream_Image, StreamPos_To, Image_ColorSpace).rfind(ColorSpace_Temp, 0)) // e.g. RGB ICC for RGBA
        {
            Fill(Stream_Image, StreamPos_To, "ColorSpace_Original", Retrieve(StreamKind, StreamPos_To, Image_ColorSpace), true);
            Fill(Stream_Image, StreamPos_To, Image_ColorSpace, ColorSpace_Temp, true);
        }
        Ztring ColorSpace_ICC=Retrieve(Stream_Image, StreamPos_To, "ColorSpace_ICC");
        if (!ColorSpace_Temp.empty() && ColorSpace_Temp!=ColorSpace_ICC && (!ColorSpace_Temp.rfind(ColorSpace_ICC, 0) || !ColorSpace_ICC.rfind(ColorSpace_Temp, 0))) // e.g. RGB ICC for RGBA
        {
            Clear(Stream_Image, StreamPos_To, "ColorSpace_ICC");
        }
    }
    if (!Delay_Source_Temp.empty() && Delay_Source_Temp!=Retrieve(StreamKind, StreamPos_To, "Delay_Source"))
    {
        Fill(StreamKind, StreamPos_To, "Delay_Original", Retrieve(StreamKind, StreamPos_To, "Delay"), true);
        Fill(StreamKind, StreamPos_To, "Delay", Delay_Temp, true);
        Fill(StreamKind, StreamPos_To, "Delay_Original_Settings", Retrieve(StreamKind, StreamPos_To, "Delay_Settings"), true);
        Fill(StreamKind, StreamPos_To, "Delay_Settings", Delay_Settings_Temp, true);
        Fill(StreamKind, StreamPos_To, "Delay_Original_DropFrame", Retrieve(StreamKind, StreamPos_To, "Delay_DropFrame"), true);
        Fill(StreamKind, StreamPos_To, "Delay_DropFrame", Delay_DropFrame_Temp, true);
        Fill(StreamKind, StreamPos_To, "Delay_Original_Source", Retrieve(StreamKind, StreamPos_To, "Delay_Source"), true);
        Fill(StreamKind, StreamPos_To, "Delay_Source", Delay_Source_Temp, true);
    }
    if (!Source_Temp.empty() && Source_Temp!=Retrieve(StreamKind, StreamPos_To, "Source"))
    {
        Fill(StreamKind, StreamPos_To, "Source_Original", Retrieve(StreamKind, StreamPos_To, "Source"), true);
        Fill(StreamKind, StreamPos_To, "Source", Source_Temp, true);
        Fill(StreamKind, StreamPos_To, "Source_Original_Kind", Retrieve(StreamKind, StreamPos_To, "Source_Kind"), true);
        Fill(StreamKind, StreamPos_To, "Source_Kind", Source_Info_Temp, true);
        Fill(StreamKind, StreamPos_To, "Source_Original_Info", Retrieve(StreamKind, StreamPos_To, "Source_Info"), true);
        Fill(StreamKind, StreamPos_To, "Source_Info", Source_Info_Temp, true);
    }
    if (!BitRate_Temp.empty() && BitRate_Temp.find(__T(" / ")) == string::npos && Retrieve(StreamKind, StreamPos_To, "BitRate").find(__T("Unknown")) != string::npos)
    {
        Ztring Temp=Retrieve(StreamKind, StreamPos_To, "BitRate");
        Temp.FindAndReplace(__T("Unknown"), BitRate_Temp, 0, Ztring_Recursive);
        Fill(StreamKind, StreamPos_To, "BitRate", Temp, true);
    }
    const Ztring& CodecID_New =ToAdd.Retrieve_Const(StreamKind, StreamPos_From, "CodecID");
    if (!CodecID_Temp.empty() && !CodecID_New.empty() && CodecID_Temp!=CodecID_New && (Config->File_IsReferenced_Get() ^ !ToAdd.Config->File_IsReferenced_Get())) //TODO: better handling of merges, avoiding duplicate merges so we can remeove hack CodecID_Temp!=CodecID_New
    {
        Fill(StreamKind, StreamPos_To, "CodecID", CodecID_Temp+__T('-')+ToAdd.Retrieve(StreamKind, StreamPos_From, "CodecID"), true);
    }
    const Ztring& Title_New=ToAdd.Retrieve_Const(StreamKind, StreamPos_From, "Title");
    if (StreamKind!=Stream_General && !Title_Temp.empty() && !Title_New.empty() && Title_Temp!=Title_New)
    {
        Title_Temp+=__T(" - ");
        if (Title_New.compare(0, Title_Temp.size(), Title_Temp)) //For a master file with title referencing a essence file with title and stream title, we check that master file title is not the essence file title (not same due to stream title)
            Fill(StreamKind, StreamPos_To, "Title", Title_Temp+Title_New, true);
    }

    Fill(StreamKind, StreamPos_To, (size_t)General_Count, Count_Get(StreamKind, StreamPos_To), 10, true);
    return 1;
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void File__Analyze::Merge_HDR(size_t StreamPos_To, const Ztring* Container, const Ztring* ToAdd, const Ztring& ToAdd_colour_range, const Ztring& ToAdd_matrix_coefficients)
{
    ZtringList Container_Formats, Stream_Formats;
    Container_Formats.Separator_Set(0, __T(" / "));
    Stream_Formats.Separator_Set(0, __T(" / "));
    Container_Formats.Write(Container[0]);
    Stream_Formats.Write(ToAdd[0]);
    auto Container_DolbyVision=Container_Formats.Find(__T("Dolby Vision"));
    auto Stream_DolbyVision=Stream_Formats.Find(__T("Dolby Vision"));
    if (!Container[0].empty() && !ToAdd[0].empty() && (Container[0]!=ToAdd[0] || (Container_DolbyVision!=Error && Stream_DolbyVision!=Error)))
    {
        //The container has priority, the stream adds the HDR formats which are not in the container
        ZtringList Container_Values[Video_HDR_Format_Compatibility-Video_HDR_Format+1], Stream_Values[Video_HDR_Format_Compatibility-Video_HDR_Format+1];
        for (size_t i=Video_HDR_Format; i<=Video_HDR_Format_Compatibility; i++)
        {
            auto& Container_Value=Container_Values[i-Video_HDR_Format];
            auto& Stream_Value=Stream_Values[i-Video_HDR_Format];
            Container_Value.Separator_Set(0, __T(" / "));
            Stream_Value.Separator_Set(0, __T(" / "));
            Container_Value.Write(Container[i-Video_HDR_Format]);
            Stream_Value.Write(ToAdd[i-Video_HDR_Format]);
            Container_Value.resize(Container_Formats.size());
            Stream_Value.resize(Stream_Formats.size());
        }

        //If all HDR formats of the container are in the stream, the stream values are kept
        auto Container_InStream=true;
        for (const auto& Container_Format : Container_Formats)
            if (Stream_Formats.Find(Container_Format)==Error)
                Container_InStream=false;

        if (Container_DolbyVision!=Error && Stream_DolbyVision!=Error)
        {
            //Dolby Vision in both container configuration and stream RPU, the configuration has priority and the RPU adds what is not in the configuration
            Container_InStream=false;

            #if MEDIAINFO_CONFORMANCE
                const auto Profile_Number=[](const Ztring& Profile) {
                    auto Dot=Profile.find(__T('.'));
                    return Dot==string::npos?Ztring():Ztring::ToZtring(Ztring(Profile.substr(Dot+1)).To_int8u());
                };
                const auto Layers_HasEL=[](const Ztring& Layers) {
                    ZtringList List;
                    List.Separator_Set(0, __T("+"));
                    List.Write(Layers);
                    return List.Find(__T("EL"))!=Error || List.Find(__T("MEL"))!=Error || List.Find(__T("FEL"))!=Error;
                };
                const auto Container_Profile=Profile_Number(Container_Values[Video_HDR_Format_Profile-Video_HDR_Format][Container_DolbyVision]);
                const auto Stream_Profile=Profile_Number(Stream_Values[Video_HDR_Format_Profile-Video_HDR_Format][Stream_DolbyVision]);
                Ztring Container_Profile_RPU; //Profile with the signature expected in the RPU, empty if unknown
                switch (Container_Profile.To_int8u())
                {
                    case  4:
                    case  5:
                    case  7:
                    case  8:
                    case 10: Container_Profile_RPU=Container_Profile; break;
                    case 20: Container_Profile_RPU=Container_Values[Video_HDR_Format_Compatibility-Video_HDR_Format][Container_DolbyVision].empty()?__T("5"):__T("8"); break; //MV-HEVC, the RPU has the signature of the single view equivalent
                    default:;
                }
                if (!Container_Profile_RPU.empty() && !Stream_Profile.empty() && Container_Profile_RPU!=Stream_Profile)
                    Fill_Conformance("Crosscheck DolbyVisionConfiguration dv_profile", ("Dolby Vision configuration dv_profile " + Container_Profile.To_UTF8() + " does not match Dolby Vision RPU profile " + Stream_Profile.To_UTF8()).c_str(), {}, Conformance_Error, Stream_Video, StreamPos_To);
                const auto& Container_Layers=Container_Values[Video_HDR_Format_Settings-Video_HDR_Format][Container_DolbyVision];
                const auto& Stream_Layers=Stream_Values[Video_HDR_Format_Settings-Video_HDR_Format][Stream_DolbyVision];
                if (!Container_Layers.empty() && !Stream_Layers.empty() && Layers_HasEL(Container_Layers)!=Layers_HasEL(Stream_Layers))
                    Fill_Conformance("Crosscheck DolbyVisionConfiguration el_present_flag", (string("Dolby Vision configuration el_present_flag ") + (Layers_HasEL(Container_Layers)?'1':'0') + " does not match Dolby Vision RPU " + (Layers_HasEL(Stream_Layers)?"with":"without") + " enhancement layer").c_str(), {}, Conformance_Error, Stream_Video, StreamPos_To);
            #endif //MEDIAINFO_CONFORMANCE

            for (size_t i=Video_HDR_Format; i<=Video_HDR_Format_Compatibility; i++)
            {
                if (i==Video_HDR_Format_String || i==Video_HDR_Format_Commercial)
                    continue;
                auto& Container_DolbyVision_Value=Container_Values[i-Video_HDR_Format][Container_DolbyVision];
                const auto& Stream_DolbyVision_Value=Stream_Values[i-Video_HDR_Format][Stream_DolbyVision];
                if (Container_DolbyVision_Value.empty())
                    Container_DolbyVision_Value=Stream_DolbyVision_Value;
                else if (i==Video_HDR_Format_Settings && !Stream_DolbyVision_Value.empty())
                {
                    //Enhancement layer type is known only from the RPU
                    ZtringList Container_Layers, Stream_Layers;
                    Container_Layers.Separator_Set(0, __T("+"));
                    Stream_Layers.Separator_Set(0, __T("+"));
                    Container_Layers.Write(Container_DolbyVision_Value);
                    Stream_Layers.Write(Stream_DolbyVision_Value);
                    auto EL_Pos=Container_Layers.Find(__T("EL"));
                    if (EL_Pos!=Error)
                        for (const auto& Stream_Layer : Stream_Layers)
                            if (Stream_Layer==__T("MEL") || Stream_Layer==__T("FEL"))
                                Container_Layers[EL_Pos]=Stream_Layer;
                    Container_DolbyVision_Value=Container_Layers.Read();
                }
            }
        }

        if (!Container_InStream)
        {
            for (size_t i=Video_HDR_Format; i<=Video_HDR_Format_Compatibility; i++)
            {
                if (i==Video_HDR_Format_String || i==Video_HDR_Format_Commercial)
                    continue;
                auto& Container_Value=Container_Values[i-Video_HDR_Format];
                const auto& Stream_Value=Stream_Values[i-Video_HDR_Format];
                for (size_t j=0; j<Stream_Formats.size(); j++)
                    if (Container_Formats.Find(Stream_Formats[j])==Error)
                        Container_Value.push_back(Stream_Value[j]);
                auto Value=Container_Value.Read();
                if (Value.size()>(Container_Value.size()-1)*3)
                    Fill(Stream_Video, StreamPos_To, i, Value, true);
            }
        }
    }
    #if MEDIAINFO_CONFORMANCE
        if (Container_DolbyVision!=Error)
        {
            ZtringList Container_Profiles;
            Container_Profiles.Separator_Set(0, __T(" / "));
            Container_Profiles.Write(Container[Video_HDR_Format_Profile-Video_HDR_Format]);
            Container_Profiles.resize(Container_Formats.size());
            const auto& Container_Profile=Container_Profiles[Container_DolbyVision];
            if (Container_Profile.size()>=3 && !Container_Profile.compare(Container_Profile.size()-3, 3, __T(".05")))
            {
                //Profile 5 VUI is either 1,2,2,2,0 or 1,9,16,15,0 (video_full_range_flag, colour_primaries, transfer_characteristics, matrix_coeffs, chroma_sample_loc_type)
                if (ToAdd_colour_range==__T("Limited"))
                    Fill_Conformance("Crosscheck DolbyVisionConfiguration dv_profile", "Dolby Vision configuration dv_profile 5 does not permit HEVC VUI video_full_range_flag 0", {}, Conformance_Error, Stream_Video, StreamPos_To);
                if (!ToAdd_matrix_coefficients.empty() && ToAdd_matrix_coefficients!=__T("IPT-PQ-C2"))
                    Fill_Conformance("Crosscheck DolbyVisionConfiguration dv_profile", ("Dolby Vision configuration dv_profile 5 does not permit HEVC VUI matrix_coeffs " + ToAdd_matrix_coefficients.To_UTF8()).c_str(), {}, Conformance_Error, Stream_Video, StreamPos_To);
            }
        }
    #endif //MEDIAINFO_CONFORMANCE
}

//---------------------------------------------------------------------------
float64 File__Analyze::Video_FrameRate_Rounded(float64 FrameRate)
{
    float64 FrameRate_Sav=FrameRate;

         if (FrameRate> 9.990 && FrameRate<=10.010) FrameRate=10.000;
    else if (FrameRate>11.984 && FrameRate<=11.994) FrameRate=11.988;
    else if (FrameRate>11.994 && FrameRate<=12.010) FrameRate=12.000;
    else if (FrameRate>14.980 && FrameRate<=14.990) FrameRate=14.985;
    else if (FrameRate>14.990 && FrameRate<=15.010) FrameRate=15.000;
    else if (FrameRate>23.952 && FrameRate<=23.988) FrameRate=23.976;
    else if (FrameRate>23.988 && FrameRate<=24.024) FrameRate=24.000;
    else if (FrameRate>24.975 && FrameRate<=25.025) FrameRate=25.000;
    else if (FrameRate>29.940 && FrameRate<=29.985) FrameRate=29.970;
    else if (FrameRate>29.970 && FrameRate<=30.030) FrameRate=30.000;
    else if (FrameRate>23.952*2 && FrameRate<=23.988*2) FrameRate=23.976*2;
    else if (FrameRate>23.988*2 && FrameRate<=24.024*2) FrameRate=24.000*2;
    else if (FrameRate>24.975*2 && FrameRate<=25.025*2) FrameRate=25.000*2;
    else if (FrameRate>29.940*2 && FrameRate<=29.985*2) FrameRate=29.970*2;
    else if (FrameRate>29.970*2 && FrameRate<=30.030*2) FrameRate=30.000*2;

    if (std::fabs(FrameRate-FrameRate_Sav)>=0.000999999)
        return FrameRate;
    else
        return FrameRate_Sav;
}

//---------------------------------------------------------------------------
void File__Analyze::Video_FrameRate_Rounding(stream_t StreamKind, size_t Pos, size_t Parameter)
{
    const Ztring Value=Retrieve_Const(StreamKind, Pos, Parameter);
    float64 FrameRate=Video_FrameRate_Rounded(Value.To_float64());
    float64 FrameRate_Sav=Value.To_float64();

    if (FrameRate!=FrameRate_Sav)
        Fill(StreamKind, Pos, Parameter, FrameRate, 3, true);
}

//---------------------------------------------------------------------------
void File__Analyze::Video_BitRate_Rounding(size_t Pos, video Parameter)
{
    if (Video_BitRate_Rounding_Disabled)
        return;

    const Ztring& Format=Retrieve(Stream_Video, Pos, Video_Format);
    int32u BitRate=Retrieve(Stream_Video, Pos, Parameter).To_int32u();
    int32u BitRate_Sav=BitRate;
    if (Format==__T("AVC"))
    {
        if (BitRate>= 54942720 && BitRate<= 57185280) BitRate= 56064000; //AVC-INTRA50
        if (BitRate>=111390720 && BitRate<=115937280) BitRate=113664000; //AVC-INTRA100
    }

    if (BitRate!=BitRate_Sav)
        Fill(Stream_Video, Pos, Parameter, BitRate, 0, true);
}

//---------------------------------------------------------------------------
void File__Analyze::Audio_BitRate_Rounding(size_t Pos, audio Parameter)
{
    const Ztring& Format=Retrieve(Stream_Audio, Pos, Audio_Format);
    int32u BitRate=Retrieve(Stream_Audio, Pos, Parameter).To_int32u();
    int32u BitRate_Sav=BitRate;
    if (Format.find(__T("MPEG"))==0)
    {
        if (BitRate>=   7500 && BitRate<=   8500) BitRate=   8000;
        if (BitRate>=  15000 && BitRate<=  17000) BitRate=  16000;
        if (BitRate>=  23000 && BitRate<=  25000) BitRate=  24000;
        if (BitRate>=  31000 && BitRate<=  33000) BitRate=  32000;
        if (BitRate>=  38000 && BitRate<=  42000) BitRate=  40000;
        if (BitRate>=  46000 && BitRate<=  50000) BitRate=  48000;
        if (BitRate>=  54000 && BitRate<=  58000) BitRate=  56000;
        if (BitRate>=  62720 && BitRate<=  65280) BitRate=  64000;
        if (BitRate>=  78400 && BitRate<=  81600) BitRate=  80000;
        if (BitRate>=  94080 && BitRate<=  97920) BitRate=  96000;
        if (BitRate>= 109760 && BitRate<= 114240) BitRate= 112000;
        if (BitRate>= 125440 && BitRate<= 130560) BitRate= 128000;
        if (BitRate>= 156800 && BitRate<= 163200) BitRate= 160000;
        if (BitRate>= 188160 && BitRate<= 195840) BitRate= 192000;
        if (BitRate>= 219520 && BitRate<= 228480) BitRate= 224000;
        if (BitRate>= 219520 && BitRate<= 228480) BitRate= 224000;
        if (BitRate>= 250880 && BitRate<= 261120) BitRate= 256000;
        if (BitRate>= 282240 && BitRate<= 293760) BitRate= 288000;
        if (BitRate>= 313600 && BitRate<= 326400) BitRate= 320000;
        if (BitRate>= 344960 && BitRate<= 359040) BitRate= 352000;
        if (BitRate>= 376320 && BitRate<= 391680) BitRate= 384000;
        if (BitRate>= 407680 && BitRate<= 424320) BitRate= 416000;
        if (BitRate>= 439040 && BitRate<= 456960) BitRate= 448000;
        if (Retrieve(Stream_Audio, Pos, "BitRate_Mode")==__T("VBR"))
            BitRate=BitRate_Sav; //If VBR, we want the exact value
    }

    else if (Format.find(__T("AC3"))==0)
    {
        if (BitRate>=  31000 && BitRate<=  33000) BitRate=  32000;
        if (BitRate>=  39000 && BitRate<=  41000) BitRate=  40000;
        if (BitRate>=  46000 && BitRate<=  50000) BitRate=  48000;
        if (BitRate>=  54000 && BitRate<=  58000) BitRate=  56000;
        if (BitRate>=  62720 && BitRate<=  65280) BitRate=  64000;
        if (BitRate>=  78400 && BitRate<=  81600) BitRate=  80000;
        if (BitRate>=  94080 && BitRate<=  97920) BitRate=  96000;
        if (BitRate>= 109760 && BitRate<= 114240) BitRate= 112000;
        if (BitRate>= 125440 && BitRate<= 130560) BitRate= 128000;
        if (BitRate>= 156800 && BitRate<= 163200) BitRate= 160000;
        if (BitRate>= 188160 && BitRate<= 195840) BitRate= 192000;
        if (BitRate>= 219520 && BitRate<= 228480) BitRate= 224000;
        if (BitRate>= 250880 && BitRate<= 261120) BitRate= 256000;
        if (BitRate>= 313600 && BitRate<= 326400) BitRate= 320000;
        if (BitRate>= 376320 && BitRate<= 391680) BitRate= 384000;
        if (BitRate>= 439040 && BitRate<= 456960) BitRate= 448000;
        if (BitRate>= 501760 && BitRate<= 522240) BitRate= 512000;
        if (BitRate>= 564480 && BitRate<= 587520) BitRate= 576000;
        if (BitRate>= 627200 && BitRate<= 652800) BitRate= 640000;
    }

    else if (Format.find(__T("AAC"))==0)
    {
        if (BitRate>=  46000 && BitRate<=  50000) BitRate=  48000;
        if (BitRate>=  64827 && BitRate<=  67473) BitRate=  66150;
        if (BitRate>=  70560 && BitRate<=  73440) BitRate=  72000;
        if (BitRate>=  94080 && BitRate<=  97920) BitRate=  96000;
        if (BitRate>= 129654 && BitRate<= 134946) BitRate= 132300;
        if (BitRate>= 141120 && BitRate<= 146880) BitRate= 144000;
        if (BitRate>= 188160 && BitRate<= 195840) BitRate= 192000;
        if (BitRate>= 259308 && BitRate<= 269892) BitRate= 264600;
        if (BitRate>= 282240 && BitRate<= 293760) BitRate= 288000;
        if (BitRate>= 345744 && BitRate<= 359856) BitRate= 352800;
        if (BitRate>= 376320 && BitRate<= 391680) BitRate= 384000;
        if (BitRate>= 518616 && BitRate<= 539784) BitRate= 529200;
        if (BitRate>= 564480 && BitRate<= 587520) BitRate= 576000;
        if (BitRate>= 648270 && BitRate<= 674730) BitRate= 661500;
    }

    else if (Format==__T("PCM") || Format==__T("Qdesign 2"))
    {
        if (BitRate>=  62720 && BitRate<=  65280) BitRate=  64000;
        if (BitRate>=  86436 && BitRate<=  89964) BitRate=  88200;
        if (BitRate>= 125440 && BitRate<= 130560) BitRate= 128000;
        if (BitRate>= 172872 && BitRate<= 179928) BitRate= 176400;
        if (BitRate>= 188160 && BitRate<= 195840) BitRate= 192000;
        if (BitRate>= 250880 && BitRate<= 261120) BitRate= 256000;
        if (BitRate>= 345744 && BitRate<= 359856) BitRate= 352800;
        if (BitRate>= 376320 && BitRate<= 391680) BitRate= 384000;
        if (BitRate>= 501760 && BitRate<= 522240) BitRate= 512000;
        if (BitRate>= 691488 && BitRate<= 719712) BitRate= 705600;
        if (BitRate>= 752640 && BitRate<= 783360) BitRate= 768000;
        if (BitRate>=1003520 && BitRate<=1044480) BitRate=1024000;
        if (BitRate>=1128960 && BitRate<=1175040) BitRate=1152000;
        if (BitRate>=1382976 && BitRate<=1439424) BitRate=1411200;
        if (BitRate>=1505280 && BitRate<=1566720) BitRate=1536000;
        if (BitRate>=4515840 && BitRate<=4700160) BitRate=4608000;
        if (BitRate>=6021120 && BitRate<=6266880) BitRate=6144000;
    }

    else if (Format.find(__T("ADPCM"))==0
          || Format.find(__T("U-Law"))==0)
    {
        if (BitRate>=  42000 && BitRate<=  46000) BitRate=  44100;
        if (BitRate>=  62720 && BitRate<=  65280) BitRate=  64000;
        if (BitRate>=  86436 && BitRate<=  89964) BitRate=  88200;
        if (BitRate>= 125440 && BitRate<= 130560) BitRate= 128000;
        if (BitRate>= 172872 && BitRate<= 179928) BitRate= 176400;
        if (BitRate>= 188160 && BitRate<= 195840) BitRate= 192000;
        if (BitRate>= 250880 && BitRate<= 261120) BitRate= 256000;
        if (BitRate>= 345744 && BitRate<= 359856) BitRate= 352800;
        if (BitRate>= 376320 && BitRate<= 391680) BitRate= 384000;
    }

    if (BitRate!=BitRate_Sav)
        Fill(Stream_Audio, Pos, Parameter, BitRate, 0, true);
}

//---------------------------------------------------------------------------
void File__Analyze::Tags()
{
    //Integrity
    if (!Count_Get(Stream_General))
        return;

    //-Movie/Album
    if (!Retrieve(Stream_General, 0, General_Title).empty() && Retrieve(Stream_General, 0, General_Movie).empty() && Retrieve(Stream_General, 0, General_Track).empty())
    {
        if (Count_Get(Stream_Video) && Retrieve(Stream_General, 0, General_Collection).empty())
            Fill(Stream_General, 0, General_Movie, Retrieve(Stream_General, 0, General_Title));
        else if (Count_Get(Stream_Audio))
            Fill(Stream_General, 0, General_Track, Retrieve(Stream_General, 0, General_Title));
    }
    if (!Retrieve(Stream_General, 0, General_Title_More).empty() && Retrieve(Stream_General, 0, General_Movie_More).empty() && Retrieve(Stream_General, 0, General_Track_More).empty())
    {
        if (Count_Get(Stream_Video) && Retrieve(Stream_General, 0, General_Collection).empty())
            Fill(Stream_General, 0, General_Movie_More, Retrieve(Stream_General, 0, General_Title_More));
        else if (Count_Get(Stream_Audio))
            Fill(Stream_General, 0, General_Track_More, Retrieve(Stream_General, 0, General_Title_More));
    }
    if (!Retrieve(Stream_General, 0, General_Title_Url).empty() && Retrieve(Stream_General, 0, General_Movie_Url).empty() && Retrieve(Stream_General, 0, General_Track_Url).empty())
    {
        if (Count_Get(Stream_Video) && Retrieve(Stream_General, 0, General_Collection).empty())
            Fill(Stream_General, 0, General_Movie_Url, Retrieve(Stream_General, 0, General_Title_Url));
        else if (Count_Get(Stream_Audio))
            Fill(Stream_General, 0, General_Track_Url, Retrieve(Stream_General, 0, General_Title_Url));
    }
    if (!Retrieve(Stream_General, 0, General_Country).empty() && Retrieve(Stream_General, 0, General_Movie_Country).empty())
    {
        if (Count_Get(Stream_Video) && Retrieve(Stream_General, 0, General_Collection).empty())
            Fill(Stream_General, 0, General_Movie_Country, Retrieve(Stream_General, 0, General_Country));
    }

    //-Title
    if (Retrieve(Stream_General, 0, General_Title).empty() && !Retrieve(Stream_General, 0, General_Movie).empty())
        Fill(Stream_General, 0, General_Title, Retrieve(Stream_General, 0, General_Movie));
    if (Retrieve(Stream_General, 0, General_Title).empty() && !Retrieve(Stream_General, 0, General_Track).empty())
        Fill(Stream_General, 0, General_Title, Retrieve(Stream_General, 0, General_Track));
    if (Retrieve(Stream_General, 0, General_Title_More).empty() && !Retrieve(Stream_General, 0, General_Movie_More).empty())
        Fill(Stream_General, 0, General_Title_More, Retrieve(Stream_General, 0, General_Movie_More));
    if (Retrieve(Stream_General, 0, General_Title_More).empty() && !Retrieve(Stream_General, 0, General_Track_More).empty())
        Fill(Stream_General, 0, General_Title_More, Retrieve(Stream_General, 0, General_Track_More));
    if (Retrieve(Stream_General, 0, General_Title_Url).empty() && !Retrieve(Stream_General, 0, General_Movie_Url).empty())
        Fill(Stream_General, 0, General_Title_Url, Retrieve(Stream_General, 0, General_Movie_Url));
    if (Retrieve(Stream_General, 0, General_Title_Url).empty() && !Retrieve(Stream_General, 0, General_Track_Url).empty())
        Fill(Stream_General, 0, General_Title_Url, Retrieve(Stream_General, 0, General_Track_Url));

    //-Genre
    if (!Retrieve(Stream_General, 0, General_Genre).empty() && Retrieve(Stream_General, 0, General_Genre).size()<4 && Retrieve(Stream_General, 0, General_Genre)[0]>=__T('0') && Retrieve(Stream_General, 0, General_Genre)[0]<=__T('9'))
    {
        Ztring Genre;
        if (Retrieve(Stream_General, 0, General_Genre).size()==1) Genre=Ztring(__T("Genre_00"))+Retrieve(Stream_General, 0, General_Genre);
        if (Retrieve(Stream_General, 0, General_Genre).size()==2) Genre=Ztring(__T("Genre_0" ))+Retrieve(Stream_General, 0, General_Genre);
        if (Retrieve(Stream_General, 0, General_Genre).size()==3) Genre=Ztring(__T("Genre_"  ))+Retrieve(Stream_General, 0, General_Genre);
        Fill(Stream_General, 0, "Genre", MediaInfoLib::Config.Language_Get(Genre), true);
    }
}

//***************************************************************************
// Internal Functions
//***************************************************************************

//---------------------------------------------------------------------------
//Duration
void File__Analyze::Duration_Duration123(stream_t StreamKind, size_t StreamPos, size_t Parameter)
{
    if (Retrieve(StreamKind, StreamPos, Parameter).empty()
     || (StreamKind==Stream_Audio && (Parameter==Audio_Interleave_Duration || Parameter==Audio_Interleave_Preload))) //Exception: string is built also from frame rate, already computed. TODO: check behavior with MIXML input
        return;

    //Clearing old data
    Clear(StreamKind, StreamPos, Parameter+1);
    Clear(StreamKind, StreamPos, Parameter+2);
    Clear(StreamKind, StreamPos, Parameter+3);
    Clear(StreamKind, StreamPos, Parameter+4);
    Clear(StreamKind, StreamPos, Parameter+5);
    Clear(StreamKind, StreamPos, Parameter+6);

    //Retrieving multiple values
    ZtringList List;
    List.Separator_Set(0, __T(" / "));
    List.Write(Retrieve(StreamKind, StreamPos, Parameter));

    //Per value
    for (size_t Pos=0; Pos<List.size(); Pos++)
    {
        int64s HH, MS;
        int32s MM, Sec;
        Ztring DurationString1, DurationString2, DurationString3;
        bool Negative=false;
        if (List[Pos].find_first_not_of(__T("0123456789.+-"))!=string::npos)
        {
            Fill(StreamKind, StreamPos, Parameter+1, List[Pos]); // /String
            continue;
        }
        MS=List[Pos].To_int64s(); //in ms
        if (MS<0)
        {
            Negative=true;
            MS=-MS;
        }

        //Hours
        HH=MS/1000/60/60; //h
        if (HH>0)
        {
            DurationString1+=Ztring::ToZtring(HH)+MediaInfoLib::Config.Language_Get(__T("h"));
            DurationString2+=Ztring::ToZtring(HH)+MediaInfoLib::Config.Language_Get(__T("h"));
            if (HH<10)
                DurationString3+=Ztring(__T("0"))+Ztring::ToZtring(HH)+__T(":");
            else
                DurationString3+=Ztring::ToZtring(HH)+__T(":");
            MS-=HH*60*60*1000;
        }
        else
        {
            DurationString3+=__T("00:");
        }

        //Minutes
        MM=(int32s)(MS/1000/60); //mn
        if (MM>0 || HH>0)
        {
            if (DurationString1.size()>0)
                DurationString1+=__T(" ");
            DurationString1+=Ztring::ToZtring(MM)+MediaInfoLib::Config.Language_Get(__T("mn"));
            if (DurationString2.size()<5)
            {
                if (DurationString2.size()>0)
                    DurationString2+=__T(" ");
                DurationString2+=Ztring::ToZtring(MM)+MediaInfoLib::Config.Language_Get(__T("mn"));
            }
            if (MM<10)
                DurationString3+=Ztring(__T("0"))+Ztring::ToZtring(MM)+__T(":");
            else
                DurationString3+=Ztring::ToZtring(MM)+__T(":");
            MS-=MM*60*1000;
        }
        else
        {
            DurationString3+=__T("00:");
        }

        //Seconds
        Sec=(int32s)(MS/1000); //s
        if (Sec>0 || MM>0 || HH>0)
        {
            if (DurationString1.size()>0)
                DurationString1+=__T(" ");
            DurationString1+=Ztring::ToZtring(Sec)+MediaInfoLib::Config.Language_Get(__T("s"));
            if (HH==0)
            {
                if (DurationString2.size()>0)
                    DurationString2+=__T(" ");
                DurationString2+=Ztring::ToZtring(Sec)+MediaInfoLib::Config.Language_Get(__T("s"));
            }
            if (Sec<10)
                DurationString3+=Ztring(__T("0"))+Ztring::ToZtring(Sec)+__T(".");
            else
                DurationString3+=Ztring::ToZtring(Sec)+__T(".");
            MS-=Sec*1000;
        }
        else
        {
            DurationString3+=__T("00.");
        }

        //Milliseconds
        if (MS>0 || Sec>0 || MM>0 || HH>0)
        {
            if (DurationString1.size()>0)
                DurationString1+=__T(" ");
            DurationString1+=Ztring::ToZtring(MS)+MediaInfoLib::Config.Language_Get(__T("ms"));
            if (HH==0 && MM==0)
            {
                if (DurationString2.size()>0)
                    DurationString2+=__T(" ");
                DurationString2+=Ztring::ToZtring(MS)+MediaInfoLib::Config.Language_Get(__T("ms"));
            }
            if (MS<10)
                DurationString3+=Ztring(__T("00"))+Ztring::ToZtring(MS);
            else if (MS<100)
                DurationString3+=Ztring(__T("0"))+Ztring::ToZtring(MS);
            else
                DurationString3+=Ztring::ToZtring(MS);
        }
        else
        {
            DurationString3+=__T("000");
        }

        if (Negative)
        {
            DurationString1=Ztring(__T("-"))+DurationString1;
            DurationString2=Ztring(__T("-"))+DurationString2;
            DurationString3=Ztring(__T("-"))+DurationString3;
        }

        Fill(StreamKind, StreamPos, Parameter+1, DurationString2); // /String
        Fill(StreamKind, StreamPos, Parameter+2, DurationString1); // /String1
        Fill(StreamKind, StreamPos, Parameter+3, DurationString2); // /String2
        Fill(StreamKind, StreamPos, Parameter+4, DurationString3); // /String3

        if (HH<=0xFF && Retrieve_Const(StreamKind, StreamPos, Parameter+6, Info_Name)==Retrieve_Const(StreamKind, StreamPos, Parameter, Info_Name)+__T("/String5"))
        {
            Ztring DurationString4;
            Ztring FrameRateS;
            if (FrameRateS.empty() && StreamKind==Stream_Audio)
                FrameRateS=Retrieve(Stream_Audio, StreamPos, "TimeCode_FirstFrame_FrameRate");
            if (FrameRateS.empty() && StreamKind==Stream_Audio)
                FrameRateS=Retrieve(Stream_Audio, StreamPos, "Dolby_Atmos_Metadata AssociatedVideo_FrameRate");
            if (FrameRateS.empty() && StreamKind!=Stream_Audio)
                FrameRateS=Retrieve(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_FrameRate));
            float64 FrameRateF=FrameRateS.To_float64();
            int64s FrameRateI=float64_int64s(FrameRateF);
            Ztring FrameCountS;
            if (Parameter==Fill_Parameter(StreamKind, Generic_Duration))
                FrameCountS=Retrieve(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_FrameCount));
            if (FrameCountS.empty() || StreamKind==Stream_Text || (StreamKind==Stream_Audio && Retrieve(Stream_Audio, StreamPos, Stream_Audio)!=__T("PCM")))
            {
                //FrameCount is not based on frame rate
                FrameCountS.From_Number(List[Pos].To_float32()*Video_FrameRate_Rounded(FrameRateF)/1000, 0);
            }
            if (!FrameRateS.empty() && !FrameCountS.empty() && FrameRateI && FrameRateI<256)
            {
                bool DropFrame=false;
                bool DropFrame_IsValid=false;

                // Testing delay
                if (!DropFrame_IsValid)
                {
                    Ztring TC=Retrieve(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Delay_Original_DropFrame));
                    if (TC.size()>=11 && TC[2]==__T(':') && TC[5]==__T(':'))
                    {
                        switch (TC[8])
                        {
                            case __T(':'):
                                            DropFrame=false;
                                            DropFrame_IsValid=true;
                                            break;
                            case __T(';'):
                                            DropFrame=true;
                                            DropFrame_IsValid=true;
                                            break;
                            default      :  ;
                        }
                    }
                }

                // Testing time code track
                if (!DropFrame_IsValid)
                {
                    for (size_t Step=Retrieve(Stream_General, 0, General_Format)==__T("MXF")?0:1; Step<2; ++Step)
                    {
                        for (size_t TC_Pos=0; TC_Pos<Count_Get(Stream_Other); ++TC_Pos)
                            if (Retrieve(Stream_Other, TC_Pos, Other_Type)==__T("Time code")
                             && (Step || Retrieve(Stream_Other, TC_Pos, Other_TimeCode_Settings)==__T("Source Package")))
                            {
                                Ztring TC=Retrieve(Stream_Other, TC_Pos, Other_TimeCode_FirstFrame);
                                if (TC.size()>=11 && TC[2]==__T(':') && TC[5]==__T(':'))
                                {
                                    switch (TC[8])
                                    {
                                        case __T(':'):
                                                        DropFrame=false;
                                                        DropFrame_IsValid=true;
                                                        break;
                                        case __T(';'):
                                                        DropFrame=true;
                                                        DropFrame_IsValid=true;
                                                        break;
                                        default      :  ;
                                    }
                                }

                                if (DropFrame_IsValid)
                                    break; //Using first time code track
                            }

                        if (DropFrame_IsValid)
                            break; //Using first time code track
                    }
                }

                // Testing time code
                if (!DropFrame_IsValid)
                {
                    Ztring TC=Retrieve(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_TimeCode_FirstFrame));
                    if (TC.size()>=11 && TC[2]==__T(':') && TC[5]==__T(':'))
                    {
                        switch (TC[8])
                        {
                            case __T(':'):
                                            DropFrame=false;
                                            DropFrame_IsValid=true;
                                            break;
                            case __T(';'):
                                            DropFrame=true;
                                            DropFrame_IsValid=true;
                                            break;
                            default      :  ;
                        }
                    }
                }

                // Value from another stream
                if (!DropFrame_IsValid)
                {
                    bool DropFrame_AlreadyThere_IsValid=false;
                    bool DropFrame_AlreadyThere;
                    for (size_t i=Stream_General; i<Stream_Max; i++)
                    {
                        size_t Count=Count_Get((stream_t)i);
                        for (size_t j=0; j<Count; j++)
                        {
                            Ztring TC=Retrieve((stream_t)i, j, Fill_Parameter((stream_t)i, Generic_TimeCode_FirstFrame));
                            if (TC.size()>=11 && TC[2]==__T(':') && TC[5]==__T(':'))
                            {
                                switch (TC[8])
                                {
                                    case __T(':'):
                                                    DropFrame=false;
                                                    DropFrame_IsValid=true;
                                                    break;
                                    case __T(';'):
                                                    DropFrame=true;
                                                    DropFrame_IsValid=true;
                                                    break;
                                    default      :  ;
                                }
                            }
                            if (!DropFrame_IsValid)
                                continue;
                            if (DropFrame_AlreadyThere_IsValid)
                            {
                                if (DropFrame_AlreadyThere==DropFrame)
                                    continue;
                                DropFrame_IsValid=false;
                                i=Stream_Max;
                                break;
                            }
                            DropFrame_AlreadyThere_IsValid=true;
                            DropFrame_AlreadyThere=DropFrame;
                        }
                    }
                }

                // Testing frame rate (1/1001)
                if (!DropFrame_IsValid)
                {
                    int32s  FrameRateI=float32_int32s(FrameRateS.To_float32());
                    if (!(FrameRateI%30))
                    {
                        float32 FrameRateF=FrameRateS.To_float32();
                        float FrameRateF_Min=((float32)FrameRateI)/((float32)1.002);
                        float FrameRateF_Max=(float32)FrameRateI;
                        if (FrameRateF>=FrameRateF_Min && FrameRateF<FrameRateF_Max)
                        {
                            // Default from user
                            #if MEDIAINFO_ADVANCED
                                switch (Config->File_DefaultTimeCodeDropFrame_Get())
                                {
                                    case 0 :
                                            DropFrame=false;
                                            break;
                                    default:
                                            DropFrame=true;
                                }
                            #else //MEDIAINFO_ADVANCED
                                DropFrame=true;
                            #endif //MEDIAINFO_ADVANCED
                        }
                        else
                            DropFrame=false;
                    }
                    else
                        DropFrame=false;
                }

                TimeCode TC((int64_t)FrameCountS.To_int64s(), (uint32_t)float32_int32s(FrameRateS.To_float32()-1), TimeCode::DropFrame(DropFrame).FPS1001(FrameRateI!=FrameRateF));
                DurationString4.From_UTF8(TC.ToString());

                Fill(StreamKind, StreamPos, Parameter+5, DurationString4); // /String4
            }
            Ztring DurationString5(DurationString3);
            if (!DurationString4.empty())
            {
                DurationString5+=__T(' ');
                DurationString5+=__T('(');
                DurationString5+=DurationString4;
                DurationString5+=__T(')');
            }
            Fill(StreamKind, StreamPos, Parameter+6, DurationString5); // /String5
        }
    }
}

//---------------------------------------------------------------------------
//FileSize
void File__Analyze::FileSize_FileSize123(stream_t StreamKind, size_t StreamPos, size_t Parameter)
{
    if (Retrieve(StreamKind, StreamPos, Parameter).empty())
        return;

    float F1=(float)Retrieve(StreamKind, StreamPos, Parameter).To_int64s(); //Video C++ 6 patch, should be int64u

    //--Bytes, KiB, MiB or GiB...
    int32u Pow3=0;
    while(F1>=1024)
    {
        F1/=1024;
        Pow3++;
    }
    //--Count of digits
    int8u I2, I3, I4;
         if (F1>=100)
    {
        I2=0;
        I3=0;
        I4=1;
    }
    else if (F1>=10)
    {
        I2=0;
        I3=1;
        I4=2;
    }
    else //if (F1>=1)
    {
        I2=1;
        I3=2;
        I4=3;
    }
    Ztring Measure; bool MeasureIsAlwaysSame;
    switch (Pow3)
    {
        case  0 : Measure=__T(" Byte"); MeasureIsAlwaysSame=false; break;
        case  1 : Measure=__T(" KiB");  MeasureIsAlwaysSame=true;  break;
        case  2 : Measure=__T(" MiB");  MeasureIsAlwaysSame=true;  break;
        case  3 : Measure=__T(" GiB");  MeasureIsAlwaysSame=true;  break;
        case  4 : Measure=__T(" TiB");  MeasureIsAlwaysSame=true;  break;
        case  5 : Measure=__T(" PiB");  MeasureIsAlwaysSame=true;  break;
        case  6 : Measure=__T(" EiB");  MeasureIsAlwaysSame=true;  break;
        default : Measure=__T(" ?iB");  MeasureIsAlwaysSame=true;
    }
    Fill(StreamKind, StreamPos, Parameter+2, MediaInfoLib::Config.Language_Get(Ztring::ToZtring(F1,  0), Measure, MeasureIsAlwaysSame), true); // /String1
    Fill(StreamKind, StreamPos, Parameter+3, MediaInfoLib::Config.Language_Get(Ztring::ToZtring(F1, I2), Measure, MeasureIsAlwaysSame), true); // /String2
    Fill(StreamKind, StreamPos, Parameter+4, MediaInfoLib::Config.Language_Get(Ztring::ToZtring(F1, I3), Measure, MeasureIsAlwaysSame), true); // /String3
    Fill(StreamKind, StreamPos, Parameter+5, MediaInfoLib::Config.Language_Get(Ztring::ToZtring(F1, I4), Measure, MeasureIsAlwaysSame), true); // /String4
    float64 F2=(float)Retrieve(StreamKind, StreamPos, Parameter).To_float64();
    float64 File_Size_WithReferencedFiles=(float)Retrieve(Stream_General, 0, General_FileSize).To_float64();
    if (File_Size_WithReferencedFiles>0 && Parameter==Fill_Parameter(StreamKind, Generic_StreamSize) && F2*100/File_Size_WithReferencedFiles<=100)
    {
        Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_StreamSize_Proportion), F2/File_Size_WithReferencedFiles, 5, true);
        Fill(StreamKind, StreamPos, Parameter+6, MediaInfoLib::Config.Language_Get(Ztring::ToZtring(F1, I3), Measure, MeasureIsAlwaysSame)+__T(" (")+Ztring::ToZtring(F2*100/File_Size_WithReferencedFiles, 0)+__T("%)"), true); // /String5
        Fill(StreamKind, StreamPos, Parameter+1,  MediaInfoLib::Config.Language_Get(Ztring::ToZtring(F1, I3), Measure, MeasureIsAlwaysSame)+__T(" (")+Ztring::ToZtring(F2*100/File_Size_WithReferencedFiles, 0)+__T("%)"), true);
    }
    else if (File_Size_WithReferencedFiles>0 && Parameter==Fill_Parameter(StreamKind, Generic_StreamSize_Encoded) && F2*100/File_Size_WithReferencedFiles<=100)
    {
        Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_StreamSize_Encoded_Proportion), F2/File_Size_WithReferencedFiles, 5, true);
        Fill(StreamKind, StreamPos, Parameter+6, MediaInfoLib::Config.Language_Get(Ztring::ToZtring(F1, I3), Measure, MeasureIsAlwaysSame)+__T(" (")+Ztring::ToZtring(F2*100/File_Size_WithReferencedFiles, 0)+__T("%)"), true); // /String5
        Fill(StreamKind, StreamPos, Parameter+1,  MediaInfoLib::Config.Language_Get(Ztring::ToZtring(F1, I3), Measure, MeasureIsAlwaysSame)+__T(" (")+Ztring::ToZtring(F2*100/File_Size_WithReferencedFiles, 0)+__T("%)"), true);
    }
    else if (File_Size_WithReferencedFiles>0 && Parameter==Fill_Parameter(StreamKind, Generic_Source_StreamSize) && F2*100/File_Size_WithReferencedFiles<=100)
    {
        Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Source_StreamSize_Proportion), F2/File_Size_WithReferencedFiles, 5, true);
        Fill(StreamKind, StreamPos, Parameter+6, MediaInfoLib::Config.Language_Get(Ztring::ToZtring(F1, I3), Measure, MeasureIsAlwaysSame)+__T(" (")+Ztring::ToZtring(F2*100/File_Size_WithReferencedFiles, 0)+__T("%)"), true); // /String5
        Fill(StreamKind, StreamPos, Parameter+1,  MediaInfoLib::Config.Language_Get(Ztring::ToZtring(F1, I3), Measure, MeasureIsAlwaysSame)+__T(" (")+Ztring::ToZtring(F2*100/File_Size_WithReferencedFiles, 0)+__T("%)"), true);
    }
    else if (File_Size_WithReferencedFiles>0 && Parameter==Fill_Parameter(StreamKind, Generic_Source_StreamSize_Encoded) && F2*100/File_Size_WithReferencedFiles<=100)
    {
        Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Source_StreamSize_Encoded_Proportion), F2/File_Size_WithReferencedFiles, 5, true);
        Fill(StreamKind, StreamPos, Parameter+6, MediaInfoLib::Config.Language_Get(Ztring::ToZtring(F1, I3), Measure, MeasureIsAlwaysSame)+__T(" (")+Ztring::ToZtring(F2*100/File_Size_WithReferencedFiles, 0)+__T("%)"), true); // /String5
        Fill(StreamKind, StreamPos, Parameter+1,  MediaInfoLib::Config.Language_Get(Ztring::ToZtring(F1, I3), Measure, MeasureIsAlwaysSame)+__T(" (")+Ztring::ToZtring(F2*100/File_Size_WithReferencedFiles, 0)+__T("%)"), true);
    }
    else
        Fill(StreamKind, StreamPos, Parameter+1,  MediaInfoLib::Config.Language_Get(Ztring::ToZtring(F1, I3), Measure, MeasureIsAlwaysSame), true);
}

//---------------------------------------------------------------------------
//FileSize
void File__Analyze::Kilo_Kilo123(stream_t StreamKind, size_t StreamPos, size_t Parameter)
{
    if (Retrieve(StreamKind, StreamPos, Parameter).empty())
        return;

    //Clearing old data
    Clear(StreamKind, StreamPos, Parameter+1);

    //Retrieving multiple values
    ZtringList List;
    List.Separator_Set(0, __T(" / "));
    List.Write(Retrieve(StreamKind, StreamPos, Parameter));
    ZtringList List2;
    List2.Separator_Set(0, __T(" / "));

    //Per value
    for (size_t Pos=0; Pos<List.size(); Pos++)
    {
        int64u BitRate=List[Pos].To_int64u();

        //Text
        if (BitRate==0 && (List[Pos].empty() || List[Pos][0]>__T('9')))
        {
            List2.push_back(MediaInfoLib::Config.Language_Get(List[Pos]));
        }
        else
        {
            //Well known values
            Ztring BitRateS;
            if (StreamKind==Stream_Audio)
            {
                if (Parameter==Audio_BitRate
                 && (Retrieve(Stream_Audio, StreamPos, Audio_Format)==__T("PCM")
                  || Retrieve(Stream_Audio, StreamPos, Audio_Format)==__T("ADPCM")
                  || Retrieve(Stream_Audio, StreamPos, Audio_Format)==__T("U-Law")
                  || Retrieve(Stream_Audio, StreamPos, Audio_Format)==__T("Qdesign 1")
                  || Retrieve(Stream_Audio, StreamPos, Audio_Format)==__T("Qdesign 2")
                  || Retrieve(Stream_Audio, StreamPos, Audio_Format)==__T("DTS")))
                {
                    if (BitRate==  66150) BitRateS=  "66.15";
                    if (BitRate== 132300) BitRateS= "132.3";
                    if (BitRate== 176400) BitRateS= "176.4";
                    if (BitRate== 264600) BitRateS= "264.6";
                    if (BitRate== 352800) BitRateS= "352.8";
                    if (BitRate== 529200) BitRateS= "529.2";
                    if (BitRate== 705600) BitRateS= "705.6";
                    if (BitRate==1411200) BitRateS="1411.2";
                }
                if (Parameter==Audio_SamplingRate)
                {
                    if (BitRate==  11024) BitRateS=  "11.024";
                    if (BitRate==  11025) BitRateS=  "11.025";
                    if (BitRate==  22050) BitRateS=  "22.05";
                    if (BitRate==  44100) BitRateS=  "44.1";
                    if (BitRate==  88200) BitRateS=  "88.2";
                    if (BitRate== 176400) BitRateS= "176.4";
                    if (BitRate== 352800) BitRateS= "352.8";
                }
            }
            if (!BitRateS.empty())
            {
                Ztring Measure=MediaInfoLib::Config.Info_Get(StreamKind).Read(Parameter, Info_Measure);
                Measure.insert(1, __T("K"));
                List2.push_back(MediaInfoLib::Config.Language_Get(BitRateS, Measure, true));
            }
            else
            {
                //Standard
                if (BitRate>10000000000LL)
                {
                    Ztring Measure=MediaInfoLib::Config.Info_Get(StreamKind).Read(Parameter, Info_Measure);
                    Measure.insert(1, __T("G"));
                    List2.push_back(MediaInfoLib::Config.Language_Get(Ztring::ToZtring(((float)BitRate)/1000000000, BitRate>100000000000LL?0:1), Measure, true));
                }
                else if (BitRate>10000000)
                {
                    Ztring Measure=MediaInfoLib::Config.Info_Get(StreamKind).Read(Parameter, Info_Measure);
                    Measure.insert(1, __T("M"));
                    List2.push_back(MediaInfoLib::Config.Language_Get(Ztring::ToZtring(((float)BitRate)/1000000, BitRate>100000000?0:1), Measure, true));
                }
                else if (BitRate>10000)
                {
                    Ztring Measure=MediaInfoLib::Config.Info_Get(StreamKind).Read(Parameter, Info_Measure);
                    Measure.insert(1, __T("K"));
                    List2.push_back(MediaInfoLib::Config.Language_Get(Ztring::ToZtring(((float)BitRate)/1000, BitRate>100000?0:1), Measure, true));
                }
                else
                    List2.push_back(MediaInfoLib::Config.Language_Get(Ztring::ToZtring(BitRate), MediaInfoLib::Config.Info_Get(StreamKind).Read(Parameter, Info_Measure), true));
            }
        }
    }

    Fill(StreamKind, StreamPos, Parameter+1, List2.Read());
}

//---------------------------------------------------------------------------
//Value --> Value with measure
void File__Analyze::Value_Value123(stream_t StreamKind, size_t StreamPos, size_t Parameter)
{
    if (Retrieve(StreamKind, StreamPos, Parameter, Info_Measure).empty())
        return;

    //Special cases
    if (StreamKind==Stream_Audio && Parameter==Audio_BitDepth_Detected && Retrieve(Stream_Audio, StreamPos, Audio_BitDepth)==Retrieve(Stream_Audio, StreamPos, Audio_BitDepth_Detected))
        return;

    //Clearing old data
    Clear(StreamKind, StreamPos, Parameter+1);

    //Retrieving multiple values
    ZtringList List;
    List.Separator_Set(0, __T(" / "));
    List.Write(Retrieve(StreamKind, StreamPos, Parameter));
    ZtringList List2;
    List2.Separator_Set(0, __T(" / "));

    //Per value
    for (size_t Pos=0; Pos<List.size(); Pos++)
    {
        //Filling
        List2.push_back(MediaInfoLib::Config.Language_Get(List[Pos], MediaInfoLib::Config.Info_Get(StreamKind).Read(Parameter, Info_Measure)));

        //Special case : Audio Channels with ChannelMode
        if (StreamKind==Stream_Audio && Parameter==Audio_Channel_s_)
        {
            const Ztring& ChannelMode=Retrieve_Const(Stream_Audio, StreamPos, "ChannelMode");
            if (ChannelMode.size()>3 || (ChannelMode.size()==3 && ChannelMode[2]!=__T('0')))
            {
                List2[List2.size()-1]+=__T(" (");
                List2[List2.size()-1]+=ChannelMode;
                List2[List2.size()-1]+=__T(")");
            }
        }

        //Special cases - Frame rate
        auto FrameRate_Index=Fill_Parameter(StreamKind, Generic_FrameRate);
        if (Parameter==FrameRate_Index)
        {
            ZtringList Temp;
            Temp.Separator_Set(0, __T(" / "));
            Temp.Write(Retrieve_Const(StreamKind, StreamPos, FrameRate_Index));
            const auto& FrameRate=Temp.Read(List2.size()-1);
            const auto& FrameRate_Num=Retrieve_Const(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_FrameRate_Num));
            const auto& FrameRate_Den=Retrieve_Const(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_FrameRate_Den));
            if (!FrameRate.empty()
             && !FrameRate_Num.empty()
             && !FrameRate_Den.empty()
             && FrameRate_Den.To_int32u()!=1)
                List2[List2.size()-1]=MediaInfoLib::Config.Language_Get(FrameRate+__T(" (")+FrameRate_Num+__T("/")+FrameRate_Den+__T(")"), __T(" fps"));
        }
        if (StreamKind==Stream_Video
         && Parameter==Video_FrameRate_Original
         && !Retrieve(StreamKind, StreamPos, Video_FrameRate_Original).empty()
         && !Retrieve(StreamKind, StreamPos, Video_FrameRate_Original_Num).empty()
         && !Retrieve(StreamKind, StreamPos, Video_FrameRate_Original_Den).empty()
         && Retrieve(StreamKind, StreamPos, Video_FrameRate_Original_Den).To_int32u()!=1)
            List2[List2.size()-1]=MediaInfoLib::Config.Language_Get(Retrieve(StreamKind, StreamPos, Video_FrameRate_Original)+__T(" (")+Retrieve(StreamKind, StreamPos, Video_FrameRate_Original_Num)+__T("/")+Retrieve(StreamKind, StreamPos, Video_FrameRate_Original_Den)+__T(")"), __T(" fps"));
        if (StreamKind==Stream_Text
         && Parameter==Text_FrameRate_Original
         && !Retrieve(StreamKind, StreamPos, Text_FrameRate_Original).empty()
         && !Retrieve(StreamKind, StreamPos, Text_FrameRate_Original_Num).empty()
         && !Retrieve(StreamKind, StreamPos, Text_FrameRate_Original_Den).empty()
         && Retrieve(StreamKind, StreamPos, Text_FrameRate_Original_Den).To_int32u()!=1)
            List2[List2.size()-1]=MediaInfoLib::Config.Language_Get(Retrieve(StreamKind, StreamPos, Text_FrameRate_Original)+__T(" (")+Retrieve(StreamKind, StreamPos, Text_FrameRate_Original_Num)+__T("/")+Retrieve(StreamKind, StreamPos, Text_FrameRate_Original_Den)+__T(")"), __T(" fps"));

        //Special cases - 120 fps 24/30 mode
        if (StreamKind==Stream_Video
         && Parameter==Video_FrameRate
         && Retrieve(Stream_Video, StreamPos, Video_FrameRate).To_int32u()==120
         && Retrieve(Stream_Video, StreamPos, Video_FrameRate_Minimum).To_int32u()==24
         && Retrieve(Stream_Video, StreamPos, Video_FrameRate_Maximum).To_int32u()==30)
            List2[List2.size()-1]=MediaInfoLib::Config.Language_Get(Retrieve(Stream_Video, StreamPos, Video_FrameRate)+__T(" (24/30)"), __T(" fps"));

        //Special case : audio with samples per frames
        if (StreamKind==Stream_Audio && Parameter==Audio_FrameRate)
        {
            const Ztring &SamplesPerFrame = Retrieve(Stream_Audio, StreamPos, Audio_SamplesPerFrame);
            if (!SamplesPerFrame.empty())
            {
                ZtringList SamplesPerFrame_List;
                SamplesPerFrame_List.Separator_Set(0, __T(" / "));
                SamplesPerFrame_List.Write(SamplesPerFrame);
                size_t i=List2.size()-1;
                while (List2.size()<SamplesPerFrame_List.size())
                    List2.push_back(List2[List2.size()-1]);
                while (SamplesPerFrame_List.size()<List2.size())
                    SamplesPerFrame_List.push_back(SamplesPerFrame_List[SamplesPerFrame_List.size()-1]);
                for (; i<List2.size(); i++)
                {
                    List2[i]+=__T(" (");
                    List2[i]+=SamplesPerFrame_List[i];
                    List2[i]+=__T(" SPF)");
                }
            }
        }
    }

    Fill(StreamKind, StreamPos, Parameter+1, List2.Read(), true);
}

//---------------------------------------------------------------------------
//Value --> Yes or No
void File__Analyze::YesNo_YesNo(stream_t StreamKind, size_t StreamPos, size_t Parameter)
{
    //Filling
    Fill(StreamKind, StreamPos, Parameter+1, MediaInfoLib::Config.Language_Get(Retrieve(StreamKind, StreamPos, Parameter)), true);
}

//---------------------------------------------------------------------------
void File__Analyze::CodecID_Fill(const Ztring &Value, stream_t StreamKind, size_t StreamPos, infocodecid_format_t Format, stream_t StreamKind_CodecID)
{
    if (StreamKind_CodecID==Stream_Max)
        StreamKind_CodecID=StreamKind;

    Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_CodecID), Value);
    const Ztring &C1=MediaInfoLib::Config.CodecID_Get(StreamKind_CodecID, Format, Value, InfoCodecID_Format);
    if (!C1.empty())
        Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Format), C1, true);
    Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_CodecID_Info), MediaInfoLib::Config.CodecID_Get(StreamKind_CodecID, Format, Value, InfoCodecID_Description), true);
    Fill(StreamKind, StreamPos, "CodecID/Hint", MediaInfoLib::Config.CodecID_Get(StreamKind_CodecID, Format, Value, InfoCodecID_Hint), true);
    Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_CodecID_Url), MediaInfoLib::Config.CodecID_Get(StreamKind_CodecID, Format, Value, InfoCodecID_Url), true);
    Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Format_Version), MediaInfoLib::Config.CodecID_Get(StreamKind_CodecID, Format, Value, InfoCodecID_Version), true);
    Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Format_Profile), MediaInfoLib::Config.CodecID_Get(StreamKind_CodecID, Format, Value, InfoCodecID_Profile), true);
    Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_ColorSpace), MediaInfoLib::Config.CodecID_Get(StreamKind_CodecID, Format, Value, InfoCodecID_ColorSpace), true);
    Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_ChromaSubsampling), MediaInfoLib::Config.CodecID_Get(StreamKind_CodecID, Format, Value, InfoCodecID_ChromaSubsampling), true);
    if (Retrieve(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_BitDepth)).empty() && !MediaInfoLib::Config.CodecID_Get(StreamKind_CodecID, Format, Value, InfoCodecID_BitDepth).empty())
        Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_BitDepth), MediaInfoLib::Config.CodecID_Get(StreamKind_CodecID, Format, Value, InfoCodecID_BitDepth), true);
    if (Retrieve(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Compression_Mode)).empty())
        Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Compression_Mode), MediaInfoLib::Config.CodecID_Get(StreamKind_CodecID, Format, Value, InfoCodecID_Compression_Mode), true);

    //Specific cases
    if (Value==__T("v210") || Value==__T("V210"))
        Fill(Stream_Video, StreamPos, Video_BitDepth, 10);
}

//---------------------------------------------------------------------------
void File__Analyze::PixelAspectRatio_Fill(const Ztring &Value, stream_t StreamKind, size_t StreamPos, size_t Parameter_Width, size_t Parameter_Height, size_t Parameter_PixelAspectRatio, size_t Parameter_DisplayAspectRatio)
{
    if (Value.empty() || !Retrieve(StreamKind, StreamPos, Parameter_DisplayAspectRatio).empty())
        return;

    if (Retrieve(StreamKind, StreamPos, Parameter_DisplayAspectRatio).empty())
    {
        //TODO: trash this crazy test after PAR is in num/den format.
        float32 PAR=Value.To_float32();
        if (PAR>(float32)12/(float32)11*0.999 && PAR<(float32)12/(float32)11*1.001)
            PAR=(float32)12/(float32)11;
        if (PAR>(float32)10/(float32)11*0.999 && PAR<(float32)10/(float32)11*1.001)
            PAR=(float32)10/(float32)11;
        if (PAR>(float32)16/(float32)11*0.999 && PAR<(float32)16/(float32)11*1.001)
            PAR=(float32)16/(float32)11;
        if (PAR>(float32)16/(float32)15*0.999 && PAR<(float32)16/(float32)15*1.001)
            PAR=(float32)16/(float32)15;
        if (PAR>(float32)40/(float32)33*0.999 && PAR<(float32)40/(float32)33*1.001)
            PAR=(float32)40/(float32)33;
        if (PAR>(float32)24/(float32)11*0.999 && PAR<(float32)24/(float32)11*1.001)
            PAR=(float32)24/(float32)11;
        if (PAR>(float32)20/(float32)11*0.999 && PAR<(float32)20/(float32)11*1.001)
            PAR=(float32)20/(float32)11;
        if (PAR>(float32)32/(float32)11*0.999 && PAR<(float32)32/(float32)11*1.001)
            PAR=(float32)32/(float32)11;
        if (PAR>(float32)80/(float32)33*0.999 && PAR<(float32)80/(float32)33*1.001)
            PAR=(float32)80/(float32)33;
        if (PAR>(float32)18/(float32)11*0.999 && PAR<(float32)18/(float32)11*1.001)
            PAR=(float32)18/(float32)11;
        if (PAR>(float32)15/(float32)11*0.999 && PAR<(float32)15/(float32)11*1.001)
            PAR=(float32)15/(float32)11;
        if (PAR>(float32)64/(float32)33*0.999 && PAR<(float32)64/(float32)33*1.001)
            PAR=(float32)64/(float32)33;
        if (PAR>(float32)160/(float32)99*0.999 && PAR<(float32)160/(float32)99*1.001)
            PAR=(float32)160/(float32)99;
        if (PAR>(float32)4/(float32)3*0.999 && PAR<(float32)4/(float32)3*1.01)
            PAR=(float32)4/(float32)3;
        if (PAR>(float32)3/(float32)2*0.999 && PAR<(float32)3/(float32)2*1.001)
            PAR=(float32)3/(float32)2;
        if (PAR>(float32)2/(float32)1*0.999 && PAR<(float32)2/(float32)1*1.001)
            PAR=(float32)2;
        if (PAR>(float32)59/(float32)54*0.999 && PAR<(float32)59/(float32)54*1.001)
            PAR=(float32)59/(float32)54;
        if (PAR>(float32)8/(float32)9*0.999 && PAR<(float32)8/(float32)9*1.001)
            PAR=(float32)8/(float32)9;
        float32 Width =Retrieve(StreamKind, StreamPos, Parameter_Width             ).To_float32();
        float32 Height=Retrieve(StreamKind, StreamPos, Parameter_Height            ).To_float32();
        if (PAR && Height && Width)
            Fill(StreamKind, StreamPos, Parameter_DisplayAspectRatio, ((float32)Width)/Height*PAR);
    }
}

//---------------------------------------------------------------------------
void File__Analyze::DisplayAspectRatio_Fill(const Ztring &Value, stream_t StreamKind, size_t StreamPos, size_t Parameter_Width, size_t Parameter_Height, size_t Parameter_PixelAspectRatio, size_t Parameter_DisplayAspectRatio)
{
    if (Value.empty())
        return;

    float DAR=Value.To_float32();

    if (Retrieve(StreamKind, StreamPos, Parameter_PixelAspectRatio).empty())
    {
        float Width =Retrieve(StreamKind, StreamPos, Parameter_Width).To_float32();
        float Height=Retrieve(StreamKind, StreamPos, Parameter_Height).To_float32();
        if (DAR && Height && Width)
        {
            if (Value==__T("1.778"))
                DAR=((float)16)/9; //More exact value
            if (Value==__T("1.333"))
                DAR=((float)4)/3; //More exact value
            Fill(StreamKind, StreamPos, Parameter_PixelAspectRatio, DAR/(((float32)Width)/Height));
        }
    }

    // /String version
    Fill(StreamKind, StreamPos, Parameter_DisplayAspectRatio+1, DisplayAspectRatio_String(DAR), true);
}

//---------------------------------------------------------------------------
size_t File__Analyze::Fill_Parameter(stream_t StreamKind, generic StreamPos)
{
    switch (StreamKind)
    {
        case Stream_General :
                                switch (StreamPos)
                                {
                                    case Generic_Format : return General_Format;
                                    case Generic_Format_Info : return General_Format_Info;
                                    case Generic_Format_Url : return General_Format_Url;
                                    case Generic_Format_String : return General_Format_String;
                                    case Generic_Format_Version : return General_Format_Version;
                                    case Generic_Format_Commercial : return General_Format_Commercial;
                                    case Generic_Format_Commercial_IfAny : return General_Format_Commercial_IfAny;
                                    case Generic_Format_Profile : return General_Format_Profile;
                                    case Generic_Format_Level: return General_Format_Level;
                                    case Generic_Format_Settings : return General_Format_Settings;
                                    case Generic_Format_AdditionalFeatures : return General_Format_AdditionalFeatures;
                                    case Generic_InternetMediaType : return General_InternetMediaType;
                                    case Generic_CodecID : return General_CodecID;
                                    case Generic_CodecID_Info : return General_CodecID_Info;
                                    case Generic_CodecID_Hint : return General_CodecID_Hint;
                                    case Generic_CodecID_Url : return General_CodecID_Url;
                                    case Generic_CodecID_Description : return General_CodecID_Description;
                                    case Generic_Codec : return General_Codec;
                                    case Generic_Codec_String : return General_Codec_String;
                                    case Generic_Codec_Info : return General_Codec_Info;
                                    case Generic_Codec_Url : return General_Codec_Url;
                                    case Generic_Duration : return General_Duration;
                                    case Generic_Duration_String : return General_Duration_String;
                                    case Generic_Duration_String1 : return General_Duration_String1;
                                    case Generic_Duration_String2 : return General_Duration_String2;
                                    case Generic_Duration_String3 : return General_Duration_String3;
                                    case Generic_Duration_String4 : return General_Duration_String4;
                                    case Generic_Duration_String5 : return General_Duration_String5;
                                    case Generic_FrameRate : return General_FrameRate;
                                    case Generic_FrameRate_String: return General_FrameRate_String;
                                    case Generic_FrameRate_Num: return General_FrameRate_Num;
                                    case Generic_FrameRate_Den: return General_FrameRate_Den;
                                    case Generic_FrameCount : return General_FrameCount;
                                    case Generic_Delay : return General_Delay;
                                    case Generic_Delay_String : return General_Delay_String;
                                    case Generic_Delay_String1 : return General_Delay_String1;
                                    case Generic_Delay_String2 : return General_Delay_String2;
                                    case Generic_Delay_String3 : return General_Delay_String3;
                                    case Generic_Delay_String4 : return General_Delay_String4;
                                    case Generic_Delay_String5 : return General_Delay_String5;
                                    case Generic_Delay_Settings : return General_Delay_Settings;
                                    case Generic_Delay_DropFrame : return General_Delay_DropFrame;
                                    case Generic_Delay_Source : return General_Delay_Source;
                                    case Generic_Delay_Source_String : return General_Delay_Source_String;
                                    case Generic_StreamSize : return General_StreamSize;
                                    case Generic_StreamSize_String : return General_StreamSize_String;
                                    case Generic_StreamSize_String1 : return General_StreamSize_String1;
                                    case Generic_StreamSize_String2 : return General_StreamSize_String2;
                                    case Generic_StreamSize_String3 : return General_StreamSize_String3;
                                    case Generic_StreamSize_String4 : return General_StreamSize_String4;
                                    case Generic_StreamSize_String5 : return General_StreamSize_String5;
                                    case Generic_StreamSize_Proportion : return General_StreamSize_Proportion;
                                    case Generic_ServiceName : return General_ServiceName;
                                    case Generic_ServiceProvider : return General_ServiceProvider;
                                    default: return (size_t)-1;
                                }
        case Stream_Video :
                                switch (StreamPos)
                                {
                                    case Generic_Format : return Video_Format;
                                    case Generic_Format_Info : return Video_Format_Info;
                                    case Generic_Format_Url : return Video_Format_Url;
                                    case Generic_Format_String: return Video_Format_String;
                                    case Generic_Format_Commercial : return Video_Format_Commercial;
                                    case Generic_Format_Commercial_IfAny : return Video_Format_Commercial_IfAny;
                                    case Generic_Format_Version : return Video_Format_Version;
                                    case Generic_Format_Profile : return Video_Format_Profile;
                                    case Generic_Format_Level: return Video_Format_Level;
                                    case Generic_Format_Tier: return Video_Format_Tier;
                                    case Generic_Format_Settings : return Video_Format_Settings;
                                    case Generic_Format_AdditionalFeatures : return Video_Format_AdditionalFeatures;
                                    case Generic_InternetMediaType : return Video_InternetMediaType;
                                    case Generic_CodecID : return Video_CodecID;
                                    case Generic_CodecID_Info : return Video_CodecID_Info;
                                    case Generic_CodecID_Hint : return Video_CodecID_Hint;
                                    case Generic_CodecID_Url : return Video_CodecID_Url;
                                    case Generic_CodecID_Description : return Video_CodecID_Description;
                                    case Generic_Codec : return Video_Codec;
                                    case Generic_Codec_String : return Video_Codec_String;
                                    case Generic_Codec_Info : return Video_Codec_Info;
                                    case Generic_Codec_Url : return Video_Codec_Url;
                                    case Generic_Codec_CC : return Video_Codec_CC;
                                    case Generic_Width: return Video_Width;
                                    case Generic_Height: return Video_Height;
                                    case Generic_Duration : return Video_Duration;
                                    case Generic_Duration_String : return Video_Duration_String;
                                    case Generic_Duration_String1 : return Video_Duration_String1;
                                    case Generic_Duration_String2 : return Video_Duration_String2;
                                    case Generic_Duration_String3 : return Video_Duration_String3;
                                    case Generic_Duration_String4 : return Video_Duration_String4;
                                    case Generic_Duration_String5 : return Video_Duration_String5;
                                    case Generic_Source_Duration : return Video_Source_Duration;
                                    case Generic_Source_Duration_String : return Video_Source_Duration_String;
                                    case Generic_Source_Duration_String1 : return Video_Source_Duration_String1;
                                    case Generic_Source_Duration_String2 : return Video_Source_Duration_String2;
                                    case Generic_Source_Duration_String3 : return Video_Source_Duration_String3;
                                    case Generic_Source_Duration_String4 : return Video_Source_Duration_String4;
                                    case Generic_Source_Duration_String5 : return Video_Source_Duration_String5;
                                    case Generic_BitRate_Mode : return Video_BitRate_Mode;
                                    case Generic_BitRate_Mode_String : return Video_BitRate_Mode_String;
                                    case Generic_BitRate : return Video_BitRate;
                                    case Generic_BitRate_String : return Video_BitRate_String;
                                    case Generic_BitRate_Minimum : return Video_BitRate_Minimum;
                                    case Generic_BitRate_Minimum_String : return Video_BitRate_Minimum_String;
                                    case Generic_BitRate_Nominal : return Video_BitRate_Nominal;
                                    case Generic_BitRate_Nominal_String : return Video_BitRate_Nominal_String;
                                    case Generic_BitRate_Maximum : return Video_BitRate_Maximum;
                                    case Generic_BitRate_Maximum_String : return Video_BitRate_Maximum_String;
                                    case Generic_BitRate_Encoded : return Video_BitRate_Encoded;
                                    case Generic_BitRate_Encoded_String : return Video_BitRate_Encoded_String;
                                    case Generic_FrameRate : return Video_FrameRate;
                                    case Generic_FrameRate_String: return Video_FrameRate_String;
                                    case Generic_FrameRate_Num: return Video_FrameRate_Num;
                                    case Generic_FrameRate_Den: return Video_FrameRate_Den;
                                    case Generic_FrameCount : return Video_FrameCount;
                                    case Generic_Source_FrameCount : return Video_Source_FrameCount;
                                    case Generic_ColorSpace : return Video_ColorSpace;
                                    case Generic_ChromaSubsampling : return Video_ChromaSubsampling;
                                    case Generic_Resolution : return Video_Resolution;
                                    case Generic_Resolution_String : return Video_Resolution_String;
                                    case Generic_BitDepth : return Video_BitDepth;
                                    case Generic_BitDepth_String : return Video_BitDepth_String;
                                    case Generic_Compression_Mode : return Video_Compression_Mode;
                                    case Generic_Compression_Mode_String : return Video_Compression_Mode_String;
                                    case Generic_Compression_Ratio : return Video_Compression_Ratio;
                                    case Generic_Delay : return Video_Delay;
                                    case Generic_Delay_String : return Video_Delay_String;
                                    case Generic_Delay_String1 : return Video_Delay_String1;
                                    case Generic_Delay_String2 : return Video_Delay_String2;
                                    case Generic_Delay_String3 : return Video_Delay_String3;
                                    case Generic_Delay_String4 : return Video_Delay_String4;
                                    case Generic_Delay_String5 : return Video_Delay_String5;
                                    case Generic_Delay_Settings : return Video_Delay_Settings;
                                    case Generic_Delay_DropFrame : return Video_Delay_DropFrame;
                                    case Generic_Delay_Source : return Video_Delay_Source;
                                    case Generic_Delay_Source_String : return Video_Delay_Source_String;
                                    case Generic_Delay_Original : return Video_Delay_Original;
                                    case Generic_Delay_Original_String : return Video_Delay_Original_String;
                                    case Generic_Delay_Original_String1 : return Video_Delay_Original_String1;
                                    case Generic_Delay_Original_String2 : return Video_Delay_Original_String2;
                                    case Generic_Delay_Original_String3 : return Video_Delay_Original_String3;
                                    case Generic_Delay_Original_String4 : return Video_Delay_Original_String4;
                                    case Generic_Delay_Original_Settings : return Video_Delay_Original_Settings;
                                    case Generic_Delay_Original_DropFrame : return Video_Delay_Original_DropFrame;
                                    case Generic_Delay_Original_Source : return Video_Delay_Original_Source;
                                    case Generic_TimeCode_FirstFrame : return Video_TimeCode_FirstFrame;
                                    case Generic_TimeCode_DropFrame: return Video_TimeCode_DropFrame;
                                    case Generic_TimeCode_Settings: return Video_TimeCode_Settings;
                                    case Generic_TimeCode_Source: return Video_TimeCode_Source;
                                    case Generic_StreamSize : return Video_StreamSize;
                                    case Generic_StreamSize_String : return Video_StreamSize_String;
                                    case Generic_StreamSize_String1 : return Video_StreamSize_String1;
                                    case Generic_StreamSize_String2 : return Video_StreamSize_String2;
                                    case Generic_StreamSize_String3 : return Video_StreamSize_String3;
                                    case Generic_StreamSize_String4 : return Video_StreamSize_String4;
                                    case Generic_StreamSize_String5 : return Video_StreamSize_String5;
                                    case Generic_StreamSize_Proportion : return Video_StreamSize_Proportion;
                                    case Generic_StreamSize_Encoded : return Video_StreamSize_Encoded;
                                    case Generic_StreamSize_Encoded_String : return Video_StreamSize_Encoded_String;
                                    case Generic_StreamSize_Encoded_String1 : return Video_StreamSize_Encoded_String1;
                                    case Generic_StreamSize_Encoded_String2 : return Video_StreamSize_Encoded_String2;
                                    case Generic_StreamSize_Encoded_String3 : return Video_StreamSize_Encoded_String3;
                                    case Generic_StreamSize_Encoded_String4 : return Video_StreamSize_Encoded_String4;
                                    case Generic_StreamSize_Encoded_String5 : return Video_StreamSize_Encoded_String5;
                                    case Generic_StreamSize_Encoded_Proportion : return Video_StreamSize_Encoded_Proportion;
                                    case Generic_Source_StreamSize : return Video_Source_StreamSize;
                                    case Generic_Source_StreamSize_String : return Video_Source_StreamSize_String;
                                    case Generic_Source_StreamSize_String1 : return Video_Source_StreamSize_String1;
                                    case Generic_Source_StreamSize_String2 : return Video_Source_StreamSize_String2;
                                    case Generic_Source_StreamSize_String3 : return Video_Source_StreamSize_String3;
                                    case Generic_Source_StreamSize_String4 : return Video_Source_StreamSize_String4;
                                    case Generic_Source_StreamSize_String5 : return Video_Source_StreamSize_String5;
                                    case Generic_Source_StreamSize_Proportion : return Video_Source_StreamSize_Proportion;
                                    case Generic_Source_StreamSize_Encoded : return Video_Source_StreamSize_Encoded;
                                    case Generic_Source_StreamSize_Encoded_String : return Video_Source_StreamSize_Encoded_String;
                                    case Generic_Source_StreamSize_Encoded_String1 : return Video_Source_StreamSize_Encoded_String1;
                                    case Generic_Source_StreamSize_Encoded_String2 : return Video_Source_StreamSize_Encoded_String2;
                                    case Generic_Source_StreamSize_Encoded_String3 : return Video_Source_StreamSize_Encoded_String3;
                                    case Generic_Source_StreamSize_Encoded_String4 : return Video_Source_StreamSize_Encoded_String4;
                                    case Generic_Source_StreamSize_Encoded_String5 : return Video_Source_StreamSize_Encoded_String5;
                                    case Generic_Source_StreamSize_Encoded_Proportion : return Video_Source_StreamSize_Encoded_Proportion;
                                    case Generic_Language : return Video_Language;
                                    default: return (size_t)-1;
                                }
        case Stream_Audio :
                                switch (StreamPos)
                                {
                                    case Generic_Format : return Audio_Format;
                                    case Generic_Format_Info : return Audio_Format_Info;
                                    case Generic_Format_Url : return Audio_Format_Url;
                                    case Generic_Format_String : return Audio_Format_String;
                                    case Generic_Format_Commercial : return Audio_Format_Commercial;
                                    case Generic_Format_Commercial_IfAny : return Audio_Format_Commercial_IfAny;
                                    case Generic_Format_Version : return Audio_Format_Version;
                                    case Generic_Format_Profile : return Audio_Format_Profile;
                                    case Generic_Format_Level: return Audio_Format_Level;
                                    case Generic_Format_Settings : return Audio_Format_Settings;
                                    case Generic_Format_AdditionalFeatures : return Audio_Format_AdditionalFeatures;
                                    case Generic_InternetMediaType : return Audio_InternetMediaType;
                                    case Generic_CodecID : return Audio_CodecID;
                                    case Generic_CodecID_Info : return Audio_CodecID_Info;
                                    case Generic_CodecID_Hint : return Audio_CodecID_Hint;
                                    case Generic_CodecID_Url : return Audio_CodecID_Url;
                                    case Generic_CodecID_Description : return Audio_CodecID_Description;
                                    case Generic_Codec : return Audio_Codec;
                                    case Generic_Codec_String : return Audio_Codec_String;
                                    case Generic_Codec_Info : return Audio_Codec_Info;
                                    case Generic_Codec_Url : return Audio_Codec_Url;
                                    case Generic_Codec_CC : return Audio_Codec_CC;
                                    case Generic_Duration : return Audio_Duration;
                                    case Generic_Duration_String : return Audio_Duration_String;
                                    case Generic_Duration_String1 : return Audio_Duration_String1;
                                    case Generic_Duration_String2 : return Audio_Duration_String2;
                                    case Generic_Duration_String3 : return Audio_Duration_String3;
                                    case Generic_Duration_String4 : return Audio_Duration_String4;
                                    case Generic_Duration_String5 : return Audio_Duration_String5;
                                    case Generic_Source_Duration : return Audio_Source_Duration;
                                    case Generic_Source_Duration_String : return Audio_Source_Duration_String;
                                    case Generic_Source_Duration_String1 : return Audio_Source_Duration_String1;
                                    case Generic_Source_Duration_String2 : return Audio_Source_Duration_String2;
                                    case Generic_Source_Duration_String3 : return Audio_Source_Duration_String3;
                                    case Generic_Source_Duration_String4 : return Audio_Source_Duration_String4;
                                    case Generic_Source_Duration_String5 : return Audio_Source_Duration_String5;
                                    case Generic_BitRate_Mode : return Audio_BitRate_Mode;
                                    case Generic_BitRate_Mode_String : return Audio_BitRate_Mode_String;
                                    case Generic_BitRate : return Audio_BitRate;
                                    case Generic_BitRate_String : return Audio_BitRate_String;
                                    case Generic_BitRate_Minimum : return Audio_BitRate_Minimum;
                                    case Generic_BitRate_Minimum_String : return Audio_BitRate_Minimum_String;
                                    case Generic_BitRate_Nominal : return Audio_BitRate_Nominal;
                                    case Generic_BitRate_Nominal_String : return Audio_BitRate_Nominal_String;
                                    case Generic_BitRate_Maximum : return Audio_BitRate_Maximum;
                                    case Generic_BitRate_Maximum_String : return Audio_BitRate_Maximum_String;
                                    case Generic_BitRate_Encoded : return Audio_BitRate_Encoded;
                                    case Generic_BitRate_Encoded_String : return Audio_BitRate_Encoded_String;
                                    case Generic_FrameRate : return Audio_FrameRate;
                                    case Generic_FrameRate_String: return Audio_FrameRate_String;
                                    case Generic_FrameRate_Num: return Audio_FrameRate_Num;
                                    case Generic_FrameRate_Den: return Audio_FrameRate_Den;
                                    case Generic_FrameCount : return Audio_FrameCount;
                                    case Generic_Source_FrameCount : return Audio_Source_FrameCount;
                                    case Generic_Resolution : return Audio_Resolution;
                                    case Generic_Resolution_String : return Audio_Resolution_String;
                                    case Generic_BitDepth : return Audio_BitDepth;
                                    case Generic_BitDepth_String : return Audio_BitDepth_String;
                                    case Generic_Compression_Mode : return Audio_Compression_Mode;
                                    case Generic_Compression_Mode_String : return Audio_Compression_Mode_String;
                                    case Generic_Compression_Ratio : return Audio_Compression_Ratio;
                                    case Generic_Delay : return Audio_Delay;
                                    case Generic_Delay_String : return Audio_Delay_String;
                                    case Generic_Delay_String1 : return Audio_Delay_String1;
                                    case Generic_Delay_String2 : return Audio_Delay_String2;
                                    case Generic_Delay_String3 : return Audio_Delay_String3;
                                    case Generic_Delay_String4 : return Audio_Delay_String4;
                                    case Generic_Delay_String5 : return Audio_Delay_String5;
                                    case Generic_Delay_Settings : return Audio_Delay_Settings;
                                    case Generic_Delay_DropFrame : return Audio_Delay_DropFrame;
                                    case Generic_Delay_Source : return Audio_Delay_Source;
                                    case Generic_Delay_Source_String : return Audio_Delay_Source_String;
                                    case Generic_Delay_Original : return Audio_Delay_Original;
                                    case Generic_Delay_Original_String : return Audio_Delay_Original_String;
                                    case Generic_Delay_Original_String1 : return Audio_Delay_Original_String1;
                                    case Generic_Delay_Original_String2 : return Audio_Delay_Original_String2;
                                    case Generic_Delay_Original_String3 : return Audio_Delay_Original_String3;
                                    case Generic_Delay_Original_String4 : return Audio_Delay_Original_String4;
                                    case Generic_Delay_Original_Settings : return Audio_Delay_Original_Settings;
                                    case Generic_Delay_Original_DropFrame : return Audio_Delay_Original_DropFrame;
                                    case Generic_Delay_Original_Source : return Audio_Delay_Original_Source;
                                    case Generic_Video_Delay : return Audio_Video_Delay;
                                    case Generic_Video_Delay_String : return Audio_Video_Delay_String;
                                    case Generic_Video_Delay_String1 : return Audio_Video_Delay_String1;
                                    case Generic_Video_Delay_String2 : return Audio_Video_Delay_String2;
                                    case Generic_Video_Delay_String3 : return Audio_Video_Delay_String3;
                                    case Generic_Video_Delay_String4 : return Audio_Video_Delay_String4;
                                    case Generic_TimeCode_FirstFrame : return Audio_TimeCode_FirstFrame;
                                    case Generic_TimeCode_DropFrame: return Audio_TimeCode_DropFrame;
                                    case Generic_TimeCode_Settings: return Audio_TimeCode_Settings;
                                    case Generic_TimeCode_Source: return Audio_TimeCode_Source;
                                    case Generic_StreamSize : return Audio_StreamSize;
                                    case Generic_StreamSize_String : return Audio_StreamSize_String;
                                    case Generic_StreamSize_String1 : return Audio_StreamSize_String1;
                                    case Generic_StreamSize_String2 : return Audio_StreamSize_String2;
                                    case Generic_StreamSize_String3 : return Audio_StreamSize_String3;
                                    case Generic_StreamSize_String4 : return Audio_StreamSize_String4;
                                    case Generic_StreamSize_String5 : return Audio_StreamSize_String5;
                                    case Generic_StreamSize_Proportion : return Audio_StreamSize_Proportion;
                                    case Generic_StreamSize_Encoded : return Audio_StreamSize_Encoded;
                                    case Generic_StreamSize_Encoded_String : return Audio_StreamSize_Encoded_String;
                                    case Generic_StreamSize_Encoded_String1 : return Audio_StreamSize_Encoded_String1;
                                    case Generic_StreamSize_Encoded_String2 : return Audio_StreamSize_Encoded_String2;
                                    case Generic_StreamSize_Encoded_String3 : return Audio_StreamSize_Encoded_String3;
                                    case Generic_StreamSize_Encoded_String4 : return Audio_StreamSize_Encoded_String4;
                                    case Generic_StreamSize_Encoded_String5 : return Audio_StreamSize_Encoded_String5;
                                    case Generic_StreamSize_Encoded_Proportion : return Audio_StreamSize_Encoded_Proportion;
                                    case Generic_Source_StreamSize : return Audio_Source_StreamSize;
                                    case Generic_Source_StreamSize_String : return Audio_Source_StreamSize_String;
                                    case Generic_Source_StreamSize_String1 : return Audio_Source_StreamSize_String1;
                                    case Generic_Source_StreamSize_String2 : return Audio_Source_StreamSize_String2;
                                    case Generic_Source_StreamSize_String3 : return Audio_Source_StreamSize_String3;
                                    case Generic_Source_StreamSize_String4 : return Audio_Source_StreamSize_String4;
                                    case Generic_Source_StreamSize_String5 : return Audio_Source_StreamSize_String5;
                                    case Generic_Source_StreamSize_Proportion : return Audio_Source_StreamSize_Proportion;
                                    case Generic_Source_StreamSize_Encoded : return Audio_Source_StreamSize_Encoded;
                                    case Generic_Source_StreamSize_Encoded_String : return Audio_Source_StreamSize_Encoded_String;
                                    case Generic_Source_StreamSize_Encoded_String1 : return Audio_Source_StreamSize_Encoded_String1;
                                    case Generic_Source_StreamSize_Encoded_String2 : return Audio_Source_StreamSize_Encoded_String2;
                                    case Generic_Source_StreamSize_Encoded_String3 : return Audio_Source_StreamSize_Encoded_String3;
                                    case Generic_Source_StreamSize_Encoded_String4 : return Audio_Source_StreamSize_Encoded_String4;
                                    case Generic_Source_StreamSize_Encoded_String5 : return Audio_Source_StreamSize_Encoded_String5;
                                    case Generic_Source_StreamSize_Encoded_Proportion : return Audio_Source_StreamSize_Encoded_Proportion;
                                    case Generic_Language : return Audio_Language;
                                    default: return (size_t)-1;
                                }
        case Stream_Text :
                                switch (StreamPos)
                                {
                                    case Generic_Format : return Text_Format;
                                    case Generic_Format_Info : return Text_Format_Info;
                                    case Generic_Format_Url : return Text_Format_Url;
                                    case Generic_Format_String : return Text_Format_String;
                                    case Generic_Format_Commercial : return Text_Format_Commercial;
                                    case Generic_Format_Commercial_IfAny : return Text_Format_Commercial_IfAny;
                                    case Generic_Format_Version : return Text_Format_Version;
                                    case Generic_Format_Profile : return Text_Format_Profile;
                                    case Generic_Format_Settings : return Text_Format_Settings;
                                    case Generic_Format_AdditionalFeatures : return Text_Format_AdditionalFeatures;
                                    case Generic_InternetMediaType : return Text_InternetMediaType;
                                    case Generic_CodecID : return Text_CodecID;
                                    case Generic_CodecID_Info : return Text_CodecID_Info;
                                    case Generic_CodecID_Hint : return Text_CodecID_Hint;
                                    case Generic_CodecID_Url : return Text_CodecID_Url;
                                    case Generic_CodecID_Description : return Text_CodecID_Description;
                                    case Generic_Codec : return Text_Codec;
                                    case Generic_Codec_String : return Text_Codec_String;
                                    case Generic_Codec_Info : return Text_Codec_Info;
                                    case Generic_Codec_Url : return Text_Codec_Url;
                                    case Generic_Codec_CC : return Text_Codec_CC;
                                    case Generic_Width: return Text_Width;
                                    case Generic_Height: return Text_Height;
                                    case Generic_Duration : return Text_Duration;
                                    case Generic_Duration_String : return Text_Duration_String;
                                    case Generic_Duration_String1 : return Text_Duration_String1;
                                    case Generic_Duration_String2 : return Text_Duration_String2;
                                    case Generic_Duration_String3 : return Text_Duration_String3;
                                    case Generic_Duration_String4 : return Text_Duration_String4;
                                    case Generic_Duration_String5 : return Text_Duration_String5;
                                    case Generic_Source_Duration : return Text_Source_Duration;
                                    case Generic_Source_Duration_String : return Text_Source_Duration_String;
                                    case Generic_Source_Duration_String1 : return Text_Source_Duration_String1;
                                    case Generic_Source_Duration_String2 : return Text_Source_Duration_String2;
                                    case Generic_Source_Duration_String3 : return Text_Source_Duration_String3;
                                    case Generic_Source_Duration_String4 : return Text_Source_Duration_String4;
                                    case Generic_Source_Duration_String5 : return Text_Source_Duration_String5;
                                    case Generic_BitRate_Mode : return Text_BitRate_Mode;
                                    case Generic_BitRate_Mode_String : return Text_BitRate_Mode_String;
                                    case Generic_BitRate : return Text_BitRate;
                                    case Generic_BitRate_String : return Text_BitRate_String;
                                    case Generic_BitRate_Minimum : return Text_BitRate_Minimum;
                                    case Generic_BitRate_Minimum_String : return Text_BitRate_Minimum_String;
                                    case Generic_BitRate_Nominal : return Text_BitRate_Nominal;
                                    case Generic_BitRate_Nominal_String : return Text_BitRate_Nominal_String;
                                    case Generic_BitRate_Maximum : return Text_BitRate_Maximum;
                                    case Generic_BitRate_Maximum_String : return Text_BitRate_Maximum_String;
                                    case Generic_BitRate_Encoded : return Text_BitRate_Encoded;
                                    case Generic_BitRate_Encoded_String : return Text_BitRate_Encoded_String;
                                    case Generic_FrameRate : return Text_FrameRate;
                                    case Generic_FrameRate_String: return Text_FrameRate_String;
                                    case Generic_FrameRate_Num: return Text_FrameRate_Num;
                                    case Generic_FrameRate_Den: return Text_FrameRate_Den;
                                    case Generic_FrameCount : return Text_FrameCount;
                                    case Generic_Source_FrameCount : return Text_Source_FrameCount;
                                    case Generic_ColorSpace : return Text_ColorSpace;
                                    case Generic_ChromaSubsampling : return Text_ChromaSubsampling;
                                    case Generic_Resolution : return Text_Resolution;
                                    case Generic_Resolution_String : return Text_Resolution_String;
                                    case Generic_BitDepth : return Text_BitDepth;
                                    case Generic_BitDepth_String : return Text_BitDepth_String;
                                    case Generic_Compression_Mode : return Text_Compression_Mode;
                                    case Generic_Compression_Mode_String : return Text_Compression_Mode_String;
                                    case Generic_Compression_Ratio : return Text_Compression_Ratio;
                                    case Generic_Delay : return Text_Delay;
                                    case Generic_Delay_String : return Text_Delay_String;
                                    case Generic_Delay_String1 : return Text_Delay_String1;
                                    case Generic_Delay_String2 : return Text_Delay_String2;
                                    case Generic_Delay_String3 : return Text_Delay_String3;
                                    case Generic_Delay_String4 : return Text_Delay_String4;
                                    case Generic_Delay_String5 : return Text_Delay_String5;
                                    case Generic_Delay_Settings : return Text_Delay_Settings;
                                    case Generic_Delay_DropFrame : return Text_Delay_DropFrame;
                                    case Generic_Delay_Source : return Text_Delay_Source;
                                    case Generic_Delay_Source_String : return Text_Delay_Source_String;
                                    case Generic_Delay_Original : return Text_Delay_Original;
                                    case Generic_Delay_Original_String : return Text_Delay_Original_String;
                                    case Generic_Delay_Original_String1 : return Text_Delay_Original_String1;
                                    case Generic_Delay_Original_String2 : return Text_Delay_Original_String2;
                                    case Generic_Delay_Original_String3 : return Text_Delay_Original_String3;
                                    case Generic_Delay_Original_String4 : return Text_Delay_Original_String4;
                                    case Generic_Delay_Original_Settings : return Text_Delay_Original_Settings;
                                    case Generic_Delay_Original_DropFrame : return Text_Delay_Original_DropFrame;
                                    case Generic_Delay_Original_Source : return Text_Delay_Original_Source;
                                    case Generic_Video_Delay : return Text_Video_Delay;
                                    case Generic_Video_Delay_String : return Text_Video_Delay_String;
                                    case Generic_Video_Delay_String1 : return Text_Video_Delay_String1;
                                    case Generic_Video_Delay_String2 : return Text_Video_Delay_String2;
                                    case Generic_Video_Delay_String3 : return Text_Video_Delay_String3;
                                    case Generic_Video_Delay_String4 : return Text_Video_Delay_String4;
                                    case Generic_TimeCode_FirstFrame : return Text_TimeCode_FirstFrame;
                                    case Generic_TimeCode_DropFrame: return Text_TimeCode_DropFrame;
                                    case Generic_TimeCode_Settings: return Text_TimeCode_Settings;
                                    case Generic_TimeCode_Source: return Text_TimeCode_Source;
                                    case Generic_StreamSize : return Text_StreamSize;
                                    case Generic_StreamSize_String : return Text_StreamSize_String;
                                    case Generic_StreamSize_String1 : return Text_StreamSize_String1;
                                    case Generic_StreamSize_String2 : return Text_StreamSize_String2;
                                    case Generic_StreamSize_String3 : return Text_StreamSize_String3;
                                    case Generic_StreamSize_String4 : return Text_StreamSize_String4;
                                    case Generic_StreamSize_String5 : return Text_StreamSize_String5;
                                    case Generic_StreamSize_Proportion : return Text_StreamSize_Proportion;
                                    case Generic_StreamSize_Encoded : return Text_StreamSize_Encoded;
                                    case Generic_StreamSize_Encoded_String : return Text_StreamSize_Encoded_String;
                                    case Generic_StreamSize_Encoded_String1 : return Text_StreamSize_Encoded_String1;
                                    case Generic_StreamSize_Encoded_String2 : return Text_StreamSize_Encoded_String2;
                                    case Generic_StreamSize_Encoded_String3 : return Text_StreamSize_Encoded_String3;
                                    case Generic_StreamSize_Encoded_String4 : return Text_StreamSize_Encoded_String4;
                                    case Generic_StreamSize_Encoded_String5 : return Text_StreamSize_Encoded_String5;
                                    case Generic_StreamSize_Encoded_Proportion : return Text_StreamSize_Encoded_Proportion;
                                    case Generic_Source_StreamSize : return Text_Source_StreamSize;
                                    case Generic_Source_StreamSize_String : return Text_Source_StreamSize_String;
                                    case Generic_Source_StreamSize_String1 : return Text_Source_StreamSize_String1;
                                    case Generic_Source_StreamSize_String2 : return Text_Source_StreamSize_String2;
                                    case Generic_Source_StreamSize_String3 : return Text_Source_StreamSize_String3;
                                    case Generic_Source_StreamSize_String4 : return Text_Source_StreamSize_String4;
                                    case Generic_Source_StreamSize_String5 : return Text_Source_StreamSize_String5;
                                    case Generic_Source_StreamSize_Proportion : return Text_Source_StreamSize_Proportion;
                                    case Generic_Source_StreamSize_Encoded : return Text_Source_StreamSize_Encoded;
                                    case Generic_Source_StreamSize_Encoded_String : return Text_Source_StreamSize_Encoded_String;
                                    case Generic_Source_StreamSize_Encoded_String1 : return Text_Source_StreamSize_Encoded_String1;
                                    case Generic_Source_StreamSize_Encoded_String2 : return Text_Source_StreamSize_Encoded_String2;
                                    case Generic_Source_StreamSize_Encoded_String3 : return Text_Source_StreamSize_Encoded_String3;
                                    case Generic_Source_StreamSize_Encoded_String4 : return Text_Source_StreamSize_Encoded_String4;
                                    case Generic_Source_StreamSize_Encoded_String5 : return Text_Source_StreamSize_Encoded_String5;
                                    case Generic_Source_StreamSize_Encoded_Proportion : return Text_Source_StreamSize_Encoded_Proportion;
                                    case Generic_Language : return Text_Language;
                                    default: return (size_t)-1;
                                }
        case Stream_Other :
                                switch (StreamPos)
                                {
                                    case Generic_Format : return Other_Format;
                                    case Generic_Format_Info : return Other_Format_Info;
                                    case Generic_Format_Url : return Other_Format_Url;
                                    case Generic_Format_String : return Other_Format_String;
                                    case Generic_Format_Commercial : return Other_Format_Commercial;
                                    case Generic_Format_Commercial_IfAny : return Other_Format_Commercial_IfAny;
                                    case Generic_Format_Version : return Other_Format_Version;
                                    case Generic_Format_Profile : return Other_Format_Profile;
                                    case Generic_Format_Settings : return Other_Format_Settings;
                                    case Generic_Format_AdditionalFeatures : return Other_Format_AdditionalFeatures;
                                    case Generic_CodecID : return Other_CodecID;
                                    case Generic_CodecID_Info : return Other_CodecID_Info;
                                    case Generic_CodecID_Hint : return Other_CodecID_Hint;
                                    case Generic_CodecID_Url : return Other_CodecID_Url;
                                    case Generic_CodecID_Description : return Other_CodecID_Description;
                                    case Generic_Duration : return Other_Duration;
                                    case Generic_Duration_String : return Other_Duration_String;
                                    case Generic_Duration_String1 : return Other_Duration_String1;
                                    case Generic_Duration_String2 : return Other_Duration_String2;
                                    case Generic_Duration_String3 : return Other_Duration_String3;
                                    case Generic_Duration_String4 : return Other_Duration_String4;
                                    case Generic_Duration_String5 : return Other_Duration_String5;
                                    case Generic_FrameRate : return Other_FrameRate;
                                    case Generic_FrameRate_String: return Other_FrameRate_String;
                                    case Generic_FrameRate_Num: return Other_FrameRate_Num;
                                    case Generic_FrameRate_Den: return Other_FrameRate_Den;
                                    case Generic_FrameCount : return Other_FrameCount;
                                    case Generic_Delay : return Other_Delay;
                                    case Generic_Delay_String : return Other_Delay_String;
                                    case Generic_Delay_String1 : return Other_Delay_String1;
                                    case Generic_Delay_String2 : return Other_Delay_String2;
                                    case Generic_Delay_String3 : return Other_Delay_String3;
                                    case Generic_Delay_String4 : return Other_Delay_String4;
                                    case Generic_Delay_String5 : return Other_Delay_String5;
                                    case Generic_Delay_Settings : return Other_Delay_Settings;
                                    case Generic_Delay_DropFrame : return Other_Delay_DropFrame;
                                    case Generic_Delay_Source : return Other_Delay_Source;
                                    case Generic_Delay_Source_String : return Other_Delay_Source_String;
                                    case Generic_Delay_Original : return Other_Delay_Original;
                                    case Generic_Delay_Original_String : return Other_Delay_Original_String;
                                    case Generic_Delay_Original_String1 : return Other_Delay_Original_String1;
                                    case Generic_Delay_Original_String2 : return Other_Delay_Original_String2;
                                    case Generic_Delay_Original_String3 : return Other_Delay_Original_String3;
                                    case Generic_Delay_Original_String4 : return Other_Delay_Original_String4;
                                    case Generic_Delay_Original_Settings : return Other_Delay_Original_Settings;
                                    case Generic_Delay_Original_DropFrame : return Other_Delay_Original_DropFrame;
                                    case Generic_Delay_Original_Source : return Other_Delay_Original_Source;
                                    case Generic_Video_Delay : return Other_Video_Delay;
                                    case Generic_Video_Delay_String : return Other_Video_Delay_String;
                                    case Generic_Video_Delay_String1 : return Other_Video_Delay_String1;
                                    case Generic_Video_Delay_String2 : return Other_Video_Delay_String2;
                                    case Generic_Video_Delay_String3 : return Other_Video_Delay_String3;
                                    case Generic_Video_Delay_String4 : return Other_Video_Delay_String4;
                                    case Generic_TimeCode_FirstFrame : return Other_TimeCode_FirstFrame;
                                    case Generic_TimeCode_DropFrame: return Other_TimeCode_DropFrame;
                                    case Generic_TimeCode_Settings: return Other_TimeCode_Settings;
                                    case Generic_TimeCode_Source: return Other_TimeCode_Source;
                                    case Generic_StreamSize : return Other_StreamSize;
                                    case Generic_StreamSize_String : return Other_StreamSize_String;
                                    case Generic_StreamSize_String1 : return Other_StreamSize_String1;
                                    case Generic_StreamSize_String2 : return Other_StreamSize_String2;
                                    case Generic_StreamSize_String3 : return Other_StreamSize_String3;
                                    case Generic_StreamSize_String4 : return Other_StreamSize_String4;
                                    case Generic_StreamSize_String5 : return Other_StreamSize_String5;
                                    case Generic_StreamSize_Proportion : return Other_StreamSize_Proportion;
                                    case Generic_StreamSize_Encoded : return Other_StreamSize_Encoded;
                                    case Generic_StreamSize_Encoded_String : return Other_StreamSize_Encoded_String;
                                    case Generic_StreamSize_Encoded_String1 : return Other_StreamSize_Encoded_String1;
                                    case Generic_StreamSize_Encoded_String2 : return Other_StreamSize_Encoded_String2;
                                    case Generic_StreamSize_Encoded_String3 : return Other_StreamSize_Encoded_String3;
                                    case Generic_StreamSize_Encoded_String4 : return Other_StreamSize_Encoded_String4;
                                    case Generic_StreamSize_Encoded_String5 : return Other_StreamSize_Encoded_String5;
                                    case Generic_StreamSize_Encoded_Proportion : return Other_StreamSize_Encoded_Proportion;
                                    case Generic_Source_StreamSize : return Other_Source_StreamSize;
                                    case Generic_Source_StreamSize_String : return Other_Source_StreamSize_String;
                                    case Generic_Source_StreamSize_String1 : return Other_Source_StreamSize_String1;
                                    case Generic_Source_StreamSize_String2 : return Other_Source_StreamSize_String2;
                                    case Generic_Source_StreamSize_String3 : return Other_Source_StreamSize_String3;
                                    case Generic_Source_StreamSize_String4 : return Other_Source_StreamSize_String4;
                                    case Generic_Source_StreamSize_String5 : return Other_Source_StreamSize_String5;
                                    case Generic_Source_StreamSize_Proportion : return Other_Source_StreamSize_Proportion;
                                    case Generic_Source_StreamSize_Encoded : return Other_Source_StreamSize_Encoded;
                                    case Generic_Source_StreamSize_Encoded_String : return Other_Source_StreamSize_Encoded_String;
                                    case Generic_Source_StreamSize_Encoded_String1 : return Other_Source_StreamSize_Encoded_String1;
                                    case Generic_Source_StreamSize_Encoded_String2 : return Other_Source_StreamSize_Encoded_String2;
                                    case Generic_Source_StreamSize_Encoded_String3 : return Other_Source_StreamSize_Encoded_String3;
                                    case Generic_Source_StreamSize_Encoded_String4 : return Other_Source_StreamSize_Encoded_String4;
                                    case Generic_Source_StreamSize_Encoded_String5 : return Other_Source_StreamSize_Encoded_String5;
                                    case Generic_Source_StreamSize_Encoded_Proportion : return Other_Source_StreamSize_Encoded_Proportion;
                                    case Generic_Language : return Other_Language;
                                    default: return (size_t)-1;
                                }
        case Stream_Image :
                                switch (StreamPos)
                                {
                                    case Generic_Format : return Image_Format;
                                    case Generic_Format_Info : return Image_Format_Info;
                                    case Generic_Format_Url : return Image_Format_Url;
                                    case Generic_Format_String : return Image_Format_String;
                                    case Generic_Format_Commercial : return Image_Format_Commercial;
                                    case Generic_Format_Commercial_IfAny : return Image_Format_Commercial_IfAny;
                                    case Generic_Format_Version : return Image_Format_Version;
                                    case Generic_Format_Profile : return Image_Format_Profile;
                                    case Generic_Format_AdditionalFeatures : return Image_Format_AdditionalFeatures;
                                    case Generic_InternetMediaType : return Image_InternetMediaType;
                                    case Generic_CodecID : return Image_CodecID;
                                    case Generic_CodecID_Info : return Image_CodecID_Info;
                                    case Generic_CodecID_Hint : return Image_CodecID_Hint;
                                    case Generic_CodecID_Url : return Image_CodecID_Url;
                                    case Generic_CodecID_Description : return Image_CodecID_Description;
                                    case Generic_Codec : return Image_Codec;
                                    case Generic_Codec_String : return Image_Codec_String;
                                    case Generic_Codec_Info : return Image_Codec_Info;
                                    case Generic_Codec_Url : return Image_Codec_Url;
                                    case Generic_Width: return Image_Width;
                                    case Generic_Height: return Image_Height;
                                    case Generic_Duration : return Menu_Duration;
                                    case Generic_Duration_String : return Menu_Duration_String;
                                    case Generic_Duration_String1 : return Menu_Duration_String1;
                                    case Generic_Duration_String2 : return Menu_Duration_String2;
                                    case Generic_Duration_String3 : return Menu_Duration_String3;
                                    case Generic_Duration_String4 : return Menu_Duration_String4;
                                    case Generic_Duration_String5 : return Menu_Duration_String5;
                                    case Generic_FrameRate : return Menu_FrameRate;
                                    case Generic_FrameRate_String: return Menu_FrameRate_String;
                                    case Generic_FrameRate_Num: return Menu_FrameRate_Num;
                                    case Generic_FrameRate_Den: return Menu_FrameRate_Den;
                                    case Generic_FrameCount : return Menu_FrameCount;
                                    case Generic_ColorSpace : return Image_ColorSpace;
                                    case Generic_ChromaSubsampling : return Image_ChromaSubsampling;
                                    case Generic_Resolution : return Image_Resolution;
                                    case Generic_Resolution_String : return Image_Resolution_String;
                                    case Generic_BitDepth : return Image_BitDepth;
                                    case Generic_BitDepth_String : return Image_BitDepth_String;
                                    case Generic_Compression_Mode : return Image_Compression_Mode;
                                    case Generic_Compression_Mode_String : return Image_Compression_Mode_String;
                                    case Generic_Compression_Ratio : return Image_Compression_Ratio;
                                    case Generic_StreamSize : return Image_StreamSize;
                                    case Generic_StreamSize_String : return Image_StreamSize_String;
                                    case Generic_StreamSize_String1 : return Image_StreamSize_String1;
                                    case Generic_StreamSize_String2 : return Image_StreamSize_String2;
                                    case Generic_StreamSize_String3 : return Image_StreamSize_String3;
                                    case Generic_StreamSize_String4 : return Image_StreamSize_String4;
                                    case Generic_StreamSize_String5 : return Image_StreamSize_String5;
                                    case Generic_StreamSize_Proportion : return Image_StreamSize_Proportion;
                                    case Generic_Language : return Image_Language;
                                    default: return (size_t)-1;
                                }
        case Stream_Menu :
                                switch (StreamPos)
                                {
                                    case Generic_Format : return Menu_Format;
                                    case Generic_Format_Info : return Menu_Format_Info;
                                    case Generic_Format_Url : return Menu_Format_Url;
                                    case Generic_Format_String : return Menu_Format_String;
                                    case Generic_Format_Commercial : return Menu_Format_Commercial;
                                    case Generic_Format_Commercial_IfAny : return Menu_Format_Commercial_IfAny;
                                    case Generic_Format_Version : return Menu_Format_Version;
                                    case Generic_Format_Profile : return Menu_Format_Profile;
                                    case Generic_Format_Settings : return Menu_Format_Settings;
                                    case Generic_CodecID : return Menu_CodecID;
                                    case Generic_CodecID_Info : return Menu_CodecID_Info;
                                    case Generic_CodecID_Hint : return Menu_CodecID_Hint;
                                    case Generic_CodecID_Url : return Menu_CodecID_Url;
                                    case Generic_CodecID_Description : return Menu_CodecID_Description;
                                    case Generic_Codec : return Menu_Codec;
                                    case Generic_Codec_String : return Menu_Codec_String;
                                    case Generic_Codec_Info : return Menu_Codec_Info;
                                    case Generic_Codec_Url : return Menu_Codec_Url;
                                    case Generic_Duration : return Menu_Duration;
                                    case Generic_Duration_String : return Menu_Duration_String;
                                    case Generic_Duration_String1 : return Menu_Duration_String1;
                                    case Generic_Duration_String2 : return Menu_Duration_String2;
                                    case Generic_Duration_String3 : return Menu_Duration_String3;
                                    case Generic_Duration_String4 : return Menu_Duration_String4;
                                    case Generic_Duration_String5 : return Menu_Duration_String5;
                                    case Generic_Language : return Menu_Language;
                                    case Generic_ServiceName : return Menu_ServiceName;
                                    case Generic_ServiceProvider : return Menu_ServiceProvider;
                                    default: return (size_t)-1;
                                }
        default: return (size_t)-1;
    }
}

} //NameSpace

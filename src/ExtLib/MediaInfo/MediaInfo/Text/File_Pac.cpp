/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
// Information about Screen Electronics PAC subtitle files
// Implemented by reverse engineering with the files we have,
// maybe not accurate
// Some ideas from Subtitle Edit
// https://github.com/SubtitleEdit/subtitleedit/blob/main/src/libse/SubtitleFormats/Pac.cs

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
#if defined(MEDIAINFO_PAC_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Text/File_Pac.h"
#include "MediaInfo/Text/File_Pac_Codepages.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include "MediaInfo/TimeCode.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
using namespace std;
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
namespace MediaInfoLib
{
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void Pac_Convert(Ztring& ToConvert, const codepage& CodePage) {
    auto Table = CodePage.List - 0x20;
    auto Table_Size = CodePage.Size + 0x20;
    for (auto& Item : ToConvert) {
        if (Item >= 0x20 && Item < Table_Size && Table[Item]) {
            Item = Table[Item];
        }
    }
}

//---------------------------------------------------------------------------
enum Pac_format {
    Format_8bit,
    Format_Skip3,
    Format_Skip4,
    Format_Utf8,
    Format_16bit,
};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Pac::File_Pac()
:File__Analyze()
{
}

//---------------------------------------------------------------------------
File_Pac::~File_Pac()
{
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Pac::Streams_Accept()
{
    Fill(Stream_General, 0, General_Format, "PAC");
    Stream_Prepare(Stream_Text);
    Fill(Stream_Text, 0, Text_Format, "PAC");
}

//---------------------------------------------------------------------------
void File_Pac::Streams_Finish()
{
    #if MEDIAINFO_ADVANCED
        const float64 FrameRate_FromConfig = Video_FrameRate_Rounded(Config->File_DefaultFrameRate_Get());
        const auto FrameMax = (FrameRate_FromConfig && FrameRate_FromConfig >= FF_Max + 1)
            ? ((const uint32_t)ceil(FrameRate_FromConfig) - 1)
            : (FF_Max <= 23 ? 23 : FF_Max <= 24 ? 24 : FF_Max <= 29 ? 29 : FF_Max); // Files I have are all with frame rate of 24 fps
        const auto FrameRate_F = FrameRate_FromConfig ? FrameRate_FromConfig : ((const float64)FrameMax + 1);
        const auto FrameRateIsGuessed = FrameRate_FromConfig == 0;
        const auto DropFrame = Config->File_DefaultTimeCodeDropFrame_Get() == 1;
    #else //MEDIAINFO_ADVANCED
        const uint32_t FrameMax = FF_Max <= 23 ? 23 : FF_Max <= 24 ? 24 : FF_Max <= 30 ? 30 : FF_Max;
        const auto FrameRate_F = (const float32)FrameMax + 1;
        const auto FrameRateIsGuessed = true;
        const auto DropFrame = false;
    #endif //MEDIAINFO_ADVANCED
    Fill(Stream_Text, 0, Text_FrameRate, FrameRate_F, FrameRateIsGuessed?0:3); // Default without decimal as a hint that this is a guessed value
    Time_Start_Command.SetFramesMax(FrameMax);
    Time_Start_Command.SetDropFrame(DropFrame);
    Time_End_Command.SetFramesMax(FrameMax);
    Time_End_Command.SetDropFrame(DropFrame);
    TimeCode Delay_TC;
    TimeCode Offset;
    if (Time_Delay.IsValid()) {
        Delay_TC = TimeCode(Delay, FrameMax, TimeCode::flags());
        Offset = Time_Delay - Delay_TC;
    }
    else {
        Offset = Time_Start_Command;
        Offset.SetMinutes(0);
        Offset.SetSeconds(0);
        Offset.SetFrames(0);
    }
    int64s Duration = (Time_End_Command - Time_Start_Command).ToMilliseconds();
    Fill(Stream_General, 0, General_Duration, Duration);
    Fill(Stream_Text, 0, Text_Duration, Duration);
    Fill(Stream_Text, 0, Text_Duration_Start_Command, (int64s)(Time_Start_Command - Offset).ToMilliseconds());
    Fill(Stream_Text, 0, Text_TimeCode_FirstFrame, Time_Start_Command.ToString());
    Fill(Stream_Text, 0, Text_Duration_End_Command, (int64s)(Time_End_Command - Offset).ToMilliseconds());
    TimeCode LastFrame = Time_End_Command;
    --LastFrame;
    Fill(Stream_Text, 0, Text_TimeCode_LastFrame, LastFrame.ToString());
    if (Time_Start.IsValid()) {
        Time_Start.SetFramesMax(FrameMax);
        Time_End.SetDropFrame(DropFrame);
        Fill(Stream_Text, 0, Text_Duration_Start, (int64s)(Time_Start - Offset).ToMilliseconds());
    }
    if (Time_End.IsValid()) {
        Time_End.SetFramesMax(FrameMax);
        Time_End.SetDropFrame(DropFrame);
        Fill(Stream_Text, 0, Text_Duration_End, (int64s)(Time_End - Offset).ToMilliseconds());
    }
    if (Time_End.IsValid() && Time_Start.IsValid()) {
        auto Start2End = (int64s)(Time_End - Time_Start).ToMilliseconds();
        Fill(Stream_Text, 0, Text_Duration_Start2End, Start2End);
    }
    if (Delay_TC.IsValid()) {
        Fill(Stream_Text, 0, Text_Delay, (int64s)Delay_TC.ToMilliseconds());
    }

    if (Frame_Count) {
        Fill(Stream_Text, 0, Text_Events_Total, Frame_Count - EmptyCount);
    }
    if (LineCount) {
        Fill(Stream_Text, 0, Text_Lines_Count, LineCount);
    }
    if (LineCount) {
        Fill(Stream_Text, 0, Text_Lines_MaxCountPerEvent, MaxCountOfLinesPerFrame);
    }
    if (MaxCountOfCharsPerLine) {
        Fill(Stream_Text, 0, Text_Lines_MaxCharacterCount, MaxCountOfCharsPerLine);
    }
    if (Count_UTF8) {
        Fill(Stream_Text, 0, "CharacterSet", "UTF-8");
    }
    if (Count_2byte) {
        Fill(Stream_Text, 0, "CharacterSet", "2-byte");
    }
    if (Count_1byte8) {
        Fill(Stream_Text, 0, "CharacterSet", "1-byte");
    }
    if (Count_1byte7 && !Count_1byte8 && !Count_UTF8 && !Count_2byte) {
        Fill(Stream_Text, 0, "CharacterSet", "ASCII");
    }
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
void File_Pac::FileHeader_Parse()
{
    if (Buffer_Size < 20) {
        Element_WaitForMoreData();
        return;
    }

    for (size_t i = 0; i < 20; i++) {
        if ((!i && Buffer[0] != 1) || (i && Buffer[i])) {
            Reject();
            return;
        }
    }

    Skip_XX(20,                                                 "Signature?");
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Pac::Header_Parse()
{
    auto PacTimeCode = [&](const char* Name) {
        Element_Begin1(Name);
        int16u HHMM, SSFF;
        Get_L2 (HHMM,                                           "HHMM");
        Get_L2 (SSFF,                                           "SSFF");
        TimeCode TC(HHMM / 100, HHMM % 100, SSFF / 100, SSFF % 100, 99);
        if (FF_Max < TC.GetFrames()) {
            FF_Max = TC.GetFrames();
        }
        if (!TC.IsValid()) {
            Reject();
        }
        Element_Info1(TC.ToString());
        Element_End0();
        return TC;
    };
    int16u ContentLength, FrameNumber;
    int8u Type, SubType;
    Get_L1 (Type,                                               "Type");
    Get_L2 (FrameNumber,                                        "Frame number");
    Get_L1 (SubType,                                            "Sub-Type?");
    auto Start_Temp = PacTimeCode("Start");
    auto End_Temp = PacTimeCode("End");
    if (!Type) {
        if (Start_Temp.IsValid()) {
            Time_Start_Command_Temp = Start_Temp;
        }
        if (End_Temp.IsValid()) {
            Time_End_Command = End_Temp;
        }
    }
    Get_L2 (ContentLength,                                      "Content length");

    if (!Status[IsAccepted]) {
        if (!Frame_Count_Metadata && !Frame_Count && FrameNumber == 1) {
            Frame_Count_Metadata++;
        }

        if ((Type && Type != 0xFF)
            || (!Type && (SubType < 0x60 || SubType > 0x62 || FrameNumber != Frame_Count_Metadata + Frame_Count)) || !Start_Temp.IsValid() || !End_Temp.IsValid()) {
            Reject();
        }
    }

    Header_Fill_Code(Type, Type?"File Footer":"Packet");
    Header_Fill_Size(Element_Offset + ContentLength);
}

//---------------------------------------------------------------------------
void File_Pac::Data_Parse()
{
    // Parsing
    if (!Element_Size) {
        return;
    }
    if (!Element_Code) {
        Skip_L1(                                                "Vertical alignment");
        if (Element_Offset != Element_Size) {
            int8u Probe;
            Peek_L1(Probe);
            if (Probe != 0xFE) {
                Skip_L1(                                        "(Unknown)");
                Skip_L1(                                        "(Unknown)");
                Skip_L1(                                        "(Unknown)");
            }
        }
    }
    ZtringList Content;
    auto NextPartIsStart = true;
    auto NextPartFormat = Format_8bit;
    while (Element_Offset < Element_Size) {
        if (NextPartIsStart) {
            if (!Element_Code) {
                int8u Start;
                Peek_L1(Start);
                auto Size = Element_Size - Element_Offset;
                if (Start != 0xFE || Size <= 2) {
                    Trusted_IsNot("0xFE");
                    Skip_XX(Size,                               "(Unknown)");
                    break;
                }
                Skip_L1(                                        "Line start marker");
                Skip_L1(                                        "Horizontal alignment");
                Skip_L1(                                        "(Unknown)");
            }
            Content.resize(Content.size() + 1);
            NextPartIsStart = false;
        }

        auto Element_Middle = Element_Offset;
        auto Format = NextPartFormat;
        NextPartFormat = Format_8bit;
        for (Element_Middle = Element_Offset; Element_Middle < Element_Size; Element_Middle += Format == Format_16bit ? 2 : 1) {
            const auto Value = Buffer[Buffer_Offset + Element_Middle];
            if (Value == 0x1F) {
                auto Buffer_Current = Buffer + Buffer_Offset + (size_t)Element_Middle + 1;
                auto NextPartMaxSize = Element_Size - Element_Middle;
                if (NextPartMaxSize >= 3) {
                    auto Probe = BigEndian2int16u(Buffer_Current);
                    if (Probe >= 0x4330 && Probe <= 0x4339) { // C0 - C9
                        NextPartFormat = Format_Skip3;
                        break;
                    }
                }
                if (NextPartMaxSize > 3) {
                    auto Probe = BigEndian2int24u(Buffer_Current);
                    if (Probe >= 0x522E30 && Probe <= 0x522E39) { // R.0 - R.9
                        NextPartFormat = Format_Skip4;
                        break;
                    }
                    if (Probe == 0xEFBBBF) {
                        NextPartFormat = Format_Utf8;
                        break;
                    }
                }
                if (NextPartMaxSize >= 5) {
                    auto Probe = BigEndian2int32u(Buffer_Current);
                    if (Probe == 0x5731362E) { // W16.
                        NextPartFormat = Format_16bit;
                        break;
                    }
                }
            }
            if (Value == 0xFE && !Element_Code) {
                NextPartIsStart = true;
                break;
            }
        }

        // Current
        auto Size = Element_Middle - Element_Offset;
        switch (Format) {
        case Format_8bit: {
            Ztring Value;
            Get_ISO_8859_1(Size, Value,                         "Content");
            bool Is8bit = false;
            for (const auto& Character : Value) {
                if ((unsigned)Character >= 0x80) {
                    Is8bit = true;
                }
            }
            if (Is8bit) {
                Count_1byte8++;
                for (auto i = Value.size() - 1; i; --i) {
                    const auto Character = Value[i];
                    if ((unsigned)Character >= 0xE0) {
                        Value.erase(i, 1); // Content reading not supported but no need for counting
                    }
                }
            }
            else if (Size) {
                Count_1byte7++;
            }
            Content.back() += Value;
            break;
        }
        case Format_Skip4:
        case Format_Skip3: {
            break;
        }
        case Format_Utf8: {
            auto Element_UTF8End = Element_Offset;
            for (; Element_UTF8End < Element_Middle; ++Element_UTF8End) {
                const auto Data = Buffer[Buffer_Offset + Element_UTF8End];
                if (Data == 0x2E || Data == 0xFF) {
                    Ztring Value;
                    Get_UTF8(Element_UTF8End - Element_Offset, Value, "Content");
                    Content.back() += Value;
                    if (Data == 0xFF) {
                        Skip_L1(                                "Dot");
                        Content.back() += __T('.');
                        continue;
                    }
                    Skip_C1(                                    "UTF-8 end");
                    NextPartFormat = Format_8bit;
                }
            }
            Count_UTF8++;
            break;
        }
        case Format_16bit: {
            auto Current = (int16u*)(Buffer + Buffer_Offset + (size_t)Element_Offset);
            auto End = Current + Size / 2;
            for (; Current < End; ++Current) {
                Content.back() += (wchar_t)*Current; // Content reading not supported but no need for counting, we need only the values < 0x80
            }
            Skip_XX(Size,                                       "W16 content");
            Count_2byte++;
            break;
        }
        }

        // Next
        switch (NextPartFormat) {
        case Format_8bit: {
            break;
        }
        case Format_Skip3: {
            Skip_C3(                                            "(Unknown)");
            break;
        }
        case Format_Skip4: {
            Skip_C4(                                            "(Unknown)");
            break;
        }
        case Format_Utf8: {
            Skip_C4(                                            "UTF-8 start");
            break;
        }
        case Format_16bit:{
            Skip_C5(                                            "W16 start");
            break;
        }
        }
    }

    FILLING_BEGIN();
        if (!Element_Code) {
            bool MayBeTags = !Frame_Count && Time_Start_Command_Temp == TimeCode(0, Time_Start_Command_Temp.GetFramesMax()) && Time_End_Command == TimeCode(8, Time_End_Command.GetFramesMax());
            bool IsTags = false;
            bool IsFrameNumber = false;
            if (MayBeTags) {
                size_t Pac_MetadataMap_Pos = 0;
                Ztring Language;
                Ztring CatalogNumber;
                Ztring Title;
                map<Ztring, Ztring> More;
                for (size_t i = 0; i < Pac_MetadataMap_Size; i++) {
                    const auto& Item = Pac_MetadataMap[i];
                    More.clear();
                    for (const auto& Line : Content) {
                        auto Separator_Pos = Line.find(Item.Separator, 0);
                        if (Separator_Pos != string::npos) {
                            auto Space_Pos = Line.rfind(__T(' '), Separator_Pos);
                            if (Space_Pos == string::npos) {
                                IsTags = true;
                                Ztring Field = Line.substr(0, Separator_Pos);
                                Separator_Pos++;
                                if (Separator_Pos < Line.size() && Line[Separator_Pos] == __T(' ')) {
                                    Separator_Pos++;
                                }
                                Ztring Value = Line.substr(Separator_Pos);
                                if (Item.Lang && Field == Item.Lang) {
                                    Pac_MetadataMap_Pos = i;
                                    Language = Value;
                                }
                                else {
                                    More[Field] = Value;
                                }
                            }
                        }
                    }
                    if (!Language.empty()) {
                        break;
                    }
                }
                if (Pac_MetadataMap_Pos == 1) {
                    Pac_Convert(Language, Pac_CodePages[0]);
                }
                auto LanguageS = Language.To_UTF8();
                for (size_t i = 0; i < Pac_CodePages_Size; i++) {
                    const auto& CodePage = Pac_CodePages[i];
                    if (CodePage.Name != LanguageS) {
                        continue;
                    }
                    Codepage_Pos = i + 1;
                    for (auto& Line : Content) {
                        Pac_Convert(Line, CodePage);
                    }
                    decltype(More) More2;
                    set<Ztring> More_ToDelete;
                    for (auto& Item : More) {
                        Ztring Field = Item.first;
                        Pac_Convert(Field, CodePage);
                        Pac_Convert(Item.second, CodePage);
                        if (Field != Item.first) {
                            More_ToDelete.insert(Item.first);
                            More2[Field] = Item.second;
                        }
                    }
                    for (auto& Item : More2) {
                        More[Item.first] = Item.second;
                    }
                    for (auto& Item : More_ToDelete) {
                        More.erase(Item);
                    }
                }
                Language.MakeLowerCase();
                for (const auto Item : Pac_LangMap)
                    if (Language.To_UTF8() == Item.From) {
                        Language.From_UTF8(Item.To);
                    }
                Fill(Stream_Text, 0, Text_Language, Language, true);
                for (const auto& Item : More) {
                    if (false) {
                    }
                    else if (Item.first == __T("STORY")) {
                        Fill(Stream_General, 0, General_CatalogNumber, Item.second, true);
                    }
                    else if (Item.first == __T("TITLE")) {
                        Fill(Stream_General, 0, General_Title, Item.second, true);
                    }
                    else {
                        Fill(Stream_General, 0, Item.first.To_UTF8().c_str(), Item.second, true);
                    }
                }
            }
            if (Codepage_Pos) {
                const auto& CodePage = Pac_CodePages[Codepage_Pos - 1];
                for (auto& Line : Content) {
                    Pac_Convert(Line, CodePage);
                }
            }
            bool FirstFrameOfVideo = !Frame_Count && Content.size() == 2 && Content[0] == __T("First Frame of Video");
            if (Content.size() == 1 + FirstFrameOfVideo) {
                const auto& Line = Content[FirstFrameOfVideo];
                if (Line.rfind(__T("FRAME "), 0) == 0 && Line.find_first_not_of(__T("0123456789"), 6) == string::npos) {
                    IsFrameNumber = true;
                    if (!Delay) {
                        Delay = stoul(Line.substr(6));
                        Time_Delay = Time_Start_Command_Temp;
                    }
                }
            }
            if (!Time_Start_Command.IsValid() && !IsTags) {
                Time_Start_Command = Time_Start_Command_Temp;
            }
            if (IsTags || IsFrameNumber) {
                Frame_Count_Metadata++;
            }
            else {
                auto CountOfLinesPerFrame = Content.size();
                if (CountOfLinesPerFrame) {
                    LineCount += CountOfLinesPerFrame;
                    if (MaxCountOfLinesPerFrame < CountOfLinesPerFrame) {
                        MaxCountOfLinesPerFrame = CountOfLinesPerFrame;
                    }
                    for (const auto& Line : Content) {
                        auto CountOfCharsPerLine = Line.size();
                        size_t Pos = 0;
                        bool ItalicBeginFound = false;
                        for (const auto& Value : Line) {
                            if (Value < 0x20) { // Non printable
                                CountOfCharsPerLine--;
                            }
                            if (Value == '<') {
                                ItalicBeginFound = true;
                            }
                            if (Value == '<') {
                                CountOfCharsPerLine -= 2; // < and > are markers of italic
                                ItalicBeginFound = false;
                            }
                        }
                        if (MaxCountOfCharsPerLine < CountOfCharsPerLine) {
                            MaxCountOfCharsPerLine = CountOfCharsPerLine;
                        }
                    }
                    Frame_Count++;
                    if (!Time_Start.IsValid() && Time_Start_Command_Temp.IsValid()) {
                        Time_Start = Time_Start_Command_Temp;
                    }
                    if (Time_End_Command.IsValid()) {
                        Time_End = Time_End_Command;
                    }
                }
                else {
                    EmptyCount++;
                }
            }
        }
        if (!Status[IsAccepted] && (Frame_Count >= 256 || File_Offset + Buffer_Offset + Element_Size == File_Size || (File_Size == (int64u)-1 && (Frame_Count || EmptyCount) && Element_Code == 0xFF)))
            Accept();
    FILLING_END();
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_PAC_YES

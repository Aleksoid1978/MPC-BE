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
    Fill(Stream_Text, 0, Text_FrameRate, 24);
}

//---------------------------------------------------------------------------
void File_Pac::Streams_Finish()
{
    #if MEDIAINFO_ADVANCED
        float64 FrameRate_F = Video_FrameRate_Rounded(Config->File_DefaultFrameRate_Get());
        if (!FrameRate_F)
            FrameRate_F = 24; // Files I have are all with frame rate of 24 fps
        int64s FrameRate_Int = float64_int64s(FrameRate_F);
        const uint32_t FrameMax = (uint32_t)(FrameRate_Int - 1);
    #else //MEDIAINFO_ADVANCED
        const uint32_t FrameMax = 23;
    #endif //MEDIAINFO_ADVANCED
    Time_Start_Command.SetFramesMax(FrameMax);
    Time_End_Command.SetFramesMax(FrameMax);
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
    LastFrame--;
    Fill(Stream_Text, 0, Text_TimeCode_LastFrame, LastFrame.ToString());
    if (Time_Start.IsValid()) {
        Time_Start.SetFramesMax(FrameMax);
        Fill(Stream_Text, 0, Text_Duration_Start, (int64s)(Time_Start - Offset).ToMilliseconds());
    }
    if (Time_End.IsValid()) {
        Time_End.SetFramesMax(FrameMax);
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
    while (Element_Size - Element_Offset >= 1) {
        int64u Current;
        if (!Element_Code) {
            int8u Start;
            Peek_L1(Start);
            if (Start != 0xFE) {
                Trusted_IsNot("0xFE");
                Skip_XX(Element_Size-Element_Offset,            "(Unknown)");
                break;
            }
            Skip_L1(                                            "0xFE");
            Skip_L1(                                            "Horizontal alignment");
            Skip_L1(                                            "(Unknown)");
            for (Current = Element_Offset; Current < Element_Size; Current++) {
                if (Buffer[Buffer_Offset + Current] == 0xFE) {
                    break;
                }
            }
        }
        else {
            Current = Element_Size;
        }
        Content.resize(Content.size() + 1);
        auto Size = Current - Element_Offset;
        bool IsUtf8 = false;
        bool IsW16 = false;
        while (Size) {
            if (Size > 2) {
                size_t Pos = Buffer_Offset + (size_t)Element_Offset;
                if (Buffer[Pos] == 0x1F && Buffer[Pos + 1] == 'C' && Buffer[Pos + 2] >= '0' && Buffer[Pos + 2] <= '9') {
                    Skip_B3(                                    "(Unknown)");
                    Size -= 3;
                }
            }
            if (Size >= 5) {
                int64u Probe;
                Peek_B5(Probe);
                if (Probe >> 8 == 0x1FEFBBBF) {
                    Skip_B4(                                    "UTF-8 start");
                    IsUtf8 = true;
                }
                if (Probe == 0x1F5731362ELL) {
                    Skip_B5(                                    "W16 start");
                    IsW16 = true;
                }
            }
            if (IsUtf8) {
                int64u End = Element_Offset;
                for (; End < Element_Size; End++) {
                    const auto Data = Buffer[Buffer_Offset + End];
                    if (Data == 0x2E || Data == 0xFF) {
                        break;
                    }
                }
                Get_UTF8(End - Element_Offset, Content.back(),  "Content");
                if (Element_Offset < Element_Size) {
                    if (Buffer[Buffer_Offset + Element_Offset] == 0xFF) {
                        Skip_L1(                                "Dot");
                        Content.back() += __T('.');
                    }
                    else { // 0x2E
                        Skip_B1(                                "UTF-8 end");
                        IsUtf8 = false;
                    }
                }
                Count_UTF8++;
            }
            else if (IsW16) {
                Skip_XX(Size,                                   "Content");
                Content.back().resize(Size / 2);
                Count_2byte++;
            }
            else {
                Get_ISO_8859_1(Size, Content.back(),            "Content");
                bool Is8bit = false;
                for (const auto& Character : Content.back()) {
                    if ((wchar_t)Character >= 0x80) {
                        Is8bit = true;
                    }
                }
                if (Is8bit) {
                    Count_1byte8++;
                    auto& Line = Content.back();
                    if (Line.size() < numeric_limits<int>::max()) {
                        int Max = (int)Line.size();
                        for (int i = Max; i > 0; i--) {
                            const auto Character = Line[i];
                            if ((wchar_t)Character >= 0xE0) {
                                Line.erase(i, 1); // Not supported but we currently don't need the real text
                            }
                        }
                    }
                }
                else {
                    Count_1byte7++;
                }
            }
            Size = Current - Element_Offset;
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
                        for (;;) {
                            Pos = Line.find('<', Pos);
                            if (Pos == string::npos) {
                                break;
                            }
                            Pos = Line.find('>', Pos + 1);
                            if (Pos == string::npos) {
                                break;
                            }
                            CountOfCharsPerLine += 2; // < and > are marker of italic
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

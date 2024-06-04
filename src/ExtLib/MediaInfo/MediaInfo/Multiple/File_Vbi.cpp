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
#if defined(MEDIAINFO_VBI_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include <algorithm>
#include "MediaInfo/Multiple/File_Vbi.h"
#if defined(MEDIAINFO_EIA608_YES)
    #include "MediaInfo/Text/File_Eia608.h"
#endif
#if defined(MEDIAINFO_TIMECODE_YES)
    #include "MediaInfo/Multiple/File_Gxf_TimeCode.h"
#endif
#if defined(MEDIAINFO_TELETEXT_YES)
    #include "MediaInfo/Text/File_Teletext.h"
#endif
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
using namespace std;
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
namespace MediaInfoLib
{
//---------------------------------------------------------------------------

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Vbi::File_Vbi()
:File__Analyze()
{
    //Configuration
    ParserName="Vbi";
    PTS_DTS_Needed=true;

    LineNumber=0;
    IsLast=false;
}

//---------------------------------------------------------------------------
void File_Vbi::Streams_Finish()
{
    for (auto& Stream : Streams) {
        if (!Stream.second.Parser) {
            continue;
        }
        Finish(Stream.second.Parser);
    }

    for (auto& Stream : Streams) {
        if (!Stream.second.Parser) {
            continue;
        }
        size_t Start[Stream_Max];
        for (size_t StreamKind = Stream_General + 1; StreamKind < Stream_Max; StreamKind++) {
            Start[StreamKind] = Count_Get((stream_t)StreamKind);
        }
        Merge(*Stream.second.Parser);
        for (size_t StreamKind = Stream_General + 1; StreamKind < Stream_Max; StreamKind++) {
            auto End = Count_Get((stream_t)StreamKind);
            for (size_t StreamPos = Start[StreamKind]; StreamPos < End; StreamPos++) {
                auto ID = Stream.first >= 313 ? (Stream.first - 313) : Stream.first >= 263 ? (Stream.first - 263) : Stream.first;
                auto ID_String = Ztring::ToZtring(ID);
                const auto& SubID = Retrieve_Const((stream_t)StreamKind, StreamPos, General_ID);
                if (!SubID.empty()) {
                    ID_String += __T('-');
                    ID_String += SubID;
                }
                Fill((stream_t)StreamKind, StreamPos, General_ID, ID_String, true);
                if (Stream.second.Type) {
                    Fill((stream_t)StreamKind, StreamPos, "MuxingMode", "VBI");
                    if (Stream.second.Type == VbiType_Vitc) {
                        Fill(Stream_Other, StreamPos, Other_Format, "SMPTE TC"); // TODO: in timecode parser directly
                        Fill(Stream_Other, StreamPos, Other_BitRate_Mode, "CBR"); // TODO: in timecode parser directly
                        auto Field2 = Streams.find(ID + 263); // NTSC
                        if (Field2 == Streams.end() || Field2->second.Type != Stream.second.Type) {
                            Field2 = Streams.find(ID + 313); //PAL
                        }
                        if (Field2 != Streams.end() && Field2->second.Type == Stream.second.Type) {
                            const auto& TimeCode_FirstFrame1 = Stream.second.Parser->Retrieve_Const(Stream_Other, 0, Other_TimeCode_FirstFrame);
                            const auto& TimeCode_FirstFrame2 = Field2->second.Parser->Retrieve_Const(Stream_Other, 0, Other_TimeCode_FirstFrame);
                            if (TimeCode_FirstFrame2 != TimeCode_FirstFrame1) {
                                Fill(Stream_Other, StreamPos, Other_TimeCode_FirstFrame, TimeCode_FirstFrame2);
                            }
                            Streams.erase(Field2);
                        }
                    }
                    if (Stream.second.Type == VbiType_Teletext) {
                        auto Field2 = Streams.find(ID + 263); // NTSC
                        if (Field2 == Streams.end() || Field2->second.Type != Stream.second.Type) {
                            Field2 = Streams.find(ID + 313); //PAL
                        }
                        if (Field2 != Streams.end() && Field2->second.Type == Stream.second.Type) {
                            const auto& ID_FirstFrame1 = Stream.second.Parser->Retrieve_Const(StreamKind_Last, 0, General_ID);
                            const auto& ID_FirstFrame2 = Field2->second.Parser->Retrieve_Const(StreamKind_Last, 0, General_ID);
                            if (ID_FirstFrame1 == ID_FirstFrame2) {
                                Streams.erase(Field2);
                            }
                        }
                    }
                }
            }
        }
    }
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Vbi::Read_Buffer_Continue()
{
    if (!Status[IsAccepted])
        Accept(); //No way to detect non-VBI content

    if (WrappingType && WrappingType <= 4 && SampleCoding == 4) { // 8-bit component luma samples, the only one currently supported
        if (WrappingType == 3 && LineNumber <= 263) {
            LineNumber += 263; // Some muxers indicate field line, some others interlaced frame line
        }
        Parse();
    }

    if (Element_Offset < Element_Size)
        Skip_XX(Element_Size - Element_Offset,                  "Unknown");

    if (IsLast)
    {
        Frame_Count++;
        Frame_Count_InThisBlock++;
        if (Frame_Count_NotParsedIncluded != (int64u)-1)
            Frame_Count_NotParsedIncluded++;
        if (!Status[IsFilled] && Config->ParseSpeed <= 0)
            Fill();
        IsLast = false;
    }
}

//---------------------------------------------------------------------------
void File_Vbi::Read_Buffer_Unsynched()
{
    for (auto& Stream : Streams) {
        if (Stream.second.Parser) {
            Stream.second.Parser->Open_Buffer_Unsynch();
        }
    }
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Vbi::Parse()
{
    auto Stream = Streams.find(LineNumber);
    if (Stream != Streams.end()) {
        switch (Stream->second.Type) {
        case VbiType_Line21: Line21(); break;
        case VbiType_Vitc: Vitc(); break;
        case VbiType_Teletext: Teletext(); break;
        default:;
        }
    }
    else {
        Line21();
        Vitc();
        Teletext();
    }
}

//---------------------------------------------------------------------------
void File_Vbi::Line21()
{
    // Check luminance min & max
    int8u Level_Min = (int8u)-1;
    int8u Level_Max = 0;
    for (int i = 0; i < Buffer_Size; i++) {
        auto Value = Buffer[i];
        Level_Min = min(Level_Min, Value);
        Level_Max = max(Level_Max, Value);
    }
    if (Level_Max - Level_Min < 16)
        return; // Not enough difference in luminance for having an actual signal
    int8u Diff4 = (Level_Max - Level_Min) >> 2;
    int8u Level_0 = Level_Min + Diff4;
    int8u Level_1 = Level_Max - Diff4;

    // Find Clock Run-In
    int ClockRunIn_Pos = 0;
    size_t ClockRunIn_0_x2 = 0;
    size_t Step_x12 = 0; // Step between each bit
    size_t Current_Pos_Min = 0; // Min index of the lowest (0) or highest (1) current value
    size_t Current_Pos_Max = 0; // Max index of the lowest (0) or highest (1) current value
    int8u Current_Value = (int8u)-1;
    bool Is1 = false;
    for (int i = 0; i < Buffer_Size; i++) {
        auto Value = Buffer[i];
        if (Value <= Level_0) {
            // Is 0
            if (Is1) {
                // New 0
                if (!ClockRunIn_Pos) {
                    ClockRunIn_0_x2 = Current_Pos_Min + Current_Pos_Max; // First Clock Run-In is found
                }
                ClockRunIn_Pos++;
                if (ClockRunIn_Pos == 7) {
                    break; // The 7 Clock Run-In are found
                }
                Current_Value = Value;
                Current_Pos_Min = i;
                Current_Pos_Max = i;
                Is1 = false;
            }
            else {
                // Another 0
            }
        }
        if (Value >= Level_1) {
            // Is 1
            if (Is1) {
                // Another 1
            }
            else {
                // New 1
                Current_Value = Value;
                Current_Pos_Min = i;
                Current_Pos_Max = i;
                Is1 = true;
            }
        }
        if (Is1) {
            if (Current_Value < Value) {
                Current_Value = Value;
                Current_Pos_Min = i;
                Current_Pos_Max = i;
            }
            else if (Current_Value == Value) {
                Current_Pos_Max = i;
            }
        }
        else {
            if (Current_Value > Value) {
                Current_Value = Value;
                Current_Pos_Min = i;
                Current_Pos_Max = i;
            }
            else if (Current_Value == Value) {
                Current_Pos_Max = i;
            }
        }
    }

    // Compute step between bits
    Step_x12 = Current_Pos_Min + Current_Pos_Max - ClockRunIn_0_x2;
    if (ClockRunIn_Pos != 7 || Step_x12 / 12 > ClockRunIn_0_x2 || Step_x12 * 25 >= Buffer_Size * 12 - ClockRunIn_0_x2 * 6) {
        return; // Clock Run-In elements are not found or not enough place for 26 elements
    }
    auto Get_Buffer_Index = [&](int i) { // i is up to 25
        return (ClockRunIn_0_x2 * 6 + Step_x12 * i) / 12;
    };

    // Check Clock Run-In coherence
    for (int i = 0; i < 7; i++) {
        auto Idx = Get_Buffer_Index(i);
        if (Buffer[Idx] < Level_1) {
            return; // One of Clock Run-In highest is not at the right place
        }
    }
    ClockRunIn_0_x2 -= Step_x12 / 12; // From middle of a bit to start of a bit
    for (int i = 0; i < 7; i++) {
        auto Idx = Get_Buffer_Index(i);
        if (Buffer[Idx] > Level_0) {
            return; // One of Clock Run-In lowest is not at the right place
        }
    }

    // Check Start Bits
    auto Idx_Min = Get_Buffer_Index(7); // Begin of S1
    ClockRunIn_0_x2 += Step_x12 / 12; // From start of a bit to middle of a bit
    auto Idx_Max = Get_Buffer_Index(8); // Middle of S2
    for (int Idx = Idx_Min; Idx < Idx_Max; Idx++) {
        if (Buffer[Idx] > Level_0) {
            return; // Issue between begin of S1 and middle of S2, not 0
        }
    }
    auto Idx = Get_Buffer_Index(9);
    if (Buffer[Idx] < Level_1) {
        return; // S3 is not 1
    }

    // Read 2 8-bit characters
    unsigned Pair = 0;
    int8u CC_Parity[2] = {};
    for (int i = 0; i < 16; i++) {
        Pair <<= 1;
        auto Idx = Get_Buffer_Index(10 + i);
        if (Buffer[Idx] >= Level_1) {
            Pair++;
            CC_Parity[i>>3]++;
        }
        else if (Buffer[Idx] > Level_0) {
            return; // Not 0 and not 1, unstable
        }
    }
    if (!(CC_Parity[0] & 1) || !(CC_Parity[1] & 1)) {
        return; // Parity fail
    }

    // Parse the characters
    int8u CC_Buffer[2];
    CC_Buffer[0] = ReverseBits(Pair >> 8);
    CC_Buffer[1] = ReverseBits(Pair & 0xFF);
    auto& Stream = Streams[LineNumber];
    if (!Stream.Parser)
    {
        Stream.Parser = new File_Eia608;
        Stream.Type = VbiType_Line21;
        Open_Buffer_Init(Stream.Parser);
    }
    Open_Buffer_Continue(Stream.Parser, CC_Buffer, 2);
    Element_Offset = Element_Size;
}

//---------------------------------------------------------------------------
void File_Vbi::Vitc()
{
    // Check luminance min & max
    int8u Level_Min = (int8u)-1;
    int8u Level_Max = 0;
    for (int i = 0; i < Buffer_Size; i++) {
        auto Value = Buffer[i];
        Level_Min = min(Level_Min, Value);
        Level_Max = max(Level_Max, Value);
    }
    if (Level_Max - Level_Min < 16)
        return; // Not enough difference in luminance for having an actual signal
    int8u Diff4 = (Level_Max - Level_Min) >> 2;
    int8u Level_0 = Level_Min + Diff4;
    int8u Level_1 = Level_Max - Diff4;

    // Find first sync
    int Sync_Pos = 0;
    size_t Sync_0_Begin = 0;
    size_t Sync_0_End = 0;
    size_t Step_x81 = 0; // Step between each bit
    size_t Current_Pos_PreviousChange = 0; // Min index of the 0 level or the 1 level
    bool Is1 = false;
    for (int i = 0; i < Buffer_Size; i++) {
        auto Value = Buffer[i];
        if (Value <= Level_0) {
            // Is 0
            if (Is1) {
                // New 0
                if (!Sync_Pos) {
                    Sync_0_Begin = Current_Pos_PreviousChange; // First 1 sync
                    Sync_0_End = i;
                }
                Sync_Pos++;
                Step_x81 = i - Sync_0_Begin;
                if (Sync_Pos == 9) {
                    break; // The 9 1 sync are found
                }
                Step_x81--; // Minus 1 because it is better to sync with the previous bit (its value is not important for catching 1 sync) than being too far, especially useful for the 2nd sync
                Current_Pos_PreviousChange = i;
                i = Sync_0_Begin + (10 * Sync_Pos) * Step_x81 / (1 + 10 * (Sync_Pos - 1));
                Is1 = false;
            }
            else {
                // Another 0
            }
        }
        if (Value >= Level_1) {
            // Is 1
            if (Is1) {
                // Another 1
            }
            else {
                // New 1
                Current_Pos_PreviousChange = i;
                Is1 = true;
            }
        }
    }

    // Compute step between bits
    auto Sync_0 = (Sync_0_Begin + Sync_0_End) / 2; // From start of a bit to middle of a bit
    if (Sync_Pos != 9 || Step_x81 * 90 >= (Buffer_Size - Sync_0_Begin) * 81) {
        return; // Sync elements are not found or not enough place for 90 elements
    }
    auto Get_Buffer_Index = [&](int i) { // i is up to 89
        return Sync_0 + (Step_x81 * i) / 81;
    };

    // Check sync bits coherence
    for (int i = 0; i < 9; i++) {
        auto Idx1 = Get_Buffer_Index(i * 10);
        auto Idx0 = Get_Buffer_Index(i * 10 + 1);
        if (Buffer[Idx1] < Level_1 || Buffer[Idx0] > Level_0) {
            return; // One of Clock Run-In highest is not at the right place
        }
    }

    // Read 2 8-bit characters
    uint64_t TimeCode_UserBits = 0;
    uint8_t Parity = 1; // 1 of the CRC bits uses 3x a sync code instead of 2x
    for (int i = 0; i < 72; i++) {
        if (i < 64) {
            TimeCode_UserBits <<= 1;
        }
        auto i_Div = i / 8;
        auto i_Mod = i % 8;
        auto Pos = 2 + i_Div * 10 + i_Mod;
        auto Idx = Get_Buffer_Index(Pos);
        if (Buffer[Idx] >= Level_1) {
            if (i < 64) {
                TimeCode_UserBits++;
            }
            Parity ^= 1 << (Pos % 8);
        }
        else if (Buffer[Idx] > Level_0) {
            return; // Not 0 and not 1, unstable
        }
    }
    if (Parity) {
        return; // Parity fail
    }

    // Parse the characters
    int8u TimeCode_UserBits_Buffer[8];
    for (int i = 0; i < 8; i++) {
        TimeCode_UserBits_Buffer[i] = ReverseBits((TimeCode_UserBits >> (8 * i)) & 0xFF);
    }
    auto& Stream = Streams[LineNumber];
    if (!Stream.Parser)
    {
        Stream.Parser = new File_Gxf_TimeCode;
        ((File_Gxf_TimeCode*)Stream.Parser)->IsTimeCodeTrack = true;
        ((File_Gxf_TimeCode*)Stream.Parser)->IsBigEndian = true;
        Stream.Type = VbiType_Vitc;
        Open_Buffer_Init(Stream.Parser);
    }
    Open_Buffer_Continue(Stream.Parser, TimeCode_UserBits_Buffer, 8);
    Element_Offset = Element_Size;
}

//---------------------------------------------------------------------------
void File_Vbi::Teletext()
{
    // Check luminance min & max
    int8u Level_Min = (int8u)-1;
    int8u Level_Max = 0;
    for (int i = 0; i < Buffer_Size; i++) {
        auto Value = Buffer[i];
        Level_Min = min(Level_Min, Value);
        Level_Max = max(Level_Max, Value);
    }
    if (Level_Max - Level_Min < 16)
        return; // Not enough difference in luminance for having an actual signal
    int8u Middle = (Level_Min + Level_Max) >> 1;

    // Find first and last 1 bit
    int8u First_Highest = 0;
    size_t First_Pos = 0;
    for (; First_Pos < Buffer_Size; First_Pos++) {
        auto Value = Buffer[First_Pos];
        if (Value <= First_Highest && First_Highest >= Middle) {
            First_Pos--;
            break;
        }
        First_Highest = Value;
    }
    int8u Last_Highest = 0;
    size_t Last_Pos = Buffer_Size - 1;
    for (; Last_Pos; Last_Pos--) {
        auto Value = Buffer[Last_Pos];
        if (Value <= Last_Highest && Last_Highest >= Middle) {
            Last_Pos++;
            break;
        }
        Last_Highest = Value;
    }
    if (Last_Pos <= First_Pos) {
        return;
    }

    // Guess the precise position of the peak based on adjacent bytes
    auto Get_PrecisePos = [&](const int8u* Buffer, size_t i) {
        if (i + 1 >= Buffer_Size) {
            return (float)Buffer[i];
        }
        if (i && Buffer[i - 1] > Buffer[i + 1]) {
            auto Diff0 = Level_Max - Buffer[i - 1];
            auto Diff1 = Level_Max - Buffer[i];
            float Sum = Diff0 + Diff1;
            float Pos = i;
            if (Sum) {
                Pos -= Diff1 / Sum;
            }
            return Pos;
        }
        else {
            auto Diff0 = Level_Max - Buffer[i];
            auto Diff1 = Level_Max - Buffer[i + 1];
            float Sum = Diff0 + Diff1;
            float Pos = i;
            if (Sum) {
                Pos += Diff0 / Sum;
            }
            return Pos;
        }
    };

    // Guess precise first 1 bit and last 1 bit
    auto First1BitPos = Get_PrecisePos(Buffer, First_Pos);
    auto Last1BitPos = Get_PrecisePos(Buffer, Last_Pos);

    // Compute step between bits
    auto Step = (Last1BitPos - First1BitPos) / 357; // Most frames have first 1 bit the first clock bit and last 1 bit the checksum of 0 padding so 3rd last bit
    if (Step < 1 || Step > 2 || First1BitPos + Step * 359 + 0.5 >= Buffer_Size) {
        return; // Not enough place for 360 elements
    }
    auto Get_Value = [&](int i) { // i is up to 359
        auto Idx = First1BitPos + Step * i;
        auto Idx_Int = (size_t)Idx;
        auto Value = Buffer[Idx_Int];
        Idx_Int++;
        if (Idx_Int < Buffer_Size) {
            auto Diff = (float)Buffer[Idx_Int] - Value;
            Idx -= (size_t)Idx;
            Value += Idx * Diff;
        }
        return Value;
    };

    //auto ID = LineNumber >= 313 ? (LineNumber - 313) : LineNumber >= 263 ? (LineNumber - 263) : LineNumber;
    //auto& Stream = Streams[ID];
    auto& Stream = Streams[LineNumber];

    // Read 45 8-bit characters
    int8u Teletext_Buffer[45];
    auto Dump = [&]() {
        int8u Current = 0;
        for (int i = 0; i < 360; i++) {
            auto Value = Get_Value(i);
            Current <<= 1;
            if (Value >= Middle) {
                Current++;
            }
            if ((i % 8) == 7) {
                Teletext_Buffer[i / 8] = ReverseBits(Current);
            }
        }
    };
    Dump();
    if (BigEndian2int24u(Teletext_Buffer) == 0x555527 && BigEndian2int32u(Teletext_Buffer + 41) == 0x20202020) {
        // Store pos when Clock Run-In and Framing Code as well of last 0 padding bytes are found
        // 0 = frame count, 1 = sum of first 1 bit, 2 = sum of last 1 bit
        Stream.Private[0]++;
        Stream.Private[1] += First1BitPos;
        Stream.Private[2] += Last1BitPos;
    }
    else if (Stream.Private[0]) {
        // Ever it is not Teletext or no last 0 padding bytes so more difficult to find the exact step, using previous values
        First1BitPos = Stream.Private[1] / Stream.Private[0];
        Last1BitPos = Stream.Private[2] / Stream.Private[0];
        Step = (Last1BitPos - First1BitPos) / 357;
        Dump();
        if (BigEndian2int24u(Teletext_Buffer) != 0x555527) {
            return; // Clock Run-In and Framing Code not found
        }
    }
    else {
        return; // Clock Run-In and Framing Code not found
    }

    // Parse the characters
    if (!Stream.Parser)
    {
        Stream.Parser = new File_Teletext;
        Stream.Type = VbiType_Teletext;
        Open_Buffer_Init(Stream.Parser);
    }
    Open_Buffer_Continue(Stream.Parser, Teletext_Buffer, 45);
    Element_Offset = Element_Size;
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_VBI_YES


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
#if defined(MEDIAINFO_SCC_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Text/File_Scc.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include "MediaInfo/Text/File_Eia608.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Utils
//***************************************************************************

//---------------------------------------------------------------------------
static unsigned char Char2Hex (unsigned char Char)
{
         if (Char<='9' && Char>='0')
        Char-='0';
    else if (Char<='f' && Char>='a')
        Char-='a'-10;
    else if (Char<='F' && Char>='A')
        Char-='A'-10;
    else
        Char =0;
    return Char;
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Scc::File_Scc()
:File__Analyze()
{
    //Configuration
    ParserName="SCC";
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_Scc;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
    PTS_DTS_Needed=true;

    //Temp
    Parser=NULL;
}

//---------------------------------------------------------------------------
File_Scc::~File_Scc()
{
    delete Parser; //Parser=NULL;
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_SEEK
size_t File_Scc::Read_Buffer_Seek (size_t Method, int64u Value, int64u ID)
{
    GoTo(0);
    Open_Buffer_Unsynch();
    return 1;
}
#endif //MEDIAINFO_SEEK

//---------------------------------------------------------------------------
#if MEDIAINFO_SEEK
void File_Scc::Read_Buffer_Unsynched()
{
    if (Parser)
        Parser->Open_Buffer_Unsynch();
}
#endif //MEDIAINFO_SEEK

//---------------------------------------------------------------------------
#if MEDIAINFO_SEEK
void File_Scc::Read_Buffer_AfterParsing()
{
}
#endif //MEDIAINFO_SEEK

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Scc::Streams_Finish()
{
    if (TimeCode_FirstFrame.GetFramesMax() && Frame_Count_NotParsedIncluded!=(int64u)-1)
        Fill(Stream_Text, 0, Text_TimeCode_LastFrame, (TimeCode_FirstFrame+(Frame_Count_NotParsedIncluded-1)).ToString());
    if (Parser && Parser->Status[IsAccepted])
    {
        Finish(Parser);
        for (size_t Pos2=0; Pos2<Parser->Count_Get(Stream_Text); Pos2++)
        {
            Stream_Prepare(Stream_Text);
            Merge(*Parser, Stream_Text, StreamPos_Last, Pos2);
            Fill(Stream_Text, StreamPos_Last, Text_ID, Parser->Retrieve(Stream_Text, Pos2, Text_ID), true);
            if (Pos2)
            {
                Fill(Stream_Text, StreamPos_Last, Text_TimeCode_FirstFrame, Retrieve_Const(Stream_Text, 0, Text_TimeCode_FirstFrame));
                Fill(Stream_Text, StreamPos_Last, Text_TimeCode_LastFrame, Retrieve_Const(Stream_Text, 0, Text_TimeCode_LastFrame));
            }
        }
    }
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Scc::FileHeader_Begin()
{
    //Element_Size
    if (File_Size<22)
    {
        Reject("N19");
        return false;
    }
    if (Buffer_Size<22)
        return false; //Must wait for more data

    if (Buffer[ 0]!=0x53
     || Buffer[ 1]!=0x63
     || Buffer[ 2]!=0x65
     || Buffer[ 3]!=0x6E
     || Buffer[ 4]!=0x61
     || Buffer[ 5]!=0x72
     || Buffer[ 6]!=0x69
     || Buffer[ 7]!=0x73
     || Buffer[ 8]!=0x74
     || Buffer[ 9]!=0x5F
     || Buffer[10]!=0x53
     || Buffer[11]!=0x43
     || Buffer[12]!=0x43
     || Buffer[13]!=0x20
     || Buffer[14]!=0x56
     || Buffer[15]!=0x31
     || Buffer[16]!=0x2E
     || Buffer[17]!=0x30
    )
    {
        Reject("SCC");
        return false;
    }

    //Element_Size
    if (Buffer_Size<File_Size)
        return false; //Must wait for more data

    //All should be OK...
    return true;
}

//---------------------------------------------------------------------------
void File_Scc::FileHeader_Parse()
{
    Skip_String(18,                                             "Magic");
    while (Element_Offset<Buffer_Size)
    {
        if (Buffer[(size_t)Element_Offset]!=0x0D && Buffer[(size_t)Element_Offset]!=0x0A)
            break;
        Element_Offset++;
    }

    Accept();
    Fill(Stream_General, 0, General_Format, "SCC");

    //Init
    Parser=new File_Eia608();
    Open_Buffer_Init(Parser);

    // Trying to detect frame rate
    vector<TimeCode> TimeCodes;
    vector<int8u> FrameRates;
    FrameRates.push_back(24);
    FrameRates.push_back(25);
    FrameRates.push_back(30);
    int8u FrameRateSlowDown[2];
    FrameRateSlowDown[0]=6;
    FrameRateSlowDown[1]=5;
    int8u HighestFrame=0;
    int8u HighestFrame_Adapted=0;
    size_t Begin=Buffer_Offset+(size_t)Element_Offset;
    for (size_t End=Begin; End<Buffer_Size; End++)
    {
        if (Buffer[End]!=0x0D && Buffer[End]!=0x0A)
            continue;
        if (End-Begin>=11)
        {
            size_t Space=Begin;
            while (Space<End && Buffer[Space]!=' ' && Buffer[Space]!='\t')
                Space++;
            TimeCode Temp(TimeCode::string_view((const char*)Buffer+Begin, Space-Begin), -1);
            if (Temp.IsSet())
            {
                if (HighestFrame<Temp.GetFrames())
                    HighestFrame=Temp.GetFrames();
                if (HighestFrame_Adapted<Temp.GetFrames())
                    HighestFrame_Adapted=Temp.GetFrames();
                if (TimeCodes.empty())
                {
                    Fill(Stream_Text, 0, Text_TimeCode_FirstFrame, string((const char*)Buffer+Begin, 11), true, true);
                    TimeCode_FirstFrame=Temp;
                    Fill(Stream_Text, 0, Text_TimeCode_Source, "Container", Unlimited, true, true);
                    for (size_t i=0; i<FrameRates.size(); i++)
                    {
                        Temp.SetFramesMax(FrameRates[i]-1);
                        Temp.SetDropFrame(Buffer[Begin+8]==';');
                        TimeCodes.push_back(Temp);
                    }
                }
                else
                {
                    for (size_t i=0; i<TimeCodes.size(); i++)
                    {
                        auto Temp2(Temp);
                        Temp2.SetFramesMax(TimeCodes[i].GetFramesMax());
                        if (Temp2<TimeCodes[i])
                            {
                                int8u NewHighestFrame=TimeCodes[i].GetFramesMax();
                                TimeCodes[i]--; // Time code of the last frame
                                while (TimeCodes[i].GetFrames()!=TimeCodes[i].GetFramesMax())
                                {
                                    NewHighestFrame++;
                                    TimeCodes[i]--;
                                }
                                if (HighestFrame_Adapted<NewHighestFrame)
                                    HighestFrame_Adapted=NewHighestFrame;
                                TimeCodes.erase(TimeCodes.begin()+i);
                                i--;
                            }
                    }
                }

                Begin+=11;
                for(; Begin+5<=End; Begin+=5)
                {
                    for (size_t i=0; i<TimeCodes.size(); i++)
                    {
                        if (i<=1)
                        {
                            FrameRateSlowDown[i]++;
                            if (FrameRateSlowDown[i]>6-i)
                            {
                                FrameRateSlowDown[i]=0;
                                continue;
                            }
                        }
                        TimeCodes[i]++;
                    }
                }
            }
        }
        while (End<Buffer_Size && (Buffer[End]==0x0D || Buffer[End]==0x0A))
            End++;
        Begin=End;
    }
    if (HighestFrame)
    {
        Fill(Stream_Text, 0, Text_TimeCode_MaxFrameNumber, HighestFrame);
        Fill(Stream_Text, 0, Text_TimeCode_MaxFrameNumber_Theory, HighestFrame_Adapted);
    }
    #if MEDIAINFO_ADVANCED
        float64 FrameRate_F=Video_FrameRate_Rounded(Config->File_DefaultFrameRate_Get());
        if (!FrameRate_F)
            FrameRate_F=30/1.001;
    #else //MEDIAINFO_ADVANCED
        const float64 FrameRate_F=30/1.001;
    #endif //MEDIAINFO_ADVANCED
    FrameDurationNanoSeconds=float64_int64s(1000000000/FrameRate_F);
    if (FrameRate_F>0 && FrameRate_F<0x100)
    {
        FrameRate=(int8u)float64_int64s(FrameRate_F);
        FrameRate_Is1001=FrameRate_F!=FrameRate;
    }
    else
        FrameRate=0;
    Fill(Stream_Text, 0, Text_FrameRate, FrameRate_F);
    TimeCode_FirstFrame.SetFramesMax(FrameRate-1);
    TimeCode_FirstFrame.Set1001fps(FrameRate_Is1001);
    Fill(Stream_Text, 0, Text_Delay, (int64s)TimeCode_FirstFrame.ToMilliseconds());
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Scc::Header_Parse()
{
    size_t End=Buffer_Offset;

    //Content
    while (End<Buffer_Size)
    {
        if (Buffer[End]==0x0D || Buffer[End]==0x0A)
            break;
        End++;
    }

    //EOL
    while (End<Buffer_Size)
    {
        if (Buffer[End]!=0x0D && Buffer[End]!=0x0A)
            break;
        End++;
    }

    //Filling
    Header_Fill_Size(End-Buffer_Offset);
    Header_Fill_Code(0, __T("Block"));
}

//---------------------------------------------------------------------------
void File_Scc::Data_Parse()
{
    while (Element_Offset<Element_Size && (Buffer[Buffer_Offset+(size_t)Element_Offset]==0x0D || Buffer[Buffer_Offset+(size_t)Element_Offset]==0x0A))
        Element_Offset++;
    if (Element_Offset==Element_Size)
        return;

    //Parsing
    string TimeStamp;
    Get_String(11, TimeStamp,                                   "TimeStamp");
    TimeCode Temp(TimeStamp, FrameRate-1, TimeCode::FPS1001(FrameRate_Is1001));
    Frame_Count_NotParsedIncluded=Temp.ToFrames()-TimeCode_FirstFrame.ToFrames();
    Parser->FrameInfo.DTS=Temp.ToMilliseconds()*1000000;
    Parser->FrameInfo.DUR=FrameDurationNanoSeconds;
    while (Element_Offset+5<=Element_Size)
    {
        int8u Buffer_Temp[2];
        Buffer_Temp[0]=Char2Hex(Buffer[Buffer_Offset+(size_t)Element_Offset+1])<<4
                     | Char2Hex(Buffer[Buffer_Offset+(size_t)Element_Offset+2]);
        Buffer_Temp[1]=Char2Hex(Buffer[Buffer_Offset+(size_t)Element_Offset+3])<<4
                     | Char2Hex(Buffer[Buffer_Offset+(size_t)Element_Offset+4]);
        Open_Buffer_Continue(Parser, Buffer_Temp, 2);
        Element_Offset+=5;
        Frame_Count_NotParsedIncluded=Parser->Frame_Count_NotParsedIncluded;
    }
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_SCC_YES

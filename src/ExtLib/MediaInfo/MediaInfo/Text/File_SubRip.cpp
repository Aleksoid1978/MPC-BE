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
#if defined(MEDIAINFO_SUBRIP_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Text/File_SubRip.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#if MEDIAINFO_EVENTS
    #include "MediaInfo/MediaInfo_Events_Internal.h"
#endif //MEDIAINFO_EVENTS
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Utils
//***************************************************************************

int64u SubRip_str2timecode(const char* Value)
{
    size_t Length=strlen(Value);

    //Xs
    if (Length
     && Value[Length-1]=='s')
        return (int64u)float64_int64s(atof(Value) * 1000000000);

    //Test if HH: is present
    if (Length<5)
        return 0;
    int64u ToReturn;
    if (Length!=5 && Value[5]==':')
    {
        if (Value[0]<'0' || Value[0]>'9'
         || Value[1]<'0' || Value[1]>'9'
         || Value[2]!=':')
            return 0;
        ToReturn=(int64u)(Value[0]-'0')*10*60*60*1000000000
                +(int64u)(Value[1]-'0')   *60*60*1000000000;
        Value+=3;
        Length-=3;
    }
    else
        ToReturn=0;
    if (Length<5
     || Value[0]<'0' || Value[0]>'9'
     || Value[1]<'0' || Value[1]>'9'
     || Value[2]!=':'
     || Value[3]<'0' || Value[3]>'9'
     || Value[4]<'0' || Value[4]>'9')
        return 0;
    ToReturn+=(int64u)(Value[0]-'0')   *10*60*1000000000
             +(int64u)(Value[1]-'0')      *60*1000000000
             +(int64u)(Value[3]-'0')      *10*1000000000
             +(int64u)(Value[4]-'0')         *1000000000;
    if (Length>5 && (Value[5]=='.' || Value[5]==','))
    {
        if (Length>6+9)
            Length=6+9; //Nanoseconds max
        const char* Value_End=Value+Length;
        Value+=6;
        int64u Multiplier=100000000;
        while (Value<Value_End && *Value>='0' && *Value<='9')
        {
            ToReturn+=(int64u)(*Value-'0')*Multiplier;
            Multiplier/=10;
            Value++;
        }
    }

    return ToReturn;
}

//---------------------------------------------------------------------------
struct timeline
{
    int64u      Time_Begin;
    int64u      Time_End;
    size_t      LineCount;

    timeline(int64u Time_Begin_, int64u Time_End_, size_t LineCount_)
        : Time_Begin(Time_Begin_)
        , Time_End(Time_End_)
        , LineCount(LineCount_)
    {}
};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_SubRip::File_SubRip()
:File__Analyze()
{
    //Configuration
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_SubRip;
        StreamIDs_Width[0]=0;
    #endif //MEDIAINFO_EVENTS

    //Init
    Frame_Count=0;

    //Temp
    IsVTT=false;
    HasBOM=false;
    #if MEDIAINFO_DEMUX
        Items_Pos=0;
    #endif //MEDIAINFO_DEMUX
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_SubRip::FileHeader_Begin()
{
    size_t MaxSize=Config->ParseSpeed>0.5?0x100000:0x1000;

    if (!IsSub && (Buffer_Size<File_Size && Buffer_Size<MaxSize))
    {
        Element_WaitForMoreData();
        return false;
    }

    ZtringListList List;
    List.Separator_Set(0, __T("\n\n"));
    List.Separator_Set(1, __T("\n"));

    if (Buffer_Size>=3
     && Buffer[0]==0xEF
     && Buffer[1]==0xBB
     && Buffer[2]==0xBF)
        HasBOM=true;
    bool IsLocal=false;
    Ztring Temp;
    Temp.From_UTF8((const char*)Buffer+(HasBOM?3:0), (Buffer_Size>MaxSize?MaxSize:Buffer_Size)-(HasBOM?3:0));
    if (Temp.empty())
    {
        #ifdef WINDOWS
        Temp.From_Local((const char*)Buffer+(HasBOM?3:0), (Buffer_Size>MaxSize?MaxSize:Buffer_Size)-(HasBOM?3:0)); // Trying from local code page
        #else //WINDOWS
        Temp.From_ISO_8859_1((const char*)Buffer+(HasBOM?3:0), (Buffer_Size>MaxSize?MaxSize:Buffer_Size)-(HasBOM?3:0));
        #endif //WINDOWS
        IsLocal=true;
    }
    Temp.FindAndReplace(__T("\r\n"), __T("\n"), 0, Ztring_Recursive);
    Temp.FindAndReplace(__T("\r"), __T("\n"), 0, Ztring_Recursive);
    List.Quote_Set(__T("\r")); // Should be empty string for indicating that there is no quote sepcial char, but old versions of ZenLib are buggy
    List.Write(Temp);
    Temp = List.Read();

    Ztring const& First=List(0, 0);
    if (First.rfind(__T("WEBVTT"))==0 && (First.size()==6 || First[6]==' ' || First[6]=='\t'))
        IsVTT=true;

    if (!IsVTT)
    {
        size_t IsOk=0;
        size_t IsNok=0;
        int64u Number=1;
        for (size_t Pos=0; Pos<List.size(); Pos++)
        {
            int64u NewNumber=List(Pos, 0).To_int64u();
            if (NewNumber==Number)
                IsOk++;
            else
            {
                IsNok++;
                Number=NewNumber;
            }
            Number++;

            if (List(Pos, 1).size()>22 && List(Pos, 1)[2]==__T(':') && List(Pos, 1)[5]==__T(':') && List(Pos, 1).find(__T(" --> "))!=string::npos)
                IsOk++;
            else
                IsNok++;
        }

        if (!IsOk || IsNok>IsOk/2)
        {
            Reject();
            return true;
        }
    }

    if (!IsSub && File_Size!=(int64u)-1 && Buffer_Size!=File_Size)
    {
        Element_WaitForMoreData();
        return false;
    }

    if (!Status[IsAccepted])
    {
        Accept();
        Fill(Stream_General, 0, General_Format, IsVTT?"WebVTT":"SubRip");
        Stream_Prepare(Stream_Text);
        Fill(Stream_Text, 0, "Format", IsVTT?"WebVTT":"SubRip");
        Fill(Stream_Text, 0, "Codec", IsVTT?"WebVTT":"SubRip");
    }

    if (IsLocal)
        #ifdef WINDOWS
        Temp.From_Local((const char*)Buffer+(HasBOM?3:0), Buffer_Size-(HasBOM?3:0));
        #else //WINDOWS
        Temp.From_ISO_8859_1((const char*)Buffer+(HasBOM?3:0), Buffer_Size-(HasBOM?3:0));
        #endif //WINDOWS
    else
        Temp.From_UTF8((const char*)Buffer+(HasBOM?3:0), Buffer_Size-(HasBOM?3:0));
    Temp.FindAndReplace(__T("\r\n"), __T("\n"), 0, Ztring_Recursive);
    Temp.FindAndReplace(__T("\r"), __T("\n"), 0, Ztring_Recursive);
    List.Write(Temp);

    int64u PTS_Begin_Min=(int64u)-1;
    int64u PTS_End_Max=0;
    size_t Events_Total=0;
    int64u Events_MinDuration=(int64u)-1;
    size_t Lines_Count=0;
    size_t EmptyCount=0;
    size_t LineMaxCountPerEvent=0;
    vector<timeline> TimeLine;

    for (size_t Pos=0; Pos<List.size(); Pos++)
    {
        if (!List[Pos].empty())
        {
            size_t Separator_Pos=List[Pos][0].find(__T("-->"));
            size_t Data_StartLine;
            if (Separator_Pos==(size_t)-1 && List[Pos].size()>1)
            {
                Separator_Pos=List[Pos][1].find(__T("-->"));
                Data_StartLine=1;
            }
            else
                Data_StartLine=0;
            if (Separator_Pos!=(size_t)-1)
            {
                Ztring PTS_Begin_String=List[Pos][Data_StartLine].substr(0, Separator_Pos);
                while (!PTS_Begin_String.empty() && (PTS_Begin_String.back()==' ' || PTS_Begin_String.back()=='\t'))
                    PTS_Begin_String.pop_back();
                Ztring PTS_End_String =List[Pos][Data_StartLine].substr(Separator_Pos+3);
                while (!PTS_End_String.empty() && (PTS_End_String.front()==' ' || PTS_End_String.front()=='\t'))
                    PTS_End_String.erase(PTS_End_String.begin());
                Data_StartLine++;
                size_t Extra_Pos=PTS_End_String.find_first_of(__T("\t "));
                if (Extra_Pos!=string::npos)
                    PTS_End_String.resize(Extra_Pos); //Discarding positioning
                int64u Time_Begin_New=SubRip_str2timecode(PTS_Begin_String.To_UTF8().c_str());
                int64u Time_End_New=SubRip_str2timecode(PTS_End_String.To_UTF8().c_str());
                size_t LineCount_New=0;
                #if MEDIAINFO_DEMUX
                    Ztring Content;
                #endif //MEDIAINFO_DEMUX
                for (size_t Pos2=Data_StartLine; Pos2<List[Pos].size(); Pos2++)
                {
                    #if MEDIAINFO_DEMUX
                        List[Pos][Pos2].Trim();
                        Content+=List[Pos][Pos2];
                        if (Pos2+1<List[Pos].size())
                            Content+=EOL;
                    #endif //MEDIAINFO_DEMUX
                    LineCount_New++;
                }
                if (LineCount_New && Time_Begin_New<Time_End_New)
                {
                    if (PTS_Begin_Min>Time_Begin_New)
                        PTS_Begin_Min=Time_Begin_New;
                    if (PTS_End_Max<Time_End_New)
                        PTS_End_Max=Time_End_New;
                   Lines_Count+=LineCount_New;

                    // Not supporting back to the past
                    for (size_t i=0; i<TimeLine.size(); i++)
                    {
                        if (Time_Begin_New<TimeLine[i].Time_Begin)
                        {
                            TimeLine.erase(TimeLine.begin()+i);
                            i--;
                            continue;
                        }
                    }

                    // Empty
                    if (!TimeLine.empty() && TimeLine[TimeLine.size()-1].Time_End<Time_Begin_New)
                        EmptyCount++;

                    // Checking same times
                    bool HasSameTime=false;
                    for (size_t i=0; i<TimeLine.size(); i++)
                    {
                        if (TimeLine[i].Time_Begin==Time_Begin_New && TimeLine[i].Time_End==Time_End_New)
                        {
                            TimeLine[i].LineCount+=LineCount_New;
                            HasSameTime=true;
                        }
                    }

                    // Checking overlappings
                    if (!HasSameTime)
                    {
                        size_t CreateFrame_Begin=1;
                        size_t CreateFrame_End=0;
                        for (size_t i=0; i<TimeLine.size(); i++)
                        {
                            if (Time_Begin_New==TimeLine[i].Time_Begin
                             || Time_Begin_New==TimeLine[i].Time_End)
                                CreateFrame_Begin=0;
                            if (Time_End_New!=TimeLine[i].Time_Begin
                             && Time_End_New!=TimeLine[i].Time_End)
                            {
                                CreateFrame_End=1;
                                int64u Duration;
                                if (Time_Begin_New>TimeLine[i].Time_Begin)
                                {
                                    Duration=Time_Begin_New-TimeLine[i].Time_Begin;
                                    if (Events_MinDuration>Duration)
                                        Events_MinDuration=Duration;
                                }
                                if (Time_Begin_New<TimeLine[i].Time_End)
                                {
                                    Duration=TimeLine[i].Time_End-Time_Begin_New;
                                    if (Events_MinDuration>Duration)
                                        Events_MinDuration=Duration;
                                }
                            }
                        }
                        Events_Total+=CreateFrame_Begin;
                        Events_Total+=CreateFrame_End;
                    }

                    for (size_t i=0; i<TimeLine.size(); i++)
                    {
                        if (TimeLine[i].Time_End<=Time_Begin_New)
                        {
                            TimeLine.erase(TimeLine.begin()+i);
                            i--;
                            continue;
                        }
                    }

                    if (!HasSameTime)
                        TimeLine.push_back(timeline(Time_Begin_New, Time_End_New, LineCount_New));

                    LineCount_New=0;
                    for (size_t i=0; i<TimeLine.size(); i++)
                    {
                        LineCount_New+=TimeLine[i].LineCount;
                        int64u Duration;
                        if (Time_Begin_New<Time_End_New)
                            Duration=Time_End_New-Time_Begin_New;
                        else
                            Duration=0;
                        if (Events_MinDuration>Duration)
                            Events_MinDuration=Duration;
                    }
                }

                if (LineMaxCountPerEvent<LineCount_New)
                    LineMaxCountPerEvent=LineCount_New;

                #if MEDIAINFO_DEMUX
                    item Item;
                    Item.PTS_Begin=Time_Begin_New;
                    Item.PTS_End=Time_End_New;
                    Item.Content=std::move(Content);
                    Items.push_back(Item);
                #endif //MEDIAINFO_DEMUX
            }
        }
    }

    //Filling
    if (PTS_End_Max>=PTS_Begin_Min)
        Fill(Stream_Text, 0, Text_Duration, (PTS_End_Max-PTS_Begin_Min+500000)/1000000);
    if (PTS_Begin_Min!=(int64u)-1)
        Fill(Stream_Text, 0, Text_Duration_Start, (PTS_Begin_Min+500000)/1000000);
    if (PTS_End_Max)
        Fill(Stream_Text, 0, Text_Duration_End, (PTS_End_Max+500000)/1000000);
    Fill(Stream_Text, 0, Text_Events_Total, Events_Total-EmptyCount);
    if (Events_MinDuration!=(int64u)-1)
        Fill(Stream_Text, 0, Text_Events_MinDuration, (Events_MinDuration+500000)/1000000);
    Fill(Stream_Text, 0, Text_Lines_Count, Lines_Count);
    if (Events_Total!=EmptyCount)
        Fill(Stream_Text, 0, Text_Lines_MaxCountPerEvent, LineMaxCountPerEvent);

    return true;
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_SEEK
size_t File_SubRip::Read_Buffer_Seek (size_t Method, int64u Value, int64u ID)
{
    GoTo(0);
    #if MEDIAINFO_DEMUX
        Items_Pos=0;
    #endif //MEDIAINFO_DEMUX
    Open_Buffer_Unsynch();
    return 1;
}
#endif //MEDIAINFO_SEEK

//---------------------------------------------------------------------------
void File_SubRip::Read_Buffer_Continue()
{
    #if MEDIAINFO_DEMUX
        if (Buffer)
            Demux(Buffer+((HasBOM && Buffer_Size>=3)?3:0), Buffer_Size-((HasBOM && Buffer_Size>=3)?3:0), ContentType_MainStream);
    #endif //MEDIAINFO_DEMUX

    // Output
    #if MEDIAINFO_EVENTS
        for (; Items_Pos<Items.size(); Items_Pos++)
        {
            Frame_Count_NotParsedIncluded=Frame_Count;
            EVENT_BEGIN(Global, SimpleText, 0)
                std::wstring Content_Unicode{ Items[Items_Pos].Content.To_Unicode() };
                Event.DTS=Items[Items_Pos].PTS_Begin;
                Event.PTS=Event.DTS;
                Event.DUR=Items[Items_Pos].PTS_End-Items[Items_Pos].PTS_Begin;
                Event.Content=Content_Unicode.c_str();
                Event.Flags=IsVTT?1:0;
                Event.MuxingMode=(int8u)-1;
                Event.Service=(int8u)-1;
                Event.Row_Max=0;
                Event.Column_Max=0;
                Event.Row_Values=NULL;
                Event.Row_Attributes=NULL;
            EVENT_END   ()
            
            if (Items_Pos+1==Items.size() || Items[Items_Pos].PTS_End!=Items[Items_Pos+1].PTS_Begin)
            {
                EVENT_BEGIN (Global, SimpleText, 0)
                    Event.DTS=Items[Items_Pos].PTS_End;
                    Event.PTS=Event.DTS;
                    Event.DUR=0;
                    Event.Content=L"";
                    Event.Flags=IsVTT?1:0;
                    Event.MuxingMode=(int8u)-1;
                    Event.Service=(int8u)-1;
                    Event.Row_Max=0;
                    Event.Column_Max=0;
                    Event.Row_Values=NULL;
                    Event.Row_Attributes=NULL;
                EVENT_END   ()
            }

            Frame_Count++;
        }
    #endif //MEDIAINFO_EVENTS

    Buffer_Offset=Buffer_Size;
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_SUBRIP_YES

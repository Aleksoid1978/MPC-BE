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
#include <iterator>
#include <map>
#include <set>
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
// Timeline of currently displayed events, indexed in order to avoid scanning all events for each new event
class SubRip_TimeLine
{
public:
    typedef pair<int64u, int64u> times; // Begin, End

    // Remove events beginning after this time (not supporting back to the past)
    void Remove_BeginAfter(int64u Time)
    {
        while (!ByBegin.empty() && ByBegin.rbegin()->first>Time)
            Remove(*ByBegin.rbegin());
    }

    // Remove events ending at or before this time
    void Remove_EndUpTo(int64u Time)
    {
        while (!ByEnd.empty() && ByEnd.begin()->first<=Time)
            Remove(make_pair(ByEnd.begin()->second, ByEnd.begin()->first));
    }

    // End of the last added event still present
    bool Last_End(int64u& End) const
    {
        if (Ids.empty())
            return false;
        End=Ids.rbegin()->second.second;
        return true;
    }

    // Add lines to the event with same begin and end, return true if there is such event
    bool AddToSameTime(int64u Begin, int64u End, size_t LineCount)
    {
        auto It=Items.find(make_pair(Begin, End));
        if (It==Items.end())
            return false;
        It->second.LineCount+=LineCount;
        LineCount_Total+=LineCount;
        return true;
    }

    // Is there an event beginning or ending at this time
    bool HasBeginOrEndAt(int64u Time) const
    {
        return Count(Count_Begin, Time) || Count(Count_End, Time);
    }

    // Is there an event not ending at this time (no event begins at this time, as all events begin before)
    bool HasEndNotAt(int64u Time) const
    {
        return Items.size()-Count(Count_End, Time)!=0;
    }

    // Latest begin before this time, for events not ending at End
    bool Begin_Before(int64u Time, int64u End, int64u& Begin) const
    {
        auto It=ByBegin.lower_bound(make_pair(Time, (int64u)0));
        if (It==ByBegin.begin())
            return false;
        --It;
        if (It->second==End)
        {
            // Going to the event before all consecutive events ending at End
            auto Run=RunStarts.upper_bound(*It);
            --Run;
            It=ByBegin.find(*Run);
            if (It==ByBegin.begin())
                return false;
            --It;
        }
        Begin=It->first;
        return true;
    }

    // Earliest end after this time, for events not ending at End
    bool End_After(int64u Time, int64u End, int64u& End_Found) const
    {
        auto It=ByEnd.upper_bound(make_pair(Time, (int64u)-1));
        if (It!=ByEnd.end() && It->first==End)
            It=ByEnd.upper_bound(make_pair(End, (int64u)-1));
        if (It==ByEnd.end())
            return false;
        End_Found=It->first;
        return true;
    }

    void Add(int64u Begin, int64u End, size_t LineCount)
    {
        const auto Times=make_pair(Begin, End);
        auto& Item=Items[Times];
        Item.LineCount=LineCount;
        Item.Id=Id_Next++;
        Ids[Item.Id]=Times;
        auto It=ByBegin.insert(Times).first;
        ByEnd.insert(make_pair(End, Begin));
        Count_Begin[Begin]++;
        Count_End[End]++;
        LineCount_Total+=LineCount;

        // Runs of consecutive events (by begin) with the same end
        auto Next=It; ++Next;
        if (It==ByBegin.begin() || prev(It)->second!=End)
            RunStarts.insert(Times);
        if (Next!=ByBegin.end())
        {
            if (Next->second==End)
                RunStarts.erase(*Next);
            else
                RunStarts.insert(*Next);
        }
    }

    size_t LineCount() const
    {
        return LineCount_Total;
    }

private:
    struct item
    {
        size_t      LineCount;
        int64u      Id;
    };
    typedef map<int64u, size_t> counts;
    map<times, item> Items;
    map<int64u, times> Ids; // Key is the insertion order
    set<times> ByBegin;
    set<times> ByEnd; // End, Begin
    set<times> RunStarts;
    counts Count_Begin;
    counts Count_End;
    size_t LineCount_Total=0;
    int64u Id_Next=0;

    static size_t Count(const counts& Counts, int64u Time)
    {
        auto It=Counts.find(Time);
        return It==Counts.end()?0:It->second;
    }

    static void Decrement(counts& Counts, int64u Time)
    {
        auto It=Counts.find(Time);
        if (!--It->second)
            Counts.erase(It);
    }

    void Remove(times Times)
    {
        auto Item=Items.find(Times);
        auto It=ByBegin.find(Times);
        auto Next=It; ++Next;
        bool HasPrev=It!=ByBegin.begin();
        int64u Prev_End=HasPrev?prev(It)->second:0;
        RunStarts.erase(Times);
        if (Next!=ByBegin.end())
        {
            if (HasPrev && Prev_End==Next->second)
                RunStarts.erase(*Next);
            else
                RunStarts.insert(*Next);
        }
        ByBegin.erase(It);
        ByEnd.erase(make_pair(Times.second, Times.first));
        Decrement(Count_Begin, Times.first);
        Decrement(Count_End, Times.second);
        LineCount_Total-=Item->second.LineCount;
        Ids.erase(Item->second.Id);
        Items.erase(Item);
    }
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
    SubRip_TimeLine TimeLine;

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
                    TimeLine.Remove_BeginAfter(Time_Begin_New);

                    // Empty
                    int64u Last_End;
                    if (TimeLine.Last_End(Last_End) && Last_End<Time_Begin_New)
                        EmptyCount++;

                    // Checking same times
                    bool HasSameTime=TimeLine.AddToSameTime(Time_Begin_New, Time_End_New, LineCount_New);

                    // Checking overlappings (all present events begin at or before Time_Begin_New, so before Time_End_New)
                    if (!HasSameTime)
                    {
                        Events_Total+=TimeLine.HasBeginOrEndAt(Time_Begin_New)?0:1;
                        Events_Total+=TimeLine.HasEndNotAt(Time_End_New)?1:0;
                        int64u Time;
                        if (TimeLine.Begin_Before(Time_Begin_New, Time_End_New, Time) && Events_MinDuration>Time_Begin_New-Time)
                            Events_MinDuration=Time_Begin_New-Time;
                        if (TimeLine.End_After(Time_Begin_New, Time_End_New, Time) && Events_MinDuration>Time-Time_Begin_New)
                            Events_MinDuration=Time-Time_Begin_New;
                    }

                    TimeLine.Remove_EndUpTo(Time_Begin_New);

                    if (!HasSameTime)
                        TimeLine.Add(Time_Begin_New, Time_End_New, LineCount_New);

                    LineCount_New=TimeLine.LineCount();
                    if (Events_MinDuration>Time_End_New-Time_Begin_New) // Timeline is never empty here
                        Events_MinDuration=Time_End_New-Time_Begin_New;
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
    #if MEDIAINFO_EVENTS && MEDIAINFO_DEMUX
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
    #endif //MEDIAINFO_EVENTS && MEDIAINFO_DEMUX

    Buffer_Offset=Buffer_Size;
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_SUBRIP_YES

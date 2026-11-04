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
#if defined(MEDIAINFO_TTML_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Text/File_Ttml.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#if MEDIAINFO_EVENTS
    #include "MediaInfo/MediaInfo_Events_Internal.h"
#endif //MEDIAINFO_EVENTS
#include "tinyxml2.h"
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <tuple>
using namespace tinyxml2;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{
    
//***************************************************************************
// Info
//***************************************************************************

static const char* Ttml_timeBase[] = {
    "media",
    "smpte",
    "clock",
};
static const auto Ttml_timeBase_Size=sizeof(Ttml_timeBase)/sizeof(*Ttml_timeBase);

//***************************************************************************
// Utils
//***************************************************************************

// Timeline of currently displayed events, indexed in order to avoid scanning all events for each new event
class Ttml_TimeLine
{
public:
    struct item
    {
        TimeCode    Time_Begin;
        TimeCode    Time_End;
        int64s      Begin;      // Time_Begin in milliseconds
        int64s      End;        // Time_End in milliseconds (0 if not set)
        bool        End_IsSet;
        int64u      LineCount;
    };

    // Remove events beginning after this time (not supporting back to the past)
    void Remove_BeginAfter(int64s Time)
    {
        while (!ByBegin.empty() && ByBegin.rbegin()->first>Time)
            Remove(ByBegin.rbegin()->second);
    }

    // Remove events ending at or before this time
    void Remove_EndUpTo(int64s Time)
    {
        while (!ByEnd.empty() && ByEnd.begin()->first<=Time)
            Remove(ByEnd.begin()->second);
    }

    // Last added event still present
    const item* Last() const
    {
        return Items.empty()?nullptr:&Items.rbegin()->second;
    }

    // Add lines to events with same begin and end, return true if there is such event
    bool AddToSameTime(const TimeCode& Time_Begin, const TimeCode& Time_End, int64s Begin, int64s End, int64u LineCount)
    {
        bool HasSameTime=false;
        for (auto It=ByTimes.lower_bound(make_tuple(Begin, End, (int64u)0)); It!=ByTimes.end() && get<0>(*It)==Begin && get<1>(*It)==End; ++It)
        {
            auto& Item=Items[get<2>(*It)];
            if (Item.Time_Begin==Time_Begin && Item.Time_End==Time_End)
            {
                Item.LineCount+=LineCount;
                LineCount_Total+=LineCount;
                HasSameTime=true;
            }
        }
        return HasSameTime;
    }

    // Is there an event beginning at this time, or with an end set at this time
    bool HasBeginOrEndAt(int64s Time) const
    {
        return Count(Count_Begin, Time) || Count(Count_EndSet_End, Time);
    }

    // Is there an event with an end set, beginning and ending not at this time
    bool HasEndSetNotAt(int64s Time) const
    {
        auto Pair=Count_EndSet_BeginEnd.find(make_pair(Time, Time));
        return Count_EndSet-Count(Count_EndSet_Begin, Time)-Count(Count_EndSet_End, Time)+(Pair==Count_EndSet_BeginEnd.end()?0:Pair->second)!=0;
    }

    void Add(const TimeCode& Time_Begin, const TimeCode& Time_End, int64u LineCount)
    {
        auto Id=Id_Next++;
        auto& Item=Items[Id];
        Item.Time_Begin=Time_Begin;
        Item.Time_End=Time_End;
        Item.Begin=Time_Begin.ToMilliseconds();
        Item.End=Time_End.ToMilliseconds();
        Item.End_IsSet=Time_End.IsSet();
        Item.LineCount=LineCount;
        ByBegin.insert(make_pair(Item.Begin, Id));
        ByEnd.insert(make_pair(Item.End, Id));
        ByTimes.insert(make_tuple(Item.Begin, Item.End, Id));
        Count_Begin[Item.Begin]++;
        if (Item.End_IsSet)
        {
            Count_EndSet++;
            Count_EndSet_Begin[Item.Begin]++;
            Count_EndSet_End[Item.End]++;
            Count_EndSet_BeginEnd[make_pair(Item.Begin, Item.End)]++;
        }
        LineCount_Total+=LineCount;
    }

    int64u LineCount() const
    {
        return LineCount_Total;
    }

private:
    typedef map<int64s, size_t> counts;
    map<int64u, item> Items; // Key is the insertion order
    set<pair<int64s, int64u> > ByBegin;
    set<pair<int64s, int64u> > ByEnd;
    set<tuple<int64s, int64s, int64u> > ByTimes;
    counts Count_Begin;
    counts Count_EndSet_Begin;
    counts Count_EndSet_End;
    map<pair<int64s, int64s>, size_t> Count_EndSet_BeginEnd;
    size_t Count_EndSet=0;
    int64u LineCount_Total=0;
    int64u Id_Next=0;

    static size_t Count(const counts& Counts, int64s Time)
    {
        auto It=Counts.find(Time);
        return It==Counts.end()?0:It->second;
    }

    static void Decrement(counts& Counts, int64s Time)
    {
        auto It=Counts.find(Time);
        if (!--It->second)
            Counts.erase(It);
    }

    void Remove(int64u Id)
    {
        auto It=Items.find(Id);
        const auto& Item=It->second;
        ByBegin.erase(make_pair(Item.Begin, Id));
        ByEnd.erase(make_pair(Item.End, Id));
        ByTimes.erase(make_tuple(Item.Begin, Item.End, Id));
        Decrement(Count_Begin, Item.Begin);
        if (Item.End_IsSet)
        {
            Count_EndSet--;
            Decrement(Count_EndSet_Begin, Item.Begin);
            Decrement(Count_EndSet_End, Item.End);
            auto Pair=Count_EndSet_BeginEnd.find(make_pair(Item.Begin, Item.End));
            if (!--Pair->second)
                Count_EndSet_BeginEnd.erase(Pair);
        }
        LineCount_Total-=Item.LineCount;
        Items.erase(It);
    }
};

//---------------------------------------------------------------------------
// Helper structure to track line and character counting
struct LineCharCounter
{
    int64u LineCount = 0;
    size_t CurrentLineCharCount = 0;
    size_t MaxCharsPerLine = 0;
    bool HasVisibleCharOnLine = false;
    bool LastCharWasSpace = false;

    void ProcessText(const char* text)
    {
        if (!text)
            return;

        while (*text)
        {
            unsigned char c = (unsigned char)*text;

            // In TTML/HTML, newlines and carriage returns in source are treated as whitespace,
            // NOT as line breaks. Only <br/> creates line breaks.
            bool isWhitespace = (c == ' ' || c == '\t' || c == '\r' || c == '\n');

            if (isWhitespace)
            {
                // Only count space if we have visible chars and last wasn't already a space
                if (HasVisibleCharOnLine && !LastCharWasSpace)
                {
                    CurrentLineCharCount++;
                    LastCharWasSpace = true;
                }
                text++;
            }
            else
            {
                // UTF-8 character handling
                size_t utf8Len = 1;
                bool isNbsp = false;

                if ((c & 0x80) == 0x00)
                {
                    // ASCII (1 byte)
                    utf8Len = 1;
                }
                else if ((c & 0xE0) == 0xC0)
                {
                    // 2-byte UTF-8
                    utf8Len = 2;
                    // Check if this is a non-breaking space (U+00A0 = 0xC2 0xA0 in UTF-8)
                    if (c == 0xC2 && text[1] && (unsigned char)text[1] == 0xA0)
                    {
                        isNbsp = true;
                    }
                }
                else if ((c & 0xF0) == 0xE0)
                {
                    // 3-byte UTF-8
                    utf8Len = 3;
                }
                else if ((c & 0xF8) == 0xF0)
                {
                    // 4-byte UTF-8
                    utf8Len = 4;
                }

                // Validate we have enough bytes for the UTF-8 sequence
                const char* p = text + 1;
                for (size_t i = 1; i < utf8Len; i++)
                {
                    if (!*p++)
                    {
                        utf8Len = i;
                        break;
                    }
                }

                // Count as 1 character regardless of byte length
                CurrentLineCharCount++;
                HasVisibleCharOnLine = true;

                // Non-breaking spaces are NOT collapsible
                LastCharWasSpace = false;

                // Advance by the UTF-8 sequence length
                text += utf8Len;
                while ((*text & 0x80) == 0x80)
                    ++text;
            }
        }
    }

    void NewLine()
    {
        // Trim trailing collapsible space (space/tab that was counted)
        // We do NOT trim nbsp because those are not collapsible
        // LastCharWasSpace is only set for regular spaces, not nbsp
        if (LastCharWasSpace && CurrentLineCharCount > 0)
            CurrentLineCharCount--;

        // Only count lines that have content
        if (HasVisibleCharOnLine && CurrentLineCharCount > 0)
        {
            if (MaxCharsPerLine < CurrentLineCharCount)
                MaxCharsPerLine = CurrentLineCharCount;
            LineCount++;
        }

        // Reset for next line
        CurrentLineCharCount = 0;
        HasVisibleCharOnLine = false;
        LastCharWasSpace = false;
    }

    void Finish()
    {
        // Trim trailing collapsible space (space/tab that was counted)
        if (LastCharWasSpace && CurrentLineCharCount > 0)
            CurrentLineCharCount--;

        // Only count final line if it has content
        if (HasVisibleCharOnLine && CurrentLineCharCount > 0)
        {
            if (MaxCharsPerLine < CurrentLineCharCount)
                MaxCharsPerLine = CurrentLineCharCount;
            LineCount++;
        }
    }
};

// Recursively process XML nodes to extract text and count lines
void ProcessNodeForLineCount(XMLNode* node, LineCharCounter& counter)
{
    if (!node)
        return;

    // Process all children (text nodes and elements)
    for (XMLNode* child = node->FirstChild(); child; child = child->NextSibling())
    {
        // Check if it's a text node
        XMLText* textNode = child->ToText();
        if (textNode)
        {
            counter.ProcessText(textNode->Value());
        }
        // Check if it's an element
        else
        {
            XMLElement* element = child->ToElement();
            if (element)
            {
                // Handle <br/> elements as line breaks
                const char* elemName = element->Value();

                // Check for br element (with or without namespace)
                // Common forms: "br", "tt:br", "ttml:br", etc.
                bool isBr = (strcmp(elemName, "br") == 0 || 
                            strcmp(elemName, "tt:br") == 0 ||
                            strcmp(elemName, "ttml:br") == 0 ||
                            strstr(elemName, ":br") != nullptr);

                if (isBr)
                {
                    counter.NewLine();
                }
                else
                {
                    // Recursively process other elements (span, b, i, etc.)
                    ProcessNodeForLineCount(element, counter);
                }
            }
        }
    }
}

double to_float64(const char* s)
{
    float64 Value=(float64)0;
    const float64 Ten=(float64)10;
    int e=0;
    char c;
    while ((c=*s++) && (c>='0' && c<='9'))
    {
        Value*=Ten;
        Value+=(c-'0');
    }
    if (c=='.')
    {
        while ((c=*s++) && (c>='0' && c<='9'))
        {
            Value*=Ten;
            Value+=(c-'0');
            e--;
        }
    }
    if (c=='e' || c=='E')
    {
        int sign=1;
        int i=0;
        c=*s++;
        if (c=='+')
            c=*s++;
        else if (c=='-')
        {
            c=*s++;
            sign=-1;
        }
        while (c>='0' && c<='9') {
            if (i<100000) // Avoid overflow, more than enough for a double
            {
                i*=10;
                i+=(c-'0');
            }
            c=*s++;
        }
        e+=i*sign;
    }
    if (e>400) // More than enough for a double
        e=400;
    if (e<-400)
        e=-400;
    while (e>0)
    {
        Value*=Ten;
        e--;
    }
    while (e<0)
    {
        Value*=0.1;
        e++;
    }
    return Value;
}

int64u Ttml_str2timecode(const char* Value, float FrameRate=0)
{
    size_t Length=strlen(Value);
         if (Length>=8
     && Value[0]>='0' && Value[0]<='9'
     && Value[1]>='0' && Value[1]<='9'
     && Value[2]==':'
     && Value[3]>='0' && Value[3]<='9'
     && Value[4]>='0' && Value[4]<='9'
     && Value[5]==':'
     && Value[6]>='0' && Value[6]<='9'
     && Value[7]>='0' && Value[7]<='9')
    {
        int64u ToReturn=(int64u)(Value[0]-'0')*10*60*60*1000000000
                       +(int64u)(Value[1]-'0')   *60*60*1000000000
                       +(int64u)(Value[3]-'0')   *10*60*1000000000
                       +(int64u)(Value[4]-'0')      *60*1000000000
                       +(int64u)(Value[6]-'0')      *10*1000000000
                       +(int64u)(Value[7]-'0')         *1000000000;
        if (Length>=9 && (Value[8]=='.' || Value[8]==','))
        {
            if (Length>9+9)
                Length=9+9; //Nanoseconds max
            const char* Value_End=Value+Length;
            Value+=9;
            int64u Multiplier=100000000;
            while (Value<Value_End)
            {
                ToReturn+=(int64u)(*Value-'0')*Multiplier;
                Multiplier/=10;
                Value++;
            }
        }
        if (FrameRate && Length>=9 && Value[8]==':')
        {
            const char* Value_End=Value+Length;
            Value+=9;
            int64u FrameNumber=0;
            while (Value<Value_End)
            {
                FrameNumber*=10;
                FrameNumber+=(int64u)(*Value-'0');
                Value++;
            }
            ToReturn+=FrameNumber/FrameRate*1000000000;
        }

        return ToReturn;
    }
    else if (Length>=2
     && Value[Length-1]=='s')
    {
        float64 ValueF=to_float64(Value);
        if (ValueF<0)
            return 0; //Npt supported
        return (int64u)float64_int64s(ValueF*1000000000);
    }
    else
        return (int64u)-1;
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Ttml::File_Ttml()
:File__Analyze()
{
    //Configuration
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_Ttml;
        StreamIDs_Width[0]=0;
    #endif //MEDIAINFO_EVENTS

    //Init
    Frame_Count=0;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ttml::Streams_Accept()
{
    Fill(Stream_General, 0, General_Format, "TTML");

    Stream_Prepare(Stream_Text);
    Fill(Stream_Text, 0, "Format", "TTML");

    LineCount=0;
    MaxCountOfLinesPerFrame=0;
    MaxCountOfCharsPerLine=0;
    EmptyCount=0;
    FrameRate_Int=0;
    FrameRateMultiplier_Num=1;
    FrameRateMultiplier_Den=1;
    FrameRate=0;
    FrameRate_Is1001=false;
}

//---------------------------------------------------------------------------
void File_Ttml::Streams_Finish()
{
    if (Time_End.IsSet() && Time_Begin.IsSet())
    {
        auto AdaptMediaTime=[&](TimeCode &TC)
        {
            if (Time_Begin.IsTimed() || TimeBase!=timeBase_media || (!TC.Is1001fps() && !TC.DropFrame())) // https://www.w3.org/TR/2018/REC-ttml2-20181108/#semantics-media-timing
                return;
            auto Frames=TC.GetFrames();
            auto MS=((float)Frames)*1000/TC.GetFrameRate();
            auto Temp(TC);
            Temp.Set1001fps(false);
            Temp.SetDropFrame(false);
            Temp.SetFrames(0);
            auto Value=Temp.ToMilliseconds();
            Value+=float32_int64s(MS);
            TC.FromSeconds(Value / 1000.0);
        };
        AdaptMediaTime(Time_Begin);
        AdaptMediaTime(Time_End);
        auto Duration = Time_End - Time_Begin;
        auto Time_Begin_ms = (int64u)Time_Begin.ToMilliseconds();
        auto Time_End_ms = (int64u)Time_End.ToMilliseconds();
        auto Duration_ms = (int64u)(Time_End - Time_Begin).ToMilliseconds();
        Fill(Stream_General, 0, General_Duration, Duration_ms);
        Fill(Stream_Text, 0, Text_Duration, Duration_ms);
        Fill(Stream_Text, 0, Text_Duration_Start_Command, Time_Begin_ms);
        Fill(Stream_Text, 0, Text_Duration_Start, Time_Begin_ms);
        Fill(Stream_Text, 0, Text_Duration_End, Time_End_ms);
        Fill(Stream_Text, 0, Text_Duration_End_Command, Time_End_ms);
        if (!Time_Begin.IsTimed())
            Fill(Stream_Text, 0, Text_TimeCode_FirstFrame, Time_Begin.ToString());
        if (!Time_End.IsTimed() && Time_End > Time_Begin)
            Fill(Stream_Text, 0, Text_TimeCode_LastFrame, (Time_End - 1).ToString());
    }
    Fill(Stream_Text, 0, Text_FrameRate_Mode, "CFR");
    const auto Events_Total = Frame_Count - EmptyCount;
    Fill(Stream_Text, 0, Text_Events_Total, Events_Total);
    if (Events_Total) {
        Fill(Stream_Text, 0, Text_Lines_Count, LineCount);
        if (LineCount) {
            Fill(Stream_Text, 0, Text_Lines_MaxCountPerEvent, MaxCountOfLinesPerFrame);
            Fill(Stream_Text, 0, Text_Lines_MaxCharacterCount, MaxCountOfCharsPerLine);
        }
    }
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ttml::Read_Buffer_Unsynched()
{
    GoTo(0);
}

//---------------------------------------------------------------------------
#if MEDIAINFO_SEEK
size_t File_Ttml::Read_Buffer_Seek (size_t Method, int64u Value, int64u ID)
{
    Open_Buffer_Unsynch();
    return 1;
}
#endif //MEDIAINFO_SEEK

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Ttml::FileHeader_Begin()
{
    //All should be OK...
    return true;
}

void File_Ttml::Read_Buffer_Continue()
{
    if (!IsSub && File_Offset+Buffer_Offset+Element_Offset)
        return; //Already parsed, no change

    tinyxml2::XMLDocument document;

    if (!FileHeader_Begin_XML(document))
        return;

    XMLElement* Root=document.FirstChildElement("tt");
    if (!Root)
    {
        Reject();
        return;
    }

    if (!Status[IsAccepted])
    {
        Accept();

        #if MEDIAINFO_EVENTS
            MuxingMode=(int8u)-1;
            if (StreamIDs_Size>=2 && ParserIDs[StreamIDs_Size-2]==MediaInfo_Parser_Mpeg4)
                MuxingMode=11; //MPEG-4
            if (StreamIDs_Size>2 && ParserIDs[StreamIDs_Size-2]==MediaInfo_Parser_Mxf) //Only if referenced MXF
                MuxingMode=13; //MXF
        #endif //MEDIAINFO_EVENTS

        #if MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
            if (Config->NextPacket_Get() && Config->Event_CallBackFunction_IsSet())
                return; // Waiting for NextPacket
        #endif //MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
    }

    // Root attributes
    bool IsSmpteTt=false, IsEbuTt=false, IsImsc1=false;
    int32u tickRate=0;
    int32u TimeCodeFrameMax=0;
    const char* Tt_Attribute;
    Tt_Attribute=Root->Attribute("ittp:aspectRatio");
    if (!Tt_Attribute)
        Tt_Attribute=Root->Attribute("aspectRatio");
    if (Tt_Attribute && Retrieve_Const(Stream_Text, 0, Text_DisplayAspectRatio).empty())
    {
        int64u DisplayAspectRatio_Num=atoi(Tt_Attribute);
        if (const char* Tt_Attribute_Space=strchr(Tt_Attribute, ' '))
        {
            int64u DisplayAspectRatio_Den=atoi(Tt_Attribute_Space+1);
            if (DisplayAspectRatio_Num && DisplayAspectRatio_Den)
            {
                float64 DisplayAspectRatio=((float64)DisplayAspectRatio_Num)/DisplayAspectRatio_Den;
                Fill(Stream_Text, 0, Text_DisplayAspectRatio, DisplayAspectRatio);
            }
        }
    }
    Tt_Attribute=Root->Attribute("ttp:frameRate");
    if (!Tt_Attribute)
        Tt_Attribute=Root->Attribute("frameRate");
    if (Tt_Attribute)
    {
        FrameRate_Int=atof(Tt_Attribute);
    }
    Tt_Attribute=Root->Attribute("ttp:frameRateMultiplier");
    if (!Tt_Attribute)
        Tt_Attribute=Root->Attribute("frameRateMultiplier");
    if (Tt_Attribute)
    {
        FrameRateMultiplier_Num=atoi(Tt_Attribute);
        if (const char* Tt_Attribute_Space=strchr(Tt_Attribute, ' '))
        {
            FrameRateMultiplier_Den=atoi(Tt_Attribute_Space+1);
        }
    }
    Tt_Attribute=Root->Attribute("ttp:tickRate");
    if (Tt_Attribute)
        tickRate=atoi(Tt_Attribute);
    if (FrameRate_Int && FrameRateMultiplier_Num && FrameRateMultiplier_Den)
    {
        FrameRate=((float64)FrameRate_Int)*FrameRateMultiplier_Num/FrameRateMultiplier_Den;
        if ((FrameRateMultiplier_Num==1000 && FrameRateMultiplier_Den==1001)
         || (FrameRateMultiplier_Num==999 && FrameRateMultiplier_Den==1000)) // Seen in a 23.976 file, mistake?
            FrameRate_Is1001=true;
    }
    if (FrameRate)
    {
        Fill(Stream_General, 0, General_FrameRate, FrameRate);
        Fill(Stream_Text, 0, Text_FrameRate, FrameRate);
    }
    if (FrameRate_Int && FrameRateMultiplier_Num && FrameRateMultiplier_Den)
    {
        Fill(Stream_Text, 0, Text_FrameRate_Num, FrameRate_Int*FrameRateMultiplier_Num, 10, true);
        Fill(Stream_Text, 0, Text_FrameRate_Den, FrameRateMultiplier_Den, 10, true);
    }
    Tt_Attribute=Root->Attribute("xml:lang");
    if (!Tt_Attribute)
        Tt_Attribute=Root->Attribute("lang");
    if (Tt_Attribute && Retrieve_Const(Stream_Text, 0, Text_Language).empty())
    {
        Fill(Stream_Text, 0, Text_Language, Tt_Attribute);
    }
    Tt_Attribute=Root->Attribute("ttp:timeBase");
    if (!Tt_Attribute)
        Tt_Attribute=Root->Attribute("timeBase");
    if (Tt_Attribute && Retrieve_Const(Stream_Text, 0, "Duration_Base").empty())
    {
        for (size_t i=0; i<Ttml_timeBase_Size; i++)
        {
            if (!strcmp(Tt_Attribute, Ttml_timeBase[i]))
                TimeBase=(timeBase_t)i;
        }
        Fill(Stream_Text, 0, "Duration_Base", Tt_Attribute);
    }
    for (const XMLAttribute* tt_attribute=Root->FirstAttribute(); tt_attribute; tt_attribute=tt_attribute->Next())
    {
        if (strstr(tt_attribute->Name(), "smpte"))
            IsSmpteTt=true;
        if (strstr(tt_attribute->Name(), "ebutt"))
            IsEbuTt=true;
        if (strstr(tt_attribute->Name(), "ittp"))
            IsImsc1=true;
        if (strstr(tt_attribute->Name(), "itts"))
            IsImsc1=true;
        if (strstr(tt_attribute->Value(), "smpte-tt"))
            IsSmpteTt=true;
        if (strstr(tt_attribute->Value(), "ebu:tt"))
            IsEbuTt=true;
        if (strstr(tt_attribute->Value(), "imsc1"))
            IsImsc1=true;
    }
    if (IsSmpteTt)
    {
        Fill(Stream_General, 0, General_Format_Profile, "SMPTE-TT");
        Fill(Stream_Text, 0, Text_Format_Profile, "SMPTE-TT");
        if (!FrameRate_Int && tickRate >= 2000)
        {
            // Not clear but it seems that tickRate is actually frame rate x 1000 in several files without frame rate
            auto Temp = float64_int64s(tickRate / 1000.0) - 1;
            if (Temp <= numeric_limits<int32u>::max())
                TimeCodeFrameMax = Temp;
        }
    }
    if (IsEbuTt)
    {
        Fill(Stream_General, 0, General_Format_Profile, "EBU-TT");
        Fill(Stream_Text, 0, Text_Format_Profile, "EBU-TT");
    }
    if (IsImsc1)
    {
        Fill(Stream_General, 0, General_Format_Profile, "IMSC1");
        Fill(Stream_Text, 0, Text_Format_Profile, "IMSC1");
    }
    if (!FrameRate_Int && !tickRate)
    {
        #if MEDIAINFO_ADVANCED
            float64 FrameRate_F=Video_FrameRate_Rounded(Config->File_DefaultFrameRate_Get());
            if (!FrameRate_F)
                FrameRate_F=30; //"if no application defined frame rate applies, then thirty (30) frames per second" https://www.w3.org/TR/2018/REC-ttml2-20181108/#parameter-attribute-frameRate
        #else //MEDIAINFO_ADVANCED
            const float64 FrameRate_F=30;
        #endif //MEDIAINFO_ADVANCED
        FrameRate_Int=float64_int64s(FrameRate_F);
        if (FrameRate_Int != FrameRate_F)
        {
            FrameRateMultiplier_Num=1000;
            FrameRateMultiplier_Den=float64_int64s(FrameRate_Int/FrameRate_F*1000);
            if (FrameRateMultiplier_Num==1000 && FrameRateMultiplier_Den==1001)
                FrameRate_Is1001=true;
        }
    }
    string Rosetta_Profile, Rosetta_Version;

    tinyxml2::XMLElement*       div=NULL;
    #if MEDIAINFO_EVENTS
    tinyxml2::XMLElement*       p=NULL;
    #endif //MEDIAINFO_EVENTS
    Ttml_TimeLine TimeLine;
    for (XMLElement* tt_element=Root->FirstChildElement(); tt_element; tt_element=tt_element->NextSiblingElement())
    {
        //body
        if (!strcmp(tt_element->Value(), "body"))
        {
            TimeCode Time_Template;
            Time_Template.SetFramesMax((int32u)(FrameRate_Int?(FrameRate_Int-1):(tickRate?(tickRate-1):0)));
            Time_Template.Set1001fps(FrameRate_Is1001);

            for (XMLElement* body_element=tt_element->FirstChildElement(); body_element; body_element=body_element->NextSiblingElement())
            {
                //div
                if (!strcmp(body_element->Value(), "div"))
                {
                    TimeCode Time_Begin_New=Time_Template;
                    TimeCode Time_End_New=Time_Template;

                    if (const char* Attribute=body_element->Attribute("begin"))
                    {
                        if (!Time_Begin_New.FromString(Attribute))
                        {
                            if (TimeCodeFrameMax && !Time_Begin_New.IsTimed())
                                Time_Begin_New.SetFramesMax(TimeCodeFrameMax);
                            if (!Time_Begin.IsSet() || Time_Begin>Time_Begin_New)
                                Time_Begin=Time_Begin_New;
                            if (!Time_End.IsSet() || Time_End<Time_Begin_New)
                                Time_End=Time_Begin_New;
                        }
                    }
                    if (const char* Attribute=body_element->Attribute("end"))
                    {
                        if (!Time_End_New.FromString(Attribute))
                        {
                            if (TimeCodeFrameMax && !Time_End_New.IsTimed())
                                Time_End_New.SetFramesMax(TimeCodeFrameMax);
                            if (!Time_Begin.IsSet() || Time_Begin>Time_End_New)
                                Time_Begin=Time_End_New;
                            if (!Time_End.IsSet() || Time_End<Time_End_New)
                                Time_End=Time_End_New;
                        }
                    }
                    if (const char* Attribute=body_element->Attribute("dur"))
                    {
                        TimeCode Time_Dur_New=Time_Template;
                        if (Time_Begin.IsSet() && !Time_Dur_New.FromString(Attribute))
                        {
                            if (TimeCodeFrameMax && !Time_Dur_New.IsTimed())
                                Time_Dur_New.SetFramesMax(TimeCodeFrameMax);
                            Time_End_New=Time_Begin_New;
                            Time_End_New+=Time_Dur_New;
                            if (!Time_Begin.IsSet() || Time_Begin>Time_End_New)
                                Time_Begin=Time_End_New;
                            if (!Time_End.IsSet() || Time_End<Time_End_New)
                                Time_End=Time_End_New;
                        }
                    }

                    for (XMLElement* div_element=body_element->FirstChildElement(); div_element; div_element=div_element->NextSiblingElement())
                    {
                        //p
                        if (!strcmp(div_element->Value(), "p"))
                        {
                            if (const char* Attribute=div_element->Attribute("begin"))
                            {
                                if (!Time_Begin_New.FromString(Attribute))
                                {
                                    if (TimeCodeFrameMax && !Time_Begin_New.IsTimed())
                                        Time_Begin_New.SetFramesMax(TimeCodeFrameMax);
                                    if (!Time_Begin.IsSet() || Time_Begin>Time_Begin_New)
                                        Time_Begin=Time_Begin_New;
                                    if (!Time_End.IsSet() || Time_End<Time_Begin_New)
                                        Time_End=Time_Begin_New;
                                }
                            }
                            if (const char* Attribute=div_element->Attribute("end"))
                            {
                                if (!Time_End_New.FromString(Attribute))
                                {
                                    if (TimeCodeFrameMax && !Time_End_New.IsTimed())
                                        Time_End_New.SetFramesMax(TimeCodeFrameMax);
                                    if (!Time_Begin.IsSet() || Time_Begin>Time_End_New)
                                        Time_Begin=Time_End_New;
                                    if (!Time_End.IsSet() || Time_End<Time_End_New)
                                        Time_End=Time_End_New;
                                }
                            }
                            if (const char* Attribute=div_element->Attribute("dur"))
                            {
                                TimeCode Time_Dur_New=Time_Template;
                                if (Time_Begin.IsSet() && !Time_Dur_New.FromString(Attribute))
                                {
                                    if (TimeCodeFrameMax && !Time_Dur_New.IsTimed())
                                        Time_Dur_New.SetFramesMax(TimeCodeFrameMax);
                                    Time_End_New=Time_Begin_New;
                                    Time_End_New+=Time_Dur_New;
                                    if (!Time_Begin.IsSet() || Time_Begin>Time_End_New)
                                        Time_Begin=Time_End_New;
                                    if (!Time_End.IsSet() || Time_End<Time_End_New)
                                        Time_End=Time_End_New;
                                }
                            }
                            if (!div)
                                div=body_element;
                            #if MEDIAINFO_EVENTS
                                if (!p)
                                    p=div_element;
                            #endif //MEDIAINFO_EVENTS

                            // Count lines and characters using recursive processing
                            LineCharCounter counter;
                            ProcessNodeForLineCount(div_element, counter);
                            counter.Finish();

                            int64u LineCount_New = counter.LineCount;
                            int64u LineCount_FromCounter = counter.LineCount;  // Save the actual line count from this <p> element
                            if (counter.MaxCharsPerLine > MaxCountOfCharsPerLine)
                                MaxCountOfCharsPerLine = counter.MaxCharsPerLine;

                            if (Time_Begin_New.IsSet())
                            {
                                const auto Begin_New=Time_Begin_New.ToMilliseconds();
                                const auto End_New=Time_End_New.ToMilliseconds();

                                // Not supporting back to the past
                                TimeLine.Remove_BeginAfter(Begin_New);

                                // Empty
                                const auto Last=TimeLine.Last();
                                if (Last && Last->End_IsSet && Last->End<Begin_New)
                                    EmptyCount++;

                                // Checking same times
                                bool HasSameTime=TimeLine.AddToSameTime(Time_Begin_New, Time_End_New, Begin_New, End_New, LineCount_New);

                                // Checking overlappings
                                if (!HasSameTime)
                                {
                                    Frame_Count+=TimeLine.HasBeginOrEndAt(Begin_New)?0:1;
                                    Frame_Count+=TimeLine.HasEndSetNotAt(End_New)?1:0;
                                }

                                TimeLine.Remove_EndUpTo(Begin_New);

                                if (!HasSameTime)
                                    TimeLine.Add(Time_Begin_New, Time_End_New, LineCount_FromCounter);

                                LineCount_New=TimeLine.LineCount();
                            }
                            else
                                Frame_Count++;

                            LineCount+=LineCount_FromCounter;  // Add the actual line count from this <p>, not the accumulated overlap count
                            if (MaxCountOfLinesPerFrame<LineCount_New)  // But MaxCountOfLinesPerFrame uses the overlap sum for simultaneous display
                                MaxCountOfLinesPerFrame=LineCount_New;
                        }
                    }
                }
            }
        }
        if (!strcmp(tt_element->Value(), "head"))
        {
            for (XMLElement* head_element = tt_element->FirstChildElement(); head_element; head_element = head_element->NextSiblingElement())
            {
                if (!strcmp(head_element->Value(), "metadata"))
                {
                    for (XMLElement* metadata_element = head_element->FirstChildElement(); metadata_element; metadata_element = metadata_element->NextSiblingElement())
                    {
                        if (!strcmp(metadata_element->Value(), "smpte:information"))
                        {
                            if (const char* Attribute = metadata_element->Attribute("mode"))
                                Fill(Stream_Text, 0, "608_Mode", Attribute);
                            if (const char* Attribute = metadata_element->Attribute("m608:channel"))
                                Fill(Stream_Text, 0, Text_ID, Attribute);
                            if (const char* Attribute = metadata_element->Attribute("m608:programName"))
                                Fill(Stream_Text, 0, Text_Title, Attribute);
                            if (const char* Attribute = metadata_element->Attribute("m608:captionService"))
                            {
                                if (strlen(Attribute) == 6
                                    && Attribute[0] == 'F'
                                    && Attribute[1] >= '1' && Attribute[1] <= '2'
                                    && Attribute[2] == 'C'
                                    && Attribute[3] >= '1' && Attribute[3] <= '2'
                                    && ((Attribute[4] == 'C' && Attribute[5] == 'C')
                                     || (Attribute[4] == 'T' && Attribute[5] == 'X'))
                                    )
                                {
                                    string ID = Attribute[4] == 'C' ? "CC" : "T";
                                    ID +=((Attribute[1] - '1') * 2) + Attribute[3];
                                    Fill(Stream_Text, 0, "CaptionServiceName", ID);
                                }
                                else
                                    Fill(Stream_Text, 0, "CaptionServiceName", Attribute);
                            }
                        }
                        if (!strcmp(metadata_element->Value(), "rosetta:format"))
                        {
                            for (auto format_element = metadata_element->FirstChild(); format_element; format_element = format_element->NextSiblingElement())
                            {
                                Rosetta_Profile = format_element->Value();
                                break;
                            }
                        }
                        if (!strcmp(metadata_element->Value(), "rosetta:version"))
                        {
                            for (auto format_element = metadata_element->FirstChild(); format_element; format_element = format_element->NextSiblingElement())
                            {
                                Rosetta_Version = format_element->Value();
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    if (!Rosetta_Profile.empty())
    {
        auto Profile = Rosetta_Profile + (Rosetta_Version.empty() ? string() : (' ' + Rosetta_Version));
        Fill(Stream_General, 0, General_Format_Profile, Profile);
        Fill(Stream_Text, 0, Text_Format_Profile, Profile);
    }

    #if MEDIAINFO_DEMUX
        Demux(Buffer, Buffer_Size, ContentType_MainStream);
    #endif //MEDIAINFO_DEMUX

    // Output
    #if MEDIAINFO_EVENTS
        for (; p; p=p->NextSiblingElement())
        {
            //p
            if (!strcmp(p->Value(), "p"))
            {
                int64u DTS_Begin=(int64u)-1;
                const char* Attribute=p->Attribute("begin");
                if (Attribute)
                    DTS_Begin=Ttml_str2timecode(Attribute);
                int64u DTS_End=(int64u)-1;
                Attribute=p->Attribute("end");
                if (Attribute)
                    DTS_End=Ttml_str2timecode(Attribute);
                string ContentUtf8;
                XMLPrinter printer;
                p->Accept(&printer);
                ContentUtf8+=printer.CStr();
                while (!ContentUtf8.empty() && (ContentUtf8[ContentUtf8.size()-1]=='\r' || ContentUtf8[ContentUtf8.size()-1]=='\n'))
                    ContentUtf8.resize(ContentUtf8.size()-1);
                Ztring Content; Content.From_UTF8(ContentUtf8.c_str());

                Frame_Count_NotParsedIncluded=Frame_Count;
                EVENT_BEGIN (Global, SimpleText, 0)
                    //Hack: remove "p", "span", "br"
                    Content.FindAndReplace(__T("\r"), Ztring(), 0, ZenLib::Ztring_Recursive);
                    Content.FindAndReplace(__T("\n"), Ztring(), 0, ZenLib::Ztring_Recursive);
                    for (;;)
                    {
                        size_t Span_Begin=Content.find(__T("<p"));
                        if (Span_Begin==string::npos)
                            break;
                        
                        size_t Span_End=Content.find(__T('>'), Span_Begin+5);
                        if (Span_End==string::npos)
                            break;

                        size_t ShlashSpan_Begin=Content.find(__T("</p>"), Span_End+1);
                        if (ShlashSpan_Begin==string::npos)
                            break;

                        Content.erase(ShlashSpan_Begin, 7);
                        Content.erase(Span_Begin, Span_End-Span_Begin+1);
                    }
                    for (;;)
                    {
                        size_t Span_Begin=Content.find(__T("<span"));
                        if (Span_Begin==string::npos)
                            break;
                        
                        size_t Span_End=Content.find(__T('>'), Span_Begin+5);
                        if (Span_End==string::npos)
                            break;

                        size_t ShlashSpan_Begin=Content.find(__T("</span>"), Span_End+1);
                        if (ShlashSpan_Begin==string::npos)
                            break;

                        Content.erase(ShlashSpan_Begin, 7);
                        Content.erase(Span_Begin, Span_End-Span_Begin+1);
                    }
                    Content.FindAndReplace(__T("<br>"), EOL, 0, ZenLib::Ztring_Recursive);
                    Content.FindAndReplace(__T("<br/>"), EOL, 0, ZenLib::Ztring_Recursive);
                    Content.FindAndReplace(__T("<br />"), EOL, 0, ZenLib::Ztring_Recursive);

                    std::wstring Content_Unicode{ Content.To_Unicode() };
                    Event.DTS=DTS_Begin;
                    Event.PTS=Event.DTS;
                    Event.DUR=DTS_End-DTS_Begin;
                    Event.Content=Content_Unicode.c_str();
                    Event.Flags=0;
                    Event.MuxingMode=MuxingMode;
                    Event.Service=(int8u)Element_Code;
                    Event.Row_Max=0;
                    Event.Column_Max=0;
                    Event.Row_Values=NULL;
                    Event.Row_Attributes=NULL;
                EVENT_END   ()
                EVENT_BEGIN (Global, SimpleText, 0)
                    Event.DTS=DTS_End;
                    Event.PTS=Event.DTS;
                    Event.DUR=0;
                    Event.Content=L"";
                    Event.Flags=0;
                    Event.MuxingMode=MuxingMode;
                    Event.Service=(int8u)Element_Code;
                    Event.Row_Max=0;
                    Event.Column_Max=0;
                    Event.Row_Values=NULL;
                    Event.Row_Attributes=NULL;
                EVENT_END   ()
            }
        }
    #endif //MEDIAINFO_EVENTS

    Buffer_Offset=Buffer_Size;
}

} //NameSpace

#endif //MEDIAINFO_TTML_YES

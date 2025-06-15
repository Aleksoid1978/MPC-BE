/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_File__AnalyzeH
#define MediaInfo_File__AnalyzeH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Base.h"
#include "MediaInfo/File__Analyse_Automatic.h"
#include "MediaInfo/File__Analyze_Element.h"
#include "ZenLib/BitStream_Fast.h"
#include "ZenLib/BitStream_LE.h"
#if MEDIAINFO_IBIUSAGE
    #include "MediaInfo/Multiple/File_Ibi_Creation.h"
#endif //MEDIAINFO_IBIUSAGE
#include "tinyxml2.h"
#if MEDIAINFO_AES
    #include <aescpp.h>
#endif //MEDIAINFO_AES
#include "MediaInfo/HashWrapper.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

class MediaInfo_Internal;

template <class T> inline Ztring Get_Hex_ID(const T& Value)
{
    Ztring ID_String;
    ID_String.From_Number(Value); 
    ID_String += __T(" (0x"); 
    ID_String += Ztring::ToZtring(Value, 16); 
    ID_String += __T(")");
    return ID_String;
}

template <class T> inline T IsAsciiLower(T Value)
{
    return ((unsigned)Value - 'a') < 26;
}

template <class T> inline T IsAsciiUpper(T Value)
{
    return ((unsigned)Value - 'A') < 26;
}

template <class T> inline T IsAsciiDigit(T Value)
{
    return ((unsigned)Value - '0') < 10;
}

struct buffer_data
{
    size_t Size;
    int8u* Data;

    buffer_data()
    {
        Size = 0;
        Data = NULL;
    }
    buffer_data(const int8u* aData, size_t aSize)
    {
        Size = aSize;
        Data = new int8u[aSize];
        std::memcpy(Data, aData, aSize);
    }

    ~buffer_data()
    {
        delete[] Data; //Data=NULL;
    }
};

static inline int8u ReverseBits(int8u c)
{
    // Input: bit order is 76543210
    //Output: bit order is 01234567
    c = (c & 0x0F) << 4 | (c & 0xF0) >> 4;
    c = (c & 0x33) << 2 | (c & 0xCC) >> 2;
    c = (c & 0x55) << 1 | (c & 0xAA) >> 1;
    return c;
}

string uint128toString(uint128 Value, int radix);

enum conformance_type {
    Conformance_Error,
    Conformance_Warning,
    Conformance_Information,
    Conformance_Max,
};

#if !MEDIAINFO_TRACE
    #include "MediaInfo/File__Analyze_MinimizeSize.h"
#else

//***************************************************************************
// Class File__Base
//***************************************************************************

class File__Analyze : public File__Base
{
public :
    //***************************************************************************
    // Constructor/Destructor
    //***************************************************************************

    File__Analyze();
    virtual ~File__Analyze();

    //***************************************************************************
    // Open
    //***************************************************************************

    void    Open_Buffer_Init        (                    int64u File_Size);
    void    Open_Buffer_Init        (File__Analyze* Sub);
    void    Open_Buffer_Init        (File__Analyze* Sub, int64u File_Size);
    void    Open_Buffer_OutOfBand   (                    const int8u* Buffer, size_t Buffer_Size) {File__Analyze::Buffer=Buffer; File__Analyze::Buffer_Size=Buffer_Size; Element_Offset=0; Element_Size=Buffer_Size; Read_Buffer_OutOfBand(); File__Analyze::Buffer=NULL; File__Analyze::Buffer_Size=0; Element_Offset=0; Element_Size=0;}
    void    Open_Buffer_OutOfBand   (File__Analyze* Sub                     , size_t Buffer_Size);
    void    Open_Buffer_OutOfBand   (File__Analyze* Sub) {Open_Buffer_OutOfBand(Sub, Element_Size-Element_Offset);};
    void    Open_Buffer_Continue    (                    const int8u* Buffer, size_t Buffer_Size);
    void    Open_Buffer_Continue    (File__Analyze* Sub, const int8u* Buffer, size_t Buffer_Size, bool IsNewPacket=true, float64 Ratio=1.0);
    void    Open_Buffer_Continue    (File__Analyze* Sub, size_t Buffer_Size) {if (Element_Offset+Buffer_Size<=Element_Size) Open_Buffer_Continue(Sub, Buffer+Buffer_Offset+(size_t)Element_Offset, Buffer_Size); Element_Offset+=Buffer_Size;}
    void    Open_Buffer_Continue    (File__Analyze* Sub) {if (Element_Offset<=Element_Size) Open_Buffer_Continue(Sub, Buffer+Buffer_Offset+(size_t)Element_Offset, (size_t)(Element_Size-Element_Offset)); Element_Offset=Element_Size;}
    void    Open_Buffer_Position_Set(int64u File_Offset);
    void    Open_Buffer_CheckFileModifications();
    #if MEDIAINFO_SEEK
    size_t  Open_Buffer_Seek        (size_t Method, int64u Value, int64u ID);
    #endif //MEDIAINFO_SEEK
    #if MEDIAINFO_ADVANCED2
    void    Open_Buffer_SegmentChange();
    #endif //MEDIAINFO_ADVANCED2
    void    Open_Buffer_Update      ();
    void    Open_Buffer_Update      (File__Analyze* Sub);
    void    Open_Buffer_Unsynch     ();
    void    Open_Buffer_Finalize    (bool NoBufferModification=false);
    void    Open_Buffer_Finalize    (File__Analyze* Sub);

    //***************************************************************************
    // In/Out (for parsers)
    //***************************************************************************

    //In
    std::string ParserName;
    #if MEDIAINFO_EVENTS
        size_t  StreamIDs_Size;
        int64u  StreamIDs[16];
        int8u   StreamIDs_Width[16];
        int8u   ParserIDs[16];
        void    Event_Prepare (struct MediaInfo_Event_Generic* Event, int32u Event_Code, size_t Event_Size);
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_DEMUX
        int8u   Demux_Level; //bit 0=frame, bit 1=container, bit 2=elementary (eg MPEG-TS), bit 3=ancillary (e.g. DTVCC), default with frame set
        bool    Demux_random_access;
        bool    Demux_UnpacketizeContainer;
        bool    Demux_IntermediateItemFound;
        size_t  Demux_Offset;
        int64u  Demux_TotalBytes;
        File__Analyze* Demux_CurrentParser;
    #endif //MEDIAINFO_DEMUX
    Ztring  File_Name_WithoutDemux;
    bool   PTS_DTS_Needed;
    enum ts_type
    {
        TS_NONE=0,
        TS_PTS=1,
        TS_DTS=2,
        TS_ALL=TS_PTS|TS_DTS,
    };
    void   TS_Clear(ts_type Type=TS_ALL);
    void   TS_Set(int64s Ticks, ts_type Type=TS_ALL);
    void   TS_Set(File__Analyze* Parser, ts_type Type=TS_ALL);
    void   TS_Add(int64s Ticks, ts_type Type=TS_ALL);
    void   TS_Ajust(int64s Ticks);
    int64s Frequency_c; //Frequency of the timestamp of the container (e.g. 90000 for MPEG-PS)
    int64s Frequency_b; //Frequency of the timestamp of the bitstream (e.g. 48000 for AC-3)
    #if MEDIAINFO_ADVANCED2
    static const int64s NoTs=0x8000000000000000LL;
    int64s PTSb; //In 1/Frequency_b if sub, in 1/(Frequency_c*Frequency_b) if in a container
    int64s DTSb; //In 1/Frequency_b if sub, in 1/(Frequency_c*Frequency_b) if in a container
    #endif //MEDIAINFO_ADVANCED2
    struct frame_info
    {
        int64u Buffer_Offset_End;
        int64u PCR; //In nanoseconds
        int64u PTS; //In nanoseconds
        int64u DTS; //In nanoseconds
        int64u DUR; //In nanoseconds
        #if MEDIAINFO_ADVANCED2
        int64s PTSc; //In 1/Frequency_c
        int64s DTSc; //In 1/Frequency_c
        int64u Frame_Count_AfterLastTimeStamp;
        #endif //MEDIAINFO_ADVANCED2

        frame_info()
        {
            Buffer_Offset_End=(int64u)-1;
            PCR=(int64u)-1;
            PTS=(int64u)-1;
            DTS=(int64u)-1;
            DUR=(int64u)-1;
            #if MEDIAINFO_ADVANCED2
            PTSc=NoTs;
            DTSc=NoTs;
            Frame_Count_AfterLastTimeStamp=0;
            #endif //MEDIAINFO_ADVANCED2
        }
    };
    frame_info FrameInfo;
    frame_info FrameInfo_Previous;
    frame_info FrameInfo_Next;
    std::vector<int64u> Offsets_Stream;
    std::vector<int64u> Offsets_Buffer;
    size_t              Offsets_Pos;
    int8u*              OriginalBuffer;
    size_t              OriginalBuffer_Size;
    size_t              OriginalBuffer_Capacity;
    #if defined(MEDIAINFO_EIA608_YES) || defined(MEDIAINFO_EIA708_YES)
        struct servicedescriptor
        {
            string language;
            std::bitset<2> wide_aspect_ratio;
        };
        typedef std::map<int8u, servicedescriptor> servicedescriptors608;
        typedef std::map<int8u, servicedescriptor> servicedescriptors708;
        struct servicedescriptors
        {
            #if defined(MEDIAINFO_EIA608_YES)
                std::map<int8u, servicedescriptor> ServiceDescriptors608;
            #endif
            #if defined(MEDIAINFO_EIA708_YES)
                std::map<int8u, servicedescriptor> ServiceDescriptors708;
            #endif
        };
        servicedescriptors* ServiceDescriptors;
    #endif
    #if defined(MEDIAINFO_TELETEXT_YES)
        struct teletext
        {
            std::map<std::string, Ztring>           Infos;
            stream_t                                StreamKind;
            size_t                                  StreamPos;

            teletext()
                : StreamKind(Stream_Max)
                , StreamPos((size_t)-1)
            {}
        };
        std::map<int16u, teletext>*                 Teletexts; //Key is teletext_magazine_number
    #endif

    //Out
    int64u PTS_Begin;                  //In nanoseconds
    #if MEDIAINFO_ADVANCED2
    int64u PTS_Begin_Segment;          //In nanoseconds
    #endif //MEDIAINFO_ADVANCED2
    int64u PTS_End;                    //In nanoseconds
    int64u DTS_Begin;                  //In nanoseconds
    int64u DTS_End;                    //In nanoseconds
    int64u Frame_Count;
    int64u Frame_Count_Previous;
    int64u Frame_Count_InThisBlock;
    int64u Field_Count;
    int64u Field_Count_Previous;
    int64u Field_Count_InThisBlock;
    int64u Frame_Count_NotParsedIncluded;
    int64u FrameNumber_PresentationOrder;
    bool   FrameIsAlwaysComplete;
    bool   Synched;                    //Data is synched
    bool   UnSynched_IsNotJunk;        //Data is actually synched
    bool   MustExtendParsingDuration;  //Data has some substreams difficult to detect (e.g. captions), must wait a bit before final filling

protected :
    //***************************************************************************
    // Streams management
    //***************************************************************************

    virtual void Streams_Accept()                                               {};
    virtual void Streams_Fill()                                                 {};
    virtual void Streams_Update()                                               {};
    virtual void Streams_Finish()                                               {};

    //***************************************************************************
    // Synchro
    //***************************************************************************

    virtual bool Synchronize()    {Synched=true; return true;}; //Look for the synchro
    virtual bool Synched_Test()   {return true;}; //Test is synchro is OK
    virtual void Synched_Init()   {}; //When synched, we can Init data
    bool Synchro_Manage();
    bool Synchro_Manage_Test();

    //***************************************************************************
    // Buffer
    //***************************************************************************

    //Buffer
    virtual void Read_Buffer_Init ()          {}; //Temp, should be in File__Base caller
    virtual void Read_Buffer_OutOfBand ()     {Open_Buffer_Continue(Buffer, Buffer_Size);} //Temp, should be in File__Base caller
    virtual void Read_Buffer_Continue ()      {}; //Temp, should be in File__Base caller
    virtual void Read_Buffer_CheckFileModifications() {} //Temp, should be in File__Base caller
    virtual void Read_Buffer_AfterParsing ()  {}; //Temp, should be in File__Base caller
    #if MEDIAINFO_SEEK
    virtual size_t Read_Buffer_Seek (size_t, int64u, int64u); //Temp, should be in File__Base caller
    size_t Read_Buffer_Seek_OneFramePerFile (size_t, int64u, int64u);
    #endif //MEDIAINFO_SEEK
    #if MEDIAINFO_ADVANCED2
    virtual void Read_Buffer_SegmentChange () {}; //Temp, should be in File__Base caller
    #endif //MEDIAINFO_ADVANCED2
    virtual void Read_Buffer_Unsynched ()     {}; //Temp, should be in File__Base caller
    void Read_Buffer_Unsynched_OneFramePerFile ();
    virtual void Read_Buffer_Finalize ()      {}; //Temp, should be in File__Base caller
    bool Buffer_Parse();

    //***************************************************************************
    // BitStream init
    //***************************************************************************

    void BS_Begin();
    void BS_Begin_LE(); //Little Endian version
    void BS_End();
    void BS_End_LE(); //Little Endian version

    //***************************************************************************
    // File Header
    //***************************************************************************

    //File Header - Management
    bool FileHeader_Manage ();

    //File Header - Begin
    virtual bool FileHeader_Begin ()                                            {return true;};

    //File Header - Parse
    virtual void FileHeader_Parse ()                                            {Element_DoNotShow();};

    //***************************************************************************
    // Header
    //***************************************************************************

    //Header - Management
    bool Header_Manage ();

    //Header - Begin
    virtual bool Header_Begin ()                                                {return true;};

    //Header - Parse
    virtual void Header_Parse ();

    //Header - Info
    void Header_Fill_Code (int64u Code);
    void Header_Fill_Code (int64u Code, const Ztring &Name);
    #define Header_Fill_Code2(A,B) Header_Fill_Code(A,B)
    void Header_Fill_Size (int64u Size);

    //***************************************************************************
    // Data
    //***************************************************************************

    //Header - Management
    bool Data_Manage ();

    //Data - Parse
    virtual void Data_Parse ()                                                  {};

    //Data - Info
    void Data_Info (const Ztring &Parameter);
    inline void Data_Info_From_Milliseconds (int64u Parameter)                  {Data_Info(Ztring().Duration_From_Milliseconds(Parameter));}

    //Data - Get info
    size_t Data_Remain ()                                                       {return (size_t)(Element_Size-(Element_Offset+BS->Offset_Get()));};
    size_t Data_BS_Remain ()                                                    {return (size_t)BS->Remain();};

    //Data - Detect EOF
    virtual void Detect_EOF ()                                                  {};
    bool EOF_AlreadyDetected;

    //Data - Helpers
    void Data_Accept        (const char* ParserName);
    void Data_Reject        (const char* ParserName);
    void Data_Finish        (const char* ParserName);
    void Data_GoTo          (int64u GoTo, const char* ParserName);
    void Data_GoToFromEnd   (int64u GoToFromEnd, const char* ParserName);

    //***************************************************************************
    // Elements
    //***************************************************************************

    //Elements - Begin
    void Element_Begin ();
    void Element_Begin (const Ztring &Name);
    void Element_Begin (const char *Name);
    #define Element_Begin0() Element_Begin()
    #define Element_Begin1(_NAME) Element_Begin(_NAME)
    #define Element_Trace_Begin0() Element_Begin()
    #define Element_Trace_Begin1(_NAME) Element_Begin(_NAME)

    //Elements - Name
    void Element_Name (const Ztring &Name);
    inline void Element_Name (const char*   Name) {Element_Name(Ztring().From_UTF8(Name));}

    //Elements - Info
#if MEDIAINFO_TRACE
    template<typename T>
    void Element_Info (T Parameter, const char* Measure=NULL, int8u AfterComma=3)
    {
        if (Config_Trace_Level<1)
            return;

        //Needed?
        if (Config_Trace_Level<=0.7)
            return;

        Element[Element_Level].TraceNode.Infos.push_back(new element_details::Element_Node_Info(Parameter, Measure, AfterComma));
    }

    void Element_Info (const char* Parameter, const char* Measure=NULL, int8u AfterComma=3)
    {
        if (Config_Trace_Level<1)
            return;

        //Needed?
        if (Config_Trace_Level<=0.7)
            return;

        if ((Parameter && std::string(Parameter) == "NOK") || (Measure && std::string(Measure) == "Error"))
            Element[Element_Level].TraceNode.HasError = true;

        Element[Element_Level].TraceNode.Infos.push_back(new element_details::Element_Node_Info(Parameter, Measure, AfterComma));
    }
#endif //MEDIAINFO_TRACE

    #ifdef SIZE_T_IS_LONG
    inline void Element_Info (size_t        Parameter, const char*   Measure=NULL) {Element_Info(Ztring::ToZtring(Parameter), Measure);}
    #endif //SIZE_T_IS_LONG
    #define Element_Info1(_A) Element_Info(_A)
    #define Element_Info2(_A,_B) Element_Info(_A, _B)
    #define Element_Info3(_A,_B,_C) Element_Info(_A, _B, _C)
    #define Element_Info1C(_CONDITION,_A) if (_CONDITION) Element_Info(_A)
    inline void Element_Info_From_Milliseconds (int64u Parameter)                  {if (Config_Trace_Level<1) return; Element_Info(Ztring().Duration_From_Milliseconds(Parameter));}

    void Element_Parser (const char* Name);
    void Element_Error (const char* Name);
    void Param_Error (const char* Name);

    //Elements - End
    inline void Element_End () {Element_End_Common_Flush();}
    void Element_End (const Ztring &Name);
    inline void Element_End (const char *Name) {Element_End(Ztring().From_UTF8(Name));}
    #define Element_End0() Element_End()
    #define Element_End1(_NAME) Element_End(_NAME)
    #define Element_Trace_End0() Element_End()
    #define Element_Trace_End1(_NAME) Element_End(_NAME)

    //Elements - Preparation of element from external app
    void Element_Prepare (int64u Size);

protected :
    //Element - Common
    void   Element_End_Common_Flush();
    void   Element_End_Common_Flush_Details();
public :

    //***************************************************************************
    // Param
    //***************************************************************************

    //TODO: put this in Ztring()
    Ztring ToZtring(const char* Value, size_t Value_Size=Unlimited, bool Utf8=true)
    {
        if (Utf8)
            return Ztring().From_UTF8(Value, Value_Size);
        else
            return Ztring().From_Local(Value, Value_Size);
    }

    //Param - Main
    template<typename T>
    inline void Param(const std::string &Parameter, T Value, int8u GenericOption=(int8u)-1)
    {
        if (!Trace_Activated)
            return;

        if (Config_Trace_Level==0 || !(Trace_Layers.to_ulong()&Config_Trace_Layers.to_ulong()))
            return ;

        //Coherancy
        if (Element[Element_Level].UnTrusted)
            return ;

        element_details::Element_Node *node = new element_details::Element_Node;
        node->Set_Name(Parameter);
        node->Pos = File_Offset+Buffer_Offset+Element_Offset;
        if (BS_Size)
        {
            int64u BS_BitOffset = BS_Size-BS->Remain();
            if (GenericOption != (int8u)-1)
                BS_BitOffset -= GenericOption;
            node->Pos += BS_BitOffset>>3; //Including Bits to Bytes
        }
        node->Value.set_Option(GenericOption);
        node->Value = Value;
        Element[Element_Level].TraceNode.Current_Child = Element[Element_Level].TraceNode.Children.size();
        Element[Element_Level].TraceNode.Children.push_back(node);
    }

    inline void Param      (const char*   Parameter, const char*   Value, size_t Value_Size, bool Utf8=true) {Param(Parameter, ToZtring(Value, Value_Size, Utf8));}
    inline void Param      (const char*   Parameter, const int8u*  Value, size_t Value_Size, bool Utf8=true) {Param(Parameter, (const char*)Value, Value_Size, Utf8);}
    inline void Param_GUID (const char*   Parameter, int128u Value){Param(Parameter, Ztring().From_GUID(Value));}
    inline void Param_UUID (const char*   Parameter, int128u Value){Param(Parameter, Ztring().From_UUID(Value));}
    inline void Param_CC   (const char*   Parameter, const int8u*  Value, int8u Value_Size){Ztring Name2; for (int8s i=0; i<Value_Size; i++) Name2.append(1, (ZenLib::Char)(Value[i])); Param(Parameter, Name2);}
    /* #ifdef SIZE_T_IS_LONG */
    /* inline void Param      (const char*   Parameter, size_t Value, intu Radix) {if (Trace_Activated) Param(Parameter, Ztring::ToZtring(Value, Radix).MakeUpperCase()+__T(" (")+Ztring::ToZtring(Value, 10).MakeUpperCase()+__T(")"));} */
    /* #endif //SIZE_T_IS_LONG */
    inline void Param      (const int32u  Parameter, const Ztring& Value) {if (Trace_Activated) Param(Ztring().From_CC4(Parameter).To_UTF8(), Value.To_UTF8());};
    inline void Param      (const int16u  Parameter, const Ztring& Value) {if (Trace_Activated) Param(Ztring().From_CC2(Parameter).To_UTF8(), Value.To_UTF8());};
    #define Param1(_A) Param_(_A)
    #define Param2(_A,_B) Param(_A, _B)
    #define Param3(_A,_B,_C) Param(_A, _B, _C)

    //Param - Info
#if MEDIAINFO_TRACE
    template<typename T>
    void Param_Info(T Parameter, const char* Measure=NULL, int8u AfterComma=3)
    {
        //Coherancy
        if (!Trace_Activated)
            return;
        if (Element[Element_Level].UnTrusted)
            return;
        if (Config_Trace_Level<=0.7)
            return;

        // if (!(Trace_Layers.to_ulong()&Config_Trace_Layers.to_ulong()) || Element[Element_Level].TraceNode.Details.size()>64*1024*1024)
        //     return;
        int32s child = Element[Element_Level].TraceNode.Current_Child;
        if (child >= 0 && Element[Element_Level].TraceNode.Children[child])
            Element[Element_Level].TraceNode.Children[child]->Infos.push_back(new element_details::Element_Node_Info(Parameter, Measure, AfterComma));
        else
            Element[Element_Level].TraceNode.Infos.push_back(new element_details::Element_Node_Info(Parameter, Measure, AfterComma));
    }

    void Param_Info(const char* Parameter, const char* Measure=NULL, int8u AfterComma=3)
    {
        //Coherancy
        if (!Trace_Activated)
            return;
        if (Element[Element_Level].UnTrusted)
            return;
        if (Config_Trace_Level<=0.7)
            return;

        if ((Parameter && std::string(Parameter) == "NOK") || (Measure && std::string(Measure) == "Error"))
            Element[Element_Level].TraceNode.HasError = true;

        int32s child = Element[Element_Level].TraceNode.Current_Child;
        if (child >= 0 && Element[Element_Level].TraceNode.Children[child])
            Element[Element_Level].TraceNode.Children[child]->Infos.push_back(new element_details::Element_Node_Info(Parameter, Measure, AfterComma));
        else
            Element[Element_Level].TraceNode.Infos.push_back(new element_details::Element_Node_Info(Parameter, Measure, AfterComma));
    }
#endif //MEDIAINFO_TRACE

    #ifdef SIZE_T_IS_LONG
    inline void Param_Info (size_t        Parameter, const char*   Measure=NULL) {if (Trace_Activated) Param_Info(Ztring::ToZtring(Parameter)+Ztring().From_UTF8(Measure));}
    #endif //SIZE_T_IS_LONG
    #define Param_Info1(_A) Param_Info(_A)
    #define Param_Info2(_A,_B) Param_Info(_A, _B)
    #define Param_Info3(_A,_B,_C) Param_Info(_A, _B, _C)
    #define Param_Info1C(_CONDITION,_A) if (_CONDITION) Param_Info(_A)
    #define Param_Info2C(_CONDITION,_A,_B) if (_CONDITION) Param_Info(_A, _B)
    #define Param_Info3C(_CONDITION,_A,_B,_C) if (_CONDITION) Param_Info(_A, _B, _C)
    inline void Param_Info_From_Milliseconds (int64u Parameter)                  {if (Trace_Activated) Param_Info(Ztring().Duration_From_Milliseconds(Parameter));}

    //***************************************************************************
    // Element Node
    //***************************************************************************
    element_details::Element_Node *Get_Trace_Node(size_t level = 0);

    //***************************************************************************
    // Information
    //***************************************************************************

    void Info (const std::string& Value, size_t Element_Level_Minus=0);

    //***************************************************************************
    // Big Endian (Integer, Float, Fixed-Point)
    //***************************************************************************

    void Get_B1   (int8u   &Info, const char* Name);
    void Get_B2   (int16u  &Info, const char* Name);
    void Get_B3   (int32u  &Info, const char* Name);
    void Get_B4   (int32u  &Info, const char* Name);
    void Get_B5   (int64u  &Info, const char* Name);
    void Get_B6   (int64u  &Info, const char* Name);
    void Get_B7   (int64u  &Info, const char* Name);
    void Get_B8   (int64u  &Info, const char* Name);
    void Get_B16  (int128u &Info, const char* Name);
    void Get_BF2  (float32 &Info, const char* Name);
    void Get_BF4  (float32 &Info, const char* Name);
    void Get_BF8  (float64 &Info, const char* Name);
    void Get_BF10 (float80 &Info, const char* Name);
    void Get_BFP4 (int8u Bits, float32 &Info, const char* Name);
    void Peek_B1  (int8u   &Info);
    void Peek_B2  (int16u  &Info);
    void Peek_B3  (int32u  &Info);
    void Peek_B4  (int32u  &Info);
    void Peek_B5  (int64u  &Info);
    void Peek_B6  (int64u  &Info);
    void Peek_B7  (int64u  &Info);
    void Peek_B8  (int64u  &Info);
    void Peek_B16 (int128u &Info);
    void Peek_BF4 (float32 &Info);
    void Peek_BF8 (float64 &Info);
    void Peek_BF10(float64 &Info);
    void Peek_BFP4(size_t Bits, float64 &Info);
    void Skip_B1  (               const char* Name);
    void Skip_B2  (               const char* Name);
    void Skip_B3  (               const char* Name);
    void Skip_B4  (               const char* Name);
    void Skip_B5  (               const char* Name);
    void Skip_B6  (               const char* Name);
    void Skip_B7  (               const char* Name);
    void Skip_B8  (               const char* Name);
    void Skip_B16 (               const char* Name);
    void Skip_BF4 (               const char* Name);
    void Skip_BF8 (               const char* Name);
    void Skip_BF10(               const char* Name);
    void Skip_BFP4(int8u Bits,                const char* Name);
    void Skip_Hexa(int8u Bytes,   const char* Name);
    #define Info_B1(_INFO, _NAME)   int8u   _INFO; Get_B1  (_INFO, _NAME)
    #define Info_B2(_INFO, _NAME)   int16u  _INFO; Get_B2  (_INFO, _NAME)
    #define Info_B3(_INFO, _NAME)   int32u  _INFO; Get_B3  (_INFO, _NAME)
    #define Info_B4(_INFO, _NAME)   int32u  _INFO; Get_B4  (_INFO, _NAME)
    #define Info_B5(_INFO, _NAME)   int64u  _INFO; Get_B5  (_INFO, _NAME)
    #define Info_B6(_INFO, _NAME)   int64u  _INFO; Get_B6  (_INFO, _NAME)
    #define Info_B7(_INFO, _NAME)   int64u  _INFO; Get_B7  (_INFO, _NAME)
    #define Info_B8(_INFO, _NAME)   int64u  _INFO; Get_B8  (_INFO, _NAME)
    #define Info_B16(_INFO, _NAME)  int128u _INFO; Get_B16 (_INFO, _NAME)
    #define Info_BF2(_INFO, _NAME)  float32 _INFO; Get_BF2 (_INFO, _NAME)
    #define Info_BF4(_INFO, _NAME)  float32 _INFO; Get_BF4 (_INFO, _NAME)
    #define Info_BF8(_INFO, _NAME)  float64 _INFO; Get_BF8 (_INFO, _NAME)
    #define Info_BF10(_INFO, _NAME) float80 _INFO; Get_BF10(_INFO, _NAME)
    #define Info_BFP4(_BITS, _INFO, _NAME) float32 _INFO; Get_BFP4(_BITS, _INFO, _NAME)

    //***************************************************************************
    // Little Endian
    //***************************************************************************

    void Get_L1  (int8u   &Info, const char* Name);
    void Get_L2  (int16u  &Info, const char* Name);
    void Get_L3  (int32u  &Info, const char* Name);
    void Get_L4  (int32u  &Info, const char* Name);
    void Get_L5  (int64u  &Info, const char* Name);
    void Get_L6  (int64u  &Info, const char* Name);
    void Get_L7  (int64u  &Info, const char* Name);
    void Get_L8  (int64u  &Info, const char* Name);
    void Get_L16 (int128u &Info, const char* Name);
    void Get_LF4 (float32 &Info, const char* Name);
    void Get_LF8 (float64 &Info, const char* Name);
    void Peek_L1 (int8u   &Info);
    void Peek_L2 (int16u  &Info);
    void Peek_L3 (int32u  &Info);
    void Peek_L4 (int32u  &Info);
    void Peek_L5 (int64u  &Info);
    void Peek_L6 (int64u  &Info);
    void Peek_L7 (int64u  &Info);
    void Peek_L8 (int64u  &Info);
    void Peek_L16(int128u &Info);
    void Peek_LF4(float32 &Info);
    void Peek_LF8(float64 &Info);
    void Skip_L1 (               const char* Name);
    void Skip_L2 (               const char* Name);
    void Skip_L3 (               const char* Name);
    void Skip_L4 (               const char* Name);
    void Skip_L5 (               const char* Name);
    void Skip_L6 (               const char* Name);
    void Skip_L7 (               const char* Name);
    void Skip_L8 (               const char* Name);
    void Skip_LF4(               const char* Name);
    void Skip_LF8(               const char* Name);
    void Skip_L16(               const char* Name);
    #define Info_L1(_INFO, _NAME)  int8u   _INFO; Get_L1 (_INFO, _NAME)
    #define Info_L2(_INFO, _NAME)  int16u  _INFO; Get_L2 (_INFO, _NAME)
    #define Info_L3(_INFO, _NAME)  int32u  _INFO; Get_L3 (_INFO, _NAME)
    #define Info_L4(_INFO, _NAME)  int32u  _INFO; Get_L4 (_INFO, _NAME)
    #define Info_L5(_INFO, _NAME)  int64u  _INFO; Get_L5 (_INFO, _NAME)
    #define Info_L6(_INFO, _NAME)  int64u  _INFO; Get_L6 (_INFO, _NAME)
    #define Info_L7(_INFO, _NAME)  int64u  _INFO; Get_L7 (_INFO, _NAME)
    #define Info_L8(_INFO, _NAME)  int64u  _INFO; Get_L8 (_INFO, _NAME)
    #define Info_L16(_INFO, _NAME) int128u _INFO; Get_L16(_INFO, _NAME)
    #define Info_LF4(_INFO, _NAME) float32 _INFO; Get_LF4(_INFO, _NAME)
    #define Info_LF8(_INFO, _NAME) float64 _INFO; Get_LF8(_INFO, _NAME)

    //***************************************************************************
    // Little and Big Endian together
    //***************************************************************************

    void Get_D1  (int8u   &Info, const char* Name);
    void Get_D2  (int16u  &Info, const char* Name);
    void Get_D3  (int32u  &Info, const char* Name);
    void Get_D4  (int32u  &Info, const char* Name);
    void Get_D5  (int64u  &Info, const char* Name);
    void Get_D6  (int64u  &Info, const char* Name);
    void Get_D7  (int64u  &Info, const char* Name);
    void Get_D8  (int64u  &Info, const char* Name);
    void Get_D16 (int128u &Info, const char* Name);
    void Get_DF4 (float32 &Info, const char* Name);
    void Get_DF8 (float64 &Info, const char* Name);
    void Peek_D1 (int8u   &Info);
    void Peek_D2 (int16u  &Info);
    void Peek_D3 (int32u  &Info);
    void Peek_D4 (int32u  &Info);
    void Peek_D5 (int64u  &Info);
    void Peek_D6 (int64u  &Info);
    void Peek_D7 (int64u  &Info);
    void Peek_D8 (int64u  &Info);
    void Peek_D16(int128u &Info);
    void Peek_DF4(float32 &Info);
    void Peek_DF8(float64 &Info);
    void Skip_D1 (               const char* Name);
    void Skip_D2 (               const char* Name);
    void Skip_D3 (               const char* Name);
    void Skip_D4 (               const char* Name);
    void Skip_D5 (               const char* Name);
    void Skip_D6 (               const char* Name);
    void Skip_D7 (               const char* Name);
    void Skip_D8 (               const char* Name);
    void Skip_DF4(               const char* Name);
    void Skip_DF8(               const char* Name);
    void Skip_D16(               const char* Name);
    #define Info_D1(_INFO, _NAME)  int8u   _INFO; Get_D1 (_INFO, _NAME)
    #define Info_D2(_INFO, _NAME)  int16u  _INFO; Get_D2 (_INFO, _NAME)
    #define Info_D3(_INFO, _NAME)  int32u  _INFO; Get_D3 (_INFO, _NAME)
    #define Info_D4(_INFO, _NAME)  int32u  _INFO; Get_D4 (_INFO, _NAME)
    #define Info_D5(_INFO, _NAME)  int64u  _INFO; Get_D5 (_INFO, _NAME)
    #define Info_D6(_INFO, _NAME)  int64u  _INFO; Get_D6 (_INFO, _NAME)
    #define Info_D7(_INFO, _NAME)  int64u  _INFO; Get_D7 (_INFO, _NAME)
    #define Info_D8(_INFO, _NAME)  int64u  _INFO; Get_D8 (_INFO, _NAME)
    #define Info_D16(_INFO, _NAME) int128u _INFO; Get_D16(_INFO, _NAME)
    #define Info_DF4(_INFO, _NAME) float32 _INFO; Get_DF4(_INFO, _NAME)
    #define Info_DF8(_INFO, _NAME) float64 _INFO; Get_DF8(_INFO, _NAME)

    //***************************************************************************
    // GUID
    //***************************************************************************

    void Get_GUID (int128u &Info, const char* Name);
    void Peek_GUID(int128u &Info);
    void Skip_GUID(               const char* Name);
    #define Info_GUID(_INFO, _NAME) int128u _INFO; Get_GUID(_INFO, _NAME)

    //***************************************************************************
    // UUID
    //***************************************************************************

    void Get_UUID (int128u &Info, const char* Name);
    void Peek_UUID(int128u &Info);
    void Skip_UUID(               const char* Name);
    #define Info_UUID(_INFO, _NAME) int128u _INFO; Get_UUID(_INFO, _NAME)

    //***************************************************************************
    // EBML
    //***************************************************************************

    void Get_EB (int64u &Info, const char* Name);
    void Get_ES (int64s &Info, const char* Name);
    void Skip_EB(              const char* Name);
    void Skip_ES(              const char* Name);
    #define Info_EB(_INFO, _NAME) int64u _INFO; Get_EB(_INFO, _NAME)
    #define Info_ES(_INFO, _NAME) int64s _INFO; Get_ES(_INFO, _NAME)

    //***************************************************************************
    // Variable Size Value
    //***************************************************************************

    void Get_VS (int64u &Info, const char* Name);
    void Skip_VS(              const char* Name);
    #define Info_VS(_INFO, _NAME) int64u _INFO; Get_VS(_INFO, _NAME)

    //***************************************************************************
    // Exp-Golomb
    //***************************************************************************

    void Get_UE (int32u &Info, const char* Name);
    void Get_SE (int32s &Info, const char* Name);
    void Skip_UE(              const char* Name);
    void Skip_SE(              const char* Name);
    #define Info_UE(_INFO, _NAME) int32u _INFO; Get_UE(_INFO, _NAME)
    #define Info_SE(_INFO, _NAME) int32s _INFO; Get_SE(_INFO, _NAME)

    //***************************************************************************
    // Interleaved Exp-Golomb
    //***************************************************************************

    void Get_UI (int32u &Info, const char* Name);
    void Get_SI (int32s &Info, const char* Name);
    void Skip_UI(              const char* Name);
    void Skip_SI(              const char* Name);
    #define Info_UI(_INFO, _NAME) int32u _INFO; Get_UI(_INFO, _NAME)
    #define Info_SI(_INFO, _NAME) int32s _INFO; Get_SI(_INFO, _NAME)

    //***************************************************************************
    // Variable Length Code
    //***************************************************************************

    struct vlc
    {
        int32u  value;
        int8u   bit_increment;
        int8s   mapped_to1;
        int8s   mapped_to2;
        int8s   mapped_to3;
    };
    struct vlc_fast
    {
        int8u*      Array;
        int8u*      BitsToSkip;
        const vlc*  Vlc;
        int8u       Size;
    };
    #define VLC_END \
        {(int32u)-1, (int8u)-1, 0, 0, 0}
    static void Get_VL_Prepare(vlc_fast &Vlc);
    void Get_VL (const vlc Vlc[], size_t &Info, const char* Name);
    void Get_VL (vlc_fast &Vlc, size_t &Info, const char* Name);
    void Skip_VL(const vlc Vlc[], const char* Name);
    void Skip_VL(vlc_fast &Vlc, const char* Name);
    #define Info_VL(Vlc, Info, Name) size_t Info; Get_VL(Vlc, Info, Name)

    //***************************************************************************
    // Characters
    //***************************************************************************

    void Get_C1 (int8u  &Info, const char* Name);
    void Get_C2 (int16u &Info, const char* Name);
    void Get_C3 (int32u &Info, const char* Name);
    void Get_C4 (int32u &Info, const char* Name);
    void Get_C5 (int64u &Info, const char* Name);
    void Get_C6 (int64u &Info, const char* Name);
    void Get_C7 (int64u &Info, const char* Name);
    void Get_C8 (int64u &Info, const char* Name);
    void Skip_C1(              const char* Name);
    void Skip_C2(              const char* Name);
    void Skip_C3(              const char* Name);
    void Skip_C4(              const char* Name);
    void Skip_C5(              const char* Name);
    void Skip_C6(              const char* Name);
    void Skip_C7(              const char* Name);
    void Skip_C8(              const char* Name);
    #define Info_C1(_INFO, _NAME) int8u  _INFO; Get_C1(_INFO, _NAME)
    #define Info_C2(_INFO, _NAME) int16u _INFO; Get_C2(_INFO, _NAME)
    #define Info_C3(_INFO, _NAME) int32u _INFO; Get_C3(_INFO, _NAME)
    #define Info_C4(_INFO, _NAME) int32u _INFO; Get_C4(_INFO, _NAME)
    #define Info_C5(_INFO, _NAME) int64u _INFO; Get_C5(_INFO, _NAME)
    #define Info_C6(_INFO, _NAME) int64u _INFO; Get_C6(_INFO, _NAME)
    #define Info_C7(_INFO, _NAME) int64u _INFO; Get_C7(_INFO, _NAME)
    #define Info_C8(_INFO, _NAME) int64u _INFO; Get_C8(_INFO, _NAME)

    //***************************************************************************
    // Text
    //***************************************************************************

    void Get_Local  (int64u Bytes, Ztring      &Info, const char* Name);
    void Get_ISO_6937_2(int64u Bytes, Ztring   &Info, const char* Name);
    void Get_ISO_8859_1(int64u Bytes, Ztring   &Info, const char* Name);
    void Get_ISO_8859_2(int64u Bytes, Ztring   &Info, const char* Name);
    void Get_ISO_8859_5(int64u Bytes, Ztring   &Info, const char* Name);
    void Get_ISO_8859_9(int64u Bytes, Ztring   &Info, const char* Name);
    void Get_MacRoman(int64u Bytes, Ztring& Info, const char* Name);
    void Get_String (int64u Bytes, std::string &Info, const char* Name);
    void Get_UTF8   (int64u Bytes, Ztring      &Info, const char* Name);
    void Get_UTF16  (int64u Bytes, Ztring      &Info, const char* Name);
    void Get_UTF16B (int64u Bytes, Ztring      &Info, const char* Name);
    void Get_UTF16L (int64u Bytes, Ztring      &Info, const char* Name);
    void Peek_Local (int64u Bytes, Ztring      &Info);
    void Peek_UTF8  (int64u Bytes, Ztring      &Info);
    void Peek_String(int64u Bytes, std::string &Info);
    void Skip_Local (int64u Bytes,                    const char* Name);
    void Skip_ISO_6937_2(int64u Bytes,                const char* Name);
    void Skip_ISO_8859_1(int64u Bytes,                const char* Name);
    void Skip_String(int64u Bytes,                    const char* Name);
    void Skip_UTF8  (int64u Bytes,                    const char* Name);
    void Skip_UTF16B(int64u Bytes,                    const char* Name);
    void Skip_UTF16L(int64u Bytes,                    const char* Name);
    #define Info_Local(_BYTES, _INFO, _NAME)  Ztring _INFO; Get_Local (_BYTES, _INFO, _NAME)
    #define Info_ISO_6937_2(_BYTES, _INFO, _NAME)  Ztring _INFO; Get_ISO_6937_2 (_BYTES, _INFO, _NAME)
    #define Info_UTF8(_BYTES, _INFO, _NAME)   Ztring _INFO; Get_UTF8  (_BYTES, _INFO, _NAME)
    #define Info_UTF16B(_BYTES, _INFO, _NAME) Ztring _INFO; Get_UTF16B(_BYTES, _INFO, _NAME)
    #define Info_UTF16L(_BYTES, _INFO, _NAME) Ztring _INFO; Get_UTF16L(_BYTES, _INFO, _NAME)
    size_t SizeUpTo0(size_t Size=(size_t)-1);

    //***************************************************************************
    // PAscal strings
    //***************************************************************************

    void Get_PA (std::string &Info, const char* Name);
    void Peek_PA(std::string &Info);
    void Skip_PA(                   const char* Name);
    #define Info_PA(_INFO, _NAME) Ztring _INFO; Get_PA (_INFO, _NAME)

    //***************************************************************************
    // Others, specialized
    //***************************************************************************

    void Attachment(const char* MuxingMode, const Ztring& Description = {}, const Ztring& Type = {}, const Ztring& MimeType = {}, bool IsCover = {});
    #if defined(MEDIAINFO_AV1_YES) || defined(MEDIAINFO_AVC_YES) || defined(MEDIAINFO_HEVC_YES) || defined(MEDIAINFO_MPEG4_YES) || defined(MEDIAINFO_MPEGTS_YES) || defined(MEDIAINFO_PNG_YES)
    void Get_MasteringDisplayColorVolume(Ztring& MasteringDisplay_ColorPrimaries, Ztring& MasteringDisplay_Luminance, bool FromAV1=false);
    void Get_LightLevel(Ztring &MaxCLL, Ztring &MaxFALL, int32u Divisor=1);
    #endif
    #if defined(MEDIAINFO_AV1_YES) || defined(MEDIAINFO_AVC_YES) || defined(MEDIAINFO_HEVC_YES) || defined(MEDIAINFO_MPEG4_YES) || defined(MEDIAINFO_MATROSKA_YES) || defined(MEDIAINFO_MXF_YES) || defined(MEDIAINFO_MPEGTS_YES)
    struct mastering_metadata_2086
    {
        int16u Primaries[8];
        int32u Luminance[2];
    };
    void Get_MasteringDisplayColorVolume(Ztring &MasteringDisplay_ColorPrimaries, Ztring &MasteringDisplay_Luminance, mastering_metadata_2086 &Meta, bool FromAV1=false);
    #endif
    #if defined(MEDIAINFO_MPEGPS_YES) || defined(MEDIAINFO_MPEGTS_YES) || defined(MEDIAINFO_MPEG4_YES) || defined(MEDIAINFO_MK_YES)
    void dvcC(bool has_dependency_pid=false, std::map<std::string, Ztring>* Infos=NULL);
    #endif

    //***************************************************************************
    // Unknown
    //***************************************************************************

    void Skip_XX(int64u Bytes, const char* Name);

    //***************************************************************************
    // Flags
    //***************************************************************************

    void Get_Flags (int64u Flags, size_t Order, bool  &Info, const char* Name);
    void Get_Flags (int64u ValueToPut,          int8u &Info, const char* Name);
    void Skip_Flags(int64u Flags, size_t Order,              const char* Name);
    void Skip_Flags(int64u ValueToPut,                       const char* Name);
    #define Get_FlagsM(_VALUE, _INFO, _NAME) Get_Flags(_VALUE, _INFO, _NAME)
    #define Skip_FlagsM(_VALUE, _NAME) Skip_Flags(_VALUE, _NAME)
    #define Info_Flags(_FLAGS, _ORDER, _INFO, _NAME) bool _INFO; Get_Flags (_FLAGS, _ORDER, _INFO, _NAME)

    //***************************************************************************
    // BitStream
    //***************************************************************************

    void Get_BS (int8u  Bits, int32u  &Info, const char* Name);
    void Get_SB (             bool    &Info, const char* Name);
    bool Get_SB(                             const char* Name)  {bool Temp; Get_SB(Temp, Name); return Temp;}
    void Get_S1 (int8u  Bits, int8u   &Info, const char* Name);
    void Get_S2 (int8u  Bits, int16u  &Info, const char* Name);
    void Get_S3 (int8u  Bits, int32u  &Info, const char* Name);
    void Get_S4 (int8u  Bits, int32u  &Info, const char* Name);
    void Get_S5 (int8u  Bits, int64u  &Info, const char* Name);
    void Get_S6 (int8u  Bits, int64u  &Info, const char* Name);
    void Get_S7 (int8u  Bits, int64u  &Info, const char* Name);
    void Get_S8 (int8u  Bits, int64u  &Info, const char* Name);
    void Peek_BS(int8u  Bits, int32u  &Info);
    void Peek_SB(             bool    &Info);
    bool Peek_SB()                                              {bool Temp; Peek_SB(Temp); return Temp;}
    void Peek_S1(int8u  Bits, int8u   &Info);
    void Peek_S2(int8u  Bits, int16u  &Info);
    void Peek_S3(int8u  Bits, int32u  &Info);
    void Peek_S4(int8u  Bits, int32u  &Info);
    void Peek_S5(int8u  Bits, int64u  &Info);
    void Peek_S6(int8u  Bits, int64u  &Info);
    void Peek_S7(int8u  Bits, int64u  &Info);
    void Peek_S8(int8u  Bits, int64u  &Info);
    void Skip_BS(size_t Bits,                const char* Name);
    void Skip_SB(                            const char* Name);
    void Skip_S1(int8u  Bits,                const char* Name);
    void Skip_S2(int8u  Bits,                const char* Name);
    void Skip_S3(int8u  Bits,                const char* Name);
    void Skip_S4(int8u  Bits,                const char* Name);
    void Skip_S5(int8u  Bits,                const char* Name);
    void Skip_S6(int8u  Bits,                const char* Name);
    void Skip_S7(int8u  Bits,                const char* Name);
    void Skip_S8(int8u  Bits,                const char* Name);
    void Mark_0 ();
    void Mark_0_NoTrustError (); //Use it for providing a warning instead of a non-trusting error
    void Mark_1 ();
    void Mark_1_NoTrustError (); //Use it for providing a warning instead of a non-trusting error
    #define Info_BS(_BITS, _INFO, _NAME) int32u  _INFO; Get_BS(_BITS, _INFO, _NAME)
    #define Info_SB(_INFO, _NAME)        bool    _INFO; Get_SB(       _INFO, _NAME)
    #define Info_S1(_BITS, _INFO, _NAME) int8u   _INFO; Get_S1(_BITS, _INFO, _NAME)
    #define Info_S2(_BITS, _INFO, _NAME) int16u  _INFO; Get_S2(_BITS, _INFO, _NAME)
    #define Info_S3(_BITS, _INFO, _NAME) int32u  _INFO; Get_S4(_BITS, _INFO, _NAME)
    #define Info_S4(_BITS, _INFO, _NAME) int32u  _INFO; Get_S4(_BITS, _INFO, _NAME)
    #define Info_S5(_BITS, _INFO, _NAME) int64u  _INFO; Get_S5(_BITS, _INFO, _NAME)
    #define Info_S6(_BITS, _INFO, _NAME) int64u  _INFO; Get_S6(_BITS, _INFO, _NAME)
    #define Info_S7(_BITS, _INFO, _NAME) int64u  _INFO; Get_S7(_BITS, _INFO, _NAME)
    #define Info_S8(_BITS, _INFO, _NAME) int64u  _INFO; Get_S8(_BITS, _INFO, _NAME)

    #define TEST_SB_GET(_CODE, _NAME) \
        { \
            Peek_SB(_CODE); \
            if (!_CODE) \
                Skip_SB(                                        _NAME); \
            else \
            { \
                Element_Begin1(_NAME); \
                Skip_SB(                                        _NAME); \

    #define TEST_SB_SKIP(_NAME) \
        { \
            if (!Peek_SB()) \
                Skip_SB(                                        _NAME); \
            else \
            { \
                Element_Begin1(_NAME); \
                Skip_SB(                                        _NAME); \

    #define TESTELSE_SB_GET(_CODE, _NAME) \
        { \
            Peek_SB(_CODE); \
            if (_CODE) \
            { \
                Element_Begin1(_NAME); \
                Skip_SB(                                        _NAME); \

    #define TESTELSE_SB_SKIP(_NAME) \
        { \
            if (Peek_SB()) \
            { \
                Element_Begin1(_NAME); \
                Skip_SB(                                        _NAME); \

    #define TESTELSE_SB_ELSE(_NAME) \
                Element_End0(); \
            } \
            else \
            { \
                Skip_SB(                                        _NAME); \

    #define TESTELSE_SB_END() \
            } \
        } \

    #define TEST_SB_END() \
                Element_End0(); \
            } \
        } \

    //***************************************************************************
    // BitStream (Little Endian)
    //***************************************************************************

    void Get_BT (size_t Bits, int32u  &Info, const char* Name);
    void Get_TB (             bool    &Info, const char* Name);
    bool Get_TB(                             const char* Name)  {bool Temp; Get_TB(Temp, Name); return Temp;}
    void Get_T1 (size_t Bits, int8u   &Info, const char* Name);
    void Get_T2 (size_t Bits, int16u  &Info, const char* Name);
    void Get_T4 (size_t Bits, int32u  &Info, const char* Name);
    void Get_T8 (size_t Bits, int64u  &Info, const char* Name);
    void Peek_BT(size_t Bits, int32u  &Info);
    void Peek_TB(              bool    &Info);
    bool Peek_TB()                                              {bool Temp; Peek_TB(Temp); return Temp;}
    void Peek_T1(size_t Bits, int8u   &Info);
    void Peek_T2(size_t Bits, int16u  &Info);
    void Peek_T4(size_t Bits, int32u  &Info);
    void Peek_T8(size_t Bits, int64u  &Info);
    void Skip_BT(size_t Bits,                const char* Name);
    void Skip_TB(                            const char* Name);
    void Skip_T1(size_t Bits,                const char* Name);
    void Skip_T2(size_t Bits,                const char* Name);
    void Skip_T4(size_t Bits,                const char* Name);
    void Skip_T8(size_t Bits,                const char* Name);
    #define Info_BT(_BITS, _INFO, _NAME) int32u  _INFO; Get_BT(_BITS, _INFO, _NAME)
    #define Info_TB(_INFO, _NAME)        bool    _INFO; Get_TB(       _INFO, _NAME)
    #define Info_T1(_BITS, _INFO, _NAME) int8u   _INFO; Get_T1(_BITS, _INFO, _NAME)
    #define Info_T2(_BITS, _INFO, _NAME) int16u  _INFO; Get_T2(_BITS, _INFO, _NAME)
    #define Info_T4(_BITS, _INFO, _NAME) int32u  _INFO; Get_T4(_BITS, _INFO, _NAME)
    #define Info_T8(_BITS, _INFO, _NAME) int64u  _INFO; Get_T8(_BITS, _INFO, _NAME)

    #define TEST_TB_GET(_CODE, _NAME) \
        { \
            Peek_TB(_CODE); \
            if (!_CODE) \
                Skip_TB(                                        _NAME); \
            else \
            { \
                Element_Begin1(_NAME); \
                Skip_TB(                                        _NAME); \

    #define TEST_TB_SKIP(_NAME) \
        { \
            if (!Peek_TB()) \
                Skip_TB(                                        _NAME); \
            else \
            { \
                Element_Begin1(_NAME); \
                Skip_TB(                                        _NAME); \

    #define TESTELSE_TB_GET(_CODE, _NAME) \
        { \
            Peek_TB(_CODE); \
            if (_CODE) \
            { \
                Element_Begin1(_NAME); \
                Skip_TB(                                        _NAME); \

    #define TESTELSE_TB_SKIP(_NAME) \
        { \
            if (Peek_TB()) \
            { \
                Element_Begin1(_NAME); \
                Skip_TB(                                        _NAME); \

    #define TESTELSE_TB_ELSE(_NAME) \
                Element_End0(); \
            } \
            else \
            { \
                Skip_TB(                                        _NAME); \

    #define TESTELSE_TB_END() \
            } \
        } \

    #define TEST_TB_END() \
                Element_End0(); \
            } \
        } \

    //***************************************************************************
    // Next code planning
    //***************************************************************************

    void NextCode_Add(int64u Code);
    void NextCode_Clear();
    bool NextCode_Test();

    //***************************************************************************
    // Element trusting
    //***************************************************************************

    void Trusted_IsNot (const char* Reason);
    bool Trusted_Get   () {return !Element[Element_Level].UnTrusted;}

    //***************************************************************************
    // Stream filling
    //***************************************************************************

    //Elements - Preparation of element from external app
    size_t Stream_Prepare   (stream_t KindOfStream, size_t StreamPos=(size_t)-1);
    size_t Stream_Erase     (stream_t KindOfStream, size_t StreamPos);

    //Fill with datas (with parameter as a size_t)
    void Fill (stream_t StreamKind, size_t StreamPos, size_t Parameter, const Ztring  &Value, bool Replace=false);
    inline void Fill (stream_t StreamKind, size_t StreamPos, size_t Parameter, const std::string &Value, bool Utf8=true, bool Replace=false) {if (Utf8) Fill(StreamKind, StreamPos, Parameter, Ztring().From_UTF8(Value.c_str(), Value.size()), Replace); else Fill(StreamKind, StreamPos, Parameter, Ztring().From_Local(Value.c_str(), Value.size()), Replace);}
    inline void Fill (stream_t StreamKind, size_t StreamPos, size_t Parameter, const char*    Value, size_t Value_Size=Unlimited, bool Utf8=true, bool Replace=false) {if (Utf8) Fill(StreamKind, StreamPos, Parameter, Ztring().From_UTF8(Value, Value_Size), Replace); else Fill(StreamKind, StreamPos, Parameter, Ztring().From_Local(Value, Value_Size), Replace);}
    inline void Fill (stream_t StreamKind, size_t StreamPos, size_t Parameter, const wchar_t* Value, size_t Value_Size=Unlimited, bool Replace=false) {Fill(StreamKind, StreamPos, Parameter, Ztring().From_Unicode(Value, Value_Size), Replace);}
    inline void Fill (stream_t StreamKind, size_t StreamPos, size_t Parameter, int8u          Value, int8u Radix=10, bool Replace=false) {Fill(StreamKind, StreamPos, Parameter, Ztring::ToZtring(Value, Radix).MakeUpperCase(), Replace);}
    inline void Fill (stream_t StreamKind, size_t StreamPos, size_t Parameter, int8s          Value, int8u Radix=10, bool Replace=false) {Fill(StreamKind, StreamPos, Parameter, Ztring::ToZtring(Value, Radix).MakeUpperCase(), Replace);}
    inline void Fill (stream_t StreamKind, size_t StreamPos, size_t Parameter, int16u         Value, int8u Radix=10, bool Replace=false) {Fill(StreamKind, StreamPos, Parameter, Ztring::ToZtring(Value, Radix).MakeUpperCase(), Replace);}
    inline void Fill (stream_t StreamKind, size_t StreamPos, size_t Parameter, int16s         Value, int8u Radix=10, bool Replace=false) {Fill(StreamKind, StreamPos, Parameter, Ztring::ToZtring(Value, Radix).MakeUpperCase(), Replace);}
    inline void Fill (stream_t StreamKind, size_t StreamPos, size_t Parameter, int32u         Value, int8u Radix=10, bool Replace=false) {Fill(StreamKind, StreamPos, Parameter, Ztring::ToZtring(Value, Radix).MakeUpperCase(), Replace);}
    inline void Fill (stream_t StreamKind, size_t StreamPos, size_t Parameter, int32s         Value, int8u Radix=10, bool Replace=false) {Fill(StreamKind, StreamPos, Parameter, Ztring::ToZtring(Value, Radix).MakeUpperCase(), Replace);}
    inline void Fill (stream_t StreamKind, size_t StreamPos, size_t Parameter, int64u         Value, int8u Radix=10, bool Replace=false) {Fill(StreamKind, StreamPos, Parameter, Ztring::ToZtring(Value, Radix).MakeUpperCase(), Replace);}
    inline void Fill (stream_t StreamKind, size_t StreamPos, size_t Parameter, int64s         Value, int8u Radix=10, bool Replace=false) {Fill(StreamKind, StreamPos, Parameter, Ztring::ToZtring(Value, Radix).MakeUpperCase(), Replace);}
           void Fill (stream_t StreamKind, size_t StreamPos, size_t Parameter, float64        Value, int8u AfterComma=3, bool Replace=false);
    #ifdef SIZE_T_IS_LONG
    inline void Fill (stream_t StreamKind, size_t StreamPos, size_t Parameter, size_t         Value, int8u Radix=10, bool Replace=false) {Fill(StreamKind, StreamPos, Parameter, Ztring::ToZtring(Value, Radix).MakeUpperCase(), Replace);}
    #endif //SIZE_T_IS_LONG
    //Fill with datas
    void Fill_Dup (stream_t StreamKind, size_t StreamPos, const char* Parameter, const Ztring  &Value, bool Replace=false);
    void Fill_Measure (stream_t StreamKind, size_t StreamPos, const char* Parameter, const Ztring  &Value, const Ztring& Measure, bool Replace=false);
    inline void Fill_Measure(stream_t StreamKind, size_t StreamPos, const char* Parameter, const string&  Value, const Ztring& Measure, bool Utf8=true, bool Replace=false) {Fill_Measure(StreamKind, StreamPos, Parameter, Ztring().From_UTF8(Value.c_str(), Value.size()), Measure, Replace);}
    inline void Fill_Measure(stream_t StreamKind, size_t StreamPos, const char* Parameter, int            Value, const Ztring& Measure, int8u Radix=10, bool Replace=false) {Fill_Measure(StreamKind, StreamPos, Parameter, Ztring::ToZtring(Value, Radix).MakeUpperCase(), Measure, Replace);}
    inline void Fill_Measure(stream_t StreamKind, size_t StreamPos, const char* Parameter, float64        Value, const Ztring& Measure, int8u AfterComma=3, bool Replace=false) {Fill_Measure(StreamKind, StreamPos, Parameter, Ztring::ToZtring(Value, AfterComma), Measure, Replace);}
    void Fill (stream_t StreamKind, size_t StreamPos, const char* Parameter, const Ztring  &Value, bool Replace=false);
    void Fill (stream_t StreamKind, size_t StreamPos, const char* Parameter, ZtringList &Value, ZtringList& Id, bool Replace=false);
    inline void Fill (stream_t StreamKind, size_t StreamPos, const char* Parameter, const std::string &Value, bool Utf8=true, bool Replace=false) {if (Utf8) Fill(StreamKind, StreamPos, Parameter, Ztring().From_UTF8(Value.c_str(), Value.size()), Replace); else Fill(StreamKind, StreamPos, Parameter, Ztring().From_Local(Value.c_str(), Value.size()), Replace);}
    inline void Fill (stream_t StreamKind, size_t StreamPos, const char* Parameter, const char*    Value, size_t Value_Size=Unlimited, bool Utf8=true, bool Replace=false) {if (Utf8) Fill(StreamKind, StreamPos, Parameter, Ztring().From_UTF8(Value, Value_Size), Replace); else Fill(StreamKind, StreamPos, Parameter, Ztring().From_Local(Value, Value_Size), Replace);}
    inline void Fill (stream_t StreamKind, size_t StreamPos, const char* Parameter, const wchar_t* Value, size_t Value_Size=Unlimited, bool Replace=false) {Fill(StreamKind, StreamPos, Parameter, Ztring().From_Unicode(Value, Value_Size), Replace);}
    inline void Fill (stream_t StreamKind, size_t StreamPos, const char* Parameter, int8u          Value, int8u Radix=10, bool Replace=false) {Fill(StreamKind, StreamPos, Parameter, Ztring::ToZtring(Value, Radix).MakeUpperCase(), Replace);}
    inline void Fill (stream_t StreamKind, size_t StreamPos, const char* Parameter, int8s          Value, int8u Radix=10, bool Replace=false) {Fill(StreamKind, StreamPos, Parameter, Ztring::ToZtring(Value, Radix).MakeUpperCase(), Replace);}
    inline void Fill (stream_t StreamKind, size_t StreamPos, const char* Parameter, int16u         Value, int8u Radix=10, bool Replace=false) {Fill(StreamKind, StreamPos, Parameter, Ztring::ToZtring(Value, Radix).MakeUpperCase(), Replace);}
    inline void Fill (stream_t StreamKind, size_t StreamPos, const char* Parameter, int16s         Value, int8u Radix=10, bool Replace=false) {Fill(StreamKind, StreamPos, Parameter, Ztring::ToZtring(Value, Radix).MakeUpperCase(), Replace);}
    inline void Fill (stream_t StreamKind, size_t StreamPos, const char* Parameter, int32u         Value, int8u Radix=10, bool Replace=false) {Fill(StreamKind, StreamPos, Parameter, Ztring::ToZtring(Value, Radix).MakeUpperCase(), Replace);}
    inline void Fill (stream_t StreamKind, size_t StreamPos, const char* Parameter, int32s         Value, int8u Radix=10, bool Replace=false) {Fill(StreamKind, StreamPos, Parameter, Ztring::ToZtring(Value, Radix).MakeUpperCase(), Replace);}
    inline void Fill (stream_t StreamKind, size_t StreamPos, const char* Parameter, int64u         Value, int8u Radix=10, bool Replace=false) {Fill(StreamKind, StreamPos, Parameter, Ztring::ToZtring(Value, Radix).MakeUpperCase(), Replace);}
    inline void Fill (stream_t StreamKind, size_t StreamPos, const char* Parameter, int64s         Value, int8u Radix=10, bool Replace=false) {Fill(StreamKind, StreamPos, Parameter, Ztring::ToZtring(Value, Radix).MakeUpperCase(), Replace);}
    inline void Fill (stream_t StreamKind, size_t StreamPos, const char* Parameter, float32        Value, int8u AfterComma=3, bool Replace=false) {Fill(StreamKind, StreamPos, Parameter, Ztring::ToZtring(Value, AfterComma), Replace);}
    inline void Fill (stream_t StreamKind, size_t StreamPos, const char* Parameter, float64        Value, int8u AfterComma=3, bool Replace=false) {Fill(StreamKind, StreamPos, Parameter, Ztring::ToZtring(Value, AfterComma), Replace);}
    inline void Fill (stream_t StreamKind, size_t StreamPos, const char* Parameter, float80        Value, int8u AfterComma=3, bool Replace=false) {Fill(StreamKind, StreamPos, Parameter, Ztring::ToZtring(Value, AfterComma), Replace);}
    #ifdef SIZE_T_IS_LONG
    inline void Fill (stream_t StreamKind, size_t StreamPos, const char* Parameter, size_t         Value, int8u Radix=10, bool Replace=false) {Fill(StreamKind, StreamPos, Parameter, Ztring::ToZtring(Value, Radix).MakeUpperCase(), Replace);}
    #endif //SIZE_T_IS_LONG
    struct fill_temp_item
    {
        Ztring Parameter;
        Ztring Value;
    };
    void Fill_SetOptions(stream_t StreamKind, size_t StreamPos, const char* Parameter, const char* Options);
    vector<fill_temp_item> Fill_Temp[Stream_Max+1]; // +1 because Fill_Temp[Stream_Max] is used when StreamKind is unknown
    map<string, string> Fill_Temp_Options[Stream_Max+1]; // +1 because Fill_Temp[Stream_Max] is used when StreamKind is unknown
    void Fill_Flush ();
    static size_t Fill_Parameter(stream_t StreamKind, generic StreamPos);

    const Ztring &Retrieve_Const (stream_t StreamKind, size_t StreamPos, size_t Parameter, info_t KindOfInfo=Info_Text);
    Ztring Retrieve (stream_t StreamKind, size_t StreamPos, size_t Parameter, info_t KindOfInfo=Info_Text);
    const Ztring &Retrieve_Const (stream_t StreamKind, size_t StreamPos, const char* Parameter, info_t KindOfInfo=Info_Text);
    Ztring Retrieve (stream_t StreamKind, size_t StreamPos, const char* Parameter, info_t KindOfInfo=Info_Text);

    void Clear (stream_t StreamKind, size_t StreamPos, size_t Parameter);
    void Clear (stream_t StreamKind, size_t StreamPos, const char* Parameter);
    void Clear (stream_t StreamKind, size_t StreamPos);
    void Clear (stream_t StreamKind);
    inline void Clear () {File__Base::Clear();}

    //***************************************************************************
    // Filling
    //***************************************************************************

    //Actions
    void Accept        (const char* ParserName=NULL);
    void Accept        (File__Analyze* Parser);
    void Reject        (const char* ParserName=NULL);
    void Reject        (File__Analyze* Parser);
    void Update        (const char* ParserName=NULL);
    void Update        (File__Analyze* Parser);
    void Fill          (const char* ParserName=NULL);
    void Fill          (File__Analyze* Parser);
    void Finish        (const char* ParserName=NULL);
    void Finish        (File__Analyze* Parser);
    void ForceFinish   (const char* ParserName=NULL);
    void ForceFinish   (File__Analyze* Parser);
    void GoTo          (int64u GoTo, const char* ParserName=NULL);
    void GoToFromEnd   (int64u GoToFromEnd, const char* ParserName=NULL);
    int64u Element_Code_Get (size_t Level);
    int64u Element_TotalSize_Get (size_t LevelLess=0);
    bool Element_IsComplete_Get ();
    void Element_ThisIsAList ();
    void Element_WaitForMoreData ();
    void Element_DoNotTrust (const char* Reason);
    void Element_DoNotShow ();
    void Element_Show ();
    void Element_Set_Remove_Children_IfNoErrors ();
    void Element_Remove_Children_IfNoErrors ();
    void Element_Children_IfNoErrors ();
    void Element_DoNotShow_Children ();
    void Element_Show_Children ();
    bool Element_Show_Get ();
    void Element_Show_Add (File__Analyze* node);

    //Status
    bool Element_IsOK ();
    bool Element_IsNotFinished ();
    bool Element_IsWaitingForMoreData ();

    //***************************************************************************
    // Merging
    //***************************************************************************

    //Utils
public :
    size_t Merge(MediaInfo_Internal &ToAdd, bool Erase=true); //Merge 2 File_Base
    size_t Merge(MediaInfo_Internal &ToAdd, stream_t StreamKind, size_t StreamPos_From, size_t StreamPos_To, bool Erase=true); //Merge 2 streams
    size_t Merge(File__Analyze &ToAdd, bool Erase=true); //Merge 2 File_Base
    size_t Merge(File__Analyze &ToAdd, stream_t StreamKind, size_t StreamPos_From, size_t StreamPos_To, bool Erase=true); //Merge 2 streams

    void CodecID_Fill           (const Ztring &Value, stream_t StreamKind, size_t StreamPos, infocodecid_format_t Format, stream_t StreamKind_CodecID=Stream_Max);
    void PixelAspectRatio_Fill  (const Ztring &Value, stream_t StreamKind, size_t StreamPos, size_t Parameter_Width, size_t Parameter_Height, size_t Parameter_PixelAspectRatio, size_t Parameter_DisplayAspectRatio);
    void DisplayAspectRatio_Fill(const Ztring &Value, stream_t StreamKind, size_t StreamPos, size_t Parameter_Width, size_t Parameter_Height, size_t Parameter_PixelAspectRatio, size_t Parameter_DisplayAspectRatio);
    #if MEDIAINFO_EVENTS
    static stream_t Streamkind_Get(int8u* ParserIDs, size_t StreamIDs_Size) {if ((ParserIDs[StreamIDs_Size-1]&0xF0)==0x80) return Stream_Video; if ((ParserIDs[StreamIDs_Size-1]&0xF0)==0xA0) return Stream_Audio; return Stream_Max;}
    stream_t Streamkind_Get() {return Streamkind_Get(ParserIDs, StreamIDs_Size);}
    #endif //MEDIAINFO_EVENTS

    //***************************************************************************
    // Finalize
    //***************************************************************************

    //End
    void Streams_Finish_Global();

protected :
    void Streams_Finish_StreamOnly();
    void Streams_Finish_StreamOnly(stream_t StreamKid, size_t StreamPos);
    void Streams_Finish_StreamOnly_General(size_t StreamPos);
    void Streams_Finish_StreamOnly_Video(size_t StreamPos);
    void Streams_Finish_StreamOnly_Audio(size_t StreamPos);
    void Streams_Finish_StreamOnly_Text(size_t StreamPos);
    void Streams_Finish_StreamOnly_Other(size_t StreamPos);
    void Streams_Finish_StreamOnly_Image(size_t StreamPos);
    void Streams_Finish_StreamOnly_Menu(size_t StreamPos);
    void Streams_Finish_InterStreams();
    void Streams_Finish_Cosmetic();
    void Streams_Finish_Cosmetic(stream_t StreamKid, size_t StreamPos);
    void Streams_Finish_Cosmetic_General(size_t StreamPos);
    void Streams_Finish_Cosmetic_Video(size_t StreamPos);
    void Streams_Finish_Cosmetic_Audio(size_t StreamPos);
    void Streams_Finish_Cosmetic_Text(size_t StreamPos);
    void Streams_Finish_Cosmetic_Chapters(size_t StreamPos);
    void Streams_Finish_Cosmetic_Image(size_t StreamPos);
    void Streams_Finish_Cosmetic_Menu(size_t StreamPos);
    void Streams_Finish_HumanReadable();
    void Streams_Finish_HumanReadable_PerStream(stream_t StreamKind, size_t StreamPos, size_t Parameter);

    void Tags ();
    float64 Video_FrameRate_Rounded (float64 Value);
    void Video_FrameRate_Rounding (stream_t StreamKind, size_t Pos, size_t Parameter);
    void Video_BitRate_Rounding (size_t Pos, video Parameter);
    void Audio_BitRate_Rounding (size_t Pos, audio Parameter);

    //Utils - Finalize
    void Duration_Duration123   (stream_t StreamKind, size_t StreamPos, size_t Parameter);
    void FileSize_FileSize123   (stream_t StreamKind, size_t StreamPos, size_t Parameter);
    void Kilo_Kilo123           (stream_t StreamKind, size_t StreamPos, size_t Parameter);
    void Value_Value123         (stream_t StreamKind, size_t StreamPos, size_t Parameter);
    void YesNo_YesNo            (stream_t StreamKind, size_t StreamPos, size_t Parameter);

    //***************************************************************************
    //
    //***************************************************************************

protected :
    //Save for speed improvement
    float                           Config_Trace_Level;
    std::bitset<32>                 Config_Trace_Layers;
    MediaInfo_Config::trace_Format  Config_Trace_Format;
    int8u                           Config_Demux;
    Ztring                          Config_LineSeparator;
    enum stream_source
    {
        IsContainer,
        IsStream,
        IsContainerExtra,
        StreamSource_Max,
    };
    stream_source                   StreamSource;

    //Configuration
    bool DataMustAlwaysBeComplete;  //Data must always be complete, else wait for more data
    bool MustUseAlternativeParser;  //Must use the second parser (example: for Data part)

    //Synchro
    bool MustParseTheHeaderFile;    //There is an header part, must parse it
    size_t Trusted;
    size_t Trusted_Multiplier;

    //Elements
    size_t Element_Level;           //Current level
    bool   Element_WantNextLevel;   //Want to go to the next leavel instead of the same level

    //Element
    int64u Element_Code;            //Code filled in the file, copy of Element[Element_Level].Code
    int64u Element_Offset;          //Position in the Element (without header)
    int64u Element_Size;            //Size of the Element (without header)

private :
    //***************************************************************************
    // Buffer
    //***************************************************************************

    void Buffer_Clear(); //Clear the buffer
protected :
    //Buffer
    bool Open_Buffer_Continue_Loop();
    const int8u* Buffer;
public : //TO CHANGE
    size_t Buffer_Size;
    int64u Buffer_TotalBytes;
    int64u Buffer_TotalBytes_FirstSynched;
    int64u Buffer_TotalBytes_LastSynched;
    int64u Buffer_PaddingBytes;
    int64u Buffer_JunkBytes;
    float64 Stream_BitRateFromContainer;
protected :
    int8u* Buffer_Temp;
    size_t Buffer_Temp_Size;
    size_t Buffer_Temp_Size_Max;
    size_t Buffer_Offset; //Temporary usage in this parser
    size_t Buffer_Offset_Temp; //Temporary usage in this parser
    size_t Buffer_MinimumSize;
    size_t Buffer_MaximumSize;
    int64u Buffer_TotalBytes_FirstSynched_Max;
    int64u Buffer_TotalBytes_Fill_Max;
    friend class File__Tags_Helper;
    friend class File_Usac;
    friend class File_Mk;
    friend class File_Riff;
    friend class File_Mpeg4;
    friend class File_Hevc;

    //***************************************************************************
    // Helpers
    //***************************************************************************

    bool FileHeader_Begin_0x000001();
    bool FileHeader_Begin_XML(tinyxml2::XMLDocument &Document);
    bool Synchronize_0x000001();
public:
    #if defined(MEDIAINFO_FILE_YES)
    void TestContinuousFileNames(size_t CountOfFiles=24, Ztring FileExtension=Ztring(), bool SkipComputeDelay=false);
    void TestDirectory();
    #else //defined(MEDIAINFO_FILE_YES)
    void TestContinuousFileNames(size_t =24, Ztring =Ztring(), bool =false) {}
    void TestDirectory() {}
    #endif //defined(MEDIAINFO_FILE_YES)
    #if MEDIAINFO_FIXITY
    bool FixFile(int64u FileOffsetForWriting, const int8u* ToWrite, const size_t ToWrite_Size);
    #endif// MEDIAINFO_FIXITY

private :

    //***************************************************************************
    // Elements
    //***************************************************************************

    //Data
    size_t Data_Level;              //Current level for Data ("Top level")

    //Element
public: //TO CHANGE
    BitStream_Fast* BS;             //For conversion from bytes to bitstream
    BitStream*      BT;             //For conversion from bytes to bitstream
    int64u          BS_Size;
public : //TO CHANGE
    int64u Header_Size;             //Size of the header of the current element
    Ztring Details_Get(size_t Level=0) { std::string str; if (Element[Level].TraceNode.Print(Config_Trace_Format, str, Config_LineSeparator.To_UTF8(), File_Size) < 0) return Ztring(); return Ztring().From_UTF8(str);}
    void   Details_Clear();
protected :
    bool Trace_DoNotSave;
    bool Trace_Activated;
    std::bitset<32> Trace_Layers;
    void Trace_Layers_Update (size_t Layer=(size_t)-1);
private :
#if MEDIAINFO_TRACE
    void Trace_Details_Handling(File__Analyze* Sub);
#endif // MEDIAINFO_TRACE
    //Elements
    size_t Element_Level_Base;      //From other parsers
    std::vector<element_details> Element;

    //NextCode
    std::map<int64u, bool> NextCode;

    //BookMarks
    size_t              BookMark_Element_Level;
    int64u              BookMark_GoTo;
    std::vector<int64u> BookMark_Code;
    std::vector<int64u> BookMark_Next;

public :
    void BookMark_Set(size_t Element_Level_ToGet=(size_t)-1);
    void BookMark_Get();
    virtual bool BookMark_Needed()                                              {return false;};

    //Temp
    std::bitset<32> Status;
    enum status
    {
        IsAccepted,
        IsFilled,
        IsUpdated,
        IsFinished,
        Reserved_04,
        Reserved_05,
        Reserved_06,
        Reserved_07,
        Reserved_08,
        Reserved_09,
        Reserved_10,
        Reserved_11,
        Reserved_12,
        Reserved_13,
        Reserved_14,
        Reserved_15,
        User_16,
        User_17,
        User_18,
        User_19,
        User_20,
        User_21,
        User_22,
        User_23,
        User_24,
        User_25,
        User_26,
        User_27,
        User_28,
        User_29,
        User_30,
        User_31,
    };
    bool ShouldContinueParsing;

    //Configuration
    bool MustSynchronize;
    bool CA_system_ID_MustSkipSlices;
    bool FillAllMergedStreams;

    //Demux
    enum contenttype
    {
        ContentType_MainStream,
        ContentType_SubStream,
        ContentType_Header,
        ContentType_Synchro
    };
    #if MEDIAINFO_DEMUX
        void Demux (const int8u* Buffer, size_t Buffer_Size, contenttype ContentType, const int8u* OriginalBuffer=NULL, size_t OriginalBuffer_Size=0);
        virtual bool Demux_UnpacketizeContainer_Test() {return true;}
        bool Demux_UnpacketizeContainer_Test_OneFramePerFile();
        void Demux_UnpacketizeContainer_Demux(bool random_access=true);
        void Demux_UnpacketizeContainer_Demux_Clear();
        bool Demux_EventWasSent_Accept_Specific;
    #else //MEDIAINFO_DEMUX
        #define Demux(_A, _B, _C)
    #endif //MEDIAINFO_DEMUX
    #if MEDIAINFO_DECODE
        void Decoded(const int8u* Buffer, size_t Buffer_Size);
    #endif //MEDIAINFO_DECODE

    //Events data
    bool    PES_FirstByte_IsAvailable;
    bool    PES_FirstByte_Value;

    int64u  Unsynch_Frame_Count;

    //AES
    #if MEDIAINFO_AES
        AESdecrypt* AES;
        int8u*      AES_IV;
        int8u*      AES_Decrypted;
        size_t      AES_Decrypted_Size;
    #endif //MEDIAINFO_AES

    //Hash
    #if MEDIAINFO_HASH
        HashWrapper*        Hash;
        int64u              Hash_Offset;
        int64u              Hash_ParseUpTo;
    #endif //MEDIAINFO_HASH

    #if MEDIAINFO_CONFORMANCE
        void*               Conformance_Data;
        void                Fill_Conformance(const char* Field, const char* Value, uint8_t Flags = {}, conformance_type Level = Conformance_Error, stream_t StreamKind = Stream_General, size_t StreamPos = 0);
        void                Fill_Conformance(const char* Field, const string& Value, uint8_t Flags = {}, conformance_type Level = Conformance_Error) { Fill_Conformance(Field, Value.c_str(), Flags, Level); }
        void                Clear_Conformance();
        void                Merge_Conformance(bool FromConfig = false);
        void                Streams_Finish_Conformance();
        virtual string      CreateElementName();
        void                IsTruncated(int64u ExpectedSize = (int64u)-1, bool MoreThan = false, const char* Prefix = nullptr);
        void                RanOutOfData(const char* Prefix = nullptr);
        void                SynchLost(const char* Prefix = nullptr);
    #else //MEDIAINFO_CONFORMANCE
        void                Fill_Conformance(const char* Field, const char* Value, uint8_t Flags = {}, conformance_type Level = Conformance_Error, stream_t StreamKind = Stream_General, size_t StreamPos = 0) {}
        void                Fill_Conformance(const char* Field, const string& Value, uint8_t Flags = {}, conformance_type Level = Conformance_Error) { Fill_Conformance(Field, Value.c_str(), Flags, Level); }
        void                Clear_Conformance() {}
        void                Merge_Conformance(bool FromConfig = false) {}
        void                Streams_Finish_Conformance() {}
        string              CreateElementName() { return {}; }
        void                IsTruncated(int64u ExpectedSize = (int64u)-1, bool MoreThan = false, const char* = nullptr) {}
        void                RanOutOfData(const char* = nullptr) { Trusted_IsNot(); }
        void                SynchLost(const char* = nullptr) { Trusted_IsNot(); }
    #endif //MEDIAINFO_CONFORMANCE

    #if MEDIAINFO_SEEK
    private:
        bool Seek_Duration_Detected;
    #endif //MEDIAINFO_SEEK

    #if MEDIAINFO_IBIUSAGE
    public:
        int64u  Ibi_SynchronizationOffset_Current;
        int64u  Ibi_SynchronizationOffset_BeginOfFrame;
    #endif //MEDIAINFO_IBIUSAGE
    #if MEDIAINFO_IBIUSAGE
    public:
        bool    Config_Ibi_Create;
        ibi     Ibi; //If Main only
        ibi::stream* IbiStream; //If sub only
        size_t  Ibi_Read_Buffer_Seek        (size_t Method, int64u Value, int64u ID);
        void    Ibi_Read_Buffer_Unsynched   ();
        void    Ibi_Stream_Finish           ();
        void    Ibi_Stream_Finish           (int64u Numerator, int64u Denominator); //Partial
        void    Ibi_Add                     ();
    #else //MEDIAINFO_IBIUSAGE
        size_t  Ibi_Read_Buffer_Seek        (size_t, int64u, int64u)            {return (size_t)-1;}
        void    Ibi_Read_Buffer_Unsynched   ()                                  {}
        void    Ibi_Stream_Finish           ()                                  {}
        void    Ibi_Stream_Finish           (int64u, int64u)                    {}
        void    Ibi_Add                     ()                                  {}
    #endif //MEDIAINFO_IBIUSAGE
};

//Helpers
#define DETAILS_INFO(_DATA) _DATA

#endif //MEDIAINFO_TRACE

} //NameSpace



/*
#define BS_END() \
    { \
        BS.Byte_Align(); \
        int32u BS_Value=0x00; \
        while(BS.Remain()>0 && BS_Value==0x00) \
            BS_Value=BS.Get(8); \
        if (BS_Value!=0x00) \
            INTEGRITY_SIZE(BS.Offset_Get()-1, BS.Offset_Get()) \
        else \
            INTEGRITY_SIZE(BS.Offset_Get(), BS.Offset_Get()) \
    } \

#define BS_END_FF() \
    { \
        BS.Byte_Align(); \
        int32u BS_Value=0xFF; \
        while(BS.Remain()>0 && BS_Value==0x00) \
            BS_Value=BS.Get(8); \
        if (BS_Value!=0xFF) \
            INTEGRITY_SIZE(BS.Offset_Get()-1, BS.Offset_Get()) \
        else \
            INTEGRITY_SIZE(BS.Offset_Get(), BS.Offset_Get()) \
    } \

#define BS_END_CANBEMORE() \
    { \
    } \
*/


    //Begin
    #define FILLING_BEGIN() \
        if (Element_IsOK()) \
        {

    #define FILLING_BEGIN_PRECISE() \
        if (Element_Offset!=Element_Size) \
        { \
            Trusted_IsNot("Size is wrong"); \
        } \
        if (Element_IsOK()) \
        {

    //Else
    #define FILLING_ELSE() \
        } \
        else \
        { \

    //End
    #define FILLING_END() \
        }

#define ATOM_BEGIN \
    if (Level!=Element_Level) \
    { \
        Level++; \
        switch (Element_Code_Get(Level)) \
        { \

#define ATOM(_ATOM) \
            case Elements::_ATOM : \
                    if (Level==Element_Level) \
                    { \
                        if (Element_IsComplete_Get()) \
                            _ATOM(); \
                        else \
                        { \
                            Element_WaitForMoreData(); \
                            return; \
                        } \
                    } \
                    break; \

#define ATOM_PARTIAL(_ATOM) \
            case Elements::_ATOM : \
                    if (Level==Element_Level) \
                        _ATOM(); \
                    break; \

#define ATOM_DEFAULT(_ATOM) \
            default : \
                    if (Level==Element_Level) \
                    { \
                        if (Element_IsComplete_Get()) \
                            _ATOM(); \
                        else \
                        { \
                            Element_WaitForMoreData(); \
                            return; \
                        } \
                    } \
                    break; \

#define ATOM_END \
            default : \
            Skip_XX(Element_TotalSize_Get(), "Unknown"); \
        } \
    } \
    break; \

#define LIST(_ATOM) \
    case Elements::_ATOM : \
            if (Level==Element_Level) \
            { \
                Element_ThisIsAList(); \
                _ATOM(); \
            } \

#define LIST_COMPLETE(_ATOM) \
    case Elements::_ATOM : \
            if (Level==Element_Level) \
            { \
                if (Element_IsComplete_Get()) \
                { \
                    Element_ThisIsAList(); \
                    _ATOM(); \
                } \
                else \
                { \
                    Element_WaitForMoreData(); \
                    return; \
                } \
            } \

#define LIST_DEFAULT(_ATOM) \
            default : \
            if (Level==Element_Level) \
            { \
                Element_ThisIsAList(); \
                _ATOM(); \
            } \

#define LIST_DEFAULT_COMPLETE(_ATOM) \
            default : \
            if (Level==Element_Level) \
            { \
                if (Element_IsComplete_Get()) \
                { \
                    Element_ThisIsAList(); \
                    _ATOM(); \
                } \
                else \
                { \
                    Element_WaitForMoreData(); \
                    return; \
                } \
            } \

#define ATOM_END_DEFAULT \
        } \
    } \
    break; \

#define ATOM_DEFAULT_ALONE(_ATOM) \
    if (Level!=Element_Level) \
    { \
        Level++; \
        if (Level==Element_Level) \
        { \
            if (Element_IsComplete_Get()) \
                _ATOM(); \
            else \
            { \
                Element_WaitForMoreData(); \
                return; \
            } \
        } \
    } \
    break; \

#define LIST_DEFAULT_ALONE_BEGIN(_ATOM) \
    if (Level!=Element_Level) \
    { \
        Level++; \
        if (Level==Element_Level) \
        { \
            Element_ThisIsAList(); \
            _ATOM(); \
        } \

#define LIST_DEFAULT_ALONE_END \
    } \
    break; \

#define LIST_SKIP(_ATOM) \
    case Elements::_ATOM : \
            if (Level==Element_Level) \
            { \
                Element_ThisIsAList(); \
                _ATOM(); \
            } \
            break; \


#define DATA_BEGIN \
    size_t Level=0; \
    ATOM_BEGIN \

#define DATA_END \
            default : ; \
                Skip_XX(Element_TotalSize_Get(), "Unknown"); \
        } \
    } \

#define DATA_DEFAULT \
        default : \

#define DATA_END_DEFAULT \
        } \
    } \

#endif

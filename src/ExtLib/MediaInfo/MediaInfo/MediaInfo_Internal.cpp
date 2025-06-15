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
#include "MediaInfo/MediaInfo_Internal.h"
#include "MediaInfo/MediaInfo_Config.h"
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/File__MultipleParsing.h"
#if defined(MEDIAINFO_FILE_YES)
#include "ZenLib/File.h"
#endif //defined(MEDIAINFO_DIRECTORY_YES)
#if defined(MEDIAINFO_DIRECTORY_YES)
#include "ZenLib/Dir.h"
#endif //defined(MEDIAINFO_DIRECTORY_YES)
#include "ZenLib/FileName.h"
#if defined(MEDIAINFO_DIRECTORY_YES)
    #include "MediaInfo/Reader/Reader_Directory.h"
#endif
#if defined(MEDIAINFO_FILE_YES)
    #include "MediaInfo/Reader/Reader_File.h"
#endif
#if defined(MEDIAINFO_LIBCURL_YES)
    #include "MediaInfo/Reader/Reader_libcurl.h"
#endif
#if defined(MEDIAINFO_LIBMMS_YES)
    #include "MediaInfo/Reader/Reader_libmms.h"
#endif
#if defined(MEDIAINFO_IBI_YES)
    #include "MediaInfo/Multiple/File_Ibi.h"
#endif
#include "MediaInfo/Multiple/File_Dxw.h"
#ifdef MEDIAINFO_COMPRESS
    #include "ThirdParty/base64/base64.h"
    #include "zlib.h"
#endif //MEDIAINFO_COMPRESS
#include <cmath>
#ifdef MEDIAINFO_DEBUG_WARNING_GET
    #include <iostream>
#endif //MEDIAINFO_DEBUG_WARNING_GET
#ifdef MEDIAINFO_DEBUG_BUFFER
    #include "ZenLib/FileName.h"
    #include <cstring>
#endif //MEDIAINFO_DEBUG_BUFFER
#if MEDIAINFO_ADVANCED
    #include <iostream>
    #if defined(WINDOWS) && !defined(WINDOWS_UWP) && !defined(__BORLANDC__)
        #include <fcntl.h>
        #include <io.h>
    #endif //defined(WINDOWS) && !defined(WINDOWS_UWP) && !defined(__BORLANDC__)
#endif //MEDIAINFO_ADVANCED
#if MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)
    #include <limits>
    #ifdef WINDOWS
        #ifndef WIN32_LEAN_AND_MEAN
            #define WIN32_LEAN_AND_MEAN
        #endif
        #ifndef NOMINMAX
            #define NOMINMAX
        #endif
        #include <windows.h>
        #undef Yield
    #else
        #include <unistd.h>
        #include <signal.h>
        #if defined(_POSIX_PRIORITY_SCHEDULING) // Note: unistd.h must be included first
            #include <sched.h>
        #endif //_POSIX_PRIORITY_SCHEDULING
    #endif
    #include <ctime>
#endif
using namespace ZenLib;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
//To clarify the code
namespace MediaInfo_Debug_MediaInfo_Internal
{

#if defined (MEDIAINFO_DEBUG_CONFIG) || defined (MEDIAINFO_DEBUG_BUFFER) || defined (MEDIAINFO_DEBUG_OUTPUT)
    #ifdef WINDOWS
        const Char* MediaInfo_Debug_Name=__T("MediaInfo_Debug");
    #else
        const Char* MediaInfo_Debug_Name=__T("/tmp/MediaInfo_Debug");
    #endif
#endif

#ifdef MEDIAINFO_DEBUG_CONFIG
    #define MEDIAINFO_DEBUG_CONFIG_TEXT(_TOAPPEND) \
        { \
            Ztring Debug; \
            _TOAPPEND; \
            Debug+=__T("\r\n"); \
            if (!Debug_Config.Opened_Get()) \
            { \
                if (Config.File_Names.empty()) \
                    Debug_Config.Create(Ztring(MediaInfo_Debug_Name)+__T(".")+Ztring::ToZtring((size_t)this, 16)+__T(".Config.txt")); \
                else \
                { \
                    Ztring File_Temp; \
                    if (Config.File_Names[0].rfind(__T('\\'))!=string::npos) \
                        File_Temp=Config.File_Names[0].substr(Config.File_Names[0].rfind(__T('\\'))+1, string::npos); \
                    else if (Config.File_Names[0].rfind(__T('/'))!=string::npos) \
                        File_Temp=Config.File_Names[0].substr(Config.File_Names[0].rfind(__T('/'))+1, string::npos); \
                    else \
                        File_Temp=Config.File_Names[0]; \
                    Debug_Config.Create(Ztring(MediaInfo_Debug_Name)+__T(".")+Ztring::ToZtring((size_t)this, 16)+__T(".")+File_Temp+__T(".Config.txt")); \
                } \
            } \
            Debug_Config.Write(Debug); \
        }
#else // MEDIAINFO_DEBUG_CONFIG
    #define MEDIAINFO_DEBUG_CONFIG_TEXT(_TOAPPEND)
#endif // MEDIAINFO_DEBUG_CONFIG

#ifdef MEDIAINFO_DEBUG_CONFIG
    #define EXECUTE_SIZE_T(_METHOD,_DEBUGB) \
        { \
            size_t ToReturn=_METHOD; \
            MEDIAINFO_DEBUG_CONFIG_TEXT(_DEBUGB) \
            return ToReturn; \
        }
#else //MEDIAINFO_DEBUG_CONFIG
    #define EXECUTE_SIZE_T(_METHOD, _DEBUGB) \
        return _METHOD;
#endif //MEDIAINFO_DEBUG_CONFIG

#ifdef MEDIAINFO_DEBUG_CONFIG
    #define EXECUTE_INT64U(_METHOD,_DEBUGB) \
        { \
            int64u ToReturn=_METHOD; \
            MEDIAINFO_DEBUG_CONFIG_TEXT(_DEBUGB) \
            return ToReturn; \
        }
#else //MEDIAINFO_DEBUG_CONFIG
    #define EXECUTE_INT64U(_METHOD, _DEBUGB) \
        return _METHOD;
#endif //MEDIAINFO_DEBUG_CONFIG

#ifdef MEDIAINFO_DEBUG_CONFIG
    #define EXECUTE_STRING(_METHOD,_DEBUGB) \
        { \
            Ztring ToReturn=_METHOD; \
            MEDIAINFO_DEBUG_CONFIG_TEXT(_DEBUGB) \
            return ToReturn; \
        }
#else //MEDIAINFO_DEBUG_CONFIG
    #define EXECUTE_STRING(_METHOD,_DEBUGB) \
        return _METHOD;
#endif //MEDIAINFO_DEBUG_CONFIG

#ifdef MEDIAINFO_DEBUG_BUFFER
    #define MEDIAINFO_DEBUG_BUFFER_SAVE(_BUFFER, _SIZE) \
        { \
            if (!Debug_Buffer_Stream.Opened_Get()) \
            { \
                if (Config.File_Names.empty()) \
                    Debug_Buffer_Stream.Create(Ztring(MediaInfo_Debug_Name)+__T(".")+Ztring::ToZtring((size_t)this, 16)+__T(".Buffer.Stream.0000000000000000")); \
                else \
                { \
                    Ztring File_Temp; \
                    if (Config.File_Names[0].rfind(__T('\\'))!=string::npos) \
                        File_Temp=Config.File_Names[0].substr(Config.File_Names[0].rfind(__T('\\'))+1, string::npos); \
                    else if (Config.File_Names[0].rfind(__T('/'))!=string::npos) \
                        File_Temp=Config.File_Names[0].substr(Config.File_Names[0].rfind(__T('/'))+1, string::npos); \
                    else \
                        File_Temp=Config.File_Names[0]; \
                    Debug_Buffer_Stream.Create(Ztring(MediaInfo_Debug_Name)+__T(".")+Ztring::ToZtring((size_t)this, 16)+__T(".")+File_Temp+__T(".Buffer.Stream.0000000000000000")); \
                } \
                Debug_Buffer_Stream_Order=0; \
                if (Config.File_Names.empty()) \
                    Debug_Buffer_Sizes.Create(Ztring(MediaInfo_Debug_Name)+__T(".")+Ztring::ToZtring((size_t)this, 16)+__T(".Buffer.Sizes.0000000000000000")); \
                else \
                { \
                    Ztring File_Temp; \
                    if (Config.File_Names[0].rfind(__T('\\'))!=string::npos) \
                        File_Temp=Config.File_Names[0].substr(Config.File_Names[0].rfind(__T('\\'))+1, string::npos); \
                    else if (Config.File_Names[0].rfind(__T('/'))!=string::npos) \
                        File_Temp=Config.File_Names[0].substr(Config.File_Names[0].rfind(__T('/'))+1, string::npos); \
                    else \
                        File_Temp=Config.File_Names[0]; \
                    Debug_Buffer_Sizes.Create(Ztring(MediaInfo_Debug_Name)+__T(".")+Ztring::ToZtring((size_t)this, 16)+__T(".")+File_Temp+__T(".Buffer.Sizes.0000000000000000")); \
                } \
                Debug_Buffer_Sizes_Count=0; \
            } \
            Debug_Buffer_Stream.Write(_BUFFER, _SIZE); \
            Debug_Buffer_Sizes.Write((int8u*)&_SIZE, sizeof(size_t)); \
            Debug_Buffer_Sizes_Count+=_SIZE; \
            if (Debug_Buffer_Sizes_Count>=MEDIAINFO_DEBUG_BUFFER_SAVE_FileSize) \
            { \
                Debug_Buffer_Stream.Close(); \
                Debug_Buffer_Sizes.Close(); \
                Ztring Before=Ztring::ToZtring(Debug_Buffer_Stream_Order-1); \
                while (Before.size()<16) \
                    Before.insert(0, 1, __T('0')); \
                Ztring Next=Ztring::ToZtring(Debug_Buffer_Stream_Order+1); \
                while (Next.size()<16) \
                    Next.insert(0, 1, __T('0')); \
                Debug_Buffer_Stream.Create(Ztring(MediaInfo_Debug_Name)+__T(".")+Ztring::ToZtring((size_t)this, 16)+__T(".Buffer.Stream.")+Next); \
                Debug_Buffer_Sizes.Create(Ztring(MediaInfo_Debug_Name)+__T(".")+Ztring::ToZtring((size_t)this, 16)+__T(".Buffer.Sizes.")+Next); \
                File::Delete(Ztring(MediaInfo_Debug_Name)+__T(".")+Ztring::ToZtring((size_t)this, 16)+__T(".Buffer.Stream.")+Before); \
                File::Delete(Ztring(MediaInfo_Debug_Name)+__T(".")+Ztring::ToZtring((size_t)this, 16)+__T(".Buffer.Sizes.")+Before); \
                Debug_Buffer_Stream_Order++; \
                Debug_Buffer_Sizes_Count=0; \
            } \
        }
#else // MEDIAINFO_DEBUG_BUFFER
    #define MEDIAINFO_DEBUG_BUFFER_SAVE(_BUFFER, _SIZE)
#endif // MEDIAINFO_DEBUG_BUFFER

#ifdef MEDIAINFO_DEBUG_OUTPUT
    #define MEDIAINFO_DEBUG_OUTPUT_INIT(_VALUE, _DEBUGB) \
        { \
            if (OptionLower==__T("file_duplicate")) \
            { \
                size_t Pos=(size_t)ToReturn2.To_int64u(); \
                if (Pos>=Debug_Output_Pos_Stream.size()) \
                { \
                    Debug_Output_Pos_Stream.resize(Pos+1); \
                    Debug_Output_Pos_Stream[Pos]=new File(); \
                    Debug_Output_Pos_Sizes.resize(Pos+1); \
                    Debug_Output_Pos_Sizes[Pos]=new File(); \
                    Debug_Output_Pos_Pointer.resize(Pos+1); \
                    Debug_Output_Pos_Pointer[Pos]=(void*)Ztring(Value).SubString(__T("memory://"), __T(":")).To_int64u(); \
                } \
            } \
            EXECUTE_STRING(_VALUE, _DEBUGB) \
        }
#else // MEDIAINFO_DEBUG_OUTPUT
    #define MEDIAINFO_DEBUG_OUTPUT_INIT(_VALUE, _DEBUGB) \
        EXECUTE_STRING(_VALUE, _DEBUGB)
#endif // MEDIAINFO_DEBUG_OUTPUT

#ifdef MEDIAINFO_DEBUG_OUTPUT
    #define MEDIAINFO_DEBUG_OUTPUT_VALUE(_VALUE, _METHOD) \
        { \
            size_t ByteCount=Info->Output_Buffer_Get(Value); \
            void* ValueH=(void*)Ztring(Value).SubString(__T("memory://"), __T(":")).To_int64u(); \
            map<void*, File>::iterator F_Stream=Debug_Output_Value_Stream.find(ValueH); \
            if (F_Stream!=Debug_Output_Value_Stream.end()) \
            { \
                map<void*, File>::iterator F_Sizes=Debug_Output_Value_Stream.find(ValueH); \
                if (!F_Stream->second.Opened_Get()) \
                { \
                    F_Stream->second.Create(Ztring(MediaInfo_Debug_Name)+__T(".")+Ztring::ToZtring((size_t)this, 16)+__T(".Output.")+Ztring::ToZtring((size_t)ValueH, 16)+__T(".Stream")); \
                    F_Sizes->second.Create(Ztring(MediaInfo_Debug_Name)+__T(".")+Ztring::ToZtring((size_t)this, 16)+__T(".Output.")+Ztring::ToZtring((size_t)ValueH, 16)+__T(".Sizes")); \
                } \
                F_Stream->second.Write((int8u*)ValueH, ByteCount); \
                F_Sizes->second.Write((int8u*)&ByteCount, sizeof(ByteCount)); \
            } \
            return ByteCount; \
        }
#else // MEDIAINFO_DEBUG_OUTPUT
    #define MEDIAINFO_DEBUG_OUTPUT_VALUE(_VALUE, _METHOD) \
        return _METHOD
#endif // MEDIAINFO_DEBUG_OUTPUT

#ifdef MEDIAINFO_DEBUG_OUTPUT
    #define MEDIAINFO_DEBUG_OUTPUT_POS(_POS, _METHOD) \
        { \
            size_t ByteCount=Info->Output_Buffer_Get(_POS); \
            if (_POS<Debug_Output_Pos_Stream.size()) \
            { \
                if (!Debug_Output_Pos_Stream[_POS]->Opened_Get()) \
                { \
                    Debug_Output_Pos_Stream[_POS]->Create(Ztring(MediaInfo_Debug_Name)+__T(".")+Ztring::ToZtring((size_t)this, 16)+__T(".Output.")+Ztring::ToZtring(Pos, 16)+__T(".Stream")); \
                    Debug_Output_Pos_Sizes[_POS]->Create(Ztring(MediaInfo_Debug_Name)+__T(".")+Ztring::ToZtring((size_t)this, 16)+__T(".Output.")+Ztring::ToZtring(Pos, 16)+__T(".Sizes")); \
                } \
                Debug_Output_Pos_Stream[_POS]->Write((int8u*)Debug_Output_Pos_Pointer[_POS], ByteCount); \
                Debug_Output_Pos_Sizes[_POS]->Write((int8u*)&ByteCount, sizeof(ByteCount)); \
            } \
            return ByteCount; \
        }
#else // MEDIAINFO_DEBUG_OUTPUT
    #define MEDIAINFO_DEBUG_OUTPUT_POS(_VALUE, _METHOD) \
        return _METHOD
#endif // MEDIAINFO_DEBUG_OUTPUT

}
using namespace MediaInfo_Debug_MediaInfo_Internal;

Ztring File__Analyze_Encoded_Library_String(const Ztring &CompanyName, const Ztring &Name, const Ztring &Version, const Ztring &Date, const Ztring &Encoded_Library);

extern const Char* MediaInfo_Version;

//***************************************************************************
// Modifiers - ChannelLayout_2018
//***************************************************************************
static const char* ChannelLayout_2018[][2] =
{
    { "BC", "Cb" },
    { "BL", "Lb" },
    { "BR", "Rb" },
    { "BS", "Rb" },
    { "CI", "Bfc" },
    { "CL", "Ls" },
    { "CR", "Rs" },
    { "Ch", "Tfc" },
    { "Chr", "Tbc" },
    { "Cl", "Ls" },
    { "Cr", "Rs" },
    { "Cs", "Cb" },
    { "Cv", "Tfc" },
    { "Cvr", "Tbc" },
    { "FC", "C" },
    { "FL", "L" },
    { "FLC", "Lc" },
    { "FR", "R" },
    { "FRC", "Rc" },
    { "LC", "Lc" },
    { "LI", "Bfl" },
    { "LS", "Ls" },
    { "Lfh", "Tfl" },
    { "Lh", "Tfl" },
    { "Lhr", "Tbl" },
    { "Lhs", "Tfl" },
    { "Lrh", "Tbl" },
    { "Lrs", "Lb" },
    { "Lsc", "Lscr" },
    { "Lsr", "Lb" },
    { "Ltm", "Tsl" },
    { "Lts", "Tsl" },
    { "Lv", "Tfl" },
    { "Lvh", "Tfl" },
    { "Lvr", "Tbl" },
    { "Lvss", "Tsl" },
    { "Oh", "Tc" },
    { "Rfh", "Tfrr" },
    { "Rh", "Tfr" },
    { "Rhr", "Tbr" },
    { "Rhs", "Tfr" },
    { "RC", "Rc" },
    { "RI", "Bfr" },
    { "RS", "Rs" },
    { "Rls", "Lb" },
    { "Rrh", "Tbr" },
    { "Rrs", "Rb" },
    { "Rsc", "Rscr" },
    { "Rsr", "Rb" },
    { "Rtm", "Tsr" },
    { "Rts", "Tsr" },
    { "Rv", "Tfr" },
    { "Rvh", "Tfr" },
    { "Rvr", "Tbr" },
    { "Rvss", "Tsr" },
    { "S", "Cb" },
    { "SL", "Ls" },
    { "SR", "Rs" },
    { "SW", "LFE" },
    { "TBC", "Tbc" },
    { "TBL", "Tbl" },
    { "TBR", "Tbr" },
    { "TC", "Tc" },
    { "TFC", "Tfc" },
    { "TFL", "Tfl" },
    { "TFR", "Tfr" },
    { "Ts", "Tc" },
    { "Vhc", "Tfc" },
};
static const size_t ChannelLayout_2018_Size=sizeof(ChannelLayout_2018)/sizeof(decltype(*ChannelLayout_2018));
static const char* ChannelLayout_2018_Aac[][2] =
{
    { "Cb", "Bfc" },
    { "Lb", "Bfl" },
    { "Rb", "Bfr" },
    // Found in DTS-UHD, similar to AAC for channels above + theses additions
    { "Ltf", "Tfl" }, // Merged with Lh ("top" vs "high")
    { "Rtf", "Tfr" }, // Merged with Rh ("top" vs "high")
    { "Ltr", "Tbl" }, // Merged with Lhr ("top" vs "high")
    { "Rtr", "Tbr" }, // Merged with Rhr ("top" vs "high")
};
static const size_t ChannelLayout_2018_Aac_Size=sizeof(ChannelLayout_2018_Aac)/sizeof(decltype(*ChannelLayout_2018_Aac));
Ztring ChannelLayout_2018_Rename(const Ztring& Channels, const Ztring& Format)
{
    ZtringList List;
    List.Separator_Set(0, __T(" "));
    List.Write(Channels);
    size_t LfePos[3];
    memset(LfePos, -1, sizeof(LfePos));
    bool IsAac=(Format==__T("AAC") || Format==__T("USAC") || Format==__T("MPEG-H 3D Audio") || Format==__T("DTS-UHD"));
    for (size_t i=0; i<List.size(); i++)
    {
        Ztring& ChannelName=List[i];
        string ChannelNameS=ChannelName.To_UTF8();
        for (size_t j=0; j<ChannelLayout_2018_Size; j++)
            if (!strcmp(ChannelNameS.c_str(), ChannelLayout_2018[j][0]))
                ChannelName.From_UTF8(ChannelLayout_2018[j][1]);
        if (IsAac)
        {
            for (size_t j=0; j< ChannelLayout_2018_Aac_Size; j++)
                if (!strcmp(ChannelNameS.c_str(), ChannelLayout_2018_Aac[j][0]))
                    ChannelName.From_UTF8(ChannelLayout_2018_Aac[j][1]);
            if (ChannelNameS.size()>=3 && ChannelNameS[0]=='L' && ChannelNameS[1]=='F' && ChannelNameS[2]=='E')
            {
                if (LfePos[0] && ChannelNameS.size()==3)
                    LfePos[0]=i;
                if (LfePos[1] && ChannelNameS.size()==4 && ChannelNameS[3]==__T('2'))
                    LfePos[1]=i;
                if (LfePos[2] && ChannelNameS.size()==4 && ChannelNameS[3]==__T('3'))
                    LfePos[2]=i;
            }
        }
    }
    if (LfePos[0]==(size_t)-1 && LfePos[1]!=(size_t)-1 && LfePos[2]!=(size_t)-1)
    {
        List[LfePos[1]].resize(3); // LFE2 --> LFE
        List[LfePos[2]][3]--; // LFE3 --> LFE2
    }
    Ztring ToReturn=List.Read();
    return ToReturn;
}
Ztring ChannelLayout_2018_Rename(stream_t StreamKind, size_t Parameter, ZtringList& Info, const Ztring& StreamFormat, bool &ShouldReturn)
{
    bool ShouldReturn_Save=ShouldReturn;
    ShouldReturn=true;
    switch (StreamKind)
    {
        case Stream_Audio:
            switch (Parameter)
            {
                case Audio_ChannelLayout:
                case Audio_ChannelLayout_Original:
                    return ChannelLayout_2018_Rename(Info[Parameter], StreamFormat);
                default:;
            }
            break;
        default:;
    }
    ShouldReturn=ShouldReturn_Save;
    return Info[Parameter];
}
Ztring ChannelLayout_2018_Rename(stream_t StreamKind, const Ztring Parameter, const Ztring Value, const Ztring StreamFormat, bool &ShouldReturn)
{
    bool ShouldReturn_Save=ShouldReturn;
    ShouldReturn=true;
    if (StreamKind==Stream_Audio && (Parameter==__T("BedChannelConfiguration") || (Parameter.size()>=14 && Parameter.find(__T(" ChannelLayout"), Parameter.size()-14)!=string::npos)))
        return ChannelLayout_2018_Rename(Value, StreamFormat);
    if (StreamKind==Stream_Audio)
    {
        size_t ObjectPos=Parameter.find(__T("Object"));
        if (ObjectPos!=string::npos)
        {
            bool NoRename=false;
            if (ObjectPos && !(Parameter[ObjectPos-1]==__T(' ')))
                NoRename=true;
            if (ObjectPos+6>=Parameter.size() || !(Parameter[ObjectPos+6]>=__T('0') && Parameter[ObjectPos+6]<=__T('9')))
                NoRename=true;
            if (!NoRename)
            {
                size_t SpacePos=Parameter.find(__T(' '), ObjectPos);
                if (SpacePos==string::npos)
                    return ChannelLayout_2018_Rename(Value, StreamFormat);
            }
        }
        size_t AltPos=Parameter.find(__T("Alt"));
        if (AltPos!=string::npos)
        {
            bool NoRename=false;
            if (AltPos && !(Parameter[AltPos-1]==__T(' ')))
                NoRename=true;
            if (AltPos+3>=Parameter.size() || !(Parameter[AltPos+3]>=__T('0') && Parameter[AltPos+3]<=__T('9')))
                NoRename=true;
            if (!NoRename)
            {
                size_t SpacePos=Parameter.find(__T(' '), AltPos);
                if (SpacePos==string::npos)
                    return ChannelLayout_2018_Rename(Value, StreamFormat);
            }
        }
        size_t BedPos=Parameter.find(__T("Bed"));
        if (BedPos!=string::npos)
        {
            bool NoRename=false;
            if (BedPos && !(Parameter[BedPos-1]==__T(' ')))
                NoRename=true;
            if (BedPos+3>=Parameter.size() || !(Parameter[BedPos+3]>=__T('0') && Parameter[BedPos+3]<=__T('9')))
                NoRename=true;
            if (!NoRename)
            {
                size_t SpacePos=Parameter.find(__T(' '), BedPos);
                if (SpacePos==string::npos)
                    return ChannelLayout_2018_Rename(Value, StreamFormat);
            }
        }
    }
    ShouldReturn=ShouldReturn_Save;
    return Value;
}

//***************************************************************************
// Modifiers - Highest format
//***************************************************************************
Ztring HighestFormat(stream_t StreamKind, size_t Parameter, const ZtringList& Info, bool &ShouldReturn)
{
    if (Parameter>=Info.size())
        return Ztring();

    size_t Parameter_Generic;
    switch (StreamKind)
    {
        case Stream_Audio:
            switch (Parameter)
            {
                case Audio_Format: Parameter_Generic=Generic_Format; break;
                case Audio_Format_String: Parameter_Generic=Generic_Format_String; break;
                case Audio_Format_Profile: Parameter_Generic=Generic_Format_Profile; break;
                case Audio_Format_Level: Parameter_Generic=Generic_Format_Level; break;
                case Audio_Format_Info: Parameter_Generic=Generic_Format_Info; break;
                case Audio_Format_AdditionalFeatures: Parameter_Generic=Generic_Format_AdditionalFeatures; break;
                case Audio_Format_Commercial: Parameter_Generic=Generic_Format_Commercial; break;
                case Audio_Format_Commercial_IfAny: Parameter_Generic=Generic_Format_Commercial_IfAny; break;
                default: return Ztring();
            }
            break;
        case Stream_General:
            switch (Parameter)
            {
                case General_Format: Parameter_Generic=Generic_Format; break;
                case General_Format_String: Parameter_Generic=Generic_Format_String; break;
                case General_Format_Profile: Parameter_Generic=Generic_Format_Profile; break;
                case General_Format_Info: Parameter_Generic=Generic_Format_Info; break;
                case General_Format_AdditionalFeatures: Parameter_Generic=Generic_Format_AdditionalFeatures; break;
                case General_Format_Commercial: Parameter_Generic=Generic_Format_Commercial; break;
                case General_Format_Commercial_IfAny: Parameter_Generic=Generic_Format_Commercial_IfAny; break;
                default: return Ztring();
            }
            break;
        default: return Ztring();
    }

    size_t Parameter_Format=File__Analyze::Fill_Parameter(StreamKind, Generic_Format);
    size_t Parameter_Format_Profile=File__Analyze::Fill_Parameter(StreamKind, Generic_Format_Profile);
    size_t Parameter_Format_AdditionalFeatures=File__Analyze::Fill_Parameter(StreamKind, Generic_Format_AdditionalFeatures);
    if (Parameter_Format>=Info.size() || Parameter_Format_Profile>=Info.size() || Parameter_Format_AdditionalFeatures>=Info.size())
        return Ztring();

    static const Char* _16ch =__T("16-ch");
    static const Char* Bluray=__T("Blu-ray Disc");
    static const Char* AC3=__T("AC-3");
    static const Char* EAC3=__T("E-AC-3");
    static const Char* EAC3Dep=__T("E-AC-3+Dep");
    static const Char* EAC3JOC=__T("E-AC-3 JOC");
    static const Char* AAC=__T("AAC");
    static const Char* AACLC=__T("AAC LC");
    static const Char* AACLTP=__T("AAC LTP");
    static const Char* AACMain=__T("AAC Main");
    static const Char* AACSSR=__T("AAC SSR");
    static const Char* AACLCSBR=__T("AAC LC SBR");
    static const Char* AACLCSBRPS=__T("AAC LC SBR PS");
    static const Char* Core=__T("Core");
    static const Char* Discrete=__T("ES Discrete without ES Matrix");
    static const Char* Dep=__T("Dep");
    static const Char* DTS=__T("DTS");
    static const Char* ERAAC=__T("ER AAC");
    static const Char* ERAACLC=__T("ER AAC LC");
    static const Char* ERAACLTP=__T("ER AAC LTP");
    static const Char* ERAACScalable=__T("ER AAC scalable");
    static const Char* ESMatrix=__T("ES Matrix");
    static const Char* ESDiscrete=__T("ES Discrete");
    static const Char* Express=__T("Express");
    static const Char* HEAACv2 = __T("HE-AACv2");
    static const Char* HEAAC = __T("HE-AAC");
    static const Char* HEAAC_ESBR = __T("HE-AAC+eSBR");
    static const Char* JOC=__T("JOC");
    static const Char* LC=__T("LC");
    static const Char* LCSBR=__T("LC SBR");
    static const Char* LCSBRPS=__T("LC SBR PS");
    static const Char* LCESBR=__T("LC SBR eSBR");
    static const Char* LTP=__T("LTP");
    static const Char* X=__T("X");
    static const Char* IMAX=__T("IMAX");
    static const Char* MA=__T("MA");
    static const Char* Main=__T("Main");
    static const Char* MLP=__T("MLP");
    static const Char* MLPFBA=__T("MLP FBA");
    static const Char* NonCore=__T("non-core");
    static const Char* Scalable=__T("scalable");
    static const Char* SSR=__T("SSR");
    static const Char* XBR=__T("XBR");
    static const Char* XCh=__T("XCh");
    static const Char* XCH=__T("XCH");
    static const Char* XXCh=__T("XXCh");
    static const Char* XXCH=__T("XXCH");
    static const Char* X96=__T("X96");
    static const Char* x96=__T("x96");

    ShouldReturn=true; 
    switch (Parameter_Generic)
    {
        case Generic_Format:
            if (Info[Parameter_Format]==DTS)
            {
                Ztring Format=Info[Parameter];
                ZtringList Profiles;
                Profiles.Separator_Set(0, __T(" / "));
                Profiles.Write(Info[Parameter_Format_Profile]);
                for (size_t i=Profiles.size()-1; i!=(size_t)-1; i--)
                {
                    if (Profiles[i]==Express)
                        Format+=__T(" LBR");
                }
                return Format;
            }
            break;
        case Generic_Format_String:
            if (Info[Parameter_Format]==AC3 || Info[Parameter_Format]==EAC3)
            {
                Ztring ToReturn=Info[Parameter_Format];
                Ztring AdditionalFeatures=HighestFormat(StreamKind, Parameter_Format_AdditionalFeatures, Info, ShouldReturn);
                if (!AdditionalFeatures.find(EAC3))
                    ToReturn.clear();

                //Remove "Dep" from Format string
                size_t HasDep=AdditionalFeatures.find(Dep);
                if (HasDep!=string::npos)
                {
                    if (ToReturn==AC3)
                        ToReturn=EAC3;
                    if (HasDep && AdditionalFeatures[HasDep-1]==__T(' '))
                        AdditionalFeatures.erase(HasDep-1, 4);
                    else if (HasDep+3<AdditionalFeatures.size() && AdditionalFeatures[HasDep+3]==__T(' '))
                        AdditionalFeatures.erase(HasDep, 4);
                    else if (AdditionalFeatures.size()==3)
                        AdditionalFeatures.clear();
                }
                if (!AdditionalFeatures.empty())
                {
                    if (!ToReturn.empty())
                        ToReturn+=__T(' ');
                    ToReturn+=AdditionalFeatures;
                }

                return ToReturn;
            }
            else
            {
                Ztring ToReturn=HighestFormat(StreamKind, Parameter_Format, Info, ShouldReturn);
                Ztring AdditionalFeatures=HighestFormat(StreamKind, Parameter_Format_AdditionalFeatures, Info, ShouldReturn);
                if (!AdditionalFeatures.empty())
                    ToReturn+=__T(' ')+AdditionalFeatures;
                return ToReturn;
            }
            break;
        case Generic_Format_AdditionalFeatures:
            if (!Info[Parameter].empty())
                break;

            if (Info[Parameter_Format]==AAC)
            {
                const Ztring& Profile=Info[Parameter_Format_Profile];
                if (Profile.find(HEAACv2)!=string::npos)
                    return LCSBRPS;
                if (Profile.find(HEAAC_ESBR)!=string::npos)
                    return LCESBR;
                if (Profile.find(HEAAC)!=string::npos)
                    return LCSBR;
                if (Profile.find(LC)!=string::npos)
                    return LC;
                if (Profile.find(LTP)!=string::npos)
                    return LTP;
                if (Profile.find(Main)!=string::npos)
                    return Main;
                if (Profile.find(SSR)!=string::npos)
                    return SSR;
            }
            if (Info[Parameter_Format]==DTS)
            {
                Ztring AdditionalFeatures;
                ZtringList Profiles;
                Profiles.Separator_Set(0, __T(" / "));
                Profiles.Write(Info[Parameter_Format_Profile]);
                for (size_t i=Profiles.size()-1; i!=(size_t)-1; i--)
                {
                    if (Profiles[i]!=Core && Profiles[i]!=Express)
                    {
                        if (!AdditionalFeatures.empty())
                            AdditionalFeatures+=__T(' ');
                             if (Profiles[i]==ESMatrix)
                            AdditionalFeatures+=__T("ES");
                        else if (Profiles[i]==Discrete)
                            AdditionalFeatures+=__T("XCh");
                        else if (Profiles[i]==ESDiscrete)
                            AdditionalFeatures+=__T("XCh");
                        else if (Profiles[i]==MA)
                            AdditionalFeatures+=__T("XLL");
                        else
                            AdditionalFeatures+=Profiles[i];
                    }
                }
                return AdditionalFeatures;
            }
            if (Info[Parameter_Format]==ERAAC)
            {
                const Ztring& Profile=Info[Parameter_Format_Profile];
                if (Profile.find(LC)!=string::npos)
                    return LC;
                if (Profile.find(LTP)!=string::npos)
                    return LTP;
                if (Profile.find(Scalable)!=string::npos)
                    return Scalable;
            }
            if ((Info[Parameter_Format]==AC3 || Info[Parameter_Format]==EAC3) || Info[Parameter_Format].find(MLP)==0)
            {
                Ztring AdditionalFeatures;
                ZtringList Profiles;
                Profiles.Separator_Set(0, __T(" / "));
                Profiles.Write(Info[Parameter_Format_Profile]);
                const Ztring& Format=Info[Parameter_Format];
                for (size_t i=Profiles.size()-1; i!=(size_t)-1; i--)
                {
                    if (!AdditionalFeatures.empty())
                        AdditionalFeatures+=__T(' ');

                            if (Profiles[i]==EAC3Dep)
                        AdditionalFeatures+=Dep;
                    else if (Profiles[i].find(JOC)!=string::npos)
                        AdditionalFeatures+=JOC;
                    else if (Profiles[i].find(_16ch)!=string::npos)
                        AdditionalFeatures+=_16ch;
                    else if (Profiles[i]==Format)
                    {
                        if (!AdditionalFeatures.empty())
                            AdditionalFeatures.resize(AdditionalFeatures.size()-1);
                    }
                    else
                        AdditionalFeatures+=Profiles[i];
                }
                return AdditionalFeatures;
            }
            break;
        case Generic_Format_Profile:
            if (Info[Parameter_Format]==AAC)
                return Info[Audio_Format_Level];
            if (Info[Parameter_Format]==AC3 && Info[Parameter].find(EAC3)!=string::npos)
                return Bluray;
            if (Info[Parameter_Format]==DTS)
                return Ztring();
            if ((Info[Parameter_Format]==AC3 || Info[Parameter_Format]==EAC3) && (Info[Parameter].find(EAC3)!=string::npos || Info[Parameter].find(JOC)!=string::npos || Info[Parameter].find(MLP)!=string::npos))
                return Ztring();
            if (Info[Parameter_Format].find(MLP)==0)
                return Ztring();
            break;
        case Generic_Format_Level:
            if (Info[Parameter_Format]==AAC)
                return Ztring();
            break;
        case Generic_Format_Info:
            if (Info[Parameter_Format]==AAC)
            {
                const Ztring& Profile=Info[Audio_Format_Profile];
                if (Profile.find(HEAACv2)!=string::npos)
                    return "Advanced Audio Codec Low Complexity with Spectral Band Replication and Parametric Stereo";
                if (Profile.find(HEAAC)!=string::npos)
                {
                    if (Profile.find(__T("eSBR"))!=string::npos)
                        return "Advanced Audio Codec Low Complexity with Enhanced Spectral Band Replication";
                    else
                        return "Advanced Audio Codec Low Complexity with Spectral Band Replication";
                }
                if (Profile.find(LC)!=string::npos)
                    return "Advanced Audio Codec Low Complexity";
            }
            if (Info[Parameter_Format].find(MLP)!=string::npos || Info[Parameter_Format_Profile].find(MLP)!=string::npos)
            {
                Ztring ToReturn;
                if (Info[Parameter_Format]==AC3)
                    ToReturn=__T("Audio Coding 3");
                else if (Info[Parameter_Format]==EAC3)
                    ToReturn=__T("Enhanced AC-3");
                if (!ToReturn.empty())
                {
                    if (Info[Parameter_Format_Profile].find(JOC)!=string::npos)
                        ToReturn+=__T(" with Joint Object Coding");
                    ToReturn+=__T(" + ");
                }
                ToReturn+=__T("Meridian Lossless Packing");
                if (Info[Parameter_Format]==MLPFBA || Info[Parameter_Format_Profile].find(MLPFBA)!=string::npos)
                    ToReturn+=__T(" FBA");
                if (Info[Parameter_Format_Profile].find(_16ch)!=string::npos)
                    ToReturn+=__T(" with 16-channel presentation");
                return ToReturn;
            }
            if (Info[Parameter_Format]==AC3 || Info[Parameter_Format]==EAC3)
            {
                if (Info[Parameter_Format_Profile].find(JOC)!=string::npos
                 || Info[Parameter_Format_AdditionalFeatures].find(JOC)!=string::npos)
                    return __T("Enhanced AC-3 with Joint Object Coding");
                if (Info[Parameter_Format_Profile].find(EAC3)!=string::npos
                 || Info[Parameter_Format_AdditionalFeatures].find(Dep)!=string::npos)
                    return __T("Enhanced AC-3");
            }
            break;
        case Generic_Format_Commercial:
        case Generic_Format_Commercial_IfAny:
            if (Info[Parameter_Format]==AAC)
            {
                const Ztring& Profile=Info[Parameter_Format_Profile];
                if (Profile.find(HEAAC_ESBR)!=string::npos)
                    return "HE-AAC eSBR";
                if (Profile.find(HEAACv2)!=string::npos)
                    return "HE-AACv2";
                if (Profile.find(HEAAC)!=string::npos)
                    return "HE-AAC";
            }
            if (Info[Parameter_Format]==DTS)
            {
                ZtringList Profiles;
                Profiles.Separator_Set(0, __T(" / "));
                Profiles.Write(Info[Parameter_Format_Profile]);
                const char* Value=nullptr;
                bool HasHRA=false, HasXLL=false;
                for (size_t i=Profiles.size()-1; i!=(size_t)-1; i--)
                {
                    const auto& Profile=Profiles[i];
                         if (Profile==Core)
                        ;
                    else if (Profile==ESMatrix)
                        Value="DTS-ES";
                    else if (Profile==ESDiscrete)
                        Value="DTS-ES Discrete";
                    else if (Profile==x96)
                        Value="DTS 96/24";
                    else if (Profile==X96 || Profile==XBR || Profile==XXCH)
                    {
                        Value="DTS-HD High Resolution Audio";
                        HasHRA=true;
                    }
                    else if (Profile==MA)
                    {
                         Value="DTS-HD Master Audio";
                         HasXLL=true;
                    }
                    else if (Profile==Express)
                        Value="DTS Express";
                    else if (Profile==X)
                    {
                        if (HasXLL)
                            Value="DTS-HD MA + DTS:X";
                        else if (HasHRA)
                            Value="DTS-HD HRA + DTS:X";
                        else
                            Value="DTS:X";
                    }
                    else if (Profile==IMAX)
                    {
                        if (HasXLL)
                            Value="DTS-HD MA + IMAX Enhanced";
                        else if (HasHRA)
                            Value="DTS-HD HRA + IMAX Enhanced";
                        else
                            Value="IMAX Enhanced";
                    }
                }
                if (Value)
                    return Value;
            }
            break;
        default:;
    }
    return Info[Parameter];
}

static stream_t Text2StreamT(const Ztring& ParameterName, size_t ToRemove)
{
    Ztring StreamKind_Text=ParameterName.substr(0, ParameterName.size()-ToRemove);
    stream_t StreamKind2=Stream_Max;
    if (StreamKind_Text==__T("General"))
        StreamKind2=Stream_General;
    if (StreamKind_Text==__T("Video"))
        StreamKind2=Stream_Video;
    if (StreamKind_Text==__T("Audio"))
        StreamKind2=Stream_Audio;
    if (StreamKind_Text==__T("Text"))
        StreamKind2=Stream_Text;
    if (StreamKind_Text==__T("Other"))
        StreamKind2=Stream_Other;
    if (StreamKind_Text==__T("Image"))
        StreamKind2=Stream_Image;
    if (StreamKind_Text==__T("Menu"))
        StreamKind2=Stream_Menu;
    return StreamKind2;
}

//***************************************************************************
// std::cin threaded interface
//***************************************************************************

#if MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES) && !defined(WINDOWS_UWP) && !defined(__BORLANDC__)
class Reader_Cin_Thread;
set<Reader_Cin_Thread*> ToTerminate;
CriticalSection ToTerminate_CS;

static void CtrlC_Register();
static void CtrlC_Unregister();

static void Reader_Cin_Add(Reader_Cin_Thread* Thread)
{
    CriticalSectionLocker ToTerminate_CSL(ToTerminate_CS);
    if (ToTerminate.empty() && MediaInfoLib::Config.AcceptSignals_Get())
        CtrlC_Register();
    ToTerminate.insert(Thread);
}

static void Reader_Cin_Remove(Reader_Cin_Thread* Thread)
{
    CriticalSectionLocker ToTerminate_CSL(ToTerminate_CS);
    ToTerminate.erase(Thread);
    if (ToTerminate.empty() && MediaInfoLib::Config.AcceptSignals_Get())
        CtrlC_Unregister();
}

class Reader_Cin_Thread : public Thread
{
public:
    int8u* Buffer[2];
    size_t Buffer_Size[2];
    size_t Buffer_MaxSize;
    bool   Buffer_Filling;
    bool   Buffer_Used;

    Reader_Cin_Thread()
    {
        Buffer_MaxSize=64*1024;
        Buffer[0]=new int8u[Buffer_MaxSize];
        Buffer[1]=new int8u[Buffer_MaxSize];
        Buffer_Size[0]=0;
        Buffer_Size[1]=0;
        Buffer_Filling=false;
        Buffer_Used=false;
        Reader_Cin_Add(this);
    }

    ~Reader_Cin_Thread()
    {
        Reader_Cin_Remove(this);
    }

    void Current(int8u*& Buffer_New, size_t& Buffer_Size_New)
    {
        //If the current buffer is full
        Buffer_New=Buffer[Buffer_Used];
        Buffer_Size_New=Buffer_Size[Buffer_Used];
        if (Buffer_Size_New==Buffer_MaxSize)
            return;

        //Not full, we accept it only if read is finished
        if (IsRunning())
            Buffer_Size_New=0;
    }

    void IsManaged()
    {
        Buffer_Size[Buffer_Used]=0;
        Buffer_Used=!Buffer_Used;
    }

    void Entry()
    {
        while (!IsTerminating() && !IsExited())
        {
            if (Buffer_Size[Buffer_Filling]==Buffer_MaxSize) //If end of buffer is reached
            {
                Buffer_Filling=!Buffer_Filling;
                while (Buffer_Size[Buffer_Filling]) //Wait for first byte free
                    Yield(); //TODO: use condition_variable
                continue; //Check again the condition before next step
            }

            //Read from stdin
            int NewChar=getchar();
            if (NewChar==EOF)
                break;
            Buffer[Buffer_Filling][Buffer_Size[Buffer_Filling]++]=(int8u)NewChar; //Fill the new char then increase offset
        }

        RequestTerminate();
        while (Buffer_Size[Buffer_Filling])
            Yield(); //Wait for the all buffer are cleared by the read thread
    }
};

static void Reader_Cin_ForceTerminate()
{
    CriticalSectionLocker ToTerminate_CSL(ToTerminate_CS);
    for (set<Reader_Cin_Thread*>::iterator ToTerminate_Item=ToTerminate.begin(); ToTerminate_Item!=ToTerminate.end(); ++ToTerminate_Item)
        (*ToTerminate_Item)->ForceTerminate();
    ToTerminate.clear();
}

static void Reader_Cin_ForceTerminate(Reader_Cin_Thread* Thread)
{
    CriticalSectionLocker ToTerminate_CSL(ToTerminate_CS);
    Thread->ForceTerminate();
    ToTerminate.erase(Thread);
}

static void CtrlC_Received()
{
    Reader_Cin_ForceTerminate();
    CtrlC_Unregister();
}

#ifdef WINDOWS
static BOOL WINAPI SignalHandler(DWORD SignalType)
{
    if (SignalType==CTRL_C_EVENT)
    {
        CtrlC_Received();
        return true;
    }

    return FALSE;
}

static void CtrlC_Register()
{
    SetConsoleCtrlHandler(SignalHandler, TRUE);
}

static void CtrlC_Unregister()
{
    SetConsoleCtrlHandler(SignalHandler, FALSE);
}
#else //WINDOWS
static void SignalHandler(int SignalType)
{
    if (SignalType==SIGINT)
        CtrlC_Received();
}

static void CtrlC_Register()
{
    signal(SIGINT, SignalHandler);
}

static void CtrlC_Unregister()
{
    signal(SIGINT, SIG_DFL);
}
#endif //WINDOWS

#endif

//***************************************************************************
// Constructor/destructor
//***************************************************************************

//---------------------------------------------------------------------------
MediaInfo_Internal::MediaInfo_Internal()
: Thread()
{
    CriticalSectionLocker CSL(CS);

    MEDIAINFO_DEBUG_CONFIG_TEXT(Debug+=__T("Construction");)

    MediaInfoLib::Config.Init(); //Initialize Configuration

    BlockMethod=BlockMethod_Local;
    Info=NULL;
    #if !defined(MEDIAINFO_READER_NO)
        Reader=NULL;
    #endif //!defined(MEDIAINFO_READER_NO)
    Info_IsMultipleParsing=false;

    Stream.resize(Stream_Max);
    Stream_More.resize(Stream_Max);

    //Threading
    BlockMethod=0;
    IsInThread=false;
}

//---------------------------------------------------------------------------
MediaInfo_Internal::~MediaInfo_Internal()
{
    Close();

    CriticalSectionLocker CSL(CS);

    MEDIAINFO_DEBUG_CONFIG_TEXT(Debug+=__T("Destruction");)

    delete Info; //Info=NULL;
    #if !defined(MEDIAINFO_READER_NO)
        delete Reader; //Reader=NULL;
    #endif //!defined(MEDIAINFO_READER_NO)
    #ifdef MEDIAINFO_DEBUG_OUTPUT
        for (size_t Pos=0; Pos<Debug_Output_Pos_Stream.size(); Pos++)
        {
            delete Debug_Output_Pos_Stream[Pos]; //Debug_Output_Pos_Stream[Pos]=NULL;
            delete Debug_Output_Pos_Sizes[Pos]; //Debug_Output_Pos_Sizes[Pos]=NULL;
        }
    #endif //MEDIAINFO_DEBUG_OUTPUT
}

//***************************************************************************
// Files
//***************************************************************************

//---------------------------------------------------------------------------
size_t MediaInfo_Internal::Open(const String &File_Name_)
{
    Close();

    //External IBI
    #if MEDIAINFO_IBIUSAGE
        if (Config.Ibi_UseIbiInfoIfAvailable_Get())
        {
            std::string IbiFile=Config.Ibi_Get();
            if (!IbiFile.empty())
            {
                Info=new File_Ibi();
                Open_Buffer_Init(IbiFile.size(), File_Name_);
                Open_Buffer_Continue((const int8u*)IbiFile.c_str(), IbiFile.size());
                Open_Buffer_Finalize();

                if (!Get(Stream_General, 0, __T("Format")).empty() && Get(Stream_General, 0, __T("Format"))!=__T("Ibi"))
                    return 1;

                //Nothing interesting
                Close();
            }
        }
    #endif //MEDIAINFO_IBIUSAGE
    {
    CriticalSectionLocker CSL(CS);

    MEDIAINFO_DEBUG_CONFIG_TEXT(Debug+=__T("Open, File=");Debug+=Ztring(File_Name_).c_str();)
    Config.File_Names.clear();
    if (Config.File_FileNameFormat_Get()==__T("CSV"))
    {
        Config.File_Names.Separator_Set(0, __T(","));
        Config.File_Names.Write(File_Name_);
    }
    else if (!File_Name_.empty())
        Config.File_Names.push_back(File_Name_);
    if (Config.File_Names.empty())
    {
        return 0;
    }
    Config.File_Names_Pos=1;
    Config.IsFinishing=false;
    }

    //Parsing
    if (BlockMethod==1)
    {
        if (!IsInThread) //If already created, the routine will read the new files
        {
            Run();
            IsInThread=true;
        }
        return 0;
    }
    else
    {
        Entry(); //Normal parsing
        return Count_Get(Stream_General);
    }
}

//---------------------------------------------------------------------------
void MediaInfo_Internal::Entry()
{
    MEDIAINFO_DEBUG_CONFIG_TEXT(Debug+=__T("Entry");)
    Config.State_Set(0);

    if ((Config.File_Names[0].size()>=6
        && Config.File_Names[0][0]==__T('m')
        && Config.File_Names[0][1]==__T('m')
        && Config.File_Names[0][2]==__T('s')
        && Config.File_Names[0][3]==__T(':')
        && Config.File_Names[0][4]==__T('/')
        && Config.File_Names[0][5]==__T('/'))
        || (Config.File_Names[0].size()>=7
        && Config.File_Names[0][0]==__T('m')
        && Config.File_Names[0][1]==__T('m')
        && Config.File_Names[0][2]==__T('s')
        && Config.File_Names[0][3]==__T('h')
        && Config.File_Names[0][4]==__T(':')
        && Config.File_Names[0][5]==__T('/')
        && Config.File_Names[0][6]==__T('/')))
        #if defined(MEDIAINFO_LIBMMS_YES)
            Reader_libmms().Format_Test(this, Config.File_Names[0]);
        #else //MEDIAINFO_LIBMMS_YES
            {
            #if MEDIAINFO_EVENTS
                struct MediaInfo_Event_Log_0 Event;
                Event.EventCode=MediaInfo_EventCode_Create(MediaInfo_Parser_None, MediaInfo_Event_Log, 0);
                Event.Type=0xC0;
                Event.Severity=0xFF;
                Event.MessageCode=0;
                Event.MessageStringU=L"Libmms cupport is disabled due to compilation options";
                Event.MessageStringA="Libmms cupport is disabled due to compilation options";
                MediaInfoLib::Config.Event_Send((const int8u*)&Event, sizeof(MediaInfo_Event_Log_0));
            #endif //MEDIAINFO_EVENTS
            }
        #endif //MEDIAINFO_LIBMMS_YES

    else if (Config.File_Names[0].find(__T("://"))!=string::npos)
        #if defined(MEDIAINFO_LIBCURL_YES)
        {
            {
            CriticalSectionLocker CSL(CS);
            if (Reader)
            {
                return; //There is a problem
            }
            Reader=new Reader_libcurl();
            }

            Reader->Format_Test(this, Config.File_Names[0]);

            #if MEDIAINFO_NEXTPACKET
                if (Config.NextPacket_Get())
                    return;
            #endif //MEDIAINFO_NEXTPACKET
        }
        #else //MEDIAINFO_LIBCURL_YES
            {
            #if MEDIAINFO_EVENTS
                struct MediaInfo_Event_Log_0 Event;
                Event.EventCode=MediaInfo_EventCode_Create(MediaInfo_Parser_None, MediaInfo_Event_Log, 0);
                Event.Type=0xC0;
                Event.Severity=0xFF;
                Event.MessageCode=0;
                Event.MessageStringU=L"Libcurl support is disabled due to compilation options";
                Event.MessageStringA="Libcurl support is disabled due to compilation options";
                MediaInfoLib::Config.Event_Send((const int8u*)&Event, sizeof(MediaInfo_Event_Log_0));
            #endif //MEDIAINFO_EVENTS
            }
        #endif //MEDIAINFO_LIBCURL_YES

    #if defined(MEDIAINFO_DIRECTORY_YES)
        else if (Dir::Exists(Config.File_Names[0]))
            Reader_Directory().Format_Test(this, Config.File_Names[0]);
    #endif //MEDIAINFO_DIRECTORY_YES

    #if defined(MEDIAINFO_FILE_YES)
        else if (File::Exists(Config.File_Names[0]))
        {
            #if defined(MEDIAINFO_REFERENCES_YES)
                string Dxw;
                if (Config.File_CheckSideCarFiles_Get() && !Config.File_IsReferenced_Get())
                {
                    FileName Test(Config.File_Names[0]);
                    Ztring FileExtension=Test.Extension_Get();
                    FileExtension.MakeLowerCase();

                    if (FileExtension!=__T("cap"))
                    {
                        Test.Extension_Set(__T("cap"));
                        if (File::Exists(Test))
                            Dxw+=" <clip file=\""+Test.Name_Get().To_UTF8()+".cap\" />\r\n";
                    }
                    if (FileExtension!=__T("dfxp"))
                    {
                        Test.Extension_Set(__T("dfxp"));
                        if (File::Exists(Test))
                            Dxw+=" <clip file=\""+Test.Name_Get().To_UTF8()+".dfxp\" />\r\n";
                    }
                    if (FileExtension!=__T("sami"))
                    {
                        Test.Extension_Set(__T("sami"));
                        if (File::Exists(Test))
                            Dxw+=" <clip file=\""+Test.Name_Get().To_UTF8()+".sami\" />\r\n";
                    }
                    if (FileExtension!=__T("sc2"))
                    {
                        Test.Extension_Set(__T("sc2"));
                        if (File::Exists(Test))
                            Dxw+=" <clip file=\""+Test.Name_Get().To_UTF8()+".sc2\" />\r\n";
                    }
                    if (FileExtension!=__T("scc"))
                    {
                        Test.Extension_Set(__T("scc"));
                        if (File::Exists(Test))
                            Dxw+=" <clip file=\""+Test.Name_Get().To_UTF8()+".scc\" />\r\n";
                    }
                    if (FileExtension!=__T("smi"))
                    {
                        Test.Extension_Set(__T("smi"));
                        if (File::Exists(Test))
                            Dxw+=" <clip file=\""+Test.Name_Get().To_UTF8()+".smi\" />\r\n";
                    }
                    if (FileExtension!=__T("srt"))
                    {
                        Test.Extension_Set(__T("srt"));
                        if (File::Exists(Test))
                            Dxw+=" <clip file=\""+Test.Name_Get().To_UTF8()+".srt\" />\r\n";
                    }
                    if (FileExtension!=__T("stl"))
                    {
                        Test.Extension_Set(__T("stl"));
                        if (File::Exists(Test))
                            Dxw+=" <clip file=\""+Test.Name_Get().To_UTF8()+".stl\" />\r\n";
                    }
                    if (FileExtension!=__T("ttml"))
                    {
                        Test.Extension_Set(__T("ttml"));
                        if (File::Exists(Test))
                            Dxw+=" <clip file=\""+Test.Name_Get().To_UTF8()+".ttml\" />\r\n";
                    }
                    if (FileExtension!=__T("ssa"))
                    {
                        Test.Extension_Set(__T("ssa"));
                        if (File::Exists(Test))
                            Dxw+=" <clip file=\""+Test.Name_Get().To_UTF8()+".ssa\" />\r\n";
                    }
                    if (FileExtension!=__T("ass"))
                    {
                        Test.Extension_Set(__T("ass"));
                        if (File::Exists(Test))
                            Dxw+=" <clip file=\""+Test.Name_Get().To_UTF8()+".ass\" />\r\n";
                    }
                    if (FileExtension!=__T("vtt"))
                    {
                        Test.Extension_Set(__T("vtt"));
                        if (File::Exists(Test))
                            Dxw+=" <clip file=\""+Test.Name_Get().To_UTF8()+".vtt\" />\r\n";
                    }
                    if (FileExtension!=__T("xml"))
                    {
                        Test.Extension_Set(__T("xml"));
                        if (File::Exists(Test))
                        {
                            MediaInfo_Internal MI;
                            Ztring ParseSpeed_Save=MI.Option(__T("ParseSpeed_Get"), __T(""));
                            Ztring Demux_Save=MI.Option(__T("Demux_Get"), __T(""));
                            MI.Option(__T("ParseSpeed"), __T("0"));
                            MI.Option(__T("Demux"), Ztring());
                            size_t MiOpenResult=MI.Open(Test);
                            MI.Option(__T("ParseSpeed"), ParseSpeed_Save); //This is a global value, need to reset it. TODO: local value
                            MI.Option(__T("Demux"), Demux_Save); //This is a global value, need to reset it. TODO: local value
                            if (MiOpenResult)
                            {
                                Ztring Format=MI.Get(Stream_General, 0, General_Format);
                                if (Format==__T("TTML"))
                                    Dxw+=" <clip file=\""+Test.Name_Get().To_UTF8()+".xml\" />\r\n";
                            }
                        }
                    }

                    #if defined(MEDIAINFO_DIRECTORY_YES)
                    Ztring Name=Test.Name_Get();
                    Ztring BaseName=Name.SubString(Ztring(), __T("_"));
                    if (!BaseName.empty())
                    {
                        ZtringList List;
                        List+=Dir::GetAllFileNames(Test.Path_Get()+PathSeparator+BaseName+__T("_*_audio.mp4"), Dir::Include_Files);
                        List+=Dir::GetAllFileNames(Test.Path_Get()+PathSeparator+BaseName+__T("_*_sub.dfxp"), Dir::Include_Files);
                        List+=Dir::GetAllFileNames(Test.Path_Get()+PathSeparator+BaseName+__T("_*_sub.sami"), Dir::Include_Files);
                        List+=Dir::GetAllFileNames(Test.Path_Get()+PathSeparator+BaseName+__T("_*_sub.sc2"), Dir::Include_Files);
                        List+=Dir::GetAllFileNames(Test.Path_Get()+PathSeparator+BaseName+__T("_*_sub.scc"), Dir::Include_Files);
                        List+=Dir::GetAllFileNames(Test.Path_Get()+PathSeparator+BaseName+__T("_*_sub.smi"), Dir::Include_Files);
                        List+=Dir::GetAllFileNames(Test.Path_Get()+PathSeparator+BaseName+__T("_*_sub.srt"), Dir::Include_Files);
                        List+=Dir::GetAllFileNames(Test.Path_Get()+PathSeparator+BaseName+__T("_*_sub.stl"), Dir::Include_Files);
                        List+=Dir::GetAllFileNames(Test.Path_Get()+PathSeparator+BaseName+__T("_*_sub.vtt"), Dir::Include_Files);
                        List+=Dir::GetAllFileNames(Test.Path_Get()+PathSeparator+BaseName+__T("_*_forcesub.dfxp"), Dir::Include_Files);
                        List+=Dir::GetAllFileNames(Test.Path_Get()+PathSeparator+BaseName+__T("_*_forcesub.sami"), Dir::Include_Files);
                        List+=Dir::GetAllFileNames(Test.Path_Get()+PathSeparator+BaseName+__T("_*_forcesub.sc2"), Dir::Include_Files);
                        List+=Dir::GetAllFileNames(Test.Path_Get()+PathSeparator+BaseName+__T("_*_forcesub.scc"), Dir::Include_Files);
                        List+=Dir::GetAllFileNames(Test.Path_Get()+PathSeparator+BaseName+__T("_*_forcesub.smi"), Dir::Include_Files);
                        List+=Dir::GetAllFileNames(Test.Path_Get()+PathSeparator+BaseName+__T("_*_forcesub.srt"), Dir::Include_Files);
                        List+=Dir::GetAllFileNames(Test.Path_Get()+PathSeparator+BaseName+__T("_*_forcesub.stl"), Dir::Include_Files);
                        List+=Dir::GetAllFileNames(Test.Path_Get()+PathSeparator+BaseName+__T("_*_forcesub.vtt"), Dir::Include_Files);
                        List+=Dir::GetAllFileNames(Test.Path_Get()+PathSeparator+BaseName+__T("_*_cc.dfxp"), Dir::Include_Files);
                        List+=Dir::GetAllFileNames(Test.Path_Get()+PathSeparator+BaseName+__T("_*_cc.sami"), Dir::Include_Files);
                        List+=Dir::GetAllFileNames(Test.Path_Get()+PathSeparator+BaseName+__T("_*_cc.sc2"), Dir::Include_Files);
                        List+=Dir::GetAllFileNames(Test.Path_Get()+PathSeparator+BaseName+__T("_*_cc.scc"), Dir::Include_Files);
                        List+=Dir::GetAllFileNames(Test.Path_Get()+PathSeparator+BaseName+__T("_*_cc.smi"), Dir::Include_Files);
                        List+=Dir::GetAllFileNames(Test.Path_Get()+PathSeparator+BaseName+__T("_*_cc.srt"), Dir::Include_Files);
                        List+=Dir::GetAllFileNames(Test.Path_Get()+PathSeparator+BaseName+__T("_*_cc.stl"), Dir::Include_Files);
                        List+=Dir::GetAllFileNames(Test.Path_Get()+PathSeparator+BaseName+__T("_*_cc.vtt"), Dir::Include_Files);
                        for (size_t Pos=0; Pos<List.size(); Pos++)
                            Dxw+=" <clip file=\""+List[Pos].To_UTF8()+"\" />\r\n";
                    }
                    #endif //defined(MEDIAINFO_DIRECTORY_YES)

                    if (!Dxw.empty())
                    {
                        Dxw.insert(0, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\r\n"
                                "<indexFile xmlns=\"urn:digimetrics-xml-wrapper\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"urn:digimetrics-xml-wrapper DMSCLIP.XSD\">\r\n"
                                " <clip source=\"main\" file=\""+FileName(Config.File_Names[0]).Name_Get().To_UTF8()+"."+FileName(Config.File_Names[0]).Extension_Get().To_UTF8()+"\" />\r\n");
                        Dxw.append("</indexFile>\r\n");
                        Config.File_FileNameFormat_Set(__T("Dxw"));
                    }
                }

                if (Dxw.empty())
                {
                    {
                    CriticalSectionLocker CSL(CS);
                    if (Reader)
                    {
                        return; //There is a problem
                    }
                    Reader=new Reader_File();
                    }

                    Reader->Format_Test(this, Config.File_Names[0]);
                }
                else
                {
                    Open_Buffer_Init(Dxw.size(), FileName(Config.File_Names[0]).Path_Get()+PathSeparator+FileName(Config.File_Names[0]).Name_Get());
                    Open_Buffer_Continue((const int8u*)Dxw.c_str(), Dxw.size());
                    #if MEDIAINFO_NEXTPACKET
                        if (Config.NextPacket_Get())
                            return;
                    #endif //MEDIAINFO_NEXTPACKET
                    Open_Buffer_Finalize();
                }
            #else //defined(MEDIAINFO_REFERENCES_YES)
                {
                CriticalSectionLocker CSL(CS);
                if (Reader)
                {
                    return; //There is a problem
                }
                Reader=new Reader_File();
                }
                Reader->Format_Test(this, Config.File_Names[0]);
            #endif //defined(MEDIAINFO_REFERENCES_YES)

            #if MEDIAINFO_NEXTPACKET
                if (Config.NextPacket_Get())
                    return;
            #endif //MEDIAINFO_NEXTPACKET
        }
    #endif //MEDIAINFO_FILE_YES
    #if MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES) && !defined(WINDOWS_UWP) && !defined(__BORLANDC__)
        else if (Config.File_Names[0]==__T("-")
            #if defined(WINDOWS) && !defined(WINDOWS_UWP) && !defined(__BORLANDC__)
                //&& WaitForSingleObject(GetStdHandle(STD_INPUT_HANDLE), 0) == WAIT_OBJECT_0 //Check if there is something is stdin
                && _setmode(_fileno(stdin), _O_BINARY) != -1 //Force binary mode
            #endif //defined(WINDOWS) && !defined(WINDOWS_UWP) && !defined(__BORLANDC__)
            )
        {
            Reader_Cin_Thread Cin;
            Cin.Run();
            Open_Buffer_Init();
            clock_t LastIn=-1;
            int64u TimeOut_Temp=MediaInfoLib::Config.TimeOut_Get();
            int64u TimeOut_Temp2=TimeOut_Temp*CLOCKS_PER_SEC;
            clock_t TimeOut=(!CLOCKS_PER_SEC || !TimeOut_Temp || TimeOut_Temp2/CLOCKS_PER_SEC==TimeOut_Temp)?((clock_t)TimeOut_Temp2):-1;

            for (;;)
            {
                int8u* Buffer_New;
                size_t Buffer_Size_New;
                if (Cin.IsExited())
                    break;
                Cin.Current(Buffer_New, Buffer_Size_New);
                if (Buffer_Size_New)
                {
                    if (Open_Buffer_Continue(Buffer_New, Buffer_Size_New)[File__Analyze::IsFinished])
                        Reader_Cin_ForceTerminate(&Cin);
                    if (Config.RequestTerminate)
                        Cin.RequestTerminate();
                    Cin.IsManaged();
                    if (TimeOut!=-1)
                        LastIn=clock();
                }
                else
                {
                    if (LastIn!=-1)
                    {
                        clock_t NewLastIn=clock();
                        if (NewLastIn-LastIn>=TimeOut)
                        {
                            Reader_Cin_ForceTerminate(&Cin);
                            LastIn=-1;
                        }
                    }

                    #ifdef WINDOWS
                        Sleep(0);
                    #elif defined(_POSIX_PRIORITY_SCHEDULING)
                        sched_yield();
                    #endif //_POSIX_PRIORITY_SCHEDULING
                }
            }
            Open_Buffer_Finalize();
        }
    #endif //MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES) && !defined(WINDOWS_UWP) && !defined(__BORLANDC__)

    Config.State_Set(1);
}

//---------------------------------------------------------------------------
size_t MediaInfo_Internal::Open (const int8u* Begin, size_t Begin_Size, const int8u* End, size_t End_Size, int64u File_Size)
{
    Open_Buffer_Init(File_Size);
    Open_Buffer_Continue(Begin, Begin_Size);
    if (End && Begin_Size+End_Size<=File_Size)
    {
        Open_Buffer_Init(File_Size, File_Size-End_Size);
        Open_Buffer_Continue(End, End_Size);
    }
    Open_Buffer_Finalize();

    return 1;
}

//---------------------------------------------------------------------------
size_t MediaInfo_Internal::Open_Buffer_Init (int64u File_Size_, const String &File_Name)
{
    CriticalSectionLocker CSL(CS);

    if (Config.File_Names.size()<=1) //If analyzing multiple files, theses members are adapted in File_Reader.cpp
    {
        if (File_Size_!=(int64u)-1)
        {
            Config.File_Size=Config.File_Current_Size=File_Size_;
            if (!Config.File_Sizes.empty())
                Config.File_Sizes[Config.File_Sizes.size()-1]=File_Size_;
        }
    }

    if (Info==NULL)
    {
        Ztring ForceParser = Config.File_ForceParser_Get();
        if (!ForceParser.empty())
        {
            CS.Leave();
            SelectFromExtension(ForceParser);
            CS.Enter();
        }
        if (Info==NULL)
        {
            Info=new File__MultipleParsing;
            Info_IsMultipleParsing=true;
        }
    }
    #if MEDIAINFO_TRACE
        Info->Init(&Config, &Details, &Stream, &Stream_More);
    #else //MEDIAINFO_TRACE
        Info->Init(&Config, &Stream, &Stream_More);
    #endif //MEDIAINFO_TRACE
    if (!File_Name.empty())
        Info->File_Name=File_Name;
    Info->Open_Buffer_Init(File_Size_);

    if (File_Name.empty())
    {
        #if MEDIAINFO_EVENTS
            {
                struct MediaInfo_Event_General_Start_0 Event;
                memset(&Event, 0xFF, sizeof(struct MediaInfo_Event_Generic));
                Event.EventCode=MediaInfo_EventCode_Create(MediaInfo_Parser_None, MediaInfo_Event_General_Start, 0);
                Event.EventSize=sizeof(struct MediaInfo_Event_General_Start_0);
                Event.StreamIDs_Size=0;
                Event.Stream_Size=File_Size_;
                Event.FileName=NULL;
                Event.FileName_Unicode=NULL;
                Config.Event_Send(NULL, (const int8u*)&Event, sizeof(MediaInfo_Event_General_Start_0));
            }
        #endif //MEDIAINFO_EVENTS
    }

    return 1;
}

//---------------------------------------------------------------------------
size_t MediaInfo_Internal::Open_Buffer_Init (int64u File_Size_, int64u File_Offset_)
{
    MEDIAINFO_DEBUG_CONFIG_TEXT(Debug+=__T("Open_Buffer_Init, File_Size=");Debug+=Ztring::ToZtring(File_Size_);Debug+=__T(", File_Offset=");Debug+=Ztring::ToZtring(File_Offset_);)
    #ifdef MEDIAINFO_DEBUG_BUFFER
        if (Info && File_Offset_>Info->File_Offset)
        {
            size_t Temp_Size=(size_t)(File_Offset_-Info->File_Offset);
            int8u* Temp=new int8u[Temp_Size];
            std::memset(Temp, 0xCC, Temp_Size);
            MEDIAINFO_DEBUG_BUFFER_SAVE(Temp, Temp_Size);
            delete[] Temp;
        }
    #endif //MEDIAINFO_DEBUG_BUFFER

    if (Config.File_Names.size()<=1) //If analyzing multiple files, theses members are adapted in File_Reader.cpp
    {
        if (File_Size_!=(int64u)-1)
        {
            Config.File_Size=Config.File_Current_Size=File_Size_;
            if (!Config.File_Sizes.empty())
                Config.File_Sizes[Config.File_Sizes.size()-1]=File_Size_;

            if (Info && !Info->Retrieve(Stream_General, 0, General_FileSize).empty())
                Info->Fill(Stream_General, 0, General_FileSize, File_Size_, 10, true); //TODO: avoid multiple tests of file size field, refactor it in order to have a single place for file size info
        }
    }

    if (Info==NULL || File_Size_!=(int64u)-1)
        Open_Buffer_Init(File_Size_);

    if (File_Offset_!=(int64u)-1 && Info)
    {
        CriticalSectionLocker CSL(CS);
        Info->Open_Buffer_Position_Set(File_Offset_);
    }

    #if MEDIAINFO_EVENTS
        if (Info && Info->Status[File__Analyze::IsAccepted])
        {
            struct MediaInfo_Event_General_Move_Done_0 Event;
            Info->Event_Prepare((struct MediaInfo_Event_Generic*)&Event, MediaInfo_EventCode_Create(MediaInfo_Parser_None, MediaInfo_Event_General_Move_Done, 0), sizeof(struct MediaInfo_Event_General_Move_Done_0));
            Event.StreamIDs_Size=0;
            Event.StreamOffset=File_Offset_;
            Config.Event_Send(NULL, (const int8u*)&Event, sizeof(MediaInfo_Event_General_Move_Done_0));
        }
        else
        {
            struct MediaInfo_Event_General_Start_0 Event;
            Info->Event_Prepare((struct MediaInfo_Event_Generic*)&Event, MediaInfo_EventCode_Create(MediaInfo_Parser_None, MediaInfo_Event_General_Start, 0), sizeof(struct MediaInfo_Event_General_Start_0));
            Event.StreamIDs_Size=0;
            Event.Stream_Size=File_Size_;
            Event.FileName=NULL;
            Event.FileName_Unicode=NULL;
            Config.Event_Send(NULL, (const int8u*)&Event, sizeof(MediaInfo_Event_General_Start_0));
        }
    #endif //MEDIAINFO_EVENTS

    EXECUTE_SIZE_T(1, Debug+=__T("Open_Buffer_Init, will return 1");)
}

//---------------------------------------------------------------------------
#if MEDIAINFO_ADVANCED2
size_t MediaInfo_Internal::Open_Buffer_SegmentChange ()
{
    MEDIAINFO_DEBUG_CONFIG_TEXT(Debug+=__T("Open_Buffer_SegmentChange"))

    if (Info == NULL)
        return 0;
    Info->Open_Buffer_SegmentChange();

    return 1;
}
#endif //MEDIAINFO_ADVANCED2

//---------------------------------------------------------------------------
void MediaInfo_Internal::Open_Buffer_Unsynch ()
{
    MEDIAINFO_DEBUG_CONFIG_TEXT(Debug+=__T("Open_Buffer_Unsynch");)

    if (Info==NULL)
        return;

    CriticalSectionLocker CSL(CS);
    Info->Open_Buffer_Unsynch();
}

//---------------------------------------------------------------------------
std::bitset<32> MediaInfo_Internal::Open_Buffer_Continue (const int8u* ToAdd, size_t ToAdd_Size)
{
    CriticalSectionLocker CSL(CS);
    MEDIAINFO_DEBUG_BUFFER_SAVE(ToAdd, ToAdd_Size);
    if (Info==NULL)
        return 0;

    //Encoded content
    #if MEDIAINFO_COMPRESS
        bool zlib=MediaInfoLib::Config.FlagsX_Get(Flags_Input_zlib);
        bool base64=MediaInfoLib::Config.FlagsX_Get(Flags_Input_base64);
        if (zlib || base64)
        {
            if (!ToAdd_Size || ToAdd_Size!=Config.File_Size)
            {
                Info->ForceFinish(); // File must be complete when this option is used
                return Info->Status;
            }
            string Input_Cache; // In case of encoded content, this string must live up to the end of the parsing
            if (base64)
            {
                Input_Cache.assign((const char*)ToAdd, ToAdd_Size);
                Input_Cache=Base64::decode(Input_Cache);
                ToAdd=(const int8u*)Input_Cache.c_str();
                ToAdd_Size=Input_Cache.size();
            }
            if (zlib)
            {
                uLongf Output_Size_Max = ToAdd_Size;
                while (Output_Size_Max)
                {
                    Output_Size_Max *=16;
                    int8u* Output = new int8u[Output_Size_Max];
                    uLongf Output_Size = Output_Size_Max;
                    if (uncompress((Bytef*)Output, &Output_Size, (const Bytef*)ToAdd, (uLong)ToAdd_Size)>=0)
                    {
                        ToAdd=Output;
                        ToAdd_Size=Output_Size;
                        break;
                    }
                    delete[] Output;
                    if (Output_Size_Max>=4*1024*1024)
                    {
                        Info->ForceFinish();
                        return Info->Status;
                    }
                }
            }
            Info->Open_Buffer_Continue(ToAdd, ToAdd_Size);
            if (zlib)
                delete[] ToAdd;
        }
        else
    #endif //MEDIAINFO_COMPRESS
    Info->Open_Buffer_Continue(ToAdd, ToAdd_Size);

    if (Info_IsMultipleParsing && Info->Status[File__Analyze::IsAccepted])
    {
        //Found
        File__Analyze* Info_ToDelete=Info;
        Info=((File__MultipleParsing*)Info)->Parser_Get();
        delete Info_ToDelete; //Info_ToDelete=NULL;
        Info_IsMultipleParsing=false;
    }

    #if 0 //temp, for old users
    //The parser wanted seek but the buffer is not seekable
    if (Info->File_GoTo!=(int64u)-1 && Config.File_IsSeekable_Get()==0)
    {
        Info->Open_Buffer_Finalize(true);
        Info->File_GoTo=(int64u)-1;
        MEDIAINFO_DEBUG_CONFIG_TEXT(Debug+=__T("Open_Buffer_Continue, will return 0");)
        return 0;
    }

    return 1;
    #else
    //The parser wanted seek but the buffer is not seekable
    if (Info->File_GoTo!=(int64u)-1 && Config.File_IsSeekable_Get()==0)
    {
        Info->Fill();
        Info->File_GoTo=(int64u)-1;
    }

    return Info->Status;
    #endif
}

//---------------------------------------------------------------------------
int64u MediaInfo_Internal::Open_Buffer_Continue_GoTo_Get ()
{
    CriticalSectionLocker CSL(CS);
    if (Info==NULL)
        return (int64u)-1;

    if (Info->File_GoTo==(int64u)-1
     || (Info->File_GoTo>=Info->File_Offset && Info->File_GoTo<Info->File_Offset+0x10000)) //If jump is tiny, this is not worth the performance cost due to seek
        return (int64u)-1;
    else
        return Info->File_GoTo;
}

//---------------------------------------------------------------------------
void MediaInfo_Internal::Open_Buffer_CheckFileModifications()
{
    CriticalSectionLocker CSL(CS);
    if (Info == NULL)
        return;

    Info->Open_Buffer_CheckFileModifications();
}

//---------------------------------------------------------------------------
bool MediaInfo_Internal::Open_Buffer_Position_Set(int64u File_Offset)
{
    CriticalSectionLocker CSL(CS);
    if (Info==NULL)
        return false;

    Info->Open_Buffer_Position_Set(File_Offset);

    return true;
}

//---------------------------------------------------------------------------
#if MEDIAINFO_SEEK
size_t MediaInfo_Internal::Open_Buffer_Seek (size_t Method, int64u Value, int64u ID)
{
    CriticalSectionLocker CSL(CS);
    if (Info==NULL)
        return 0;

    return Info->Open_Buffer_Seek(Method, Value, ID);
}
#endif //MEDIAINFO_SEEK

//---------------------------------------------------------------------------
size_t MediaInfo_Internal::Open_Buffer_Finalize ()
{
    CriticalSectionLocker CSL(CS);
    MEDIAINFO_DEBUG_CONFIG_TEXT(Debug+=__T("Open_Buffer_Finalize");)
    if (Info==NULL)
        return 0;

    Info->Open_Buffer_Finalize();
    #if MEDIAINFO_DEMUX
        if (Config.Demux_EventWasSent)
            return 0;
    #endif //MEDIAINFO_DEMUX

    //Cleanup
    if (!Config.File_IsSub_Get() && !Config.File_KeepInfo_Get()) //We need info for the calling parser
    {
        #if MEDIAINFO_TRACE
        ParserName=Ztring().From_UTF8(Info->ParserName); //Keep it in memory in case we need it after delete of Info
        #endif //MEDIAINFO_TRACE
        delete Info; Info=NULL;
    }
    if (Config.File_Names_Pos>=Config.File_Names.size())
    {
        delete[] Config.File_Buffer; Config.File_Buffer=NULL; Config.File_Buffer_Size=0; Config.File_Buffer_Size_Max=0;
    }
    #if MEDIAINFO_EVENTS
        if (!Config.File_IsReferenced_Get()) //TODO: get its own metadata in order to know if it was created by this instance
        {
            delete Config.Config_PerPackage; Config.Config_PerPackage=NULL;
        }
    #endif //MEDIAINFO_EVENTS

    EXECUTE_SIZE_T(1, Debug+=__T("Open_Buffer_Finalize, will return 1"))
}

//---------------------------------------------------------------------------
std::bitset<32> MediaInfo_Internal::Open_NextPacket ()
{
    CriticalSectionLocker CSL(CS);

    bool Demux_EventWasSent=false;
    if (Info==NULL || !Info->Status[File__Analyze::IsFinished])
    {
        #if !defined(MEDIAINFO_READER_NO)
            if (Reader)
            {
                CS.Leave();
                Demux_EventWasSent=(Reader->Format_Test_PerParser_Continue(this)==2);
                CS.Enter();
            }
            else
        #endif //defined(MEDIAINFO_READER_NO)
            {
                #if MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
                    Config.Demux_EventWasSent=false;
                #endif //MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
                Open_Buffer_Continue(NULL, 0);
                #if MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
                    if (!Config.Demux_EventWasSent)
                #endif //MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
                        Open_Buffer_Finalize();
                #if MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
                    Demux_EventWasSent=Config.Demux_EventWasSent;
                #endif //MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
            }
    }

    std::bitset<32> ToReturn=Info==NULL?std::bitset<32>(0x0F):Info->Status;
    if (Demux_EventWasSent)
        ToReturn[8]=true; //bit 8 is for the reception of a frame

    return ToReturn;
}

//---------------------------------------------------------------------------
void MediaInfo_Internal::Close()
{
    if (IsRunning() || IsTerminating())
    {
        RequestTerminate();
        while(!IsExited())
            Yield();
    }

    CriticalSectionLocker CSL(CS);
    MEDIAINFO_DEBUG_CONFIG_TEXT(Debug+=__T("Close");)
    Stream.clear();
    Stream.resize(Stream_Max);
    Stream_More.clear();
    Stream_More.resize(Stream_Max);
    delete Info; Info=NULL;
    #if !defined(MEDIAINFO_READER_NO)
        delete Reader; Reader=NULL;
    #endif //defined(MEDIAINFO_READER_NO)
}

//***************************************************************************
// Get File info
//***************************************************************************

/*//---------------------------------------------------------------------------
Ztring MediaInfo_Internal::Inform(size_t)
{
    //Info case
    if (Info)
        return Info->Inform();

    if (!Info)
        return MediaInfoLib::Config.EmptyString_Get();

    return Info->Inform();
} */

//---------------------------------------------------------------------------
Ztring MediaInfo_Internal::Get(stream_t StreamKind, size_t StreamPos, size_t Parameter, info_t KindOfInfo)
{
    #if MEDIAINFO_ADVANCED
    if (StreamKind==Stream_General && KindOfInfo==Info_Text)
    {
        switch (Parameter)
        {
            case General_Video_Codec_List: 
            case General_Audio_Codec_List: 
            case General_Text_Codec_List: 
            case General_Image_Codec_List: 
            case General_Other_Codec_List: 
            case General_Menu_Codec_List:
                if (!MediaInfoLib::Config.Legacy_Get())
                {
                    // MediaInfo GUI is using them by default in one of its old templates, using the "Format" ones.
                    return Get(StreamKind, StreamPos, Parameter-2, Info_Text);
                }
                {
                Ztring ParameterName=Get(Stream_General, 0, Parameter, Info_Name);
                stream_t StreamKind2=Text2StreamT(ParameterName, 12);
                size_t Parameter2=File__Analyze::Fill_Parameter(StreamKind2, Generic_Codec_String);
                Ztring Temp;
                size_t Count=Count_Get(StreamKind2);
                for (size_t i=0; i<Count; i++)
                {
                    if (i)
                        Temp+=__T(" / ");
                    size_t Temp_Size=Temp.size();
                    Temp+=Get(StreamKind2, i, Parameter2, Info_Text);
                }
                return (Temp.size()>(Count-1)*3)?Temp:Ztring();
                }
                break;
            case General_Video_Format_List: 
            case General_Audio_Format_List:
            case General_Text_Format_List:
            case General_Image_Format_List:
            case General_Other_Format_List:
            case General_Menu_Format_List:
                {
                Ztring ParameterName=Get(Stream_General, 0, Parameter, Info_Name);
                stream_t StreamKind2=Text2StreamT(ParameterName, 12);
                size_t Parameter2=File__Analyze::Fill_Parameter(StreamKind2, Generic_Format_String);
                Ztring Temp;
                size_t Count=Count_Get(StreamKind2);
                for (size_t i=0; i<Count; i++)
                {
                    if (i)
                        Temp+=__T(" / ");
                    size_t Temp_Size=Temp.size();
                    Temp+=Get(StreamKind2, i, Parameter2, Info_Text);
                }
                return (Temp.size()>(Count-1)*3)?Temp:Ztring();
                }
            case General_Video_Format_WithHint_List:
            case General_Audio_Format_WithHint_List:
            case General_Text_Format_WithHint_List:
            case General_Image_Format_WithHint_List:
            case General_Other_Format_WithHint_List:
            case General_Menu_Format_WithHint_List:
                {
                Ztring ParameterName=Get(Stream_General, 0, Parameter, Info_Name);
                stream_t StreamKind2=Text2StreamT(ParameterName, 21);
                size_t Parameter2=File__Analyze::Fill_Parameter(StreamKind2, Generic_Format_String);
                Ztring Temp;
                size_t Count=Count_Get(StreamKind2);
                for (size_t i=0; i<Count; i++)
                {
                    if (i)
                        Temp+=__T(" / ");
                    size_t Temp_Size=Temp.size();
                    Temp+=Get(StreamKind2, i, Parameter2, Info_Text);
                    Ztring Hint=Get(StreamKind2, i, File__Analyze::Fill_Parameter(StreamKind2, Generic_CodecID_Hint), Info_Text);
                    if (!Hint.empty())
                    {
                        Temp+=__T(" (");
                        Temp+=Hint;
                        Temp+=__T(')');
                    }
                }
                return (Temp.size()>(Count-1)*3)?Temp:Ztring();
                }
            case General_Video_Language_List:
            case General_Audio_Language_List:
            case General_Text_Language_List:
            case General_Image_Language_List:
            case General_Other_Language_List:
            case General_Menu_Language_List:
                {
                Ztring ParameterName=Get(Stream_General, 0, Parameter, Info_Name);
                stream_t StreamKind2=Text2StreamT(ParameterName, 14);
                size_t Parameter2=File__Analyze::Fill_Parameter(StreamKind2, Generic_Language)+1;
                Ztring Temp;
                size_t Count=Count_Get(StreamKind2);
                for (size_t i=0; i<Count; i++)
                {
                    if (i)
                        Temp+=__T(" / ");
                    size_t Temp_Size=Temp.size();
                    Temp+=Get(StreamKind2, i, Parameter2, Info_Text);
                }
                return (Temp.size()>(Count-1)*3)?Temp:Ztring();
                }
            case General_VideoCount: 
            case General_AudioCount: 
            case General_TextCount: 
            case General_ImageCount:
            case General_OtherCount:
            case General_MenuCount:
                {
                stream_t StreamKind2=Text2StreamT(Get(Stream_General, 0, Parameter, Info_Name), 5);
                size_t Count=Count_Get(StreamKind2);
                if (Count)
                    return Ztring::ToZtring(Count);
                else
                    return Ztring();
                }
            default:;
        }
    }
    #endif //MEDIAINFO_ADVANCED

    CriticalSectionLocker CSL(CS);
    MEDIAINFO_DEBUG_CONFIG_TEXT(Debug+=__T("Get, StreamKind=");Debug+=Ztring::ToZtring((size_t)StreamKind);Debug+=__T(", StreamPos=");Debug+=Ztring::ToZtring(StreamPos);Debug+=__T(", Parameter=");Debug+=Ztring::ToZtring(Parameter);)

    if (Info && Info->Status[File__Analyze::IsUpdated])
    {
        Info->Open_Buffer_Update();
        Info->Status[File__Analyze::IsUpdated]=false;
        for (size_t Pos=File__Analyze::User_16; Pos<File__Analyze::User_16+16; Pos++)
            Info->Status[Pos]=false;
    }

    //Check integrity
    if (StreamKind>=Stream_Max || StreamPos>=Stream[StreamKind].size() || Parameter>=MediaInfoLib::Config.Info_Get(StreamKind).size()+Stream_More[StreamKind][StreamPos].size() || KindOfInfo>=Info_Max)
        return MediaInfoLib::Config.EmptyString_Get(); //Parameter is unknown

    else if (Parameter<MediaInfoLib::Config.Info_Get(StreamKind).size())
    {
        //Optimization : KindOfInfo>Info_Text is in static lists
        if (KindOfInfo!=Info_Text)
            EXECUTE_STRING(MediaInfoLib::Config.Info_Get(StreamKind, Parameter, KindOfInfo), Debug+=__T("Get, will return ");Debug+=ToReturn;) //look for static information only
        else if (Parameter<Stream[StreamKind][StreamPos].size())
        {
            bool ShouldReturn=false;
            #if MEDIAINFO_ADVANCED
            if (Config.File_HighestFormat_Get())
            #endif //MEDIAINFO_ADVANCED
            {
                if (StreamKind==Stream_General && (Parameter==General_Audio_Format_List || Parameter==General_Audio_Format_WithHint_List))
                {
                    ZtringList List;
                    List.Separator_Set(0, __T(" / "));
                    List.Write(Stream[Stream_General][StreamPos][Parameter]);
                    for (size_t i=0; i<List.size(); i++)
                        List[i]=HighestFormat(Stream_Audio, Audio_Format_String, Stream[Stream_Audio][i], ShouldReturn);
                    if (ShouldReturn)
                    {
                        Ztring ToReturn=List.Read();
                        EXECUTE_STRING(ToReturn, Debug+=__T("Get, will return "); Debug+=ToReturn;)
                    }
                }
                else
                {
                    Ztring ToReturn=HighestFormat(StreamKind, Parameter, Stream[StreamKind][StreamPos], ShouldReturn);
                    if (ShouldReturn)
                        EXECUTE_STRING(ToReturn, Debug+=__T("Get, will return "); Debug+=ToReturn;)
                }
            }
            size_t Format_Pos=File__Analyze::Fill_Parameter(StreamKind, Generic_Format);
            #if MEDIAINFO_ADVANCED
            if (Config.File_ChannelLayout_Get())
            #endif //MEDIAINFO_ADVANCED
            {
                Ztring ToReturn;
                if (Format_Pos<Stream[StreamKind][StreamPos].size())
                    ToReturn=ChannelLayout_2018_Rename(StreamKind, Parameter, Stream[StreamKind][StreamPos], Stream[StreamKind][StreamPos][Format_Pos], ShouldReturn);
                if (ShouldReturn)
                    EXECUTE_STRING(ToReturn, Debug+=__T("Get, will return "); Debug += ToReturn;)
            }
            if (Format_Pos<Stream[StreamKind][StreamPos].size() && Stream[StreamKind][StreamPos][Parameter].empty() && Parameter==File__Analyze::Fill_Parameter(StreamKind, Generic_Format_String))
                EXECUTE_STRING(Stream[StreamKind][StreamPos][Format_Pos], Debug += __T("Get, will return "); Debug+=ToReturn;)
            EXECUTE_STRING(Stream[StreamKind][StreamPos][Parameter], Debug += __T("Get, will return "); Debug+=ToReturn;)
        }
        else
            EXECUTE_STRING(MediaInfoLib::Config.EmptyString_Get(), Debug+=__T("Get, will return ");Debug+=ToReturn;) //This parameter is known, but not filled
    }
    else
    {
        #if MEDIAINFO_ADVANCED
        if (KindOfInfo==Info_Text && Config.File_ChannelLayout_Get())
        #endif //MEDIAINFO_ADVANCED
        {
            size_t Format_Pos=File__Analyze::Fill_Parameter(StreamKind, Generic_Format);
            Ztring ToReturn;
            bool ShouldReturn=false;
            if (Format_Pos<Stream[StreamKind][StreamPos].size())
                ToReturn=ChannelLayout_2018_Rename(StreamKind, Stream_More[StreamKind][StreamPos][Parameter-MediaInfoLib::Config.Info_Get(StreamKind).size()][Info_Name], Stream_More[StreamKind][StreamPos][Parameter-MediaInfoLib::Config.Info_Get(StreamKind).size()](KindOfInfo), Stream[StreamKind][StreamPos][Format_Pos], ShouldReturn);
            if (ShouldReturn)
                EXECUTE_STRING(ToReturn, Debug+=__T("Get, will return ");Debug+=ToReturn;)
        }
        EXECUTE_STRING(Stream_More[StreamKind][StreamPos][Parameter-MediaInfoLib::Config.Info_Get(StreamKind).size()](KindOfInfo), Debug+=__T("Get, will return ");Debug+=ToReturn;)
    }
}

//---------------------------------------------------------------------------
Ztring MediaInfo_Internal::Get(stream_t StreamKind, size_t StreamPos, const String &Parameter, info_t KindOfInfo, info_t KindOfSearch)
{
    //Legacy
    if (Parameter.find(__T("_String"))!=Error)
    {
        Ztring S1=Parameter;
        S1.FindAndReplace(__T("_String"), __T("/String"));
        return Get(StreamKind, StreamPos, S1, KindOfInfo, KindOfSearch);
    }
    if (Parameter==__T("Channels"))
        return Get(StreamKind, StreamPos, __T("Channel(s)"), KindOfInfo, KindOfSearch);
    if (Parameter==__T("Artist"))
        return Get(StreamKind, StreamPos, __T("Performer"), KindOfInfo, KindOfSearch);
    if (Parameter==__T("AspectRatio"))
        return Get(StreamKind, StreamPos, __T("DisplayAspectRatio"), KindOfInfo, KindOfSearch);
    if (Parameter==__T("AspectRatio/String"))
        return Get(StreamKind, StreamPos, __T("DisplayAspectRatio/String"), KindOfInfo, KindOfSearch);
    if (Parameter==__T("Choregrapher"))
        return Get(StreamKind, StreamPos, __T("Choreographer"), KindOfInfo, KindOfSearch);
    if (Parameter==__T("Chroma"))
        return Get(StreamKind, StreamPos, __T("Colorimetry"), KindOfInfo, KindOfSearch);
    if (Parameter==__T("PlayTime"))
        return Get(StreamKind, StreamPos, __T("Duration"), KindOfInfo, KindOfSearch);
    if (Parameter==__T("PlayTime/String"))
        return Get(StreamKind, StreamPos, __T("Duration/String"), KindOfInfo, KindOfSearch);
    if (Parameter==__T("PlayTime/String1"))
        return Get(StreamKind, StreamPos, __T("Duration/String1"), KindOfInfo, KindOfSearch);
    if (Parameter==__T("PlayTime/String2"))
        return Get(StreamKind, StreamPos, __T("Duration/String2"), KindOfInfo, KindOfSearch);
    if (Parameter==__T("PlayTime/String3"))
        return Get(StreamKind, StreamPos, __T("Duration/String3"), KindOfInfo, KindOfSearch);
    if (StreamKind==Stream_General && Parameter==__T("BitRate"))
        return Get(Stream_General, StreamPos, __T("OverallBitRate"), KindOfInfo, KindOfSearch);
    if (StreamKind==Stream_General && Parameter==__T("BitRate/String"))
        return Get(Stream_General, StreamPos, __T("OverallBitRate/String"), KindOfInfo, KindOfSearch);
    if (StreamKind==Stream_General && Parameter==__T("BitRate_Minimum"))
        return Get(Stream_General, StreamPos, __T("OverallBitRate_Minimum"), KindOfInfo, KindOfSearch);
    if (StreamKind==Stream_General && Parameter==__T("BitRate_Minimum/String"))
        return Get(Stream_General, StreamPos, __T("OverallBitRate_Minimum/String"), KindOfInfo, KindOfSearch);
    if (StreamKind==Stream_General && Parameter==__T("BitRate_Nominal"))
        return Get(Stream_General, StreamPos, __T("OverallBitRate_Nominal"), KindOfInfo, KindOfSearch);
    if (StreamKind==Stream_General && Parameter==__T("BitRate_Nominal/String"))
        return Get(Stream_General, StreamPos, __T("OverallBitRate_Nominal/String"), KindOfInfo, KindOfSearch);
    if (StreamKind==Stream_General && Parameter==__T("BitRate_Maximum"))
        return Get(Stream_General, StreamPos, __T("OverallBitRate_Maximum"), KindOfInfo, KindOfSearch);
    if (StreamKind==Stream_General && Parameter==__T("BitRate_Maximum/String"))
        return Get(Stream_General, StreamPos, __T("OverallBitRate_Maximum/String"), KindOfInfo, KindOfSearch);
    if (Parameter==__T("AFD"))
        return Get(StreamKind, StreamPos, __T("ActiveFormatDescription"), KindOfInfo, KindOfSearch);
    if (Parameter==__T("Encoded_Application") && Info && !Info->Retrieve(StreamKind, StreamPos, "Encoded_Application/String").empty())
        return Get(StreamKind, StreamPos, __T("Encoded_Application/String"), KindOfInfo, KindOfSearch);
    if (Parameter==__T("Encoded_Library") && Info && !Info->Retrieve(StreamKind, StreamPos, "Encoded_Library/String").empty())
        return Get(StreamKind, StreamPos, __T("Encoded_Library/String"), KindOfInfo, KindOfSearch);
    if (Parameter==__T("Encoded_Library/String") && !MediaInfoLib::Config.ReadByHuman_Get())
    {
        //TODO: slight duplicate of content in Streams_Finish_HumanReadable_PerStream, should be refactorized
        Ztring CompanyName=Get(StreamKind, StreamPos, __T("Encoded_Library_CompanyName"));
        Ztring Name=Get(StreamKind, StreamPos, __T("Encoded_Library_Name"));
        Ztring Version=Get(StreamKind, StreamPos, __T("Encoded_Library_Version"));
        Ztring Date=Get(StreamKind, StreamPos, __T("Encoded_Library_Date"));
        Ztring Encoded_Library=Get(StreamKind, StreamPos, __T("Encoded_Library"));
        return File__Analyze_Encoded_Library_String(CompanyName, Name, Version, Date, Encoded_Library);
    }
    if (!Parameter.compare(0, 11, __T("DolbyVision")))
    {
        Ztring Temp=Get(StreamKind, StreamPos, __T("HDR_Format")+Parameter.substr(11), KindOfInfo, KindOfSearch).substr(14);
        return Temp.substr(0, Temp.find(__T(" / ")));
    }

    CS.Enter();
    MEDIAINFO_DEBUG_CONFIG_TEXT(Debug+=__T("Get, StreamKind=");Debug+=Ztring::ToZtring((size_t)StreamKind);Debug+=__T(", StreamKind=");Debug+=Ztring::ToZtring(StreamPos);Debug+=__T(", Parameter=");Debug+=Ztring(Parameter);)

    if (Info && Info->Status[File__Analyze::IsUpdated])
    {
        Info->Open_Buffer_Update();
        Info->Status[File__Analyze::IsUpdated]=false;
        for (size_t Pos=File__Analyze::User_16; Pos<File__Analyze::User_16+16; Pos++)
            Info->Status[Pos]=false;
    }

    //Check integrity
    if (StreamKind>=Stream_Max || StreamPos>=Stream[StreamKind].size() || KindOfInfo>=Info_Max)
    {
        CS.Leave();
        EXECUTE_STRING(MediaInfoLib::Config.EmptyString_Get(), Debug+=__T("Get, stream and/or pos is invalid will return empty string");) //Parameter is unknown
    }

    //Special cases
    //-Inform for a stream
#if defined(MEDIAINFO_TEXT_YES) || defined(MEDIAINFO_HTML_YES) || defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_CSV_YES) || defined(MEDIAINFO_CUSTOM_YES)
    if (Parameter==__T("Inform"))
    {
        CS.Leave();
        const Ztring InformZtring=Inform(StreamKind, StreamPos, true);
        CS.Enter();
        size_t Pos=MediaInfoLib::Config.Info_Get(StreamKind).Find(__T("Inform"));
        if (Pos!=Error)
            Stream[StreamKind][StreamPos](Pos)=InformZtring;
    }
#endif

    //Case of specific info
    size_t ParameterI=MediaInfoLib::Config.Info_Get(StreamKind).Find(Parameter, KindOfSearch);
    if (ParameterI==Error)
    {
        ParameterI=Stream_More[StreamKind][StreamPos].Find(Parameter, KindOfSearch);
        if (ParameterI==Error)
        {
            #ifdef MEDIAINFO_DEBUG_WARNING_GET
                if (Ztring(Parameter)!=__T("SCTE35_PID")) //TODO: define a special interface for parser-specific parameters
                    std::cerr<<"MediaInfo: Warning, Get(), parameter \""<<Ztring(Parameter).To_Local()<<"\""<<std::endl;
            #endif //MEDIAINFO_DEBUG_WARNING_GET

            CS.Leave();
            EXECUTE_STRING(MediaInfoLib::Config.EmptyString_Get(), Debug+=__T("Get, parameter is unknown, will return empty string");) //Parameter is unknown
        }
        CS.Leave();
        CriticalSectionLocker CSL(CS);
        #if MEDIAINFO_ADVANCED
        if (KindOfInfo==Info_Text && Config.File_ChannelLayout_Get())
        #endif //MEDIAINFO_ADVANCED
        {
            size_t Format_Pos=File__Analyze::Fill_Parameter(StreamKind, Generic_Format);
            Ztring ToReturn;
            bool ShouldReturn=false;
            if (Format_Pos<Stream[StreamKind][StreamPos].size())
                ToReturn=ChannelLayout_2018_Rename(StreamKind, Stream_More[StreamKind][StreamPos][ParameterI][Info_Name], Stream_More[StreamKind][StreamPos][ParameterI](KindOfInfo), Stream[StreamKind][StreamPos][Format_Pos], ShouldReturn);
            if (ShouldReturn)
                EXECUTE_STRING(ToReturn, Debug+=__T("Get, will return ");Debug+=ToReturn;)
        }
        EXECUTE_STRING(Stream_More[StreamKind][StreamPos][ParameterI](KindOfInfo), Debug+=__T("Get, will return ");Debug+=ToReturn;)
    }

    CS.Leave();

    EXECUTE_STRING(Get(StreamKind, StreamPos, ParameterI, KindOfInfo), Debug+=__T("Get, will return ");Debug+=ToReturn;)
}

//***************************************************************************
// Set File info
//***************************************************************************

//---------------------------------------------------------------------------
size_t MediaInfo_Internal::Set(const String &ToSet, stream_t StreamKind, size_t StreamPos, size_t Parameter, const String &OldValue)
{
    CriticalSectionLocker CSL(CS);
    if (!Info)
        return 0;

    return Info->Set(StreamKind, StreamPos, Parameter, ToSet, OldValue);
}

//---------------------------------------------------------------------------
size_t MediaInfo_Internal::Set(const String &ToSet, stream_t StreamKind, size_t StreamPos, const String &Parameter, const String &OldValue)
{
    CriticalSectionLocker CSL(CS);
    if (!Info)
        return 0;

    return Info->Set(StreamKind, StreamPos, Parameter, ToSet, OldValue);
}

//***************************************************************************
// Output buffer
//***************************************************************************

//---------------------------------------------------------------------------
size_t MediaInfo_Internal::Output_Buffer_Get (const String &Value)
{
    CriticalSectionLocker CSL(CS);
    if (!Info)
        return 0;

    MEDIAINFO_DEBUG_OUTPUT_VALUE(Value, Info->Output_Buffer_Get(Value));
}

//---------------------------------------------------------------------------
size_t MediaInfo_Internal::Output_Buffer_Get (size_t Pos)
{
    CriticalSectionLocker CSL(CS);
    if (!Info)
        return 0;

    MEDIAINFO_DEBUG_OUTPUT_POS(Pos, Info->Output_Buffer_Get(Pos));
}

//***************************************************************************
// Information
//***************************************************************************

//---------------------------------------------------------------------------
String MediaInfo_Internal::Option (const String &Option, const String &Value)
{
    CriticalSectionLocker CSL(CS);
    MEDIAINFO_DEBUG_CONFIG_TEXT(Debug+=__T("Option, Option=");Debug+=Ztring(Option);Debug+=__T(", Value=");Debug+=Ztring(Value);)
    Ztring OptionLower=Option; OptionLower.MakeLowerCase();
    if (Option.empty())
        return String();
    else if (OptionLower==__T("language_update"))
    {
        if (!Info || Info->Get(Stream_General, 0, __T("CompleteName"))==__T(""))
            return __T("");

        ZtringListList Language=Value.c_str();
        MediaInfoLib::Config.Language_Set(Language);

        return __T("");
    }
    else if (OptionLower==__T("create_dummy"))
    {
        CreateDummy (Value);
        delete Info; Info=NULL;
        return __T("");
    }
    else if (OptionLower==__T("thread"))
    {
        BlockMethod=1;
        return __T("");
    }
    else if (Option==__T("info_capacities"))
    {
        return __T("Option removed");
    }
    #if MEDIAINFO_TRACE
    else if (OptionLower.find(__T("file_details_clear"))==0)
    {
        if (Info)
            Info->Details_Clear();

        return __T("");
    }
    #endif //MEDIAINFO_TRACE
    else if (OptionLower.find(__T("file_seek"))==0)
    {
        #if MEDIAINFO_SEEK
            #if !defined(MEDIAINFO_READER_NO)
                if (Reader==NULL && Info==NULL)
                    return __T("Error: Reader pointer is empty");
            #endif //MEDIAINFO_READER_NO

            size_t Method=(size_t)-1;
            int64u SeekValue=(int64u)-1;
            int64u ID=(int64u)-1;

            ZtringList List; List.Separator_Set(0, __T(","));
            List.Write(Value);
            for (size_t Pos=0; Pos<List.size(); Pos++)
            {
                if (!List[Pos].empty() && List[Pos].find(__T('%'))==List[Pos].size()-1)
                {
                    Method=1;
                    SeekValue=(int64u)(Ztring(List[Pos]).To_float32()*100);
                }
                else if (!List[Pos].empty() && List[Pos].find_first_not_of(__T("0123456789"))==string::npos)
                {
                    Method=0;
                    SeekValue=Ztring(List[Pos]).To_int64u();
                }
                else if (!List[Pos].empty() && List[Pos].find(__T("Frame="))!=string::npos)
                {
                    Method=3;
                    Ztring FrameNumberZ=List[Pos].substr(List[Pos].find(__T("Frame="))+6, string::npos);
                    SeekValue=FrameNumberZ.To_int64u();
                }
                else if (!List[Pos].empty() && List[Pos].find(__T(':'))!=string::npos)
                {
                    Method=2;
                    Ztring ValueZ=List[Pos];
                    SeekValue=0;
                    size_t Value_Pos=ValueZ.find(__T(':'));
                    if (Value_Pos==string::npos)
                        Value_Pos=ValueZ.size();
                    SeekValue+=Ztring(ValueZ.substr(0, Value_Pos)).To_int64u()*60*60*1000*1000*1000;
                    ValueZ.erase(0, Value_Pos+1);
                    Value_Pos=ValueZ.find(__T(':'));
                    if (Value_Pos==string::npos)
                        Value_Pos=ValueZ.size();
                    SeekValue+=Ztring(ValueZ.substr(0, Value_Pos)).To_int64u()*60*1000*1000*1000;
                    ValueZ.erase(0, Value_Pos+1);
                    Value_Pos=ValueZ.find(__T('.'));
                    if (Value_Pos==string::npos)
                        Value_Pos=ValueZ.size();
                    SeekValue+=Ztring(ValueZ.substr(0, Value_Pos)).To_int64u()*1000*1000*1000;
                    ValueZ.erase(0, Value_Pos+1);
                    if (!ValueZ.empty())
                        SeekValue+=Ztring(ValueZ).To_int64u()*1000*1000*1000/(int64u)pow(10.0, (int)ValueZ.size());
                }
                else if (!List[Pos].empty() && List[Pos].find(__T("ID="))!=string::npos)
                {
                    Ztring IDZ=List[Pos].substr(List[Pos].find(__T("ID="))+3, string::npos);
                    ID=IDZ.To_int64u();
                }
            }

            CS.Leave();
            size_t Result;
            #if !defined(MEDIAINFO_READER_NO)
                if (Reader)
                    Result=Reader->Format_Test_PerParser_Seek(this, Method, SeekValue, ID);
                else
            #endif //MEDIAINFO_READER_NO
                    Result=Open_Buffer_Seek(Method, SeekValue, ID);
            CS.Enter();
            switch (Result)
            {
                case 1  : return __T("");
                case 2  : return __T("Invalid value");
                #if MEDIAINFO_IBIUSAGE
                case 3  : return __T("Feature not supported / IBI file not provided");
                case 4  : return __T("Problem during IBI file parsing");
                #endif //MEDIAINFO_IBIUSAGE
                case 5  : return __T("Invalid ID");
                case 6  : return __T("Internal error");
                #if !MEDIAINFO_IBIUSAGE
                case (size_t)-2 : return __T("Feature not supported / IBI support disabled due to compilation options");
                #endif //MEDIAINFO_IBIUSAGE
                case (size_t)-1 : return __T("Feature not supported");
                default : return __T("Unknown error");
            }
        #else //MEDIAINFO_SEEK
            return __T("Seek manager is disabled due to compilation options");
        #endif //MEDIAINFO_SEEK
    }
    #if MEDIAINFO_TRACE
        if (OptionLower.find(__T("file_details_stringpointer")) == 0 && MediaInfoLib::Config.Inform_Get()!=__T("MAXML") && (MediaInfoLib::Config.Trace_Level_Get() || MediaInfoLib::Config.Inform_Get()==__T("Details")) && !Details.empty())
        {
            return Ztring::ToZtring((int64u)Details.data())+__T(':')+Ztring::ToZtring((int64u)Details.size());
        }
    #endif //MEDIAINFO_TRACE
    #if MEDIAINFO_ADVANCED
        if (OptionLower.find(__T("file_inform_stringpointer")) == 0)
        {
            Inform_Cache = Inform(this).To_UTF8();
            #if MEDIAINFO_COMPRESS
                if (Value.find(__T("zlib"))==0)
                {
                    uLongf Compressed_Size=(uLongf)(Inform_Cache.size() + 16);
                    Bytef* Compressed=new Bytef[Inform_Cache.size()+16];
                    if (compress(Compressed, &Compressed_Size, (const Bytef*)Inform_Cache.c_str(), (uLong)Inform_Cache.size()) < 0)
                    {
                        delete[] Compressed;
                        return __T("Error during zlib compression");
                    }
                    Inform_Cache.assign((char*)Compressed, (size_t)Compressed_Size);
                    if (Value.find(__T("+base64"))+7==Value.size())
                    {
                        Inform_Cache=Base64::encode(Inform_Cache);
                    }
                }
            #endif //MEDIAINFO_COMPRESS
            return Ztring::ToZtring((int64u)Inform_Cache.data()) + __T(':') + Ztring::ToZtring((int64u)Inform_Cache.size());
        }
    #endif //MEDIAINFO_ADVANCED
    else if (OptionLower.find(__T("reset"))==0)
    {
        MediaInfoLib::Config.Init(true);
        return Ztring();
    }
    else if (OptionLower.find(__T("file_"))==0)
    {
        Ztring ToReturn2=Config.Option(Option, Value);
        if (Info)
            Info->Option_Manage();

        MEDIAINFO_DEBUG_OUTPUT_INIT(ToReturn2, Debug+=__T("Option, will return ");Debug+=ToReturn;)
    }
    else
        EXECUTE_STRING(MediaInfoLib::Config.Option(Option, Value), Debug+=__T("Option, will return ");Debug+=ToReturn;)
}

//---------------------------------------------------------------------------
size_t MediaInfo_Internal::Count_Get (stream_t StreamKind, size_t StreamPos)
{
    CriticalSectionLocker CSL(CS);

    if (Info && Info->Status[File__Analyze::IsUpdated])
    {
        Info->Open_Buffer_Update();
        Info->Status[File__Analyze::IsUpdated]=false;
        for (size_t Pos=File__Analyze::User_16; Pos<File__Analyze::User_16+16; Pos++)
            Info->Status[Pos]=false;
    }

    //Integrity
    if (StreamKind>=Stream_Max)
        return 0;

    //Count of streams
    if (StreamPos==Error)
        return Stream[StreamKind].size();

    //Integrity
    if (StreamPos>=Stream[StreamKind].size())
        return 0;

    //Count of piece of information in a stream
    return MediaInfoLib::Config.Info_Get(StreamKind).size()+Stream_More[StreamKind][StreamPos].size();
}

//---------------------------------------------------------------------------
size_t MediaInfo_Internal::State_Get ()
{
    return (size_t)(Config.State_Get()*10000);
}

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_FILE_YES)
void MediaInfo_Internal::TestContinuousFileNames ()
{
    CriticalSectionLocker CSL(CS);
    if (Info)
        Info->TestContinuousFileNames();
}
#endif //defined(MEDIAINFO_FILE_YES)

//---------------------------------------------------------------------------
#if MEDIAINFO_EVENTS
void MediaInfo_Internal::Event_Prepare (struct MediaInfo_Event_Generic* Event, int32u Event_Code, size_t Event_Size)
{
    CriticalSectionLocker CSL(CS);
    if (Info)
        Info->Event_Prepare(Event, Event_Code, Event_Size);
}
#endif // MEDIAINFO_EVENTS

//---------------------------------------------------------------------------
Ztring MediaInfo_Internal::Inform(MediaInfo_Internal* Info)
{
    std::vector<MediaInfoLib::MediaInfo_Internal*> Info2;
    Info2.push_back(Info);
    return MediaInfoLib::MediaInfo_Internal::Inform(Info2);
}

//---------------------------------------------------------------------------
Ztring MediaInfo_Internal::Inform(std::vector<MediaInfo_Internal*>& Info)
{
    Ztring Result;

    #if defined(MEDIAINFO_XML_YES)
    if (MediaInfoLib::Config.Inform_Get()==__T("MAXML"))
    {
        Result+=__T("<?xml version=\"1.0\" encoding=\"UTF-8\"?>")+MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T('<');
        Result+=__T("MediaArea");
        Result+=MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T("    xmlns=\"http")+(MediaInfoLib::Config.Https_Get()?Ztring(__T("s")):Ztring())+__T("://mediaarea.net/mediaarea\"");
        Result+=MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T("    xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"");
        Result+=MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T("    xsi:schemaLocation=\"http")+(MediaInfoLib::Config.Https_Get()?Ztring(__T("s")):Ztring())+__T("://mediaarea.net/mediaarea http")+(MediaInfoLib::Config.Https_Get()?Ztring(__T("s")):Ztring())+__T("://mediaarea.net/mediaarea/mediaarea_0_1.xsd\"");
        Result+=MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T("    version=\"0.1\"");
        Result+=__T(">")+MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T("<creatingLibrary version=\"")+Ztring(MediaInfo_Version).SubString(__T(" - v"), Ztring())+__T("\" url=\"http")+(MediaInfoLib::Config.Https_Get()?Ztring(__T("s")):Ztring())+__T("://mediaarea.net/MediaInfo\">MediaInfoLib</creatingLibrary>");
        Result+=MediaInfoLib::Config.LineSeparator_Get();

        for (size_t FilePos=0; FilePos<Info.size(); FilePos++)
            Result+=Info[FilePos]->Inform();

        if (!Result.empty() && Result[Result.size()-1]!=__T('\r') && Result[Result.size()-1]!=__T('\n'))
            Result+=MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T("</MediaArea");
        Result+=__T(">")+MediaInfoLib::Config.LineSeparator_Get();
    }

    else if (MediaInfoLib::Config.Trace_Level_Get() && MediaInfoLib::Config.Trace_Format_Get()==MediaInfoLib::Config.Trace_Format_XML)
    {
        Result+=__T("<?xml version=\"1.0\" encoding=\"UTF-8\"?>")+MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T('<');
        Result+=__T("MediaTrace");
        Result+=MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T("    xmlns=\"http")+(MediaInfoLib::Config.Https_Get()?Ztring(__T("s")):Ztring())+__T("://mediaarea.net/mediatrace\"");
        Result+=MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T("    xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"");
        Result+=MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T("    xsi:schemaLocation=\"http")+(MediaInfoLib::Config.Https_Get()?Ztring(__T("s")):Ztring())+__T("://mediaarea.net/mediatrace http")+(MediaInfoLib::Config.Https_Get()?Ztring(__T("s")):Ztring())+__T("://mediaarea.net/mediatrace/mediatrace_0_2.xsd\"");
        Result+=MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T("    version=\"0.2\"");
        Result+=__T(">")+MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T("<creatingLibrary version=\"")+Ztring(MediaInfo_Version).SubString(__T(" - v"), Ztring())+__T("\" url=\"http")+(MediaInfoLib::Config.Https_Get()?Ztring(__T("s")):Ztring())+__T("://mediaarea.net/MediaInfo\">MediaInfoLib</creatingLibrary>");
        Result+=MediaInfoLib::Config.LineSeparator_Get();

        for (size_t FilePos=0; FilePos<Info.size(); FilePos++)
        {
            size_t Modified;
            Result+=__T("<media");
            Ztring Options=Info[FilePos]->Get(Stream_General, 0, General_CompleteName, Info_Options);
            if (InfoOption_ShowInInform<Options.size() && Options[InfoOption_ShowInInform]==__T('Y'))
                Result+=__T(" ref=\"")+MediaInfo_Internal::Xml_Content_Escape(Info[FilePos]->Get(Stream_General, 0, General_CompleteName), Modified)+__T("\"");
            if (Info[FilePos] && !Info[FilePos]->ParserName.empty())
                Result+=__T(" parser=\"")+Info[FilePos]->ParserName+=__T("\"");
            Result+= __T('>');
            Result+=MediaInfoLib::Config.LineSeparator_Get();
            Result+=Info[FilePos]->Inform();
            if (!Result.empty() && Result[Result.size()-1]!=__T('\r') && Result[Result.size()-1]!=__T('\n'))
                Result+=MediaInfoLib::Config.LineSeparator_Get();
            Result+=__T("</media>");
            Result+=MediaInfoLib::Config.LineSeparator_Get();
        }

        if (!Result.empty() && Result[Result.size()-1]!=__T('\r') && Result[Result.size()-1]!=__T('\n'))
            Result+=MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T("</MediaTrace");
        Result+=__T(">")+MediaInfoLib::Config.LineSeparator_Get();
    }

    else if (MediaInfoLib::Config.Trace_Level_Get() && MediaInfoLib::Config.Trace_Format_Get()==MediaInfoLib::Config.Trace_Format_MICRO_XML)
    {
        Result+=__T("<?xml version=\"1.0\" encoding=\"UTF-8\"?>")+MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T('<');
        Result+=__T("MicroMediaTrace");
        Result+=__T(" xmlns=\"http")+(MediaInfoLib::Config.Https_Get()?Ztring(__T("s")):Ztring())+__T("://mediaarea.net/micromediatrace\"");
        Result+=__T(" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"");
        Result+=__T(" mtsl=\"http")+(MediaInfoLib::Config.Https_Get()?Ztring(__T("s")):Ztring())+__T("://mediaarea.net/micromediatrace http")+(MediaInfoLib::Config.Https_Get()?Ztring(__T("s")):Ztring())+__T("://mediaarea.net/micromediatrace/micromediatrace.xsd\"");
        Result+=__T(" version=\"0.1\">");
        Result+=__T("<creatingLibrary version=\"")+Ztring(MediaInfo_Version).SubString(__T(" - v"), Ztring())+__T("\" url=\"http")+(MediaInfoLib::Config.Https_Get()?Ztring(__T("s")):Ztring())+__T("://mediaarea.net/MediaInfo\">MediaInfoLib</creatingLibrary>");

        for (size_t FilePos=0; FilePos<Info.size(); FilePos++)
        {
            size_t Modified;
            Result+=__T("<media");
            Ztring Options=Info[FilePos]->Get(Stream_General, 0, General_CompleteName, Info_Options);
            if (InfoOption_ShowInInform<Options.size() && Options[InfoOption_ShowInInform]==__T('Y'))
                Result+=__T(" ref=\"")+MediaInfo_Internal::Xml_Content_Escape(Info[FilePos]->Get(Stream_General, 0, General_CompleteName), Modified)+__T("\"");
            if (Info[FilePos] && !Info[FilePos]->ParserName.empty())
                Result+=__T(" parser=\"")+Info[FilePos]->ParserName+=__T("\"");
            Result+= __T('>');
            Result+=Info[FilePos]->Inform();
            Result+=__T("</media>");
        }

        Result+=__T("</MicroMediaTrace>");
    }

    else if (MediaInfoLib::Config.Inform_Get().MakeLowerCase()==__T("timecodexml"))
    {
        Result+=__T("<?xml version=\"1.0\" encoding=\"UTF-8\"?>")+MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T('<');
        Result+=__T("MediaTimecode");
        Result+=MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T("    xmlns=\"http")+(MediaInfoLib::Config.Https_Get()?Ztring(__T("s")):Ztring())+__T("://mediaarea.net/mediatimecode\"");
        Result+=MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T("    xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"");
        Result+=MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T("    xsi:schemaLocation=\"http")+(MediaInfoLib::Config.Https_Get()?Ztring(__T("s")):Ztring())+__T("://mediaarea.net/mediatimecode http")+(MediaInfoLib::Config.Https_Get()?Ztring(__T("s")):Ztring())+__T("://mediaarea.net/xsd/mediatimecode.xsd\"");
        Result+=MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T("    version=\"0.0\"");
        Result+=__T(">")+MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T("<creatingLibrary version=\"")+Ztring(MediaInfo_Version).SubString(__T(" - v"), Ztring())+__T("\" url=\"http")+(MediaInfoLib::Config.Https_Get()?Ztring(__T("s")):Ztring())+__T("://mediaarea.net/MediaInfo\">MediaInfoLib</creatingLibrary>");
        Result+=MediaInfoLib::Config.LineSeparator_Get();

        for (size_t FilePos=0; FilePos<Info.size(); FilePos++)
            Result+=Info[FilePos]->Inform();

        if (!Result.empty() && Result[Result.size()-1]!=__T('\r') && Result[Result.size()-1]!=__T('\n'))
            Result+=MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T("</MediaTimecode");
        Result+=__T(">")+MediaInfoLib::Config.LineSeparator_Get();
    }

    else if (MediaInfoLib::Config.Inform_Get()==__T("XML") || MediaInfoLib::Config.Inform_Get()==__T("MIXML"))
    {
        Result+=__T("<?xml version=\"1.0\" encoding=\"UTF-8\"?>")+MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T('<');
        Result+=__T("MediaInfo");
        Result+=MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T("    xmlns=\"http")+(MediaInfoLib::Config.Https_Get()?Ztring(__T("s")):Ztring())+__T("://mediaarea.net/mediainfo\"");
        Result+=MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T("    xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"");
        Result+=MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T("    xsi:schemaLocation=\"http")+(MediaInfoLib::Config.Https_Get()?Ztring(__T("s")):Ztring())+__T("://mediaarea.net/mediainfo http")+(MediaInfoLib::Config.Https_Get()?Ztring(__T("s")):Ztring())+__T("://mediaarea.net/mediainfo/mediainfo_2_0.xsd\"");
        Result+=MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T("    version=\"2.0\"");
        Result+=__T(">")+MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T("<creatingLibrary version=\"")+Ztring(MediaInfo_Version).SubString(__T(" - v"), Ztring())+__T("\" url=\"http")+(MediaInfoLib::Config.Https_Get()?Ztring(__T("s")):Ztring())+__T("://mediaarea.net/MediaInfo\">MediaInfoLib</creatingLibrary>");
        Result+=MediaInfoLib::Config.LineSeparator_Get();

        for (size_t FilePos=0; FilePos<Info.size(); FilePos++)
            Result+=Info[FilePos]->Inform();

        if (!Result.empty() && Result[Result.size()-1]!=__T('\r') && Result[Result.size()-1]!=__T('\n'))
            Result+=MediaInfoLib::Config.LineSeparator_Get();
        Result+=__T("</MediaInfo");
        Result+=__T(">")+MediaInfoLib::Config.LineSeparator_Get();
    }
    else
    #endif //defined(MEDIAINFO_XML_YES)
    #if defined(MEDIAINFO_JSON_YES)
    if (MediaInfoLib::Config.Inform_Get()==__T("JSON"))
    {
        if (Info.size() > 1)
            Result+=__T("[")+MediaInfoLib::Config.LineSeparator_Get();
        for (size_t FilePos=0; FilePos<Info.size(); FilePos++)
        {
            Result+=Info[FilePos]->Inform();

            if (FilePos < Info.size() -1)
                Result+=__T(",");

            Result+=MediaInfoLib::Config.LineSeparator_Get();
        }
        if (Info.size() > 1)
            Result+=__T("]")+MediaInfoLib::Config.LineSeparator_Get();
    }
    else
    #endif //defined(MEDIAINFO_JSON_YES)
    {
        size_t FilePos=0;
        ZtringListList MediaInfo_Custom_View; MediaInfo_Custom_View.Write(MediaInfoLib::Config.Option(__T("Inform_Get")));
        #if defined(MEDIAINFO_XML_YES)
        bool XML=false;
        if (MediaInfoLib::Config.Inform_Get()==__T("OLDXML"))
            XML=true;
        if (XML)
        {
            Result+=__T("<?xml version=\"1.0\" encoding=\"UTF-8\"?>")+MediaInfoLib::Config.LineSeparator_Get();
            Result+=__T("<Mediainfo version=\"")+MediaInfoLib::Config.Info_Version_Get().SubString(__T(" v"), Ztring())+__T("\">");
            Result+=MediaInfoLib::Config.LineSeparator_Get();
        }
        else
        #endif //defined(MEDIAINFO_XML_YES)
        Result+=MediaInfo_Custom_View("Page_Begin");
        while (FilePos<Info.size())
        {
            Result+=Info[FilePos]->Inform();
            if (FilePos<Info.size()-1)
            {
                Result+=MediaInfo_Custom_View("Page_Middle");
            }
            FilePos++;
        }
        #if defined(MEDIAINFO_XML_YES)
        if (XML)
        {
            if (!Result.empty() && Result[Result.size()-1]!=__T('\r') && Result[Result.size()-1]!=__T('\n'))
                Result+=MediaInfoLib::Config.LineSeparator_Get();
            Result+=__T("</");
            if (MediaInfoLib::Config.Trace_Format_Get()==MediaInfoLib::Config.Trace_Format_XML)
                Result+=__T("MediaTrace");
            else if (MediaInfoLib::Config.Trace_Format_Get()==MediaInfoLib::Config.Trace_Format_MICRO_XML)
                Result+=__T("MicroMediaTrace");
            else
                Result+=__T("Mediainfo");
            Result+=__T(">")+MediaInfoLib::Config.LineSeparator_Get();
        }
        else
        #endif //defined(MEDIAINFO_XML_YES)
            Result+=MediaInfo_Custom_View("Page_End");//
    }

    #if MEDIAINFO_COMPRESS
        bool zlib=MediaInfoLib::Config.FlagsX_Get(Flags_Inform_zlib);
        bool base64=MediaInfoLib::Config.FlagsX_Get(Flags_Inform_base64);
        if (zlib || base64)
        {
            string Inform_Cache = Result.To_UTF8();
            if (zlib)
            {
                uLongf Compressed_Size=(uLongf)(Inform_Cache.size() + 16);
                Bytef* Compressed=new Bytef[Inform_Cache.size()+16];
                if (compress(Compressed, &Compressed_Size, (const Bytef*)Inform_Cache.c_str(), (uLong)Inform_Cache.size()) < 0)
                {
                    delete[] Compressed;
                    return __T("Error during zlib compression");
                }
                Inform_Cache.assign((char*)Compressed, (size_t)Compressed_Size);
            }
            if (base64)
            {
                Inform_Cache=Base64::encode(Inform_Cache);
            }
            Result.From_UTF8(Inform_Cache);
        }
    #endif //MEDIAINFO_COMPRESS

    return Result.c_str();
}

} //NameSpace

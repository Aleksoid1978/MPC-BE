/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Global configuration of MediaInfo
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Pre-compilation
#include "MediaInfo/PreComp.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Debug
#ifdef MEDIAINFO_DEBUG
    #include <stdio.h>
    #include <windows.h>
    namespace MediaInfo_Config_Debug
    {
        FILE* F;
        std::string Debug;
        SYSTEMTIME st_In;

        void Debug_Open(bool Out)
        {
            F=fopen("C:\\Temp\\MediaInfo_Debug.txt", "a+t");
            Debug.clear();
            SYSTEMTIME st;
            GetLocalTime( &st );

            char Duration[100];
            if (Out)
            {
                FILETIME ft_In;
                if (SystemTimeToFileTime(&st_In, &ft_In))
                {
                    FILETIME ft_Out;
                    if (SystemTimeToFileTime(&st, &ft_Out))
                    {
                        ULARGE_INTEGER UI_In;
                        UI_In.HighPart=ft_In.dwHighDateTime;
                        UI_In.LowPart=ft_In.dwLowDateTime;

                        ULARGE_INTEGER UI_Out;
                        UI_Out.HighPart=ft_Out.dwHighDateTime;
                        UI_Out.LowPart=ft_Out.dwLowDateTime;

                        ULARGE_INTEGER UI_Diff;
                        UI_Diff.QuadPart=UI_Out.QuadPart-UI_In.QuadPart;

                        FILETIME ft_Diff;
                        ft_Diff.dwHighDateTime=UI_Diff.HighPart;
                        ft_Diff.dwLowDateTime=UI_Diff.LowPart;

                        SYSTEMTIME st_Diff;
                        if (FileTimeToSystemTime(&ft_Diff, &st_Diff))
                        {
                            sprintf(Duration, "%02hd:%02hd:%02hd.%03hd", st_Diff.wHour, st_Diff.wMinute, st_Diff.wSecond, st_Diff.wMilliseconds);
                        }
                        else
                            strcpy(Duration, "            ");
                    }
                    else
                        strcpy(Duration, "            ");

                }
                else
                    strcpy(Duration, "            ");
            }
            else
            {
                st_In=st;
                strcpy(Duration, "            ");
            }

            fprintf(F,"                                       %02hd:%02hd:%02hd.%03hd %s", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, Duration);
        }

        void Debug_Close()
        {
            Debug += "\r\n";
            fwrite(Debug.c_str(), Debug.size(), 1, F); \
            fclose(F);
        }
    }
    using namespace MediaInfo_Config_Debug;

    #define MEDIAINFO_DEBUG1(_NAME,_TOAPPEND) \
        Debug_Open(false); \
        Debug+=", ";Debug+=_NAME; \
        _TOAPPEND; \
        Debug_Close();

    #define MEDIAINFO_DEBUG2(_NAME,_TOAPPEND) \
        Debug_Open(true); \
        Debug+=", ";Debug+=_NAME; \
        _TOAPPEND; \
        Debug_Close();
#else // MEDIAINFO_DEBUG
    #define MEDIAINFO_DEBUG1(_NAME,_TOAPPEND)
    #define MEDIAINFO_DEBUG2(_NAME,_TOAPPEND)
#endif // MEDIAINFO_DEBUG

//---------------------------------------------------------------------------
#include "MediaInfo/Setup.h"
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#include "MediaInfo/MediaInfo_Config.h"
#include "ZenLib/ZtringListListF.h"
#if defined(MEDIAINFO_FILE_YES)
#include "ZenLib/File.h"
#endif //defined(MEDIAINFO_REFERENCES_YES)
#include <algorithm>
#if defined(MEDIAINFO_LIBCURL_YES)
    #include "MediaInfo/Reader/Reader_libcurl.h"
#endif //defined(MEDIAINFO_LIBCURL_YES)
#if defined(MEDIAINFO_GRAPHVIZ_YES)
    #include "MediaInfo/Export/Export_Graph.h"
#endif //defined(MEDIAINFO_GRAPHVIZ_YES)
#if defined(MEDIAINFO_EBUCORE_YES)
    #include "MediaInfo/Export/Export_EbuCore.h"
#endif //defined(MEDIAINFO_EBUCORE_YES)
#include <limits>
using namespace ZenLib;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
const Char*  MediaInfo_Version=__T("MediaInfoLib - v24.12");
const Char*  MediaInfo_Url=__T("http://MediaArea.net/MediaInfo");
      Ztring EmptyZtring;       //Use it when we can't return a reference to a true Ztring
const Ztring EmptyZtring_Const; //Use it when we can't return a reference to a true Ztring, const version
const ZtringListList EmptyZtringListList_Const; //Use it when we can't return a reference to a true ZtringListList, const version
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
extern const int16u Ztring_MacRoman[128]=
{
    0x00C4,
    0x00C5,
    0x00C7,
    0x00C9,
    0x00D1,
    0x00D6,
    0x00DC,
    0x00E1,
    0x00E0,
    0x00E2,
    0x00E4,
    0x00E3,
    0x00E5,
    0x00E7,
    0x00E9,
    0x00E8,
    0x00EA,
    0x00EB,
    0x00ED,
    0x00EC,
    0x00EE,
    0x00EF,
    0x00F1,
    0x00F3,
    0x00F2,
    0x00F4,
    0x00F6,
    0x00F5,
    0x00FA,
    0x00F9,
    0x00FB,
    0x00FC,
    0x2020,
    0x00B0,
    0x00A2,
    0x00A3,
    0x00A7,
    0x2022,
    0x00B6,
    0x00DF,
    0x00AE,
    0x00A9,
    0x2122,
    0x00B4,
    0x00A8,
    0x2260,
    0x00C6,
    0x00D8,
    0x221E,
    0x00B1,
    0x2264,
    0x2265,
    0x00A5,
    0x00B5,
    0x2202,
    0x2211,
    0x220F,
    0x03C0,
    0x222B,
    0x00AA,
    0x00BA,
    0x03A9,
    0x00E6,
    0x00F8,
    0x00BF,
    0x00A1,
    0x00AC,
    0x221A,
    0x0192,
    0x2248,
    0x2206,
    0x00AB,
    0x00BB,
    0x2026,
    0x00A0,
    0x00C0,
    0x00C3,
    0x00D5,
    0x0152,
    0x0153,
    0x2013,
    0x2014,
    0x201C,
    0x201D,
    0x2018,
    0x2019,
    0x00F7,
    0x25CA,
    0x00FF,
    0x0178,
    0x2044,
    0x20AC,
    0x2039,
    0x203A,
    0xFB01,
    0xFB02,
    0x2021,
    0x00B7,
    0x201A,
    0x201E,
    0x2030,
    0x00C2,
    0x00CA,
    0x00C1,
    0x00CB,
    0x00C8,
    0x00CD,
    0x00CE,
    0x00CF,
    0x00CC,
    0x00D3,
    0x00D4,
    0xF8FF,
    0x00D2,
    0x00DA,
    0x00DB,
    0x00D9,
    0x0131,
    0x02C6,
    0x02DC,
    0x00AF,
    0x02D8,
    0x02D9,
    0x02DA,
    0x00B8,
    0x02DD,
    0x02DB,
    0x02C7,
};

//---------------------------------------------------------------------------
void MediaInfo_Config_CodecID_General_Mpeg4   (InfoMap &Info);
void MediaInfo_Config_CodecID_Video_Matroska  (InfoMap &Info);
void MediaInfo_Config_CodecID_Video_Mpeg4     (InfoMap &Info);
void MediaInfo_Config_CodecID_Video_Ogg       (InfoMap &Info);
void MediaInfo_Config_CodecID_Video_Real      (InfoMap &Info);
void MediaInfo_Config_CodecID_Video_Riff      (InfoMap &Info);
void MediaInfo_Config_CodecID_Audio_Matroska  (InfoMap &Info);
void MediaInfo_Config_CodecID_Audio_Mpeg4     (InfoMap &Info);
void MediaInfo_Config_CodecID_Audio_Ogg       (InfoMap &Info);
void MediaInfo_Config_CodecID_Audio_Real      (InfoMap &Info);
void MediaInfo_Config_CodecID_Audio_Riff      (InfoMap &Info);
void MediaInfo_Config_CodecID_Text_Matroska   (InfoMap &Info);
void MediaInfo_Config_CodecID_Text_Mpeg4      (InfoMap &Info);
void MediaInfo_Config_CodecID_Text_Riff       (InfoMap &Info);
void MediaInfo_Config_CodecID_Other_Mpeg4     (InfoMap &Info);
void MediaInfo_Config_Codec                   (InfoMap &Info);
void MediaInfo_Config_DefaultLanguage         (Translation &Info);
void MediaInfo_Config_Iso639_1                (InfoMap &Info);
void MediaInfo_Config_Iso639_2                (InfoMap &Info);
void MediaInfo_Config_General                 (ZtringListList &Info);
void MediaInfo_Config_Video                   (ZtringListList &Info);
void MediaInfo_Config_Audio                   (ZtringListList &Info);
void MediaInfo_Config_Text                    (ZtringListList &Info);
void MediaInfo_Config_Other                   (ZtringListList &Info);
void MediaInfo_Config_Image                   (ZtringListList &Info);
void MediaInfo_Config_Menu                    (ZtringListList &Info);
void MediaInfo_Config_Summary                 (ZtringListList &Info);
void MediaInfo_Config_Format                  (InfoMap &Info);
void MediaInfo_Config_Library_DivX            (InfoMap &Info);
void MediaInfo_Config_Library_XviD            (InfoMap &Info);
void MediaInfo_Config_Library_MainConcept_Avc (InfoMap &Info);
void MediaInfo_Config_Library_VorbisCom       (InfoMap &Info);
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Output formats

static const char* OutputFormats_JSONFields[] =
{
    "name",
    "desc",
    "mime",
};
static const size_t output_formats_item_size = sizeof(OutputFormats_JSONFields) / sizeof(decltype(*OutputFormats_JSONFields));
typedef const char* output_formats_item[output_formats_item_size];
static output_formats_item OutputFormats[] =
{
    { "Text",                   "Text",                                                         "text/plain",       },
    { "HTML",                   "HTML",                                                         "text/html",        },
    { "XML",                    "MediaInfo XML",                                                "text/xml",         },
    { "JSON",                   "MediaInfo JSON",                                               "text/json",        },
    { "EBUCore_1.8_ps",         "EBUCore 1.8 (XML; acq. metadata: parameter then segment)",     "text/xml",         },
    { "EBUCore_1.8_sp",         "EBUCore 1.8 (XML; acq. metadata: segment then parameter)",     "text/xml",         },
    { "EBUCore_1.8_ps_JSON",    "EBUCore 1.8 (JSON; acq. metadata: parameter then segment)",    "text/json",        },
    { "EBUCore_1.8_sp_JSON",    "EBUCore 1.8 (JSON; acq. metadata: segment then parameter)",    "text/json",        },
    { "EBUCore_1.6",            "EBUCore 1.6",                                                  "text/xml",         },
    { "FIMS_1.3",               "FIMS 1.3",                                                     "text/xml",         },
    { "MPEG-7_Strict",          "MPEG-7 (Strict)",                                              "text/xml",         },
    { "MPEG-7_Relaxed",         "MPEG-7 (Relaxed)",                                             "text/xml",         },
    { "MPEG-7_Extended",        "MPEG-7 (Extended)",                                            "text/xml",         },
    { "PBCore_2.1",             "PBCore 2.1",                                                   "text/xml",         },
    { "PBCore_2.0",             "PBCore 2.0",                                                   "text/xml",         },
    { "PBCore_1.2",             "PBCore 1.2",                                                   "text/xml",         },
    { "NISO_Z39.87",            "NISO Z39.87",                                                  "text/xml",         },
};
static const size_t output_formats_size = sizeof(OutputFormats) / sizeof(decltype(*OutputFormats));

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
MediaInfo_Config Config;
//---------------------------------------------------------------------------

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

void MediaInfo_Config::Init(bool Force)
{
    {
        CriticalSectionLocker CSL(CS);
    //We use Init() instead of COnstructor because for some backends (like WxWidgets...) does NOT like constructor of static object with Unicode conversion

    //Test
    if (Force)
    {
        #if defined(MEDIAINFO_EBUCORE_YES) || defined(MEDIAINFO_NISO_YES) || MEDIAINFO_ADVANCED
            ExternalMetadata.clear();
            ExternalMetaDataConfig.clear();
        #endif //defined(MEDIAINFO_EBUCORE_YES) || defined(MEDIAINFO_NISO_YES) || MEDIAINFO_ADVANCED
        #ifdef MEDIAINFO_ADVANCED
            ParseOnlyKnownExtensions.clear();
        #endif
        Trace_Layers.reset();
        Trace_Modificators.clear();
        Version.clear();
        ColumnSeparator.clear();
        LineSeparator.clear();
        TagSeparator.clear();
        Quote.clear();
        DecimalPoint.clear();
        ThousandsPoint.clear();
        CarriageReturnReplace.clear();
        Language.clear();
        Custom_View.clear();
        Custom_View_Replace.clear();
        Container.clear();
        for (size_t Format=0; Format<InfoCodecID_Format_Max; Format++)
            for (size_t StreamKind=0; StreamKind<Stream_Max; StreamKind++)
                CodecID[Format][StreamKind].clear();
        Format.clear();
        Codec.clear();
        for (size_t Format=0; Format<InfoLibrary_Format_Max; Format++)
            Library[Format].clear();
        Iso639_1.clear();
        Iso639_2.clear();
        for (size_t StreamKind=0; StreamKind<Stream_Max; StreamKind++)
            Info[StreamKind].clear();
        SubFile_Config.clear();
        CustomMapping.clear();
        #if defined(MEDIAINFO_LIBCURL_YES)
            Ssh_PublicKeyFileName.clear();
            Ssh_PrivateKeyFileName.clear();
            Ssh_KnownHostsFileName.clear();
            Ssl_CertificateFileName.clear();
            Ssl_CertificateFormat.clear();
            Ssl_PrivateKeyFileName.clear();
            Ssl_PrivateKeyFormat.clear();
            Ssl_CertificateAuthorityFileName.clear();
            Ssl_CertificateAuthorityPath.clear();
            Ssl_CertificateRevocationListFileName.clear();
        #endif //defined(MEDIAINFO_LIBCURL_YES)
    }
    else if (!LineSeparator.empty())
    {
        return; //Already done
    }

    //Filling
    FormatDetection_MaximumOffset=0;
    #if MEDIAINFO_ADVANCED
        VariableGopDetection_Occurences=4;
        VariableGopDetection_GiveUp=false;
        InitDataNotRepeated_Occurences=(int64u)-1; //Disabled by default
        InitDataNotRepeated_GiveUp=false;
    #endif //MEDIAINFO_ADVANCED
    #if MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)
        TimeOut=(int64u)-1;
        AcceptSignals=true;
    #endif //MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)
    MpegTs_MaximumOffset=64*1024*1024;
    MpegTs_MaximumScanDuration=30000000000LL;
    MpegTs_ForceStreamDisplay=false;
    #if MEDIAINFO_ADVANCED
        MpegTs_VbrDetection_Delta=0;
        MpegTs_VbrDetection_Occurences=4;
        MpegTs_VbrDetection_GiveUp=false;
    #endif //MEDIAINFO_ADVANCED
    #if MEDIAINFO_ADVANCED
        Format_Profile_Split=false;
    #endif //MEDIAINFO_ADVANCED
    #if defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_ADM_YES)
        Graph_Adm_ShowTrackUIDs=false;
        Graph_Adm_ShowChannelFormats=false;
    #endif //defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_ADM_YES)
    #if defined(MEDIAINFO_EBUCORE_YES)
        AcquisitionDataOutputMode=Export_EbuCore::AcquisitionDataOutputMode_Default;
    #endif //defined(MEDIAINFO_EBUCORE_YES)
    #if defined(MEDIAINFO_EBUCORE_YES) || defined(MEDIAINFO_NISO_YES)
        ExternalMetadata=Ztring();
        ExternalMetaDataConfig=Ztring();
    #endif //defined(MEDIAINFO_EBUCORE_YES) || defined(MEDIAINFO_NISO_YES)

    Complete=0;
    BlockMethod=0;
    Internet=0;
    MultipleValues=0;
    ShowFiles_Nothing=1;
    ShowFiles_VideoAudio=1;
    ShowFiles_VideoOnly=1;
    ShowFiles_AudioOnly=1;
    ShowFiles_TextOnly=1;
    ParseSpeed=(float32)0.5;
    Verbosity=(float32)0.5;
    Trace_Level=(float32)0.0;
    Compat=70778;
    Https=true;
    Trace_TimeSection_OnlyFirstOccurrence=false;
    Trace_Format=Trace_Format_Tree;
    Language_Raw=false;
    ReadByHuman=true;
    Inform_Version=false;
    Inform_Timestamp=false;
    Legacy=false;
    LegacyStreamDisplay=false;
    SkipBinaryData=false;
    Demux=0;
    LineSeparator=EOL;
    ColumnSeparator=__T(";");
    TagSeparator=__T(" / ");
    Quote=__T("\"");
    DecimalPoint=__T(".");
    ThousandsPoint=Ztring();
    CarriageReturnReplace=__T(" / ");
    #if MEDIAINFO_ADVANCED
        Conformance_Limit=32;
        Collection_Trigger=-2;
        Collection_Display=display_if::Needed;
    #endif
    #if MEDIAINFO_EVENTS
        Event_CallBackFunction=NULL;
        Event_UserHandler=NULL;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_CONFORMANCE
        Usac_Profile=(int8u)-1;
        Warning_Error=false;
    #endif //MEDIAINFO_CONFORMANCE
    #if defined(MEDIAINFO_LIBCURL_YES)
        URLEncode=URLEncode_Guess;
        Ssh_IgnoreSecurity=false;
        Ssl_IgnoreSecurity=false;
    #endif //defined(MEDIAINFO_LIBCURL_YES)
    #if MEDIAINFO_FIXITY
        TryToFix=false;
    #endif //MEDIAINFO_FIXITY
    #if MEDIAINFO_FLAG1
        Flags1=0;
    #endif //MEDIAINFO_FLAGX
    #if MEDIAINFO_FLAGX
        FlagsX=0;
    #endif //MEDIAINFO_FLAGX

    }

    ZtringListList ZLL1; Language_Set(ZLL1);
}

//***************************************************************************
// Info
//***************************************************************************

static inline int _OctDigitValue(const String::value_type &ch)
{
    switch (ch)
    {
    case __T('0'): return 0;
    case __T('1'): return 1;
    case __T('2'): return 2;
    case __T('3'): return 3;
    case __T('4'): return 4;
    case __T('5'): return 5;
    case __T('6'): return 6;
    case __T('7'): return 7;
    }
    return -1;
}

static inline int _HexDigitValue(const String::value_type &ch)
{
    switch (ch)
    {
    case __T('0'): return 0;
    case __T('1'): return 1;
    case __T('2'): return 2;
    case __T('3'): return 3;
    case __T('4'): return 4;
    case __T('5'): return 5;
    case __T('6'): return 6;
    case __T('7'): return 7;
    case __T('8'): return 8;
    case __T('9'): return 9;
    case __T('a'): case __T('A'): return 10;
    case __T('b'): case __T('B'): return 11;
    case __T('c'): case __T('C'): return 12;
    case __T('d'): case __T('D'): return 13;
    case __T('e'): case __T('E'): return 14;
    case __T('f'): case __T('F'): return 15;
    }
    return -1;
}

static String _DecodeEscapeC(String::const_iterator first, String::const_iterator last)
{
    String decoded;
    for (String::const_iterator it = first; it != last; ++it)
    {
        String::value_type ch = 0;
        int inc = 0;
        if (*it == __T('\\') && (it+1) != last)
        {
            switch (*(it+1))
            {
            case __T('a'):   ch = __T('\a'); inc = 1; break;
            case __T('b'):   ch = __T('\b'); inc = 1; break;
            case __T('f'):   ch = __T('\f'); inc = 1; break;
            case __T('n'):   ch = __T('\n'); inc = 1; break;
            case __T('r'):   ch = __T('\r'); inc = 1; break;
            case __T('t'):   ch = __T('\t'); inc = 1; break;
            case __T('v'):   ch = __T('\v'); inc = 1; break;
            case __T('\''):  ch = __T('\''); inc = 1; break;
            case __T('\"'):  ch = __T('\"'); inc = 1; break;
            case __T('\\'):  ch = __T('\\'); inc = 1; break;
            case __T('?'):   ch = __T('?');  inc = 1; break;
            case __T('x'): // Hex
                {
                    int d;
                    if ((it+2) != last && (d = _HexDigitValue(*(it+2))) >= 0)
                    {
                        ch = String::value_type(d);
                        inc = 2;
                        if ((it+3) != last && (d = _HexDigitValue(*(it+3))) >= 0)
                        {
                            ch = (ch << 4) | String::value_type(d);
                            ++inc;
                        }
                    }
                }
                break;
#if defined(__UNICODE__)
            case __T('u'): case __T('U'): // Unicode
                {
                    int d;
                    if ((it+2) != last && (d = _HexDigitValue(*(it+2))) >= 0)
                    {
                        ch = String::value_type(d);
                        inc = 2;
                        for (int i = 0; i < 3; ++i)
                        {
                            if ((it+3+i) != last && (d = _HexDigitValue(*(it+3+i))) >= 0)
                            {
                                ch = (ch << 4) | String::value_type(d);
                                ++inc;
                            }
                        }
                    }
                }
                break;
#endif
            default:
                {
                    int d;
                    if ((d = _OctDigitValue(*(it+1))) >= 0) // Oct
                    {
                        ch = String::value_type(d);
                        inc = 1;
                        if ((it+2) != last && (d = _OctDigitValue(*(it+2))) >= 0)
                        {
                            ch = (ch << 3) | String::value_type(d);
                            ++inc;
                            if ((it+3) != last && (d = _OctDigitValue(*(it+3))) >= 0)
                            {
                                ch = (ch << 3) | String::value_type(d);
                                ++inc;
                            }
                        }
                    }
                }
                break;
            }
        }
        if (inc > 0)
        {
            decoded += ch;
            it += inc;
        }
        else
        {
            decoded += *it;
        }
    }
    return decoded;
}

Ztring MediaInfo_Config::Option (const String &Option, const String &Value_Raw)
{
    {
    CriticalSectionLocker CSL(CS);
    SubFile_Config(Option)=Value_Raw;
    }

    String Option_Lower(Option);
    size_t Egal_Pos=Option_Lower.find(__T('='));
    if (Egal_Pos==string::npos)
        Egal_Pos=Option_Lower.size();
    transform(Option_Lower.begin(), Option_Lower.begin()+Egal_Pos, Option_Lower.begin(), (int(*)(int))tolower); //(int(*)(int)) is a patch for unix

    //Parsing pointer to a file
    Ztring Value;
    #if defined(MEDIAINFO_FILE_YES)
    if (Value_Raw.find(__T("file://"))==0)
    {
        //Open
        Ztring FileName(Value_Raw, 7, Ztring::npos);
        File F(FileName.c_str());

        //Read
        int64u Size=F.Size_Get();
        if (Size>=0xFFFFFFFF)
            Size=1024*1024;
        int8u* Buffer=new int8u[(size_t)Size+1];
        size_t Pos=F.Read(Buffer, (size_t)Size);
        F.Close();
        Buffer[Pos]='\0';
        Ztring FromFile; FromFile.From_UTF8((char*)Buffer);
        if (FromFile.empty())
             FromFile.From_UTF8((char*)Buffer);
        delete[] Buffer; //Buffer=NULL;

        //Merge
        Value=FromFile;
    }
    else
    #endif //defined(MEDIAINFO_FILE_YES)
    if (Value_Raw.substr(0, 7)==__T("cstr://"))
    {
        Value=_DecodeEscapeC(Value_Raw.begin() + 7, Value_Raw.end());
    }
    else
        Value=Value_Raw;

    if (Option_Lower.empty())
    {
        return Ztring();
    }
    if (Option_Lower==__T("charset_config"))
    {
        return Ztring(); //Only used in DLL, no Library action
    }
    if (Option_Lower==__T("charset_output"))
    {
        return Ztring(); //Only used in DLL, no Library action
    }
    if (Option_Lower==__T("complete"))
    {
        Complete_Set(Value.To_int8u()?true:false);
        return Ztring();
    }
    if (Option_Lower==__T("complete_get"))
    {
        if (Complete_Get())
            return __T("1");
        else
            return Ztring();
    }
    if (Option_Lower==__T("blockmethod"))
    {
        if (Value.empty())
            BlockMethod_Set(0);
        else
            BlockMethod_Set(1);
        return Ztring();
    }
    if (Option_Lower==__T("blockmethod_get"))
    {
        if (BlockMethod_Get())
            return __T("1");
        else
            return Ztring();
    }
    if (Option_Lower==__T("internet"))
    {
        if (Value.empty())
            Internet_Set(0);
        else
            Internet_Set(1);
        return Ztring();
    }
    if (Option_Lower==__T("internet_get"))
    {
        if (Internet_Get())
            return __T("1");
        else
            return Ztring();
    }
    if (Option_Lower==__T("demux"))
    {
        String Value_Lower(Value);
        transform(Value_Lower.begin(), Value_Lower.end(), Value_Lower.begin(), (int(*)(int))tolower); //(int(*)(int)) is a patch for unix

             if (Value_Lower==__T("all"))
            Demux_Set(7);
        else if (Value_Lower==__T("frame"))
            Demux_Set(1);
        else if (Value_Lower==__T("container"))
            Demux_Set(2);
        else if (Value_Lower==__T("elementary"))
            Demux_Set(4);
        else
            Demux_Set(0);
        return Ztring();
    }
    if (Option_Lower==__T("demux_get"))
    {
        switch (Demux_Get())
        {
            case 7 : return __T("All");
            case 1 : return __T("Frame");
            case 2 : return __T("Container");
            case 4 : return __T("Elementary");
            default: return Ztring();
        }
    }
    if (Option_Lower==__T("multiplevalues"))
    {
        if (Value.empty())
            MultipleValues_Set(0);
        else
            MultipleValues_Set(1);
        return Ztring();
    }
    if (Option_Lower==__T("multiplevalues_get"))
    {
        if (MultipleValues_Get())
            return __T("1");
        else
            return Ztring();
    }
    if (Option_Lower==__T("parseonlyknownextensions"))
    {
        #if MEDIAINFO_ADVANCED
            ParseOnlyKnownExtensions_Set(Value);
            return Ztring();
        #else // MEDIAINFO_ADVANCED
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED
    }
    if (Option_Lower==__T("parseonlyknownextensions_get"))
    {
        #if MEDIAINFO_ADVANCED
            return ParseOnlyKnownExtensions_Get();
        #else // MEDIAINFO_ADVANCED
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED
    }
    if (Option_Lower==__T("parseonlyknownextensions_getlist"))
    {
        #if MEDIAINFO_ADVANCED
            return ParseOnlyKnownExtensions_GetList_String();
        #else // MEDIAINFO_ADVANCED
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED
    }
    if (Option_Lower==__T("parseunknownextensions"))
    {
        #if MEDIAINFO_ADVANCED
            if (Value==__T("0"))
                ParseOnlyKnownExtensions_Set(__T("1"));
            else
                ParseOnlyKnownExtensions_Set(Ztring());
            return Ztring();
        #else // MEDIAINFO_ADVANCED
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED
    }
    if (Option_Lower==__T("parseunknownextensions_get"))
    {
        #if MEDIAINFO_ADVANCED
            if (ParseOnlyKnownExtensions_Get().empty())
                return __T("1");
            else
                return Ztring();
        #else // MEDIAINFO_ADVANCED
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED
    }
    if (Option_Lower==__T("showfiles_set"))
    {
        ShowFiles_Set(Value.c_str());
        return Ztring();
    }
    if (Option_Lower==__T("readbyhuman"))
    {
        ReadByHuman_Set(Value.To_int8u()?true:false);
        return Ztring();
    }
    if (Option_Lower==__T("readbyhuman_get"))
    {
        return ReadByHuman_Get()?__T("1"):__T("0");
    }
    if (Option_Lower==__T("legacy"))
    {
        Legacy_Set(Value.To_int8u()?true:false);
        return Ztring();
    }
    if (Option_Lower==__T("legacy_get"))
    {
        return Legacy_Get()?__T("1"):__T("0");
    }
    if (Option_Lower==__T("legacystreamdisplay"))
    {
        LegacyStreamDisplay_Set(Value.To_int8u()?true:false);
        return Ztring();
    }
    if (Option_Lower==__T("legacystreamdisplay_get"))
    {
        return LegacyStreamDisplay_Get()?__T("1"):__T("0");
    }
    if (Option_Lower==__T("skipbinarydata"))
    {
        SkipBinaryData_Set(Value.To_int8u()?true:false);
        return Ztring();
    }
    if (Option_Lower==__T("skipbinarydata_get"))
    {
        return SkipBinaryData_Get()?__T("1"):__T("0");
    }
    if (Option_Lower==__T("parsespeed"))
    {
        ParseSpeed_Set(Value.To_float32());
        return Ztring();
    }
    if (Option_Lower==__T("parsespeed_get"))
    {
        return Ztring::ToZtring(ParseSpeed_Get(), 3);
    }
    if (Option_Lower==__T("verbosity"))
    {
        Verbosity_Set(Value.To_float32());
        return Ztring();
    }
    if (Option_Lower==__T("verbosity_get"))
    {
        return Ztring::ToZtring(Verbosity_Get(), 3);
    }
    if (Option_Lower==__T("lineseparator"))
    {
        LineSeparator_Set(Value);
        return Ztring();
    }
    if (Option_Lower==__T("lineseparator_get"))
    {
        return LineSeparator_Get();
    }
    if (Option_Lower==__T("version"))
    {
        Version_Set(Value);
        return Ztring();
    }
    if (Option_Lower==__T("version_get"))
    {
        return Version_Get();
    }
    if (Option_Lower==__T("columnseparator"))
    {
        ColumnSeparator_Set(Value);
        return Ztring();
    }
    if (Option_Lower==__T("columnseparator_get"))
    {
        return ColumnSeparator_Get();
    }
    if (Option_Lower==__T("tagseparator"))
    {
        TagSeparator_Set(Value);
        return Ztring();
    }
    if (Option_Lower==__T("tagseparator_get"))
    {
        return TagSeparator_Get();
    }
    if (Option_Lower==__T("quote"))
    {
        Quote_Set(Value);
        return Ztring();
    }
    if (Option_Lower==__T("quote_get"))
    {
        return Quote_Get();
    }
    if (Option_Lower==__T("decimalpoint"))
    {
        DecimalPoint_Set(Value);
        return Ztring();
    }
    if (Option_Lower==__T("decimalpoint_get"))
    {
        return DecimalPoint_Get();
    }
    if (Option_Lower==__T("thousandspoint"))
    {
        ThousandsPoint_Set(Value);
        return Ztring();
    }
    if (Option_Lower==__T("thousandspoint_get"))
    {
        return ThousandsPoint_Get();
    }
    if (Option_Lower==__T("carriagereturnreplace"))
    {
        if (Value.find_first_of(__T("\r\n"))!=string::npos)
            return __T("\\r or \\n in CarriageReturnReplace is not supported");
        CarriageReturnReplace_Set(Value);
        return Ztring();
    }
    if (Option_Lower==__T("carriagereturnreplace_get"))
    {
        return CarriageReturnReplace_Get();
    }
    if (Option_Lower==__T("streammax"))
    {
        ZtringListList StreamMax=Value.c_str();
        StreamMax_Set(StreamMax);
        return Ztring();
    }
    if (Option_Lower==__T("streammax_get"))
    {
        return StreamMax_Get();
    }
    if (Option_Lower==__T("language"))
    {
        ZtringListList Language=Value.c_str();
        Language_Set(Language);
        return Ztring();
    }
    if (Option_Lower==__T("language_get"))
    {
        return Language_Get();
    }
    if (Option_Lower==__T("inform"))
    {
        Inform_Set(Value.c_str());
        return Ztring();
    }
    if (Option_Lower==__T("output"))
    {
        Inform_Set(Value.c_str());
        return Ztring();
    }
    if (Option_Lower==__T("inform_get"))
    {
        return Inform_Get();
    }
    if (Option_Lower==__T("output_get"))
    {
        return Inform_Get();
    }
    if (Option_Lower==__T("inform_replace"))
    {
        Inform_Replace_Set(Value.c_str());
        return Ztring();
    }
    if (Option_Lower==__T("inform_replace_get"))
    {
        return Inform_Get();
    }
    if (Option_Lower==__T("inform_version"))
    {
        Inform_Version_Set(Value.To_int8u()?true:false);
        return Ztring();
    }
    if (Option_Lower==__T("inform_version_get"))
    {
        return Inform_Version_Get()?__T("1"):__T("0");
    }
    if (Option_Lower==__T("inform_timestamp"))
    {
        Inform_Timestamp_Set(Value.To_int8u()?true:false);
        return Ztring();
    }
    if (Option_Lower==__T("inform_timestamp_get"))
    {
        return Inform_Timestamp_Get()?__T("1"):__T("0");
    }
    #if MEDIAINFO_ADVANCED
        if (Option_Lower==__T("cover_data"))
        {
            return Cover_Data_Set(Value.c_str());
        }
        if (Option_Lower==__T("cover_data_get"))
        {
            return Cover_Data_Get();
        }
    #endif //MEDIAINFO_ADVANCED
    #if MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)
        if (Option_Lower==__T("enable_ffmpeg"))
        {
            return Enable_FFmpeg_Set(Value.To_int8u()?true:false);
        }
        if (Option_Lower==__T("enable_ffmpeg_get"))
        {
            return Enable_FFmpeg_Get()?__T("1"):__T("0");
        }
    #endif //MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)
    #if MEDIAINFO_COMPRESS
        if (Option_Lower==__T("inform_compress"))
        {
            return Inform_Compress_Set(Value.c_str());
        }
        if (Option_Lower==__T("inform_compress_get"))
        {
            return Inform_Compress_Get();
        }
        if (Option_Lower==__T("input_compressed"))
        {
            return Input_Compressed_Set(Value.c_str());
        }
        if (Option_Lower==__T("input_compressed_get"))
        {
            return Input_Compressed_Get();
        }
    #endif //MEDIAINFO_COMPRESS
    if (Option_Lower==__T("details")) //Legacy for trace_level
    {
        if (Value == __T("0"))
            Trace_Level=0;
        return MediaInfo_Config::Option(__T("Trace_Level"), Value);
    }
    if (Option_Lower==__T("details_get")) //Legacy for trace_level
    {
        return MediaInfo_Config::Option(__T("Trace_Level_Get"), Value);
    }
    if (Option_Lower==__T("detailslevel")) //Legacy for trace_level
    {
        return MediaInfo_Config::Option(__T("Trace_Level"), Value);
    }
    if (Option_Lower==__T("detailslevel_get")) //Legacy for trace_level
    {
        return MediaInfo_Config::Option(__T("Trace_Level_Get"), Value);
    }
    if (Option_Lower==__T("trace_level"))
    {
        Trace_Level_Set(Value);
        if (Inform_Get()==__T("MAXML"))
            Trace_Format_Set(Trace_Format_XML); // All must be XML
        if (Inform_Get()==__T("XML") || Inform_Get()==__T("MAXML"))
        {
            Inform_Set(Ztring());
            Trace_Format_Set(Trace_Format_XML); // TODO: better coherency in options
        }
        if (Inform_Get()==__T("MICRO_XML"))
        {
            Inform_Set(Ztring());
            Trace_Format_Set(Trace_Format_MICRO_XML);
        }
        return Ztring();
    }
    if (Option_Lower==__T("trace_level_get"))
    {
        return Ztring::ToZtring(Trace_Level_Get());
    }
    if (Option_Lower==__T("https"))
    {
        Https_Set(Value.To_int64u()?true:false);
        return Ztring();
    }
    if (Option_Lower==__T("no-https"))
    {
        Https_Set(false);
        return Ztring();
    }
    if (Option_Lower==__T("https_get"))
    {
        return Https_Get()?__T("1"):__T("0");
    }
    if (Option_Lower==__T("trace_timesection_onlyfirstoccurrence"))
    {
        Trace_TimeSection_OnlyFirstOccurrence_Set(Value.To_int64u()?true:false);
        return Ztring();
    }
    if (Option_Lower==__T("trace_timesection_onlyfirstoccurrence_get"))
    {
        return Trace_TimeSection_OnlyFirstOccurrence_Get()?__T("1"):__T("0");
    }
    if (Option_Lower==__T("detailsformat")) //Legacy for trace_format
    {
        return MediaInfo_Config::Option(__T("Trace_Format"), Value);
    }
    if (Option_Lower==__T("detailsformat_get")) //Legacy for trace_format
    {
        return MediaInfo_Config::Option(__T("Trace_Format_Get"), Value);
    }
    if (Option_Lower==__T("trace_format"))
    {
        String NewValue_Lower(Value);
        transform(NewValue_Lower.begin(), NewValue_Lower.end(), NewValue_Lower.begin(), (int(*)(int))tolower); //(int(*)(int)) is a patch for unix

        if (NewValue_Lower==__T("csv"))
            Trace_Format_Set(Trace_Format_CSV);
        else if (NewValue_Lower==__T("xml") || NewValue_Lower==__T("MAXML"))
            Trace_Format_Set(Trace_Format_XML);
        else if (NewValue_Lower==__T("micro_xml"))
            Trace_Format_Set(Trace_Format_MICRO_XML);
        else
            Trace_Format_Set(Trace_Format_Tree);
        return Ztring();
    }
    if (Option_Lower==__T("trace_format_get"))
    {
        switch (Trace_Format_Get())
        {
            case Trace_Format_CSV : return __T("CSV");
            default : return __T("Tree");
        }
    }
    if (Option_Lower==__T("detailsmodificator"))
    {
        Trace_Modificator_Set(Value);
        return Ztring();
    }
    if (Option_Lower==__T("detailsmodificator_get"))
    {
        return Trace_Modificator_Get(Value);
    }
    if (Option_Lower==__T("hideparameter"))
    {
        return HideShowParameter(Value, __T('N'));
    }
    if (Option_Lower==__T("showparameter"))
    {
        return HideShowParameter(Value, __T('Y'));
    }
    if (Option_Lower==__T("info_parameters"))
    {
        ZtringListList ToReturn=Info_Parameters_Get();

        //Adapt first column
        for (size_t Pos=0; Pos<ToReturn.size(); Pos++)
        {
            Ztring &C1=ToReturn(Pos, 0);
            if (!ToReturn(Pos, 1).empty())
            {
                C1.resize(25, ' ');
                ToReturn(Pos, 0)=C1 + __T(" :");
            }
        }

        ToReturn.Separator_Set(0, LineSeparator_Get());
        ToReturn.Separator_Set(1, __T(" "));
        ToReturn.Quote_Set(Ztring());
        return ToReturn.Read();
    }
    if (Option_Lower==__T("info_outputformats"))
    {
        return Info_OutputFormats_Get(BasicFormat_Text);
    }
    if (Option_Lower==__T("info_outputformats_csv"))
    {
        return Info_OutputFormats_Get(BasicFormat_CSV);
    }
    if (Option_Lower==__T("info_outputformats_json"))
    {
        return Info_OutputFormats_Get(BasicFormat_JSON);
    }
    if (Option_Lower==__T("info_parameters_csv"))
    {
        return Info_Parameters_Get(Value==__T("Complete"));
    }
    if (Option_Lower==__T("info_codecs"))
    {
        return Info_Codecs_Get();
    }
    if (Option_Lower==__T("info_version"))
    {
        return Info_Version_Get();
    }
    if (Option_Lower==__T("info_url"))
    {
        return Info_Url_Get();
    }
    if (Option_Lower==__T("formatdetection_maximumoffset"))
    {
        FormatDetection_MaximumOffset_Set(Value==__T("-1")?(int64u)-1:((Ztring*)&Value)->To_int64u());
        return Ztring();
    }
    if (Option_Lower==__T("formatdetection_maximumoffset_get"))
    {
        return FormatDetection_MaximumOffset_Get()==(int64u)-1?Ztring(__T("-1")):Ztring::ToZtring(FormatDetection_MaximumOffset_Get());
    }
    if (Option_Lower==__T("variablegopdetection_occurences"))
    {
        #if MEDIAINFO_ADVANCED
            VariableGopDetection_Occurences_Set(Value.To_int64u());
            return Ztring();
        #else // MEDIAINFO_ADVANCED
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED
    }
    if (Option_Lower==__T("variablegopdetection_occurences_get"))
    {
        #if MEDIAINFO_ADVANCED
            return Ztring::ToZtring(VariableGopDetection_Occurences_Get());
        #else // MEDIAINFO_ADVANCED
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED
    }
    if (Option_Lower==__T("variablegopdetection_giveup"))
    {
        #if MEDIAINFO_ADVANCED
            VariableGopDetection_GiveUp_Set(Value.To_int8u()?true:false);
            return Ztring();
        #else // MEDIAINFO_ADVANCED
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED
    }
    if (Option_Lower==__T("variablegopdetection_giveup_get"))
    {
        #if MEDIAINFO_ADVANCED
            return VariableGopDetection_GiveUp_Get()?__T("1"):__T("0");
        #else // MEDIAINFO_ADVANCED
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED
    }
    if (Option_Lower==__T("initdatanotrepeated_occurences"))
    {
        #if MEDIAINFO_ADVANCED
            InitDataNotRepeated_Occurences_Set(Value.To_int64u());
            return Ztring();
        #else // MEDIAINFO_ADVANCED
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED
    }
    if (Option_Lower==__T("initdatanotrepeated_occurences_get"))
    {
        #if MEDIAINFO_ADVANCED
            return Ztring::ToZtring(InitDataNotRepeated_Occurences_Get());
        #else // MEDIAINFO_ADVANCED
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED
    }
    if (Option_Lower==__T("initdatanotrepeated_giveup"))
    {
        #if MEDIAINFO_ADVANCED
            InitDataNotRepeated_GiveUp_Set(Value.To_int8u()?true:false);
            return Ztring();
        #else // MEDIAINFO_ADVANCED
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED
    }
    if (Option_Lower==__T("initdatanotrepeated_giveup_get"))
    {
        #if MEDIAINFO_ADVANCED
            return InitDataNotRepeated_GiveUp_Get()?__T("1"):__T("0");
        #else // MEDIAINFO_ADVANCED
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED
    }
    if (Option_Lower==__T("timeout"))
    {
        #if MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)
            TimeOut_Set(Value.empty()?((int64u)-1):Value.To_int64u());
            return Ztring();
        #else // MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)
    }
    if (Option_Lower==__T("timeout_get"))
    {
        #if MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)
            return Ztring::ToZtring(TimeOut_Get());
        #else // MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)
    }
    if (Option_Lower==__T("acceptsignals"))
    {
        #if MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)
            AcceptSignals_Set(!(Value==__T("0") || Value.empty()));
            return Ztring();
        #else // MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)
    }
    if (Option_Lower==__T("acceptsignals_get"))
    {
        #if MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)
            return AcceptSignals_Get()?__T("1"):__T("0");
        #else // MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)
    }
    if (Option_Lower==__T("mpegts_maximumoffset"))
    {
        MpegTs_MaximumOffset_Set(Value==__T("-1")?(int64u)-1:((Ztring*)&Value)->To_int64u());
        return Ztring();
    }
    if (Option_Lower==__T("mpegts_maximumoffset_get"))
    {
        return MpegTs_MaximumOffset_Get()==(int64u)-1?Ztring(__T("-1")):Ztring::ToZtring(MpegTs_MaximumOffset_Get());
    }
    if (Option_Lower==__T("mpegts_vbrdetection_delta"))
    {
        #if MEDIAINFO_ADVANCED
            MpegTs_VbrDetection_Delta_Set(Value.To_float64());
            return Ztring();
        #else // MEDIAINFO_ADVANCED
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED
    }
    if (Option_Lower==__T("mpegts_vbrdetection_delta_get"))
    {
        #if MEDIAINFO_ADVANCED
            return Ztring::ToZtring(MpegTs_VbrDetection_Delta_Get(), 9);
        #else // MEDIAINFO_ADVANCED
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED
    }
    if (Option_Lower==__T("mpegts_vbrdetection_occurences"))
    {
        #if MEDIAINFO_ADVANCED
            MpegTs_VbrDetection_Occurences_Set(Value.To_int64u());
            return Ztring();
        #else // MEDIAINFO_ADVANCED
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED
    }
    if (Option_Lower==__T("mpegts_vbrdetection_occurences_get"))
    {
        #if MEDIAINFO_ADVANCED
            return Ztring::ToZtring(MpegTs_VbrDetection_Occurences_Get());
        #else // MEDIAINFO_ADVANCED
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED
    }
    if (Option_Lower==__T("mpegts_vbrdetection_giveup"))
    {
        #if MEDIAINFO_ADVANCED
            MpegTs_VbrDetection_GiveUp_Set(Value.To_int8u()?true:false);
            return Ztring();
        #else // MEDIAINFO_ADVANCED
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED
    }
    if (Option_Lower==__T("mpegts_vbrdetection_giveup_get"))
    {
        #if MEDIAINFO_ADVANCED
            return MpegTs_VbrDetection_GiveUp_Get()?__T("1"):__T("0");
        #else // MEDIAINFO_ADVANCED
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED
    }
    if (Option_Lower==__T("mpegts_maximumscanduration"))
    {
        MpegTs_MaximumScanDuration_Set(float64_int64s((((Ztring*)&Value)->To_float64())*1000000000));
        return Ztring();
    }
    if (Option_Lower==__T("mpegts_maximumscanduration_get"))
    {
        return MpegTs_MaximumScanDuration_Get()==(int64u)-1?Ztring(__T("-1")):Ztring::ToZtring(MpegTs_MaximumOffset_Get());
    }
    if (Option_Lower==__T("mpegts_forcestreamdisplay"))
    {
        MpegTs_ForceStreamDisplay_Set(Value.To_int8u()?true:false);
        return Ztring();
    }
    if (Option_Lower==__T("mpegts_forcestreamdisplay_get"))
    {
        return MpegTs_ForceStreamDisplay_Get()?__T("1"):__T("0");
    }
    if (Option_Lower == __T("mpegts_forcetextstreamdisplay"))
    {
        MpegTs_ForceTextStreamDisplay_Set(Value.To_int8u() ? true : false);
        return Ztring();
    }
    if (Option_Lower == __T("mpegts_forcetextstreamdisplay_get"))
    {
        return MpegTs_ForceTextStreamDisplay_Get() ? __T("1") : __T("0");
    }
    if (Option_Lower==__T("maxml_streamkinds"))
    {
        #if MEDIAINFO_ADVANCED
            return MAXML_StreamKinds_Get();
        #else // MEDIAINFO_ADVANCED
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED
    }
    if (Option_Lower==__T("maxml_fields"))
    {
        #if MEDIAINFO_ADVANCED
            return MAXML_Fields_Get(Value);
        #else // MEDIAINFO_ADVANCED
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED
    }
    if (Option_Lower==__T("custommapping"))
    {
        CustomMapping_Set(Value);
        return Ztring();
    }
    if (Option_Lower==__T("conformance_limit"))
    {
        #if MEDIAINFO_CONFORMANCE
            Conformance_Limit_Set(Value);
            return Ztring();
        #else // MEDIAINFO_CONFORMANCE
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_CONFORMANCE
    }
    if (Option_Lower==__T("collection_trigger"))
    {
        #if MEDIAINFO_ADVANCED
            Collection_Trigger_Set(Value);
            return Ztring();
        #else // MEDIAINFO_ADVANCED
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED
    }
    if (Option_Lower==__T("collection_display"))
    {
        #if MEDIAINFO_ADVANCED
            String Value_Lower(Value);
            transform(Value_Lower.begin(), Value_Lower.end(), Value_Lower.begin(), (int(*)(int))tolower); //(int(*)(int)) is a patch for unix
            return Collection_Display_Set(Value_Lower);
        #else // MEDIAINFO_ADVANCED
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED
    }
    if (Option_Lower==__T("format_profile_split"))
    {
        #if MEDIAINFO_ADVANCED
            Format_Profile_Split_Set(Value.To_int8u()?true:false);
            return Ztring();
        #else // MEDIAINFO_ADVANCED
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED
    }
    if (Option_Lower==__T("format_profile_split_get"))
    {
        #if MEDIAINFO_ADVANCED
            return Format_Profile_Split_Get()?__T("1"):__T("0");;
        #else // MEDIAINFO_ADVANCED
            return __T("advanced features are disabled due to compilation options");
        #endif // MEDIAINFO_ADVANCED
    }
    if (Option_Lower==__T("graph_adm_showtrackuids"))
    {
        #if defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_ADM_YES)
            Graph_Adm_ShowTrackUIDs_Set(Value.To_int8u()?true:false);
            return Ztring();
        #else //defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_ADM_YES)
            return __T("Feature disabled due to compilation options");
        #endif // defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_ADM_YES)
    }
    if (Option_Lower == __T("graph_adm_showtrackuids_get"))
    {
        #if defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_ADM_YES)
            return Graph_Adm_ShowTrackUIDs_Get()?__T("1"):__T("0");
        #else //defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_ADM_YES)
            return __T("Feature disabled due to compilation options");
        #endif // defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_ADM_YES)
    }
    if (Option_Lower==__T("graph_adm_showchannelformats"))
    {
        #if defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_ADM_YES)
            Graph_Adm_ShowChannelFormats_Set(Value.To_int8u()?true:false);
            return Ztring();
        #else //defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_ADM_YES)
            return __T("Feature disabled due to compilation options");
        #endif // defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_ADM_YES)
    }
    if (Option_Lower == __T("graph_adm_showchannelformats_get"))
    {
        #if defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_ADM_YES)
            return Graph_Adm_ShowChannelFormats_Get()?__T("1"):__T("0");
        #else //defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_ADM_YES)
            return __T("Feature disabled due to compilation options");
        #endif // defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_ADM_YES)
    }
    if (Option_Lower==__T("acquisitiondataoutputmode"))
    {
        #if defined(MEDIAINFO_EBUCORE_YES)
            Ztring Mode(Value);
            Mode.MakeLowerCase();
            if (     Mode==__T("default"))
                AcquisitionDataOutputMode_Set(Export_EbuCore::AcquisitionDataOutputMode_Default);
            else if (Mode==__T("parametersegment"))
                AcquisitionDataOutputMode_Set(Export_EbuCore::AcquisitionDataOutputMode_parameterSegment);
            else if (Mode==__T("segmentparameter"))
                AcquisitionDataOutputMode_Set(Export_EbuCore::AcquisitionDataOutputMode_segmentParameter);
            else
                return __T("Invalid value");
            return Ztring();
        #else // MEDIAINFO_EBUCORE_YES
            return __T("EBUCore features are disabled due to compilation options");
        #endif // MEDIAINFO_EBUCORE_YES
    }
    if (Option_Lower==__T("externalmetadata"))
    {
        #if defined(MEDIAINFO_EBUCORE_YES) || defined(MEDIAINFO_NISO_YES)
            ExternalMetadata_Set(Value);
            return Ztring();
        #else // MEDIAINFO_EBUCORE_YES || defined(MEDIAINFO_NISO_YES)
            return __T("Feature disabled due to compilation options");
        #endif // MEDIAINFO_EBUCORE_YES || defined(MEDIAINFO_NISO_YES)
    }
    if (Option_Lower==__T("externalmetadataconfig"))
    {
        #if defined(MEDIAINFO_EBUCORE_YES) || defined(MEDIAINFO_NISO_YES)
            ExternalMetaDataConfig_Set(Value);
            return Ztring();
        #else // MEDIAINFO_EBUCORE_YES || defined(MEDIAINFO_NISO_YES)
            return __T("Features disabled due to compilation options");
        #endif // MEDIAINFO_EBUCORE_YES || defined(MEDIAINFO_NISO_YES)
    }
    if (Option_Lower==__T("event_callbackfunction"))
    {
        #if MEDIAINFO_EVENTS
            return Event_CallBackFunction_Set(Value);
        #else //MEDIAINFO_EVENTS
            return __T("Event manager is disabled due to compilation options");
        #endif //MEDIAINFO_EVENTS
    }
    if (Option_Lower==__T("urlencode"))
    {
        #if defined(MEDIAINFO_LIBCURL_YES)
            String Value_Lower(Value);
            transform(Value_Lower.begin(), Value_Lower.end(), Value_Lower.begin(), (int(*)(int))tolower); //(int(*)(int)) is a patch for unix

                 if (Value_Lower==__T("guess"))
                URLEncode_Set(URLEncode_Guess);
            else
                URLEncode_Set(Value.To_int8u()?URLEncode_Yes:URLEncode_No);
            return Ztring();
        #else // defined(MEDIAINFO_LIBCURL_YES)
            return __T("Libcurl support is disabled due to compilation options");
        #endif // defined(MEDIAINFO_LIBCURL_YES)
    }
    if (Option_Lower==__T("ssh_knownhostsfilename"))
    {
        #if defined(MEDIAINFO_LIBCURL_YES)
            Ssh_KnownHostsFileName_Set(Value);
            return Ztring();
        #else // defined(MEDIAINFO_LIBCURL_YES)
            return __T("Libcurl support is disabled due to compilation options");
        #endif // defined(MEDIAINFO_LIBCURL_YES)
    }

    if (Option_Lower==__T("info_graph_svg_plugin_state"))
    {
        #if defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_GRAPHVIZ_YES)
            return GraphSvgPluginState()?__T("1"):__T("0");
        #else // defined(MEDIAINFO_GRAPHVIZ_YES)
            return __T("Graphviz support is disabled due to compilation options");
        #endif // defined(MEDIAINFO_GRAPHVIZ_YES)
    }

    if (Option_Lower==__T("admprofile"))
    {
        #if MEDIAINFO_CONFORMANCE
            return Ztring().From_UTF8(AdmProfile(Value));
        #else // MEDIAINFO_CONFORMANCE
            return __T("conformance features are disabled due to compilation options");
        #endif // MEDIAINFO_CONFORMANCE
    }
    if (Option_Lower==__T("admprofile_list"))
    {
        #if MEDIAINFO_CONFORMANCE
            return Ztring().From_UTF8(AdmProfile_List());
        #else // MEDIAINFO_CONFORMANCE
            return Ztring();
        #endif // MEDIAINFO_CONFORMANCE
    }

    if (Option_Lower==__T("mp4profile"))
    {
        #if MEDIAINFO_CONFORMANCE
            return Ztring().From_UTF8(Mp4Profile(Value));
        #else // MEDIAINFO_CONFORMANCE
            return __T("conformance features are disabled due to compilation options");
        #endif // MEDIAINFO_CONFORMANCE
    }
    if (Option_Lower==__T("mp4profile_list"))
    {
        #if MEDIAINFO_CONFORMANCE
            return Ztring().From_UTF8(Mp4Profile_List());
        #else // MEDIAINFO_CONFORMANCE
            return Ztring();
        #endif // MEDIAINFO_CONFORMANCE
    }

    if (Option_Lower==__T("usacprofile"))
    {
        #if MEDIAINFO_CONFORMANCE
            return Ztring().From_UTF8(UsacProfile(Value));
        #else // MEDIAINFO_CONFORMANCE
            return __T("conformance features are disabled due to compilation options");
        #endif // MEDIAINFO_CONFORMANCE
    }
    if (Option_Lower==__T("usacprofile_list"))
    {
        #if MEDIAINFO_CONFORMANCE
            return Ztring().From_UTF8(UsacProfile_List());
        #else // MEDIAINFO_CONFORMANCE
            return Ztring();
        #endif // MEDIAINFO_CONFORMANCE
    }

    if (Option_Lower==__T("profile_list"))
    {
        #if MEDIAINFO_CONFORMANCE
            return Ztring().From_UTF8(Profile_List());
        #else // MEDIAINFO_CONFORMANCE
            return __T("conformance features are disabled due to compilation options");
        #endif // MEDIAINFO_CONFORMANCE
    }
    if (Option_Lower==__T("warning"))
    {
        #if MEDIAINFO_CONFORMANCE
            String Value_Lower(Value);
            transform(Value_Lower.begin(), Value_Lower.end(), Value_Lower.begin(), (int(*)(int))tolower); //(int(*)(int)) is a patch for unix
            WarningError(Value_Lower==__T("error"));
            return Ztring();
        #else // MEDIAINFO_CONFORMANCE
            return __T("conformance features are disabled due to compilation options");
        #endif // MEDIAINFO_CONFORMANCE
    }
    if (Option_Lower==__T("info_canhandleurls"))
    {
        #if defined(MEDIAINFO_LIBCURL_YES)
            return CanHandleUrls()?__T("1"):__T("0");
        #else // defined(MEDIAINFO_LIBCURL_YES)
            return __T("Libcurl support is disabled due to compilation options");
        #endif // defined(MEDIAINFO_LIBCURL_YES)
    }
    if (Option_Lower==__T("ssh_publickeyfilename"))
    {
        #if defined(MEDIAINFO_LIBCURL_YES)
            Ssh_PublicKeyFileName_Set(Value);
            return Ztring();
        #else // defined(MEDIAINFO_LIBCURL_YES)
            return __T("Libcurl support is disabled due to compilation options");
        #endif // defined(MEDIAINFO_LIBCURL_YES)
    }
    if (Option_Lower==__T("ssh_privatekeyfilename"))
    {
        #if defined(MEDIAINFO_LIBCURL_YES)
            Ssh_PrivateKeyFileName_Set(Value);
            return Ztring();
        #else // defined(MEDIAINFO_LIBCURL_YES)
            return __T("Libcurl support is disabled due to compilation options");
        #endif // defined(MEDIAINFO_LIBCURL_YES)
    }
    if (Option_Lower==__T("ssh_ignoresecurity"))
    {
        #if defined(MEDIAINFO_LIBCURL_YES)
            Ssh_IgnoreSecurity_Set(Value.empty() || Value.To_float32());
            return Ztring();
        #else // defined(MEDIAINFO_LIBCURL_YES)
            return __T("Libcurl support is disabled due to compilation options");
        #endif // defined(MEDIAINFO_LIBCURL_YES)
    }
    if (Option_Lower==__T("ssl_certificatefilename"))
    {
        #if defined(MEDIAINFO_LIBCURL_YES)
            Ssl_CertificateFileName_Set(Value);
            return Ztring();
        #else // defined(MEDIAINFO_LIBCURL_YES)
            return __T("Libcurl support is disabled due to compilation options");
        #endif // defined(MEDIAINFO_LIBCURL_YES)
    }
    if (Option_Lower==__T("ssl_certificateFormat"))
    {
        #if defined(MEDIAINFO_LIBCURL_YES)
            Ssl_CertificateFormat_Set(Value);
            return Ztring();
        #else // defined(MEDIAINFO_LIBCURL_YES)
            return __T("Libcurl support is disabled due to compilation options");
        #endif // defined(MEDIAINFO_LIBCURL_YES)
    }
    if (Option_Lower==__T("ssl_privatekeyfilename"))
    {
        #if defined(MEDIAINFO_LIBCURL_YES)
            Ssl_PrivateKeyFileName_Set(Value);
            return Ztring();
        #else // defined(MEDIAINFO_LIBCURL_YES)
            return __T("Libcurl support is disabled due to compilation options");
        #endif // defined(MEDIAINFO_LIBCURL_YES)
    }
    if (Option_Lower==__T("ssl_privatekeyformat"))
    {
        #if defined(MEDIAINFO_LIBCURL_YES)
            Ssl_PrivateKeyFormat_Set(Value);
            return Ztring();
        #else // defined(MEDIAINFO_LIBCURL_YES)
            return __T("Libcurl support is disabled due to compilation options");
        #endif // defined(MEDIAINFO_LIBCURL_YES)
    }
    if (Option_Lower==__T("ssl_certificateauthorityfilename"))
    {
        #if defined(MEDIAINFO_LIBCURL_YES)
            Ssl_CertificateAuthorityFileName_Set(Value);
            return Ztring();
        #else // defined(MEDIAINFO_LIBCURL_YES)
            return __T("Libcurl support is disabled due to compilation options");
        #endif // defined(MEDIAINFO_LIBCURL_YES)
    }
    if (Option_Lower==__T("ssl_certificateauthoritypath"))
    {
        #if defined(MEDIAINFO_LIBCURL_YES)
            Ssl_CertificateAuthorityPath_Set(Value);
            return Ztring();
        #else // defined(MEDIAINFO_LIBCURL_YES)
            return __T("Libcurl support is disabled due to compilation options");
        #endif // defined(MEDIAINFO_LIBCURL_YES)
    }
    if (Option_Lower==__T("ssl_certificaterevocationlistfilename"))
    {
        #if defined(MEDIAINFO_LIBCURL_YES)
            Ssl_CertificateRevocationListFileName_Set(Value);
            return Ztring();
        #else // defined(MEDIAINFO_LIBCURL_YES)
            return __T("Libcurl support is disabled due to compilation options");
        #endif // defined(MEDIAINFO_LIBCURL_YES)
    }
    if (Option_Lower==__T("ssl_ignoresecurity"))
    {
        #if defined(MEDIAINFO_LIBCURL_YES)
            Ssl_IgnoreSecurity_Set(Value.empty() || Value.To_float32());
            return Ztring();
        #else // defined(MEDIAINFO_LIBCURL_YES)
            return __T("Libcurl support is disabled due to compilation options");
        #endif // defined(MEDIAINFO_LIBCURL_YES)
    }
    if (Option_Lower==__T("trytofix"))
    {
        #if MEDIAINFO_FIXITY
            TryToFix_Set(!(Value==__T("0") || Value.empty()));
            return Ztring();
        #else //MEDIAINFO_FIXITY
            return __T("Fixity support is disabled due to compilation options");
        #endif //MEDIAINFO_FIXITY
    }
    return __T("Option not known");
}

//***************************************************************************
// Info
//***************************************************************************

//---------------------------------------------------------------------------
void MediaInfo_Config::Complete_Set (size_t NewValue)
{
    CriticalSectionLocker CSL(CS);
    Complete=NewValue;
}

size_t MediaInfo_Config::Complete_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Complete;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::BlockMethod_Set (size_t NewValue)
{
    CriticalSectionLocker CSL(CS);
    BlockMethod=NewValue;
}

size_t MediaInfo_Config::BlockMethod_Get ()
{
    CriticalSectionLocker CSL(CS);
    return BlockMethod;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::Internet_Set (size_t NewValue)
{
    CriticalSectionLocker CSL(CS);
    Internet=NewValue;
}

size_t MediaInfo_Config::Internet_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Internet;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::MultipleValues_Set (size_t NewValue)
{
    CriticalSectionLocker CSL(CS);
    MultipleValues=NewValue;
}

size_t MediaInfo_Config::MultipleValues_Get ()
{
    CriticalSectionLocker CSL(CS);
    return MultipleValues;
}

//---------------------------------------------------------------------------
#if MEDIAINFO_ADVANCED
void MediaInfo_Config::ParseOnlyKnownExtensions_Set(const Ztring &NewValue)
{
    CriticalSectionLocker CSL(CS);
    ParseOnlyKnownExtensions=NewValue;
}

Ztring MediaInfo_Config::ParseOnlyKnownExtensions_Get()
{
    CriticalSectionLocker CSL(CS);
    return ParseOnlyKnownExtensions;
}

bool MediaInfo_Config::ParseOnlyKnownExtensions_IsSet()
{
    CriticalSectionLocker CSL(CS);
    return !ParseOnlyKnownExtensions.empty();
}

set<Ztring> MediaInfo_Config::ParseOnlyKnownExtensions_GetList_Set()
{
    // Example: "+M,-dat" add all extensions for Containers (M) except "dat"

    if (!ParseOnlyKnownExtensions_IsSet())
        return set<Ztring>();

    Ztring Value=ParseOnlyKnownExtensions_Get();
    Ztring StreamKinds;
    bool DefaultList;
    set<Ztring> List;
    set<Ztring> Excluded;

    // Know extensions
    if (Value.empty() || (Value[0]==__T('1') && (Value.size()==1 || Value[1]==__T(',') || Value[1]==__T(';'))))
    {
        DefaultList=true;
        if (Value.size()>=2)
            Value.erase(0, 2);
        else
            Value.clear();
    }
    else
        DefaultList=false;

    //With custom list
    if (!Value.empty())
    {
        ZtringList ValidExtensions;
        size_t SeparatorPos=Value.find_first_of(__T(",;"));
        if (SeparatorPos==string::npos)
            List.insert(Value);
        else
        {
            ValidExtensions.Separator_Set(0, Ztring(1, Value[SeparatorPos]));
            ValidExtensions.Write(Value);
        }
        for (size_t i = 0; i<ValidExtensions.size(); i++)
        {
            if (!ValidExtensions[i].empty())
            {
                switch (ValidExtensions[i][0])
                {
                    case __T('+'):
                                    StreamKinds+=ValidExtensions[i].substr(1);
                                    DefaultList=true;
                                    continue;
                    case __T('-'):
                                    Excluded.insert(ValidExtensions[i].substr(1));
                                    DefaultList=true;
                                    continue;
                    default:;
                }
            }
            List.insert(ValidExtensions[i]);
        }
    }

    //Default list
    if (DefaultList)
    {
        InfoMap &FormatList=MediaInfoLib::Config.Format_Get();
        for (InfoMap::iterator Format=FormatList.begin(); Format!=FormatList.end(); ++Format)
            if (InfoFormat_Extensions<Format->second.size())
            {
                if (!StreamKinds.empty() && Format->second[InfoFormat_KindofFormat].find_first_of(StreamKinds)==string::npos)
                    continue;

                ZtringList ValidExtensions;
                ValidExtensions.Separator_Set(0, __T(" "));
                ValidExtensions.Write(Format->second[InfoFormat_Extensions]);
                for (size_t i=0; i<ValidExtensions.size(); i++)
                    if (Excluded.find(ValidExtensions[i])==Excluded.end())
                        List.insert(ValidExtensions[i]);
            }
    }

    return List;
}

Ztring MediaInfo_Config::ParseOnlyKnownExtensions_GetList_String()
{
    set<Ztring> Extensions=ParseOnlyKnownExtensions_GetList_Set();
    Ztring List;
    for (set<Ztring>::iterator Extension=Extensions.begin(); Extension!=Extensions.end(); ++Extension)
    {
        List+=*Extension;
        List+=__T(',');
    }
    if (!List.empty())
        List.resize(List.size()-1);

    return List;
}
#endif //MEDIAINFO_ADVANCED

//---------------------------------------------------------------------------
void MediaInfo_Config::ShowFiles_Set (const ZtringListList &NewShowFiles)
{
    CriticalSectionLocker CSL(CS);
    for  (size_t Pos=0; Pos<NewShowFiles.size(); Pos++)
    {
        const Ztring& Object=NewShowFiles.Read(Pos, 0);
             if (Object==__T("Nothing"))
            ShowFiles_Nothing=NewShowFiles.Read(Pos, 1).empty()?1:0;
        else if (Object==__T("VideoAudio"))
            ShowFiles_VideoAudio=NewShowFiles.Read(Pos, 1).empty()?1:0;
        else if (Object==__T("VideoOnly"))
            ShowFiles_VideoOnly=NewShowFiles.Read(Pos, 1).empty()?1:0;
        else if (Object==__T("AudioOnly"))
            ShowFiles_AudioOnly=NewShowFiles.Read(Pos, 1).empty()?1:0;
        else if (Object==__T("TextOnly"))
            ShowFiles_TextOnly=NewShowFiles.Read(Pos, 1).empty()?1:0;
    }
}

size_t MediaInfo_Config::ShowFiles_Nothing_Get ()
{
    CriticalSectionLocker CSL(CS);
    return ShowFiles_Nothing;
}

size_t MediaInfo_Config::ShowFiles_VideoAudio_Get ()
{
    CriticalSectionLocker CSL(CS);
    return ShowFiles_VideoAudio;
}

size_t MediaInfo_Config::ShowFiles_VideoOnly_Get ()
{
    CriticalSectionLocker CSL(CS);
    return ShowFiles_VideoOnly;
}

size_t MediaInfo_Config::ShowFiles_AudioOnly_Get ()
{
    CriticalSectionLocker CSL(CS);
    return ShowFiles_AudioOnly;
}

size_t MediaInfo_Config::ShowFiles_TextOnly_Get ()
{
    CriticalSectionLocker CSL(CS);
    return ShowFiles_TextOnly;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::ParseSpeed_Set (float32 NewValue)
{
    CriticalSectionLocker CSL(CS);
    ParseSpeed=NewValue;
}

float32 MediaInfo_Config::ParseSpeed_Get ()
{
    CriticalSectionLocker CSL(CS);
    return ParseSpeed;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::Verbosity_Set (float32 NewValue)
{
    CriticalSectionLocker CSL(CS);
    Verbosity=NewValue;
}

float32 MediaInfo_Config::Verbosity_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Verbosity;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::ReadByHuman_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    ReadByHuman=NewValue;
}

bool MediaInfo_Config::ReadByHuman_Get ()
{
    CriticalSectionLocker CSL(CS);
    return ReadByHuman;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::Legacy_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    Legacy=NewValue;
}

bool MediaInfo_Config::Legacy_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Legacy;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::LegacyStreamDisplay_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    LegacyStreamDisplay=NewValue;
}

bool MediaInfo_Config::LegacyStreamDisplay_Get ()
{
    CriticalSectionLocker CSL(CS);
    return LegacyStreamDisplay;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::SkipBinaryData_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    SkipBinaryData=NewValue;
}

bool MediaInfo_Config::SkipBinaryData_Get ()
{
    CriticalSectionLocker CSL(CS);
    return SkipBinaryData;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::Trace_Level_Set (const ZtringListList &NewTrace_Level)
{
    CriticalSectionLocker CSL(CS);

    //Global
    if (NewTrace_Level.size()==1 && NewTrace_Level[0].size()==1)
    {
        Trace_Level=NewTrace_Level[0][0].To_float32();
        if (Trace_Layers.to_ulong()==0) //if not set to a specific layer
            Trace_Layers.set();
        return;
    }

    //Per item
    else
    {
        Trace_Layers.reset();
        for (size_t Pos=0; Pos<NewTrace_Level.size(); Pos++)
        {
            if (NewTrace_Level[Pos].size()==2)
            {
                if (NewTrace_Level[Pos][0]==__T("Container1"))
                    Trace_Layers.set(0, NewTrace_Level[Pos][1].To_int64u()?true:false);
            }
        }
    }
}

float32 MediaInfo_Config::Trace_Level_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Trace_Level;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::Compat_Set (int64u NewValue)
{
    CriticalSectionLocker CSL(CS);
    Compat=NewValue;
}

int64u MediaInfo_Config::Compat_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Compat;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::Https_Set(bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    Https=NewValue;
}

bool MediaInfo_Config::Https_Get()
{
    CriticalSectionLocker CSL(CS);
    return Https;
}

//---------------------------------------------------------------------------
std::bitset<32> MediaInfo_Config::Trace_Layers_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Trace_Layers;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::Trace_TimeSection_OnlyFirstOccurrence_Set (bool Value)
{
    CriticalSectionLocker CSL(CS);
    Trace_TimeSection_OnlyFirstOccurrence=Value;
}

bool MediaInfo_Config::Trace_TimeSection_OnlyFirstOccurrence_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Trace_TimeSection_OnlyFirstOccurrence;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::Trace_Format_Set (trace_Format NewValue)
{
    CriticalSectionLocker CSL(CS);
    Trace_Format=NewValue;
}

MediaInfo_Config::trace_Format MediaInfo_Config::Trace_Format_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Trace_Format;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::Trace_Modificator_Set (const ZtringList &NewValue)
{
    ZtringList List(NewValue);
    if (List.size()!=2)
        return;
    transform(List[0].begin(), List[0].end(), List[0].begin(), (int(*)(int))tolower); //(int(*)(int)) is a patch for unix

    CriticalSectionLocker CSL(CS);
    Trace_Modificators[List[0]]=List[1]==__T("1");
}

Ztring MediaInfo_Config::Trace_Modificator_Get (const Ztring &Value)
{
    CriticalSectionLocker CSL(CS);
    std::map<Ztring, bool>::iterator ToReturn=Trace_Modificators.find(Value);
    if (ToReturn!=Trace_Modificators.end())
        return ToReturn->second?__T("1"):__T("0");
    else
        return Ztring();
}

//---------------------------------------------------------------------------
void MediaInfo_Config::Demux_Set (int8u NewValue)
{
    CriticalSectionLocker CSL(CS);
    Demux=NewValue;
}

int8u MediaInfo_Config::Demux_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Demux;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::LineSeparator_Set (const Ztring &NewValue)
{
    CriticalSectionLocker CSL(CS);
    LineSeparator=NewValue;
}

Ztring MediaInfo_Config::LineSeparator_Get ()
{
    CriticalSectionLocker CSL(CS);
    return LineSeparator;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::Version_Set (const Ztring &NewValue)
{
    CriticalSectionLocker CSL(CS);
    Version=ZtringListList(NewValue).Read(0); //Only the 1st value
}

Ztring MediaInfo_Config::Version_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Version;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::ColumnSeparator_Set (const Ztring &NewValue)
{
    CriticalSectionLocker CSL(CS);
    ColumnSeparator=NewValue;
}

Ztring MediaInfo_Config::ColumnSeparator_Get ()
{
    CriticalSectionLocker CSL(CS);
    return ColumnSeparator;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::TagSeparator_Set (const Ztring &NewValue)
{
    CriticalSectionLocker CSL(CS);
    TagSeparator=NewValue;
}

Ztring MediaInfo_Config::TagSeparator_Get ()
{
    CriticalSectionLocker CSL(CS);
    return TagSeparator;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::Quote_Set (const Ztring &NewValue)
{
    CriticalSectionLocker CSL(CS);
    Quote=NewValue;
}

Ztring MediaInfo_Config::Quote_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Quote;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::DecimalPoint_Set (const Ztring &NewValue)
{
    CriticalSectionLocker CSL(CS);
    DecimalPoint=NewValue;
}

Ztring MediaInfo_Config::DecimalPoint_Get ()
{
    CriticalSectionLocker CSL(CS);
    return DecimalPoint;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::ThousandsPoint_Set (const Ztring &NewValue)
{
    CriticalSectionLocker CSL(CS);
    ThousandsPoint=NewValue;
}

Ztring MediaInfo_Config::ThousandsPoint_Get ()
{
    CriticalSectionLocker CSL(CS);
    return ThousandsPoint;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::CarriageReturnReplace_Set (const Ztring &NewValue)
{
    CriticalSectionLocker CSL(CS);
    CarriageReturnReplace=NewValue;
}

Ztring MediaInfo_Config::CarriageReturnReplace_Get ()
{
    CriticalSectionLocker CSL(CS);
    return CarriageReturnReplace;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::StreamMax_Set (const ZtringListList &)
{
    CriticalSectionLocker CSL(CS);
    //TODO : implementation
}

Ztring MediaInfo_Config::StreamMax_Get ()
{
    CriticalSectionLocker CSL(CS);
    ZtringListList StreamMax;
    //TODO : implementation
    return StreamMax.Read();
}

//---------------------------------------------------------------------------
void MediaInfo_Config::Language_Set (const ZtringListList &NewValue)
{
    CriticalSectionLocker CSL(CS);

    //Which language to choose?
    //-Raw
         if (NewValue.size()==1 && NewValue[0].size()==1 && NewValue[0][0]==__T("raw"))
    {
        Language_Raw=true;
        Language.clear();
        //Exceptions
        Language.Write(__T("  Config_Text_ColumnSize"), __T("32"));
        Language.Write(__T("  Config_Text_Separator"), __T(" : "));
        Language.Write(__T("  Config_Text_NumberTag"), __T(" #"));
        Language.Write(__T("  Config_Text_FloatSeparator"), __T("."));
        Language.Write(__T("  Config_Text_ThousandsSeparator"), Ztring());
    }
    //-Add custom language to English language
    else
    {
        Language_Raw=false;
        //Fill base words (with English translation)
        MediaInfo_Config_DefaultLanguage(Language);
        //Add custom language to English language
        for (size_t Pos=0; Pos<NewValue.size(); Pos++)
            if (NewValue[Pos].size()>=2)
                Language.Write(NewValue[Pos][0], NewValue[Pos][1]);
            else if (NewValue[Pos].size()==1 && NewValue[0]==__T("  Config_Text_ThousandsSeparator")) // Only the thousands separator is authorized to be empty, else empty content means keeping default value
                Language.Write(NewValue[Pos][0], Ztring());
    }

    //Fill Info
    for (size_t StreamKind=0; StreamKind<Stream_Max; StreamKind++)
        if (!Info[StreamKind].empty())
            Language_Set((stream_t)StreamKind);
}

void MediaInfo_Config::Language_Set (stream_t StreamKind)
{
    //CriticalSectionLocker CSL(CS); //No, only used internaly

    //Fill Info
    for (size_t Pos=0; Pos<Info[StreamKind].size(); Pos++)
    {
        //Strings - Info_Name_Text
        Ztring ToReplace=Info[StreamKind](Pos, Info_Name);
        if (!Language_Raw && ToReplace.find(__T("/String"))!=Error)
        {
            ToReplace.FindAndReplace(__T("/String1"), Ztring());
            ToReplace.FindAndReplace(__T("/String2"), Ztring());
            ToReplace.FindAndReplace(__T("/String3"), Ztring());
            ToReplace.FindAndReplace(__T("/String4"), Ztring());
            ToReplace.FindAndReplace(__T("/String5"), Ztring());
            ToReplace.FindAndReplace(__T("/String6"), Ztring());
            ToReplace.FindAndReplace(__T("/String7"), Ztring());
            ToReplace.FindAndReplace(__T("/String8"), Ztring());
            ToReplace.FindAndReplace(__T("/String9"), Ztring());
            ToReplace.FindAndReplace(__T("/String"),  Ztring());
        }
        if (!Language_Raw && ToReplace.find(__T('/'))!=Error) //Complex values, like XXX/YYY --> We translate both XXX and YYY
        {
            Ztring ToReplace1=ToReplace.SubString(Ztring(), __T("/"));
            Ztring ToReplace2=ToReplace.SubString(__T("/"), Ztring());
            Info[StreamKind](Pos, Info_Name_Text)=Language.Get(ToReplace1);
            Info[StreamKind](Pos, Info_Name_Text)+=__T("/");
            Info[StreamKind](Pos, Info_Name_Text)+=Language.Get(ToReplace2);
        }
        else
            Info[StreamKind](Pos, Info_Name_Text)=Language.Get(ToReplace);
        //Strings - Info_Measure_Text
        Info[StreamKind](Pos, Info_Measure_Text).clear(); //I don(t know why, but if I don't do this Delphi/C# debugger make crashing the calling program
        Info[StreamKind](Pos, Info_Measure_Text)=Language.Get(Info[StreamKind](Pos, Info_Measure));
        //Slashes

    }
}

Ztring MediaInfo_Config::Language_Get ()
{
    CriticalSectionLocker CSL(CS);
    Ztring ToReturn;//TODO =Language.Read();
    return ToReturn;
}

Ztring MediaInfo_Config::Language_Get_Translate(const Ztring &Par, const Ztring &Value)
{
    const Ztring Translated = Language_Get(Par + Value);
    return Translated.find(Par.c_str()) ? Translated : Value;
}

Ztring MediaInfo_Config::Language_Get (const Ztring &Value)
{
    CriticalSectionLocker CSL(CS);

    if (Value.find(__T(" / "))==string::npos)
    {
        if (Value.size()<7 || Value.find(__T("/String"))+7!=Value.size())
            return Language.Get(Value);
        Ztring Temp(Value);
        Temp.resize(Value.size()-7);
        return Language.Get(Temp);
    }

    ZtringList List;
    List.Separator_Set(0, __T(" / "));
    List.Write(Value);

    //Per value
    for (size_t Pos=0; Pos<List.size(); Pos++)
        List[Pos]=Language.Get(List[Pos]);

    return List.Read();
}

//---------------------------------------------------------------------------
Ztring MediaInfo_Config::Language_Get (const Ztring &Count, const Ztring &Value, bool ValueIsAlwaysSame)
{
    //Integrity
    if (Count.empty() || Count.find_first_not_of(__T("0123456789.+-/*() "))!=string::npos)
        return Count;

    //Different Plurals are available or not?
    Ztring Value1=Value+__T('1');
    if (!ValueIsAlwaysSame && Language_Get(Value1)==Value1)
        ValueIsAlwaysSame=true;

    //Detecting plural form for multiple plurals
    int8u  Form=(int8u)-1;

    if (!ValueIsAlwaysSame)
    {
        //Polish has 2 plurial, Algorithm of Polish
        size_t CountI=Count.To_int32u();
        size_t Pos3=CountI/100;
        int8u  Pos2=(int8u)((CountI-Pos3*100)/10);
        int8u  Pos1=(int8u)(CountI-Pos3*100-Pos2*10);
        if (Pos3==0)
        {
            if (Pos2==0)
            {
                     if (Pos1==0 && Count.size()==1) //Only "0", not "0.xxx"
                    Form=0; //000 to 000 kanal?
                else if (Pos1<=1)
                    Form=1; //001 to 001 kanal
                else if (Pos1<=4)
                    Form=2; //002 to 004 kanaly
                else //if (Pos1>=5)
                    Form=3; //005 to 009 kanalow
            }
            else if (Pos2==1)
                    Form=3; //010 to 019 kanalow
            else //if (Pos2>=2)
            {
                     if (Pos1<=1)
                    Form=3; //020 to 021, 090 to 091 kanalow
                else if (Pos1<=4)
                    Form=2; //022 to 024, 092 to 094 kanali
                else //if (Pos1>=5)
                    Form=3; //025 to 029, 095 to 099 kanalow
            }
        }
        else //if (Pos3>=1)
        {
            if (Pos2==0)
            {
                     if (Pos1<=1)
                    Form=3; //100 to 101 kanalow
                else if (Pos1<=4)
                    Form=2; //102 to 104 kanaly
                else //if (Pos1>=5)
                    Form=3; //105 to 109 kanalow
            }
            else if (Pos2==1)
                    Form=3; //110 to 119 kanalow
            else //if (Pos2>=2)
            {
                     if (Pos1<=1)
                    Form=3; //120 to 121, 990 to 991 kanalow
                else if (Pos1<=4)
                    Form=2; //122 to 124, 992 to 994 kanali
                else //if (Pos1>=5)
                    Form=3; //125 to 129, 995 to 999 kanalow
            }
        }
    }

    //Replace dot and thousand separator
    Ztring ToReturn=Count;
    Ztring DecimalPoint=Ztring().From_Number(0.0, 1).substr(1, 1); //Getting Decimal point
    size_t DotPos=ToReturn.find(DecimalPoint);
    if (DotPos!=string::npos)
        ToReturn.FindAndReplace(DecimalPoint, Language_Get(__T("  Config_Text_FloatSeparator")), DotPos);
    else
        DotPos=ToReturn.size();
    if (DotPos>3 && ToReturn[0]==__T('-'))
        DotPos--;
    if (DotPos>3)
        ToReturn.insert(DotPos-3, Language_Get(__T("  Config_Text_ThousandsSeparator")));

    //Selecting the form
         if (Form==0)
        ToReturn =Language_Get(Value+__T("0")); //Only the translation
    else if (Form==1)
        ToReturn+=Language_Get(Value+__T("1"));
    else if (Form==2)
        ToReturn+=Language_Get(Value+__T("2"));
    else if (Form==3)
        ToReturn+=Language_Get(Value+__T("3"));
    else
        ToReturn+=Language_Get(Value);
    return ToReturn;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::Inform_Set (const ZtringListList &NewValue)
{
    if (NewValue.Read(0, 0)==__T("Details"))
        Trace_Level_Set(NewValue.Read(0, 1));
    else if (Trace_Level_Get() && NewValue.Read(0, 0)==__T("XML"))
    {
        Trace_Format_Set(Trace_Format_XML); // TODO: better coherency in options
        return;
    }
    else if (Trace_Level_Get() && NewValue.Read(0, 0)==__T("MICRO_XML"))
    {
        Trace_Format_Set(Trace_Format_MICRO_XML);
        return;
    }
    else
    {
        if (NewValue.Read(0, 0)==__T("MAXML"))
            Trace_Format_Set(Trace_Format_XML); //All must be XML
        else
            Trace_Format_Set(Trace_Format_Tree); // TODO: better coherency in options

        CriticalSectionLocker CSL(CS);

        //Inform
        if (NewValue==__T("Summary"))
            MediaInfo_Config_Summary(Custom_View);
        else
            Custom_View=NewValue;
    }

    CriticalSectionLocker CSL(CS);

    //Parsing pointers to files in streams
    #if defined(MEDIAINFO_FILE_YES)
    for (size_t Pos=0; Pos<Custom_View.size(); Pos++)
    {
        if (Custom_View[Pos].size()>1 && Custom_View(Pos, 1).find(__T("file://"))==0)
        {
            //Open
            Ztring FileName(Custom_View(Pos, 1), 7, Ztring::npos);
            File F(FileName.c_str());

            //Read
            int64u Size=F.Size_Get();
            if (Size>=0xFFFFFFFF)
                Size=1024*1024;
            int8u* Buffer=new int8u[(size_t)Size+1];
            size_t F_Offset=F.Read(Buffer, (size_t)Size);
            F.Close();
            Buffer[F_Offset]='\0';
            Ztring FromFile; FromFile.From_UTF8((char*)Buffer);
            delete[] Buffer; //Buffer=NULL;

            //Merge
            FromFile.FindAndReplace(__T("\r\n"), __T("\\r\\n"), 0, Ztring_Recursive);
            FromFile.FindAndReplace(__T("\n"), __T("\\r\\n"), 0, Ztring_Recursive);
            Custom_View(Pos, 1)=FromFile;
        }
    }
    #endif //defined(MEDIAINFO_FILE_YES)
}

Ztring MediaInfo_Config::Inform_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Custom_View.Read();
}

Ztring MediaInfo_Config::Inform_Get (const Ztring &Value)
{
    CriticalSectionLocker CSL(CS);
    size_t Pos=Custom_View.Find(Value);
    if (Pos==Error || 1>=Custom_View[Pos].size())
        return EmptyString_Get();
    return Custom_View[Pos][1];
}

//---------------------------------------------------------------------------
void MediaInfo_Config::Inform_Replace_Set (const ZtringListList &NewValue_Replace)
{
    CriticalSectionLocker CSL(CS);

    //Parsing
    for (size_t Pos=0; Pos<NewValue_Replace.size(); Pos++)
    {
        if (NewValue_Replace[Pos].size()==2)
            Custom_View_Replace(NewValue_Replace[Pos][0])=NewValue_Replace[Pos][1];
    }
}

ZtringListList MediaInfo_Config::Inform_Replace_Get_All ()
{
    CriticalSectionLocker CSL(CS);
    return Custom_View_Replace;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::Inform_Version_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    Inform_Version=NewValue;
}

bool MediaInfo_Config::Inform_Version_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Inform_Version;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::Inform_Timestamp_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    Inform_Timestamp=NewValue;
}

bool MediaInfo_Config::Inform_Timestamp_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Inform_Timestamp;
}

//---------------------------------------------------------------------------
#if MEDIAINFO_ADVANCED
Ztring MediaInfo_Config::Cover_Data_Set (const Ztring &NewValue_)
{
    Ztring NewValue(NewValue_);
    transform(NewValue.begin(), NewValue.end(), NewValue.begin(), (int(*)(int))tolower); //(int(*)(int)) is a patch for unix
    const int64u Mask=~((1<<Flags_Cover_Data_base64));
    int64u Value;
    if (NewValue.empty())
        Value=0;
    else if (NewValue==__T("base64"))
        Value=(1<< Flags_Cover_Data_base64);
    else
        return __T("Unsupported");

    CriticalSectionLocker CSL(CS);
    Flags1&=Mask;
    Flags1|=Value;
    return Ztring();
}

Ztring MediaInfo_Config::Cover_Data_Get ()
{
    CriticalSectionLocker CSL(CS);
    Ztring ToReturn;
    if (Flags1&(1<< Flags_Cover_Data_base64))
        ToReturn=__T("base64");

    return ToReturn;
}
#endif //MEDIAINFO_ADVANCED


//---------------------------------------------------------------------------
#if MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)
Ztring MediaInfo_Config::Enable_FFmpeg_Set (bool NewValue)
{
    const int64u Mask=~((1<<Flags_Enable_FFmpeg));
    int64u Value;
    if (NewValue)
        Value=(1<<Flags_Enable_FFmpeg);
    else
        Value=0;

    CriticalSectionLocker CSL(CS);
    Flags1&=Mask;
    Flags1|=Value;
    return Ztring();
}

bool MediaInfo_Config::Enable_FFmpeg_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Flags1&(1<<Flags_Enable_FFmpeg);
}
#endif //MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)

//---------------------------------------------------------------------------
#if MEDIAINFO_COMPRESS
Ztring MediaInfo_Config::Inform_Compress_Set (const Ztring &NewValue_)
{
    Ztring NewValue(NewValue_);
    transform(NewValue.begin(), NewValue.end(), NewValue.begin(), (int(*)(int))tolower); //(int(*)(int)) is a patch for unix
    const int64u Mask=~((1<<Flags_Inform_zlib)|(1<<Flags_Inform_base64));
    int64u Value;
    if (NewValue.empty())
        Value=0;
    else if (NewValue== __T("base64"))
        Value=(1<<Flags_Inform_base64);
    else if (NewValue== __T("zlib+base64"))
        Value=(1<<Flags_Inform_zlib)|(1<<Flags_Inform_base64);
    else
        return __T("Unsupported");

    CriticalSectionLocker CSL(CS);
    FlagsX&=Mask;
    FlagsX|=Value;
    return Ztring();
}

Ztring MediaInfo_Config::Inform_Compress_Get ()
{
    CriticalSectionLocker CSL(CS);
    Ztring ToReturn;
    if (FlagsX&(1<<Flags_Inform_zlib))
        ToReturn=__T("zlib");
    if (FlagsX&(1<<Flags_Inform_base64))
    {
        if (!ToReturn.empty())
            ToReturn+=__T('+');
        ToReturn+=__T("base64");
    }

    return ToReturn;
}
#endif //MEDIAINFO_COMPRESS

//---------------------------------------------------------------------------
#if MEDIAINFO_COMPRESS
Ztring MediaInfo_Config::Input_Compressed_Set (const Ztring &NewValue_)
{
    Ztring NewValue(NewValue_);
    transform(NewValue.begin(), NewValue.end(), NewValue.begin(), (int(*)(int))tolower); //(int(*)(int)) is a patch for unix
    const int64u Mask=~((1<<Flags_Input_zlib)|(1<<Flags_Input_base64));
    int64u Value;
    if (NewValue.empty())
        Value=0;
    else if (NewValue== __T("zlib"))
        Value=(1<<Flags_Input_zlib);
    else if (NewValue== __T("base64"))
        Value=(1<<Flags_Input_base64);
    else if (NewValue== __T("zlib+base64"))
        Value=(1<<Flags_Input_zlib)|(1<<Flags_Input_base64);
    else
        return __T("Unsupported");

    CriticalSectionLocker CSL(CS);
    FlagsX&=Mask;
    FlagsX|=Value;
    return Ztring();
}

Ztring MediaInfo_Config::Input_Compressed_Get ()
{
    CriticalSectionLocker CSL(CS);
    Ztring ToReturn;
    if (FlagsX&(1<<Flags_Input_zlib))
        ToReturn=__T("zlib");
    if (FlagsX&(1<<Flags_Input_base64))
    {
        if (!ToReturn.empty())
            ToReturn+=__T('+');
        ToReturn+=__T("base64");
    }

    return ToReturn;
}
#endif //MEDIAINFO_COMPRESS

//---------------------------------------------------------------------------
const Ztring &MediaInfo_Config::Format_Get (const Ztring &Value, infoformat_t KindOfFormatInfo)
{
    //Loading codec table if not yet done
    {
        CriticalSectionLocker CSL(CS);
    if (Format.empty())
        MediaInfo_Config_Format(Format);
    }

    return Format.Get(Value, KindOfFormatInfo);
}

//---------------------------------------------------------------------------
InfoMap &MediaInfo_Config::Format_Get ()
{
    //Loading codec table if not yet done
    {
        CriticalSectionLocker CSL(CS);
    if (Format.empty())
        MediaInfo_Config_Format(Format);
    }

    return Format;
}

//---------------------------------------------------------------------------
const Ztring &MediaInfo_Config::Codec_Get (const Ztring &Value, infocodec_t KindOfCodecInfo)
{
    //Loading codec table if not yet done
    {
        CriticalSectionLocker CSL(CS);
    if (Codec.empty())
        MediaInfo_Config_Codec(Codec);
    }

    return Codec.Get(Value, KindOfCodecInfo);
}

//---------------------------------------------------------------------------
const Ztring &MediaInfo_Config::Codec_Get (const Ztring &Value, infocodec_t KindOfCodecInfo, stream_t KindOfStream)
{
    //Loading codec table if not yet done
    {
    CriticalSectionLocker CSL(CS);
    if (Codec.empty())
        MediaInfo_Config_Codec(Codec);
    }

    //Transform to text
    Ztring KindOfStreamS;
    switch (KindOfStream)
    {
        case Stream_General  : KindOfStreamS=__T("G"); break;
        case Stream_Video    : KindOfStreamS=__T("V"); break;
        case Stream_Audio    : KindOfStreamS=__T("A"); break;
        case Stream_Text     : KindOfStreamS=__T("T"); break;
        case Stream_Image    : KindOfStreamS=__T("I"); break;
        case Stream_Other : KindOfStreamS=__T("C"); break;
        case Stream_Menu     : KindOfStreamS=__T("M"); break;
        case Stream_Max      : KindOfStreamS=__T(" "); break;
    }

    return Codec.Get(Value, KindOfCodecInfo, KindOfStreamS, InfoCodec_KindOfStream);
}

//---------------------------------------------------------------------------
const Ztring &MediaInfo_Config::CodecID_Get (stream_t KindOfStream, infocodecid_format_t Format, const Ztring &Value, infocodecid_t KindOfCodecIDInfo)
{
    if (Format>=InfoCodecID_Format_Max || KindOfStream>=Stream_Max)
        return EmptyString_Get();
    {
    CriticalSectionLocker CSL(CS);
    if (CodecID[Format][KindOfStream].empty())
    {
        switch (KindOfStream)
        {
            case Stream_General :
                                    switch (Format)
                                    {
                                        case InfoCodecID_Format_Mpeg4 : MediaInfo_Config_CodecID_General_Mpeg4(CodecID[Format][KindOfStream]); break;
                                        default: ;
                                    }
                                    break;
            case Stream_Video   :
                                    switch (Format)
                                    {
                                        case InfoCodecID_Format_Matroska : MediaInfo_Config_CodecID_Video_Matroska(CodecID[Format][KindOfStream]); break;
                                        case InfoCodecID_Format_Mpeg4    : MediaInfo_Config_CodecID_Video_Mpeg4(CodecID[Format][KindOfStream]); break;
                                        case InfoCodecID_Format_Real     : MediaInfo_Config_CodecID_Video_Real(CodecID[Format][KindOfStream]); break;
                                        case InfoCodecID_Format_Riff     : MediaInfo_Config_CodecID_Video_Riff(CodecID[Format][KindOfStream]); break;
                                        default: ;
                                    }
                                    break;
            case Stream_Audio   :
                                    switch (Format)
                                    {
                                        case InfoCodecID_Format_Matroska : MediaInfo_Config_CodecID_Audio_Matroska(CodecID[Format][KindOfStream]); break;
                                        case InfoCodecID_Format_Mpeg4    : MediaInfo_Config_CodecID_Audio_Mpeg4(CodecID[Format][KindOfStream]); break;
                                        case InfoCodecID_Format_Real     : MediaInfo_Config_CodecID_Audio_Real(CodecID[Format][KindOfStream]); break;
                                        case InfoCodecID_Format_Riff     : MediaInfo_Config_CodecID_Audio_Riff(CodecID[Format][KindOfStream]); break;
                                        default: ;
                                    }
                                    break;
            case Stream_Text    :
                                    switch (Format)
                                    {
                                        case InfoCodecID_Format_Matroska : MediaInfo_Config_CodecID_Text_Matroska(CodecID[Format][KindOfStream]); break;
                                        case InfoCodecID_Format_Mpeg4    : MediaInfo_Config_CodecID_Text_Mpeg4(CodecID[Format][KindOfStream]); break;
                                        case InfoCodecID_Format_Riff     : MediaInfo_Config_CodecID_Text_Riff(CodecID[Format][KindOfStream]); break;
                                        default: ;
                                    }
                                    break;
            case Stream_Other   :
                                    switch (Format)
                                    {
                                        case InfoCodecID_Format_Mpeg4    : MediaInfo_Config_CodecID_Other_Mpeg4(CodecID[Format][KindOfStream]); break;
                                        default: ;
                                    }
                                    break;
            default: ;
        }
    }
    }
    return CodecID[Format][KindOfStream].Get(Value, KindOfCodecIDInfo);
}

//---------------------------------------------------------------------------
const Ztring &MediaInfo_Config::Library_Get (infolibrary_format_t Format, const Ztring &Value, infolibrary_t KindOfLibraryInfo)
{
    if (Format>=InfoLibrary_Format_Max)
        return EmptyString_Get();
    {
    CriticalSectionLocker CSL(CS);
    if (Library[Format].empty())
    {
        switch (Format)
        {
            case InfoLibrary_Format_DivX : MediaInfo_Config_Library_DivX(Library[Format]); break;
            case InfoLibrary_Format_XviD : MediaInfo_Config_Library_XviD(Library[Format]); break;
            case InfoLibrary_Format_MainConcept_Avc : MediaInfo_Config_Library_MainConcept_Avc(Library[Format]); break;
            case InfoLibrary_Format_VorbisCom : MediaInfo_Config_Library_VorbisCom(Library[Format]); break;
            default: ;
        }
    }
    }
    return Library[Format].Get(Value, KindOfLibraryInfo);
}

//---------------------------------------------------------------------------
const Ztring &MediaInfo_Config::Iso639_1_Get (const Ztring &Value)
{
    //Loading codec table if not yet done
    {
        CriticalSectionLocker CSL(CS);
    if (Iso639_1.empty())
        MediaInfo_Config_Iso639_1(Iso639_1);
    }

    return Iso639_1.Get(Ztring(Value).MakeLowerCase(), 1);
}

//---------------------------------------------------------------------------
const Ztring &MediaInfo_Config::Iso639_2_Get (const Ztring &Value)
{
    //Loading codec table if not yet done
    {
        CriticalSectionLocker CSL(CS);
    if (Iso639_2.empty())
        MediaInfo_Config_Iso639_2(Iso639_2);
    }

    return Iso639_2.Get(Ztring(Value).MakeLowerCase(), 1);
}

//---------------------------------------------------------------------------
const Ztring MediaInfo_Config::Iso639_Find (const Ztring &Value)
{
    Translation Info;
    MediaInfo_Config_DefaultLanguage (Info);
    Ztring Value_Lower(Value);
    Value_Lower.MakeLowerCase();

    for (Translation::iterator Trans=Info.begin(); Trans!=Info.end(); ++Trans)
    {
        Trans->second.MakeLowerCase();
        if (Trans->second==Value_Lower && Trans->first.find(__T("Language_"))==0)
            return Trans->first.substr(9, string::npos);
    }
    return Ztring();
}

//---------------------------------------------------------------------------
const Ztring MediaInfo_Config::Iso639_Translate (const Ztring &Value)
{
    Ztring Code(Value);
    if (Code.size()==3 && !MediaInfoLib::Config.Iso639_1_Get(Code).empty())
        Code=MediaInfoLib::Config.Iso639_1_Get(Code);
    if (Code.size()>3 && !MediaInfoLib::Config.Iso639_Find(Code).empty())
        Code=MediaInfoLib::Config.Iso639_Find(Code);
    if (Code.size()>3)
        return Value;
    Ztring Language_Translated=MediaInfoLib::Config.Language_Get(__T("Language_")+Code);
    if (Language_Translated.find(__T("Language_"))==0)
        return Value; //No translation found
    return Language_Translated;
}

//---------------------------------------------------------------------------
void MediaInfo_Config::Language_Set_Internal(stream_t KindOfStream)
{
    //Loading codec table if not yet done
    if (KindOfStream<Stream_Max && Info[KindOfStream].empty())
        switch (KindOfStream)
        {
            case Stream_General :   MediaInfo_Config_General(Info[Stream_General]);   Language_Set(Stream_General); break;
            case Stream_Video :     MediaInfo_Config_Video(Info[Stream_Video]);       Language_Set(Stream_Video); break;
            case Stream_Audio :     MediaInfo_Config_Audio(Info[Stream_Audio]);       Language_Set(Stream_Audio); break;
            case Stream_Text :      MediaInfo_Config_Text(Info[Stream_Text]);         Language_Set(Stream_Text); break;
            case Stream_Other :     MediaInfo_Config_Other(Info[Stream_Other]);       Language_Set(Stream_Other); break;
            case Stream_Image :     MediaInfo_Config_Image(Info[Stream_Image]);       Language_Set(Stream_Image); break;
            case Stream_Menu :      MediaInfo_Config_Menu(Info[Stream_Menu]);         Language_Set(Stream_Menu); break;
        default:;
        }
}

//---------------------------------------------------------------------------
const Ztring &MediaInfo_Config::Info_Get (stream_t KindOfStream, const Ztring &Value, info_t KindOfInfo)
{
    Language_Set_All(KindOfStream);
    if (KindOfStream>=Stream_Max)
        return EmptyString_Get();
    size_t Pos=Info[KindOfStream].Find(Value);
    if (Pos==Error || (size_t)KindOfInfo>=Info[KindOfStream][Pos].size())
        return EmptyString_Get();
    return Info[KindOfStream][Pos][KindOfInfo];
}

const Ztring &MediaInfo_Config::Info_Get (stream_t KindOfStream, size_t Pos, info_t KindOfInfo)
{
    Language_Set_All(KindOfStream);

    if (KindOfStream>=Stream_Max)
        return EmptyString_Get();
    if (Pos>=Info[KindOfStream].size() || (size_t)KindOfInfo>=Info[KindOfStream][Pos].size())
        return EmptyString_Get();
    return Info[KindOfStream][Pos][KindOfInfo];
}

const ZtringListList &MediaInfo_Config::Info_Get(stream_t KindOfStream)
{
    if (KindOfStream>=Stream_Max)
        return EmptyStringListList_Get();

    Language_Set_All(KindOfStream);

    return Info[KindOfStream];
}

//---------------------------------------------------------------------------
Ztring MediaInfo_Config::Info_Parameters_Get (bool Complete)
{
    ZtringListList ToReturn;
    {
    CriticalSectionLocker CSL(CS);

    //Loading all
    MediaInfo_Config_General(Info[Stream_General]);
    MediaInfo_Config_Video(Info[Stream_Video]);
    MediaInfo_Config_Audio(Info[Stream_Audio]);
    MediaInfo_Config_Text(Info[Stream_Text]);
    MediaInfo_Config_Other(Info[Stream_Other]);
    MediaInfo_Config_Image(Info[Stream_Image]);
    MediaInfo_Config_Menu(Info[Stream_Menu]);

    //Building
    size_t ToReturn_Pos=0;

    for (size_t StreamKind=0; StreamKind<Stream_Max; StreamKind++)
    {
        ToReturn(ToReturn_Pos, 0)=Info[StreamKind].Read(__T("StreamKind"), Info_Text);
        ToReturn_Pos++;
        for (size_t Pos=0; Pos<Info[StreamKind].size(); Pos++)
            if (!Info[StreamKind].Read(Pos, Info_Name).empty())
            {
                if (Complete)
                    ToReturn.push_back(Info[StreamKind].Read(Pos));
                else
                {
                    ToReturn(ToReturn_Pos, 0)=Info[StreamKind].Read(Pos, Info_Name);
                    ToReturn(ToReturn_Pos, 1)=Info[StreamKind].Read(Pos, Info_Info);
                }
                ToReturn_Pos++;
            }
        ToReturn_Pos++;
    }
    }

    //Reset of language file
    Language_Set(Ztring()); //TODO: it is reseted to English, it should actually not modify the language config (MediaInfo_Config_xxx() modifies the language config)

    return ToReturn.Read();
}

//---------------------------------------------------------------------------
Ztring MediaInfo_Config::HideShowParameter(const Ztring &Value, Char Show)
{
    ZtringList List;
    List.Separator_Set(0, __T(","));
    List.Write(Value);

    for (size_t j=0; j<List.size(); j++)
    {
        String KindOfStreamS=List[j].substr(0, List[j].find(__T('_')));
        stream_t StreamKind=Stream_Max;
        if (KindOfStreamS==__T("General")) StreamKind=Stream_General;
        if (KindOfStreamS==__T("Video")) StreamKind=Stream_Video;
        if (KindOfStreamS==__T("Audio")) StreamKind= Stream_Audio;
        if (KindOfStreamS==__T("Text")) StreamKind= Stream_Text;
        if (KindOfStreamS==__T("Other")) StreamKind= Stream_Other;
        if (KindOfStreamS==__T("Image")) StreamKind= Stream_Image;
        if (KindOfStreamS==__T("Menu")) StreamKind= Stream_Menu;
        if (StreamKind==Stream_Max)
            return List[j]+=__T(" is unknown");

        //Loading codec table if not yet done
        {
        CriticalSectionLocker CSL(CS);
        if (Info[StreamKind].empty())
            switch (StreamKind)
            {
                case Stream_General :   MediaInfo_Config_General(Info[Stream_General]);   Language_Set(Stream_General); break;
                case Stream_Video :     MediaInfo_Config_Video(Info[Stream_Video]);       Language_Set(Stream_Video); break;
                case Stream_Audio :     MediaInfo_Config_Audio(Info[Stream_Audio]);       Language_Set(Stream_Audio); break;
                case Stream_Text :      MediaInfo_Config_Text(Info[Stream_Text]);         Language_Set(Stream_Text); break;
                case Stream_Other :     MediaInfo_Config_Other(Info[Stream_Other]);       Language_Set(Stream_Other); break;
                case Stream_Image :     MediaInfo_Config_Image(Info[Stream_Image]);       Language_Set(Stream_Image); break;
                case Stream_Menu :      MediaInfo_Config_Menu(Info[Stream_Menu]);         Language_Set(Stream_Menu); break;
                default:;
            }
        }

        String FieldName=List[j].substr(List[j].find(__T('_'))+1);
        bool Found=false;
        for (size_t i=0; i<Info[StreamKind].size(); i++)
            if (Info[StreamKind][i](Info_Name)==FieldName)
            {
                if (Info_Options<Info[StreamKind][i].size())
                {
                    Info[StreamKind][i][Info_Options].resize(InfoOption_Max, __T(' '));
                    Info[StreamKind][i][Info_Options][InfoOption_ShowInInform]=Show;
                    Info[StreamKind][i][Info_Options][InfoOption_ShowInXml]=Show;
                }
                Found=true;
                break;
            }
        if (!Found)
            return List[j]+=__T(" is unknown");
    }

    return Ztring();
}

//---------------------------------------------------------------------------
Ztring MediaInfo_Config::Info_OutputFormats_Get(basicformat Format)
{
    switch (Format)
    {
        case BasicFormat_Text:
                                {
                                ZtringListList ToReturn;
                                for (size_t i = 0; i < output_formats_size; i++)
                                    for (size_t j = 0; j < output_formats_item_size; j++)
                                        ToReturn(i, j).From_UTF8(OutputFormats[i][j]);

                                size_t max_len = 0;
                                for (size_t i = 0; i < ToReturn.size(); i++)
                                    max_len = std::max(max_len, ToReturn(i, 0).size());
                                //Adapt first column
                                for (size_t Pos = 0; Pos<ToReturn.size(); Pos++)
                                {
                                    Ztring &C1 = ToReturn(Pos, 0);
                                    if (!ToReturn(Pos, 1).empty())
                                    {
                                        C1.resize(max_len + 1, ' ');
                                        C1 += __T(':');
                                    }
                                }
                                ToReturn.Separator_Set(0, LineSeparator_Get());
                                ToReturn.Separator_Set(1, __T(" "));
                                ToReturn.Quote_Set(Ztring());
                                return ToReturn.Read();
                                }
        case BasicFormat_CSV:
                                {
                                ZtringListList ToReturn;
                                for (size_t i = 0; i < output_formats_size; i++)
                                    for (size_t j = 0; j < output_formats_item_size; j++)
                                        ToReturn(i, j).From_UTF8(OutputFormats[i][j]);

                                ToReturn.Separator_Set(0, EOL);
                                ToReturn.Separator_Set(1, ",");
                                return ToReturn.Read();
                                }
        case BasicFormat_JSON:
                                {
                                string ToReturn("{\"output\":[");
                                for (size_t i = 0; i < output_formats_size; i++)
                                {
                                    ToReturn += "{";
                                    for (size_t j = 0; j < output_formats_item_size; j++)
                                    {
                                        ToReturn += "\"";
                                        ToReturn += OutputFormats_JSONFields[j];
                                        ToReturn += "\":\"";
                                        ToReturn += OutputFormats[i][j];
                                        ToReturn += (j + 1 < output_formats_item_size)?"\",":"\"";
                                    }
                                    ToReturn += (i + 1 < output_formats_size)?"},":"}";
                                }
                                ToReturn += "]}";
                                return Ztring().From_UTF8(ToReturn.c_str());
                                }
        default: return String();
    }
}

//---------------------------------------------------------------------------
Ztring MediaInfo_Config::Info_Tags_Get () const
{
    return Ztring();
}

Ztring MediaInfo_Config::Info_Codecs_Get ()
{
    CriticalSectionLocker CSL(CS);

    //Loading
    MediaInfo_Config_Codec(Codec);

    //Building
    Ztring ToReturn;
    InfoMap::iterator Temp=Codec.begin();
    while (Temp!=Codec.end())
    {
        ToReturn+=Temp->second.Read();
        ToReturn+=EOL;
        ++Temp;
    }

    return ToReturn;
}

Ztring MediaInfo_Config::Info_Version_Get () const
{
    return MediaInfo_Version;
}

Ztring MediaInfo_Config::Info_Url_Get () const
{
    return MediaInfo_Url;
}

const Ztring &MediaInfo_Config::EmptyString_Get () const
{
    return EmptyZtring_Const;
}

const ZtringListList &MediaInfo_Config::EmptyStringListList_Get () const
{
    return EmptyZtringListList_Const;
}

void MediaInfo_Config::FormatDetection_MaximumOffset_Set (int64u Value)
{
    CriticalSectionLocker CSL(CS);
    FormatDetection_MaximumOffset=Value;
}

int64u MediaInfo_Config::FormatDetection_MaximumOffset_Get ()
{
    CriticalSectionLocker CSL(CS);
    return FormatDetection_MaximumOffset;
}

#if MEDIAINFO_ADVANCED
void MediaInfo_Config::VariableGopDetection_Occurences_Set (int64u Value)
{
    CriticalSectionLocker CSL(CS);
    VariableGopDetection_Occurences=Value;
}

int64u MediaInfo_Config::VariableGopDetection_Occurences_Get ()
{
    CriticalSectionLocker CSL(CS);
    return VariableGopDetection_Occurences;
}
#endif // MEDIAINFO_ADVANCED

#if MEDIAINFO_ADVANCED
void MediaInfo_Config::VariableGopDetection_GiveUp_Set (bool Value)
{
    CriticalSectionLocker CSL(CS);
    VariableGopDetection_GiveUp=Value;
}

bool MediaInfo_Config::VariableGopDetection_GiveUp_Get ()
{
    CriticalSectionLocker CSL(CS);
    return VariableGopDetection_GiveUp;
}
#endif // MEDIAINFO_ADVANCED

#if MEDIAINFO_ADVANCED
void MediaInfo_Config::InitDataNotRepeated_Occurences_Set (int64u Value)
{
    CriticalSectionLocker CSL(CS);
    InitDataNotRepeated_Occurences=Value;
}

int64u MediaInfo_Config::InitDataNotRepeated_Occurences_Get ()
{
    CriticalSectionLocker CSL(CS);
    return InitDataNotRepeated_Occurences;
}
#endif // MEDIAINFO_ADVANCED

#if MEDIAINFO_ADVANCED
void MediaInfo_Config::InitDataNotRepeated_GiveUp_Set (bool Value)
{
    CriticalSectionLocker CSL(CS);
    InitDataNotRepeated_GiveUp=Value;
}

bool MediaInfo_Config::InitDataNotRepeated_GiveUp_Get ()
{
    CriticalSectionLocker CSL(CS);
    return InitDataNotRepeated_GiveUp;
}
#endif // MEDIAINFO_ADVANCED


#if MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)
void MediaInfo_Config::TimeOut_Set (int64u Value)
{
    CriticalSectionLocker CSL(CS);
    TimeOut=Value;
}

int64u MediaInfo_Config::TimeOut_Get ()
{
    CriticalSectionLocker CSL(CS);
    return TimeOut;
}

void MediaInfo_Config::AcceptSignals_Set (bool Value)
{
    CriticalSectionLocker CSL(CS);
    AcceptSignals=Value;
}

bool MediaInfo_Config::AcceptSignals_Get ()
{
    CriticalSectionLocker CSL(CS);
    return AcceptSignals;
}
#endif // MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)

void MediaInfo_Config::MpegTs_MaximumOffset_Set (int64u Value)
{
    CriticalSectionLocker CSL(CS);
    MpegTs_MaximumOffset=Value;
}

int64u MediaInfo_Config::MpegTs_MaximumOffset_Get ()
{
    CriticalSectionLocker CSL(CS);
    return MpegTs_MaximumOffset;
}

void MediaInfo_Config::MpegTs_MaximumScanDuration_Set (int64u Value)
{
    CriticalSectionLocker CSL(CS);
    MpegTs_MaximumScanDuration=Value;
}

int64u MediaInfo_Config::MpegTs_MaximumScanDuration_Get ()
{
    CriticalSectionLocker CSL(CS);
    return MpegTs_MaximumScanDuration;
}

#if MEDIAINFO_ADVANCED
void MediaInfo_Config::MpegTs_VbrDetection_Delta_Set (float64 Value)
{
    CriticalSectionLocker CSL(CS);
    MpegTs_VbrDetection_Delta=Value;
}

float64 MediaInfo_Config::MpegTs_VbrDetection_Delta_Get ()
{
    CriticalSectionLocker CSL(CS);
    return MpegTs_VbrDetection_Delta;
}
#endif // MEDIAINFO_ADVANCED

#if MEDIAINFO_ADVANCED
void MediaInfo_Config::MpegTs_VbrDetection_Occurences_Set (int64u Value)
{
    CriticalSectionLocker CSL(CS);
    MpegTs_VbrDetection_Occurences=Value;
}

int64u MediaInfo_Config::MpegTs_VbrDetection_Occurences_Get ()
{
    CriticalSectionLocker CSL(CS);
    return MpegTs_VbrDetection_Occurences;
}
#endif // MEDIAINFO_ADVANCED

#if MEDIAINFO_ADVANCED
void MediaInfo_Config::MpegTs_VbrDetection_GiveUp_Set (bool Value)
{
    CriticalSectionLocker CSL(CS);
    MpegTs_VbrDetection_GiveUp=Value;
}

bool MediaInfo_Config::MpegTs_VbrDetection_GiveUp_Get ()
{
    CriticalSectionLocker CSL(CS);
    return MpegTs_VbrDetection_GiveUp;
}
#endif // MEDIAINFO_ADVANCED

void MediaInfo_Config::MpegTs_ForceStreamDisplay_Set (bool Value)
{
    CriticalSectionLocker CSL(CS);
    MpegTs_ForceStreamDisplay=Value;
}

bool MediaInfo_Config::MpegTs_ForceStreamDisplay_Get ()
{
    CriticalSectionLocker CSL(CS);
    return MpegTs_ForceStreamDisplay;
}

void MediaInfo_Config::MpegTs_ForceTextStreamDisplay_Set(bool Value)
{
    CriticalSectionLocker CSL(CS);
    MpegTs_ForceTextStreamDisplay = Value;
}

bool MediaInfo_Config::MpegTs_ForceTextStreamDisplay_Get()
{
    CriticalSectionLocker CSL(CS);
    return MpegTs_ForceTextStreamDisplay;
}

#if MEDIAINFO_ADVANCED
Ztring MediaInfo_Config::MAXML_StreamKinds_Get ()
{
    ZtringList List;
    CriticalSectionLocker CSL(CS);
    for (size_t StreamKind=0; StreamKind<Stream_Max; StreamKind++)
    {
        Language_Set_Internal(stream_t(StreamKind));
        List.push_back(Info[StreamKind](__T("StreamKind"), Info_Name, Info_Text));
    }

    List.Separator_Set(0, __T(","));
    return List.Read();
}
#endif // MEDIAINFO_ADVANCED

#if MEDIAINFO_ADVANCED
extern Ztring Xml_Name_Escape_0_7_78 (const Ztring &Name);
Ztring MediaInfo_Config::MAXML_Fields_Get (const Ztring &StreamKind_String)
{
    CriticalSectionLocker CSL(CS);

    size_t StreamKind=Stream_General;
    for (; StreamKind<Stream_Max; StreamKind++)
    {
        Language_Set_Internal(stream_t(StreamKind));

        if (StreamKind_String==Info[StreamKind](__T("StreamKind"), Info_Name, Info_Text))
            break;
    }

    if (StreamKind>=Stream_Max)
        return Ztring();

    ZtringList List;
    for (size_t FieldPos = 0; FieldPos < Info[StreamKind].size(); FieldPos++)
        if (Info_Options < Info[StreamKind][FieldPos].size()
         && InfoOption_ShowInXml < Info[StreamKind][FieldPos][Info_Options].size()
         && Info[StreamKind][FieldPos][Info_Options][InfoOption_ShowInXml]==__T('Y'))
            List.push_back(Xml_Name_Escape_0_7_78(Info[StreamKind][FieldPos][Info_Name]));

    List.Separator_Set(0, __T(","));
    return List.Read();
}
#endif // MEDIAINFO_ADVANCED

//***************************************************************************
// SubFile
//***************************************************************************

//---------------------------------------------------------------------------
ZtringListList MediaInfo_Config::SubFile_Config_Get ()
{
    CriticalSectionLocker CSL(CS);

    return SubFile_Config;
}

//***************************************************************************
// Collections
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_CONFORMANCE
Ztring MediaInfo_Config::Conformance_Limit_Set(const Ztring& Value)
{
    int64s ValueI;
    auto Value_Lower(Value);
    transform(Value_Lower.begin(), Value_Lower.end(), Value_Lower.begin(), (int(*)(int))tolower); //(int(*)(int)) is a patch for unix
    if (Value_Lower==__T("inf"))
        ValueI=-1;
    else
    {
        ValueI=-Value.To_int64s();
        if ((!ValueI && Value != __T("0")) || ValueI>numeric_limits<int64s>::max())
            return __T("Invalid Conformance_Limit value");
    }

    CriticalSectionLocker CSL(CS);
    Conformance_Limit=(int64u)ValueI;
    return {};
}

int64u MediaInfo_Config::Conformance_Limit_Get()
{
    CriticalSectionLocker CSL(CS);

    return Conformance_Limit;
}
#endif // MEDIAINFO_CONFORMANCE

//---------------------------------------------------------------------------
#if MEDIAINFO_ADVANCED
void MediaInfo_Config::Collection_Trigger_Set(const Ztring& Value)
{
    int64s ValueI;
    if (!Value.empty() && Value.back()==__T('x'))
        ValueI=-Value.To_int64s();
    else
        ValueI=(int64s)(Value.To_float32()*1000);

    CriticalSectionLocker CSL(CS);

    Collection_Trigger=ValueI;
}

int64s MediaInfo_Config::Collection_Trigger_Get()
{
    CriticalSectionLocker CSL(CS);

    return Collection_Trigger;
}
#endif // MEDIAINFO_ADVANCED

//---------------------------------------------------------------------------
#if MEDIAINFO_ADVANCED
Ztring MediaInfo_Config::Collection_Display_Set(const Ztring& Value)
{
    display_if ValueI;
    if (Value==__T("never"))
        ValueI=display_if::Never;
    else if (Value==__T("needed"))
        ValueI=display_if::Needed;
    else if (Value.empty() || Value==__T("supported"))
        ValueI=display_if::Supported;
    else if (Value==__T("always"))
        ValueI=display_if::Always;
    else
        return __T("Unknown Collection_Display value");

    CriticalSectionLocker CSL(CS);

    Collection_Display=ValueI;
    return Ztring();
}

display_if MediaInfo_Config::Collection_Display_Get()
{
    CriticalSectionLocker CSL(CS);

    return Collection_Display;
}
#endif // MEDIAINFO_ADVANCED

//***************************************************************************
// Custom mapping
//***************************************************************************

void MediaInfo_Config::CustomMapping_Set (const Ztring &Value)
{
    ZtringList List; List.Separator_Set(0, __T(","));
    List.Write(Value);
    if (List.size()==3)
    {
        CriticalSectionLocker CSL(CS);
        CustomMapping[List[0]][List[1]]=List[2];
    }
}

Ztring MediaInfo_Config::CustomMapping_Get (const Ztring &Format, const Ztring &Field)
{
    CriticalSectionLocker CSL(CS);
    return CustomMapping[Format][Field];
}

bool MediaInfo_Config::CustomMapping_IsPresent(const Ztring &Format, const Ztring &Field)
{
    CriticalSectionLocker CSL(CS);
    std::map<Ztring, std::map<Ztring, Ztring> >::iterator PerFormat=CustomMapping.find(Format);
    if (PerFormat==CustomMapping.end())
        return false;
    std::map<Ztring, Ztring>::iterator PerField=PerFormat->second.find(Field);
    if (PerField==PerFormat->second.end())
        return false;
    return true;
}

//***************************************************************************
// Report changes
//***************************************************************************

#if MEDIAINFO_ADVANCED
void MediaInfo_Config::Format_Profile_Split_Set(bool Value)
{
    CriticalSectionLocker CSL(CS);
    Format_Profile_Split=Value;
}

bool MediaInfo_Config::Format_Profile_Split_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Format_Profile_Split;
}
#endif // MEDIAINFO_ADVANCED

#if defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_ADM_YES)
void MediaInfo_Config::Graph_Adm_ShowTrackUIDs_Set(bool Value)
{
    CriticalSectionLocker CSL(CS);
    Graph_Adm_ShowTrackUIDs=Value;
}

bool MediaInfo_Config::Graph_Adm_ShowTrackUIDs_Get()
{
    CriticalSectionLocker CSL(CS);
    return Graph_Adm_ShowTrackUIDs;
}

void MediaInfo_Config::Graph_Adm_ShowChannelFormats_Set(bool Value)
{
    CriticalSectionLocker CSL(CS);
    Graph_Adm_ShowChannelFormats=Value;
}

bool MediaInfo_Config::Graph_Adm_ShowChannelFormats_Get()
{
    CriticalSectionLocker CSL(CS);
    return Graph_Adm_ShowChannelFormats;
}
#endif //defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_ADM_YES)

#if defined(MEDIAINFO_EBUCORE_YES)
void MediaInfo_Config::AcquisitionDataOutputMode_Set(size_t Value)
{
    CriticalSectionLocker CSL(CS);
    AcquisitionDataOutputMode=Value;
}

size_t MediaInfo_Config::AcquisitionDataOutputMode_Get ()
{
    CriticalSectionLocker CSL(CS);
    return AcquisitionDataOutputMode;
}
#endif // MEDIAINFO_EBUCORE_YES
#if defined(MEDIAINFO_EBUCORE_YES) || defined(MEDIAINFO_NISO_YES) || MEDIAINFO_ADVANCED
void MediaInfo_Config::ExternalMetadata_Set(Ztring Value)
{
    CriticalSectionLocker CSL(CS);
    if (!ExternalMetadata.empty() && !Value.empty() && Value.find_first_of(__T("\r\n"))==string::npos) //Exception: if new value is on a single line, we add content to the previous content
    {
        ExternalMetadata+=LineSeparator;
        ExternalMetadata+=Value;
        return;
    }
    ExternalMetadata=Value;
}

Ztring MediaInfo_Config::ExternalMetadata_Get ()
{
    CriticalSectionLocker CSL(CS);
    return ExternalMetadata;
}

void MediaInfo_Config::ExternalMetaDataConfig_Set(Ztring Value)
{
    CriticalSectionLocker CSL(CS);
    ExternalMetaDataConfig=Value;
}

Ztring MediaInfo_Config::ExternalMetaDataConfig_Get ()
{
    CriticalSectionLocker CSL(CS);
    return ExternalMetaDataConfig;
}
#endif //defined(MEDIAINFO_EBUCORE_YES) || defined(MEDIAINFO_NISO_YES) || MEDIAINFO_ADVANCED

//***************************************************************************
// Event
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_EVENTS
bool MediaInfo_Config::Event_CallBackFunction_IsSet ()
{
    CriticalSectionLocker CSL(CS);

    return Event_CallBackFunction?true:false;
}
#endif //MEDIAINFO_EVENTS

//---------------------------------------------------------------------------
#if MEDIAINFO_EVENTS
Ztring MediaInfo_Config::Event_CallBackFunction_Set (const Ztring &Value)
{
    ZtringList List=Value;

    CriticalSectionLocker CSL(CS);

    if (List.empty())
    {
        Event_CallBackFunction=(MediaInfo_Event_CallBackFunction*)NULL;
        Event_UserHandler=NULL;
    }
    else
        for (size_t Pos=0; Pos<List.size(); Pos++)
        {
            if (List[Pos].find(__T("CallBack=memory://"))==0)
                Event_CallBackFunction=(MediaInfo_Event_CallBackFunction*)Ztring(List[Pos].substr(18, std::string::npos)).To_int64u();
            else if (List[Pos].find(__T("UserHandle=memory://"))==0)
                Event_UserHandler=(void*)Ztring(List[Pos].substr(20, std::string::npos)).To_int64u();
            else if (List[Pos].find(__T("UserHandler=memory://"))==0)
                Event_UserHandler=(void*)Ztring(List[Pos].substr(21, std::string::npos)).To_int64u();
            else
                return("Problem during Event_CallBackFunction value parsing");
        }

    return Ztring();
}
#endif //MEDIAINFO_EVENTS

//---------------------------------------------------------------------------
#if MEDIAINFO_EVENTS
Ztring MediaInfo_Config::Event_CallBackFunction_Get ()
{
    CriticalSectionLocker CSL(CS);

    return __T("CallBack=memory://")+Ztring::ToZtring((size_t)Event_CallBackFunction)+__T(";UserHandler=memory://")+Ztring::ToZtring((size_t)Event_UserHandler);
}
#endif //MEDIAINFO_EVENTS

//---------------------------------------------------------------------------
#if MEDIAINFO_EVENTS
void MediaInfo_Config::Event_Send (const int8u* Data_Content, size_t Data_Size)
{
    CriticalSectionLocker CSL(CS);

    if (Event_CallBackFunction)
    {
        MEDIAINFO_DEBUG1(   "Event",
                            Debug+=", EventID=";Debug+=Ztring::ToZtring(LittleEndian2int32u(Data_Content), 16).To_UTF8();)

        Event_CallBackFunction ((unsigned char*)Data_Content, Data_Size, Event_UserHandler);

        MEDIAINFO_DEBUG2(   "Event",
                            )
    }
}
#endif //MEDIAINFO_EVENTS

//---------------------------------------------------------------------------
#if MEDIAINFO_EVENTS
void MediaInfo_Config::Event_Send (const int8u* Data_Content, size_t Data_Size, const Ztring &File_Name)
{
    CriticalSectionLocker CSL(CS);

    if (Event_CallBackFunction)
    {
        MEDIAINFO_DEBUG1(   "Event",
                            Debug+=", EventID=";Debug+=Ztring::ToZtring(LittleEndian2int32u(Data_Content), 16).To_UTF8();)

        Event_CallBackFunction ((unsigned char*)Data_Content, Data_Size, Event_UserHandler);

        MEDIAINFO_DEBUG2(   "Event",
                            )
    }
}
#endif //MEDIAINFO_EVENTS

//---------------------------------------------------------------------------
#if MEDIAINFO_EVENTS
void MediaInfo_Config::Log_Send (int8u Type, int8u Severity, int32u MessageCode, const Ztring &Message)
{
    struct MediaInfo_Event_Log_0 Event;
    Event.EventCode=MediaInfo_EventCode_Create(MediaInfo_Parser_None, MediaInfo_Event_Log, 0);
    Event.Type=Type;
    Event.Severity=Severity;
    Event.Reserved2=(int8u)-1;
    Event.Reserved3=(int8u)-1;
    Event.MessageCode=MessageCode;
    Event.Reserved4=(int32u)-1;
    wstring MessageU=Message.To_Unicode();
    string MessageA=Message.To_Local();
    Event.MessageStringU=MessageU.c_str();
    Event.MessageStringA=MessageA.c_str();
    Event_Send((const int8u*)&Event, sizeof(MediaInfo_Event_Log_0));
}
#endif //MEDIAINFO_EVENTS

//***************************************************************************
// Graphviz
//***************************************************************************

#if defined(MEDIAINFO_GRAPHVIZ_YES)
bool MediaInfo_Config::GraphSvgPluginState()
{
    CriticalSectionLocker CSL(CS);
    return Export_Graph::Load();
}
#endif //defined(MEDIAINFO_GRAPHVIZ_YES)

//***************************************************************************
// Default profiles
//***************************************************************************

#if MEDIAINFO_CONFORMANCE
string MediaInfo_Config::AdmProfile(const Ztring& Value2)
{
    Ztring Value(Value2);
    transform(Value.begin(), Value.end(), Value.begin(), (int(*)(int))tolower); //(int(*)(int)) is a patch for unix
    ZtringList List;
    List.Separator_Set(0, __T(","));
    List.Write(Value);
    adm_profile Result;
    for (const auto& ItemZ : List)
    {
        const auto Item = ItemZ.To_UTF8();
        if (Item == "auto")
            Adm_Profile.Auto = true;
        else if (Item == "itu-r_bs.2076-0")
            Adm_Profile.BS2076 = 0;
        else if (Item == "itu-r_bs.2076-1")
            Adm_Profile.BS2076 = 1;
        else if (Item == "itu-r_bs.2076-2")
            Adm_Profile.BS2076 = 2;
        else if (Item.rfind("urn:ebu:tech:3392:1.0:", 0) == 0)
        {
            if (Item.size() > 22 && Item[22] >= '1' && Item[22] <= '4')
                Adm_Profile.Ebu3392 = Item[22] - '0';
            else
                return "Unknown ADM profile " + Item;
        }
        else
            return "Unknown ADM profile " + Item;
    }

    CriticalSectionLocker CSL(CS);
    Adm_Profile = Result;
    return {};
}
#endif //MEDIAINFO_CONFORMANCE

#if MEDIAINFO_CONFORMANCE
string MediaInfo_Config::AdmProfile_List()
{
    string Result;
    string LineSep = LineSeparator_Get().To_UTF8();
    for (int8u i = 1; i <= 4; i++)
    {
        Result += "urn:ebu:tech:3392:1.0:" + to_string(i);
        Result += LineSep;
    }
    Result.erase(Result.size() - LineSep.size());
    return Result;
}
#endif //MEDIAINFO_CONFORMANCE

#if MEDIAINFO_CONFORMANCE
MediaInfo_Config::adm_profile MediaInfo_Config::AdmProfile()
{
    CriticalSectionLocker CSL(CS);
    return Adm_Profile;
}
#endif //MEDIAINFO_CONFORMANCE

#if MEDIAINFO_CONFORMANCE
string MediaInfo_Config::Mp4Profile(const Ztring& Value)
{
    ZtringList List;
    List.Separator_Set(0, __T(","));
    List.Write(Value);
    string Temp;
    for (const auto& ItemZ : List)
    {
        const auto Item = ItemZ.To_UTF8();
        if (false)
            ;
        else if (Item == "cmfc" || Item == "cmff" || Item == "cmfl" || Item == "cmfs" || Item == "cmaf")
            Temp += "cmfc";
        else
            return "Unknown MP4 profile " + Item;
    }

    CriticalSectionLocker CSL(CS);
    Mp4_Profile = Temp;
    return {};
}
#endif //MEDIAINFO_CONFORMANCE

#if MEDIAINFO_CONFORMANCE
string MediaInfo_Config::Mp4Profile()
{
    CriticalSectionLocker CSL(CS);
    return Mp4_Profile;
}
#endif //MEDIAINFO_ADVANCED

#if MEDIAINFO_CONFORMANCE
string MediaInfo_Config::Mp4Profile_List()
{
    return "CMAF";
}
#endif //MEDIAINFO_CONFORMANCE

#if MEDIAINFO_CONFORMANCE
extern string Mpeg4_Descriptors_AudioProfileLevelString(int8u AudioProfileLevelIndication);
string MediaInfo_Config::UsacProfile(const Ztring& ValueZ)
{
    auto Value = ValueZ.To_UTF8();
    if (Value.empty())
    {
        CriticalSectionLocker CSL(CS);
        Usac_Profile = (int8u)-1;
        return {};
    }
    transform(Value.begin(), Value.end(), Value.begin(), (int(*)(int))tolower); //(int(*)(int)) is a patch for unix

    for (int8u AudioProfileLevelIndication = 0; AudioProfileLevelIndication < 0xFF; AudioProfileLevelIndication++)
    {
        string Test;
        switch (AudioProfileLevelIndication)
        {
            case 0x00 : Test = "No Profile"; break;
            case 0xFE : Test = "Unspecified"; break;
            case 0xFF : Test = "No Audio"; break;
            default   : Test = Mpeg4_Descriptors_AudioProfileLevelString(AudioProfileLevelIndication);
        }
        transform(Test.begin(), Test.end(), Test.begin(), (int(*)(int))tolower); //(int(*)(int)) is a patch for unix
        if (Test == Value)
        {
            CriticalSectionLocker CSL(CS);
            Usac_Profile = AudioProfileLevelIndication;
            return {};
        }
    }

    return "Unknown USAC profile " + Value;
}
#endif //MEDIAINFO_ADVANCED

#if MEDIAINFO_CONFORMANCE
int8u MediaInfo_Config::UsacProfile()
{
    CriticalSectionLocker CSL(CS);
    return Usac_Profile;
}
#endif //MEDIAINFO_CONFORMANCE

#if MEDIAINFO_CONFORMANCE
string MediaInfo_Config::UsacProfile_List()
{
    string LineSep = LineSeparator_Get().To_UTF8();
    string Result;
    Result += "No Profile";
    Result += LineSep;
    for (int8u AudioProfileLevelIndication = 1; AudioProfileLevelIndication < 0xFE; AudioProfileLevelIndication++)
    {
        auto Temp = Mpeg4_Descriptors_AudioProfileLevelString(AudioProfileLevelIndication);
        if (!Temp.empty())
        {
            Result += LineSep;
            Result += Temp;
        }
    }
    Result += "Unspecified";
    Result += LineSep;
    Result += "No Audio";
    return Result;
}
#endif //MEDIAINFO_CONFORMANCE

#if MEDIAINFO_CONFORMANCE
string MediaInfo_Config::Profile_List()
{
    auto LineSep = LineSeparator_Get().To_UTF8();
    return "ADM" + LineSep + "MP4" + LineSep + "USAC";
}
#endif //MEDIAINFO_CONFORMANCE

#if MEDIAINFO_CONFORMANCE
void MediaInfo_Config::WarningError(bool Value)
{
    CriticalSectionLocker CSL(CS);
    Warning_Error=Value;
}
#endif //MEDIAINFO_CONFORMANCE

#if MEDIAINFO_CONFORMANCE
bool MediaInfo_Config::WarningError()
{
    CriticalSectionLocker CSL(CS);
    return Warning_Error;
}
#endif //MEDIAINFO_CONFORMANCE

//***************************************************************************
// Curl
//***************************************************************************

#if defined(MEDIAINFO_LIBCURL_YES)
bool MediaInfo_Config::CanHandleUrls()
{
    CriticalSectionLocker CSL(CS);
    return Reader_libcurl::Load();
}

void MediaInfo_Config::URLEncode_Set (MediaInfo_Config::urlencode Value)
{
    CriticalSectionLocker CSL(CS);
    URLEncode=Value;
}

MediaInfo_Config::urlencode MediaInfo_Config::URLEncode_Get()
{
    CriticalSectionLocker CSL(CS);
    return URLEncode;
}

void MediaInfo_Config::Ssh_PublicKeyFileName_Set (const Ztring &Value)
{
    CriticalSectionLocker CSL(CS);
    Ssh_PublicKeyFileName=Value;
}

Ztring MediaInfo_Config::Ssh_PublicKeyFileName_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Ssh_PublicKeyFileName;
}

void MediaInfo_Config::Ssh_PrivateKeyFileName_Set (const Ztring &Value)
{
    CriticalSectionLocker CSL(CS);
    Ssh_PrivateKeyFileName=Value;
}

Ztring MediaInfo_Config::Ssh_PrivateKeyFileName_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Ssh_PrivateKeyFileName;
}

void MediaInfo_Config::Ssh_KnownHostsFileName_Set (const Ztring &Value)
{
    if (Value.empty())
        return; //empty value means "disable security" for libcurl, not acceptable

    CriticalSectionLocker CSL(CS);
    Ssh_KnownHostsFileName=Value;
}

Ztring MediaInfo_Config::Ssh_KnownHostsFileName_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Ssh_KnownHostsFileName;
}

void MediaInfo_Config::Ssh_IgnoreSecurity_Set (bool Value)
{
    CriticalSectionLocker CSL(CS);
    Ssh_IgnoreSecurity=Value;
}

bool MediaInfo_Config::Ssh_IgnoreSecurity_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Ssh_IgnoreSecurity;
}

void MediaInfo_Config::Ssl_CertificateFileName_Set (const Ztring &Value)
{
    CriticalSectionLocker CSL(CS);
    Ssl_CertificateFileName=Value;
}

Ztring MediaInfo_Config::Ssl_CertificateFileName_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Ssl_CertificateFileName;
}

void MediaInfo_Config::Ssl_CertificateFormat_Set (const Ztring &Value)
{
    CriticalSectionLocker CSL(CS);
    Ssl_CertificateFormat=Value;
}

Ztring MediaInfo_Config::Ssl_CertificateFormat_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Ssl_CertificateFormat;
}

void MediaInfo_Config::Ssl_PrivateKeyFileName_Set (const Ztring &Value)
{
    CriticalSectionLocker CSL(CS);
    Ssl_PrivateKeyFileName=Value;
}

Ztring MediaInfo_Config::Ssl_PrivateKeyFileName_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Ssl_PrivateKeyFileName;
}

void MediaInfo_Config::Ssl_PrivateKeyFormat_Set (const Ztring &Value)
{
    CriticalSectionLocker CSL(CS);
    Ssl_PrivateKeyFormat=Value;
}

Ztring MediaInfo_Config::Ssl_PrivateKeyFormat_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Ssl_PrivateKeyFormat;
}

void MediaInfo_Config::Ssl_CertificateAuthorityFileName_Set (const Ztring &Value)
{
    CriticalSectionLocker CSL(CS);
    Ssl_CertificateAuthorityFileName=Value;
}

Ztring MediaInfo_Config::Ssl_CertificateAuthorityFileName_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Ssl_CertificateAuthorityFileName;
}

void MediaInfo_Config::Ssl_CertificateAuthorityPath_Set (const Ztring &Value)
{
    CriticalSectionLocker CSL(CS);
    Ssl_CertificateAuthorityPath=Value;
}

Ztring MediaInfo_Config::Ssl_CertificateAuthorityPath_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Ssl_CertificateAuthorityPath;
}

void MediaInfo_Config::Ssl_CertificateRevocationListFileName_Set (const Ztring &Value)
{
    CriticalSectionLocker CSL(CS);
    Ssl_CertificateRevocationListFileName=Value;
}

Ztring MediaInfo_Config::Ssl_CertificateRevocationListFileName_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Ssl_CertificateRevocationListFileName;
}

void MediaInfo_Config::Ssl_IgnoreSecurity_Set (bool Value)
{
    CriticalSectionLocker CSL(CS);
    Ssl_IgnoreSecurity=Value;
}

bool MediaInfo_Config::Ssl_IgnoreSecurity_Get ()
{
    CriticalSectionLocker CSL(CS);
    return Ssl_IgnoreSecurity;
}
#endif //defined(MEDIAINFO_LIBCURL_YES)


#if MEDIAINFO_FIXITY
//---------------------------------------------------------------------------
void MediaInfo_Config::TryToFix_Set (bool NewValue)
{
    CriticalSectionLocker CSL(CS);
    TryToFix=NewValue;
}

bool MediaInfo_Config::TryToFix_Get ()
{
    CriticalSectionLocker CSL(CS);
    return TryToFix;
}
#endif //MEDIAINFO_FIXITY

} //NameSpace

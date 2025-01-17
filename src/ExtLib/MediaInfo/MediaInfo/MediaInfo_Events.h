/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

/*Structures for MediaInfo events */

#ifndef MediaInfo_EventsH
#define MediaInfo_EventsH

#ifdef __cplusplus
#include <stdlib.h>
#include <string.h>
#include <sstream>
#endif //__cplusplus

/***************************************************************************/
/* Platforms (from libzen)                                                 */
/***************************************************************************/

/*-------------------------------------------------------------------------*/
/*Win32*/
#if defined(__NT__) || defined(_WIN32) || defined(WIN32)
    #ifndef WIN32
        #define WIN32
    #endif
    #ifndef _WIN32
        #define _WIN32
    #endif
    #ifndef __WIN32__
        #define __WIN32__ 1
    #endif
#endif

/*-------------------------------------------------------------------------*/
/*Win64*/
#if defined(_WIN64) || defined(WIN64)
    #ifndef WIN64
        #define WIN64
    #endif
    #ifndef _WIN64
        #define _WIN64
    #endif
    #ifndef __WIN64__
        #define __WIN64__ 1
    #endif
#endif

/*-------------------------------------------------------------------------*/
/*Windows*/
#if defined(WIN32) || defined(WIN64)
    #ifndef WINDOWS
        #define WINDOWS
    #endif
    #ifndef _WINDOWS
        #define _WINDOWS
    #endif
    #ifndef __WINDOWS__
        #define __WINDOWS__ 1
    #endif
#endif

/*-------------------------------------------------------------------------*/
/*Unix (Linux, HP, Sun, BeOS...)*/
#if defined(UNIX) || defined(_UNIX) || defined(__UNIX__) \
    || defined(__unix) || defined(__unix__) \
    || defined(____SVR4____) || defined(__LINUX__) || defined(__sgi) \
    || defined(__hpux) || defined(sun) || defined(__SUN__) || defined(_AIX) \
    || defined(__EMX__) || defined(__VMS) || defined(__BEOS__)
    #ifndef UNIX
        #define UNIX
    #endif
    #ifndef _UNIX
        #define _UNIX
    #endif
    #ifndef __UNIX__
        #define __UNIX__ 1
    #endif
#endif

/*-------------------------------------------------------------------------*/
/*MacOS Classic*/
#if defined(macintosh)
    #ifndef MACOS
        #define MACOS
    #endif
    #ifndef _MACOS
        #define _MACOS
    #endif
    #ifndef __MACOS__
        #define __MACOS__ 1
    #endif
#endif

/*-------------------------------------------------------------------------*/
/*MacOS X*/
#if defined(__APPLE__) && defined(__MACH__)
    #ifndef MACOSX
        #define MACOSX
    #endif
    #ifndef _MACOSX
        #define _MACOSX
    #endif
    #ifndef __MACOSX__
        #define __MACOSX__ 1
    #endif
#endif

/*Test of targets*/
#if defined(WINDOWS) && defined(UNIX) && defined(MACOS) && defined(MACOSX)
    #pragma message Multiple platforms???
#endif

#if !defined(WIN32) && !defined(UNIX) && !defined(MACOS) && !defined(MACOSX)
    #pragma message No known platforms, assume default
#endif

/*-------------------------------------------------------------------------*/
/*8-bit int                                                                */
#if UCHAR_MAX==0xff
    #undef  MAXTYPE_INT
    #define MAXTYPE_INT 8
    typedef signed   char       MediaInfo_int8s;
    typedef unsigned char       MediaInfo_int8u;
#else
    #pragma message This machine has no 8-bit integertype?
#endif

/*-------------------------------------------------------------------------*/
/*16-bit int                                                               */
#if UINT_MAX == 0xffff
    #undef  MAXTYPE_INT
    #define MAXTYPE_INT 16
    typedef signed   int        MediaInfo_int16s;
    typedef unsigned int        MediaInfo_int16u;
#elif USHRT_MAX == 0xffff
    #undef  MAXTYPE_INT
    #define MAXTYPE_INT 16
    typedef signed   short      MediaInfo_int16s;
    typedef unsigned short      MediaInfo_int16u;
#else
    #pragma message This machine has no 16-bit integertype?
#endif

/*-------------------------------------------------------------------------*/
/*32-bit int                                                               */
#if UINT_MAX == 0xfffffffful
    #undef  MAXTYPE_INT
    #define MAXTYPE_INT 32
    typedef signed   int        MediaInfo_int32s;
    typedef unsigned int        MediaInfo_int32u;
#elif ULONG_MAX == 0xfffffffful
    #undef  MAXTYPE_INT
    #define MAXTYPE_INT 32
    typedef signed   long       MediaInfo_int32s;
    typedef unsigned long       MediaInfo_int32u;
#elif USHRT_MAX == 0xfffffffful
    #undef  MAXTYPE_INT
    #define MAXTYPE_INT 32
    typedef signed   short      MediaInfo_int32s;
    typedef unsigned short      MediaInfo_int32u;
#else
    #pragma message This machine has no 32-bit integer type?
#endif

/*-------------------------------------------------------------------------*/
/*64-bit int                                                               */
#if defined(__MINGW32__) || defined(__CYGWIN32__) || defined(__UNIX__) || defined(__MACOSX__)
    #undef  MAXTYPE_INT
    #define MAXTYPE_INT 64
    typedef unsigned long long  MediaInfo_int64u;
    typedef   signed long long  MediaInfo_int64s;
#elif defined(__WIN32__) || defined(_WIN32)
    #undef  MAXTYPE_INT
    #define MAXTYPE_INT 64
    typedef unsigned __int64    MediaInfo_int64u;
    typedef   signed __int64    MediaInfo_int64s;
#else
    #pragma message This machine has no 64-bit integer type?
#endif
/*-------------------------------------------------------------------------*/


/***************************************************************************/
/* The callback function                                                   */
/***************************************************************************/

#if !defined(__WINDOWS__) && !defined(__stdcall)
    #define __stdcall
#endif //!defined(__WINDOWS__)

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    typedef void (__stdcall MediaInfo_Event_CallBackFunction)(unsigned char* Data_Content, size_t Data_Size, void* UserHandler);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/***************************************************************************/
/* EventCode management                                                    */
/***************************************************************************/

#define MediaInfo_EventCode_Create(ParserID, EventID, EventVersion) \
  (  ((MediaInfo_int32u)ParserID    )<<24 \
   | ((MediaInfo_int32u)EventID     )<< 8 \
   | ((MediaInfo_int32u)EventVersion)    )

/***************************************************************************/
/* Global                                                                  */
/***************************************************************************/

/*-------------------------------------------------------------------------*/
/* Time code                                                               */
typedef struct MediaInfo_time_code
{
    MediaInfo_int8u     Hours;
    MediaInfo_int8u     Minutes;
    MediaInfo_int8u     Seconds;
    MediaInfo_int8u     Frames;
    MediaInfo_int8u     FramesPerSecond;
    MediaInfo_int8u     DropFrame; //0= No, 1=Yes
    MediaInfo_int8u     Reserved[2];
} MediaInfo_time_code;

/*-------------------------------------------------------------------------*/
/* Generic                                                                 */
#define MEDIAINFO_EVENT_GENERIC \
    MediaInfo_int32u        EventCode; \
    MediaInfo_int32u        ReservedI32; \
    size_t                  EventSize; \
    size_t                  StreamIDs_Size; \
    MediaInfo_int64u        StreamIDs[16]; \
    MediaInfo_int8u         StreamIDs_Width[16]; \
    MediaInfo_int8u         ParserIDs[16]; \
    MediaInfo_int64u        StreamOffset; \
    MediaInfo_int64u        FrameNumber; \
    MediaInfo_int64u        PCR; \
    MediaInfo_int64u        PTS; \
    MediaInfo_int64u        DTS; \
    MediaInfo_int64u        DUR; \
    MediaInfo_int64u        FrameNumber_PresentationOrder; \
    MediaInfo_int64u        ReservedI64[1]; \
    MediaInfo_time_code     TimeCode_Container; \
    MediaInfo_time_code     TimeCode_SDTI; \
    MediaInfo_time_code     TimeCode_RawStream; \
    MediaInfo_time_code     ReservedT[5]; \

typedef struct MediaInfo_Event_Generic
{
    MEDIAINFO_EVENT_GENERIC
} MediaInfo_Event_Generic;

/*-------------------------------------------------------------------------*/
/* MediaInfo_Event_Log_0                                                   */
#define MediaInfo_Event_Log 0x0F00
struct MediaInfo_Event_Log_0
{
    MEDIAINFO_EVENT_GENERIC
    MediaInfo_int8u         Type;
    MediaInfo_int8u         Severity;
    MediaInfo_int8u         Reserved2;
    MediaInfo_int8u         Reserved3;
    MediaInfo_int32u        MessageCode;
    MediaInfo_int32u        Reserved4;
    const wchar_t*          MessageString;
    const wchar_t*          MessageStringU;
    const char*             MessageStringA;
};

/*-------------------------------------------------------------------------*/
/* Demux                                                                   */
#define MediaInfo_Event_Global_Demux 0xAF00
enum MediaInfo_Event_Global_Demux_0_contenttype
{
    MediaInfo_Event_Global_Demux_0_ContentType_MainStream,
    MediaInfo_Event_Global_Demux_0_ContentType_SubStream,
    MediaInfo_Event_Global_Demux_0_ContentType_Header,
    MediaInfo_Event_Global_Demux_0_ContentType_Synchro
};

struct MediaInfo_Event_Global_Demux_4
{
    MEDIAINFO_EVENT_GENERIC
    MediaInfo_int8u         Content_Type; /*MediaInfo_Event_Global_Demux_0_contenttype*/
    size_t                  Content_Size;
    const MediaInfo_int8u*  Content;
    MediaInfo_int64u        Flags; /*bit0=random_access*/
    size_t                  Offsets_Size;
    const MediaInfo_int64u* Offsets_Stream; /* From the begin of the stream */
    const MediaInfo_int64u* Offsets_Content; /* From the begin of the demuxed content */
    size_t                  OriginalContent_Size; /* In case of decoded content inside MediaInfo, OriginalContent contain the not-decoded stream */
    const MediaInfo_int8u*  OriginalContent; /* In case of decoded content inside MediaInfo, OriginalContent contain the not-decoded stream */
};

/*-------------------------------------------------------------------------*/
/* Simple text                                                             */
#define MediaInfo_Event_Global_SimpleText 0xAF01
struct MediaInfo_Event_Global_SimpleText_0
{
    MEDIAINFO_EVENT_GENERIC
    const wchar_t*      Content;
    MediaInfo_int8u     Flags;
    MediaInfo_int8u     MuxingMode;
    MediaInfo_int8u     Service;
    MediaInfo_int32u    Row_Max;
    MediaInfo_int32u    Column_Max;
    wchar_t**           Row_Values; //First indice is the row number, second indice is the column number. Row ends with \0
    MediaInfo_int8u**   Row_Attributes;
};

/*-------------------------------------------------------------------------*/
/* BytesRead                                                               */
#define MediaInfo_Event_Global_BytesRead 0xAF02
struct MediaInfo_Event_Global_BytesRead_0
{
    MEDIAINFO_EVENT_GENERIC
    size_t                  Content_Size;
    const MediaInfo_int8u*  Content;
};

/*-------------------------------------------------------------------------*/
/* Decoded                                                                 */
#define MediaInfo_Event_Global_Decoded 0xAF03

struct MediaInfo_Event_Global_Decoded_0
{
    MEDIAINFO_EVENT_GENERIC
    size_t                  Content_Size;
    const MediaInfo_int8u*  Content;
    MediaInfo_int64u        Flags;
};

/*-------------------------------------------------------------------------*/
/* AttachedFile                                                            */
#define MediaInfo_Event_Global_AttachedFile 0xAF04

struct MediaInfo_Event_Global_AttachedFile_0
{
    MEDIAINFO_EVENT_GENERIC
    size_t                  Content_Size;
    const MediaInfo_int8u*  Content;
    MediaInfo_int64u        Flags;
    const char*             Name;
    const char*             MimeType;
    const char*             Description;
};

/*-------------------------------------------------------------------------*/
/* MediaInfo_Event_Video_SliceInfo_0                                       */
#define MediaInfo_Event_Video_SliceInfo 0x7801
struct MediaInfo_Event_Video_SliceInfo_0
{
    MEDIAINFO_EVENT_GENERIC
    MediaInfo_int64u        FieldPosition;
    MediaInfo_int64u        SlicePosition;
    MediaInfo_int8u         SliceType;
    MediaInfo_int64u        Flags;
};

/***************************************************************************/
/* General                                                                 */
/***************************************************************************/

#define MediaInfo_Parser_None           0x00
#define MediaInfo_Parser_General        0x00
#define MediaInfo_Parser_Global         0x00
#define MediaInfo_Parser_Video          0x01

/*-------------------------------------------------------------------------*/
/* SubFile_Missing                                                         */
#define MediaInfo_Event_General_SubFile_Missing 0x1F01
struct MediaInfo_Event_General_SubFile_Missing_0
{
    MEDIAINFO_EVENT_GENERIC
    const char*             FileName_Relative;
    const wchar_t*          FileName_Relative_Unicode;
    const char*             FileName_Absolute;
    const wchar_t*          FileName_Absolute_Unicode;
};

/*-------------------------------------------------------------------------*/
/* Start                                                                   */
#define MediaInfo_Event_General_Start 0x7001
struct MediaInfo_Event_General_Start_0
{
    MEDIAINFO_EVENT_GENERIC
    MediaInfo_int64u        Stream_Size;
    const char*             FileName;
    const wchar_t*          FileName_Unicode;
};

/*-------------------------------------------------------------------------*/
/* End                                                                     */
#define MediaInfo_Event_General_End 0x7002
struct MediaInfo_Event_General_End_0
{
    MEDIAINFO_EVENT_GENERIC
    MediaInfo_int64u        Stream_Bytes_Analyzed;
    MediaInfo_int64u        Stream_Size;
    MediaInfo_int64u        Stream_Bytes_Padding;
    MediaInfo_int64u        Stream_Bytes_Junk;
};

/*-------------------------------------------------------------------------*/
/* Parser_Selected                                                         */
#define MediaInfo_Event_General_Parser_Selected 0x7003
struct MediaInfo_Event_General_Parser_Selected_0
{
    MEDIAINFO_EVENT_GENERIC
    char                    Name[16];
};

/*-------------------------------------------------------------------------*/
/* Move request                                                            */
#define MediaInfo_Event_General_Move_Request 0x7004
struct MediaInfo_Event_General_Move_Request_0
{
    MEDIAINFO_EVENT_GENERIC
};

/*-------------------------------------------------------------------------*/
/* Move done                                                               */
#define MediaInfo_Event_General_Move_Done 0x7005
struct MediaInfo_Event_General_Move_Done_0
{
    MEDIAINFO_EVENT_GENERIC
};

/*-------------------------------------------------------------------------*/
/* SubFile_Start                                                           */
#define MediaInfo_Event_General_SubFile_Start 0x7006
struct MediaInfo_Event_General_SubFile_Start_0
{
    MEDIAINFO_EVENT_GENERIC
    const char*             FileName_Relative;
    const wchar_t*          FileName_Relative_Unicode;
    const char*             FileName_Absolute;
    const wchar_t*          FileName_Absolute_Unicode;
};

/*-------------------------------------------------------------------------*/
/* SubFile_End                                                             */
#define MediaInfo_Event_General_SubFile_End 0x7007
struct MediaInfo_Event_General_SubFile_End_0
{
    MEDIAINFO_EVENT_GENERIC
};

/*-------------------------------------------------------------------------*/
/* WaitForMoreData_Start                                                   */
#define MediaInfo_Event_General_WaitForMoreData_Start 0x7008
struct MediaInfo_Event_General_WaitForMoreData_Start_0
{
    MEDIAINFO_EVENT_GENERIC
    double                  Duration_Max;       //In seconds
};
/*-------------------------------------------------------------------------*/
/* WaitForMoreData_End                                                     */
#define MediaInfo_Event_General_WaitForMoreData_End 0x7009
struct MediaInfo_Event_General_WaitForMoreData_End_0
{
    MEDIAINFO_EVENT_GENERIC
    double                  Duration_Max;       //In seconds
    double                  Duration_Actual;    //In seconds
    MediaInfo_int8u         Flags;              //bit 0: Outcome (1 is giving up)
};

/***************************************************************************/
/* MPEG-TS / BDAV / TSP                                                    */
/***************************************************************************/

#define MediaInfo_Parser_SideCar        0x72

/***************************************************************************/
/* MPEG-TS / BDAV / TSP                                                    */
/***************************************************************************/

#define MediaInfo_Parser_MpegTs         0x01

/***************************************************************************/
/* MPEG-PS                                                                 */
/***************************************************************************/

#define MediaInfo_Parser_MpegPs         0x02
#define MediaInfo_Parser_MpegPs_Ext     0x70

/***************************************************************************/
/* DV / DIF                                                                 */
/***************************************************************************/

#define MediaInfo_Parser_DvDif          0x03

/*-------------------------------------------------------------------------*/
/* Analysis                                                                */
#define MediaInfo_Event_DvDif_Analysis_Frame 0xB001
struct MediaInfo_Event_DvDif_Analysis_Frame_0
{
    MediaInfo_int32u        EventCode;
    MediaInfo_int32u        TimeCode;
    MediaInfo_int32u        RecordedDateTime1;
    MediaInfo_int16u        RecordedDateTime2;
    MediaInfo_int8u         Arb;
    MediaInfo_int8u         Verbosity;
    const char*             Errors;
};
struct MediaInfo_Event_DvDif_Analysis_Frame_1
{
    MEDIAINFO_EVENT_GENERIC
    MediaInfo_int32u        TimeCode;
    MediaInfo_int32u        RecordedDateTime1;
    MediaInfo_int16u        RecordedDateTime2Buggy;
    MediaInfo_int8u         Arb;
    MediaInfo_int8u         Verbosity;
    const char*             Errors;
    size_t                  Video_STA_Errors_Count;
    size_t*                 Video_STA_Errors;
    size_t                  Audio_Data_Errors_Count;
    size_t*                 Audio_Data_Errors;
    MediaInfo_int32u        Captions_Errors; // bit 0 = parity issue
    MediaInfo_int32u        Coherency_Flags; // bit 0 = no pack sub, bit 1 = no pack vid, bit 2 = no pack aud, bit 3 = no data vid, bit 4 = no data aud, bit 5 = no vid source/control, bit 6 = no aud source/control
    MediaInfo_int16u        RecordedDateTime2;
    size_t                  BlockStatus_Count;
    const MediaInfo_int8u*  BlockStatus;
    MediaInfo_int32u        AbstBf;
    MediaInfo_int32u        MoreFlags;
    const MediaInfo_int8u*  MoreData;
};
enum MediaInfo_Event_Analysis_Frame_Enums {
    MediaInfo_Event_Analysis_Frame_None,
    MediaInfo_Event_Analysis_Frame_AudioErrorValues,
    MediaInfo_Event_Analysis_Frame_DirectionSpeed,
    MediaInfo_Event_Analysis_Frame_TimeCode,
};

/*-------------------------------------------------------------------------*/
/* Change in the stream config                                             */
#define MediaInfo_Event_DvDif_Change 0xB002
struct MediaInfo_Event_DvDif_Change_0
{
    MEDIAINFO_EVENT_GENERIC
    MediaInfo_int32u        Width;
    MediaInfo_int32u        Height;
    MediaInfo_int32u        VideoChromaSubsampling; // -1=unknown, 0=411, 1=420, 2=422
    MediaInfo_int32u        VideoScanType; // -1=unknown
    MediaInfo_int32u        VideoRatio_N;
    MediaInfo_int32u        VideoRatio_D;
    MediaInfo_int32u        VideoRate_N;
    MediaInfo_int32u        VideoRate_D;
    MediaInfo_int32u        AudioRate_N;
    MediaInfo_int32u        AudioRate_D;
    MediaInfo_int32u        AudioChannels;
    MediaInfo_int32u        AudioBitDepth;
    MediaInfo_int32u        Captions_Flags; // bit 0 = present
    const MediaInfo_int8u*  MoreData;
};
enum MediaInfo_Event_Change_MoreData_Enums {
    MediaInfo_Event_Change_MoreData_None,
    MediaInfo_Event_Change_MoreData_Emphasis,
    MediaInfo_Event_Change_MoreData_ProgramNumber,
};

/***************************************************************************/
/* CDXA                                                                    */
/***************************************************************************/

#define MediaInfo_Parser_Cdxa           0x04

/***************************************************************************/
/* FLV                                                                     */
/***************************************************************************/

#define MediaInfo_Parser_Flv            0x06

/***************************************************************************/
/* GXF                                                                     */
/***************************************************************************/

#define MediaInfo_Parser_Gxf            0x07

/***************************************************************************/
/* Matroska                                                                */
/***************************************************************************/

#define MediaInfo_Parser_Matroska       0x08

/***************************************************************************/
/* MPEG-4                                                                  */
/***************************************************************************/

#define MediaInfo_Parser_Mpeg4          0x09
#define MediaInfo_Parser_Mpeg4_Desc     0x71

/***************************************************************************/
/* MXF                                                                     */
/***************************************************************************/

#define MediaInfo_Parser_Mxf            0x0A

/***************************************************************************/
/* OGG                                                                     */
/***************************************************************************/

#define MediaInfo_Parser_Ogg            0x0B

/***************************************************************************/
/* RIFF                                                                    */
/***************************************************************************/

#define MediaInfo_Parser_Riff           0x0C

/***************************************************************************/
/* WM                                                                      */
/***************************************************************************/

#define MediaInfo_Parser_Wm             0x0D

/***************************************************************************/
/* LXF                                                                     */
/***************************************************************************/

#define MediaInfo_Parser_Lxf            0x0E

/***************************************************************************/
/* HLS                                                                     */
/***************************************************************************/

#define MediaInfo_Parser_Hls            0x60

/***************************************************************************/
/* HLS Index                                                               */
/***************************************************************************/

#define MediaInfo_Parser_HlsIndex       0x61

/***************************************************************************/
/* Internet Streaming Media                                                */
/***************************************************************************/

#define MediaInfo_Parser_Ism            0x62

/***************************************************************************/
/* DASH MPD                                                                */
/***************************************************************************/

#define MediaInfo_Parser_DashMpd        0x63

/***************************************************************************/
/* HDS F4M                                                                 */
/***************************************************************************/

#define MediaInfo_Parser_HdsF4m         0x64

/***************************************************************************/
/* DCP Composition Asset Map                                               */
/***************************************************************************/

#define MediaInfo_Parser_DcpAm          0x65

/***************************************************************************/
/* DCP Composition Playlist                                                */
/***************************************************************************/

#define MediaInfo_Parser_DcpCpl         0x66

/***************************************************************************/
/* DCP Package List                                                        */
/***************************************************************************/

#define MediaInfo_Parser_DcpPkl         0x67

/***************************************************************************/
/* DCP Output List                                                        */
/***************************************************************************/

#define MediaInfo_Parser_DcpOpl         0x68

/***************************************************************************/
/* Pro Tools session 10                                                    */
/***************************************************************************/

#define MediaInfo_Parser_Ptx            0x69

/***************************************************************************/
/* AAF                                                                     */
/***************************************************************************/

#define MediaInfo_Parser_Aaf            0x6A

/***************************************************************************/
/* DXW                                                                     */
/***************************************************************************/

#define MediaInfo_Parser_Dxw            0x6B

/***************************************************************************/
/* MediaInfo XML                                                           */
/***************************************************************************/

#define MediaInfo_Parser_MiXml          0x6C

/***************************************************************************/
/* NSV                                                                     */
/***************************************************************************/

#define MediaInfo_Parser_Nsv            0x6D

/***************************************************************************/
/* MPEG Video                                                              */
/***************************************************************************/

#define MediaInfo_Parser_Mpegv          0x80

/***************************************************************************/
/* AVC                                                                     */
/***************************************************************************/

#define MediaInfo_Parser_Avc            0x81

/***************************************************************************/
/* AVC                                                                     */
/***************************************************************************/

#define MediaInfo_Parser_Hevc            0x83

/***************************************************************************/
/* VC-1                                                                    */
/***************************************************************************/

#define MediaInfo_Parser_Vc1            0x82

/***************************************************************************/
/* Active Format Description (AFD)                                         */
/***************************************************************************/

#define MediaInfo_Parser_Afd            0x83

/***************************************************************************/
/* Bar Data                                                                */
/***************************************************************************/

#define MediaInfo_Parser_BarData        0x84

/***************************************************************************/
/* MPEG-4 Visual                                                           */
/***************************************************************************/

#define MediaInfo_Parser_Mpeg4v         0x85

/***************************************************************************/
/* DTS                                                                     */
/***************************************************************************/

#define MediaInfo_Parser_Dts            0xA0

/***************************************************************************/
/* AC-3                                                                    */
/***************************************************************************/

#define MediaInfo_Parser_Ac3            0xA1

/***************************************************************************/
/* AAC                                                                     */
/***************************************************************************/

#define MediaInfo_Parser_Aac            0xA2

/***************************************************************************/
/* MPEG Audio                                                              */
/***************************************************************************/

#define MediaInfo_Parser_Mpega          0xA3

/***************************************************************************/
/* PCM                                                                     */
/***************************************************************************/

#define MediaInfo_Parser_Pcm            0xA4

/***************************************************************************/
/* AES-3                                                                   */
/***************************************************************************/

#define MediaInfo_Parser_Aes3           0xA5

/***************************************************************************/
/* Dolby E                                                                 */
/***************************************************************************/

#define MediaInfo_Parser_DolbyE         0xA6

/***************************************************************************/
/* Channel grouping intermediate module                                    */
/***************************************************************************/

#define MediaInfo_Parser_ChannelGrouping 0xA7

/***************************************************************************/
/* JPEG                                                                    */
/***************************************************************************/

#define MediaInfo_Parser_Jpeg           0xC0

/***************************************************************************/
/* CEA-608 (formely IEA-608)                                               */
/***************************************************************************/

#define MediaInfo_Parser_Eia608         0xF0

/*-------------------------------------------------------------------------*/
/* MediaInfo_Event_Eia608_Content                                          */
#define MediaInfo_Event_Eia608_CC_Content 0xA000
struct MediaInfo_Event_Eia608_CC_Content_0
{
    MEDIAINFO_EVENT_GENERIC
    MediaInfo_int8u     Field;
    MediaInfo_int8u     MuxingMode;
    MediaInfo_int8u     Service;
    wchar_t             Row_Values[15][33]; /*offset 32 is for \0*/
    MediaInfo_int8u     Row_Attributes[15][32];
};

/***************************************************************************/
/* DTVCC Transport (CEA-708, formely IEA-708)                              */
/***************************************************************************/

#define MediaInfo_Parser_DtvccTransport 0xF1
#define MediaInfo_Parser_Eia708         0xF1 /*Deprecated*/

/***************************************************************************/
/* DTVCC Caption (CEA-708, formely IEA-708)                                */
/***************************************************************************/

#define MediaInfo_Parser_DtvccCaption   0xF2

/*-------------------------------------------------------------------------*/
/* MediaInfo_Event_DtvccCaption_Content_Minimal                            */
#define MediaInfo_Event_DtvccCaption_Content_Minimal 0xA000
struct MediaInfo_Event_DtvccCaption_Content_Minimal_0
{
    MEDIAINFO_EVENT_GENERIC
    MediaInfo_int8u     MuxingMode;
    MediaInfo_int8u     Service;
    wchar_t             Row_Values[15][65]; /*offset 32 (4:3) or 42 (16:9) is for \0, reserving data after 42 for future extensions*/
    MediaInfo_int8u     Row_Attributes[15][64];
};

/*-------------------------------------------------------------------------*/
/* MediaInfo_Event_DtvccCaption_Window_Content_Minimal                     */
#define MediaInfo_Event_DtvccCaption_Window_Content_Minimal 0xA001
struct MediaInfo_Event_DtvccCaption_Window_Content_Minimal_0
{
    MEDIAINFO_EVENT_GENERIC
    MediaInfo_int8u     MuxingMode;
    MediaInfo_int8u     Service;
    MediaInfo_int8u     Window;
    wchar_t             Row_Values[15][33]; /*offset 32 is for \0*/
    MediaInfo_int8u     Row_Attributes[15][32];
};

/***************************************************************************/
/* CDP                                                                     */
/***************************************************************************/

#define MediaInfo_Parser_Cdp            0xF3

/***************************************************************************/
/* DVD CC                                                                  */
/***************************************************************************/

#define MediaInfo_Parser_DvdCc          0xF4

/***************************************************************************/
/* SCTE 20                                                                 */
/***************************************************************************/

#define MediaInfo_Parser_Scte20         0xF5

/***************************************************************************/
/* DVB Subtitle                                                            */
/***************************************************************************/

#define MediaInfo_Parser_DvbSubtitle    0xF6

/***************************************************************************/
/* Teletext                                                                */
/***************************************************************************/

#define MediaInfo_Parser_Teletext       0xF7

/***************************************************************************/
/* SCC                                                                     */
/***************************************************************************/

#define MediaInfo_Parser_Scc            0xF8

/***************************************************************************/
/* ARIB STD B24/B37                                                        */
/***************************************************************************/

#define MediaInfo_Parser_AribStdB24B37  0xF9

/***************************************************************************/
/* TTML                                                                    */
/***************************************************************************/

#define MediaInfo_Parser_Ttml           0xFA

/***************************************************************************/
/* SubRip                                                                  */
/***************************************************************************/

#define MediaInfo_Parser_SubRip         0xFB

/***************************************************************************/
/* N19                                                                     */
/***************************************************************************/

#define MediaInfo_Parser_N19            0xFC

/***************************************************************************/
/* SDP                                                                     */
/***************************************************************************/

#define MediaInfo_Parser_Sdp            0xFD

/*-------------------------------------------------------------------------*/
/* Utilities                                                               */
/*-------------------------------------------------------------------------*/

// Flags: bit 0 = has ID containing 2 numbers (e.g. 0x00010002, main ID is 1 and subID is 2) 
#define MediaInfo_ID_FromEvent_int MediaInfo_ID_FromEvent_intA
inline MediaInfo_int64u MediaInfo_ID_FromEvent_intA (MediaInfo_Event_Generic* Event, int) // Flags)
{
    if (Event->StreamIDs_Size>=2 && Event->ParserIDs[0]==MediaInfo_Parser_MpegPs && Event->ParserIDs[1]==MediaInfo_Parser_MpegPs_Ext)   // DVD-Video
        return (Event->StreamIDs[0]<<16) | Event->StreamIDs[1];
    if (Event->StreamIDs_Size>=2 && Event->ParserIDs[0]==MediaInfo_Parser_HdsF4m && Event->StreamIDs_Width[1])                          // HDS F4M having more than one stream per referenced file
        return Event->StreamIDs[0]<<16 | Event->StreamIDs[1];
    if (Event->StreamIDs_Size>=3 && Event->ParserIDs[0]==MediaInfo_Parser_HlsIndex)                                                     // HLS Index
    {
        if (Event->ParserIDs[1]==MediaInfo_Parser_Hls)
            return (Event->StreamIDs[0]<<16) | Event->StreamIDs[2];
        else
            return (Event->StreamIDs[0]<<16) | Event->StreamIDs[1];
    }
    if (Event->StreamIDs_Size>=2 && Event->ParserIDs[0]==MediaInfo_Parser_Hls)                                                          // HLS Sequence
        return Event->StreamIDs[1];
    if (Event->StreamIDs_Size>=2 && Event->ParserIDs[0]==MediaInfo_Parser_Ism && Event->StreamIDs_Width[1])                             // ISM having more than one stream per referenced file
        return Event->StreamIDs[0]<<16 | Event->StreamIDs[1];
    if (Event->StreamIDs_Size>=3 && Event->ParserIDs[0]==MediaInfo_Parser_Dxw && Event->ParserIDs[1]==MediaInfo_Parser_MpegTs)          // DXW with MPEG-TS
        return (Event->StreamIDs[0]<<16) | Event->StreamIDs[1];
    if (Event->StreamIDs_Size>=3 && Event->ParserIDs[0]==MediaInfo_Parser_DcpAm)                                                        // DCP/IMF AM
        return (Event->StreamIDs[0]<<24) | (Event->StreamIDs[1]<<16) | Event->StreamIDs[2];
    if (Event->StreamIDs_Size>=3 && Event->ParserIDs[0]==MediaInfo_Parser_DcpPkl)                                                       // DCP/IMF PKL
        return (Event->StreamIDs[0]<<24) | (Event->StreamIDs[1]<<16) | Event->StreamIDs[2];
    if (Event->StreamIDs_Size>=2 && Event->ParserIDs[0]==MediaInfo_Parser_DcpCpl)                                                       // DCP/IMF CPL
        return (Event->StreamIDs[0]<<16) | Event->StreamIDs[1];
    if (Event->StreamIDs_Size)
        return Event->StreamIDs[0];
    return (MediaInfo_int64u)-1; // No identifier
}

#ifdef __cplusplus
#define MediaInfo_ID_FromEvent_string MediaInfo_ID_FromEvent_stringA
inline std::string MediaInfo_ID_FromEvent_stringA (MediaInfo_Event_Generic* Event, int) // Flags)
{
    std::stringstream ToReturn;

    if (Event->StreamIDs_Size>=2 && Event->ParserIDs[0]==MediaInfo_Parser_MpegPs && Event->ParserIDs[1]==MediaInfo_Parser_MpegPs_Ext)        // DVD-Video
    {
        ToReturn<<Event->StreamIDs[0];
        ToReturn<<'-';
        ToReturn<<Event->StreamIDs[1];
    }
    else if (Event->StreamIDs_Size>=2 && Event->ParserIDs[0]==MediaInfo_Parser_HdsF4m && Event->StreamIDs_Width[1])                          // HDS F4M having more than one stream per referenced file
    {
        ToReturn<<Event->StreamIDs[0];
        ToReturn<<'-';
        ToReturn<<Event->StreamIDs[1];
    }
    else if (Event->StreamIDs_Size>=3 && Event->ParserIDs[0]==MediaInfo_Parser_HlsIndex)                                                     // HLS Index
    {
        ToReturn<<Event->StreamIDs[0];
        ToReturn<<'-';
        if (Event->ParserIDs[1]==MediaInfo_Parser_Hls)
            ToReturn<<Event->StreamIDs[2];
        else
            ToReturn<<Event->StreamIDs[1];
    }
    else if (Event->StreamIDs_Size>=2 && Event->ParserIDs[0]==MediaInfo_Parser_Hls)                                                          // HLS Sequence
    {
        ToReturn<<Event->StreamIDs[1];
    }
    else if (Event->StreamIDs_Size>=2 && Event->ParserIDs[0]==MediaInfo_Parser_Ism && Event->StreamIDs_Width[1])                             // ISM having more than one stream per referenced file
    {
        ToReturn<<Event->StreamIDs[0];
        ToReturn<<'-';
        ToReturn<<Event->StreamIDs[1];
    }
    else if (Event->StreamIDs_Size>=3 && Event->ParserIDs[0]==MediaInfo_Parser_Dxw && Event->ParserIDs[1]==MediaInfo_Parser_MpegTs)          // DXW with MPEG-TS
    {
        ToReturn<<Event->StreamIDs[0];
        ToReturn<<'-';
        ToReturn<<Event->StreamIDs[1];
    }
    else if (Event->StreamIDs_Size>=3 && Event->ParserIDs[0]==MediaInfo_Parser_DcpAm)                                                        // DCP/IMF AM
    {
        ToReturn<<Event->StreamIDs[0];
        ToReturn<<'-';
        ToReturn<<Event->StreamIDs[1];
        ToReturn<<'-';
        ToReturn<<Event->StreamIDs[2];
    }
    else if (Event->StreamIDs_Size>=3 && Event->ParserIDs[0]==MediaInfo_Parser_DcpPkl)                                                       // DCP/IMF PKL
    {
        ToReturn<<Event->StreamIDs[0];
        ToReturn<<'-';
        ToReturn<<Event->StreamIDs[1];
        ToReturn<<'-';
        ToReturn<<Event->StreamIDs[2];
    }
    else if (Event->StreamIDs_Size>=3 && Event->ParserIDs[0]==MediaInfo_Parser_DcpCpl)                                                       // DCP/IMF CPL
    {
        ToReturn<<Event->StreamIDs[0];
        ToReturn<<'-';
        ToReturn<<Event->StreamIDs[1];
    }
    else if (Event->StreamIDs_Size)
    {
        ToReturn<<Event->StreamIDs[0];
    }
    return ToReturn.str();
}
#endif //__cplusplus

#define MediaInfo_ID_FromGet_int MediaInfo_ID_FromGet_intA
inline MediaInfo_int64u MediaInfo_ID_FromGet_intA (const char* ID, const char* ContainerFormat, const char* MuxingMode)
{
    MediaInfo_int64u ToReturn;
    const char* SubID;

    ToReturn=(MediaInfo_int64u)atoi(ID);
    SubID= strchr(ID, '-');
    if (SubID)
    {
        MediaInfo_int64u ToReturn2;

        ToReturn2=atoi(SubID+1);

        SubID= strchr(SubID+1, '-');
        if (SubID)
        {
            MediaInfo_int64u ToReturn3;

            ToReturn3=atoi(SubID+1);
            if (strcmp(ContainerFormat, "DCP AM")==0                                            // DCP AM
             || strcmp(ContainerFormat, "DCP PKL")==0                                           // DCP PKL
             || strcmp(ContainerFormat, "IMF AM")==0                                            // IMF AM
             || strcmp(ContainerFormat, "IMF PKL")==0)                                          // IMF PKL
                ToReturn=(ToReturn<<24) | (ToReturn2<<16) | ToReturn3;
        }
        else if (
            strcmp(MuxingMode, "DVD-Video")==0                                                  // DVD-Video
         || strcmp(ContainerFormat, "HDS F4M")==0                                               // HDS F4M having more than one stream per referenced file
         || (strcmp(ContainerFormat, "HLS")==0 && strstr(MuxingMode, "HLS")==MuxingMode)        // HLS Index
         || (strcmp(ContainerFormat, "HLS")==0 && strstr(MuxingMode, "MPEG-TS")==MuxingMode)    // HLS Index
         || strcmp(ContainerFormat, "ISM")==0                                                   // ISM having more than one stream per referenced file
         || (strcmp(ContainerFormat, "DXW")==0 && strstr(MuxingMode, "MPEG-TS")==MuxingMode)    // DXW with MPEG-TS
         || strcmp(ContainerFormat, "DCP CPL")==0                                               // DCP CPL
         || strcmp(ContainerFormat, "IMF CPL")==0)                                              // IMF CPL
            ToReturn=(ToReturn<<16) | ToReturn2;
    }

    return ToReturn;
}

#endif //MediaInfo_EventsH

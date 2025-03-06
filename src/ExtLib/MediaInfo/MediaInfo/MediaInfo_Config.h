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
#ifndef MediaInfo_ConfigH
#define MediaInfo_ConfigH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Setup.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/MediaInfo_Internal_Const.h"
#if MEDIAINFO_EVENTS
    #include "MediaInfo/MediaInfo_Events.h"
#endif //MEDIAINFO_EVENTS
#include "ZenLib/CriticalSection.h"
#include "ZenLib/ZtringListList.h"
#include "ZenLib/Translation.h"
#include "ZenLib/InfoMap.h"
#include <set>
#include <bitset>
using namespace ZenLib;
using std::vector;
using std::string;
using std::map;
using std::set;
using std::make_pair;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

enum basicformat
{
    BasicFormat_Text,
    BasicFormat_CSV,
    BasicFormat_XML,
    BasicFormat_JSON,
};
#if MEDIAINFO_ADVANCED
    #define MEDIAINFO_FLAG1 1
    enum config_flags1
    {
        Flags_Cover_Data_base64,
        Flags_Enable_FFmpeg,
    };
#else //MEDIAINFO_COMPRESS
    #define MEDIAINFO_FLAG1 0
#endif //MEDIAINFO_COMPRESS

#if MEDIAINFO_COMPRESS
    #define MEDIAINFO_FLAGX 1
    enum config_flagsX
    {
        Flags_Inform_zlib,
        Flags_Inform_base64,
        Flags_Input_zlib,
        Flags_Input_base64,
    };
#else //MEDIAINFO_COMPRESS
    #define MEDIAINFO_FLAGX 0
#endif //MEDIAINFO_COMPRESS

enum class display_if
{
    Never,
    Needed,
    Supported,
    Always,
};

//***************************************************************************
// Class MediaInfo_Config
//***************************************************************************

class MediaInfo_Config
{
public :
    //Constructor/Destructor
    MediaInfo_Config() {}
    void Init(bool Force=false); //Must be called instead of constructor

    //General
    Ztring Option (const String &Option, const String &Value=Ztring());

    //Info
          void      Complete_Set (size_t NewValue);
          size_t    Complete_Get ();

          void      BlockMethod_Set (size_t NewValue);
          size_t    BlockMethod_Get ();

          void      Internet_Set (size_t NewValue);
          size_t    Internet_Get ();

          void      MultipleValues_Set (size_t NewValue);
          size_t    MultipleValues_Get ();

          #if MEDIAINFO_ADVANCED
          void      ParseOnlyKnownExtensions_Set (const Ztring &NewValue);
          Ztring    ParseOnlyKnownExtensions_Get();
          bool      ParseOnlyKnownExtensions_IsSet();
          set<Ztring> ParseOnlyKnownExtensions_GetList_Set();
          Ztring    ParseOnlyKnownExtensions_GetList_String();
          #endif //MEDIAINFO_ADVANCED

          void      ShowFiles_Set (const ZtringListList &NewShowFiles);
          size_t    ShowFiles_Nothing_Get ();
          size_t    ShowFiles_VideoAudio_Get ();
          size_t    ShowFiles_VideoOnly_Get ();
          size_t    ShowFiles_AudioOnly_Get ();
          size_t    ShowFiles_TextOnly_Get ();

          void      ReadByHuman_Set (bool NewValue);
          bool      ReadByHuman_Get ();

          void      Legacy_Set (bool NewValue);
          bool      Legacy_Get ();

          void      LegacyStreamDisplay_Set (bool Value);
          bool      LegacyStreamDisplay_Get ();

          void      SkipBinaryData_Set (bool Value);
          bool      SkipBinaryData_Get ();

          void      ParseSpeed_Set (float32 NewValue);
          float32   ParseSpeed_Get ();

          void      Verbosity_Set (float32 NewValue);
          float32   Verbosity_Get ();

          void      Trace_Level_Set (const ZtringListList &NewDetailsLevel);
          float32   Trace_Level_Get ();
          std::bitset<32> Trace_Layers_Get ();

          void      Compat_Set (int64u NewValue);
          int64u    Compat_Get ();

          void      Https_Set(bool NewValue);
          bool      Https_Get();

          void      Trace_TimeSection_OnlyFirstOccurrence_Set (bool Value);
          bool      Trace_TimeSection_OnlyFirstOccurrence_Get ();

          enum trace_Format
          {
              Trace_Format_Tree,
              Trace_Format_CSV,
              Trace_Format_XML,
              Trace_Format_MICRO_XML,
          };
          void      Trace_Format_Set (trace_Format NewValue);
          trace_Format Trace_Format_Get ();

          void      Trace_Modificator_Set (const ZtringList &NewModifcator); //Not implemented
          Ztring    Trace_Modificator_Get (const Ztring &Modificator); //Not implemented

          void      Demux_Set (int8u NewValue);
          int8u     Demux_Get ();

          void      LineSeparator_Set (const Ztring &NewValue);
          Ztring    LineSeparator_Get ();

          void      Version_Set (const Ztring &NewValue);
          Ztring    Version_Get ();

          void      ColumnSeparator_Set (const Ztring &NewValue);
          Ztring    ColumnSeparator_Get ();

          void      TagSeparator_Set (const Ztring &NewValue);
          Ztring    TagSeparator_Get ();

          void      Quote_Set (const Ztring &NewValue);
          Ztring    Quote_Get ();

          void      DecimalPoint_Set (const Ztring &NewValue);
          Ztring    DecimalPoint_Get ();

          void      ThousandsPoint_Set (const Ztring &NewValue);
          Ztring    ThousandsPoint_Get ();

          void      CarriageReturnReplace_Set (const Ztring &NewValue);
          Ztring    CarriageReturnReplace_Get ();

          void      StreamMax_Set (const ZtringListList &NewValue);
          Ztring    StreamMax_Get ();

          void      Language_Set (const ZtringListList &NewLanguage);
          Ztring    Language_Get ();
          Ztring    Language_Get (const Ztring &Value);
          Ztring    Language_Get_Translate(const Ztring &Par, const Ztring &Value);
          Ztring    Language_Get (const Ztring &Count, const Ztring &Value, bool ValueIsAlwaysSame=false);

          void      Inform_Set (const ZtringListList &NewInform);
          Ztring    Inform_Get ();
          Ztring    Inform_Get (const Ztring &Value);

          void      Inform_Replace_Set (const ZtringListList &NewInform_Replace);
          ZtringListList Inform_Replace_Get_All ();

          void      Inform_Version_Set (bool NewValue);
          bool      Inform_Version_Get ();

          void      Inform_Timestamp_Set (bool NewValue);
          bool      Inform_Timestamp_Get ();

          #if MEDIAINFO_ADVANCED
          Ztring    Cover_Data_Set (const Ztring &NewValue);
          Ztring    Cover_Data_Get ();
          #endif //MEDIAINFO_ADVANCED
          #if MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)
          Ztring    Enable_FFmpeg_Set (bool NewValue);
          bool      Enable_FFmpeg_Get ();
          #endif //MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES
          #if MEDIAINFO_COMPRESS
          Ztring    Inform_Compress_Set (const Ztring &NewInform);
          Ztring    Inform_Compress_Get ();
          Ztring    Input_Compressed_Set(const Ztring &NewInform);
          Ztring    Input_Compressed_Get();
          #endif //MEDIAINFO_COMPRESS
          #if MEDIAINFO_FLAG1
          bool      Flags1_Get(config_flags1 Flag) { return Flags1&(static_cast<int64u>(1) << Flag); }
          #endif //MEDIAINFO_FLAGX
          #if MEDIAINFO_FLAGX
          bool      FlagsX_Get(config_flagsX Flag) { return FlagsX&(static_cast<int64u>(1) << Flag); }
          #endif //MEDIAINFO_FLAGX

    const Ztring   &Format_Get (const Ztring &Value, infoformat_t KindOfFormatInfo=InfoFormat_Name);
          InfoMap  &Format_Get(); //Should not be, but too difficult to hide it

    const Ztring   &Codec_Get (const Ztring &Value, infocodec_t KindOfCodecInfo=InfoCodec_Name);
    const Ztring   &Codec_Get (const Ztring &Value, infocodec_t KindOfCodecInfo, stream_t KindOfStream);

    const Ztring   &CodecID_Get (stream_t KindOfStream, infocodecid_format_t Format, const Ztring &Value, infocodecid_t KindOfCodecIDInfo=InfoCodecID_Format);

    const Ztring   &Library_Get (infolibrary_format_t Format, const Ztring &Value, infolibrary_t KindOfLibraryInfo=InfoLibrary_Version);

    const Ztring   &Iso639_1_Get (const Ztring &Value);
    const Ztring   &Iso639_2_Get (const Ztring &Value);
    const Ztring    Iso639_Find (const Ztring &Value);
    const Ztring    Iso639_Translate (const Ztring &Value);

    const Ztring   &Info_Get (stream_t KindOfStream, const Ztring &Value, info_t KindOfInfo=Info_Text);
    const Ztring   &Info_Get (stream_t KindOfStream, size_t Pos, info_t KindOfInfo=Info_Text);
    const ZtringListList &Info_Get(stream_t KindOfStream); //Should not be, but too difficult to hide it

          Ztring    Info_Parameters_Get (bool Complete=false);
          Ztring    HideShowParameter   (const Ztring &Value, ZenLib::Char Show);
          Ztring    Info_OutputFormats_Get(basicformat Format);
          Ztring    Info_Tags_Get       () const;
          Ztring    Info_CodecsID_Get   ();
          Ztring    Info_Codecs_Get     ();
          Ztring    Info_Version_Get    () const;
          Ztring    Info_Url_Get        () const;

    const Ztring   &EmptyString_Get() const; //Use it when we can't return a reference to a true string
    const ZtringListList &EmptyStringListList_Get() const; //Use it when we can't return a reference to a true string list list

          void      FormatDetection_MaximumOffset_Set (int64u Value);
          int64u    FormatDetection_MaximumOffset_Get ();

    #if MEDIAINFO_ADVANCED
          void      VariableGopDetection_Occurences_Set (int64u Value);
          int64u    VariableGopDetection_Occurences_Get ();
          void      VariableGopDetection_GiveUp_Set (bool Value);
          bool      VariableGopDetection_GiveUp_Get ();
          void      InitDataNotRepeated_Occurences_Set (int64u Value);
          int64u    InitDataNotRepeated_Occurences_Get ();
          void      InitDataNotRepeated_GiveUp_Set (bool Value);
          bool      InitDataNotRepeated_GiveUp_Get ();
    #endif //MEDIAINFO_ADVANCED
    #if MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)
          void      TimeOut_Set (int64u Value);
          int64u    TimeOut_Get ();
          void      AcceptSignals_Set (bool Value);
          bool      AcceptSignals_Get ();
    #endif //MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)

          void      MpegTs_MaximumOffset_Set (int64u Value);
          int64u    MpegTs_MaximumOffset_Get ();
          void      MpegTs_MaximumScanDuration_Set (int64u Value);
          int64u    MpegTs_MaximumScanDuration_Get ();
          void      MpegTs_ForceStreamDisplay_Set (bool Value);
          bool      MpegTs_ForceStreamDisplay_Get ();
          void      MpegTs_ForceTextStreamDisplay_Set(bool Value);
          bool      MpegTs_ForceTextStreamDisplay_Get();
    #if MEDIAINFO_ADVANCED
          void      MpegTs_VbrDetection_Delta_Set (float64 Value);
          float64   MpegTs_VbrDetection_Delta_Get ();
          void      MpegTs_VbrDetection_Occurences_Set (int64u Value);
          int64u    MpegTs_VbrDetection_Occurences_Get ();
          void      MpegTs_VbrDetection_GiveUp_Set (bool Value);
          bool      MpegTs_VbrDetection_GiveUp_Get ();
    #endif //MEDIAINFO_ADVANCED

    #if MEDIAINFO_ADVANCED
          Ztring      MAXML_StreamKinds_Get ();
          Ztring      MAXML_Fields_Get (const Ztring &StreamKind);
    #endif //MEDIAINFO_ADVANCED

    #if MEDIAINFO_ADVANCED
          void        Format_Profile_Split_Set (bool Value);
          bool        Format_Profile_Split_Get ();
    #endif //MEDIAINFO_ADVANCED

    #if defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_ADM_YES)
        void        Graph_Adm_ShowTrackUIDs_Set(bool Value);
        bool        Graph_Adm_ShowTrackUIDs_Get();
        void        Graph_Adm_ShowChannelFormats_Set(bool Value);
        bool        Graph_Adm_ShowChannelFormats_Get();
    #endif //defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_ADM_YES)

    #if defined(MEDIAINFO_EBUCORE_YES)
          void        AcquisitionDataOutputMode_Set (size_t Value);
          size_t      AcquisitionDataOutputMode_Get ();
    #endif //MEDIAINFO_EBUCORE_YES
    #if defined(MEDIAINFO_EBUCORE_YES) || defined(MEDIAINFO_NISO_YES) || MEDIAINFO_ADVANCED
          void        ExternalMetadata_Set (Ztring Value);
          Ztring      ExternalMetadata_Get ();
          void        ExternalMetaDataConfig_Set (Ztring Value);
          Ztring      ExternalMetaDataConfig_Get ();
    #endif //MEDIAINFO_EBUCORE_YES || defined(MEDIAINFO_NISO_YES) || MEDIAINFO_ADVANCED

    ZtringListList  SubFile_Config_Get ();

    #if MEDIAINFO_CONFORMANCE
          Ztring      Conformance_Limit_Set (const Ztring &Value);
          int64u      Conformance_Limit_Get ();
    #endif //MEDIAINFO_CONFORMANCE
    #if MEDIAINFO_ADVANCED
          void        Collection_Trigger_Set (const Ztring& Value);
          int64s      Collection_Trigger_Get();
          Ztring      Collection_Display_Set(const Ztring& Value);
          display_if  Collection_Display_Get();
    #else //MEDIAINFO_ADVANCED
          display_if  Collection_Display_Get() {return display_if::Needed;}
    #endif //MEDIAINFO_ADVANCED

    void            CustomMapping_Set (const Ztring &Value);
    Ztring          CustomMapping_Get (const Ztring &Format, const Ztring &Field);
    bool            CustomMapping_IsPresent (const Ztring &Format, const Ztring &Field);

          void      ErrorLog_Callback_Set(const Ztring &Value);
          void      ErrorLog(const Ztring &Value);

    #if MEDIAINFO_EVENTS
          bool     Event_CallBackFunction_IsSet ();
          Ztring   Event_CallBackFunction_Set (const Ztring &Value);
          Ztring   Event_CallBackFunction_Get ();
          void     Event_Send(const int8u* Data_Content, size_t Data_Size);
          void     Event_Send(const int8u* Data_Content, size_t Data_Size, const Ztring &File_Name);
          void     Log_Send(int8u Type, int8u Severity, int32u MessageCode, const Ztring &Message);
          void     Log_Send(int8u Type, int8u Severity, int32u MessageCode, const char* Message) {return Log_Send(Type, Severity, MessageCode, Ztring().From_Local(Message));}
    #else //MEDIAINFO_EVENTS
          inline void Log_Send(int8u Type, int8u Severity, int32u MessageCode, const Ztring &Message) {}
          inline void Log_Send(int8u Type, int8u Severity, int32u MessageCode, const char* Message) {}
    #endif //MEDIAINFO_EVENTS

    #if defined(MEDIAINFO_GRAPHVIZ_YES)
        bool GraphSvgPluginState();
    #endif //defined(MEDIAINFO_GRAPH_YES)

    #if MEDIAINFO_CONFORMANCE
          string        AdmProfile (const Ztring& Value);
          struct adm_profile
          {
              bool Auto;
              int8u BS2076;
              int8u Ebu3392;

              adm_profile() :
                  Auto(false),
                  BS2076((int8u)-1),
                  Ebu3392((int8u)-1)
              {}
          };
          adm_profile   AdmProfile();
          string        AdmProfile_List();
          string        Mp4Profile(const Ztring& Value);
          string        Mp4Profile();
          string        Mp4Profile_List();
          string        UsacProfile(const Ztring& Value);
          int8u         UsacProfile();
          string        UsacProfile_List();
          string        Profile_List();
          void          WarningError(bool Value);
          bool          WarningError();
    #endif

    #if defined(MEDIAINFO_LIBCURL_YES)
          bool      CanHandleUrls();
          enum urlencode
          {
              URLEncode_No,
              URLEncode_Guess,
              URLEncode_Yes,
          };
          void      URLEncode_Set(urlencode NewValue);
          urlencode URLEncode_Get();
          void      Ssh_PublicKeyFileName_Set (const Ztring &NewValue);
          Ztring    Ssh_PublicKeyFileName_Get ();
          void      Ssh_PrivateKeyFileName_Set (const Ztring &NewValue);
          Ztring    Ssh_PrivateKeyFileName_Get ();
          void      Ssh_IgnoreSecurity_Set (bool NewValue);
          void      Ssh_KnownHostsFileName_Set (const Ztring &NewValue);
          Ztring    Ssh_KnownHostsFileName_Get ();
          bool      Ssh_IgnoreSecurity_Get ();
          void      Ssl_CertificateFileName_Set (const Ztring &NewValue);
          Ztring    Ssl_CertificateFileName_Get ();
          void      Ssl_CertificateFormat_Set (const Ztring &NewValue);
          Ztring    Ssl_CertificateFormat_Get ();
          void      Ssl_PrivateKeyFileName_Set (const Ztring &NewValue);
          Ztring    Ssl_PrivateKeyFileName_Get ();
          void      Ssl_PrivateKeyFormat_Set (const Ztring &NewValue);
          Ztring    Ssl_PrivateKeyFormat_Get ();
          void      Ssl_CertificateAuthorityFileName_Set (const Ztring &NewValue);
          Ztring    Ssl_CertificateAuthorityFileName_Get ();
          void      Ssl_CertificateAuthorityPath_Set (const Ztring &NewValue);
          Ztring    Ssl_CertificateAuthorityPath_Get ();
          void      Ssl_CertificateRevocationListFileName_Set (const Ztring &NewValue);
          Ztring    Ssl_CertificateRevocationListFileName_Get ();
          void      Ssl_IgnoreSecurity_Set (bool NewValue);
          bool      Ssl_IgnoreSecurity_Get ();
    #endif //defined(MEDIAINFO_LIBCURL_YES)
    #if MEDIAINFO_FIXITY
          void      TryToFix_Set (bool NewValue);
          bool      TryToFix_Get ();
    #endif //MEDIAINFO_FIXITY

private :
    int64u          FormatDetection_MaximumOffset;
    #if MEDIAINFO_ADVANCED
        int64u      VariableGopDetection_Occurences;
        bool        VariableGopDetection_GiveUp;
        int64u      InitDataNotRepeated_Occurences;
        bool        InitDataNotRepeated_GiveUp;
    #endif //MEDIAINFO_ADVANCED
    #if MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)
        int64u      TimeOut;
        bool        AcceptSignals;
    #endif //MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)

    int64u          MpegTs_MaximumOffset;
    int64u          MpegTs_MaximumScanDuration;
    bool            MpegTs_ForceStreamDisplay;
    bool            MpegTs_ForceTextStreamDisplay;
    #if MEDIAINFO_ADVANCED
        float64     MpegTs_VbrDetection_Delta;
        int64u      MpegTs_VbrDetection_Occurences;
        bool        MpegTs_VbrDetection_GiveUp;
    #endif //MEDIAINFO_ADVANCED
    #if MEDIAINFO_ADVANCED
        bool        Format_Profile_Split;
    #endif //MEDIAINFO_ADVANCED
    #if defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_ADM_YES)
        bool        Graph_Adm_ShowTrackUIDs;
        bool        Graph_Adm_ShowChannelFormats;
    #endif //defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_ADM_YES)
    #if defined(MEDIAINFO_EBUCORE_YES) || defined(MEDIAINFO_NISO_YES) || MEDIAINFO_ADVANCED
        size_t      AcquisitionDataOutputMode;
        Ztring      ExternalMetadata;
        Ztring      ExternalMetaDataConfig;
    #endif //defined(MEDIAINFO_EBUCORE_YES) || defined(MEDIAINFO_NISO_YES) || MEDIAINFO_ADVANCED
    size_t          Complete;
    size_t          BlockMethod;
    size_t          Internet;
    size_t          MultipleValues;
    #ifdef MEDIAINFO_ADVANCED
    Ztring          ParseOnlyKnownExtensions;
    #endif
    size_t          ShowFiles_Nothing;
    size_t          ShowFiles_VideoAudio;
    size_t          ShowFiles_VideoOnly;
    size_t          ShowFiles_AudioOnly;
    size_t          ShowFiles_TextOnly;
    float32         ParseSpeed;
    float32         Verbosity;
    float32         Trace_Level;
    int64u          Compat;
    bool            Https;
    bool            Trace_TimeSection_OnlyFirstOccurrence;
    std::bitset<32> Trace_Layers; //0-7: Container, 8: Stream
    std::map<Ztring, bool> Trace_Modificators; //If we want to add/remove some details
    bool            Language_Raw;
    bool            ReadByHuman;
    bool            Inform_Version;
    bool            Inform_Timestamp;
    bool            Legacy;
    bool            LegacyStreamDisplay;
    bool            SkipBinaryData;
    int8u           Demux;
    Ztring          Version;
    Ztring          ColumnSeparator;
    Ztring          LineSeparator;
    Ztring          TagSeparator;
    Ztring          Quote;
    Ztring          DecimalPoint;
    Ztring          ThousandsPoint;
    Ztring          CarriageReturnReplace;
    Translation     Language; //ex. : "KB;Ko"
    ZtringListList  Custom_View; //Definition of "General", "Video", "Audio", "Text", "Other", "Image"
    ZtringListList  Custom_View_Replace; //ToReplace;ReplaceBy
    #if MEDIAINFO_FLAG1
        int64u      Flags1;
    #endif //MEDIAINFO_FLAG1
    #if MEDIAINFO_FLAGX
        int64u      FlagsX;
    #endif //MEDIAINFO_FLAGX
    trace_Format    Trace_Format;

    InfoMap         Container;
    InfoMap         CodecID[InfoCodecID_Format_Max][Stream_Max];
    InfoMap         Format;
    InfoMap         Codec;
    InfoMap         Library[InfoLibrary_Format_Max];
    InfoMap         Iso639_1;
    InfoMap         Iso639_2;
    ZtringListList  Info[Stream_Max]; //General info

    ZtringListList  SubFile_Config;

    #if MEDIAINFO_CONFORMANCE
        int64u      Conformance_Limit;
    #endif //MEDIAINFO_CONFORMANCE
    #if MEDIAINFO_ADVANCED
        int64s      Collection_Trigger;
        display_if  Collection_Display;
    #endif //MEDIAINFO_ADVANCED

    std::map<Ztring, std::map<Ztring, Ztring> > CustomMapping;

    ZenLib::CriticalSection CS;

    void      Language_Set (stream_t StreamKind);
    void      Language_Set_Internal(stream_t KindOfStream);
    void      Language_Set_All(stream_t KindOfStream)
    {
        CriticalSectionLocker CSL(CS);
        Language_Set_Internal(KindOfStream);
    }
    //Event
    #if MEDIAINFO_EVENTS
    MediaInfo_Event_CallBackFunction* Event_CallBackFunction; //void Event_Handler(unsigned char* Data_Content, size_t Data_Size, void* UserHandler)
    void*           Event_UserHandler;
    #endif //MEDIAINFO_EVENTS

    #if MEDIAINFO_CONFORMANCE
    adm_profile     Adm_Profile;
    string          Mp4_Profile;
    int8u           Usac_Profile;
    bool            Warning_Error;
    #endif

    #if defined(MEDIAINFO_LIBCURL_YES)
          urlencode URLEncode;
          Ztring    Ssh_PublicKeyFileName;
          Ztring    Ssh_PrivateKeyFileName;
          Ztring    Ssh_KnownHostsFileName;
          bool      Ssh_IgnoreSecurity;
          Ztring    Ssl_CertificateFileName;
          Ztring    Ssl_CertificateFormat;
          Ztring    Ssl_PrivateKeyFileName;
          Ztring    Ssl_PrivateKeyFormat;
          Ztring    Ssl_CertificateAuthorityFileName;
          Ztring    Ssl_CertificateAuthorityPath;
          Ztring    Ssl_CertificateRevocationListFileName;
          bool      Ssl_IgnoreSecurity;
    #endif //defined(MEDIAINFO_LIBCURL_YES)

    #if MEDIAINFO_FIXITY
        bool        TryToFix;
    #endif //MEDIAINFO_SEEK

    //Constructor
    MediaInfo_Config (const MediaInfo_Config&);             // Prevent copy-construction
    MediaInfo_Config& operator=(const MediaInfo_Config&);   // Prevent assignment
};

extern MediaInfo_Config Config;

} //NameSpace

#endif

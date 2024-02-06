/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

 // audioProgramme
 // - audioContent
 // - - audioObject
 // - - - audioPackFormat
 // - - - - audioChannelFormat
 // - - - audioTrackUID
 // - - - - audioChannelFormat +
 // - - - - audioPackFormat
 // - - - - - audioChannelFormat +
 // - - - - audioTrackFormat
 // - - - - - audioStreamFormat
 // - - - - - - audioChannelFormat +
 // - - - - - - audioPackFormat +
 // - - - - - - audioTrackFormat +
 
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
#if defined(MEDIAINFO_ADM_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Adm.h"
#include "ThirdParty/tfsxml/tfsxml.h"
#include <algorithm>
#include <bitset>
#include <cstdlib>
using namespace ZenLib;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

#define IncoherencyMessage "Incoherency between enums and message strings"

static const char* ADM_Dolby_1_0 = " (additional constraint from Dolby Atmos Master ADM Profile v1.0)";
static const char* ADM_Dolby_1_1 = " (additional constraint from Dolby Atmos Master ADM Profile v1.1)";

//---------------------------------------------------------------------------
static const char Hex2String_List[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
static string Hex2String(int Value, size_t Size)
{
    string Result;
    Result.resize(Size);
    for (int i = (int)Size - 1; i >= 0; i--) {
        Result[i] = Hex2String_List[Value & 0xF];
        Value >>= 4;
    }
    return Result;
}

//---------------------------------------------------------------------------
static int32u pow10(int i)
{
    static int pow10[10] =
    {
        1,
        10,
        100,
        1000,
        10000, 
        100000,
        1000000,
        10000000,
        100000000,
        1000000000,
    };
    return pow10[i];
}

static float32 TimeCodeToFloat(string v)
{
    if (v.size() < 8 || v[2] != ':' || v[5] != ':')
        return 0;
    for (int i = 0; i < 8; i++)
    {
        switch (i)
        {
        case 2:
        case 5:
            if (v[i] != ':')
                return 0;
            break;
        default:
            if (v[i] < '0' || v[i] > '9')
                return 0;
        }
    }
    int32u  Value = (v[0] - '0') * 10 * 6 * 10 * 6 * 10
                  + (v[1] - '0')      * 6 * 10 * 6 * 10
                  + (v[3] - '0')          * 10 * 6 * 10
                  + (v[4] - '0')               * 6 * 10
                  + (v[6] - '0')                   * 10
                  + (v[7] - '0')                       ;
    if (v.size() < 9 || v[8] != '.')
        return Value;
    int i = 9;
    int32u ValueF = 0;
    int ValueF_Exponent = 0;
    const int Exponent_Max = 9;
    while (i < v.size() && v[i] >= '0' && v[i] <= '9')
    {
        ValueF_Exponent++;
        if (ValueF_Exponent <= Exponent_Max)
        {
            ValueF *= 10;
            ValueF += v[i] - '0';
        }
        i++;
    }
    if (i >= v.size() || v[i] != 'S')
        return Value + (float32)ValueF / pow10(ValueF_Exponent <= Exponent_Max ? ValueF_Exponent : Exponent_Max);
    int32u SampleRate = 0;
    int SampleRate_Exponent = 0;
    i++;
    while (i < v.size() && v[i] >= '0' && v[i] <= '9')
    {
        SampleRate_Exponent++;
        if (SampleRate_Exponent <= Exponent_Max)
        {
            SampleRate *= 10;
            SampleRate += v[i] - '0';
        }
        i++;
    }
    while (SampleRate_Exponent > Exponent_Max)
    {
        if (ValueF_Exponent < Exponent_Max)
            ValueF_Exponent /= 10;
        if (ValueF_Exponent)
            ValueF_Exponent--;
        SampleRate_Exponent--;
    }
    return Value + (float32)ValueF / SampleRate;
}

//---------------------------------------------------------------------------
static const char* profile_names[]=
{
    "profileName",
    "profileVersion",
    "profileID",
    "levelID",
};
static const int profile_names_size=(int)sizeof(profile_names)/sizeof(const char*);
static const char* profile_names_InternalID[profile_names_size]=
{
    "Format",
    "Version",
    "Profile",
    "Level",
};
struct profile_info
{
    string Strings[4];
    string profile_info_build(size_t Max=profile_names_size)
    {
        bool HasParenthsis=false;
        string ToReturn;
        for (size_t i=0; i<Max; i++)
        {
            if (!Strings[i].empty())
            {
                if (!ToReturn.empty())
                {
                    if (i==1)
                        ToReturn+=", Version";
                    if (!HasParenthsis)
                        ToReturn+=' ';
                }
                if (i>=2)
                {
                    if (!HasParenthsis)
                    {
                        ToReturn+='(';
                        HasParenthsis=true;
                    }
                    else
                    {
                        ToReturn+=',';
                        ToReturn+=' ';
                    }
                }
                if (i>=2)
                {
                    ToReturn+=profile_names[i];
                    ToReturn+='=';
                }
                ToReturn+=Strings[i];
            }
        }
        if (HasParenthsis)
            ToReturn+=')';
        return ToReturn;
    }
};

static bool IsHexaDigit(char Value)
{
    return (Value >= '0' && Value <= '9')
        || (Value >= 'A' && Value <= 'F')
        || (Value >= 'a' && Value <= 'f');
}

static bool IsHexaDigit(const string& Value, size_t pos, size_t len)
{
    len += pos;
    for (; pos < len; pos++) {
        if (!IsHexaDigit(Value[pos]))
            return false;
    }
    return true;
}

enum check_flags_items {
    Version0,
    Version1,
    Version2,
    Count0,
    Count2,
    Dolby0,
    Dolby1,
    Dolby2,
    flags_Max
};
class check_flags : public bitset<flags_Max> {
public:
    constexpr check_flags(unsigned long val) : bitset(val) {}
    constexpr check_flags(unsigned long val0, unsigned long val1, unsigned long val2, unsigned long val3, unsigned long val4, unsigned long val5, unsigned long val6, unsigned long val7)
    : check_flags((val0 << 0) | (val1 << 1) | (val2 << 2) | (val3 << 3) | (val4 << 4) | (val5 << 5) | (val6 << 6) | (val7 << 7)) {}
};

struct attribute_item {
    const char*         Name;
    check_flags         Flags;
};
typedef attribute_item attribute_items[];

struct element_item {
    const char*         Name;
    check_flags         Flags;
};
typedef element_item element_items[];

enum item {
    item_audioTrack,
    item_audioProgramme,
    item_audioContent,
    item_audioObject,
    item_audioPackFormat,
    item_audioChannelFormat,
    item_audioTrackUID,
    item_audioTrackFormat,
    item_audioStreamFormat,
    item_audioBlockFormat,
    item_position,
    item_alternativeValueSet,
    item_Max
};

enum audioTrack_Attribute {
    audioTrack_trackID,
    audioTrack_formatLabel,
    audioTrack_formatDefinition,
    audioTrack_Attribute_Max
};

enum audioTrack_Element {
    audioTrack_audioTrackUIDRef,
    audioTrack_Element_Max
};

enum audioProgramme_Attribute {
    audioProgramme_audioProgrammeID,
    audioProgramme_audioProgrammeName,
    audioProgramme_audioProgrammeLanguage,
    audioProgramme_start,
    audioProgramme_end,
    audioProgramme_typeLabel,
    audioProgramme_typeDefinition,
    audioProgramme_typeLink,
    audioProgramme_typeLanguage,
    audioProgramme_formatLabel,
    audioProgramme_formatDefinition,
    audioProgramme_formatLink,
    audioProgramme_formatLanguage,
    audioProgramme_maxDuckingDepth,
    audioProgramme_Attribute_Max
};

enum audioProgramme_Element {
    audioProgramme_audioProgrammeLabel,
    audioProgramme_audioContentIDRef,
    audioProgramme_loudnessMetadata,
    audioProgramme_loudnessMetadata_integratedLoudness,
    audioProgramme_audioProgrammeReferenceScreen,
    audioProgramme_authoringInformation,
    audioProgramme_authoringInformation_referenceLayout,
    audioProgramme_authoringInformation_referenceLayout_audioPackFormatIDRef,
    audioProgramme_alternativeValueSetIDRef,
    audioProgramme_Element_Max
};

enum audioContent_Attribute {
    audioContent_audioContentID,
    audioContent_audioContentName,
    audioContent_audioContentLanguage,
    audioContent_typeLabel,
    audioContent_Attribute_Max
};

enum audioContent_Element {
    audioContent_audioContentLabel,
    audioContent_audioObjectIDRef,
    audioContent_loudnessMetadata,
    audioContent_loudnessMetadata_integratedLoudness,
    audioContent_dialogue,
    audioContent_alternativeValueSetIDRef,
    audioContent_Element_Max
};

enum audioObject_Attribute {
    audioObject_audioObjectID,
    audioObject_audioObjectName,
    audioObject_start,
    audioObject_startTime,
    audioObject_duration,
    audioObject_dialogue,
    audioObject_importance,
    audioObject_interact,
    audioObject_disableDucking,
    audioObject_typeLabel,
    audioObject_Attribute_Max
};

enum audioObject_Element {
    audioObject_audioPackFormatIDRef,
    audioObject_audioObjectIDRef,
    audioObject_audioObjectLabel,
    audioObject_audioComplementaryObjectGroupLabel,
    audioObject_audioComplementaryObjectIDRef,
    audioObject_audioTrackUIDRef,
    audioObject_audioObjectInteraction,
    audioObject_gain,
    audioObject_headLocked,
    audioObject_positionOffset,
    audioObject_mute,
    audioObject_alternativeValueSet,
    audioObject_Element_Max
};

enum audioPackFormat_Attribute {
    audioPackFormat_audioPackFormatID,
    audioPackFormat_audioPackFormatName,
    audioPackFormat_typeDefinition,
    audioPackFormat_typeLabel,
    audioPackFormat_typeLink,
    audioPackFormat_typeLanguage,
    audioPackFormat_importance,
    audioPackFormat_Attribute_Max
};

enum audioPackFormat_Element {
    audioPackFormat_audioChannelFormatIDRef,
    audioPackFormat_audioPackFormatIDRef,
    audioPackFormat_absoluteDistance,
    audioPackFormat_encodePackFormatIDRef,
    audioPackFormat_decodePackFormatIDRef,
    audioPackFormat_inputPackFormatIDRef,
    audioPackFormat_outputPackFormatIDRef,
    audioPackFormat_normalization,
    audioPackFormat_nfcRefDist,
    audioPackFormat_screenRef,
    audioPackFormat_Element_Max
};

enum audioChannelFormat_Attribute {
    audioChannelFormat_audioChannelFormatID,
    audioChannelFormat_audioChannelFormatName,
    audioChannelFormat_typeDefinition,
    audioChannelFormat_typeLabel,
    audioChannelFormat_typeLink,
    audioChannelFormat_typeLanguage,
    audioChannelFormat_Attribute_Max
};

enum audioChannelFormat_Element {
    audioChannelFormat_audioBlockFormat,
    audioChannelFormat_frequency,
    audioChannelFormat_Element_Max
};

enum audioTrackUID_Attribute {
    audioTrackUID_UID,
    audioTrackUID_sampleRate,
    audioTrackUID_bitDepth,
    audioTrackUID_typeLabel,
    audioTrackUID_Attribute_Max
};

enum audioTrackUID_Element {
    audioTrackUID_audioMXFLookUp,
    audioTrackUID_audioTrackFormatIDRef,
    audioTrackUID_audioChannelFormatIDRef,
    audioTrackUID_audioPackFormatIDRef,
    audioTrackUID_Element_Max
};

enum audioTrackFormat_Attribute {
    audioTrackFormat_audioTrackFormatID,
    audioTrackFormat_audioTrackFormatName,
    audioTrackFormat_typeDefinition,
    audioTrackFormat_typeLabel,
    audioTrackFormat_formatLabel,
    audioTrackFormat_formatDefinition,
    audioTrackFormat_formatLink,
    audioTrackFormat_formatLanguage,
    audioTrackFormat_Attribute_Max
};

enum audioTrackFormat_Element {
    audioTrackFormat_audioStreamFormatIDRef,
    audioTrackFormat_Element_Max
};

enum audioStreamFormat_Attribute {
    audioStreamFormat_audioStreamFormatID,
    audioStreamFormat_audioStreamFormatName,
    audioStreamFormat_typeLabel,
    audioStreamFormat_typeDefinition,
    audioStreamFormat_formatLabel,
    audioStreamFormat_formatDefinition,
    audioStreamFormat_formatLink,
    audioStreamFormat_formatLanguage,
    audioStreamFormat_Attribute_Max
};

enum audioStreamFormat_Element {
    audioStreamFormat_audioChannelFormatIDRef,
    audioStreamFormat_audioPackFormatIDRef,
    audioStreamFormat_audioTrackFormatIDRef,
    audioStreamFormat_Element_Max
};

enum audioBlockFormat_Attribute {
    audioBlockFormat_audioBlockFormatID,
    audioBlockFormat_rtime,
    audioBlockFormat_duration,
    audioBlockFormat_lstart,
    audioBlockFormat_lduration,
    audioBlockFormat_initializeBlock,
    audioBlockFormat_Attribute_Max
};

enum audioBlockFormat_Element {
    audioBlockFormat_gain,
    audioBlockFormat_importance,
    audioBlockFormat_headLocked,
    audioBlockFormat_headphoneVirtualise,
    audioBlockFormat_speakerLabel,
    audioBlockFormat_position,
    audioBlockFormat_outputChannelFormatIDRef,
    audioBlockFormat_outputChannelIDRef,
    audioBlockFormat_jumpPosition,
    audioBlockFormat_matrix,
    audioBlockFormat_coefficient,
    audioBlockFormat_width,
    audioBlockFormat_height,
    audioBlockFormat_depth,
    audioBlockFormat_cartesian,
    audioBlockFormat_diffuse,
    audioBlockFormat_channelLock,
    audioBlockFormat_objectDivergence,
    audioBlockFormat_zoneExclusion,
    audioBlockFormat_equation,
    audioBlockFormat_order,
    audioBlockFormat_degree,
    audioBlockFormat_normalization,
    audioBlockFormat_nfcRefDist,
    audioBlockFormat_screenRef,
    audioBlockFormat_Element_Max
};

enum position_Attribute {
    position_coordinate,
    position_bound,
    position_screenEdgeLock,
    position_Attribute_Max
};

enum position_Element {
    position_Element_Max
};

enum error_Type {
    Error,
    Warning,
    error_Type_Max,
};

static const char* error_Type_String[] = {
    "Errors",
    "Warnings",
};

enum schema {
    Schema_Unknown,
    Schema_ebuCore_2014,
    Schema_ebuCore_2016,
    Schema_Max
};

static const char* schema_String[] = {
    "ebuCore_2014",
    "ebuCore_2016",
};
static constexpr size_t schema_String_Size = sizeof(schema_String) / sizeof(schema_String[0]);
static_assert(schema_String_Size + 1 == Schema_Max, IncoherencyMessage);


                                                  // Version |Count|Dolby
                                                  // 0  1  2 |0  2+|0  1  2+
                                                  // 0  1  2 |0  2+|0  1  2+
static attribute_items audioTrack_Attributes =
{
    { "trackID"                                 , {  1, 1, 1, 1, 0, 0, 0, 0 } },
    { "formatLabel"                             , {  1, 1, 1, 1, 0, 0, 0, 0 } },
    { "formatDefinition"                        , {  1, 1, 1, 1, 0, 0, 0, 0 } },
};
static_assert(sizeof(audioTrack_Attributes) / sizeof(attribute_item) == audioTrack_Attribute_Max, IncoherencyMessage);

static element_items audioTrack_Elements =
{
    { "audioTrackUIDRef"                        , {  1, 1, 1, 1, 1, 0, 0, 0 } },
};
static_assert(sizeof(audioTrack_Elements) / sizeof(attribute_item) == audioTrack_Element_Max, IncoherencyMessage);

static attribute_items audioProgramme_Attributes =
{
    { "audioProgrammeID"                        , { 1, 1, 1, 0, 0, 0, 1, 0 } },
    { "audioProgrammeName"                      , { 1, 1, 1, 0, 0, 0, 1, 0 } },
    { "audioProgrammeLanguage"                  , { 1, 1, 1, 1, 0, 1, 1, 0 } },
    { "start"                                   , { 1, 1, 1, 1, 0, 0, 1, 0 } },
    { "end"                                     , { 1, 1, 1, 1, 0, 0, 1, 0 } },
    { "typeLabel"                               , { 1, 0, 0, 1, 0, 1, 0, 0 } },
    { "typeDefinition"                          , { 1, 0, 0, 1, 0, 1, 0, 0 } },
    { "typeLink"                                , { 1, 0, 0, 1, 0, 1, 0, 0 } },
    { "typeLanguage"                            , { 1, 0, 0, 1, 0, 1, 0, 0 } },
    { "formatLabel"                             , { 1, 0, 0, 1, 0, 1, 0, 0 } },
    { "formatDefinition"                        , { 1, 0, 0, 1, 0, 1, 0, 0 } },
    { "formatLink"                              , { 1, 0, 0, 1, 0, 1, 0, 0 } },
    { "formatLanguage"                          , { 1, 0, 0, 1, 0, 1, 0, 0 } },
    { "maxDuckingDepth"                         , { 1, 1, 1, 1, 0, 1, 0, 0 } },
};
static_assert(sizeof(audioProgramme_Attributes) / sizeof(attribute_item) == audioProgramme_Attribute_Max, IncoherencyMessage);

static element_items audioProgramme_Elements =
{
    { "audioProgrammeLabel"                     , { 0, 0, 1, 1, 1, 1, 0, 0 } },
    { "audioContentIDRef"                       , { 1, 1, 1, 0, 1, 0, 1, 1 } },
    { "loudnessMetadata"                        , { 1, 1, 1, 1, 1, 1, 1, 0 } },
    { "integratedLoudness"                      , { 1, 1, 1, 1, 1, 1, 1, 1 } }, // TODO
    { "audioProgrammeReferenceScreen"           , { 1, 1, 1, 1, 0, 1, 1, 0 } },
    { "authoringInformation"                    , { 0, 0, 1, 1, 0, 1, 0, 0 } },
    { "referenceLayout"                         , { 0, 0, 1, 1, 0, 1, 0, 0 } }, // TODO
    { "audioPackFormatIDRef"                    , { 0, 0, 1, 1, 0, 1, 0, 0 } }, // TODO
    { "alternativeValueSetIDRef"                , { 0, 0, 1, 1, 1, 1, 0, 0 } },
};
static_assert(sizeof(audioProgramme_Elements) / sizeof(attribute_item) == audioProgramme_Element_Max, IncoherencyMessage);

static attribute_items audioContent_Attributes =
{
    { "audioContentID"                          , { 1, 1, 1, 0, 0, 0, 1, 0 } },
    { "audioContentName"                        , { 1, 1, 1, 0, 0, 0, 1, 0 } },
    { "audioContentLanguage"                    , { 1, 1, 1, 1, 0, 1, 1, 0 } },
    { "typeLabel"                               , { 1, 0, 0, 1, 0, 1, 0, 0 } }, // TODO
};
static_assert(sizeof(audioContent_Attributes) / sizeof(attribute_item) == audioContent_Attribute_Max, IncoherencyMessage);

static element_items audioContent_Elements =
{
    { "audioContentLabel"                       , { 0, 0, 1, 1, 1, 1, 0, 0 } },
    { "audioObjectIDRef"                        , { 1, 1, 1, 0, 1, 0, 1, 1 } },
    { "loudnessMetadata"                        , { 1, 1, 1, 1, 1, 1, 1, 0 } },
    { "loudnessMetadata_integratedLoudness"     , { 1, 1, 1, 1, 1, 1, 1, 1 } }, // TODO
    { "dialogue"                                , { 1, 1, 1, 1, 0, 0, 1, 0 } },
    { "alternativeValueSetIDRef"                , { 0, 0, 1, 1, 1, 1, 0, 0 } },
};
static_assert(sizeof(audioContent_Elements) / sizeof(attribute_item) == audioContent_Element_Max, IncoherencyMessage);

static attribute_items audioObject_Attributes =
{
    { "audioObjectID"                           , { 1, 1, 1, 0, 0, 0, 1, 0 } },
    { "audioObjectName"                         , { 1, 1, 1, 0, 0, 0, 1, 0 } },
    { "start"                                   , { 0, 1, 1, 1, 0, 1, 1, 0 } },
    { "startTime"                               , { 1, 0, 0, 1, 0, 1, 1, 0 } },
    { "duration"                                , { 1, 1, 1, 1, 0, 0, 1, 0 } },
    { "dialogue"                                , { 1, 1, 1, 1, 0, 1, 1, 0 } },
    { "importance"                              , { 1, 1, 1, 1, 0, 1, 1, 0 } },
    { "interact"                                , { 1, 1, 1, 1, 0, 1, 1, 0 } },
    { "disableDucking"                          , { 1, 1, 1, 1, 0, 1, 1, 0 } },
    { "typeLabel"                               , { 1, 0, 0, 1, 0, 1, 0, 0 } }, // TODO
};
static_assert(sizeof(audioObject_Attributes) / sizeof(attribute_item) == audioObject_Attribute_Max, IncoherencyMessage);

static element_items audioObject_Elements =
{
    { "audioPackFormatIDRef"                    , { 1, 1, 1, 1, 1, 0, 1, 0 } },
    { "audioObjectIDRef"                        , { 1, 1, 1, 1, 1, 1, 0, 0 } },
    { "audioObjectLabel"                        , { 0, 0, 1, 1, 1, 1, 0, 0 } },
    { "audioComplementaryObjectGroupLabel"      , { 0, 0, 1, 1, 1, 1, 0, 0 } },
    { "audioComplementaryObjectIDRef"           , { 1, 1, 1, 1, 1, 1, 0, 0 } },
    { "audioTrackUIDRef"                        , { 1, 1, 1, 1, 1, 0, 1, 1 } },
    { "audioObjectInteraction"                  , { 1, 1, 1, 1, 0, 1, 0, 0 } },
    { "gain"                                    , { 0, 0, 1, 1, 0, 1, 0, 0 } },
    { "headLocked"                              , { 0, 0, 1, 1, 0, 1, 0, 0 } },
    { "positionOffset"                          , { 0, 0, 1, 1, 0, 1, 0, 0 } },
    { "mute"                                    , { 0, 0, 1, 1, 0, 1, 0, 0 } },
    { "alternativeValueSet"                     , { 0, 0, 1, 1, 1, 1, 0, 0 } },
};
static_assert(sizeof(audioObject_Elements) / sizeof(attribute_item) == audioObject_Element_Max, IncoherencyMessage);

static attribute_items audioPackFormat_Attributes =
{
    { "audioPackFormatID"                       , { 1, 1, 1, 0, 0, 0, 1, 0 } },
    { "audioPackFormatName"                     , { 1, 1, 1, 0, 0, 0, 1, 0 } },
    { "typeDefinition"                          , { 1, 1, 1, 1, 0, 0, 1, 0 } },
    { "typeLabel"                               , { 1, 1, 1, 1, 0, 0, 1, 0 } },
    { "typeLink"                                , { 1, 0, 0, 1, 0, 1, 0, 0 } },
    { "typeLanguage"                            , { 1, 0, 0, 1, 0, 1, 0, 0 } },
    { "importance"                              , { 1, 1, 1, 1, 0, 1, 1, 0 } },
};
static_assert(sizeof(audioPackFormat_Attributes) / sizeof(attribute_item) == audioPackFormat_Attribute_Max, IncoherencyMessage);

static element_items audioPackFormat_Elements =
{
    { "audioChannelFormatIDRef"                 , { 1, 1, 1, 1, 1, 0, 1, 1 } },
    { "audioPackFormatIDRef"                    , { 1, 1, 1, 1, 1, 1, 0, 0 } },
    { "absoluteDistance"                        , { 1, 1, 1, 1, 0, 1, 0, 0 } },
    { "encodePackFormatIDRef"                   , { 0, 1, 1, 1, 1, 1, 0, 0 } },
    { "decodePackFormatIDRef"                   , { 0, 1, 1, 1, 1, 1, 0, 0 } },
    { "inputPackFormatIDRef"                    , { 0, 1, 1, 1, 0, 1, 0, 0 } },
    { "outputPackFormatIDRef"                   , { 0, 1, 1, 1, 0, 1, 0, 0 } },
    { "normalization"                           , { 0, 1, 1, 1, 0, 1, 0, 0 } },
    { "nfcRefDist"                              , { 0, 1, 1, 1, 0, 1, 0, 0 } },
    { "screenRef"                               , { 0, 1, 1, 1, 0, 1, 0, 0 } },
};
static_assert(sizeof(audioPackFormat_Elements) / sizeof(attribute_item) == audioPackFormat_Element_Max, IncoherencyMessage);

static attribute_items audioChannelFormat_Attributes =
{
    { "audioChannelFormatID"                    , { 1, 1, 1, 0, 0, 0, 1, 0 } },
    { "audioChannelFormatName"                  , { 1, 1, 1, 0, 0, 0, 1, 0 } },
    { "typeDefinition"                          , { 1, 1, 1, 1, 0, 0, 1, 0 } },
    { "typeLabel"                               , { 1, 1, 1, 1, 0, 0, 1, 0 } },
    { "typeLink"                                , { 1, 0, 0, 1, 0, 1, 0, 0 } },
    { "typeLanguage"                            , { 1, 0, 0, 1, 0, 1, 0, 0 } },
};
static_assert(sizeof(audioChannelFormat_Attributes) / sizeof(attribute_item) == audioChannelFormat_Attribute_Max, IncoherencyMessage);

static element_items audioChannelFormat_Elements =
{
    { "audioBlockFormat"                        , { 1, 1, 1, 0, 1, 0, 1, 1 } },
    { "frequency"                               , { 1, 1, 1, 1, 1, 1, 1, 1 } },
};
static_assert(sizeof(audioChannelFormat_Elements) / sizeof(attribute_item) == audioChannelFormat_Element_Max, IncoherencyMessage);

static attribute_items audioTrackUID_Attributes =
{
    { "UID"                                     , { 1, 1, 1, 0, 0, 0, 1, 0 } },
    { "sampleRate"                              , { 1, 1, 1, 1, 0, 0, 1, 0 } },
    { "bitDepth"                                , { 1, 1, 1, 1, 0, 0, 1, 0 } },
    { "typeLabel"                               , { 1, 1, 1, 1, 0, 1, 0, 0 } }, // TODO
};
static_assert(sizeof(audioTrackUID_Attributes) / sizeof(attribute_item) == audioTrackUID_Attribute_Max, IncoherencyMessage);

static element_items audioTrackUID_Elements =
{
    { "audioMXFLookUp"                          , { 1, 1, 1, 1, 0, 1, 0, 0 } },
    { "audioTrackFormatIDRef"                   , { 1, 1, 1, 1, 0, 0, 1, 0 } },
    { "audioChannelFormatIDRef"                 , { 0, 0, 1, 1, 0, 1, 0, 0 } },
    { "audioPackFormatIDRef"                    , { 1, 1, 1, 1, 0, 0, 1, 0 } },
};
static_assert(sizeof(audioTrackUID_Elements) / sizeof(attribute_item) == audioTrackUID_Element_Max, IncoherencyMessage);

static attribute_items audioTrackFormat_Attributes =
{
    { "audioTrackFormatID"                      , { 1, 1, 1, 0, 0, 0, 1, 0 } },
    { "audioTrackFormatName"                    , { 1, 1, 1, 0, 0, 0, 1, 0 } },
    { "typeLabel"                               , { 1, 1, 1, 1, 0, 1, 0, 0 } }, // TODO: present in some Dolby files
    { "typeDefinition"                          , { 0, 0, 0, 1, 0, 1, 0, 0 } }, // TODO: present in some Dolby files
    { "formatLabel"                             , { 1, 1, 1, 1, 0, 0, 1, 0 } },
    { "formatDefinition"                        , { 1, 1, 1, 1, 0, 0, 1, 0 } },
    { "formatLink"                              , { 1, 1, 1, 1, 0, 1, 0, 0 } },
    { "formatLanguage"                          , { 1, 1, 1, 1, 0, 1, 0, 0 } },
};
static_assert(sizeof(audioTrackFormat_Attributes) / sizeof(attribute_item) == audioTrackFormat_Attribute_Max, IncoherencyMessage);

static element_items audioTrackFormat_Elements =
{
    { "audioStreamFormatIDRef"                  , { 1, 1, 1, 0, 0, 0, 1, 0 } },
};
static_assert(sizeof(audioTrackFormat_Elements) / sizeof(attribute_item) == audioTrackFormat_Element_Max, IncoherencyMessage);

static attribute_items audioStreamFormat_Attributes =
{
    { "audioStreamFormatID"                     , { 1, 1, 1, 0, 0, 0, 1, 0 } },
    { "audioStreamFormatName"                   , { 1, 1, 1, 0, 0, 0, 1, 0 } },
    { "typeLabel"                               , { 0, 0, 0, 1, 0, 1, 0, 0 } }, // TODO: present in some Dolby files
    { "typeDefinition"                          , { 0, 0, 0, 1, 0, 1, 0, 0 } }, // TODO: present in some Dolby files
    { "formatLabel"                             , { 1, 1, 1, 1, 0, 0, 1, 0 } },
    { "formatDefinition"                        , { 1, 0, 1, 1, 0, 0, 1, 0 } },
    { "formatLink"                              , { 1, 1, 1, 1, 0, 1, 0, 0 } },
    { "formatLanguage"                          , { 1, 0, 1, 1, 0, 1, 0, 0 } },
};
static_assert(sizeof(audioStreamFormat_Attributes) / sizeof(attribute_item) == audioStreamFormat_Attribute_Max, IncoherencyMessage);

static element_items audioStreamFormat_Elements =
{
    { "audioChannelFormatIDRef"                 , { 1, 1, 1, 1, 1, 0, 1, 0 } },
    { "audioPackFormatIDRef"                    , { 1, 1, 1, 1, 1, 0, 1, 0 } },
    { "audioTrackFormatIDRef"                   , { 1, 1, 1, 1, 1, 0, 1, 0 } },
};
static_assert(sizeof(audioStreamFormat_Elements) / sizeof(attribute_item) == audioStreamFormat_Element_Max, IncoherencyMessage);

static attribute_items audioBlockFormat_Attributes =
{
    { "audioBlockFormatID"                      , { 1, 1, 1, 0, 0, 0, 1, 0 } },
    { "rtime"                                   , { 1, 1, 1, 1, 0, 1, 1, 0 } },
    { "duration"                                , { 1, 1, 1, 1, 0, 1, 1, 0 } },
    { "lstart"                                  , { 1, 1, 1, 1, 0, 1, 0, 0 } },
    { "lduration"                               , { 1, 1, 1, 1, 0, 1, 0, 0 } },
    { "initializeBlock"                         , { 1, 1, 1, 1, 0, 1, 0, 0 } },
};
static_assert(sizeof(audioBlockFormat_Attributes) / sizeof(attribute_item) == audioBlockFormat_Attribute_Max, IncoherencyMessage);

static element_items audioBlockFormat_Elements =
{
    { "gain"                                    , { 1, 1, 1, 1, 0, 1, 1, 0 } },
    { "importance"                              , { 1, 1, 1, 1, 0, 1, 1, 0 } },
    { "headLocked"                              , { 0, 0, 1, 1, 0, 1, 0, 0 } },
    { "headphoneVirtualise"                     , { 0, 0, 1, 1, 0, 1, 0, 0 } },
    { "speakerLabel"                            , { 1, 1, 1, 1, 1, 1, 1, 0 } }, //TODO should be Dolby1 only for Objects only
    { "position"                                , { 1, 1, 1, 1, 1, 0, 1, 1 } },
    { "outputChannelFormatIDRef"                , { 0, 1, 1, 1, 0, 1, 0, 0 } },
    { "outputChannelIDRef,"                     , { 0, 1, 1, 1, 0, 1, 0, 0 } },
    { "jumpPosition"                            , { 1, 1, 1, 1, 0, 1, 1, 0 } }, //TODO should be Dolby1 only for Objects only
    { "matrix"                                  , { 1, 1, 1, 1, 0, 1, 0, 0 } },
    { "coefficient"                             , { 1, 1, 1, 1, 0, 1, 0, 0 } },
    { "width"                                   , { 1, 1, 1, 1, 0, 1, 1, 0 } },
    { "height"                                  , { 1, 1, 1, 1, 0, 1, 1, 0 } },
    { "depth"                                   , { 1, 1, 1, 1, 0, 1, 1, 0 } },
    { "cartesian"                               , { 1, 1, 1, 1, 0, 1, 1, 0 } },
    { "diffuse"                                 , { 1, 1, 1, 1, 1, 1, 1, 0 } },
    { "channelLock"                             , { 1, 1, 1, 1, 1, 1, 1, 0 } },
    { "objectDivergence"                        , { 1, 1, 1, 1, 1, 1, 0, 0 } },
    { "zoneExclusion"                           , { 1, 1, 1, 1, 1, 1, 1, 0 } },
    { "equation"                                , { 1, 1, 1, 1, 1, 1, 0, 0 } },
    { "order"                                   , { 1, 1, 1, 1, 1, 1, 0, 0 } },
    { "degree"                                  , { 1, 1, 1, 1, 1, 1, 0, 0 } },
    { "normalization"                           , { 0, 1, 1, 1, 1, 1, 0, 0 } },
    { "nfcRefDist"                              , { 0, 1, 1, 1, 1, 1, 0, 0 } },
    { "screenRef"                               , { 1, 1, 1, 1, 1, 1, 0, 0 } },
};
static_assert(sizeof(audioBlockFormat_Elements) / sizeof(attribute_item) == audioBlockFormat_Element_Max, IncoherencyMessage);

static attribute_items position_Attributes =
{
    { "coordinate"                              , { 1, 1, 1, 1, 0, 0, 1, 0 } },
    { "bound"                                   , { 1, 1, 1, 1, 0, 1, 0, 0 } },
    { "screenEdgeLock"                          , { 1, 1, 1, 1, 0, 1, 0, 0 } },
};
static_assert(sizeof(position_Attributes) / sizeof(attribute_item) == position_Attribute_Max, IncoherencyMessage);

static const char* formatDefinition_List[] = { "PCM" };
enum format
{
    Format_Unknown,
    Format_PCM,
    Format_Max
};
static_assert(1 + sizeof(formatDefinition_List) / sizeof(formatDefinition_List[0]) == Format_Max, IncoherencyMessage);
static const char* typeDefinition_List[] = { "DirectSpeakers", "Matrix", "Objects", "HOA", "Binaural" };
enum type
{
    Type_Unknown,
    Type_DirectSpeakers,
    Type_Matrix,
    Type_Objects,
    Type_HOA,
    Type_Binaural,
    Type_Max
};
static_assert(1 + sizeof(typeDefinition_List) / sizeof(typeDefinition_List[0]) == Type_Max, IncoherencyMessage);
enum style : int8u {
    Style_Format,
    Style_Type,
    Style_Max = (style)-1
};
struct item_style_info {
    const char** formatDefinition_List;
    const int8u formatDefinition_Size;
};
static item_style_info Style_Infos[] = {
    {formatDefinition_List, Format_Max},
    {typeDefinition_List, Type_Max},
};

struct item_info {
    attribute_items* Attribute_Infos;
    element_items* Element_Infos;
    const char* Name;
    const char* ID_Begin;
    enum flags {
        Flags_ID_W,
        Flags_ID_YX,
        Flags_ID_Z1, // Z width x2
        Flags_ID_Z2, // Z width x2
        Flags_ID_N,
        Flags_Max,
    };
    bitset<item_info::Flags_Max> ID_Flags;
    int8u       ID_Pos;
    int16u      MaxKnown; //TODO: per yyyy
};
typedef item_info item_infos[];
#define D(_0,_1,_2,_3,_4,_5,_6,_7) {_0,_1,_2,_3,_4,_5,_6,_7}
#define F(_0) (1 << item_info::Flags_##_0)
static const item_infos item_Infos = {
    { (attribute_items*)&audioTrack_Attributes, (element_items*)&audioTrack_Elements, "TrackIndex", nullptr, 0, (int8u)-1, 0 },
    { (attribute_items*)&audioProgramme_Attributes, (element_items*)&audioProgramme_Elements, "Programme", "APR", F(ID_W), audioProgramme_audioProgrammeID, 0 },
    { (attribute_items*)&audioContent_Attributes, (element_items*)&audioContent_Elements, "Content", "ACO", F(ID_W), audioContent_audioContentID, 0 },
    { (attribute_items*)&audioObject_Attributes, (element_items*)&audioObject_Elements, "Object", "AO", F(ID_W), audioObject_audioObjectID, 0 },
    { (attribute_items*)&audioPackFormat_Attributes, (element_items*)&audioPackFormat_Elements, "PackFormat", "AP", F(ID_YX), audioPackFormat_audioPackFormatID, 0xFFF },
    { (attribute_items*)&audioChannelFormat_Attributes, (element_items*)&audioChannelFormat_Elements, "ChannelFormat", "AC", F(ID_YX), audioChannelFormat_audioChannelFormatID, 0xFFF },
    { (attribute_items*)&audioTrackUID_Attributes, (element_items*)&audioTrackUID_Elements, "TrackUID", "ATU", F(ID_N), audioTrackUID_UID, 0 },
    { (attribute_items*)&audioTrackFormat_Attributes, (element_items*)&audioTrackFormat_Elements, "TrackFormat", "AT", F(ID_YX) | F(ID_Z1), audioTrackFormat_audioTrackFormatID, 0xFFF },
    { (attribute_items*)&audioStreamFormat_Attributes, (element_items*)&audioStreamFormat_Elements, "StreamFormat", "AS", F(ID_YX), audioStreamFormat_audioStreamFormatID, 0xFFF },
    { (attribute_items*)&audioBlockFormat_Attributes, (element_items*)&audioBlockFormat_Elements, "BlockFormat", "AB", F(ID_YX) | F(ID_Z1) | F(ID_Z2), (int8u)-1, 0xFFF },
    { (attribute_items*)&position_Attributes, nullptr, "position", nullptr, 0, (int8u)-1, 0xFFF},
    { nullptr, nullptr, "alternativeValueSet", "AVS", F(ID_W) | F(ID_Z2), (int8u)-1, 0 },
};
#undef D
#undef F
static_assert(sizeof(item_Infos) / sizeof(item_info) == item_Max, IncoherencyMessage);

struct label_info {
    item        item_Type;
    int8u       Label_Pos;
    int8u       Definition_Pos;
    style       Label_Style;
};
typedef label_info label_infos[];
static constexpr label_infos label_Infos = {
    { item_audioTrack, audioTrack_formatLabel, audioTrack_formatDefinition, Style_Format },
    { item_audioPackFormat, audioPackFormat_typeLabel, audioPackFormat_typeDefinition, Style_Type },
    { item_audioChannelFormat, audioChannelFormat_typeLabel, audioChannelFormat_typeDefinition, Style_Type },
    { item_audioTrackFormat, audioTrackFormat_formatLabel, audioTrackFormat_formatDefinition, Style_Format },
    { item_audioStreamFormat, audioStreamFormat_formatLabel, audioStreamFormat_formatDefinition, Style_Format },
};

struct idref {
    item   Source_Type;
    size_t Source_Pos;
    size_t Target_Type;
};
typedef idref idrefs[];
static constexpr idrefs IDRefs = {
    { item_audioTrack, audioTrack_audioTrackUIDRef, item_audioTrackUID },
    { item_audioProgramme, audioProgramme_audioContentIDRef, item_audioContent },
    //{ item_audioProgramme, audioProgramme_alternativeValueSetIDRef, item_alternativeValueSet },
    { item_audioContent, audioContent_audioObjectIDRef, item_audioObject },
    //{ item_audioContent, audioContent_alternativeValueSetIDRef, item_alternativeValueSet },
    { item_audioObject, audioObject_audioPackFormatIDRef, item_audioPackFormat },
    { item_audioObject, audioObject_audioObjectIDRef, item_audioObject },
    { item_audioObject, audioObject_audioComplementaryObjectIDRef, item_audioObject },
    { item_audioObject, audioObject_audioTrackUIDRef, item_audioTrackUID },
    { item_audioPackFormat, audioPackFormat_audioChannelFormatIDRef, item_audioChannelFormat },
    { item_audioPackFormat, audioPackFormat_audioPackFormatIDRef, item_audioPackFormat },
    { item_audioPackFormat, audioPackFormat_encodePackFormatIDRef, item_audioPackFormat },
    { item_audioPackFormat, audioPackFormat_decodePackFormatIDRef, item_audioPackFormat },
    { item_audioPackFormat, audioPackFormat_inputPackFormatIDRef, item_audioPackFormat },
    { item_audioPackFormat, audioPackFormat_outputPackFormatIDRef, item_audioPackFormat },
    { item_audioTrackUID, audioTrackUID_audioTrackFormatIDRef, item_audioTrackFormat },
    { item_audioTrackUID, audioTrackUID_audioChannelFormatIDRef, item_audioChannelFormat },
    { item_audioTrackUID, audioTrackUID_audioPackFormatIDRef, item_audioPackFormat },
    { item_audioTrackFormat, audioTrackFormat_audioStreamFormatIDRef, item_audioStreamFormat },
    { item_audioStreamFormat, audioStreamFormat_audioChannelFormatIDRef, item_audioChannelFormat },
    { item_audioStreamFormat, audioStreamFormat_audioPackFormatIDRef, item_audioPackFormat },
    { item_audioStreamFormat, audioStreamFormat_audioTrackFormatIDRef, item_audioTrackFormat },
};

enum atmos_audioChannelFormatName {
    Atmos_L,
    Atmos_R,
    Atmos_C,
    Atmos_LFE,
    Atmos_Lss,
    Atmos_Rss,
    Atmos_Lrs,
    Atmos_Rrs,
    Atmos_Lts,
    Atmos_Rts,
    Atmos_Ls,
    Atmos_Rs,
    Atmos_audioChannelFormatName_Max
};
struct atmos_audioChannel_Values {
    const char* Name;
    const char* SpeakerLabel;
    float       Pos0;
    float       Pos1;
    float       Pos2;
};
static const atmos_audioChannel_Values Atmos_audioChannelFormat_Content[] = {
    { "RoomCentricLeft"                 , "RC_L"    , -1      ,  1      ,  0       },
    { "RoomCentricRight"                , "RC_R"    ,  1      ,  1      ,  0       },
    { "RoomCentricCenter"               , "RC_C"    ,  0      ,  1      ,  0       },
    { "RoomCentricLFE"                  , "RC_LFE"  , -1      ,  1      , -1       },
    { "RoomCentricLeftSideSurround"     , "RC_Lss"  , -1      ,  0      ,  0       },
    { "RoomCentricRightSideSurround"    , "RC_Rss"  ,  1      ,  0      ,  0       },
    { "RoomCentricLeftRearSurround"     , "RC_Lrs"  , -1      , -1      ,  0       },
    { "RoomCentricRightRearSurround"    , "RC_Rrs"  ,  1      , -1      ,  0       },
    { "RoomCentricLeftTopSurround"      , "RC_Lts"  , -1      ,  0      ,  1       },
    { "RoomCentricRightTopSurround"     , "RC_Rts"  ,  1      ,  0      ,  1       },
    { "RoomCentricLeftSurround"         , "RC_Ls"   , -1      , -0.36397,  0       },
    { "RoomCentricRightSurround"        , "RC_Rs"   ,  1      , -0.36397,  0       },
};
static_assert(sizeof(Atmos_audioChannelFormat_Content) / sizeof(Atmos_audioChannelFormat_Content[0]) == Atmos_audioChannelFormatName_Max, IncoherencyMessage);
atmos_audioChannelFormatName Atmos_audioChannelFormat_Pos(const string& Value, bool Speaker = false) {
    const auto Value_Size = Value.size();
    for (size_t i = 0; i < Atmos_audioChannelFormatName_Max; i++) {
        const auto& Test = Speaker ? Atmos_audioChannelFormat_Content[i].SpeakerLabel : Atmos_audioChannelFormat_Content[i].Name;
        const auto Test_Size = strlen(Test);
        if (Test_Size == Value_Size && !memcmp((void*)Value.c_str(), Test, Value_Size)) {
            return (atmos_audioChannelFormatName)i;
        }
    }
    return (atmos_audioChannelFormatName)-1;
}
atmos_audioChannelFormatName Atmos_audioChannelFormat_Pos(float32 Pos0, float32 Pos1, float32 Pos2, atmos_audioChannelFormatName Prefered = (atmos_audioChannelFormatName)-1) {
    for (size_t i = 0; i < Atmos_audioChannelFormatName_Max; i++) {
        const auto& Test = Atmos_audioChannelFormat_Content[i];
        if (Test.Pos0 == Pos0 && Test.Pos1 == Pos1 && Test.Pos2 == Pos2) {
            if ((i == Atmos_Lrs && Prefered == Atmos_Ls) || (i == Atmos_Rrs && Prefered == Atmos_Rs)) {
                i = Prefered;
            }
            return (atmos_audioChannelFormatName)i;
        }
    }
    return (atmos_audioChannelFormatName)-1;
}

static const atmos_audioChannelFormatName Atmos_ChannelOrder[] = {
    (atmos_audioChannelFormatName) 2, Atmos_L, Atmos_R,
    (atmos_audioChannelFormatName) 3, Atmos_L, Atmos_R, Atmos_C,
    (atmos_audioChannelFormatName) 5, Atmos_L, Atmos_R, Atmos_C, Atmos_Ls, Atmos_Rs,
    (atmos_audioChannelFormatName) 6, Atmos_L, Atmos_R, Atmos_C, Atmos_LFE, Atmos_Ls, Atmos_Rs,
    (atmos_audioChannelFormatName) 7, Atmos_L, Atmos_R, Atmos_C, Atmos_Lss, Atmos_Rss, Atmos_Lrs, Atmos_Rrs,
    (atmos_audioChannelFormatName) 8, Atmos_L, Atmos_R, Atmos_C, Atmos_LFE, Atmos_Lss, Atmos_Rss, Atmos_Lrs, Atmos_Rrs,
    (atmos_audioChannelFormatName) 9, Atmos_L, Atmos_R, Atmos_C, Atmos_Lss, Atmos_Rss, Atmos_Lrs, Atmos_Rrs, Atmos_Lts, Atmos_Rts,
    (atmos_audioChannelFormatName)10, Atmos_L, Atmos_R, Atmos_C, Atmos_LFE, Atmos_Lss, Atmos_Rss, Atmos_Lrs, Atmos_Rrs, Atmos_Lts, Atmos_Rts,
    (atmos_audioChannelFormatName) 0
};
bool Atmos_ChannelOrder_Find(const vector<atmos_audioChannelFormatName>& List) {
    auto List_Size = List.size();
    size_t i = 0;
    for (;;) {
        const size_t ChannelCount = Atmos_ChannelOrder[i++];
        if (!ChannelCount) {
            return false;
        }
        if (ChannelCount != List_Size) {
            i += ChannelCount;
            continue;
        }
        size_t j = 0;
        for (; j < ChannelCount; j++) {
            if (List[j] != Atmos_ChannelOrder[i + j]) {
                break;
            }
        }
        if (j < ChannelCount) {
            continue;
        }
        return true;
    }
}

struct Item_Struct {
    vector<string> Attributes;
    bitset<64> Attributes_Present;
    vector<vector<string> > Elements;
    map<string, string> Extra;
    vector<string> Errors[error_Type_Max];

    void AddError(error_Type Type, const string& NewValue)
    {
        auto& Error = Errors[Type];
        if (find(Error.begin(), Error.end(), NewValue) == Error.end()) {
            if (Error.size() < 9) {
                Errors[Type].push_back(NewValue);
            }
            else if (Error.size() == 9) {
                if (!NewValue.empty() && NewValue[0] == ':') {
                    auto Space = NewValue.find(' ');
                    auto End = NewValue.rfind(':', Space);
                    if (End != string::npos) {
                        Errors[Type].push_back(NewValue.substr(0, End + 1) + "[...]");
                    }
                }
            }
        }
    }
};

struct Items_Struct {
    void Init(size_t Strings_Size_, size_t Elements_Size_) {
        Attributes_Size = Strings_Size_;
        Elements_Size =Elements_Size_;
    }

    Item_Struct& New(bool Init = true)
    {
        if (!Init) {
            return Items.back();
        }
        Items.resize(Items.size() + 1);
        Item_Struct& Item = Items[Items.size() - 1];
        Item.Attributes.resize(Attributes_Size);
        Item.Elements.resize(Elements_Size);
        return Item;
    }

    Item_Struct& Last()
    {
        Item_Struct& Item = Items[Items.size() - 1];
        return Item;
    }

    vector<Item_Struct> Items;
    size_t Attributes_Size;
    size_t Elements_Size;
};

static string Apply_Init(File__Analyze& F, const char* Name, size_t i, const Items_Struct& audioProgramme_List, Ztring Summary) {
    const Item_Struct& audioProgramme = audioProgramme_List.Items[i];
    string P = Name + to_string(i);
    F.Fill(Stream_Audio, 0, P.c_str(), Summary.empty() ? __T("Yes") : Summary);
    F.Fill(Stream_Audio, 0, (P + " Pos").c_str(), i);
    F.Fill_SetOptions(Stream_Audio, 0, (P + " Pos").c_str(), "N NIY");
    return P;
}

static void Apply_SubStreams(File__Analyze& F, const string& P_And_LinkedTo, Item_Struct& Source, size_t i, Items_Struct& Dest) {
    ZtringList SubstreamPos, SubstreamNum;
    for (size_t j = 0; j < Source.Elements[i].size(); j++) {
        const string& ID = Source.Elements[i][j];
        size_t Pos = -1;
        for (size_t k = 0; k < Dest.Items.size(); k++) {
            if (Dest.Items[k].Attributes[0] == ID) {
                Pos = k;
                break;
            }
        }
        if (Pos == -1) {
            auto LinkedTo_Pos = P_And_LinkedTo.find(" LinkedTo_TrackUID_Pos");
            auto HasTransport = !F.Retrieve_Const(Stream_Audio, 0, "Transport0").empty();
            if (HasTransport && LinkedTo_Pos != string::npos) { // TODO: better way to avoid common definitions
                string Message;
                if (LinkedTo_Pos) {
                    auto Sub_Pos = P_And_LinkedTo.rfind(' ', LinkedTo_Pos - 1);
                    if (Sub_Pos != string::npos) {
                        Message += ":transportTrackFormat:audioTrack:audioTrackUIDRef:\"";
                        Message += ID;
                        Message += "\" is referenced but its description is missing";
                        Source.Errors[Warning].push_back(Message);
                    }
                }
            }
            continue;
        }
        SubstreamPos.push_back(Ztring::ToZtring(Pos));
        SubstreamNum.push_back(Ztring::ToZtring(Pos + 1));
    }
    if (SubstreamPos.empty())
        return;
    SubstreamPos.Separator_Set(0, __T(" + "));
    F.Fill(Stream_Audio, 0, P_And_LinkedTo.c_str(), SubstreamPos.Read());
    F.Fill_SetOptions(Stream_Audio, 0, P_And_LinkedTo.c_str(), "N NIY");
    SubstreamNum.Separator_Set(0, __T(" + "));
    F.Fill(Stream_Audio, 0, (P_And_LinkedTo + "/String").c_str(), SubstreamNum.Read());
    F.Fill_SetOptions(Stream_Audio, 0, (P_And_LinkedTo + "/String").c_str(), "Y NIN");
}


//***************************************************************************
// Private class
//***************************************************************************

static bool CheckErrors_ID(file_adm_private* File_Adm_Private, const string& ID, const item_info& ID_Start, vector<Item_Struct>* Items = nullptr, const char* Sub = nullptr);

class tfsxml
{
public:
    void Enter();
    void Leave();
    int Init(const void* const Buffer, size_t Buffer_Size);
    int NextElement();
    int Attribute();
    int Value();
    bool IsInit() { return IsInit_; }
    size_t Remain() { return p.len < 0 ? 0 : (size_t)p.len; }
    void SetSavedAttribute(int Pos) { Attributes_[Level].insert({Pos, tfsxml_decode(v)}); }
    const map<int, string>::iterator GetSavedAttribute(int Pos, int UpperLevel = 0) { return Attributes_[Level - UpperLevel].find(Pos); }
    const map<int, string>::iterator NoSavedAttribute(int UpperLevel = 0) { return Attributes_[Level - UpperLevel].end(); }

private:
    tfsxml_string p;

public:
    tfsxml_string b, v;

//private:
    string Code[16];
    map<int, string> Attributes_[16];
    int8u Level = 0;
    int8u Level_Max = 0;
    bool IsInit_ = false;
    bool MustEnter = false;
    bool ParsingAttr = false;
};

class file_adm_private : public tfsxml
{
public:
    // In
    bool IsSub;

    // Out
    Items_Struct Items[item_Max];
    int Version = 0;
    int Version_S = -1; // S-ADM
    schema Schema = Schema_Unknown;
    bool DolbyProfileCanNotBeVersion1 = false;
    vector<profile_info> profileInfos;
    map<string, string> More;
    float32 FrameRate_Sum = 0;
    float32 FrameRate_Den = 0;

    file_adm_private()
    {
    }

    // Actions
    int parse(const void* const Buffer, size_t Buffer_Size);
    void clear();

    // Helpers
    void chna_Add(int32u Index, const string& TrackUID)
    {
        if (Index >= 0x10000)
            return;
        if (Items[item_audioTrack].Items.empty())
            Items[item_audioTrack].Init(audioTrack_Attribute_Max, audioTrack_Element_Max);
        while (Items[item_audioTrack].Items.size() <= Index)
            Items[item_audioTrack].New();
        Item_Struct& Item = Items[item_audioTrack].Items[Items[item_audioTrack].Items.size() - 1];
        Item.Elements[audioTrack_audioTrackUIDRef].push_back(TrackUID);
    }

    void Check_Attributes_NotPartOfSpecs(size_t Type, size_t Pos, const tfsxml_string& b, Item_Struct& Content)
    {
        Content.AddError(Warning, ":audio" + string(item_Infos[Type].Name) + to_string(Pos) + ":GeneralCompliance:\"" + tfsxml_decode(b) + "\" attribute is not part of specs");
    }

    void Check_Elements_NotPartOfSpecs(size_t Type, size_t Pos, const tfsxml_string& b, Item_Struct& Content)
    {
        Content.AddError(Warning, ":audio" + string(item_Infos[Type].Name) + to_string(Pos) + ":GeneralCompliance:\"" + tfsxml_decode(b) + "\" element is not part of specs");
    }

    void Check_Attributes_ShallBePresent(size_t Type, size_t Pos, const vector<size_t>& Attributes_Counts, const attribute_item* const Attributes, Item_Struct& Content)
    {
        for (size_t i = 0; i < Attributes_Counts.size(); ++i) {
            if (Attributes_Counts[i] == 0 && !(Attributes[i].Flags[Count0])) {
                Content.AddError(Error, ":audio" + string(item_Infos[Type].Name) + to_string(Pos) + ":" + string(Attributes[i].Name) + ":" + string(Attributes[i].Name) + " attribute is not present");
            }
            if (Attributes_Counts[i] > 1) {
                Content.AddError(Error, ":audio" + string(item_Infos[Type].Name) + to_string(Pos) + ":" + string(Attributes[i].Name) + ":" + string(Attributes[i].Name) + " attribute shall be unique");
            }
            if (Attributes_Counts[i]) {
                Content.Attributes_Present.set(i);
            }
        }
    }

    void Check_Elements_ShallBePresent(size_t Type, size_t Pos, const vector<vector<string> >& Elements_Values, const element_item* const Elements, Item_Struct& Content)
    {
        for (size_t i = 0; i < Elements_Values.size(); ++i) {
            if (Elements_Values[i].empty() && !(Elements[i].Flags[Count0])) {
                auto Level = (!strcmp(item_Infos[Type].Name, "TrackFormat") && strcmp(item_Infos[Type].Name, "TrackFormat")) ? Error : Warning;
                Content.AddError(Level, ":audio" + string(item_Infos[Type].Name) + to_string(Pos) + ":" + string(Elements[i].Name) + ":" + string(Elements[i].Name) + " element is not present");
            }
            if (Elements_Values[i].size() > 1 && !(Elements[i].Flags[Count2])) {
                Content.AddError(Error, ":audio" + string(item_Infos[Type].Name) + to_string(Pos) + ":" + string(Elements[i].Name) + ":" + string(Elements[i].Name) + " element shall be unique");
            }
        }
    }

//private:

    int coreMetadata();
    int format();
    int audioFormatExtended();
    int frameHeader();
    int transportTrackFormat();

    // Common definitions
    vector<string> audioChannelFormatIDRefs;

    // Temp
    vector<size_t> Attributes_Counts[16];
};

void tfsxml::Enter() {
    if (Level == Level_Max) {
        if (MustEnter) {
            return;
        }
        MustEnter = true;
        Level++;
        Level_Max = Level;
    }
    else {
        Level++;
    }
}

void tfsxml::Leave() {
    b.len = 0;
    v.len = 0;
    IsInit_ = false;
    MustEnter = false;
    if (Level != (decltype(Level))-1) {
        Level--;
        Level_Max = Level;
    }
}


int tfsxml::Init(const void* const Buffer, size_t Buffer_Size) {
    if (!p.buf)
    {
        int Result = tfsxml_init(&p, Buffer, Buffer_Size, 0);
        if (Result) {
            return Result;
        }
    }
    else
    {
        p.buf = (const char*)Buffer;
        p.len = Buffer_Size;
    }
    return 0;
}

int tfsxml::NextElement() {
    IsInit_ = false;
    if (MustEnter && Level == Level_Max + ParsingAttr) {
        int Result;
        if (Level > 1)
            Result = tfsxml_enter(&p);
        else
            Result = 0;
        if (Result > 0) {
            Level = 0;
            return Result;
        }
        MustEnter = false;
        if (Result) {
            return Result;
        }
    }
    if (Level == Level_Max + ParsingAttr) {
        auto Result = tfsxml_next(&p, &b);
        if (Result < 0) {
            return Result;
        }
        if (Result > 0) {
            Level = 0;
            return Result;
        }
        IsInit_ = true;
        Attributes_[Level].clear();
        Code[Level] = tfsxml_decode(b);
        Code[Level + 1].clear();
    }
    else {
        b.buf = Code[Level].c_str();
        b.len = Code[Level].size();
    }
    return 0;
}

int tfsxml::Attribute() {
    IsInit_ = false;
    if (Level == Level_Max) {
        auto Result = tfsxml_attr(&p, &b, &v);
        if (Result > 0) {
            ParsingAttr = true;
            Level = 0;
            return Result;
        }
        ParsingAttr = false;
        return Result;
    }
    return -1;
}

int tfsxml::Value() {
    auto Result = tfsxml_value(&p, &b);
    if (Result > 0) {
        ParsingAttr = true;
        Level = 0;
        return Result;
    }
    ParsingAttr = false;
    return Result;
}

int file_adm_private::parse(const void* const Buffer, size_t Buffer_Size)
{
    # define STRUCTS(NAME) \
        Items[item_##NAME].Init(NAME##_Attribute_Max, NAME##_Element_Max);

    STRUCTS(audioTrack);
    STRUCTS(audioProgramme);
    STRUCTS(audioContent);
    STRUCTS(audioObject);
    STRUCTS(audioPackFormat);
    STRUCTS(audioChannelFormat);
    STRUCTS(audioTrackUID);
    STRUCTS(audioTrackFormat);
    STRUCTS(audioStreamFormat);
    STRUCTS(audioBlockFormat);
    STRUCTS(position);
 
    #define XML_BEGIN \

    #define XML_ATTR_START \
        for (;;) { \
            { \
                int Result = Attribute(); \
                if (Result < 0) { \
                    break; \
                } \
                if (Result > 0) { \
                    return Result; \
                } \
            } \
            if (false) { \
            } \

    #define XML_ATTR_END \
        } \

    #define XML_VALUE \
        { \
            auto Result = Value(); \
            if (Result > 0) { \
                return Result; \
            } \
        } \

    #define XML_ELEM_START \
        Enter(); \
        for (;;) { \
            { \
                int Result = NextElement(); \
                if (Result < 0) { \
                    break; \
                } \
                if (Result > 0) { \
                    return Result; \
                } \
            } \
            if (false) { \
            } \

    #define XML_ELEM_END \
        } \
        Leave(); \

    #define XML_SUB(_A) \
        { \
            int Result = _A; \
            if (Result > 0) { \
                return Result; \
            } \
        } \

    #define XML_END \
        return -1;
   
    #define ELEMENT_START2(NAME,CALL) \
        if (!tfsxml_strcmp_charp(b, #NAME)) \
        { \
            Item_Struct& NAME##_Content = Items[item_##NAME].New(IsInit()); \
            if (IsInit()) { \
                CALL; \
                Attributes_Counts[Level].clear(); \
                Attributes_Counts[Level].resize(NAME##_Attribute_Max); \
            } \
            XML_ATTR_START \

    #define ELEMENT_START(NAME) \
        ELEMENT_START2(NAME,{}) \

    #define ELEMENT_MIDDLE(NAME) \
                else if (!Items[item_##NAME].Items.empty() && &NAME##_Content == &Items[item_##NAME].Items.back()) { \
                    Check_Attributes_NotPartOfSpecs(item_##NAME, Items[item_##NAME].Items.size() - 1, b, NAME##_Content); \
                } \
            XML_ATTR_END \
            Check_Attributes_ShallBePresent(item_##NAME, Items[item_##NAME].Items.size() - 1, Attributes_Counts[Level], NAME##_Attributes, NAME##_Content); \
            if (item_Infos[item_##NAME].Element_Infos) { \
                XML_ELEM_START \

    #define ELEMENT_END(NAME) \
                    else if (!Items[item_##NAME].Items.empty() && &NAME##_Content == &Items[item_##NAME].Items.back()) { \
                        Check_Elements_NotPartOfSpecs(item_##NAME, Items[item_##NAME].Items.size() - 1, b, NAME##_Content); \
                    } \
                XML_ELEM_END \
                if (Level == Level_Max) { \
                    Check_Elements_ShallBePresent(item_##NAME, Items[item_##NAME].Items.size() - 1, NAME##_Content.Elements, *item_Infos[item_##NAME].Element_Infos, NAME##_Content); \
                } \
            } \
            else { \
                if (!strcmp(#NAME, "position")) /* // TODO: remove this hack */ { \
                    XML_VALUE \
                    Items[item_audioBlockFormat].Items.back().Elements[audioBlockFormat_position].push_back(tfsxml_decode(b)); \
                } \
            } \
        } \

    #define ATTRIBUTE(NAME,ATTR) \
        else if (!tfsxml_strcmp_charp(b, #ATTR)) { \
            NAME##_Content.Attributes[NAME##_##ATTR].assign(tfsxml_decode(v)); \
            Attributes_Counts[Level][NAME##_##ATTR]++; \
        } \

    #define ATTRIB_ID(NAME,ATTR) \
        else if (!tfsxml_strcmp_charp(b, #ATTR)) { \
            NAME##_Content.Attributes[NAME##_##ATTR].assign(tfsxml_decode(v)); \
            Attributes_Counts[Level][NAME##_##ATTR]++; \
            CheckErrors_ID(this, NAME##_Content.Attributes[NAME##_##ATTR], item_Infos[item_##NAME], &this->Items[item_##NAME].Items, #NAME":"#ATTR); \
        } \

    #define ATTRIBU_I(NAME,ATTR) \
        else if (!tfsxml_strcmp_charp(b, #ATTR)) { \
            Attributes_Counts[Level][NAME##_##ATTR]++; \
        } \

    #define ELEMENT(NAME,ELEM) \
        else if (!tfsxml_strcmp_charp(b, #ELEM)) { \
            XML_VALUE \
            NAME##_Content.Elements[NAME##_##ELEM].push_back(tfsxml_decode(b)); \
        } \

    #define ELEME_I(NAME,ELEM) \
        else if (!tfsxml_strcmp_charp(b, #ELEM)) { \
        } \

    if (auto Result = Init(Buffer, Buffer_Size)) {
        return Result;
    }

    XML_BEGIN
    XML_ELEM_START
        if (!tfsxml_strcmp_charp(b, "audioFormatExtended"))
        {
            XML_SUB(audioFormatExtended())
        }
        if (!tfsxml_strcmp_charp(b, "ebuCoreMain"))
        {
            XML_ATTR_START
                if ((!tfsxml_strcmp_charp(b, "xmlns") || !tfsxml_strcmp_charp(b, "xsi:schemaLocation"))) {
                    for (size_t i = 0; i < schema_String_Size; i++) {
                        if (tfsxml_strstr_charp(v, schema_String[i]).len) {
                            Schema = (schema)(i + 1);
                        }
                    }
                    if (Schema == Schema_Unknown) {
                        DolbyProfileCanNotBeVersion1 = true;
                    }
                }
            XML_ATTR_END
            XML_ELEM_START
                    if (!tfsxml_strcmp_charp(b, "coreMetadata"))
                    {
                        XML_SUB(coreMetadata());
                    }
            XML_ELEM_END
        }
        if (!tfsxml_strcmp_charp(b, "frame"))
        {
            if (IsInit()) {
                Version_S = 0;
            }
            XML_ATTR_START
                if (!tfsxml_strcmp_charp(b, "version")) {
                    if (!tfsxml_strcmp_charp(v, "ITU-R_BS.2125-1")) {
                        Version_S = 1;
                    }
                    if (!tfsxml_strcmp_charp(v, "ITU-R_BS.2125-2")) {
                        Version_S = 2;
                    }
                }
            XML_ATTR_END
            XML_SUB(format());
        }
        if (!tfsxml_strcmp_charp(b, "format"))
        {
            XML_SUB(format());
        }
    XML_ELEM_END
    XML_END
}


void file_adm_private::clear()
{
    for (auto& Item : Items)
        Item = Items_Struct();
    profileInfos.clear();
}

int file_adm_private::coreMetadata()
{
    XML_BEGIN
    XML_ELEM_START
        if (!tfsxml_strcmp_charp(b, "format"))
        {
            XML_SUB(format());
        }
    XML_ELEM_END
    XML_END
}

int file_adm_private::format()
{
    XML_BEGIN
    XML_ELEM_START
        if (!tfsxml_strcmp_charp(b, "audioFormatCustom")) {
            XML_ELEM_START
                if (!tfsxml_strcmp_charp(b, "audioFormatCustomSet")) {
                    XML_ELEM_START
                        if (!tfsxml_strcmp_charp(b, "admInformation")) {
                            if (IsInit()) {
                                profileInfos.clear();
                            }
                            XML_ELEM_START
                                if (!tfsxml_strcmp_charp(b, "profile")) {
                                    if (IsInit()) {
                                        profileInfos.resize(profileInfos.size() + 1);
                                    }
                                    profile_info& profileInfo = profileInfos[profileInfos.size() - 1];
                                    XML_ATTR_START
                                        for (size_t i = 0; i < profile_names_size; i++)
                                        {
                                            if (!tfsxml_strcmp_charp(b, profile_names[i])) {
                                                profileInfo.Strings[i] = tfsxml_decode(v);
                                                if (!i && profileInfo.Strings[0].size() >= 12 && !profileInfo.Strings[0].compare(profileInfo.Strings[0].size() - 12, 12, " ADM Profile"))
                                                    profileInfo.Strings[0].resize(profileInfo.Strings[0].size() - 12);
                                            }
                                        }
                                    XML_ATTR_END
                                }
                            XML_ELEM_END
                        }
                    XML_ELEM_END
                }
            XML_ELEM_END
        }
        if (!tfsxml_strcmp_charp(b, "audioFormatExtended")) {
            XML_SUB(audioFormatExtended());
        }
        if (!tfsxml_strcmp_charp(b, "frameHeader")) {
            XML_SUB(frameHeader());
        }
    XML_ELEM_END
    XML_END
}

int file_adm_private::audioFormatExtended()
{
    if (IsInit()) {
        for (size_t i = 0; i < item_Max; i++) {
            if (i == item_audioTrack) {
                continue;
            }
            auto& Item = Items[i];
            Item.Items.clear();
        }
    }

    XML_BEGIN
    XML_ATTR_START
        if (!tfsxml_strcmp_charp(b, "version")) {
            if (!tfsxml_strcmp_charp(v, "ITU-R_BS.2076-1")) {
                Version = 1;
            }
            if (!tfsxml_strcmp_charp(v, "ITU-R_BS.2076-2")) {
                Version = 2;
            }
        }
    XML_ATTR_END
    XML_ELEM_START
        ELEMENT_START(audioProgramme)
            ATTRIB_ID(audioProgramme, audioProgrammeID)
            ATTRIBUTE(audioProgramme, audioProgrammeName)
            ATTRIBUTE(audioProgramme, audioProgrammeLanguage)
            ATTRIBUTE(audioProgramme, start)
            ATTRIBUTE(audioProgramme, end)
            ATTRIBUTE(audioProgramme, typeLabel)
            ATTRIBUTE(audioProgramme, typeDefinition)
            ATTRIBUTE(audioProgramme, typeLink)
            ATTRIBUTE(audioProgramme, typeLanguage)
            ATTRIBUTE(audioProgramme, formatLabel)
            ATTRIBUTE(audioProgramme, formatDefinition)
            ATTRIBUTE(audioProgramme, formatLink)
            ATTRIBUTE(audioProgramme, formatLanguage)
            ATTRIBUTE(audioProgramme, maxDuckingDepth)
        ELEMENT_MIDDLE(audioProgramme)
            else if (!tfsxml_strcmp_charp(b, "audioProgrammeLabel")) {
                XML_ATTR_START
                    if (!tfsxml_strcmp_charp(b, "language")) {
                        SetSavedAttribute(0);
                    }
                XML_ATTR_END
                XML_VALUE
                string Value = tfsxml_decode(b);
                if (!Value.empty() && GetSavedAttribute(0) != NoSavedAttribute()) {
                    Value.insert(0, '(' + GetSavedAttribute(0)->second + ')');
                }
                audioProgramme_Content.Elements[audioProgramme_audioProgrammeLabel].push_back(Value);
            }
            ELEMENT(audioProgramme, audioContentIDRef)
            else if (!tfsxml_strcmp_charp(b, "loudnessMetadata")) {
                XML_ELEM_START
                        if (!tfsxml_strcmp_charp(b, "integratedLoudness")) {
                            XML_VALUE
                            audioProgramme_Content.Elements[audioProgramme_loudnessMetadata_integratedLoudness].push_back(tfsxml_decode(b));
                        }
                XML_ELEM_END
            }
            ELEMENT(audioProgramme, audioProgrammeReferenceScreen)
            else if (!tfsxml_strcmp_charp(b, "authoringInformation")) {
                XML_ELEM_START
                        if (!tfsxml_strcmp_charp(b, "referenceLayout")) {
                            XML_ELEM_START
                                    if (!tfsxml_strcmp_charp(b, "audioPackFormatIDRef")) {
                                        XML_VALUE
                                        audioProgramme_Content.Elements[audioProgramme_authoringInformation_referenceLayout_audioPackFormatIDRef].push_back(tfsxml_decode(b));
                                    }
                            XML_ELEM_END
                        }
                XML_ELEM_END
            }
            ELEMENT(audioProgramme, alternativeValueSetIDRef)
        ELEMENT_END(audioProgramme)
        ELEMENT_START(audioContent)
            ATTRIB_ID(audioContent, audioContentID)
            ATTRIBUTE(audioContent, audioContentName)
            ATTRIBUTE(audioContent, audioContentLanguage)
            ATTRIBUTE(audioContent, typeLabel)
        ELEMENT_MIDDLE(audioContent)
            ELEMENT(audioContent, audioObjectIDRef)
            else if (!tfsxml_strcmp_charp(b, "dialogue")) {
                XML_ATTR_START
                    if (!tfsxml_strcmp_charp(b, "nonDialogueContentKind")
                        || !tfsxml_strcmp_charp(b, "dialogueContentKind")
                        || !tfsxml_strcmp_charp(b, "mixedContentKind")) {
                        SetSavedAttribute(0);
                    }
                XML_ATTR_END
                XML_VALUE
                string Value;
                string Kind;
                auto KindP = GetSavedAttribute(0);
                if (KindP != NoSavedAttribute())
                    Kind = KindP->second;
                if (!tfsxml_strcmp_charp(b, "0")) {
                    if (Kind == "1") {
                        Value = "Music";
                    }
                    else if (Kind == "2") {
                        Value = "Effect";
                    }
                    else {
                        Value = "No Dialogue";
                        if (!Kind.empty() && Kind != "0") {
                            Value += " (" + Kind + ')';
                        }
                    }
                }
                else if (!tfsxml_strcmp_charp(b, "1")) {
                    if (Kind == "1") {
                        Value = "Music";
                    }
                    else if (Kind == "2") {
                        Value = "Effect";
                    }
                    else if (Kind == "3") {
                        Value = "Spoken Subtitle";
                    }
                    else if (Kind == "4") {
                        Value = "Visually Impaired";
                    }
                    else if (Kind == "5") {
                        Value = "Commentary";
                    }
                    else if (Kind == "6") {
                        Value = "Emergency";
                    }
                    else {
                        Value = "Dialogue";
                        if (!Kind.empty() && Kind != "0") {
                            Value += " (" + Kind + ')';
                        }
                    }
                }
                else if (!tfsxml_strcmp_charp(b, "2")) {
                    if (Kind == "1") {
                        Value = "Complete Main";
                    }
                    else if (Kind == "2") {
                        Value = "Mixed (Mixed)";
                    }
                    else if (Kind == "3") {
                        Value = "Hearing Impaired";
                    }
                    else {
                        Value = "Mixed";
                        if (!Kind.empty() && Kind != "0") {
                            Value += " (" + Kind + ')';
                        }
                    }
                }
                else {
                    Value = tfsxml_decode(b);
                    if (!Kind.empty()) {
                        Value += " (" + Kind + ')';
                    }
                }
                audioContent_Content.Elements[audioContent_dialogue].push_back(Value);
            }
            else if (!tfsxml_strcmp_charp(b, "audioContentLabel")) {
                XML_ATTR_START
                    if (!tfsxml_strcmp_charp(b, "language")) {
                        SetSavedAttribute(0);
                    }
                XML_ATTR_END
                XML_VALUE
                string Value = tfsxml_decode(b);
                if (!Value.empty() && GetSavedAttribute(0) != NoSavedAttribute()) {
                    Value.insert(0, '(' + GetSavedAttribute(0)->second + ')');
                }
                audioContent_Content.Elements[audioContent_audioContentLabel].push_back(Value);
            }
            else if (!tfsxml_strcmp_charp(b, "loudnessMetadata")) {
                XML_ELEM_START
                        if (!tfsxml_strcmp_charp(b, "integratedLoudness")) {
                            XML_VALUE
                            audioContent_Content.Elements[audioContent_loudnessMetadata_integratedLoudness].push_back(tfsxml_decode(b));
                        }
                XML_ELEM_END
            }
            ELEMENT(audioContent, dialogue)
        ELEMENT_END(audioContent)
        ELEMENT_START(audioObject)
            ATTRIB_ID(audioObject, audioObjectID)
            ATTRIBUTE(audioObject, audioObjectName)
            ATTRIBUTE(audioObject, duration)
            ATTRIBUTE(audioObject, start)
            ATTRIBUTE(audioObject, startTime)
            ATTRIBUTE(audioObject, duration)
            ATTRIBUTE(audioObject, dialogue)
            ATTRIBUTE(audioObject, importance)
            ATTRIBUTE(audioObject, interact)
            ATTRIBUTE(audioObject, disableDucking)
            ATTRIBUTE(audioObject, typeLabel)
        ELEMENT_MIDDLE(audioObject)
            ELEMENT(audioObject, audioPackFormatIDRef)
            ELEMENT(audioObject, audioObjectIDRef)
            ELEMENT(audioObject, audioObjectLabel)
            ELEMENT(audioObject, audioComplementaryObjectGroupLabel)
            ELEMENT(audioObject, audioComplementaryObjectIDRef)
            ELEMENT(audioObject, audioTrackUIDRef)
            ELEMENT(audioObject, audioObjectInteraction)
            ELEMENT(audioObject, gain)
            ELEMENT(audioObject, headLocked)
            ELEMENT(audioObject, positionOffset)
            ELEMENT(audioObject, mute)
            ELEMENT(audioObject, alternativeValueSet)
        ELEMENT_END(audioObject)
        ELEMENT_START(audioPackFormat)
            ATTRIB_ID(audioPackFormat, audioPackFormatID)
            ATTRIBUTE(audioPackFormat, audioPackFormatName)
            ATTRIBUTE(audioPackFormat, typeDefinition)
            ATTRIBUTE(audioPackFormat, typeLabel)
            ATTRIBUTE(audioPackFormat, typeLink)
            ATTRIBUTE(audioPackFormat, typeLanguage)
            ATTRIBUTE(audioPackFormat, importance)
        ELEMENT_MIDDLE(audioPackFormat)
            ELEMENT(audioPackFormat, audioChannelFormatIDRef)
            ELEMENT(audioPackFormat, audioPackFormatIDRef)
            ELEMENT(audioPackFormat, absoluteDistance)
            ELEMENT(audioPackFormat, encodePackFormatIDRef)
            ELEMENT(audioPackFormat, decodePackFormatIDRef)
            ELEMENT(audioPackFormat, inputPackFormatIDRef)
            ELEMENT(audioPackFormat, outputPackFormatIDRef)
            ELEMENT(audioPackFormat, normalization)
            ELEMENT(audioPackFormat, nfcRefDist)
            ELEMENT(audioPackFormat, screenRef)
        ELEMENT_END(audioPackFormat)
        ELEMENT_START(audioChannelFormat)
            ATTRIB_ID(audioChannelFormat, audioChannelFormatID)
            ATTRIBUTE(audioChannelFormat, audioChannelFormatName)
            ATTRIBUTE(audioChannelFormat, typeDefinition)
            ATTRIBUTE(audioChannelFormat, typeLabel)
        ELEMENT_MIDDLE(audioChannelFormat)
            ELEMENT_START2(audioBlockFormat, Items[item_audioChannelFormat].Items.back().Elements[audioChannelFormat_audioBlockFormat].push_back({}))
                ATTRIB_ID(audioBlockFormat, audioBlockFormatID)
                ATTRIBUTE(audioBlockFormat, rtime)
                ATTRIBUTE(audioBlockFormat, duration)
                ATTRIBUTE(audioBlockFormat, lstart)
                ATTRIBUTE(audioBlockFormat, lduration)
                ATTRIBUTE(audioBlockFormat, initializeBlock)
            ELEMENT_MIDDLE(audioBlockFormat)
                ELEMENT(audioBlockFormat, gain)
                ELEMENT(audioBlockFormat, importance)
                ELEMENT(audioBlockFormat, headLocked)
                ELEMENT(audioBlockFormat, headphoneVirtualise)
                //ELEMENT(audioBlockFormat, speakerLabel)
                else if (!tfsxml_strcmp_charp(b, "speakerLabel")) {
                    XML_VALUE
                    string SpeakerLabel = tfsxml_decode(b);
                    audioBlockFormat_Content.Elements[audioBlockFormat_speakerLabel].push_back(SpeakerLabel);
                    if (SpeakerLabel.rfind("RC_", 0) == 0) {
                        SpeakerLabel.erase(0, 3);
                    }
                    map<string, string>::iterator Extra_SpeakerLabel = audioChannelFormat_Content.Extra.find("ChannelLayout");
                    map<string, string>::iterator Extra_Type = audioChannelFormat_Content.Extra.find("Type");
                    if (Extra_SpeakerLabel == audioChannelFormat_Content.Extra.end() || Extra_SpeakerLabel->second == SpeakerLabel) {
                        audioChannelFormat_Content.Extra["ChannelLayout"] = SpeakerLabel;
                        audioChannelFormat_Content.Extra["Type"] = "Static";
                    }
                    else if (Extra_Type != audioChannelFormat_Content.Extra.end()) {
                        audioChannelFormat_Content.Extra.clear();
                        audioChannelFormat_Content.Extra["Type"] = "Dynamic";
                    }
                }
                //ELEMENT(audioBlockFormat, position)
                else ELEMENT_START(position)
                    ATTRIBUTE(position, coordinate)
                    ATTRIBUTE(position, bound)
                    ATTRIBUTE(position, screenEdgeLock)
                ELEMENT_MIDDLE(position)
                ELEMENT_END(position)
                ELEMENT(audioBlockFormat, outputChannelFormatIDRef)
                ELEMENT(audioBlockFormat, jumpPosition)
                ELEMENT(audioBlockFormat, matrix)
                ELEMENT(audioBlockFormat, coefficient)
                ELEMENT(audioBlockFormat, width)
                ELEMENT(audioBlockFormat, height)
                ELEMENT(audioBlockFormat, depth)
                ELEMENT(audioBlockFormat, cartesian)
                ELEMENT(audioBlockFormat, diffuse)
                ELEMENT(audioBlockFormat, channelLock)
                ELEMENT(audioBlockFormat, objectDivergence)
                ELEMENT(audioBlockFormat, jumpPosition)
                ELEMENT(audioBlockFormat, zoneExclusion)
                ELEMENT(audioBlockFormat, equation)
                ELEMENT(audioBlockFormat, order)
                ELEMENT(audioBlockFormat, degree)
                ELEMENT(audioBlockFormat, normalization)
                ELEMENT(audioBlockFormat, nfcRefDist)
                ELEMENT(audioBlockFormat, screenRef)
            ELEMENT_END(audioBlockFormat)
            ELEMENT(audioChannelFormat, frequency)
        ELEMENT_END(audioChannelFormat)
        ELEMENT_START(audioTrackUID)
            ATTRIB_ID(audioTrackUID, UID)
            ATTRIBUTE(audioTrackUID, sampleRate)
            ATTRIBUTE(audioTrackUID, bitDepth)
            ATTRIBUTE(audioTrackUID, typeLabel)
        ELEMENT_MIDDLE(audioTrackUID)
            ELEMENT(audioTrackUID, audioChannelFormatIDRef)
            ELEMENT(audioTrackUID, audioPackFormatIDRef)
            ELEMENT(audioTrackUID, audioTrackFormatIDRef)
        ELEMENT_END(audioTrackUID)
        ELEMENT_START(audioTrackFormat)
            ATTRIB_ID(audioTrackFormat, audioTrackFormatID)
            ATTRIBUTE(audioTrackFormat, audioTrackFormatName)
            ATTRIBUTE(audioTrackFormat, typeLabel)
            ATTRIBUTE(audioTrackFormat, typeDefinition)
            ATTRIBUTE(audioTrackFormat, formatLabel)
            ATTRIBUTE(audioTrackFormat, formatDefinition)
            ATTRIBUTE(audioTrackFormat, formatLink)
            ATTRIBUTE(audioTrackFormat, formatLanguage)
            ELEMENT_MIDDLE(audioTrackFormat)
            ELEMENT(audioTrackFormat, audioStreamFormatIDRef)
        ELEMENT_END(audioTrackFormat)
        ELEMENT_START(audioStreamFormat)
            ATTRIB_ID(audioStreamFormat, audioStreamFormatID)
            ATTRIBUTE(audioStreamFormat, audioStreamFormatName)
            ATTRIBUTE(audioStreamFormat, typeLabel)
            ATTRIBUTE(audioStreamFormat, typeDefinition)
            ATTRIBUTE(audioStreamFormat, formatLabel)
            ATTRIBUTE(audioStreamFormat, formatDefinition)
            ATTRIBUTE(audioStreamFormat, formatLink)
            ATTRIBUTE(audioStreamFormat, formatLanguage)
            ELEMENT_MIDDLE(audioStreamFormat)
            ELEMENT(audioStreamFormat, audioChannelFormatIDRef)
            ELEMENT(audioStreamFormat, audioPackFormatIDRef)
            ELEMENT(audioStreamFormat, audioTrackFormatIDRef)
        ELEMENT_END(audioStreamFormat)
    XML_ELEM_END
    XML_END
}

int file_adm_private::frameHeader()
{
    XML_BEGIN
    XML_ELEM_START
        if (!tfsxml_strcmp_charp(b, "frameFormat")) {
            XML_ATTR_START
                if (!tfsxml_strcmp_charp(b, "duration")) {
                    auto Duration = TimeCodeToFloat(tfsxml_decode(v));
                    if (FrameRate_Den < 5) // Handling of 1.001 frames rates
                    {
                        FrameRate_Sum += Duration;
                        FrameRate_Den++;
                    }
                    Duration = FrameRate_Sum / FrameRate_Den;
                    if (IsSub)
                        More["FrameRate"] = Ztring().From_Number(1 / Duration).To_UTF8();
                    else
                        More["Duration"] = Ztring().From_Number(Duration * 1000, 0).To_UTF8();
                }
                if (!tfsxml_strcmp_charp(b, "flowID")) {
                    More["FlowID"] = tfsxml_decode(v);
                }
                if (!tfsxml_strcmp_charp(b, "start") && More["TimeCode_FirstFrame"].empty()) {
                    More["TimeCode_FirstFrame"] = tfsxml_decode(v);
                }
                if (!tfsxml_strcmp_charp(b, "type")) {
                    More["Metadata_Format_Type"] = tfsxml_decode(v);
                }
            XML_ATTR_END
        }
        if (!tfsxml_strcmp_charp(b, "transportTrackFormat")) {
            XML_SUB(transportTrackFormat());
        }
    XML_ELEM_END
    XML_END
}

int file_adm_private::transportTrackFormat()
{
    if (IsInit()) {
        Items[item_audioTrack].Items.clear();
    }
    XML_BEGIN
    XML_ELEM_START
        ELEMENT_START(audioTrack)
            else if (!tfsxml_strcmp_charp(b, "trackID")) {
                SetSavedAttribute(0);
            }
            ATTRIBUTE(audioTrack, formatLabel)
            ATTRIBUTE(audioTrack, formatDefinition)
        ELEMENT_MIDDLE(audioTrack)
            else if (!tfsxml_strcmp_charp(b, "audioTrackUIDRef")) {
                XML_VALUE
                auto trackIDP = GetSavedAttribute(0, 1);
                if (trackIDP != NoSavedAttribute(1)) {
                    auto trackID = Ztring().From_UTF8(trackIDP->second.c_str()).To_int32u();
                    chna_Add(trackID, tfsxml_decode(b));
                }
            }
        ELEMENT_END(audioTrack)
    XML_ELEM_END
    XML_END
}

//---------------------------------------------------------------------------
static type GetType(file_adm_private* File_Adm_Private, const item item_Type, size_t i) {
    for (const auto& Label_Info : label_Infos) {
        if (Label_Info.item_Type == item_Type) {
            const auto& Item = File_Adm_Private->Items[item_Type].Items[i];

            const auto& typeLabel = Item.Attributes[Label_Info.Label_Pos];
            if (typeLabel.size() == 4 && IsHexaDigit(typeLabel, 0, 4)) {
                return (type)strtoul(typeLabel.c_str(), nullptr, 16);
            }

            const auto& typeDefinition = Item.Attributes[Label_Info.Definition_Pos];
            if (!typeDefinition.empty()) {
                for (size_t i = 0; i < Type_Max - 1; i++) {
                    const auto& List_Item = typeDefinition_List[i];
                    if (List_Item == typeDefinition) {
                        return (type)(i + 1);
                    }
                }
            }

            const auto& Item_ID = Item.Attributes[item_Infos[item_Type].ID_Pos];
            if (item_Infos[item_Type].ID_Flags[item_info::Flags_ID_YX] && !CheckErrors_ID(File_Adm_Private, Item_ID, item_Infos[item_Type])) {
                const auto Item_ID_yyyy = Item_ID.substr(strlen(item_Infos[item_Type].ID_Begin) + 1, 4);
                if (IsHexaDigit(Item_ID_yyyy, 0, 4)) {
                    return (type)strtoul(Item_ID_yyyy.c_str(), nullptr, 16);
                }
            }
        }
    }

    if (item_Type == item_audioObject) {
        for (const auto& ID : File_Adm_Private->Items[item_Type].Items[i].Elements[audioObject_audioPackFormatIDRef]) {
            for (size_t j = 0; j < File_Adm_Private->Items[item_audioPackFormat].Items.size(); j++) {
                const auto& Target = File_Adm_Private->Items[item_audioPackFormat].Items[j];
                if (ID == Target.Attributes[audioPackFormat_audioPackFormatID]) {
                    return GetType(File_Adm_Private, item_audioPackFormat, j);
                }
            }
        }
    }

    return Type_Unknown;
};

//---------------------------------------------------------------------------
static void FillErrors(file_adm_private* File_Adm_Private, const item item_Type, size_t i, const char* Name, vector<string>* Errors_Field, vector<string>* Errors_Value, bool WarningError, size_t& Error_Count_Per_Type)
{
    for (size_t k = 0; k < error_Type_Max; k++) {
        auto& Item = File_Adm_Private->Items[item_Type];
        if (!Item.Items[i].Errors[k].empty()) {
            for (size_t j = 0; j < Item.Items[i].Errors[k].size(); j++) {
                string Field = Name;
                string Value = Item.Items[i].Errors[k][j];
                if (!Value.empty() && Value[0] == '!') {
                    auto End = Value.find('!', 1);
                    if (End != string::npos)
                    {
                        Field.insert(0, Value.substr(1, End - 1) + ' ');
                        Value.erase(0, End + 1);
                    }
                }
                if (!Value.empty() && Value[0] == ':') {
                    Field.clear();
                    auto Space = Value.find(' ');
                    auto End = Value.rfind(':', Space);
                    if (End != string::npos)
                    {
                        if (!Field.empty())
                            Field += ' ';
                        Field += Value.substr(1, End - 1);
                        Value.erase(0, End + 1);
                        for (;;)
                        {
                            auto Next = Field.find(':');
                            if (Next == string::npos)
                                break;
                            Field[Next] = ' ';
                        }
                    }
                }
                auto ErrorType = WarningError ? Error : k;
                if (ErrorType == Error) {
                    Error_Count_Per_Type++;
                }
                if (Error_Count_Per_Type < 9)
                {
                    Errors_Field[ErrorType].push_back(Field);
                    Errors_Value[ErrorType].push_back(Value);
                }
                else if (Error_Count_Per_Type == 9)
                {
                    Errors_Field[ErrorType].push_back(Field);
                    Errors_Value[ErrorType].push_back("[...]");
                }
            }
        }
    }
}

//---------------------------------------------------------------------------
static bool CheckErrors_ID(file_adm_private* File_Adm_Private, const string& ID, const item_info& ID_Start, vector<Item_Struct>* Items, const char* Sub)
{
    auto BeginSize = strlen(ID_Start.ID_Begin);
    const auto Flags = ID_Start.ID_Flags;
    auto MiddleSize = (Flags[item_info::Flags_ID_YX] || Flags[item_info::Flags_ID_N]) ? 8 : (Flags[item_info::Flags_ID_W] ? 4 : 0);
    static const int end_size []  = {0, 2, 4, 8};
    auto EndSize = end_size[Flags[item_info::Flags_ID_Z1] + Flags[item_info::Flags_ID_Z2] * 2];
    if (!MiddleSize && !EndSize && ID.size() > BeginSize) {
        EndSize = ID.size() - BeginSize - 1;
    }
    auto TotalMiddleSize = MiddleSize ? (1 + MiddleSize) : 0;
    auto TotalEndSize = EndSize ? (1 + EndSize) : 0;
    if (ID.size() == BeginSize + TotalMiddleSize + TotalEndSize
        && !ID.compare(0, BeginSize, ID_Start.ID_Begin, 0, BeginSize)
        && ID[BeginSize] == '_'
        && IsHexaDigit(ID, BeginSize + 1, MiddleSize)
        && (!EndSize || ID[BeginSize + TotalMiddleSize] == '_')
        && IsHexaDigit(ID, BeginSize + TotalMiddleSize + 1, EndSize)
        ) {
        return false;
    }
    if (Items) {
        string Middle;
        if (Flags[item_info::Flags_ID_YX]) {
            Middle.append(4, 'y');
            Middle.append(4, 'x');
        }
        if (Flags[item_info::Flags_ID_W]) {
            Middle.append(4, 'w');
        }
        if (Flags[item_info::Flags_ID_N]) {
            Middle.append(8, 'n');
        }
        string End;
        if (EndSize) {
            End.append(EndSize, 'z');
        }
        string Message;
        if (Sub) {
            Message += ':';
            Message += Sub;
            auto Index_Pos = Message.find(':', 1);
            if (Index_Pos != string::npos) {
                Message.insert(Index_Pos, to_string(Items->size() - 1));
            }
            Message += ':';
        }
        Message += '"';
        Message += ID;
        Message += '"';
        Message += " is not a valid form (";
        if (BeginSize) {
            Message.append(ID_Start.ID_Begin, BeginSize);
        }
        Message += '_';
        if (!Middle.empty()) {
            Message += Middle;
        }
        if (!Middle.empty() && !End.empty()) {
            Message += '_';
        }
        if (!End.empty()) {
            Message += End;
        }
        Message += " form, ";
        if (!Middle.empty()) {
            Message += Middle;
        }
        if (!Middle.empty() && !End.empty()) {
            Message += " and ";
        }
        if (!End.empty()) {
            Message += End;
        }
        Message += " being hexadecimal digits)";
        Items->back().Errors->push_back(Message);
    }
    return true;
}

//---------------------------------------------------------------------------
static void CheckErrors_ID_Additions(file_adm_private* File_Adm_Private, item item_Type, size_t i, bool IsAtmos) {
    auto& Item = File_Adm_Private->Items[item_Type].Items[i];
    const auto& ID = Item.Attributes[item_Infos[item_Type].ID_Pos];
    if (IsAtmos && !CheckErrors_ID(File_Adm_Private, ID, item_Infos[item_Type])) {
        const auto Flags = item_Infos[item_Type].ID_Flags;
        if (Flags[item_info::Flags_ID_YX]) {
            auto xxxx = ID.substr(strlen(item_Infos[item_Type].ID_Begin) + 5, 4);
            if (xxxx[0] == '0' || xxxx == "1000") {
                Item.AddError(Error, ":audio" + string(item_Infos[item_Type].Name) + to_string(i) + ":audio" + string(item_Infos[item_Type].Name) + "ID:audio" + string(item_Infos[item_Type].Name) + "ID attribute xxxx value \"" + xxxx + "\" is not permitted, permitted values are \"1001\" to \"FFFF\"" + ADM_Dolby_1_0);
            }
        }
        if (Flags[item_info::Flags_ID_W]) {
            auto wwww = ID.substr(strlen(item_Infos[item_Type].ID_Begin) + 1, 4);
            auto wwww_int = strtoul(wwww.c_str(), nullptr, 16);
            const char* min;
            if (item_Type == item_audioObject && GetType(File_Adm_Private, item_Type, i) == Type_Objects) {
                min = "100b";
            }
            else {
                min = "1001";
            }
            const char* max;
            switch (item_Type)
            {
            case item_audioProgramme: max = "1001"; break;
            case item_audioObject: max = "1080"; break;
            default: max = "FFFF"; break;
            }
            auto min_int = strtoul(min, nullptr, 16);
            auto max_int = strtoul(max, nullptr, 16);
            if ((!i || i - 1 < max_int - min_int) && (wwww_int <= 0x1000 || wwww_int < min_int || wwww_int > max_int)) {
                Item.AddError(Error, ":audio" + string(item_Infos[item_Type].Name) + to_string(i) + ":audio" + string(item_Infos[item_Type].Name) + "ID:audio" + string(item_Infos[item_Type].Name) + "ID attribute wwww value \"" + wwww + "\" is not permitted" + (min_int == 0x100b ? " due to the Objects typed object" : "") + ", permitted values are \"" + min + "\" to \"" + max + "\"" + ADM_Dolby_1_0);
            }
        }
        if (Flags[item_info::Flags_ID_Z1] && !Flags[item_info::Flags_ID_Z2]) {
            if (ID[12] != '0' || ID[13] != '1') { // zz != 01
                Item.AddError(Error, ":audio" + string(item_Infos[item_Type].Name) + to_string(i) + ":audio" + string(item_Infos[item_Type].Name) + "ID:audio" + string(item_Infos[item_Type].Name) + "ID attribute zz value \"" + ID.substr(12, 2) + "\" is not permitted, permitted value is \"01\"" + ADM_Dolby_1_0);
            }
        }
        if (Flags[item_info::Flags_ID_N]) {
            auto nnnnnnnn = ID.substr(strlen(item_Infos[item_Type].ID_Begin) + 1, 8);
            if (nnnnnnnn == "00000000") {
                Item.AddError(Error, ":audio" + string(item_Infos[item_Type].Name) + to_string(i) + ":" + (item_Type == item_audioTrackUID ? "U" : ("audio" + string(item_Infos[item_Type].Name))) + "ID:" + (item_Type == item_audioTrackUID ? "U" : ("audio" + string(item_Infos[item_Type].Name))) + "ID attribute nnnnnnnn value \"" + nnnnnnnn + "\" is not permitted, permitted values are \"00000001\" to \"FFFFFFFF\"" + ADM_Dolby_1_0);
            }
        }
    }
};

//---------------------------------------------------------------------------
static void CheckErrors_formatLabelDefinition(file_adm_private* File_Adm_Private, item item_Type, size_t i, const label_info& label_Info, bool IsAtmos) {
    auto& Item = File_Adm_Private->Items[item_Type].Items[i];
    const auto Label_Present = Item.Attributes_Present[label_Info.Label_Pos];
    const auto Definition_Present = Item.Attributes_Present[label_Info.Definition_Pos];
    const auto& Label = Item.Attributes[label_Info.Label_Pos];
    const auto& Definition = Item.Attributes[label_Info.Definition_Pos];
    const auto Style = label_Info.Label_Style;
    const auto& Style_Info = Style_Infos[Style];
    const auto List = (Style == (style)-1) ? nullptr : Style_Infos[Style].formatDefinition_List;
    const unsigned long List_Size = (Style == (style)-1) ? 0 : Style_Infos[Style].formatDefinition_Size;
    unsigned long formatLabel_Int = (unsigned long)-1;

    if (Label_Present) {
        if (Label.size() != 4 || !IsHexaDigit(Label, 0, 4)) {
            Item.AddError(Error, ":audio" + string(item_Infos[item_Type].Name) + to_string(i) + ":formatLabel:formatLabel attribute value \"" + Label + "\" is not a valid form (yyyy form, yyyy being hexadecimal digits)");
        }
        else {
            formatLabel_Int = strtoul(Label.c_str(), nullptr, 16);
            if (formatLabel_Int && formatLabel_Int > List_Size) {
                Item.AddError(Warning, ":audio" + string(item_Infos[item_Type].Name) + to_string(i) + ":formatLabel:formatLabel attribute value \"" + Label + "\" is not a known value");
            }
            else if (Definition_Present && List && formatLabel_Int < List_Size && Definition != List[formatLabel_Int - 1]) {
                Item.AddError(Error, ":audio" + string(item_Infos[item_Type].Name) + to_string(i) + ":formatDefinition:formatDefinition attribute value \"" + Definition + "\" shall be \"" + List[formatLabel_Int - 1] + "\" in order to match the term corresponding to formatLabel attribute value \"" + Label + "\"");
            }
            if (IsAtmos && ((Style == Style_Format && formatLabel_Int != 1) || (Style == Style_Type && formatLabel_Int != 1 && formatLabel_Int != 3))) {
                Item.AddError(Error, ":audio" + string(item_Infos[item_Type].Name) + to_string(i) + ":formatLabel:formatLabel attribute value \"" + Label + "\" is not permitted, permitted value is \"0001\"" + ADM_Dolby_1_0);
            }
        }
    }

    if (Definition_Present) {
        bool IsKnown = false;
        for (size_t j = 0; j < List_Size; j++) {
            const auto& List_Item = List[j];
            if (List_Item == Definition) {
                j++;
                if (formatLabel_Int == (unsigned long)-1) {
                    formatLabel_Int = j;
                }
                else if (formatLabel_Int != j) {
                    Item.AddError(Error, ":audio" + string(item_Infos[item_Type].Name) + to_string(i) + ":formatLabel:formatLabel attribute value \"" + Label + "\" shall be \"" + Hex2String(j, 4) + "\" in order to match the term corresponding to formatDefinition attribute value \"" + Definition + "\"");
                }
                IsKnown = true;
                break;
            }
        }
        if (!IsKnown) {
            Item.AddError(Warning, ":audio" + string(item_Infos[item_Type].Name) + to_string(i) + ":formatDefinition:formatDefinition attribute value \"" + Definition + "\" is not a known value");
        }
        if (IsAtmos && ((Style == Style_Format && Definition != List[0]) || (Style == Style_Type && Definition != List[0] && Definition != List[2]))) {
            Item.AddError(Error, ":audio" + string(item_Infos[item_Type].Name) + to_string(i) + ":formatDefinition:formatDefinition attribute value \"" + Definition + "\" is not permitted, permitted value is \"PCM\"" + ADM_Dolby_1_0);
        }
    }

    if (label_Info.Label_Style == Style_Type && formatLabel_Int != (unsigned long)-1) {
        const auto& Item_ID = Item.Attributes[item_Infos[item_Type].ID_Pos];
        if (item_Infos[item_Type].ID_Flags[item_info::Flags_ID_YX] && !CheckErrors_ID(File_Adm_Private, Item_ID, item_Infos[item_Type])) {
            const auto Item_ID_yyyy = Item_ID.substr(strlen(item_Infos[item_Type].ID_Begin) + 1, 4);
            if ((Label_Present && Item_ID_yyyy != Label) || (Definition_Present && formatLabel_Int != strtoul(Item_ID_yyyy.c_str(), nullptr, 16))) {
                Item.AddError(Error, ":audio" + string(item_Infos[item_Type].Name) + to_string(i) + ":audio" + string(item_Infos[item_Type].Name) + "ID:audio" + string(item_Infos[item_Type].Name) + "ID attribute yyyy value \"" + Item_ID_yyyy + "\" does not match " + (Label_Present ? ("formatLabel \"" + Label) : ("formatDefinition \"" + Definition)) + '\"');
            }
        }
    }
};

//---------------------------------------------------------------------------
static void  CheckErrors_Attribute(file_adm_private* File_Adm_Private, item item_Type, size_t i, size_t Attribute_Pos, bool IsAtmos) {
    auto& Item = File_Adm_Private->Items[item_Type].Items[i];
    const auto& Attribute = Item.Attributes[Attribute_Pos];
    const auto Attribute_Infos = item_Infos[item_Type].Attribute_Infos;
    if (Attribute_Infos) {
        const auto& Info = (*Attribute_Infos)[Attribute_Pos];
        if (IsAtmos) {
            if (Info.Flags[Count0] && !Info.Flags[Dolby0] && Attribute.empty()) {
                Item.AddError(Error, ":audio" + string(item_Infos[item_Type].Name) + to_string(i) + ':' + string(Info.Name) + ':' + string(Info.Name) + " attribute is not present" + ADM_Dolby_1_0);
            }
            if (!Info.Flags[Dolby1] && !Attribute.empty() && (/*File_Adm_Private->Schema != Schema_ebuCore_2014 ||*/ strcmp(Info.Name, "typeLabel") && strcmp(Info.Name, "typeDefinition"))) {
                Item.AddError(Error, ":audio" + string(item_Infos[item_Type].Name) + to_string(i) + ':' + string(Info.Name) + ':' + string(Info.Name) + " attribute is present" + ADM_Dolby_1_0);
            }
        }
        if (Attribute.size() > 64) {
            auto Attribute_Unicode = Ztring().From_UTF8(Attribute).To_Unicode();
            if (Attribute_Unicode.size() > 64) {
                Item.AddError(Warning, ":audio" + string(item_Infos[item_Type].Name) + to_string(i) + ':' + string(Info.Name) + ':' + string(Info.Name) + " attribute value \"" + Attribute + "\" is long");
            }
        }
        else if (Attribute.empty()) {
            const auto& Attribute_Present = Item.Attributes_Present[Attribute_Pos];
            if (Attribute_Present) {
                Item.AddError(Warning, ":audio" + string(item_Infos[item_Type].Name) + to_string(i) + ':' + string(Info.Name) + ':' + string(Info.Name) + " attribute is present but empty");
            }
        }
    }
}

//---------------------------------------------------------------------------
static void  CheckErrors_Element(file_adm_private* File_Adm_Private, item item_Type, size_t i, size_t Element_Pos, bool IsAtmos) {
    auto& Item = File_Adm_Private->Items[item_Type].Items[i];
    const auto& Element = Item.Elements[Element_Pos];
    if (IsAtmos) {
        const auto Element_Infos = item_Infos[item_Type].Element_Infos;
        if (Element_Infos) {
            const auto& Info = (*Element_Infos)[Element_Pos];
            if (Info.Flags[Count0] && !Info.Flags[Dolby0] && Element.empty()) {
                Item.AddError(Error, ":audio" + string(item_Infos[item_Type].Name) + to_string(i) + ':' + string(Info.Name) + ':' + string(Info.Name) + " element is not present" + ADM_Dolby_1_0);
            }
            if (!Info.Flags[Dolby1] && !Element.empty()) {
                Item.AddError(Error, ":audio" + string(item_Infos[item_Type].Name) + to_string(i) + ':' + string(Info.Name) + ':' + string(Info.Name) + " element is present" + ADM_Dolby_1_0);
            }
            else if (Info.Flags[Count2] && !Info.Flags[Dolby2] && Element.size() > 1) {
                Item.AddError(Error, ":audio" + string(item_Infos[item_Type].Name) + to_string(i) + ':' + string(Info.Name) + ':' + string(Info.Name) + " subelement count " + to_string(Element.size()) + " is not permitted, max is 1" + ADM_Dolby_1_0);
            }
        }
    }
};

//---------------------------------------------------------------------------
static void  CheckErrors_Element_Target(file_adm_private* File_Adm_Private, item item_Type, size_t i, size_t Element_Pos, size_t Target_Type, bool IsAtmos) {
    auto& Item = File_Adm_Private->Items[item_Type].Items[i];
    const auto& Element = Item.Elements[Element_Pos];
    const auto& Targets = File_Adm_Private->Items[Target_Type].Items;
    for (const auto& TargetIDRef : Element) {
        if ((item_Type == item_audioTrackFormat && Target_Type == item_audioStreamFormat)
         || (item_Type == item_audioStreamFormat && Target_Type == item_audioChannelFormat)) {
            const auto& Item_ID = Item.Attributes[item_Infos[item_Type].ID_Pos];
            if (!CheckErrors_ID(File_Adm_Private, Item_ID, item_Infos[item_Type]) && !CheckErrors_ID(File_Adm_Private, TargetIDRef, item_Infos[Target_Type])) {
                const auto Item_ID_yyyyxxxx = Item_ID.substr(3, 8);
                const auto TargetIDRef_yyyyxxxx = TargetIDRef.substr(3, 8);
                if (Item_ID_yyyyxxxx != TargetIDRef_yyyyxxxx) {
                    Item.AddError(IsAtmos ? Error : Warning, ":audio" + string(item_Infos[item_Type].Name) + to_string(i) + ":audio" + string(item_Infos[Target_Type].Name) + "IDRef:audio" + string(item_Infos[Target_Type].Name) + "IDRef subelement with yyyyxxxx value \"" + TargetIDRef_yyyyxxxx + "\" not same as audio" + string(item_Infos[item_Type].Name) + "ID attribute yyyyxxxx value \"" + Item_ID_yyyyxxxx + "\"" + (IsAtmos ? ADM_Dolby_1_0 : ""));
                }
            }
        }

        int16u Ref_Int = (int16u)-1;
        if (item_Infos[Target_Type].ID_Flags[item_info::Flags_ID_YX]) {
            const auto TargetIDRef_xxxx = TargetIDRef.substr(strlen(item_Infos[Target_Type].ID_Begin) + 5, 4);
            Ref_Int = strtoul(TargetIDRef_xxxx.c_str(), nullptr, 16);
        }
        if (item_Infos[Target_Type].ID_Flags[item_info::Flags_ID_W]) {
            const auto TargetIDRef_wwww = TargetIDRef.substr(strlen(item_Infos[Target_Type].ID_Begin) + 1, 4);
            Ref_Int = strtoul(TargetIDRef_wwww.c_str(), nullptr, 16);
        }
        if (Ref_Int < 0x1000) {
            if (Ref_Int > item_Infos[Target_Type].MaxKnown) {
                Item.AddError(Warning, ":audio" + string(item_Infos[item_Type].Name) + to_string(i) + ":audio" + string(item_Infos[Target_Type].Name) + "IDRef:audio" + string(item_Infos[Target_Type].Name) + "IDRef value \"" + TargetIDRef + "\" is not a known value");
            }
        }
        else {
            bool Target_Found = false;
            for (const auto& Target : Targets) {
                if (Target.Attributes[item_Infos[Target_Type].ID_Pos] == TargetIDRef) {
                    Target_Found = true;
                }
            }
            if (!Target_Found) {
                Item.AddError(Error, ":audio" + string(item_Infos[item_Type].Name) + to_string(i) + ":audio" + string(item_Infos[Target_Type].Name) + "IDRef:audio" + string(item_Infos[Target_Type].Name) + "IDRef value \"" + TargetIDRef + "\" shall match the audio" + string(item_Infos[Target_Type].Name) + "ID attribute of an audio" + string(item_Infos[Target_Type].Name) + " element");
            }
        }
    }
};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Adm::File_Adm()
:File__Analyze()
{
    //Configuration
    Buffer_MaximumSize = 256 * 1024 * 1024;

    File_Adm_Private = new file_adm_private();
}

//---------------------------------------------------------------------------
File_Adm::~File_Adm()
{
    delete File_Adm_Private;
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
void File_Adm::Streams_Fill()
{
    // Clean up
    if (!File_Adm_Private->Items[item_audioTrack].Items.empty())
    {
        if (File_Adm_Private->Items[item_audioTrack].Items[0].Elements[audioTrack_audioTrackUIDRef].empty())
            File_Adm_Private->Items[item_audioTrack].Items.erase(File_Adm_Private->Items[item_audioTrack].Items.begin());
    }

    #define FILL_COUNT(NAME) \
        if (!File_Adm_Private->Items[item_##NAME].Items.empty()) \
            Fill(Stream_Audio, 0, ("NumberOf" + string(item_Infos[item_##NAME].Name) + 's').c_str(), File_Adm_Private->Items[item_##NAME].Items.size());

    #define FILL_START(NAME,ATTRIBUTE) \
        for (size_t i = 0; i < File_Adm_Private->Items[item_##NAME].Items.size(); i++) { \
            Ztring Summary = Ztring().From_UTF8(File_Adm_Private->Items[item_##NAME].Items[i].Attributes[NAME##_##ATTRIBUTE]); \
            string P = Apply_Init(*this, item_Infos[item_##NAME].Name, i, File_Adm_Private->Items[item_##NAME], Summary); \

    #define FILL_START_SUB(NAME,ATTRIBUTE,FIELD) \
        for (size_t i = 0; i < File_Adm_Private->Items[item_##NAME].Items.size(); i++) { \
            Ztring Summary = Ztring().From_UTF8(File_Adm_Private->Items[item_##NAME].Items[i].Attributes[NAME##_##ATTRIBUTE]); \
            string P = Apply_Init(*this, FIELD, i, File_Adm_Private->Items[item_##NAME], Summary); \

#define FILL_A(NAME,ATTRIBUTE,FIELD) \
        Fill(Stream_Audio, StreamPos_Last, (P + ' ' + FIELD).c_str(), File_Adm_Private->Items[item_##NAME].Items[i].Attributes[NAME##_##ATTRIBUTE].c_str(), Unlimited, true, true); \

    #define FILL_E(NAME,ATTRIBUTE,FIELD) \
        { \
        ZtringList List; \
        List.Separator_Set(0, " + "); \
        for (size_t j = 0; j < File_Adm_Private->Items[item_##NAME].Items[i].Elements[NAME##_##ATTRIBUTE].size(); j++) { \
            List.push_back(Ztring().From_UTF8(File_Adm_Private->Items[item_##NAME].Items[i].Elements[NAME##_##ATTRIBUTE][j].c_str())); \
        } \
        string FieldName = P; \
        if (FIELD[0]) \
        { \
            FieldName += ' '; \
            FieldName += FIELD; \
        } \
        Fill(Stream_Audio, StreamPos_Last, FieldName.c_str(), List.Read(), true); \
        } \

    #define LINK(NAME,FIELD,VECTOR,TARGET) \
        Apply_SubStreams(*this, P + " LinkedTo_" FIELD "_Pos", File_Adm_Private->Items[item_##NAME].Items[i], NAME##_##VECTOR, File_Adm_Private->Items[item_##TARGET]); \

    //Filling
    Stream_Prepare(Stream_Audio);
    if (!IsSub)
        Fill(Stream_Audio, StreamPos_Last, Audio_Format, "ADM");

    if (File_Adm_Private->Version_S >= 0)
        Fill(Stream_Audio, StreamPos_Last, "Metadata_Format", "S-ADM, Version " + Ztring::ToZtring(File_Adm_Private->Version_S).To_UTF8());
    Fill(Stream_Audio, StreamPos_Last, "Metadata_Format", "ADM, Version " + Ztring::ToZtring(File_Adm_Private->Version).To_UTF8() + File_Adm_Private->More["Metadata_Format"]);
    if (!MuxingMode.empty())
        Fill(Stream_Audio, StreamPos_Last, "Metadata_MuxingMode", MuxingMode);
    for (map<string, string>::iterator It = File_Adm_Private->More.begin(); It != File_Adm_Private->More.end(); ++It)
        //if (It->first != "Metadata_Format")
            Fill(Stream_Audio, StreamPos_Last, It->first.c_str(), It->second);
    if (File_Adm_Private->Items[item_audioProgramme].Items.size() == 1 && File_Adm_Private->Items[item_audioProgramme].Items[0].Attributes[audioProgramme_audioProgrammeName] == "Atmos_Master") {
        if (!File_Adm_Private->DolbyProfileCanNotBeVersion1 && File_Adm_Private->Version>1)
            File_Adm_Private->DolbyProfileCanNotBeVersion1=true;
        Fill(Stream_Audio, 0, "AdmProfile", (!File_Adm_Private->DolbyProfileCanNotBeVersion1)?"Dolby Atmos Master, Version 1":"Dolby Atmos Master");
        Fill(Stream_Audio, 0, "AdmProfile_Format", "Dolby Atmos Master");
        Fill_SetOptions(Stream_Audio, 0, "AdmProfile_Format", "N NTY");
        if (!File_Adm_Private->DolbyProfileCanNotBeVersion1)
        {
            Fill(Stream_Audio, 0, "AdmProfile_Version", "1");
            Fill_SetOptions(Stream_Audio, 0, "AdmProfile_Version", "N NTY");
        }
    }
    vector<profile_info>& profileInfos = File_Adm_Private->profileInfos;
    if (!profileInfos.empty())
    {
        // Find what is in common
        int PosCommon = profile_names_size;
        for (int i = 0; i < PosCommon; i++)
            for (size_t j = 1; j < profileInfos.size(); j++)
                if (profileInfos[j].Strings[i] != profileInfos[0].Strings[i])
                    PosCommon = i;

        Fill(Stream_Audio, 0, "AdmProfile", PosCommon ? profileInfos[0].profile_info_build(PosCommon) : string("Multiple"));
        if (profileInfos.size() > 1)
        {
            for (size_t i = 0; i < profileInfos.size(); i++)
            {
                Fill(Stream_Audio, 0, ("AdmProfile AdmProfile" + Ztring::ToZtring(i).To_UTF8()).c_str(), profileInfos[i].profile_info_build());
                for (size_t j = 0; j < profile_names_size; j++)
                {
                    Fill(Stream_Audio, 0, ("AdmProfile AdmProfile" + Ztring::ToZtring(i).To_UTF8() + ' ' + profile_names_InternalID[j]).c_str(), profileInfos[i].Strings[j]);
                    Fill_SetOptions(Stream_Audio, 0, ("AdmProfile AdmProfile" + Ztring::ToZtring(i).To_UTF8() + ' ' + profile_names_InternalID[j]).c_str(), "N NTY");
                }
            }
        }
        for (size_t j = 0; j < (PosCommon == 0 ? 1 : PosCommon); j++)
        {
            Fill(Stream_Audio, 0, (string("AdmProfile_") + profile_names_InternalID[j]).c_str(), j < PosCommon ? profileInfos[0].Strings[j] : "Multiple");
            Fill_SetOptions(Stream_Audio, 0, (string("AdmProfile_") + profile_names_InternalID[j]).c_str(), "N NTY");
        }
    }
    size_t TotalCount = 0;
    for (size_t i = 0; i < item_Max; i++) {
        if (i >= item_audioBlockFormat) {
            break;
        }
        TotalCount += File_Adm_Private->Items[i].Items.size();
    }
    bool Full = TotalCount < 0x1000 ? true : false;
    FILL_COUNT(audioProgramme);
    FILL_COUNT(audioContent);
    FILL_COUNT(audioObject);
    FILL_COUNT(audioPackFormat);
    FILL_COUNT(audioChannelFormat);
    if (Full)
    {
        FILL_COUNT(audioTrackUID);
        FILL_COUNT(audioTrackFormat);
        FILL_COUNT(audioStreamFormat);
    }
    #if MEDIAINFO_CONFORMANCE
    vector<string> Errors_Field[error_Type_Max];
    vector<string> Errors_Value[error_Type_Max];
    MediaInfo_Config::adm_profile Config_AdmProfile=MediaInfoLib::Config.AdmProfile();
    bool WarningError=MediaInfoLib::Config.WarningError();
    #endif

    // Common definitions
    for (size_t i = 0; i < File_Adm_Private->Items[item_audioPackFormat].Items.size(); i++) {
        const Item_Struct& Source = File_Adm_Private->Items[item_audioPackFormat].Items[i];
        for (size_t j = 0; j < Source.Elements[audioPackFormat_audioChannelFormatIDRef].size(); j++) {

        }
    }

    FILL_START(audioProgramme, audioProgrammeName)
        if (Full)
            FILL_A(audioProgramme, audioProgrammeID, "ID");
        FILL_A(audioProgramme, audioProgrammeName, "Title");
        FILL_E(audioProgramme, audioProgrammeLabel, "Label");
        FILL_A(audioProgramme, audioProgrammeLanguage, "Language");
        FILL_A(audioProgramme, start, "Start");
        FILL_A(audioProgramme, start, "Start/String");
        FILL_A(audioProgramme, start, "Start/TimeCode");
        FILL_A(audioProgramme, start, "Start/TimeCodeSubFrames");
        FILL_A(audioProgramme, start, "Start/TimeCodeSamples");
        FILL_A(audioProgramme, end, "End");
        FILL_A(audioProgramme, end, "End/String");
        FILL_A(audioProgramme, end, "End/TimeCode");
        FILL_A(audioProgramme, end, "End/TimeCodeSubFrames");
        FILL_A(audioProgramme, end, "End/TimeCodeSamples");
        FILL_E(audioProgramme, loudnessMetadata_integratedLoudness, "IntegratedLoudness");
        LINK(audioProgramme, "Content", audioContentIDRef, audioContent);
        LINK(audioProgramme, "PackFormat", authoringInformation_referenceLayout_audioPackFormatIDRef, audioPackFormat);
        const Ztring& Label = Retrieve_Const(StreamKind_Last, StreamPos_Last, (P + " Label").c_str());
        if (!Label.empty()) {
            Summary += __T(' ');
            Summary += __T('(');
            Summary += Label;
            Summary += __T(')');
            Fill(StreamKind_Last, StreamPos_Last, P.c_str(), Summary, true);
        }

        #if MEDIAINFO_CONFORMANCE
        //TODO: expand this proof of concept
        if (Config_AdmProfile.Ebu3392==1)
        {
            const string& audioProgrammeName = File_Adm_Private->Items[item_audioProgramme].Items[i].Attributes[audioProgramme_audioProgrammeName];
            if (Ztring().From_UTF8(audioProgrammeName).size() > 32)
            {
                Errors_Field[Error].push_back("audioProgramme");
                Errors_Value[Error].push_back("audioProgrammeName length is more than EBU Tech 3392 constraint");
            }
        }
        #endif // MEDIAINFO_CONFORMANCE
    }

    FILL_START(audioContent, audioContentName)
        if (Full)
            FILL_A(audioContent, audioContentID, "ID");
        FILL_A(audioContent, audioContentName, "Title");
        FILL_E(audioContent, audioContentLabel, "Label");
        FILL_A(audioContent, audioContentLanguage, "Language");
        FILL_E(audioContent, dialogue, "Mode");
        FILL_E(audioContent, loudnessMetadata_integratedLoudness, "IntegratedLoudness");
        LINK(audioContent, "Object", audioObjectIDRef, audioObject);
        const Ztring& Label = Retrieve_Const(StreamKind_Last, StreamPos_Last, (P + " Label").c_str());
        if (!Label.empty()) {
            Summary += __T(' ');
            Summary += __T('(');
            Summary += Label;
            Summary += __T(')');
            Fill(StreamKind_Last, StreamPos_Last, P.c_str(), Summary, true);
        }
    }

    FILL_START(audioObject, audioObjectName)
        if (Full)
            FILL_A(audioObject, audioObjectID, "ID");
        FILL_A(audioObject, audioObjectName, "Title");
        FILL_A(audioObject, startTime, "Start");
        FILL_A(audioObject, start, "Start");
        FILL_A(audioObject, duration, "Duration");
        LINK(audioObject, "PackFormat", audioPackFormatIDRef, audioPackFormat);
        LINK(audioObject, "Object", audioObjectIDRef, audioObject);
        LINK(audioObject, "ComplementaryObject", audioComplementaryObjectIDRef, audioObject);
        if (Full)
            LINK(audioObject, "TrackUID", audioTrackUIDRef, audioTrackUID);
    }

    FILL_START(audioPackFormat, audioPackFormatName)
        if (Full)
            FILL_A(audioPackFormat, audioPackFormatID, "ID");
        FILL_A(audioPackFormat, audioPackFormatName, "Title");
        FILL_A(audioPackFormat, typeDefinition, "TypeDefinition");
        const Item_Struct& Source = File_Adm_Private->Items[item_audioPackFormat].Items[i];
        const Items_Struct& Dest = File_Adm_Private->Items[item_audioChannelFormat];
        string SpeakerLabel;
        for (size_t j = 0; j < Source.Elements[audioPackFormat_audioChannelFormatIDRef].size(); j++) {
            const string& ID = Source.Elements[audioPackFormat_audioChannelFormatIDRef][j];
            for (size_t k = 0; k < Dest.Items.size(); k++) {
                if (Dest.Items[k].Attributes[audioChannelFormat_audioChannelFormatID] != ID) {
                    continue;
                }
                for (map<string, string>::const_iterator Extra = Dest.Items[k].Extra.begin(); Extra != Dest.Items[k].Extra.end(); ++Extra) {
                    if (Extra->first == "ChannelLayout") {
                        if (!SpeakerLabel.empty()) {
                            SpeakerLabel += ' ';
                        }
                        SpeakerLabel += Extra->second;
                    }
                }
            }
        }
        if (!SpeakerLabel.empty()) {
            Fill(StreamKind_Last, StreamPos_Last, (P + " ChannelLayout").c_str(), SpeakerLabel, true, true);
        }

        LINK(audioPackFormat, "ChannelFormat", audioChannelFormatIDRef, audioChannelFormat);
    }

    FILL_START(audioChannelFormat, audioChannelFormatName)
        if (Full)
            FILL_A(audioChannelFormat, audioChannelFormatID, "ID");
        FILL_A(audioChannelFormat, audioChannelFormatName, "Title");
        FILL_A(audioChannelFormat, typeDefinition, "TypeDefinition");
        for (map<string, string>::iterator Extra = File_Adm_Private->Items[item_audioChannelFormat].Items[i].Extra.begin(); Extra != File_Adm_Private->Items[item_audioChannelFormat].Items[i].Extra.end(); ++Extra) {
            Fill(Stream_Audio, StreamPos_Last, (P + ' ' + Extra->first).c_str(), Extra->second);
        }
    }

    if (Full) {
        FILL_START(audioTrackUID, UID)
            FILL_A(audioTrackUID, UID, "ID");
            FILL_A(audioTrackUID, bitDepth, "BitDepth");
            FILL_A(audioTrackUID, sampleRate, "SamplingRate");
            string& ID = File_Adm_Private->Items[item_audioTrackUID].Items[i].Attributes[audioTrackUID_UID];
            for (size_t j = 0; j < File_Adm_Private->Items[item_audioTrack].Items.size(); j++)
            {
                Item_Struct& Item = File_Adm_Private->Items[item_audioTrack].Items[j];
                vector<string>& Ref = Item.Elements[audioTrack_audioTrackUIDRef];
                for (size_t k = 0; k < Ref.size(); k++)
                {
                    if (Ref[k] == ID)
                    {
                        Fill(Stream_Audio, 0, (P + " TrackIndex/String").c_str(), j + 1);
                        Fill_SetOptions(Stream_Audio, 0, (P + " TrackIndex/String").c_str(), "Y NIN");
                    }
                }
            }
            LINK(audioTrackUID, "ChannelFormat", audioChannelFormatIDRef, audioChannelFormat);
            LINK(audioTrackUID, "PackFormat", audioPackFormatIDRef, audioPackFormat);
            LINK(audioTrackUID, "TrackFormat", audioTrackFormatIDRef, audioTrackFormat);
        }

        FILL_START(audioTrackFormat, audioTrackFormatName)
            FILL_A(audioTrackFormat, audioTrackFormatID, "ID");
            FILL_A(audioTrackFormat, audioTrackFormatName, "Title");
            FILL_A(audioTrackFormat, formatDefinition, "FormatDefinition");
            FILL_A(audioTrackFormat, typeDefinition, "TypeDefinition");
            LINK(audioTrackFormat, "StreamFormat", audioStreamFormatIDRef, audioStreamFormat);
        }

        FILL_START(audioStreamFormat, audioStreamFormatName)
            FILL_A(audioStreamFormat, audioStreamFormatID, "ID");
            FILL_A(audioStreamFormat, audioStreamFormatName, "Title");
            FILL_A(audioStreamFormat, formatDefinition, "Format");
            FILL_A(audioStreamFormat, typeDefinition, "TypeDefinition");
            LINK(audioStreamFormat, "ChannelFormat", audioChannelFormatIDRef, audioChannelFormat);
            LINK(audioStreamFormat, "PackFormat", audioPackFormatIDRef, audioPackFormat);
            LINK(audioStreamFormat, "TrackFormat", audioTrackFormatIDRef, audioTrackFormat);
        }
    }

    if (!File_Adm_Private->Items[item_audioTrack].Items.empty())
    {
        Fill(Stream_Audio, 0, "Transport0", "Yes");
        FILL_START_SUB(audioTrack, formatDefinition, "Transport0 TrackIndex")
            FILL_E(audioTrack, audioTrackUIDRef, "");
            if (Full)
                LINK(audioTrack, "TrackUID", audioTrackUIDRef, audioTrackUID);
        }
    }

    if (!Full)
        Fill(Stream_Audio, 0, "PartialDisplay", "Yes");

    // Errors
    #if MEDIAINFO_CONFORMANCE
    auto IsAtmos = !File_Adm_Private->Items[item_audioProgramme].Items.empty() && File_Adm_Private->Items[item_audioProgramme].Items[0].Attributes[audioProgramme_audioProgrammeName] == "Atmos_Master";
    auto& Programmes = File_Adm_Private->Items[item_audioProgramme].Items;
    auto& Contents = File_Adm_Private->Items[item_audioContent].Items;
    auto& Objects = File_Adm_Private->Items[item_audioObject].Items;
    auto& PackFormats = File_Adm_Private->Items[item_audioPackFormat].Items;
    auto& ChannelFormats = File_Adm_Private->Items[item_audioChannelFormat].Items;
    auto& StreamFormats = File_Adm_Private->Items[item_audioStreamFormat].Items;
    auto& TrackFormats = File_Adm_Private->Items[item_audioTrackFormat].Items;
    auto& TrackUIDs = File_Adm_Private->Items[item_audioTrackUID].Items;
    auto& BlockFormats = File_Adm_Private->Items[item_audioBlockFormat].Items;
    auto& Positions = File_Adm_Private->Items[item_position].Items;

    auto OldLocale_Temp = setlocale(LC_NUMERIC, nullptr);
    string OldLocale;
    if (*OldLocale_Temp != 'C' || *(OldLocale_Temp + 1)) {
        OldLocale = OldLocale_Temp;
        setlocale(LC_NUMERIC, "C");
    }

    // Errors - For all
    for (size_t item_Type = 0; item_Type < item_Max; item_Type++) {
        if (item_Type == item_audioTrack) {
            continue; // TODO
        }
        auto& Items = File_Adm_Private->Items[item_Type].Items;
        if (item_Type == item_audioProgramme && Items.empty()) {
            File_Adm_Private->Items[item_Type].New().AddError(Error, ":audioProgramme0:GeneralCompliance:audioProgramme element count 0 is not permitted, min is 1");
        }
        if (IsAtmos) {
            static const int8u MaxCounts[] = { (int8u)-1, 1, 123, 123, 123, 128, 128, 128, 128, (int8u)-1, (int8u)-1, (int8u)-1 };
            static_assert (sizeof(MaxCounts) / sizeof(MaxCounts[0]) == item_Max, IncoherencyMessage);
            if (int8u(MaxCounts[item_Type] + 1) && Items.size() > MaxCounts[item_Type]) {
                Items[MaxCounts[item_Type]].AddError(Error, ":audio" + string(item_Infos[item_Type].Name) + to_string(MaxCounts[item_Type]) + ":GeneralCompliance:audio" + string(item_Infos[item_Type].Name) + " element count " + to_string(Items.size()) + " is not permitted, max is " + to_string(MaxCounts[item_Type]) + "" + ADM_Dolby_1_0);
            }
        }
        set<string> PreviousIDs;
        set<string> PreviousIDs_Forbidden;
        for (size_t i = 0; i < Items.size(); i++) {
            auto& Item = Items[i];
            for (size_t j = 0; j < Item.Attributes.size(); j++) {
                CheckErrors_Attribute(File_Adm_Private, (item)item_Type, i, j, IsAtmos);
            }
            for (size_t j = 0; j < Item.Elements.size(); j++) {
                CheckErrors_Element(File_Adm_Private, (item)item_Type, i, j, IsAtmos);
            }
            if (item_Infos[item_Type].ID_Pos != (int8u)-1) {
                const auto& ID = Item.Attributes[item_Infos[item_Type].ID_Pos];
                if (PreviousIDs.find(ID) != PreviousIDs.end()) {
                    Item.AddError(Error, ":audio" + string(item_Infos[item_Type].Name) + to_string(i) + ":audio" + string(item_Infos[item_Type].Name) + "ID:audio" + string(item_Infos[item_Type].Name) + "ID value \"" + ID + "\" shall be unique");
                }
                else if (PreviousIDs_Forbidden.find(ID) != PreviousIDs_Forbidden.end()) {
                    Item.AddError(Error, ":audio" + string(item_Infos[item_Type].Name) + to_string(i) + ":audio" + string(item_Infos[item_Type].Name) + "ID:audio" + string(item_Infos[item_Type].Name) + "ID value \"" + ID + "\" is not permitted due to the ID of the DirectSpeakers typed object" + ADM_Dolby_1_0);
                }
                else {
                    PreviousIDs.insert(ID);
                    if (IsAtmos && item_Type == Type_Objects && !CheckErrors_ID(File_Adm_Private, ID, item_Infos[item_Type])) {
                        const auto audioTrackUIDRefs_Size = Item.Elements[audioObject_audioTrackUIDRef].size();
                        if (audioTrackUIDRefs_Size) {
                            const auto ID_wwww = ID.substr(3, 4);
                            auto ID_wwww_Int = strtoul(ID_wwww.c_str(), nullptr, 16);
                            auto ID_wwww_Int_Max = ID_wwww_Int + audioTrackUIDRefs_Size;
                            for (; ID_wwww_Int < ID_wwww_Int_Max; ID_wwww_Int++) {
                                PreviousIDs_Forbidden.insert("AO_" + Hex2String(ID_wwww_Int, 4));
                            }
                        }
                    }
                }
                CheckErrors_ID_Additions(File_Adm_Private, (item)item_Type, i, IsAtmos);
            }
        }
    }
    for (const auto& label_Info : label_Infos) {
        auto& Items = File_Adm_Private->Items[label_Info.item_Type].Items;
        for (size_t i = 0; i < Items.size(); i++) {
            CheckErrors_formatLabelDefinition(File_Adm_Private, label_Info.item_Type, i, label_Info, IsAtmos);
        }
    }
    for (const auto& IDRef : IDRefs) {
        auto& Items = File_Adm_Private->Items[IDRef.Source_Type].Items;
        for (size_t i = 0; i < Items.size(); i++) {
            CheckErrors_Element_Target(File_Adm_Private, IDRef.Source_Type, i, IDRef.Source_Pos, IDRef.Target_Type, IsAtmos);
        }
    }

    // Errors - audioProgramme
    for (size_t i = 0; i < Programmes.size(); i++) {
        auto& Programme = Programmes[i];
        if (IsAtmos && Programme.Elements[audioProgramme_audioContentIDRef].size() > 123) {
            Programme.AddError(Error, ":audioProgramme" + to_string(i) + ":audioContentIDRef:audioContentIDRef element count " + to_string(Programme.Elements[audioProgramme_audioContentIDRef].size()) + " is not permitted, max is 123" + ADM_Dolby_1_0);
        }
    }

    // Errors - audioContent
    for (size_t i = 0; i < Contents.size(); i++) {
        auto& Content = Contents[i];
        const auto& dialogues = Content.Elements[audioContent_dialogue];
        for (size_t j = 0; j < dialogues.size(); j++) {
            const auto& dialogue = dialogues[j];
            if (IsAtmos && dialogue != "Mixed") {
                Content.AddError(Error, ":audioObject" + to_string(i) + ":dialogue" + to_string(j) + ":dialogue element value \"" + dialogue + "\" is not permitted, permitted value is \"Mixed\"" + ADM_Dolby_1_0);
            }
        }
    }

    // Errors - audioObject
    size_t Object_Objects_Count = 0;
    for (size_t i = 0; i < Objects.size(); i++) {
        auto& Object = Objects[i];
        if (IsAtmos) {
            auto Type = GetType(File_Adm_Private, item_audioObject, i);
            const auto& ChannelFormatIDRefs = Object.Elements[audioObject_audioTrackUIDRef];
            if (!ChannelFormatIDRefs.empty()) {
                auto ChannelFormatIDRefs_Size = ChannelFormatIDRefs.size();
                if ((Type == Type_DirectSpeakers && ChannelFormatIDRefs_Size > 10) || (Type == Type_Objects && ChannelFormatIDRefs_Size > 1)) {
                    Object.AddError(Error, ":audioObject" + to_string(i) + ":audioTrackUIDRef:audioTrackUIDRef element count " + to_string(ChannelFormatIDRefs_Size) + " is not permitted, max is " + to_string(Type == Type_DirectSpeakers ? 10 : 1) + "" + ADM_Dolby_1_0);
                }
            }
        }
        const auto& start = Object.Attributes[audioObject_start];
        const auto& startTime = Object.Attributes[audioObject_startTime];
        const auto RealStart = start.empty() ? &startTime : &start;
        if (!start.empty() && !startTime.empty() && start != startTime) {
            Object.AddError(Warning, ":audioObject" + to_string(i) + ":start:startTime attribute should be same as start attribute");
        }
        if (IsAtmos) {
            if (RealStart->empty()) {
            }
            else if (*RealStart != "00:00:00.00000") {
                Object.AddError(Error, ":audioObject" + to_string(i) + ":start:start attribute value \"" + *RealStart + "\" is not permitted, permitted value is \"00:00:00.00000\"" + ADM_Dolby_1_0);
            }
            const auto& interact = Object.Attributes[audioObject_interact];
            if (!interact.empty() && interact != "0") {
                Object.AddError(Error, ":audioObject" + to_string(i) + ":interact:interact attribute value \"" + interact + "\" is not permitted, permitted value is \"0\"" + ADM_Dolby_1_0);
            }
            const auto& disableDucking = Object.Attributes[audioObject_disableDucking];
            if (!disableDucking.empty() && disableDucking != "1") {
                Object.AddError(Error, ":audioObject" + to_string(i) + ":disableDucking:disableDucking attribute value \"" + disableDucking + "\" is not permitted, permitted value is \"1\"" + ADM_Dolby_1_0);
            }
        }
        for (const auto& audioPackFormatIDRef : Object.Elements[audioObject_audioPackFormatIDRef]) {
            for (const auto& audioPackFormat : PackFormats) {
                if (audioPackFormat.Attributes[audioChannelFormat_audioChannelFormatID] == audioPackFormatIDRef && (audioPackFormat.Attributes[audioChannelFormat_typeLabel] == "0003" || audioPackFormat.Attributes[audioChannelFormat_typeDefinition] == "Objects")) {
                    Object_Objects_Count++;
                }
            }
        }
    }
    if (Object_Objects_Count > 118) {
        Objects[118].AddError(Error, ":audioObject118:GeneralCompliance:audioObject pointing to audioChannelFormat@typeDefinition==\"Objects\" element count " + to_string(Object_Objects_Count) + " is not permitted, max is 118" + ADM_Dolby_1_0);
    }

    // Errors - audioPackFormat
    size_t PackFormat_Objects_Count = 0;
    for (size_t i = 0; i < PackFormats.size(); i++) {
        auto& PackFormat = PackFormats[i];
        auto Type = GetType(File_Adm_Private, item_audioPackFormat, i);
        if (IsAtmos) {
            const auto& ChannelFormatIDRefs = PackFormat.Elements[audioPackFormat_audioChannelFormatIDRef];
            if (!ChannelFormatIDRefs.empty()) {
                auto ChannelFormatIDRefs_Size = ChannelFormatIDRefs.size();
                if ((Type == Type_DirectSpeakers && ChannelFormatIDRefs_Size > 10) || (Type == Type_Objects && ChannelFormatIDRefs_Size > 1)) {
                    PackFormat.AddError(Error, ":audioPackFormat" + to_string(i) + ":audioChannelFormatIDRef:audioChannelFormatIDRef subelement count " + to_string(ChannelFormatIDRefs_Size) + " is not permitted, max is " + to_string(Type == Type_DirectSpeakers ? 10 : 1) + "" + ADM_Dolby_1_0);
                }
                if (Type == Type_DirectSpeakers) {
                    vector<atmos_audioChannelFormatName> List;
                    vector<string> List_Strings;
                    for (const auto& ChannelFormatIDRef : ChannelFormatIDRefs) {
                        for (const auto& ChannelFormat : ChannelFormats) {
                            const auto& ChannelFormatID = ChannelFormat.Attributes[audioChannelFormat_audioChannelFormatID];
                            if (ChannelFormatID == ChannelFormatIDRef) {
                                const auto& ChannelFormatName = ChannelFormat.Attributes[audioChannelFormat_audioChannelFormatName];
                                List.push_back(Atmos_audioChannelFormat_Pos(ChannelFormatName));
                                List_Strings.push_back(ChannelFormatName);
                            }
                        }
                    }
                    if (!Atmos_ChannelOrder_Find(List)) {
                        string List_Strings_Flat;
                        for (const auto& Item : List_Strings) {
                            if (!List_Strings_Flat.empty()) {
                                List_Strings_Flat += ' ';
                            }
                            List_Strings_Flat += Item;
                        }
                        PackFormat.AddError(Error, ":audioPackFormat" + to_string(i) + ":audioChannelFormatIDRef:audioChannelFormatIDRef order \"" + List_Strings_Flat + "\" is not permitted" + ADM_Dolby_1_0);
                    }
                }
            }
        }
        if (Type == Type_Objects) {
            PackFormat_Objects_Count++;
        }
    }
    if (PackFormat_Objects_Count > 118) {
        PackFormats[118].AddError(Error, ":audioPackFormat118:GeneralCompliance:audioPackFormat@typeDefinition==\"Objects\" element count " + to_string(PackFormat_Objects_Count) + " is not permitted, max is 118" + ADM_Dolby_1_0);
    }

    // Errors - audioChannelFormat
    size_t BlockFormat_Pos = 0;
    size_t Position_Pos = 0;
    for (size_t i = 0; i < ChannelFormats.size(); i++) {
        auto& ChannelFormat = ChannelFormats[i];
        size_t BlockFormat_Count = ChannelFormat.Elements[audioChannelFormat_audioBlockFormat].size();
        const auto& ID = ChannelFormat.Attributes[audioChannelFormat_audioChannelFormatID];
        const auto Type = GetType(File_Adm_Private, item_audioChannelFormat, i);

        atmos_audioChannelFormatName ChannelAssignment;
        if (IsAtmos) {
            if (Type == Type_DirectSpeakers) {
                if (ChannelFormat.Attributes_Present[audioChannelFormat_audioChannelFormatName]) {
                    const auto& ChannelFormatName = ChannelFormat.Attributes[audioChannelFormat_audioChannelFormatName];
                    ChannelAssignment = Atmos_audioChannelFormat_Pos(ChannelFormatName);
                    if (ChannelAssignment == (size_t)-1) {
                        ChannelFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioChannelFormatName:audioChannelFormatName " + ChannelFormatName + " is not permitted" + ADM_Dolby_1_0);
                    }
                }
                const auto& BlockFormats_Size = ChannelFormat.Elements[audioChannelFormat_audioBlockFormat].size();
                if (BlockFormats_Size != 1) {
                    ChannelFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat1:GeneralCompliance:audioBlockFormat subelement count " + to_string(BlockFormats_Size) + " is not permitted, max is 1" + ADM_Dolby_1_0);
                }
            }
        }

        if (!CheckErrors_ID(File_Adm_Private, ID, item_Infos[item_audioChannelFormat])) {
            bool HasNoInit;
            const auto ID_yyyyxxxx = ID.substr(3, 8);
            for (size_t j = 0; j < BlockFormat_Count; j++) {
                auto& BlockFormat = BlockFormats[BlockFormat_Pos + j];

                if (IsAtmos) {
                    if (Type == Type_DirectSpeakers) {
                        if (BlockFormat.Attributes_Present[audioBlockFormat_rtime]) {
                            BlockFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ":rtime:rtime attribute is present" + ADM_Dolby_1_0);
                        }
                        if (BlockFormat.Attributes_Present[audioBlockFormat_duration]) {
                            BlockFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ":duration:duration attribute is present" + ADM_Dolby_1_0);
                        }
                    }
                    if (Type == Type_Objects) {
                        if (!BlockFormat.Attributes_Present[audioBlockFormat_rtime]) {
                            BlockFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ":rtime:rtime attribute is not present" + ADM_Dolby_1_0);
                        }
                        if (!BlockFormat.Attributes_Present[audioBlockFormat_duration]) {
                            BlockFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ":duration:duration attribute is not present" + ADM_Dolby_1_0);
                        }
                    }
                }

                if (!j)
                    HasNoInit = File_Adm_Private->Version_S < 0 || BlockFormat.Attributes[audioBlockFormat_initializeBlock] != "1";
                const auto& ID_Block = BlockFormat.Attributes[audioBlockFormat_audioBlockFormatID];
                if (!CheckErrors_ID(File_Adm_Private, ID_Block, item_Infos[item_audioBlockFormat])) {
                    const auto ID_Block_yyyyxxxx = ID_Block.substr(3, 8);
                    if (ID_Block_yyyyxxxx != ID_yyyyxxxx) {
                        ChannelFormat.AddError(IsAtmos ? Error : Warning, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ":audioBlockFormatID:audioBlockFormatID attribute with yyyyxxxx value \"" + ID_Block_yyyyxxxx + "\" not same as audioChannelFormatID attribute yyyyxxxx value \"" + ID_yyyyxxxx + "\"" + (IsAtmos ? ADM_Dolby_1_0 : ""));
                    }
                    const auto ID_Block_zzzzzzzz = ID_Block.substr(12, 8);
                    auto ID_Block_zzzzzzzz_Int = strtoul(ID_Block_zzzzzzzz.c_str(), nullptr, 16);
                    const auto ID_Block_zzzzzzzz_Expected = j + HasNoInit;
                    if ((HasNoInit || !j) && ID_Block_zzzzzzzz_Int != ID_Block_zzzzzzzz_Expected) {
                        const auto ID_Block_Expected = ID_Block.substr(0, 12) + Hex2String(ID_Block_zzzzzzzz_Expected, 8);
                        ChannelFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ":audioBlockFormatID:audioBlockFormatID attribute value \"" + ID_Block + "\" shall be \"" + ID_Block_Expected + "\" in order to match the audioBlockFormat index");
                    }
                }

                auto List_Check = [&](audioBlockFormat_Element* List, size_t List_Size) {
                    for (size_t l = audioBlockFormat_headphoneVirtualise + 1; l < audioBlockFormat_Element_Max; l++) {
                        if (BlockFormat.Elements[l].empty()) {
                            continue;
                        }
                        auto IsInList = false;
                        for (size_t m = 0; m < List_Size; m++) {
                            if (l == List[m]) {
                                IsInList = true;
                            }
                        }
                        if (!IsInList) {
                            BlockFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ":GeneralConformance:" + (*item_Infos[item_audioBlockFormat].Element_Infos)[l].Name + " subelement is present");
                        }
                    }
                };
                atmos_audioChannelFormatName speakerLabel_ChannelAssignment = (atmos_audioChannelFormatName)-1;
                switch (Type) {
                    case Type_DirectSpeakers: {
                        const audioBlockFormat_Element BlockFormat_DirectSpeakers_List[] = { audioBlockFormat_cartesian, audioBlockFormat_speakerLabel, audioBlockFormat_position }; // TODO: cartesian is not in specs but lot of files have it
                        List_Check((audioBlockFormat_Element*)&BlockFormat_DirectSpeakers_List, sizeof(BlockFormat_DirectSpeakers_List) / sizeof(*BlockFormat_DirectSpeakers_List));
                        if (BlockFormat.Elements[audioBlockFormat_speakerLabel].empty()) {
                            BlockFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ':' + (*item_Infos[item_audioBlockFormat].Element_Infos)[audioBlockFormat_speakerLabel].Name + ':' + (*item_Infos[item_audioBlockFormat].Element_Infos)[audioBlockFormat_speakerLabel].Name + " element is not present");
                        }
                        else {
                            if (IsAtmos) {
                                const auto& speakerLabel = BlockFormat.Elements[audioBlockFormat_speakerLabel].back();
                                speakerLabel_ChannelAssignment = Atmos_audioChannelFormat_Pos(speakerLabel, true);
                                if (speakerLabel_ChannelAssignment == (size_t)-1) {
                                    ChannelFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ":speakerLabel:speakerLabel element value " + speakerLabel + " is not permitted" + ADM_Dolby_1_0);
                                }
                                else {
                                    if (ChannelAssignment != (size_t)-1 && speakerLabel_ChannelAssignment != ChannelAssignment) {
                                        ChannelFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ":speakerLabel:speakerLabel element value " + speakerLabel + " is not permitted, permitted value is " + Atmos_audioChannelFormat_Content[ChannelAssignment].SpeakerLabel + "" + ADM_Dolby_1_0);
                                    }
                                }
                            }
                        }
                        break;
                    }
                }

                unsigned long is_cartesian;
                static const char* Cartesian_0_Names[] = { "azimuth", "elevation", "distance" };
                const auto& cartesians = BlockFormat.Elements[audioBlockFormat_cartesian];
                size_t position_Count = BlockFormat.Elements[audioBlockFormat_position].size();
                if (!cartesians.empty()) {
                    const auto& cartesian = BlockFormat.Elements[audioBlockFormat_cartesian].back();
                    char* End;
                    is_cartesian = strtoul(cartesian.c_str(), &End, 10);
                    if (to_string(is_cartesian) != cartesian) {
                        BlockFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ":cartesian:cartesian element value \"" + cartesian + "\" is malformed");
                        is_cartesian = (unsigned long)-1;
                    }
                    else if (is_cartesian > 1) {
                        BlockFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ":cartesian:cartesian element value \"" + cartesian + "\" is not permitted, permitted values are 0 or 1");
                    }
                    else {
                        if (IsAtmos && !is_cartesian) {
                            BlockFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ":cartesian:cartesian element value is not 1" + ADM_Dolby_1_0);
                        }
                    }
                }
                else {
                    if (IsAtmos && Type == Type_Objects) {
                        BlockFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ":cartesian:cartesian element is not present" + ADM_Dolby_1_0);
                    }

                    // Autodetection
                    bitset<3> HasAED;
                    bitset<3> HasXYZ;
                    for (size_t k = 0; k < position_Count; k++) {
                        auto& Position = Positions[Position_Pos + k];
                        if (Position.Attributes_Present[position_coordinate]) {
                            if (Position.Attributes[position_coordinate].size() == 1 && Position.Attributes[position_coordinate][0] >= 'X' && Position.Attributes[position_coordinate][0] <= 'Z') {
                                auto Pos = Position.Attributes[position_coordinate][0] - 'X';
                                HasXYZ.set(Pos);
                            }
                            size_t Pos = 0;
                            for (; Pos < 3 && Position.Attributes[position_coordinate] != Cartesian_0_Names[Pos]; Pos++) {
                            }
                            if (Pos < 3) {
                                HasAED.set(Pos);
                            }
                        }
                    }
                    if (HasXYZ.count() > HasAED.count()) {
                        BlockFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ":cartesian:cartesian element is not present" + ADM_Dolby_1_0);

                        is_cartesian = 1;
                    }
                    else if (HasAED.any()) {
                        is_cartesian = 0;
                    }
                    else {
                        is_cartesian = (unsigned long)-1;
                    }
                }

                if (is_cartesian == 0) {
                    bitset<3> HasAED;
                    static float32 Cartesian_0_Limits[][2] = { {-180, 180}, { -90, 90}, {0, 1 } };
                    for (size_t k = 0; k < position_Count; k++) {
                        auto& Position = Positions[Position_Pos + k];
                        if (Position.Attributes_Present[position_coordinate]) {
                            size_t Pos = 0;
                            for (; Pos < 3 && Position.Attributes[position_coordinate] != Cartesian_0_Names[Pos]; Pos++) {
                            }
                            if (Pos < 3) {
                                HasAED.set(Pos);
                                const auto& Element = BlockFormat.Elements[audioBlockFormat_position][k];
                                char* End;
                                auto Value = strtof(Element.c_str(), &End);
                                if (End - Element.c_str() != Element.size()) {
                                    Position.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ":position"/*0*/":position element value \"" + BlockFormat.Elements[audioBlockFormat_position][k] + "\" is malformed");
                                }
                                else if (Value < Cartesian_0_Limits[Pos][0] || Value > Cartesian_0_Limits[Pos][1]) {
                                    Position.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ":position"/*0*/":position element value \"" + BlockFormat.Elements[audioBlockFormat_position][k] + "\" is not permitted, permitted values are [" + to_string(Cartesian_0_Limits[Pos][0]) + " - " + to_string(Cartesian_0_Limits[Pos][1]) + "]");
                                }
                            }
                            else {
                                Position.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ":position"/*0*/":coordinate attribute \"" + Position.Attributes[position_coordinate] + "\" is present");
                            }
                        }
                    }
                    for (size_t k = 0; k < 2; k++) {
                        if (!HasAED[k]) {
                            BlockFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ":position"/*0*/":position@coordinate==\"" + Cartesian_0_Names[k] + "\" element is not present");
                        }
                    }
                }
                if (is_cartesian == 1) {
                    bitset<3> HasXYZ;
                    float32 Values[3] = {};
                    for (size_t k = 0; k < position_Count; k++) {
                        auto& Position = Positions[Position_Pos + k];
                        if (Position.Attributes_Present[position_coordinate]) {
                            if (Position.Attributes[position_coordinate].size() == 1 && Position.Attributes[position_coordinate][0] >= 'X' && Position.Attributes[position_coordinate][0] <= 'Z') {
                                auto Pos = Position.Attributes[position_coordinate][0] - 'X';
                                HasXYZ.set(Pos);
                                const auto& Element = BlockFormat.Elements[audioBlockFormat_position][k];
                                char* End;
                                Values[Pos] = strtof(Element.c_str(), &End);
                                if (End - Element.c_str() != Element.size()) {
                                    BlockFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ':' + (*item_Infos[item_audioBlockFormat].Element_Infos)[k].Name + ':' + (*item_Infos[item_audioBlockFormat].Element_Infos)[k].Name + " element value \"" + Element + "\" is malformed");
                                }
                                else if (Values[Pos] < -1 || Values[Pos] > 1) {
                                    Position.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ":position"/*0*/":position@coordinate=\"" + Position.Attributes[position_coordinate] + "\" element value \"" + BlockFormat.Elements[audioBlockFormat_position][k] + "\" is not permitted, permitted values are [-1,1]");
                                }
                            }
                            else {
                                Position.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ":position"/*0*/":coordinate attribute \"" + Position.Attributes[position_coordinate] + "\" is present");
                            }
                        }
                    }
                    bool ValuesAreNok = false;
                    for (size_t l = 0; l < 2; l++) {
                        if (!HasXYZ[l]) {
                            ValuesAreNok = true;
                            BlockFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ":position"/*0*/":position@coordinate==\"" + string(1, 'X' + l) + "\" element is not present");
                        }
                    }
                    if (!ValuesAreNok && speakerLabel_ChannelAssignment != -1) {
                        if (IsAtmos) {
                            auto position_ChannelAssignment = Atmos_audioChannelFormat_Pos(Values[0], Values[1], Values[2], speakerLabel_ChannelAssignment);
                            if (position_ChannelAssignment == -1) {
                                BlockFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ":position:position@coordinate=\"X Y Z\" element value \"" + Ztring::ToZtring(Values[0], (Values[0] - (int)Values[0]) ? 5 : 0).To_UTF8() + ' ' + Ztring::ToZtring(Values[1], (Values[2] - (int)Values[2]) ? 5 : 0).To_UTF8() + ' ' + Ztring::ToZtring(Values[2], (Values[2] - (int)Values[2]) ? 5 : 0).To_UTF8() + "\" is not valid" + ADM_Dolby_1_0);
                            }
                            else if (position_ChannelAssignment != speakerLabel_ChannelAssignment) {
                                BlockFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ":position:position@coordinate=\"X Y Z\" element value \"" + Ztring::ToZtring(Values[0], (Values[0] - (int)Values[0]) ? 5 : 0).To_UTF8() + ' ' + Ztring::ToZtring(Values[1], (Values[2] - (int)Values[2]) ? 5 : 0).To_UTF8() + ' ' + Ztring::ToZtring(Values[2], (Values[2] - (int)Values[2]) ? 5 : 0).To_UTF8() + "\" so \"" + Atmos_audioChannelFormat_Content[position_ChannelAssignment].SpeakerLabel + "\" does not match corresponding speakerLabel element value \"" + Atmos_audioChannelFormat_Content[speakerLabel_ChannelAssignment].SpeakerLabel + "\"" + ADM_Dolby_1_0);
                            }
                        }
                    }
                }
                if (is_cartesian || is_cartesian == 1) {
                    static_assert(audioBlockFormat_depth - audioBlockFormat_width == 2, "");
                    auto Count = 0;
                    string Value_Ref;
                    for (size_t k = audioBlockFormat_width; k <= audioBlockFormat_depth; k++) {
                        const auto& Elements = BlockFormat.Elements[k];
                        if (!Elements.empty()) {
                            const auto& Element = Elements.back();
                            if (Value_Ref.empty()) {
                                Value_Ref = Element;
                            }
                            else if (Element != Value_Ref) {
                                BlockFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ":GeneralCompliance:width/height/depth element values are not same" + ADM_Dolby_1_0);
                            }
                            auto Max = (is_cartesian || k == audioBlockFormat_depth) ? 1 : 360;
                            char* End;
                            auto Value = strtof(Element.c_str(), &End);
                            if (End - Element.c_str() < Element.size()) {
                                BlockFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ':' + (*item_Infos[item_audioBlockFormat].Element_Infos)[k].Name + ':' + (*item_Infos[item_audioBlockFormat].Element_Infos)[k].Name + " element value \"" + Element + "\" is malformed");
                            }
                            else if (Value < 0 || Value > Max) {
                                BlockFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ':' + (*item_Infos[item_audioBlockFormat].Element_Infos)[k].Name + ':' + (*item_Infos[item_audioBlockFormat].Element_Infos)[k].Name + " element value \"" + Element + "\" is not permitted, permitted values are [0," + to_string(Max) + "]");
                            }
                            Count++;
                        }
                    }
                    if (IsAtmos && (Count && Count != 3)) {
                        BlockFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ":GeneralCompliance:width/height/depth element values are not all present" + ADM_Dolby_1_0);
                    }
                }

                Position_Pos += position_Count;
            }
        }

        if (IsAtmos) {
            if (!ChannelFormat.Elements[audioChannelFormat_frequency].empty()) {
                ChannelFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":frequency:frequency element should not be present" + ADM_Dolby_1_0);
            }
        }

        BlockFormat_Pos += BlockFormat_Count;
    }

    // Errors - audioStreamFormat
    for (size_t i = 0; i < StreamFormats.size(); i++) {
        auto& StreamFormat = StreamFormats[i];
    }

    // Errors - audioTrackFormat
    for (size_t i = 0; i < TrackFormats.size(); i++) {
        auto& TrackFormat = TrackFormats[i];
    }

    // Errors - TrackUID
    for (size_t i = 0; i < TrackUIDs.size(); i++) {
        auto& TrackUID = TrackUIDs[i];
    }

    // Errors - Fill
    for (size_t t = 0; t < item_Max; t++) {
        size_t Error_Count_Per_Type = 0;
        for (size_t i = 0; i < File_Adm_Private->Items[t].Items.size(); i++) {
            FillErrors(File_Adm_Private, (item)t, i, item_Infos[t].Name, &Errors_Field[0], &Errors_Value[0], WarningError, Error_Count_Per_Type);
        }
    }

    if (!OldLocale.empty()) {
        setlocale(LC_NUMERIC, OldLocale.c_str());
    }

    //Conformance
    for (size_t k = 0; k < error_Type_Max; k++) {
        if (!Errors_Field[k].empty()) {
            auto FieldPrefix = "Conformance" + string(error_Type_String[k]);
            Fill(StreamKind_Last, StreamPos_Last, FieldPrefix.c_str(), Errors_Field[k].size());
            FieldPrefix += ' ';
            for (size_t i = 0; i < Errors_Field[k].size(); i++) {
                size_t Space = 0;
                for (;;)
                {
                    Space = Errors_Field[k][i].find(' ', Space + 1);
                    if (Space == string::npos) {
                        break;
                    }
                    const auto Field = FieldPrefix + Errors_Field[k][i].substr(0, Space);
                    const auto& Value = Retrieve_Const(StreamKind_Last, StreamPos_Last, Field.c_str());
                    if (Value.empty()) {
                        Fill(StreamKind_Last, StreamPos_Last, Field.c_str(), "Yes");
                    }
                }
                const auto Field = FieldPrefix + Errors_Field[k][i];
                const auto& Value = Retrieve_Const(StreamKind_Last, StreamPos_Last, Field.c_str());
                if (Value == __T("Yes")) {
                    Clear(StreamKind_Last, StreamPos_Last, Field.c_str());
                }
                Fill(StreamKind_Last, StreamPos_Last, Field.c_str(), Errors_Value[k][i]);
            }
        }
    }
   #endif // MEDIAINFO_CONFORMANCE

    Element_Offset=File_Size;
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Adm::FileHeader_Begin()
{
    // File must be fully loaded
    if (!IsSub && Buffer_Size < File_Size)
    {
        if (Buffer_Size && Buffer[0] != '<')
        {
            Reject();
            return false;
        }
    }

    return true;
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
void File_Adm::Read_Buffer_Init()
{
    File_Adm_Private->IsSub = IsSub;
}

//---------------------------------------------------------------------------
void File_Adm::Read_Buffer_Continue()
{
    auto Result = File_Adm_Private->parse((void*)Buffer, Buffer_Size);
    if (!Status[IsAccepted]) {
        for (const auto& Items : File_Adm_Private->Items) {
            if (!Items.Items.empty()) {
                Accept("ADM");
                break;
            }
        }
    }
    if (Result > 0 && File_Offset + Buffer_Size < File_Size)
    {
        Buffer_Offset = Buffer_Size - File_Adm_Private->Remain();
        Element_WaitForMoreData();
        return;
    }
}

//***************************************************************************
// In
//***************************************************************************

void File_Adm::chna_Add(int32u Index, const string& TrackUID)
{
    File_Adm_Private->chna_Add(Index, TrackUID);
}

void* File_Adm::chna_Move()
{
    if (!File_Adm_Private)
        return NULL;
    return &File_Adm_Private->Items[item_audioTrack];
}

void File_Adm::chna_Move(File_Adm* Adm)
{
    if (!Adm)
        return;
    if (!File_Adm_Private)
        File_Adm_Private = new file_adm_private();
    File_Adm_Private->Items[item_audioTrack] = Adm->File_Adm_Private->Items[item_audioTrack];
}

} //NameSpace

#endif //MEDIAINFO_ADM_YES

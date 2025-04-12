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
#include "MediaInfo/TimeCode.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include "ThirdParty/tfsxml/tfsxml.h"
#include <algorithm>
#include <bitset>
#include <cstdlib>
using namespace ZenLib;
using namespace std;
#if __cplusplus > 202002L || (defined(_MSC_VER) && _MSC_VER >= 1910 && _MSVC_LANG > 202002L)
    #define constexpr23 constexpr
#else
    #define constexpr23
#endif
#if __cplusplus >= 202002L || (defined(_MSC_VER) && _MSC_VER >= 1910 && _MSVC_LANG >= 202002L)
#else
    #define consteval constexpr
#endif
#if  __cplusplus >= 201703 || _MSC_VER >= 1900
#else
    template <typename T, size_t N>
    inline constexpr size_t size(const T(&)[N]) noexcept { return N; }
#endif
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

#define IncoherencyMessage "Incoherency between enums and message strings"

static const char* ADM_Atmos_1_0 = " (additional constraint from Dolby Atmos Master ADM Profile v1.0)";
static const char* ADM_Atmos_1_1 = " (additional constraint from Dolby Atmos Master ADM Profile v1.1)";
static const char* ADM_AdvSSE_1 = " (additional constraint from AdvSS Emission v1)";

static const uint8_t Version_Max = 3;

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

static char* strnstr(const char* Str, size_t Size, const char* ToSearch)
{
    size_t ToSearch_Size = strlen(ToSearch);
    if (ToSearch_Size == 0)
        return (char *)Str;

    if (ToSearch_Size > Size)
        return NULL;

    const char* LastPos = (const char *)Str + Size - ToSearch_Size;
    for (const char* Start = Str; Start <= LastPos; Start++)
        if (Start[0] == ToSearch[0] && !memcmp((const void*)&Start[1], (const void*)&ToSearch[1], ToSearch_Size - 1))
            return (char*)Start;

    return NULL;
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
        bool IsMpegH = Strings[0] == "MPEG-H";
        for (size_t i=0; i<Max; i++)
        {
            if (!Strings[i].empty())
            {
                if (!ToReturn.empty())
                {
                    if (i==1)
                        ToReturn+=", Version ";
                }
                if (i>=2 && IsMpegH)
                {
                    if (!HasParenthsis)
                    {
                        ToReturn+=' ';
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
                    if (IsMpegH)
                    {
                        ToReturn+=profile_names[i];
                        ToReturn+='=';
                    }
                    else
                    {
                        ToReturn+=',';
                        ToReturn+=' ';
                        ToReturn+=profile_names_InternalID[i];
                        ToReturn+=' ';
                    }
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
    Count0,
    Count1,
    Count2,
    Dolby0,
    Dolby1,
    Dolby2,
    AdvSSE0,
    AdvSSE1,
    AdvSSE2,
    Reserved9,
    Version_Max0,
    Version_Max1,
    Version_Max2,
    Version_Min0,
    Version_Min1,
    Version_Min3,
    flags_Max
};
class check_flags : public bitset<flags_Max> {
public:
    consteval check_flags(unsigned long val) : bitset(val) {}
    consteval check_flags(unsigned long min, unsigned long max, unsigned long val0, unsigned long val1, unsigned long val2, unsigned long val3, unsigned long val4, unsigned long val5, unsigned long val6, unsigned long val7, unsigned long val8)
    : check_flags((val0 << 0) | (val1 << 1) | (val2 << 2) | (val3 << 3) | (val4 << 4) | (val5 << 5) | (val6 << 6) | (val7 << 7) | (val8 << 8) | (min << Version_Min0) | (max << Version_Max0)) {}
};

struct attribute_item {
    const char*         Name;
    check_flags         Flags;
    unsigned long       Min() const {
        return (Flags.to_ulong() >> Version_Min0);
    }
    unsigned long       Max() const {
        return (Flags.to_ulong() >> Version_Max0) & 0x7;
    }
};
typedef attribute_item attribute_items[];

struct element_item {
    const char* const   Name;
    const check_flags   Flags;
    const uint8_t       LinkedItem;
    constexpr element_item(const char* const Name, const check_flags Flags, const uint8_t LinkedItem = 0)
        : Name(Name), Flags(Flags), LinkedItem(LinkedItem) {}
    constexpr23 unsigned long Min() const noexcept {
        return (Flags.to_ulong() >> Version_Min0);
    }
    constexpr23 unsigned long Max() const noexcept {
        return (Flags.to_ulong() >> Version_Max0) & 0x7;
    }
};
typedef element_item element_items[];

enum item {
    item_root,
    item_audioFormatExtended,
    item_audioProgramme,
    item_audioContent,
    item_audioObject,
    item_audioPackFormat,
    item_audioChannelFormat,
    item_audioTrackUID,
    item_audioTrackFormat,
    item_audioStreamFormat,
    item_profileList,
    item_tagList,
    item_frameHeader,
    item_frameFormat,
    item_transportTrackFormat,
    item_audioTrack,
    item_changedIDs,
    item_audioProgrammeLabel,
    item_loudnessMetadata,
    item_renderer,
    item_audioProgrammeReferenceScreen,
    item_screenCentrePosition,
    item_screenWidth,
    item_authoringInformation,
    item_referenceLayout,
    item_audioContentLabel,
    item_dialogue,
    item_audioObjectLabel,
    item_audioComplementaryObjectGroupLabel,
    item_audioObjectInteraction,
    item_gainInteractionRange,
    item_positionInteractionRange,
    item_audioBlockFormat,
    item_gain,
    item_headphoneVirtualise,
    item_position,
    item_positionOffset,
    item_channelLock,
    item_objectDivergence,
    item_jumpPosition,
    item_zoneExclusion,
    item_zone,
    item_matrix,
    item_coefficient,
    item_alternativeValueSet,
    item_frequency,
    item_profile,
    item_tagGroup,
    item_tag,
    item_Max
};

enum root_Attribute {
    root_Attribute_Max
};

enum root_Element {
    root_frameHeader,
    root_audioFormatExtended,
    root_Element_Max
};

enum audioFormatExtended_Attribute {
    audioFormatExtended_Attribute_Max
};

enum audioFormatExtended_Element {
    audioFormatExtended_audioProgramme,
    audioFormatExtended_audioContent,
    audioFormatExtended_audioObject,
    audioFormatExtended_audioPackFormat,
    audioFormatExtended_audioChannelFormat,
    audioFormatExtended_audioTrackUID,
    audioFormatExtended_audioTrackFormat,
    audioFormatExtended_audioStreamFormat,
    audioFormatExtended_profileList,
    audioFormatExtended_tagList,
    audioFormatExtended_Element_Max
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
    audioProgramme_audioProgrammeReferenceScreen,
    audioProgramme_authoringInformation,
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

enum profileList_Attribute {
    profileList_Attribute_Max
};

enum profileList_Element {
    profileList_profile,
    profileList_Element_Max
};

enum tagList_Attribute {
    tagList_Attribute_Max
};

enum tagList_Element {
    tagList_tagGroup,
    tagList_Element_Max
};

enum tag_Attribute {
    tag_class,
    tag_Attribute_Max
};

enum tagElement {
    tag_Element_Max
};

enum frameHeader_Attribute {
    frameHeader_Attribute_Max
};

enum frameHeader_Element {
    frameHeader_profileList,
    frameHeader_frameFormat,
    frameHeader_transportTrackFormat,
    frameHeader_Element_Max
};

enum frameFormat_Attribute {
    frameFormat_frameFormatID,
    frameFormat_start,
    frameFormat_duration,
    frameFormat_type,
    frameFormat_timeReference,
    frameFormat_flowID,
    frameFormat_countToFull,
    frameFormat_numMetadataChunks,
    frameFormat_countToSameChunk,
    frameFormat_Attribute_Max
};

enum frameFormat_Element {
    frameFormat_changedIDs,
    frameFormat_chunkAdmElement,
    frameFormat_Element_Max
};

enum transportTrackFormat_Attribute {
    transportTrackFormat_transportID,
    transportTrackFormat_transportName,
    transportTrackFormat_numTracks,
    transportTrackFormat_numIDs,
    transportTrackFormat_Attribute_Max
};

enum transportTrackFormat_Element {
    transportTrackFormat_audioTrack,
    transportTrackFormat_Element_Max
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

enum changedIDs_Attribute {
    changedIDs_Attribute_Max
};

enum changedIDs_Element {
    // TODO: changedIDs sub elements
    changedIDs_Element_Max
};

enum audioProgrammeLabel_Attribute {
    audioProgrammeLabel_language,
    audioProgrammeLabel_Attribute_Max
};

enum audioProgrammeLabel_Element {
    audioProgrammeLabel_Element_Max
};

enum loudnessMetadata_Attribute {
    loudnessMetadata_loudnessMethod,
    loudnessMetadata_loudnessRecType,
    loudnessMetadata_loudnessCorrectionType,
    loudnessMetadata_Attribute_Max
};

enum loudnessMetadata_Element {
    loudnessMetadata_integratedLoudness,
    loudnessMetadata_loudnessRange,
    loudnessMetadata_maxTruePeak,
    loudnessMetadata_maxMomentary,
    loudnessMetadata_maxShortTerm,
    loudnessMetadata_dialogueLoudness,
    loudnessMetadata_renderer,
    loudnessMetadata_Element_Max
};

enum renderer_Attribute {
    renderer_uri,
    renderer_name,
    renderer_version,
    renderer_coordinateMode,
    renderer_Attribute_Max
};

enum renderer_Element {
    renderer_audioPackFormatIDRef,
    renderer_audioObjectIDRef,
    renderer_Element_Max
};

enum audioProgrammeReferenceScreen_Attribute {
    audioProgrammeReferenceScreen_aspectRatio,
    audioProgrammeReferenceScreen_Attribute_Max
};

enum audioProgrammeReferenceScreen_Element {
    audioProgrammeReferenceScreen_screenCentrePosition,
    audioProgrammeReferenceScreen_screenWidth,
    audioProgrammeReferenceScreen_Element_Max
};

enum screenCentrePosition_Attribute {
    screenCentrePosition_coordinate,
    screenCentrePosition_azimuth,
    screenCentrePosition_elevation,
    screenCentrePosition_distance,
    screenCentrePosition_X,
    screenCentrePosition_Y,
    screenCentrePosition_Z,
    screenCentrePosition_Attribute_Max
};

enum screenCentrePosition_Element {
    screenCentrePosition_Element_Max
};

enum screenWidth_Attribute {
    screenWidth_coordinate,
    screenWidth_azimuth,
    screenWidth_X,
    screenWidth_Attribute_Max
};

enum screenWidth_Element {
    screenWidth_Element_Max
};

enum authoringInformation_Attribute {
    authoringInformation_Attribute_Max
};

enum authoringInformation_Element {
    authoringInformation_referenceLayout,
    authoringInformation_renderer,
    authoringInformation_Element_Max
};

enum referenceLayout_Attribute {
    referenceLayout_Attribute_Max
};

enum referenceLayout_Element {
    referenceLayout_audioPackFormatIDRef,
    referenceLayout_Element_Max
};

enum audioContentLabel_Attribute {
    audioContentLabel_language,
    audioContentLabel_Attribute_Max
};

enum audioContentLabel_Element {
    audioContentLabel_Element_Max
};

enum dialogue_Attribute {
    dialogue_nonDialogueContentKind,
    dialogue_dialogueContentKind,
    dialogue_mixedContentKind,
    dialogue_Attribute_Max
};

enum dialogue_Element {
    dialogue_Element_Max
};

enum audioObjectLabel_Attribute {
    audioObjectLabel_language,
    audioObjectLabel_Attribute_Max
};

enum audioObjectLabel_Element {
    audioObjectLabel_Element_Max
};

enum audioComplementaryObjectGroupLabel_Attribute {
    audioComplementaryObjectGroupLabel_language,
    audioComplementaryObjectGroupLabel_Attribute_Max
};

enum audioComplementaryObjectGroupLabel_Element {
    audioComplementaryObjectGroupLabel_Element_Max
};

enum audioObjectInteraction_Attribute {
    audioObjectInteraction_onOffInteract,
    audioObjectInteraction_gainInteract,
    audioObjectInteraction_positionInteract,
    audioObjectInteraction_Attribute_Max
};

enum audioObjectInteraction_Element {
    audioObjectInteraction_gainInteractionRange,
    audioObjectInteraction_positionInteractionRange,
    audioObjectInteraction_Element_Max
};

enum gainInteractionRange_Attribute {
    gainInteractionRange_gainUnit,
    gainInteractionRange_bound,
    gainInteractionRange_Attribute_Max
};

enum gainInteractionRange_Element {
    gainInteractionRange_Element_Max
};

enum positionInteractionRange_Attribute {
    positionInteractionRange_coordinate,
    positionInteractionRange_bound,
    positionInteractionRange_Attribute_Max
};

enum positionInteractionRange_Element {
    positionInteractionRange_Element_Max
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
    audioBlockFormat_jumpPosition,
    audioBlockFormat_headLocked,
    audioBlockFormat_headphoneVirtualise,
    audioBlockFormat_speakerLabel,
    audioBlockFormat_position,
    audioBlockFormat_outputChannelFormatIDRef,
    audioBlockFormat_outputChannelIDRef,
    audioBlockFormat_matrix,
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

enum gain_Attribute {
    gain_gainUnit,
    gain_Attribute_Max
};

enum gain_Element {
    gain_Element_Max
};

enum headphoneVirtualise_Attribute {
    headphoneVirtualise_bypass,
    headphoneVirtualise_DRR,
    headphoneVirtualise_Attribute_Max
};

enum headphoneVirtualise_Element {
    headphoneVirtualise_Element_Max
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

enum positionOffset_Attribute {
    positionOffset_coordinate,
    positionOffset_Attribute_Max
};

enum positionOffset_Element {
    positionOffset_Element_Max
};

enum channelLock_Attribute {
    channelLock_maxDistance,
    channelLock_Attribute_Max
};

enum channelLock_Element {
    channelLock_Element_Max
};

enum objectDivergence_Attribute {
    objectDivergence_azimuthRange,
    objectDivergence_positionRange,
    objectDivergence_Attribute_Max
};

enum objectDivergence_Element {
    objectDivergence_Element_Max
};
enum jumpPosition_Attribute {
    jumpPosition_interpolationLength,
    jumpPosition_Attribute_Max
};

enum jumpPosition_Element {
    jumpPosition_Element_Max
};

enum zoneExclusion_Attribute {
    zoneExclusion_Attribute_Max
};

enum zoneExclusion_Element {
    zoneExclusion_zone,
    zoneExclusion_Element_Max
};

enum zone_Attribute {
    zone_minX,
    zone_maxX,
    zone_minY,
    zone_maxY,
    zone_minZ,
    zone_maxZ,
    zone_minElevation,
    zone_maxElevation,
    zone_minAzimuth,
    zone_maxAzimuth,
    zone_Attribute_Max
};

enum zone_Element {
    zone_Element_Max
};

enum matrix_Attribute {
    matrix_Attribute_Max
};

enum matrix_Element {
    matrix_coefficient,
    matrix_Element_Max
};

enum coefficient_Attribute {
    coefficient_gainUnit,
    coefficient_gain,
    coefficient_gainVar,
    coefficient_phase,
    coefficient_phaseVar,
    coefficient_delay,
    coefficient_delayVar,
    coefficient_Attribute_Max
};

enum coefficient_Element {
    coefficient_Element_Max
};

enum alternativeValueSet_Attribute {
    alternativeValueSet_alternativeValueSetID,
    alternativeValueSet_Attribute_Max
};

enum alternativeValueSet_Element {
    alternativeValueSet_Element_Max
};

enum frequency_Attribute {
    frequency_typeDefinition,
    frequency_Attribute_Max
};

enum frequency_Element {
    frequency_Element_Max
};

enum profile_Attribute {
    profile_profileName,
    profile_profileVersion,
    profile_profileLevel,
    profile_Attribute_Max
};

enum profile_Element {
    profile_Element_Max
};

enum tagGroup_Attribute {
    tagGroup_Attribute_Max
};

enum tagGroup_Element {
    tagGroup_tag,
    tagGroup_audioProgrammeIDRef,
    tagGroup_audioContentIDRef,
    tagGroup_audioObjectIDRef,
    tagGroup_Element_Max
};

enum error_Type {
    Error,
    Warning,
    Information,
    error_Type_Max,
};

enum source {
    Source_ADM,
    Source_Atmos_1_0,
    Source_Atmos_1_1,
    Source_AdvSSE_1,
    source_Max,
};
static const char* Source_Strings[] = {
    "",
    ADM_Atmos_1_0,
    ADM_Atmos_1_1,
    ADM_AdvSSE_1,
};
static_assert(sizeof(Source_Strings) / sizeof(Source_Strings[0]) == source_Max, "");

static const char* error_Type_String[] = {
    "Errors",
    "Warnings",
    "Infos",
};
static_assert(sizeof(error_Type_String) / sizeof(error_Type_String[0]) == error_Type_Max, IncoherencyMessage);

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


                                                 // Ver  |Count|Atmos   |AdvSS-E
                                                 // m  M |0  2+|0  1  2+|0  1  2+
static constexpr element_items root_Elements =
{
    { "frameHeader"                             , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 }, item_frameHeader },
    { "audioFormatExtended"                     , { 0, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 }, item_audioFormatExtended },
};
static_assert(sizeof(root_Elements) / sizeof(element_item) == root_Element_Max, IncoherencyMessage);

static element_items audioFormatExtended_Elements =
{
    { "audioProgramme"                          , { 0, 7, 0, 1, 1, 0, 1, 1, 0, 1, 1 }, item_audioProgramme },
    { "audioContent"                            , { 0, 7, 1, 1, 1, 1, 1, 1, 0, 1, 1 }, item_audioContent },
    { "audioObject"                             , { 0, 7, 1, 1, 1, 1, 1, 1, 0, 1, 1 }, item_audioObject },
    { "audioPackFormat"                         , { 0, 7, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, item_audioPackFormat },
    { "audioChannelFormat"                      , { 0, 7, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, item_audioChannelFormat },
    { "audioTrackUID"                           , { 0, 7, 1, 1, 1, 1, 1, 1, 0, 1, 1 }, item_audioTrackUID },
    { "audioTrackFormat"                        , { 0, 7, 1, 1, 1, 1, 1, 1, 1, 0, 0 }, item_audioTrackFormat },
    { "audioStreamFormat"                       , { 0, 7, 1, 1, 1, 1, 1, 1, 1, 0, 0 }, item_audioStreamFormat },
    { "profileList"                             , { 3, 7, 1, 1, 0, 1, 1, 0, 0, 1, 0 }, item_profileList },
    { "tagList"                                 , { 3, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 }, item_tagList },
};
static_assert(sizeof(audioFormatExtended_Elements) / sizeof(element_item) == audioFormatExtended_Element_Max, IncoherencyMessage);

static attribute_items audioProgramme_Attributes =
{
    { "audioProgrammeID"                        , { 0, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "audioProgrammeName"                      , { 0, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "audioProgrammeLanguage"                  , { 0, 7, 1, 1, 0, 1, 1, 0, 0, 1, 0 } },
    { "start"                                   , { 0, 7, 1, 1, 0, 0, 1, 0, 1, 0, 0 } },
    { "end"                                     , { 0, 7, 1, 1, 0, 0, 1, 0, 1, 0, 0 } },
    { "typeLabel"                               , { 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "typeDefinition"                          , { 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "typeLink"                                , { 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "typeLanguage"                            , { 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "formatLabel"                             , { 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "formatDefinition"                        , { 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "formatLink"                              , { 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "formatLanguage"                          , { 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "maxDuckingDepth"                         , { 0, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
};
static_assert(sizeof(audioProgramme_Attributes) / sizeof(attribute_item) == audioProgramme_Attribute_Max, IncoherencyMessage);

static element_items audioProgramme_Elements =
{
    { "audioProgrammeLabel"                     , { 2, 7, 1, 1, 1, 1, 0, 0, 1, 1, 1 }, item_audioProgrammeLabel },
    { "audioContentIDRef"                       , { 0, 7, 0, 1, 1, 0, 1, 1, 0, 1, 1 } },
    { "loudnessMetadata"                        , { 0, 7, 1, 1, 1, 1, 1, 0, 0, 1, 0 }, item_loudnessMetadata },
    { "audioProgrammeReferenceScreen"           , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 }, item_audioProgrammeReferenceScreen },
    { "authoringInformation"                    , { 2, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 }, item_authoringInformation },
    { "alternativeValueSetIDRef"                , { 2, 7, 1, 1, 1, 1, 0, 0, 1, 1, 1 } },
};
static_assert(sizeof(audioProgramme_Elements) / sizeof(element_item) == audioProgramme_Element_Max, IncoherencyMessage);

static attribute_items audioContent_Attributes =
{
    { "audioContentID"                          , { 0, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "audioContentName"                        , { 0, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "audioContentLanguage"                    , { 0, 7, 1, 1, 0, 1, 0, 0, 0, 1, 0 } },
    { "typeLabel"                               , { 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0 } }, // TODO
};
static_assert(sizeof(audioContent_Attributes) / sizeof(attribute_item) == audioContent_Attribute_Max, IncoherencyMessage);

static element_items audioContent_Elements =
{
    { "audioContentLabel"                       , { 2, 7, 1, 1, 1, 1, 0, 0, 1, 1, 1 }, item_audioContentLabel },
    { "audioObjectIDRef"                        , { 0, 7, 0, 1, 1, 0, 1, 1, 0, 1, 0 } },
    { "loudnessMetadata"                        , { 0, 7, 1, 1, 1, 1, 0, 0, 0, 1, 0 }, item_loudnessMetadata },
    { "dialogue"                                , { 0, 7, 1, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "alternativeValueSetIDRef"                , { 2, 7, 1, 1, 1, 1, 0, 0, 1, 0, 1 } },
};
static_assert(sizeof(audioContent_Elements) / sizeof(element_item) == audioContent_Element_Max, IncoherencyMessage);

static attribute_items audioObject_Attributes =
{
    { "audioObjectID"                           , { 0, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "audioObjectName"                         , { 0, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "start"                                   , { 1, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 } },
    { "startTime"                               , { 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0 } },
    { "duration"                                , { 0, 7, 1, 1, 0, 0, 1, 0, 1, 0, 0 } },
    { "dialogue"                                , { 0, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "importance"                              , { 0, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "interact"                                , { 0, 7, 1, 1, 0, 1, 0, 0, 0, 1, 0 } },
    { "disableDucking"                          , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 } },
    { "typeLabel"                               , { 0, 0, 1,1,  0, 1, 0, 0, 1, 0, 0 } }, // TODO
};
static_assert(sizeof(audioObject_Attributes) / sizeof(attribute_item) == audioObject_Attribute_Max, IncoherencyMessage);

static element_items audioObject_Elements =
{
    { "audioPackFormatIDRef"                    , { 0, 7, 1, 1, 1, 0, 1, 0, 1, 1, 0 } },
    { "audioObjectIDRef"                        , { 0, 7, 1, 1, 1, 1, 0, 0, 1, 1, 1 } },
    { "audioObjectLabel"                        , { 2, 7, 1, 1, 1, 1, 0, 0, 1, 0, 0 }, item_audioObjectLabel },
    { "audioComplementaryObjectGroupLabel"      , { 2, 7, 1, 1, 1, 1, 0, 0, 1, 1, 1 }, item_audioComplementaryObjectGroupLabel },
    { "audioComplementaryObjectIDRef"           , { 0, 7, 1, 1, 1, 1, 0, 0, 1, 1, 1 } },
    { "audioTrackUIDRef"                        , { 0, 7, 1, 1, 1, 0, 1, 1, 1, 1, 1 } },
    { "audioObjectInteraction"                  , { 0, 7, 1, 1, 0, 1, 0, 0, 1, 1, 0 }, item_audioObjectInteraction },
    { "gain"                                    , { 2, 7, 1, 1, 0, 1, 0, 0, 1, 1, 0 }, item_gain },
    { "headLocked"                              , { 2, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "positionOffset"                          , { 2, 7, 1, 1, 0, 1, 0, 0, 1, 1, 0 }, item_positionOffset },
    { "mute"                                    , { 2, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "alternativeValueSet"                     , { 2, 7, 1, 1, 1, 1, 0, 0, 1, 1, 1 }, item_alternativeValueSet },
};
static_assert(sizeof(audioObject_Elements) / sizeof(element_item) == audioObject_Element_Max, IncoherencyMessage);

static attribute_items audioPackFormat_Attributes =
{
    { "audioPackFormatID"                       , { 0, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "audioPackFormatName"                     , { 0, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "typeDefinition"                          , { 0, 7, 1, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "typeLabel"                               , { 0, 7, 1, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "typeLink"                                , { 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "typeLanguage"                            , { 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "importance"                              , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 } },
};
static_assert(sizeof(audioPackFormat_Attributes) / sizeof(attribute_item) == audioPackFormat_Attribute_Max, IncoherencyMessage);

static element_items audioPackFormat_Elements =
{
    { "audioChannelFormatIDRef"                 , { 0, 7, 1, 1, 1, 0, 1, 1, 0, 1, 1 } },
    { "audioPackFormatIDRef"                    , { 0, 7, 1, 1, 1, 1, 0, 0, 1, 0, 0 } },
    { "absoluteDistance"                        , { 0, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "encodePackFormatIDRef"                   , { 1, 7, 1, 1, 1, 1, 0, 0, 1, 0, 0 } },
    { "decodePackFormatIDRef"                   , { 1, 7, 1, 1, 1, 1, 0, 0, 1, 0, 0 } },
    { "inputPackFormatIDRef"                    , { 1, 7, 1, 1, 0, 1, 0, 0, 1, 1, 0 } },
    { "outputPackFormatIDRef"                   , { 1, 7, 1, 1, 0, 1, 0, 0, 1, 1, 0 } },
    { "normalization"                           , { 1, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "nfcRefDist"                              , { 1, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "screenRef"                               , { 1, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
};
static_assert(sizeof(audioPackFormat_Elements) / sizeof(element_item) == audioPackFormat_Element_Max, IncoherencyMessage);

static attribute_items audioChannelFormat_Attributes =
{
    { "audioChannelFormatID"                    , { 0, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "audioChannelFormatName"                  , { 0, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "typeDefinition"                          , { 0, 7, 1, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "typeLabel"                               , { 0, 7, 1, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "typeLink"                                , { 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "typeLanguage"                            , { 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
};
static_assert(sizeof(audioChannelFormat_Attributes) / sizeof(attribute_item) == audioChannelFormat_Attribute_Max, IncoherencyMessage);

static element_items audioChannelFormat_Elements =
{
    { "audioBlockFormat"                        , { 0, 7, 0, 1, 1, 0, 1, 1, 0, 1, 1 }, item_audioBlockFormat },
    { "frequency"                               , { 0, 7, 1, 1, 1, 1, 1, 1, 1, 0, 1 }, item_frequency },
};
static_assert(sizeof(audioChannelFormat_Elements) / sizeof(element_item) == audioChannelFormat_Element_Max, IncoherencyMessage);

static attribute_items audioTrackUID_Attributes =
{
    { "UID"                                     , { 0, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "sampleRate"                              , { 0, 7, 1, 1, 0, 0, 1, 0, 1, 1, 0 } },
    { "bitDepth"                                , { 0, 7, 1, 1, 0, 0, 1, 0, 1, 1, 0 } },
    { "typeLabel"                               , { 0, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 } }, // TODO
};
static_assert(sizeof(audioTrackUID_Attributes) / sizeof(attribute_item) == audioTrackUID_Attribute_Max, IncoherencyMessage);

static element_items audioTrackUID_Elements =
{
    { "audioMXFLookUp"                          , { 0, 2, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "audioTrackFormatIDRef"                   , { 0, 7, 1, 1, 0, 0, 1, 0, 1, 0, 0 } },
    { "audioChannelFormatIDRef"                 , { 2, 7, 1, 1, 0, 1, 0, 0, 0, 1, 0 } },
    { "audioPackFormatIDRef"                    , { 0, 7, 1, 1, 0, 0, 1, 0, 0, 1, 0 } },
};
static_assert(sizeof(audioTrackUID_Elements) / sizeof(element_item) == audioTrackUID_Element_Max, IncoherencyMessage);

static attribute_items audioTrackFormat_Attributes =
{
    { "audioTrackFormatID"                      , { 0, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "audioTrackFormatName"                    , { 0, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "typeLabel"                               , { 0, 7, 1, 1, 0, 1, 0, 0, 1, 1, 0 } }, // TODO: present in some Atmos files
    { "typeDefinition"                          , { 7, 7, 1, 1, 0, 1, 0, 0, 1, 1, 0 } }, // TODO: present in some Atmos files
    { "formatLabel"                             , { 0, 7, 1, 1, 0, 0, 1, 0, 1, 1, 0 } },
    { "formatDefinition"                        , { 0, 7, 1, 1, 0, 0, 1, 0, 1, 1, 0 } },
    { "formatLink"                              , { 0, 7, 1, 1, 0, 1, 0, 0, 1, 1, 0 } },
    { "formatLanguage"                          , { 0, 7, 1, 1, 0, 1, 0, 0, 1, 1, 0 } },
};
static_assert(sizeof(audioTrackFormat_Attributes) / sizeof(attribute_item) == audioTrackFormat_Attribute_Max, IncoherencyMessage);

static element_items audioTrackFormat_Elements =
{
    { "audioStreamFormatIDRef"                  , { 0, 7, 0, 1, 0, 0, 1, 0, 1, 1, 0 } },
};
static_assert(sizeof(audioTrackFormat_Elements) / sizeof(element_item) == audioTrackFormat_Element_Max, IncoherencyMessage);

static attribute_items audioStreamFormat_Attributes =
{
    { "audioStreamFormatID"                     , { 0, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "audioStreamFormatName"                   , { 0, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "typeLabel"                               , { 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0 } }, // TODO: present in some Atmos files
    { "typeDefinition"                          , { 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0 } }, // TODO: present in some Atmos files
    { "formatLabel"                             , { 0, 7, 1, 1, 0, 0, 1, 0, 1, 1, 0 } },
    { "formatDefinition"                        , { 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 } },
    { "formatLink"                              , { 0, 7, 1, 1, 0, 1, 0, 0, 1, 1, 0 } },
    { "formatLanguage"                          , { 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0 } },
};
static_assert(sizeof(audioStreamFormat_Attributes) / sizeof(attribute_item) == audioStreamFormat_Attribute_Max, IncoherencyMessage);

static element_items audioStreamFormat_Elements =
{
    { "audioChannelFormatIDRef"                 , { 0, 7, 1, 1, 1, 0, 1, 0, 1, 1, 1 } },
    { "audioPackFormatIDRef"                    , { 0, 7, 1, 1, 1, 0, 1, 0, 1, 1, 1 } },
    { "audioTrackFormatIDRef"                   , { 0, 7, 1, 1, 1, 0, 1, 0, 1, 1, 1 } },
};
static_assert(sizeof(audioStreamFormat_Elements) / sizeof(element_item) == audioStreamFormat_Element_Max, IncoherencyMessage);

static element_items profileList_Elements =
{
    { "profile"                                 , { 3, 7, 0, 1, 1, 0, 1, 1, 0, 1, 1 } },
};
static_assert(sizeof(profileList_Elements) / sizeof(element_item) == profileList_Element_Max, IncoherencyMessage);

static element_items tagList_Elements =
{
    { "tagGroup"                                , { 3, 7, 0, 1, 1, 0, 1, 1, 0, 1, 1 } },
};
static_assert(sizeof(tagList_Elements) / sizeof(element_item) == tagList_Element_Max, IncoherencyMessage);

static element_items frameHeader_Elements =
{
    { "profileList"                             , { 0, 7, 1, 1, 0, 1, 1, 0, 0, 1, 0 }, item_profileList },
    { "frameFormat"                             , { 0, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 }, item_frameFormat },
    { "transportTrackFormat"                    , { 0, 7, 0, 1, 1, 0, 1, 1, 0, 1, 1 }, item_transportTrackFormat },
};
static_assert(sizeof(frameHeader_Elements) / sizeof(element_item) == frameHeader_Element_Max, IncoherencyMessage);

static attribute_items frameFormat_Attributes =
{
    { "frameFormatID"                           , { 0, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "start"                                   , { 0, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "duration"                                , { 0, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "type"                                    , { 0, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "timeReference"                           , { 0, 7, 1, 1, 0, 1, 1, 0, 0, 1, 0 } },
    { "flowID"                                  , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "countToFull"                             , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "numMetadataChunks"                       , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "countToSameChunk"                        , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
};
static_assert(sizeof(frameFormat_Attributes) / sizeof(attribute_item) == frameFormat_Attribute_Max, IncoherencyMessage);

static element_items frameFormat_Elements =
{
    { "changedIDs"                              , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 } },
    { "chunkAdmElement"                         , { 0, 7, 1, 1, 1, 1, 1, 0, 1, 0, 0 } },
};
static_assert(sizeof(frameFormat_Elements) / sizeof(element_item) == frameFormat_Element_Max, IncoherencyMessage);

static attribute_items transportTrackFormat_Attributes =
{
    { "transportID"                             , { 0, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "transportName"                           , { 0, 7, 1, 1, 0, 1, 1, 0, 0, 1, 0 } },
    { "numTracks"                               , { 0, 7, 1, 1, 0, 1, 1, 0, 0, 1, 0 } },
    { "numIDs"                                  , { 0, 7, 1, 1, 0, 1, 1, 0, 0, 1, 0 } },
};
static_assert(sizeof(transportTrackFormat_Attributes) / sizeof(attribute_item) == transportTrackFormat_Attribute_Max, IncoherencyMessage);

static element_items transportTrackFormat_Elements =
{
    { "audioTrack"                              , { 0, 7, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, item_audioTrack },
};
static_assert(sizeof(transportTrackFormat_Elements) / sizeof(element_item) == transportTrackFormat_Element_Max, IncoherencyMessage);

static attribute_items audioTrack_Attributes =
{
    { "trackID"                                 , { 0, 7, 0, 1, 0, 1, 0, 0, 0, 1, 0 } },
    { "formatLabel"                             , { 0, 7, 1, 1, 0, 1, 1, 0, 0, 1, 0 } },
    { "formatDefinition"                        , { 0, 7, 1, 1, 0, 1, 1, 0, 0, 1, 0 } },
};
static_assert(sizeof(audioTrack_Attributes) / sizeof(attribute_item) == audioTrack_Attribute_Max, IncoherencyMessage);

static element_items audioTrack_Elements =
{
    { "audioTrackUIDRef"                        , { 0, 7, 1, 1, 1, 1, 1, 1, 1, 1, 1 } },
};
static_assert(sizeof(audioTrack_Elements) / sizeof(element_item) == audioTrack_Element_Max, IncoherencyMessage);

static attribute_items audioProgrammeLabel_Attributes =
{
    { "language"                                , { 0, 7, 1, 1, 0, 0, 1, 0, 1, 1, 0 } },
};
static_assert(sizeof(audioProgrammeLabel_Attributes) / sizeof(attribute_item) == audioProgrammeLabel_Attribute_Max, IncoherencyMessage);

static attribute_items loudnessMetadata_Attributes =
{
    { "loudnessMethod"                          , { 0, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "loudnessRecType"                         , { 0, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "loudnessCorrectionType"                  , { 0, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
};
static_assert(sizeof(loudnessMetadata_Attributes) / sizeof(attribute_item) == loudnessMetadata_Attribute_Max, IncoherencyMessage);

static element_items loudnessMetadata_Elements =
{
    { "integratedLoudness"                      , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "loudnessRange"                           , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 } },
    { "maxTruePeak"                             , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 } },
    { "maxMomentary"                            , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 } },
    { "maxShortTerm"                            , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 } },
    { "dialogueLoudness"                        , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "renderer"                                , { 3, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 }, item_renderer },
};
static_assert(sizeof(loudnessMetadata_Elements) / sizeof(element_item) == loudnessMetadata_Element_Max, IncoherencyMessage);

static attribute_items renderer_Attributes =
{
    { "uri"                                     , { 0, 7, 1, 1, 0, 0, 1, 0, 1, 1, 0 } },
    { "name"                                    , { 0, 7, 1, 1, 0, 0, 1, 0, 1, 1, 0 } },
    { "version"                                 , { 0, 7, 1, 1, 0, 0, 1, 0, 1, 1, 0 } },
    { "coordinateMode"                          , { 3, 7, 1, 1, 0, 0, 1, 0, 1, 1, 0 } },
};
static_assert(sizeof(renderer_Attributes) / sizeof(attribute_item) == renderer_Attribute_Max, IncoherencyMessage);

static element_items renderer_Elements =
{
    { "audioPackFormatIDRef"                    , { 3, 7, 1, 1, 1, 1, 1, 1, 1, 1, 0 } },
    { "audioObjectIDRef"                        , { 3, 7, 1, 1, 1, 1, 1, 1, 1, 1, 1 } },
};
static_assert(sizeof(renderer_Elements) / sizeof(element_item) == renderer_Element_Max, IncoherencyMessage);

static attribute_items audioProgrammeReferenceScreen_Attributes =
{
    { "aspectRatio"                             , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
};
static_assert(sizeof(audioProgrammeReferenceScreen_Attributes) / sizeof(attribute_item) == audioProgrammeReferenceScreen_Attribute_Max, IncoherencyMessage);

static element_items audioProgrammeReferenceScreen_Elements =
{
    { "screenCentrePosition"                    , { 0, 7, 1, 1, 1, 1, 0, 0, 1, 1, 1 }, item_screenCentrePosition },
    { "screenWidth"                             , { 0, 7, 1, 1, 1, 1, 1, 0, 1, 1, 1 }, item_screenWidth },
};
static_assert(sizeof(audioProgrammeReferenceScreen_Elements) / sizeof(element_item) == audioProgrammeReferenceScreen_Element_Max, IncoherencyMessage);

static attribute_items screenCentrePosition_Attributes =
{
    { "coordinate"                              , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } }, // TODO: clarify coordinate="X" or X=
    { "azimuth"                                 , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "elevation"                               , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "distance"                                , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "X"                                       , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "Y"                                       , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "Z"                                       , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
};
static_assert(sizeof(screenCentrePosition_Attributes) / sizeof(attribute_item) == screenCentrePosition_Attribute_Max, IncoherencyMessage);

static attribute_items screenWidth_Attributes =
{
    { "coordinate"                              , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } }, // TODO: clarify coordinate="X" or X=
    { "azimuth"                                 , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "X"                                       , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
};
static_assert(sizeof(screenWidth_Attributes) / sizeof(attribute_item) == screenWidth_Attribute_Max, IncoherencyMessage);

static element_items authoringInformation_Elements =
{
    { "referenceLayout"                         , { 2, 7, 1, 1, 1, 1, 0, 0, 1, 1, 1 }, item_referenceLayout },
    { "renderer"                                , { 2, 7, 1, 1, 1, 1, 0, 0, 1, 1, 1 }, item_renderer },
};
static_assert(sizeof(authoringInformation_Elements) / sizeof(element_item) == authoringInformation_Element_Max, IncoherencyMessage);

static element_items referenceLayout_Elements =
{
    { "audioPackFormatIDRef"                    , { 2, 7, 0, 1, 0, 1, 0, 0, 0, 1, 0 } },
};
static_assert(sizeof(referenceLayout_Elements) / sizeof(element_item) == referenceLayout_Element_Max, IncoherencyMessage);

static attribute_items audioContentLabel_Attributes =
{
    { "language"                                , { 0, 7, 1, 1, 0, 0, 1, 0, 1, 1, 0 } },
};
static_assert(sizeof(audioContentLabel_Attributes) / sizeof(attribute_item) == audioContentLabel_Attribute_Max, IncoherencyMessage);

static attribute_items dialogue_Attributes =
{
    { "nonDialogueContentKind"                  , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "dialogueContentKind"                     , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "mixedContentKind"                        , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
};
static_assert(sizeof(dialogue_Attributes) / sizeof(attribute_item) == dialogue_Attribute_Max, IncoherencyMessage);

static attribute_items audioObjectLabel_Attributes =
{
    { "language"                                , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
};
static_assert(sizeof(audioObjectLabel_Attributes) / sizeof(attribute_item) == audioObjectLabel_Attribute_Max, IncoherencyMessage);

static attribute_items audioComplementaryObjectGroupLabel_Attributes =
{
    { "language"                                , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
};
static_assert(sizeof(audioComplementaryObjectGroupLabel_Attributes) / sizeof(attribute_item) == audioComplementaryObjectGroupLabel_Attribute_Max, IncoherencyMessage);

static attribute_items audioObjectInteraction_Attributes =
{
    { "onOffInteract"                           , { 0, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "gainInteract"                            , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "positionInteract"                        , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
};
static_assert(sizeof(audioObjectInteraction_Attributes) / sizeof(attribute_item) == audioObjectInteraction_Attribute_Max, IncoherencyMessage);

static element_items audioObjectInteraction_Elements =
{
    { "gainInteractionRange"                    , { 0, 7, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, item_gainInteractionRange },
    { "positionInteractionRange"                , { 0, 7, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, item_positionInteractionRange },
};
static_assert(sizeof(audioObjectInteraction_Elements) / sizeof(element_item) == audioObjectInteraction_Element_Max, IncoherencyMessage);

static attribute_items gainInteractionRange_Attributes =
{
    { "gainUnit"                                , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "bound"                                   , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
};
static_assert(sizeof(gainInteractionRange_Attributes) / sizeof(attribute_item) == gainInteractionRange_Attribute_Max, IncoherencyMessage);

static attribute_items positionInteractionRange_Attributes =
{
    { "coordinate"                              , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "bound"                                   , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
};
static_assert(sizeof(positionInteractionRange_Attributes) / sizeof(attribute_item) == positionInteractionRange_Attribute_Max, IncoherencyMessage);

static attribute_items audioBlockFormat_Attributes =
{
    { "audioBlockFormatID"                      , { 0, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "rtime"                                   , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "duration"                                , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "lstart"                                  , { 0, 7, 1, 1, 0, 1, 0, 0, 1, 1, 0 } },
    { "lduration"                               , { 0, 7, 1, 1, 0, 1, 0, 0, 1, 1, 0 } },
    { "initializeBlock"                         , { 0, 7, 1, 1, 0, 1, 0, 0, 1, 1, 0 } },
};
static_assert(sizeof(audioBlockFormat_Attributes) / sizeof(attribute_item) == audioBlockFormat_Attribute_Max, IncoherencyMessage);

static element_items audioBlockFormat_DirectSpeakers_Elements =
{
    { "gain"                                    , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 }, item_gain },
    { "importance"                              , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 } },
    { "jumpPosition"                            , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "headLocked"                              , { 2, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "headphoneVirtualise"                     , { 2, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "speakerLabel"                            , { 0, 7, 1, 1, 1, 1, 1, 0, 1, 0, 0 } },
    { "position"                                , { 0, 7, 0, 0, 1, 0, 0, 1, 0, 0, 1 }, item_position },
    { "outputChannelFormatIDRef"                , { 1, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "outputChannelIDRef,"                     , { 1, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "matrix"                                  , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "width"                                   , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "height"                                  , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "depth"                                   , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "cartesian"                               , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "diffuse"                                 , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "channelLock"                             , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "objectDivergence"                        , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "zoneExclusion"                           , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "equation"                                , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "order"                                   , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "degree"                                  , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "normalization"                           , { 1, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "nfcRefDist"                              , { 1, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "screenRef"                               , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
};
static_assert(sizeof(audioBlockFormat_DirectSpeakers_Elements) / sizeof(element_item) == audioBlockFormat_Element_Max, IncoherencyMessage);

static element_items audioBlockFormat_Matrix_Elements =
{
    { "gain"                                    , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 }, item_gain },
    { "importance"                              , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 } },
    { "jumpPosition"                            , { 0, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "headLocked"                              , { 2, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "headphoneVirtualise"                     , { 2, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 }, item_headphoneVirtualise },
    { "speakerLabel"                            , { 0, 7, 1, 1, 1, 1, 1, 0, 1, 0, 0 } },
    { "position"                                , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "outputChannelFormatIDRef"                , { 1, 7, 1, 1, 0, 1, 0, 0, 0, 1, 0 } },
    { "outputChannelIDRef,"                     , { 1, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "matrix"                                  , { 0, 7, 0, 1, 1, 1, 0, 0, 0, 1, 0 }, item_matrix },
    { "width"                                   , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "height"                                  , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "depth"                                   , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "cartesian"                               , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "diffuse"                                 , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "channelLock"                             , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "objectDivergence"                        , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "zoneExclusion"                           , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "equation"                                , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "order"                                   , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "degree"                                  , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "normalization"                           , { 1, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "nfcRefDist"                              , { 1, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "screenRef"                               , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
};
static_assert(sizeof(audioBlockFormat_Matrix_Elements) / sizeof(element_item) == audioBlockFormat_Element_Max, IncoherencyMessage);

static element_items audioBlockFormat_Object_Elements =
{
    { "gain"                                    , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 }, item_gain },
    { "importance"                              , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 } },
    { "jumpPosition"                            , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 }, item_jumpPosition },
    { "headLocked"                              , { 2, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "headphoneVirtualise"                     , { 2, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 }, item_headphoneVirtualise },
    { "speakerLabel"                            , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "position"                                , { 0, 7, 1, 1, 1, 0, 1, 1, 1, 1, 1 }, item_position },
    { "outputChannelFormatIDRef"                , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "outputChannelIDRef,"                     , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "matrix"                                  , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 0, 0 } },
    { "width"                                   , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 } },
    { "height"                                  , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 } },
    { "depth"                                   , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 } },
    { "cartesian"                               , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "diffuse"                                 , { 0, 7, 1, 1, 1, 1, 1, 0, 1, 0, 0 } },
    { "channelLock"                             , { 0, 7, 1, 1, 1, 1, 1, 0, 1, 0, 0 }, item_channelLock },
    { "objectDivergence"                        , { 0, 7, 1, 1, 0, 1, 0, 0, 1, 1, 0 }, item_objectDivergence },
    { "zoneExclusion"                           , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 }, item_zoneExclusion },
    { "equation"                                , { 0, 7, 1, 1, 1, 1, 0, 0, 1, 0, 0 } },
    { "order"                                   , { 0, 7, 1, 1, 1, 1, 0, 0, 1, 0, 0 } },
    { "degree"                                  , { 0, 7, 1, 1, 1, 1, 0, 0, 1, 0, 0 } },
    { "normalization"                           , { 1, 7, 1, 1, 1, 1, 0, 0, 1, 0, 0 } },
    { "nfcRefDist"                              , { 1, 7, 1, 1, 1, 1, 0, 0, 1, 0, 0 } },
    { "screenRef"                               , { 0, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
};
static_assert(sizeof(audioBlockFormat_Object_Elements) / sizeof(element_item) == audioBlockFormat_Element_Max, IncoherencyMessage);

static element_items audioBlockFormat_Elements =
{
    { "gain"                                    , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 }, item_gain },
    { "importance"                              , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 } },
    { "jumpPosition"                            , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 }, item_jumpPosition },
    { "headLocked"                              , { 2, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "headphoneVirtualise"                     , { 2, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 }, item_headphoneVirtualise },
    { "speakerLabel"                            , { 0, 7, 1, 1, 1, 1, 1, 0, 1, 0, 0 } },
    { "position"                                , { 0, 7, 1, 1, 1, 0, 1, 1, 1, 1, 1 }, item_position },
    { "outputChannelFormatIDRef"                , { 1, 7, 1, 1, 0, 1, 0, 0, 1, 1, 0 } },
    { "outputChannelIDRef,"                     , { 1, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "matrix"                                  , { 0, 7, 1, 0, 0, 1, 0, 0, 1, 1, 0 }, item_matrix },
    { "width"                                   , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 } },
    { "height"                                  , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 } },
    { "depth"                                   , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 } },
    { "cartesian"                               , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "diffuse"                                 , { 0, 7, 1, 1, 1, 1, 1, 0, 1, 0, 0 } },
    { "channelLock"                             , { 0, 7, 1, 1, 1, 1, 1, 0, 1, 0, 0 }, item_channelLock },
    { "objectDivergence"                        , { 0, 7, 1, 1, 0, 1, 0, 0, 1, 1, 0 }, item_objectDivergence },
    { "zoneExclusion"                           , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 }, item_zoneExclusion },
    { "equation"                                , { 0, 7, 1, 1, 1, 1, 0, 0, 1, 0, 0 } },
    { "order"                                   , { 0, 7, 1, 1, 1, 1, 0, 0, 1, 0, 0 } },
    { "degree"                                  , { 0, 7, 1, 1, 1, 1, 0, 0, 1, 0, 0 } },
    { "normalization"                           , { 1, 7, 1, 1, 1, 1, 0, 0, 1, 0, 0 } },
    { "nfcRefDist"                              , { 1, 7, 1, 1, 1, 1, 0, 0, 1, 0, 0 } },
    { "screenRef"                               , { 0, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
};
static_assert(sizeof(audioBlockFormat_Elements) / sizeof(element_item) == audioBlockFormat_Element_Max, IncoherencyMessage);

static attribute_items gain_Attributes =
{
    { "gainUnit"                                , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
};
static_assert(sizeof(gain_Attributes) / sizeof(attribute_item) == gain_Attribute_Max, IncoherencyMessage);

static attribute_items headphoneVirtualise_Attributes =
{
    { "bypass"                                  , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "DRR"                                     , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
};
static_assert(sizeof(headphoneVirtualise_Attributes) / sizeof(attribute_item) == headphoneVirtualise_Attribute_Max, IncoherencyMessage);

static attribute_items position_Attributes =
{
    { "coordinate"                              , { 0, 7, 1, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "bound"                                   , { 0, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
    { "screenEdgeLock"                          , { 0, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
};
static_assert(sizeof(position_Attributes) / sizeof(attribute_item) == position_Attribute_Max, IncoherencyMessage);

static attribute_items positionOffset_Attributes =
{
    { "coordinate"                              , { 0, 7, 1, 1, 0, 0, 1, 0, 0, 1, 0 } },
};
static_assert(sizeof(position_Attributes) / sizeof(attribute_item) == position_Attribute_Max, IncoherencyMessage);

static attribute_items channelLock_Attributes =
{
    { "maxDistance"                             , { 0, 7, 1, 1, 0, 1, 0, 0, 1, 0, 0 } },
};
static_assert(sizeof(channelLock_Attributes) / sizeof(attribute_item) == channelLock_Attribute_Max, IncoherencyMessage);

static attribute_items objectDivergence_Attributes =
{
    { "azimuthRange"                            , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "positionRange"                           , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
};
static_assert(sizeof(objectDivergence_Attributes) / sizeof(attribute_item) == objectDivergence_Attribute_Max, IncoherencyMessage);

static attribute_items jumpPosition_Attributes =
{
    { "interpolationLength"                     , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 } },
};
static_assert(sizeof(jumpPosition_Attributes) / sizeof(attribute_item) == jumpPosition_Attribute_Max, IncoherencyMessage);

static element_items zoneExclusion_Elements =
{
    { "zone"                                    , { 0, 7, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, item_zone },
};
static_assert(sizeof(zoneExclusion_Elements) / sizeof(element_item) == zoneExclusion_Element_Max, IncoherencyMessage);

static attribute_items zone_Attributes =
{
    { "minX"                                    , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "maxX"                                    , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "minY"                                    , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "maxY"                                    , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "minZ"                                    , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "maxZ"                                    , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "minElevation"                            , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "maxElevation"                            , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "minAzimuth"                              , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "maxAzimuth"                              , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
};
static_assert(sizeof(zone_Attributes) / sizeof(attribute_item) == zone_Attribute_Max, IncoherencyMessage);

static element_items matrix_Elements =
{
    { "coefficient"                             , { 0, 7, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, item_coefficient },
};
static_assert(sizeof(matrix_Elements) / sizeof(element_item) == matrix_Element_Max, IncoherencyMessage);

static attribute_items coefficient_Attributes =
{
    { "gainUnit"                                , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "gain"                                    , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
    { "gainVar"                                 , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 } },
    { "phase"                                   , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 } },
    { "phaseVar"                                , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 } },
    { "delay"                                   , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 } },
    { "delayVar"                                , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 0, 0 } },
};
static_assert(sizeof(coefficient_Attributes) / sizeof(attribute_item) == coefficient_Attribute_Max, IncoherencyMessage);

static attribute_items alternativeValueSet_Attributes =
{
    { "alternativeValueSetID"                   , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
};
static_assert(sizeof(alternativeValueSet_Attributes) / sizeof(attribute_item) == alternativeValueSet_Attribute_Max, IncoherencyMessage);

static attribute_items frequency_Attributes =
{
    { "typeDefinition"                          , { 0, 7, 1, 1, 0, 1, 1, 0, 1, 1, 0 } },
};
static_assert(sizeof(frequency_Attributes) / sizeof(attribute_item) == frequency_Attribute_Max, IncoherencyMessage);

static attribute_items profile_Attributes =
{
    { "profileName"                             , { 3, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "profileVersion"                          , { 3, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
    { "profileLevel"                            , { 3, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
};
static_assert(sizeof(profile_Attributes) / sizeof(attribute_item) == profile_Attribute_Max, IncoherencyMessage);

static element_items tagGroup_Elements =
{
    { "tag"                                     , { 3, 7, 0, 1, 1, 0, 1, 1, 0, 1, 1 } },
    { "audioProgrammeIDRef"                     , { 3, 7, 1, 1, 1, 1, 1, 1, 1, 1, 1 } },
    { "audioContentIDRef"                       , { 3, 7, 1, 1, 1, 1, 1, 1, 1, 1, 1 } },
    { "audioObjectIDRef"                        , { 3, 7, 1, 1, 1, 1, 1, 1, 1, 1, 1 } },
};
static_assert(sizeof(tagGroup_Elements) / sizeof(element_item) == tagGroup_Element_Max, IncoherencyMessage);

static attribute_items tag_Attributes =
{
    { "class"                                   , { 0, 7, 0, 1, 0, 0, 1, 0, 0, 1, 0 } },
};
static_assert(sizeof(tag_Attributes) / sizeof(attribute_item) == tag_Attribute_Max, IncoherencyMessage);

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
        Flags_ID_V,
        Flags_ID_XZ, // X 8 + option Z 2
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
    { nullptr, (element_items*)&root_Elements, nullptr, nullptr, 0, (int8u)-1, 0 },
    { nullptr, (element_items*)&audioFormatExtended_Elements, nullptr, nullptr, 0, (int8u)-1, 0 },
    { (attribute_items*)&audioProgramme_Attributes, (element_items*)&audioProgramme_Elements, "Programme", "APR", F(ID_W), audioProgramme_audioProgrammeID, 0 },
    { (attribute_items*)&audioContent_Attributes, (element_items*)&audioContent_Elements, "Content", "ACO", F(ID_W), audioContent_audioContentID, 0 },
    { (attribute_items*)&audioObject_Attributes, (element_items*)&audioObject_Elements, "Object", "AO", F(ID_W), audioObject_audioObjectID, 0 },
    { (attribute_items*)&audioPackFormat_Attributes, (element_items*)&audioPackFormat_Elements, "PackFormat", "AP", F(ID_YX), audioPackFormat_audioPackFormatID, 0xFFF },
    { (attribute_items*)&audioChannelFormat_Attributes, (element_items*)&audioChannelFormat_Elements, "ChannelFormat", "AC", F(ID_YX), audioChannelFormat_audioChannelFormatID, 0xFFF },
    { (attribute_items*)&audioTrackUID_Attributes, (element_items*)&audioTrackUID_Elements, "TrackUID", "ATU", F(ID_V), audioTrackUID_UID, 0 },
    { (attribute_items*)&audioTrackFormat_Attributes, (element_items*)&audioTrackFormat_Elements, "TrackFormat", "AT", F(ID_YX) | F(ID_Z1), audioTrackFormat_audioTrackFormatID, 0xFFF },
    { (attribute_items*)&audioStreamFormat_Attributes, (element_items*)&audioStreamFormat_Elements, "StreamFormat", "AS", F(ID_YX), audioStreamFormat_audioStreamFormatID, 0xFFF },
    { nullptr, (element_items*)&profileList_Elements, "profileList", nullptr, 0, (int8u)-1, 0xFFF},
    { nullptr, (element_items*)&tagList_Elements, "tagList", nullptr, 0, (int8u)-1, 0xFFF},
    { nullptr, (element_items*)&frameHeader_Elements, "frameHeader", nullptr, 0, (int8u)-1, 0xFFF},
    { (attribute_items*)&frameFormat_Attributes, (element_items*)&frameFormat_Elements, "frameFormat", "FF", F(ID_V), (int8u)-1, 0},
    { (attribute_items*)&transportTrackFormat_Attributes, (element_items*)&transportTrackFormat_Elements, "transportTrackFormat", "TP", F(ID_W), (int8u)-1, 0},
    { (attribute_items*)&audioTrack_Attributes, (element_items*)&audioTrack_Elements, "Track", nullptr, 0, (int8u)-1, 0 },
    { nullptr, nullptr, "changedIDs", nullptr, 0, (int8u)-1, 0 },
    { (attribute_items*)&audioProgrammeLabel_Attributes, nullptr, "audioProgrammeLabel", nullptr, 0, (int8u)-1, 0},
    { (attribute_items*)&loudnessMetadata_Attributes, (element_items*)&loudnessMetadata_Elements, "loudnessMetadata", nullptr, 0, (int8u)-1, 0},
    { (attribute_items*)&renderer_Attributes, (element_items*)&renderer_Elements, "renderer", nullptr, 0, (int8u)-1, 0},
    { (attribute_items*)&audioProgrammeReferenceScreen_Attributes, (element_items*)&audioProgrammeReferenceScreen_Elements, "audioProgrammeReferenceScreen", nullptr, 0, (int8u)-1, 0},
    { (attribute_items*)&screenCentrePosition_Attributes, nullptr, "screenCentrePosition", nullptr, 0, (int8u)-1, 0},
    { (attribute_items*)&screenWidth_Attributes, nullptr, "screenWidth", nullptr, 0, (int8u)-1, 0},
    { nullptr, (element_items*)&authoringInformation_Elements, "authoringInformation", nullptr, 0, (int8u)-1, 0},
    { nullptr, (element_items*)&referenceLayout_Elements, "referenceLayout", nullptr, 0, (int8u)-1, 0},
    { (attribute_items*)&audioContentLabel_Attributes, nullptr, "audioContentLabel", nullptr, 0, (int8u)-1, 0},
    { (attribute_items*)&dialogue_Attributes, nullptr, "dialogue", nullptr, 0, (int8u)-1, 0},
    { (attribute_items*)&audioObjectLabel_Attributes, nullptr, "dialogue", nullptr, 0, (int8u)-1, 0},
    { (attribute_items*)&audioComplementaryObjectGroupLabel_Attributes, nullptr, "audioComplementaryObjectGroupLabel", nullptr, 0, (int8u)-1, 0},
    { (attribute_items*)&audioObjectInteraction_Attributes, (element_items*)&audioObjectInteraction_Elements, "audioObjectInteraction", nullptr, 0, (int8u)-1, 0},
    { (attribute_items*)&gainInteractionRange_Attributes, nullptr, "gainInteractionRange", nullptr, 0, (int8u)-1, 0},
    { (attribute_items*)&positionInteractionRange_Attributes, nullptr, "positionInteractionRange", nullptr, 0, (int8u)-1, 0},
    { (attribute_items*)&audioBlockFormat_Attributes, (element_items*)&audioBlockFormat_Elements, "BlockFormat", "AB", F(ID_YX) | F(ID_Z1) | F(ID_Z2), (int8u)-1, 0xFFF },
    { (attribute_items*)&gain_Attributes, nullptr, "gain", nullptr, 0, (int8u)-1, 0xFFF},
    { (attribute_items*)&headphoneVirtualise_Attributes, nullptr, "headphoneVirtualise", nullptr, 0, (int8u)-1, 0xFFF},
    { (attribute_items*)&position_Attributes, nullptr, "position", nullptr, 0, (int8u)-1, 0xFFF},
    { (attribute_items*)&positionOffset_Attributes, nullptr, "positionOffset", nullptr, 0, (int8u)-1, 0xFFF},
    { (attribute_items*)&channelLock_Attributes, nullptr, "channelLock", nullptr, 0, (int8u)-1, 0xFFF},
    { (attribute_items*)&objectDivergence_Attributes, nullptr, "objectDivergence", nullptr, 0, (int8u)-1, 0xFFF},
    { (attribute_items*)&jumpPosition_Attributes, nullptr, "jumpPosition", nullptr, 0, (int8u)-1, 0xFFF},
    { nullptr, (element_items*)&zoneExclusion_Elements, "zoneExclusion", nullptr, 0, (int8u)-1, 0xFFF},
    { (attribute_items*)&zone_Attributes, nullptr, "zone", nullptr, 0, (int8u)-1, 0xFFF},
    { nullptr, (element_items*)&matrix_Elements, "matrix", nullptr, 0, (int8u)-1, 0xFFF},
    { (attribute_items*)&coefficient_Attributes, nullptr, "coefficient", nullptr, 0, (int8u)-1, 0xFFF},
    { (attribute_items*)&alternativeValueSet_Attributes, nullptr, "alternativeValueSet", "AVS", F(ID_W) | F(ID_Z2), alternativeValueSet_alternativeValueSetID, 0 },
    { (attribute_items*)&frequency_Attributes, nullptr, "frequency", nullptr, F(ID_W) | F(ID_Z2), (int8u)-1, 0 },
    { (attribute_items*)&profile_Attributes, nullptr, "profile", nullptr, 0, (int8u)-1, 0xFFF},
    { nullptr, (element_items*)&tagGroup_Elements, "tagGroup", nullptr, 0, (int8u)-1, 0xFFF},
    { (attribute_items*)&tag_Attributes, nullptr, "tag", nullptr, 0, (int8u)-1, 0xFFF},
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
    item    Source_Type;
    int8u   Source_Pos;
    int8u   Target_Type;
};
typedef idref idrefs[];
static constexpr idrefs IDRefs = {
    { item_audioProgramme, audioProgramme_audioContentIDRef, item_audioContent },
    { item_audioProgramme, audioProgramme_alternativeValueSetIDRef, item_alternativeValueSet },
    { item_audioContent, audioContent_audioObjectIDRef, item_audioObject },
    { item_audioContent, audioContent_alternativeValueSetIDRef, item_alternativeValueSet },
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
    { item_audioTrack, audioTrack_audioTrackUIDRef, item_audioTrackUID },
};

// First value of each line is the line count
static const int8u audioPackFormat_2_audioChannelFormatIDRef_Table[] = {
     1, 0x03,
     2, 0x01, 0x02,
     6, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
     8, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0D, 0x0F,
    10, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0D, 0x0F, 0x10, 0x12,
    11, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0D, 0x0F, 0x10, 0x12, 0x15,
    12, 0x03, 0x01, 0x02, 0x22, 0x23, 0x0A, 0x0B, 0x1C, 0x1D, 0x28, 0x20, 0x21,
    14, 0x01, 0x02, 0x03, 0x04, 0x0A, 0x0B, 0x1C, 0x1D, 0x22, 0x23, 0x1E, 0x1F, 0x24, 0x25,
    24, 0x18, 0x19, 0x03, 0x20, 0x1C, 0x1D, 0x01, 0x02, 0x09, 0x21, 0x0A, 0x0B, 0x22, 0x23, 0x0E, 0x0C, 0x1E, 0x1F, 0x13, 0x14, 0x11, 0x15, 0x16, 0x17,
     3, 0x01, 0x02, 0x03,
     4, 0x01, 0x02, 0x03, 0x09,
     5, 0x01, 0x02, 0x03, 0x05, 0x06,
     7, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x09,
     8, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x26, 0x27,
     8, 0x01, 0x02, 0x03, 0x04, 0x0A, 0x0B, 0x1C, 0x1D,
    22, 0x18, 0x19, 0x03, 0x1C, 0x1D, 0x01, 0x02, 0x09, 0x0A, 0x0B, 0x22, 0x23, 0x0E, 0x0C, 0x1E, 0x1F, 0x13, 0x14, 0x11, 0x15, 0x16, 0x17,
    19, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0A, 0x0B, 0x1A, 0x1B, 0x0D, 0x0F, 0x0E, 0x10, 0x12, 0x13, 0x14, 0x1E, 0x1F,
     8, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x24, 0x25,
     8, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x13, 0x14,
    10, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x13, 0x14, 0x24, 0x25,
    12, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0D, 0x0F, 0x10, 0x12, 0x24, 0x25,
    10, 0x01, 0x02, 0x03, 0x04, 0x0A, 0x0B, 0x1C, 0x1D, 0x13, 0x14,
    12, 0x01, 0x02, 0x03, 0x04, 0x0A, 0x0B, 0x1C, 0x1D, 0x22, 0x23, 0x1E, 0x1F,
    14, 0x01, 0x02, 0x03, 0x04, 0x0A, 0x0B, 0x1C, 0x1D, 0x18, 0x19, 0x22, 0x23, 0x1E, 0x1F,
    16, 0x01, 0x02, 0x03, 0x04, 0x0A, 0x0B, 0x1C, 0x1D, 0x18, 0x19, 0x22, 0x23, 0x13, 0x14, 0x1E, 0x1F,
    24, 0x18, 0x19, 0x03, 0x20, 0x1C, 0x1D, 0x01, 0x02, 0x09, 0x21, 0x0A, 0x0B, 0x22, 0x23, 0x0E, 0x0C, 0x1E, 0x1F, 0x13, 0x14, 0x11, 0x15, 0x16, 0x17,
     7, 0x01, 0x02, 0x03, 0x0A, 0x0B, 0x1C, 0x1D,
     7, 0x01, 0x02, 0x03, 0x05, 0x06, 0x0D, 0x0F,
     7, 0x01, 0x02, 0x03, 0x05, 0x06, 0x13, 0x14,
     9, 0x01, 0x02, 0x03, 0x05, 0x06, 0x0D, 0x0F, 0x10, 0x12,
    11, 0x01, 0x02, 0x03, 0x0A, 0x0B, 0x1C, 0x1D, 0x22, 0x23, 0x1E, 0x1F,
     8, 0x01, 0x02, 0x05, 0x06, 0x0D, 0x0F, 0x10, 0x12,
    11, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0D, 0x0F, 0x10, 0x12, 0x0C,
    12, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0D, 0x0F, 0x10, 0x12, 0x0C, 0x0E,
    12, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x29, 0x2A, 0x0D, 0x0F, 0x10, 0x12,
    14, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x29, 0x2A, 0x0D, 0x0F, 0x10, 0x12, 0x0C, 0x0E,
    13, 0x03, 0x01, 0x02, 0x05, 0x06, 0x0E, 0x0D, 0x0F, 0x10, 0x12, 0x15, 0x2B, 0x2C,
     0
};
static const int8u audioPackFormat_2_audioChannelFormatIDRef_Table8[] = {
     1, 0x03,
     2, 0x01, 0x02,
     6, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
     8, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0D, 0x0F,
    10, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0D, 0x0F, 0x10, 0x12,
    11, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0D, 0x0F, 0x10, 0x12, 0x15,
    12, 0x03, 0x01, 0x02, 0x0D, 0x0F, 0x0A, 0x0B, 0x05, 0x06, 0x28, 0x20, 0x21,
    14, 0x01, 0x02, 0x03, 0x04, 0x0A, 0x0B, 0x05, 0x06, 0x0D, 0x0F, 0x10, 0x12, 0x24, 0x25,
    24, 0x01, 0x02, 0x03, 0x20, 0x05, 0x06, 0x07, 0x08, 0x09, 0x21, 0x0A, 0x0B, 0x0D, 0x0F, 0x0E, 0x0C, 0x10, 0x12, 0x13, 0x14, 0x11, 0x15, 0x16, 0x17,
     3, 0x01, 0x02, 0x03,
     4, 0x01, 0x02, 0x03, 0x09,
     5, 0x01, 0x02, 0x03, 0x05, 0x06,
     7, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x09,
     8, 0x07, 0x08, 0x03, 0x04, 0x05, 0x06, 0x01, 0x02,
     8, 0x01, 0x02, 0x03, 0x04, 0x0A, 0x0B, 0x05, 0x06,
    22, 0x01, 0x02, 0x03, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0D, 0x0F, 0x0E, 0x0C, 0x10, 0x12, 0x13, 0x14, 0x11, 0x15, 0x16, 0x17,
     1, 0x00,
     8, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x24, 0x25,
     8, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x13, 0x14,
    10, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x13, 0x14, 0x24, 0x25,
    12, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0D, 0x0F, 0x10, 0x12, 0x24, 0x25,
    10, 0x01, 0x02, 0x03, 0x04, 0x0A, 0x0B, 0x05, 0x06, 0x13, 0x14,
    12, 0x01, 0x02, 0x03, 0x04, 0x0A, 0x0B, 0x05, 0x06, 0x0D, 0x0F, 0x10, 0x12,
    14, 0x01, 0x02, 0x03, 0x04, 0x0A, 0x0B, 0x05, 0x06, 0x18, 0x19, 0x0D, 0x0F, 0x10, 0x12,
    16, 0x01, 0x02, 0x03, 0x04, 0x0A, 0x0B, 0x05, 0x06, 0x18, 0x19, 0x0D, 0x0F, 0x13, 0x14, 0x10, 0x12,
    24, 0x18, 0x19, 0x03, 0x20, 0x05, 0x06, 0x01, 0x02, 0x09, 0x21, 0x0A, 0x0B, 0x0D, 0x0F, 0x0E, 0x0C, 0x10, 0x12, 0x13, 0x14, 0x11, 0x15, 0x16, 0x17,
     7, 0x01, 0x02, 0x03, 0x0A, 0x0B, 0x05, 0x06,
     7, 0x01, 0x02, 0x03, 0x05, 0x06, 0x0D, 0x0F,
     7, 0x01, 0x02, 0x03, 0x05, 0x06, 0x13, 0x14,
     9, 0x01, 0x02, 0x03, 0x05, 0x06, 0x0D, 0x0F, 0x10, 0x12,
    11, 0x01, 0x02, 0x03, 0x0A, 0x0B, 0x05, 0x06, 0x0D, 0x0F, 0x10, 0x12,
     8, 0x01, 0x02, 0x05, 0x06, 0x0D, 0x0F, 0x10, 0x12,
    11, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0D, 0x0F, 0x10, 0x12, 0x0C,
    12, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0D, 0x0F, 0x10, 0x12, 0x0C, 0x0E,
    12, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x29, 0x2A, 0x0D, 0x0F, 0x10, 0x12,
    14, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x29, 0x2A, 0x0D, 0x0F, 0x10, 0x12, 0x0C, 0x0E,
    13, 0x03, 0x01, 0x02, 0x05, 0x06, 0x0E, 0x0D, 0x0F, 0x10, 0x12, 0x15, 0x16, 0x17,
     0
};

set<string> audioPackFormatID_2_audioChannelFormatIDRef (int16u audioPackFormatID_yyyy) {
    size_t Pos = 0;
    size_t i = 0;
    auto SearchedPos = audioPackFormatID_yyyy & 0xF7FF; // 0x8xx values
    auto Table = audioPackFormatID_yyyy - SearchedPos ? audioPackFormat_2_audioChannelFormatIDRef_Table8 : audioPackFormat_2_audioChannelFormatIDRef_Table;
    for (;;) {
        const auto Count = Table[i++];
        if (!Count) {
            return {};
        }
        if (++Pos != SearchedPos) {
            i += Count;
            continue;
        }
        if (!Table[i]) {
            return {};
        }
        set<string> Result;
        for (int j = 0; j < Count; j++) {
            Result.insert("AC_000100" + Hex2String(Table[i++], 2));
        }
        return Result;
    }
}

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

struct atmos_zone_Values {
    const char* Name;
    float       Values[6]; // X/Y/Z min/max
};
static const atmos_zone_Values Atmos_zone_Content[] = {
    { "ZM1"     , { -1      ,  1      , -1      , -0.41934, -0.499  ,  0.499   } },
    { "ZM2L"    , { -1      , -0.75806, -0.41934,  0.83871, -0.499  ,  0.499   } },
    { "ZM2R"    , {  0.75806,  1      , -0.41934,  0.83871, -0.499  ,  0.499   } },
    { "ZM3L"    , { -1      , -0.16129,  0.5    ,  1      , -0.499  ,  0.499   } },
    { "ZM3Lss"  , { -1      , -0.51611, -0.707  ,  0.49999, -0.499  ,  0.499   } },
    { "ZM3R"    , {  0.16129,  1      ,  0.5    ,  1      , -0.499  ,  0.499   } },
    { "ZM3Rss"  , {  0.51611,  1      , -0.707  ,  0.49999, -0.499  ,  0.499   } },
    { "ZM4"     , { -1      ,  1      , -1      ,  0.83871, -0.499  ,  0.499   } },
    { "ZM5"     , { -1      ,  1      ,  0.5    ,  1      , -0.499  ,  0.499   } },
    { "ZB"      , { -1      ,  1      , -1      ,  1      , -1      , -0.4995  } },
    { "ZT"      , { -1      ,  1      , -1      ,  1      ,  0.4995 ,  1       } },
};
const size_t Atmos_zone_Max = sizeof(Atmos_zone_Content) / sizeof(Atmos_zone_Content[0]);
size_t Atmos_zone_Pos(const string& Name, float32* Values) {
    for (size_t i = 0; i < Atmos_zone_Max; i++) {
        const auto& Test = Atmos_zone_Content[i];
        if (Test.Name == Name) {
            bool IsNok = false;
            for (size_t j = 0; j < 6; j++) {
                if (Test.Values[j] != Values[j]) {
                    IsNok = true;
                }
            }
            if (!IsNok) {
                return i;
            }
        }
    }
    return (size_t)-1;
}

string CraftName(const char* Name, bool ID = false) {
    if (Name)
        return (ID && !strcmp(Name, "Track")) ? "track" : ((Name && Name[0] < 'a' ? "audio" : "") + string(Name));
    else
        return "";
}

enum class E {
    Present0,
    Present1,
    Present2,
    Form,
    Permitted,
    Size
};
static const char* E_Strings[] = {
    "is not present",
    "is present",
    "count {} is not permitted, max is 1",
    "value \"{}\" is malformed",
    "value \"{}\" is not permitted",
};
static_assert(sizeof(E_Strings) / sizeof(E_Strings[0]) == (size_t)E::Size, "");

enum class e {
    Magic, // 0
    ErrorType,
    AttEle, // bit 7 = 1 means attribute
    Opt0,
    Max
};

struct Item_Struct {
    vector<string> Attributes;
    bitset<64> Attributes_Present;
    vector<vector<string> > Elements;
    vector<string> Errors[error_Type_Max][source_Max];

    void AddError(error_Type Type, const string& NewValue, source Source = Source_ADM)
    {
        auto& Error = Errors[Type][Source];
        if (Error.size() < 9) {
            Error.push_back(NewValue);
        }
        else if (Error.size() == 9) {
            if (!NewValue.empty() && NewValue[0] == ':') {
                auto Space = NewValue.find(' ');
                auto End = NewValue.rfind(':', Space);
                if (End != string::npos) {
                    Error.push_back(NewValue.substr(0, End + 1) + "[...]");
                }
            }
        }
    }
    void AddError(error_Type Error_Type, item Item_Type, size_t i, const string& NewValue, source Source = Source_ADM) {
        AddError(Error_Type, ':' + CraftName(item_Infos[Item_Type].Name) + (i != -1 ? to_string(i) : "") + NewValue, Source);
    }
    void AddError(error_Type Error_Type, size_t Item_Type, size_t i, const string& NewValue, source Source = Source_ADM) {
        AddError(Error_Type, (item)Item_Type, i, NewValue, Source);
    }
    void AddError(error_Type Error_Type, int8u AttEle, E Error_Value, int8u Opt0, source Source = Source_ADM) {
        string NewValue;
        NewValue.resize((size_t)e::Max);
        NewValue[(size_t)e::ErrorType] = (char)Error_Value;
        NewValue[(size_t)e::AttEle] = (char)AttEle;
        NewValue[(size_t)e::Opt0] = (char)Opt0;
        AddError(Error_Type, NewValue, Source);
    }
    void AddError(error_Type Error_Type, int8u AttEle, E Error_Value, file_adm_private* File_Adm_Private, const string& Opt0, source Source = Source_ADM);
};

struct Items_Struct {
    void Init(size_t Strings_Size_, size_t Elements_Size_) {
        Attributes_Size = Strings_Size_;
        Elements_Size =Elements_Size_;
    }

    Item_Struct& New()
    {
        Items.resize(Items.size() + 1);
        Item_Struct& Item = Items.back();
        Item.Attributes.resize(Attributes_Size);
        Item.Elements.resize(Elements_Size);
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

static void Apply_SubStreams(File__Analyze& F, const string& P_And_LinkedTo, Item_Struct& Source, size_t i, Items_Struct& Dest, bool NoError) {
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
            // Trying case insensitive, this is permitted by specs
            auto Start = ID.rfind('_');
            if (Start != string::npos) {
                auto ID_Up = ID;
                for (size_t i = Start; i < ID_Up.size(); i++) {
                    auto& Letter = ID_Up[i];
                    if (Letter >= 'A' && Letter <= 'F') {
                        Letter += 'a' - 'A';
                    }
                }
                for (size_t k = 0; k < Dest.Items.size(); k++) {
                    auto Target_Up = Dest.Items[k].Attributes[0];
                    for (size_t i = Start; i < Target_Up.size(); i++) {
                        auto& Letter = Target_Up[i];
                        if (Letter >= 'A' && Letter <= 'F') {
                            Letter += 'a' - 'A';
                        }
                    }
                    if (Target_Up == ID_Up) {
                        Pos = k;
                        break;
                    }
                }
            }
        }
        if (Pos == -1) {
            auto LinkedTo_Pos = P_And_LinkedTo.find(" LinkedTo_TrackUID_Pos");
            auto HasTransport = !F.Retrieve_Const(Stream_Audio, 0, "Transport0").empty();
            if (!NoError && HasTransport && LinkedTo_Pos != string::npos) { // TODO: better way to avoid common definitions
                string Message;
                if (LinkedTo_Pos) {
                    auto Sub_Pos = P_And_LinkedTo.rfind(' ', LinkedTo_Pos - 1);
                    if (Sub_Pos != string::npos) {
                        bool Target_Found = false;
                        auto Start = ID.rfind('_');
                        if (Start != string::npos) {
                            auto Not0 = ID.find_last_not_of('0');
                            Target_Found = Start == Not0; // Fake: ID 00000000 means not available, we don't raise an error for them
                        }
                        if (!Target_Found) {
                            Message += ":transportTrackFormat:audioTrack:audioTrackUIDRef:\"";
                            Message += ID;
                            Message += "\" is referenced but its description is missing";
                            Source.Errors[Warning][Source_ADM].push_back(Message);
                        }
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

class tfsxml
{
public:
    void Enter();
    void Leave();
    int Init(const void* const Buffer, size_t Buffer_Size);
    int Resynch(const string& Value);
    int NextElement();
    int Attribute();
    int Value();
    bool IsInit() { return IsInit_; }
    size_t Remain() { return p.len < 0 ? 0 : (size_t)p.len; }

private:
    tfsxml_string p = {};

public:
    tfsxml_string b = {}, v = {};

//private:
    string Code[16] = {};
    set<string> Extra[16];
    int8u Level = 0;
    int8u Level_Max = 0;
    bool IsInit_ = false;
    bool MustEnter = false;
    bool ParsingAttr = false;
    size_t* File_Buffer_Size_Hint_Pointer = {};
};

enum cartesion_test : int8_t {
    cartesian_unknown,
    cartesian_alreadyincoherent,
    cartesian_0,
};

struct channel_list {
    size_t              BlockPos;
    vector<string>      List;
};

struct channel_outputChannelFormatIDRef {
    size_t              BlockPos;
    string              outputChannelFormatIDRef;
};

class file_adm_private : public tfsxml
{
public:
    // In
    bool IsSub;

    // Out
    Items_Struct Items[item_Max];
    string Version_String;
    string Version_S_String;
    uint8_t Version = 0;
    uint8_t Version_S = -1; // S-ADM
    bool IsAtmos = false;
    schema Schema = Schema_Unknown;
    bool DolbyProfileCanNotBeVersion1 = false;
    bool IsPartial = false;
    bool IsLocalTimeReference = false;
    cartesion_test CartesianAlreadyNotCoherent = cartesian_unknown;
    TimeCode LastBlockFormatEnd;
    TimeCode LastBlockFormatEnd_S;
    vector<vector<channel_list>> ChannelFormat_Matrix_coefficients;
    vector<vector<channel_list>> ChannelFormat_Matrix_outputChannelFormatIDRefs;
    vector<string> coefficients;
    vector<profile_info> profileInfos;
    map<string, string> More;
    float32 FrameRate_Sum = 0;
    float32 FrameRate_Den = 0;
    vector<char> loudnessMetadata_Source;
    vector<size_t> ChannelFormat_BlockFormat_ReduceCount;
    string OldLocale;
    vector<string> Errors_Tips[error_Type_Max][source_Max];

    file_adm_private()
    {
        auto OldLocale_Temp = setlocale(LC_NUMERIC, nullptr);
        if (OldLocale_Temp && (*OldLocale_Temp != 'C' || *(OldLocale_Temp + 1))) {
            OldLocale = OldLocale_Temp;
            setlocale(LC_NUMERIC, "C");
        }

        # define STRUCTS(NAME) \
            Items[item_##NAME].Init(NAME##_Attribute_Max, NAME##_Element_Max);

        STRUCTS(root);
        STRUCTS(audioFormatExtended);
        STRUCTS(audioProgramme);
        STRUCTS(audioContent);
        STRUCTS(audioObject);
        STRUCTS(audioPackFormat);
        STRUCTS(audioChannelFormat);
        STRUCTS(audioTrackUID);
        STRUCTS(audioTrackFormat);
        STRUCTS(audioStreamFormat);
        STRUCTS(profileList);
        STRUCTS(tagList);
        STRUCTS(frameHeader);
        STRUCTS(frameFormat);
        STRUCTS(transportTrackFormat);
        STRUCTS(audioTrack);
        STRUCTS(changedIDs);
        STRUCTS(audioProgrammeLabel);
        STRUCTS(loudnessMetadata);
        STRUCTS(renderer);
        STRUCTS(audioProgrammeReferenceScreen);
        STRUCTS(screenCentrePosition);
        STRUCTS(screenWidth);
        STRUCTS(authoringInformation);
        STRUCTS(referenceLayout);
        STRUCTS(audioContentLabel);
        STRUCTS(dialogue);
        STRUCTS(audioObjectLabel);
        STRUCTS(audioComplementaryObjectGroupLabel);
        STRUCTS(audioObjectInteraction);
        STRUCTS(gainInteractionRange);
        STRUCTS(positionInteractionRange);
        STRUCTS(audioBlockFormat);
        STRUCTS(gain);
        STRUCTS(headphoneVirtualise);
        STRUCTS(position);
        STRUCTS(positionOffset);
        STRUCTS(channelLock);
        STRUCTS(objectDivergence);
        STRUCTS(jumpPosition);
        STRUCTS(zoneExclusion);
        STRUCTS(zone);
        STRUCTS(matrix);
        STRUCTS(coefficient);
        STRUCTS(alternativeValueSet);
        STRUCTS(frequency);
        STRUCTS(profile);
        STRUCTS(tagGroup);
        STRUCTS(tag);
    }

    ~file_adm_private()
    {
        if (!OldLocale.empty()) {
            setlocale(LC_NUMERIC, OldLocale.c_str());
        }
    }

    // Actions
    int parse(const void* const Buffer, size_t Buffer_Size);
    void clear();

    // Helpers
    void chna_Add(int32u Index, const string& TrackUID)
    {
        if (!Index || Index > 0x10000)
            return;
        if (Items[item_audioTrack].Items.empty())
            Items[item_audioTrack].Init(audioTrack_Attribute_Max, audioTrack_Element_Max);
        while (Items[item_audioTrack].Items.size() < Index)
            Items[item_audioTrack].New();
        Item_Struct& Item = Items[item_audioTrack].Items[Index - 1];
        Item.Elements[audioTrack_audioTrackUIDRef].push_back(TrackUID);
    }

    void Check_Attributes_NotPartOfSpecs(size_t Type, size_t Pos, const tfsxml_string& b, Item_Struct& Content, const char* SubElement = nullptr)
    {
        Content.AddError(Warning, ':' + CraftName(item_Infos[Type].Name) + to_string(Pos) + (SubElement ? (string(":") + SubElement) : "") + ":GeneralCompliance:\"" + tfsxml_decode(b) + "\" attribute is not part of specs");
    }

    void Check_Elements_NotPartOfSpecs(size_t Type, size_t Pos, const tfsxml_string& b, Item_Struct& Content)
    {
        Content.AddError(Warning, ':' + CraftName(item_Infos[Type].Name) + to_string(Pos) + ":GeneralCompliance:\"" + tfsxml_decode(b) + "\" element is not part of specs");
    }

//private:

    int coreMetadata();
    int format();
    int audioFormatExtended();
    int frameHeader();

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

int tfsxml::Resynch(const string& Value) {
    for (size_t i = 0; i < Level_Max; i++) {
        if (Code[i] != Value) {
            continue;
        }
        Level_Max = i + 1;
        IsInit_ = false;
        MustEnter = false;
        ParsingAttr = false;
        return 0;
    }
    return 1;
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
    auto Result = tfsxml_value(&p, &v);
    if (Result > 0) {
        ParsingAttr = true;
        Level = 0;
        return Result;
    }
    ParsingAttr = false;
    return Result;
}


//---------------------------------------------------------------------------
void Item_Struct::AddError(error_Type Error_Type, int8u AttEle, E Error_Value, file_adm_private* File_Adm_Private, const string& Opt0, source Source) {
    auto& Tips = File_Adm_Private->Errors_Tips[Error_Type][Source];
    auto Tips_Size = Tips.size();
    if (Tips_Size < 255) {
        Tips.push_back(Opt0);
    }
    else {
        Tips_Size = 255;
        Tips[255].clear();
    }
    AddError(Error_Type, AttEle, Error_Value, (int8u)Tips_Size, Source);
}

//---------------------------------------------------------------------------
static void MoveErrors (file_adm_private* File_Adm_Private, item Up_Type) {
    auto& Items = File_Adm_Private->Items;
    auto& Ups = Items[Up_Type].Items;
    auto& Up = Ups.back();
    size_t i = Ups.size() - 1;
    const auto Up_Infos_Ptr = item_Infos[Up_Type].Element_Infos;
    if (Up_Infos_Ptr) {
        const auto& Up_Infos = *Up_Infos_Ptr;
        for (size_t j = 0; j < Items[Up_Type].Elements_Size; j++) {
            const auto& Up_Info = Up_Infos[j];
            if (!Up_Info.LinkedItem) {
                continue;
            }
            const auto& Elements = Up.Elements[j];
            size_t Count = Elements.size();
            auto& Sources = Items[Up_Info.LinkedItem].Items;
            const auto k_Start = Sources.size() - Count;
            for (size_t k = k_Start; k < Sources.size(); k++) {
                auto& Item = Sources[k];
                for (size_t Error_Type = 0; Error_Type < error_Type_Max; Error_Type++) {
                    for (size_t Source = 0; Source < source_Max; Source++) {
                        for (const auto& Error : Item.Errors[Error_Type][Source]) {
                            if (!Error[0]) {
                                Up.AddError((error_Type)Error_Type, Error + (char)Up_Info.LinkedItem + (char)(k - k_Start), (source)Source);
                            }
                            else {
                                string Intermediate;
                                string Intermediate2 = ':' + CraftName(Up_Info.Name);
                                if (Error.rfind(Intermediate2, 0) || (Error.size() > Intermediate2.size() && !isdigit(Error[Intermediate2.size()]))) {
                                    Intermediate = Intermediate2 + to_string(k - k_Start);
                                }
                                Up.AddError((error_Type)Error_Type, ':' + CraftName(item_Infos[Up_Type].Name) + to_string(i) + Intermediate + Error, (source)Source);
                            }
                        }
                        Item.Errors[Error_Type][Source].clear();
                    }
                }
            }
        }
    }
}

//---------------------------------------------------------------------------
static type GetType(file_adm_private* File_Adm_Private, const item item_Type, size_t i);

//---------------------------------------------------------------------------
static bool CheckErrors_ID(file_adm_private* File_Adm_Private, const string& ID, const item_info& ID_Start, vector<Item_Struct>* Items = nullptr, const char* Sub = nullptr)
{
    auto BeginSize = strlen(ID_Start.ID_Begin);
    const auto Flags = ID_Start.ID_Flags;
    auto MiddleSize = (Flags[item_info::Flags_ID_YX] || Flags[item_info::Flags_ID_V]) ? 8 : (Flags[item_info::Flags_ID_W] ? 4 : 0);
    static const int end_size[] = { 0, 2, 4, 8 };
    auto EndSize = end_size[Flags[item_info::Flags_ID_Z1] + Flags[item_info::Flags_ID_Z2] * 2];
    if (!MiddleSize && !EndSize && ID.size() > BeginSize) {
        EndSize = ID.size() - BeginSize - 1;
    }
    if (ID.size() >= 14 && !strcmp(ID_Start.Name, "frameFormat")) {
        if (ID[11] != '_') {
            // Specs: "Any software that reads S-ADM files should tolerate both 8-digit and 11-digit variants of frameFormatID"
            MiddleSize += 3;
        }
        if (ID[ID.size() - 3] == '_') {
            // _zz is optional
            EndSize = 2;
        }
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
        if (Flags[item_info::Flags_ID_V]) {
            Middle.append(8, 'v');
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
        Items->back().Errors[Error][Source_ADM].push_back(Message);
    }
    return true;
}

//---------------------------------------------------------------------------
static void CheckErrors_ID_Additions(file_adm_private* File_Adm_Private, item item_Type, size_t i) {
    const bool IsAtmos = File_Adm_Private->IsAtmos;
    auto& Item = File_Adm_Private->Items[item_Type].Items[i];
    const auto& ID = Item.Attributes[item_Infos[item_Type].ID_Pos];
    if (IsAtmos && !CheckErrors_ID(File_Adm_Private, ID, item_Infos[item_Type])) {
        const auto Flags = item_Infos[item_Type].ID_Flags;
        if (Flags[item_info::Flags_ID_YX]) {
            auto xxxx = ID.substr(strlen(item_Infos[item_Type].ID_Begin) + 5, 4);
            if (xxxx[0] == '0' || xxxx == "1000") {
                Item.AddError(Error, item_Type, i, ':' + CraftName(item_Infos[item_Type].Name, true) + "ID:" + CraftName(item_Infos[item_Type].Name, true) + "ID attribute xxxx value \"" + xxxx + "\" is not permitted, permitted values are \"1001\" to \"FFFF\"", Source_Atmos_1_0);
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
                Item.AddError(Error, item_Type, i, ':' + CraftName(item_Infos[item_Type].Name, true) + "ID:" + CraftName(item_Infos[item_Type].Name, true) + "ID attribute wwww value \"" + wwww + "\" is not permitted" + (min_int == 0x100b ? " due to the Objects typed object" : "") + ", permitted values are \"" + min + "\" to \"" + max + "\"", Source_Atmos_1_0);
            }
        }
        if (Flags[item_info::Flags_ID_Z1] && !Flags[item_info::Flags_ID_Z2]) {
            if (ID[12] != '0' || ID[13] != '1') { // zz != 01
                Item.AddError(Error, item_Type, i, ':' + CraftName(item_Infos[item_Type].Name, true) + "ID:" + CraftName(item_Infos[item_Type].Name, true) + "ID attribute zz value \"" + ID.substr(12, 2) + "\" is not permitted, permitted value is \"01\"", Source_Atmos_1_0);
            }
        }
        if (Flags[item_info::Flags_ID_V]) {
            auto vvvvvvvv = ID.substr(strlen(item_Infos[item_Type].ID_Begin) + 1, 8);
            if (vvvvvvvv == "00000000") {
                Item.AddError(Error, item_Type, i, ":" + (item_Type == item_audioTrackUID ? "U" : ("" + CraftName(item_Infos[item_Type].Name, true))) + "ID:" + (item_Type == item_audioTrackUID ? "U" : ("" + CraftName(item_Infos[item_Type].Name))) + "ID attribute vvvvvvvv value \"" + vvvvvvvv + "\" is not permitted, permitted values are \"00000001\" to \"FFFFFFFF\"", Source_Atmos_1_0);
            }
        }
    }
};

//---------------------------------------------------------------------------
#pragma warning( push )
#pragma warning( disable : 26813 ) //false positive "Use 'bitwise and' to check if a flag is set."
static void CheckErrors_formatLabelDefinition(file_adm_private* File_Adm_Private, item item_Type, size_t i, const label_info& label_Info) {
    const bool IsAtmos = File_Adm_Private->IsAtmos;
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
            Item.AddError(Error, item_Type, i, ":formatLabel:formatLabel attribute value \"" + Label + "\" is not a valid form (yyyy form, yyyy being hexadecimal digits)");
        }
        else {
            formatLabel_Int = strtoul(Label.c_str(), nullptr, 16);
            if (formatLabel_Int && formatLabel_Int > List_Size) {
                Item.AddError(Warning, item_Type, i, ":formatLabel:formatLabel attribute value \"" + Label + "\" is not a known value");
            }
            else if (Definition_Present && List && formatLabel_Int < List_Size && Definition != List[formatLabel_Int - 1]) {
                Item.AddError(Error, item_Type, i, ":formatDefinition:formatDefinition attribute value \"" + Definition + "\" shall be \"" + List[formatLabel_Int - 1] + "\" in order to match the term corresponding to formatLabel attribute value \"" + Label + "\"");
            }
            if (IsAtmos && ((Style == Style_Format && formatLabel_Int != 1) || (Style == Style_Type && formatLabel_Int != 1 && formatLabel_Int != 3))) {
                Item.AddError(Error, item_Type, i, ":formatLabel:formatLabel attribute value \"" + Label + "\" is not permitted, permitted value is \"0001\"", Source_Atmos_1_0);
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
                    Item.AddError(Error, item_Type, i, ":formatLabel:formatLabel attribute value \"" + Label + "\" shall be \"" + Hex2String(j, 4) + "\" in order to match the term corresponding to formatDefinition attribute value \"" + Definition + "\"");
                }
                IsKnown = true;
                break;
            }
        }
        if (!IsKnown) {
            Item.AddError(Warning, item_Type, i, ":formatDefinition:formatDefinition attribute value \"" + Definition + "\" is not a known value");
        }
        if (IsAtmos && ((Style == Style_Format && Definition != List[0]) || (Style == Style_Type && Definition != List[0] && Definition != List[2]))) {
            Item.AddError(Error, item_Type, i, ":formatDefinition:formatDefinition attribute value \"" + Definition + "\" is not permitted, permitted value is \"PCM\"", Source_Atmos_1_0);
        }
    }

    if (label_Info.Label_Style == Style_Type && formatLabel_Int != (unsigned long)-1) {
        const auto& Item_ID = Item.Attributes[item_Infos[item_Type].ID_Pos];
        if (item_Infos[item_Type].ID_Flags[item_info::Flags_ID_YX] && !CheckErrors_ID(File_Adm_Private, Item_ID, item_Infos[item_Type])) {
            const auto Item_ID_yyyy = Item_ID.substr(strlen(item_Infos[item_Type].ID_Begin) + 1, 4);
            if ((Label_Present && Item_ID_yyyy != Label) || (Definition_Present && formatLabel_Int != strtoul(Item_ID_yyyy.c_str(), nullptr, 16))) {
                Item.AddError(Error, item_Type, i, ':' + CraftName(item_Infos[item_Type].Name, true) + "ID:" + CraftName(item_Infos[item_Type].Name, true) + "ID attribute yyyy value \"" + Item_ID_yyyy + "\" does not match " + (Label_Present ? ("formatLabel \"" + Label) : ("formatDefinition \"" + Definition)) + '\"');
            }
        }
    }
};
#pragma warning( pop )

//---------------------------------------------------------------------------
static void CheckErrors_Attributes(file_adm_private* File_Adm_Private, item Item_Type, const vector<size_t>& Attributes_Counts) {
    auto& Items = File_Adm_Private->Items[Item_Type].Items;
    const auto& Item_Info = item_Infos[Item_Type];
    const auto Attribute_Infos_Ptr = Item_Info.Attribute_Infos;
    if (!Attribute_Infos_Ptr) {
        return;
    }
    const auto& Attribute_Infos = *Attribute_Infos_Ptr;
    auto& Item = Items.back();
    size_t i = Items.size() - 1;
    auto& Attributes = Item.Attributes;
    auto& Attributes_Present = Item.Attributes_Present;
    for (size_t j = 0; j < Attributes.size(); j++) {
        const auto& Info = Attribute_Infos[j];
        switch (Attributes_Counts[j]) {
        case 0:
            if (!Info.Flags[Count0]) {
                Item.AddError(Error, 0x80 | (int8u)j, E::Present0, 0);
                break;
            }
            if (!Info.Flags[AdvSSE0]) {
                Item.AddError(Error, 0x80 | (int8u)j, E::Present0, 0, Source_AdvSSE_1);
            }
            if (!Info.Flags[Dolby0]) {
                Item.AddError(Error, 0x80 | (int8u)j, E::Present0, 0, Source_Atmos_1_0);
            }
            break;
        default:
            Item.AddError(Error, ':' + CraftName(item_Infos[Item_Type].Name) + to_string(i) + ":" + CraftName(Attribute_Infos[j].Name) + ":" + string(Attribute_Infos[j].Name) + " attribute shall be unique");
            [[fallthrough]];
        case 1:
        {
            Attributes_Present[j] = true;
            if (!Info.Flags[AdvSSE1]) {
                Item.AddError(Error, 0x80 | (int8u)j, E::Present1, 0, Source_AdvSSE_1);
            }
            if (!Info.Flags[Dolby1] && (/*File_Adm_Private->Schema != Schema_ebuCore_2014 ||*/ strcmp(Info.Name, "typeLabel") && strcmp(Info.Name, "typeDefinition"))) {
                Item.AddError(Error, 0x80 | (int8u)j, E::Present1, 0, Source_Atmos_1_0);
            }
            const auto& Attribute = Attributes[j];
            if (Attribute.size() > 64) {
                auto Attribute_Unicode = Ztring().From_UTF8(Attribute).To_Unicode();
                if (Attribute_Unicode.size() > 64) {
                    Item.AddError(Warning, Item_Type, i, ':' + string(Info.Name) + ':' + string(Info.Name) + " attribute value \"" + Attribute + "\" is long");
                }
            }
            else if (Attribute.empty()) {
                const auto& Attribute_Present = Item.Attributes_Present[j];
                if (Attribute_Present) {
                    Item.AddError(Warning, Item_Type, i, ':' + string(Info.Name) + ':' + string(Info.Name) + " attribute is present but empty");
                }
            }
        }
        }
    }
};

//---------------------------------------------------------------------------
static void CheckErrors_Elements(file_adm_private* File_Adm_Private, item Item_Type) {
    auto& Items = File_Adm_Private->Items[Item_Type].Items;
    const auto& Item_Info = item_Infos[Item_Type];
    auto& Item = Items.back();
    size_t i = Items.size() - 1;
    auto& Elements = Item.Elements;
    const element_items* Element_Infos_Ptr;
    if (Item_Type == item_audioBlockFormat) {
        static const element_items* audioBlockFormat_xxx_Elements[] = { (element_items*)audioBlockFormat_Elements, (element_items*)audioBlockFormat_DirectSpeakers_Elements, (element_items*)audioBlockFormat_Matrix_Elements, (element_items*)audioBlockFormat_Object_Elements };
        auto Type = GetType(File_Adm_Private, item_audioChannelFormat, File_Adm_Private->Items[item_audioChannelFormat].Items.size() - 1);
        if (Type >= size(audioBlockFormat_xxx_Elements)) {
            Type = Type_Unknown;
        }
        Element_Infos_Ptr = audioBlockFormat_xxx_Elements[Type];
    }
    else {
        Element_Infos_Ptr = Item_Info.Element_Infos;
    }
    const auto& Element_Infos = *Element_Infos_Ptr;
    for (size_t j = 0; j < Elements.size(); j++) {
        const auto& Element = Elements[j];
        const auto& Info = Element_Infos[j];
        const auto Element_Size = Element.size();
        switch (Element.size()) {
        case 0:
            if (!Info.Flags[Count0]) {
                Item.AddError(Error, j, E::Present0, 0);
                break;
            }
            else {
                if (!Info.Flags[AdvSSE0]) {
                    Item.AddError(Error, j, E::Present0, 0, Source_AdvSSE_1);
                }
                if (!Info.Flags[Dolby0]) {
                    Item.AddError(Error, j, E::Present0, 0, Source_Atmos_1_0);
                }
            }
            break;
        case 1:
            if (!Info.Flags[Count1]) {
                Item.AddError(Error, j, E::Present1, 0);
            }
            else {
                if (!Info.Flags[AdvSSE1]) {
                    Item.AddError(Error, j, E::Present1, 0, Source_AdvSSE_1);
                }
                if (!Info.Flags[Dolby1]) {
                    Item.AddError(Error, j, E::Present1, 0, Source_Atmos_1_0);
                }
            }
            break;
        default:
            if (!Info.Flags[Count2]) {
                Item.AddError(Error, j, E::Present2, (int8u)Element_Size);
                break;
            }
            else {
                if (!Info.Flags[AdvSSE2]) {
                    Item.AddError(Error, j, E::Present2, (int8u)Element_Size, Source_AdvSSE_1);
                }
                if (!Info.Flags[Dolby2]) {
                    Item.AddError(Error, j, E::Present2, (int8u)Element_Size, Source_Atmos_1_0);
                }
            }
        }
        for (size_t k = 0; k < Element.size(); k++) {
            const auto& Elem = Element[k];
            if (Elem.size() > 64) {
                auto Elem_Unicode = Ztring().From_UTF8(Elem).To_Unicode();
                if (Elem_Unicode.size() > 64) {
                    Item.AddError(Warning, Item_Type, i, ':' + string(Info.Name) + ':' + string(Info.Name) + " element value \"" + Elem + "\" is long");
                }
            }
            else if (Elem.empty() && Item_Type) {
                #define ITEM_ELEM(A,B) ((A << 8) | B)
                switch (ITEM_ELEM(static_cast<size_t>(Item_Type), j)) {
                case ITEM_ELEM(item_audioProgrammeReferenceScreen, audioProgrammeReferenceScreen_screenCentrePosition):
                case ITEM_ELEM(item_audioProgrammeReferenceScreen, audioProgrammeReferenceScreen_screenWidth):
                case ITEM_ELEM(item_audioBlockFormat, audioBlockFormat_headphoneVirtualise):
                #undef ITEM_ELEM
                    break;
                default:
                    Item.AddError(Warning, Item_Type, i, ':' + string(Info.Name) + ':' + string(Info.Name) + " element is present but empty");
                }
            }
        }
    }
};

//---------------------------------------------------------------------------
static void CheckErrors_Element_Target(file_adm_private* File_Adm_Private, item item_Type, size_t i, size_t Element_Pos, size_t Target_Type) {
    const bool IsAtmos = File_Adm_Private->IsAtmos;
    auto& Item = File_Adm_Private->Items[item_Type].Items[i];
    const auto& Element = Item.Elements[Element_Pos];
    const auto& Targets = File_Adm_Private->Items[Target_Type].Items;
    set<int16u> Ref_Int_NotKnown;
    for (const auto& TargetIDRef : Element) {
        if ((item_Type == item_audioTrackFormat && Target_Type == item_audioStreamFormat)
            || (item_Type == item_audioStreamFormat && Target_Type == item_audioChannelFormat)) {
            const auto& Item_ID = Item.Attributes[item_Infos[item_Type].ID_Pos];
            if (!CheckErrors_ID(File_Adm_Private, Item_ID, item_Infos[item_Type]) && !CheckErrors_ID(File_Adm_Private, TargetIDRef, item_Infos[Target_Type])) {
                const auto Item_ID_yyyyxxxx = Item_ID.substr(3, 8);
                const auto TargetIDRef_yyyyxxxx = TargetIDRef.substr(3, 8);
                if (Item_ID_yyyyxxxx != TargetIDRef_yyyyxxxx) {
                    Item.AddError(IsAtmos ? Error : Warning, item_Type, i, ':' + CraftName(item_Infos[Target_Type].Name) + "IDRef:" + CraftName(item_Infos[Target_Type].Name) + "IDRef subelement with yyyyxxxx value \"" + TargetIDRef_yyyyxxxx + "\" not same as " + CraftName(item_Infos[item_Type].Name, true) + "ID attribute yyyyxxxx value \"" + Item_ID_yyyyxxxx + "\"" + (IsAtmos ? ADM_Atmos_1_0 : ""));
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
            if (Ref_Int > item_Infos[Target_Type].MaxKnown && Ref_Int_NotKnown.find(Ref_Int) == Ref_Int_NotKnown.end()) {
                Ref_Int_NotKnown.insert(Ref_Int);
                Item.AddError(Warning, item_Type, i, ":" + CraftName(item_Infos[Target_Type].Name) + "IDRef:" + CraftName(item_Infos[Target_Type].Name) + "IDRef value \"" + TargetIDRef + "\" is not a known value");
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
                auto Start = TargetIDRef.rfind('_');
                if (Start != string::npos) {
                    auto Not0 = TargetIDRef.find_last_not_of('0');
                    if (Start == Not0) {
                        Target_Found = true; // Fake: ID 00000000 means not available, we don't raise an error for them
                    }
                    else {
                        // Trying case insensitive, this is permitted by specs
                        auto TargetIDRef_Up = TargetIDRef;
                        for (size_t i = Start; i < TargetIDRef_Up.size(); i++) {
                            auto& Letter = TargetIDRef_Up[i];
                            if (Letter >= 'A' && Letter <= 'F') {
                                Letter += 'a' - 'A';
                            }
                        }
                        for (const auto& Target : Targets) {
                            auto Target_Up = Target.Attributes[item_Infos[Target_Type].ID_Pos];
                            for (size_t i = Start; i < Target_Up.size(); i++) {
                                auto& Letter = Target_Up[i];
                                if (Letter >= 'A' && Letter <= 'F') {
                                    Letter += 'a' - 'A';
                                }
                            }
                            if (Target_Up == TargetIDRef_Up) {
                                Target_Found = true;
                            }
                        }
                    }
                }
            }
            if (!Target_Found) {
                Item.AddError(Error, item_Type, i, ':' + CraftName(item_Infos[Target_Type].Name) + "IDRef:" + CraftName(item_Infos[Target_Type].Name) + "IDRef value \"" + TargetIDRef + "\" shall match the " + CraftName(item_Infos[Target_Type].Name, true) + "ID attribute of an " + CraftName(item_Infos[Target_Type].Name) + " element");
            }
        }
    }
};

//---------------------------------------------------------------------------
void CheckError_Language(file_adm_private* File_Adm_Private, item Item_Type, size_t j) {
    auto& Items = File_Adm_Private->Items[Item_Type].Items;
    auto& Item = Items.back();
    auto& Language = Item.Attributes[j];
    if (Language.empty()) {
        return;
    }
    if (Language.size() < 2 || Language.size() > 3 || Language.find_first_not_of("abcdefghijklmnopqrstuvwxyz") != string::npos) {
        Item.AddError(Error, 0x80 | (int8u)j, E::Form, File_Adm_Private, Language);
    }
    else if (Language.size() != 3) {
        Item.AddError(Error, 0x80 | (int8u)j, E::Form, File_Adm_Private, Language, Source_AdvSSE_1);
    }
    else if (Language.size() != 2) {
        Item.AddError(Error, 0x80 | (int8u)j, E::Form, File_Adm_Private, Language, Source_Atmos_1_0);
    }
}

//---------------------------------------------------------------------------
TimeCode CheckError_Time(file_adm_private* File_Adm_Private, item Item_Type, size_t j) {
    auto& Items = File_Adm_Private->Items[Item_Type].Items;
    auto& Item = Items.back();
    if (!Item.Attributes_Present[j]) {
        return TimeCode();
    }
    auto& TimeInfo = Item.Attributes[j];
    TimeCode TimeInfo_TC = TimeInfo;
    if (!TimeInfo_TC.IsValid()) {
        Item.AddError(Error, 0x80 | (int8u)j, E::Form, File_Adm_Private, TimeInfo);
    }
    return TimeInfo_TC;
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
void loudnessMetadata_Check(file_adm_private* File_Adm_Private, item Item_Type) {
    auto& Items = File_Adm_Private->Items;
    auto& Programmes = Items[Item_Type].Items;
    auto& Programme = Programmes.back();
    auto& loudnessMetadatas = Items[item_loudnessMetadata].Items;
    auto& loudnessMetadata = loudnessMetadatas.back();

    size_t i = Programmes.size() - 1;
    size_t j = Programme.Elements[Item_Type == item_audioProgramme ? (size_t)audioProgramme_loudnessMetadata : (size_t)audioContent_loudnessMetadata].size() - 1;

    if (loudnessMetadata.Elements[loudnessMetadata_integratedLoudness].empty() && loudnessMetadata.Elements[loudnessMetadata_dialogueLoudness].empty())
        Programme.AddError(Error, string(":") + item_Infos[Item_Type].Name + to_string(i) + ":loudnessMetadata" + to_string(j) + ":integratedLoudness is not present", Source_AdvSSE_1);

    MoveErrors(File_Adm_Private, item_loudnessMetadata);
}

//---------------------------------------------------------------------------
void audioProgrammeReferenceScreen_Check(file_adm_private* File_Adm_Private) {
    auto& Items = File_Adm_Private->Items;
    auto& Programmes = Items[item_audioProgramme].Items;
    auto& Programme = Programmes.back();
    auto& ProgrammeReferenceScreens = Items[item_audioProgrammeReferenceScreen].Items;
    auto& ProgrammeReferenceScreen = ProgrammeReferenceScreens.back();

    size_t i = Programmes.size() - 1;
    size_t j = Programme.Elements[audioProgramme_audioProgrammeReferenceScreen].size() - 1;

    MoveErrors(File_Adm_Private, item_audioProgrammeReferenceScreen);
}

//---------------------------------------------------------------------------
void screenWidth_Check(file_adm_private* File_Adm_Private) {
    auto& Items = File_Adm_Private->Items;
    auto& Programmes = Items[item_audioProgramme].Items;
    auto& Programme = Programmes.back();
    auto& ProgrammeReferenceScreens = Items[item_audioProgrammeReferenceScreen].Items;
    auto& ProgrammeReferenceScreen = ProgrammeReferenceScreens.back();
    auto& screenWidths = Items[item_screenWidth].Items;
    auto& screenWidth = screenWidths.back();

    size_t i = Programmes.size() - 1;
    size_t j = Programme.Elements[audioProgramme_audioProgrammeReferenceScreen].size() - 1;
    size_t k = ProgrammeReferenceScreen.Elements[audioProgrammeReferenceScreen_screenWidth].size() - 1;

    if (screenWidth.Attributes_Present[screenWidth_X]) {
        const auto& Element = screenWidth.Attributes[screenWidth_X];
        char* End;
        auto Value = strtof(Element.c_str(), &End);
        if (End - Element.c_str() != Element.size()) {
            screenWidth.AddError(Error, ":X:X attribute value \"" + Element + "\" is malformed");
        }
        else if (Value < 0 || Value > 2) {
            screenWidth.AddError(Error, ":X:X attribute value \"" + Element + "\" is not permitted, permitted values are [0 - 2]");
        }
    }
}

//---------------------------------------------------------------------------
void authoringInformation_Check(file_adm_private* File_Adm_Private) {
    auto& Items = File_Adm_Private->Items;
    auto& Programmes = Items[item_audioProgramme].Items;
    auto& Programme = Programmes.back();
    auto& authoringInformations = Items[item_authoringInformation].Items;
    auto& authoringInformation = authoringInformations.back();

    size_t i = Programmes.size() - 1;
    size_t j = Programme.Elements[audioProgramme_authoringInformation].size() - 1;


    MoveErrors(File_Adm_Private, item_authoringInformation);
}

//---------------------------------------------------------------------------
void audioProgrammeLabel_Check(file_adm_private* File_Adm_Private) {
    CheckError_Language(File_Adm_Private, item_audioProgrammeLabel, audioProgrammeLabel_language);
}

//---------------------------------------------------------------------------
void audioProgramme_Check(file_adm_private* File_Adm_Private) {
    auto& Items = File_Adm_Private->Items;
    auto& Programmes = Items[item_audioProgramme].Items;
    auto& Programme = Programmes.back();

    size_t i = Programmes.size() - 1;

    CheckError_Language(File_Adm_Private, item_audioProgramme, audioProgramme_audioProgrammeLanguage);

    MoveErrors(File_Adm_Private, item_audioProgramme);

    if (!File_Adm_Private->IsAtmos && Programme.Attributes[audioProgramme_audioProgrammeName] == "Atmos_Master") {
        File_Adm_Private->IsAtmos = true;
    }
}

//---------------------------------------------------------------------------
void audioContent_Check(file_adm_private* File_Adm_Private) {
    auto& Items = File_Adm_Private->Items;
    auto& Contents = Items[item_audioContent].Items;
    auto& Content = Contents.back();

    size_t i = Contents.size() - 1;

    CheckError_Language(File_Adm_Private, item_audioContent, audioContent_audioContentLanguage);

    const auto& ID = Content.Attributes[audioContent_audioContentID];
    const auto& audioObjectIDRefs = Content.Elements[audioContent_audioObjectIDRef];
    if (!audioObjectIDRefs.empty() && !CheckErrors_ID(File_Adm_Private, ID, item_Infos[item_audioContent])) {
        for (size_t j = 0; j < audioObjectIDRefs.size(); j++) {
            const auto& audioObjectIDRef = audioObjectIDRefs[j];
            if (!CheckErrors_ID(File_Adm_Private, audioObjectIDRef, item_Infos[item_audioObject])) {
                const auto ID_wwww = ID.substr(4);
                const auto audioObjectIDRef_wwww = audioObjectIDRef.substr(3);
                if (ID_wwww != audioObjectIDRef_wwww) {
                    Content.AddError(Error, ":audioContent" + to_string(i) + ":audioObjectIDRef:audioObjectIDRef wwww value " + audioObjectIDRef_wwww + " is not same as audioContentID wwww value " + ID_wwww, Source_AdvSSE_1);
                }
            }
        }
    }

    auto& ContentLabels = Items[item_audioContentLabel].Items;
    auto audioContentLabel_Start = Items[item_audioContentLabel].Items.size() - Content.Elements[audioContent_audioContentLabel].size();
    set<string> PreviousLanguages;
    for (size_t k = audioContentLabel_Start; k < ContentLabels.size(); k++) {
        const auto& language = ContentLabels[k].Attributes[audioContentLabel_language];
        if (PreviousLanguages.find(language) != PreviousLanguages.end()) {
            ContentLabels[k].AddError(Error, item_audioContentLabel, k - audioContentLabel_Start, ":language:language attribute value \"" + language + "\" shall be unique");
        }
        else {
            PreviousLanguages.insert(language);
        }
    }

    MoveErrors(File_Adm_Private, item_audioContent);
}

//---------------------------------------------------------------------------
void audioContentLabel_Check(file_adm_private* File_Adm_Private) {
    CheckError_Language(File_Adm_Private, item_audioContentLabel, audioContentLabel_language);
}

//---------------------------------------------------------------------------
void audioObjectLabel_Check(file_adm_private* File_Adm_Private) {
    CheckError_Language(File_Adm_Private, item_audioObjectLabel, audioObjectLabel_language);
}

//---------------------------------------------------------------------------
void audioComplementaryObjectGroupLabel_Check(file_adm_private* File_Adm_Private) {
    CheckError_Language(File_Adm_Private, item_audioComplementaryObjectGroupLabel, audioComplementaryObjectGroupLabel_language);
}

//---------------------------------------------------------------------------
void audioObject_Check(file_adm_private* File_Adm_Private) {
    CheckError_Time(File_Adm_Private, item_audioObject, audioObject_start);
    CheckError_Time(File_Adm_Private, item_audioObject, audioObject_startTime);
    CheckError_Time(File_Adm_Private, item_audioObject, audioObject_duration);
}

//---------------------------------------------------------------------------
void audioObjectInteraction_Check(file_adm_private* File_Adm_Private) {
    auto& Items = File_Adm_Private->Items;
    auto& audioObjects = Items[item_audioObject].Items;
    auto& audioObject = audioObjects.back();
    auto& audioObjectInteractions = Items[item_audioObjectInteraction].Items;
    auto& audioObjectInteraction = audioObjectInteractions.back();

    size_t i = audioObjects.size() - 1;
    size_t j = audioObject.Elements[audioObject_audioObjectInteraction].size() - 1;

    MoveErrors(File_Adm_Private, item_audioObjectInteraction);
}

//---------------------------------------------------------------------------
void Object_Check(file_adm_private* File_Adm_Private) {
    auto& Items = File_Adm_Private->Items;
    auto& Objects = Items[item_audioObject].Items;
    auto& Object = Objects.back();

    size_t i = Objects.size() - 1;

    MoveErrors(File_Adm_Private, item_audioObject);
}

//---------------------------------------------------------------------------
void audioBlockFormat_Check(file_adm_private* File_Adm_Private) {
    auto& Items = File_Adm_Private->Items;
    auto& ChannelFormats = Items[item_audioChannelFormat].Items;
    auto& ChannelFormat = ChannelFormats.back();
    auto& BlockFormats = Items[item_audioBlockFormat].Items;
    auto& BlockFormat = BlockFormats.back();
    auto& Positions = Items[item_position].Items;
    auto& objectDivergences = Items[item_objectDivergence].Items;

    size_t audioBlockFormat_Count = ChannelFormat.Elements[audioChannelFormat_audioBlockFormat].size();

    size_t i = ChannelFormats.size() - 1;
    size_t j = audioBlockFormat_Count - 1;
    if (i < File_Adm_Private->ChannelFormat_BlockFormat_ReduceCount.size()) {
        j += File_Adm_Private->ChannelFormat_BlockFormat_ReduceCount[i];
    }
    size_t Position_Pos = Positions.size() - BlockFormat.Elements[audioBlockFormat_position].size();
    const auto objectDivergence_Pos = objectDivergences.size() - BlockFormat.Elements[audioBlockFormat_objectDivergence].size();

    const auto& ID = ChannelFormat.Attributes[audioChannelFormat_audioChannelFormatID];
    const auto Type = GetType(File_Adm_Private, item_audioChannelFormat, i);
    const auto ID_yyyyxxxx = CheckErrors_ID(File_Adm_Private, ID, item_Infos[item_audioChannelFormat]) ? string() : ID.substr(3, 8);

    bool initializeBlockActive = (int8s)File_Adm_Private->Version_S >= 0 && BlockFormat.Attributes[audioBlockFormat_initializeBlock] == "1";
    bool CheckNotPresent = Type == Type_Objects && !initializeBlockActive;
    auto GetStart = [&](size_t Element_Pos) -> TimeCode {
        if (BlockFormat.Attributes_Present[Element_Pos]) {
            if (File_Adm_Private->IsLocalTimeReference != (Element_Pos == audioBlockFormat_lstart) || initializeBlockActive) {
                BlockFormat.AddError(Error, 0x80 | (int8u)Element_Pos, E::Present1, 0);
            }
            else if (Type != Type_Objects) {
                BlockFormat.AddError(Error, 0x80 | (int8u)Element_Pos, E::Present1, 0, Source_AdvSSE_1);
            }
            auto Start_TC = CheckError_Time(File_Adm_Private, item_audioBlockFormat, Element_Pos);
            if (Start_TC.IsValid() && audioBlockFormat_Count == 1) {
                const auto& frameFormats = Items[item_frameFormat].Items;
                for (size_t k = 0; k < frameFormats.size(); k++) {
                    const auto& frameFormat = frameFormats[k];
                    TimeCode frameFormat_start_TC = frameFormat.Attributes[frameFormat_start];
                    if (frameFormat_start_TC.IsValid() && frameFormat_start_TC != Start_TC) {
                        BlockFormat.AddError(Error, string(1, ':') + (*item_Infos[item_audioBlockFormat].Attribute_Infos)[Element_Pos].Name + ':' + (*item_Infos[item_audioBlockFormat].Attribute_Infos)[Element_Pos].Name + " attribute value does not match the frameFormat start attribute value", Source_AdvSSE_1);
                    }
                }
                static const TimeCode Zero_TC(0, 0, 0, 0, 0);
                if (Start_TC != Zero_TC) {
                    BlockFormat.AddError(Error, string(1, ':') + (*item_Infos[item_audioBlockFormat].Attribute_Infos)[Element_Pos].Name + ':' + (*item_Infos[item_audioBlockFormat].Attribute_Infos)[Element_Pos].Name + " attribute value is not 0", Source_AdvSSE_1);
                }
            }
            return Start_TC;
        }
        else if (File_Adm_Private->IsLocalTimeReference == (Element_Pos == audioBlockFormat_lstart) && CheckNotPresent) {
            BlockFormat.AddError(Error, 0x80 | (int8u)Element_Pos, E::Present0, 0, Source_AdvSSE_1);
        }
        return {};
    };
    auto GetDuration = [&](size_t Element_Pos) -> TimeCode {
        if (BlockFormat.Attributes_Present[Element_Pos]) {
            if (File_Adm_Private->IsLocalTimeReference != (Element_Pos == audioBlockFormat_lduration) || initializeBlockActive) {
                BlockFormat.AddError(Error, 0x80 | (int8u)Element_Pos, E::Present1, 0);
            }
            else if (Type != Type_Objects) {
                BlockFormat.AddError(Error, 0x80 | (int8u)Element_Pos, E::Present1, 0, Source_AdvSSE_1);
            }
            auto Duration_TC = CheckError_Time(File_Adm_Private, item_audioBlockFormat, Element_Pos);
            if (Duration_TC.IsValid()) {
                const auto Duration_ms = Duration_TC.ToMilliseconds();
                if (Duration_ms && Duration_ms < 5) {
                    BlockFormat.AddError(Error, string(1, ':') + (*item_Infos[item_audioBlockFormat].Attribute_Infos)[Element_Pos].Name + ':' + (*item_Infos[item_audioBlockFormat].Attribute_Infos)[Element_Pos].Name + " attribute value is not permitted, permitted values are 0 or > 5 ms", Source_AdvSSE_1);
                }
            }
            return Duration_TC;
        }
        else if (File_Adm_Private->IsLocalTimeReference == (Element_Pos == audioBlockFormat_lduration) && CheckNotPresent) {
            BlockFormat.AddError(Error, 0x80 | (int8u)Element_Pos, E::Present0, 0, Source_AdvSSE_1);
        }
        return {};
    };
    auto CheckTimeOffset = [&](size_t Element_Pos, TimeCode& LastBlockFormatEnd, const TimeCode& Start, const TimeCode& Duration) {
        if (audioBlockFormat_Count != 1 && LastBlockFormatEnd.IsValid() && Start.IsValid() && LastBlockFormatEnd != Start) {
            BlockFormat.AddError(Error, string(1, ':') + (*item_Infos[item_audioBlockFormat].Attribute_Infos)[Element_Pos].Name + ':' + (*item_Infos[item_audioBlockFormat].Attribute_Infos)[Element_Pos].Name + " attribute value does not match the previous audioBlockFormat", Source_AdvSSE_1);
        }
        if (Start.IsValid() && Duration.IsValid()) {
            LastBlockFormatEnd = Start + Duration;
        }
        else {
            LastBlockFormatEnd = {};
        }
    };

    TimeCode rtime, duration, lstart, lduration;
    rtime = GetStart(audioBlockFormat_rtime);
    duration = GetDuration(audioBlockFormat_duration);
    if ((int8s)File_Adm_Private->Version_S < 0 && BlockFormat.Attributes_Present[audioBlockFormat_initializeBlock]) {
        BlockFormat.AddError(Error, 0x80 | (int8u)audioBlockFormat_initializeBlock, E::Present1, 0);
    }
    lstart = GetStart(audioBlockFormat_lstart);
    lduration = GetDuration(audioBlockFormat_lduration);
    CheckTimeOffset(audioBlockFormat_rtime, File_Adm_Private->LastBlockFormatEnd, rtime, duration);
    CheckTimeOffset(audioBlockFormat_lstart, File_Adm_Private->LastBlockFormatEnd_S, lstart, lduration);

    atmos_audioChannelFormatName ChannelAssignment = (atmos_audioChannelFormatName)-1;
    if (Type == Type_DirectSpeakers && ChannelFormat.Attributes_Present[audioChannelFormat_audioChannelFormatName]) {
        const auto& ChannelFormatName = ChannelFormat.Attributes[audioChannelFormat_audioChannelFormatName];
        ChannelAssignment = Atmos_audioChannelFormat_Pos(ChannelFormatName);
    }

    auto channelLocks = BlockFormat.Elements[audioBlockFormat_channelLock];
    for (size_t k = 0; k < channelLocks.size(); k++) {
        const auto& channelLock = channelLocks[k];
        if (channelLock != "0" && channelLock != "1") {
            BlockFormat.AddError(Error, ":channelLock" + to_string(k) + ":GeneralCompliance:channelLock element value " + channelLock + " is not permitted, permitted value are \"0\" or \"1\"");
        }
    }

    auto diffuses = BlockFormat.Elements[audioBlockFormat_diffuse];
    for (size_t k = 0; k < diffuses.size(); k++) {
        const auto& Element = diffuses[k];
        char* End;
        auto Value = strtof(Element.c_str(), &End);
        if (End - Element.c_str() != Element.size()) {
            BlockFormat.AddError(Error, ":diffuse" + to_string(k) + ":GeneralCompliance:diffuse element value \"" + Element + "\" is malformed");
        }
        else if (Value < 0 || Value > 1) {
            BlockFormat.AddError(Error, ":diffuse" + to_string(k) + ":GeneralCompliance:diffuse element value \"" + Element + "\" is not permitted, permitted values are [0 - 1]");
        }
        else if (Value != 0 && Value != 1) {
            BlockFormat.AddError(Error, ":diffuse" + to_string(k) + ":GeneralCompliance:diffuse element value \"" + Element + "\" is not permitted, permitted values are 0 or 1", Source_Atmos_1_0);
        }
    }

    string Gains_Not0;
    size_t Gains_Not0_Pos;
    auto gains = BlockFormat.Elements[audioBlockFormat_gain];
    for (size_t k = 0; k < gains.size(); k++) {
        const auto& Element = gains[k];
        char* End;
        auto Value = strtof(Element.c_str(), &End);
        if (End - Element.c_str() != Element.size()) {
            BlockFormat.AddError(Error, ":gain:gain attribute value \"" + Element + "\" is malformed");
        }
        else if (Value){
            Gains_Not0 = Element;
            Gains_Not0_Pos = k;
        }
    }

    auto importances = BlockFormat.Elements[audioBlockFormat_importance];
    for (size_t k = 0; k < importances.size(); k++) {
        const auto& Element = importances[k];
        char* End;
        auto Value = strtof(Element.c_str(), &End);
        if (End - Element.c_str() != Element.size()) {
            BlockFormat.AddError(Error, ":importance:importance element value \"" + Element + "\" is malformed");
        }
        else if (Value < 0 || Value > 10) {
            BlockFormat.AddError(Error, ":importance:importance element value \"" + Element + "\" is not permitted, permitted values are [0 - 10]");
        }
        else if (Value == 0) {
            if (BlockFormat.Elements[audioBlockFormat_gain].empty()) {
                BlockFormat.AddError(Error, ":gain element is not present", Source_Atmos_1_0);
            }
            else {
                BlockFormat.AddError(Error, ":gain" + to_string(Gains_Not0_Pos) + ":gain element value \"" + Gains_Not0 + "\" is not permitted, permitted value is 0 due to importance element value not 0", Source_Atmos_1_0);
            }
        }
    }

    auto jumpPositions = BlockFormat.Elements[audioBlockFormat_jumpPosition];
    for (size_t k = 0; k < jumpPositions.size(); k++) {
        const auto& jumpPosition = jumpPositions[k];
        if (jumpPosition != "0" && jumpPosition != "1") {
            BlockFormat.AddError(Error, ":jumpPosition" + to_string(k) + ":GeneralCompliance:jumpPosition element value " + jumpPosition + " is not permitted, permitted value are \"0\" or  \"1\"");
        }
    }
    switch (jumpPositions.size()) {
    case 0:
        switch (Type) {
        case Type_Objects:
            BlockFormat.AddError(Error, ":jumpPosition:jumpPosition element is not present", Source_Atmos_1_0);
            break;
        default:;
        }
        break;
    default:
        switch (Type) {
        case Type_DirectSpeakers:
            BlockFormat.AddError(Error, ":GeneralCompliance:jumpPosition subelement count " + to_string(jumpPositions.size()) + " is not permitted, max is 1", Source_Atmos_1_0);
            break;
        default:;
        }
        [[fallthrough]];
    case 1:
        switch (Type) {
        case Type_Objects: {
            const auto& jumpPositions_Items = Items[item_jumpPosition].Items;
            const auto& jumpPosition = jumpPositions_Items[jumpPositions_Items.size() - jumpPositions.size()];
            if (jumpPosition.Attributes_Present[jumpPosition_interpolationLength]) {
                const auto& interpolationLength = jumpPosition.Attributes[jumpPosition_interpolationLength];
                char* End;
                auto Value = strtof(interpolationLength.c_str(), &End);
                if (End - interpolationLength.c_str() != interpolationLength.size()) {
                    BlockFormat.AddError(Error, ":jumpPosition0:interpolationLength:interpolationLength attribute value \"" + interpolationLength + "\" is malformed");
                }
                else {
                    float ValidValue = j ? 0.005208 : 0;
                    if (Value != ValidValue) {
                        BlockFormat.AddError(Error, ":jumpPosition0:interpolationLength:interpolationLength attribute value \"" + interpolationLength + "\" is not permitted, permitted value is " + (j ? "0.005208" : "0"), Source_Atmos_1_0);
                    }
                }
            }
            else {
                BlockFormat.AddError(Error, ":jumpPosition0:interpolationLength attribute is not present", Source_Atmos_1_0);
            }
            break;
        }
        default:;
        }
    }

    if (Type == Type_DirectSpeakers) {
        if (BlockFormat.Attributes_Present[audioBlockFormat_rtime]) {
            BlockFormat.AddError(Error, ":rtime:rtime attribute is present", Source_Atmos_1_0);
        }
        if (BlockFormat.Attributes_Present[audioBlockFormat_duration]) {
            BlockFormat.AddError(Error, ":duration:duration attribute is present", Source_Atmos_1_0);
        }
    }
    if (Type == Type_Objects) {
        if (!BlockFormat.Attributes_Present[audioBlockFormat_rtime]) {
            BlockFormat.AddError(Error, ":rtime:rtime attribute is not present", Source_Atmos_1_0);
        }
        if (!BlockFormat.Attributes_Present[audioBlockFormat_duration]) {
            BlockFormat.AddError(Error, ":duration:duration attribute is not present", Source_Atmos_1_0);
        }
    }

    bool HasNoInit = (int8_t)File_Adm_Private->Version_S < 0 || BlockFormats[BlockFormats.size() - ChannelFormat.Elements[audioChannelFormat_audioBlockFormat].size()].Attributes[audioBlockFormat_initializeBlock] != "1";
    const auto& ID_Block = BlockFormat.Attributes[audioBlockFormat_audioBlockFormatID];
    if (!CheckErrors_ID(File_Adm_Private, ID_Block, item_Infos[item_audioBlockFormat])) {
        if (!CheckErrors_ID(File_Adm_Private, ID, item_Infos[item_audioChannelFormat])) {
            const auto ID_Block_yyyyxxxx = ID_Block.substr(3, 8);
            bool Found = ID_Block_yyyyxxxx == ID_yyyyxxxx;
            if (!Found) {
                // Trying case insensitive, this is permitted by specs
                auto ID_Block_yyyyxxxx_Up = ID_Block_yyyyxxxx;
                for (auto& Letter : ID_Block_yyyyxxxx_Up) {
                    if (Letter >= 'A' && Letter <= 'F') {
                        Letter += 'a' - 'A';
                    }
                }
                auto ID_yyyyxxxx_Up = ID_yyyyxxxx;
                for (auto& Letter : ID_yyyyxxxx_Up) {
                    if (Letter >= 'A' && Letter <= 'F') {
                        Letter += 'a' - 'A';
                    }
                }
                Found = ID_Block_yyyyxxxx_Up == ID_yyyyxxxx_Up;
            }
            if (!Found) {
                ChannelFormat.AddError(File_Adm_Private->IsAtmos ? Error : Warning, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat" + to_string(j) + ":audioBlockFormatID:audioBlockFormatID attribute with yyyyxxxx value \"" + ID_Block_yyyyxxxx + "\" not same as audioChannelFormatID attribute yyyyxxxx value \"" + ID_yyyyxxxx + "\"", File_Adm_Private->IsAtmos ? Source_Atmos_1_0 : Source_ADM);
            }
        }
        const auto ID_Block_zzzzzzzz = ID_Block.substr(12, 8);
        auto ID_Block_zzzzzzzz_Int = strtoul(ID_Block_zzzzzzzz.c_str(), nullptr, 16);
        auto ID_Block_zzzzzzzz_Expected = j + HasNoInit;
        if ((HasNoInit || !j) && ID_Block_zzzzzzzz_Int != ID_Block_zzzzzzzz_Expected) {
            const auto ID_Block_Expected = ID_Block.substr(0, 12) + Hex2String(ID_Block_zzzzzzzz_Expected, 8);
            BlockFormat.AddError(Error, ":audioBlockFormatID:audioBlockFormatID attribute value \"" + ID_Block + "\" shall be \"" + ID_Block_Expected + "\" in order to match the audioBlockFormat index");
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
                BlockFormat.AddError(Error, string(":GeneralConformance:") + (*item_Infos[item_audioBlockFormat].Element_Infos)[l].Name + " subelement is present");
            }
        }
        };
    atmos_audioChannelFormatName speakerLabel_ChannelAssignment = (atmos_audioChannelFormatName)-1;
    switch (Type) {
    case Type_DirectSpeakers: {
        const audioBlockFormat_Element BlockFormat_DirectSpeakers_List[] = { audioBlockFormat_cartesian, audioBlockFormat_speakerLabel, audioBlockFormat_position }; // TODO: cartesian is not in specs but lot of files have it
        List_Check((audioBlockFormat_Element*)&BlockFormat_DirectSpeakers_List, sizeof(BlockFormat_DirectSpeakers_List) / sizeof(*BlockFormat_DirectSpeakers_List));
        if (BlockFormat.Elements[audioBlockFormat_speakerLabel].empty()) {
            BlockFormat.AddError(Error, string(1, ':') + (*item_Infos[item_audioBlockFormat].Element_Infos)[audioBlockFormat_speakerLabel].Name + ':' + (*item_Infos[item_audioBlockFormat].Element_Infos)[audioBlockFormat_speakerLabel].Name + " element is not present");
        }
        else {
            const auto& speakerLabel = BlockFormat.Elements[audioBlockFormat_speakerLabel].back();
            speakerLabel_ChannelAssignment = Atmos_audioChannelFormat_Pos(speakerLabel, true);
            if (speakerLabel_ChannelAssignment == (atmos_audioChannelFormatName)-1) {
                BlockFormat.AddError(Error, ":speakerLabel:speakerLabel element value " + speakerLabel + " is not permitted", Source_Atmos_1_0);
            }
            else {
                if (ChannelAssignment != (atmos_audioChannelFormatName)-1 && speakerLabel_ChannelAssignment != ChannelAssignment) {
                    BlockFormat.AddError(Error, ":speakerLabel:speakerLabel element value " + speakerLabel + " is not permitted, permitted value is " + Atmos_audioChannelFormat_Content[ChannelAssignment].SpeakerLabel + "", Source_Atmos_1_0);
                }
            }
        }
        break;
    }
    default:;
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
            BlockFormat.AddError(Error, ":cartesian:cartesian element value \"" + cartesian + "\" is malformed");
            is_cartesian = (unsigned long)-1;
        }
        else if (is_cartesian > 1) {
            BlockFormat.AddError(Error, ":cartesian:cartesian element value \"" + cartesian + "\" is not permitted, permitted values are 0 or 1");
        }
        else {
            if (!is_cartesian) {
                BlockFormat.AddError(Error, ":cartesian:cartesian element value is not 1", Source_Atmos_1_0);
            }
        }
    }
    else {
        // Autodetection
        bitset<3> HasAED;
        bitset<3> HasXYZ;
        for (size_t k = 0; k < position_Count; k++) {
            auto& Position = Positions[Position_Pos + k];
            if (Position.Attributes_Present[position_coordinate] && !Position.Attributes_Present[position_bound]) {
                if (Position.Attributes[position_coordinate].size() == 1 && Position.Attributes[position_coordinate][0] >= 'X' && Position.Attributes[position_coordinate][0] <= 'Z') {
                    auto Pos = Position.Attributes[position_coordinate][0] - 'X';
                    if (HasXYZ[Pos]) {
                        BlockFormat.AddError(Error, ":coordinate:coordinate@position=\"" + Position.Attributes[position_coordinate] + "\" element is not unique");
                    }
                    else {
                        HasXYZ.set(Pos);
                    }
                }
                size_t Pos = 0;
                for (; Pos < 3 && Position.Attributes[position_coordinate] != Cartesian_0_Names[Pos]; Pos++) {
                }
                if (Pos < 3) {
                    if (HasAED[Pos]) {
                        BlockFormat.AddError(Error, ":coordinate:coordinate@position=\"" + Position.Attributes[position_coordinate] + "\" element is not unique");
                    }
                    else {
                        HasAED.set(Pos);
                    }
                }
            }
        }
        if (HasXYZ.count() > HasAED.count()) {
            if (Type == Type_Objects) {
                BlockFormat.AddError(Error, ":cartesian:cartesian element is not present");
            }
            else {
                BlockFormat.AddError(Error, ":cartesian:cartesian element is not present", Source_Atmos_1_0);
            }

            is_cartesian = 1;
        }
        else if (HasAED.any()) {
            is_cartesian = 0;
        }
        else {
            is_cartesian = (unsigned long)-1;
        }

        if (Type == Type_Objects) {
            if (is_cartesian == 0) {
                BlockFormat.AddError(Error, ":position:position element use polar attributes", Source_Atmos_1_0);
            }
        }
    }

    if (is_cartesian == 0) {
        bitset<3> HasAED;
        static float32 Cartesian_0_Limits[][2] = { {-180, 180}, { -90, 90}, {0, 1 } };
        for (size_t k = 0; k < position_Count; k++) {
            auto& Position = Positions[Position_Pos + k];
            if (Position.Attributes_Present[position_coordinate] && !Position.Attributes_Present[position_bound]) {
                size_t Pos = 0;
                for (; Pos < 3 && Position.Attributes[position_coordinate] != Cartesian_0_Names[Pos]; Pos++) {
                }
                if (Pos < 3) {
                    if (HasAED[Pos]) {
                        BlockFormat.AddError(Error, ":coordinate:coordinate@position=\"" + Position.Attributes[position_coordinate] + "\" element is not unique");
                    }
                    else {
                        HasAED.set(Pos);
                    }
                    const auto& Element = BlockFormat.Elements[audioBlockFormat_position][k];
                    char* End;
                    auto Value = strtof(Element.c_str(), &End);
                    if (End - Element.c_str() != Element.size()) {
                        BlockFormat.AddError(Error, ":position" + to_string(k) + ":GeneralCompliance:position element value \"" + BlockFormat.Elements[audioBlockFormat_position][k] + "\" is malformed");
                    }
                    else if (Value < Cartesian_0_Limits[Pos][0] || Value > Cartesian_0_Limits[Pos][1]) {
                        BlockFormat.AddError(Error, ":position" + to_string(k) + ":GeneralCompliance:position element value \"" + BlockFormat.Elements[audioBlockFormat_position][k] + "\" is not permitted, permitted values are [" + to_string(Cartesian_0_Limits[Pos][0]) + " - " + to_string(Cartesian_0_Limits[Pos][1]) + "]");
                    }
                }
                else {
                    BlockFormat.AddError(Error, ":position" + to_string(k) + ":coordinate:coordinate attribute \"" + Position.Attributes[position_coordinate] + "\" is present");
                }
            }
        }
        for (size_t k = 0; k < 2; k++) {
            if (!HasAED[k]) {
                BlockFormat.AddError(Error, ":position" + to_string(k) + ":coordinate:coordinate==\"" + Cartesian_0_Names[k] + "\" element is not present");
            }
        }

        for (size_t k = objectDivergence_Pos; k < objectDivergences.size(); k++) {
            auto& objectDivergence = objectDivergences[k];
            if (!objectDivergence.Attributes_Present[objectDivergence_azimuthRange]) {
                objectDivergence.AddError(Error, 0x80 | (int8u)objectDivergence_azimuthRange, E::Present0, 0);
            }
            if (objectDivergence.Attributes_Present[objectDivergence_positionRange]) {
                objectDivergence.AddError(Error, 0x80 | (int8u)objectDivergence_positionRange, E::Present1, 0);
            }
        }
    }
    if (is_cartesian == 1) {
        bitset<3> HasXYZ;
        float32 Values[3] = {};
        bool ValuesAreNok = false;
        for (size_t k = 0; k < position_Count; k++) {
            auto& Position = Positions[Position_Pos + k];
            if (Position.Attributes_Present[position_coordinate] && !Position.Attributes_Present[position_bound]) {
                if (Position.Attributes[position_coordinate].size() == 1 && Position.Attributes[position_coordinate][0] >= 'X' && Position.Attributes[position_coordinate][0] <= 'Z') {
                    auto Pos = Position.Attributes[position_coordinate][0] - 'X';
                    if (HasXYZ[Pos]) {
                        BlockFormat.AddError(Error, ":coordinate:coordinate@position=\"" + Position.Attributes[position_coordinate] + "\" element is not unique");
                    }
                    HasXYZ.set(Pos);
                    const auto& Element = BlockFormat.Elements[audioBlockFormat_position][k];
                    char* End;
                    Values[Pos] = strtof(Element.c_str(), &End);
                    if (End - Element.c_str() != Element.size()) {
                        ValuesAreNok = true;
                        BlockFormat.AddError(Error, string(1, ':') + (*item_Infos[item_audioBlockFormat].Element_Infos)[k].Name + ':' + (*item_Infos[item_audioBlockFormat].Element_Infos)[k].Name + " element value \"" + Element + "\" is malformed");
                    }
                    else if (Values[Pos] < -1 || Values[Pos] > 1) {
                        ValuesAreNok = true;
                        BlockFormat.AddError(Error, ":position" + to_string(k) + ":coordinate:coordinate=\"" + Position.Attributes[position_coordinate] + "\" element value \"" + BlockFormat.Elements[audioBlockFormat_position][k] + "\" is not permitted, permitted values are [-1,1]");
                    }
                }
                else {
                    BlockFormat.AddError(Error, ":position" + to_string(k) + ":coordinate:coordinate attribute \"" + Position.Attributes[position_coordinate] + "\" is present");
                }
            }
        }
        for (size_t l = 0; l < 2; l++) {
            if (!HasXYZ[l]) {
                BlockFormat.AddError(Error, ":position:coordinate:coordinate==\"" + string(1, 'X' + l) + "\" element is not present");
            }
        }
        if (!ValuesAreNok && speakerLabel_ChannelAssignment != (atmos_audioChannelFormatName)-1) {
            auto position_ChannelAssignment = Atmos_audioChannelFormat_Pos(Values[0], Values[1], Values[2], speakerLabel_ChannelAssignment);
            if (position_ChannelAssignment == (atmos_audioChannelFormatName)-1) {
                BlockFormat.AddError(Error, ":position:position@coordinate=\"X\" \"Y\" \"Z\" element value \"" + Ztring::ToZtring(Values[0], (Values[0] - (int)Values[0]) ? 5 : 0).To_UTF8() + "\" \"" + Ztring::ToZtring(Values[1], (Values[2] - (int)Values[2]) ? 5 : 0).To_UTF8() + "\" \"" + Ztring::ToZtring(Values[2], (Values[2] - (int)Values[2]) ? 5 : 0).To_UTF8() + "\" is not valid", Source_Atmos_1_0);
            }
            else if (position_ChannelAssignment != speakerLabel_ChannelAssignment) {
                BlockFormat.AddError(Error, ":position:position@coordinate=\"X\" \"Y\" \"Z\" element value \"" + Ztring::ToZtring(Values[0], (Values[0] - (int)Values[0]) ? 5 : 0).To_UTF8() + "\" \"" + Ztring::ToZtring(Values[1], (Values[2] - (int)Values[2]) ? 5 : 0).To_UTF8() + "\" \"" + Ztring::ToZtring(Values[2], (Values[2] - (int)Values[2]) ? 5 : 0).To_UTF8() + "\" so \"" + Atmos_audioChannelFormat_Content[position_ChannelAssignment].SpeakerLabel + "\" does not match corresponding speakerLabel element value \"" + Atmos_audioChannelFormat_Content[speakerLabel_ChannelAssignment].SpeakerLabel + "\"", Source_Atmos_1_0);
            }
        }

        for (size_t k = objectDivergence_Pos; k < objectDivergences.size(); k++) {
            auto& objectDivergence = objectDivergences[k];
            if (objectDivergence.Attributes_Present[objectDivergence_azimuthRange]) {
                objectDivergence.AddError(Error, 0x80 | (int8u)objectDivergence_azimuthRange, E::Present1, 0);
            }
            if (!objectDivergence.Attributes_Present[objectDivergence_positionRange]) {
                objectDivergence.AddError(Error, 0x80 | (int8u)objectDivergence_positionRange, E::Present0, 0);
            }
        }
    }
    if (!is_cartesian || is_cartesian == 1) {
        static_assert(audioBlockFormat_depth - audioBlockFormat_width == 2, "");
        auto Count = 0;
        string Value_Ref;
        bool AreNotSame = false;
        for (size_t k = audioBlockFormat_width; k <= audioBlockFormat_depth; k++) {
            const auto& Elements = BlockFormat.Elements[k];
            if (!Elements.empty()) {
                const auto& Element = Elements.back();
                if (Value_Ref.empty()) {
                    Value_Ref = Element;
                }
                else if (Element != Value_Ref && !AreNotSame) {
                    AreNotSame = true;
                    BlockFormat.AddError(Error, ":GeneralCompliance:width/height/depth element values are not same", Source_Atmos_1_0);
                }
                auto Max = (is_cartesian || k == audioBlockFormat_depth) ? 1 : 360;
                char* End;
                auto Value = strtof(Element.c_str(), &End);
                if (End - Element.c_str() < Element.size()) {
                    BlockFormat.AddError(Error, string(1, ':') + (*item_Infos[item_audioBlockFormat].Element_Infos)[k].Name + ':' + (*item_Infos[item_audioBlockFormat].Element_Infos)[k].Name + " element value \"" + Element + "\" is malformed");
                }
                else if (Value < 0 || Value > Max) {
                    BlockFormat.AddError(Error, string(1, ':') + (*item_Infos[item_audioBlockFormat].Element_Infos)[k].Name + ':' + (*item_Infos[item_audioBlockFormat].Element_Infos)[k].Name + " element value \"" + Element + "\" is not permitted, permitted values are [0," + to_string(Max) + "]");
                }
                Count++;
            }
        }
        if (Count && Count != 3) {
            BlockFormat.AddError(Error, ":GeneralCompliance:width/height/depth element values are not all present", Source_Atmos_1_0);
        }
    }

    if (Type == Type_Objects) {
        switch (File_Adm_Private->CartesianAlreadyNotCoherent) {
        case cartesian_unknown:
            if (is_cartesian <= 1) {
                File_Adm_Private->CartesianAlreadyNotCoherent = cartesion_test(cartesian_0 + is_cartesian);
            }
            break;
        case cartesian_alreadyincoherent:
            break;
        default:
            if (is_cartesian != File_Adm_Private->CartesianAlreadyNotCoherent - cartesian_0) {
                BlockFormat.AddError(Error, ":cartesian:cartesian element values are not consistant between audioBlockFormat elements", Source_AdvSSE_1);
                File_Adm_Private->CartesianAlreadyNotCoherent = cartesian_alreadyincoherent;
            }
        }
    }

    while (File_Adm_Private->ChannelFormat_Matrix_outputChannelFormatIDRefs.size() < ChannelFormats.size()) {
        File_Adm_Private->ChannelFormat_Matrix_outputChannelFormatIDRefs.push_back({});
    }
    const auto& outputChannelFormatIDRefs = BlockFormat.Elements[audioBlockFormat_outputChannelFormatIDRef];
    if (File_Adm_Private->ChannelFormat_Matrix_outputChannelFormatIDRefs.back().empty() || outputChannelFormatIDRefs != File_Adm_Private->ChannelFormat_Matrix_outputChannelFormatIDRefs.back().back().List) {
        if (File_Adm_Private->ChannelFormat_Matrix_outputChannelFormatIDRefs.back().size() < 0x100) { // Limit the count of different outputChannelFormatIDRefs stored
            File_Adm_Private->ChannelFormat_Matrix_outputChannelFormatIDRefs.back().push_back({ audioBlockFormat_Count - 1, outputChannelFormatIDRefs });
        }
    }

    MoveErrors(File_Adm_Private, item_audioBlockFormat);
}

//---------------------------------------------------------------------------
void gain_Check(file_adm_private* File_Adm_Private) {
    auto& Items = File_Adm_Private->Items;
    auto& ChannelFormats = Items[item_audioChannelFormat].Items;
    auto& ChannelFormat = ChannelFormats.back();
    auto& BlockFormats = Items[item_audioBlockFormat].Items;
    auto& BlockFormat = BlockFormats.back();
    auto& Gains = Items[item_gain].Items;
    auto& Gain = Gains.back();

    size_t i = ChannelFormats.size() - 1;
    size_t j = ChannelFormat.Elements[audioChannelFormat_audioBlockFormat].size() - 1;

    static const char* gain_List[] = { "linear", "dB" };
    int Unit = -1;
    if (Gain.Attributes_Present[gain_gainUnit]) {
        const auto& Type = Gain.Attributes[gain_gainUnit];
        size_t List_Size = sizeof(gain_List) / sizeof(gain_List[0]);
        for (size_t i = 0; i < List_Size; i++) {
            if (Type == gain_List[i]) {
                Unit = i;
            }
        }
        if (Unit < 0) {
            Gain.AddError(Error, 0x80 | (int8u)gain_gainUnit, E::Form, File_Adm_Private, Type);
        }
    }
    else {
        Unit = 0;
    }

    const auto& Value = BlockFormat.Elements[audioBlockFormat_gain].back();
    char* End;
    auto Float = strtod(Value.c_str(), &End);
    if (End - Value.c_str() != Value.size()) {
        BlockFormat.AddError(Error, audioBlockFormat_gain, E::Form, File_Adm_Private, Value);
    }
    else if (Unit >= 0) {
        static const double Linear_10dB = 3.1622776601683793319988935444327; // 10 ^ ( dB / 20)
        if ((Unit == 0 && Float > Linear_10dB) || (Unit == 1 && Float > 10)) {
            BlockFormat.AddError(Error, ":gain:gain element value \"" + Value + "\" is not permitted", Source_AdvSSE_1);
        }
    }
}

//---------------------------------------------------------------------------
void coefficient_Check(file_adm_private* File_Adm_Private) {
    auto& Items = File_Adm_Private->Items;
    auto& Matrixes = Items[item_matrix].Items;
    auto& Matrix = Matrixes.back();
    const auto Matrix_Coefficients = Matrix.Elements[matrix_coefficient];
    const auto& coefficient = Matrix_Coefficients.back();
    auto& Coefficients = Items[item_coefficient].Items;
    auto& Coefficient = Coefficients.back();

    static const char* gain_List[] = { "linear", "dB" };
    int Unit = -1;
    if (Coefficient.Attributes_Present[coefficient_gainUnit]) {
        const auto& Type = Coefficient.Attributes[coefficient_gainUnit];
        size_t List_Size = sizeof(gain_List) / sizeof(gain_List[0]);
        for (size_t i = 0; i < List_Size; i++) {
            if (Type == gain_List[i]) {
                Unit = i;
            }
        }
        if (Unit < 0) {
            Coefficient.AddError(Error, 0x80 | (int8u)coefficient_gainUnit, E::Form, File_Adm_Private, Type);
        }
    }
    else {
        Unit = 0;
    }

    if (Coefficient.Attributes_Present[coefficient_gain]) {
        const auto& Value = Coefficient.Attributes[coefficient_gain];
        char* End;
        auto Float = strtod(Value.c_str(), &End);
        if (End - Value.c_str() != Value.size()) {
            Coefficient.AddError(Error, coefficient_gain, E::Form, File_Adm_Private, Value);
        }
        else if (Unit >= 0) {
            static const int Linear_20dB = 10; // 10 ^ ( dB / 20)
            if ((Unit == 0 && Float > Linear_20dB) || (Unit == 1 && Float > 20)) {
                Coefficient.AddError(Error, ":gain:gain attribute value \"" + Value + "\" is not permitted", Source_AdvSSE_1);
            }
        }
    }

    if (find(File_Adm_Private->coefficients.begin(), File_Adm_Private->coefficients.end(), coefficient) != File_Adm_Private->coefficients.end()) {
        Matrix.AddError(Error, item_matrix, 0, ":coefficient" + to_string(Matrix_Coefficients.size() - 1) + ":coefficient value \"" + coefficient + "\" shall be unique");
    }
    else {
        File_Adm_Private->coefficients.push_back(coefficient);
    }
}

//---------------------------------------------------------------------------
void matrix_Check(file_adm_private* File_Adm_Private) {
    auto& Items = File_Adm_Private->Items;
    auto& ChannelFormats = Items[item_audioChannelFormat].Items;
    auto& ChannelFormat = ChannelFormats.back();
    auto& BlockFormats = Items[item_audioBlockFormat].Items;
    auto& BlockFormat = BlockFormats.back();
    auto& Matrixes = Items[item_matrix].Items;
    auto& Matrix = Matrixes.back();

    while (File_Adm_Private->ChannelFormat_Matrix_coefficients.size() < ChannelFormats.size()) {
        File_Adm_Private->ChannelFormat_Matrix_coefficients.push_back({});
    }
    if (!File_Adm_Private->ChannelFormat_Matrix_coefficients.back().empty() && File_Adm_Private->coefficients == File_Adm_Private->ChannelFormat_Matrix_coefficients.back().back().List) {
        File_Adm_Private->coefficients.clear();
        return;
    }

    auto audioBlockFormat_Count = ChannelFormat.Elements[audioChannelFormat_audioBlockFormat].size();
    if (File_Adm_Private->ChannelFormat_Matrix_coefficients.back().size() < 0x100) { // Limit the count of different coefficients stored
        File_Adm_Private->ChannelFormat_Matrix_coefficients.back().push_back({ audioBlockFormat_Count - 1, std::move(File_Adm_Private->coefficients) });
    }
}

//---------------------------------------------------------------------------
void objectDivergence_Check(file_adm_private* File_Adm_Private) {
    auto& Items = File_Adm_Private->Items;
    auto& ChannelFormats = Items[item_audioChannelFormat].Items;
    auto& ChannelFormat = ChannelFormats.back();
    auto& BlockFormats = Items[item_audioBlockFormat].Items;
    auto& BlockFormat = BlockFormats.back();
    auto& objectDivergence_Items = BlockFormat.Elements[audioBlockFormat_objectDivergence];
    auto& objectDivergences = Items[item_objectDivergence].Items;
    auto& objectDivergence = objectDivergences.back();

    const size_t k = objectDivergence_Items.size() - 1;

    if (objectDivergence.Attributes_Present[objectDivergence_azimuthRange])
    {
        const auto& Element = objectDivergence.Attributes[objectDivergence_azimuthRange];
        char* End;
        auto Value = strtof(Element.c_str(), &End);
        if (End - Element.c_str() != Element.size()) {
            objectDivergence.AddError(Error, 0x80 | (int8u)objectDivergence_azimuthRange, E::Form, File_Adm_Private, Element);
        }
        else if (Value < 0 || Value > 1) {
            objectDivergence.AddError(Error, ":azimuthRange" + to_string(k) + ":GeneralCompliance:azimuthRange attribute value \"" + Element + "\" is not permitted, permitted values are [0,180]");
        }
    }

    if (objectDivergence.Attributes_Present[objectDivergence_positionRange])
    {
        const auto& Element = objectDivergence.Attributes[objectDivergence_positionRange];
        char* End;
        auto Value = strtof(Element.c_str(), &End);
        if (End - Element.c_str() != Element.size()) {
            objectDivergence.AddError(Error, 0x80 | (int8u)objectDivergence_positionRange, E::Form, File_Adm_Private, Element);
        }
        else if (Value < 0 || Value > 1) {
            objectDivergence.AddError(Error, ":positionRange" + to_string(k) + ":GeneralCompliance:positionRange attribute value \"" + Element + "\" is not permitted, permitted values are [0,1]");
        }
    }

    const auto& Element = objectDivergence_Items.back();
    char* End;
    auto Value = strtof(Element.c_str(), &End);
    if (End - Element.c_str() != Element.size()) {
        BlockFormat.AddError(Error, (char)audioBlockFormat_objectDivergence, E::Form, File_Adm_Private, Element);
    }
    else if (Value < 0 || Value > 1) {
        BlockFormat.AddError(Error, ":objectDivergence" + to_string(k) + ":GeneralCompliance:objectDivergence element value \"" + Element + "\" is not permitted, permitted values are [0,1]");
    }
}

//---------------------------------------------------------------------------
void zoneExclusion_Check(file_adm_private* File_Adm_Private) {
    auto& Items = File_Adm_Private->Items;
    auto& ChannelFormats = Items[item_audioChannelFormat].Items;
    auto& ChannelFormat = ChannelFormats.back();
    auto& BlockFormats = Items[item_audioBlockFormat].Items;
    auto& BlockFormat = BlockFormats.back();
    auto& ZoneExclusions = Items[item_zoneExclusion].Items;
    auto& ZoneExclusion = ZoneExclusions.back();
    auto& Zones = Items[item_zone].Items;

    size_t i = ChannelFormats.size() - 1;
    size_t j = ChannelFormat.Elements[audioChannelFormat_audioBlockFormat].size() - 1;
    size_t zoneExclusion_Count = BlockFormat.Elements[audioBlockFormat_zoneExclusion].size();
    size_t zoneExclusion_Pos = ZoneExclusions.size() - zoneExclusion_Count;
    size_t zone_Count = ZoneExclusion.Elements[zoneExclusion_zone].size();
    size_t zone_Pos = Zones.size() - zone_Count;

    for (size_t k = 0; k < zoneExclusion_Count; k++) {
        size_t zone_Count = ZoneExclusion.Elements[zoneExclusion_zone].size();
        for (size_t l = 0; l < zone_Count; l++) {
            auto& Zone = Zones[zone_Pos + l];

            // Autodetection
            bitset<4> HasAED = 0;
            bitset<6> HasXYZ = 0;
            for (size_t m = 0; m < zone_Attribute_Max; m++) {
                if (Zone.Attributes_Present[m]) {
                    const auto& Info = (*item_Infos[item_zone].Attribute_Infos)[m];
                    if (Info.Name[3] >= 'X') {
                        auto Pos = m - zone_minX;
                        HasXYZ.set(Pos);
                    }
                    else {
                        auto Pos = m - zone_minElevation;
                        HasAED.set(Pos);
                    }
                }
            }
            unsigned long is_cartesian;
            if (HasXYZ.count() * 4 > HasAED.count() * 6) {
                is_cartesian = 1;
            }
            else if (HasAED.any()) {
                is_cartesian = 0;
            }
            else {
                is_cartesian = (unsigned long)-1;
            }

            if (is_cartesian == 1) {
                atmos_zone_Values Values;
                Values.Name = ZoneExclusion.Elements[zoneExclusion_zone][l].c_str();
                bool ValuesAreNok = false;
                for (size_t m = zone_minX; m <= zone_maxZ; m++) {
                    if (m >= zone_minX && m <= zone_maxZ) {
                        const auto& Attribute = Zone.Attributes[m - zone_minX];
                        char* End;
                        Values.Values[m] = strtof(Attribute.c_str(), &End);
                        if (End - Attribute.c_str() != Attribute.size()) {
                            ValuesAreNok = true;
                            const auto& Info = (*item_Infos[item_zone].Attribute_Infos)[m];
                            BlockFormat.AddError(Error, ":zoneExclusion" + to_string(k) + ":zone" + to_string(l) + ':' + Info.Name + ':' + Info.Name + " attribute value \"" + Attribute + "\" is malformed");
                        }
                        else if (Values.Values[m] < -1 || Values.Values[m] > 1) {
                            ValuesAreNok = true;
                            const auto& Info = (*item_Infos[item_zone].Attribute_Infos)[m];
                            BlockFormat.AddError(Error, ":zoneExclusion" + to_string(k) + ":zone" + to_string(l) + ':' + Info.Name + ':' + Info.Name + " attribute value \"" + Attribute + "\" is not permitted, permitted values are [-1,1]");
                        }
                    }
                }
                for (size_t m = 0; m < 6; m++) {
                    if (!HasXYZ[m]) {
                        const auto& Info = (*item_Infos[item_zone].Attribute_Infos)[m];
                        BlockFormat.AddError(Error, ":zoneExclusion" + to_string(k) + ":zone" + to_string(l) + ':' + Info.Name + ':' + Info.Name + " attribute is not present");
                    }
                }
                if (!ValuesAreNok) {
                    auto position_Channezone_PoslAssignment = Atmos_zone_Pos(Values.Name, Values.Values);
                    if (position_Channezone_PoslAssignment == -1) {
                        BlockFormat.AddError(Error, ":zoneExclusion" + to_string(k) + ":zone" + to_string(l) + ":GeneralCompliance:zone@minX maxX minY maxY minZ maxZ attributes and element value \"" + Zone.Attributes[zone_minX] + "\" \"" + Zone.Attributes[zone_maxX] + "\" \"" + Zone.Attributes[zone_minY] + "\" \"" + Zone.Attributes[zone_maxY] + "\" \"" + Zone.Attributes[zone_minZ] + "\" \"" + Zone.Attributes[zone_maxZ] + "\" \"" + ZoneExclusion.Elements[zoneExclusion_zone][l] + "\" is not valid", Source_Atmos_1_0);
                    }
                }
            }
        }
    }

    MoveErrors(File_Adm_Private, item_zoneExclusion);
}

//---------------------------------------------------------------------------
void audioChannelFormat_Check(file_adm_private* File_Adm_Private) {
    auto& Items = File_Adm_Private->Items;
    auto& ChannelFormats = Items[item_audioChannelFormat].Items;
    auto& ChannelFormat = ChannelFormats.back();

    if (!ChannelFormat.Elements[audioChannelFormat_audioBlockFormat].empty()) {
        auto& BlockFormats = Items[item_audioBlockFormat].Items;
        auto& BlockFormat = BlockFormats.back();

        auto CheckTimeOffset = [&](size_t Element_Pos, TimeCode& LastBlockFormatEnd) {
            if (!LastBlockFormatEnd.IsValid()) {
                return;
            }
            const auto& frameFormats = Items[item_frameFormat].Items;
            for (size_t k = 0; k < frameFormats.size(); k++) {
                const auto& frameFormat = frameFormats[k];
                TimeCode frameFormat_start_TC = frameFormat.Attributes[frameFormat_start];
                TimeCode frameFormat_duration_TC = frameFormat.Attributes[frameFormat_duration];
                if (frameFormat_start_TC.IsValid() && frameFormat_duration_TC.IsValid() && frameFormat_start_TC + frameFormat_duration_TC != LastBlockFormatEnd) {
                    BlockFormat.AddError(Error, string(1, ':') + (*item_Infos[item_audioBlockFormat].Attribute_Infos)[Element_Pos].Name + ':' + (*item_Infos[item_audioBlockFormat].Attribute_Infos)[Element_Pos].Name + " attribute value does not match the frameFormat start+duration attribute value", Source_AdvSSE_1);
                }
            }
        };

        CheckTimeOffset(audioBlockFormat_duration, File_Adm_Private->LastBlockFormatEnd);
        CheckTimeOffset(audioBlockFormat_lduration, File_Adm_Private->LastBlockFormatEnd_S);
    }

    MoveErrors(File_Adm_Private, item_audioChannelFormat);
}

//---------------------------------------------------------------------------
void profileList_Check(file_adm_private* File_Adm_Private) {
    auto& Items = File_Adm_Private->Items;
    auto& ProfileLists = Items[item_profileList].Items;
    auto& ProfileList = ProfileLists.back();
    auto& Profiles = Items[item_profile].Items;

    size_t i = ProfileLists.size() - 1;
    size_t Profile_Count = ProfileList.Elements[profileList_profile].size();
    size_t Profile_Start = Profiles.size() - Profile_Count;

    for (size_t j = 0; j < Profile_Count; j++) {
        const auto& Profile = Profiles[Profile_Start + j];
        for (size_t k = 0; k < j; k++) {
            const auto& Profile2 = Profiles[Profile_Start + k];
            if (Profile2.Attributes[profile_profileName] == Profile.Attributes[profile_profileName]
                && Profile2.Attributes[profile_profileVersion] == Profile.Attributes[profile_profileVersion]
                && Profile2.Attributes[profile_profileLevel] == Profile.Attributes[profile_profileLevel]) {
                ProfileList.AddError(Error, ":profileList" + to_string(i) + ":profile" + to_string(j) + ":GeneralCompliance:profile attributes are identical to a previous profile element", Source_AdvSSE_1);
                break;
            }
        }
    }
}

//---------------------------------------------------------------------------
void frameFormat_Check(file_adm_private* File_Adm_Private) {
    auto& Items = File_Adm_Private->Items;
    auto& FrameFormats = Items[item_frameFormat].Items;
    auto& FrameFormat = FrameFormats.back();

    CheckError_Time(File_Adm_Private, item_frameFormat, frameFormat_start);
    if (FrameFormat.Attributes_Present[frameFormat_start]) {
        auto& TimeCode_FirstFrame = File_Adm_Private->More["TimeCode_FirstFrame"];
        if (TimeCode_FirstFrame.empty()) {
            TimeCode_FirstFrame = FrameFormat.Attributes[frameFormat_start];
        }
    }

    auto Duration_TC = CheckError_Time(File_Adm_Private, item_frameFormat, frameFormat_duration);
    if (Duration_TC.IsValid()) {
        auto Duration = Duration_TC.ToSeconds();
        if (File_Adm_Private->FrameRate_Den < 5) // Handling of 1.001 frames rates
        {
            File_Adm_Private->FrameRate_Sum += Duration;
            File_Adm_Private->FrameRate_Den++;
        }
        Duration = File_Adm_Private->FrameRate_Sum / File_Adm_Private->FrameRate_Den;
        if (File_Adm_Private->IsSub)
            File_Adm_Private->More["FrameRate"] = Ztring().From_Number(1 / Duration).To_UTF8();
        else
            File_Adm_Private->More["Duration"] = Ztring().From_Number(Duration * 1000, 0).To_UTF8();
    }

    if (FrameFormat.Attributes_Present[frameFormat_type]) {
        auto& Metadata_Format_Type = File_Adm_Private->More["Metadata_Format_Type"];
        if (Metadata_Format_Type.empty()) {
            Metadata_Format_Type = FrameFormat.Attributes[frameFormat_type];
        }
        const auto& Type = FrameFormat.Attributes[frameFormat_type];
        static const char* type_List[] = {"header", "full", "divided", "intermediate", "all"};
        size_t type_List_Size = sizeof(type_List) / sizeof(type_List[0]);
        bool Found = false;
        for (size_t i = 0; i < type_List_Size; i++) {
            if (Type == type_List[i]) {
                if (i > 1) {
                    FrameFormat.AddError(Error, 0x80 | (int8u)frameFormat_type, E::Permitted, File_Adm_Private, Type, Source_AdvSSE_1);
                }
                Found = true;
            }
        }
        if (!Found) {
            FrameFormat.AddError(Error, 0x80 | (int8u)frameFormat_type, E::Form, File_Adm_Private, Type);
        }
    }

    static const char* timeReference_List[] = { "total", "local" };
    if (FrameFormat.Attributes_Present[frameFormat_timeReference]) {
        const auto& Type = FrameFormat.Attributes[frameFormat_timeReference];
        size_t timeReference_List_Size = sizeof(timeReference_List) / sizeof(timeReference_List[0]);
        bool Found = false;
        for (size_t i = 0; i < timeReference_List_Size; i++) {
            if (Type == timeReference_List[i]) {
                if (i != 1) {
                    FrameFormat.AddError(Error, 0x80 | (int8u)frameFormat_timeReference, E::Permitted, File_Adm_Private, Type, Source_AdvSSE_1);
                }
                Found = true;
            }
        }
        if (!Found) {
            FrameFormat.AddError(Error, 0x80 | (int8u)frameFormat_timeReference, E::Form, File_Adm_Private, Type);
        }
    }
    File_Adm_Private->IsLocalTimeReference = FrameFormat.Attributes[frameFormat_timeReference] == timeReference_List[1];

    if (FrameFormat.Attributes_Present[frameFormat_flowID]) {
        auto& FlowID = File_Adm_Private->More["FlowID"];
        if (FlowID.empty()) {
            FlowID = FrameFormat.Attributes[frameFormat_flowID];
        }
    }
}

//---------------------------------------------------------------------------
void audioTrack_Check(file_adm_private* File_Adm_Private) {
}

//---------------------------------------------------------------------------
void transportTrackFormat_Check(file_adm_private* File_Adm_Private) {
    auto& Items = File_Adm_Private->Items;
    auto& TransportTrackFormats = Items[item_transportTrackFormat].Items;
    auto& TransportTrackFormat = TransportTrackFormats.back();
    auto& Tracks = Items[item_audioTrack].Items;

    size_t i = TransportTrackFormats.size() - 1;

    if (TransportTrackFormat.Attributes_Present[transportTrackFormat_numIDs]) {
        const auto& numIDs = TransportTrackFormat.Attributes[transportTrackFormat_numIDs];
        if (TransportTrackFormat.Attributes_Present[transportTrackFormat_numTracks]) {
            const auto& numTracks = TransportTrackFormat.Attributes[transportTrackFormat_numTracks];
            if (numTracks != numIDs) {
                TransportTrackFormat.AddError(Error, ":transportTrackFormat" + to_string(i) + ":numIDs:numIDs attribute value \"" + numIDs + "\" is not same as numTracks attribute value \"" + numTracks + "\"", Source_AdvSSE_1);
            }
        }
    }
    // Check on numIDs == 0 is made during the other check

    const auto& Track_Items = TransportTrackFormat.Elements[transportTrackFormat_audioTrack];
    set<string> PreviousIDs;
    set<string> PreviousaudioTrackUIDRef_Itemss;
    const auto Tracks_Pos = Tracks.size() - Track_Items.size();
    for (size_t j = 0; j < Track_Items.size();  j++) {
        auto& Track = Tracks[Tracks_Pos + j];
        if (Track.Attributes_Present[audioTrack_trackID]) {
            const auto& ID = Track.Attributes[audioTrack_trackID];
            if (PreviousIDs.find(ID) != PreviousIDs.end()) {
                Track.AddError(Error, item_audioTrack, j, ':' + CraftName(item_Infos[item_audioTrack].Name, true) + "ID:" + CraftName(item_Infos[item_audioTrack].Name, true) + "ID value \"" + ID + "\" shall be unique");
            }
            else {
                PreviousIDs.insert(ID);
                char* End;
                auto ID_Int = strtoul(ID.c_str(), &End, 10);
                if (to_string(ID_Int) != ID) {
                    Track.AddError(Error, item_audioTrack, j, ':' + CraftName(item_Infos[item_audioTrack].Name, true) + "ID:" + CraftName(item_Infos[item_audioTrack].Name, true) + "ID attribute value \"" + ID + "\" is malformed");
                }
                else if (!ID_Int) {
                    Track.AddError(Error, item_audioTrack, j, ':' + CraftName(item_Infos[item_audioTrack].Name, true) + "ID:" + CraftName(item_Infos[item_audioTrack].Name, true) + "ID attribute value \"" + ID + "\" is not permitted, permitted values are [1...]");
                }
            }
        }
        const auto& audioTrackUIDRef_Items = Track.Elements[audioTrack_audioTrackUIDRef];
        for (size_t j = 0; j < audioTrackUIDRef_Items.size(); j++) {
            auto& ID = audioTrackUIDRef_Items[j];
            if (ID == "ATU_00000000") {
                auto& TransportTrackFormats = Items[item_transportTrackFormat].Items;
                auto& TransportTrackFormat = TransportTrackFormats.back();
                const auto& Track_Items = TransportTrackFormat.Elements[transportTrackFormat_audioTrack];
                const auto Tracks_Pos = Tracks.size() - Track_Items.size();
                Track.AddError(Error, item_audioTrack, i - Tracks_Pos, ":audioTrackUIDRef:audioTrackUIDRef element value \"ATU_00000000\" is not permitted");
            }
            else {
                if (PreviousaudioTrackUIDRef_Itemss.find(ID) != PreviousaudioTrackUIDRef_Itemss.end()) {
                    Track.AddError(Error, item_audioTrack, j, ':' + CraftName(item_Infos[item_audioTrack].Name, true) + "ID:" + CraftName(item_Infos[item_audioTrack].Name, true) + "ID value \"" + ID + "\" shall be unique");
                }
                else {
                    PreviousaudioTrackUIDRef_Itemss.insert(ID);
                }
            }
        }
    }

    if (TransportTrackFormat.Attributes_Present[transportTrackFormat_numIDs]) {
        const auto& numIDs = TransportTrackFormat.Attributes[transportTrackFormat_numIDs];
        auto numIDs_Int = strtoul(numIDs.c_str(), nullptr, 10);
        if (to_string(numIDs_Int) != numIDs) {
            TransportTrackFormat.AddError(Error, 0x80 | (int8u)transportTrackFormat_numIDs, E::Form, File_Adm_Private, numIDs);
        }
    }

    MoveErrors(File_Adm_Private, item_transportTrackFormat);
}

//---------------------------------------------------------------------------
int file_adm_private::parse(const void* const Buffer, size_t Buffer_Size)
{
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
   
    // Element Start
    #define ELEMENT_s(UP,NAME,CALL) \
        else if (!tfsxml_strcmp_charp(b, #NAME)) \
        { \
            auto& Elements_Up = UP##_Content.Elements[UP##_##NAME]; \
            auto& Items_##NAME = Items[item_##NAME]; \
            if (IsInit()) { \
                CALL; \
                if (item_Infos[item_##NAME].Element_Infos) { \
                    Elements_Up.emplace_back(string(1, '\0')); \
                } \
            } \
            Item_Struct& NAME##_Content = IsInit() ? Items_##NAME.New() : Items_##NAME.Items.back(); \
            if (IsInit()) { \
                Attributes_Counts[Level].clear(); \
                Attributes_Counts[Level].resize(NAME##_Attribute_Max); \
                Extra[Level].clear(); \
            } \
            XML_ATTR_START \

    #define ELEMENT_S(UP,NAME) \
        ELEMENT_s(UP,NAME,{}) \

    // Element Middle
    #define ELEMENT_M(NAME) \
                else { \
                    Check_Attributes_NotPartOfSpecs(item_##NAME, Items[item_##NAME].Items.size() - 1, b, NAME##_Content); \
                } \
            XML_ATTR_END \
            CheckErrors_Attributes(this, item_##NAME, Attributes_Counts[Level]); \
            if (item_Infos[item_##NAME].Element_Infos) { \
                XML_ELEM_START \

    #define ELEMENT_e(NAME,UP,CALL) \
                else { \
                    string Value = tfsxml_decode(b); \
                    if (Extra[Level].find(Value) == Extra[Level].end()) { \
                        Extra[Level].insert(Value); \
                        Check_Elements_NotPartOfSpecs(item_##NAME, Items[item_##NAME].Items.size() - 1, b, NAME##_Content); \
                    } \
                } \
                XML_ELEM_END \
                CheckErrors_Elements(this, item_##NAME); \
            } \
            else { \
                XML_VALUE \
                Elements_Up.emplace_back(tfsxml_decode(v)); \
            } \
            CALL; \
        } \

    // Element End
    #define ELEMENT_E(NAME,UP) \
        ELEMENT_e(NAME,UP,{}) \

    // Attribute
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
            if (!tfsxml_decode(v).empty()) { \
                NAME##_Content.Attributes[NAME##_##ATTR].assign(string(1, '\0')); \
            } \
            Attributes_Counts[Level][NAME##_##ATTR]++; \
        } \

    // Element
    #define ELEMENT__(NAME,ELEM) \
        else if (!tfsxml_strcmp_charp(b, #ELEM)) { \
            XML_ATTR_START \
                else { \
                    Check_Attributes_NotPartOfSpecs(item_##NAME, Items[item_##NAME].Items.size() - 1, b, NAME##_Content, #ELEM); \
                } \
            XML_ATTR_END \
            XML_VALUE \
            if (NAME##_Content.Elements[NAME##_##ELEM].empty()) { \
                const auto Element_Infos = item_Infos[item_##NAME].Element_Infos; \
                if (Element_Infos) { \
                    const auto& Info = (*Element_Infos)[NAME##_##ELEM]; \
                    if (Version < Info.Min() || Version > Info.Max()) { \
                        Check_Elements_NotPartOfSpecs(item_##NAME, Items[item_##NAME].Items.size() - 1, b, NAME##_Content); \
                    } \
                } \
            } \
            NAME##_Content.Elements[NAME##_##ELEM].push_back(tfsxml_decode(v)); \
        } \

    #define ELEMENT_I(NAME,ELEM) \
        else if (!tfsxml_strcmp_charp(b, #ELEM)) { \
            XML_ATTR_START \
                else { \
                    Check_Attributes_NotPartOfSpecs(item_##NAME, Items[item_##NAME].Items.size() - 1, b, NAME##_Content, #ELEM); \
                } \
            XML_ATTR_END \
            XML_VALUE \
            if (v.len) { \
                NAME##_Content.Elements[NAME##_##ELEM].push_back(string(1, '\0')); \
            } \
        } \

    if (auto Result = Init(Buffer, Buffer_Size)) {
        return Result;
    }

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
                clear();
                Version_S = 0;
            }
            XML_ATTR_START
                if (!tfsxml_strcmp_charp(b, "version")) {
                    if (!tfsxml_strncmp_charp(v, "ITU-R_BS.2125-", 14) && v.len == 15 && v.buf[14] >= '0' && v.buf[14] <= '9') {
                        Version_S = v.buf[14] - '0';
                    }
                    else {
                        Version_S_String = tfsxml_decode(v);
                        Version_S = -1;
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
    bool IsFromChna = Items[item_transportTrackFormat].Items.empty() && !Items[item_audioTrack].Items.empty();
    for (size_t i = 0; i < item_Max; i++) {
        if (IsFromChna && i == item_audioTrack) {
            continue;
        }
        auto& Item = Items[i];
        Item.Items.clear();
    }
    Version_String.clear();
    Version_S_String.clear();
    Version = 0;
    Version_S = -1;
    IsAtmos = false;
    Schema = Schema_Unknown;
    DolbyProfileCanNotBeVersion1 = false;
    IsLocalTimeReference = false;
    IsPartial = false;
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
    if (IsInit()) {
        clear();
        Items[item_root].New();
    }

    XML_BEGIN
    XML_ELEM_START
    else if (!tfsxml_strcmp_charp(b, "audioFormatCustom")) {
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
    else if (!tfsxml_strcmp_charp(b, "audioFormatExtended")) {
        XML_SUB(audioFormatExtended());
    }
    else if (!tfsxml_strcmp_charp(b, "frameHeader")) {
        XML_SUB(frameHeader());
    }
    XML_ELEM_END
    XML_END
}

static const size_t BlockFormat_MaxParsed = 2;

int file_adm_private::audioFormatExtended()
{
    if (IsInit()) {
        if (Items[item_root].Items.empty()) {
            Items[item_root].New();
        }
        Items[item_root].Items.back().Elements[root_audioFormatExtended].push_back({});
        Items[item_audioFormatExtended].New();
    }

    auto audioBlockformat_Reduce = [&]() {
        auto& ChannelFormats = Items[item_audioChannelFormat].Items;
        auto& ChannelFormat = ChannelFormats.back();
        auto& ChannelFormat_BlockFormats = ChannelFormat.Elements[audioChannelFormat_audioBlockFormat];
        if (ChannelFormat_BlockFormats.size() < BlockFormat_MaxParsed) {
            return;
        }

        // We keep track of only few first blocks + the last one, while we keep doing conformance check on all blocks
        auto& BlockFormats = Items[item_audioBlockFormat].Items;

        auto& BlockFormat_gains = BlockFormats.back().Elements[audioBlockFormat_gain];
        auto& Gains = Items[item_gain].Items;
        Gains.resize(BlockFormat_gains.size());

        auto& BlockFormat_headphoneVirtualises = BlockFormats.back().Elements[audioBlockFormat_headphoneVirtualise];
        auto& HeadphoneVirtualises = Items[item_headphoneVirtualise].Items;
        HeadphoneVirtualises.resize(BlockFormat_headphoneVirtualises.size());

        auto& BlockFormat_positions = BlockFormats.back().Elements[audioBlockFormat_position];
        auto& Positions = Items[item_position].Items;
        Positions.resize(BlockFormat_positions.size());

        auto& BlockFormat_jumpPosition = BlockFormats.back().Elements[audioBlockFormat_jumpPosition];
        auto& JumpPositions = Items[item_jumpPosition].Items;
        JumpPositions.resize(BlockFormat_jumpPosition.size());

        auto& BlockFormat_channelLock = BlockFormats.back().Elements[audioBlockFormat_channelLock];
        auto& ChannelLocks = Items[item_channelLock].Items;
        ChannelLocks.resize(BlockFormat_channelLock.size());

        auto& BlockFormat_objectDivergence = BlockFormats.back().Elements[audioBlockFormat_objectDivergence];
        auto& ObjectDivergences = Items[item_objectDivergence].Items;
        ObjectDivergences.resize(BlockFormat_objectDivergence.size());

        auto& BlockFormat_zoneExclusions = BlockFormats.back().Elements[audioBlockFormat_zoneExclusion];
        auto& ZoneExclusions = Items[item_zoneExclusion].Items;
        auto& Zones = Items[item_zone].Items;
        size_t Zones_ToRemove = 0;
        for (size_t i = ZoneExclusions.size() - BlockFormat_zoneExclusions.size(); i < ZoneExclusions.size(); i++) {
            const auto& ZoneExclusion = ZoneExclusions[i];
            Zones_ToRemove += ZoneExclusion.Elements[zoneExclusion_zone].size();
        }
        Zones.resize(Zones.size() - Zones_ToRemove);
        ZoneExclusions.resize(BlockFormat_zoneExclusions.size());

        auto& matrixes = Items[item_matrix].Items;
        matrixes.clear();
        
        auto& coefficients = Items[item_coefficient].Items;
        coefficients.clear();

        MoveErrors(this, item_audioChannelFormat);
        ChannelFormat_BlockFormats.pop_back();
        BlockFormats.pop_back();
        if (ChannelFormat_BlockFormat_ReduceCount.size() < ChannelFormats.size()) {
            ChannelFormat_BlockFormat_ReduceCount.resize(ChannelFormats.size());
        }
        ChannelFormat_BlockFormat_ReduceCount.back()++;
    };

    Item_Struct& audioFormatExtended_Content = Items[item_audioFormatExtended].Items.back();
    auto& Items_audioFormatExtended = Items[item_audioFormatExtended].Items;
    auto& Items_Up = Items_audioFormatExtended;
    XML_BEGIN
    XML_ATTR_START
        if (!tfsxml_strcmp_charp(b, "version")) {
            if (!tfsxml_strncmp_charp(v, "ITU-R_BS.2076-", 14) && v.len == 15 && v.buf[14] >= '0' && v.buf[14] <= '9') {
                Version = v.buf[14] - '0';
            }
            else {
                Version_String = tfsxml_decode(v);
                Version = -1;
            }
        }
    XML_ATTR_END
    XML_ELEM_START
    ELEMENT_S(audioFormatExtended, audioProgramme)
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
        ELEMENT_M(audioProgramme)
        ELEMENT_S(audioProgramme, audioProgrammeLabel)
            ATTRIBUTE(audioProgrammeLabel, language)
            ELEMENT_M(audioProgrammeLabel)
            ELEMENT_e(audioProgrammeLabel, audioProgramme, audioProgrammeLabel_Check(this))
        ELEMENT__(audioProgramme, audioContentIDRef)
        ELEMENT_s(audioProgramme, loudnessMetadata, loudnessMetadata_Source.push_back('P'))
            ATTRIBUTE(loudnessMetadata, loudnessMethod)
            ATTRIBUTE(loudnessMetadata, loudnessRecType)
            ATTRIBUTE(loudnessMetadata, loudnessCorrectionType)
            ELEMENT_M(loudnessMetadata)
            ELEMENT__(loudnessMetadata, integratedLoudness)
            ELEMENT_I(loudnessMetadata, loudnessRange)
            ELEMENT_I(loudnessMetadata, maxTruePeak)
            ELEMENT_I(loudnessMetadata, maxMomentary)
            ELEMENT_I(loudnessMetadata, maxShortTerm)
            ELEMENT_I(loudnessMetadata, dialogueLoudness)
            ELEMENT_S(loudnessMetadata, renderer)
                ATTRIBUTE(renderer, uri)
                ATTRIBUTE(renderer, name)
                ATTRIBUTE(renderer, version)
                ATTRIBUTE(renderer, coordinateMode)
                ELEMENT_M(renderer)
                ELEMENT_I(renderer, audioPackFormatIDRef)
                ELEMENT_I(renderer, audioObjectIDRef)
                ELEMENT_E(renderer, loudnessMetadata)
            ELEMENT_e(loudnessMetadata, audioProgramme, loudnessMetadata_Check(this, item_audioProgramme))
        ELEMENT_S(audioProgramme, audioProgrammeReferenceScreen)
            ATTRIBUTE(audioProgrammeReferenceScreen, aspectRatio)
            ELEMENT_M(audioProgrammeReferenceScreen)
            ELEMENT_S(audioProgrammeReferenceScreen, screenCentrePosition)
                ATTRIBUTE(screenCentrePosition, coordinate)
                ATTRIBUTE(screenCentrePosition, azimuth)
                ATTRIBUTE(screenCentrePosition, elevation)
                ATTRIBUTE(screenCentrePosition, distance)
                ATTRIBUTE(screenCentrePosition, X)
                ATTRIBUTE(screenCentrePosition, Y)
                ATTRIBUTE(screenCentrePosition, Z)
                ELEMENT_M(screenCentrePosition)
                ELEMENT_E(screenCentrePosition, audioProgrammeReferenceScreen)
            ELEMENT_S(audioProgrammeReferenceScreen, screenWidth)
                ATTRIBUTE(screenWidth, coordinate)
                ATTRIBUTE(screenWidth, azimuth)
                ATTRIBUTE(screenWidth, X)
                ELEMENT_M(screenWidth)
                ELEMENT_e(screenWidth, audioProgrammeReferenceScreen, screenWidth_Check(this))
            ELEMENT_e(audioProgrammeReferenceScreen, audioProgramme, audioProgrammeReferenceScreen_Check(this))
        ELEMENT_S(audioProgramme, authoringInformation)
            ELEMENT_M(authoringInformation)
            ELEMENT_S(authoringInformation, referenceLayout)
                ELEMENT_M(referenceLayout)
                ELEMENT__(referenceLayout, audioPackFormatIDRef)
                ELEMENT_E(referenceLayout, authoringInformation)
            ELEMENT_S(authoringInformation, renderer)
                ATTRIBUTE(renderer, uri)
                ATTRIBUTE(renderer, name)
                ATTRIBUTE(renderer, version)
                ATTRIBUTE(renderer, coordinateMode)
                ELEMENT_M(renderer)
                ELEMENT_I(renderer, audioPackFormatIDRef)
                ELEMENT_I(renderer, audioObjectIDRef)
                ELEMENT_E(renderer, authoringInformation)
            ELEMENT_e(authoringInformation, audioProgramme, authoringInformation_Check(this))
        ELEMENT__(audioProgramme, alternativeValueSetIDRef)
        ELEMENT_e(audioProgramme, audioFormatExtended, audioProgramme_Check(this))
    ELEMENT_S(audioFormatExtended, audioContent)
        ATTRIB_ID(audioContent, audioContentID)
        ATTRIBUTE(audioContent, audioContentName)
        ATTRIBUTE(audioContent, audioContentLanguage)
        ATTRIBUTE(audioContent, typeLabel)
        ELEMENT_M(audioContent)
        ELEMENT__(audioContent, audioObjectIDRef)
        ELEMENT_S(audioContent, audioContentLabel)
            ATTRIBUTE(audioContentLabel, language)
            ELEMENT_M(audioContentLabel)
            ELEMENT_e(audioContentLabel, audioContent, audioContentLabel_Check(this))
        ELEMENT_s(audioContent, loudnessMetadata, loudnessMetadata_Source.push_back('C'))
            ATTRIBUTE(loudnessMetadata, loudnessMethod)
            ATTRIBUTE(loudnessMetadata, loudnessRecType)
            ATTRIBUTE(loudnessMetadata, loudnessCorrectionType)
            ELEMENT_M(loudnessMetadata)
            ELEMENT__(loudnessMetadata, integratedLoudness)
            ELEMENT_I(loudnessMetadata, loudnessRange)
            ELEMENT_I(loudnessMetadata, maxTruePeak)
            ELEMENT_I(loudnessMetadata, maxMomentary)
            ELEMENT_I(loudnessMetadata, maxShortTerm)
            ELEMENT_I(loudnessMetadata, dialogueLoudness)
            ELEMENT_S(loudnessMetadata, renderer)
                ATTRIBUTE(renderer, uri)
                ATTRIBUTE(renderer, name)
                ATTRIBUTE(renderer, version)
                ATTRIBUTE(renderer, coordinateMode)
                ELEMENT_M(renderer)
                ELEMENT_I(renderer, audioPackFormatIDRef)
                ELEMENT_I(renderer, audioObjectIDRef)
                ELEMENT_E(renderer, loudnessMetadata)
            ELEMENT_e(loudnessMetadata, audioContent, loudnessMetadata_Check(this, item_audioContent))
        ELEMENT_S(audioContent, dialogue)
            ATTRIBUTE(dialogue, nonDialogueContentKind)
            ATTRIBUTE(dialogue, dialogueContentKind)
            ATTRIBUTE(dialogue, mixedContentKind)
            ELEMENT_M(dialogue)
            ELEMENT_E(dialogue, audioContent)
        ELEMENT__(audioContent, alternativeValueSetIDRef)
        ELEMENT_e(audioContent, audioFormatExtended, audioContent_Check(this))
    ELEMENT_S(audioFormatExtended, audioObject)
        ATTRIB_ID(audioObject, audioObjectID)
        ATTRIBUTE(audioObject, audioObjectName)
        ATTRIBUTE(audioObject, start)
        ATTRIBUTE(audioObject, startTime)
        ATTRIBUTE(audioObject, duration)
        ATTRIBUTE(audioObject, dialogue)
        ATTRIBUTE(audioObject, importance)
        ATTRIBUTE(audioObject, interact)
        ATTRIBUTE(audioObject, disableDucking)
        ATTRIBUTE(audioObject, typeLabel)
        ELEMENT_M(audioObject)
        ELEMENT__(audioObject, audioPackFormatIDRef)
        ELEMENT__(audioObject, audioObjectIDRef)
        ELEMENT_S(audioObject, audioObjectLabel)
            ATTRIBUTE(audioObjectLabel, language)
            ELEMENT_M(audioObjectLabel)
            ELEMENT_e(audioObjectLabel, audioObject, audioObjectLabel_Check(this))
        ELEMENT_S(audioObject, audioComplementaryObjectGroupLabel)
            ATTRIBUTE(audioComplementaryObjectGroupLabel, language)
            ELEMENT_M(audioComplementaryObjectGroupLabel)
            ELEMENT_e(audioComplementaryObjectGroupLabel, audioObject, audioComplementaryObjectGroupLabel_Check(this))
        ELEMENT__(audioObject, audioComplementaryObjectIDRef)
        ELEMENT__(audioObject, audioTrackUIDRef)
        ELEMENT_S(audioObject, audioObjectInteraction)
            ATTRIBUTE(audioObjectInteraction, onOffInteract)
            ATTRIBUTE(audioObjectInteraction, gainInteract)
            ATTRIBUTE(audioObjectInteraction, positionInteract)
            ELEMENT_M(audioObjectInteraction)
            ELEMENT_S(audioObjectInteraction, gainInteractionRange)
                ATTRIBUTE(gainInteractionRange, gainUnit)
                ATTRIBUTE(gainInteractionRange, bound)
                ELEMENT_M(gainInteractionRange)
                ELEMENT_E(gainInteractionRange, audioObjectInteraction)
            ELEMENT_S(audioObjectInteraction, positionInteractionRange)
                ATTRIBUTE(positionInteractionRange, coordinate)
                ATTRIBUTE(positionInteractionRange, bound)
                ELEMENT_M(positionInteractionRange)
                ELEMENT_E(positionInteractionRange, audioObjectInteraction)
            ELEMENT_E(audioObjectInteraction, audioObject)
        ELEMENT_S(audioObject, gain)
            ATTRIBUTE(gain, gainUnit)
            ELEMENT_M(gain)
            ELEMENT_E(gain, audioObject)
        ELEMENT__(audioObject, headLocked)
        ELEMENT_S(audioObject, positionOffset)
            ATTRIBUTE(positionOffset, coordinate)
            ELEMENT_M(positionOffset)
            ELEMENT_E(positionOffset, audioObject)
        ELEMENT__(audioObject, mute)
        ELEMENT_S(audioObject, alternativeValueSet)
            ATTRIBUTE(alternativeValueSet, alternativeValueSetID)
            ELEMENT_M(alternativeValueSet)
            ELEMENT_E(alternativeValueSet, audioObject)
        ELEMENT_e(audioObject, audioFormatExtended, audioObject_Check(this))
    ELEMENT_S(audioFormatExtended, audioPackFormat)
        ATTRIB_ID(audioPackFormat, audioPackFormatID)
        ATTRIBUTE(audioPackFormat, audioPackFormatName)
        ATTRIBUTE(audioPackFormat, typeDefinition)
        ATTRIBUTE(audioPackFormat, typeLabel)
        ATTRIBUTE(audioPackFormat, typeLink)
        ATTRIBUTE(audioPackFormat, typeLanguage)
        ATTRIBUTE(audioPackFormat, importance)
        ELEMENT_M(audioPackFormat)
        ELEMENT__(audioPackFormat, audioChannelFormatIDRef)
        ELEMENT__(audioPackFormat, audioPackFormatIDRef)
        ELEMENT__(audioPackFormat, absoluteDistance)
        ELEMENT__(audioPackFormat, encodePackFormatIDRef)
        ELEMENT__(audioPackFormat, decodePackFormatIDRef)
        ELEMENT__(audioPackFormat, inputPackFormatIDRef)
        ELEMENT__(audioPackFormat, outputPackFormatIDRef)
        ELEMENT__(audioPackFormat, normalization)
        ELEMENT__(audioPackFormat, nfcRefDist)
        ELEMENT__(audioPackFormat, screenRef)
        ELEMENT_E(audioPackFormat, audioFormatExtended)
    ELEMENT_S(audioFormatExtended, audioChannelFormat)
        ATTRIB_ID(audioChannelFormat, audioChannelFormatID)
        ATTRIBUTE(audioChannelFormat, audioChannelFormatName)
        ATTRIBUTE(audioChannelFormat, typeDefinition)
        ATTRIBUTE(audioChannelFormat, typeLabel)
        ELEMENT_M(audioChannelFormat)
        ELEMENT_s(audioChannelFormat, audioBlockFormat, audioBlockformat_Reduce())
            ATTRIB_ID(audioBlockFormat, audioBlockFormatID)
            ATTRIBUTE(audioBlockFormat, rtime)
            ATTRIBUTE(audioBlockFormat, duration)
            ATTRIBUTE(audioBlockFormat, lstart)
            ATTRIBUTE(audioBlockFormat, lduration)
            ATTRIBUTE(audioBlockFormat, initializeBlock)
            ELEMENT_M(audioBlockFormat)
            ELEMENT_S(audioBlockFormat, gain)
                ATTRIBUTE(gain, gainUnit)
                ELEMENT_M(gain)
                ELEMENT_e(gain, audioBlockFormat, gain_Check(this))
            ELEMENT__(audioBlockFormat, importance)
            ELEMENT__(audioBlockFormat, headLocked)
            ELEMENT_S(audioBlockFormat, jumpPosition)
                ATTRIBUTE(jumpPosition, interpolationLength)
                ELEMENT_M(jumpPosition)
                ELEMENT_E(jumpPosition, audioBlockFormat)
            ELEMENT_S(audioBlockFormat, headphoneVirtualise)
                ATTRIBUTE(headphoneVirtualise, bypass)
                ATTRIBUTE(headphoneVirtualise, DRR)
                ELEMENT_M(headphoneVirtualise)
                ELEMENT_E(headphoneVirtualise, audioBlockFormat)
            ELEMENT__(audioBlockFormat, speakerLabel)
            ELEMENT_S(audioBlockFormat, position)
                ATTRIBUTE(position, coordinate)
                ATTRIBUTE(position, bound)
                ATTRIBUTE(position, screenEdgeLock)
                ELEMENT_M(position)
                ELEMENT_E(position, audioBlockFormat)
            ELEMENT__(audioBlockFormat, outputChannelFormatIDRef)
            ELEMENT_S(audioBlockFormat, matrix)
                ELEMENT_M(matrix)
                ELEMENT_S(matrix, coefficient)
                    ATTRIBUTE(coefficient, gainUnit)
                    ATTRIBUTE(coefficient, gain)
                    ATTRIBUTE(coefficient, gainVar)
                    ATTRIBUTE(coefficient, phase)
                    ATTRIBUTE(coefficient, phaseVar)
                    ATTRIBUTE(coefficient, delay)
                    ATTRIBUTE(coefficient, delayVar)
                    ELEMENT_M(coefficient)
                    ELEMENT_e(coefficient, matrix, coefficient_Check(this))
                ELEMENT_e(matrix, audioBlockFormat, matrix_Check(this))
            ELEMENT__(audioBlockFormat, width)
            ELEMENT__(audioBlockFormat, height)
            ELEMENT__(audioBlockFormat, depth)
            ELEMENT__(audioBlockFormat, cartesian)
            ELEMENT__(audioBlockFormat, diffuse)
            ELEMENT_S(audioBlockFormat, channelLock)
                ATTRIBUTE(channelLock, maxDistance)
                ELEMENT_M(channelLock)
                ELEMENT_E(channelLock, audioBlockFormat)
            ELEMENT_S(audioBlockFormat, objectDivergence)
                ATTRIBUTE(objectDivergence, azimuthRange)
                ATTRIBUTE(objectDivergence, positionRange)
                ELEMENT_M(objectDivergence)
                ELEMENT_e(objectDivergence, audioBlockFormat, objectDivergence_Check(this))
            ELEMENT_S(audioBlockFormat, zoneExclusion)
                ELEMENT_M(zoneExclusion)
                ELEMENT_S(zoneExclusion, zone)
                    ATTRIBUTE(zone, minX)
                    ATTRIBUTE(zone, maxX)
                    ATTRIBUTE(zone, minY)
                    ATTRIBUTE(zone, maxY)
                    ATTRIBUTE(zone, minZ)
                    ATTRIBUTE(zone, maxZ)
                    ATTRIBUTE(zone, minElevation)
                    ATTRIBUTE(zone, maxElevation)
                    ATTRIBUTE(zone, minAzimuth)
                    ATTRIBUTE(zone, maxAzimuth)
                    ELEMENT_M(zone)
                    ELEMENT_E(zone, zoneExclusion)
                ELEMENT_e(zoneExclusion, audioBlockFormat, zoneExclusion_Check(this))
            ELEMENT__(audioBlockFormat, equation)
            ELEMENT__(audioBlockFormat, order)
            ELEMENT__(audioBlockFormat, degree)
            ELEMENT__(audioBlockFormat, normalization)
            ELEMENT__(audioBlockFormat, nfcRefDist)
            ELEMENT__(audioBlockFormat, screenRef)
            ELEMENT_e(audioBlockFormat, audioChannelFormat, audioBlockFormat_Check(this))
        ELEMENT_S(audioChannelFormat, frequency)
            ATTRIBUTE(frequency, typeDefinition)
            ELEMENT_M(frequency)
            ELEMENT_E(frequency, audioChannelFormat)
        ELEMENT_e(audioChannelFormat, audioFormatExtended, audioChannelFormat_Check(this))
            ELEMENT_S(audioFormatExtended, audioTrackUID)
        ATTRIB_ID(audioTrackUID, UID)
        ATTRIBUTE(audioTrackUID, sampleRate)
        ATTRIBUTE(audioTrackUID, bitDepth)
        ATTRIBUTE(audioTrackUID, typeLabel)
        ELEMENT_M(audioTrackUID)
        ELEMENT__(audioTrackUID, audioMXFLookUp)
        ELEMENT__(audioTrackUID, audioTrackFormatIDRef)
        ELEMENT__(audioTrackUID, audioChannelFormatIDRef)
        ELEMENT__(audioTrackUID, audioPackFormatIDRef)
        ELEMENT_E(audioTrackUID, audioFormatExtended)
    ELEMENT_S(audioFormatExtended, audioTrackFormat)
        ATTRIB_ID(audioTrackFormat, audioTrackFormatID)
        ATTRIBUTE(audioTrackFormat, audioTrackFormatName)
        ATTRIBUTE(audioTrackFormat, typeLabel)
        ATTRIBUTE(audioTrackFormat, typeDefinition)
        ATTRIBUTE(audioTrackFormat, formatLabel)
        ATTRIBUTE(audioTrackFormat, formatDefinition)
        ATTRIBUTE(audioTrackFormat, formatLink)
        ATTRIBUTE(audioTrackFormat, formatLanguage)
        ELEMENT_M(audioTrackFormat)
        ELEMENT__(audioTrackFormat, audioStreamFormatIDRef)
        ELEMENT_E(audioTrackFormat, audioFormatExtended)
    ELEMENT_S(audioFormatExtended, audioStreamFormat)
        ATTRIB_ID(audioStreamFormat, audioStreamFormatID)
        ATTRIBUTE(audioStreamFormat, audioStreamFormatName)
        ATTRIBUTE(audioStreamFormat, typeLabel)
        ATTRIBUTE(audioStreamFormat, typeDefinition)
        ATTRIBUTE(audioStreamFormat, formatLabel)
        ATTRIBUTE(audioStreamFormat, formatDefinition)
        ATTRIBUTE(audioStreamFormat, formatLink)
        ATTRIBUTE(audioStreamFormat, formatLanguage)
        ELEMENT_M(audioStreamFormat)
        ELEMENT__(audioStreamFormat, audioChannelFormatIDRef)
        ELEMENT__(audioStreamFormat, audioPackFormatIDRef)
        ELEMENT__(audioStreamFormat, audioTrackFormatIDRef)
        ELEMENT_E(audioStreamFormat, audioFormatExtended)
    ELEMENT_S(audioFormatExtended, profileList)
        ELEMENT_M(profileList)
        ELEMENT_S(profileList, profile)
            ATTRIBUTE(profile, profileName)
            ATTRIBUTE(profile, profileVersion)
            ATTRIBUTE(profile, profileLevel)
            ELEMENT_M(profile)
            ELEMENT_E(profile, profileList)
        ELEMENT_e(profileList, audioFormatExtended, profileList_Check(this))
    ELEMENT_S(audioFormatExtended, tagList)
        ELEMENT_M(tagList)
        ELEMENT_S(tagList, tagGroup)
            ATTRIBUTE(tagGroup, tag)
            ELEMENT_M(tagGroup)
            ELEMENT_S(tagGroup, tag)
                ATTRIBUTE(tag, class)
                ELEMENT_M(tag)
                ELEMENT_E(tag, tagGroup)
            ELEMENT_E(tagGroup, tagList)
        ELEMENT_E(tagList, audioFormatExtended)
    XML_ELEM_END
    CheckErrors_Elements(this, item_audioFormatExtended);
    XML_END
}

int file_adm_private::frameHeader()
{
    if (IsInit()) {
        if (Items[item_root].Items.empty()) {
            Items[item_root].New();
        }
        Items[item_root].Items.back().Elements[root_frameHeader].push_back({});
        Items[item_frameHeader].New();
    }

    Item_Struct& frameHeader_Content = Items[item_frameHeader].Items.back();
    auto& Items_frameHeader = Items[item_frameHeader].Items;
    auto& Items_Up = Items_frameHeader;
    XML_BEGIN
    XML_ATTR_START
    XML_ATTR_END
    XML_ELEM_START
    ELEMENT_S(frameHeader, profileList)
        ELEMENT_M(profileList)
        ELEMENT_S(profileList, profile)
            ATTRIBUTE(profile, profileName)
            ATTRIBUTE(profile, profileVersion)
            ATTRIBUTE(profile, profileLevel)
            ELEMENT_M(profile)
            ELEMENT_E(profile, profileList)
        ELEMENT_e(profileList, audioFormatExtended, profileList_Check(this))
    ELEMENT_S(frameHeader, frameFormat)
        ATTRIB_ID(frameFormat, frameFormatID)
        ATTRIBUTE(frameFormat, start)
        ATTRIBUTE(frameFormat, duration)
        ATTRIBUTE(frameFormat, type)
        ATTRIBUTE(frameFormat, timeReference)
        ATTRIBUTE(frameFormat, flowID)
        ATTRIBUTE(frameFormat, countToFull)
        ATTRIBUTE(frameFormat, numMetadataChunks)
        ATTRIBUTE(frameFormat, countToSameChunk)
        ELEMENT_M(frameFormat)
        ELEMENT_S(frameFormat, changedIDs)
            ELEMENT_M(changedIDs)
            ELEMENT_E(changedIDs, frameFormat)
        ELEMENT_e(frameFormat, frameHeader, frameFormat_Check(this))
    ELEMENT_S(frameHeader, transportTrackFormat)
        ATTRIB_ID(transportTrackFormat, transportID)
        ATTRIBUTE(transportTrackFormat, transportName)
        ATTRIBUTE(transportTrackFormat, numTracks)
        ATTRIBUTE(transportTrackFormat, numIDs)
        ELEMENT_M(transportTrackFormat)
        ELEMENT_S(transportTrackFormat, audioTrack)
            ATTRIBUTE(audioTrack, trackID)
            ATTRIBUTE(audioTrack, formatLabel)
            ATTRIBUTE(audioTrack, formatDefinition)
            ELEMENT_M(audioTrack)
            ELEMENT__(audioTrack, audioTrackUIDRef)
            ELEMENT_e(audioTrack, transportTrackFormat, audioTrack_Check(this))
        ELEMENT_e(transportTrackFormat, frameHeader, transportTrackFormat_Check(this))
    XML_ELEM_END
    CheckErrors_Elements(this, item_frameHeader);
    XML_END
}

//---------------------------------------------------------------------------
static void FillErrors(file_adm_private* File_Adm_Private, const item item_Type, size_t i, const char* Name, vector<string>* Errors_Field, vector<string>* Errors_Value, bool WarningError, size_t* Error_Count_Per_Type, bool IsAdvSSE)
{
    const bool IsAtmos = File_Adm_Private->IsAtmos;
    for (size_t k = 0; k < error_Type_Max; k++) {
    for (size_t l = 0; l < source_Max; l++) {
        if ((!IsAtmos && (l == Source_Atmos_1_0 || l == Source_Atmos_1_1))
         || (!IsAdvSSE && l == Source_AdvSSE_1)) {
            continue;
        }
        auto& Item = File_Adm_Private->Items[item_Type];
        if (!Item.Items[i].Errors[k][l].empty()) {
            for (size_t j = 0; j < Item.Items[i].Errors[k][l].size(); j++) {
                string Field = Name ? string(Name) : string();
                string Value = Item.Items[i].Errors[k][l][j];
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
                Error_Count_Per_Type[ErrorType]++;
                if (Error_Count_Per_Type[ErrorType] < 256)
                {

                    if (!Value.empty() && !Value[0]) {
                        const auto ErrorType = Value[(size_t)e::ErrorType];
                        const auto Opt0 = Value[(size_t)e::Opt0];
                        const auto Element_Pos = Value[(size_t)e::AttEle] & 0x7F;
                        bool IsAttribute = ((int8s)Value[(size_t)e::AttEle]) < 0;

                        size_t Value_Pos = Value.size();
                        Field = item_Infos[item_Type].Name ? (CraftName(item_Infos[item_Type].Name) + to_string(i) + ' ') : string();
                        auto item_Type2 = item_Type;
                        while (Value_Pos > (size_t)e::Max) {
                            Value_Pos--;
                            const auto j = Value[Value_Pos];
                            Value_Pos--;
                            item_Type2 = (item)Value[Value_Pos];
                            const auto Element_Infos = item_Infos[item_Type2].Element_Infos;
                            Field += CraftName(item_Infos[item_Type2].Name) + to_string(j) + ' ';
                        }
                        const auto& item_Info = item_Infos[item_Type2];
                        const auto Info_Name = IsAttribute ? (*item_Info.Attribute_Infos)[Element_Pos].Name : (*item_Info.Element_Infos)[Element_Pos].Name;
                        Field += Info_Name;
                        Value = Info_Name;
                        Value += ' ';
                        if (ErrorType == 2) {
                            Value += "sub";
                        }
                        Value += IsAttribute ? "attribute" : "element";
                        Value += ' ';
                        Value += E_Strings[ErrorType];
                        auto Pos = Value.find("{}");
                        if (Pos != string::npos) {
                            Value.erase(Pos, 2);
                            switch ((E)ErrorType) {
                            case E::Form:
                            case E::Permitted:
                                if (Opt0 < File_Adm_Private->Errors_Tips[k][l].size()) {
                                    Value.insert(Pos, File_Adm_Private->Errors_Tips[k][l][Opt0]);
                                }
                                break;
                            default:
                                Value.insert(Pos, to_string(Opt0));
                            }
                        }
                    }
                    Errors_Field[ErrorType].push_back(Field);
                    Errors_Value[ErrorType].push_back(Value + Source_Strings[l]);
                }
                else if (Error_Count_Per_Type[ErrorType] == 9)
                {
                    Errors_Field[ErrorType].push_back(Field);
                    Errors_Value[ErrorType].push_back("[...]");
                }
            }
        }
    }
    }
}

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
void File_Adm::Streams_Accept()
{
    Fill(Stream_General, 0, General_Format, "ADM");
    Stream_Prepare(Stream_Audio);
    if (!IsSub)
        Fill(Stream_Audio, StreamPos_Last, Audio_Format, "ADM");
}

//---------------------------------------------------------------------------
void File_Adm::Streams_Fill()
{
    // chna mapping
    auto& Transports = File_Adm_Private->Items[item_transportTrackFormat].Items;
    auto& Tracks = File_Adm_Private->Items[item_audioTrack].Items;
    if (Transports.empty() && !Tracks.empty()) {
        File_Adm_Private->Items[item_transportTrackFormat].New();
        Transports.back().Elements[transportTrackFormat_audioTrack].resize(Tracks.size());
    }
    // S-ADM mapping
    size_t Track_Pos = 0;
    for (size_t i = 0; i < Transports.size(); i++) {
        auto& Transport = Transports[i];
        auto& audioTrack_Items = Transport.Elements[transportTrackFormat_audioTrack];
        for (size_t j = 0; j < audioTrack_Items.size(); Track_Pos++, j++) {
            auto& Track_Item = Tracks[Track_Pos];
            const auto& trackIDs = Track_Item.Attributes[audioTrack_trackID];
            if (!trackIDs.empty()) {
                auto trackID = Ztring().From_UTF8(trackIDs.c_str()).To_int32u();
                if (trackID && trackID <= 0x10000) {
                    trackID--;
                    if (trackID < j) {
                        audioTrack_Items.insert(audioTrack_Items.begin() + trackID, {});
                        Tracks[trackID] = Track_Item;
                        Track_Item = {};
                        Track_Item.Attributes.resize(audioTrack_Attribute_Max);
                        Track_Item.Elements.resize(audioTrack_Element_Max);
                    }
                    if (trackID > j) {
                        audioTrack_Items.insert(audioTrack_Items.begin() + j, trackID - j, {});
                        Tracks.insert(Tracks.begin() + j, trackID - j, {});
                        for (; j < trackID; j++) {
                            Tracks[j].Attributes.resize(audioTrack_Attribute_Max);
                            Tracks[j].Elements.resize(audioTrack_Element_Max);
                        }
                    }
                }
            }
        }
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

    #define FILL_START_SUB_OFFSET(NAME,ELEMENT__,OFFSET) \
        auto OFFSET##_Previous = OFFSET; \
        OFFSET += File_Adm_Private->Items[item_##NAME].Items[i].Elements[NAME##_##ELEMENT__].size(); \
        for (size_t i = OFFSET##_Previous; i < OFFSET ; i++) { \

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
        if (FIELD) \
        { \
            FieldName += ' '; \
            FieldName += FIELD; \
            const auto& Previous = Retrieve(Stream_Audio, StreamPos_Last, FieldName.c_str()); \
            if (!Previous.empty() && Previous != __T("Yes")) { \
                List.insert(List.begin(), Previous); \
            } \
        } \
        Fill(Stream_Audio, StreamPos_Last, FieldName.c_str(), List.Read(), FIELD == nullptr || List.size() > 1); \
        } \

    #define LINK(NAME,FIELD,VECTOR,TARGET) \
        Apply_SubStreams(*this, P + " LinkedTo_" FIELD "_Pos", File_Adm_Private->Items[item_##NAME].Items[i], NAME##_##VECTOR, File_Adm_Private->Items[item_##TARGET], File_Adm_Private->IsPartial || File_Adm_Private->Items[item_audioFormatExtended].Items.empty()); \

    //Filling
    if ((int8_t)File_Adm_Private->Version_S >= 0)
        Fill(Stream_Audio, StreamPos_Last, "Metadata_Format", "S-ADM, Version " + Ztring::ToZtring(File_Adm_Private->Version_S).To_UTF8());
    string Version_Temp = File_Adm_Private->Version_String.empty() ? to_string(File_Adm_Private->Version) : File_Adm_Private->Version_String;
    Fill(Stream_Audio, StreamPos_Last, "Metadata_Format", "ADM, Version " + Version_Temp + File_Adm_Private->More["Metadata_Format"]);
    if (!MuxingMode.empty())
        Fill(Stream_Audio, StreamPos_Last, "Metadata_MuxingMode", MuxingMode);
    for (map<string, string>::iterator It = File_Adm_Private->More.begin(); It != File_Adm_Private->More.end(); ++It)
        //if (It->first != "Metadata_Format")
            Fill(Stream_Audio, StreamPos_Last, It->first.c_str(), It->second);
    if (File_Adm_Private->Items[item_audioProgramme].Items.size() == 1 && File_Adm_Private->Items[item_audioProgramme].Items[0].Attributes[audioProgramme_audioProgrammeName] == "Atmos_Master") {
        File_Adm_Private->profileInfos.resize(File_Adm_Private->profileInfos.size() + 1);
        File_Adm_Private->profileInfos.back().Strings[0] = "Dolby Atmos Master";
        if (!File_Adm_Private->DolbyProfileCanNotBeVersion1)
            File_Adm_Private->profileInfos.back().Strings[1] = "1";
    }
    vector<profile_info>& profileInfos = File_Adm_Private->profileInfos;
    auto& ProfileList = File_Adm_Private->Items[item_profileList].Items;
    auto& Profiles = File_Adm_Private->Items[item_profile].Items;
    size_t Profiles_Pos = 0;
    bool IsAdvSSE = false;
    vector<int8u> IsAdvSSE_Versions;
    vector<int8u> IsAdvSSE_Levels;
    for (size_t i = 0; i < ProfileList.size(); i++) {
        auto& ProfileList_Item = ProfileList[i];
        const auto& Profile_Items = ProfileList_Item.Elements[profileList_profile];
        for (size_t j = 0; j < Profile_Items.size(); Profiles_Pos++, j++) {
            const auto& Profile = Profile_Items[j];
            profile_info ProfileInfo;
            auto& Profile_Item = Profiles[Profiles_Pos];
            bool Found = false;
            for (size_t k = 0; k < Profiles_Pos; k++) {
                const auto& Profile2 = Profiles[k];
                if (Profile2.Attributes[profile_profileName] == Profile_Item.Attributes[profile_profileName]
                    && Profile2.Attributes[profile_profileVersion] == Profile_Item.Attributes[profile_profileVersion]
                    && Profile2.Attributes[profile_profileLevel] == Profile_Item.Attributes[profile_profileLevel]) {
                    Found = true;
                }
            }
            if (Found) {
                continue;
            }
            if (Profile_Item.Attributes[profile_profileName] == "Advanced sound system: ADM and S-ADM Profile for emission"
                || Profile_Item.Attributes[profile_profileName] == "AdvSS Emission S-ADM Profile") {
                ProfileInfo.Strings[0] = "AdvSS Emission";
                IsAdvSSE = true;
                IsAdvSSE_Versions.push_back(strtoul(Profile_Item.Attributes[profile_profileVersion].c_str(), nullptr, 10));
                IsAdvSSE_Levels.push_back(strtoul(Profile_Item.Attributes[profile_profileLevel].c_str(), nullptr, 10));
                if (IsAdvSSE_Levels.back() > 2 && (Profile == "ITU-R BS.[ADM-NGA-EMISSION]-0")) {
                    Profiles.back().AddError(Error, ':' + CraftName(item_Infos[item_profile].Name) + to_string(i) + ":profileLevel:profileLevel attribute value " + Profile_Item.Attributes[profile_profileLevel] + " is not permitted, max is 2", Source_AdvSSE_1);
                }
            }
            else {
                ProfileInfo.Strings[0] = Profile_Item.Attributes[profile_profileName];
            }
            ProfileInfo.Strings[1] = Profile_Item.Attributes[profile_profileVersion];
            ProfileInfo.Strings[3] = Profile_Item.Attributes[profile_profileLevel];
            profileInfos.insert(profileInfos.begin(), ProfileInfo);
        }
    }
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
                    if (profileInfos[i].Strings[j].empty()) {
                        continue;
                    }
                    Fill(Stream_Audio, 0, ("AdmProfile AdmProfile" + Ztring::ToZtring(i).To_UTF8() + ' ' + profile_names_InternalID[j]).c_str(), profileInfos[i].Strings[j]);
                    Fill_SetOptions(Stream_Audio, 0, ("AdmProfile AdmProfile" + Ztring::ToZtring(i).To_UTF8() + ' ' + profile_names_InternalID[j]).c_str(), "N NTY");
                }
            }
        }
        for (size_t j = 0; j < (PosCommon == 0 ? 1 : PosCommon); j++)
        {
            if (profileInfos[0].Strings[j].empty()) {
                continue;
            }
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

    size_t audioProgrammeLabel_Pos = 0;
    size_t loudnessMetadata_Pos = 0;
    size_t authoringInformation_Pos = 0;
    size_t referenceLayout_Pos = 0;
    FILL_START(audioProgramme, audioProgrammeName)
        if (Full)
            FILL_A(audioProgramme, audioProgrammeID, "ID");
        FILL_A(audioProgramme, audioProgrammeName, "Title");
        auto& audioProgrammeLabels = File_Adm_Private->Items[item_audioProgramme].Items[i].Elements[audioProgramme_audioProgrammeLabel];
        auto audioProgrammeLabels_Copy = audioProgrammeLabels;
        for (auto& audioProgrammeLabel : audioProgrammeLabels) {
            auto& audioProgrammeLabel_Item = File_Adm_Private->Items[item_audioProgrammeLabel].Items[audioProgrammeLabel_Pos];
            const auto& Language = audioProgrammeLabel_Item.Attributes[audioProgrammeLabel_language];
            if (!audioProgrammeLabel.empty() && !Language.empty()) {
                audioProgrammeLabel.insert(0, '(' + Language + ')');
            }
            audioProgrammeLabel_Pos++;
        }
        FILL_E(audioProgramme, audioProgrammeLabel, "Label");
        audioProgrammeLabels = audioProgrammeLabels_Copy;
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
        while (loudnessMetadata_Pos < File_Adm_Private->loudnessMetadata_Source.size() && File_Adm_Private->loudnessMetadata_Source[loudnessMetadata_Pos] != 'P') {
            loudnessMetadata_Pos++;
        }
        FILL_START_SUB_OFFSET(audioProgramme, loudnessMetadata, loudnessMetadata_Pos)
            FILL_E(loudnessMetadata, integratedLoudness, "IntegratedLoudness");
        }
        LINK(audioProgramme, "Content", audioContentIDRef, audioContent);
        FILL_START_SUB_OFFSET(audioProgramme, authoringInformation, authoringInformation_Pos)
            //FILL_START_SUB_OFFSET(authoringInformation, referenceLayout, referenceLayout_Pos)
            auto referenceLayout_Pos_Previous = referenceLayout_Pos;
            referenceLayout_Pos += File_Adm_Private->Items[item_authoringInformation].Items[i].Elements[authoringInformation_referenceLayout].size();
            for (size_t i = referenceLayout_Pos_Previous; i < referenceLayout_Pos; i++) {
            
            LINK(referenceLayout, "PackFormat", audioPackFormatIDRef, audioPackFormat);
            }
        }
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

    size_t audioContentLabel_Pos = 0;
    loudnessMetadata_Pos = 0;
    size_t dialogue_Pos = 0;
    FILL_START(audioContent, audioContentName)
        if (Full)
            FILL_A(audioContent, audioContentID, "ID");
        FILL_A(audioContent, audioContentName, "Title");
        auto& audioContentLabels = File_Adm_Private->Items[item_audioContent].Items[i].Elements[audioContent_audioContentLabel];
        auto audioContentLabels_Copy = audioContentLabels;
        for (auto& audioContentLabel : audioContentLabels) {
            auto& audioContentLabel_Item = File_Adm_Private->Items[item_audioContentLabel].Items[audioContentLabel_Pos];
            const auto& Language = audioContentLabel_Item.Attributes[audioContentLabel_language];
            if (!audioContentLabel.empty() && !Language.empty()) {
                audioContentLabel.insert(0, '(' + Language + ')');
            }
            audioContentLabel_Pos++;
        }
        FILL_E(audioContent, audioContentLabel, "Label");
        audioContentLabels = audioContentLabels_Copy;
        FILL_A(audioContent, audioContentLanguage, "Language");
        auto& dialogues = File_Adm_Private->Items[item_audioContent].Items[i].Elements[audioContent_dialogue];
        auto dialogues_Copy = dialogues;
        for (auto& dialogue : dialogues) {
            auto& dialogue_Item = File_Adm_Private->Items[item_dialogue].Items[dialogue_Pos];
            if (dialogue == "0") {
                const auto& Kind = dialogue_Item.Attributes[dialogue_nonDialogueContentKind];
                if (Kind == "1") {
                    dialogue = "Music";
                }
                else if (Kind == "2") {
                    dialogue = "Effect";
                }
                else {
                    dialogue = "No Dialogue";
                    if (!Kind.empty() && Kind != "0") {
                        dialogue += " (" + Kind + ')';
                    }
                }
            }
            else if (dialogue == "1") {
                const auto& Kind = dialogue_Item.Attributes[dialogue_dialogueContentKind];
                if (Kind == "1") {
                    dialogue = "Music";
                }
                else if (Kind == "2") {
                    dialogue = "Effect";
                }
                else if (Kind == "3") {
                    dialogue = "Spoken Subtitle";
                }
                else if (Kind == "4") {
                    dialogue = "Visually Impaired";
                }
                else if (Kind == "5") {
                    dialogue = "Commentary";
                }
                else if (Kind == "6") {
                    dialogue = "Emergency";
                }
                else {
                    dialogue = "Dialogue";
                    if (!Kind.empty() && Kind != "0") {
                        dialogue += " (" + Kind + ')';
                    }
                }
            }
            else if (dialogue == "2") {
                const auto& kind = dialogue_Item.Attributes[dialogue_mixedContentKind];
                if (kind == "1") {
                    dialogue = "Complete Main";
                }
                else if (kind == "2") {
                    dialogue = "Mixed (Mixed)";
                }
                else if (kind == "3") {
                    dialogue = "Hearing Impaired";
                }
                else {
                    dialogue = "Mixed";
                    if (!kind.empty() && kind != "0") {
                        dialogue += " (" + kind + ')';
                    }
                }
            }
            dialogue_Pos++;
        }
        FILL_E(audioContent, dialogue, "Mode");
        dialogues = dialogues_Copy;
        while (loudnessMetadata_Pos < File_Adm_Private->loudnessMetadata_Source.size() && File_Adm_Private->loudnessMetadata_Source[loudnessMetadata_Pos] != 'C') {
            loudnessMetadata_Pos++;
        }
        FILL_START_SUB_OFFSET(audioContent, loudnessMetadata, loudnessMetadata_Pos)
            FILL_E(loudnessMetadata, integratedLoudness, "IntegratedLoudness");
        }
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

    // Preprocessing speakeLabel;
    size_t audioBlockFormat_Pos = 0;
    vector<string> audioChannelFormat_ChannelLayout_List;
    audioChannelFormat_ChannelLayout_List.resize(File_Adm_Private->Items[item_audioChannelFormat].Items.size());
    const auto& audioChannelFormats = File_Adm_Private->Items[item_audioChannelFormat].Items;
    for (size_t i = 0; i < audioChannelFormats.size(); i++) {
        const auto& audioChannelFormat_Item = audioChannelFormats[i];
        const auto& audioChannelFormat_audioBlockFormats = audioChannelFormat_Item.Elements[audioChannelFormat_audioBlockFormat];
        auto& audioChannelFormat_ChannelLayout_Item = audioChannelFormat_ChannelLayout_List[i];
        if (!audioChannelFormat_audioBlockFormats.empty()) {
            const auto audioBlockFormat_Items = File_Adm_Private->Items[item_audioBlockFormat].Items;
            const auto& audioBlockFormat = audioBlockFormat_Items[audioBlockFormat_Pos];
            auto const& SpeakerLabels = audioBlockFormat.Elements[audioBlockFormat_speakerLabel];
            if (!SpeakerLabels.empty()) {
                auto SpeakerLabel = SpeakerLabels.front();
                size_t Cur = audioBlockFormat_Pos + 1;
                size_t End = audioBlockFormat_Pos + audioChannelFormat_audioBlockFormats.size();
                for (; Cur < End; Cur++) {
                    const auto& audioBlockFormat1 = audioBlockFormat_Items[Cur];
                    auto const& SpeakerLabels1 = audioBlockFormat1.Elements[audioBlockFormat_speakerLabel];
                    if (!SpeakerLabels.empty()) {
                        auto SpeakerLabel1 = SpeakerLabels1.front();
                        if (SpeakerLabel1 != SpeakerLabel) {
                            SpeakerLabel.assign(1, '\0');
                            break;
                        }
                    }
                }
                if (SpeakerLabel.rfind("RC_", 0) == 0) {
                    SpeakerLabel.erase(0, 3);
                }
                audioChannelFormat_ChannelLayout_Item = SpeakerLabel;
            }
        }
        audioBlockFormat_Pos += audioChannelFormat_audioBlockFormats.size();
    }

    FILL_START(audioPackFormat, audioPackFormatName)
        if (Full)
            FILL_A(audioPackFormat, audioPackFormatID, "ID");
        FILL_A(audioPackFormat, audioPackFormatName, "Title");
        FILL_A(audioPackFormat, typeDefinition, "TypeDefinition");
        const Item_Struct& Source = File_Adm_Private->Items[item_audioPackFormat].Items[i];
        const Items_Struct& Dest = File_Adm_Private->Items[item_audioChannelFormat];
        string ChannelLayout;
        for (size_t j = 0; j < Source.Elements[audioPackFormat_audioChannelFormatIDRef].size(); j++) {
            const string& ID = Source.Elements[audioPackFormat_audioChannelFormatIDRef][j];
            bool HasChannelLayout = false;
            for (size_t k = 0; k < Dest.Items.size(); k++) {
                if (Dest.Items[k].Attributes[audioChannelFormat_audioChannelFormatID] != ID) {
                    continue;
                }
                const auto& ChannelLayout_Item = audioChannelFormat_ChannelLayout_List[k];
                if (!ChannelLayout_Item.empty()) {
                    if (!ChannelLayout.empty()) {
                        ChannelLayout += ' ';
                    }
                    if (ChannelLayout_Item[0]) {
                        HasChannelLayout = true;
                        ChannelLayout += ChannelLayout_Item;
                    }
                    else {
                        ChannelLayout += '?';
                    }
                }
            }
        }
        if (!ChannelLayout.empty()) {
            Fill(StreamKind_Last, StreamPos_Last, (P + " ChannelLayout").c_str(), ChannelLayout, true, true);
        }

        LINK(audioPackFormat, "ChannelFormat", audioChannelFormatIDRef, audioChannelFormat);
    }

    FILL_START(audioChannelFormat, audioChannelFormatName)
        if (Full)
            FILL_A(audioChannelFormat, audioChannelFormatID, "ID");
        FILL_A(audioChannelFormat, audioChannelFormatName, "Title");
        FILL_A(audioChannelFormat, typeDefinition, "TypeDefinition");
        const auto& ChannelLayout = audioChannelFormat_ChannelLayout_List[i];
        if (!ChannelLayout.empty()) {
            if (ChannelLayout.size() == 1 && !ChannelLayout[0]) {
                Fill(Stream_Audio, StreamPos_Last, (P + " Type").c_str(), "Dynamic");
            }
            else if (!ChannelLayout.empty()) {
                Fill(Stream_Audio, StreamPos_Last, (P + " ChannelLayout").c_str(), ChannelLayout);
                Fill(Stream_Audio, StreamPos_Last, (P + " Type").c_str(), "Static");
            }
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
            FILL_E(audioTrack, audioTrackUIDRef, nullptr);
            if (Full)
                LINK(audioTrack, "TrackUID", audioTrackUIDRef, audioTrackUID);
            if (!File_Adm_Private->Items[item_transportTrackFormat].Items.empty() && i + 1 >= File_Adm_Private->Items[item_transportTrackFormat].Items.front().Elements[transportTrackFormat_audioTrack].size()) {
                break; // TODO: support 2 transport
            }
        }
    }

    if (!Full)
        Fill(Stream_Audio, 0, "PartialDisplay", "Yes");
    if (File_Adm_Private->IsPartial)
        Fill(Stream_Audio, 0, "PartialParsing", "Yes");

    // Errors
    #if MEDIAINFO_CONFORMANCE
    auto IsAtmos = File_Adm_Private->IsAtmos;
    auto& Root = File_Adm_Private->Items[item_root].Items;
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
    auto& ZoneExclusions = File_Adm_Private->Items[item_zoneExclusion].Items;
    auto& Zones = File_Adm_Private->Items[item_zone].Items;
    auto& TransportTrackFormats = File_Adm_Private->Items[item_transportTrackFormat].Items;
    auto& frameHeaders = File_Adm_Private->Items[item_frameHeader].Items;

    CheckErrors_Elements(File_Adm_Private, item_root);
    if (File_Adm_Private->Version_S != (uint8_t)-1 && frameHeaders.empty()) {
        Root[0].AddError(Error, root_frameHeader, E::Present0, 0);
    }

    set<string> TopLevelObjects;
    set<string> TopLevel_alternativeValueSetIDRefs;
    for (size_t i = 0; i < Contents.size(); i++) {
        auto& Content = Contents[i];
        for (const auto& audioObjectIDRef : Content.Elements[audioContent_audioObjectIDRef]) {
            if (!audioObjectIDRef.empty()) {
                TopLevelObjects.insert(audioObjectIDRef);
            }
        }
    }
    for (size_t i = 0; i < Objects.size(); i++) {
        auto& Object = Objects[i];
        if (TopLevelObjects.find(Object.Attributes[audioObject_audioObjectID]) == TopLevelObjects.end()) {
            continue;
        }
        for (const auto& alternativeValueS : Object.Elements[audioObject_alternativeValueSet]) {
            TopLevel_alternativeValueSetIDRefs.insert(alternativeValueS);
        }
    }

    size_t Max_Group_Comp = 0;
    for (size_t i = 0; i < Objects.size(); i++) {
        auto& Object = Objects[i];
        if (TopLevelObjects.find(Object.Attributes[audioObject_audioObjectID]) == TopLevelObjects.end()) {
            continue;
        }
        if (!Object.Elements[audioObject_audioComplementaryObjectIDRef].empty()) {
            Max_Group_Comp++;
        }
    }

    // Errors - For all
    for (size_t item_Type = 0; item_Type < item_Max; item_Type++) {
        if (item_Type == item_audioTrack) {
            continue; // TODO
        }
        auto& Items = File_Adm_Private->Items[item_Type].Items;
        static const size_t MaxCount_Size = 11;
        for (auto Level : IsAdvSSE_Levels) {
            if (Level) {
                static const int8u MaxCounts[][MaxCount_Size] =
                {
                    { 0, 0,   8,  16,  48,  32,  32,  32, 0, 0 },
                    { 0, 0,  16,  28,  84,  56,  56,  56, 0, 0 },
                };
                Level--;
                if (Level < sizeof(MaxCounts) / sizeof(MaxCounts[0]) && item_Type < sizeof(MaxCounts[0]) / sizeof(MaxCounts[0][0]) && MaxCounts[Level][item_Type] && Items.size() > MaxCounts[Level][item_Type]) {
                    Items[MaxCounts[Level][item_Type]].AddError(Error, ':' + CraftName(item_Infos[item_Type].Name) + to_string(MaxCounts[Level][item_Type]) + ":GeneralCompliance:" + CraftName(item_Infos[item_Type].Name) + " element count " + to_string(Items.size()) + " is not permitted, max is " + to_string(MaxCounts[Level][item_Type]) + "", Source_AdvSSE_1);
                }
            }
        }
        if (IsAtmos) {
            static const int8u MaxCounts[] = { 0, 0, 1, 123, 123, 123, 128, 128, 128, 128 };
            if (item_Type < sizeof(MaxCounts) / sizeof(MaxCounts[0]) && MaxCounts[item_Type] && Items.size() > MaxCounts[item_Type]) {
                Items[MaxCounts[item_Type]].AddError(Error, ':' + CraftName(item_Infos[item_Type].Name) + to_string(MaxCounts[item_Type]) + ":GeneralCompliance:" + CraftName(item_Infos[item_Type].Name) + " element count " + to_string(Items.size()) + " is not permitted, max is " + to_string(MaxCounts[item_Type]) + "", Source_Atmos_1_0);
            }
        }
        set<string> PreviousIDs;
        set<string> PreviousIDs_Forbidden;
        for (size_t i = 0; i < Items.size(); i++) {
            auto& Item = Items[i];
            if (item_Infos[item_Type].ID_Pos != (int8u)-1) {
                const auto& ID = Item.Attributes[item_Infos[item_Type].ID_Pos];
                if (PreviousIDs.find(ID) != PreviousIDs.end()) {
                    Item.AddError(Error, item_Type, i, ':' + CraftName(item_Infos[item_Type].Name, true) + "ID:" + CraftName(item_Infos[item_Type].Name, true) + "ID value \"" + ID + "\" shall be unique");
                }
                else if (PreviousIDs_Forbidden.find(ID) != PreviousIDs_Forbidden.end()) {
                    Item.AddError(Error, item_Type, i, ':' + CraftName(item_Infos[item_Type].Name, true) + "ID:" + CraftName(item_Infos[item_Type].Name, true) + "ID value \"" + ID + "\" is not permitted due to the ID of the DirectSpeakers typed object", Source_Atmos_1_0);
                }
                else {
                    PreviousIDs.insert(ID);
                    if (IsAtmos && item_Type == item_audioObject && !CheckErrors_ID(File_Adm_Private, ID, item_Infos[item_Type])) {
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
                CheckErrors_ID_Additions(File_Adm_Private, (item)item_Type, i);
            }
        }
    }
    for (const auto& label_Info : label_Infos) {
        auto& Items = File_Adm_Private->Items[label_Info.item_Type].Items;
        for (size_t i = 0; i < Items.size(); i++) {
            CheckErrors_formatLabelDefinition(File_Adm_Private, label_Info.item_Type, i, label_Info);
        }
    }
    if (!File_Adm_Private->IsPartial && !File_Adm_Private->Items[item_audioFormatExtended].Items.empty()) {
        for (const auto& IDRef : IDRefs) {
            auto& Items = File_Adm_Private->Items[IDRef.Source_Type].Items;
            for (size_t i = 0; i < Items.size(); i++) {
                CheckErrors_Element_Target(File_Adm_Private, IDRef.Source_Type, i, IDRef.Source_Pos, IDRef.Target_Type);
            }
        }
    }

    // Errors - General
    if (IsAdvSSE) {
        for (auto Version : IsAdvSSE_Versions) {
            if (Version == 1) {
                if (File_Adm_Private->Version != 3 && !File_Adm_Private->Items[item_audioFormatExtended].Items.empty()) {
                    Root[0].AddError(Error, ":GeneralCompliance:ADM version " + to_string(File_Adm_Private->Version) + " is not permitted, permitted version is 3", Source_AdvSSE_1);
                }
            }
        }
    }

    // Errors - audioProgramme
    float32 Programme0_duration = 0;
    for (size_t i = 0; i < Programmes.size(); i++) {
        auto& Programme = Programmes[i];
        const auto& start = Programme.Attributes[audioProgramme_start];
        const auto& end = Programme.Attributes[audioProgramme_end];
        const TimeCode start_TC = start;
        TimeCode end_TC = end;
        float32 duration;
        if (end_TC.IsValid()) {
            if (start_TC.IsValid()) {
                end_TC -= start_TC;
            }
            duration = end_TC.ToSeconds();
            if (!i) {
                Programme0_duration = duration;
            }
        }
        else {
            duration = 0;
        }
        static const size_t MaxCount_Size = 6;
        for (auto Level : IsAdvSSE_Levels) {
            if (Level) {
                static const int8u MaxCounts[][MaxCount_Size] =
                {
                    {   4,  16,   0,   0,   0,  16 },
                    {   8,  28,   0,   0,   0,  28 },
                };
                Level--;
                if (Level < sizeof(MaxCounts) / sizeof(MaxCounts[0])) {
                    for (size_t j = 0; j < MaxCount_Size; j++) {
                        if (MaxCounts[Level][j]) {
                            auto MaxCount = MaxCounts[Level][j];
                            if (!((int8u)(MaxCount + 1))) {
                            }
                            if (Programme.Elements[j].size() > MaxCount) {
                                const auto ElementName = (*item_Infos[item_audioProgramme].Element_Infos)[j].Name;
                                Programme.AddError(Error, ":audioProgramme" + to_string(i) + ':' + ElementName + ':' + ElementName + " element count " + to_string(Programme.Elements[j].size()) + " is not permitted, max is " + to_string(MaxCount), Source_AdvSSE_1);
                            }
                        }
                    }
                }
            }
        }
        if (IsAtmos) {
            if (Container_Duration && duration && (duration <= Container_Duration * 0.998 || duration >= Container_Duration * 1.002)) {
                Programme.AddError(Error, ":audioProgramme" + to_string(i) + ":end:start attribute value \"" + start + "\" and end attribute value \"" + end + "\" don't match content duration " + Ztring::ToZtring(Container_Duration, 3).To_UTF8() + " s", Source_Atmos_1_0);
            }
            if (Programme.Elements[audioProgramme_audioContentIDRef].size() > 123) {
                Programme.AddError(Error, ":audioProgramme" + to_string(i) + ":audioContentIDRef:audioContentIDRef element count " + to_string(Programme.Elements[audioProgramme_audioContentIDRef].size()) + " is not permitted, max is 123", Source_Atmos_1_0);
            }
        }
    }

    // Errors - audioContent
    dialogue_Pos = 0;
    for (size_t i = 0; i < Contents.size(); i++) {
        auto& Content = Contents[i];
        const auto& dialogues = Content.Elements[audioContent_dialogue];
        for (size_t j = 0; j < dialogues.size(); j++) {
            const auto& dialogue = dialogues[j];
            auto& dialogues = File_Adm_Private->Items[item_audioContent].Items[i].Elements[audioContent_dialogue];
            auto dialogues_Copy = dialogues;
            if (IsAtmos) {
                if (dialogue != "2") {
                    Content.AddError(Error, ":audioObject" + to_string(i) + ":dialogue" + to_string(j) + ":dialogue element value \"" + dialogue + "\" is not permitted, permitted value is \"2\"", Source_Atmos_1_0);
                }
                auto kind = File_Adm_Private->Items[item_dialogue].Items[dialogue_Pos].Attributes[dialogue_mixedContentKind];
                if (kind != "0") {
                    Content.AddError(Error, ":audioObject" + to_string(i) + ":dialogue" + to_string(j) + ":dialogue@mixedContentKind attribute value \"" + kind + "\" is not permitted, permitted value is \"0\"", Source_Atmos_1_0);
                }
            }
            dialogue_Pos++;
        }
        static const size_t MaxCount_Size = audioContent_audioContentLabel + 1;
        for (auto Level : IsAdvSSE_Levels) {
            if (Level) {
                static const int8u MaxCounts[][MaxCount_Size] =
                {
                    {  4 },
                    {  8 },
                };
                Level--;
                if (Level < sizeof(MaxCounts) / sizeof(MaxCounts[0])) {
                    for (size_t j = 0; j < MaxCount_Size; j++) {
                        if (MaxCounts[Level][j]) {
                            auto MaxCount = MaxCounts[Level][j];
                            if (!((int8u)(MaxCount + 1))) {
                            }
                            if (Content.Elements[j].size() > MaxCount) {
                                const auto ElementName = (*item_Infos[item_audioContent].Element_Infos)[j].Name;
                                Content.AddError(Error, ":audioContent" + to_string(i) + ':' + ElementName + ':' + ElementName + " element count " + to_string(Content.Elements[j].size()) + " is not permitted, max is " + to_string(MaxCount), Source_AdvSSE_1);
                            }
                        }
                    }
                }
            }
        }
        if (IsAtmos) {
            auto Item_ID = Content.Attributes[audioContent_audioContentID];
            if (!CheckErrors_ID(File_Adm_Private, Item_ID, item_Infos[item_audioContent])) {
                char* End;
                auto Value = strtoul(Item_ID.c_str() + 4, &End, 16);
                if (Value != 0x1001 + i) {
                    Content.AddError(Error, ":audioContent" + to_string(i) + ":audioContentID:audioContentID attribute wwww value \"" + Item_ID.substr(4) + "\" is not permitted, permitted value is \"" + Ztring::ToZtring(0x1001 + i, 16).To_UTF8() + "\"", Source_Atmos_1_0);
                }
            }
        }
    }

    // Errors - audioObject
    map<string, size_t> audioTrackUID_Used;
    size_t Object_Objects_Count = 0;
    for (size_t i = 0; i < Objects.size(); i++) {
        auto& Object = Objects[i];
        if (IsAtmos) {
            auto Type = GetType(File_Adm_Private, item_audioObject, i);
            const auto& ChannelFormatIDRefs = Object.Elements[audioObject_audioTrackUIDRef];
            if (!ChannelFormatIDRefs.empty()) {
                auto ChannelFormatIDRefs_Size = ChannelFormatIDRefs.size();
                if ((Type == Type_DirectSpeakers && ChannelFormatIDRefs_Size > 10) || (Type == Type_Objects && ChannelFormatIDRefs_Size > 1)) {
                    Object.AddError(Error, ":audioObject" + to_string(i) + ":audioTrackUIDRef:audioTrackUIDRef element count " + to_string(ChannelFormatIDRefs_Size) + " is not permitted, max is " + to_string(Type == Type_DirectSpeakers ? 10 : 1) + "", Source_Atmos_1_0);
                }
            }
        }
        const auto& start = Object.Attributes[audioObject_start];
        const auto& startTime = Object.Attributes[audioObject_startTime];
        const auto RealStart = start.empty() ? &startTime : &start;
        if (!start.empty() && !startTime.empty() && start != startTime) {
            Object.AddError(Warning, ":audioObject" + to_string(i) + ":start:startTime attribute should be same as start attribute");
        }
        static const size_t MaxCount_Size = audioObject_alternativeValueSet + 1;
        for (auto Level : IsAdvSSE_Levels) {
            if (Level) {
                static const int8u MaxCounts[][MaxCount_Size] =
                {
                    {  0, 16, 12,  4, 15,  0,  0,  0,  0,  0,  0,  8 },
                    {  0, 28, 24,  8, 27,  0,  0,  0,  0,  0,  0, 16 },
                };
                Level--;
                if (Level < sizeof(MaxCounts) / sizeof(MaxCounts[0])) {
                    for (size_t j = 0; j < MaxCount_Size; j++) {
                        if (MaxCounts[Level][j]) {
                            auto MaxCount = MaxCounts[Level][j];
                            if (!((int8u)(MaxCount + 1))) {
                            }
                            if (Object.Elements[j].size() > MaxCount) {
                                const auto ElementName = (*item_Infos[item_audioObject].Element_Infos)[j].Name;
                                Object.AddError(Error, ":audioObject" + to_string(i) + ':' + ElementName + ':' + ElementName + " element count " + to_string(Object.Elements[j].size()) + " is not permitted, max is " + to_string(MaxCount), Source_AdvSSE_1);
                            }
                        }
                    }
                }
            }
        }
        if (IsAdvSSE) {
            if (TopLevelObjects.find(Object.Attributes[audioObject_audioObjectID]) == TopLevelObjects.end()) {
                const auto& alternativeValueSets = Object.Elements[audioObject_alternativeValueSet];
                const auto& positionOffsets = Object.Elements[audioObject_positionOffset];
                for (size_t j = 0; j < positionOffsets.size(); j++) {
                    Object.AddError(Error, ":audioObject" + to_string(i) + ":positionOffset:positionOffset element is present but this is not a top-level audioObject", Source_AdvSSE_1);
                }
                for (size_t j = 0; j < alternativeValueSets.size(); j++) {
                    Object.AddError(Error, ":audioObject" + to_string(i) + ":alternativeValueSet:alternativeValueSet element is present but this is not a top-level audioObject", Source_AdvSSE_1);
                }
            }
        }
        if (IsAtmos) {
            if (!Object.Attributes_Present[audioObject_start] && !Object.Attributes_Present[audioObject_startTime]) {
                Object.AddError(Error, ":audioObject" + to_string(i) + ":start:start attribute is not present", Source_Atmos_1_0);
            }
            if (RealStart->empty()) {
            }
            else if (*RealStart != "00:00:00.00000") {
                Object.AddError(Error, ":audioObject" + to_string(i) + ":start:start attribute value \"" + *RealStart + "\" is not permitted, permitted value is \"00:00:00.00000\"", Source_Atmos_1_0);
            }
            if (Programme0_duration && Object.Attributes_Present[audioObject_duration]) {
                const auto& duration = Object.Attributes[audioObject_duration];
                const TimeCode duration_TC = Object.Attributes[audioObject_duration];
                if (duration_TC.IsValid()) {
                    const auto& Object_duration = duration_TC.ToSeconds();
                    if (Object_duration && (Object_duration <= Programme0_duration * 0.998 || Object_duration >= Programme0_duration * 1.002)) {
                        Object.AddError(Error, ":audioObject" + to_string(i) + ":duration:duration attribute value \"" + duration + "\" doesn't match programme start attribute value \"" + Programmes[0].Attributes[audioProgramme_start] + "\" and end attribute value \"" + Programmes[0].Attributes[audioProgramme_end] + "\"", Source_Atmos_1_0);
                    }
                }
            }
            const auto& interact = Object.Attributes[audioObject_interact];
            if (!interact.empty() && interact != "0") {
                Object.AddError(Error, ":audioObject" + to_string(i) + ":interact:interact attribute value \"" + interact + "\" is not permitted, permitted value is \"0\"", Source_Atmos_1_0);
            }
            const auto& disableDucking = Object.Attributes[audioObject_disableDucking];
            if (!disableDucking.empty() && disableDucking != "1") {
                Object.AddError(Error, ":audioObject" + to_string(i) + ":disableDucking:disableDucking attribute value \"" + disableDucking + "\" is not permitted, permitted value is \"1\"", Source_Atmos_1_0);
            }
        }
        for (const auto& audioPackFormatIDRef : Object.Elements[audioObject_audioPackFormatIDRef]) {
            for (const auto& audioPackFormat : PackFormats) {
                if (audioPackFormat.Attributes[audioChannelFormat_audioChannelFormatID] == audioPackFormatIDRef && (audioPackFormat.Attributes[audioChannelFormat_typeLabel] == "0003" || audioPackFormat.Attributes[audioChannelFormat_typeDefinition] == "Objects")) {
                    Object_Objects_Count++;
                }
            }
        }
        set<string> audioTrackUID_Used_Current;
        for (const auto& audioTrackUIDRef : Object.Elements[audioObject_audioTrackUIDRef]) {
            if (audioTrackUID_Used_Current.find(audioTrackUIDRef) == audioTrackUID_Used_Current.end()) {
                audioTrackUID_Used_Current.insert(audioTrackUIDRef);
                audioTrackUID_Used[audioTrackUIDRef]++;
            }
        }
    }
    if (Object_Objects_Count > 118) {
        Objects[0].AddError(Error, ":audioObject:audioObject pointing to audioChannelFormat@typeDefinition==\"Objects\" element count " + to_string(Object_Objects_Count) + " is not permitted, max is 118", Source_Atmos_1_0);
    }

    // Errors - audioPackFormat
    size_t PackFormat_Objects_Count = 0;
    for (size_t i = 0; i < PackFormats.size(); i++) {
        auto& PackFormat = PackFormats[i];
        auto Type = GetType(File_Adm_Private, item_audioPackFormat, i);
        const auto& ChannelFormatIDRefs = PackFormat.Elements[audioPackFormat_audioChannelFormatIDRef];
        if (IsAtmos) {
            if (!ChannelFormatIDRefs.empty()) {
                auto ChannelFormatIDRefs_Size = ChannelFormatIDRefs.size();
                if ((Type == Type_DirectSpeakers && ChannelFormatIDRefs_Size > 10) || (Type == Type_Objects && ChannelFormatIDRefs_Size > 1)) {
                    PackFormat.AddError(Error, ":audioPackFormat" + to_string(i) + ":audioChannelFormatIDRef:audioChannelFormatIDRef subelement count " + to_string(ChannelFormatIDRefs_Size) + " is not permitted, max is " + to_string(Type == Type_DirectSpeakers ? 10 : 1) + "", Source_Atmos_1_0);
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
                        PackFormat.AddError(Error, ":audioPackFormat" + to_string(i) + ":audioChannelFormatIDRef:audioChannelFormatIDRef order \"" + List_Strings_Flat + "\" is not permitted", Source_Atmos_1_0);
                    }
                }
            }
        }
        if (Type == Type_Matrix) {
            for (const auto& inputPackFormatIDRef : PackFormat.Elements[audioPackFormat_inputPackFormatIDRef]) {
                set<string> audioChannelFormatIDRefs;
                for (const auto& TrackUID : TrackUIDs) {
                    for (const auto& audioPackFormatIDRef : TrackUID.Elements[audioTrackUID_audioPackFormatIDRef]) {
                        if (audioPackFormatIDRef != inputPackFormatIDRef) {
                            continue;
                        }
                        for (const auto& audioChannelFormatIDRef : TrackUID.Elements[audioTrackUID_audioChannelFormatIDRef]) {
                            audioChannelFormatIDRefs.insert(audioChannelFormatIDRef);
                        }
                    }
                }
                for (const auto& audioChannelFormatIDRef : PackFormat.Elements[audioPackFormat_audioChannelFormatIDRef]) {
                    for (size_t j = 0; j < ChannelFormats.size(); j++) {
                        auto& ChannelFormat = ChannelFormats[j];
                        if (ChannelFormat.Attributes[audioChannelFormat_audioChannelFormatID] != audioChannelFormatIDRef || j >= File_Adm_Private->ChannelFormat_Matrix_coefficients.size()) {
                            continue;
                        }
                        const auto& coefficients_List = File_Adm_Private->ChannelFormat_Matrix_coefficients[j];
                        for (size_t k = 0; k < coefficients_List.size(); k++) {
                            const auto& coefficients = coefficients_List[k];
                            for (size_t l = 0; l < coefficients.List.size(); l++) {
                                const auto& coefficient = coefficients.List[l];
                                if (audioChannelFormatIDRefs.find(coefficient) == audioChannelFormatIDRefs.end()) {
                                    ChannelFormat.AddError(Error, ":audioChannelFormat" + to_string(j) + ":audioBlockFormat" + to_string(coefficients.BlockPos) + ":matrix" + to_string(0) + ":coefficient" + to_string(l) + ":coefficient value \"" + coefficient + "\" is not in corresponding inputPackFormatIDRef \"" + inputPackFormatIDRef + '\"', Source_AdvSSE_1);
                                }
                            }
                        }
                    }
                }
            }
            for (const auto& outputPackFormatIDRef : PackFormat.Elements[audioPackFormat_outputPackFormatIDRef]) {
                set<string> audioChannelFormatIDRefs;
                for (const auto& TrackUID : TrackUIDs) {
                    for (const auto& audioPackFormatIDRef : TrackUID.Elements[audioTrackUID_audioPackFormatIDRef]) {
                        if (audioPackFormatIDRef != outputPackFormatIDRef) {
                            continue;
                        }
                        for (const auto& audioChannelFormatIDRef : TrackUID.Elements[audioTrackUID_audioChannelFormatIDRef]) {
                            audioChannelFormatIDRefs.insert(audioChannelFormatIDRef);
                        }
                    }
                }
                bool CommonDefIsInvalid = false;
                if (audioChannelFormatIDRefs.empty() && !CheckErrors_ID(File_Adm_Private, outputPackFormatIDRef, item_Infos[item_audioPackFormat]) && outputPackFormatIDRef.rfind("AP_00010", 0) == 0) {
                    auto Value = strtoul(outputPackFormatIDRef.c_str() + 8, nullptr, 16);
                    audioChannelFormatIDRefs = audioPackFormatID_2_audioChannelFormatIDRef(Value);
                    if (audioChannelFormatIDRefs.empty()) {
                        CommonDefIsInvalid = true;
                    }
                }
                for (const auto& audioChannelFormatIDRef : PackFormat.Elements[audioPackFormat_audioChannelFormatIDRef]) {
                    for (size_t j = 0; j < ChannelFormats.size(); j++) {
                        auto& ChannelFormat = ChannelFormats[j];
                        if (ChannelFormat.Attributes[audioChannelFormat_audioChannelFormatID] != audioChannelFormatIDRef || j >= File_Adm_Private->ChannelFormat_Matrix_outputChannelFormatIDRefs.size()) {
                            continue;
                        }
                        const auto& outputChannelFormatIDRef_List = File_Adm_Private->ChannelFormat_Matrix_outputChannelFormatIDRefs[j];
                        for (size_t k = 0; k < outputChannelFormatIDRef_List.size(); k++) {
                            const auto& outputChannelFormatIDRefs = outputChannelFormatIDRef_List[k];
                            for (size_t l = 0; l < outputChannelFormatIDRefs.List.size(); l++) {
                                const auto& outputChannelFormatIDRef = outputChannelFormatIDRefs.List[l];
                                if (audioChannelFormatIDRefs.find(outputChannelFormatIDRef) == audioChannelFormatIDRefs.end()) {
                                    if (CommonDefIsInvalid) {
                                        ChannelFormat.AddError(Information, ":audioChannelFormat" + to_string(j) + ":audioBlockFormat" + to_string(outputChannelFormatIDRefs.BlockPos) + ":outputChannelFormatIDRef" + to_string(0) + ":outputChannelFormatIDRef value \"" + outputPackFormatIDRef.substr(7) + "\" is not allocated in ITU-R BS.2094-1");
                                        break;
                                    }
                                    else {
                                        ChannelFormat.AddError(Error, ":audioChannelFormat" + to_string(j) + ":audioBlockFormat" + to_string(outputChannelFormatIDRefs.BlockPos) + ":matrix" + to_string(0) + ":outputChannelFormatIDRef" + to_string(l) + ":Value \"" + outputChannelFormatIDRef + "\" is not in corresponding outputPackFormatIDRef \"" + outputPackFormatIDRef + '\"', Source_AdvSSE_1);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if (Type == Type_Objects) {
            PackFormat_Objects_Count++;
        }
        if (IsAdvSSE) {
            const auto& inputPackFormatIDRefs = PackFormat.Elements[audioPackFormat_inputPackFormatIDRef];
            const auto& outputPackFormatIDRefs = PackFormat.Elements[audioPackFormat_outputPackFormatIDRef];
            if (Type == Type_Matrix) {
                switch (inputPackFormatIDRefs.size()) {
                case 0:
                    PackFormat.AddError(Error, audioPackFormat_inputPackFormatIDRef, E::Present0, 0, Source_AdvSSE_1);
                    break;
                }
                switch (outputPackFormatIDRefs.size()) {
                case 0:
                    PackFormat.AddError(Error, audioPackFormat_outputPackFormatIDRef, E::Present0, 0, Source_AdvSSE_1);
                    break;
                }
                if (!inputPackFormatIDRefs.empty() && !outputPackFormatIDRefs.empty() && inputPackFormatIDRefs[0] == outputPackFormatIDRefs[0]) {
                    PackFormat.AddError(Error, ":audioPackFormat" + to_string(i) + ":outputPackFormatIDRef0:outputPackFormatIDRef is same as inputPackFormatIDRef", Source_AdvSSE_1);
                }
                if (!inputPackFormatIDRefs.empty() && !CheckErrors_ID(File_Adm_Private, inputPackFormatIDRefs[0], item_Infos[item_audioPackFormat])) {
                    auto Value = strtoul(inputPackFormatIDRefs[0].c_str() + 3, nullptr, 16);
                    bool IsNotValid = false;
                    if ((Value & 0xFFFFF700) != 0x00010000) {
                        IsNotValid = true;
                    }
                    else {
                        Value -= 0x00010000;
                        if (Value >= 0x800) {
                            Value -= 0x800;
                        }
                        static int8u List[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x09, 0x0A, 0x0C, 0x0F, 0x10, 0x17, 0x1B, 0x1C, 0x1E, 0x1F };
                        IsNotValid = true;
                        for (auto List_Item : List) {
                            if (List_Item == Value) {
                                IsNotValid = false;
                            }
                        }
                        if (IsNotValid) {
                            PackFormat.AddError(Error, ":audioPackFormat" + to_string(i) + ":inputPackFormatIDRef0:inputPackFormatIDRef " + inputPackFormatIDRefs[0] + " is not permitted", Source_AdvSSE_1);
                        }
                    }
                }
                if (!outputPackFormatIDRefs.empty() && !CheckErrors_ID(File_Adm_Private, outputPackFormatIDRefs[0], item_Infos[item_audioPackFormat])) {
                    auto Value = strtoul(outputPackFormatIDRefs[0].c_str() + 3, nullptr, 16);
                    bool IsNotValid = false;
                    if ((Value & 0xFFFFF700) != 0x00010000) {
                        IsNotValid = true;
                    }
                    else {
                        Value -= 0x00010000;
                        if (Value >= 0x800) {
                            Value -= 0x800;
                        }
                        static int8u List[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x09, 0x0F, 0x17 };
                        IsNotValid = true;
                        for (auto List_Item : List) {
                            if (List_Item == Value) {
                                IsNotValid = false;
                            }
                        }
                        if (IsNotValid) {
                            PackFormat.AddError(Error, ":audioPackFormat" + to_string(i) + ":outputPackFormatIDRef0:outputPackFormatIDRef " + outputPackFormatIDRefs[0] + " is not permitted", Source_AdvSSE_1);
                        }
                    }
                }
            }
            else {
                switch (inputPackFormatIDRefs.size()) {
                case 1:
                    PackFormat.AddError(Error, audioPackFormat_inputPackFormatIDRef, E::Present1, 0, Source_AdvSSE_1);
                    break;
                }
                switch (outputPackFormatIDRefs.size()) {
                case 1:
                    PackFormat.AddError(Error, audioPackFormat_outputPackFormatIDRef, E::Present1, 0, Source_AdvSSE_1);
                    break;
                }
            }
            if (PackFormat.Attributes[audioPackFormat_typeLabel] == "0003") {
                const auto& ID = PackFormat.Attributes[audioPackFormat_audioPackFormatID];
                bool Found = false;
                for (size_t i = 0; i < Objects.size(); i++) {
                    const auto& Object = Objects[i];
                    const auto& audioChannelFormatIDRefs = Object.Elements[audioObject_audioPackFormatIDRef];
                    for (size_t j = 0; j < audioChannelFormatIDRefs.size(); j++) {
                        const auto& audioChannelFormatIDRef = audioChannelFormatIDRefs[j];
                        if (audioChannelFormatIDRef == ID) {
                            Found = true;
                        }
                    }
                }
                if (!Found) {
                    PackFormat.AddError(Error, ":audioPackFormat" + to_string(i) + ":GeneralCompliance:this audioPackFormat is not referenced by any audioObject", Source_AdvSSE_1);
                }
            }
            if (PackFormat.Elements[audioPackFormat_audioChannelFormatIDRef].size() > 24) {
                PackFormat.AddError(Error, ":audioPackFormat:audioChannelFormatIDRef24:audioChannelFormatIDRef element count " + to_string(PackFormat.Elements[audioPackFormat_audioChannelFormatIDRef].size()) + " is not permitted, max is 24", Source_AdvSSE_1);
            }
        }
    }
    if (PackFormat_Objects_Count > 118) {
        PackFormats[0].AddError(Error, ":audioPackFormat0:GeneralCompliance:audioPackFormat@typeDefinition==\"Objects\" element count " + to_string(PackFormat_Objects_Count) + " is not permitted, max is 118", Source_Atmos_1_0);
    }

    // Errors - audioChannelFormat
    size_t BlockFormat_Pos = 0;
    size_t Position_Pos = 0;
    size_t zoneExclusion_Pos = 0;
    size_t zone_Pos = 0;
    for (size_t i = 0; i < ChannelFormats.size(); i++) {
        auto& ChannelFormat = ChannelFormats[i];
        const auto& ID = ChannelFormat.Attributes[audioChannelFormat_audioChannelFormatID];
        const auto Type = GetType(File_Adm_Private, item_audioChannelFormat, i);

        if (IsAdvSSE) {
            if (Type == Type_Matrix) {
                auto audioBlockFormat_Size = ChannelFormat.Elements[audioChannelFormat_audioBlockFormat].size();
                switch (audioBlockFormat_Size) {
                case 0:
                case 1:
                    break;
                default:
                    ChannelFormat.AddError(Error, audioChannelFormat_audioBlockFormat, E::Present2, (int8u)audioBlockFormat_Size, Source_AdvSSE_1);
                    break;
                }
            }

            const auto& ID = ChannelFormat.Attributes[audioChannelFormat_audioChannelFormatID];
            size_t Found = 0;
            for (size_t i = 0; i < PackFormats.size(); i++) {
                const auto& PackFormat = PackFormats[i];
                const auto& audioChannelFormatIDRefs = PackFormat.Elements[audioPackFormat_audioChannelFormatIDRef];
                for (size_t j = 0; j < audioChannelFormatIDRefs.size(); j++) {
                    const auto& audioChannelFormatIDRef = audioChannelFormatIDRefs[j];
                    if (audioChannelFormatIDRef == ID) {
                        Found++;
                    }
                }
            }
            if (Found != 1) {
                ChannelFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":GeneralCompliance:this audioChannelFormat is referenced by " + to_string(Found) + " audioPackFormat", Source_AdvSSE_1);
            }
        }

        if (IsAtmos) {
            atmos_audioChannelFormatName ChannelAssignment;
            if (Type == Type_DirectSpeakers) {
                if (ChannelFormat.Attributes_Present[audioChannelFormat_audioChannelFormatName]) {
                    const auto& ChannelFormatName = ChannelFormat.Attributes[audioChannelFormat_audioChannelFormatName];
                    ChannelAssignment = Atmos_audioChannelFormat_Pos(ChannelFormatName);
                    if (ChannelAssignment == (atmos_audioChannelFormatName)-1) {
                        ChannelFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioChannelFormatName:audioChannelFormatName \"" + ChannelFormatName + "\" is not permitted", Source_Atmos_1_0);
                    }
                }
                auto BlockFormats_Size = ChannelFormat.Elements[audioChannelFormat_audioBlockFormat].size();
                if (i < File_Adm_Private->ChannelFormat_BlockFormat_ReduceCount.size()) {
                    BlockFormats_Size += File_Adm_Private->ChannelFormat_BlockFormat_ReduceCount[i];
                }
                if (BlockFormats_Size != 1) {
                    ChannelFormat.AddError(Error, ":audioChannelFormat" + to_string(i) + ":audioBlockFormat1:GeneralCompliance:audioBlockFormat subelement count " + to_string(BlockFormats_Size) + " is not permitted, max is 1", Source_Atmos_1_0);
                }
            }

            if (!ChannelFormat.Elements[audioChannelFormat_frequency].empty()) {
                ChannelFormat.AddError(Warning, ":audioChannelFormat" + to_string(i) + ":frequency:frequency element should not be present", Source_Atmos_1_0);
            }
        }
    }

    // Errors - audioStreamFormat
    for (size_t i = 0; i < StreamFormats.size(); i++) {
        auto& StreamFormat = StreamFormats[i];
    }

    // Errors - audioTrackFormat
    for (size_t i = 0; i < TrackFormats.size(); i++) {
        auto& TrackFormat = TrackFormats[i];
    }

    // Errors - audioTrackUID
    for (size_t i = 0; i < TrackUIDs.size(); i++) {
        auto& TrackUID = TrackUIDs[i];

        if (TrackUID.Attributes_Present[audioTrackUID_sampleRate]) {
            const auto& Element = TrackUID.Attributes[audioTrackUID_sampleRate];
            char* End;
            auto Value = strtof(Element.c_str(), &End);
            if (End - Element.c_str() < Element.size()) {
                TrackUID.AddError(Error, ":audioTrackUID" + to_string(i) + ":sampleRate:sampleRate attribute value \"" + Element + "\" is malformed");
            }
            else if (IsAtmos && Value != 48000 && Value != 96000) {
                TrackUID.AddError(Error, ":audioTrackUID" + to_string(i) + ":sampleRate:sampleRate attribute value \"" + Element + "\" is not permitted", Source_Atmos_1_0);
            }
        }

        if (TrackUID.Attributes_Present[audioTrackUID_bitDepth]) {
            const auto& Element = TrackUID.Attributes[audioTrackUID_bitDepth];
            char* End;
            auto Value = strtof(Element.c_str(), &End);
            if (End - Element.c_str() < Element.size()) {
                TrackUID.AddError(Error, ":audioTrackUID" + to_string(i) + ":bitDepth:bitDepth attribute value \"" + Element + "\" is malformed");
            }
            else if (IsAtmos && Value != 24) {
                TrackUID.AddError(Error, ":audioTrackUID" + to_string(i) + ":bitDepth:bitDepth attribute value \"" + Element + "\" is not permitted", Source_Atmos_1_1);
            }
        }

        if (IsAdvSSE) {
            const auto& UID = TrackUID.Attributes[audioTrackUID_UID];
            auto Found = audioTrackUID_Used.find(UID);
            bool IsNotUsed = Found == audioTrackUID_Used.end();
            if (IsNotUsed) {
                //TrackUID.AddError(Error, ":audioTrackUID" + to_string(i) + ":GeneralCompliance:this audioTrackUID is not used", Source_AdvSSE_1);
            }
            else if (Found->second > 1) {
                TrackUID.AddError(Error, ":audioTrackUID" + to_string(i) + ":GeneralCompliance:this audioTrackUID is used " + to_string(Found->second) + " times", Source_AdvSSE_1);
            }
            if (!IsNotUsed) {
                const auto& audioChannelFormatIDRefs = TrackUID.Elements[audioTrackUID_audioChannelFormatIDRef];
                for (size_t j = 0; j < audioChannelFormatIDRefs.size(); j++) {
                    const auto& audioChannelFormatIDRef = audioChannelFormatIDRefs[j];
                    if (!CheckErrors_ID(File_Adm_Private, audioChannelFormatIDRef, item_Infos[item_audioChannelFormat])) {
                        auto audioChannelFormatIDRef_xxxx = audioChannelFormatIDRef.substr(3, 4);
                        auto Value = strtoul(audioChannelFormatIDRef_xxxx.c_str(), nullptr, 16);
                        switch (Value) {
                        case 1:
                        case 3:
                            break;
                        default:
                            TrackUID.AddError(Error, ":audioTrackUID" + to_string(i) + ":audioChannelFormatIDRef:audioChannelFormatIDRef attribute xxxx value \"" + audioChannelFormatIDRef_xxxx + "\" is not permitted, permitted values are \"0001\" or \"0003\"", Source_AdvSSE_1);
                        }
                        if (audioChannelFormatIDRef[7] == '0') {
                            auto audioChannelFormatIDRef_yyyy = audioChannelFormatIDRef.substr(7, 4);
                            Value = strtoul(audioChannelFormatIDRef_yyyy.c_str(), nullptr, 16);
                            if (Value > 0x0028) {
                                TrackUID.AddError(Information, ":audioTrackUID" + to_string(i) + ":audioChannelFormatIDRef:audioChannelFormatIDRef attribute yyyy value \"" + audioChannelFormatIDRef_yyyy + "\" is not allocated in ITU-R BS.2094-1");
                            }
                        }
                    }
                }
                const auto& audioPackFormatIDRefs = TrackUID.Elements[audioTrackUID_audioPackFormatIDRef];
                for (size_t j = 0; j < audioPackFormatIDRefs.size(); j++) {
                    const auto& audioPackFormatIDRef = audioPackFormatIDRefs[j];
                    if (!CheckErrors_ID(File_Adm_Private, audioPackFormatIDRef, item_Infos[item_audioPackFormat])) {
                        auto audioPackFormatIDRef_xxxx = audioPackFormatIDRef.substr(3, 4);
                        auto Value = strtoul(audioPackFormatIDRef_xxxx.c_str(), nullptr, 16);
                        switch (Value) {
                        case 1:
                        case 3:
                            break;
                        default:
                            TrackUID.AddError(Error, ":audioTrackUID" + to_string(i) + ":audioPackFormatIDRef:audioPackFormatIDRef attribute xxxx value \"" + audioPackFormatIDRef_xxxx + "\" is not permitted, permitted values are \"0001\" or \"0003\"", Source_AdvSSE_1);
                        }
                    }
                }
            }
        }

        if (IsAtmos) {
            auto Item_ID = TrackUID.Attributes[audioTrackUID_UID];
            if (!CheckErrors_ID(File_Adm_Private, Item_ID, item_Infos[item_audioTrackUID])) {
                char* End;
                auto Value = strtoul(Item_ID.c_str() + 8, &End, 16);
                if (Value != 0x00000001 + i) {
                    auto ExpectedValue = Ztring::ToZtring(0x00000001 + i, 16).To_UTF8();
                    if (ExpectedValue.size() < 8) {
                        ExpectedValue.insert(ExpectedValue.begin(), 8 - ExpectedValue.size(), '0');
                    }
                    TrackUID.AddError(Error, ":audioTrackUID" + to_string(i) + ":UID:UID attribute vvvvvvvv value \"" + Item_ID.substr(4) + "\" is not permitted, permitted value is \"" + ExpectedValue + "\"", Source_Atmos_1_0);
                }
            }
        }
    }

    // Errors - transportTrackFormat
    for (size_t i = 0; i < TransportTrackFormats.size(); i++) {
        auto& TransportTrackFormat = TransportTrackFormats[i];
        if (TransportTrackFormat.Attributes_Present[transportTrackFormat_numIDs]) {
            const auto& numIDs = TransportTrackFormat.Attributes[transportTrackFormat_numIDs];
            auto numIDs_Int = strtoul(numIDs.c_str(), nullptr, 10);
            if (to_string(numIDs_Int) != numIDs) {
            }
            else if (numIDs_Int < TrackUIDs.size()) {
                TransportTrackFormat.AddError(Error, ":transportTrackFormat" + to_string(i) + ":numIDs:numIDs attribute value " + numIDs + " is less than audioTrackUID element count " + to_string(TrackUIDs.size()), Source_AdvSSE_1);
            }
        }
    }   

    // Errors - Fill
    for (size_t t = 0; t < item_Max; t++) {
        size_t Error_Count_Per_Type[error_Type_Max] = {};
        for (size_t i = 0; i < File_Adm_Private->Items[t].Items.size(); i++) {
            FillErrors(File_Adm_Private, (item)t, i, item_Infos[t].Name, &Errors_Field[0], &Errors_Value[0], WarningError, &Error_Count_Per_Type[0], IsAdvSSE);
        }
    }

    //Conformance
    for (size_t k = 0; k < error_Type_Max; k++) {
        if (!Errors_Field[k].empty() && File_Adm_Private->Version <= Version_Max) {
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

    File_Adm_Private->clear();
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
    File_Adm_Private->File_Buffer_Size_Hint_Pointer = Config->File_Buffer_Size_Hint_Pointer_Get();
}

//---------------------------------------------------------------------------
void File_Adm::Read_Buffer_Continue()
{
    if (NeedToJumpToEnd) {
        // There was a jump, trying to resynch
        NeedToJumpToEnd = false;
        static const char* ToSearch = "</audioChannelFormat>";
        const char* Nok = (const char*)Buffer - 1;
        const char* LastPos = Nok;
        while (auto NextPos = strnstr(LastPos + 1, Buffer_Size - ((const int8u*)(LastPos + 1)-Buffer), ToSearch)) {
            LastPos = NextPos;
        }
        if (LastPos == Nok || File_Adm_Private->Resynch("audioFormatExtended")) {
            Buffer_Offset = Buffer_Size;
            ForceFinish();
            return;
        }
        size_t Offset = (const int8u*)LastPos - Buffer + 21; // + length of ToSearch
        Buffer += Offset;
        Buffer_Size -= Offset;
        Read_Buffer_Continue();
        Buffer_Size += Offset;
        Buffer -= Offset;
    }

    auto Result = File_Adm_Private->parse((void*)Buffer, Buffer_Size);
    if (!Status[IsAccepted]) {
        for (const auto& Items : File_Adm_Private->Items) {
            if (!Items.Items.empty()) {
                Accept("ADM");
                break;
            }
        }
    }
    Buffer_Offset = Buffer_Size - File_Adm_Private->Remain();
    if (Buffer_Offset < Buffer_Size) {
        Element_WaitForMoreData();
    }
    if (Status[IsAccepted]) {
        if (!File_Adm_Private->ChannelFormat_BlockFormat_ReduceCount.empty() && !File_Adm_Private->IsPartial && TotalSize > 512 * 1024 * 1024) {
            // Too big, we stop parsing here
            File_Adm_Private->IsPartial = true;
            NeedToJumpToEnd = true;
        }
        if (Result && TotalSize > 16 * 1024 * 1024 && File_Adm_Private->File_Buffer_Size_Hint_Pointer) {
            auto File_Offset_Now = File_Offset + Buffer_Size;
            auto Size = File_Size - File_Offset_Now;
            if (Size > 16 * 1024 * 1024) {
                Size = 16 * 1024 * 1024;
            }
            if (Size >= 64 * 1024) {
                *File_Adm_Private->File_Buffer_Size_Hint_Pointer = Size;
            }
            Element_WaitForMoreData();
        }
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
    File_Adm_Private->OldLocale = std::move(Adm->File_Adm_Private->OldLocale);
}

} //NameSpace

#endif //MEDIAINFO_ADM_YES

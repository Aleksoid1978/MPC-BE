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
#include <bitset>
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_ADM_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Adm.h"
#include "ThirdParty/tfsxml/tfsxml.h"
using namespace ZenLib;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
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
    float32 Value = (v[0] - '0') * 10 * 6 * 10 * 6 * 10
                  + (v[1] - '0')      * 6 * 10 * 6 * 10
                  + (v[3] - '0')          * 10 * 6 * 10
                  + (v[4] - '0')               * 6 * 10
                  + (v[6] - '0')                   * 10
                  + (v[7] - '0')                       ;
    if (v.size() < 9 || v[8] != '.')
        return Value;
    int i = 9;
    float32 Divider = 1;
    while (i < v.size() && v[i] >= '0' && v[i] <= '9')
    {
        Divider *= 10;
        Value += ((float32)(v[i] - '0')) / Divider;
        i++;
    }
    return Value;
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

enum flags {
    Version0,
    Version1,
    Version2,
    Count0,
    Count1,
    Count2,
    flags_Max
};

struct attribute_item {
    const char*         Name;
    bitset<flags_Max>   Flags;
};

struct element_item {
    const char*         Name;
    int                 Flags;
};

struct element {
    attribute_item*     Attributes;
    element_item*       Elements;
    int                 Attributes_Count;
    int                 Elements_Count;
};

enum items {
    item_audioTrack,
    item_audioProgramme,
    item_audioContent,
    item_audioObject,
    item_audioPackFormat,
    item_audioChannelFormat,
    item_audioTrackUID,
    item_audioTrackFormat,
    item_audioStreamFormat,
    item_Max
};

enum audioTrack_String {
    audioTrack_Summary,
    audioTrack_String_Max
};

enum audioTrack_StringVector {
    audioTrack_audioTrackUIDRef,
    audioTrack_StringVector_Max
};

enum audioProgramme_String {
    audioProgramme_audioProgrammeID,
    audioProgramme_audioProgrammeName,
    audioProgramme_audioProgrammeLanguage,
    audioProgramme_start,
    audioProgramme_end,
    audioProgramme_typeLabel,
    audioProgramme_String_Max
};

enum audioProgramme_StringVector {
    audioProgramme_audioProgrammeLabel,
    audioProgramme_audioContentIDRef,
    audioProgramme_StringVector_Max
};

enum audioContent_String {
    audioContent_audioContentID,
    audioContent_audioContentName,
    audioContent_audioContentLanguage,
    audioContent_typeLabel,
    audioContent_String_Max
};

enum audioContent_StringVector {
    audioContent_dialogue,
    audioContent_audioContentLabel,
    audioContent_integratedLoudness,
    audioContent_audioObjectIDRef,
    audioContent_StringVector_Max
};

enum audioObject_String {
    audioObject_audioObjectID,
    audioObject_audioObjectName,
    audioObject_startTime,
    audioObject_duration,
    audioObject_typeLabel,
    audioObject_String_Max
};

enum audioObject_StringVector {
    audioObject_audioPackFormatIDRef,
    audioObject_audioTrackUIDRef,
    audioObject_StringVector_Max
};

enum audioPackFormat_String {
    audioPackFormat_audioPackFormatID,
    audioPackFormat_audioPackFormatName,
    audioPackFormat_typeDefinition,
    audioPackFormat_typeLabel,
    audioPackFormat_String_Max
};

enum audioPackFormat_StringVector {
    audioPackFormat_audioChannelFormatIDRef,
    audioPackFormat_StringVector_Max
};

enum audioChannelFormat_String {
    audioChannelFormat_audioChannelFormatID,
    audioChannelFormat_audioChannelFormatName,
    audioChannelFormat_typeDefinition,
    audioChannelFormat_typeLabel,
    audioChannelFormat_String_Max
};

enum audioChannelFormat_StringVector {
    audioChannelFormat_StringVector_Max
};

enum audioTrackUID_String {
    audioTrackUID_UID,
    audioTrackUID_bitDepth,
    audioTrackUID_sampleRate,
    audioTrackUID_typeLabel,
    audioTrackUID_String_Max
};

enum audioTrackUID_StringVector {
    audioTrackUID_audioChannelFormatIDRef,
    audioTrackUID_audioPackFormatIDRef,
    audioTrackUID_audioTrackFormatIDRef,
    audioTrackUID_StringVector_Max
};

enum audioTrackFormat_String {
    audioTrackFormat_audioTrackFormatID,
    audioTrackFormat_audioTrackFormatName,
    audioTrackFormat_formatDefinition,
    audioTrackFormat_typeDefinition,
    audioTrackFormat_typeLabel,
    audioTrackFormat_String_Max
};

enum audioTrackFormat_StringVector {
    audioTrackFormat_audioStreamFormatIDRef,
    audioTrackFormat_StringVector_Max
};

enum audioStreamFormat_String {
    audioStreamFormat_audioStreamFormatID,
    audioStreamFormat_audioStreamFormatName,
    audioStreamFormat_formatDefinition,
    audioStreamFormat_formatLabel,
    audioStreamFormat_typeDefinition,
    audioStreamFormat_typeLabel,
    audioStreamFormat_String_Max
};

enum audioStreamFormat_StringVector {
    audioStreamFormat_audioChannelFormatIDRef,
    audioStreamFormat_audioPackFormatIDRef,
    audioStreamFormat_audioTrackFormatIDRef,
    audioStreamFormat_StringVector_Max
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

static attribute_item audioProgramme_Attributes[] =
{
    { "audioProgrammeID"                , (1 << Version0) | (1 << Version1) | (1 << Version2) | (1 << Count1) },
    { "audioProgrammeName"              , (1 << Version0) | (1 << Version1) | (1 << Version2) | (1 << Count1) },
    { "audioProgrammeLanguage"          , (1 << Version0) | (1 << Version1) | (1 << Version2) },
    { "start"                           , (1 << Version0) | (1 << Version1) | (1 << Version2) },
    { "end"                             , (1 << Version0) | (1 << Version1) | (1 << Version2) },
    { "maxDuckingDepth"                 , (1 << Version0) | (1 << Version1) | (1 << Version2) },
};

static element_item audioProgramme_Elements[] =
{
    { "audioProgrammeLabel"             ,                                     (1 << Version2) },
    { "audioContentIDRef"               , (1 << Version0) | (1 << Version1) | (1 << Version2) | (1 << Count1) | (1 << Count2) },
    { "loudnessMetadata"                , (1 << Version0) | (1 << Version1) | (1 << Version2) },
    { "audioProgrammeReferenceScreen"   , (1 << Version0) | (1 << Version1) | (1 << Version2) | (1 << Count0) | (1 << Count1) },
    { "authoringInformation"            ,                                     (1 << Version2) | (1 << Count0) | (1 << Count1) },
    { "alternativeValueSetIDRef"        ,                                     (1 << Version2) | (1 << Count0) | (1 << Count1) },
};

static element audioProgramme_Element =
{
    audioProgramme_Attributes,
    audioProgramme_Elements,
    sizeof(audioProgramme_Attributes) / sizeof(attribute_item),
    sizeof(audioProgramme_Elements) / sizeof(element_item),
};

struct Item_Struct {
    vector<string> Strings;
    vector<vector<string> > StringVectors;
    map<string, string> Extra;
    vector<string> Errors[error_Type_Max];
};

struct Items_Struct {
    void Init(size_t Strings_Size_, size_t StringVectors_Size_) {
        Strings_Size = Strings_Size_;
        StringVectors_Size =StringVectors_Size_;
    }

    Item_Struct& New()
    {
        Items.resize(Items.size() + 1);
        Item_Struct& Item = Items[Items.size() - 1];
        Item.Strings.resize(Strings_Size);
        Item.StringVectors.resize(StringVectors_Size);
        return Item;
    }

    Item_Struct& Last()
    {
        Item_Struct& Item = Items[Items.size() - 1];
        return Item;
    }

    vector<Item_Struct> Items;
    size_t Strings_Size;
    size_t StringVectors_Size;
};

static string Apply_Init(File__Analyze& F, const Char* Name, size_t i, const Items_Struct& audioProgramme_List, Ztring Summary) {
    const Item_Struct& audioProgramme = audioProgramme_List.Items[i];
    string P = Ztring(Name + Ztring::ToZtring(i)).To_UTF8();
    F.Fill(Stream_Audio, 0, P.c_str(), Summary.empty() ? __T("Yes") : Summary);
    F.Fill(Stream_Audio, 0, (P + " Pos").c_str(), i);
    F.Fill_SetOptions(Stream_Audio, 0, (P + " Pos").c_str(), "N NIY");
    return P;
}

static void Apply_SubStreams(File__Analyze& F, const string& P_And_LinkedTo, const Item_Struct& Source, size_t i, const Items_Struct& Dest) {
    ZtringList SubstreamPos, SubstreamNum;
    for (size_t j = 0; j < Source.StringVectors[i].size(); j++) {
        const string& ID = Source.StringVectors[i][j];
        size_t Pos = -1;
        for (size_t k = 0; k < Dest.Items.size(); k++) {
            if (Dest.Items[k].Strings[0] == ID) {
                Pos = k;
                break;
            }
        }
        if (Pos == -1) {
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

class file_adm_private
{
public:
    // In
    bool IsSub;

    // Out
    Items_Struct Items[item_Max];
    int Version;
    bool DolbyProfileCanNotBeVersion1;
    vector<profile_info> profileInfos;
    map<string, string> More;

    // Constructor / Destructor
    file_adm_private() {
        Version = 0;
        DolbyProfileCanNotBeVersion1 = false;
    };

    // Actions
    void parse();

    // Helpers
    void chna_Add(int32u Index, const string& TrackUID)
    {
        if (Index >= 0x10000)
            return;
        if (Items[item_audioTrack].Items.empty())
            Items[item_audioTrack].Init(audioTrack_String_Max, audioTrack_StringVector_Max);
        while (Items[item_audioTrack].Items.size() <= Index)
            Items[item_audioTrack].New();
        Item_Struct& Item = Items[item_audioTrack].Items[Items[item_audioTrack].Items.size() - 1];
        Item.StringVectors[audioTrack_audioTrackUIDRef].push_back(TrackUID);
    }

//private:
    tfsxml_string p;

    void coreMetadata();
    void format();
    void audioFormatExtended();
    void frameHeader();
    void transportTrackFormat();
};

void file_adm_private::parse()
{
    tfsxml_string b, v;

    # define STRUCTS(NAME) \
        Items[item_##NAME].Init(NAME##_String_Max, NAME##_StringVector_Max);

    STRUCTS(audioTrack);
    STRUCTS(audioProgramme);
    STRUCTS(audioContent);
    STRUCTS(audioObject);
    STRUCTS(audioPackFormat);
    STRUCTS(audioChannelFormat);
    STRUCTS(audioTrackUID);
    STRUCTS(audioTrackFormat);
    STRUCTS(audioStreamFormat);

    
    #define ELEMENT_START(NAME) \
        if (!tfsxml_strcmp_charp(b, #NAME)) \
        { \
            Item_Struct& NAME##_Content = Items[item_##NAME].New(); \
            for (;;) { \
                if (tfsxml_attr(&p, &b, &v)) \
                    break; \
                if (false) { \
                } \

    #define ELEMENT_MIDDLE(NAME) \
                else if (tfsxml_strcmp_charp(b, "typeLabel")) { \
                    NAME##_Content.Errors[Warning].push_back("Attribute \"" + tfsxml_decode(b) + "\" is out of specs"); \
                } \
            } \
        if (!tfsxml_enter(&p)) \
        { \
            for (;;) { \
                if (tfsxml_next(&p, &b)) \
                    break; \
                if (false) { \
                } \

    #define ELEMENT_END(NAME) \
                    else if (tfsxml_strcmp_charp(b, "loudnessMetadata") && tfsxml_strcmp_charp(b, "authoringInformation") && tfsxml_strcmp_charp(b, "alternativeValueSetIDRef")) { \
                        NAME##_Content.Errors[Warning].push_back("Element \"" + tfsxml_decode(b) + "\" is out of specs"); \
                    } \
                } \
            } \
        } \

    #define ATTRIBUTE(NAME,ATTR) \
        else if (!tfsxml_strcmp_charp(b, #ATTR)) { \
            NAME##_Content.Strings[NAME##_##ATTR].assign(tfsxml_decode(v)); \
        } \

    #define ATTRIBUTE_I(NAME,ATTR) \
        else if (!tfsxml_strcmp_charp(b, #ATTR)) { \
        } \

    #define ELEMENT(NAME,ELEM) \
        else if (!tfsxml_strcmp_charp(b, #ELEM)) { \
            tfsxml_value(&p, &b); \
            NAME##_Content.StringVectors[NAME##_##ELEM].push_back(tfsxml_decode(b)); \
        } \

    #define ELEMENT_I(NAME,ELEM) \
        else if (!tfsxml_strcmp_charp(b, #ELEM)) { \
        } \

    for (;;) {
        if (tfsxml_next(&p, &b))
            break;
        if (!tfsxml_strcmp_charp(b, "audioFormatExtended"))
        {
            audioFormatExtended();
        }
        if (!tfsxml_strcmp_charp(b, "ebuCoreMain"))
        {
            while (!tfsxml_attr(&p, &b, &v)) {
                if (!tfsxml_strcmp_charp(b, "xmlns") || !tfsxml_strcmp_charp(b, "xsi:schemaLocation")) {
                    DolbyProfileCanNotBeVersion1 = false;
                    if (!tfsxml_strstr_charp(v, "ebuCore_2014").len && !tfsxml_strstr_charp(v, "ebuCore_2016").len) {
                        DolbyProfileCanNotBeVersion1 = true;
                    }
                    if (!DolbyProfileCanNotBeVersion1)
                        break;
                }
            }
            if (!tfsxml_enter(&p))
            {
                for (;;) {
                    if (tfsxml_next(&p, &b))
                        break;
                    if (!tfsxml_strcmp_charp(b, "coreMetadata"))
                    {
                        coreMetadata();
                    }
                }
            }
        }
        if (!tfsxml_strcmp_charp(b, "frame"))
        {
            format();
        }
        if (!tfsxml_strcmp_charp(b, "format"))
        {
            format();
        }
    }
}

void file_adm_private::coreMetadata()
{
    tfsxml_string b;

    tfsxml_enter(&p);
    for (;;) {
        if (tfsxml_next(&p, &b))
            break;
        if (!tfsxml_strcmp_charp(b, "format"))
        {
            format();
        }
    }
}

void file_adm_private::format()
{
    tfsxml_string b, v;

    tfsxml_enter(&p);
    for (;;) {
        if (tfsxml_next(&p, &b))
            break;
        if (!tfsxml_strcmp_charp(b, "audioFormatCustom")) {
            tfsxml_enter(&p);
            while (!tfsxml_next(&p, &b)) {
                if (!tfsxml_strcmp_charp(b, "audioFormatCustomSet")) {
                    tfsxml_enter(&p);
                    while (!tfsxml_next(&p, &b)) {
                        if (!tfsxml_strcmp_charp(b, "admInformation")) {
                            tfsxml_enter(&p);
                            while (!tfsxml_next(&p, &b)) {
                                if (!tfsxml_strcmp_charp(b, "profile")) {
                                    profileInfos.resize(profileInfos.size() + 1);
                                    profile_info& profileInfo = profileInfos[profileInfos.size() - 1];
                                    while (!tfsxml_attr(&p, &b, &v)) {
                                        for (size_t i = 0; i < profile_names_size; i++)
                                        {
                                            if (!tfsxml_strcmp_charp(b, profile_names[i])) {
                                                profileInfo.Strings[i] = tfsxml_decode(v);
                                                if (!i && profileInfo.Strings[0].size() >= 12 && !profileInfo.Strings[0].compare(profileInfo.Strings[0].size() - 12, 12, " ADM Profile"))
                                                    profileInfo.Strings[0].resize(profileInfo.Strings[0].size() - 12);
                                            }
                                        }
                                    }
                                    while (!tfsxml_next(&p, &b)) {
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if (!tfsxml_strcmp_charp(b, "audioFormatExtended")) {
            audioFormatExtended();
        }
        if (!tfsxml_strcmp_charp(b, "frameHeader")) {
            frameHeader();
        }
    }
}

void file_adm_private::audioFormatExtended()
{
    tfsxml_string b, v;

    while (!tfsxml_attr(&p, &b, &v)) {
        if (!tfsxml_strcmp_charp(b, "version")) {
            if (!tfsxml_strcmp_charp(v, "ITU-R_BS.2076-1")) {
                Version = 1;
            }
            if (!tfsxml_strcmp_charp(v, "ITU-R_BS.2076-2")) {
                Version = 2;
            }
        }
    }
    tfsxml_enter(&p);
    for (;;) {
        if (tfsxml_next(&p, &b))
            break;
        ELEMENT_START(audioProgramme)
            ATTRIBUTE(audioProgramme, audioProgrammeID)
            ATTRIBUTE(audioProgramme, audioProgrammeName)
            ATTRIBUTE(audioProgramme, audioProgrammeLanguage)
            ATTRIBUTE(audioProgramme, start)
            ATTRIBUTE(audioProgramme, end)
        ELEMENT_MIDDLE(audioProgramme)
            ELEMENT(audioProgramme, audioContentIDRef)
            else if (!tfsxml_strcmp_charp(b, "audioProgrammeLabel")) {
                string Language;
                for (;;) {
                    if (tfsxml_attr(&p, &b, &v))
                        break;
                    if (!tfsxml_strcmp_charp(b, "language")) {
                        Language += tfsxml_decode(v);
                    }
                }
                tfsxml_value(&p, &b);
                string Value = tfsxml_decode(b);
                if (!Value.empty() && !Language.empty()) {
                    Value.insert(0, '(' + Language + ')');
                }
                audioProgramme_Content.StringVectors[audioProgramme_audioProgrammeLabel].push_back(Value);
            }
        ELEMENT_END(audioProgramme)
        ELEMENT_START(audioContent)
            ATTRIBUTE(audioContent, audioContentID)
            ATTRIBUTE(audioContent, audioContentName)
            ATTRIBUTE(audioContent, audioContentLanguage)
            ATTRIBUTE(audioContent, typeLabel)
        ELEMENT_MIDDLE(audioContent)
            ELEMENT(audioContent, audioObjectIDRef)
            else if (!tfsxml_strcmp_charp(b, "dialogue")) {
                string Type;
                for (;;) {
                    if (tfsxml_attr(&p, &b, &v))
                        break;
                    if (!tfsxml_strcmp_charp(b, "nonDialogueContentKind")
                        || !tfsxml_strcmp_charp(b, "dialogueContentKind")
                        || !tfsxml_strcmp_charp(b, "mixedContentKind")) {
                        Type += tfsxml_decode(v);
                    }
                }
                tfsxml_value(&p, &b);
                string Value;
                if (!tfsxml_strcmp_charp(b, "0")) {
                    if (Type == "1") {
                        Value = "Music";
                    }
                    else if (Type == "2") {
                        Value = "Effect";
                    }
                    else {
                        Value = "No Dialogue";
                        if (!Type.empty() && Type != "0") {
                            Value += " (" + Type + ')';
                        }
                    }
                }
                else if (!tfsxml_strcmp_charp(b, "1")) {
                    if (Type == "1") {
                        Value = "Music";
                    }
                    else if (Type == "2") {
                        Value = "Effect";
                    }
                    else if (Type == "3") {
                        Value = "Spoken Subtitle";
                    }
                    else if (Type == "4") {
                        Value = "Visually Impaired";
                    }
                    else if (Type == "5") {
                        Value = "Commentary";
                    }
                    else if (Type == "6") {
                        Value = "Emergency";
                    }
                    else {
                        Value = "Dialogue";
                        if (!Type.empty() && Type != "0") {
                            Value += " (" + Type + ')';
                        }
                    }
                }
                else if (!tfsxml_strcmp_charp(b, "2")) {
                    if (Type == "1") {
                        Value = "Complete Main";
                    }
                    else if (Type == "2") {
                        Value = "Mixed (Mixed)";
                    }
                    else if (Type == "3") {
                        Value = "Hearing Impaired";
                    }
                    else {
                        Value = "Mixed";
                        if (!Type.empty() && Type != "0") {
                            Value += " (" + Type + ')';
                        }
                    }
                }
                else {
                    Value = tfsxml_decode(b);
                    if (!Type.empty()) {
                        Value += " (" + Type + ')';
                    }
                }
                audioContent_Content.StringVectors[audioContent_dialogue].push_back(Value);
            }
            else if (!tfsxml_strcmp_charp(b, "audioContentLabel")) {
                string Language;
                for (;;) {
                    if (tfsxml_attr(&p, &b, &v))
                        break;
                    if (!tfsxml_strcmp_charp(b, "language")) {
                        Language += tfsxml_decode(v);
                    }
                }
                tfsxml_value(&p, &b);
                string Value = tfsxml_decode(b);
                if (!Value.empty() && !Language.empty()) {
                    Value.insert(0, '(' + Language + ')');
                }
                audioContent_Content.StringVectors[audioContent_audioContentLabel].push_back(Value);
            }
            if (!tfsxml_strcmp_charp(b, "loudnessMetadata")) {
                if (!tfsxml_enter(&p))
                {
                    for (;;) {
                        if (tfsxml_next(&p, &b))
                            break;
                        if (!tfsxml_strcmp_charp(b, "integratedLoudness")) {
                            tfsxml_value(&p, &b);
                            audioContent_Content.StringVectors[audioContent_integratedLoudness].push_back(tfsxml_decode(b));
                        }
                    }
                }
            }
            ELEMENT(audioContent, dialogue)
        ELEMENT_END(audioContent)
        ELEMENT_START(audioObject)
            ATTRIBUTE(audioObject, audioObjectID)
            ATTRIBUTE(audioObject, audioObjectName)
            ATTRIBUTE(audioObject, duration)
            ATTRIBUTE(audioObject, startTime)
            ATTRIBUTE(audioObject, typeLabel)
        ELEMENT_MIDDLE(audioObject)
            ELEMENT(audioObject, audioPackFormatIDRef)
            ELEMENT(audioObject, audioTrackUIDRef)
        ELEMENT_END(audioObject)
        ELEMENT_START(audioPackFormat)
            ATTRIBUTE(audioPackFormat, audioPackFormatID)
            ATTRIBUTE(audioPackFormat, audioPackFormatName)
            ATTRIBUTE(audioPackFormat, typeDefinition)
            ATTRIBUTE(audioPackFormat, typeLabel)
        ELEMENT_MIDDLE(audioPackFormat)
            ELEMENT(audioPackFormat, audioChannelFormatIDRef)
        ELEMENT_END(audioPackFormat)
        ELEMENT_START(audioChannelFormat)
            ATTRIBUTE(audioChannelFormat, audioChannelFormatID)
            ATTRIBUTE(audioChannelFormat, audioChannelFormatName)
            ATTRIBUTE(audioChannelFormat, typeDefinition)
            ATTRIBUTE(audioChannelFormat, typeLabel)
        ELEMENT_MIDDLE(audioChannelFormat)
            else if (!tfsxml_strcmp_charp(b, "audioBlockFormat")) {
                if (!tfsxml_enter(&p))
                {
                    for (;;) {
                        if (tfsxml_next(&p, &b))
                            break;
                        if (!tfsxml_strcmp_charp(b, "speakerLabel")) {
                            tfsxml_value(&p, &b);
                            string SpeakerLabel = tfsxml_decode(b);
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
                                tfsxml_leave(&p); // audioBlockFormat
                                tfsxml_leave(&p); // audioChannelFormat
                                break;
                            }
                        }
                    }
                }
            }
        ELEMENT_END(audioChannelFormat)
        ELEMENT_START(audioTrackUID)
            ATTRIBUTE(audioTrackUID, UID)
            ATTRIBUTE(audioTrackUID, bitDepth)
            ATTRIBUTE(audioTrackUID, sampleRate)
            ATTRIBUTE(audioTrackUID, typeLabel)
        ELEMENT_MIDDLE(audioTrackUID)
            ELEMENT(audioTrackUID, audioChannelFormatIDRef)
            ELEMENT(audioTrackUID, audioPackFormatIDRef)
            ELEMENT(audioTrackUID, audioTrackFormatIDRef)
        ELEMENT_END(audioTrackUID)
        ELEMENT_START(audioTrackFormat)
            ATTRIBUTE(audioTrackFormat, audioTrackFormatID)
            ATTRIBUTE(audioTrackFormat, audioTrackFormatName)
            ATTRIBUTE(audioTrackFormat, formatDefinition)
            ATTRIBUTE(audioTrackFormat, typeDefinition)
            ATTRIBUTE(audioTrackFormat, typeLabel)
        ELEMENT_MIDDLE(audioTrackFormat)
            ELEMENT(audioTrackFormat, audioStreamFormatIDRef)
        ELEMENT_END(audioTrackFormat)
        ELEMENT_START(audioStreamFormat)
            ATTRIBUTE(audioStreamFormat, audioStreamFormatID)
            ATTRIBUTE(audioStreamFormat, audioStreamFormatName)
            ATTRIBUTE(audioStreamFormat, formatDefinition)
            ATTRIBUTE(audioStreamFormat, formatLabel)
            ATTRIBUTE(audioStreamFormat, typeDefinition)
            ATTRIBUTE(audioStreamFormat, typeLabel)
        ELEMENT_MIDDLE(audioStreamFormat)
            ELEMENT(audioStreamFormat, audioChannelFormatIDRef)
            ELEMENT(audioStreamFormat, audioPackFormatIDRef)
            ELEMENT(audioStreamFormat, audioTrackFormatIDRef)
        ELEMENT_END(audioStreamFormat)
    }
}

void file_adm_private::frameHeader()
{
    tfsxml_string a, v;
    tfsxml_enter(&p);
    while (!tfsxml_next(&p, &v)) {
        if (!tfsxml_strcmp_charp(v, "frameFormat")) {
            while (!tfsxml_attr(&p, &a, &v)) {
                if (!tfsxml_strcmp_charp(a, "duration")) {
                    if (IsSub)
                        More["FrameRate"] = Ztring().From_Number(1 / TimeCodeToFloat(tfsxml_decode(v))).To_UTF8();
                    else
                        More["Duration"] = Ztring().From_Number(TimeCodeToFloat(tfsxml_decode(v)) * 1000, 0).To_UTF8();
                }
                if (!tfsxml_strcmp_charp(a, "flowID")) {
                    More["FlowID"] = tfsxml_decode(v);
                }
                if (!tfsxml_strcmp_charp(a, "start")) {
                    More["TimeCode_FirstFrame"] = tfsxml_decode(v);
                }
                if (!tfsxml_strcmp_charp(a, "type")) {
                    More["Metadata_Format_Type"] = tfsxml_decode(v);
                }
            }
        }
        if (!tfsxml_strcmp_charp(v, "transportTrackFormat")) {
            transportTrackFormat();
        }
    }
}

void file_adm_private::transportTrackFormat()
{
    Items[item_audioTrack].Items.clear();
    tfsxml_string b, v;
    int32u trackID = 0;
    tfsxml_enter(&p);
    for (;;) {
        if (tfsxml_next(&p, &b))
            break;
        ELEMENT_START(audioTrack)
            else if (!tfsxml_strcmp_charp(b, "trackID")) {
                trackID = Ztring().From_UTF8(v.buf, v.len).To_int32u();
            }
        ELEMENT_MIDDLE(audioTrack)
            else if (!tfsxml_strcmp_charp(b, "audioTrackUIDRef")) {
                tfsxml_value(&p, &b);
                chna_Add(trackID, tfsxml_decode(b));
            }
        ELEMENT_END(audioTrack)
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
void File_Adm::Streams_Fill()
{
    // Clean up
    if (!File_Adm_Private->Items[item_audioTrack].Items.empty())
    {
        if (File_Adm_Private->Items[item_audioTrack].Items[0].StringVectors[audioTrack_audioTrackUIDRef].empty())
            File_Adm_Private->Items[item_audioTrack].Items.erase(File_Adm_Private->Items[item_audioTrack].Items.begin());
    }

    #define FILL_COUNT(NAME,FIELD) \
        if (!File_Adm_Private->Items[item_##NAME].Items.empty()) \
            Fill(Stream_Audio, 0, "NumberOf" FIELD "s", File_Adm_Private->Items[item_##NAME].Items.size());

    #define FILL_START(NAME,ATTRIBUTE,FIELD) \
        for (size_t i = 0; i < File_Adm_Private->Items[item_##NAME].Items.size(); i++) { \
            Ztring Summary = Ztring().From_UTF8(File_Adm_Private->Items[item_##NAME].Items[i].Strings[NAME##_##ATTRIBUTE]); \
            string P = Apply_Init(*this, __T(FIELD), i, File_Adm_Private->Items[item_##NAME], Summary); \

    #define FILL_A(NAME,ATTRIBUTE,FIELD) \
        Fill(Stream_Audio, StreamPos_Last, (P + ' ' + FIELD).c_str(), File_Adm_Private->Items[item_##NAME].Items[i].Strings[NAME##_##ATTRIBUTE].c_str(), Unlimited, true, true); \

    #define FILL_E(NAME,ATTRIBUTE,FIELD) \
        { \
        ZtringList List; \
        List.Separator_Set(0, " + "); \
        for (size_t j = 0; j < File_Adm_Private->Items[item_##NAME].Items[i].StringVectors[NAME##_##ATTRIBUTE].size(); j++) { \
            List.push_back(Ztring().From_UTF8(File_Adm_Private->Items[item_##NAME].Items[i].StringVectors[NAME##_##ATTRIBUTE][j].c_str())); \
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

    Fill(Stream_Audio, StreamPos_Last, "Metadata_Format", "ADM, Version " + Ztring::ToZtring(File_Adm_Private->Version).To_UTF8() + File_Adm_Private->More["Metadata_Format"]);
    if (!MuxingMode.empty())
        Fill(Stream_Audio, StreamPos_Last, "Metadata_MuxingMode", MuxingMode);
    for (map<string, string>::iterator It = File_Adm_Private->More.begin(); It != File_Adm_Private->More.end(); ++It)
        //if (It->first != "Metadata_Format")
            Fill(Stream_Audio, StreamPos_Last, It->first.c_str(), It->second);
    if (File_Adm_Private->Items[item_audioProgramme].Items.size() == 1 && File_Adm_Private->Items[item_audioProgramme].Items[0].Strings[audioProgramme_audioProgrammeName] == "Atmos_Master") {
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
    for (size_t i = 0; i < item_Max; i++)
        TotalCount += File_Adm_Private->Items[i].Items.size();
    bool Full = TotalCount < 0x100 ? true : false;
    FILL_COUNT(audioProgramme, "Programme");
    FILL_COUNT(audioContent, "Content");
    FILL_COUNT(audioObject, "Object");
    FILL_COUNT(audioPackFormat, "PackFormat");
    FILL_COUNT(audioChannelFormat, "ChannelFormat");
    if (Full)
    {
        FILL_COUNT(audioTrackUID, "TrackUID");
        FILL_COUNT(audioTrackFormat, "TrackFormat");
        FILL_COUNT(audioStreamFormat, "StreamFormat");
    }
    #if MEDIAINFO_ADVANCED
    vector<string> Errors_Field[error_Type_Max];
    vector<string> Errors_Value[error_Type_Max];
    MediaInfo_Config::adm_profile Config_AdmProfile=MediaInfoLib::Config.AdmProfile();
    bool WarningError=MediaInfoLib::Config.WarningError();
    #endif

    FILL_START(audioProgramme, audioProgrammeName, "Programme")
        if (Full)
            FILL_A(audioProgramme, audioProgrammeID, "ID");
        FILL_A(audioProgramme, audioProgrammeName, "Title");
        FILL_E(audioProgramme, audioProgrammeLabel, "Label");
        FILL_A(audioProgramme, audioProgrammeLanguage, "Language");
        FILL_A(audioProgramme, start, "Start");
        FILL_A(audioProgramme, end, "End");
        LINK(audioProgramme, "Content", audioContentIDRef, audioContent);
        const Ztring& Label = Retrieve_Const(StreamKind_Last, StreamPos_Last, (P + " Label").c_str());
        if (!Label.empty()) {
            Summary += __T(' ');
            Summary += __T('(');
            Summary += Label;
            Summary += __T(')');
            Fill(StreamKind_Last, StreamPos_Last, P.c_str(), Summary, true);
        }

        // Errors
        const string& audioProgrammeID = File_Adm_Private->Items[item_audioProgramme].Items[i].Strings[audioProgramme_audioProgrammeID];
        if (audioProgrammeID.size() != 8
            || audioProgrammeID[0] != 'A'
            || audioProgrammeID[1] != 'P'
            || audioProgrammeID[2] != 'R'
            || audioProgrammeID[3] != '_'
            || !IsHexaDigit(audioProgrammeID[4])
            || !IsHexaDigit(audioProgrammeID[5])
            || !IsHexaDigit(audioProgrammeID[6])
            || !IsHexaDigit(audioProgrammeID[7])
            ) {
            File_Adm_Private->Items[item_audioProgramme].Items[i].Errors->push_back(audioProgrammeID + " is not a valid form (APR_wwww form, wwww is hexadecimal value)");
        }

        #if MEDIAINFO_ADVANCED
        for (size_t k = 0; k < error_Type_Max; k++) {
            if (!File_Adm_Private->Items[item_audioProgramme].Items[i].Errors[k].empty()) {
                for (size_t j = 0; j < File_Adm_Private->Items[item_audioProgramme].Items[i].Errors[k].size(); j++) {
                    Errors_Field[WarningError?Error:k].push_back("audioProgramme");
                    Errors_Value[WarningError?Error:k].push_back(File_Adm_Private->Items[item_audioProgramme].Items[i].Errors[k][j]);
                }
            }
        }

        //TODO: expand this proof of concept
        if (Config_AdmProfile.Ebu3392==1)
        {
            const string& audioProgrammeName = File_Adm_Private->Items[item_audioProgramme].Items[i].Strings[audioProgramme_audioProgrammeName];
            if (Ztring().From_UTF8(audioProgrammeName).size() > 32)
            {
                Errors_Field[Error].push_back("audioProgramme");
                Errors_Value[Error].push_back("audioProgrammeName length is more than EBU Tech 3392 constraint");
            }
        }
        #endif // MEDIAINFO_ADVANCED
    }

    FILL_START(audioContent, audioContentName, "Content")
        if (Full)
            FILL_A(audioContent, audioContentID, "ID");
        FILL_A(audioContent, audioContentName, "Title");
        FILL_E(audioContent, audioContentLabel, "Label");
        FILL_A(audioContent, audioContentLanguage, "Language");
        FILL_E(audioContent, dialogue, "Mode");
        FILL_E(audioContent, integratedLoudness, "IntegratedLoudness");
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

    FILL_START(audioObject, audioObjectName, "Object")
        if (Full)
            FILL_A(audioObject, audioObjectID, "ID");
        FILL_A(audioObject, audioObjectName, "Title");
        FILL_A(audioObject, startTime, "Start");
        FILL_A(audioObject, duration, "Duration");
        LINK(audioObject, "PackFormat", audioPackFormatIDRef, audioPackFormat);
        if (Full)
            LINK(audioObject, "TrackUID", audioTrackUIDRef, audioTrackUID);
    }

    FILL_START(audioPackFormat, audioPackFormatName, "PackFormat")
        if (Full)
            FILL_A(audioPackFormat, audioPackFormatID, "ID");
        FILL_A(audioPackFormat, audioPackFormatName, "Title");
        FILL_A(audioPackFormat, typeDefinition, "TypeDefinition");
        const Item_Struct& Source = File_Adm_Private->Items[item_audioPackFormat].Items[i];
        const Items_Struct& Dest = File_Adm_Private->Items[item_audioChannelFormat];
        string SpeakerLabel;
        for (size_t j = 0; j < Source.StringVectors[audioPackFormat_audioChannelFormatIDRef].size(); j++) {
            const string& ID = Source.StringVectors[audioPackFormat_audioChannelFormatIDRef][j];
            for (size_t k = 0; k < Dest.Items.size(); k++) {
                if (Dest.Items[k].Strings[audioChannelFormat_audioChannelFormatID] != ID) {
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

    FILL_START(audioChannelFormat, audioChannelFormatName, "ChannelFormat")
        if (Full)
            FILL_A(audioChannelFormat, audioChannelFormatID, "ID");
        FILL_A(audioChannelFormat, audioChannelFormatName, "Title");
        FILL_A(audioChannelFormat, typeDefinition, "TypeDefinition");
        for (map<string, string>::iterator Extra = File_Adm_Private->Items[item_audioChannelFormat].Items[i].Extra.begin(); Extra != File_Adm_Private->Items[item_audioChannelFormat].Items[i].Extra.end(); ++Extra) {
            Fill(Stream_Audio, StreamPos_Last, (P + ' ' + Extra->first).c_str(), Extra->second);
        }
    }

    if (Full) {
        FILL_START(audioTrackUID, UID, "TrackUID")
            FILL_A(audioTrackUID, UID, "ID");
            FILL_A(audioTrackUID, bitDepth, "BitDepth");
            FILL_A(audioTrackUID, sampleRate, "SamplingRate");
            string& ID = File_Adm_Private->Items[item_audioTrackUID].Items[i].Strings[audioTrackUID_UID];
            for (size_t j = 0; j < File_Adm_Private->Items[item_audioTrack].Items.size(); j++)
            {
                Item_Struct& Item = File_Adm_Private->Items[item_audioTrack].Items[j];
                vector<string>& Ref = Item.StringVectors[audioTrack_audioTrackUIDRef];
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

        FILL_START(audioTrackFormat, audioTrackFormatName, "TrackFormat")
            FILL_A(audioTrackFormat, audioTrackFormatID, "ID");
            FILL_A(audioTrackFormat, audioTrackFormatName, "Title");
            FILL_A(audioTrackFormat, formatDefinition, "FormatDefinition");
            FILL_A(audioTrackFormat, typeDefinition, "TypeDefinition");
            LINK(audioTrackFormat, "StreamFormat", audioStreamFormatIDRef, audioStreamFormat);
        }

        FILL_START(audioStreamFormat, audioStreamFormatName, "StreamFormat")
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
        FILL_START(audioTrack, Summary, "Transport0 TrackIndex")
            FILL_E(audioTrack, audioTrackUIDRef, "");
            if (Full)
                LINK(audioTrack, "TrackUID", audioTrackUIDRef, audioTrackUID);
        }

    }

    if (!Full)
        Fill(Stream_Audio, 0, "PartialDisplay", "Yes");

    #if MEDIAINFO_ADVANCED
    for (size_t k = 0; k < error_Type_Max; k++) {
        if (!Errors_Field[k].empty()) {
            Fill(StreamKind_Last, StreamPos_Last, error_Type_String[k], Errors_Field[k].size());
            for (size_t i = 0; i < Errors_Field[k].size(); i++) {
                Fill(StreamKind_Last, StreamPos_Last, (string(error_Type_String[k]) + ' ' + Errors_Field[k][i]).c_str(), Errors_Value[k][i]);
            }
        }
    }
   #endif // MEDIAINFO_ADVANCED

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

        Element_WaitForMoreData();
        return false; //Must wait for more data
    }

    if (tfsxml_init(&File_Adm_Private->p, (void*)(Buffer), Buffer_Size))
        return true;
    File_Adm_Private->IsSub = IsSub;
    File_Adm_Private->parse();
    if (File_Adm_Private->Items[item_audioContent].Items.empty())
    {
        Reject();
        return false;
    }

    //All should be OK...
    Accept("ADM");
    return true;
}

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


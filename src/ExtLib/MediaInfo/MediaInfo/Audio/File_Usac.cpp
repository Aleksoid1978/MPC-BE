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
#if defined(MEDIAINFO_AAC_YES) || defined(MEDIAINFO_MPEGH3DA_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Usac.h"
#include <algorithm>
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Info
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_CONFORMANCE
const char* ConformanceLevel_Strings[]=
{
    "Errors",
    "Warnings",
    "Infos",
};
static_assert(sizeof(ConformanceLevel_Strings)/sizeof(const char*)==File_Usac::ConformanceLevel_Max, "");
#endif

//---------------------------------------------------------------------------
extern int8u Aac_AudioSpecificConfig_sampling_frequency_index(const int64s sampling_frequency);
extern const size_t Aac_sampling_frequency_Size_Usac;
extern const int32u Aac_sampling_frequency[];
extern const int8u Aac_Channels_Size_Usac;
extern const int8u Aac_Channels[];
extern string Aac_Channels_GetString(int8u ChannelLayout);
extern string Aac_ChannelConfiguration_GetString(int8u ChannelLayout);
extern string Aac_ChannelConfiguration2_GetString(int8u ChannelLayout);
extern string Aac_ChannelLayout_GetString(int8u ChannelLayout, bool IsMpegH=false);
extern string Aac_OutputChannelPosition_GetString(int8u OutputChannelPosition);

//---------------------------------------------------------------------------
enum usacElementType_Value
{
    ID_USAC_SCE,
    ID_USAC_CPE,
    ID_USAC_LFE,
    ID_USAC_EXT,
    ID_USAC_Max
};
static const char* usacElementType_IdNames[]=
{
    "ID_USAC_SCE",
    "ID_USAC_CPE",
    "ID_USAC_LFE",
    "ID_USAC_EXT",
};
static_assert(sizeof(usacElementType_IdNames)/sizeof(const char*)==ID_USAC_Max, "");

//---------------------------------------------------------------------------
enum usacExtElementType_Value
{
    ID_EXT_ELE_FILL,
    ID_EXT_ELE_MPEGS,
    ID_EXT_ELE_SAOC,
    ID_EXT_ELE_AUDIOPREROLL,
    ID_EXT_ELE_UNI_DRC,
    ID_EXT_ELE_Max
};
const char* usacExtElementType_IdNames[]=
{
    "ID_EXT_ELE_FILL",
    "ID_EXT_ELE_MPEGS",
    "ID_EXT_ELE_SAOC",
    "ID_EXT_ELE_AUDIOPREROLL",
    "ID_EXT_ELE_UNI_DRC",
};
static_assert(sizeof(usacExtElementType_IdNames)/sizeof(const char*)==ID_EXT_ELE_Max, "");
const char* usacExtElementType_Names[] =
{
    "Fill",
    "SpatialFrame",
    "SaocFrame",
    "AudioPreRoll",
    "uniDrcGain",
};
static_assert(sizeof(usacExtElementType_Names)/sizeof(const char*)==ID_EXT_ELE_Max, "");
const char* usacExtElementType_ConfigNames[] =
{
    "",
    "SpatialSpecificConfig",
    "SaocSpecificConfig",
    "",
    "uniDrcConfig",
};
static_assert(sizeof(usacExtElementType_ConfigNames)/sizeof(const char*)==ID_EXT_ELE_Max, "");

//---------------------------------------------------------------------------
enum loudnessInfoSetExtType_Value
{
    UNIDRCLOUDEXT_TERM,
    UNIDRCLOUDEXT_EQ,
    UNIDRCLOUDEXT_Max
};
static const char* loudnessInfoSetExtType_IdNames[]=
{
    "UNIDRCLOUDEXT_TERM",
    "UNIDRCLOUDEXT_EQ",
};
static_assert(sizeof(loudnessInfoSetExtType_IdNames)/sizeof(const char*)==UNIDRCLOUDEXT_Max, "");
static const char* loudnessInfoSetExtType_ConfNames[]=
{
    "",
    "V1loudnessInfo",
};
static_assert(sizeof(loudnessInfoSetExtType_ConfNames)/sizeof(const char*)==UNIDRCLOUDEXT_Max, "");

//---------------------------------------------------------------------------
enum uniDrcConfigExtType_Value
{
    UNIDRCCONFEXT_TERM,
    UNIDRCCONFEXT_PARAM_DRC,
    UNIDRCCONFEXT_V1,
    UNIDRCCONFEXT_Max
};
static const char* uniDrcConfigExtType_IdNames[]=
{
    "UNIDRCCONFEXT_TERM",
    "UNIDRCCONFEXT_PARAM_DRC",
    "UNIDRCCONFEXT_V1",
};
static_assert(sizeof(uniDrcConfigExtType_IdNames)/sizeof(const char*)==UNIDRCCONFEXT_Max, "");
static const char* uniDrcConfigExtType_ConfNames[]=
{
    "",
    "ParametricDrc",
    "V1Drc",
};
static_assert(sizeof(uniDrcConfigExtType_ConfNames)/sizeof(const char*)==UNIDRCCONFEXT_Max, "");

//---------------------------------------------------------------------------
enum usacConfigExtType_Value
{
    ID_CONFIG_EXT_FILL,
    ID_CONFIG_EXT_1,
    ID_CONFIG_EXT_LOUDNESS_INFO,
    ID_CONFIG_EXT_3,
    ID_CONFIG_EXT_4,
    ID_CONFIG_EXT_5,
    ID_CONFIG_EXT_6,
    ID_CONFIG_EXT_STREAM_ID,
    ID_CONFIG_EXT_Max
};
static const char* usacConfigExtType_IdNames[]=
{
    "ID_CONFIG_EXT_FILL",
    "",
    "ID_CONFIG_EXT_LOUDNESS_INFO",
    "",
    "",
    "",
    "",
    "ID_CONFIG_EXT_STREAM_ID",
};
static_assert(sizeof(usacConfigExtType_IdNames)/sizeof(const char*)==ID_CONFIG_EXT_Max, "");
static const char* usacConfigExtType_ConfNames[]=
{
    "ConfigExtFill",
    "",
    "loudnessInfoSet",
    "",
    "",
    "",
    "",
    "streamId",
};
static_assert(sizeof(usacConfigExtType_ConfNames)/sizeof(const char*)==ID_CONFIG_EXT_Max, "");

//---------------------------------------------------------------------------
struct coreSbrFrameLengthIndex_mapping
{
    int8u    sbrRatioIndex;
    int8u    outputFrameLengthDivided256;
};
const size_t coreSbrFrameLengthIndex_Mapping_Size=5;
coreSbrFrameLengthIndex_mapping coreSbrFrameLengthIndex_Mapping[coreSbrFrameLengthIndex_Mapping_Size]=
{
    { 0,  3 },
    { 0,  4 },
    { 2,  8 },
    { 3,  8 },
    { 1, 16 },
};

//---------------------------------------------------------------------------
static const size_t LoudnessMeaning_Size=9;
static const char* LoudnessMeaning[LoudnessMeaning_Size]=
{
    "Program",
    "Anchor",
    "MaximumOfRange",
    "MaximumMomentary",
    "MaximumShortTerm",
    "Range",
    "ProductionMixingLevel",
    "RoomType",
    "ShortTerm",
};

static Ztring Loudness_Value(int8u methodDefinition, int8u methodValue)
{
    switch (methodDefinition)
    {
        case 0:
                return Ztring();
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
                return Ztring::ToZtring(-57.75+((double)methodValue)/4, 2)+__T(" LKFS");
        case 6: 
                {
                int8u Value;
                if (methodValue==0)
                    Value=0;
                else if (methodValue<=128)
                    Value=methodValue/4;
                else if (methodValue<=204)
                    Value=methodValue/2-32;
                else
                    Value=methodValue-134; 
                return Ztring::ToZtring(Value)+__T(" LU");
                }
        case 7: 
                return Ztring::ToZtring(80+methodValue)+__T(" dB");
        case 8:
                switch(methodValue)
                {
                    case 0: return Ztring();
                    case 1: return __T("Large room");
                    case 2: return __T("Small room");
                    default: return Ztring::ToZtring(methodValue);
                }
        case 9:
                return Ztring::ToZtring(-116+((double)methodValue)/2, 1)+__T(" LKFS");
        default:
                return Ztring().From_Number(methodValue);
    }
}

//---------------------------------------------------------------------------
static const char* measurementSystems[]=
{
    "",
    "EBU R-128",
    "ITU-R BS.1770-4",
    "ITU-R BS.1770-4 with pre-processing",
    "User",
    "Expert/panel",
    "ITU-R BS.1771-1",
    "Reserved Measurement System A",
    "Reserved Measurement System B",
    "Reserved Measurement System C",
    "Reserved Measurement System D",
    "Reserved Measurement System E",
};
auto measurementSystems_Size=sizeof(measurementSystems)/sizeof(const char*);

//---------------------------------------------------------------------------
static const char* reliabilities[]=
{
    "",
    "Unverified",
    "Not to exceed ceiling",
    "Accurate",
};
auto reliabilities_Size=sizeof(reliabilities)/sizeof(const char*);

//---------------------------------------------------------------------------
static const char* drcSetEffect_List[]=
{
    "Night",
    "Noisy",
    "Limited",
    "LowLevel",
    "Dialog",
    "General",
    "Expand",
    "Artistic",
    "Clipping",
    "Fade",
    "DuckOther",
    "DuckSelf",
};
auto drcSetEffect_List_Size=sizeof(drcSetEffect_List)/sizeof(const char*);

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Usac::File_Usac()
:File__Analyze()
{
    channelConfiguration=(int8u)-1;
    sampling_frequency_index=(int8u)-1;
    extension_sampling_frequency_index=(int8u)-1;
    IsParsingRaw=0;
    #if MEDIAINFO_CONFORMANCE
        Format_Profile=AudioProfile_Max;
    #endif
}

//---------------------------------------------------------------------------
File_Usac::~File_Usac()
{
}

//***********************************************************************
// BS_Bookmark
//***********************************************************************

//---------------------------------------------------------------------------
File_Usac::bs_bookmark File_Usac::BS_Bookmark(size_t NewSize)
{
    bs_bookmark B;
    if (Data_BS_Remain()>=NewSize)
        B.End=Data_BS_Remain()-NewSize;
    else
        B.End=Data_BS_Remain();
    B.Element_Offset=Element_Offset;
    B.Trusted=Trusted;
    B.UnTrusted=Element[Element_Level].UnTrusted;
    B.NewSize=NewSize;
    B.BitsNotIncluded=B.End%8;
    if (B.BitsNotIncluded)
        B.NewSize+=B.BitsNotIncluded;
    BS->Resize(B.NewSize);
    #if MEDIAINFO_CONFORMANCE
        for (size_t Level=0; Level<ConformanceLevel_Max; Level++)
            B.ConformanceErrors[Level]=ConformanceErrors[Level];
    #endif
    return B;
}

//---------------------------------------------------------------------------
#if MEDIAINFO_CONFORMANCE
bool File_Usac::BS_Bookmark(File_Usac::bs_bookmark& B, const string& ConformanceFieldName)
#else
bool File_Usac::BS_Bookmark(File_Usac::bs_bookmark& B)
#endif
{
    if (Data_BS_Remain()>B.BitsNotIncluded)
    {
        int8u LastByte=0xFF;
        auto BitsRemaining=Data_BS_Remain()-B.BitsNotIncluded;
        if (BitsRemaining<8)
            Peek_S1((int8u)(Data_BS_Remain()-B.BitsNotIncluded), LastByte);
        Skip_BS(BitsRemaining,                                  LastByte?"Unknown":"Padding");
    }
    else if (Data_BS_Remain()<B.BitsNotIncluded)
        Trusted_IsNot("Too big");
    #if MEDIAINFO_CONFORMANCE
        bool ToReturn = !Trusted_Get();
        if (ToReturn)
        {
            for (size_t Level=0; Level<ConformanceLevel_Max; Level++)
                ConformanceErrors[Level]=B.ConformanceErrors[Level];
            Fill_Conformance(ConformanceFieldName.c_str(), "Malformed bitstream");
        }
    #endif
    BS->Resize(B.End);
    Element_Offset=B.Element_Offset;
    Trusted=B.Trusted;
    Element[Element_Level].UnTrusted=B.UnTrusted;
    #if MEDIAINFO_CONFORMANCE
        return ToReturn;
    #else
        return true;
    #endif
}

//***********************************************************************
// Conformance
//***********************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_CONFORMANCE
void File_Usac::Fill_Conformance(const char* Field, const char* Value, bitset8 Flags, conformance_level Level)
{
    field_value FieldValue(Field, Value, Flags, (int64u)-1);
    auto& Conformance = ConformanceErrors[Level];
    auto Current = find(Conformance.begin(), Conformance.end(), FieldValue);
    if (Current != Conformance.end())
        return;
    ConformanceErrors[Level].emplace_back(FieldValue);
}
#endif

//---------------------------------------------------------------------------
#if MEDIAINFO_CONFORMANCE
void File_Usac::Clear_Conformance()
{
    for (size_t Level = 0; Level < ConformanceLevel_Max; Level++)
    {
        auto& Conformance = ConformanceErrors[Level];
        Conformance.clear();
    }
}
#endif

//---------------------------------------------------------------------------
#if MEDIAINFO_CONFORMANCE
void File_Usac::Merge_Conformance(bool FromConfig)
{
    for (size_t Level = 0; Level < ConformanceLevel_Max; Level++)
    {
        auto& Conformance = ConformanceErrors[Level];
        auto& Conformance_Total = ConformanceErrors_Total[Level];
        for (const auto& FieldValue : Conformance)
        {
            auto Current = find(Conformance_Total.begin(), Conformance_Total.end(), FieldValue);
            if (Current != Conformance_Total.end())
            {
                if (Current->FramePoss.size() < 8)
                {
                    if (FromConfig)
                    {
                        if (Current->FramePoss.empty() || Current->FramePoss[0] != (int64u)-1)
                            Current->FramePoss.insert(Current->FramePoss.begin(), (int64u)-1);
                    }
                    else
                        Current->FramePoss.push_back(Frame_Count_NotParsedIncluded);
                }
                else if (Current->FramePoss.size() == 8)
                    Current->FramePoss.push_back((int64u)-1); //Indicating "..."
                continue;
            }
            Conformance_Total.push_back(FieldValue);
            if (!FromConfig)
                Conformance_Total.back().FramePoss.front() = Frame_Count_NotParsedIncluded;
        }
        Conformance.clear();
    }
}
#endif

//---------------------------------------------------------------------------
#if MEDIAINFO_CONFORMANCE
void File_Usac::Streams_Finish_Conformance()
{
    Merge_Conformance(true);

    for (size_t Level = 0; Level < ConformanceLevel_Max; Level++)
    {
        auto& Conformance_Total = ConformanceErrors_Total[Level];
        if (Conformance_Total.empty())
            continue;
        for (size_t i = Conformance_Total.size() - 1; i < Conformance_Total.size(); i--) {
            if (!CheckIf(Conformance_Total[i].Flags)) {
                Conformance_Total.erase(Conformance_Total.begin() + i);
            }
        }
        if (Conformance_Total.empty())
            continue;
        string Conformance_String = "Conformance";
        Conformance_String += ConformanceLevel_Strings[Level];
        Fill(Stream_Audio, 0, Conformance_String.c_str(), Conformance_Total.size());
        Conformance_String += ' ';
        for (const auto& ConformanceError : Conformance_Total) {
            size_t Space = 0;
            for (;;) {
                Space = ConformanceError.Field.find(' ', Space + 1);
                if (Space == string::npos) {
                    break;
                }
                const auto Field = Conformance_String + ConformanceError.Field.substr(0, Space);
                const auto& Value = Retrieve_Const(StreamKind_Last, StreamPos_Last, Field.c_str());
                if (Value.empty()) {
                    Fill(StreamKind_Last, StreamPos_Last, Field.c_str(), "Yes");
                }
            }
            auto Value = ConformanceError.Value;
            if (!ConformanceError.FramePoss.empty() && (ConformanceError.FramePoss.size() != 1 || ConformanceError.FramePoss[0] != (int64u)-1))
            {
                auto HasConfError = ConformanceError.FramePoss[0] == (int64u)-1;
                Value += " (";
                if (HasConfError)
                    Value += "conf & ";
                Value += "frame";
                if (ConformanceError.FramePoss.size() - HasConfError > 1)
                    Value += 's';
                Value += ' ';
                for (size_t i = HasConfError; i < ConformanceError.FramePoss.size(); i++)
                {
                    auto FramePos = ConformanceError.FramePoss[i];
                    if (FramePos == (int64u)-1)
                        Value += "...";
                    else
                        Value += to_string(FramePos);
                    Value += '+';
                }
                Value.back() = ')';
            }
            Fill(Stream_Audio, 0, (Conformance_String + ConformanceError.Field).c_str(), Value);
        }
        Conformance_Total.clear();
    }
}
#endif

//***************************************************************************
// Elements - USAC - Config
//***************************************************************************

//---------------------------------------------------------------------------
void File_Usac::UsacConfig(size_t BitsNotIncluded)
{
    // Init
    C = usac_config();
    C.loudnessInfoSet_IsNotValid = false;
    #if MEDIAINFO_CONFORMANCE
        C.loudnessInfoSet_Present[0] = 0;
        C.loudnessInfoSet_Present[1] = 0;
    #endif

    Element_Begin1("UsacConfig");
    bool usacConfigExtensionPresent;
    Get_S1 (5, C.sampling_frequency_index,                      "usacSamplingFrequencyIndex"); Param_Info1C(C.sampling_frequency_index<Aac_sampling_frequency_Size_Usac && Aac_sampling_frequency[C.sampling_frequency_index], Aac_sampling_frequency[C.sampling_frequency_index]);
    if (C.sampling_frequency_index==Aac_sampling_frequency_Size_Usac)
    {
        int32u samplingFrequency;
        Get_S3 (24, samplingFrequency,                          "usacSamplingFrequency");
        C.sampling_frequency_index=Aac_AudioSpecificConfig_sampling_frequency_index(samplingFrequency);
        C.sampling_frequency=samplingFrequency;
    }
    else
    {
        C.sampling_frequency=Aac_sampling_frequency[C.sampling_frequency_index];
    }
    #if MEDIAINFO_CONFORMANCE
        if (!IsParsingRaw && Frequency_b && C.sampling_frequency && C.sampling_frequency != Frequency_b)
            Fill_Conformance("Crosscheck AudioSpecificConfig+UsacConfig samplingFrequency+usacSamplingFrequency", (to_string(Frequency_b) + " vs " + to_string(C.sampling_frequency) + " are not coherent").c_str());
    #endif
    Get_S1 (3, C.coreSbrFrameLengthIndex,                       "coreSbrFrameLengthIndex");
    Get_S1 (5, C.channelConfiguration,                          "channelConfiguration"); Param_Info1C(C.channelConfiguration, Aac_ChannelLayout_GetString(C.channelConfiguration));
    channelConfiguration=C.channelConfiguration;
    if (!C.channelConfiguration)
    {
        escapedValue(C.numOutChannels, 5, 8, 16,                "numOutChannels");
        for (int32u i=0; i<C.numOutChannels; i++)
        {
            Info_S1(5, bsOutChannelPos,                         "bsOutChannelPos"); Param_Info1(Aac_OutputChannelPosition_GetString(bsOutChannelPos));
        }
    }
    else if (C.channelConfiguration<Aac_Channels_Size_Usac)
        C.numOutChannels=Aac_Channels[C.channelConfiguration];
    else
        C.numOutChannels=(int32u)-1;
    UsacDecoderConfig();
    Get_SB (usacConfigExtensionPresent,                         "usacConfigExtensionPresent");
    if (usacConfigExtensionPresent)
        UsacConfigExtension();
    Element_End0();

    if (BitsNotIncluded!=(size_t)-1)
    {
        if (Data_BS_Remain()>BitsNotIncluded)
        {
            int8u LastByte=0xFF;
            auto BitsRemaining=Data_BS_Remain()-BitsNotIncluded;
            if (BitsRemaining<8)
                Peek_S1((int8u)BitsRemaining, LastByte);
            Skip_BS(BitsRemaining,                              LastByte?"Unknown":"Padding");
        }
        else if (Data_BS_Remain()<BitsNotIncluded)
            Trusted_IsNot("Too big");
    }

    if (Trusted_Get())
    {
        // Filling
        C.IsNotValid=false;
        if (!IsParsingRaw)
        {
            if (C.coreSbrFrameLengthIndex<coreSbrFrameLengthIndex_Mapping_Size)
                Fill(Stream_Audio, 0, Audio_SamplesPerFrame, coreSbrFrameLengthIndex_Mapping[C.coreSbrFrameLengthIndex].outputFrameLengthDivided256<<8, 10, true);
            Fill_DRC();
            Fill_Loudness();
            #if MEDIAINFO_CONFORMANCE
                Merge_Conformance(true);
            #endif
            Conf=C;
        }
    }
    else
    {
        #if MEDIAINFO_CONFORMANCE
            if (!IsParsingRaw)
            {
                Clear_Conformance();
                Fill_Conformance("UsacConfig Coherency", "Malformed bitstream");
                Merge_Conformance();
            }
        #endif

        C.IsNotValid=true;
        if (!IsParsingRaw)
            Conf.IsNotValid=true;
    }
    #if !MEDIAINFO_CONFORMANCE && MEDIAINFO_TRACE
        if (!Trace_Activated)
            Finish();
    #endif
}

//---------------------------------------------------------------------------
void File_Usac::Fill_DRC(const char* Prefix)
{
    if (!C.drcInstructionsUniDrc_Data.empty())
    {
        string FieldPrefix;
        if (Prefix)
        {
            FieldPrefix+=Prefix;
            FieldPrefix += ' ';
        }

        Fill(Stream_Audio, 0, (FieldPrefix+"DrcSets_Count").c_str(), C.drcInstructionsUniDrc_Data.size());
        Fill_SetOptions(Stream_Audio, 0, (FieldPrefix + "DrcSets_Count").c_str(), "N NI"); // Hidden in text output
        ZtringList Ids, Data;
        for (const auto& Item : C.drcInstructionsUniDrc_Data)
        {
            int8u drcSetId=Item.first.drcSetId;
            int8u downmixId=Item.first.downmixId;
            Ztring Id;
            if (drcSetId || downmixId)
                Id=Ztring::ToZtring(drcSetId)+=__T('-')+Ztring::ToZtring(downmixId);
            Ids.push_back(Id);
            Data.push_back(Ztring().From_UTF8(Item.second.drcSetEffectTotal));
        }
        Fill(Stream_Audio, 0, (FieldPrefix+"DrcSets_Effects").c_str(), Data, Ids);
    }
}

//---------------------------------------------------------------------------
void File_Usac::Fill_Loudness(const char* Prefix, bool NoConCh)
{
    string FieldPrefix;
    if (Prefix)
    {
        FieldPrefix += Prefix;
        FieldPrefix += ' ';
    }
    string FieldSuffix;

    bool DefaultIdPresent = false;
    for (int8u i = 0; i < 2; i++)
    {
        if (i)
            FieldSuffix = "_Album";
        if (!C.loudnessInfo_Data[i].empty())
        {
            Fill(Stream_Audio, 0, (FieldPrefix + "Loudness_Count" + FieldSuffix).c_str(), C.loudnessInfo_Data[i].size());
            Fill_SetOptions(Stream_Audio, 0, (FieldPrefix + "Loudness_Count" + FieldSuffix).c_str(), "N NI"); // Hidden in text output
        }
        vector<drc_id> Ids;
        ZtringList SamplePeakLevel;
        ZtringList TruePeakLevel;
        ZtringList Measurements[16];
        for (const auto& Item : C.loudnessInfo_Data[i])
        {
            Ids.push_back(Item.first);
            SamplePeakLevel.push_back(Item.second.SamplePeakLevel);
            TruePeakLevel.push_back(Item.second.TruePeakLevel);
            for (int8u j = 1; j < 16; j++)
                Measurements[j].push_back(Item.second.Measurements.Values[j]);
        }
        if (Ids.size() >= 1 && Ids.front().empty())
        {
            if (!i)
                DefaultIdPresent = true;
        }
        ZtringList Ids_Ztring;
        for (const auto& Id : Ids)
            Ids_Ztring.emplace_back(Ztring().From_UTF8(Id.to_string()));
        if (Ids_Ztring.empty())
            continue;
        Fill(Stream_Audio, 0, (FieldPrefix + "SamplePeakLevel" + FieldSuffix).c_str(), SamplePeakLevel, Ids_Ztring);
        Fill(Stream_Audio, 0, (FieldPrefix + "TruePeakLevel" + FieldSuffix).c_str(), TruePeakLevel, Ids_Ztring);
        for (int8u j = 0; j < 16; j++)
        {
            string Field;
            if (j && j <= LoudnessMeaning_Size)
                Field = LoudnessMeaning[j - 1];
            else
                Field = Ztring::ToZtring(j).To_UTF8();
            Fill(Stream_Audio, 0, (FieldPrefix + "Loudness_" + Field + FieldSuffix).c_str(), Measurements[j], Ids_Ztring);
        }
    }

    #if MEDIAINFO_CONFORMANCE
        if (NoConCh || C.IsNotValid)
            return;
        auto loudnessInfoSet_Present_Total=C.loudnessInfoSet_Present[0]+C.loudnessInfoSet_Present[1];
        constexpr14 auto CheckFlags = bitset8().set(xHEAAC).set(MpegH);
        if (!CheckIf(CheckFlags) || Format_Profile==AudioProfile_Unspecified)
        {
        }
        else if (!loudnessInfoSet_Present_Total)
        {
            if (!C.loudnessInfoSet_IsNotValid)
                Fill_Conformance("loudnessInfoSet Coherency", "Is missing", CheckFlags);
            Fill(Stream_Audio, 0, (FieldPrefix + "ConformanceCheck").c_str(), "Invalid: loudnessInfoSet is missing");
            Fill(Stream_Audio, 0, "ConformanceCheck/Short", "Invalid: loudnessInfoSet missing");
        }
        else if (C.loudnessInfo_Data[0].empty())
        {
            if (!C.loudnessInfoSet_IsNotValid)
                Fill_Conformance("loudnessInfoSet loudnessInfoCount", "Is 0", CheckFlags);
            Fill(Stream_Audio, 0, (FieldPrefix + "ConformanceCheck").c_str(), "Invalid: loudnessInfoSet is empty");
            Fill(Stream_Audio, 0, "ConformanceCheck/Short", "Invalid: loudnessInfoSet empty");
        }
        else if (!DefaultIdPresent)
        {
            if (!C.loudnessInfoSet_IsNotValid)
                Fill_Conformance("loudnessInfoSet Coherency", "Default loudnessInfo is missing", CheckFlags);
            Fill(Stream_Audio, 0, (FieldPrefix + "ConformanceCheck").c_str(), "Invalid: Default loudnessInfo is missing");
            Fill(Stream_Audio, 0, "ConformanceCheck/Short", "Invalid: Default loudnessInfo missing");
        }
        else if (C.loudnessInfo_Data[0].begin()->second.Measurements.Values[1].empty() && C.loudnessInfo_Data[0].begin()->second.Measurements.Values[2].empty())
        {
            if (!C.loudnessInfoSet_IsNotValid)
                Fill_Conformance("loudnessInfoSet Coherency", "None of program loudness or anchor loudness is present in default loudnessInfo", CheckFlags);
            Fill(Stream_Audio, 0, (FieldPrefix + "ConformanceCheck").c_str(), "Invalid: None of program loudness or anchor loudness is present in default loudnessInfo");
            Fill(Stream_Audio, 0, "ConformanceCheck/Short", "Invalid: Default loudnessInfo incomplete");
        }
        if (!Retrieve_Const(Stream_Audio, 0, "ConformanceCheck/Short").empty())
        {
            Fill_SetOptions(Stream_Audio, 0, "ConformanceCheck", "N NT"); // Hidden in text output (deprecated)
            Fill_SetOptions(Stream_Audio, 0, "ConformanceCheck/Short", "N NT"); // Hidden in text output
        }
    #endif
}

//---------------------------------------------------------------------------
void File_Usac::UsacDecoderConfig()
{
    Element_Begin1("UsacDecoderConfig");
    int32u numElements;
    escapedValue(numElements, 4, 8, 16,                         "numElements minus 1");

    for (int32u elemIdx=0; elemIdx<=numElements; elemIdx++)
    {
        Element_Begin1("usacElement");
        int8u usacElementType;
        Get_S1(2, usacElementType,                              "usacElementType"); Element_Info1(usacElementType_IdNames[usacElementType]);
        C.usacElements.push_back({usacElementType, 0});
        switch (usacElementType)
        {
            case ID_USAC_SCE                                    : UsacSingleChannelElementConfig(); break;
            case ID_USAC_CPE                                    : UsacChannelPairElementConfig(); break;
            case ID_USAC_LFE                                    : UsacLfeElementConfig(); break;
            case ID_USAC_EXT                                    : UsacExtElementConfig(); break;
        }
        Element_End0();
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::UsacSingleChannelElementConfig()
{
    Element_Begin1("UsacSingleChannelElementConfig");

    UsacCoreConfig();
    if (C.coreSbrFrameLengthIndex>=coreSbrFrameLengthIndex_Mapping_Size || coreSbrFrameLengthIndex_Mapping[C.coreSbrFrameLengthIndex].sbrRatioIndex)
        SbrConfig();

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::UsacChannelPairElementConfig()
{
    Element_Begin1("UsacChannelPairElementConfig");

    UsacCoreConfig();
    if (C.coreSbrFrameLengthIndex>=coreSbrFrameLengthIndex_Mapping_Size || coreSbrFrameLengthIndex_Mapping[C.coreSbrFrameLengthIndex].sbrRatioIndex)
    {
        int8u stereoConfiglindex;
        SbrConfig();
        Get_S1(2, stereoConfiglindex,                           "stereoConfiglindex");
        if (stereoConfiglindex)
            Mps212Config(stereoConfiglindex);
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::UsacLfeElementConfig()
{
    // Nothing here
}

//---------------------------------------------------------------------------
void File_Usac::UsacExtElementConfig()
{
    Element_Begin1("UsacExtElementConfig");

    int32u usacExtElementType, usacExtElementConfigLength;
    bool usacExtElementDefaultLengthPresent, usacExtElementPayloadFrag;
    escapedValue(usacExtElementType, 4, 8, 16,                  "usacExtElementType"); Element_Level--; Element_Info1C(usacExtElementType<ID_EXT_ELE_Max, usacExtElementType_IdNames[usacExtElementType]); Element_Level++;
    C.usacElements.back().usacElementType+=usacExtElementType<<2;
    escapedValue(usacExtElementConfigLength, 4, 8, 16,          "usacExtElementConfigLength");
    Get_SB (usacExtElementDefaultLengthPresent,                 "usacExtElementDefaultLengthPresent");
    if (usacExtElementDefaultLengthPresent)
    {
        int32u usacExtElementDefaultLength;
        escapedValue(usacExtElementDefaultLength, 8, 16, 0,     "usacExtElementDefaultLength");
        C.usacElements.back().usacExtElementDefaultLength=usacExtElementDefaultLength+1;
    }
    Get_SB (usacExtElementPayloadFrag,                          "usacExtElementPayloadFlag");
    C.usacElements.back().usacExtElementPayloadFrag=usacExtElementPayloadFrag;

    if (usacExtElementConfigLength)
    {
        usacExtElementConfigLength*=8;
        if (usacExtElementConfigLength>Data_BS_Remain())
        {
            Trusted_IsNot("Too big");
            Element_End0();
            return;
        }
        auto B=BS_Bookmark(usacExtElementConfigLength);
        switch (usacExtElementType)
        {
            case ID_EXT_ELE_FILL                                :
            case ID_EXT_ELE_AUDIOPREROLL                        : break;
            case ID_EXT_ELE_UNI_DRC                             : uniDrcConfig(); break;
            default:
                Skip_BS(usacExtElementConfigLength,             "Unknown");
        }
        BS_Bookmark(B, (usacExtElementType<ID_EXT_ELE_Max?string(usacExtElementType_Names[usacExtElementType]):("usacExtElementType"+to_string(usacExtElementType)))+"Config Coherency");
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::UsacCoreConfig()
{
    Element_Begin1("UsacCoreConfig");

    Skip_SB(                                                    "tw_mdct");
    Skip_SB(                                                    "noiseFilling");

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::SbrConfig()
{
    Element_Begin1("SbrConfig");

    Get_SB (C.harmonicSBR,                                      "harmonicSBR");
    Skip_SB(                                                    "bs_interTes");
    Skip_SB(                                                    "bs_pvc");
    SbrDlftHeader();

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::SbrDlftHeader()
{
    Element_Begin1("SbrDlftHeader");

    bool dflt_header_extra1, dflt_header_extra2;
    Skip_S1(4,                                                  "dflt_start_freq");
    Skip_S1(4,                                                  "dflt_stop_freq");
    Get_SB (   dflt_header_extra1,                              "dflt_header_extra1");
    Get_SB (   dflt_header_extra2,                              "dflt_header_extra2");
    if (dflt_header_extra1)
    {
        Skip_S1(2,                                              "dflt_freq_scale");
        Skip_SB(                                                "dflt_alter_scale");
        Skip_S1(2,                                              "dflt_noise_bands");
    }
    if (dflt_header_extra2)
    {
        Skip_S1(2,                                              "dflt_limiter_bands");
        Skip_S1(2,                                              "dflt_limiter_gains");
        Skip_SB(                                                "dflt_interpol_freq");
        Skip_SB(                                                "dflt_smoothing_mode");
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::Mps212Config(int8u StereoConfigindex)
{
    Element_Begin1("Mps212Config");

    int8u bsTempShapeConfig;
    bool bsOttBandsPhasePresent;
    Skip_S1(3,                                                  "bsFreqRes");
    Skip_S1(3,                                                  "bsFixedGainDMX");
    Get_S1 (2, bsTempShapeConfig,                               "bsTempShapeConfig");
    Skip_S1(2,                                                  "bsDecorrConfig");
    Skip_SB(                                                    "bsHighRatelMode");
    Skip_SB(                                                    "bsPhaseCoding");
    Get_SB (   bsOttBandsPhasePresent,                          "bsOttBandsPhasePresent");
    if (bsOttBandsPhasePresent)
    {
        Skip_S1(5,                                              "bsOttBandsPhase");
    }
    if (StereoConfigindex>1)
    {
        Skip_S1(5,                                              "bsResidualBands");
        Skip_SB(                                                "bSPseudor");
    }
    if (bsTempShapeConfig==2)
    {
        Skip_SB(                                                "bSEnvOuantMode");
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::uniDrcConfig()
{
    C.downmixInstructions_Data.clear();
    C.drcInstructionsUniDrc_Data.clear();
    C.loudnessInfo_Data[0].clear();
    C.loudnessInfo_Data[1].clear();

    Element_Begin1("uniDrcConfig");

    int8u downmixInstructionsCount, drcCoefficientsBasicCount, drcInstructionsBasicCount, drcCoefficientsUniDrcCount, drcInstructionsUniDrcCount;
    TEST_SB_SKIP(                                               "sampleRatePresent");
        int32u bsSampleRate;
        Get_S3 (18, bsSampleRate ,                              "bsSampleRate"); bsSampleRate+=1000; Param_Info2(bsSampleRate, " Hz");
    TEST_SB_END();
    Get_S1 (7, downmixInstructionsCount,                        "downmixInstructionsCount");
    TESTELSE_SB_SKIP(                                           "drcDescriptionBasicPresent");
        Get_S1 (3, drcCoefficientsBasicCount,                   "drcCoefficientsBasicCount");
        Get_S1 (4, drcInstructionsBasicCount,                   "drcInstructionsBasicCount");
    TESTELSE_SB_ELSE(                                           "drcDescriptionBasicPresent");
        drcCoefficientsBasicCount=0;
        drcInstructionsBasicCount=0;
    TESTELSE_SB_END();
    Get_S1 (3, drcCoefficientsUniDrcCount,                      "drcCoefficientsUniDrcCount");
    Get_S1 (6, drcInstructionsUniDrcCount,                      "drcInstructionsUniDrcCount");
    channelLayout();
    for (int8u i=0; i<downmixInstructionsCount; i++)
        downmixInstructions();
    for (int8u i=0; i<drcCoefficientsBasicCount; i++)
        drcCoefficientsBasic();
    for (int8u i=0; i<drcInstructionsBasicCount; i++)
        drcInstructionsBasic();
    for (int8u i=0; i<drcCoefficientsUniDrcCount; i++)
        drcCoefficientsUniDrc();
    for (int8u i=0; i<drcInstructionsUniDrcCount; i++)
        drcInstructionsUniDrc();
    bool uniDrcConfigExtPresent;
    Get_SB (   uniDrcConfigExtPresent,                          "uniDrcConfigExtPresent");
    if (uniDrcConfigExtPresent)
        uniDrcConfigExtension();

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::uniDrcConfigExtension()
{
    for (;;)
    {
        Element_Begin1("uniDrcConfigExtension");
        int32u bitSize;
        int8u uniDrcConfigExtType, extSizeBits;
        Get_S1 (4, uniDrcConfigExtType,                         "uniDrcConfigExtType"); Element_Info1C(uniDrcConfigExtType<UNIDRCCONFEXT_Max, uniDrcConfigExtType_IdNames[uniDrcConfigExtType]);
        if (!uniDrcConfigExtType) // UNIDRCCONFEXT_TERM
        {
            Element_End0();
            break;
        }
        Get_S1 (4, extSizeBits,                                 "bitSizeLen");
        extSizeBits+=4;
        Get_S4 (extSizeBits, bitSize,                           "bitSize");
        bitSize++;
        if (bitSize>Data_BS_Remain())
        {
            Trusted_IsNot("Too big");
            Element_End0();
            break;
        }

        auto B=BS_Bookmark(bitSize);
        switch (uniDrcConfigExtType)
        {
            /*
            case UNIDRCCONFEXT_PARAM_DRC :
                {
                drcCoefficientsParametricDrc();
                int8u parametricDrcInstructionsCount;
                Get_S1 (4, parametricDrcInstructionsCount,      "parametricDrcInstructionsCount");
                for (int8u i=0; i<parametricDrcInstructionsCount; i++)
                    parametricDrcInstructions();
                Skip_BS(Data_BS_Remain(),                       "(Not implemented)");
                }
                break;
            */
            case UNIDRCCONFEXT_V1 :
                {
                TEST_SB_SKIP(                                   "downmixInstructionsV1Present");
                    int8u downmixInstructionsV1Count;
                    Get_S1 (7, downmixInstructionsV1Count,      "downmixInstructionsV1Count");
                    for (int8u i=0; i<downmixInstructionsV1Count; i++)
                        downmixInstructions(true);
                TEST_SB_END();
                TEST_SB_SKIP(                                   "drcCoeffsAndInstructionsUniDrcV1Present");
                    int8u drcCoefficientsUniDrcV1Count;
                    Get_S1 (3, drcCoefficientsUniDrcV1Count,    "drcCoefficientsUniDrcV1Count");
                    for (int8u i=0; i<drcCoefficientsUniDrcV1Count; i++)
                        drcCoefficientsUniDrc(true);
                    int8u drcInstructionsUniDrcV1Count;
                    Get_S1 (6, drcInstructionsUniDrcV1Count,    "drcInstructionsUniDrcV1Count");
                    for (int8u i=0; i<drcInstructionsUniDrcV1Count; i++)
                        drcInstructionsUniDrc(true);
                TEST_SB_END();
                bool MustSkip=false;
                TEST_SB_SKIP(                                   "loudEqInstructionsPresent");
                    int8u loudEqInstructionsCount;
                    Get_S1 (4, loudEqInstructionsCount,         "loudEqInstructionsCount");
                    for (int8u i=0; i<loudEqInstructionsCount; i++)
                        MustSkip=true; // Not yet implemented
                    //    loudEqInstructions();
                TEST_SB_END();
                if (!MustSkip)
                TEST_SB_SKIP(                                   "eqPresent");
                    // Not yet implemented
                    //int8u eqInstructionsCount;
                    //eqCoefficients();
                    //Get_S1 (4, eqInstructionsCount,             "eqInstructionsCount");
                    //for (int8u i=0; i<eqInstructionsCount; i++)
                    //    eqInstructions();
                TEST_SB_END();
                if (Data_BS_Remain()>B.BitsNotIncluded)
                    Skip_BS(Data_BS_Remain()-B.BitsNotIncluded, "(Not implemented)");
                }
                break;
            default:
                Skip_BS(bitSize,                                "Unknown");
        }
        BS_Bookmark(B, (uniDrcConfigExtType<UNIDRCCONFEXT_Max?string(uniDrcConfigExtType_ConfNames[uniDrcConfigExtType]):("uniDrcConfigExtType"+to_string(uniDrcConfigExtType)))+" Coherency");
        Element_End0();
    }
}

//---------------------------------------------------------------------------
void File_Usac::downmixInstructions(bool V1)
{
    Element_Begin1("downmixInstructionsV1");

    bool layoutSignalingPresent;
    int8u downmixId, targetChannelCount;
    Get_S1 (7, downmixId,                                       "downmixId");
    Get_S1 (7, targetChannelCount,                              "targetChannelCount");
    Skip_S1(8,                                                  "targetLayout");
    Get_SB (   layoutSignalingPresent,                          "layoutSignalingPresent");
    if (layoutSignalingPresent)
    {
        if (V1)
            Skip_S1(4,                                          "bsDownmixOffset");
        for (int8u i=0; i<targetChannelCount; i++)
            for (int8u j=0; j<C.baseChannelCount; j++)
                Skip_S1(V1?5:4,                                 V1?"bsDownmixCoefficientV1":"bsDownmixCoefficient");
    }
    C.downmixInstructions_Data[downmixId].targetChannelCount=targetChannelCount;
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::drcCoefficientsBasic()
{
    Element_Begin1("drcCoefficientsBasic");

    Skip_S1(4,                                                  "drcLocation");
    Skip_S1(7,                                                  "drcCharacteristic");

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::drcCoefficientsUniDrc(bool V1)
{
    Element_Begin1(V1?"drcCoefficientsUniDrcV1":"drcCoefficientsUniDrc");

    bool drcFrameSizePresent;
    Skip_S1(4,                                                  "drcLocation");
    Get_SB (   drcFrameSizePresent,                             "drcFrameSizePresent");
    if (drcFrameSizePresent)
        Skip_S2(15,                                             "bsDrcFrameSize");
    if (V1)
    {
        bool drcCharacteristicLeftPresent;
        Get_SB (   drcCharacteristicLeftPresent,                "drcCharacteristicLeftPresent");
        if (drcCharacteristicLeftPresent)
        {
            int8u characteristicLeftCount;
            Get_S1 (4, characteristicLeftCount,                 "characteristicLeftCount");
            for (int8u k=0; k<characteristicLeftCount; k++)
            {
                bool characteristicFormat;
                Get_SB (   characteristicFormat,                "characteristicFormat");
                if (!characteristicFormat)
                {
                    Skip_S1(6,                                  "bsGainLeft");
                    Skip_S1(4,                                  "bsIoRatioLeft");
                    Skip_S1(4,                                  "bsExpLeft");
                    Skip_SB(                                    "flipSignLeft");
                }
                else
                {
                    int8u bsCharNodeCount;
                    Get_S1 (2, bsCharNodeCount,                 "bsCharNodeCount");
                    for (int8u n=0; n<=bsCharNodeCount; n++)
                    {
                        Skip_S1(5,                              "bsNodeLevelDelta");
                        Skip_S1(8,                              "bsNodeGain");
                    }
                }
            }
        }
        bool drcCharacteristicRightPresent;
        Get_SB (   drcCharacteristicRightPresent,               "drcCharacteristicRightPresent");
        if (drcCharacteristicLeftPresent)
        {
            int8u characteristicRightCount;
            Get_S1 (4, characteristicRightCount,                "characteristicRightCount");
            for (int8u k=0; k<characteristicRightCount; k++)
            {
                bool characteristicFormat;
                Get_SB (   characteristicFormat,                "characteristicFormat");
                if (!characteristicFormat)
                {
                    Skip_S1(6,                                  "bsGainLeft");
                    Skip_S1(4,                                  "bsIoRatioLeft");
                    Skip_S1(4,                                  "bsExpLeft");
                    Skip_SB(                                    "flipSignLeft");
                }
                else
                {
                    int8u bsCharNodeCount;
                    Get_S1 (2, bsCharNodeCount,                 "bsCharNodeCount");
                    for (int8u n=0; n<=bsCharNodeCount; n++)
                    {
                        Skip_S1(5,                              "bsNodeLevelDelta");
                        Skip_S1(8,                              "bsNodeGain");
                    }
                }
            }
        }
        bool shapeFiltersPresent;
        Get_SB (   shapeFiltersPresent,                         "shapeFiltersPresent");
        if (shapeFiltersPresent)
        {
            int8u shapeFilterCount;
            Get_S1 (4, shapeFilterCount,                        "shapeFilterCount");
            for (int8u k=0; k<shapeFilterCount; k++)
            {
                TEST_SB_SKIP(                                   "lfCutFilterPresent");
                    Skip_S1(3,                                  "lfCornerFreqIndex");
                    Skip_S1(2,                                  "lfFilterStrengthIndex");
                TEST_SB_END();
                TEST_SB_SKIP(                                   "lfBoostFilterPresent");
                    Skip_S1(3,                                  "lfCornerFreqIndex");
                    Skip_S1(2,                                  "lfFilterStrengthIndex");
                TEST_SB_END();
                TEST_SB_SKIP(                                   "hfCutFilterPresent");
                    Skip_S1(3,                                  "lfCornerFreqIndex");
                    Skip_S1(2,                                  "lfFilterStrengthIndex");
                TEST_SB_END();
                TEST_SB_SKIP(                                   "hfBoostFilterPresent");
                    Skip_S1(3,                                  "lfCornerFreqIndex");
                    Skip_S1(2,                                  "lfFilterStrengthIndex");
                TEST_SB_END();
            }
        }
        Skip_S1(6,                                              "gainSequenceCount");
    }
    int8u gainSetCount;
    Get_S1 (6, gainSetCount,                                    "gainSetCount");
    C.gainSets.clear();
    for (int8u i=0; i<gainSetCount; i++)
    {
        Element_Begin1("gainSet");
        gain_set gainSet;
        int8u gainCodingProfile;
        Get_S1 (2, gainCodingProfile,                           "gainCodingProfile");
        Skip_SB(                                                "gainInterpolationType");
        Skip_SB(                                                "fullFrame");
        Skip_SB(                                                "timeAlignment");
        TEST_SB_SKIP(                                           "timeDeltaMinPresent");
            Skip_S2(11,                                         "bsTimeDeltaMin");
        TEST_SB_END();
        if (gainCodingProfile != 3)
        {
            bool drcBandType;
            Get_S1 (4, gainSet.bandCount,                       "bandCount");
            if (gainSet.bandCount>1)
                Get_SB (drcBandType,                            "drcBandType");
            for (int8u i=0; i<gainSet.bandCount; i++)
            {
                Element_Begin1("bandCount");
                if (V1)
                {
                    TEST_SB_SKIP(                               "indexPresent");
                        Skip_S1(6,                              "bsIndex");
                    TEST_SB_END();
                }
                if (!V1)
                {
                    Skip_S1(7,                                  "drcCharacteristic");
                }
                else //V1
                {
                    TEST_SB_SKIP(                               "drcCharacteristicPresent");
                        bool drcCharacteristicFormatIsCICP;
                        Get_SB (drcCharacteristicFormatIsCICP,  "drcCharacteristicFormatIsCICP");
                        if (drcCharacteristicFormatIsCICP)
                        {
                            Skip_S1(7,                          "drcCharacteristic");
                        }
                        else
                        {
                            Skip_S1(4,                          "drcCharacteristicLeftIndex");
                            Skip_S1(4,                          "drcCharacteristicRightIndex");
                        }
                    TEST_SB_END();
                }
                Element_End0();
            }
            for (int8u i=1; i <gainSet.bandCount; i++)
            {
                if (drcBandType)
                    Skip_S1( 4,                                 "crossoverFreqIndex");
                else
                    Skip_S2(10,                                 "startSubBandIndex");
            }
        }
        else
            gainSet.bandCount=1;
        C.gainSets.push_back(gainSet);
        Element_End0();
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::drcInstructionsBasic()
{
    Element_Begin1("drcInstructionsBasic");

    int16u drcSetEffect;
    Skip_S1(6,                                                  "drcSetId");
    Skip_S1(4,                                                  "drcLocation");
    Skip_S1(7,                                                  "downmixId");
    TEST_SB_SKIP(                                               "additionalDownmixIdPresent");
        int8u additionalDownmixIdCount;
        Get_S1 (3, additionalDownmixIdCount,                    "additionalDownmixIdCount");
        for (int8u i=1; i <additionalDownmixIdCount; i++)
            Skip_S1(7,                                          "additionalDownmixId");
    TEST_SB_END();
    Get_S2 (16, drcSetEffect,                                   "drcSetEffect");
    if ((drcSetEffect & (3<<10)) == 0)
    {
        TEST_SB_SKIP(                                           "limiterPeakTargetPresent");
            Skip_S1(8,                                          "bsLimiterPeakTarget");
        TEST_SB_END();
    }
    TEST_SB_SKIP(                                               "drcSetTargetLoudnessPresent");
        Skip_S1(6,                                              "bsDrcSetTargetLoudnessValueUpper");
        TEST_SB_SKIP(                                           "drcSetTargetLoudnessValueLowerPresent");
            Skip_S1(6,                                          "bsDrcSetTargetLoudnessValueLower");
        TEST_SB_END();
    TEST_SB_END();

    Element_End0();
}

//---------------------------------------------------------------------------
bool File_Usac::drcInstructionsUniDrc(bool V1, bool NoV0)
{
    Element_Begin1(V1?"drcInstructionsUniDrcV1":"drcInstructionsUniDrc");

    int8u channelCount=C.baseChannelCount;
    vector<int8s> gainSetIndex;
    int16u drcSetEffect;
    int8u drcSetId, downmixId;
    bool downmixIdPresent;
    Get_S1 (6, drcSetId,                                        "drcSetId");
    if (V1)
        Skip_S1(4,                                              "drcSetComplexityLevel");
    Skip_S1(4,                                                  "drcLocation");
    if (V1)
        Get_SB (downmixIdPresent,                               "downmixIdPresent");
    else
        downmixIdPresent=true;
    if (downmixIdPresent)
    {
        bool drcApplyToDownmix;
        Get_S1(7, downmixId,                                    "downmixId");
        if (V1)
            Get_SB (   drcApplyToDownmix,                       "drcApplyToDownmix");
        else
            drcApplyToDownmix=downmixId?true:false;
        int8u additionalDownmixIdCount;
        TESTELSE_SB_SKIP(                                       "additionalDownmixIdPresent");
            Get_S1 (3, additionalDownmixIdCount,                "additionalDownmixIdCount");
            for (int8u i=0; i<additionalDownmixIdCount; i++)
                Skip_S1(7,                                      "additionalDownmixId");
        TESTELSE_SB_ELSE(                                       "additionalDownmixIdPresent");
            additionalDownmixIdCount=0;
        TESTELSE_SB_END();
        if ((!V1 || drcApplyToDownmix) && downmixId && downmixId!=0x7F && !additionalDownmixIdCount)
        {
            std::map<int8u, downmix_instruction>::iterator downmixInstruction_Data=C.downmixInstructions_Data.find(downmixId);
            if (downmixInstruction_Data!=C.downmixInstructions_Data.end())
                channelCount=downmixInstruction_Data->second.targetChannelCount;
            else
                channelCount=1;
        }
        else if ((!V1 || drcApplyToDownmix) && (downmixId==0x7F || additionalDownmixIdCount))
            channelCount=1;
    }
    else
        downmixId=0; // 0 is default
    Get_S2 (16, drcSetEffect,                                   "drcSetEffect");
    bool IsNOK=false;
    if ((drcSetEffect & (3<<10)) == 0)
    {
        TESTELSE_SB_SKIP(                                       "limiterPeakTargetPresent");
            Skip_S1(8,                                          "bsLimiterPeakTarget");
        TESTELSE_SB_ELSE(                                       "limiterPeakTargetPresent");
        TESTELSE_SB_END();
    }
    else
        channelCount=C.baseChannelCount; // TEMP
    TEST_SB_SKIP(                                               "drcSetTargetLoudnessPresent");
        Skip_S1(6,                                              "bsDrcSetTargetLoudnessValueUpper");
        TEST_SB_SKIP(                                           "drcSetTargetLoudnessValueLowerPresent");
            Skip_S1(6,                                          "bsDrcSetTargetLoudnessValueLower");
        TEST_SB_END();
    TEST_SB_END();
    TESTELSE_SB_SKIP(                                           "dependsOnDrcSetPresent");
        Skip_S1(6,                                              "dependsOnDrcSet");
    TESTELSE_SB_ELSE(                                           "dependsOnDrcSetPresent");
        Skip_SB(                                                "noIndependentUse");
    TESTELSE_SB_END();
    if (V1)
        Skip_SB(                                                "requiresEq");
    for (int8u c=0; c<channelCount; c++)
    {
        Element_Begin1("channel");
        int8u bsGainSetIndex;
        Get_S1 (6, bsGainSetIndex,                              "bsGainSetIndex");
        gainSetIndex.push_back(bsGainSetIndex);
        if ((drcSetEffect & (3<<10)) != 0)
        {
            TEST_SB_SKIP(                                       "duckingScalingPresent");
                Skip_S1(4,                                      "bsDuckingScaling");
            TEST_SB_END();
        }
        TEST_SB_SKIP(                                           "repeatGainSetIndex");
            int8u bsRepeatGainSetIndexCount;
            Get_S1 (5, bsRepeatGainSetIndexCount,               "bsRepeatGainSetIndexCount");
            bsRepeatGainSetIndexCount++;
            gainSetIndex.resize(gainSetIndex.size()+bsRepeatGainSetIndexCount, bsGainSetIndex);
            c+=bsRepeatGainSetIndexCount;
        TEST_SB_END();
        Element_End0();
    }

    set<int8s> DrcChannelGroups=set<int8s>(gainSetIndex.begin(), gainSetIndex.end());

    for (set<int8s>::iterator DrcChannelGroup=DrcChannelGroups.begin(); DrcChannelGroup!=DrcChannelGroups.end(); ++DrcChannelGroup)
    {
        if (!*DrcChannelGroup || (drcSetEffect & (3<<10)))
            continue; // 0 means not present
        Element_Begin1("DrcChannel");
        int8s gainSetIndex=*DrcChannelGroup-1;
        int8u bandCount=V1?(gainSetIndex<C.gainSets.size()?C.gainSets[gainSetIndex].bandCount:0):1;
        for (int8u k=0; k<bandCount; k++)
        {
            Element_Begin1("band");
            if (V1)
            {
            TEST_SB_SKIP(                                       "targetCharacteristicLeftPresent");
                Skip_S1(4,                                      "targetCharacteristicLeftIndex");
            TEST_SB_END();
            TEST_SB_SKIP(                                       "targetCharacteristicRightPresent");
                Skip_S1(4,                                      "targetCharacteristicRightIndex");
            TEST_SB_END();
            }
            TEST_SB_SKIP(                                       "gainScalingPresent");
                Skip_S1(4,                                      "bsAttenuationScaling");
                Skip_S1(4,                                      "bsAmplificationScaling");
            TEST_SB_END();
            TEST_SB_SKIP(                                       "gainOffsetPresent");
                Skip_SB(                                        "bsGainSign");
                Skip_S1(5,                                      "bsGainOffset");
            TEST_SB_END();
            Element_End0();
        }
        if (V1 && bandCount==1)
        {
            TEST_SB_SKIP(                                       "shapeFilterPresent");
                Skip_S1(4,                                      "shapeFilterIndex");
            TEST_SB_END();
        }
        Element_End0();
    }

    Element_End0();

    if (V1 || NoV0) //We want to display only V1 information
    {
        string Value;
        for (int8u i=0; i<16; i++)
            if (drcSetEffect&(1<<i))
            {
                if (!Value.empty())
                    Value+=" & ";
                if (i<drcSetEffect_List_Size)
                    Value+=drcSetEffect_List[i];
                else
                {
                    if (i>=10)
                        Value+='1';
                    Value+='0'+(i%10);
                }
            }
        drc_id Id={drcSetId, downmixId, 0};
        C.drcInstructionsUniDrc_Data[Id].drcSetEffectTotal=Value;
    }

    return false;
}

//---------------------------------------------------------------------------
void File_Usac::channelLayout()
{
    Element_Begin1("channelLayout");

    bool layoutSignalingPresent;
    Get_S1 (7, C.baseChannelCount,                              "C.baseChannelCount");
    Get_SB (   layoutSignalingPresent,                          "layoutSignalingPresent");
    if (layoutSignalingPresent)
    {
        int8u definedLayout;
        Get_S1 (8, definedLayout,                               "definedLayout");
        if (!definedLayout)
        {
            for (int8u i=0; i<C.baseChannelCount; i++)
            {
                Info_S1(7, speakerPosition,                     "speakerPosition"); Param_Info1(Aac_OutputChannelPosition_GetString(speakerPosition));
            }
        }
    }

    Element_End0();
}

//---------------------------------------------------------------------------
static const size_t UsacConfigExtension_usacConfigExtType_Size=8;
static const char* UsacConfigExtension_usacConfigExtType[UsacConfigExtension_usacConfigExtType_Size]=
{
    "FILL",
    NULL,
    "LOUDNESS_INFO",
    NULL,
    NULL,
    NULL,
    NULL,
    "STREAM_ID",
};
void File_Usac::UsacConfigExtension()
{
    Element_Begin1("UsacConfigExtension");

    int32u numConfigExtensions;
    escapedValue(numConfigExtensions, 2, 4, 8,                  "numConfigExtensions minus 1");

    for (int32u confExtIdx=0; confExtIdx<=numConfigExtensions; confExtIdx++) 
    {
        Element_Begin1("usacConfigExtension");
        int32u usacConfigExtType, usacConfigExtLength;
        escapedValue(usacConfigExtType, 4, 8, 16,               "usacConfigExtType"); Param_Info1C(usacConfigExtType<UsacConfigExtension_usacConfigExtType_Size && UsacConfigExtension_usacConfigExtType[usacConfigExtType], UsacConfigExtension_usacConfigExtType[usacConfigExtType]);
        escapedValue(usacConfigExtLength, 4, 8, 16,             "usacExtElementConfigLength");

        if (usacConfigExtLength)
        {
            usacConfigExtLength*=8;
            if (usacConfigExtLength>Data_BS_Remain())
            {
                Trusted_IsNot("Too big");
                Element_End0();
                break;
            }
            auto B=BS_Bookmark(usacConfigExtLength);
            switch (usacConfigExtType)
            {
                case ID_CONFIG_EXT_FILL                         :
                    if (usacConfigExtLength)
                        Skip_BS(usacConfigExtLength,            "10100101"); //TODO: check if value is the right one
                    break;
                case ID_CONFIG_EXT_LOUDNESS_INFO                : loudnessInfoSet(); break;
                case ID_CONFIG_EXT_STREAM_ID                    : streamId(); break;
                default:
                    Skip_BS(usacConfigExtLength,                "Unknown");
            }
            if (BS_Bookmark(B, (usacConfigExtType<ID_CONFIG_EXT_Max?string(usacConfigExtType_ConfNames[usacConfigExtType]):("usacConfigExtType"+to_string(usacConfigExtType)))+" Coherency"))
            {
                switch (usacConfigExtType)
                {
                    case ID_CONFIG_EXT_LOUDNESS_INFO            : C.loudnessInfoSet_IsNotValid=true; break;
                }
            }
        }
        Element_End0();
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::loudnessInfoSet(bool V1)
{
    Element_Begin1(V1?"loudnessInfoSetV1":"loudnessInfoSet");

    #if MEDIAINFO_CONFORMANCE
        if (V1)
            C.loudnessInfoSet_Present[V1]++;
    #endif

    int8u loudnessInfoAlbumCount, loudnessInfoCount;
    bool loudnessInfoSetExtPresent;
    Get_S1 (6, loudnessInfoAlbumCount,                          "loudnessInfoAlbumCount");
    Get_S1 (6, loudnessInfoCount,                               "loudnessInfoCount");
    #if MEDIAINFO_CONFORMANCE
        if (!V1 && (loudnessInfoAlbumCount || loudnessInfoCount))
            C.loudnessInfoSet_Present[V1]++;
    #endif
    for (int8u i=0; i<loudnessInfoAlbumCount; i++)
        loudnessInfo(true, V1);
    for (int8u i=0; i<loudnessInfoCount; i++)
        loudnessInfo(false, V1);
    if (!V1)
    {
        Get_SB (loudnessInfoSetExtPresent,                      "loudnessInfoSetExtPresent");
        if (loudnessInfoSetExtPresent)
            loudnessInfoSetExtension();
    }

    if (!Trusted_Get())
        C.loudnessInfoSet_IsNotValid=true;
    Element_End0();
}

//---------------------------------------------------------------------------
static const size_t methodDefinition_Format_Size=10;
static const int8u methodDefinition_Format[methodDefinition_Format_Size]=
{
    8, 8, 8, 8, 8, 8, 8, 5, 2, 8,
};
bool File_Usac::loudnessInfo(bool FromAlbum, bool V1)
{
    Element_Begin1(V1?"loudnessInfoV1":"loudnessInfo");

    loudness_info::measurements Measurements;
    int16u bsSamplePeakLevel, bsTruePeakLevel;
    int8u measurementCount;
    bool samplePeakLevelPresent, truePeakLevelPresent;
    int8u drcSetId, eqSetId, downmixId;
    Get_S1 (6, drcSetId,                                        "drcSetId");
    if (V1)
        Get_S1 (6, eqSetId,                                     "eqSetId");
    else
        eqSetId=0;
    Get_S1 (7, downmixId,                                       "downmixId");
    Get_SB (samplePeakLevelPresent,                             "samplePeakLevelPresent");
    if (samplePeakLevelPresent)
    {
        Get_S2(12, bsSamplePeakLevel,                           "bsSamplePeakLevel"); Param_Info1C(bsSamplePeakLevel, Ztring::ToZtring(20-((double)bsSamplePeakLevel)/32)+__T(" dBTP"));
    }
    Get_SB (truePeakLevelPresent,                               "truePeakLevelPresent");
    if (truePeakLevelPresent)
    {
        int8u measurementSystem, reliability;
        Get_S2 (12, bsTruePeakLevel,                            "bsTruePeakLevel"); Param_Info1C(bsTruePeakLevel, Ztring::ToZtring(20-((double)bsTruePeakLevel)/32)+__T(" dBTP"));
        Get_S1 ( 4, measurementSystem,                          "measurementSystem"); Param_Info1C(measurementSystem<measurementSystems_Size, measurementSystems[measurementSystem]);
        Get_S1 ( 2, reliability,                                "reliability"); Param_Info1(reliabilities[reliability]);
    }
    Get_S1 (4, measurementCount,                                "measurementCount");
    bool IsNOK=false;
    for (int8u i=0; i<measurementCount; i++)
    {
        Element_Begin1("measurement");
        int8u methodDefinition, methodValue, measurementSystem, reliability;
        Get_S1 (4, methodDefinition,                            "methodDefinition"); Param_Info1C(methodDefinition && methodDefinition<=LoudnessMeaning_Size, LoudnessMeaning[methodDefinition-1]);
        int8u Size;
        if (methodDefinition>=methodDefinition_Format_Size)
        {
            Param_Info1("(Unsupported)");
            Measurements.Values[methodDefinition].From_UTF8("(Unsupported)");
            IsNOK=true;
            Element_End0();
            break;
        }
        Get_S1 (methodDefinition_Format[methodDefinition], methodValue, "methodValue");
        Ztring measurement=Loudness_Value(methodDefinition, methodValue);
        Param_Info1(measurement);
        Get_S1 (4, measurementSystem,                           "measurementSystem"); Param_Info1C(measurementSystem<measurementSystems_Size, measurementSystems[measurementSystem]);
        Get_S1 ( 2, reliability,                                "reliability"); Param_Info1(reliabilities[reliability]);
        if (methodDefinition)
        {
            Ztring& Content=Measurements.Values[methodDefinition];
            if (!Content.empty())
                Content+=__T(" & ");
            Content+=measurement;
        }
        Element_End0();
    }

    drc_id Id={drcSetId, downmixId, eqSetId};
    C.loudnessInfo_Data[FromAlbum][Id].SamplePeakLevel=((samplePeakLevelPresent && bsSamplePeakLevel)?(Ztring::ToZtring(20-((double)bsSamplePeakLevel)/32)+__T(" dBFS")):Ztring());
    C.loudnessInfo_Data[FromAlbum][Id].TruePeakLevel=((truePeakLevelPresent && bsTruePeakLevel)?(Ztring::ToZtring(20-((double)bsTruePeakLevel)/32)+__T(" dBTP")):Ztring());
    C.loudnessInfo_Data[FromAlbum][Id].Measurements=Measurements;
    Element_End0();

    return IsNOK;
}

//---------------------------------------------------------------------------
void File_Usac::loudnessInfoSetExtension()
{
    for (;;)
    {
        Element_Begin1("loudnessInfoSetExtension");
        int32u bitSize;
        int8u loudnessInfoSetExtType, extSizeBits;
        Get_S1 (4, loudnessInfoSetExtType,                      "loudnessInfoSetExtType"); Element_Info1C(loudnessInfoSetExtType<UNIDRCLOUDEXT_Max, loudnessInfoSetExtType_IdNames[loudnessInfoSetExtType]);
        if (!loudnessInfoSetExtType) // UNIDRCLOUDEXT_TERM
        {
            Element_End0();
            break;
        }
        Get_S1 (4, extSizeBits,                                 "bitSizeLen");
        extSizeBits+=4;
        Get_S4 (extSizeBits, bitSize,                           "bitSize");
        bitSize++;
        if (bitSize>Data_BS_Remain())
        {
            Trusted_IsNot("Too big");
            Element_End0();
            break;
        }

        auto B=BS_Bookmark(bitSize);
        switch (loudnessInfoSetExtType)
        {
            case UNIDRCLOUDEXT_EQ                               : loudnessInfoSet(true); break;
            default:
                Skip_BS(bitSize,                                "Unknown");
        }
        BS_Bookmark(B, (loudnessInfoSetExtType<UNIDRCLOUDEXT_Max?string(uniDrcConfigExtType_ConfNames[loudnessInfoSetExtType]):("loudnessInfoSetExtType"+to_string(loudnessInfoSetExtType)))+" Coherency");
        Element_End0();
    }
}

//---------------------------------------------------------------------------
void File_Usac::streamId()
{
    Element_Begin1("streamId");

    int16u streamIdentifier;
    Get_S2 (16, streamIdentifier,                               "streamIdentifier");

    if (!IsParsingRaw)
        Fill(Stream_Audio, 0,  "streamIdentifier", streamIdentifier, 10, true);
    Element_End0();
}

//***************************************************************************
// Elements - USAC - Frame
//***************************************************************************
#if MEDIAINFO_TRACE || MEDIAINFO_CONFORMANCE

//---------------------------------------------------------------------------
void File_Usac::UsacFrame(size_t BitsNotIncluded)
{
    Element_Begin1("UsacFrame");
    Element_Info1(Frame_Count_NotParsedIncluded==(int64u)-1? Frame_Count :Frame_Count_NotParsedIncluded);
    bool usacIndependencyFlag;
    Get_SB (usacIndependencyFlag,                               "usacIndependencyFlag");
    if (!IsParsingRaw)
    {
        if (Conf.IsNotValid || !usacIndependencyFlag)
        {
            Skip_BS(Data_BS_Remain(),                           "Data");
            Element_End0();
            return;
        }
        IsParsingRaw++;
    }

    for (size_t elemIdx=0; elemIdx<C.usacElements.size(); elemIdx++)
    {
        switch (C.usacElements[elemIdx].usacElementType&3)
        {
            case ID_USAC_SCE                                    : UsacSingleChannelElement(); elemIdx=C.usacElements.size()-1; break;
            case ID_USAC_CPE                                    : UsacChannelPairElement(); elemIdx=C.usacElements.size()-1; break;
            case ID_USAC_LFE                                    : UsacLfeElement(); elemIdx=C.usacElements.size()-1; break;
            case ID_USAC_EXT                                    : UsacExtElement(elemIdx, usacIndependencyFlag); break;
            default                                             : ; //Not parsed
        }
        if (!Trusted_Get())
            break;
    }

    auto BitsNotIncluded2=BitsNotIncluded!=(size_t)-1?BitsNotIncluded:0;
    if (Data_BS_Remain()>BitsNotIncluded2)
        Skip_BS(Data_BS_Remain()-BitsNotIncluded2,              "(Not parsed)");

    if (BitsNotIncluded!=(size_t)-1)
    {
        if (Data_BS_Remain()>BitsNotIncluded)
        {
            int8u LastByte=0xFF;
            auto BitsRemaining=Data_BS_Remain()-BitsNotIncluded;
            if (BitsRemaining<8)
                Peek_S1((int8u)BitsRemaining, LastByte);
            Skip_BS(BitsRemaining,                                  LastByte?"Unknown":"Padding");
        }
        else if (Data_BS_Remain()<BitsNotIncluded)
            Trusted_IsNot("Too big");
    }
    Element_End0();

    if (Trusted_Get())
    {
        #if MEDIAINFO_CONFORMANCE
            if (IsParsingRaw == 1)
                Merge_Conformance();
        #endif
    }
    else
    {
        #if MEDIAINFO_CONFORMANCE
            if (IsParsingRaw == 1)
            {
                Clear_Conformance();
                Fill_Conformance("UsacFrame Coherency", "Malformed bitstream");
                Merge_Conformance();
            }
        #endif

        if (IsParsingRaw==1)
        {
            IsParsingRaw--;

            //Counting, TODO: remove duplicate of this code due to not executed in case of parsing issue
            Frame_Count++;
            if (Frame_Count_NotParsedIncluded!=(int64u)-1)
                Frame_Count_NotParsedIncluded++;
        }
    }
}

//---------------------------------------------------------------------------
void File_Usac::UsacSingleChannelElement()
{
    Element_Begin1("UsacSingleChannelElement");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::UsacChannelPairElement()
{
    Element_Begin1("UsacChannelPairElement");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::UsacLfeElement()
{
    Element_Begin1("UsacLfeElement");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::UsacExtElement(size_t elemIdx, bool usacIndependencyFlag)
{
    Element_Begin1("UsacExtElement");
    int8u usacExtElementType;
    usacExtElementType=Conf.usacElements[elemIdx].usacElementType>>2;
    Element_Info1C(usacExtElementType<ID_EXT_ELE_Max, usacExtElementType_IdNames[usacExtElementType]);
    bool usacExtElementPresent;
    Get_SB (usacExtElementPresent,                              "usacExtElementPresent");
    if (usacExtElementPresent)
    {
        int32u usacExtElementPayloadLength;
        bool usacExtElementUseDefaultLength;;
        Get_SB (usacExtElementUseDefaultLength,                 "usacExtElementUseDefaultLength");
        if (usacExtElementUseDefaultLength)
            usacExtElementPayloadLength=Conf.usacElements[elemIdx].usacExtElementDefaultLength;
        else
        {
            Get_S4 (8, usacExtElementPayloadLength,             "usacExtElementPayloadLength");
            if (usacExtElementPayloadLength==0xFF)
            {
                Get_S4 (16, usacExtElementPayloadLength,        "usacExtElementPayloadLength");
                usacExtElementPayloadLength+=255-2;
            }
        }
        if (Conf.usacElements[elemIdx].usacExtElementPayloadFrag)
        {
            Skip_SB(                                            "usacExtElementStart");
            Skip_SB(                                            "usacExtElementStop");
        }

        if (usacExtElementPayloadLength)
        {
            usacExtElementPayloadLength*=8;
            if (usacExtElementPayloadLength>Data_BS_Remain())
            {
                Trusted_IsNot("Too big");
                Element_End0();
                return;
            }
            auto B=BS_Bookmark(usacExtElementPayloadLength);
            switch (usacExtElementType)
            {
                case ID_EXT_ELE_AUDIOPREROLL                    : AudioPreRoll(); break;
                default:
                    Skip_BS(usacExtElementPayloadLength,        "Unknown");
            }
            BS_Bookmark(B, (usacExtElementType<ID_EXT_ELE_Max?string(usacExtElementType_Names[usacExtElementType]):("usacExtElementType"+to_string(usacExtElementType)))+" Coherency");
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Usac::AudioPreRoll()
{
    Element_Begin1("AudioPreRoll");
    int32u configLen;
    escapedValue(configLen, 4, 4, 8,                            "configLen");
    if (configLen)
    {
        configLen*=8;
        if (configLen>Data_BS_Remain())
        {
            Trusted_IsNot("Too big");
            Element_End0();
            return;
        }
        if (IsParsingRaw<=1)
        {
            Element_Begin1("Config");
            auto B=BS_Bookmark(configLen);
            UsacConfig(B.BitsNotIncluded);
            if (!Trusted_Get())
                C=Conf; //Using default conf if new conf has a problem
            BS_Bookmark(B, "UsacConfig Coherency");
            Element_End0();
        }
        else
        {
            //No nested in nested
            Skip_BS(configLen,                                  "Config");
        }
    }
    else
    {
        if (IsParsingRaw <= 1)
            C = Conf; //Using default conf if there is no replacing conf
    }
    Skip_SB(                                                    "applyCrossfade");
    Skip_SB(                                                    "reserved");
    escapedValue(F.numPreRollFrames, 2, 4, 0,                   "numPreRollFrames");
    for (int32u frameIdx=0; frameIdx<F.numPreRollFrames; frameIdx++)
    {
        Element_Begin1("PreRollFrame");
            int32u auLen;
            escapedValue(auLen, 16, 16, 0,                      "auLen");
            auLen*=8;
            if (auLen)
            {
                if (auLen>Data_BS_Remain())
                {
                    Trusted_IsNot("Too big");
                    C=Conf; //Using default conf if new conf has a problem
                    Element_End0();
                    break;
                }
                if (IsParsingRaw<=1)
                {
                    auto FSav=F;
                    IsParsingRaw+=frameIdx+1;
                    Element_Begin1("AccessUnit");
                    auto B=BS_Bookmark(auLen);
                    UsacFrame(B.BitsNotIncluded);
                    BS_Bookmark(B, "AudioPreRoll UsacFrame Coherency");
                    Element_End0();
                    IsParsingRaw-=frameIdx+1;
                    F=FSav;
                }
                else
                {
                    //No nested in nested
                    Skip_BS(auLen,                              "AccessUnit");
                }
            }
        Element_End0();
    }
    if (!Trusted_Get())
        C=Conf; //Using default conf if new conf has a problem
    Element_End0();
}

//---------------------------------------------------------------------------
#endif //MEDIAINFO_TRACE || MEDIAINFO_CONFORMANCE

//***************************************************************************
// Utils
//***************************************************************************

//---------------------------------------------------------------------------
void File_Usac::escapedValue(int32u &Value, int8u nBits1, int8u nBits2, int8u nBits3, const char* Name)
{
    Element_Begin1(Name);
    Get_S4(nBits1, Value,                                       "nBits1");
    if (Value==((1<<nBits1)-1))
    {
        int32u ValueAdd;
        Get_S4(nBits2, ValueAdd,                                "nBits2");
        Value+=ValueAdd;
        if (nBits3 && ValueAdd==((1<<nBits2)-1))
        {
            Get_S4(nBits3, ValueAdd,                            "nBits3");
            Value+=ValueAdd;
        }
    }
    Element_Info1(Value);
    Element_End0();
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //defined(MEDIAINFO_AAC_YES) || defined(MEDIAINFO_MPEGH3DA_YES)

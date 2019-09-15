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

//TODO: move USAC content to its on class instead of expanding AAC class

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_AAC_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Aac.h"
#include <algorithm>
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Info
//***************************************************************************

//---------------------------------------------------------------------------
extern int8u Aac_AudioSpecificConfig_sampling_frequency_index(const int64s sampling_frequency);
extern const size_t Aac_sampling_frequency_Size_Usac; // USAC expands Aac_sampling_frequency[]
extern const int32u Aac_sampling_frequency[];
extern const char* const Aac_ChannelLayout[];

//---------------------------------------------------------------------------
struct coreSbrFrameLengthIndex_mapping
{
    int8u    sbrRatioIndex;
    int8u    outputFrameLengthDivided256;
};
static const size_t coreSbrFrameLengthIndex_Mapping_Size=5;
static coreSbrFrameLengthIndex_mapping coreSbrFrameLengthIndex_Mapping[coreSbrFrameLengthIndex_Mapping_Size] =
{
    { 0,  3 },
    { 0,  4 },
    { 2,  8 },
    { 3,  8 },
    { 1, 16 },
};

//---------------------------------------------------------------------------
static const size_t LoudnessMeaning_Size=8;
static const char* LoudnessMeaning[LoudnessMeaning_Size]=
{
    "Loudness_Program",
    "Loudness_Anchor",
    "Loudness_MaximumOfRange",
    "Loudness_MaximumMomentary",
    "Loudness_MaximumShortTerm",
    "Loudness_Range",
    "Loudness_ProductionMixingLevel",
    "Loudness_RoomType",
};

//***************************************************************************
// Elements - USAC
//***************************************************************************

//---------------------------------------------------------------------------
void File_Aac::UsacConfig()
{
    // Init
    loudnessInfoSet_Present=false;

    Element_Begin1("UsacConfig");
    int8u coreSbrFrameLengthIndex;
    bool usacConfigExtensionPresent;
    Get_S1 (5, sampling_frequency_index,                        "usacSamplingFrequencyIndex"); Param_Info1C(sampling_frequency_index<Aac_sampling_frequency_Size_Usac, Aac_sampling_frequency[sampling_frequency_index]);
    if (sampling_frequency_index==Aac_sampling_frequency_Size_Usac)
    {
        int32u samplingFrequency;
        Get_S3 (24, samplingFrequency,                          "usacSamplingFrequency");
        Frequency_b=samplingFrequency;
        sampling_frequency_index=Aac_AudioSpecificConfig_sampling_frequency_index(Frequency_b);
    }
    else
        Frequency_b=Aac_sampling_frequency[sampling_frequency_index];
    Get_S1 (3, coreSbrFrameLengthIndex,                         "coreSbrFrameLengthIndex");
    Get_S1 (5, channelConfiguration,                            "channelConfiguration"); Param_Info1C(channelConfiguration<8, Aac_ChannelLayout[channelConfiguration]);
    if (!channelConfiguration)
    {
        int32u numOutChannels;
        escapedValue(numOutChannels, 5, 8, 16,                  "numOutChannels");
        for (int32u i=0; i<numOutChannels; i++)
            Skip_S1(5,                                          "bsOutChannelPos"); //TODO: mapping
    }
    if (coreSbrFrameLengthIndex>=coreSbrFrameLengthIndex_Mapping_Size)
    {
        Skip_BS(Data_BS_Remain(),                               "(Not implemented)");
        Element_End0();
        return;
    }
    UsacDecoderConfig(coreSbrFrameLengthIndex);
    Get_SB (usacConfigExtensionPresent,                         "usacConfigExtensionPresent");
    if (usacConfigExtensionPresent)
        UsacConfigExtension();
    Element_End0();

    // Filling
    Fill(Stream_Audio, 0, Audio_SamplesPerFrame, coreSbrFrameLengthIndex_Mapping[coreSbrFrameLengthIndex].outputFrameLengthDivided256 << 8, true);
    if (!drcInstructionsUniDrc_Data.empty())
    {
        Fill(Stream_Audio, 0, "DrcSets_Count", drcInstructionsUniDrc_Data.size());
        ZtringList Ids, Data;
        for (std::map<Ztring, drc_info>::iterator Item=drcInstructionsUniDrc_Data.begin(); Item!=drcInstructionsUniDrc_Data.end(); ++Item)
        {
            Ids.push_back(Item->first);
            Data.push_back(Ztring().From_UTF8(Item->second.drcSetEffectTotal));
        }
        Fill(Stream_Audio, 0, "DrcSets_Effects", Data, Ids);
    }
    bool DefaultIdPresent=false;
    for (int8u i=0; i<2; i++)
    {
        if (!loudnessInfo_Data[i].empty())
            Fill(Stream_Audio, 0, i?"Loudness_Count_Album":"Loudness_Count", loudnessInfo_Data[i].size());
        ZtringList Ids;
        ZtringList SamplePeakLevel;
        ZtringList TruePeakLevel;
        ZtringList Measurements[16];
        for (std::map<Ztring, loudness_info>::iterator Item=loudnessInfo_Data[i].begin(); Item!=loudnessInfo_Data[i].end(); ++Item)
        {
            Ids.push_back(Item->first);
            SamplePeakLevel.push_back(Item->second.SamplePeakLevel);
            TruePeakLevel.push_back(Item->second.TruePeakLevel);
            for (int8u j=1; j<16; j++)
                Measurements[j].push_back(Item->second.Measurements.Values[j]);
        }
        if (Ids.size()>=1 && Ids.front().empty())
        {
            if (!i)
                DefaultIdPresent=true;
            if (Ids.size()==1)
                Ids.clear();
        }
        Fill(Stream_Audio, 0, i?"SamplePeakLevel_Album":"SamplePeakLevel",SamplePeakLevel, Ids);
        Fill(Stream_Audio, 0, i?"TruePeakLevel_Album":"TruePeakLevel", TruePeakLevel, Ids);
        for (int8u j=1; j<16; j++)
        {
            string Field;
            if (j<LoudnessMeaning_Size)
                Field=LoudnessMeaning[j-1];
            if (i)
                Field+="_Album";
            Fill(Stream_Audio, 0, Field.c_str(), Measurements[j], Ids);
        }

    }
    if (!loudnessInfoSet_Present)
    {
        Fill(Stream_Audio, 0, "ConformanceCheck", "Invalid: loudnessInfoSet is missing");
        Fill(Stream_Audio, 0, "ConformanceCheck/Short", "Invalid: loudnessInfoSet missing");
    }
    else if (loudnessInfo_Data[0].empty())
    {
        Fill(Stream_Audio, 0, "ConformanceCheck", "Invalid: loudnessInfoSet is empty");
        Fill(Stream_Audio, 0, "ConformanceCheck/Short", "Invalid: loudnessInfoSet empty");
    }
    else if (!DefaultIdPresent)
    {
        Fill(Stream_Audio, 0, "ConformanceCheck", "Invalid: Default loudnessInfo is missing");
        Fill(Stream_Audio, 0, "ConformanceCheck/Short", "Invalid: Default loudnessInfoSet missing");
    }
    else if (loudnessInfo_Data[0].begin()->second.Measurements.Values[1].empty() && loudnessInfo_Data[0].begin()->second.Measurements.Values[2].empty())
    {
        Fill(Stream_Audio, 0, "ConformanceCheck", "Invalid: None of program loudness or anchor loudness is present in default loudnessInfo");
        Fill(Stream_Audio, 0, "ConformanceCheck/Short", "Invalid: Default loudnessInfo incomplete");
    }
}

//---------------------------------------------------------------------------
void File_Aac::UsacDecoderConfig(int8u coreSbrFrameLengthIndex)
{
    Element_Begin1("UsacDecoderConfig");
    int32u numElements;
    escapedValue(numElements, 4, 8, 16,                         "numElements minus 1");

    for (int32u elemIdx=0; elemIdx<=numElements; elemIdx++)
    {
        Element_Begin1("usacElement");
        int8u usacElementType;
        Get_S1(2, usacElementType,                              "usacElementType");
        switch (usacElementType)
        {
            case 0: //ID_USAC_SCE
                UsacSingleChannelElementConfig(coreSbrFrameLengthIndex);
                break;
            case 1: //ID_USAC_CPE
                UsacChannelPairElementConfig(coreSbrFrameLengthIndex);
                break;
            case 2: //ID_USAC_LFE
                UsacLfeElementConfig(); 
                break;
            case 3: //ID_USAC_EXT
                UsacExtElementConfig();
                break;
        }
        Element_End0();
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Aac::UsacSingleChannelElementConfig(int8u coreSbrFrameLengthIndex)
{
    Element_Begin1("UsacSingleChannelElementConfig");

    UsacCoreConfig();
    if (coreSbrFrameLengthIndex_Mapping[coreSbrFrameLengthIndex].sbrRatioIndex)
        SbrConfig();

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Aac::UsacChannelPairElementConfig(int8u coreSbrFrameLengthIndex)
{
    Element_Begin1("UsacChannelPairElementConfig");

    UsacCoreConfig();
    if (coreSbrFrameLengthIndex_Mapping[coreSbrFrameLengthIndex].sbrRatioIndex)
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
void File_Aac::UsacLfeElementConfig()
{
    // Nothing here
}

//---------------------------------------------------------------------------
static const size_t UsacExtElementConfig_usacExtElementType_Size=5;
static const char* UsacExtElementConfig_usacExtElementType[UsacExtElementConfig_usacExtElementType_Size]=
{
    "FILL",
    "MPEGS",
    "SAOC",
    "AUDIOPREROLL",
    "UNI_DRC",
};
void File_Aac::UsacExtElementConfig()
{
    Element_Begin1("UsacExtElementConfig");

    int32u usacExtElementType, usacExtElementConfigLength, usacExtElementDefaultLength;
    bool usacExtElementDefaultLengthPresent, usacExtElementPayloadFlag;
    escapedValue(usacExtElementType, 4, 8, 16,                  "usacExtElementType"); Param_Info1C(usacExtElementType<UsacExtElementConfig_usacExtElementType_Size, UsacExtElementConfig_usacExtElementType[usacExtElementType]); Element_Info1C(usacExtElementType<UsacExtElementConfig_usacExtElementType_Size, UsacExtElementConfig_usacExtElementType[usacExtElementType]);
    escapedValue(usacExtElementConfigLength, 4, 8, 16,          "usacExtElementConfigLength");
    Get_SB (usacExtElementDefaultLengthPresent,                 "usacExtElementDefaultLengthPresent");
    if (usacExtElementDefaultLengthPresent)
        escapedValue(usacExtElementDefaultLength, 8, 16, 0,     "usacExtElementDefaultLength");
    else
        usacExtElementDefaultLength=0;
    Get_SB (usacExtElementPayloadFlag,                          "usacExtElementPayloadFlag");

    size_t End;
    if (Data_BS_Remain()>usacExtElementConfigLength*8)
        End=Data_BS_Remain()-usacExtElementConfigLength*8;
    else
        End=0; //Problem
    switch (usacExtElementType)
    {
        case 0: //ID_EXT_ELE_FILL
        case 3: //ID_EXT_ELE_AUDIOPREROLL
            break;
        case 4: //ID_EXT_ELE_UNI_DRC
            uniDrcConfig();
            break;
        default:
            if (usacExtElementConfigLength)
                Skip_BS(usacExtElementConfigLength*8,           "(Unknown)");
            break;
    }
    if (Data_BS_Remain()>End)
    {
        size_t Size=Data_BS_Remain()-End;
        int8u Padding=1;
        if (Size<8)
            Peek_S1((int8u)Size, Padding);
        Skip_BS(Data_BS_Remain()-End,                           Padding?"(Unknown)":"Padding");
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Aac::UsacCoreConfig()
{
    Element_Begin1("UsacCoreConfig");

    Skip_SB(                                                    "tw_mdct");
    Skip_SB(                                                    "noiseFilling");

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Aac::SbrConfig()
{
    Element_Begin1("SbrConfig");

    Skip_SB(                                                    "harmonicsSBR");
    Skip_SB(                                                    "bs_interTes");
    Skip_SB(                                                    "bs_pvc");
    SbrDlftHeader();

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Aac::SbrDlftHeader()
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
        Skip_S1(2,                                              "dflt_limiter_bands;");
        Skip_S1(2,                                              "dflt_limiter_gains");
        Skip_SB(                                                "dflt_interpol_freq;");
        Skip_SB(                                                "dflt_smoothing_mode;");
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Aac::Mps212Config(int8u StereoConfigindex)
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
void File_Aac::uniDrcConfig()
{
    Element_Begin1("uniDrcConfig");

    int8u downmixInstructionsCount, drcCoefficientsBasicCount, drcInstructionsBasicCount, drcCoefficientsUniDrcCount, drcInstructionsUniDrcCount;
    TEST_SB_SKIP(                                               "sampleRatePresent");
        Skip_S3(18,                                             "bsSampleRate");
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
void File_Aac::uniDrcConfigExtension()
{
    Element_Begin1("uniDrcConfigExtension");

    for (;;)
    {
        int32u bitSize;
        int8u uniDrcConfigExtType, extSizeBits;
        Get_S1 (4, uniDrcConfigExtType,                         "uniDrcConfigExtType");
        if (!uniDrcConfigExtType) // UNIDRCCONFEXT_TERM
            break;
        Get_S1 (4, extSizeBits,                                 "bitSizeLen");
        extSizeBits+=4;
        Get_S4 (extSizeBits, bitSize,                           "bitSize");
        bitSize++;

        switch (uniDrcConfigExtType)
        {
            /*
            case 1 : // UNIDRCCONFEXT_PARAM_DRC
                {
                size_t End;
                if (Data_BS_Remain()>bitSize)
                    End=Data_BS_Remain()-bitSize;
                else
                    End=0; //Problem
                drcCoefficientsParametricDrc();
                int8u parametricDrcInstructionsCount;
                Get_S1 (4, parametricDrcInstructionsCount,      "parametricDrcInstructionsCount");
                for (int8u i=0; i<parametricDrcInstructionsCount; i++)
                    parametricDrcInstructions();
                if (Data_BS_Remain()>End)
                    Skip_BS(Data_BS_Remain()-End,               "(Unknown)");
                }
                break;
            */
            case 2 : // UNIDRCCONFEXT_V1
                {
                size_t End;
                if (Data_BS_Remain()>bitSize)
                    End=Data_BS_Remain()-bitSize;
                else
                    End=0; //Problem
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
                if (Data_BS_Remain()>End)
                    Skip_BS(Data_BS_Remain()-End,               "(Unknown)");
                }
                break;
            default:
                if (bitSize)
                    Skip_BS(bitSize,                            "(Unknown)");
        }
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Aac::downmixInstructions(bool V1)
{
    Element_Begin1("downmixInstructionsV1");

    bool layoutSignalingPresent;
    Skip_S1(7,                                                  "downmixId");
    Get_S1 (7, targetChannelCount,                              "targetChannelCount");
    Skip_S1(8,                                                  "targetLayout");
    Get_SB (   layoutSignalingPresent,                          "layoutSignalingPresent");
    if (layoutSignalingPresent)
    {
        if (V1)
            Skip_S1(4,                                          "bsDownmixOffset");
        for (int8u i=0; i<targetChannelCount; i++)
            for (int8u j=0; j<baseChannelCount; j++)
                Skip_S1(V1?5:4,                                 V1?"bsDownmixCoefficientV1":"bsDownmixCoefficient");
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Aac::drcCoefficientsBasic()
{
    Element_Begin1("drcCoefficientsBasic");

    Skip_S1(4,                                                  "drcLocation");
    Skip_S1(7,                                                  "drcCharacteristic");

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Aac::drcCoefficientsUniDrc(bool V1)
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
    gainSets.clear();
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
        gainSets.push_back(gainSet);
        Element_End0();
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Aac::drcInstructionsBasic()
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

static const size_t drcSetEffect_List_Size=12;
static const char* drcSetEffect_List[drcSetEffect_List_Size] =
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

//---------------------------------------------------------------------------
void File_Aac::drcInstructionsUniDrc(bool V1)
{
    Element_Begin1("drcInstructionsUniDrcV1");

    int8u channelCount=baseChannelCount;
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
            drcApplyToDownmix=drcApplyToDownmix?true:false;
        TEST_SB_SKIP(                                           "additionalDownmixIdPresent");
            int8u additionalDownmixIdCount;
            Get_S1 (3, additionalDownmixIdCount,                "additionalDownmixIdCount");
            for (int8u i=0; i<additionalDownmixIdCount; i++)
                Skip_S1(7,                                      "additionalDownmixId");
        TEST_SB_END();
        if ((!V1 || drcApplyToDownmix) && downmixId && downmixId!=0x7F)
            channelCount=targetChannelCount; //targetChannelCountFromDownmix
        else if (downmixId==0x7F)
            channelCount=1;
    }
    else
        downmixId=0; // 0 is default
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
        if (!bsGainSetIndex || bsGainSetIndex>gainSets.size()) // 1-based
        {
            Skip_BS(Data_BS_Remain(),                               "(Not implemented)");
            Element_End0();
            Element_End0();
            return;
        }
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
        Element_Begin1("DrcChannel");
        int8u bandCount=gainSets[*DrcChannelGroup-1].bandCount;
        for (int8u k=0; k<bandCount; k++)
        {
            Element_Begin1("band");
            TEST_SB_SKIP(                                       "targetCharacteristicLeftPresent");
                Skip_S1(4,                                      "targetCharacteristicLeftIndex");
            TEST_SB_END();
            TEST_SB_SKIP(                                       "targetCharacteristicRightPresent");
                Skip_S1(4,                                      "targetCharacteristicRightIndex");
            TEST_SB_END();
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
        if (bandCount==1)
        {
            TEST_SB_SKIP(                                       "shapeFilterPresent");
                Skip_S1(4,                                      "shapeFilterIndex");
            TEST_SB_END();
        }
        Element_End0();
    }

    Element_End0();

    if (V1) //We want to display only V1 information
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

        Ztring Id=Ztring::ToZtring(drcSetId)+=__T('-')+Ztring::ToZtring(downmixId);
        if (Id==__T("0-0"))
            Id.clear();
        drcInstructionsUniDrc_Data[Id].drcSetEffectTotal=Value;
    }
}

//---------------------------------------------------------------------------
void File_Aac::channelLayout()
{
    Element_Begin1("channelLayout");

    bool layoutSignalingPresent;
    Get_S1 (7, baseChannelCount,                                "baseChannelCount");
    targetChannelCount=baseChannelCount;
    Get_SB (   layoutSignalingPresent,                          "layoutSignalingPresent");
    if (layoutSignalingPresent)
    {
        int8u definedLayout;
        Get_S1 (8, definedLayout,                               "definedLayout");
        if (!definedLayout)
        {
            for (int8u i=0; i<baseChannelCount; i++)
                Skip_S1(7,                                      "speakerPosition");
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
void File_Aac::UsacConfigExtension()
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
        size_t End;
        if (Data_BS_Remain()>usacConfigExtLength*8)
            End=Data_BS_Remain()-usacConfigExtLength*8;
        else
            End=0; //Problem

        switch (usacConfigExtType)
        {
            case 0: //ID_CONFIG_EXT_FILL
                if (usacConfigExtLength)
                    Skip_BS(usacConfigExtLength *8,             "10100101"); //TODO: check if value is the right one
                break;
            case 2: //ID_CONFIG_EXT_LOUDNESS_INFO
                loudnessInfoSet();
                break;
            case 7: //ID_CONFIG_EXT_STREAM_ID
                streamId();
                break;
            default:
                if (usacConfigExtLength)
                    Skip_BS(usacConfigExtLength *8,             "(Unknown)");
                break;
        }

        if (Data_BS_Remain()>End)
        {
            size_t Size=Data_BS_Remain()-End;
            int8u Padding=1;
            if (Size<8)
                Peek_S1((int8u)Size, Padding);
            Skip_BS(Data_BS_Remain()-End,                           Padding?"(Unknown)":"Padding");
        }
        Element_End0();
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Aac::loudnessInfoSet(bool V1)
{
    Element_Begin1(V1?"loudnessInfoSetV1":"loudnessInfoSet");
    loudnessInfoSet_Present=true;

    int8u loudnessInfoAlbumCount, loudnessInfoCount;
    bool loudnessInfoSetExtPresent;
    Get_S1 (6, loudnessInfoAlbumCount,                          "loudnessInfoAlbumCount");
    Get_S1 (6, loudnessInfoCount,                               "loudnessInfoCount");
    for (int8u i=0; i<loudnessInfoAlbumCount; i++)
        loudnessInfo(true, V1);
    for (int8u i=0; i<loudnessInfoCount; i++)
        loudnessInfo(false, V1);
    if (!V1)
    {
        Get_SB (loudnessInfoSetExtPresent,                     "loudnessInfoSetExtPresent");
        if (loudnessInfoSetExtPresent)
            loudnessInfoSetExtension();
    }

    Element_End0();
}

//---------------------------------------------------------------------------
static const size_t methodDefinition_Format_Size=10;
static const int8u methodDefinition_Format[methodDefinition_Format_Size]=
{
    8, 8, 8, 8, 8, 8, 8, 5, 2, 8,
};
void File_Aac::loudnessInfo(bool FromAlbum, bool V1)
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
    Get_S1 (7, downmixId,                                       "downmixId");
    Get_SB (samplePeakLevelPresent,                             "samplePeakLevelPresent");
    if (samplePeakLevelPresent)
        Get_S2(12, bsSamplePeakLevel,                           "bsSamplePeakLevel");
    Get_SB (truePeakLevelPresent,                               "truePeakLevelPresent");
    if (truePeakLevelPresent)
    {
        Get_S2 (12, bsTruePeakLevel,                            "bsTruePeakLevel");
        Skip_S1( 4,                                             "measurementSystem");
        Skip_S1( 2,                                             "reliability");
    }
    Get_S1 (4, measurementCount,                                "measurementCount");
    for (int8u i=0; i<measurementCount; i++)
    {
        int8u methodDefinition, methodValue;
        Get_S1 (4, methodDefinition,                            "methodDefinition");
        if (methodDefinition>=methodDefinition_Format_Size)
        {
            Skip_BS(Data_BS_Remain(),                           "(Unsupported)");
        }
        Get_S1 (methodDefinition_Format[methodDefinition], methodValue, "methodValue");
        Skip_S1(4,                                              "measurementSystem");
        Skip_S1(2,                                              "reliability");

        Ztring measurement;
        switch (methodDefinition)
        {
            case 0:
                    break;
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
                    measurement=Ztring::ToZtring(-57.75+((double)methodValue)/4, 2)+__T(" LKFS");
                    break;
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
                    measurement=Ztring::ToZtring(Value)+__T(" LU");
                    }
                    break;
            case 7: 
                    measurement=Ztring::ToZtring(80+methodValue)+__T(" dB");
                    break;
            case 8:
                    switch(methodValue)
                    {
                        case 0: break;
                        case 1: measurement=__T("Large room"); break;
                        case 2: measurement=__T("Small room"); break;
                        default: measurement=Ztring::ToZtring(methodValue); break;
                    }
                    break;
            case 9:
                    measurement=Ztring::ToZtring(-116+((double)methodValue)/2, 1)+__T(" LKFS");
                    break;
            default:
                    measurement.From_Number(methodValue);
        }

        if (methodDefinition)
        {
            Ztring& Content=Measurements.Values[methodDefinition];
            if (!Content.empty())
                Content+=__T(" & ");
            Content+=measurement;
        }
    }

    Ztring Id=Ztring::ToZtring(drcSetId);
    Id+=__T('-')+Ztring::ToZtring(downmixId);
    //if (V1)
    //    Id+=__T('-')+Ztring::ToZtring(eqSetId); // No eqSetId for the moment
    if (Id==__T("0-0") || Id==__T("0-0-0"))
        Id.clear();
    loudnessInfo_Data[FromAlbum][Id].SamplePeakLevel=((samplePeakLevelPresent && bsSamplePeakLevel)?(Ztring::ToZtring(20-((double)bsSamplePeakLevel)/32)+__T(" dBFS")):Ztring());
    loudnessInfo_Data[FromAlbum][Id].TruePeakLevel=((truePeakLevelPresent && bsTruePeakLevel)?(Ztring::ToZtring(20-((double)bsTruePeakLevel)/32)+__T(" dBTP")):Ztring());
    loudnessInfo_Data[FromAlbum][Id].Measurements=Measurements;
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Aac::loudnessInfoSetExtension()
{
    Element_Begin1("loudnessInfoSetExtension");

    for (;;)
    {
        int32u bitSize;
        int8u loudnessInfoSetExtType, extSizeBits;
        Get_S1 (4, loudnessInfoSetExtType,                      "loudnessInfoSetExtType");
        if (!loudnessInfoSetExtType) // UNIDRCLOUDEXT_TERM
            break;
        Get_S1 (4, extSizeBits,                                 "bitSizeLen");
        extSizeBits+=4;
        Get_S4 (extSizeBits, bitSize,                           "bitSize");
        bitSize++;

        switch (loudnessInfoSetExtType)
        {
            case 1 : // UNIDRCLOUDEXT_EQ
                loudnessInfoSet(true);
                break;
            default:
                if (bitSize)
                    Skip_BS(bitSize,                            "(Unknown)");
        }
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Aac::streamId()
{
    Element_Begin1("streamId");

    int16u streamIdentifier;
    Get_S2 (16, streamIdentifier,                               "streamIdentifier");

    Fill(Stream_Audio, 0,  "streamIdentifier", streamIdentifier);
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Aac::escapedValue(int32u &Value, int8u nBits1, int8u nBits2, int8u nBits3, const char* Name)
{
    Element_Begin1(Name);
    Get_S4(nBits1, Value,                                       "nBits1");
    if (Value==((1<<nBits1)-1))
    {
        int32u ValueAdd;
        Get_S4(nBits2, ValueAdd,                                "nBits2");
        Value+=ValueAdd;
        if (nBits3 && Value==((1<<nBits2)-1))
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

#endif //MEDIAINFO_AAC_YES

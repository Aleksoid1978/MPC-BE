/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_Mpegh3daH
#define MediaInfo_File_Mpegh3daH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Tag/File__Tags.h"
#include "MediaInfo/Audio/File_Usac.h"
#include "MediaInfo/Audio/File_Aac.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Mpegh3da
//***************************************************************************

struct speaker_info
{
    Aac_OutputChannel CICPspeakerIdx;
    int16u AzimuthAngle;
    bool AzimuthDirection;
    int16u ElevationAngle;
    bool ElevationDirection;
    bool isLFE;
};

class File_Mpegh3da : public File_Usac
{
public :
    //In
    bool   MustParse_mhaC;
    bool   MustParse_mpegh3daFrame;

    //Constructor/Destructor
    File_Mpegh3da();

private :
    //Info
    enum SignalGroupType
    {
        SignalGroupTypeChannels,
        SignalGroupTypeObject,
        SignalGroupTypeSAOC,
        SignalGroupTypeHOA,
    };

    enum UsacElementType
    {
        ID_USAC_SCE,
        ID_USAC_CPE,
        ID_USAC_LFE,
        ID_USAC_EXT,
    };

    enum UsacExtElementType
    {
        ID_EXT_ELE_FILL,
        ID_EXT_ELE_MPEGS,
        ID_EXT_ELE_SAOC,
        ID_EXT_ELE_AUDIOPREROLL,
        ID_EXT_ELE_UNI_DRC,
        ID_EXT_ELE_OBJ_METADATA,
        ID_EXT_ELE_SAOC_3D,
        ID_EXT_ELE_HOA,
        ID_EXT_ELE_FMT_CNVRTR,
        ID_EXT_ELE_MCT,
        ID_EXT_ELE_TCC,
        ID_EXT_ELE_HOA_ENH_LAYER,
        ID_EXT_ELE_HREP,
        ID_EXT_ELE_ENHANCED_OBJ_METADATA,
    };

    enum UsacConfigExtType
    {
        ID_CONFIG_EXT_FILL,
        ID_CONFIG_EXT_DOWNMIX,
        ID_CONFIG_EXT_LOUDNESS_INFO,
        ID_CONFIG_EXT_AUDIOSCENE_INFO,
        ID_CONFIG_EXT_HOA_MATRIX,
        ID_CONFIG_EXT_ICG,
        ID_CONFIG_EXT_SIG_GROUP_INFO,
        ID_CONFIG_EXT_COMPATIBLE_PROFILE_LEVEL_SET,
    };

    enum MaeDataType
    {
        ID_MAE_GROUP_DESCRIPTION,
        ID_MAE_SWITCHGROUP_DESCRIPTION,
        ID_MAE_GROUP_CONTENT,
        ID_MAE_GROUP_COMPOSITE,
        ID_MAE_SCREEN_SIZE,
        ID_MAE_GROUP_PRESET_DESCRIPTION,
        ID_MAE_DRC_UI_INFO,
        ID_MAE_SCREEN_SIZE_EXTENSION,
        ID_MAE_GROUP_PRESET_EXTENSION,
        ID_MAE_LOUDNESS_COMPENSATION,
    };

    struct usac_element
    {
        UsacElementType Type;

        usac_element(UsacElementType Type) :
            Type(Type)
        {};
    };
    vector<usac_element> Elements;

    struct speaker_layout
    {
        int32u numSpeakers;
        vector<Aac_OutputChannel> CICPspeakerIdxs;
        vector<speaker_info> SpeakersInfo;

        int8u ChannelLayout;
        speaker_layout() :
            numSpeakers(0),
            ChannelLayout(0)
        {};
    };

    speaker_layout referenceLayout;

    int32u numElements;

    int16u numAudioChannels;
    int16u numAudioObjects;
    int16u numSAOCTransportChannels;
    int16u numHOATransportChannels;

    int8u bsNumSignalGroups;

    int8u isMainStream;
    int8u audioSceneInfoID;
    int8u mpegh3daProfileLevelIndication;
    vector<int8u> mpegh3daCompatibleProfileLevelSet;
    int32u usacSamplingFrequency;
    int8u coreSbrFrameLengthIndex;
    std::map<int8u, drc_infos> Mpegh3da_drcInstructionsUniDrc_Data[4]; // By type, by group id, By id
    struct loudness_info_data
    {
        loudness_infos Data[2];
    };
    std::map<int8u, loudness_info_data> Mpegh3da_loudnessInfo_Data[4]; // By type, by group id, By non-album/album then by id
    set<int32u> MHASPacketLabels;

    struct switch_group
    {
        vector<int8u> MemberID;
        map<string, string> Description;
        int8u ID;
        int8u DefaultGroupID;
        bool allowOnOff;
        bool defaultOnOff;
    };
    vector<switch_group> SwitchGroups;

    struct group
    {
        vector<int8u> MemberID;
        map<string, string> Description;
        string Language;
        int8u ID;
        int8u Kind;
        bool allowOnOff;
        bool defaultOnOff;

        group() :
            Kind(0)
        {}
    };
    vector<group> Groups;

    struct group_preset
    {
        struct condition
        {
            int8u ReferenceID;
            bool ConditionOnOff;
        };
        vector<condition> Conditions;
        map<string, string> Description;
        int8u ID;
        int8u Kind;
    };
    vector<group_preset> GroupPresets;

    struct signal_group
    {
        int8u Type;
        int32u bsNumberOfSignals;
        speaker_layout Layout;
    };
    vector<signal_group> SignalGroups;

    //Streams management
    void Streams_Fill();
    void Streams_Finish();

    //Buffer - Global
    void Read_Buffer_Continue();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void Sync();
    void Marker();
    void Crc16();
    void BufferInfo();
    void mpegh3daConfig();
    void SpeakerConfig3d(speaker_layout& Layout);
    void mpegh3daFlexibleSpeakerConfig(speaker_layout& Layout);
    void mpegh3daSpeakerDescription(speaker_info& SpeakerInfo, bool angularPrecision);
    void mpegh3daFrame();
    void FrameworkConfig3d();
    void mpegh3daDecoderConfig();
    void mpegh3daSingleChannelElementConfig(int8u sbrRatioIndex);
    void mpegh3daChannelPairElementConfig(int8u sbrRatioIndex);
    void mpegh3daExtElementConfig();
    bool mpegh3daCoreConfig();
    void mpegh3daUniDrcConfig();
    void downmixConfig();
    void mpegh3daLoudnessInfoSet();
    void ObjectMetadataConfig();
    void SAOC3DSpecificConfig();
    int32u SAOC3DgetNumChannels(speaker_layout Layout);
    void MCTConfig();
    void TccConfig();
    void EnhancedObjectMetadataConfig();
    void mpegh3daConfigExtension();
    void SignalGroupInformation();
    void CompatibleProfileLevelSet();
    void HoaRenderingMatrixSet();
    void ICGConfig();
    void mae_AudioSceneInfo();
    void mae_GroupDefinition(int8u numGroups);
    void mae_SwitchGroupDefinition(int8u numSwitchGroups);
    void mae_GroupPresetDefinition(int8u numGroupPresets);
    void mae_Data(int8u numGroups, int8u numGroupPresets);
    void mae_Description(MaeDataType type);
    void mae_ContentData();
    void mae_CompositePair();
    void mae_ProductionScreenSizeData();
    void mae_DrcUserInterfaceInfo(int16u dataLength);
    void mae_ProductionScreenSizeDataExtension();
    void mae_GroupPresetDefinitionExtension(int8u numGroupPresets);
    void mae_LoudnessCompensationData(int8u numGroups, int8u numGroupPresets);
    void audioTruncationInfo();
    void mhaC();

    //Helpers
    size_t num_objects_Get();
    void Streams_Fill_ChannelLayout(const string& Prefix, const speaker_layout& Layout, int8u speakerLayoutType=0);
};

} //NameSpace

#endif

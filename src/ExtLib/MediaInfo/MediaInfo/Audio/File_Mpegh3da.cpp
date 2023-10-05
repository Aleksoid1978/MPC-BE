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
#if defined(MEDIAINFO_MPEGH3DA_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Mpegh3da.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include <cmath>

using namespace ZenLib;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Info
//***************************************************************************

extern const int32u Aac_sampling_frequency[];
struct coreSbrFrameLengthIndex_mapping
{
    int8u    sbrRatioIndex;
    int16u   coreCoderFrameLength;
    int8u    outputFrameLengthDivided256;
};
extern const size_t coreSbrFrameLengthIndex_Mapping_Size;
extern coreSbrFrameLengthIndex_mapping coreSbrFrameLengthIndex_Mapping[];
extern int8u Aac_Channels_Get(int8u ChannelLayout);
extern string Aac_Channels_GetString(int8u ChannelLayout);
extern string Aac_ChannelConfiguration_GetString(int8u ChannelLayout);
extern string Aac_ChannelConfiguration2_GetString(int8u ChannelLayout);
extern string Aac_ChannelLayout_GetString(const Aac_OutputChannel* const OutputChannels, size_t OutputChannels_Size);
extern string Aac_ChannelLayout_GetString(int8u ChannelLayout, bool IsMpegh3da=false, bool IsTip=false);
extern string Aac_ChannelLayout_GetString(const vector<Aac_OutputChannel>& OutputChannels);
extern string Aac_ChannelMode_GetString(int8u ChannelLayout, bool IsMpegh3da=false);
extern string Aac_ChannelMode_GetString(const vector<Aac_OutputChannel>& OutputChannels);
extern string Aac_OutputChannelPosition_GetString(int8u OutputChannelPosition);

//---------------------------------------------------------------------------
static const char* const Mpegh3da_Profile[]=
{
    "Main",
    "High",
    "LC",
    "BL",
};
static size_t Mpegh3da_Profile_Size=sizeof(Mpegh3da_Profile)/sizeof(const char* const);
extern string Mpegh3da_Profile_Get(int8u mpegh3daProfileLevelIndication)
{
    if (!mpegh3daProfileLevelIndication)
        return string();
    if (mpegh3daProfileLevelIndication>=5*Mpegh3da_Profile_Size)
        return Ztring::ToZtring(mpegh3daProfileLevelIndication).To_UTF8(); // Raw value
    return string(Mpegh3da_Profile[(mpegh3daProfileLevelIndication-1)/5])+"@L"+char('1'+((mpegh3daProfileLevelIndication-1)%5));
}

//---------------------------------------------------------------------------
static const char* const Mpegh3da_MHASPacketType[]=
{
    "FILLDATA",
    "MPEGH3DACFG",
    "MPEGH3DAFRAME",
    "AUDIOSCENEINFO",
    "",
    "",
    "SYNC",
    "SYNCGAP",
    "MARKER",
    "CRC16",
    "CRC32",
    "DESCRIPTOR",
    "USERINTERACTION",
    "LOUDNESS_DRC",
    "BUFFERINFO",
    "GLOBAL_CRC16",
    "GLOBAL_CRC32",
    "AUDIOTRUNCATION",
    "GENDATA",
};
static const size_t Mpegh3da_MHASPacketType_Size=sizeof(Mpegh3da_MHASPacketType)/sizeof(const char* const);

//---------------------------------------------------------------------------
static const char* const Mpegh3da_contentKind[]=
{
    "",
    "Complete Main",
    "Dialogue",
    "Music",
    "Effect",
    "Mixed",
    "LFE",
    "Voiceover",
    "Spoken Subtitle",
    "Visually Impaired or Audio Description",
    "Commentary",
    "Hearing Impaired",
    "Emergency",
};
static const size_t Mpegh3da_contentKind_Size=sizeof(Mpegh3da_contentKind)/sizeof(const char* const);

//---------------------------------------------------------------------------
static const char* const Mpegh3da_groupPresetKind[]=
{
    "",
    "Integrated TV Loudspeaker",
    "High Quality Loudspeaker",
    "Mobile Loudspeakers",
    "Mobile Headphones",
    "Hearing Impaired (light)",
    "Hearing Impaired (heavy)",
    "Visually Impaired or Audio Description",
    "Spoken Subtitles",
    "Loudness or DRC",
};
static const size_t Mpegh3da_groupPresetKind_Size=sizeof(Mpegh3da_groupPresetKind)/sizeof(const char* const);

//---------------------------------------------------------------------------
static const char* const Mpegh3da_signalGroupType[]=
{
    "Channels",
    "Object",
    "SAOC",
    "HOA",
};
static const size_t Mpegh3da_signalGroupType_Size=sizeof(Mpegh3da_signalGroupType)/sizeof(const char* const);

//---------------------------------------------------------------------------
static const char* const Mpegh3da_marker_byte[]=
{
    "",
    "Configuration change marker",
    "Random access or Immediate playout marker",
    "Program boundary marker",
};
static const size_t Mpegh3da_marker_byte_Size=sizeof(Mpegh3da_marker_byte)/sizeof(const char* const);

//---------------------------------------------------------------------------
static const char* const Mpegh3da_usacElementType[4]=
{
    "SCE",
    "CPE",
    "LFE",
    "EXT",
};

//---------------------------------------------------------------------------
static const char* const Mpegh3da_usacExtElementType[]=
{
    "FILL",
    "MPEGS",
    "SAOC",
    "AUDIOPREROLL",
    "UNI_DRC",
    "OBJ_METADATA",
    "SAOC_3D",
    "HOA",
    "FMT_CNVRTR",
    "MCT",
    "TCC",
    "HOA_ENH_LAYER",
    "HREP",
    "ENHANCED_OBJ_METADATA",
};
static const size_t Mpegh3da_usacExtElementType_Size=sizeof(Mpegh3da_usacExtElementType)/sizeof(const char* const);

//---------------------------------------------------------------------------
static const char* const Mpegh3da_usacConfigExtType[]=
{
    "FILL",
    "DOWNMIX",
    "LOUDNESS_INFO",
    "AUDIOSCENE_INFO",
    "HOA_MATRIX",
    "ICG",
    "SIG_GROUP_INFO",
    "COMPATIBLE_PROFILE_LEVEL_SET",
};
static const size_t Mpegh3da_usacConfigExtType_Size=sizeof(Mpegh3da_usacConfigExtType)/sizeof(const char* const);

//---------------------------------------------------------------------------
static const size_t Mpegh3da_SpeakerInfo_Size=43;
static const speaker_info Mpegh3da_SpeakerInfo[Mpegh3da_SpeakerInfo_Size]=
{
    {CH_M_L030,  30, false,  0, false, false},
    {CH_M_R030,  30, true ,  0, false, false},
    {CH_M_000 ,   0, false,  0, false, false},
    {CH_LFE   ,   0, false, 15, true ,  true},
    {CH_M_L110, 110, false,  0, false, false},
    {CH_M_R110, 110, true ,  0, false, false},
    {CH_M_L022,  22, false,  0, false, false},
    {CH_M_R022,  22, true ,  0, false, false},
    {CH_M_L135, 135, false,  0, false, false},
    {CH_M_R135, 135, true ,  0, false, false},
    {CH_M_180 , 180, false,  0, false, false},
    {CH_M_LSD , 135, false,  0, false, false},
    {CH_M_RSD , 135, true ,  0, false, false},
    {CH_M_L090,  90, false,  0, false, false},
    {CH_M_R090,  90, true ,  0, false, false},
    {CH_M_L060,  60, false,  0, false, false},
    {CH_M_R060,  60, true ,  0, false, false},
    {CH_U_L030,  30, false, 35, false, false},
    {CH_U_R030,  30, true , 35, false, false},
    {CH_U_000 ,   0, false, 35, false, false},
    {CH_U_L135, 135, false, 35, false, false},
    {CH_U_R135, 135, true , 35, false, false},
    {CH_U_180 , 180, false, 35, false, false},
    {CH_U_L090,  90, false, 35, false, false},
    {CH_U_R090,  90, true , 35, false, false},
    {CH_T_000 ,   0, false, 90, false, false},
    {CH_LFE2  ,  45, false, 15, true ,  true},
    {CH_L_L045,  45, false, 15, true , false},
    {CH_L_R045,  45, true , 15, true , false},
    {CH_L_000 ,   0, false, 15, true , false},
    {CH_U_L110, 110, false, 35, false, false},
    {CH_U_R110, 110, true , 35, false, false},
    {CH_U_L045,  45, false, 35, false, false},
    {CH_U_R045,  45, true , 35, false, false},
    {CH_M_L045,  45, false,  0, false, false},
    {CH_M_R045,  45, true ,  0, false, false},
    {CH_LFE3  ,  45, true , 15, true ,  true},
    {CH_M_LSCR,   2, false,  0, false, false},
    {CH_M_RSCR,   2, true ,  0, false, false},
    {CH_M_LSCH,   1, false,  0, false, false},
    {CH_M_RSCH,   1, true ,  0, false, false},
    {CH_M_L150, 150, false,  0, false, false},
    {CH_M_R150, 150, true ,  0, false, false},
};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Mpegh3da::File_Mpegh3da()
:File_Usac()
{
    //Configuration
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE

    //In
    MustParse_mhaC=false;
    MustParse_mpegh3daFrame=false;
    #if MEDIAINFO_CONFORMANCE
        ConformanceFlags.set(MpegH);
    #endif

    //Temp
    audioSceneInfoID=0;
    isMainStream=(int8u)-1;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mpegh3da::Streams_Fill()
{
    Stream_Prepare(Stream_Audio);
    Fill(Stream_Audio, 0, Audio_Format, "MPEG-H 3D Audio");
    string Format_Profile=Mpegh3da_Profile_Get(mpegh3daProfileLevelIndication);
    for (size_t Pos=0; Pos<mpegh3daCompatibleProfileLevelSet.size(); Pos++)
    {
        Format_Profile+=", ";
        Format_Profile+=Mpegh3da_Profile_Get(mpegh3daCompatibleProfileLevelSet[Pos]);
    }
    Fill(Stream_Audio, 0, Audio_Format_Profile, Format_Profile);
    Fill(Stream_Audio, 0, Audio_SamplingRate, usacSamplingFrequency);
    Fill(Stream_Audio, 0, Audio_SamplesPerFrame, coreSbrFrameLengthIndex_Mapping[coreSbrFrameLengthIndex].outputFrameLengthDivided256<<8);
    if (isMainStream!=(int8u)-1)
        Fill(Stream_Audio, 0, "Type", isMainStream?"Main":"Auxiliary");
    Fill_SetOptions(Stream_Audio, 0, "Type", "N NTY");
    for (set<int32u>::iterator Label=MHASPacketLabels.begin(); Label!=MHASPacketLabels.end(); ++Label)
        Fill(Stream_Audio, 0, "Label", *Label);
    Fill_SetOptions(Stream_Audio, 0, "Label", "N NTY");
    Ztring LabelString=Retrieve(Stream_Audio, 0, "Label");
    const Ztring& Type=Retrieve_Const(Stream_Audio, 0, "Type");
    if (!LabelString.empty() && !Type.empty())
    {
        if (!LabelString.empty())
            LabelString+=__T(' ');
        LabelString+=__T('(')+Type+__T(')');
    }
    if (LabelString.empty())
        Fill(Stream_Audio, 0, "Label/String", LabelString);
    Fill_SetOptions(Stream_Audio, 0, "Label/String", "Y NTN");
    if (audioSceneInfoID)
        Fill(Stream_Audio, 0, "AudioSceneInfoID", audioSceneInfoID);
    Streams_Fill_ChannelLayout(string(), referenceLayout);
    if (!GroupPresets.empty())
    {
        Fill(Stream_Audio, 0, "GroupPresetCount", GroupPresets.size());
        Fill_SetOptions(Stream_Audio, 0, "GroupPresetCount", "N NIY");
    }
    if (!SwitchGroups.empty())
    {
        Fill(Stream_Audio, 0, "SwitchGroupCount", SwitchGroups.size());
        Fill_SetOptions(Stream_Audio, 0, "SwitchGroupCount", "N NIY");
    }
    if (!Groups.empty())
    {
        Fill(Stream_Audio, 0, "GroupCount", Groups.size());
        Fill_SetOptions(Stream_Audio, 0, "GroupCount", "N NIY");
    }
    if (!SignalGroups.empty())
    {
        Fill(Stream_Audio, 0, "SignalGroupCount", SignalGroups.size());
        Fill_SetOptions(Stream_Audio, 0, "SignalGroupCount", "N NIY");
    }

    // Filling
    if (!Mpegh3da_drcInstructionsUniDrc_Data[0].empty())
    {
        C.drcInstructionsUniDrc_Data=Mpegh3da_drcInstructionsUniDrc_Data[0].begin()->second;
        Fill_DRC();
        C.drcInstructionsUniDrc_Data.clear();
    }
    /*
    if (!Mpegh3da_loudnessInfo_Data[0].empty() && !GroupPresets.empty())
    {
        int8u ID=(int8u)-1;
        for (size_t i=0; i<GroupPresets.size(); i++)
        {
            const group_preset& P=GroupPresets[i];
            if (P.ID<ID)
                ID=P.ID;
            if (Mpegh3da_loudnessInfo_Data[3][ID].Data[0].empty())
            {
                Mpegh3da_loudnessInfo_Data[3][ID].Data[0]=Mpegh3da_loudnessInfo_Data[0].begin()->second.Data[0];
                Mpegh3da_loudnessInfo_Data[0].begin()->second.Data[0].clear();
            }
        }
    }
    */
    bool NoLoudnesConch;
    if (!Mpegh3da_loudnessInfo_Data[0].empty())
    {
        C.Reset(false, true);
        C.loudnessInfo_Data[0]=Mpegh3da_loudnessInfo_Data[0].begin()->second.Data[0];
        C.loudnessInfo_Data[1]=Mpegh3da_loudnessInfo_Data[0].begin()->second.Data[1];
        Fill_Loudness(NULL);
        C.loudnessInfo_Data[0].clear();
        C.loudnessInfo_Data[1].clear();
        NoLoudnesConch=true;
    }
    else
        NoLoudnesConch=false;

    for (size_t i=0; i<GroupPresets.size(); i++)
    {
        const group_preset& P=GroupPresets[i];

        string Summary;
        if (!P.Description.empty())
            Summary+=P.Description.begin()->second;
        if (Summary.empty())
            Summary="Yes";

        string p=string("GroupPreset")+Ztring::ToZtring(i).To_UTF8();
        Fill(Stream_Audio, 0, p.c_str(), Summary);
        Fill(Stream_Audio, 0, (p+" Pos").c_str(), i);
        Fill_SetOptions(Stream_Audio, 0, (p+" Pos").c_str(), "N NIY");
        Fill(Stream_Audio, 0, (p+" ID").c_str(), P.ID);
        for (map<string, string>::const_iterator Desc=P.Description.begin(); Desc!=P.Description.end(); ++Desc)
        {
            Ztring Language=MediaInfoLib::Config.Iso639_1_Get(Ztring().From_UTF8(Desc->first));
            if (Language.empty())
                Language=Ztring().From_UTF8(Desc->first);
            Fill(Stream_Audio, 0, (p+" Title").c_str(), '('+MediaInfoLib::Config.Language_Get_Translate(__T("Language_"), Language).To_UTF8() + ") "+Desc->second);
        }
        if (P.Kind<Mpegh3da_groupPresetKind_Size)
            Fill(Stream_Audio, 0, (p+" Kind").c_str(), Mpegh3da_groupPresetKind[P.Kind]);
        if (!Mpegh3da_drcInstructionsUniDrc_Data[3].empty())
        {
            auto drcInstructionsUniDrc=Mpegh3da_drcInstructionsUniDrc_Data[3].find(P.ID);
            if (drcInstructionsUniDrc!=Mpegh3da_drcInstructionsUniDrc_Data[3].end())
            {
                C.drcInstructionsUniDrc_Data=drcInstructionsUniDrc->second;
                Fill_DRC(p.c_str());
                C.drcInstructionsUniDrc_Data.clear();
            }
        }
        if (!Mpegh3da_loudnessInfo_Data[3].empty())
        {
            std::map<int8u, loudness_info_data>::iterator Loudness=Mpegh3da_loudnessInfo_Data[3].find(P.ID);
            if (Loudness!=Mpegh3da_loudnessInfo_Data[3].end())
            {
                C.Reset(false, true);
                C.loudnessInfo_Data[0]=Loudness->second.Data[0];
                Fill_Loudness(p.c_str(), NoLoudnesConch);
                C.loudnessInfo_Data[0].clear();
            }
        }
        /* Disabled for the moment, need more details about how to show it
        ZtringList IDs;
        if (P.Conditions.size()==1 && P.Conditions[0].ReferenceID==127 && !P.Conditions[0].ConditionOnOff)
        {
            Fill(Stream_Audio, 0, (p+" LinkedTo_Group_Pos").c_str(), "Full user interactivity");
            Fill_SetOptions(Stream_Audio, 0, (p+" LinkedTo_Group_Pos").c_str(), "N NTY");
            Fill(Stream_Audio, 0, (p+" LinkedTo_Group_Pos/String").c_str(), "Full user interactivity");
            Fill_SetOptions(Stream_Audio, 0, (p+" LinkedTo_Group_Pos/String").c_str(), "Y NTN");
        }
        else
        {
            ZtringList GroupPos, GroupNum;
            for (size_t i=0; i<P.Conditions.size(); i++)
            {
                for (size_t j=0; j<Groups.size(); j++)
                    if (Groups[j].ID==P.Conditions[i].ReferenceID)
                    {
                        GroupPos.push_back(Ztring::ToZtring(j));
                        GroupNum.push_back(Ztring::ToZtring(j+1));
                    }
            }
            GroupPos.Separator_Set(0, __T(" + "));
            Fill(Stream_Audio, 0, (p+" LinkedTo_Group_Pos").c_str(), GroupPos.Read());
            Fill_SetOptions(Stream_Audio, 0, (p+" LinkedTo_Group_Pos").c_str(), "N NTY");
            GroupNum.Separator_Set(0, __T(" + "));
            Fill(Stream_Audio, 0, (p+" LinkedTo_Group_Pos/String").c_str(), GroupNum.Read());
            Fill_SetOptions(Stream_Audio, 0, (p+" LinkedTo_Group_Pos/String").c_str(), "Y NTN");
        }
        */
    }

    for (size_t i=0; i<SwitchGroups.size(); i++)
    {
        const switch_group& S=SwitchGroups[i];

        string Summary;
        if (!S.Description.empty())
            Summary+=S.Description.begin()->second;
        if (Summary.empty())
            Summary="Yes";

        string s=string("SwitchGroup")+Ztring::ToZtring(i).To_UTF8();
        Fill(Stream_Audio, 0, s.c_str(), Summary);
        Fill(Stream_Audio, 0, (s+" Pos").c_str(), i);
        Fill_SetOptions(Stream_Audio, 0, (s+" Pos").c_str(), "N NIY");
        Fill(Stream_Audio, 0, (s+" ID").c_str(), S.ID);
        for (map<string, string>::const_iterator Desc=S.Description.begin(); Desc!=S.Description.end(); ++Desc)
        {
            Ztring Language=MediaInfoLib::Config.Iso639_1_Get(Ztring().From_UTF8(Desc->first));
            if (Language.empty())
                Language=Ztring().From_UTF8(Desc->first);
            Fill(Stream_Audio, 0, (s+" Title").c_str(), '('+MediaInfoLib::Config.Language_Get_Translate(__T("Language_"), Language).To_UTF8() + ") "+Desc->second);
        }
        Fill(Stream_Audio, 0, (s+" Allow").c_str(), S.allowOnOff?"Yes":"No");
        if (S.allowOnOff)
            Fill(Stream_Audio, 0, (s+" Default").c_str(), S.defaultOnOff?"Yes":"No");
        Fill(Stream_Audio, 0, (s+" DefaultGroupID").c_str(), S.DefaultGroupID);

        ZtringList GroupPos, GroupNum;
        for (size_t i=0; i<S.MemberID.size(); i++)
        {
            for (size_t j=0; j<Groups.size(); j++)
                if (Groups[j].ID==S.MemberID[i])
                {
                    GroupPos.push_back(Ztring::ToZtring(j));
                    GroupNum.push_back(Ztring::ToZtring(j+1));
                }
        }
        GroupPos.Separator_Set(0, __T(" + "));
        Fill(Stream_Audio, 0, (s+" LinkedTo_Group_Pos").c_str(), GroupPos.Read());
        Fill_SetOptions(Stream_Audio, 0, (s+" LinkedTo_Group_Pos").c_str(), "N NTY");
        GroupNum.Separator_Set(0, __T(" + "));
        Fill(Stream_Audio, 0, (s+" LinkedTo_Group_Pos/String").c_str(), GroupNum.Read());
        Fill_SetOptions(Stream_Audio, 0, (s+" LinkedTo_Group_Pos/String").c_str(), "Y NTN");
    }

    for (size_t i=0; i<Groups.size(); i++)
    {
        const group& G=Groups[i];

        string Summary;
        if (!G.Description.empty())
            Summary+=G.Description.begin()->second;
        if (Summary.empty())
            Summary="Yes";

        string g=string("Group")+Ztring::ToZtring(i).To_UTF8();
        Fill(Stream_Audio, 0, g.c_str(), Summary);
        Fill(Stream_Audio, 0, (g+" Pos").c_str(), i);
        Fill_SetOptions(Stream_Audio, 0, (g+" Pos").c_str(), "N NIY");
        Fill(Stream_Audio, 0, (g+" ID").c_str(), G.ID);
        for (map<string, string>::const_iterator Desc=G.Description.begin(); Desc!=G.Description.end(); ++Desc)
        {
             Ztring Language=MediaInfoLib::Config.Iso639_1_Get(Ztring().From_UTF8(Desc->first));
            if (Language.empty())
                Language=Ztring().From_UTF8(Desc->first);
            Fill(Stream_Audio, 0, (g+" Title").c_str(), '('+MediaInfoLib::Config.Language_Get_Translate(__T("Language_"), Language).To_UTF8() + ") "+Desc->second);
        }
        if (!G.Language.empty())
        {
            Fill(Stream_Audio, 0, (g+" Language").c_str(), G.Language);
            Fill(Stream_Audio, 0, (g+" Language/String").c_str(), MediaInfoLib::Config.Iso639_Translate(Ztring().From_UTF8(G.Language)));
            Fill_SetOptions(Stream_Audio, 0, (g+" Language").c_str(), "N NTY");
            Fill_SetOptions(Stream_Audio, 0, (g+" Language/String").c_str(), "Y NTN");
        }
        if (G.Kind<Mpegh3da_contentKind_Size)
            Fill(Stream_Audio, 0, (g+" Kind").c_str(), Mpegh3da_contentKind[G.Kind]);
        Fill(Stream_Audio, 0, (g+" Allow").c_str(), G.allowOnOff?"Yes":"No");
        if (G.allowOnOff)
            Fill(Stream_Audio, 0, (g+" Default").c_str(), G.defaultOnOff?"Yes":"No");
        if (!Mpegh3da_drcInstructionsUniDrc_Data[1].empty())
        {
            auto drcInstructionsUniDrc=Mpegh3da_drcInstructionsUniDrc_Data[1].find(G.ID);
            if (drcInstructionsUniDrc!=Mpegh3da_drcInstructionsUniDrc_Data[1].end())
            {
                C.drcInstructionsUniDrc_Data=drcInstructionsUniDrc->second;
                Fill_DRC(g.c_str());
                C.drcInstructionsUniDrc_Data.clear();
            }
        }
        if (!Mpegh3da_loudnessInfo_Data[1].empty())
        {
            std::map<int8u, loudness_info_data>::iterator Loudness=Mpegh3da_loudnessInfo_Data[1].find(G.ID);
            if (Loudness!=Mpegh3da_loudnessInfo_Data[1].end())
            {
                C.Reset(false, true);
                C.loudnessInfo_Data[0]=Loudness->second.Data[0];
                Fill_Loudness(g.c_str(), NoLoudnesConch);
                C.loudnessInfo_Data[0].clear();
            }
        }
        if (!Mpegh3da_drcInstructionsUniDrc_Data[2].empty()) // Not sure
        {
            auto drcInstructionsUniDrc=Mpegh3da_drcInstructionsUniDrc_Data[2].find(G.ID);
            if (drcInstructionsUniDrc!=Mpegh3da_drcInstructionsUniDrc_Data[2].end())
            {
                C.drcInstructionsUniDrc_Data=drcInstructionsUniDrc->second;
                Fill_DRC(g.c_str());
                C.drcInstructionsUniDrc_Data.clear();
            }
        }
        if (!Mpegh3da_loudnessInfo_Data[2].empty()) // Not sure
        {
            std::map<int8u, loudness_info_data>::iterator Loudness=Mpegh3da_loudnessInfo_Data[2].find(G.ID);
            if (Loudness!=Mpegh3da_loudnessInfo_Data[2].end())
            {
                C.Reset(false, true);
                C.loudnessInfo_Data[0]=Loudness->second.Data[0];
                Fill_Loudness(g.c_str(), NoLoudnesConch);
                C.loudnessInfo_Data[0].clear();
            }
        }

        ZtringList GroupPos, GroupNum;
        for (size_t i=0; i<G.MemberID.size(); i++)
        {
            size_t Signal_Count=0;
            for (size_t j=0; j<SignalGroups.size(); j++)
            {
                const signal_group& E=SignalGroups[j];
                if (Signal_Count==G.MemberID[i])
                {
                    GroupPos.push_back(Ztring::ToZtring(j));
                    GroupNum.push_back(Ztring::ToZtring(j+1));
                }
                if (Aac_Channels_Get(E.Layout.ChannelLayout))
                    Signal_Count+=Aac_Channels_Get(E.Layout.ChannelLayout);
                else if (E.Layout.numSpeakers)
                    Signal_Count+=E.Layout.numSpeakers;
            }
        }
        GroupPos.Separator_Set(0, __T(" + "));
        Fill(Stream_Audio, 0, (g+" LinkedTo_SignalGroup_Pos").c_str(), GroupPos.Read());
        Fill_SetOptions(Stream_Audio, 0, (g+" LinkedTo_SignalGroup_Pos").c_str(), "N NTY");
        GroupNum.Separator_Set(0, __T(" + "));
        Fill(Stream_Audio, 0, (g+" LinkedTo_SignalGroup_Pos/String").c_str(), GroupNum.Read());
        Fill_SetOptions(Stream_Audio, 0, (g+" LinkedTo_SignalGroup_Pos/String").c_str(), "Y NTN");
    }

    for (size_t i=0; i<SignalGroups.size(); i++)
    {
        const signal_group& E=SignalGroups[i];

        string e=string("SignalGroup")+Ztring::ToZtring(i).To_UTF8();
        Fill(Stream_Audio, 0, e.c_str(), "Yes");
        Fill(Stream_Audio, 0, (e+" Pos").c_str(), i);
        Fill_SetOptions(Stream_Audio, 0, (e+" Pos").c_str(), "N NIY");
        if (E.Type<Mpegh3da_signalGroupType_Size)
            Fill(Stream_Audio, 0, (e+" Type").c_str(), Mpegh3da_signalGroupType[E.Type]);
        Streams_Fill_ChannelLayout(e+' ', E.Layout, E.Type);

        string Summary;
        switch (E.Type)
        {
            case 0 : //Channels
                    Summary+=Retrieve_Const(Stream_Audio, 0, (e+" Channel(s)/String").c_str()).To_UTF8();
                    break;
            case 1 : // Objects
                    Summary+=Retrieve_Const(Stream_Audio, 0, (e+" NumberOfObjects/String").c_str()).To_UTF8();
                    break;
            default:
                    if (E.Type<Mpegh3da_signalGroupType_Size)
                        Summary+=Mpegh3da_signalGroupType[E.Type];
                    else
                        Summary+=to_string(E.Type);
        }
        if (!Summary.empty())
            Fill(Stream_Audio, 0, e.c_str(), Summary, true, true);
    }
}

//---------------------------------------------------------------------------
void File_Mpegh3da::Streams_Finish()
{
    Streams_Finish_Conformance();
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mpegh3da::Read_Buffer_Continue()
{
    if (MustParse_mhaC)
    {
        mhaC();
        MustParse_mhaC=false;
        MustParse_mpegh3daFrame=true;
        Skip_XX(Element_Size-Element_Offset,                    "Unknown");
        return;
    }
    if (MustParse_mpegh3daFrame)
    {
        mpegh3daFrame();
    }
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mpegh3da::Header_Parse()
{
    //Parsing
    int32u MHASPacketType, MHASPacketLabel, MHASPacketLength;
    BS_Begin();
    escapedValue(MHASPacketType, 3, 8, 8,                       "MHASPacketType");
    escapedValue(MHASPacketLabel, 2, 8, 32,                     "MHASPacketLabel");
    escapedValue(MHASPacketLength, 11, 24, 24,                  "MHASPacketLength");
    BS_End();

    FILLING_BEGIN();
        if (MHASPacketLabel)
            MHASPacketLabels.insert(MHASPacketLabel);
        Header_Fill_Code(MHASPacketType, MHASPacketType<Mpegh3da_MHASPacketType_Size?Ztring().From_UTF8(Mpegh3da_MHASPacketType[MHASPacketType]):Ztring().From_CC3(MHASPacketType));
        Header_Fill_Size(Element_Offset+MHASPacketLength);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::Data_Parse()
{
    //Parsing
    switch (Element_Code)
    {
        case  1 : mpegh3daConfig(); break;
        case  2 : mpegh3daFrame(); break;
        case  3 : BS_Begin(); mae_AudioSceneInfo(); BS_End(); break;
        case  6 : Sync(); break;
        case  8 : Marker(); break;
        case  9 : Crc16(); break;
        case 14 : BufferInfo(); break;
        case 17 : audioTruncationInfo(); break;
        default : Skip_XX(Element_Size-Element_Offset,          "Data");
    }

    if (!Trusted_Get())
        Fill(Stream_Audio, 0, "NOK", "NOK", Unlimited, true, true);
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mpegh3da::mpegh3daConfig()
{
    Element_Begin1("mpegh3daConfig");
    BS_Begin();
    int8u usacSamplingFrequencyIndex;
    Get_S1(8, mpegh3daProfileLevelIndication, "mpegh3daProfileLevelIndication"); Param_Info1(Mpegh3da_Profile_Get(mpegh3daProfileLevelIndication));
    Get_S1 (5, usacSamplingFrequencyIndex,                      "usacSamplingFrequencyIndex");
    if (usacSamplingFrequencyIndex==0x1f)
        Get_S3 (24, usacSamplingFrequency,                      "usacSamplingFrequency");
    else
    {
        if (usacSamplingFrequencyIndex<Aac_sampling_frequency_Size_Usac)
            usacSamplingFrequency=Aac_sampling_frequency[usacSamplingFrequencyIndex];
        else
            usacSamplingFrequency=0;
    }
    Get_S1 (3, coreSbrFrameLengthIndex,                         "coreSbrFrameLengthIndex");
    Skip_SB(                                                    "cfg_reserved");
    Skip_SB(                                                    "receiverDelayCompensation");
    SpeakerConfig3d(referenceLayout);
    FrameworkConfig3d();
    mpegh3daDecoderConfig();
    TEST_SB_SKIP(                                               "usacConfigExtensionPresent");
        mpegh3daConfigExtension();
    TEST_SB_END();
    BS_End();
    Element_End0();

    FILLING_BEGIN();
        //Filling
        if (!Status[IsAccepted])
            Accept("MPEG-H 3D Audio");
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::SpeakerConfig3d(speaker_layout& Layout)
{
    int8u speakerLayoutType;
    Element_Begin1("SpeakerConfig3d");
    Get_S1(2, speakerLayoutType,                                "speakerLayoutType");
    if (speakerLayoutType==0)
    {
        Get_S1 (6, Layout.ChannelLayout,                        "CICPspeakerLayoutIdx"); Param_Info2(Aac_Channels_Get(Layout.ChannelLayout), " channels");
    }
    else
    {
        int32u numSpeakers;
        escapedValue(numSpeakers, 5, 8, 16,                     "numSpeakers");
        numSpeakers++;
        Layout.numSpeakers=numSpeakers;

        if (speakerLayoutType==1)
        {
            Layout.CICPspeakerIdxs.resize(numSpeakers);
            for (size_t Pos=0; Pos<numSpeakers; Pos++)
            {
                int8u CICPspeakerIdx;
                Get_S1(7, CICPspeakerIdx,                       "CICPspeakerIdx");
                Layout.CICPspeakerIdxs[Pos]=(Aac_OutputChannel)CICPspeakerIdx;
            }
        }
        else if (speakerLayoutType==2)
        {
            mpegh3daFlexibleSpeakerConfig(Layout);
        }
    }
    Element_End0();

    FILLING_BEGIN();
        //Finish
        if (Status[IsAccepted])
            Finish("MPEG-H 3D Audio");
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mpegh3daFlexibleSpeakerConfig(speaker_layout& Layout)
{
    bool angularPrecision;
    Element_Begin1("mpegh3daFlexibleSpeakerConfig");
    Get_SB(angularPrecision,                                    "angularPrecision");
    for (size_t Pos=0; Pos<Layout.numSpeakers; Pos++)
    {
        Layout.SpeakersInfo.push_back(speaker_info());
        speaker_info& SpeakerInfo=Layout.SpeakersInfo[Layout.SpeakersInfo.size()-1];
        mpegh3daSpeakerDescription(SpeakerInfo, angularPrecision);
        if (SpeakerInfo.AzimuthAngle && SpeakerInfo.AzimuthAngle!=180)
        {
            bool alsoAddSymmetricPair;
            Get_SB (alsoAddSymmetricPair,                       "alsoAddSymmetricPair");
            if (alsoAddSymmetricPair)
                Pos++;
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mpegh3daSpeakerDescription(speaker_info& SpeakerInfo, bool angularPrecision)
{
    Element_Begin1("mpegh3daSpeakerDescription");
    TESTELSE_SB_SKIP(                                           "isCICPspeakerIdx");
    {
        int8u CICPspeakerIdx;
        Get_S1 (7, CICPspeakerIdx,                              "CICPspeakerIdx");
        if (CICPspeakerIdx<Mpegh3da_SpeakerInfo_Size)
            SpeakerInfo=Mpegh3da_SpeakerInfo[CICPspeakerIdx];
        else
            SpeakerInfo.CICPspeakerIdx=(Aac_OutputChannel)CICPspeakerIdx;
    }
    TESTELSE_SB_ELSE(                                           "isCICPspeakerIdx");
        int8u ElevationClass;
        Get_S1(2, ElevationClass,                               "ElevationClass");

        switch (ElevationClass)
        {
        case 0:
            SpeakerInfo.ElevationAngle=0;
            break;
        case 1:
            SpeakerInfo.ElevationAngle=35;
            SpeakerInfo.ElevationDirection=false;
            break;
        case 2:
            SpeakerInfo.ElevationAngle=15;
            SpeakerInfo.ElevationDirection=true;
            break;
        case 3:
            int8u ElevationAngleIdx;
            Get_S1(angularPrecision?7:5, ElevationAngleIdx, "ElevationAngleIdx");
            SpeakerInfo.ElevationAngle=ElevationAngleIdx*(angularPrecision?1:5);

            if (SpeakerInfo.ElevationAngle)
                Get_SB(SpeakerInfo.ElevationDirection,         "ElevationDirection");
            break;
        }

        int8u AzimuthAngleIdx;
        Get_S1(angularPrecision?8:6, AzimuthAngleIdx, "AzimuthAngleIdx");
        SpeakerInfo.AzimuthAngle=AzimuthAngleIdx*(angularPrecision?1:5);

        if (SpeakerInfo.AzimuthAngle && SpeakerInfo.AzimuthAngle!=180)
            Get_SB(SpeakerInfo.AzimuthDirection, "AzimuthDirection");

        Get_SB(SpeakerInfo.isLFE,                "isLFE");

        SpeakerInfo.CICPspeakerIdx=(Aac_OutputChannel)-1;
    TESTELSE_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mpegh3daFrame()
{
    Skip_XX(Element_Size,                                       "mpegh3daFrame");

    FILLING_BEGIN();
        //Filling
        if (Status[IsAccepted])
            Finish("MPEG-H 3D Audio");
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::Sync()
{
    //Parsing
    Skip_B1(                                                    "syncword");
}

//---------------------------------------------------------------------------
void File_Mpegh3da::Marker()
{
    //Parsing
    Info_B1(marker_byte,                                        "marker_byte"); Param_Info1C(marker_byte<Mpegh3da_marker_byte_Size, Mpegh3da_marker_byte[marker_byte]);
}

//---------------------------------------------------------------------------
void File_Mpegh3da::Crc16()
{
    //Parsing
    Skip_B2(                                                    "mhasParity16Data");
}

//---------------------------------------------------------------------------
void File_Mpegh3da::BufferInfo()
{
    //Parsing
    BS_Begin();
    bool mhas_buffer_fullness_present;
    Get_SB (mhas_buffer_fullness_present,                       "mhas_buffer_fullness_present");
    if (mhas_buffer_fullness_present)
    {
        int32u mhas_buffer_fullness;
        escapedValue(mhas_buffer_fullness, 15, 39, 71,          "mhas_buffer_fullness");
    }
    BS_End();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::FrameworkConfig3d()
{
    numAudioChannels=0;
    numAudioObjects=0;
    numSAOCTransportChannels=0;
    numHOATransportChannels=0;

    Element_Begin1("FrameworkConfig3d");
    Element_Begin1("Signals3d");
    Get_S1(5, bsNumSignalGroups,                                "bsNumSignalGroups");
    bsNumSignalGroups++; Param_Info2(bsNumSignalGroups, " signals");
    SignalGroups.resize(bsNumSignalGroups);
    for (int8u Pos=0; Pos<bsNumSignalGroups; Pos++)
    {
        signal_group& E=SignalGroups[Pos];
        Element_Begin1("signalGroup");
        Get_S1(3, E.Type,                                       "signalGroupType");
        escapedValue(E.bsNumberOfSignals, 5, 8, 16,             "bsNumberOfSignals");
        E.bsNumberOfSignals++;

        if (E.Type==SignalGroupTypeChannels)
        {
            numAudioChannels+=E.bsNumberOfSignals;
            TESTELSE_SB_SKIP(                                   "differsFromReferenceLayout");
                SpeakerConfig3d(E.Layout);
            TESTELSE_SB_ELSE(                                   "differsFromReferenceLayout");
                E.Layout=referenceLayout;
            TESTELSE_SB_END();
        }
        else if (E.Type==SignalGroupTypeObject)
        {
            numAudioObjects+=E.bsNumberOfSignals;
            E.Layout.numSpeakers=E.bsNumberOfSignals;
        }
        else if (E.Type==SignalGroupTypeSAOC)
        {
            numSAOCTransportChannels+=E.bsNumberOfSignals;
            TEST_SB_SKIP(                                       "saocDmxLayoutPresent");
                SpeakerConfig3d(E.Layout);
            TEST_SB_END();
        }
        else if (E.Type==SignalGroupTypeHOA)
        {
            numHOATransportChannels+=E.bsNumberOfSignals;
            E.Layout.numSpeakers=E.bsNumberOfSignals;
        }
        Element_End0();
    }
    Element_End0();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mpegh3daDecoderConfig()
{
    Elements.clear();

    Element_Begin1("mpegh3daDecoderConfig");
    escapedValue(numElements, 4, 8, 16,                         "numElements");
    numElements++;

    bool elementLengthPresent;
    Get_SB (elementLengthPresent,                               "elementLengthPresent");

    for (size_t Pos=0; Pos<numElements; Pos++)
    {
        Element_Begin1("Element");
        int8u usacElementType;
        Get_S1(2, usacElementType,                              "usacElementType"); Element_Info1(Mpegh3da_usacElementType[usacElementType]);
        switch (usacElementType)
        {
        case ID_USAC_SCE:
            mpegh3daSingleChannelElementConfig(coreSbrFrameLengthIndex_Mapping[coreSbrFrameLengthIndex].sbrRatioIndex);
            Elements.push_back(usac_element(ID_USAC_SCE));
            break;
        case ID_USAC_CPE:
            mpegh3daChannelPairElementConfig(coreSbrFrameLengthIndex_Mapping[coreSbrFrameLengthIndex].sbrRatioIndex);
            Elements.push_back(usac_element(ID_USAC_CPE));
            break;
        case ID_USAC_LFE:
            Elements.push_back(usac_element(ID_USAC_LFE));
            break;
        case ID_USAC_EXT:
            mpegh3daExtElementConfig();
            Elements.push_back(usac_element(ID_USAC_EXT));
            break;
        }
        Element_End0();
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mpegh3daSingleChannelElementConfig(int8u sbrRatioIndex)
{
    Element_Begin1("mpegh3daSingleChannelElementConfig");
    mpegh3daCoreConfig();
    if (sbrRatioIndex)
        SbrConfig();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mpegh3daChannelPairElementConfig(int8u sbrRatioIndex)
{
    int32u nBits=floor(log2(numAudioChannels+numAudioObjects+numHOATransportChannels+numSAOCTransportChannels-1))+1;
    int8u stereoConfigIndex=0;
    int8u qceIndex;
    Element_Begin1("mpegh3daChannelPairElementConfig");
    bool enhancedNoiseFilling=mpegh3daCoreConfig();
    if(enhancedNoiseFilling)
        Skip_SB(                                                "igfIndependentTiling");

    if (sbrRatioIndex)
    {
        SbrConfig();
        Get_S1(2, stereoConfigIndex,                            "stereoConfigIndex");
    }

    if (stereoConfigIndex) {
        Mps212Config(stereoConfigIndex);
    }

    Get_S1(2, qceIndex,                                         "qceIndex");
    if (qceIndex)
    {
        TEST_SB_SKIP(                                           "shiftIndex0");
            Skip_BS(nBits,                                      "shiftChannel0");
        TEST_SB_END();
    }

    TEST_SB_SKIP(                                               "shiftIndex1");
        Skip_BS(nBits,                                          "shiftChannel1");
    TEST_SB_END();

    if (sbrRatioIndex==0 && qceIndex==0)
        Skip_SB(                                                "lpdStereoIndex");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mpegh3daExtElementConfig()
{
    Element_Begin1("mpegh3daExtElementConfig");
    int32u usacExtElementType;
    escapedValue(usacExtElementType, 4, 8, 16,                  "usacExtElementType"); Element_Level--; Element_Info1C(usacExtElementType<Mpegh3da_usacExtElementType_Size, Mpegh3da_usacExtElementType[usacExtElementType]); Element_Level++;

    int32u usacExtElementConfigLength;
    escapedValue(usacExtElementConfigLength, 4, 8, 16,          "usacExtElementConfigLength");

    int32u usacExtElementDefaultLength=0;
    TEST_SB_SKIP(                                               "usacExtElementDefaultLengthPresent");
        escapedValue(usacExtElementDefaultLength, 8, 16, 0,     "usacExtElementDefaultLength"); //TODO: check if call is valid
        usacExtElementDefaultLength++;
    TEST_SB_END();

    Skip_SB(                                                    "usacExtElementPayloadFrag");

    size_t Remain_Before=BS->Remain();
    switch (usacExtElementType)
    {
    case ID_EXT_ELE_FILL:
        break; // No configuration element
    //case ID_EXT_ELE_MPEGS:
        //TODO: SpatialSpecificConfig();
        //break;
    //case ID_EXT_ELE_SAOC:
        //TODO: SAOCSpecificConfig();
        //break;
    case ID_EXT_ELE_AUDIOPREROLL:
        break; // No configuration element
    case ID_EXT_ELE_UNI_DRC:
        //if (referenceLayout.ChannelLayout!=19) //TEMP
            mpegh3daUniDrcConfig();
        break;
    case ID_EXT_ELE_OBJ_METADATA:
        ObjectMetadataConfig();
        break;
    //case ID_EXT_ELE_SAOC_3D:
    //    SAOC3DSpecificConfig();
    //    break;
    //case ID_EXT_ELE_HOA:
        //HOAConfig();
        //break;
    case ID_EXT_ELE_FMT_CNVRTR:
        break; // No configuration element
    //case ID_EXT_ELE_MCT:
        //MCTConfig();
        //break;
    case ID_EXT_ELE_TCC:
        TccConfig();
        break;
    //case ID_EXT_ELE_HOA_ENH_LAYER:
        //HOAEnhConfig();
        //break;
    //case ID_EXT_ELE_HREP:
        //HREPConfig(current_signal_group);
        //break;
    //case ID_EXT_ELE_ENHANCED_OBJ_METADATA:
        //EnhancedObjectMetadataConfig();
        //break;
        break;
    default:
        if (usacExtElementConfigLength)
            Skip_BS(usacExtElementConfigLength*8,               "reserved");
        break;
    }
    if (BS->Remain()+usacExtElementConfigLength*8>Remain_Before)
    {
        size_t Size=BS->Remain()+usacExtElementConfigLength*8-Remain_Before;
        int8u Padding=1;
        if (Size<8)
            Peek_S1((int8u)Size, Padding);

        if (Padding && Remain_Before!=BS->Remain() && usacExtElementType!=ID_EXT_ELE_OBJ_METADATA)
            Fill(Stream_Audio, 0, "NOK", "NOK", Unlimited, true, true);
        Skip_BS(Size, Padding?"(Unknown)":"Padding");
    }

    Element_End0();
}

//---------------------------------------------------------------------------
bool File_Mpegh3da::mpegh3daCoreConfig()
{
    bool enhancedNoiseFilling;
    Element_Begin1("mpegh3daCoreConfig");
    Skip_SB(                                                    "tw_mdct");
    Skip_SB(                                                    "fullbandLpd");
    Skip_SB(                                                    "noiseFilling");
    TEST_SB_GET(enhancedNoiseFilling,                           "enhancedNoiseFilling");
        Skip_SB(                                                "igfUseEnf");
        Skip_SB(                                                "igfUseHighRes");
        Skip_SB(                                                "igfUseWhitening");
        Skip_SB(                                                "igfAfterTnsSynth");
        Skip_S1(5,                                              "igfStartIndex");
        Skip_S1(4,                                              "igfStopIndex");
    TEST_SB_END();
    Element_End0();

    return enhancedNoiseFilling;
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mpegh3daUniDrcConfig()
{
    Element_Begin1("mpegh3daUniDrcConfig");
    int8u drcCoefficientsUniDrcCount;
    Get_S1(3, drcCoefficientsUniDrcCount,                       "drcCoefficientsUniDrcCount");

    int8u drcInstructionsUniDrcCount;
    Get_S1(6, drcInstructionsUniDrcCount,                       "drcInstructionsUniDrcCount");

    Element_Begin1("mpegh3daUniDrcChannelLayout");
    Get_S1 (7, C.baseChannelCount,                              "baseChannelCount");
    Element_End0();
    if (!drcCoefficientsUniDrcCount)
        Fill(Stream_Audio, 0, "TEMP_drcCoefficientsUniDrcCount", drcCoefficientsUniDrcCount); //TEMP

    for (int8u Pos=0; Pos<drcCoefficientsUniDrcCount; Pos++)
        drcCoefficientsUniDrc(); // in File_USAC.cpp

    for (int8u Pos=0; Pos<drcInstructionsUniDrcCount; Pos++)
    {
        int8u drcInstructionsType;
        Get_S1(Peek_SB()?2:1, drcInstructionsType,              "drcInstructionsType");

        int8u ID;
        if (drcInstructionsType==2)
            Get_S1 (7, ID,                                      "mae_groupID");
        else if (drcInstructionsType==3)
            Get_S1 (5, ID,                                      "mae_groupPresetID");
        else
            ID=0;

        drcInstructionsUniDrc(false, true); // in File_USAC.cpp
        Mpegh3da_drcInstructionsUniDrc_Data[drcInstructionsType][ID][C.drcInstructionsUniDrc_Data.begin()->first]=C.drcInstructionsUniDrc_Data.begin()->second;
        C.drcInstructionsUniDrc_Data.clear();
    }

    TEST_SB_SKIP(                                               "uniDrcConfigExtPresent");
        uniDrcConfigExtension(); // in File_USAC.cpp
    TEST_SB_END();

    TEST_SB_SKIP(                                               "loudnessInfoSetPresent");
        mpegh3daLoudnessInfoSet();
    TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::downmixConfig()
{
    Element_Begin1("downmixConfig");
    int8u downmixConfigType;
    Get_S1 (2, downmixConfigType,                                "downmixConfigType");
    switch (downmixConfigType)
    {
        case 0:
        case 2:
        {
            bool passiveDownmixFlag;
            Get_SB (passiveDownmixFlag,                         "passiveDownmixFlag");
            if (!passiveDownmixFlag)
                Skip_S1(3,                                      "phaseAlignStrength");
            Skip_SB(                                            "immersiveDownmixFlag");
        }
        break;
    }
    switch (downmixConfigType)
    {
        case 1:
        case 2:
        {
            Skip_S1(5,                                           "DownmixMatrixSet - TODO");
        }
        break;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mpegh3daLoudnessInfoSet()
{
    Element_Begin1("mpegh3daLoudnessInfoSet");
    int8u loudnessInfoCount;
    Get_S1(6, loudnessInfoCount,                                "loudnessInfoCount");
    for(int8u Pos=0; Pos<loudnessInfoCount; Pos++)
    {
        int8u loudnessInfoType;
        Get_S1(2, loudnessInfoType,                             "loudnessInfoType");

        int8u ID;
        if (loudnessInfoType==1 || loudnessInfoType==2)
            Get_S1 (7, ID,                                      "mae_groupID");
        else if (loudnessInfoType==3)
            Get_S1 (5, ID,                                      "mae_groupPresetID");
        else
            ID=0;

        bool IsNOK=loudnessInfo(false); // in File_USAC.cpp
        Mpegh3da_loudnessInfo_Data[loudnessInfoType][ID].Data[0][C.loudnessInfo_Data[0].begin()->first]=C.loudnessInfo_Data[0].begin()->second;
        C.loudnessInfo_Data[0].clear();
        if (IsNOK)
        {
            Element_End0();
            return;
        }
    }

    TEST_SB_SKIP(                                               "loudnessInfoAlbumPresent");
        int8u loudnessInfoAlbumCount;
        Get_S1(6, loudnessInfoAlbumCount,                       "loudnessInfoAlbumCount");
        for (int8u Pos=0; Pos<loudnessInfoAlbumCount; Pos++)
        {
            loudnessInfo(true); // in File_USAC.cpp
            Mpegh3da_loudnessInfo_Data[0][0].Data[1][C.loudnessInfo_Data[1].begin()->first]=C.loudnessInfo_Data[1].begin()->second;
            C.loudnessInfo_Data[1].clear();
        }
    TEST_SB_END();

    TEST_SB_SKIP(                                               "loudnessInfoSetExtensionPresent");
        loudnessInfoSetExtension(); // in File_USAC.cpp
    TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::ObjectMetadataConfig()
{
    Element_Begin1("ObjectMetadataConfig");
    Skip_SB(                                                    "lowDelayMetadataCoding");
    TESTELSE_SB_SKIP(                                           "hasCoreLength");
    TESTELSE_SB_ELSE(                                           "hasCoreLength");
        Skip_S1(6,                                              "frameLength");
    TESTELSE_SB_END();

    TEST_SB_SKIP("hasScreenRelativeObjects");
        size_t num_objects=num_objects_Get();
        for (int16u Pos=0; Pos<num_objects; Pos++)
        {
            Skip_SB(                                            "isScreenRelativeObject");
        }
    TEST_SB_END();
    Skip_SB(                                                    "hasDynamicObjectPriority");
    Skip_SB(                                                    "hasUniformSpread");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::SAOC3DSpecificConfig()
{
    int8u bsSamplingFrequencyIndex, bsNumSaocDmxChannels, bsNumSaocDmxObjects, bsNumSaocObjects;
    int32u NumSaocChannels=0, NumInputSignals=0;
    Element_Begin1("SAOC3DSpecificConfig");
    Get_S1(4, bsSamplingFrequencyIndex,                         "bsSamplingFrequencyIndex");
    if (bsSamplingFrequencyIndex==15)
        Skip_S3(24,                                             "bsSamplingFrequency");

    Skip_S1(3,                                                  "bsFreqRes");
    Skip_SB(                                                    "bsDoubleFrameLengthFlag");
    Get_S1(5, bsNumSaocDmxChannels,                             "bsNumSaocDmxChannels");
    Get_S1(5, bsNumSaocDmxObjects,                              "bsNumSaocDmxObjects");
    Skip_SB(                                                    "bsDecorrelationMethod");

    if (bsNumSaocDmxChannels)
    {
        speaker_layout saocChannelLayout;
        SpeakerConfig3d(saocChannelLayout);
        NumSaocChannels=SAOC3DgetNumChannels(saocChannelLayout);
        NumInputSignals+=NumSaocChannels;
    }

    Get_S1(8, bsNumSaocObjects ,                                "bsNumSaocObjects");
    NumInputSignals+=bsNumSaocObjects;

    for (int8u Pos=0; Pos<NumSaocChannels; Pos++)
    {
        for(int8u Pos2=Pos+1; Pos2<NumSaocChannels; Pos2++)
            Skip_SB(                                            "bsRelatedTo");
    }

    for (int8u Pos=NumSaocChannels; Pos<NumInputSignals; Pos++)
    {
        for(int8u Pos2=Pos+1; Pos2<NumInputSignals; Pos2++)
            Skip_SB(                                            "bsRelatedTo");
    }

    Skip_SB(                                                    "bsOneIOC");
    TEST_SB_SKIP(                                               "bsSaocDmxMethod");
        SAOC3DgetNumChannels(referenceLayout);
    TEST_SB_END();

    TEST_SB_SKIP(                                               "bsDualMode");
        Skip_S1(5,                                              "bsBandsLow");
    TEST_SB_END();

    TEST_SB_SKIP(                                               "bsDcuFlag");
        Skip_SB(                                                "bsDcuMandatory");
        TEST_SB_SKIP(                                           "bsDcuDynamic");
            Skip_SB(                                            "bsDcuMode");
            Skip_S1(4,                                          "bsDcuParam");
        TEST_SB_END();
    TEST_SB_END();
    Skip_S1(BS->Remain()%8,                                     "byte_align");
    //TODO: SAOC3DExtensionConfig();
    Element_End0();
}

//---------------------------------------------------------------------------
int32u File_Mpegh3da::SAOC3DgetNumChannels(speaker_layout Layout)
{
    int32u ToReturn=Layout.numSpeakers;

    for (int32u Pos=0; Pos<Layout.numSpeakers; Pos++)
    {
        if (Layout.SpeakersInfo.size()>Pos && Layout.SpeakersInfo[Pos].isLFE)
            ToReturn--;
    }

    return ToReturn;
}

//---------------------------------------------------------------------------
void File_Mpegh3da::MCTConfig()
{
    Element_Begin1("MCTConfig");
    for(int32u chan=0; chan<numAudioChannels; ++chan) // bsNumberOfSignals[grp] but which grp?
    {
        Skip_SB(                                                "mctChanMask");
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::TccConfig()
{
    Element_Begin1("TccConfig");
    for(int32u Pos=0; Pos<numElements; Pos++)
    {
        if(Elements.size()>Pos && (Elements[Pos].Type==ID_USAC_SCE || Elements[Pos].Type==ID_USAC_CPE))
            Skip_S1(2,                                       "tccMode");
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::EnhancedObjectMetadataConfig()
{
    Element_Begin1("EnhancedObjectMetadataConfig");
    bool hasCommonGroupExcludedSectors=false;
    TEST_SB_SKIP(                                               "hasDiffuseness");
        Skip_SB(                                                "hasCommonGroupDiffuseness");
    TEST_SB_END();

    TEST_SB_SKIP(                                               "hasExcludedSectors");
        TEST_SB_GET(hasCommonGroupExcludedSectors,              "hasCommonGroupExcludedSectors");
            Skip_SB(                                            "useOnlyPredefinedSectors");
        TEST_SB_END();
    TEST_SB_END();

    TEST_SB_SKIP(                                               "hasClosestSpeakerCondition");
        Skip_S1(7,                                              "closestSpeakerThresholdAngle");
    TEST_SB_END();

    size_t num_objects=num_objects_Get();
    for (int8u Pos=0; Pos<num_objects; Pos++)
    {
        TEST_SB_SKIP(                                           "hasDivergence");
            Skip_S1(6,                                          "divergenceAzimuthRange");
        TEST_SB_END();

        if (!hasCommonGroupExcludedSectors)
            Skip_SB(                                            "useOnlyPredefinedSectors");
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mpegh3daConfigExtension()
{
    Element_Begin1("mpegh3daConfigExtension");
    int32u numConfigExtensions;
    escapedValue(numConfigExtensions, 2, 4, 8,                  "numConfigExtensions");
    numConfigExtensions++;

    for (int32u Pos=0; Pos<numConfigExtensions; Pos++)
    {
        Element_Begin1("configExtension");
        int32u usacConfigExtType;
        escapedValue(usacConfigExtType, 4, 8, 16,               "usacConfigExtType"); Element_Info1C(usacConfigExtType<Mpegh3da_usacConfigExtType_Size, Mpegh3da_usacConfigExtType[usacConfigExtType]);

        int32u usacConfigExtLength;
        escapedValue(usacConfigExtLength, 4, 8, 16,             "usacConfigExtLength");
        if (!usacConfigExtLength)
        {
            Element_End0();
            continue;
        }

        size_t Remain_Before=BS->Remain();
        switch ((UsacConfigExtType)usacConfigExtType)
        {
        case ID_CONFIG_EXT_FILL:
            while (usacConfigExtLength)
            {
                usacConfigExtLength--;
                Skip_S1(8,                                      "fill_byte"); // should be '10100101'
            }
            break;
        case ID_CONFIG_EXT_DOWNMIX:
            downmixConfig();
            break;
        case ID_CONFIG_EXT_LOUDNESS_INFO:
            mpegh3daLoudnessInfoSet();
            break;
        case ID_CONFIG_EXT_AUDIOSCENE_INFO:
            mae_AudioSceneInfo();
            break;
        //case ID_CONFIG_EXT_HOA_MATRIX:
        //    HoaRenderingMatrixSet();
        //    break;
        case ID_CONFIG_EXT_ICG:
            ICGConfig();
            break;
        case ID_CONFIG_EXT_SIG_GROUP_INFO:
            SignalGroupInformation();
            break;
        case ID_CONFIG_EXT_COMPATIBLE_PROFILE_LEVEL_SET:
            CompatibleProfileLevelSet();
            break;
        default:
            Skip_BS(usacConfigExtLength*8,                      "reserved");
        }
        if (BS->Remain()+usacConfigExtLength*8>Remain_Before)
        {
            size_t Size=BS->Remain()+usacConfigExtLength*8-Remain_Before;
            int8u Padding=1;
            if (Size<8)
                Peek_S1((int8u)Size, Padding);

            if (Padding && Remain_Before!=BS->Remain() && usacConfigExtType!=ID_CONFIG_EXT_DOWNMIX && usacConfigExtType!=ID_CONFIG_EXT_HOA_MATRIX)
                Fill(Stream_Audio, 0, "NOK", "NOK", Unlimited, true, true);
            Skip_BS(Size, Padding?"(Unknown)":"Padding");
        }
        Element_End0();
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::SignalGroupInformation()
{
    Element_Begin1("SignalGroupInformation");
    for (int8u Pos=0; Pos<bsNumSignalGroups+1; Pos++)
    {
        Skip_S1(3,                                              "groupPriority");
        Skip_SB(                                                "fixedPosition");
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::CompatibleProfileLevelSet()
{
    Element_Begin1("CompatibleProfileLevelSet");
    int8u bsNumCompatibleSets;
    Get_S1 (4, bsNumCompatibleSets,                             "bsNumCompatibleSets");
    Skip_S1(4,                                                  "reserved");
    mpegh3daCompatibleProfileLevelSet.resize(bsNumCompatibleSets+1);
    for (int8u Pos=0; Pos<=bsNumCompatibleSets; Pos++)
    {
        Get_S1(8, mpegh3daCompatibleProfileLevelSet[Pos], "CompatibleSetIndication"); Param_Info1(Mpegh3da_Profile_Get(mpegh3daCompatibleProfileLevelSet[Pos]));
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::HoaRenderingMatrixSet()
{
    Element_Begin1("HoaRenderingMatrixSet - TODO");
    Skip_S1(5,                                                  "numOfHoaRenderingMatrices");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::ICGConfig()
{
    Element_Begin1("ICGConfig");
    TEST_SB_SKIP(                                               "ICPresent");
        for (int32u Pos=0; Pos<numElements; Pos++)
        {
            if (Elements.size()>Pos && Elements[Pos].Type==ID_USAC_CPE)
                Skip_SB(                                        "ICinCPE");
        }

        TEST_SB_SKIP(                                           "ICGPreAppliedPresent");
            for (int32u Pos=0; Pos<numElements; Pos++)
            {
                if (Elements.size()>Pos && Elements[Pos].Type==ID_USAC_CPE)
                    Skip_SB(                                     "ICGPreAppliedCPE");
            }
        TEST_SB_END();
    TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mae_AudioSceneInfo()
{
    Groups.clear();
    SwitchGroups.clear();
    GroupPresets.clear();

    Element_Begin1("mae_AudioSceneInfo");
    bool mae_isMainStream;
    TESTELSE_SB_GET (mae_isMainStream,                          "mae_isMainStream");
        TEST_SB_SKIP(                                           "mae_audioSceneInfoIDPresent");
            Get_S1 (8, audioSceneInfoID,                        "mae_audioSceneInfoID");
        TEST_SB_END();
        int8u mae_numGroups;
        Get_S1(7, mae_numGroups,                                "mae_numGroups");
        mae_GroupDefinition(mae_numGroups);
        int8u mae_numSwitchGroups;
        Get_S1(5, mae_numSwitchGroups,                          "mae_numSwitchGroups");
        mae_SwitchGroupDefinition(mae_numSwitchGroups);
        int8u mae_numGroupPresets;
        Get_S1(5, mae_numGroupPresets,                          "mae_numGroupPresets");
        mae_GroupPresetDefinition(mae_numGroupPresets);
        mae_Data(mae_numGroups, mae_numGroupPresets);
        Skip_S1(7,                                              "mae_metaDataElementIDmaxAvail");
    TESTELSE_SB_ELSE(                                           "mae_isMainStream");
        Skip_S1(7,                                              "mae_bsMetaDataElementIDoffset");
        Skip_S1(7,                                              "mae_metaDataElementIDmaxAvail");
    TESTELSE_SB_END();
    Element_End0();

    isMainStream=mae_isMainStream;
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mae_GroupDefinition(int8u numGroups)
{
    Element_Begin1("mae_GroupDefinition");
    Groups.resize(numGroups);
    for (int8u Pos=0; Pos<numGroups; Pos++)
    {
        Element_Begin1("mae_group");
        group& G=Groups[Pos];
        Get_S1 (7, G.ID,                                        "mae_groupID");
        Element_Info1(Ztring::ToZtring(G.ID));
        Get_SB (G.allowOnOff,                                   "mae_allowOnOff");
        Get_SB (G.defaultOnOff,                                 "mae_defaultOnOff");

        TEST_SB_SKIP(                                           "mae_allowPositionInteractivity");
            Skip_S1(7,                                          "mae_interactivityMinAzOffset");
            Skip_S1(7,                                          "mae_interactivityMaxAzOffset");
            Skip_S1(5,                                          "mae_interactivityMinElOffset");
            Skip_S1(5,                                          "mae_interactivityMaxElOffset");
            Skip_S1(4,                                          "mae_interactivityMinDistFactor");
            Skip_S1(4,                                          "mae_interactivityMaxDistFactor");
        TEST_SB_END();

        TEST_SB_SKIP(                                           "mae_allowGainInteractivity");
            Skip_S1(6,                                          "mae_interactivityMinGain");
            Skip_S1(5,                                          "mae_interactivityMaxGain");
        TEST_SB_END();

        int8u mae_bsGroupNumMembers;
        Get_S1(7, mae_bsGroupNumMembers,                        "mae_bsGroupNumMembers");
        mae_bsGroupNumMembers++;
        G.MemberID.resize(mae_bsGroupNumMembers);
        TESTELSE_SB_SKIP(                                       "mae_hasConjunctMembers");
            int8u mae_startID;
            Get_S1 (7, mae_startID,                             "mae_startID");
            for (int8u Pos2=0; Pos2<mae_bsGroupNumMembers; Pos2++)
                G.MemberID[Pos2]=mae_startID++;
        TESTELSE_SB_ELSE(                                       "mae_hasConjunctMembers");
            for (int8u Pos2=0; Pos2<mae_bsGroupNumMembers; Pos2++)
                Get_S1 (7, G.MemberID[Pos2],                    "mae_metaDataElementID");
        TESTELSE_SB_END();
        Element_End0();
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mae_SwitchGroupDefinition(int8u numSwitchGroups)
{
    Element_Begin1("mae_SwitchGroupDefinition");
    SwitchGroups.resize(numSwitchGroups);
    for(int8u Pos=0; Pos<numSwitchGroups; Pos++)
    {
        Element_Begin1("mae_switchGroup");
        switch_group& S=SwitchGroups[Pos];
        Get_S1 (5, S.ID,                                        "mae_switchGroupID");
        Element_Info1(Ztring::ToZtring(S.ID));

        TESTELSE_SB_GET(S.allowOnOff,                           "mae_switchGroupAllowOnOff");
            Get_SB (S.defaultOnOff,                             "mae_switchGroupDefaultOnOff");
        TESTELSE_SB_ELSE(                                       "mae_switchGroupAllowOnOff");
            S.defaultOnOff=false;
        TESTELSE_SB_END();

        int8u mae_bsSwitchGroupNumMembers;
        Get_S1(5, mae_bsSwitchGroupNumMembers,                  "mae_bsSwitchGroupNumMembers");
        mae_bsSwitchGroupNumMembers++;
        S.MemberID.resize(mae_bsSwitchGroupNumMembers);
        for (int8u Pos2=0; Pos2<mae_bsSwitchGroupNumMembers; Pos2++)
            Get_S1 (7, S.MemberID[Pos2],                        "mae_switchGroupMemberID");

        Get_S1 (7, S.DefaultGroupID,                            "mae_switchGroupDefaultGroupID");
        Element_End0();
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mae_GroupPresetDefinition(int8u numGroupPresets)
{
    Element_Begin1("mae_GroupPresetDefinition");
    GroupPresets.resize(numGroupPresets);
    for (int8u Pos=0; Pos<numGroupPresets; Pos++)
    {
        Element_Begin1("mae_groupPreset");
        group_preset& P=GroupPresets[Pos];
        Get_S1 (5, P.ID,                                        "mae_groupPresetID");
        Element_Info1(Ztring::ToZtring(P.ID));
        Get_S1 (5, P.Kind,                                      "mae_groupPresetKind");

        int8u mae_bsGroupPresetNumConditions;
        Get_S1 (4, mae_bsGroupPresetNumConditions,              "mae_bsGroupPresetNumConditions");
        mae_bsGroupPresetNumConditions++;
        P.Conditions.resize(mae_bsGroupPresetNumConditions);
        for (int8u Pos2=0; Pos2<mae_bsGroupPresetNumConditions; Pos2++)
        {
            Element_Begin1("mae_groupPreset");
            Get_S1 (7, P.Conditions[Pos2].ReferenceID,          "mae_groupPresetReferenceID");
            Element_Info1(P.Conditions[Pos2].ReferenceID);
            TEST_SB_GET (P.Conditions[Pos2].ConditionOnOff,     "mae_groupPresetConditionOnOff");
                Skip_SB(                                        "mae_groupPresetDisableGainInteractivity");
                TEST_SB_SKIP(                                   "mae_groupPresetGainFlag");
                    Skip_S1(8,                                  "mae_groupPresetGain");
                TEST_SB_END();
                Skip_SB(                                        "mae_groupPresetDisablePositionInteractivity");
                TEST_SB_SKIP(                                   "mae_groupPresetPositionFlag");
                    Skip_S1(8,                                  "mae_groupPresetAzOffset");
                    Skip_S1(6,                                  "mae_groupPresetElOffset");
                    Skip_S1(4,                                  "mae_groupPresetDistFactor");
                TEST_SB_END();
            TEST_SB_END();
            Element_End0();
        }
        Element_End0();
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mae_Data(int8u numGroups, int8u numGroupPresets)
{
    Element_Begin1("mae_Data");
    int8u mae_numDataSets;
    Get_S1(4, mae_numDataSets,                                  "mae_numDataSets");
    for (int8u Pos=0; Pos<mae_numDataSets; Pos++)
    {
        Element_Begin1("mae_data");
        int8u mae_dataType;
        Get_S1(4, mae_dataType,                                 "mae_dataType");
        int16u mae_dataLength;
        Get_S2(16, mae_dataLength,                              "mae_dataLength");

        size_t Remain_Before=BS->Remain();
        switch ((MaeDataType)mae_dataType)
        {
        case ID_MAE_GROUP_DESCRIPTION:
        case ID_MAE_SWITCHGROUP_DESCRIPTION:
        case ID_MAE_GROUP_PRESET_DESCRIPTION:
            mae_Description((MaeDataType)mae_dataType);
            break;
        case ID_MAE_GROUP_CONTENT:
            mae_ContentData();
            break;
        case ID_MAE_GROUP_COMPOSITE:
            mae_CompositePair();
            break;
        case ID_MAE_SCREEN_SIZE:
            mae_ProductionScreenSizeData();
            break;
        case ID_MAE_DRC_UI_INFO:
            mae_DrcUserInterfaceInfo(mae_dataLength);
            break;
        case ID_MAE_SCREEN_SIZE_EXTENSION:
            mae_ProductionScreenSizeDataExtension();
            break;
        case ID_MAE_GROUP_PRESET_EXTENSION:
            mae_GroupPresetDefinitionExtension(numGroupPresets);
            break;
        case ID_MAE_LOUDNESS_COMPENSATION:
            mae_LoudnessCompensationData(numGroups, numGroupPresets);
            break;
        default:
            Skip_BS(mae_dataLength*8,                           "reserved");
        }
        if (BS->Remain()+mae_dataLength*8>Remain_Before)
        {
            size_t Size=BS->Remain()+mae_dataLength*8-Remain_Before;
            int8u Padding=1;
            if (Size<8)
                Peek_S1((int8u)Size, Padding);

            if (Padding)
                Fill(Stream_Audio, 0, "NOK", "NOK", Unlimited, true, true);
            Skip_BS(Size, Padding?"(Unknown)":"Padding");
        }
        Element_End0();
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mae_Description(MaeDataType type)
{
    Element_Info1("mae_Description");
    Element_Begin1("mae_Description");
    int8u mae_bsNumDescriptionBlocks;
    Get_S1(7, mae_bsNumDescriptionBlocks,                       "mae_bsNumDescriptionBlocks");
    mae_bsNumDescriptionBlocks++;
    for (int8u Pos=0; Pos<mae_bsNumDescriptionBlocks; Pos++)
    {
        Element_Begin1("mae_descriptionGroup");
        int8u ID;
        switch (type)
        {
            case ID_MAE_GROUP_DESCRIPTION:
                Get_S1 (7, ID,                                  "mae_descriptionGroupID");
                break;
            case ID_MAE_SWITCHGROUP_DESCRIPTION:
                Get_S1 (5, ID,                                  "mae_descriptionSwitchGroupID");
                break;
            case ID_MAE_GROUP_PRESET_DESCRIPTION:
                Get_S1 (5, ID,                                  "mae_descriptionGroupPresetID");
                break;
            default:;
        }
        Element_Info1(ID);

        int8u mae_bsNumDescLanguages;
        Get_S1(4, mae_bsNumDescLanguages,                       "mae_bsNumDescLanguages");
        mae_bsNumDescLanguages++;
        for (int8u Pos2=0; Pos2<mae_bsNumDescLanguages; Pos2++)
        {
            Element_Begin1("mae_bsDescription");
            int32u mae_bsDescriptionLanguage;
            Get_S3 (24, mae_bsDescriptionLanguage,              "mae_bsDescriptionLanguage");
            string Language;
            for (int i=16; i>=0; i-=8)
            {
                char LanguageChar=char(mae_bsDescriptionLanguage>>i);
                if (LanguageChar)
                    Language+=LanguageChar;
            }
            Param_Info1(Language);
            Element_Info1(Language);

            int8u mae_bsDescriptionDataLength;
            Get_S1(8, mae_bsDescriptionDataLength,              "mae_bsDescriptionDataLength");
            mae_bsDescriptionDataLength++;
            string Description;
            Description.reserve(mae_bsDescriptionDataLength);
            for (int8u Pos3=0; Pos3<mae_bsDescriptionDataLength; Pos3++)
            {
                int8u mae_descriptionData;
                Get_S1 (8, mae_descriptionData,                 "mae_descriptionData");
                Description+=(char)mae_descriptionData;
            }
            Param_Info1(Ztring().From_UTF8(Description));
            Element_Info1(Ztring().From_UTF8(Description));
            switch (type)
            {
                case ID_MAE_GROUP_DESCRIPTION:
                    for (size_t i=0; i<Groups.size(); i++)
                        if (Groups[i].ID==ID)
                            Groups[i].Description[Language]=Description;
                    break;
                case ID_MAE_SWITCHGROUP_DESCRIPTION:
                    for (size_t i=0; i<SwitchGroups.size(); i++)
                        if (SwitchGroups[i].ID==ID)
                            SwitchGroups[i].Description[Language]=Description;
                    break;
                case ID_MAE_GROUP_PRESET_DESCRIPTION:
                    for (size_t i=0; i<GroupPresets.size(); i++)
                        if (GroupPresets[i].ID==ID)
                            GroupPresets[i].Description[Language]=Description;
                    break;
                default: ;
            }
            Element_End0();
        }
        Element_End0();
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mae_ContentData()
{
    Element_Info1("mae_ContentData");
    Element_Begin1("mae_ContentData");
    int8u mae_bsNumContentDataBlocks;
    Get_S1(7, mae_bsNumContentDataBlocks,                       "mae_bsNumContentDataBlocks");
    for (int8u Pos=0; Pos<mae_bsNumContentDataBlocks+1; Pos++)
    {
        Element_Begin1("mae_ContentDataGroup");
        int8u ID, Kind;
        Get_S1 (7, ID,                                          "mae_ContentDataGroupID");
        Element_Info1(ID);
        Get_S1 (4, Kind,                                        "mae_contentKind");
        Param_Info1C(Kind<Mpegh3da_contentKind_Size, Mpegh3da_contentKind[Kind]);
        Element_Info1C(Kind<Mpegh3da_contentKind_Size, Mpegh3da_contentKind[Kind]);
        string Language;
        TEST_SB_SKIP(                                           "mae_hasContentLanguage");
            int32u mae_contentLanguage;
            Get_S3 (24, mae_contentLanguage,                    "mae_contentLanguage");
            for (int i=16; i>=0; i-=8)
            {
                char LanguageChar=char(mae_contentLanguage>>i);
                if (LanguageChar)
                    Language+=LanguageChar;
            }
            Param_Info1(Language);
            Element_Info1(Language);
            TEST_SB_END();

        for (size_t i=0; i<Groups.size(); i++)
            if (Groups[i].ID==ID)
            {
                Groups[i].Language=Language;
                Groups[i].Kind=Kind;
            }
        Element_End0();
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mae_CompositePair()
{
    Element_Begin1("mae_CompositePair");
    int8u mae_bsNumCompositePairs;
    Get_S1(7, mae_bsNumCompositePairs,                          "mae_bsNumCompositePairs");
    for (int8u Pos=0; Pos<mae_bsNumCompositePairs+1; Pos++)
    {
        Skip_S1(7,                                              "mae_CompositeElementID0");
        Skip_S1(7,                                              "mae_CompositeElementID1");
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mae_ProductionScreenSizeData()
{
    Element_Begin1("mae_ProductionScreenSizeData");
    TEST_SB_SKIP(                                               "hasNonStandardScreenSize");
        Skip_S2(9,                                              "bsScreenSizeAz");
        Skip_S2(9,                                              "bsScreenSizeTopEl");
        Skip_S2(9,                                              "bsScreenSizeBottomEl");
    TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mae_DrcUserInterfaceInfo(int16u dataLength)
{
    Element_Begin1("mae_DrcUserInterfaceInfo");
    int8u version;
    Get_S1(2, version,                                          "version");
    if (version==0)
    {
        int8u bsNumTargetLoudnessConditions;
        Get_S1(3, bsNumTargetLoudnessConditions,                "bsNumTargetLoudnessConditions");
        int16u bsNumTargetLoudnessConditions_Real;
        if (dataLength>2)
            bsNumTargetLoudnessConditions_Real=(dataLength*8-(2+3))/(6+16);
        else
            bsNumTargetLoudnessConditions_Real=0;
        if (bsNumTargetLoudnessConditions!=bsNumTargetLoudnessConditions_Real)
            Param_Info1("Error");
        for (int16u Pos=0; Pos<bsNumTargetLoudnessConditions_Real; Pos++)
        {
            Skip_S1(6,                                          "bsTargetLoudnessValueUpper");
            Skip_S2(16,                                         "drcSetEffectAvailable");
        }
    }
    else
    {
        Skip_BS((dataLength-2)*8,                               "reserved");
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mae_ProductionScreenSizeDataExtension()
{
    Element_Begin1("mae_ProductionScreenSizeDataExtension");
    TEST_SB_SKIP(                                               "mae_overwriteProductionScreenSizeData");
        Skip_S2(10,                                             "bsScreenSizeLeftAz");
        Skip_S2(10,                                             "bsScreenSizeRightAz");
    TEST_SB_END();
    int8u mae_NumPresetProductionScreens;
    Get_S1(5, mae_NumPresetProductionScreens,                   "mae_NumPresetProductionScreens");
    for (int8u Pos=0; Pos< mae_NumPresetProductionScreens; Pos++)
    {
        Skip_S1(5,                                              "mae_productionScreenGroupPresetID");
        TEST_SB_SKIP(                                           "mae_hasNonStandardScreenSize");
            TESTELSE_SB_SKIP(                                   "isCenteredInAzimuth");
                Skip_S2(9,                                      "bsScreenSizeAz");
            TESTELSE_SB_ELSE(                                   "isCenteredInAzimuth");
                Skip_S2(10,                                     "bsScreenSizeLeftAz");
                Skip_S2(10,                                     "bsScreenSizeRightAz");
            TESTELSE_SB_END();
            Skip_S2(9,                                          "bsScreenSizeTopEl");
            Skip_S2(9,                                          "bsScreenSizeBottomEl");
        TEST_SB_END();
    }
    Element_End0();
}


//---------------------------------------------------------------------------
void File_Mpegh3da::mae_GroupPresetDefinitionExtension(int8u numGroupPresets)
{
    Element_Begin1("mae_GroupPresetDefinitionExtension");
    for (int8u Pos=0; Pos<numGroupPresets; Pos++)
    {
        TEST_SB_SKIP(                                           "mae_hasSwitchGroupConditions");
            int8u Temp=Pos<GroupPresets.size()?GroupPresets[Pos].Conditions.size():0;
            for(int8u Pos2=0; Pos2<Temp; Pos2++)
                Skip_SB(                                        "mae_isSwitchGroupCondition");
        TEST_SB_END();

        TEST_SB_SKIP(                                           "mae_hasDownmixIdGroupPresetExtensions");
            int8u mae_numDownmixIdGroupPresetExtensions;
            Get_S1(5, mae_numDownmixIdGroupPresetExtensions,    "mae_numDownmixIdGroupPresetExtensions");
            for (int8u Pos2=1; Pos2<mae_numDownmixIdGroupPresetExtensions+1; Pos2++)
            {
                Skip_S1(7,                                      "mae_groupPresetDownmixId");

                int8u mae_bsGroupPresetNumConditions;
                Get_S1(4, mae_bsGroupPresetNumConditions,       "mae_bsGroupPresetNumConditions");
                for (int8u Pos3=0; Pos3<mae_bsGroupPresetNumConditions+1; Pos3++)
                {
                    TESTELSE_SB_SKIP(                           "mae_isSwitchGroupCondition");
                        Skip_S1(5,                              "mae_groupPresetSwitchGroupID");
                    TESTELSE_SB_ELSE(                           "mae_isSwitchGroupCondition");
                        Skip_S1(7,                              "mae_groupPresetGroupID");
                    TESTELSE_SB_END();
                    TEST_SB_SKIP(                               "mae_groupPresetConditionOnOff");
                        Skip_SB(                                "mae_groupPresetDisableGainInteractivity");
                        TEST_SB_SKIP(                           "mae_groupPresetGainFlag");
                            Skip_S1(8,                          "mae_groupPresetGain");
                        TEST_SB_END();
                        Skip_SB(                                "mae_groupPresetDisablePositionInteractivity");
                        TEST_SB_SKIP(                           "mae_groupPresetPositionFlag");
                            Skip_S1(8,                          "mae_groupPresetAzOffset");
                            Skip_S1(6,                          "mae_groupPresetElOffset");
                            Skip_S1(4,                          "mae_groupPresetDistFactor");
                        TEST_SB_END();
                    TEST_SB_END();
                }
            }
        TEST_SB_END();
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mae_LoudnessCompensationData(int8u numGroups, int8u numGroupPresets)
{
    Element_Begin1("mae_LoudnessCompensationData");
    TEST_SB_SKIP(                                               "mae_loudnessCompGroupLoudnessPresent");
        for (int8u Pos=0; Pos<numGroups; Pos++)
            Skip_S1(8,                                          "mae_bsLoudnessCompGroupLoudness");
    TEST_SB_END();

    TEST_SB_SKIP(                                               "mae_loudnessCompDefaultParamsPresent");
        for (int8u Pos=0; Pos<numGroups; Pos++)
            Skip_SB(                                            "mae_loudnessCompDefaultIncludeGroup");

        TEST_SB_SKIP(                                           "mae_loudnessCompDefaultMinMaxGainPresent");
            Skip_S1(4,                                          "mae_bsLoudnessCompDefaultMinGain");
            Skip_S1(4,                                          "mae_bsLoudnessCompDefaultMaxGain");
        TEST_SB_END();
    TEST_SB_END();

    for (int8u Pos=0; Pos<numGroupPresets; Pos++)
    {
        TEST_SB_SKIP(                                           "mae_loudnessCompPresetParamsPresent");
            for (int8u Pos2=0; Pos2<numGroups; Pos2++)
                Skip_SB(                                        "mae_loudnessCompPresetIncludeGroup");

            TEST_SB_SKIP(                                       "mae_loudnessCompPresetMinMaxGainPresent");
                Skip_S1(4,                                      "mae_bsLoudnessCompPresetMinGain");
                Skip_S1(4,                                      "mae_bsLoudnessCompPresetMaxGain");
            TEST_SB_END();
        TEST_SB_END();
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::audioTruncationInfo()
{
    Element_Begin1("audioTruncationInfo");
    BS_Begin();
    Skip_SB(                                                    "isActive");
    Skip_SB(                                                    "ati_reserved");
    Skip_SB(                                                    "truncFromBegin");
    Skip_S2(13,                                                 "nTruncSamples");
    BS_End();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Mpegh3da::mhaC()
{
    Element_Begin1("MHADecoderConfigurationRecord");
    Skip_B1(                                                    "configurationVersion");
    Skip_B1(                                                    "mpegh3daProfileLevelIndication");
    Skip_B1(                                                    "referenceChannelLayout");
    Skip_B2(                                                    "mpegh3daConfigLength");
    mpegh3daConfig();
    Element_End0();
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
size_t File_Mpegh3da::num_objects_Get()
{
    size_t num_signals_InElements=0;
    for (size_t i=0; i<Elements.size(); i++)
        if (Elements[i].Type==ID_USAC_SCE || Elements[i].Type==ID_USAC_CPE)
            num_signals_InElements++;
    size_t num_signals_InSignals=0;
    for (size_t i=0; i<SignalGroups.size(); i++)
    {
        if (num_signals_InElements==num_signals_InSignals)
        {
            return SignalGroups[i].bsNumberOfSignals;
            break;
        }
        num_signals_InSignals+=SignalGroups[i].bsNumberOfSignals;
    }

    return 0; //Problem
}


//---------------------------------------------------------------------------
void File_Mpegh3da::Streams_Fill_ChannelLayout(const string& Prefix, const speaker_layout& Layout, int8u speakerLayoutType)
{
    if (Aac_Channels_Get(Layout.ChannelLayout))
    {
        Fill(Stream_Audio, 0, (Prefix+"Channel(s)").c_str(), Aac_Channels_GetString(Layout.ChannelLayout));
        if (!Prefix.empty())
            Fill_SetOptions(Stream_Audio, 0, (Prefix + "Channel(s)").c_str(), "N NTY");
        if (!Prefix.empty())
        {
            string ChannelString=MediaInfoLib::Config.Language_Get(Ztring().From_UTF8(Aac_Channels_GetString(Layout.ChannelLayout)), __T(" channel")).To_UTF8();
            string ChannelMode=Aac_ChannelMode_GetString(Layout.ChannelLayout, true);
            if (ChannelMode.size()>3 || (ChannelMode.size()==3 && ChannelMode[2]!='0'))
                ChannelString+=" ("+Aac_ChannelMode_GetString(Layout.ChannelLayout, true)+")";
            Fill(Stream_Audio, 0, (Prefix+"Channel(s)/String").c_str(), ChannelString);
            Fill_SetOptions(Stream_Audio, 0, (Prefix + "Channel(s)/String").c_str(), "Y NTN");
        }
        Fill(Stream_Audio, 0, (Prefix+"ChannelPositions").c_str(), Aac_ChannelConfiguration_GetString(Layout.ChannelLayout));
        if (!Prefix.empty())
            Fill_SetOptions(Stream_Audio, 0, (Prefix+"ChannelPositions").c_str(), "N YTY");
        Fill(Stream_Audio, 0, (Prefix+"ChannelPositions/String2").c_str(), Aac_ChannelConfiguration2_GetString(Layout.ChannelLayout));
        if (!Prefix.empty())
            Fill_SetOptions(Stream_Audio, 0, (Prefix+"ChannelPositions/String2").c_str(), "N YTY");
        Fill(Stream_Audio, 0, (Prefix+"ChannelMode").c_str(), Aac_ChannelMode_GetString(Layout.ChannelLayout, true));
        Fill_SetOptions(Stream_Audio, 0, (Prefix+"ChannelMode").c_str(), "N NTY");
        Fill(Stream_Audio, 0, (Prefix+"ChannelLayout").c_str(), Aac_ChannelLayout_GetString(Layout.ChannelLayout, true));
    }
    else if (Layout.numSpeakers)
    {
        if (speakerLayoutType==1) // Objects
        {
            Fill(Stream_Audio, 0, (Prefix+"NumberOfObjects").c_str(), Layout.numSpeakers);
            Fill_SetOptions(Stream_Audio, 0, (Prefix+"NumberOfObjects").c_str(), "N YTY");
            Fill(Stream_Audio, 0, (Prefix + "NumberOfObjects/String").c_str(), MediaInfoLib::Config.Language_Get(Ztring::ToZtring(Layout.numSpeakers), __T(" object")));
            Fill_SetOptions(Stream_Audio, 0, (Prefix+"NumberOfObjects/String").c_str(), "Y YTN");
        }
        else
        {
            Fill(Stream_Audio, 0, (Prefix+"Channel(s)").c_str(), Layout.numSpeakers);
            Fill_SetOptions(Stream_Audio, 0, (Prefix+"Channel(s)").c_str(), "N YTY");
            Fill(Stream_Audio, 0, (Prefix + "Channel(s)/String").c_str(), MediaInfoLib::Config.Language_Get(Ztring::ToZtring(Layout.numSpeakers), __T(" channel")));
            Fill_SetOptions(Stream_Audio, 0, (Prefix+"Channel(s)/String").c_str(), "Y YTN");
        }
        if (!Layout.CICPspeakerIdxs.empty())
        {
            Fill(Stream_Audio, 0, (Prefix+"ChannelMode").c_str(), Aac_ChannelMode_GetString(Layout.CICPspeakerIdxs));
            Fill(Stream_Audio, 0, (Prefix+"ChannelLayout").c_str(), Aac_ChannelLayout_GetString(Layout.CICPspeakerIdxs));
        }
        else
        {
            vector<Aac_OutputChannel> CICPspeakerIdxs;
            string ChannelLayout;
            for (size_t i=0; i<Layout.SpeakersInfo.size(); i++)
            {
                if (i)
                    ChannelLayout+=' ';
                if (Layout.SpeakersInfo[i].CICPspeakerIdx!=(Aac_OutputChannel)-1)
                {
                    ChannelLayout+=Aac_ChannelLayout_GetString(&Layout.SpeakersInfo[i].CICPspeakerIdx, 1);
                    CICPspeakerIdxs.push_back(Layout.SpeakersInfo[i].CICPspeakerIdx);
                }
                else
                {
                    if (Layout.SpeakersInfo[i].ElevationAngle==0)
                        ChannelLayout+='M';
                    else
                        ChannelLayout+=Layout.SpeakersInfo[i].ElevationDirection?'B':'U';
                    ChannelLayout+='_';
                    if (Layout.SpeakersInfo[i].AzimuthAngle!=0 && Layout.SpeakersInfo[i].AzimuthAngle!=180)
                        ChannelLayout+=Layout.SpeakersInfo[i].AzimuthDirection?'L':'R';
                    string AzimuthAngleString=Ztring::ToZtring(Layout.SpeakersInfo[i].AzimuthAngle).To_UTF8();
                    AzimuthAngleString.insert(0, 3-AzimuthAngleString.size(), '0');
                    ChannelLayout.append(AzimuthAngleString);
                }
            }
            if (CICPspeakerIdxs.size()==Layout.SpeakersInfo.size())
            {
                Fill(Stream_Audio, 0, (Prefix+"ChannelMode").c_str(), Aac_ChannelMode_GetString(CICPspeakerIdxs));
                Fill(Stream_Audio, 0, (Prefix+"ChannelLayout").c_str(), Aac_ChannelLayout_GetString(CICPspeakerIdxs));
            }
            else
                Fill(Stream_Audio, 0, (Prefix+"ChannelLayout").c_str(), ChannelLayout);
        }
    }
    else if (Layout.ChannelLayout)
    {
        Fill(Stream_Audio, 0, (Prefix+"ChannelLayout").c_str(), Layout.ChannelLayout);
    }
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_MPEGH3DA_YES


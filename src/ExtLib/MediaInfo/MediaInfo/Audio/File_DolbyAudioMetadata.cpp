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
#if defined(MEDIAINFO_RIFF_YES) || defined(MEDIAINFO_MXF_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_DolbyAudioMetadata.h"
#include "MediaInfo/TimeCode.h"
#include "tinyxml2.h"
#include "ThirdParty/base64/base64.h"
using namespace tinyxml2;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Info
//***************************************************************************

static const char* metadata_segment_Name[] =
{
    "End",
    "Dolby E Metadata",
    NULL,
    "Dolby Digital Metadata",
    NULL,
    NULL,
    NULL,
    "Dolby Digital Plus Metadata",
    "Audio Info",
    "Dolby Atmos Metadata",
    "Dolby Atmos Supplemental Metadata",
};
static size_t metadata_segment_Name_Size=sizeof(metadata_segment_Name)/sizeof(const char*);

static const char* binaural_render_mode_Name[] =
{
    "Off",
    "Near",
    "Far",
    "Mid",
    "Not Indicated (Mid)",
};
static size_t binaural_render_mode_Name_Size=sizeof(binaural_render_mode_Name)/sizeof(const char*);

static const char* warp_mode_Name[] =
{
    "Direct Render",
    "Direct Render with room balance",
    "Dolby Pro Logic IIx",
    "Standard (Lo/Ro)",
    NULL,
};
static size_t warp_mode_Name_Size=sizeof(warp_mode_Name)/sizeof(const char*);

static const char* trim_bypass_Name[] =
{
    "Trim applied",
    "Trim ignored",
    "Not Indicated",
    NULL,
};

static const float32 frames_per_second_Values[] =
{
    0,
    24/1.001,
    24,
    25,
    30/1.001, // Drop frame
    30/1.001,
    30,
};
static size_t frames_per_second_Size=sizeof(frames_per_second_Values)/sizeof(float32);

static const char* downmix_type_5to2_Values[] =
{
    "Not indicated (Lo/Ro)",
    "Lo/Ro",
    "Lt/Rt (Dolby Pro Logic)",
    "Lt/Rt (Dolby Pro Logic II)",
    "Direct stereo render",
};
static size_t downmix_type_5to2_Size=sizeof(downmix_type_5to2_Values)/sizeof(const char*);

static const char* phaseshift_90deg_5to2_Values[] =
{
    "w/o Phase 90",
    "w/ Phase 90",
};
static size_t phaseshift_90deg_5to2_Size=sizeof(phaseshift_90deg_5to2_Values)/sizeof(const char*);

//---------------------------------------------------------------------------
void Merge_FillTimeCode(File__Analyze& In, const string& Prefix, const TimeCode& TC_Time, float FramesPerSecondF, bool DropFrame, TimeCode::rounding Rounding=TimeCode::Nearest, int32u Frequency=0);

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_DolbyAudioMetadata::File_DolbyAudioMetadata()
{
    //Configuration
    StreamSource=IsContainerExtra;

    //In
    IsXML=false;

    //Out
    HasSegment9=false;
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_DolbyAudioMetadata::FileHeader_Begin()
{
    if (!IsXML)
        return true;

    XMLDocument document;
    if (!FileHeader_Begin_XML(document))
    {
        return false;
    }

    XMLElement* Base64DbmdWrapper=document.FirstChildElement();
    if (!Base64DbmdWrapper || strcmp(Base64DbmdWrapper->Name(), "Base64DbmdWrapper"))
    {
        return false;
    }

    if (const char* Text=Base64DbmdWrapper->GetText())
    {
        const int8u* Save_Buffer=Buffer;
        size_t Save_Buffer_Size=Buffer_Size;

        string BufferS = Base64::decode(Text);
        Buffer=(const int8u*)BufferS.c_str();
        Element_Size=Buffer_Size=BufferS.size();

        Element_Begin1("Header");
        int32u Name, Size;
        Get_C4 (Name,                                           "Name");
        Get_L4 (Size,                                           "Size");
        if (Name==0x64626D64 && Size==Element_Size-Element_Offset)
            Read_Buffer_Continue();
        else
            Skip_XX(Element_Size-Element_Offset,                "Unknown");

        Buffer=Save_Buffer;
        Element_Offset=Element_Size=Buffer_Size=Save_Buffer_Size;
    }

    return true;
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_DolbyAudioMetadata::Read_Buffer_Continue()
{
    Accept("DolbyAudioMetadata");
    Stream_Prepare(Stream_Audio);

    //Parsing
    int32u version;
    Get_L4 (version,                                            "version");
    if ((version>>24)>1)
    {
        Skip_XX(Element_Size-Element_Offset,                    "Data");
        return;
    }
    while(Element_Offset<Element_Size)
    {
        Element_Begin1("metadata_segment");
        int8u metadata_segment_id;
        Get_L1 (metadata_segment_id,                            "metadata_segment_id"); Element_Info1(Ztring::ToZtring(metadata_segment_id));
        if (metadata_segment_id<metadata_segment_Name_Size && metadata_segment_Name[metadata_segment_id])
            Element_Info1(metadata_segment_Name[metadata_segment_id]);
        if (!metadata_segment_id)
        {
            Element_End0();
            break;
        }
        int16u metadata_segment_size;
        Get_L2 (metadata_segment_size,                          "metadata_segment_size");
        int64u metadata_segment_size_Max=Element_Size-Element_Offset;
        if (metadata_segment_size_Max)
            metadata_segment_size_Max--; //metadata_segment_checksum
        if (metadata_segment_size>metadata_segment_size_Max)
            metadata_segment_size=metadata_segment_size_Max;
        size_t End=Element_Offset+metadata_segment_size;
        Element_Begin1("metadata_segment_payload");
        switch (metadata_segment_id)
        {
            case 9:
                Dolby_Atmos_Metadata_Segment();
                break;
            case 10:
                Dolby_Atmos_Supplemental_Metadata_Segment();
                break;
            default:;
        }
        Skip_XX(End-Element_Offset,                             "Unknown");
        Element_End0();
        Skip_L1(                                                "metadata_segment_checksum");
        Element_End0();
    }

    Finish();
}


//***************************************************************************
// Elements
//***************************************************************************


//---------------------------------------------------------------------------
void File_DolbyAudioMetadata::Dolby_Atmos_Metadata_Segment()
{
    HasSegment9=true; // Needed for flagging Dolby Atmos Master ADM profile
    Fill(Stream_Audio, 0, "Dolby_Atmos_Metadata", "Yes");

    //Parsing
    Ztring content_creation_tool;
    int32u content_creation_tool_version, first_action_time_SS;
    int8u warp_mode, frames_per_second, downmix_type_5to2, phaseshift_90deg_5to2, first_action_time_HH, first_action_time_MM;
    Skip_String(32,                                             "reserved");
    Element_Begin1("content_information");
    Get_UTF8(64, content_creation_tool,                         "content_creation_tool");
    Get_B3 (content_creation_tool_version,                      "content_creation_tool_version");
    Skip_XX(12,                                                 "Unknown");
    BS_Begin();
    Skip_S1(4,                                                  "Unknown");
    Get_S1 (4, frames_per_second,                               "frames_per_second"); Param_Info1C(frames_per_second<frames_per_second_Size, frames_per_second_Values[frames_per_second]);
    BS_End();
    Element_Begin1("first_action_time");
        Get_L1 (first_action_time_HH,                           "HH");
        Get_L1 (first_action_time_MM,                           "MM");
        Get_L4 (first_action_time_SS,                           "1/100000 SS");
    Element_End0();
    Skip_XX(21,                                                 "Unknown");
    BS_Begin();
    Skip_SB(                                                    "Unknown");
    Get_S1 (3, downmix_type_5to2,                               "downmix_type_5to2"); Param_Info1C(downmix_type_5to2<downmix_type_5to2_Size, downmix_type_5to2_Values[downmix_type_5to2]);
    Skip_S1(2,                                                  "Unknown");
    Get_S1 (2, phaseshift_90deg_5to2,                           "phaseshift_90deg_5to2"); Param_Info1C(phaseshift_90deg_5to2<phaseshift_90deg_5to2_Size, phaseshift_90deg_5to2_Values[phaseshift_90deg_5to2]);
    BS_End();
    Skip_XX(12,                                                 "Unknown");
    Element_Begin1("additional_rendering_metadata");
    BS_Begin();
    Skip_S1(2,                                                  "bed_distribution");
    Skip_S1(3,                                                  "reserved");
    Get_S1 (3, warp_mode,                                       "warp_mode");
    BS_End();
    Skip_XX(15,                                                 "reserved");
    Element_End0();
    Element_End0();
    FILLING_BEGIN()
        Ztring Version=__T("Version ")
            +Ztring::ToZtring(content_creation_tool_version>>16)+__T('.')
            +Ztring::ToZtring((content_creation_tool_version>>8)&0xFF)+__T('.')
            +Ztring::ToZtring(content_creation_tool_version&0xFF);
        Fill(Stream_Audio, 0, "Dolby_Atmos_Metadata Encoded_Application", content_creation_tool+__T(", ")+Version);
        string Downmix_5to2;
        if (downmix_type_5to2 && downmix_type_5to2<downmix_type_5to2_Size)
            Downmix_5to2=downmix_type_5to2_Values[downmix_type_5to2];
        if (phaseshift_90deg_5to2<phaseshift_90deg_5to2_Size)
        {
            if (!Downmix_5to2.empty())
                Downmix_5to2+=' ';
            Downmix_5to2+=phaseshift_90deg_5to2_Values[phaseshift_90deg_5to2];
        }
        Fill(Stream_Audio, 0, "Dolby_Atmos_Metadata Downmix_5to2", Downmix_5to2);
        if (warp_mode!=4) // Avoid "Not indicated"
            Fill(Stream_Audio, 0, "Dolby_Atmos_Metadata Downmix_5.1.x", warp_mode<warp_mode_Name_Size?warp_mode_Name[warp_mode]:Ztring().ToZtring(warp_mode).To_UTF8().c_str());
        if (frames_per_second && frames_per_second<frames_per_second_Size)
        {
            Ztring FrameRate; FrameRate.From_Number(frames_per_second_Values[frames_per_second]);
            Fill(Stream_Audio, 0, "Dolby_Atmos_Metadata AssociatedVideo_FrameRate", FrameRate);
            FrameRate+=__T(" FPS");
            switch (frames_per_second)
            {
                case 4:
                case 5:
                    Fill(Stream_Audio, 0, "Dolby_Atmos_Metadata AssociatedVideo_FrameRate/String", FrameRate+(frames_per_second==4?__T(" DF"):__T(" NDF")));
                    Fill(Stream_Audio, 0, "Dolby_Atmos_Metadata AssociatedVideo_FrameRate_DropFrame", frames_per_second==4?"Yes":"No");
                    break;
                default:
                    Fill(Stream_Audio, 0, "Dolby_Atmos_Metadata AssociatedVideo_FrameRate/String", FrameRate);
            }
            Fill_SetOptions(Stream_Audio, 0, "Dolby_Atmos_Metadata AssociatedVideo_FrameRate", "N NTY");
            Fill_SetOptions(Stream_Audio, 0, "Dolby_Atmos_Metadata AssociatedVideo_FrameRate/String", "Y NTN");
            Fill_SetOptions(Stream_Audio, 0, "Dolby_Atmos_Metadata AssociatedVideo_FrameRate_DropFrame", "N NTY");
        }
        if (first_action_time_HH!=(int8u)-1)
        {
            bool IsNegative=((int8s)first_action_time_HH)<0;
            if (IsNegative)
                first_action_time_HH=(int8u)((-((int8s)first_action_time_HH)));
            const int32u FrameRate = 100000;
            TimeCode TC(first_action_time_HH, first_action_time_MM, first_action_time_SS/FrameRate, first_action_time_SS%FrameRate, FrameRate-1, TimeCode::Timed().Negative(IsNegative));
            Merge_FillTimeCode(*this, "Dolby_Atmos_Metadata FirstFrameOfAction", TC, frames_per_second_Values[frames_per_second], frames_per_second==4, TimeCode::Ceil, 48000);
        }
        FILLING_END()
}

//---------------------------------------------------------------------------
void File_DolbyAudioMetadata::Dolby_Atmos_Supplemental_Metadata_Segment()
{
    //Parsing
    int32u dasms_sync;
    Get_L4 (dasms_sync,                                         "dasms_sync");
    if (dasms_sync!=0xF8726FBD)
    {
        Skip_XX(Element_Size-Element_Offset,                    "Unknown");
        return;
    }
    int16u object_count;
    Get_L2 (object_count,                                       "object_count");
    Element_Begin1("trim_metadata");
    Skip_L1(                                                    "reserved");
    bitset<2> TrimAutoSet;
    bool TrimNotAlwaysManual=false;
    Fill(Stream_Audio, 0, "Dolby_Atmos_Metadata TrimMode", "Yes");
    for (int cfg=0; cfg<9; cfg++)
    {
        Element_Begin1("config_trim");
        bool auto_trim;
        BS_Begin();
        Skip_S1(7,                                              "reserved");
        Get_SB (   auto_trim,                                   "auto_trim");
        BS_End();
        string FieldPrefix="Dolby_Atmos_Metadata TrimMode TrimMode"+Ztring::ToZtring(cfg).To_UTF8()+'i';
        Fill(Stream_Audio, 0, FieldPrefix.c_str(), auto_trim?"Automatic":"Manual");
        TrimAutoSet.set(auto_trim);
        if (auto_trim)
        {
            switch (cfg)
            {
                case 1: // 5.1
                case 2: // 7.1
                case 4: // 5.1.2
                    break;
                default:
                    Fill_SetOptions(Stream_Audio, 0, FieldPrefix.c_str(), "N N Y");
            }
            Skip_XX(14,                                         "reserved");
            Fill(Stream_Audio, 0, (FieldPrefix+" Trim_Center").c_str(), "Automatic");
            Fill_SetOptions(Stream_Audio, 0, (FieldPrefix+" Trim_Center").c_str(), "N NTY");
            Fill(Stream_Audio, 0, (FieldPrefix+" Trim_Surround").c_str(), "Automatic");
            Fill_SetOptions(Stream_Audio, 0, (FieldPrefix+" Trim_Surround").c_str(), "N NTY");
            Fill(Stream_Audio, 0, (FieldPrefix+" Trim_Height").c_str(), "Automatic");
            Fill_SetOptions(Stream_Audio, 0, (FieldPrefix+" Trim_Height").c_str(), "N NTY");
        }
        else
        {
            float center_trim, surround_trim, height_trim;
            int8u frontback_balance_ohfl_amount, frontback_balance_lstr_amount;
            bool frontback_balance_ohfl_direction, frontback_balance_lstr_direction;
            Get_LF4 (center_trim,                               "center_trim");
            Get_LF4 (surround_trim,                             "surround_trim");
            Get_LF4 (height_trim,                               "height_trim");
            BS_Begin();
            Get_SB (   frontback_balance_ohfl_direction,        "frontback_balance_ohfl_direction");
            Get_S1 (7, frontback_balance_ohfl_amount,           "frontback_balance_ohfl_amount");
            Get_SB (   frontback_balance_lstr_direction,        "frontback_balance_lstr_direction");
            Get_S1 (7, frontback_balance_lstr_amount,           "frontback_balance_lstr_amount");
            BS_End();
            if (center_trim)
            {
                Fill(Stream_Audio, 0, (FieldPrefix+" Trim_Center").c_str(), center_trim);
                Fill_SetOptions(Stream_Audio, 0, (FieldPrefix+" Trim_Center").c_str(), "N NTY");
                Fill(Stream_Audio, 0, (FieldPrefix+" Trim_Center/String").c_str(), Ztring::ToZtring(center_trim, 2)+__T(" dB"));
                Fill_SetOptions(Stream_Audio, 0, (FieldPrefix+" Trim_Center/String").c_str(), "Y NTN");
            }
            if (surround_trim)
            {
                Fill(Stream_Audio, 0, (FieldPrefix+" Trim_Surround").c_str(), surround_trim);
                Fill_SetOptions(Stream_Audio, 0, (FieldPrefix+" Trim_Surround").c_str(), "N NTY");
                Fill(Stream_Audio, 0, (FieldPrefix+" Trim_Surround/String").c_str(), Ztring::ToZtring(surround_trim, 2)+__T(" dB"));
                Fill_SetOptions(Stream_Audio, 0, (FieldPrefix+" Trim_Surround/String").c_str(), "Y NTN");
            }
            if (height_trim)
            {
                Fill(Stream_Audio, 0, (FieldPrefix+" Trim_Height").c_str(), height_trim);
                Fill_SetOptions(Stream_Audio, 0, (FieldPrefix+" Trim_Height").c_str(), "N NTY");
                Fill(Stream_Audio, 0, (FieldPrefix+" Trim_Height/String").c_str(), Ztring::ToZtring(height_trim, 2)+__T(" dB"));
                Fill_SetOptions(Stream_Audio, 0, (FieldPrefix+" Trim_Height/String").c_str(), "Y NTN");
            }
            if (frontback_balance_ohfl_amount)
            {
                float32 frontback_balance_ohfl=((float32)frontback_balance_ohfl_amount)/127;
                if (frontback_balance_ohfl_direction)
                    frontback_balance_ohfl=-frontback_balance_ohfl;
                Fill(Stream_Audio, 0, (FieldPrefix+" Balance_FrontBackOverheadFloor").c_str(), frontback_balance_ohfl, 2);
            }
            if (frontback_balance_lstr_amount)
            {
                float32 frontback_balance_lstr=((float32)frontback_balance_lstr_amount)/127;
                if (frontback_balance_lstr_direction)
                    frontback_balance_lstr=-frontback_balance_lstr;
                Fill(Stream_Audio, 0, (FieldPrefix+" Balance_FrontBackListener").c_str(), frontback_balance_lstr, 2);
            }
        }
        Element_End0();
    }
    ZtringList List;
    List.Separator_Set(0, __T(" / "));
    if (TrimAutoSet[0])
        List.push_back(__T("Manual"));
    if (TrimAutoSet[1])
    {
        List.push_back(__T("Automatic"));
        if (!TrimAutoSet[0])
        {
            for (int cfg=0; cfg<9; cfg++)
            {
                Fill_SetOptions(Stream_Audio, 0, (string("Dolby_Atmos_Metadata TrimMode TrimMode")+Ztring::ToZtring(cfg).To_UTF8()+'i').c_str(), "N NTY");
            }
        }
    }
    Fill(Stream_Audio, 0, "Dolby_Atmos_Metadata TrimMode", List.Read(), true);

    if (object_count)
    {
        BS_Begin();
        // Fill(Stream_Audio, 0, ("Dolby_Atmos_Metadata Objects"), "Yes");
        for (int obj = 0; obj<object_count; obj++)
        {
            Element_Begin1("object_trim");
            int8u trim_bypass;
            Skip_S1(6,                                              "reserved");
            Get_S1 (2, trim_bypass,                                 "trim_bypass");
            /*
            if (trim_bypass<2)
            {
                Fill(Stream_Audio, 0, ("Dolby_Atmos_Metadata Objects Object"+Ztring::ToZtring(obj).To_UTF8()).c_str(), trim_bypass_Name[trim_bypass]);
                Fill(Stream_Audio, 0, ("Dolby_Atmos_Metadata Objects Object"+Ztring::ToZtring(obj).To_UTF8()+" TrimApplied").c_str(), trim_bypass?"Yes":"No");
                Fill_SetOptions(Stream_Audio, 0, ("Dolby_Atmos_Metadata Objects Object"+Ztring::ToZtring(obj).To_UTF8()+" TrimApplied").c_str(), "N NTY");
            }
            */
            Element_End0();
        }
        BS_End();
    }
    Element_End0();
    Element_Begin1("headphone_metadata");
    BS_Begin();
    for (int obj=0; obj<object_count; obj++)
    {
        int8u head_track_mode, binaural_render_mode;
        Get_S1 (2, head_track_mode,                             "head_track_mode");
        Skip_S1(3,                                              "reserved");
        Get_S1 (3, binaural_render_mode,                        "binaural_render_mode"); Param_Info1C(binaural_render_mode<binaural_render_mode_Name_Size, binaural_render_mode_Name[binaural_render_mode]);
        BinauralRenderModes.push_back(binaural_render_mode);
    }
    BS_End();
    Element_End0();
}

//***************************************************************************
// Delayed
//***************************************************************************

//---------------------------------------------------------------------------
void File_DolbyAudioMetadata::Merge(File__Analyze& In, size_t StreamPos)
{
    const auto& FirstFrameOfAction=In.Retrieve_Const(Stream_Audio, 0, "Dolby_Atmos_Metadata FirstFrameOfAction");
    if (!FirstFrameOfAction.empty())
    {
        const auto& Start=In.Retrieve_Const(Stream_Audio, 0, "Programme0 Start");
        if (!Start.empty())
        {
            TimeCode TC_FirstFrameOfAction(FirstFrameOfAction.To_UTF8());
            TimeCode TC_Start_Time(Start.To_UTF8());
            auto TC=TC_FirstFrameOfAction+TC_Start_Time;
            In.Fill(Stream_Audio, 0, "Dolby_Atmos_Metadata FirstFrameOfAction", TC.ToString(), true, true);
        }
    }

    if (BinauralRenderModes.empty())
        return;

    auto TransportEmpty = In.Retrieve_Const(Stream_Audio, 0, "Transport0").empty();
    ZtringList TrackUID_Pos;
    TrackUID_Pos.Separator_Set(0, " + ");
    static const auto BinauralRenderMode_String=" BinauralRenderMode";
    static const auto LinkedTo_TrackUID_Pos_String=" LinkedTo_TrackUID_Pos";
    static const auto TrackUID_String="TrackUID";

    for (size_t TrackIndex=0; TrackIndex<BinauralRenderModes.size(); TrackIndex++)
    {
        if (TransportEmpty)
            TrackUID_Pos.Write(Ztring().From_Number(TrackIndex)); // Fallback
        else
        {
            TrackUID_Pos.Write(In.Retrieve_Const(Stream_Audio, 0, ("Transport0 TrackIndex"+to_string(TrackIndex)+LinkedTo_TrackUID_Pos_String).c_str()));
            if (TrackUID_Pos.empty())
                TrackUID_Pos.Write(Ztring().From_Number(TrackIndex)); // Fallback
        }
        auto binaural_render_mode=BinauralRenderModes[TrackIndex];
        auto BinauralRenderMode=binaural_render_mode<binaural_render_mode_Name_Size?Ztring().From_UTF8(binaural_render_mode_Name[binaural_render_mode]):Ztring().ToZtring(binaural_render_mode).To_UTF8().c_str();
        for (const auto& Pos : TrackUID_Pos)
        {
            string Name=TrackUID_String+Pos.To_UTF8();
            auto PosI=(size_t)Pos.To_int64u();
            if (In.Retrieve_Const(Stream_Audio, 0, Name.c_str()).empty())
                In.Fill(Stream_Audio, 0, Name.c_str(), "Yes");
            Name+=BinauralRenderMode_String;
            In.Fill(Stream_Audio, 0, Name.c_str(), BinauralRenderMode);
        }
    }

    int64u NumberOfObjects=In.Retrieve_Const(Stream_Audio, 0, "NumberOfObjects").To_int64u();
    for (size_t i=0; i<NumberOfObjects; i++)
    {
        string Name="Object"+Ztring::ToZtring(i).To_UTF8();
        const Ztring& LinkedTos=In.Retrieve_Const(Stream_Audio, 0, (Name+LinkedTo_TrackUID_Pos_String).c_str());
        ZtringList LinkedToList;
        LinkedToList.Separator_Set(0, " + ");
        LinkedToList.Write(LinkedTos);
        if (In.Retrieve_Const(Stream_Audio, 0, Name.c_str()).empty())
            In.Fill(Stream_Audio, 0, Name.c_str(), "Yes");
        Name+=BinauralRenderMode_String;
        ZtringList BinauralRenderModes;
        set<Ztring> BinauralRenderMode_Diffs;
        BinauralRenderModes.Separator_Set(0, " + ");
        for (size_t i=0; i<LinkedToList.size(); i++)
        {
            const auto& LinkedTo=LinkedToList[i].To_UTF8();
            const Ztring& BinauralRenderMode=In.Retrieve_Const(Stream_Audio, 0, (TrackUID_String+LinkedTo+BinauralRenderMode_String).c_str());
            BinauralRenderModes.push_back(BinauralRenderMode);
            BinauralRenderMode_Diffs.insert(BinauralRenderMode);
        }
        if (BinauralRenderMode_Diffs.size()==1)
            BinauralRenderModes.resize(1);
        In.Fill(Stream_Audio, 0, Name.c_str(), BinauralRenderModes.Read());
    }
}

} //NameSpace

#endif //defined(MEDIAINFO_RIFF_YES) || defined(MEDIAINFO_MXF_YES)

/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Contributor: Lionel Duchateau, kurtnoise@free.fr
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
// Pre-compilation
#include "MediaInfo/PreComp.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Setup.h"
using namespace ZenLib;
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_MPEGPS_YES) || defined(MEDIAINFO_MPEGTS_YES) || defined(MEDIAINFO_HEVC_YES)
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
extern const char* Hevc_tier_flag(bool tier_flag)
{
    return tier_flag ? "High" : "Main";
}

//---------------------------------------------------------------------------
extern const char* Hevc_profile_idc(int32u profile_idc)
{
    switch (profile_idc)
    {
        case   1 : return "Main";
        case   2 : return "Main 10";
        case   3 : return "Main Still";
        case   4 : return "Format Range"; // extensions
        case   5 : return "High Throughput";
        case   6 : return "Multiview Main";
        case   7 : return "Scalable Main"; // can be "Scalable Main 10" depending on general_max_8bit_constraint_flag
        case   8 : return "3D Main";
        case   9 : return "Screen Content"; // coding extensions
        case  10 : return "Scalable Format Range"; // extensions
        default  : return "";
    }
}

} //NameSpace

//---------------------------------------------------------------------------
#endif //...
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_HEVC_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Video/File_Hevc.h"
#if defined(MEDIAINFO_AFDBARDATA_YES)
    #include "MediaInfo/Video/File_AfdBarData.h"
#endif //defined(MEDIAINFO_AFDBARDATA_YES)
#if defined(MEDIAINFO_DTVCCTRANSPORT_YES)
    #include "MediaInfo/Text/File_DtvccTransport.h"
#endif //defined(MEDIAINFO_DTVCCTRANSPORT_YES)
#include <cmath>
#include <algorithm>
#include "MediaInfo/TimeCode.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#if MEDIAINFO_EVENTS
    #include "MediaInfo/MediaInfo_Config_PerPackage.h"
    #include "MediaInfo/MediaInfo_Events.h"
    #include "MediaInfo/MediaInfo_Events_Internal.h"
#endif //MEDIAINFO_EVENTS
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Info
//***************************************************************************

//---------------------------------------------------------------------------
static const int8u Hevc_SubWidthC[]=
{
    1,
    2,
    2,
    1,
};

//---------------------------------------------------------------------------
static const int8u Hevc_SubHeightC[]=
{
    1,
    2,
    1,
    1,
};

//---------------------------------------------------------------------------
static const char* Hevc_chroma_format_idc(int8u chroma_format_idc)
{
    switch (chroma_format_idc)
    {
        case 1: return "4:2:0";
        case 2: return "4:2:2";
        case 3: return "4:4:4";
        default: return "";
    }
}

//---------------------------------------------------------------------------
static const char* Hevc_chroma_format_idc_ColorSpace(int8u chroma_format_idc)
{
    switch (chroma_format_idc)
    {
        case 0: return "Y";
        case 1: return "YUV";
        case 2: return "YUV";
        default: return "";
    }
}

//---------------------------------------------------------------------------
static const char* Hevc_pic_type[]=
{
    "I",
    "I, P",
    "I, P, B",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
};

//---------------------------------------------------------------------------
static const char* Hevc_slice_type(int32u slice_type)
{
    switch (slice_type)
    {
        case 0 : return "P";
        case 1 : return "B";
        case 2 : return "I";
        default: return "";
    }
};

//---------------------------------------------------------------------------
extern const char* Mpegv_colour_primaries(int8u colour_primaries);
extern const char* Mpegv_transfer_characteristics(int8u transfer_characteristics);
extern const char* Mpegv_matrix_coefficients(int8u matrix_coefficients);
const char* Mpegv_matrix_coefficients_ColorSpace(int8u matrix_coefficients);

//---------------------------------------------------------------------------
struct par
{
    int8u w;
    int8u h;
};
extern const par Avc_PixelAspectRatio[];
extern const size_t Avc_PixelAspectRatio_Size;
extern const char* Avc_video_format[];
extern const char* Avc_video_full_range[];

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Hevc::File_Hevc()
{
    //Config
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_Hevc;
        StreamIDs_Width[0]=0;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
    MustSynchronize=true;
    PTS_DTS_Needed=true;
    StreamSource=IsStream;
    Frame_Count_NotParsedIncluded=0;

    //In
    Frame_Count_Valid=0;
    FrameIsAlwaysComplete=false;
    MustParse_VPS_SPS_PPS=false;
    MustParse_VPS_SPS_PPS_FromMatroska=false;
    MustParse_VPS_SPS_PPS_FromFlv=false;
    MustParse_VPS_SPS_PPS_FromLhvc=false;
    SizedBlocks=false;
    SizedBlocks_FileThenStream=0;

    //File specific
    lengthSizeMinusOne=(int8u)-1;

    //Specific
    RiskCalculationN=0;
    RiskCalculationD=0;

    //Temporal references
    TemporalReferences_DelayedElement=NULL;
    TemporalReferences_Min=0;
    TemporalReferences_Max=0;
    TemporalReferences_Reserved=0;
    TemporalReferences_Offset=0;
    TemporalReferences_Offset_pic_order_cnt_lsb_Last=0;
    TemporalReferences_pic_order_cnt_Min=0;
    pic_order_cnt_DTS_Ref=(int64u)-1;

    //Text
    #if defined(MEDIAINFO_DTVCCTRANSPORT_YES)
        GA94_03_IsPresent=false;
        GA94_03_Parser=NULL;
    #endif //defined(MEDIAINFO_DTVCCTRANSPORT_YES)
}

//---------------------------------------------------------------------------
File_Hevc::~File_Hevc()
{
    Clean_Seq_Parameter();
    Clean_Temp_References();
    #if defined(MEDIAINFO_DTVCCTRANSPORT_YES)
        delete GA94_03_Parser; //GA94_03_Parser=NULL;
    #endif //defined(MEDIAINFO_DTVCCTRANSPORT_YES)
}
//---------------------------------------------------------------------------
void File_Hevc::Clean_Seq_Parameter()
{
    for (size_t Pos = 0; Pos < seq_parameter_sets.size(); Pos++)
        delete seq_parameter_sets[Pos]; //seq_parameter_sets[Pos]=NULL;
    seq_parameter_sets.clear();
    for (size_t Pos = 0; Pos<pic_parameter_sets.size(); Pos++)
        delete pic_parameter_sets[Pos]; //pic_parameter_sets[Pos]=NULL;
    pic_parameter_sets.clear();
    for (size_t Pos = 0; Pos<video_parameter_sets.size(); Pos++)
        delete video_parameter_sets[Pos]; //video_parameter_sets[Pos]=NULL;
    video_parameter_sets.clear();
    
}

//---------------------------------------------------------------------------
void File_Hevc::Clean_Temp_References()
{
    for (size_t Pos = 0; Pos<TemporalReferences.size(); Pos++)
        delete TemporalReferences[Pos]; //TemporalReferences[Pos]=NULL;
    TemporalReferences.clear();
    pic_order_cnt_DTS_Ref=(int64u)-1;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Hevc::Streams_Fill()
{
    if (MustParse_VPS_SPS_PPS_FromFlv)
        return;

    if (Count_Get(Stream_Video)==0)
        Stream_Prepare(Stream_Video);
    Fill(Stream_Video, 0, Video_Format, "HEVC");
    Fill(Stream_Video, 0, Video_Codec, "HEVC");

    for (std::vector<video_parameter_set_struct*>::iterator video_parameter_set_Item=video_parameter_sets.begin(); video_parameter_set_Item!=video_parameter_sets.end(); ++video_parameter_set_Item)
        if ((*video_parameter_set_Item))
            Streams_Fill(video_parameter_set_Item);

    for (std::vector<seq_parameter_set_struct*>::iterator seq_parameter_set_Item=seq_parameter_sets.begin(); seq_parameter_set_Item!=seq_parameter_sets.end(); ++seq_parameter_set_Item)
        if ((*seq_parameter_set_Item))
            Streams_Fill(seq_parameter_set_Item);

    //Library name
    Fill(Stream_General, 0, General_Encoded_Library, Encoded_Library);
    Fill(Stream_General, 0, General_Encoded_Library_Name, Encoded_Library_Name);
    Fill(Stream_General, 0, General_Encoded_Library_Version, Encoded_Library_Version);
    Fill(Stream_General, 0, General_Encoded_Library_Settings, Encoded_Library_Settings);
    Fill(Stream_Video, 0, Video_Encoded_Library, Encoded_Library);
    Fill(Stream_Video, 0, Video_Encoded_Library_Name, Encoded_Library_Name);
    Fill(Stream_Video, 0, Video_Encoded_Library_Version, Encoded_Library_Version);
    Fill(Stream_Video, 0, Video_Encoded_Library_Settings, Encoded_Library_Settings);

    //Merge info about different HDR formats
    auto HDR_Format=HDR.find(Video_HDR_Format);
    if (HDR_Format!=HDR.end())
    {
        bitset<HdrFormat_Max> HDR_Present;
        size_t HDR_FirstFormatPos=(size_t)-1;
        for (size_t i=0; i<HdrFormat_Max; i++)
            if (!HDR_Format->second[i].empty())
            {
                if (HDR_FirstFormatPos==(size_t)-1)
                    HDR_FirstFormatPos=i;
                HDR_Present[i]=true;
            }
        bool LegacyStreamDisplay=MediaInfoLib::Config.LegacyStreamDisplay_Get();
        for (const auto& HDR_Item: HDR)
        {
            size_t i=HDR_FirstFormatPos;
            size_t HDR_FirstFieldNonEmpty=(size_t)-1;
            if (HDR_Item.first>Video_HDR_Format_Compatibility)
            {
                for (; i<HdrFormat_Max; i++)
                {
                    if (!HDR_Present[i])
                        continue;
                    if (HDR_FirstFieldNonEmpty==(size_t)-1 && !HDR_Item.second[i].empty())
                        HDR_FirstFieldNonEmpty=i;
                    if (!HDR_Item.second[i].empty() && HDR_Item.second[i]!=HDR_Item.second[HDR_FirstFieldNonEmpty])
                        break;
                }
            }
            if (i==HdrFormat_Max && HDR_FirstFieldNonEmpty!=(size_t)-1)
                Fill(Stream_Video, 0, HDR_Item.first, HDR_Item.second[HDR_FirstFieldNonEmpty]);
            else
            {
                ZtringList Value;
                Value.Separator_Set(0, __T(" / "));
                if (i!=HdrFormat_Max)
                    for (i=HDR_FirstFormatPos; i<HdrFormat_Max; i++)
                    {
                        if (!LegacyStreamDisplay && HDR_FirstFormatPos!=HdrFormat_SmpteSt2086 && i>=HdrFormat_SmpteSt2086)
                            break;
                        if (!HDR_Present[i])
                            continue;
                        Value.push_back(HDR_Item.second[i]);
                    }
                auto Value_Flat = Value.Read();
                if (!Value.empty() && Value_Flat.size() > (Value.size() - 1) * 3)
                    Fill(Stream_Video, 0, HDR_Item.first, Value.Read());
            }
        }
    }
    if (!EtsiTS103433.empty())
    {
        Fill(Stream_Video, 0, "EtsiTS103433", EtsiTS103433);
        Fill_SetOptions(Stream_Video, 0, "EtsiTS103433", "N NTN");
    }
    if (!maximum_content_light_level.empty())
        Fill(Stream_Video, 0, Video_MaxCLL, maximum_content_light_level);
    if (!maximum_frame_average_light_level.empty())
        Fill(Stream_Video, 0, Video_MaxFALL, maximum_frame_average_light_level);
    if (chroma_sample_loc_type_top_field != (int32u)-1)
    {
    Fill(Stream_Video, 0, "ChromaSubsampling_Position", __T("Type ") + Ztring::ToZtring(chroma_sample_loc_type_top_field));
    if (chroma_sample_loc_type_bottom_field != (int32u)-1 && chroma_sample_loc_type_bottom_field != chroma_sample_loc_type_top_field)
        Fill(Stream_Video, 0, "ChromaSubsampling_Position", __T("Type ") + Ztring::ToZtring(chroma_sample_loc_type_bottom_field));
    }
}

//---------------------------------------------------------------------------
void File_Hevc::Streams_Fill_Profile(const profile_tier_level_struct& p)
{
    bool LegacyStreamDisplay=MediaInfoLib::Config.LegacyStreamDisplay_Get();
    if (!LegacyStreamDisplay && !Retrieve_Const(Stream_Video, 0, Video_Format_Profile).empty())
        return;

    Ztring Profile;
    if (p.profile_space==0)
    {
        if (p.profile_idc)
        {
            Profile=Ztring().From_UTF8(Hevc_profile_idc(p.profile_idc));
            int8u Profile_Addition_Bits=0;
            switch (p.profile_idc)
            {
                case 6 :
                case 7 :
                    if (!p.general_max_8bit_constraint_flag)
                    {
                        if (!p.general_max_10bit_constraint_flag)
                        {
                            if (!p.general_max_12bit_constraint_flag)
                            {
                                if (!p.general_max_14bit_constraint_flag)
                                    Profile_Addition_Bits=4;
                                else
                                    Profile_Addition_Bits=3;
                            }
                            else
                                Profile_Addition_Bits=2;
                        }
                        else
                            Profile_Addition_Bits=1;
                    }
            }
            if (Profile_Addition_Bits)
            {
                Profile+=' ';
                Profile+=Ztring::ToZtring(8+Profile_Addition_Bits*2);
            }
        }
        if (p.level_idc)
        {
            if (p.profile_idc)
                Profile+=__T('@');
            Profile+=__T('L')+Ztring().From_Number(((float)p.level_idc)/30, (p.level_idc%10)?1:0);
            Profile+=__T('@');
            Profile+=Ztring().From_UTF8(Hevc_tier_flag(p.tier_flag));
        }
    }
    Fill(Stream_Video, 0, Video_Format_Profile, Profile);
    Fill(Stream_Video, 0, Video_Codec_Profile, Profile);
}

//---------------------------------------------------------------------------
void File_Hevc::Streams_Fill(std::vector<video_parameter_set_struct*>::iterator video_parameter_set_Item)
{
    if ((*video_parameter_set_Item)->profile_tier_level_info_layers.size()==1)
        return; //Priority on seq_parameter_set

    const auto&p=(*video_parameter_set_Item)->profile_tier_level_info_layers.back();
    Streams_Fill_Profile(p);
    if (!(*video_parameter_set_Item)->view_id_val.empty())
    {
        size_t Count=0;
        for (const auto val : (*video_parameter_set_Item)->view_id_val)
            if (val!=(int16u)-1)
                Count++;
        Fill(Stream_Video, 0, Video_MultiView_Count, Count);
    }
}

//---------------------------------------------------------------------------
void File_Hevc::Streams_Fill(std::vector<seq_parameter_set_struct*>::iterator seq_parameter_set_Item_)
{
    auto seq_parameter_set_Item = *seq_parameter_set_Item_;
    if (seq_parameter_set_Item->nuh_layer_id)
        return;

    int32u Width = seq_parameter_set_Item->pic_width_in_luma_samples;
    int32u Height= seq_parameter_set_Item->pic_height_in_luma_samples;
    int8u chromaArrayType = seq_parameter_set_Item->ChromaArrayType();
    if (chromaArrayType >= 4)
        chromaArrayType = 0;
    int32u CropUnitX=Hevc_SubWidthC [chromaArrayType];
    int32u CropUnitY=Hevc_SubHeightC[chromaArrayType];
    Width -=(seq_parameter_set_Item->conf_win_left_offset+seq_parameter_set_Item->conf_win_right_offset)*CropUnitX;
    Height-=(seq_parameter_set_Item->conf_win_top_offset +seq_parameter_set_Item->conf_win_bottom_offset)*CropUnitY;

    const auto& p=seq_parameter_set_Item->profile_tier_level_info;
    Streams_Fill_Profile(p);
    Fill(Stream_Video, StreamPos_Last, Video_Width, Width);
    Fill(Stream_Video, StreamPos_Last, Video_Height, Height);
    if (seq_parameter_set_Item->conf_win_left_offset || seq_parameter_set_Item->conf_win_right_offset)
        Fill(Stream_Video, StreamPos_Last, Video_Stored_Width, seq_parameter_set_Item->pic_width_in_luma_samples);
    if (seq_parameter_set_Item->conf_win_top_offset || seq_parameter_set_Item->conf_win_bottom_offset)
        Fill(Stream_Video, StreamPos_Last, Video_Stored_Height, seq_parameter_set_Item->pic_height_in_luma_samples);

    Fill(Stream_Video, 0, Video_ColorSpace, Hevc_chroma_format_idc_ColorSpace(seq_parameter_set_Item->chroma_format_idc));
    Fill(Stream_Video, 0, Video_ChromaSubsampling, Hevc_chroma_format_idc(seq_parameter_set_Item->chroma_format_idc));
    if (seq_parameter_set_Item->bit_depth_luma_minus8==seq_parameter_set_Item->bit_depth_chroma_minus8)
        Fill(Stream_Video, 0, Video_BitDepth, seq_parameter_set_Item->bit_depth_luma_minus8+8);

    if (preferred_transfer_characteristics!=2)
        Fill(Stream_Video, 0, Video_transfer_characteristics, Mpegv_transfer_characteristics(preferred_transfer_characteristics));
    const auto vui_parameters = seq_parameter_set_Item->vui_parameters;
    if (vui_parameters)
    {
        if (vui_parameters->time_scale && vui_parameters->num_units_in_tick)
            Fill(Stream_Video, StreamPos_Last, Video_FrameRate, (float32)vui_parameters->time_scale / vui_parameters->num_units_in_tick);

        if (vui_parameters->sar_width && vui_parameters->sar_height)
        {
            auto PixelAspectRatio = ((float32)vui_parameters->sar_width) / vui_parameters->sar_height;
            Fill(Stream_Video, 0, Video_PixelAspectRatio, PixelAspectRatio, 3, true);
            if (Width && Height)
                Fill(Stream_Video, 0, Video_DisplayAspectRatio, Width*PixelAspectRatio/Height, 3, true); //More precise
        }

        if (vui_parameters->flags[video_signal_type_present_flag])
        {
            Fill(Stream_Video, 0, Video_Standard, Avc_video_format[vui_parameters->video_format]);
            Fill(Stream_Video, 0, Video_colour_range, Avc_video_full_range[vui_parameters->flags[video_full_range_flag]]);
            if (vui_parameters->flags[colour_description_present_flag])
            {
                Fill(Stream_Video, 0, Video_colour_description_present, "Yes");
                Fill(Stream_Video, 0, Video_colour_primaries, Mpegv_colour_primaries(vui_parameters->colour_primaries));
                Fill(Stream_Video, 0, Video_transfer_characteristics, Mpegv_transfer_characteristics(vui_parameters->transfer_characteristics));
                Fill(Stream_Video, 0, Video_matrix_coefficients, Mpegv_matrix_coefficients(vui_parameters->matrix_coefficients));
                if (vui_parameters->matrix_coefficients!=2)
                    Fill(Stream_Video, 0, Video_ColorSpace, Mpegv_matrix_coefficients_ColorSpace(vui_parameters->matrix_coefficients), Unlimited, true, true);
            }
        }
    }
}

//---------------------------------------------------------------------------
void File_Hevc::Streams_Finish()
{
    //GA94 captions
    #if defined(MEDIAINFO_DTVCCTRANSPORT_YES)
        if (GA94_03_Parser && GA94_03_Parser->Status[IsAccepted])
        {
            Clear(Stream_Text);

            Finish(GA94_03_Parser);
            Merge(*GA94_03_Parser);

            Ztring LawRating=GA94_03_Parser->Retrieve(Stream_General, 0, General_LawRating);
            if (!LawRating.empty())
                Fill(Stream_General, 0, General_LawRating, LawRating, true);
            Ztring Title=GA94_03_Parser->Retrieve(Stream_General, 0, General_Title);
            if (!Title.empty() && Retrieve(Stream_General, 0, General_Title).empty())
                Fill(Stream_General, 0, General_Title, Title);

            for (size_t Pos=0; Pos<Count_Get(Stream_Text); Pos++)
            {
                Ztring MuxingMode=Retrieve(Stream_Text, Pos, "MuxingMode");
                Fill(Stream_Text, Pos, "MuxingMode", __T("SCTE 128 / ")+MuxingMode, true);
            }
        }
    #endif //defined(MEDIAINFO_DTVCCTRANSPORT_YES)
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Hevc::FileHeader_Begin()
{
    if (!File__Analyze::FileHeader_Begin_0x000001())
        return false;

    if (!MustSynchronize)
    {
        Synched_Init();
        Buffer_TotalBytes_FirstSynched=0;
        File_Offset_FirstSynched=File_Offset;
    }

    //All should be OK
    return true;
}

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Hevc::Synchronize()
{
    //Synchronizing
    size_t Buffer_Offset_Min=Buffer_Offset;
    while(Buffer_Offset+4<=Buffer_Size && (Buffer[Buffer_Offset  ]!=0x00
                                        || Buffer[Buffer_Offset+1]!=0x00
                                        || Buffer[Buffer_Offset+2]!=0x01))
    {
        Buffer_Offset+=2;
        while(Buffer_Offset<Buffer_Size && Buffer[Buffer_Offset]!=0x00)
            Buffer_Offset+=2;
        if (Buffer_Offset>=Buffer_Size || Buffer[Buffer_Offset-1]==0x00)
            Buffer_Offset--;
    }
    if (Buffer_Offset>Buffer_Offset_Min && Buffer[Buffer_Offset-1]==0x00)
        Buffer_Offset--;

    //Parsing last bytes if needed
    if (Buffer_Offset+4==Buffer_Size && (Buffer[Buffer_Offset  ]!=0x00
                                      || Buffer[Buffer_Offset+1]!=0x00
                                      || Buffer[Buffer_Offset+2]!=0x00
                                      || Buffer[Buffer_Offset+3]!=0x01))
        Buffer_Offset++;
    if (Buffer_Offset+3==Buffer_Size && (Buffer[Buffer_Offset  ]!=0x00
                                      || Buffer[Buffer_Offset+1]!=0x00
                                      || Buffer[Buffer_Offset+2]!=0x01))
        Buffer_Offset++;
    if (Buffer_Offset+2==Buffer_Size && (Buffer[Buffer_Offset  ]!=0x00
                                      || Buffer[Buffer_Offset+1]!=0x00))
        Buffer_Offset++;
    if (Buffer_Offset+1==Buffer_Size &&  Buffer[Buffer_Offset  ]!=0x00)
        Buffer_Offset++;

    if (Buffer_Offset+4>Buffer_Size)
        return false;

    if (File_Offset==0 && Buffer_Offset==0 && (Buffer[3]==0xE0 || Buffer[3]==0xFE))
    {
        //It is from MPEG-PS
        Reject();
        return false;
    }

    //Synched is OK
    Synched=true;
    return true;
}

//---------------------------------------------------------------------------
bool File_Hevc::Synched_Test()
{
    //Must have enough buffer for having header
    if (Buffer_Offset+6>Buffer_Size)
        return false;

    //Quick test of synchro
    if (Buffer[Buffer_Offset  ]!=0x00
     || Buffer[Buffer_Offset+1]!=0x00
     || (Buffer[Buffer_Offset+2]!=0x01 && (Buffer[Buffer_Offset+2]!=0x00 || Buffer[Buffer_Offset+3]!=0x01)))
    {
        Synched=false;
        return true;
    }

    //Quick search
    if (!Header_Parser_QuickSearch())
        return false;

    //We continue
    return true;
}

//***************************************************************************
// Buffer - Demux
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_DEMUX
bool File_Hevc::Demux_UnpacketizeContainer_Test()
{
    const int8u*    Buffer_Temp=NULL;
    size_t          Buffer_Temp_Size=0;
    bool            RandomAccess=true; //Default, in case of problem

    if ((MustParse_VPS_SPS_PPS || SizedBlocks) && Demux_Transcode_Iso14496_15_to_AnnexB)
    {
        if (MustParse_VPS_SPS_PPS)
            return true; //Wait for SPS and PPS

        //Random access check
        RandomAccess=false;

        //Computing final size
        size_t TranscodedBuffer_Size=0;
        size_t Buffer_Offset_Save=Buffer_Offset;
        while (Buffer_Offset+lengthSizeMinusOne+1+1<=Buffer_Size)
        {
            size_t Size;
            if (Buffer_Offset+lengthSizeMinusOne>Buffer_Size)
            {
                Size=0;
                Buffer_Offset=Buffer_Size;
            }
            else
            switch (lengthSizeMinusOne)
            {
                case 0: Size=Buffer[Buffer_Offset];
                        TranscodedBuffer_Size+=2;
                        break;
                case 1: Size=BigEndian2int16u(Buffer+Buffer_Offset);
                        TranscodedBuffer_Size++;
                        break;
                case 2: Size=BigEndian2int24u(Buffer+Buffer_Offset);
                        break;
                case 3: Size=BigEndian2int32u(Buffer+Buffer_Offset);
                        TranscodedBuffer_Size--;
                        break;
                default:    return true; //Problem
            }
            Size+=lengthSizeMinusOne+1;

            //Coherency checking
            if (Size<lengthSizeMinusOne+1+2 || Buffer_Offset+Size>Buffer_Size || (Buffer_Offset+Size!=Buffer_Size && Buffer_Offset+Size+lengthSizeMinusOne+1>Buffer_Size))
                Size=Buffer_Size-Buffer_Offset;
            size_t Buffer_Offset_Temp=Buffer_Offset+lengthSizeMinusOne+1;

            //In case there are more than 1 NAL in the block (in Stream format), trying to find the first NAL being a slice
            size_t Buffer_Offset_Temp_Max=Buffer_Offset+Size;
            if (!RandomAccess && Buffer_Offset_Temp<Buffer_Offset_Temp_Max && (Buffer[Buffer_Offset_Temp]&0x40)!=0) //Is not a slice
            {
                while (Buffer_Offset_Temp+3<=Buffer_Offset+Size)
                {
                    if (CC3(Buffer+Buffer_Offset_Temp)==0x000001)
                    {
                        if (!RandomAccess && Buffer_Offset+Buffer_Offset_Temp+3<Buffer_Size && (Buffer[Buffer_Offset+Buffer_Offset_Temp+3]&0x40)==0) //Is a slice
                        {
                            Buffer_Offset_Temp+=3;
                            break;
                        }
                    }
                    Buffer_Offset_Temp+=2;
                    while (Buffer_Offset_Temp<Buffer_Offset_Temp_Max && Buffer[Buffer_Offset_Temp]!=0x00)
                        Buffer_Offset_Temp+=2;
                    if (Buffer_Offset_Temp>=Buffer_Offset_Temp_Max || Buffer[Buffer_Offset_Temp]==0x00)
                        Buffer_Offset_Temp--;
                }
            }

            //Random access check
            if (!RandomAccess && Buffer_Offset_Temp+2<Buffer_Size && (Buffer[Buffer_Offset_Temp]&0x40)==0) //Is a slice
            {
                Element_Offset=Buffer_Offset_Temp+2-Buffer_Offset;
                Element_Size=Size-Buffer_Offset_Temp;

                BS_Begin();
                Get_SB (   first_slice_segment_in_pic_flag,                 "first_slice_segment_in_pic_flag");
                if (first_slice_segment_in_pic_flag)
                {
                    Element_Code=(Buffer[Buffer_Offset_Temp]&0x3E)>>1; //nal_unit_type
                    RapPicFlag=Element_Code>=16 && Element_Code<=23;
                    if (RapPicFlag)
                        Skip_SB(                                                "no_output_of_prior_pics_flag");
                    Get_UE (   slice_pic_parameter_set_id,                      "slice_pic_parameter_set_id");
                    int8u num_extra_slice_header_bits=(int8u)-1;
                    std::vector<pic_parameter_set_struct*>::iterator pic_parameter_set_Item;
                    if (MustParse_VPS_SPS_PPS_FromFlv)
                        num_extra_slice_header_bits=0; // We bet it is old, so without num_extra_slice_header_bits
                    else if (!(slice_pic_parameter_set_id>=pic_parameter_sets.size() || (*(pic_parameter_set_Item=pic_parameter_sets.begin()+slice_pic_parameter_set_id))==NULL))
                    {
                        num_extra_slice_header_bits=(*pic_parameter_set_Item)->num_extra_slice_header_bits;
                    }
                    if (num_extra_slice_header_bits!=(int8u)-1)
                    {
                        int32u slice_type;
                        Skip_S1(num_extra_slice_header_bits,                    "slice_reserved_flags");
                        Get_UE (slice_type,                                     "slice_type");

                        switch (slice_type)
                        {
                            case 2 :
                            case 7 :
                                        RandomAccess=true;
                        }
                    }
                }
                BS_End();
            }

            TranscodedBuffer_Size+=Size;
            Buffer_Offset+=Size;
        }
        Buffer_Offset=Buffer_Offset_Save;

        //Adding VPS/SPS/PPS sizes
        if (RandomAccess)
        {
            for (video_parameter_set_structs::iterator Data_Item=video_parameter_sets.begin(); Data_Item!=video_parameter_sets.end(); ++Data_Item)
                TranscodedBuffer_Size+=(*Data_Item)->AnnexB_Buffer_Size;
            for (seq_parameter_set_structs::iterator Data_Item=seq_parameter_sets.begin(); Data_Item!=seq_parameter_sets.end(); ++Data_Item)
                TranscodedBuffer_Size+=(*Data_Item)->AnnexB_Buffer_Size;
            for (pic_parameter_set_structs::iterator Data_Item=pic_parameter_sets.begin(); Data_Item!=pic_parameter_sets.end(); ++Data_Item)
                TranscodedBuffer_Size+=(*Data_Item)->AnnexB_Buffer_Size;
        }

        //Copying
        int8u* TranscodedBuffer=new int8u[TranscodedBuffer_Size+100];
        size_t TranscodedBuffer_Pos=0;
        if (RandomAccess)
        {
            for (video_parameter_set_structs::iterator Data_Item=video_parameter_sets.begin(); Data_Item!=video_parameter_sets.end(); ++Data_Item)
            {
                std::memcpy(TranscodedBuffer+TranscodedBuffer_Pos, (*Data_Item)->AnnexB_Buffer, (*Data_Item)->AnnexB_Buffer_Size);
                TranscodedBuffer_Pos+=(*Data_Item)->AnnexB_Buffer_Size;
            }
            for (seq_parameter_set_structs::iterator Data_Item=seq_parameter_sets.begin(); Data_Item!=seq_parameter_sets.end(); ++Data_Item)
            {
                std::memcpy(TranscodedBuffer+TranscodedBuffer_Pos, (*Data_Item)->AnnexB_Buffer, (*Data_Item)->AnnexB_Buffer_Size);
                TranscodedBuffer_Pos+=(*Data_Item)->AnnexB_Buffer_Size;
            }
            for (pic_parameter_set_structs::iterator Data_Item=pic_parameter_sets.begin(); Data_Item!=pic_parameter_sets.end(); ++Data_Item)
            {
                std::memcpy(TranscodedBuffer+TranscodedBuffer_Pos, (*Data_Item)->AnnexB_Buffer, (*Data_Item)->AnnexB_Buffer_Size);
                TranscodedBuffer_Pos+=(*Data_Item)->AnnexB_Buffer_Size;
            }
        }
        while (Buffer_Offset<Buffer_Size)
        {
            //Sync layer
            TranscodedBuffer[TranscodedBuffer_Pos]=0x00;
            TranscodedBuffer_Pos++;
            TranscodedBuffer[TranscodedBuffer_Pos]=0x00;
            TranscodedBuffer_Pos++;
            TranscodedBuffer[TranscodedBuffer_Pos]=0x01;
            TranscodedBuffer_Pos++;

            //Block
            size_t Size;
            switch (lengthSizeMinusOne)
            {
                case 0: Size=Buffer[Buffer_Offset];
                        Buffer_Offset++;
                        break;
                case 1: Size=BigEndian2int16u(Buffer+Buffer_Offset);
                        Buffer_Offset+=2;
                        break;
                case 2: Size=BigEndian2int24u(Buffer+Buffer_Offset);
                        Buffer_Offset+=3;
                        break;
                case 3: Size=BigEndian2int32u(Buffer+Buffer_Offset);
                        Buffer_Offset+=4;
                        break;
                default: //Problem
                        delete [] TranscodedBuffer;
                        return false;
            }

            //Coherency checking
            if (Size==0 || Buffer_Offset+Size>Buffer_Size || (Buffer_Offset+Size!=Buffer_Size && Buffer_Offset+Size+lengthSizeMinusOne+1>Buffer_Size))
                Size=Buffer_Size-Buffer_Offset;

            std::memcpy(TranscodedBuffer+TranscodedBuffer_Pos, Buffer+Buffer_Offset, Size);
            TranscodedBuffer_Pos+=Size;
            Buffer_Offset+=Size;
        }
        Buffer_Offset=0;

        Buffer_Temp=Buffer;
        Buffer=TranscodedBuffer;
        Buffer_Temp_Size=Buffer_Size;
        Buffer_Size=TranscodedBuffer_Size;
        Demux_Offset=Buffer_Size;
    }
    else
    {
        bool zero_byte=Buffer[Buffer_Offset+2]==0x00;
        if (!(((Buffer[Buffer_Offset+(zero_byte?4:3)]&0x40)==0 && (Buffer[Buffer_Offset+(zero_byte?6:5)]&0x80)!=0x80)
           || (Buffer[Buffer_Offset+(zero_byte?4:3)]&0x7E)==(38<<1)))
        {
            if (Demux_Offset==0)
            {
                Demux_Offset=Buffer_Offset;
                Demux_IntermediateItemFound=false;
            }
            while (Demux_Offset+6<=Buffer_Size)
            {
                //Synchronizing
                while(Demux_Offset+6<=Buffer_Size && (Buffer[Demux_Offset  ]!=0x00
                                                   || Buffer[Demux_Offset+1]!=0x00
                                                   || Buffer[Demux_Offset+2]!=0x01))
                {
                    Demux_Offset+=2;
                    while(Demux_Offset<Buffer_Size && Buffer[Buffer_Offset]!=0x00)
                        Demux_Offset+=2;
                    if (Demux_Offset>=Buffer_Size || Buffer[Demux_Offset-1]==0x00)
                        Demux_Offset--;
                }

                if (Demux_Offset+6>Buffer_Size)
                {
                    if (Config->IsFinishing)
                        Demux_Offset=Buffer_Size;
                    break;
                }

                zero_byte=Buffer[Demux_Offset+2]==0x00;
                int8u nal_unit_type=Buffer[Demux_Offset+(zero_byte?4:3)]>>1;
                bool Next;
                switch (nal_unit_type)
                {
                    case  0 :
                    case  1 :
                    case  2 :
                    case  3 :
                    case  4 :
                    case  5 :
                    case  6 :
                    case  7 :
                    case  8 :
                    case  9 :
                    case 16 :
                    case 17 :
                    case 18 :
                    case 19 :
                    case 20 :
                    case 21 :
                                if (Demux_IntermediateItemFound)
                                {
                                    if (Buffer[Demux_Offset+(zero_byte?6:5)]&0x80)
                                        Next=true;
                                    else
                                        Next=false;
                                }
                                else
                                {
                                    Next=false;
                                    Demux_IntermediateItemFound=true;
                                }
                              break;
                    case 32 :
                    case 33 :
                    case 34 :
                    case 35 :
                              if (Demux_IntermediateItemFound)
                                  Next=true;
                              else
                                  Next=false;
                              break;
                    default : Next=false;
                }

                if (Next)
                {
                    Demux_IntermediateItemFound=false;
                    break;
                }

                Demux_Offset++;
            }

            if (Demux_Offset+6>Buffer_Size && !FrameIsAlwaysComplete && !Config->IsFinishing)
                return false; //No complete frame

            if (Demux_Offset && Buffer[Demux_Offset-1]==0x00)
                Demux_Offset--;

            zero_byte=Buffer[Buffer_Offset+2]==0x00;
            size_t Buffer_Offset_Random=Buffer_Offset;
            if ((Buffer[Buffer_Offset_Random+(zero_byte?4:3)]&0x7E)==(35<<1))
            {
                Buffer_Offset_Random++;
                if (zero_byte)
                    Buffer_Offset_Random++;
                while(Buffer_Offset_Random+6<=Buffer_Size && (Buffer[Buffer_Offset_Random  ]!=0x00
                                                           || Buffer[Buffer_Offset_Random+1]!=0x00
                                                           || Buffer[Buffer_Offset_Random+2]!=0x01))
                    Buffer_Offset_Random++;
                zero_byte=Buffer[Buffer_Offset_Random+2]==0x00;
            }
            RandomAccess=Buffer_Offset_Random+6<=Buffer_Size && (Buffer[Buffer_Offset_Random+(zero_byte?4:3)]&0x7E)==(32<<1); //video_parameter_set
        }
    }

    if (!Status[IsAccepted])
    {
        if (Config->Demux_EventWasSent)
            return false;
        File_Hevc* MI=new File_Hevc;
        Element_Code=(int64u)-1;
        Open_Buffer_Init(MI);
        #ifdef MEDIAINFO_EVENTS
            MediaInfo_Config_PerPackage* Config_PerPackage_Temp=MI->Config->Config_PerPackage;
            MI->Config->Config_PerPackage=NULL;
        #endif //MEDIAINFO_EVENTS
        Open_Buffer_Continue(MI, Buffer, Buffer_Size);
        #ifdef MEDIAINFO_EVENTS
            MI->Config->Config_PerPackage=Config_PerPackage_Temp;
        #endif //MEDIAINFO_EVENTS
        bool IsOk=MI->Status[IsAccepted];
        delete MI;
        if (!IsOk)
            return false;
    }

    if (IFrame_Count || RandomAccess)
    {
        int64u PTS_Temp=FrameInfo.PTS;
        if (!IsSub)
            FrameInfo.PTS=(int64u)-1;
        Demux_UnpacketizeContainer_Demux(RandomAccess);
        if (!IsSub)
            FrameInfo.PTS=PTS_Temp;
    }
    else
        Demux_UnpacketizeContainer_Demux_Clear();

    if (Buffer_Temp)
    {
        Demux_TotalBytes-=Buffer_Size;
        Demux_TotalBytes+=Buffer_Temp_Size;
        delete[] Buffer;
        Buffer=Buffer_Temp;
        Buffer_Size=Buffer_Temp_Size;
    }

    return true;
}
#endif //MEDIAINFO_DEMUX

//---------------------------------------------------------------------------
void File_Hevc::Synched_Init()
{
    if (!Frame_Count_Valid)
        Frame_Count_Valid=Config->ParseSpeed>=0.3?16:(IsSub?1:2); //Note: should be replaced by "512:2" when I-frame/GOP detection is OK

    //FrameInfo
    PTS_End=0;
    if (!IsSub)
        FrameInfo.DTS=0; //No DTS in container
    DTS_Begin=FrameInfo.DTS;
    DTS_End=FrameInfo.DTS;

    //Status
    IFrame_Count=0;

    //Temp
    chroma_sample_loc_type_top_field=(int32u)-1;
    chroma_sample_loc_type_bottom_field=(int32u)-1;
    preferred_transfer_characteristics=2;
    chroma_format_idc=0;

    //Default values
    Streams.resize(0x100);
    Streams[32].Searching_Payload=true; //video_parameter_set
    Streams[35].Searching_Payload=true; //access_unit_delimiter
    Streams[39].Searching_Payload=true; //sei
    for (int8u Pos=0xFF; Pos>=48; Pos--)
        Streams[Pos].Searching_Payload=true; //unspecified

    #if MEDIAINFO_DEMUX
        Demux_Transcode_Iso14496_15_to_AnnexB=Config->Demux_Hevc_Transcode_Iso14496_15_to_AnnexB_Get();
    #endif //MEDIAINFO_DEMUX
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Hevc::Read_Buffer_Unsynched()
{
    //Impossible to know TimeStamps now
    PTS_End=0;
    DTS_End=0;

    //Temporal references
    Clean_Temp_References();
    delete TemporalReferences_DelayedElement; TemporalReferences_DelayedElement=NULL;
    TemporalReferences_Min=0;
    TemporalReferences_Max=0;
    TemporalReferences_Reserved=0;
    TemporalReferences_Offset=0;
    TemporalReferences_Offset_pic_order_cnt_lsb_Last=0;
    TemporalReferences_pic_order_cnt_Min=0;
    pic_order_cnt_DTS_Ref=(int64u)-1;

    //Text
    #if defined(MEDIAINFO_DTVCCTRANSPORT_YES)
        if (GA94_03_Parser)
            GA94_03_Parser->Open_Buffer_Unsynch();
    #endif //defined(MEDIAINFO_DTVCCTRANSPORT_YES)
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Hevc::Header_Parse()
{
    //Specific case
    if (MustParse_VPS_SPS_PPS)
    {
        Header_Fill_Size(Element_Size);
        Header_Fill_Code((int64u)-1, "Specific");
        return;
    }

    //Parsing
    int8u nal_unit_type, nuh_temporal_id_plus1;
    if (!SizedBlocks || SizedBlocks_FileThenStream)
    {
        if (Buffer[Buffer_Offset+2]==0x00)
            Skip_B1(                                            "zero_byte");
        Skip_B3(                                                "start_code_prefix_one_3bytes");
        BS_Begin();
        Mark_0 ();
        Get_S1 (6, nal_unit_type,                               "nal_unit_type");
        Get_S1 (6, nuh_layer_id,                                "nuh_layer_id");
        Get_S1 (3, nuh_temporal_id_plus1,                       "nuh_temporal_id_plus1");
        BS_End();

        if (!Header_Parser_Fill_Size())
        {
            Element_WaitForMoreData();
            return;
        }

        //if (nuh_temporal_id_plus1==0) // Found 1 stream with nuh_temporal_id_plus1==0, lets disable this coherency test for the moment
        //    Trusted_IsNot("nuh_temporal_id_plus1");

        //In case there are more than 1 NAL in the block (in Stream format), trying to find the first NAL being a slice
        if (SizedBlocks_FileThenStream && Element[Element_Level-1].Next>=SizedBlocks_FileThenStream)
        {
            if (Element[Element_Level-1].Next>SizedBlocks_FileThenStream)
                Header_Fill_Size(SizedBlocks_FileThenStream-(File_Offset+Buffer_Offset));
            SizedBlocks_FileThenStream=0; //TODO: more integrity tests, handling of first element size of first element (wrong in the trace)
        }
    }
    else
    {
        int32u Size;
        switch (lengthSizeMinusOne)
        {
            case 0: {
                        int8u Size_;
                        Get_B1 (Size_,                          "size");
                        Size=Size_;
                    }
                    break;
            case 1: {
                        int16u Size_;
                        Get_B2 (Size_,                          "size");
                        Size=Size_;
                    }
                    break;
            case 2: {
                        int32u Size_;
                        Get_B3 (Size_,                          "size");
                        Size=Size_;
                    }
                    break;
            case 3:     Get_B4 (Size,                           "size");
                    break;
            default:    Trusted_IsNot("No size of NALU defined");
                        Header_Fill_Size(Buffer_Size-Buffer_Offset);
                        return;
        }
        if (Element_Size<(int64u)lengthSizeMinusOne+1+2 || Size>Element_Size-Element_Offset)
            return RanOutOfData("HEVC");

        //In case there are more than 1 NAL in the block (in Stream format), trying to find the first NAL being a slice
        size_t Buffer_Offset_Temp=Buffer_Offset+lengthSizeMinusOne+1;
        size_t Buffer_Offset_Temp_Max=Buffer_Offset+Size;
        while (Buffer_Offset_Temp+3<=Buffer_Offset+Size)
        {
            if (CC3(Buffer+Buffer_Offset_Temp)==0x000001 || CC3(Buffer+Buffer_Offset_Temp)==0x000000)
            {
                break;
            }
            Buffer_Offset_Temp+=2;
            while (Buffer_Offset_Temp<Buffer_Offset_Temp_Max && Buffer[Buffer_Offset_Temp]!=0x00)
                Buffer_Offset_Temp+=2;
            if (Buffer_Offset_Temp>=Buffer_Offset_Temp_Max || Buffer[Buffer_Offset_Temp]==0x00)
                Buffer_Offset_Temp--;
        }
        if (Buffer_Offset_Temp+3<=Buffer_Offset+Size)
        {
            SizedBlocks_FileThenStream=File_Offset+Buffer_Offset+Size;
            Size=Buffer_Offset_Temp-(Buffer_Offset+Element_Offset);
        }

        Header_Fill_Size(Element_Offset+Size);

        BS_Begin();
        Mark_0 ();
        Get_S1 (6, nal_unit_type,                               "nal_unit_type");
        Get_S1 (6, nuh_layer_id,                                "nuh_layer_id");
        Get_S1 (3, nuh_temporal_id_plus1,                       "nuh_temporal_id_plus1");
        BS_End();

        //if (nuh_temporal_id_plus1==0) // Found 1 stream with nuh_temporal_id_plus1==0, lets disable this coherency test for the moment
        //    Trusted_IsNot("nuh_temporal_id_plus1");
    }

    //Filling
    #if MEDIAINFO_TRACE
        if (Trace_Activated)
            Header_Fill_Code(nal_unit_type, Ztring().From_CC1(nal_unit_type));
        else
    #endif //MEDIAINFO_TRACE
            Header_Fill_Code(nal_unit_type);
}

//---------------------------------------------------------------------------
bool File_Hevc::Header_Parser_Fill_Size()
{
    //Look for next Sync word
    if (Buffer_Offset_Temp==0) //Buffer_Offset_Temp is not 0 if Header_Parse_Fill_Size() has already parsed first frames
        Buffer_Offset_Temp=Buffer_Offset+4;
    while (Buffer_Offset_Temp+5<=Buffer_Size
        && CC3(Buffer+Buffer_Offset_Temp)!=0x000001)
    {
        Buffer_Offset_Temp+=2;
        while(Buffer_Offset_Temp<Buffer_Size && Buffer[Buffer_Offset_Temp]!=0x00)
            Buffer_Offset_Temp+=2;
        if (Buffer_Offset_Temp>=Buffer_Size || Buffer[Buffer_Offset_Temp-1]==0x00)
            Buffer_Offset_Temp--;
    }

    //Must wait more data?
    if (Buffer_Offset_Temp+5>Buffer_Size)
    {
        if (FrameIsAlwaysComplete || Config->IsFinishing)
            Buffer_Offset_Temp=Buffer_Size; //We are sure that the next bytes are a start
        else
            return false;
    }

    if (Buffer[Buffer_Offset_Temp-1]==0x00)
        Buffer_Offset_Temp--;

    //OK, we continue
    Header_Fill_Size(Buffer_Offset_Temp-Buffer_Offset);
    Buffer_Offset_Temp=0;
    return true;
}

//---------------------------------------------------------------------------
bool File_Hevc::Header_Parser_QuickSearch()
{
    while (       Buffer_Offset+6<=Buffer_Size
      &&   Buffer[Buffer_Offset  ]==0x00
      &&   Buffer[Buffer_Offset+1]==0x00
      &&  (Buffer[Buffer_Offset+2]==0x01
        || (Buffer[Buffer_Offset+2]==0x00 && Buffer[Buffer_Offset+3]==0x01)))
    {
        //Getting start_code
        int8u nal_unit_type;
        if (Buffer[Buffer_Offset+2]==0x00)
            nal_unit_type=(CC1(Buffer+Buffer_Offset+4)&0x7E)>>1;
        else
            nal_unit_type=(CC1(Buffer+Buffer_Offset+3)&0x7E)>>1;

        //Searching start
        if (Streams[nal_unit_type].Searching_Payload)
            return true;

        //Synchronizing
        Buffer_Offset+=4;
        Synched=false;
        if (!Synchronize())
        {
            UnSynched_IsNotJunk=true;
            return false;
        }

        if (Buffer_Offset+6>Buffer_Size)
        {
            UnSynched_IsNotJunk=true;
            return false;
        }
    }

    Trusted_IsNot("HEVC, Synchronisation lost");
    return Synchronize();
}

//---------------------------------------------------------------------------
void File_Hevc::Data_Parse()
{
    //Specific case
    if (Element_Code==(int64u)-1)
    {
        VPS_SPS_PPS();
        return;
    }

    if (nuh_layer_id)
    {
        //Skip_XX(Element_Size,                                   "(Extension)");
        //return;
    }

    //Trailing zeroes
    int64u Element_Size_SaveBeforeZeroes=Element_Size;
    if (Element_Size)
    {
        while (Element_Size && Buffer[Buffer_Offset+(size_t)Element_Size-1]==0)
            Element_Size--;
    }

    //Searching emulation_prevention_three_byte
    int8u* Buffer_3Bytes=NULL;
    const int8u* Save_Buffer=Buffer;
    int64u Save_File_Offset=File_Offset;
    size_t Save_Buffer_Offset=Buffer_Offset;
    int64u Save_Element_Size=Element_Size;
    size_t Element_Offset_3Bytes=(size_t)Element_Offset;
    std::vector<size_t> ThreeByte_List;
    while (Element_Offset_3Bytes+3<=Element_Size)
    {
        if (CC3(Buffer+Buffer_Offset+(size_t)Element_Offset_3Bytes)==0x000003)
            ThreeByte_List.push_back(Element_Offset_3Bytes+2);
        Element_Offset_3Bytes+=2;
        while(Element_Offset_3Bytes<Element_Size && Buffer[Buffer_Offset+(size_t)Element_Offset_3Bytes]!=0x00)
            Element_Offset_3Bytes+=2;
        if (Element_Offset_3Bytes>=Element_Size || Buffer[Buffer_Offset+(size_t)Element_Offset_3Bytes-1]==0x00)
            Element_Offset_3Bytes--;
    }

    if (!ThreeByte_List.empty())
    {
        //We must change the buffer for keeping out
        Element_Size=Save_Element_Size-ThreeByte_List.size();
        File_Offset+=Buffer_Offset;
        Buffer_Offset=0;
        Buffer_3Bytes=new int8u[(size_t)Element_Size];
        for (size_t Pos=0; Pos<=ThreeByte_List.size(); Pos++)
        {
            size_t Pos0=(Pos==ThreeByte_List.size())?(size_t)Save_Element_Size:(ThreeByte_List[Pos]);
            size_t Pos1=(Pos==0)?0:(ThreeByte_List[Pos-1]+1);
            size_t Buffer_3bytes_Begin=Pos1-Pos;
            size_t Save_Buffer_Begin  =Pos1;
            size_t Size=               Pos0-Pos1;
            std::memcpy(Buffer_3Bytes+Buffer_3bytes_Begin, Save_Buffer+Save_Buffer_Offset+Save_Buffer_Begin, Size);
        }
        Buffer=Buffer_3Bytes;
    }

    //Parsing
    switch (Element_Code)
    {
        case  0 :
        case  1 :
        case  2 :
        case  3 :
        case  4:
        case  5:
        case  6:
        case  7:
        case  8:
        case  9:
        case 16 :
        case 17 :
        case 18 :
        case 19 :
        case 20 :
        case 21 :
                  slice_segment_layer(); break;
        case 32 : video_parameter_set(); break;
        case 33 : seq_parameter_set(); break;
        case 34 : pic_parameter_set(); break;
        case 35 : access_unit_delimiter(); break;
        case 36 : end_of_seq(); break;
        case 37 : end_of_bitstream(); break;
        case 38 : filler_data(); break;
        case 39 :
        case 40 :
                  sei(); break;
        default :
                  Skip_XX(Element_Size-Element_Offset, "Data");
                  if (Element_Code>=48)
                      Trusted_IsNot("Unspecified");
    }

    if (!ThreeByte_List.empty())
    {
        //We must change the buffer for keeping out
        Element_Size=Save_Element_Size;
        File_Offset=Save_File_Offset;
        Buffer_Offset=Save_Buffer_Offset;
        delete[] Buffer; Buffer=Save_Buffer;
        Buffer_3Bytes=NULL; //Same as Buffer...
        Element_Offset+=ThreeByte_List.size();
    }

    #if MEDIAINFO_DEMUX
        if (Demux_Transcode_Iso14496_15_to_AnnexB)
        {
            if (Element_Code==32)
            {
                std::vector<video_parameter_set_struct*>::iterator Data_Item=video_parameter_sets.begin();
                if (Data_Item!=video_parameter_sets.end() && (*Data_Item))
                {
                    delete[] (*Data_Item)->AnnexB_Buffer;
                    (*Data_Item)->AnnexB_Buffer_Size=(size_t)(Element_Size+5);
                    (*Data_Item)->AnnexB_Buffer=new int8u[(*Data_Item)->AnnexB_Buffer_Size];
                    (*Data_Item)->AnnexB_Buffer[0]=0x00;
                    (*Data_Item)->AnnexB_Buffer[1]=0x00;
                    (*Data_Item)->AnnexB_Buffer[2]=0x01;
                    (*Data_Item)->AnnexB_Buffer[3]=Buffer[Buffer_Offset-2];
                    (*Data_Item)->AnnexB_Buffer[4]=Buffer[Buffer_Offset-1];
                    std::memcpy((*Data_Item)->AnnexB_Buffer+5, Buffer+Buffer_Offset, (size_t)Element_Size);
                }
            }
            if (Element_Code==33)
            {
                std::vector<seq_parameter_set_struct*>::iterator Data_Item=seq_parameter_sets.begin();
                if (Data_Item!=seq_parameter_sets.end() && (*Data_Item))
                {
                    delete[] (*Data_Item)->AnnexB_Buffer;
                    (*Data_Item)->AnnexB_Buffer_Size=(size_t)(Element_Size+5);
                    (*Data_Item)->AnnexB_Buffer=new int8u[(*Data_Item)->AnnexB_Buffer_Size];
                    (*Data_Item)->AnnexB_Buffer[0]=0x00;
                    (*Data_Item)->AnnexB_Buffer[1]=0x00;
                    (*Data_Item)->AnnexB_Buffer[2]=0x01;
                    (*Data_Item)->AnnexB_Buffer[3]=Buffer[Buffer_Offset-2];
                    (*Data_Item)->AnnexB_Buffer[4]=Buffer[Buffer_Offset-1];
                    std::memcpy((*Data_Item)->AnnexB_Buffer+5, Buffer+Buffer_Offset, (size_t)Element_Size);
                }
            }
            if (Element_Code==34)
            {
                std::vector<pic_parameter_set_struct*>::iterator Data_Item=pic_parameter_sets.begin();
                if (Data_Item!=pic_parameter_sets.end() && (*Data_Item))
                {
                    delete[] (*Data_Item)->AnnexB_Buffer;
                    (*Data_Item)->AnnexB_Buffer_Size=(size_t)(Element_Size+5);
                    (*Data_Item)->AnnexB_Buffer=new int8u[(*Data_Item)->AnnexB_Buffer_Size];
                    (*Data_Item)->AnnexB_Buffer[0]=0x00;
                    (*Data_Item)->AnnexB_Buffer[1]=0x00;
                    (*Data_Item)->AnnexB_Buffer[2]=0x01;
                    (*Data_Item)->AnnexB_Buffer[3]=Buffer[Buffer_Offset-2];
                    (*Data_Item)->AnnexB_Buffer[4]=Buffer[Buffer_Offset-1];
                    std::memcpy((*Data_Item)->AnnexB_Buffer+5, Buffer+Buffer_Offset, (size_t)Element_Size);
                }
            }
        }
    #endif //MEDIAINFO_DEMUX

    //Trailing zeroes
    Element_Size=Element_Size_SaveBeforeZeroes;
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
// Packets
void File_Hevc::slice_segment_layer()
{
    #if MEDIAINFO_TRACE
        Element_Name("slice_segment_layer");
        switch (Element_Code)
        {
            case 0 :
            case 1 : break;
            case 2 :
            case 3 : Element_Info("TSA"); break;
            case 4:
            case 5: Element_Info("STSA"); break;
            case 6:
            case 7: Element_Info("RADL"); break;
            case 8:
            case 9: Element_Info("RASL"); break;
            case 16 :
            case 17 :
            case 18 : Element_Info("BLA"); break;
            case 19 :
            case 20 : Element_Info("IDR"); break;
            case 21 : Element_Info("CRA"); break;
            default: ;
        }
    #endif //MEDIAINFO_TRACE

    //Parsing
    RapPicFlag=Element_Code>=16 && Element_Code<=23;
    BS_Begin();
    slice_segment_header();
    BS_End();
    Skip_XX(Element_Size-Element_Offset,                        "(ToDo)");

    FILLING_BEGIN();
        if (slice_pic_parameter_set_id==(int32u)-1)
            return;

        //Count of I-Frames
        if (first_slice_segment_in_pic_flag && (Element_Code==19 || Element_Code==20))
            IFrame_Count++;

        if (first_slice_segment_in_pic_flag)
        {
            //Frame_Count
            if (Frame_Count_NotParsedIncluded<16 && TC_Current.IsSet())
            {
                TimeCode TC_Previous(Retrieve_Const(Stream_Video, 0, Video_TimeCode_FirstFrame).To_UTF8(), TC_Current.GetFramesMax());
                if (!TC_Previous.IsSet() || TC_Current<TC_Previous)
                    Fill(Stream_Video, 0, Video_TimeCode_FirstFrame, TC_Current.ToString(), true, true);
                TC_Current=TimeCode();
            }
            Frame_Count++;
            if (IFrame_Count && Frame_Count_NotParsedIncluded!=(int64u)-1)
                Frame_Count_NotParsedIncluded++;
            Frame_Count_InThisBlock++;

            //Filling only if not already done
            if (Frame_Count==1 && !Status[IsAccepted])
            {
                if (RiskCalculationD && RiskCalculationN*2>=RiskCalculationD) // Check if we trust or not the sync
                {
                    Reject("HEVC");
                    return;
                }
                Accept("HEVC");
            }
            if (!Status[IsFilled])
            {
                if (IFrame_Count>=8)
                    Frame_Count_Valid=Frame_Count; //We have enough frames
                if (Frame_Count>=Frame_Count_Valid)
                {
                    Fill("HEVC");
                    if (!IsSub && Config->ParseSpeed<1.0)
                        Finish("HEVC");
                }
            }
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Hevc::video_parameter_sets_creating_data(int8u vps_video_parameter_set_id, const vector<profile_tier_level_struct>& profile_tier_level_info_layers, int8u vps_max_sub_layers_minus1, const vector<int16u>& view_id_val)
{
    //Creating Data
    if (vps_video_parameter_set_id >= video_parameter_sets.size())
        video_parameter_sets.resize(vps_video_parameter_set_id + 1);
    std::vector<video_parameter_set_struct*>::iterator Data_Item = video_parameter_sets.begin() + vps_video_parameter_set_id;
    delete *Data_Item; *Data_Item = new video_parameter_set_struct(
        profile_tier_level_info_layers,
        vps_max_sub_layers_minus1,
        view_id_val
    );

    //NextCode
    NextCode_Clear();
    NextCode_Add(33);

    //Autorisation of other streams
    Streams[33].Searching_Payload = true; //seq_parameter_set
    Streams[36].Searching_Payload = true; //end_of_seq
    Streams[37].Searching_Payload = true; //end_of_bitstream
    Streams[38].Searching_Payload = true; //filler_data
}
//---------------------------------------------------------------------------
// Packet "32"
void File_Hevc::video_parameter_set()
{
    Element_Name("video_parameter_set");

    //Parsing
    vector<profile_tier_level_struct> ps;
    int32u  vps_num_layer_sets_minus1;
    int8u   vps_video_parameter_set_id, vps_max_sub_layers_minus1, vps_max_layers_minus1, vps_max_layer_id;
    bool    vps_base_layer_internal_flag, vps_temporal_id_nesting_flag, vps_sub_layer_ordering_info_present_flag;
    BS_Begin();
    Get_S1 (4,  vps_video_parameter_set_id,                     "vps_video_parameter_set_id");
    if (MustParse_VPS_SPS_PPS_FromFlv)
    {
        BS_End();
        Skip_XX(Element_Size-Element_Offset,                     "Data");

        //Creating Data
        video_parameter_sets_creating_data(vps_video_parameter_set_id, {}, 0, {}); //TODO: check which code is intended here

        return;
    }
    Get_SB (    vps_base_layer_internal_flag,                   "vps_base_layer_internal_flag");
    Skip_SB(                                                    "vps_base_layer_available_flag");
    Get_S1 (6,  vps_max_layers_minus1,                          "vps_max_layers_minus1");
    if (vps_max_layers_minus1==63)
    {
        Param_Info1("(Not supported)");
        RiskCalculationN++;
        RiskCalculationD++;
        BS_End();
        return;
    }
    Get_S1 (3,  vps_max_sub_layers_minus1,                      "vps_max_sub_layers_minus1");
    if (vps_max_sub_layers_minus1>6)
    {
        Trusted_IsNot("vps_max_sub_layers_minus1 not valid");
        RiskCalculationN++;
        RiskCalculationD++;
        BS_End();
        return; //Problem, not valid
    }
    Get_SB (    vps_temporal_id_nesting_flag,                   "vps_temporal_id_nesting_flag");
    //if (vps_max_sub_layers_minus1==0 && !vps_temporal_id_nesting_flag)
    //{
    //    Trusted_IsNot("vps_temporal_id_nesting_flag not valid");
    //    BS_End();
    //    return; //Problem, not valid
    //}
    Skip_S2(16,                                                 "vps_reserved_0xffff_16bits");
    ps.resize(1);
    profile_tier_level(ps[0], true, vps_max_sub_layers_minus1);
    Get_SB (   vps_sub_layer_ordering_info_present_flag,        "vps_sub_layer_ordering_info_present_flag");
    for (int32u SubLayerPos=(vps_sub_layer_ordering_info_present_flag?0:vps_max_sub_layers_minus1); SubLayerPos<=vps_max_sub_layers_minus1; SubLayerPos++)
    {
        Element_Begin1("SubLayer");
        Skip_UE(                                                "vps_max_dec_pic_buffering_minus1");
        Skip_UE(                                                "vps_max_num_reorder_pics");
        Skip_UE(                                                "vps_max_latency_increase_plus1");
        Element_End0();
    }
    Get_S1 ( 6, vps_max_layer_id,                               "vps_max_layer_id");
    Get_UE (    vps_num_layer_sets_minus1,                      "vps_num_layer_sets_minus1");
    if (vps_num_layer_sets_minus1>=1024)
    {
        Trusted_IsNot("vps_num_layer_sets_minus1 not valid");
        RiskCalculationN++;
        RiskCalculationD++;
        BS_End();
        return; //Problem, not valid
    }
    for (int32u LayerSetPos=1; LayerSetPos<=vps_num_layer_sets_minus1; LayerSetPos++)
        for (int8u LayerId=0; LayerId<=vps_max_layer_id; LayerId++)
            Skip_SB(                                            "layer_id_included_flag");
    TEST_SB_SKIP(                                               "vps_timing_info_present_flag");
        int32u vps_time_scale, vps_num_hrd_parameters;
        Skip_S4(32,                                             "vps_num_units_in_tick");
        Get_S4 (32, vps_time_scale,                             "vps_time_scale");
        if (vps_time_scale==0)
        {
            Trusted_IsNot("vps_time_scale not valid");
            RiskCalculationN++;
            RiskCalculationD++;
            Element_End0();
            BS_End();
            return; //Problem, not valid
        }
        TEST_SB_SKIP(                                           "vps_poc_proportional_to_timing_flag");
            Skip_UE(                                            "vps_num_ticks_poc_diff_one_minus1");
        TEST_SB_END();
        Get_UE (    vps_num_hrd_parameters,                     "vps_num_hrd_parameters");
        if (vps_num_hrd_parameters>1024)
        {
            Trusted_IsNot("vps_num_hrd_parameters not valid");
            RiskCalculationN++;
            RiskCalculationD++;
            vps_num_hrd_parameters=0;
        }
        for (int32u HrdPos=0; HrdPos<vps_num_hrd_parameters; HrdPos++)
        {
            seq_parameter_set_struct::vui_parameters_struct::xxl_common *xxL_Common=NULL;
            seq_parameter_set_struct::vui_parameters_struct::xxl *NAL=NULL, *VCL=NULL;
            int32u hrd_layer_set_idx;
            bool   cprms_present_flag;
            Get_UE (   hrd_layer_set_idx,                       "hrd_layer_set_idx");
            if (hrd_layer_set_idx>=1024)
                Trusted_IsNot("hrd_layer_set_idx not valid");
            if (HrdPos)
                Get_SB (cprms_present_flag,                     "cprms_present_flag");
            else
                cprms_present_flag=true;
            hrd_parameters(cprms_present_flag, vps_max_sub_layers_minus1, xxL_Common, NAL, VCL);
            delete xxL_Common; xxL_Common=NULL; //TODO: keep VPS hrd_parameters
            delete NAL; NAL=NULL;
            delete VCL; VCL=NULL;
        }
    TEST_SB_END();
    vector<int16u> view_id_val;
    TESTELSE_SB_SKIP(                                           "vps_extension_flag");
        int8u view_id_len;
        bool splitting_flag, vps_nuh_layer_id_present_flag;
        for (auto Bits=(Data_BS_Remain()%8); Bits; Bits--)
            Mark_1();
        if (vps_max_layers_minus1 && vps_base_layer_internal_flag)
        {
            ps.resize(ps.size()+1);
            profile_tier_level(ps.back(), false, vps_max_sub_layers_minus1);
        }
        Get_SB (splitting_flag,                                 "splitting_flag");
        int NumScalabilityTypes=0;
        bool scalability_mask_flag[16];
        for (int i=0; i<16; i++)
        {
            Get_SB (scalability_mask_flag[i],                  "scalability_mask_flag");
            NumScalabilityTypes+=scalability_mask_flag[i];
        }
        int8u dimension_id_len_minus1[16];
        for (int i=0; i<(NumScalabilityTypes-splitting_flag); i++)
        {
            Get_S1 (3, dimension_id_len_minus1[i],              "dimension_id_len_minus1");
            if (dimension_id_len_minus1[i]>=6)
            {
                Param_Info1("dimension_id_len_minus1 not valid");
                RiskCalculationN++;
                RiskCalculationD++;
                BS_End();
                return;
            }
        }
        Get_SB (vps_nuh_layer_id_present_flag,                  "vps_nuh_layer_id_present_flag");
        auto MaxLayersMinus1=vps_max_layers_minus1>62?62:vps_max_layers_minus1;
        int8u dimension_id[64][16];
        int8u layer_id_in_nuh[64];
        if (!splitting_flag)
            for (int j=0; j<NumScalabilityTypes; j++)
                dimension_id[0][j]=0;
        else
        {
            Param_Info1("(Not supported)");
            RiskCalculationN++;
            RiskCalculationD++;
            BS_End();
            return;
        }
        layer_id_in_nuh[0]=0;
        for (int i=1; i<=MaxLayersMinus1; i++)
        {
            if (vps_nuh_layer_id_present_flag)
                Get_S1(6, layer_id_in_nuh[i],                  "layer_id_in_nuh");
            else
                layer_id_in_nuh[i]=i;
            if (!splitting_flag)
                for (int j=0; j<NumScalabilityTypes; j++)
                    Get_S1 (dimension_id_len_minus1[j]+1, dimension_id[i][j], "dimension_id");
        }
        Get_S1 (4, view_id_len,                                 "view_id_len");
        auto NumViews = 1;
        int8u ScalabilityId[64][16];
        //int8u DepthLayerFlag[64];
        int8u ViewOrderIdx[64];
        //int8u DependencyId[64];
        //int8u AuxId[64];
        memset(ScalabilityId, -1, 64 * 16 * sizeof(int8u));
        memset(ViewOrderIdx, -1, 64 * sizeof(int8u));
        for (int i = 0; i <= MaxLayersMinus1; i++)
        {
            auto lId = layer_id_in_nuh[i];
            for (int smIdx = 0, j = 0; smIdx < 16; smIdx++)
            {
                if (scalability_mask_flag[smIdx])
                    ScalabilityId[i][smIdx] = dimension_id[i][j++];
                else
                    ScalabilityId[i][smIdx] = 0;
            }
            //DepthLayerFlag[lId] = ScalabilityId[i][0];
            ViewOrderIdx[lId] = ScalabilityId[i][1];
            //DependencyId[lId] = ScalabilityId[i][2];
            //AuxId[lId] = ScalabilityId[i][3];
            if (i > 0)
            {
                auto newViewFlag = 1;
                for (int j = 0; j < i; j++)
                    if (ViewOrderIdx[lId] == ViewOrderIdx[layer_id_in_nuh[j]])
                        newViewFlag = 0;
                NumViews += newViewFlag;
            }
        }
        if (view_id_len)
        {
            view_id_val.resize(64, -1);
            for (int i=0; i<NumViews; i++)
                if (ViewOrderIdx[i]!=(int8u)-1)
                    Get_S2 (view_id_len, view_id_val[ViewOrderIdx[i]], "view_id_val");
                else
                    Trusted_IsNot("ViewOrderIdx");
        }
        bool direct_dependency_flag[64][64];
        memset(direct_dependency_flag, 0, sizeof(direct_dependency_flag));
        for (int i = 1; i <= MaxLayersMinus1; i++)
            for (int j = 0; j < i; j++)
                Get_SB(direct_dependency_flag[i][j],            "direct_dependency_flag");
        int32u num_add_layer_sets;
        int8u layerIdInListFlag[64];
        bool DependencyFlag[64][64];
        for (int i = 0; i <= MaxLayersMinus1; i++)
            for (int j = 0; j <= MaxLayersMinus1; j++)
            {
                DependencyFlag[i][j] = direct_dependency_flag[i][j];
                for (int k = 0; k < i; k++)
                    if (direct_dependency_flag[i][k] && DependencyFlag[k][j])
                        DependencyFlag[i][j] = 1;
            }
        int8u IdDirectRefLayer[64][64];
        int8u IdRefLayer[64][64];
        int8u IdPredictedLayer[64][64];
        int8u NumDirectRefLayers[64];
        int8u NumRefLayers[64];
        int8u NumPredictedLayers[64];
        int8u TreePartitionLayerIdList[64][64];
        int8u NumLayersInTreePartition[64];
        for (int i = 0; i <= MaxLayersMinus1; i++)
        {
            auto iNuhLId = layer_id_in_nuh[i];
            int8u d = 0, r = 0, p = 0;
            for (int j = 0; j <= MaxLayersMinus1; j++)
            {
                auto jNuhLid = layer_id_in_nuh[j];
                if (direct_dependency_flag[i][j])
                    IdDirectRefLayer[iNuhLId][d++] = jNuhLid;
                if (DependencyFlag[i][j])
                    IdRefLayer[iNuhLId][r++] = jNuhLid;
                if (DependencyFlag[j][i])
                    IdPredictedLayer[iNuhLId][p++] = jNuhLid;
            }
            NumDirectRefLayers[iNuhLId] = d;
            NumRefLayers[iNuhLId] = r;
            NumPredictedLayers[iNuhLId] = p;
        }
        for (int i = 0; i <= 63; i++)
            layerIdInListFlag[i] = 0;
        int8u k = 0;
        for (int i = 0; i <= MaxLayersMinus1; i++)
        {
            auto iNuhLId = layer_id_in_nuh[i];
            if (NumDirectRefLayers[iNuhLId] == 0)
            {
                TreePartitionLayerIdList[k][0] = iNuhLId;
                int8u  h = 1;
                for (int j = 0; j < NumPredictedLayers[iNuhLId]; j++)
                {
                    auto predLId = IdPredictedLayer[iNuhLId][j];
                    if (!layerIdInListFlag[predLId])
                    {
                        TreePartitionLayerIdList[k][h++] = predLId;
                        layerIdInListFlag[predLId] = 1;
                    }
                }
                NumLayersInTreePartition[k++] = h;
            }
        }
        auto NumIndependentLayers = k;


        if (NumIndependentLayers > 1)
            Get_UE (num_add_layer_sets,                         "num_add_layer_sets");
        else
            num_add_layer_sets=0;
        vector<int32u> NumLayersInIdList;
        vector<vector<int32u> > LayerSetLayerIdList;
        vector<vector<int32u> > highest_layer_idx_plus1List;
        highest_layer_idx_plus1List.resize(num_add_layer_sets);
        for (int i = 0; i < num_add_layer_sets; i++)
        {
            highest_layer_idx_plus1List[i].reserve(NumIndependentLayers);
            highest_layer_idx_plus1List[i].push_back(0); // Unused?
            for (int j = 1; j < NumIndependentLayers; j++)
            {
                int32u highest_layer_idx_plus1;
                Get_S4 (ceil(log2(NumLayersInTreePartition[j] + 1)), highest_layer_idx_plus1, "highest_layer_idx_plus1");
                highest_layer_idx_plus1List[i].push_back(highest_layer_idx_plus1);
            }
        }
        for (int i = 0; i < num_add_layer_sets; i++)
        {
            int32u layerNum = 0;
            auto lsIdx = vps_num_layer_sets_minus1 + 1 + i;
            for (int8u treeIdx = 1; treeIdx < NumIndependentLayers; treeIdx++)
            {
                for (int8u layerCnt = 0; layerCnt < highest_layer_idx_plus1List[i][treeIdx]; layerCnt++)
                {
                    if (LayerSetLayerIdList.size() <= lsIdx)
                        LayerSetLayerIdList.resize(lsIdx + 1);
                    if (LayerSetLayerIdList[lsIdx].size() <= layerNum)
                        LayerSetLayerIdList[lsIdx].resize(layerNum + 1);
                    LayerSetLayerIdList[lsIdx][layerNum++] = TreePartitionLayerIdList[treeIdx][layerCnt];
                }
            }
            if (NumLayersInIdList.size() <= lsIdx)
                NumLayersInIdList.resize(lsIdx + 1);
            NumLayersInIdList[lsIdx] = layerNum;
        }
        TEST_SB_SKIP(                                           "vps_sub_layers_max_minus1_present_flag")
            for (int i = 0; i <= MaxLayersMinus1; i++)
                Skip_S1(3,                                      "sub_layers_vps_max_minus1[");
        TEST_SB_END();
        TEST_SB_SKIP(                                           "max_tid_ref_present_flag")
            for (int i = 0; i <= MaxLayersMinus1; i++)
                for (int j = i + 1; j <= MaxLayersMinus1; j++)
                    Skip_S1(3,                                  "max_tid_il_ref_pics_plus1[");
        TEST_SB_END();
        Skip_SB(                                                "default_ref_layers_active_flag");
        int32u vps_num_profile_tier_level_minus1;
        Get_UE(vps_num_profile_tier_level_minus1,               "vps_num_profile_tier_level_minus1");
        for (int i = vps_base_layer_internal_flag ? 2 : 1; i <= vps_num_profile_tier_level_minus1; i++)
        {
            bool vps_profile_present_flag;
            Get_SB(vps_profile_present_flag,                    "vps_profile_present_flag");
            ps.resize(ps.size()+1);
            profile_tier_level(ps.back(), vps_profile_present_flag, vps_max_sub_layers_minus1);
        }
        auto NumLayerSets = vps_num_layer_sets_minus1 + 1 + num_add_layer_sets;
        int32u num_add_olss;
        int8u default_output_layer_idc;
        if (NumLayerSets > 1) {
            Get_UE (num_add_olss,                               "num_add_olss");
            Get_S1 (2, default_output_layer_idc,                "default_output_layer_idc");
        }
        else
        {
            num_add_olss = 0;
        }
        Skip_BS(Data_BS_Remain(),                               "(Not parsed)");
    TESTELSE_SB_ELSE(                                           "vps_extension_flag");
        rbsp_trailing_bits();
    TESTELSE_SB_END();
    BS_End();

    FILLING_BEGIN_PRECISE();
        //Creating Data
        video_parameter_sets_creating_data(vps_video_parameter_set_id, ps, vps_max_sub_layers_minus1, view_id_val);
    FILLING_END();
}

//---------------------------------------------------------------------------
// Packet "33"
void File_Hevc::seq_parameter_set()
{
    Element_Name("seq_parameter_set");

    //Parsing
    profile_tier_level_struct p;
    seq_parameter_set_struct::vui_parameters_struct* vui_parameters_Item=NULL;
    int32u  sps_seq_parameter_set_id, chroma_format_idc, pic_width_in_luma_samples, pic_height_in_luma_samples, bit_depth_luma_minus8, bit_depth_chroma_minus8, log2_max_pic_order_cnt_lsb_minus4, num_short_term_ref_pic_sets;
    int32u  conf_win_left_offset=0, conf_win_right_offset=0, conf_win_top_offset=0, conf_win_bottom_offset=0, sps_max_num_reorder_pics=0;
    int8u   video_parameter_set_id, max_sub_layers_minus1, sps_ext_or_max_sub_layers_minus1;
    bool    separate_colour_plane_flag=false, sps_sub_layer_ordering_info_present_flag;
    BS_Begin();
    Get_S1 (4, video_parameter_set_id,                          "sps_video_parameter_set_id");
    std::vector<video_parameter_set_struct*>::iterator video_parameter_set_Item;
    if (video_parameter_set_id >= video_parameter_sets.size() || (*(video_parameter_set_Item = video_parameter_sets.begin() + video_parameter_set_id)) == NULL)
    {
        //Not yet present
        Skip_BS(Data_BS_Remain(),                               "Data (video_parameter_set is missing)");
        BS_End();
        RiskCalculationN++;
        RiskCalculationD++;
        return;
    }
    if (nuh_layer_id)
    {
        const auto vps_max_sub_layers_minus1 = (*video_parameter_set_Item)->vps_max_sub_layers_minus1;
        Get_S1 (3, sps_ext_or_max_sub_layers_minus1,            "sps_ext_or_max_sub_layers_minus1");
        max_sub_layers_minus1=sps_ext_or_max_sub_layers_minus1 == 7 ? vps_max_sub_layers_minus1 : sps_ext_or_max_sub_layers_minus1;
    }
    else
    {
    sps_ext_or_max_sub_layers_minus1=0;
    Get_S1 (3, max_sub_layers_minus1,                           "sps_max_sub_layers_minus1");
    }
    auto MultiLayerExtSpsFlag=(nuh_layer_id && sps_ext_or_max_sub_layers_minus1==7);
    if (MultiLayerExtSpsFlag)
    {
        p.profile_space=-1;
        p.tier_flag=true;
        p.profile_idc=-1;
        p.level_idc=-1;
    }
    else
    {
    Skip_SB(                                                    "sps_temporal_id_nesting_flag");
    profile_tier_level(p, true, max_sub_layers_minus1);
    }
    Get_UE (   sps_seq_parameter_set_id,                        "sps_seq_parameter_set_id");
    if (MustParse_VPS_SPS_PPS_FromFlv)
    {
        BS_End();
        Skip_XX(Element_Size-Element_Offset,                     "Data");

        //Creating Data
        if (sps_seq_parameter_set_id>=seq_parameter_sets.size())
            seq_parameter_sets.resize(sps_seq_parameter_set_id+1);
        std::vector<seq_parameter_set_struct*>::iterator Data_Item=seq_parameter_sets.begin()+sps_seq_parameter_set_id;
        delete *Data_Item; *Data_Item=new seq_parameter_set_struct(
                                                                    0,
                                                                    NULL, //TODO: check which code is intended here
                                                                    profile_tier_level_struct().Clear(),
                                                                    0,
                                                                    0,
                                                                    0,
                                                                    0,
                                                                    0,
                                                                    0,
                                                                    0,
                                                                    0,
                                                                    0,
                                                                    0,
                                                                    0,
                                                                    0,
                                                                    0
                                                                    );

        //NextCode
        NextCode_Clear();
        NextCode_Add(34);

        //Autorisation of other streams
        Streams[34].Searching_Payload=true; //pic_parameter_set

        return;
    }
    if (MultiLayerExtSpsFlag) {
        TEST_SB_SKIP(                                               "update_rep_format_flag");
            Skip_S1(1,                                              "sps_rep_format_idx");
        TEST_SB_END();
        chroma_format_idc=-1;
        pic_width_in_luma_samples=-1;
        pic_height_in_luma_samples=-1;
        bit_depth_luma_minus8=-1;
        bit_depth_chroma_minus8=-1;
    }
    else {
    Get_UE (   chroma_format_idc,                               "chroma_format_idc"); Param_Info1(Hevc_chroma_format_idc((int8u)chroma_format_idc));
    if (chroma_format_idc>=4)
    {
        Trusted_IsNot("chroma_format_idc not valid");
        RiskCalculationN++;
        RiskCalculationD++;
        BS_End();
        return; //Problem, not valid
    }
    if (chroma_format_idc==3)
        Get_SB (separate_colour_plane_flag,                     "separate_colour_plane_flag");
    Get_UE (    pic_width_in_luma_samples,                      "pic_width_in_luma_samples");
    Get_UE (    pic_height_in_luma_samples,                     "pic_height_in_luma_samples");
    TEST_SB_SKIP(                                               "conformance_window_flag");
        Get_UE (conf_win_left_offset,                           "conf_win_left_offset");
        Get_UE (conf_win_right_offset,                          "conf_win_right_offset");
        Get_UE (conf_win_top_offset,                            "conf_win_top_offset");
        Get_UE (conf_win_bottom_offset,                         "conf_win_bottom_offset");
    TEST_SB_END();
    Get_UE (   bit_depth_luma_minus8,                           "bit_depth_luma_minus8");
    if (bit_depth_luma_minus8>6)
    {
        Trusted_IsNot("bit_depth_luma_minus8 not valid");
        RiskCalculationN++;
        RiskCalculationD++;
        return; //Problem, not valid
    }
    Get_UE (   bit_depth_chroma_minus8,                         "bit_depth_chroma_minus8");
    if (bit_depth_chroma_minus8>6)
    {
        Trusted_IsNot("bit_depth_chroma_minus8 not valid");
        RiskCalculationN++;
        RiskCalculationD++;
        return; //Problem, not valid
    }
    }
    Get_UE (   log2_max_pic_order_cnt_lsb_minus4,               "log2_max_pic_order_cnt_lsb_minus4");
    if (log2_max_pic_order_cnt_lsb_minus4>12)
    {
        Trusted_IsNot("log2_max_pic_order_cnt_lsb_minus4 not valid");
        RiskCalculationN++;
        RiskCalculationD++;
        return; //Problem, not valid
    }
    if (!MultiLayerExtSpsFlag) {
    Get_SB (   sps_sub_layer_ordering_info_present_flag,        "sps_sub_layer_ordering_info_present_flag");
    for (int32u SubLayerPos = (sps_sub_layer_ordering_info_present_flag ? 0 : max_sub_layers_minus1); SubLayerPos <= max_sub_layers_minus1; SubLayerPos++)
    {
        Element_Begin1("SubLayer");
        Skip_UE(                                                "sps_max_dec_pic_buffering_minus1");
        Get_UE (sps_max_num_reorder_pics,                       "sps_max_num_reorder_pics");
        Skip_UE(                                                "sps_max_latency_increase_plus1");
        Element_End0();
    }
    }
    Skip_UE(                                                    "log2_min_luma_coding_block_size_minus3");
    Skip_UE(                                                    "log2_diff_max_min_luma_coding_block_size");
    Skip_UE(                                                    "log2_min_transform_block_size_minus2");
    Skip_UE(                                                    "log2_diff_max_min_transform_block_size");
    Skip_UE(                                                    "max_transform_hierarchy_depth_inter");
    Skip_UE(                                                    "max_transform_hierarchy_depth_intra");
    TEST_SB_SKIP(                                               "scaling_list_enabled_flag");
        bool sps_infer_scaling_list_flag = false;
        if (MultiLayerExtSpsFlag)
           Get_SB (sps_infer_scaling_list_flag,                 "sps_infer_scaling_list_flag");
        if (sps_infer_scaling_list_flag)
           Skip_S1 (6,                                          "sps_scaling_list_ref_layer_id");
        else {
            TEST_SB_SKIP(                                       "sps_scaling_list_data_present_flag");
                scaling_list_data();
            TEST_SB_END();
        }
    TEST_SB_END();
    Skip_SB(                                                    "amp_enabled_flag");
    Skip_SB(                                                    "sample_adaptive_offset_enabled_flag");
    TEST_SB_SKIP(                                               "pcm_enabled_flag");
        Element_Begin1("pcm");
        Skip_S1(4,                                              "pcm_sample_bit_depth_luma_minus1");
        Skip_S1(4,                                              "pcm_sample_bit_depth_chroma_minus1");
        Skip_UE(                                                "log2_min_pcm_luma_coding_block_size_minus3");
        Skip_UE(                                                "log2_diff_max_min_pcm_luma_coding_block_size");
        Skip_SB(                                                "pcm_loop_filter_disabled_flag");
        Element_End0();
    TEST_SB_END();
    Get_UE (   num_short_term_ref_pic_sets,                     "num_short_term_ref_pic_sets");
    if (num_short_term_ref_pic_sets>64)
    {
        BS_End();
        Trusted_IsNot("num_short_term_ref_pic_sets not valid");
        RiskCalculationN++;
        RiskCalculationD++;
        return; //Problem, not valid
    }
    short_term_ref_pic_sets((int8u)num_short_term_ref_pic_sets);
    TEST_SB_SKIP(                                               "long_term_ref_pics_present_flag");
        Element_Begin1("long_term_ref_pics");
        int32u num_long_term_ref_pics_sps;
        Get_UE (num_long_term_ref_pics_sps,                     "num_long_term_ref_pics_sps");
        for (int32u long_term_ref_pics_sps_pos=0; long_term_ref_pics_sps_pos<num_long_term_ref_pics_sps; long_term_ref_pics_sps_pos++)
        {
            Skip_BS(log2_max_pic_order_cnt_lsb_minus4+4,        "lt_ref_pic_poc_lsb_sps");
            Skip_SB(                                            "used_by_curr_pic_lt_sps_flag");
        }
        Element_End0();
    TEST_SB_END();
    Skip_SB(                                                    "sps_temporal_mvp_enabled_flag");
    Skip_SB(                                                    "strong_intra_smoothing_enabled_flag");
    TEST_SB_SKIP(                                               "vui_parameters_present_flag");
        vui_parameters(vui_parameters_Item, max_sub_layers_minus1);
    TEST_SB_END();
    TESTELSE_SB_SKIP(                                           "sps_extension_flag");
        int8u sps_extension_4bits;
        bool sps_range_extension_flag, sps_multilayer_extension_flag, sps_3d_extension_flag, sps_scc_extension_flag;
        Get_SB (sps_range_extension_flag,                       "sps_range_extension_flag");
        Get_SB (sps_multilayer_extension_flag,                  "sps_multilayer_extension_flag");
        Get_SB (sps_3d_extension_flag,                          "sps_3d_extension_flag");
        Get_SB (sps_scc_extension_flag,                         "sps_scc_extension_flag");
        Get_S1 (4, sps_extension_4bits,                         "sps_extension_4bits");
        if (sps_range_extension_flag)
        {
            Element_Begin1("sps_range_extension");
            Skip_SB(                                            "transform_skip_rotation_enabled_flag");
            Skip_SB(                                            "transform_skip_context_enabled_flag");
            Skip_SB(                                            "implicit_rdpcm_enabled_flag");
            Skip_SB(                                            "explicit_rdpcm_enabled_flag");
            Skip_SB(                                            "extended_precision_processing_flag");
            Skip_SB(                                            "intra_smoothing_disabled_flag");
            Skip_SB(                                            "high_precision_offsets_enabled_flag");
            Skip_SB(                                            "persistent_rice_adaptation_enabled_flag");
            Skip_SB(                                            "cabac_bypass_alignment_enabled_flag");
            Element_End0();
        }
        if (sps_multilayer_extension_flag)
        {
            Element_Begin1("sps_multilayer_extension");
            Skip_SB(                                            "inter_view_mv_vert_constraint_flag");
            Element_End0();
        }
        if (sps_3d_extension_flag)
        {
            Element_Begin1("sps_3d_extension");
            Skip_SB(                                            "iv_di_mc_enabled_flag");
            Skip_SB(                                            "iv_mv_scal_enabled_flag");
            Skip_UE(                                            "log2_ivmc_sub_pb_size_minus3");
            Skip_SB(                                            "iv_res_pred_enabled_flag");
            Skip_SB(                                            "depth_ref_enabled_flag");
            Skip_SB(                                            "vsp_mc_enabled_flag[");
            Skip_SB(                                            "dbbp_enabled_flag");
            Skip_SB(                                            "iv_di_mc_enabled_flag");
            Skip_SB(                                            "iv_mv_scal_enabled_flag");
            Skip_SB(                                            "tex_mc_enabled_flag");
            Skip_UE(                                            "log2_texmc_sub_pb_size_minus3");
            Skip_SB(                                            "intra_contour_enabled_flag");
            Skip_SB(                                            "intra_dc_only_wedge_enabled_flag");
            Skip_SB(                                            "cqt_cu_part_pred_enabled_flag");
            Skip_SB(                                            "inter_dc_only_enabled_flag");
            Skip_SB(                                            "skip_intra_enabled_flag");
            Element_End0();
        }
        if (sps_scc_extension_flag)
        {
            Element_Begin1("sps_scc_extension");
            Skip_SB(                                            "sps_curr_pic_ref_enabled_flag");
            TEST_SB_SKIP(                                       "palette_mode_enabled_flag");
                Skip_UE(                                        "palette_max_size");
                Skip_UE(                                        "delta_palette_max_predictor_size");
                TEST_SB_SKIP(                                   "sps_palette_predictor_initializers_present_flag");
                    int32u sps_num_palette_predictor_initializers_minus1;
                    Get_UE (sps_num_palette_predictor_initializers_minus1, "sps_num_palette_predictor_initializers_minus1");
                    auto numComps=(chroma_format_idc==0)?1:3;
                    for(int comp=0; comp<numComps; comp++)
                        for (int32u i=0; i<=sps_num_palette_predictor_initializers_minus1; i++)
                            Skip_S4(8+(comp==0?bit_depth_luma_minus8:bit_depth_chroma_minus8), "sps_palette_predictor_initializer");
                TEST_SB_END();
                Skip_S1(2,                                      "motion_vector_resolution_control_idc");
                Skip_SB(                                        "intra_boundary_filtering_disabled_flag");
            TEST_SB_END();
            Element_End0();
        }
        if (sps_extension_4bits)
        {
            Skip_BS(Data_BS_Remain(),                           "(Not parsed)");
            RiskCalculationN++; //xxx_extension_flag is set, we can not check the end of the content, so a bit risky
            RiskCalculationD++;
        }
    TESTELSE_SB_ELSE(                                           "sps_extension_flag");
        rbsp_trailing_bits();
    TESTELSE_SB_END();
    BS_End();

    FILLING_BEGIN_PRECISE();
        //Creating Data
        if (sps_seq_parameter_set_id>=seq_parameter_sets.size())
            seq_parameter_sets.resize(sps_seq_parameter_set_id+1);
        std::vector<seq_parameter_set_struct*>::iterator Data_Item=seq_parameter_sets.begin()+sps_seq_parameter_set_id;
        if (!MustParse_VPS_SPS_PPS_FromLhvc || !*Data_Item)
        {
        delete *Data_Item; *Data_Item=new seq_parameter_set_struct(
                                                                    nuh_layer_id,
                                                                    vui_parameters_Item,
                                                                    p,
                                                                    pic_width_in_luma_samples,
                                                                    pic_height_in_luma_samples,
                                                                    conf_win_left_offset,
                                                                    conf_win_right_offset,
                                                                    conf_win_top_offset,
                                                                    conf_win_bottom_offset,
                                                                    (int8u)video_parameter_set_id,
                                                                    (int8u)chroma_format_idc,
                                                                    separate_colour_plane_flag,
                                                                    (int8u)log2_max_pic_order_cnt_lsb_minus4,
                                                                    (int8u)bit_depth_luma_minus8,
                                                                    (int8u)bit_depth_chroma_minus8,
                                                                    (int8u)sps_max_num_reorder_pics
                                                                  );
        }

        //NextCode
        NextCode_Clear();
        NextCode_Add(34);

        //Autorisation of other streams
        Streams[34].Searching_Payload=true; //pic_parameter_set

        //Computing values (for speed)
        size_t MaxNumber=(int32u)std::pow(2.0, (int)((*Data_Item)->log2_max_pic_order_cnt_lsb_minus4 + 4)); // TODO
        /*
        switch (Data_Item_New->pic_order_cnt_type)
        {
            case 0 :
                        MaxNumber=Data_Item_New->MaxPicOrderCntLsb;
                        break;
            case 1 :
            case 2 :
                        MaxNumber=Data_Item_New->MaxFrameNum*2;
                        break;
            default:
                        MaxNumber = 0;
        }
        */

        if (MaxNumber>TemporalReferences_Reserved)
        {
            TemporalReferences.resize(4*MaxNumber);
            TemporalReferences_Reserved=MaxNumber;
        }
    FILLING_ELSE();
        delete vui_parameters_Item; //vui_parameters_Item=NULL;
    FILLING_END();
}

//---------------------------------------------------------------------------
// Packet "34"
void File_Hevc::pic_parameter_set()
{
    Element_Name("pic_parameter_set");

    //Parsing
    int32u  pps_pic_parameter_set_id, pps_seq_parameter_set_id, num_ref_idx_l0_default_active_minus1, num_ref_idx_l1_default_active_minus1;
    int8u   num_extra_slice_header_bits;
    bool    tiles_enabled_flag, dependent_slice_segments_enabled_flag;
    BS_Begin();
    Get_UE (    pps_pic_parameter_set_id,                       "pps_pic_parameter_set_id");
    if (pps_pic_parameter_set_id>=64)
    {
        Trusted_IsNot("pic_parameter_set_id not valid");
        BS_End();
        RiskCalculationN++;
        RiskCalculationD++;
        return; //Problem, not valid
    }
    Get_UE (    pps_seq_parameter_set_id,                       "pps_seq_parameter_set_id");
    if (pps_seq_parameter_set_id>=16)
    {
        Trusted_IsNot("seq_parameter_set_id not valid");
        BS_End();
        RiskCalculationN++;
        RiskCalculationD++;
        return; //Problem, not valid
    }
    //std::vector<seq_parameter_set_struct*>::iterator seq_parameter_set_Item;
    if (pps_seq_parameter_set_id>=seq_parameter_sets.size() || (*(seq_parameter_sets.begin()+pps_seq_parameter_set_id))==NULL) //(seq_parameter_set_Item=seq_parameter_sets.begin()+pps_seq_parameter_set_id) not usd for the moment
    {
        //Not yet present
        Skip_BS(Data_BS_Remain(),                               "Data (seq_parameter_set is missing)");
        BS_End();
        RiskCalculationN++;
        RiskCalculationD++;
        return;
    }
    if (MustParse_VPS_SPS_PPS_FromFlv)
    {
        BS_End();
        Skip_XX(Element_Size-Element_Offset,                     "Data");

        //Filling
        if (pps_pic_parameter_set_id>=pic_parameter_sets.size())
            pic_parameter_sets.resize(pps_pic_parameter_set_id+1);
        std::vector<pic_parameter_set_struct*>::iterator pic_parameter_sets_Item=pic_parameter_sets.begin()+pps_pic_parameter_set_id;
        delete *pic_parameter_sets_Item; *pic_parameter_sets_Item=new pic_parameter_set_struct(
                                                                                                    0,
                                                                                                    0,
                                                                                                    0,
                                                                                                    0,
                                                                                                    false
                                                                                              );

        //NextCode
        NextCode_Clear();

        //Autorisation of other streams
        Streams[ 0].Searching_Payload=true; //slice_segment_layer
        Streams[ 1].Searching_Payload=true; //slice_segment_layer
        Streams[ 2].Searching_Payload=true; //slice_segment_layer
        Streams[ 3].Searching_Payload=true; //slice_segment_layer
        Streams[ 4].Searching_Payload=true; //slice_layer
        Streams[ 5].Searching_Payload=true; //slice_layer
        Streams[ 6].Searching_Payload=true; //slice_layer
        Streams[ 7].Searching_Payload=true; //slice_layer
        Streams[ 8].Searching_Payload=true; //slice_layer
        Streams[ 9].Searching_Payload=true; //slice_layer
        Streams[16].Searching_Payload=true; //slice_segment_layer
        Streams[17].Searching_Payload=true; //slice_segment_layer
        Streams[18].Searching_Payload=true; //slice_segment_layer
        Streams[19].Searching_Payload=true; //slice_segment_layer
        Streams[20].Searching_Payload=true; //slice_segment_layer
        Streams[21].Searching_Payload=true; //slice_segment_layer

        return;
    }
    Get_SB (    dependent_slice_segments_enabled_flag,          "dependent_slice_segments_enabled_flag");
    Skip_SB(                                                    "output_flag_present_flag");
    Get_S1 (3,  num_extra_slice_header_bits,                    "num_extra_slice_header_bits");
    Skip_SB(                                                    "sign_data_hiding_flag");
    Skip_SB(                                                    "cabac_init_present_flag");
    Get_UE (    num_ref_idx_l0_default_active_minus1,           "num_ref_idx_l0_default_active_minus1");
    Get_UE (    num_ref_idx_l1_default_active_minus1,           "num_ref_idx_l1_default_active_minus1");
    Skip_SE(                                                    "init_qp_minus26");
    Skip_SB(                                                    "constrained_intra_pred_flag");
    Skip_SB(                                                    "transform_skip_enabled_flag");
    TEST_SB_SKIP(                                               "cu_qp_delta_enabled_flag");
        Skip_UE(                                                "diff_cu_qp_delta_depth");
    TEST_SB_END();
    Skip_SE(                                                    "pps_cb_qp_offset");
    Skip_SE(                                                    "pps_cr_qp_offset");
    Skip_SB(                                                    "pps_slice_chroma_qp_offsets_present_flag");
    Skip_SB(                                                    "weighted_pred_flag");
    Skip_SB(                                                    "weighted_bipred_flag");
    Skip_SB(                                                    "transquant_bypass_enable_flag");
    Get_SB (    tiles_enabled_flag,                             "tiles_enabled_flag");
    Skip_SB(                                                    "entropy_coding_sync_enabled_flag");
    if (tiles_enabled_flag)
    {
        Element_Begin1("tiles");
        int32u  num_tile_columns_minus1, num_tile_rows_minus1;
        bool    uniform_spacing_flag;
        Get_UE (    num_tile_columns_minus1,                    "num_tile_columns_minus1");
        Get_UE (    num_tile_rows_minus1,                       "num_tile_rows_minus1");
        Get_SB (    uniform_spacing_flag,                       "uniform_spacing_flag");
        if (!uniform_spacing_flag)
        {
            for (int32u tile_pos=0; tile_pos<num_tile_columns_minus1; tile_pos++)
               Skip_UE(                                         "column_width_minus1");
            for (int32u tile_pos=0; tile_pos<num_tile_rows_minus1; tile_pos++)
               Skip_UE(                                         "row_height_minus1");
        }
        Skip_SB(                                                "loop_filter_across_tiles_enabled_flag");
        Element_End0();
    }
    Skip_SB(                                                    "pps_loop_filter_across_slices_enabled_flag");
    TEST_SB_SKIP(                                               "deblocking_filter_control_present_flag");
        bool pps_disable_deblocking_filter_flag;
        Skip_SB(                                                "deblocking_filter_override_enabled_flag");
        Get_SB (    pps_disable_deblocking_filter_flag,         "pps_disable_deblocking_filter_flag");
        if (!pps_disable_deblocking_filter_flag)
        {
           Skip_SE(                                             "pps_beta_offset_div2");
           Skip_SE(                                             "pps_tc_offset_div2");
        }
    TEST_SB_END();
    TEST_SB_SKIP(                                               "pps_scaling_list_data_present_flag ");
        scaling_list_data();
    TEST_SB_END();
    Skip_SB(                                                    "lists_modification_present_flag");
    Skip_UE(                                                    "log2_parallel_merge_level_minus2");
    Skip_SB(                                                    "slice_segment_header_extension_present_flag");
    EndOfxPS(                                                   "pps_extension_flag", "pps_extension_data");
    BS_End();

    FILLING_BEGIN_PRECISE();
        //NextCode
        //NextCode_Clear();
        //NextCode_Add(0x05);
        //NextCode_Add(0x06);

        //Filling
        if (pps_pic_parameter_set_id>=pic_parameter_sets.size())
            pic_parameter_sets.resize(pps_pic_parameter_set_id+1);
        std::vector<pic_parameter_set_struct*>::iterator pic_parameter_sets_Item=pic_parameter_sets.begin()+pps_pic_parameter_set_id;
        if (!MustParse_VPS_SPS_PPS_FromLhvc || !*pic_parameter_sets_Item)
        {
        delete *pic_parameter_sets_Item; *pic_parameter_sets_Item = new pic_parameter_set_struct(
                                                                                                    (int8u)pps_seq_parameter_set_id,
                                                                                                    (int8u)num_ref_idx_l0_default_active_minus1,
                                                                                                    (int8u)num_ref_idx_l1_default_active_minus1,
                                                                                                    num_extra_slice_header_bits,
                                                                                                    dependent_slice_segments_enabled_flag
                                                                                                );
        }

        //Autorisation of other streams
        //if (!seq_parameter_sets.empty())
        //{
        //    for (int8u Pos=0x01; Pos<=0x06; Pos++)
        //    {
        //        Streams[Pos].Searching_Payload=true; //Coded slice...
        //        if (Streams[0x08].ShouldDuplicate)
        //            Streams[Pos].ShouldDuplicate=true;
        //    }
        //}

        //NextCode
        NextCode_Clear();

        //Autorisation of other streams
        Streams[ 0].Searching_Payload=true; //slice_segment_layer
        Streams[ 1].Searching_Payload=true; //slice_segment_layer
        Streams[ 2].Searching_Payload=true; //slice_segment_layer
        Streams[ 3].Searching_Payload=true; //slice_segment_layer
        Streams[ 4].Searching_Payload=true; //slice_layer
        Streams[ 5].Searching_Payload=true; //slice_layer
        Streams[ 6].Searching_Payload=true; //slice_layer
        Streams[ 7].Searching_Payload=true; //slice_layer
        Streams[ 8].Searching_Payload=true; //slice_layer
        Streams[ 9].Searching_Payload=true; //slice_layer
        Streams[16].Searching_Payload=true; //slice_segment_layer
        Streams[17].Searching_Payload=true; //slice_segment_layer
        Streams[18].Searching_Payload=true; //slice_segment_layer
        Streams[19].Searching_Payload=true; //slice_segment_layer
        Streams[20].Searching_Payload=true; //slice_segment_layer
        Streams[21].Searching_Payload=true; //slice_segment_layer
    FILLING_END();
}

//---------------------------------------------------------------------------
// Packet "35"
void File_Hevc::access_unit_delimiter()
{
    Element_Name("access_unit_delimiter");

    //Parsing
    BS_Begin();
    Info_S1( 3, pic_type,                                   "pic_type"); Param_Info1(Hevc_pic_type[pic_type]);
    Mark_1();
    BS_End();

    FILLING_BEGIN_PRECISE();
    FILLING_ELSE();
        RiskCalculationN++;
    FILLING_END();
    RiskCalculationD++;
}

//---------------------------------------------------------------------------
// Packet "36"
void File_Hevc::end_of_seq()
{
    Element_Name("end_of_seq");
}

//---------------------------------------------------------------------------
// Packet "37"
void File_Hevc::end_of_bitstream()
{
    Element_Name("end_of_bitstream");
}

//---------------------------------------------------------------------------
// Packet "38"
void File_Hevc::filler_data()
{
    Element_Name("filler_data");

    //Parsing
    Skip_XX(Element_Size,                                       "ff_bytes");
}

//---------------------------------------------------------------------------
// Packet "39" or "40
void File_Hevc::sei()
{
    Element_Name("sei");

    //Parsing
    int32u seq_parameter_set_id=(int32u)-1;
    while(Element_Offset+1<Element_Size)
    {
        Element_Begin1("sei message");
            sei_message(seq_parameter_set_id);
        Element_End0();
    }
    BS_Begin();
    if (!Data_BS_Remain() || !Peek_SB())
    {
        Fill(Stream_Video, 0, "SEI_rbsp_stop_one_bit", "Missing", Unlimited, true, true);
        RiskCalculationN++;
        RiskCalculationD++;
    }
    else
        rbsp_trailing_bits();
    BS_End();

    FILLING_BEGIN_PRECISE();
    FILLING_ELSE();
        RiskCalculationN++;
    FILLING_END();
    RiskCalculationD++;
}

//---------------------------------------------------------------------------
void File_Hevc::sei_message(int32u &seq_parameter_set_id)
{
    //Parsing
    int32u  payloadType=0, payloadSize=0;
    int8u   payload_type_byte, payload_size_byte;
    Element_Begin1("sei message header");
        do
        {
            Get_B1 (payload_type_byte,                          "payload_type_byte");
            payloadType+=payload_type_byte;
        }
        while(payload_type_byte==0xFF);
        do
        {
            Get_B1 (payload_size_byte,                          "payload_size_byte");
            payloadSize+=payload_size_byte;
        }
        while(payload_size_byte==0xFF);
    Element_End0();

    //Manage buggy files not having final bit stop
    const int8u* Buffer_Buggy;
    int64u Buffer_Offset_Buggy, Element_Size_Buggy;
    if (Element_Offset+payloadSize>Element_Size)
    {
        Buffer_Buggy=Buffer;
        Buffer_Offset_Buggy=Buffer_Offset;
        Element_Size_Buggy=Element_Size;
        Element_Size=Element_Offset+payloadSize;
        Buffer=new int8u[(size_t)Element_Size];
        Buffer_Offset=0;
        memcpy((void*)Buffer, Buffer_Buggy+Buffer_Offset, (size_t)Element_Size_Buggy);
        memset((void*)(Buffer+(size_t)Element_Size_Buggy), 0x00, (size_t)(Element_Size-Element_Size_Buggy)); //Last 0x00 bytes are discarded, we recreate them
    }
    else
        Buffer_Buggy=NULL;

    int64u Element_Offset_Save=Element_Offset+payloadSize;
    if (Element_Offset_Save>Element_Size)
    {
        Trusted_IsNot("Wrong size");
        Skip_XX(Element_Size-Element_Offset,                    "unknown");
        return;
    }
    int64u Element_Size_Save=Element_Size;
    Element_Size=Element_Offset_Save;
    switch (payloadType)
    {
        case   0 :   sei_message_buffering_period(seq_parameter_set_id, payloadSize); break;
        case   1 :   sei_message_pic_timing(seq_parameter_set_id, payloadSize); break;
        case   4 :   sei_message_user_data_registered_itu_t_t35(); break;
        case   5 :   sei_message_user_data_unregistered(payloadSize); break;
        case   6 :   sei_message_recovery_point(); break;
        //case  32 :   sei_message_mainconcept(payloadSize); break;
        case 129 :   sei_message_active_parameter_sets(); break;
        case 132 :   sei_message_decoded_picture_hash(payloadSize); break;
        case 136 :   sei_time_code(); break;
        case 137 :   sei_message_mastering_display_colour_volume(); break;
        case 144 :   sei_message_light_level(); break;
        case 147 :   sei_alternative_transfer_characteristics(); break;
        case 176 :   three_dimensional_reference_displays_info(payloadSize); break;
        default :
                    Element_Info1("unknown");
                    Skip_XX(payloadSize,                        "data");
    }
    Element_Offset=Element_Offset_Save; //Positionning in the right place.
    Element_Size=Element_Size_Save; //Positionning in the right place.

    //Manage buggy files not having final bit stop
    if (Buffer_Buggy)
    {
        delete[] Buffer;
        Buffer=Buffer_Buggy;
        Buffer_Offset=Buffer_Offset_Buggy;
        Element_Size=Element_Size_Buggy;
    }
}

//---------------------------------------------------------------------------
// SEI - 0
void File_Hevc::sei_message_buffering_period(int32u &seq_parameter_set_id, int32u payloadSize)
{
    Element_Info1("buffering_period");

    //Parsing
    if (Element_Offset==Element_Size)
        return; //Nothing to do
    BS_Begin();
    Get_UE (seq_parameter_set_id,                               "seq_parameter_set_id");
    std::vector<seq_parameter_set_struct*>::iterator seq_parameter_set_Item;
    if (seq_parameter_set_id>=seq_parameter_sets.size() || (*(seq_parameter_set_Item=seq_parameter_sets.begin()+seq_parameter_set_id))==NULL)
    {
        //Not yet present
        Skip_BS(Data_BS_Remain(),                               "Data (seq_parameter_set is missing)");
        BS_End();
        RiskCalculationN++;
        RiskCalculationD++;
        return;
    }
    bool sub_pic_hrd_params_present_flag=false; //Default
    bool irap_cpb_params_present_flag=((*seq_parameter_set_Item)->vui_parameters && (*seq_parameter_set_Item)->vui_parameters->xxL_Common)?(*seq_parameter_set_Item)->vui_parameters->xxL_Common->sub_pic_hrd_params_present_flag:false;
    if (!sub_pic_hrd_params_present_flag)
        Get_SB (irap_cpb_params_present_flag,                   "irap_cpb_params_present_flag");
    int8u au_cpb_removal_delay_length_minus1=((*seq_parameter_set_Item)->vui_parameters && (*seq_parameter_set_Item)->vui_parameters->xxL_Common)?(*seq_parameter_set_Item)->vui_parameters->xxL_Common->au_cpb_removal_delay_length_minus1:23;
    int8u dpb_output_delay_length_minus1=((*seq_parameter_set_Item)->vui_parameters && (*seq_parameter_set_Item)->vui_parameters->xxL_Common)?(*seq_parameter_set_Item)->vui_parameters->xxL_Common->dpb_output_delay_length_minus1:23;
    if (irap_cpb_params_present_flag)
    {
         Skip_S4(au_cpb_removal_delay_length_minus1+1,          "cpb_delay_offset");
         Skip_S4(dpb_output_delay_length_minus1+1,              "dpb_delay_offset");
    }
    Skip_SB(                                                    "concatenation_flag");
    Skip_S4(au_cpb_removal_delay_length_minus1+1,               "au_cpb_removal_delay_delta_minus1");
    if ((*seq_parameter_set_Item)->NalHrdBpPresentFlag())
        sei_message_buffering_period_xxl((*seq_parameter_set_Item)->vui_parameters?(*seq_parameter_set_Item)->vui_parameters->xxL_Common:NULL, irap_cpb_params_present_flag, (*seq_parameter_set_Item)->vui_parameters->NAL);
    if ((*seq_parameter_set_Item)->VclHrdBpPresentFlag())
        sei_message_buffering_period_xxl((*seq_parameter_set_Item)->vui_parameters?(*seq_parameter_set_Item)->vui_parameters->xxL_Common:NULL, irap_cpb_params_present_flag, (*seq_parameter_set_Item)->vui_parameters->VCL);
    BS_End();
}

void File_Hevc::sei_message_buffering_period_xxl(seq_parameter_set_struct::vui_parameters_struct::xxl_common* xxL_Common, bool irap_cpb_params_present_flag, seq_parameter_set_struct::vui_parameters_struct::xxl* xxl)
{
    if (xxL_Common==NULL || xxl==NULL)
    {
        //Problem?
        Skip_BS(Data_BS_Remain(),                               "Problem?");
        return;
    }
    for (int32u SchedSelIdx=0; SchedSelIdx<xxl->SchedSel.size(); SchedSelIdx++)
    {
        //Get_S4 (xxl->SchedSel[SchedSelIdx].initial_cpb_removal_delay_length_minus1+1, xxl->SchedSel[SchedSelIdx].initial_cpb_removal_delay, "initial_cpb_removal_delay"); Param_Info2(xxl->SchedSel[SchedSelIdx].initial_cpb_removal_delay/90, " ms");
        //Get_S4 (xxl->SchedSel[SchedSelIdx].initial_cpb_removal_delay_length_minus1+1, xxl->SchedSel[SchedSelIdx].initial_cpb_removal_delay_offset, "initial_cpb_removal_delay_offset"); Param_Info2(xxl->SchedSel[SchedSelIdx].initial_cpb_removal_delay_offset/90, " ms");
        Info_S4 (xxL_Common->initial_cpb_removal_delay_length_minus1+1, initial_cpb_removal_delay, "initial_cpb_removal_delay"); Param_Info2(initial_cpb_removal_delay/90, " ms");
        Info_S4 (xxL_Common->initial_cpb_removal_delay_length_minus1+1, initial_cpb_removal_delay_offset, "initial_cpb_removal_delay_offset"); Param_Info2(initial_cpb_removal_delay_offset/90, " ms");
        if (xxL_Common->sub_pic_hrd_params_present_flag || irap_cpb_params_present_flag)
        {
            Info_S4 (xxL_Common->initial_cpb_removal_delay_length_minus1+1, initial_alt_cpb_removal_delay, "initial_alt_cpb_removal_delay"); Param_Info2(initial_alt_cpb_removal_delay/90, " ms");
            Info_S4 (xxL_Common->initial_cpb_removal_delay_length_minus1+1, initial_alt_cpb_removal_delay_offset, "initial_alt_cpb_removal_delay_offset"); Param_Info2(initial_alt_cpb_removal_delay_offset/90, " ms");
        }
    }
}

//---------------------------------------------------------------------------
// SEI - 1
void File_Hevc::sei_message_pic_timing(int32u &seq_parameter_set_id, int32u payloadSize)
{
    Element_Info1("pic_timing");

    //Testing if we can parsing it now. TODO: handle case seq_parameter_set_id is unknown (buffering of message, decoding in slice parsing)
    if (seq_parameter_set_id==(int32u)-1 && seq_parameter_sets.size()==1)
        seq_parameter_set_id=0;
    std::vector<seq_parameter_set_struct*>::iterator seq_parameter_set_Item;
    if (seq_parameter_set_id>=seq_parameter_sets.size() || (*(seq_parameter_set_Item=seq_parameter_sets.begin()+seq_parameter_set_id))==NULL)
    {
        //Not yet present
        Skip_BS(Data_BS_Remain(),                               "Data (seq_parameter_set is missing)");
        return;
    }

    //Parsing
    BS_Begin();
    if ((*seq_parameter_set_Item)->vui_parameters?(*seq_parameter_set_Item)->vui_parameters->flags[frame_field_info_present_flag]:((*seq_parameter_set_Item)->profile_tier_level_info.general_progressive_source_flag && (*seq_parameter_set_Item)->profile_tier_level_info.general_interlaced_source_flag))
    {
        Skip_S1(4,                                              "pic_struct");
        Skip_S1(2,                                              "source_scan_type");
        Skip_SB(                                                "duplicate_flag");
    }
    if ((*seq_parameter_set_Item)->CpbDpbDelaysPresentFlag())
    {
        int8u au_cpb_removal_delay_length_minus1=(*seq_parameter_set_Item)->vui_parameters->xxL_Common->au_cpb_removal_delay_length_minus1;
        int8u dpb_output_delay_length_minus1=(*seq_parameter_set_Item)->vui_parameters->xxL_Common->dpb_output_delay_length_minus1;
        bool  sub_pic_hrd_params_present_flag=(*seq_parameter_set_Item)->vui_parameters->xxL_Common->sub_pic_hrd_params_present_flag;
        Skip_S4(au_cpb_removal_delay_length_minus1+1,           "au_cpb_removal_delay_minus1");
        Skip_S4(dpb_output_delay_length_minus1+1,               "pic_dpb_output_delay");
        if (sub_pic_hrd_params_present_flag)
        {
            int8u dpb_output_delay_du_length_minus1=(*seq_parameter_set_Item)->vui_parameters->xxL_Common->dpb_output_delay_du_length_minus1;
            Skip_S4(dpb_output_delay_du_length_minus1+1,        "pic_dpb_output_du_delay");
        }
    }
    BS_End();
}

//---------------------------------------------------------------------------
// SEI - 4
void File_Hevc::sei_message_user_data_registered_itu_t_t35()
{
    Element_Info1("user_data_registered_itu_t_t35");

    int8u itu_t_t35_country_code;
    Get_B1(itu_t_t35_country_code,                              "itu_t_t35_country_code");

    switch (itu_t_t35_country_code)
    {
        case 0xB5:  sei_message_user_data_registered_itu_t_t35_B5(); break; // USA
        case 0x26:  sei_message_user_data_registered_itu_t_t35_26(); break; // China
    }
}

//---------------------------------------------------------------------------
// SEI - 4 - USA
void File_Hevc::sei_message_user_data_registered_itu_t_t35_B5()
{
    int16u itu_t_t35_terminal_provider_code;
    Get_B2 (itu_t_t35_terminal_provider_code,                   "itu_t_t35_terminal_provider_code");

    switch (itu_t_t35_terminal_provider_code)
    {
        case 0x0031: sei_message_user_data_registered_itu_t_t35_B5_0031(); break;
        case 0x003A: sei_message_user_data_registered_itu_t_t35_B5_003A(); break;
        case 0x003C: sei_message_user_data_registered_itu_t_t35_B5_003C(); break;
    }
}

//---------------------------------------------------------------------------
// SEI - 4 - USA - 0031
void File_Hevc::sei_message_user_data_registered_itu_t_t35_B5_0031()
{
    int32u Identifier;
    Peek_B4(Identifier);
    switch (Identifier)
    {
        case 0x44544731 :   sei_message_user_data_registered_itu_t_t35_B5_0031_DTG1(); return;
        case 0x47413934 :   sei_message_user_data_registered_itu_t_t35_B5_0031_GA94(); return;
        default         :   if (Element_Size-Element_Offset)
                                Skip_XX(Element_Size-Element_Offset, "Unknown");
    }
}

//---------------------------------------------------------------------------
// SEI - 4 - USA - 0031 - DTG1
void File_Hevc::sei_message_user_data_registered_itu_t_t35_B5_0031_DTG1()
{
    Element_Info1("Active Format Description");

    //Parsing
    Skip_C4(                                                    "afd_identifier");
    if (Element_Offset<Element_Size)
    {
        File_AfdBarData DTG1_Parser;
        for (auto seq_parameter_set_Item : seq_parameter_sets)
        {
            if (seq_parameter_set_Item && seq_parameter_set_Item->vui_parameters && seq_parameter_set_Item->vui_parameters->sar_width && seq_parameter_set_Item->vui_parameters->sar_height)
            {
                //TODO: avoid duplicated code
                int32u Width = seq_parameter_set_Item->pic_width_in_luma_samples;
                int32u Height= seq_parameter_set_Item->pic_height_in_luma_samples;
                int8u chromaArrayType = seq_parameter_set_Item->ChromaArrayType();
                if (chromaArrayType >= 4)
                    chromaArrayType = 0;
                int32u CropUnitX=Hevc_SubWidthC [chromaArrayType];
                int32u CropUnitY=Hevc_SubHeightC[chromaArrayType];
                Width -=(seq_parameter_set_Item->conf_win_left_offset+seq_parameter_set_Item->conf_win_right_offset)*CropUnitX;
                Height-=(seq_parameter_set_Item->conf_win_top_offset +seq_parameter_set_Item->conf_win_bottom_offset)*CropUnitY;
                if (Height)
                {
                    float32 PixelAspectRatio=((float32)seq_parameter_set_Item->vui_parameters->sar_width) / seq_parameter_set_Item->vui_parameters->sar_height;
                    auto DAR=Width*PixelAspectRatio/Height;
                    if (DAR>=4.0/3.0*0.95 && DAR<4.0/3.0*1.05) DTG1_Parser.aspect_ratio_FromContainer=0; //4/3
                    if (DAR>=16.0/9.0*0.95 && DAR<16.0/9.0*1.05) DTG1_Parser.aspect_ratio_FromContainer=1; //16/9
                }
                break;
            }
        }
        Open_Buffer_Init(&DTG1_Parser);
        DTG1_Parser.Format=File_AfdBarData::Format_A53_4_DTG1;
        Open_Buffer_Continue(&DTG1_Parser, Buffer+Buffer_Offset+(size_t)Element_Offset, Element_Size-Element_Offset);
        Merge(DTG1_Parser, Stream_Video, 0, 0);
        Element_Offset=Element_Size;
    }
}

//---------------------------------------------------------------------------
// SEI - 4 - USA - 0031 - GA94
void File_Hevc::sei_message_user_data_registered_itu_t_t35_B5_0031_GA94()
{
    //Parsing
    int8u user_data_type_code;
    Skip_B4("GA94_identifier");
    Get_B1(user_data_type_code, "user_data_type_code");
    switch (user_data_type_code)
    {
        case 0x03: sei_message_user_data_registered_itu_t_t35_B5_0031_GA94_03(); break;
        case 0x09: sei_message_user_data_registered_itu_t_t35_B5_0031_GA94_09(); break;
        default: Skip_XX(Element_Size - Element_Offset, "GA94_reserved_user_data");
    }
}

//---------------------------------------------------------------------------
// SEI - 4 - USA - 0031 - GA94 - 03
void File_Hevc::sei_message_user_data_registered_itu_t_t35_B5_0031_GA94_03()
{
    #if defined(MEDIAINFO_DTVCCTRANSPORT_YES)
        GA94_03_IsPresent=true;
        MustExtendParsingDuration=true;
        Buffer_TotalBytes_Fill_Max=(int64u)-1; //Disabling this feature for this format, this is done in the parser

        Element_Info1("DTVCC Transport");

        //Coherency
        delete TemporalReferences_DelayedElement; TemporalReferences_DelayedElement=new temporal_reference();

        TemporalReferences_DelayedElement->GA94_03=new buffer_data(Buffer+Buffer_Offset+(size_t)Element_Offset,(size_t)(Element_Size-Element_Offset));

        //Parsing
        Skip_XX(Element_Size-Element_Offset,                    "CC data");
    #else //defined(MEDIAINFO_DTVCCTRANSPORT_YES)
        Skip_XX(Element_Size-Element_Offset,                    "DTVCC Transport data");
    #endif //defined(MEDIAINFO_DTVCCTRANSPORT_YES)
}

void File_Hevc::sei_message_user_data_registered_itu_t_t35_B5_0031_GA94_03_Delayed(int32u seq_parameter_set_id)
{
    // Skipping missing frames
    if (TemporalReferences_Max-TemporalReferences_Min>(size_t)(4*(seq_parameter_sets[seq_parameter_set_id]->sps_max_num_reorder_pics+3))) // max_num_ref_frames ref frame maximum
    {
        size_t TemporalReferences_Min_New=TemporalReferences_Max-4*(seq_parameter_sets[seq_parameter_set_id]->sps_max_num_reorder_pics+3);
        while (TemporalReferences_Min_New>TemporalReferences_Min && TemporalReferences[TemporalReferences_Min_New-1])
            TemporalReferences_Min_New--;
        TemporalReferences_Min=TemporalReferences_Min_New;
        while (TemporalReferences[TemporalReferences_Min]==NULL)
        {
            TemporalReferences_Min++;
            if (TemporalReferences_Min>=TemporalReferences.size())
                return;
        }
    }

    // Parsing captions
    while (TemporalReferences[TemporalReferences_Min] && TemporalReferences_Min+2*seq_parameter_sets[seq_parameter_set_id]->sps_max_num_reorder_pics<TemporalReferences_Max)
    {
        #if defined(MEDIAINFO_DTVCCTRANSPORT_YES)
            Element_Begin1("Reordered DTVCC Transport");

            //Parsing
            #if MEDIAINFO_DEMUX
                int64u Element_Code_Old=Element_Code;
                Element_Code=0x4741393400000003LL;
            #endif //MEDIAINFO_DEMUX
            if (GA94_03_Parser==NULL)
            {
                GA94_03_Parser=new File_DtvccTransport;
                Open_Buffer_Init(GA94_03_Parser);
                ((File_DtvccTransport*)GA94_03_Parser)->Format=File_DtvccTransport::Format_A53_4_GA94_03;
            }
            if (((File_DtvccTransport*)GA94_03_Parser)->AspectRatio==0)
            {
                std::vector<seq_parameter_set_struct*>::iterator seq_parameter_set_Item_=seq_parameter_sets.begin();
                for (; seq_parameter_set_Item_!=seq_parameter_sets.end(); ++seq_parameter_set_Item_)
                    if ((*seq_parameter_set_Item_))
                        break;
                if (seq_parameter_set_Item_!=seq_parameter_sets.end())
                {
                    const auto seq_parameter_set_Item=*seq_parameter_set_Item_;
                    const auto vui_parameters=seq_parameter_set_Item->vui_parameters;
                    if (vui_parameters && vui_parameters->sar_width && vui_parameters->sar_height)
                    {
                        const int32u Width =seq_parameter_set_Item->pic_width_in_luma_samples;
                        const int32u Height=seq_parameter_set_Item->pic_height_in_luma_samples;
                        if (Height)
                        {
                            auto PixelAspectRatio=((float32)seq_parameter_set_Item->vui_parameters->sar_width)/seq_parameter_set_Item->vui_parameters->sar_height;
                            ((File_DtvccTransport*)GA94_03_Parser)->AspectRatio=Width*PixelAspectRatio/Height;
                        }
                    }
                }
            }
            if (GA94_03_Parser->PTS_DTS_Needed)
            {
                GA94_03_Parser->FrameInfo.PCR=FrameInfo.PCR;
                GA94_03_Parser->FrameInfo.PTS=FrameInfo.PTS;
                GA94_03_Parser->FrameInfo.DTS=FrameInfo.DTS;
            }
            #if MEDIAINFO_DEMUX
                if (TemporalReferences[TemporalReferences_Min]->GA94_03)
                {
                    int8u Demux_Level_Save=Demux_Level;
                    Demux_Level=8; //Ancillary
                    Demux(TemporalReferences[TemporalReferences_Min]->GA94_03->Data, TemporalReferences[TemporalReferences_Min]->GA94_03->Size, ContentType_MainStream);
                    Demux_Level=Demux_Level_Save;
                }
                Element_Code=Element_Code_Old;
            #endif //MEDIAINFO_DEMUX
            if (TemporalReferences[TemporalReferences_Min]->GA94_03)
            {
                #if defined(MEDIAINFO_EIA608_YES) || defined(MEDIAINFO_EIA708_YES)
                    GA94_03_Parser->ServiceDescriptors=ServiceDescriptors;
                #endif
                Open_Buffer_Continue(GA94_03_Parser, TemporalReferences[TemporalReferences_Min]->GA94_03->Data, TemporalReferences[TemporalReferences_Min]->GA94_03->Size);
            }

            Element_End0();
        #endif //defined(MEDIAINFO_DTVCCTRANSPORT_YES)

        //TemporalReferences_Min+=((seq_parameter_sets[seq_parameter_set_id]->frame_mbs_only_flag | !TemporalReferences[TemporalReferences_Min]->IsField)?2:1);
        TemporalReferences_Min++;
    }
}

//---------------------------------------------------------------------------
// SEI - 4 - USA - 0031 - GA94 - 09 - SMPTE ST 2094-10
static const char* Smpte209410_BlockNames[]=
{
    nullptr,
    "Content Range",
    "Trim Pass",
    nullptr,
    nullptr,
    "Active Area",
};
static const auto Smpte209410_BlockNames_Size=sizeof(Smpte209410_BlockNames)/sizeof(decltype(*Smpte209410_BlockNames));
void File_Hevc::sei_message_user_data_registered_itu_t_t35_B5_0031_GA94_09()
{
    Element_Info1("SMPTE ST 2094-10");
    int32u app_identifier, app_version;
    bool metadata_refresh_flag;
    vector<int32u> ext_block_level_List;
    BS_Begin();
    Get_UE(app_identifier, "app_identifier");
    if (app_identifier!=1)
        return;
    Get_UE (app_version,                                        "app_version");
    if (!app_version)
    {
        Get_SB(metadata_refresh_flag,                           "metadata_refresh_flag");
        if (metadata_refresh_flag)
        {
            int32u num_ext_blocks;
            Get_UE (num_ext_blocks,                             "num_ext_blocks");
            if (num_ext_blocks)
            {
                auto Align=Data_BS_Remain()%8;
                if (Align)
                    Skip_BS(Align,                              "dm_alignment_zero_bits");
                for (int32u i=0; i<num_ext_blocks; i++)
                {
                    Element_Begin1("block");
                    Element_Begin1("Header");
                    int32u ext_block_length;
                    int8u ext_block_level;
                    Get_UE (ext_block_length,                   "ext_block_length");
                    Get_S1 (8, ext_block_level,                 "ext_block_level");
                    Element_End0();
                    Element_Info1((ext_block_level<Smpte209410_BlockNames_Size && Smpte209410_BlockNames[ext_block_level])?Smpte209410_BlockNames[ext_block_level]:to_string(ext_block_level).c_str());
                    if (ext_block_length>Data_BS_Remain())
                    {
                        Element_End0();
                        Trusted_IsNot("Coherency");
                        break;
                    }
                    ext_block_length*=8;
                    if (ext_block_length>Data_BS_Remain())
                    {
                        Element_End0();
                        Trusted_IsNot("Coherency");
                        break;
                    }
                    auto End=Data_BS_Remain()-ext_block_length;
                    ext_block_level_List.push_back(ext_block_level);
                    switch (ext_block_level)
                    {
                        case 1:
                            Skip_S2(12,                         "min_PQ");
                            Skip_S2(12,                         "max_PQ");
                            Skip_S2(12,                         "avg_PQ");
                            break;
                        case 2:
                            Skip_S2(12,                         "target_max_PQ");
                            Skip_S2(12,                         "trim_slope");
                            Skip_S2(12,                         "trim_offset");
                            Skip_S2(12,                         "trim_power");
                            Skip_S2(12,                         "trim_chroma_weight");
                            Skip_S2(12,                         "trim_saturation_gain");
                            Skip_S1( 3,                         "ms_weight");
                            break;
                        case 3:
                            Skip_S2(12,                         "min_PQ_offset");
                            Skip_S2(12,                         "max_PQ_offset");
                            Skip_S2(12,                         "avg_PQ_offset");
                            break;
                        case 4:
                            Skip_S2(12,                         "TF_PQ_mean");
                            Skip_S2(12,                         "TF_PQ_stdev");
                            break;
                        case 5:
                            Skip_S2(13,                         "active_area_left_offset");
                            Skip_S2(13,                         "active_area_right_offset");
                            Skip_S2(13,                         "active_area_top_offset");
                            Skip_S2(13,                         "active_area_bottom_offset");
                            break;
                    }
                    if (Data_BS_Remain()>End)
                    {
                        auto Align=Data_BS_Remain()-End;
                        if (Align)
                            Skip_BS(Align,                      Align>=8?"(Unknown)":"dm_alignment_zero_bits");
                    }
                    Element_End0();
                }
            }
        }
        auto Align=Data_BS_Remain()%8;
        if (Align)
            Skip_BS(Align,                                      Align>=8?"(Unknown)":"dm_alignment_zero_bits");
        BS_End();
    }

    auto& HDR_Format=HDR[Video_HDR_Format][HdrFormat_SmpteSt209410];
    if (HDR_Format.empty())
    {
        HDR_Format=__T("SMPTE ST 2094-10");
        FILLING_BEGIN();
            HDR[Video_HDR_Format_Version][HdrFormat_SmpteSt209410].From_Number(app_version);
        FILLING_END();
    }
    if (!Trusted_Get())
    {
        Fill(Stream_Video, 0, "ConformanceErrors", 1, 10, true);
        Fill(Stream_Video, 0, "ConformanceErrors SMPTE_ST_2049_CVT", 1, 10, true);
        Fill(Stream_Video, 0, "ConformanceErrors SMPTE_ST_2049_CVT Coherency", "Bitstream parsing ran out of data to read before the end of the syntax was reached, most probably the bitstream is malformed", Unlimited, true, true);
    }
}

//---------------------------------------------------------------------------
// SEI - 4 - USA - 003A
void File_Hevc::sei_message_user_data_registered_itu_t_t35_B5_003A()
{
    int8u itu_t_t35_terminal_provider_oriented_code;
    Get_B1 (itu_t_t35_terminal_provider_oriented_code,          "itu_t_t35_terminal_provider_oriented_code");

    switch (itu_t_t35_terminal_provider_oriented_code)
    {
        case 0x00: sei_message_user_data_registered_itu_t_t35_B5_003A_00(); break;
        case 0x02: sei_message_user_data_registered_itu_t_t35_B5_003A_02(); break;
    }
}

//---------------------------------------------------------------------------
// SEI - 4 - USA - 003A - ETSI 103-433-1
void File_Hevc::sei_message_user_data_registered_itu_t_t35_B5_003A_00()
{
    Element_Info1("SL-HDR message");
    BS_Begin();
    int8u sl_hdr_mode_value_minus1, sl_hdr_spec_major_version_idc, sl_hdr_spec_minor_version_idc;
    bool sl_hdr_cancel_flag;
    Get_S1 (4, sl_hdr_mode_value_minus1,                        "sl_hdr_mode_value_minus1");
    Get_S1 (4, sl_hdr_spec_major_version_idc,                   "sl_hdr_spec_major_version_idc");
    Get_S1 (7, sl_hdr_spec_minor_version_idc,                   "sl_hdr_spec_minor_version_idc");
    Get_SB (sl_hdr_cancel_flag,                                 "sl_hdr_cancel_flag");
    int8u sl_hdr_payload_mode;
    int8u k_coefficient_value[3];
    if (!sl_hdr_cancel_flag)
    {
        mastering_metadata_2086 Meta;
        bool coded_picture_info_present_flag, target_picture_info_present_flag, src_mdcv_info_present_flag;
        Skip_SB(                                                "sl_hdr_persistence_flag");
        Get_SB (coded_picture_info_present_flag,                "coded_picture_info_present_flag");
        Get_SB (target_picture_info_present_flag,               "target_picture_info_present_flag");
        Get_SB (src_mdcv_info_present_flag,                     "src_mdcv_info_present_flag");
        Skip_SB(                                                "sl_hdr_extension_present_flag");
        Get_S1 (3, sl_hdr_payload_mode,                         "sl_hdr_payload_mode");
        BS_End();
        if (coded_picture_info_present_flag)
        {
            Skip_B1(                                            "coded_picture_primaries");
            Skip_B2(                                            "coded_picture_max_luminance");
            Skip_B2(                                            "coded_picture_min_luminance");
        }
        if (target_picture_info_present_flag)
        {
            Skip_B1(                                            "target_picture_primaries");
            Skip_B2(                                            "target_picture_max_luminance");
            Skip_B2(                                            "target_picture_min_luminance");
        }
        if (src_mdcv_info_present_flag)
        {
            int16u max, min;
            for (int8u i = 0; i < 3; i++)
            {
                Get_B2 (Meta.Primaries[i*2  ],                  "src_mdcv_primaries_x");
                Get_B2 (Meta.Primaries[i*2+1],                  "src_mdcv_primaries_y");
            }
            Get_B2 (Meta.Primaries[3*2  ],                      "src_mdcv_ref_white_x");
            Get_B2 (Meta.Primaries[3*2+1],                      "src_mdcv_ref_white_y");
            Get_B2 (max,                                        "src_mdcv_max_mastering_luminance");
            Get_B2 (min,                                        "src_mdcv_min_mastering_luminance");
            Meta.Luminance[0]=min;
            Meta.Luminance[1]=((int32u)max)*10000;
        }
        for (int8u i = 0; i < 4; i++)
            Skip_B2(                                            "matrix_coefficient_value");
        for (int8u i = 0; i < 2; i++)
            Skip_B2(                                            "chroma_to_luma_injection");
        for (int8u i = 0; i < 3; i++)
            Get_B1 (k_coefficient_value[i],                     "k_coefficient_value");

        FILLING_BEGIN()
            auto& HDR_Format=HDR[Video_HDR_Format][HdrFormat_EtsiTs103433];
            if (HDR_Format.empty())
            {
                HDR_Format=__T("SL-HDR")+Ztring().From_Number(sl_hdr_mode_value_minus1+1);
                HDR[Video_HDR_Format_Version][HdrFormat_EtsiTs103433]=Ztring().From_Number(sl_hdr_spec_major_version_idc)+__T('.')+Ztring().From_Number(sl_hdr_spec_minor_version_idc);
                Get_MasteringDisplayColorVolume(HDR[Video_MasteringDisplay_ColorPrimaries][HdrFormat_EtsiTs103433], HDR[Video_MasteringDisplay_Luminance][HdrFormat_EtsiTs103433], Meta);
                auto& HDR_Format_Settings=HDR[Video_HDR_Format_Settings][HdrFormat_EtsiTs103433];
                if (sl_hdr_payload_mode<2)
                    HDR_Format_Settings=sl_hdr_payload_mode?__T("Table-based"):__T("Parameter-based");
                else
                    HDR_Format_Settings=__T("Payload Mode ") + Ztring().From_Number(sl_hdr_payload_mode);
                if (!sl_hdr_mode_value_minus1)
                    HDR_Format_Settings+=k_coefficient_value[0]==0 && k_coefficient_value[1]==0 && k_coefficient_value[2]==0?__T(", non-constant"):__T(", constant");

                EtsiTS103433 = __T("SL-HDR") + Ztring().From_Number(sl_hdr_mode_value_minus1 + 1);
                if (!sl_hdr_mode_value_minus1)
                    EtsiTS103433 += k_coefficient_value[0] == 0 && k_coefficient_value[1] == 0 && k_coefficient_value[2] == 0 ? __T(" NCL") : __T(" CL");
                EtsiTS103433 += __T(" specVersion=") + Ztring().From_Number(sl_hdr_spec_major_version_idc) + __T(".") + Ztring().From_Number(sl_hdr_spec_minor_version_idc);
                EtsiTS103433 += __T(" payloadMode=") + Ztring().From_Number(sl_hdr_payload_mode);
            }
        FILLING_END();
    }
    else
        BS_End();
}

//---------------------------------------------------------------------------
// SEI - 4 - USA - 003A - ETSI 103-433
void File_Hevc::sei_message_user_data_registered_itu_t_t35_B5_003A_02()
{
    Element_Info1("SL-HDR information");
    int8u ts_103_433_spec_version;
    BS_Begin();
    Get_S1 (4, ts_103_433_spec_version,                         "ts_103_433_spec_version");
    if (ts_103_433_spec_version==0)
    {
        Skip_S1 (4,                                             "ts_103_433_payload_mode");
    }
    else if (ts_103_433_spec_version==1)
    {
        Skip_S1 (3,                                             "sl_hdr_mode_support");
    }
    else
        Skip_S1 (Data_BS_Remain(),                              "Unknown");
    BS_End();
}

//---------------------------------------------------------------------------
// SEI - 4 - USA - 003C
void File_Hevc::sei_message_user_data_registered_itu_t_t35_B5_003C()
{
    int16u itu_t_t35_terminal_provider_oriented_code;
    Get_B2 (itu_t_t35_terminal_provider_oriented_code,          "itu_t_t35_terminal_provider_oriented_code");

    switch (itu_t_t35_terminal_provider_oriented_code)
    {
        case 0x0001: sei_message_user_data_registered_itu_t_t35_B5_003C_0001(); break;
    }
}

//---------------------------------------------------------------------------
// SEI - 4 - USA - 003C - 0001
void File_Hevc::sei_message_user_data_registered_itu_t_t35_B5_003C_0001()
{
    int8u application_identifier;
    Get_B1 (application_identifier,                             "application_identifier");

    switch (application_identifier)
    {
        case 0x04: sei_message_user_data_registered_itu_t_t35_B5_003C_0001_04(); break;
    }
}

//---------------------------------------------------------------------------
// SEI - 4 - USA - 003C - 0001 - SMPTE ST 2094-40 (HDR10+)
void File_Hevc::sei_message_user_data_registered_itu_t_t35_B5_003C_0001_04()
{
    Element_Info1("SMPTE ST 2094 App 4");
    int8u application_version;
    bool IsHDRplus=false, tone_mapping_flag;
    Get_B1 (application_version,                                "application_version");
    if (application_version==1)
    {
        int32u targeted_system_display_maximum_luminance, maxscl[4]{}, distribution_maxrgb_percentiles[16];
        int16u fraction_bright_pixels;
        int8u num_distribution_maxrgb_percentiles, distribution_maxrgb_percentages[16], num_windows, num_bezier_curve_anchors;
        bool targeted_system_display_actual_peak_luminance_flag, mastering_display_actual_peak_luminance_flag, color_saturation_mapping_flag;
        BS_Begin();
        Get_S1 ( 2, num_windows,                                "num_windows");

        for (int8u w=1; w<num_windows; w++)
        {
            Element_Begin1("window");
            Skip_S2(16,                                         "window_upper_left_corner_x");
            Skip_S2(16,                                         "window_upper_left_corner_y");
            Skip_S2(16,                                         "window_lower_right_corner_x");
            Skip_S2(16,                                         "window_lower_right_corner_y");
            Skip_S2(16,                                         "center_of_ellipse_x");
            Skip_S2(16,                                         "center_of_ellipse_y");
            Skip_S1( 8,                                         "rotation_angle");
            Skip_S2(16,                                         "semimajor_axis_internal_ellipse");
            Skip_S2(16,                                         "semimajor_axis_external_ellipse");
            Skip_S2(16,                                         "semiminor_axis_external_ellipse");
            Skip_SB(                                            "overlap_process_option");
            Element_End0();
        }

        Get_S4 (27, targeted_system_display_maximum_luminance,  "targeted_system_display_maximum_luminance");
        TEST_SB_GET (targeted_system_display_actual_peak_luminance_flag, "targeted_system_display_actual_peak_luminance_flag");
            int8u num_rows_targeted_system_display_actual_peak_luminance, num_cols_targeted_system_display_actual_peak_luminance;
            Get_S1(5, num_rows_targeted_system_display_actual_peak_luminance, "num_rows_targeted_system_display_actual_peak_luminance");
            Get_S1(5, num_cols_targeted_system_display_actual_peak_luminance, "num_cols_targeted_system_display_actual_peak_luminance");
            for(int8u i=0; i<num_rows_targeted_system_display_actual_peak_luminance; i++)
                for(int8u j=0; j<num_cols_targeted_system_display_actual_peak_luminance; j++)
                    Skip_S1(4,                                   "targeted_system_display_actual_peak_luminance");
        TEST_SB_END();

        for (int8u w=0; w<num_windows; w++)
        {
            Element_Begin1("window");
            for(int8u i=0; i<3; i++)
            {
                Get_S3 (17, maxscl[i],                          "maxscl"); Param_Info2(Ztring::ToZtring(((float)maxscl[i])/100000, 5), " cd/m2");
            }
            Get_S3 (17, maxscl[3],                              "average_maxrgb");   Param_Info2(Ztring::ToZtring(((float)maxscl[3])/100000, 5), " cd/m2");

            Get_S1(4, num_distribution_maxrgb_percentiles,      "num_distribution_maxrgb_percentiles");
            for (int8u i=0; i< num_distribution_maxrgb_percentiles; i++)
            {
                Element_Begin1(                                 "distribution_maxrgb");
                Get_S1 ( 7, distribution_maxrgb_percentages[i], "distribution_maxrgb_percentages");
                Get_S3 (17, distribution_maxrgb_percentiles[i], "distribution_maxrgb_percentiles");
                Element_Info1(distribution_maxrgb_percentages[i]);
                Element_Info1(distribution_maxrgb_percentiles[i]);
                Element_End0();
            }
            Get_S2 (10, fraction_bright_pixels,                 "fraction_bright_pixels");
            Element_End0();
        }

        TEST_SB_GET (mastering_display_actual_peak_luminance_flag, "mastering_display_actual_peak_luminance_flag");
            int8u num_rows_mastering_display_actual_peak_luminance, num_cols_mastering_display_actual_peak_luminance;
            Get_S1(5, num_rows_mastering_display_actual_peak_luminance, "num_rows_mastering_display_actual_peak_luminance");
            Get_S1(5, num_cols_mastering_display_actual_peak_luminance, "num_cols_mastering_display_actual_peak_luminance");
            for(int8u i=0; i< num_rows_mastering_display_actual_peak_luminance; i++)
                for(int8u j=0; j< num_cols_mastering_display_actual_peak_luminance; j++)
                    Skip_S1(4,                                   "mastering_display_actual_peak_luminance");
        TEST_SB_END();

        for (int8u w=0; w<num_windows; w++)
        {
            Element_Begin1("window");
            TEST_SB_GET (tone_mapping_flag,                     "tone_mapping_flag");
                Skip_S2(12,                                     "knee_point_x");
                Skip_S2(12,                                     "knee_point_y");
                Get_S1(4, num_bezier_curve_anchors,             "num_bezier_curve_anchors");
                for (int8u i = 0; i < num_bezier_curve_anchors; i++)
                    Skip_S2(10,                                 "bezier_curve_anchor");
            TEST_SB_END();
            Element_End0();
        }
        TEST_SB_GET (color_saturation_mapping_flag,             "color_saturation_mapping_flag");
            Info_S1(6, color_saturation_weight,                 "color_saturation_weight"); Param_Info1(((float)color_saturation_weight)/8);
        TEST_SB_END();
        BS_End();

        FILLING_BEGIN();
            IsHDRplus=true;
            if (num_windows!=1 || targeted_system_display_actual_peak_luminance_flag || num_distribution_maxrgb_percentiles!=9 || fraction_bright_pixels || mastering_display_actual_peak_luminance_flag || (distribution_maxrgb_percentages[2]>100 && distribution_maxrgb_percentages[2]!=0xFF) || (!tone_mapping_flag && targeted_system_display_maximum_luminance) || (tone_mapping_flag && num_bezier_curve_anchors>9) || color_saturation_mapping_flag)
                IsHDRplus=false;
            for(int8u i=0; i<4; i++)
                if (maxscl[i]>100000)
                    IsHDRplus=false;
            if (IsHDRplus)
                for(int8u i=0; i<9; i++)
                {
                    static const int8u distribution_maxrgb_percentages_List[9]={1, 5, 10, 25, 50, 75, 90, 95, 99};
                    if (distribution_maxrgb_percentages[i]!=distribution_maxrgb_percentages_List[i])
                        IsHDRplus=false;
                    if (distribution_maxrgb_percentiles[i]>100000)
                        IsHDRplus=false;
                }
        FILLING_END();
    }

    FILLING_BEGIN();
        auto& HDR_Format=HDR[Video_HDR_Format][HdrFormat_SmpteSt209440];
        if (HDR_Format.empty())
        {
            HDR_Format=__T("SMPTE ST 2094 App 4");
            HDR[Video_HDR_Format_Version][HdrFormat_SmpteSt209440].From_Number(application_version);
            if (IsHDRplus)
                HDR[Video_HDR_Format_Compatibility][HdrFormat_SmpteSt209440]=tone_mapping_flag?__T("HDR10+ Profile B"):__T("HDR10+ Profile A");
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
// SEI - 4 - China
void File_Hevc::sei_message_user_data_registered_itu_t_t35_26()
{
    int16u itu_t_t35_terminal_provider_code;
    Get_B2 (itu_t_t35_terminal_provider_code,                   "itu_t_t35_terminal_provider_code");

    //HDR Vivid terminal provide code 0x0004, terminal provide oriented code 0x0005 , 0x0005 indicates HDR Vivid version 1
    switch (itu_t_t35_terminal_provider_code)
    {
        case 0x0004: sei_message_user_data_registered_itu_t_t35_26_0004(); break;
    }
}

//---------------------------------------------------------------------------
// SEI - 4 - China    similar to sei_message_user_data_registered_itu_t_t35_B5_003C()
void File_Hevc::sei_message_user_data_registered_itu_t_t35_26_0004()
{
    int16u itu_t_t35_terminal_provider_oriented_code;
    Get_B2 (itu_t_t35_terminal_provider_oriented_code,                   "itu_t_t35_terminal_provider_oriented_code");
    //HDR Vivid terminal provide code 0x0004, terminal provide oriented code 0x0005
    switch (itu_t_t35_terminal_provider_oriented_code)
    {
        case 0x0005: sei_message_user_data_registered_itu_t_t35_26_0004_0005(); break;
    }
}

//---------------------------------------------------------------------------
// SEI - 4 - China - 0004
void File_Hevc::sei_message_user_data_registered_itu_t_t35_26_0004_0005()
{
    //Parsing
    int16u targeted_system_display_maximum_luminance_Max=0;
    int8u system_start_code;
    bool color_saturation_mapping_flag;
    Get_B1(system_start_code,                                   "system_start_code");
    if (system_start_code!=0x01)
    {
        Skip_XX(Element_Size,                                   "Unknown");
        return;
    }
    BS_Begin();
    //int8u num_windows=1;
    //for (int8u w=0; w<num_windows; w++)
    {
        Skip_S2(12,                                             "minimum_maxrgb");
        Skip_S2(12,                                             "average_maxrgb");
        Skip_S2(12,                                             "variance_maxrgb");
        Skip_S2(12,                                             "maximum_maxrgb");
    }

    //for (int8u w=0; w<num_windows; w++)
    {
        bool tone_mapping_enable_mode_flag;
        Get_SB (tone_mapping_enable_mode_flag,                  "tone_mapping_mode_flag");
        if (tone_mapping_enable_mode_flag)
        {
            bool tone_mapping_param_enable_num_b;
            Get_SB (tone_mapping_param_enable_num_b,            "tone_mapping_param_num");
            int tone_mapping_param_enable_num=(int)tone_mapping_param_enable_num_b; // Just for avoiding some compiler warning
            for (int i=0; i<=tone_mapping_param_enable_num; i++)
            {
                Element_Begin1("tone_mapping_param");
                int16u targeted_system_display_maximum_luminance;
                bool base_enable_flag, ThreeSpline_enable_flag;
                Get_S2 (12, targeted_system_display_maximum_luminance, "targeted_system_display_maximum_luminance");
                if (targeted_system_display_maximum_luminance_Max<targeted_system_display_maximum_luminance)
                    targeted_system_display_maximum_luminance_Max=targeted_system_display_maximum_luminance;
                Get_SB (    base_enable_flag,                   "base_enable_flag");
                if (base_enable_flag)
                {
                    Skip_S2(14,                                 "base_param_m_p");
                    Skip_S1( 6,                                 "base_param_m_m");
                    Skip_S2(10,                                 "base_param_m_a");
                    Skip_S2(10,                                 "base_param_m_b");
                    Skip_S1( 6,                                 "base_param_m_n");
                    Skip_S1( 2,                                 "base_param_k1");
                    Skip_S1( 2,                                 "base_param_k2");
                    Skip_S1( 4,                                 "base_param_k2");
                    Skip_S1( 3,                                 "base_param_Delta_enable_mode");
                    Skip_S1( 7,                                 "base_param_Delta");
                    Get_SB (    ThreeSpline_enable_flag,        "3Spline_enable_flag");
                    if (ThreeSpline_enable_flag)
                    {
                        bool ThreeSpline_num_b;
                        Get_SB(    ThreeSpline_num_b,           "3Spline_num");
                        int ThreeSpline_num=(int)ThreeSpline_num_b; // Just for avoiding some compiler warning
                        for (int j=0; j<=ThreeSpline_num; j++)
                        {
                            Element_Begin1("3Spline");
                            int8u ThreeSpline_TH_mode;
                            Get_S1 (2, ThreeSpline_TH_mode,     "3Spline_TH_mode");
                            switch (ThreeSpline_TH_mode)
                            {
                                case 0:
                                case 2:
                                    Skip_S1(8,                  "3Spline_TH_enable_MB");
                                    break;
                                default:;
                            }
                            Skip_S2(12,                         "3Spline_TH");
                            Skip_S2(10,                         "3Spline_TH_Delta1");
                            Skip_S2(10,                         "3Spline_TH_Delta2");
                            Skip_S1( 8,                         "3Spline_enable_Strength");
                            Element_End0();
                        }
                    }
                }
                Element_End0();
            }
        }
    }
    Get_SB (color_saturation_mapping_flag,                      "color_saturation_mapping_flag");
    if (color_saturation_mapping_flag)
    {
        int8u color_saturation_enable_num;
        Get_S1 (3, color_saturation_enable_num,                 "color_saturation_enable_num");
        for (int i=0; i<color_saturation_enable_num; i++)
        {
            Skip_S1(8,                                          "color_saturation_enable_gain");
        }
    }
    BS_End();

    FILLING_BEGIN();
        auto& HDR_Format=HDR[Video_HDR_Format][HdrFormat_HdrVivid];
        if (HDR_Format.empty())
        {
            HDR_Format=__T("HDR Vivid");
            HDR[Video_HDR_Format_Version][HdrFormat_HdrVivid]=Ztring::ToZtring(system_start_code);
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
// SEI - 5
void File_Hevc::sei_message_user_data_unregistered(int32u payloadSize)
{
    Element_Info1("user_data_unregistered");

    //Parsing
    int128u uuid_iso_iec_11578;
    Get_UUID(uuid_iso_iec_11578,                                "uuid_iso_iec_11578");

    switch (uuid_iso_iec_11578.hi)
    {
        case 0x427FCC9BB8924821LL : Element_Info1("Ateme");
                                     sei_message_user_data_unregistered_Ateme(payloadSize-16); break;
        case 0x2CA2DE09B51747DBLL : Element_Info1("x265");
                                     sei_message_user_data_unregistered_x265(payloadSize-16); break;
        default :
                    Element_Info1("unknown");
                    Skip_XX(payloadSize-16,                     "data");
    }
}

//---------------------------------------------------------------------------
// SEI - 5 - Ateme
void File_Hevc::sei_message_user_data_unregistered_Ateme(int32u payloadSize)
{
    //Parsing
    Get_UTF8 (payloadSize, Encoded_Library,                     "Library name");

    //Encoded_Library
    if (Encoded_Library.find(__T("ATEME "))==0)
    {
        size_t Pos=Encoded_Library.find_first_of(__T("0123456789"));
        if (Pos && Encoded_Library[Pos-1]==__T(' '))
        {
            Encoded_Library_Name=Encoded_Library.substr(0, Pos-1);
            Encoded_Library_Version=Encoded_Library.substr(Pos);
        }
    }
}

//---------------------------------------------------------------------------
// SEI - 5 - x265
void File_Hevc::sei_message_user_data_unregistered_x265(int32u payloadSize)
{
    //Parsing
    string Data;
    Peek_String(payloadSize, Data);
    if (Data.size()!=payloadSize && Data.size()+1!=payloadSize)
    {
        Skip_XX(payloadSize,                                    "Unknown");
        return; //This is not a text string
    }
    size_t Data_Pos_Before=0;
    size_t Loop=0;
    do
    {
        size_t Data_Pos=Data.find(" - ", Data_Pos_Before);
        if (Data_Pos==std::string::npos)
            Data_Pos=Data.size();
        if (Data.find("options: ", Data_Pos_Before)==Data_Pos_Before)
        {
            Element_Begin1("options");
            size_t Options_Pos_Before=Data_Pos_Before;
            Encoded_Library_Settings.clear();
            while (Options_Pos_Before!=Data.size())
            {
                size_t Options_Pos=Data.find(__T(' '), Options_Pos_Before);
                if (Options_Pos==std::string::npos)
                    Options_Pos=Data.size();
                string option;
                Get_String (Options_Pos-Options_Pos_Before, option, "option");
                Options_Pos_Before=Options_Pos;
                while (Options_Pos_Before!=Data.size())
                {
                    string Separator;
                    Peek_String(1, Separator);
                    if (Separator==" ")
                    {
                        Skip_UTF8(1,                                "separator");
                        Options_Pos_Before+=1;
                    }
                    else
                        break;
                }

                //Filling
                if (option!="options:" && !(!option.empty() && option[0]>='0' && option[0]<='9') && option.find("fps=")!=0 && option.find("bitdepth=")!=0) //Ignoring redundant information e.g. width, height, frame rate, bit depth
                {
                    if (!Encoded_Library_Settings.empty())
                        Encoded_Library_Settings+=__T(" / ");
                    Encoded_Library_Settings+=Ztring().From_UTF8(option.c_str());
                }
            }
            Element_End0();
        }
        else
        {
            string Value;
            Get_String(Data_Pos-Data_Pos_Before, Value,          "data");

            //Saving
            if (Loop==0)
            {
                //Cleaning a little the value
                while (!Value.empty() && Value[0]<0x30)
                    Value.erase(Value.begin());
                while (!Value.empty() && Value[Value.size()-1]<0x30)
                    Value.erase(Value.end()-1);
                size_t Value_Pos=Value.find(__T(' '));
                if (Value_Pos!=string::npos)
                    Value.resize(Value_Pos);
                Encoded_Library.From_UTF8(Value.c_str());
            }
            if (Loop==1 && Encoded_Library.find(__T("x265"))==0)
            {
                size_t Value_Pos=Value.find(" 8bpp");
                if (Value_Pos!=string::npos)
                    Value.resize(Value_Pos);

                Encoded_Library+=__T(" - ");
                Encoded_Library+=Ztring().From_UTF8(Value.c_str());
            }
        }
        Data_Pos_Before=Data_Pos;
        if (Data_Pos_Before+3<=Data.size())
        {
            Skip_UTF8(3,                                        "separator");
            Data_Pos_Before+=3;
        }

        Loop++;
    }
    while (Data_Pos_Before!=Data.size());

    //Encoded_Library
    if (Encoded_Library.find(__T("x265 - "))==0)
    {
        Encoded_Library_Name=__T("x265");
        Encoded_Library_Version=Encoded_Library.SubString(__T("x265 - "), Ztring());
    }
    else
        Encoded_Library_Name=Encoded_Library;
}

//---------------------------------------------------------------------------
// SEI - 6
void File_Hevc::sei_message_recovery_point()
{
    Element_Info1("recovery_point");

    //Parsing
    BS_Begin();
    Skip_SE(                                                    "recovery_poc_cnt");
    Skip_SB(                                                    "exact_match_flag");
    Skip_SB(                                                    "broken_link_flag");
    BS_End();
}

//---------------------------------------------------------------------------
void File_Hevc::sei_message_active_parameter_sets()
{
    Element_Info1("active_parameter_sets");

    //Parsing
    int32u num_sps_ids_minus1;
    BS_Begin();
    Skip_S1(4,                                                  "active_video_parameter_set_id");
    Skip_SB(                                                    "self_contained_cvs_flag");
    Skip_SB(                                                    "no_parameter_set_update_flag");
    Get_UE (   num_sps_ids_minus1,                              "num_sps_ids_minus1");
    for (int32u i=0; i<=num_sps_ids_minus1; ++i)
    {
        Skip_UE(                                                "active_seq_parameter_set_id");
    }
    BS_End();
}

//---------------------------------------------------------------------------
void File_Hevc::sei_time_code()
{
    Element_Info1("time_code");

    //Parsing
    int8u num_clock_ts;
    BS_Begin();
    Get_S1 (2, num_clock_ts,                                    "num_clock_ts");
    for (int32u i=0; i<num_clock_ts; i++)
    {
        Element_Begin1("clock_ts");
        bool clock_timestamp_flag;
        Get_SB (clock_timestamp_flag,                           "clock_timestamp_flag");
        if (clock_timestamp_flag)
        {
            int16u n_frames;
            int8u counting_type, seconds_value, minutes_value, hours_value, time_offset_length;
            bool units_field_based_flag, full_timestamp_flag, seconds_flag, minutes_flag, hours_flag;
            Get_SB (units_field_based_flag,                     "units_field_based_flag");
            Get_S1 (5, counting_type,                           "counting_type");
            Get_SB (full_timestamp_flag,                        "full_timestamp_flag");
            Skip_SB(                                            "discontinuity_flag");
            Skip_SB(                                            "cnt_dropped_flag");
            Get_S2 (9, n_frames,                                "n_frames");
            seconds_flag=minutes_flag=hours_flag=full_timestamp_flag;
            if (!full_timestamp_flag)
                Get_SB (seconds_flag,                           "seconds_flag");
            if (seconds_flag)
                Get_S1 (6, seconds_value,                       "seconds_value");
            if (!full_timestamp_flag && seconds_flag)
                Get_SB (minutes_flag,                           "minutes_flag");
            if (minutes_flag)
                Get_S1 (6, minutes_value,                       "minutes_value");
            if (!full_timestamp_flag && minutes_flag)
                Get_SB (hours_flag,                             "hours_flag");
            if (hours_flag)
                Get_S1 (5, hours_value,                         "hours_value");
            Get_S1 (5, time_offset_length,                      "time_offset_length");
            if (time_offset_length)
                Skip_S1(time_offset_length,                     "time_offset_value");
            FILLING_BEGIN();
                if (!i && seconds_flag && minutes_flag && hours_flag && Frame_Count_NotParsedIncluded<16)
                {
                    int32u FrameMax;
                    if (counting_type>1 && counting_type!=4)
                    {
                        n_frames=0;
                        FrameMax=0; //Unsupported type
                    }
                    else if (!seq_parameter_sets.empty() && seq_parameter_sets[0] && seq_parameter_sets[0]->vui_parameters && seq_parameter_sets[0]->vui_parameters->time_scale && seq_parameter_sets[0]->vui_parameters->num_units_in_tick) //TODO: get the exact seq
                        FrameMax=(int32u)(float64_int64s((float64)seq_parameter_sets[0]->vui_parameters->time_scale/seq_parameter_sets[0]->vui_parameters->num_units_in_tick)-1);
                    else if (n_frames>99)
                        FrameMax=n_frames;
                    else
                        FrameMax=99;

                    TC_Current=TimeCode(hours_value, minutes_value, seconds_value, n_frames, FrameMax, TimeCode::DropFrame(counting_type==4));
                    Element_Info1(TC_Current.ToString());
                }
            FILLING_END();
        }
        Element_End0();
    }
    BS_End();
}

//---------------------------------------------------------------------------
void File_Hevc::sei_message_decoded_picture_hash(int32u payloadSize)
{
    Element_Info1("decoded_picture_hash");

    //Parsing
    int8u hash_type;
    Get_B1 (hash_type,                                          "hash_type");
    for (int8u cIdx=0; cIdx<(chroma_format_idc?3:1); cIdx++)
        switch (hash_type)
        {
            case 0 :    // md5
                        Skip_XX(16,                             "md5");
                        break;
            case 1 :    // crc
                        Skip_XX( 2,                             "crc");
                        break;
            case 2 :    // checksum
                        Skip_XX( 4,                             "checksum");
                        break;
            default :   //
                        Skip_XX((Element_Size-1)/(chroma_format_idc?1:3), "unknown");
                        break;
        }
}

//---------------------------------------------------------------------------
void File_Hevc::sei_message_mastering_display_colour_volume()
{
    Element_Info1("mastering_display_colour_volume");

    auto& HDR_Format=HDR[Video_HDR_Format][HdrFormat_SmpteSt2086];
    if (HDR_Format.empty())
    {
        HDR_Format=__T("SMPTE ST 2086");
        HDR[Video_HDR_Format_Compatibility][HdrFormat_SmpteSt2086]="HDR10";
    }
    Get_MasteringDisplayColorVolume(HDR[Video_MasteringDisplay_ColorPrimaries][HdrFormat_SmpteSt2086], HDR[Video_MasteringDisplay_Luminance][HdrFormat_SmpteSt2086]);
}

//---------------------------------------------------------------------------
void File_Hevc::sei_message_light_level()
{
    Element_Info1("light_level");

    //Parsing
    Get_LightLevel(maximum_content_light_level, maximum_frame_average_light_level);
}

//---------------------------------------------------------------------------
void File_Hevc::sei_alternative_transfer_characteristics()
{
    Element_Info1("alternative_transfer_characteristics");

    //Parsing
    Get_B1(preferred_transfer_characteristics,                  "preferred_transfer_characteristics"); Param_Info1(Mpegv_transfer_characteristics(preferred_transfer_characteristics));
}

//---------------------------------------------------------------------------
void File_Hevc::three_dimensional_reference_displays_info(int32u payloadSize)
{
    Element_Info1("three_dimensional_reference_displays_info");

    //Parsing
    BS_Begin();
    auto End=Data_BS_Remain()-payloadSize*8;
    int32u prec_ref_display_width, num_ref_displays_minus1;
    int32u left_view_id0, right_view_id0;
    bool ref_viewing_distance_flag;
    Get_UE (prec_ref_display_width,                             "prec_ref_display_width");
    if (prec_ref_display_width>=32)
    {
        Trusted_IsNot("prec_ref_display_width out of range");
        BS_End();
        return;
    }
    TEST_SB_GET (ref_viewing_distance_flag,                     "ref_viewing_distance_flag");
        Skip_UE(                                                "prec_ref_viewing_dist");
    TEST_SB_END();
    Get_UE (num_ref_displays_minus1,                            "num_ref_displays_minus1");
    if (num_ref_displays_minus1>=32)
    {
        Trusted_IsNot("num_ref_displays_minus1 out of range");
        BS_End();
        return;
    }
    for (int32u i=0; i<=num_ref_displays_minus1; i++)
    {
        Element_Begin1("ref_display");
        int32u left_view_id, right_view_id;
        int8u exponent_ref_display_width;
        Get_UE (left_view_id,                                   "left_view_id");
        Get_UE (right_view_id,                                  "right_view_id");
        if (!i)
        {
            left_view_id0=left_view_id;
            right_view_id0=right_view_id;
        }
        Get_S1 (6, exponent_ref_display_width,                  "exponent_ref_display_width");
        if (exponent_ref_display_width>=63)
        {
            if (exponent_ref_display_width>63)
                Trusted_IsNot("exponent_ref_display_width out of range");
            else
                Param_Info1("(Not supported)");
            BS_End();
            return;
        }
        int8u refDispWidthBits;
        if (exponent_ref_display_width)
        {
            auto temp=exponent_ref_display_width+prec_ref_display_width;
            if (temp>31)
                refDispWidthBits=(int8u)(temp-31);
            else
                refDispWidthBits=0;
        }
        else
            refDispWidthBits=(prec_ref_display_width==31);
        if (refDispWidthBits)
            Skip_BS(refDispWidthBits,                           "mantissa_ref_display_width");
        if (ref_viewing_distance_flag)
        {
            int8u exponent_ref_viewing_distance;
            Get_S1 (6, exponent_ref_viewing_distance,           "exponent_ref_viewing_distance");
            if (exponent_ref_viewing_distance>=63)
            {
                if (exponent_ref_viewing_distance>63)
                    Trusted_IsNot("exponent_ref_viewing_distance out of range");
                else
                    Param_Info1("(Not supported)");
                BS_End();
                return;
            }
            int8u refViewDistBits;
            if (exponent_ref_viewing_distance)
            {
                auto temp=exponent_ref_viewing_distance+prec_ref_display_width;
                if (temp>31)
                    refViewDistBits=(int8u)(temp-31);
                else
                    refViewDistBits=0;
            }
            else
                refViewDistBits=(exponent_ref_viewing_distance==31);
            if (refViewDistBits)
                Skip_BS(refViewDistBits,                        "mantissa_ref_viewing_distance");
        }
        TEST_SB_SKIP(                                           "additional_shift_present_flag");
            Skip_S2(10,                                         "num_sample_shift_plus512");
        TEST_SB_END();
        Element_End0();
    }
    TEST_SB_SKIP(                                               "three_dimensional_reference_displays_extension_flag");
        Skip_BS(Data_BS_Remain()-End,                           "(Not parsed)");
    TEST_SB_END();
    BS_End();
    FILLING_BEGIN_PRECISE();
        /*
        if (!video_parameter_sets.empty())
        {
            const auto& view_id_val=(*video_parameter_sets.begin())->view_id_val;
            if (left_view_id0==view_id_val[0] && right_view_id0==view_id_val[1])
                Fill(Stream_Video, 0, Video_MultiView_Order, "L R");
            if (left_view_id0==view_id_val[1] && right_view_id0==view_id_val[0])
                Fill(Stream_Video, 0, Video_MultiView_Order, "R L");
        }
        */
    FILLING_END();
}

//***************************************************************************
// Sub-elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Hevc::slice_segment_header()
{
    Element_Begin1("slice_segment_header");

    //Parsing
    bool    dependent_slice_segment_flag=false;
    Get_SB (   first_slice_segment_in_pic_flag,                 "first_slice_segment_in_pic_flag");
    if (RapPicFlag)
        Skip_SB(                                                "no_output_of_prior_pics_flag");
    Get_UE (   slice_pic_parameter_set_id,                      "slice_pic_parameter_set_id");
    std::vector<pic_parameter_set_struct*>::iterator pic_parameter_set_Item;
    if (slice_pic_parameter_set_id>=pic_parameter_sets.size() || (*(pic_parameter_set_Item=pic_parameter_sets.begin()+slice_pic_parameter_set_id))==NULL)
    {
        //Not yet present
        RiskCalculationN++;
        RiskCalculationD++;
        Skip_BS(Data_BS_Remain(),                               "Data (pic_parameter_set is missing)");
        Element_End0();
        slice_pic_parameter_set_id=(int32u)-1;
        slice_type=(int32u)-1;
        return;
    }
    if (!first_slice_segment_in_pic_flag)
    {
        if (!MustParse_VPS_SPS_PPS_FromFlv && (*pic_parameter_set_Item)->dependent_slice_segments_enabled_flag)
            Get_SB (dependent_slice_segment_flag,               "dependent_slice_segment_flag");
        //Skip_BS(Ceil( Log2( PicSizeInCtbsY ) ),               "slice_segment_address");
        Skip_BS(Data_BS_Remain(),                               "(ToDo)");
        Element_End0();
        slice_type=(int32u)-1;
        return;
    }
    if (!dependent_slice_segment_flag)
    {
        if (!MustParse_VPS_SPS_PPS_FromFlv)
            Skip_S1((*pic_parameter_set_Item)->num_extra_slice_header_bits, "slice_reserved_flags");
        Get_UE (slice_type,                                     "slice_type"); Param_Info1(Hevc_slice_type(slice_type));
    }

    //TODO...
    Skip_BS(Data_BS_Remain(),                                   "(ToDo)");

    Element_End0();

    #if MEDIAINFO_EVENTS
        if (first_slice_segment_in_pic_flag)
        {
            switch(Element_Code)
            {
                case 19 :
                case 20 :   // This is an IDR frame
                case 21 :
                case 22 :
                case 23 :
                            TemporalReferences_Offset=TemporalReferences_Max+1;
                            pic_order_cnt_DTS_Ref=FrameInfo.DTS;
                            if (Config->Config_PerPackage && Element_Code==0x05) // First slice of an IDR frame
                            {
                                // IDR
                                Config->Config_PerPackage->FrameForAlignment(this, true);
                                Config->Config_PerPackage->IsClosedGOP(this);
                            }
            }
        }
    #endif //MEDIAINFO_EVENTS

    //Saving some info
    std::vector<seq_parameter_set_struct*>::iterator seq_parameter_set_Item;
    if ((*pic_parameter_set_Item)->seq_parameter_set_id>=seq_parameter_sets.size() || (*(seq_parameter_set_Item=seq_parameter_sets.begin()+(*pic_parameter_set_Item)->seq_parameter_set_id))==NULL || (*seq_parameter_set_Item)->vui_parameters==NULL)
        return;
    float FrameRate=0;
    if ((*seq_parameter_set_Item)->vui_parameters->time_scale && (*seq_parameter_set_Item)->vui_parameters->num_units_in_tick)
        FrameRate=(float64)(*seq_parameter_set_Item)->vui_parameters->time_scale/(*seq_parameter_set_Item)->vui_parameters->num_units_in_tick;
    else
        FrameRate=0;
    if (first_slice_segment_in_pic_flag && pic_order_cnt_DTS_Ref!=(int64u)-1 && FrameInfo.PTS!=(int64u)-1 && FrameRate && TemporalReferences_Reserved)
    {
        int64s pic_order_cnt=float64_int64s(int64s(FrameInfo.PTS-pic_order_cnt_DTS_Ref)*FrameRate/1000000000);
        if (pic_order_cnt>=TemporalReferences.size()/4 || pic_order_cnt<=-((int64s)TemporalReferences.size()/4))
            pic_order_cnt_DTS_Ref=(int64u)-1; // Incoherency in DTS? Disabling compute by DTS, TODO: more generic test (all formats)
    }
    if (first_slice_segment_in_pic_flag && pic_order_cnt_DTS_Ref!=(int64u)-1 && FrameInfo.PTS!=(int64u)-1 && FrameRate && TemporalReferences_Reserved)
    {
        //Frame order detection
        int64s pic_order_cnt=float64_int64s(int64s(FrameInfo.PTS-pic_order_cnt_DTS_Ref)*FrameRate/1000000000);
        if (pic_order_cnt<TemporalReferences_pic_order_cnt_Min)
        {
            if (pic_order_cnt<0)
            {
                size_t Base=(size_t)(TemporalReferences_Offset+TemporalReferences_pic_order_cnt_Min);
                size_t ToInsert=(size_t)(TemporalReferences_pic_order_cnt_Min-pic_order_cnt);
                if (Base+ToInsert>=4*TemporalReferences_Reserved || Base>=4*TemporalReferences_Reserved || ToInsert+TemporalReferences_Max>=4*TemporalReferences_Reserved || TemporalReferences_Max>=4*TemporalReferences_Reserved || TemporalReferences_Max-Base>=4*TemporalReferences_Reserved)
                {
                    Trusted_IsNot("Problem in temporal references");
                    return;
                }
                Element_Info1(__T("Offset of ")+Ztring::ToZtring(ToInsert));
                TemporalReferences.insert(TemporalReferences.begin()+Base, ToInsert, NULL);
                TemporalReferences_Offset+=ToInsert;
                TemporalReferences_Offset_pic_order_cnt_lsb_Last += ToInsert;
                TemporalReferences_Max+=ToInsert;
                TemporalReferences_pic_order_cnt_Min=pic_order_cnt;
            }
            else if (TemporalReferences_Min>(size_t)(TemporalReferences_Offset+pic_order_cnt))
                TemporalReferences_Min=(size_t)(TemporalReferences_Offset+pic_order_cnt);
        }

        if (pic_order_cnt<0 && TemporalReferences_Offset<(size_t)(-pic_order_cnt)) //Found in playreadyEncryptedBlowUp.ts without encryption test
        {
            Trusted_IsNot("Problem in temporal references");
            return;
        }

        if ((size_t)(TemporalReferences_Offset+pic_order_cnt)>=3*TemporalReferences_Reserved)
        {
            size_t Offset=TemporalReferences_Max-TemporalReferences_Offset;
            if (Offset%2)
                Offset++;
            if (Offset>=TemporalReferences_Reserved && pic_order_cnt>=(int64s)TemporalReferences_Reserved)
            {
                TemporalReferences_Offset+=TemporalReferences_Reserved;
                pic_order_cnt-=TemporalReferences_Reserved;
                TemporalReferences_pic_order_cnt_Min-=TemporalReferences_Reserved/2;
            }
            while (TemporalReferences_Offset+pic_order_cnt>=3*TemporalReferences_Reserved)
            {
                for (size_t Pos=0; Pos<TemporalReferences_Reserved; Pos++)
                {
                    if (TemporalReferences[Pos])
                    {
                        /*
                        if ((Pos % 2) == 0)
                            PictureTypes_PreviousFrames+=Avc_slice_type[TemporalReferences[Pos]->slice_type];
                        */
                        delete TemporalReferences[Pos];
                        TemporalReferences[Pos]=NULL;
                    }
                    /*
                    else if (!PictureTypes_PreviousFrames.empty()) //Only if stream already started
                    {
                        if ((Pos%2)==0)
                            PictureTypes_PreviousFrames+=' ';
                    }
                    */
                }
                /*
                if (PictureTypes_PreviousFrames.size()>=8*TemporalReferences.size())
                    PictureTypes_PreviousFrames.erase(PictureTypes_PreviousFrames.begin(), PictureTypes_PreviousFrames.begin()+PictureTypes_PreviousFrames.size()-TemporalReferences.size());
                */
                TemporalReferences.erase(TemporalReferences.begin(), TemporalReferences.begin()+TemporalReferences_Reserved);
                TemporalReferences.resize(4*TemporalReferences_Reserved);
                if (TemporalReferences_Reserved<TemporalReferences_Offset)
                    TemporalReferences_Offset-=TemporalReferences_Reserved;
                else
                    TemporalReferences_Offset=0;
                if (TemporalReferences_Reserved<TemporalReferences_Min)
                    TemporalReferences_Min-=TemporalReferences_Reserved;
                else
                    TemporalReferences_Min=0;
                if (TemporalReferences_Reserved<TemporalReferences_Max)
                    TemporalReferences_Max-=TemporalReferences_Reserved;
                else
                    TemporalReferences_Max=0;
                if (TemporalReferences_Reserved<TemporalReferences_Offset_pic_order_cnt_lsb_Last)
                    TemporalReferences_Offset_pic_order_cnt_lsb_Last-=TemporalReferences_Reserved;
                else
                    TemporalReferences_Offset_pic_order_cnt_lsb_Last=0;
            }
        }

        //TemporalReferences_Offset_pic_order_cnt_lsb_Diff=(int32s)((int32s)(TemporalReferences_Offset+pic_order_cnt)-TemporalReferences_Offset_pic_order_cnt_lsb_Last);
        TemporalReferences_Offset_pic_order_cnt_lsb_Last=(size_t)(TemporalReferences_Offset+pic_order_cnt);
        /*
        if (field_pic_flag && (*seq_parameter_set_Item)->pic_order_cnt_type == 2 && TemporalReferences_Offset_pic_order_cnt_lsb_Last<TemporalReferences.size() && TemporalReferences[TemporalReferences_Offset_pic_order_cnt_lsb_Last])
            TemporalReferences_Offset_pic_order_cnt_lsb_Last++;
        */
        if (TemporalReferences_Max<=TemporalReferences_Offset_pic_order_cnt_lsb_Last)
            TemporalReferences_Max=TemporalReferences_Offset_pic_order_cnt_lsb_Last;//+((*seq_parameter_set_Item)->frame_mbs_only_flag?2:1);
        if (TemporalReferences_Min>TemporalReferences_Offset_pic_order_cnt_lsb_Last)
            TemporalReferences_Min=TemporalReferences_Offset_pic_order_cnt_lsb_Last;
        if (TemporalReferences_DelayedElement)
        {
            delete TemporalReferences[TemporalReferences_Offset_pic_order_cnt_lsb_Last]; TemporalReferences[TemporalReferences_Offset_pic_order_cnt_lsb_Last]=TemporalReferences_DelayedElement;
        }
        if (TemporalReferences[TemporalReferences_Offset_pic_order_cnt_lsb_Last]==NULL)
            TemporalReferences[TemporalReferences_Offset_pic_order_cnt_lsb_Last]=new temporal_reference();
        /*
        TemporalReferences[TemporalReferences_Offset_pic_order_cnt_lsb_Last]->frame_num=frame_num;
        TemporalReferences[TemporalReferences_Offset_pic_order_cnt_lsb_Last]->slice_type=(int8u)slice_type;
        TemporalReferences[TemporalReferences_Offset_pic_order_cnt_lsb_Last]->IsTop=!bottom_field_flag;
        TemporalReferences[TemporalReferences_Offset_pic_order_cnt_lsb_Last]->IsField=field_pic_flag;
        */
        if (TemporalReferences_DelayedElement)
        {
            TemporalReferences_DelayedElement=NULL;
            sei_message_user_data_registered_itu_t_t35_B5_0031_GA94_03_Delayed((*pic_parameter_set_Item)->seq_parameter_set_id);
        }
    }
}

//---------------------------------------------------------------------------
void File_Hevc::profile_tier_level(profile_tier_level_struct& p, bool profilePresentFlag, int8u maxNumSubLayersMinus1)
{
    Element_Begin1("profile_tier_level");

    //Parsing
    std::vector<bool>sub_layer_profile_present_flags, sub_layer_level_present_flags;
    if (profilePresentFlag)
    {
    Get_S1 (2,  p.profile_space,                                "general_profile_space");
    Get_SB (    p.tier_flag,                                    "general_tier_flag");
    Get_S1 (5,  p.profile_idc,                                  "general_profile_idc"); Param_Info1(Hevc_profile_idc(p.profile_idc));
    Element_Begin1("general_profile_compatibility_flags");
        for (int8u profile_pos=0; profile_pos<32; profile_pos++)
            if (profile_pos==p.profile_idc)
            {
                bool general_profile_compatibility_flag;
                Get_SB (    general_profile_compatibility_flag, "general_profile_compatibility_flag");
                //if (!general_profile_compatibility_flag && !profile_space)  //found some files without this flag, ignoring the test for the moment (not really important)
                //    Trusted_IsNot("general_profile_compatibility_flag not valid");
            }
            else
                Skip_SB(                                        "general_profile_compatibility_flag");
    Element_End0();
    Element_Begin1("general_profile_compatibility_flags");
        Get_SB (    p.general_progressive_source_flag,          "general_progressive_source_flag");
        Get_SB (    p.general_interlaced_source_flag,           "general_interlaced_source_flag");
        Skip_SB(                                                "general_non_packed_constraint_flag");
        Get_SB (    p.general_frame_only_constraint_flag,       "general_frame_only_constraint_flag");
        Get_SB (    p.general_max_12bit_constraint_flag,        "general_max_12bit_constraint_flag");
        Get_SB (    p.general_max_10bit_constraint_flag,        "general_max_10bit_constraint_flag");
        Get_SB (    p.general_max_8bit_constraint_flag,         "general_max_8bit_constraint_flag");
        Skip_SB(                                                "general_max_422chroma_constraint_flag");
        Skip_SB(                                                "general_max_420chroma_constraint_flag");
        Skip_SB(                                                "general_max_monochrome_constraint_flag");
        Skip_SB(                                                "general_intra_constraint_flag");
        Skip_SB(                                                "general_one_picture_only_constraint_flag");
        Skip_SB(                                                "general_lower_bit_rate_constraint_flag");
        Get_SB (    p.general_max_14bit_constraint_flag,        "general_max_14bit_constraint_flag");
        for (int8u constraint_pos=0; constraint_pos<33; constraint_pos++)
            Skip_SB(                                            "general_reserved");
        Skip_SB(                                                "general_inbld_flag");
    Element_End0();
    }
    Get_S1 (8,  p.level_idc,                                    "general_level_idc");
    for (int32u SubLayerPos=0; SubLayerPos<maxNumSubLayersMinus1; SubLayerPos++)
    {
        Element_Begin1("SubLayer");
        bool sub_layer_profile_present_flag, sub_layer_level_present_flag;
        Get_SB (   sub_layer_profile_present_flag,              "sub_layer_profile_present_flag");
        Get_SB (   sub_layer_level_present_flag,                "sub_layer_level_present_flag");
        sub_layer_profile_present_flags.push_back(sub_layer_profile_present_flag);
        sub_layer_level_present_flags.push_back(sub_layer_level_present_flag);
        Element_End0();
    }
    if (maxNumSubLayersMinus1)
        for(int32u SubLayerPos=maxNumSubLayersMinus1; SubLayerPos<8; SubLayerPos++)
            Skip_S1(2,                                          "reserved_zero_2bits");
    for (int32u SubLayerPos=0; SubLayerPos<maxNumSubLayersMinus1; SubLayerPos++)
    {
        Element_Begin1("SubLayer");
        if (sub_layer_profile_present_flags[SubLayerPos])
        {
            Skip_S1(2,                                          "sub_layer_profile_space");
            Skip_SB(                                            "sub_layer_tier_flag");
            Info_S1(5, sub_layer_profile_idc,                   "sub_layer_profile_idc"); Param_Info1(Hevc_profile_idc(sub_layer_profile_idc));
            Skip_S4(32,                                         "sub_layer_profile_compatibility_flags");
            Skip_SB(                                            "sub_layer_progressive_source_flag");
            Skip_SB(                                            "sub_layer_interlaced_source_flag");
            Skip_SB(                                            "sub_layer_non_packed_constraint_flag");
            Skip_SB(                                            "sub_layer_frame_only_constraint_flag");
            Skip_S8(44,                                         "sub_layer_reserved_zero_44bits");
        }
        if (sub_layer_level_present_flags[SubLayerPos])
        {
            Skip_S1(8,                                          "sub_layer_level_idc");
        }
        Element_End0();
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Hevc::short_term_ref_pic_sets(int8u num_short_term_ref_pic_sets)
{
    Element_Begin1("short_term_ref_pic_sets");
    int32u num_pics=0;

    for (int32u stRpsIdx=0; stRpsIdx<num_short_term_ref_pic_sets; stRpsIdx++)
    {
        Element_Begin1("short_term_ref_pic_set");
        bool inter_ref_pic_set_prediction_flag=false;
        if (stRpsIdx)
            Get_SB (inter_ref_pic_set_prediction_flag,              "inter_ref_pic_set_prediction_flag");
        if (inter_ref_pic_set_prediction_flag)
        {
            int32u  delta_idx_minus1=0, abs_delta_rps_minus1;
            bool    delta_rps_sign;
            if (stRpsIdx==num_short_term_ref_pic_sets)
                Get_UE (delta_idx_minus1,                           "delta_idx_minus1");
            if (delta_idx_minus1+1>stRpsIdx)
            {
                Skip_BS(Data_BS_Remain(),                           "(Problem)");
                Element_End0();
                Element_End0();
                return;
            }
            Get_SB (   delta_rps_sign,                              "delta_rps_sign");
            Get_UE (   abs_delta_rps_minus1,                        "abs_delta_rps_minus1");
            int32u num_pics_new=0;
            for(int32u pic_pos=0 ; pic_pos<=num_pics; pic_pos++)
            {
                TESTELSE_SB_SKIP(                                   "used_by_curr_pic_flag");
                    num_pics_new++;
                TESTELSE_SB_ELSE(                                   "used_by_curr_pic_flag");
                    bool use_delta_flag;
                    Get_SB (use_delta_flag,                         "use_delta_flag");
                    if (use_delta_flag)
                        num_pics_new++;
                TESTELSE_SB_END();
            }
            num_pics=num_pics_new;
        }
        else
        {
            int32u num_negative_pics, num_positive_pics;
            Get_UE (num_negative_pics,                              "num_negative_pics");
            Get_UE (num_positive_pics,                              "num_positive_pics");
            num_pics=num_negative_pics+num_positive_pics;
            for (int32u i=0; i<num_negative_pics; i++)
            {
                Skip_UE(                                            "delta_poc_s0_minus1");
                Skip_SB(                                            "used_by_curr_pic_s0_flag");
            }
            for (int32u i=0; i<num_positive_pics; i++)
            {
                Skip_UE(                                            "delta_poc_s1_minus1");
                Skip_SB(                                            "used_by_curr_pic_s1_flag");
            }
        }
        Element_End0();
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Hevc::vui_parameters(seq_parameter_set_struct::vui_parameters_struct* &vui_parameters_Item_, int8u maxNumSubLayersMinus1)
{
    //Parsing
    seq_parameter_set_struct::vui_parameters_struct::xxl_common *xxL_Common=NULL;
    seq_parameter_set_struct::vui_parameters_struct::xxl *NAL = NULL, *VCL = NULL;
    vui_flags flags;
    int32u  num_units_in_tick=0, time_scale=0;
    int16u  sar_width=0, sar_height=0;
    int8u   video_format=5, colour_primaries=2, transfer_characteristics=2, matrix_coefficients=2;
    bool flag;
    TEST_SB_SKIP(                                               "aspect_ratio_info_present_flag");
        int8u aspect_ratio_idc=0;
        Get_S1 (8, aspect_ratio_idc,                            "aspect_ratio_idc");
        if (aspect_ratio_idc==0xFF)
        {
            Get_S2 (16, sar_width,                              "sar_width");
            Get_S2 (16, sar_height,                             "sar_height");
        }
        else if (aspect_ratio_idc && aspect_ratio_idc <= Avc_PixelAspectRatio_Size)
        {
            aspect_ratio_idc--;
            const auto& aspect_ratio = Avc_PixelAspectRatio[aspect_ratio_idc];
            Param_Info1(aspect_ratio.w);
            Param_Info1(aspect_ratio.h);
            sar_width = aspect_ratio.w;
            sar_height = aspect_ratio.h;
        }
    TEST_SB_END();
    TEST_SB_SKIP(                                               "overscan_info_present_flag");
        Skip_SB(                                                "overscan_appropriate_flag");
    TEST_SB_END();
    TEST_SB_SKIP(                                               "video_signal_type_present_flag");
        flags[video_signal_type_present_flag]=true;
        bool video_full_range_flag_;
        Get_S1 (3, video_format,                                "video_format"); Param_Info1(Avc_video_format[video_format]);
        Get_SB (   video_full_range_flag_,                      "video_full_range_flag"); Param_Info1(Avc_video_full_range[video_full_range_flag_]);
        flags[video_full_range_flag]=video_full_range_flag_;
        TEST_SB_SKIP(                                           "colour_description_present_flag");
            flags[colour_description_present_flag]=true;
            Get_S1 (8, colour_primaries,                        "colour_primaries"); Param_Info1(Mpegv_colour_primaries(colour_primaries));
            Get_S1 (8, transfer_characteristics,                "transfer_characteristics"); Param_Info1(Mpegv_transfer_characteristics(transfer_characteristics));
            Get_S1 (8, matrix_coefficients,                     "matrix_coefficients"); Param_Info1(Mpegv_matrix_coefficients(matrix_coefficients));
        TEST_SB_END();
    TEST_SB_END();
    TEST_SB_SKIP(                                               "chroma_loc_info_present_flag");
        Get_UE (chroma_sample_loc_type_top_field,               "chroma_sample_loc_type_top_field");
        Get_UE (chroma_sample_loc_type_bottom_field,            "chroma_sample_loc_type_bottom_field");
    TEST_SB_END();
    Skip_SB(                                                    "neutral_chroma_indication_flag");
    Skip_SB(                                                    "field_seq_flag");
    Get_SB (   flag,                                            "frame_field_info_present_flag");
    flags[frame_field_info_present_flag]=flag;
    TEST_SB_SKIP(                                               "default_display_window_flag ");
        Skip_UE(                                                "def_disp_win_left_offset");
        Skip_UE(                                                "def_disp_win_right_offset");
        Skip_UE(                                                "def_disp_win_top_offset");
        Skip_UE(                                                "def_disp_win_bottom_offset");
    TEST_SB_END();
    TEST_SB_SKIP(                                               "timing_info_present_flag");
        Get_S4 (32, num_units_in_tick,                          "num_units_in_tick");
        Get_S4 (32, time_scale,                                 "time_scale");
        TEST_SB_SKIP(                                           "vui_poc_proportional_to_timing_flag");
            Skip_UE(                                            "vui_num_ticks_poc_diff_one_minus1");
        TEST_SB_END();
        TEST_SB_SKIP(                                           "hrd_parameters_present_flag");
            hrd_parameters(true, maxNumSubLayersMinus1, xxL_Common, NAL, VCL);
        TEST_SB_END();
    TEST_SB_END();
    TEST_SB_SKIP(                                               "bitstream_restriction_flag");
        Skip_SB(                                                "tiles_fixed_structure_flag");
        Skip_SB(                                                "motion_vectors_over_pic_boundaries_flag");
        Skip_SB(                                                "restricted_ref_pic_lists_flag");
        Skip_UE(                                                "min_spatial_segmentation_idc");
        Skip_UE(                                                "max_bytes_per_pic_denom");
        Skip_UE(                                                "max_bits_per_min_cu_denom");
        Skip_UE(                                                "log2_max_mv_length_horizontal");
        Skip_UE(                                                "log2_max_mv_length_vertical");
    TEST_SB_END();

    FILLING_BEGIN();
        vui_parameters_Item_ = new seq_parameter_set_struct::vui_parameters_struct(
                                                                                    NAL,
                                                                                    VCL,
                                                                                    xxL_Common,
                                                                                    num_units_in_tick,
                                                                                    time_scale,
                                                                                    sar_width,
                                                                                    sar_height,
                                                                                    video_format,
                                                                                    colour_primaries,
                                                                                    transfer_characteristics,
                                                                                    matrix_coefficients,
                                                                                    flags
                                                                                  );
    FILLING_ELSE();
    delete xxL_Common; xxL_Common=NULL;
    delete NAL; NAL=NULL;
    delete VCL; VCL=NULL;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Hevc::hrd_parameters(bool commonInfPresentFlag, int8u maxNumSubLayersMinus1, seq_parameter_set_struct::vui_parameters_struct::xxl_common* &xxL_Common, seq_parameter_set_struct::vui_parameters_struct::xxl* &NAL, seq_parameter_set_struct::vui_parameters_struct::xxl* &VCL)
{
    //Parsing
    int8u bit_rate_scale=0, cpb_size_scale=0, du_cpb_removal_delay_increment_length_minus1=0, dpb_output_delay_du_length_minus1=0, initial_cpb_removal_delay_length_minus1=0, au_cpb_removal_delay_length_minus1=0, dpb_output_delay_length_minus1=0;
    bool nal_hrd_parameters_present_flag=false, vcl_hrd_parameters_present_flag=false, sub_pic_hrd_params_present_flag=false;
    if (commonInfPresentFlag)
    {
        Get_SB (nal_hrd_parameters_present_flag,                "nal_hrd_parameters_present_flag");
        Get_SB (vcl_hrd_parameters_present_flag,                "vcl_hrd_parameters_present_flag");
        if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag)
        {
            TEST_SB_GET (sub_pic_hrd_params_present_flag,       "sub_pic_hrd_params_present_flag");
                Skip_S1(8,                                      "tick_divisor_minus2");
                Get_S1 (5, du_cpb_removal_delay_increment_length_minus1,  "du_cpb_removal_delay_increment_length_minus1");
                Skip_SB(                                        "sub_pic_cpb_params_in_pic_timing_sei_flag");
                Get_S1 (5, dpb_output_delay_du_length_minus1,   "dpb_output_delay_du_length_minus1");
            TEST_SB_END();
            Get_S1 (4, bit_rate_scale,                          "bit_rate_scale");
            Get_S1 (4, cpb_size_scale,                          "cpb_size_scale");
            if (sub_pic_hrd_params_present_flag)
                Skip_S1(4,                                      "cpb_size_du_scale");
            Get_S1 (5, initial_cpb_removal_delay_length_minus1, "initial_cpb_removal_delay_length_minus1");
            Get_S1 (5, au_cpb_removal_delay_length_minus1,      "au_cpb_removal_delay_length_minus1");
            Get_S1 (5, dpb_output_delay_length_minus1,          "dpb_output_delay_length_minus1");
        }
    }

    for (int8u NumSubLayer=0; NumSubLayer<=maxNumSubLayersMinus1; NumSubLayer++)
    {
        int32u cpb_cnt_minus1=0;
        bool fixed_pic_rate_general_flag, fixed_pic_rate_within_cvs_flag=true, low_delay_hrd_flag=false;
        Get_SB (fixed_pic_rate_general_flag,                   "fixed_pic_rate_general_flag");
        if (!fixed_pic_rate_general_flag)
            Get_SB (fixed_pic_rate_within_cvs_flag,            "fixed_pic_rate_within_cvs_flag");
        if (fixed_pic_rate_within_cvs_flag)
            Skip_UE(                                           "elemental_duration_in_tc_minus1");
        else
            Get_SB (low_delay_hrd_flag,                        "low_delay_hrd_flag");
        if (!low_delay_hrd_flag)
        {
            Get_UE (cpb_cnt_minus1,                            "cpb_cnt_minus1");
            if (cpb_cnt_minus1>31)
            {
                Trusted_IsNot("cpb_cnt_minus1 too high");
                cpb_cnt_minus1=0;
                return;
            }
        }
        if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag)
            xxL_Common=new seq_parameter_set_struct::vui_parameters_struct::xxl_common(
                                                                                        sub_pic_hrd_params_present_flag,
                                                                                        du_cpb_removal_delay_increment_length_minus1,
                                                                                        dpb_output_delay_du_length_minus1,
                                                                                        initial_cpb_removal_delay_length_minus1,
                                                                                        au_cpb_removal_delay_length_minus1,
                                                                                        dpb_output_delay_length_minus1
                                                                                      );
        if (nal_hrd_parameters_present_flag)
            sub_layer_hrd_parameters(xxL_Common, bit_rate_scale, cpb_size_scale, cpb_cnt_minus1, NAL); //TODO: save HRD per NumSubLayer
        if (vcl_hrd_parameters_present_flag)
            sub_layer_hrd_parameters(xxL_Common, bit_rate_scale, cpb_size_scale, cpb_cnt_minus1, VCL); //TODO: save HRD per NumSubLayer
    }
}

//---------------------------------------------------------------------------
void File_Hevc::sub_layer_hrd_parameters(seq_parameter_set_struct::vui_parameters_struct::xxl_common* xxL_Common, int8u bit_rate_scale, int8u cpb_size_scale, int32u cpb_cnt_minus1, seq_parameter_set_struct::vui_parameters_struct::xxl* &hrd_parameters_Item_)
{
    //Parsing
    vector<seq_parameter_set_struct::vui_parameters_struct::xxl::xxl_data>  SchedSel;
    SchedSel.reserve(cpb_cnt_minus1 + 1);
    for (int8u SchedSelIdx = 0; SchedSelIdx <= cpb_cnt_minus1; ++SchedSelIdx)
    {
        Element_Begin1("ShedSel");
        int64u bit_rate_value, cpb_size_value;
        int32u bit_rate_value_minus1, cpb_size_value_minus1;
        bool cbr_flag;
        Get_UE (bit_rate_value_minus1,                          "bit_rate_value_minus1");
        bit_rate_value = (int64u)((bit_rate_value_minus1 + 1)*pow(2.0, 6 + bit_rate_scale)); Param_Info2(bit_rate_value, " bps");
        Get_UE (cpb_size_value_minus1,                          "cpb_size_value_minus1");
        cpb_size_value = (int64u)((cpb_size_value_minus1 + 1)*pow(2.0, 4 + cpb_size_scale)); Param_Info2(cpb_size_value, " bits");
        if (xxL_Common->sub_pic_hrd_params_present_flag)
        {
            Skip_UE(                                            "cpb_size_du_value_minus1");
            Skip_UE(                                            "bit_rate_du_value_minus1");
        }
        Get_SB (cbr_flag,                                       "cbr_flag");
        Element_End0();

        FILLING_BEGIN();
        SchedSel.push_back(seq_parameter_set_struct::vui_parameters_struct::xxl::xxl_data(
                                                                                            bit_rate_value,
                                                                                            cpb_size_value,
                                                                                            cbr_flag
                                                                                         ));
        FILLING_END();
    }

    //Validity test
    if (!Element_IsOK() || (SchedSel.size() == 1 && SchedSel[0].bit_rate_value == 64))
    {
        return; //We do not trust this kind of data
    }

    //Filling
    hrd_parameters_Item_=new seq_parameter_set_struct::vui_parameters_struct::xxl(
                                                                                    SchedSel
                                                                                 );
}

//---------------------------------------------------------------------------
void File_Hevc::scaling_list_data()
{
    for(int8u sizeId=0; sizeId<4; sizeId++)
        for (int8u matrixId = 0; matrixId<((sizeId == 3) ? 2 : 6); matrixId++)
        {
            bool scaling_list_pred_mode_flag;
            Get_SB (scaling_list_pred_mode_flag,                "scaling_list_pred_mode_flag");
            if(!scaling_list_pred_mode_flag)
                Skip_UE(                                        "scaling_list_pred_matrix_id_delta");
            else
            {
                //nextCoef = 8
                size_t coefNum=std::min(64, (1<<(4+(sizeId<<1))));
                if( sizeId > 1 )
                {
                    Skip_SE(                                    "scaling_list_dc_coef_minus8"); //[ sizeId ? 2 ][ matrixId ] se(p)
                    //nextCoef = scaling_list_dc_coef_minus8[ sizeId ? 2 ][ matrixId ] + 8
                }
                for(size_t i=0; i<coefNum; i++)
                {
                    Skip_SE(                                    "scaling_list_delta_coef");
                    //nextCoef = ( nextCoef + scaling_list_delta_coef + 256 ) % 256
                    //ScalingList[ sizeId ][ matrixId ][ i ] = nextCoef
                }
            }
        }
}

//***************************************************************************
// Specific
//***************************************************************************

//---------------------------------------------------------------------------
void File_Hevc::VPS_SPS_PPS()
{
    if (MustParse_VPS_SPS_PPS_FromMatroska || MustParse_VPS_SPS_PPS_FromFlv)
    {
        if (Element_Size>=5
         && Buffer[Buffer_Offset  ]==0x01
         && Buffer[Buffer_Offset+1]==0x00
         && Buffer[Buffer_Offset+2]==0x00
         && Buffer[Buffer_Offset+3]==0x00
         && Buffer[Buffer_Offset+4]==0xFF) //Trying to detect old proposal of the form of Matroska implementation
            return VPS_SPS_PPS_FromMatroska();

        MustParse_VPS_SPS_PPS_FromMatroska=false;
        MustParse_VPS_SPS_PPS_FromFlv=false;
    }

    //Parsing
    int64u general_constraint_indicator_flags;
    int32u general_profile_compatibility_flags;
    int8u  configurationVersion;
    int8u  chromaFormat, bitDepthLumaMinus8, bitDepthChromaMinus8;
    int8u  general_profile_space, general_profile_idc, general_level_idc;
    int8u  numOfArrays, constantFrameRate, numTemporalLayers;
    bool   general_tier_flag, temporalIdNested;
    Get_B1 (configurationVersion,                               "configurationVersion");
    if (!MustParse_VPS_SPS_PPS_FromLhvc)
    {
    BS_Begin();
        Get_S1 (2, general_profile_space,                       "general_profile_space");
        Get_SB (   general_tier_flag,                           "general_tier_flag");
        Get_S1 (5, general_profile_idc,                         "general_profile_idc"); Param_Info1(Hevc_profile_idc(general_profile_idc));
    BS_End();
    Get_B4 (general_profile_compatibility_flags,                "general_profile_compatibility_flags");
    Get_B6 (general_constraint_indicator_flags,                 "general_constraint_indicator_flags");
    Get_B1 (general_level_idc,                                  "general_level_idc");
    }
    BS_Begin();
        Mark_1_NoTrustError();
        Mark_1_NoTrustError();
        Mark_1_NoTrustError();
        Mark_1_NoTrustError();
        Skip_S2(12,                                             "min_spatial_segmentation_idc");
    BS_End();
    BS_Begin();
        Mark_1_NoTrustError();
        Mark_1_NoTrustError();
        Mark_1_NoTrustError();
        Mark_1_NoTrustError();
        Mark_1_NoTrustError();
        Mark_1_NoTrustError();
        Skip_S1(2,                                              "parallelismType");
    BS_End();
    if (!MustParse_VPS_SPS_PPS_FromLhvc)
    {
    BS_Begin();
        Mark_1_NoTrustError();
        Mark_1_NoTrustError();
        Mark_1_NoTrustError();
        Mark_1_NoTrustError();
        Mark_1_NoTrustError();
        Mark_1_NoTrustError();
        Get_S1 (2, chromaFormat,                                "chromaFormat");
    BS_End();
    BS_Begin();
        Mark_1_NoTrustError();
        Mark_1_NoTrustError();
        Mark_1_NoTrustError();
        Mark_1_NoTrustError();
        Mark_1_NoTrustError();
        Get_S1 (3, bitDepthLumaMinus8,                          "bitDepthLumaMinus8");
    BS_End();
    BS_Begin();
        Mark_1_NoTrustError();
        Mark_1_NoTrustError();
        Mark_1_NoTrustError();
        Mark_1_NoTrustError();
        Mark_1_NoTrustError();
        Get_S1 (3, bitDepthChromaMinus8,                        "bitDepthChromaMinus8");
    BS_End();
    Skip_B2(                                                    "avgFrameRate");
    }
    BS_Begin();
        Get_S1 (2, constantFrameRate,                           "constantFrameRate");
        Get_S1 (3, numTemporalLayers,                           "numTemporalLayers");
        Get_SB (   temporalIdNested,                            "temporalIdNested");
        Get_S1 (2, lengthSizeMinusOne,                          "lengthSizeMinusOne");
    BS_End();
    Get_B1 (numOfArrays,                                        "numOfArrays");
    for (size_t ArrayPos=0; ArrayPos<numOfArrays; ArrayPos++)
    {
        Element_Begin1("Array");
        int16u numNalus;
        int8u NAL_unit_type;
        BS_Begin();
            Skip_SB(                                            "array_completeness");
            Mark_0_NoTrustError();
            Get_S1 (6, NAL_unit_type,                           "NAL_unit_type");
        BS_End();
        Get_B2 (numNalus,                                       "numNalus");
        for (size_t NaluPos=0; NaluPos<numNalus; NaluPos++)
        {
            Element_Begin1("nalUnit");
            int16u nalUnitLength;
            Get_B2 (nalUnitLength,                              "nalUnitLength");
            if (nalUnitLength<2 || Element_Offset+nalUnitLength>Element_Size)
            {
                Trusted_IsNot("Size is wrong");
                break; //There is an error
            }

            //Header
            int8u nal_unit_type, nuh_temporal_id_plus1;
            BS_Begin();
            Mark_0 ();
            Get_S1 (6, nal_unit_type,                           "nal_unit_type");
            Get_S1 (6, nuh_layer_id,                            "nuh_layer_id");
            Get_S1 (3, nuh_temporal_id_plus1,                   "nuh_temporal_id_plus1");
            if (nuh_temporal_id_plus1==0)
                Trusted_IsNot("nuh_temporal_id_plus1 is invalid");
            BS_End();

            //Data
            int64u Element_Offset_Save=Element_Offset;
            int64u Element_Size_Save=Element_Size;
            Buffer_Offset+=(size_t)Element_Offset_Save;
            Element_Offset=0;
            Element_Size=nalUnitLength-2;
            Element_Code=nal_unit_type;
            Data_Parse();
            Buffer_Offset-=(size_t)Element_Offset_Save;
            Element_Offset=Element_Offset_Save+nalUnitLength-2;
            Element_Size=Element_Size_Save;

            Element_End0();
        }
        Element_End0();
    }

    MustParse_VPS_SPS_PPS=false;
    FILLING_BEGIN_PRECISE();
        Accept("HEVC");
    FILLING_ELSE();
        Frame_Count_NotParsedIncluded--;
        RanOutOfData("HEVC");
        Frame_Count_NotParsedIncluded++;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Hevc::VPS_SPS_PPS_FromMatroska()
{
    //Parsing
    int8u Profile, Level, vid_parameter_set_count, seq_parameter_set_count, pic_parameter_set_count;
    if (SizedBlocks)
        Skip_B1(                                                "Version");
    Get_B1 (Profile,                                            "Profile");
    Skip_B1(                                                    "Compatible profile");
    Get_B1 (Level,                                              "Level");
    BS_Begin();
    Skip_S1(6,                                                  "Reserved");
    Get_S1 (2, lengthSizeMinusOne,                              "Size of NALU length minus 1");
    Skip_S1(3,                                                  "Reserved");
    Get_S1 (5, vid_parameter_set_count,                         MustParse_VPS_SPS_PPS_FromFlv?"vid_parameter_set+seq_parameter_set count":"vid_parameter_set count");
    BS_End();
    for (int8u Pos=0; Pos<vid_parameter_set_count; Pos++)
    {
        Element_Begin1("nalUnit");
        int16u nalUnitLength;
        Get_B2 (nalUnitLength,                              "nalUnitLength");
        if (nalUnitLength<2 || Element_Offset+nalUnitLength>Element_Size)
        {
            Trusted_IsNot("Size is wrong");
            break; //There is an error
        }

        //Header
        int8u nal_unit_type, nuh_temporal_id_plus1;
        BS_Begin();
        Mark_0 ();
        Get_S1 (6, nal_unit_type,                           "nal_unit_type");
        Get_S1 (6, nuh_layer_id,                            "nuh_layer_id");
        Get_S1 (3, nuh_temporal_id_plus1,                   "nuh_temporal_id_plus1");
        if (nuh_temporal_id_plus1==0)
            Trusted_IsNot("nuh_temporal_id_plus1 is invalid");
        BS_End();

        //Data
        int64u Element_Offset_Save=Element_Offset;
        int64u Element_Size_Save=Element_Size;
        Buffer_Offset+=(size_t)Element_Offset_Save;
        Element_Offset=0;
        Element_Size=nalUnitLength-2;
        Element_Code=nal_unit_type;
        Data_Parse();
        Buffer_Offset-=(size_t)Element_Offset_Save;
        Element_Offset=Element_Offset_Save+nalUnitLength-2;
        Element_Size=Element_Size_Save;

        Element_End0();
    }
    if (MustParse_VPS_SPS_PPS_FromFlv)
        seq_parameter_set_count=0;
    else
    {
        BS_Begin();
        Skip_S1(3,                                              "Reserved");
        Get_S1 (5, seq_parameter_set_count,                     "seq_parameter_set count");
        BS_End();
    }
    for (int8u Pos=0; Pos<seq_parameter_set_count; Pos++)
    {
        Element_Begin1("nalUnit");
        int16u nalUnitLength;
        Get_B2 (nalUnitLength,                              "nalUnitLength");
        if (nalUnitLength<2 || Element_Offset+nalUnitLength>Element_Size)
        {
            Trusted_IsNot("Size is wrong");
            break; //There is an error
        }

        //Header
        int8u nal_unit_type, nuh_temporal_id_plus1;
        BS_Begin();
        Mark_0 ();
        Get_S1 (6, nal_unit_type,                           "nal_unit_type");
        Get_S1 (6, nuh_layer_id,                            "nuh_layer_id");
        Get_S1 (3, nuh_temporal_id_plus1,                   "nuh_temporal_id_plus1");
        if (nuh_temporal_id_plus1==0)
            Trusted_IsNot("nuh_temporal_id_plus1 is invalid");
        BS_End();

        //Data
        int64u Element_Offset_Save=Element_Offset;
        int64u Element_Size_Save=Element_Size;
        Buffer_Offset+=(size_t)Element_Offset_Save;
        Element_Offset=0;
        Element_Size=nalUnitLength-2;
        Element_Code=nal_unit_type;
        Data_Parse();
        Buffer_Offset-=(size_t)Element_Offset_Save;
        Element_Offset=Element_Offset_Save+nalUnitLength-2;
        Element_Size=Element_Size_Save;

        Element_End0();
    }
    Get_B1 (pic_parameter_set_count,                            "pic_parameter_set count");
    for (int8u Pos=0; Pos<pic_parameter_set_count; Pos++)
    {
        Element_Begin1("nalUnit");
        int16u nalUnitLength;
        Get_B2 (nalUnitLength,                              "nalUnitLength");
        if (nalUnitLength<2 || Element_Offset+nalUnitLength>Element_Size)
        {
            Trusted_IsNot("Size is wrong");
            break; //There is an error
        }

        //Header
        int8u nal_unit_type, nuh_temporal_id_plus1;
        BS_Begin();
        Mark_0 ();
        Get_S1 (6, nal_unit_type,                           "nal_unit_type");
        Get_S1 (6, nuh_layer_id,                            "nuh_layer_id");
        Get_S1 (3, nuh_temporal_id_plus1,                   "nuh_temporal_id_plus1");
        if (nuh_temporal_id_plus1==0)
            Trusted_IsNot("nuh_temporal_id_plus1 is invalid");
        BS_End();

        //Data
        int64u Element_Offset_Save=Element_Offset;
        int64u Element_Size_Save=Element_Size;
        Buffer_Offset+=(size_t)Element_Offset_Save;
        Element_Offset=0;
        Element_Size=nalUnitLength-2;
        Element_Code=nal_unit_type;
        Data_Parse();
        Buffer_Offset-=(size_t)Element_Offset_Save;
        Element_Offset=Element_Offset_Save+nalUnitLength-2;
        Element_Size=Element_Size_Save;

        Element_End0();
    }
    if (Element_Offset<Element_Size)
        Skip_XX(Element_Size-Element_Offset,                    "Padding?");

    //Filling
    MustParse_VPS_SPS_PPS=false;
    FILLING_BEGIN_PRECISE();
        Accept("HEVC");
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Hevc::EndOfxPS(const char* FlagName, const char* DataName)
{
    TESTELSE_SB_SKIP(                                           FlagName);
        Skip_BS(Data_BS_Remain(),                               DataName);
        RiskCalculationN++; //xxx_extension_flag is set, we can not check the end of the content, so a bit risky
        RiskCalculationD++;
    TESTELSE_SB_ELSE(                                           FlagName);
        rbsp_trailing_bits();
    TESTELSE_SB_END();
}

//---------------------------------------------------------------------------
void File_Hevc::rbsp_trailing_bits()
{
    size_t IsRisky;
    size_t Remain=Data_BS_Remain();
    if (!Remain)
        IsRisky=1; //rbsp_stop_one_bit is missing 
    else if (Remain>8)
        IsRisky=1+Remain/80; //there is lot of unexpected content, not normal, so highly risky (risk weight based on remaining content size)
    else
    {
        int8u RealValue;
        const int8u ExpectedValue=1<<(Remain-1);
        Peek_S1(Remain, RealValue);
        if (RealValue==ExpectedValue)
        {
            // Conform to specs
            IsRisky=0;
            Mark_1();
            while (Data_BS_Remain())
                Mark_0();
        }
        else
            IsRisky=1; //there is some unexpected content, not normal, so highly risky
    }
    if (IsRisky)
    {
        Skip_BS(Remain,                                     "Unknown");
        RiskCalculationN+=IsRisky;
        RiskCalculationD+=IsRisky;
    }
    else
        RiskCalculationD++;
}

} //NameSpace

#endif //MEDIAINFO_HEVC_YES

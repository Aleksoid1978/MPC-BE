/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

 //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 //
 // Contributor: Paul Higgs, paul_higgs@hotmail.com
 // 
 // Decode AVSV (T/AI 109/2) according to DVB Bluebook A001
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
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_AVS3V_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Video/File_Avs3V.h"
//---------------------------------------------------------------------------

constexpr auto HAVE_HDRVivid_DMI = 1;
constexpr auto HAVE_DolbyVision_DMI = 2;
constexpr auto HAVE_SLHDR2_DMI = 4;
constexpr auto HAVE_HDR10plus_DMI = 8;

#define Patch_Start_Code(sc)            ((sc)>=0x00 && (sc)<=0x7F)
#define Patch_End_Code                  0x8F
#define Video_Sequence_Start_Code       0xB0
#define Video_Sequence_End_Code         0xB1
#define User_Data_Start_Code            0xB2
#define Intra_Picture_Start_Code        0xB3
#define Avs3_Reserved_B4                0xB4
#define Extension_Start_Code            0xB5
#define Inter_Picture_Start_Code        0xB6
#define Video_Edit_Code                 0xB7
#define Avs3_Reserved_B8                0xB8

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
static Ztring Avs3V_profile(int8u profile_id)
{
    switch (profile_id)
    {
        case 0x20 : return "Main 8-bit";
        case 0x22 : return "Main 10-bit";
        case 0x30 : return "High 8-bit";
        case 0x32 : return "High 10-bit";
    }
    return Ztring::ToZtring(profile_id);
}

//---------------------------------------------------------------------------
static Ztring Avs3V_level(int8u level_id)
{
    switch (level_id)
    {
        case 0x10 : return "2.0.15";
        case 0x12 : return "2.0.30";
        case 0x14 : return "2.0.60";
        case 0x20 : return "4.0.30";
        case 0x22 : return "4.0.60";
        case 0x40 : return "6.0.30";
        case 0x42 : return "6.2.30";
        case 0x41 : return "6.4.30";
        case 0x43 : return "6.6.30";
        case 0x44 : return "6.0.60";
        case 0x46 : return "6.2.60";
        case 0x45 : return "6.4.60";
        case 0x47 : return "6.6.60";
        case 0x48 : return "6.0.120";
        case 0x4A : return "6.2.120";
        case 0x49 : return "6.4.120";
        case 0x4B : return "6.6.120";
        case 0x50 : return "8.0.30";
        case 0x52 : return "8.2.30";
        case 0x51 : return "8.4.30";
        case 0x53 : return "8.6.30";
        case 0x54 : return "8.0.60";
        case 0x56 : return "8.2.60";
        case 0x55 : return "8.4.60";
        case 0x57 : return "8.6.60";
        case 0x58 : return "8.0.120";
        case 0x5A : return "8.2.120";
        case 0x59 : return "8.4.120";
        case 0x5B : return "8.6.120";
        case 0x60 : return "10.0.30";
        case 0x62 : return "10.2.30";
        case 0x61 : return "10.4.30";
        case 0x63 : return "10.6.30";
        case 0x64 : return "10.0.60";
        case 0x66 : return "10.2.60";
        case 0x65 : return "10.4.60";
        case 0x67 : return "10.6.60";
        case 0x68 : return "10.0.120";
        case 0x6A : return "10.2.120";
        case 0x69 : return "10.4.120";
        case 0x6B : return "10.6.120";
    }
    return Ztring::ToZtring(level_id);
}

//---------------------------------------------------------------------------
static const float32 Avs3V_frame_rate[]=
{
    (float32)0,
    ((float32)24000)/1001,
    (float32)24,
    (float32)25,
    ((float32)30000)/1001,
    (float32)30,
    (float32)50,
    ((float32)60000)/1001,
    (float32)60,
    (float32)100,
    (float32)120,
    (float32)200,
    (float32)240,
    (float32)300,
    ((float32)120000)/1001,
    (float32)0,
};

//---------------------------------------------------------------------------
static const char* Avs3V_chroma_format[]=
{
    "",
    "4:2:0",
    "4:2:2",
    "",
};

//---------------------------------------------------------------------------
static const char* Avs3V_extension_start_code_identifier[]=
{
  /* 0000 */  "",
  /* 0001 */  "",
  /* 0010 */  "sequence_display",
  /* 0011 */  "temporal_scalability_extension",
  /* 0100 */  "copyright_extension",
  /* 0101 */  "high_dynamic_range_picture_metadata_extension",
  /* 0110 */  "",
  /* 0111 */  "picture_display_extension",
  /* 1000 */  "",
  /* 1001 */  "",
  /* 1010 */  "mastering_display_and_content_metadata_extension",
  /* 1011 */  "camera_parameters_extension",
  /* 1100 */  "ROI_parameters_extension",
  /* 1101 */  "reference_to_library_picture_extension", 
  /* 1110 */  "adaptive_intra_refresh_parameters",
  /* 1111 */  "",
};

//---------------------------------------------------------------------------
static const char* Avs3V_video_format[]=
{
    /* 000 */ "Component",
    /* 001 */ "PAL",
    /* 010 */ "NTSC",
    /* 011 */ "SECAM",
    /* 100 */ "MAC",
    /* 101 */ "", // Unspecified
    /* 110 */ "",
    /* 111 */ "",
};

//---------------------------------------------------------------------------
static const float32 Avs3V_aspect_ratio[]=
{
    (float32)0,
    (float32)1,
    (float32)4/(float32)3,
    (float32)16/(float32)9,
    (float32)2.21,
    (float32)0,
    (float32)0,
    (float32)0,
    (float32)0,
    (float32)0,
    (float32)0,
    (float32)0,
    (float32)0,
    (float32)0,
    (float32)0,
    (float32)0,
};

//---------------------------------------------------------------------------
static const char* Avs3V_picture_coding_type[]=
{
    "",
    "P",
    "B",
    "",
};

//---------------------------------------------------------------------------
const char* Mpegv_colour_primaries(int8u colour_primaries);
const char* Mpegv_transfer_characteristics(int8u transfer_characteristics);
const char* Mpegv_matrix_coefficients(int8u matrix_coefficients);
const char* Avs3V_matrix_coefficients(int8u matrix_coefficients)
{
    switch (matrix_coefficients)
    {
        case  8:
        case  9: return Mpegv_matrix_coefficients(matrix_coefficients + 1);
        case 10: return ""; // No idea about what they plan to do there
    }
    return Mpegv_matrix_coefficients(matrix_coefficients);
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Avs3V::File_Avs3V()
:File__Analyze(),
num_ref_pic_list_set { 0, 0 },
picture_alf_enable_flag { false, false, false }
{
    //Config
    MustSynchronize=true;
    StreamSource=IsStream;
    Buffer_TotalBytes_FirstSynched_Max=64*1024;

    //In
    Frame_Count_Valid=30;
    FrameIsAlwaysComplete=false;

    //Temp
    video_sequence_start_IsParsed=false;

    #if MEDIAINFO_TRACE
        Trace_Activated = true;
    #endif //MEDIAINFO_TRACE
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Avs3V::Streams_Fill()
{
    //Filling
    Stream_Prepare(Stream_Video);
    Fill(Stream_Video, 0, Video_Format, "AVS3 Video");

    //From sequence header
    Fill(Stream_Video, 0, Video_Format_Profile, Avs3V_profile(profile_id)+__T('@')+Avs3V_level(level_id));
    Fill(Stream_Video, 0, Video_Codec_Profile, Avs3V_profile(profile_id)+__T('@')+Avs3V_level(level_id));
    Fill(Stream_Video, StreamPos_Last, Video_Width, horizontal_size);
    Fill(Stream_Video, StreamPos_Last, Video_Height, vertical_size);
    Fill(Stream_Video, 0, Video_FrameRate, Avs3V_frame_rate[frame_rate_code]/(progressive_sequence?1:2));
    if (aspect_ratio==0)
        ;//Forbidden
    else if (aspect_ratio==1)
            Fill(Stream_Video, 0, Video_PixelAspectRatio, 1.000, 3, true);
    else if (display_horizontal_size && display_vertical_size)
    {
        if (vertical_size && Avs3V_aspect_ratio[aspect_ratio])
            Fill(Stream_Video, StreamPos_Last, Video_DisplayAspectRatio, (float)horizontal_size/vertical_size
                                                                         *Avs3V_aspect_ratio[aspect_ratio]/((float)display_horizontal_size/display_vertical_size), 3, true);
    }
    else if (Avs3V_aspect_ratio[aspect_ratio])
        Fill(Stream_Video, StreamPos_Last, Video_DisplayAspectRatio, Avs3V_aspect_ratio[aspect_ratio], 3, true);
    Fill(Stream_Video, 0, Video_ChromaSubsampling, Avs3V_chroma_format[chroma_format]);
    if (progressive_frame_Count && progressive_frame_Count!=Frame_Count)
    {
        //This is mixed
    }
    else if (Frame_Count>0) //Only if we have at least one progressive_frame definition
    {
        if (progressive_sequence || progressive_frame_Count==Frame_Count)
        {
            Fill(Stream_Video, 0, Video_ScanType, "Progressive");
            Fill(Stream_Video, 0, Video_Interlacement, "PPF");
        }
        else
        {
            Fill(Stream_Video, 0, Video_ScanType, "Interlaced");
            if ((Interlaced_Top && Interlaced_Bottom) || (!Interlaced_Top && !Interlaced_Bottom))
                Fill(Stream_Video, 0, Video_Interlacement, "Interlaced");
            else
            {
                Fill(Stream_Video, 0, Video_ScanOrder, Interlaced_Top?"TFF":"BFF");
                Fill(Stream_Video, 0, Video_Interlacement, Interlaced_Top?"TFF":"BFF");
            }
        }
    }
    if (bit_rate)
        Fill(Stream_Video, 0, Video_BitRate_Nominal, bit_rate*8);

    switch (sample_precision) {
    case 0x01:
        Fill(Stream_Video, 0, Video_BitDepth, 8);
        break;
    case 0x02:
        Fill(Stream_Video, 0, Video_BitDepth, 10);
        break;
    }

    Fill(Stream_Video, 0, Video_colour_primaries, Mpegv_colour_primaries(colour_primaries));
    Fill(Stream_Video, 0, Video_transfer_characteristics, Mpegv_transfer_characteristics(transfer_characteristics));
    Fill(Stream_Video, 0, Video_matrix_coefficients, Avs3V_matrix_coefficients(matrix_coefficients));

    // from HDR Dynamic Metadata extension
    if (DMI_Found)
    {
        if (DMI_Found & HAVE_HDR10plus_DMI)
            Fill(Stream_Video, 0, Video_HDR_Format, "SMPTE ST 2094 App 4");
        if (DMI_Found & HAVE_SLHDR2_DMI)
            Fill(Stream_Video, 0, Video_HDR_Format, "SL-HDR2");
        if (DMI_Found & HAVE_DolbyVision_DMI)
            Fill(Stream_Video, 0, Video_HDR_Format, "SMPTE ST 2094-10");
        if (DMI_Found & HAVE_HDRVivid_DMI)
            Fill(Stream_Video, 0, Video_HDR_Format, "HDR Vivid");
    }

    // from Mastering Display and Content Metadata extension
    if (have_MaxCLL)
        Fill(Stream_Video, 0, Video_MaxCLL, max_content_light_level);
    if (have_MaxFALL)
        Fill(Stream_Video, 0, Video_MaxFALL, max_picture_average_light_level);

    //From extensions
    Fill(Stream_Video, 0, Video_Standard, Avs3V_video_format[video_format]);

    //Library name
    if (!Library.empty())
    {
        Fill(Stream_Video, 0, Video_Encoded_Library, Library);
        Fill(Stream_Video, 0, Video_Encoded_Library_Name, Library_Name);
        Fill(Stream_Video, 0, Video_Encoded_Library_Version, Library_Version);
        Fill(Stream_Video, 0, Video_Encoded_Library_Date, Library_Date);
    }
}

//---------------------------------------------------------------------------
void File_Avs3V::Streams_Finish()
{
    //Purge what is not needed anymore
    if (!File_Name.empty()) //Only if this is not a buffer, with buffer we can have more data
        Streams.clear();
}

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Avs3V::Synched_Test()
{
    //Must have enough buffer for having header
    if (Buffer_Offset+3>Buffer_Size)
        return false;

    //Quick test of synchro
    if (CC3(Buffer+Buffer_Offset)!=0x000001)
        Synched=false;

    //Quick search
    if (Synched && !Header_Parser_QuickSearch())
        return false;

    //We continue
    return true;
}

//---------------------------------------------------------------------------
void File_Avs3V::Synched_Init()
{
    //Count of a Packets
    progressive_frame_Count=0;
    Interlaced_Top=0;
    Interlaced_Bottom=0;

    //Temp
    bit_rate=0;
    horizontal_size=0;
    vertical_size=0;
    display_horizontal_size=0;
    display_vertical_size=0;
    profile_id=0;
    level_id=0;
    chroma_format=0;
    aspect_ratio=0;
    frame_rate_code=0;
    video_format=5; //Unspecified video format
    progressive_sequence=false;
    low_delay=false;
    temporal_id_enable_flag=false;

    DMI_Found=0;
    have_MaxCLL=false;
    have_MaxFALL=false;

    picture_structure=true;

    colour_primaries = 2;
    transfer_characteristics = 2;
    matrix_coefficients = 2;
    
    picture_alf_enable_flag[0]=false;
    picture_alf_enable_flag[1]=false;
    picture_alf_enable_flag[2]=false;

    //Default stream values
    Streams.resize(0x100);
    Streams[Video_Sequence_Start_Code].Searching_Payload=true; //video_sequence_start
    for (int8u Pos=0xFF; Pos>=0xB9; Pos--)
        Streams[Pos].Searching_Payload=true; //Testing MPEG-PS
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Avs3V::Header_Parse()
{
    //Parsing
    int8u start_code;
    Skip_B3(                                                    "synchro");
    Get_B1 (start_code,                                         "start_code");
    if (!Header_Parser_Fill_Size())
    {
        Element_WaitForMoreData();
        return;
    }

    //Filling
    Header_Fill_Code(start_code, Ztring().From_CC1(start_code));
}

//---------------------------------------------------------------------------
bool File_Avs3V::Header_Parser_QuickSearch()
{
    while (       Buffer_Offset+4<=Buffer_Size
      &&   Buffer[Buffer_Offset  ]==0x00
      &&   Buffer[Buffer_Offset+1]==0x00
      &&   Buffer[Buffer_Offset+2]==0x01)
    {
        //Getting start_code
        int8u start_code=Buffer[Buffer_Offset+3];

        //Searching start or timestamp
        if (Streams[start_code].Searching_Payload)
            return true;

        //Synchronizing
        Buffer_Offset+=4;
        Synched=false;
        if (!Synchronize_0x000001())
        {
            UnSynched_IsNotJunk=true;
            return false;
        }
    }

    if (Buffer_Offset+3==Buffer_Size)
        return false; //Sync is OK, but start_code is not available
    Trusted_IsNot("AVS3 Video, Synchronisation lost");
    return Synchronize();
}

//---------------------------------------------------------------------------
bool File_Avs3V::Header_Parser_Fill_Size()
{
    //Look for next Sync word
    if (Buffer_Offset_Temp==0) //Buffer_Offset_Temp is not 0 if Header_Parse_Fill_Size() has already parsed first frames
        Buffer_Offset_Temp=Buffer_Offset+4;
    while (Buffer_Offset_Temp+4<=Buffer_Size
        && CC3(Buffer+Buffer_Offset_Temp)!=0x000001)
    {
        Buffer_Offset_Temp+=2;
        while(Buffer_Offset_Temp<Buffer_Size && Buffer[Buffer_Offset_Temp]!=0x00)
            Buffer_Offset_Temp+=2;
        if (Buffer_Offset_Temp>=Buffer_Size || Buffer[Buffer_Offset_Temp-1]==0x00)
            Buffer_Offset_Temp--;
    }

    //Must wait more data?
    if (Buffer_Offset_Temp+4>Buffer_Size)
    {
        if (FrameIsAlwaysComplete || File_Offset+Buffer_Size==File_Size)
            Buffer_Offset_Temp=Buffer_Size; //We are sure that the next bytes are a start
        else
            return false;
    }

    //OK, we continue
    Header_Fill_Size(Buffer_Offset_Temp-Buffer_Offset);
    Buffer_Offset_Temp=0;
    return true;
}

//---------------------------------------------------------------------------
void File_Avs3V::Data_Parse()
{
    //Parsing
    switch (Element_Code)
    {
        case Video_Sequence_Start_Code: 
            video_sequence_start(); 
            break;
        case Video_Sequence_End_Code: 
            video_sequence_end(); 
            break;
        case User_Data_Start_Code: 
            user_data_start(); 
            break;
        case Extension_Start_Code: 
            extension_start(); 
            break;
        case Intra_Picture_Start_Code:
        case Inter_Picture_Start_Code: 
            picture_start(); 
            break;
        case Video_Edit_Code: 
            video_edit(); 
            break;
        case Avs3_Reserved_B4:
        case Avs3_Reserved_B8: 
            reserved(); 
            break;
        default:
            if (Element_Code<=0xAF)
                slice();
            else
            {
                if (Frame_Count==0 && Buffer_TotalBytes>Buffer_TotalBytes_FirstSynched_Max)
                    Trusted=0;
                Trusted_IsNot("Unattended element");
            }
    }

    LastElementIsSlice=Patch_Start_Code(Element_Code);

    if (!Status[IsAccepted] && File_Offset+Buffer_Offset+Element_Size==File_Size && Frame_Count) //Finalize frames in case of there are less than Frame_Count_Valid frames
    {
        //No need of more
        Accept("AVS Video");
        Finish("AVS Video");
    }
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
// Packet "00" to "AF"
void File_Avs3V::slice()
{
    Element_Name("Slice");
    if (!LastElementIsSlice)
        Element_Info1(Ztring::ToZtring(Frame_Count));

    //Parsing
    Skip_XX(Element_Size,                                       "Unknown");

    FILLING_BEGIN();
        //NextCode
        NextCode_Test();

        if (!LastElementIsSlice)
        {
            //Counting
            Frame_Count++;
            if (!progressive_frame)
            {
                if (picture_structure)
                {
                    if (top_field_first)
                        Interlaced_Top++;
                    else
                        Interlaced_Bottom++;
                }
            }
            else
                progressive_frame_Count++;

            //Filling only if not already done
            if (File_Offset+Buffer_Offset+Element_Size>=File_Size)
                Frame_Count_Valid=Frame_Count; //Finish frames in case of there are less than Frame_Count_Valid frames
            if (!Status[IsAccepted] && Frame_Count>=Frame_Count_Valid)
            {
                //No need of more
                Accept("AVS Video");
                Finish("AVS Video");
            }
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
// reference_picture_list_set
void File_Avs3V::reference_picture_list_set(int8u list, int32u rpls)
{
    int32u i, j;
    bool reference_to_library_enable_flag=false;
    int32u num_of_ref_pic;

    if (library_picture_enable_flag)
        Get_SB(reference_to_library_enable_flag, "reference_to_library_enable_flag");

    Get_UE(num_of_ref_pic, "num_of_ref_pic");
    for (i = 0; i < num_of_ref_pic; i++) {
        bool library_index_flag = false;
        char indexStr[16]; sprintf(indexStr, "[%i][%i][%i]", list, rpls, i);
        if (reference_to_library_enable_flag) {
            char elName[64]; sprintf(elName, "library_index_flag%s", indexStr);
            Get_SB(library_index_flag, elName);
        }

        if (library_index_flag) {
            char elName[64]; sprintf(elName, "referenced_library_picture_index%s", indexStr);
            Skip_UE(elName);
        }
        else {
            int32u abs_delta_doi;
            char elName[64]; sprintf(elName, "abs_delta_doi%s", indexStr);
            Get_UE(abs_delta_doi, elName);
            if (abs_delta_doi > 0) {
                sprintf(elName, "sign_delta_doi%s", indexStr);
                Skip_SB(elName);
            }
        }
     }
}

//---------------------------------------------------------------------------
// weight_quant_matrix
void File_Avs3V::weight_quant_matrix()
{
    int32u i, j;
    int8u  sizeId, WQMSize;

    for (sizeId = 0; sizeId < 2; sizeId++) {
        WQMSize = 1 << (sizeId + 2);
        for (i = 0; i<WQMSize; i++)
            for (j=0; j<WQMSize; j++) {
                char elName[64]; sprintf(elName, "weight_quant_coeff - WeightQuantMatrix%s[%i][%i]", (sizeId == 0 ? "4x4" : "8x8"), i, j);
                Skip_UE(elName);
            }    
    }
}

//---------------------------------------------------------------------------
// Packet "B0"
void File_Avs3V::video_sequence_start()
{
    Element_Name("video_sequence_start");

    //Parsing
    int32u bit_rate_upper, bit_rate_lower;
    int32u j;
    Get_B1 (    profile_id,                                     "profile_id");
    Get_B1 (    level_id,                                       "level_id");
    BS_Begin();
    Get_SB (    progressive_sequence,                           "progressive_sequence");
    Get_SB (    field_coded_sequence,                           "field_coded_sequence");
    Get_SB (    library_stream_flag,                            "library_stream_flag");
    library_picture_enable_flag = 0;
    duplicate_sequence_header_flag = 0;
    if (!library_stream_flag) {
        Get_SB(    library_picture_enable_flag,                 "library_picture_enable_flag");
        if (library_picture_enable_flag)
            Get_SB(    duplicate_sequence_header_flag,          "duplicate_sequence_header_flag");
    }
    Mark_1 ();
    Get_S2 (14, horizontal_size,                                "horizontal_size");
    Mark_1 ();
    Get_S2 (14, vertical_size,                                  "vertical_size");
    Get_S1 ( 2, chroma_format,                                  "chroma_format");
    Get_S1 ( 3, sample_precision,                               "sample_precision");
    if (profile_id == 0x22 || profile_id == 0x32)
            Get_S1( 3, encoding_precision,                      "encoding_precision");
    else
            encoding_precision = 1;  // The precision of the luma and chroma samples is 8-bit;
    Mark_1 ();
    Get_S1 ( 4, aspect_ratio,                                   "aspect_ratio"); Param_Info1(Avs3V_aspect_ratio[aspect_ratio]);
    Get_S1 ( 4, frame_rate_code,                                "frame_rate_code"); Param_Info1(Avs3V_frame_rate[frame_rate_code]);
    Mark_1 ();
    Get_S3 (18, bit_rate_lower,                                 "bit_rate_lower");
    Mark_1 ();
    Get_S3 (12, bit_rate_upper,                                 "bit_rate_upper");
    bit_rate=(bit_rate_upper<<18)+bit_rate_lower; Param_Info2(bit_rate*8, " bps");

    Get_SB (    low_delay,                                      "low_delay");
    Get_SB (    temporal_id_enable_flag,                        "temporal_id_enable_flag");
    Mark_1 ();
    Skip_S3 (18,                                                "bbv_buffer_size");
    Mark_1 ();
    Skip_S1 ( 4,                                                "max_dpb_minus1"); 
    Skip_SB (                                                   "rpl1_index_exist_flag");
    Get_SB (     rpl1_same_as_rpl0_flag,                        "rpl1_same_as_rpl0_flag");
    Mark_1 ();

    Get_UE (     num_ref_pic_list_set[0],                       "num_ref_pic_list_set[0]");
   
     for (j=0; j<num_ref_pic_list_set[0]; j++)
        reference_picture_list_set(0, j);

     if (!rpl1_same_as_rpl0_flag) {
        Get_UE(    num_ref_pic_list_set[1],                     "num_ref_pic_list_set[1]");
        for (j=0; j<num_ref_pic_list_set[1]; j++)
            reference_picture_list_set(1, j);
    }
    
    Skip_UE(                                                    "num_ref_default_active_minus1[0]"); 
    Skip_UE(                                                    "num_ref_default_active_minus1[1]"); 
    Skip_S1( 3,                                                 "log2_lcu_size_minus2");
    Skip_S1( 2,                                                 "log2_min_cu_size_minus2");
    Skip_S1( 2,                                                 "log2_max_part_ratio_minus2");
    Skip_S1( 3,                                                 "split_max_times_minus6");
    Skip_S1( 3,                                                 "log2_min_qt_minus2");
    Skip_S1( 3,                                                 "log2_max_bt_minus2");
    Skip_S1( 2,                                                 "log2_max_eqt_size_minus3");
    Mark_1();
    TEST_SB_SKIP(                                               "weight_quant_enable_flag");
        TEST_SB_SKIP(                                           "load_seq_weight_quant_data_flag");
            weight_quant_matrix();
        TEST_SB_END();
    TEST_SB_END();

    Skip_SB(                                                    "st_enable_flag");
    Skip_SB(                                                    "sao_enable_flag");
    Get_SB(alf_enable_flag,                                     "alf_enable_flag");
    Get_SB(affine_enable_flag,                                  "affine_enable_flag");
    Skip_SB(                                                    "smvd_enable_flag");
    Skip_SB(                                                    "ipcm_enable_flag");
    Get_SB(amvr_enable_flag,                                    "amvr_enable_flag");
    Get_S1( 4, num_of_hmvp_cand,                                "num_of_hmvp_cand");
    Skip_SB(                                                    "umve_enable_flag");
    if ((num_of_hmvp_cand!=0) && amvr_enable_flag)
        Skip_SB(                                                "emvr_enable_flag");
    Skip_SB(                                                    "intra_pf_enable_flag");
    Skip_SB(                                                    "tscpm_enable_flag");
    Mark_1();
    TEST_SB_SKIP(                                               "dt_enable_flag")
        Skip_S1(2,                                              "log2_max_dt_size_minus4");
    TEST_SB_END();
    Skip_SB(                                                    "pbt_enable_flag");

    if (profile_id == 0x30 || profile_id == 0x32) {
        Skip_SB(                                                "pmc_enable_flag");
        Skip_SB(                                                "iip_enable_flag");
        Skip_SB(                                                "sawp_enable_flag");
        if (affine_enable_flag)
            Skip_SB(                                            "asr_enable_flag");
        Skip_SB(                                                "awp_enable_flag");
        Skip_SB(                                                "etmvp_mvap_enable_flag");
        Skip_SB(                                                "dmvr_enable_flag");
        Skip_SB(                                                "bio_enable_flag");
        Skip_SB(                                                "bgc_enable_flag");
        Skip_SB(                                                "inter_pf_enable_flag");
        Skip_SB(                                                "inter_pc_enable_flag");
        Skip_SB(                                                "obmc_enable_flag");
        Skip_SB(                                                "sbt_enable_flag");
        Skip_SB(                                                "ist_enable_flag");
        Skip_SB(                                                "esao_enable_flag");
        Skip_SB(                                                "ccsao_enable_flag");
        if (alf_enable_flag)
            Skip_SB(                                            "ealf_enable_flag");
        Get_SB(ibc_enable_flag,                                 "ibc_enable_flag");
        Mark_1();
        Get_SB(isc_enable_flag,                                 "isc_enable_flag");
        if (ibc_enable_flag || isc_enable_flag)
            Skip_S1( 4,                                         "num_of_intra_hmvp_cand");
        Skip_SB(                                                "fimc_enable_flag");
        Get_S1(8, nn_tools_set_hook,                            "nn_tools_set_hook");
        bool NnFilterEnableFlag = nn_tools_set_hook & 0x01;
        if (NnFilterEnableFlag)
            Skip_UE(                                            "num_of_nn_filter_minus1");
        Mark_1();
    }
    if (low_delay == 0) {
        Skip_S1(5,                                              "output_reorder_delay");
    }
    Skip_SB(                                                    "cross_patch_loop_filter_enable_flag");
    Skip_SB(                                                    "ref_colocated_patch_flag");
    TEST_SB_SKIP(                                               "stable_patch_flag");
        TEST_SB_SKIP(                                           "uniform_patch_flag");
            Mark_1();
            Skip_UE(                                            "patch_width_minus1");
            Skip_UE(                                            "patch_height_minus1");
        TEST_SB_END();
    TEST_SB_END();

    Skip_SB(                                                    "reserved");
    Skip_SB(                                                    "reserved");
    BS_End();

    //Not sure, but the 3 first official files have this
    if (Element_Size-Element_Offset)
    {
        BS_Begin();
        Mark_1();
        BS_End();
    }

    FILLING_BEGIN();
        //NextCode
        NextCode_Clear();
        NextCode_Add(User_Data_Start_Code); //user_data_start
        NextCode_Add(Intra_Picture_Start_Code); //picture_start (I)
        NextCode_Add(Extension_Start_Code); //extension_start

        //Autorisation of other streams
        Streams[Video_Sequence_End_Code].Searching_Payload=true, //video_sequence_end
        Streams[User_Data_Start_Code].Searching_Payload=true; //user_data_start
        Streams[Intra_Picture_Start_Code].Searching_Payload=true, //picture_start (I)
        Streams[Avs3_Reserved_B4].Searching_Payload=true, //reserved
        Streams[Extension_Start_Code].Searching_Payload=true; //extension_start
        Streams[Inter_Picture_Start_Code].Searching_Payload=true, //picture_start (P or B)
        Streams[Video_Edit_Code].Searching_Payload=true; //video_edit
        Streams[Avs3_Reserved_B8].Searching_Payload=true, //reserved

        video_sequence_start_IsParsed=true;
    FILLING_END();
}

//---------------------------------------------------------------------------
// Packet "B1"
void File_Avs3V::video_sequence_end()
{
    Element_Name("video_sequence_start");

    FILLING_BEGIN();
        //NextCode
        NextCode_Clear();
        NextCode_Add(Video_Sequence_Start_Code); //SeqenceHeader
    FILLING_END();
}

//---------------------------------------------------------------------------
// Packet "B2", User defined size, this is often used of library name
void File_Avs3V::user_data_start()
{
    Element_Name("user_data_start");

    /* TODO: too many false positives
    
    //Rejecting junk from the end
    size_t Library_End_Offset=(size_t)Element_Size;
    while (Library_End_Offset>0
        && (Buffer[Buffer_Offset+Library_End_Offset-1]<0x20
         || Buffer[Buffer_Offset+Library_End_Offset-1]>0x7D
         || (Buffer[Buffer_Offset+Library_End_Offset-1]>=0x3A
          && Buffer[Buffer_Offset+Library_End_Offset-1]<=0x40)))
        Library_End_Offset--;
    if (Library_End_Offset==0)
        return; //No good info

    //Accepting good data after junk
    size_t Library_Start_Offset=Library_End_Offset-1;
    while (Library_Start_Offset>0 && (Buffer[Buffer_Offset+Library_Start_Offset-1]>=0x20 && Buffer[Buffer_Offset+Library_Start_Offset-1]<=0x7D))
        Library_Start_Offset--;

    //But don't accept non-alpha characters at the beginning (except for "3ivx")
    if (Library_End_Offset-Library_Start_Offset!=4 || CC4(Buffer+Buffer_Offset+Library_Start_Offset)!=0x33697678) //3ivx
        while (Library_Start_Offset<Library_End_Offset && Buffer[Buffer_Offset+Library_Start_Offset]<=0x40)
            Library_Start_Offset++;

    //Parsing
    Ztring Temp;
    if (Library_Start_Offset>0)
        Skip_XX(Library_Start_Offset,                           "junk");
    if (Library_End_Offset-Library_Start_Offset)
        Get_UTF8(Library_End_Offset-Library_Start_Offset, Temp, "data");
    if (Element_Offset<Element_Size)
        Skip_XX(Element_Size-Element_Offset,                    "junk");
    */

    FILLING_BEGIN();
        //NextCode
        NextCode_Test();

        /*
        if (Temp.size() >= 4)
            Library=Temp;
        */
    FILLING_END();
}

//---------------------------------------------------------------------------
int8u File_Avs3V::NumberOfFrameCentreOffsets()
{
    if (progressive_sequence)
    {
        if (repeat_first_field)
            return top_field_first ? 3 : 2;
        else return 1;
    }
    else
    {
        if (picture_structure == 0)
            return 1;
        else
            return repeat_first_field ? 3 : 2;
    }
}

//---------------------------------------------------------------------------
// Packet "B5"
void File_Avs3V::extension_start()
{
    Element_Name("Extension");

    //Parsing
    int8u extension_start_code_identifier;
    BS_Begin();
    Get_S1 ( 4, extension_start_code_identifier,                "extension_start_code_identifier"); 
    Param_Info1(Avs3V_extension_start_code_identifier[extension_start_code_identifier]);
    Element_Info1(Avs3V_extension_start_code_identifier[extension_start_code_identifier]);

    switch (extension_start_code_identifier)
    {
        case 2  : //sequence_display
                {
                    //Parsing
                    Get_S1 ( 3, video_format,                   "video_format"); 
                    Param_Info1(Avs3V_video_format[video_format]);
                    Skip_SB(                                    "sample_range");
                    TEST_SB_SKIP(                               "colour_description");
                        Get_S1( 8, colour_primaries,            "colour_primaries");
                        Get_S1( 8, transfer_characteristics,    "transfer_characteristics");
                        Get_S1( 8, matrix_coefficients,         "matrix_coefficients");
                    TEST_SB_END();
                    Get_S2 (14, display_horizontal_size,        "display_horizontal_size");
                    Mark_1 ();
                    Get_S2 (14, display_vertical_size,          "display_vertical_size");
                    TEST_SB_SKIP(                               "td_mode_flag");
                        Skip_S1(8,                              "td_packing_mode");
                        Skip_SB(                                "view_reverse_flag");
                    TEST_SB_END();
                    BS_End();
                }
                break;
        case 3: // temporal scalabiity
                {
                    int8u num_of_temporal_layers_minus1, i;
                    Get_S1(3, num_of_temporal_layers_minus1,    "num_of_temporal_layers_minus1");
                    for (i=0; i<num_of_temporal_layers_minus1; i++) {
                        char elName[64]; sprintf(elName,        "temporal_frame_rate_code[%i]", i);
                        Skip_S1(4, elName);
                        sprintf(elName,                         "temporal_bit_rate_lower[%i]", i);
                        Skip_S3(18, elName);
                        Mark_1();
                        sprintf(elName,                         "temporal_bit_rate_upper[%i]", i);
                        Skip_S2(12, elName);
                    }
                    BS_End();
                }
                break; 
        case 4  : //copyright
                {
                    //Parsing
                    Skip_SB(                                    "copyright_flag");
                    Skip_S1( 8,                                 "copyright_id");
                    Skip_SB(                                    "original_or_copy");
                    Skip_S1( 7,                                 "reserved");
                    Mark_1 ();
                    Info_S3(20, copyright_number_1,             "copyright_number_1");
                    Mark_1 ();
                    Info_S3(22, copyright_number_2,             "copyright_number_2");
                    Mark_1 ();
                    Info_S3(22, copyright_number_3,             "copyright_number_3"); Param_Info1(Ztring::ToZtring(((int64u)copyright_number_1<<44)+((int64u)copyright_number_2<<22)+(int64u)copyright_number_3, 16));
                    BS_End();
                }
                break;
        case 5  : // high dynamic range picture metadata
                {
                    //Parsing
                    int8u hdr_dynamic_metadata_type;
                    Get_S1(4, hdr_dynamic_metadata_type,        "hdr_dynamic_metadata_type");
                    switch (hdr_dynamic_metadata_type) {
                        // values 1, 6, 4 according to DVB Bluebook A001, 5 in AVSV (T/AI 109.2)
                        case 1: DMI_Found |= HAVE_HDR10plus_DMI; break;
                        case 4: DMI_Found |= HAVE_DolbyVision_DMI; break;
                        case 5: DMI_Found |= HAVE_HDRVivid_DMI; break;
                        case 6: DMI_Found |= HAVE_SLHDR2_DMI; break;
                    }
                    BS_End();
                    Skip_XX(Element_Size - Element_Offset,      "extension_data_byte");
                }
                break;
        case 7:  // picture display
                {
                    int8u i;
                    for (i=0; i<NumberOfFrameCentreOffsets(); i++) {
                        Skip_S2(16,                             "picture_centre_horizontal_offset");
                        Mark_1();
                        Skip_S2(16,                             "picture_centre_vertical_offset");
                        Mark_1();
                    }
                    BS_End();
                }
                break;
        case 10: // mastering display and content metadata
                {
                    //Parsing
                    int8u c;
                    for (c = 0; c<3; c++) {
                        char idx[64], idy[64]; 
                        sprintf(idx,                            "display_primaries_x[%i]", c); 
                        Skip_S2(16, idx);
                        Mark_1();
                        sprintf(idy,                            "display_primaries_y[%i]", c);
                        Skip_S2(16, idy);
                        Mark_1();
                    }
                    Skip_S2(16,                                 "white_point_x");
                    Mark_1();
                    Skip_S2(16,                                 "white_point_y");
                    Mark_1();
                    Skip_S2(16,                                 "max_mastering_display_luminance");
                    Mark_1();
                    Skip_S2(16,                                 "min_mastering_display_luminance");
                    Mark_1();
                    Get_S2(16, max_content_light_level,         "max_content_light_level");
                    have_MaxCLL=true;
                    Mark_1();
                    Get_S2(16, max_picture_average_light_level, "max_picture_average_light_level");
                    have_MaxFALL=true;
                    Mark_1();
                    Skip_S2(16,                                 "reserved");
                    BS_End();
                }
                break;
        case 11 : //camera_parameters
                {
                    //Parsing
                    Skip_SB(                                    "reserved");
                    Skip_S1( 7,                                 "camera_id");
                    Mark_1 ();
                    Skip_S3(22,                                 "height_of_image_device");
                    Mark_1 ();
                    Skip_S3(22,                                 "focal_length");
                    Mark_1 ();
                    Skip_S3(22,                                 "f_number");
                    Mark_1 ();
                    Skip_S3(22,                                 "vertical_angle_of_view");
                    Mark_1 ();
                    Skip_S3(16,                                 "camera_position_x_upper");
                    Mark_1 ();
                    Skip_S3(16,                                 "camera_position_x_lower");
                    Mark_1 ();
                    Skip_S3(16,                                 "camera_position_y_upper");
                    Mark_1 ();
                    Skip_S3(16,                                 "camera_position_y_lower");
                    Mark_1 ();
                    Skip_S3(16,                                 "camera_position_z_upper");
                    Mark_1 ();
                    Skip_S3(16,                                 "camera_position_z_lower");
                    Mark_1 ();
                    Skip_S3(22,                                 "camera_direction_x");
                    Mark_1 ();
                    Skip_S3(22,                                 "camera_direction_y");
                    Mark_1 ();
                    Skip_S3(22,                                 "camera_direction_z");
                    Mark_1 ();
                    Skip_S3(22,                                 "image_plane_vertical_x");
                    Mark_1 ();
                    Skip_S3(22,                                 "image_plane_vertical_y");
                    Mark_1 ();
                    Skip_S3(22,                                 "image_plane_vertical_z");
                    Mark_1 ();
                    Skip_S2(16,                                 "reserved");
                    BS_End();
                }
                break;
        case 13: //referemce_to_library_picture
                {
                    int8u crr_lib_number, i=0;
                    char t[64];
                    //Parsing
                    Get_S1(3, crr_lib_number,                   "crr_lib_number");
                    Mark_1();
                    while (i < crr_lib_number) {
                        sprintf(t,                              "crr_lib_pid[%d]", i);
                        Skip_S2(9, t);
                        i++;
                        if (i % 2 == 0)
                            Mark_1();
                    }
                    BS_End();
                }
                break;
        case 14: // adaptive_intra_refresh
                {
                    Skip_S2(10,                                 "air_bound_x");
                    Skip_S2(10,                                 "air_bound_y");
                    BS_End();
                }
                break;
        default :
                {
                    //Parsing
                    Skip_S1(4,                                  "data");
                    BS_End();
                    Skip_XX(Element_Size-Element_Offset,        "data");
                }
    }

    //Not sure, but the 3 first official files have this
    if (Element_Size-Element_Offset)
    {
        BS_Begin();
        Mark_1();
        BS_End();
    }

    FILLING_BEGIN();
        //NextCode
        NextCode_Test();
    FILLING_END();
}

//---------------------------------------------------------------------------
// Packet Inter_Picture_Start_Code or Intra_Picture_Start_Code
void File_Avs3V::picture_start()
{
    Accept();

    //Name
    Element_Name("picture_start");

    //Parsing
    int8u picture_coding_type=(int8u)-1;
    BS_Begin();
    if (Element_Code==Inter_Picture_Start_Code)
        Skip_SB(                                                "random_access_decodabe_flag");
    Skip_S4(32,                                                 "bbv_delay");

    if (Element_Code==Inter_Picture_Start_Code)
    {
        Get_S1(2, picture_coding_type,                          "picture_coding_type"); Element_Info1(Avs3V_picture_coding_type[picture_coding_type]);
    }
    else
        Element_Info1("I");
 
    if (Element_Code==Intra_Picture_Start_Code) //Only I
    {
        TEST_SB_SKIP(                                           "time_code_flag");
            Skip_SB(                                            "time_code_dropframe");
            Skip_S1(5,                                          "time_code_hours");
            Skip_S1(6,                                          "time_code_minutes");
            Skip_S1(6,                                          "time_code_seconds");
            Skip_S1(6,                                          "time_code_pictures");
        TEST_SB_END();
    }

    Skip_S1( 8,                                                 "decode_order_index");
    if (Element_Code==Intra_Picture_Start_Code)
        if (library_stream_flag)
            Skip_UE(                                            "library_picture_index");

    if (temporal_id_enable_flag)
        Skip_S1(3,                                              "temporal_id");
    if (!low_delay)
        Skip_UE(                                                "picture_output_delay");
    if (low_delay)
        Skip_UE(                                                "bbv_check_times");
    Get_SB (    progressive_frame,                              "progressive_frame");
    if (!progressive_frame)
    {
        repeat_first_field=0;
        Get_SB(    picture_structure,                           "picture_structure");
    }
    Get_SB (    top_field_first,                                "top_field_first");
    Get_SB (    repeat_first_field,                             "repeat_first_field");
    if (field_coded_sequence) {
        Skip_SB(                                                "top_field_picture_flag");
        Skip_S1(1,                                              "reserved_bits");
    }
    BS_End();

    if (Element_Size-Element_Offset)
        Skip_XX(Element_Size-Element_Offset,                    "Unknown");

    FILLING_BEGIN();
        //NextCode
        NextCode_Test();
        NextCode_Clear();
        for (int8u Pos=0x00; Pos<=0xAF; Pos++)
            NextCode_Add(Pos); //slice
        NextCode_Add(Video_Sequence_Start_Code);
        NextCode_Add(Intra_Picture_Start_Code);
        NextCode_Add(Inter_Picture_Start_Code);
        NextCode_Add(User_Data_Start_Code);
        NextCode_Add(Extension_Start_Code); // PH - AVSV allows extension data after the picture

        //Authorisation of other streams
        for (int8u Pos=0x00; Pos<=0xAF; Pos++)
            Streams[Pos].Searching_Payload=true; //slice
    FILLING_END();
}

//---------------------------------------------------------------------------
// Packet "B7"
void File_Avs3V::video_edit()
{
    Element_Name("video_edit");
}

//---------------------------------------------------------------------------
// Packet "B4" and "B8"
void File_Avs3V::reserved()
{
    Element_Name("reserved");

    //Parsing
    if (Element_Size)
        Skip_XX(Element_Size,                                   "reserved");
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_AVS3V_YES

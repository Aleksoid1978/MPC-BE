/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about AVS 3 Video files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_Avs3V
#define MediaInfo_Avs3V
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/Multiple/File_Mpeg4.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Avs3V
//***************************************************************************

class File_Avs3V : public File__Analyze
{
public :
    //In
    int64u Frame_Count_Valid;
    bool   FrameIsAlwaysComplete;

    //constructor/Destructor
    File_Avs3V();

private :
    //Streams management
    void Streams_Fill();
    void Streams_Finish();

    //Buffer - File header
    bool FileHeader_Begin() {return FileHeader_Begin_0x000001();}

    //Buffer - Synchro
    bool Synchronize() {return Synchronize_0x000001();}
    bool Synched_Test();
    void Synched_Init();

    //Buffer - Per element
    void Header_Parse();
    bool Header_Parser_QuickSearch();
    bool Header_Parser_Fill_Size();
    void Data_Parse();

    //Elements
    void slice();
    void video_sequence_start();
    void video_sequence_end();
    void user_data_start();
    void extension_start();
    void picture_start();
    void video_edit();
    void reserved();

    void reference_picture_list_set(int8u list, int32u rpls);
    void weight_quant_matrix();

    int8u NumberOfFrameCentreOffsets();

    //Count of a Packets
    size_t progressive_frame_Count =0;
    size_t Interlaced_Top = 0;
    size_t Interlaced_Bottom = 0;

    //From user_data
    Ztring Library;
    Ztring Library_Name;
    Ztring Library_Version;
    Ztring Library_Date;

    //Temp
    int32u  bit_rate = 0;                           //From video_sequence_start
    int16u  horizontal_size = 0;                    //From video_sequence_start
    int16u  vertical_size = 0;                      //From video_sequence_start
    int16u  display_horizontal_size = 0;            //From sequence_display
    int16u  display_vertical_size = 0;              //From sequence_display
    int8u   profile_id = 0;                         //From video_sequence_start
    int8u   level_id = 0;                           //From video_sequence_start
    int8u   chroma_format = 0;                      //From video_sequence_start
    int8u   sample_precision = 0;                   //From video_sequence_start
    int8u   encoding_precision = 0;                 //From video_sequence_start
    int8u   aspect_ratio = 0;                       //From video_sequence_start
    int8u   frame_rate_code = 0;                    //From video_sequence_start
    int8u   video_format = 0;                       //From sequence_display;

    int8u   num_of_hmvp_cand = 0;
    int8u   nn_tools_set_hook = 0;
    int8u   colour_primaries = 0;                   //From sequence display extension
    int8u   transfer_characteristics = 0;           //From sequence display extension
    int8u   matrix_coefficients = 0;                //From sequence display extension

    bool    have_MaxCLL = false;
    int16u  max_content_light_level = 0;            //From mastering diaplay and content metadata extension
    bool    have_MaxFALL = false;
    int16u  max_picture_average_light_level = 0;    //From mastering diaplay and content metadata extension

    int32u  num_ref_pic_list_set[2];
    bool    progressive_frame = false;
    bool    picture_structure = false;
    bool    top_field_first = false;
    bool    repeat_first_field = false;

    int8u   DMI_Found = 0;
    bool    LastElementIsSlice = false;

    bool    progressive_sequence = false;               //From video_sequence_start
    bool    field_coded_sequence = false;               //From video_sequence_start
    bool    library_stream_flag = false;                //From video_sequence_start
    bool    library_picture_enable_flag = false;        //From video_sequence_start
    bool    duplicate_sequence_header_flag = false;     //From video_sequence_start
    bool    low_delay = false;                          //From video_sequence_start
    bool    temporal_id_enable_flag = false;
    bool    video_sequence_start_IsParsed = false;      //From video_sequence_start
    bool    rpl1_same_as_rpl0_flag = false;             //From video_sequence_start
    bool    weight_quant_enable_flag = false;
    bool    load_seq_weight_quant_data_flag = false;

    bool    alf_enable_flag = false;
    bool    affine_enable_flag = false;
    bool    amvr_enable_flag = false;

    bool    ibc_enable_flag = false;
    bool    isc_enable_flag = false;

    bool    picture_alf_enable_flag[3];

    std::vector<stream_payload> Streams;

};

} //NameSpace

#endif

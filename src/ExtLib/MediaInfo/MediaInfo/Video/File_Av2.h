/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_Av2H
#define MediaInfo_Av2H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Av2
//***************************************************************************

class File_Av2 : public File__Analyze
{
public :
    //In
    int64u  Frame_Count_Valid{};
    bool    IsAnnexB{};

    //Constructor/Destructor
    File_Av2();
    ~File_Av2() override;

private :
    //Streams management
    void Streams_Accept() override;
    void Streams_Fill() override;
    void Streams_Finish() override;

    //Buffer - Global
    void Read_Buffer_OutOfBand() override;
    void Read_Buffer_Init() override;

    //Buffer - Per element
    void Header_Parse() override;
    void Data_Parse() override;

    //Elements
    void obu_extension_data(int64u sz);
    void trailing_bits(int64u nbBits);
    void byte_alignment();
    void reserved_obu();
    void sequence_header_obu();
    void sequence_partition_config();
    void sequence_segment_config();
    void sequence_intra_config();
    void sequence_inter_config();
    void sequence_scc_config();
    void sequence_transform_quant_entropy_config();
    void seg_info(int8u numSegments);
    void sequence_filter_config();
    void temporal_delimiter_obu();
    void multi_stream_decoder_operation_obu();
    void multi_frame_header_obu();
    void layer_config_record_obu();
    void atlas_segment_info_obu();
    void operating_point_set_obu();
    void buffer_removal_timing_obu();
    void quantizer_matrix_obu();
    void film_grain_obu();
    void content_interpretation_obu();
    void padding_obu();
    void metadata_unit(int64u metadata_type, int64u payload_size);
    void metadata_short_obu();
    void metadata_group_obu();
    void metadata_itu_t_t35();
    void metadata_hdr_cll();
    void metadata_hdr_mdcv();
    void metadata_timecode();
    void metadata_banding_hints();
    void metadata_icc_profile();
    void metadata_scan_type();
    void metadata_temporal_point_info();

    void frame_header(bool isFirst);
    void tile_params(int16u frameWidth, int16u frameHeight, int8u uniformSbSize, int8u sbSize, bool isBridge);
    void tile_group_obu(int64u obuPayloadSize);

    //Functions
    bool  is_extensible_obu(int8u obu_type) const;
    int8u get_seq_sb_size() const;
    int   tile_log2(int blkSize, int target) const;

    //Custom functions for common code
    void  obu_end();

    //Temp
    int8u   obu_mlayer_id{};
    int8u   obu_xlayer_id{};
    bool    sequence_header_Parsed{};
    bool    SeenFrameHeader{};
    bool    single_picture_header_flag{};
    bool    Monochrome{};
    bool    use_256x256_superblock{};
    bool    use_128x128_superblock{};
    bool    ColorDescriptionPresent{};
    bool    FullRange{};
    int8u   ColorPrimaries{ 2 };
    int8u   TransferCharacteristics{ 2 };
    int8u   MatrixCoefficients{ 2 };
    int32u  chroma_format_idc{};
    Ztring  maximum_content_light_level;
    Ztring  maximum_frame_average_light_level;
    enum hdr_format
    {
        HdrFormat_SmpteSt209440,
        HdrFormat_SmpteSt2086,
        HdrFormat_Max,
    };
    typedef std::map<video, Ztring[HdrFormat_Max]> hdr;
    hdr     HDR;

    //Helpers
    void Get_leb128(int64u& Info, const char* Name);
    void Get_uvlc(int32u& Info, const char* Name);
    void Skip_uvlc(const char* Name);
    void Get_su(int8u n, int64s& Info, const char* Name);
    void Skip_su(int8u n, const char* Name);
    void Get_ns(int8u n, int64u& Info, const char* Name);
    void Skip_ns(int8u n, const char* Name);
    void Get_rg(int8u n, int64u& Info, const char* Name);
};

} //NameSpace

#endif

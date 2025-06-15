/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about AAC files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_AacH
#define MediaInfo_File_AacH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#ifdef MEDIAINFO_MPEG4_YES
    #include "MediaInfo/Multiple/File_Mpeg4_Descriptors.h"
#endif
#include "MediaInfo/Tag/File__Tags.h"
#include "MediaInfo/Audio/File_Usac.h"
#include "MediaInfo/Audio/File_Aac_GeneralAudio_Sbr.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
enum Aac_OutputChannel
{
    //USAC
    CH_M_L030,
    CH_M_R030,
    CH_M_000,
    CH_LFE,
    CH_M_L110,
    CH_M_R110,
    CH_M_L022,
    CH_M_R022,
    CH_M_L135,
    CH_M_R135,
    CH_M_180,
    CH_M_LSD,
    CH_M_RSD,
    CH_M_L090,
    CH_M_R090,
    CH_M_L060,
    CH_M_R060,
    CH_U_L030,
    CH_U_R030,
    CH_U_000,
    CH_U_L135,
    CH_U_R135,
    CH_U_180,
    CH_U_L090,
    CH_U_R090,
    CH_T_000,
    CH_LFE2,
    CH_L_L045,
    CH_L_R045,
    CH_L_000,
    CH_U_L110,
    CH_U_R110,
    //MPEG-H 3D Audio
    CH_U_L045,
    CH_U_R045,
    CH_M_L045,
    CH_M_R045,
    CH_LFE3,
    CH_M_LSCR,
    CH_M_RSCR,
    CH_M_LSCH,
    CH_M_RSCH,
    CH_M_L150,
    CH_M_R150,
    CH_MAX
};

//***************************************************************************
// Class File_Aac
//***************************************************************************

typedef const int8s (*sbr_huffman)[2];

class File_Aac : public File_Usac, public File__Tags_Helper
{
public :
    //In
    int64u  Frame_Count_Valid;
    enum mode
    {
        Mode_Unknown,
        Mode_AudioSpecificConfig,
        Mode_payload,
        Mode_ADIF,
        Mode_ADTS,
        Mode_LATM,
    };
    mode   Mode;
    bool   FromIamf;
    void   AudioSpecificConfig_OutOfBand(int64s sampling_frequency, int8u audioObjectType=(int8u)-1, bool sbrData=false, bool psData=false, bool sbrPresentFlag=false, bool psPresentFlag=false);

    // Conformance
    #if MEDIAINFO_CONFORMANCE
        int16u SamplingRate;
    #endif

    //Constructor/Destructor
    File_Aac();
    ~File_Aac();

protected :
    //Streams management
    void Streams_Accept();
    void Streams_Fill();
    void Streams_Update();
    void Streams_Finish();

    //Buffer - File header
    bool FileHeader_Begin();
    void FileHeader_Parse();
    void FileHeader_Parse_ADIF();
    void FileHeader_Parse_ADTS();

    //Buffer - Global
    void Read_Buffer_Init();
    void Read_Buffer_Continue ();
    void Read_Buffer_Continue_AudioSpecificConfig();
    void Read_Buffer_Continue_payload();
    void Read_Buffer_Unsynched();

    //Buffer - Synchro
    bool Synchronize();
    bool Synchronize_ADTS();
    bool Synchronize_LATM();
    bool Synched_Test();
    bool Synched_Test_ADTS();
    bool Synched_Test_LATM();

    //Buffer - Demux
    #if MEDIAINFO_DEMUX
    bool Demux_UnpacketizeContainer_Test();
    bool Demux_UnpacketizeContainer_Test_ADTS();
    bool Demux_UnpacketizeContainer_Test_LATM();
    #endif //MEDIAINFO_DEMUX

    //Buffer - Per element
    bool Header_Begin();
    bool Header_Begin_ADTS();
    bool Header_Begin_LATM();
    void Header_Parse();
    void Header_Parse_ADTS();
    void Header_Parse_LATM();
    void Data_Parse();
    void Data_Parse_ADTS();
    void Data_Parse_LATM();

    //***********************************************************************
    // Elements - Main
    //***********************************************************************

    //Elements - Interface to MPEG-4 container
    void AudioSpecificConfig                (size_t End=(size_t)-1);
    void GetAudioObjectType                 (int8u &ObjectType, const char* Name);

    //Elements - Multiplex layer
    void EPMuxElement                       ();
    void AudioMuxElement                    ();
    void StreamMuxConfig                    ();
    int32u LatmGetValue                     ();
    void PayloadLengthInfo                  ();
    void PayloadMux                         ();
    bool muxConfigPresent;

    //Elements - Error protection
    void ErrorProtectionSpecificConfig      ();

    //Elements - MPEG-2 AAC Audio_Data_Interchange_Format, ADIF
    void adif_header                        ();

    //Elements - Audio_Data_Transport_Stream frame, ADTS
    void adts_frame                         ();
    void adts_fixed_header                  ();
    void adts_variable_header               ();

    //Temp
    int8u   numSubFrames;
    int8u   numProgram;
    int8u   numLayer;
    int8u   numChunk;
    bool    audioMuxVersionA;
    int8u   streamID[16][8];
    int8u   progSIndx[128];
    int8u   laySIndx[128];
    int8u   progCIndx[128];
    int8u   layCIndx[128];
    int8u   frameLengthType[128];
    int16u  frameLength[128];
    int32u  MuxSlotLengthBytes[128];
    int32u  otherDataLenBits;
    bool    allStreamsSameTimeFraming;
    int8u   audioObjectType;
    int8u   extensionAudioObjectType;
    int16u  frame_length;
    int32u  extension_sampling_frequency;
    bool    aacScalefactorDataResilienceFlag;
    bool    aacSectionDataResilienceFlag;
    bool    aacSpectralDataResilienceFlag;
    int8u   num_raw_data_blocks;
    bool    protection_absent;
    int64u  FrameSize_Min;
    int64u  FrameSize_Max;
    bool    adts_buffer_fullness_Is7FF;
    #if MEDIAINFO_ADVANCED
        int64u  aac_frame_length_Total;
    #endif //MEDIAINFO_ADVANCED
    #if MEDIAINFO_MACROBLOCKS
        int     ParseCompletely;
    #else //MEDIAINFO_MACROBLOCKS
        static constexpr int ParseCompletely=0;
    #endif //MEDIAINFO_MACROBLOCKS

    //***********************************************************************
    // Elements - Speech coding (HVXC)
    //***********************************************************************

    void HvxcSpecificConfig                 ();
    void HVXCconfig                         ();
    void ErrorResilientHvxcSpecificConfig   ();
    void ErHVXCconfig                       ();

    //***********************************************************************
    // Elements - Speech Coding (CELP)
    //***********************************************************************

    void CelpSpecificConfig                 ();
    void CelpHeader                         ();
    void ErrorResilientCelpSpecificConfig   ();
    void ER_SC_CelpHeader                   ();

    //***********************************************************************
    // Elements - General Audio (GA)
    //***********************************************************************

    //Elements - Decoder configuration
    void GASpecificConfig                   ();
    void program_config_element             ();

    //Elements - GA bitstream
    void payload                            (size_t BitsNotIncluded=(size_t)-1);
    void raw_data_block                     ();
    void single_channel_element             ();
    void channel_pair_element               ();
    void ics_info                           ();
    void pulse_data                         ();
    void coupling_channel_element           ();
    void lfe_channel_element                ();
    void data_stream_element                ();
    void fill_element                       (int8u old_id);
    void gain_control_data                  ();

    //Elements - Subsidiary
    void individual_channel_stream          (bool common_window, bool scale_flag);
    void section_data                       ();
    void scale_factor_data                  ();
    void tns_data                           ();
    void ltp_data                           ();
    void spectral_data                      ();
    void extension_payload                  (size_t End, int8u id_aac);
    void dynamic_range_info                 ();
    void sac_extension_data                 (size_t End);

    //Elements - SBR
    void sbr_extension_data                 (size_t End, int8u id_aac, bool crc_flag);
    void sbr_header                         ();
    void sbr_data                           (int8u id_aac);
    void sbr_single_channel_element         ();
    void sbr_channel_pair_element           ();
    void sbr_grid                           (bool ch);
    void sbr_dtdf                           (bool ch);
    void sbr_invf                           (bool ch);
    void sbr_envelope                       (bool ch, bool bs_coupling);
    void sbr_noise                          (bool ch, bool bs_coupling);
    void sbr_sinusoidal_coding              (bool ch);
    int16u sbr_huff_dec                     (const sbr_huffman& Table, const char* Name);

    //Elements - SBR - Extensions
    void ps_data                            (size_t End);
    void esbr_data                          (size_t End);

    //Elements - Perceptual noise substitution (PNS)
    bool is_noise                           (size_t group, size_t sfb);
    int  is_intensity                       (size_t group, size_t sfb);

    //Elements - Enhanced Low Delay Codec
    void ELDSpecificConfig                  ();
    void ld_sbr_header                      ();

    //Helpers
    void hcod                               (int8u sect_cb, const char* Name);
    void hcod_sf                            (const char* Name);
    void hcod_binary                        (int8u CodeBook, int8s* Values, int8u Values_Count);
    void hcod_2step                         (int8u CodeBook, int8s* Values, int8u Values_Count);

    //Temp - channel_pair_element
    bool    common_window;

    //Temp - ics_info
    int8u   window_sequence;
    int8u   max_sfb;
    int8u   scale_factor_grouping;
    int8u   num_windows;
    int8u   num_window_groups;
    int8u   window_group_length             [8];
    int16u  sect_sfb_offset                 [8][1024];
    int16u  swb_offset                      [64];
    int8u   sfb_cb                          [8][64];
    int8u   num_swb;

    //Temp - section_data
    int8u   num_sec                         [8];
    int8u   sect_cb                         [8][64];
    int16u  sect_start                      [8][64];
    int16u  sect_end                        [8][64];

    //Temp - ltp_data
    int16u  ltp_lag;

    //Temp - SBR
    vector<sbr_handler*> sbrs;
    sbr_handler* sbr;

    //Temp - PS
    vector<ps_handler*> pss;
    ps_handler* ps;

    //Temp - Position
    size_t  raw_data_block_Pos;
    size_t  ChannelPos_Temp;
    size_t  ChannelCount_Temp;

    //***********************************************************************
    // Elements - Structured Audio (SA)
    //***********************************************************************

    void StructuredAudioSpecificConfig      ();

    //***********************************************************************
    // Elements - Text to Speech Interface (TTSI)
    //***********************************************************************

    void TTSSpecificConfig                  ();

    //***********************************************************************
    // Elements - Parametric Audio (HILN)
    //***********************************************************************

    void HILNconfig                         ();
    void HILNenexConfig                     ();
    void ParametricSpecificConfig           ();
    void PARAconfig                         ();

    //***********************************************************************
    // Elements - Technical description of parametric coding for high quality audio
    //***********************************************************************

    void SSCSpecificConfig                  ();

    //***********************************************************************
    // Elements - MPEG-1/2 Audio
    //***********************************************************************

    void MPEG_1_2_SpecificConfig            ();

    //***********************************************************************
    // Elements - Technical description of lossless coding of oversampled audio
    //***********************************************************************

    void DSTSpecificConfig                  ();
    //***********************************************************************
    // Elements - Audio Lossless
    //***********************************************************************

    void ALSSpecificConfig                  ();

    //***********************************************************************
    // Elements - Scalable lossless
    //***********************************************************************

    void SLSSpecificConfig                  ();

    //***********************************************************************
    // Temp
    //***********************************************************************

    std::map<std::string, Ztring>   Infos_General;
    std::map<std::string, Ztring>   Infos;
    std::map<std::string, Ztring>   Infos_AudioSpecificConfig;
    bool                            CanFill;

private :
    void FillInfosHEAACv2(const Ztring& Format_Settings);
};

} //NameSpace

#endif


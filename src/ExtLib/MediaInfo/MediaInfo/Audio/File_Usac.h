/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about USAC payload of various files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_UsacH
#define MediaInfo_File_UsacH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#ifdef MEDIAINFO_MPEG4_YES
    #include "MediaInfo/Multiple/File_Mpeg4_Descriptors.h"
#endif
#include "MediaInfo/Audio/File_Aac_GeneralAudio_Sbr.h"
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/TimeCode.h"
#include <cstring>
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

namespace MediaInfoLib
{

constexpr int8u Aac_Channels_Size_Usac = 14; // USAC expands Aac_Channels[]
constexpr int8u Aac_Channels_Size = 21; //MPEG-H 3D Audio expands Aac_Channels[]
constexpr int8u Aac_sampling_frequency_Size = 13;
constexpr int8u Aac_sampling_frequency_Size_Usac = 31; // USAC expands Aac_sampling_frequency[]

struct stts_struct;
struct edts_struct;
struct sgpd_prol_struct;
struct sbgp_struct;

//***************************************************************************
// Class File_Usac
//***************************************************************************

class File_Usac : public File__Analyze
{
public :
    //Constructor/Destructor
    File_Usac();
    ~File_Usac();

    // Temp
    void   hcod_sf(const char* Name);
    int32u arith_decode(int16u& low, int16u& high, int16u& value, const int16u* cf, int32u cfl, size_t* TooMuch);
    int16u sbr_huff_dec(const int8s(*Table)[2], const char* Name);
    int16s huff_dec_1D(const int16s (*Table)[2], const char* Name);
    bool   huff_dec_2D(const int16s (*Table)[2], int8s (&aTmp)[2], const char* Name);
    enum ec_data_type
    {
        CLD,
        ICC,
        IPD
    };
    enum frame_class
    {
        FIXFIX,
        FIXVAR,
        VARFIX,
        VARVAR
    };
    enum sequence_type
    {
        ONLY_LONG_SEQUENCE,
        LONG_START_SEQUENCE,
        EIGHT_SHORT_SEQUENCE,
        LONG_STOP_SEQUENCE
    };

    #if MEDIAINFO_CONFORMANCE
    enum conformance_level
    {
        Error,
        Warning,
        Info,
        ConformanceLevel_Max
    };
    struct field_value
    {
        string Field;
        string Value;
        bitset8 Flags;
        struct frame_pos
        {
            int64u Main;
            int64u Sub;
        };
        vector<frame_pos> FramePoss;

        field_value(string&& Field, string&& Value, bitset8 Flags, int64u FramePos, int64u SubFramePos)
            : Field(Field)
            , Value(Value)
            , Flags(Flags)
        {
            FramePoss.push_back({FramePos, SubFramePos});
        }

        friend bool operator==(const field_value& l, const field_value& r)
        {
            return l.Field==r.Field && l.Value==r.Value && l.Flags.to_int8u()==r.Flags.to_int8u();
        }
    };
    #endif

    //Bookmark
    struct bs_bookmark
    {
        int64u                      Element_Offset;
        int64u                      Element_Size;
        size_t                      Trusted;
        size_t                      NewSize;
        size_t                      End;
        int8u                       BitsNotIncluded;
        bool                        UnTrusted;
        #if MEDIAINFO_CONFORMANCE
            vector<field_value>     ConformanceErrors[ConformanceLevel_Max];
        #endif
    };
    bs_bookmark                     BS_Bookmark(size_t NewSize);
    #if MEDIAINFO_CONFORMANCE
    bool                            BS_Bookmark(bs_bookmark& B, const string& ConformanceFieldName);
    #else
    bool                            BS_Bookmark(bs_bookmark& B);
    inline bool                     BS_Bookmark(bs_bookmark& B, const string&) {return BS_Bookmark(B);}
    #endif

    //Fill
    void Fill_DRC(const char* Prefix=NULL);
    void Fill_Loudness(const char* Prefix=NULL, bool NoConCh=false);

    //Elements - USAC - Config
    void UsacConfig                         (size_t BitsNotIncluded=(size_t)-1);
    void UsacDecoderConfig                  ();
    void UsacSingleChannelElementConfig     ();
    void UsacChannelPairElementConfig       ();
    void UsacLfeElementConfig               ();
    void UsacExtElementConfig               ();
    void UsacCoreConfig                     ();
    void SbrConfig                          ();
    void SbrDlftHeader                      ();
    void Mps212Config                       (int8u StereoConfigindex);
    void uniDrcConfig                       ();
    void uniDrcConfigExtension              ();
    void downmixInstructions                (bool V1=false);
    void drcCoefficientsBasic               ();
    void drcCoefficientsUniDrc              (bool V1=false);
    void drcInstructionsBasic               ();
    bool drcInstructionsUniDrc              (bool V1=false, bool NoV0=false);
    void channelLayout                      ();
    void UsacConfigExtension                ();
    void fill_bytes                         (size_t usacConfigExtLength);
    void loudnessInfoSet                    (bool V1=false);
    bool loudnessInfo                       (bool FromAlbum, bool V1=false);
    void loudnessInfoSetExtension           ();
    void streamId                           ();

    //Elements - USAC - Frame
    #if MEDIAINFO_TRACE || MEDIAINFO_CONFORMANCE
    void UsacFrame                          (size_t BitsNotIncluded=(size_t)-1);
    void UsacSingleChannelElement           (bool usacIndependencyFlag);
    void UsacChannelPairElement             (bool usacIndependencyFlag);
    void twData                             ();
    void icsInfo                            ();
    void scaleFactorData                    (size_t ch);
    void arithData                          (size_t ch, int16u N, int16u lg, int16u lg_max, bool arith_reset_flag);
    void acSpectralData                     (size_t ch, bool usacIndependencyFlag);
    void tnsData                            ();
    void fdChannelStream                    (size_t ch, bool commonWindow, bool commonTw, bool tnsDataPresent, bool usacIndependencyFlag);
    void cplxPredData                       (int8u max_sfb_ste, bool usacIndependencyFlag);
    void StereoCoreToolInfo                 (bool& tns_data_present0, bool& tns_data_present1, bool core_mode0, bool core_mode1, bool usacIndependencyFlag);
    void UsacCoreCoderData                  (size_t nrChannels, bool usacIndependencyFlag);
    void sbrInfo                            ();
    void sbrHeader                          ();
    void sbrGrid                            (size_t chan);
    void sbrDtdf                            (size_t chan, bool usacIndependencyFlag);
    void sbrInvf                            (size_t chan);
    void pvcEnvelope                        (bool usacIndependencyFlag);
    void sbrEnvelope                        (bool ch, bool bs_coupling);
    void sbrNoise                           (bool ch, bool bs_coupling);
    void sbrSinusoidalCoding                (bool ch, int8u bs_pvc_mode);
    void sbrSingleChannelElement            (bool usacIndependencyFlag);
    void sbrChannelPairElement              (bool usacIndependencyFlag);
    void sbrData                            (size_t nrSbrChannels, bool usacIndependencyFlag);
    void UsacSbrData                        (size_t nrSbrChannels, bool usacIndependencyFlag);
    void FramingInfo                        ();
    void EcData                             (ec_data_type dataType, int8u paramIdx, int8u startBand, int8u stopBand, bool usacIndependencyFlag);
    void GroupedPcmData                     (ec_data_type dataType, bool pairFlag, int8u numQuantSteps, int8u dataBands);
    void HuffData1D                         (ec_data_type dataType, int8u diffType, int8u dataBands);
    void HuffData2DFreqPair                 (ec_data_type dataType, int8u diffType, int8u dataBands);
    void HuffData2DTimePair                 (ec_data_type dataType, int8u* aDiffType, int8u dataBands);
    void DiffHuffData                       (ec_data_type dataType, bool bsDataPairXXX, bool allowDiffTimeBackFlag, int8u dataBands);
    void LsbData                            (ec_data_type dataType, bool bsQuantCoarseXXX, int8u dataBands);
    void EcDataPair                         (ec_data_type dataType, int8u paramIdx, int8u setIdx, int8u dataBands, bool bsDataPairXXX, bool bsQuantCoarseXXX, bool usacIndependencyFlag);
    void SymmetryData                       (ec_data_type dataType, int8s (&aTmp)[2], int8u lav);
    void EnvelopeReshapeHuff                (bool (&bsTempShapeEnableChannel)[2]);
    void TempShapeData                      (bool& bsTsdEnable);
    void SmgData                            ();
    void TsdData                            ();
    void OttData                            (bool usacIndependencyFlag);
    void Mps212Data                         (bool usacIndependencyFlag);
    void UsacLfeElement                     (bool usacIndependencyFlag);
    void UsacExtElement                     (size_t elemIdx, bool usacIndependencyFlag);
    void AudioPreRoll                       ();
    #endif //MEDIAINFO_TRACE || MEDIAINFO_CONFORMANCE

    //Utils
    void escapedValue                       (int32u &Value, int8u nBits1, int8u nBits2, int8u nBits3, const char* Name);

    //***********************************************************************
    // Temp AAC
    //***********************************************************************

    int8u   channelConfiguration;
    int8u   sampling_frequency_index;
    int8u   extension_sampling_frequency_index;
    int8u   num_window_groups;
    int8u   num_windows;
    int8u   max_sfb;
    int8u   max_sfb1;

    //***********************************************************************
    // Conformance
    //***********************************************************************

    #if MEDIAINFO_CONFORMANCE
    enum conformance_flags
    {
        Usac,
        BaselineUsac,
        xHEAAC,
        MpegH,
        Conformance_Max
    };
    bitset8                         ConformanceFlags;
    vector<field_value>             ConformanceErrors_Total[ConformanceLevel_Max];
    vector<field_value>             ConformanceErrors[ConformanceLevel_Max];
    bool                            Warning_Error;
    profilelevel_struct             ProfileLevel;
    set<int8u>                      usacExtElementType_Present;
    const std::vector<int64u>*      Immediate_FramePos;
    const bool*                     Immediate_FramePos_IsPresent;
    const bool*                     IsCmaf;
    const std::vector<stts_struct>* outputFrameLength;
    const size_t*                   FirstOutputtedDecodedSample;
    const std::vector<sgpd_prol_struct>* roll_distance_Values;
    const std::vector<sbgp_struct>* roll_distance_FramePos;
    const bool*                     roll_distance_FramePos_IsPresent;
    bool CheckIf(const bitset8 Flags) { return !Flags || (ConformanceFlags & Flags); }
    void SetProfileLevel(int8u AudioProfileLevelIndication);
    void Fill_Conformance(const char* Field, const char* Value, bitset8 Flags={}, conformance_level Level=Error);
    void Fill_Conformance(const char* Field, const string Value, bitset8 Flags={}, conformance_level Level=Error) { Fill_Conformance(Field, Value.c_str(), Flags, Level); }
    void Fill_Conformance(const char* Field, const char* Value, conformance_flags Flag, conformance_level Level=Error) { Fill_Conformance(Field, Value, bitset8().set(Flag)); }
    void Fill_Conformance(const char* Field, const string Value, conformance_flags Flag, conformance_level Level=Error) { Fill_Conformance(Field, Value.c_str(), Flag, Level); }
    void Clear_Conformance();
    void Merge_Conformance(bool FromConfig=false);
    void Streams_Finish_Conformance();
    struct usac_config;
    struct usac_frame;
    void Streams_Finish_Conformance_Profile(usac_config& CurrentConf);
    void numPreRollFrames_Check(usac_config& CurrentConf, int32u numPreRollFrames, const string numPreRollFramesConchString);
    #else
    inline void Streams_Finish_Conformance() {}
    #endif

    //***********************************************************************
    // Others
    //***********************************************************************

    struct usac_element
    {
        int32u                      usacElementType;
        int32u                      usacExtElementDefaultLength;
        bool                        usacExtElementPayloadFrag;
    };
    struct downmix_instruction
    {
        int8u                       targetChannelCount;
    };
    typedef std::map<int8u, downmix_instruction> downmix_instructions;
    struct drc_id
    {
        int8u   drcSetId;
        int8u   downmixId;
        int8u   eqSetId;

        drc_id(int8u drcSetId, int8u downmixId=0, int8u eqSetId=0)
          : drcSetId(drcSetId),
            downmixId(downmixId),
            eqSetId(eqSetId)
        {}

        bool empty() const
        {
            return !drcSetId && !downmixId && !eqSetId;
        }

        string to_string() const
        {
            if (empty())
                return string();
            string Id=std::to_string(drcSetId);
            Id+='-';
            Id+= std::to_string(downmixId);
            //if (V1)
            //{
            //    Id+='-';
            //    Id+=std::to_string(eqSetId);
            //}
            return Id;
        }
        friend bool operator<(const drc_id& l, const drc_id& r)
        {
            //return memcmp(&l, &r, sizeof(drc_id));
            if (l.drcSetId<r.drcSetId)
                return true;
            if (l.drcSetId!=r.drcSetId)
                return false;
            if (l.downmixId<r.downmixId)
                return true;
            if (l.downmixId!=r.downmixId)
                return false;
            if (l.eqSetId<r.eqSetId)
                return true;
            return false;
        }
    };
    struct loudness_info
    {
        struct measurements
        {
            Ztring                  Values[16];
        };
        Ztring                      SamplePeakLevel;
        Ztring                      TruePeakLevel;
        measurements                Measurements;
    };
    typedef std::map<drc_id, loudness_info> loudness_infos;
    struct drc_info
    {
        string                      drcSetEffectTotal;
    };
    typedef std::map<drc_id, drc_info> drc_infos;
    struct gain_set
    {
        int8u                       bandCount;
    };

    struct arith_context
    {
        int16u                      previous_window_size;
        int8u                       q[2][512];

        arith_context() :           previous_window_size((int16u)-1)
        {
            memset(&q, 0, sizeof(q));
        }
    };

    struct usac_dlft_handler
    {
        int8u  dflt_start_freq;
        int8u  dflt_stop_freq;
        bool   dflt_header_extra1;
        int8u  dflt_freq_scale;
        bool   dflt_alter_scale;
        int8u  dflt_noise_bands;
    };

    struct mps212_handler
    {
        //Mps212Config
        bool  bsHighRatelMode;
        bool  bsPhaseCoding;
        int8u bsOttBandsPhase;
        int8u bsTempShapeConfig;
        //FramingInfo
        int8u numParamSets;
        //Computed
        int8u numSlots;
        int8u numBands;
    };

    typedef std::vector<gain_set> gain_sets;
    struct usac_config
    {
        vector<usac_element>        usacElements;
        downmix_instructions        downmixInstructions_Data;
        loudness_infos              loudnessInfo_Data[2]; // By non-album/album
        drc_infos                   drcInstructionsUniDrc_Data;
        gain_sets                   gainSets;
        #if MEDIAINFO_CONFORMANCE
        size_t                      loudnessInfoSet_Present[2];
        bool                        loudnessInfoSet_HasContent[2];
        vector<size_t>              numOutChannels_Lfe;
        #endif
        int32u                      numOutChannels;
        int32u                      sampling_frequency;
        int8u                       channelConfigurationIndex;
        int8u                       sampling_frequency_index;
        int8u                       coreSbrFrameLengthIndex;
        int8u                       baseChannelCount;
        int8u                       stereoConfigIndex;
        #if MEDIAINFO_CONFORMANCE
        int8u                       drcRequired_Present;
        bool                        LoudnessInfoIsNotValid;
        #endif
        bool                        WaitForNextIndependantFrame;
        bool                        harmonicSBR;
        bool                        bs_interTes;
        bool                        bs_pvc;
        bool                        noiseFilling;
        bool                        common_window;
        bool                        common_tw;
        bool                        tw_mdct;
        arith_context               arithContext[2];
        sbr_handler                 sbrHandler;
        usac_dlft_handler           dlftHandler;
        mps212_handler              mps212Handler;

        void Reset(bool V0=false, bool V1=false)
        {
            #if MEDIAINFO_CONFORMANCE
            loudnessInfoSet_HasContent[0]=V0;
            loudnessInfoSet_HasContent[1]=V1;
            loudnessInfoSet_Present[0]=V0;
            loudnessInfoSet_Present[1]=V1;
            #endif
        }
    };
    struct usac_frame
    {
        int32u                      numPreRollFrames;
    };
    usac_config                     Conf; //Main conf
    usac_config                     C; //Current conf
    usac_frame                      F; //Current frame
    int8u                           IsParsingRaw;
};

} //NameSpace

#endif


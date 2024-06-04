/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about DV-DIF (DV Digital Interface Format)
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_DvDifH
#define MediaInfo_File_DvDifH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/TimeCode.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_DvDif
//***************************************************************************

class File_DvDif : public File__Analyze
{
public :
    //In
    int64u Frame_Count_Valid;
    int8u  AuxToAnalyze; //Only Aux must be parsed
    bool   IgnoreAudio;

    //Constructor/Destructor
    File_DvDif();
    ~File_DvDif();

protected :
    //Streams management
    void Streams_Fill();
    void Streams_Finish();

    //Buffer - File header
    bool FileHeader_Begin();

    //Buffer - Synchro
    bool Synchronize();
    bool Synched_Test();
    void Synched_Test_Reset();
    void Synched_Init();

    //Buffer - Demux
    #if MEDIAINFO_DEMUX
    bool Demux_UnpacketizeContainer_Test();
    #endif //MEDIAINFO_DEMUX

    //Buffer - Global
    #ifdef MEDIAINFO_DVDIF_ANALYZE_YES
    void Read_Buffer_Init();
    void Read_Buffer_Continue();
    #endif //MEDIAINFO_DVDIF_ANALYZE_YES
    void Read_Buffer_Unsynched();
    #if MEDIAINFO_SEEK
    size_t Read_Buffer_Seek (size_t Method, int64u Value, int64u ID);
    #endif //MEDIAINFO_SEEK

    //Buffer
    void Header_Parse();
    void Data_Parse();

    //Elements - Main
    void Header();
    void Subcode();
    void Subcode_Ssyb(int8u syb_num);
    void VAUX();
    void Audio();
    void Video();

    //Elements - Sub
    void Element();
    void timecode();
    void binary_group();
    void audio_source();
    void audio_sourcecontrol();
    void audio_recdate();
    void audio_rectime();
    void video_source();
    void video_sourcecontrol();
    void video_recdate();
    void video_rectime();
    void closed_captions();
    void consumer_camera_1();
    void consumer_camera_2();

    //Helpers
    void recdate(bool FromVideo=false);
    void rectime(bool FromVideo=false);

    //Streams
    struct stream
    {
        std::map<std::string, Ztring> Infos;
    };
    std::vector<stream*> Streams_Audio;

    //Temp
    #if defined(MEDIAINFO_EIA608_YES)
        std::vector<File__Analyze*> CC_Parsers;
    #endif
    Ztring Recorded_Date_Date;
    Ztring Recorded_Date_Time;
    Ztring Encoded_Library_Settings;
    TimeCode TimeCode_FirstFrame;
    int64u FrameSize_Theory; //The size of a frame
    int8u  SCT;
    int8u  SCT_Old;
    int8u  Dseq;
    int8u  Dseq_Old;
    int8u  DBN;
    int8u  DBN_Olds[8];
    int8u  video_source_stype;
    int8u  audio_source_stype;
    bool   FSC;
    bool   FSP;
    bool   DSF;
    bool   DSF_IsValid;
    int8u  APT;
    bool   TF1;
    bool   TF2;
    bool   TF3;
    int8u  aspect;
    int8u  ssyb_AP3;
    bool   FieldOrder_FF;
    bool   FieldOrder_FS;
    bool   Interlaced;
    bool   system;
    bool   FSC_WasSet;
    bool   FSP_WasNotSet;
    bool   video_sourcecontrol_IsParsed;
    bool   audio_locked;

    #if MEDIAINFO_SEEK
        bool            Duration_Detected;
        int64u          TotalFrames;
    #endif //MEDIAINFO_SEEK

    #ifdef MEDIAINFO_DVDIF_ANALYZE_YES
    bool Analyze_Activated;

    void Errors_Stats_Update();
    void Errors_Stats_Update_Finnish();
    Ztring Errors_Stats_03;
    Ztring Errors_Stats_05;
    Ztring Errors_Stats_09;
    Ztring Errors_Stats_10;
    Ztring Date;
    Ztring Time;
    int64u Speed_FrameCount_StartOffset;
    int64u Speed_FrameCount;                            //Global    - Total
    int64u Speed_FrameCount_Video_STA_Errors;           //Global    - Error 1
    std::vector<int64u> Speed_FrameCount_Audio_Errors;  //Global    - Error 2
    int64u Speed_FrameCount_Timecode_Incoherency;       //Global    - Error 3
    int64u Speed_FrameCount_Contains_NULL;              //Global    - Error 4
    int64u Speed_Contains_NULL;                         //Per Frame - Error 4
    int64u Speed_FrameCount_Arb_Incoherency;            //Global    - Error 5
    int64u Speed_FrameCount_Stts_Fluctuation;           //Global    - Error 6
    int64u Speed_FrameCount_system[2];                  //Global    - Total per system (NTSC or PAL)
    struct abst_bf
    {
        struct value_trust
        {
            int32s Value;
            int32s Trust;

            value_trust()
            {
                Value=0;
                Trust=0;
            }

            bool operator < (const value_trust& b) const
            {
                if (Trust==b.Trust)
                    return Value<b.Value;
                return Trust>b.Trust;
            }
        };
        vector<value_trust> abst[2]; //0=standard, 1=non-standard x1.5
        set<int32s> StoredValues;
        size_t bf[2];
        size_t Frames_NonStandard[2]; //0=x0.5, 1=x1.5

        abst_bf()
        {
            Frames_NonStandard[0]=0;
            Frames_NonStandard[1]=0;
            reset();
        }

        void reset()
        {
            abst[0].clear();
            abst[1].clear();
            bf[0]=0;
            bf[1]=0;
            StoredValues.clear();
        }
    };
    int16u FSC_WasSet_Sum;
    int16u FSC_WasNotSet_Sum;
    abst_bf AbstBf_Current_Weighted;
    int32u AbstBf_Current;
    int32u AbstBf_Previous;
    int32s AbstBf_Previous_MaxAbst;
    int8u  SMP;
    int8u  QU;
    bool   QU_FSC; //Validity is with QU
    bool   QU_System; //Validity is with QU
    bool   REC_ST;
    bool   REC_END;
    bool   REC_IsValid;
    std::vector<int8u> DirectionSpeed;
    struct dvdate
    {
        int8u  Days;
        int8u  Months;
        int8u  Years;
        bool   MultipleValues;
        bool   IsValid;

        dvdate() {Clear();}

        void Clear()
        {
            MultipleValues=false;
            IsValid=false;
        }
    };
    struct dvtime
    {
        struct time
        {
            int8u  Frames;
            int8u  Seconds;
            int8u  Minutes;
            int8u  Hours;
            bool   DropFrame;

            time()
            {
                Frames=(int8u)-1;
                Seconds=(int8u)-1;
                Minutes=(int8u)-1;
                Hours=(int8u)-1;
                DropFrame=false;
            }
        };
        time    Time;
        bool    MultipleValues;
        bool    IsValid;

        dvtime() {Clear();}

        void Clear()
        {
            MultipleValues=false;
            IsValid=false;
        }
    };
    dvtime Speed_TimeCode_Last;
    dvtime Speed_TimeCode_Current;
    dvtime Speed_TimeCode_Current_Theory;
    dvtime Speed_TimeCode_Current_Theory2;
    Ztring Speed_TimeCodeZ_First;
    Ztring Speed_TimeCodeZ_Last;
    Ztring Speed_TimeCodeZ_Current;
    bool   Speed_TimeCode_IsValid;
    dvtime Speed_RecTime_Current;
    dvtime Speed_RecTime_Current_Theory;
    dvtime Speed_RecTime_Current_Theory2;
    Ztring Speed_RecTimeZ_First;
    Ztring Speed_RecTimeZ_Last;
    Ztring Speed_RecTimeZ_Current;
    dvdate Speed_RecDate_Current;
    dvdate Speed_RecDate_Current_Theory2;
    Ztring Speed_RecDateZ_First;
    Ztring Speed_RecDateZ_Last;
    Ztring Speed_RecDateZ_Current;
    std::vector<size_t> Video_STA_Errors; //Per STA type
    std::vector<size_t> Video_STA_Errors_ByDseq; //Per Dseq & STA type
    std::vector<size_t> Video_STA_Errors_Total; //Per STA type
    static const size_t ChannelGroup_Count=2;
    static const size_t Dseq_Count=16;
    enum caption
    {
        Caption_Present,
        Caption_ParityIssueAny,
        PreviousFrameHasNoAudioSourceControl,
    };
    bitset<32> Captions_Flags;
    enum coherency
    {
        Coherency_PackInSub,
        Coherency_PackInVid,
        Coherency_PackInAud,
        Coherency_video_source,
        Coherency_video_control,
        Coherency_audio_source,
        Coherency_audio_control,
    };
    bitset<32> Coherency_Flags;
    std::vector<std::vector<int8u> > audio_source_mode; //Per ChannelGroup and Dseq, -1 means not present
    bitset<ChannelGroup_Count*2> ChannelInfo;
    struct audio_errors
    {
        size_t Count;
        std::set<int16u> Values;

        audio_errors():
            Count(0)
        {}
    };
    std::vector<std::vector<audio_errors> > Audio_Errors; //Per ChannelGroup and Dseq
    std::vector<std::vector<size_t> > Audio_Errors_TotalPerChannel; //Per Channel and Dseq

    struct recZ_Single
    {
        int64u FramePos;
        Ztring Date;
        Ztring Time;

        recZ_Single()
        {
            FramePos=(int64u)-1;
        }
    };
    struct recZ
    {
        recZ_Single First;
        recZ_Single Last;
    };
    std::vector<recZ> Speed_RecZ;
    struct timeCodeZ_Single
    {
        int64u FramePos;
        Ztring TimeCode;

        timeCodeZ_Single()
        {
            FramePos=(int64u)-1;
        }
    };
    struct timeCodeZ
    {
        timeCodeZ_Single First;
        timeCodeZ_Single Last;
    };
    std::vector<timeCodeZ> Speed_TimeCodeZ;
    struct timeStampsZ_Single
    {
        int64u FramePos;
        Ztring Time;
        Ztring TimeCode;
        Ztring Date;

        timeStampsZ_Single()
        {
            FramePos=(int64u)-1;
        }
    };
    struct timeStampsZ
    {
        timeStampsZ_Single First;
        timeStampsZ_Single Last;
    };
    std::vector<timeStampsZ> Speed_TimeStampsZ;

    enum status
    {
        BlockStatus_Unk,
        BlockStatus_OK,
        BlockStatus_NOK,
    };
    static const size_t BlockStatus_MaxSize=1800*4; // Max 1800 if PAL * 4 if DV100
    MediaInfo_int8u BlockStatus[BlockStatus_MaxSize];

    struct arb
    {
        std::vector<size_t> Value_Counters;
        int8u  Value;
        bool   MultipleValues;
        bool   IsValid;

        arb() {Clear();}

        void Clear()
        {
            Value_Counters.clear();
            Value_Counters.resize(16);
            Value=0xF; //Used only when we are sure
            MultipleValues=false;
            IsValid=false;
        }
    };
    arb Speed_Arb_Last;
    arb Speed_Arb_Current;
    arb Speed_Arb_Current_Theory;
    bool   Speed_Arb_IsValid;

    //Stats
    std::vector<size_t> Stats;
    size_t              Stats_Total;
    size_t              Stats_Total_WithoutArb;
    bool                Stats_Total_AlreadyDetected;

public:
    //From MPEG-4 container
    struct stts_part
    {
        int64u Pos_Begin;
        int64u Pos_End;
        int32u Duration;
    };
    typedef std::vector<stts_part> stts;
    stts* Mpeg4_stts;
    size_t Mpeg4_stts_Pos;
    #endif //MEDIAINFO_DVDIF_ANALYZE_YES
};

} //NameSpace

#endif

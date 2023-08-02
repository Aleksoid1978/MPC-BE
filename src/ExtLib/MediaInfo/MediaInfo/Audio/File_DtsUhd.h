/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_DtsUhdH
#define MediaInfo_DtsUhdH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Dts.h" // Should be File__Analyze.h but we need DTSHDHDR parsing
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_DtsUhd
//***************************************************************************

extern const int32u FrequencyCodeTable[2];
extern const char* RepresentationTypeTable[8];

struct DTSUHD_ChannelMaskInfo
{
    int32u ChannelCount=0;
    int32u CountFront=0, CountSide=0, CountRear=0, CountLFE=0, CountHeights=0, CountLows=0;
    std::string ChannelLayoutText, ChannelPositionsText, ChannelPositions2Text;
};

struct DTSUHD_ChannelMaskInfo DTSUHD_DecodeChannelMask(int32u ChannelMask);

class File_DtsUhd : public File_Dts_Common
{
public :
    //Constructor/Destructor
    File_DtsUhd();

protected :
    //Streams management
    void Streams_Fill();

    //Buffer - Synchro
    bool Synchronize();
    bool Synched_Test();
    void Read_Buffer_Unsynched();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    bool FrameSynchPoint_Test(bool AcceptNonSync);

    struct MDObject
    {
        bool Started=false;  /* Object seen since last reset. */
        int PresIndex=0;
        int8u RepType=0;
        int32u ChActivityMask=0;
    };

    struct MD01
    {
        MDObject Object[257]; /* object id max value is 256 */
        bool StaticMDParamsExtracted=false;
        bool StaticMetadataUpdtFlag=false;
        int ChunkId=0;
        int16u ObjectList[256]={0};
        int32u NumObjects=0; /* Valid entries in 'ObjectList' array */
        int32u NumStaticMDPackets=0;
        int PacketsAcquired=0;
        int32u StaticMDPacketByteSize=0;
        std::vector<int8u> Buffer;
    };

    struct audio_chunk
    {
        bool Present=false;
        int32u AudioChunkSize=0;
        int32u AudioChunkID=0;
        int32u Index=0;
    };

    struct UHDAudPresParam
    {
        bool Selectable=false;
        int DepAuPresMask=0;
    };

    struct md_chunk
    {
        bool MDChunkCRCFlag=false;
        int32u MDChunkSize=0;
    };

    struct UHDFrameDescriptor
    {
        int BaseSampleFreqCode;
        int ChannelCount;
        int ChannelMask;
        int DecoderProfileCode;
        int MaxPayloadCode;
        int NumPresCode;
        int SampleCount;
        int RepType;
    };

    int32u ReadBitsMD01(MD01* MD01, int Bits, const char* Name =nullptr);
    MD01* ChunkAppendMD01(int Id);
    MD01* ChunkFindMD01(int Id);
    MDObject* FindDefaultAudio();
    void ExtractObjectInfo(MDObject*);
    void UpdateDescriptor();
    int ExtractExplicitObjectsLists(int Mask, int Index);
    int ResolveAudPresParams();
    void DecodeVersion();
    int ExtractStreamParams();
    void NaviPurge();
    int NaviFindIndex(int DesiredIndex, int32u* ListIndex);
    int ExtractChunkNaviData();
    int ExtractMDChunkObjIDList(MD01*);
    void ExtractLTLMParamSet(MD01*, bool NominalLD_DescriptionFlag);
    int ParseStaticMDParams(MD01*, bool OnlyFirst);
    int ExtractMultiFrameDistribStaticMD(MD01*);
    bool CheckIfMDIsSuitableforImplObjRenderer(MD01*, int ObjectId);
    void ExtractChMaskParams(MD01*, MDObject*);
    int ExtractObjectMetadata(MD01*, MDObject*, bool, int);
    int ParseMD01(MD01*, int PresIndex);
    int UnpackMDFrame();
    void UnpackMDFrame_1(int8u MDChunkID);
    bool CheckCurrentFrame();
    int Frame();

    UHDAudPresParam AudPresParam[256];
    UHDFrameDescriptor FrameDescriptor;
    bool FullChannelBasedMixFlag;
    bool InteractObjLimitsPresent;
    bool HasLoudness;
    bool SyncFrameFlag;
    int32u ChunkBytes;
    int ClockRateInHz;
    int32u FTOCPayloadinBytes;
    int FrameDuration;
    int8u LongTermLoudnessIndex;
    int8u StreamMajorVerNum;
    int32u NumAudioPres;
    int SampleRate;
    int32u FrameSize;
    std::vector<MD01> MD01List;
    std::vector<audio_chunk> Audio_Chunks;
    std::vector<md_chunk> MD_Chunks;

    //Helpers
    void Get_VR(const uint8_t Table[], int32u& Info, const char* Name);
    void Skip_VR(const uint8_t Table[], const char* Name) { int32u Info; Get_VR(Table, Info, Name); }
};

} //NameSpace

#endif

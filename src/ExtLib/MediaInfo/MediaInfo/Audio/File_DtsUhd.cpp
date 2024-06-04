/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

// Initial code provided by Xperi Inc. in 2023
// via a Contributor License Agreement

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
#if defined(MEDIAINFO_DTSUHD_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_DtsUhd.h"
#include "ZenLib/Utils.h"
#include "ZenLib/BitStream.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#if MEDIAINFO_EVENTS
    #include "MediaInfo/MediaInfo_Events.h"
#endif //MEDIAINFO_EVENTS
#include <algorithm>
using namespace ZenLib;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

static constexpr int32u DTSUHD_NONSYNCWORD=0x71C442E8;
static constexpr int32u DTSUHD_SYNCWORD=0x40411BF2;

/* Return codes from dtsuhd_frame */
enum DTSUHDStatus {
    DTSUHD_OK,
    DTSUHD_INCOMPLETE,    /* Entire frame not in buffer. */
    DTSUHD_INVALID_FRAME, /* Error parsing frame. */
    DTSUHD_NOSYNC,        /* No sync frame prior to non-sync frame. */
    DTSUHD_NULL,          /* Function parameter may not be NULL. */
};

enum RepType {
    REP_TYPE_CH_MASK_BASED,
    REP_TYPE_MTRX2D_CH_MASK_BASED,
    REP_TYPE_MTRX3D_CH_MASK_BASED,
    REP_TYPE_BINAURAL,
    REP_TYPE_AMBISONIC,
    REP_TYPE_AUDIO_TRACKS,
    REP_TYPE_3D_OBJECT_SINGLE_SRC_PER_WF,
    REP_TYPE_3D_MONO_OBJECT_SINGLE_SRC_PER_WF,
};

// Section 5.2.2.2: Long Term Loudness Measure Table
static constexpr float LongTermLoudnessMeasure[] =
{
    -40.00, -39.00, -38.00, -37.00, -36.00, -35.00, -34.00, -33.00,
    -32.00, -31.00, -30.00, -29.50, -29.00, -28.50, -28.00, -27.50,
    -27.00, -26.75, -26.50, -26.25, -26.00, -25.75, -25.50, -25.25,
    -25.00, -24.75, -24.50, -24.25, -24.00, -23.75, -23.50, -23.25,
    -23.00, -22.75, -22.50, -22.25, -22.00, -21.75, -21.50, -21.25,
    -21.00, -20.50, -20.00, -19.50, -19.00, -18.00, -17.00, -16.00,
    -15.00, -14.00, -13.00, -12.00, -11.00, -10.00,  -9.00,  -8.00,
     -7.00,  -6.00,  -5.00,  -4.00,  -3.00,  -2.00,  -1.00,   0.00,
};

static int CountBits(int32u Mask)
{
    int Count=0;
    for (; Mask; Mask>>=1)
        Count+=Mask&1;
    return Count;
}

/** Read a variable number of bits from a buffer.
    In the ETSI TS 103 491 V1.2.1 specification, the pseudo code defaults
    the 'add' parameter to true.  Table 7-30 shows passing an explicit false,
    most other calls do not pass the extractAndAdd parameter.  This function
    is based on code in Table 5-2

*/
void File_DtsUhd::Get_VR(const uint8_t Table[], int32u& Value, const char* Name)
{
    Element_Begin1(Name?Name:"?");
    static const uint8_t BitsUsed[8] = {1, 1, 1, 1, 2, 2, 3, 3};
    static const uint8_t IndexTable[8] = { 0, 0, 0, 0, 1, 1, 2, 3 };

    uint8_t Code;
    Peek_S1(3, Code);
    Skip_S1(BitsUsed[Code],                                     "index (partial)");
    auto Index=IndexTable[Code];
    Value=0;
    if (Table[Index]>0)
    {
        for (int i=0; i<Index; i++)
            Value+=1<<Table[i];
        uint32_t Add;
        Get_S4 (Table[Index], Add,                              "addition");
        Value+=Add;
    }

    Element_Info1(Value);
    Element_End0();
}

/* Implied by Table 6-2, MD01 chunk objects appended in for loop */
File_DtsUhd::MD01* File_DtsUhd::ChunkAppendMD01(int Id)
{
    MD01List.push_back(MD01());
    MD01List.back().ChunkId=Id;
    return &MD01List.back();
}

/* Return existing MD01 chunk based on chunkID */
File_DtsUhd::MD01* File_DtsUhd::ChunkFindMD01(int Id)
{
    for (auto& MD01 : MD01List)
        if (Id==MD01.ChunkId)
            return &MD01;
    return nullptr;
}

File_DtsUhd::MDObject* File_DtsUhd::FindDefaultAudio()
{
    for (auto& MD01 : MD01List)
    {
        int ObjIndex=-1;
        for (int i=0; i<257; i++)
        {
            MDObject* Object = MD01.Object+i;
            if (Object->Started && AudPresParam[Object->PresIndex].Selectable)
            {
                if (ObjIndex < 0 || (Object->PresIndex < MD01.Object[ObjIndex].PresIndex))
                    ObjIndex = i;
            }
        }
        if (ObjIndex>=0)
            return MD01.Object+ObjIndex;
    }

    return nullptr;
}

/* Save channel mask, count, and rep type to descriptor info.
   ETSI TS 103 491 Table 7-28 channel activity mask bits
   mapping and SCTE DVS 243-4 Rev. 0.2 DG X Table 4.  Convert activity mask and
   representation type to channel mask and channel counts.
*/
void File_DtsUhd::ExtractObjectInfo(MDObject* Object)
{
    if (!Object)
        return;

    constexpr struct
    {
        int32u ActivityMask, ChannelMask; // ChannelMask is defined by ETSI TS 103 491
    }
    ActivityMap[] = {
        // act mask | chan mask | ffmpeg channel mask
        { 0x000001, 0x00000001 },
        { 0x000002, 0x00000006 },
        { 0x000004, 0x00000018 },
        { 0x000008, 0x00000020 },
        { 0x000010, 0x00000040 },
        { 0x000020, 0x0000A000 },
        { 0x000040, 0x00000180 },
        { 0x000080, 0x00004000 },
        { 0x000100, 0x00080000 },
        { 0x000200, 0x00001800 },
        { 0x000400, 0x00060000 },
        { 0x000800, 0x00000600 },
        { 0x001000, 0x00010000 },
        { 0x002000, 0x00300000 },
        { 0x004000, 0x00400000 },
        { 0x008000, 0x01800000 },
        { 0x010000, 0x02000000 },
        { 0x020000, 0x0C000000 },
        { 0x140000, 0x30000000 },
        { 0x080000, 0xC0000000 },
        { 0 } // Terminator
    };

    for (int i = 0; ActivityMap[i].ActivityMask; i++)
        if (ActivityMap[i].ActivityMask & Object->ChActivityMask)
            FrameDescriptor.ChannelMask |= ActivityMap[i].ChannelMask;

    FrameDescriptor.ChannelCount = CountBits(FrameDescriptor.ChannelMask);
    FrameDescriptor.RepType = Object->RepType;
}

/* Assemble information for MP4 Sample Entry box.  Sample Size is always
   16 bits.  The coding name is the name of the SampleEntry sub-box and is
   'dtsx' unless the version of the bitstream is > 2.
   If DecoderProfile == 2, then MaxPayloadCode will be zero.
*/
void File_DtsUhd::UpdateDescriptor()
{
    FrameDescriptor.ChannelMask=0;
    FrameDescriptor.RepType=0;
    ExtractObjectInfo(FindDefaultAudio());

    /* 6.3.6.9: audio frame duration may be a fraction of metadata frame duration. */
    int Fraction=1;
    for (const auto& Audio_Chunk : Audio_Chunks)
    {
        if (Audio_Chunk.Present)
        {
            if (Audio_Chunk.AudioChunkID==3)
                Fraction=2;
            else if (Audio_Chunk.AudioChunkID==4)
                Fraction=4;
        }
    }

    FrameDescriptor.BaseSampleFreqCode=SampleRate==48000;
    FrameDescriptor.ChannelCount=CountBits(FrameDescriptor.ChannelMask);
    FrameDescriptor.DecoderProfileCode=StreamMajorVerNum-2;
    FrameDescriptor.MaxPayloadCode=0+(StreamMajorVerNum>=2);
    FrameDescriptor.NumPresCode=NumAudioPres-1;
    FrameDescriptor.SampleCount=(FrameDuration*SampleRate)/(ClockRateInHz*Fraction);
}

/* Table 6-17 p47 */
int File_DtsUhd::ExtractExplicitObjectsLists(int DepAuPresExplObjListMask, int CurrentAuPresInd)
{
    Element_Begin1("ExtractExplicitObjectsLists");
    constexpr int8u Table[4] = {4, 8, 16, 32};

    for (int i=0; i<CurrentAuPresInd; i++)
        if ((DepAuPresExplObjListMask>>i)&1)
        {
            bool UpdFlag=SyncFrameFlag;
            if (!UpdFlag)
                Get_SB (UpdFlag,                                "UpdFlag");

            if (UpdFlag)
                Skip_VR(Table,                                  "ExplObjListMasks");
        }

    Element_End0();
    return 0;
}

/* Table 6-15 p44, Table 6-16 p45 */
int File_DtsUhd::ResolveAudPresParams()
{
    Element_Begin1("ResolveAudPresParams");
    constexpr uint8_t Table[4] = {0, 2, 4, 5};

    if (SyncFrameFlag)
    {
        if (FullChannelBasedMixFlag)
            NumAudioPres=0;
        else
            Get_VR (Table, NumAudioPres,                        "NumAudioPres");
        NumAudioPres++;
        memset(AudPresParam, 0, sizeof(AudPresParam[0])*NumAudioPres);
    }

    for (int AuPresInd=0; AuPresInd<NumAudioPres; AuPresInd++)
    {
        Element_Begin1("AudPres");
        if (SyncFrameFlag)
        {
            if (FullChannelBasedMixFlag)
                AudPresParam[AuPresInd].Selectable=true;
            else
                Get_SB (AudPresParam[AuPresInd].Selectable,     "AudPresSelectableFlag");
        }

        if (AudPresParam[AuPresInd].Selectable)
        {
            if (SyncFrameFlag)
            {
                int32u DepAuPresMask;
                Get_S4 (AuPresInd, DepAuPresMask,               "DepAuPresMask");
                AudPresParam[AuPresInd].DepAuPresMask=0;
                for (int i = 0; DepAuPresMask; i++, DepAuPresMask>>=1)
                    if (DepAuPresMask&1)
                    {
                        bool DepAuPresMask;
                        Get_SB (DepAuPresMask,                  "DepAuPresExplObjListPresMask");
                        AudPresParam[AuPresInd].DepAuPresMask|=DepAuPresMask<<i;
                    }
            }

            if (AuPresInd && ExtractExplicitObjectsLists(AudPresParam[AuPresInd].DepAuPresMask, AuPresInd))
                return 1;
        }
        else
            AudPresParam[AuPresInd].DepAuPresMask=0;
        Element_End0();
    }

    Element_End0();
    return 0;
}

//---------------------------------------------------------------------------
static const int16u CRC_CCIT_Table[256]=
{
    0x0000, 0x2110, 0x4220, 0x6330, 0x8440, 0xA550, 0xC660, 0xE770,
    0x0881, 0x2991, 0x4AA1, 0x6BB1, 0x8CC1, 0xADD1, 0xCEE1, 0xEFF1,
    0x3112, 0x1002, 0x7332, 0x5222, 0xB552, 0x9442, 0xF772, 0xD662,
    0x3993, 0x1883, 0x7BB3, 0x5AA3, 0xBDD3, 0x9CC3, 0xFFF3, 0xDEE3,
    0x6224, 0x4334, 0x2004, 0x0114, 0xE664, 0xC774, 0xA444, 0x8554,
    0x6AA5, 0x4BB5, 0x2885, 0x0995, 0xEEE5, 0xCFF5, 0xACC5, 0x8DD5,
    0x5336, 0x7226, 0x1116, 0x3006, 0xD776, 0xF666, 0x9556, 0xB446,
    0x5BB7, 0x7AA7, 0x1997, 0x3887, 0xDFF7, 0xFEE7, 0x9DD7, 0xBCC7,
    0xC448, 0xE558, 0x8668, 0xA778, 0x4008, 0x6118, 0x0228, 0x2338,
    0xCCC9, 0xEDD9, 0x8EE9, 0xAFF9, 0x4889, 0x6999, 0x0AA9, 0x2BB9,
    0xF55A, 0xD44A, 0xB77A, 0x966A, 0x711A, 0x500A, 0x333A, 0x122A,
    0xFDDB, 0xDCCB, 0xBFFB, 0x9EEB, 0x799B, 0x588B, 0x3BBB, 0x1AAB,
    0xA66C, 0x877C, 0xE44C, 0xC55C, 0x222C, 0x033C, 0x600C, 0x411C,
    0xAEED, 0x8FFD, 0xECCD, 0xCDDD, 0x2AAD, 0x0BBD, 0x688D, 0x499D,
    0x977E, 0xB66E, 0xD55E, 0xF44E, 0x133E, 0x322E, 0x511E, 0x700E,
    0x9FFF, 0xBEEF, 0xDDDF, 0xFCCF, 0x1BBF, 0x3AAF, 0x599F, 0x788F,
    0x8891, 0xA981, 0xCAB1, 0xEBA1, 0x0CD1, 0x2DC1, 0x4EF1, 0x6FE1,
    0x8010, 0xA100, 0xC230, 0xE320, 0x0450, 0x2540, 0x4670, 0x6760,
    0xB983, 0x9893, 0xFBA3, 0xDAB3, 0x3DC3, 0x1CD3, 0x7FE3, 0x5EF3,
    0xB102, 0x9012, 0xF322, 0xD232, 0x3542, 0x1452, 0x7762, 0x5672,
    0xEAB5, 0xCBA5, 0xA895, 0x8985, 0x6EF5, 0x4FE5, 0x2CD5, 0x0DC5,
    0xE234, 0xC324, 0xA014, 0x8104, 0x6674, 0x4764, 0x2454, 0x0544,
    0xDBA7, 0xFAB7, 0x9987, 0xB897, 0x5FE7, 0x7EF7, 0x1DC7, 0x3CD7,
    0xD326, 0xF236, 0x9106, 0xB016, 0x5766, 0x7676, 0x1546, 0x3456,
    0x4CD9, 0x6DC9, 0x0EF9, 0x2FE9, 0xC899, 0xE989, 0x8AB9, 0xABA9,
    0x4458, 0x6548, 0x0678, 0x2768, 0xC018, 0xE108, 0x8238, 0xA328,
    0x7DCB, 0x5CDB, 0x3FEB, 0x1EFB, 0xF98B, 0xD89B, 0xBBAB, 0x9ABB,
    0x754A, 0x545A, 0x376A, 0x167A, 0xF10A, 0xD01A, 0xB32A, 0x923A,
    0x2EFD, 0x0FED, 0x6CDD, 0x4DCD, 0xAABD, 0x8BAD, 0xE89D, 0xC98D,
    0x267C, 0x076C, 0x645C, 0x454C, 0xA23C, 0x832C, 0xE01C, 0xC10C,
    0x1FEF, 0x3EFF, 0x5DCF, 0x7CDF, 0x9BAF, 0xBABF, 0xD98F, 0xF89F,
    0x176E, 0x367E, 0x554E, 0x745E, 0x932E, 0xB23E, 0xD10E, 0xF01E,
};
static int16u CheckCRC(const int8u* Buffer, int Size)
{
    int16u C=0xFFFF;
    const uint8_t *End=Buffer+Size;

    while (Buffer<End)
        C = (C>>8)^CRC_CCIT_Table[((int8u)C)^*Buffer++];

    return C;
}

/* Table 6-12 p 53 */
void File_DtsUhd::DecodeVersion()
{
    bool ShortRevNum;
    Get_SB (ShortRevNum,                                        "ShortRevNum");
    int8u Bits=6>>(unsigned)ShortRevNum;
    Get_S1 (Bits, StreamMajorVerNum,                            "StreamMajorVerNum");
    StreamMajorVerNum+=2;
    Skip_S1(Bits,                                               "StreamMinorRevNum");
}

/* Table 6-12 p 53 */
int File_DtsUhd::ExtractStreamParams()
{
    Element_Begin1("ExtractStreamParams");
    static constexpr uint16_t TableBaseDuration[4] = {512, 480, 384, 0};
    static constexpr uint16_t TableClockRate[4] = {32000, 44100, 48000, 0};

    if (SyncFrameFlag)
        Get_SB (FullChannelBasedMixFlag,                        "FullChannelMixFlag");

    if (SyncFrameFlag || !FullChannelBasedMixFlag)
        if (CheckCRC(Buffer+Buffer_Offset, FTOCPayloadinBytes))
            return 1;

    if (SyncFrameFlag)
    {
        if (FullChannelBasedMixFlag)
            StreamMajorVerNum=2;
        else
            DecodeVersion();

        int8u BaseDuration, FrameDurationCode, Temp, SampleRateMod;
        bool TimeStampPresent;
        Get_S1 (2, BaseDuration,                                "BaseDuration");
        FrameDuration=TableBaseDuration[BaseDuration];
        Get_S1 (3, FrameDurationCode,                           "FrameDurationCode");
        FrameDuration*=FrameDurationCode+1;
        Param_Info2(FrameDuration, " samples");
        Get_S1 (2, Temp,                                        "ClockRateInHz");
        ClockRateInHz=TableClockRate[Temp];
        Param_Info2(ClockRateInHz, " Hz");
        if (FrameDuration==0 || ClockRateInHz==0)
            return 1; /* bitstream error */
        Get_SB (TimeStampPresent,                               "TimeStampPresent");
        if (TimeStampPresent)
            Skip_BS(36,                                         "TimeStamp");
        Get_S1 (2, SampleRateMod,                               "SampleRateMod");
        SampleRate=ClockRateInHz*(1<<SampleRateMod);
        Param_Info2(SampleRate, " Hz");
        if (FullChannelBasedMixFlag)
        {
            InteractObjLimitsPresent=false;
        }
        else
        {
            Skip_SB(                                            "Reserved");
            Get_SB (InteractObjLimitsPresent,                   "InteractObjLimitsPresent");
        }
    }
    Element_End0();
    return 0;
}

/* Table 6-24 p52 */
void File_DtsUhd::NaviPurge()
{
    for (auto& Navi : Audio_Chunks)
        if (!Navi.Present)
            Navi.AudioChunkSize=0;
}

/* Table 6-23 p51.  Return 0 on success, and the index is returned in
   the *listIndex parameter.
*/
int File_DtsUhd::NaviFindIndex(int DesiredIndex, int32u* ListIndex)
{
    for (auto& Navi : Audio_Chunks)
    {
        if (Navi.Index==DesiredIndex)
        {
            Navi.Present=true;
            *ListIndex=Navi.Index;
            return 0;
        }
    }

    int Index=0;
    for (auto& Navi : Audio_Chunks)
    {
        if (Navi.Present&&Navi.AudioChunkSize==0)
            break;
        Index++;
    }

    if (Index>=Audio_Chunks.size())
        Audio_Chunks.push_back(audio_chunk());

    auto& Navi=Audio_Chunks[Index];
    Navi.AudioChunkSize=0;
    Navi.Present=true;
    Navi.AudioChunkID=256;
    Navi.Index=Index;
    *ListIndex=Index;

    return 0;
}

/* Table 6-20 p48 */
int File_DtsUhd::ExtractChunkNaviData()
{
    Element_Begin1("ExtractChunkNaviData");
    constexpr int8u Table2468[4] = {2, 4, 6, 8};
    constexpr int8u TableAudioChunkSizes[4] = {9, 11, 13, 16};
    constexpr int8u TableChunkSizes[4] = {6, 9, 12, 15};

    ChunkBytes = 0;
    if (FullChannelBasedMixFlag)
        MD_Chunks.resize(SyncFrameFlag);
    else
    {
        int32u Num_MD_Chunks;
        Get_VR(Table2468, Num_MD_Chunks,                        "Num_MD_Chunks");
        MD_Chunks.resize(Num_MD_Chunks);
    }

    for (auto& MD_Chunk : MD_Chunks)
    {
        Get_VR(TableChunkSizes, MD_Chunk.MDChunkSize,           "MDChunkSize");
        ChunkBytes+=MD_Chunk.MDChunkSize;
        if (FullChannelBasedMixFlag)
            MD_Chunk.MDChunkCRCFlag=false;
        else
            Get_SB (MD_Chunk.MDChunkCRCFlag,                    "MDChunkCRCFlag");
    }

    int32u Num_Audio_Chunks;
    if (FullChannelBasedMixFlag)
        Num_Audio_Chunks=1;
    else
        Get_VR (Table2468, Num_Audio_Chunks,                    "Num_Audio_Chunks");

    if (!SyncFrameFlag)
    {
        for (auto& Navi : Audio_Chunks)
            Navi.Present=false;
    }
    else
        Audio_Chunks.clear();

    for (int32u i=0; i<Num_Audio_Chunks; i++)
    {
        int32u AudioChunkIndex=0;
        if (FullChannelBasedMixFlag)
            AudioChunkIndex=0;
        else
            Get_VR (Table2468, AudioChunkIndex,                 "AudioChunkIndex");

        if (NaviFindIndex(AudioChunkIndex, &AudioChunkIndex))
            return 1;

        bool ACIDsPresentFlag;
        if (SyncFrameFlag)
            ACIDsPresentFlag=true;
        else if (FullChannelBasedMixFlag)
            ACIDsPresentFlag=false;
        else
            Get_SB (ACIDsPresentFlag,                           "ACIDsPresentFlag");
        if (ACIDsPresentFlag)
            Get_VR (Table2468, Audio_Chunks[AudioChunkIndex].AudioChunkID, "AudioChunkID");
        Get_VR(TableAudioChunkSizes, Audio_Chunks[AudioChunkIndex].AudioChunkSize, "AudioChunkSize");
        ChunkBytes+=Audio_Chunks[AudioChunkIndex].AudioChunkSize;
    }

    NaviPurge();

    Element_End0();
    return 0;
}


/* Table 6-6 */
int File_DtsUhd::ExtractMDChunkObjIDList(MD01* MD01)
{
    Element_Begin1("ExtractMDChunkObjIDList");
    if (FullChannelBasedMixFlag)
    {
        MD01->NumObjects=1;
        MD01->ObjectList[0]=256;
    }
    else
    {
        constexpr int8u Table[4] = {3, 4, 6, 8};
        Get_VR (Table, MD01->NumObjects,                        "NumObjects");
        for (int i=0; i<MD01->NumObjects; i++)
        {
            bool NumBitsforObjID_b;
            Get_SB (NumBitsforObjID_b,                          "NumBitsforObjID");
            int8u NumBitsforObjID=4<<((unsigned)NumBitsforObjID_b);
            Get_S2 (NumBitsforObjID, MD01->ObjectList[i],       "ObjectIDList");
        }
    }
    Element_End0();

    return 0;
}


/* Table 7-9 */
void File_DtsUhd::ExtractLTLMParamSet(MD01* MD01, bool NominalLD_DescriptionFlag)
{
    Element_Begin1("ExtractLTLMParamSet");
    Get_S1 (6, LongTermLoudnessIndex,                           "LongTermLoudnessMeasureIndex"); /* rLoudness */
    Param_Info2(LongTermLoudnessMeasure[LongTermLoudnessIndex], " LKFS");

    if (!NominalLD_DescriptionFlag)
        Skip_S1(5,                                              "AssociatedAssetType");

    Skip_S1(NominalLD_DescriptionFlag?2:4,                      "BitWidth");
    Element_End0();
}

/* Table 7-8 */
int File_DtsUhd::ParseStaticMDParams(MD01* MD01, bool OnlyFirst)
{
    bool NominalLD_DescriptionFlag=true;
    int8u NumLongTermLoudnessMsrmSets=1;

    if (FullChannelBasedMixFlag==false)
        Get_SB (NominalLD_DescriptionFlag,                      "NominalLD_DescriptionFlag");

    if (NominalLD_DescriptionFlag)
    {
        if (!FullChannelBasedMixFlag)
        {
            Get_S1 (1, NumLongTermLoudnessMsrmSets,            "NumLongTermLoudnessMsrmSets");
            NumLongTermLoudnessMsrmSets=1+(NumLongTermLoudnessMsrmSets<<1); //?3:1
            Param_Info2(NumLongTermLoudnessMsrmSets, " sets");
        }
    }
    else
    {
        Get_S1 (4, NumLongTermLoudnessMsrmSets,                 "NumLongTermLoudnessMsrmSets");
        NumLongTermLoudnessMsrmSets++;
        Param_Info2(NumLongTermLoudnessMsrmSets, " sets");
    }

    for (int i=0; i<NumLongTermLoudnessMsrmSets; i++)
        ExtractLTLMParamSet(MD01, NominalLD_DescriptionFlag);

    if (OnlyFirst)
        return 0;

    if (!NominalLD_DescriptionFlag)
        Skip_SB(                                                "IsLTLoudnMsrsmOffLine");

    const int NUMDRCOMPRTYPES=3;
    for (int i=0; i<NUMDRCOMPRTYPES; i++) /* Table 7-12 suggest 3 types */
    {
        bool CustomDRCCurveMDPresent;
        Get_SB (CustomDRCCurveMDPresent,                        "CustomDRCCurveMDPresent");
        if (CustomDRCCurveMDPresent)
        {
            Element_Begin1("ExtractCustomDRCCurves");
            int8u DRCCurveIndex;
            Get_S1 (4, DRCCurveIndex,                           "DRCCurveIndex"); /* Table 7-14 */
            if (DRCCurveIndex==15)
                Skip_S2(15,                                     "DRCCurveCode");
            Element_End0();
        }
        bool CustomDRCSmoothMDPresent;
        Get_SB (CustomDRCSmoothMDPresent,                       "CustomDRCSmoothMDPresent");
        if (CustomDRCSmoothMDPresent)
            Skip_BS(6*6,                                        "CDRCProfiles");
        if (CustomDRCSmoothMDPresent)
        {
            Skip_S1(6,                                          "FastAttack");
            Skip_S1(6,                                          "SlowAttack");
            Skip_S1(6,                                          "FastRelease");
            Skip_S1(6,                                          "SlowRelease");
            Skip_S1(6,                                          "AttackThreshld");
            Skip_S1(6,                                          "ReleaseThreshld");
        }
    }

    MD01->StaticMDParamsExtracted=true;

    return 0;
}

/* Table 7-7 */
int File_DtsUhd::ExtractMultiFrameDistribStaticMD(MD01* MD01)
{
    Element_Begin1("ExtractMultiFrameDistribStaticMD");
    static const uint8_t Table1[4] = {0, 6, 9, 12};
    static const uint8_t Table2[4] = {5, 7, 9, 11};

    if (SyncFrameFlag)
    {
        MD01->PacketsAcquired=0;
        if (FullChannelBasedMixFlag)
        {
            MD01->NumStaticMDPackets=1;
            MD01->StaticMDPacketByteSize=0/*2*/; //TEMP, only with p2.mp4
        }
        else
        {
            Get_VR (Table1, MD01->NumStaticMDPackets,           "NumStaticMDPackets");
            MD01->NumStaticMDPackets++;
            Get_VR (Table2, MD01->StaticMDPacketByteSize,       "StaticMDPacketByteSize");
            MD01->StaticMDPacketByteSize+=3;
        }

        MD01->Buffer.resize(MD01->NumStaticMDPackets*MD01->StaticMDPacketByteSize);
        if (MD01->NumStaticMDPackets>1)
            Get_SB (MD01->StaticMetadataUpdtFlag,               "StaticMetadataUpdtFlag");
        else
            MD01->StaticMetadataUpdtFlag=true;
    }

    if (MD01->PacketsAcquired<MD01->NumStaticMDPackets)
    {
        int n=MD01->PacketsAcquired*MD01->StaticMDPacketByteSize;
        for (int32u i = 0; i < MD01->StaticMDPacketByteSize; i++)
            Get_S1 (8/*i==1?6:8*/, MD01->Buffer[n+i],                       ("MetadataPacketPayload[" + std::to_string(n+i) + "]").c_str()); //TEMP, only with p2.mp4
        //MD01->Buffer[1]<<=2; //TEMP, only with p2.mp4
        //MD01->Buffer.resize(3); //TEMP, only with p2.mp4
        MD01->PacketsAcquired++;

        if (MD01->PacketsAcquired==MD01->NumStaticMDPackets || MD01->PacketsAcquired==1)
        {
            if (MD01->StaticMetadataUpdtFlag||!MD01->StaticMDParamsExtracted)
            {
                const int8u* Save_Buffer=nullptr;
                size_t Save_Buffer_Offset=0;
                size_t Save_Buffer_Size=0;
                int64u Save_Element_Offset=0;
                int64u Save_Element_Size=0;
                BitStream_Fast Save_BS;
                #if MEDIAINFO_TRACE
                    size_t Save_BS_Size=0;
                #endif
                if (!MD01->Buffer.empty())
                {
                    //Use the buffered content
                    Save_Buffer=Buffer;
                    Save_Buffer_Offset=Buffer_Offset;
                    Save_Buffer_Size=Buffer_Size;
                    Save_Element_Offset=Element_Offset;
                    Save_Element_Size=Element_Size;
                    Save_BS=*BS;
                    #if MEDIAINFO_TRACE
                        Save_BS_Size=BS_Size;
                    #endif
                    File_Offset+=Buffer_Offset+Element_Size-(Data_BS_Remain()+7)/8-MD01->StaticMDPacketByteSize;
                    Buffer=MD01->Buffer.data();
                    Buffer_Offset=0;
                    Buffer_Size=MD01->Buffer.size();
                    Element_Offset=0;
                    Element_Size=Buffer_Size;
                    BS_Begin();
                }
                auto Result=ParseStaticMDParams(MD01, MD01->PacketsAcquired!=MD01->NumStaticMDPackets);
                if (!MD01->Buffer.empty())
                {
                    //Back to normal buffer
                    if (Data_BS_Remain())
                        Skip_BS(Data_BS_Remain(),               "Padding");
                    BS_End();
                    Buffer=Save_Buffer;
                    Buffer_Offset=Save_Buffer_Offset;
                    Buffer_Size=Save_Buffer_Size;
                    Element_Offset=Save_Element_Offset;
                    Element_Size=Save_Element_Size;
                    *BS=Save_BS;
                    #if MEDIAINFO_TRACE
                        BS_Size=Save_BS_Size;
                    #endif
                    File_Offset-=Buffer_Offset+Element_Size-(Data_BS_Remain()+7)/8-MD01->StaticMDPacketByteSize;
                }
                if (Result)
                {
                    Element_End0();
                    return 1;
                }
            }
        }
    }

    Element_End0();
    return 0;
}

/* Return 1 if suitable, 0 if not.  Table 7-18.  OBJGROUPIDSTART=224 Sec 7.8.7 p75 */
bool File_DtsUhd::CheckIfMDIsSuitableforImplObjRenderer(MD01* MD01, int ObjectId)
{
    if (ObjectId>=224)
        return true;
    Element_Begin1("CheckIfMDIsSuitableforImplObjRenderer");
    bool MDUsedByAllRenderersFlag;
    Get_SB (MDUsedByAllRenderersFlag,                           "MDUsedByAllRenderersFlag");
    {
        Element_End0();
        return true;
    }

    /*  Reject the render and skip the render data. */
    int32u NumBits2Skip;
    constexpr int8u Table[4] = {8, 10, 12, 14};
    Skip_SB(                                                    "RequiredRendererType");
    Get_VR (Table, NumBits2Skip,                                "NumBits2Skip");
    Skip_BS (1+NumBits2Skip,                                    "data");
    Element_End0();
    return false;
}

/* Table 7-26 */
void File_DtsUhd::ExtractChMaskParams(MD01* MD01, MDObject* Object)
{
    Element_Begin1("ExtractChMaskParams");
    constexpr int32u MaskTable[14] = /* Table 7-27 */
    {
        0x000001, 0x000002, 0x000006, 0x00000F, 0x00001F, 0x00084B, 0x00002F, 0x00802F,
        0x00486B, 0x00886B, 0x03FBFB, 0x000003, 0x000007, 0x000843,
    };

    int8u ChLayoutIndex;
    if (Object->RepType==REP_TYPE_BINAURAL)
        ChLayoutIndex=1;
    else
        Get_S1 (4, ChLayoutIndex,                               "ChLayoutIndex");
    if (ChLayoutIndex>=14) // Index=14->read 16 bits, Index=15->read 32 bits
        Get_S4 (16<<(ChLayoutIndex-14), Object->ChActivityMask, "ChActivityMask");
    else
        Object->ChActivityMask=MaskTable[ChLayoutIndex];
    Element_End0();

    IsType1CertifiedContent=ChLayoutIndex==14 && (Object->ChActivityMask==0xF || Object->ChActivityMask==0x2F || Object->ChActivityMask==0x802F);
}

/* Table 7-22 */
int File_DtsUhd::ExtractObjectMetadata(MD01* MD01, MDObject* Object,
                                     bool ObjStartFrameFlag, int ObjectId)
{
    Element_Begin1("ExtractObjectMetadata");
    if (ObjectId!=256)
        Skip_SB(                                                "ObjActiveFlag");
    if (ObjStartFrameFlag)
    {
        bool ChMaskObjectFlag=false, Object3DMetaDataPresent=false;
        Get_S1 (3, Object->RepType,                             "ObjRepresTypeIndex");
        switch (Object->RepType)
        {
            case REP_TYPE_BINAURAL:
            case REP_TYPE_CH_MASK_BASED:
            case REP_TYPE_MTRX2D_CH_MASK_BASED:
            case REP_TYPE_MTRX3D_CH_MASK_BASED:
                ChMaskObjectFlag=true;
                break;
            case REP_TYPE_3D_OBJECT_SINGLE_SRC_PER_WF:
            case REP_TYPE_3D_MONO_OBJECT_SINGLE_SRC_PER_WF:
                Object3DMetaDataPresent=true;
                break;
        }

        if (ChMaskObjectFlag)
        {
            if (ObjectId!=256)
            {
                bool ObjTypeDescrPresent;
                Skip_S1(3,                                      "ObjectImportanceLevel");
                Get_SB (ObjTypeDescrPresent,                    "ObjTypeDescrPresent");
                if (ObjTypeDescrPresent)
                {
                    bool TypeBitWidth;
                    Get_SB (TypeBitWidth,                       "TypeBitWidth");
                    Skip_S1(TypeBitWidth?3:5,                   "ObjTypeDescrIndex");
                }

                constexpr int8u Table1[4]={1, 4, 4, 8};
                constexpr int8u Table2[4]={3, 3, 4, 8};
                Skip_VR(Table1,                                 "ObjAudioChunkIndex");
                Skip_VR(Table2,                                 "ObjNaviWithinACIndex");

                /* Skip optional Loudness block. */
                bool PerObjLTLoudnessMDPresent;
                Get_SB (PerObjLTLoudnessMDPresent,              "PerObjLTLoudnessMDPresent");
                if (PerObjLTLoudnessMDPresent)
                    Skip_S1(8,                                  "PerObjLTLoudnessMD");

                /* Skip optional Object Interactive MD (Table 7-25). */
                bool ObjInteractiveFlag;
                Get_SB (ObjInteractiveFlag,                     "ObjInteractiveFlag");
                if (ObjInteractiveFlag && InteractObjLimitsPresent)
                {
                    bool ObjInterLimitsFlag;
                    Get_SB (ObjInterLimitsFlag,                 "ObjInterLimitsFlag");
                    if (ObjInterLimitsFlag)
                        Skip_S2(5+6*Object3DMetaDataPresent,    "ObjectInteractMD");
                }
            }
            ExtractChMaskParams(MD01, Object);
        }
    }

    /* Skip rest of object */
    Element_End0();
    return 0;
}

/* Table 7-4 */
int File_DtsUhd::ParseMD01(MD01 *MD01, int AuPresInd)
{
    if (AudPresParam[AuPresInd].Selectable)
    {
        Element_Begin1("ExtractPresScalingParams");
        for (int i = 0; i<4; i++)  /* Table 7-5.  Scaling data. */
        {
            bool OutScalePresent;
            Get_SB (OutScalePresent,                            "OutScalePresent");
            if (OutScalePresent)
                Skip_S1(5,                                      "OutScale");
        }
        Element_End0();
        bool MFDistrStaticMDPresent;
        Get_SB (MFDistrStaticMDPresent,                         "MFDistrStaticMDPresent");
        if (MFDistrStaticMDPresent && ExtractMultiFrameDistribStaticMD(MD01))
            return 1;
    }

    /* Table 7-16: Object metadata. */
    memset(MD01->Object, 0, sizeof(MD01->Object));
    if (!FullChannelBasedMixFlag)
    {
        bool MixStudioParamsPresent;
        Get_SB (MixStudioParamsPresent,                     "MixStudioParamsPresent");
        if (MixStudioParamsPresent)
            Skip_S2(11,                                     "MixStudioParams");
    }

    for (int i=0; i<MD01->NumObjects; i++)
    {
        int32u Id = MD01->ObjectList[i];
        if (!CheckIfMDIsSuitableforImplObjRenderer(MD01, Id))
            continue;

        MD01->Object[Id].PresIndex = AuPresInd;
        bool StartFlag=false;
        if (!MD01->Object[Id].Started)
        {
            if (Id!=256)
                Skip_SB(                                    "ObjStaticFlag");
            StartFlag=MD01->Object[Id].Started=true;
        }

        if ((Id<224||Id>255) && ExtractObjectMetadata(MD01, MD01->Object+Id, StartFlag, Id))
            return 1;

        break;
    }

    return 0;
}

/* Table 6-2 */
int File_DtsUhd::UnpackMDFrame()
{
    Element_Begin1("UnpackMDFrame");
    for (auto& MDChunk : MD_Chunks)
    {
        if (!MDChunk.MDChunkSize)
            continue;
        if (MDChunk.MDChunkCRCFlag && CheckCRC(Buffer+Buffer_Offset, MDChunk.MDChunkSize))
        {
            Element_End0();
            return 1;
        }
        Element_Begin1("MDChunk");
        int64u End=Element_Offset+MDChunk.MDChunkSize;
        int8u MDChunkID;
        Get_B1 (MDChunkID,                                      "MDChunkID");
        switch (MDChunkID)
        {
            case 1 : UnpackMDFrame_1(MDChunkID); break;
        }
        if (End>Element_Offset)
            Skip_XX(End-Element_Offset,                        "(Unknown)");
        Element_End0();
    }

    Element_End0();
    return 0;
}

/* Table 6-2 */
void File_DtsUhd::UnpackMDFrame_1(int8u MDChunkID)
{
    BS_Begin();
    static constexpr int8u Table[4] = { 0, 2, 4, 4 };
    int32u AudPresIndex;
    Get_VR (Table, AudPresIndex,                                "AudPresIndex");
    if (AudPresIndex>255)
    {
        BS_End();
        return;
    }
    MD01* MD01=ChunkFindMD01(MDChunkID);
    if (MD01==nullptr)
        MD01=ChunkAppendMD01(MDChunkID);
    if (!MD01)
    {
        BS_End();
        return;
    }
    if (ExtractMDChunkObjIDList(MD01))
    {
        BS_End();
        return;
    }
    if (ParseMD01(MD01, AudPresIndex))
    {
        BS_End();
        return;
    }
    BS_End();
}

/** Parse a single DTS:X Profile 2 frame.
    The frame must start at the first byte of the data buffer, and enough
    of the frame must be present to decode the majority of the FTOC.
    From Table 6-11 p52.  A sync frame must be provided.
*/
int File_DtsUhd::Frame()
{
    int32u SyncWord;
    Get_B4 (SyncWord,                                           "SyncWord");
    SyncFrameFlag=SyncWord==DTSUHD_SYNCWORD;
    if (SyncFrameFlag)
        Element_Info1("Key frame");
    BS_Begin();
    static constexpr int8u Table[4]={5, 8, 10, 12};
    Get_VR (Table, FTOCPayloadinBytes,                          "FTOCPayloadinBytes");
    FTOCPayloadinBytes++;
    if (FTOCPayloadinBytes<=4 || FTOCPayloadinBytes>=FrameSize)
        return DTSUHD_INCOMPLETE;  //Data buffer does not contain entire FTOC
    if (ExtractStreamParams())
        return DTSUHD_INVALID_FRAME;
    if (ResolveAudPresParams())
        return DTSUHD_INVALID_FRAME;
    if (ExtractChunkNaviData())  //AudioChunkTypes and payload sizes.
        return DTSUHD_INVALID_FRAME;
    size_t Remain8=Data_BS_Remain()%8;
    if (Remain8)
        Skip_S1(Remain8,                                        "Padding");
    BS_End();
    if (SyncFrameFlag || !FullChannelBasedMixFlag)
        Skip_B2(                                                "CRC16");
    if (Element_Offset!=FTOCPayloadinBytes)
        return DTSUHD_INVALID_FRAME;

    //At this point in the parsing, we can calculate the size of the frame.
    if (FrameSize<FTOCPayloadinBytes+ChunkBytes)
        return DTSUHD_INCOMPLETE;
    FrameSize=FTOCPayloadinBytes+ChunkBytes;

    //Skip PBRSmoothParams (Table 6-26) and align to the chunks immediatelyfollowing the FTOC CRC.
    if (Element_Offset<FTOCPayloadinBytes)
        Skip_XX(FTOCPayloadinBytes-Element_Offset,              "PBRSmoothParams");
    if (UnpackMDFrame())
        return DTSUHD_INVALID_FRAME;
    UpdateDescriptor();

    return DTSUHD_OK;
}

const int32u FrequencyCodeTable[2]={44100, 48000};
const char* RepresentationTypeTable[8]=
{
    "Channel Mask Based Representation",
    "Matrix 2D Channel Mask Based Representation",
    "Matrix 3D Channel Mask Based Representation",
    "Binaurally Processed Audio Representation",
    "Ambisonic Representation Representation",
    "Audio Tracks with Mixing Matrix Representation",
    "3D Object with One 3D Source Per Waveform Representation",
    "Mono 3D Object with Multiple 3D Sources Per Waveform Representation",
};

//---------------------------------------------------------------------------
struct DTSUHD_ChannelMaskInfo DTSUHD_DecodeChannelMask(int32u ChannelMask)
{
    DTSUHD_ChannelMaskInfo Info={};

    // ETSI TS 103 491 V1.2.1 Table C-4: Speaker Labels for ChannelMask
    if (ChannelMask)
    {
        if (ChannelMask & 0x00000003)
        {
            Info.ChannelPositionsText+=", Front:";
            if (ChannelMask & 0x00000002)
            {
                Info.ChannelLayoutText+=" L";
                Info.ChannelPositionsText+=" L";
                Info.CountFront++;
            }
            if (ChannelMask & 0x00000001)
            {
                Info.ChannelLayoutText+=" C";
                Info.ChannelPositionsText+=" C";
                Info.CountFront++;
            }
            if (ChannelMask & 0x00000004)
            {
                Info.ChannelLayoutText+=" R";
                Info.ChannelPositionsText+=" R";
                Info.CountFront++;
            }
        }

        if (ChannelMask & 0x00000020)
            Info.ChannelLayoutText+=" LFE";
        if (ChannelMask & 0x00010000)
            Info.ChannelLayoutText+=" LFE2";

        if (ChannelMask & 0x00000618)
        {
            Info.ChannelPositionsText+=", Side:";
            if (ChannelMask & 0x00000008)
            {
                Info.ChannelLayoutText+=" Ls";
                Info.ChannelPositionsText+=" L";
                Info.CountSide++;
            }
            if (ChannelMask & 0x00000010)
            {
                Info.ChannelLayoutText+=" Rs";
                Info.ChannelPositionsText+=" R";
                Info.CountSide++;
            }
            if (ChannelMask & 0x00000200)
            {
                Info.ChannelLayoutText+=" Lss";
                Info.ChannelPositionsText+=" L";
                Info.CountSide++;
            }
            if (ChannelMask & 0x00000400)
            {
                Info.ChannelLayoutText+=" Rss";
                Info.ChannelPositionsText+=" R";
                Info.CountSide++;
            }
        }

        if (ChannelMask & 0x000001C0)
        {
            Info.ChannelPositionsText+=", Rear:";
            if (ChannelMask & 0x00000080)
            {
                Info.ChannelLayoutText+=" Lsr";
                Info.ChannelPositionsText+=" L";
                Info.CountRear++;
            }
            if (ChannelMask & 0x00000040)
            {
                Info.ChannelLayoutText+=" Cs";
                Info.ChannelPositionsText+=" C";
                Info.CountRear++;
            }
            if (ChannelMask & 0x00000100)
            {
                Info.ChannelLayoutText+=" Rsr";
                Info.ChannelPositionsText+=" R";
                Info.CountRear++;
            }
        }

        if (ChannelMask & 0x0E000000)
        {
            Info.ChannelPositionsText+=", LowFront:";
            if (ChannelMask & 0x04000000)
            {
                Info.ChannelLayoutText+=" Lb";
                Info.ChannelPositionsText+=" L";
                Info.CountLows++;
            }
            if (ChannelMask & 0x02000000)
            {
                Info.ChannelLayoutText+=" Cb";
                Info.ChannelPositionsText+=" C";
                Info.CountLows++;
            }
            if (ChannelMask & 0x08000000)
            {
                Info.ChannelLayoutText+=" Rb";
                Info.ChannelPositionsText+=" R";
                Info.CountLows++;
            }
        }

        if (ChannelMask & 0x0000E000)
        {
            Info.ChannelPositionsText+=", High:";
            if (ChannelMask & 0x00002000)
            {
                Info.ChannelLayoutText+=" Lh";
                Info.ChannelPositionsText+=" L";
                Info.CountHeights++;
            }
            if (ChannelMask & 0x00004000)
            {
                Info.ChannelLayoutText+=" Ch";
                Info.ChannelPositionsText+=" C";
                Info.CountHeights++;
            }
            if (ChannelMask & 0x00008000)
            {
                Info.ChannelLayoutText+=" Rh";
                Info.ChannelPositionsText+=" R";
                Info.CountHeights++;
            }
        }

        if (ChannelMask & 0x00060000)
        {
            Info.ChannelPositionsText+=", Wide:";
            if (ChannelMask & 0x00020000)
            {
                Info.ChannelLayoutText+=" Lw";
                Info.ChannelPositionsText+=" L";
                Info.CountFront++;
            }
            if (ChannelMask & 0x00040000)
            {
                Info.ChannelLayoutText+=" Rw";
                Info.ChannelPositionsText+=" R";
                Info.CountFront++;
            }
        }

        if (ChannelMask & 0x30000000)
        {
            Info.ChannelPositionsText+=", TopFront:";
            if (ChannelMask & 0x10000000)
            {
                Info.ChannelLayoutText+=" Ltf";
                Info.ChannelPositionsText+=" L";
                Info.CountHeights++;
            }
            if (ChannelMask & 0x20000000)
            {
                Info.ChannelLayoutText+=" Rtf";
                Info.ChannelPositionsText+=" R";
                Info.CountHeights++;
            }
        }

        if (ChannelMask & 0x00080000)
        {
            Info.ChannelPositionsText+=", TopCtrSrrd";
            Info.ChannelLayoutText+=" Oh";
            Info.CountHeights++;
        }

        if (ChannelMask & 0x00001800)
        {
            Info.ChannelPositionsText+=", Center:";
            if (ChannelMask & 0x00000800)
            {
                Info.ChannelLayoutText+=" Lc";
                Info.ChannelPositionsText+=" L";
                Info.CountFront++;
            }
            if (ChannelMask & 0x00001000)
            {
                Info.ChannelLayoutText+=" Rc";
                Info.ChannelPositionsText+=" R";
                Info.CountFront++;
            }
        }

        if (ChannelMask & 0xC0000000)
        {
            Info.ChannelPositionsText+=", TopRear:";
            if (ChannelMask & 0x40000000)
            {
                Info.ChannelLayoutText+=" Ltr";
                Info.ChannelPositionsText+=" L";
                Info.CountHeights++;
            }
            if (ChannelMask & 0x80000000)
            {
                Info.ChannelLayoutText+=" Rtr";
                Info.ChannelPositionsText+=" R";
                Info.CountHeights++;
            }
        }

        if (ChannelMask & 0x00300000)
        {
            Info.ChannelPositionsText+=", HighSide:";
            if (ChannelMask & 0x00100000)
            {
                Info.ChannelLayoutText+=" Lhs";
                Info.ChannelPositionsText+=" L";
                Info.CountHeights++;
            }
            if (ChannelMask & 0x00200000)
            {
                Info.ChannelLayoutText+=" Rhs";
                Info.ChannelPositionsText+=" R";
                Info.CountHeights++;
            }
        }

        if (ChannelMask & 0x01C00000)
        {
            Info.ChannelPositionsText+=", HighRear:";
            if (ChannelMask & 0x00800000)
            {
                Info.ChannelLayoutText+=" Lhr";
                Info.ChannelPositionsText+=" L";
                Info.CountHeights++;
            }
            if (ChannelMask & 0x00400000)
            {
                Info.ChannelLayoutText+=" Chr";
                Info.ChannelPositionsText+=" C";
                Info.CountHeights++;
            }
            if (ChannelMask & 0x01000000)
            {
                Info.ChannelLayoutText+=" Rhr";
                Info.ChannelPositionsText+=" R";
                Info.CountHeights++;
            }
        }

        if (ChannelMask & 0x00000020)
        {
            Info.ChannelPositionsText+=", LFE";
            Info.CountLFE++;
        }
        if (ChannelMask & 0x00010000)
        {
            Info.ChannelPositionsText+=", LFE2";
            Info.CountLFE++;
        }

        Info.ChannelLayoutText.erase(0, 1);
        Info.ChannelPositionsText.erase(0, 2);
        Info.ChannelPositions2Text = std::to_string(Info.CountFront) + "/" +
                std::to_string(Info.CountSide) + "/" +
                std::to_string(Info.CountRear) + "." +
                std::to_string(Info.CountLFE);
        if (Info.CountHeights)
            Info.ChannelPositions2Text+="." + std::to_string(Info.CountHeights);
        if (Info.CountLows)
            Info.ChannelPositions2Text+="." + std::to_string(Info.CountLows);
    }

    Info.ChannelCount=Info.CountFront+Info.CountSide+Info.CountRear+Info.CountLFE+Info.CountHeights+Info.CountLows;

    return Info;
}

bool File_DtsUhd::CheckCurrentFrame()
{
    //Read length of CRC'd frame data and perform CRC check
    #if MEDIAINFO_TRACE
    auto Trace_Activated_Sav=Trace_Activated;
    Trace_Activated=false; // Do not show trace while trying to sync
    #endif
    constexpr int8u VbitsPayload[4]={5, 8, 10, 12}; //varbits decode table
    bool SyncFrame=CC4(Buffer+Buffer_Offset)==DTSUHD_SYNCWORD;
    Buffer_Offset+=4; // SyncWord
    Element_Size=Buffer_Size-Buffer_Offset;
    BS_Begin();
    int32u FtocSize;
    Get_VR(VbitsPayload, FtocSize,                              "FTOCPayloadinBytes");
    FtocSize++;
    if (SyncFrame)
        Get_SB(FullChannelBasedMixFlag,                         "FullChannelBasedMixFlag");
    BS_End();
    #if MEDIAINFO_TRACE
    Trace_Activated=Trace_Activated_Sav;
    #endif
    Buffer_Offset-=4; // SyncWord
    Element_Offset=0; // Reset BitStream parser
    if (FtocSize>Buffer_Size-Buffer_Offset)
        return false;
    bool HasCRC=SyncFrame||!FullChannelBasedMixFlag;
    return !HasCRC || CheckCRC(Buffer+Buffer_Offset, FtocSize)==0;
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_DtsUhd::File_DtsUhd()
:File_Dts_Common()
{
    //Configuration
    ParserName="DtsUhd";
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_Dts;
        StreamIDs_Width[0]=0;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
    IsType1CertifiedContent=false;
    LongTermLoudnessIndex=(int8u)-1;
    MustSynchronize=true;
    Buffer_TotalBytes_FirstSynched_Max=64*1024;
    PTS_DTS_Needed=true;
    StreamSource=IsStream;
}

//---------------------------------------------------------------------------
void File_DtsUhd::Streams_Fill()
{
    DTSUHD_ChannelMaskInfo ChannelMaskInfo = DTSUHD_DecodeChannelMask(FrameDescriptor.ChannelMask);
    float32 BitRate_Max=0;
    if (FrameDuration && Retrieve_Const(Stream_Audio, 0, Audio_BitRate_Maximum).empty()) // Only if not provided by header
    {
        int32u MaxPayload=2048<<FrameDescriptor.MaxPayloadCode;
        BitRate_Max = 8.0f * MaxPayload * SampleRate / FrameDuration;
    }
    std::string AudioCodec="dtsx", CommercialString="DTS:X P2";
    AudioCodec.back()+=FrameDescriptor.DecoderProfileCode > 0;
    CommercialString.back()+=FrameDescriptor.DecoderProfileCode;
    if (IsType1CertifiedContent)
        CommercialString+=" with IMAX Enhanced";

    Fill(Stream_General, 0, General_Format, "DTS-UHD");
    Fill(Stream_General, 0, General_OverallBitRate_Mode, "VBR");

    Stream_Prepare(Stream_Audio);
    if (BitRate_Max)
        Fill(Stream_Audio, 0, Audio_BitRate_Maximum, BitRate_Max, 0, true);
    Fill(Stream_Audio, 0, Audio_BitRate_Mode, "VBR", Unlimited, true, true);
    Fill(Stream_Audio, 0, Audio_Codec, AudioCodec);
    Fill(Stream_Audio, 0, Audio_Format, "DTS-UHD");
    Fill(Stream_Audio, 0, Audio_Format_Commercial_IfAny, CommercialString);
    Fill(Stream_Audio, 0, Audio_Format_Profile, FrameDescriptor.DecoderProfileCode + 2);
    Fill(Stream_Audio, 0, Audio_Format_Settings, RepresentationTypeTable[FrameDescriptor.RepType]);
    if (IsType1CertifiedContent)
        Fill(Stream_Audio, 0, Audio_Format_Settings, "T1-CC");
    Fill(Stream_Audio, 0, Audio_SamplesPerFrame, FrameDescriptor.SampleCount, 10, true);
    if (SampleRate)
        Fill(Stream_Audio, 0, Audio_SamplingRate, SampleRate);

    if (LongTermLoudnessIndex<64)
    {
        Fill(Stream_Audio, 0, "Loudness", "Yes", Unlimited, true, true);
        Fill_Measure(Stream_Audio, 0, "Loudness LongTermLoudness", Ztring::ToZtring(LongTermLoudnessMeasure[LongTermLoudnessIndex], 2), __T(" LKFS"));
    }

    if (FrameDescriptor.ChannelMask)
    {
        Fill(Stream_Audio, 0, Audio_Channel_s_, ChannelMaskInfo.ChannelCount);
        Fill(Stream_Audio, 0, Audio_ChannelLayout, ChannelMaskInfo.ChannelLayoutText);
        Fill(Stream_Audio, 0, Audio_ChannelPositions, ChannelMaskInfo.ChannelPositionsText);
        Fill(Stream_Audio, 0, Audio_ChannelPositions_String2, ChannelMaskInfo.ChannelPositions2Text);
    }
}

//---------------------------------------------------------------------------
bool File_DtsUhd::Synchronize()
{
    while (Buffer_Offset+4<=Buffer_Size)
    {
        if (!FrameSynchPoint_Test(false))
            return false; //Need more data
        if (Synched)
            break;
        Buffer_Offset++;
    }

    return true;
}

//---------------------------------------------------------------------------
bool File_DtsUhd::Synched_Test()
{
     //Quick test of synchro
    if (!FrameSynchPoint_Test(true))
        return false; //Need more data
    if (!Synched)
    {
        if (Stream_Offset_Max!=-1 && File_Offset+Buffer_Offset==Stream_Offset_Max && File_Size!=-1)
            Synched=true; // It is the file footer
        return true;
    }

    //We continue
    return true;
}

//---------------------------------------------------------------------------
void File_DtsUhd::Read_Buffer_Unsynched()
{
    FrameInfo=frame_info();
}

//---------------------------------------------------------------------------
void File_DtsUhd::Header_Parse()
{
    Header_Fill_Size(FrameSize);
}

//---------------------------------------------------------------------------
void File_DtsUhd::Data_Parse()
{
    Element_Name("Frame");
    Element_Info1(Frame_Count);
    if (Frame()!=DTSUHD_OK)
        Trusted_IsNot("Parsing issue");
    for (const auto& Audio_Chunk : Audio_Chunks)
        Skip_XX(Audio_Chunk.AudioChunkSize,                     "AudioChunk");
    Skip_XX(Element_Size-Element_Offset,                        "(Unknown)");

    FILLING_BEGIN();
        if (!Status[IsAccepted])
            Accept("DTS-UHD");
        Frame_Count++;
        if (Frame_Count>=Frame_Count_Valid)
        {
            Fill("DTS-UHD");

            //No more need data
            if (!IsSub && Config->ParseSpeed<1.0)
            {
                if (Stream_Offset_Max!=-1)
                    GoTo(Stream_Offset_Max);
                else
                    Finish("DTS");
            }
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
bool File_DtsUhd::FrameSynchPoint_Test(bool AcceptNonSync)
{
    if (Buffer_Offset+16>Buffer_Size)
        return false; //Must wait for more data

    int32u SyncWord=CC4(Buffer+Buffer_Offset);
    if (SyncWord!=DTSUHD_SYNCWORD && (!AcceptNonSync || SyncWord!=DTSUHD_NONSYNCWORD))
    {
        Synched=false;
        return true; //Not a syncpoint, done
    }

    Synched=CheckCurrentFrame();

    if (Synched)
    {
        FrameSize=4;
        if (IsSub)
        {
            FrameSize+=Element_Size;
            return true; // We trust the container enough
        }
        while (Buffer_Offset+FrameSize+4<=Buffer_Size)
        {
            int32u SyncWord=CC4(Buffer+Buffer_Offset+FrameSize);
            if (SyncWord==DTSUHD_SYNCWORD||SyncWord==DTSUHD_NONSYNCWORD)
            {
                Buffer_Offset+=FrameSize;
                bool IsOk=CheckCurrentFrame();
                Buffer_Offset-=FrameSize;
                if (IsOk)
                    return true;
            }
            FrameSize++;
        }
    }

    return false; //Must wait for more data
}

//---------------------------------------------------------------------------
} //NameSpace

#endif //MEDIAINFO_DTSUHD_YES

/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_DtsH
#define MediaInfo_DtsH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#ifdef ES
   #undef ES //Solaris defines this somewhere
#endif
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Dts
//***************************************************************************

class File_Dts_Common : public File__Analyze
{
public :
    //In
    int64u Frame_Count_Valid;

    //Buffer - File header
    bool FileHeader_Begin();
    void FileHeader_Parse();

    //Buffer - Per element
    bool Header_Begin();

protected :
    int64u Stream_Offset_Max=-1;
};

//***************************************************************************
// Class File_Dts
//***************************************************************************

class File_Dts : public File_Dts_Common
{
public :
    //Constructor/Destructor
    File_Dts();

private :
    //Streams management
    void Streams_Fill();
    void Streams_Finish();

    //Buffer - Synchro
    bool Synchronize();
    bool Synched_Test();
    void Read_Buffer_Unsynched();

    //Buffer - Demux
    #if MEDIAINFO_DEMUX
    bool Demux_UnpacketizeContainer_Test();
    #endif //MEDIAINFO_DEMUX

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void Core();
    void Extensions();
    void Extensions_Resynch(bool Known);
    void Extensions_Padding();
    void Padding4() { Extensions_Resynch(true); }
    void LBR();
    void X96();
    void XLL();
    void XCh();
    void XXCH();
    void XBR();
    void Aux() { Extensions_Resynch(true); }
    void AfterAssets();
    void Extensions2();

    //Buffer
    bool FrameSynchPoint_Test();
    const int8u* Save_Buffer;
    size_t Save_Buffer_Offset;
    size_t Save_Buffer_Size;

    //Temp
    std::vector<ZenLib::int32u> Asset_Sizes;
    int32u Original_Size;
    int32u HD_size;
    int16u Primary_Frame_Byte_Size;
    int16u Number_Of_PCM_Sample_Blocks;
    int16u HD_SpeakerActivityMask;
    int8u  channel_arrangement;
    int8u  sample_frequency;
    int8u  bit_rate;
    int8u  lfe_effects;
    int8u  bits_per_sample;
    int8u  ExtensionAudioDescriptor;
    int8u  HD_BitResolution;
    int8u  HD_BitResolution_Real;
    int8u  HD_MaximumSampleRate;
    int8u  HD_MaximumSampleRate_Real;
    int8u  HD_TotalNumberChannels;
    int8u  HD_ExSSFrameDurationCode;
    bool   AuxiliaryData;
    bool   ExtendedCoding;
    bool   Word;
    bool   BigEndian;
    bool   ES;
    bool   Core_Exists;
    bool   One2OneMapChannels2Speakers;
    enum   presence
    {
        presence_Core_Core,
        presence_Core_XXCH,
        presence_Core_X96,
        presence_Core_XCh,
        presence_Extended_Core,
        presence_Extended_XBR,
        presence_Extended_XXCH,
        presence_Extended_X96,
        presence_Extended_LBR,
        presence_Extended_XLL,
        presence_Extended_ReservedA,
        presence_Extended_ReservedB,
        // Extra not in extension mask
        presence_Extended_XCh,
        presence_Extended_X,
        presence_Extended_IMAX,
        presence_Max
    };
    std::bitset<presence_Max> Presence;
    enum   data
    {
        Profiles,
        Channels,
        ChannelPositions,
        ChannelPositions2,
        ChannelLayout,
        BitDepth,
        SamplingRate,
        SamplesPerFrame,
        BitRate,
        BitRate_Mode,
        Compression_Mode,
        data_Max,
    };
    ZtringList Data[data_Max];
    int8u  Core_Core_AMODE;
    int8u  Core_Core_LFF;
    int8u  Core_XCh_AMODE;
    int8u  Core_XXCH_nuNumChSetsInXXCH;

    //Helpers
    float64 BitRate_Get(bool WithHD=false);
    void    Streams_Fill_Extension();
    void    Streams_Fill_Core_ES();
    void    Streams_Fill_Core(bool With96k=false);
};

} //NameSpace

#endif

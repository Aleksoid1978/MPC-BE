/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

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
#if defined(MEDIAINFO_AC4_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Ac4.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
using namespace ZenLib;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
// CRC_16_Table
extern const int16u CRC_16_Table[256];

//---------------------------------------------------------------------------
extern const float64 Ac4_frame_rate[2][16]=
{
    { //44.1 kHz
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)11025/(float64)512,
        (float64)0,
        (float64)0,
    },
    { //48 kHz
        (float64)24000/(float64)1001,
        (float64)24,
        (float64)25,
        (float64)30000/(float64)1001,
        (float64)30,
        (float64)48000/(float64)1001,
        (float64)48,
        (float64)50,
        (float64)60000/(float64)1001,
        (float64)60,
        (float64)100,
        (float64)120000/(float64)1001,
        (float64)120,
        (float64)12000/(float64)512,
        (float64)0,
        (float64)0,
    },
};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Ac4::File_Ac4()
:File__Analyze()
{
    //Configuration
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
    MustSynchronize=true;
    Buffer_TotalBytes_FirstSynched_Max=32*1024;
    Buffer_TotalBytes_Fill_Max=1024*1024;
    PTS_DTS_Needed=true;
    StreamSource=IsStream;
    Frame_Count_NotParsedIncluded=0;

    //In
    Frame_Count_Valid=0;
    MustParse_dac4=false;
}

//---------------------------------------------------------------------------
File_Ac4::~File_Ac4()
{
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ac4::Streams_Fill()
{
    Fill(Stream_General, 0, General_Format, "AC-4");

    Stream_Prepare(Stream_Audio);
    Fill(Stream_Audio, 0, Audio_Format, "AC-4");
    Fill(Stream_Audio, 0, Audio_Format_Commercial_IfAny, "Dolby AC-4");
    Fill(Stream_Audio, 0, Audio_Format_Version, __T("Version ")+Ztring::ToZtring(bitstream_version));
    Fill(Stream_Audio, 0, Audio_SamplingRate, fs_index?48000:44100);
    Fill(Stream_Audio, 0, Audio_FrameRate, Ac4_frame_rate[fs_index][frame_rate_index]);
}

//---------------------------------------------------------------------------
void File_Ac4::Streams_Finish()
{
}

//---------------------------------------------------------------------------
void File_Ac4::Read_Buffer_Unsynched()
{
}

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Ac4::Synchronize()
{
    //Synchronizing
    size_t Buffer_Offset_Current;
    while (Buffer_Offset<Buffer_Size)
    {
        Buffer_Offset_Current=Buffer_Offset;
        Synched=true; //For using Synched_Test()
        int8s i=0;
        const int8s count=4;
        for (; i<count; i++) //4 frames in a row tested
        {
            if (!Synched_Test())
            {
                Buffer_Offset=Buffer_Offset_Current;
                Synched=false;
                return false;
            }
            if (!Synched)
                break;
            Buffer_Offset+=frame_size;
        }
        Buffer_Offset=Buffer_Offset_Current;
        if (i==count)
            break;
        Buffer_Offset++;
    }
    Buffer_Offset=Buffer_Offset_Current;

    //Parsing last bytes if needed
    if (Buffer_Offset+4>Buffer_Size)
    {
        while (Buffer_Offset+2<=Buffer_Size && (BigEndian2int16u(Buffer+Buffer_Offset)>>1)!=(0xAC40>>1))
            Buffer_Offset++;
        if (Buffer_Offset+1==Buffer_Size && Buffer[Buffer_Offset]==0xAC)
            Buffer_Offset++;
        return false;
    }

    //Synched
    return true;
}

//---------------------------------------------------------------------------
void File_Ac4::Synched_Init()
{
    Accept();
    
    if (!Frame_Count_Valid)
        Frame_Count_Valid=Config->ParseSpeed>=0.3?32:2;

    //FrameInfo
    PTS_End=0;
    if (!IsSub)
    {
        FrameInfo.DTS=0; //No DTS in container
        FrameInfo.PTS=0; //No PTS in container
    }
    DTS_Begin=FrameInfo.DTS;
    DTS_End=FrameInfo.DTS;
    if (Frame_Count_NotParsedIncluded==(int64u)-1)
        Frame_Count_NotParsedIncluded=0; //No Frame_Count_NotParsedIncluded in the container
}

//---------------------------------------------------------------------------
bool File_Ac4::Synched_Test()
{
    //Must have enough buffer for having header
    if (Buffer_Offset+4>=Buffer_Size)
        return false;

    //sync_word
    sync_word=BigEndian2int16u(Buffer+Buffer_Offset);
    if ((sync_word>>1)!=(0xAC40>>1)) //0xAC40 or 0xAC41
    {
        Synched=false;
        return true;
    }

    //frame_size
    frame_size=BigEndian2int16u(Buffer+Buffer_Offset+2);
    if (frame_size==(int16u)-1)
    {
        if (Buffer_Offset+7>Buffer_Size)
            return false;
        frame_size=BigEndian2int24u(Buffer+Buffer_Offset+4)+7;
    }
    else
        frame_size+=4;

    //crc_word
    if (sync_word&1)
    {
        frame_size+=2;
        if (Buffer_Offset+frame_size>Buffer_Size)
            return false;
        if (!CRC_Compute(frame_size))
            Synched=false;
    }

    //We continue
    return true;
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ac4::Read_Buffer_Continue()
{
    if (MustParse_dac4)
    {
        dac4();
        return;
    }

    if (!MustSynchronize)
    {
        raw_ac4_frame();
        Buffer_Offset=Buffer_Size;
    }
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ac4::Header_Parse()
{
    //Parsing
    //sync_word & frame_size16 were previously calculated in Synched_Test()
    int16u frame_size16;
    Skip_B2 (                                                   "sync_word");
    Get_B2 (frame_size16,                                       "frame_size");
    if (frame_size16==0xFFFF)
        Skip_B3(                                                "frame_size");

    //Filling
    Header_Fill_Size(frame_size);
    Header_Fill_Code(sync_word, "ac4_syncframe");
}

//---------------------------------------------------------------------------
void File_Ac4::Data_Parse()
{
    //CRC
    if (Element_Code==0xAC41)
        Element_Size-=2;

    //Parsing
    raw_ac4_frame();
    
    //CRC
    Element_Offset=Element_Size;
    if (Element_Code==0xAC41)
    {
        Element_Size+=2;
        Skip_B2(                                                "crc_word");
    }
}

//---------------------------------------------------------------------------
void File_Ac4::raw_ac4_frame()
{
    Element_Begin1("raw_ac4_frame");
    int16u sequence_counter;
    BS_Begin();
    Get_S1 (2, bitstream_version,                               "bitstream_version");
    if (bitstream_version==3)
    {
        int32u bitstream_version32; 
        Get_V4 (2, bitstream_version32,                         "bitstream_version");
        bitstream_version32+=3;
        bitstream_version=(int8u)bitstream_version32;
    }
    Get_S2 (10, sequence_counter,                               "sequence_counter");
    TEST_SB_SKIP(                                               "b_wait_frames");
        int8u wait_frames;
        Get_S1 (3, wait_frames,                                 "wait_frames");
        if (wait_frames)
            Skip_S1(2,                                          "reserved");
    TEST_SB_END();
    Get_SB (   fs_index,                                        "fs_index");
    Get_S1 (4, frame_rate_index,                                "frame_rate_index"); Param_Info1(Ac4_frame_rate[fs_index][frame_rate_index]);
    BS_End();
    Element_End0();

    FILLING_BEGIN();
        Frame_Count++;
        if (!Status[IsFilled] && Frame_Count>=Frame_Count_Valid)
        {
            Fill();
            Finish();
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_toc()
{
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_presentation_info()
{
}

//---------------------------------------------------------------------------
void File_Ac4::substream_index_table()
{
}

//---------------------------------------------------------------------------
void File_Ac4::dac4()
{
    Element_Begin1("ac4_dsi");
    BS_Begin();
    int16u n_presentations;
    int8u ac4_dsi_version;
    Get_S1 (3, ac4_dsi_version,                                 "ac4_dsi_version");
    if (ac4_dsi_version>1)
    {
        Skip_BS(Data_BS_Remain(),                               "Unknown");
        BS_End();
        return;
    }
    Get_S1 (7, bitstream_version,                               "bitstream_version");
    if (bitstream_version>2)
    {
        Skip_BS(Data_BS_Remain(),                               "Unknown");
        BS_End();
        return;
    }
    Get_SB (   fs_index,                                        "fs_index");
    Get_S1 (4, frame_rate_index,                                "frame_rate_index"); Param_Info1(Ac4_frame_rate[fs_index][frame_rate_index]);
    Get_S2 (9, n_presentations,                                 "n_presentations");
    BS_End();
    Element_End0();

    FILLING_BEGIN();
        Accept();
    FILLING_END();
    Element_Offset=Element_Size;
    MustParse_dac4=false;
}

//***************************************************************************
// Parsing
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ac4::Get_V4(int8u  Bits, int32u  &Info, const char* Name)
{
    Info = 0;

    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            int8u Count = 0;
            do
            {
                Info += BS->Get4(Bits);
                Count += Bits;
            }
            while (BS->GetB());
            Param(Name, Info, Count);
            Param_Info(__T("(")+Ztring::ToZtring(Count)+__T(" bits)"));
        }
        else
    #endif //MEDIAINFO_TRACE
        {
            do
                Info += BS->Get4(Bits);
            while (BS->GetB());
        }
}

//---------------------------------------------------------------------------
void File_Ac4::Skip_V4(int8u  Bits, const char* Name)
{
    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            int8u Info = 0;
            int8u Count = 0;
            do
            {
                Info += BS->Get4(Bits);
                Count += Bits;
            }
            while (BS->GetB());
            Param(Name, Info, Count);
            Param_Info(__T("(")+Ztring::ToZtring(Count)+__T(" bits)"));
        }
        else
    #endif //MEDIAINFO_TRACE
        {
            do
                BS->Skip(Bits);
            while (BS->GetB());
        }
}

//***************************************************************************
// Utils
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Ac4::CRC_Compute(size_t Size)
{
    int16u CRC_16=0x0000;
    const int8u* CRC_16_Buffer=Buffer+Buffer_Offset+2; //After sync_word
    const int8u* CRC_16_Buffer_End=Buffer+Buffer_Offset+Size; //End of frame
    while(CRC_16_Buffer<CRC_16_Buffer_End)
    {
        CRC_16=(CRC_16<<8) ^ CRC_16_Table[(CRC_16>>8)^(*CRC_16_Buffer)];
        CRC_16_Buffer++;
    }

    return (CRC_16==0x0000);
}

} //NameSpace

#endif //MEDIAINFO_AC4_YES

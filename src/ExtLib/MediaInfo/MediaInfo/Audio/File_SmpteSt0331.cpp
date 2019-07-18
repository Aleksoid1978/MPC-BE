/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// AES3 PCM and non-PCM (SMPTE 337M)
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
#if defined(MEDIAINFO_SMPTEST0331_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_SmpteSt0331.h"
#if MEDIAINFO_EVENTS
    #include "MediaInfo/MediaInfo_Events.h"
#endif //MEDIAINFO_EVENTS
#if MEDIAINFO_SEEK
    #include "MediaInfo/MediaInfo_Internal.h"
#endif //MEDIAINFO_SEEK
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_SmpteSt0331::File_SmpteSt0331()
:File__Analyze()
{
    //Configuration
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_Aes3;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
    PTS_DTS_Needed=true;
    StreamSource=IsStream;

    //In
    QuantizationBits=0;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_SmpteSt0331::Streams_Fill()
{
    int8u Channels_Count=0;
    for (int8u Pos=0; Pos<8; Pos++)
        if (Channels_valid&(1<<Pos))
            Channels_Count++;

    Stream_Prepare(Stream_Audio);
    Fill(Stream_Audio, 0, Audio_Format, "PCM");
    Fill(Stream_Audio, 0, Audio_Codec, "PCM");
    Fill(Stream_Audio, 0, Audio_SamplingRate, 48000);
    if (QuantizationBits)
        Fill(Stream_Audio, 0, Audio_BitRate, Channels_Count*QuantizationBits*48000);
    Fill(Stream_Audio, 0, Audio_BitRate_Encoded, 8*32*48000);
    Fill(Stream_Audio, 0, Audio_BitRate_Mode, "CBR");

    Fill(Stream_Audio, 0, Audio_Format_Settings_Endianness, "Little");
    Fill(Stream_Audio, 0, Audio_Channel_s_, Channels_Count);
    if (QuantizationBits)
        Fill(Stream_Audio, 0, Audio_BitDepth, QuantizationBits);
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_SmpteSt0331::Read_Buffer_Continue()
{
    if (!Status[IsAccepted])
        Accept("SMPTE ST 331");

    //SMPTE 331M
    BS_Begin();
    Skip_SB(                                                "FVUCP Valid Flag");
    Skip_S1(4,                                              "Reserved");
    Skip_S1(3,                                              "5-sequence count");
    BS_End();
    Skip_L2(                                                "Audio Sample Count");
    Get_B1 (Channels_valid,                                 "Channels valid");

    if (QuantizationBits && Element_Offset<Element_Size)
    {
        int8u* Info = new int8u[(size_t)((Element_Size - Element_Offset)*((QuantizationBits == 16) ? 2 : 3) / 4)];
        size_t Info_Offset=0;

        while (Element_Offset+8*4<=Element_Size)
        {
            for (int8u Pos=0; Pos<8; Pos++)
            {
                if (Channels_valid&(1<<Pos))
                {
                    size_t Buffer_Pos=Buffer_Offset+(size_t)Element_Offset;

                    if (QuantizationBits==16)
                    {
                        Info[Info_Offset+0] = (Buffer[Buffer_Pos+1]>>4) | ((Buffer[Buffer_Pos+2]<<4)&0xF0 );
                        Info[Info_Offset+1] = (Buffer[Buffer_Pos+2]>>4) | ((Buffer[Buffer_Pos+3]<<4)&0xF0 );
                    }
                    else
                    {
                        Info[Info_Offset+0] = (Buffer[Buffer_Pos+0]>>4) | ((Buffer[Buffer_Pos+1]<<4)&0xF0 );
                        Info[Info_Offset+1] = (Buffer[Buffer_Pos+1]>>4) | ((Buffer[Buffer_Pos+2]<<4)&0xF0 );
                        Info[Info_Offset+2] = (Buffer[Buffer_Pos+2]>>4) | ((Buffer[Buffer_Pos+3]<<4)&0xF0 );
                    }

                    Info_Offset+=QuantizationBits==16?2:3;
                }
                Element_Offset+=4;
            }
        }
        Element_Offset=4;

        #if MEDIAINFO_DEMUX
            OriginalBuffer_Size=(size_t)Element_Size;
            OriginalBuffer=(int8u*)(Buffer+Buffer_Offset);

            FrameInfo.PTS=FrameInfo.DTS;
            FrameInfo.DUR=(Element_Size-4)*1000000000/48000/32; // 48 kHz, 4 bytes per sample
            Demux_random_access=true;
            Element_Code=(int64u)-1;
            Element_Offset=0;
            Demux(Info, Info_Offset, ContentType_MainStream);
            Element_Offset=4;

            OriginalBuffer_Size=0;
            OriginalBuffer=NULL;
        #endif //MEDIAINFO_DEMUX

        delete[] Info;
    }

    Skip_XX(Element_Size-4,                             "Data");

    Frame_Count_InThisBlock++;
    if (Frame_Count_NotParsedIncluded!=(int64u)-1)
        Frame_Count_NotParsedIncluded++;
    #if MEDIAINFO_DEMUX
        if (FrameInfo.DUR!=(int64u)-1)
        {
            if (FrameInfo.DTS!=(int64u)-1)
                FrameInfo.DTS+=FrameInfo.DUR;
            if (FrameInfo.PTS!=(int64u)-1)
                FrameInfo.PTS+=FrameInfo.DUR;
        }
        else
        {
            FrameInfo.DTS=(int64u)-1;
            FrameInfo.PTS=(int64u)-1;
        }
    #endif //MEDIAINFO_DEMUX

    FILLING_BEGIN();
    if (!Status[IsAccepted])
    {
        Accept("SMPTE ST 331");

        int8u Channels=0;
        for (int8u Pos=0; Pos<8; Pos++)
        {
            if (Channels_valid&(1<<Pos))
                Channels++;
            Element_Offset+=4;
        }

        Stream_Prepare(Stream_Audio);
        Fill(Stream_Audio, 0, Audio_Format, "PCM");
        Fill(Stream_Audio, 0, Audio_Channel_s_, Channels);
    }
    FILLING_END();
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_SMPTEST0331_YES

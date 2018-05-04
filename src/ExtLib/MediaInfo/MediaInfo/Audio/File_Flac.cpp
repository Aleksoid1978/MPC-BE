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
#if defined(MEDIAINFO_FLAC_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Flac.h"
#include "MediaInfo/Tag/File_VorbisCom.h"
#include "ThirdParty/base64/base64.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
extern const char* Id3v2_PictureType(int8u Type); //In Tag/File_Id3v2.cpp
extern std::string ExtensibleWave_ChannelMask (int32u ChannelMask); //In Multiple/File_Riff_Elements.cpp
extern std::string ExtensibleWave_ChannelMask2 (int32u ChannelMask); //In Multiple/File_Riff_Elements.cpp

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Flac::File_Flac()
:File__Analyze(), File__Tags_Helper()
{
    //File__Tags_Helper
    Base=this;

    //In
    VorbisHeader=false;

    //Temp
    Last_metadata_block=false;
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Flac::FileHeader_Begin()
{
    if (!File__Tags_Helper::FileHeader_Begin())
        return false;

    //Element_Size
    if (Buffer_Size<Buffer_Offset+4+(VorbisHeader?9:0))
        return false; //Must wait for more data

    if (CC4(Buffer+Buffer_Offset+(VorbisHeader?9:0))!=0x664C6143) //"fLaC"
    {
        File__Tags_Helper::Finish("Flac");
        return false;
    }

    //All should be OK...
    return true;
}

//---------------------------------------------------------------------------
void File_Flac::FileHeader_Parse()
{
    //Parsing
    if (VorbisHeader)
    {
        Skip_B1(                                                "Signature");
        Skip_Local(4,                                           "Signature");
        Skip_B1(                                                "Major version");
        Skip_B1(                                                "Minor version");
        Skip_B2(                                                "Number of header");
    }
    Skip_C4(                                                    "Signature");
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Flac::Header_Parse()
{
    //Parsing
    int32u Length;
    int8u BLOCK_TYPE;
    BS_Begin();
    Get_SB (   Last_metadata_block,                             "Last-metadata-block");
    Get_S1 (7, BLOCK_TYPE,                                      "BLOCK_TYPE");
    BS_End();
    Get_B3 (Length,                                             "Length");

    //Filling
    Header_Fill_Code(BLOCK_TYPE, Ztring().From_CC1(BLOCK_TYPE));
    Header_Fill_Size(Element_Offset+Length);
}

//---------------------------------------------------------------------------
void File_Flac::Data_Parse()
{
    #define CASE_INFO(_NAME) \
        case Flac::_NAME : Element_Info1(#_NAME); _NAME(); break;

    //Parsing
    switch ((int16u)Element_Code)
    {
        CASE_INFO(STREAMINFO);
        CASE_INFO(PADDING);
        CASE_INFO(APPLICATION);
        CASE_INFO(SEEKTABLE);
        CASE_INFO(VORBIS_COMMENT);
        CASE_INFO(CUESHEET);
        CASE_INFO(PICTURE);
        default : Skip_XX(Element_Size,                         "Data");
    }

    if (Last_metadata_block)
    {
        if (!IsSub)
            Fill(Stream_Audio, 0, Audio_StreamSize, File_Size-(File_Offset+Buffer_Offset+Element_Size));

    if (Retrieve(Stream_Audio, 0, Audio_ChannelPositions).empty() && Retrieve(Stream_Audio, 0, Audio_ChannelPositions_String2).empty())
    {
        int32u t = 0;
        int32s c = Retrieve(Stream_Audio, 0, Audio_Channel_s_).To_int32s();
        if (c==1) t=4;
        else if (c==2) t=3;
        else if (c==3) t=7;
        else if (c==4) t=1539;
        else if (c==5) t=1543;
        else if (c==6) t=1551;
        else if (c==7) t=1807;
        else if (c==8) t=1599;
        if (t) {
            Fill(Stream_Audio, 0, Audio_ChannelPositions, ExtensibleWave_ChannelMask(t));
            Fill(Stream_Audio, 0, Audio_ChannelPositions_String2, ExtensibleWave_ChannelMask2(t));
        }
    }

        //No more need data
        File__Tags_Helper::Finish("Flac");
    }
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Flac::STREAMINFO()
{
    //Parsing
    int64u Samples;
    int32u FrameSize_Min, FrameSize_Max, SampleRate;
    int8u  Channels, BitPerSample;
    Skip_B2(                                                    "BlockSize_Min"); //The minimum block size (in samples) used in the stream.
    Skip_B2(                                                    "BlockSize_Max"); //The maximum block size (in samples) used in the stream. (Minimum blocksize == maximum blocksize) implies a fixed-blocksize stream.
    Get_B3 (    FrameSize_Min,                                  "FrameSize_Min"); //The minimum frame size (in bytes) used in the stream. May be 0 to imply the value is not known.
    Get_B3 (    FrameSize_Max,                                  "FrameSize_Max"); //The maximum frame size (in bytes) used in the stream. May be 0 to imply the value is not known.
    BS_Begin();
    Get_S3 (20, SampleRate,                                     "SampleRate"); //Sample rate in Hz. Though 20 bits are available, the maximum sample rate is limited by the structure of frame headers to 1048570Hz. Also, a value of 0 is invalid.
    Get_S1 ( 3, Channels,                                       "Channels"); Param_Info2(Channels+1, " channels"); //(number of channels)-1. FLAC supports from 1 to 8 channels
    Get_S1 ( 5, BitPerSample,                                   "BitPerSample"); Param_Info2(BitPerSample+1, " bits"); //(bits per sample)-1. FLAC supports from 4 to 32 bits per sample. Currently the reference encoder and decoders only support up to 24 bits per sample.
    Get_S5 (36, Samples,                                        "Samples");
    BS_End();
    Skip_B16(                                                   "MD5 signature of the unencoded audio data");

    FILLING_BEGIN();
        if (SampleRate==0)
            return;
        File__Tags_Helper::Accept("FLAC");

        File__Tags_Helper::Streams_Fill();

        File__Tags_Helper::Stream_Prepare(Stream_Audio);
        Fill(Stream_Audio, 0, Audio_Format, "FLAC");
        Fill(Stream_Audio, 0, Audio_Codec, "FLAC");
        if (FrameSize_Min==FrameSize_Max && FrameSize_Min!=0 ) // 0 means it is unknown
            Fill(Stream_Audio, 0, Audio_BitRate_Mode, "CBR");
         else
            Fill(Stream_Audio, 0, Audio_BitRate_Mode, "VBR");
        Fill(Stream_Audio, 0, Audio_SamplingRate, SampleRate);
        Fill(Stream_Audio, 0, Audio_Channel_s_, Channels+1);
        Fill(Stream_Audio, 0, Audio_BitDepth, BitPerSample+1);
        if (!IsSub)
            Fill(Stream_Audio, 0, Audio_Duration, Samples*1000/SampleRate);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Flac::APPLICATION()
{
    //Parsing
    Skip_C4(                                                    "Application");
    if (Element_Size>4)
        Skip_XX(Element_Size-4,                                 "(Application specific)");
}

//---------------------------------------------------------------------------
void File_Flac::VORBIS_COMMENT()
{
    //Parsing
    #if defined(MEDIAINFO_VORBISCOM_YES)
        File_VorbisCom VorbisCom;
        VorbisCom.StreamKind_Specific=Stream_Audio;
        Open_Buffer_Init(&VorbisCom);
        Open_Buffer_Continue(&VorbisCom);
        File__Analyze::Finish(&VorbisCom);

        //Specific case: bit depth
        if (!VorbisCom.Retrieve(Stream_Audio, 0, Audio_BitDepth).empty() && VorbisCom.Retrieve(Stream_Audio, 0, Audio_BitDepth).To_int64u()<Retrieve(Stream_Audio, 0, Audio_BitDepth).To_int64u())
        {
            //Bit depth information from tags is the real value, the one from Flac is the count of bits stored
            Fill(Stream_Audio, 0, Audio_BitDepth_Stored, Retrieve(Stream_Audio, 0, Audio_BitDepth));
            Fill(Stream_Audio, 0, Audio_BitDepth, VorbisCom.Retrieve(Stream_Audio, 0, Audio_BitDepth), true);
            VorbisCom.Clear(Stream_Audio, 0, Audio_BitDepth);
        }

        Merge(VorbisCom, Stream_General,  0, 0);
        Merge(VorbisCom, Stream_Audio,    0, 0);
        Merge(VorbisCom, Stream_Menu,     0, 0);
    #else
        Skip_XX(Element_Offset,                                 "Data");
    #endif
}

//---------------------------------------------------------------------------
void File_Flac::PICTURE()
{
    //Parsing
    int32u PictureType, MimeType_Size, Description_Size, Data_Size;
    Ztring MimeType, Description;
    Get_B4 (PictureType,                                        "Picture type"); Element_Info1(Id3v2_PictureType((int8u)PictureType));
    Get_B4 (MimeType_Size,                                      "MIME type size");
    Get_Local(MimeType_Size, MimeType,                          "MIME type");
    Get_B4 (Description_Size,                                   "Description size");
    Get_UTF8(Description_Size, Description,                     "Description");
    Skip_B4(                                                    "Width");
    Skip_B4(                                                    "Height");
    Skip_B4(                                                    "Color depth");
    Skip_B4(                                                    "Number of colors used");
    Get_B4 (Data_Size,                                          "Data size");
    if (Element_Offset+Data_Size>Element_Size)
        return; //There is a problem

    //Filling
    Fill(Stream_General, 0, General_Cover, "Yes");
    Fill(Stream_General, 0, General_Cover_Description, Description);
    Fill(Stream_General, 0, General_Cover_Type, Id3v2_PictureType((int8u)PictureType));
    Fill(Stream_General, 0, General_Cover_Mime, MimeType);
    #if MEDIAINFO_ADVANCED
        if (MediaInfoLib::Config.Flags1_Get(Flags_Cover_Data_base64))
        {
            std::string Data_Raw((const char*)(Buffer+(size_t)(Buffer_Offset+Element_Offset)), Data_Size);
            std::string Data_Base64(Base64::encode(Data_Raw));
            Fill(Stream_General, 0, General_Cover_Data, Data_Base64);
        }
    #endif //MEDIAINFO_ADVANCED

    Skip_XX(Data_Size,                                          "Data");
    if (Element_Offset<Element_Size)
        Skip_XX(Element_Size-Element_Offset,                    "?");
}

} //NameSpace

#endif //MEDIAINFO_FLAC_YES



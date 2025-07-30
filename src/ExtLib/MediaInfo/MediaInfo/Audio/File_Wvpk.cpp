/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Sources : http://www.wavpack.com/file_format.txt
//           https://www.wavpack.com/WavPack5FileFormat.pdf
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
#if defined(MEDIAINFO_WVPK_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Wvpk.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constants
//***************************************************************************

//---------------------------------------------------------------------------
static const int16u Wvpk_Resolution[]=
{
     8,
    16,
    24,
    32,
};

//---------------------------------------------------------------------------
static const int32u Wvpk_SamplingRate[]=
{
      6000,
      8000,
      9600,
     11025,
     12000,
     16000,
     22050,
     24000,
     32000,
     44100,
     48000,
     64000,
     88200,
     96000,
    192000,
         0,
};

//---------------------------------------------------------------------------
static const char* Wvpk_id(int8u ID)
{
    switch (ID)
    {
        case 0x00 : return "could be used to pad WavPack blocks";
        case 0x02 : return "decorrelation terms & deltas";
        case 0x03 : return "initial decorrelation weights";
        case 0x04 : return "decorrelation sample history";
        case 0x05 : return "initial entropy variables";
        case 0x06 : return "entropy variables specific to hybrid mode";
        case 0x07 : return "info needed for hybrid lossless (wvc) mode";
        case 0x08 : return "specific info for floating point decode";
        case 0x09 : return "specific info for decoding integers > 24 bits";
        case 0x0A : return "normal compressed audio bitstream (wv file)";
        case 0x0B : return "correction file bitstream (wvc file)";
        case 0x0C : return "special extended bitstream for floating point data or integers > 24 bit";
        case 0x0D : return "contains channel count and channel_mask";
        case 0x0E : return "contains compressed DSD audio"; // Ver 5.0+
        // ids from here are "optional" so decoders should skip them if they don't understand them
        case 0x21 : return "RIFF header for .wav files (before audio)";
        case 0x22 : return "RIFF trailer for .wav files (after audio)";
        case 0x25 : return "some encoding details for info purposes";
        case 0x26 : return "16-byte MD5 sum of raw audio data";
        case 0x27 : return "non-standard sampling rate info";
        // added with version 5.0 to handle non-wav files and block checksums
        case 0x23 : return "header for non-wav files";
        case 0x24 : return "trailer for non-wav files";
        case 0x28 : return "target filename extension";
        case 0x29 : return "16-byte MD5 sum of raw audio data with non-wav standard";
        case 0x2A : return "new file configuration stuff";
        case 0x2B : return "identities of non-MS channels";
        case 0x2F : return "2- or 4-byte checksum of entire block";
        default:    return "";
    }
}

//---------------------------------------------------------------------------
extern std::string ExtensibleWave_ChannelMask_ChannelLayout(int32u ChannelMask); //In Multiple/File_Riff_Elements.cpp
                                                                                 
//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Wvpk::File_Wvpk()
:File__Analyze(), File__Tags_Helper()
{
    //File__Tags_Helper
    Base=this;

    //Configuration
    MustSynchronize=true;
    Buffer_TotalBytes_FirstSynched_Max=64*1024;

    //In
    Frame_Count_Valid=2;
    FromMKV=false;
    FromMKV_CodecPrivateParsed=false;

    //Temp - Technical info
    total_samples_FirstFrame=(int32u)-1;
    block_index_FirstFrame=0;
    block_index_LastFrame=0;
    SamplingRate_Index=(int8u)-1;
    SamplingRate_Shift=0;
    SamplingRate=0;
    num_channels=0;
    channel_mask=0;
    mono=false;
    hybrid=false;
    resolution0=false;
    resolution1=false;
    correction=false;
    dsf=false;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Wvpk::Streams_Finish()
{
    //Specific case
    if (FromMKV)
        return;

    //Duration
    if (!SamplingRate && SamplingRate_Index<15)
        SamplingRate=Wvpk_SamplingRate[SamplingRate_Index]<<SamplingRate_Shift;
    if (SamplingRate)
    {
        int64u Samples=(int64u)(block_index_LastFrame+block_samples_LastFrame-block_index_FirstFrame); //Don't forget the last frame with block_samples...
        int64u BitDepth=dsf?1:(Wvpk_Resolution[(resolution1?1:0)*2+(resolution0?1:0)]);
        int64u Duration=Samples*1000/SamplingRate;
        int64u CompressedSize=File_Size-TagsSize;
        int64u UncompressedSize=Duration*(num_channels?num_channels:(mono?1:2))*BitDepth*(static_cast<int64u>(SamplingRate)<<(3*dsf))/8/1000;
        float32 CompressionRatio=((float32)UncompressedSize)/CompressedSize;
        Fill(Stream_Audio, 0, Audio_StreamSize, CompressedSize, 3, true);
        Fill(Stream_Audio, 0, Audio_Duration, Duration, 10, true);
        Fill(Stream_Audio, 0, Audio_Compression_Ratio, CompressionRatio, 3, true);
    }

    File__Tags_Helper::Streams_Finish();
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Wvpk::FileHeader_Begin()
{
    //Element_Size
    if (Buffer_Size<2)
        return false; //Must wait for more data

    if (CC2(Buffer)==0x4D5A) //"MZ"
    {
        File__Tags_Helper::Reject("WavPack");
        return false; //Executable with WavPack data are currently not well supported --> It is preferable to set them as executable
    }

    //All should be OK...
    return true;
}

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Wvpk::Synchronize()
{
    //Specific case
    if (FromMKV)
        return true;

    //Tags
    bool Tag_Found;
    if (!File__Tags_Helper::Synchronize(Tag_Found))
        return false;
    if (Tag_Found)
        return true;

    //Synchronizing
    while (Buffer_Offset+4<=Buffer_Size)
    {
        while (Buffer_Offset+4<=Buffer_Size && (Buffer[Buffer_Offset  ]!=0x77
                                             || Buffer[Buffer_Offset+1]!=0x76
                                             || Buffer[Buffer_Offset+2]!=0x70
                                             || Buffer[Buffer_Offset+3]!=0x6B)) //"wvpk"
            Buffer_Offset++;

        if (Buffer_Offset+4<=Buffer_Size)//Testing if size is coherant
        {
            //Testing next start, to be sure
            size_t Size=LittleEndian2int32u(Buffer+Buffer_Offset+4)+8;
            if (Buffer_Offset+Size+4>Buffer_Size)
                return false; //Need more data

            //Testing
            if (Buffer[Buffer_Offset+Size  ]!=0x77
             || Buffer[Buffer_Offset+Size+1]!=0x76
             || Buffer[Buffer_Offset+Size+2]!=0x70
             || Buffer[Buffer_Offset+Size+3]!=0x6B) //"wvpk"
                Buffer_Offset++;
            else
                break; //while()
        }
    }

    //Parsing last bytes if needed
    if (Buffer_Offset+4>Buffer_Size)
    {
        if (Buffer_Offset+3==Buffer_Size && CC3(Buffer+Buffer_Offset)!=0x777670)   //"wvp"
            Buffer_Offset++;
        if (Buffer_Offset+2==Buffer_Size && CC2(Buffer+Buffer_Offset)!=0x7776)     //"wv"
            Buffer_Offset++;
        if (Buffer_Offset+1==Buffer_Size && CC1(Buffer+Buffer_Offset)!=0x77)       //"w"
            Buffer_Offset++;
        return false;
    }

    //Synched is OK
    return true;
}

//---------------------------------------------------------------------------
bool File_Wvpk::Synched_Test()
{
    //Specific case
    if (FromMKV)
        return true;

    //Tags
    if (!File__Tags_Helper::Synched_Test())
        return false;

    //Must have enough buffer for having header
    if (Buffer_Offset+3>Buffer_Size)
        return false;

    //Quick test of synchro
    if (Buffer[Buffer_Offset  ]!=0x77
     || Buffer[Buffer_Offset+1]!=0x76
     || Buffer[Buffer_Offset+2]!=0x70
     || Buffer[Buffer_Offset+3]!=0x6B) //"wvpk"
        Synched=false;

    //We continue
    return true;
}

//***************************************************************************
// Format
//***************************************************************************

//---------------------------------------------------------------------------
void File_Wvpk::Read_Buffer_Continue()
{
    //Tags
    if (!FromMKV)
        File__Tags_Helper::Read_Buffer_Continue();
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Wvpk::Header_Parse()
{
    //Specific cases
    if (FromMKV)
    {
        Header_Fill_Size(Element_Size);
        Header_Fill_Code(0, "Block");
        return;
    }

    //Parsing
    int32u ckSize;
    Skip_C4(                                                    "ckID");
    Get_L4 (ckSize,                                             "ckSize");

    //Filling
    Header_Fill_Size(8+ckSize);
    Header_Fill_Code(0, "Block");
}

//---------------------------------------------------------------------------
void File_Wvpk::Data_Parse()
{
    //Specific
    if (FromMKV && !FromMKV_CodecPrivateParsed)
    {
        //Parsing
        Get_L2 (version,                                        "version");

        FILLING_BEGIN();
        FromMKV_CodecPrivateParsed=true;
        FILLING_END();
        return;
    }

    //Counting
    Frame_Count++;

    //Parsing
    Element_Begin1("Block Header");
    if (!FromMKV)
        Get_L2 (version,                                        "version");
    if (version/0x100==0x4)
    {
        while (Element_Offset<Element_Size)
        {
            int32u total_samples=(int32u)-1, block_index=(int32u)-1, block_samples=0, flags, blocksize=(int32u)-1;
            bool initial_block=true, final_block=true;
            if (!FromMKV)
            {
                Skip_L1(                                            "track_no");
                Skip_L1(                                            "index_no");
                Get_L4 (total_samples,                              "total_samples");
                Get_L4 (block_index,                                "block_index");
            }
            if (!FromMKV || Element_Offset==0)
                Get_L4 (block_samples,                              "block_samples");
            if (block_samples!=0) //empty frames have other values empty
            {
                if (!FromMKV)
                {
                    if (block_index==0) //Only the frame with block_index==0
                        total_samples_FirstFrame=total_samples; //Note: total_samples is not trustable for a cutted file
                    if (Frame_Count==1)
                        block_index_FirstFrame=block_index; //Save the block_index of the first block
                    block_index_LastFrame=block_index;
                    block_samples_LastFrame=block_samples;
                }
                Get_L4 (flags,                                      "flags");
                    Get_Flags (flags,  0, resolution0,              "resolution0");
                    Get_Flags (flags,  1, resolution1,              "resolution1");
                    Get_Flags (flags,  2, mono,                     "mono");
                    Get_Flags (flags,  3, hybrid,                   "hybrid");
                    Get_Flags (flags,  4, joint_stereo,             "joint stereo");
                    Get_Flags (flags,  5, cross_channel_decorrelation, "cross-channel decorrelation");
                    Skip_Flags(flags,  6,                           "hybrid noise shaping");
                    Skip_Flags(flags,  7,                           "floating point data");
                    Skip_Flags(flags,  8,                           "extended size integers");
                    Skip_Flags(flags,  9,                           "hybrid mode parameters control bitrate");
                    Skip_Flags(flags, 10,                           "hybrid noise balanced between channels");
                    Get_Flags (flags, 11, initial_block,            "initial block in sequence");
                    Get_Flags (flags, 12, final_block,              "final block in sequence");
                    Skip_Flags(flags, 13,                           "amount of data left-shift after decode");
                    Skip_Flags(flags, 14,                           "amount of data left-shift after decode");
                    Skip_Flags(flags, 15,                           "amount of data left-shift after decode");
                    Skip_Flags(flags, 16,                           "amount of data left-shift after decode");
                    Skip_Flags(flags, 17,                           "amount of data left-shift after decode");
                    Skip_Flags(flags, 18,                           "maximum magnitude of decoded data");
                    Skip_Flags(flags, 19,                           "maximum magnitude of decoded data");
                    Skip_Flags(flags, 20,                           "maximum magnitude of decoded data");
                    Skip_Flags(flags, 21,                           "maximum magnitude of decoded data");
                    Skip_Flags(flags, 22,                           "maximum magnitude of decoded data");
                    Skip_Flags(flags, 23,                           "sampling rate");
                    Skip_Flags(flags, 24,                           "sampling rate");
                    Skip_Flags(flags, 25,                           "sampling rate");
                    Skip_Flags(flags, 26,                           "sampling rate"); SamplingRate_Index=(int8u)(((flags>>23)&0xF)); Param_Info1(Wvpk_SamplingRate[SamplingRate_Index]);
                    Skip_Flags(flags, 27,                           "reserved");
                    Skip_Flags(flags, 28,                           "block contains checksum in last 2 or 4 bytes"); // Ver 5.0+
                    Skip_Flags(flags, 29,                           "use IIR for negative hybrid noise shaping");
                    Skip_Flags(flags, 30,                           "false stereo");
                    Get_Flags (flags, 31, dsf,                      "dsf"); // Ver 5.0+
            }
            else
            {
                Skip_L4(                                            "flags (empty)");

                //Counting
                Frame_Count--; //This is not a real frame
            }
            Skip_L4(                                                "crc");
            Element_End0();

            int64u End=Element_Size;
            if (FromMKV && !(initial_block && final_block))
            {
                Get_L4 (blocksize,                                  "blocksize");
                End=Element_Offset+blocksize;
                if (End>=Element_Size)
                    End=Element_Size;
            }

            //Sub-block
            int8u id;
            while (Element_Offset<End)
            {
                Element_Begin0();
                int32u word_size;
                bool large, odd_size;
                BS_Begin();
                Get_SB (large,                                      "large");
                Get_SB (odd_size,                                   "odd_size");
                Get_S1 (6, id,                                      "id"); Element_Info1(Wvpk_id(id));
                BS_End();
                if (large)
                {
                    Get_L3 (word_size,                              "word_size");
                }
                else
                {
                    int8u word_size1;
                    Get_L1 (word_size1,                             "word_size");
                    word_size=word_size1;
                }
                if (word_size==0 && odd_size)
                    Size=0; //Problem!
                else
                    Size=word_size*2-(odd_size?1:0);
                Element_Name(Ztring().From_CC1(id));
                switch (id)
                {
                    case 0x07 : id_07(); break;
                    case 0x0D : id_0D(); break;
                    case 0x0E : id_0E(); break;
                    case 0x25 : id_25(); break;
                    case 0x26 : id_26(); break;
                    case 0x27 : id_27(); break;
                    case 0x29 : id_29(); break;
                    default   : if (word_size)
                                    Skip_XX(Size,                   "data");
                }
                if (odd_size)
                    Skip_XX(1,                                      "padding");
                Element_End0();
            }
        }
    }

    //Filling
    if (!Status[IsAccepted])
    {
        if (File_Offset+Buffer_Offset+Element_Size==File_Size)
            Frame_Count_Valid=Frame_Count;
        if (Frame_Count>=Frame_Count_Valid)
        {
            File__Tags_Helper::Accept("WavPack");
            Data_Parse_Fill();
        }
    }
}

//---------------------------------------------------------------------------
void File_Wvpk::Data_Parse_Fill()
{
    //Filling
    File__Tags_Helper::Stream_Prepare(Stream_Audio);
    Fill(Stream_Audio, 0, Audio_Format, "WavPack");
    Ztring Version_Minor=Ztring::ToZtring(version%0x100);
    if (Version_Minor.size()==1)
        Version_Minor.insert(0, 1, __T('0'));
    Fill(Stream_Audio, 0, Audio_Format_Profile, Ztring::ToZtring(version/0x100)+__T('.')+Version_Minor);
    Fill(Stream_Audio, 0, Audio_Codec, "Wavpack");
    if (!dsf)
        Fill(Stream_Audio, 0, Audio_BitDepth, Wvpk_Resolution[(resolution1?1:0)*2+(resolution0?1:0)]);
    Fill(Stream_Audio, StreamPos_Last, Audio_Channel_s_, num_channels?num_channels:(mono?1:2));
    if (channel_mask)
    {
        Ztring Channels_Positions, Channels_Positions2;
        if (channel_mask&0x00C7)
        {
            int8u Count=0;
            Channels_Positions+=__T("Front:");
            if (channel_mask&0x0001)
            {
                Channels_Positions+=__T(" L");
                Count++;
            }
            if (channel_mask&0x0004)
            {
                Channels_Positions+=__T(" C");
                Count++;
            }
            if (channel_mask&0x0040)
            {
                Channels_Positions+=__T(" C");
                Count++;
            }
            if (channel_mask&0x0080)
            {
                Channels_Positions+=__T(" C");
                Count++;
            }
            if (channel_mask&0x0002)
            {
                Channels_Positions+=__T(" R");
                Count++;
            }
            Channels_Positions2+=Ztring::ToZtring(Count);
        }
        if (channel_mask&0x0600)
        {
            int8u Count=0;
            if (!Channels_Positions.empty())
                Channels_Positions+=__T(", ");
            Channels_Positions+=__T("Side:");
            if (channel_mask&0x0200)
            {
                Channels_Positions+=__T(" L");
                Count++;
            }
            if (channel_mask&0x0400)
            {
                Channels_Positions+=__T(" R");
                Count++;
            }
            Channels_Positions2+=__T('.')+Ztring::ToZtring(Count);
        }
        else
            Channels_Positions2+=__T("/0");
        if (channel_mask&0x0130)
        {
            int8u Count=0;
            if (!Channels_Positions.empty())
                Channels_Positions+=__T(", ");
            Channels_Positions+=__T("Back:");
            if (channel_mask&0x0010)
            {
                Channels_Positions+=__T(" L");
                Count++;
            }
            if (channel_mask&0x0100)
            {
                Channels_Positions+=__T(" C");
                Count++;
            }
            if (channel_mask&0x0020)
            {
                Channels_Positions+=__T(" R");
                Count++;
            }
            Channels_Positions2+=__T('/')+Ztring::ToZtring(Count);
        }
        else
            Channels_Positions2+=__T("/0");
        if (channel_mask&0x0008)
        {
            if (!Channels_Positions.empty())
                Channels_Positions+=__T(", ");
            Channels_Positions+=__T("LFE");
            Channels_Positions2+=__T(".1");
        }
        Fill(Stream_Audio, 0, Audio_ChannelPositions, Channels_Positions);
        Fill(Stream_Audio, 0, Audio_ChannelPositions_String2, Channels_Positions2);
        Fill(Stream_Audio, 0, Audio_ChannelLayout, ExtensibleWave_ChannelMask_ChannelLayout(channel_mask));
    }

    if (!FromMKV && SamplingRate_Index<15)
    {
        Fill(Stream_Audio, StreamPos_Last, Audio_SamplingRate, (Wvpk_SamplingRate[SamplingRate_Index]<<SamplingRate_Shift)<<(3*dsf));
        if (total_samples_FirstFrame!=(int32u)-1) //--> this is a valid value
            Fill(Stream_Audio, 0, Audio_Duration, ((int64u)total_samples_FirstFrame)*1000/((int64u)Wvpk_SamplingRate[SamplingRate_Index]<<SamplingRate_Shift));
    }
    Fill(Stream_Audio, 0, Audio_Encoded_Library_Settings, Encoded_Library_Settings);
    Fill(Stream_Audio, 0, Audio_BitRate_Mode, "VBR");
    const char* Mode=(hybrid && !correction)?"Lossy":"Lossless";
    Fill(Stream_Audio, 0, Audio_Compression_Mode, Mode);
    Fill(Stream_Audio, 0, Audio_Codec_Settings, Mode);
    if (dsf)
    {
        Fill(Stream_Audio, 0, Audio_Format_Settings_Mode, "DSD");
        Fill(Stream_Audio, 0, Audio_Format_Settings, "DSD");
    }
    if (correction)
        Fill(Stream_Audio, 0, Audio_Format_AdditionalFeatures, "Correction");

    //No more need data
    if (File_Size!=(int64u)-1 && File_Size-(File_Offset+Buffer_Offset)>=2*512*1024)
        File__Tags_Helper::GoToFromEnd(512*1024, "WavPack");
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Wvpk::id_07()
{
    //Parsing
    Skip_XX(Size,                                               "Data");

    correction=true;
}

//---------------------------------------------------------------------------
void File_Wvpk::id_0D()
{
    //Parsing
    if (Size > 7)
    {
        //No backward compatibility guaranteed, skipping all
        Skip_XX(Size,                                           "(Not parsed)");
        return;
    }
    int8u num_chans_low8;
    Get_L1 (num_chans_low8,                                     "num_channels");
    num_channels = num_chans_low8;
    if (Size >= 6)
    {
        int8u num_chans_high4;
        Skip_L1(                                                "num_streams");
        BS_Begin();
        Skip_S1(4,                                              "reserved");
        Get_S1 (4, num_chans_high4,                             "num_channels (hi)");
        BS_End();
        num_channels |= (num_chans_high4 << 8);
        num_channels++;
        Param_Info2(num_channels, " channels");
    }
    switch (Size)
    {
        case 1 :
                    break;
        case 2 :
                    {
                    int8u channel_mask_1;
                    Get_L1 (channel_mask_1,                     "channel_mask");
                    channel_mask=channel_mask_1;
                    }
                    break;
        case 3 :
                    {
                    int16u channel_mask_2;
                    Get_L2 (channel_mask_2,                     "channel_mask");
                    channel_mask=channel_mask_2;
                    }
                    break;
        case 4 :
        case 6 :
                    Get_L3 (channel_mask,                       "channel_mask");
                    break;
        default:
                    Get_L4 (channel_mask,                       "channel_mask");
                    break;
    }
}

//---------------------------------------------------------------------------
void File_Wvpk::id_0E()
{
    //Parsing
    int8u Temp;
    Get_L1 (Temp,                                               "framerate multiplier");
    if (Temp<31)
        SamplingRate_Shift=Temp;
    Skip_XX(Size-1,                                             "(Not parsed)");
}

//---------------------------------------------------------------------------
void File_Wvpk::id_26()
{
    int128u MD5Stored;
    Get_B16(MD5Stored,                                          "16-byte MD5 sum of raw audio data");

    FILLING_BEGIN();
    Ztring MD5_PerItem;
    MD5_PerItem.From_UTF8(uint128toString(MD5Stored, 16));
    while (MD5_PerItem.size() < 32)
        MD5_PerItem.insert(MD5_PerItem.begin(), '0'); //Padding with 0, this must be a 32-byte string
    Fill(Stream_Audio, 0, "MD5_Unencoded", MD5_PerItem);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Wvpk::id_25()
{
    //Parsing
    int32u flags;
    int8u  extra=1;
    Get_L3 (flags,                                              "flags");
        Skip_Flags(flags,  0,                                   "");
        Skip_Flags(flags,  1,                                   "fast mode");
        Skip_Flags(flags,  2,                                   "");
        Skip_Flags(flags,  3,                                   "high quality mode");

        Skip_Flags(flags,  4,                                   "very high quality mode");
        Skip_Flags(flags,  5,                                   "bitrate is kbps, not bits/sample");
        Skip_Flags(flags,  6,                                   "automatic noise shaping");
        Skip_Flags(flags,  7,                                   "shaping mode specified");

        Skip_Flags(flags,  8,                                   "joint-stereo mode specified");
        Skip_Flags(flags,  9,                                   "dynamic noise shaping");
        Skip_Flags(flags, 10,                                   "create executable");
        Skip_Flags(flags, 11,                                   "create correction file");

        Skip_Flags(flags, 12,                                   "maximize bybrid compression");
        Skip_Flags(flags, 13,                                   "");
        Skip_Flags(flags, 14,                                   "");
        Skip_Flags(flags, 15,                                   "calc noise in hybrid mode");

        Skip_Flags(flags, 16,                                   "lossy mode");
        Skip_Flags(flags, 17,                                   "extra processing mode");
        Skip_Flags(flags, 18,                                   "no wvx stream w/ floats & big ints");
        Skip_Flags(flags, 19,                                   "store MD5 signature");

        Skip_Flags(flags, 20,                                   "merge blocks of equal redundancy (for lossyWAV)");
        Skip_Flags(flags, 21,                                   "");
        Skip_Flags(flags, 22,                                   "");
        Skip_Flags(flags, 23,                                   "optimize for mono streams posing as stereo");
    if (flags&0x20000 && Size>=4)
    {
        Get_L1(extra,                                           "extra");
    }

    if ((flags&0x20000 && Size>4) || (!(flags&0x20000) && Size>3))
        Skip_XX(Size-3-(flags&0x20000?1:0),                     "unknown");

    //Filling
    if (flags&0x000001)
        Encoded_Library_Settings+=__T(" -?");
    if (flags&0x000002)
        Encoded_Library_Settings+=__T(" -f");
    if (flags&0x000004)
        Encoded_Library_Settings+=__T(" -?");
    if (flags&0x000008)
        Encoded_Library_Settings+=__T(" -h");
    if (flags&0x000010)
        Encoded_Library_Settings+=__T(" -hh");
    if (flags&0x000020)
        Encoded_Library_Settings+=__T(" -?(bitrate is kbps, not bits/sample)");
    if (flags&0x000040)
        Encoded_Library_Settings+=__T(" -?(automatic noise shaping)");
    if (flags&0x000080)
        Encoded_Library_Settings+=__T(" -sn");
    if (flags&0x000100)
        Encoded_Library_Settings+=__T(" -jn");
    if (flags&0x000200)
        Encoded_Library_Settings+=__T(" -use-dns");
    if (flags&0x000400)
        Encoded_Library_Settings+=__T(" -e");
    if (flags&0x000800)
        Encoded_Library_Settings+=__T(" -c");
    if (flags&0x001000)
        Encoded_Library_Settings+=__T(" -cc");
    if (flags&0x002000)
        Encoded_Library_Settings+=__T(" -?");
    if (flags&0x004000)
        Encoded_Library_Settings+=__T(" -?");
    if (flags&0x008000)
        Encoded_Library_Settings+=__T(" -n");
    if (flags&0x010000)
        Encoded_Library_Settings+=__T(" -?(lossy mode)");
    if (flags&0x020000)
    {
        Encoded_Library_Settings+=__T(" -x");
        if (extra)
            Encoded_Library_Settings+=Ztring::ToZtring(extra);
    }
    if (flags&0x04000)
        Encoded_Library_Settings+=__T(" -?");
    if (flags&0x080000)
        Encoded_Library_Settings+=__T(" -m");
    if (flags&0x100000)
        Encoded_Library_Settings+=__T(" --merge-blocks");
    if (flags&0x200000)
        Encoded_Library_Settings+=__T(" -?");
    if (flags&0x400000)
        Encoded_Library_Settings+=__T(" -?");
    if (flags&0x800000)
        Encoded_Library_Settings+=__T(" --optimize-mono");
    if (!Encoded_Library_Settings.empty())
        Encoded_Library_Settings.erase(Encoded_Library_Settings.begin());
}

//---------------------------------------------------------------------------
void File_Wvpk::id_27()
{
    //Parsing
    Get_L3 (SamplingRate,                                       "data");

    if (SamplingRate)
        Fill(Stream_Audio, StreamPos_Last, Audio_SamplingRate, SamplingRate, 10, true);
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_WVPK_YES

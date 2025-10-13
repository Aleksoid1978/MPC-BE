/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Contributor: Lionel Duchateau, kurtnoise@free.fr
//
// Note : the buffer must be given in ONE call
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
#if defined(MEDIAINFO_OPUS_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Opus.h"
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
static const size_t Opus_ChannelLayout_Max=8;
static const char*  Opus_ChannelPositions[Opus_ChannelLayout_Max]=
{
    "Front: C",
    "Front: L R",
    "Front: L C R",
    "Front: L R,   Rear: L R",
    "Front: L C R, Back: L R",
    "Front: L C R, Back: L R, LFE",
    "Front: L C R, Side: L R, Back: C, LFE",
    "Front: L C R, Side: L R, Back: L R, LFE",
};

//---------------------------------------------------------------------------
static const char*  Opus_ChannelPositions2[Opus_ChannelLayout_Max]=
{
    "1/0/0",
    "2/0/0",
    "3/0/0",
    "2/0/2",
    "3/0/2",
    "3/0/2.1",
    "3/2/1.1",
    "3/2/2.1",
};

//---------------------------------------------------------------------------
static const char*  Opus_ChannelLayout[Opus_ChannelLayout_Max]=
{
    "M",
    "L R",
    "L R C",
    "L R BL BR",
    "L R BL BR LFE",
    "L R C BL BR LFE",
    "L R C SL SR BC LFE",
    "L R C SL SR BL BR LFE",
};

//---------------------------------------------------------------------------
static const string Opus_Config(int8u config)
{
    struct ConfigDescription {
        const int8u start;
        const int8u end;
        const char* mode;
        const char* bandwidth;
        const std::vector<int8u> frame_sizes_ms;
    };

    const ConfigDescription config_table[] = {
        {0, 3, "SILK-only", "NB", {10, 20, 40, 60}},
        {4, 7, "SILK-only", "MB", {10, 20, 40, 60}},
        {8, 11, "SILK-only", "WB", {10, 20, 40, 60}},
        {12, 13, "Hybrid", "SWB", {10, 20}},
        {14, 15, "Hybrid", "FB", {10, 20}},
        {16, 19, "CELT-only", "NB", {2, 5, 10, 20}},
        {20, 23, "CELT-only", "WB", {2, 5, 10, 20}},
        {24, 27, "CELT-only", "SWB", {2, 5, 10, 20}},
        {28, 31, "CELT-only", "FB", {2, 5, 10, 20}}
    };

    if (config > 31)
        return "";

    for (const auto& config_it : config_table) {
        if (config >= config_it.start && config <= config_it.end) {
            int8u index = config - config_it.start;
            if (index < config_it.frame_sizes_ms.size())
                return string(config_it.mode) + ", " + config_it.bandwidth + ", " + to_string(config_it.frame_sizes_ms[index]) + " ms";
        }
    }
    return "";
}

//---------------------------------------------------------------------------
static const char* Opus_c[4] =
{
    "1 frame in the packet",
    "2 frames in the packet, each with equal compressed size",
    "2 frames in the packet, with different compressed sizes",
    "an arbitrary number of frames in the packet"
};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Opus::File_Opus()
:File__Analyze()
{
    //Internal
    Identification_Done=false;
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Opus::Header_Parse()
{
    //Filling
    Header_Fill_Code(0, "Opus");
    Header_Fill_Size(Element_Size);
}

//---------------------------------------------------------------------------
void File_Opus::Data_Parse()
{
    //Parsing
    if (Identification_Done)
        Stream();
    else
        Identification();
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Opus::Identification()
{
    Element_Name("Identification");

    //Parsing
    Ztring opus_version, Channels_Positions;
    int8u Opus_version_id, ch_count, ch_map;
    int16u preskip;
    int32u sample_rate;
    if (!(FromMP4 || FromIamf))
        Get_UTF8(8, opus_version,                               "opus_codec_id");
    Get_L1 (Opus_version_id,                                    "opus_version_id");
    Get_L1 (ch_count,                                           "channel_count");
    Get_X2 (preskip,                                            "preskip");
    Get_X4 (sample_rate,                                        "rate");
    Skip_X2(                                                    "ouput_gain");
    Get_L1 (ch_map,                                             "channel_map");
    if (FromIamf && ch_map) {
        Identification_Done = true;
        Trusted_IsNot("Opus ID Header does not conform with ChannelMappingFamily = 0 when in IAMF");
        return;
    }
    if (ch_map)
    {
        Skip_L1(                                                "Stream count (N)");
        Skip_L1(                                                "Two-channel stream count (M)");
        for (int8u Pos=0; Pos<ch_count; Pos++)
            Skip_L1(                                            "Channel mapping");
    }
    if (Element_Offset<Element_Size)
        Skip_XX(Element_Size-Element_Offset,                    "unknown");

    //Filling
    FILLING_BEGIN();
        Accept("Opus");

        Stream_Prepare(Stream_Audio);
        Fill(Stream_Audio, 0, Audio_Format, "Opus");
        Fill(Stream_Audio, 0, Audio_Codec, "Opus");

        if (!opus_version.empty() || FromMP4 || FromIamf)
        {
            Fill(Stream_Audio, 0, Audio_SamplingRate, sample_rate?sample_rate:48000);
            if (!FromIamf)
                Fill(Stream_Audio, 0, Audio_Channel_s_, ch_count);
        }

        if (!FromIamf) {
            //      0       Mono, L/R stereo                    [RFC7845, Section 5.1.1.1][RFC8486, Section 5]
            //      1       1 - 8 channel surround              [RFC7845, Section 5.1.1.2][RFC8486, Section 5]
            //      2       Ambisonics as individual channels   [RFC8486, Section 3.1]
            //      3       Ambisonics with demixing matrix     [RFC8486, Section 3.2]
            //    4 - 239   Unassigned
            //  240 - 254   Experimental use                    [RFC8486, Section 6]
            //     255      Discrete channels                   [RFC7845, Section 5.1.1.3][RFC8486, Section 5]
            switch (ch_map)
            {
            case 0 : // Mono/Stereo
                    if (ch_count>2)
                        break; // Not in spec
                    [[fallthrough]]; // else it is as Vorbis specs, no break
            case 1 : // Vorbis order
                    if (ch_count && ch_count<=Opus_ChannelLayout_Max)
                    {
                    Ztring ChannelPositions; ChannelPositions.From_UTF8(Opus_ChannelPositions[ch_count-1]);
                    Ztring ChannelPositions2; ChannelPositions2.From_UTF8(Opus_ChannelPositions2[ch_count-1]);
                    Ztring ChannelLayout2; ChannelLayout2.From_UTF8(Opus_ChannelLayout[ch_count-1]);
                    if (ChannelPositions!=Retrieve(Stream_Audio, 0, Audio_ChannelPositions))
                        Fill(Stream_Audio, 0, Audio_ChannelPositions, ChannelPositions);
                    if (ChannelPositions2!=Retrieve(Stream_Audio, 0, Audio_ChannelPositions_String2))
                        Fill(Stream_Audio, 0, Audio_ChannelPositions_String2, ChannelPositions2);
                    if (ChannelLayout2!=Retrieve(Stream_Audio, 0, Audio_ChannelLayout))
                        Fill(Stream_Audio, 0, Audio_ChannelLayout, ChannelLayout2);
                    }
                    break;
            default: ; //Unknown
            }
        }

    FILLING_END();

    //Filling
    Identification_Done=true;
}

//---------------------------------------------------------------------------
void File_Opus::Stream()
{
    Element_Name("Stream");

    Element_Begin1("TOC");
    int8u config, c;
    bool s;
    BS_Begin();
    Get_S1(5, config,                                           "config"); Param_Info1(Opus_Config(config));
    Get_SB(s,                                                   "s (Stereo)");
    Get_S1(2, c,                                                "c (Code)"); Param_Info1(Opus_c[c]);
    BS_End();
    Element_End0();

    switch (c) {
    case 0:
        Skip_XX(Element_Size - 1,                               "Compressed frame 1");
        break;
    case 1:
        Skip_XX((Element_Size - 1) / 2,                         "Compressed frame 1");
        Skip_XX((Element_Size - 1) / 2,                         "Compressed frame 2");
        break;
    case 2:
    {
        int8u N1_1{}, N1_2{};
        int16u size{};
        Get_L1(N1_1,                                            "N1");
        if (N1_1 < 252)
            size = N1_1;
        else {
            Get_L1(N1_2,                                        "N1");
            size = N1_2 * 4 + N1_1;
        }
        Skip_XX(size,                                           "Compressed frame 1");
        Skip_XX(Element_Size - Element_Offset,                  "Compressed frame 2");
        break;
    }
    default:
        Skip_XX(Element_Size,                                   "Data");
        break;
    }

    if (!FromIamf)
        Finish("Opus");
}

//---------------------------------------------------------------------------
void File_Opus::Get_X2(int16u& Info, const char* Name)
{
    if (FromMP4 || FromIamf)
        Get_B2(Info, Name);
    else
        Get_L2(Info, Name);
}

//---------------------------------------------------------------------------
void File_Opus::Get_X4(int32u& Info, const char* Name)
{
    if (FromMP4 || FromIamf)
        Get_B4(Info, Name);
    else
        Get_L4(Info, Name);
}

//---------------------------------------------------------------------------
void File_Opus::Skip_X2(const char* Name)
{
    if (FromMP4 || FromIamf)
        Skip_B2(Name);
    else
        Skip_L2(Name);
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_OPUS_YES

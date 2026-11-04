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
#if defined(MEDIAINFO_SMPTEST0337_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_SmpteSt0337.h"
#if defined(MEDIAINFO_AAC_YES)
    #include "MediaInfo/Audio/File_Aac.h"
#endif
#if defined(MEDIAINFO_AC3_YES)
    #include "MediaInfo/Audio/File_Ac3.h"
#endif
#if defined(MEDIAINFO_AC4_YES)
    #include "MediaInfo/Audio/File_Ac4.h"
#endif
#if defined(MEDIAINFO_ADM_YES)
    #include "MediaInfo/Audio/File_Adm.h"
#endif
#if defined(MEDIAINFO_DOLBYE_YES)
    #include "MediaInfo/Audio/File_DolbyE.h"
#endif
#if defined(MEDIAINFO_MPEGA_YES)
    #include "MediaInfo/Audio/File_Mpega.h"
#endif
#if MEDIAINFO_EVENTS
    #include "MediaInfo/MediaInfo_Events.h"
#endif // MEDIAINFO_EVENTS
#if MEDIAINFO_SEEK
    #include "MediaInfo/MediaInfo_Internal.h"
#endif // MEDIAINFO_SEEK
#if defined(MEDIAINFO_ADM_YES)
    #include <zlib.h>
#endif
#include "MediaInfo/File_Unknown.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
static const char* Smpte_St0337_data_type[]= // SMPTE ST 338
{
    "",
    "AC-3",
    "Time stamp",
    "Pause",
    "MPEG Audio",
    "MPEG Audio",
    "MPEG Audio",
    "AAC",
    "MPEG Audio",
    "MPEG Audio",
    "AAC",
    "AAC",
    "",
    "",
    "",
    "",
    "E-AC-3",
    "DTS",
    "WMA",
    "AAC",
    "AAC",
    "E-AC-3",
    "",
    "",
    "AC-4",
    "MPEG-H 3D Audio",
    "Utility",
    "KLV",
    "Dolby E",
    "Captioning",
    "User defined",
    "Extended",
    "ADM",
};

//---------------------------------------------------------------------------
static stream_t Smpte_St0337_data_type_StreamKind[sizeof(Smpte_St0337_data_type)/sizeof(char*)]= // SMPTE 338M
{
    Stream_Max,
    Stream_Audio,
    Stream_Max,
    Stream_Max,
    Stream_Audio,
    Stream_Audio,
    Stream_Audio,
    Stream_Max,
    Stream_Audio,
    Stream_Audio,
    Stream_Max,
    Stream_Max,
    Stream_Max,
    Stream_Max,
    Stream_Max,
    Stream_Max,
    Stream_Max,
    Stream_Max,
    Stream_Max,
    Stream_Max,
    Stream_Max,
    Stream_Max,
    Stream_Max,
    Stream_Max,
    Stream_Max,
    Stream_Max,
    Stream_Max,
    Stream_Menu,
    Stream_Audio,
    Stream_Text,
    Stream_Max,
    Stream_Max,
    Stream_Audio,
};

#if defined(MEDIAINFO_ADM_YES)
static const char* Smpte_St0337_Adm_multiple_chunk_flag[4]=
{
    "Full",
    "First",
    "Intermediate",
    "Last",
};
static const char* Smpte_St0337_Adm_format_type[2]=
{
    "UTF-8",
    "UTF-8 gzip",
};
#endif

#if MEDIAINFO_CONFORMANCE
//---------------------------------------------------------------------------
// Editions of SMPTE ST 337 and associated standards, as in the MxfProfile option
static const char* const Smpte_St0337_Generation_Names[]=
{
    "SMPTE_ST_337_2008", // SMPTE ST 337:2008 (same requirements as SMPTE 337M-2000), SMPTE ST 338:2010, SMPTE ST 340:2008
    "SMPTE_ST_337_2015", // SMPTE ST 337:2015 (extended data type), SMPTE ST 338:2015, SMPTE ST 340:2015
    "SMPTE_ST_337_2016", // SMPTE ST 338:2016
    "SMPTE_ST_337_2019", // SMPTE ST 338:2016 Amendment 1:2019
};
static const char* const Smpte_St0337_Generation_DataTypes[]= // Document defining the data types
{
    "SMPTE ST 338:2010",
    "SMPTE ST 338:2015",
    "SMPTE ST 338:2016",
    "SMPTE ST 338:2016 Amendment 1:2019",
};
static const size_t Smpte_St0337_Generation_Size=sizeof(Smpte_St0337_Generation_Names)/sizeof(*Smpte_St0337_Generation_Names);

//---------------------------------------------------------------------------
// Edition from which each data_type is defined, (int8u)-1 if reserved (SMPTE ST 338)
static const int8u Smpte_St0337_data_type_Generation[32]=
{
    0, // Null data
    0, // AC-3
    0, // Time stamp
    0, // Pause
    0, // MPEG-1 Layer I
    0, // MPEG-1 Layer II or III, MPEG-2 Layer I, II or III without extension
    0, // MPEG-2 Layer I, II or III with extension
    0, // MPEG-2 AAC in ADTS
    (int8u)-1, // Reserved for compatibility with IEC 61937
    (int8u)-1,
    0, // MPEG-4 AAC in ADTS or LATM/LOAS
    0, // MPEG-4 HE-AAC in ADTS or LATM/LOAS
    (int8u)-1, // Reserved for compatibility with IEC 61937
    (int8u)-1,
    (int8u)-1,
    (int8u)-1,
    0, // Enhanced AC-3 (professional applications)
    2, // DTS
    (int8u)-1, // Reserved for compatibility with IEC 61937
    (int8u)-1,
    (int8u)-1,
    (int8u)-1, // Reserved for compatibility with IEC 61937 (Enhanced AC-3 for consumer applications only in SMPTE ST 338:2010)
    (int8u)-1,
    (int8u)-1,
    2, // AC-4
    (int8u)-1, // Reserved
    0, // Utility (V sync)
    0, // SMPTE KLV
    0, // Dolby E
    0, // Captioning
    0, // User defined
    1, // Data type in extended code space (Pe)
};
#endif //MEDIAINFO_CONFORMANCE

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_SmpteSt0337::File_SmpteSt0337()
:File_Pcm_Base()
{
    // Configuration
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_Aes3;
    #endif // MEDIAINFO_EVENTS
    MustSynchronize=true;
    Buffer_TotalBytes_FirstSynched_Max=1024*1024;
    PTS_DTS_Needed=true;

    // In
    Aligned=false;

    // Temp
    FrameRate=0;
    Stream_Bits=0;
    data_type=(int32u)-1;
    GuardBand_Before=0;
    GuardBand_After=0;
    NullPadding_Size=0;

    // Parser
    Parser=NULL;

    // Conformance
    #if MEDIAINFO_CONFORMANCE
        const auto Profiles=','+MediaInfoLib::Config.MxfProfile()+',';
        Conformance_Generation=(int8u)(Smpte_St0337_Generation_Size-1); // Latest edition by default
        Conformance_DataType_IsChecked=false; // Data types may be defined in a later edition
        for (size_t i=0; i<Smpte_St0337_Generation_Size; i++)
            if (Profiles.find(','+string(Smpte_St0337_Generation_Names[i])+',')!=string::npos)
            {
                Conformance_Generation=(int8u)i;
                Conformance_DataType_IsChecked=true;
            }
        Conformance_DataStreamNumbers=0;
        Conformance_Unsynch();
    #endif //MEDIAINFO_CONFORMANCE

    #if MEDIAINFO_SEEK
        Duration_Detected=false;
    #endif // MEDIAINFO_SEEK
}

//---------------------------------------------------------------------------
File_SmpteSt0337::~File_SmpteSt0337()
{
    delete Parser; // Parser=NULL;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_SmpteSt0337::Streams_Accept()
{
    Fill(Stream_General, 0, General_Format, "SMPTE ST 337");
    Fill(Stream_General, 0, General_OverallBitRate_Mode, "CBR");
}

//---------------------------------------------------------------------------
void File_SmpteSt0337::Streams_Fill()
{
    if (Parser && Parser->Status[IsAccepted])
    {
        Fill(Parser);
        Merge(*Parser);

        if (Parser->Count_Get(Stream_Audio))
        {
            FrameRate=Retrieve(Stream_Audio, 0, Audio_FrameRate).To_float64();
            float64 FrameRate_Int=float64_int64s(FrameRate);
            if (FrameRate>=FrameRate_Int/1.0015 && FrameRate<=FrameRate_Int/1.0005)
                FrameRate=FrameRate_Int/1.001;
        }
    }
    else if (data_type<sizeof(Smpte_St0337_data_type_StreamKind)/sizeof(stream_t))
    {
        if (Retrieve(Stream_Audio, 0, Audio_Format).empty() && Smpte_St0337_data_type_StreamKind[data_type]!=Stream_Max)
        {
            Stream_Prepare(Smpte_St0337_data_type_StreamKind[data_type]);
            Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Format), Smpte_St0337_data_type[data_type]);
            Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Codec), Smpte_St0337_data_type[data_type]);
        }
    }

    // Bit rate
    if (FrameRate)
    {
        size_t StartPosToClear=0;
        float64 FrameSize=0;

        if (FrameSizes.size()==1)
        {
            FrameSize=FrameSizes.begin()->first;
        }
        else if (FrameSizes.size()==2 && ((--FrameSizes.end())->first-FrameSizes.begin()->first)*4==BitDepth && FrameSizes.begin()->second*3<=(--FrameSizes.end())->second*2 && (FrameSizes.begin()->second+1)*3>=(--FrameSizes.end())->second*2)
        {
            // Maybe NTSC frame rate and 48 kHz.
            FrameSize=FrameSizes.begin()->first+((float64)BitDepth)/4*3/5; //2x small then 3x big
        }
        else
        {
            int64u FrameSize_Total=0;
            int64u FrameSize_Count=0;
            for (std::map<int64u, int64u>::iterator F=FrameSizes.begin(); F!=FrameSizes.end(); ++F)
            {
                FrameSize_Total+=F->first*F->second;
                FrameSize_Count+=F->second;
            }
            if (FrameSize_Count>=10)
                FrameSize=((float64)FrameSize_Total/FrameSize_Count);
        }

        if (FrameSize)
        {
            float64 BitRate=FrameSize*8*FrameRate;
            float64 BitRate_Theory=BitDepth*2*48000;
            if (BitRate>=BitRate_Theory*0.999 && BitRate<=BitRate_Theory*1.001)
                BitRate=BitRate_Theory;
            Fill(Stream_General, 0, General_OverallBitRate, BitRate, 0, true);
            Fill(Stream_Audio, 0, Audio_BitRate_Encoded, BitRate, 0, true);
            StartPosToClear=1;
        }

        //Underlying encoded bit rate has no meaning
        if (StartPosToClear)
        {
            for (size_t i=StartPosToClear; i<Count_Get(Stream_Audio); i++)
                Fill(Stream_Audio, i, Audio_BitRate_Encoded, 0, 10, true);
        }
        else
        {
            for (size_t i=0; i<Count_Get(Stream_Audio); i++)
                Clear(Stream_Audio, i, Audio_BitRate_Encoded);
        }
    }

    for (size_t Pos=0; Pos<Count_Get(StreamKind_Last); Pos++)
    {
        if (!IsSub || Retrieve_Const(StreamKind_Last, Pos, "Metadata_MuxingMode").empty())
        {
            if (!IsSub && StreamKind_Last==Stream_Audio && Retrieve_Const(StreamKind_Last, Pos, "Format").empty())
            {
                Fill(Stream_Audio, Pos, Audio_Format, "PCM");
                Fill(Stream_Audio, Pos, Audio_Channel_s_, 2);
            }
            if (Endianness=='L' && Retrieve(StreamKind_Last, Pos, "Format_Settings_Endianness")==__T("Little"))
                Endianness='B';
            switch (Endianness)
            {
                case 'B' :
                            Fill(StreamKind_Last, Pos, "Format_Settings", "Big");
                            Fill(StreamKind_Last, Pos, "Format_Settings_Endianness", "Big", Unlimited, true, true);
                            break;
                case 'L' :
                            Fill(StreamKind_Last, Pos, "Format_Settings", "Little");
                            Fill(StreamKind_Last, Pos, "Format_Settings_Endianness", "Little", Unlimited, true, true);
                            break;
                default  : ;
            }
            Fill(StreamKind_Last, Pos, "Format_Settings_Mode", BitDepth);
            if (Retrieve(StreamKind_Last, Pos, Fill_Parameter(StreamKind_Last, Generic_BitDepth)).empty())
                Fill(StreamKind_Last, Pos, Fill_Parameter(StreamKind_Last, Generic_BitDepth), Stream_Bits);
            if (Retrieve(StreamKind_Last, Pos, Fill_Parameter(StreamKind_Last, Generic_BitRate_Mode))!=__T("CBR"))
                Fill(StreamKind_Last, Pos, Fill_Parameter(StreamKind_Last, Generic_BitRate_Mode), "CBR");
        }

        if (IsSub && Retrieve_Const(StreamKind_Last, Pos, "Metadata_MuxingMode").empty())
            Fill(StreamKind_Last, Pos, "MuxingMode", "SMPTE ST 337");
    }
}

//---------------------------------------------------------------------------
void File_SmpteSt0337::Streams_Finish()
{
    if (Parser && Parser->Status[IsAccepted])
    {
        Finish(Parser);
        for (size_t Pos=0; Pos<Count_Get(Stream_Audio); Pos++)
        {
            if (!Parser->Retrieve(Stream_Audio, Pos, Audio_Duration).empty())
                Fill(Stream_Audio, Pos, Audio_Duration, Parser->Retrieve(Stream_Audio, Pos, Audio_Duration), true);
            if (!Parser->Retrieve(Stream_Audio, Pos, Audio_FrameCount).empty())
                Fill(Stream_Audio, Pos, Audio_FrameCount, Parser->Retrieve(Stream_Audio, Pos, Audio_FrameCount), true);

            if (!IsSub)
            {
                if (Retrieve(StreamKind_Last, Pos, Fill_Parameter(Stream_Audio, Generic_FrameCount)).empty() && File_Size!=(int64u)-1 && FrameSizes.size()==1)
                    Fill(StreamKind_Last, Pos, Fill_Parameter(StreamKind_Last, Generic_FrameCount), File_Size/FrameSizes.begin()->first);
                if (Retrieve(StreamKind_Last, Pos, Fill_Parameter(StreamKind_Last, Generic_Duration)).empty())
                    Fill(StreamKind_Last, Pos, Fill_Parameter(StreamKind_Last, Generic_Duration), Retrieve(Stream_General, 0, General_Duration));
            }
        }
    }

    if (!IsSub && File_Size!=(int64u)-1)
    {
        Fill(Stream_Audio, 0, Audio_StreamSize_Encoded, File_Size, 10, true);
        for (size_t Pos=1; Pos<Count_Get(Stream_Audio); Pos++)
            Fill(Stream_Audio, Pos, Audio_StreamSize_Encoded, 0, 10, true);
    }

    #if MEDIAINFO_CONFORMANCE
        if (Conformance_DataStreamNumbers && !(Conformance_DataStreamNumbers&1))
        {
            conformance_errors Errors;
            Errors.Add("No data burst has data_stream_number 0 (main audio service)");
            Fill_Conformance_Specification("burst_info data_stream_number", string(), Errors);
        }
        Streams_Finish_Conformance();
    #endif //MEDIAINFO_CONFORMANCE
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

#if MEDIAINFO_SEEK
//---------------------------------------------------------------------------
void File_SmpteSt0337::Read_Buffer_Unsynched()
{
    if (Frame_Count_NotParsedIncluded!=(int64u)-1 && FrameRate)
    {
        Frame_Count_NotParsedIncluded=float64_int64s(File_GoTo/FrameRate);
        FrameInfo.DTS=Frame_Count_NotParsedIncluded*1000000000/48000;
    }

    Aligned=false;
    if (Parser)
        Parser->Open_Buffer_Unsynch();
    #if MEDIAINFO_CONFORMANCE
        Conformance_Unsynch();
    #endif //MEDIAINFO_CONFORMANCE
}
#endif // MEDIAINFO_SEEK

//---------------------------------------------------------------------------
#if MEDIAINFO_SEEK
size_t File_SmpteSt0337::Read_Buffer_Seek (size_t Method, int64u Value, int64u ID)
{
    // Init
    if (!Duration_Detected)
    {
        MediaInfo_Internal MI;
        MI.Option(__T("File_KeepInfo"), __T("1"));
        Ztring ParseSpeed_Save=MI.Option(__T("ParseSpeed_Get"), __T(""));
        Ztring Demux_Save=MI.Option(__T("Demux_Get"), __T(""));
        MI.Option(__T("ParseSpeed"), __T("0"));
        MI.Option(__T("Demux"), Ztring());
        size_t MiOpenResult=MI.Open(File_Name);
        MI.Option(__T("ParseSpeed"), ParseSpeed_Save); // This is a global value, need to reset it. TODO: local value
        MI.Option(__T("Demux"), Demux_Save); // This is a global value, need to reset it. TODO: local value
        if (!MiOpenResult)
            return 0;

        FrameRate=MI.Get(Stream_Audio, 0, __T("FrameRate")).To_float64();

        Duration_Detected=true;
    }

    // Parsing
    switch (Method)
    {
        case 0  :
                    if (FrameRate)
                    {
                        float64 FrameSize=3072000/FrameRate;
                        int64u  FrameCount=float64_int64s(Value/FrameSize);
                        Value=float64_int64s(FrameCount*FrameSize);
                    }
                    GoTo(Value);
                    Open_Buffer_Unsynch();
                    return 1;
        case 1  :
                    return Read_Buffer_Seek(0, File_Size*Value/10000, ID);
        case 2  :   // Timestamp
                    {
                    if (FrameRate)
                        return (size_t)-1; // Not supported

                    {
                        float64 FrameSize=3072000/FrameRate;
                        Unsynch_Frame_Count=float64_int64s(((float64)Value)/1000000000*FrameRate);
                        GoTo(float64_int64s(Unsynch_Frame_Count*FrameSize));
                        Open_Buffer_Unsynch();
                        return 1;
                    }
                    }
        case 3  :   // FrameNumber
                    {
                    if (FrameRate)
                        return (size_t)-1; // Not supported

                    {
                        float64 FrameSize=3072000/FrameRate;
                        Unsynch_Frame_Count=Value;
                        GoTo(float64_int64s(Unsynch_Frame_Count*FrameSize));
                        Open_Buffer_Unsynch();
                        return 1;
                    }
                    }
        default :   return (size_t)-1; // Not supported
    }
}
#endif // MEDIAINFO_SEEK

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
bool File_SmpteSt0337::Synchronize()
{
    // Guard band
    size_t Buffer_Offset_Base = Buffer_Offset;

    static const uint8_t Sync16_BE    [4] = { 0xF8,0x72,0x4E,0x1F };
    static const uint8_t Sync16_LE    [4] = { 0x72,0xF8,0x1F,0x4E };

    static const uint8_t Sync20_BE    [5] = { 0x6F,0x87,0x25,0x4E,0x1F };
    static const uint8_t Sync20_LE    [5] = { 0x72,0xF8,0xF6,0xE1,0x54 };

    static const uint8_t Sync24_BE    [6] = { 0x96,0xF8,0x72,0xA5,0x4E,0x1F };
    static const uint8_t Sync24_LE    [6] = { 0x72,0xF8,0x96,0x1F,0x4E,0xA5 };
    static const uint8_t Sync16in24_BE[6] = { 0x00,0xF8,0x72,0x00,0x4E,0x1F };
    static const uint8_t Sync16in24_LE[6] = { 0x00,0x72,0xF8,0x00,0x1F,0x4E };
    static const uint8_t Sync20in24_BE[6] = { 0x6F,0x87,0x20,0x54,0xE1,0xF0 };
    static const uint8_t Sync20in24_LE[6] = { 0x20,0x87,0x6F,0xF0,0xE1,0x54 };

    static const uint8_t Sync16in32_BE[8] = { 0x00,0x00,0xF8,0x72,0x00,0x00,0x4E,0x1F };
    static const uint8_t Sync16in32_LE[8] = { 0x00,0x00,0x72,0xF8,0x00,0x00,0x1F,0x4E };
    static const uint8_t Sync20in32_BE[8] = { 0x00,0x6F,0x87,0x20,0x00,0x54,0xE1,0xF0 };
    static const uint8_t Sync20in32_LE[8] = { 0x00,0x20,0x87,0x6F,0x00,0xF0,0xE1,0x54 };
    static const uint8_t Sync24in32_BE[8] = { 0x00,0x96,0xF8,0x72,0x00,0xA5,0x4E,0x1F };
    static const uint8_t Sync24in32_LE[8] = { 0x00,0x72,0xF8,0x96,0x00,0x1F,0x4E,0xA5 };

    // Synchronizing
    while (Buffer_Offset + 16 <= Buffer_Size)
    {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(Buffer + Buffer_Offset);

        if ((BitDepth == 0 || BitDepth == 16) && (!Aligned || ((Buffer_TotalBytes + Buffer_Offset) % 4) == 0))
        {
            if (memcmp(p, Sync16_BE, 4) == 0) // 16-bit, BE
            {
                BitDepth=16;
                Stream_Bits=16;
                Endianness='B'; // BE
                break; // while()
            }
            if (memcmp(p, Sync16_LE, 4) == 0) // 16-bit, LE
            {
                BitDepth=16;
                Stream_Bits=16;
                Endianness='L'; // LE
                break; // while()
            }
        }
        if ((BitDepth == 0 || BitDepth == 20) && (!Aligned || ((Buffer_TotalBytes + Buffer_Offset) % 5) == 0))
        {
            if (memcmp(p, Sync20_BE, 5) == 0) // 20-bit, BE
            {
                BitDepth=20;
                Stream_Bits=20;
                Endianness='B'; // BE
                break; // while()
            }
            if (memcmp(p, Sync20_LE, 5) == 0) // 20-bit, LE
            {
                BitDepth=20;
                Stream_Bits=20;
                Endianness='L'; // BE
                break; // while()
            }
        }
        if ((BitDepth == 0 || BitDepth == 24) && (!Aligned || ((Buffer_TotalBytes + Buffer_Offset) % 6) == 0))
        {
            if (memcmp(p, Sync24_BE, 6) == 0) // 24-bit, BE
            {
                BitDepth=24;
                Stream_Bits=24;
                Endianness='B'; // BE
                break; // while()
            }
            if (memcmp(p, Sync24_LE, 6) == 0) // 24-bit, LE
            {
                BitDepth=24;
                Stream_Bits=24;
                Endianness='L'; // LE
                break; // while()
            }
            if (memcmp(p, Sync16in24_BE, 6) == 0) // 16-bit in 24-bit, BE
            {
                BitDepth=24;
                Stream_Bits=16;
                Endianness='B'; // BE
                NullPadding_Size=1;
                break; // while()
            }
            if (memcmp(p, Sync16in24_LE, 6) == 0) // 16-bit in 24-bit, LE
            {
                BitDepth=24;
                Stream_Bits=16;
                Endianness='L'; // LE
                NullPadding_Size=1;
                break; // while()
            }
            if (memcmp(p, Sync20in24_BE, 6) == 0) // 20-bit in 24-bit, BE
            {
                BitDepth=24;
                Stream_Bits=20;
                Endianness='B'; // BE
                break; // while()
            }
            if (memcmp(p, Sync20in24_LE, 6) == 0) // 20-bit in 24-bit, LE
            {
                BitDepth=24;
                Stream_Bits=20;
                Endianness='L'; // LE
                break; // while()
            }
        }
        if ((BitDepth == 0 || BitDepth == 32) && (!Aligned || ((Buffer_TotalBytes + Buffer_Offset) % 8) == 0))
        {
            if (memcmp(p, Sync16in32_BE, 8) == 0) // 16-bit in 32-bit, BE
            {
                BitDepth=32;
                Stream_Bits=16;
                Endianness='B'; // BE
                NullPadding_Size=2;
                break; // while()
            }
            if (memcmp(p, Sync16in32_LE, 8) == 0) // 16-bit in 32-bit, LE
            {
                BitDepth=32;
                Stream_Bits=16;
                Endianness='L'; // LE
                NullPadding_Size=2;
                break; // while()
            }
            if (memcmp(p, Sync20in32_BE, 8) == 0) // 20-bit in 32-bit, BE
            {
                BitDepth=32;
                Stream_Bits=20;
                Endianness='B'; // BE
                NullPadding_Size=1;
                break; // while()
            }
            if (memcmp(p, Sync20in32_LE, 8) == 0) // 20-bit in 32-bit, LE
            {
                BitDepth=32;
                Stream_Bits=20;
                Endianness='L'; // LE
                NullPadding_Size=1;
                break; // while()
            }
            if (memcmp(p, Sync24in32_BE, 8) == 0) // 24-bit in 32-bit, BE
            {
                BitDepth=32;
                Stream_Bits=24;
                Endianness='B'; // BE
                NullPadding_Size=1;
                break; // while()
            }
            if (memcmp(p, Sync24in32_LE, 8) == 0) // 24-bit in 32-bit, LE
            {
                BitDepth=32;
                Stream_Bits=24;
                Endianness='L'; // LE
                NullPadding_Size=1;
                break; // while()
            }
        }

        if (BitDepth>=4 && Aligned)
            Buffer_Offset+=BitDepth/4;
        else
            Buffer_Offset++;
    }

    // Parsing last bytes if needed
    if (Buffer_Offset+16>Buffer_Size)
    {
        if (!Status[IsAccepted])
            GuardBand_Before+=Buffer_Offset;
        return false;
    }

    // Guard band
    GuardBand_Before+=Buffer_Offset-Buffer_Offset_Base;
    if (GuardBand_After)
    {
        if (GuardBand_Before>GuardBand_After)
            GuardBand_Before-=GuardBand_After;
        else
            GuardBand_Before=0;
        GuardBand_After=0;
    }

    // Synched
    return true;
}

//---------------------------------------------------------------------------
bool File_SmpteSt0337::Synched_Test()
{
    // Guard band
    size_t Buffer_Offset_Base=Buffer_Offset;

    // Skip NULL padding
    size_t Buffer_Offset_Temp=Buffer_Offset;
    if (Aligned)
    {
        if (BitDepth==16)
        {
            while ((Buffer_TotalBytes+Buffer_Offset_Temp)%4) // Padding in part of the AES3 block
            {
                if (Buffer_Offset_Temp+1>Buffer_Size)
                {
                    Element_WaitForMoreData();
                    return false;
                }
                if (Buffer[Buffer_Offset_Temp])
                {
                    Trusted_IsNot("Bad sync");
                    Open_Buffer_Unsynch();
                    return true;
                }
                Buffer_Offset_Temp++;
            }
            while(Buffer_Offset_Temp+4<=Buffer_Size && CC4(Buffer+Buffer_Offset_Temp)==0x00000000)
                Buffer_Offset_Temp+=4;
            if (Buffer_Offset_Temp+4>Buffer_Size)
            {
                Element_WaitForMoreData();
                return false;
            }
        }
        if (BitDepth==20)
        {
            while ((Buffer_TotalBytes+Buffer_Offset_Temp)%5) // Padding in part of the AES3 block
            {
                if (Buffer_Offset_Temp+1>Buffer_Size)
                {
                    Element_WaitForMoreData();
                    return false;
                }
                if (Buffer[Buffer_Offset_Temp])
                {
                    Trusted_IsNot("Bad sync");
                    Open_Buffer_Unsynch();
                    return true;
                }
                Buffer_Offset_Temp++;
            }
            while(Buffer_Offset_Temp+5<=Buffer_Size && CC5(Buffer+Buffer_Offset_Temp)==0x0000000000LL)
                Buffer_Offset_Temp+=5;
            if (Buffer_Offset_Temp+5>Buffer_Size)
            {
                Element_WaitForMoreData();
                return false;
            }
        }
        if (BitDepth==24)
        {
            while ((Buffer_TotalBytes+Buffer_Offset_Temp)%6) // Padding in part of the AES3 block
            {
                if (Buffer_Offset_Temp+1>Buffer_Size)
                {
                    Element_WaitForMoreData();
                    return false;
                }
                if (Buffer[Buffer_Offset_Temp])
                {
                    Trusted_IsNot("Bad sync");
                    Open_Buffer_Unsynch();
                    return true;
                }
                Buffer_Offset_Temp++;
            }
            while(Buffer_Offset_Temp+6<=Buffer_Size && CC6(Buffer+Buffer_Offset_Temp)==0x000000000000LL)
                Buffer_Offset_Temp+=6;
            if (Buffer_Offset_Temp+6>Buffer_Size)
            {
                Element_WaitForMoreData();
                return false;
            }
        }
        else if (BitDepth==32)
        {
            while ((Buffer_TotalBytes+Buffer_Offset_Temp)%8) // Padding in part of the AES3 block
            {
                if (Buffer_Offset_Temp+1>Buffer_Size)
                {
                    Element_WaitForMoreData();
                    return false;
                }
                if (Buffer[Buffer_Offset_Temp])
                {
                    Trusted_IsNot("Bad sync");
                    Open_Buffer_Unsynch();
                    return true;
                }
                Buffer_Offset_Temp++;
            }
            while(Buffer_Offset_Temp+8<=Buffer_Size && CC8(Buffer+Buffer_Offset_Temp)==0x0000000000000000LL)
                Buffer_Offset_Temp+=8;
            if (Buffer_Offset_Temp+8>Buffer_Size)
            {
                Element_WaitForMoreData();
                return false;
            }
        }
    }
    else
    {
        while(Buffer_Offset_Temp+NullPadding_Size<Buffer_Size && !Buffer[Buffer_Offset_Temp+NullPadding_Size])
            Buffer_Offset_Temp++;
        if (Buffer_Offset_Temp+NullPadding_Size>=Buffer_Size)
        {
            Element_WaitForMoreData();
            return false;
        }
    }

    #if MEDIAINFO_TRACE
        if (Buffer_Offset_Temp-Buffer_Offset)
        {
            Element_Size=Buffer_Offset_Temp-Buffer_Offset;
            Skip_XX(Buffer_Offset_Temp-Buffer_Offset,           "Guard band");
        }
    #endif // MEDIAINFO_TRACE
    Buffer_Offset=Buffer_Offset_Temp;

    // Must have enough buffer for having header
    if (Buffer_Offset+16>Buffer_Size)
        return false;

    // Quick test of synchro
    switch (Endianness)
    {
        case 'B' :
                    switch (BitDepth)
                    {
                        case 16 :   if (CC4(Buffer+Buffer_Offset)!=0xF8724E1F) {Synched=false; return true;} break;
                        case 20 :   if (CC5(Buffer+Buffer_Offset)!=0x6F87254E1FLL) {Synched=false; return true;} break;
                        case 24 :
                                    switch (Stream_Bits)
                                    {
                                        case 16 : if (CC6(Buffer+Buffer_Offset)!=0x00F872004E1FLL) {Synched=false; return true;} break;
                                        case 20 : if (CC6(Buffer+Buffer_Offset)!=0x6F872054E1F0LL) {Synched=false; return true;} break;
                                        case 24 : if (CC6(Buffer+Buffer_Offset)!=0x96F872A54E1FLL) {Synched=false; return true;} break;
                                        default : ;
                                    }
                                    break;
                        case 32 :
                                    switch (Stream_Bits)
                                    {
                                        case 16 : if (CC8(Buffer+Buffer_Offset)!=0x0000F87200004E1FLL) {Synched=false; return true;} break;
                                        case 20 : if (CC8(Buffer+Buffer_Offset)!=0x006F87200054E1F0LL) {Synched=false; return true;} break;
                                        case 24 : if (CC8(Buffer+Buffer_Offset)!=0x0096F87200A5F41FLL) {Synched=false; return true;} break;
                                        default : ;
                                    }
                                    break;
                        default : ;
                    }
                    break;
        case 'L'  :
                    switch (BitDepth)
                    {
                        case 16 :   if (CC4(Buffer+Buffer_Offset)!=0x72F81F4E) {Synched=false; return true;} break;
                        case 20 :   if (CC5(Buffer+Buffer_Offset)!=0x72F8F6E154LL) {Synched=false; return true;} break;
                        case 24 :
                                    switch (Stream_Bits)
                                    {
                                        case 16 : if (CC6(Buffer+Buffer_Offset)!=0x0072F8001F4ELL) {Synched=false; return true;} break;
                                        case 20 : if (CC6(Buffer+Buffer_Offset)!=0x20876FF0E154LL) {Synched=false; return true;} break;
                                        case 24 : if (CC6(Buffer+Buffer_Offset)!=0x72F8961F4EA5LL) {Synched=false; return true;} break;
                                        default : ;
                                    }
                                    break;
                        case 32 :
                                    switch (Stream_Bits)
                                    {
                                        case 16 : if (CC8(Buffer+Buffer_Offset)!=0x000072F800001F4ELL) {Synched=false; return true;} break;
                                        case 20 : if (CC8(Buffer+Buffer_Offset)!=0x0020876F00F0E154LL) {Synched=false; return true;} break;
                                        case 24 : if (CC8(Buffer+Buffer_Offset)!=0x0072F896001F4EA5LL) {Synched=false; return true;} break;
                                        default : ;
                                    }
                                    break;
                        default : ;
                    }
                    break;
        default    : ; // Should never happen
    }

    // Guard band
    GuardBand_Before+=Buffer_Offset-Buffer_Offset_Base;
    if (GuardBand_After)
    {
        if (GuardBand_Before>GuardBand_After)
            GuardBand_Before-=GuardBand_After;
        else
            GuardBand_Before=0;
        GuardBand_After=0;
    }

    // We continue
    return true;
}

//---------------------------------------------------------------------------
void File_SmpteSt0337::Synched_Init()
{
    if (Frame_Count_NotParsedIncluded==(int64u)-1)
        Frame_Count_NotParsedIncluded=0;
    if (!IsSub)
    {
        FrameInfo.DTS=0; //No DTS in container
        FrameInfo.PTS=0; //No PTS in container
    }
}

//***************************************************************************
// Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_SmpteSt0337::Header_Parse()
{
    // Parsing
    int32u Size=0;
    switch (Endianness)
    {
        case 'B' :
                    switch (BitDepth)
                    {
                        case 16 :   Size=BigEndian2int16u(Buffer+Buffer_Offset+6)         ; break;
                        case 20 :   Size=BigEndian2int24u(Buffer+Buffer_Offset+7)&0x0FFFFF; break;
                        case 24 :
                                    switch (Stream_Bits)
                                    {
                                        case 16 : Size=BigEndian2int16u(Buffer+Buffer_Offset+9)   ; break;
                                        case 20 : Size=BigEndian2int24u(Buffer+Buffer_Offset+9)>>4; break;
                                        case 24 : Size=BigEndian2int24u(Buffer+Buffer_Offset+9)   ; break;
                                        default : ;
                                    }
                                    break;
                        case 32 :
                                    switch (Stream_Bits)
                                    {
                                        case 16 : Size=BigEndian2int16u(Buffer+Buffer_Offset+0xE)   ; break;
                                        case 20 : Size=BigEndian2int24u(Buffer+Buffer_Offset+0xD)>>4; break;
                                        case 24 : Size=BigEndian2int24u(Buffer+Buffer_Offset+0xD)   ; break;
                                        default : ;
                                    }
                                    break;
                        default : ;
                    }
                    break;
        case 'L'  :
                    switch (BitDepth)
                    {
                        case 16 :   Size=LittleEndian2int16u(Buffer+Buffer_Offset+6)   ; break;
                        case 20 :   Size=LittleEndian2int24u(Buffer+Buffer_Offset+7)>>4; break;
                        case 24 :
                                    switch (Stream_Bits)
                                    {
                                        case 16 : Size=LittleEndian2int16u(Buffer+Buffer_Offset+0xA)   ; break;
                                        case 20 : Size=LittleEndian2int24u(Buffer+Buffer_Offset+0x9)>>4; break;
                                        case 24 : Size=LittleEndian2int24u(Buffer+Buffer_Offset+0x9)   ; break;
                                        default : ;
                                    }
                                    break;
                        case 32 :
                                    switch (Stream_Bits)
                                    {
                                        case 16 : Size=LittleEndian2int16u(Buffer+Buffer_Offset+0xE)   ; break;
                                        case 20 : Size=LittleEndian2int24u(Buffer+Buffer_Offset+0xD)>>4; break;
                                        case 24 : Size=LittleEndian2int24u(Buffer+Buffer_Offset+0xD)   ; break;
                                        default : ;
                                    }
                                    break;
                        default : ;
                    }
                    break;
        default   : ; // Should never happen
    }

    // Adaptation
    if (BitDepth!=Stream_Bits)
    {
        Size*=BitDepth; Size/=Stream_Bits;
    }

    // Coherency test
    if (!IsSub && !Status[IsAccepted] && File_Offset+Buffer_Size<File_Size)
    {
        size_t Offset=Buffer_Offset+(size_t)(BitDepth*4/8+Size/8);
        while (Offset<Buffer_Size && Buffer[Offset]==0x00)
            Offset++;
        if (Offset+BitDepth/4>Buffer_Size)
        {
            Element_WaitForMoreData();
            return;
        }
        Offset/=BitDepth/4;
        Offset*=BitDepth/4;
        bool IsOK=true;
        for (int8u Pos=0; Pos<BitDepth/4; Pos++)
            if (Buffer[Buffer_Offset+Pos]!=Buffer[Offset+Pos])
            {
                IsOK=false;
                break;
            }
        if (!IsOK)
        {
            Trusted_IsNot("Bad sync");
            Open_Buffer_Unsynch();
            Buffer_Offset++;
            return;
        }
    }

    // Filling
    Padding=(int8u)(Size%BitDepth);
    if (Padding)
        Size+=BitDepth-Padding;
    Header_Fill_Size(BitDepth*4/8+Size/8);
    Header_Fill_Code(0, "SMPTE ST 337");
}

//---------------------------------------------------------------------------
void File_SmpteSt0337::Data_Parse()
{
    #if MEDIAINFO_DEMUX
        FrameInfo.PTS=FrameInfo.DTS;
        Demux_random_access=true;
        Element_Code=(int64u)-1;
    #endif //MEDIAINFO_DEMUX

    #if MEDIAINFO_CONFORMANCE
        const auto Burst_Begin=File_Offset+Buffer_Offset;
        const auto Burst_Size=Element_Size;
    #endif //MEDIAINFO_CONFORMANCE

    // Adapting
    const int8u* Save_Buffer=NULL;
    size_t Save_Buffer_Offset=0;
    size_t Save_Buffer_Size=0;
    int64u Save_Element_Size=0;

    if (Endianness=='L'|| BitDepth!=Stream_Bits)
    {
        int8u* Info=new int8u[(size_t)Element_Size];
        int8u* Info_Temp=Info;

        if (Endianness=='L' && BitDepth==16 && Stream_Bits==16)
        {
            // Source: 16LE / L1L0 L3L2 R1R0 R3R2
            // Dest  : 16BE / L3L2 L1L0 R3R2 R1R0
            while (Element_Offset+4<=Element_Size)
            {
                size_t Buffer_Pos=Buffer_Offset+(size_t)Element_Offset;

                *(Info_Temp++)= Buffer[Buffer_Pos+1]                                    ;
                *(Info_Temp++)= Buffer[Buffer_Pos  ]                                    ;
                *(Info_Temp++)= Buffer[Buffer_Pos+3]                                    ;
                *(Info_Temp++)= Buffer[Buffer_Pos+2]                                    ;

                Element_Offset+=4;
            }
            if (Element_Offset+2<=Element_Size) // Only in half of the AES3 stream
            {
                size_t Buffer_Pos=Buffer_Offset+(size_t)Element_Offset;

                *(Info_Temp++)= Buffer[Buffer_Pos+1]                                    ;
                *(Info_Temp++)= Buffer[Buffer_Pos  ]                                    ;

                Element_Offset+=2;
            }
        }

        if (Endianness=='L' && BitDepth==20 && Stream_Bits==20)
        {
            // Source: 20LE / L1L0 L3L2 R0L4 R2R1 R4R3
            // Dest  : 20BE / L4L3 L2L1 L0R4 R3R2 R1R0
            while (Element_Offset+5<=Element_Size)
            {
                size_t Buffer_Pos=Buffer_Offset+(size_t)Element_Offset;

                *(Info_Temp++)=(Buffer[Buffer_Pos+2]<<4  ) | (Buffer[Buffer_Pos+1]>>4  );
                *(Info_Temp++)=(Buffer[Buffer_Pos+1]<<4  ) | (Buffer[Buffer_Pos+0]>>4  );
                *(Info_Temp++)=(Buffer[Buffer_Pos+0]<<4  ) | (Buffer[Buffer_Pos+4]>>4  );
                *(Info_Temp++)=(Buffer[Buffer_Pos+4]<<4  ) | (Buffer[Buffer_Pos+3]>>4  );
                *(Info_Temp++)=(Buffer[Buffer_Pos+3]<<4  ) | (Buffer[Buffer_Pos+2]>>4  );

                Element_Offset+=5;
            }
        }

        if (Endianness=='L' && BitDepth==24 && Stream_Bits==16)
        {
            // Source:        XXXX L1L0 L3L2 XXXX R1R0 R3R2
            // Dest  : 16BE / L3L2 L1L0 R3R2 R1R0
            while (Element_Offset+6<=Element_Size)
            {
                size_t Buffer_Pos=Buffer_Offset+(size_t)Element_Offset;

                *(Info_Temp++)= Buffer[Buffer_Pos+2]                                    ;
                *(Info_Temp++)= Buffer[Buffer_Pos+1]                                    ;
                *(Info_Temp++)= Buffer[Buffer_Pos+5]                                    ;
                *(Info_Temp++)= Buffer[Buffer_Pos+4]                                    ;

                Element_Offset+=6;
            }
        }

        if (Endianness=='L' && BitDepth==24 && Stream_Bits==20)
        {
            // Source:        L0XX L2L1 L4L3 R0XX R2R1 R4R3
            // Dest  : 20BE / L4L3 L2L1 L0R4 R3R2 R1R0
            while (Element_Offset+6<=Element_Size)
            {
                size_t Buffer_Pos=Buffer_Offset+(size_t)Element_Offset;

                *(Info_Temp++)= Buffer[Buffer_Pos+2]                                    ;
                *(Info_Temp++)= Buffer[Buffer_Pos+1]                                    ;
                *(Info_Temp++)=(Buffer[Buffer_Pos  ]&0xF0) | (Buffer[Buffer_Pos+5]>>4  );
                *(Info_Temp++)=(Buffer[Buffer_Pos+5]<<4  ) | (Buffer[Buffer_Pos+4]>>4  );
                *(Info_Temp++)=(Buffer[Buffer_Pos+4]<<4  ) | (Buffer[Buffer_Pos+3]>>4  );

                Element_Offset+=6;
            }
            if (Element_Offset+3<=Element_Size)
            {
                size_t Buffer_Pos=Buffer_Offset+(size_t)Element_Offset;

                *(Info_Temp++)= Buffer[Buffer_Pos+2]                                    ;
                *(Info_Temp++)= Buffer[Buffer_Pos+1]                                    ;
                *(Info_Temp++)=(Buffer[Buffer_Pos  ]&0xF0)                              ;
            }
        }

        if (Endianness=='L' && BitDepth==24 && Stream_Bits==24)
        {
            // Source: 24LE / L1L0 L3L2 L5L3 R1R0 R3R2 R5R4
            // Dest  : 24BE / L5L3 L3L2 L1L0 R5R4 R3R2 R1R0
            while (Element_Offset+6<=Element_Size)
            {
                size_t Buffer_Pos=Buffer_Offset+(size_t)Element_Offset;

                *(Info_Temp++)= Buffer[Buffer_Pos+2]                                    ;
                *(Info_Temp++)= Buffer[Buffer_Pos+1]                                    ;
                *(Info_Temp++)= Buffer[Buffer_Pos  ]                                    ;
                *(Info_Temp++)= Buffer[Buffer_Pos+5]                                    ;
                *(Info_Temp++)= Buffer[Buffer_Pos+4]                                    ;
                *(Info_Temp++)= Buffer[Buffer_Pos+3]                                    ;

                Element_Offset+=6;
            }
        }

        if (Endianness=='L' && BitDepth==32 && Stream_Bits==16)
        {
            // Source:        XXXX XXXX L1L0 L3L2 XXXX XXXX R1R0 R3R2
            // Dest  : 16BE / L3L2 L1L0 R3R2 R1R0
            while (Element_Offset+8<=Element_Size)
            {
                size_t Buffer_Pos=Buffer_Offset+(size_t)Element_Offset;

                *(Info_Temp++)= Buffer[Buffer_Pos+3]                                    ;
                *(Info_Temp++)= Buffer[Buffer_Pos+2]                                    ;
                *(Info_Temp++)= Buffer[Buffer_Pos+7]                                    ;
                *(Info_Temp++)= Buffer[Buffer_Pos+6]                                    ;

                Element_Offset+=8;
            }
        }
        if (Endianness=='L' && BitDepth==32 && Stream_Bits==20)
        {
            // Source:        XXXX L0XX L2L1 L4L3 XXXX R0XX R2R1 R4R3
            // Dest  : 20BE / L4L3 L2L1 L0R4 R3R2 R1R0
            while (Element_Offset+8<=Element_Size)
            {
                size_t Buffer_Pos=Buffer_Offset+(size_t)Element_Offset;

                *(Info_Temp++)= Buffer[Buffer_Pos+3]                                    ;
                *(Info_Temp++)= Buffer[Buffer_Pos+2]                                    ;
                *(Info_Temp++)=(Buffer[Buffer_Pos+1]&0xF0) | (Buffer[Buffer_Pos+7]>>4  );
                *(Info_Temp++)=(Buffer[Buffer_Pos+7]<<4  ) | (Buffer[Buffer_Pos+6]>>4  );
                *(Info_Temp++)=(Buffer[Buffer_Pos+6]<<4  ) | (Buffer[Buffer_Pos+5]>>4  );

                Element_Offset+=8;
            }
        }

        if (Endianness=='L' && BitDepth==32 && Stream_Bits==24)
        {
            // Source:        XXXX L1L0 L3L2 L5L3 XXXX R1R0 R3R2 R5R4
            // Dest  : 24BE / L5L3 L3L2 L1L0 R5R4 R3R2 R1R0
            while (Element_Offset+8<=Element_Size)
            {
                size_t Buffer_Pos=Buffer_Offset+(size_t)Element_Offset;

                *(Info_Temp++)= Buffer[Buffer_Pos+3]                                    ;
                *(Info_Temp++)= Buffer[Buffer_Pos+2]                                    ;
                *(Info_Temp++)= Buffer[Buffer_Pos+1]                                    ;
                *(Info_Temp++)= Buffer[Buffer_Pos+7]                                    ;
                *(Info_Temp++)= Buffer[Buffer_Pos+6]                                    ;
                *(Info_Temp++)= Buffer[Buffer_Pos+5]                                    ;

                Element_Offset+=8;
            }
        }

        if (Endianness=='B' && BitDepth==24 && Stream_Bits==20)
        {
            // Source:        L4L3 L2L1 L0XX R4R3 R2R1 R0XX
            // Dest  : 20BE / L4L3 L2L1 L0R4 R3R2 R1R0
            while (Element_Offset+6<=Element_Size)
            {
                size_t Buffer_Pos=Buffer_Offset+(size_t)Element_Offset;

                *(Info_Temp++)= Buffer[Buffer_Pos  ]                                    ;
                *(Info_Temp++)= Buffer[Buffer_Pos+1]                                    ;
                *(Info_Temp++)=(Buffer[Buffer_Pos+2]&0xF0) | (Buffer[Buffer_Pos+3]>>4  );
                *(Info_Temp++)=(Buffer[Buffer_Pos+3]<<4  ) | (Buffer[Buffer_Pos+4]>>4  );
                *(Info_Temp++)=(Buffer[Buffer_Pos+4]<<4  ) | (Buffer[Buffer_Pos+5]>>4  );

                Element_Offset+=6;
            }
        }

        Save_Buffer=Buffer;
        Save_Buffer_Offset=Buffer_Offset;
        Save_Buffer_Size=Buffer_Size;
        Save_Element_Size=Element_Size;
        File_Offset+=Buffer_Offset;
        Buffer=Info;
        Buffer_Offset=0;
        Buffer_Size=Info_Temp-Info;
        Element_Offset=0;
        Element_Size=Buffer_Size;
    }

    // Parsing
    int32u  length_code;
    int8u data_stream_number;
    int32u data_type_New;
    int8u data_type_dependent;
    int8u data_mode;
    int8u Pc_Reserved=0, Pe_Reserved=0;
    int32u Pf=0, extended_data_type=(int32u)-1;
    int8u* UncompressedData=NULL;
    size_t UncompressedData_Size=0;
    string MuxingMode;
    Element_Begin1("Header");
        BS_Begin();
        Skip_S3(Stream_Bits,                                    "Pa");
        Skip_S3(Stream_Bits,                                    "Pb");
        Element_Begin1("Pc");
            Get_S1 ( 3, data_stream_number,                     "data_stream_number");
            Get_S1 ( 5, data_type_dependent,                    "data_type_dependent");
            Skip_SB(                                            "error_flag");
            Get_S1 ( 2, data_mode,                              "data_mode"); Param_Info2(16+4*data_mode, " bits");
            Get_S4 ( 5, data_type_New,                          "data_type"); Param_Info1(Smpte_St0337_data_type[data_type_New]);
            if (Stream_Bits>16)
                Get_S1 (Stream_Bits-16, Pc_Reserved,            "reserved");
        Element_End0();
        Get_S3 (Stream_Bits, length_code,                       "length_code"); Param_Info2(length_code/8, " bytes");
        const auto data_type_Pc=(int8u)data_type_New;
        if (data_type_New==31)
        {
            if (Stream_Bits>16)
                Get_S1 (Stream_Bits-16, Pe_Reserved,            "reserved");
            Get_S4 (16, data_type_New,                          "data_type");
            extended_data_type=data_type_New;
            data_type_New+=32;
            Get_S3 (Stream_Bits, Pf,                            "reserved");
            if (data_type_New==32+1) // ADM
            {
                int8u multiple_chunk_flag=data_type_dependent>>3;               //2-bit
                bool format_flag=((data_type_dependent>>2)&1)?true:false;       //1-bit
                bool assemble_flag=((data_type_dependent>>1)&1)?true:false;     //1-bit
                bool changedMetadata_flag=(data_type_dependent&1)?true:false;   //1-bit
                Param_Info1(Smpte_St0337_Adm_multiple_chunk_flag[multiple_chunk_flag]);
                int8u format_type=0, Track_ID=0, track_numbers=0, in_timeline_flag=0;
                if (format_flag)
                {
                    Element_Begin1("format_info");
                    Skip_S2(12,                                 "reserved");
                    Get_S1 (4, format_type,                     "format_type"); Param_Info1C(format_type<sizeof(Smpte_St0337_Adm_format_type)/sizeof(const char*), Smpte_St0337_Adm_format_type[format_type]);
                    if (Stream_Bits>16)
                        Skip_S1(Stream_Bits-16,                 "reserved");
                }
                if (assemble_flag)
                {
                    Element_Begin1("assemble_info");
                    Skip_S2(2,                                  "reserved");
                    Get_S1 (6, Track_ID,                        "Track_ID");
                    Get_S1 (6, track_numbers,                   "track_numbers");
                    Get_S1 (2, in_timeline_flag,                "in_timeline_flag"); Param_Info1(Smpte_St0337_Adm_multiple_chunk_flag[in_timeline_flag]);
                    if (Stream_Bits>16)
                        Skip_S1(Stream_Bits-16,                 "reserved");
                    Element_End0();
                }
                MuxingMode=string("SMPTE ST 337 / SMPTE ST 2116");
                if (!data_stream_number && !multiple_chunk_flag && !in_timeline_flag && format_type<=1)
                {
                    MuxingMode+=" Level ";
                    MuxingMode+='A';
                    if (format_type==1)
                        MuxingMode+='X';
                    if (track_numbers<10)
                        MuxingMode+='1'+track_numbers;
                    else
                    {
                        MuxingMode+='1';
                        MuxingMode+='0'-10+track_numbers;
                    }
                }
                if (Parser || data_stream_number || multiple_chunk_flag || in_timeline_flag || format_type>1 || Track_ID || track_numbers)
                {
                    Skip_BS(Data_BS_Remain(),                   "Data (Unsupported)");
                }
                else if (format_type==1)
                {
                    size_t Compressed_Size=Data_BS_Remain()/8;
                    int8u* Compressed=new int8u[Compressed_Size];
                    size_t Compressed_Offset=0;
                    while (Data_BS_Remain()>=(size_t)Stream_Bits*2)
                    {
                        int64u Data;
                        Get_S6(Stream_Bits*2, Data, "Data");
                        for (int8u i=0; i<Stream_Bits/4; i++)
                        {
                            Compressed[Compressed_Offset++]=(int8u)(Data>>((Stream_Bits/4-i-1)*8));
                        }
                    }
                    if (Data_BS_Remain())
                        Skip_BS(Data_BS_Remain(),                "Padding");
                    BS_End();

                    // Adapting
                    const int8u* Save_Buffer=Buffer;
                    size_t Save_Buffer_Offset=Buffer_Offset;
                    size_t Save_Buffer_Size=Buffer_Size;
                    int64u Save_Element_Size=Element_Size;
                    Buffer=Compressed;
                    Buffer_Offset=0;
                    Buffer_Size=Compressed_Offset;
                    Element_Offset=0;
                    Element_Size=Buffer_Size;

                    //Uncompress init
                    z_stream strm;
                    strm.next_in=(Bytef*)Compressed;;
                    strm.avail_in=Compressed_Offset;
                    strm.next_out=NULL;
                    strm.avail_out=0;
                    strm.total_out=0;
                    strm.zalloc=Z_NULL;
                    strm.zfree=Z_NULL;
                    inflateInit2(&strm, 15+16); // 15 + 16 are magic values for gzip

                    //Prepare out
                    strm.avail_out=0x10000; //Blocks of 64 KiB, arbitrary chosen, as a begin
                    strm.next_out=(Bytef*)new Bytef[strm.avail_out];

                    //Parse compressed data, with handling of the case the output buffer is not big enough
                    for (;;)
                    {
                        //inflate
                        int inflate_Result=inflate(&strm, Z_NO_FLUSH);
                        if (inflate_Result<0)
                            break;

                        //Check if we need to stop
                        if (strm.avail_out || inflate_Result || strm.total_out>=0x4000000) //Arbitrary limit of 64 MiB
                            break;

                        //Need to increase buffer
                        size_t UncompressedData_NewMaxSize=strm.total_out*4;
                        int8u* UncompressedData_New=new int8u[UncompressedData_NewMaxSize];
                        memcpy(UncompressedData_New, strm.next_out-strm.total_out, strm.total_out);
                        delete[](strm.next_out - strm.total_out); strm.next_out=UncompressedData_New;
                        strm.next_out=strm.next_out+strm.total_out;
                        strm.avail_out=UncompressedData_NewMaxSize-strm.total_out;
                    }
                    UncompressedData=strm.next_out-strm.total_out;
                    UncompressedData_Size=strm.total_out;
                    inflateEnd(&strm);
                    delete[] Compressed;
                    // Adapting
                    Buffer=Save_Buffer;
                    Buffer_Offset=Save_Buffer_Offset;
                    Buffer_Size=Save_Buffer_Size;
                    Element_Offset=Save_Element_Size;
                    Element_Size=Save_Element_Size;
                }
                Element_End0();
            }
        }
        BS_End();
    Element_End0();

    #if MEDIAINFO_CONFORMANCE
        Conformance_Burst({Burst_Begin, Burst_Size, length_code, extended_data_type, Pf, data_stream_number, data_type_dependent, data_mode, data_type_Pc, Pc_Reserved, Pe_Reserved});
    #endif //MEDIAINFO_CONFORMANCE

    if (data_type_New!=data_type)
    {
        delete Parser; Parser=NULL;
        data_type=data_type_New;
    }
    if (Parser==NULL)
    {
        switch(data_type)
        {
            // SMPTE ST338
            case  1 :   // AC-3
            case 16 :   // E-AC-3 (professional)
            case 21 :   // E-AC-3 (consumer)
                        Parser=new File_Ac3();
                        ((File_Ac3*)Parser)->Frame_Count_Valid=2;
                        #if MEDIAINFO_DEMUX
                            if (Config->Demux_Unpacketize_Get())
                            {
                                Demux_UnpacketizeContainer=false; //No demux from this parser
                                Demux_Level=4; //Intermediate
                                Parser->Demux_Level=2; //Container
                                Parser->Demux_UnpacketizeContainer=true;
                            }
                        #endif //MEDIAINFO_DEMUX
                        break;
            case  4 :   // MPEG-1 Layer 1
            case  5 :   // MPEG-1 Layer 2/3, MPEG-2 Layer 1/2/3 without extension
            case  6 :   // MPEG-2 Layer 1/2/3 with extension
            case  8 :   // MPEG-2 Layer 1 low frequency
            case  9 :   // MPEG-2 Layer 2/3 low frequency
                        Parser=new File_Mpega();
                        break;
            case  7 :   // MPEG-2 AAC in ADTS
            case 19 :   // MPEG-2 AAC in ADTS low frequency
                        #if defined(MEDIAINFO_AAC_YES)
                            Parser=new File_Aac();
                            ((File_Aac*)Parser)->Mode=File_Aac::Mode_ADTS;
                        #else
                        {
                            //Filling
                            File__Analyze* Parser=new File_Unknown();
                            Open_Buffer_Init(Parser);
                            Parser->Stream_Prepare(Stream_Audio);
                            Parser->Fill(Stream_Audio, 0, Audio_Format, "AAC");
                            Parser->Fill(Stream_Audio, 0, Audio_MuxingMode, "ADTS");
                        }
                        #endif //defined(MEDIAINFO_AAC_YES)
                        break;
            case 10 :   // MPEG-4 AAC in ADTS or LATM
            case 11 :   // MPEG-4 AAC in ADTS or LATM
                        #if defined(MEDIAINFO_AAC_YES)
                            Parser=new File_Aac();
                        #else
                        {
                            //Filling
                            File__Analyze* Parser=new File_Unknown();
                            Open_Buffer_Init(Parser);
                            Parser->Stream_Prepare(Stream_Audio);
                            Parser->Fill(Stream_Audio, 0, Audio_Format, "AAC");
                        }
                        #endif //defined(MEDIAINFO_AAC_YES)
                        break;
            case 24 :   // AC-4
                        Parser=new File_Ac4();
                        ((File_Ac4*)Parser)->Frame_Count_Valid=1;
                        break;
            case 28 :   // Dolby E
                        #if defined(MEDIAINFO_DOLBYE_YES)
                        Parser=new File_DolbyE();
                        #else
                        {
                            //Filling
                            File__Analyze* Parser=new File_Unknown();
                            Open_Buffer_Init(Parser);
                            Parser->Stream_Prepare(Stream_Audio);
                            Parser->Fill(Stream_Audio, 0, Audio_Format, "DDE");
                        }
                        #endif
                        break;
            case 32+1 : // ADM
                        #if defined(MEDIAINFO_ADM_YES)
                        {
                        if (UncompressedData || Element_Offset<Element_Size)
                        {
                            Parser=new File_Adm();
                            ((File_Adm*)Parser)->MuxingMode=MuxingMode;
                        }
                        else
                        {
                            //Unsupported features are present
                            Parser=new File_Unknown();
                            Open_Buffer_Init(Parser);
                            Parser->Accept();
                            Parser->Stream_Prepare(Stream_Audio);
                            Parser->Fill(Stream_Audio, 0, "Metadata_Format", "ADM");
                            Parser->Fill(Stream_Audio, 0, "Metadata_MuxingMode", MuxingMode);
                            Parser->Finish();
                        }
                        #else
                        {
                            //Filling
                            Parser=new File_Unknown();
                            Open_Buffer_Init(Parser);
                            Parser->Accept();
                            Parser->Stream_Prepare(Stream_Audio);
                            Parser->Fill(Stream_Audio, 0, "Metadata_Format", "ADM");
                            Parser->Finish();
                        }
                        #endif
                        }
                        break;
            default : ;
        }

        if (Parser)
        {
            Open_Buffer_Init(Parser);
        }
    }

    #if MEDIAINFO_DEMUX
        if (Save_Buffer)
        {
            std::swap(Buffer, Save_Buffer);
            std::swap(Buffer_Offset, Save_Buffer_Offset);
            std::swap(Buffer_Size, Save_Buffer_Size);
            std::swap(Element_Size, Save_Element_Size);
            File_Offset-=Buffer_Offset;
        }

        if (data_type==28) //If Dolby E, we must demux the SMPTE ST 337 header too (TODO: add an option for forcing SMPTE ST 337 header)
        {
            int64u Demux_Element_Offset=Element_Offset;
            Element_Offset=0;

            if (BitDepth==20)
            {
                //We must pad to 24 bits
                int8u*          Info2=new int8u[(size_t)Element_Size*6/5];
                size_t          Info2_Offset=0;
                const int8u*    Demux_Buffer=Buffer+Buffer_Offset;
                size_t          Demux_Buffer_Size=(size_t)Element_Size;
                size_t          Demux_Buffer_Pos=0;

                // Source: 20LE L1L0 L3L2 R0L4 R2R1 R4R3
                // Dest  :      L0XX L2L1 L4L3 R0XX R2R1 R4R3
                while (Demux_Buffer_Pos+5<=Demux_Buffer_Size)
                {
                    Info2[Info2_Offset+0]= Demux_Buffer[Demux_Buffer_Pos+0]<<4                                             ;
                    Info2[Info2_Offset+1]=(Demux_Buffer[Demux_Buffer_Pos+1]<<4  ) | (Demux_Buffer[Demux_Buffer_Pos+0]>>4  );
                    Info2[Info2_Offset+2]=(Demux_Buffer[Demux_Buffer_Pos+2]<<4  ) | (Demux_Buffer[Demux_Buffer_Pos+1]>>4  );
                    Info2[Info2_Offset+3]= Demux_Buffer[Demux_Buffer_Pos+2]&0xF0                                           ;
                    Info2[Info2_Offset+4]= Demux_Buffer[Demux_Buffer_Pos+3]                                                ;
                    Info2[Info2_Offset+5]= Demux_Buffer[Demux_Buffer_Pos+4]                                                ;

                    Info2_Offset+=6;
                    Demux_Buffer_Pos+=5;
                }

                Demux(Info2, Info2_Offset, ContentType_MainStream);

                delete[] Info2;
            }
            else
                Demux(Buffer+Buffer_Offset, (size_t)Element_Size, ContentType_MainStream);

            Element_Offset=Demux_Element_Offset;
        }
        else
            Demux(Buffer+Buffer_Offset+BitDepth/2, (size_t)(Element_Size-BitDepth/2), ContentType_MainStream);

        if (Save_Buffer)
        {
            File_Offset+=Buffer_Offset;
            std::swap(Buffer, Save_Buffer);
            std::swap(Buffer_Offset, Save_Buffer_Offset);
            std::swap(Buffer_Size, Save_Buffer_Size);
            std::swap(Element_Size, Save_Element_Size);
        }
     #endif //MEDIAINFO_DEMUX

    // Guard band
    GuardBand_After=0;

    if (Parser && !Parser->Status[IsFinished])
    {
        #if defined(MEDIAINFO_DOLBYE_YES)
        switch(data_type)
        {
            case 28 :
                        ((File_DolbyE*)Parser)->GuardBand_Before=GuardBand_Before;
                        ((File_DolbyE*)Parser)->GuardBand_Before*=Stream_Bits;
                        ((File_DolbyE*)Parser)->GuardBand_Before/=BitDepth;
                        break;
            default : ;
        }
        #endif

        Parser->FrameInfo=FrameInfo;
        if (UncompressedData)
        {
            Open_Buffer_Continue(Parser, UncompressedData, UncompressedData_Size);
            delete[] UncompressedData;
        }
        else            
        {
            int64u Element_Size_Temp=Element_Size-Padding/8;
            if (Element_Offset<Element_Size_Temp)
                Open_Buffer_Continue(Parser, Buffer+Buffer_Offset+(size_t)Element_Offset, (size_t)(Element_Size_Temp-Element_Offset));
        }
        Element_Offset=Element_Size;
        #if MEDIAINFO_DEMUX
            FrameInfo.DUR=Parser->FrameInfo.DUR;
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
        #endif // MEDIAINFO_DEMUX

        #if defined(MEDIAINFO_DOLBYE_YES)
        switch (data_type)
        {
            case 28 :
                        GuardBand_After=((File_DolbyE*)Parser)->GuardBand_After;
                        GuardBand_After*=BitDepth;
                        GuardBand_After/=Stream_Bits;
                        break;
            default : ;
        }
        #endif
    }
    else
    {
        Skip_XX(Element_Size-Element_Offset,                    "Data");
    }

    if (Save_Buffer)
    {
        delete[] Buffer;
        Buffer=Save_Buffer;
        Buffer_Offset=Save_Buffer_Offset;
        Buffer_Size=Save_Buffer_Size;
        File_Offset-=Buffer_Offset;
        Element_Size=Save_Element_Size;
    }

    FILLING_BEGIN();
        if (Frame_Count) // Ignore first GuardBand_Before
            FrameSizes[(IsSub && !GuardBand_Before && !GuardBand_After)?Buffer_Size:(GuardBand_Before+Element_Size+GuardBand_After)]++;

        Frame_Count++;
        if (Frame_Count_NotParsedIncluded!=(int64u)-1)
            Frame_Count_NotParsedIncluded++;

        int64u Frame_Count_Valid=1+(File_Offset+Buffer_Size<File_Size);
        if (!Status[IsAccepted] && (Frame_Count>=Frame_Count_Valid || (Parser && Parser->Status[IsAccepted])))
            Accept("SMPTE ST 337");
        if (!Status[IsFilled] && Frame_Count>=2 && (!Parser || Parser->Status[IsFilled]))
        {
            Fill("SMPTE ST 337");
            if (!IsSub && Config->ParseSpeed<1.0)
            {
                Open_Buffer_Unsynch();
                Finish();
            }
        }
        if (!Status[IsFinished] && Frame_Count>=2 && (!Parser || Parser->Status[IsFinished]))
            Finish("SMPTE ST 337");
    FILLING_END();

    // Guard band
    GuardBand_Before=0;
}

//***************************************************************************
// Conformance
//***************************************************************************

#if MEDIAINFO_CONFORMANCE
//---------------------------------------------------------------------------
void File_SmpteSt0337::Conformance_Burst(const burst_preamble& Burst)
{
    // SMPTE ST 337 burst_info
    if (Burst.data_mode==3)
        Fill_Conformance("burst_info data_mode", "data_mode is reserved (actual 3)");
    else if (16+4*Burst.data_mode!=Stream_Bits)
        Fill_Conformance("burst_info data_mode", "data_mode is not the bit depth of the sync words (actual "+std::to_string(16+4*Burst.data_mode)+"-bit, expected "+std::to_string(Stream_Bits)+"-bit)");
    if (Burst.Pc_Reserved)
        Fill_Conformance("burst_info reserved", "Reserved bits of burst_info are not 0");
    if (Burst.data_type==2 && Burst.data_stream_number!=7)
        Fill_Conformance("burst_info data_stream_number", "data_stream_number of the time stamp data burst is wrong (actual "+std::to_string(Burst.data_stream_number)+", expected 7)");
    else if (Burst.data_type!=2 && Burst.data_stream_number==7)
        Fill_Conformance("burst_info data_stream_number", "data_stream_number is reserved for time stamp data bursts (actual 7)");
    if (Burst.data_type)
        Conformance_DataStreamNumbers|=(int8u)(1<<Burst.data_stream_number);

    // SMPTE ST 338 data_type, depending on the edition
    const auto data_type_Generation=Smpte_St0337_data_type_Generation[Burst.data_type];
    if (Conformance_DataType_IsChecked && data_type_Generation==(int8u)-1)
        Fill_Conformance("burst_info data_type", "data_type is reserved (actual "+std::to_string(Burst.data_type)+')');
    else if (Conformance_DataType_IsChecked && data_type_Generation>Conformance_Generation)
        Fill_Conformance("burst_info data_type", "data_type is not defined in the checked edition (actual "+std::to_string(Burst.data_type)+", checked edition "+Smpte_St0337_Generation_DataTypes[Conformance_Generation]+')');

    // SMPTE ST 337:2015 extended data type
    if (Burst.extended_data_type!=(int32u)-1 && data_type_Generation<=Conformance_Generation)
    {
        if (Burst.Pe_Reserved)
            Fill_Conformance("extended_data_type", "Higher significance bits of extended_data_type are not 0");
        if (Burst.Pf)
            Fill_Conformance("Pf", "Pf is wrong (actual "+std::to_string(Burst.Pf)+", expected 0)");
        if (Burst.length_code<2*(int32u)Stream_Bits)
            Fill_Conformance("length_code", "length_code does not count Pe and Pf");
        int8u extended_data_type_Generation=(int8u)-1; // SMPTE ST 338 extended data types
        if (Burst.extended_data_type==0x0001)
            extended_data_type_Generation=3; // Serial ADM (SMPTE ST 2116)
        string extended_data_type_String;
        for (int Shift=12; Shift>=0; Shift-=4)
            extended_data_type_String+="0123456789ABCDEF"[(Burst.extended_data_type>>Shift)&0xF];
        if (Burst.extended_data_type==0x4E1F || Burst.extended_data_type==0xF872)
            Fill_Conformance("extended_data_type", "extended_data_type is forbidden (actual 0x"+extended_data_type_String+')');
        else if (Conformance_DataType_IsChecked && extended_data_type_Generation==(int8u)-1)
            Fill_Conformance("extended_data_type", "extended_data_type is reserved (actual 0x"+extended_data_type_String+')');
        else if (Conformance_DataType_IsChecked && extended_data_type_Generation>Conformance_Generation)
            Fill_Conformance("extended_data_type", "extended_data_type is not defined in the checked edition (actual 0x"+extended_data_type_String+", checked edition "+Smpte_St0337_Generation_DataTypes[Conformance_Generation]+')');
    }

    // SMPTE ST 337 burst spacing, a data burst preceded by 4 subframes of 0 is needed at least every 4096 AES3 frames
    // Only for AES3 samples from a container, files with only data bursts (e.g. Dolby E data files) have no padding between bursts
    const int64u Frame_Size=BitDepth/4; // Bytes per AES3 frame
    if (Frame_Size && IsSub)
    {
        const auto Gap=Conformance_Burst_End==(int64u)-1?(int64u)GuardBand_Before:(Burst.Begin>Conformance_Burst_End?(Burst.Begin-Conformance_Burst_End):0); // Bytes of 0 before the burst
        const auto Burst_Frames=(Burst.Size+Frame_Size-1)/Frame_Size;
        if (Gap>=2*Frame_Size)
            Conformance_Frames_SinceExtendedSync=Burst_Frames;
        else
            Conformance_Frames_SinceExtendedSync+=(Gap+Frame_Size-1)/Frame_Size+Burst_Frames;
        if (Conformance_Frames_SinceExtendedSync>=4096)
        {
            Fill_Conformance("Burst spacing", "No data burst is preceded by 4 subframes of 0 during 4096 AES3 frames");
            Conformance_Frames_SinceExtendedSync=0;
        }
    }
    Conformance_Burst_End=Burst.Begin+Burst.Size;

    // SMPTE ST 340 AC-3 and Enhanced AC-3
    if ((Burst.data_type==1 || Burst.data_type==16) && Burst.data_stream_number<7)
    {
        if (Burst.data_type_dependent&0x07)
            Fill_Conformance("burst_info data_type_dependent", "Reserved bits of data_type_dependent are not 0");
        auto& Burst_Last=Conformance_Ac3_Burst_Last[Burst.data_stream_number];
        if (Frame_Size && !(Burst.Begin%Frame_Size))
        {
            const auto Burst_Frame=Burst.Begin/Frame_Size;
            if (!(Burst.data_type_dependent&0x08) && Burst_Last!=(int64u)-1 && Burst_Frame-Burst_Last!=1536) // rep_rate_flag is 0
                Fill_Conformance("Burst spacing", "Data burst spacing is wrong as rep_rate_flag is 0 (actual "+std::to_string(Burst_Frame-Burst_Last)+" AES3 frames, expected 1536)");
            Burst_Last=Burst_Frame;
        }
        else
            Burst_Last=(int64u)-1;
    }

    Merge_Conformance();
}
#endif //MEDIAINFO_CONFORMANCE

#if MEDIAINFO_CONFORMANCE
//---------------------------------------------------------------------------
void File_SmpteSt0337::Conformance_Unsynch()
{
    Conformance_Burst_End=(int64u)-1;
    Conformance_Frames_SinceExtendedSync=0;
    for (auto& Burst_Last : Conformance_Ac3_Burst_Last)
        Burst_Last=(int64u)-1;
}
#endif //MEDIAINFO_CONFORMANCE

//***************************************************************************
// C++
//***************************************************************************

} // NameSpace

#endif // MEDIAINFO_SMPTEST0337_YES

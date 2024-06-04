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
#if defined(MEDIAINFO_NSV_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File_Nsv.h"
#if defined(MEDIAINFO_AVC_YES)
    #include "MediaInfo/Video/File_Avc.h"
#endif
#if defined(MEDIAINFO_MPEG4V_YES)
    #include "MediaInfo/Video/File_Mpeg4v.h"
#endif
#if defined(MEDIAINFO_AAC_YES)
    #include "MediaInfo/Audio/File_Aac.h"
#endif
#if defined(MEDIAINFO_MPEGA_YES)
    #include "MediaInfo/Audio/File_Mpega.h"
#endif
#include "MediaInfo/File_Unknown.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include "MediaInfo/TimeCode.h"
#include <algorithm>
#include <limits>
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Private
//***************************************************************************

struct buffer
{
    int8u* Data;
    size_t Size;

    buffer(int8u* Data_, size_t Size_)
        : Data(Data_)
        , Size(Size_)
    {}
};

struct stream
{
    File__Analyze*  Parser=nullptr;
    int32u          len=0;
    int32u          codecid=0;
    vector<buffer>  Buffers;
};

class Private
{
public:
    stream          Streams[2];
    int64s          AudioDelay=0;
    int32u          AuxTotalLen=0;
    bool            IsMajorSynched=false;
};

//***************************************************************************
// Const
//***************************************************************************

int8u Nsv_FrameRate_Multiplier[4] =
{
    30,
    30,
    25,
    34,
};

//---------------------------------------------------------------------------
namespace Elements
{
    const int32u AAC_=0x41414320;
    const int32u AACP=0x41414350;
    const int32u AAV_=0x41415620;
    const int32u DIVX=0x44495658;
    const int32u H264=0x48323634;
    const int32u MP3_=0x4D503320;
    const int32u NONE=0x4E4F4E45;
    const int32u NSVf=0x4E535666;
    const int32u NSVs=0x4E535673;
    const int32u PCM_=0x50434D20;
    const int32u RGB3=0x52474233;
    const int32u SPX_=0x53505820;
    const int32u VLB_=0x564C4220;
    const int32u VP3_=0x56503320;
    const int32u VP30=0x56503330;
    const int32u VP31=0x56503331;
    const int32u VP4_=0x56503420;
    const int32u VP40=0x56503430;
    const int32u VP5_=0x56503520;
    const int32u VP50=0x56503530;
    const int32u VP6_=0x56503620;
    const int32u VP60=0x56503630;
    const int32u VP61=0x56503631;
    const int32u VP62=0x56503632;
    const int32u XVID=0x58564944;
    const int32u YV12=0x59563132;
}

stream_t Stream_Type[2] =
{
    Stream_Video,
    Stream_Audio,
};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Nsv::File_Nsv()
: P(nullptr)
{
}

//---------------------------------------------------------------------------
File_Nsv::~File_Nsv()
{
    delete P;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Nsv::Streams_Accept()
{
    P = new Private;

    Fill(Stream_General, 0, General_Format, "NSV");

    //Configuration
    ParserName="NSV";
    #if MEDIAINFO_EVENTS
        StreamIDs_Size=1;
        ParserIDs[0]=MediaInfo_Parser_Nsv;
        StreamIDs_Width[0]=1;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_DEMUX
        Demux_Level=2; //Container
    #endif //MEDIAINFO_DEMUX
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(0); //Container1
    #endif //MEDIAINFO_TRACE
}

//---------------------------------------------------------------------------
void File_Nsv::Streams_Finish()
{
    for (int i=0; i<2; i++)
    {
        auto Parser=P->Streams[i].Parser;
        if (!Parser)
            continue;
        Fill(Parser);
        if (Config->ParseSpeed<1.0)
            Parser->Open_Buffer_Unsynch();
        Finish(Parser);
        Merge(*Parser, Stream_Type[i], 0, 0);
    }

    float64 DisplayAspectRatio=Retrieve_Const(Stream_Video, 0, Video_DisplayAspectRatio).To_float64();
    if (!DisplayAspectRatio)
    {
        float64 Width=Retrieve_Const(Stream_Video, 0, Video_Width).To_float64();
        float64 Height=Retrieve_Const(Stream_Video, 0, Video_Height).To_float64();
        float64 PixelAspectRatio=Retrieve_Const(Stream_Video, 0, Video_PixelAspectRatio).To_float64();
        if (Width && Height && PixelAspectRatio)
            Fill(Stream_Video, 0, Video_DisplayAspectRatio, Width/Height/PixelAspectRatio);
    }
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
void File_Nsv::FileHeader_Parse()
{
    // Configuration
    MustSynchronize=true;

    //Parsing
    if (Buffer_Size<4)
    {
        Element_WaitForMoreData();
        return; //Must wait for more data
    }
    int32u nsv_sig;
    Peek_B4(nsv_sig);
    if (nsv_sig!=Elements::NSVf)
        return;
    if (Element_Size<28)
    {
        Element_WaitForMoreData();
        return; //Must wait for more data
    }
    int32u header_size, file_size, file_len_ms, metadata_len, toc_alloc, toc_size;
    Skip_C4(                                                    "nsv_sig");
    Get_L4 (header_size,                                        "header_size");
    Get_L4 (file_size,                                          "file_size");
    Get_L4 (file_len_ms,                                        "file_len_ms");
    Get_L4 (metadata_len,                                       "metadata_len");
    Get_L4 (toc_alloc,                                          "toc_alloc");
    Get_L4 (toc_size,                                           "toc_size");

    // Coherency
    if (!P)
    {
        if (header_size<28
         || header_size>file_size
         || metadata_len>header_size-28
         || toc_alloc<toc_size
         || toc_alloc>(header_size-28)/4
         || toc_size>(header_size-28)/4
         || ((int32u)-1)-toc_alloc<metadata_len // Next add overflow prevention
         || toc_alloc+metadata_len>header_size-28)
        {
            Reject();
            return;
        }
        Accept();
    }

    // Parsing
    if (Element_Size<header_size)
    {
        Element_WaitForMoreData();
        return; //Must wait for more data
    }
    if (file_len_ms)
    {
        Fill(Stream_General, 0, General_Duration, file_len_ms);
        Fill(Stream_Video, 0, Video_Duration, file_len_ms);
        Fill(Stream_Audio, 0, Audio_Duration, file_len_ms);
    }
    if (file_size>File_Size)
        IsTruncated(file_size, false, "NSV");
    if (metadata_len)
    {
        Element_Begin1("metadata");
        int64u End=Element_Offset+metadata_len;
        while (Element_Offset<End)
        {
            while (Element_Offset<End)
            {
                int8u Space;
                Peek_B1(Space);
                if (Space!=' ')
                    break;
                Element_Offset++;
            }
            if (Element_Offset>=End)
                break;

            Element_Begin1("item");
            int64u Start_Offset=Element_Offset;
            while (Element_Offset<End)
            {
                int8u Equal;
                Peek_B1(Equal);
                if (Equal=='=')
                {
                    int64u Name_Size=Element_Offset-Start_Offset;
                    Element_Offset=Start_Offset;
                    string Name;
                    Get_String (Name_Size, Name,                "name");
                    Element_Offset++;
                    if (Element_Offset<End)
                    {
                        int8u Separator;
                        Peek_B1(Separator);
                        Element_Offset++;
                        Start_Offset=Element_Offset;
                        while (Element_Offset<End)
                        {
                            int8u SecondSeparator;
                            Peek_B1(SecondSeparator);
                            if (SecondSeparator==Separator)
                            {
                                int64u Value_Size=Element_Offset-Start_Offset;
                                Element_Offset=Start_Offset;
                                string Value;
                                Get_String (Value_Size, Value,  "value");
                                auto asciitolower = [](char in) {
                                    if (in <= 'Z' && in >= 'A')
                                        return (char)(in - ('Z' - 'z'));
                                    return in;
                                };
                                string Name2(Name);
                                transform(Name2.begin(), Name2.end(), Name2.begin(), asciitolower);
                                stream_t StreamKind=Stream_General;
                                if (Name2=="title")
                                    Name2="Title";
                                else if (Name2=="aspect")
                                {
                                    Name2="PixelAspectRatio";
                                    StreamKind=Stream_Video;
                                }
                                else
                                    Name2=Name;
                                Fill(StreamKind, 0, Name2.c_str(), Value);
                                Element_Offset++;
                                break;
                            }
                            Element_Offset++;
                        }
                    }
                    break;
                }
                Element_Offset++;
            }
            Element_End0();
        }
        Element_End0();
    }
    if (toc_size)
    {
        Element_Begin1("toc");
        int64u End=Element_Offset+toc_alloc;
        for (int32u i=0; i<toc_size; i++)
        {
            Info_L4(offset,                                     "offset"); Param_Info1(header_size+offset);
        }
        if (toc_size<(toc_alloc+1)/2)
        {
            int32u toc2_marker;
            Peek_B4(toc2_marker);
            if (toc2_marker==0x544F4332) // "TOC2"
            {
                Skip_C4("toc2_marker");
                for (int32u i=0; i<toc_size; i++)
                {
                    Info_L4(frame,                              "frame");
                }
            }
        }
        if (Element_Offset<End)
            Skip_XX(End-Element_Offset,                         "toc_padding");
        Element_End0();
    }

    // StarDiva metadata
    if (header_size==0x96000 && Element_Offset<0x4B000 && BigEndian2int32u(Buffer+0x4B000)==0x4A4C1A00)
    {
        Skip_XX(0x4B000-Element_Offset,                         "Zeroes");

        File_StarDiva StarDiva;
        Open_Buffer_Init(&StarDiva);
        Open_Buffer_Continue(&StarDiva, 0x4B000);
        if (StarDiva.Status[IsAccepted])
        {
            Merge(StarDiva, Stream_General, 0, 0);
            Stream_Prepare(Stream_Menu);
            Merge(StarDiva, Stream_Menu, 0, 0);
        }
    }

    if (Element_Offset<header_size)
        Skip_XX(header_size-Element_Offset,                     "header_padding");
    Element_End0();
}

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Nsv::Synchronize()
{
    //Synchronizing
    if (Buffer_Size-Buffer_Offset<4)
        return false;
    auto sync_hdr0=BigEndian2int32u(Buffer+Buffer_Offset);
    bool Accepted=Status[IsAccepted];
    auto Buffer_Size_m4=Buffer_Size-4;
    for (;;)
    {
        // Check sync
        auto IsMainSync=sync_hdr0==Elements::NSVs;
        if (IsMainSync || (Accepted && (sync_hdr0>>16)==0xEFBE))
        {
            size_t HeaderBeginSize=IsMainSync?19:2;
            size_t HeaderSize=HeaderBeginSize+5;
            if (Buffer_Size-Buffer_Offset<HeaderSize)
                return false;
            auto Buffer_Temp=Buffer+Buffer_Offset+HeaderBeginSize;
            auto aux_plus_video_len=LittleEndian2int24u(Buffer_Temp);
            Buffer_Temp+=3;
            auto video_len=aux_plus_video_len>>4;
            auto audio_len=LittleEndian2int16u(Buffer_Temp);
            Buffer_Temp+=2;
            auto Size=HeaderSize+video_len+audio_len;
            if (File_Size-(File_Offset+Buffer_Offset)==Size)
                break;
            if (Buffer_Size_m4-Buffer_Offset<Size)
                return false;
            auto sync_hdr1=BigEndian2int32u(Buffer+Buffer_Offset+Size);
            if (sync_hdr1==Elements::NSVs || (sync_hdr1>>16)==0xEFBE)
                break;
        }

        // Next byte
        if (Buffer_Offset<Buffer_Size_m4)
        {
            sync_hdr0=(sync_hdr0<<8)|Buffer[Buffer_Offset+4];
            Buffer_Offset++;
            continue;
        }

        //Parsing last bytes if needed
        Buffer_Offset++;
        auto sync_hdr0_3=sync_hdr0&0xFFFFFF;
        if (sync_hdr0_3!=(Elements::NSVs>>8) && (sync_hdr0_3>>8)!=0xEFBE)
        {
            Buffer_Offset++;
            auto sync_hdr0_2=sync_hdr0&0xFFFF;
            if (sync_hdr0_2!=(Elements::NSVs>>16) && sync_hdr0_2!=0xEFBE)
            {
                Buffer_Offset++;
                auto sync_hdr0_1=sync_hdr0&0xFF;
                if (sync_hdr0_1!=(Elements::NSVs>>24) && sync_hdr0_1!=0xEF)
                    Buffer_Offset++;
            }
        }

        return false;
    }

    //Synched is OK
    Buffer_TotalBytes_LastSynched=Buffer_Offset;
    return true;
}

//---------------------------------------------------------------------------
bool File_Nsv::Synched_Test()
{
    //Must have enough buffer for having header
    if (Buffer_Size-Buffer_Offset<4)
        return false;

    int32u sync_hdr=BigEndian2int32u(Buffer+Buffer_Offset);
    if (sync_hdr!=Elements::NSVs && (sync_hdr>>16)!=0xEFBE)
    {
        Synched=false;
        return true;
    }

    //We continue
    return true;
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Nsv::Header_Parse()
{
    //Parsing
    int32u sync_hdr;
    Peek_B4(sync_hdr);
    if (sync_hdr==Elements::NSVs)
    {
        Element_Level--;
        Element_Info1("Sync");
        Element_Level++;
        int32u vidfmt, audfmt;
        int16u width, height, syncoffs;
        int8u framerate_idx;
        Skip_C4(                                                "sync_hdr");
        Get_C4 (vidfmt,                                         "vidfmt");
        Get_C4 (audfmt,                                         "audfmt");
        Get_L2 (width,                                          "width");
        Get_L2 (height,                                         "height");
        Get_L1 (framerate_idx,                                  "framerate_idx");
        Get_L2 (syncoffs,                                       "syncoffs");

        if (!Frame_Count)
        {
            if (!P)
                Accept();
            if (Element_Size<24)
            {
                Element_WaitForMoreData();
                return; //Must wait for more data
            }
            P->Streams[0].codecid=vidfmt==Elements::NONE?0:vidfmt;
            P->Streams[1].codecid=audfmt==Elements::NONE?0:audfmt;
            if (framerate_idx)
            {
                float64 FrameRate;
                if (!(framerate_idx>>7))
                    FrameRate=framerate_idx;
                else
                {
                    int8u T=(framerate_idx&0x7F)>>2;
                    float64 S;
                    if (T<16)
                        S=((float64)1)/(T+1);
                    else
                        S=T-1;
                    if (framerate_idx&1)
                        S=S/1.001;
                    FrameRate=S*Nsv_FrameRate_Multiplier[framerate_idx&3];
                }
                if (FrameRate)
                    FrameInfo.DUR=float64_int64s(1000000000/FrameRate);
                FrameInfo.PTS=0;
            }
            if (width)
                Fill(Stream_Video, 0, Video_Width, width, 10, true);
            if (height)
                Fill(Stream_Video, 0, Video_Height, height, 10, true);
        }
        if (P->AudioDelay!=numeric_limits<int64s>::min())
            P->AudioDelay=FrameInfo.PTS==(int64u)-1?((int64u)-1):(FrameInfo.PTS+(int64u)syncoffs*1000000);
        if (!P->IsMajorSynched)
            P->IsMajorSynched=true;
    }
    else if ((sync_hdr>>16)==0xEFBE)
    {
        Skip_B2(                                                "nosync_hdr");
    }
    int32u aux_plus_video_len;
    int16u audio_len, AuxTotalLen=0;
    Get_L3 (aux_plus_video_len,                                 "aux_plus_video_len");
    Get_L2 (audio_len,                                          "audio_len");
    auto num_aux=aux_plus_video_len&0xF;
    int32u video_len=aux_plus_video_len>>4;
    for (auto i=0; i<num_aux; i++)
    {
        int16u aux_chunk_len;
        Get_L2 (aux_chunk_len,                                  "aux_chunk_len");
        Skip_C4(                                                "aux_chunk_type");
        AuxTotalLen+=aux_chunk_len;
    }

    FILLING_BEGIN();
        if (video_len<AuxTotalLen)
        {
            Trusted_IsNot("aux size too big");
            return;
        }
        video_len-=AuxTotalLen;
        if (video_len<=0x80000)
            P->Streams[0].len=video_len;
        else
        {
            P->Streams[0].len=(int32u)-1;
            video_len=0;
        }
        if (audio_len<=0x8000)
            P->Streams[1].len=audio_len;
        else
        {
            P->Streams[1].len=(int32u)-1;
            audio_len=0; // Found in some files, content is missing
        }
        P->AuxTotalLen=AuxTotalLen;
        Header_Fill_Code(0, "Frame");
        Header_Fill_Size(Element_Offset+video_len+audio_len);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Nsv::Data_Parse()
{
    if (P->IsMajorSynched)
        Element_Info1(Frame_Count);
    if (FrameInfo.PTS!=(int64u)-1)
        Element_Info1(FrameInfo.PTS/1000000000.0);

    for (int i=0; i<2; i++)
    {
        auto& Stream=P->Streams[i];
        if (Stream.len)
        {
            int32u len=Stream.len!=(int32u)-1?Stream.len:0;

            // Init
            if (!Stream.Parser)
            {
                if (!P->IsMajorSynched)
                {
                    if (len)
                    {
                        auto NewBuffer=new int8u[Stream.len];
                        memcpy(NewBuffer, Buffer+Buffer_Offset+(size_t)Element_Offset, len);
                        Stream.Buffers.emplace_back(NewBuffer, Stream.len);
                        Skip_XX(Stream.len,                     "stream"); Param_Info1(i);
                    }
                    continue;
                }

                Stream_Prepare(Stream_Type[i]);
                Fill(Stream_Type[i], 0, Fill_Parameter(Stream_Type[i], Generic_CodecID), Ztring().From_CC4(Stream.codecid));
                File__Analyze* Parser;
                switch(Stream_Type[i])
                {
                    case Stream_Video:
                                        switch (Stream.codecid)
                                        {
                                            case Elements::DIVX:
                                            case Elements::XVID:
                                                                    #if defined(MEDIAINFO_MPEG4V_YES)
                                                                        Parser=new File_Mpeg4v();
                                                                    #else
                                                                        Parser=new File_Unknown();
                                                                        Open_Buffer_Init(Parser);
                                                                        Parser->Stream_Prepare(Stream_Video);
                                                                        Parser->Fill(Stream_Video, 0, Video_Format, "MPEG-4 Visual");
                                                                    #endif //defined(MEDIAINFO_MPEG4V_YES)
                                                                    break;
                                            case Elements::H264:
                                                                    #if defined(MEDIAINFO_AVC_YES)
                                                                        Parser=new File_Avc();
                                                                        ((File_Avc*)Parser)->FrameIsAlwaysComplete=true;
                                                                    #else
                                                                        Parser=new File_Unknown();
                                                                        Open_Buffer_Init(Parser);
                                                                        Parser->Stream_Prepare(Stream_Video);
                                                                        Parser->Fill(Stream_Video, 0, Video_Format, "AVC");
                                                                    #endif //defined(MEDIAINFO_AVC_YES)
                                                                    break;
                                            case Elements::RGB3:
                                                                    Parser=new File_Unknown;
                                                                    Open_Buffer_Init(Parser);
                                                                    Parser->Stream_Prepare(Stream_Video);
                                                                    Parser->Fill(Stream_Video, 0, Video_Format, "RGB");
                                                                    Parser->Fill(Stream_Video, 0, Video_ColorSpace, "RGB");
                                                                    Parser->Fill(Stream_Video, 0, Video_BitDepth, 8);
                                                                    break;
                                            case Elements::VP3_:
                                            case Elements::VP30:
                                            case Elements::VP31:
                                            case Elements::VP4_:
                                            case Elements::VP40:
                                            case Elements::VP5_:
                                            case Elements::VP50:
                                            case Elements::VP6_:
                                            case Elements::VP60:
                                            case Elements::VP61:
                                            case Elements::VP62:
                                                                    Parser=new File_Unknown;
                                                                    Open_Buffer_Init(Parser);
                                                                    Parser->Stream_Prepare(Stream_Video);
                                                                    Parser->Fill(Stream_Video, 0, Video_Format, "VP"+string(1, (Stream.codecid>>8)&0xFF));
                                                                    break;
                                            case Elements::YV12:
                                                                    Parser=new File_Unknown;
                                                                    Open_Buffer_Init(Parser);
                                                                    Parser->Stream_Prepare(Stream_Video);
                                                                    Parser->Fill(Stream_Video, 0, Video_Format, "YUV");
                                                                    Parser->Fill(Stream_Video, 0, Video_ColorSpace, "YUV");
                                                                    Parser->Fill(Stream_Video, 0, Video_ChromaSubsampling, "4:2:0");
                                                                    Parser->Fill(Stream_Video, 0, Video_BitDepth, 8);
                                                                    break;
                                            default            :    Parser=new File_Unknown();
                                        }
                                        break;
                    case Stream_Audio:
                                        switch (Stream.codecid)
                                        {
                                            case Elements::AAC_:
                                            case Elements::AACP:
                                            case Elements::AAV_:
                                            case Elements::VLB_:
                                                                    #if defined(MEDIAINFO_AAC_YES)
                                                                        Parser=new File_Aac();
                                                                        ((File_Aac*)Parser)->Mode=File_Aac::Mode_ADTS;
                                                                    #else
                                                                        Parser=new File_Unknown();
                                                                        Open_Buffer_Init(Parser);
                                                                        Parser->Stream_Prepare(Stream_Audio);
                                                                        Parser->Fill(Stream_Audio, 0, Audio_Format, "AAC");
                                                                        Parser->Fill(Stream_Audio, 0, Audio_MuxingMode, "ADTS");
                                                                    #endif //defined(MEDIAINFO_AAC_YES)
                                                                    break;
                                            case Elements::MP3_:
                                                                    #if defined(MEDIAINFO_MPEGA_YES)
                                                                        Parser=new File_Mpega();
                                                                    #else
                                                                        Parser=new File_Unknown();
                                                                        Open_Buffer_Init(Parser);
                                                                        Parser->Stream_Prepare(Stream_Audio);
                                                                        Parser->Fill(Stream_Audio, 0, Audio_Format, "MPEG Audio");
                                                                    #endif //defined(MEDIAINFO_MPEGA_YES)
                                                                    break;
                                            case Elements::PCM_:
                                                                    Parser=new File_Unknown();
                                                                    Open_Buffer_Init(Parser);
                                                                    Parser->Stream_Prepare(Stream_Audio);
                                                                    Parser->Fill(Stream_Audio, 0, Audio_Format, "PCM");
                                                                    if (len>=4)
                                                                    {
                                                                        auto AudioBuffer=Buffer+Buffer_Offset+(size_t)Element_Offset;
                                                                        Parser->Fill(Stream_Audio, 0, Audio_BitDepth, LittleEndian2int8u(AudioBuffer++));
                                                                        Parser->Fill(Stream_Audio, 0, Audio_Channel_s_, LittleEndian2int8u(AudioBuffer++));
                                                                        Parser->Fill(Stream_Audio, 0, Audio_SamplingRate, LittleEndian2int16u(AudioBuffer));
                                                                    }
                                                                    break;
                                            case Elements::SPX_:
                                                                    Parser=new File_Unknown();
                                                                    Open_Buffer_Init(Parser);
                                                                    Parser->Stream_Prepare(Stream_Audio);
                                                                    Parser->Fill(Stream_Audio, 0, Audio_Format, "Speex");
                                                                    break;
                                            default            :    Parser=new File_Unknown();
                                        }
                                        break;
                    default          :  Parser=new File_Unknown();
                }
                Open_Buffer_Init(Parser);
                Parser->FrameInfo.PTS=i?P->AudioDelay:0;
                Stream.Parser=Parser;
            }

            // Parse
            Element_Begin1("stream");
            Element_Info1(i);
            Element_Code=i;
            if (!Stream.Buffers.empty())
            {
                Stream.Parser->FrameInfo.DTS=0;
                Stream.Parser->FrameInfo.PTS=0;
                for (const auto& Buffer : Stream.Buffers)
                {
                    Open_Buffer_Continue(Stream.Parser, Buffer.Data, Buffer.Size);
                    if (Stream.Parser->Status[IsAccepted])
                        Demux(Buffer.Data, Buffer.Size, ContentType_MainStream);
                    delete[] Buffer.Data;
                }
                if (Stream.Parser->Status[IsAccepted] && Stream.Parser->FrameInfo.PTS!=(int64u)-1)
                {
                    if (i)
                    {
                        if (P->AudioDelay!=numeric_limits<int64s>::min())
                            P->AudioDelay-=Stream.Parser->FrameInfo.PTS;
                    }
                    else
                    {
                        if (FrameInfo.PTS!=(int64u)-1)
                            FrameInfo.PTS-=Stream.Parser->FrameInfo.PTS;
                    }
                }
                Stream.Buffers.clear();
            }
            Open_Buffer_Continue(Stream.Parser, len);
            if (Stream.Parser->Status[IsAccepted])
            {
                Demux(Buffer+Buffer_Offset+(size_t)Element_Offset-len, len, ContentType_MainStream);
                if (FrameInfo.DUR!=-1)
                {
                    switch(Stream_Type[i])
                    {
                        case Stream_Video:
                                            if (Retrieve(Stream_Video, 0, Video_Delay).empty())
                                                Fill(Stream_Video, 0, Video_Delay, float64_int64s(((float64)FrameInfo.PTS)/1000000));
                                            break;
                        case Stream_Audio:
                                            if (P->AudioDelay!=numeric_limits<int64s>::min() && Retrieve(Stream_Audio, 0, Audio_Delay).empty())
                                                Fill(Stream_Audio, 0, Audio_Delay, float64_int64s(((float64)P->AudioDelay)/1000000));
                                            break;
                    }
                }
            }
            else if (i)
                P->AudioDelay=numeric_limits<int64s>::min();
            #if MEDIAINFO_TRACE
            else
            {
                Element_Show();
                Element_Offset-=len;
                Skip_XX(len,                                    "Can not be decoded");
            }
            #endif //MEDIAINFO_TRACE
            Element_End0();
        }
    }

    if (P->IsMajorSynched)
        Frame_Count++;
    if (FrameInfo.PTS!=(int64u)-1 && FrameInfo.DUR!=(int64u)-1)
        FrameInfo.PTS+=FrameInfo.DUR;
    if (Config->ParseSpeed<1.0
     && (Frame_Count>=300
      || (P->IsMajorSynched
       && (!P->Streams[0].codecid || (P->Streams[0].Parser && P->Streams[0].Parser->Status[IsAccepted]))
       && (!P->Streams[1].codecid || (P->Streams[1].Parser && P->Streams[1].Parser->Status[IsAccepted])))))
        Finish();
}

//***************************************************************************
// StarDiva
//***************************************************************************

//---------------------------------------------------------------------------
void File_StarDiva::Read_Buffer_Continue()
{
    int32u Magic;
    Get_B4(Magic, "Always 0x4A4C1A00");
    if (Magic!=0x4A4C1A00)
    {
        Skip_XX(Element_Size-Element_Offset,                    "Unknown");
        Reject();
        return;
    }

    Accept();
    Stream_Prepare(Stream_Menu);
    Fill(Stream_Menu, 0, Menu_Format, "StarDiva");


    size_t Begin;
    size_t End;

    Element_Begin1("StarDiva time line data");
        Element_Begin1("Header");
            int32u JL_TotalSize;
            Get_B4 (JL_TotalSize,                               "Time line data total size");
            if (JL_TotalSize<16 && JL_TotalSize>0x49000)
            {
                Element_End0();
                Element_End0();
                Skip_XX(Element_Size-Element_Offset,            "Unknown");
                Fill(Stream_Menu, 0, "Errors", "StarDiva metadata");
                return;
            }
        Element_End0();
        size_t JL_End=Element_Offset-8+JL_TotalSize;

        Element_Begin1("Footer");
            Element_Offset=JL_End-16;
            int32u Footer_Flags1, Footer_Offset1, Footer_Size1, Footer_Offset2;
            Get_B4 (Footer_Flags1,                              "0x80000000");
            Get_B4 (Footer_Offset1,                             "Offset to footer");
            Get_B4 (Footer_Size1,                               "0x80000000 + diff of offset?");
            Get_B4 (Footer_Offset2,                             "Offset to stats?");
            int32u Footer_Size1_31=Footer_Size1>>31;
            Footer_Size1&=0x7FFFFFFF;
            if (Footer_Flags1!=0x80000000
                || Footer_Offset1!=JL_TotalSize-16
                || Footer_Offset2>=Footer_Offset1
                || Footer_Size1_31!=1
                || Footer_Size1!=Footer_Offset1-Footer_Offset2
                || (Footer_Size1!=0x35 && Footer_Size1!=0x36)
                )
            {
                Element_End0();
                Element_End0();
                Skip_XX(Element_Size-Element_Offset,            "Unknown");
                Fill(Stream_Menu, 0, "Errors", "StarDiva metadata");
                return;
            }
        Element_End0();

        Element_Begin1("Stats?");
            Element_Offset=Footer_Offset2;
            int8u Stats_Byte0, Stats_Byte1, Stats_Byte32, Stats_Byte33, Stats_Byte34, Stats_Byte35;
            Get_B1 (Stats_Byte0,                                "0x80");
            Get_B1 (Stats_Byte1,                                "0xB0");
            bool Stats_Metadata_NotFound=memcmp(Buffer+Buffer_Offset+Element_Offset, "metadata[time:S,offset:I,typ:I,data:S,speaker:S]", 0x30);
            Skip_String(0x30,                                   "metadata string");
            Get_B1 (Stats_Byte32,                               "0x81");
            Get_B1 (Stats_Byte33,                               "Unknown");
            Get_B1 (Stats_Byte34,                               "Unknown");
            if (Footer_Size1>=0x36)
                Get_B1 (Stats_Byte35,                           "Unknown");
            if (Stats_Byte0!=0x80
                || Stats_Byte1!=0xB0
                || Stats_Metadata_NotFound
                || Stats_Byte32!=0x81
            )
            {
                Element_End0();
                Element_End0();
                Skip_XX(Element_Size-Element_Offset,            "Unknown");
                Fill(Stream_Menu, 0, "Errors", "StarDiva metadata");
                return;
            }
            bool Skip5ZeroesBeforeAndAfterFlags=Footer_Size1==0x35 && Stats_Byte32==0x81 && Stats_Byte33==0x91 && Stats_Byte34==0xB1;
            JL_End=Footer_Offset2; // All after Footer_Offset2 is already parsed
        Element_End0();

        vector<string> Times;
        Element_Begin1("Times");
            Element_Offset=8;
            while (Buffer[Element_Offset]!=0x99 && Buffer[Element_Offset]!=0x09)
            {
                string Time;
                Get_String(8, Time,                             "Time");
                bool Problem;
                if (Time.size()<8
                    || Time[0]<'0' || Time[0]>'2'
                    || Time[1]<'0' || Time[1]>'9'
                    || Time[2]!=':'
                    || Time[3]<'0' || Time[3]>'5'
                    || Time[4]<'0' || Time[4]>'9'
                    || Time[5]!=':'
                    || Time[6]<'0' || Time[6]>'5'
                    || Time[7]<'0' || Time[7]>'9'
                    )
                    Problem=true;
                else
                    Problem=false;
                if (Problem || Element_Offset>=JL_End || Buffer[Element_Offset])
                {
                    Element_End0();
                    Element_End0();
                    Skip_XX(Element_Size-Element_Offset,        "Unknown");
                    Fill(Stream_Menu, 0, "Errors", "StarDiva metadata");
                    return;
                }
                Times.push_back(Time);
                Element_Offset++;
            }
            Element_Info1(Ztring(Ztring::ToZtring(Times.size())+__T(" lines")).To_UTF8().c_str());
        Element_End0();

        Element_Begin1("Always 9 - 4 bits per line");
            BS_Begin_LE();
            for (size_t i=0; i<Times.size(); i++)
            {
                int8u Nine;
                Get_T1(4, Nine,                                 "9");
                if (Nine!=9)
                {
                    Element_End0();
                    Element_End0();
                    Skip_XX(Element_Size-Element_Offset,        "Unknown");
                    Fill(Stream_Menu, 0, "Errors", "StarDiva metadata");
                    return;
                }
            }
            if (Times.size()%2)
            {
                int8u Padding;
                Get_T1(4, Padding, "Padding");
                if (Padding)
                {
                    Element_End0();
                    Element_End0();
                    Skip_XX(Element_Size-Element_Offset,        "Unknown");
                    Fill(Stream_Menu, 0, "Errors", "StarDiva metadata");
                    return;
                }
            }
            BS_End_LE();
        Element_End0();

        // Zeroes in some specific cases
        if (Skip5ZeroesBeforeAndAfterFlags)
        {
            int64u Zero;
            Get_B5 (Zero,                                       "0x0000000000");
            if (Zero)
            {
                Element_End0();
                Skip_XX(Element_Size-Element_Offset,            "Unknown");
                Fill(Stream_Menu, 0, "Errors", "StarDiva metadata");
                return;
            }
        }

        // Offsets
        vector<int32u> Offsets;
        if (!Skip5ZeroesBeforeAndAfterFlags)
        {
            // Guessing size of offset values
            int OffsetValueSize=0;
            if (Times.size()>2)
            {
                for (int j=2; j<=4; j+=2)
                {
                    int32u Previous=0;
                    bool IsNOK=false;
                    size_t i_Max=Times.size();
                    if (j==4 && !OffsetValueSize && i_Max>8) // Ignoring some values, found one file with buggy values at the end, accepting that in case that there are enough values
                        i_Max=8;
                    for (size_t i=0; i<i_Max; i++)
                    {
                        size_t CurrentOffset=Element_Offset+i*j;
                        int32u Current;
                        switch (j)
                        {
                        case 2: Current=LittleEndian2int16u(Buffer+CurrentOffset); break;
                        case 4: Current=LittleEndian2int32u(Buffer+CurrentOffset); break;
                        }
                        if (Current<Previous)
                        {
                            IsNOK=true;
                            break;
                        }
                        Previous=Current;
                    }
                    if (!IsNOK)
                        OffsetValueSize+=j;
                }
            }
            if (Times.size()<=2 || OffsetValueSize>4)
            {
                // Not enough offsets, using an empirical test which may fail if first offset is not 0
                if (Footer_Size1==0x36)
                    OffsetValueSize=4;
                else if (Footer_Size1==0x35)
                {
                    if (BigEndian2int32u(Buffer+Element_Offset))
                        OffsetValueSize=2;
                    else
                        OffsetValueSize=4;
                }
            }
            if (!OffsetValueSize || OffsetValueSize>4)
            {
                Element_End0();
                Skip_XX(Element_Size-Element_Offset,            "Unknown");
                Fill(Stream_Menu, 0, "Errors", "StarDiva metadata");
                return;
            }

            Element_Begin1("Offsets");
                for (size_t i=0; i<Times.size(); i++)
                {
                    int32s Offset;
                    if (OffsetValueSize==4)
                    {
                        int32u Offset32;
                        Get_L4(Offset32,                        "Offset");
                        Offset=(int32s)Offset32;
                    }
                    else
                    {
                        int16u Offset16;
                        Get_L2(Offset16,                        "Offset");
                        Offset=(int16s)Offset16;
                    }
                    if (Offset<0)
                    {
                        Offset+=48*3600*1000; // Hack: add 24 hours in order to keep the description
                    }
                    Offsets.push_back(Offset);
                }
            Element_End0();
        }

        // Flags
        vector<bool> HasSeq;
        vector<bool> HasAgenda;
        vector<bool> HasSpeaker;
        vector<size_t> Seqs;
        size_t HasSeqAgenda_Count=0;
        size_t HasSpeaker_Count=0;
        map<int8u, size_t> Flags_Count;
        Element_Begin1("Flags");
            BS_Begin_LE();
            for (size_t i=0; i<Times.size(); i++)
            {
                int8u Flags;
                Get_T1(4, Flags,                                "Flags");
                Flags_Count[Flags]++;
                switch (Flags)
                {
                    case 0x1:
                    case 0x2:
                    case 0x3:
                    case 0xC:
                    case 0xD:
                        break;
                    default:
                    {
                        Element_End0();
                        Element_End0();
                        Skip_XX(Element_Size-Element_Offset,    "Unknown");
                        Fill(Stream_Menu, 0, "Errors", "StarDiva metadata");
                        return;
                    }
                }
                switch (Flags)
                {
                    case 0x3:
                        HasAgenda.push_back(false);
                        HasSpeaker.push_back(true);
                        HasSpeaker_Count++;
                        break;
                    default:
                        HasAgenda.push_back(true);
                        HasSeqAgenda_Count++;
                        HasSpeaker.push_back(false);
                }
                switch (Flags)
                {
                    case 0xD:
                        Seqs.push_back(i);
                        HasSeq.push_back(true);
                        break;
                    default:
                        HasSeq.push_back(false);
                }
                if (HasAgenda.back())
                    Param_Info1(HasSeq.back()?"Seq":"Agenda");
                if (HasSpeaker.back())
                    Param_Info1("Speaker");
            }
            if (Times.size()%2)
            {
                int8u Flags;
                Get_T1(4, Flags, "Padding"); // Note: not checking 0 value because some files with non 0 values were found
            }
            BS_End_LE();
            Element_Info2(HasSeqAgenda_Count, " seq+agendas");
            Element_Info2(HasSpeaker_Count, " speakers");
            for (const auto& Count : Flags_Count)
                Element_Info2(Count.second, (Ztring(__T(" Value ")+Ztring::ToZtring(Count.first))).To_UTF8().c_str());
        Element_End0();

        // Zeroes in some specific cases
        if (Skip5ZeroesBeforeAndAfterFlags)
        {
            int64u Zero;
            Get_B5 (Zero, "0x0000000000");
            if (Zero)
            {
                Element_End0();
                Skip_XX(Element_Size-Element_Offset,            "Unknown");
                Fill(Stream_Menu, 0, "Errors", "StarDiva metadata");
                return;
            }
        }

        vector<string> SeqAgendas;
        Element_Begin1("Seqs + Agendas");
            for (size_t Pos=0; Pos<HasAgenda.size(); Pos++)
            {
                if (!HasAgenda[Pos])
                {
                    SeqAgendas.emplace_back(string());
                    continue;
                }
                Begin=Element_Offset;
                End=Element_Offset;
                while (Buffer[End])
                    End++;
                Element_Offset=Begin;
                string SeqAgenda;
                Get_String(End-Begin, SeqAgenda,                "Seq + Agenda");
                SeqAgendas.push_back(SeqAgenda);
                Element_Offset++;
            }
            SeqAgendas.resize(Times.size());
        Element_End0();

        // Unknown bytes before speakers
        if (HasSpeaker_Count)
        {
            // Note: found 0.5, 1 or 2 times the count of lines
            size_t Max=JL_TotalSize;
            map<int, size_t> CheckOfMultiplesOfTimes2x_Values;
            int CheckOfMultiplesOfTimes2x=4;
            int CheckOfMultiplesOfTimes2x_Previous=CheckOfMultiplesOfTimes2x;
            for (;;)
            {
                size_t i=Element_Offset+Times.size()*CheckOfMultiplesOfTimes2x/2;
                if (CheckOfMultiplesOfTimes2x==1 && Times.size()%2)
                    i++; // Padding in case of 0.5 byte per line

                size_t Count=0;
                size_t StringSizes=0;
                size_t MinStringSizes=(size_t)-1;
                for (size_t j=i; j<Max; j++)
                {
                    // Check of the terminating null byte
                    if (!Buffer[j])
                    {
                        if (MinStringSizes>StringSizes)
                            MinStringSizes=StringSizes;
                        StringSizes=0;
                        Count++;
                        if (Count==HasSpeaker_Count)
                            break;
                        j++;
                        if (j>=Max)
                            break;
                    }
                    
                    // Check that it may be a valid char
                    if (Buffer[j]<0x20)
                    {
                        break;
                    }
                    StringSizes++;
                }
                if (Count==HasSpeaker_Count && MinStringSizes)
                {
                    // Check if it is part of the previous catch
                    bool PartOfPrevious=false;
                    if (CheckOfMultiplesOfTimes2x_Previous!=CheckOfMultiplesOfTimes2x)
                    {
                        PartOfPrevious=true;
                        size_t j_Max=Element_Offset+CheckOfMultiplesOfTimes2x_Previous/2*Times.size();
                        for (size_t j=i; j<j_Max; j++)
                        {
                            if (Buffer[j]<0x20)
                                PartOfPrevious=false;
                        }
                    }
                    if (PartOfPrevious)
                    {
                        const auto& Previous=CheckOfMultiplesOfTimes2x_Values.find(CheckOfMultiplesOfTimes2x_Previous);
                        if (Previous!=CheckOfMultiplesOfTimes2x_Values.end())
                            CheckOfMultiplesOfTimes2x_Values.erase(Previous);
                    }
                    CheckOfMultiplesOfTimes2x_Values[CheckOfMultiplesOfTimes2x]=MinStringSizes;
                    CheckOfMultiplesOfTimes2x_Previous=CheckOfMultiplesOfTimes2x;
                }

                CheckOfMultiplesOfTimes2x/=2;
                if (!CheckOfMultiplesOfTimes2x)
                    break;
            }
            size_t CheckOfMultiplesOfTimes2x_Value=0;
            if (CheckOfMultiplesOfTimes2x_Values.size()>1)
            {
                // Eliminate obviously wrong catches: 1 char long but another catch has longer strings
                size_t MinMinStringSize=(size_t)-1;
                size_t MaxMinStringSize=0;
                for(map<int, size_t>::iterator It=CheckOfMultiplesOfTimes2x_Values.begin(); It!=CheckOfMultiplesOfTimes2x_Values.end(); It++)
                {
                    if (MinMinStringSize>It->second)
                        MinMinStringSize=It->second;
                    if (MaxMinStringSize<It->second)
                        MaxMinStringSize=It->second;
                }
                if (MinMinStringSize==1 && MaxMinStringSize>1)
                    for(map<int, size_t>::iterator It=CheckOfMultiplesOfTimes2x_Values.begin(); It!=CheckOfMultiplesOfTimes2x_Values.end(); It++)
                    {
                        if (It->second!=1)
                        {
                            if (CheckOfMultiplesOfTimes2x_Value)
                            {
                                // 2 catches have long strings, too risky
                                CheckOfMultiplesOfTimes2x_Value=0;
                                break;
                            }
                            CheckOfMultiplesOfTimes2x_Value=It->first;
                        }
                    }
                else if (CheckOfMultiplesOfTimes2x_Values.begin()->second==MaxMinStringSize) // If first one is the biggest one, take it
                    CheckOfMultiplesOfTimes2x_Value=CheckOfMultiplesOfTimes2x_Values.begin()->first;
            }
            if (CheckOfMultiplesOfTimes2x_Values.size()==1)
            {
                CheckOfMultiplesOfTimes2x_Value=CheckOfMultiplesOfTimes2x_Values.begin()->first;
            }
            if (!CheckOfMultiplesOfTimes2x_Value)
            {
                Element_End0();
                Skip_XX(Element_Size-Element_Offset,            "Unknown");
                Fill(Stream_Menu, 0, "Errors", "StarDiva metadata");
                return;
            }
            const char* Name;
            switch (CheckOfMultiplesOfTimes2x_Value)
            {
                case 1 : Name="Unknown - 4 bits per line"; break;
                case 2 : Name="Unknown - 1 byte per line"; break;
                default: Name="Unknown - 2 bytes per line"; break;
            }
            Element_Begin1(Name);
                if (CheckOfMultiplesOfTimes2x_Value==1)
                    BS_Begin_LE();
                for (size_t i=0; i<Times.size(); i++)
                {
                    switch (CheckOfMultiplesOfTimes2x_Value)
                    {
                        case 1 : Skip_T1(4,                     "Unknown"); break;
                        case 2 : Skip_L1(                       "Unknown"); break;
                        default: Skip_L2(                       "Unknown"); break;
                    }
                    ;
                }
                if (CheckOfMultiplesOfTimes2x_Value==1)
                {
                    if (Times.size()%2)
                        Skip_T1(4,                              "Padding");
                    BS_End_LE();
                }
            Element_End0();
        }

        // Speakers
        vector<string> Speakers;
        if (HasSpeaker_Count)
        {
            Element_Begin1("Speakers");
                for (size_t Pos=0; Pos<HasSpeaker.size(); Pos++)
                {
                    if (!HasSpeaker[Pos])
                    {
                        Speakers.emplace_back(string());
                        continue;
                    }
                    Begin=Element_Offset;
                    End=Element_Offset;
                    while (Buffer[End])
                        End++;
                    Element_Offset=Begin;
                    string Speaker;
                    Get_String(End-Begin, Speaker,                  "Speaker");
                    Speakers.push_back(Speaker);
                    Element_Offset++;
                }
            Element_End0();
        }
        Speakers.resize(Times.size());

        // Unknown after speakers 
        Element_Begin1("Unknown");
            while (Element_Offset<JL_End)
            {
                Skip_L1("Unknown");
            }
        Element_End0();

        //Filling
        size_t Seqs_Pos=0;
        size_t Subtitles_Index=1;
        Offsets.resize(Times.size());
        Fill(Stream_Menu, StreamPos_Last, Menu_Chapters_Pos_Begin, Count_Get(Stream_Menu, StreamPos_Last), 10, true);
        for (size_t i = 0; i < Times.size(); i++)
        {
            bool Skip=false;
            if (Seqs_Pos<Seqs.size() && Seqs[Seqs_Pos]==i)
            {
                Seqs_Pos++;
                while (!SeqAgendas[i].empty() && SeqAgendas[i][0]!=' ')
                    SeqAgendas[i].erase(0, 1);
                if (SeqAgendas[i].size()>2 && SeqAgendas[i][0]==' ' && SeqAgendas[i][1]==':')
                    SeqAgendas[i].erase(0, 2);
            }

            if (!SeqAgendas[i].empty())
            {
                int32s Offset=Offsets[i];
                TimeCode TC(Offset, 999, TimeCode::Timed());
                string Time=TC.ToString();
                string Content;
                Content+=Times[i];
                Content+=" - ";
                if (!SeqAgendas[i].empty())
                {
                    if (SeqAgendas[i]=="Pause on")
                    {
                        bool HasPauseOff=i+1<Times.size() && SeqAgendas[i+1]=="Pause off" && Speakers[i].empty();
                        if (i+1+HasPauseOff<Times.size() && !Speakers[i+1+HasPauseOff].empty())
                            Content.insert(0, "+ ");
                        Content+="Pause";
                        if (HasPauseOff)
                        {
                            Content+=" - ";
                            Content+=Times[i+1];
                            Skip=true;
                        }
                    }
                    else
                        Content+=SeqAgendas[i];
                }
                if (!Content.empty() && Content.back()==' ') // Often seen
                    Content.pop_back();
                Fill(Stream_Menu, 0, Time.c_str(), Content);
            }

            if (!Speakers[i].empty())
            {
                int32s Offset=Offsets[i];
                TimeCode TC(Offset, 999, TimeCode::Timed());
                string Time=TC.ToString();
                string Content;
                Content += Times[i];
                Content += " - ";
                Content += Speakers[i];
                if (!Content.empty() && Content.back()==' ') // Often seen
                    Content.pop_back();
                Fill(Stream_Menu, 0, Time.c_str(), "+ "+Content);

            }
            if (Skip)
            {
                i++;
                Skip=false;
            }
        }
        Fill(Stream_Menu, StreamPos_Last, Menu_Chapters_Pos_End, Count_Get(Stream_Menu, StreamPos_Last), 10, true);
    Element_End0();

    Element_Offset=End;
    if (Buffer_Size==0x4B000)
        Skip_XX(0x4AC00-Element_Offset,                         "Zeroes");

    Element_Begin1("StarDiva file data");
        string Meeting, Commission, Room, ConferenceBegin, ConferenceEnd;
        Get_String(0x51, Meeting,                               "Meeting");
        Get_String(0x51, Commission,                            "Commission");
        Get_String(0x51, Room,                                  "Room");
        Skip_L4(                                                "Unknown");
        Skip_L4(                                                "Unknown");
        Skip_L4(                                                "Unknown");
        Get_String(0x13, ConferenceBegin,                       "Begin");
        Skip_L6(                                                "Zeroes");
        Get_String(0x13, ConferenceEnd,                         "End");
        Skip_L5(                                                "Zeroes");
        Skip_L4(                                                "Unknown");
        Skip_L4(                                                "Unknown");
        Skip_L4(                                                "Unknown");
        Skip_L4(                                                "Unknown");
        Skip_String(0x100,                                      "File name");
        Fill(Stream_General, 0, "Meeting", Meeting);
        Fill(Stream_General, 0, "Commission", Commission);
        Fill(Stream_General, 0, "Room", Room);
        auto AdaptDate = [](string& Date)
        {
            Date.erase(Date.find_last_not_of('\0') + 1);
            if (Date.size()!=19
             || Date[ 2]!='.'
             || Date[ 5]!='.'
             || Date[10]!=' '
             || Date[13]!=':'
             || Date[16]!=':')
                return;
            Date = Date.substr(6, 4) + '-' + Date.substr(3, 2) + '-' + Date.substr(0, 2) + Date.substr(10);
        };
        AdaptDate(ConferenceBegin);
        AdaptDate(ConferenceEnd);
        if (!ConferenceEnd.empty() && ConferenceEnd!=ConferenceBegin)
            Fill(Stream_General, 0, General_Recorded_Date, ConferenceBegin + " - " + ConferenceEnd);
        else
            Fill(Stream_General, 0, General_Recorded_Date, ConferenceBegin);
        Begin=(size_t)Element_Offset;
        End=Begin;
        while (End<Element_Size)
        {
            while (End<Element_Size && (Buffer[End]<0x20 || Buffer[End]>=0x7F))
            {
                if (Buffer[End])
                {
                    Element_Offset=End;
                    Skip_L1(                                    "Unknown");
                }
                End++;
            }
            if (End>=Element_Size)
                break;
            Begin=End;
            while (End<Element_Size && Buffer[End]>=0x20 && Buffer[End]<0x7F)
                End++;
            if (Begin<End)
            {
                Element_Offset=Begin;
                Skip_String(End-Begin,                          "Unknown");
            }
            End++;
        }
        Element_Offset=Element_Size;
    Element_End0();
}

} //NameSpace

#endif //MEDIAINFO_NSV_YES

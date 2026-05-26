/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Contributor: Lionel Duchateau, kurtnoise@free.fr
//
// From : http://www.webmproject.org/
// Specs: http://wiki.multimedia.cx/index.php?title=IVF
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
#if defined(MEDIAINFO_IVF_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File_Ivf.h"
#if defined(MEDIAINFO_AV1_YES)
    #include "MediaInfo/Video/File_Av1.h"
#endif
#if defined(MEDIAINFO_AV2_YES)
    #include "MediaInfo/Video/File_Av2.h"
#endif
#if defined(MEDIAINFO_VP8_YES)
    #include "MediaInfo/Video/File_Vp8.h"
#endif
#if defined(MEDIAINFO_VP9_YES)
    #include "MediaInfo/Video/File_Vp9.h"
#endif
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Ivf::FileHeader_Begin()
{
    //Synchro
    if (4>Buffer_Size)
        return false;
    if (Buffer[0]!=0x44 //"DKIF"
     || Buffer[1]!=0x4B
     || Buffer[2]!=0x49
     || Buffer[3]!=0x46)
    {
        Reject();
        return false;
    }
    if (6>Buffer_Size)
        return false;

    return true;
}

//---------------------------------------------------------------------------
void File_Ivf::FileHeader_Parse()
{
    //Parsing
    int32u fourcc, frame_rate_num, frame_rate_den, frame_count;
    int16u version, header_size, width, height;

    Skip_C4 (                                                   "Signature");
    Get_L2 (version,                                            "Version");
    if (version==0)
    {
        Get_L2 (header_size,                                    "Header Size");
        if (header_size>=32)
        {
            Get_C4 (fourcc,                                     "Fourcc");
            Get_L2 (width,                                      "Width");
            Get_L2 (height,                                     "Height");
            Get_L4 (frame_rate_num,                             "FrameRate Numerator");
            Get_L4 (frame_rate_den,                             "FrameRate Denominator");
            Get_L4 (frame_count,                                "Frame Count");
            Skip_L4(                                            "Unused");
            if (header_size-32)
                Skip_XX(header_size-32,                         "Unknown");
        }
        else
        {
            fourcc=0x00000000;
            width=0;
            height=0;
            frame_rate_num=0;
            frame_rate_den=0;
            frame_count=0;
        }
    }
    else
    {
        header_size=0;
        fourcc=0x00000000;
        width=0;
        height=0;
        frame_rate_num=0;
        frame_rate_den=0;
        frame_count=0;
    }

    FILLING_BEGIN();
        Accept("IVF");

        Fill(Stream_General, 0, General_Format, "IVF");

        if (version==0 && header_size>=32)
        {
            Stream_Prepare(Stream_Video);
            CodecID_Fill(Ztring().From_CC4(fourcc), Stream_Video, 0, InfoCodecID_Format_Riff);
            if (frame_rate_den)
                Fill(Stream_Video, 0, Video_FrameRate, (float)frame_rate_num / frame_rate_den);
            Fill(Stream_Video, 0, Video_FrameCount, frame_count);
            Fill(Stream_Video, 0, Video_Width, width);
            Fill(Stream_Video, 0, Video_Height, height);
            Fill(Stream_Video, 0, Video_StreamSize, File_Size-header_size-12*frame_count); //Overhead is 12 byte per frame
        }

        auto const Format = Retrieve(Stream_Video, 0, Video_Format).To_UTF8();
        #if defined(MEDIAINFO_AV1_YES)
            if (Format == "AV1") {
                MI.reset(new File_Av1());
            }
        #endif
        #if defined(MEDIAINFO_AV2_YES)
            if (Format == "AV2") {
                MI.reset(new File_Av2());
                static_cast<File_Av2*>(MI.get())->IsAnnexB = true;
            }
        #endif
        #if defined(MEDIAINFO_VP8_YES)
            if (Format == "VP8") {
                MI.reset(new File_Vp8());
            }
        #endif
        #if defined(MEDIAINFO_VP9_YES)
            if (Format == "VP9") {
                MI.reset(new File_Vp9());
            }
        #endif
    FILLING_END();

    if (MI) {
        MI->FrameIsAlwaysComplete = true;
        Open_Buffer_Init(MI.get());
    }
    else {
        //No more need data
        Finish("IVF");
    }
}

//---------------------------------------------------------------------------
void File_Ivf::Header_Parse()
{
    int32u frame_size;
    Get_L4 (frame_size,                                         "Frame Size");
    Skip_L8(                                                    "Presentation Timestamp");

    FILLING_BEGIN();
    Header_Fill_Size(frame_size + 12LL);
    Header_Fill_Code(0x0, "Frame");
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Ivf::Data_Parse()
{
    Open_Buffer_Continue(MI.get());
    if (MI->Status[IsFinished]) {
        Open_Buffer_Finalize(MI.get());
        Merge(*MI, Stream_Video, 0, 0, false);
        Finish();
    }
}

} //NameSpace

#endif //MEDIAINFO_IVF_*


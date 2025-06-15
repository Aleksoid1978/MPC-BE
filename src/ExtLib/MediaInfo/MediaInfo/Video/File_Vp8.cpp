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
#if defined(MEDIAINFO_VP8_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Video/File_Vp8.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include "ZenLib/BitStream.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
namespace MediaInfoLib
{
//---------------------------------------------------------------------------

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Vp8::File_Vp8()
:File__Analyze()
{
    //Configuration
    ParserName="VP8";
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
    StreamSource=IsStream;
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE

    //In
    Frame_Count_Valid=1;
    StreamKind=Stream_Video;
}

//---------------------------------------------------------------------------
File_Vp8::~File_Vp8()
{
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Vp8::Streams_Accept()
{
    Stream_Prepare(StreamKind);
    Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Format), "VP8");
    Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Codec), "VP8");
    Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_BitDepth), 8);
    Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_ColorSpace), "YUV");
}

//---------------------------------------------------------------------------
void File_Vp8::Streams_Update()
{
}

//---------------------------------------------------------------------------
void File_Vp8::Streams_Finish()
{
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Vp8::Read_Buffer_Continue()
{
    BS_Begin_LE(); //VP8 bitstream is Little Endian
    bool frame_type;
    Get_TB (    frame_type,                                     "frame type");
    Skip_T1( 3,                                                 "version number");
    Skip_TB(                                                    "show_frame flag");
    Skip_T4(19,                                                 "size of the first data partition");
    BS_End_LE();
    if (!frame_type) { //I-Frame
        int32u start_code;
        int16u width, height;
        Get_B3 (start_code,                                     "start code");
        if (start_code != 0x9D012A) {
            Trusted_IsNot("start code");
            return;
        }
        Get_L2 (width,                                          "width");
        Get_L2 (height,                                         "height");

        FILLING_BEGIN();
            if (!Status[IsAccepted]) {
                Accept();
                Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Width), width & 0x3FFF);
                Fill(StreamKind_Last, 0, Fill_Parameter(StreamKind_Last, Generic_Height), height & 0x3FFF);
            }
        FILLING_END();
    }
    Skip_XX(Element_Size - Element_Offset,                      "(Data)");
    if (frame_type && !Frame_Count) {
        return;
    }

    FILLING_BEGIN();
        Frame_Count++;
        if (Frame_Count>=Frame_Count_Valid)
            Finish();
    FILLING_END();
}

} //NameSpace

#endif //MEDIAINFO_VP8_YES

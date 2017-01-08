/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about PCM files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_PcmH
#define MediaInfo_File_PcmH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include <deque>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Pcm
//***************************************************************************

class File_Pcm : public File__Analyze
{
public :
    //In
    int64u          Frame_Count_Valid;
    ZenLib::Ztring  Codec;
    int32u          SamplingRate;
    int8u           BitDepth;
    int8u           BitDepth_Significant;
    int8u           Channels;
    int8u           Endianness;
    int8u           Sign;

    //Buffer - Global
    void Read_Buffer_Continue ();
    #if MEDIAINFO_DEMUX
    struct demux_item
    {
        int64u Size;
        int64u DTS;
        int64u DUR;
    };
    std::deque<demux_item> Demux_Items;
    #endif //MEDIAINFO_DEMUX

    //Constructor/Destructor
    File_Pcm();

private :
    //Streams management
    void Streams_Fill();
    void Streams_Finish();

    //Buffer - File header
    bool FileHeader_Begin();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();
};

} //NameSpace

#endif

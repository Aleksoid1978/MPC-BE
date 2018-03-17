/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_Av1H
#define MediaInfo_Av1H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/File__Duplicate.h"
#include <cmath>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Av1
//***************************************************************************

class File_Av1 : public File__Analyze
{
public :
    //In
    int64u Frame_Count_Valid;
    bool   FrameIsAlwaysComplete;

    //Constructor/Destructor
    File_Av1();
    ~File_Av1();

private :
    File_Av1(const File_Av1 &File_Av1); //No copy
    File_Av1 &operator =(const File_Av1 &); //No copy

    //Streams management
    void Streams_Accept();
    void Streams_Fill();
    void Streams_Finish();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void sequence_header();
    void temporal_delimiter();
    void frame_header();
    void tile_group();
    void metadata();
    void metadata_hdr_cll();
    void metadata_hdr_mdcv();
    void padding();

    //Temp
    Ztring  MasteringDisplay_ColorPrimaries;
    Ztring  MasteringDisplay_Luminance;
    int16u  maximum_content_light_level;
    int16u  maximum_frame_average_light_level;
    bool  sequence_header_Parsed;
    bool  SeenFrameHeader;
    string GOP;

    //Helpers
    std::string GOP_Detect(std::string PictureTypes);
};

} //NameSpace

#endif

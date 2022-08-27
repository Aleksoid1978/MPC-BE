/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_Mpeg4_TimeCodeH
#define MediaInfo_File_Mpeg4_TimeCodeH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Mpeg4_TimeCode
//***************************************************************************

class File_Mpeg4_TimeCode : public File__Analyze
{
public :
    //In
    int8u   NumberOfFrames;
    bool    DropFrame;
    bool    NegativeTimes;
    int64u  FrameMultiplier;
    int64s  FirstEditOffset;
    int64u  FirstEditDuration;
    int64u  tkhd_Duration;
    int64u  mvhd_Duration_TimeScale;
    int64u  mdhd_Duration;
    int64u  mdhd_Duration_TimeScale;
    int64u  tmcd_Duration;
    int64u  tmcd_Duration_TimeScale;

    //Out
    int64s  Pos;
    int64s  Pos_Last;
    int64u  FrameMultiplier_Pos;

    //Constructor/Destructor
    File_Mpeg4_TimeCode();

protected :
    //Streams management
    void Streams_Accept();
    void Streams_Fill();
    void Streams_Finish();

    //Buffer - Global
    void Read_Buffer_Init();
    void Read_Buffer_Continue();
};

} //NameSpace

#endif

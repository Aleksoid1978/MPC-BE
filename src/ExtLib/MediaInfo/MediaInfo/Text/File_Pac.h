/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about Screen Electronics PAC subtitle files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_PacH
#define MediaInfo_File_PacH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/TimeCode.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Pac
//***************************************************************************

class File_Pac : public File__Analyze
{
public :
    //Constructor/Destructor
    File_Pac();
    ~File_Pac();

private :
    //Streams management
    void Streams_Accept();
    void Streams_Finish();

    //Buffer - File header
    void FileHeader_Parse();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Temp
    uint64_t Frame_Count_Metadata = 0;
    uint64_t EmptyCount = 0;
    uint64_t LineCount = 0;
    uint64_t Delay = 0;
    size_t MaxCountOfLinesPerFrame = 0;
    size_t MaxCountOfCharsPerLine = 0;
    size_t Count_1byte7 = 0;
    size_t Count_1byte8 = 0;
    size_t Count_2byte = 0;
    size_t Count_UTF8 = 0;
    size_t Codepage_Pos = 0;
    TimeCode Time_Start_Command;
    TimeCode Time_Start_Command_Temp;
    TimeCode Time_End_Command;
    TimeCode Time_Start;
    TimeCode Time_End;
    TimeCode Time_Delay;
};

} //NameSpace

#endif

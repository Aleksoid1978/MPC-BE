/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about N19 files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_N19H
#define MediaInfo_File_N19H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_N19
//***************************************************************************

class File_N19 : public File__Analyze
{
public :
    File_N19();
    ~File_N19();

private :
    //Buffer - File header
    bool FileHeader_Begin();
    void FileHeader_Parse();

    //Buffer - Global
    #if MEDIAINFO_SEEK
    size_t Read_Buffer_Seek (size_t Method, int64u Value, int64u ID);
    #endif //MEDIAINFO_SEEK

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Temp
    int32u TCI_FirstFrame;
    int16u LineCount;
    int16u LineCount_Max;
    int8u  FrameRate;
    int8u  CharSet;
    bool   Line_HasContent;
    size_t TotalLines;
    #if MEDIAINFO_DEMUX
        int32u TCO_Previous;
        int8u  Row_Max;
        int8u  Column_Max;
        bool   IsTeletext;
        wchar_t** Row_Values;
    #endif //MEDIAINFO_DEMUX
};

} //NameSpace

#endif

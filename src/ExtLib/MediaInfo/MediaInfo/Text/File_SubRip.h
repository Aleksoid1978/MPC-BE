/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about SubRip files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_SubRipH
#define MediaInfo_File_SubRipH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include <vector>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_SubRip
//***************************************************************************

class File_SubRip : public File__Analyze
{
public :
    File_SubRip();

private :
    //Buffer - File header
    bool FileHeader_Begin();

    //Buffer - Global
    #if MEDIAINFO_SEEK
    size_t Read_Buffer_Seek (size_t Method, int64u Value, int64u ID);
    #endif //MEDIAINFO_SEEK
    void Read_Buffer_Continue();

    //Temp
    bool IsVTT;
    bool HasBOM;
    #if MEDIAINFO_DEMUX
    struct item
    {
        int64u PTS_Begin;
        int64u PTS_End;
        Ztring Content;
    };
    std::vector<item> Items;
    #endif //MEDIAINFO_DEMUX
};

} //NameSpace

#endif

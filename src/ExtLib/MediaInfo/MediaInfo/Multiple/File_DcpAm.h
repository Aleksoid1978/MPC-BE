/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about DCP AssetMap files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_DcpAmH
#define MediaInfo_File_DcpAmH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/File__HasReferences.h"
#include "MediaInfo/Multiple/File_DcpPkl.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_DcpAm
//***************************************************************************

class File_DcpAm : public File__Analyze, File__HasReferences
{
public :
    //Constructor/Destructor
    File_DcpAm();

    //Streams
    File_DcpPkl::streams Streams;

private :
    //Streams management
    void Streams_Finish ();

    //Buffer - Global
    #if MEDIAINFO_SEEK
    size_t Read_Buffer_Seek (size_t Method, int64u Value, int64u ID) {return ReferenceFiles_Seek(Method, Value, ID);}
    #endif //MEDIAINFO_SEEK

    //Buffer - File header
    bool FileHeader_Begin();

    //PKL
    size_t PKL_Pos;
    void MergeFromPkl (File_DcpPkl::streams &StreamsToMerge);
};

} //NameSpace

#endif


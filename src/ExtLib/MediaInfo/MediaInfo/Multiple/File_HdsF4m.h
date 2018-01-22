/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about HDS (.f4m) files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_HdsF4mH
#define MediaInfo_File_HdsF4mH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/File__HasReferences.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_HdsF4m
//***************************************************************************

class File_HdsF4m : public File__Analyze, File__HasReferences
{
public :
    //Constructor/Destructor
    File_HdsF4m();

private :
    //Streams management
    void Streams_Finish() {ReferenceFiles_Finish();}

    //Buffer - Global
    #if MEDIAINFO_SEEK
    size_t Read_Buffer_Seek (size_t Method, int64u Value, int64u ID) {return ReferenceFiles_Seek(Method, Value, ID);}
    #endif //MEDIAINFO_SEEK

    //Buffer - File header
    bool FileHeader_Begin();
};

} //NameSpace

#endif


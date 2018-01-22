/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about DXW files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_IsmH
#define MediaInfo_File_IsmH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/File__HasReferences.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Ism
//***************************************************************************

class File_Ism : public File__Analyze, File__HasReferences
{
public :
    //Constructor/Destructor
    File_Ism();

private :
    //Streams management
    void Streams_Accept ();
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


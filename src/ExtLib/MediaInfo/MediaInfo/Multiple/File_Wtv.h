/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_WtvH
#define MediaInfo_File_WtvH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include <set>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Wtv
//***************************************************************************

class File_Wtv : public File__Analyze
{
public :
    File_Wtv();

protected :
    //Streams management
    void Streams_Accept();

private :
    //Buffer - File header
    bool FileHeader_Begin();
    void FileHeader_Parse();
};

} //NameSpace

#endif

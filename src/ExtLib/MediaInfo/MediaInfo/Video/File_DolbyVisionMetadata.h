/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about Dolby Vision Metadata files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_DolbyVisionMetadataH
#define MediaInfo_File_DolbyVisionMetadataH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/File__HasReferences.h"
#include "MediaInfo/Multiple/File_DcpPkl.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_DolbyVisionMetadata
//***************************************************************************

class File_DolbyVisionMetadata : public File__Analyze, File__HasReferences
{
public:
    //Constructor/Destructor
    File_DolbyVisionMetadata();

private :
    //Buffer - File header
    bool FileHeader_Begin();
};

} //NameSpace

#endif


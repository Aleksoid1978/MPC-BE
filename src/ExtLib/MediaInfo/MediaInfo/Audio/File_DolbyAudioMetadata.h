/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about Dolby Audio Metadata files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_DolbyAudioMetadataH
#define MediaInfo_File_DolbyAudioMetadataH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/File__HasReferences.h"
#include "MediaInfo/Multiple/File_DcpPkl.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_DolbyAudioMetadata
//***************************************************************************

class File_DolbyAudioMetadata : public File__Analyze, File__HasReferences
{
public:
    //In
    bool IsXML;
    
    //Out
    bool HasSegment9;

    //Constructor/Destructor
    File_DolbyAudioMetadata();

private :
    //Buffer - File header
    bool FileHeader_Begin();

    //Buffer - Global
    void Read_Buffer_Continue();
};

} //NameSpace

#endif


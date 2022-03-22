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
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_DolbyAudioMetadata
//***************************************************************************

class File_DolbyAudioMetadata : public File__Analyze
{
public:
    //In
    bool IsXML;
    
    //Out
    bool HasSegment9;
    vector<int8u> BinauralRenderModes;

    //Constructor/Destructor
    File_DolbyAudioMetadata();

    //Delayed
    void Merge(File__Analyze& In, size_t StreamPos);

private :
    //Buffer - File header
    bool FileHeader_Begin();

    //Buffer - Global
    void Read_Buffer_Continue();

    //Elements
    void Dolby_Atmos_Metadata_Segment();
    void Dolby_Atmos_Supplemental_Metadata_Segment();
};

} //NameSpace

#endif


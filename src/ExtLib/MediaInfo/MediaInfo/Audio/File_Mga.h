/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_MgaH
#define MediaInfo_MgaH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Mga
//***************************************************************************

class File_Mga : public File__Analyze
{
public :
    //In
    int64u Frame_Count_Valid;

    //Constructor/Destructor
    File_Mga();
    ~File_Mga();

private :
    //Streams management
    void Streams_Accept();
    void Streams_Finish();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void AudioMetadataPayload();
    void SerialAudioDefinitionModelMetadataPayload(int64u Length);

    //Helpers
    void Get_BER(int64u& Value, const char* Name);

    //Temp
    File__Analyze* Parser;
};

} //NameSpace

#endif

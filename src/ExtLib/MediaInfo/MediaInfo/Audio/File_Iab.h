/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_IabH
#define MediaInfo_IabH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Iab
//***************************************************************************

class File_Iab : public File__Analyze
{
public :
    //In
    int64u Frame_Count_Valid;
    //Constructor/Destructor
    File_Iab();
    ~File_Iab();

private :
    //Streams management
    void Streams_Fill();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void FrameHeader();
    void BedDefinition();
    void BedRemap();

    //Temp
    int8u Version;
    int8u SampleRate;
    int8u BitDepth;
    int8u FrameRate;

    //Helpers
    void Get_Flex8(int32u& Info, const char* Name);
};

} //NameSpace

#endif

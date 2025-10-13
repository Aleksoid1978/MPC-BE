/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_Vp9H
#define MediaInfo_Vp9H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Vp9
//***************************************************************************

class File_Vp9 : public File__Analyze
{
public :
    //In
    int64u Frame_Count_Valid{};
    bool fromMP4{};

    //Constructor/Destructor
    File_Vp9();
    ~File_Vp9();

private :
    //Streams management
    void Streams_Accept();

    //Buffer - Global
    void Read_Buffer_OutOfBand();
    void Read_Buffer_Continue();
};

} //NameSpace

#endif

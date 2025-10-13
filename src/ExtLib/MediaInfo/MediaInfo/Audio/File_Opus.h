/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about OPUS files
//
// Contributor: Lionel Duchateau, kurtnoise@free.fr
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_OpusH
#define MediaInfo_File_OpusH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Opus
//***************************************************************************

class File_Opus : public File__Analyze
{
public :
    File_Opus();

    //In
    bool FromMP4{};
    bool FromIamf{};

private :
    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void Identification();
    void Stream();

    //Helpers
    void Get_X2(int16u& Info, const char* Name);
    void Get_X4(int32u& Info, const char* Name);
    void Skip_X2(const char* Name);

    //Temp
    bool Identification_Done;
};

} //NameSpace

#endif

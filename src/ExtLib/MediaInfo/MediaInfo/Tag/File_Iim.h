/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about IPTC-NAA Information Interchange Model
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_IimH
#define MediaInfo_File_IimH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Sami
//***************************************************************************

class File_Iim : public File__Analyze
{
private :
    //Buffer - File header
    bool FileHeader_Begin();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void CodedCharacterSet();
    void RecordVersion();
    void ObjectName();
    void DateCreated();
    void TimeCreated();
    void Byline();
    void CopyrightNotice();
    void CaptionAbstract();
    void PhotoMechanicPrefs();

    //Helpers
    void SkipString();

    //Temp
    int8u CodedCharSet = 0;
};

} //NameSpace

#endif

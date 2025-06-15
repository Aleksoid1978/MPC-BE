/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_C2pa4H
#define MediaInfo_File_C2pa4H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_C2pa
//***************************************************************************

class File_C2pa : public File__Analyze
{
protected :
    //Streams management
    void Streams_Accept();

public :
    File_C2pa();
    ~File_C2pa();

private :
    //Buffer - File header
    void FileHeader_Parse();

    //Buffer - Global
    void Read_Buffer_Continue();

    //Buffer - Elements
    void Header_Parse();
    void Data_Parse();

    //Elements
    void ISO__c2pa();
    void ISO__c2pa_c2ma();
    void ISO__c2pa_c2ma_c2as();
    void ISO__c2pa_c2ma_c2as_bfdb() { bfdb(); }
    void ISO__c2pa_c2ma_c2as_bidb() { bidb(); }
    void ISO__c2pa_c2ma_c2as_cbor() { cbor(); }
    void ISO__c2pa_c2ma_c2as_json() { json(); }
    void ISO__c2pa_c2ma_c2cl();
    void ISO__c2pa_c2ma_c2cs();
    void bfdb();
    void bidb();
    void cbor();
    void json();
    void Default();

    //Temp
    string Label;
    string Label_c2ma;
};

} //NameSpace

#endif

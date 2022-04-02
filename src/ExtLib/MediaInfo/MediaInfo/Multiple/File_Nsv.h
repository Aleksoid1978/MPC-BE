/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about Nullsoft Streaming Video (NSV) files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_NsvH
#define MediaInfo_File_NsvH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

class Private;

//***************************************************************************
// Class File_Nsv
//***************************************************************************

class File_Nsv : public File__Analyze
{
public :
    File_Nsv();
    ~File_Nsv();

private :
    //Streams management
    void Streams_Accept();
    void Streams_Finish();

    //Buffer - File header
    void FileHeader_Parse();

    //Buffer - Synchro
    bool Synchronize();
    bool Synched_Test();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    Private* P;
};


//***************************************************************************
// Class File_StarDiva
//***************************************************************************

class File_StarDiva : public File__Analyze
{
private :
    //Buffer - Global
    void Read_Buffer_Continue();
};
} //NameSpace

#endif


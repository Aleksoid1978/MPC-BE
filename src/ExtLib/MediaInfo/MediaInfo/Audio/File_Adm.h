/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about ADM files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_AdmH
#define MediaInfo_File_AdmH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include <map>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

class file_adm_private;

//***************************************************************************
// Class File_Adm
//***************************************************************************

class File_Adm : public File__Analyze
{
public :
    //In
    string MuxingMode;
    float Container_Duration = {};
    int64u TotalSize = 0;
    void chna_Add(int32u Index, const string& TrackUID);
    void* chna_Move();
    void chna_Move(File_Adm*);

    //Out
    bool NeedToJumpToEnd = false;

    //Constructor/Destructor
    File_Adm();
    ~File_Adm();

private :
    //Streams management
    void Streams_Accept();
    void Streams_Fill();
    void Streams_Finish();

    //Buffer - File header
    bool FileHeader_Begin();

    //Buffer - Global
    void Read_Buffer_Init();
    void Read_Buffer_Continue();
 
    //Temp
    file_adm_private* File_Adm_Private;
};

} //NameSpace

#endif


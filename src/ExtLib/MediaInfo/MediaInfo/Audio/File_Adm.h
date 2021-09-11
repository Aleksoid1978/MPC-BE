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
    const char* MuxingMode;

    //Constructor/Destructor
    File_Adm();
    ~File_Adm();

private :
    //Buffer - File header
    bool FileHeader_Begin();

    //Temp
    file_adm_private* File_Adm_Private;
};

} //NameSpace

#endif


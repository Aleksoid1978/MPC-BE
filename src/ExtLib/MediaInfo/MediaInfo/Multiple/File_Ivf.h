/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Contributor: Lionel Duchateau, kurtnoise@free.fr
//
// Information about IVF files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_IvfH
#define MediaInfo_IvfH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include <memory>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Ivf
//***************************************************************************

class File_Ivf : public File__Analyze
{
public :
    //In
    int64u Frame_Count_Valid;
private :
    //Buffer
    bool FileHeader_Begin() override;
    void FileHeader_Parse() override;

    //Buffer - Per element
    void Header_Parse() override;
    void Data_Parse() override;

    //Temp
    std::unique_ptr<File__Analyze> MI;
};

} //NameSpace

#endif


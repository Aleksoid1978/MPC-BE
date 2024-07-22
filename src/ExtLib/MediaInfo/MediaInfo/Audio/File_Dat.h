/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_DatH
#define MediaInfo_File_DatH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Tag/File__Tags.h"
//---------------------------------------------------------------------------

namespace ZenLib
{
class TimeCode;
}

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Dat
//***************************************************************************

class file_dat_priv;

class File_Dat : public File__Analyze
{
public :
    //Constructor/Destructor
    File_Dat();
    ~File_Dat();

private :
    //Streams management
    void Streams_Accept();
    void Streams_Finish();

    //Buffer - Global
    void Read_Buffer_Unsynched();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Helpers
    void dttimepack(TimeCode& TC);
    void Get_BCD(int8u& Value, const char* Name0, const char* Name1);

    //Temp
    file_dat_priv* Priv = nullptr;
};

} //NameSpace

#endif

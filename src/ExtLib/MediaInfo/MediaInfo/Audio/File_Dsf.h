/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_DsfH
#define MediaInfo_DsfH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Tag/File__Tags.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Dsf
//***************************************************************************

class File_Dsf : public File__Analyze, public File__Tags_Helper
{
public :
    //Constructor/Destructor
    File_Dsf();

private :
    //Streams management
    void Streams_Accept();
    void Streams_Finish();

    //Buffer - File header
    bool FileHeader_Begin();

    //Buffer - Synchro
    bool Synched_Test();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void data();
    void DSD_();
    void fmt_();

    //Temp
    int64u Metadata;
};

} //NameSpace

#endif

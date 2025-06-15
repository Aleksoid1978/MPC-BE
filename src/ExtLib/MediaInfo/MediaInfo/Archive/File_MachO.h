/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about Mach-O and Universal Binary files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_MachOH
#define MediaInfo_File_MachOH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_MachO
//***************************************************************************

class File_MachO : public File__Analyze
{
protected :
    //Buffer - File header
    bool FileHeader_Begin();

    //Buffer - Global
    void Read_Buffer_Continue ();

    //Temp
    struct BinaryInfo {
        int64u size;
        int32u cputype;
        int32u align;
    };
    std::map<int64u, BinaryInfo> Universal_Positions; // Key is offset
    std::map<int64u, BinaryInfo>::iterator Universal_Current;
};

} //NameSpace

#endif

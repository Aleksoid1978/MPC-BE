/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Helper class for parser having references to external files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File__HasReferencesH
#define MediaInfo_File__HasReferencesH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Setup.h"
#include "ZenLib/Conf.h"
using namespace ZenLib;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

class File__ReferenceFilesHelper;
class File__Analyze;
class MediaInfo_Config_MediaInfo;

//***************************************************************************
// Class File__HasReferences
//***************************************************************************

#if defined(MEDIAINFO_REFERENCES_YES)
class File__HasReferences
{
public:
    //Constructor/Destructor
    File__HasReferences();
    ~File__HasReferences();

    // Streams management
    void ReferenceFiles_Accept(File__Analyze* MI, MediaInfo_Config_MediaInfo* Config);
    void ReferenceFiles_Finish();

    // Buffer - Global
    size_t ReferenceFiles_Seek(size_t Method, int64u Value, int64u ID);

    //Temp
    File__ReferenceFilesHelper*     ReferenceFiles;
};
#else //defined(MEDIAINFO_REFERENCES_YES)
class File__HasReferences
{
public:
    // Streams management
    void ReferenceFiles_Accept(File__Analyze*, MediaInfo_Config_MediaInfo*) {}
    void ReferenceFiles_Finish() {}

    // Buffer - Global
    size_t ReferenceFiles_Seek(size_t Method, int64u Value, int64u ID) {return (size_t)-1;}
};
#endif //defined(MEDIAINFO_REFERENCES_YES)

} //NameSpace

#endif

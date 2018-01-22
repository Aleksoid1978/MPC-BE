/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
// Pre-compilation
#include "MediaInfo/PreComp.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Setup.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_REFERENCES_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__HasReferences.h"
#include "MediaInfo/Multiple/File__ReferenceFilesHelper.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File__HasReferences::File__HasReferences()
: ReferenceFiles(NULL)
{
}

//---------------------------------------------------------------------------
File__HasReferences::~File__HasReferences()
{
    delete ReferenceFiles;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File__HasReferences::ReferenceFiles_Accept(File__Analyze* MI, MediaInfo_Config_MediaInfo* Config)
{
    ReferenceFiles=new File__ReferenceFilesHelper(MI, Config);
}

//---------------------------------------------------------------------------
void File__HasReferences::ReferenceFiles_Finish()
{
    ReferenceFiles->ParseReferences();
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_REFERENCES_YES) && MEDIAINFO_SEEK
size_t File__HasReferences::ReferenceFiles_Seek(size_t Method, int64u Value, int64u ID)
{
    if (ReferenceFiles==NULL)
        return 0;

    return ReferenceFiles->Seek(Method, Value, ID);
}
#endif //MEDIAINFO_SEEK

} //NameSpace

#endif

/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about VBI data (SMPTE ST436M),Teletext (ETSI EN300 706)
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_VbiH
#define MediaInfo_VbiH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include <vector>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Vbi
//***************************************************************************

class File_Vbi : public File__Analyze
{
public :

    #if defined(MEDIAINFO_TELETEXT_YES)
		File__Analyze*  Teletext_Parser;
    #endif //defined(MEDIAINFO_TELETEXT_YES)
	
    //Constructor/Destructor
	File_Vbi();
	~File_Vbi();

private :
    //Streams management
    void Streams_Finish();

    //Buffer - Global
    void Read_Buffer_Continue();
    void Read_Buffer_Unsynched();
};

} //NameSpace

#endif


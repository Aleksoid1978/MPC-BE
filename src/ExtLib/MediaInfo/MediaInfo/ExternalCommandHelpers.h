/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_External_Program_Helpers
#define MediaInfo_External_Program_Helpers
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/MediaInfo_Internal.h"
#include <ZenLib/ZtringList.h>
#include <ZenLib/Ztring.h>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{
    ZenLib::Ztring External_Command_Exists(const ZenLib::ZtringList& PossibleNames);
    int External_Command_Run(const ZenLib::Ztring& Command, const ZtringList& Arguments, ZenLib::Ztring* StdOut, ZenLib::Ztring* StdErr);

    extern const ZtringList ffmpeg_PossibleNames;
} //NameSpace

#endif

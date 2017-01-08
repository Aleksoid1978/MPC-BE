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
#if defined(MEDIAINFO_VBI_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File_Vbi.h"
#if defined(MEDIAINFO_TELETEXT_YES)
    #include "MediaInfo/Text/File_Teletext.h"
#endif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
namespace MediaInfoLib
{
//---------------------------------------------------------------------------

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Vbi::File_Vbi()
:File__Analyze()
{
    //Configuration
    ParserName="Vbi";
    PTS_DTS_Needed=true;

    #if defined(MEDIAINFO_TELETEXT_YES)
        Teletext_Parser=NULL;
    #endif //defined(MEDIAINFO_TELETEXT_YES)
}

//---------------------------------------------------------------------------
File_Vbi::~File_Vbi()
{
    #if defined(MEDIAINFO_TELETEXT_YES)
        if (Teletext_Parser)
            delete Teletext_Parser;
    #endif //defined(MEDIAINFO_TELETEXT_YES)
}

//---------------------------------------------------------------------------
void File_Vbi::Streams_Finish()
{
    #if defined(MEDIAINFO_TELETEXT_YES)
        if (Teletext_Parser && !Teletext_Parser->Status[IsFinished] && Teletext_Parser->Status[IsAccepted])
        {
            Finish(Teletext_Parser);
            for (size_t StreamKind=Stream_General+1; StreamKind<Stream_Max; StreamKind++)
                for (size_t StreamPos=0; StreamPos<Teletext_Parser->Count_Get((stream_t)StreamKind); StreamPos++)
                {
                    Merge(*Teletext_Parser, (stream_t)StreamKind, StreamPos, StreamPos);
                    Fill((stream_t)StreamKind, StreamPos, "MuxingMode", "VBI", Unlimited, true);
                }
        }
    #endif //defined(MEDIAINFO_TELETEXT_YES)
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Vbi::Read_Buffer_Continue()
{
    if (!Status[IsAccepted])
        Accept(); //No way to detect non-VBI content

    Buffer_Offset=Buffer_Size; //This is per frame

    Frame_Count++;
    Frame_Count_InThisBlock++;
    if (Frame_Count_NotParsedIncluded!=(int64u)-1)
        Frame_Count_NotParsedIncluded++;
}

//---------------------------------------------------------------------------
void File_Vbi::Read_Buffer_Unsynched()
{
    #if defined(MEDIAINFO_TELETEXT_YES)
       if (Teletext_Parser)
            Teletext_Parser->Open_Buffer_Unsynch();
    #endif //defined(MEDIAINFO_TELETEXT_YES)
}

//***************************************************************************
// Helpers
//***************************************************************************

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_VBI_YES


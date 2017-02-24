/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Main part
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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
#ifdef MEDIAINFO_WTV_YES
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File_Wtv.h"
using namespace ZenLib;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Format
//***************************************************************************

//---------------------------------------------------------------------------
File_Wtv::File_Wtv()
:File__Analyze()
{
    //Configuration
    ParserName="Wtv";
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Wtv::Streams_Accept()
{
    Fill(Stream_General, 0, General_Format, "WTV");
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Wtv::FileHeader_Begin()
{
    //Element_Size
    if (Buffer_Size<16)
        return false; //Must wait for more data

    if (CC8(Buffer)!=0xB7D800203749DA11LL || CC8(Buffer+8)!=0xA64E0007E95EAD8DLL)
    {
        Reject();
        return false;
    }

    //All should be OK...
    Accept();
    return true;
}

//---------------------------------------------------------------------------
void File_Wtv::FileHeader_Parse()
{
    Skip_GUID(                                                  "WTV classId");
    Skip_GUID(                                                  "WTV subClassId");
    Skip_L4(                                                    "Unknown");
    Skip_L4(                                                    "Unknown");
    Skip_L4(                                                    "Unknown");
    Skip_L4(                                                    "Unknown");
    Skip_L4(                                                    "Unknown");
    Skip_L4(                                                    "Unknown");
    Skip_L4(                                                    "Unknown");
    Skip_L4(                                                    "Unknown");
    Skip_L4(                                                    "Unknown");
    Skip_L4(                                                    "Unknown");
    Skip_L4(                                                    "Unknown");
    Skip_L4(                                                    "Unknown");
    Skip_L4(                                                    "Unknown");
    Skip_L4(                                                    "Unknown");
    Skip_L4(                                                    "File lenght in meta units");

    ForceFinish();
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_WTV_YES

/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_IabH
#define MediaInfo_IabH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Iab
//***************************************************************************

class File_Iab : public File__Analyze
{
public :
    //In
    int64u Frame_Count_Valid;

    //Constructor/Destructor
    File_Iab();
    ~File_Iab();

    //Extra
    void Streams_Fill_ForAdm();

private :
    //Streams management
    void Streams_Fill();

    //Buffer - Global
    void Read_Buffer_Continue();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void IAFrame();
    void BedDefinition();
    void ObjectDefinition();
    void BedRemap();
    void AudioDataPCM();

    //Temp
    int8u Version;
    int8u SampleRate;
    int8u BitDepth;
    int8u FrameRate;
    struct object
    {
        vector<int32u> ChannelLayout;
    };
    struct frame
    {
        vector<object> Objects;
    };
    frame                       Frame;
    frame                       F;

    //Helpers
    void Get_Plex(int8u Bits, int32u& Info, const char* Name);
    void Get_Plex8(int32u& Info, const char* Name);
    void Skip_Plex(int8u Bits, const char* Name) { int32u Info; Get_Plex(Bits, Info, Name); }
    void Skip_Plex8(const char* Name) { int32u Info; Get_Plex8(Info, Name); }
};

} //NameSpace

#endif

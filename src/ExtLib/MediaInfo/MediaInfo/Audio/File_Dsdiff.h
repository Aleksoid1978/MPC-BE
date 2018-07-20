/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_DsdiffH
#define MediaInfo_DsdiffH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Dsdiff
//***************************************************************************

class File_Dsdiff : public File__Analyze
{
public :
    //Constructor/Destructor
    File_Dsdiff();

private :
    //Streams management
    void Streams_Accept();
    void Streams_Finish();

    //Buffer - File header
    bool FileHeader_Begin();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void DSD_();
    void DSD__COMT();
    void DSD__DIIN();
    void DSD__DIIN_DIAR();
    void DSD__DIIN_DITI();
    void DSD__DIIN_EMID();
    void DSD__DIIN_MARK();
    void DSD__DSD_();
    void DSD__DST_();
    void DSD__DST__DSTC();
    void DSD__DST__DSTF();
    void DSD__DST__FRTE();
    void DSD__DSTI();
    void DSD__FVER();
    void DSD__ID3_();
    void DSD__PROP();
    void DSD__PROP_ABSS();
    void DSD__PROP_CHNL();
    void DSD__PROP_CMPR();
    void DSD__PROP_FS__();
    void DSD__PROP_LSCO();

    //Temp
    bool   pad;
};

} //NameSpace

#endif

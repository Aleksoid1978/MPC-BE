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

//---------------------------------------------------------------------------
enum vbi_type {
    VbiType_Unknown,
    VbiType_Line21,
    VbiType_Vitc,
    VbiType_Teletext,
    VbiType_Max
};

//***************************************************************************
// Class File_Vbi
//***************************************************************************

class File_Vbi : public File__Analyze
{
public :
    int8u               WrappingType;
    int8u               SampleCoding;
    int16u              LineNumber;
    bool                IsLast;
 
    //Constructor/Destructor
    File_Vbi();

private :
    //Streams management
    void Streams_Finish();

    //Buffer - Global
    void Read_Buffer_Continue();
    void Read_Buffer_Unsynched();

    //Elements
    void Parse();
    void Line21();
    void Vitc();
    void Teletext();

    //Stream
    struct stream {
        File__Analyze*  Parser = nullptr;
        vbi_type        Type = VbiType_Unknown;
        float           Private[4] = {};

        ~stream() {
            delete Parser; //Parser=NULL;
        }
    };
    std::map<int16u, stream> Streams;
};

} //NameSpace

#endif


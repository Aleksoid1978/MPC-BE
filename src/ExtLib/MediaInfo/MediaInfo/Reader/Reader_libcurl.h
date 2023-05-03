/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Give information about a lot of media files
// Dispatch the file to be tested by all containers
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef Reader_libcurlH
#define Reader_libcurlH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Reader/Reader__Base.h"
typedef void CURL;

//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
/// @brief Reader_libcurl
//***************************************************************************

class Reader_libcurl : public Reader__Base
{
public :
    //Constructor/Destructor
    Reader_libcurl ();
    virtual ~Reader_libcurl();

    //Format testing
    size_t Format_Test(MediaInfo_Internal* MI, String File_Name);
    size_t Format_Test_PerParser(MediaInfo_Internal* MI, const String &File_Name);
    size_t Format_Test_PerParser_Continue (MediaInfo_Internal* MI);
    size_t Format_Test_PerParser_Seek (MediaInfo_Internal* MI, size_t Method, int64u Value, int64u ID);

    // Open/Close library
    static bool Load(const Ztring File_Name=Ztring());

    struct curl_data;

    curl_data* Curl_Data;
    void Curl_Log(int Result);
    void Curl_Log(int Result, const Ztring &Message);
    size_t SetOptions();
};

} //NameSpace
#endif


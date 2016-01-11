/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef HashWrapperH
#define HashWrapperH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Setup.h"
#if MEDIAINFO_MD5 || MEDIAINFO_SHA1 || MEDIAINFO_SHA2
    #define MEDIAINFO_HASH 1
#else //MEDIAINFO_MD5 || MEDIAINFO_SHA1 || MEDIAINFO_SHA2
    #define MEDIAINFO_HASH 0
#endif //MEDIAINFO_MD5 || MEDIAINFO_SHA1 || MEDIAINFO_SHA2
//---------------------------------------------------------------------------

#if MEDIAINFO_HASH

//---------------------------------------------------------------------------
#include <string>
#include <bitset>
#include "ZenLib/Conf.h"
using namespace std;
using namespace ZenLib;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
class HashWrapper
{
public:
    enum HashFunction
    {
        MD5,
        SHA1,
        SHA224,
        SHA256,
        SHA384,
        SHA512,
        HashFunction_Max,
    };
    typedef bitset<HashFunction_Max> HashFunctions;

    HashWrapper                 (const HashFunctions &Functions);
    ~HashWrapper                ();

    void            Update      (const int8u* Buffer, const size_t Buffer_Size);
    string          Generate    (const HashFunction Function);
    
    static string   Name        (const HashFunction Function);

private:
    void*   m;
};

} //NameSpace

#endif //MEDIAINFO_HASH

#endif //HashWrapperH

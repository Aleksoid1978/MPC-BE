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
using namespace ZenLib;
using std::bitset;
using std::string;
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

    HashWrapper                 (const HashFunctions &Functions)                                                {Init(Functions);}
    HashWrapper                 (const HashFunctions &Functions, const int8u* Buffer, const size_t Buffer_Size) {Init(Functions); Update(Buffer, Buffer_Size);}
    ~HashWrapper                ();

    void            Update      (const int8u* Buffer, const size_t Buffer_Size);
    string          Generate    (const HashFunction Function);
    static string   Generate    (const HashFunction Function, const int8u* Buffer, const size_t Buffer_Size) {return HashWrapper(1<<Function, Buffer, Buffer_Size).Generate(Function);}
    
    static string   Name        (const HashFunction Function);
    static string   Hex2String  (const int8u* Digest, const size_t Digest_Size);

private:
    void    Init(const HashFunctions &Functions);
    void*   m;
};

} //NameSpace

#endif //MEDIAINFO_HASH

#endif //HashWrapperH

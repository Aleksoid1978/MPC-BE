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
#include "MediaInfo/HashWrapper.h" //For getting MEDIAINFO_HASH, not in setup
//---------------------------------------------------------------------------

#if MEDIAINFO_HASH

//---------------------------------------------------------------------------
#include "ZenLib/Ztring.h"
using namespace ZenLib;
#if MEDIAINFO_MD5
    extern "C"
    {
        #include <md5.h>
    }
#endif //MEDIAINFO_MD5
#if MEDIAINFO_SHA1
    extern "C"
    {
        #include <sha1.h>
    }
#endif //MEDIAINFO_SHA1
#if MEDIAINFO_SHA2
    extern "C"
    {
        #include <sha2.h>
    }
#endif //MEDIAINFO_SHA2
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// info
//***************************************************************************

static const char HashWrapper_Hex[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};


//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
void HashWrapper::Init (const HashFunctions &Functions)
{
    //Init to zero
    if (sizeof(m))
    {
        memset(m, 0, sizeof(m));
    }

    #if MEDIAINFO_MD5
        if (Functions[MD5])
        {
            m[MD5]=new struct MD5Context;
            MD5Init((struct MD5Context*)m[MD5]);
        }
    #endif //MEDIAINFO_MD5

    #if MEDIAINFO_SHA1
        if (Functions[SHA1])
        {
            m[SHA1]=new sha1_ctx;
            sha1_begin((sha1_ctx*)m[SHA1]);
        }
    #endif //MEDIAINFO_SHA1

    #if MEDIAINFO_SHA2
        if (Functions[SHA224])
        {
            m[SHA224]=new sha224_ctx;
            sha224_begin((sha224_ctx*)m[SHA224]);
        }
        if (Functions[SHA256])
        {
            m[SHA256]=new sha256_ctx;
            sha256_begin((sha256_ctx*)m[SHA256]);
        }
        if (Functions[SHA384])
        {
            m[SHA384]=new sha384_ctx;
            sha384_begin((sha384_ctx*)m[SHA384]);
        }
        if (Functions[SHA512])
        {
            m[SHA512]=new sha512_ctx;
            sha512_begin((sha512_ctx*)m[SHA512]);
        }
    #endif //MEDIAINFO_SHA2
}

HashWrapper::~HashWrapper ()
{
    #if MEDIAINFO_MD5
        delete (struct MD5Context*)m[MD5];
    #endif //MEDIAINFO_MD5

    #if MEDIAINFO_SHA1
        delete (sha1_ctx*)m[SHA1];
    #endif //MEDIAINFO_SHA1

    #if MEDIAINFO_SHA2
        delete (sha224_ctx*)m[SHA224];
        delete (sha256_ctx*)m[SHA256];
        delete (sha384_ctx*)m[SHA384];
        delete (sha512_ctx*)m[SHA512];
    #endif //MEDIAINFO_SHA2
}

void HashWrapper::Update (const int8u* Buffer, const size_t Buffer_Size)
{
    #if MEDIAINFO_MD5
        if (m[MD5])
            MD5Update((struct MD5Context*)m[MD5], Buffer, (unsigned int)Buffer_Size);
    #endif //MEDIAINFO_MD5

    #if MEDIAINFO_SHA1
        if (m[SHA1])
            sha1_hash(Buffer, (unsigned long)Buffer_Size, (sha1_ctx*)m[SHA1]);
    #endif //MEDIAINFO_SHA1

    #if MEDIAINFO_SHA2
        if (m[SHA224])
            sha224_hash(Buffer, (unsigned long)Buffer_Size, (sha224_ctx*)m[SHA224]);
        if (m[SHA256])
            sha256_hash(Buffer, (unsigned long)Buffer_Size, (sha256_ctx*)m[SHA256]);
        if (m[SHA384])
            sha384_hash(Buffer, (unsigned long)Buffer_Size, (sha384_ctx*)m[SHA384]);
        if (m[SHA512])
            sha512_hash(Buffer, (unsigned long)Buffer_Size, (sha512_ctx*)m[SHA512]);
    #endif //MEDIAINFO_SHA2
}

string HashWrapper::Generate (const HashFunction Function)
{
    #if MEDIAINFO_MD5
        if (Function==MD5 && m[MD5])
        {
            unsigned char Digest[16];
            MD5Final(Digest, (struct MD5Context*)m[MD5]);
            return Hex2String(Digest, 16);
        }
    #endif //MEDIAINFO_MD5

    #if MEDIAINFO_SHA1
        if (Function==SHA1 && m[SHA1])
        {
            unsigned char Digest[20];
            sha1_end(Digest, (sha1_ctx*)m[SHA1]);
            return Hex2String(Digest, 20);
        }
    #endif //MEDIAINFO_SHA1

    #if MEDIAINFO_SHA2
        if (Function==SHA224 && m[SHA224])
        {
            unsigned char Digest[28];
            sha224_end(Digest, (sha224_ctx*)m[SHA224]);
            return Hex2String(Digest, 28);
        }
        if (Function==SHA256 && m[SHA256])
        {
            unsigned char Digest[32];
            sha256_end(Digest, (sha256_ctx*)m[SHA256]);
            return Hex2String(Digest, 32);
        }
        if (Function==SHA384 && m[SHA384])
        {
            unsigned char Digest[48];
            sha384_end(Digest, (sha384_ctx*)m[SHA384]);
            return Hex2String(Digest, 48);
        }
        if (Function==SHA512 && m[SHA512])
        {
            unsigned char Digest[64];
            sha512_end(Digest, (sha512_ctx*)m[SHA512]);
            return Hex2String(Digest, 64);
        }
    #endif //MEDIAINFO_SHA2

    return string();
}

string HashWrapper::Name (const HashFunction Function)
{
    #if MEDIAINFO_MD5
        if (Function==MD5)
            return "MD5";
    #endif //MEDIAINFO_MD5

    #if MEDIAINFO_SHA1
        if (Function==SHA1)
            return "SHA-1";
    #endif //MEDIAINFO_SHA1

    #if MEDIAINFO_SHA2
        if (Function==SHA224)
            return "SHA-224";
        if (Function==SHA256)
            return "SHA-256";
        if (Function==SHA384)
            return "SHA-384";
        if (Function==SHA512)
            return "SHA-512";
    #endif //MEDIAINFO_SHA2

    return string();
}

string HashWrapper::Hex2String(const int8u* Digest, const size_t Digest_Size)
{
    string DigestS;
    DigestS.resize(Digest_Size*2);
    for (size_t i=0, j=0; i<Digest_Size; ++i)
    {
        DigestS[j++]=HashWrapper_Hex[Digest[i] >> 4];
        DigestS[j++]=HashWrapper_Hex[Digest[i] & 0xF];
    }
    return DigestS;
}

} //NameSpace

#endif //MEDIAINFO_HASH

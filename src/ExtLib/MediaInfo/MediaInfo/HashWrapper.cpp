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

const char* HashWrapper_Hex = "0123456789abcdef";

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
HashWrapper::HashWrapper (const HashFunctions &Functions)
{
    m = new void*[HashFunction_Max];

    #if MEDIAINFO_MD5
        if (Functions[MD5])
        {
            ((void**)m)[MD5]=new struct MD5Context;
            MD5Init((struct MD5Context*)((void**)m)[MD5]);
        }
        else
            ((void**)m)[MD5]=NULL;
    #endif //MEDIAINFO_MD5

    #if MEDIAINFO_SHA1
        if (Functions[SHA1])
        {
            ((void**)m)[SHA1]=new sha1_ctx;
            sha1_begin((sha1_ctx*)((void**)m)[SHA1]);
        }
        else
            ((void**)m)[SHA1]=NULL;
    #endif //MEDIAINFO_SHA1

    #if MEDIAINFO_SHA2
        if (Functions[SHA224])
        {
            ((void**)m)[SHA224]=new sha224_ctx;
            sha224_begin((sha224_ctx*)((void**)m)[SHA224]);
        }
        else
            ((void**)m)[SHA224]=NULL;
        if (Functions[SHA256])
        {
            ((void**)m)[SHA256]=new sha256_ctx;
            sha256_begin((sha256_ctx*)((void**)m)[SHA256]);
        }
        else
            ((void**)m)[SHA256]=NULL;
        if (Functions[SHA384])
        {
            ((void**)m)[SHA384]=new sha384_ctx;
            sha384_begin((sha384_ctx*)((void**)m)[SHA384]);
        }
        else
            ((void**)m)[SHA384]=NULL;
        if (Functions[SHA512])
        {
            ((void**)m)[SHA512]=new sha512_ctx;
            sha512_begin((sha512_ctx*)((void**)m)[SHA512]);
        }
        else
            ((void**)m)[SHA512]=NULL;
    #endif //MEDIAINFO_SHA2
}

HashWrapper::~HashWrapper ()
{
    #if MEDIAINFO_MD5
        delete (struct MD5Context*)((void**)m)[MD5];
    #endif //MEDIAINFO_MD5

    #if MEDIAINFO_SHA1
        delete (sha1_ctx*)((void**)m)[SHA1];
    #endif //MEDIAINFO_SHA1

    #if MEDIAINFO_SHA2
        delete (sha224_ctx*)((void**)m)[SHA224];
        delete (sha256_ctx*)((void**)m)[SHA256];
        delete (sha384_ctx*)((void**)m)[SHA384];
        delete (sha512_ctx*)((void**)m)[SHA512];
    #endif //MEDIAINFO_SHA2

    delete[] m;
}

void HashWrapper::Update (const int8u* Buffer, const size_t Buffer_Size)
{
    #if MEDIAINFO_MD5
        if (((void**)m)[MD5])
            MD5Update((struct MD5Context*)((void**)m)[MD5], Buffer, (unsigned int)Buffer_Size);
    #endif //MEDIAINFO_MD5

    #if MEDIAINFO_SHA1
        if (((void**)m)[SHA1])
            sha1_hash(Buffer, (unsigned long)Buffer_Size, (sha1_ctx*)((void**)m)[SHA1]);
    #endif //MEDIAINFO_SHA1

    #if MEDIAINFO_SHA2
        if (((void**)m)[SHA224])
            sha224_hash(Buffer, (unsigned long)Buffer_Size, (sha224_ctx*)((void**)m)[SHA224]);
        if (((void**)m)[SHA256])
            sha256_hash(Buffer, (unsigned long)Buffer_Size, (sha256_ctx*)((void**)m)[SHA256]);
        if (((void**)m)[SHA384])
            sha384_hash(Buffer, (unsigned long)Buffer_Size, (sha384_ctx*)((void**)m)[SHA384]);
        if (((void**)m)[SHA512])
            sha512_hash(Buffer, (unsigned long)Buffer_Size, (sha512_ctx*)((void**)m)[SHA512]);
    #endif //MEDIAINFO_SHA2
}

string HashWrapper::Generate (const HashFunction Function)
{
    #if MEDIAINFO_MD5
        if (Function==MD5 && ((void**)m)[MD5])
        {
            unsigned char Digest[16];
            MD5Final(Digest, (struct MD5Context*)((void**)m)[MD5]);
            Ztring Temp;
            Temp+=Ztring().From_CC2(BigEndian2int16u(Digest+ 0));
            Temp+=Ztring().From_CC2(BigEndian2int16u(Digest+ 2));
            Temp+=Ztring().From_CC2(BigEndian2int16u(Digest+ 4));
            Temp+=Ztring().From_CC2(BigEndian2int16u(Digest+ 6));
            Temp+=Ztring().From_CC2(BigEndian2int16u(Digest+ 8));
            Temp+=Ztring().From_CC2(BigEndian2int16u(Digest+10));
            Temp+=Ztring().From_CC2(BigEndian2int16u(Digest+12));
            Temp+=Ztring().From_CC2(BigEndian2int16u(Digest+14));
            Temp.MakeLowerCase();
            return Temp.To_UTF8();
        }
    #endif //MEDIAINFO_MD5

    #if MEDIAINFO_SHA1
        if (Function==SHA1 && ((void**)m)[SHA1])
        {
            unsigned char Digest[20];
            sha1_end(Digest, (sha1_ctx*)((void**)m)[SHA1]);
            string DigestS;
            DigestS.reserve(40);
            for (size_t i=0; i<20; ++i)
            {
                DigestS.append(1, HashWrapper_Hex[Digest[i] >> 4]);
                DigestS.append(1, HashWrapper_Hex[Digest[i] & 0xF]);
            }
            return DigestS;
        }
    #endif //MEDIAINFO_SHA1

    #if MEDIAINFO_SHA2
        if (Function==SHA224 && ((void**)m)[SHA224])
        {
            unsigned char Digest[28];
            sha224_end(Digest, (sha224_ctx*)((void**)m)[SHA224]);
            string DigestS;
            DigestS.reserve(28*2);
            for (size_t i=0; i<28; ++i)
            {
                DigestS.append(1, HashWrapper_Hex[Digest[i] >> 4]);
                DigestS.append(1, HashWrapper_Hex[Digest[i] & 0xF]);
            }
            return DigestS;
        }
        if (Function==SHA256 && ((void**)m)[SHA256])
        {
            unsigned char Digest[32];
            sha256_end(Digest, (sha256_ctx*)((void**)m)[SHA256]);
            string DigestS;
            DigestS.reserve(32*2);
            for (size_t i=0; i<32; ++i)
            {
                DigestS.append(1, HashWrapper_Hex[Digest[i] >> 4]);
                DigestS.append(1, HashWrapper_Hex[Digest[i] & 0xF]);
            }
            return DigestS;
        }
        if (Function==SHA384 && ((void**)m)[SHA384])
        {
            unsigned char Digest[48];
            sha384_end(Digest, (sha384_ctx*)((void**)m)[SHA384]);
            string DigestS;
            DigestS.reserve(48*2);
            for (size_t i=0; i<48; ++i)
            {
                DigestS.append(1, HashWrapper_Hex[Digest[i] >> 4]);
                DigestS.append(1, HashWrapper_Hex[Digest[i] & 0xF]);
            }
            return DigestS;
        }
        if (Function==SHA512 && ((void**)m)[SHA512])
        {
            unsigned char Digest[64];
            sha512_end(Digest, (sha512_ctx*)((void**)m)[SHA512]);
            string DigestS;
            DigestS.reserve(64*2);
            for (size_t i=0; i<64; ++i)
            {
                DigestS.append(1, HashWrapper_Hex[Digest[i] >> 4]);
                DigestS.append(1, HashWrapper_Hex[Digest[i] & 0xF]);
            }
            return DigestS;
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

} //NameSpace

#endif //MEDIAINFO_HASH

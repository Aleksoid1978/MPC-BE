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

#if !MEDIAINFO_TRACE
//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/MediaInfo_Config.h"
#include "ZenLib/BitStream_LE.h"
#include <cmath>
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
extern MediaInfo_Config Config;
//---------------------------------------------------------------------------

//Integrity test
#define INTEGRITY(TOVALIDATE) \
    if (!(TOVALIDATE)) \
    { \
        Trusted_IsNot(); \
        return; \
    } \

#define INTEGRITY_BOOL(TOVALIDATE) \
    if (!(TOVALIDATE)) \
    { \
        Trusted_IsNot(); \
        Info=false; \
        return; \
    } \

#define INTEGRITY_INT(TOVALIDATE) \
    if (!(TOVALIDATE)) \
    { \
        Trusted_IsNot(); \
        Info=0; \
        return; \
    } \

#define INTEGRITY_SIZE_ATLEAST(_BYTES) \
    if (Element_Offset+_BYTES>Element_Size) \
    { \
        Trusted_IsNot(); \
        return; \
    } \

#define INTEGRITY_SIZE_ATLEAST_STRING(_BYTES) \
    if (Element_Offset+_BYTES>Element_Size) \
    { \
        Trusted_IsNot(); \
        Info.clear(); \
        return; \
    } \

#define INTEGRITY_SIZE_ATLEAST_INT(_BYTES) \
    if (Element_Offset+_BYTES>Element_Size) \
    { \
        Trusted_IsNot(); \
        Info=0; \
        return; \
    } \

#define INTEGRITY_SIZE_ATLEAST_BUFFER() \
    if (BS->Remain()==0) \
    { \
        Trusted_IsNot(); \
        Info=0; \
        return; \
    } \

//***************************************************************************
// Init
//***************************************************************************

//---------------------------------------------------------------------------
void File__Analyze::BS_Begin()
{
    size_t BS_Size;
    if (Element_Offset>=Element_Size)
        BS_Size=0;
    else if (Buffer_Offset+Element_Size<=Buffer_Size)
        BS_Size=(size_t)(Element_Size-Element_Offset);
    else if (Buffer_Offset+Element_Offset<=Buffer_Size)
        BS_Size=Buffer_Size-(size_t)(Buffer_Offset+Element_Offset);
    else
        BS_Size=0;

    BS->Attach(Buffer+Buffer_Offset+(size_t)Element_Offset, BS_Size);
}

//---------------------------------------------------------------------------
void File__Analyze::BS_Begin_LE()
{
    size_t BS_Size;
    if (Buffer_Offset+Element_Size<=Buffer_Size)
        BS_Size=(size_t)(Element_Size-Element_Offset);
    else if (Buffer_Offset+Element_Offset<=Buffer_Size)
        BS_Size=Buffer_Size-(size_t)(Buffer_Offset+Element_Offset);
    else
        BS_Size=0;

    BT->Attach(Buffer+Buffer_Offset+(size_t)Element_Offset, BS_Size);
}

//---------------------------------------------------------------------------
void File__Analyze::BS_End()
{
    BS->Byte_Align();
    Element_Offset+=BS->Offset_Get();
    BS->Attach(NULL, 0);
}

//---------------------------------------------------------------------------
void File__Analyze::BS_End_LE()
{
    BT->Byte_Align();
    Element_Offset+=BT->Offset_Get();
    BT->Attach(NULL, 0);
}

//***************************************************************************
// Big Endian
//***************************************************************************

//---------------------------------------------------------------------------
void File__Analyze::Get_B1_(int8u  &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(1);
    Info=BigEndian2int8u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=1;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_B2_(int16u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(2);
    Info=BigEndian2int16u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=2;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_B3_(int32u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(3);
    Info=BigEndian2int24u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=3;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_B4_(int32u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(4);
    Info=BigEndian2int32u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=4;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_B5_(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(5);
    Info=BigEndian2int40u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=5;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_B6_(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(6);
    Info=BigEndian2int48u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=6;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_B7_(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(7);
    Info=BigEndian2int56u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=7;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_B8_(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(8);
    Info=BigEndian2int64u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=8;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_B16_(int128u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(16);
    //Info=BigEndian2int128u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Info.hi=BigEndian2int64u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Info.lo=BigEndian2int64u(Buffer+Buffer_Offset+(size_t)Element_Offset+8);
    Element_Offset+=16;
}

//---------------------------------------------------------------------------
// Big Endian - float 16 bits
// TODO: remove it when Linux version of ZenLib is updated
float32 BigEndian2float16corrected(const char* Liste)
{
    //sign          1 bit
    //exponent      5 bit
    //significand  10 bit

    //Retrieving data
    int16u Integer=BigEndian2int16u(Liste);

    //Retrieving elements
    bool   Sign    =(Integer&0x8000)?true:false;
    int32u Exponent=(Integer>>10)&0xFF;
    int32u Mantissa= Integer&0x03FF;

    //Some computing
    if (Exponent==0 || Exponent==0xFF)
        return 0; //These are denormalised numbers, NANs, and other horrible things
    Exponent-=0x0F; //Bias
    float64 Answer=(((float64)Mantissa)/8388608+1.0)*std::pow((float64)2, (int)Exponent); //(1+Mantissa) * 2^Exponent
    if (Sign)
        Answer=-Answer;

    return (float32)Answer;
}
inline float32 BigEndian2float16corrected  (const int8u* List) {return BigEndian2float16corrected   ((const char*)List);}

//---------------------------------------------------------------------------
void File__Analyze::Get_BF2_(float32 &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(4);
    Info=BigEndian2float16corrected(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=4;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_BF4_(float32 &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(4);
    Info=BigEndian2float32(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=4;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_BF8_(float64 &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(8);
    Info=BigEndian2float64(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=8;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_BF10_(float80 &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(10);
    Info=BigEndian2float80(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=10;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_BFP4_(int8u  Bits, float32 &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(4);
    BS_Begin();
    int32s Integer=(int32s)BS->Get4(Bits);
    int32u Fraction=BS->Get4(32-Bits);
    BS_End();
    if (Integer>=(1<<Bits)/2)
        Integer-=1<<Bits;
    Info=Integer+((float32)Fraction)/(1<<(32-Bits));
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_B1(int8u  &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(1);
    Info=BigEndian2int8u(Buffer+Buffer_Offset+(size_t)Element_Offset);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_B2(int16u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(2);
    Info=BigEndian2int16u(Buffer+Buffer_Offset+(size_t)Element_Offset);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_B3(int32u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(3);
    Info=BigEndian2int24u(Buffer+Buffer_Offset+(size_t)Element_Offset);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_B4(int32u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(4);
    Info=BigEndian2int32u(Buffer+Buffer_Offset+(size_t)Element_Offset);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_B5(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(5);
    Info=BigEndian2int40u(Buffer+Buffer_Offset+(size_t)Element_Offset);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_B6(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(6);
    Info=BigEndian2int48u(Buffer+Buffer_Offset+(size_t)Element_Offset);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_B7(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(7);
    Info=BigEndian2int56u(Buffer+Buffer_Offset+(size_t)Element_Offset);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_B8(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(8);
    Info=BigEndian2int64u(Buffer+Buffer_Offset+(size_t)Element_Offset);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_B16(int128u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(16);
    Info=BigEndian2int128u(Buffer+Buffer_Offset+(size_t)Element_Offset);
}

//***************************************************************************
// Little Endian
//***************************************************************************

//---------------------------------------------------------------------------
void File__Analyze::Get_L1_(int8u  &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(1);
    Info=LittleEndian2int8u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=1;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_L2_(int16u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(2);
    Info=LittleEndian2int16u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=2;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_L3_(int32u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(3);
    Info=LittleEndian2int24u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=3;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_L4_(int32u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(4);
    Info=LittleEndian2int32u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=4;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_L5_(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(5);
    Info=LittleEndian2int40u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=5;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_L6_(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(6);
    Info=LittleEndian2int48u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=6;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_L7_(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(7);
    Info=LittleEndian2int56u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=7;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_L8_(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(8);
    Info=LittleEndian2int64u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=8;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_L16_(int128u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(16);
    //Info=LittleEndian2int128u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Info.lo=LittleEndian2int64u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Info.hi=LittleEndian2int64u(Buffer+Buffer_Offset+(size_t)Element_Offset+8);
    Element_Offset+=16;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_LF4_(float32 &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(4);
    Info=LittleEndian2float32(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=4;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_LF8_(float64 &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(8);
    Info=LittleEndian2float64(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=8;
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_L1(int8u  &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(1);
    Info=LittleEndian2int8u(Buffer+Buffer_Offset+(size_t)Element_Offset);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_L2(int16u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(2);
    Info=LittleEndian2int16u(Buffer+Buffer_Offset+(size_t)Element_Offset);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_L3(int32u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(3);
    Info=LittleEndian2int24u(Buffer+Buffer_Offset+(size_t)Element_Offset);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_L4(int32u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(4);
    Info=LittleEndian2int32u(Buffer+Buffer_Offset+(size_t)Element_Offset);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_L5(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(5);
    Info=LittleEndian2int40u(Buffer+Buffer_Offset+(size_t)Element_Offset);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_L6(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(6);
    Info=LittleEndian2int48u(Buffer+Buffer_Offset+(size_t)Element_Offset);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_L7(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(7);
    Info=LittleEndian2int56u(Buffer+Buffer_Offset+(size_t)Element_Offset);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_L8(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(8);
    Info=LittleEndian2int64u(Buffer+Buffer_Offset+(size_t)Element_Offset);
}

//***************************************************************************
// Little and Big Endian together
//***************************************************************************

//---------------------------------------------------------------------------
void File__Analyze::Get_D1_(int8u  &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(2);
    Info=LittleEndian2int8u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=2;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_D2_(int16u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(4);
    Info=LittleEndian2int16u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=4;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_D3_(int32u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(6);
    Info=LittleEndian2int24u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=6;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_D4_(int32u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(8);
    Info=LittleEndian2int32u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=8;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_D5_(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(10);
    Info=LittleEndian2int40u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=10;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_D6_(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(12);
    Info=LittleEndian2int48u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=12;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_D7_(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(14);
    Info=LittleEndian2int56u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=14;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_D8_(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(16);
    Info=LittleEndian2int64u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=16;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_D16_(int128u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(32);
    //Info=LittleEndian2int128u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Info.lo=LittleEndian2int64u(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Info.hi=LittleEndian2int64u(Buffer+Buffer_Offset+(size_t)Element_Offset+8);
    Element_Offset+=32;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_DF4_(float32 &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(8);
    Info=LittleEndian2float32(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=8;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_DF8_(float64 &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(16);
    Info=LittleEndian2float64(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=16;
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_D1(int8u  &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(2);
    Info=LittleEndian2int8u(Buffer+Buffer_Offset+(size_t)Element_Offset);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_D2(int16u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(4);
    Info=LittleEndian2int16u(Buffer+Buffer_Offset+(size_t)Element_Offset);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_D3(int32u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(6);
    Info=LittleEndian2int24u(Buffer+Buffer_Offset+(size_t)Element_Offset);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_D4(int32u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(8);
    Info=LittleEndian2int32u(Buffer+Buffer_Offset+(size_t)Element_Offset);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_D5(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(10);
    Info=LittleEndian2int40u(Buffer+Buffer_Offset+(size_t)Element_Offset);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_D6(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(12);
    Info=LittleEndian2int48u(Buffer+Buffer_Offset+(size_t)Element_Offset);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_D7(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(14);
    Info=LittleEndian2int56u(Buffer+Buffer_Offset+(size_t)Element_Offset);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_D8(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(16);
    Info=LittleEndian2int64u(Buffer+Buffer_Offset+(size_t)Element_Offset);
}

//***************************************************************************
// GUID
//***************************************************************************

//---------------------------------------------------------------------------
void File__Analyze::Get_GUID(int128u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(16);
    Info.hi=BigEndian2int64u   (Buffer+Buffer_Offset+(size_t)Element_Offset);
    Info.lo=BigEndian2int64u   (Buffer+Buffer_Offset+(size_t)Element_Offset+8);
    Element_Offset+=16;
}

//***************************************************************************
// EBML
//***************************************************************************

//---------------------------------------------------------------------------
void File__Analyze::Get_EB(int64u &Info)
{
    //Element size
    INTEGRITY_SIZE_ATLEAST_INT(1);
    if (Buffer[Buffer_Offset+Element_Offset]==0xFF)
    {
        Info=File_Size-(File_Offset+Buffer_Offset+Element_Offset);
        Element_Offset++;
        return;
    }
    int8u  Size=0;
    int8u  Size_Mark=0;
    BS_Begin();
    while (Size_Mark==0 && BS->Remain() && Size<=8)
    {
        Size++;
        Peek_S1(Size, Size_Mark);
    }
    BS_End();

    //Integrity
    if (!Size_Mark || Size>8)
    {
        Trusted_IsNot("EBML integer parsing error");
        Info=0;
        return;
    }
    INTEGRITY_SIZE_ATLEAST_INT(Size);

    //Element Name
    switch (Size)
    {
        case 1 : {
                    int8u Element_Name;
                    Peek_B1 (Element_Name);
                    Info=Element_Name&0x7F; //Keep out first bit
                 }
                 break;
        case 2 : {
                    int16u Element_Name;
                    Peek_B2(Element_Name);
                    Info=Element_Name&0x3FFF; //Keep out first bits
                 }
                 break;
        case 3 : {
                    int32u Element_Name;
                    Peek_B3(Element_Name);
                    Info=Element_Name&0x1FFFFF; //Keep out first bits
                 }
                 break;
        case 4 : {
                    int32u Element_Name;
                    Peek_B4(Element_Name);
                    Info=Element_Name&0x0FFFFFFF; //Keep out first bits
                 }
                 break;
        case 5 : {
                    int64u Element_Name;
                    Peek_B5(Element_Name);
                    Info=Element_Name&0x07FFFFFFFFLL; //Keep out first bits
                 }
                 break;
        case 6 : {
                    int64u Element_Name;
                    Peek_B6(Element_Name);
                    Info=Element_Name&0x03FFFFFFFFFFLL; //Keep out first bits
                 }
                 break;
        case 7 : {
                    int64u Element_Name;
                    Peek_B7(Element_Name);
                    Info=Element_Name&0x01FFFFFFFFFFFFLL; //Keep out first bits
                 }
                 break;
        case 8 : {
                    int64u Element_Name;
                    Peek_B8(Element_Name);
                    Info=Element_Name&0x00FFFFFFFFFFFFFFLL; //Keep out first bits
                 }
                 break;
    }

    Element_Offset+=Size;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_ES(int64s &Info)
{
    //Element size
    INTEGRITY_SIZE_ATLEAST_INT(1);
    int8u  Size=0;
    int8u  Size_Mark=0;
    BS_Begin();
    while (Size_Mark==0 && BS->Remain() && Size<=8)
    {
        Size++;
        Peek_S1(Size, Size_Mark);
    }
    BS_End();

    //Integrity
    if (!Size_Mark || Size>8)
    {
        Trusted_IsNot("EBML integer parsing error");
        Info=0;
        return;
    }
    INTEGRITY_SIZE_ATLEAST_INT(Size);

    //Element Name
    switch (Size)
    {
        case 1 : {
                    int8u Element_Name;
                    Peek_B1 (Element_Name);
                    Info=(Element_Name&0x7F)-0x3F; //Keep out first bit and sign
                 }
                 break;
        case 2 : {
                    int16u Element_Name;
                    Peek_B2(Element_Name);
                    Info=(Element_Name&0x3FFF)-0x1FFF; //Keep out first bits and sign
                 }
                 break;
        case 3 : {
                    int32u Element_Name;
                    Peek_B3(Element_Name);
                    Info=(Element_Name&0x1FFFFF)-0x0FFFFF; //Keep out first bits and sign
                 }
                 break;
        case 4 : {
                    int32u Element_Name;
                    Peek_B4(Element_Name);
                    Info=(Element_Name&0x0FFFFFFF)-0x07FFFFFF; //Keep out first bits and sign
                 }
                 break;
        case 5 : {
                    int64u Element_Name;
                    Peek_B5(Element_Name);
                    Info=(Element_Name&0x07FFFFFFFFLL)-0x03FFFFFFFFLL; //Keep out first bits and sign
                 }
                 break;
        case 6 : {
                    int64u Element_Name;
                    Peek_B6(Element_Name);
                    Info=(Element_Name&0x03FFFFFFFFFFLL)-0x01FFFFFFFFFFLL; //Keep out first bits and sign
                 }
                 break;
        case 7 : {
                    int64u Element_Name;
                    Peek_B7(Element_Name);
                    Info=(Element_Name&0x01FFFFFFFFFFFFLL)-0x00FFFFFFFFFFFFLL; //Keep out first bits and sign
                 }
                 break;
        case 8 : {
                    int64u Element_Name;
                    Peek_B8(Element_Name);
                    Info=(Element_Name&0x00FFFFFFFFFFFFFFLL)-0x007FFFFFFFFFFFFFLL; //Keep out first bits and sign
                 }
                 break;
    }

    Element_Offset+=Size;
}

//***************************************************************************
// Variable Length Value
//***************************************************************************

//---------------------------------------------------------------------------
void File__Analyze::Get_VS(int64u &Info)
{
    //Element size
    Info=0;
    int8u  Size=0;
    bool more_data;
    BS_Begin();
    do
    {
        Size++;
        INTEGRITY_INT(8<=BS->Remain())
        more_data=BS->GetB();
        Info=128*Info+BS->Get1(7);
    }
    while (more_data && Size<=8 && BS->Remain());
    BS_End();

    //Integrity
    if (more_data || Size>8)
    {
        Trusted_IsNot("Variable Length Value parsing error");
        Info=0;
        return;
    }
}

//---------------------------------------------------------------------------
void File__Analyze::Skip_VS()
{
    //Element size
    int64u Info=0;
    int8u  Size=0;
    bool more_data;
    BS_Begin();
    do
    {
        Size++;
        INTEGRITY_INT(8<=BS->Remain())
        more_data=BS->GetB();
        Info=128*Info+BS->Get1(7);
    }
    while (more_data && Size<=8 && BS->Remain());
    BS_End();

    //Integrity
    if (more_data || Size>8)
    {
        Trusted_IsNot("Variable Size Value parsing error");
        Info=0;
        return;
    }
}

//***************************************************************************
// Exp-Golomb
//***************************************************************************

//---------------------------------------------------------------------------
void File__Analyze::Get_SE(int32s &Info)
{
    INTEGRITY_SIZE_ATLEAST_BUFFER();
    int8u LeadingZeroBits=0;
    while(BS->Remain()>0 && !BS->GetB())
        LeadingZeroBits++;
    INTEGRITY_INT(LeadingZeroBits<=32)
    double InfoD=pow((float)2, (float)LeadingZeroBits)-1+BS->Get4(LeadingZeroBits);
    INTEGRITY_INT(InfoD<int32u(-1))
    Info=(int32s)(pow((double)-1, InfoD+1)*(int32u)ceil(InfoD/2));
}

//---------------------------------------------------------------------------
void File__Analyze::Get_UE(int32u &Info)
{
    INTEGRITY_SIZE_ATLEAST_BUFFER();
    int8u LeadingZeroBits=0;
    while(BS->Remain()>0 && !BS->GetB())
        LeadingZeroBits++;
    INTEGRITY_INT(LeadingZeroBits<=32)
    double InfoD=pow((float)2, (float)LeadingZeroBits);
    Info=(int32u)InfoD-1+BS->Get4(LeadingZeroBits);
}

//---------------------------------------------------------------------------
void File__Analyze::Skip_UE()
{
    INTEGRITY(BS->Remain())
    int8u LeadingZeroBits=0;
    while(BS->Remain()>0 && !BS->GetB())
        LeadingZeroBits++;
    BS->Skip(LeadingZeroBits);
}

//***************************************************************************
// Inverted Exp-Golomb
//***************************************************************************

//---------------------------------------------------------------------------
void File__Analyze::Get_SI(int32s &Info)
{
    INTEGRITY_SIZE_ATLEAST_BUFFER();

    Info=1;
    while(BS->Remain()>0 && BS->GetB()==0)
    {
        Info<<=1;
        if (BS->Remain()==0)
        {
            Trusted_IsNot("(Problem)");
            Info=0;
            return;
        }
        if(BS->GetB()==1)
            Info++;
    }
    Info--;

    if (Info!=0 && BS->Remain()>0 && BS->GetB()==1)
        Info=-Info;
}

//---------------------------------------------------------------------------
void File__Analyze::Skip_SI()
{
    int32s Info;
    Get_SI(Info);
}

//---------------------------------------------------------------------------
void File__Analyze::Get_UI(int32u &Info)
{
    INTEGRITY_SIZE_ATLEAST_BUFFER();
    Info=1;
    while(BS->Remain()>0 && BS->GetB()==0)
    {
        Info<<=1;
        if (BS->Remain()==0)
        {
            Trusted_IsNot("(Problem)");
            Info=0;
            return;
        }
        if(BS->GetB()==1)
            Info++;
    }
    Info--;
}

//---------------------------------------------------------------------------
void File__Analyze::Skip_UI()
{
    int32u Info;
    Get_UI(Info);
}

//***************************************************************************
// Variable Length Code
//***************************************************************************

//---------------------------------------------------------------------------
void File__Analyze::Get_VL_(const vlc Vlc[], size_t &Info)
{
    Info=0;
    int32u Value=0;

    for(;;)
    {
        switch (Vlc[Info].bit_increment)
        {
            case 255 :
                        Trusted_IsNot();
                        return;
            default  : ;
                        Value<<=Vlc[Info].bit_increment;
                        Value|=BS->Get1(Vlc[Info].bit_increment);
                        break;
            case   1 :
                        Value<<=1;
                        if (BS->GetB())
                            Value++;
            case   0 :  ;
        }

        if (Value==Vlc[Info].value)
            return;
        Info++;
    }
}

//---------------------------------------------------------------------------
void File__Analyze::Skip_VL_(const vlc Vlc[])
{
    size_t Info=0;
    Get_VL_(Vlc, Info);
}

//---------------------------------------------------------------------------
void File__Analyze::Get_VL_Prepare(vlc_fast &Vlc)
{
    Vlc.Array=new int8u[((size_t)1)<<Vlc.Size];
    Vlc.BitsToSkip=new int8u[((size_t)1)<<Vlc.Size];
    memset(Vlc.Array, 0xFF, ((size_t)1)<<Vlc.Size);
    int8u  Increment=0;
    int8u  Pos=0;
    for(; ; Pos++)
    {
        if (Vlc.Vlc[Pos].bit_increment==255)
            break;
        Increment+=Vlc.Vlc[Pos].bit_increment;
        size_t Value = ((size_t)Vlc.Vlc[Pos].value) << (Vlc.Size - Increment);
        size_t ToFill_Size=((size_t)1)<<(Vlc.Size-Increment);
        for (size_t ToFill_Pos=0; ToFill_Pos<ToFill_Size; ToFill_Pos++)
        {
            Vlc.Array[Value+ToFill_Pos]=Pos;
            Vlc.BitsToSkip[Value+ToFill_Pos]=Increment;
        }
    }
    for (size_t Pos2=0; Pos2<(((size_t)1)<<Vlc.Size); Pos2++)
        if (Vlc.Array[Pos2]==(int8u)-1)
        {
            Vlc.Array[Pos2]=Pos;
            Vlc.BitsToSkip[Pos2]=(int8u)-1;
        }
}

//---------------------------------------------------------------------------
void File__Analyze::Get_VL_(const vlc_fast &Vlc, size_t &Info)
{
    if (BS->Remain()<Vlc.Size)
    {
        Get_VL(Vlc.Vlc, Info, Name);
        return;
    }

    int32u Value=BS->Peek4(Vlc.Size);
    Info=Vlc.Array[Value];

    if (Vlc.BitsToSkip[Value]==(int8u)-1)
    {
        Trusted_IsNot();
        return;
    }

    BS->Skip(Vlc.BitsToSkip[Value]);
}

//---------------------------------------------------------------------------
void File__Analyze::Skip_VL_(const vlc_fast &Vlc)
{
    if (BS->Remain()<Vlc.Size)
    {
        Skip_VL_(Vlc.Vlc);
        return;
    }

    int32u Value=BS->Peek4(Vlc.Size);

    if (Vlc.BitsToSkip[Value]==(int8u)-1)
    {
        Trusted_IsNot();
        return;
    }

    BS->Skip(Vlc.BitsToSkip[Value]);
}

//***************************************************************************
// Characters
//***************************************************************************

//---------------------------------------------------------------------------
void File__Analyze::Get_C1(int8u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(1);
    Info=CC1(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=1;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_C2(int16u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(2);
    Info=CC2(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=2;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_C3(int32u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(3);
    Info=CC3(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=3;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_C4(int32u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(4);
    Info=CC4(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=4;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_C5(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(5);
    Info=CC5(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=5;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_C6(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(6);
    Info=CC6(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=6;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_C7(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(7);
    Info=CC7(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=7;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_C8(int64u &Info)
{
    INTEGRITY_SIZE_ATLEAST_INT(8);
    Info=CC8(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Element_Offset+=8;
}

//***************************************************************************
// Text
//***************************************************************************

//---------------------------------------------------------------------------
void File__Analyze::Get_Local(int64u Bytes, Ztring &Info)
{
    INTEGRITY_SIZE_ATLEAST_STRING(Bytes);
    #ifdef WINDOWS
    Info.From_Local((const char*)(Buffer+Buffer_Offset+(size_t)Element_Offset), (size_t)Bytes);
    #else //WINDOWS
    Info.From_ISO_8859_1((const char*)(Buffer+Buffer_Offset+(size_t)Element_Offset), (size_t)Bytes); //Trying with the most commonly used charset before UTF8
    #endif //WINDOWS
    Element_Offset+=Bytes;
}

//---------------------------------------------------------------------------
extern const wchar_t ISO_6937_2_Tables[];
void File__Analyze::Get_ISO_6937_2(int64u Bytes, Ztring &Info)
{
    INTEGRITY_SIZE_ATLEAST_STRING(Bytes);
    Info.clear();
    size_t End = Buffer_Offset + (size_t)Element_Offset + (size_t)Bytes;
    for (size_t Pos=Buffer_Offset+(size_t)Element_Offset; Pos<End; ++Pos)
    {
        wchar_t EscapeChar=L'\x0000';
        wchar_t NewChar=L'\x0000';
        switch (Buffer[Pos])
        {
            case 0xA9 :    NewChar=L'\x2018'; break;
            case 0xAA :    NewChar=L'\x201C'; break;
            case 0xAC :    NewChar=L'\x2190'; break;
            case 0xAD :    NewChar=L'\x2191'; break;
            case 0xAE :    NewChar=L'\x2192'; break;
            case 0xAF :    NewChar=L'\x2193'; break;
            case 0xB4 :    NewChar=L'\x00D7'; break;
            case 0xB8 :    NewChar=L'\x00F7'; break;
            case 0xB9 :    NewChar=L'\x2019'; break;
            case 0xBA :    NewChar=L'\x201D'; break;
            case 0xC1 : EscapeChar=L'\x0300'; break;
            case 0xC2 : EscapeChar=L'\x0301'; break;
            case 0xC3 : EscapeChar=L'\x0302'; break;
            case 0xC4 : EscapeChar=L'\x0303'; break;
            case 0xC5 : EscapeChar=L'\x0304'; break;
            case 0xC6 : EscapeChar=L'\x0306'; break;
            case 0xC7 : EscapeChar=L'\x0307'; break;
            case 0xC8 : EscapeChar=L'\x0308'; break;
            case 0xCA : EscapeChar=L'\x030A'; break;
            case 0xCB : EscapeChar=L'\x0327'; break;
            case 0xCD : EscapeChar=L'\x030B'; break;
            case 0xCE : EscapeChar=L'\x0328'; break;
            case 0xCF : EscapeChar=L'\x030C'; break;
            case 0xD0 :    NewChar=L'\x2015'; break;
            case 0xD1 :    NewChar=L'\x00B9'; break;
            case 0xD2 :    NewChar=L'\x00AE'; break;
            case 0xD3 :    NewChar=L'\x00A9'; break;
            case 0xD4 :    NewChar=L'\x2122'; break;
            case 0xD5 :    NewChar=L'\x266A'; break;
            case 0xD6 :    NewChar=L'\x00AC'; break;
            case 0xD7 :    NewChar=L'\x00A6'; break;
            case 0xDC :    NewChar=L'\x215B'; break;
            case 0xDD :    NewChar=L'\x215C'; break;
            case 0xDE :    NewChar=L'\x215D'; break;
            case 0xDF :    NewChar=L'\x215E'; break;
            case 0xE0 :    NewChar=L'\x2126'; break;
            case 0xE1 :    NewChar=L'\x00C6'; break;
            case 0xE2 :    NewChar=L'\x0110'; break;
            case 0xE3 :    NewChar=L'\x00AA'; break;
            case 0xE4 :    NewChar=L'\x0126'; break;
            case 0xE6 :    NewChar=L'\x0132'; break;
            case 0xE7 :    NewChar=L'\x013F'; break;
            case 0xE8 :    NewChar=L'\x0141'; break;
            case 0xE9 :    NewChar=L'\x00D8'; break;
            case 0xEA :    NewChar=L'\x0152'; break;
            case 0xEB :    NewChar=L'\x00BA'; break;
            case 0xEC :    NewChar=L'\x00DE'; break;
            case 0xED :    NewChar=L'\x0166'; break;
            case 0xEE :    NewChar=L'\x014A'; break;
            case 0xEF :    NewChar=L'\x0149'; break;
            case 0xF0 :    NewChar=L'\x0138'; break;
            case 0xF1 :    NewChar=L'\x00E6'; break;
            case 0xF2 :    NewChar=L'\x0111'; break;
            case 0xF3 :    NewChar=L'\x00F0'; break;
            case 0xF4 :    NewChar=L'\x0127'; break;
            case 0xF5 :    NewChar=L'\x0131'; break;
            case 0xF6 :    NewChar=L'\x0133'; break;
            case 0xF7 :    NewChar=L'\x0140'; break;
            case 0xF8 :    NewChar=L'\x0142'; break;
            case 0xF9 :    NewChar=L'\x00F8'; break;
            case 0xFA :    NewChar=L'\x0153'; break;
            case 0xFB :    NewChar=L'\x0153'; break;
            case 0xFC :    NewChar=L'\x00FE'; break;
            case 0xFD :    NewChar=L'\x00FE'; break;
            case 0xFE :    NewChar=L'\x014B'; break;
            case 0xFF :    NewChar=L'\x00AD'; break;
            case 0xC0 :
            case 0xC9 :
            case 0xCC :
            case 0xD8 :
            case 0xD9 :
            case 0xDA :
            case 0xDB :
            case 0xE5 :
                                              break;
            default  : NewChar=(wchar_t)(Buffer[Pos]);
        }

        if (EscapeChar)
        {
            if (Pos+1<End)
            {
                if (Buffer[Pos]>=0xC0 && Buffer[Pos]<=0xCF && Buffer[Pos+1]>=0x40 && Buffer[Pos+1]<=0x7F)
                    Info+=Ztring().From_Unicode(ISO_6937_2_Tables[((Buffer[Pos]-0xC0))*0x40+(Buffer[Pos+1]-0x40)]);
                else
                {
                Info+=(Char)(Buffer[Pos+1]);
                Info+=Ztring().From_Unicode(&EscapeChar, 1); //(EscapeChar) after new ZenLib release
                }
                EscapeChar=L'\x0000';
                Pos++;
            }
        }
        else if (NewChar)
            Info+=Ztring().From_Unicode(&NewChar, 1); //(NewChar) after new ZenLib release
    }
    Element_Offset+=Bytes;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_ISO_8859_1(int64u Bytes, Ztring &Info)
{
    INTEGRITY_SIZE_ATLEAST_STRING(Bytes);
    Info.From_ISO_8859_1((const char*)(Buffer+Buffer_Offset+(size_t)Element_Offset), (size_t)Bytes);
    Element_Offset+=Bytes;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_ISO_8859_2(int64u Bytes, Ztring &Info)
{
    INTEGRITY_SIZE_ATLEAST_STRING(Bytes);
    Info.From_ISO_8859_2((const char*)(Buffer+Buffer_Offset+(size_t)Element_Offset), (size_t)Bytes);
    Element_Offset+=Bytes;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_ISO_8859_5(int64u Bytes, Ztring &Info)
{
    INTEGRITY_SIZE_ATLEAST_STRING(Bytes);
    Info.clear();
    size_t End = Buffer_Offset + (size_t)Element_Offset + (size_t)Bytes;
    for (size_t Pos=Buffer_Offset+(size_t)Element_Offset; Pos<End; ++Pos)
    {
        switch (Buffer[Pos])
        {
            case 0xAD : Info+=Ztring().From_Unicode(L"\xAD"); break; //L'\xAD' after new ZenLib release
            case 0xF0 : Info+=Ztring().From_Unicode(L"\x2116"); break; //L'\x2116' after new ZenLib release
            case 0xFD : Info+=Ztring().From_Unicode(L"\xA7"); break; //L'\xA7' after new ZenLib release
            default   :
                        {
                        wchar_t NewChar=(Buffer[Pos]<=0xA0?0x0000:0x0360)+Buffer[Pos];
                        Info+=Ztring().From_Unicode(&NewChar, 1); //(NewChar) after new ZenLib release
                        }
        }
    }
    Element_Offset+=Bytes;
}

//---------------------------------------------------------------------------
extern const int16u Ztring_MacRoman[128];
void File__Analyze::Get_MacRoman(int64u Bytes, Ztring &Info)
{
    INTEGRITY_SIZE_ATLEAST_STRING(Bytes);

    // Use From_MacRoman() after new ZenLib release
    const int8u* Input =Buffer+Buffer_Offset+(size_t)Element_Offset;

    wchar_t* Temp=new wchar_t[Bytes];

    for (size_t Pos=0; Pos<Bytes; Pos++)
    {
        if (Input[Pos]>=0x80)
            Temp[Pos]=(wchar_t)Ztring_MacRoman[Input[Pos]-0x80];
        else
            Temp[Pos]=(wchar_t)Input[Pos];
    }

    Info.From_Unicode(Temp, Bytes);
    delete[] Temp;

    Element_Offset+=Bytes;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_String(int64u Bytes, std::string &Info)
{
    INTEGRITY_SIZE_ATLEAST_STRING(Bytes);
    Info.assign((const char*)(Buffer+Buffer_Offset+(size_t)Element_Offset), (size_t)Bytes);
    Element_Offset+=Bytes;
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_Local(int64u Bytes, Ztring &Info)
{
    INTEGRITY_SIZE_ATLEAST_STRING(Bytes);
    #ifdef WINDOWS
    Info.From_Local((const char*)(Buffer+Buffer_Offset+(size_t)Element_Offset), (size_t)Bytes);
    #else //WINDOWS
    Info.From_ISO_8859_1((const char*)(Buffer+Buffer_Offset+(size_t)Element_Offset), (size_t)Bytes); //Trying with the most commonly used charset before UTF8
    #endif //WINDOWS
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_String(int64u Bytes, std::string &Info)
{
    INTEGRITY_SIZE_ATLEAST_STRING(Bytes);
    Info.assign((const char*)(Buffer+Buffer_Offset+(size_t)Element_Offset), (size_t)Bytes);
}

//---------------------------------------------------------------------------
void File__Analyze::Get_UTF8(int64u Bytes, Ztring &Info)
{
    INTEGRITY_SIZE_ATLEAST_STRING(Bytes);
    Info.From_UTF8((const char*)(Buffer+Buffer_Offset+(size_t)Element_Offset), (size_t)Bytes);
    Element_Offset+=Bytes;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_UTF16(int64u Bytes, Ztring &Info)
{
    INTEGRITY_SIZE_ATLEAST_STRING(Bytes);
    Info.From_UTF16((const char*)(Buffer+Buffer_Offset+(size_t)Element_Offset), (size_t)Bytes);
    Element_Offset+=Bytes;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_UTF16B(int64u Bytes, Ztring &Info)
{
    INTEGRITY_SIZE_ATLEAST_STRING(Bytes);
    Info.From_UTF16BE((const char*)(Buffer+Buffer_Offset+(size_t)Element_Offset), (size_t)Bytes);
    Element_Offset+=Bytes;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_UTF16L(int64u Bytes, Ztring &Info)
{
    INTEGRITY_SIZE_ATLEAST_STRING(Bytes);
    Info.From_UTF16LE((const char*)(Buffer+Buffer_Offset+(size_t)Element_Offset), (size_t)Bytes);
    Element_Offset+=Bytes;
}

//***************************************************************************
// Text
//***************************************************************************

//---------------------------------------------------------------------------
void File__Analyze::Skip_PA()
{
    INTEGRITY_SIZE_ATLEAST(1);
    int8u Size=Buffer[Buffer_Offset+Element_Offset];
    int8u Pad=(Size%2)?0:1;
    INTEGRITY_SIZE_ATLEAST(1+Size+Pad);
    Element_Offset+=(size_t)(1+Size+Pad);
}

//***************************************************************************
// Unknown
//***************************************************************************

//---------------------------------------------------------------------------
void File__Analyze::Skip_XX(int64u Bytes)
{
    if (Element_Offset+Bytes!=Element_TotalSize_Get()) //Exception for seek to end of the element
    {
        INTEGRITY_SIZE_ATLEAST(Bytes);
    }
    Element_Offset+=Bytes;
}

//***************************************************************************
// Flags
//***************************************************************************

//---------------------------------------------------------------------------
void File__Analyze::Get_Flags_ (int64u Flags, size_t Order, bool &Info)
{
    if (Flags&((int64u)1<<Order))
        Info=true;
    else
        Info=false;
}

//---------------------------------------------------------------------------
void File__Analyze::Get_FlagsM_ (int64u ValueToPut, int8u &Info)
{
    Info=(int8u)ValueToPut;
}

//***************************************************************************
// BitStream
//***************************************************************************

//---------------------------------------------------------------------------
void File__Analyze::Get_BS_(int8u Bits, int32u &Info)
{
    INTEGRITY_INT(Bits<=BS->Remain())
    Info=BS->Get4(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Get_SB_(             bool &Info)
{
    INTEGRITY_INT(1<=BS->Remain())
    Info=BS->GetB();
}

//---------------------------------------------------------------------------
void File__Analyze::Get_S1_(int8u  Bits, int8u &Info)
{
    INTEGRITY_INT(Bits<=BS->Remain())
    Info=BS->Get1(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Get_S2_(int8u  Bits, int16u &Info)
{
    INTEGRITY_INT(Bits<=BS->Remain())
    Info=BS->Get2(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Get_S3_(int8u  Bits, int32u &Info)
{
    INTEGRITY_INT(Bits<=BS->Remain())
    Info=BS->Get4(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Get_S4_(int8u  Bits, int32u &Info)
{
    INTEGRITY_INT(Bits<=BS->Remain())
    Info=BS->Get4(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Get_S5_(int8u  Bits, int64u &Info)
{
    INTEGRITY_INT(Bits<=BS->Remain())
    Info=BS->Get8(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Get_S6_(int8u  Bits, int64u &Info)
{
    INTEGRITY_INT(Bits<=BS->Remain())
    Info=BS->Get8(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Get_S7_(int8u  Bits, int64u &Info)
{
    INTEGRITY_INT(Bits<=BS->Remain())
    Info=BS->Get8(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Get_S8_(int8u  Bits, int64u &Info)
{
    INTEGRITY_INT(Bits<=BS->Remain())
    Info=BS->Get8(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_BS(int8u  Bits, int32u &Info)
{
    INTEGRITY_INT(Bits<=BS->Remain())
    Info=BS->Peek4(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_SB(              bool &Info)
{
    INTEGRITY_INT(1<=BS->Remain())
    Info=BS->PeekB();
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_S1(int8u  Bits, int8u &Info)
{
    INTEGRITY_INT(Bits<=BS->Remain())
    Info=BS->Peek1(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_S2(int8u  Bits, int16u &Info)
{
    INTEGRITY_INT(Bits<=BS->Remain())
    Info=BS->Peek2(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_S3(int8u  Bits, int32u &Info)
{
    INTEGRITY_INT(Bits<=BS->Remain())
    Info=BS->Peek4(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_S4(int8u  Bits, int32u &Info)
{
    INTEGRITY_INT(Bits<=BS->Remain())
    Info=BS->Peek4(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_S5(int8u  Bits, int64u &Info)
{
    INTEGRITY_INT(Bits<=BS->Remain())
    Info=BS->Peek8(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_S6(int8u  Bits, int64u &Info)
{
    INTEGRITY_INT(Bits<=BS->Remain())
    Info=BS->Peek8(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_S7(int8u  Bits, int64u &Info)
{
    INTEGRITY_INT(Bits<=BS->Remain())
    Info=BS->Peek8(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_S8(int8u  Bits, int64u &Info)
{
    INTEGRITY_INT(Bits<=BS->Remain())
    Info=BS->Peek8(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Mark_0()
{
    INTEGRITY(1<=BS->Remain())
    if (BS->GetB())
        Element_DoNotTrust("Mark bit is wrong");
}

//---------------------------------------------------------------------------
void File__Analyze::Mark_1()
{
    INTEGRITY(1<=BS->Remain())
    if (!BS->GetB())
        Element_DoNotTrust("Mark bit is wrong");
}

//***************************************************************************
// BitStream (Little Endian)
//***************************************************************************

//---------------------------------------------------------------------------
void File__Analyze::Get_BT_(int8u Bits, int32u &Info)
{
    INTEGRITY_INT(Bits<=BT->Remain())
    Info=BT->Get(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Get_TB_(             bool &Info)
{
    INTEGRITY_INT(1<=BT->Remain())
    Info=BT->GetB();
}

//---------------------------------------------------------------------------
void File__Analyze::Get_T1_(int8u  Bits, int8u &Info)
{
    INTEGRITY_INT(Bits<=BT->Remain())
    Info=BT->Get1(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Get_T2_(int8u  Bits, int16u &Info)
{
    INTEGRITY_INT(Bits<=BT->Remain())
    Info=BT->Get2(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Get_T4_(int8u  Bits, int32u &Info)
{
    INTEGRITY_INT(Bits<=BT->Remain())
    Info=BT->Get4(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Get_T8_(int8u  Bits, int64u &Info)
{
    INTEGRITY_INT(Bits<=BT->Remain())
    Info=BT->Get8(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_BT(int8u  Bits, int32u &Info)
{
    INTEGRITY_INT(Bits<=BT->Remain())
    Info=BT->Peek(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_TB(              bool &Info)
{
    INTEGRITY_INT(1<=BT->Remain())
    Info=BT->PeekB();
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_T1(int8u  Bits, int8u &Info)
{
    INTEGRITY_INT(Bits<=BT->Remain())
    Info=BT->Peek1(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_T2(int8u  Bits, int16u &Info)
{
    INTEGRITY_INT(Bits<=BT->Remain())
    Info=BT->Peek2(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_T4(int8u  Bits, int32u &Info)
{
    INTEGRITY_INT(Bits<=BT->Remain())
    Info=BT->Peek4(Bits);
}

//---------------------------------------------------------------------------
void File__Analyze::Peek_T8(int8u  Bits, int64u &Info)
{
    INTEGRITY_INT(Bits<=BT->Remain())
    Info=BT->Peek8(Bits);
}

} //NameSpace
#endif //MEDIAINFO_TRACE

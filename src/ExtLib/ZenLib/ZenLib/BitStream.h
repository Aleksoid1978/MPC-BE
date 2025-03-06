/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a zlib-style license that can
 *  be found in the License.txt file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Read a stream bit per bit
// Can read up to 32 bits at once
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef ZenBitStreamH
#define ZenBitStreamH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "ZenLib/Conf.h"
//---------------------------------------------------------------------------

namespace ZenLib
{

#ifndef MIN
    #define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif

class BitStream
{
public:
    BitStream ()                                                                { Buffer = NULL;
                                                                                  Buffer_Size = 0;
                                                                                  Buffer_Size_Init = 0;
                                                                                  Buffer_Size_BeforeLastCall = 0;
                                                                                  LastByte = 0;
                                                                                  LastByte_Size = 0;
                                                                                  BufferUnderRun = true;
                                                                                  BookMark = false;
                                                                                  Buffer_BookMark = 0;
                                                                                  Buffer_Size_BookMark = 0;
                                                                                  LastByte_BookMark = 0;
                                                                                  LastByte_Size_BookMark = 0;
                                                                                  BufferUnderRun_BookMark = false; }
    BitStream (const int8u* Buffer_, size_t Size_)                              { Buffer = Buffer_;
                                                                                  Buffer_Size = Size_ * 8; //Size is in bits
                                                                                  Buffer_Size_Init = Size_ * 8; //Size is in bits
                                                                                  Buffer_Size_BeforeLastCall = Size_ * 8; //Size is in bits
                                                                                  LastByte = 0;
                                                                                  LastByte_Size = 0;
                                                                                  BufferUnderRun = (Buffer_Size ? false : true);
                                                                                  BookMark = false;
                                                                                  Buffer_BookMark = 0;
                                                                                  Buffer_Size_BookMark = 0;
                                                                                  LastByte_BookMark = 0;
                                                                                  LastByte_Size_BookMark = 0;
                                                                                  BufferUnderRun_BookMark = false; }
    virtual ~BitStream ()                                                       {};

    virtual void Attach(const int8u* Buffer_, size_t Size_)
    {
        if (Buffer_==Buffer)
            return; //Nothing to do
        Buffer=Buffer_;
        Buffer_Size=Buffer_Size_Init=Buffer_Size_BeforeLastCall=Size_*8; //Size is in bits
        LastByte_Size=0;
        BufferUnderRun=Buffer_Size?false:true;
        BookMark=false;
    };

    virtual int32u Get (size_t HowMany)
    {
        size_t ToReturn;
        static const int32u Mask[33]={
          0x00000000,
          0x00000001, 0x00000003, 0x00000007, 0x0000000f,
          0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
          0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
          0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
          0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff,
          0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,
          0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff,
          0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff,
        };

        if (HowMany==0 || HowMany>32)
            return 0;
        if ((size_t)HowMany>Buffer_Size+LastByte_Size)
        {
            Buffer_Size=0;
            LastByte_Size=0;
            BufferUnderRun=true;
            return 0;
        }

        Buffer_Size_BeforeLastCall=Buffer_Size+LastByte_Size;

        if (HowMany<=LastByte_Size)
        {
            LastByte_Size-=HowMany;
            ToReturn=LastByte>>LastByte_Size;
        }
        else
        {
            size_t NewBits=HowMany-LastByte_Size;
            if (NewBits==32)
                ToReturn=0;
            else
                ToReturn=LastByte<<NewBits;
            switch ((NewBits-1)/8)
            {
                case 3 :    NewBits-=8;
                            ToReturn |= ((size_t)*Buffer) << NewBits;
                            Buffer++;
                            Buffer_Size-=8;
                            [[fallthrough]];
                case 2 :    NewBits-=8;
                            ToReturn |= ((size_t)*Buffer) << NewBits;
                            Buffer++;
                            Buffer_Size-=8;
                            [[fallthrough]];
                case 1 :    NewBits-=8;
                            ToReturn |= ((size_t)*Buffer) << NewBits;
                            Buffer++;
                            Buffer_Size-=8;
                            [[fallthrough]];
                case 0 :
                            LastByte=*Buffer;
                            Buffer++;
            }
            LastByte_Size=MIN(8, Buffer_Size)-NewBits;
            Buffer_Size -=MIN(8, Buffer_Size);
            ToReturn|=(LastByte>>LastByte_Size)&Mask[NewBits];
        }
        return (int32u)(ToReturn&Mask[HowMany]);
    };

    bool  GetB ()
    {
        return Get(1)?true:false;
    }

    int8u  Get1 (size_t HowMany)
    {
        return (int8u )Get(HowMany);
    }

    int16u Get2 (size_t HowMany)
    {
        return (int16u)Get(HowMany);
    }

    int32u Get4 (size_t HowMany)
    {
        return (int32u)Get(HowMany);
    }

    int64u Get8 (size_t HowMany)
    {
        if (HowMany>64)
            return 0; //Not supported
        size_t HowMany1, HowMany2;
        int64u Value1, Value2;
        if (HowMany>32)
            HowMany1=HowMany-32;
        else
            HowMany1=0;
        HowMany2=HowMany-HowMany1;
        Value1=Get(HowMany1);
        Value2=Get(HowMany2);
        if (BufferUnderRun)
            return 0;
        return Value1*0x100000000LL+Value2;
    }

    virtual void Skip (size_t HowMany)
    {
        if (HowMany==0)
            return;
        if (HowMany>32) //Algorithm is only for <=32 bits
        {
            do
            {
                Skip(32);
                HowMany-=32;
            }
            while(HowMany>32);
            if (HowMany)
                Skip(HowMany);
            return;
        }
        if ((size_t)HowMany>Buffer_Size+LastByte_Size)
        {
            Buffer_Size=0;
            LastByte_Size=0;
            BufferUnderRun=true;
            return;
        }

        Buffer_Size_BeforeLastCall=Buffer_Size+LastByte_Size;

        if (HowMany<=LastByte_Size)
            LastByte_Size-=HowMany;
        else
        {
            size_t NewBits=HowMany-LastByte_Size;
            switch ((NewBits-1)/8)
            {
                case 3 :    NewBits-=8;
                            Buffer++;
                            Buffer_Size-=8;
                            [[fallthrough]];
                case 2 :    NewBits-=8;
                            Buffer++;
                            Buffer_Size-=8;
                            [[fallthrough]];
                case 1 :    NewBits-=8;
                            Buffer++;
                            Buffer_Size-=8;
                            [[fallthrough]];
                case 0 :
                            LastByte=*Buffer;
                            Buffer++;
            }
            LastByte_Size=MIN(8, Buffer_Size)-NewBits;
            Buffer_Size -=MIN(8, Buffer_Size);
        }
    };

    void SkipB ()
    {
        Skip(1);
    }

    void Skip1 (size_t HowMany)
    {
        Skip(HowMany);
    }

    void Skip2 (size_t HowMany)
    {
        Skip(HowMany);
    }

    void Skip4 (size_t HowMany)
    {
        Skip(HowMany);
    }

    void Skip8 (size_t HowMany)
    {
        if (HowMany>64)
            return; //Not supported
        size_t HowMany1, HowMany2;
        if (HowMany>32)
            HowMany1=HowMany-32;
        else
            HowMany1=0;
        HowMany2=HowMany-HowMany1;
        Skip(HowMany1);
        Skip(HowMany2);
    }

    int32u Peek(size_t HowMany)
    {
        BookMarkPos(true);
        int32u ToReturn=Get(HowMany);
        BookMarkPos(false);
        return ToReturn;
    }

    bool   PeekB()
    {
        return Peek(1)?true:false;
    }

    int8u  Peek1(size_t HowMany)
    {
        return (int8u )Peek(HowMany);
    }

    int16u Peek2(size_t HowMany)
    {
        return (int16u)Peek(HowMany);
    }

    int32u Peek4(size_t HowMany)
    {
        return (int32u)Peek(HowMany);
    }

    int32u Peek3(size_t HowMany)
    {
        return (int32u)Peek(HowMany);
    }

    int64u Peek8(size_t HowMany)
    {
        return (int64u)Peek(HowMany);
    }

    void BookMarkPos(bool ToSet)
    {
        if (ToSet)
        {
            BookMark=1;
            Buffer_BookMark=Buffer;
            Buffer_Size_BookMark=Buffer_Size;
            LastByte_BookMark=LastByte;
            LastByte_Size_BookMark=LastByte_Size;
            BufferUnderRun_BookMark=BufferUnderRun;
        }
        else
        {
            BookMark=0;
            Buffer=Buffer_BookMark;
            Buffer_Size=Buffer_Size_BookMark;
            LastByte=LastByte_BookMark;
            LastByte_Size=LastByte_Size_BookMark;
            BufferUnderRun=BufferUnderRun_BookMark;
        }
    };

    virtual int32u Remain () //How many bits remain?
    {
        return (int32u)(Buffer_Size+LastByte_Size);
    };

    virtual void Byte_Align()
    {
        Get(LastByte_Size);
    };

    virtual size_t Offset_Get()
    {
        if (BufferUnderRun)
            return 0;
        return (Buffer_Size_Init-Buffer_Size)/8;
    };

    virtual size_t BitOffset_Get()
    {
        if (BufferUnderRun)
            return 0;
        return LastByte_Size;
    };

    virtual size_t OffsetBeforeLastCall_Get()
    {
        if (BufferUnderRun)
            return 0;
        return (Buffer_Size_Init-Buffer_Size_BeforeLastCall)/8;
    };

private :
    const int8u*    Buffer;
    size_t          Buffer_Size;
    size_t          Buffer_Size_Init;
    size_t          Buffer_Size_BeforeLastCall;
    size_t          LastByte;
    size_t          LastByte_Size;
    bool            BufferUnderRun;

    bool            BookMark;
    const int8u*    Buffer_BookMark;
    size_t          Buffer_Size_BookMark;
    size_t          LastByte_BookMark;
    size_t          LastByte_Size_BookMark;
    bool            BufferUnderRun_BookMark;
};

} //namespace ZenLib
#endif

/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_TimeCodeH
#define MediaInfo_TimeCodeH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Unknown
//***************************************************************************

class TimeCode
{
public:
    //constructor/Destructor
    TimeCode ();
    TimeCode (int8u Hours, int8u Minutes, int8u Seconds, int8u Frames, int8u FramesPerSecond, bool DropFrame, bool MustUseSecondField=false, bool IsSecondField=false);
    TimeCode (int64s Frames, int8u FramesPerSecond, bool DropFrame, bool MustUseSecondField=false, bool IsSecondField_=false);
    TimeCode(const char* Value, size_t Length); // return false if all fine
    TimeCode(const char* Value) { *this = TimeCode(Value, strlen(Value)); }
    TimeCode(const string& Value) { *this = TimeCode(Value.c_str(), Value.size()); }

    //Operators
    TimeCode& operator +=(const TimeCode& b)
    {
        return operator+=(b.ToFrames());
    }
    TimeCode& operator +=(const int64s Frames)
    {
        FromFrames(ToFrames()+Frames);
        return *this;
    }
    TimeCode& operator -=(const TimeCode& b)
    {
        return operator-=(b.ToFrames());
    }
    TimeCode& operator -=(const int64s)
    {
        FromFrames(ToFrames()-Frames);
        return *this;
    }
    TimeCode &operator ++()
    {
        PlusOne();
        return *this;
    }
    TimeCode operator ++(int)
    {
        PlusOne();
        return *this;
    }
    TimeCode &operator --()
    {
        MinusOne();
        return *this;
    }
    TimeCode operator --(int)
    {
        MinusOne();
        return *this;
    }
    friend TimeCode operator +(TimeCode a, const TimeCode& b)
    {
        a+=b;
        return a;
    }
    friend TimeCode operator +(TimeCode a, const int64s b)
    {
        a+=b;
        return a;
    }
    friend TimeCode operator -(TimeCode a, const TimeCode& b)
    {
        a-=b;
        return a;
    }
    friend TimeCode operator -(TimeCode a, const int64s b)
    {
        a-=b;
        return a;
    }
    bool operator== (const TimeCode &tc) const
    {
        return Hours                ==tc.Hours
            && Minutes              ==tc.Minutes
            && Seconds              ==tc.Seconds
            && Frames               ==tc.Frames
            && FramesPerSecond      ==tc.FramesPerSecond
            && DropFrame            ==tc.DropFrame
            && MustUseSecondField   ==tc.MustUseSecondField
            && IsSecondField        ==tc.IsSecondField;
    }
    bool operator!= (const TimeCode &tc) const
    {
        return !(*this == tc);
    }

    //Helpers
    bool IsValid() const
    {
        return FramesPerSecond && HasValue();
    }
    bool HasValue() const
    {
        return Hours!=(int8u)-1;
    }
    void PlusOne();
    void MinusOne();
    bool FromString(const char* Value, size_t Length); // return false if all fine
    bool FromString(const char* Value) {return FromString(Value, strlen(Value));}
    bool FromString(const string& Value) {return FromString(Value.c_str(), Value.size());}
    bool FromFrames(int64s Value);
    string ToString() const;
    int64s ToFrames() const;
    int64s ToMilliseconds() const;

public:
    int8u Hours;
    int8u Minutes;
    int8u Seconds;
    int8u Frames;
    int32s MoreSamples;
    int32s MoreSamples_Frequency;
    bool  FramesPerSecond_Is1001;
    int8u FramesPerSecond;
    bool  DropFrame;
    bool  MustUseSecondField;
    bool  IsSecondField;
    bool  IsNegative;
};

Ztring Date_MJD(int16u Date);
Ztring Time_BCD(int32u Time);

} //NameSpace

#endif

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
#include <cstring>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class bitset8
//***************************************************************************

#if __cplusplus >= 201400 || (_MSC_VER >= 1910 && _MSVC_LANG >= 201400)
    #define constexpr14 constexpr
#else
    #define constexpr14
#endif
#if __cplusplus >= 202000 || (_MSC_VER >= 1910 && _MSVC_LANG >= 202000)
    #define constexpr20 constexpr
#else
    #define constexpr20
#endif

class bitset8
{
public:
    class proxy
    {
        friend bitset8;

    public:
        constexpr20 ~proxy() noexcept {}

        constexpr14 proxy& operator=(bool Value) noexcept {
            ref->set(pos, Value);
            return *this;
        }

        constexpr14 proxy& operator=(const proxy& p) noexcept {
            ref->set(pos, static_cast<bool>(p));
            return *this;
        }

        constexpr14 operator bool() const noexcept {
            return ref->test(pos);
        }

    private:
        constexpr14 proxy(bitset8* _ref, size_t _pos) : ref(_ref), pos(_pos) {}

        bitset8* const ref;
        size_t const pos;
    };

    constexpr14 bitset8() : stored(0) {}
    constexpr14 bitset8(int8u val) : stored(val) {}

    constexpr14 bitset8& reset() noexcept
    {
        stored=0;
        return *this;
    }

    constexpr14 bitset8& reset(size_t pos) noexcept
    {
        stored&=~(1<<pos);
        return *this;
    }

    constexpr14 bitset8& set(size_t pos) noexcept
    {
        stored|=(1<<pos);
        return *this;
    }

    constexpr14 bitset8& set(size_t pos, bool val) noexcept
    {
        return val?set(pos):reset(pos);
    }

    constexpr14 bool test(size_t pos) const noexcept
    {
        return (stored>>pos)&1;
    }

    constexpr20 proxy operator[](size_t pos) noexcept
    {
        return proxy(this, pos);
    }

    constexpr14 bool operator[](size_t pos) const noexcept
    {
        return test(pos);
    }

    constexpr14 int8u to_int8u() const noexcept
    {
        return stored;
    }

    constexpr14 explicit operator bool() const noexcept
    {
        return stored;
    }

private:
    int8u stored=0;
};

inline int8u operator | (const bitset8 a, const bitset8 b) noexcept
{
    return a.to_int8u() | b.to_int8u();
}

inline int8u operator & (const bitset8 a, const bitset8 b) noexcept
{
    return a.to_int8u() & b.to_int8u();
}



//***************************************************************************
// Class TimeCode
//***************************************************************************

class TimeCode
{
public:
    //constructor/Destructor
    TimeCode ();
    TimeCode (int32u Hours, int8u Minutes, int8u Seconds, int32u Frames, int32u FramesMax, bool DropFrame, bool MustUseSecondField=false, bool IsSecondField=false);
    TimeCode (int64s Frames, int32u FramesMax, bool DropFrame, bool MustUseSecondField=false, bool IsSecondField_=false);
    TimeCode(const char* Value, size_t Length); // return false if all fine
    TimeCode(const char* Value) { *this = TimeCode(Value, strlen(Value)); }
    TimeCode(const string& Value) { *this = TimeCode(Value.c_str(), Value.size()); }

    //Operators
    TimeCode& operator +=(const TimeCode& b)
    {
        int64u FrameRate1=GetFramesMax();
        int64u FrameRate2=b.GetFramesMax();
        if (FrameRate1==FrameRate2)
        {
            bool IsTimeSav=Flags.test(IsTime);
            *this+=b.ToFrames();
            Flags.set(IsTime, IsTimeSav);
            return *this;
        }
        FrameRate1++;
        FrameRate2++;
        int64u Frames1=ToFrames();
        int64u Frames2=b.ToFrames();
        int64u Result=Frames1*FrameRate2+Frames2*FrameRate1;
        Result=(Result+FrameRate2/2)/FrameRate2;
        bool IsTimeSav=Flags.test(IsTime);
        FromFrames(Result);
        Flags.set(IsTime, IsTimeSav);
        return *this;
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
        return Hours==tc.Hours
            && Minutes==tc.Minutes
            && Seconds==tc.Seconds
            && Frames==tc.Frames;
    }
    bool operator!= (const TimeCode &tc) const
    {
        return !(*this == tc);
    }
    bool operator< (const TimeCode& tc) const
    {
        int64u Total1=((int64u)Hours)  <<16
                    | ((int64u)Minutes)<< 8
                    | ((int64u)Seconds);
        int64u Total2=((int64u)tc.Hours)  <<16
                    | ((int64u)tc.Minutes)<< 8
                    | ((int64u)tc.Seconds);
        if (Total1==Total2)
        {
            if (FramesMax==tc.FramesMax)
                return Frames<tc.Frames;
            int64u Mix1=((int64s)Frames)*(((int64s)tc.FramesMax)+1);
            int64u Mix2=((int64s)tc.Frames)*(((int64s)FramesMax)+1);
            return Mix1<Mix2;
        }
        return Total1<Total2;
    }
    bool operator> (const TimeCode& tc) const
    {
        int64u Total1 = ((int64u)Hours) << 16
                    | ((int64u)Minutes)<< 8
                    | ((int64u)Seconds);
        int64u Total2=((int64u)tc.Hours)  <<16
                    | ((int64u)tc.Minutes)<< 8
                    | ((int64u)tc.Seconds);
        if (Total1==Total2)
        {
            if (FramesMax==tc.FramesMax)
                return Frames>tc.Frames;
            int64u Mix1=((int64s)Frames)*(((int64s)tc.FramesMax)+1);
            int64u Mix2=((int64s)tc.Frames)*(((int64s)FramesMax)+1);
            return Mix1>Mix2;
        }
        return Total1>Total2;
    }

    //Helpers
    bool HasValue() const
    {
        return Flags.test(IsValid);
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

    int32u GetHours() const { return Hours; }
    void SetHours(int32u Value) { Hours=Value; }
    int8u GetMinutes() const { return Minutes; }
    void SetMinutes(int8u Value) { Minutes=Value; }
    int8u GetSeconds() const { return Seconds; }
    void SetSeconds(int8u Value) { Seconds=Value; }
    int32u GetFrames() const { return Frames; }
    void SetFrames(int32u Value) { Frames=Value; }
    int32u GetFramesMax() const { return FramesMax; }
    void SetFramesMax(int32u Value=0) { FramesMax=Value; }
    bool GetDropFrame() const { return Flags.test(DropFrame); }
    void SetDropFrame(bool Value=true) { Flags.set(DropFrame, Value); }
    bool GetNegative() const { return Flags.test(IsNegative); }
    void SetNegative(bool Value=true) { Flags.set(IsNegative, Value); }
    bool Get1001() const { return Flags.test(FramesPerSecond_Is1001); }
    void Set1001(bool Value=true) { Flags.set(FramesPerSecond_Is1001, Value); }
    bool GetMustUseSecondField() const { return Flags.test(MustUseSecondField); }
    void SetMustUseSecondField(bool Value=true) { Flags.set(MustUseSecondField, Value); }
    bool GetIsTime() const { return Flags.test(IsTime); }
    void SetIsTime(bool Value=true) { Flags.set(IsTime, Value); }
    bool GetIsValid() const { return Flags.test(IsValid); }
    void SetIsValid(bool Value=true) { Flags.set(IsValid, Value); }
    float GetFrameRate() { return (FramesMax+1)/((Flags.test(DropFrame) || Flags.test(FramesPerSecond_Is1001))?1.001:1.000);}

private:
    int32u Frames;
    int32u FramesMax;
    int32u Hours;
    int8u Minutes;
    int8u Seconds;
    enum flag
    {
        DropFrame,
        FramesPerSecond_Is1001,
        MustUseSecondField,
        IsSecondField,
        IsNegative,
        HasNoFramesInfo,
        IsTime,
        IsValid,
        Flag_Max
    };
    bitset8 Flags;
};

Ztring Date_MJD(int16u Date);
Ztring Time_BCD(int32u Time);

} //NameSpace

#endif

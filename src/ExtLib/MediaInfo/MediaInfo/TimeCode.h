/* MIT License
Copyright MediaArea.net SARL
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//---------------------------------------------------------------------------
#ifndef ZenLib_TimeCodeH
#define ZenLib_TimeCodeH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include <cstdint>
#include <cstring>
#include <string>
//---------------------------------------------------------------------------

namespace ZenLib
{

//***************************************************************************
// Class bitset8
//***************************************************************************

#if __cplusplus >= 201400 || (defined(_MSC_VER) && _MSC_VER >= 1910 && _MSVC_LANG >= 201400)
    #define constexpr14 constexpr
#else
    #define constexpr14
#endif
#if __cplusplus >= 202000 || (defined(_MSC_VER) && _MSC_VER >= 1910 && _MSVC_LANG >= 202000)
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
    constexpr14 bitset8(uint8_t val) : stored(val) {}

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

    constexpr14 uint8_t to_int8u() const noexcept
    {
        return stored;
    }

    constexpr14 explicit operator bool() const noexcept
    {
        return stored;
    }

private:
    uint8_t stored=0;
};

inline uint8_t operator | (const bitset8 a, const bitset8 b) noexcept
{
    return a.to_int8u() | b.to_int8u();
}

inline uint8_t operator & (const bitset8 a, const bitset8 b) noexcept
{
    return a.to_int8u() & b.to_int8u();
}



//***************************************************************************
// Class TimeCode
//***************************************************************************

class TimeCode
{
public:
    typedef bitset8 flags_type;
    class flags : public flags_type
    {
    public:
        flags() : flags_type() {}
        flags& DropFrame(bool Value = true) { set(IsDrop_, Value); return *this; }
        flags& FPS1001(bool Value = true) { set(Is1001_, Value); return *this; }
        flags& Field(bool Value = true) { set(IsField_, Value); return *this; }
        flags& Wrapped24Hours(bool Value = true) { set(IsWrapped24Hours_, Value); return *this; }
        flags& Negative(bool Value = true) { set(IsNegative_, Value); return *this; }
        flags& Timed(bool Value = true) { set(IsTimed_, Value); return *this; }

    private:
        enum flag
        {
            IsDrop_,
            Is1001_,
            IsField_,
            IsWrapped24Hours_,
            IsNegative_,
            IsTimed_,
            IsValid_,
            IsUndefined_,
        };
        bool IsDropFrame() const { return test(IsDrop_); }
        bool Is1001fps() const { return test(Is1001_); }
        bool IsField() const { return test(IsField_); }
        bool IsWrapped24Hours() const { return test(IsWrapped24Hours_); }
        bool IsNegative() const { return test(IsNegative_); }
        bool IsTimed() const { return test(IsTimed_); }
        flags& SetValid(bool Value = true) { set(IsValid_, Value); return *this; }
        bool IsValid() const { return test(IsValid_); }
        flags& SetUndefined(bool Value = true) { set(IsUndefined_, Value); return *this; }
        bool IsUndefined() const { return test(IsUndefined_); }
        flags(uint8_t);
        friend class TimeCode;
    };

    enum rounding : uint8_t
    {
        Nearest,
        Floor,
        Ceil,
    };

    class string_view
    {
    public:
        constexpr string_view(const char* s, size_t count) : s_(s), count_(count) {}
        constexpr const char* data() const noexcept { return s_; }
        constexpr size_t size() const noexcept { return count_; }
    private:
        const char* s_;
        size_t count_;
    };

    //constructor/Destructor
    TimeCode();
    TimeCode(uint32_t Hours, uint8_t Minutes, uint8_t Seconds, uint32_t Frames = 0, uint32_t FramesMax = 0, flags Flags = {});
    TimeCode(uint64_t FrameCount, uint32_t FramesMax = 0, flags Flags = {});
    TimeCode(int64_t FrameCount, uint32_t FramesMax = 0, flags Flags = {});
    TimeCode(uint32_t FrameCount, uint32_t FramesMax = 0, flags Flags = {}) : TimeCode((uint64_t)FrameCount, FramesMax, Flags) {}
    TimeCode(int32_t FrameCount, uint32_t FramesMax = 0, flags Flags = {}) : TimeCode((int64_t)FrameCount, FramesMax, Flags) {}
    TimeCode(double Seconds, uint32_t FramesMax = 0, flags Flags = {}, bool Truncate = false, bool TimeIsDropFrame = false);
    TimeCode(float Seconds, uint32_t FramesMax = 0, flags Flags = {}, bool Truncate = false, bool TimeIsDropFrame = false) : TimeCode((double)Seconds, FramesMax, Flags, Truncate, TimeIsDropFrame) {}
    TimeCode(const string_view& Value, uint32_t FramesMax = 0, flags Flags = {}, bool Ignore1001FromDropFrame = false);
    TimeCode(const char* Value, uint32_t FramesMax = 0, flags Flags = {}, bool Ignore1001FromDropFrame = false) : TimeCode(string_view(Value, std::strlen(Value)), FramesMax, Flags, Ignore1001FromDropFrame) {}
    TimeCode(const std::string& Value, uint32_t FramesMax = 0, flags Flags = {}, bool Ignore1001FromDropFrame = false) : TimeCode(string_view(Value.c_str(), Value.size()), FramesMax, Flags, Ignore1001FromDropFrame) {}

    //Operators
    TimeCode& operator +=(const TimeCode& b);
    TimeCode& operator +=(const int64_t FrameCount)
    {
        FromFrames(ToFrames() + FrameCount);
        return *this;
    }
    TimeCode& operator -=(const TimeCode& b);
    TimeCode& operator -=(const int64_t FrameCount)
    {
        FromFrames(ToFrames() - FrameCount);
        return *this;
    }
    TimeCode& operator ++()
    {
        IsNegative() ? MinusOne() : PlusOne();
        return *this;
    }
    TimeCode operator ++(int)
    {
        IsNegative() ? MinusOne() : PlusOne();
        return *this;
    }
    TimeCode& operator --()
    {
        IsNegative() ? PlusOne() : MinusOne();
        return *this;
    }
    TimeCode operator --(int)
    {
        IsNegative() ? PlusOne() : MinusOne();
        return *this;
    }
    friend TimeCode operator +(TimeCode a, const TimeCode& b)
    {
        a += b;
        return a;
    }
    friend TimeCode operator +(TimeCode a, const int64_t b)
    {
        a += b;
        return a;
    }
    friend TimeCode operator -(TimeCode a, const TimeCode& b)
    {
        a -= b;
        return a;
    }
    friend TimeCode operator -(TimeCode a, const int64_t b)
    {
        a -= b;
        return a;
    }
    bool operator== (const TimeCode& tc) const
    {
        if (!IsSet() && !tc.IsSet())
            return true;
        return GetHours() == tc.GetHours()
            && GetMinutes() == tc.GetMinutes()
            && GetSeconds() == tc.GetSeconds()
            && GetFrames() * (tc.GetFramesMax() + 1) == tc.GetFrames() * (GetFramesMax() + 1);
    }
    bool operator!= (const TimeCode& tc) const
    {
        return !(*this == tc);
    }
    bool operator< (const TimeCode& tc) const
    {
        uint64_t Total1 = ((uint64_t)GetHours()) << 16
            | ((uint64_t)GetMinutes()) << 8
            | ((uint64_t)GetSeconds());
        uint64_t Total2 = ((uint64_t)tc.GetHours()) << 16
            | ((uint64_t)tc.GetMinutes()) << 8
            | ((uint64_t)tc.GetSeconds());
        if (Total1 == Total2)
        {
            if (GetFramesMax() == tc.GetFramesMax())
                return GetFrames() < tc.GetFrames();
            uint64_t Mix1 = ((int64_t)GetFrames()) * (((int64_t)tc.GetFramesMax()) + 1);
            uint64_t Mix2 = ((int64_t)tc.GetFrames()) * (((int64_t)GetFramesMax()) + 1);
            return Mix1 < Mix2;
        }
        return Total1 < Total2;
    }
    bool operator> (const TimeCode& tc) const
    {
        uint64_t Total1 = ((uint64_t)GetHours()) << 16
            | ((uint64_t)GetMinutes()) << 8
            | ((uint64_t)GetSeconds());
        uint64_t Total2 = ((uint64_t)tc.GetHours()) << 16
            | ((uint64_t)tc.GetMinutes()) << 8
            | ((uint64_t)tc.GetSeconds());
        if (Total1 == Total2)
        {
            if (GetFramesMax() == tc.GetFramesMax())
                return GetFrames() > tc.GetFrames();
            uint64_t Mix1 = ((int64_t)GetFrames()) * (((int64_t)tc.GetFramesMax()) + 1);
            uint64_t Mix2 = ((int64_t)tc.GetFrames()) * (((int64_t)GetFramesMax()) + 1);
            return Mix1 > Mix2;
        }
        return Total1 > Total2;
    }
    bool operator<= (const TimeCode& tc) const
    {
        uint64_t Total1 = ((uint64_t)GetHours()) << 16
            | ((uint64_t)GetMinutes()) << 8
            | ((uint64_t)GetSeconds());
        uint64_t Total2 = ((uint64_t)tc.GetHours()) << 16
            | ((uint64_t)tc.GetMinutes()) << 8
            | ((uint64_t)tc.GetSeconds());
        if (Total1 == Total2)
        {
            if (GetFramesMax() == tc.GetFramesMax())
                return GetFrames() <= tc.GetFrames();
            uint64_t Mix1 = ((int64_t)GetFrames()) * (((int64_t)tc.GetFramesMax()) + 1);
            uint64_t Mix2 = ((int64_t)tc.GetFrames()) * (((int64_t)GetFramesMax()) + 1);
            return Mix1 <= Mix2;
        }
        return Total1 < Total2;
    }
    bool operator>= (const TimeCode& tc) const
    {
        uint64_t Total1 = ((uint64_t)GetHours()) << 16
            | ((uint64_t)GetMinutes()) << 8
            | ((uint64_t)GetSeconds());
        uint64_t Total2 = ((uint64_t)tc.GetHours()) << 16
            | ((uint64_t)tc.GetMinutes()) << 8
            | ((uint64_t)tc.GetSeconds());
        if (Total1 == Total2)
        {
            if (GetFramesMax() == tc.GetFramesMax())
                return GetFrames() >= tc.GetFrames();
            uint64_t Mix1 = ((int64_t)GetFrames()) * (((int64_t)tc.GetFramesMax()) + 1);
            uint64_t Mix2 = ((int64_t)tc.GetFrames()) * (((int64_t)GetFramesMax()) + 1);
            return Mix1 >= Mix2;
        }
        return Total1 > Total2;
    }

    int FromString(const string_view& Value, bool Ignore1001FromDropFrame = false);
    int FromString(const char* Value, bool Ignore1001FromDropFrame = false) { return FromString(string_view(Value, strlen(Value)), Ignore1001FromDropFrame); }
    int FromString(const std::string& Value, bool Ignore1001FromDropFrame = false) { return FromString(string_view(Value.c_str(), Value.size()), Ignore1001FromDropFrame); }
    int FromFrames(uint64_t Value);
    int FromFrames(int64_t Value);
    int FromSeconds(double Value, bool Truncate = false, bool TimeIsDropFrame = false);

    std::string ToString() const;
    int64_t ToFrames() const;
    int64_t ToMilliseconds() const;
    double ToSeconds(bool TimeIsDropFrame = false) const;
    TimeCode ToRescaled(uint32_t FramesMax = 0, flags Flags = {}, rounding Rounding = Nearest) const;

    uint32_t GetHours() const { return Hours_; }
    void SetHours(int32_t Value) { Hours_ = Value; }
    uint8_t GetMinutes() const { return Minutes_; }
    void SetMinutes(uint8_t Value) { Minutes_ = Value; }
    uint8_t GetSeconds() const { return Seconds_; }
    void SetSeconds(uint8_t Value) { Seconds_ = Value; }
    uint32_t GetFrames() const { return Frames_; }
    void SetFrames(int64_t Value) { Frames_ = Value; }

    uint32_t GetFramesMax() const { return FramesMax_; }
    void SetFramesMax(uint32_t Value = 0) { FramesMax_ = Value; }
    float GetFrameRate() { return ((uint64_t)GetFramesMax() + 1) / (Is1001fps() ? 1.001 : 1.000); }

    TimeCode& SetDropFrame(bool Value = true) { Flags_.DropFrame(Value); return *this; }
    bool IsDropFrame() const { return Flags_.IsDropFrame(); }
    TimeCode& Set1001fps(bool Value = true) { Flags_.FPS1001(Value); return *this; }
    bool Is1001fps() const { return Flags_.Is1001fps(); }
    TimeCode& SetField(bool Value = true) { Flags_.Field(Value); return *this; }
    bool IsField() const { return Flags_.IsField(); }
    TimeCode& SetWrapped24Hours(bool Value = true) { Flags_.Wrapped24Hours(Value); return *this; }
    bool IsWrapped24Hours() const { return Flags_.IsWrapped24Hours(); }
    TimeCode& SetTimed(bool Value = true) { Flags_.Timed(Value); return *this; }
    bool IsTimed() const { return Flags_.IsTimed(); }
    TimeCode& SetNegative(bool Value = true) { Flags_.Negative(Value); return *this; }
    bool IsNegative() const { return Flags_.IsNegative(); }
    bool IsValid() const { return Flags_.IsValid(); }
    bool IsUndefined() const { return Flags_.IsUndefined(); }
    bool IsSet() const { return IsValid() && !IsUndefined(); }

    static flags DropFrame(bool Value = true) { return flags().DropFrame(Value); }
    static flags FPS1001(bool Value = true) { return flags().FPS1001(Value); }
    static flags Field(bool Value = true) { return flags().Field(Value); }
    static flags Wrapped24Hours(bool Value = true) { return flags().Wrapped24Hours(Value); }
    static flags Negative(bool Value = true) { return flags().Negative(Value); }
    static flags Timed(bool Value = true) { return flags().Timed(Value); }

private:
    uint32_t Frames_;
    uint32_t FramesMax_;
    int32_t Hours_;
    uint8_t Minutes_;
    uint8_t Seconds_;
    flags Flags_;

    TimeCode& SetValid(bool Value = true) { Flags_.set(flags::IsValid_, Value); return *this; }
    TimeCode& SetUndefined(bool Value = true) { Flags_.set(flags::IsUndefined_, Value); return *this; }

    void PlusOne();
    void MinusOne();
};

std::string Date_MJD(uint16_t Date);
std::string Time_BCD(uint32_t Time);

} //NameSpace

#endif
